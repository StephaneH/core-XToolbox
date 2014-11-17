/*
* This file is part of Wakanda software, licensed by 4D under
*  (i) the GNU General Public License version 3 (GNU GPL v3), or
*  (ii) the Affero General Public License version 3 (AGPL v3) or
*  (iii) a commercial license.
* This file remains the exclusive property of 4D and/or its licensors
* and is protected by national and international legislations.
* In any event, Licensee's compliance with the terms and conditions
* of the applicable license constitutes a prerequisite to any use of this file.
* Except as otherwise expressly stated in the applicable license,
* such license does not include any other license or rights on this file,
* 4D's and/or its licensors' trademarks and/or other proprietary rights.
* Consequently, no title, copyright or other proprietary rights
* other than those specified in the applicable license is granted.
*/
#include "VKernelPrecompiled.h"
#include "XMacFiber.h"

#include <pthread.h>
#include <mach/vm_map.h>


//===============================================================================================

static void* ApparentlyUselessThread( void *inTask)
{
	while(true)
		::YieldToAnyThread();
}


class VMacFiber
{
public:
								VMacFiber( void *inParam):fParam( inParam)		{;}
	virtual						~VMacFiber()									{;}

	static	VMacFiber*			GetCurrent()									{ return static_cast<VMacFiber*>( ::pthread_getspecific( sKey)); }
	static	void				SetCurrent( VMacFiber *inFiber)					{ int r = ::pthread_setspecific( sKey, inFiber); xbox_assert( r == 0); }

			void*				GetParam() const								{ return fParam;}

	virtual	void				Run() = 0;
	virtual	bool				IsValid() const	= 0;

	static	void				Init()
		{
			if (sKey == 0)
			{
				::pthread_key_create( &sKey, NULL);

				CFStringRef value = (CFStringRef) ::CFBundleGetValueForInfoDictionaryKey( ::CFBundleGetMainBundle(), CFSTR( "ThreadingModel"));
				if (value != NULL)
				{
					if (::CFStringCompare( value, CFSTR( "longjmp"), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
						sThreadingModel = 0;
					else if (::CFStringCompare( value, CFSTR( "NewThread"), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
						sThreadingModel = 1;
				}
				
				if (false)	//sThreadingModel == 1)
				{
					// Apple Bug rdar://4433706
					ThreadID uselessThreadID = kNoThreadID;
					OSErr error = ::NewThread( kCooperativeThread, ApparentlyUselessThread, NULL, 64*1024L, kCreateIfNeeded, NULL, &uselessThreadID);
					xbox_assert(error == noErr);
				}
			}
		}


	static VMacFiber*			ConvertThreadToFiber( void *inParam);
	static VMacFiber*			CreateFiber( size_t inStackReserveSize, LPFIBER_START_ROUTINE inRunProc, void *inParam);

private:
	static	pthread_key_t				sKey;
	static	sLONG						sThreadingModel;
			void*						fParam;
};


pthread_key_t	VMacFiber::sKey = 0;
sLONG			VMacFiber::sThreadingModel = 0;	// longjmp by default


//===============================================================================================

#if !__LP64__ && !defined(__clang__)

class VMacFiber_longjmp : public VMacFiber
{
public:
								// Convert main thread to fiber
								VMacFiber_longjmp( void *inParam);

								// Create a new fiber
								VMacFiber_longjmp( size_t inStackSize, LPFIBER_START_ROUTINE inProc, void *inParam);

								~VMacFiber_longjmp();

	virtual	bool				IsValid() const									{ return fIsMain || (fStackAddress != 0);}
			char*				GetStackAddress() const							{ return (char *) fStackAddress;}
			
	virtual	void				Run();

private:
			void*						fParamTemp;	// duplicates fParam for conveniency in assembly code
			jmp_buf						fRegisters;
			vm_address_t				fStackAddress;
			size_t						fStackSize;
			LPFIBER_START_ROUTINE		fProc;
			bool						fIsMain;
			bool						fInited;
};



//===============================================================================================


VMacFiber_longjmp::VMacFiber_longjmp( void *inParam):VMacFiber( inParam)
{
	// main fiber
	fIsMain = true;
	fInited = true;
	fStackAddress = 0;
	fStackSize = 1024*1024*1024L;	// 1Mo?
	fProc = NULL;
}


VMacFiber_longjmp::VMacFiber_longjmp( size_t inStackSize, LPFIBER_START_ROUTINE inProc, void *inParam):VMacFiber( inParam)
{
	fIsMain = false;
	fInited = false;
	fProc = inProc;

	// stack allocation with one guard page	
	fStackSize = ((inStackSize + vm_page_size - 1)/ vm_page_size) * vm_page_size;

	xbox_assert(fStackSize >= PTHREAD_STACK_MIN); // 8192

	// allocate a guard page
	kern_return_t kr = vm_allocate(mach_task_self(), (vm_address_t *) &fStackAddress, fStackSize + vm_page_size, VM_MAKE_TAG(VM_MEMORY_STACK)| TRUE);
	if (kr != KERN_SUCCESS)
	{
		fStackAddress = 0;
	}
	else
	{
	    /* The guard page is at the lowest address */
	    /* The stack base is the highest address */
		kr = vm_protect(mach_task_self(), fStackAddress, vm_page_size, FALSE, VM_PROT_NONE);
		fStackAddress += vm_page_size;
	}

	if (fStackAddress != 0)
	{
		long *p = (long *) (fStackAddress + fStackSize - 64);
		p[0] = p[1] = 0;
	}
}


VMacFiber_longjmp::~VMacFiber_longjmp()
{
	if (fStackAddress != 0)
	{
		kern_return_t kr = vm_deallocate(mach_task_self(), fStackAddress - vm_page_size, fStackSize + vm_page_size);
	}
}


#pragma global_optimizer off

/*
Warning : inline asm instruction 'MR', defines reserved register r1.
Mixing assembler and C code depends on the inline assembler not modifying the reserved registers directly.
Use function level assembler if you need to directly modify the reserved registers.
*/
// malheureusement CW s'en fout
#pragma suppress_warnings on


void VMacFiber_longjmp::Run()
{
	VMacFiber_longjmp *oldFiber = static_cast<VMacFiber_longjmp*>( VMacFiber_longjmp::GetCurrent());

	xbox_assert( oldFiber != NULL);	// ConvertThreadToFiber has not been called
	xbox_assert( oldFiber != this);
	
	VMacFiber_longjmp::SetCurrent( this);

//	Ptr oldStackBase = LMGetCurStackBase();
//	Ptr oldHiHeapMark = LMGetHighHeapMark();

	if (fInited)
	{
		
		if (setjmp( oldFiber->fRegisters) == 0)
			longjmp( fRegisters, 1);
	}
	else
	{
		// first time
		fInited = true;


		if (setjmp( oldFiber->fRegisters) == 0)
		{
			#if __ppc__
			fParamTemp = GetParam();
			register Ptr stackPtr = (Ptr) fStackAddress + fStackSize - 64;
			asm {
				mr sp, stackPtr
			}
			fProc( fParamTemp);
			#elif __i386__

			// set param
			register Ptr stackPtr = (Ptr) fStackAddress + fStackSize - 64;
			* (void**) stackPtr = GetParam();
			stackPtr -= 4;	// for return address
			LPFIBER_START_ROUTINE proc = fProc;
			
			asm volatile ("movl %0, %%edx" :: "m" (proc));
			asm volatile ("movl %0, %%esp" :: "m" (stackPtr));
			asm volatile ("movl $0, %ebp");
			asm volatile ("jmp %edx");// clear frame pointer to please gdb

/*
CW syntax so much easier to read but unsupported by llvm
			asm {
				mov edx, proc

				mov esp, stackPtr
				mov ebp, 0	// clear frame pointer to please gdb

				jmp edx
			}
*/
			#else
				#error processor not supported !
			#endif
		}

	}

//	LMSetCurStackBase( oldStackBase);
//	LMSetHighHeapMark( oldHiHeapMark);
}

#pragma suppress_warnings reset

#pragma global_optimizer reset

#endif // #if __LP64__ && !defined(__clang__)

//===============================================================================================

class VMacFiber_thread : public VMacFiber
{
public:
								// Convert main thread to fiber
								VMacFiber_thread( void *inParam);

								// Create a new fiber
								VMacFiber_thread( size_t inStackSize, LPFIBER_START_ROUTINE inProc, void *inParam);

								~VMacFiber_thread()
								{
									if (!fIsMain && (fSystemID != kNoThreadID) )
										::DisposeThread( fSystemID, NULL, false);
								}

	virtual	bool				IsValid() const									{ return fSystemID != kNoThreadID;}
			
	virtual	void				Run()
								{
									VMacFiber_thread *current = static_cast<VMacFiber_thread*>( GetCurrent());
									if (current != this)
									{
										OSErr macErr;
										
										xbox_assert( !fRunning);
										xbox_assert( current->fRunning);
										current->fRunning = false;
										fRunning = true;
										
										#if 0
										ThreadState state;
										macErr = ::GetThreadState( fSystemID, &state);
										xbox_assert( (macErr == 0) && (state == kStoppedThreadState) );
										#endif

										macErr = ::SetThreadState( fSystemID, kReadyThreadState, kCurrentThreadID);
										xbox_assert(macErr == 0);

										macErr = ::SetThreadState( kCurrentThreadID, kStoppedThreadState, fSystemID);
										xbox_assert(macErr == 0);
										
										while( !current->fRunning)
										{
											macErr = ::SetThreadState( kCurrentThreadID, kStoppedThreadState, kNoThreadID);
											xbox_assert(macErr == 0);
										}

										#if 0
										macErr = ::GetThreadState( fSystemID, &state);
										xbox_assert( (macErr == 0) && (state == kStoppedThreadState) );
										#endif
										
										xbox_assert( GetCurrent() == current);
										xbox_assert( current->fRunning);
										xbox_assert( !fRunning);
									}
								}

private:
	static	pascal void*				_ThreadProc( void *inTask);
	static	pascal void					_TaskSwitchIn( unsigned long inThread, void* inTask);
	static	pascal void					_TaskSwitchOut( unsigned long inThread, void* inTask);
	static	pascal ThreadID				_TaskScheduler( SchedulerInfoRecPtr schedulerInfo);
	static	ThreadSwitchTPP				sThreadSwitchIn;
	static	ThreadSwitchTPP				sThreadSwitchOut;
	static	ThreadSchedulerTPP			sThreadScheduler;
			
			ThreadID					fSystemID;
			LPFIBER_START_ROUTINE		fProc;
			bool						fIsMain;
			bool						fInited;
			bool						fRunning;
};

ThreadSwitchTPP		VMacFiber_thread::sThreadSwitchIn = ::NewThreadSwitchUPP( _TaskSwitchIn);
ThreadSwitchTPP		VMacFiber_thread::sThreadSwitchOut = ::NewThreadSwitchUPP( _TaskSwitchOut);
ThreadSchedulerTPP	VMacFiber_thread::sThreadScheduler = ::NewThreadSchedulerUPP( _TaskScheduler);

//===============================================================================================


pascal void VMacFiber_thread::_TaskSwitchIn( unsigned long inThread, void* inTask)
{
	VMacFiber_thread* theTask = reinterpret_cast<VMacFiber_thread*>(inTask);
	xbox_assert( theTask->fRunning);
}


pascal void VMacFiber_thread::_TaskSwitchOut( unsigned long inThread, void* inTask)
{
	VMacFiber_thread* theTask = reinterpret_cast<VMacFiber_thread*>(inTask);
	xbox_assert( !theTask->fRunning);
}


pascal ThreadID VMacFiber_thread::_TaskScheduler( SchedulerInfoRecPtr schedulerInfo)
{
	return schedulerInfo->SuggestedThreadID;
}


VMacFiber_thread::VMacFiber_thread( void *inParam):VMacFiber( inParam)
{
	// main fiber
	fIsMain = true;
	fInited = true;
	fRunning = true;
	fProc = NULL;
	OSErr error = ::GetCurrentThread( &fSystemID);
	xbox_assert( (error == noErr) && (fSystemID != kNoThreadID) );

	#if 0 //VERSIONDEBUG
	error = ::SetThreadSwitcher( fSystemID, sThreadSwitchIn, this, true);
	error = ::SetThreadSwitcher( fSystemID, sThreadSwitchOut, this, false);
	#endif
}


VMacFiber_thread::VMacFiber_thread( size_t inStackSize, LPFIBER_START_ROUTINE inProc, void *inParam):VMacFiber( inParam)
{
	fIsMain = false;
	fInited = false;
	fRunning = false;
	fProc = inProc;
	fSystemID = kNoThreadID;

	static ThreadEntryUPP sThreadEntry = ::NewThreadEntryUPP( _ThreadProc);
	
	/// can't create a cooperative thread from a non cooperative one
	ThreadID currentThread = kNoThreadID;
	OSErr error = ::GetCurrentThread( &currentThread);
	if (testAssert( (error == noErr) && (currentThread != kNoThreadID) ))
	{
		error = ::NewThread( kCooperativeThread, sThreadEntry, this, inStackSize, kCreateIfNeeded | kNewSuspend, NULL, &fSystemID);
		xbox_assert(error == noErr);
		
		#if 0 //VERSIONDEBUG
		if (error == noErr)
		{
			::SetThreadSwitcher( fSystemID, sThreadSwitchIn, this, true);
			::SetThreadSwitcher( fSystemID, sThreadSwitchOut, this, false);
		}
		#endif
	}
}


pascal void* VMacFiber_thread::_ThreadProc( void *inTask)
{
	VMacFiber_thread *task = static_cast<VMacFiber_thread*>( inTask);

#if VERSIONDEBUG
	ThreadID systemid;
	::GetCurrentThread( &systemid);
	xbox_assert( task->fSystemID == systemid);
#endif

	xbox_assert( task->fRunning);
	
	SetCurrent( task);

	task->fProc( task->GetParam());

	return NULL;
}



//===============================================================================================


VMacFiber *VMacFiber::ConvertThreadToFiber( void *inParam)
{
	Init();

	VMacFiber *fiber = GetCurrent();
	if (fiber == NULL)
	{
		#if !__LP64__ && !defined(__clang__)
		if (sThreadingModel == 0)
			fiber = new VMacFiber_longjmp( inParam);
		else
		#endif
			fiber = new VMacFiber_thread( inParam);

		SetCurrent( fiber);
	}
	return fiber;
}


VMacFiber *VMacFiber::CreateFiber( size_t inStackReserveSize, LPFIBER_START_ROUTINE inRunProc, void *inParam)
{
	VMacFiber *fiber;

	#if !__LP64__ && !defined(__clang__)
	if (sThreadingModel == 0)
		fiber = new VMacFiber_longjmp( inStackReserveSize, inRunProc, inParam);
	else
	#endif
		fiber = new VMacFiber_thread( inStackReserveSize, inRunProc, inParam);

	if (!fiber->IsValid())
	{
		delete fiber;
		fiber = NULL;
	}

	return fiber;
}



//===============================================================================================


void *ConvertThreadToFiber( void *inParam)
{
	return VMacFiber::ConvertThreadToFiber( inParam);
}


void *CreateFiberEx( size_t /*inStackCommitSize*/, size_t inStackReserveSize, uLONG /*inFlags*/, LPFIBER_START_ROUTINE inRunProc, void *inParam)
{
	return VMacFiber::CreateFiber( inStackReserveSize, inRunProc, inParam);
}


void *GetCurrentFiber()
{
	return VMacFiber::GetCurrent();
}


void *GetFiberData()
{
	return VMacFiber::GetCurrent()->GetParam();
}


void DeleteFiber( void *inFiber)
{
	delete static_cast<VMacFiber*>( inFiber);
}


void SwitchToFiber( void *inFiber)
{
	static_cast<VMacFiber*>( inFiber)->Run();
}


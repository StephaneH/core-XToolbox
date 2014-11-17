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
#include "VTask.h"
#include "VProcess.h"
#include "VString.h"
#include "VError.h"
#include "XMacFiber.h"

#include <pthread.h>
#include <mach/vm_map.h>
#include <mach/task.h>
#include <mach/semaphore.h>
#include <mach/semaphore.h>
#include <mach/thread_act.h>

sLONG	XMacTaskMgr::sMacNoYield = 0;	// No yield if > 0



//================================================================================================================


_FiberData::_FiberData( VTask *inTask):fTask( inTask), fTaskID( inTask ? inTask->GetID() : NULL_TASK_ID)
{
}


void _FiberData::SetTask( VTask *inTask)
{
	fTask = inTask;
	fTaskID = inTask->GetID();
}


//================================================================================================================


XMacTask::XMacTask( VTask *inOwner, bool inMainTask)
: fOwner( inOwner)
, fStackAddress( inMainTask ? (char*) ::pthread_get_stackaddr_np( ::pthread_self()) : NULL)
, fStackSize( inMainTask ? ::pthread_get_stacksize_np( ::pthread_self()) : 0)
, fFiber( NULL)
, fFiberGroupOwner( NULL)
, fMainTask( inMainTask)
, fMachThreadPort(0)
{
	memset(&fPThreadID, 0, sizeof(pthread_t));
}


XMacTask::~XMacTask()
{
	if (fFiber)
	{
		if (fMainTask)
		{
			// delete main thread fiber data here.
			// for other threads, it's done in ::Exit()
			delete _FiberData::Current();
		}
		::DeleteFiber( fFiber);
	}
}


VTaskID XMacTask::GetValueForVTaskID() const
{
	return NULL_TASK_ID;
}


XMacTaskMgr *XMacTask::GetManager() const
{
	return VTaskMgr::Get()->GetImpl();
}


size_t XMacTask::GetCurrentFreeStackSize()
{
	char c;
	return fStackSize - (fStackAddress - &c);
}


bool XMacTask::GetCPUTimes(Real& outSystemTime, Real& outUserTime) const
{
	bool validValue = false;
	
	outSystemTime = 0.0;
	outUserTime = 0.0;
	struct thread_basic_info t_info;
	mach_msg_type_number_t count = THREAD_BASIC_INFO_COUNT;
	if(thread_info(fMachThreadPort, THREAD_BASIC_INFO, (thread_info_t)&t_info, &count) == KERN_SUCCESS){
		outSystemTime = t_info.system_time.seconds + t_info.system_time.microseconds / 1e6;
		outUserTime = t_info.user_time.seconds + t_info.user_time.microseconds / 1e6;
		validValue = true;
	}
	else{
		xbox_assert(false);
	}
	
	return validValue;
}


bool XMacTask::GetCPUUsage(Real& outCPUUsage) const
{
	bool validValue = false;

	integer_t cpuUsage = 0;
	struct thread_basic_info t_info;
	mach_msg_type_number_t count = THREAD_BASIC_INFO_COUNT;
	if (thread_info(fMachThreadPort, THREAD_BASIC_INFO, (thread_info_t)&t_info, &count) == KERN_SUCCESS)
	{
		cpuUsage = t_info.cpu_usage;
		validValue = true;
	}
	
	outCPUUsage = (Real)cpuUsage;
	outCPUUsage /= (Real)TH_USAGE_SCALE;
	
	return validValue;
}



void XMacTask::_Run()
{
	char c;
	fStackAddress = &c;
	
	fPThreadID = ::pthread_self();
	fMachThreadPort = pthread_mach_thread_np(fPThreadID);
	xbox_assert(fMachThreadPort > 0);

	GetManager()->SetCurrentTask( GetOwner());

	try
	{
		fOwner->_Run();
	}
	catch(...)
	{
	}
	
	VTask *destinationTask = (fOwner == fFiberGroupOwner) ? NULL : fFiberGroupOwner;
	Exit( destinationTask);
}


//================================================================================================================

XMacTask_preemptive::XMacTask_preemptive( VTask *inOwner, bool /* for main task */ )
: XMacTask( inOwner, true)
, fSema( 0)
, fAllocatedStackAddress( 0)
{	
	fPThreadID = ::pthread_self();
	fMachThreadPort = ::pthread_mach_thread_np(fPThreadID);
	
	if (GetManager()->WithFibers())
	{
		// main task has already been converted into a fiber by the XTaskMgr,
		// we just have to set the VTask now we know it.
		SetFiber( ::GetCurrentFiber(), inOwner);

		_FiberData::Current()->SetTask( inOwner);
	}
	
	// the main thread will wait on this semaphore for Wait & SLeep
	kern_return_t kr = semaphore_create( mach_task_self(), &fSema, SYNC_POLICY_FIFO, 0);
	xbox_assert(kr == KERN_SUCCESS);
}


XMacTask_preemptive::XMacTask_preemptive( VTask *inOwner)
: XMacTask( inOwner, false)
, fSema( 0)
, fAllocatedStackAddress( 0)
{
}


XMacTask_preemptive::~XMacTask_preemptive()
{
	kern_return_t kr;

	// the task must be dead so one can deallocate its stack safely
	if (fPThreadID != 0)
	{
		// the thread might be still waitingfor Run() to be called.
		if (fSema != 0)
		{
			kr = semaphore_signal( fSema);
		}
		if (!IsMainTask())
        {
            int r = pthread_join( fPThreadID, NULL);
            xbox_assert( r == 0);
        }
	}
	
	if (fAllocatedStackAddress != 0)
		kr = vm_deallocate( mach_task_self(), fAllocatedStackAddress, fAllocatedStackSize);

	if (fSema != 0)
		kr = semaphore_destroy( mach_task_self(), fSema);
} 


VTaskID XMacTask_preemptive::GetValueForVTaskID() const
{
	mach_port_t tid = pthread_mach_thread_np( fPThreadID);
	if ( (VTaskID) tid == tid)
		return (VTaskID) tid;
	return NULL_TASK_ID;
}


void XMacTask_preemptive::Exit( VTask *inDestinationTask)
{
	delete _FiberData::Current();

	// now this can be deleted at anytime.
	// but the pthread_join in ~XMacTask_preemptiv will wait for us.
	GetOwner()->_SetStateDead();

	pthread_exit( NULL); // never returns
}


void *XMacTask_preemptive::_ThreadProc(void* inParam)
{
	XMacTask_preemptive *task = static_cast<XMacTask_preemptive*>( inParam);
	VTask *owner = task->GetOwner();

	if (task->GetManager()->WithFibers())
	{
		void *fiber = ::ConvertThreadToFiber( new _FiberData( owner));
		
		task->SetFiber( fiber, owner);
	}
	
	kern_return_t kr = semaphore_wait( task->fSema);

	// we may have been wakened up to delete self without Run() ever being called.
	if (owner->GetState() == TS_RUN_PENDING)
		task->_Run();	// never returns
	
	return NULL;
}


bool XMacTask_preemptive::_CreateThread()
{
	xbox_assert( (fPThreadID == 0) && (fAllocatedStackAddress == 0) && (fSema == 0) );

	size_t stackSize = ((GetOwner()->GetStackSize() + vm_page_size - 1)/ vm_page_size) * vm_page_size;
	xbox_assert(stackSize >= PTHREAD_STACK_MIN); // 8192

	// allocate a guard page
	void *stackAddress = NULL;
	fAllocatedStackSize = stackSize + vm_page_size;
	kern_return_t kr = vm_allocate( mach_task_self(), &fAllocatedStackAddress, fAllocatedStackSize, VM_MAKE_TAG(VM_MEMORY_STACK) | TRUE);
	if (kr == KERN_SUCCESS)
	{
	    /* The guard page is at the lowest address */
	    /* The stack base is the highest address */
		kr = vm_protect( mach_task_self(), fAllocatedStackAddress, vm_page_size, FALSE, VM_PROT_NONE);
		stackAddress = (void *) (fAllocatedStackAddress + vm_page_size);

		SetStackSize( stackSize);
	}

	// the thread will wait on this semaphore to be sure fPThreadID is properly set before it runs
	if (testAssert( kr == KERN_SUCCESS))
		kr = semaphore_create( mach_task_self(), &fSema, SYNC_POLICY_FIFO, 0);

	if (kr != KERN_SUCCESS)
		vThrowMachError( kr);
	else
	{
		pthread_attr_t att;
		int r = pthread_attr_init( &att);

		if (testAssert( r == 0))
			r = pthread_attr_setstack( &att, stackAddress, stackSize);

		// one need to use pthread_join to safely dispose the thread stack.
		if (testAssert( r == 0))
			r = pthread_attr_setdetachstate( &att, PTHREAD_CREATE_JOINABLE);

		if (testAssert( r == 0))
			r = pthread_create( &fPThreadID, &att, _ThreadProc, this);
	/*
		 [EAGAIN]           The system lacked the necessary resources to create
							another thread, or the system-imposed limit on the
							total number of threads in a process
							[PTHREAD_THREADS_MAX] would be exceeded.

		 [EINVAL]           The value specified by attr is invalid.
	*/

		if (r != 0)
		{
			vThrowPosixError( r);
		}

		r = pthread_attr_destroy( &att);
	}
	
	return fPThreadID != 0;
}


/*
	static
*/
XMacTask_preemptive *XMacTask_preemptive::Create( VTask *inOwner)
{
	XMacTask_preemptive *task = new XMacTask_preemptive( inOwner);
	if (!task->_CreateThread())
	{
		delete task;
		task = NULL;
	}
	return task;
}


bool XMacTask_preemptive::Run()
{
	bool ok;
	if (fPThreadID != 0)
	{
		ok = true;
		kern_return_t kr = semaphore_signal( fSema);
		xbox_assert( kr == KERN_SUCCESS);
	}
	else
	{
		ok = false;
	}
	
	return ok;
}


void XMacTask_preemptive::Sleep( sLONG inMilliSeconds)
{
    mach_timespec_t waitTime;

    waitTime.tv_sec   = inMilliSeconds / 1000;
    waitTime.tv_nsec  = 1000 * 1000 * (inMilliSeconds % 1000);

	kern_return_t kr = semaphore_timedwait( fSema, waitTime);
}


void XMacTask_preemptive::WakeUp()
{
	kern_return_t kr = semaphore_signal( fSema);
}


void XMacTask_preemptive::Freeze()
{
	kern_return_t kr = semaphore_wait( fSema);
}


//================================================================================================================



XMacTask_fiber::XMacTask_fiber( VTask *inOwner):XMacTask( inOwner, false),fSleepStamp( 0)
{
}


XMacTask_fiber::~XMacTask_fiber()
{
}


void XMacTask_fiber::Exit( VTask *inDestinationTask)
{
	delete _FiberData::Current();

	// now this can be deleted at anytime.
	GetOwner()->_SetStateDead();

	inDestinationTask->GetImpl()->SwitchToFiber();
}


bool XMacTask_fiber::Run()
{
	// the new fiber will belong to the calling thread.
	
	size_t stackSize = GetOwner()->GetStackSize();
	SetStackSize( stackSize);

	/*commit size: 0 = default */
	void *fiber = ::CreateFiberEx( 0, stackSize, FIBER_FLAG_FLOAT_SWITCH, _ThreadProc, new _FiberData( GetOwner()));
	if (testAssert( fiber != NULL))
	{
		VTask *currentTask = GetManager()->GetCurrentTask();
		VTask *fiberGroupOwner = currentTask->GetImpl()->GetFiberGroupOwner();
		if (fiberGroupOwner == NULL)
			fiberGroupOwner = currentTask;
		
		SetFiber( fiber, fiberGroupOwner);

		SwitchToFiber();

		GetManager()->SetCurrentTask( currentTask);
	}
	return fiber != NULL;
}


void XMacTask_fiber::_ThreadProc( void *inParam)
{
	_FiberData *data = static_cast<_FiberData*>( inParam);
	VTask *owner = data->GetTask();

	owner->GetImpl()->_Run();
}


void XMacTask_fiber::Sleep( sLONG inMilliSeconds)
{
	VTaskMgr::Get()->CurrentFiberWaitFlagWithTimeout( &fSleepStamp, fSleepStamp, inMilliSeconds);
}


void XMacTask_fiber::WakeUp()
{
	++fSleepStamp;
	VTaskMgr::Get()->YieldNow();
}


void XMacTask_fiber::Freeze()
{
	VTaskMgr::Get()->CurrentFiberWaitFlag( &fSleepStamp, fSleepStamp);
}



bool XMacTask_fiber::CheckSystemMessages()
{
	return false;
}


//================================================================================================================


XMacTaskMgr::XMacTaskMgr():fWithFibers( false), fThreadingModel(-1)
{
	int r = ::pthread_key_create( &fSlot_CurrentTask, NULL);
	xbox_assert( r == 0);

	r = ::pthread_key_create( &fSlot_CurrentTaskID, NULL);
	xbox_assert( r == 0);
}


XMacTaskMgr::~XMacTaskMgr()
{
	int r = ::pthread_key_delete( fSlot_CurrentTask);
	xbox_assert( r == 0);

	r = ::pthread_key_delete( fSlot_CurrentTaskID);
	xbox_assert( r == 0);
}


XMacTask *XMacTaskMgr::CreateMain( VTask* inOwner)
{
	XMacTask *task = new XMacTask_preemptive( inOwner, true);

	SetCurrentTask( inOwner);

	return task;
}


XMacTask *XMacTaskMgr::Create( VTask* inOwner, ETaskStyle inStyle)
{
	XMacTask* macTask;
	
	switch( inStyle)
	{
		case eTaskStyleFiber:
			{
				macTask = testAssert( fWithFibers) ? new XMacTask_fiber( inOwner) : NULL;
				break;
			}

		case eTaskStylePreemptive:
			{
				macTask = XMacTask_preemptive::Create( inOwner);
				break;
			}
		
		default:
			macTask = NULL;
	}
	return macTask;
}


void XMacTaskMgr::TaskStarted( VTask *inTask)
{
}


void XMacTaskMgr::TaskStopped( VTask *inTask)
{
}


void XMacTaskMgr::YieldToFiber( VTask *inPreferredTask)
{
	// on ne peut switcher qu'entre fibres du meme groupe
	VTask *currentTask = GetCurrentTask();
	if (currentTask->GetImpl()->GetFiberGroupOwner() == inPreferredTask->GetImpl()->GetFiberGroupOwner())
	{
		inPreferredTask->GetImpl()->SwitchToFiber();
		SetCurrentTask( currentTask);
	}
}


void XMacTaskMgr::YieldFiber()
{
	if (sMacNoYield == 0)
		::YieldToAnyThread();
}


size_t XMacTaskMgr::ComputeMainTaskStackSize() const
{
	return ::pthread_get_stacksize_np( ::pthread_self());
}


bool XMacTaskMgr::Init( VProcess* inProcess)
{
	fWithFibers = inProcess->IsFibered();
	if (fWithFibers)
	{
		// on convertit tout de suite la main thread, la VTask sera fournie plus tard
		::ConvertThreadToFiber( new _FiberData( NULL));
	}

	::GetThreadCurrentTaskRef( &fTaskRef);
	return true;
}

void XMacTaskMgr::Deinit( )
{

}

bool XMacTaskMgr::IsOneThreadRunning()
{
	// pour determiner le sleep a passert a WaitNextEvent
	bool isRunning = true;
/*	
	ThreadState state;
	for (sLONG i = fThreadIDs->GetCount() ; i > 0 ; --i)
	{
		OSErr macerr = ::GetThreadState(fThreadIDs->GetLong(i), &state);
		if (macerr != noErr)
			fThreadIDs->Delete(i);
		else
		{
			if (state == kReadyThreadState)
			{
				isRunning = true;
				break;
			}
		}
	}
*/
	return isRunning;
}


size_t XMacTaskMgr::GetCurrentFreeStackSize() const
{
	VTask *task = GetCurrentTask();
	if (task == NULL)
		return GetCurrentFreeStackSizeStatic();
	return task->GetImpl()->GetCurrentFreeStackSize();
}


size_t XMacTaskMgr::GetCurrentFreeStackSizeStatic()
{
	size_t stackSize = 0;
	OSErr error = ::ThreadCurrentStackSpace( kCurrentThreadID, &stackSize);
	xbox_assert( error == noErr);
	return stackSize;
}

bool XMacTaskMgr::IsFibersThreadingModel() 
{
	if(fThreadingModel == -1){
		
		fThreadingModel = 0; //longjmp by default
		
		CFStringRef value = (CFStringRef) ::CFBundleGetValueForInfoDictionaryKey( ::CFBundleGetMainBundle(), CFSTR( "ThreadingModel"));
		if (value != NULL)
		{
			if (::CFStringCompare( value, CFSTR( "longjmp"), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
				fThreadingModel = 0;
			else if (::CFStringCompare( value, CFSTR( "NewThread"), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
				fThreadingModel = 1;				
		}
	}
	
	return (fThreadingModel == 0);
}


void XMacTaskMgr::SetCurrentThreadName( const VString& inName, VTaskID inTaskID) const
{
#if defined(MAC_OS_X_VERSION_10_6) && (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6)
	if (VSystem::MAC_GetSystemVersionDecimal() >= 100600)
	{
		VString name( inName);
		name.AppendPrintf( " (id = %d)", inTaskID);
		VStringConvertBuffer buffer( name, VTC_UTF_8);
		pthread_setname_np( buffer.GetCPointer());
	}
#endif
}



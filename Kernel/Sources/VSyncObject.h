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
#ifndef __VSyncObject__
#define __VSyncObject__

#include "Kernel/Sources/VObject.h"
#include "Kernel/Sources/IRefCountable.h"
#include "Kernel/Sources/VSystem.h"
#include "Kernel/Sources/VTask.h"
#include "Kernel/Sources/VInterlocked.h"

BEGIN_TOOLBOX_NAMESPACE

typedef	sLONG	SpinLockType;	// 0: unlocked. Uses only bit 31 (convenient to use first 31 bits for someting else)

inline	void	SpinLockFiber( SpinLockType& ioLock)	{ while( VInterlocked::TestAndSet(&ioLock, 31)) VTask::YieldNow(); }
inline	void	SpinLockThread( SpinLockType& ioLock)	{ while( VInterlocked::TestAndSet(&ioLock, 31)) XTaskMgrImpl::YieldNow(); }
inline	void	SpinUnlock( SpinLockType& ioLock)		{ xbox_assert(ioLock & (1 << 31) ); ioLock &= ~(1 << 31); }


// Root class private to VTaskLock
// Cant inherit from VObject because it uses VCppMem which uses a VCriticalSection

class XTOOLBOX_API VSyncObject
{
public:
									VSyncObject()			{;}

protected:
	template<class T>
	static	bool					_FiberLock( T *inSyncObject, sLONG *inUnlockStamp)
									{
										sLONG stamp = *inUnlockStamp;
										while( !inSyncObject->TryToLock())
										{
											VTaskMgr::Get()->CurrentFiberWaitFlag( inUnlockStamp, stamp);
											stamp = *inUnlockStamp;
										}

										return true;
									}

	template<class T>
	static	bool					_FiberLock( T *inSyncObject, sLONG *inUnlockStamp, sLONG inTimeoutMilliseconds)
									{
										xbox_assert(inTimeoutMilliseconds > 0);

										sLONG stamp = *inUnlockStamp;
										if (inSyncObject->TryToLock())
											return true;

										uLONG t1 = VSystem::GetCurrentTime() + inTimeoutMilliseconds;
										uLONG delta = inTimeoutMilliseconds;
										do
										{
											VTaskMgr::Get()->CurrentFiberWaitFlagWithTimeout( inUnlockStamp, stamp, delta);

											// if we returned from CurrentFiberWaitFlagWithTimeout and the stamp has not changed,
											// that's means the timeout expired or we have been waken up: let's exit.
											if (*inUnlockStamp == stamp)
												return false;

											// check if timeout expired
											uLONG t0 = VSystem::GetCurrentTime();
											if (t0 >= t1)
												return false;

											delta = t1 - t0;
											stamp = *inUnlockStamp;
										} while( !inSyncObject->TryToLock());

										return true;
									}

private:
									VSyncObject( const VSyncObject& );	// no copy
			VSyncObject&			operator=( const VSyncObject& );	// no copy
};



END_TOOLBOX_NAMESPACE


#if VERSIONMAC
#include "Kernel/Sources/XMacSyncObject.h"
#elif VERSION_LINUX
#include "Kernel/Sources/XLinuxSyncObject.h"
#elif VERSIONWIN
#include "Kernel/Sources/XWinSyncObject.h"
#elif VERSION_DARWIN
#include "Kernel/Sources/XDarwinSyncObject.h"
#endif

BEGIN_TOOLBOX_NAMESPACE


class XTOOLBOX_API VSemaphore : public VSyncObject, public IRefCountable
{
public:
	/*
			Traditionnal semaphore.
			Lock() tries to get a resource (blocks if resource count is zero).
			Unlock release one resource.
			The number of resource cannot be less than zero or greater than inMaxCount.

			fiber-aware.

			ref-countable

	*/

									VSemaphore( sLONG inInitialCount = 1, sLONG inMaxCount = 1):fImpl(inInitialCount, inMaxCount),fUnlockStamp( 0)	{;}
			bool					Lock()										{ return VTask::CurrentCanBlockOnSyncObject() ? fImpl.Lock() : _FiberLock( &fImpl, &fUnlockStamp);}
			bool					Lock( sLONG inTimeoutMilliseconds)			{ return VTask::CurrentCanBlockOnSyncObject() ? fImpl.Lock( inTimeoutMilliseconds) : _FiberLock( &fImpl, &fUnlockStamp, inTimeoutMilliseconds);}
			bool					TryToLock()									{ return fImpl.TryToLock();}

			// WARNING: Unlock() is not atomic: make sure the VSemaphore object remains valid during Unlock()
			bool					Unlock()									{ bool ok = fImpl.Unlock(); VInterlocked::Increment( &fUnlockStamp); return ok;}

private:
			XSemaphoreImpl			fImpl;
			sLONG					fUnlockStamp;	// incremented for each Unlock
};


class XTOOLBOX_API VSyncEvent : public VSyncObject, public IRefCountable
{
public:
	/*
			An event is in signaled state or not.
			It is created in non-signaled state.
			Lock() blocks if the event is not signaled.
			Unlock() puts the event in signaled state.

			fiber-aware.

	*/
									VSyncEvent():fUnlockStamp( 0)				{;}

			// Wait until event occurs
			bool					Lock()										{ return VTask::CurrentCanBlockOnSyncObject() ? fImpl.Lock() : _FiberLock( &fImpl, &fUnlockStamp);}
			bool					Lock( sLONG inTimeoutMilliseconds)			{ return VTask::CurrentCanBlockOnSyncObject() ? fImpl.Lock( inTimeoutMilliseconds) : _FiberLock( &fImpl, &fUnlockStamp, inTimeoutMilliseconds);}
			bool					TryToLock()									{ return fImpl.TryToLock();}

			// Signal the event (all waiting tasks are unblocked)
			// WARNING: Unlock() is not atomic: make sure the VSyncEvent object remains valid during Unlock()
			bool					Unlock()									{ bool ok = fImpl.Unlock(); VInterlocked::Increment( &fUnlockStamp); return ok;}

			// resets the event which makes it available for the next successfull Lock()
			bool					Reset()										{ return fImpl.Reset();}

private:
			XSyncEventImpl			fImpl;
			sLONG					fUnlockStamp;	// incremented for each Unlock
};

#define VCriticalSection_USE_SPINLOCK 1

class XTOOLBOX_API VCriticalSection : public VSyncObject
{
friend class VConditionVariable;
public:
	/*
			Recursive mutex intended to protect a small piece of non thred-safe code.

			VCriticalSection is custom made in order to be fiber-aware (on windows, cooperative VTask is not a system thread).
	*/
									VCriticalSection();
									~VCriticalSection();

			bool					Lock();

			// Returns false if already locked by another process
			bool					TryToLock();
			bool					Unlock();

			// No protection - should be called only if Lock() returns true
#if VCriticalSection_USE_SPINLOCK
			sLONG					GetUseCount () const						{ return fSpinLockAndUseCount & 0x7FFFFFFF; }
#else
			sLONG					GetUseCount () const						{ return fUseCount; }
#endif

			VTaskID					GetOwnerTaskID() const						{ return fOwner;}

private:
			VSyncEvent*				fEvent;
			VTaskID					fOwner;
#if VCriticalSection_USE_SPINLOCK
			SpinLockType			fSpinLockAndUseCount;		/* Used for internal mutex on structure */
			void					_Lock()															{ SpinLockThread( fSpinLockAndUseCount);}
			void					_Unlock()														{ SpinUnlock( fSpinLockAndUseCount);}
#else
			sLONG					fUseCount;
	static	sLONG					sUnlockCount;
#endif
};


class XTOOLBOX_API VSystemCriticalSection : public VSyncObject
{
public:
			/*
				Recursive mutex intended to protect a small piece of non thred-safe code.

				NOT fiber-safe: blocking one fiber blocks all fibers in same thread.

				cf VMutex.
			*/

			bool					Lock()										{ return fImpl.Lock();}
			bool					TryToLock()									{ return fImpl.TryToLock();}
			bool					Unlock()									{ return fImpl.Unlock();}

private:
			XCriticalSectionImpl	fImpl;
};


class XTOOLBOX_API VMutex : public VSyncObject, public IRefCountable
{
public:
			/*
				Recursive mutex intended to protect a small piece of non thred-safe code.

				NOT fiber-safe: blocking one fiber blocks all fibers in same thread.

				This is functionnaly the same as a VSystemCriticalSection but is more general since it provides locking with a timeout.
				VSystemCriticalSection is generally lighter than a VMutex.
			*/

			bool					Lock()										{ return fImpl.Lock();}
			bool					TryToLock()									{ return fImpl.TryToLock();}
			bool					Lock( sLONG inTimeoutMilliseconds)			{ return fImpl.Lock( inTimeoutMilliseconds);}

			bool					Unlock()									{ return fImpl.Unlock();}

private:
			XMutexImpl				fImpl;
};


class XTOOLBOX_API VConditionVariable : public VSyncObject
{
public:
									VConditionVariable():fEvent(NULL), fBusy( NULL), fSpinLock(0), fWaiters( 0), fSigsPending( 0) {}
									~VConditionVariable();

			bool					Broadcast();
			bool					Wait( VCriticalSection *inMutex, sLONG inTimeoutMilliseconds)	{ return _Wait( inMutex, true, inTimeoutMilliseconds);}
			bool					Wait( VCriticalSection *inMutex)								{ return _Wait( inMutex, false, 0);}

private:
			void					_Lock()															{ SpinLockThread( fSpinLock);}
			void					_Unlock()														{ SpinUnlock( fSpinLock);}

			bool					_Wait( VCriticalSection *inMutex, bool inWithTimeout, sLONG inTimeoutMilliseconds);

			VSyncEvent*				fEvent;
			VCriticalSection*		fBusy;			/* mutex associated with variable for safety check */
			SpinLockType			fSpinLock;		/* Used for internal mutex on structure */
			sWORD					fWaiters;		/* Number of threads waiting */
			sWORD					fSigsPending;	/* Number of outstanding signals */
};

template<class T>
class StLocker
{
public:
			StLocker( T* inSyncObject) : fSyncObject( inSyncObject)	{ fSyncObject->Lock(); }
			~StLocker()												{ fSyncObject->Unlock(); }

private:
	T*		fSyncObject;
};

typedef StLocker<VCriticalSection> VTaskLock;


END_TOOLBOX_NAMESPACE

#endif


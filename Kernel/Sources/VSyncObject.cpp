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
#include "VSyncObject.h"
#include "VTask.h"
#include "VInterlocked.h"


// Class macros
#define DEBUG_SEMA	0	// Set to '1' to enable debug mode, not fiber safe !!


// Class statics
#if !VCriticalSection_USE_SPINLOCK
sLONG	VCriticalSection::sUnlockCount = 0;
#endif

//================================================================================================================


VCriticalSection::VCriticalSection()
: fOwner( NULL_TASK_ID)
#if VCriticalSection_USE_SPINLOCK
, fSpinLockAndUseCount( 0)
#else
, fUseCount( 0)
#endif
{
#if DEBUG_SEMA
	fEvent = reinterpret_cast<VSyncEvent*>(new VSystemCriticalSection);
#else
	fEvent = NULL;
#endif
}


VCriticalSection::~VCriticalSection()
{
#if DEBUG_SEMA
	reinterpret_cast<VSystemCriticalSection*>(fEvent)->Release();
#else
	xbox_assert( GetUseCount() == 0 && fOwner == NULL_TASK_ID && fEvent == NULL);
	
	while (GetUseCount() > 0)
	{
		Unlock();
	}
#endif
}


bool VCriticalSection::TryToLock()
{
#if DEBUG_SEMA
	return false;
#else
	VTaskID currentTaskID = VTask::GetCurrentID();

#if VCriticalSection_USE_SPINLOCK
	bool ok;
	_Lock();
	VTaskID owner = fOwner;
	if (owner == NULL_TASK_ID)
	{
		fOwner = currentTaskID;
		xbox_assert(GetUseCount() == 0);
		++fSpinLockAndUseCount;
		ok = true;
	}
	else if (owner == currentTaskID)
	{
		++fSpinLockAndUseCount;
		ok = true;
	}
	else
	{
		ok = false;
	}
	_Unlock();
	return ok;
#else
	VTaskID owner = (VTaskID) VInterlocked::CompareExchange((sLONG*) &fOwner, (sLONG) NULL_TASK_ID, (sLONG) currentTaskID);

	if (owner == NULL_TASK_ID)
	{
		xbox_assert(fUseCount == 0);
		fUseCount = 1;
	}
	else if (owner == currentTaskID)
	{
		++fUseCount;
	}
		
	return (fOwner == currentTaskID);
#endif

#endif
}


bool VCriticalSection::Lock()
{
#if DEBUG_SEMA
	reinterpret_cast<VSystemCriticalSection*>(fEvent)->Lock();
	fUseCount++;
	return true;
#else

	assert_compile( sizeof( VTaskID) == sizeof( sLONG));
	
	
	VTaskID currentTaskID = VTask::GetCurrentID();
#if VCriticalSection_USE_SPINLOCK
	_Lock();
	do {
		VTaskID	owner = fOwner;
		if (owner == NULL_TASK_ID)
		{
			fOwner = currentTaskID;
			// We are the new owner
			xbox_assert(GetUseCount() == 0);
			++fSpinLockAndUseCount;
			_Unlock();
			break;
		}
		else if (fOwner == currentTaskID)
		{
			// We are already the owner
			++fSpinLockAndUseCount;
			_Unlock();
			break;
		}
		else
		{
			// Currently used by some other task.
			// We have to use a VSyncEvent to block on.
			// if there's one let's retain it
			VSyncEvent*	event = fEvent;
			if (event != NULL)
			{
				event->Retain();
				_Unlock();
				event->Lock();
				event->Release();
				_Lock();
			}
			else
			{
				// have to allocate a new event
				_Unlock();
				event = new VSyncEvent;
				if (event != NULL)
				{
					_Lock();
					if (owner == NULL_TASK_ID)
					{
						// the critical section has been released while we were creating the event
						fOwner = currentTaskID;
						xbox_assert(GetUseCount() == 0);
						++fSpinLockAndUseCount;
						_Unlock();
						event->Release();
						break;
					}
					else if (fEvent == NULL)
					{
						fEvent = event;
					}
					else
					{
						// some other task has installed an event before us.
						_Unlock();
						event->Release();
						XTaskMgrImpl::YieldNow();
						_Lock();
					}
				}
				else
				{
					_Lock();
				}
			}
		}
	} while(true);
#else
	do {
		VTaskID	owner = (VTaskID) VInterlocked::CompareExchange((sLONG*) &fOwner, (sLONG) NULL_TASK_ID, (sLONG) currentTaskID);
		if (owner == NULL_TASK_ID)
		{
			// We are the new owner
			xbox_assert(fUseCount == 0);
			fUseCount = 1;
		}
		else if (fOwner == currentTaskID)
		{
			// We are already the owner
			++fUseCount;	// Unlock or another successfull lock can only be called from same thread (so no contention can occur)
		}
		else
		{
			// Currently used by some other task.
			// We have to use a VSyncEvent to block on.
			sLONG	unlockCount = sUnlockCount;

			// Capture currently set event to make a safe Retain
			VSyncEvent*	event = VInterlocked::ExchangePtr( &fEvent);
			if (event != NULL)
			{
				// Got one event, retain it and try to use it.
				event->Retain();
				
				// Try to set it back
				if (VInterlocked::CompareExchangePtr((void**)&fEvent, NULL, event) != NULL)
				{
					// Could not set it back, so unlock waiting tasks and release it
					event->Unlock();
					event->Release();
					VTask::YieldNow(); // Even with preemptiv scheduling, relinquish remaining time slice
				}
				else
				{
					// S'il n'y a pas eu d'unlock pendant le swap de l'event, on se met en attente dessus
					// sinon on refait un tour
					
					// Pour etre sur que sUnlockCount est a jour en multi-processeurs
					// pas sur que ce soit utile car on fait un AtomicIncrement dessus
					while (VInterlocked::CompareExchange(&sUnlockCount, sUnlockCount, sUnlockCount) != sUnlockCount)
						VTask::YieldNow();

					if (sUnlockCount == unlockCount)
						event->Lock(100); // timeout de 0.1 second pour contourner un bug
					else
						VTask::YieldNow(); // even with preemptiv scheduling, relinquish remaining time slice
				}
				
				event->Release();
			}
			else
			{
				// No event was created yet, so create a new one, install it and loop one more time
				event = new VSyncEvent;

				if (VInterlocked::CompareExchangePtr((void**)&fEvent, NULL, event) != NULL)
				{
					// Someone just put a new event so delete our own and try again
					event->Unlock();
					event->Release();
					VTask::YieldNow(); // Even with preemptiv scheduling, relinquish remaining time slice
				}
			}
		}
	} while (fOwner != currentTaskID);
#endif
	return true;
#endif
}


bool VCriticalSection::Unlock()
{
#if DEBUG_SEMA
	reinterpret_cast<VSystemCriticalSection*>(fEvent)->Unlock();
	--fUseCount;
#else
	xbox_assert((fOwner == VTask::GetCurrentID()) && (GetUseCount() > 0) && (GetUseCount() < 32000L));

#if VCriticalSection_USE_SPINLOCK
	_Lock();
	if (--fSpinLockAndUseCount == 0x80000000)
	{
		VSyncEvent*	event = fEvent;
		fOwner = NULL_TASK_ID;
		VInterlocked::ExchangePtr<VSyncEvent>( &fEvent, NULL);
		_Unlock();
		if (event != NULL)
		{
			event->Unlock();
			event->Release();
		}
	}
	else
	{
		_Unlock();
	}
#else
	VInterlocked::Increment(&sUnlockCount);

	// Unlock or another successfull lock can only be called from same thread (so no contention can occur on fUseCount)
	if (--fUseCount == 0)
	{
		// Capture currently set event
		VSyncEvent*	event = VInterlocked::ExchangePtr(&fEvent);
		VTaskID	owner = VInterlocked::Exchange((sLONG*) &fOwner, (sLONG) NULL_TASK_ID);
		xbox_assert(owner == VTask::GetCurrentID());

		if (event != NULL)
		{
			event->Unlock();
			event->Release();
		}
	}
#endif
#endif
	return true;
}


//================================================================================================================


VConditionVariable::~VConditionVariable()
{
	xbox_assert( fBusy == NULL);
	ReleaseRefCountable( &fEvent);
}


/*
 * Signal a condition variable, waking up all threads waiting for it.
 */
bool VConditionVariable::Broadcast()
{
	_Lock();
	VSyncEvent *event = fEvent;
	if (event == NULL)
	{
		/* Avoid kernel call since there are no waiters... */
		xbox_assert( fWaiters == 0);
		_Unlock();
		return true;
	}
	fSigsPending++;
	_Unlock();

	event->Unlock();

	_Lock();
	fSigsPending--;
	if (fWaiters == 0 && fSigsPending == 0)
	{
		if (fEvent != NULL)
			fEvent->Reset();
	}
	_Unlock();

	return true;
}


bool VConditionVariable::_Wait( VCriticalSection *inMutex, bool inWithTimeout, sLONG inTimeoutMilliseconds)
{
	_Lock();
	if (++fWaiters == 1)
	{
		if (fEvent == NULL)
			fEvent = new VSyncEvent;
		fBusy = inMutex;
	}
	else if (!testAssert( fBusy == inMutex))
	{
		/* Must always specify the same mutex! */
		fWaiters--;
		_Unlock();
		return false;
	}
	_Unlock();

	inMutex->Unlock();
	
	bool ok;
	if (inWithTimeout)
		ok = fEvent->Lock( inTimeoutMilliseconds);
	else
		ok = fEvent->Lock();

	_Lock();
	if (--fWaiters == 0)
	{
		if (fSigsPending == 0)
		{
			fEvent->Reset();
		}
		fBusy = NULL;
	}
	_Unlock();

	if (!inMutex->Lock())
		ok = false;

	return ok;
}


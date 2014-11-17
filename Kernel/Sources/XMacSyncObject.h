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
#ifndef __XMacSyncObject__
#define __XMacSyncObject__

#include "Kernel/Sources/VArrayValue.h"


BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API XMacSemaphore
{
public:
		XMacSemaphore( sLONG inInitialCount, sLONG inMaxCount):fCount( inInitialCount), fMaxCount( inMaxCount)
		{
			pthread_mutex_init( &fMutex, NULL);	// no need for a recursive mutex
			pthread_cond_init( &fCondition, NULL);
		}
		
		
		~XMacSemaphore()
		{
			// in case of error, relinquish any pending lock
			while( pthread_cond_destroy( &fCondition) == EBUSY)
			{
				pthread_cond_broadcast( &fCondition);
				pthread_yield_np();
			}
			pthread_mutex_destroy( &fMutex);
		}


		bool	Lock( sLONG inTimeoutMilliseconds)
		{
			bool ok;
			int r = pthread_mutex_lock( &fMutex);
			if (fCount > 0)
			{
				ok = true;
				--fCount;
			}
			else if (inTimeoutMilliseconds > 0)
			{
				timespec timeout = { inTimeoutMilliseconds / 1000, (inTimeoutMilliseconds % 1000) * 1000000};
				r = pthread_cond_timedwait_relative_np( &fCondition, &fMutex, &timeout);
				if (fCount > 0)
				{
					ok = true;
					--fCount;
				}
				else
				{
					ok = false;
				}
			}
			else
			{
				ok = false;
			}
			r = pthread_mutex_unlock( &fMutex);

			return ok;
		}
		
		
		bool	Lock()
		{
			int r = pthread_mutex_lock( &fMutex);
			while( fCount == 0)
				r = pthread_cond_wait( &fCondition, &fMutex);
			--fCount;
			r = pthread_mutex_unlock( &fMutex);

			return true;
		}


		bool	TryToLock()
		{
			bool ok;
			int r = pthread_mutex_lock( &fMutex);
			if (fCount > 0)
			{
				--fCount;
				ok = true;
			}
			else
			{
				ok = false;
			}
			r = pthread_mutex_unlock( &fMutex);

			return ok;
		}

		
		bool	Unlock()
		{
			int r = pthread_mutex_lock( &fMutex);
			if (fCount < fMaxCount)
				fCount++;
			pthread_cond_signal( &fCondition);
			r = pthread_mutex_unlock( &fMutex);
			return true;
		}

private:
		pthread_mutex_t		fMutex;
		pthread_cond_t		fCondition;
		sLONG				fCount;
		sLONG				fMaxCount;
};


class XTOOLBOX_API XMacCriticalSection
{
public:
		XMacCriticalSection()
		{
			pthread_mutexattr_t	attr;
			pthread_mutexattr_init( &attr);
			pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE);

			pthread_mutex_init( &fMutex, &attr);

			pthread_mutexattr_destroy( &attr);
		}
		
		
		~XMacCriticalSection ()
		{
			pthread_mutex_destroy( &fMutex);
		}
		
		
		bool	Lock()
		{
			return pthread_mutex_lock( &fMutex) == 0;
		}
		
		bool	TryToLock()
		{
			return pthread_mutex_trylock( &fMutex) == 0;
		}
		
		bool	Unlock()
		{
			return pthread_mutex_unlock( &fMutex) == 0;
		}

private:
		pthread_mutex_t		fMutex;
};


class XTOOLBOX_API XMacMutex
{
/*
	in order to provide the timeout facility, we need to use pthread_cond_timedwait on a condition variable.
*/
public:
		XMacMutex():fOwner( 0), fCount( 0)
		{
			int r = pthread_mutex_init( &fMutex, NULL);	// no need for a recursive mutex
			r = pthread_cond_init( &fCondition, NULL);
		}
		
		
		~XMacMutex ()
		{
			// in case of error, relinquish any pending lock
			while( pthread_cond_destroy( &fCondition) == EBUSY)
			{
				pthread_cond_broadcast( &fCondition);
				pthread_yield_np();
			}
			pthread_mutex_destroy( &fMutex);
		}


		bool	Lock( sLONG inTimeoutMilliseconds)
		{
			pthread_t currentThread = pthread_self();
			int r = pthread_mutex_lock( &fMutex);
			if ( (fOwner == 0) || (fOwner == currentThread))
			{
				++fCount;
				fOwner = currentThread;
			}
			else if (inTimeoutMilliseconds > 0)
			{
				timespec timeout = { inTimeoutMilliseconds / 1000, (inTimeoutMilliseconds % 1000) * 1000000};
				r = pthread_cond_timedwait_relative_np( &fCondition, &fMutex, &timeout);
				if (r == 0)
				{
					++fCount;
					fOwner = currentThread;
				}
			}
			r = pthread_mutex_unlock( &fMutex);

			return fOwner == currentThread;
		}
		
		
		bool	Lock()
		{
			pthread_t currentThread = pthread_self();
			int r = pthread_mutex_lock( &fMutex);
			while( (fOwner != 0) && (fOwner != currentThread))
				r = pthread_cond_wait( &fCondition, &fMutex);
			++fCount;
			fOwner = currentThread;
			r = pthread_mutex_unlock( &fMutex);

			return true;
		}


		bool	TryToLock()
		{
			bool ok;
			pthread_t currentThread = pthread_self();
			int r = pthread_mutex_lock( &fMutex);
			if ( (fOwner == 0) || (fOwner == currentThread))
			{
				++fCount;
				fOwner = currentThread;
				ok = true;
			}
			else
			{
				ok = false;
			}
			r = pthread_mutex_unlock( &fMutex);

			return ok;
		}

		
		bool	Unlock()
		{
			int r = pthread_mutex_lock( &fMutex);
			assert( fCount > 0);
			if (--fCount == 0)
			{
				fOwner = 0;
				pthread_cond_signal( &fCondition);
			}
			r = pthread_mutex_unlock( &fMutex);
			return true;
		}

private:
		pthread_mutex_t		fMutex;
		pthread_cond_t		fCondition;
		pthread_t			fOwner;
		sLONG				fCount;
};


class XTOOLBOX_API XMacSyncEvent
{
public:
		XMacSyncEvent():fRaised( false)
		{
			int r = pthread_mutex_init( &fMutex, NULL);	// no need for a recursive mutex
			r = pthread_cond_init( &fCondition, NULL);
		}
		
		
		~XMacSyncEvent ()
		{
			// in case of error, relinquish any pending lock
			while( pthread_cond_destroy( &fCondition) == EBUSY)
			{
				pthread_cond_broadcast( &fCondition);
				pthread_yield_np();
			}
			pthread_mutex_destroy( &fMutex);
		}


		bool	Lock( sLONG inTimeoutMilliseconds)
		{
			int r = pthread_mutex_lock( &fMutex);
			if (!fRaised && (inTimeoutMilliseconds > 0))
			{
				timespec timeout = { inTimeoutMilliseconds / 1000, (inTimeoutMilliseconds % 1000) * 1000000};
				r = pthread_cond_timedwait_relative_np( &fCondition, &fMutex, &timeout);
			}
			bool raised = fRaised;
			r = pthread_mutex_unlock( &fMutex);

			return raised;
		}
		
		
		bool	Lock()
		{
			int r = pthread_mutex_lock( &fMutex);
			while( !fRaised)
				r = pthread_cond_wait( &fCondition, &fMutex);
			r = pthread_mutex_unlock( &fMutex);

			return true;
		}


		bool	TryToLock()
		{
			return fRaised;
		}

		
		bool	Unlock()
		{
			int r = pthread_mutex_lock( &fMutex);
			fRaised = true;
			pthread_cond_broadcast( &fCondition);
			r = pthread_mutex_unlock( &fMutex);
			return true;
		}

		bool	Reset()
		{
			int r = pthread_mutex_lock( &fMutex);
			fRaised = false;
			r = pthread_mutex_unlock( &fMutex);
			return true;
		}

private:
		pthread_mutex_t		fMutex;
		pthread_cond_t		fCondition;
		bool				fRaised;
};


class XTaskMgrMutexImpl : public XMacMutex	{};

typedef XMacCriticalSection XCriticalSectionImpl;
typedef XMacMutex XMutexImpl;
typedef XMacSemaphore XSemaphoreImpl;
typedef XMacSyncEvent XSyncEventImpl;

END_TOOLBOX_NAMESPACE

#endif
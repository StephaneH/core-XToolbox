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
#ifndef __XLinuxSyncObject__
#define __XLinuxSyncObject__


BEGIN_TOOLBOX_NAMESPACE


class XTOOLBOX_API XLinuxSemaphore
{
public:

	XLinuxSemaphore( sLONG inInitialCount, sLONG inMaxCount);				
	~XLinuxSemaphore();
		
	bool Lock( sLONG inTimeoutMilliseconds);
	bool Lock();
	bool TryToLock();
	bool Unlock();

private:

	pthread_mutex_t		fMutex;
	pthread_cond_t		fCondition;
	sLONG				fCount;
	sLONG				fMaxCount;
};


class XTOOLBOX_API XLinuxCriticalSection
{
public:

	XLinuxCriticalSection();
	~XLinuxCriticalSection();
		
	bool Lock();
	bool TryToLock();
	bool Unlock();

private:

	pthread_mutex_t		fMutex;
};


class XTOOLBOX_API XLinuxMutex
{
public:

	XLinuxMutex();		
	~XLinuxMutex ();
	
	bool Lock( sLONG inTimeoutMilliseconds);		
	bool Lock();
	bool TryToLock();		
	bool Unlock();

private:

	/*
	  in order to provide the timeout facility, we need to use pthread_cond_timedwait on a condition variable.
	*/

	pthread_mutex_t		fMutex;
	pthread_cond_t		fCondition;
	pthread_t			fOwner;
	sLONG				fCount;
};


class XTOOLBOX_API XLinuxSyncEvent
{
public:

	XLinuxSyncEvent();		
	~XLinuxSyncEvent();

	bool Lock( sLONG inTimeoutMilliseconds);
	bool Lock();
	bool TryToLock();		
	bool Unlock();

	bool Reset();
 
private:

		pthread_mutex_t		fMutex;
		pthread_cond_t		fCondition;
		bool				fRaised;
};


class XTaskMgrMutexImpl : public XLinuxMutex	{};

typedef XLinuxCriticalSection XCriticalSectionImpl;
typedef XLinuxMutex XMutexImpl;
typedef XLinuxSemaphore XSemaphoreImpl;
typedef XLinuxSyncEvent XSyncEventImpl;


END_TOOLBOX_NAMESPACE

#endif

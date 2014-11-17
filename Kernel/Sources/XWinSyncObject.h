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
#ifndef __XWinSyncObject__
#define __XWinSyncObject__

//#include <process.h>

#include "Kernel/Sources/VObject.h"
#include "Kernel/Sources/VTask.h"

BEGIN_TOOLBOX_NAMESPACE

class XWinSyncObject
{
public:
					XWinSyncObject( HANDLE inObject):fObject( inObject)		{;}
					~XWinSyncObject()										{ if (fObject != NULL) ::CloseHandle(fObject);}


			bool	Lock( sLONG inTimeoutMilliseconds)
			{
				return ::WaitForSingleObject( fObject, inTimeoutMilliseconds) == WAIT_OBJECT_0;
			}
	
			bool	Lock()
			{
				return ::WaitForSingleObject( fObject, INFINITE) == WAIT_OBJECT_0;
			}
	
			bool	TryToLock()
			{
				return ::WaitForSingleObject( fObject, 0) == WAIT_OBJECT_0;
			}

protected:
			HANDLE	fObject;
};


class XWinSemaphore : public XWinSyncObject
{
public:
					// no security attributes, no name
					XWinSemaphore( sLONG inInitialCount, sLONG inMaxCount)
						:XWinSyncObject( ::CreateSemaphore(NULL, inInitialCount, inMaxCount, NULL))	{;}

			bool	Unlock ()							{ return ::ReleaseSemaphore( fObject, 1, NULL) != 0;}
};


class XWinMutex : public XWinSyncObject
{
public:
					// no security attributes, not initial owner, no name
					XWinMutex()
						:XWinSyncObject( ::CreateMutex( NULL, false, NULL))	{;}
	
			bool	Unlock ()							{ return ::ReleaseMutex( fObject) != 0;}
};


class XWinSyncEvent : public XWinSyncObject
{
public:
			// no security attributes, manual reset, initially not signaled, no name
			XWinSyncEvent()
				:XWinSyncObject( ::CreateEvent(NULL, TRUE, FALSE, NULL))	{;}
	
			bool	Unlock()							{ return ::SetEvent(fObject) != 0;}
			bool	Reset()								{ return ::ResetEvent( fObject ) != 0;}
};



class XWinCriticalSection
{
public:
			XWinCriticalSection()
			{
				DWORD spinCount = 4000; // they say they use this value for the heap manager...

				// means preallocate the event
				// spinCount &= 0x80000000;
				
				::InitializeCriticalSectionAndSpinCount( &fSection, spinCount);
			}

			~XWinCriticalSection()						{ ::DeleteCriticalSection( &fSection);}
	
			bool	Lock()								{ ::EnterCriticalSection( &fSection); return true;}
			bool	TryToLock()							{ return ::TryEnterCriticalSection( &fSection) != 0;}
			bool	Unlock()							{ ::LeaveCriticalSection( &fSection); return true;}

protected:
	CRITICAL_SECTION	fSection;
};

class XTaskMgrMutexImpl	: public XWinCriticalSection {};

typedef XWinCriticalSection XCriticalSectionImpl;
typedef XWinMutex XMutexImpl;
typedef XWinSemaphore XSemaphoreImpl;
typedef XWinSyncEvent XSyncEventImpl;

END_TOOLBOX_NAMESPACE

#endif

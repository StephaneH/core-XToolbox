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
#include <stdio.h>
#include <stdlib.h>

#include "VKernelIPCPrecompiled.h"
#include "XWinIPC.h"



VError XWinSharedSemaphore::Init(uLONG inKey, uLONG inInitCount)
{
	char name[32];
	//(According to msdn) The name can have a "Global\" or "Local\" prefix to explicitly create the object in the global or session name space.
	_snprintf(name, sizeof(name), "%lu", inKey);

	HANDLE handle=CreateSemaphore(NULL /*default security attributes*/, inInitCount, inInitCount /*max count*/, (LPCSTR)name);

	fIsNew=(GetLastError()==ERROR_ALREADY_EXISTS) ? false : true;

	fSemPtr=SharedHandlePtr(new IPCHandleWrapper(handle));

	return (handle!=NULL) ? VE_OK : vThrowNativeError(GetLastError());
}


VError XWinSharedSemaphore::Lock()
{
	DWORD res=WaitForSingleObject(fSemPtr->GetHandle(), INFINITE);

	return (res!=WAIT_FAILED) ? VE_OK : vThrowNativeError(GetLastError());
}


VError XWinSharedSemaphore::Unlock()
{
	BOOL res=ReleaseSemaphore(fSemPtr->GetHandle(), 1 /*increase count by 1*/, NULL /*not interested in previous value*/);

	return res ? VE_OK : vThrowNativeError(GetLastError());
}


VError XWinSharedMemory::Init(uLONG inKey, VSize inSize)
{
	char name[32];
	_snprintf(name, sizeof(name), "%lu", inKey);
	
#if ARCH_64
	DWORD sizeHi=inSize>>32;	/*max size, high-order*/
	DWORD sizeLo=inSize&0xffff; /*max size, low-order*/
#elif ARCH_32
	DWORD sizeHi=0;
	DWORD sizeLo=inSize;
#endif

	HANDLE handle=CreateFileMapping(INVALID_HANDLE_VALUE /*use paging file*/, NULL /*default security attributes*/,
									//PAGE_READWRITE, sizeHi , sizeLo, (LPCSTR)name);
									PAGE_READWRITE, 0 , inSize, (LPCSTR)name);
	fIsNew=true;

	if(handle==NULL && GetLastError()==ERROR_ALREADY_EXISTS)
	{
		handle=OpenFileMapping(FILE_MAP_WRITE ,false /*children don't inherit handle*/, (LPCSTR)name);
		fIsNew=false;
	}

	fShmPtr=SharedHandlePtr(new IPCHandleWrapper(handle));

	return (handle!=NULL) ? VE_OK : vThrowNativeError(GetLastError());
}

VError XWinSharedMemory::Attach()
{
	fAddr=MapViewOfFile(fShmPtr->GetHandle(), FILE_MAP_ALL_ACCESS, 0 /*view offset, high-order*/, 0 /*low-order*/, 0 /*view extends to EOF*/);

	return (fAddr!=NULL) ? VE_OK : vThrowNativeError(GetLastError());
}


VError XWinSharedMemory::Detach()
{
	BOOL res=UnmapViewOfFile(fAddr);

	return res ? VE_OK : vThrowNativeError(GetLastError());
}

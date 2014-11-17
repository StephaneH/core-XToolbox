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
#ifndef __XWinSharedSemaphore__
#define __XWinSharedSemaphore__

#include <memory>

#include "Kernel/VKernel.h"



BEGIN_TOOLBOX_NAMESPACE



class IPCHandleWrapper
{
public :	
	IPCHandleWrapper(HANDLE inHandle=NULL) : fHandle(inHandle) {};
	~IPCHandleWrapper() { if(fHandle!=NULL) CloseHandle(fHandle); }
	HANDLE GetHandle() { return fHandle; }
	
private :
	HANDLE fHandle;
};

typedef std::tr1::shared_ptr<IPCHandleWrapper> SharedHandlePtr;


class XTOOLBOX_API XWinSharedSemaphore : public VObject
{
public:
	XWinSharedSemaphore() : fIsNew(false) {};

	VError	Init(uLONG inKey, uLONG inInitCount=1);	
	bool	IsNew()		{ return fIsNew; }
	VError	Lock();
	VError	Unlock();
  
private:
	SharedHandlePtr fSemPtr;
	bool			fIsNew;
};

typedef XWinSharedSemaphore XSharedSemaphoreImpl;


class XTOOLBOX_API XWinSharedMemory : public VObject
{
public:
	XWinSharedMemory() : fIsNew(false), fAddr(NULL) {}

	VError	Init(uLONG inKey, VSize inSize);
	bool	IsNew()		{ return fIsNew; }
	VError  Attach();
	void*	GetAddr()	{ return fAddr; }
	VError	Detach();

private:
	SharedHandlePtr fShmPtr;
	bool			fIsNew;
	void*			fAddr;
};

typedef XWinSharedMemory XSharedMemoryImpl;



END_TOOLBOX_NAMESPACE



#endif

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
#include "VKernelIPCPrecompiled.h"
#include "XSysVIPC.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>


// ACI0072276

// Both semget() (semaphore) and shmget() (shared memory) will fail getting on existing
// semaphore or shared memory if permission is 0644 (user (who created semaphore) has read/write
// permission, all others can only read). 0666 should be used instead.

#define PERMISSIONS	0666

//(according to the man) On Linux the calling program must define this union :

#if VERSION_LINUX

union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO
                                (Linux-specific) */
};

#endif


VError XSysVSemaphore::Get(uLONG inKey)
{
	//IPC_PRIVATE, which is most of the time 0, is a special value that ask the
	//system for a new semaphore to be created. Our API doesn't allow us to use
	//it properly right now.

	if(inKey==IPC_PRIVATE)
		return VE_INVALID_PARAMETER;

	//According to Posix, the values of the semaphores in a newly created set are indeterminate.
    //According to man, Linux, like many other implementations, initializes the semaphore values to 0
	//(which mean you can't take it, and prevent any race condition on Init())
	//Not sure how OS X behaves...

    fId=semget(inKey, 1, IPC_CREAT|IPC_EXCL|PERMISSIONS);
	fIsNew=true;

	if(fId<0 & errno==EEXIST)
	{
		fId=semget(inKey, 1, PERMISSIONS);
		fIsNew=false;
	}

	return (fId!=-1) ? VE_OK : vThrowNativeError(errno);
}


VError XSysVSemaphore::Init(uLONG inInitCount)
{
	semun arg;
	arg.val=inInitCount;
	
	int res=semctl(fId, 0 /*fisrt and only sem*/, SETVAL, arg);
	
	return (res!=-1) ? VE_OK : vThrowNativeError(errno);
}


VError XSysVSemaphore::Init(uLONG inKey, uLONG inInitCount)
{
	VError verr=Get(inKey);
	
	if(verr==VE_OK)
		verr=Init(inInitCount);
	
	return verr;
}


VError XSysVSemaphore::Lock()
{
	struct sembuf op;
	//Decrement first sem, with auto undo on exit/crash.
	op.sem_num=0, op.sem_op=-1, op.sem_flg=SEM_UNDO;

	int res=semop(fId, &op, 1 /*one operation*/);

	return (res!=-1) ? VE_OK : vThrowNativeError(errno);
}


VError XSysVSemaphore::Unlock()
{
	struct sembuf op;
	//Increment first sem, with auto undo on exit/crash.
	op.sem_num=0, op.sem_op=1, op.sem_flg=SEM_UNDO;

	int res=semop(fId, &op, 1 /*one operation*/);

	return (res!=-1) ? VE_OK : vThrowNativeError(errno);
}


VError XSysVSemaphore::Remove()
{
	int res=semctl(fId, 0 /*fisrt and only sem*/, IPC_RMID);

	fId=-1;

	return (res!=-1) ? VE_OK : vThrowNativeError(errno);
}


VError XSysVSharedMemory::Init(uLONG inKey, VSize inSize)
{
	if(inKey==IPC_PRIVATE)
		return VE_INVALID_PARAMETER;

	//(According to Posix) When the shared memory segment is created, it shall be initialized with all zero values.
    fId=shmget(inKey, inSize, IPC_CREAT|IPC_EXCL|PERMISSIONS);
	fIsNew=true;

	if(fId<0 & errno==EEXIST)
	{
		fId=shmget(inKey, inSize, IPC_CREAT|PERMISSIONS);
		fIsNew=false;
	}

	return (fId!=-1) ? VE_OK : vThrowNativeError(errno);
}


VError XSysVSharedMemory::Attach()
{
	fAddr=shmat(fId, NULL, 0 /*no specific flag : attach in RW mode*/);

	/*that's weird, but shmat returns -1 on error*/
	return (fAddr!=(void*)-1) ? VE_OK : vThrowNativeError(errno);
}


void* XSysVSharedMemory::GetAddr()
{
	return (fAddr!=(void*)-1) ? fAddr : NULL;
}


//static
VError XSysVSharedMemory::Detach(void* inAddr)
{
	int res=shmdt(inAddr);
	
	return (res!=-1) ? VE_OK : vThrowNativeError(errno);
}


VError XSysVSharedMemory::Detach()
{
	return Detach(fAddr);
}


VError XSysVSharedMemory::Remove()
{
	int res=shmctl(fId, IPC_RMID, NULL);

	fId=-1;

	return (res!=-1) ? VE_OK : vThrowNativeError(errno);
}

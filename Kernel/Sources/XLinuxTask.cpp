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


#include "VTask.h"	//indirect include of "XLinuxTask.h"
#include "VError.h"


#include <time.h>
#include <pthread.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/prctl.h>

#if USE_POSIX_UNNAMED_SEMAPHORE
# include <semaphore.h>
#elif USE_MACH_SEMAPHORE
# include <mach/mach.h>
#endif


////////////////////////////////////////////////////////////////////////////////
//
// XLinuxTask
//
////////////////////////////////////////////////////////////////////////////////

XLinuxTask::XLinuxTask(VTask* inOwner, bool /*inMainTask*/)
	: fOwner(inOwner)
{
}


XLinuxTask::~XLinuxTask()
{
	//Nothing to do
}


//static
size_t XLinuxTask::GetCurrentFreeStackSize()
{
	char* stackAddr;

	XLinuxTask_preemptive::GetStack((void**)&stackAddr, NULL);

	return ((char*)&stackAddr > stackAddr ? (char*)&stackAddr - stackAddr : stackAddr - (char*)&stackAddr);
}


XLinuxTaskMgr* XLinuxTask::GetManager() const
{
	return VTaskMgr::Get()->GetImpl();
}


VTask* XLinuxTask::GetOwner() const
{
	return fOwner;
}


//virtual
bool XLinuxTask::GetCPUUsage(Real& outCPUUsage) const
{
	// Used in VTask::GetCPUUsage
	xbox_assert(false && "GetCPUUsage - Postponed Linux Implementation !");
	return false;
}


//virtual
bool XLinuxTask::GetCPUTimes(Real& outSystemTime, Real& outUserTime) const
{
	// Used in VTask::GetCPUTimes
	xbox_assert(false && "GetCPUTimes - Postponed Linux Implementation !");
	return false;
}


void XLinuxTask::_Run()
{
	XLinuxTaskMgr* mgr=GetManager();
	xbox_assert(mgr!=NULL);

	VTask* owner=GetOwner();
	xbox_assert(owner!=NULL);

	mgr->SetCurrentTask(owner);

	try
	{
		fOwner->_Run();
	}
	catch(...)
	{
		throw;
	}

	Exit();
}


//virtual
VTaskID	XLinuxTask::GetValueForVTaskID() const
{
	//Active waiting and no explicit atomic stuff (discussed with L.E.)
	while(GetSystemID()==-1)
		pthread_yield();	//Do not fail on Linux

	return GetSystemID();
}



////////////////////////////////////////////////////////////////////////////////
//
// SemWrapper : a wrapper for Posix/Mach semaphore API
//
////////////////////////////////////////////////////////////////////////////////

#if USE_POSIX_UNNAMED_SEMAPHORE

XLinuxTask_preemptive::SemWrapper::SemWrapper()
{
	memset(&fSem, 0, sizeof(fSem));
}

int XLinuxTask_preemptive::SemWrapper::Init(uLONG val)
{
	int res=sem_init(&fSem, 0/*not shared between process*/, 0 /*init. count*/);
	xbox_assert(res==0);

	return res;
}

int XLinuxTask_preemptive::SemWrapper::Post()
{
	int res=sem_post(&fSem);
	xbox_assert(res==0);

	return res;
}

int XLinuxTask_preemptive::SemWrapper::Wait()
{
	int res=sem_wait(&fSem);
	xbox_assert(res==0);

	return res;
}

int XLinuxTask_preemptive::SemWrapper::Destroy()
{
	int res=sem_destroy(&fSem);
	xbox_assert(res==0);

	return res;
}

//static
bool XLinuxTask_preemptive::SemWrapper::IsOk(int res)
{
	return res==0;
}

#elif USE_MACH_SEMAPHORE

XLinuxTask_preemptive::SemWrapper::SemWrapper()
{
	memset(&fSem, 0, sizeof(fSem));
}

int XLinuxTask_preemptive::SemWrapper::Init(uLONG val)
{
	kern_return_t kres=semaphore_create(mach_task_self(), &fSem, SYNC_POLICY_FIFO, 0);
	xbox_assert(kres==KERN_SUCCESS);

	return static_cast<int>(kres);
}

int XLinuxTask_preemptive::SemWrapper::Post()
{
	kern_return_t kres=semaphore_signal(fSem);
	xbox_assert(kres==KERN_SUCCESS);

	return static_cast<int>(kres);
}

int XLinuxTask_preemptive::SemWrapper::Wait()
{
	kern_return_t kres=semaphore_wait(fSem);
	xbox_assert(kres==KERN_SUCCESS);

	return static_cast<int>(kres);
}

int XLinuxTask_preemptive::SemWrapper::Destroy()
{
	kern_return_t kres=semaphore_destroy(mach_task_self(), fSem);
	xbox_assert(kres==KERN_SUCCESS);

	return static_cast<int>(kres);
}

//static
bool XLinuxTask_preemptive::SemWrapper::IsOk(int res)
{
	return res==KERN_SUCCESS;
}

#endif



////////////////////////////////////////////////////////////////////////////////
//
// XLinuxTask_preemptive
//
////////////////////////////////////////////////////////////////////////////////

XLinuxTask_preemptive *XLinuxTask_preemptive::Create( VTask* inOwner)
{
	XLinuxTask_preemptive *task = new XLinuxTask_preemptive( inOwner);
	if (!task->_CreateThread())
	{
		delete task;
		task = NULL;
	}
	return task;
}


//This ctor is only used by main task ?
XLinuxTask_preemptive::XLinuxTask_preemptive(VTask* inOwner, bool unUsedFromMainTask) :
	XLinuxTask(inOwner, true), fPthread(0)
{
	fTid=syscall(SYS_gettid);	//gettid() is always successful.

	fSem.Init(0);
	fSem.Post();
}


//This ctor is nerver used by main task ?
XLinuxTask_preemptive::XLinuxTask_preemptive(VTask* inOwner) :
	XLinuxTask(inOwner, false), fTid(-1)
{
	fSem.Init(0);
}


XLinuxTask_preemptive::~XLinuxTask_preemptive()
{
	if(fPthread!=0)
	{
		int res=pthread_join(fPthread, NULL);
		xbox_assert(res==0);
	}

	fSem.Destroy();
}


void XLinuxTask_preemptive::Exit(VTask *unusedDestTask)
{
	VTask* owner=GetOwner();
	xbox_assert(owner!=NULL);

	owner->_SetStateDead();

	pthread_exit(NULL); //never returns
}


//static
void* XLinuxTask_preemptive::readySteadyGO(void* thisPtr)
{
	xbox_assert(thisPtr!=NULL);

	XLinuxTask_preemptive* task=static_cast<XLinuxTask_preemptive*>(thisPtr);

	//Parent thread is waiting (looping) for this ID. (Discussed with L.E.)
	task->fTid=syscall(SYS_gettid);	//gettid() is always successful.

	task->Freeze();

	task->_Run();   //never returns

	return NULL;
}


bool XLinuxTask_preemptive::_CreateThread()
{
	bool ok;
	pthread_attr_t attr;
	int res=pthread_attr_init(&attr);
	xbox_assert(res==0);

	//Emulate mac behabior
	res=pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
	xbox_assert(res==0);

	VTask* owner=GetOwner();
	xbox_assert(owner!=NULL);

	size_t stackSize=owner->GetStackSize();

	stackSize=VSystem::RoundUpVMPageSize(stackSize);

	if(stackSize<PTHREAD_STACK_MIN)
		stackSize=PTHREAD_STACK_MIN;

	res=pthread_attr_setstacksize(&attr, stackSize);
	xbox_assert(res==0);

	//As long as we do not play with the stack adress, the (standard) default
	//behavior of pthread is to put a gard page to detect stack overflow.

	res=pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	xbox_assert(res==0);

	res=pthread_create(&fPthread, &attr, readySteadyGO, this);
	xbox_assert(res==0);
	if (res == 0)
	{
		ok = true;
	}
	else
	{
		vThrowPosixError(res);
		ok = false;
	}

	res=pthread_attr_destroy(&attr);
	xbox_assert(res==0);

	return ok;
}


//virtual
bool XLinuxTask_preemptive::Run()
{
	fSem.Post();

	return true;
}


//virtual
void XLinuxTask_preemptive::WakeUp()
{
	fSem.Post();
}


//virtual
void XLinuxTask_preemptive::Freeze()
{
	fSem.Wait();
}


//virtual
void XLinuxTask_preemptive::Sleep(sLONG inMilliSeconds)
{
	timespec  timeA;
	timespec  timeB;

	timespec* waitPtr	   = &timeA;
	timespec* remainingPtr  = &timeB;
	timespec* tmpPtr		= NULL;

	waitPtr->tv_sec  = inMilliSeconds/1000;
	waitPtr->tv_nsec = (inMilliSeconds%1000)*1000000;

	int res;

	do
	{
		res=nanosleep(waitPtr, remainingPtr);

		tmpPtr=waitPtr, waitPtr=remainingPtr, remainingPtr=tmpPtr;
	}
	while(res==-1 && errno==EINTR);

	xbox_assert(res==0);
}


//virtual
bool XLinuxTask_preemptive::CheckSystemMessages()
{
	return false;
}


//static
void XLinuxTask_preemptive::GetStack(void** outStackAddr, size_t* outStackSize)
{
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	int res = pthread_getattr_np(pthread_self(), &attr);
	xbox_assert(res==0);

	void*   stackAddr = NULL;
	size_t  stackSize = 0;

	res = pthread_attr_getstack(&attr, &stackAddr, &stackSize);
	xbox_assert(res == 0);

	if (outStackAddr)
		*outStackAddr = stackAddr;

	if (outStackSize)
		*outStackSize = stackSize;

	pthread_attr_destroy(&attr);
}


//virtual
sLONG XLinuxTask_preemptive::GetSystemID() const
{
	 return fTid;	//pid_t -> signed 32 bit int
}



////////////////////////////////////////////////////////////////////////////////
//
// XLinuxTaskMgr
//
////////////////////////////////////////////////////////////////////////////////

XLinuxTaskMgr::XLinuxTaskMgr()
{
	int res=pthread_key_create(&fSlotCurrentTask, NULL);
	xbox_assert(res==0);

	res=pthread_key_create(&fSlotCurrentTaskID, NULL);
	xbox_assert(res==0);

	VTaskID* tid=new VTaskID();
	xbox_assert(tid!=NULL);

	res=pthread_setspecific(fSlotCurrentTaskID, tid);
	xbox_assert(res==0);
}


//virtual
XLinuxTaskMgr::~XLinuxTaskMgr()
{
	int res=pthread_key_delete(fSlotCurrentTask);
	xbox_assert(res==0);

	VTaskID* tid=reinterpret_cast<VTaskID*>(pthread_getspecific(fSlotCurrentTaskID));
	xbox_assert(tid!=NULL);

	if(tid) delete tid;

	res=pthread_key_delete(fSlotCurrentTaskID);
	xbox_assert(res==0);
}


bool XLinuxTaskMgr::Init(VProcess* inProcess) const
{
	//Place holder for fiber stuff we don't need anymore
	return true;
}



void XLinuxTaskMgr::Deinit() const
{
	//empty
}



void XLinuxTaskMgr::TaskStarted(VTask* inTask) const
{
	//Place holder for Windows specific thread issues
}


void XLinuxTaskMgr::TaskStopped(VTask* inTask) const
{
	//Place holder for Windows specific thread issues
}


//static
void XLinuxTaskMgr::YieldNow()
{
	sched_yield();
}


bool XLinuxTaskMgr::IsFibersThreadingModel() const
{
	return false;
}


size_t XLinuxTaskMgr::ComputeMainTaskStackSize() const
{
	size_t size=0;

	XLinuxTask_preemptive::GetStack(NULL, &size);

	return size;
}


XLinuxTask* XLinuxTaskMgr::Create(VTask* inOwner, ETaskStyle inStyle)
{
	xbox_assert(inStyle==eTaskStylePreemptive);

	XLinuxTask* linuxTask=NULL;

	if(inStyle==eTaskStylePreemptive)
		linuxTask=XLinuxTask_preemptive::Create(inOwner);

	return linuxTask;
}


XLinuxTask* XLinuxTaskMgr::CreateMain(VTask* inOwner)
{
	XLinuxTask* task=new XLinuxTask_preemptive(inOwner, true);
	inOwner->SetCanBlockOnSyncObject(true);	//BUG ICI ???

	SetCurrentTask(inOwner);

	return task;
}


size_t XLinuxTaskMgr::GetCurrentFreeStackSize() const
{
	return XLinuxTask::GetCurrentFreeStackSize();
}


VTask* XLinuxTaskMgr::GetCurrentTask() const
{
	return static_cast<VTask*>(pthread_getspecific(fSlotCurrentTask));
}


void XLinuxTaskMgr::SetCurrentTask(VTask *inTask) const
{
	int res=pthread_setspecific(fSlotCurrentTask, inTask);
	xbox_assert(res==0);
}


VTaskID XLinuxTaskMgr::GetCurrentTaskID() const
{
	VTaskID* tid=reinterpret_cast<VTaskID*>(pthread_getspecific(fSlotCurrentTaskID));
	xbox_assert(tid!=NULL);

	return *tid;
}


void XLinuxTaskMgr::SetCurrentTaskID(VTaskID inTaskID) const
{
	VTaskID* tid=reinterpret_cast<VTaskID*>(pthread_getspecific(fSlotCurrentTaskID));
	xbox_assert(tid!=NULL);

	*tid=inTaskID;
}


void XLinuxTaskMgr::SetCurrentThreadName(const VString& inName, VTaskID inTaskID) const
{
	VStringConvertBuffer buffer(inName, VTC_UTF_8);

	//Thread name are limited to 15 char
	char name[16];
	strncpy(name, buffer.GetCPointer(), sizeof(name)-1);
	name[sizeof(name)-1]='\0';

	//Need ubuntu 13.10 for pthread_setname_np :(
	//int res=pthread_setname_np(pthread_self(), name);

	int res=prctl(PR_SET_NAME, (unsigned long)name, 0, 0, 0);
	xbox_assert(res==0);
}



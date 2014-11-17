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
#ifndef __XLinuxTask__
#define __XLinuxTask__


#define USE_POSIX_UNNAMED_SEMAPHORE 1
#define USE_LINUX_PTHREAD_NP 1


#include "Kernel/Sources/VObject.h"
#include <pthread.h>
#include <semaphore.h>



BEGIN_TOOLBOX_NAMESPACE


// Define bellow
class XLinuxSyncObject;

// Needed declarations
class VTask;
class VArrayLong;
class VProcess;
class XLinuxTaskMgr;

typedef pthread_t SystemTaskID;



class XLinuxTask : public VObject
{
public:
	XLinuxTask(VTask *inOwner, bool inMainTask);
	~XLinuxTask();

	virtual bool	Run() = 0;
	virtual void	WakeUp() = 0;
	virtual void	Freeze() = 0;
	virtual void	Sleep(sLONG inMilliSeconds) = 0;
	virtual bool	CheckSystemMessages() = 0;
	virtual void	Exit(VTask *unusedDestTask=NULL) = 0;

	static  size_t  GetCurrentFreeStackSize();

	virtual bool	GetCPUUsage(Real& outCPUUsage) const;
	virtual bool	GetCPUTimes(Real& outSystemTime, Real& outUserTime) const;

	XLinuxTaskMgr*  GetManager() const;
	VTask*		  GetOwner() const;

	virtual sLONG	GetSystemID() const = 0;
	virtual VTaskID	GetValueForVTaskID() const; //Wrapper for GetSystemID (active waiting !)

protected :

	void	_Run();

private:

	VTask*  fOwner;

};


class XLinuxTask_preemptive : public XLinuxTask
{
public:
	static XLinuxTask_preemptive* Create(VTask* inOwner);

	XLinuxTask_preemptive(VTask* inOwner, bool /*for main task*/);
	~XLinuxTask_preemptive();

	virtual void Exit(VTask *unusedDestTask=NULL);
	virtual bool Run();
	virtual void WakeUp();
	virtual void Freeze();
	virtual void Sleep(sLONG inMilliSeconds);
	virtual bool CheckSystemMessages();

	static  void GetStack(void** outStackAddr, size_t* outStackSize);

	virtual sLONG GetSystemID() const;

private:

	XLinuxTask_preemptive(VTask* inOwner);
	static void* readySteadyGO(void* thisPtr);
	bool _CreateThread();

	class SemWrapper
	{
	public :

		SemWrapper();
		int Init(uLONG val=0);
		int Post();
		int Wait();
		int Destroy();

		static bool IsOk(int res);

	private :

#if USE_POSIX_UNNAMED_SEMAPHORE
		sem_t fSem;
#elif USE_MACH_SEMAPHORE
		semaphore_t fSem;
#else
		#error You must choose a semaphore family !
#endif
	};

	pthread_t	fPthread;	//opaque pthread type, 8 bytes
	pid_t		fTid;		//Linux thread id, 4 bytes, signed
	SemWrapper  fSem;
};


class XTOOLBOX_API XLinuxTaskMgr
{
 public:
	XLinuxTaskMgr();
	virtual ~XLinuxTaskMgr();

	bool		 Init(VProcess* inProcess) const;
	void		 Deinit() const;

	void		 TaskStarted(VTask *inTask) const;
	void		 TaskStopped(VTask *inTask) const;

	static  void YieldNow();

	bool		 IsFibersThreadingModel() const;

	size_t	   ComputeMainTaskStackSize() const;

	XLinuxTask*  Create(VTask* inOwner, ETaskStyle inStyle);
	XLinuxTask*  CreateMain(VTask* inOwner);


	size_t	   GetCurrentFreeStackSize() const;


	VTask*	   GetCurrentTask() const;
	void		 SetCurrentTask(VTask *inTask) const;


	VTaskID	  GetCurrentTaskID() const;
	void		 SetCurrentTaskID(VTaskID inTaskID) const;


	void		 SetCurrentThreadName(const VString& inName, VTaskID inTaskID) const;


 protected:
	pthread_key_t   fSlotCurrentTask;
	pthread_key_t	fSlotCurrentTaskID;

};


typedef XLinuxTask	  XTaskImpl;
typedef XLinuxTaskMgr   XTaskMgrImpl;


END_TOOLBOX_NAMESPACE

#endif

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
#ifndef __XMacTask__
#define __XMacTask__

#include "Kernel/Sources/VObject.h"
#include "Kernel/Sources/XMacFiber.h"

BEGIN_TOOLBOX_NAMESPACE

typedef struct TaskTimerInfo
{
	TMTask			fTimer;
	class XMacTask_cooperative*	fThread;
} TaskTimerInfo;


// Define bellow
class XMacSyncObject;

// Needed declarations
class VTask;
class VArrayLong;
class VProcess;
class XMacTaskMgr;

typedef pthread_t SystemTaskID;

class _FiberData
{
public:
										_FiberData( VTask *inTask);
	
	static	_FiberData*					Current()									{ return reinterpret_cast<_FiberData*>( ::GetFiberData());}

			VTaskID						GetTaskID()									{ return fTaskID;}

			VTask*						GetTask()									{ return fTask;}
			void						SetTask( VTask *inTask);

private:
			VTask*						fTask;
			VTaskID						fTaskID;	// at the very beginning, VTask may be NULL so we have this fTaskID field to avoid having to test fTask != NULL
};


class XMacTask : public VObject
{
friend class XMacTask_fiber;
public:
										XMacTask( VTask *inOwner, bool inMainTask);
										~XMacTask();

	virtual bool						Run() = 0;
	virtual void						WakeUp() = 0;
	virtual void						Freeze() = 0;
	virtual void						Sleep( sLONG inMilliSeconds) = 0;
	virtual	bool						CheckSystemMessages() = 0;
	virtual	void						Exit( VTask *inDestinationTask) = 0;

			void						SwitchToFiber()										{ ::SwitchToFiber( fFiber);}
			size_t						GetCurrentFreeStackSize();
			XMacTaskMgr*				GetManager() const;
			VTask*						GetOwner() const									{ return fOwner;}
			VTask*						GetFiberGroupOwner() const							{ return fFiberGroupOwner;}
			size_t						GetStackSize() const								{ return fStackSize;}
			bool						IsMainTask() const									{ return fMainTask;}
			pthread_t					GetSystemID() const									{ return fPThreadID;}
	virtual	VTaskID						GetValueForVTaskID() const;
	virtual	bool						GetCPUUsage(Real& outCPUUsage) const;
	virtual bool						GetCPUTimes(Real& outSystemTime, Real& outUserTime) const;
			

protected:
			void						SetFiber( void *inFiber, VTask *inFiberGroupOwner)	{ fFiber = inFiber; fFiberGroupOwner = inFiberGroupOwner; }
			void						SetStackSize( size_t inSize)						{ fStackSize = inSize;}
			void						_Run();
			
			pthread_t					fPThreadID; //This value is not very useful when using longjmp threads. Be careful.
			mach_port_t					fMachThreadPort;

private:
			void*						fFiber;
			VTask*						fOwner;
			VTask*						fFiberGroupOwner;
			size_t						fStackSize;
			char*						fStackAddress;
			bool						fMainTask;
};


class XMacTask_preemptive : public XMacTask
{
public:
	static	XMacTask_preemptive*		Create( VTask* inOwner);

										XMacTask_preemptive( VTask* inOwner, bool /*for main task*/);
										~XMacTask_preemptive();

	virtual bool						Run();
	virtual void						WakeUp();
	virtual void						Freeze();
	virtual void						Sleep( sLONG inMilliSeconds);
	virtual	bool						CheckSystemMessages()						{ return false;}					
	virtual void						Exit( VTask *inDestinationTask);

	virtual	VTaskID						GetValueForVTaskID() const;

private:
										XMacTask_preemptive( VTask* inOwner);
	static	void*						_ThreadProc( void* inParam);
			bool						_CreateThread();

			vm_address_t				fAllocatedStackAddress;
			size_t						fAllocatedStackSize;
			semaphore_t					fSema;
};


class XMacTask_fiber : public XMacTask
{
public:
										XMacTask_fiber( VTask *inOwner);
	virtual								~XMacTask_fiber();

	virtual bool						Run();
	virtual void						WakeUp();
	virtual void						Freeze();
	virtual void						Sleep( sLONG inMilliSeconds);
	virtual bool						CheckSystemMessages();
	virtual void						Exit( VTask *inDestinationTask);

	
private:
	static	void						_ThreadProc( void *inParam);
			
			sLONG						fSleepStamp;
};


class XTOOLBOX_API XMacTaskMgr
{
public:
										XMacTaskMgr();
	virtual								~XMacTaskMgr();

			bool						Init( VProcess* inProcess);
			void						Deinit();
			void						TaskStarted( VTask *inTask);
			void						TaskStopped( VTask *inTask);
			
	static	void						YieldNow()										{ sched_yield(); }
			void						YieldToFiber( VTask* inPreferredTask);
			void						YieldFiber();

			size_t						ComputeMainTaskStackSize() const;
			
			XMacTask*					Create( VTask* inOwner, ETaskStyle inStyle);
			XMacTask*					CreateMain( VTask* inOwner);

			bool						IsOneThreadRunning();

		 	size_t						GetCurrentFreeStackSize() const;
	static 	size_t						GetCurrentFreeStackSizeStatic();
	
			void*						GetThreadManagerTaskRef() const					{ return fTaskRef; }

	static	sLONG						sMacNoYield;

			VTask*						GetCurrentTask() const							{ return static_cast<VTask*>( ::pthread_getspecific( fSlot_CurrentTask));}
			void						SetCurrentTask( VTask *inTask) const			{ ::pthread_setspecific( fSlot_CurrentTask, inTask);}

			VTaskID						GetCurrentTaskID() const						{ return (VTaskID) reinterpret_cast<sLONG_PTR>( ::pthread_getspecific( fSlot_CurrentTaskID));}
			void						SetCurrentTaskID( VTaskID inTaskID) const		{ ::pthread_setspecific( fSlot_CurrentTaskID, reinterpret_cast<void *>( inTaskID));}
	
			bool						WithFibers() const								{ return fWithFibers; }

			bool						IsFibersThreadingModel();

			void						SetCurrentThreadName( const VString& inName, VTaskID inTaskID) const;
	
protected:
			pthread_key_t				fSlot_CurrentTask;
			pthread_key_t				fSlot_CurrentTaskID;

			ThreadTaskRef				fTaskRef;
			bool						fWithFibers;
			sLONG						fThreadingModel;
};



typedef XMacTask			XTaskImpl;
typedef XMacTaskMgr			XTaskMgrImpl;

END_TOOLBOX_NAMESPACE

#endif
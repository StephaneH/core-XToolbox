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
#ifndef __XWinTask__
#define __XWinTask__

#include "Kernel/Sources/VObject.h"

BEGIN_TOOLBOX_NAMESPACE

// Needed declaration
class VTask;
class VProcess;
class VSyncObject;
class XWinTaskMgr;

typedef DWORD SystemTaskID;

class _FiberData
{
public:
										_FiberData( VTask *inTask);
	
	static	_FiberData*					Current()										{ return reinterpret_cast<_FiberData*>( ::GetFiberData());}

			VTaskID						GetTaskID()										{ return fTaskID;}

			VTask*						GetTask()										{ return fTask;}
			void						SetTask( VTask *inTask);

private:
			VTask*						fTask;
			VTaskID						fTaskID;	// at the very beginning, VTask may be NULL so we have this fTaskID field to avoid having to test fTask != NULL
};


class XWinTask : public VObject
{
friend class XWinTask_fiber;
public:
										XWinTask( VTask *inOwner, bool inMainTask);
										~XWinTask();

	virtual bool						Run() = 0;
	virtual void						WakeUp() = 0;
	virtual void						Freeze() = 0;
	virtual void						Sleep( sLONG inMilliSeconds) = 0;
	virtual	bool						CheckSystemMessages() = 0;
	virtual	DWORD						GetSystemID() const = 0;
	virtual	HANDLE						GetThreadHandle() const = 0;
	virtual void						Exit( VTask *inDestinationTask) = 0;
	

			void						SwitchToFiber()										{ ::SwitchToFiber( fFiber);}
			size_t						GetCurrentFreeStackSize();
	static	XWinTaskMgr*				GetManager();
			VTask*						GetOwner() const									{ return fOwner;}
			VTask*						GetFiberGroupOwner() const							{ return fFiberGroupOwner;}
			size_t						GetStackSize() const								{ return fStackSize;}
			bool						IsMainTask() const									{ return fMainTask;}
	virtual	bool						GetCPUUsage(Real& outCPUUsage) const;
	virtual bool						GetCPUTimes(Real& outSystemTime, Real& outUserTime) const;
	virtual	VTaskID						GetValueForVTaskID() const;

protected:
			void						SetFiber( LPVOID inFiber, VTask *inFiberGroupOwner)	{ fFiber = inFiber; fFiberGroupOwner = inFiberGroupOwner; }
			void						SetStackSize( size_t inSize)						{ fStackSize = inSize;}
			void						_Run();

private:
			LPVOID						fFiber;
			VTask*						fOwner;
			VTask*						fFiberGroupOwner;
			size_t						fStackSize;
			char*						fStackAddress;
			bool						fMainTask;
};


class XWinTask_preemptive : public XWinTask
{
public:
	static	XWinTask_preemptive*		Create( VTask* inOwner);

										XWinTask_preemptive( VTask* inOwner, bool /*for main task*/);
										~XWinTask_preemptive();

	virtual bool						Run();
	virtual void						WakeUp();
	virtual void						Freeze();
	virtual void						Sleep( sLONG inMilliSeconds);
	virtual	bool						CheckSystemMessages();
	virtual	DWORD						GetSystemID() const								{ return fSystemID; }
	virtual	HANDLE						GetThreadHandle() const							{ return fThread; }
	virtual void						Exit( VTask *inDestinationTask);

	virtual	bool						GetCPUUsage(Real& outCPUUsage) const;
	virtual bool						GetCPUTimes(Real& outSystemTime, Real& outUserTime) const;
	virtual	VTaskID						GetValueForVTaskID() const;

private:
										XWinTask_preemptive( VTask* inOwner);
	static	UINT APIENTRY				_ThreadProc (void* inParam);
			bool						_CreateThread();

			HANDLE						fThread;
			DWORD						fSystemID;
};


class XWinTask_fiber : public XWinTask
{
public:
										XWinTask_fiber( VTask *inOwner);
										~XWinTask_fiber();

	virtual bool						Run();
	virtual void						WakeUp();
	virtual void						Freeze();
	virtual void						Sleep( sLONG inMilliSeconds);
	virtual bool						CheckSystemMessages();
	virtual void						Exit( VTask *inDestinationTask);

	virtual DWORD						GetSystemID() const								{ return 0; }
	virtual	HANDLE						GetThreadHandle() const							{ return NULL; }

private:
	static	VOID CALLBACK				_ThreadProc( LPVOID inParam);

			sLONG						fSleepStamp;
};



class XWinTaskMgr
{
friend class XWinTask;
public:
										XWinTaskMgr();
										~XWinTaskMgr();

			bool						Init( VProcess* inProcess);
			void						Deinit( );
			
			void						TaskStarted( VTask* inTask);
			void						TaskStopped( VTask* inTask);

	static	void						YieldNow()										{ ::Sleep( 0);}
			void						YieldToFiber( VTask* inPreferredTask);
			void						YieldFiber()									{;}

			size_t						ComputeMainTaskStackSize() const;

			XWinTask*					Create( VTask* inOwner, ETaskStyle inStyle);
			XWinTask*					CreateMain( VTask* inOwner);

		 	size_t						GetCurrentFreeStackSize() const;
	static 	size_t						GetCurrentFreeStackSizeStatic();

			VTask*						GetCurrentTask() const							{ return static_cast<VTask*>( ::TlsGetValue( fSlot_CurrentTask));}
			void						SetCurrentTask( VTask *inTask) const			{ ::TlsSetValue( fSlot_CurrentTask, inTask);}

//#pragma warning (disable: 4311)	// warning C4311: 'reinterpret_cast' : pointer truncation from 'LPVOID' to 'xbox::VTaskID'
//#pragma warning (disable: 4312) // warning C4312: 'reinterpret_cast' : conversion from 'xbox::VTaskID' to 'void *' of greater size
			VTaskID						GetCurrentTaskID() const						{ return (VTaskID) ((char *) ::TlsGetValue( fSlot_CurrentTaskID) - (char *) 0);}
			void						SetCurrentTaskID( VTaskID inTaskID) const		{ ::TlsSetValue( fSlot_CurrentTaskID, (char *) 0 + inTaskID);}
//#pragma warning (restore: 4311)
//#pragma warning (restore: 4312)

			bool						WithFibers() const								{ return fWithFibers; }

			bool						IsFibersThreadingModel();

			void						SetCurrentThreadName( const VString& inName, VTaskID inTaskID) const;
			
protected:
			DWORD						fSlot_CurrentTask;
			DWORD						fSlot_CurrentTaskID;

			bool						fWithFibers;
};


typedef XWinTask			XTaskImpl;
typedef XWinTaskMgr			XTaskMgrImpl;

END_TOOLBOX_NAMESPACE

#endif

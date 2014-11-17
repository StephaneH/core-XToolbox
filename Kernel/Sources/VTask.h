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
#ifndef __VTask__
#define __VTask__

#include <stack>
#include "Kernel/Sources/VObject.h"
#include "Kernel/Sources/IRefCountable.h"
#include "Kernel/Sources/VMessage.h"


#if VERSIONMAC
typedef struct OpaqueWindowPtr * WindowRef;
#endif


BEGIN_TOOLBOX_NAMESPACE

// Defined bellow
class VTask;
class VTaskMgr;
class VValueBag;

#define TaskNeedSession (void*)0x44444443

// Needed declarations
class VCriticalSection;
class VErrorTaskContext;
class VErrorContext;
class VErrorBase;
class IIdleable;
class VString;
class IMessageable;
class VMessage;
class VMessageQueue;
class XTaskMgrMutexImpl;
class VIntlMgr;
class VSyncEvent;

typedef enum
{
	eTaskStylePreemptive = 1,			// native threading, one event queue for each (mac, win)
	eTaskStyleCooperative = 2,			// only one at a time, automatic scheduling, shared event queue (mac only)
	eTaskStyleFiber = 3					// only one at a time, manual scheduling, shared event queue (mac, win)
} ETaskStyle;

typedef void (*VTaskDataKeyDisposeProc)( void *inData);

typedef struct VTaskDataKeyDesc
{
	VTaskDataKeyDisposeProc	fDisposeProc;
	bool					fFree;
} VTaskDataKeyDesc;

typedef std::vector<void*>				VTaskDataVector;
typedef std::vector<VTaskDataKeyDesc>	VTaskDataKeyDescVector;
typedef VTaskDataVector::size_type		VTaskDataKey;

/*
	Interface prototype for a custom scheduler for fibers
*/
class IFiberScheduler : public IRefCountable
{
public:
	virtual	void	Yield() = 0;
	virtual	void	YieldNow() = 0;
	virtual	void	WaitFlag( sLONG *inValuePtr, sLONG inValue) = 0;
	virtual	void	WaitFlagWithTimeout( sLONG *inValuePtr, sLONG inValue, sLONG inTimeoutMilliseconds) = 0;
	virtual	void	SetSystemEventPolling( bool inSet) = 0;
	virtual	void	PollAndExecuteUpdateEvent() = 0;
	virtual	void	SetDirectUpdate( bool inIsOn) = 0;
#if VERSIONMAC
	virtual void	SetSystemCallCFRunloop(bool inValue) = 0;
	virtual bool	ExecuteInProcess0() = 0;
	//La WebArea a besoin que tous les get events soient traiter dans le process 0. Quand une action risque de faire des get event en dehors du process0 (HIViewClick, PopUpMenuSelect, Growwindow etc ...)
	//On bascule dans le process 0 pour executer Ce genre d'action (HIViewClick, PopUpMenuSelect, Growwindow etc ...).
	//ExecuteInProcess0 renvoie true si une WebArea exist et que l'option d'execution dans le process 0 des appel de la web area est active.
#endif

	// tell Direct Update is allowed even if the process is waiting for a flag.
	// necessary when forwarding some Tracking to main task.
	virtual	void	SetDirectUpdateAllowedEvenIfWaitingForFlag( bool inAllowed) = 0;
};


END_TOOLBOX_NAMESPACE

// Platform specific implementations
#if VERSIONMAC
#include "Kernel/Sources/XMacTask.h"
#elif VERSIONWIN
#include "Kernel/Sources/XWinTask.h"
#elif VERSION_DARWIN
#include "Kernel/Sources/XDarwinTask.h"
#elif VERSION_LINUX
#include "Kernel/Sources/XLinuxTask.h"
#endif

BEGIN_TOOLBOX_NAMESPACE

/*!
	@class	VTaskMgr
	@abstract	Task Manager class.
	@discussion

*/

class XTOOLBOX_API VTaskMgr : public VObject
{
public:
	// Singleton access (never NULL)
	static	VTaskMgr*					Get()									{ return &sInstance;}

	// Tasks management support
			bool						KillAllTasks( sLONG inMilliSeconds);
	// Dead tasks are released asynchronously.
	// PurgeDeadTasks is called internnaly by VTaskMgr so you should not have to call it by yourself.
			void						PurgeDeadTasks();

	// Task search support( avoid using a dead task)
			VTask*						RetainTaskByID( const VTaskID inTaskID);	// returns NULL if not found

	// Returns all tasks
			void						GetTasks( std::vector<VRefPtr<VTask> >& outTasks) const;

	// Messaging support
			void						CancelMessages( IMessageable* inTarget);
			bool						RegisterDelayedMessage( VMessage* inMessage, sLONG inDelayMilliseconds);
			sLONG						CountMessagesFor( IMessageable* inTarget);

	// Cooperative model support
			void						YieldNow();

			void						Yield();
			void						YieldToFiber( VTask *inPreferredTask);

			void						SetDirectUpdate( bool inIsOn);
			void						SetSystemEventPolling( bool inSet);
			void						PollAndExecuteUpdateEvent();

	// aliases

	// Accessors
			VTask*						GetMainTask() const						{ return fMainTask; }
			VTask*						GetCurrentTask() const					{ return sInstanceInited ? fImpl.GetCurrentTask() : NULL; }
			VTaskID						GetCurrentTaskID() const;
			sLONG						GetTaskCount(bool inNotDying = false) const;

			size_t						GetStackSizeForMainTask() const;
			size_t						GetCurrentFreeStackSize() const;

			size_t						GetDefaultStackSize() const;
			void						SetDefaultStackSize( size_t inSize);

	// Class utilities
	static	bool						Init( VProcess* inProcess);
	static	void						DeInit();

#if VERSIONMAC
			bool						MAC_IsOneThreadRunning()				{ return fImpl.IsOneThreadRunning(); }

			/** @name Event loop management */
			//@{
			void						MAC_BeginHandlingUpdateEventsOnly();
			void						MAC_StopHandlingUpdateEventsOnly();

			/** @brief Update events timer callback. Do not use this function */
	static	void						MAC_RunLoopTimerCallback(CFRunLoopTimerRef inTimer, void *inInfo);
			//@}
#endif

			bool						IsFibersThreadingModel()				{ return fImpl.IsFibersThreadingModel(); }

			const XTaskMgrImpl*			GetImpl() const							{ return &fImpl;}
			XTaskMgrImpl*				GetImpl()								{ return &fImpl;}

			IFiberScheduler*			RetainFiberScheduler() const;
			void						SetFiberScheduler( IFiberScheduler *inScheduler);

			/* internal : the fiber scheduler must notify the task mgr when a fiber is no more scheduled */
			/* the task state will be set to TS_DEAD */
			void						FiberStoppedBeingScheduled( VTask *inFiber);

			void						CurrentFiberWaitFlag( sLONG *inFlagPtr, sLONG inValue);
			void						CurrentFiberWaitFlagWithTimeout( sLONG *inFlagPtr, sLONG inValue, sLONG inTimeoutMilliseconds);

			// 4D doesn't want VTask to pool system messages but other apps may want it
			void						SetNeedsCheckSystemMessages( bool inNeedsCheckSystemMessages)	{ fNeedsCheckSystemMessages = inNeedsCheckSystemMessages;}
			bool						GetNeedsCheckSystemMessages() const								{ return fNeedsCheckSystemMessages;}

	// The stamp is incremented eachtime a task created or dead
			uLONG						GetStamp() const						{ return fTaskListStamp; }

protected:
friend class VTask;

			void						RegisterTask( VTask* inTask);
			VTaskID						GetNewTaskID( VTask* inTask);

			VTaskDataKey				CreateDataKey( VTaskDataKeyDisposeProc inDisposeProc);
			void						DeleteDataKey( VTaskDataKey inKey);
			bool						CheckDataKey( VTaskDataKey inKey) const;
			void						DisposeData( VTaskDataVector::iterator inBegin, VTaskDataVector::iterator inEnd);

			XTaskMgrMutexImpl*			GetMutex() const						{ return fCriticalSection;}

private:

										VTaskMgr();

			bool						_Init( VProcess* inProcess);
			void						_DeInit();

			void						_TaskStarted( VTask* inTask);	// Never called for the main task
			void						_TaskStopped( VTask* inTask);	// Never called for the main task

			void						_CheckDelayedMessages();

	static  VTaskMgr						sInstance;	// singleton
	static	bool							sInstanceInited;
			VTask*							fMainTask;	// The main task is assumed to be the current task a creation time
	mutable	size_t							fStackSizeForMainTask;
	mutable	size_t							fDefaultStackSize;
			XTaskMgrMutexImpl*				fCriticalSection;
			XTaskMgrImpl					fImpl;
			std::vector<VMessage*>*			fDelayedMessages;	// Synchronized for time sorting
			std::vector<sLONG>*				fDelayedMessagesTimes;
			VTaskID							fSeedFiberTaskID;	// next fiber task ID
			std::stack<VTaskID>				fRecycledFiberTaskIDs;	// dead fibers task ids free to be used again
		 	VTaskDataKeyDescVector			fDataDescriptors;
			IFiberScheduler*				fFiberScheduler;
	mutable	VTaskID							fSeedForeignTaskID;	// id generator for foreign tasks
			bool							fNeedsCheckSystemMessages;	// each VTask checks system messages
			uLONG							fTaskListStamp;

#if VERSIONMAC
			CFRunLoopTimerRef				fTimer;	///< timer for yielding to other fibers
#endif
};


/*!
	@class	VTask
	@abstract	Task class.
	@discussion
		A VTask implements a flow of instructions.

		It may follows the cooperativ or preemptiv model depending on the capabilities
		of the target platform.
		Currently, on MacOS VTask encapsulates a ThreadManager cooperativ thread
		and on Windows, VTask encapsulates a Win32 preemptiv thread.
		Because you may be using cooperativ thread it is necessary to call vYield() periodically.



*/

class XTOOLBOX_API VTask : public VObject, public IMessageable, public IRefCountable
{
public:
	typedef sLONG (*TaskRunProcPtr)( VTask* inTask);

	/*!
		@function	VTask
		@abstract	Creates a VTask.
		@discussion
			This is the main constructor.
			You specify a creator (may be NULL), a stack size and optionnaly a running procedure address.
			Once created, you call VTask::Run to start the thread.
		@param	inCreator A pointer to the object that created the VTask (not used for now).
			Should be replaced with a IMessageable object that would receive a death notification message.
		@param	inStackSize Number of bytes to allocate for the new thread stack.
			If you pass 0, VTaskMgr::GetDefaultTaskSize() is used.
		@param inRunProcPtr The procedure address to execute when you call VTask::Run.
			If you pass NULL, a default procedure is executed.
			This default procedure is as follows:
			DoInit();
			do {
				if (!DoRun())
					Kill();
				else
					ExecuteMessages();
			} while (!killed);
			DoDeinit();


	*/
										VTask( VObject *inCreator, size_t inStackSize, ETaskStyle inStyle, TaskRunProcPtr inRunProcPtr);


	//=========== State support ===========

	/*!
		@function	Run
		@abstract Starts the task.
		@discussion
			Run the task by executing the running procedure given in the constructor.
			See VTask::VTask.
			returns true if the task was successfully created.
	*/
			bool						Run();

	/*!
		@function Sleep
		@abstract Puts the current task into sleep for a certain amount of time.
		@param inMilliSeconds number of milliseconds to put the task into sleep. Must be > 0.
	*/
	static	void						Sleep( sLONG inMilliSeconds = 0);

	/*!
		@function Freeze
		@abstract Stops the current task until its waken up.
	*/
	static	void						Freeze();

	/*!
		@function WakeUp
		@abstract Wakup the task from sleep or freeze.
	*/
			void						WakeUp();

	/*!
		@function Kill
		@abstract Ask the task to kill himself.
		@discussion
			This methods simply sets the internal state of the task to TS_DYING.
			The default running procedure (see VTask::VTask) checks for this condition and exists.
			If you provided your own running procedure you must check this condition by yourself.
	*/
			void						Kill();

	/** @brief lock running task until this task is in TS_DEAD state. Returns true is the task is dead when function returns. **/
			bool						WaitForDeath( sLONG inTimeoutMilliseconds);

	/*
		@function Exit
		@abstract Exit the currently running task.
		@discussion
			This method performs all necessary cleanup and asks the system to stop scheduling this task.
			If the task is a fiber, Exit() returns and it is your responsibility to stop scheduling it.

			The VTask is put is TS_DEAD state and will be deleted when its refcount drops to zero.

		@param inDestinationTask is used only if Exit is called on a fiber task. It is the task that will be
			scheduled after termination.
	*/
	static	void						Exit( VTask *inDestinationTask);


	//========== Error support ==========

	/*!
		@function GetLastError
		@abstract Return last pushed error code
	*/
			VError						GetLastError() const;

			bool						FailedForMemory() const;

	/*!
		@function FlushErrors
		@abstract 	Forget all errors by releasing the error stack on the current task.
	*/
	static	void						FlushErrors();

	/*!
		@function GetErrorContext
		@abstract Returns the error context of current task
	*/
	static	VErrorTaskContext*			GetErrorContext( bool inMayCreateIt);

	/*!
		@function PushError
		@abstract
		@discussion
			Register a new error for the current task, create the error stack if necessary. inError is retained on success.
			you should use ThrowError instead of using PushError directly.
			return true if properly pushed
	*/
	static	bool						PushError( VErrorBase* inError);

	/*!
		@function PushRetainedError
		@abstract
		@discussion
			Same as PushError but the release is done for you
			let you write: PushRetainedError( new VError);
	*/
	static	bool						PushRetainedError( VErrorBase* inError);

	// Push errors from one context into current task error context.
	// Convenient to transfer one task error stack into another.
	// Does nothing if inErrorContext is NULL or empty.
	static	void						PushErrors( const VErrorContext* inErrorContext);


	//============= Message support =============

	/*!
		@function ExecuteMessages
		@abstract See if any message is pending and execute them for the current task
	*/
	static	bool						ExecuteMessages( bool inProcessIdlers = true);
	static	bool						ExecuteMessagesWithTimeout( sLONG inTimeoutMilliseconds, bool inProcessIdlers = true);

	/*!
		@function RetainMessageWithTimeout.
		@abstract Retain first message pending for current task.
	*/
	static	VMessage*					RetainMessageWithTimeout( sLONG inTimeoutMilliseconds);

	/*!
		@function IsAnyMessagePending
		@abstract Tell if at least one message is pending.
	*/
			bool						IsAnyMessagePending() const;

	/*!
		@function TriggerMessageQueue
		@abstract Set the event associated with this task message queue. Useful to return early from ExecuteMessagesWithTimeout.
	*/
			void						SetMessageQueueEvent();
			VSyncEvent*					RetainMessageQueueSyncEvent();

	/*!
		@function CountMessagesFor
		@abstract Count pending messages for specified target
	*/
			sLONG						CountMessagesFor( IMessageable* inTarget) const;

	// Stack size support
			size_t						GetStackSize() const;
			VPtr						GetStackAddress() const								{ return fStackStartAddr;}

	// Accessors
			TaskState					GetState() const;
			VTaskID						GetID() const										{ return fTaskId; }
			SystemTaskID				GetSystemID() const;
			ETaskStyle					GetTaskStyle() const								{ return fStyle;}

	// four-letter code to tell the kind of this task.
			OsType						GetKind() const										{ return fKind;}
			void						SetKind( OsType inKind)								{ fKind = inKind;}

			sLONG_PTR					GetKindData() const									{ return fKindData;}
			void						SetKindData( sLONG_PTR inData)						{ fKindData = inData;}

			bool						IsMain() const										{ return GetMain() == this; }
			bool						IsCurrent() const									{ return GetCurrent() == this; }
			bool						IsInUse()											{ return GetRefCount() > 1; }

	// tells if Kill() has been called on this task.
	// optionnally returns for how long in milliseconds.
			bool						IsDying( sLONG *outDyingSinceMilliseconds = NULL) const;

			bool						GetCPUUsage(Real& outCPUUsage) const;
			bool						GetCPUTimes(Real& outSystemTime, Real& outUserTime) const;

	// if this task tries to Lock a VSyncObject (semaphores & mutexes), one can stop scheduling other fibers that depends on it.
	// default is false for the main task and for non-preemptive tasks.
			bool						CanBlockOnSyncObject() const						{ return fCanBlockOnSyncObject; }
			void						SetCanBlockOnSyncObject( bool inValue)				{ fCanBlockOnSyncObject = inValue; }

	// same thing but is prepared for a NULL VTask*.
	static	bool						CurrentCanBlockOnSyncObject()						{ VTask *task = GetCurrent(); return (task != NULL) ? task->fCanBlockOnSyncObject : true;}

	// if SetName is being called for current thread, update OS data structure (useful for debugging purpose).
	// (windows and OSX doesn't support setting the name from another thread)
			void						SetName( const VString& inName);
			void						GetName( VString& outName) const;

	// properties are a vvaluebag you can get and set (thread-safe).
	// If you want to modify the properties you need to clone the original and modify the copy.
			const VValueBag*			RetainProperties() const;
			void						SetProperties( const VValueBag *inProperties);

	// Associate an idler with a task.
			void						RegisterIdler( IIdleable* inIdler);
	// Remove asscoiation.
			void						UnregisterIdler( IIdleable* inIdler);

	// Memory allocator support
			void						SetCurrentAllocator( bool inIsAlternate)			{ fCurrentAllocator = VObject::GetAllocator(inIsAlternate); }
			VCppMemMgr*					GetCurrentAllocator() const							{ return fCurrentAllocator; }
			bool						IsAlternateCurrentAllocator() const					{ return fCurrentAllocator == VObject::GetAllocator(true); }
	static	bool						CurrentTaskUsesAlternateAllocator()					{ VTask *task = GetCurrent(); return (task != NULL) ? task->IsAlternateCurrentAllocator() : false;}

			void						SetCurrentDeallocator( bool inIsAlternate)			{ fCurrentDeallocator = VObject::GetAllocator(inIsAlternate); }
			VCppMemMgr*					GetCurrentDeallocator() const						{ return fCurrentDeallocator; }
			bool						IsAlternateCurrentDeallocator() const				{ return fCurrentDeallocator == VObject::GetAllocator(true); }

	// Debugging support
			VDebugContext&				GetDebugContext()									{ return fDebugContext; }


			XTaskImpl*					GetImpl() const										{ return fImpl;}

	//============= Private per-task data =============

	// Create a data key usable for any VTask.
	// inDisposeProc, if not NULL, will be called when a task terminates, to let you dispose your data.
	// The proc is called in the task context.
	static	VTaskDataKey				CreateDataKey( VTaskDataKeyDisposeProc inDisposeProc)	{return VTaskMgr::Get()->CreateDataKey( inDisposeProc);}

	// Dispose a data key.
	// It is your responsibility to ensure you disposed any per-task data associated with this key. (inDisposeProc won't be called)
	static	void						DeleteDataKey( VTaskDataKey inKey)						{return VTaskMgr::Get()->DeleteDataKey( inKey);}

	// Get the task data associated with a previously created data key.
	// Returns NULL if no data was previously set for this key.
	// only available for current task.
	static	void*						GetCurrentData( VTaskDataKey inKey);

	// Set the task data associated with a previously created data key.
	// only available for current task.
	static	void						SetCurrentData( VTaskDataKey inKey, void *inData);

	//============= Global accessors =============

	// Main task (the one created by the system when the application launches)
	static	VTask*						GetMain()											{ return VTaskMgr::Get()->GetMainTask(); }
	static	bool						IsCurrentMain();

	// The currently running task
	static	VTask*						GetCurrent()										{ return VTaskMgr::Get()->GetCurrentTask(); }
	static	VTaskID						GetCurrentID()										{ return VTaskMgr::Get()->GetCurrentTaskID(); }
	static	size_t						GetCurrentFreeStackSize()							{ return VTaskMgr::Get()->GetCurrentFreeStackSize(); }

	// Cooperative model support
	static 	void						YieldNow()											{ VTaskMgr::Get()->YieldNow(); }

	static 	void						Yield()												{ VTaskMgr::Get()->Yield(); }
	static 	void						YieldToFiber( VTask *inPreferredTask)				{ VTaskMgr::Get()->YieldToFiber( inPreferredTask); }
	static	void						SetSystemEventPolling( bool inSet)					{ VTaskMgr::Get()->SetSystemEventPolling( inSet); }
	static	void						PollAndExecuteUpdateEvent()							{ VTaskMgr::Get()->PollAndExecuteUpdateEvent(); }

	// Internationalization
	static	VIntlMgr*					GetCurrentIntlManager();
	static	void						SetCurrentIntlManager( VIntlMgr *inIntlManager);
	static	void						SetCurrentDialectCode( DialectCode inDialect, CollatorOptions inCollatorOptions);
			DialectCode					GetDialectCode() const;
	static	UniChar						GetWildChar();

	// Private utilities. XTaskImpl should be declared friend but one can't because it's a typedef.
			void						_Run();	// This is the task's message loop
			void						_SetStateDead();

	static VTaskDataKey GetHTTPServerSessionDataKey();


protected:
	friend class VTaskMgr;
	friend class VMessage;

	// Constructor / destructor( not to be called directly)
										VTask( VTaskMgr* inTaskMgr);
	virtual								~VTask();

	// Override to implement task behaviour
	// Default run behaviour call TaskRunProcPtr if provided
	virtual void						DoInit();
	virtual void						DoDeInit();
	virtual Boolean						DoRun();
	virtual void						DoIdle();
	virtual void						DoPrepareToDie();

			// Idle support
			void						CallIdlers();

			// Messaging support
			bool						AddMessage( VMessage* inMessage);
			void						CancelMessages( IMessageable* inTarget);

			// Task chaining support
			VTask*						GetNextTask()										{ return fNext; }
			void						SetNextTask( VTask* inTask)							{ fNext = inTask; }

			void						DisposeAllData();

	// if this task calls Yield(), then the task manager Yield callbacks are called.
	// reserved for fibers and the main task if the process is fibered.
			bool						IsForYieldCallbacks() const							{ return fIsForYieldCallbacks; }

			XTaskImpl*					fImpl;

private:
			typedef std::vector<VRefPtr<VSyncEvent> > VectorOfVSyncEvent;

			bool						_CheckIdleEvents();
			VMessage*					_RetainMessageWithTimeout( sLONG inTimeoutMilliseconds);
			bool						_ExecuteMessages( VMessage *inFirstMessage, bool inProcessIdlers);
			void						_Exit();
			bool						_AllocateMessageQueueIfNeeded();

			VTaskID						fTaskId;
			ETaskStyle					fStyle;
			VTaskMgr*					fManager;
			sLONG						fState;	// Actually a TaskState. Use as sLONG to be able to call CompareAndSwap
			uLONG						fDyingSince;	// time of first call to Kill()
			VTask*						fNext;
			uLONG						fLastIdle;
			uLONG						fIdleTicks;
			VPtr						fStackStartAddr;
			VCppMemMgr*					fCurrentAllocator;
			VCppMemMgr*					fCurrentDeallocator;
			OsType						fKind;
			sLONG_PTR					fKindData;
			TaskRunProcPtr				fRunProcPtr;
			VString*					fName;
			const VValueBag*			fProperties;
			VMessageQueue*				fMessageQueue;
	mutable	size_t 						fStackSize;
	mutable	VErrorTaskContext*			fErrorContext;
			bool						fIsForYieldCallbacks;
			bool						fCanBlockOnSyncObject;
			VDebugContext				fDebugContext;
			std::vector<IIdleable*>		fIdlers;
			bool						fIsIdlersValid;
			VTaskDataVector				fData;
			VIntlMgr*					fIntlManager;
			VectorOfVSyncEvent*			fDeathEvents;
			sLONG						fFiberSleepFlag;
	static	VTaskDataKey				sHTTPServerSessionDataKey;
};


class XTOOLBOX_API VTaskRef : public VObject
{
friend class VTaskMgr;
public:
										VTaskRef( const VTaskID inTaskID)				{ fTask = VTaskMgr::Get()->RetainTaskByID( inTaskID); }
	virtual								~VTaskRef()										{ IRefCountable::Copy(&fTask, (VTask*) NULL); }

			// State management
			void						WakeUp()										{ if (fTask != NULL) fTask->WakeUp(); }
			void						Freeze()										{ if (fTask != NULL) fTask->Freeze(); }
			void						Sleep( sLONG inMilliSeconds)					{ if (fTask != NULL) fTask->Sleep(inMilliSeconds); }
			void						Kill()											{ if (fTask != NULL) fTask->Kill(); }

			// Accessors
			bool						IsMain() const									{ return (fTask != NULL ? fTask->IsMain() : false); }
			bool						IsCurrent() const								{ return (fTask != NULL ? fTask->IsCurrent() : false); }

			size_t						GetStackSize() const							{ return (fTask != NULL ? fTask->GetStackSize() : 0); }

			VTaskID						GetID() const									{ return (fTask != NULL ? fTask->GetID() : NULL_TASK_ID); }
			TaskState					GetState() const								{ return (fTask != NULL ? fTask->GetState() : TS_DEAD); }

			// Operators (CAUTION: returned value may be NULL)
			VTask*						operator->() const								{ return (VTask*) fTask; }
										operator VTask*() const							{ return (VTask*) fTask; }

private:
			VTask*						fTask;

			// Private contructor
										VTaskRef( VTask* inTask)						{ fTask = inTask; }
};


class StAllocateInMainMem
{
public:
	StAllocateInMainMem()
	{
		VTask *task = VTask::GetCurrent();
		fWasAlternate = task->IsAlternateCurrentAllocator();
		task->SetCurrentAllocator( false);
	}

	~StAllocateInMainMem()
	{
		VTask *task = VTask::GetCurrent();
		task->SetCurrentAllocator( fWasAlternate);
	}

private:
	bool	fWasAlternate;
};


inline VError vGetLastError ()
{
	VTask*	task = VTask::GetCurrent();
	return (task == NULL) ? VE_OK : task->GetLastError();
}


inline void vFlushErrors ()
{
	VTask*	task = VTask::GetCurrent();
	if (task != NULL)
		task->FlushErrors();
}

END_TOOLBOX_NAMESPACE

#endif

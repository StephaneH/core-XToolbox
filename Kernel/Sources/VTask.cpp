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
#include "VArray.h"
#include "VErrorContext.h"
#include "VSyncObject.h"
#include "VInterlocked.h"
#include "VSystem.h"
#include "VString.h"
#include "IIdleable.h"
#include "VIntlMgr.h"
#include "VTask.h"
#include "VProcess.h"
#include "VValueBag.h"

BEGIN_TOOLBOX_NAMESPACE

typedef	StLocker<XTaskMgrMutexImpl>	StTaskMgrLocker;


class VScheduler_void : public VObject, public IFiberScheduler
{
public:
	virtual	void	Yield()																				{;}
	virtual	void	YieldNow()																			{;}
	virtual	void	WaitFlag( sLONG */*inValuePtr*/, sLONG /*inValue*/)									{;}
	virtual	void	WaitFlagWithTimeout( sLONG */*inValuePtr*/, sLONG /*inValue*/, sLONG /*inTimeoutMilliseconds*/)	{;}
	virtual	void	SetSystemEventPolling( bool /*inSet*/)												{;}
	virtual	void	PollAndExecuteUpdateEvent()															{;}
	virtual	void	SetDirectUpdate( bool /*inIsOn*/)													{;}

#if VERSIONMAC
	virtual void	CheckWindowUpdate(WindowRef /*inWindow*/)											{;}
	virtual void	SetSystemCallCFRunloop(bool /*inValue*/)											{;}
	virtual bool	ExecuteInProcess0()																	{return false;}
#endif

	virtual	void	SetDirectUpdateAllowedEvenIfWaitingForFlag(bool /*inAllowed*/)						{;}
};


END_TOOLBOX_NAMESPACE


bool VTaskMgr::sInstanceInited = false;

VTask::VTask( VObject * /*inCreator*/, size_t inStackSize, ETaskStyle inStyle, TaskRunProcPtr inRunProcPtr)
	: fStyle( inStyle)
	, fManager( VTaskMgr::Get())
	, fState( TS_CREATING)
	, fDyingSince( 0)
	, fNext( NULL)
	, fLastIdle( 0)
	, fIdleTicks( 1/*VSystem::GetCaretTicks()*/)
	, fStackStartAddr( NULL)
	, fCurrentAllocator( VObject::sCppMemMgr)
	, fCurrentDeallocator( NULL)
	, fKind( 0)
	, fKindData( 0)
	, fRunProcPtr( inRunProcPtr)
	, fName( NULL)
	, fProperties( NULL)
	, fMessageQueue( NULL)
	, fStackSize( inStackSize)
	, fErrorContext( NULL)
	, fIsForYieldCallbacks( inStyle == eTaskStyleFiber)
	, fCanBlockOnSyncObject( inStyle == eTaskStylePreemptive)
	, fIsIdlersValid(false)
	, fIntlManager(NULL)
	, fDeathEvents( NULL)
{
	xbox_assert(GetMain() != NULL);

	fIntlManager = VProcess::Get()->GetIntlManager()->Clone();

	fImpl = fManager->fImpl.Create( this, inStyle);	// may throw a VError and return NULL
	fTaskId = fManager->GetNewTaskID( this);

	SetMessagingTask( fTaskId);
}


VTask::VTask( VTaskMgr *inTaskMgr)
	: fStyle( eTaskStylePreemptive)
	, fManager( inTaskMgr)
	, fState( TS_RUNNING)
	, fDyingSince( 0)
	, fNext( NULL)
	, fLastIdle( 0)
	, fIdleTicks( 1/*VSystem::GetCaretTicks()*/)
	, fCurrentAllocator( VObject::sCppMemMgr)
	, fCurrentDeallocator( NULL)
	, fKind( 0)
	, fKindData( 0)
	, fRunProcPtr( NULL)
	, fName( NULL)
	, fProperties( NULL)
	, fMessageQueue( NULL)
	, fStackSize( inTaskMgr->GetStackSizeForMainTask())
	, fErrorContext( NULL)
	, fIsForYieldCallbacks( VProcess::Get()->IsFibered())
	, fCanBlockOnSyncObject( !VProcess::Get()->IsFibered())
	, fIsIdlersValid(false)
	, fDeathEvents( NULL)
{
	// This is the main task, there should be only one of these
	xbox_assert(inTaskMgr != NULL);
	xbox_assert(inTaskMgr->GetMainTask() == NULL && inTaskMgr->GetTaskCount() == 0);

	// We need to compute the stack address because _Run is not called for the main task
	char firstStackBasedVar;
	fStackStartAddr = &firstStackBasedVar;

	fIntlManager = VProcess::Get()->GetIntlManager()->Clone();

	fImpl = fManager->fImpl.CreateMain( this);

	fTaskId = fManager->GetNewTaskID( this);

	SetMessagingTask( fTaskId);
}


VTask::~VTask()
{
	xbox_assert( fData.size() == 0);	// the user has used SetData after the task terminated. Leakage because one can't call disposedata.
	
	xbox_assert(GetState() == TS_DEAD || GetState() == TS_CREATING);
	
	ReleaseRefCountable( &fIntlManager);
	ReleaseRefCountable( &fProperties);
	
	if (fMessageQueue)
	{
		fMessageQueue->CancelMessages();
		delete fMessageQueue;
	}
	
	if (fErrorContext != NULL)
		fErrorContext->Release();

	delete fDeathEvents;
	delete fImpl;
	delete fName;
}


/*
	static
*/
VIntlMgr* VTask::GetCurrentIntlManager()
{
	VIntlMgr *manager;
	const VTask* task = VTask::GetCurrent();
	if (task != NULL)
	{
		manager = task->fIntlManager;
	}
	else
	{
		VProcess* process = VProcess::Get();
		if (process)
		{
			manager = process->GetIntlManager();
		}
		else
		{
			// can happen for global variables
			manager = NULL;
		}
	}
	return manager;
}


/*
	static
*/
UniChar VTask::GetWildChar()
{
	VIntlMgr *manager = VTask::GetCurrentIntlManager();
	if (manager == NULL)
		return '@';
	else
		return manager->GetWildChar();
}

/*
	static
*/
void VTask::SetCurrentIntlManager( VIntlMgr *inIntlManager)
{
	VTask* task = VTask::GetCurrent();
	if ( (task != NULL) && testAssert( inIntlManager != NULL) )
	{
		CopyRefCountable( &task->fIntlManager, inIntlManager);
	}
}


/*
	static
*/
void VTask::SetCurrentDialectCode( DialectCode inDialect, CollatorOptions inCollatorOptions)
{
	VIntlMgr* manager = VIntlMgr::Create( inDialect, inCollatorOptions);
	SetCurrentIntlManager( manager);
	ReleaseRefCountable( &manager);
}


DialectCode VTask::GetDialectCode() const
{
	return fIntlManager->GetDialectCode();
}


bool VTask::_CheckIdleEvents()
{
	bool	isGotOne = false;

	if (ABSTICKS(fLastIdle) > fIdleTicks)
	{
		fLastIdle = VSystem::GetCurrentTicks();
		
		DoIdle();
		
		CallIdlers();
		
		fManager->_CheckDelayedMessages();
		isGotOne = true;
	}
	return isGotOne;
}


void VTask::CancelMessages( IMessageable* inTarget)
{
	if (fMessageQueue)
		fMessageQueue->CancelMessages(inTarget);
}

/*
bool VTask::RemoveMessage( VMessage* inMessage)
{
	return fMessageQueue ? fMessageQueue->RemoveMessage(inMessage) : false;
}
*/

TaskState VTask::GetState() const
{
	return (TaskState) fState;
}


SystemTaskID VTask::GetSystemID() const
{
	return (fImpl != NULL) ? fImpl->GetSystemID() : 0;
}

bool VTask::GetCPUUsage(Real& outCPUUsage) const
{
	return (fImpl != NULL) ? fImpl->GetCPUUsage(outCPUUsage) : false;
}


bool VTask::GetCPUTimes(Real& outSystemTime, Real& outUserTime) const
{
	return (fImpl != NULL) ? fImpl->GetCPUTimes(outSystemTime, outUserTime) : false;
}


void VTask::Sleep( sLONG inMilliSeconds)
{
	xbox_assert(inMilliSeconds >= 0);

	VTask *task = GetCurrent();
	if (task != NULL && inMilliSeconds > 0)
	{
		if (task->CanBlockOnSyncObject())
		{
			task->fImpl->Sleep( inMilliSeconds);
		}
		else
		{
			task->fFiberSleepFlag = 0;
			VTaskMgr::Get()->CurrentFiberWaitFlagWithTimeout( &task->fFiberSleepFlag, 0, inMilliSeconds);
		}
	}
}


void VTask::Freeze()
{
	VTask *task = GetCurrent();
	if (task != NULL)
	{
		if (task->CanBlockOnSyncObject())
		{
			task->fImpl->Freeze();
		}
		else
		{
			task->fFiberSleepFlag = 0;
			VTaskMgr::Get()->CurrentFiberWaitFlag( &task->fFiberSleepFlag, 0);
		}
	}
}


void VTask::WakeUp()
{
	if (fImpl != NULL)
	{
		if (CanBlockOnSyncObject())
		{
			fImpl->WakeUp();
		}
		else
		{
			fFiberSleepFlag = 1;
		}
	}
}


bool VTask::Run()
{
	bool ok = false;
	
	// ask the task manager to keep an eye on this task
	xbox_assert(GetState() == TS_CREATING);
	
	// a fiber task must be run from the main task
	VTask *task = GetCurrent();
	if ( (fImpl != NULL) && testAssert( (task != NULL) && ((fStyle != eTaskStyleFiber) || task->IsMain()) ))
	{
		if (VInterlocked::CompareExchange( &fState, TS_CREATING, TS_RUN_PENDING) == TS_CREATING)
		{
			fManager->RegisterTask(this);
			ok = fImpl->Run();
			if (!ok)
			{
				// failed to launch
				xbox_assert( fState == TS_RUN_PENDING);
				// set expected state for _SetStateDead
				VInterlocked::CompareExchange( &fState, TS_RUN_PENDING, TS_DYING);
				_SetStateDead();
			}
		}
	}
	return ok;
}


void VTask::_Exit()
{
	StopMessaging();
	if (fMessageQueue != NULL)
		fMessageQueue->CancelMessages();

	DisposeAllData();
	fManager->_TaskStopped( this);
}


void VTask::_SetStateDead()
{
	VectorOfVSyncEvent deathEvents;

	fManager->GetMutex()->Lock();
	if (fDeathEvents != NULL)
		deathEvents.swap( *fDeathEvents);
	VInterlocked::CompareExchange( &fState, TS_DYING, TS_DEAD);
	fManager->GetMutex()->Unlock();

	// now the task manager can release us at any time
	
	for( VectorOfVSyncEvent::iterator i = deathEvents.begin() ; i != deathEvents.end() ; ++i)
		(*i)->Unlock();
}


void VTask::Exit( VTask *inDestinationTask)
{
	// exit current thread
	VTask *task = GetCurrent();
	if(task == NULL)
		return;

	// uniquement si on est dans _Run().
	// mais en principe si on est "current" c'est que le test est verifie.
	if (task->fState >= TS_RUN_PENDING && task->fState < TS_DEAD)
	{
		VInterlocked::Exchange( &task->fState, TS_DYING);
		task->_Exit();
		task->fImpl->Exit( inDestinationTask);
	}
}


void VTask::Kill()
{
	xbox_assert(GetState() >= TS_RUN_PENDING);
	
	if (!IsCurrent())
	{
		// if the task is just being started, we have to wait for it.
		// this is a spinlock but should be rare and fast.
		while(GetState() == TS_RUN_PENDING)
			VTask::YieldNow();
	}
	DoPrepareToDie();
	if (VInterlocked::CompareExchange( &fState, TS_RUNNING, TS_DYING) == TS_RUNNING)
		fDyingSince = VSystem::GetCurrentTicks();
}


bool VTask::IsDying( sLONG *outDyingSinceMilliseconds) const
{
	bool isDying = GetState() >= TS_DYING;
	if (outDyingSinceMilliseconds != NULL)
		*outDyingSinceMilliseconds = isDying ? ABSTICKS( fDyingSince ) : 0;
	return isDying;
}


bool VTask::WaitForDeath( sLONG inTimeoutMilliseconds)
{
	// can't wait for himself!
	if (!testAssert( !IsCurrent()))
		return false;
	
	uLONG t0 = VSystem::GetCurrentTime();

	// give the soon dead task the current task message queue event.
	VRefPtr<VSyncEvent> deathEvent;
	
	fManager->GetMutex()->Lock();
	bool pushedDeathEvent = (GetState() != TS_DEAD);
	if (pushedDeathEvent)
	{
		if (fDeathEvents == NULL)
			fDeathEvents = new VectorOfVSyncEvent;
		if (fDeathEvents != NULL)
		{
			deathEvent.Adopt( VTask::GetCurrent()->RetainMessageQueueSyncEvent());
			fDeathEvents->push_back( deathEvent);
		}
	}
	fManager->GetMutex()->Unlock();

	while(GetState() != TS_DEAD)
	{
		// wait for messages or the death status
		uLONG t1 = VSystem::GetCurrentTime();
		sLONG delta = (t1 >= t0) ? (t1 - t0) : (t0 - t1);
		if (delta >= inTimeoutMilliseconds)
			break;	// timeout expired

		ExecuteMessagesWithTimeout( inTimeoutMilliseconds - delta);
	}

	// remove our event from the zombie task
	if (pushedDeathEvent)
	{
		fManager->GetMutex()->Lock();
		if (fDeathEvents != NULL)
		{
			VectorOfVSyncEvent::iterator i = std::find( fDeathEvents->begin(), fDeathEvents->end(), deathEvent);
			if (i != fDeathEvents->end())
				fDeathEvents->erase( i);
		}
		fManager->GetMutex()->Unlock();
	}
	
	return (GetState() == TS_DEAD);
}


Boolean VTask::DoRun()
{
	// to be overriden
	// returns true to be called again and again until the task is aborted
	return true;
}


void VTask::DoIdle()
{
	// to be overriden
}


void VTask::DoInit()
{
	// to be overriden
}


void VTask::DoDeInit()
{
	// to be overriden
}


void VTask::DoPrepareToDie()
{
	// to be overriden
}


	
void VTask::SetName( const VString& inName)
{
	fManager->GetMutex()->Lock();
	if (fName == NULL)
		fName = inName.Clone();
	else
		fName->FromString( inName);
	fManager->GetMutex()->Unlock();

	if (IsCurrent())
		fManager->GetImpl()->SetCurrentThreadName( inName, GetID());
}


void VTask::GetName( VString& outName) const
{
	fManager->GetMutex()->Lock();
	if (fName == NULL)
		outName.Clear();
	else
		outName.FromString( *fName);
	fManager->GetMutex()->Unlock();
}


const VValueBag* VTask::RetainProperties() const
{
	fManager->GetMutex()->Lock();
	const VValueBag* properties = fProperties;
	if (properties != NULL)
		properties->Retain();
	fManager->GetMutex()->Unlock();
	
	return properties;
}


void VTask::SetProperties( const VValueBag *inProperties)
{
	fManager->GetMutex()->Lock();
	CopyRefCountable( &fProperties, inProperties);
	fManager->GetMutex()->Unlock();
}


bool VTask::_AllocateMessageQueueIfNeeded()
{
	if (fMessageQueue == NULL)
	{
		VMessageQueue *queue = new VMessageQueue;
		if (VInterlocked::CompareExchangePtr( (void**)&fMessageQueue, NULL, queue) != NULL)
			delete queue;
	}
	return fMessageQueue != NULL;
}


bool VTask::AddMessage( VMessage* inMessage)
{
	// if the target has too many messages waiting, put the caller into sleep mode for a while
#if 0
	if (fMessageQueue->CountMessages() > 10)
	{
		VTask* currentTask = VTask::GetCurrent();
		if (currentTask != this)
		{
			uLONG t0 = VSystem::GetCurrentTicks();
			do {
				#if VERSIONDEBUG
				if (abs((long) (VSystem::GetCurrentTicks() - t0)) > 600)
				{
					VDebugMgr::StartLowLevelMode();
					VDebugMgr::AssertFailed("Messaging overflow !", __FILE__, __LINE__);
					VDebugMgr::EndLowLevelMode();
				}
				#endif
				currentTask->Sleep(100L);
				
			} while(fMessageQueue->CountMessages() > 10);
		}
	}

#endif
	return _AllocateMessageQueueIfNeeded() ? fMessageQueue->AddMessage(inMessage) : false;
}


size_t VTask::GetStackSize() const
{
	if (fStackSize == 0)
	{
		if (IsMain())
			fStackSize = fManager->GetStackSizeForMainTask();
		else
			fStackSize = fManager->GetDefaultStackSize();
	}
	xbox_assert(fStackSize > 0);
	return fStackSize;
}


VMessage *VTask::_RetainMessageWithTimeout( sLONG inTimeoutMilliseconds)
{
	// while waiting for any message, we allow the scheduler to send direct updates messages using old mechanism.
	// that's temp fix waiting to generalize to using VMessage for direct updates.
	IFiberScheduler *scheduler = VTaskMgr::Get()->RetainFiberScheduler();
	scheduler->SetDirectUpdateAllowedEvenIfWaitingForFlag( true);
	
	VMessage *message = _AllocateMessageQueueIfNeeded() ? fMessageQueue->RetainMessageWithTimeout( inTimeoutMilliseconds) : NULL;

	scheduler->SetDirectUpdateAllowedEvenIfWaitingForFlag( false);
	ReleaseRefCountable( &scheduler);
	
	return message;
}


bool VTask::_ExecuteMessages( VMessage *inFirstMessage, bool inProcessIdlers)
{
	bool isGotOne = false;

	for( VMessage *msg = inFirstMessage ; msg != NULL ; msg = fMessageQueue->RetainMessage())
	{
		StErrorContextInstaller	errorContext( false, false); // sc 03/05/2013, use non silent error context
		isGotOne = true;
		msg->Execute();
		msg->Release();
	}

	if ( !isGotOne && (!fManager->GetNeedsCheckSystemMessages() || !GetImpl()->CheckSystemMessages()) )
	{
		if (inProcessIdlers)
			_CheckIdleEvents();
	}
	
	return isGotOne;
}


void VTask::_Run()
{
	char c;
	fStackStartAddr = &c;
	
	if (VInterlocked::CompareExchange( &fState, TS_RUN_PENDING, TS_RUNNING) == TS_RUN_PENDING)
	{
		fManager->_TaskStarted( this);
		if (fRunProcPtr != NULL)
		{
			sLONG exitCode = fRunProcPtr( this);
			VInterlocked::CompareExchange( &fState, TS_RUNNING, TS_DYING);
		}
		else
		{
			DoInit();
			do {
				fManager->Yield();
				Boolean isLoop = DoRun();
				if (!isLoop)
				{
					Kill();
					break;
				}
				ExecuteMessages();
			} while(GetState() < TS_DYING);
			xbox_assert(fState == TS_DYING);
			DoDeInit();
		}
		_Exit();
	}
	else
	{
		// hum, should never happen.
		xbox_assert(false);
	}
}


bool VTask::IsAnyMessagePending() const
{
	return (fMessageQueue != NULL) && !fMessageQueue->IsEmpty();
}


bool VTask::ExecuteMessages( bool inProcessIdlers)
{
	VTask *task = GetCurrent();
	if (task == NULL)
		return false;
	
	VMessage *firstMessage = (task->fMessageQueue != NULL) ? task->fMessageQueue->RetainMessage() : NULL;
	
	return task->_ExecuteMessages( firstMessage, inProcessIdlers);
}


bool VTask::ExecuteMessagesWithTimeout( sLONG inTimeoutMilliseconds, bool inProcessIdlers)
{
	VTask *task = GetCurrent();
	if (task == NULL)
		return false;

	VMessage *firstMessage = task->_RetainMessageWithTimeout( inTimeoutMilliseconds);

	return task->_ExecuteMessages( firstMessage, inProcessIdlers);
}


VMessage *VTask::RetainMessageWithTimeout( sLONG inTimeoutMilliseconds)
{
	VTask *task = GetCurrent();
	if (task == NULL)
		return NULL;
	
	return task->_RetainMessageWithTimeout( inTimeoutMilliseconds);
}


void VTask::SetMessageQueueEvent()
{
	if (fMessageQueue != NULL)
		fMessageQueue->SetEvent();
}


VSyncEvent *VTask::RetainMessageQueueSyncEvent()
{
	VSyncEvent *syncEvent = _AllocateMessageQueueIfNeeded() ? fMessageQueue->GetSyncEvent() : NULL;
	if (syncEvent != NULL)
		syncEvent->Retain();
	
	return syncEvent;
}


VError VTask::GetLastError() const
{
	return (fErrorContext == NULL) ? VE_OK : fErrorContext->GetLastError();
}


bool VTask::FailedForMemory() const
{
	return (fErrorContext == NULL) ? false : fErrorContext->FailedForMemory();
}


VErrorTaskContext* VTask::GetErrorContext( bool inMayCreateIt)
{
	VTask *task = GetCurrent();
	if(task == NULL)
		return NULL;
	
	if (task->fErrorContext == NULL && inMayCreateIt)
		task->fErrorContext = new VErrorTaskContext;
	
	return task->fErrorContext;
}


bool VTask::PushError(VErrorBase* inError)
{
	VTask *task = GetCurrent();
	if ( (task == NULL) || (inError == NULL) )
		return false;

	StAllocateInMainMem mainAllocator;

	if (task->fErrorContext == NULL)
		task->fErrorContext = new VErrorTaskContext;

	if (task->fErrorContext == NULL)
		return false;

	bool silent;
	bool pushed = task->fErrorContext->PushError( inError, &silent);

	if (!silent)
		VDebugMgr::Get()->ErrorThrown( inError);
	
	return pushed;
}


bool VTask::PushRetainedError( VErrorBase* inError)
{
	bool isPushed = PushError(inError);

	if (inError != NULL)
		inError->Release();

	return isPushed;
}


void VTask::PushErrors( const VErrorContext* inErrorContext)
{
	VTask *task = GetCurrent();
	if ( (task == NULL) || (inErrorContext == NULL) || inErrorContext->IsEmpty() )
		return;

	StAllocateInMainMem mainAllocator;
	
	if (task->fErrorContext == NULL)
		task->fErrorContext = new VErrorTaskContext;
	
	task->fErrorContext->PushErrorsFromContext( inErrorContext);
}


void VTask::FlushErrors()
{
	VTask *task = GetCurrent();
	if (task != NULL && task->fErrorContext != NULL)
		task->fErrorContext->Flush();
}


sLONG VTask::CountMessagesFor( IMessageable* inTarget) const
{
	return fMessageQueue ? fMessageQueue->CountMessagesFor( inTarget) : 0;
}


void VTask::RegisterIdler( IIdleable* inIdler)
{
	if (inIdler == NULL)
		return;

	StTaskMgrLocker lock( fManager->GetMutex());

	if (std::find( fIdlers.begin(), fIdlers.end(), inIdler) == fIdlers.end())
	{
		fIdlers.push_back( inIdler);
		fIsIdlersValid = false;
	}
}


void VTask::UnregisterIdler( IIdleable* inIdler)
{
	if (testAssert(IsCurrent()))	// sc 30/01/2008
	{
		StTaskMgrLocker lock( fManager->GetMutex());

		std::vector<IIdleable*>::iterator i = std::find( fIdlers.begin(), fIdlers.end(), inIdler);
		
		if (i != fIdlers.end())
		{
			fIdlers.erase( i);
			fIsIdlersValid = false;
		}
	}
}


void VTask::CallIdlers()
{
	if (testAssert(IsCurrent()))
	{
		// copy to protect against RegisterIdler/UnregisterIdler during the loop.
		// an idler MUST be deleted from its task, so it's safe to call it outside the lock

		fManager->GetMutex()->Lock();
		std::vector<IIdleable*> idlers( fIdlers);
		fIsIdlersValid = true;
		fManager->GetMutex()->Unlock();
		
		for( std::vector<IIdleable*>::iterator i = idlers.begin() ; i != idlers.end() ; ++i)
		{
			if(fIsIdlersValid)
			{
				XBOX_ASSERT_VOBJECT(dynamic_cast<VObject*>( *i));
				(*i)->Idle();
			}
			else
				break;
		}
	}
}


void *VTask::GetCurrentData( VTaskDataKey inKey)
{
	void *data;

	// check only in debug mode for performance reasons
	xbox_assert( VTaskMgr::Get()->CheckDataKey( inKey));

	const VTask* task = VTask::GetCurrent();
	if (task != NULL)
	{
		if (inKey > 0 && inKey <= task->fData.size())
			data = task->fData[inKey - 1];
		else
			data = NULL;
	}
	else
	{
		data = NULL;
	}
	
	return data;
}


void VTask::SetCurrentData( VTaskDataKey inKey, void *inData)
{
	// check only in debug mode for performance reasons
	xbox_assert( VTaskMgr::Get()->CheckDataKey( inKey));
	
	VTask* task = VTask::GetCurrent();
	if (task != NULL)
	{
		if (inKey > task->fData.size())
		{
			task->fData.resize( inKey, NULL);
		}
		task->fData[inKey - 1] = inData;
	}
}


void VTask::DisposeAllData()
{
	fManager->DisposeData( fData.begin(), fData.end());
	fData.clear();
}


/*
	static
*/
bool VTask::IsCurrentMain()
{
	return VTaskMgr::Get()->GetMainTask()->IsCurrent();
}

//======================================================================================================

#pragma mark-


VTaskMgr VTaskMgr::sInstance;



VTaskMgr::VTaskMgr()
	: fMainTask( NULL)
	, fStackSizeForMainTask( 0)
	, fDefaultStackSize( 0)
	, fCriticalSection( NULL)
	, fDelayedMessages( NULL)
	, fDelayedMessagesTimes( NULL)
	, fSeedFiberTaskID( 0)
	, fFiberScheduler( NULL)
	, fSeedForeignTaskID( 0)
	, fNeedsCheckSystemMessages( false)
	, fTaskListStamp(0)
{
#if VERSIONMAC
	fTimer = NULL;
#endif
}


bool VTaskMgr::Init( VProcess *inProcess)
{
	sInstanceInited = sInstance._Init( inProcess);

	return sInstanceInited;
}


void VTaskMgr::DeInit()
{
	sInstance._DeInit();
	
	sInstanceInited = false;
}


bool VTaskMgr::_Init( VProcess *inProcess)
{
	// Not thread safe
	xbox_assert(inProcess != NULL && fMainTask == NULL);

	fFiberScheduler = new VScheduler_void;
	fDelayedMessages = new std::vector<VMessage*>;
	fDelayedMessagesTimes = new std::vector<sLONG>;
	
	bool isOK = fImpl.Init( inProcess);
	if (isOK)
	{
		fCriticalSection = new XTaskMgrMutexImpl;
		fMainTask = new VTask( this); // la Main Task aura 1 comme ID
		_TaskStarted( fMainTask);
	}

	return isOK;
}


void VTaskMgr::_DeInit()
{
	// Not thread safe
	xbox_assert(fMainTask == NULL || fMainTask->GetNextTask() == NULL);

	if (fMainTask != NULL)
	{
		// dispose main task local data
		fMainTask->DisposeAllData();
		
		fMainTask->fState = TS_DEAD;
		xbox_assert(fMainTask->GetRefCount() == 1);
		fMainTask->Release();
	}
	
	delete fCriticalSection;
	fCriticalSection = NULL;

	delete fDelayedMessages;
	fDelayedMessages = NULL;

	delete fDelayedMessagesTimes;
	fDelayedMessagesTimes = NULL;

	ReleaseRefCountable( &fFiberScheduler);

#if VERSIONMAC
	xbox_assert( fTimer == NULL);
#endif

	fImpl.Deinit();
}

IFiberScheduler *VTaskMgr::RetainFiberScheduler() const
{
	StTaskMgrLocker lock( fCriticalSection);
	IFiberScheduler *scheduler = fFiberScheduler;
	scheduler->Retain();
	return scheduler;
}


void VTaskMgr::SetFiberScheduler( IFiberScheduler *inScheduler)
{
	if (testAssert( inScheduler != NULL))
	{
		StTaskMgrLocker lock( fCriticalSection);
		CopyRefCountable( &fFiberScheduler, inScheduler);
	}
}


void VTaskMgr::FiberStoppedBeingScheduled( VTask *inFiber)
{
	if (inFiber != NULL)
	{
		if (testAssert( inFiber->GetTaskStyle() == eTaskStyleFiber && !inFiber->IsCurrent() ))
		{
			xbox_assert( inFiber->GetState() >= TS_DYING);
			if (inFiber->GetState() != TS_DEAD)
			{
				/*
					that's a very incomplete way to cancel a fiber.
					On mac, a fiber is a actually pthread and we should call pthread_cancel on it.
					So this minimal support is only intended to force quit 4D.
				*/
				inFiber->_SetStateDead();
			}
		}
		PurgeDeadTasks();
	}
}


void VTaskMgr::RegisterTask( VTask *inTask)
{
	// called from within VTask::Run
	xbox_assert(inTask->GetState() == TS_RUN_PENDING);
	
	PurgeDeadTasks();

	{
		StTaskMgrLocker lock( fCriticalSection);

		// retained in our linked list
		inTask->Retain();

		// link the new task above main task
		inTask->SetNextTask( fMainTask->GetNextTask());
		fMainTask->SetNextTask( inTask);

		fTaskListStamp++;
	}
}


sLONG VTaskMgr::CountMessagesFor( IMessageable* inTarget)
{
	StTaskMgrLocker lock( fCriticalSection);

	sLONG count = 0;
	
	// count delayed messages
	for( std::vector<VMessage*>::iterator i = fDelayedMessages->begin() ; i != fDelayedMessages->end() ; ++i)
	{
		if ((*i)->GetTarget() == inTarget)
			++count;
	}
	
	// count pending messages
	for( VTask* oneTask = fMainTask ; oneTask != NULL ; oneTask = oneTask->GetNextTask())
	{
		count += oneTask->CountMessagesFor( inTarget);
	}

	return count;
}


sLONG VTaskMgr::GetTaskCount(bool inNotDying) const
{
	StTaskMgrLocker lock( fCriticalSection);

	sLONG count = 0;
	
	if(inNotDying)
	{
		for( VTask *oneTask = fMainTask ; oneTask != NULL ; oneTask = oneTask->GetNextTask())
		{
			if (oneTask->GetState() < TS_DYING)
				++count;
		}
	}
	else
	{
		for( VTask *oneTask = fMainTask ; oneTask != NULL ; oneTask = oneTask->GetNextTask())
		{
			if (oneTask->GetState() == TS_RUNNING || oneTask->GetState() == TS_DYING)
				++count;
		}
	}
	
	return count;
}


void VTaskMgr::_TaskStarted( VTask *inTask)
{
	// called at the very beginning of a task run
	xbox_assert(inTask->GetState() == TS_RUNNING);

	fImpl.TaskStarted( inTask);
	
	VString	taskName;
	inTask->GetName( taskName);
	fImpl.SetCurrentThreadName( taskName, inTask->GetID());
}


void VTaskMgr::_TaskStopped( VTask *inTask)
{
	// called just before the death of a task
	xbox_assert(inTask->GetState() == TS_DYING);
	
	fImpl.TaskStopped( inTask);
	
	PurgeDeadTasks();
}


VTask* VTaskMgr::RetainTaskByID( const VTaskID inTaskID)
{
	StTaskMgrLocker lock( fCriticalSection);

	VTask* theTask = NULL;
	
	for (VTask* oneTask = fMainTask; oneTask != NULL; oneTask = oneTask->GetNextTask())
	{
		if (oneTask->GetID() == inTaskID && oneTask->GetState() != TS_DEAD)
		{
			theTask = oneTask;
			theTask->Retain();
			break;
		}
	}

	return theTask;
}


void VTaskMgr::GetTasks( std::vector<VRefPtr<VTask> >& outTasks) const
{
	StTaskMgrLocker lock( fCriticalSection);

	for (VTask* oneTask = fMainTask; oneTask != NULL; oneTask = oneTask->GetNextTask())
		outTasks.push_back( oneTask);
}


size_t VTaskMgr::GetStackSizeForMainTask() const
{
	if (fStackSizeForMainTask == 0)
		fStackSizeForMainTask = fImpl.ComputeMainTaskStackSize();
	if (fStackSizeForMainTask == 0)
		fStackSizeForMainTask = 65536L;
	return fStackSizeForMainTask;
}


size_t VTaskMgr::GetDefaultStackSize() const
{
	StTaskMgrLocker lock( fCriticalSection);

	if (fDefaultStackSize == 0)
	{
		// Under Snow Leopard, the default stack size if 8 Mo for the main thread. This is a bit too much.
		size_t defaultSize = GetStackSizeForMainTask();
		fDefaultStackSize = defaultSize > 1048576 ? 1048576 : defaultSize;
	}
	return fDefaultStackSize;
}


void VTaskMgr::SetDefaultStackSize( size_t inSize)
{
	StTaskMgrLocker lock( fCriticalSection);

	fDefaultStackSize = inSize;
}


size_t VTaskMgr::GetCurrentFreeStackSize() const
{
	return fImpl.GetCurrentFreeStackSize();
}
 
 
VTaskID	VTaskMgr::GetNewTaskID( VTask* inTask)
{
	if ( (inTask == NULL) || (inTask->GetImpl() == NULL) )
		return NULL_TASK_ID;
	
	VTaskID id = inTask->GetImpl()->GetValueForVTaskID();
	if (id == NULL_TASK_ID)
	{
		StTaskMgrLocker lock( fCriticalSection);
		if (fRecycledFiberTaskIDs.empty())
		{
			id = --fSeedFiberTaskID;
		}
		else
		{
			id = fRecycledFiberTaskIDs.top();
			fRecycledFiberTaskIDs.pop();
		}
	}
	return id;
}


VTaskID VTaskMgr::GetCurrentTaskID() const
{
	VTaskID id;
	if (sInstanceInited)
	{
		VTask *task = fImpl.GetCurrentTask();
		if (task != NULL)
		{
			id = task->GetID();
		}
		else
		{
			id = fImpl.GetCurrentTaskID();
			if (id == NULL_TASK_ID)
			{
				// assign a new task id for this currently running preemptive thread we don't own.
				// VTaskMgr::GetCurrentTask() will still return NULL but we have an id so one can use our mutexes.
				// ids for such task are always negative.
				fSeedForeignTaskID--;
				if (fSeedForeignTaskID >= 0)
					fSeedForeignTaskID = -1;
				id = fSeedForeignTaskID;
				fImpl.SetCurrentTaskID( id);
			}
		}
	}
	else
	{
		id = NULL_TASK_ID;
	}
	return id;
}



bool VTaskMgr::RegisterDelayedMessage( VMessage* inMessage, sLONG inDelayMilliseconds)
{
	if (testAssert(inMessage != NULL))
	{
		StTaskMgrLocker lock( fCriticalSection);
		
		sLONG ticksTime = VSystem::GetCurrentTicks() + (inDelayMilliseconds*60)/1000;
		inMessage->Retain();
		fDelayedMessages->push_back( inMessage);
		fDelayedMessagesTimes->push_back( ticksTime);
	}
	return true;
}


void VTaskMgr::_CheckDelayedMessages()
{
	sLONG curTicks = VSystem::GetCurrentTicks();
	VMessage* theMessage = NULL;
	do {
		// one must not keep the VTaskLock while reposting a message
		theMessage = NULL;
		{
			StTaskMgrLocker lock( fCriticalSection);
			std::vector<sLONG>::iterator i = fDelayedMessagesTimes->begin();
			for( ; i != fDelayedMessagesTimes->end() && (*i >= curTicks) ; ++i)
				;

			if (i != fDelayedMessagesTimes->end())
			{
				std::vector<VMessage*>::iterator j = fDelayedMessages->begin() + (i - fDelayedMessagesTimes->begin());
				theMessage = *j;
				fDelayedMessages->erase( j);
				fDelayedMessagesTimes->erase( i);
			}
		}
		
		if (theMessage != NULL)
		{
			theMessage->RePost();
			theMessage->Release();
		}
		
	} while(theMessage != NULL);
}


void VTaskMgr::CancelMessages( IMessageable* inTarget)
{
	// cancel delayed messages
	VMessage* msg = NULL;
	do {
		msg = NULL;
		{
			StTaskMgrLocker lock( fCriticalSection);
		
			std::vector<VMessage*>::iterator i = fDelayedMessages->begin();
			for(  ; i != fDelayedMessages->end() && ((*i)->GetTarget() != inTarget) ; ++i)
				;

			if (i != fDelayedMessages->end())
			{
				msg = *i;
				fDelayedMessages->erase( i);
			}
		}
		if (msg != NULL)
		{
			msg->Abort();
			msg->Release();
		}
	} while(msg != NULL);
	
	StTaskMgrLocker lock( fCriticalSection); // warning: should release it before calling CancelMessages

	for (VTask* oneTask = fMainTask; oneTask != NULL; oneTask = oneTask->GetNextTask())
		oneTask->CancelMessages(inTarget);
}


bool VTaskMgr::KillAllTasks( sLONG inMilliSeconds)
{
	if (testAssert( fMainTask->IsCurrent()))
	{
		uLONG t1 = VSystem::GetCurrentTime() + inMilliSeconds;
		do {
			{
				StTaskMgrLocker lock( fCriticalSection);
				// skip main task
				for (VTask* oneTask = fMainTask->GetNextTask(); oneTask != NULL; oneTask = oneTask->GetNextTask())
				{
					if (oneTask->GetState() >= TS_RUNNING)
						oneTask->Kill();
				}
			}
			PurgeDeadTasks();

			YieldNow();

			VTask::ExecuteMessages( false);

			uLONG t0 = VSystem::GetCurrentTime();
			if (t0 >= t1)
				break;
		} while( fMainTask->GetNextTask() != NULL);
	}
	
	return (fMainTask->GetNextTask() == NULL);
}


void VTaskMgr::PurgeDeadTasks()
{
	StTaskMgrLocker lock( fCriticalSection);

	if (fMainTask != NULL)
	{
		VTask* previous = fMainTask;

		// skip main task which can't be dead
		for( VTask* oneTask = fMainTask->GetNextTask() ; oneTask != NULL ; oneTask = previous->GetNextTask())
		{
			if (oneTask->GetState() == TS_DEAD)
			{
				// release it
				previous->SetNextTask(oneTask->GetNextTask());
				oneTask->SetNextTask(NULL); // safeguard

				if (oneTask->GetImpl()->GetValueForVTaskID() != oneTask->GetID())
				{
					fRecycledFiberTaskIDs.push( oneTask->GetID());
				}

				oneTask->Release();

				fTaskListStamp++;
			} else
				previous = oneTask;
		}
	}
}


VTaskDataKey VTaskMgr::CreateDataKey( VTaskDataKeyDisposeProc inDisposeProc)
{
	StTaskMgrLocker lock( fCriticalSection);

	// look for free slot
	
	VTaskDataKey key = 0;
	VTaskDataKeyDesc desc = {inDisposeProc, false};
	
	for( VTaskDataKeyDescVector::iterator i = fDataDescriptors.begin() ; i != fDataDescriptors.end() ; ++i)
	{
		if ((*i).fFree)
		{
			*i = desc;
			key = (i - fDataDescriptors.begin()) + 1;
			break;
		}
	}
	
	// if no free slot, add a new one
	if (key == 0)
	{
		fDataDescriptors.push_back( desc);
		key = fDataDescriptors.size();
	}
	
	return key;
}


void VTaskMgr::DeleteDataKey( VTaskDataKey inKey)
{
	// guard against cleanup on non fully inited application
	if ( (inKey != 0) && sInstanceInited)
	{
		StTaskMgrLocker lock( fCriticalSection);

		if (testAssert( inKey > 0 && inKey <= fDataDescriptors.size()) && !fDataDescriptors[inKey - 1].fFree)
		{
			fDataDescriptors[inKey - 1].fFree = true;
		}
	}
}


bool VTaskMgr::CheckDataKey( VTaskDataKey inKey) const
{
	StTaskMgrLocker lock( fCriticalSection);

	return (inKey > 0) && (inKey <= fDataDescriptors.size()) && !fDataDescriptors[inKey - 1].fFree;
}


void VTaskMgr::DisposeData( VTaskDataVector::iterator inBegin, VTaskDataVector::iterator inEnd)
{
	// don't keep task mgr lock while in dispose proc

	for( VTaskDataVector::iterator j = inBegin ; j != inEnd ; ++j)
	{
		if (*j != NULL)
		{
			VTaskDataKeyDisposeProc proc = NULL;
			{
				StTaskMgrLocker lock( fCriticalSection);
				
				VTaskDataVector::difference_type offset = j - inBegin;
				if (offset < fDataDescriptors.end() - fDataDescriptors.begin())
				{
					VTaskDataKeyDescVector::iterator i = fDataDescriptors.begin() + offset;
					if (!(*i).fFree)
					{
						proc = (*i).fDisposeProc;
					}
				}
			}
				
			if (proc != NULL)
			{
				proc( *j);
			}
		}
	}
}


void VTaskMgr::Yield()
{
	VTask *task = GetCurrentTask();
	if ( (task != NULL) && task->IsForYieldCallbacks())
		fFiberScheduler->Yield();
}



void VTaskMgr::YieldNow()
{
	VTask *task = GetCurrentTask();
	if ( (task != NULL) && task->IsForYieldCallbacks())
		fFiberScheduler->YieldNow();
	else
		fImpl.YieldNow();
}


void VTaskMgr::YieldToFiber( VTask *inPreferredTask)
{
#if !VERSION_LINUX

	fImpl.YieldToFiber( inPreferredTask);

#else
	(void) inPreferredTask; // avoid compiler warning
#endif
}



void VTaskMgr::CurrentFiberWaitFlag( sLONG *inFlagPtr, sLONG inValue)
{
	fFiberScheduler->WaitFlag( inFlagPtr, inValue);
}


void VTaskMgr::CurrentFiberWaitFlagWithTimeout( sLONG *inFlagPtr, sLONG inValue, sLONG inTimeoutMilliseconds)
{
	fFiberScheduler->WaitFlagWithTimeout( inFlagPtr, inValue, inTimeoutMilliseconds);
}



#if VERSIONMAC

void VTaskMgr::MAC_BeginHandlingUpdateEventsOnly()
{	
	// stop WaitNextEvent calls
	VTask::SetSystemEventPolling(false);
	
	// install a timer on the main run loop
	CFAbsoluteTime firedate = CFAbsoluteTimeGetCurrent();
	CFTimeInterval interval = (CFTimeInterval) 0.01;
	CFRunLoopRef loopref = CFRunLoopGetCurrent ();
	
	xbox_assert( fTimer == NULL);
	fTimer = CFRunLoopTimerCreate(kCFAllocatorDefault, firedate, interval, 0, 0, VTaskMgr::MAC_RunLoopTimerCallback, NULL);
	CFRunLoopAddTimer(loopref, fTimer, kCFRunLoopCommonModes);
}


void VTaskMgr::MAC_StopHandlingUpdateEventsOnly()
{
	// remove the timer
	CFRunLoopTimerInvalidate(fTimer);
	CFRelease( fTimer);
	fTimer = NULL;
	
	// allow WaitNextevent calls
	VTask::SetSystemEventPolling(true);
}


void VTaskMgr::MAC_RunLoopTimerCallback( CFRunLoopTimerRef inTimer, void *inInfo)
{
	VTask::Yield();
	VTask::PollAndExecuteUpdateEvent();
}

#endif //VERSIONMAC


void VTaskMgr::SetSystemEventPolling( bool inSet)
{
	fFiberScheduler->SetSystemEventPolling( inSet);
}


void VTaskMgr::PollAndExecuteUpdateEvent()
{
	fFiberScheduler->PollAndExecuteUpdateEvent();
}


void VTaskMgr::SetDirectUpdate( bool inIsOn)
{
	fFiberScheduler->SetDirectUpdate( inIsOn);
}


VTaskDataKey VTask::sHTTPServerSessionDataKey = 0;

VTaskDataKey VTask::GetHTTPServerSessionDataKey()
{
	if (sHTTPServerSessionDataKey == 0)
		sHTTPServerSessionDataKey = VTask::CreateDataKey(NULL);
	return sHTTPServerSessionDataKey;
}

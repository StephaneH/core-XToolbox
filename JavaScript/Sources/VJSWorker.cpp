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
#include "VJavaScriptPrecompiled.h"

#include "VJSWorker.h"

#include "VJSClass.h"
#include "VJSGlobalClass.h"

#include "VJSEvent.h"
#include "VJSMessagePort.h"
#include "VJSWorkerProxy.h"

#include "VJSW3CFileSystem.h"

#include "VJSProcess.h"
#include "VJSWebSocket.h"
#include "VJSSystemWorker.h"

USING_TOOLBOX_NAMESPACE

// VJSIsInitializedMessage has not content. It is used to notify parent that child has finished
// initialization (successfully or failed). In case of error, it is thrown.

class VJSIsInitializedMessage : public XBOX::VMessage
{
public:

					VJSIsInitializedMessage (XBOX::VError inError, const XBOX::VString &inExplanation);
	void			DoExecute ();

private:

	XBOX::VError	fError;
	XBOX::VString	fExplanation;

	virtual			~VJSIsInitializedMessage()	{}
};

VJSIsInitializedMessage::VJSIsInitializedMessage (XBOX::VError inError, const XBOX::VString &inExplanation)
{
	fError = inError; 
	fExplanation = inExplanation;	// Can be empty.
}

void VJSIsInitializedMessage::DoExecute ()
{
	if (fError != XBOX::VE_OK)

		XBOX::vThrowError(fError, fExplanation);
}

XBOX::VCriticalSection	VJSWorker::sMutex;				
std::list<VJSWorker *>	VJSWorker::sDedicatedWorkers;	
std::list<VJSWorker *>	VJSWorker::sSharedWorkers;
std::list<VJSWorker *>	VJSWorker::sRootWorkers;
IJSWorkerDelegate*		VJSWorker::sDelegate = NULL;

VJSHasEventMessage::VJSHasEventMessage (VJSWorker *inWorker)
{
	xbox_assert(inWorker != NULL);

	fWorker = XBOX::RetainRefCountable<VJSWorker>(inWorker);
}

VJSHasEventMessage::~VJSHasEventMessage ()
{
	XBOX::ReleaseRefCountable<VJSWorker>(&fWorker);
}

void VJSHasEventMessage::DoExecute ()
{
	fWorker->ExecuteHasEventMessage(this);
}

void VJSWorker::SetDelegate (IJSWorkerDelegate *inDelegate)
{
	// Set to NULL when finishing.

	sDelegate = inDelegate;
}

VJSWorker::VJSWorker (XBOX::VJSContext &inParentContext, 
	XBOX::VJSWorker *inParentWorker, 
	const XBOX::VString &inURL, 
	bool inReUseContext, 
	VJSWebSocketObject *inWebSocket)
:fParentContext(inParentContext)
,fReUseContext(inReUseContext)
{
	xbox_assert((inParentWorker != NULL && inWebSocket == NULL) 
	|| (inParentWorker == NULL && inWebSocket != NULL));

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&sMutex);
	XBOX::VError							error;
		
	fStartTime.FromSystemTime();	 
	
	fInsideWaitCount = 0;
	fTotalIdleDuration.Clear();

	if (inParentWorker != NULL)

		fParentWorkers.push_back(inParentWorker);
	
	fHasReleasedAll = fClosingFlag = fIsLockedWaiting = fExitWaitFlag = false;

	fWorkerType = TYPE_DEDICATED;
	fURL = inURL;
	fOnMessagePort = NULL;

	fRootVTask = XBOX::VTask::GetCurrent();
	fLocalFileSystem = NULL;

	if (inWebSocket != NULL)

		fWebSocket = XBOX::RetainRefCountable<VJSWebSocketObject>(inWebSocket);

	else

		fWebSocket = NULL;

	fGlobalContext = NULL;
	Retain();

#if VJSWORKER_WITH_PROJECT_INFO_RETAIN_JS

	fProjectInfo = sDelegate->GetProjectInfo(error, inParentContext);

#endif

	fVTask = new XBOX::VTask(NULL, 0, XBOX::eTaskStylePreemptive, _RunProc);
	fVTask->SetName("VJSWorker dedicated handler");
	fVTask->SetKindData((sLONG_PTR) this);

	sDedicatedWorkers.push_back(this);
}

VJSWorker *VJSWorker::RetainSharedWorker (XBOX::VJSContext &inParentContext,	
	XBOX::VJSWorker *inParentWorker,
	const XBOX::VString &inURL, 
	const XBOX::VString &inName, 
	bool inReUseContext, 
	bool *outCreatedWorker)
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&sMutex);	
	VJSWorker								*sharedWorker;
	std::list<VJSWorker *>::iterator		i;	
	
	sharedWorker = NULL;
	for (i = sSharedWorkers.begin(); i != sSharedWorkers.end(); i++) {

		xbox_assert((*i)->fWorkerType == TYPE_SHARED);
		if ((*i)->fURL.EqualToString(inURL) && (*i)->fName.EqualToString(inName)) {

			// The SharedWorker has already been instantied.

			(*i)->Retain();
			if (inParentWorker != NULL) {

				(*i)->fMutex.Lock();
				(*i)->fParentWorkers.push_back(inParentWorker);
				(*i)->fMutex.Unlock();
				inParentWorker->fSharedWorkers.push_back(*i);

			}

			*outCreatedWorker = false;
			sharedWorker = *i;

			break;

		}

	}

	if (sharedWorker == NULL) {

		*outCreatedWorker = true;

		sharedWorker = new VJSWorker(inParentContext, inParentWorker, inURL, inName, inReUseContext);
		if (inParentWorker != NULL)

			inParentWorker->fSharedWorkers.push_back(sharedWorker);

	}

	return sharedWorker;
}

void VJSWorker::SetMessagePort (VJSMessagePort *inMessagePort)
{
	xbox_assert(IsDedicatedWorker() && inMessagePort != NULL);
	
	fOnMessagePort = XBOX::RetainRefCountable<VJSMessagePort>(inMessagePort);
}
	
void VJSWorker::Connect (VJSMessagePort *inMessagePort, VJSMessagePort *inErrorPort)
{
	xbox_assert(!IsDedicatedWorker());

	if (fConnectionPort == NULL) {
	
		sLONG	time = 0;

		while (fConnectionPort == NULL || time <= 1000) {
		
			VTask::Sleep(10);
			time += 10;
		
		}
	
	}
	
	if (fConnectionPort != NULL)

		QueueEvent(VJSConnectionEvent::Create(fConnectionPort, inMessagePort, inErrorPort));

	else

		vThrowError(XBOX::VE_JVSC_INVALID_STATE);
}

void VJSWorker::ConnectWebSocket (XBOX::VWebSocket *inWebSocket)
{
	xbox_assert(inWebSocket != NULL);
	xbox_assert(!IsDedicatedWorker());

	QueueEvent(VJSWebSocketConnectEvent::Create(inWebSocket));
}

void VJSWorker::AddErrorPort (VJSMessagePort *inErrorPort)
{
	xbox_assert(inErrorPort != NULL);
	
	// Clean-up first.

	XBOX::StLocker<XBOX::VCriticalSection>			lock(&fMutex);	
	std::list< VRefPtr<VJSMessagePort> >::iterator	i;

	for (i = fErrorPorts.begin(); i != fErrorPorts.end(); )

		if ((*i)->IsOtherClosed(this)) {

			(*i)->Close(this);
			i = fErrorPorts.erase(i);

		} else

			i++;

	// Then add error port.
	
	fErrorPorts.push_back(inErrorPort);
}

void VJSWorker::BroadcastError (const XBOX::VString &inMessage, const XBOX::VString &inFileNumber, sLONG inLineNumber)
{
	XBOX::StLocker<XBOX::VCriticalSection>			lock(&fMutex);	
	std::list< VRefPtr<VJSMessagePort> >::iterator	i;

	for (i = fErrorPorts.begin(); i != fErrorPorts.end(); ) {

		if ((*i)->IsOtherClosed(this)) {

			// Clean-up.

			(*i)->Close(this);
			i = fErrorPorts.erase(i);

		} else {

			(*i)->PostError((*i)->GetOther(this), inMessage, inFileNumber, inLineNumber); 
			i++;

		}
		
	}
}

void VJSWorker::Run ()
{
	xbox_assert(fWorkerType == TYPE_DEDICATED || fWorkerType == TYPE_SHARED);
		
	if (fVTask != NULL) {
				
		fVTask->Run();	// _DoRun() will release refcount.
		
		XBOX::VMessage	*message;

		message = fVTask->RetainMessageWithTimeout(5000);
		if (message != NULL) {

			message->Execute();
			message->Release();
		}		
		
	} else {

		// VTask allocation failed !

		xbox_assert(fGlobalContext == NULL);

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
		Release();

	}
}

sLONG VJSWorker::WaitFor (XBOX::VJSContext &inContext, sLONG inWaitingDuration, IJSEventGenerator *inEventGenerator, sLONG inEventType)
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fWaitForMutex);	

//**	XBOX::DebugMsg("%p inside wait, workertype = %d\n", this, fWorkerType);
 
	fInsideWaitCount++;

	bool		isEventMatched;
	XBOX::VTime	endTime, currentTime;
	
	isEventMatched = false;		
	if (inWaitingDuration >= 0) {

		endTime.FromSystemTime();
		endTime.AddMilliseconds(inWaitingDuration);

	}

	fMutex.Lock();
	currentTime.FromSystemTime();
	while (!fClosingFlag && !fExitWaitFlag) {
		
		// If there is an event to be triggered, process it.
		
		if (!fEventQueue.empty() && currentTime >= fEventQueue.front()->GetTriggerTime()) {

			IJSEvent	*event;

			event = fEventQueue.front();
			fEventQueue.pop_front();

			// If an event generator and event type has been specified, check if the event to process is matching.

			if (inEventGenerator != NULL && inEventGenerator->MatchEvent(event, inEventType))

				isEventMatched = true;

			// Process event.

			fMutex.Unlock();

			if ((JS4D::ContextRef) inContext == NULL) {
			
				VJSContext	context(fGlobalContext);

				event->Process(context, this);

			} else

				event->Process(inContext, this);

			fMutex.Lock();

			// Event has been matched, exit.

			if (isEventMatched)

				break;

			currentTime.FromSystemTime();

			// If waiting time has elapsed, exit. If polling (inWaitingDuration is zero), execute all pending events.
			// Note: Place a limit on polling executions ? If no limit (always execute all pending events), the wait() 
			// may never end (events keep adding new events).

			if (inWaitingDuration > 0 && currentTime >= endTime) 
	
				break;	

			else

				continue;

		}

		// Check if we are polling. If so, do not wait, just exit.

		if (!inWaitingDuration)

			break;

		// Compute time to wait until then next event.
		
		bool			isTimedOutWait;
		XBOX::VDuration	duration;
		uLONG			waitDuration;
						
		isTimedOutWait = true;
		if (fEventQueue.empty()) {

			if (inWaitingDuration > 0) {

				if (endTime > currentTime) {

					endTime.Substract(currentTime, duration);
					waitDuration = duration.ConvertToMilliseconds();

				} else 

					break;	// Waiting duration elapsed.

			} else

				isTimedOutWait = false;
				
		} else {

			XBOX::VTime	minimum;

			minimum = fEventQueue.front()->GetTriggerTime();
			if (inWaitingDuration > 0 && minimum > endTime)

				minimum = endTime;
		
			minimum.Substract(currentTime, duration);	
			waitDuration = duration.ConvertToMilliseconds();
			
		}

		// Wait until it's time to trigger next event.
		
		XBOX::VTime	waitStartTime;
		bool		syncBoolean;
		
		waitStartTime = currentTime;
		fIsLockedWaiting = true;
		fMutex.Unlock();
		if (isTimedOutWait) {
		
			if (waitDuration)

				syncBoolean = fSyncEvent.Lock(waitDuration);

			else

				syncBoolean = false;		
		
		} else {

			syncBoolean = fSyncEvent.Lock();

			// Stopped waiting because fClosingFlag has been set to true or an event has been queued.
			// Cannot be a timeout.

			xbox_assert(syncBoolean);	

		}

		// If an fSyncEvent.Unlock() is sent, it will be ignored.

		fIsLockedWaiting = false;
		fMutex.Lock();

		fSyncEvent.Reset();

		// Find out the actual time idled for statistics.
		
		XBOX::VDuration	actualWaitDuration;
		
		currentTime.FromSystemTime();
		currentTime.Substract(waitStartTime, actualWaitDuration);
		
		fTotalIdleDuration.Add(actualWaitDuration);		
		
		// If timeout, check if waiting time has elapsed.
				
		if (!syncBoolean && inWaitingDuration > 0 && currentTime >= endTime) 
	
			break;	

	}
	fMutex.Unlock();
	
	fInsideWaitCount--;
	xbox_assert(fInsideWaitCount >= 0);

	fExitWaitFlag = false;

//**	XBOX::DebugMsg("%p outside wait, workertype = %d\n", this, fWorkerType);

	if (inEventGenerator != NULL && isEventMatched)
 
		return -1;

	else 

		return fClosingFlag ? 1 : 0;
}

void VJSWorker::Terminate ()
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	fClosingFlag = true;

	// If locked waiting for an event, send signal.

	if (fIsLockedWaiting)

		fSyncEvent.Unlock();
}
   
void VJSWorker::TerminateAll ()
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&sMutex);
	std::list<VJSWorker *>::iterator		i;

	VJSSystemWorker::KillAll();	// Request all system workers to terminate.

	for (i = sDedicatedWorkers.begin(); i != sDedicatedWorkers.end(); i++)

		(*i)->Terminate();

	for (i = sSharedWorkers.begin(); i != sSharedWorkers.end(); i++)

		(*i)->Terminate();

	for (i = sRootWorkers.begin(); i != sRootWorkers.end(); i++)

		(*i)->Terminate();
}

bool VJSWorker::RecycleWorker (const XBOX::VJSContext &inContext)
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&sMutex);
	XBOX::VJSGlobalObject					*globalObject;
	VJSWorker								*worker;
	
	globalObject = inContext.GetGlobalObjectPrivateInstance();
	xbox_assert(globalObject != NULL);

	worker = (VJSWorker *)globalObject->GetSpecific((VJSSpecifics::key_type) kSpecificKey);
	if (worker != NULL) {

		worker->fClosingFlag = false;
		return true;

	} else

		return false;
}

VJSWorker *VJSWorker::RetainWorker (const XBOX::VJSContext &inContext)
{
	XBOX::VJSGlobalObject	*globalObject;
	VJSWorker				*worker;
	bool					needRetain;

	globalObject = inContext.GetGlobalObjectPrivateInstance();
	xbox_assert(globalObject != NULL);

	// Only "root" javascript execution can return NULL because it hasn't been created as a dedicated or shared worker. 
	// In which case, an empty (without vtask and no postMessage() method or onmessage attribute globals) worker is created.

	if ((worker = (VJSWorker *) globalObject->GetSpecific((VJSSpecifics::key_type) kSpecificKey)) == NULL) {

		XBOX::StLocker<XBOX::VCriticalSection>	lock(&sMutex);
	
		// Re-check with mutex acquired (race condition avoidance).

		if ((worker = (VJSWorker *) globalObject->GetSpecific((VJSSpecifics::key_type) kSpecificKey)) == NULL) {

			worker = new VJSWorker(inContext);
			needRetain = false;

		} else

			needRetain = true;
			
	} else 

		needRetain = true;

	xbox_assert(worker != NULL);

	if (needRetain)

		worker->Retain();

	return worker;
}

void VJSWorker::QueueEvent (IJSEvent *inEvent)
{
	xbox_assert(inEvent != NULL);

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	// If worker has already been garbage collected, but C++ object is still "alive" (refcount), 
	// then it will never process events. So discard the event.

	if (fHasReleasedAll) {

		inEvent->Discard();
		return;

	}

	std::list<IJSEvent *>::iterator	i;
	
	for (i = fEventQueue.begin(); i != fEventQueue.end(); i++) 

		// If two events have identical trigger time, first queued will be processed first.
		
		if ((*i)->GetTriggerTime() > inEvent->GetTriggerTime())

			break;

	fEventQueue.insert(i, inEvent);	

	// Send a VMessage to running VTask, only in studio and for SystemWorker events.

	if (sDelegate != NULL && sDelegate->GetType() == IJSWorkerDelegate::TYPE_WAKANDA_STUDIO
	&& inEvent->GetType() == IJSEvent::eTYPE_SYSTEM_WORKER && fRootVTask != NULL) {

		VJSHasEventMessage	*message;

		if ((message = new VJSHasEventMessage(this)) != NULL) {

			message->PostTo(fRootVTask);
			XBOX::ReleaseRefCountable<VJSHasEventMessage>(&message);

		}

	}

	// If locked waiting for an event, send signal.

	if (fIsLockedWaiting)

		fSyncEvent.Unlock();
}

void VJSWorker::UnscheduleTimer (VJSTimer *inTimer)
{
	xbox_assert(inTimer != NULL);

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	VJSTimerEvent::RemoveTimerEvent(&fEventQueue, inTimer);	
}

void VJSWorker::AddMessagePort (VJSMessagePort *inMessagePort)
{
	xbox_assert(inMessagePort != NULL);

	_AddMessagePort(&fMessagePorts, inMessagePort);
}

void VJSWorker::RemoveMessagePort (VJSMessagePort *inMessagePort)
{
//	xbox_assert(inMessagePort != NULL);

	_RemoveMessagePort(&fMessagePorts, inMessagePort);
}

void VJSWorker::Close (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *inGlobalObject)
{
	VJSWorker	*worker;

	worker = VJSWorker::RetainWorker(ioParms.GetContext());
	worker->Terminate();

	if (worker->IsDedicatedWorker() && worker->fWebSocket != NULL) {

		uBYTE	reason[2];
		bool	isOk;

		reason[0] = 1000 >> 8;
		reason[1] = 1000 & 0xff;
		worker->fWebSocket->Close(reason, 2, &isOk);

	}

	XBOX::ReleaseRefCountable<VJSWorker>(&worker);
}

void VJSWorker::Wait (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *inGlobalObject)
{
	XBOX::VJSContext	context(ioParms.GetContext());
	sLONG				waitDuration;
	VJSWorker			*worker;
		
	waitDuration = GetWaitDuration(ioParms);
	worker = VJSWorker::RetainWorker(context);
	ioParms.ReturnBool(worker->WaitFor(context, waitDuration, NULL, 0) == 1);
	XBOX::ReleaseRefCountable<VJSWorker>(&worker);
}

void VJSWorker::ExitWait (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *inGlobalObject)
{
	VJSWorker	*worker;

	worker = VJSWorker::RetainWorker(ioParms.GetContext());
	if (worker->fInsideWaitCount > 0) {

		XBOX::StLocker<XBOX::VCriticalSection>	lock(&worker->fMutex);

		worker->fExitWaitFlag = true;
		if (worker->fIsLockedWaiting)

			worker->fSyncEvent.Unlock();

	}
	XBOX::ReleaseRefCountable<VJSWorker>(&worker);
}

VJSLocalFileSystem *VJSWorker::RetainLocalFileSystem ()
{
	return XBOX::RetainRefCountable<VJSLocalFileSystem>(fLocalFileSystem);	
}

sLONG VJSWorker::GetWaitDuration (VJSParms_callStaticFunction &ioParms)
{
	// If no duration (in milliseconds) or if it is invalid (negative), wait duration is set to -1 (infinite).

	if (!ioParms.CountParams())

		return -1;

	Real	duration;

	if (!ioParms.GetRealParam(1, &duration))

		return -1;

	// If negative, infinite, or NaN, return -1 (infinite).

	if (duration < 0.0 || duration > XBOX::kMAX_Real || duration != duration)

		return -1;

	else

		return (sLONG) duration;	
}

bool VJSWorker::IsSharedWorkerRunning (const XBOX::VString &inURL, const XBOX::VString &inName)
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&sMutex);	// C++ std is not thread safe.
	std::list<VJSWorker *>::const_iterator	i;
	
	for (i = sSharedWorkers.begin(); i != sSharedWorkers.end(); i++) {

		xbox_assert((*i)->fWorkerType == TYPE_SHARED);
		if ((*i)->fURL.EqualToString(inURL) && (*i)->fName.EqualToString(inName)) 
			
			return true;

	}

	return false;
}

sLONG VJSWorker::GetNumberRunning (sLONG inType)
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&sMutex);	// C++ std is not thread safe.

	if (inType == TYPE_ROOT) 

		return (sLONG) sRootWorkers.size();

	else if (inType == TYPE_DEDICATED)

		return (sLONG) sDedicatedWorkers.size();

	else if (inType == TYPE_SHARED)

		return (sLONG) sSharedWorkers.size();

	else {

		xbox_assert(false);	// Should never happen.
		return 0;

	}
}

void VJSWorker::ExecuteHasEventMessage (VJSHasEventMessage *inMessage)
{
	if (fWaitForMutex.TryToLock()) {
	
		if (!fInsideWaitCount && fEventQueue.size()) {

			XBOX::VJSContext		context((XBOX::JS4D::ContextRef) fRootGlobalContext);
			XBOX::VJSGlobalObject	*globalObject	= context.GetGlobalObjectPrivateInstance();

			xbox_assert(globalObject != NULL);

			// If the interpreter is running (probably in a wait()), "drop" the message but do not modify the event queue.
			// So even if the message is dropped, the event is still there.

			if (globalObject->TryToUseContext()) {

				WaitFor(context, 10, NULL, 0);
				globalObject->UnuseContext();

			}

		}
		fWaitForMutex.Unlock();

	}
	inMessage->AnswerIt();
}

// sMutex must be locked before creating a new shared worker.

VJSWorker::VJSWorker (XBOX::VJSContext &inParentContext, XBOX::VJSWorker *inParentWorker, const XBOX::VString &inURL, const XBOX::VString &inName, bool inReUseContext)
:fParentContext(inParentContext)
, fReUseContext(inReUseContext)
{
	// Shared worker instantied by a WebSocket handler don't have a parent worker (inParentWorker == NULL).

	XBOX::VError	error;
	
	fStartTime.FromSystemTime();	
	
	fInsideWaitCount = 0;
	fTotalIdleDuration.Clear();
	
	fWorkerType = TYPE_SHARED;
	fURL = inURL;
	fName = inName;

	if (inParentWorker != NULL)

		fParentWorkers.push_back(inParentWorker);

	fHasReleasedAll = fClosingFlag = fIsLockedWaiting = fExitWaitFlag = false;

	fRootVTask = XBOX::VTask::GetCurrent();
	fLocalFileSystem = NULL;
	fWebSocket = NULL;

	fGlobalContext = NULL;
	Retain();

#if VJSWORKER_WITH_PROJECT_INFO_RETAIN_JS

	fProjectInfo = sDelegate->GetProjectInfo(error, inParentContext);

#endif

	fConnectionPort = NULL;

	fVTask = new XBOX::VTask(NULL, 0, XBOX::eTaskStylePreemptive, _RunProc);
	fVTask->SetName("VJSWorker shared handler");
	fVTask->SetKindData((sLONG_PTR) this);

	sSharedWorkers.push_back(this);
}

// sMutex must be locked before creating an "empty" worker for "root".

VJSWorker::VJSWorker (const XBOX::VJSContext &inContext)
:fParentContext(inContext)
,fReUseContext(false)
{
	fGlobalContext = NULL;
	
	fStartTime.FromSystemTime();	
	
	fInsideWaitCount = 0;
	fTotalIdleDuration.Clear();

	fURL = "";	//** TODO: Set proper name?

	fVTask = NULL;
	fHasReleasedAll = fClosingFlag = fIsLockedWaiting = fExitWaitFlag = false;

	fWorkerType = TYPE_ROOT;
	fRootVTask = XBOX::VTask::GetCurrent();
	fRootGlobalContext = inContext.GetGlobalObjectPrivateInstance()->GetContext();

	fLocalFileSystem = NULL;
	fWebSocket = NULL;

	_PopulateGlobalObject(inContext);

	sRootWorkers.push_back(this);
}

// See comment in _PopulateGlobalObject() about destructor call.

VJSWorker::~VJSWorker ()
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&sMutex);

	// All references should have been released.
	
	xbox_assert(fEventQueue.empty());

	// Free VTask and remove worker from dedicated, shared, or root worker list.

	std::list<VJSWorker *>	*list;
	
	if (fWorkerType != TYPE_ROOT) {

		// If not "root", release context.

		xbox_assert(fWorkerType == TYPE_DEDICATED || fWorkerType == TYPE_SHARED);

		// If the dedicated or shared worker has started successfully, release its VTask.

		if (fVTask != NULL) {

			XBOX::ReleaseRefCountable<XBOX::VTask>(&fVTask);
			list = fWorkerType == TYPE_DEDICATED ? &sDedicatedWorkers : &sSharedWorkers;

		} else

			list = NULL;	// If not started, not in any list.

	} else {

		xbox_assert(fGlobalContext == NULL);

		list = &sRootWorkers;

	}	

	if (list != NULL) {
	
		std::list<VJSWorker* >::iterator	j;

		for (j = list->begin(); j != list->end(); j++) 

			if ((VJSWorker *) *j == this) 

				break;

		if (j != list->end())

			list->erase(j);

		else {

			if (fWorkerType != TYPE_SHARED) {

				xbox_assert(false);		// Impossible!

			}

		}

	}

	// Release file systems, if any.

	fLocalFileSystem->ReleaseAllFileSystems(); 
	ReleaseRefCountable<VJSLocalFileSystem>(&fLocalFileSystem);
	
#if VERSIONDEBUG
	
	_DumpExecutionStatistics();
	
#endif
}

void VJSWorker::_AddMessagePort (std::list< VRefPtr<VJSMessagePort> > *ioMessagePorts, VJSMessagePort *inMessagePort)
{
	xbox_assert(ioMessagePorts != NULL && inMessagePort != NULL);

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	ioMessagePorts->push_back(inMessagePort);
}

void VJSWorker::_RemoveMessagePort (std::list< VRefPtr<VJSMessagePort> > *ioMessagePorts, VJSMessagePort *inMessagePort)
{
//	xbox_assert(ioMessagePorts != NULL && inMessagePort != NULL);

	XBOX::StLocker<XBOX::VCriticalSection>			lock(&fMutex);
	std::list< VRefPtr<VJSMessagePort> >::iterator	i;

	for (i = ioMessagePorts->begin(); i != ioMessagePorts->end(); i++) 

		if ((VJSMessagePort *) *i == inMessagePort)

			break;

	// Check only if "main" message port list fMessagePorts.

//	xbox_assert(!(ioMessagePorts == &fMessagePorts && i != ioMessagePorts->end()));	// assert() à remettre.

	if (i != ioMessagePorts->end())

		ioMessagePorts->erase(i);
}

void VJSWorker::_PopulateGlobalObject (const XBOX::VJSContext &inContext)
{
	XBOX::VJSObject	globalObject	= inContext.GetGlobalObject();

	if (fWorkerType == TYPE_DEDICATED) {
	
		XBOX::VJSObject	postMessageObject(inContext);

		if (fWebSocket == NULL) 

			postMessageObject.MakeCallback(XBOX::js_callback<VJSGlobalObject, _PostMessage>);

		else {

			xbox_assert(fWorkerType == TYPE_DEDICATED);
			postMessageObject.MakeCallback(XBOX::js_callback<VJSGlobalObject, _PostWebSocketMessage>);			
			globalObject.SetProperty("binaryType", "buffer");

		}
		globalObject.SetProperty("postMessage", postMessageObject);

	}	

	globalObject.SetProperty("self", globalObject);
	globalObject.SetProperty("location", VJSWorkerLocationClass::CreateInstance(inContext, new VJSWorkerLocationObject(fURL)));

	// Setup W3C File System API.

	xbox_assert(fLocalFileSystem == NULL);

	XBOX::VFolder		*folder;
	XBOX::VJSContext	context(inContext);
	
	folder = inContext.GetGlobalObjectPrivateInstance()->GetRuntimeDelegate()->RetainScriptsFolder();
	fLocalFileSystem = VJSLocalFileSystem::CreateLocalFileSystem(context, folder->GetPath());
	ReleaseRefCountable<XBOX::VFolder>(&folder);

	// All "root", dedicated, and shared workers are refcountable objects. Do a retain for the SetSpecific().
	
	Retain();	
	inContext.GetGlobalObjectPrivateInstance()->SetSpecific(
		(VJSSpecifics::key_type) kSpecificKey, 
		this, 
		_SetSpecificDestructor);		
}

void VJSWorker::_SetSpecificDestructor (void *p) 
{	
	VJSWorker	*worker;

	worker = (VJSWorker *) p;
	worker->_ReleaseAllReferences();
	worker->Release();
}

bool VJSWorker::_LoadAndCheckScript (XBOX::VError *outError, XBOX::VString *outExplanation)
{
	xbox_assert(outError != NULL && outExplanation != NULL);
	xbox_assert(fGlobalContext != NULL);

	XBOX::VString	fullPath;

#if VERSIONWIN

	if (fURL.GetLength() > 3 
	&& ::isalpha(fURL.GetUniChar(1))
	&& fURL.GetUniChar(2) == ':' 
	&& fURL.GetUniChar(3) == '/')

#else 

	if (fURL.GetUniChar(1) == '/') 

#endif

		fullPath = fURL;

	else {

		XBOX::VJSContext	context(fGlobalContext);
		XBOX::VFolder		*folder;
		XBOX::VString		path;		
		
		folder = context.GetGlobalObjectPrivateInstance()->GetRuntimeDelegate()->RetainScriptsFolder();
		folder->GetPath(path, XBOX::FPS_POSIX);
		XBOX::ReleaseRefCountable<XBOX::VFolder>(&folder);
		fullPath = path + fURL;

	}

	XBOX::VFile			file(fullPath, XBOX::FPS_POSIX);
	XBOX::VFileStream	stream(&file);
	XBOX::VError		error;
	
	if ((error = stream.OpenReading()) == XBOX::VE_OK
	&& (error = stream.GuessCharSetFromLeadingBytes(XBOX::VTC_DefaultTextExport)) == XBOX::VE_OK) {

		stream.SetCarriageReturnMode(XBOX::eCRM_NATIVE);
		error = stream.GetText(fScript);

	}
	stream.CloseReading();
	
	if (error != XBOX::VE_OK) {

		// Fail reading script file, advise parent.

		*outError = XBOX::VE_JVSC_SCRIPT_NOT_FOUND;
		*outExplanation = fullPath;

		XBOX::vThrowError(XBOX::VE_JVSC_SCRIPT_NOT_FOUND, fullPath);
		BroadcastError("Unable to read script file!", fURL, 0);

	} else {

		XBOX::VJSContext	context(fGlobalContext);
		VJSException		exception;
		
		fURLObject = XBOX::VURL(fullPath, eURL_POSIX_STYLE, CVSTR ("file://"));			
		if (!context.CheckScriptSyntax(fScript, &fURLObject, &exception)) {

			// Syntax error!

			*outError = XBOX::VE_JVSC_SYNTAX_ERROR;
			*outExplanation = fullPath;

			XBOX::vThrowError(XBOX::VE_JVSC_SYNTAX_ERROR, fullPath);
			if (exception.GetValue() != NULL) {

				XBOX::VJSValue	value(context,exception); 

				if (value.IsObject()) {

					XBOX::VJSObject	errorObject(context);
					XBOX::VString	message;
					sLONG			lineNumber;

					errorObject = value.GetObject();

					message = "";
					if (errorObject.HasProperty("message"))

						errorObject.GetPropertyAsString("message", NULL, message);

					lineNumber = 0;
					if (errorObject.HasProperty("line")) {

						bool	ok;

						lineNumber = errorObject.GetPropertyAsLong("line", NULL, &ok);
						xbox_assert(ok);

					}

					BroadcastError(message, fURL, lineNumber);

				}

			}

		} else {

			*outError = XBOX::VE_OK;
			outExplanation->Clear();

		}

	}

	return *outError == XBOX::VE_OK;;
}

sLONG VJSWorker::_RunProc (XBOX::VTask *inVTask)
{
	((VJSWorker *) inVTask->GetKindData())->_DoRun();	
	return 0;
}

void VJSWorker::_DoRun ()
{
	xbox_assert(fWorkerType == TYPE_DEDICATED || fWorkerType == TYPE_SHARED);
	xbox_assert(fGlobalContext == NULL);

	XBOX::VError	error;

	// Retain a context.

	sMutex.Lock();

#if VJSWORKER_WITH_PROJECT_INFO_RETAIN_JS

	fGlobalContext = sDelegate->RetainJSContext(error, fProjectInfo, fReUseContext);
	
#else 

	fGlobalContext = sDelegate->RetainJSContext(error, fParentContext, fReUseContext);

#endif
	
	sMutex.Unlock();
	
	if (fGlobalContext != NULL) {
		
		XBOX::VJSContext	context(fGlobalContext);
		XBOX::VJSObject     globalObject = context.GetGlobalObject();
		
		// Finish to initialize the worker's runtime context.

		if (fWorkerType == TYPE_DEDICATED) {

			if (fWebSocket != NULL) {

				fWebSocket->RetainAndNeutralize(globalObject, this);

				// Object was already retained by the VJSWorker constructor. 
				// This is one time too often, release it.

				fWebSocket->Release();

			} else {

				fOnMessagePort->SetObject(this, globalObject);
			
				// Dedicated workers have on "onerror" callback, which is the one and only first error port of the list.

				xbox_assert(fErrorPorts.front() != NULL);
				fErrorPorts.front()->SetObject(this, globalObject);

			}

		}
		else {

			fConnectionPort = VJSMessagePort::Create(this, globalObject, "onconnect");
			AddMessagePort(fConnectionPort);

		}

		_PopulateGlobalObject(context);
		
		// Load and execute script then go to event (wait) loop.

		bool			isOk;
		XBOX::VString	explanation;

		isOk = _LoadAndCheckScript(&error, &explanation);
		_NotifyParent(error, explanation);
		
		// If ok, then run the script.

		if (isOk) {

			JS4D::ExceptionRef	exception;
			
			if (!context.EvaluateScript(fScript, &fURLObject, NULL, &exception, &globalObject)) {

				//** TODO:	Faire la différence entre une exception runtime de type Error 
				//			(avec message + numéro de ligne), et une exception quelconque.

				if (exception != NULL)

					BroadcastError("Uncaught exception!", fURL, 0);

			} else {

				XBOX::VJSContext	nullContext((JS4D::ContextRef) NULL);

				WaitFor(nullContext, -1, NULL, 0);

				// Implement the "exit" event.

				XBOX::VJSObject	processObject = globalObject.GetPropertyAsObject("process");

				if (processObject.IsObject() && processObject.IsOfClass(VJSProcess::Class())) {

					VJSEventEmitter	*eventEmitter;

					if ((eventEmitter = processObject.GetPrivateData<VJSProcess>()) != NULL) {

						std::vector<XBOX::VJSValue>	arguments;

						eventEmitter->Emit(context, "exit", &arguments);

					}
					else {

						// Fail safe, should never happen.

						xbox_assert(false);

					}

				}

				std::list<VRefPtr<VJSWorker> >::iterator	it;

				for (it = fParentWorkers.begin(); it != fParentWorkers.end(); it++) {

					//**			DebugMsg("queue termination from %p to %p\n", this, (*it));
					(*it)->QueueEvent(VJSTerminationEvent::Create(this));

				}

				fParentWorkers.clear();	// Decrement all refcounts.

			}

		}

		// Release everything.

		sDelegate->ReleaseJSContext(fGlobalContext);

	} else

		_NotifyParent(XBOX::VE_MEMORY_FULL, "");	// Failed to retain a JS context.

	// For shared workers only, remove it from list.

	if (fWorkerType == TYPE_SHARED) {

		XBOX::VTaskLock						lock(&sMutex);
		std::list<VJSWorker *>::iterator	it;

 		for (it = sSharedWorkers.begin(); it != sSharedWorkers.end(); it++) 

			if ((VJSWorker *) *it == this) 

				break;

		if (it != sSharedWorkers.end())

			sSharedWorkers.erase(it);

	}

	Release();	
}

void VJSWorker::_NotifyParent (XBOX::VError inError, const XBOX::VString &inExplanation)
{
	VJSIsInitializedMessage	*message = new VJSIsInitializedMessage(inError, inExplanation);

	if (message != NULL) {

		// In case we are out of memory, parent will eventually time-out.

		message->PostTo(fRootVTask);
		XBOX::ReleaseRefCountable<VJSIsInitializedMessage>(&message);

	}
}

void VJSWorker::_PostMessage (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *inGlobalObject)
{
	VJSWorker	*worker;

	worker = VJSWorker::RetainWorker(ioParms.GetContext());

	xbox_assert(worker->IsDedicatedWorker() && worker->fOnMessagePort != NULL);
	VJSMessagePort::PostMessageMethod(ioParms, worker->fOnMessagePort);

	XBOX::ReleaseRefCountable<VJSWorker>(&worker);
}

void VJSWorker::_PostWebSocketMessage (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *inGlobalObject)
{
	VJSWorker	*worker;

	worker = VJSWorker::RetainWorker(ioParms.GetContext());

	xbox_assert(worker->IsDedicatedWorker() && worker->fWebSocket != NULL);

	VJSWebSocketObject::Send(ioParms, worker->fWebSocket);

	XBOX::ReleaseRefCountable<VJSWorker>(&worker);
}

void VJSWorker::_ReleaseAllReferences ()
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

//**	XBOX::DebugMsg("release all ref %p\n", this);

	if (fHasReleasedAll)

		return;

	std::list<VRefPtr<VJSWorker> >::iterator	workerIterator;

	for (workerIterator = fSharedWorkers.begin(); workerIterator != fSharedWorkers.end(); workerIterator++) {

		XBOX::StLocker<XBOX::VCriticalSection>		sharedWorkerLock(&(*workerIterator)->fMutex);
		std::list<VRefPtr<VJSWorker> >::iterator	parentWorkerIterator;

		for (parentWorkerIterator = (*workerIterator)->fParentWorkers.begin(); parentWorkerIterator != (*workerIterator)->fParentWorkers.end(); parentWorkerIterator++) 

			if ((VJSWorker *) *parentWorkerIterator == this) {

				(*workerIterator)->fParentWorkers.erase(parentWorkerIterator);
				break;

			}

		// Note that a shared worker may have terminated in the meantime and queued the termination event.

	}
	fSharedWorkers.clear();

	// Discard all events.

	while (!fEventQueue.empty()) {

		fEventQueue.front()->Discard();
		fEventQueue.pop_front();

	}
	
	// Release all error ports, requesting termination of "child" dedicated workers if needed.

	std::list< VRefPtr<VJSMessagePort> >::iterator	i;

	for (i = fErrorPorts.begin(); i != fErrorPorts.end(); i++) {

		// Don't close port yet, this will be done when iterating fMessagePorts.

		if (!(*i)->IsInside(this)) {

			// Current worker is the "outside" of the other worker. If the "inside" worker is
			// a dedicated worker, its parent has terminated, so request termination. This will
			// terminate the "worker tree" of dedicated workers. Shared workers are "free" 
			// running.

			if (!(*i)->IsOtherClosed(this) && (*i)->GetOther(this)->IsDedicatedWorker())

				(*i)->GetOther(this)->Terminate();

		}

	}
	fErrorPorts.clear();

	// For dedicated and shared workers, respectively release onmessage and onconnect ports.

	if (fWorkerType == TYPE_DEDICATED) {
	
//**		DebugMsg("Dedicated released %p\n", this);
		XBOX::ReleaseRefCountable<VJSMessagePort>(&fOnMessagePort);
 
	} else if (fWorkerType == TYPE_SHARED) {

		XBOX::ReleaseRefCountable<VJSMessagePort>(&fConnectionPort);

	} else {

//**		DebugMsg("release all for root %p\n", this);

	}

	// Close all message ports. 
	//**
/*
	for (i = fMessagePorts.begin(); i != fMessagePorts.end(); i++)

		(*i)->Close(this);
*/
	fMessagePorts.clear();

	// Mark all timers as "cleared" (they will be freed).

	fTimerContext.ClearAll();

	if (fWebSocket != NULL) {

		xbox_assert(fWorkerType == TYPE_DEDICATED);
		XBOX::ReleaseRefCountable<VJSWebSocketObject>(&fWebSocket);

	}

	fHasReleasedAll = true;
}

#if VERSIONDEBUG

void VJSWorker::_DumpExecutionStatistics ()
{
	XBOX::VTime		currentTime;
	XBOX::VDuration	totalDuration;
	
	currentTime.FromSystemTime();
	currentTime.Substract(fStartTime, totalDuration);
		
	sLONG8	total;
	Real	totalInSeconds, idlePercentage, executionPercentage;
	
	total = totalDuration.GetLong8();	// In milliseconds;
	totalInSeconds = total / 1000.0;
	if (total) {

		idlePercentage = (fTotalIdleDuration.GetLong8() * 100.0) / total;	
		executionPercentage = 100.0 - idlePercentage;
	
	} else 
		
		idlePercentage = executionPercentage = 0.0;
			
	XBOX::VString	message;
	
	if (fWorkerType == TYPE_ROOT)

		message = "Root worker";

	else if (fWorkerType == TYPE_DEDICATED)

		message = "Dedicated worker";

	else	// TYPE_SHARED

		message = "Shared worker";

	message.AppendString(" total execution time: ");
	message.AppendReal(totalInSeconds);
	message.AppendString(" second(s): execution: ");
	message.AppendReal(executionPercentage);
	message.AppendString("% idle: ");
	message.AppendReal(idlePercentage);

	sMutex.Lock();
	message.AppendString("(");
	message.AppendLong((sLONG) sDedicatedWorkers.size());
	message.AppendString(", ");
	message.AppendLong((sLONG) sSharedWorkers.size());
	message.AppendString(", ");
	message.AppendLong((sLONG) sRootWorkers.size());
	message.AppendString(")\n");
	sMutex.Unlock();
	
	XBOX::DebugMsg(message);
}

#endif

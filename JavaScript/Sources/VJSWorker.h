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
#ifndef __VJS_WORKER__
#define __VJS_WORKER__
 
#include <list>

#include "VJSContext.h"
#include "VJSClass.h"
#include "VJSValue.h"
#include "VJSTimer.h"

#define VJSWORKER_WITH_PROJECT_INFO_RETAIN_JS	0

BEGIN_TOOLBOX_NAMESPACE

class VJSMessagePort;
class IJSEvent;
class IJSEventGenerator;
class VJSLocalFileSystem;
class VJSWebSocketObject;

class XTOOLBOX_API IJSWorkerDelegate
{
public:

	enum WorkerDelegateType {

		TYPE_WAKANDA_SERVER,
		TYPE_WAKANDA_STUDIO,
		TYPE_4D,

	};

	virtual WorkerDelegateType	GetType () = 0;
	virtual	VJSGlobalContext	*RetainJSContext (VError& outError, const VJSContext& inParentContext, bool inReusable) = 0;
	virtual	VError				ReleaseJSContext (VJSGlobalContext* inContext) = 0;

#if VJSWORKER_WITH_PROJECT_INFO_RETAIN_JS

	virtual void				*GetProjectInfo (VError& outError, const VJSContext& inParentContext) = 0;
	virtual	VJSGlobalContext	*RetainJSContext (VError& outError, void *inProjectInfo, bool inReusable) = 0;

#endif
};

class VJSHasEventMessage : public XBOX::VMessage
{
public:

			VJSHasEventMessage (VJSWorker *inWorker);

	void	DoExecute ();

private:
	virtual	~VJSHasEventMessage ();

	VJSWorker	*fWorker;
};

// Worker execution.

class XTOOLBOX_API VJSWorker : public XBOX::VObject, public XBOX::IRefCountable
{
public:

	enum {

		TYPE_ROOT,
		TYPE_DEDICATED,
		TYPE_SHARED

	};

	// The caller is responsible for the destruction of the delegate

	static void					SetDelegate (IJSWorkerDelegate *inDelegate);
	static IJSWorkerDelegate	*GetDelegate ()	{	return sDelegate;	}

	// Create a dedicated worker. 

						VJSWorker (XBOX::VJSContext &inParentContext, 
							XBOX::VJSWorker *inParentWorker, 
							const XBOX::VString &inURL, 
							bool inReUseContext,
							VJSWebSocketObject *inWebSocket);
						
	// If shared worker with given URL and name exists, return a pointer to it. If not, create it.

	static VJSWorker	*RetainSharedWorker (XBOX::VJSContext &inParentContext, XBOX::VJSWorker *inParentWorker,
											const XBOX::VString &inURL, const XBOX::VString &inName, bool inReUseContext, 
											bool *outCreatedWorker);

	// For dedicated workers only, set message port for "onmessage"/postMessage().
 
	void				SetMessagePort (VJSMessagePort *inMessagePort);
	
	// For shared workers only. Connect() will queue an event to trigger the "onconnect" callback, allowing to transfer communication ports.

	void				Connect (VJSMessagePort *inMessagePort, VJSMessagePort *inErrorPort);

	// For shared workers only, queue a WebSocket "onconnect" event.

	void				ConnectWebSocket (XBOX::VWebSocket *inWebSocket);

	// Add an error port, allowing to report errors back to "outside" worker(s).

	void				AddErrorPort (VJSMessagePort *inErrorPort);
	
	// Send an error event to all error port(s), only one (toward parent) for dedicated workers.

	void				BroadcastError (const XBOX::VString &inMessage, const XBOX::VString &inFileNumber, sLONG inLineNumber);
	
	// Launch worker thread. fGlobalContext must have been set properly.
	
	void				Run ();

	// Process events during inWaitingDuration milliseconds. Zero millisecond means "polling": execute all pending events but do not wait. 
	// Use a negative value for no limit in duration. Set inEventGenerator and inEventType to the event generator object and event type to
	// match. Set inEventGenerator to NULL if no matching is needed. 
	// Return 1 if terminated (close flag is set), return 0 if waiting time elapsed, or return -1 if an event has been matched.
	// Worker threads should use a NULL JSContextRef (XBOX::VJSContext((JS4D::ContextRef) NULL)), WaitFor() will then use fGlobalContext 
	// to create the actual context used to process the events.
		
	sLONG				WaitFor (XBOX::VJSContext &inContext, sLONG inWaitingDuration, IJSEventGenerator *inEventGenerator, sLONG inEventType);

	// Set closing flag to true, this will unconditionaly terminate worker. Can be called several times, as long as object exists.

	void				Terminate ();		
	static void			TerminateAll ();	

	// JavaScript context may be re-used. This will "recycle" a working (reset its closing flag).

	static bool			RecycleWorker (const XBOX::VJSContext &inContext);

	// Retrieve worker from context (then get it from global object). If VJSWorker doesn't exist ("root" javascript execution"), 
	// create one so it can receive message from other workers. Hence, RetainWorker() never return NULL.
	
	static VJSWorker	*RetainWorker (const XBOX::VJSContext &inContext);
	
	// Queue a message on worker's message queue.

	void				QueueEvent (IJSEvent *inEvent);

	// A timer can call UnscheduleTimer() on itself during the triggered callback.
	// Because the timer event is removed from queue before triggering it, this will
	// do nothing.
	
	void				UnscheduleTimer (VJSTimer *inTimer);

	// Add or remove a message port.

	void				AddMessagePort (VJSMessagePort *inMessagePort);
	void				RemoveMessagePort (VJSMessagePort *inMessagePort);

	// Accessors.
	
	bool				IsDedicatedWorker () const	{	return fWorkerType == TYPE_DEDICATED;	}

	sLONG				GetWorkerType () const 		{	return fWorkerType;						}

	VJSTimerContext		*GetTimerContext ()			{	return &fTimerContext;					}
	
	bool				IsInsideWaitFor () const	{	return fInsideWaitCount > 0;			}

	VString				GetName() const				{	return fName;							}

	VString				GetURL() const				{	return fURL;							}

	// Global object's methods.

	static void			Close (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *inGlobalObject);
	static void			Wait (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *inGlobalObject);
	static void			ExitWait (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *inGlobalObject);
	
	VJSLocalFileSystem	*RetainLocalFileSystem ();

	// Return wait duration for wait() methods. Negative means infinite duration. 
	// Zero just polling, execute all pending events. Positive, the value to wait 
	// in milliseconds.

	static sLONG		GetWaitDuration (VJSParms_callStaticFunction &ioParms);

	// Return true if the queried shared worker is running.

	static bool			IsSharedWorkerRunning (const XBOX::VString &inURL, const XBOX::VString &inName);

	// Return number of workers running of queried type.

	static sLONG		GetNumberRunning (sLONG inType);

	// This will call wait WaitFor() and execute pending events.

	void				ExecuteHasEventMessage (VJSHasEventMessage *inMessage);

	bool				GetClosingFlag () const	{	return fClosingFlag;	}

	VJSWebSocketObject	*GetWebSocket () const {	xbox_assert(IsDedicatedWorker()); return fWebSocket;	}

private:

	static const uLONG						kSpecificKey	= ('J' << 24) | ('S' << 16) | ('W' << 8) | 'W'; 
	
	static XBOX::VCriticalSection			sMutex;

	static std::list<VJSWorker *>			sDedicatedWorkers;
	static std::list<VJSWorker *>			sSharedWorkers;
	static std::list<VJSWorker *>			sRootWorkers;				// Several "root" workers can run simultaneously.

	static IJSWorkerDelegate				*sDelegate;

	XBOX::VCriticalSection					fMutex;
	XBOX::VCriticalSection					fWaitForMutex;				// Prevent two threads from executing a WaitFor(). 
																		// However a same thread can still call WaitFor() recursively.

	XBOX::VTask								*fVTask;

	XBOX::VTask								*fRootVTask;				// VTask of the root worker.
	XBOX::JS4D::GlobalContextRef			fRootGlobalContext;			// Only for root worker.

	XBOX::VJSGlobalContext					*fGlobalContext;			// "Child" (dedicated or shared) workers only. 
	std::list<VRefPtr<VJSWorker> >			fParentWorkers;				// For dedicated workers, the list contains only one element, the parent.
																		// For shared workers, this is for notification of termination to all who
																		// have instantied a SharedWorker object.
	std::list<VRefPtr<VJSWorker> >			fSharedWorkers;				// Maintain a list of instantiated shared workers.

	bool									fHasReleasedAll, fClosingFlag, fExitWaitFlag;
	XBOX::VSyncEvent						fSyncEvent;
	bool									fIsLockedWaiting;			// True if locked waiting on fSyncEvent.
	std::list<IJSEvent *>					fEventQueue;		
	std::list< VRefPtr<VJSMessagePort> >	fMessagePorts;				// List of all message ports (any type), they may be "duplicated" elsewhere.
	VJSTimerContext							fTimerContext;				// All timers.
	
	sLONG									fWorkerType;				// Root, dedicated, or shared.
	XBOX::VString							fURL, fName;				// URL and name make an unique couple, which identify each shared worker.

	VJSMessagePort							*fOnMessagePort;			// "onmessage" and postMessage() (dedicated workers only).	
	VJSMessagePort							*fConnectionPort;			// "onconnect" (shared workers only).

	std::list< VRefPtr<VJSMessagePort> >	fErrorPorts;				// List of error ports.

	sLONG									fInsideWaitCount;			// Count recursive wait() calls.
	XBOX::VTime								fStartTime;	
	XBOX::VDuration							fTotalIdleDuration;

	VJSLocalFileSystem						*fLocalFileSystem;

	XBOX::VURL								fURLObject;					// Fullpath URL.
	XBOX::VString							fScript;					// Loaded script to execute.

	VJSWebSocketObject						*fWebSocket;				// WebSocket for dedicated worker.

	XBOX::VJSContext						fParentContext;				// To be used only for transfer to the worker thread, unused after it completed its initialization.
	bool									fReUseContext;

#if VJSWORKER_WITH_PROJECT_INFO_RETAIN_JS

	void									*fProjectInfo;

#endif

	// Run() will lock and _RunProc() will unlock it when it has finished initializing.

	// Create a shared worker.

					VJSWorker (XBOX::VJSContext &inParentContext, XBOX::VJSWorker *inParentWorker, 
							const XBOX::VString &inURL, const XBOX::VString &inName, bool inReUseContext);

	// Private constructor, construct an "empty" worker for "root" javascript execution.

					VJSWorker (const XBOX::VJSContext &inContext);
	virtual			~VJSWorker ();

	void			_AddMessagePort (std::list< VRefPtr<VJSMessagePort> > *ioMessagePorts, VJSMessagePort *inMessagePort);
	void			_RemoveMessagePort (std::list< VRefPtr<VJSMessagePort> > *ioMessagePorts, VJSMessagePort *inMessagePort);

	// Add worker attributes and functions to global object.

	void			_PopulateGlobalObject (const XBOX::VJSContext &inContext);	
	
	static void		_SetSpecificDestructor (void *p);

	// Load and check script, return true if successful. 

	bool			_LoadAndCheckScript (XBOX::VError *outError, XBOX::VString *outExplanation);

	// Execute script then service events.

	static sLONG	_RunProc (XBOX::VTask *inVTask);
	void			_DoRun ();
	
	// Notify parent that child has finished (successfully or not) to initialize.

	void			_NotifyParent (XBOX::VError inError, const XBOX::VString &inExplanation);

	// Dedicated workers only, with or without associated WebSocket.
	
	static void		_PostMessage (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *inGlobalObject);
	static void		_PostWebSocketMessage (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *inGlobalObject);

	// Release all references to this worker. That means discarding all events as some event objects
	// may hold references. Closing all message ports and terminating all dedicated workers. Mutex 
	// must have been acquired before calling.
	
	void			_ReleaseAllReferences ();
	
#if VERSIONDEBUG
	
	// Debug information, should make those data available from JS runtime.
	
	void			_DumpExecutionStatistics ();
	
#endif
};

END_TOOLBOX_NAMESPACE

#endif
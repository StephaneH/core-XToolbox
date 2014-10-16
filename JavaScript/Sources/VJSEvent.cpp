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

#include "VJSEvent.h"

#include "VJSContext.h"
#include "VJSGlobalClass.h"

#include "VJSDOMEvent.h"
#include "VJSMessagePort.h"
#include "VJSWorker.h"
#include "VJSStructuredClone.h"
#include "VJSBuffer.h"
#include "VJSEventEmitter.h"
#include "VJSNet.h"
#include "VJSNetServer.h"
#include "VJSNetSocket.h"
#include "VJSW3CFileSystem.h"
#include "VJSSystemWorker.h"
#include "VJSWorkerProxy.h"
#include "VJSWebSocket.h"
#include "VJSRuntime_blob.h"

// Set to 1 to trace SystemWorker termination event.

#define TRACE_SYSTEM_WORKER	0

USING_TOOLBOX_NAMESPACE

bool IJSEvent::InsertEvent (std::list<IJSEvent *> *ioEventQueue, IJSEvent *inEvent)
{
	std::list<IJSEvent *>::iterator	i;
	bool							isFirstPosition;
		
	for (i = ioEventQueue->begin(); i != ioEventQueue->end(); i++)
		
		if (inEvent->fTriggerTime <= (*i)->fTriggerTime)
			
			break;

	isFirstPosition = i == ioEventQueue->begin();
	ioEventQueue->insert(i, inEvent);

	return isFirstPosition;
}

VJSMessageEvent *VJSMessageEvent::Create (VJSMessagePort *inMessagePort, VJSStructuredClone *inMessage)
{
	xbox_assert(inMessagePort != NULL && inMessage != NULL);

	VJSMessageEvent	*messageEvent;

	messageEvent = new VJSMessageEvent();
	messageEvent->fType = eTYPE_MESSAGE; 
	messageEvent->fTriggerTime.FromSystemTime();	// Service as soon as possible.
	messageEvent->fMessagePort = XBOX::RetainRefCountable<VJSMessagePort>(inMessagePort);
	messageEvent->fData = XBOX::RetainRefCountable<VJSStructuredClone>(inMessage);

	return messageEvent;
}

void VJSMessageEvent::Process (XBOX::VJSContext inContext, VJSWorker *inWorker)
{
	xbox_assert(inWorker != NULL);
		
	XBOX::VJSObject		callbackObject(inContext);	

	callbackObject = fMessagePort->GetCallback(inWorker, inContext);
	if (callbackObject.IsFunction()) {

		XBOX::VJSObject				object(inContext);	// Should implement EventTarget interface instead.
		XBOX::VJSValue				data = fData->MakeValue(inContext);
		std::vector<XBOX::VJSValue>	callbackArguments;

		XBOX::VJSObject	messageEventObject	= VJSDOMMessageEventClass::NewInstance(inContext, object, fTriggerTime, data);
					
		callbackArguments.push_back(messageEventObject);

		inContext.GetGlobalObject().CallFunction(callbackObject, &callbackArguments, NULL);

	}

	Discard();
}

void VJSMessageEvent::Discard ()
{
	XBOX::ReleaseRefCountable<VJSMessagePort>(&fMessagePort);
	XBOX::ReleaseRefCountable<VJSStructuredClone>(&fData);
	Release();
}

void VJSMessageEvent::RemoveMessageFrom (std::list<IJSEvent *> *ioEventQueue, VJSMessagePort *inMessagePort)
{
	std::list<IJSEvent *>::iterator	i;

	for (i = ioEventQueue->begin(); i != ioEventQueue->end(); i++)

		if ((*i)->GetType() == eTYPE_MESSAGE && ((VJSMessageEvent *) (*i))->fMessagePort == inMessagePort) {

			(*i)->Discard();
			ioEventQueue->erase(i);
	
		}
}

VJSConnectionEvent *VJSConnectionEvent::Create (VJSMessagePort *inConnectionPort, VJSMessagePort *inMessagePort, VJSMessagePort *inErrorPort)
{
	xbox_assert(inMessagePort != NULL && inConnectionPort != NULL && inErrorPort != NULL);

	VJSConnectionEvent	*connectionEvent;

	connectionEvent = new VJSConnectionEvent();
	connectionEvent->fType = eTYPE_CONNECTION; 
	connectionEvent->fTriggerTime.FromSystemTime();	// Service as soon as possible.
	connectionEvent->fConnectionPort = XBOX::RetainRefCountable<VJSMessagePort>(inConnectionPort);	
	connectionEvent->fMessagePort = XBOX::RetainRefCountable<VJSMessagePort>(inMessagePort);	
	connectionEvent->fErrorPort = XBOX::RetainRefCountable<VJSMessagePort>(inErrorPort);
	
	return connectionEvent;
}

void VJSConnectionEvent::Process (XBOX::VJSContext inContext, VJSWorker *inWorker)
{
	xbox_assert(!inWorker->IsDedicatedWorker());

	XBOX::VJSObject	callbackObject(inContext);

	callbackObject = fConnectionPort->GetCallback(inWorker, inContext);
	if (callbackObject.IsFunction()) { 

		// Connection event is a special MessageEvent with "connect" type (section 4.7.3 of Web Workers specification).

		XBOX::VJSObject	object(inContext);
		XBOX::VJSValue	emptyStringValue(inContext);

		// Should implement EventTarget interface instead.
		emptyStringValue.SetString("");
 
		XBOX::VJSObject	messageEventObject	= VJSDOMMessageEventClass::NewInstance(inContext, object, fTriggerTime, emptyStringValue);
		VJSDOMEvent		*domEvent			= messageEventObject.GetPrivateData<VJSDOMMessageEventClass>();

		domEvent->fType = "connect";
				
		XBOX::VJSArray	portsArrayObject(inContext);
		XBOX::VJSObject	messagePort = VJSMessagePortClass::CreateInstance(inContext, fMessagePort);

		JS4D::ContextRef	globalContextRef;

		globalContextRef = inContext.GetGlobalObjectPrivateInstance()->GetContext();
		VJSContext	vjsContext(globalContextRef);

		fMessagePort->SetObject(inWorker, messagePort);
		fErrorPort->SetObject(inWorker, messagePort);
		messagePort.Protect();
		
		portsArrayObject.PushValue(messagePort);
		messageEventObject.SetProperty("ports", portsArrayObject);
		portsArrayObject.Protect();
		
		std::vector<XBOX::VJSValue>	callbackArguments;
												
		callbackArguments.push_back(messageEventObject);
		inContext.GetGlobalObject().CallFunction(callbackObject, &callbackArguments, NULL);

	} else {

		// Unable to invoke "onconnect" callback of shared worker. Thus it has no way to communicate back 
		// with other ("outside") javascript thread. So close its side of the port.

		fMessagePort->Close(inWorker);

	}			
	
	Discard();
}

void VJSConnectionEvent::Discard ()
{
	XBOX::ReleaseRefCountable<VJSMessagePort>(&fConnectionPort);
	XBOX::ReleaseRefCountable<VJSMessagePort>(&fMessagePort);
	XBOX::ReleaseRefCountable<VJSMessagePort>(&fErrorPort);	
	Release();
}

VJSErrorEvent *VJSErrorEvent::Create (VJSMessagePort *inMessagePort, 
										const XBOX::VString &inMessage, 
										const XBOX::VString &inFileName, 
										sLONG inLineNumber)
{
	xbox_assert(inMessagePort != NULL);

	VJSErrorEvent	*errorEvent;

	errorEvent = new VJSErrorEvent();
	errorEvent->fType = eTYPE_ERROR; 
	errorEvent->fTriggerTime.FromSystemTime();	// Service as soon as possible.
	errorEvent->fMessagePort = XBOX::RetainRefCountable<VJSMessagePort>(inMessagePort);
	errorEvent->fMessage = inMessage;
	errorEvent->fFileName = inFileName;
	errorEvent->fLineNumber = inLineNumber;

	return errorEvent;
}


void VJSErrorEvent::Process (XBOX::VJSContext inContext, VJSWorker *inWorker)
{
	xbox_assert(inWorker != NULL);
		
	XBOX::VJSObject	callbackObject(inContext);
	
	callbackObject = fMessagePort->GetCallback(inWorker, inContext);
	if (callbackObject.IsFunction()) {

		XBOX::VJSObject	object(inContext);

		// Should implement EventTarget interface instead.

		XBOX::VJSObject	errorEventObject	= VJSDOMErrorEventClass::NewInstance(inContext, object, fTriggerTime, fMessage, fFileName, fLineNumber);
		
		std::vector<XBOX::VJSValue>	callbackArguments;		

		callbackArguments.push_back(errorEventObject);

		inContext.GetGlobalObject().CallFunction(callbackObject, &callbackArguments, NULL);

	} else {

		// No callback to handle error: If inWorker is a dedicated worker, propagate to error its parent.
		// Section 4.6 "Runtime Script Errors" of web workers specification.

		if (inWorker->IsDedicatedWorker())

			inWorker->BroadcastError(fMessage, fFileName, fLineNumber);

	}

	Discard();
}

void VJSErrorEvent::Discard ()
{
	XBOX::ReleaseRefCountable<VJSMessagePort>(&fMessagePort);
	Release();
}

VJSTerminationEvent	*VJSTerminationEvent::Create (VJSWorker *inWorker)
{
	xbox_assert(inWorker != NULL);
	xbox_assert(inWorker->GetWorkerType() == VJSWorker::TYPE_DEDICATED || inWorker->GetWorkerType() == VJSWorker::TYPE_SHARED);

	VJSTerminationEvent	*terminationEvent;

	terminationEvent = new VJSTerminationEvent();
	terminationEvent->fType = eTYPE_TERMINATION; 
	terminationEvent->fTriggerTime.FromSystemTime();

//**	DebugMsg("create termination event for %p\n", inWorker);

	terminationEvent->fWorker = XBOX::RetainRefCountable<VJSWorker>(inWorker);
	
	return terminationEvent;
}

void VJSTerminationEvent::Process (XBOX::VJSContext inContext, VJSWorker *inWorker)
{
	xbox_assert(inWorker != NULL);

	bool			isDedicated;
	XBOX::VString	constructorName;

	if (fWorker->GetWorkerType() == VJSWorker::TYPE_DEDICATED) {

		isDedicated = true;
		constructorName = "Worker";

	} else {

		isDedicated = false;
		constructorName =  "SharedWorker";

	}
	
	XBOX::VJSValue	value(inContext);
	
	if (inContext.GetGlobalObject().HasProperty(constructorName)) {

		XBOX::VJSObject	workerObject	= inContext.GetGlobalObject().GetPropertyAsObject(constructorName);

		if (workerObject.IsObject() && workerObject.HasProperty("list")) {

			value = workerObject.GetProperty("list");
			
		} else {

			xbox_assert(false);

		}
		
	} else {

		xbox_assert(false);

	}

	if (value.IsArray()) {

		XBOX::VJSArray	listArray(value);
		JS4D::ClassRef	classRef;
		sLONG			i;

		classRef = isDedicated ? VJSDedicatedWorkerClass::Class() : VJSSharedWorkerClass::Class();
		for (i = 0; i < listArray.GetLength(); i++) {

			value = listArray.GetValueAt(i);
			if (value.IsObject()) {

				XBOX::VJSObject	object = value.GetObject((VJSException*)NULL);

				if (object.IsOfClass(classRef)) {

					VJSWebWorkerObject	*workerProxy;

					if (isDedicated) 

						workerProxy = object.GetPrivateData<VJSDedicatedWorkerClass>();

					else

						workerProxy = object.GetPrivateData<VJSSharedWorkerClass>();

					if (workerProxy->GetWorker() == fWorker) {

						value.SetNull();
						listArray.SetValueAt(i, value);	// Set worker proxy reference to null to allow garbage collection.
						break;

					}

				}

			}

		}

		xbox_assert(i != listArray.GetLength());	// Not found!

	} else {

		xbox_assert(false);

	}

	Discard();
}

void VJSTerminationEvent::Discard ()
{
	xbox_assert(fWorker != NULL);

//**	DebugMsg("discard termination event for %p\n", fWorker);
	XBOX::ReleaseRefCountable<VJSWorker>(&fWorker);
}

VJSTimerEvent *VJSTimerEvent::Create (VJSTimer *inTimer, XBOX::VTime &inTriggerTime, std::vector<XBOX::VJSValue> *inArguments)
{
	xbox_assert(inTimer != NULL && !inTimer->IsCleared() && inArguments != NULL);

	VJSTimerEvent	*timerEvent;

	timerEvent = new VJSTimerEvent();
	timerEvent->fType = eTYPE_TIMER;
	timerEvent->fTriggerTime = inTriggerTime;
	timerEvent->fTimer = inTimer;
	timerEvent->fArguments = inArguments;

	return timerEvent;
}

void VJSTimerEvent::Process (XBOX::VJSContext inContext, VJSWorker *inWorker)
{
	xbox_assert(!fTimer->IsCleared());

	xbox_assert(fTimer->fFunctionObject.IsFunction());

  	inContext.GetGlobalObject().CallFunction(fTimer->fFunctionObject, fArguments,NULL);

	if (fTimer->IsInterval()) {
		
		if (!fTimer->IsCleared()) {

			// If clearInterval() hasn't been called, re-schedule.
			// Note that same VJSTimerEvent object is re-used, no call to Discard().

			fTriggerTime.AddMilliseconds(fTimer->fInterval);
			inWorker->QueueEvent(this);

		} else

			Discard();

	} else {

		fTimer->_Clear();
		Discard();

	}
}

void VJSTimerEvent::Discard ()
{
	fTimer->_ReleaseIfCleared();
	delete fArguments;
	Release();
}

void VJSTimerEvent::RemoveTimerEvent (std::list<IJSEvent *> *ioEventQueue, VJSTimer *inTimer)
{
	std::list<IJSEvent *>::iterator	i;

	for (i = ioEventQueue->begin(); i != ioEventQueue->end(); i++)

		if ((*i)->GetType() == eTYPE_TIMER 
		&& ((VJSTimerEvent *) (*i))->fTimer == inTimer) {

			(*i)->Discard();
			ioEventQueue->erase(i);
			break;

		}
}

VJSSystemWorkerEvent *VJSSystemWorkerEvent::Create (VJSSystemWorker *inSystemWorker, sLONG inType, XBOX::VJSObject &inObjectRef, uBYTE *inData, sLONG inSize)
{
	xbox_assert(inSystemWorker != NULL);
	xbox_assert(inType == eTYPE_STDOUT_DATA || inType == eTYPE_STDERR_DATA || inType == eTYPE_TERMINATION);
	xbox_assert(inObjectRef.HasRef() && inData != NULL && inSize > 0);
			
	VJSSystemWorkerEvent	*systemWorkerEvent;

	systemWorkerEvent = new VJSSystemWorkerEvent(inObjectRef);
	systemWorkerEvent->fSystemWorker = XBOX::RetainRefCountable<VJSSystemWorker>(inSystemWorker);
	systemWorkerEvent->fType = eTYPE_SYSTEM_WORKER;
	systemWorkerEvent->fTriggerTime.FromSystemTime();

	systemWorkerEvent->fSubType = inType;
	//systemWorkerEvent->fObject = inObjectRef;
	systemWorkerEvent->fData = inData;
	systemWorkerEvent->fSize = inSize;
		
	return systemWorkerEvent;
}

void VJSSystemWorkerEvent::Process (XBOX::VJSContext inContext, VJSWorker *inWorker)
{
	VJSObject	proxyObject = fObject;
	proxyObject.SetContext(inContext);
	XBOX::VJSObject	callbackObject(inContext);
	XBOX::VJSObject	argumentObject(inContext);

	argumentObject.MakeEmpty();
	switch (fSubType) {

		case eTYPE_STDOUT_DATA: {
	
			callbackObject = proxyObject.GetPropertyAsObject("onmessage");
			if (callbackObject.IsFunction()) {
			
				argumentObject.MakeEmpty();
				argumentObject.SetProperty("type", CVSTR( "message"));
				argumentObject.SetProperty("target", proxyObject);
				if (proxyObject.GetPropertyAsBool("isbinary", NULL, NULL)) 

					argumentObject.SetProperty("data", VJSBufferClass::NewInstance(inContext, fSize, fData));

				else {

					XBOX::VString	string((const char *) fData);
				
					argumentObject.SetProperty("data", string);
					::free(fData);					

				}

			} else

				::free(fData);

			// fData has been freed already or is to be freed by Buffer when done, 
			// so set fData to NULL to prevent Discard() from freeing it.

			fData = NULL;
			break;

		}

		case eTYPE_STDERR_DATA: {

			// See comment for eTYPE_STDOUT_DATA

			callbackObject = proxyObject.GetPropertyAsObject("onerror");
			if (callbackObject.IsFunction()) {

				argumentObject.MakeEmpty();
				argumentObject.SetProperty("type", CVSTR( "error"));
				argumentObject.SetProperty("target", proxyObject);
				if (proxyObject.GetPropertyAsBool("isbinary", NULL, NULL))

					argumentObject.SetProperty("data", VJSBufferClass::NewInstance(inContext, fSize, fData));
	
				else {

					XBOX::VString	string((const char *) fData);
				
					argumentObject.SetProperty("data", string);
					::free(fData);					

				}

			} else

				::free(fData);

			fData = NULL;
			break;

		}
		
		case eTYPE_TERMINATION: {

//** Because object is protected in constructor, it must be unprotected.
//** Hence termination must be processed, otherwise it will never be garbage collected.
			proxyObject.Unprotect();

#if TRACE_SYSTEM_WORKER

			XBOX::DebugMsg("Process termination event of SysWorker %p.\n", fSystemWorker);

#endif

			callbackObject = proxyObject.GetPropertyAsObject("onterminated");
			if (callbackObject.IsFunction()) {

				STerminationData	*data;

				data = (STerminationData *) fData;

				argumentObject.MakeEmpty();
				argumentObject.SetProperty("type", CVSTR( "terminate"));
				argumentObject.SetProperty("target", proxyObject);
				argumentObject.SetProperty("hasStarted", data->fHasStarted);
				argumentObject.SetProperty("forced", data->fForcedTermination);
				argumentObject.SetProperty("exitStatus", (Real) data->fExitStatus);
				argumentObject.SetProperty("pid", data->fPID);	

			}

			delete fData;
			fData = NULL;

			break;		
		
		}

	}

	if (callbackObject.IsFunction()) {

#if TRACE_SYSTEM_WORKER

		if (fSubType == eTYPE_TERMINATION) 

			XBOX::DebugMsg("Calling termination callback of SysWorker %p.\n", fSystemWorker);

#endif

		std::vector<XBOX::VJSValue>	callbackArguments;

		callbackArguments.push_back(argumentObject);	
		inContext.GetGlobalObject().CallFunction(callbackObject, &callbackArguments, NULL);

	}		

	Discard();
}

void VJSSystemWorkerEvent::Discard ()
{
	if (fSubType == eTYPE_TERMINATION) {

		XBOX::StLocker<XBOX::VCriticalSection>	lock(&fSystemWorker->fCriticalSection);

		fSystemWorker->fTerminationEventDiscarded = true;

#if TRACE_SYSTEM_WORKER

		XBOX::DebugMsg("Discard termination event of SysWorker %p.\n", fSystemWorker);

#endif

	}

	XBOX::ReleaseRefCountable<VJSSystemWorker>(&fSystemWorker);

	// If Process() hasn't be called, free fData.

	if (fData != NULL) {

		if (fSubType == eTYPE_TERMINATION) 

			delete fData;

		else

			::free(fData);

		fData = NULL;

	}	
	Release();
}

VJSNetEvent *VJSNetEvent::Create (VJSEventEmitter *inEventEmitter, const XBOX::VString &inEventName)
{
	xbox_assert(inEventEmitter != NULL);
		
	VJSNetEvent	*netEvent;

	netEvent = new VJSNetEvent();
	netEvent->fType = eTYPE_NET;
	netEvent->fTriggerTime.FromSystemTime();

	netEvent->fSubType = eTYPE_NO_ARGUMENT;
	netEvent->fEventEmitter = XBOX::RetainRefCountable<VJSEventEmitter>(inEventEmitter);
	netEvent->fEventName = inEventName;
	
	return netEvent;
}

VJSNetEvent *VJSNetEvent::CreateData (VJSNetSocketObject *inSocketObject, uBYTE *inData, sLONG inSize)
{
	xbox_assert(inSocketObject != NULL);
	xbox_assert(inSize >= 0);
	xbox_assert(!(inSize > 0 && inData == NULL));
	
	VJSNetEvent	*netEvent;

	netEvent = new VJSNetEvent();
	netEvent->fType = eTYPE_NET;
	netEvent->fTriggerTime.FromSystemTime();

	netEvent->fSubType = eTYPE_DATA;
	netEvent->fEventEmitter = XBOX::RetainRefCountable<VJSNetSocketObject>(inSocketObject);
	netEvent->fData = inData;
	netEvent->fSize = inSize;
	
	return netEvent;
}

VJSNetEvent *VJSNetEvent::CreateError (VJSEventEmitter *inEventEmitter, const XBOX::VString &inExceptionName)
{
	xbox_assert(inEventEmitter != NULL);
			
	VJSNetEvent	*netEvent;

	netEvent = new VJSNetEvent();
	netEvent->fType = eTYPE_NET;
	netEvent->fTriggerTime.FromSystemTime();

	netEvent->fSubType = eTYPE_ERROR;
	netEvent->fEventEmitter = XBOX::RetainRefCountable<VJSEventEmitter>(inEventEmitter);
	netEvent->fExceptionName = inExceptionName;
		
	return netEvent;
}

VJSNetEvent *VJSNetEvent::CreateClose (VJSEventEmitter *inEventEmitter, bool inHadError)
{
	xbox_assert(inEventEmitter != NULL);
		
	VJSNetEvent	*netEvent;

	netEvent = new VJSNetEvent();
	netEvent->fType = eTYPE_NET;
	netEvent->fTriggerTime.FromSystemTime();

	netEvent->fSubType = eTYPE_CLOSE;
	netEvent->fEventEmitter = XBOX::RetainRefCountable<VJSEventEmitter>(inEventEmitter);
	netEvent->fHadError = inHadError;
		
	return netEvent;
}

VJSNetEvent	*VJSNetEvent::CreateConnection (VJSNetServerObject *inServerObject, XBOX::VTCPEndPoint *inEndPoint, bool inIsSSL)
{
	xbox_assert(inServerObject != NULL && inEndPoint != NULL);
	
	VJSNetEvent	*netEvent;

	netEvent = new VJSNetEvent();
	netEvent->fType = eTYPE_NET;
	netEvent->fTriggerTime.FromSystemTime();

	netEvent->fSubType = inIsSSL ? eTYPE_CONNECTION_SSL : eTYPE_CONNECTION;
	netEvent->fEventEmitter = XBOX::RetainRefCountable<VJSNetServerObject>(inServerObject);
	RetainRefCountable ( inEndPoint );
	netEvent->fEndPoint = inEndPoint;
	
	return netEvent;
}

void VJSNetEvent::Process (XBOX::VJSContext inContext, VJSWorker *inWorker)
{
	xbox_assert(inWorker != NULL);

	XBOX::VString	callbackName;
	bool			isOk;

	isOk = true;
	switch (fSubType) {

		case eTYPE_NO_ARGUMENT:		callbackName = fEventName; break;
		case eTYPE_DATA:			callbackName = "data"; break;
		case eTYPE_ERROR:			callbackName = "error"; break;
		case eTYPE_CLOSE:			callbackName = "close"; break;
		case eTYPE_CONNECTION:		callbackName = "connection"; break;
		case eTYPE_CONNECTION_SSL:	callbackName = "secureConnection"; break;
		default:					xbox_assert(false); break;
		
	}

	std::vector<XBOX::VJSValue>	callbackArguments;

	if (fSubType == eTYPE_DATA) {

		// If encoding is XBOX::VTC_UNKNOWN (default when Socket object is constructed), then data is 
		// considered as binary and no string conversion takes place, returning a Buffer object.

		CharSet	encoding;

		if ((encoding = ((VJSNetSocketObject *) fEventEmitter)->GetEncoding()) == XBOX::VTC_UNKNOWN) {

			callbackArguments.push_back(VJSBufferClass::NewInstance(inContext, fSize, fData));
	
			// This will prevent Discard() from freeing memory.

			fData = NULL;

		} else {

			VJSBufferObject	*buffer;
			VJSValue		value(inContext);
						
			if ((buffer = new VJSBufferObject(fSize, fData)) == NULL) {

				value.SetString("");			
				XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

				// Discard() will free fData.

			} else {

				XBOX::VString	decodedString;

				// Ignore conversion failure.

				buffer->ToString(encoding, 0, fSize, &decodedString);
				value.SetString(decodedString);

				// Buffer object destructor will free fData, Discard() doesn't have to do it.

				ReleaseRefCountable(&buffer);
				fData = NULL;

			}

			callbackArguments.push_back(value);

		}

	} else if (fSubType == eTYPE_ERROR) {

		// Should do a "new fExceptionName()" instead of the current null object.
		// Currently, just an error string.

		XBOX::VJSValue	value(inContext);

		value.SetString(fExceptionName);
		callbackArguments.push_back(value);

	} else if (fSubType == eTYPE_CLOSE) {

		XBOX::VJSValue	value(inContext);

		value.SetBool(fHadError);
		callbackArguments.push_back(value);

	} else if (fSubType == eTYPE_CONNECTION || fSubType == eTYPE_CONNECTION_SSL) {

		XBOX::VJSValue	value(inContext);
		
		VJSNetSocketClass::NewInstance(inContext, fEventEmitter, fEndPoint, &value);
		
		// Variable value is not checked for null!

		callbackArguments.push_back(value);

	}

	if (isOk)

		fEventEmitter->Emit(inContext, callbackName, &callbackArguments);

	Discard();
}

void VJSNetEvent::Discard ()
{
	if (fSubType == eTYPE_DATA && fData != NULL)

		::free(fData);

	else if (fSubType == eTYPE_CONNECTION || fSubType == eTYPE_CONNECTION_SSL) {

		xbox_assert(fEndPoint != NULL);

		// If the "connection" or "secureConnection" event has no callback, this will be the "last" release.
		// VTCPEndPoint's destructor will called and socket will be closed.

		ReleaseRefCountable(&fEndPoint);

	}

	XBOX::ReleaseRefCountable<VJSEventEmitter>(&fEventEmitter);

	Release();
}

VJSCallbackEvent *VJSCallbackEvent::Create (XBOX::VJSObject &inObject, const XBOX::VString &inCallbackName, std::vector<XBOX::VJSValue> *inArguments)
{
	xbox_assert(inObject.HasRef() && inArguments != NULL);
	
	VJSCallbackEvent	*callbackEvent;

	callbackEvent = new VJSCallbackEvent(inObject);
	callbackEvent->fType = eTYPE_CALLBACK;
	callbackEvent->fTriggerTime.FromSystemTime();

	//callbackEvent->fObject = inObject;
	callbackEvent->fCallbackName = inCallbackName;
	callbackEvent->fArguments = inArguments;
	
	return callbackEvent;
}
												
void VJSCallbackEvent::Process (XBOX::VJSContext inContext, VJSWorker *inWorker)
{	
	xbox_assert(inWorker != NULL);

	XBOX::VJSObject	callbackObject	= fObject.GetPropertyAsObject(fCallbackName);

	if (callbackObject.IsFunction())
	
		inContext.GetGlobalObject().CallFunction(callbackObject, fArguments, NULL);

	Discard();
}

void VJSCallbackEvent::Discard ()
{
	delete fArguments;
	Release();
}

VJSW3CFSEvent *VJSW3CFSEvent::RequestFS (VJSLocalFileSystem *inLocalFileSystem, 
	sLONG inType, VSize inQuota, const XBOX::VString &inFileSystemName, 
	const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	xbox_assert(inLocalFileSystem != NULL);
	
	VJSW3CFSEvent	*fsEvent = new VJSW3CFSEvent(eOPERATION_REQUEST_FILE_SYSTEM, inSuccessCallback, inErrorCallback);
	
	if (fsEvent != NULL) {

		fsEvent->fLocalFileSystem = XBOX::RetainRefCountable<VJSLocalFileSystem>(inLocalFileSystem);
		fsEvent->fType = inType;
		fsEvent->fQuota = inQuota;
		fsEvent->fFileSystemName = inFileSystemName;

	}
	
	return fsEvent;
}

VJSW3CFSEvent *VJSW3CFSEvent::ResolveURL (VJSLocalFileSystem *inLocalFileSystem, const XBOX::VString &inURL, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	xbox_assert(inLocalFileSystem != NULL);
	
	VJSW3CFSEvent	*fsEvent = new VJSW3CFSEvent(eOPERATION_RESOLVE_URL, inSuccessCallback, inErrorCallback);
	
	if (fsEvent)
	{
		fsEvent->fLocalFileSystem = RetainRefCountable(inLocalFileSystem);
		fsEvent->fURL = inURL;
	}

	return fsEvent;
}

VJSW3CFSEvent *VJSW3CFSEvent::GetMetadata (VJSEntry *inEntry, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	xbox_assert(inEntry != NULL);
	
	VJSW3CFSEvent	*fsEvent = new VJSW3CFSEvent(eOPERATION_RESOLVE_URL, inSuccessCallback, inErrorCallback);
	
	if (fsEvent != NULL)
		fsEvent->fEntry = RetainRefCountable(inEntry);
	
	return fsEvent;	
}

VJSW3CFSEvent *VJSW3CFSEvent::MoveTo (VJSEntry *inEntry, VJSEntry *inTargetEntry, const XBOX::VString &inNewName, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	xbox_assert(inEntry != NULL && inTargetEntry != NULL && !inTargetEntry->IsFile());
	
	VJSW3CFSEvent	*fsEvent = new VJSW3CFSEvent(eOPERATION_MOVE_TO, inSuccessCallback, inErrorCallback);
	
	if (fsEvent != NULL)
	{
		fsEvent->fEntry = RetainRefCountable(inEntry);
		fsEvent->fTargetEntry = RetainRefCountable(inTargetEntry);
		fsEvent->fURL = inNewName;
	}
	
	return fsEvent;
}

VJSW3CFSEvent *VJSW3CFSEvent::CopyTo (VJSEntry *inEntry, VJSEntry *inTargetEntry, const XBOX::VString &inNewName, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	xbox_assert(inEntry != NULL && inTargetEntry != NULL && !inTargetEntry->IsFile());
	
	VJSW3CFSEvent	*fsEvent = new VJSW3CFSEvent(eOPERATION_COPY_TO, inSuccessCallback, inErrorCallback);
	
	if (fsEvent != NULL)
	{
		fsEvent->fEntry = XBOX::RetainRefCountable<VJSEntry>(inEntry);
		fsEvent->fTargetEntry = XBOX::RetainRefCountable<VJSEntry>(inTargetEntry);
		fsEvent->fURL = inNewName;
	}
	
	return fsEvent;
}

VJSW3CFSEvent *VJSW3CFSEvent::Remove (VJSEntry *inEntry, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	xbox_assert(inEntry != NULL);
	
	VJSW3CFSEvent	*fsEvent = new VJSW3CFSEvent(eOPERATION_REMOVE, inSuccessCallback, inErrorCallback);
	
	if (fsEvent != NULL)
		fsEvent->fEntry = RetainRefCountable(inEntry);
	
	return fsEvent;	
}


VJSW3CFSEvent *VJSW3CFSEvent::GetParent (VJSEntry *inEntry, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	xbox_assert(inEntry != NULL);
	
	VJSW3CFSEvent	*fsEvent = new VJSW3CFSEvent(eOPERATION_GET_PARENT, inSuccessCallback, inErrorCallback);
	
	if (fsEvent != NULL)
		fsEvent->fEntry = RetainRefCountable(inEntry);
	
	return fsEvent;	
}

VJSW3CFSEvent *VJSW3CFSEvent::GetFile (VJSEntry *inEntry, const XBOX::VString &inURL, sLONG inFlags, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	xbox_assert(inEntry != NULL && !inEntry->IsFile());
	
	VJSW3CFSEvent	*fsEvent;
	
	if ((fsEvent = new VJSW3CFSEvent(eOPERATION_GET_FILE, inSuccessCallback, inErrorCallback)) != NULL) {

		fsEvent->fEntry = XBOX::RetainRefCountable<VJSEntry>(inEntry);
		fsEvent->fURL = inURL;
		fsEvent->fFlags = inFlags;

	}

	return fsEvent;	
}	

VJSW3CFSEvent *VJSW3CFSEvent::GetDirectory (VJSEntry *inEntry, const XBOX::VString &inURL, sLONG inFlags, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	xbox_assert(inEntry != NULL && !inEntry->IsFile());
	
	VJSW3CFSEvent	*fsEvent;
	
	if ((fsEvent = new VJSW3CFSEvent(eOPERATION_GET_DIRECTORY, inSuccessCallback, inErrorCallback)) != NULL) {

		fsEvent->fEntry = XBOX::RetainRefCountable<VJSEntry>(inEntry);
		fsEvent->fURL = inURL;
		fsEvent->fFlags = inFlags;

	}

	return fsEvent;	
}	

VJSW3CFSEvent *VJSW3CFSEvent::RemoveRecursively (VJSEntry *inEntry, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	xbox_assert(inEntry != NULL && !inEntry->IsFile());
	
	VJSW3CFSEvent	*fsEvent = new VJSW3CFSEvent(eOPERATION_REMOVE_RECURSIVELY, inSuccessCallback, inErrorCallback);
	
	if (fsEvent != NULL)
		fsEvent->fEntry = RetainRefCountable(inEntry);
	
	return fsEvent;	
}

VJSW3CFSEvent *VJSW3CFSEvent::Folder (VJSEntry *inEntry, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	xbox_assert(inEntry != NULL && !inEntry->IsFile());
	
	VJSW3CFSEvent	*fsEvent;
	
	if ((fsEvent = new VJSW3CFSEvent(eOPERATION_FOLDER, inSuccessCallback, inErrorCallback)) != NULL)

		fsEvent->fEntry = XBOX::RetainRefCountable<VJSEntry>(inEntry);
	
	return fsEvent;	
}

VJSW3CFSEvent *VJSW3CFSEvent::CreateWriter (VJSEntry *inEntry, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	xbox_assert(inEntry != NULL && inEntry->IsFile());
	
	VJSW3CFSEvent	*fsEvent = new VJSW3CFSEvent(eOPERATION_CREATE_WRITER, inSuccessCallback, inErrorCallback);
	
	if (fsEvent != NULL)
		fsEvent->fEntry = RetainRefCountable(inEntry);
	
	return fsEvent;	
}

VJSW3CFSEvent *VJSW3CFSEvent::File (VJSEntry *inEntry, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	xbox_assert(inEntry != NULL && inEntry->IsFile());
	
	VJSW3CFSEvent	*fsEvent = new VJSW3CFSEvent(eOPERATION_FILE, inSuccessCallback, inErrorCallback);
	
	if (fsEvent != NULL)
		fsEvent->fEntry = RetainRefCountable(inEntry);
	
	return fsEvent;	
}

VJSW3CFSEvent *VJSW3CFSEvent::ReadEntries (VJSDirectoryReader *inDirectoryReader, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	xbox_assert(inDirectoryReader != NULL && !inDirectoryReader->IsReading());

	VJSW3CFSEvent	*fsEvent = new VJSW3CFSEvent(eOPERATION_READ_ENTRIES, inSuccessCallback, inErrorCallback);
	
	if (fsEvent != NULL)
		fsEvent->fDirectoryReader= RetainRefCountable(inDirectoryReader);
	
	return fsEvent;	
}

void VJSW3CFSEvent::Process ( XBOX::VJSContext inContext, VJSWorker *inWorker)
{
	xbox_assert(inWorker != NULL);
	
	// no error propagation from event handler
	StErrorContextInstaller errorContext( false, false);
	
	bool needDiscard = true;
	VJSObject	resultObject(inContext);	

	switch (fSubType)
	{
		case eOPERATION_REQUEST_FILE_SYSTEM:	fLocalFileSystem->RequestFileSystem(inContext, fType, fQuota, resultObject, false, fFileSystemName); break;
		case eOPERATION_RESOLVE_URL:			fLocalFileSystem->ResolveURL(inContext, fURL, false, resultObject); break;
		case eOPERATION_GET_FILE:				fEntry->GetFile(inContext, fURL, fFlags, resultObject); break;
		case eOPERATION_GET_DIRECTORY:			fEntry->GetDirectory(inContext, fURL, fFlags, resultObject); break;
		case eOPERATION_GET_METADATA:			fEntry->GetMetadata(inContext, resultObject); break;
		case eOPERATION_MOVE_TO:				fEntry->MoveTo(inContext, fTargetEntry, fURL, resultObject); break;
		case eOPERATION_COPY_TO:				fEntry->CopyTo(inContext, fTargetEntry, fURL, resultObject); break;
		case eOPERATION_REMOVE:					fEntry->Remove(inContext); break;
		case eOPERATION_GET_PARENT:				fEntry->GetParent(inContext, resultObject); break;
		case eOPERATION_REMOVE_RECURSIVELY:		fEntry->RemoveRecursively(inContext); break;
		case eOPERATION_FOLDER:					fEntry->Folder(inContext, resultObject); break;
		case eOPERATION_CREATE_WRITER:			fEntry->CreateWriter(inContext, resultObject); break;
		case eOPERATION_FILE:					fEntry->File(inContext, resultObject); break;

		case eOPERATION_READ_ENTRIES:
			{
				if (fDirectoryReader->ReadEntries(inContext, resultObject))
				{
					xbox_assert(resultObject.IsArray());
					XBOX::VJSArray	entries(resultObject, false);	// No check, already an assert before.

					// If array is empty, there are no more entries to read. If not, reschedule event.
					if (entries.GetLength())
					{
						inWorker->QueueEvent(this);
						needDiscard = false;
					}
				}
				break;

		}
	}
	
	if (errorContext.GetLastError() != VE_OK)
	{
		if (fErrorCallback.HasRef())
		{
			VJSException	exception;
			JS4D::ConvertErrorContextToException( inContext, errorContext.GetContext(), exception.GetPtr() );
			VJSContext	vjsContext(inContext);
			VJSFunction callbackObject( fErrorCallback, inContext);
			callbackObject.AddParam( VJSValue( vjsContext, exception ));
			callbackObject.Call();
		}
	}
	else
	{
		if (fSuccessCallback.HasRef())
		{
			VJSFunction callbackObject( fSuccessCallback, inContext);
			callbackObject.AddParam( resultObject);
			callbackObject.Call();
		}
	}

	if (needDiscard)
		Discard();
}


void VJSW3CFSEvent::Discard ()
{
	if (fSubType == eOPERATION_REQUEST_FILE_SYSTEM || fSubType == eOPERATION_RESOLVE_URL) {
	
		xbox_assert(fLocalFileSystem != NULL);
		
		XBOX::ReleaseRefCountable<VJSLocalFileSystem>(&fLocalFileSystem);
	
	} else if (fSubType == eOPERATION_READ_ENTRIES) {

		xbox_assert(fDirectoryReader != NULL);

		XBOX::ReleaseRefCountable<VJSDirectoryReader>(&fDirectoryReader);

	} else {
	
		xbox_assert(fEntry != NULL);

		XBOX::ReleaseRefCountable<VJSEntry>(&fEntry);		
		if (fSubType == eOPERATION_MOVE_TO || fSubType == eOPERATION_COPY_TO) {
			
			xbox_assert(fTargetEntry != NULL);
			
			XBOX::ReleaseRefCountable<VJSEntry>(&fTargetEntry);
			
		}		

	}
	Release();
}

VJSW3CFSEvent::VJSW3CFSEvent (sLONG inSubType, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
:fSuccessCallback(inSuccessCallback)
,fErrorCallback(inErrorCallback)
{
	fType = eTYPE_W3C_FS;
	fTriggerTime.FromSystemTime();	
	
	fSubType = inSubType;	
}

VJSNewListenerEvent	*VJSNewListenerEvent::Create (VJSEventEmitter *inEventEmitter, const XBOX::VString &inEvent, XBOX::VJSObject& inListener)
{
	xbox_assert(inEventEmitter != NULL);

	VJSNewListenerEvent	*newListenerEvent;

	newListenerEvent = new VJSNewListenerEvent(inListener);
	newListenerEvent->fType = eTYPE_EVENT_EMITTER;
	newListenerEvent->fTriggerTime.FromSystemTime();

	newListenerEvent->fEventEmitter = XBOX::RetainRefCountable<VJSEventEmitter>(inEventEmitter);
	newListenerEvent->fEvent = inEvent;
	//newListenerEvent->fListener = inListener;
	
	return newListenerEvent;
}

void VJSNewListenerEvent::Process (XBOX::VJSContext inContext, VJSWorker *inWorker)
{
	xbox_assert(fEventEmitter != NULL);

	std::vector<XBOX::VJSValue>	callbackArguments;
	VJSValue					value(inContext);

	value.SetString(fEvent);
	callbackArguments.push_back(value);

	if (fListener.HasRef()) {

		// No check that the given listener is actually a function.

		value = fListener;
		callbackArguments.push_back(value);

	}

	fEventEmitter->Emit(inContext, "newListener", &callbackArguments);

	Discard();
}

void VJSNewListenerEvent::Discard ()
{
	xbox_assert(fEventEmitter != NULL);

	XBOX::ReleaseRefCountable<VJSEventEmitter>(&fEventEmitter);

	Release();
}

VJSWebSocketEvent *VJSWebSocketEvent::CreateOpenEvent (VJSWebSocketObject *inWebSocket, const XBOX::VString &inProtocol, const XBOX::VString &inExtensions)
{
	xbox_assert(inWebSocket != NULL);

	VJSWebSocketEvent	*webSocketEvent;

	webSocketEvent = new VJSWebSocketEvent();
	webSocketEvent->fType = eTYPE_WEB_SOCKET;
	webSocketEvent->fTriggerTime.FromSystemTime();

	webSocketEvent->fSubType = eTYPE_OPEN;
	webSocketEvent->fWebSocket = XBOX::RetainRefCountable<VJSWebSocketObject>(inWebSocket);

	webSocketEvent->fProtocol = inProtocol;
	webSocketEvent->fExtensions = inExtensions;
	
	return webSocketEvent;
}

VJSWebSocketEvent *VJSWebSocketEvent::CreateMessageEvent (VJSWebSocketObject *inWebSocket, bool inIsText, uBYTE *inData, VSize inSize)
{
	xbox_assert(inWebSocket != NULL && inData != NULL && inSize > 0);

	VJSWebSocketEvent	*webSocketEvent;

	webSocketEvent = new VJSWebSocketEvent();
	webSocketEvent->fType = eTYPE_WEB_SOCKET;
	webSocketEvent->fTriggerTime.FromSystemTime();

	webSocketEvent->fSubType = eTYPE_MESSAGE;
	webSocketEvent->fWebSocket = XBOX::RetainRefCountable<VJSWebSocketObject>(inWebSocket);
	webSocketEvent->fIsText = inIsText;
	webSocketEvent->fData = inData;
	webSocketEvent->fSize = inSize;
	
	return webSocketEvent;
}

VJSWebSocketEvent *VJSWebSocketEvent::CreateErrorEvent (VJSWebSocketObject *inWebSocket)
{
	xbox_assert(inWebSocket != NULL);

	VJSWebSocketEvent	*webSocketEvent;

	webSocketEvent = new VJSWebSocketEvent();
	webSocketEvent->fType = eTYPE_WEB_SOCKET;
	webSocketEvent->fTriggerTime.FromSystemTime();

	webSocketEvent->fSubType = eTYPE_ERROR;
	webSocketEvent->fWebSocket = XBOX::RetainRefCountable<VJSWebSocketObject>(inWebSocket);
	
	return webSocketEvent;
}

VJSWebSocketEvent *VJSWebSocketEvent::CreateCloseEvent (VJSWebSocketObject *inWebSocket, bool inWasClean, sLONG inCode, const XBOX::VString &inReason)
{
	xbox_assert(inWebSocket != NULL);

	VJSWebSocketEvent	*webSocketEvent;

	webSocketEvent = new VJSWebSocketEvent();
	webSocketEvent->fType = eTYPE_WEB_SOCKET;
	webSocketEvent->fTriggerTime.FromSystemTime();

	webSocketEvent->fSubType = eTYPE_CLOSE;
	webSocketEvent->fWebSocket = XBOX::RetainRefCountable<VJSWebSocketObject>(inWebSocket);
	webSocketEvent->fWasClean = inWasClean;
	webSocketEvent->fCode = inCode;
	webSocketEvent->fReason = inReason;

	return webSocketEvent;
}

void VJSWebSocketEvent::Process (XBOX::VJSContext inContext, VJSWorker *inWorker)
{
	xbox_assert(inWorker != NULL);

	XBOX::VString	callbackName;
	bool			isOk;

	isOk = true;
	switch (fSubType) {

		case eTYPE_OPEN:	callbackName = "onopen"; break;
		case eTYPE_MESSAGE:	callbackName = "onmessage"; break;
		case eTYPE_ERROR:	callbackName = "onerror"; break;
		case eTYPE_CLOSE:	callbackName = "onclose"; break;
		default:			xbox_assert(false); break;
		
	}
	
	XBOX::VJSObject	object(inContext);

	if (fWebSocket->IsDedicatedWorker())

		object = inContext.GetGlobalObject();

	else {

		object = fWebSocket->GetObject();
		object.SetContext(inContext);

	}

	std::vector<XBOX::VJSValue>	callbackArguments;

	object.SetContext(inContext);
	if (fSubType == eTYPE_OPEN) {
				
		object.SetProperty("protocol", fProtocol);
		object.SetProperty("extensions", fExtensions);

	} else if (fSubType == eTYPE_MESSAGE) {

		XBOX::VJSObject	messageObject(inContext);
		XBOX::VJSValue	value(object.GetProperty("binaryType"));
		XBOX::VString	string;
		bool			isString;

		isString = value.IsString() && value.GetString(string) && string.EqualToString("string");
		messageObject.MakeEmpty();
		if (isString) {

			string.FromBlock(fData, fSize, VTC_UTF_8);
			messageObject.SetProperty("data", string);

			// Discard() will free fData.

		} else if (string.EqualToString("blob")) {

			messageObject.SetProperty("data", VJSBlob::NewInstance(inContext, fData, fSize, ""));

			// Discard() will free fData.

		} else {
			
			// binaryType == "buffer".

			messageObject.SetProperty("data", VJSBufferClass::NewInstance(inContext, fSize, fData));
			fData = NULL;	// Prevent Discard() from freeing memory.

		}
		value = messageObject;
		 
		callbackArguments.push_back(value);

	}  else if (fSubType == eTYPE_CLOSE) {

		XBOX::VJSObject	closeEventObject(inContext);
		
		closeEventObject.MakeEmpty();
		closeEventObject.SetProperty("wasClean", fWasClean);
		closeEventObject.SetProperty("code", (sLONG) fCode);
		closeEventObject.SetProperty("reason", fReason);
		VJSContext	vjsContext(inContext);
		XBOX::VJSValue	value = closeEventObject;
		
		callbackArguments.push_back(value);

	}	// eTYPE_OPEN and eTYPE_ERROR don't have argument.

	// Call callback if it exists.

	if (object.IsObject()) {

		XBOX::VJSValue	value	= object.GetProperty(callbackName);

		if (value.IsFunction())
		
			object.CallMemberFunction(callbackName, &callbackArguments, &value);
		
	}

	Discard();
}

void VJSWebSocketEvent::Discard ()
{
	XBOX::ReleaseRefCountable<VJSWebSocketObject>(&fWebSocket);
	if (fSubType == eTYPE_MESSAGE && fData != NULL)
		
		VMemory::DisposePtr(fData);
	
	Release();
} 

VJSWebSocketServerEvent	*VJSWebSocketServerEvent::CreateConnectionEvent (XBOX::VJSObject& inObject, XBOX::VWebSocket *inWebSocket)
{
	xbox_assert(inWebSocket != NULL);

	VJSWebSocketServerEvent	*webSocketServerEvent;

	webSocketServerEvent = new VJSWebSocketServerEvent(inObject);
	webSocketServerEvent->fType = eTYPE_WEB_SOCKET_SERVER;
	webSocketServerEvent->fTriggerTime.FromSystemTime();

	webSocketServerEvent->fSubType = eTYPE_CONNECTION;

	webSocketServerEvent->fWebSocket = inWebSocket;
	
	return webSocketServerEvent;
}

VJSWebSocketServerEvent	*VJSWebSocketServerEvent::CreateErrorEvent (XBOX::VJSObject& inObject)
{
	VJSWebSocketServerEvent	*webSocketServerEvent;

	webSocketServerEvent = new VJSWebSocketServerEvent(inObject);
	webSocketServerEvent->fType = eTYPE_WEB_SOCKET_SERVER;
	webSocketServerEvent->fTriggerTime.FromSystemTime();

	webSocketServerEvent->fSubType = eTYPE_ERROR;

	return webSocketServerEvent;
}


VJSWebSocketServerEvent	*VJSWebSocketServerEvent::CreateCloseEvent (XBOX::VJSObject& inObject)
{
	VJSWebSocketServerEvent	*webSocketServerEvent;

	webSocketServerEvent = new VJSWebSocketServerEvent(inObject);
	webSocketServerEvent->fType = eTYPE_WEB_SOCKET_SERVER;
	webSocketServerEvent->fTriggerTime.FromSystemTime();

	webSocketServerEvent->fSubType = eTYPE_CLOSE;

	return webSocketServerEvent;
}
	
void VJSWebSocketServerEvent::Process (XBOX::VJSContext inContext, VJSWorker *inWorker)
{
	xbox_assert(inWorker != NULL);

	XBOX::VJSValue				value(inContext);
	std::vector<XBOX::VJSValue>	callbackArguments;
	XBOX::VString				callbackName;
	XBOX::VJSObject				webSocketServer(fObject);
	
	webSocketServer.SetContext(inContext);
	if (fSubType == eTYPE_CONNECTION) {

		value = VJSWebSocketClass::MakeObject(inContext, fWebSocket, inWorker, true);
		if (value.IsNull()) 

			fSubType = eTYPE_ERROR;

		else {

			callbackArguments.push_back(value);
			fWebSocket = NULL;						// Prevent Discard() from deleting it.

		}

	}
	
	switch (fSubType) {

		case eTYPE_CONNECTION:	callbackName = "onconnect"; break;
		case eTYPE_ERROR:		callbackName = "onerror"; break;
		case eTYPE_CLOSE:		callbackName = "onclose"; break;
		default:				xbox_assert(false); break;
		
	}

	if (webSocketServer.IsObject()) {

		value = webSocketServer.GetProperty(callbackName);
		if (value.IsFunction())
		
			webSocketServer.CallMemberFunction(callbackName, &callbackArguments, &value);
		
	}

	Discard();
}

void VJSWebSocketServerEvent::Discard ()
{
	if (fSubType == eTYPE_CONNECTION && fWebSocket != NULL)
		
		delete fWebSocket;
			
	Release();
}

VJSWebSocketConnectEvent *VJSWebSocketConnectEvent::Create (XBOX::VWebSocket *inWebSocket)
{
	xbox_assert(inWebSocket != NULL);

	VJSWebSocketConnectEvent	*webSocketConnectEvent;
	
	webSocketConnectEvent = new VJSWebSocketConnectEvent();
	webSocketConnectEvent->fType = eTYPE_WEB_SOCKET_CONNECT;
	webSocketConnectEvent->fTriggerTime.FromSystemTime();
	webSocketConnectEvent->fWebSocket = inWebSocket;

	return webSocketConnectEvent;
}
	
void VJSWebSocketConnectEvent::Process (XBOX::VJSContext inContext, VJSWorker *inWorker)
{
	xbox_assert(!inWorker->IsDedicatedWorker());

	XBOX::VJSObject	globalObject	= inContext.GetGlobalObject();
	XBOX::VJSObject	callbackObject	= globalObject.GetPropertyAsObject("onconnect");

	if (callbackObject.IsFunction()) { 
		
		XBOX::VJSValue	emptyStringValue(inContext);

		emptyStringValue.SetString("");
 
		XBOX::VJSObject	object(inContext);
		XBOX::VJSObject	messageEventObject	= VJSDOMMessageEventClass::NewInstance(inContext, object, fTriggerTime, emptyStringValue);
		VJSDOMEvent		*domEvent			= messageEventObject.GetPrivateData<VJSDOMMessageEventClass>();

		domEvent->fType = "connect";
				
		XBOX::VJSArray	portsArrayObject(inContext);
		XBOX::VJSValue	webSocket(VJSWebSocketClass::MakeObject(inContext, fWebSocket, inWorker, false));

		webSocket.Protect();
		portsArrayObject.PushValue(webSocket);
		messageEventObject.SetProperty("ports", portsArrayObject);
		
		std::vector<XBOX::VJSValue>	callbackArguments;
												
		callbackArguments.push_back(messageEventObject);
		inContext.GetGlobalObject().CallFunction(callbackObject, &callbackArguments, NULL);

		fWebSocket = NULL;	// Prevent Discard() from deleting it.

	} 

	Discard();
}	
	
void VJSWebSocketConnectEvent::Discard ()
{
	if (fWebSocket != NULL)

		delete fWebSocket;

	Release();
}
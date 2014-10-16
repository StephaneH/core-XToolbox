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

#include "VJSWorkerProxy.h"

#include "VJSGlobalClass.h"
#include "VJSContext.h"
#include "VJSWorker.h"
#include "VJSMessagePort.h"
#include "VJSWebSocket.h"

USING_TOOLBOX_NAMESPACE

// Add constructed worker proxy to either Worker.list[] or SharedWorker.list[], trying to fill "holes" inside the array.

static void	_AddToList (XBOX::VJSContext &inContext, const XBOX::VString &inConstructorName, XBOX::VJSObject &inConstructedObject);

VJSWorkerLocationObject::VJSWorkerLocationObject (const XBOX::VString &inURL)
{
	XBOX::VURL	url	= XBOX::VURL(inURL, true);
		
	url.GetAbsoluteURL(fHRef, false);
	url.GetScheme(fProtocol);
	url.GetNetworkLocation(fHost, false);
	url.GetHostName(fHostName, false);
	url.GetPortNumber(fPort, false);
	url.GetPath(fPathName, eURL_POSIX_STYLE, false);
	url.GetQuery(fSearch, false);
	url.GetFragment(fHash, false);		
}

void VJSWorkerLocationClass::GetDefinition (ClassDefinition &outDefinition)
{
    outDefinition.className	= "WorkerLocation";
    outDefinition.initialize = js_initialize<_Initialize>;
}

void VJSWorkerLocationClass::_Initialize (const XBOX::VJSParms_initialize &inParms, VJSWorkerLocationObject *inWorkerLocation)
{
	xbox_assert(inWorkerLocation != NULL);

	XBOX::VJSObject	object(inParms.GetObject());

	object.SetProperty("href",		inWorkerLocation->fHRef,		JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete);
	object.SetProperty("protocol",	inWorkerLocation->fProtocol,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete);
	object.SetProperty("host",		inWorkerLocation->fHost,		JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete);
	object.SetProperty("hostname",	inWorkerLocation->fHostName,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete);
	object.SetProperty("port",		inWorkerLocation->fPort,		JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete);
	object.SetProperty("pathname",	inWorkerLocation->fPathName,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete);
	object.SetProperty("search",	inWorkerLocation->fSearch,		JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete);
	object.SetProperty("hash",		inWorkerLocation->fHash,		JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete);
}

VJSWebWorkerObject::VJSWebWorkerObject (VJSMessagePort *inMessagePort, VJSMessagePort *inErrorPort, VJSWorker *inWorker)
{
	xbox_assert(inMessagePort != NULL && inErrorPort != NULL && inWorker != NULL);

	fOnMessagePort = XBOX::RetainRefCountable<VJSMessagePort>(inMessagePort);
	fOnErrorPort = XBOX::RetainRefCountable<VJSMessagePort>(inErrorPort);
	fWorker = inWorker;
}

VJSWebWorkerObject::~VJSWebWorkerObject () 
{
	XBOX::ReleaseRefCountable<VJSMessagePort>(&fOnMessagePort);
	XBOX::ReleaseRefCountable<VJSMessagePort>(&fOnErrorPort);
}

VJSWebWorkerObject *VJSWebWorkerObject::_CreateWorker (XBOX::VJSContext &inParentContext, VJSWorker *inOutsideWorker, 
														bool inReUseContext, VJSWebSocketObject *inWebSocket,
														const XBOX::VString &inUrl, bool inIsDedicated, const XBOX::VString &inName)
{
	xbox_assert(inOutsideWorker != NULL);
	
	bool		needToRun;
	VJSWorker	*insideWorker;
			
	if (inIsDedicated) {
	
		needToRun = true;
		insideWorker = new VJSWorker(inParentContext, inOutsideWorker, inUrl, inReUseContext, inWebSocket);
				
	} else 

		insideWorker = VJSWorker::RetainSharedWorker(inParentContext, inOutsideWorker, inUrl, inName, inReUseContext, &needToRun);

	xbox_assert(insideWorker != NULL);

	if (insideWorker == inOutsideWorker) {

		// Recursive SharedWorker constructor calls are forbidden.

		xbox_assert(!inIsDedicated);

		XBOX::ReleaseRefCountable<VJSWorker>(&insideWorker);
		XBOX::vThrowError(XBOX::VE_JVSC_RECURSIVE_SHARED_WORKER, inUrl);

		return NULL;

	}

	VJSMessagePort	*onMessagePort, *onErrorPort;
	
	onMessagePort = VJSMessagePort::Create(inOutsideWorker, insideWorker,inParentContext);
	onMessagePort->SetCallbackName(inOutsideWorker, "onmessage");	
	onMessagePort->SetCallbackName(insideWorker, "onmessage");
	
	onErrorPort = VJSMessagePort::Create(inOutsideWorker, insideWorker,inParentContext);
	onErrorPort->SetCallbackName(inOutsideWorker, "onerror");	
	onErrorPort->SetCallbackName(insideWorker, "onerror");

	inOutsideWorker->AddErrorPort(onErrorPort);
	insideWorker->AddErrorPort(onErrorPort);

	if (inIsDedicated)

		insideWorker->SetMessagePort(onMessagePort);

	VJSWebWorkerObject	*webWorkerObject;

	webWorkerObject = new VJSWebWorkerObject(onMessagePort, onErrorPort, insideWorker);

	xbox_assert(!(inIsDedicated && !needToRun));
	if (needToRun)

		insideWorker->Run();
	
	if (!inIsDedicated)

		insideWorker->Connect(onMessagePort, onErrorPort);

	XBOX::ReleaseRefCountable<VJSMessagePort>(&onMessagePort);
	XBOX::ReleaseRefCountable<VJSMessagePort>(&onErrorPort);
	XBOX::ReleaseRefCountable<VJSWorker>(&insideWorker);

	return webWorkerObject;
}

void VJSDedicatedWorkerClass::GetDefinition (ClassDefinition &outDefinition)
{
	static inherited::StaticFunction functions[] =
	{
		{ "terminate",		js_callStaticFunction<_terminate>,		JS4D::PropertyAttributeDontDelete	},		
		{ "postMessage",	js_callStaticFunction<_postMessage>,	JS4D::PropertyAttributeDontDelete	},
		{ 0,				0,										0									},
	};	
	
	outDefinition.className	= "Worker";
	outDefinition.staticFunctions = functions;
	outDefinition.finalize = js_finalize<_Finalize>;
}


JS4D::StaticFunction VJSDedicatedWorkerClass::sConstrFunctions[] =
{
	{	"getNumberRunning",	XBOX::js_callback<void, _GetNumberRunning>,	JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
	{	0,				0,										0									},
};
XBOX::VJSObject	VJSDedicatedWorkerClass::MakeConstructor (XBOX::VJSContext inContext)
{
	XBOX::VJSObject	constructor = 
		JS4DMakeConstructor(inContext, Class(), _Construct, false,	VJSDedicatedWorkerClass::sConstrFunctions );

	XBOX::VJSArray	listArray(inContext);
	
	constructor.SetProperty("list", listArray, JS4D::PropertyAttributeDontDelete);
	
	return constructor;
}

void VJSDedicatedWorkerClass::_Construct (VJSParms_construct &ioParms)
{
	XBOX::VString		url;
	bool				reUseContext;
	XBOX::VJSObject		webSocket(ioParms.GetContext());
	bool				isOk;
	
	if (!ioParms.CountParams() || !ioParms.IsStringParam(1) || !ioParms.GetStringParam(1, url)) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
		isOk = false;

	} else {

		reUseContext = false;
		isOk = true;

	}

	if (isOk && ioParms.CountParams() >= 2) {

		if (ioParms.IsBooleanParam(2)) {

			if (!ioParms.GetBoolParam(2, &reUseContext)) {
				
				XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "2");
				isOk = false;

			}

		} else if (!ioParms.GetParamObject(2, webSocket) || !webSocket.IsOfClass(VJSWebSocketClass::Class())) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER, "2");
			isOk = false;

		} else if (ioParms.CountParams() >= 3 
		&& !(ioParms.IsBooleanParam(3) && ioParms.GetBoolParam(3, &reUseContext))) {
			
			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "3");
			isOk = false;

		}

	}

	if (isOk) {

		VJSWorker			*outsideWorker;
		VJSWebWorkerObject	*workerProxy;
		XBOX::VJSContext	context(ioParms.GetContext());
		XBOX::VJSObject		constructedObject(context);	
		VJSWebSocketObject	*webSocketObject;

		outsideWorker = VJSWorker::RetainWorker(context);
		xbox_assert(outsideWorker != NULL);

		if (!webSocket.HasRef()) 

			webSocketObject = NULL;

		else {

			webSocketObject = webSocket.GetPrivateData<VJSWebSocketClass>();
			xbox_assert(webSocketObject != NULL);

		}

		if ((workerProxy = VJSWebWorkerObject::_CreateWorker(context, outsideWorker, reUseContext, webSocketObject, url, true)) != NULL) {

			constructedObject = VJSDedicatedWorkerClass::CreateInstance(context, workerProxy);

			if (!webSocket.IsOfClass(VJSWebSocketClass::Class())) {

				workerProxy->fOnMessagePort->SetObject(outsideWorker, constructedObject);
				workerProxy->fOnErrorPort->SetObject(outsideWorker, constructedObject);

			}
			_AddToList(context, "Worker", constructedObject);

		}
		
		ioParms.ReturnConstructedObject(constructedObject);

		XBOX::ReleaseRefCountable<VJSWorker>(&outsideWorker);

	}
}

void VJSDedicatedWorkerClass::_Finalize (const XBOX::VJSParms_finalize &inParms, VJSWebWorkerObject *inDedicatedWorker)
{
	xbox_assert(inDedicatedWorker != NULL);

	delete inDedicatedWorker;
}

void VJSDedicatedWorkerClass::_terminate (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebWorkerObject *inDedicatedWorker)
{
	xbox_assert(inDedicatedWorker != NULL);

	VJSWorker	*outsideWorker	= VJSWorker::RetainWorker(ioParms.GetContext());
	VJSWorker	*insideWorker	= inDedicatedWorker->fOnMessagePort->GetOther(outsideWorker);
	
//**

	// ACI0081079: 
	//
	// Defensive programming, check pointer is not NULL. Pending definitive fix (and architecture overall in WAK5).
	// insideWorker should never be NULL !

	if (insideWorker != NULL)

		insideWorker->Terminate();


//**

	XBOX::ReleaseRefCountable<VJSWorker>(&outsideWorker);
}

void VJSDedicatedWorkerClass::_postMessage (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebWorkerObject *inDedicatedWorker)
{
	xbox_assert(inDedicatedWorker != NULL && inDedicatedWorker->fWorker != NULL);

	if (inDedicatedWorker->fWorker->GetWebSocket() == NULL)

		VJSMessagePort::PostMessageMethod(ioParms, inDedicatedWorker->fOnMessagePort);

	else

		XBOX::vThrowError(XBOX::VE_JVSC_INVALID_STATE);	// If the dedicated worker is WebSocket related, then it makes no sense.
}

void VJSDedicatedWorkerClass::_GetNumberRunning (XBOX::VJSParms_callStaticFunction &ioParms, void *)
{
	ioParms.ReturnNumber<sLONG>(VJSWorker::GetNumberRunning(VJSWorker::TYPE_DEDICATED));
}

void VJSSharedWorkerClass::GetDefinition (ClassDefinition &outDefinition)
{
	outDefinition.className	= "SharedWorker";
    outDefinition.finalize = js_finalize<_Finalize>;	
}

JS4D::StaticFunction VJSSharedWorkerClass::sConstrFunctions[] =
{
	{	"isRunning",	XBOX::js_callback<void, _IsRunning>,	JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
	{	"getNumberRunning",	XBOX::js_callback<void, _GetNumberRunning>,	JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
	{	0,				0,										0									},
};

XBOX::VJSObject	VJSSharedWorkerClass::MakeConstructor (XBOX::VJSContext inContext)
{
	XBOX::VJSObject	constructor =
		JS4DMakeConstructor(inContext, Class(), _Construct, false,	VJSSharedWorkerClass::sConstrFunctions );

	XBOX::VJSArray	listArray(inContext);
	
	constructor.SetProperty("list", listArray, JS4D::PropertyAttributeDontDelete);	

	return constructor;
}

void VJSSharedWorkerClass::_Construct (VJSParms_construct &ioParms)
{
	if (!ioParms.CountParams()
	|| !ioParms.IsStringParam(1)
	|| (ioParms.CountParams() >= 2 && !ioParms.IsStringParam(2))) {
	
		XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
		return;

	}

	XBOX::VJSContext	context(ioParms.GetContext());
	XBOX::VString		url, name;
	bool				reUseContext;
	VJSWorker			*outsideWorker;
	VJSWebWorkerObject	*workerProxy;	
	XBOX::VJSObject		constructedObject(context);

	ioParms.GetStringParam(1, url);
	if (ioParms.CountParams() == 2)

		ioParms.GetStringParam(2, name);

	outsideWorker = VJSWorker::RetainWorker(context);

	if (ioParms.CountParams() < 3 || !ioParms.IsBooleanParam(3) || !ioParms.GetBoolParam(3, &reUseContext))

		reUseContext = false;

	if ((workerProxy = VJSWebWorkerObject::_CreateWorker(context, outsideWorker, reUseContext, NULL, url, false, name)) != NULL) {

		constructedObject = VJSSharedWorkerClass::CreateInstance(context, workerProxy);
		workerProxy->fOnErrorPort->SetObject(outsideWorker, constructedObject);

		XBOX::VJSObject	messagePortObject(context);
		
		messagePortObject = VJSMessagePortClass::CreateInstance(context, workerProxy->fOnMessagePort);
		workerProxy->fOnMessagePort->SetObject(outsideWorker, messagePortObject);
		constructedObject.SetProperty("port", messagePortObject);

		_AddToList(context, "SharedWorker", constructedObject);

	}

	ioParms.ReturnConstructedObject(constructedObject);

	XBOX::ReleaseRefCountable<VJSWorker>(&outsideWorker);
}

void VJSSharedWorkerClass::_Finalize (const XBOX::VJSParms_finalize &inParms, VJSWebWorkerObject *inSharedWorker)
{
	xbox_assert(inSharedWorker != NULL);

	delete inSharedWorker;
}

void VJSSharedWorkerClass::_IsRunning (XBOX::VJSParms_callStaticFunction &ioParms, void *)
{
	XBOX::VString	url, name;

	if (!ioParms.GetStringParam(1, url)) 

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");

	else if (!ioParms.GetStringParam(2, name)) 

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2");

	else 

		ioParms.ReturnBool(VJSWorker::IsSharedWorkerRunning(url, name));
}

void VJSSharedWorkerClass::_GetNumberRunning (XBOX::VJSParms_callStaticFunction &ioParms, void *)
{
	ioParms.ReturnNumber<sLONG>(VJSWorker::GetNumberRunning(VJSWorker::TYPE_SHARED));
}

static void _AddToList (XBOX::VJSContext &inContext, const XBOX::VString &inConstructorName, XBOX::VJSObject &inConstructedObject)
{
	if (inContext.GetGlobalObject().HasProperty(inConstructorName)) {

		XBOX::VJSObject	workerObject	= inContext.GetGlobalObject().GetPropertyAsObject(inConstructorName);

		if (workerObject.IsObject() && workerObject.HasProperty("list")) {

			XBOX::VJSValue	value	= workerObject.GetProperty("list");

			if (value.IsArray()) {

				XBOX::VJSArray	listArray(value);
				sLONG			i;

				for (i = 0; i < listArray.GetLength(); i++) {

					value = listArray.GetValueAt(i);
					if (value.IsNull()) {

						listArray.SetValueAt(i, inConstructedObject);
						break;

					}	

				}
				if (i == listArray.GetLength()) 

					listArray.PushValue(inConstructedObject);

			} else {

				xbox_assert(false);

			}

		} else {

			xbox_assert(false);

		}
			
	} else {

		xbox_assert(false);

	}
}
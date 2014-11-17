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

#include "VJSNet.h"

#include "VJSContext.h"
#include "VJSGlobalClass.h"

#include "VJSEvent.h"
#include "VJSNetServer.h"
#include "VJSNetSocket.h"

USING_TOOLBOX_NAMESPACE

const XBOX::VString	VJSNetClass::kModuleName	= CVSTR("net");

void VJSNetClass::RegisterModule ()
{
	VJSGlobalClass::RegisterModuleConstructor(kModuleName, _ModuleConstructor);
}

XBOX::VJSObject VJSNetClass::_ModuleConstructor (const VJSContext &inContext, const VString &inModuleName)
{
	xbox_assert(inModuleName.EqualToString(kModuleName, true));

	VJSNetClass::Class();
	return VJSNetClass::CreateInstance(inContext, NULL);
}

void VJSNetClass::GetDefinition (ClassDefinition &outDefinition)
{
	static inherited::StaticFunction functions[] =
	{
		{ "createServer",			js_callStaticFunction<_createServer>,			JS4D::PropertyAttributeDontDelete	},
		{ "createConnection",		js_callStaticFunction<_createConnection>,		JS4D::PropertyAttributeDontDelete	},
		{ "connect",				js_callStaticFunction<_connect>,				JS4D::PropertyAttributeDontDelete	},

		{ "createServerSync",		js_callStaticFunction<_createServerSync>,		JS4D::PropertyAttributeDontDelete	},
		{ "createConnectionSync",	js_callStaticFunction<_createConnectionSync>,	JS4D::PropertyAttributeDontDelete	},
		{ "connectSync",			js_callStaticFunction<_connectSync>,			JS4D::PropertyAttributeDontDelete	},

		{ "isIP",					js_callStaticFunction<_isIP>,					JS4D::PropertyAttributeDontDelete	},
 		{ "isIPv4",					js_callStaticFunction<_isIPv4>,					JS4D::PropertyAttributeDontDelete	},
		{ "isIPv6",					js_callStaticFunction<_isIPv6>,					JS4D::PropertyAttributeDontDelete	},

		{ 0,						0,												0									},
	};

    outDefinition.className	= "net";    
	outDefinition.initialize = js_initialize<_Initialize>;
	outDefinition.staticFunctions = functions;	
}

// Create a Server or ServerSync object.

void VJSNetClass::CreateServer (XBOX::VJSParms_callStaticFunction &ioParms, bool inIsSync, bool inIsSSL)
{
	XBOX::VJSObject	listener(ioParms.GetContext());
	bool			allowHalfOpen;	
	XBOX::VString	key, certificate;

	allowHalfOpen = true;
	if (ioParms.CountParams() == 1)	{

		XBOX::VJSObject	object(ioParms.GetContext());
 
		if (!ioParms.IsObjectParam(1) || !ioParms.GetParamObject(1, object)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_OBJECT, "1");
			return;

		} else if (object.IsFunction()) 

			listener = object;

		else {

			allowHalfOpen = object.GetPropertyAsBool("allowHalfOpen", NULL, NULL);
			if (inIsSSL) {

				object.GetPropertyAsString("key", NULL, key);
				object.GetPropertyAsString("cert", NULL, certificate);

			}

		}

	} else if (ioParms.CountParams() == 2) {

		XBOX::VJSObject	options(ioParms.GetContext());

		if (!ioParms.IsObjectParam(1) || !ioParms.GetParamObject(1, options)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_OBJECT, "1");
			return;

		} 

		if (!ioParms.IsObjectParam(2) || !ioParms.GetParamObject(2, listener) || !listener.IsFunction()) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "2");
			return;

		}
		allowHalfOpen = options.GetPropertyAsBool("allowHalfOpen", NULL, NULL);

		if (inIsSSL) {

			options.GetPropertyAsString("key", NULL, key);
			options.GetPropertyAsString("cert", NULL, certificate);

		}

	} else {

		XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
		return;

	}

	// If SSL, check key and certificate have been specified.

	if (inIsSSL && (!key.GetLength() || !certificate.GetLength())) {

		XBOX::vThrowError(XBOX::VE_JVSC_EXPECTING_SSL_INFO);
		return;

	}

	VJSNetServerObject	*serverObject;

	if ((serverObject = new VJSNetServerObject(inIsSync, allowHalfOpen)) == NULL) {

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
		return;

	}

	if (inIsSSL) 

		serverObject->SetKeyAndCertificate(key, certificate);

	XBOX::VJSObject	createdObject(ioParms.GetContext());

	if (inIsSync) 

		createdObject = VJSNetServerSyncClass::CreateInstance(ioParms.GetContext(), serverObject);

	else
		
		createdObject = VJSNetServerClass::CreateInstance(ioParms.GetContext(), serverObject);

	// If the object is ServerSync, the "connection" event listener, if any, is silently ignored.

	if (!inIsSync && listener.IsFunction()) {

		VJSWorker	*worker;

		worker = VJSWorker::RetainWorker(ioParms.GetContext());
		xbox_assert(worker != NULL);

		serverObject->AddListener(worker, inIsSSL ? "secureConnection" : "connection", listener, false);

		XBOX::ReleaseRefCountable<VJSWorker>(&worker);

	}
	
	ioParms.ReturnValue(createdObject);
}

// Create a Socket or SocketSync object and connect it.

void VJSNetClass::ConnectSocket (XBOX::VJSParms_callStaticFunction &ioParms, bool inIsSync, bool inIsSSL)
{
	VJSNetSocketObject	*socketObject; 

	if ((socketObject = new VJSNetSocketObject(inIsSync, VJSNetSocketObject::eTYPE_TCP4, true,ioParms.GetContext())) != NULL) {

		socketObject->fWorker = VJSWorker::RetainWorker(ioParms.GetContext());
		if (!inIsSync) {

			socketObject->fObject = VJSNetSocketClass::CreateInstance(ioParms.GetContext(), socketObject);
			VJSNetSocketClass::Connect(ioParms, socketObject->fObject, inIsSSL);

		} else {

			socketObject->fObject = VJSNetSocketSyncClass::CreateInstance(ioParms.GetContext(), socketObject);
			VJSNetSocketSyncClass::Connect(ioParms, socketObject->fObject, inIsSSL);
	
		}

		ioParms.ReturnValue(socketObject->fObject);

	} else {

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
		ioParms.ReturnUndefinedValue();

	}
}

void VJSNetClass::_Initialize (const XBOX::VJSParms_initialize &inParms, void *)
{
	VJSObject function = inParms.GetContext().GetGlobalObject().GetPropertyAsObject("Socket");
	xbox_assert(function.IsFunction());
	XBOX::VJSObject	thisObject = inParms.GetObject();

	thisObject.SetProperty(CVSTR("Socket"), function, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);

	function = inParms.GetContext().GetGlobalObject().GetPropertyAsObject("Server");
	thisObject.SetProperty(CVSTR("Server"), function, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);

	function = inParms.GetContext().GetGlobalObject().GetPropertyAsObject("ServerSync");
	thisObject.SetProperty(CVSTR("ServerSync"), function, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);


	function = inParms.GetContext().GetGlobalObject().GetPropertyAsObject("SocketSync");
	thisObject.SetProperty(CVSTR("SocketSync"), function, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
}


void VJSNetClass::_createServer (XBOX::VJSParms_callStaticFunction &ioParms, void *)
{
	VJSNetClass::CreateServer(ioParms, false, false);
}

void VJSNetClass::_createConnection (XBOX::VJSParms_callStaticFunction &ioParms, void *)
{
	VJSNetClass::ConnectSocket(ioParms, false, false);
}

void VJSNetClass::_connect (XBOX::VJSParms_callStaticFunction &ioParms, void *)
{
	VJSNetClass::ConnectSocket(ioParms, false, false);
}

void VJSNetClass::_createServerSync (XBOX::VJSParms_callStaticFunction &ioParms, void *)
{
	VJSNetClass::CreateServer(ioParms, true, false);
}

void VJSNetClass::_createConnectionSync (XBOX::VJSParms_callStaticFunction &ioParms, void *)
{
	VJSNetClass::ConnectSocket(ioParms, true, false);
}

void VJSNetClass::_connectSync (XBOX::VJSParms_callStaticFunction &ioParms, void *)
{
	VJSNetClass::ConnectSocket(ioParms, true, false);
}

void VJSNetClass::_isIP (XBOX::VJSParms_callStaticFunction &ioParms, void *)
{
	ioParms.ReturnNumber(_isIPAddress(ioParms));
}

void VJSNetClass::_isIPv4 (XBOX::VJSParms_callStaticFunction &ioParms, void *)
{
	ioParms.ReturnBool(_isIPAddress(ioParms) == 4);
}

void VJSNetClass::_isIPv6 (XBOX::VJSParms_callStaticFunction &ioParms, void *)
{
	XBOX::vThrowError(XBOX::VE_JVSC_UNSUPPORTED_FUNCTION, "net.ipIPv6()");	
	ioParms.ReturnUndefinedValue();
}

// Return 0 if inIPAddress is invalid, 4 or 6 for a valid IPv4 or IPv6 address.

uLONG VJSNetClass::_isIPAddress (XBOX::VJSParms_callStaticFunction &ioParms)
{

	XBOX::VString	ipAddress;

	if (!ioParms.CountParams() || !ioParms.IsStringParam(1) || !ioParms.GetStringParam(1, ipAddress)) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
		return 0;

	} else {

		StErrorContextInstaller	context(false, true);
		XBOX::VNetAddress			addr(ipAddress, 1234);
		
		if (context.GetLastError() != XBOX::VE_OK)
			
			return 0;
		
		else
			
			return addr.IsV4() ? 4 : 6;
		
	}
}

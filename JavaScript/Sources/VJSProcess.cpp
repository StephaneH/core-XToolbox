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

#include <list>

#include "ServerNet/VServerNet.h"
#include "VJSProcess.h"

#include "VJSContext.h"
#include "VJSGlobalClass.h"

#include "VJSWorker.h"

#define SSJS_ENV_VARIABLES_ENABLED 1

#if SSJS_ENV_VARIABLES_ENABLED
#include "VJSEnvironment.h"
#endif


USING_TOOLBOX_NAMESPACE


void VJSProcess::Initialize ( const XBOX::VJSParms_initialize& inParms, VJSEventEmitter *inEventEmitter)
{
	xbox_assert(inEventEmitter != NULL);

	inEventEmitter->Retain();
}

void VJSProcess::Finalize ( const XBOX::VJSParms_finalize& inParms, VJSEventEmitter *inEventEmitter)
{
	xbox_assert(inEventEmitter != NULL);

	inEventEmitter->Release();
}

void VJSProcess::GetDefinition( ClassDefinition& outDefinition)
{
	static inherited::StaticFunction functions[] =
	{
		{ 0, 0, 0}
	};

	static inherited::StaticValue values[] =
	{
		{ "version",		js_getProperty<_Version>,		NULL,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ "buildNumber",	js_getProperty<_BuildNumber>,	NULL,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ "closingFlag",	js_getProperty<_ClosingFlag>,	NULL,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ "pid",			js_getProperty<_PID>,			NULL,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ "productName",	js_getProperty<_ProductName>,	NULL,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ "productType",	js_getProperty<_ProductType>,	NULL,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ "env",			js_getProperty<_Env>,			NULL,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ 0, 0, 0, 0}
	};

	outDefinition.className = "process";
	outDefinition.parentClass = VJSEventEmitterClass::Class();
	outDefinition.initialize = js_initialize<Initialize>;
	outDefinition.finalize = js_finalize<Finalize>;
	outDefinition.staticFunctions = functions;
	outDefinition.staticValues = values;
}

void VJSProcess::_Version ( XBOX::VJSParms_getProperty& ioParms, VJSEventEmitter *)
{
	XBOX::VString		vstrVersion;
	XBOX::VProcess::Get ( )-> GetProductVersion ( vstrVersion );

	ioParms. ReturnString ( vstrVersion );
}

void VJSProcess::_BuildNumber ( XBOX::VJSParms_getProperty& ioParms, VJSEventEmitter *)
{
	VString s;
	VProcess::Get()->GetBuildNumber( s);
	ioParms.ReturnString( s);
}

void VJSProcess::_ClosingFlag ( XBOX::VJSParms_getProperty& ioParms, VJSEventEmitter *)
{
	VJSWorker	*worker;

	if ((worker = VJSWorker::RetainWorker(ioParms.GetContext())) != NULL) {

		ioParms.ReturnBool(worker->GetClosingFlag());
		worker->Release();

	} else {

		// Should be impossible, but fail safe.

		xbox_assert(false);	
		ioParms.ReturnBool(false);

	}
}


void VJSProcess::_PID( XBOX::VJSParms_getProperty& ioParms, VJSEventEmitter *)
{
	ioParms.ReturnNumber( VProcess::Get()->GetSystemID());
}


void VJSProcess::_ProductName( XBOX::VJSParms_getProperty& ioParms, VJSEventEmitter *)
{
	VString productName;
	VProcess::Get()->GetProductName( productName);
	ioParms.ReturnString( productName);
}


void VJSProcess::_ProductType( XBOX::VJSParms_getProperty& ioParms, VJSEventEmitter *)
{
#if VERSIONDEBUG
    ioParms.ReturnString( L"debug");
#elif WITH_ASSERT
    ioParms.ReturnString( L"beta");
#else
	ioParms.ReturnString( L"release");
#endif
}

void VJSProcess::_Env(XBOX::VJSParms_getProperty& ioParms, VJSEventEmitter *)
{
#if SSJS_ENV_VARIABLES_ENABLED
	VJSEnvironmentObject* envObj = new VJSEnvironmentObject();

	ioParms.ReturnValue(VJSEnvironmentClass::CreateInstance(ioParms.GetContext(), envObj));

	ReleaseRefCountable(&envObj);
#else
	//Just expose an empty object
	VJSObject envObj(ioParms.GetContext());

	envObj.MakeEmpty();

	ioParms.ReturnValue(envObj);
#endif
}

void VJSProcess::_getAccessor( XBOX::VJSParms_getProperty& ioParms, XBOX::VJSGlobalObject* inGlobalObject)
{
	ioParms.ReturnValue( VJSProcess::CreateInstance( ioParms.GetContext(), NULL));
}

void VJSOS::GetDefinition( ClassDefinition& outDefinition)
{
	typedef XBOX::VJSClass<VJSOS, void>	inherited;

	static inherited::StaticFunction functions[] =
	{
		{	"type",					js_callStaticFunction<_Type>,				JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"networkInterfaces",	js_callStaticFunction<_NetworkInterfaces>,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"getProxy",				js_callStaticFunction<_getProxy>,			JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	0,						0,											0																	},
	};

	static inherited::StaticValue values[] =
	{
		{	"isWindows",			js_getProperty<_IsWindows>,			NULL,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{	"isMac",				js_getProperty<_IsMac>,				NULL,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{	"isLinux",				js_getProperty<_IsLinux>,			NULL,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	0,						0,									0,		0																	},
	};

	outDefinition.className			= "os";
	outDefinition.initialize		= js_initialize<_Initialize>;
	outDefinition.finalize			= js_finalize<_Finalize>;
	outDefinition.staticFunctions	= functions;
	outDefinition.staticValues		= values;
}

void VJSOS::_Initialize (const XBOX::VJSParms_initialize &inParms, void *)
{
}

void VJSOS::_Finalize (const XBOX::VJSParms_finalize &inParms, void *)
{
}

void VJSOS::_Type (XBOX::VJSParms_callStaticFunction &ioParms, void *)
{
	XBOX::VString	vstrOSVersion;

	XBOX::VSystem::GetOSVersionString(vstrOSVersion);
	ioParms.ReturnString(vstrOSVersion);
}

void VJSOS::_NetworkInterfaces (XBOX::VJSParms_callStaticFunction &ioParms, void *)
{
	XBOX::VNetAddressList	addressList;
	
	addressList.FromLocalInterfaces();


	std::list<Address>						addresses;
	XBOX::VNetAddressList::const_iterator	i;

	// TODO: 
	//	1) Retrieve actual interface name (currently set dummy name). 
	//	2) Support listing of both IPv4 *and* IPv6 addresses if the interface handles both.

//**
	sLONG			counter			= 0;
	XBOX::VString	interfaceName	= "InterfaceX";
//**

	for (i = addressList.begin(); i != addressList.end(); i++) {

		Address	address;

//**
		interfaceName.SetUniChar(10, '0' + counter++);
//**

		address.fInterfaceName = interfaceName;
		address.fIPAddress = i->GetIP();
		address.fIsIPv6 = i->IsV6();
		address.fIsInternal = i->IsLoopBack();

		addresses.push_back(address);
		
	}

	XBOX::VJSObject	result(ioParms.GetContext());

	result.MakeEmpty();
	if (!addresses.empty()) {

		std::list<Address>::iterator	j;

		for (j = addresses.begin(); j != addresses.end(); j++) {

			XBOX::VJSObject	entryObject(ioParms.GetContext());

			entryObject.MakeEmpty();
			entryObject.SetProperty("address", j->fIPAddress);
			entryObject.SetProperty("family", j->fIsIPv6 ? XBOX::VString("IPv6") : XBOX::VString("IPv4"));
			entryObject.SetProperty("internal", j->fIsInternal);
			
			if (result.HasProperty(j->fInterfaceName)) {

				XBOX::VJSObject	object(ioParms.GetContext());

				object = result.GetPropertyAsObject(j->fInterfaceName);
				xbox_assert(object.IsArray());

				XBOX::VJSArray	arrayObject(object);

				arrayObject.PushValue(entryObject);	

			} else {

				XBOX::VJSArray	arrayObject(ioParms.GetContext());

				result.SetProperty(j->fInterfaceName, arrayObject);
				arrayObject.PushValue(entryObject);	
				
			}

		}

	}
	ioParms.ReturnValue(result);
}


void VJSOS::_getProxy(XBOX::VJSParms_callStaticFunction &ioParms, void *)
{
	XBOX::VString pUrl;
    bool resUrl;
    resUrl=ioParms.GetStringParam(1, pUrl);

	XBOX::VURL url(pUrl, true /*is encoded*/);

	XBOX::VString proxy;
	PortNumber port=kBAD_PORT;

	XBOX::VError verr=XBOX::VProxyManager::GetProxyForURL(url, &proxy, &port);

	if(verr==VE_OK && !proxy.IsEmpty())
	{
		XBOX::VJSObject	result(ioParms.GetContext());

		result.MakeEmpty();

		result.SetProperty("host", proxy);
		result.SetProperty("port", port);

		ioParms.ReturnValue(result);
	}
}

void VJSOS::_IsMac (XBOX::VJSParms_getProperty &ioParms, void *)
{
#if VERSIONMAC
	ioParms.ReturnBool(true);
#else
	ioParms.ReturnBool(false);
#endif
}

void VJSOS::_IsWindows (XBOX::VJSParms_getProperty &ioParms, void *)
{
#if VERSIONWIN
	ioParms.ReturnBool(true);
#else
	ioParms.ReturnBool(false);
#endif
}

void VJSOS::_IsLinux (XBOX::VJSParms_getProperty &ioParms, void *)
{
#if VERSION_LINUX
	ioParms.ReturnBool(true);
#else
	ioParms.ReturnBool(false);
#endif
}

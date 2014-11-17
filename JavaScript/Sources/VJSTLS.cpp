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

#include "VJSTLS.h"

#include "VJSContext.h"
#include "VJSGlobalClass.h"
#include "VJSWorker.h"
#include "VJSNet.h"
#include "VJSNetServer.h"
#include "VJSNetSocket.h"

USING_TOOLBOX_NAMESPACE

const XBOX::VString	VJSTLSClass::kModuleName	= CVSTR("tls");

void VJSTLSClass::RegisterModule ()
{
	VJSGlobalClass::RegisterModuleConstructor(kModuleName, _ModuleConstructor);
}

XBOX::VJSObject VJSTLSClass::_ModuleConstructor (const VJSContext &inContext, const VString &inModuleName)
{
	xbox_assert(inModuleName.EqualToString(kModuleName, true));

	VJSTLSClass::Class();
	return VJSTLSClass::CreateInstance(inContext, NULL);
}

void VJSTLSClass::GetDefinition (ClassDefinition &outDefinition)
{
	static inherited::StaticFunction functions[] =
	{
		{	"createServer",		js_callStaticFunction<_createServer>,		JS4D::PropertyAttributeDontDelete	},
		{	"createServerSync",	js_callStaticFunction<_createServerSync>,	JS4D::PropertyAttributeDontDelete	},	
		{	"connect",			js_callStaticFunction<_connect>,			JS4D::PropertyAttributeDontDelete	},
		{	"connectSync",		js_callStaticFunction<_connectSync>,		JS4D::PropertyAttributeDontDelete	},
		{	0,					0,											0									},
	};

    outDefinition.className			= "tls";    
	outDefinition.staticFunctions	= functions;		
}

void VJSTLSClass::_createServer (XBOX::VJSParms_callStaticFunction &ioParms, void *)
{
	VJSNetClass::CreateServer(ioParms, false, true);
}

void VJSTLSClass::_createServerSync (XBOX::VJSParms_callStaticFunction &ioParms, void *)
{
	VJSNetClass::CreateServer(ioParms, true, true);
}

void VJSTLSClass::_connect(XBOX::VJSParms_callStaticFunction &ioParms, void *)
{
	VJSNetClass::ConnectSocket(ioParms, false, true);
}

void VJSTLSClass::_connectSync(XBOX::VJSParms_callStaticFunction &ioParms, void *)
{
	VJSNetClass::ConnectSocket(ioParms, true, true);
}
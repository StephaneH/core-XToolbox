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
#ifndef __VJS_NET__
#define __VJS_NET__

#include "VJSClass.h"
#include "VJSValue.h"

#include "VJSWorker.h"
#include "VJSBuffer.h"

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API VJSNetClass : public XBOX::VJSClass<VJSNetClass, void>
{
public:

	static const XBOX::VString	kModuleName;	// "net"

	static void		RegisterModule ();

	static void		GetDefinition (ClassDefinition &outDefinition);
	static void		CreateServer (XBOX::VJSParms_callStaticFunction &ioParms, bool inIsSync, bool inIsSSL);	
	static void		ConnectSocket (XBOX::VJSParms_callStaticFunction &ioParms, bool inIsSync, bool inIsSSL);
	
private:

	static XBOX::VJSObject	_ModuleConstructor (const VJSContext &inContext, const VString &inModuleName);

	typedef XBOX::VJSClass<VJSNetClass, VObject>	inherited;
	
	static void		_Initialize (const XBOX::VJSParms_initialize &inParms, void *);

	static void		_createServer (XBOX::VJSParms_callStaticFunction &ioParms, void *);
	static void		_createConnection (XBOX::VJSParms_callStaticFunction &ioParms, void *);
	static void		_connect (XBOX::VJSParms_callStaticFunction &ioParms, void *);

	static void		_createServerSync (XBOX::VJSParms_callStaticFunction &ioParms, void *);
	static void		_createConnectionSync (XBOX::VJSParms_callStaticFunction &ioParms, void *);
	static void		_connectSync (XBOX::VJSParms_callStaticFunction &ioParms, void *);

	static void		_isIP (XBOX::VJSParms_callStaticFunction &ioParms, void *);
	static void		_isIPv4 (XBOX::VJSParms_callStaticFunction &ioParms, void *);
	static void		_isIPv6 (XBOX::VJSParms_callStaticFunction &ioParms, void *);

	static uLONG	_isIPAddress (XBOX::VJSParms_callStaticFunction &ioParms);
};

END_TOOLBOX_NAMESPACE

#endif
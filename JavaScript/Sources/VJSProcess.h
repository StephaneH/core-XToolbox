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
#ifndef __VJS_PROCESS__
#define __VJS_PROCESS__

#include "VJSClass.h"
#include "VJSValue.h"
#include "VJSEventEmitter.h"

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API VJSProcess : public XBOX::VJSClass<VJSProcess, VJSEventEmitter>
{
public:
	typedef XBOX::VJSClass<VJSProcess, VJSEventEmitter> inherited;

	static	void			Initialize ( const XBOX::VJSParms_initialize& inParms, VJSEventEmitter *inEventEmitter);
	static	void			Finalize ( const XBOX::VJSParms_finalize& inParms, VJSEventEmitter *inEventEmitter);
	static	void			GetDefinition( ClassDefinition& outDefinition);

	static	void			_getAccessor( XBOX::VJSParms_getProperty& ioParms, XBOX::VJSGlobalObject* inGlobalObject);

	// Functions

	// Properties
	static	void			_Version		( XBOX::VJSParms_getProperty& ioParms, VJSEventEmitter *);
	static	void			_BuildNumber	( XBOX::VJSParms_getProperty& ioParms, VJSEventEmitter *);
	static	void			_ClosingFlag	( XBOX::VJSParms_getProperty& ioParms, VJSEventEmitter *);
	static	void			_PID			( XBOX::VJSParms_getProperty& ioParms, VJSEventEmitter *);
	static	void			_ProductName	( XBOX::VJSParms_getProperty& ioParms, VJSEventEmitter *);
	static	void			_ProductType	( XBOX::VJSParms_getProperty& ioParms, VJSEventEmitter *);
	static	void			_Env			( XBOX::VJSParms_getProperty& ioParms, VJSEventEmitter *);

private:
};


class XTOOLBOX_API VJSOS : public XBOX::VJSClass<VJSOS, void>
{
public:
	
	static void	GetDefinition( ClassDefinition& outDefinition);
	
private:
		
	struct Address {
		
		XBOX::VString	fInterfaceName, fIPAddress;
		bool			fIsIPv6, fIsInternal;	
		
	};

	static void	_Initialize (const XBOX::VJSParms_initialize &inParms, void *);
	static void	_Finalize (const XBOX::VJSParms_finalize &inParms, void *);

	// Functions

	static void	_Type (XBOX::VJSParms_callStaticFunction &ioParms, void *);
	static void	_NetworkInterfaces (XBOX::VJSParms_callStaticFunction &ioParms, void *);
	static void _getProxy(XBOX::VJSParms_callStaticFunction &ioParms, void *);

	// Properties

	static void	_IsMac (XBOX::VJSParms_getProperty &ioParms, void *);
	static void	_IsWindows (XBOX::VJSParms_getProperty &ioParms, void *);
	static void	_IsLinux (XBOX::VJSParms_getProperty &ioParms, void *);
};

END_TOOLBOX_NAMESPACE

#endif


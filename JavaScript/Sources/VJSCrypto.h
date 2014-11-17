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
#ifndef __VJSCrypto__
#define __VJSCrypto__

#include "VJSClass.h"
#include "VJSValue.h"

BEGIN_TOOLBOX_NAMESPACE


class XTOOLBOX_API VJSCryptoClass : public XBOX::VJSClass<VJSCryptoClass, void>
{
public:

	static const XBOX::VString	kModuleName;	// "crypto"

	static void		RegisterModule ();

	static void		GetDefinition (ClassDefinition &outDefinition);
	
private:

	static XBOX::VJSObject	_ModuleConstructor (const VJSContext &inContext, const VString &inModuleName);

	typedef XBOX::VJSClass<VJSCryptoClass, void>	inherited;

	static	void		_Initialize( const XBOX::VJSParms_initialize &inParms, void *);

	static	void		_createHash( XBOX::VJSParms_callStaticFunction &ioParms, void *);
	static	void		_getHashes( VJSParms_callStaticFunction& ioParms, void *);
	static	void		_createVerify( XBOX::VJSParms_callStaticFunction &ioParms, void *);
};


END_TOOLBOX_NAMESPACE


#endif
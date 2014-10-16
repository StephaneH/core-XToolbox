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
#ifndef __VJSRuntime_progressIndicator__
#define __VJSRuntime_progressIndicator__

#include "JS4D.h"
#include "VJSClass.h"

BEGIN_TOOLBOX_NAMESPACE


class XTOOLBOX_API VJSProgressIndicator : public VJSClass<VJSProgressIndicator, XBOX::VProgressIndicator>
{
public:
	typedef VJSClass<VJSProgressIndicator, XBOX::VProgressIndicator> inherited;
	
	static	void			Initialize( const VJSParms_initialize& inParms, XBOX::VProgressIndicator* inProgress);
	static	void			Finalize( const VJSParms_finalize& inParms, XBOX::VProgressIndicator* inProgress);
	static	void			GetDefinition( ClassDefinition& outDefinition);
	
	static void _SubSession(VJSParms_callStaticFunction& ioParms, XBOX::VProgressIndicator* inProgress); // SubSession( { number : maxvalue, string : sessiontext, bool : caninterrupt = false } )
	static void _EndSession(VJSParms_callStaticFunction& ioParms, XBOX::VProgressIndicator* inProgress); // EndSession()
	static void _SetValue(VJSParms_callStaticFunction& ioParms, XBOX::VProgressIndicator* inProgress); // bool SetValue( number : currentvalue)
	static void _SetMax(VJSParms_callStaticFunction& ioParms, XBOX::VProgressIndicator* inProgress); // SetMax(number : newmax)
	static void _IncValue(VJSParms_callStaticFunction& ioParms, XBOX::VProgressIndicator* inProgress); // bool: IncValue()
	static void _SetMessage(VJSParms_callStaticFunction& ioParms, XBOX::VProgressIndicator* inProgress); // SetMessage(string : sessionMessage)
	static void _stop(VJSParms_callStaticFunction& ioParms, XBOX::VProgressIndicator* inProgress); // stop({sessionnum})

	
};


// --------------------------------------------------------------------


END_TOOLBOX_NAMESPACE

#endif
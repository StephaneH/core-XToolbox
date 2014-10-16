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

#include "VJSValue.h"
#include "VJSContext.h"
#include "VJSClass.h"
#include "VJSJSON.h"


#include "VJSRuntime_progressIndicator.h"

USING_TOOLBOX_NAMESPACE



void VJSProgressIndicator::Initialize( const VJSParms_initialize& inParms, VProgressIndicator* inProgress)
{
	inProgress->Retain();
}


void VJSProgressIndicator::Finalize( const VJSParms_finalize& inParms, VProgressIndicator* inProgress)
{
	for (sLONG i = 0, nb = inProgress->GetSessionCount(); i < nb; i++)
	{
		inProgress->EndSession();
	}
	inProgress->Release();
}



void VJSProgressIndicator::_SubSession(VJSParms_callStaticFunction& ioParms, VProgressIndicator* inProgress)
{
	VString sessiontile;
	bool caninterrupt = false;
	Real maxvalue = -1;
	
	bool ok = true;

	if (ioParms.IsNumberParam(1))
		ioParms.GetRealParam(1, &maxvalue);
	else
	{
		ok = false;
		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
	}
	
	if (ioParms.IsStringParam(2))
		ioParms.GetStringParam(2, sessiontile);
	else
	{
		ok = false;
		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2");
	}

	caninterrupt = ioParms.GetBoolParam( 3, L"Can Interrupt", "Cannot Interrupt");
		
	if (ok)
		inProgress->BeginSession((sLONG8)maxvalue, sessiontile, caninterrupt);
}


void VJSProgressIndicator::_EndSession(VJSParms_callStaticFunction& ioParms, VProgressIndicator* inProgress)
{
	inProgress->EndSession();
}

void VJSProgressIndicator::_stop(VJSParms_callStaticFunction& ioParms, VProgressIndicator* inProgress)
{
	inProgress->UserAbort();
}

void VJSProgressIndicator::_SetValue(VJSParms_callStaticFunction& ioParms, VProgressIndicator* inProgress)
{
	Real newvalue = 0;
	bool result = true;
	
	if (ioParms.IsNumberParam(1))
	{
		ioParms.GetRealParam(1, &newvalue);
		result = inProgress->Progress((sLONG8)newvalue);
	}
	else
	{
		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
	}
		
	if (inProgress->IsInterrupted())
		result = false;
	ioParms.ReturnBool(result);
}


void VJSProgressIndicator::_SetMax(VJSParms_callStaticFunction& ioParms, VProgressIndicator* inProgress)
{
	Real newvalue = 0;
	if (ioParms.IsNumberParam(1))
	{
		ioParms.GetRealParam(1, &newvalue);
		inProgress->SetMaxValue((sLONG8)newvalue);
	}
	else
	{
		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
	}
}


void VJSProgressIndicator::_IncValue(VJSParms_callStaticFunction& ioParms, VProgressIndicator* inProgress)
{
	bool cont = inProgress->Increment(1);
	if (inProgress->IsInterrupted())
		cont = false;
	ioParms.ReturnBool(cont);
}


void VJSProgressIndicator::_SetMessage(VJSParms_callStaticFunction& ioParms, VProgressIndicator* inProgress)
{
	VString sessiontile;
	
	if (ioParms.IsStringParam(1))
	{
		ioParms.GetStringParam(1, sessiontile);
		inProgress->SetMessage(sessiontile);
	}
	else
		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
}




void VJSProgressIndicator::GetDefinition( ClassDefinition& outDefinition)
{
	static inherited::StaticFunction functions[] = 
	{
		{ "subSession", js_callStaticFunction<_SubSession>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "endSession", js_callStaticFunction<_EndSession>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "setValue", js_callStaticFunction<_SetValue>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "setMax", js_callStaticFunction<_SetMax>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "incValue", js_callStaticFunction<_IncValue>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "setMessage", js_callStaticFunction<_SetMessage>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "stop", js_callStaticFunction<_stop>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ 0, 0, 0}
	};
	
	outDefinition.className = "ProgressIndicator";
	outDefinition.initialize = js_initialize<Initialize>;
	outDefinition.finalize = js_finalize<Finalize>;
	outDefinition.staticFunctions = functions;
}



//======================================================


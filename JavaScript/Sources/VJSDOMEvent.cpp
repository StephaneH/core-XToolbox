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

#include "VJSDOMEvent.h"

#include "VJSContext.h"
#include "VJSGlobalClass.h"

USING_TOOLBOX_NAMESPACE

VJSDOMEvent::VJSDOMEvent (const XBOX::VString &inType, XBOX::VJSObject &inTarget, const XBOX::VTime &inTimeStamp)
: fTarget(inTarget), fCurrentTarget(inTarget)
{
	fType = inType;
	fEventPhase = kPhaseAtTarget;
	fBubbles = fCancelable = fDefaultPrevented = false;
	fTrusted = true;

	XBOX::VTime	unixStartTime;

	unixStartTime.FromUTCTime(1970, 1, 1, 0, 0, 0, 0);
	fTimeStamp = inTimeStamp.GetMilliseconds() - unixStartTime.GetMilliseconds();
	if (fTimeStamp < 0)

		fTimeStamp = 0;
}

void VJSDOMEventClass::GetDefinition (ClassDefinition &outDefinition)
{
	static inherited::StaticValue values[] = 
	{ 
		{ "type",				js_getProperty<_GetProperty>,	0,	JS4D::PropertyAttributeReadOnly	},
		{ "target",				js_getProperty<_GetProperty>,	0,	JS4D::PropertyAttributeReadOnly	},
		{ "currentTarget",		js_getProperty<_GetProperty>,	0,	JS4D::PropertyAttributeReadOnly	},
		{ "eventPhase",			js_getProperty<_GetProperty>,	0,	JS4D::PropertyAttributeReadOnly	},
		{ "bubbles",			js_getProperty<_GetProperty>,	0,	JS4D::PropertyAttributeReadOnly	},
		{ "cancelable",			js_getProperty<_GetProperty>,	0,	JS4D::PropertyAttributeReadOnly	},
		{ "timeStamp",			js_getProperty<_GetProperty>,	0,	JS4D::PropertyAttributeReadOnly	},
		{ "defaultPrevented",	js_getProperty<_GetProperty>,	0,	JS4D::PropertyAttributeReadOnly	},
		{ "trusted",			js_getProperty<_GetProperty>,	0,	JS4D::PropertyAttributeReadOnly	},
		{ 0, 0, 0, 0 },
		
	};

    static inherited::StaticFunction functions[] =
	{
		{ "stopPropagation",			js_callStaticFunction<_StopPropagation>,			JS4D::PropertyAttributeNone	},
		{ "preventDefault",				js_callStaticFunction<_PreventDefault>,				JS4D::PropertyAttributeNone	},
		{ "initEvent",					js_callStaticFunction<InitEvent>,					JS4D::PropertyAttributeNone	},		
		{ "stopImmediatePropagation",	js_callStaticFunction<_StopImmediatePropagation>,	JS4D::PropertyAttributeNone	},
		{ 0, 0, 0 },
	};	
			
    outDefinition.className			= "Event";
    outDefinition.staticValues		= values;
	outDefinition.staticFunctions	= functions;
    outDefinition.initialize		= js_initialize<_Initialize>;
    outDefinition.finalize			= js_finalize<_Finalize>;
}

void VJSDOMEventClass::InitEvent (VJSParms_callStaticFunction &ioParms, VJSDOMEvent *inDOMEvent)
{
	xbox_assert(inDOMEvent != NULL);

	if (ioParms.CountParams() >= 1 && ioParms.IsStringParam(1))

		ioParms.GetStringParam(1, inDOMEvent->fType);

	if (ioParms.CountParams() >= 2 && ioParms.IsBooleanParam(2))

		ioParms.GetBoolParam(2, &inDOMEvent->fBubbles);

	if (ioParms.CountParams() >= 3 && ioParms.IsBooleanParam(3))

		ioParms.GetBoolParam(3, &inDOMEvent->fCancelable);

	inDOMEvent->fTrusted = false;	// Never to be trusted, see section 4.1 of specification.
}

void VJSDOMEventClass::_Initialize (const XBOX::VJSParms_initialize &inParms, VJSDOMEvent *inDOMEvent)
{
	XBOX::VJSContext	context	= inParms.GetContext();
	XBOX::VJSObject		object	= inParms.GetObject();

	object.SetContext(context);
	object.SetProperty("CAPTURING_PHASE", (sLONG) 1, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
	object.SetProperty("AT_TARGET", (sLONG) 2, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
	object.SetProperty("BUBBLING_PHASE", (sLONG) 3, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
}

void VJSDOMEventClass::_Finalize (const XBOX::VJSParms_finalize &inParms, VJSDOMEvent *inDOMEvent)
{
	xbox_assert(inDOMEvent != NULL);

	delete inDOMEvent;
}

void VJSDOMEventClass::_GetProperty (XBOX::VJSParms_getProperty &ioParms, VJSDOMEvent *inDOMEvent)
{
	XBOX::VString	attributeName;
	
	ioParms.GetPropertyName(attributeName);
	if (attributeName.EqualToUSASCIICString("type"))

		ioParms.ReturnString(inDOMEvent->fType);
		
	else if (attributeName.EqualToUSASCIICString("target"))

		ioParms.ReturnValue(inDOMEvent->fTarget);

	else if (attributeName.EqualToUSASCIICString("currentTarget"))

		ioParms.ReturnValue(inDOMEvent->fCurrentTarget);

	else if (attributeName.EqualToUSASCIICString("eventPhase"))

		ioParms.ReturnNumber(inDOMEvent->fEventPhase);

	else if (attributeName.EqualToUSASCIICString("bubbles"))

		ioParms.ReturnBool(inDOMEvent->fBubbles);

	else if (attributeName.EqualToUSASCIICString("cancelable"))

		ioParms.ReturnBool(inDOMEvent->fCancelable);
	
	else if (attributeName.EqualToUSASCIICString("timeStamp"))

		ioParms.ReturnNumber(inDOMEvent->fTimeStamp);
	
	else if (attributeName.EqualToUSASCIICString("defaultPrevented"))

		ioParms.ReturnBool(inDOMEvent->fDefaultPrevented);

	else if (attributeName.EqualToUSASCIICString("trusted"))

		ioParms.ReturnBool(inDOMEvent->fTrusted);

	else

		ioParms.ReturnUndefinedValue();
}

void VJSDOMEventClass::_StopPropagation (VJSParms_callStaticFunction &ioParms, VJSDOMEvent *inDOMEvent)
{
	xbox_assert(inDOMEvent != NULL);

	// Do nothing, not supported.
}

void VJSDOMEventClass::_PreventDefault (VJSParms_callStaticFunction &ioParms, VJSDOMEvent *inDOMEvent)
{
	xbox_assert(inDOMEvent != NULL);

	inDOMEvent->fDefaultPrevented = true;
}

void VJSDOMEventClass::_StopImmediatePropagation (VJSParms_callStaticFunction &ioParms, VJSDOMEvent *inDOMEvent)
{
	xbox_assert(inDOMEvent != NULL);

	// Do nothing, not supported.
}

void VJSDOMMessageEventClass::GetDefinition (ClassDefinition &outDefinition)
{
	VJSDOMEventClass::GetDefinition(outDefinition);
	outDefinition.className = "MessageEvent";
}

XBOX::VJSObject	VJSDOMMessageEventClass::NewInstance (VJSContext &inContext, XBOX::VJSObject &inTarget, const XBOX::VTime &inTimeStamp, XBOX::VJSValue &inData)
{
	VJSDOMEvent		*domEvent	= new VJSDOMEvent("message", inTarget, inTimeStamp);
	XBOX::VJSObject	newObject	= CreateInstance(inContext, domEvent);
	
	// Add data attribute.

	newObject.SetProperty("data", inData, JS4D::PropertyAttributeDontDelete);

	return newObject;
}

void VJSDOMErrorEventClass::GetDefinition (ClassDefinition &outDefinition)
{
	VJSDOMEventClass::GetDefinition(outDefinition);
	outDefinition.className = "ErrorEvent";
}

XBOX::VJSObject	VJSDOMErrorEventClass::NewInstance (
	VJSContext &inContext, XBOX::VJSObject &inTarget, const XBOX::VTime &inTimeStamp, 
	const XBOX::VString &inMessage, const XBOX::VString &inFileName, sLONG inLineNumber)
{
	VJSDOMEvent		*domEvent	= new VJSDOMEvent("error", inTarget, inTimeStamp);
	XBOX::VJSObject	newObject	= CreateInstance(inContext, domEvent);

	domEvent->fCancelable = true;

	// Add attributes.

	if (inMessage.GetLength())

		newObject.SetProperty("message", inMessage, JS4D::PropertyAttributeReadOnly);

	if (inFileName.GetLength())

		newObject.SetProperty("filename", inFileName, JS4D::PropertyAttributeReadOnly);

	if (inLineNumber >= 0)

		newObject.SetProperty("lineno", (sLONG) inLineNumber, JS4D::PropertyAttributeReadOnly);

	// Remove initEvent() method and add initErrorEvent() instead.

	VJSException	except;
	newObject.DeleteProperty("initEvent",except);

	XBOX::VJSObject	initErrorEventObject(inContext);

	initErrorEventObject.MakeCallback(XBOX::js_callback<VJSDOMEvent, _InitErrorEvent>);
	newObject.SetProperty("initErrorEvent", initErrorEventObject);

	return newObject;
}

void VJSDOMErrorEventClass::_InitErrorEvent (VJSParms_callStaticFunction &ioParms, VJSDOMEvent *inDOMEvent)
{
	VJSDOMEventClass::InitEvent(ioParms, inDOMEvent);

	XBOX::VJSContext	context	= ioParms.GetContext();
	XBOX::VJSObject		object	= ioParms.GetObject();
	XBOX::VString		string;
	sLONG				lineNumber;

	object.SetContext(context);
	if (ioParms.CountParams() >= 4 && ioParms.IsStringParam(4)) {

		ioParms.GetStringParam(4, string);
		object.SetProperty("message", string, JS4D::PropertyAttributeReadOnly);

	}

	if (ioParms.CountParams() >= 5 && ioParms.IsBooleanParam(5)) {

		ioParms.GetStringParam(5, string);
		object.SetProperty("filename", string, JS4D::PropertyAttributeReadOnly);

	}

	if (ioParms.CountParams() >= 6 && ioParms.IsNumberParam(6)) {

		ioParms.GetLongParam(6, &lineNumber);
		object.SetProperty("lineno", (sLONG) lineNumber, JS4D::PropertyAttributeReadOnly);

	}
}
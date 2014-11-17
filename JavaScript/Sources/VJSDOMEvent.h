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
#ifndef __VJS_DOM_EVENT__
#define __VJS_DOM_EVENT__

#include "JS4D.h"
#include "VJSClass.h"
#include "VJSValue.h"

// Specifications:
//
//	DOM Event		http://www.w3.org/TR/DOM-Level-3-Events/
//	HTML5			http://www.w3.org/TR/html5/
//	Web Workers		http://www.w3.org/TR/workers/
//  Web Storage		http://www.w3.org/TR/webstorage/

// For server side events, there should be only one phase : "at target" as there is no element
// to capture from or bubble up to. Hence attribute currentTarget is always the same as target.
// Propagation is unsupported.

BEGIN_TOOLBOX_NAMESPACE

// DOM events should be implemented as sub-classes of VJSDOMEvent. But "simpler" events (containing
// only a few more attributes) can be implemented as "generic" VJSDOMEventClass objects, just add 
// the additional properties thereafter.

class XTOOLBOX_API VJSDOMEvent : public XBOX::VObject
{
public:

	// DOM event phase.

	static const sLONG	kPhaseCapturing	= 1;	// Calling callbacks from outer to inner elements.
	static const sLONG	kPhaseAtTarget	= 2;	// At the target callback of the event.
	static const sLONG	kPhaseBubbling	= 3;	// Calling callbacks from inner to outer elements.

	// DOM event attributes.

	XBOX::VString		fType;
	XBOX::VJSObject		fTarget;
	XBOX::VJSObject		fCurrentTarget;
	sLONG				fEventPhase;
	bool				fBubbles;
	bool				fCancelable;
	sLONG8				fTimeStamp;
	bool				fDefaultPrevented;
	bool				fTrusted;

	// By default, created event is "at target" phase, cannot bubble, not cancelable, default prevented is false.

						VJSDOMEvent	(const XBOX::VString &inType, XBOX::VJSObject &inTarget, const XBOX::VTime &inTimeStamp);
};

// "Generic" class implementing the Event interface.

class XTOOLBOX_API VJSDOMEventClass : public XBOX::VJSClass<VJSDOMEventClass, VJSDOMEvent>
{
public:

	// For classes implementing the Event interface, use GetDefinition() as a basis and patch it.

	static void	GetDefinition (ClassDefinition &outDefinition);
	static void	InitEvent (VJSParms_callStaticFunction &ioParms, VJSDOMEvent *inDOMEvent);	
	
private:

	typedef XBOX::VJSClass<VJSDOMEventClass, VJSDOMEvent> inherited;

	// "Generic" methods for event class construction. 

	static void _Initialize (const XBOX::VJSParms_initialize &inParms, VJSDOMEvent *inDOMEvent);
	static void _Finalize (const XBOX::VJSParms_finalize &inParms, VJSDOMEvent *inDOMEvent);
	static void	_GetProperty (XBOX::VJSParms_getProperty &ioParms, VJSDOMEvent *inDOMEvent);
		
	// Event methods.

	static void	_StopPropagation (VJSParms_callStaticFunction &ioParms, VJSDOMEvent *inDOMEvent);
	static void _PreventDefault (VJSParms_callStaticFunction &ioParms, VJSDOMEvent *inDOMEvent);	
	static void	_StopImmediatePropagation (VJSParms_callStaticFunction &ioParms, VJSDOMEvent *inDOMEvent);	
};	

// Web worker message event, implemented as an Event with a "data" attribute (containing message).

class XTOOLBOX_API VJSDOMMessageEventClass : public XBOX::VJSClass<VJSDOMMessageEventClass, VJSDOMEvent>
{
public:

	static void				GetDefinition (ClassDefinition &outDefinition);

	// Do not use CreateInstance(), but always use NewInstance().
		
	static XBOX::VJSObject	NewInstance (VJSContext &inContext, XBOX::VJSObject &inTarget, const XBOX::VTime &inTimeStamp, XBOX::VJSValue &inData);

private:

	typedef XBOX::VJSClass<VJSDOMMessageEventClass, VJSDOMEvent> inherited;
};

// DOM error event, implemented as described in section "4.6 Runtime script errors" of Web worker specification.
// Error events are mentionned in both HTML5 and DOM event specifications, but without any *actual* specification!

class XTOOLBOX_API VJSDOMErrorEventClass : public XBOX::VJSClass<VJSDOMErrorEventClass, VJSDOMEvent>
{
public:

	static void				GetDefinition (ClassDefinition &outDefinition);

	// Do not use CreateInstance(), but always use NewInstance() instead.

	// If a message or filename string is empty, attribute is not set. If line number is negative, it is not set.
	// This will allow initErrorEvent() to set them, otherwise they are read-only.
		
	static XBOX::VJSObject	NewInstance (VJSContext &inContext, XBOX::VJSObject &inTarget, const XBOX::VTime &inTimeStamp, 
										const XBOX::VString &inMessage = "", const XBOX::VString &inFileName = "", sLONG inLineNumber = -1);

private:

	typedef XBOX::VJSClass<VJSDOMErrorEventClass, VJSDOMEvent> inherited;

	static void				_InitErrorEvent (VJSParms_callStaticFunction &ioParms, VJSDOMEvent *inDOMEvent);
};

// Web storage notification event.
/*
class XTOOLBOX_API VJSDOMStorageEventClass : public XBOX::VJSClass<VJSDOMStorageEventClass, VJSDOMEvent>
{
public:

	static void				GetDefinition (ClassDefinition &outDefinition);
	static XBOX::VJSObject	NewInstance (VJSContext &inContext, XBOX::VJSObject &inTarget, const XBOX::VTime &inTimeStamp, 
										const XBOX::VString &inKey, 
										XBOX::VJSObject inOldValue, XBOX::VJSObject inNewObject,										
										const XBOX::VString &inURL, XBOX::VJSObject inStorageObject);

private:

	typedef XBOX::VJSClass<VJSDOMStorageEventClass, VJSDOMEvent> inherited;

	static void				_InitStorageEvent (VJSParms_callStaticFunction &ioParms, VJSDOMEvent *inDOMEvent);
};
*/
END_TOOLBOX_NAMESPACE

#endif
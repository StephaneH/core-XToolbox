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
#ifndef __VJS_EVENT_EMITTER__
#define __VJS_EVENT_EMITTER__

#include <list>
#include <map>
#include <vector>

#include "VJSClass.h"
#include "VJSValue.h"
#include "VJSWorker.h"

// The 'newListener' event (each time addListener() is called) is not handled.

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API VJSEventEmitter : public XBOX::VObject, public XBOX::IRefCountable
{
friend class VJSEventEmitterClass;

public:

						VJSEventEmitter();
	virtual				~VJSEventEmitter();

	void				AddListener (VJSWorker *inWorker, const XBOX::VString &inEventName, XBOX::VJSObject &inCallback, bool inOnce);
	bool				HasListener (const XBOX::VString &inEventName);
	void				Emit (XBOX::VJSContext inContext, const XBOX::VString &inEventName, std::vector<XBOX::VJSValue> *inArguments);

private:

	class SListener {
	public:
								SListener (XBOX::VJSObject &inCallback) 
								: fOnce(true)
								, fContext((JS4D::ContextRef) NULL), fCallback(inCallback) {}
		bool					fOnce;
		XBOX::VJSContext		fContext;
		XBOX::VJSObject			fCallback;		

	};

	class CCompare {

	public:
		
		bool operator() (const XBOX::VString &inStringA, const XBOX::VString &inStringB) const
		{ 
			return inStringA.CompareToString(inStringB, true) == XBOX::CR_SMALLER;				
		}

	};

	typedef	std::list<SListener>								SListenerList;
	typedef std::map<XBOX::VString, SListenerList, CCompare>	SEventMap;

	static const sLONG	kDefaultMaximumListeners	= 10;

	sLONG				fMaximumListeners;
	sLONG				fNumberListeners;
	SEventMap			fAllListeners;

	static void			_addListener (XBOX::VJSParms_callStaticFunction &ioParms, VJSEventEmitter *inEventEmitter);
	static void			_on (XBOX::VJSParms_callStaticFunction &ioParms, VJSEventEmitter *inEventEmitter);
	static void			_once (XBOX::VJSParms_callStaticFunction &ioParms, VJSEventEmitter *inEventEmitter);
	static void			_removeListener (XBOX::VJSParms_callStaticFunction &ioParms, VJSEventEmitter *inEventEmitter);
	static void			_removeAllListeners (XBOX::VJSParms_callStaticFunction &ioParms, VJSEventEmitter *inEventEmitter);
	static void			_setMaxListeners (XBOX::VJSParms_callStaticFunction &ioParms, VJSEventEmitter *inEventEmitter);
	static void			_listeners (XBOX::VJSParms_callStaticFunction &ioParms, VJSEventEmitter *inEventEmitter);
	static void			_emit (XBOX::VJSParms_callStaticFunction &ioParms, VJSEventEmitter *inEventEmitter);

	static void			_add (XBOX::VJSParms_callStaticFunction &ioParms, VJSEventEmitter *inEventEmitter, bool inOnce);

	// Retrieve a list of callback(s) for the given event name. If inRemoveOnce is true, then one-time listener(s) are removed during 
	// the building of the list. 
	
	sLONG				_getCallbacks (const XBOX::VString &inEventName, std::list<XBOX::VJSObject> *outList, bool inRemoveOnce);

};

class XTOOLBOX_API VJSEventEmitterClass : public XBOX::VJSClass<VJSEventEmitterClass, VJSEventEmitter>
{
public:

	static const XBOX::VString	kModuleName;	// "events"

	static void					RegisterModule ();
	static void					GetDefinition (ClassDefinition &outDefinition);
	static void					_Construct (XBOX::VJSParms_construct &ioParms);
		
private:

	static XBOX::VJSObject		_ModuleConstructor (const VJSContext &inContext, const VString &inModuleName);

	static void					_Initialize (const XBOX::VJSParms_initialize &inParms, VJSEventEmitter *inEventEmitter);
	static void					_Finalize (const XBOX::VJSParms_finalize &inParms, VJSEventEmitter *inEventEmitter);

#if !USE_V8_ENGINE

	static void					_CallAsFunction (VJSParms_callAsFunction &ioParms);

#endif

	static XBOX::VJSObject		_NewInstance (const XBOX::VJSContext &inContext);	
};

END_TOOLBOX_NAMESPACE

#endif
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

#include "VJSEventEmitter.h"

#include "VJSContext.h"
#include "VJSGlobalClass.h"
#include "VJSEvent.h"

USING_TOOLBOX_NAMESPACE

void VJSEventEmitter::AddListener (VJSWorker *inWorker, const XBOX::VString &inEventName, XBOX::VJSObject &inCallback, bool inOnce)
{
	xbox_assert(inWorker != NULL);

	// Note: That it is possible to add a same function several times for a single event.

	SListener	newListener(inCallback);

	inCallback.Protect();

	newListener.fOnce = inOnce;
	//newListener.fCallback = inCallback;
	newListener.fContext = inCallback.GetContext();
	xbox_assert(newListener.fCallback.HasRef());

	if (fAllListeners.find(inEventName) == fAllListeners.end()) {

		SListenerList	newListenerList;

		newListenerList.push_back(newListener);
		fAllListeners.insert(std::pair<XBOX::VString,SListenerList>(inEventName, newListenerList));

	} else

		fAllListeners[inEventName].push_back(newListener);

	if (++fNumberListeners > fMaximumListeners) {

		// TODO: See what NodeJS does exactly (console.warning("too many listeners") ?).

	}

	// Queue a "newListener" event.

	inWorker->QueueEvent(VJSNewListenerEvent::Create(this, inEventName, newListener.fCallback));
}

bool VJSEventEmitter::HasListener (const XBOX::VString &inEventName)
{
	if (fAllListeners.find(inEventName) == fAllListeners.end())

		return false;

	else 

		return !fAllListeners[inEventName].empty();
}

void VJSEventEmitter::Emit (XBOX::VJSContext inContext, const XBOX::VString &inEventName, std::vector<XBOX::VJSValue> *inArguments)
{
	xbox_assert(inArguments != NULL);

	std::list<XBOX::VJSObject>	callbackList;
	
	if (_getCallbacks(inEventName, &callbackList, true)) {

		XBOX::VJSObject							globalObject	= inContext.GetGlobalObject();
		std::list<XBOX::VJSObject>::iterator	i;

		for (i = callbackList.begin(); i != callbackList.end(); i++) {

			XBOX::VJSObject	functionObject(*i);

			functionObject.SetContext(inContext);
			globalObject.CallFunction(functionObject, inArguments, NULL);

		}

	}
}

VJSEventEmitter::VJSEventEmitter()
{
	fMaximumListeners = VJSEventEmitter::kDefaultMaximumListeners;
	fNumberListeners = 0;
	fAllListeners.clear();
}

VJSEventEmitter::~VJSEventEmitter ()
{
	SEventMap::iterator		it1;
	SListenerList::iterator	it2;

	for (it1 = fAllListeners.begin(); it1 != fAllListeners.end(); it1++) {

		for (it2 = fAllListeners[it1->first].begin(); it2 != fAllListeners[it1->first].end(); it2++) {

			;	

		// Do not do unprotect, the heap has always been cleaned-up by garbage collector.
			
		//	XBOX::JS4D::UnprotectValue(it2->fContext, it2->fCallback);

		}

	}

}

void VJSEventEmitter::_addListener (XBOX::VJSParms_callStaticFunction &ioParms, VJSEventEmitter *inEventEmitter)
{
	xbox_assert(inEventEmitter != NULL);
	_add(ioParms, inEventEmitter, false);
}

void VJSEventEmitter::_on (XBOX::VJSParms_callStaticFunction &ioParms, VJSEventEmitter *inEventEmitter)
{
	xbox_assert(inEventEmitter != NULL);
	_addListener(ioParms, inEventEmitter);
}

void VJSEventEmitter::_once (XBOX::VJSParms_callStaticFunction &ioParms, VJSEventEmitter *inEventEmitter)
{
	xbox_assert(inEventEmitter != NULL);
	_add(ioParms, inEventEmitter, true);
}

void VJSEventEmitter::_removeListener (XBOX::VJSParms_callStaticFunction &ioParms, VJSEventEmitter *inEventEmitter)
{
	xbox_assert(inEventEmitter != NULL);

	XBOX::VString	eventName;
	XBOX::VJSObject	callbackObject(ioParms.GetContext());

	if (ioParms.CountParams() < 2
	|| !ioParms.IsStringParam(1) || !ioParms.GetStringParam(1, eventName)
	|| !ioParms.IsObjectParam(2) || !ioParms.GetParamObject(2, callbackObject) || !callbackObject.IsFunction()) {

		XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
		return;

	}

	// If the event has no listener at all, or if the callback object isn't found in the listener list, do nothing. 

	if (inEventEmitter->fAllListeners.find(eventName) == inEventEmitter->fAllListeners.end())

		return;

	// To remove callback in a LIFO way, loop twice to find the last one inserted (reverse_iterator doesn't work with erase()).

	XBOX::JS4D::ObjectRef	callbackObjectRef;
	SListenerList			*listenerList;
	SListenerList::iterator	i;
	sLONG					j, k;

	listenerList = &inEventEmitter->fAllListeners[eventName];
	for (i = listenerList->begin(), j = 0; i != listenerList->end(); i++) 

		if (i->fCallback == callbackObject) 

			j++;

	if (j)

		for (i = listenerList->begin(), k = 0; i != listenerList->end(); i++) 

			if (++k == j) {

				i->fCallback.Unprotect();
				listenerList->erase(i);
				xbox_assert(inEventEmitter->fNumberListeners);
				inEventEmitter->fNumberListeners--;
				break;

			}
}

void VJSEventEmitter::_removeAllListeners (XBOX::VJSParms_callStaticFunction &ioParms, VJSEventEmitter *inEventEmitter)
{
	xbox_assert(inEventEmitter != NULL);

	SEventMap::iterator		it1;
	SListenerList::iterator	it2;

	// If no event name argument, remove all listeners for all events.

	if (!ioParms.CountParams()) {

		for (it1 = inEventEmitter->fAllListeners.begin(); it1 != inEventEmitter->fAllListeners.end(); it1++)

			for (it2 = inEventEmitter->fAllListeners[it1->first].begin(); it2 != inEventEmitter->fAllListeners[it1->first].end(); it2++)
			{
				;//it2->fCallback->Unprotect();
			}
		inEventEmitter->fAllListeners.clear();
		inEventEmitter->fNumberListeners = 0;

	} else {

		XBOX::VString	eventName;

		if (ioParms.CountParams() < 1
		|| !ioParms.IsStringParam(1) || !ioParms.GetStringParam(1, eventName))	{

			XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
			return;

		}

		if ((it1 = inEventEmitter->fAllListeners.find(eventName)) == inEventEmitter->fAllListeners.end())

			return;

		sLONG	numberListeners;

		numberListeners = (sLONG) it1->second.size();

		for (it2 = inEventEmitter->fAllListeners[it1->first].begin(); it2 != inEventEmitter->fAllListeners[it1->first].end(); it2++)
		{
			//it2->fCallback->Unprotect();
		}
		inEventEmitter->fAllListeners.erase(it1);

		xbox_assert(inEventEmitter->fNumberListeners >= numberListeners);
		inEventEmitter->fNumberListeners -= numberListeners;	

	}
}

void VJSEventEmitter::_setMaxListeners (XBOX::VJSParms_callStaticFunction &ioParms, VJSEventEmitter *inEventEmitter)
{
	xbox_assert(inEventEmitter != NULL);

	sLONG	maximum;

	if (ioParms.CountParams() < 1
	|| !ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &maximum) || maximum < 0) {

		XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
		return;

	}

	inEventEmitter->fMaximumListeners = maximum;
}

void VJSEventEmitter::_listeners (XBOX::VJSParms_callStaticFunction &ioParms, VJSEventEmitter *inEventEmitter)
{
	xbox_assert(inEventEmitter != NULL);

	XBOX::VString	eventName;

	if (ioParms.CountParams() < 1
	|| !ioParms.IsStringParam(1) || !ioParms.GetStringParam(1, eventName))	{

		XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
		return;

	}

	XBOX::VJSArray						arrayObject(ioParms.GetContext());
	std::list<XBOX::VJSObject>			callbackList;
	
	if (inEventEmitter->_getCallbacks(eventName, &callbackList, false)) {

		std::list<XBOX::VJSObject>::iterator	i;

		for (i = callbackList.begin(); i != callbackList.end(); i++) {

			XBOX::VJSValue	value = (*i);

			arrayObject.PushValue(value);

		}

	}	

	ioParms.ReturnValue(arrayObject);
}

void VJSEventEmitter::_emit (XBOX::VJSParms_callStaticFunction &ioParms, VJSEventEmitter *inEventEmitter)
{
	xbox_assert(inEventEmitter != NULL);

	XBOX::VString	eventName;

	if (ioParms.CountParams() < 1
	|| !ioParms.IsStringParam(1) || !ioParms.GetStringParam(1, eventName))	{

		XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
		return;

	}

	std::vector<XBOX::VJSValue>	arguments;
	sLONG						i;

	for (i = 2; i <= ioParms.CountParams(); i++) 

		arguments.push_back(ioParms.GetParamValue(i));

	inEventEmitter->Emit(ioParms.GetContext(), eventName, &arguments);
}

void VJSEventEmitter::_add (XBOX::VJSParms_callStaticFunction &ioParms, VJSEventEmitter *inEventEmitter, bool inOnce)
{
	xbox_assert(inEventEmitter != NULL);

	XBOX::VString	eventName;
	XBOX::VJSObject	callbackObject(ioParms.GetContext());

	if (ioParms.CountParams() < 2
	|| !ioParms.IsStringParam(1) || !ioParms.GetStringParam(1, eventName)
	|| !ioParms.IsObjectParam(2) || !ioParms.GetParamObject(2, callbackObject) || !callbackObject.IsFunction()) 

		XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
		
	else {

		VJSWorker	*worker;

		worker = VJSWorker::RetainWorker(ioParms.GetContext());
		xbox_assert(worker != NULL);
	
		inEventEmitter->AddListener(worker, eventName, callbackObject, inOnce);

		XBOX::ReleaseRefCountable<VJSWorker>(&worker);

	}
}

sLONG VJSEventEmitter::_getCallbacks (const XBOX::VString &inEventName, std::list<XBOX::VJSObject> *outList, bool inRemoveOnce)
{
	xbox_assert(outList != NULL && outList->empty());

	if (fAllListeners.find(inEventName) == fAllListeners.end())

		return 0;

	SListenerList			*listeners	= &fAllListeners[inEventName];
	SListenerList::iterator	i;
	sLONG					n;

	if (inRemoveOnce) 

		for (i = listeners->begin(), n = 0; i != listeners->end(); n++) {

/*			XBOX::DebugMsg("VJSEventEmitter::getcallback(): context %lu: objectref: %lu\n", 
				(unsigned long) i->fContext, (unsigned long) i->fCallback); */


			outList->push_back(i->fCallback);
			if (i->fOnce) {

				i = listeners->erase(i);
				xbox_assert(fNumberListeners);
				fNumberListeners--;

			} else

				i++;

		}

	else 

		for (i = listeners->begin(), n = 0; i != listeners->end(); i++, n++) {
/*
			XBOX::DebugMsg("VJSEventEmitter::getcallback(): context %lu: objectref: %lu\n", 
				(unsigned long) i->fContext, (unsigned long) i->fCallback);
*/
			outList->push_back(i->fCallback);

		}

	return n;
}

const XBOX::VString	VJSEventEmitterClass::kModuleName	= CVSTR("events");

void VJSEventEmitterClass::RegisterModule ()
{
	VJSGlobalClass::RegisterModuleConstructor(kModuleName, _ModuleConstructor);
}

XBOX::VJSObject VJSEventEmitterClass::_ModuleConstructor (const VJSContext &inContext, const VString &inModuleName)
{
	xbox_assert(inModuleName.EqualToString(kModuleName, true));

	XBOX::VJSObject	module(inContext);

	VJSEventEmitterClass::Class();
	module.MakeEmpty();
	module.SetProperty("EventEmitter", _NewInstance(inContext), JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete);
	
	return module;
}

void VJSEventEmitterClass ::GetDefinition (ClassDefinition &outDefinition)
{
	static XBOX::VJSClass<VJSEventEmitterClass, VJSEventEmitter>::StaticFunction functions[] =
	{
		{ "addListener", js_callStaticFunction<VJSEventEmitter::_addListener>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ "on", js_callStaticFunction<VJSEventEmitter::_on>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ "once", js_callStaticFunction<VJSEventEmitter::_once>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ "removeListener", js_callStaticFunction<VJSEventEmitter::_removeListener>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ "removeAllListeners", js_callStaticFunction<VJSEventEmitter::_removeAllListeners>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ "setMaxListeners", js_callStaticFunction<VJSEventEmitter::_setMaxListeners>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ "listeners", js_callStaticFunction<VJSEventEmitter::_listeners>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ "emit", js_callStaticFunction<VJSEventEmitter::_emit>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ 0,	0,	0 },
	};

	outDefinition.className			= "EventEmitter";	
	outDefinition.initialize		= js_initialize<_Initialize>;
    outDefinition.finalize			= js_finalize<_Finalize>;
	outDefinition.staticFunctions	= functions;
}

void VJSEventEmitterClass::_Construct ( VJSParms_construct &ioParms)
{
	ioParms.ReturnConstructedObject(_NewInstance(ioParms.GetContext()));
}

void VJSEventEmitterClass::_Initialize (const XBOX::VJSParms_initialize &inParms, VJSEventEmitter *inEventEmitter)
{
	xbox_assert(inEventEmitter != NULL);

	inEventEmitter->Retain();
}

void VJSEventEmitterClass::_Finalize (const XBOX::VJSParms_finalize &inParms, VJSEventEmitter *inEventEmitter)
{
	xbox_assert(inEventEmitter != NULL);

	inEventEmitter->Release();
}

XBOX::VJSObject VJSEventEmitterClass::_NewInstance (const XBOX::VJSContext &inContext)
{
	XBOX::VJSObject	constructedObject(inContext);
	VJSEventEmitter	*eventEmitter;

	if ((eventEmitter = new VJSEventEmitter()) == NULL) {

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
	
	} else {

		constructedObject = CreateInstance(inContext, eventEmitter);
		eventEmitter->Release();

	}

	return constructedObject;
}
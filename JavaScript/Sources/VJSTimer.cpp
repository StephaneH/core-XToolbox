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

#include "VJSTimer.h"

#include "VJSContext.h"
#include "VJSGlobalClass.h"

#include "VJSEvent.h"
#include "VJSWorker.h"

USING_TOOLBOX_NAMESPACE

const sLONG	VJSTimer::kMinimumTimeout	= 4;
const sLONG	VJSTimer::kMinimumInterval	= 10;

const sLONG	VJSTimer::kTimeOut			= -1;
const sLONG	VJSTimer::kTimeOutCleared	= -2;
const sLONG	VJSTimer::kIntervalCleared	= -3;

VJSTimerContext::VJSTimerContext ()
{
	fCounter = 0;
	fTimers.clear();
}

VJSTimerContext::~VJSTimerContext () 
{
	// All timers are refcounted, fTimers will decrement them.
}

VJSTimer *VJSTimerContext::LookUpTimer (sLONG inID)
{
	std::map<sLONG, VRefPtr<VJSTimer> >::iterator	it;

	if ((it = fTimers.find(inID)) == fTimers.end()) 

		return NULL;

	else {

		xbox_assert(it->second != NULL);

		return it->second;

	}
}

sLONG VJSTimerContext::InsertTimer (VJSTimer *inTimer)
{
	xbox_assert(inTimer != NULL);

	sLONG	index;

	index = fCounter;
	while (LookUpTimer(index) != NULL) 

		if ((index = (index + 1) & kCounterMask) == fCounter)

			// Loop around! All IDs used, should never happen.

			return -1;

	fCounter = (index + 1) & kCounterMask;

	fTimers[index] = inTimer;
	inTimer->fTimerContext = this;
	inTimer->fID = index;

	return index;
}

void VJSTimerContext::RemoveTimer (sLONG inID)
{
	xbox_assert(!(inID & ~kCounterMask));

	std::map<sLONG, VRefPtr<VJSTimer> >::iterator	it;

	it = fTimers.find(inID);

	xbox_assert(it != fTimers.end());

	it->second->fTimerContext = NULL;
	it->second->fID = -1;
	it->second->Release();

	fTimers.erase(it);
}

void VJSTimerContext::ClearAll ()
{
	std::map<sLONG, VRefPtr<VJSTimer> >::iterator	it;

	for (it = fTimers.begin(); it != fTimers.end(); it++)

		it->second->_Clear();
}

void VJSTimer::SetTimeout (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *inGlobalObject)
{
	VJSWorker	*worker;

	worker = VJSWorker::RetainWorker(ioParms.GetContext());
	VJSTimer::_SetTimer(ioParms, worker, false);
	XBOX::ReleaseRefCountable<VJSWorker>(&worker);
}

void VJSTimer::ClearTimeout (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *inGlobalObject)
{
	VJSWorker	*worker;

	worker = VJSWorker::RetainWorker(ioParms.GetContext());
	VJSTimer::_ClearTimer(ioParms, worker, false);
	XBOX::ReleaseRefCountable<VJSWorker>(&worker);
}

void VJSTimer::SetInterval (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *inGlobalObject)
{
	VJSWorker	*worker;

	worker = VJSWorker::RetainWorker(ioParms.GetContext());
	VJSTimer::_SetTimer(ioParms, worker, true);
	XBOX::ReleaseRefCountable<VJSWorker>(&worker);
}

void VJSTimer::ClearInterval (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *inGlobalObject)
{
	VJSWorker	*worker;

	worker = VJSWorker::RetainWorker(ioParms.GetContext());
	VJSTimer::_ClearTimer(ioParms, worker, true);
	XBOX::ReleaseRefCountable<VJSWorker>(&worker);
}

VJSTimer::VJSTimer (XBOX::VJSObject &inFunctionObject, sLONG inInterval)
:fFunctionObject(inFunctionObject)
{
	xbox_assert(inFunctionObject.HasRef());
	xbox_assert(inInterval >= 0 || inInterval == VJSTimer::kTimeOut);

	fTimerContext = NULL;
	fID = -1;
	fInterval = inInterval;
	//fFunctionObject = new VJSObject(inFunctionObject);	
}

VJSTimer::~VJSTimer ()
{
	// Before destructor is called, timer must have been properly removed from its VJSTimerContext object.

	xbox_assert(fTimerContext == NULL && fID == -1);
}

void VJSTimer::_SetTimer (VJSParms_callStaticFunction &ioParms, VJSWorker *inWorker, bool inIsInterval) 
{
	xbox_assert(inWorker != NULL);

	if (!ioParms.CountParams())
		
		return;

	XBOX::VJSContext	context(ioParms.GetContext());
	XBOX::VJSObject		functionObject(context);

	ioParms.GetParamObject(1, functionObject);
	if (!functionObject.IsFunction())

		return;

	functionObject.Protect();

	Real	duration;

	duration = 0.0;
	if (ioParms.CountParams() >= 2) {

		if (ioParms.IsNumberParam(2)) {
			
			if (!ioParms.GetRealParam(2, &duration))
			
				duration = 0.0;

		} else {

			// According to specification, if timeout is an object, call its toString() method if any. 
			// Then apply ToNumber() on the string to obtain duration.
			
			XBOX::VJSObject	timeOutObject(context);

			if (ioParms.GetParamObject(2, timeOutObject)) {

				timeOutObject.SetContext(context);
				if (timeOutObject.HasProperty("toString")) {

					XBOX::VJSObject	toStringObject = timeOutObject.GetPropertyAsObject("toString");

					if (toStringObject.IsFunction()) {

						std::vector<XBOX::VJSValue>	values;
						XBOX::VJSValue				string(context);

						toStringObject.SetContext(context);
						timeOutObject.CallFunction(toStringObject, &values, &string);

						if (string.IsString()) {

							// If Number() is called as a function (and not as a constructor), it acts as ToNumber().
							// See section 15.7.1 of ECMA-262 specification.

							XBOX::VJSObject	toNumberObject = context.GetGlobalObject().GetPropertyAsObject("Number");

							if (toNumberObject.IsFunction()) {

								XBOX::VJSValue	number(context);

								values.clear();
								values.push_back(string);
								toNumberObject.SetContext(context);
								context.GetGlobalObject().CallFunction(toNumberObject, &values, &number);

								if (number.IsNumber() && !number.GetReal(&duration))

									duration = 0.0;

							}

						}

					}

				}

			}
		
		}

		// (value != value) is true if value is a NaN.

		if (duration < 0.0 || duration > XBOX::kMAX_Real || duration != duration)
		
			duration = 0.0;

	}	
	
	std::vector<XBOX::VJSValue> *arguments;

	arguments = new std::vector<XBOX::VJSValue>;
	for (sLONG i = 3; i <= ioParms.CountParams(); i++) 

		arguments->push_back(ioParms.GetParamValue(i));

	sLONG		period, id;
	VJSTimer	*timer;

	period = (sLONG) duration;
	if (inIsInterval) {

		if (period < VJSTimer::kMinimumInterval)

			period = VJSTimer::kMinimumInterval;

	} else {

		if (period < VJSTimer::kMinimumTimeout)

			period = VJSTimer::kMinimumTimeout;

	}		 
	timer = new VJSTimer(functionObject, inIsInterval ? period : VJSTimer::kTimeOut);
	if ((id = inWorker->GetTimerContext()->InsertTimer(timer)) < 0) {

		// Too many timers (should never happen). Silently ignore. 
		// Returned ID (-1) isn't valid and a clear on it, will do nothing.

		timer->Release();
		delete arguments;
	
	} else {

		XBOX::VTime	triggerTime;
	
		triggerTime.FromSystemTime();
		triggerTime.AddMilliseconds(period);

		inWorker->QueueEvent(VJSTimerEvent::Create(timer, triggerTime, arguments));

	}

	ioParms.ReturnNumber(id);
}

void VJSTimer::_ClearTimer (VJSParms_callStaticFunction &ioParms, VJSWorker *inWorker, bool inIsInterval) 
{
	xbox_assert(inWorker != NULL);

	if (!ioParms.CountParams() || !ioParms.IsNumberParam(1))

		return;
		
	sLONG			id;
	XBOX::VJSTimer	*timer;

	ioParms.GetLongParam(1, &id);
	if ((timer = inWorker->GetTimerContext()->LookUpTimer(id)) == NULL)

		return;	// No timer with given ID found.

	if (timer->IsInterval() != inIsInterval)

		return;	// Mismatched call (cannot used clearInterval() to clear a timeout for example).

	// Mark timer as "cleared".

	timer->_Clear();	

	// This will loop the event queue, trying to find the VJSTimerEvent.
	//
	// If the clearTimeout() or clearInterval() is executed inside "itself" (in its callback),
	// this will do nothing. The event is already executing, but as the timer is marked as 
	// "cleared", VJSTimerEvent::Discard() will free it.
	//
	// Otherwise, the loop will find the VJSTimerEvent object, calling its Discard() and thus 
	// freeing the timer.
	
	inWorker->UnscheduleTimer(timer);
}

void VJSTimer::_Clear ()
{
	fInterval = IsInterval() ? VJSTimer::kIntervalCleared : VJSTimer::kTimeOutCleared;
}

void VJSTimer::_ReleaseIfCleared ()
{	
	xbox_assert(fID >= 0);
	xbox_assert(fTimerContext != NULL);

	if (fInterval == VJSTimer::kIntervalCleared || fInterval == VJSTimer::kTimeOutCleared)

		fTimerContext->RemoveTimer(fID);
}
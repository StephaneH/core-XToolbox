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
#ifndef __VJS_TIMER__
#define __VJS_TIMER__

#include "VJSClass.h"
#include "VJSValue.h"

BEGIN_TOOLBOX_NAMESPACE

// Implement JavaScript timers. 
// See HTML5 specification (http://www.w3.org/TR/html5), section "6.2 Timers".

class VJSGlobalObject;
class VJSWorker;

class VJSTimer;

// All timers (context) of a JavaScript execution.

class XTOOLBOX_API VJSTimerContext : public XBOX::VObject
{
public:

				VJSTimerContext ();
	virtual		~VJSTimerContext ();

	VJSTimer	*LookUpTimer (sLONG inID);

	sLONG		InsertTimer (VJSTimer *inTimer);	// Return -1 if failed (should never happen).

	void		RemoveTimer (sLONG inID);			// This will free (release) timer.

	void		ClearAll ();						// Mark all timers for removal.

private:

	static const sLONG					kCounterMask	= 0x00ffffff;

	sLONG								fCounter;
	std::map<sLONG, VRefPtr<VJSTimer> >	fTimers;
};

// A one-shot (setTimeout()) or periodic (setInterval()) timer function.

class XTOOLBOX_API VJSTimer : public XBOX::VObject, public XBOX::IRefCountable
{
public:

	// Minimum timeout or interval, in milliseconds (see specification).

	static const sLONG	kMinimumTimeout;
	static const sLONG	kMinimumInterval;
	
	// Function scheduling.

	static void	SetTimeout (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *inGlobalObject);
	static void	ClearTimeout (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *inGlobalObject);
	static void	SetInterval (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *inGlobalObject);
	static void	ClearInterval (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *inGlobalObject);

	// Is interval (periodic) timer? If not, it's a timeout (delay).

	bool		IsInterval ()	{	return fInterval >= 0 || fInterval == kIntervalCleared;	}

	// Is this timer cleared ? Has been processed (timeout), cleared (clearTimeout() or clearInterval()), 
	// or has its timer event discarded?

	bool		IsCleared ()	{	return fInterval == VJSTimer::kTimeOutCleared || fInterval == VJSTimer::kIntervalCleared;	}

private:

friend class VJSTimerContext;
friend class VJSTimerEvent;

	// Interval is in milliseconds, negative values are special constants. 
	// For timeouts, actual delay is set in the trigger time of timer event.
	
	static const sLONG		kTimeOut;
	
	static const sLONG		kTimeOutCleared;	// Processed, discarded, or cleared.
	static const sLONG		kIntervalCleared;	// Discarded or cleared.
	
	VJSTimerContext			*fTimerContext;
	sLONG					fID;
	sLONG					fInterval;
	XBOX::VJSObject			fFunctionObject;	
				
				VJSTimer ();
				VJSTimer (XBOX::VJSObject &inFunctionObject, sLONG inInterval);
	virtual		~VJSTimer ();

	static void _SetTimer (VJSParms_callStaticFunction &ioParms, VJSWorker *inWorker, bool inIsInterval);
	static void	_ClearTimer (VJSParms_callStaticFunction &ioParms, VJSWorker *inWorker, bool inIsInterval);

	// Mark timer has been executed or canceled.

	void		_Clear ();

	// Only to be called by VJSTimerEvent::Discard(). This is the only way to de-allocate memory for a timer.
	// See comment in _ClearTimer() code.

	void		_ReleaseIfCleared ();	
};

END_TOOLBOX_NAMESPACE

#endif
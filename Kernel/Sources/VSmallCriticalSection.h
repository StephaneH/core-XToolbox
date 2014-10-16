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
#ifndef __VSmallCriticalSection__
#define __VSmallCriticalSection__

BEGIN_TOOLBOX_NAMESPACE

// Defined bellow
class VNonVirtualCriticalSection;
class VSmallCriticalSection;
class VSyncEvent;

class XTOOLBOX_API VNonVirtualCriticalSection  // Similar VCriticalSection but takes only 12 bytes
{
public:
	VNonVirtualCriticalSection ();
	~VNonVirtualCriticalSection ();	// Non virtual on purpose (should never been overriden)

	Boolean	Lock ();
	Boolean	TryToLock ();
	Boolean	Unlock ();

	sLONG	GetUseCount() const	{ return fUseCount; };	// No protection - may be called only if Lock() returns true

private:
	VSyncEvent*	fEvent;
	VTaskID		fOwner;
	sLONG		fUseCount;

	static sLONG	sUnlockCount;
};


class XTOOLBOX_API VSmallCriticalSection	// Similar VCriticalSection but takes only 8 bytes
{ 
public:
	VSmallCriticalSection ();
	~VSmallCriticalSection ();	// Non virtual on purpose (should never been overriden)

	Boolean	Lock ();
	Boolean	TryToLock ();
	Boolean	Unlock ();

	sLONG	GetUseCount () const { return fUseCount; };	// No protection - may be called only if Lock() returns true
	
protected:
	VSyncEvent*	fEvent;
	sWORD		fOwner;		// CAUTION: Assumes fOwner and fUseCount are contiguous
	sWORD		fUseCount;	//	and ordered.

	static sLONG	sUnlockCount;
};

END_TOOLBOX_NAMESPACE


#endif

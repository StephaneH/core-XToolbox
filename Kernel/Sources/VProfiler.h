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
#ifndef __VProfiler__
#define __VProfiler__

#include "Kernel/Sources/VString.h"
#include "Kernel/Sources/VAssert.h"

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API VMicrosecondsCounter : public VObject
{
public:
			VMicrosecondsCounter ();	
	virtual	~VMicrosecondsCounter ();
	
	void	Start ();
	sLONG8	Stop ();
	
	sLONG8	GetDuration () const;
	
protected:
#if VERSIONWIN
	LARGE_INTEGER		fStartTime;
	LARGE_INTEGER		fStopTime;
	sLONG8				fCorrection;
#else
	sLONG8				fStartTime;
	sLONG8				fStopTime;
#endif

	sLONG8	fFrequency;
};

class XTOOLBOX_API VProfilingCounter : public VObject
{
public:
			VProfilingCounter (const VString* inCounterName = NULL, Boolean inStartNow = false, Boolean inDebugDumpOnDelete = false);	
	virtual	~VProfilingCounter ();
	
	void	Start ();
	void	Stop (Boolean inDebugDump = false);
	void	Reset ();
	
	Real	GetDuration () const;
	sLONG8	GetProfilingFrequency () { return fFrequency; };
	
	void	DumpStats ();
	void	DumpDuration ();
	
	void	SetCounterName (const VString& inCounterName) { fCounterName = inCounterName; };
	VString&	GetCounterName () { return fCounterName; };
	
protected:
#if VERSIONWIN
	LARGE_INTEGER	fStartTime;
	LARGE_INTEGER	fStopTime;
	sLONG8	fCorrection;
#else
	sLONG8	fStartTime;
	sLONG8	fStopTime;
#endif

	sLONG8	fFrequency;
	uLONG	fNbVal;
	Real	fMin;
	Real	fMax;
	Real	fTotal;
	Boolean	fDebugDumpOnDelete;
	VString	fCounterName;
	void _message(const VString& inCounterName);
};

END_TOOLBOX_NAMESPACE

#endif

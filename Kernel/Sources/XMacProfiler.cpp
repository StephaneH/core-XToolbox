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
#include "VKernelPrecompiled.h"
#include "VProfiler.h"


VProfilingCounter::VProfilingCounter(const VString* inCounterName, Boolean inStartNow, Boolean inDebugDumpOnDelete)
{
	fFrequency = 1000000L;
	fDebugDumpOnDelete = inDebugDumpOnDelete;
	
	if (inCounterName != NULL)
		fCounterName.FromString(*inCounterName);
	
	Reset();
	
	if (inStartNow)
		Start();
}


VProfilingCounter::~VProfilingCounter()
{
	if (fDebugDumpOnDelete)
	{
		DumpStats();
		Stop(false);
	}
}


void VProfilingCounter::Start()
{
	::Microseconds((UnsignedWide *) &fStartTime);
}


void VProfilingCounter::Stop(Boolean inDebugDump)
{
	::Microseconds((UnsignedWide *) &fStopTime);
	
	Real	duration = GetDuration();
	
	if (duration < fMin)
		fMin = duration;
	
	if (duration > fMax)
		fMax = duration;
	
	fTotal += duration;
	fNbVal++;
	
	if (inDebugDump)
		DumpDuration();
}


Real VProfilingCounter::GetDuration() const
{
	return (Real) (fStopTime - fStartTime);
}


void VProfilingCounter::DumpStats()
{
	VString	string;
	string = GetCounterName();
	string += " : \r\n";
	string.AppendLong(fNbVal);
	string += " value\r\n";
	DebugMsg(string);
	
	string = "last value : ";
	string.AppendReal(GetDuration());
	string += "\r\n";
	DebugMsg(string);
	
	string = "min : ";
	string.AppendReal(fMin);
	string += "\r\n";
	DebugMsg(string);
	
	string = "max : ";
	string.AppendReal(fMax);
	string += "\r\n";
	DebugMsg(string);
	
	string = "average : ";
	string.AppendReal(fTotal/fNbVal);
	string += "\r\n";
	DebugMsg(string);
}


void VProfilingCounter::DumpDuration()
{
	VString	string = fCounterName;
	string += " ";
	string.AppendReal(GetDuration());
	string += "\r\n";
	DebugMsg(string);
}


void VProfilingCounter::Reset()
{
	fNbVal = 0;
	fMin = 1.79769313486231E+308;
	fMax = 0;
	fTotal = 0.0;
}


/*************************************************************************************/
// compteur simple
VMicrosecondsCounter::VMicrosecondsCounter()
{
	fFrequency = 1000000L;
}


VMicrosecondsCounter::~VMicrosecondsCounter()
{
	
}


void VMicrosecondsCounter::Start()
{
	::Microseconds((UnsignedWide *) &fStartTime);
}


sLONG8 VMicrosecondsCounter::Stop()
{
	::Microseconds((UnsignedWide *) &fStopTime);
	
	return GetDuration();
}


sLONG8 VMicrosecondsCounter::GetDuration() const
{
	return (fStopTime - fStartTime);
}



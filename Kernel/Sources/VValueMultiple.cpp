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
#include "VValueMultiple.h"


VValueMultiple::VValueMultiple()
{
}


VValueMultiple::~VValueMultiple()
{
}


void VValueMultiple::FromBoolean(Boolean inValue, sLONG inElement)
{
	FromWord(sWORD(inValue), inElement);
}


void VValueMultiple::FromLong(sLONG inValue, sLONG inElement)
{
	FromReal(Real(inValue), inElement);
}


void VValueMultiple::FromLong8(sLONG8 inValue, sLONG inElement)
{
	FromReal(Real(inValue), inElement);
}


void VValueMultiple::FromReal(Real /*inValue*/, sLONG /*inElement*/)
{
	assert(false);
}


void VValueMultiple::FromFloat(const VFloat& /*inValue*/, sLONG /*inElement*/)
{
	assert(false);
}


void VValueMultiple::FromString(const VString& /*inValue*/, sLONG /*inElement*/)
{
	assert(false);
}


void VValueMultiple::FromWord(sWORD inValue, sLONG inElement)
{
	FromLong(sLONG(inValue), inElement);
}

void VValueMultiple::FromByte(sBYTE inValue, sLONG inElement)
{
	FromLong(sLONG(inValue), inElement);
}

void VValueMultiple::FromDuration(const VDuration& /*inValue*/, sLONG /*inElement*/)
{
	assert(false);
}


void VValueMultiple::FromTime(const VTime& /*inValue*/, sLONG /*inElement*/)
{
	assert(false);
}


CompareResult VValueMultiple::CompareTo(const VValueSingle& /*inValue*/, Boolean /*inDiacritic*/, sLONG /*inElement*/) const
{
	return CR_UNRELEVANT;
}


VValue* VValueMultiple::Clone() const
{
	assert(false);
	return NULL;
}
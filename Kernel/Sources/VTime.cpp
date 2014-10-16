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
#include "VTime.h"
#include "VString.h"
#include "VFloat.h"
#include "VStream.h"
#include "VSystem.h"
#include "VMemoryCpp.h"
#include "VIntlMgr.h"
#include "VJSONValue.h"

const VDuration::InfoType	VDuration::sInfo;
const VTime::InfoType		VTime::sInfo;


VDuration::VDuration(const VDuration& inOriginal)
{
	FromDuration(inOriginal);
}


VDuration::VDuration(sLONG8 inMilliseconds)
{
	fMilliseconds = inMilliseconds;
	GotValue(false);
}


VDuration::VDuration(sLONG8* inDataPtr, bool inInit)
{
	if (inInit)
		Clear();
	else
		fMilliseconds = *inDataPtr;
}


void VDuration::Clear()
{
	fMilliseconds = 0;
	GotValue();
}


CompareResult VDuration::CompareTo( const VValueSingle& inValue, Boolean /*inDiacritical*/) const
{
	VDuration vDuration;
	vDuration.FromValue( inValue);
	return CompareTo( vDuration);
}


CompareResult VDuration::CompareTo(const VDuration& inDuration) const
{
	if (fMilliseconds > inDuration.fMilliseconds)
		return CR_BIGGER;
	else if (fMilliseconds < inDuration.fMilliseconds)
		return CR_SMALLER;
	else
		return CR_EQUAL;
}


Boolean VDuration::GetBoolean() const
{
	return fMilliseconds != 0;
}


sBYTE VDuration::GetByte() const
{
	return (sBYTE) fMilliseconds;
}


sWORD VDuration::GetWord() const
{
	return (sWORD) fMilliseconds;
}


sLONG VDuration::GetLong() const
{
	return (sLONG) fMilliseconds;
}


sLONG8 VDuration::GetLong8() const
{
	return fMilliseconds;
}


Real VDuration::GetReal() const
{
	return (Real) fMilliseconds;
}


void VDuration::GetFloat(VFloat& outValue) const
{
	outValue.FromLong8(fMilliseconds);
}


void VDuration::GetDuration(VDuration& outValue) const
{
	outValue.fMilliseconds = fMilliseconds;
}


// Sous la forme "255:16:25:45:333"
void VDuration::GetString(VString& inValue) const
{
	inValue.Clear();
	
	if (! IsNull())
	{
		inValue.FromLong8(fMilliseconds);
		/*
		sLONG days, hours, minutes, seconds, milliseconds;

		GetDuration(days, hours, minutes, seconds, milliseconds);
		
		inValue.AppendLong(days);
		inValue.AppendCString(":");
		inValue.AppendLong(hours);
		inValue.AppendCString(":");
		inValue.AppendLong(minutes);
		inValue.AppendCString(":");
		inValue.AppendLong(seconds);
		inValue.AppendCString(":");
		inValue.AppendLong(milliseconds);
		*/
	}
}


/** convert to xml duration string : PdddDTddHddMdd[.dddddd]S (where d is digit)  (XSO_Duration_ISO)
									 or
									 number of seconds (XSO_Duration_Seconds)

*/
void VDuration::GetXMLString( VString& outString, XMLStringOptions inOptions) const
{
	if (inOptions & XSO_Duration_ISO)
	{
		//xml duration format

		sLONG days, hours, minutes, seconds, milliseconds;
		GetDuration(days, hours, minutes, seconds, milliseconds);
		char buffer[256];
		if (days != 0)
		{
			if (milliseconds != 0)
			{
				float secondsDecimal = seconds + (float) (milliseconds/1000.0);
				#if COMPIL_VISUAL
				sprintf( buffer, "P%dDT%dH%dM%.14fS", days, hours, minutes, secondsDecimal);
				#else
				snprintf( buffer, sizeof( buffer), "P%dDT%dH%dM%.14fS", days, hours, minutes, secondsDecimal);
				#endif
			}
			else
			{
				#if COMPIL_VISUAL
				sprintf( buffer, "P%dDT%dH%dM%dS", days, hours, minutes, seconds);
				#else
				snprintf( buffer, sizeof( buffer), "P%dDT%dH%dM%dS", days, hours, minutes, seconds);
				#endif
			}
		}
		else
		{
			if (milliseconds != 0)
			{
				float secondsDecimal = seconds + (float) (milliseconds/1000.0);
				#if COMPIL_VISUAL
				sprintf( buffer, "PT%dH%dM%.14fS", hours, minutes, secondsDecimal);
				#else
				snprintf( buffer, sizeof( buffer), "PT%dH%dM%.14fS", hours, minutes, secondsDecimal);
				#endif
			}
			else
			{
				#if COMPIL_VISUAL
				sprintf( buffer, "PT%dH%dM%dS", hours, minutes, seconds);
				#else
				snprintf( buffer, sizeof( buffer), "PT%dH%dM%dS", hours, minutes, seconds);
				#endif
			}
		}
		outString.FromCString( buffer);
	}
	else
	{
		//format as number of seconds

		outString.FromLong8( ConvertToSeconds());
	}
}

/** convert from xml duration string : P[00Y][00M][00D]T00H00M00.000S 
									   or
									   number of seconds (long integer)
									   (or  
									   DDD:HH:MM:SS:MS (for backward compatibility))
@remark
	as duration is a relative time interval,
	the number of days per month is undeterminate so
	it is assumed to be 30 
*/
bool VDuration::FromXMLString( const VString& inString)
{
	if (inString.GetLength() >= 1 && inString.GetUniChar(1) == 'P')
	{
		//parse xml duration string

		const UniChar *c = inString.GetCPointer();
		sLONG year = 0, month = 0, day = 0, hour = 0, minute = 0;
		float second = (float) 0.0;
		VString decimal;
		decimal.EnsureSize(16);
		bool parseDate = true;
		if (*c == 'P')
		{
			c++;
			while (*c)
			{

				if (*c == 'Y' && parseDate)
				{
					year = decimal.GetLong();	
					decimal.SetEmpty();
				}
				else if (*c == 'M' && parseDate)
				{
					month = decimal.GetLong();
					decimal.SetEmpty();
				}
				else if (*c == 'D' && parseDate)
				{
					day = decimal.GetLong();
					decimal.SetEmpty();
				}
				else if (*c == 'H' && (!parseDate))
				{
					hour = decimal.GetLong();
					decimal.SetEmpty();
				}
				else if (*c == 'M' && (!parseDate))
				{
					minute = decimal.GetLong();
					decimal.SetEmpty();
				}
				else if (*c == 'S' && (!parseDate))
				{
					char buffer[256];
					decimal.ToCString( buffer, sizeof( buffer));
					::sscanf( buffer, "%f", &second);
				}
				else if (*c == 'T')
					parseDate = false;
				else
				{
					decimal.AppendUniChar( *c);
				}
				c++;
			}
		}
		SetDuration( day+month*30+year*12*30,
			hour, minute, (sLONG)(second), (sLONG)((second-((sLONG)second))*1000));
		return true;
	}

	if (inString.FindUniChar(':') > 0)
	{
		//for backward compatibility, 
		//parse old format 
		FromString( inString);
		return true;
	}
	else
	{
		//parse number of seconds from long integer
		FromNbSeconds( inString.GetLong8());
		return true;
	}
}


void VDuration::GetValue(VValueSingle& inValue) const
{
	inValue.FromDuration(*this);
}


void VDuration::FromBoolean(Boolean inValue)
{
	fMilliseconds = inValue ? 1 : 0;
	GotValue();
}


void VDuration::FromByte(sBYTE inValue)
{
	fMilliseconds = (sLONG8) inValue;
	GotValue();
}


void VDuration::FromWord(sWORD inValue)
{
	fMilliseconds = (sLONG8) inValue;
	GotValue();
}


void VDuration::FromLong(sLONG inValue)
{
	fMilliseconds = (sLONG8) inValue;
	GotValue();
}


void VDuration::FromLong8(sLONG8 inValue)
{
	fMilliseconds = inValue;
	GotValue();
}


void VDuration::FromReal(Real inValue)
{
	fMilliseconds = (sLONG8) inValue;
	GotValue();
}


void VDuration::FromFloat(const VFloat& inValue)
{
	fMilliseconds = inValue.GetLong8();
	GotValue();
}


void VDuration::FromDuration(const VDuration& inValue)
{
	fMilliseconds = inValue.fMilliseconds;
	GotValue();
}


void VDuration::FromString(const VString& inValue)
{
	/*
	sLONG ar[] = { 0, 0, 0, 0, 0 };
	UniChar c;
	uLONG pos = 0;
	bool inNumber = false;
	VIndex i;
	VString num;

	for (i = 0; (pos < 5) && (i < inValue.GetLength()); i++)
	{
		c = inValue[ i ];

		if (c >= '0' && c <= '9')
		{
			num.AppendUniChar(c);
			inNumber = true;
		}
		else
		{
			if (inNumber)
			{
				ar[ pos ] = num.GetLong();
				num.Clear();
				pos++;
				inNumber = false;
			}
		}
	}

	SetDuration(ar[0], ar[1], ar[2], ar[3], ar[4]);
	*/
	fMilliseconds = inValue.GetLong8();
	GotValue();
}


VError VDuration::FromJSONString(const VString& inJSONString, JSONOption inModifier)
{
	fMilliseconds = inJSONString.GetLong8();
	GotValue();
	return VE_OK;
}


VError VDuration::GetJSONString(VString& outJSONString, JSONOption inModifier) const
{
	outJSONString.FromLong8(fMilliseconds);
	return VE_OK;
}


VError VDuration::FromJSONValue( const VJSONValue& inJSONValue)
{
	if (inJSONValue.IsNull())
		SetNull( true);
	else
		FromNbMilliseconds( (sLONG8) inJSONValue.GetNumber());
	return VE_OK;
}


VError VDuration::GetJSONValue( VJSONValue& outJSONValue) const
{
	if (IsNull())
		outJSONValue.SetNull();
	else
		outJSONValue.SetNumber( ConvertToMilliseconds());
	return VE_OK;
}


void VDuration::FromValue(const VValueSingle& inValue)
{
	inValue.GetDuration(*this);
	GotValue(inValue.IsNull());
}



void* VDuration::WriteToPtr(void* inDataPtr, Boolean /*inRefOnly*/, VSize inMax) const
{
	sLONG8* pt = (sLONG8*) inDataPtr;

	*pt++ = fMilliseconds;

	return pt;
}


void* VDuration::LoadFromPtr(const void* inDataPtr, Boolean /*inRefOnly*/)
{
	sLONG8* pt = (sLONG8*) inDataPtr;

	fMilliseconds = *pt++;

	GotValue();
	return pt;
}


VError VDuration::ReadFromStream(VStream* inStream, sLONG /*inParam*/)
{
	fMilliseconds = inStream->GetLong8();
	return inStream->GetLastError();
}


VError VDuration::WriteToStream(VStream* inStream, sLONG inParam) const
{
	VValue::WriteToStream(inStream, inParam);

	inStream->PutLong8(fMilliseconds);

	return inStream->GetLastError();
}


bool VDuration::operator == (const VDuration &inDuration) const
{
	return fMilliseconds == inDuration.fMilliseconds;
}


bool VDuration::operator > (const VDuration &inDuration) const
{
	return fMilliseconds > inDuration.fMilliseconds;
}


bool VDuration::operator < (const VDuration &inDuration) const
{
	return fMilliseconds < inDuration.fMilliseconds;
}


bool VDuration::operator >= (const VDuration &inDuration) const
{
	return fMilliseconds >= inDuration.fMilliseconds;
}


bool VDuration::operator <= (const VDuration &inDuration) const
{
	return fMilliseconds <= inDuration.fMilliseconds;
}


bool VDuration::operator != (const VDuration &inDuration) const
{
	return fMilliseconds != inDuration.fMilliseconds;
}


CompareResult VDuration::CompareToSameKind(const VValue* inValue, Boolean /*inDiacritical*/) const
{
	XBOX_ASSERT_KINDOF(VDuration, inValue);
	
	const VDuration* theValue = reinterpret_cast<const VDuration*>(inValue);
	return CompareTo(*theValue);
}


Boolean VDuration::EqualToSameKind(const VValue* inValue, Boolean /*inDiacritical*/) const
{
	XBOX_ASSERT_KINDOF(VDuration, inValue);
	
	const VDuration* theValue = reinterpret_cast<const VDuration*>(inValue);
	return CompareTo(*theValue) == CR_EQUAL;
}


CompareResult VDuration::CompareToSameKindPtr(const void* inPtrToValueData, Boolean /*inDiacritical*/) const
{
	if (fMilliseconds == *((sLONG8*)inPtrToValueData))
		return CR_EQUAL;
	else if (fMilliseconds > *((sLONG8*)inPtrToValueData))
		return CR_BIGGER;
	else
		return CR_SMALLER;
}


Boolean VDuration::EqualToSameKindPtr(const void* inPtrToValueData, Boolean /*inDiacritical*/) const
{
	return fMilliseconds == *((sLONG8*)inPtrToValueData);
}


CompareResult VDuration::CompareToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const
{
	XBOX_ASSERT_KINDOF(VDuration, inValue);
	
	const VDuration* theValue = reinterpret_cast<const VDuration*>(inValue);
	return CompareTo(*theValue);
}


bool VDuration::EqualToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const
{
	XBOX_ASSERT_KINDOF(VDuration, inValue);
	
	const VDuration* theValue = reinterpret_cast<const VDuration*>(inValue);
	return CompareTo(*theValue) == CR_EQUAL;
}


CompareResult VDuration::CompareToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const
{
	if (fMilliseconds == *((sLONG8*)inPtrToValueData))
		return CR_EQUAL;
	else if (fMilliseconds > *((sLONG8*)inPtrToValueData))
		return CR_BIGGER;
	else
		return CR_SMALLER;
}

bool VDuration::EqualToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const
{
	return fMilliseconds == *((sLONG8*)inPtrToValueData);
}


Boolean VDuration::FromValueSameKind(const VValue* inValue)
{
	XBOX_ASSERT_KINDOF(VDuration, inValue);
	
	const VDuration* theValue = reinterpret_cast<const VDuration*>(inValue);
	FromDuration(*theValue);
	GotValue(theValue->IsNull());
	return true;
}


VDuration VDuration::operator = (const VDuration& inValue)
{
	FromDuration(inValue); 
	return *this;
}


void VDuration::GetDuration(sLONG& outDay, sLONG& outHour, sLONG& outMinute, sLONG& outSecond, sLONG& outMillisecond) const
{
	if (IsNull())
		outDay = outHour = outMinute = outSecond = outMillisecond = -1;
	else
	{
		outDay = GetDay();
		outHour = GetHour();
		outMinute = GetMinute();
		outSecond = GetSecond();
		outMillisecond = GetMillisecond();
	}
}


void VDuration::SetDuration(sLONG inDay, sLONG inHour, sLONG inMinute, sLONG inSecond, sLONG inMillisecond)
{
	fMilliseconds = (((sLONG8)inDay) * 24 * 60 * 60 * 1000)
					+ (inHour * 60 * 60 * 1000)
					+ (inMinute * 60 * 1000)
					+ (inSecond * 1000)
					+ (inMillisecond );
	GotValue();
}


void VDuration::AddDuration(sLONG inDays, sLONG inHour, sLONG inMinute, sLONG inSecond, sLONG inMillisecond)
{
	fMilliseconds += (((sLONG8)inDays) * 24 * 60 * 60 * 1000)
					 + (inHour * 60 * 60 * 1000)
					 + (inMinute * 60 * 1000)
					 + (inSecond * 1000)
					 + (inMillisecond );
	GotValue();
}


void VDuration::Add(const VDuration& inDuration)
{
	fMilliseconds += inDuration.fMilliseconds;
}


void VDuration::Substract(const VDuration& inDuration)
{
	fMilliseconds -= inDuration.fMilliseconds;
}


void VDuration::Multiply(Real inValue)
{
	fMilliseconds = (sLONG8)((Real)fMilliseconds * inValue);
}


void VDuration::Divide(Real inValue)
{
	fMilliseconds = (sLONG8)((Real)fMilliseconds / inValue);
}


Real VDuration::Divide(const VDuration& inDuration) const
{
	return (Real) fMilliseconds / (Real) inDuration.fMilliseconds;
}


void VDuration::Remainder(const VDuration& inDuration)
{
	fMilliseconds %= inDuration.fMilliseconds;
}


sLONG8 VDuration::ConvertToHours() const
{
	return fMilliseconds / (60 * 60 * 1000);
}


sLONG8 VDuration::ConvertToMinutes() const
{
	return fMilliseconds / (60 * 1000);
}

sLONG8 VDuration::ConvertToSeconds() const
{
	return fMilliseconds / 1000;
}


sLONG8 VDuration::ConvertToMilliseconds() const
{
	return fMilliseconds;
}


sLONG VDuration::GetDay() const
{
	return (sLONG) (fMilliseconds / (24 * 60 * 60 * 1000));
}


sLONG VDuration::GetHour() const
{
	sLONG hour = (sLONG) (fMilliseconds % (24 * 60 * 60 * 1000));
	hour /= ( 60 * 60 * 1000);

	return hour;
}


sLONG VDuration::GetMinute() const
{
	sLONG minute = (sLONG) (fMilliseconds % (60 * 60 * 1000));
	minute /= ( 60 * 1000);

	return minute;
}


sLONG VDuration::GetSecond() const
{
	sLONG second = (sLONG) (fMilliseconds % (60 * 1000));
	second /= ( 1000);

	return second;
}


sLONG VDuration::GetMillisecond() const
{
	return (sLONG) (fMilliseconds % 1000);
}


void VDuration::FromNbDays(sLONG8 inNbDays)
{
	fMilliseconds = (sLONG8) inNbDays * 24 * 60 * 60 * 1000;
	GotValue();
}


void VDuration::FromNbHours(sLONG8 inNbHours)
{
	fMilliseconds = (sLONG8) inNbHours * 60 * 60 * 1000;
	GotValue();
}


void VDuration::FromNbMinutes(sLONG8 inNbMinutes)
{
	fMilliseconds = (sLONG8) inNbMinutes * 60 * 1000;
	GotValue();
}


void VDuration::FromNbSeconds(sLONG8 inNbSeconds)
{
	fMilliseconds = (sLONG8) inNbSeconds * 1000;
	GotValue();
}


void VDuration::FromNbMilliseconds(sLONG8 inNbMilliseconds)
{
	fMilliseconds = inNbMilliseconds;
	GotValue();
}


void VDuration::AdjustDay(sLONG inDay)
{
	sLONG reste = (sLONG) (fMilliseconds % (24 * 60 * 60 * 1000));
	fMilliseconds = reste + inDay * (24 * 60 * 60 * 1000);
	GotValue();
}

void VDuration::AdjustHour(sLONG inHour)
{
	inHour -= GetHour();
	fMilliseconds += inHour * (60 * 60 * 1000);
	GotValue();
}


void VDuration::AdjustMinute(sLONG inMinute)
{
	inMinute -= GetMinute();
	fMilliseconds += inMinute * (60 * 1000);
	GotValue();
}


void VDuration::AdjustSecond(sLONG inSecond)
{
	inSecond -= GetSecond();
	fMilliseconds += inSecond * 1000;
	GotValue();
}


void VDuration::AdjustMillisecond(sLONG inMillisecond)
{
	inMillisecond -= GetMillisecond();
	fMilliseconds += inMillisecond;
	GotValue();
}


void VDuration::AddDays(sLONG inDay)
{
	fMilliseconds += inDay * (24 * 60 * 60 * 1000);
	GotValue();
}


void VDuration::AddHours(sLONG inHour)
{
	fMilliseconds += inHour * (60 * 60 * 1000);
	GotValue();
}


void VDuration::AddMinutes(sLONG inMinute)
{
	fMilliseconds += inMinute * (60 * 1000);
	GotValue();
}


void VDuration::AddSeconds(sLONG inSecond)
{
	fMilliseconds += inSecond * 1000;
	GotValue();
}


void VDuration::AddMilliseconds(sLONG8 inMillisecond)
{
	fMilliseconds += inMillisecond;
	GotValue();
}


Real VDuration::GetDecimalHour() const
{
	return (Real) fMilliseconds / (Real) (60 * 60 * 1000);
}


void VDuration::FromDecimalHour(Real inDecHour)
{
	fMilliseconds = (sLONG8) (inDecHour * (Real) (60 * 60 * 1000));
	GotValue();
}


void VDuration::AddDecimalHour(Real inDecHour)
{
	fMilliseconds += (sLONG8) (inDecHour * (Real) (60 * 60 * 1000));
	GotValue();
}


const VValueInfo *VDuration::GetValueInfo() const
{
	return &sInfo;
}


#pragma mark-

VTime::VTime(const VTime& inOriginal)
{
	FromTime(inOriginal);
}


VTime::VTime(uLONG8 inTimeStamp)
{
	FromStamp(inTimeStamp);
}

VTime::VTime(VTimeInitParam inInitMethod)
{
	switch ( inInitMethod )
	{
	case eInitWithCurrentTime :
		{
			sWORD localTime[7];
			VSystem::GetSystemLocalTime (localTime);
			FromLocalTime(localTime[0], localTime[1], localTime[2], localTime[3], localTime[4], localTime[5], localTime[6]);
		}
		break;
	case eInitWithZero:
	default :
		Clear();
		break;
	}
}


VTime::VTime(uLONG8* inDataPtr, bool inInit)
{
	if (inInit)
		Clear();
	else
		FromStamp(*inDataPtr);
}


void VTime::Clear()
{
	fMonth = fDay = 0;
	fYear = 0;
	fMilliseconds = 0;
	GotValue();
}

CompareResult VTime::CompareTo (const VValueSingle& inValue, Boolean inDiacritical) const
{
	VTime vTime;
	vTime.FromValue(inValue);
	return CompareTo(vTime);
}

CompareResult VTime::CompareTo(const VTime& inTime) const
{
	if (fYear > inTime.fYear)
		return CR_BIGGER;
	else if (fYear < inTime.fYear)
		return CR_SMALLER;
	else
	{
		if (fMonth > inTime.fMonth)
			return CR_BIGGER;
		else if(fMonth < inTime.fMonth)
			return CR_SMALLER;
		else
		{
			if (fDay > inTime.fDay)
				return CR_BIGGER;
			else if (fDay < inTime.fDay)
				return CR_SMALLER;
			else
			{
				if (fMilliseconds > inTime.fMilliseconds)
					return CR_BIGGER;
				else if (fMilliseconds < inTime.fMilliseconds)
					return CR_SMALLER;
				else
					return CR_EQUAL;
			}
		}
	}
}


CompareResult VTime::CompareToSameKindPtr(const void* inPtrToValueData, Boolean /*inDiacritical*/) const
{
	VTime other( (uLONG8*) inPtrToValueData, false);
	
	return CompareTo( other);
}


Boolean VTime::EqualToSameKindPtr(const void* inPtrToValueData, Boolean inDiacritical) const
{
	return CompareToSameKindPtr(inPtrToValueData, inDiacritical) == CR_EQUAL;
}


CompareResult VTime::CompareToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const
{
	XBOX_ASSERT_KINDOF(VTime, inValue);
	
	const VTime* theValue = reinterpret_cast<const VTime*>(inValue);
	return CompareTo(*theValue);
}


bool VTime::EqualToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const
{
	XBOX_ASSERT_KINDOF(VTime, inValue);
	
	const VTime* theValue = reinterpret_cast<const VTime*>(inValue);
	return CompareTo(*theValue) == CR_EQUAL;
}


CompareResult VTime::CompareToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const
{
	VTime other( (uLONG8*) inPtrToValueData, false);
	
	return CompareTo( other);
}


bool VTime::EqualToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const
{
	return CompareToSameKindPtrWithOptions(inPtrToValueData, inOptions) == CR_EQUAL;
}


void VTime::GetStringLocalTime(VString& inValue) const
{
	inValue.Clear();

	if (! IsNull())
	{
		sWORD year, month, day, hour, minute, second, millisecond;
		
		GetLocalTime(year, month, day, hour, minute, second, millisecond);
		
		inValue.AppendPrintf("%04d-%02d-%02d %02d:%02d:%02d:%03d", year, month, day, hour, minute, second, millisecond);
	}
}

void VTime::GetString( VString& outValue) const
{
	FormatByOS( outValue, true);
}

void VTime::toSimpleDateString(VString& outString) const
{
	if (IsNull())
		outString.SetNull(true);
	else
	{
		outString = ToString((sLONG)fDay) + "!" + ToString((sLONG)fMonth) + "!" + ToString((sLONG)fYear);
	}
}


void VTime::FormatByOS(VString& outDateValue,VString& outTimeValue,bool useGMT)const
{
	if (! IsNull())
	{
		VIntlMgr *intl = VTask::GetCurrentIntlManager();
		intl->FormatTime( *this, outTimeValue, eOS_SHORT_FORMAT, useGMT); 
		intl->FormatDate( *this, outDateValue, eOS_SHORT_FORMAT, useGMT); 
	}
	else
	{
		outDateValue.Clear();
		outTimeValue.Clear();
	}
}
void VTime::FormatByOS( VString& outValue, bool useGMT) const
{
	if (! IsNull())
	{
		VString date, hour;
		FormatByOS(date,hour,useGMT);
		outValue = date + " " + hour;
	}
	else
	{
		outValue.Clear();
	}
}


void VTime::GetXMLString( VString& outString, XMLStringOptions inOptions) const
{
	GetRfc3339String( outString, inOptions);
}


bool VTime::FromXMLString( const VString& inString)
{
	if (inString.FindUniChar('!') > 0)	// 'simple date' format for wakanda bridge
	{
		FromString(inString);
		if (fYear == 0 && fMonth == 0 && fDay == 0)
			SetNull(true);
		return true;
	}
	return FromRfc3339String( inString);
}


void VTime::GetRfc3339String( VString& outString, XMLStringOptions inOptions) const
{
	if (!IsNull())
	{
		sWORD year, month, day, hour, minute, second, millisecond;
		char buffer[255];
		if (inOptions & XSO_Time_UTC)
		{
			//datetime or time format with GMT+0 timezone
			GetUTCTime(year, month, day, hour, minute, second, millisecond);
			if (inOptions & XSO_Time_TimeOnly)
			{
				#if COMPIL_VISUAL
				sprintf( buffer, "%02d:%02d:%02dZ", hour, minute, second);
				#else
				snprintf( buffer, sizeof( buffer), "%02d:%02d:%02dZ", hour, minute, second);
				#endif
			}
			else if (inOptions & XSO_Time_DateOnly)
			{
				#if COMPIL_VISUAL
				sprintf( buffer, "%04d-%02d-%02dZ", year, month, day);
				#else
				snprintf( buffer, sizeof( buffer), "%04d-%02d-%02dZ", year, month, day);
				#endif
			}
			else
			{
				if (inOptions & XSO_Time_DateNull)
					year = month = day = 0;
				#if COMPIL_VISUAL
				sprintf( buffer, "%04d-%02d-%02dT%02d:%02d:%02dZ", year, month, day, hour, minute, second);
				#else
				snprintf( buffer, sizeof( buffer), "%04d-%02d-%02dT%02d:%02d:%02dZ", year, month, day, hour, minute, second);
				#endif
			}
		}
		else if (inOptions & XSO_Time_NoTimezone)
		{
			//datetime or time format without timezone (local time)
			GetLocalTime(year, month, day, hour, minute, second, millisecond);
			if (inOptions & XSO_Time_TimeOnly)
			{
				#if COMPIL_VISUAL
				sprintf( buffer, "%02d:%02d:%02d", hour, minute, second);
				#else
				snprintf( buffer, sizeof( buffer), "%02d:%02d:%02d", hour, minute, second);
				#endif
			}
			else if (inOptions & XSO_Time_DateOnly)
			{
				#if COMPIL_VISUAL
				sprintf( buffer, "%04d-%02d-%02d", year, month, day);
				#else
				snprintf( buffer, sizeof( buffer), "%04d-%02d-%02d", year, month, day);
				#endif
			}
			else
			{
				if (inOptions & XSO_Time_DateNull)
					year = month = day = 0;

				#if COMPIL_VISUAL
				sprintf( buffer, "%04d-%02d-%02dT%02d:%02d:%02d", year, month, day, hour, minute, second);
				#else
				snprintf( buffer, sizeof( buffer), "%04d-%02d-%02dT%02d:%02d:%02d", year, month, day, hour, minute, second);
				#endif
			}
		}
		else
		{
			//datetime or time format with timezone (local time)
			sLONG offset = VSystem::GetGMTOffset( true); // includes daylight offset
			bool timeZoneAdd = true;
			if (offset < 0)
			{
				timeZoneAdd = false;
				offset = -offset;
			}
			GetLocalTime(year, month, day, hour, minute, second, millisecond);
			if (timeZoneAdd)
			{
				if (inOptions & XSO_Time_TimeOnly)
				{
					#if COMPIL_VISUAL
					sprintf( buffer, "%02d:%02d:%02d+%02d:%02d", hour, minute, second, offset / 3600L, (offset % 3600L) / 60);
					#else
					snprintf( buffer, sizeof( buffer), "%02d:%02d:%02d+%02ld:%02ld", hour, minute, second, offset / 3600L, (offset % 3600L) / 60);
					#endif
				}
				else if (inOptions & XSO_Time_DateOnly)
				{
					#if COMPIL_VISUAL
					sprintf( buffer, "%04d-%02d-%02d+%02d:%02d", year, month, day, offset / 3600L, (offset % 3600L) / 60);
					#else
					snprintf( buffer, sizeof( buffer), "%04d-%02d-%02d+%02ld:%02ld", year, month, day, offset / 3600L, (offset % 3600L) / 60);
					#endif
				}
				else
				{
					if (inOptions & XSO_Time_DateNull)
						year = month = day = 0;

					#if COMPIL_VISUAL
					sprintf( buffer, "%04d-%02d-%02dT%02d:%02d:%02d+%02d:%02d", year, month, day, hour, minute, second, offset / 3600L, (offset % 3600L) / 60);
					#else
					snprintf( buffer, sizeof( buffer), "%04d-%02d-%02dT%02d:%02d:%02d+%02ld:%02ld", year, month, day, hour, minute, second, offset / 3600L, (offset % 3600L) / 60);
					#endif
				}
			}
			else
			{
				if (inOptions & XSO_Time_TimeOnly)
				{
					#if COMPIL_VISUAL
					sprintf( buffer, "%02d:%02d:%02d-%02d:%02d", hour, minute, second, offset / 3600L, (offset % 3600L) / 60);
					#else
					snprintf( buffer, sizeof( buffer), "%02d:%02d:%02d-%02ld:%02ld", hour, minute, second, offset / 3600L, (offset % 3600L) / 60);
					#endif
				}
				else if (inOptions & XSO_Time_DateOnly)
				{
					#if COMPIL_VISUAL
					sprintf( buffer, "%04d-%02d-%02d-%02d:%02d", year, month, day, offset / 3600L, (offset % 3600L) / 60);
					#else
					snprintf( buffer, sizeof( buffer), "%04d-%02d-%02d-%02ld:%02ld", year, month, day, offset / 3600L, (offset % 3600L) / 60);
					#endif
				}
				else
				{
					if (inOptions & XSO_Time_DateNull)
						year = month = day = 0;

					#if COMPIL_VISUAL
					sprintf( buffer, "%04d-%02d-%02dT%02d:%02d:%02d-%02d:%02d", year, month, day, hour, minute, second, offset / 3600L, (offset % 3600L) / 60);
					#else
					snprintf( buffer, sizeof( buffer), "%04d-%02d-%02dT%02d:%02d:%02d-%02ld:%02ld", year, month, day, hour, minute, second, offset / 3600L, (offset % 3600L) / 60);
					#endif
				}
			}
		}
		outString.FromCString( buffer);
	}
	else
	{
		outString.FromCString( "0000-00-00T00:00:00Z");
	}
}


bool VTime::FromRfc3339String( const VString& inString)
{
	bool good = false;
	if ( !inString.IsNull() && inString.GetLength() >= 1)
	{
		char buffer[256];
		inString.ToCString( buffer, sizeof( buffer));
		size_t len = strlen( buffer);
		int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0, milliseconds = 0;
		int count = 0;
		//ensure we use float type for %f 
		float secondReal = (float) 0.0;
		UniChar c = 0;

		bool isDateTimeFormat = true;
		bool isDateFormat = false;
		if (inString.FindUniChar('T') == 11)
		{
			//datetime format
			count = ::sscanf( buffer, "%4d-%2d-%2dT%2d:%2d:%f", &year, &month, &day, &hour, &minute, &secondReal);
			if (year == 0 && month == 0 && day == 0)
			{
				//no date: assume it is only time format
				isDateTimeFormat = (count == 6);
				isDateFormat = false;
				if (count == 6)
					count = 3;
			}
		}
		else if (((c = inString.FindUniChar(':')) >= 1) && c >= 3 && c <= 5) 
		{	
			//time format 
			isDateTimeFormat = false;
			isDateFormat = false;
			count = ::sscanf( buffer, "%d:%2d:%f", &hour, &minute, &secondReal);
		}
		else if (inString.FindUniChar('-') == 5)
		{
			//date format
			isDateTimeFormat = false;
			isDateFormat = true;
			count = ::sscanf( buffer, "%4d-%2d-%2d", &year, &month, &day);
		}
		else
		{
			//get as elapse time in seconds
			isDateTimeFormat = false;
			isDateFormat = false;
			int elapseTime;
			count = ::sscanf( buffer, "%d", &elapseTime);
			hour = elapseTime / 3600;
			elapseTime = elapseTime % 3600;
			minute = elapseTime / 60;
			second = elapseTime % 60;
			if (count == 1)
				count = 3;
		}
		second = (int)secondReal;
		milliseconds = (secondReal-second)*1000;

		bool isUTCFormat = inString.FindUniChar('Z') >= 1;
		bool timeZoneAdd = false;
		//search positive timezone ?
		VIndex posTimeZoneChunk = inString.FindUniChar('+', inString.GetLength(), true);
		bool isTimeZoneFormat = posTimeZoneChunk >= 1;
		if (isTimeZoneFormat)
			timeZoneAdd = true;
		else
		{
			//search negative timezone ?
			posTimeZoneChunk = inString.FindUniChar('-', inString.GetLength(), true);
			if (isDateFormat || isDateTimeFormat)
				isTimeZoneFormat = posTimeZoneChunk > 10;
			else
				isTimeZoneFormat = posTimeZoneChunk >= 1;
			if (isTimeZoneFormat)
				timeZoneAdd = false;
		}

		if(year == 0 && month == 0 && day == 0)
		{
			isTimeZoneFormat = false;
			timeZoneAdd = false;
		}

		if ((isDateTimeFormat && count == 6) 
			|| 
			( !isDateTimeFormat && !isDateFormat && (count == 3 || count == 2))
			||
			(isDateFormat && count == 3)
			||
			(isDateTimeFormat && year == 0 && month == 0 && day == 0 && count == 3)
			)
		{
			if (year == 0 && month == 0 && day == 0 && hour == 0 && minute == 0 && second == 0 && milliseconds == 0)
			{
				SetNull( true);
				good = true;	// valid null;
			}
			else if( year == 0 && month == 0 && day == 0)
			{
				FromUTCTime( year, month, day, hour, minute, second, milliseconds);
				good = true;
			}
			else
			{
				if ( isUTCFormat )
				{
					//date or time is in GMT+0 timezone

					if (isDateTimeFormat)
						//get as UTC date and time
						FromUTCTime( year, month, day, hour, minute, second, 0);
					else if (isDateFormat)
						//get as UTC date 
						FromUTCTime( year, month, day, 0, 0, 0, 0);
					else
					{
						//time format:
						//add time to today time + gmt offset
						FromToday( ( (sLONG) (hour*60*60+minute*60+second) + VSystem::GetGMTOffset( true)) * 1000);
					}
					good = true;
				}
				else if ( isTimeZoneFormat )
				{
					//date or time is in custom timezone

					//parse  custom timezone
					int offset_minute, offset_hour;
					count = ::sscanf( buffer+posTimeZoneChunk, "%2d:%2d", &offset_hour, &offset_minute);
					if (!timeZoneAdd)
					{
						offset_hour = -offset_hour;
						offset_minute = -offset_minute;
					}
					if (count == 2)
					{
						if (isDateTimeFormat)
						{
							//get as local time and add local gmt offset - gmt offset from custom timezone
							FromLocalTime( year, month, day, hour, minute, second, 0);
							//add difference between local timezone and timezone from input string
							sLONG offset = VSystem::GetGMTOffset( true); // includes daylight offset
							AddSeconds( (sLONG) (offset - (offset_hour * 60L + offset_minute)*60));
							good = true;
						}
						else if (isDateFormat)
							//get as local time (as hour is undeterminate we do not consider timezone)
							FromLocalTime( year, month, day, 0, 0, 0, 0);
						else
						{
							//time format:
							//add time to today time + local gmt offset - gmt offset from custom timezone
							sLONG offset = VSystem::GetGMTOffset( true); // includes daylight offset
							FromToday( (sLONG) (hour*60*60+minute*60+second + offset - (offset_hour * 60L + offset_minute)*60L)*1000);
							good = true;
						}
					}
				}
				else
				{
					//date or time is in local timezone 

					if (isDateTimeFormat)
						//get as local time, no offset
						FromLocalTime( year, month, day, hour, minute, second, 0);
					else if (isDateFormat)
						//get as local time 
						FromLocalTime( year, month, day, 0, 0, 0, 0);
					else
						//time format:
						//add time to today time, no offset
						FromToday( (sLONG) (hour*60*60+minute*60+second)*1000);
					good = true;
				}
			}
		}
		if (good && milliseconds)
			AddMilliseconds( milliseconds);
	}
	if (!good)
		SetNull( true);

	return good;
}


void VTime::GetValue(VValueSingle& inValue) const
{
	inValue.FromTime(*this);
}


void VTime::FromString(const VString& inValue)
{
	sWORD ar[] = { 0, 0, 0, 0, 0, 0, 0 };
	UniChar c;
	uLONG pos = 0;
	bool inNumber = false;
	VIndex i;
	VString num;
	bool simpleDate = false;

	for (i = 0; (pos < 7) && (i < inValue.GetLength()); i++)
	{
		c = inValue[ i ];

		if (c >= '0' && c <= '9')
		{
			num.AppendUniChar(c);
			inNumber = true;
		}
		else
		{
			if (c == '!')
			{
				if (pos == 0)
					simpleDate = true;
				else
				{
					if (!simpleDate)
					{
						++pos;
						num.Clear();
						inNumber = false;
						break;
					}
				}
			}
			if (inNumber)
			{
				ar[ pos ] = (sWORD) (num.GetLong());
				num.Clear();
				pos++;
				inNumber = false;
			}
		}
	}

	// flush du dernier nombre
	if (inNumber && pos < 7)
		ar[ pos ] = (sWORD) (num.GetLong());

	if (simpleDate)
		FromUTCTime(ar[2], ar[1], ar[0], 0, 0, 0, 0);
	else
		FromUTCTime(ar[0], ar[1], ar[2], ar[3], ar[4], ar[5], ar[6]);
}


void VTime::FromValue(const VValueSingle& inValue)
{
	inValue.GetTime(*this);
	GotValue(inValue.IsNull());
}


void VTime::GetTime(VTime& outValue) const
{
	outValue.FromTime(*this);
}


void VTime::FromTime(const VTime& inValue)
{
	fYear = inValue.fYear;
	fMonth = inValue.fMonth;
	fDay = inValue.fDay;
	fMilliseconds = inValue.fMilliseconds;
	SetNull(inValue.IsNull());
}


void* VTime::WriteToPtr(void* inDataPtr, Boolean /*inRefOnly*/, VSize inMax) const
{
	*(uLONG8*) inDataPtr = GetStamp();

	return 1 + (uLONG8*) inDataPtr;
}


void* VTime::LoadFromPtr(const void* inDataPtr, Boolean /*inRefOnly*/)
{
	FromStamp( *(uLONG8*) inDataPtr);

	return 1 + (uLONG8*) inDataPtr;
}


VError VTime::ReadFromStream(VStream* inStream, sLONG /*inParam*/)
{
	fYear = inStream->GetWord();
	fMonth = inStream->GetByte();
	fDay = inStream->GetByte();
	fMilliseconds = inStream->GetLong();
	return inStream->GetLastError();
}


VError VTime::WriteToStream(VStream* inStream, sLONG inParam) const
{
	VValue::WriteToStream(inStream, inParam);

	inStream->PutWord(fYear);
	inStream->PutByte(fMonth);
	inStream->PutByte(fDay);
	inStream->PutLong(fMilliseconds);

	return inStream->GetLastError();
}


bool VTime::operator == (const VTime &inTime) const
{
	return CompareTo(inTime)==CR_EQUAL;
}


bool VTime::operator > (const VTime &inTime) const
{
	return CompareTo(inTime)==CR_BIGGER;
}


bool VTime::operator < (const VTime &inTime) const
{
	return CompareTo(inTime)==CR_SMALLER;
}


bool VTime::operator >= (const VTime &inTime) const
{
	return CompareTo(inTime)>=CR_EQUAL;
}


bool VTime::operator <= (const VTime &inTime) const
{
	return CompareTo(inTime)<=CR_EQUAL;
}


bool VTime::operator != (const VTime &inTime) const
{
	return CompareTo(inTime)!=CR_EQUAL;
}


VTime VTime::operator = (const VTime& inValue)
{
	FromTime(inValue); 
	return *this;
}


CompareResult VTime::CompareToSameKind(const VValue* inValue, Boolean /*inDiacritical*/) const
{
	XBOX_ASSERT_KINDOF(VTime, inValue);
	
	const VTime* theValue = reinterpret_cast<const VTime*>(inValue);
	return CompareTo(*theValue);
}


Boolean VTime::EqualToSameKind(const VValue* inValue, Boolean /*inDiacritical*/) const
{
	XBOX_ASSERT_KINDOF(VTime, inValue);
	
	const VTime* theValue = reinterpret_cast<const VTime*>(inValue);
	return CompareTo(*theValue) == CR_EQUAL;
}


Boolean VTime::FromValueSameKind(const VValue* inValue)
{
	XBOX_ASSERT_KINDOF(VTime, inValue);
	
	const VTime* theValue = reinterpret_cast<const VTime*>(inValue);
	FromTime(*theValue);
	GotValue(theValue->IsNull());
	return true;
}

void VTime::Add(const VDuration& inDuration)
{
	sLONG8 m = GetMilliseconds();
	m += inDuration.ConvertToMilliseconds();
	FromMilliseconds(m);
}


void VTime::Substract(const VTime& inTime, VDuration& outDuration) const
{
	sLONG8 m1 = GetMilliseconds();
	sLONG8 m2 = inTime.GetMilliseconds();

	outDuration.FromNbMilliseconds(m1 - m2);
}

void VTime::Substract(const VDuration& inDuration)
{
	sLONG8 m = GetMilliseconds();
	m -= inDuration.ConvertToMilliseconds();
	FromMilliseconds(m);
}

void VTime::Now(VTime &outTime)
{
	sWORD localTime[7];

	VSystem::GetSystemLocalTime (localTime);
	outTime.FromLocalTime(localTime[0], localTime[1], localTime[2], localTime[3], localTime[4], localTime[5], localTime[6]);
}

static sWORD sNbDayInMonthArray[] = { 0, 306, 337, 0, 31, 61, 92, 122, 153, 184, 214, 245, 275 };
static sWORD sNbDaysPerMonth[] = { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }; 


// calcule le nombre de millisecondes ecoulees depuis le jour julien 1
// ce nombre permet d'effectuer des calculs
sLONG8 VTime::GetMilliseconds() const
{
	if (IsNull())
		return -1;
	else
	{
		sLONG8 jDay = GetJulianDay();

		return (jDay * 24 * 60 * 60 * 1000) + fMilliseconds;
	}
}


// retrouve une date a partir du nombre de millisecondes ecoulees depuis le jour julien 1
void VTime::FromMilliseconds(sLONG8 inMilliseconds)
{
	if (inMilliseconds == -1)
		GotValue(true);
	else
	{
		sLONG jDay = (sLONG) (inMilliseconds / (24 * 60 * 60 * 1000));
		FromJulianDay(jDay);

		fMilliseconds = (sLONG) (inMilliseconds % (24 * 60 * 60 * 1000));
		GotValue();
	}
}


bool VTime::CheckDate( sWORD inYear, sWORD inMonth, sWORD inDay)
{
	return (inYear >= 0)
		&& (inMonth >= 1 && inMonth <= 12)
		&& (inDay >= 1 && inDay <= sNbDaysPerMonth[ inMonth ])
		&& (inDay != 29 || inMonth != 2 || IsLeapYear(inYear) );
}


void VTime::GetLocalTime(sWORD& outYear, sWORD& outMonth, sWORD& outDay, sWORD& outHour, sWORD& outMinute, sWORD& outSecond, sWORD& outMillisecond) const
{
	sWORD	lt[7];
	
	GetUTCTime(lt[0], lt[1], lt[2], lt[3], lt[4], lt[5], lt[6]);
	VSystem::UTCToLocalTime(lt);

	outYear = lt[0];
	outMonth = lt[1]; 
	outDay = lt[2]; 
	outHour = lt[3]; 
	outMinute = lt[4]; 
	outSecond = lt[5]; 
	outMillisecond = lt[6]; 
}


void VTime::FromLocalTime(sWORD inYear, sWORD inMonth, sWORD inDay, sWORD inHour, sWORD inMinute, sWORD inSecond, sWORD inMillisecond)
{	
	sWORD	lt[7];

	lt[0] = inYear;
	lt[1] = inMonth; 
	lt[2] = inDay; 
	lt[3] = inHour; 
	lt[4] = inMinute; 
	lt[5] = inSecond; 
	lt[6] = inMillisecond; 

	VSystem::LocalToUTCTime(lt);

	FromUTCTime(lt[0], lt[1], lt[2], lt[3], lt[4], lt[5], lt[6]);
}


void VTime::GetUTCTime(sWORD& outYear, sWORD& outMonth, sWORD& outDay, sWORD& outHour, sWORD& outMinute, sWORD& outSecond, sWORD& outMillisecond) const
{
	if (IsNull())
		outYear = outMonth = outDay = outHour = outMinute = outSecond = outMillisecond = 0;
	else
	{
		outYear = fYear;
		outMonth = fMonth;
		outDay = fDay;
		outHour = (sWORD) (fMilliseconds / (60L * 60L * 1000L));
		outMinute = (sWORD) ((fMilliseconds % (60L * 60L * 1000L)) / (60L * 1000L));
		outSecond = (sWORD) ((fMilliseconds % (60L * 1000L)) / 1000L);
		outMillisecond = (sWORD) (fMilliseconds % 1000L);
	}
}

void VTime::FromUTCTime(sWORD inYear, sWORD inMonth, sWORD inDay, sWORD inHour, sWORD inMinute, sWORD inSecond, sWORD inMillisecond)
{
	if ( (inYear == 0 && inMonth == 0 && inDay == 0) || (CheckDate(inYear, inMonth, inDay)
		 && (inHour >= 0 && inHour < 24)
		 && (inMinute >= 0 && inMinute < 60)
		 && (inSecond >= 0 && inSecond < 60)
		 && (inMillisecond >= 0 && inMillisecond < 1000))
	 )
	{
		fYear = inYear;
		fMonth = (sBYTE) inMonth;
		fDay = (sBYTE) inDay;
		
		fMilliseconds = (inHour * 60 * 60 * 1000)
						+ (inMinute * 60 * 1000)
						+ (inSecond * 1000)
						+ (inMillisecond );

		GotValue();
	}
	else
	{
		Clear();
		//GotValue(true);
		SetNull(true);
	}
}


sLONG VTime::GetWeekDay() const
{
	if ( IsNull() )
		return -1;

	sLONG jd = GetJulianDay();
	return (jd + 2) % 7;
}


/*
	static
*/
sLONG VTime::ComputeWeekDay( sLONG inYear, sLONG inMonth, sLONG inDay)
{
	sLONG jd = ComputeJulianDay( inYear, inMonth, inDay);
	return (jd + 2) % 7;
}


/*
	static
*/
sLONG VTime::ComputeYearDay( sLONG inYear, sLONG inMonth, sLONG inDay)
{
	sLONG jd = ComputeJulianDay( inYear, inMonth, inDay);
	sLONG jdsy = ComputeJulianDay( inYear, 1, 1);
	return jd - jdsy + 1;
}


sLONG VTime::GetYearDay() const
{
	if ( IsNull() )
		return -1;

	sLONG jd = GetJulianDay();
	VTime startOfYear;
	startOfYear.FromUTCTime( fYear, 1, 1, 0, 0, 0, 0 );
	sLONG jdsy = startOfYear.GetJulianDay();

	return jd - jdsy + 1;
}


void VTime::FromSystemTime()
{
	sWORD vals[7];

	VSystem::GetSystemUTCTime(vals);

	fYear = vals[0];
	fMonth = (sBYTE) vals[1];
	fDay = (sBYTE) vals[2];
	fMilliseconds = (vals[3] * 60 * 60 * 1000)		// heures
					+ (vals[4] * 60 * 1000)		// minute
					+ (vals[5] * 1000)		// secondes
					+ (vals[6] );		// millisecondes

	GotValue();
}


// attention, le stamp regroupe sur 8 octets les differents champs
// il permet uniquement des comparaisons mais pas des calculs
uLONG8 VTime::GetStamp() const
{
	uLONG8 val;
	
	val = ((uLONG8) fYear) << 48;
	val |= ((uLONG8) fMonth) << 40;
	val |= ((uLONG8) fDay ) << 32;
	val |= (uLONG8) fMilliseconds;

	return val;
}


// attention, la date n'est pas verifiee, puisqu'elle est censee provenir d'un VTime valide...
void VTime::FromStamp(uLONG8 inStamp)
{
	fYear = (sWORD) ((inStamp & XBOX_LONG8(0xFFFF000000000000)) >> 48);
	fMonth = (sBYTE) ((inStamp & XBOX_LONG8(0x0000FF0000000000)) >> 40);
	fDay = (sBYTE) ((inStamp & XBOX_LONG8(0x000000FF00000000)) >> 32);
	fMilliseconds = (sLONG) ((inStamp & XBOX_LONG8(0x00000000FFFFFFFF)));

	GotValue(false);
}


void VTime::AddYears(sLONG inYears)
{
	fYear += inYears;
}


void VTime::AddMonths(sLONG inMonths)
{
	sLONG m = fMonth + inMonths;

	fMonth = (sBYTE) (((m - 1) % 12) + 1);
	fYear += (sWORD) ((m - 1) / 12);
}


void VTime::AddDays(sLONG inDays)
{
	sLONG8 val = GetMilliseconds();
	val += static_cast<sLONG8>(inDays) * 24 * 60 * 60 * 1000;
	FromMilliseconds(val);
}


void VTime::AddHours(sLONG inHours)
{
	sLONG8 val = GetMilliseconds();
	val += static_cast<sLONG8>(inHours) * 60 * 60 * 1000;
	FromMilliseconds(val);
}


void VTime::AddMinutes(sLONG inMinutes)
{
	sLONG8 val = GetMilliseconds();
	val += static_cast<sLONG8>(inMinutes) * 60 * 1000;
	FromMilliseconds(val);
}


void VTime::AddSeconds(sLONG inSeconds)
{
	sLONG8 val = GetMilliseconds();
	val += static_cast<sLONG8>(inSeconds) * 1000;
	FromMilliseconds(val);
}


void VTime::AddMilliseconds(sLONG8 inMilliseconds)
{
	sLONG8 val = GetMilliseconds();
	val += inMilliseconds;
	FromMilliseconds(val);
}


sWORD VTime::GetLocalYear() const
{
	sWORD year, month, day, hour, minute, second, millisecond;
	GetLocalTime(year, month, day, hour, minute, second, millisecond);

	return year;
}


sWORD VTime::GetLocalMonth() const
{
	sWORD year, month, day, hour, minute, second, millisecond;
	GetLocalTime(year, month, day, hour, minute, second, millisecond);

	return month;
}


sWORD VTime::GetLocalDay() const
{
	sWORD year, month, day, hour, minute, second, millisecond;
	GetLocalTime(year, month, day, hour, minute, second, millisecond);

	return day;
}


sWORD VTime::GetLocalHour() const
{
	sWORD year, month, day, hour, minute, second, millisecond;
	GetLocalTime(year, month, day, hour, minute, second, millisecond);

	return hour;
}


sWORD VTime::GetLocalMinute() const
{
	sWORD year, month, day, hour, minute, second, millisecond;
	GetLocalTime(year, month, day, hour, minute, second, millisecond);

	return minute;
}


sWORD VTime::GetLocalSecond() const
{
	sWORD year, month, day, hour, minute, second, millisecond;
	GetLocalTime(year, month, day, hour, minute, second, millisecond);

	return second;
}


sWORD VTime::GetLocalMillisecond() const
{
	sWORD year, month, day, hour, minute, second, millisecond;
	GetLocalTime(year, month, day, hour, minute, second, millisecond);

	return millisecond;
}


VDuration VTime::GetLocalDurationSinceMidnight() const
{
	sWORD year, month, day, hour, minute, second, millisecond;
	GetLocalTime(year, month, day, hour, minute, second, millisecond);

	return VDuration( (sLONG8)hour*3600000 + (sLONG8)minute*60000 + (sLONG8)second * 1000 + (sLONG8)millisecond);
}


void VTime::FromRfc822String(XBOX::VString &inStr)
{
	// currently it only supports with GMT string:
	// Wed, 02 Oct 2002 13:00:00 GMT

	sWORD year = 0;
	sWORD month_num = 0;
	sWORD day = 0;
	sWORD hours = 0;
	sWORD minutes = 0;
	sWORD seconds = 0;

	// first, go straight to the day's number:
	VIndex str_len = inStr.GetLength();
	VIndex lap_index = 0;
	VIndex begin_num = 0;

	bool found = false;

	for (lap_index=0;lap_index<str_len;++lap_index)
	{
		if (XBOX::VIntlMgr::GetDefaultMgr()->IsDigit(inStr[lap_index]))
		{
			// we found the beginning of a number...
			begin_num = lap_index;
			++lap_index;

			// nom find the end of the number
			while ((lap_index<str_len) && XBOX::VIntlMgr::GetDefaultMgr()->IsDigit(inStr[lap_index]))
				++lap_index;
			found = true;
			break;
		}
	}

	if (found)
	{
		XBOX::VString str_number;

		inStr.GetSubString(begin_num+1,lap_index-begin_num,str_number);
		day = str_number.GetLong();

		found = false;
		begin_num = 0;

		// we continue from the same offset, looking for the year:
		for (;lap_index<str_len;++lap_index)
		{
			if (XBOX::VIntlMgr::GetDefaultMgr()->IsAlpha(inStr[lap_index]))
			{
				// we found the beginning of a number...
				begin_num = lap_index;
				++lap_index;

				// nom find the end of the number
				while ((lap_index<str_len) && XBOX::VIntlMgr::GetDefaultMgr()->IsAlpha(inStr[lap_index]))
					++lap_index;
				found = true;
				break;
			}
		}

		if (found)
		{
			XBOX::VString str_monthname;

			inStr.GetSubString(begin_num+1,lap_index-begin_num,str_monthname);

			month_num = 0;

			if (str_monthname == "Jan")
				month_num = 1;
			else if (str_monthname == "Feb")
				month_num = 2;
			else if (str_monthname == "Mar")
				month_num = 3;
			else if (str_monthname == "Apr")
				month_num = 4;
			else if (str_monthname == "May")
				month_num = 5;
			else if (str_monthname == "Jun")
				month_num = 6;
			else if (str_monthname == "Jul")
				month_num = 7;
			else if (str_monthname == "Aug")
				month_num = 8;
			else if (str_monthname == "Sep")
				month_num = 9;
			else if (str_monthname == "Oct")
				month_num = 10;
			else if (str_monthname == "Nov")
				month_num = 11;
			else if (str_monthname == "Dec")
				month_num = 12;

			found = false;
			begin_num = 0;

			// we continue from the same offset, looking for the month's name:
			for (;lap_index<str_len;++lap_index)
			{
				if (XBOX::VIntlMgr::GetDefaultMgr()->IsDigit(inStr[lap_index]))
				{
					// we found the beginning of a number...
					begin_num = lap_index;
					++lap_index;

					// nom find the end of the number
					while ((lap_index<str_len) && XBOX::VIntlMgr::GetDefaultMgr()->IsDigit(inStr[lap_index]))
						++lap_index;
					found = true;
					break;
				}
			}

			if (found)
			{
				inStr.GetSubString(begin_num+1,lap_index-begin_num,str_number);
				year = str_number.GetLong();

				found = false;
				begin_num = 0;

				// we continue from the same offset, looking for the month's name:
				for (;lap_index<str_len;++lap_index)
				{
					if (XBOX::VIntlMgr::GetDefaultMgr()->IsDigit(inStr[lap_index]))
					{
						// we found the beginning of a number...
						begin_num = lap_index;
						++lap_index;

						// nom find the end of the number
						while ((lap_index<str_len) && XBOX::VIntlMgr::GetDefaultMgr()->IsDigit(inStr[lap_index]))
							++lap_index;
						found = true;
						break;
					}
				}

				if (found)
				{
					inStr.GetSubString(begin_num+1,lap_index-begin_num,str_number);
					hours = str_number.GetLong();

					found = false;
					begin_num = 0;

					// we continue from the same offset, looking for the month's name:
					for (;lap_index<str_len;++lap_index)
					{
						if (XBOX::VIntlMgr::GetDefaultMgr()->IsDigit(inStr[lap_index]))
						{
							// we found the beginning of a number...
							begin_num = lap_index;
							++lap_index;

							// nom find the end of the number
							while ((lap_index<str_len) && XBOX::VIntlMgr::GetDefaultMgr()->IsDigit(inStr[lap_index]))
								++lap_index;
							found = true;
							break;
						}
					}

					if (found)
					{
						inStr.GetSubString(begin_num+1,lap_index-begin_num,str_number);
						minutes = str_number.GetLong();

						found = false;
						begin_num = 0;

						// we continue from the same offset, looking for the month's name:
						for (;lap_index<str_len;++lap_index)
						{
							if (XBOX::VIntlMgr::GetDefaultMgr()->IsDigit(inStr[lap_index]))
							{
								// we found the beginning of a number...
								begin_num = lap_index;
								++lap_index;

								// nom find the end of the number
								while ((lap_index<str_len) && XBOX::VIntlMgr::GetDefaultMgr()->IsDigit(inStr[lap_index]))
									++lap_index;
								found = true;
								break;
							}
						}

						if (found)
						{
							inStr.GetSubString(begin_num+1,lap_index-begin_num,str_number);
							seconds = str_number.GetLong();
						}
					}

				}
			}
		}
	}

	FromUTCTime(year, month_num, day, hours, minutes, seconds, 0);
}


void VTime::GetRfc822String( VString& outString) const
{
	// it assumes that the VTime is a GMT value
	// ---> Wed, 02 Oct 2002 13:00:00 GMT

	sWORD year, month, day, hour, minute, second, millisecond;
	GetUTCTime (year, month, day, hour, minute, second, millisecond);

	outString.Clear();

	switch (GetWeekDay())
	{
		case 0: outString.AppendCString("Sun"); break;
		case 1: outString.AppendCString("Mon"); break;
		case 2: outString.AppendCString("Tue"); break;
		case 3: outString.AppendCString("Wed"); break;
		case 4: outString.AppendCString("Thu"); break;
		case 5: outString.AppendCString("Fri"); break;
		case 6: outString.AppendCString("Sat"); break;
	}

	outString.AppendPrintf (", %02d ", day);	// YT 29-Apr-2010 - ACI0062699 - was: GetLocalDay()

	switch (month)	// YT 29-Apr-2010 - ACI0062699 - was: GetLocalMonth()
	{
		case 1: outString.AppendCString("Jan"); break;
		case 2: outString.AppendCString("Feb"); break;
		case 3: outString.AppendCString("Mar"); break;
		case 4: outString.AppendCString("Apr"); break;
		case 5: outString.AppendCString("May"); break;
		case 6: outString.AppendCString("Jun"); break;
		case 7: outString.AppendCString("Jul"); break;
		case 8: outString.AppendCString("Aug"); break;
		case 9: outString.AppendCString("Sep"); break;
		case 10: outString.AppendCString("Oct"); break;
		case 11: outString.AppendCString("Nov"); break;
		case 12: outString.AppendCString("Dec"); break;
	}

	outString.AppendPrintf (" %04d %02d:%02d:%02d GMT", year, hour, minute, second);
}

void VTime::FromCompilerDateTime(const char *inDate, const char *inTime)
{
	// date is like "Jul 14, 1985"
	// time is like "13:01:45"

	char buff[255];

	::strcpy(buff, inDate);
	::strcat(buff, " ");
	::strcat(buff, inTime);

	sLONG index = 0;

	sWORD year = 0;
	sWORD month = 0;
	sWORD day = 0;
	sWORD hours = 0;
	sWORD minutes = 0;
	sWORD seconds = 0;
	for(char *token = ::strtok(buff, " ,:") ; (token != NULL) && index < 6 ; token = ::strtok(NULL, " ,:")) {
		switch(++index) {
		
			case 1:
				if (strcmp("jan", token) == 0 || strcmp("Jan", token) == 0)
					month = 1;
				else if (strcmp("feb", token) == 0 || strcmp("Jan", token) == 0)
					month = 2;
				else if (strcmp("mar", token) == 0 || strcmp("Mar", token) == 0)
					month = 3;
				else if (strcmp("apr", token) == 0 || strcmp("Apr", token) == 0)
					month = 4;
				else if (strcmp("may", token) == 0 || strcmp("May", token) == 0)
					month = 5;
				else if (strcmp("jun", token) == 0 || strcmp("Jun", token) == 0)
					month = 6;
				else if (strcmp("jul", token) == 0 || strcmp("Jul", token) == 0)
					month = 7;
				else if (strcmp("aug", token) == 0 || strcmp("Aug", token) == 0)
					month = 8;
				else if (strcmp("sep", token) == 0 || strcmp("Sep", token) == 0)
					month = 9;
				else if (strcmp("oct", token) == 0 || strcmp("Oct", token) == 0)
					month = 10;
				else if (strcmp("nov", token) == 0 || strcmp("Nov", token) == 0)
					month = 11;
				else if (strcmp("dec", token) == 0 || strcmp("Dec", token) == 0)
					month = 12;
				break;
			
			case 2:
				day = (sWORD) atoi(token);
				break;
			
			case 3:
				year = (sWORD) atoi(token);
				break;
			
			case 4:
				hours = (sWORD) atoi(token);
				break;
			
			case 5:
				minutes = (sWORD) atoi(token);
				break;
			
			case 6:
				seconds = (sWORD) atoi(token);
				break;
			
			default:
				break;
				
		}
	}
	FromLocalTime(year, month, day, hours, minutes, seconds, 0);
}


void VTime::FromToday( sLONG inMillisecondsSinceMidnight)
{
	sWORD localTime[7];

	VSystem::GetSystemLocalTime (localTime);
	FromLocalTime(localTime[0], localTime[1], localTime[2], 0, 0, 0, 0);
	AddMilliseconds( inMillisecondsSinceMidnight);
}


void VTime::NullIfZero()
{
	if (fDay == 0 && fMonth == 0 && fYear == 0)
		SetNull(true);
}



void VTime::SetLocalYear(sLONG inYear)
{
	sWORD year, month, day, hour, minute, second, millisecond;

	GetLocalTime(year, month, day, hour, minute, second, millisecond);
	year = (sWORD) inYear;
	FromLocalTime(year, month, day, hour, minute, second, millisecond);
}


void VTime::SetLocalMonth(sLONG inMonth)
{
	sWORD year, month, day, hour, minute, second, millisecond;

	GetLocalTime(year, month, day, hour, minute, second, millisecond);

	month = (sWORD) (((inMonth - 1) % 12) + 1);
	year += (sWORD) ((inMonth - 1) / 12);

	FromLocalTime(year, month, day, hour, minute, second, millisecond);
}


void VTime::SetLocalDay(sLONG inDay)
{
	sWORD year, month, day, hour, minute, second, millisecond;
	VTime local;
	sLONG jd;

	GetLocalTime(year, month, day, hour, minute, second, millisecond);
	local.FromUTCTime(year, month, day, hour, minute, second, millisecond);

	jd = local.GetJulianDay();
	jd -= day;
	jd += inDay;
	local.FromJulianDay(jd);

	FromLocalTime(local.fYear, local.fMonth, local.fDay, hour, minute, second, millisecond);
}


void VTime::SetLocalHour(sLONG inHour)
{
	sWORD year, month, day, hour, minute, second, millisecond;
	VTime local;
	sLONG8 ms;

	GetLocalTime(year, month, day, hour, minute, second, millisecond);
	local.FromUTCTime(year, month, day, 0, minute, second, millisecond);

	// fixe les heures
	ms = local.GetMilliseconds();
	ms += inHour * (60 * 60 * 1000);
	local.FromMilliseconds(ms);

	local.GetUTCTime(year, month, day, hour, minute, second, millisecond);
	FromLocalTime(year, month, day, hour, minute, second, millisecond);
}


void VTime::SetLocalMinute(sLONG inMinute)
{
	sWORD year, month, day, hour, minute, second, millisecond;
	VTime local;
	sLONG8 ms;

	GetLocalTime(year, month, day, hour, minute, second, millisecond);
	local.FromUTCTime(year, month, day, hour, 0, second, millisecond);

	// fixe les minutes
	ms = local.GetMilliseconds();
	ms += inMinute * (60 * 1000);
	local.FromMilliseconds(ms);

	local.GetUTCTime(year, month, day, hour, minute, second, millisecond);
	FromLocalTime(year, month, day, hour, minute, second, millisecond);
}


void VTime::SetLocalSecond(sLONG inSecond)
{
	sWORD year, month, day, hour, minute, second, millisecond;
	VTime local;
	sLONG8 ms;

	GetLocalTime(year, month, day, hour, minute, second, millisecond);
	local.FromUTCTime(year, month, day, hour, minute, 0, millisecond);

	// fixe les secondes
	ms = local.GetMilliseconds();
	ms += inSecond * (1000);
	local.FromMilliseconds(ms);

	local.GetUTCTime(year, month, day, hour, minute, second, millisecond);
	FromLocalTime(year, month, day, hour, minute, second, millisecond);
}


void VTime::SetLocalMillisecond(sLONG inMillisecond)
{
	sWORD year, month, day, hour, minute, second, millisecond;
	VTime local;
	sLONG8 ms;

	GetLocalTime(year, month, day, hour, minute, second, millisecond);
	local.FromUTCTime(year, month, day, hour, minute, second, 0);

	// fixe les millisecondes
	ms = local.GetMilliseconds();
	ms += inMillisecond;
	local.FromMilliseconds(ms);

	local.GetUTCTime(year, month, day, hour, minute, second, millisecond);
	FromLocalTime(year, month, day, hour, minute, second, millisecond);
}


/*
	static
*/
sLONG VTime::ComputeJulianDay( sLONG inYear, sLONG inMonth, sLONG inDay)
{
	sWORD z, f;
	sLONG jd;

	if (inMonth < 3)
		z = (sWORD) (inYear - 1);
	else
		z = inYear;

	f = sNbDayInMonthArray[ inMonth ];

	jd = (sLONG) (inDay + f + 365 * z + floor((double) z / 4.0) - floor((double) z / 100.0) + floor((double) z / 400.0) + 1721118);

	return jd;
}


sLONG VTime::GetJulianDay() const
{
	sLONG jd;

	if (IsNull())
		jd = -1;
	else
		jd = ComputeJulianDay( fYear, fMonth, fDay);

	return jd;
}


void VTime::FromJulianDay(sLONG inJulianDay)
{
	sLONG z;
	sLONG a;
	sLONG b;
	sLONG h;
	sLONG c;

	z = inJulianDay - 1721118;
	h = (100 * z) - 25;
	a = (sLONG) (floor((double) h / 3652425.0));
	b = (sLONG) (a - floor((double) a / 4.0));
	
	fYear = (sWORD) (floor((double) ((100 * b) + h) / 36525.0));
	c = (sLONG) (b + z - (365 * fYear) - floor((double) fYear / 4.0));

	fMonth = (sBYTE) ((5 * c + 456) / 153);
	fDay = (sBYTE) (c - (153 * fMonth - 457) / 5);

	if (fMonth > 12)
	{
		fYear++;
		fMonth -= 12;
	}

	fMilliseconds = 0;

	SetNull(false);
}


bool VTime::IsLeapYear(sWORD inYear)
{
	// dans le calendrier gregorien, une annee est bissextile 
	// si elle est divisible par 4 sauf si elle est divisible par 100
	// a moins qu'elle ne soit divisible par 400
	return ((0 == inYear % 4) && (0 != inYear % 100)) || (0 == inYear % 400);
}


void VTime::GetJulianDate(sWORD& outYear, sWORD& outMonth, sWORD& outDay) const
{
	sLONG jd;
	sLONG z;
	sLONG c;

	if (IsNull())
	{
		outYear = 0;
		outMonth = 0;
		outDay = 0;
	}
	else
	{
		jd = GetJulianDay();
		z = jd - 1721116;

		outYear = (sWORD) (floor((double) ((100 * z) - 25) / 36525.0));
		c = (sLONG) (z - 365 * outYear - floor((double) outYear / 4.0));
		outMonth = (sWORD) ((5 * c + 456) / 153);
		outDay = (sWORD) (c - (153 * outMonth - 457) / 5);
		if (outMonth > 12)
		{
			outYear++;
			outMonth -= 12;
		}
	}
}


void VTime::FromJulianDate(sWORD inYear, sWORD inMonth, sWORD inDay)
{
	sWORD f;
	sLONG jd;

	if (inMonth < 1 || inMonth > 12)
		SetNull(true);
	else
	{
		if (inDay < 1 || inDay > sNbDaysPerMonth[ inMonth ])
			SetNull(true);
		else
		{
			// on interdit le 29 fevrier pour les annees bissextiles
			// petite entorse historique puisque les romains vivaient 
			// deux fois le 24 fevrier au lieu d'avoir un 29 fevrier
			if (inDay == 29 && inMonth == 2 && (fYear % 4))
				SetNull(true);
			else
			{
				sLONG z;
				if (inMonth < 3)
					z = inYear - 1;
				else
					z = inYear;

				f = sNbDayInMonthArray[ (sLONG)fMonth ];

				jd = (sLONG) (inDay + f + 365 * z + floor((double) z / 4.0) + 1721116);

				FromJulianDay(jd);
			}
		}
	}
}


VError VTime::FromJSONString(const VString& inJSONString, JSONOption inModifier)
{
	FromXMLString(inJSONString);
	return VE_OK;
}


VError VTime::GetJSONString(VString& outJSONString, JSONOption inModifier) const
{
	GetXMLString(outJSONString, XSO_Default);
	if ((inModifier & JSON_WithQuotesIfNecessary) != 0)
	{
		outJSONString = L"\""+outJSONString+L"\"";
	}
	return VE_OK;
}


VError VTime::FromJSONValue( const VJSONValue& inJSONValue)
{
	if (inJSONValue.IsNull())
		SetNull( true);
	else
	{
		VString s;
		inJSONValue.GetString( s);
		FromXMLString( s);
	}
	return VE_OK;
}


VError VTime::GetJSONValue( VJSONValue& outJSONValue) const
{
	if (IsNull())
		outJSONValue.SetNull();
	else
	{
		VString s;
		GetJSONString( s, JSON_Default);
		outJSONValue.SetString( s);
	}
	return VE_OK;
}


const VValueInfo *VTime::GetValueInfo() const
{
	return &sInfo;
}

Real VTime::GetReal ( ) const
{
	return GetStamp ( );
}

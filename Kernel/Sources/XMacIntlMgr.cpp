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
#include "VTextConverter.h"
#include "VString.h"
#include "VIntlMgr.h"
#include "XMacTextConverter.h"
#include "XMacIntlMgr.h"


XMacIntlMgr::XMacIntlMgr( DialectCode inDialect)
: fDialect( inDialect)
, fCFLocaleRef( NULL)
, fWeekdaySymbols( NULL)
, fMonthSymbols( NULL)
, fShortWeekdaySymbols( NULL)
, fShortMonthSymbols( NULL)
{
}


XMacIntlMgr::XMacIntlMgr( const XMacIntlMgr& inOther)
: fDialect( inOther.fDialect)
, fCFLocaleRef( NULL)	// CFLocaleRef is supposed to be non thread safe. Don't retain it
, fWeekdaySymbols( NULL)
, fMonthSymbols( NULL)
, fShortWeekdaySymbols( NULL)
, fShortMonthSymbols( NULL)
{
}


XMacIntlMgr::~XMacIntlMgr()
{
	if (fWeekdaySymbols != NULL)
		CFRelease( fWeekdaySymbols);
	if (fMonthSymbols != NULL)
		CFRelease( fMonthSymbols);
	if (fShortWeekdaySymbols != NULL)
		CFRelease( fShortWeekdaySymbols);
	if (fShortMonthSymbols != NULL)
		CFRelease( fShortMonthSymbols);
	if (fCFLocaleRef != NULL)
		CFRelease( fCFLocaleRef);
}


/*
	static
*/
CFLocaleRef XMacIntlMgr::CreateCFLocaleRef( DialectCode inDialect)
{
	VString language;
	VIntlMgr::GetBCP47LanguageCode( inDialect, language);
	
	// Create the locale.
	CFStringRef langStr = language.MAC_RetainCFStringCopy();
	CFLocaleRef locale = ::CFLocaleCreate(NULL,langStr);
	::CFRelease( langStr);

	return locale;
}


CFLocaleRef XMacIntlMgr::GetCFLocaleRef() const
{
	if (fCFLocaleRef == NULL)
	{
		fCFLocaleRef = CreateCFLocaleRef( fDialect);
	}
	return fCFLocaleRef;
}


CFLocaleRef XMacIntlMgr::RetainCFLocaleRef() const
{
	CFLocaleRef ref = GetCFLocaleRef();
	if (ref != NULL)
		::CFRetain( ref);
	return ref;
}


bool XMacIntlMgr::ToUpperLowerCase(const UniChar* inSrc, sLONG inSrcLen, UniPtr outDst, sLONG& ioDstLen, bool inIsUpper)
{
	xbox_assert(inSrcLen >= 0); // doit etre verifie par l'appelant
	
	CFMutableStringRef mstr = CFStringCreateMutable( kCFAllocatorDefault, 0);
	
	CFStringAppendCharacters( mstr, inSrc, inSrcLen);

	if ( inIsUpper )
		CFStringUppercase(mstr, NULL);
	else
		CFStringLowercase(mstr, NULL);
	
	CFIndex len = ::CFStringGetLength( mstr );
	CFIndex stringLength = Min<CFIndex>(len, ioDstLen);
	ioDstLen = (sLONG) len;

	const UniChar*	uniPtr = ::CFStringGetCharactersPtr( mstr );

	if (uniPtr != NULL)
	{
		::memcpy( outDst, uniPtr, stringLength * sizeof(UniChar));
	}
	else
	{
		CFIndex	rangeStart = 0;
		UniChar	buffer[1024];
		do
		{
			CFIndex count = Min<CFIndex>(stringLength, sizeof(buffer));
			stringLength -= count;
			
			::CFStringGetCharacters(mstr, CFRangeMake(rangeStart, count), buffer);
			::memcpy( outDst + rangeStart, buffer, count * sizeof( UniChar) );
			
			rangeStart += count;
		}
		while( stringLength > 0);
	}
	CFRelease ( mstr );

	return (ioDstLen != 0) || (inSrcLen == 0);
}


/*
	static
*/
VToUnicodeConverter* XMacIntlMgr::NewToUnicodeConverter(CharSet inCharSet)
{
	XMacToUnicodeConverter* converter = new XMacToUnicodeConverter(inCharSet);
	if (converter != NULL && !converter->IsValid())
	{
		delete converter;
		converter = NULL;
	}
	return converter;
}


/*
	static
*/
VFromUnicodeConverter* XMacIntlMgr::NewFromUnicodeConverter(CharSet inCharSet)
{
	XMacFromUnicodeConverter* converter = new XMacFromUnicodeConverter(inCharSet);
	if (converter != NULL && !converter->IsValid())
	{
		delete converter;
		converter = NULL;
	}
	return converter;
}


void XMacIntlMgr::FormatVDuration( const VDuration& inTime, VString& outText, CFDateFormatterStyle inDateStyle, CFDateFormatterStyle inTimeStyle)
{
	outText.Clear();

	CFDateFormatterRef dateFormatter = ::CFDateFormatterCreate( NULL, GetCFLocaleRef(), inDateStyle, inTimeStyle);
	if (dateFormatter != NULL)
	{
		// Init CFGregorianDate structure from inDate.
		sLONG day, hour, minute, second, millisecond;
		inTime.GetDuration( day, hour, minute, second, millisecond);

		CFGregorianDate date;
		date.year = 0;
		date.month = 0;
		date.day = day;
		date.hour = hour;
		date.minute = minute;
		date.second = second;

		// VTime is in GMT time zone (CFDateFormatterCreateStringWithAbsoluteTime works in local time zone by default)

		CFAbsoluteTime time = CFGregorianDateGetAbsoluteTime( date, NULL);	// use gmt

		CFStringRef p_FormattedDate = ::CFDateFormatterCreateStringWithAbsoluteTime( NULL, dateFormatter, time);
		if (p_FormattedDate)
		{
			outText.MAC_FromCFString(p_FormattedDate);
			::CFRelease(p_FormattedDate);
		}

		::CFRelease( dateFormatter);
	}
}


/*
	static
*/
void XMacIntlMgr::FormatVTime( const VTime& inDate, VString& outText, CFDateFormatterRef inFormatter, bool inUseGMTTimeZoneForDisplay)
{
	outText.Clear();
	
	// Init CFGregorianDate structure from inDate.
	sWORD year, month, day, hour, minute, second, millisecond;
	inDate.GetUTCTime( year, month, day, hour, minute, second, millisecond);

	CFGregorianDate date;
	date.year = year;
	date.month = month;
	date.day = day;
	date.hour = hour;
	date.minute = minute;
	date.second = second;

	// VTime is in GMT time zone (CFDateFormatterCreateStringWithAbsoluteTime works in local time zone by default)

	CFAbsoluteTime time = CFGregorianDateGetAbsoluteTime( date, NULL);	// use gmt

	if (inUseGMTTimeZoneForDisplay)
	{
		CFTimeZoneRef utc_timeZone = CFTimeZoneCreateWithTimeIntervalFromGMT(NULL,0);
		::CFDateFormatterSetProperty( inFormatter, kCFDateFormatterTimeZone, utc_timeZone);
		::CFRelease( utc_timeZone);
	}

	CFStringRef p_FormattedDate = ::CFDateFormatterCreateStringWithAbsoluteTime( NULL, inFormatter, time);
	if (p_FormattedDate)
	{
		outText.MAC_FromCFString(p_FormattedDate);
		::CFRelease(p_FormattedDate);
	}
}


/*
	static
*/
void XMacIntlMgr::FormatVTime( const VTime& inDate, VString& outText, CFDateFormatterStyle inDateStyle, CFDateFormatterStyle inTimeStyle, bool inUseGMTTimeZoneForDisplay)
{
    CFLocaleRef userPrefs=::CFLocaleCopyCurrent();
	CFDateFormatterRef cfFormatter = ::CFDateFormatterCreate( NULL, userPrefs, inDateStyle, inTimeStyle);
	
	if (testAssert( cfFormatter != NULL))
	{
		FormatVTime( inDate, outText, cfFormatter, inUseGMTTimeZoneForDisplay);
		::CFRelease( cfFormatter);
	}
    ::CFRelease(userPrefs);
}


/*
	static
*/
void XMacIntlMgr::FormatVTimeWithTemplate( const VTime& inDate, VString& outText, CFStringRef inTemplate, bool inUseGMTTimeZoneForDisplay)
{
	CFDateFormatterRef cfFormatter = ::CFDateFormatterCreate( NULL, GetCFLocaleRef(), kCFDateFormatterNoStyle, kCFDateFormatterNoStyle);

	CFStringRef cfDateFormat = CFDateFormatterCreateDateFormatFromTemplate( NULL, inTemplate, 0, GetCFLocaleRef());

	if (testAssert( cfFormatter != NULL) && testAssert( cfDateFormat != NULL))
	{
		CFDateFormatterSetFormat( cfFormatter, cfDateFormat);
		FormatVTime( inDate, outText, cfFormatter, inUseGMTTimeZoneForDisplay);
	}

	if (cfFormatter != NULL)
		::CFRelease( cfFormatter);

	if (cfDateFormat != NULL)
		::CFRelease( cfDateFormat);
}


void XMacIntlMgr::FormatDate( const VTime& inDate, VString& outDate, EOSFormats inFormat, bool inUseGMTTimeZoneForDisplay)
{
	// Get the locale and date formatter
	// Select date formatter settings
	switch(inFormat)
	{
		case eOS_SHORT_FORMAT:
			FormatVTime( inDate, outDate, kCFDateFormatterShortStyle, kCFDateFormatterNoStyle, inUseGMTTimeZoneForDisplay);
			break;

		case eOS_MEDIUM_FORMAT:
			FormatVTime( inDate, outDate, kCFDateFormatterMediumStyle, kCFDateFormatterNoStyle, inUseGMTTimeZoneForDisplay);
			break;

		case eOS_LONG_FORMAT:
			FormatVTime( inDate, outDate, kCFDateFormatterFullStyle, kCFDateFormatterNoStyle, inUseGMTTimeZoneForDisplay);
			break;

		default:
			break;
	}
}


void XMacIntlMgr::FormatTime( const VTime& inTime, VString& outTime, EOSFormats inFormat, bool inUseGMTTimeZoneForDisplay)
{
	// Get the time formatter style from inFormat.
	CFDateFormatterStyle timeStyle;
	switch(inFormat)
	{
		case eOS_SHORT_FORMAT:
			timeStyle=kCFDateFormatterShortStyle;
			break;
		case eOS_MEDIUM_FORMAT:
			timeStyle=kCFDateFormatterMediumStyle;
			break;
		case eOS_LONG_FORMAT:
			timeStyle=kCFDateFormatterLongStyle;
			break;
		default:
			xbox_assert( false);
			timeStyle = kCFDateFormatterNoStyle;
			break;
	}

	// Create the time formatter according inFormat.
	FormatVTime( inTime, outTime, kCFDateFormatterNoStyle, timeStyle, inUseGMTTimeZoneForDisplay);
}


void XMacIntlMgr::FormatDuration( const VDuration& inTime, VString& outTime, EOSFormats inFormat)
{
	// Get the time formatter style from inFormat.
	CFDateFormatterStyle timeStyle;
	switch(inFormat)
	{
		case eOS_SHORT_FORMAT:
			timeStyle=kCFDateFormatterShortStyle;
			break;
		case eOS_MEDIUM_FORMAT:
			timeStyle=kCFDateFormatterMediumStyle;
			break;
		case eOS_LONG_FORMAT:
			timeStyle=kCFDateFormatterLongStyle;
			break;
		default:
			xbox_assert( false);
			timeStyle = kCFDateFormatterNoStyle;
			break;
	}

	// Create the time formatter according inFormat.
	FormatVDuration( inTime, outTime, kCFDateFormatterNoStyle, timeStyle);
}


void XMacIntlMgr::GetMonthName( sLONG inIndex, bool inAbbreviated, VString& outName) const
{
	if (LoadCalendarSymbols())
	{
		outName.MAC_FromCFString((CFStringRef)CFArrayGetValueAtIndex( inAbbreviated ? fShortMonthSymbols : fMonthSymbols, inIndex - 1));
	}
}


void XMacIntlMgr::GetWeekDayName( sLONG inIndex, bool inAbbreviated, VString& outName) const
{
	if (LoadCalendarSymbols())
	{
		outName.MAC_FromCFString((CFStringRef)CFArrayGetValueAtIndex( inAbbreviated ? fShortWeekdaySymbols : fWeekdaySymbols, inIndex));
	}
}


bool XMacIntlMgr::LoadCalendarSymbols() const
{
	if ( (fWeekdaySymbols != NULL) && (fMonthSymbols != NULL) && (fShortWeekdaySymbols != NULL) && (fShortMonthSymbols != NULL) )
		return true;

	CFDateFormatterRef cfFormatter = ::CFDateFormatterCreate( NULL, GetCFLocaleRef(), kCFDateFormatterNoStyle, kCFDateFormatterNoStyle);
	if (testAssert( cfFormatter != NULL))
	{
		// Get the list of names
		if (fWeekdaySymbols == NULL)
			fWeekdaySymbols = (CFArrayRef) ::CFDateFormatterCopyProperty( cfFormatter, kCFDateFormatterWeekdaySymbols);
		if (fMonthSymbols == NULL)
			fMonthSymbols = (CFArrayRef) ::CFDateFormatterCopyProperty( cfFormatter, kCFDateFormatterMonthSymbols);
		if (fShortWeekdaySymbols == NULL)
			fShortWeekdaySymbols = (CFArrayRef) ::CFDateFormatterCopyProperty( cfFormatter, kCFDateFormatterShortWeekdaySymbols);
		if (fShortMonthSymbols == NULL)
			fShortMonthSymbols = (CFArrayRef) ::CFDateFormatterCopyProperty( cfFormatter, kCFDateFormatterShortMonthSymbols);
		::CFRelease( cfFormatter);
	}

	return (fWeekdaySymbols != NULL) && (fMonthSymbols != NULL) && (fShortWeekdaySymbols != NULL) && (fShortMonthSymbols != NULL);
}


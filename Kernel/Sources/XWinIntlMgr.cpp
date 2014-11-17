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
#include "VIntlMgr.h"
#include "VArrayValue.h"
#include "XWinIntlMgr.h"
#include "XWinTextConverter.h"

#include <mlang.h>

XWinIntlMgr::XWinIntlMgr( DialectCode inDialect)
: fDialect( inDialect)
{
}


XWinIntlMgr::XWinIntlMgr( const XWinIntlMgr& inOther)
: fDialect( inOther.fDialect)
{
}


XWinIntlMgr::~XWinIntlMgr()
{
}


bool XWinIntlMgr::ToUpperLowerCase(const UniChar *inSrc, sLONG inSrcLen, UniPtr outDst, sLONG& ioDstLen, bool inIsUpper)
{
	xbox_assert(inSrcLen > 0); // doit etre verifie par l'appelant
	
	DWORD flags = LCMAP_LINGUISTIC_CASING + (inIsUpper ? LCMAP_UPPERCASE : LCMAP_LOWERCASE);	// it'd be simpler to use CharUpper, but there's at least a chance that LCMapString will properly handle characters like LATIN SMALL LETTER SHARP S
	
	DWORD count = ::LCMapStringW(LOCALE_USER_DEFAULT /*locale*/, flags, (LPCWSTR) inSrc, inSrcLen, (LPWSTR) outDst, ioDstLen);
	xbox_assert(count > 0);
	ioDstLen = count;
	return count > 0;
}


/*
	static
*/
IMultiLanguage2 *XWinIntlMgr::RetainMultiLanguage()
{
	IMultiLanguage2 *multilanguage = NULL;
	HRESULT hr = ::CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_IMultiLanguage2, (void**) &multilanguage);
	if (hr == CO_E_NOTINITIALIZED)
	{
		hr = ::OleInitialize(NULL);	// J.A. : COINIT_MULTITHREADED doesn't work with OLE objects
		hr = ::CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_IMultiLanguage2, (void**) &multilanguage);
	}
	
	return multilanguage;
}


/*
	static
*/
VToUnicodeConverter* XWinIntlMgr::NewToUnicodeConverter(CharSet inCharSet)
{
	XWinToUnicodeConverter* converter = NULL;
	IMultiLanguage2* multiLanguage = RetainMultiLanguage();
	if (multiLanguage != NULL)
	{
		converter = new XWinToUnicodeConverter( multiLanguage, inCharSet);
		if (converter != NULL && !converter->IsValid())
		{
			delete converter;
			converter = NULL;
		}
		multiLanguage->Release();
	}
	return converter;
}


/*
	static
*/
VFromUnicodeConverter* XWinIntlMgr::NewFromUnicodeConverter(CharSet inCharSet)
{
	XWinFromUnicodeConverter* converter = NULL;
	IMultiLanguage2* multiLanguage = RetainMultiLanguage();
	if (multiLanguage != NULL)
	{
		converter = new XWinFromUnicodeConverter( multiLanguage, inCharSet);
		if (converter != NULL && !converter->IsValid())
		{
			delete converter;
			converter = NULL;
		}
		multiLanguage->Release();
	}
	return converter;
}


bool XWinIntlMgr::GetLocaleInfo( LCTYPE inType, VString& outInfo) const
{
	UniChar info[256];
	int r = ::GetLocaleInfoW( fDialect, inType, info, sizeof( info) / sizeof( UniChar));
	if (testAssert( r != 0))
		outInfo = info;
	else
		outInfo.Clear();
	return r != 0;
}


void XWinIntlMgr::FormatDate( const VTime& inDate, VString& outDate, EOSFormats inFormat, bool inUseGMTTimeZoneForDisplay)
{
	// Prepare SYSTEMTIME for windows.
	sWORD YY = 0;
	sWORD MM = 0;
	sWORD DD = 0;
	sWORD hh = 0;
	sWORD mm = 0;
	sWORD ss = 0;
	sWORD ms = 0;
	if (inUseGMTTimeZoneForDisplay)
		inDate.GetUTCTime (YY,MM,DD,hh,mm,ss,ms);
	else
		inDate.GetLocalTime (YY,MM,DD,hh,mm,ss,ms);

	// Verify if date >1st Jan 1601 (GetDateFormat doesn't support earlier dates.
	if (YY>=1601)
	{
		// Let the OS do it's job.
		UniChar acBuffer[256];

		SYSTEMTIME osDate={0};
		osDate.wYear=YY;
		osDate.wMonth=MM;
		osDate.wDay=DD;
		osDate.wHour=hh;
		osDate.wMinute=mm;
		osDate.wSecond=ss;
		osDate.wMilliseconds=ms;

		if (inFormat == eOS_MEDIUM_FORMAT)
		{
			VString pattern;
			if (GetLocaleInfo( LOCALE_SLONGDATE, pattern))
			{
				// replace long month and date by medium ones
				pattern.ExchangeRawString( CVSTR( "MMMM"), CVSTR( "MMM"));
				pattern.ExchangeRawString( CVSTR( "dddd"), CVSTR( "ddd"));
				if (::GetDateFormatW( fDialect, 0, &osDate, pattern.GetCPointer(), acBuffer, sizeof(acBuffer)))
					outDate = acBuffer;
			}
		}
		else
		{
			// Let the OS do the stuff.
			DWORD dateFormat = (inFormat == eOS_SHORT_FORMAT) ? DATE_SHORTDATE : DATE_LONGDATE;
			if (::GetDateFormatW(fDialect,dateFormat,&osDate,NULL,acBuffer,sizeof(acBuffer)))
				outDate = acBuffer;
		}
	}
	else
	{
		// Get the date pattern
		VString pattern;
		if (GetLocaleInfo( (inFormat == eOS_LONG_FORMAT) ? LOCALE_SLONGDATE : LOCALE_SSHORTDATE, pattern))
		{
			XBOX::VString tokens="gyMd";
			UniChar oldToken=0;
			sLONG count=0;
			pattern.AppendUniChar(' ');
			sLONG YY2=YY%100;
			XBOX::VString oneName;
			for (int pos=0;pos<pattern.GetLength();pos++)
			{
				UniChar token=pattern[pos];
				if (tokens.FindUniChar(token)>=1)
				{
					if (token==oldToken)
						count++;
					else
					{
						if (!count)
						{
							count=1;
							oldToken=token;
						}
					}
				}

				if (count && token!=oldToken)
				{
					switch(oldToken)
					{
					case 'g':
						if (count==2)
						{
							// TODO: ERA will be added if really wanted.
						}
						else
						{
							for (int i=0;i<count;i++)
								outDate.AppendUniChar(oldToken);
						}
						break;

					case 'y':	// YEAR
						switch(count)
						{
						case 5:
						case 4:		// 4 or more digits date
							outDate.AppendLong(YY);
							break;

						case 2:		// 2 digits with starting 0.
							if (YY2<=9)
								outDate.AppendLong(0);
						case 1:		// 1 or 2 digits
							outDate.AppendLong(YY2);
							break;

						default:
							for (int i=0;i<count;i++)
								outDate.AppendUniChar(oldToken);
							break;
						}
						break;

					case 'M':	// MONTH
						switch(count)
						{
						case 4:	// Long name
						case 3:	// Abbreviated name
							if (GetLocaleInfo( ((count==4) ? LOCALE_SMONTHNAME1 : LOCALE_SABBREVMONTHNAME1) + MM - 1, oneName))
								outDate += oneName;
							break;

						case 2:	// 2 digits number (leading 0)
							if (MM<=9)
								outDate.AppendLong(0);
						case 1:	// 1 or 2 digits number
							outDate.AppendLong(MM);
							break;

						default:
							for (int i=0;i<count;i++)
								outDate.AppendUniChar(oldToken);
							break;
						}
						break;

					case 'd':	// DAY
						switch(count)
						{
						case 4:	// Weekday long name
						case 3:	// Weekday abbreviated name
							if (GetLocaleInfo( ((count==4) ? LOCALE_SDAYNAME1 : LOCALE_SABBREVDAYNAME1) + (inDate.GetWeekDay() + 6) % 7, oneName))
								outDate += oneName;
							break;

						case 2:	// Month day on 2 digits (leading 0)
							if (DD<=9)
								outDate.AppendLong(0);
						case 1:	// Month day on 1 or 2 digits.
							outDate.AppendLong(DD);
							break;

						default:
							for (int i=0;i<count;i++)
								outDate.AppendUniChar(oldToken);
							break;
						}
						break;

					}
					count=0;
				}

				if (!count)
					outDate.AppendUniChar(token);
				oldToken=token;
			}

			// Remove the extra space at end of pattern.
			if (outDate.GetLength()>1)
				outDate.SubString(1,outDate.GetLength()-1);
		}
	}
}


void XWinIntlMgr::FormatTime( const VTime& inTime, VString& outTime, EOSFormats inFormat, bool inUseGMTTimeZoneForDisplay)
{
	// 1:system short time; 2:system medium time; 3:system long time

	DWORD timeFormat=0;
	switch(inFormat)
	{
		case eOS_SHORT_FORMAT:// No signs
		case eOS_MEDIUM_FORMAT:// No signs
			timeFormat = TIME_NOTIMEMARKER | TIME_NOSECONDS;
			break;
		case eOS_LONG_FORMAT://all
			break;
		default:
			break;
	};

	// Prepare SYSTEMTIME for windows.
	sWORD YY=0,MM=0,DD=0,hh=0,mm=0,ss=0,ms=0;
	SYSTEMTIME osTime={0};
	if (inUseGMTTimeZoneForDisplay)
		inTime.GetUTCTime (YY,MM,DD,hh,mm,ss,ms);
	else
		inTime.GetLocalTime (YY,MM,DD,hh,mm,ss,ms);
	osTime.wYear=YY;
	osTime.wMonth=MM;
	osTime.wDay=DD;
	osTime.wHour=hh;
	osTime.wMinute=mm;
	osTime.wSecond=ss;
	osTime.wMilliseconds=ms;

	// Let the OS do the stuff.
	UniChar acBuffer[256];
	if (::GetTimeFormatW( fDialect,timeFormat,&osTime,NULL,acBuffer,sizeof(acBuffer)))
		outTime=acBuffer;
	else
		outTime.Clear();
}


void XWinIntlMgr::FormatDuration( const VDuration& inTime, VString& outTime, EOSFormats inFormat)
{
	// 1:system short time; 2:system medium time; 3:system long time

	DWORD timeFormat=0;
	switch(inFormat)
	{
	case eOS_SHORT_FORMAT:// No signs
	case eOS_MEDIUM_FORMAT:// No signs
		timeFormat=TIME_NOTIMEMARKER;
		break;
	case eOS_LONG_FORMAT://all
		break;
	default:
		break;
	};

	// Prepare SYSTEMTIME for windows.
	sLONG DD=0,hh=0,mm=0,ss=0,ms=0;
	SYSTEMTIME osTime={0};
	inTime.GetDuration (DD,hh,mm,ss,ms);
	osTime.wYear=0;
	osTime.wMonth=0;
	osTime.wDay=DD;
	osTime.wHour=hh;
	osTime.wMinute=mm;
	osTime.wSecond=ss;
	osTime.wMilliseconds=ms;

	// Let the OS do the stuff.
	UniChar acBuffer[256];
	if (::GetTimeFormatW( fDialect,timeFormat,&osTime,NULL,acBuffer,sizeof(acBuffer)))
		outTime=acBuffer;
	else
		outTime.Clear();
}


void XWinIntlMgr::GetMonthName( sLONG inIndex, bool inAbbreviated, VString& outName) const
{
	xbox_assert( inIndex >= 1 && inIndex <= 12);
	static LCTYPE sTypes[] = {
		LOCALE_SMONTHNAME1,
		LOCALE_SABBREVMONTHNAME1,
		LOCALE_SMONTHNAME2,
		LOCALE_SABBREVMONTHNAME2,
		LOCALE_SMONTHNAME3,
		LOCALE_SABBREVMONTHNAME3,
		LOCALE_SMONTHNAME4,
		LOCALE_SABBREVMONTHNAME4,
		LOCALE_SMONTHNAME5,
		LOCALE_SABBREVMONTHNAME5,
		LOCALE_SMONTHNAME6,
		LOCALE_SABBREVMONTHNAME6,
		LOCALE_SMONTHNAME7,
		LOCALE_SABBREVMONTHNAME7,
		LOCALE_SMONTHNAME8,
		LOCALE_SABBREVMONTHNAME8,
		LOCALE_SMONTHNAME9,
		LOCALE_SABBREVMONTHNAME9,
		LOCALE_SMONTHNAME10,
		LOCALE_SABBREVMONTHNAME10,
		LOCALE_SMONTHNAME11,
		LOCALE_SABBREVMONTHNAME11,
		LOCALE_SMONTHNAME12,
		LOCALE_SABBREVMONTHNAME12,
		LOCALE_SMONTHNAME13,
		LOCALE_SABBREVMONTHNAME13,
		0,
		0
	};

	GetLocaleInfo( sTypes[inIndex + (inAbbreviated ? 0 : -1)], outName);
}


void XWinIntlMgr::GetWeekDayName( sLONG inIndex, bool inAbbreviated, VString& outName) const
{
	// Sunday as day 0
	xbox_assert( inIndex >= 0 && inIndex <= 6);
	static LCTYPE sTypes[] = {
		LOCALE_SDAYNAME7,
		LOCALE_SABBREVDAYNAME7,
		LOCALE_SDAYNAME1,
		LOCALE_SABBREVDAYNAME1,
		LOCALE_SDAYNAME2,
		LOCALE_SABBREVDAYNAME2,
		LOCALE_SDAYNAME3,
		LOCALE_SABBREVDAYNAME3,
		LOCALE_SDAYNAME4,
		LOCALE_SABBREVDAYNAME4,
		LOCALE_SDAYNAME5,
		LOCALE_SABBREVDAYNAME5,
		LOCALE_SDAYNAME6,
		LOCALE_SABBREVDAYNAME6,
		0,
		0
	};
	
	GetLocaleInfo( sTypes[inIndex + (inAbbreviated ? 1 : 0)], outName);
}


void XWinIntlMgr::GetDayNames( VectorOfVString& outNames, bool inAbbreviated) const
{
	// Sunday as day 1
	static LCTYPE sTypes[] = {
		LOCALE_SDAYNAME7,
		LOCALE_SABBREVDAYNAME7,
		LOCALE_SDAYNAME1,
		LOCALE_SABBREVDAYNAME1,
		LOCALE_SDAYNAME2,
		LOCALE_SABBREVDAYNAME2,
		LOCALE_SDAYNAME3,
		LOCALE_SABBREVDAYNAME3,
		LOCALE_SDAYNAME4,
		LOCALE_SABBREVDAYNAME4,
		LOCALE_SDAYNAME5,
		LOCALE_SABBREVDAYNAME5,
		LOCALE_SDAYNAME6,
		LOCALE_SABBREVDAYNAME6,
		0,
		0
	};
	
	for( LCTYPE *pType = inAbbreviated ? &sTypes[1] : &sTypes[0] ; *pType ; pType += 2)
	{
		VString name;
		if (GetLocaleInfo( *pType, name))
			outNames.push_back( name);
	}
}


void XWinIntlMgr::GetMonthNames( VectorOfVString& outNames, bool inAbbreviated) const
{
	static LCTYPE sTypes[] = {
		LOCALE_SMONTHNAME1,
		LOCALE_SABBREVMONTHNAME1,
		LOCALE_SMONTHNAME2,
		LOCALE_SABBREVMONTHNAME2,
		LOCALE_SMONTHNAME3,
		LOCALE_SABBREVMONTHNAME3,
		LOCALE_SMONTHNAME4,
		LOCALE_SABBREVMONTHNAME4,
		LOCALE_SMONTHNAME5,
		LOCALE_SABBREVMONTHNAME5,
		LOCALE_SMONTHNAME6,
		LOCALE_SABBREVMONTHNAME6,
		LOCALE_SMONTHNAME7,
		LOCALE_SABBREVMONTHNAME7,
		LOCALE_SMONTHNAME8,
		LOCALE_SABBREVMONTHNAME8,
		LOCALE_SMONTHNAME9,
		LOCALE_SABBREVMONTHNAME9,
		LOCALE_SMONTHNAME10,
		LOCALE_SABBREVMONTHNAME10,
		LOCALE_SMONTHNAME11,
		LOCALE_SABBREVMONTHNAME11,
		LOCALE_SMONTHNAME12,
		LOCALE_SABBREVMONTHNAME12,
		LOCALE_SMONTHNAME13,
		LOCALE_SABBREVMONTHNAME13,
		0,
		0
	};

	for( LCTYPE *pType = inAbbreviated ? &sTypes[1] : &sTypes[0] ; *pType ; pType += 2)
	{
		VString name;
		if (GetLocaleInfo( *pType, name))
			outNames.push_back( name);
	}
}
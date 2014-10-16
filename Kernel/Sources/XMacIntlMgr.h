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
#ifndef __XMacIntlMgr__
#define __XMacIntlMgr__


BEGIN_TOOLBOX_NAMESPACE

// Needed declarations
class VToUnicodeConverter;
class VFromUnicodeConverter;

class XMacIntlMgr
{
public:
			XMacIntlMgr( DialectCode inDialect);
			XMacIntlMgr( const XMacIntlMgr& inOther);
	virtual	~XMacIntlMgr();
	
			bool					ToUpperLowerCase (const UniChar* inSrc, sLONG inSrcLen, UniPtr outDst, sLONG& ioDstLen, bool inIsUpper);

	static	VToUnicodeConverter*	NewToUnicodeConverter (CharSet inCharSet);
	static	VFromUnicodeConverter*	NewFromUnicodeConverter (CharSet inCharSet);

			void					FormatDate( const VTime& inDate, VString& outDate, EOSFormats inFormat, bool inUseGMTTimeZoneForDisplay);
			void					FormatTime( const VTime& inTime, VString& outTime, EOSFormats inFormat, bool inUseGMTTimeZoneForDisplay);
			void					FormatDuration( const VDuration& inTime, VString& outTime, EOSFormats inFormat);

			void					GetMonthName( sLONG inIndex, bool inAbbreviated, VString& outName) const;	// 1 (january) to 12 (december)
			void					GetWeekDayName( sLONG inIndex, bool inAbbreviated, VString& outName) const;	// 0 (sunday) to 6 (saturday)
	
			CFLocaleRef				GetCFLocaleRef() const;
			CFLocaleRef				RetainCFLocaleRef() const;
	static	CFLocaleRef				CreateCFLocaleRef( DialectCode inDialect);

protected:
	static	void					FormatVTime( const VTime& inDate, VString& outText, CFDateFormatterRef inFormatter, bool inUseGMTTimeZoneForDisplay);
			void					FormatVTime( const VTime& inDate, VString& outText, CFDateFormatterStyle inDateStyle, CFDateFormatterStyle inTimeStyle, bool inUseGMTTimeZoneForDisplay);
			void					FormatVTimeWithTemplate( const VTime& inDate, VString& outText, CFStringRef inTemplate, bool inUseGMTTimeZoneForDisplay);
			void					FormatVDuration( const VDuration& inTime, VString& outText, CFDateFormatterStyle inDateStyle, CFDateFormatterStyle inTimeStyle);

			bool					LoadCalendarSymbols() const;

			DialectCode				fDialect;
	mutable	CFLocaleRef				fCFLocaleRef;
	mutable	CFArrayRef				fWeekdaySymbols;
	mutable	CFArrayRef				fMonthSymbols;
	mutable	CFArrayRef				fShortWeekdaySymbols;
	mutable	CFArrayRef				fShortMonthSymbols;
};

typedef XMacIntlMgr XIntlMgrImpl;

END_TOOLBOX_NAMESPACE

#endif

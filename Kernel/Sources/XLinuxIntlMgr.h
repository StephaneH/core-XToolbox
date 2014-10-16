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
#ifndef __XLinuxIntlMgr__
#define __XLinuxIntlMgr__



namespace xbox_icu
{
	class Locale;
	class Transliterator;
	class BreakIterator;
	class GregorianCalendar;
}


BEGIN_TOOLBOX_NAMESPACE


class VToUnicodeConverter;
class VFromUnicodeConverter;




class XLinuxIntlMgr
{
public:
	XLinuxIntlMgr( DialectCode inDialect);
	XLinuxIntlMgr( const XLinuxIntlMgr& inOther);
	virtual ~XLinuxIntlMgr();

	//Pourquoi deux types bool ?

	// Implementation available in VIntlMgr::ToUpperLowerCase if compiled with USE_ICU flag
	//Boolean		ToUpperLowerCase(const UniChar* inSrc, sLONG inSrcLen, UniPtr outDst, sLONG& ioDstLen, Boolean inIsUpper);

	// Implementation available in VICUCollator::CompareString if compiled with USE_ICU flag
	//CompareResult CompareString(const UniChar* inText1, sLONG inSize1, const UniChar* inText2, sLONG inSize2, Boolean inWithDiacritics, Boolean inForLookup);

	// Implementation available in VICUCollator::EqualString if compiled with USE_ICU flag
	//bool			EqualString(const UniChar* inText1, sLONG inSize1, const UniChar* inText2, sLONG inSize2, Boolean inWithDiacritics);


	static	VToUnicodeConverter*	NewToUnicodeConverter(CharSet inCharSet);
	static	VFromUnicodeConverter*	NewFromUnicodeConverter(CharSet inCharSet);

	//Boolean	LINUX_ConvertUniToLinux(const UniChar* inSrc, sLONG inSrcSize, char* outDst, sLONG& ioDstSize);
	//Boolean	LINUX_ConvertLinuxToUni(const char* inSrc, sLONG inSrcSize, UniPtr outDst, sLONG& ioDstSize);


	//Specific to Linux implementation (built on top of ICU)

	void		Init(const xbox_icu::Locale& inLocale);

	VString		GetDecimalSeparator()	const;
	VString		GetThousandSeparator()	const;
	VString		GetCurrency()			const;

	//void	GetMonthDayNames(EMonthDayList inListType, VArrayString* outArray); const

	VString		GetShortDatePattern()	const;
	VString		GetLongDatePattern()	const;

	VString		GetShortTimePattern()	const;
	VString		GetMediumTimePattern()	const;
	VString		GetLongTimePattern()	const;

	VString		GetAMString()			const;
	VString		GetPMString()			const;

	VString		GetDateSeparator()		const;
	VString		GetTimeSeparator()		const;
	DateOrder	GetDateOrder()			const;
	bool		UseAmPm()				const;

	void		FormatDate(const VTime& inDate, VString& outDate, EOSFormats inFormat, bool inUseGMTTimeZoneForDisplay) const;
	void		FormatTime(const VTime& inTime, VString& outTime, EOSFormats inFormat, bool inUseGMTTimeZoneForDisplay) const;
	void		FormatNumber(const VValueSingle& inValue, VString& outNumber) const;
	void		FormatDuration(const VDuration& inDuration, VString& outDuration, EOSFormats inFormat) const;

	void		GetMonthName( sLONG inIndex, bool inAbbreviated, VString& outName) const;	// 1 (january) to 12 (december)
	void		GetWeekDayName( sLONG inIndex, bool inAbbreviated, VString& outName) const;	// 0 (sunday) to 6 (saturday)

private:
	xbox_icu::Locale* fLocale;

}; // class XLinuxIntlMgr


typedef XLinuxIntlMgr XIntlMgrImpl;



END_TOOLBOX_NAMESPACE

#endif

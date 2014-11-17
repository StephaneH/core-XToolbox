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
#include "XMacIntlMgr.h"
#include "XMacStringCompare.h"

/***** system compare string ****/


XMacStringCompare::XMacStringCompare( DialectCode inDialect, VIntlMgr *inIntlMgr)
{
	if (inIntlMgr != NULL)
	{
		fCFLocale = inIntlMgr->MAC_RetainCFLocale();
	}
	else
	{
		fCFLocale = XMacIntlMgr::CreateCFLocaleRef( inDialect);
	}
}


XMacStringCompare::XMacStringCompare( const XMacStringCompare& inOther)
{
	fCFLocale = CFLocaleCreateCopy( kCFAllocatorDefault, inOther.GetCFLocale());
}


XMacStringCompare::~XMacStringCompare()
{
	if (fCFLocale != NULL)
		CFRelease( fCFLocale);
}


CFComparisonResult XMacStringCompare::_CompareCFString( const UniChar *inText1, sLONG inSize1, const UniChar *inText2, sLONG inSize2, bool inWithDiacritics)
{
	CFStringRef	string1 = ::CFStringCreateWithCharactersNoCopy( kCFAllocatorDefault, inText1, inSize1, kCFAllocatorNull);
	CFStringRef	string2 = ::CFStringCreateWithCharactersNoCopy( kCFAllocatorDefault, inText2, inSize2, kCFAllocatorNull);

	CFComparisonResult result = kCFCompareEqualTo;
	if ( (string1 != NULL) && (string2 != NULL) )
	{
		CFStringCompareFlags flags = kCFCompareWidthInsensitive | kCFCompareLocalized;
		if (!inWithDiacritics)
			flags |= (kCFCompareDiacriticInsensitive | kCFCompareCaseInsensitive);

		result = CFStringCompareWithOptionsAndLocale( string1, string2, CFRangeMake( 0, CFStringGetLength( string1)), flags, GetCFLocale());
	}
	
	if (string1 != NULL)
		CFRelease( string1);

	if (string2 != NULL)
		CFRelease( string2);

	return result;
}


bool XMacStringCompare::EqualString( const UniChar *inText1, sLONG inSize1, const UniChar *inText2, sLONG inSize2, bool inWithDiacritics)
{
	return _CompareCFString( inText1, inSize1, inText2, inSize2, inWithDiacritics) == kCFCompareEqualTo;
}


CompareResult XMacStringCompare::CompareString (const UniChar *inText1, sLONG inSize1, const UniChar *inText2, sLONG inSize2, bool inWithDiacritics)
{
	CFComparisonResult result = _CompareCFString( inText1, inSize1, inText2, inSize2, inWithDiacritics);

	CompareResult r;
	if (result == kCFCompareLessThan)
		r = CR_SMALLER;
	else if (result == kCFCompareGreaterThan)
		r = CR_BIGGER;
	else
		r = CR_EQUAL;

	return r;
}


sLONG XMacStringCompare::FindString( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics, sLONG *outFoundLength)
{
	sLONG pos = 0;
	sLONG foundLength = 0;
	
	CFStringRef	text = ::CFStringCreateWithCharactersNoCopy( kCFAllocatorDefault, inText, inTextSize, kCFAllocatorNull);
	CFStringRef	pattern = ::CFStringCreateWithCharactersNoCopy( kCFAllocatorDefault, inPattern, inPatternSize, kCFAllocatorNull);

	if ( (text != NULL) && (pattern != NULL) )
	{
		CFStringCompareFlags flags = kCFCompareWidthInsensitive | kCFCompareLocalized;
		if (!inWithDiacritics)
			flags |= (kCFCompareDiacriticInsensitive | kCFCompareCaseInsensitive);

		CFRange result;
		Boolean found = CFStringFindWithOptionsAndLocale( text, pattern, CFRangeMake( 0, CFStringGetLength( text)), flags, GetCFLocale(), &result);
		if (found)
		{
			pos = (sLONG) (result.location + 1);
			foundLength = (sLONG) (result.length);
		}
	}

	if (text != NULL)
		CFRelease( text);

	if (pattern != NULL)
		CFRelease( pattern);
	
	if (outFoundLength != NULL)
		*outFoundLength = foundLength;
	
	return pos;
}


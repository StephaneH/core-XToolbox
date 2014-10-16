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
#include "XWinStringCompare.h"

/***** system compare string ****/

XWinStringCompare::XWinStringCompare( DialectCode inDialect, VIntlMgr *inIntlMgr)
: fLocale( MAKELCID(inDialect,SORT_DEFAULT))
{
}


XWinStringCompare::XWinStringCompare( const XWinStringCompare& inOther)
: fLocale( inOther.fLocale)
{
}


XWinStringCompare::~XWinStringCompare()
{

}


CompareResult XWinStringCompare::CompareString (const UniChar *inText1, sLONG inSize1, const UniChar *inText2, sLONG inSize2, bool inWithDiacritics)
{
	DWORD flags = inWithDiacritics ? 0 : (NORM_IGNORENONSPACE | NORM_IGNORECASE);
	DWORD result = ::CompareStringW(fLocale, flags, (LPCWSTR) inText1, inSize1, (LPCWSTR) inText2, inSize2);
	xbox_assert(result != 0);
	CompareResult r = CR_BIGGER;
	if (result == CSTR_LESS_THAN)
		r = CR_SMALLER;
	else if (result == CSTR_EQUAL)
		r = CR_EQUAL;
	return r;
}


bool XWinStringCompare::EqualString( const UniChar *inText1, sLONG inSize1, const UniChar *inText2, sLONG inSize2, bool inWithDiacritics)
{
	DWORD flags = inWithDiacritics ? 0 : (NORM_IGNORENONSPACE | NORM_IGNORECASE);
	DWORD result = ::CompareStringW(fLocale, flags, (LPCWSTR) inText1, inSize1, (LPCWSTR) inText2, inSize2);
	xbox_assert(result != 0);
	return result == CSTR_EQUAL;
}


sLONG XWinStringCompare::FindString( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics, sLONG *outFoundLength)
{
// FindNLSString is not available on XP
typedef	int (WINAPI *FindNLSStringProc)( LCID Locale, DWORD dwFindNLSStringFlags, LPCWSTR lpStringSource, int cchSource, LPCWSTR lpStringValue, int cchValue, LPINT pcchFound);
static FindNLSStringProc FindNLSStringPtr = (FindNLSStringProc) ::GetProcAddress(GetModuleHandle("Kernel32.dll"),"FindNLSString");

	// FindNLSString does not accept null strings
	xbox_assert( inTextSize != 0);
	xbox_assert( inPatternSize != 0);

	sLONG pos;
	if (FindNLSStringPtr == NULL)
	{
		// brute force and false implementation
		pos = 0;
		const UniChar* ptrLast = inText + inTextSize - inPatternSize;
		for( const UniChar* ptr = inText ; ptr <= ptrLast ; ++ptr)
		{
			if (EqualString( inPattern, inPatternSize, ptr, inPatternSize, inWithDiacritics))
			{
				pos = (ptr - inText) + 1;
				break;
			}
		}
		if (outFoundLength != NULL)
			*outFoundLength = (pos > 0) ? inPatternSize : 0;
	}
	else
	{
		DWORD flags = inWithDiacritics ? 0 : (NORM_IGNORENONSPACE | NORM_IGNORECASE);
		flags |= FIND_FROMSTART;
		
		int foundLength;
		int r = FindNLSStringPtr( fLocale, flags, inText, inTextSize, inPattern, inPatternSize, &foundLength);
		
		if (r >= 0)
		{
			// found
			pos = r + 1;	// result from FindNLSString is zero-based
			if (outFoundLength != NULL)
				*outFoundLength = foundLength;
		}
		else
		{
			xbox_assert( GetLastError() == ERROR_SUCCESS);	// means no error but not found
			pos = 0;
			if (outFoundLength != NULL)
				*outFoundLength = foundLength;
		}
	}
	return pos;
}


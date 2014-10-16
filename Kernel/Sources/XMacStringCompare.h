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
#ifndef __XWinStringCompare__
#define __XWinStringCompare__

#include "Kernel/Sources/VKernelTypes.h"

BEGIN_TOOLBOX_NAMESPACE

class VIntlMgr;

class XMacStringCompare
{
public:
			XMacStringCompare( DialectCode inDialect, VIntlMgr *inIntlMgr);
			XMacStringCompare( const XMacStringCompare& inOther);
	virtual	~XMacStringCompare();
	
	CompareResult	CompareString( const UniChar *inText1, sLONG inSize1, const UniChar *inText2, sLONG inSize2, bool inWithDiacritics);
	bool			EqualString( const UniChar *inText1, sLONG inSize1, const UniChar *inText2, sLONG inSize2, bool inWithDiacritics);
	sLONG			FindString( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics, sLONG *outFoundLength);

	CFLocaleRef		GetCFLocale() const	{ return fCFLocale;}

private:
	CFComparisonResult	_CompareCFString( const UniChar *inText1, sLONG inSize1, const UniChar *inText2, sLONG inSize2, bool inWithDiacritics);
	CFLocaleRef			fCFLocale;
};
typedef XMacStringCompare XStringCompareImpl;


END_TOOLBOX_NAMESPACE

#endif

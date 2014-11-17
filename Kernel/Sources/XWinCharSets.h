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
#ifndef __XWinCharSets__
#define __XWinCharSets__

#include "Kernel/Sources/VKernelTypes.h"
#include <mlang.h>

BEGIN_TOOLBOX_NAMESPACE

typedef struct WIN_CharSetMap
{
	CharSet			fCharSet;
	UINT			fCodePage;	
} WIN_CharSetMap;


// http://msdn.microsoft.com/en-us/library/ms776446.aspx
static WIN_CharSetMap	sWIN_CharSetMap[] = 
{
	VTC_UTF_16_SMALLENDIAN,	1200,
	VTC_UTF_16_BIGENDIAN,	1201,
	VTC_UTF_7,				65000,
	VTC_UTF_8,				65001,
	VTC_US_ASCII,			20127,
	VTC_US_EBCDIC,			37,
	VTC_MAC_ROMAN,			10000,
	VTC_WIN_ROMAN,			1252,
	VTC_MAC_CENTRALEUROPE,	10029,
	VTC_WIN_CENTRALEUROPE,	1250,
	VTC_MAC_CYRILLIC,		10007,
	VTC_WIN_CYRILLIC,		1251,
	VTC_MAC_GREEK,			10006,
	VTC_WIN_GREEK,			1253,
	VTC_MAC_TURKISH,		10081,
	VTC_WIN_TURKISH,		1254,
	VTC_MAC_ARABIC,			10004,
	VTC_WIN_ARABIC,			1256,
	VTC_MAC_HEBREW,			10005,
	VTC_WIN_HEBREW,			1255,
	VTC_MAC_BALTIC,			10029,	// eq. a X-MAC-CENTREURO
	VTC_WIN_BALTIC,			1257,
	VTC_MAC_JAPANESE,		10001,
	VTC_SHIFT_JIS,			932,	// code page 932 is actually Windows-31J (there's no code page for the iana SHIFT-JIS)
	VTC_Windows31J,			932,
	VTC_JIS,				50220,
	VTC_BIG5,				950,
	VTC_EUC_KR,				51949,
	VTC_KOI8R,				20866,
	VTC_IBM437,				437,
	
	VTC_ISO_8859_1,			28591,
	VTC_ISO_8859_2,			28592,	// 1250
	VTC_ISO_8859_3,			28593,
	VTC_ISO_8859_4,			28594,	
	VTC_ISO_8859_5,			28595,
	VTC_ISO_8859_6,			28596,
	VTC_ISO_8859_7,			28597,
	VTC_ISO_8859_8,			28598,
	VTC_ISO_8859_9,			28599,
	VTC_ISO_8859_10,		0,
	VTC_ISO_8859_13,		28603,
	VTC_ISO_8859_15,		28605,
	VTC_GB2312,				936,
	VTC_GB2312_80,			20936,
	VTC_MAC_CHINESE_SIMP,	10008,
	VTC_WIN_CHINESE_SIMP,	936,
	VTC_MAC_CHINESE_TRAD,	10002,
	VTC_WIN_CHINESE_TRAD,	950,
	VTC_UNKNOWN,			0
};

END_TOOLBOX_NAMESPACE

#endif
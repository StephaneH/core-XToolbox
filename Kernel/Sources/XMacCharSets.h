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
#ifndef __XMacCharSets__
#define __XMacCharSets__

#include "Kernel/Sources/VKernelTypes.h"

BEGIN_TOOLBOX_NAMESPACE

typedef struct MAC_CharSetMap
{
	CharSet				fCode;
	TextEncodingBase	fEncodingBase;	
	TextEncodingVariant	fEncodingVariant;
	TextEncodingFormat	fEncodingFormat;
} MAC_CharSetMap;


static MAC_CharSetMap	sMAC_CharSetMap[] = 
{
	VTC_US_ASCII,			kTextEncodingUS_ASCII,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_US_EBCDIC,			kTextEncodingEBCDIC_CP037,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_MAC_ROMAN,			kTextEncodingMacRoman,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_WIN_ROMAN,			kTextEncodingWindowsLatin1,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_MAC_CENTRALEUROPE,	kTextEncodingMacCentralEurRoman,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_WIN_CENTRALEUROPE,	kTextEncodingWindowsLatin2,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_MAC_CYRILLIC,		kTextEncodingMacCyrillic,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_WIN_CYRILLIC,		kTextEncodingWindowsCyrillic,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_MAC_GREEK,			kTextEncodingMacGreek,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_WIN_GREEK,			kTextEncodingWindowsGreek,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_MAC_TURKISH,		kTextEncodingMacTurkish,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_WIN_TURKISH,		kTextEncodingWindowsLatin5,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_MAC_ARABIC,			kTextEncodingMacArabic,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_WIN_ARABIC,			kTextEncodingWindowsArabic,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_MAC_HEBREW,			kTextEncodingMacHebrew,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_WIN_HEBREW,			kTextEncodingWindowsHebrew,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_MAC_BALTIC,			kTextEncodingMacEastEurRoman,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat, // eq. a kTextEncodingMacCentralEurRoman
	VTC_WIN_BALTIC,			kTextEncodingWindowsBalticRim,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_SHIFT_JIS,			kTextEncodingShiftJIS,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_Windows31J,			kTextEncodingDOSJapanese,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_JIS,				kTextEncodingISO_2022_JP,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_MAC_JAPANESE,		kTextEncodingMacJapanese,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_BIG5,				kTextEncodingBig5,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_EUC_KR,				kTextEncodingEUC_KR,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_KOI8R,				kTextEncodingKOI8_R,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_ISO_8859_1,			kTextEncodingISOLatin1,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_ISO_8859_2,			kTextEncodingISOLatin2,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_ISO_8859_3,			kTextEncodingISOLatin3,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_ISO_8859_4,			kTextEncodingISOLatin4,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_ISO_8859_5,			kTextEncodingISOLatinCyrillic,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_ISO_8859_6,			kTextEncodingISOLatinArabic,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_ISO_8859_7,			kTextEncodingISOLatinGreek,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_ISO_8859_8,			kTextEncodingISOLatinHebrew,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_ISO_8859_9,			kTextEncodingISOLatin5,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_ISO_8859_10,		kTextEncodingISOLatin6,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_ISO_8859_13,		kTextEncodingISOLatin7,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_ISO_8859_15,		kTextEncodingISOLatin9,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_GB2312,				kTextEncodingDOSChineseSimplif,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat, // pp kTextEncodingGB_2312_80 ne fonctionne pas, on utilise le GBK a la place
	VTC_GB2312_80,			kTextEncodingGB_2312_80,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_MAC_CHINESE_SIMP,	kTextEncodingMacChineseSimp,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_WIN_CHINESE_SIMP,	kTextEncodingDOSChineseSimplif,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_MAC_CHINESE_TRAD,	kTextEncodingMacChineseTrad,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_WIN_CHINESE_TRAD,	kTextEncodingDOSChineseTrad,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,
	VTC_IBM437,				kTextEncodingDOSLatinUS,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat,

	VTC_UTF_32,				kTextEncodingUnicodeDefault,	kTextEncodingDefaultVariant,	kUnicode32BitFormat,

	VTC_UNKNOWN,			kTextEncodingUnknown,	kTextEncodingDefaultVariant,	kTextEncodingDefaultFormat
};


typedef struct MacToWinCharSetMap
{
	TextEncodingBase	fMacBase;
	TextEncodingBase	fWinBase;
} MacToWinCharSetMap;


static MacToWinCharSetMap	sMacToWinCharSetMap[] = 
{
	kTextEncodingMacThai,		kTextEncodingDOSThai, /* code page 874, also for Windows*/
	kTextEncodingMacJapanese,	kTextEncodingDOSJapanese, /* code page 932, also for Windows; Shift-JIS with additions*/
	kTextEncodingMacChineseSimp,	kTextEncodingDOSChineseSimplif, /* code page 936, also for Windows; was EUC-CN, now GBK (EUC-CN extended)*/
	kTextEncodingMacKorean,		kTextEncodingWindowsKoreanJohab,	/* code page 1361, for Windows NT*/
//kTextEncodingDOSKorean        = 0x0422, /* code page 949, also for Windows; Unified Hangul Code (EUC-KR extended)*/
	kTextEncodingMacChineseTrad,	kTextEncodingDOSChineseTrad, /* code page 950, also for Windows; Big-5*/
	kTextEncodingMacRoman,		kTextEncodingWindowsLatin1, /* code page 1252*/
	kTextEncodingMacCentralEurRoman,	kTextEncodingWindowsLatin2, /* code page 1250, Central Europe*/
	kTextEncodingMacCyrillic,	kTextEncodingWindowsCyrillic, /* code page 1251, Slavic Cyrillic*/
	kTextEncodingMacGreek,		kTextEncodingWindowsGreek, /* code page 1253*/
	kTextEncodingMacTurkish,	kTextEncodingWindowsLatin5, /* code page 1254, Turkish*/
	kTextEncodingMacHebrew,		kTextEncodingWindowsHebrew, /* code page 1255*/
	kTextEncodingMacArabic,		kTextEncodingWindowsArabic, /* code page 1256*/
	kTextEncodingMacVietnamese,	kTextEncodingWindowsVietnamese, /* code page 1258*/
//  kTextEncodingWindowsBalticRim = 0x0507, /* code page 1257*/
	kTextEncodingUnknown,			kTextEncodingUnknown
};

END_TOOLBOX_NAMESPACE

#endif
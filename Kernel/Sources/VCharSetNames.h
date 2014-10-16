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
#ifndef __VCharSetNames__
#define __VCharSetNames__

#include "Kernel/Sources/VKernelTypes.h"
#include "Kernel/Sources/VTextTypes.h"

BEGIN_TOOLBOX_NAMESPACE


/*
    More informations on charset names and charset MIBs here:
        <http://www.iana.org/assignments/character-sets>
        <http://www.icir.org/fenner/mibs/mib-index.html#IANA-CHARSET-MIB>
*/


typedef struct VCSNameMap
{
    CharSet         fCharSet;           // CharSet enum (see VTextTypes.h)
    sLONG           fMIBEnum;           // Unique CharSet identifier defined by IANA
    const wchar_t*  fName;              // Standard CharSet Name (or alias)
    const char*     fCharName;          // ICU Converter API needs ANSI char strings (see XLinuxTextConverter.cpp)
    bool            fWebCharSet;        // Charset used for Webserver
}
VCSNameMap;


static VCSNameMap sCharSetNameMap[] =
{
	{VTC_UTF_32,             1017,   L"UCS4",               "UCS4",                false},
	{VTC_UTF_32_BIGENDIAN,   1018,   L"UCS4BE",             "UCS4BE",              false},
	{VTC_UTF_32_SMALLENDIAN, 1019,   L"UCS4LE",             "UCS4LE",              false},
	{VTC_UTF_32,             1017,   L"UCS_4",              "UCS_4",               false},
	{VTC_UTF_32_BIGENDIAN,   1018,   L"UCS_4BE",            "UCS_4BE",             false},
	{VTC_UTF_32_SMALLENDIAN, 1019,   L"UCS_4LE",            "UCS_4LE",             false},
	{VTC_UTF_32,             1017,   L"UCS-4",              "UCS-4",               false},
	{VTC_UTF_32_BIGENDIAN,   1018,   L"UCS-4BE",            "UCS-4BE",             false},
	{VTC_UTF_32_SMALLENDIAN, 1019,   L"UCS-4LE",            "UCS-4LE",             false},
	{VTC_UTF_32,             1017,   L"UTF-32",             "UTF-32",              false},
	{VTC_UTF_32_BIGENDIAN,   1018,   L"UTF-32BE",           "UTF-32BE",            false},
	{VTC_UTF_32_SMALLENDIAN, 1019,   L"UTF-32LE",           "UTF-32LE",            false},
	{VTC_UTF_16,             1015,   L"UTF-16",             "UTF-16",              false},
	{VTC_UTF_16_BIGENDIAN,   1013,   L"UTF-16BE",           "UTF-16BE",            false},
	{VTC_UTF_16_SMALLENDIAN, 1014,   L"UTF-16LE",           "UTF-16LE",            false},
	{VTC_UTF_8,              106,    L"UTF-8",              "UTF-8",               true},
	{VTC_UTF_7,              1012,   L"UTF-7",              "UTF-7",               true},
	{VTC_US_ASCII,           3,      L"US-ASCII",           "US-ASCII",            false},
	{VTC_US_ASCII,           3,      L"ANSI_X3.4-1968",     "ANSI_X3.4-1968",      false},
	{VTC_US_ASCII,           3,      L"ANSI_X3.4-1986",     "ANSI_X3.4-1986",      false},
	{VTC_US_ASCII,           3,      L"ASCII",              "ASCII",               false},
	{VTC_US_ASCII,           3,      L"cp367",              "cp367",               false},
	{VTC_US_ASCII,           3,      L"csASCII",            "csASCII",             false},
	{VTC_US_ASCII,           3,      L"IBM367",             "IBM367",              false},
	{VTC_US_ASCII,           3,      L"iso-ir-6",           "iso-ir-6",            false},
	{VTC_US_ASCII,           3,      L"ISO_646.irv:1991",   "ISO_646.irv:1991",    false},
	{VTC_US_ASCII,           3,      L"ISO646-US",          "ISO646-US",           false},
	{VTC_US_ASCII,           3,      L"us",                 "us",                  false},
	{VTC_IBM437,             2011,   L"IBM437",             "IBM437",              false},
	{VTC_IBM437,             2011,   L"cp437",              "cp437",               false},
	{VTC_IBM437,             2011,   L"437",                "437",                 false},
	{VTC_IBM437,             2011,   L"csPC8CodePage437",   "csPC8CodePage437",    false},
	{VTC_US_EBCDIC,          2028,   L"ebcdic-cp-us",       "ebcdic-cp-us",        false},
	{VTC_US_EBCDIC,          2028,   L"cp037",              "cp037",               false},
	{VTC_US_EBCDIC,          2028,   L"csIBM037",           "csIBM037",            false},
	{VTC_US_EBCDIC,          2028,   L"ebcdic-cp-ca",       "ebcdic-cp-ca",        false},
	{VTC_US_EBCDIC,          2028,   L"ebcdic-cp-nl",       "ebcdic-cp-nl",        false},
	{VTC_US_EBCDIC,          2028,   L"ebcdic-cp-wt",       "ebcdic-cp-wt",        false},
	{VTC_US_EBCDIC,          2028,   L"IBM037",             "IBM037",              false},
	{VTC_MAC_ROMAN,          2027,   L"MacRoman",           "MacRoman",            false},
	{VTC_MAC_ROMAN,          2027,   L"x-mac-roman",        "x-mac-roman",         false},
	{VTC_MAC_ROMAN,          2027,   L"mac",                "mac",                 false},
	{VTC_MAC_ROMAN,          2027,   L"macintosh",          "macintosh",           false},
	{VTC_MAC_ROMAN,          2027,   L"csMacintosh",        "csMacintosh",         false},
	{VTC_WIN_ROMAN,          2252,   L"windows-1252",       "windows-1252",        false},
	{VTC_MAC_CENTRALEUROPE,  1250,   L"MacCE",              "MacCE",               false},  // Not a standard MIB enum
	{VTC_MAC_CENTRALEUROPE,  1250,   L"x-mac-ce",           "x-mac-ce",            false},  // Not a standard MIB enum
	{VTC_WIN_CENTRALEUROPE,  2250,   L"windows-1250",       "windows-1250",        false},
	{VTC_MAC_CYRILLIC,       1251,   L"x-mac-cyrillic",     "x-mac-cyrillic",      false},  // Not a standard MIB enum
	{VTC_WIN_CYRILLIC,       2251,   L"windows-1251",       "windows-1251",        false},
	{VTC_MAC_GREEK,          1253,   L"x-mac-greek",        "x-mac-greek",         false},  // Not a standard MIB enum
	{VTC_WIN_GREEK,          2253,   L"windows-1253",       "windows-1253",        false},
	{VTC_MAC_TURKISH,        1254,   L"x-mac-turkish",      "x-mac-turkish",       false},  // Not a standard MIB enum
	{VTC_WIN_TURKISH,        2254,   L"windows-1254",       "windows-1254",        false},
	{VTC_MAC_ARABIC,         1256,   L"x-mac-arabic",       "x-mac-arabic",        false},  // Not a standard MIB enum
	{VTC_WIN_ARABIC,         2256,   L"windows-1256",       "windows-1256",        false},
	{VTC_MAC_HEBREW,         1255,   L"x-mac-hebrew",       "x-mac-hebrew",        false},  // Not a standard MIB enum
	{VTC_WIN_HEBREW,         2255,   L"windows-1255",       "windows-1255",        false},
	{VTC_MAC_BALTIC,         1257,   L"x-mac-ce",           "x-mac-ce",            false},  // Not a standard MIB enum
	{VTC_WIN_BALTIC,         2257,   L"windows-1257",       "windows-1257",        false},
	{VTC_SHIFT_JIS,          17,     L"Shift_JIS",          "Shift_JIS",           true},
	{VTC_SHIFT_JIS,          17,     L"csShiftJIS",         "csShiftJIS",          false},
	{VTC_SHIFT_JIS,          17,     L"MS_Kanji",           "MS_Kanji",            false},
	{VTC_SHIFT_JIS,          17,     L"Shift-JIS",          "Shift-JIS",           false},
	{VTC_MAC_JAPANESE,       1258,   L"x-mac-japanese",     "x-mac-japanese",      false},  // Not a standard MIB enum
	{VTC_Windows31J,         2024,   L"Windows-31J",        "Windows-31J",         false},
	{VTC_Windows31J,         2024,   L"csWindows31J",       "csWindows31J",        false},
	{VTC_JIS,                39,     L"ISO-2022-JP",        "ISO-2022-JP",         false},
	{VTC_JIS,                39,     L"csISO2022JP",        "csISO2022JP",         false},
	{VTC_BIG5,               2026,   L"Big5",               "Big5",                true},
	{VTC_BIG5,               2026,   L"csBig5",             "csBig5",              false},
	{VTC_EUC_KR,             38,     L"EUC-KR",             "EUC-KR",              true},
	{VTC_EUC_KR,             38,     L"csEUCKR",            "csEUCKR",             false},
	{VTC_KOI8R,              2084,   L"KOI8-R",             "KOI8-R",              true},
	{VTC_KOI8R,              2084,   L"csKOI8R",            "csKOI8R",             false},
	{VTC_ISO_8859_1,         4,      L"ISO-8859-1",         "ISO-8859-1",          true},
	{VTC_ISO_8859_1,         4,      L"CP819",              "CP819",               false},
	{VTC_ISO_8859_1,         4,      L"csISOLatin1",        "csISOLatin1",         false},
	{VTC_ISO_8859_1,         4,      L"IBM819",             "IBM819",              false},
	{VTC_ISO_8859_1,         4,      L"iso-ir-100",         "iso-ir-100",          false},
	{VTC_ISO_8859_1,         4,      L"ISO_8859-1",         "ISO_8859-1",          false},
	{VTC_ISO_8859_1,         4,      L"ISO_8859-1:1987",    "ISO_8859-1:1987",     false},
	{VTC_ISO_8859_1,         4,      L"l1",                 "l1",                  false},
	{VTC_ISO_8859_1,         4,      L"latin1",             "latin1",              false},
	{VTC_ISO_8859_2,         5,      L"ISO-8859-2",         "ISO-8859-2",          true},
	{VTC_ISO_8859_2,         5,      L"csISOLatin2",        "csISOLatin2",         false},
	{VTC_ISO_8859_2,         5,      L"iso-ir-101",         "iso-ir-101",          false},
	{VTC_ISO_8859_2,         5,      L"ISO_8859-2",         "ISO_8859-2",          false},
	{VTC_ISO_8859_2,         5,      L"ISO_8859-2:1987",    "ISO_8859-2:1987",     false},
	{VTC_ISO_8859_2,         5,      L"l2",                 "l2",                  false},
	{VTC_ISO_8859_2,         5,      L"latin2",             "latin2",              false},
	{VTC_ISO_8859_3,         6,      L"ISO-8859-3",         "ISO-8859-3",          true},
	{VTC_ISO_8859_3,         6,      L"csISOLatin3",        "csISOLatin3",         false},
	{VTC_ISO_8859_3,         6,      L"ISO-8859-3:1988",    "ISO-8859-3:1988",     false},
	{VTC_ISO_8859_3,         6,      L"iso-ir-109",         "iso-ir-109",          false},
	{VTC_ISO_8859_3,         6,      L"ISO_8859-3",         "ISO_8859-3",          false},
	{VTC_ISO_8859_3,         6,      L"l3",                 "l3",                  false},
	{VTC_ISO_8859_3,         6,      L"latin3",             "latin3",              false},
	{VTC_ISO_8859_4,         7,      L"ISO-8859-4",         "ISO-8859-4",          true},
	{VTC_ISO_8859_4,         7,      L"csISOLatin4",        "csISOLatin4",         false},
	{VTC_ISO_8859_4,         7,      L"ISO-8859-4:1988",    "ISO-8859-4:1988",     false},
	{VTC_ISO_8859_4,         7,      L"iso-ir-110",         "iso-ir-110",          false},
	{VTC_ISO_8859_4,         7,      L"ISO_8859-4",         "ISO_8859-4",          false},
	{VTC_ISO_8859_4,         7,      L"l4",                 "l4",                  false},
	{VTC_ISO_8859_4,         7,      L"latin4",             "latin4",              false},
	{VTC_ISO_8859_5,         8,      L"ISO-8859-5",         "ISO-8859-5",          true},
	{VTC_ISO_8859_5,         8,      L"csISOLatinCyrillic", "csISOLatinCyrillic",  false},
	{VTC_ISO_8859_5,         8,      L"cyrillic",           "cyrillic",            false},
	{VTC_ISO_8859_5,         8,      L"ISO-8859-5:1988",    "ISO-8859-5:1988",     false},
	{VTC_ISO_8859_5,         8,      L"iso-ir-144",         "iso-ir-144",          false},
	{VTC_ISO_8859_5,         8,      L"ISO_8859-5",         "ISO_8859-5",          false},
	{VTC_ISO_8859_6,         9,      L"ISO-8859-6",         "ISO-8859-6",          true},
	{VTC_ISO_8859_6,         9,      L"arabic",             "arabic",              false},
	{VTC_ISO_8859_6,         9,      L"ASMO-708",           "ASMO-708",            false},
	{VTC_ISO_8859_6,         9,      L"csISOLatinArabic",   "csISOLatinArabic",    false},
	{VTC_ISO_8859_6,         9,      L"ECMA-114",           "ECMA-114",            false},
	{VTC_ISO_8859_6,         9,      L"ISO-8859-6:1987",    "ISO-8859-6:1987",     false},
	{VTC_ISO_8859_6,         9,      L"iso-ir-127",         "iso-ir-127",          false},
	{VTC_ISO_8859_6,         9,      L"ISO_8859-6",         "ISO_8859-6",          false},
	{VTC_ISO_8859_7,         10,     L"ISO-8859-7",         "ISO-8859-7",          true},
	{VTC_ISO_8859_7,         10,     L"csISOLatinGreek",    "csISOLatinGreek",     false},
	{VTC_ISO_8859_7,         10,     L"ECMA-118",           "ECMA-118",            false},
	{VTC_ISO_8859_7,         10,     L"ELOT_928",           "ELOT_928",            false},
	{VTC_ISO_8859_7,         10,     L"greek",              "greek",               false},
	{VTC_ISO_8859_7,         10,     L"greek8",             "greek8",              false},
	{VTC_ISO_8859_7,         10,     L"iso-ir-126",         "iso-ir-126",          false},
	{VTC_ISO_8859_7,         10,     L"ISO_8859-7",         "ISO_8859-7",          false},
	{VTC_ISO_8859_7,         10,     L"ISO_8859-7:1987",    "ISO_8859-7:1987",     false},
	{VTC_ISO_8859_8,         11,     L"ISO-8859-8",         "ISO-8859-8",          true},
	{VTC_ISO_8859_8,         11,     L"csISOLatinHebrew",   "csISOLatinHebrew",    false},
	{VTC_ISO_8859_8,         11,     L"hebrew",             "hebrew",              false},
	{VTC_ISO_8859_8,         11,     L"iso-ir-138",         "iso-ir-138",          false},
	{VTC_ISO_8859_8,         11,     L"ISO_8859-8",         "ISO_8859-8",          false},
	{VTC_ISO_8859_8,         11,     L"ISO_8859-8:1988",    "ISO_8859-8:1988",     false},
	{VTC_ISO_8859_9,         12,     L"ISO-8859-9",         "ISO-8859-9",          true},
	{VTC_ISO_8859_9,         12,     L"csISOLatin5",        "csISOLatin5",         false},
	{VTC_ISO_8859_9,         12,     L"iso-ir-148",         "iso-ir-148",          false},
	{VTC_ISO_8859_9,         12,     L"ISO_8859-9",         "ISO_8859-9",          false},
	{VTC_ISO_8859_9,         12,     L"ISO_8859-9:1989",    "ISO_8859-9:1989",     false},
	{VTC_ISO_8859_9,         12,     L"l5",                 "l5",                  false},
	{VTC_ISO_8859_9,         12,     L"latin5",             "latin5",              false},
	{VTC_ISO_8859_10,        13,     L"ISO-8859-10",        "ISO-8859-10",         true},
	{VTC_ISO_8859_10,        13,     L"csISOLatin6",        "csISOLatin6",         false},
	{VTC_ISO_8859_10,        13,     L"iso-ir-157",         "iso-ir-157",          false},
	{VTC_ISO_8859_10,        13,     L"ISO_8859-10",        "ISO_8859-10",         false},
	{VTC_ISO_8859_10,        13,     L"ISO_8859-10:1992",   "ISO_8859-10:1992",    false},
	{VTC_ISO_8859_10,        13,     L"l6",                 "l6",                  false},
	{VTC_ISO_8859_10,        13,     L"latin6",             "latin6",              false},
	{VTC_ISO_8859_13,        109,    L"ISO-8859-13",        "ISO-8859-13",         true},
	{VTC_ISO_8859_15,        111,    L"ISO-8859-15",        "ISO-8859-15",         true},
	{VTC_ISO_8859_15,        111,    L"Latin-9",            "Latin-9",             false},
	{VTC_GB2312,             2025,   L"GB2312",             "GB2312",              true},
	{VTC_GB2312,             2025,   L"csGB2312",           "csGB2312",            false},
	{VTC_GB2312,             2025,   L"x-mac-chinesesimp",  "x-mac-chinesesimp",   false},
	{VTC_MAC_CHINESE_SIMP,   2025,   L"x-mac-chinesesimp",  "x-mac-chinesesimp",   false},
	{VTC_GB2312_80,          57,     L"GB_2312-80",         "GB_2312-80",          false},
	{VTC_GB2312_80,          57,     L"csISO58GB231280",    "csISO58GB231280",     false},
	{VTC_WIN_CHINESE_SIMP,   113,    L"GBK",                "GBK",                 true},
	{VTC_WIN_CHINESE_SIMP,   113,    L"CP936",              "CP936",               false},
	{VTC_WIN_CHINESE_SIMP,   113,    L"MS936",              "MS936",               false},
	{VTC_WIN_CHINESE_SIMP,   113,    L"windows-936",        "windows-936",         false},
	{VTC_UNKNOWN,            0,      NULL,                  NULL,                  false}
};


END_TOOLBOX_NAMESPACE

#endif

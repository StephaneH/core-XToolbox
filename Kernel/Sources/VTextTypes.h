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
#ifndef __VTextTypes__
#define __VTextTypes__

#include "Kernel/Sources/VUnicodeTableLow.h"

BEGIN_TOOLBOX_NAMESPACE

// same values as windows virtual key codes as possible
typedef enum KeyCode
{
	KEY_UNKNOWN		= 0,	// There's no known key code available.

	KEY_BREAK		= 0x03, // windows only
	KEY_BACKSPACE	= 0x08,
	KEY_TAB			= 0x09,
	KEY_CLEAR		= 0x0C,
	KEY_RETURN		= 0x0D,
	KEY_FUNCTION	= 0x0E,	// mac function key
	KEY_COMMAND		= 0x0F,	// mac command key
	KEY_SHIFT		= 0x10,
	KEY_CTRL		= 0x11,
	KEY_ALT			= 0x12, // ALT on windows, Option on mac
	KEY_PAUSE		= 0x13, // windows only
	KEY_CAPSLOCK	= 0x14,
	KEY_ESCAPE		= 0x1B,
	KEY_SPACE		= 0x20,

	KEY_PAGEUP		= 0x21,
	KEY_PAGEDOWN	= 0x22,
	KEY_END			= 0x23,
	KEY_HOME		= 0x24,
	KEY_LEFT		= 0x25,
	KEY_UP			= 0x26,
	KEY_RIGHT		= 0x27,
	KEY_DOWN		= 0x28,
	KEY_PRINT		= 0x2A, // windows only
	KEY_ENTER		= 0x2B,
	KEY_INSERT		= 0x2D, // windows only
	KEY_DELETE		= 0x2E,
	KEY_HELP		= 0x2F, // windows only

	KEY_F1			= 0x70,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12,
	KEY_F13,
	KEY_F14,
	KEY_F15,
	KEY_F16,
	KEY_F17,
	KEY_F18,
	KEY_F19,
	KEY_F20,
	KEY_F21,
	KEY_F22,
	KEY_F23,
	KEY_F24

} KeyCode;


// IMPORTANT:
// DialectCodes are mapped exactly on Microsoft LANG and SUBLANG
// if you add DialectCodes please refer to Microsoft headers
// also check that VIntlMgr::_AdjustDialect can handle the new dialects
// INFORMATIONS : http://msdn.microsoft.com/en-us/library/ee490957.aspx
// + http://www.microsoft.com/resources/msdn/goglobal/default.mspx?OS=Windows%207
enum
{
	DC_NONE = (uLONG) 0xfffffffe,
	DC_SYSTEM = (uLONG) 0xffffffff,
	DC_USER = 0,
	DC_AFRIKAANS = (0x0001<<10) | 0x0036,
	DC_ALBANIAN = (0x0001<<10) | 0x001C,
	DC_AMHARIC = 0x005E,
	DC_ARABIC = 0x0001,
	DC_ARABIC_SAUDI_ARABIA = (0x0001<<10) |	DC_ARABIC,
	DC_ARABIC_IRAQ = (0x0002<<10) | DC_ARABIC,
	DC_ARABIC_EGYPT = (0x0003<<10) | DC_ARABIC,
	DC_ARABIC_LIBYA = (0x0004<<10) | DC_ARABIC,
	DC_ARABIC_ALGERIA = (0x0005<<10) | DC_ARABIC,
	DC_ARABIC_MOROCCO = (0x0006<<10) | DC_ARABIC,
	DC_ARABIC_TUNISIA = (0x0007<<10) | DC_ARABIC,
	DC_ARABIC_OMAN = (0x0008<<10) | DC_ARABIC,
	DC_ARABIC_YEMEN = (0x0009<<10) | DC_ARABIC,
	DC_ARABIC_SYRIA = (0x000A<<10) | DC_ARABIC,
	DC_ARABIC_JORDAN = (0x000B<<10) | DC_ARABIC,
	DC_ARABIC_LEBANON = (0x000C<<10) | DC_ARABIC,
	DC_ARABIC_KUWAIT = (0x000D<<10) | DC_ARABIC,
	DC_ARABIC_UAE = (0x000E<<10) | DC_ARABIC,
	DC_ARABIC_BAHRAIN = (0x000F<<10) | DC_ARABIC,
	DC_ARABIC_QATAR = (0x0010<<10) | DC_ARABIC,
	DC_ARMENIAN = (0x0001<<10) | 0x002B,
	DC_AZERBAIJANI = 0x002C,
	DC_AZERI_LATIN = 0x042c,
	DC_AZERI_CYRILLIC = 0x082c,
	DC_BASQUE = (0x0001<<10) | 0x002D,
	DC_BELARUSIAN = (0x0001<<10) | 0x0023,
	DC_BENGALI_INDIA = (0x0001<<10) | 0x0045,
	DC_BENGALI_BANGLADESH = (0x0002<<10) | 0x0045,
	DC_BOSNIAN = 0x141a,
	DC_BULGARIAN = (0x0001<<10) | 0x0002,
	DC_CATALAN = (0x0001<<10) | 0x0003,
	//DC_CHICHEWA,										// pas de code ?
	DC_CHINESE_TRADITIONAL = (0x0001<<10) | 0x0004,
	DC_CHINESE_SIMPLIFIED = (0x0002<<10) | 0x0004,
	DC_CHINESE_HONGKONG = (0x0003<<10) | 0x0004,
	DC_CHINESE_SINGAPORE = (0x0004<<10) | 0x0004,
	//DC_COPTIC,											// pas de code ?
	DC_CROATIAN = (0x0001<<10) | 0x001A,
	DC_CROATIAN_BOSNIA_HERZEGOVINA = 0x101a,
	DC_CZECH = (0x0001<<10) | 0x0005,
	DC_DANISH = (0x0001<<10) | 0x0006,
	DC_DUTCH = (0x0001<<10) | 0x0013,
	DC_DUTCH_BELGIAN = (0x0002<<10) | 0x0013,
	DC_DUTCH_FRISIAN = (0x0001<<10) | 0x0062,
	DC_ENGLISH_US = (0x0001<<10) | 0x0009,
	DC_ENGLISH_UK = (0x0002<<10) | 0x0009,
	DC_ENGLISH_AUSTRALIA = (0x0003<<10) | 0x0009,
	DC_ENGLISH_CANADA = (0x0004<<10) | 0x0009,
	DC_ENGLISH_NEWZEALAND = (0x0005<<10) | 0x0009,
	DC_ENGLISH_EIRE = (0x0006<<10) | 0x0009,
	DC_ENGLISH_SOUTH_AFRICA = (0x0007<<10) | 0x0009,
	DC_ENGLISH_JAMAICA = (0x0008<<10) | 0x0009,
	DC_ENGLISH_CARIBBEAN = (0x0009<<10) | 0x0009,
	DC_ENGLISH_BELIZE = (0x000A<<10) | 0x0009,
	DC_ENGLISH_TRINIDAD = (0x000B<<10) | 0x0009,
	//DC_ESPERANTO,										// pas de code ?
	DC_ESTONIAN = (0x0001<<10) | 0x0025,
	DC_FAEROESE = (0x0001<<10) | 0x0038,
	DC_FARSI = (0x0001<<10) | 0x0029,					// garder a titre conservatoire, equivaut a PERSIAN
	//DC_FIJIAN,											// pas de code ?
	DC_FINNISH = (0x0001<<10) | 0x000B,
	DC_FRENCH = (0x0001<<10) | 0x000C,
	DC_FRENCH_BELGIAN = (0x0002<<10) | 0x000C,
	DC_FRENCH_CANADIAN = (0x0003<<10) | 0x000C,
	DC_FRENCH_SWISS = (0x0004<<10) | 0x000C,
	DC_FRENCH_LUXEMBOURG = (0x0005<<10) | 0x000C,
	DC_FRISIAN = (0x0001<<10) | 0x0062,
	//DC_FRIULIAN,										// pas de code ?
	DC_GAELIC_SCOTLAND = (0x0001<<10) | 0x0091,
	DC_GALICIAN_SPANISH = (0x0001<<10) | 0x0056,
	//DC_GASCON,											// pas de code ?
	DC_GERMAN = (0x0001<<10) | 0x0007,
	DC_GERMAN_SWISS = (0x0002<<10) | 0x0007,
	DC_GERMAN_AUSTRIAN = (0x0003<<10) | 0x0007,
	DC_GERMAN_LUXEMBOURG = (0x0004<<10) | 0x0007,
	DC_GERMAN_LIECHTENSTEIN = (0x0005<<10) | 0x0007,
	DC_LOWER_SORBIAN = (0x0002<<10) | 0x002E,
	DC_UPPER_SORBIAN = (0x0001<<10) | 0x002E,
	DC_GREEK = (0x0001<<10) | 0x0008,
	DC_GUJARATI = (0x0001<<10) | 0x0047,
	//DC_HAWAIIAN_UNITED_STATE,							// pas de code ?
	DC_HINDI = (0x0001<<10) | 0x0039,
	DC_HEBREW = (0x0001<<10) | 0x000D,
	DC_HUNGARIAN = (0x0001<<10) | 0x000E,
	DC_ICELANDIC = (0x0001<<10) | 0x000F,
	DC_INDONESIAN = (0x0001<<10) | 0x0021,
	//DC_INTERLINGUA,										// pas de code ?
	DC_IRISH = (0x0002<<10)| 0x003C,
	DC_ITALIAN = (0x0001<<10) | 0x0010,
	DC_ITALIAN_SWISS = (0x0002<<10) | 0x0010,
	DC_JAPANESE = (0x0001<<10) | 0x0011,
	//DC_KASHUBIAN,										// pas de code ?
	DC_KHMER = 0x0053,
	DC_KINYARWANDA = (0x0001<<10) | 0x0087,
	DC_KISWAHILI = (0x0001<<10) | 0x0041,
	DC_KOREAN_WANSUNG = (0x0001<<10) | 0x0012,
	DC_KOREAN_JOHAB = (0x0002<<10) | 0x0012,
	//DC_KURDISH,											// pas de code ?
	DC_LATVIAN = (0x0001<<10) | 0x0026,
	//DC_LINGALA,											// pas de code ?
	DC_LITHUANIAN = (0x0001<<10) | 0x0027,
	DC_LUXEMBOURGISH = (0x0001<<10) | 0x006E,
	DC_MACEDONIAN = (0x0001<<10) | 0x002F,
	DC_MALAYSIAN = (0x0001<<10) | 0x003E,
	//DC_MALAGASY,										// pas de code ?
	DC_MALTE = 0x003A,
	DC_MAORI = 0x0081,
	DC_MARATHI = (0x0001<<10) | 0x004E,
	DC_MALAYALAM = (0x0001<<10) | 0x004C,
	DC_MONGOLIAN = (0x0001<<10) | 0x0050,
	//DC_NDEBELE,											// pas de code ?
	DC_NEPALI = (0x0001<<10) | 0x0061,
	//DC_NORTHEN_SOTHO,									// pas de code ?
	DC_OCCITAN = (0x0001<<10) | 0x0082,
	DC_ORIYA = 0x0048,
	DC_NORWEGIAN = (0x0001<<10) | 0x0014,
	DC_NORWEGIAN_BOKMAL = (0x0001<<10) | 0x0014,
	DC_NORWEGIAN_NYNORSK = (0x0002<<10) | 0x0014,
	DC_PERSIAN = (0x0001<<10) | 0x0029,
	DC_POLISH = (0x0001<<10) | 0x0015,
	DC_PORTUGUESE = (0x0002<<10) | 0x0016,
	DC_PORTUGUESE_BRAZILIAN = (0x0001<<10) | 0x0016,
	DC_PUNJABI = 0x0046,
	DC_QUECHUA = (0x0001<<10) | 0x006B,
	//DC_QUICHUA,											// pas de code ?
	DC_ROMANIAN = (0x0001<<10) | 0x0018,
	DC_RUSSIAN = (0x0001<<10) | 0x0019,
	DC_SAMI_NORTHERN_NORWAY  =  (0x0001 << 10) | 0x003B,     // Northern Sami (Norway)
	DC_SAMI_NORTHERN_SWEDEN  =  (0x0002 << 10) | 0x003B,    // Northern Sami (Sweden)
	DC_SAMI_NORTHERN_FINLAND =  (0x0003 << 10) | 0x003B,    // Northern Sami (Finland)
	DC_SAMI_LULE_NORWAY = (0x0004 << 10) | 0x003B,    // Lule Sami (Norway)
	DC_SAMI_LULE_SWEDEN  =  (0x0005 << 10) | 0x003B,    // Lule Sami (Sweden)
	DC_SAMI_SOUTHERN_NORWAY = (0x0006 << 10) | 0x003B,    // Southern Sami (Norway)
	DC_SAMI_SOUTHERN_SWEDEN = (0x0007 << 10) | 0x003B,    // Southern Sami (Sweden)
	DC_SAMI_SKOLT_FINLAND   = (0x0008 << 10) | 0x003B,    // Skolt Sami (Finland)
	DC_SAMI_INARI_FINLAND   = (0x0009 << 10) | 0x003B,    // Inari Sami (Finland)
	DC_SERBIAN = (0x0001<<10) | 0x001A,
	DC_SERBIAN_LATIN = (0x0002<<10) | 0x001A,
	DC_SERBIAN_LATIN_BOSNIAN = 0x181a,
	DC_SERBIAN_CYRILLIC = (0x0003<<10) | 0x001A,
	DC_SERBIAN_CYRILLIC_BOSNIAN = 0x1c1a,
	DC_SETSWANA = (0x0001<<10) | 0x0032,
	DC_SINHALA = (0x0001<<10) | 0x005B,
	DC_SLOVAK = (0x0001<<10) | 0x001B,
	DC_SLOVENIAN = (0x0001<<10) | 0x0024,
	DC_SPANISH_CASTILLAN = (0x0001<<10) | 0x000A,
	DC_SPANISH_MEXICAN = (0x0002<<10) | 0x000A,
	DC_SPANISH_MODERN = (0x0003<<10) | 0x000A,
	DC_SPANISH_GUATEMALA = (0x0004<<10) | 0x000A,
	DC_SPANISH_COSTA_RICA = (0x0005<<10) | 0x000A,
	DC_SPANISH_PANAMA = (0x0006<<10) | 0x000A,
	DC_SPANISH_DOMINICAN_REPUBLIC = (0x0007<<10) | 0x000A,
	DC_SPANISH_VENEZUELA = (0x0008<<10) | 0x000A,
	DC_SPANISH_COLOMBIA = (0x0009<<10) | 0x000A,
	DC_SPANISH_PERU = (0x000A<<10) | 0x000A,
	DC_SPANISH_ARGENTINA = (0x000B<<10) | 0x000A,
	DC_SPANISH_ECUADOR = (0x000C<<10) | 0x000A,
	DC_SPANISH_CHILE = (0x000D<<10) | 0x000A,
	DC_SPANISH_URUGUAY = (0x000E<<10) | 0x000A,
	DC_SPANISH_PARAGUAY = (0x000F<<10) | 0x000A,
	DC_SPANISH_BOLIVIA = (0x0010<<10) | 0x000A,
	DC_SPANISH_EL_SALVADOR = (0x0011<<10) | 0x000A,
	DC_SPANISH_HONDURAS = (0x0012<<10) | 0x000A,
	DC_SPANISH_NICARAGUA = (0x0013<<10) | 0x000A,
	DC_SPANISH_PUERTO_RICO = (0x0014<<10) | 0x000A,
	//DC_SOUTHERN_SOTHO,									// pas de code ?
	//DC_SWAZI_SWATI,										// pas de code ?
	DC_SWEDISH = (0x0001<<10) | 0x001D,
	DC_SWEDISH_FINLAND = (0x0002<<10) | 0x001D,
	//DC_TAGALOG,											// pas de code ?
	DC_TADJIK = 0x0428,
	DC_TAMIL = 0x0049,
	//DC_TETUM,											// pas de code ?
	DC_TATAR = 0x0444,
	DC_THAI = (0x0001<<10) | 0x001E,
	//DC_TSONGA,											// pas de code ?
	DC_TURKISH = (0x0001<<10) | 0x001F,
	DC_UKRAINIAN = (0x0001<<10) | 0x0022,
	DC_URDU = (0x0001<<10) | 0x0020,
	//DC_VENDA,											// pas de code ?
	DC_UZBEK_LATIN = 0x0443,
	DC_UZBEK_CYRILLIC = 0x0843,
	DC_VIETNAMESE = (0x0001<<10) | 0x002A,
	DC_WELSH = (0x0001<<10) | 0x0052,
	DC_XHOSA = (0x0001<<10) | 0x0034,
	//DC_YIDDISH,											// pas de code ?
	DC_ZULU = (0x0001<<10) | 0x0035
};		// IMPORTANT: please read comment above
typedef uLONG	DialectCode;	// fixed size type for easy streaming

/**
	VCollator options
**/
enum
{
	COL_ICU = 1,	// use ICU
	COL_IgnoreWildCharInMiddle	= 2,	//  in CompareLike the wildchar is only considered when at the beginning or at the end of the pattern
	COL_EqualityUsesSecondaryStrength = 4,		// uses secondary collation strength when matching strings
	COL_ComparisonUsesQuaternaryStrength = 8,	// uses quaternary collation strength when sorting strings
	COL_ComparisonUsesHiraganaQuaternary = 16,	// Hiragana code points will sort before everything else on the quaternary level.
	COL_ConsiderOnlyDeadCharsForKeywords = 32,	// Use language neutral deadchar algorithm in VIntlMgr::FindWord and VIntl::GetWordBoundaries. Else uses icu::BreakIterator.
	COL_TraditionalStyleOrdering = 64	// Use traditional-style sorting
};
typedef uBYTE	CollatorOptions;	// fixed size type for easy streaming


/** A dialect with ISO properties.
* This struct gathers all the ISO properties that define a language.
* You can obtain the RFC3066bis code of a language easily : RFC3066 Code = fISO6391Code + fISO3166RegionCode.
* @see VIntlMgr
*/
typedef struct Dialect
{
	DialectCode			fDialectCode;
	const char*			fISO6391Code;
	const char*			fISO6392Code;
	const char*			fISOLegacyName;
	const char*			fISO3166RegionCode;
	const char*			fRFC3066SubTag;
}Dialect;

typedef struct DialectKeyboard
{
	DialectCode			fDialectCode;
	const char*			fKeyboardName;
	long				fKeyboardID;
	long				fMapMode;
}DialectKeyboard;


typedef enum CharSet
{
	VTC_UNKNOWN = 0,

	VTC_UTF_16_BIGENDIAN = 1,
	VTC_UTF_16_SMALLENDIAN = 2,
	
#if BIGENDIAN
	VTC_UTF_16 = VTC_UTF_16_BIGENDIAN,
	VTC_UTF_16_ByteSwapped = VTC_UTF_16_SMALLENDIAN,
#else
	VTC_UTF_16 = VTC_UTF_16_SMALLENDIAN,
	VTC_UTF_16_ByteSwapped = VTC_UTF_16_BIGENDIAN,
#endif
	
	VTC_UTF_32_BIGENDIAN = 3,
	VTC_UTF_32_SMALLENDIAN = 4,

#if BIGENDIAN
	VTC_UTF_32 = VTC_UTF_32_BIGENDIAN,
#else
	VTC_UTF_32 = VTC_UTF_32_SMALLENDIAN,
#endif

	// UTF32_RAW is basically a UTF16 coded on 4 bytes, so it is not a true UTF32.
	// But that's usefull for hard-coded strings like L"coucou" with 32 bits wchar_t types.

	// VTC_WCHAR might be VTC_UTF_16 or VTC_UTF_32 (defined in platform includes),
	// it corresponds to the charset of the wchar_t type.
	
	VTC_UTF_32_RAW_BIGENDIAN = 5,
	VTC_UTF_32_RAW_SMALLENDIAN = 6,

#if BIGENDIAN
	VTC_UTF_32_RAW = VTC_UTF_32_RAW_BIGENDIAN,
	VTC_UTF_32_RAW_ByteSwapped = VTC_UTF_32_RAW_SMALLENDIAN,
#else
	VTC_UTF_32_RAW = VTC_UTF_32_RAW_SMALLENDIAN,
	VTC_UTF_32_RAW_ByteSwapped = VTC_UTF_32_RAW_BIGENDIAN,
#endif

#if COMPIL_GCC
	VTC_WCHAR_T = VTC_UTF_32,
#elif COMPIL_VISUAL
	VTC_WCHAR_T = VTC_UTF_16,
#endif

	VTC_UTF_8 = 7,
	VTC_UTF_7 = 8,

	// ASCII (7 bits)
	VTC_US_ASCII = 9,
	VTC_US_EBCDIC = 10,

	VTC_IBM437 = 11,	// common in DIF and DBF file formats
	
	// VTC_MacOSAnsi & VTC_Win32Ansi are the charsets used by non-unicode system apis.
#if VERSIONMAC
	VTC_MacOSAnsi = -2,
#elif VERSION_LINUX
    VTC_LinuxAnsi = -2,
#elif VERSIONWIN
	VTC_Win32Ansi = -2,
#endif

	// VTC_DefaultTextExport is the charset typically used to produce a simple .txt file
	// This is usually enhanced ascii which may vary with the localization of the system.
	// This is why it cannot be defined as some ISO value.
	// ex: VTC_DefaultTextExport might be VTC_MAC_ROMAN or VTC_MAC_GREEK and so on
#if VERSIONMAC
	VTC_DefaultTextExport = VTC_MacOSAnsi,
#elif VERSIONWIN
	VTC_DefaultTextExport = VTC_Win32Ansi,
#elif VERSION_DARWIN
	VTC_DefaultTextExport = VTC_UTF_8,
#elif VERSION_LINUX
	VTC_DefaultTextExport = VTC_UTF_8,
#endif

	// the charset used by compilers for char* constants
#if COMPIL_GCC
	VTC_StdLib_char = VTC_UTF_8,		// just a guess
#elif COMPIL_VISUAL
	VTC_StdLib_char = VTC_Win32Ansi,	// just a guess
#endif
	
	// Platform specific
	VTC_MAC_ROMAN	= 100,
	VTC_WIN_ROMAN,
	VTC_MAC_CENTRALEUROPE,
	VTC_WIN_CENTRALEUROPE,
	VTC_MAC_CYRILLIC,
	VTC_WIN_CYRILLIC,
	VTC_MAC_GREEK,
	VTC_WIN_GREEK,
	VTC_MAC_TURKISH,
	VTC_WIN_TURKISH,
	VTC_MAC_ARABIC,
	VTC_WIN_ARABIC,
	VTC_MAC_HEBREW,
	VTC_WIN_HEBREW,
	VTC_MAC_BALTIC,
	VTC_WIN_BALTIC,
	VTC_MAC_CHINESE_SIMP,
	VTC_WIN_CHINESE_SIMP,
	VTC_MAC_CHINESE_TRAD,
	VTC_WIN_CHINESE_TRAD,
	VTC_MAC_JAPANESE,	// code page 10001 on windows. not defined by IANA.
	// Internet set
	VTC_SHIFT_JIS	= 1000,	// Japan - Shift-JIS (Mac et Windows)
	VTC_JIS,			// Japan - JIS ou ISO-2022-JP (pour les e-mails)
	VTC_BIG5,			// Chinese (Traditional)
	VTC_EUC_KR,			// Corean
	VTC_KOI8R,			// Cyrillic
	VTC_ISO_8859_1,		// West Europe
	VTC_ISO_8859_2,		// Central Europe CP1250
	VTC_ISO_8859_3,		// 
	VTC_ISO_8859_4,		// Baltic
	VTC_ISO_8859_5,		// Cyrillic
	VTC_ISO_8859_6,		// Arab
	VTC_ISO_8859_7,		// Greek
	VTC_ISO_8859_8,		// Hebrew
	VTC_ISO_8859_9,		// Turkish
	VTC_ISO_8859_10,	// Nordic + Baltic (not available under Windows)
	VTC_ISO_8859_13,	// Baltic countries (not available under Windows)
	VTC_GB2312,			// Chinese (simplified)
	VTC_GB2312_80,		// Chinese (simplified)
	VTC_ISO_8859_15,	// ISO-Latin-9
	VTC_Windows31J,		// code page 932 on windows is actually Windows-31J and not SHIFT-JIS according to IANA

// !WARNING! CharSet values are streamed. Never insert or delete a value in the enum definition.

	VTC_LastCharset
} CharSet;

END_TOOLBOX_NAMESPACE

#endif

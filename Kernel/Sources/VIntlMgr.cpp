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

#if USE_ICU
#include "unicode/uclean.h"
#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/locid.h"
#include "unicode/translit.h"
#include "unicode/unorm.h"
#include "unicode/brkiter.h"
#include "unicode/rbbi.h"
#include "unicode/ubidi.h"
//#include "unicode/ustring.h"
//#include "unicode/ucnv.h"
//#include "unicode/unistr.h"
//#include "unicode/coll.h"
//#include "unicode/coleitr.h"
//#include "unicode/usearch.h"
#endif

#include "VCollator.h"
#include "VIntlMgr.h"
#include "VMemoryCpp.h"
#include "VArrayValue.h"
#include "VProcess.h"
#include "VResource.h"
#include "VFolder.h"
#include "VFile.h"
#include "VFileStream.h"
#include "VLibrary.h"
#include "VUnicodeResources.h"
#include "VUnicodeRanges.h"
#include "VUnicodeTableFull.h"
#include "VProfiler.h"

#if USE_MECAB
#include "Mecab4DInterface.h"
#endif

/*typedef struct Dialect
	{
		DialectCode			fDialectCode;
		const char*			fISO6391Code;
		const char*			fISO6392Code;
		const char*			fISOLegacyName;
		const char*			fISO3166RegionCode;
		const char*			fRFC3066SubTag;
	}Dialect;*/

// INFORMATION
// code 6393 et 6392 : http://www.loc.gov/standards/iso639-2/php/code_list.php 
// et http://gbif-spreadsheet-processor.googlecode.com/svn-history/r281/trunk/api/classes/gbif.custom-eml.php
// ISO 3166 (on peut trouver aussi des rfc 3066) : http://www.i18nguy.com/unicode/language-identifiers.html 
static Dialect sDialectsList[] =
{
	{DC_AFRIKAANS,					"af",	"afr",	"afrikaans",	"ZA",	 	""},		// nluc_Afrikaans_h
	{DC_ALBANIAN,					"sq",	"alb",	"albanian",		"AL",	 	""},
	{DC_AMHARIC,						"am",	"amh",	"amharic",		"",			""},		// nluc_Amharic_h
	{DC_ARABIC,						"ar",	"ara",	"arabic",		"",			""},		// nluc_Arabic_h
	{DC_ARABIC_SAUDI_ARABIA,			"ar",	"ara",	"arabic",		"SA",	 	"sa"},
	{DC_ARABIC_IRAQ,					"ar",	"ara",	"arabic",		"IQ",	 	"iq"},
	{DC_ARABIC_EGYPT,				"ar",	"ara",	"arabic",		"EG",	 	"eg"},
	{DC_ARABIC_LIBYA,				"ar",	"ara",	"arabic",		"LY",	 	"ly"},
	{DC_ARABIC_ALGERIA,				"ar",	"ara",	"arabic",		"DZ",	 	"dz"},
	{DC_ARABIC_MOROCCO,				"ar",	"ara",	"arabic",		"MA",	 	"ma"},
	{DC_ARABIC_TUNISIA,				"ar",	"ara",	"arabic",		"TN",	 	"tn"},
	{DC_ARABIC_OMAN,					"ar",	"ara",	"arabic",		"OM",	 	"om"},
	{DC_ARABIC_YEMEN,				"ar",	"ara",	"arabic",		"YE",	 	"ye"},
	{DC_ARABIC_SYRIA,				"ar",	"ara",	"arabic",		"SY",	 	"sy"},
	{DC_ARABIC_JORDAN,				"ar",	"ara",	"arabic",		"JO",	 	"jo"},
	{DC_ARABIC_LEBANON,				"ar",	"ara",	"arabic",		"LB",	 	"lb"},
	{DC_ARABIC_KUWAIT,				"ar",	"ara",	"arabic",		"KW",	 	"kw"},
	{DC_ARABIC_UAE,					"ar",	"ara",	"arabic",		"AE",	 	"ae"},
	{DC_ARABIC_BAHRAIN,				"ar",	"ara",	"arabic",		"BH",	 	"bh"},
	{DC_ARABIC_QATAR,				"ar",	"ara",	"arabic",		"QA",	 	"qa"},
	{DC_ARMENIAN,					"hy",	"hye",	"armenian",		"AM",		""},		// nluc_Armenian_h
	{DC_AZERBAIJANI,					"az",	"aze",	"azerbaijani",	"",			""},		// nluc_Azerbaijani_h
	{DC_BASQUE,						"eu",	"baq",	"spanish",		"ES",	 	"es"},	// nluc_Basque_h
	{DC_BELARUSIAN,					"be",	"bel",	"belarusian",	"BY",	 	""},		// nluc_Belarusian_h
	{DC_BENGALI_INDIA,				"bn",	"ben",	"bangladesh",	"BD",		"bd"},	// nluc_Bengali_h
	{DC_BULGARIAN,					"bg",	"bul",	"bulgarian",	"BG",	 	""},		// nluc_Bulgarian_h
	{DC_CATALAN,						"ca",	"cat",	"catalan",		"ES",		""},		// nluc_Catalan_h
	//{DC_CHICHEWA,					"ny",	"nya",	"chichewa",		"MW",		""},		// nluc_Chichewa_h
	{DC_CHINESE_TRADITIONAL,			"zh",	"chi",	"chinese",		"TW",		"Hant"},
	{DC_CHINESE_SIMPLIFIED,			"zh",	"chi",	"chinese",		"CN",		"Hans"},
	{DC_CHINESE_HONGKONG,			"zh",	"chi",	"chinese",		"HK",	 	"hk"},
	{DC_CHINESE_SINGAPORE,			"zh",	"chi",	"chinese",		"SG",	 	"sg"},
	//{DC_COPTIC,						"cop",	"cop",	"coptic",		"EG",		""},		// nluc_Coptic_h
	{DC_CROATIAN,					"hr",	"scr",	"croatian",		"HR",	 	""},		// nluc_Croatian_h
	{DC_CZECH,						"cs",	"cze",	"czech",		"CZ",	 	""},		// nluc_Czech_h
	{DC_DANISH,						"da",	"dan",	"danish",		"DK",	 	""},		// nluc_Danish_h
	{DC_DUTCH,						"nl",	"dut",	"dutch",		"NL",	 	""},		// nluc_Dutch_h
	{DC_DUTCH_BELGIAN,				"nl",	"dut",	"dutch",		"BE",	 	"be"},
	{DC_DUTCH_FRISIAN,				"fy",	"dut",	"dutch",		"NL",		"nl"},	// nluc_Dutch_Frisian_h
	{DC_ENGLISH_US,					"en",	"eng",	"english",		"US",	 	""},		// nluc_American_h
	{DC_ENGLISH_UK,					"en",	"eng",	"english",		"GB",	 	"gb"},	// nluc_English_h
	{DC_ENGLISH_AUSTRALIA,			"en",	"eng",	"english",		"AU",	 	"au"},	// nluc_EnglishAustralian_h
	{DC_ENGLISH_CANADA,				"en",	"eng",	"english",		"CA",	 	"ca"},	// nluc_EnglishCanadian_h
	{DC_ENGLISH_NEWZEALAND,			"en",	"eng",	"english",		"NZ",	 	"nz"},	// nluc_EnglishNewZealand_h
	{DC_ENGLISH_EIRE,				"en",	"eng",	"english",		"IE",	 	"ie"},	// nluc_EnglishIrish_h
	{DC_ENGLISH_SOUTH_AFRICA,		"en",	"eng",	"english",		"ZA",	 	"za"},	// nluc_EnglishSouthAfrican_h
	{DC_ENGLISH_JAMAICA,				"en",	"eng",	"english",		"JM",	 	"jm"},
	{DC_ENGLISH_CARIBBEAN,			"en",	"eng",	"english",		"CB",	 	"cb"},
	{DC_ENGLISH_BELIZE,				"en",	"eng",	"english",		"BZ",	 	"bz"},
	{DC_ENGLISH_TRINIDAD,			"en",	"eng",	"english",		"TT",	 	"tt"},
	//{DC_ESPERANTO,					"eo",	"epo",	"esperanto",	"l3",		""},		// nluc_Esperanto_h
	{DC_ESTONIAN,					"et",	"est",	"estonian",		"EE",	 	""},		// nluc_Estonian_h
	{DC_FAEROESE,					"fo",	"fao",	"faorese",		"FO",	 	""},		// nluc_Faroese_h
	{DC_FARSI,						"fa",	"per",	"persian",		"IR",	 	""},
	//{DC_FIJIAN,						"fj",	"fij",	"fijian",		"FJ",		""},		// nluc_Fijian_h
	{DC_FINNISH,						"fi",	"fin",	"finnish",		"FI",	 	""},		// nluc_Finnish_h
	{DC_FRENCH,						"fr",	"fre",	"french",		"FR",	 	""},		// nluc_French_h
	{DC_FRENCH_BELGIAN,				"fr",	"fre",	"french",		"BE",	 	"be"},
	{DC_FRENCH_CANADIAN,				"fr",	"fre",	"french",		"CA",	 	"ca"},
	{DC_FRENCH_SWISS,				"fr",	"fre",	"french",		"CH",	 	"ch"},
	{DC_FRENCH_LUXEMBOURG,			"fr",	"fre",	"french",		"LU",	 	"lu"},
	//{DC_FRIULIAN,					"fur",	"fur",	"italian",		"IT",		"it"},	// nluc_Friulian_h
	{DC_GAELIC_SCOTLAND,				"gd",	"eng",	"english",		"GB",		""},		// nluc_Scottish_Gaelic_h
	{DC_GALICIAN_SPANISH,			"gl",	"spa",	"spanish",		"ES",		""},		// nluc_Galicien_h
	//{DC_GASCON,						"gsc",	"fre",	"french",		"FR",		""},		// nluc_Gascon_h
	{DC_GERMAN,						"de",	"ger",	"german",		"DE",	 	""},		// nluc_German_h
	{DC_GERMAN_SWISS,				"de",	"ger",	"german",		"CH",	 	"ch"},	// nluc_GermanSwiss_h
	{DC_GERMAN_AUSTRIAN,				"de",	"ger",	"german",		"AT",	 	"at"},	// nluc_GermanAustrian_h
	{DC_GERMAN_LUXEMBOURG,			"de",	"ger",	"german",		"LU",	 	"lu"},
	{DC_GERMAN_LIECHTENSTEIN,		"de",	"ger",	"german",		"LI",	 	"li"},
	{DC_GREEK,						"el",	"gre",	"greek",		"GR",	 	""},		// nluc_Greek_h
	{DC_GUJARATI,					"gu",	"guj",	"india",		"IN",		""},		// nluc_Gujarati_h
	//{DC_HAWAIIAN_UNITED_STATE,		"haw",	"eng",	"english",		"US",		""},		// nluc_Hawaiian_h
	{DC_HEBREW,						"he",	"heb",	"hebrew",		"IL",	 	""},		// nluc_Hebrew_h
	{DC_HINDI,						"hi",	"hin",	"india",		"IN",		""},		// nluc_Hindi_h
	{DC_HUNGARIAN,					"hu",	"hun",	"hungarian",	"HU", 	 	""}, 	// nluc_Hungrian_h
	{DC_ICELANDIC,					"is",	"ice",	"iceland",		"IS", 	 	""}, 	// nluc_Icelandic_h
	{DC_INDONESIAN,					"id",	"ind",	"indonesian",	"ID", 	 	""}, 	// nluc_Indonesian_h
	//{DC_INTERLINGUA,					"ia",	"ina",	"interlingua",	"",			""},		// nluc_Interlingua_h
	{DC_IRISH,						"ga",	"gle",	"ireland",		"IE",		""},		// nluc_Irish_h
	{DC_ITALIAN,						"it",	"ita",	"italian",		"IT", 	 	""}, 	// nluc_Italian_h
	{DC_ITALIAN_SWISS,				"it",	"ita",	"italian",		"CH", 	 	"ch"},
	{DC_JAPANESE,					"ja",	"jpn",	"japanese",		"JP", 	 	""},
	//{DC_KASHUBIAN,					"csb",	"csb",	"kashubian",	"",			""},		// nluc_Kashubian_h
	{DC_KHMER,						"kh",	"khm",	"khmer",		"",			""},		// nluc_Khmer_h
	{DC_KINYARWANDA,					"rw",	"kin",	"kinyarwanda",	"RW",		""},		// nluc_Kinyarwanda_h
	{DC_KISWAHILI,					"sw",	"swk",	"kiswahili",	"KE",		""},		// nluc_Kiswahili_h
	{DC_KOREAN_WANSUNG,				"ko",	"kor",	"korean",		"",		 	""},		// nluc_Korean_h
	{DC_KOREAN_JOHAB,				"ko",	"kor",	"korean",		"",		 	""},
	//{DC_KURDISH,						"ku",	"kur",	"kurdish",		"TR",		""},		// nluc_Kurdish_h
	{DC_LATVIAN,						"lv",	"lav",	"latvian",		"LV",		""},		// nluc_Latvian_h
	//{DC_LINGALA,						"ln",	"lin",	"lingala",		"CD",		"cd"},	// nluc_Lingala_h
	{DC_LITHUANIAN,					"lt",	"lit",	"lithuanian",	"LT",	 	""},		// nluc_Lithuanian_h
	{DC_LUXEMBOURGISH,				"lb",	"ltz",	"luxembourg",	"LU",		"lu"},	// nluc_Luxembourgish_h
	{DC_MACEDONIAN,					"mk",	"mkd",	"macedonian",	"MK",		""},		// nluc_Macedonian_h
	{DC_MALAYSIAN,					"ms",	"may",	"malaysian",	"MY",		"my"},	// nluc_Malaya_h
	//{DC_MALAGASY,					"mg",	"mlg",	"malagasy",		"MG",		""},		// nluc_Malagasy_h
	{DC_MALTE,						"mt",	"mlt",	"malte",		"",			""},
	{DC_MAORI,						"mi",	"mao",	"new zealand",	"",			""},		// nluc_Maori_h
	{DC_MARATHI,						"mr",	"mar",	"india",		"IN",		""},		// nluc_Marathi_h
	{DC_MALAYALAM,					"ml",	"mal",	"india",		"IN",		""},		// nluc_Malayalam_h
	{DC_MONGOLIAN,					"mn",	"mon",	"mongolian",	"MN",		""},		// nluc_Mongolian_h
	//{DC_NDEBELE,						"nr",	"nbl",	"afrikaans",	"ZA",		""},		// nluc_Ndebele_h
	{DC_NEPALI,						"ne",	"nep",	"nepali",		"NP",		""},		// nluc_Nepali_h
	//{DC_NORTHEN_SOTHO,				"ns",	"nso",	"afrikaans",	"ZA",		""},		// nluc_Northern_Sotho_h
	{DC_NORWEGIAN,					"no",	"nor",	"norwegian",	"NO",    	""},
	{DC_NORWEGIAN_BOKMAL,			"nb",	"nob",	"bokmal",		"NO",    	"no"},   // nluc_NorskBokmal_h
	{DC_NORWEGIAN_NYNORSK,			"nn",	"nno",	"nynorsk",		"NO",    	"no"},   // nluc_NorskNynorsk_h
	{DC_OCCITAN,						"oc",	"oci",	"french",		"FR",		"fr"},	// nluc_Occitan_h
	{DC_ORIYA,						"or",	"ori",	"oriya",		"",			""},		// nluc_Oriya_h
	{DC_PERSIAN,						"fa",	"peo",	"persian",		"IR",		"ir"},	// nluc_Persian_h
	{DC_POLISH,						"pl",	"pol",	"polish",		"PL",    	""},		// nluc_Polish_h
	{DC_PORTUGUESE,					"pt",	"por",	"portuguese",	"PT",    	""},		// nluc_Portuguese_h
	{DC_PORTUGUESE_BRAZILIAN,		"pt",	"por",	"portuguese",	"BR",	 	"br"},	// nluc_PortugueseBrazilian_h
	{DC_PUNJABI,						"pa",	"pan",	"punjabi",		"",			""},		// nluc_Punjabi_h
	{DC_QUECHUA,						"qu",	"que",	"bolivia",		"BO",		""},		// nluc_Quechua_h
	//{DC_QUICHUA,						"qu",	"",		"ecuador",		"EC",		""},		// nluc_Quichua_h
	{DC_ROMANIAN,					"ro",	"rum",	"romanian",		"RO",	 	""},		// nluc_Rumanian_h
	{DC_RUSSIAN,						"ru",	"rus",	"russian",		"RU",	 	""},		// nluc_Russian_h
	{DC_SAMI_NORTHERN_NORWAY,		"se",	"sme",	"Sami",			"",			""},		// nluc_Sami_Northern_h
	{DC_SAMI_NORTHERN_SWEDEN,		"se",	"sme",	"Sami",			"",			""},		// nluc_Sami_Northern_h
	{DC_SAMI_NORTHERN_FINLAND,		"se",	"sme",	"Sami",			"",			""},		// nluc_Sami_Northern_h
	{DC_SAMI_LULE_NORWAY,			"smj",	"smj",	"Sami",			"",			""},		// nluc_Sami_Lule_h
	{DC_SAMI_LULE_SWEDEN,			"smj",	"smj",	"Sami",			"",			""},		// nluc_Sami_Lule_h
	{DC_SAMI_SOUTHERN_NORWAY,		"sma",	"sma",	"Sami",			"",			""},		// nluc_Sami_Southern_h
	{DC_SAMI_SOUTHERN_SWEDEN,		"sma",	"sma",	"Sami",			"",			""},		// nluc_Sami_Southern_h
	{DC_SAMI_SKOLT_FINLAND,			"sms",	"sms",	"Sami",			"",			""},// Skolt Sami (Finland)
	{DC_SAMI_INARI_FINLAND,			"smn",	"smn",	"Sami",			"",			""},// Inari Sami (Finland)
	{DC_SERBIAN,						"sr",	"scc",	"serbian",		"",			""},		// nluc_Serbian_h
	{DC_SERBIAN_LATIN,				"sr",	"scc",	"serbian",		"RS",	 	"Latn"},
	{DC_SERBIAN_CYRILLIC,			"sr",	"scc",	"serbian",		"RS",	 	"Cyrl"},
	{DC_SETSWANA,					"tn",	"afr",	"afrikaans",	"ZA",		""},		// nluc_Setswana_h
	{DC_SINHALA,						"si",	"sin",	"sri lanka",	"LK",		""},		// nluc_Sinhala_h
	{DC_SLOVAK,						"sk",	"slo",	"slovak",		"SK",	 	""},		// nluc_Slovak_h
	{DC_SLOVENIAN,					"sl",	"slv",	"slovenian",	"SI",    	""},		// nluc_Sloven_h
	{DC_SPANISH_CASTILLAN,			"es",	"spa",	"spanish",		"ES",	 	"es"},	// nluc_Spanish_h
	{DC_SPANISH_MEXICAN,				"es",	"spa",	"spanish",		"MX",	 	"mx"},	// nluc_SpanishMexican_h
	{DC_SPANISH_MODERN,				"es",	"spa",	"spanish",		"",	 		""},
	{DC_SPANISH_GUATEMALA,			"es",	"spa",	"spanish",		"GT",	 	"gt"},
	{DC_SPANISH_COSTA_RICA,			"es",	"spa",	"spanish",		"CR",	 	"cr"},
	{DC_SPANISH_PANAMA,				"es",	"spa",	"spanish",		"PA",	 	"pa"},
	{DC_SPANISH_DOMINICAN_REPUBLIC,	"es",	"spa",	"spanish",		"DO",	 	"do"},
	{DC_SPANISH_VENEZUELA,			"es",	"spa",	"spanish",		"VE",	 	"ve"},
	{DC_SPANISH_COLOMBIA,			"es",	"spa",	"spanish",		"CO",	 	"co"},
	{DC_SPANISH_PERU,				"es",	"spa",	"spanish",		"PE",	 	"pe"},
	{DC_SPANISH_ARGENTINA,			"es",	"spa",	"spanish",		"AR",	 	"ar"},
	{DC_SPANISH_ECUADOR,				"es",	"spa",	"spanish",		"EC",	 	"ec"},
	{DC_SPANISH_CHILE,				"es",	"spa",	"spanish",		"CL",	 	"cl"},
	{DC_SPANISH_URUGUAY,				"es",	"spa",	"spanish",		"UY",	 	"uy"},
	{DC_SPANISH_PARAGUAY,			"es",	"spa",	"spanish",		"PY",	 	"py"},
	{DC_SPANISH_BOLIVIA,				"es",	"spa",	"spanish",		"BO",	 	"bo"},
	{DC_SPANISH_EL_SALVADOR,			"es",	"spa",	"spanish",		"SV",	 	"sv"},
	{DC_SPANISH_HONDURAS,			"es",	"spa",	"spanish",		"HN",	 	"hn"},
	{DC_SPANISH_NICARAGUA,			"es",	"spa",	"spanish",		"NI",	 	"ni"},
	{DC_SPANISH_PUERTO_RICO,			"es",	"spa",	"spanish",		"PR",	 	"pr"},
	//{DC_SOUTHERN_SOTHO,				"st",	"sot",	"afrikaans",	"ZA",		""},		// nluc_Southern_Sotho_h
	//{DC_SWAZI_SWATI,					"ss",	"ssw",	"afrikaans",	"ZA",		""},		// nluc_Swazi_Swati_h
	{DC_SWEDISH,						"sv",	"swe",	"swedish",		"SE",	 	""},		// nluc_Swedish_h
	{DC_SWEDISH_FINLAND,				"sv",	"swe",	"swedish",		"FI",	 	"fi"},
	//{DC_TAGALOG,						"tl",	"tgl",	"philippines",	"PH",		""},		// nluc_Tagalog_h
	{DC_TAMIL,						"ta",	"tam",	"indian",		"",			""},		// nluc_tamil_h
	//{DC_TETUM,						"tet",	"tet",	"indonesian",	"ID",		""},		// nluc_tetum_h
	{DC_THAI,						"th",	"tha",	"thai",			"TH",	 	""},		// nluc_Thai_h
	//{DC_TSONGA,						"ts",	"tso",	"afrikaans",	"ZA",		""},		// nluc_Tsonga_h
	{DC_TURKISH,						"tr",	"tur",	"turkish",		"TR",	 	""},
	{DC_UKRAINIAN,					"uk",	"ukr",	"ukrainian",	"UA",	 	""},		// nluc_Ukrainian_h
	{DC_URDU,						"ur",	"urd",	"pakistan",		"PK",		"pk"},	// nluc_Urdu_h
	//{DC_VENDA,						"ve",	"ven",	"afrikaans",	"ZA",		""},		// nluc_Venda_h
	{DC_VIETNAMESE,					"vi",	"vie",	"vietnamese",	"VN",	 	""},
	{DC_WELSH,						"cy",	"wel",	"english",		"GB",		"gb"},	// nluc_Welsh_h
	{DC_XHOSA,						"xh",	"xho",	"afrikaans",	"ZA",		""},		// nluc_Xhosa_h
	//{DC_YIDDISH,						"yi",	"yid",	"yiddish",		"",			""},		// nluc_Yiddish_h
	{DC_ZULU,						"zu",	"zul",	"afrikaans",	"ZA",		""}		// nlcu_Zulu_h
};




static const DialectKeyboard sDialectKeyboardList[]=
{
	{DC_AFRIKAANS, "U.S.",0,1 },
	{DC_ALBANIAN, "",0,0},
	{DC_ARABIC_SAUDI_ARABIA, "Arabic",-17920,1},
	{DC_ARABIC_IRAQ, "Arabic",-17920,1},
	{DC_ARABIC_EGYPT, "Arabic",-17920,1},
	{DC_ARABIC_LIBYA, "Arabic",-17920,1},
	{DC_ARABIC_ALGERIA, "Arabic",-17920,1},
	{DC_ARABIC_MOROCCO, "Arabic",-17920,1},
	{DC_ARABIC_TUNISIA, "Arabic",-17920,1},
	{DC_ARABIC_OMAN, "Arabic",-17920,1},
	{DC_ARABIC_YEMEN, "Arabic",-17920,1},
	{DC_ARABIC_SYRIA, "Arabic",-17920,1},
	{DC_ARABIC_JORDAN, "Arabic",-17920,1},
	{DC_ARABIC_LEBANON, "Arabic",-17920,1},
	{DC_ARABIC_KUWAIT, "Arabic",-17920,1},
	{DC_ARABIC_UAE, "Arabic",-17920,1},
	{DC_ARABIC_BAHRAIN, "Arabic",-17920,1},
	{DC_ARABIC_QATAR, "Arabic",-17920,1},
	{DC_BASQUE, "",0,0},
	{DC_BELARUSIAN, "",0,0},
	{DC_BULGARIAN, "Bulgarian",19528,1},
	{DC_CATALAN, "",0,0},
	{DC_CHINESE_TRADITIONAL, "TRADITIONAL CHINESE",16896,1},
	{DC_CHINESE_SIMPLIFIED, "SIMPLIFIED CHINESE",28672,1},
	{DC_CHINESE_HONGKONG, "TRADITIONAL CHINESE",16896,1},
	{DC_CHINESE_SINGAPORE, "TRADITIONAL CHINESE",16896,1},
	{DC_CROATIAN, "Croatian",-68,1},
	{DC_CZECH, "Czech",30776,1},
	{DC_DANISH, "Danish",9,1},
	{DC_DUTCH, "Dutch",26,1},
	{DC_DUTCH_BELGIAN, "Dutch",26,1},
	{DC_ENGLISH_US, "U.S.",0,1},
	{DC_ENGLISH_UK, "British",2,1},
	{DC_ENGLISH_AUSTRALIA, "Australian",15,1},
	{DC_ENGLISH_CANADA, "U.S.",0,1},
	{DC_ENGLISH_NEWZEALAND, "U.S.",0,1},
	{DC_ENGLISH_EIRE, "U.S.",0,1},
	{DC_ENGLISH_SOUTH_AFRICA, "U.S.",0,1},
	{DC_ENGLISH_JAMAICA, "U.S.",0,1},
	{DC_ENGLISH_CARIBBEAN, "U.S.",0,1},
	{DC_ENGLISH_BELIZE, "U.S.",0,1},
	{DC_ENGLISH_TRINIDAD, "U.S.",0,1},
	{DC_ESTONIAN, "Estonian",30764,1},
	{DC_FAEROESE, "Faroese",-47,1},
	{DC_FARSI, "Arabic",-17920,1},
	{DC_FINNISH, "Finnish",17,1},
	{DC_FRENCH, "French",1,1},
	{DC_FRENCH_BELGIAN, "Belgian",6,1},
	{DC_FRENCH_CANADIAN, "French",1,1},
	{DC_FRENCH_SWISS, "Swiss French",19,1},
	{DC_FRENCH_LUXEMBOURG, "French",1,1},
	{DC_GERMAN, "German",3,1},
	{DC_GERMAN_SWISS, "Swiss German",19,1},
	{DC_GERMAN_AUSTRIAN, "Austrian",92,1},
	{DC_GERMAN_LUXEMBOURG, "German",3,1},
	{DC_GERMAN_LIECHTENSTEIN, "German",3,1},
	{DC_GREEK, "Greek",-18944,1},
	{DC_HEBREW, "Hebrew",-18432,1},
	{DC_HUNGARIAN, "Hungarian",30763,1},
	{DC_ICELANDIC, "Icelandic",-21,1},
	{DC_INDONESIAN, "",0,0},
	{DC_ITALIAN, "Italian - Pro",223,1},
	{DC_ITALIAN_SWISS, "Italian - Pro",223,1},
	{DC_JAPANESE, "KANA",16384,1},
	{DC_KOREAN_WANSUNG, "KOREAN",17408,1},
	{DC_KOREAN_JOHAB,"KOREAN",17408,1},
	{DC_LATVIAN, "Latvian",30765,1},
	{DC_LITHUANIAN, "Lithuanian",30761,1},
	{DC_NORWEGIAN, "Norwegian",12,1},
	{DC_NORWEGIAN_NYNORSK, "Norwegian Extended",-12,1},
	{DC_POLISH, "Polish",30762,1},
	{DC_PORTUGUESE, "Portuguese",10,1},
	{DC_PORTUGUESE_BRAZILIAN, "Brazilian",71,1},
	{DC_ROMANIAN, "Romanian",-39,1},
	{DC_RUSSIAN, "Russian",19456,1},
	{DC_SERBIAN_LATIN, "Serbian-Latin",-19521,1},
	{DC_SERBIAN_CYRILLIC, "Serbian",19521,1},
	{DC_SLOVAK, "Slovak",30777,1},
	{DC_SLOVENIAN, "Slovenian",-66,1},
	{DC_SPANISH_CASTILLAN, "Spanish",8,1},
	{DC_SPANISH_MEXICAN, "Spanish",8,1},
	{DC_SPANISH_MODERN, "Spanish",8,1},
	{DC_SPANISH_GUATEMALA, "Spanish",8,1},
	{DC_SPANISH_COSTA_RICA, "Spanish",8,1},
	{DC_SPANISH_PANAMA, "Spanish",8,1},
	{DC_SPANISH_DOMINICAN_REPUBLIC, "Spanish",8,1},
	{DC_SPANISH_VENEZUELA, "Spanish",8,1},
	{DC_SPANISH_COLOMBIA, "Spanish",8,1},
	{DC_SPANISH_PERU, "Spanish",8,1},
	{DC_SPANISH_ARGENTINA, "Spanish",8,1},
	{DC_SPANISH_ECUADOR, "Spanish",8,1},
	{DC_SPANISH_CHILE, "Spanish",8,1},
	{DC_SPANISH_URUGUAY, "Spanish",8,1},
	{DC_SPANISH_PARAGUAY, "Spanish",8,1},
	{DC_SPANISH_BOLIVIA, "Spanish",8,1},
	{DC_SPANISH_EL_SALVADOR, "Spanish",8,1},
	{DC_SPANISH_HONDURAS, "Spanish",8,1},
	{DC_SPANISH_NICARAGUA, "Spanish",8,1},
	{DC_SPANISH_PUERTO_RICO, "Spanish",8,1},
	{DC_SWEDISH, "Swedish",224,1},
	{DC_SWEDISH_FINLAND, "Swedish",224,1},
	{DC_THAI, "Thai",-26624,1},
	{DC_TURKISH, "Turkish",-24,1},
	{DC_UKRAINIAN, "Ukrainian",19518,1},
	{DC_VIETNAMESE, "Vietnamese",-31232,1},
};


static void _compile_time_assertions()
{
#if USE_ICU
	assert_compile( (NormalizationMode) UNORM_NONE == NM_NONE);
	assert_compile( (NormalizationMode) UNORM_NFD == NM_NFD);
	assert_compile( (NormalizationMode) UNORM_NFKD == NM_NFKD);
	assert_compile( (NormalizationMode) UNORM_NFC == NM_NFC);
	assert_compile( (NormalizationMode) UNORM_NFKC == NM_NFKC);
	assert_compile( (NormalizationMode) UNORM_FCD == NM_FCD);
#endif
}


class VStringPredicate_paired
{
public:
	VStringPredicate_paired( VCollator *inCollator, bool inDiacritical):fCollator( inCollator), fDiacritical( inDiacritical)	{}

	template<class T>
	bool operator()( const T& inValue1, const T& inValue2)
	{
		return fCollator->CompareString( inValue1.first.GetCPointer(), inValue1.first.GetLength(), inValue2.first.GetCPointer(), inValue2.first.GetLength(), fDiacritical) == CR_SMALLER;
	}

protected:
	VCollator*	fCollator;
	bool		fDiacritical;
};


/* fast strcmp ignoring case for dialect names which are us ascii */

inline char usascii_upper( char c)
{
	return (c >= 'a' && c <= 'z') ? (c + 'A' - 'a') : c;
}


static int usascii_stricmp( const char *s1, const char *s2)
{
	while( usascii_upper( *s1) == usascii_upper( *s2++))
		if (*s1++ == 0)
			return 0;
	return *(const unsigned char *) s1 - *(const unsigned char *)(s2 - 1);
}


// Class statics
//VIntlMgr*	VIntlMgr::sDefaultMgr = NULL;
bool VIntlMgr::sICU_Available = false;

#if USE_MECAB
class IMecabModel* VIntlMgr::sMecabModel = NULL;
#endif


VIntlMgr::VIntlMgr( DialectCode inDialect, EKeyWordOptions inKeyWordOptions)
: fDialect( inDialect)
, fImpl( inDialect)
, fUseICUCollator( false)
, fKeyWordOptions( inKeyWordOptions)
, fCollator( NULL)
#if USE_ICU
, fLocale( (ICU_Available()) ? new xbox_icu::Locale( GetISO6391LanguageCode( inDialect), GetISO3166RegionCode( inDialect)) : NULL)
, fUpperStripDiacriticsTransformation( NULL)
, fLowerStripDiacriticsTransformation( NULL)
, fBreakIterator( NULL)
, fBreakLineIterator( NULL)
#endif
#if USE_MECAB
, fMecabLattice( NULL)
#endif
{
}


VIntlMgr::VIntlMgr( const VIntlMgr& inMgr)
: fUseICUCollator( inMgr.fUseICUCollator)
, fImpl( inMgr.fImpl)
, fKeyWordOptions( inMgr.fKeyWordOptions)
, fDialect( inMgr.fDialect)
, fAMString( inMgr.fAMString)
, fPMString( inMgr.fPMString)
, fUsingAmPm( inMgr.fUsingAmPm)
, fDecimalSeparator( inMgr.fDecimalSeparator)
, fThousandsSeparator( inMgr.fThousandsSeparator)
, fDateSeparator( inMgr.fDateSeparator)
, fTimeSeparator( inMgr.fTimeSeparator)
, fCurrency( inMgr.fCurrency)
, fShortDatePattern( inMgr.fShortDatePattern)
, fMediumDatePattern( inMgr.fMediumDatePattern)
, fLongDatePattern( inMgr.fLongDatePattern)
, fShortTimePattern( inMgr.fShortTimePattern)
, fMediumTimePattern( inMgr.fMediumTimePattern)
, fLongTimePattern( inMgr.fLongTimePattern)
, fDateOrder( inMgr.fDateOrder)
, fCollator( inMgr.fCollator->Clone())
#if USE_ICU
, fLocale( (inMgr.fLocale == NULL) ? NULL : inMgr.fLocale->clone() )
, fUpperStripDiacriticsTransformation( NULL)
, fLowerStripDiacriticsTransformation( NULL)
, fBreakIterator( NULL)
, fBreakLineIterator( NULL)
#endif
#if USE_MECAB
, fMecabLattice( NULL)
#endif
{
}


VIntlMgr::~VIntlMgr()
{
	ReleaseRefCountable( &fCollator);

	#if USE_ICU
	delete fLocale;
	delete fUpperStripDiacriticsTransformation;
	delete fLowerStripDiacriticsTransformation;
	if (fBreakIterator)
		delete fBreakIterator;
	if (fBreakLineIterator)
		delete fBreakLineIterator;
	#endif

	#if USE_MECAB
	if (fMecabLattice != NULL)
		_GetMecabModel()->ReleaseLattice( fMecabLattice);
	#endif
}


/*
	static
*/
VIntlMgr *VIntlMgr::Create( DialectCode inDialect, DialectCode inDialectForCollator, CollatorOptions inCollatorOptions)
{
	_InitICU();

#if VERSIONWIN && VERSIONDEBUG
	// Hitting 'U' at startup reset Unicode tables
	if (::GetAsyncKeyState('U') & 0x8000)
		::WIN_BuildUnicodeResources();
#endif

	VIntlMgr *manager = NULL;
	DialectCode dialect = ResolveDialectCode( inDialect);
	DialectCode dialectForCollator = ResolveDialectCode( inDialectForCollator);
	
	// optim: see if one can simply clone the current thread one which is way faster
	VIntlMgr *currentTaskManager = VTask::GetCurrentIntlManager();
	if ( (currentTaskManager != NULL)
		&& (currentTaskManager->GetDialectCode() == dialect)
		&& (currentTaskManager->GetCollator()->GetDialectCode() == dialectForCollator)
		&& (currentTaskManager->GetCollator()->GetOptions() == inCollatorOptions) )
	{
		manager = currentTaskManager->Clone();

		// restore non const settings
		if (manager != NULL)
			manager->Reset();
	}

	if (manager == NULL)
	{
		EKeyWordOptions keyWordOptions = 0;
		
		if (inCollatorOptions & COL_ConsiderOnlyDeadCharsForKeywords)
		{
			keyWordOptions = eKW_ConsiderOnlyDeadChars;
		}
		else
		{
			#if USE_MECAB
			if (dialectForCollator == DC_JAPANESE)
				keyWordOptions = eKW_UseMeCab;
			else
			#endif
			if (ICU_Available())
				keyWordOptions = eKW_UseICU;
			else
				keyWordOptions = eKW_ConsiderOnlyDeadChars;
		}

		manager = new VIntlMgr( dialect, keyWordOptions);
		if (manager != NULL)
		{
			bool ok = true;
			
			#if USE_ICU
            if (ICU_Available())
                ok = (manager->fLocale != NULL) && !manager->fLocale->isBogus();
            
			#endif
			
			if (ok)
				ok = manager->_InitCollator( dialectForCollator, inCollatorOptions);

			if (ok)
				manager->_InitLocaleVariables();

			if (!ok)
			{
				delete manager;
				manager = NULL;
			}
		}
	}
	
	return manager;
}


VIntlMgr* VIntlMgr::Clone()
{
	return new VIntlMgr(*this);
}


void VIntlMgr::Reset()
{
	GetCollator()->Reset();
}


void VIntlMgr::FormatDate( const VTime& inDate, VString& outDate, EOSFormats inFormat, bool inUseGMTTimeZoneForDisplay)
{
    GetImpl().FormatDate( inDate, outDate, inFormat, inUseGMTTimeZoneForDisplay);
}


void VIntlMgr::FormatTime( const VTime& inTime, VString& outTime, EOSFormats inFormat, bool inUseGMTTimeZoneForDisplay)
{
    GetImpl().FormatTime(inTime, outTime, inFormat, inUseGMTTimeZoneForDisplay);
}


void VIntlMgr::FormatDuration( const VDuration& inTime, VString& outTime, EOSFormats inFormat)
{
    GetImpl().FormatDuration(inTime, outTime, inFormat);
}


static sLONG ConsumeSameUniChar( const UniChar*& inString, sLONG inMax)
{
	sLONG count = 1;
	UniChar c = *inString;
	while( (inString[1] == c) && (count < inMax) )
	{
		++inString;
		++count;
	}
	return count;
}


void VIntlMgr::FormatDateTimeWithPattern( const VTime& inTime, const XBOX::VString& inPattern, bool inUseUTC, const XBOX::VString *inMonthName, XBOX::VString& outString)
{
	VString result;
	VString s;

	sWORD year, month, day, hour, minute, second, millisecond;
	if (inUseUTC)
	{
		inTime.GetUTCTime( year, month, day, hour, minute, second, millisecond);
	}
	else
	{
		inTime.GetLocalTime( year, month, day, hour, minute, second, millisecond);
	}

	const UniChar *begin = inPattern.GetCPointer();
	const UniChar *end = begin + inPattern.GetLength();
	for( const UniChar *p = begin ; p != end ; ++p)
	{
		sLONG count;
		switch( *p)
		{
			case 'G':	// era
				{
					count = ConsumeSameUniChar( p, 5);
					switch( count)
					{
						case 1:
						case 2:
						case 3:
							result += CVSTR( "AD");	// BC non supporte par VTime
							break;

						case 4:
							result += CVSTR( "Anno Domini");
							break;
						
						default:
							result += 'A';
							break;
					}
					break;
				}

			case 'y':	// YEAR
			case 'Y':	// UNSUPPORTED: Year (of "Week of Year"), used in ISO year-week calendar. May differ from calendar year.
				{
					count = ConsumeSameUniChar( p, 999);
					switch( count)
					{
						case 1:
							result.AppendLong( year);
							break;
						
						default:
							result.AppendPrintf( "%0*d", count, year);
							break;
					}
					break;
				}

			case 'M':	// MONTH
			case 'L':	// Stand-alone Month
				{
					count = ConsumeSameUniChar( p, 5);
					switch( count)
					{
						case 1:
						case 2:
							result.Printf( "%0*d", count, month);
							break;
						
						case 3:
							if (inMonthName != NULL)
								s = *inMonthName;
							else
								GetMonthName( month, true, s);
							result += s;
							break;
						
						case 4:
							if (inMonthName != NULL)
								s = *inMonthName;
							else
								GetMonthName( month, false, s);
							result += s;
							break;
						
						default:
							if (inMonthName != NULL)
								s = *inMonthName;
							else
								GetMonthName( month, false, s);
							result += s[0];
							break;
					}
					break;
				}

			case 'd':	// day of the month
				{
					count = ConsumeSameUniChar( p, 2);
					result.AppendPrintf( "%0*d", count, day);
					break;
				}
			
			case 'D':	// Day of year
				{
					count = ConsumeSameUniChar( p, 3);
					result.AppendPrintf( "%0*d", count, VTime::ComputeYearDay( year, month, day));
					break;
				}
			
			case 'g':	// Modified Julian day
				{
					count = ConsumeSameUniChar( p, 999);
					result.AppendPrintf( "%0*d", count, VTime::ComputeJulianDay( year, month, day));
					break;
				}
			
			case 'E':	// Day of week
				{
					sLONG weekDay = XBOX::VTime::ComputeWeekDay( year, month, day);
					count = ConsumeSameUniChar( p, 6);
					switch( count)
					{
						case 1:
						case 2:
						case 3:
							GetWeekDayName( weekDay, true, s);
							result += s;
							break;
						
						case 4:
							GetWeekDayName( weekDay, false, s);
							result += s;
							break;
						
						case 5:
							GetWeekDayName( month, false, s);
							result += s[0];
							break;

						case 6:
							GetWeekDayName( month, false, s);
							result += s[0];
							result += s[1];
							break;
					}
					break;
				}
			
			case 'a':
				{
					if (hour < 12)
						result += CVSTR( "AM");
					else
						result += CVSTR( "PM");
					break;
				}
			
			case 'h':
				{
					count = ConsumeSameUniChar( p, 2);
					sLONG h = hour % 12;
					if (h == 0)
						h = 12;
					result.AppendPrintf( "%0*d", count, h);
					break;
				}
			
			case 'H':
				{
					count = ConsumeSameUniChar( p, 2);
					result.AppendPrintf( "%0*d", count, hour);
					break;
				}
			
			case 'K':
				{
					count = ConsumeSameUniChar( p, 2);
					result.AppendPrintf( "%0*d", count, hour % 12);
					break;
				}
			
			case 'k':
				{
					count = ConsumeSameUniChar( p, 2);
					result.AppendPrintf( "%0*d", count, (hour == 0) ? 24 : hour);
					break;
				}
			
			case 'm':
				{
					count = ConsumeSameUniChar( p, 2);
					result.AppendPrintf( "%0*d", count, minute);
					break;
				}

			case 's':
				{
					count = ConsumeSameUniChar( p, 2);
					result.AppendPrintf( "%0*d", count, second);
					break;
				}

			case 'u':	// UNSUPPORTED: Extended year. This is a single number designating the year of this calendar system, encompassing all supra-year fields.
			case 'Q':	// UNSUPPORTED: Quarter
			case 'q':	// UNSUPPORTED: Stand-Alone Quarter 
			case 'w':	// UNSUPPORTED: Week of Year
			case 'W':	// UNSUPPORTED: Week of Month
			case 'F':	// UNSUPPORTED: Day of Week in Month. The example is for the 2nd Wed in July
			case 'e':	// UNSUPPORTED: Local day of week.
			case 'c':	// UNSUPPORTED: Stand-alone Local day of week.
			case 'S':	// UNSUPPORTED: Fractional Second
			case 'A':	// UNSUPPORTED: Milliseconds in day.
			case 'Z':	// UNSUPPORTED: Time Zone - RFC 822 GMT format
			case 'z':	// UNSUPPORTED: Time Zone - with the specific non-location format.
			case 'V':	// UNSUPPORTED: Time Zone - Identical to the format for z.
			case 'v':	// UNSUPPORTED: Time Zone - with the generic non-location format.
				break;

			default:
				result += *p;
				break;

		}
	}
	outString = result;
}



void VIntlMgr::GetMonthName( sLONG inIndex, bool inAbbreviated, VString& outName) const
{
	if (testAssert( inIndex >= 1 && inIndex <= 12))
		GetImpl().GetMonthName( inIndex, inAbbreviated, outName);
	else
		outName.Clear();
}


void VIntlMgr::GetWeekDayName( sLONG inIndex, bool inAbbreviated, VString& outName) const
{
	if (testAssert( inIndex >= 0 && inIndex <= 6))
		GetImpl().GetWeekDayName( inIndex, inAbbreviated, outName);
	else
		outName.Clear();
}


/*
	static
*/
#if USE_ICU
#if VERSIONMAC
extern void __attribute__ ((weak)) u_init( UErrorCode *);
#endif
#endif

void VIntlMgr::_InitICU()
{
	static	bool sInited = false;
	
	#if USE_ICU
	if (!sInited)
	{
		#if VERSIONMAC
        if (&u_init != NULL)
        {
			sICU_Available = true;
		}
        else
        {
			sICU_Available = false;
		}
        
        //sICU_Available = (u_init!=NULL);
        #elif VERSION_LINUX
        sICU_Available=true;    // We link against ICU on Linux. It's always available.
		#elif VERSIONWIN
		try
		{
			// pp l'appel de u_init sert juste a charger la dll, avec icu 3.6 cet appel n'est plus utile
			UErrorCode success = U_ZERO_ERROR;
			u_init(&success);
			sICU_Available = (success==U_ZERO_ERROR);
		}
		catch(...)
		{
			sICU_Available = false;
		}
		#endif
		sInited = true;
	}
	#endif
}


/*
	static
*/
void VIntlMgr::_InitMeCab()
{
	#if USE_MECAB
	static	bool sInited = false;
	static	VLibrary*	sMecabLibrary = NULL;
	static	VSystemCriticalSection	sMecabMutex;
	
	sMecabMutex.Lock();
	if (!sInited)
	{
		VFolder *resourcesFolder = VProcess::Get()->RetainFolder( VProcess::eFS_Resources);
		VFilePath path( resourcesFolder->GetPath());
		#if VERSIONWIN
		path.ToSubFolder( CVSTR( "MeCab")).ToSubFile( CVSTR( "mecab.dll"));
		#elif VERSIONMAC
		path.ToSubFolder( CVSTR( "MeCab")).ToSubFolder( CVSTR( "mecab.bundle"));
		#endif
		sMecabLibrary = new VLibrary( path);
		if (sMecabLibrary->Load())
		{
			CreateMecabModelProc proc = (CreateMecabModelProc) sMecabLibrary->GetProcAddress( CVSTR( "CreateMecabModel"));
			if (testAssert( proc != NULL))
			{
				sMecabModel = (*proc)();
				xbox_assert( sMecabModel != NULL);
			}
		}
		ReleaseRefCountable( &resourcesFolder);
		sInited = true;
	}
	sMecabMutex.Unlock();
	#endif
}


/*
	static
*/
IMecabModel *VIntlMgr::_GetMecabModel()
{
	_InitMeCab();
#if USE_MECAB
	return sMecabModel;
#else
	return NULL;
#endif
}


bool VIntlMgr::_InitLocaleVariables()
{
	bool ok = true;

#if VERSIONMAC
	CFLocaleRef userLocale = MAC_RetainCFLocale();
	if (userLocale)
	{
		// Decimal separator
		CFStringRef value=(CFStringRef)::CFLocaleGetValue(userLocale,kCFLocaleDecimalSeparator);
		fDecimalSeparator.MAC_FromCFString(value);
		
		// Thousands separator
		value=(CFStringRef)::CFLocaleGetValue(userLocale,kCFLocaleGroupingSeparator);
		fThousandsSeparator.MAC_FromCFString(value);
		
		// Currency symbol
		value=(CFStringRef)::CFLocaleGetValue(userLocale,kCFLocaleCurrencySymbol);
		fCurrency.MAC_FromCFString(value);
		if ((fCurrency.GetLength()==1) && (fCurrency[0]==0xa4))
			fCurrency=(UniChar)0x20ac;

		// Values for time variables
		CFDateFormatterRef p_TimeFormatter=::CFDateFormatterCreate(NULL,userLocale,kCFDateFormatterNoStyle,kCFDateFormatterShortStyle);
		if (p_TimeFormatter)
		{
			// AM String
			CFStringRef p_value=(CFStringRef)::CFDateFormatterCopyProperty(p_TimeFormatter,kCFDateFormatterAMSymbol);
			fAMString.MAC_FromCFString(p_value);
			::CFRelease(p_value);
			
			// PM String
			p_value=(CFStringRef)::CFDateFormatterCopyProperty(p_TimeFormatter,kCFDateFormatterPMSymbol);
			fPMString.MAC_FromCFString(p_value);
			::CFRelease(p_value);
			
			// Time format
			p_value=(CFStringRef)::CFDateFormatterCopyProperty(p_TimeFormatter,kCFDateFormatterDefaultFormat);
			VString vFormat;
			vFormat.MAC_FromCFString(p_value);
			::CFRelease(p_value);

			// Get the time separator and am/pm infos.
			fUsingAmPm = false;
			for (int i=0;i<vFormat.GetLength();i++)
			{
				UniChar c=vFormat[i];
				switch(c)
				{
				case '\'':	// Quoted text
				case '"':
					// Skip the text
					do {
						i++;
						if (vFormat[i]==c)
							break;
					} while (i<vFormat.GetLength());
					break;
					
				case 'a': // am/pm marker
					fUsingAmPm = true;
					break;
					
				default:
					if (fTimeSeparator.IsEmpty() && (((c<'A') || (c>'z')) || ((c>'Z') && (c<'a'))))
					{
						fTimeSeparator=c;
					}
				}
			}

			::CFRelease(p_TimeFormatter);
		}

		// Values for date variables
		CFDateFormatterRef p_DateFormatter=::CFDateFormatterCreate(NULL,userLocale,kCFDateFormatterShortStyle,kCFDateFormatterNoStyle);
		if (p_DateFormatter)
		{
			// Date format
			CFStringRef p_value=(CFStringRef)::CFDateFormatterCopyProperty(p_DateFormatter,kCFDateFormatterDefaultFormat);
			VString vFormat;
			vFormat.MAC_FromCFString(p_value);
			::CFRelease(p_value);

			// Get the time separator and dmy order infos.
			VString vOrder;
			for (int i=0;i<vFormat.GetLength();i++)
			{
				UniChar c=vFormat[i];
				switch(c)
				{
				case '\'':	// Quoted text
				case '"':
					// Skip the text
					do {
						i++;
						if (vFormat[i]==c)
							break;
					} while (i<vFormat.GetLength());
					break;
					
				case 'y': // year
				case 'M': // month
				case 'd': // day
					if (vOrder.FindUniChar(c)<=0)
						vOrder.AppendUniChar(c);
					break;
					
				default:
					if (fDateSeparator.IsEmpty() && (((c<'A') || (c>'z')) || ((c>'Z') && (c<'a'))))
					{
						fDateSeparator=c;
					}
				}
			}

			// on ne peut utiliser VString::Find car cela utilise le VIntlMgr qu'on est en train de construire...
			if (vOrder.EqualToUSASCIICString( "Mdy"))
				fDateOrder=DO_MONTH_DAY_YEAR;
			else if (vOrder.EqualToUSASCIICString( "dMy"))
				fDateOrder=DO_DAY_MONTH_YEAR;
			else if (vOrder.EqualToUSASCIICString( "yMd"))
				fDateOrder=DO_YEAR_MONTH_DAY;
			else if (vOrder.EqualToUSASCIICString( "Myd"))
				fDateOrder=DO_MONTH_YEAR_DAY;
			else if (vOrder.EqualToUSASCIICString( "dyM"))
				fDateOrder=DO_DAY_YEAR_MONTH;
			else if (vOrder.EqualToUSASCIICString( "ydM"))
				fDateOrder=DO_YEAR_DAY_MONTH;
			else
				fDateOrder=DO_MONTH_DAY_YEAR;
			
			::CFRelease(p_DateFormatter);
		}

		// Init for date & time patterns
		typedef struct {
			CFDateFormatterStyle DateFormat;
			CFDateFormatterStyle TimeFormat;
			VString* pStringToInit;
		} DateTimeFormatInitializer;
		DateTimeFormatInitializer initializer[]={
			{kCFDateFormatterNoStyle,kCFDateFormatterShortStyle,&fShortTimePattern},
			{kCFDateFormatterNoStyle,kCFDateFormatterMediumStyle,&fMediumTimePattern},
			{kCFDateFormatterNoStyle,kCFDateFormatterLongStyle,&fLongTimePattern},
			{kCFDateFormatterShortStyle,kCFDateFormatterNoStyle,&fShortDatePattern},
			{kCFDateFormatterMediumStyle,kCFDateFormatterNoStyle,&fMediumDatePattern},
			{kCFDateFormatterLongStyle,kCFDateFormatterNoStyle,&fLongDatePattern}
		};
		// Date and time patterns loop.
		for (int i=0; i<(sizeof(initializer)/sizeof(DateTimeFormatInitializer));i++)
		{
			CFDateFormatterRef p_DateTimeFormatter=::CFDateFormatterCreate(NULL,userLocale,initializer[i].DateFormat,initializer[i].TimeFormat);
			if (p_DateTimeFormatter)
			{
				CFStringRef p_value=(CFStringRef)::CFDateFormatterCopyProperty(p_DateTimeFormatter,kCFDateFormatterDefaultFormat);
				initializer[i].pStringToInit->MAC_FromCFString(p_value);
				::CFRelease(p_value);
				::CFRelease(p_DateTimeFormatter);
			}
		}
		
		::CFRelease(userLocale);
	}

#elif VERSION_LINUX

    xbox_assert(fLocale!=NULL);

    if(fLocale)
        GetImpl().Init(*fLocale);

    fDecimalSeparator   = GetImpl().GetDecimalSeparator();
    fThousandsSeparator = GetImpl().GetThousandSeparator();
    fDateSeparator      = GetImpl().GetDateSeparator();
    fTimeSeparator      = GetImpl().GetTimeSeparator();
    fCurrency           = GetImpl().GetCurrency();
    fLongDatePattern    = GetImpl().GetLongDatePattern();
    fShortDatePattern   = GetImpl().GetShortDatePattern();
    fDateOrder          = GetImpl().GetDateOrder();
	fLongTimePattern    = GetImpl().GetLongTimePattern();
	fMediumTimePattern  = GetImpl().GetMediumTimePattern();
	fShortTimePattern   = GetImpl().GetShortTimePattern();
    fAMString           = GetImpl().GetAMString();
    fPMString           = GetImpl().GetPMString();
    fUsingAmPm          = GetImpl().UseAmPm();

#elif VERSIONWIN
	
	GetImpl().GetLocaleInfo( LOCALE_SDECIMAL, fDecimalSeparator);
	GetImpl().GetLocaleInfo( LOCALE_STHOUSAND, fThousandsSeparator);
	GetImpl().GetLocaleInfo( LOCALE_SDATE, fDateSeparator);
	GetImpl().GetLocaleInfo( LOCALE_STIME, fTimeSeparator);
	GetImpl().GetLocaleInfo( LOCALE_SCURRENCY, fCurrency);
	GetImpl().GetLocaleInfo( LOCALE_SLONGDATE, fLongDatePattern);
	GetImpl().GetLocaleInfo( LOCALE_SSHORTDATE, fShortDatePattern);
	GetImpl().GetLocaleInfo( LOCALE_STIMEFORMAT, fLongTimePattern);
	GetImpl().GetLocaleInfo( LOCALE_SDATE, fDateSeparator);
	GetImpl().GetLocaleInfo( LOCALE_S1159, fAMString);
	GetImpl().GetLocaleInfo( LOCALE_S2359, fPMString);
	
	fMediumDatePattern = fShortDatePattern;
	fMediumTimePattern = fLongTimePattern;
	fShortTimePattern = fLongTimePattern;
	
	VString s;
	GetImpl().GetLocaleInfo( LOCALE_IDATE, s);
	switch(s[0])
	{
		case '0':
			fDateOrder = DO_MONTH_DAY_YEAR;
			break;
		case '1':
			fDateOrder = DO_DAY_MONTH_YEAR;
			break;
		case '2':
			fDateOrder = DO_YEAR_MONTH_DAY;
			break;
		default:
			// Window returns more info than it was.
			xbox_assert(false);
			break;
	}

	
	GetImpl().GetLocaleInfo( LOCALE_ITIME, s);
	fUsingAmPm = (s[0] == '0');

#endif

	return ok;
}


CompareResult VIntlMgr::CompareString_Like(const UniChar* inText1, sLONG inSize1, const UniChar* inText2, sLONG inSize2, bool inWithDiacritics)
{
	return fCollator->CompareString_Like(inText1, inSize1, inText2, inSize2, inWithDiacritics);
}


CompareResult VIntlMgr::CompareString(const UniChar* inText1, sLONG inSize1, const UniChar* inText2, sLONG inSize2, bool inWithDiacritics)
{
	return fCollator->CompareString( inText1, inSize1, inText2, inSize2, inWithDiacritics);
}


CompareResult VIntlMgr::CompareString (const UniChar* inText1, sLONG inSize1, const UniChar* inText2, sLONG inSize2, const VCompareOptions& inOptions)
{
	if (inOptions.IsLike())
		return fCollator->CompareString_Like(inText1, inSize1, inText2, inSize2, inOptions.IsDiacritical());
	else
		return fCollator->CompareString(inText1, inSize1, inText2, inSize2, inOptions.IsDiacritical());
}


bool VIntlMgr::EqualString(const UniChar* inText1, sLONG inSize1, const UniChar* inText2, sLONG inSize2, bool inWithDiacritics)
{
	return fCollator->EqualString(inText1, inSize1, inText2, inSize2, inWithDiacritics);
}


bool VIntlMgr::EqualString_Like(const UniChar* inText1, sLONG inSize1, const UniChar* inText2, sLONG inSize2, bool inWithDiacritics)
{
	return fCollator->EqualString_Like(inText1, inSize1, inText2, inSize2, inWithDiacritics);
}


/*
	static
*/
DialectCode VIntlMgr::ResolveDialectCode( DialectCode inDialect)
{
	DialectCode	adjustedDialect = (inDialect == DC_NONE) ? DC_USER : inDialect;
	
	if (adjustedDialect == DC_USER || adjustedDialect == DC_SYSTEM)
	{
	#if VERSIONWIN
		LCID	lcid = (adjustedDialect == DC_USER) ? ::GetUserDefaultLCID() : ::GetSystemDefaultLCID();
		adjustedDialect = (DialectCode) LANGIDFROMLCID(lcid);
	#elif VERSIONMAC
		// ne pas prendre le locale System avec CFLocaleGetSystem car il est bidon (pas de langue associee)
		static DialectCode sCurrentDialect = DC_NONE;
		
		if (sCurrentDialect == DC_NONE)
		{
			
			CFLocaleRef cfLocale = ::CFLocaleCopyCurrent();
			
			CFStringRef cfLocalID = (CFStringRef) ::CFLocaleGetValue( cfLocale, kCFLocaleIdentifier);
			cfLocalID = CFLocaleCreateCanonicalLanguageIdentifierFromString(NULL,cfLocalID);
			
			char language[80];
			language[0] = 0;
			if (testAssert( cfLocalID != NULL))
				::CFStringGetCString( cfLocalID, language, sizeof( language), kCFStringEncodingUTF8);

			bool found = GetDialectCodeWithRFC3066BisLanguageCode( language, sCurrentDialect, true);
			if (!testAssert( found))
				sCurrentDialect = DC_ENGLISH_US;
			
			::CFRelease( cfLocalID);
			::CFRelease( cfLocale);
		}
		adjustedDialect = sCurrentDialect;

    #elif VERSION_LINUX

        //jmo - en principe on connait déjà la locale avec ICU
        adjustedDialect=DC_ENGLISH_US;  // Postponed Linux Implementation !

	#endif
	}
	
	return adjustedDialect;
}

DialectCode VIntlMgr::GetSystemUIDialectCode()
{
	DialectCode result=DC_ENGLISH_US;
	
	#if VERSIONWIN
	result = (DialectCode) GetUserDefaultUILanguage();
	
	#elif VERSIONMAC
		
	CFPropertyListRef pref;
	
	pref=CFPreferencesCopyValue(CFSTR ("AppleLanguages"),kCFPreferencesAnyApplication,kCFPreferencesCurrentUser,kCFPreferencesAnyHost);
	if(pref)
	{
		CFStringRef strref;
		strref=(CFStringRef)CFArrayGetValueAtIndex((CFArrayRef)pref,0);
		if(strref)
		{
			XBOX::VString vstr;
			strref = CFLocaleCreateCanonicalLanguageIdentifierFromString(NULL,strref);
			vstr.MAC_FromCFString(strref);
			bool found = VIntlMgr::GetDialectCodeWithRFC3066BisLanguageCode( vstr, result, true);
			if(!found)
			{
				result=DC_ENGLISH_US;
			}
			CFRelease(strref);
		}
		CFRelease(pref);
	}
    
    #elif VERSION_LINUX
	bool UIDialectCodeFixedToEnglishUs=false;
	xbox_assert(UIDialectCodeFixedToEnglishUs); // Postponed Linux Implementation !

	#endif
	
	return result;
}
bool VIntlMgr::IsAlpha(UniChar inChar) const
{
	#if USE_ICU
	if (ICU_Available())
		return u_isalpha(inChar) != 0;
	#endif

	return _CharTest(inChar, (UniChar*)sUniAlphaRanges);
}


bool VIntlMgr::IsDigit(UniChar inChar) const
{
	#if USE_ICU
	if (ICU_Available())
		return u_isdigit(inChar) != 0;
	#endif

	return _CharTest(inChar, (UniChar*)sUniDigitRanges);
}


sLONG VIntlMgr::CharDigitValue( UniChar inChar) const
{
	#if USE_ICU
	if (ICU_Available())
		return u_charDigitValue( inChar);
	#endif
	
	return (inChar >= '0') && (inChar <= '9') ? (inChar - '0') : -1;
}


uBYTE VIntlMgr::CharType( uLONG inCodePoint)
{
	#if USE_ICU
	if (ICU_Available())
		return u_charType( inCodePoint);
	#endif
	xbox_assert( false);
	return 0;
}


bool VIntlMgr::IsPunctuation(UniChar inChar) const
{
	#if USE_ICU
	if (ICU_Available())
		return u_ispunct(inChar) != 0;	
	#endif

	return _CharTest(inChar, (UniChar*)sUniPunctRanges);
}


bool VIntlMgr::IsSpace(UniChar inChar, bool inExcludingNoBreakSpace) const
{
	#if USE_ICU
	if (ICU_Available())
		return inExcludingNoBreakSpace ? u_isWhitespace(inChar) != 0 : u_isUWhiteSpace(inChar) != 0;	
	#endif

	return _CharTest(inChar, (UniChar*)sUniSpaceRanges);
}


UniChar* VIntlMgr::_FillOneRangesArray(UniChar* inArray, uBYTE inMatchFlag)
{
	const uWORD		*inArray0;
	UniChar			rBegin, rEnd;
	UniChar			*tempArray, *rPtr;
	UniChar			*result = 0;
	sLONG			count, sum;
	sLONG			i;

	inArray0 = inArray;
	rBegin = rEnd = 0x0000;
	count = 0;
	sum = 0;

	tempArray = rPtr = (UniChar*)VMemory::NewPtr((2 + 65536) * sizeof(UniChar), 'strU');
	rPtr += 2;
	for (i = 0;i <= 0xffff;i++)
		rPtr[i] = 0x0000;
	
	for (i = 0x0000;i <= 0xffff;i++)
	{
		if (*inArray & inMatchFlag)
		{
			if (rBegin == 0x0000)
				rBegin = (UniChar)i;
			rEnd = (UniChar)i;
		}
		else
		{
			if (rBegin != 0x0000)
			{
				*rPtr++ = rBegin;
				*rPtr++ = rEnd;
				count++;
				sum += rBegin;
				rBegin = rEnd = 0x0000;
			}
		}
		inArray++;
	}
	
	tempArray[0] = (UniChar) count;
	tempArray[1] = (UniChar) (count ? (sum / count) : 0);

	result = (UniChar*)VMemory::NewPtr(((1 + count) * 2) * sizeof(UniChar), 'strU');
	::CopyBlock(tempArray, result, ((1 + count) * 2) * sizeof(UniChar));
	
	VMemory::DisposePtr((VPtr)tempArray);

	return result;
}


bool VIntlMgr::_InitCollator( DialectCode inDialectForCollator, CollatorOptions inCollatorOptions)
{
	xbox_assert( (fCollator == NULL) && !fUseICUCollator);

	#if USE_ICU
	if (inCollatorOptions & COL_ICU)
	{
		fCollator = VICUCollator::Create( inDialectForCollator, (fDialect == inDialectForCollator) ? fLocale : NULL, inCollatorOptions);
		if (fCollator != NULL)
		{
			fUseICUCollator = true;
			//DebugMsg( "**** USING ICU ****\r\n");
		}
	}
	#endif

    #if !VERSION_LINUX
	if (fCollator == NULL)
	{
		fCollator = VCollator_system::Create( inDialectForCollator, (fDialect == inDialectForCollator) ? this : NULL);
		DebugMsg( "**** USING SYSTEM SORT TABLE ****\r\n");
	}
    #endif

	return (fCollator != NULL);
}


void VIntlMgr::_FillRangesArrays()
{
	UniChar		*s;
	uBYTE		*flagsArray;
	sLONG		i;

	flagsArray = (uBYTE*)VMemory::NewPtr(65536, 'intl');
	
	for (i = 0x0000;i <= 0xffff;i++)
		flagsArray[i] = 0;

	for (s = (UniChar*)sUniAlphaRanges;*s;s++)
	{
		flagsArray[*s] |= kIsAlphaRange;
	}
	for (s = (UniChar*)sUniDigitRanges;*s;s++)
	{
		flagsArray[*s] |= kIsDigitRange;
	}
	for (s = (UniChar*)sUniSpaceRanges;*s;s++)
	{
		flagsArray[*s] |= kIsSpaceRange;
	}
	for (s = (UniChar*)sUniPunctRanges;*s;s++)
	{
		flagsArray[*s] |= kIsPunctRange;
	}

	VMemory::DisposePtr((VPtr)flagsArray);
}


bool VIntlMgr::_CharTest(UniChar inChar, const UniChar* inRangesArray) const
{
	bool	result = false;
	sLONG	count;
	UniChar	middle;
	UniChar	lastupper = 0x0000;
	UniChar	lastlower = 0xffff;

	count = inRangesArray[0];
	middle = inRangesArray[1];

	inRangesArray += 2;
	
	if (inChar <= middle)
	{
		while (--count >= 0 && !result)
		{
			if (inChar > lastupper && inChar < inRangesArray[0])
				break; // lors intervalle

			if (inChar >= inRangesArray[0] && inChar <= inRangesArray[1])
				result = true;
			lastupper = inRangesArray[1];
			inRangesArray += 2;
		}
	}
	else
	{
		inRangesArray += (count * 2);
		while (--count >= 0 && !result)
		{
			inRangesArray -= 2;

			if (inChar > inRangesArray[1] && inChar < lastlower)
				break; // lors intervalle
			
			if (inChar >= inRangesArray[0] && inChar <= inRangesArray[1])
				result = true;
			lastlower = inRangesArray[0];
		}
	}

	return result;
}


void VIntlMgr::ToUpperLowerCase( VString& ioText, bool inStripDiac, bool inIsUpper)
{
	#if USE_ICU
	if (ICU_Available())
	{
		UniChar *p = ioText.GetCPointerForWrite();
		if (p != NULL)
		{
			UErrorCode status = U_ZERO_ERROR;
			xbox_icu::UnicodeString string( p, ioText.GetLength(), ioText.GetEnsuredSize());

			if (inIsUpper)
			{
				if (inStripDiac)
				{
					if (fUpperStripDiacriticsTransformation == NULL)
						fUpperStripDiacriticsTransformation = xbox_icu::Transliterator::createInstance( "any-NFD; [:nonspacing mark:] any-remove; Upper; any-NFC", UTRANS_FORWARD, status);
					if (fUpperStripDiacriticsTransformation != NULL)
					{
						fUpperStripDiacriticsTransformation->transliterate( string);
					}
					else
					{
						string.toUpper( *fLocale);
					}
				}
				else
				{
					string.toUpper( *fLocale);
				}
			}
			else
			{
				if (inStripDiac)
				{
					if (fLowerStripDiacriticsTransformation == NULL)
						fLowerStripDiacriticsTransformation = xbox_icu::Transliterator::createInstance( "any-NFD; [:nonspacing mark:] any-remove; Lower; any-NFC", UTRANS_FORWARD, status);
					if (fLowerStripDiacriticsTransformation != NULL)
					{
						fLowerStripDiacriticsTransformation->transliterate( string);
					}
					else
					{
						string.toLower( *fLocale);
					}
				}
				else
				{
					string.toLower( *fLocale);
				}
			}
			
			// call GetCPointerForWrite again with exact length 
			p = ioText.GetCPointerForWrite( string.length());
			if (p != NULL)
			{
				ioText.Validate( string.extract( p, ioText.GetEnsuredSize(), status));
			}
		}

		return;
	}
	#endif
	
    #if !VERSION_LINUX

	UniChar	buffer[512L];
	UniChar *pBuffer = NULL;

	if (ioText.GetLength() == 0)
		return;
	
	UniPtr dest = NULL;
	sLONG size = 0;
	Boolean isOK = false;
	if (sizeof(buffer) / sizeof(UniChar) > 2*ioText.GetLength())
	{
		dest = buffer;
		size = sizeof(buffer) / sizeof(UniChar);
		isOK = fImpl.ToUpperLowerCase(ioText.GetCPointer(), ioText.GetLength(), dest, size, inIsUpper);
	}
	if (!isOK) {
		// on essaie avec un + gros buffer
		pBuffer = (UniChar *) VMemory::NewPtr(ioText.GetLength()*4, 'tupc');
		if (pBuffer != NULL)
		{
			size = (sLONG) (ioText.GetLength()*4 / sizeof(UniChar));
			dest = pBuffer;
			isOK = fImpl.ToUpperLowerCase(ioText.GetCPointer(), ioText.GetLength(), dest, size, inIsUpper);
		}
	}
	
	if (isOK)
		ioText.FromBlock(dest, size * sizeof(UniChar), VTC_UTF_16);
	
	VMemory::DisposePtr(pBuffer);
    #endif
}


void VIntlMgr::NormalizeString( VString& ioString, NormalizationMode inMode)
{
	#if USE_ICU
	if (ICU_Available())
	{
		UNormalizationMode mode = (UNormalizationMode) inMode;

		// do a quick check to know if that's eally necessary
		UErrorCode status = U_ZERO_ERROR;
		if (unorm_quickCheck( ioString.GetCPointer(), ioString.GetLength(), mode, &status) != UNORM_YES)
		{
			int32_t length = unorm_normalize( ioString.GetCPointer(), ioString.GetLength(), mode, 0, NULL, 0, &status);
			if (status == U_BUFFER_OVERFLOW_ERROR)
			{
				VString temp;
				UniChar *p = temp.GetCPointerForWrite( length);
				if (p != NULL)
				{
					status = U_ZERO_ERROR;
					length = unorm_normalize( ioString.GetCPointer(), ioString.GetLength(), mode, 0, p, temp.GetEnsuredSize(), &status);

					if (testAssert( U_SUCCESS( status) && (length <= temp.GetEnsuredSize()) ))
					{
						temp.Validate( length);
						ioString.FromString( temp);
					}
				}
			}
		}
	}
	#endif
}


bool VIntlMgr::_IsItaDeadChar( UniChar c)
{
 	if (c<32)
		return true;
	else
		return !(IsAlpha(c) || IsDigit(c));
}


bool VIntlMgr::_HasApostropheSecondFollowedByVowel( const VString& inText, VIndex inStart, VIndex inLength)
{
	bool found = false;
	if (inLength > 3 && (inText[inStart] == CHAR_APOSTROPHE || inText[inStart] == CHAR_RIGHT_SINGLE_QUOTATION_MARK) )
	{
		VString temp;
		inText.GetSubString( inStart, inLength, temp);
		ToUpperLowerCase( temp, true, false);	// lowercas, strip diacritics for easy vowel testing
		xbox_assert( temp[1] == CHAR_APOSTROPHE || temp[1] == CHAR_RIGHT_SINGLE_QUOTATION_MARK);
		xbox_assert( temp.GetLength() > 3);
		UniChar vowel = temp[2];
		if ( vowel == 'a' || vowel == 'e' || vowel == 'i' || vowel == 'o' || vowel == 'u' || vowel == 'y' || vowel == 'h')
		{
			found = true;
		}
	}
	
	return found;
}


#if USE_ICU
xbox_icu::Locale* VIntlMgr::_GetICUCollatorLocale()
{
	return fUseICUCollator ? dynamic_cast<VICUCollator*>( fCollator)->GetLocale() : NULL;
}
#endif


bool VIntlMgr::GetWordBoundariesWithOptions( const VString& inText, VectorOfStringSlice& outBoundaries, EKeyWordOptions inOptions)
{
	bool ok = false;

	VectorOfStringSlice boundaries;
	
	if (inText.IsEmpty())
	{
		ok = true;
	}
	else
	{
		if (inOptions & eKW_ConsiderOnlyDeadChars)
		{
			VIndex curpos = 0;
			VIndex totlen = inText.GetLength();
			const UniChar* curC = inText.GetCPointer();

			try
			{
				while( curpos < totlen)
				{
					UniChar c = *curC;
					if (_IsItaDeadChar( c))
					{
						curpos++;
						curC++;
					}
					else
					{
						VIndex len = 0;
						VIndex pos = curpos;
						do 
						{
							len++;
							curpos++;
							curC++;

							if (curpos<totlen)
							{
								c = *curC;
								if (_IsItaDeadChar( c))
									break;
							}
						} while(curpos < totlen);

						boundaries.push_back( VectorOfStringSlice::value_type( pos+1, len));

						curpos++;
						curC++;
					}
				}
			
				ok = true;
			}
			catch (...)
			{
			}
		}
		else if (inOptions & eKW_UseICU)
		{
			#if USE_ICU
		
			DialectCode dialectForCollator = GetCollator()->GetDialectCode();
			bool specialApostrophe = (DC_PRIMARYLANGID( dialectForCollator) == DC_PRIMARYLANGID( DC_FRENCH)) || (DC_PRIMARYLANGID( dialectForCollator) == DC_PRIMARYLANGID( DC_ITALIAN));
		
			try
			{
				UErrorCode status = U_ZERO_ERROR;

				xbox_icu::BreakIterator* breakIterator = NULL;
				if (inOptions & eKW_UseLineBreakingRule)
				{
					if (fBreakLineIterator == NULL)
						fBreakLineIterator = xbox_icu::BreakIterator::createLineInstance( *_GetICUCollatorLocale(), status);
					breakIterator = fBreakLineIterator;
				}
				else
				{
					if (fBreakIterator == NULL)
						fBreakIterator = xbox_icu::BreakIterator::createWordInstance( *_GetICUCollatorLocale(), status);
					breakIterator = fBreakIterator;
				}

				if (testAssert( U_SUCCESS(status)))
				{
					// need cast to use RuleBasedBreakIterator::getRuleStatus
					xbox_assert( dynamic_cast<RuleBasedBreakIterator*>( breakIterator) == breakIterator);
					RuleBasedBreakIterator *bi = (RuleBasedBreakIterator *) breakIterator;
				
					xbox_icu::UnicodeString string( true, inText.GetCPointer(), inText.GetLength());
					breakIterator->setText( string);

					int32_t start = bi->first();
					for( int32_t end = bi->next(); end != BreakIterator::DONE ; start = end, end = bi->next())
					{
						if (((inOptions & eKW_UseLineBreakingRule) != 0) || (bi->getRuleStatus() != UBRK_WORD_NONE) )
						{
							//if the word is a conson + apostrophe + voyel:
							//for line-breaking, never line-break between ' and voyel 
							//for word-breaking, we should break after the apostrophe
							if (((inOptions & eKW_UseLineBreakingRule) == 0) && specialApostrophe && _HasApostropheSecondFollowedByVowel( inText, start+1, end - start))
							{
								start += 2;
							}
							boundaries.push_back( VectorOfStringSlice::value_type( start + 1, end - start));
						}
					}
					ok = true;
				}
			}
			catch(...)
			{
			}
			#else
			xbox_assert( false);
			#endif
		}
		else if (inOptions & eKW_UseMeCab)
		{
			#if USE_MECAB
			IMecabModel *model = _GetMecabModel();
			if (fMecabLattice == NULL)
			{
				if (testAssert( model != NULL))
				{
					fMecabLattice = model->CreateLattice();
				}
			}

			if (fMecabLattice != NULL)
			{
				StStringConverter<uint8_t> buffer( inText, VTC_UTF_8);
			
				size_t *words = NULL;
				size_t count;
				ok = model->GetWordBoundaries( fMecabLattice, buffer.GetCPointer(), buffer.GetSize(), &words, &count);
			
				if (ok)
				{
					VToUnicodeConverter_UTF8 converter;
					UniChar word[256];	// big enough for one single word
					size_t *oneWord = words;
					for( size_t i = 0 ; i < count ; ++i)
					{
						size_t oneWordOffset = *oneWord++;
						size_t oneWordLength = *oneWord++;

						VSize consumed;
						VIndex produced;
						const UniChar *textBegin = inText.GetCPointer();
						const UniChar *textEnd = inText.GetCPointer() + inText.GetLength();
						VIndex pos = 0;
						if (testAssert( converter.Convert( buffer.GetCPointer() + oneWordOffset, oneWordLength, &consumed, &word[0], sizeof( word), &produced)))
						{
							const UniChar *found = std::search( textBegin + pos, textEnd, &word[0], &word[produced]);
							if (testAssert( found != textEnd))
							{
								pos = found - textBegin;
								boundaries.push_back( VectorOfStringSlice::value_type( pos+1, produced));
								pos += produced;
							}
						}
					}
					model->ReleaseWordBoundaries( words);
				}
			}
			else
			{
				ok = false;
			}
			#else
			xbox_assert( false);
			#endif
		}
	}
	
	boundaries.swap( outBoundaries);
	
	return ok;
}

/** get the bidi runs for the specified text 
@param inText
	text to analyze
@param outBidiRuns
	on output, will contain the bidi runs (see remarks below) ordered in LOGICAL order 
	(use outMapVisualToLogical to draw runs in visual order)
@param outMapVisualToLogical
	on output, will contain the map from visual run index to logical run index
	(for instance you draw from left to right outBidiLevelRuns[outMapVisualToLogical[0]], then outBidiLevelRuns[outMapVisualToLogical[1]], ...)
@param inBaseBidiLevel 
	VINTLMGR_UBIDI_DEFAULT_LTR (default): if first char is strongly directional, use its direction as base direction otherwise assume LTR base direction
	VINTLMGR_UBIDI_DEFAULT_RTL: if first char is strongly directional, use its direction as base direction otherwise assume RTL base direction
	other value:
	if even: assume LTR base direction
	if odd: assume RTL base direction 
@remarks
	return value is equal to the first paragraph first bidi level (might be not equal to inBaseBidiLevel)

	it is recommended to call this method per paragraph and not for a full text (as bidi should be determined per paragraph for proper layout per paragraph)
@note
	bidi level value is odd if RTL, even if LTR
*/
sLONG VIntlMgr::GetBidiRuns( const UniChar* inText, const sLONG inLength, VectorOfBidiRuns& outBidiRuns, VectorOfBidiVisualToLogical& outMapVisualToLogical, sLONG inBaseBidiLevel, bool inIgnoreLastCR)
{
	outBidiRuns.clear();
	outMapVisualToLogical.clear();
	bool isMixed = false;
	sLONG paraDirection = inBaseBidiLevel;
	xbox_assert(inLength >= 0);
	sLONG length = inLength;
	if (inIgnoreLastCR && length > 0 && inText[length-1] == 13)
		length--;

#if USE_ICU
	if (ICU_Available())
	{
		try
		{
			UBiDi *para = NULL;
			UErrorCode error = U_ZERO_ERROR;

			if (length > 0)
			{
				para = ubidi_openSized(length+1, 0, &error);
				if	(para != NULL)
				{
					
					xbox_assert(sizeof(UChar) == sizeof(UniChar));

					switch( inBaseBidiLevel)
					{
					case VINTLMGR_UBIDI_DEFAULT_LTR:
						inBaseBidiLevel = UBIDI_DEFAULT_LTR;
						break;
					case VINTLMGR_UBIDI_DEFAULT_RTL:
						inBaseBidiLevel = UBIDI_DEFAULT_RTL;
						break;
					default:
						break;
					}
					ubidi_setPara(	para, (const UChar *)inText, length,
									inBaseBidiLevel, 
									NULL, &error);
					if (U_SUCCESS(error)) 
					{
						UBiDiDirection direction = ubidi_getDirection(para);
						if (direction == UBIDI_NEUTRAL)
							direction = (inBaseBidiLevel & 1) ? UBIDI_RTL : UBIDI_LTR;
						paraDirection = direction;
						if (direction == UBIDI_MIXED) 
						{
							isMixed = true;
							sLONG count = ubidi_countRuns(para, &error);
							if (U_SUCCESS(error) && testAssert(count))
							{
								for (int i = 0; i < count; i++)
								{
									int32_t runStart = 0;
									int32_t runLength = 0;
									direction = ubidi_getVisualRun( para, i, &runStart, &runLength);
									outBidiRuns.push_back( VBidiRun( runStart, runStart + runLength, i, direction & 1));
								}
								//sort in logical order
								std::sort( outBidiRuns.begin(), outBidiRuns.end());
								
								//build visual to logical index map
								outMapVisualToLogical.resize(count);
								sLONG logicalIndex = 0;
								VectorOfBidiRuns::const_iterator itRun = outBidiRuns.begin();
								for (;itRun != outBidiRuns.end(); itRun++, logicalIndex++)
									outMapVisualToLogical[itRun->fVisualIndex] = logicalIndex;
							}
							else
								isMixed = false;
						}
					}
					ubidi_close( para);
				}
			}
		}
		catch (...)
		{
			isMixed = false;
			paraDirection = inBaseBidiLevel;
		}
	}
#endif

	if (!isMixed)
	{
		//one run only
		outBidiRuns.push_back( VBidiRun( 0, length, 0, paraDirection & 1));
		outMapVisualToLogical.push_back( 0);
	}
	else
		//multiple runs -> return direction of first logical run
		paraDirection = outBidiRuns[0].fBidiLevel;

	return (paraDirection & 1);
}


bool VIntlMgr::FindWord( const VString& inText, const VString& inPattern, const VCompareOptions& inOptions)
{
	bool ok = false;
	
	// need expensive algo when asked for pattern matching or when there's no icu or when using MeCab

	if ( (inOptions.IsLike() && (inPattern.FindUniChar( fCollator->GetWildChar()) > 0)) || (fKeyWordOptions & eKW_UseMeCab) )
	{
		VectorOfStringSlice boundaries;
		ok = GetWordBoundaries( inText, boundaries);
		if (ok)
		{
			// on ne peut faire de recherche dichotomique avec une pattern
			ok = false;
			for( VectorOfStringSlice::const_iterator i = boundaries.begin() ; (i != boundaries.end()) && !ok ; ++i)
				ok = fCollator->EqualString_Like( inText.GetCPointer() + i->first - 1, i->second, inPattern.GetCPointer(), inPattern.GetLength(), inOptions.IsDiacritical());
		}
	}
	else
	{
		if (fKeyWordOptions & eKW_ConsiderOnlyDeadChars)
		{
			sLONG matchedLength;
			const UniChar* pString = inText.GetCPointer();
			sLONG lenString = inText.GetLength();
			sLONG pos;

			do 
			{
				pos = fCollator->FindString( pString, lenString, inPattern.GetCPointer(), inPattern.GetLength(), inOptions.IsDiacritical(), &matchedLength);
				if (pos > 0)
				{
					// see if the found string is enclosed by dead chars
					bool deadChar_onLeft = (pos > 1) ? _IsItaDeadChar( pString[pos-2]) : true;
					bool deadChar_onRight = (pos + matchedLength - 1 < lenString) ? _IsItaDeadChar( pString[pos + matchedLength - 1]) : true;
					ok = deadChar_onLeft && deadChar_onRight;
					if (!ok)
					{
						pString = pString + pos;
						lenString = lenString - pos;
					}
				}

			} while(pos > 0 && !ok);
		}
		else if (fKeyWordOptions & eKW_UseICU)
		{
			#if USE_ICU
			UErrorCode status = U_ZERO_ERROR;

			if (fBreakIterator == NULL)
				fBreakIterator = xbox_icu::BreakIterator::createWordInstance( *_GetICUCollatorLocale(), status);

			if (testAssert( U_SUCCESS(status)))
			{
				xbox_icu::UnicodeString string( true, inText.GetCPointer(), inText.GetLength());
				fBreakIterator->setText( string);

				ok = false;
				sLONG matchedLength;
				const UniChar* pString = inText.GetCPointer();
				sLONG lenString = inText.GetLength();
				sLONG absolutePos = 0; // DH 28-Feb-2013 ACI0079481 Starting position for next find, used instead of shifting pString start (which makes offsets incompatible with fBreakIterator)
				sLONG pos;

				do 
				{
					pos = fCollator->FindString((pString + absolutePos), (lenString - absolutePos), inPattern.GetCPointer(), inPattern.GetLength(), inOptions.IsDiacritical(), &matchedLength);
					if (pos > 0)
					{
						// see if the found string is enclosed by dead chars
						absolutePos += pos;
						bool deadChar_onLeft = (absolutePos > 1) ? (fBreakIterator->isBoundary(absolutePos - 1) != 0) : true;
						bool deadChar_onRight = (absolutePos + matchedLength <= lenString) ? (fBreakIterator->isBoundary(absolutePos + matchedLength - 1) != 0) : true;
						ok = deadChar_onLeft && deadChar_onRight;
					}

				} while(pos > 0 && !ok);
				
				// si on a trouve qqchose on verifie que la pattern n'etait pas constituee de plusieurs mots
				if (ok)
				{
					DialectCode dialectForCollator = GetCollator()->GetDialectCode();
					bool specialApostrophe = (DC_PRIMARYLANGID( dialectForCollator) == DC_PRIMARYLANGID( DC_FRENCH)) || (DC_PRIMARYLANGID( dialectForCollator) == DC_PRIMARYLANGID( DC_ITALIAN));
					if (specialApostrophe && _HasApostropheSecondFollowedByVowel( inPattern, 1, inPattern.GetLength()))
						ok = false;
					else
					{
						xbox_icu::UnicodeString pattern_string( true, inPattern.GetCPointer(), inPattern.GetLength());
						fBreakIterator->setText( pattern_string);
						
						int32_t start = fBreakIterator->first();
						int32_t end = fBreakIterator->next();
						
						if ( (start != 0) || (end != inPattern.GetLength()) )
							ok = false;
					}
				}
			}
			#else
			xbox_assert( false);
			#endif
		}
	}

	return ok;
}


void VIntlMgr::FindStrings( const VString& inText, const VString& inPattern, EFindStringMode inOperator, bool inWithDiacritics, VectorOfStringSlice& outMatches)
{
	outMatches.clear();
	if (!inPattern.IsEmpty())
	{
		VectorOfStringSlice wordBoundaries;
		bool gotWordBoundaries = false;

		sLONG pos;
		sLONG offset = 0;
		do
		{
			sLONG foundLength;
			pos = GetCollator()->FindString( inText.GetCPointer() + offset, inText.GetLength() - offset, inPattern.GetCPointer(), inPattern.GetLength(), inWithDiacritics, &foundLength);
			if (pos > 0)
			{
				bool ok;
				pos += offset;
				if (inOperator == eFindContains)
				{
					ok = true;
				}
				else
				{
					ok = true;
					if (!gotWordBoundaries)
					{
						GetWordBoundaries( inText, wordBoundaries);
						gotWordBoundaries = true;
					}
					
					XBOX::VIndex endPos = pos + foundLength;
					switch( inOperator)
					{
						case eFindStartsWith:
							{
								// check the pos is not inside a word
								for( VectorOfStringSlice::iterator i = wordBoundaries.begin() ; i != wordBoundaries.end() ; ++i)
								{
									if ( (pos > i->first) && (pos < i->first + i->second) )
									{
										ok = false;
										break;
									}
								}
								break;
							}

						case eFindWholeWords:
							{
								// check both the start and the end are not inside a word
								for( VectorOfStringSlice::iterator i = wordBoundaries.begin() ; i != wordBoundaries.end() ; ++i)
								{
									if ( ( (pos > i->first) && (pos < i->first + i->second) ) || ( (endPos > i->first) && (endPos < i->first + i->second) ) )
									{
										ok = false;
										break;
									}
								}
								break;
							}
						
						case eFindEndsWith:
							{
								// check the end pos is not inside a word
								for( VectorOfStringSlice::iterator i = wordBoundaries.begin() ; i != wordBoundaries.end() ; ++i)
								{
									if ( (endPos > i->first) && (endPos < i->first + i->second) )
									{
										ok = false;
										break;
									}
								}
								break;
							}
						
						default:
							xbox_assert( false);
						
					}
				}
				if (ok)
					outMatches.push_back( VectorOfStringSlice::value_type( pos, foundLength));
				offset = pos + foundLength - 1;
			}
		} while( (pos > 0) && (offset < inText.GetLength()) );
	}
}



bool VIntlMgr::IsAMacCharacter(uBYTE cc, uBYTE set)
{
	switch (set)
	{
		case 0: // any valid character
			return (cc==(uBYTE)KEY_RETURN || cc==(uBYTE)KEY_TAB || (cc>=(uBYTE)0x20 && cc!=(uBYTE)0x7F));
		case 1: // chars & nums
			return (	(cc>=(uBYTE)'0' && cc<=(uBYTE)'9')
						||	(cc>=(uBYTE)'a' && cc<=(uBYTE)'z')
						||	(cc>=(uBYTE)'A' && cc<=(uBYTE)'Z')
						||	(cc>=(uBYTE)0x80 && cc<=(uBYTE)0x9F)
						||	(cc==(uBYTE)0xAE) || (cc==(uBYTE)0xBE)
						||	(cc>=(uBYTE)0xCB && cc<=(uBYTE)0xCF)
						||	(cc=='_'));
		case 2: // chars 
			return (	(cc>=(uBYTE)'a' && cc<=(uBYTE)'z')
						||	(cc>=(uBYTE)'A' && cc<=(uBYTE)'Z')
						||	(cc>=(uBYTE)0x80 && cc<=(uBYTE)0x9F)
						||	(cc==(uBYTE)0xAE) || (cc==(uBYTE)0xBE)
						||	(cc>=(uBYTE)0xCB && cc<=(uBYTE)0xCF)
						||	(cc=='_'));
		case 3: // chars & nums strict
			return (	(cc>=(uBYTE)'0' && cc<=(uBYTE)'9')
						||	(cc>=(uBYTE)'a' && cc<=(uBYTE)'z')
						||	(cc>=(uBYTE)'A' && cc<=(uBYTE)'Z')
						||	(cc=='_'));
		case 4: // chars strict
			return (	(cc>=(uBYTE)'a' && cc<=(uBYTE)'z') ||	(cc>=(uBYTE)'A' && cc<=(uBYTE)'Z') ||	(cc=='_'));
		case 5: // nums strict
			return (cc>=(uBYTE)'0' && cc<=(uBYTE)'9');
		default:
			return false;
	}
}

void VIntlMgr::GetAllDialects( VectorOfDialect& outDialects)
{
	size_t count = sizeof(sDialectsList) / sizeof(Dialect);

	outDialects.reserve( count );

	for( size_t i = 0 ; i < count ; ++i )
	{
		DialectCode dialect = sDialectsList[i].fDialectCode;
		outDialects.push_back( dialect);
	}
}

void VIntlMgr::GetDialectsHavingCollator( VectorOfNamedDialect& outDialects)
{
	#if USE_ICU
	if (ICU_Available())
	{
		std::vector<const char*> locales;
		std::vector<VString> names;

		VICUCollator::GetLocalesHavingCollator( *fLocale, locales, names);
		
		// pour chaque locale retournee, il faut trouver son dialectcode
		outDialects.reserve( locales.size());
		for( std::vector<const char*>::const_iterator i = locales.begin() ; i != locales.end() ; ++i)
		{
			char lang[256];
			xbox_assert( strlen( *i) < 256);
			strcpy( lang, *i);
			char *subLang = strstr( lang, "_");
			if (subLang != NULL)
				*subLang++ = '\0';
			
			DialectCode dialect;
			
			if (GetDialectCodeWithRFC3066BisLanguageCode( lang, subLang, dialect, false))
			{
				outDialects.push_back( VectorOfNamedDialect::value_type( names.at( i - locales.begin()), dialect));
			}
		}
		
		// sort by display name
		std::sort( outDialects.begin(), outDialects.end(), VStringPredicate_paired( GetCollator(), true));

		return;
	}
	#endif
	
	size_t count = sizeof(sDialectsList) / sizeof(Dialect);

	outDialects.reserve( count );

	for( size_t i = 0 ; i < count ; ++i )
	{
		DialectCode dialect = sDialectsList[i].fDialectCode;

		outDialects.push_back( VectorOfNamedDialect::value_type( VString( GetISO6391LanguageCode( dialect)), dialect));	// boff
	}
}

bool VIntlMgr::GetISO6391LanguageCode( DialectCode inDialect, VString& outLanguageCode )
{
	short maxCount = sizeof(sDialectsList) / sizeof(Dialect);
	outLanguageCode.Clear();

	short i;
	for( i = 0 ; i < maxCount; ++i ) {
		if( sDialectsList[i].fDialectCode == inDialect ) {
			outLanguageCode.FromCString( sDialectsList[i].fISO6391Code );
			break;	
		}
	}

	return( outLanguageCode.GetLength() != 0 );
}

const char* VIntlMgr::GetISO6391LanguageCode( DialectCode inDialect)
{
	short maxCount = sizeof(sDialectsList) / sizeof(Dialect);
	short i;
	for( i = 0 ; i < maxCount; ++i ) {
		if( sDialectsList[i].fDialectCode == inDialect ) {
			return sDialectsList[i].fISO6391Code;
			break;	
		}
	}

	return NULL;
}

bool VIntlMgr::GetISO6392LanguageCode( DialectCode inDialect, VString& outLanguageCode )
{
	short maxCount = sizeof(sDialectsList) / sizeof(Dialect);
	outLanguageCode.Clear();

	short i;
	for( i = 0 ; i < maxCount; ++i ) {
		if( sDialectsList[i].fDialectCode == inDialect ) {
			outLanguageCode.FromCString( sDialectsList[i].fISO6392Code );
			break;	
		}
	}
	return( outLanguageCode.GetLength() != 0 );
}

/*
	static
*/
bool VIntlMgr::GetBCP47LanguageCode( DialectCode inDialect, VString& outLanguageCode)
{
	// Get language and region strings.
	outLanguageCode.Clear();
	for( size_t i = 0; i<(sizeof(sDialectsList)/sizeof(Dialect)) ; ++i)
	{
		if (sDialectsList[i].fDialectCode == inDialect)
		{
			outLanguageCode = sDialectsList[i].fISO6391Code;
			if (sDialectsList[i].fISO3166RegionCode[0] != 0)
			{
				outLanguageCode += '-';
				outLanguageCode += sDialectsList[i].fISO3166RegionCode;
			}
			break;
		}
	}
	
	return !outLanguageCode.IsEmpty();
}

bool VIntlMgr::GetISOLanguageName( DialectCode inDialect, VString& outLanguageName )
{
	short maxCount = sizeof(sDialectsList) / sizeof(Dialect);
	outLanguageName.Clear();

	short i;
	for( i = 0 ; i < maxCount; ++i ) {
		if( sDialectsList[i].fDialectCode == inDialect ) {
			outLanguageName.FromCString( sDialectsList[i].fISOLegacyName );
			break;	
		}
	}
	return( outLanguageName.GetLength() != 0 );
}

/*
	static

	Indique si le dialectCode passe en parametre fait partie de la dialectsList
*/
bool VIntlMgr::IsDialectCode(DialectCode inDialect)
{
	bool bfound = false;
	short maxCount = sizeof(sDialectsList) / sizeof(Dialect);

	short i;
	for( i = 0 ; i < maxCount; ++i ) 
	{
		if( sDialectsList[i].fDialectCode == inDialect )
		{
			bfound = true;
			break;	
		}
	}
	return bfound;
}


/*
	static
*/
bool VIntlMgr::IsRightToLeftDialect( DialectCode inDialect)
{
	uWORD langID = DC_PRIMARYLANGID( ResolveDialectCode( inDialect));
	return (langID == DC_PRIMARYLANGID( DC_ARABIC))
		|| (langID == DC_PRIMARYLANGID( DC_FARSI))
		|| (langID == DC_PRIMARYLANGID( DC_HEBREW))
//		|| (langID == DC_PRIMARYLANGID( DC_HINDI))
		|| (langID == DC_PRIMARYLANGID( DC_URDU));
}


// recupere la langue et le pays sous la forme "langue (region)"
// retourne true si la langue est valide
bool VIntlMgr::GetLocalizedLanguageName( DialectCode inDialect, VString& outLanguageName)
{
	bool ok = false;
	
	#if USE_ICU
	if (fLocale != NULL)
	{
		// suivant le dialecte, on recupere l'ISO6391_ISO3166 (xx_XX)
		xbox_icu::Locale locale( GetISO6391LanguageCode( inDialect), GetISO3166RegionCode( inDialect));
		
		// recuperation du langage
		UnicodeString displayName;
		locale.getDisplayLanguage( *fLocale, displayName);
		
		// recuperation de la region
		UnicodeString displayCountry;
		locale.getDisplayCountry( *fLocale, displayCountry);
		UnicodeString displayEntireName;

		if(displayCountry.length() == 0)
		{
			// si la region est vide, on affiche que le langage
			displayEntireName = displayName;
		}
		else
		{
			// sinon on affiche au format "langue (region)"
			displayEntireName = displayName +" (" + displayCountry +")";
		}

		UniChar *p = outLanguageName.GetCPointerForWrite( displayEntireName.length());
		if (p != NULL)
		{
			UErrorCode err = U_ZERO_ERROR;
			ok = outLanguageName.Validate( displayEntireName.extract( p, outLanguageName.GetEnsuredSize(), err));
		}
	}
	#endif
	return ok;
}


bool VIntlMgr::GetISO3166RegionCode( DialectCode inDialect, VString& outRegionCode )
{
	short maxCount = sizeof(sDialectsList) / sizeof(Dialect);
	outRegionCode.Clear();

	short i;
	for( i = 0 ; i < maxCount; ++i ) {
		if( sDialectsList[i].fDialectCode == inDialect ) {
			outRegionCode.FromCString( sDialectsList[i].fISO3166RegionCode );
			break;	
		}
	}
	return( outRegionCode.GetLength() != 0 );
}

const char* VIntlMgr::GetISO3166RegionCode( DialectCode inDialect)
{
	short maxCount = sizeof(sDialectsList) / sizeof(Dialect);
	short i;
	for( i = 0 ; i < maxCount; ++i ) {
		if( sDialectsList[i].fDialectCode == inDialect ) {
			return sDialectsList[i].fISO3166RegionCode ;
			break;	
		}
	}
	return NULL;
}

bool VIntlMgr::GetRFC3066BisSubTagRegionCode( DialectCode inDialect, VString& outRFC3066BisSubTag )
{
	short maxCount = sizeof(sDialectsList) / sizeof(Dialect);
	outRFC3066BisSubTag.Clear();
	
	short i;
	for( i = 0 ; i < maxCount; ++i ) {
		if( sDialectsList[i].fDialectCode == inDialect ) {
			outRFC3066BisSubTag.FromCString( sDialectsList[i].fRFC3066SubTag );
			break;	
		}
	}
	return( outRFC3066BisSubTag.GetLength() != 0 );
}




bool VIntlMgr::GetRFC3066BisLanguageCodeWithDialectCode( DialectCode inDialectCode, VString& outRFC3066LanguageCode,char inSeparator)
{
	outRFC3066LanguageCode.Clear();
	VString iSO6391Code, rfc3066BisSubTagCode;

	if (!GetISO6391LanguageCode(inDialectCode, iSO6391Code))
		return false;

	outRFC3066LanguageCode += iSO6391Code;

	if (GetRFC3066BisSubTagRegionCode(inDialectCode, rfc3066BisSubTagCode))
	{
		outRFC3066LanguageCode += inSeparator;
		outRFC3066LanguageCode += rfc3066BisSubTagCode;
	}

	return true;
}

bool VIntlMgr::GetDialectCodeWithRFC3066BisLanguageCode( const char *inISO6391Code,
														 const char *inRFC3066SubTag, 
														 DialectCode& outDialectCode, 
														 bool searchForISO6391CodeIfNothingFound, 
														 bool calculateDialectIfNoSubTag)
{
	outDialectCode = DC_NONE;

	const Dialect *begin = sDialectsList;
	const Dialect *end = begin + sizeof(sDialectsList) / sizeof(Dialect);

	for( const Dialect *i = begin ; i != end; ++i)
	{
		if ( usascii_stricmp( inISO6391Code, i->fISO6391Code ) == 0)
		{
			if (inRFC3066SubTag == NULL)
			{
				if (i->fRFC3066SubTag[0] == 0 && calculateDialectIfNoSubTag == true)
				{
					outDialectCode = DC_MAKEDIALECT( DC_PRIMARYLANGID(i->fDialectCode), 1 );
					break;
				}
				else if(i->fRFC3066SubTag[0] == 0 && calculateDialectIfNoSubTag == false)
				{
					outDialectCode = i->fDialectCode;
					break;
				}
			}
			else if ( usascii_stricmp( inRFC3066SubTag, i->fRFC3066SubTag) == 0 )
			{
				outDialectCode = i->fDialectCode;
				break;	
			}
		}
	}

	if ( (outDialectCode == DC_NONE) && (inRFC3066SubTag != NULL) && searchForISO6391CodeIfNothingFound)
	{
		VIntlMgr::GetDialectCodeWithRFC3066BisLanguageCode( inISO6391Code, NULL, outDialectCode, false);
	}

	return outDialectCode != DC_NONE;
}


bool VIntlMgr::GetDialectCodeWithRFC3066BisLanguageCode( const VString& inRFC3066LanguageCode, 
														 DialectCode& outDialectCode, 
														 bool searchForISO6391CodeIfNothingFound, 
														 bool calculateDialectIfNoSubTag)
{
	// HM = HYPHEN_MINUS
	// LL = LOW LINE
	VectorOfVString languageCodesByHM, languageCodesByLL;

	bool hyphen_minus_found =  inRFC3066LanguageCode.GetSubStrings(CHAR_HYPHEN_MINUS, languageCodesByHM);
	bool low_line_found = inRFC3066LanguageCode.GetSubStrings(CHAR_LOW_LINE, languageCodesByLL);
	
	// en principe, on devrait avoir language-territory pour le RFC3066
	// or dans la pratique, au niveau fichier, on a language_territory
	// il faut donc regarder les deux
	VString languageCode, subTagCode;
	char ascii_languageCode[256];
	char ascii_subTagCode[256];
	bool found;
	if (languageCodesByHM.size() == 1 && languageCodesByLL.size() == 1 && !hyphen_minus_found && !low_line_found) // pp uniquement valable si on a trouver ni - ni _
	{
		languageCodesByHM.front().ToCString( ascii_languageCode, sizeof( ascii_languageCode));
		found = GetDialectCodeWithRFC3066BisLanguageCode( ascii_languageCode, NULL, outDialectCode, searchForISO6391CodeIfNothingFound, calculateDialectIfNoSubTag);
	}
	else if (languageCodesByHM.size() == 2)
	{
		languageCodesByHM.at( 0).ToCString( ascii_languageCode, sizeof( ascii_languageCode));
		languageCodesByHM.at( 1).ToCString( ascii_subTagCode, sizeof( ascii_subTagCode));
		found = GetDialectCodeWithRFC3066BisLanguageCode( ascii_languageCode, ascii_subTagCode, outDialectCode, searchForISO6391CodeIfNothingFound, calculateDialectIfNoSubTag);
	}
	else if (languageCodesByLL.size() == 2)
	{
		languageCodesByLL.at( 0).ToCString( ascii_languageCode, sizeof( ascii_languageCode));
		languageCodesByLL.at( 1).ToCString( ascii_subTagCode, sizeof( ascii_subTagCode));
		found = GetDialectCodeWithRFC3066BisLanguageCode( ascii_languageCode, ascii_subTagCode, outDialectCode, searchForISO6391CodeIfNothingFound, calculateDialectIfNoSubTag);
	}
	else
	{
		outDialectCode = DC_NONE;
		found = false;
	}

	return found;
}

bool VIntlMgr::GetDialectCodeWithISOLanguageName(const VString& inISOLanguageName, DialectCode& outDialectCode)
{
	short maxCount = sizeof(sDialectsList) / sizeof(Dialect);
	bool weHaveAResult = false;
	VString iSOLanguageNameAltered = inISOLanguageName;
	iSOLanguageNameAltered.ToLowerCase();
	short i;
	for( i = 0 ; i < maxCount; ++i ) 
	{
		// If the RFC3066 code is empty, we know that this is the language to use by default
		if( iSOLanguageNameAltered == sDialectsList[i].fISOLegacyName && VString("") == sDialectsList[i].fRFC3066SubTag)
		{
			outDialectCode = sDialectsList[i].fDialectCode;
			outDialectCode = (XBOX::DialectCode)(outDialectCode & 0x00FF);
			outDialectCode = (XBOX::DialectCode)(outDialectCode | (0x0001<<10));
			weHaveAResult = true;
			break;	
		}
	}
	return weHaveAResult;
}

RFC3066CodeCompareResult VIntlMgr::Compare2RFC3066LanguageCodes(const VString & inFirstRFC3066Code, const VString & inSecondRFC3066Code)
{
	VString rFC3066Code1 = inFirstRFC3066Code;
	VString rFC3066Code2 = inSecondRFC3066Code;
	rFC3066Code1.ToLowerCase();
	rFC3066Code2.ToLowerCase();

	DialectCode dialectCode1 = DC_NONE;
	DialectCode dialectCode2 = DC_NONE;
	if(VIntlMgr::GetDialectCodeWithRFC3066BisLanguageCode(rFC3066Code1, dialectCode1) == false || VIntlMgr::GetDialectCodeWithRFC3066BisLanguageCode(rFC3066Code2, dialectCode2) == false)
		return RFC3066_CODES_NOT_VALID;

	RFC3066CodeCompareResult compareResult = RFC3066_CODES_ARE_NOT_EQUAL;
	if(rFC3066Code1 == rFC3066Code2)
		compareResult = RFC3066_CODES_ARE_EQUAL;
	else{
		VArrayString rFC3066Code1SubStrings, rFC3066Code2SubStrings;
		rFC3066Code1.GetSubStrings(CHAR_HYPHEN_MINUS, rFC3066Code1SubStrings);
		rFC3066Code2.GetSubStrings(CHAR_HYPHEN_MINUS, rFC3066Code2SubStrings);

		if(rFC3066Code1SubStrings.GetCount() == 2 && rFC3066Code2SubStrings.GetCount() == 1){
			VString rFC3066Code1Value1, rFC3066Code2Value1;
			rFC3066Code1SubStrings.GetValue(rFC3066Code1Value1, 1);
			rFC3066Code2SubStrings.GetValue(rFC3066Code2Value1, 1);
			if(rFC3066Code1Value1 == rFC3066Code2Value1)
				compareResult = RFC3066_CODES_SPECIFIC_LANGUAGE_REGION_AND_GLOBAL_LANGUAGE_VARIANT;
		}
		else if(rFC3066Code1SubStrings.GetCount() == 1 && rFC3066Code2SubStrings.GetCount() == 2){
			VString rFC3066Code1Value1, rFC3066Code2Value1;
			rFC3066Code1SubStrings.GetValue(rFC3066Code1Value1, 1);
			rFC3066Code2SubStrings.GetValue(rFC3066Code2Value1, 1);
			if(rFC3066Code1Value1 == rFC3066Code2Value1)
				compareResult = RFC3066_CODES_GLOBAL_LANGUAGE_AND_SPECIFIC_LANGUAGE_REGION_VARIANT;
		}
		else if(rFC3066Code1SubStrings.GetCount() == 2 && rFC3066Code2SubStrings.GetCount() == 2){
			VString rFC3066Code1Value1, rFC3066Code2Value1;
			rFC3066Code1SubStrings.GetValue(rFC3066Code1Value1, 1);
			rFC3066Code2SubStrings.GetValue(rFC3066Code2Value1, 1);
			if(rFC3066Code1Value1 == rFC3066Code2Value1){
				compareResult = RFC3066_CODES_LANGUAGE_REGION_VARIANT;
			}
		}
	}

	return compareResult;
}

bool VIntlMgr::GetLocalizationFoldersWithDialect( DialectCode inDialectCode, const VFilePath & inLocalizationFoldersPath, VectorOfVFolder& outRetainedLocalizationFoldersList)
{
	if(!inLocalizationFoldersPath.IsValid())
		return false;

	VFilePath alteredLocalizationFoldersPath = inLocalizationFoldersPath;

	//We try to find the localization folder with the RF3066 code name in a first step
	VString rF3066LanguageFolderName;
	VIntlMgr::GetRFC3066BisLanguageCodeWithDialectCode(inDialectCode, rF3066LanguageFolderName);
	rF3066LanguageFolderName += CVSTR(".lproj");
	VFolder * retainedRFC3066LocalizationFolder = new VFolder(alteredLocalizationFoldersPath.ToFolder().ToSubFolder(rF3066LanguageFolderName));

	if(retainedRFC3066LocalizationFolder->Exists()){
		outRetainedLocalizationFoldersList.push_back(retainedRFC3066LocalizationFolder); 
	}
	ReleaseRefCountable( &retainedRFC3066LocalizationFolder);

	//We try to find the localization folder with the RF3066 code name with _ separator
	VString rF3066LanguageFolderName_;
	VIntlMgr::GetRFC3066BisLanguageCodeWithDialectCode(inDialectCode, rF3066LanguageFolderName_,'_');
	rF3066LanguageFolderName_ += CVSTR(".lproj");
	if(rF3066LanguageFolderName_ != rF3066LanguageFolderName)
	{
		alteredLocalizationFoldersPath = inLocalizationFoldersPath;
		VFolder * retainedRFC3066LocalizationFolder_ = new VFolder(alteredLocalizationFoldersPath.ToFolder().ToSubFolder(rF3066LanguageFolderName_));
	
		if(retainedRFC3066LocalizationFolder_->Exists()){
			outRetainedLocalizationFoldersList.push_back(retainedRFC3066LocalizationFolder_); 
		}
		ReleaseRefCountable( &retainedRFC3066LocalizationFolder_);
	}
	
	//We use the ISO639-1 language code (ie : fr-fr -> fr)
	VString iSO6391LanguageFolderName;
	VIntlMgr::GetISO6391LanguageCode(inDialectCode, iSO6391LanguageFolderName);
	iSO6391LanguageFolderName += CVSTR(".lproj");
	if(iSO6391LanguageFolderName != rF3066LanguageFolderName){
		alteredLocalizationFoldersPath = inLocalizationFoldersPath;
		VFolder * retainedISO6391LocalizationFolder = new VFolder(alteredLocalizationFoldersPath.ToFolder().ToSubFolder(iSO6391LanguageFolderName));
		if(retainedISO6391LocalizationFolder->Exists()){
			outRetainedLocalizationFoldersList.push_back(retainedISO6391LocalizationFolder); 
		}
		ReleaseRefCountable( &retainedISO6391LocalizationFolder);
	}

	//We use the ISO639-1 language code  + region code(ie : fr-FR -> fr_FR)
	
	VString ISO3166RegionCode;
	
	VIntlMgr::GetISO3166RegionCode(inDialectCode, ISO3166RegionCode);
	if(!ISO3166RegionCode.IsEmpty())
	{
		VString Language_RegionFolderName;
		
		VIntlMgr::GetISO6391LanguageCode(inDialectCode, Language_RegionFolderName);
		
		Language_RegionFolderName +="_";
		Language_RegionFolderName +=ISO3166RegionCode;
		
		Language_RegionFolderName += CVSTR(".lproj");
		if(Language_RegionFolderName != rF3066LanguageFolderName){
			alteredLocalizationFoldersPath = inLocalizationFoldersPath;
			VFolder * retainedISO6391LocalizationFolder = new VFolder(alteredLocalizationFoldersPath.ToFolder().ToSubFolder(Language_RegionFolderName));
			if(retainedISO6391LocalizationFolder->Exists()){
				outRetainedLocalizationFoldersList.push_back(retainedISO6391LocalizationFolder); 
			}
			ReleaseRefCountable( &retainedISO6391LocalizationFolder);
		}
	}
	
	//We use the ISO language to find the localization folder name (fr -> French)
	VString iSOLanguageFolderName;
	VIntlMgr::GetISOLanguageName(inDialectCode, iSOLanguageFolderName);
	if(!iSOLanguageFolderName.IsEmpty()){
		VString firstCharacter = VString(iSOLanguageFolderName[0]);
		firstCharacter.ToUpperCase();
		iSOLanguageFolderName.Replace(firstCharacter, 1, 1);
		iSOLanguageFolderName += CVSTR(".lproj");
		alteredLocalizationFoldersPath = inLocalizationFoldersPath;
		VFolder * retainedISOLanguageLocalizationFolder = new VFolder(alteredLocalizationFoldersPath.ToFolder().ToSubFolder(iSOLanguageFolderName));

		if(retainedISOLanguageLocalizationFolder->Exists()){
			outRetainedLocalizationFoldersList.push_back(retainedISOLanguageLocalizationFolder); 
		}
		ReleaseRefCountable( &retainedISOLanguageLocalizationFolder);
	}

	return !outRetainedLocalizationFoldersList.empty();
}


VFile *VIntlMgr::RetainLocalizationFile( const VFilePath& inResourcesFolderPath, const VString& inFileName, DialectCode inDialect, bool *outAtLeastOneFolderScaned)
{
	VFile *file = NULL;
	VectorOfVFolder folders;
	bool atLeastOneFolderScaned = false;
	if (VIntlMgr::GetLocalizationFoldersWithDialect( inDialect, inResourcesFolderPath, folders))
	{
		// reverse scan to check EN-US.lproj before EN.lproj
		for( VectorOfVFolder::reverse_iterator i = folders.rbegin() ; i != folders.rend() ; ++i)
		{
			atLeastOneFolderScaned = true;
			file = new VFile( **i, inFileName);
			if (file->Exists())
				break;
			ReleaseRefCountable( &file);
		}
	}

	if (outAtLeastOneFolderScaned != NULL)
		*outAtLeastOneFolderScaned = atLeastOneFolderScaned;
	
	return file;
}


UniChar VIntlMgr::GetWildChar() const
{
	return fCollator->GetWildChar();
}



#if VERSIONWIN && VERSIONDEBUG

static VArrayString* gLocalesNames = NULL;
static VArrayLong* gLocalesIDs = NULL;

static BOOL CALLBACK WIN_EnumSystemLocales_Proc(LPTSTR szLocaleString)
{
static DWORD gLastLocale = 0;

	VStr255 spString;
	spString.FromCString(szLocaleString);

	assert(spString.GetLength() == 8);
	spString.Remove(1, 4);
	uLONG dialect = spString.GetHexLong();
	//dialect &= 0x3ffL;
	
	if (gLastLocale != dialect) {
		gLastLocale = dialect;

		char spName[255];
		DWORD locale = dialect; //MAKELCID((WORD) dialect, SORT_DEFAULT);
		DWORD length = ::GetLocaleInfoA(locale, LOCALE_NOUSEROVERRIDE | LOCALE_SENGLANGUAGE, spName, sizeof(spName)-1)-1;
		spName[length] = 0;
		spString.FromCString(spName);

		if (SUBLANGID(locale) != SUBLANG_DEFAULT)
		{
			VStr255 spCountry;
			length = ::GetLocaleInfoA(locale, LOCALE_NOUSEROVERRIDE | LOCALE_SENGCOUNTRY, spName, sizeof(spName)-1)-1;
			spName[length] = 0;
			spCountry.FromCString(spName);
		
			spString.AppendCString(" (");
			spString.AppendString(spCountry);
			spString.AppendCString(")");
		}

		gLocalesIDs->AppendLong(locale);
		gLocalesNames->AppendString(spString);
	}
	return true;
}

#if USE_MAC_RESFILES

void VIntlMgr::WIN_BuildDialectTMPL()
{
	// L.E. 19/04/1999 genere un fichier de ressource pour les dialectes
	VFilePath path = VProcess::Get()->GetFolder()->GetPath();
	VMacRsrcFork file(path, FA_READ_WRITE, true);
	
	if (file.IsValid())
	{
		VHandle h = NULL;
		VHandle h2 = NULL;
		VHandle h3 = NULL;
		VHandleStream* gEnumSystemLocalesStream = new VHandleStream(&h);
		VHandleStream* gDialectsStream = new VHandleStream(&h2);
		VHandleStream* gDialectNamesStream = new VHandleStream(&h3);

		gLocalesNames = new VArrayString;
		gLocalesIDs = new VArrayLong;
		
		gEnumSystemLocalesStream->SetBigEndian();
		gDialectsStream->SetBigEndian();
		gDialectNamesStream->SetBigEndian();

		gEnumSystemLocalesStream->OpenWriting();
		gDialectsStream->OpenWriting();
		gDialectNamesStream->OpenWriting();

		BOOL isOK = EnumSystemLocales((LOCALE_ENUMPROCA) WIN_EnumSystemLocales_Proc, LCID_SUPPORTED);

		if (isOK) {

			// gLocalesIDs->SynchronizeWith(gLocalesNames);
			// gLocalesNames->Sort();
			VArrayValue::SynchronizedSort(false, gLocalesNames, gLocalesIDs, 0);
			
			gDialectsStream->PutWord((sWORD) gLocalesIDs->GetCount());
			gDialectNamesStream->PutWord((sWORD) gLocalesIDs->GetCount());
			for (long i = 1 ; i<= gLocalesIDs->GetCount() ; ++i) {
				VStr255 spTemp;
				VStringConvertBuffer spMacString(VTC_MacOSAnsi);
				VStringConvertBuffer spMacString2(VTC_MacOSAnsi);
				
				gLocalesNames->GetString(spTemp, i);
				spMacString2.ConvertString(spTemp);
				gEnumSystemLocalesStream->PutByte((sBYTE) spMacString2.GetSize() + spMacString.GetSize() + 1);
				gEnumSystemLocalesStream->PutBytes(spMacString2.GetCPointer(), spMacString2.GetSize());
				gEnumSystemLocalesStream->PutByte('=');

				sLONG locale = gLocalesIDs->GetLong(i);
				spTemp.FromLong(locale);
				spMacString.ConvertString(spTemp);
				gEnumSystemLocalesStream->PutBytes(spMacString.GetCPointer(), spMacString.GetSize());

				gEnumSystemLocalesStream->PutLong((sLONG) 'CASE');

				gDialectsStream->PutLong(locale);
				gDialectsStream->PutWord(0);	// mac script code
				gDialectsStream->PutWord(0);	// mac lang code
				gDialectsStream->PutWord(0);	// mac region code
				gDialectsStream->PutLong(0);	// win kbd device
				gDialectsStream->PutLong(0);	// filler
				gDialectsStream->PutLong(0);	// filler

				gDialectNamesStream->PutLong(locale);
				gDialectNamesStream->PutByte((sBYTE) spMacString2.GetSize());
				gDialectNamesStream->PutBytes(spMacString2.GetCPointer(), spMacString2.GetSize());
			}
		}

		gEnumSystemLocalesStream->CloseWriting();
		gDialectsStream->CloseWriting();
		gDialectNamesStream->CloseWriting();

		if (isOK) {
			VResourceFile &theFile = file; // etonnant non ?
			theFile.WriteResource(VResTypeString('TMPL'), 128, h, NULL);
			theFile.WriteResource(VResTypeString('XDLC'), 128, h2, NULL);
			theFile.WriteResource(VResTypeString('XDLN'), 128, h3, NULL);
		}
		delete gEnumSystemLocalesStream;
		delete gDialectsStream;
		delete gDialectNamesStream;
		VMemory::DisposeHandle(h);
		VMemory::DisposeHandle(h2);
		VMemory::DisposeHandle(h3);
		delete gLocalesIDs;
		delete gLocalesNames;
	}
}

#endif	// USE_MAC_RESFILES

#endif	// VERSIONWIN


bool VIntlMgr::DebugConvertString(void* inSrcText, sLONG inSrcBytes, CharSet inSrcCharSet, void* inDstText, sLONG& inDstMaxBytes, CharSet inDstCharSet)
{
	bool	isOK = false;

	if (inSrcCharSet == inDstCharSet)
	{
		sLONG length = inSrcBytes;
		if (length > inDstMaxBytes)
			length = inDstMaxBytes;
		CopyBlock(inSrcText, inDstText, length);
		inDstMaxBytes = length;
		isOK = true;
	}
	else
	{
	#if VERSIONWIN
		if (inSrcCharSet == VTC_UTF_16 && inDstCharSet == VTC_Win32Ansi)
		{
			inDstMaxBytes = ::WideCharToMultiByte(CP_ACP, 0, (LPCWSTR) inSrcText, inSrcBytes/2, (LPSTR) inDstText , inDstMaxBytes, NULL, NULL);	
			isOK = (inDstMaxBytes != 0);
		}
	#elif VERSIONMAC && VERSIONDEBUG
		
		if (inSrcCharSet == VTC_UTF_16 && inDstCharSet == VTC_MacOSAnsi)
		{
			Str255	spTemp;
			UnicodeToTextInfo	info;
			::CreateUnicodeToTextInfoByEncoding(kTextEncodingMacRoman, &info);
			OSStatus	macErr = ::ConvertFromUnicodeToPString(info, inSrcBytes, (uWORD*) inSrcText, spTemp);
			if (macErr == noErr)
			{
				isOK = true;
				inDstMaxBytes = (spTemp[0] > inDstMaxBytes) ? inDstMaxBytes : spTemp[0];
				::memmove( inDstText, &spTemp[1], inDstMaxBytes);
			}
			
			::DisposeUnicodeToTextInfo(&info);
		}
    #elif VERSION_LINUX
		bool DebugConvertStringMethod=false;
        xbox_assert(DebugConvertStringMethod); // Postponed Linux Implementation !

	#endif
	}
	
	return isOK;
}


MultiByteCode VIntlMgr::DialectCodeToScriptCode(DialectCode inDialect)
{
	MultiByteCode	result;
	
	switch(inDialect)
	{
	case DC_AFRIKAANS:
		result = SC_ROMAN;
		break;
		
	case DC_ALBANIAN:
		result = SC_ROMAN;
		break;
		
	case DC_ARABIC_SAUDI_ARABIA:
		result = SC_ARABIC;
		break;
		
	case DC_ARABIC_IRAQ:
		result = SC_ARABIC;
		break;
		
	case DC_ARABIC_EGYPT:
		result = SC_ARABIC;
		break;
		
	case DC_ARABIC_LIBYA:
		result = SC_ARABIC;
		break;
		
	case DC_ARABIC_ALGERIA:
		result = SC_ARABIC;
		break;
		
	case DC_ARABIC_MOROCCO:
		result = SC_ARABIC;
		break;
		
	case DC_ARABIC_TUNISIA:
		result = SC_ARABIC;
		break;
		
	case DC_ARABIC_OMAN:
		result = SC_ARABIC;
		break;
		
	case DC_ARABIC_YEMEN:
		result = SC_ARABIC;
		break;
		
	case DC_ARABIC_SYRIA:
		result = SC_ARABIC;
		break;
		
	case DC_ARABIC_JORDAN:
		result = SC_ARABIC;
		break;
		
	case DC_ARABIC_LEBANON:
		result = SC_ARABIC;
		break;
		
	case DC_ARABIC_KUWAIT:
		result = SC_ARABIC;
		break;
		
	case DC_ARABIC_UAE:
		result = SC_ARABIC;
		break;
		
	case DC_ARABIC_BAHRAIN:
		result = SC_ARABIC;
		break;
		
	case DC_ARABIC_QATAR:
		result = SC_ARABIC;
		break;
	
	case DC_AZERI_LATIN:
		result = SC_ROMAN;
		break;
	case DC_AZERI_CYRILLIC:
		result = SC_CYRILLIC;
		break;
			
	case DC_BASQUE:
		result = SC_ROMAN;
		break;
		
	case DC_BELARUSIAN:
		result = SC_CYRILLIC;
		break;
	
	case DC_BOSNIAN:
		result = SC_ROMAN;
		break;	
	
	case DC_BULGARIAN:
		result = SC_CYRILLIC;
		break;
		
	case DC_CATALAN:
		result = SC_ROMAN;
		break;
		
	case DC_CHINESE_TRADITIONAL:
		result = SC_TRADCHINESE;
		break;
		
	case DC_CHINESE_SIMPLIFIED:
		result = SC_SIMPCHINESE;
		break;
		
	case DC_CHINESE_HONGKONG:
		result = SC_SIMPCHINESE;
		break;
		
	case DC_CHINESE_SINGAPORE:
		result = SC_TRADCHINESE;
		break;
		
	case DC_CROATIAN:
	case DC_CROATIAN_BOSNIA_HERZEGOVINA:
		result = SC_CROATIAN;
		break;
		
	case DC_CZECH:
		result = SC_CENTRALEUROROMAN;
		break;
		
	case DC_DANISH:
		result = SC_ROMAN;
		break;
		
	case DC_FRISIAN:
	case DC_DUTCH:
		result = SC_ROMAN;
		break;
		
	case DC_DUTCH_BELGIAN:
		result = SC_ROMAN;
		break;
		
	case DC_ENGLISH_US:
		result = SC_ROMAN;
		break;
		
	case DC_ENGLISH_UK:
		result = SC_ROMAN;
		break;
		
	case DC_ENGLISH_AUSTRALIA:
		result = SC_ROMAN;
		break;
		
	case DC_ENGLISH_CANADA:
		result = SC_ROMAN;
		break;
		
	case DC_ENGLISH_NEWZEALAND:
		result = SC_ROMAN;
		break;
		
	case DC_ENGLISH_EIRE:
		result = SC_ROMAN;
		break;
		
	case DC_ENGLISH_SOUTH_AFRICA:
		result = SC_ROMAN;
		break;
		
	case DC_ENGLISH_JAMAICA:
		result = SC_ROMAN;
		break;
		
	case DC_ENGLISH_CARIBBEAN:
		result = SC_ROMAN;
		break;
		
	case DC_ENGLISH_BELIZE:
		result = SC_ROMAN;
		break;
		
	case DC_ENGLISH_TRINIDAD:
		result = SC_ROMAN;
		break;
		
	case DC_ESTONIAN:
		result = SC_CENTRALEUROROMAN;
		break;
		
	case DC_FAEROESE:
		result = SC_ICELANDIC;
		break;
		
	case DC_FARSI:
		result = SC_FARSI;
		break;
		
	case DC_FINNISH:
		result = SC_ROMAN;
		break;
		
	case DC_FRENCH:
		result = SC_ROMAN;
		break;
		
	case DC_FRENCH_BELGIAN:
		result = SC_ROMAN;
		break;
		
	case DC_FRENCH_CANADIAN:
		result = SC_ROMAN;
		break;
		
	case DC_FRENCH_SWISS:
		result = SC_ROMAN;
		break;
		
	case DC_FRENCH_LUXEMBOURG:
		result = SC_ROMAN;
		break;
		
	case DC_GERMAN:
	case DC_GERMAN_SWISS:
	case DC_GERMAN_AUSTRIAN:
	case DC_GERMAN_LUXEMBOURG:
	case DC_GERMAN_LIECHTENSTEIN:
	case DC_LOWER_SORBIAN:
	case DC_UPPER_SORBIAN:
		result = SC_ROMAN;
		break;
		
	case DC_GREEK:
		result = SC_GREEK;
		break;
		
	case DC_HEBREW:
		result = SC_HEBREW;
		break;
		
	case DC_HUNGARIAN:
		result = SC_CENTRALEUROROMAN;
		break;
		
	case DC_ICELANDIC:
		result = SC_ICELANDIC;
		break;
		
	case DC_INDONESIAN:
		result = SC_ROMAN;
		break;
		
	case DC_ITALIAN:
		result = SC_ROMAN;
		break;
		
	case DC_ITALIAN_SWISS:
		result = SC_ROMAN;
		break;
		
	case DC_JAPANESE:
		result = SC_JAPANESE;
		break;
		
	case DC_KOREAN_WANSUNG:
		result = SC_KOREAN;
		break;
		
	case DC_KOREAN_JOHAB:
		result = SC_KOREAN;
		break;
		
	case DC_LATVIAN:
		result = SC_CENTRALEUROROMAN;
		break;
		
	case DC_LITHUANIAN:
		result = SC_CENTRALEUROROMAN;
		break;
	
	case DC_MACEDONIAN:
		result = SC_CYRILLIC;
		break;
	
	case DC_MONGOLIAN:
		result = SC_CYRILLIC;
		break;	
	
	case DC_NORWEGIAN:
		result = SC_ROMAN;
		break;
		
	case DC_NORWEGIAN_NYNORSK:
		result = SC_ROMAN;
		break;
		
	case DC_POLISH:
		result = SC_CENTRALEUROROMAN;
		break;
		
	case DC_PORTUGUESE:
		result = SC_ROMAN;
		break;
		
	case DC_PORTUGUESE_BRAZILIAN:
		result = SC_ROMAN;
		break;
		
	case DC_ROMANIAN:
		result = SC_ROMANIAN;
		break;
		
	case DC_RUSSIAN:
		result = SC_CYRILLIC;
		break;
		
	case DC_SERBIAN_LATIN:
	case DC_SERBIAN_LATIN_BOSNIAN:
		result = SC_CENTRALEUROROMAN;
		break;
	
	case DC_SERBIAN_CYRILLIC:
	case DC_SERBIAN_CYRILLIC_BOSNIAN:
		result = SC_CYRILLIC;
		break;
		
	case DC_SLOVAK:
		result = SC_CENTRALEUROROMAN;
		break;
		
	case DC_SLOVENIAN:
		result = SC_CROATIAN;
		break;
		
	case DC_SPANISH_CASTILLAN:
		result = SC_ROMAN;
		break;
		
	case DC_SPANISH_MEXICAN:
		result = SC_ROMAN;
		break;
		
	case DC_SPANISH_MODERN:
		result = SC_ROMAN;
		break;
		
	case DC_SPANISH_GUATEMALA:
		result = SC_ROMAN;
		break;
		
	case DC_SPANISH_COSTA_RICA:
		result = SC_ROMAN;
		break;
		
	case DC_SPANISH_PANAMA:
		result = SC_ROMAN;
		break;
		
	case DC_SPANISH_DOMINICAN_REPUBLIC:
		result = SC_ROMAN;
		break;
		
	case DC_SPANISH_VENEZUELA:
		result = SC_ROMAN;
		break;
		
	case DC_SPANISH_COLOMBIA:
		result = SC_ROMAN;
		break;
		
	case DC_SPANISH_PERU:
		result = SC_ROMAN;
		break;
		
	case DC_SPANISH_ARGENTINA:
		result = SC_ROMAN;
		break;
		
	case DC_SPANISH_ECUADOR:
		result = SC_ROMAN;
		break;
		
	case DC_SPANISH_CHILE:
		result = SC_ROMAN;
		break;
		
	case DC_SPANISH_URUGUAY:
		result = SC_ROMAN;
		break;
		
	case DC_SPANISH_PARAGUAY:
		result = SC_ROMAN;
		break;
		
	case DC_SPANISH_BOLIVIA:
		result = SC_ROMAN;
		break;
		
	case DC_SPANISH_EL_SALVADOR:
		result = SC_ROMAN;
		break;
		
	case DC_SPANISH_HONDURAS:
		result = SC_ROMAN;
		break;
		
	case DC_SPANISH_NICARAGUA:
		result = SC_ROMAN;
		break;
		
	case DC_SPANISH_PUERTO_RICO:
		result = SC_ROMAN;
		break;
		
	case DC_SWEDISH:
		result = SC_ROMAN;
		break;
		
	case DC_SWEDISH_FINLAND:
		result = SC_ROMAN;
		break;
	case DC_TADJIK:
		result = SC_CYRILLIC;
		break;
	case DC_TATAR:
		result = SC_CYRILLIC;
		break;
	case DC_THAI:
		result = SC_THAI;
		break;
		
	case DC_TURKISH:
		result = SC_TURKISH;
		break;
		
	case DC_UKRAINIAN:
		result = SC_CYRILLIC;
		break;

	case DC_UZBEK_LATIN:
		result = SC_CENTRALEUROROMAN;
		break;

	case DC_UZBEK_CYRILLIC:
		result = SC_CYRILLIC;
		break;	

	case DC_VIETNAMESE:
		result = SC_VIETNAMESE;
		break;

	case DC_SAMI_NORTHERN_NORWAY:
	case DC_SAMI_NORTHERN_SWEDEN:
	case DC_SAMI_NORTHERN_FINLAND:
	case DC_SAMI_LULE_NORWAY:
	case DC_SAMI_LULE_SWEDEN:
	case DC_SAMI_SOUTHERN_NORWAY:
	case DC_SAMI_SOUTHERN_SWEDEN:
	case DC_SAMI_SKOLT_FINLAND:
	case DC_SAMI_INARI_FINLAND:
		result = SC_ROMAN;
		break;	
	case DC_MALAYSIAN:
		result = SC_ROMAN;
		break;
	default:
		xbox_assert(false);
		result = SC_UNKNOWN;
		break;
	}
	
	return result;
}


#if VERSIONMAC

DialectCode	VIntlMgr::MAC_LanguageCodeToDialectCode(sLONG inLanguageCode)
{
	DialectCode	dialectCode;
	switch (inLanguageCode)
	{
		case langEnglish:
			dialectCode = DC_ENGLISH_US;
			break;
			
		case langFrench:
			dialectCode = DC_FRENCH;
			break;
			
		case langGerman:
			dialectCode = DC_GERMAN;
			break;
			
		case langItalian:
			dialectCode = DC_ITALIAN;
			break;
			
		case langDutch:
			dialectCode = DC_DUTCH;
			break;
			
		case langSwedish:
			dialectCode = DC_SWEDISH;
			break;
			
		case langSpanish:
			dialectCode = DC_SPANISH_MODERN;
			break;
			
		case langDanish:
			dialectCode = DC_DANISH;
			break;
			
		case langPortuguese:
			dialectCode = DC_PORTUGUESE;
			break;
			
		case langNorwegian:
			dialectCode = DC_NORWEGIAN;
			break;
			
		case langHebrew:
			dialectCode = DC_HEBREW;
			break;
		case langJapanese:
			dialectCode = DC_JAPANESE;
			break;
			
		case langArabic:
			dialectCode = DC_ARABIC_SAUDI_ARABIA;
			break;
			
		case langFinnish:
			dialectCode = DC_FINNISH;
			break;
			
		case langGreek:
			dialectCode = DC_GREEK;
			break;
			
		case langIcelandic:
			dialectCode = DC_ICELANDIC;
			break;
			
		case langTurkish:
			dialectCode = DC_TURKISH;
			break;
			
		case langCroatian:
			dialectCode = DC_CROATIAN;
			break;
			
		case langTradChinese:
			dialectCode = DC_CHINESE_TRADITIONAL;
			break;
			
		case langThai:
			dialectCode = DC_THAI;
			break;
			
		case langKorean:
			dialectCode = DC_KOREAN_WANSUNG;
			break;
			
		case langLithuanian:
			dialectCode = DC_LITHUANIAN;
			break;
			
		case langPolish:
			dialectCode = DC_POLISH;
			break;
			
		case langHungarian:
			dialectCode = DC_HUNGARIAN;
			break;
			
		case langLatvian:
			dialectCode = DC_LATVIAN;
			break;
			
		case langFaroese:
			dialectCode = DC_FAEROESE;
			break;
			
		case langFarsi:
			dialectCode = DC_FARSI;
			break;
			
		case langRussian:
			dialectCode = DC_RUSSIAN;
			break;
			
		case langSimpChinese:
			dialectCode = DC_CHINESE_SIMPLIFIED;
			break;
			
		case langAlbanian:
			dialectCode = DC_ALBANIAN;
			break;
			
		case langRomanian:
			dialectCode = DC_ROMANIAN;
			break;
			
		case langCzech:
			dialectCode = DC_CZECH;
			break;
			
		case langSlovak:
			dialectCode = DC_SLOVAK;
			break;
			
		case langSlovenian:
			dialectCode = DC_SLOVENIAN;
			break;
			
		case langSerbian:
			dialectCode = DC_SERBIAN_LATIN;
			break;
			
		case langBulgarian:
			dialectCode = DC_BULGARIAN;
			break;
			
		case langUkrainian:
			dialectCode = DC_UKRAINIAN;
			break;
			
		case langByelorussian:
			dialectCode = DC_BELARUSIAN;
			break;
			
		case langBasque:
			dialectCode = DC_BASQUE;
			break;
			
		case langCatalan:
			dialectCode = DC_CATALAN;
			break;
			
		case langAfrikaans:
			dialectCode = DC_AFRIKAANS;
			break;
			
		case langVietnamese:
			dialectCode = DC_VIETNAMESE;
			break;
			
		case langIndonesian:
			dialectCode = DC_INDONESIAN;
			break;
			
		case langEstonian:
		case langSami:
		case langFlemish:
		case langIrishGaelic:
		case langYiddish:
		case langMacedonian:
		case langUzbek:
		case langKazakh:
		case langAzerbaijani:
		case langAzerbaijanAr:
		case langArmenian:
		case langGeorgian:
		case langMoldavian:
		case langKirghiz:
		case langTajiki:
		case langTurkmen:
		case langMongolian:
		case langMongolianCyr:
		case langPashto:
		case langKurdish:
		case langKashmiri:
		case langSindhi:
		case langTibetan:
		case langNepali:
		case langSanskrit:
		case langMarathi:
		case langBengali:
		case langAssamese:
		case langGujarati:
		case langPunjabi:
		case langOriya:
		case langMalayalam:
		case langKannada:
		case langTamil:
		case langTelugu:
		case langSinhalese:
		case langBurmese:
		case langKhmer:
		case langLao:
		case langTagalog:
		case langMalayRoman:
		case langMalayArabic:
		case langAmharic:
		case langTigrinya:
		case langOromo:
		case langSomali:
		case langSwahili:
		case langKinyarwanda:
		case langRundi:
		case langNyanja:
		case langMalagasy:
		case langEsperanto:
		case langWelsh:
		case langLatin:
		case langQuechua:
		case langGuarani:
		case langAymara:
		case langTatar:
		case langUighur:
		case langDzongkha:
		case langJavaneseRom:
		case langSundaneseRom:
		case langGalician:
		case langBreton:
		case langInuktitut:
		case langScottishGaelic:
		case langManxGaelic:
		case langIrishGaelicScript:
		case langTongan:					
		case langGreekPoly:			
		case langMaltese:
		case langUrdu:
		case langHindi:
		default:
			assert(false);
			dialectCode = DC_ENGLISH_US;
			break;
	}
	
	return dialectCode;
}
#endif	// VERSIONMAC



void VIntlMgr::GetStringComparisonAlgorithmSignature( VString& outSignature) const
{
	fCollator->GetStringComparisonAlgorithmSignature( outSignature);
}


void VIntlMgr::GetKewordAlgorithmSignature( VString& outSignature) const
{
	if (fKeyWordOptions & eKW_ConsiderOnlyDeadChars)
	{
		outSignature = CVSTR( "AlphaNum");	// algo is stable regarding icu
	}
	else if (fKeyWordOptions & eKW_UseMeCab)
	{
		#if USE_MECAB
		IMecabModel *model = _GetMecabModel();
		if (testAssert( model != NULL))
		{
			outSignature.Printf( "mecab:%s;%s", model->GetVersion(), model->GetSignature());
		}
		#else
		xbox_assert( false);
		#endif
	}
	else if (fKeyWordOptions & eKW_UseICU)
	{
		#if USE_ICU
		outSignature.Printf( "icu:%d.%d.%d", U_ICU_VERSION_MAJOR_NUM, U_ICU_VERSION_MINOR_NUM, U_ICU_VERSION_PATCHLEVEL_NUM);
		#else
		xbox_assert( false);
		#endif
	}
	else
		outSignature.Clear();
}


#if VERSIONWIN && VERSIONDEBUG

void VIntlMgr::WIN_BuildFilesFromResources()
{
	// Ce code est a appeler pour creer des fichiers binaires contenant l'equivalent
	// des ressources de conversion unicode, mais deja swappees.
	// ces ressources devront ensuite etre integrees aux applications runtime sous
	// forme de vraies ressources Windows.
	// Cela permet d'initialiser VIntlMgr avant les autres managers (en particulier VResFile)

	uLONG	kindList[] = { 'macu', 'umac', 'winu', 'uwin', 'lwrn', 'lwra', 'uprn', 'upra', 'uset', 'sort', 0 };
	uLONG*	kindPtr = kindList;

	VResourceFile*	resFile = VProcess::Get()->RetainResourceFile();
	while (*kindPtr)
	{
		VHandle resHandle;
		VResourceIterator* iterator = resFile->NewIterator(VResTypeString(*kindPtr));
		while ((resHandle = iterator->Next()) != NULL)
		{
			VPtr	ptr = VMemory::LockHandle(resHandle);
			VSize	length = VMemory::GetHandleSize(resHandle);

			VStr31 resTypeStr;
			resTypeStr.FromOsType(*kindPtr);
			resTypeStr.AppendLong(iterator->GetCurrentResourceID());

			VFile file(resTypeStr);
			file.Open( FA_READ_WRITE, NULL, true );
			
			VFileStream	stream(&file);
			if (VE_OK == stream.OpenWriting())
			{
				stream.PutData(ptr, length);
				stream.CloseWriting();
			}
			
			VMemory::DisposeHandle(resHandle);
		}
		
		delete iterator;
		kindPtr++;
	}
	
	resFile->Release();
}

#endif	// VERSIONWIN && VERSIONDEBUG



void VIntlMgr::ICU_Cleanup()
{
	#if USE_ICU
	// Clean up the system resources, such as allocated memory or open files, used in all ICU libraries
	// warning: `u_cleanup` is not thread safe
	//
	// The use of u_cleanup() just before an application terminates is optional, but
	// it should be called only once for performance reasons. The primary benefit is
	// to eliminate reports of memory or resource leaks originating in ICU code from
	// the results generated by heap analysis tools.
	if (ICU_Available())
		u_cleanup();
	#endif
}


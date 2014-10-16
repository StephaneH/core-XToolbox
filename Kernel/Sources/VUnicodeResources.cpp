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
#include "VUnicodeResources.h"
#include "VUnicodeRanges.h"
#include "VUnicodeTableFull.h"
#include "VIntlMgr.h"
#include "VMemory.h"


BEGIN_TOOLBOX_NAMESPACE

#define	PATCH_EURO 1	// permet de forcer la conversion du symbole euro (non géré par ::WideCharToMultibyte)
#define	PATCH_SOFTHYPHEN 1	// permet de forcer la conversion du symbole SoftHyphen en tiret sur Mac.

#if VERSIONWIN
static LCID		sDebugCurrentLCID = 0;
static sWORD	sDebugMaxDBCSSize = 0;
#endif



#if VERSIONWIN


void _AddUnicodeResource(VHandle inData, OsType inType, sLONG inResID, char* inResName)
{
	VSize size = VMemory::GetHandleSize(inData);
	VPtr data = VMemory::LockHandle(inData);
	char name[255];
	char type[4];
	type[0] = ((char*)&inType)[3];
	type[1] = ((char*)&inType)[2];
	type[2] = ((char*)&inType)[1];
	type[3] = ((char*)&inType)[0];
	sprintf(name, "%.4s%d", &type, inResID);
	HANDLE theFile = ::CreateFileA(name, GENERIC_WRITE | GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, 0); 
	if (theFile != NULL) {
		DWORD wrote = 0;
		::WriteFile(theFile, data, (DWORD) size, &wrote, 0);
		::CloseHandle(theFile);
	}
	VMemory::UnlockHandle(inData);
}


int __cdecl WIN_SortUnichar(const void* p1, const void* p2);
int __cdecl WIN_SortUnichar(const void* p1, const void* p2)
{
	int result;
	
	result = ::CompareStringW(sDebugCurrentLCID, NORM_IGNOREKANATYPE, (LPCWSTR)p1, 1, (LPCWSTR)p2, 1);
	if (result)
		result -= 2;
	return result-2;
}


int __cdecl WIN_SortDBCS(const void* p1, const void* p2);
int __cdecl WIN_SortDBCS(const void* p1, const void* p2)
{
	sWORD	i;
	
	for (i=0;i<sDebugMaxDBCSSize;i++)
	{
		if (	*((uBYTE*)p1+i+2) > *((uBYTE*)p2+i+2))
			return 1;
		if (	*((uBYTE*)p1+i+2) < *((uBYTE*)p2+i+2))
			return -1;
	}
	return 0;		
}


void WIN_GetDialectLangSortCodes(DialectCode inDialect, uLONG& outLanguage, uLONG& outSubLanguage, uLONG& outSortCode)
{
	switch(inDialect)
	{
	case DC_AFRIKAANS:
		outLanguage = LANG_AFRIKAANS;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ALBANIAN:
		outLanguage = LANG_ALBANIAN;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ARABIC_SAUDI_ARABIA:
		outLanguage = LANG_ARABIC;
		outSubLanguage = SUBLANG_ARABIC_SAUDI_ARABIA;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ARABIC_IRAQ:
		outLanguage = LANG_ARABIC;
		outSubLanguage = SUBLANG_ARABIC_IRAQ;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ARABIC_EGYPT:
		outLanguage = LANG_ARABIC;
		outSubLanguage = SUBLANG_ARABIC_EGYPT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ARABIC_LIBYA:
		outLanguage = LANG_ARABIC;
		outSubLanguage = SUBLANG_ARABIC_LIBYA;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ARABIC_ALGERIA:
		outLanguage = LANG_ARABIC;
		outSubLanguage = SUBLANG_ARABIC_ALGERIA;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ARABIC_MOROCCO:
		outLanguage = LANG_ARABIC;
		outSubLanguage = SUBLANG_ARABIC_MOROCCO;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ARABIC_TUNISIA:
		outLanguage = LANG_ARABIC;
		outSubLanguage = SUBLANG_ARABIC_TUNISIA;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ARABIC_OMAN:
		outLanguage = LANG_ARABIC;
		outSubLanguage = SUBLANG_ARABIC_OMAN;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ARABIC_YEMEN:
		outLanguage = LANG_ARABIC;
		outSubLanguage = SUBLANG_ARABIC_YEMEN;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ARABIC_SYRIA:
		outLanguage = LANG_ARABIC;
		outSubLanguage = SUBLANG_ARABIC_SYRIA;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ARABIC_JORDAN:
		outLanguage = LANG_ARABIC;
		outSubLanguage = SUBLANG_ARABIC_JORDAN;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ARABIC_LEBANON:
		outLanguage = LANG_ARABIC;
		outSubLanguage = SUBLANG_ARABIC_LEBANON;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ARABIC_KUWAIT:
		outLanguage = LANG_ARABIC;
		outSubLanguage = SUBLANG_ARABIC_KUWAIT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ARABIC_UAE:
		outLanguage = LANG_ARABIC;
		outSubLanguage = SUBLANG_ARABIC_UAE;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ARABIC_BAHRAIN:
		outLanguage = LANG_ARABIC;
		outSubLanguage = SUBLANG_ARABIC_BAHRAIN;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ARABIC_QATAR:
		outLanguage = LANG_ARABIC;
		outSubLanguage = SUBLANG_ARABIC_QATAR;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_BASQUE:
		outLanguage = LANG_BASQUE;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_BELARUSIAN:
		outLanguage = LANG_BELARUSIAN;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_BULGARIAN:
		outLanguage = LANG_BULGARIAN;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_CATALAN:
		outLanguage = LANG_CATALAN;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_CHINESE_TRADITIONAL:
		outLanguage = LANG_CHINESE;
		outSubLanguage = SUBLANG_CHINESE_TRADITIONAL;
		outSortCode = SORT_CHINESE_UNICODE;
		break;
	case DC_CHINESE_SIMPLIFIED:
		outLanguage = LANG_CHINESE;
		outSubLanguage = SUBLANG_CHINESE_SIMPLIFIED;
		outSortCode = SORT_CHINESE_UNICODE;
		break;
	case DC_CHINESE_HONGKONG:
		outLanguage = LANG_CHINESE;
		outSubLanguage = SUBLANG_CHINESE_HONGKONG;
		outSortCode = SORT_CHINESE_UNICODE;
		break;
	case DC_CHINESE_SINGAPORE:
		outLanguage = LANG_CHINESE;
		outSubLanguage = SUBLANG_CHINESE_SINGAPORE;
		outSortCode = SORT_CHINESE_UNICODE;
		break;
	case DC_CROATIAN:
		outLanguage = LANG_CROATIAN;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_CZECH:
		outLanguage = LANG_CZECH;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_DANISH:
		outLanguage = LANG_DANISH;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_DUTCH:
		outLanguage = LANG_DUTCH;
		outSubLanguage = SUBLANG_DUTCH;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_DUTCH_BELGIAN:
		outLanguage = LANG_DUTCH;
		outSubLanguage = SUBLANG_DUTCH_BELGIAN;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ENGLISH_US:
		outLanguage = LANG_ENGLISH;
		outSubLanguage = SUBLANG_ENGLISH_US;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ENGLISH_UK:
		outLanguage = LANG_ENGLISH;
		outSubLanguage = SUBLANG_ENGLISH_UK;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ENGLISH_AUSTRALIA:
		outLanguage = LANG_ENGLISH;
		outSubLanguage = SUBLANG_ENGLISH_AUS;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ENGLISH_CANADA:
		outLanguage = LANG_ENGLISH;
		outSubLanguage = SUBLANG_ENGLISH_CAN;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ENGLISH_NEWZEALAND:
		outLanguage = LANG_ENGLISH;
		outSubLanguage = SUBLANG_ENGLISH_NZ;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ENGLISH_EIRE:
		outLanguage = LANG_ENGLISH;
		outSubLanguage = SUBLANG_ENGLISH_EIRE;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ENGLISH_SOUTH_AFRICA:
		outLanguage = LANG_ENGLISH;
		outSubLanguage = SUBLANG_ENGLISH_SOUTH_AFRICA;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ENGLISH_JAMAICA:
		outLanguage = LANG_ENGLISH;
		outSubLanguage = SUBLANG_ENGLISH_JAMAICA;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ENGLISH_CARIBBEAN:
		outLanguage = LANG_ENGLISH;
		outSubLanguage = SUBLANG_ENGLISH_CARIBBEAN;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ENGLISH_BELIZE:
		outLanguage = LANG_ENGLISH;
		outSubLanguage = SUBLANG_ENGLISH_BELIZE;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ENGLISH_TRINIDAD:
		outLanguage = LANG_ENGLISH;
		outSubLanguage = SUBLANG_ENGLISH_TRINIDAD;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ESTONIAN:
		outLanguage = LANG_ESTONIAN;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_FAEROESE:
		outLanguage = LANG_FAEROESE;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_FARSI:
		outLanguage = LANG_FARSI;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_FINNISH:
		outLanguage = LANG_FINNISH;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_FRENCH:
		outLanguage = LANG_FRENCH;
		outSubLanguage = SUBLANG_FRENCH;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_FRENCH_BELGIAN:
		outLanguage = LANG_FRENCH;
		outSubLanguage = SUBLANG_FRENCH_BELGIAN;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_FRENCH_CANADIAN:
		outLanguage = LANG_FRENCH;
		outSubLanguage = SUBLANG_FRENCH_CANADIAN;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_FRENCH_SWISS:
		outLanguage = LANG_FRENCH;
		outSubLanguage = SUBLANG_FRENCH_SWISS;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_FRENCH_LUXEMBOURG:
		outLanguage = LANG_FRENCH;
		outSubLanguage = SUBLANG_FRENCH_LUXEMBOURG;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_GERMAN:
		outLanguage = LANG_GERMAN;
		outSubLanguage = SUBLANG_GERMAN;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_GERMAN_SWISS:
		outLanguage = LANG_GERMAN;
		outSubLanguage = SUBLANG_GERMAN_SWISS;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_GERMAN_AUSTRIAN:
		outLanguage = LANG_GERMAN;
		outSubLanguage = SUBLANG_GERMAN_AUSTRIAN;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_GERMAN_LUXEMBOURG:
		outLanguage = LANG_GERMAN;
		outSubLanguage = SUBLANG_GERMAN_LUXEMBOURG;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_GERMAN_LIECHTENSTEIN:
		outLanguage = LANG_GERMAN;
		outSubLanguage = SUBLANG_GERMAN_LIECHTENSTEIN;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_GREEK:
		outLanguage = LANG_GREEK;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_HEBREW:
		outLanguage = LANG_HEBREW;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_HUNGARIAN:
		outLanguage = LANG_HUNGARIAN;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ICELANDIC:
		outLanguage = LANG_ICELANDIC;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_INDONESIAN:
		outLanguage = LANG_INDONESIAN;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ITALIAN:
		outLanguage = LANG_ITALIAN;
		outSubLanguage = SUBLANG_ITALIAN;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ITALIAN_SWISS:
		outLanguage = LANG_ITALIAN;
		outSubLanguage = SUBLANG_ITALIAN_SWISS;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_JAPANESE:
		outLanguage = LANG_JAPANESE;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_JAPANESE_UNICODE;
		break;
	case DC_KOREAN_WANSUNG:
		outLanguage = LANG_KOREAN;
		outSubLanguage = SUBLANG_KOREAN;
		outSortCode = SORT_KOREAN_UNICODE;
		break;
	case DC_KOREAN_JOHAB:
		outLanguage = LANG_KOREAN;
	// phg 21 apr 1999.
	// Visual C++ 6.0 compatibility.
	#if (_MSC_VER >= 1200) || !COMPIL_VISUAL
		outSubLanguage = SUBLANG_KOREAN;
	#else
		outSubLanguage = SUBLANG_KOREAN_JOHAB;
	#endif
		outSortCode = SORT_KOREAN_UNICODE;
		break;
	case DC_LATVIAN:
		outLanguage = LANG_LATVIAN;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_LITHUANIAN:
		outLanguage = LANG_LITHUANIAN;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_NORWEGIAN:
		outLanguage = LANG_NORWEGIAN;
		outSubLanguage = SUBLANG_NORWEGIAN_BOKMAL;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_NORWEGIAN_NYNORSK:
		outLanguage = LANG_NORWEGIAN;
		outSubLanguage = SUBLANG_NORWEGIAN_NYNORSK;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_POLISH:
		outLanguage = LANG_POLISH;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_PORTUGUESE:
		outLanguage = LANG_PORTUGUESE;
		outSubLanguage = SUBLANG_PORTUGUESE;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_PORTUGUESE_BRAZILIAN:
		outLanguage = LANG_PORTUGUESE;
		outSubLanguage = SUBLANG_PORTUGUESE_BRAZILIAN;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_ROMANIAN:
		outLanguage = LANG_ROMANIAN;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_RUSSIAN:
		outLanguage = LANG_RUSSIAN;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_SERBIAN_LATIN:
		outLanguage = LANG_SERBIAN;
		outSubLanguage = SUBLANG_SERBIAN_LATIN;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_SERBIAN_CYRILLIC:
		outLanguage = LANG_SERBIAN;
		outSubLanguage = SUBLANG_SERBIAN_CYRILLIC;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_SLOVAK:
		outLanguage = LANG_SLOVAK;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_SLOVENIAN:
		outLanguage = LANG_SLOVENIAN;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_SPANISH_CASTILLAN:
		outLanguage = LANG_SPANISH;
		outSubLanguage = SUBLANG_SPANISH;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_SPANISH_MEXICAN:
		outLanguage = LANG_SPANISH;
		outSubLanguage = SUBLANG_SPANISH_MEXICAN;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_SPANISH_MODERN:
		outLanguage = LANG_SPANISH;
		outSubLanguage = SUBLANG_SPANISH_MODERN;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_SPANISH_GUATEMALA:
		outLanguage = LANG_SPANISH;
		outSubLanguage = SUBLANG_SPANISH_GUATEMALA;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_SPANISH_COSTA_RICA:
		outLanguage = LANG_SPANISH;
		outSubLanguage = SUBLANG_SPANISH_COSTA_RICA;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_SPANISH_PANAMA:
		outLanguage = LANG_SPANISH;
		outSubLanguage = SUBLANG_SPANISH_PANAMA;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_SPANISH_DOMINICAN_REPUBLIC:
		outLanguage = LANG_SPANISH;
		outSubLanguage = SUBLANG_SPANISH_DOMINICAN_REPUBLIC;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_SPANISH_VENEZUELA:
		outLanguage = LANG_SPANISH;
		outSubLanguage = SUBLANG_SPANISH_VENEZUELA;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_SPANISH_COLOMBIA:
		outLanguage = LANG_SPANISH;
		outSubLanguage = SUBLANG_SPANISH_COLOMBIA;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_SPANISH_PERU:
		outLanguage = LANG_SPANISH;
		outSubLanguage = SUBLANG_SPANISH_PERU;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_SPANISH_ARGENTINA:
		outLanguage = LANG_SPANISH;
		outSubLanguage = SUBLANG_SPANISH_ARGENTINA;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_SPANISH_ECUADOR:
		outLanguage = LANG_SPANISH;
		outSubLanguage = SUBLANG_SPANISH_ECUADOR;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_SPANISH_CHILE:
		outLanguage = LANG_SPANISH;
		outSubLanguage = SUBLANG_SPANISH_CHILE;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_SPANISH_URUGUAY:
		outLanguage = LANG_SPANISH;
		outSubLanguage = SUBLANG_SPANISH_URUGUAY;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_SPANISH_PARAGUAY:
		outLanguage = LANG_SPANISH;
		outSubLanguage = SUBLANG_SPANISH_PARAGUAY;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_SPANISH_BOLIVIA:
		outLanguage = LANG_SPANISH;
		outSubLanguage = SUBLANG_SPANISH_BOLIVIA;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_SPANISH_EL_SALVADOR:
		outLanguage = LANG_SPANISH;
		outSubLanguage = SUBLANG_SPANISH_EL_SALVADOR;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_SPANISH_HONDURAS:
		outLanguage = LANG_SPANISH;
		outSubLanguage = SUBLANG_SPANISH_HONDURAS;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_SPANISH_NICARAGUA:
		outLanguage = LANG_SPANISH;
		outSubLanguage = SUBLANG_SPANISH_NICARAGUA;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_SPANISH_PUERTO_RICO:
		outLanguage = LANG_SPANISH;
		outSubLanguage = SUBLANG_SPANISH_PUERTO_RICO;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_SWEDISH:
		outLanguage = LANG_SWEDISH;
		outSubLanguage = SUBLANG_SWEDISH;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_SWEDISH_FINLAND:
		outLanguage = LANG_SWEDISH;
		outSubLanguage = SUBLANG_SWEDISH_FINLAND;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_THAI:
		outLanguage = LANG_THAI;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_TURKISH:
		outLanguage = LANG_TURKISH;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_UKRAINIAN:
		outLanguage = LANG_UKRAINIAN;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	case DC_VIETNAMESE:
		outLanguage = LANG_VIETNAMESE;
		outSubLanguage = SUBLANG_DEFAULT;
		outSortCode = SORT_DEFAULT;
		break;
	default:
		assert(false);
		break;
	}	
}

static uLONG WIN_CharUpper( uLONG inChar)
{
	WCHAR c = (WCHAR) inChar;
	DWORD result = ::CharUpperBuffW( &c, 1);
	return c;
}

static uLONG WIN_CharLower( uLONG inChar)
{
	WCHAR c = (WCHAR) inChar;
	DWORD result = ::CharLowerBuffW( &c, 1);
	return c;
}

void WIN_BuildDialectResources(DialectCode	inDialect, sWORD inCount)
{
	uLONG								language;
	uLONG								sublanguage;
	uLONG								sortcode;
	sWORD								maxlen;
	sWORD								maxleneven;
	char								buffer[1024];
	sLONG								bufferlen;
	BOOL								unmapped;
	WCHAR								src[16];
	WCHAR								dst[16];
	SingleRangeBlock* 					range;
	MultiRangeBlock*					fullrange;
	SingleRangeBlock* 					diacsrange;
	MultiRangeBlock*					diacsfullrange;
	VHandle								diacsres;
	char*								curxxx;
	char*								data;
	char*								xxx2;
	char*								curxxx2;
	UniChar*							www;
	UniChar								curfirst;
	UniChar								curlast;
	uLONG								uchar, result;
	VSize								size, moresize;
	sLONG								count;
	LCID								curlocale, newlocale;
	CPINFO								cpinfo;
	VStr255								dialectname;
	uBYTE								languagename[256];
	uWORD								langcode;
	MultiByteCode						script;
	VHandle								res;
	VHandle								uwinResource = NULL;

	// set to required language
	WIN_GetDialectLangSortCodes(inDialect, language, sublanguage, sortcode);
	langcode = MAKELANGID(language, sublanguage);
	newlocale = MAKELCID(langcode, sortcode);
	sDebugCurrentLCID = newlocale;
	memset(buffer, 0, 1024);
	// retrieve language name
	bufferlen = ::GetLocaleInfoA(newlocale, LOCALE_NOUSEROVERRIDE | LOCALE_SENGLANGUAGE, buffer, 1024);
	buffer[bufferlen] = 0;
	bufferlen = ::GetLocaleInfoA(newlocale, LOCALE_NOUSEROVERRIDE | LOCALE_SENGCOUNTRY, buffer, 1024);
	memset(buffer, 0, 1024);
	curlocale = ::GetThreadLocale();

	if (!testAssert(::SetThreadLocale(newlocale)))
		return;
	
	// in order to avoid huge memory space
	// we use ranges of characters
	// all resources have the Dialect id ans MultiByteCode id
	
 	VPtr xxx = VMemory::NewPtr(0x00010002*sizeof(uWORD), 'dial'); // a general use buffer
	
	// we build a 'diac' resource for diacritic/non-diacritic conversions
	// the format of the resource is
	// uWORD : range count
	// followed for each range by
	// uWORD : first UniChar in range
	// uWORD : last UniChar in range
	// (last-first+1) sequences of UniChar
	if (inDialect == DC_ENGLISH_US) {
		res = VMemory::NewHandleClear(sizeof(sWORD));
		range = NULL;
		for (uchar=0;uchar<=0xFFFF;uchar++) {
			src[0] = uchar;
			size = ::FoldStringW(MAP_COMPOSITE, src, 1, dst, 16);
			if (size && src[0]!=dst[0]) {
				if (!range) { // entering a converting range
					range = (SingleRangeBlock*)xxx;
					range->firstchar = uchar;
				}
				range->lastchar = uchar;
				range->converts[uchar-range->firstchar] = dst[0];
			} else if (range) { // getting out of a converting range
				size = VMemory::GetHandleSize(res);
				moresize = sizeof(uWORD)+sizeof(uWORD)+(range->lastchar-range->firstchar+1)*sizeof(uWORD); // size of the range block header + conversions
				if (VMemory::SetHandleSize(res, size+moresize)) {
					fullrange = (MultiRangeBlock*)VMemory::LockHandle(res);
					fullrange->count++;
					VMemory::CopyBlock(xxx, ((char*)fullrange)+size, moresize);
					VMemory::UnlockHandle(res);
				}
				range = NULL;
			}
		}
		_AddUnicodeResource(res, 'diac', 0, "Unicode");
		diacsres = res; // we'll dispose it later on

		// we build a 'uprn' resource for upper conversions
		// the format of the resource is
		// uWORD : range count
		// followed for each range by
		// uWORD : first UniChar in range
		// uWORD : last UniChar in range
		// (last-first+1) sequences of UniChar
		res = VMemory::NewHandleClear(sizeof(sWORD));
		range = NULL;
		for (uchar=0;uchar<=0xFFFF;uchar++) {
			result = WIN_CharUpper( uchar);
			if (result!=uchar) {
				if (!range) { // entering a converting range
					range = (SingleRangeBlock*)xxx;
					range->firstchar = uchar;
				}
				range->lastchar = uchar;
				range->converts[uchar-range->firstchar] = result;
			} else if (range) { // getting out of a converting range
				size = VMemory::GetHandleSize(res);
				moresize = sizeof(uWORD)+sizeof(uWORD)+(range->lastchar-range->firstchar+1)*sizeof(uWORD); // size of the range block header + conversions
				if (VMemory::SetHandleSize(res, size+moresize)) {
					fullrange = (MultiRangeBlock*)VMemory::LockHandle(res);
					fullrange->count++;
					VMemory::CopyBlock(xxx, ((char*)fullrange)+size, moresize);
					VMemory::UnlockHandle(res);
				}
				range = NULL;
			}
		}
		_AddUnicodeResource(res, 'uprn', 0, "Unicode");
		VMemory::DisposeHandle(res);

		// we build a 'lwrn' resource for lower conversions
		// the format of the resource is
		// uWORD : range count
		// followed for each range by
		// uWORD : first UniChar in range
		// uWORD : last UniChar in range
		// (last-first+1) sequences of UniChar
		res = VMemory::NewHandleClear(sizeof(sWORD));
		range = NULL;
		for (uchar=0;uchar<=0xFFFF;uchar++) {
			result = WIN_CharLower( uchar);
			if (result!=uchar) {
				if (!range) { // entering a converting range
					range = (SingleRangeBlock*)xxx;
					range->firstchar = uchar;
				}
				range->lastchar = uchar;
				range->converts[uchar-range->firstchar] = result;
			} else if (range) { // getting out of a converting range
				size = VMemory::GetHandleSize(res);
				moresize = sizeof(uWORD)+sizeof(uWORD)+(range->lastchar-range->firstchar+1)*sizeof(uWORD); // size of the range block header + conversions
				if (VMemory::SetHandleSize(res, size+moresize)) {
					fullrange = (MultiRangeBlock*)VMemory::LockHandle(res);
					fullrange->count++;
					VMemory::CopyBlock(xxx, ((char*)fullrange)+size, moresize);
					VMemory::UnlockHandle(res);
				}
				range = NULL;
			}
		}
		_AddUnicodeResource(res, 'lwrn', 0, "Unicode");
		VMemory::DisposeHandle(res);

		// we load the diacritics conversion table for non-diacritical conversions
		diacsfullrange = (MultiRangeBlock*)VMemory::LockHandle(diacsres);

		// we build a 'upra' resource for upper non-diacritical conversions
		// the format of the resource is
		// uWORD : range count
		// followed for each range by
		// uWORD : first UniChar in range
		// uWORD : last UniChar in range
		// (last-first+1) sequences of UniChar
		res = VMemory::NewHandleClear(sizeof(sWORD));
		range = NULL;
		for (uchar=0;uchar<=0xFFFF;uchar++) {
			// find the non-diacritic corresponding character
			count = diacsfullrange->count;
			diacsrange = &diacsfullrange->ranges[0];
			result = uchar;
			while(count--) {
				if (uchar<diacsrange->firstchar)
					break;
				else if (uchar<=diacsrange->lastchar) {
					result = diacsrange->converts[uchar-diacsrange->firstchar];
					break;
				} else
					diacsrange = (SingleRangeBlock*)(((char*)diacsrange) + sizeof(uWORD) + sizeof(uWORD) + ((diacsrange->lastchar-diacsrange->firstchar+1)*sizeof(uWORD)));
			}
			// convert to uppercase
			result = WIN_CharUpper( result);
			if (result!=uchar) {
				if (!range) { // entering a converting range
					range = (SingleRangeBlock*)xxx;
					range->firstchar = uchar;
				}
				range->lastchar = uchar;
				range->converts[uchar-range->firstchar] = result;
			} else if (range) { // getting out of a converting range
				size = VMemory::GetHandleSize(res);
				moresize = sizeof(uWORD)+sizeof(uWORD)+(range->lastchar-range->firstchar+1)*sizeof(uWORD); // size of the range block header + conversions
				if (VMemory::SetHandleSize(res, size+moresize)) {
					fullrange = (MultiRangeBlock*)VMemory::LockHandle(res);
					fullrange->count++;
					VMemory::CopyBlock(xxx, ((char*)fullrange)+size, moresize);
					VMemory::UnlockHandle(res);
				}
				range = NULL;
			}
		}
		// the ACI implementation of this resource
		_AddUnicodeResource(res, 'upra', 0, "Unicode");
		VMemory::DisposeHandle(res);

		// we build a 'lwra' resource for lower non-diacritical conversions
		// the format of the resource is
		// uWORD : range count
		// followed for each range by
		// uWORD : first UniChar in range
		// uWORD : last UniChar in range
		// (last-first+1) sequences of UniChar
		res = VMemory::NewHandleClear(sizeof(sWORD));
		range = NULL;
		for (uchar=0;uchar<=0xFFFF;uchar++) {
			// find the non-diacritic corresponding character
			count = diacsfullrange->count;
			diacsrange = &diacsfullrange->ranges[0];
			result = uchar;
			while(count--) {
				if (uchar<diacsrange->firstchar)
					break;
				else if (uchar<=diacsrange->lastchar) {
					result = diacsrange->converts[uchar-diacsrange->firstchar];
					break;
				} else
					diacsrange = (SingleRangeBlock*)(((char*)diacsrange) + sizeof(uWORD) + sizeof(uWORD) + ((diacsrange->lastchar-diacsrange->firstchar+1)*sizeof(uWORD)));
			}
			// convert to lowercase
			result = WIN_CharLower( uchar);
			if (result!=uchar) {
				if (!range) { // entering a converting range
					range = (SingleRangeBlock*)xxx;
					range->firstchar = uchar;
				}
				range->lastchar = uchar;
				range->converts[uchar-range->firstchar] = result;
			} else if (range) { // getting out of a converting range
				size = VMemory::GetHandleSize(res);
				moresize = sizeof(uWORD)+sizeof(uWORD)+(range->lastchar-range->firstchar+1)*sizeof(uWORD); // size of the range block header + conversions
				if (VMemory::SetHandleSize(res, size+moresize)) {
					fullrange = (MultiRangeBlock*)VMemory::LockHandle(res);
					fullrange->count++;
					VMemory::CopyBlock(xxx, ((char*)fullrange)+size, moresize);
					VMemory::UnlockHandle(res);
				}
				range = NULL;
			}
		}
		_AddUnicodeResource(res, 'lwra', 0, "Unicode");
		VMemory::DisposeHandle(res);

		// clean-up
		VMemory::UnlockHandle(diacsres);
		VMemory::DisposeHandle(diacsres);
	}

	// now charset conversions
	script = VIntlMgr::DialectCodeToScriptCode(inDialect);
	if (script!=SC_UNKNOWN)
	{
		uLONG	wincodepage;
		uLONG	maccodepage;
		char*	scriptname;
		char n1[] = "us-european";
		char n2[] = "japanese";
		char n3[] = "chinese traditional";
		char n4[] = "chinese simplified";
		char n5[] = "korean";
		char n6[] = "arabic";
		char n7[] = "hebrew";
		char n8[] = "greek";
		char n9[] = "cyrillic";
		char n10[] = "eastern european";
		char n11[] = "croatian";
		char n12[] = "icelandic";
		char n13[] = "farsi";
		char n14[] = "romanian";
		char n15[] = "thai";
		char n16[] = "vietnamese";
		char n17[] = "turkish";

		switch(script)
		{
		case SC_ROMAN:
			wincodepage = 1252;
			maccodepage = 10000;
			scriptname = n1;
			break;
		case SC_JAPANESE:
			wincodepage = 932;
			maccodepage = 10001;
			scriptname = n2;
			break;
		case SC_TRADCHINESE:
			wincodepage = 936;
			maccodepage = 0;
			scriptname = n3;
			break;
		case SC_SIMPCHINESE:
			wincodepage = 950;
			maccodepage = 0;
			scriptname = n4;
			break;
		case SC_KOREAN:
			wincodepage = 949;
			maccodepage = 10003;
			scriptname = n5;
			break;
		case SC_ARABIC:
			wincodepage = 1256;
			maccodepage = 10004;
			scriptname = n6;
			break;
		case SC_HEBREW:
			wincodepage = 1255;
			maccodepage = 10005;
			scriptname = n7;
			break;
		case SC_GREEK:
			wincodepage = 1253;
			maccodepage = 10006;
			scriptname = n8;
			break;
		case SC_CYRILLIC:
			wincodepage = 1251;
			maccodepage = 10007;
			scriptname = n9;
			break;
		case SC_CENTRALEUROROMAN:
			wincodepage = 1250;
			maccodepage = 10029;
			scriptname = n10;
			break;
		case SC_CROATIAN:
			wincodepage = 0;
			maccodepage = 0;
			scriptname = n11;
			break;
		case SC_ICELANDIC:
			wincodepage = 0;
			maccodepage = 10079;
			scriptname = n12;
			break;
		case SC_FARSI:
			wincodepage = 0;
			maccodepage = 0;
			scriptname = n13;
			break;
		case SC_ROMANIAN:
			wincodepage = 0;
			maccodepage = 0;
			scriptname = n14;
			break;
		case SC_THAI:
			wincodepage = 874;
			maccodepage = 0;
			scriptname = n15;
			break;
		case SC_VIETNAMESE:
			wincodepage = 0;
			maccodepage = 0;
			scriptname = n16;
			break;
		case SC_TURKISH:
			wincodepage = 1254;
			maccodepage = 10081;
			scriptname = n17;
			break;
		default:
			wincodepage = 0;
			maccodepage = 0;
			break;
		}
		for (sWORD i=1;i<=2;i++)
		{
			uLONG	codepage, uwinkind, winukind;
			if (i==1)
			{
				codepage = wincodepage;
				uwinkind = 'uwin';
				winukind = 'winu';
			}
			else
			{
				codepage = maccodepage;
				uwinkind = 'umac';
				winukind = 'macu';
			}
			if (!codepage)
				continue;
			if (!::IsValidCodePage(codepage))
				continue;
			if (!::GetCPInfo(codepage, &cpinfo))
				continue;
			// we build a 'uwin' or 'umac' resource for UniCode to Ansi conversion
			// the format of the resource is:
			// sWORD : maxbytelength
			// sWORD : range count
			// followed for each range by
			// uWORD : first UniChar in range
			// uWORD : last UniChar in range
			// (last-first+1) sequences of maxbytelength
			// we also build a 'winu' or 'macu' resource for UniCode to Ansi conversion
			// the format of the resource is:
			// sWORD : maxbytelength
			// then if maxbytelength == 1
				// UniChar[256] conversions
			// otherwise:
				// sWORD : conversions count
				// followed for each conversion by
				// uWORD : UniChar result
				// uBYTE[] : maxbytelength padded bytes
			maxlen = cpinfo.MaxCharSize;
			maxleneven = (maxlen+1) & 0xFFFE;
			// storage for unicode to byte
			res = VMemory::NewHandleClear(2*sizeof(sWORD));
			data = VMemory::LockHandle(res);
			*(sWORD*)data = maxlen;
			VMemory::UnlockHandle(res);
			curxxx = 0;
			// storage for multibyte to unicode
			if (maxlen==1)
			{
				xxx2 = VMemory::NewPtr(256*sizeof(UniChar), 'dial');
				memset(xxx2, 0xFF, 512);
			}
			else
			{
				xxx2 = VMemory::NewPtr(0x10000*(maxleneven+sizeof(UniChar)), 'dial');
			}
			curxxx2 = xxx2;
			for (uchar=0;uchar<=0xFFFF;uchar++)
			{
				// convert to multi-byte string
				src[0] = uchar;

				// pour que les tables soient propres...
				buffer[1] = 0;
				#if PATCH_EURO
				// force la conversion euro
				if (uchar == 0x20AC)
				{
					unmapped = 0;
					result = 1;
					if (codepage == maccodepage)
						*(uBYTE*)buffer = 0xDB;
					else if (codepage == wincodepage)
						*(uBYTE*)buffer = 0x80;
				}
				else
				#endif
				#if PATCH_SOFTHYPHEN
				if (uchar == 0x00AD && codepage == maccodepage) {
					unmapped = 0;
					result = 1;
					*(uBYTE*)buffer = '-';
				}
				else
				#endif
				{
					result = ::WideCharToMultiByte(codepage, WC_COMPOSITECHECK, src, 1, buffer, 256, 0, &unmapped);
					#if PATCH_EURO
					// empeche d'autres code de retourner le symbole euro
					if (codepage == maccodepage)
					{
						if (0xDB == *(uBYTE*)buffer)
							unmapped = true;
					}
					else if (codepage == wincodepage)
					{
						if (0x80 == *(uBYTE*)buffer)
							unmapped = true;
					}
					#endif
				}

				if (!result)
				{
					DWORD	error = ::GetLastError();
					break;
				}
				if (!unmapped)
				{ // Unicode character successfully converted
					 // fill unused bytes with zeros
					 while(result<(uLONG)maxleneven)
						buffer[result++] = 0;
					// store Unicode to byte conversion
					if (!curxxx)
					{ // entering a converting range
						curxxx = xxx;
						curfirst = uchar;
						*(UniChar*)curxxx = curfirst; // first char in range
					}
					curlast = uchar;
					*(UniChar*)(curxxx+sizeof(sWORD)) = curlast; // last char in range
					data = xxx + 2*sizeof(sWORD) + (curlast-curfirst)*maxleneven;
					VMemory::CopyBlock(buffer, data, maxlen);
					// store byte to Unicode conversion
					if (maxlen==1)
					{
						UniChar* dst = (UniChar*)xxx2;
						dst += (uBYTE)buffer[0];
						if (*dst==0xFFFF) // don't fill it twice
							*dst = uchar;
					}
					else
					{
						*(sWORD*)curxxx2 = uchar;
						curxxx2 += sizeof(UniChar);
						VMemory::CopyBlock(buffer, curxxx2, maxleneven);
						curxxx2 += maxleneven;
					}
				}
				else if (curxxx)
				{ // getting out of a converting range
					size = VMemory::GetHandleSize(res);
					moresize = sizeof(uWORD)+sizeof(uWORD)+(curlast-curfirst+1)*maxleneven; // size of the range block header + conversions
					if (VMemory::SetHandleSize(res, size+moresize))
					{
						data = VMemory::LockHandle(res);
						(*(sWORD*)(data + sizeof(sWORD)))++;
						VMemory::CopyBlock(xxx, data+size, moresize);
						VMemory::UnlockHandle(res);
					}
					curxxx = NULL;
				}
			}
			if (maxlen>1)
			{
				// now sort the byte to Unicode array
				//Cast pour éviter le C4267
				count = (sLONG) ((curxxx2-xxx2)/(maxleneven+sizeof(UniChar)));
				sDebugMaxDBCSSize = maxlen;
				::qsort(xxx2, count, maxleneven+sizeof(UniChar), WIN_SortDBCS);
			}
			_AddUnicodeResource(res, uwinkind, script, scriptname);
			if (uwinkind == 'uwin') // L.E. 2/05/00 on en a besoin plus bas pour 'sort'
				uwinResource = res;
			else
				VMemory::DisposeHandle(res);
			
			if (maxlen==1)
			{
				size = 257*sizeof(UniChar);
				res = VMemory::NewHandle(size);
				VPtr px = VMemory::LockHandle(res);
				*(sWORD*)px = maxlen;
				VMemory::CopyBlock(xxx2, px+sizeof(sWORD), 256*sizeof(UniChar));
				VMemory::UnlockHandle(res);
			}
			else
			{
				size = (curxxx2-xxx2)+2*sizeof(sWORD);
				res = VMemory::NewHandle(size);
				VPtr px = VMemory::LockHandle(res);
				*(sWORD*)px = maxlen;
				*(sWORD*)(px+sizeof(sWORD)) = count;
				VMemory::CopyBlock(xxx2, px+2*sizeof(sWORD), curxxx2-xxx2);
				VMemory::UnlockHandle(res);
			}
			_AddUnicodeResource(res, winukind, script, scriptname);
			VMemory::DisposeHandle(res);
			VMemory::DisposePtr(xxx2);
		}
	}

	// we build a 'sort' resource for diacritical sort
	// the format of the resource is
	// uWORD : range count
	// followed for each range by
	// uWORD : first UniChar in range
	// uWORD : last UniChar in range
	// (last-first+1) sequences of sort values (uWORDs)
	if (uwinResource != NULL)
	{
		www = (uWORD*)xxx;
		for (uchar=0, www = (uWORD*)xxx;uchar<=0xFFFF;uchar++, www++)
			*www = uchar;
		::qsort(xxx, 0x1000, sizeof(uWORD), WIN_SortUnichar);
		data = VMemory::LockHandle(uwinResource);
		maxleneven = *(sWORD*)data;
		maxleneven = (maxleneven+1) & 0xFFFE;
		data += sizeof(sWORD);
		count = *(sWORD*)data; // range count
		data += sizeof(sWORD);
		res = VMemory::NewHandleClear(sizeof(sWORD));
		xxx2 = VMemory::NewPtr(0x10000*sizeof(UniChar), 'dial');
		while(count--) { // for each range
			range = (SingleRangeBlock*)xxx2;
			range->firstchar = *(sWORD*)data;
			data += sizeof(sWORD);
			range->lastchar = *(sWORD*)data;
			data += sizeof(sWORD);
			data += (1+range->lastchar-range->firstchar)*maxleneven; // UniToWin data
			for (uchar = range->firstchar; uchar <= range->lastchar; uchar++)
				range->converts[uchar-range->firstchar] = xxx[uchar];
			size = VMemory::GetHandleSize(res);
			moresize = (3 + range->lastchar - range->firstchar)*sizeof(UniChar);
			if (VMemory::SetHandleSize(res, size+moresize))
			{
				char* data2 = VMemory::LockHandle(res);
				(*(sWORD*)data2)++; // increment range count
				VMemory::CopyBlock(xxx2, data2+size, moresize);
				VMemory::UnlockHandle(res);
			}
		}
		VMemory::DisposePtr(xxx2);
		
		/*
		// the ACI implementation of this resource
		size = VMemory::GetHandleSize(res);
		data = VMemory::LockHandle(res);
		hh = ::NewHandle(size);
		VMemory::CopyBlock(xxx, *hh, size);
		VMemory::UnlockHandle(res);
		_AddUnicodeResource(hh, 'sort', inDialect, (uBYTE*) ""); //(uBYTE*)languagename);
		*/
		_AddUnicodeResource(res, 'sort', inDialect, ""); //(uBYTE*)languagename);
		VMemory::DisposeHandle(res);
		VMemory::UnlockHandle(uwinResource);
		VMemory::DisposeHandle(uwinResource);
	}

	// restore language
	::SetThreadLocale(curlocale);
	// clean up
	VMemory::DisposePtr(xxx);
}


void WIN_BuildUnicodeResources()
{
	DialectCode dialects[] = {
	DC_AFRIKAANS, 
	DC_ALBANIAN, 
	DC_ARABIC_SAUDI_ARABIA, 
	DC_ARABIC_IRAQ, 
	DC_ARABIC_EGYPT, 
	DC_ARABIC_LIBYA, 
	DC_ARABIC_ALGERIA, 
	DC_ARABIC_MOROCCO, 
	DC_ARABIC_TUNISIA, 
	DC_ARABIC_OMAN, 
	DC_ARABIC_YEMEN, 
	DC_ARABIC_SYRIA, 
	DC_ARABIC_JORDAN, 
	DC_ARABIC_LEBANON, 
	DC_ARABIC_KUWAIT, 
	DC_ARABIC_UAE, 
	DC_ARABIC_BAHRAIN, 
	DC_ARABIC_QATAR, 
	DC_BASQUE, 
	DC_BELARUSIAN, 
	DC_BULGARIAN, 
	DC_CATALAN, 
	DC_CHINESE_TRADITIONAL, 
	DC_CHINESE_SIMPLIFIED, 
	DC_CHINESE_HONGKONG, 
	DC_CHINESE_SINGAPORE, 
	DC_CROATIAN, 
	DC_CZECH, 
	DC_DANISH, 
	DC_DUTCH, 
	DC_DUTCH_BELGIAN, 
	DC_ENGLISH_US, 
	DC_ENGLISH_UK, 
	DC_ENGLISH_AUSTRALIA, 
	DC_ENGLISH_CANADA, 
	DC_ENGLISH_NEWZEALAND, 
	DC_ENGLISH_EIRE, 
	DC_ENGLISH_SOUTH_AFRICA, 
	DC_ENGLISH_JAMAICA, 
	DC_ENGLISH_CARIBBEAN, 
	DC_ENGLISH_BELIZE, 
	DC_ENGLISH_TRINIDAD, 
	DC_ESTONIAN, 
	DC_FAEROESE, 
	DC_FARSI, 
	DC_FINNISH, 
	DC_FRENCH, 
	DC_FRENCH_BELGIAN, 
	DC_FRENCH_CANADIAN, 
	DC_FRENCH_SWISS, 
	DC_FRENCH_LUXEMBOURG, 
	DC_GERMAN, 
	DC_GERMAN_SWISS, 
	DC_GERMAN_AUSTRIAN, 
	DC_GERMAN_LUXEMBOURG, 
	DC_GERMAN_LIECHTENSTEIN, 
	DC_GREEK, 
	DC_HEBREW, 
	DC_HUNGARIAN, 
	DC_ICELANDIC, 
	DC_INDONESIAN, 
	DC_ITALIAN, 
	DC_ITALIAN_SWISS, 
	DC_JAPANESE, 
	DC_KOREAN_WANSUNG, 
	DC_KOREAN_JOHAB, 
	DC_LATVIAN, 
	DC_LITHUANIAN, 
	DC_NORWEGIAN, 
	DC_NORWEGIAN_NYNORSK, 
	DC_POLISH, 
	DC_PORTUGUESE, 
	DC_PORTUGUESE_BRAZILIAN, 
	DC_ROMANIAN, 
	DC_RUSSIAN, 
	DC_SERBIAN_LATIN, 
	DC_SERBIAN_CYRILLIC, 
	DC_SLOVAK, 
	DC_SLOVENIAN, 
	DC_SPANISH_CASTILLAN, 
	DC_SPANISH_MEXICAN, 
	DC_SPANISH_MODERN, 
	DC_SPANISH_GUATEMALA, 
	DC_SPANISH_COSTA_RICA, 
	DC_SPANISH_PANAMA, 
	DC_SPANISH_DOMINICAN_REPUBLIC, 
	DC_SPANISH_VENEZUELA, 
	DC_SPANISH_COLOMBIA, 
	DC_SPANISH_PERU, 
	DC_SPANISH_ARGENTINA, 
	DC_SPANISH_ECUADOR, 
	DC_SPANISH_CHILE, 
	DC_SPANISH_URUGUAY, 
	DC_SPANISH_PARAGUAY, 
	DC_SPANISH_BOLIVIA, 
	DC_SPANISH_EL_SALVADOR, 
	DC_SPANISH_HONDURAS, 
	DC_SPANISH_NICARAGUA, 
	DC_SPANISH_PUERTO_RICO, 
	DC_SWEDISH, 
	DC_SWEDISH_FINLAND, 
	DC_THAI, 
	DC_TURKISH, 
	DC_UKRAINIAN, 
	DC_VIETNAMESE, 
	DC_NONE
	};
	DialectCode*	dialect = dialects;
	uLONG kinds[] = {'umac', 'macu', 'uwin', 'winu', 'diac', 'uprn', 'upra', 'lwrn', 'lwra', 'uset', 'sort', 0};
	uLONG*	kind;
	sWORD	count;
	VHandle	hh;
	uLONG	rkind;
	sWORD	rid, size;
	Boolean	inrange;
	UniChar	firstinrange;
	UniChar	lastinrange;
	UniChar*	set;
	char alphastr[] = "Alphas";
	char digitstr[] = "Digits";
	char spacestr[] = "Spaces";
	char pointstr[] = "Points";
	char* str;
	
	// build dialect dependant resources
	count = 0;
	while(*dialect!=DC_NONE)
	{
		::WIN_BuildDialectResources(*dialect, ++count);
		dialect++;
	}
	// build character kind tables
	for (rid=0;rid<=3;rid++)
	{
		switch(rid)
		{
		case RES_ALPHAS_RANGES:
			set = (UniChar*)sUniAlphaRanges;
			str = alphastr;
			break;

		case RES_DIGITS_RANGES:
			set = (UniChar*)sUniDigitRanges;
			str = digitstr;
			break;

		case RES_SPACES_RANGES:
			set = (UniChar*)sUniSpaceRanges;
			str = spacestr;
			break;

		case RES_PUNCTUATIONS_RANGES:
			set = (UniChar*)sUniPunctRanges;
			str = pointstr;
			break;
		}
		inrange = false;
		firstinrange = 0;
		lastinrange = 0;
		size = sizeof(sWORD);
		count = 0;
		hh = VMemory::NewHandleClear(size);
		while(*set)
		{
			if (!inrange)
			{
				firstinrange = *set;
				switch(firstinrange)
				{
				case CHAR_CJK_IDEOGRAPH_FIRST_4E00:
					lastinrange = CHAR_CJK_IDEOGRAPH_LAST_9FA5 - 1;
					break;

				case CHAR_HANGUL_SYLLABLE_FIRST_AC00:
					lastinrange = CHAR_HANGUL_SYLLABLE_LAST_D7A3 - 1;
					break;

				case CHAR_UNASSIGNED_HIGH_SURROGATE_FIRST_D800:
					lastinrange = CHAR_UNASSIGNED_HIGH_SURROGATE_LAST_DB7F;
					break;

				case CHAR_PRIVATE_USE_HIGH_SURROGATE_FIRST_DB80:
					lastinrange = CHAR_PRIVATE_USE_HIGH_SURROGATE_LAST_DBFF;
					break;

				case CHAR_LOW_SURROGATE_FIRST_DC00:
					lastinrange = CHAR_LOW_SURROGATE_LAST_DFFF;
					break;

				case CHAR_PRIVATE_USE_FIRST_E000:
					lastinrange = CHAR_PRIVATE_USE_LAST_F8FF;
					break;

				default:
					lastinrange = *set;
					break;
				}
				inrange = true;
				set++;
			}
			else if (*set-lastinrange>1) // getting out of a range, save it
			{
				size += 2*sizeof(sWORD);
				VMemory::SetHandleSize(hh, size);
				**(sWORD**)hh = ++count;
				(*(UniChar**)hh)[(count<<1)-1] = firstinrange;
				(*(UniChar**)hh)[count<<1] = lastinrange;
				inrange = false;
			}
			else
			{
				lastinrange = *set;
				set++;
			}
		}
		
		_AddUnicodeResource(hh, 'uset', rid, str);
		VMemory::DisposeHandle(hh);
	}
}

#endif


END_TOOLBOX_NAMESPACE

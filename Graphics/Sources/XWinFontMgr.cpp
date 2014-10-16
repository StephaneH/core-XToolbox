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
#include "VGraphicsPrecompiled.h"
#include "VFontMgr.h"
#include "XWinGDIGraphicContext.h"


#if USE_GDIPLUS
using namespace Gdiplus;
#endif

#include "richedit.h"


XWinFontMgr::XWinFontMgr()
{
	// TBD
	BuildFontList();
}


XWinFontMgr::~XWinFontMgr()
{
	// TBD
}


void XWinFontMgr::BuildFontList()
{
	VTaskLock lock(&fCriticalSection);

	HDC hDC ;

	fFontFamilies.SetCount(0);
	//fFontNames.SetCount(0);
	fFontNames.clear();
	// TBD
	LOGFONTW lf;
	memset(&lf, 0, sizeof(LOGFONTW));
	lf.lfCharSet = DEFAULT_CHARSET;

	hDC = GetDC(NULL);
	
	::EnumFontFamiliesExW(hDC, &lf,
				(FONTENUMPROCW) EnumFamScreenCallBackEx, (LPARAM) this, NULL);

	ReleaseDC(NULL, hDC);

	std::sort(fFontNames.begin(),fFontNames.end());
	
	// try to load inactive fonte list from the registry
	HKEY hKey;
	if (RegOpenKeyExW(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Font Management",NULL,KEY_READ ,&hKey) == ERROR_SUCCESS)
	{
		DWORD datasize=0;
		DWORD datatype;
		sLONG err = RegQueryValueExW (hKey,L"Inactive Fonts",NULL,&datatype,NULL,&datasize);
		if(err==ERROR_SUCCESS && datasize> 0 )
		{
			void* buff = malloc(datasize + 2);
			memset(buff,0,datasize + 2);
			err = RegQueryValueExW (hKey,L"Inactive Fonts",NULL,&datatype,(LPBYTE)buff,&datasize);
			if(err==ERROR_SUCCESS)
			{
				XBOX::VString str;
				UniChar* c =(UniChar*)buff;
				sLONG len=datasize/2;
				sLONG slen;
				while (len>0)
				{
					str.FromUniCString(c);
					if(*c!=0)
						fInactiveFontNames.push_back(str);
					slen = str.GetLength()+1;
					len-=slen;
					c+=slen;
				};
			}
			free(buff);
		}
		RegCloseKey(hKey);
		std::sort(fInactiveFontNames.begin(),fInactiveFontNames.end());
	}
	// remove inactive font from list
	fActiveFontNames = fFontNames;
	if(fInactiveFontNames.size()>0)
	{
		VectorOfVString::iterator it;
		for(it=fInactiveFontNames.begin();it!=fInactiveFontNames.end();++it)
		{
			VectorOfVString::iterator last = std::remove(fActiveFontNames.begin(),fActiveFontNames.end(),*it);
			fActiveFontNames.erase(last,fActiveFontNames.end());
		}
	}
	
	//keep only the first 100 font names (and not vertical fonts) for the context menu

	fContextualMenuFontNames = fActiveFontNames;
	if(fContextualMenuFontNames.size()>100)
	{
		VectorOfVString::iterator it;
		for(VectorOfVString::iterator it=fContextualMenuFontNames.begin();it!=fContextualMenuFontNames.end();)
		{
			if((*it)[0]=='@')
				it = fContextualMenuFontNames.erase(it);
			else 
				it++;
			
		}
		if(fContextualMenuFontNames.size()>100)
		{
			fContextualMenuFontNames.resize(100);
		}
	}
}

BOOL CALLBACK XWinFontMgr::EnumFamScreenCallBackEx(ENUMLOGFONTEXW* pelf, NEWTEXTMETRICEXW* /*lpntm*/, int FontType,LPVOID pThis)
{
	XBOX::VString fontName;
	fontName.FromUniCString(pelf->elfLogFont.lfFaceName);
	
	((XWinFontMgr *)pThis)->AddFont(fontName);
	
	return 1;
}

BOOL CALLBACK XWinFontMgr::FaceNameToFullNameCallBackEx(ENUMLOGFONTEXW* pelf, NEWTEXTMETRICEXW* /*lpntm*/, int FontType,LPVOID pThis)
{
	XBOX::VString fontName;
	fontName.FromUniCString(pelf->elfLogFont.lfFaceName);
	
	((XWinFontMgr *)pThis)->AddInactiveFont(fontName);
	
	return 1;
}

void XWinFontMgr::AddFont(const XBOX::VString& inFontName)
{
	VTaskLock lock(&fCriticalSection);

	if( std::find(fFontNames.begin(), fFontNames.end(), inFontName) == fFontNames.end())
		fFontNames.push_back(inFontName);
}

void XWinFontMgr::AddInactiveFont(const VString& inFontName)
{
	VTaskLock lock(&fCriticalSection);

	if( std::find(fInactiveFontNames.begin(), fInactiveFontNames.end(), inFontName) == fInactiveFontNames.end())
		fInactiveFontNames.push_back(inFontName);
}

void XWinFontMgr::GetFontNameList(VectorOfVString& outFontNames, bool inWithScreenFonts)
{
	VTaskLock lock(&fCriticalSection);
	outFontNames = fFontNames;
}


void XWinFontMgr::GetUserActiveFontNameList(VectorOfVString& outFontNames)
{
	VTaskLock lock(&fCriticalSection);
	outFontNames = fActiveFontNames;
}

void XWinFontMgr::GetFontNameListForContextualMenu(VectorOfVString& outFontNames)
{
	VTaskLock lock(&fCriticalSection);
	outFontNames = fContextualMenuFontNames;
}

void XWinFontMgr::GetStdFont(StdFont inFont, VString& outName, VFontFace& outFace, GReal& outSize)
{
	HFONT		hFont=NULL;
	LOGFONTW*	winFont = NULL;
	LOGFONTW	logFont;

	NONCLIENTMETRICSW	metrics;
	metrics.cbSize = sizeof(NONCLIENTMETRICSW);
	
	switch(inFont) 
	{
		case STDF_APPLICATION:
			if (::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 0,(void*)&metrics,0))
				winFont = &metrics.lfMessageFont;
			break;
		case STDF_LIST:
		case STDF_ICON:
			if (::SystemParametersInfoW(SPI_GETICONTITLELOGFONT, 0,(void*)&logFont,0))
				winFont = &logFont;
			break;
		case STDF_MENU:
			if (::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 0,(void*)&metrics,0))
				winFont = &metrics.lfMenuFont;
			break;
		case STDF_MENU_MINI:
		case STDF_TIP:
			if (::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 0,(void*)&metrics,0))
				winFont = &metrics.lfStatusFont;
			break;
		case STDF_CAPTION:
		case STDF_SYSTEM_MINI:
			if (::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 0,(void*)&metrics,0))
				winFont = &metrics.lfMessageFont;
			break;
		case STDF_SYSTEM_SMALL:
			if (::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 0,(void*)&metrics,0))
				winFont = &metrics.lfMessageFont;
			break;
		case STDF_EDIT:
		case STDF_TEXT:
		case STDF_SYSTEM:	
		
			if (::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 0,(void*)&metrics,0))
				winFont = &metrics.lfMessageFont;
			break;
		case STDF_WINDOW_CAPTION:
			if (::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 0,(void*)&metrics,0))
				winFont = &metrics.lfCaptionFont;
				break;
		case STDF_WINDOW_SMALL_CAPTION:
			if (::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 0,(void*)&metrics,0))
				winFont = &metrics.lfSmCaptionFont;
				break;
		default:
			assert(false);
			break;
	}
	
	if (winFont != NULL)
	{
		outName.FromUniCString(winFont->lfFaceName);
		outFace = (winFont->lfWeight > 500) ? KFS_BOLD : KFS_NORMAL;
		outFace |= (winFont->lfItalic) ? KFS_ITALIC : KFS_NORMAL;
		outSize = PixelsToPoints(Abs(winFont->lfHeight));
	}
}


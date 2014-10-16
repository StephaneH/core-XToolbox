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
#include "VFont.h"
#if VERSIONWIN
#include "XWinGDIGraphicContext.h"
#endif


VFontMgr::VFontMgr()
{
	for(sLONG i=0;i<STDF_LAST;i++)
		fStdFonts[i]=NULL;
		
	VLastUsedFontNameMgr::Init();
}


VFontMgr::~VFontMgr()
{
	fFonts.Destroy(DESTROY);
	for(sLONG i=0;i<STDF_LAST;i++)
		ReleaseRefCountable(&fStdFonts[i]);

	VLastUsedFontNameMgr::Deinit();
}

	
void VFontMgr::GetFontNameList(VectorOfVString& outFontNames, bool inWithScreenFonts)
{
	fFontMgr.GetFontNameList(outFontNames, inWithScreenFonts);
}


void VFontMgr::GetFontFullNameList(VectorOfVString& outFontNames, bool inWithScreenFonts)
{
	fFontMgr.GetFontFullNameList(outFontNames, inWithScreenFonts);
}


void VFontMgr::GetUserActiveFontNameList(VectorOfVString& outFontNames)
{
	fFontMgr.GetUserActiveFontNameList(outFontNames);
}
void VFontMgr::GetFontNameListForContextualMenu(VectorOfVString& outFontNames)
{
	fFontMgr.GetFontNameListForContextualMenu(outFontNames);
}

/** localize font name (if font name is not localized) according to current system UI language */
void VFontMgr::LocalizeFontName(VString& ioFontName) const
{
    fFontMgr.LocalizeFontName(ioFontName);
}


VFont* VFontMgr::RetainFont(const VString& inFontFamilyName, const VFontFace& inFace, GReal inSize, const GReal inDPI, bool inReturnNULLIfNotExist)
{
	VTaskLock lock(&fCriticalSection);
	
#if VERSIONWIN
	GReal pixelSize = 0.0f;
#endif
	if (inDPI)
	{
	//scale font size to the desired DPI
#if VERSIONWIN
#if ENABLE_D2D
#if D2D_GDI_COMPATIBLE
	if (inDPI == 72)
		pixelSize = inSize; //to avoid math discrepancies with font size in 4D form, we ensure to pass initial pixel font size to a new VFont
							//(to ensure a 4D form 12pt font will actually have a real 12px font size)
	inSize = inSize*inDPI/VWinGDIGraphicContext::GetLogPixelsY();
#else
	inSize = inSize*inDPI/96.0f;
#endif
#else
	inSize = inSize*inDPI/VWinGDIGraphicContext::GetLogPixelsY();
#endif
#else
	inSize = inSize*inDPI/72.0f;
#endif
	}

	VFont*	theFont = NULL;
	
	for(sLONG i = 0 ; i < fFonts.GetCount() ; ++i)
	{
		VFont* oneFont = fFonts[i];
		if(oneFont->Match(inFontFamilyName, inFace, inSize))
		{
			oneFont->Retain();
			theFont = oneFont;
			break;
		}
	}
	
	if(theFont == NULL)
	{
#if VERSIONWIN
		theFont = new VFont(inFontFamilyName, inFace, inSize, STDF_NONE, pixelSize);
#else
		theFont = new VFont(inFontFamilyName, inFace, inSize);
#endif
		if (inReturnNULLIfNotExist)
			if (!theFont->GetImpl().GetFontRef())
			{
				delete theFont;
				return NULL;
			}
		if(fFonts.AddTail(theFont))
		{
			theFont->Retain();
		}
	}
	
	return theFont;
}


VFont* VFontMgr::RetainStdFont(StdFont inFont)
{
	VTaskLock lock(&fCriticalSection);
	
	VFont*	theFont = NULL;
	
	xbox_assert(inFont>STDF_NONE && inFont<STDF_LAST);

	theFont=fStdFonts[inFont];
	if(theFont==NULL)
	{
		VStr31	name;
		VFontFace	style;
		GReal	size;
		fFontMgr.GetStdFont(inFont, name, style, size);
		theFont = RetainFont(name, style, size);
		fStdFonts[inFont] = theFont;
	}
	if(theFont)
		theFont->Retain();
	
	return theFont;
}


VFontMgr::VLastUsedFontNameMgr* VFontMgr::VLastUsedFontNameMgr::sSingleton = NULL;

VFontMgr::VLastUsedFontNameMgr* VFontMgr::VLastUsedFontNameMgr::Get()
{
	if (!sSingleton)
		sSingleton = new VLastUsedFontNameMgr();
	return sSingleton;
}

bool VFontMgr::VLastUsedFontNameMgr::Init()
{
	return Get() != NULL;
}
void VFontMgr::VLastUsedFontNameMgr::Deinit()
{
	if (sSingleton)
		delete sSingleton;
}


/** add font name to the list of last used font names */
void VFontMgr::VLastUsedFontNameMgr::AddFontName( const VString& inFontName)
{
	VTaskLock lock(&fCriticalSection);

	VectorOfLastUsedFont::iterator it = fLastUsedFonts.begin();
	for (;it != fLastUsedFonts.end(); it++)
	{
		if (it->first.EqualToString( inFontName, true))
		{
			//update only time stamp
			it->second = VSystem::GetCurrentTime();
			return;
		}
	}

	while (fLastUsedFonts.size() >= VFONTMGR_LAST_USED_FONT_LIST_SIZE)
	{
		//free oldest used font name
		VectorOfLastUsedFont::iterator itOldest = fLastUsedFonts.begin();
		VectorOfLastUsedFont::iterator it = itOldest;
		it++;
		for (;it != fLastUsedFonts.end(); it++)
		{
			if (it->second < itOldest->second)
				itOldest = it;
		}
		fLastUsedFonts.erase( itOldest);
	}

	//add new font name 
	fLastUsedFonts.push_back( LastUsedFont( inFontName, VSystem::GetCurrentTime()));	
}

/** get last used font names */
void VFontMgr::VLastUsedFontNameMgr::GetFontNames(VectorOfVString& outFontNames) const		
{
	VTaskLock lock(&fCriticalSection);

	outFontNames.clear();
	VectorOfLastUsedFont::const_iterator it = fLastUsedFonts.begin();
	for (;it != fLastUsedFonts.end(); it++)
	{
		if (outFontNames.empty())
			outFontNames.push_back( it->first);
		else
		{
			VectorOfVString::iterator itName = outFontNames.begin();
			for (;itName != outFontNames.end(); itName++)
			{
				if (itName->CompareToString( it->first) == XBOX::CR_BIGGER)
				{
					outFontNames.insert( itName, it->first);
					break;
				}
			}
			if (itName == outFontNames.end())
				outFontNames.push_back( it->first);
		}
	}
}




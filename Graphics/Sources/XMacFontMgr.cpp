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
#include "XMacFont.h"
#include "VFont.h"

#include <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>

XMacFontMgr::XMacFontMgr()
{
	BuildFontList();
}


XMacFontMgr::~XMacFontMgr()
{
}

void XMacFontMgr::_UpdateMapOfFontNeutralNamePerLocalizedName( CTFontDescriptorRef inDesc)
{
    //CoreText CTFontCreateWithName accepts only neutral name so build reverse map here in order to be able to pass neutral name
    
    CFStringRef familyLocalized = (CFStringRef)CTFontDescriptorCopyLocalizedAttribute(inDesc, kCTFontFamilyNameAttribute, NULL);
    CFStringRef fullLocalized = (CFStringRef)CTFontDescriptorCopyLocalizedAttribute(inDesc, kCTFontDisplayNameAttribute, NULL);
    
    CFStringRef family = (CFStringRef)CTFontDescriptorCopyAttribute(inDesc, kCTFontFamilyNameAttribute);
    CFStringRef full = (CFStringRef)CTFontDescriptorCopyAttribute(inDesc, kCTFontDisplayNameAttribute);

    VString xfamily, xfull, xfamilyLocalized, xfullLocalized;
    
    xfamily.MAC_FromCFString(family);
    xfamilyLocalized.MAC_FromCFString(familyLocalized);
    xfull.MAC_FromCFString(full);
    xfullLocalized.MAC_FromCFString(fullLocalized);
    
    if (xfamilyLocalized != xfamily)
    {
        fFontNeutralNamePerLocalizedNames[xfamilyLocalized] = xfamily;
#if WITH_DEBUGMSG 
        VString msg = "XMacfontMgr: add localized family name pair < ";
        msg.AppendString(xfamily);
        msg.AppendCString(" , ");
        msg.AppendString(xfamilyLocalized);
        msg.AppendCString(" >\n");
        XBOX::DebugMsg(msg);
#endif
    }
    if (xfullLocalized != xfull)
    {
        fFontNeutralNamePerLocalizedNames[xfullLocalized] = xfull;
#if WITH_DEBUGMSG
        VString msg = "XMacfontMgr: add localized full name pair < ";
        msg.AppendString(xfull);
        msg.AppendCString(" , ");
        msg.AppendString(xfullLocalized);
        msg.AppendCString(" >\n");
        XBOX::DebugMsg(msg);
#endif
    }
   
    CFRelease(full);
    CFRelease(family);
    CFRelease(fullLocalized);
    CFRelease(familyLocalized);
}

void XMacFontMgr::FontNameLocalizedToNeutral( VString& ioName) const
{
    MapOfFontNeutralNamePerLocalizedName::const_iterator itName = fFontNeutralNamePerLocalizedNames.find(ioName);
    if (itName != fFontNeutralNamePerLocalizedNames.end())
        ioName = itName->second;
}



void XMacFontMgr::BuildFontList()
{
	VTaskLock lock(&fCriticalSection);

	fFontFamilies.SetCount(0);
	//fFontNames.SetCount(0);
	fFontNames.clear();
    fFontNeutralNamePerLocalizedNames.clear();
    
	/*
	FMFontFamilyIterator	iterator;
	OSErr	error = ::FMCreateFontFamilyIterator(NULL, NULL, kFMDefaultOptions, &iterator);
	if (error == noErr)
	{
		FMFontFamily	family;
		
		assert(sizeof(FMFontFamily) == sizeof(sWORD));
		while (::FMGetNextFontFamily(&iterator, &family) == noErr)
		{
			Str255 spFontName;
			if (testAssert(::FMGetFontFamilyName(family, spFontName) == noErr))
			{
				VStr255	name;
				CFStringRef cfname;
				ATSFontFamilyRef ffref = ::ATSFontFamilyFindFromQuickDrawName(spFontName);
				::ATSFontFamilyGetName(ffref, kATSOptionFlagsDefault, &cfname);
				//name.MAC_FromMacPString(spFontName);
				name.MAC_FromCFString(cfname);
				if(name[0] != '.' && name[0] != '%')
				{	
					fFontNames.AppendString(name);
					fFontFamilies.AppendWord((sWORD) family);
				}
				//CFRelease(cfname);
			}
		}
		
		::FMDisposeFontFamilyIterator(&iterator);
	}*/
	
	CTFontCollectionRef FontCollection = ::CTFontCollectionCreateFromAvailableFonts(0);//kCTFontCollectionRemoveDuplicatesOption
	if(FontCollection)
    {
		CFArrayRef fonts = CTFontCollectionCreateMatchingFontDescriptors(FontCollection);
		if(fonts)
		{
			const int numFonts = CFArrayGetCount(fonts);
			for(int i = 0; i < numFonts; ++i) 
			{
				VString	vname;
				CTFontDescriptorRef vfont = (CTFontDescriptorRef)CFArrayGetValueAtIndex(fonts, i);
                _UpdateMapOfFontNeutralNamePerLocalizedName(vfont);
				CFStringRef family_name = (CFStringRef)CTFontDescriptorCopyLocalizedAttribute(vfont, kCTFontFamilyNameAttribute, NULL);
				CFStringRef full_name = (CFStringRef)CTFontDescriptorCopyLocalizedAttribute(vfont, kCTFontDisplayNameAttribute, NULL);
				if(family_name!=NULL)
				{
					vname.MAC_FromCFString(family_name);
				
					if( std::find(fFontNames.begin(), fFontNames.end(), vname) == fFontNames.end())
						fFontNames.push_back(vname);
					CFRelease(family_name);
				}
				if(full_name!=NULL)
				{
					vname.MAC_FromCFString(full_name);
				
					if( std::find(fFontFullNames.begin(), fFontFullNames.end(), vname) == fFontFullNames.end())
						fFontFullNames.push_back(vname);
					CFRelease(full_name);
				}
			}
		}
	}	
	
	//fFontNames.SynchronizedSort(false, &fFontFamilies);
	//fFontNames.Sort(0, fFontNames.GetCount()-1);
	std::sort(fFontNames.begin(),fFontNames.end());
	std::sort(fFontFullNames.begin(),fFontFullNames.end());
	fFontNamesForContextMenu = fFontNames;
	fFontNamesForContextMenu.resize(100); //keep only the 100 first font names for context menu
}

void XMacFontMgr::GetFontNameList(VectorOfVString& outFontNames, bool inWithScreenFonts)
{
	VTaskLock lock(&fCriticalSection);
	outFontNames = fFontNames;
}

void XMacFontMgr::GetFontFullNameList(VectorOfVString& outFontNames, bool inWithScreenFonts)
{
	VTaskLock lock(&fCriticalSection);
	outFontNames = fFontFullNames;
}

void XMacFontMgr::GetUserActiveFontNameList(VectorOfVString& outFontNames)
{
	VTaskLock lock(&fCriticalSection);

    //important: system favorites font collection might have changed since last call to GetUserActiveFontNameList()
    //           because favorites font collection can be updated from the system font panel so we need to rebuild always
    
    
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    NSString* nsFavoritesCollectionName = @"com.apple.Favorites";
    
    //check if favorites collection actually exists
    static bool hasFavorites = false;
    if (nsFavoritesCollectionName)
    {
        NSArray *collectionNames = [[NSFontManager sharedFontManager] collectionNames];
        for (NSString *name in collectionNames)
        {
            if ([name compare:nsFavoritesCollectionName] == NSOrderedSame)
            {
                hasFavorites = true;
                break;
            }
        }
    }
    
    if (hasFavorites)
    {
        //build favorites font name list
        
        fFontFavoritesNames.clear();
        VString xFontFamily;
        
        NSArray* fontDescriptors = [[NSFontManager sharedFontManager] fontDescriptorsInCollection:nsFavoritesCollectionName];
        for (NSFontDescriptor* fontDescriptor in fontDescriptors)
        {
            NSString *fontFamily = [fontDescriptor objectForKey:NSFontFamilyAttribute];
            if (fontFamily)
            {
                xFontFamily.MAC_FromCFString((CFStringRef)fontFamily);
                VFont::LocalizeFontName(xFontFamily, kFONTNAMEKIND_FAMILY);
                if (std::find(fFontFavoritesNames.begin(), fFontFavoritesNames.end(), xFontFamily) == fFontFavoritesNames.end())
                    fFontFavoritesNames.push_back(xFontFamily);
            }
        }
        std::sort(fFontFavoritesNames.begin(), fFontFavoritesNames.end());
        
        [pool release];
        
        outFontNames = fFontFavoritesNames;
    }
    else
    {
        //fallback to font family name list
        [pool release];
        
        outFontNames = fFontNames;
    }
}

void XMacFontMgr::GetFontNameListForContextualMenu(VectorOfVString& outFontNames)
{
	VTaskLock lock(&fCriticalSection);
	outFontNames = fFontNamesForContextMenu;
}

void XMacFontMgr::GetStdFont( StdFont inFont, VString& outName, VFontFace& outFace, GReal& outSize)
{
	// on risque de perdre des infos, il faudrait retourner une VFont
	
	CTFontUIFontType fontType = StdFontToThemeFontID( inFont);
	CTFontRef fontRef = ::CTFontCreateUIFontForLanguage( fontType, 0 /*size*/, NULL /*language*/ );

	if (testAssert( fontRef != NULL))
	{
		outSize = ::CTFontGetSize( fontRef);

		CFStringRef cfName = ::CTFontCopyFamilyName( fontRef);
		if (cfName != NULL)
		{
			outName.MAC_FromCFString( cfName);
			::CFRelease( cfName);
		}
		else
		{
			outName.Clear();
		}
		
		CTFontSymbolicTraits traits = ::CTFontGetSymbolicTraits( fontRef);
		outFace = 0;
		
		if (traits & kCTFontItalicTrait)
			outFace |= KFS_ITALIC;
		
		if (traits & kCTFontBoldTrait)
			outFace |= KFS_BOLD;
		
		if (traits & kCTFontExpandedTrait)
			outFace |= KFS_EXTENDED;
		
		if (traits & kCTFontCondensedTrait)
			outFace |= KFS_CONDENSED;

		CFRelease( fontRef); 
	}
	else
	{
		outName.Clear();
		outSize = 0;
		outFace = 0;
	}
}

CTFontUIFontType XMacFontMgr::StdFontToThemeFontID(StdFont inFont)
{
	CTFontUIFontType	macFont;
	switch (inFont)
	{
		case STDF_TEXT:
			macFont = kCTFontUserFontType;
			break;
			
		case STDF_MENU:
			macFont = kCTFontMenuItemFontType;
			break;
			
		case STDF_MENU_MINI:
			macFont = kCTFontSmallEmphasizedSystemFontType;
			break;
			
		case STDF_SYSTEM:
			macFont = kCTFontSystemFontType;
			break;
		case STDF_APPLICATION:
			macFont = kCTFontApplicationFontType;
			break;	
		case STDF_CAPTION:
			macFont = kCTFontLabelFontType;
			break;
		
		case STDF_SYSTEM_SMALL:
			macFont = kCTFontSmallSystemFontType;
			break;
			
		case STDF_SYSTEM_MINI:
			macFont = kCTFontMiniSystemFontType;
			break;
			
		case STDF_PUSH_BUTTON:
			macFont = kCTFontPushButtonFontType;
			break;
			
		case STDF_ICON:
			macFont = kCTFontToolbarFontType;
			break;
			
		case STDF_EDIT:
		case STDF_LIST:
			macFont = kCTFontViewsFontType;
			break;
			
		case STDF_TIP:
			macFont = kCTFontToolTipFontType;
			break;
			
		case STDF_WINDOW_SMALL_CAPTION:
			macFont = kCTFontUtilityWindowTitleFontType;
			break;
			
		case STDF_WINDOW_CAPTION:
			macFont = kCTFontWindowTitleFontType;
			break;
			
		case STDF_NONE:
			macFont = kCTFontSystemFontType;
			break;
			
		default:
			xbox_assert(false);
			macFont = kCTFontSystemFontType;
			break;
	}
	
	return macFont;
}
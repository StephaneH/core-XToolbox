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
#include "VFont.h"
#include "VGraphicContext.h"
#if VERSIONWIN
#include "XWinGDIGraphicContext.h"
#if ENABLE_D2D
#include "XWinD2DGraphicContext.h"
#endif
#endif

VFontMgr*	VFont::sManager;


VFont::VFont(const VString& inFontFamilyName, const VFontFace& inFace, GReal inSize, StdFont inStdFont, 
#if VERSIONWIN
			 GReal inPixelSize
#else
			 GReal
#endif
			 ):
#if VERSIONWIN
	fPixelSize(inPixelSize),
#endif
	fFont(this, inFontFamilyName, inFace, Abs(inSize))
{
	fStdFont = inStdFont;
	fDesignName = inFontFamilyName;
	fFace = inFace;
	fSize = Abs(inSize);

	fFontIDValid=false;   
	fFontID=0;

#if WITH_DEBUGMSG
	VString msg = "\nVFont: design font name: ";
	msg.AppendString(inFontFamilyName);
	msg.AppendCString( " -- font face:");
	if (inFace & KFS_BOLD)
		msg.AppendCString( " bold");
	if (inFace & KFS_ITALIC)
		msg.AppendCString( " italic");
	if (inFace & KFS_CONDENSED)
		msg.AppendCString( " condensed");
	if (inFace & KFS_EXTENDED)
		msg.AppendCString( " extended");
	if (inFace == 0)
		msg.AppendCString( " normal");
	msg.AppendCString( " -- font size: ");
	msg.AppendLong( (sLONG)floor(inSize+0.5));
	msg.AppendCString( "pt");
		
	msg.AppendCString( "\nVFont: family name (neutral): ");
	msg.AppendString( GetName( kFONTNAMEKIND_FAMILY, false));
	msg.AppendCString( "\nVFont: family name (localized): ");
	msg.AppendString( GetName( kFONTNAMEKIND_FAMILY, true));

#if VERSIONMAC || ENABLE_D2D
	msg.AppendCString( "\nVFont: postcript name (neutral): ");
	msg.AppendString( GetName( kFONTNAMEKIND_POSTSCRIPT, false));
	msg.AppendCString( "\nVFont: postscript name (localized): ");
	msg.AppendString( GetName( kFONTNAMEKIND_POSTSCRIPT, true));

	msg.AppendCString( "\nVFont: full name (neutral): ");
	msg.AppendString( GetName( kFONTNAMEKIND_FULL, false));
	msg.AppendCString( "\nVFont: full name (localized): ");
	msg.AppendString( GetName( kFONTNAMEKIND_FULL, true));
#endif
	msg.AppendCString( "\n");

	XBOX::DebugMsg(msg);
#endif
}

/*	get font name
				
	@param inFontNameKind
		desired font name kind:

		kFONTNAMEKIND_DESIGN		//return name as it was passed at font creation (param inLocalized is ignored)
		kFONTNAMEKIND_FAMILY		//return family name (localized or not depending on param inLocalized) 
		kFONTNAMEKIND_POSTSCRIPT	//return postcript name localized or not depending on param inLocalized) 
		kFONTNAMEKIND_FULL			//return full name (localized or not depending on param inLocalized) 
*/
const VString&	VFont::GetName (eFontNameKind inFontNameKind, bool inLocalized) const
{
	switch (inFontNameKind)
	{
		case kFONTNAMEKIND_FAMILY:
			if (inLocalized)
			{
				if (fFamilyNameLocalized.IsEmpty())
					fFont.GetName( inFontNameKind, fFamilyNameLocalized, inLocalized);
				return fFamilyNameLocalized;
			}
			else
			{
				if (fFamilyName.IsEmpty())
					fFont.GetName( inFontNameKind, fFamilyName, inLocalized);
				return fFamilyName;
			}
			break;
		case kFONTNAMEKIND_POSTSCRIPT: 
			if (inLocalized)
			{
				if (fPostscriptNameLocalized.IsEmpty())
					fFont.GetName( inFontNameKind, fPostscriptNameLocalized, inLocalized);
				return fPostscriptNameLocalized;
			}
			else
			{
				if (fPostscriptName.IsEmpty())
					fFont.GetName( inFontNameKind, fPostscriptName, inLocalized);
				return fPostscriptName;
			}
			break;
		case kFONTNAMEKIND_FULL: 
			if (inLocalized)
			{
				if (fDisplayNameLocalized.IsEmpty())
					fFont.GetName( inFontNameKind, fDisplayNameLocalized, inLocalized);
				return fDisplayNameLocalized;
			}
			else
			{
				if (fDisplayName.IsEmpty())
					fFont.GetName( inFontNameKind, fDisplayName, inLocalized);
				return fDisplayName;
			}
		default:
			return fDesignName; //design name is returned as it is 
			break;
	}
}


/** return the best existing font family name which matches the specified generic ID 
@see
	eGenericFont
*/
void VFont::GetGenericFontFamilyName(eGenericFont inGenericID, VString& outFontFamilyName, const VGraphicContext *inGC)
{
#if VERSIONWIN
	const Gdiplus::FontFamily *fontFamily = NULL;
	switch( inGenericID)
	{
	case GENERIC_FONT_SERIF:
		fontFamily = Gdiplus::FontFamily::GenericSerif();
		break;
	case GENERIC_FONT_SANS_SERIF:
		fontFamily = Gdiplus::FontFamily::GenericSansSerif();
		break;
	case GENERIC_FONT_CURSIVE:
		//gdiplus implementation constraint: 
		//try with "Comic Sans MS"
		//and if failed fallback to sans-serif
		{
			if (FontExists( "Comic Sans MS", KFS_NORMAL, 12, 0, inGC))
			{
				outFontFamilyName = "Comic Sans MS";
				return;
			}
			fontFamily = Gdiplus::FontFamily::GenericSansSerif();
		}
		break;
	case GENERIC_FONT_FANTASY:
		//gdiplus implementation constraint: fallback to sans-serif
		fontFamily = Gdiplus::FontFamily::GenericSansSerif();
		break;
	case GENERIC_FONT_MONOSPACE:
		fontFamily = Gdiplus::FontFamily::GenericMonospace();
		break;
	default:
		xbox_assert(false);
		fontFamily = Gdiplus::FontFamily::GenericSansSerif();
		break;
	}
	if (fontFamily && fontFamily->IsAvailable())
	{
		WCHAR  name[LF_FACESIZE];
		Gdiplus::Status status = fontFamily->GetFamilyName( name, LANG_NEUTRAL);
		xbox_assert(sizeof(WCHAR) == sizeof(UniChar));
		outFontFamilyName.FromBlock( name, wcslen( name)*sizeof(UniChar), VTC_UTF_16);
	}
	else
		outFontFamilyName = "Times New Roman";
#if ENABLE_D2D
#if VERSION_DEBUG
	//check if font family is supported by DWrite too
	//(should always be the case because GDIPlus has support only for TrueType fonts
	// & all TrueType fonts are supported by D2D)
	if (VWinD2DGraphicContext::IsAvailable())
	{
		if (inGC && inGC->IsD2DImpl())
			xbox_assert(FontExists( outFontFamilyName, KFS_NORMAL, 12, inGC));
	}
#endif
#endif
#else
	//MAC OS implementation constraint:
	//as there is no system-provided feature that can feed us with generic fonts
	//(of course if you know about such a feature feel free to modify code below)
	//we need to provide ourselves generic font family names
	//so we choose to use font family names that are always installed on MAC OS X

	VString fontFamily;
	switch( inGenericID)
	{
	case GENERIC_FONT_SERIF:
		fontFamily = "Times New Roman";
		break;
	case GENERIC_FONT_SANS_SERIF:
		fontFamily = "Helvetica";
		break;
	case GENERIC_FONT_CURSIVE:
		fontFamily = "Apple Chancery";
		break;
	case GENERIC_FONT_FANTASY:
		fontFamily = "American Typewriter";
		break;
	case GENERIC_FONT_MONOSPACE:
		fontFamily = "Courier New";
		break;
	default:
		xbox_assert(false);
		fontFamily = "Times New Roman";
		break;
	}
	if (FontExists( fontFamily, KFS_NORMAL, 12))
		outFontFamilyName = fontFamily;
	else
		//fallback to "Times Roman" just in case...
		outFontFamilyName = "Times New Roman";
#endif
}


/** return true if font with the specified font family name, style and size exists on the current system */
bool VFont::FontExists(const VString& inFontFamilyName, const VFontFace& inFace, GReal inSize, const GReal inDPI, const VGraphicContext *inGC)
{
	bool exists = false;
#if VERSIONWIN
	//JQ 21/06/2010: fixed and optimized FontExists (was not coherent with font creation)
	VFont *font = RetainFont( inFontFamilyName, inFace, inSize, inDPI, true);
	if (font)
	{
		if (!font->IsTrueTypeFont() || (!inGC || inGC->IsGDIImpl()))
		{
			//not TrueType fonts are drawed with GDI (only if inGC->GetTextRenderingMode() & TRM_LEGACY_OFF == 0)
			//so check with GDI font family name here

			if (inGC && !inGC->IsGDIImpl() && (inGC->GetTextRenderingMode() & TRM_LEGACY_OFF))
				return false; //no fallback to GDI:	as not vectorial font is not supported by DWrite or GDIPlus, return false
				
			HFONT hFont = font->GetFontRef();
			
			LOGFONTW lf;
			int ok = ::GetObjectW(hFont, sizeof(LOGFONTW), &lf);
			if (!ok)
				return false;
			WCHAR *familyName = &(lf.lfFaceName[0]);
			
			size_t familyNameLength = wcslen(familyName);
			exists = wcsncmp(inFontFamilyName.GetCPointer(), familyName, familyNameLength) == 0 && (inFontFamilyName.GetLength() == familyNameLength || inFontFamilyName[familyNameLength] == ' ');
			if (exists)
				exists = VFont::FontGDIExists(inFontFamilyName); //logfont does not contain substituted font family if font is not supported but only design font family: we need to check with really supported fonts

			//JQ 02/04/2014: fixed ACI0086730 - special case for "Eras ITC" & "DINOT-Black" - for GDI family name is "DINOT-Black" and for D2D it is "DIN OT"...
			if (!exists)
			{
				if (inFontFamilyName.EqualToString("DIN OT"))
				{
					VString familyName2 = "DINOT-Black";
					exists = wcsncmp(familyName2.GetCPointer(), familyName, familyNameLength) == 0 && (familyName2.GetLength() == familyNameLength);
				}
				else if (inFontFamilyName.BeginsWith("Eras"))
					exists = wcsncmp(inFontFamilyName.GetCPointer(), familyName, 4) == 0 && (inFontFamilyName.GetLength() == 4 || inFontFamilyName[4] == ' ');
			}

			font->Release();
		}
#if ENABLE_D2D
		else if (inGC && inGC->IsD2DImpl())
		{
			//if DWrite font family name is not equal to inFontFamilyName
			//DWrite has returned a compatible font 
			//so return true only if DWrite font family name matches inFontFamilyName

			VTaskLock lock(&VWinD2DGraphicContext::GetMutexDWriteFactory());
			exists = false;
			if (font->GetImpl().GetDWriteTextFormat() != NULL)
			{
				UINT32 nameLength = font->GetImpl().GetDWriteTextFormat()->GetFontFamilyNameLength();
				if (nameLength+1 <= 80)
				{
					WCHAR familyName[80];
					HRESULT hr = font->GetImpl().GetDWriteTextFormat()->GetFontFamilyName( familyName, nameLength+1);
					//JQ 24/02/2014: fixed ACI0085448 - return true if passed font name starts with DWrite font family 
					size_t familyNameLength = wcslen(familyName);
					
					exists = wcsncmp(inFontFamilyName.GetCPointer(), familyName, familyNameLength) == 0 && (inFontFamilyName.GetLength() == familyNameLength || inFontFamilyName[familyNameLength] == ' ');
					if (!exists)
					{
						//JQ 22/05/2014: fixed ACI0086730: no matter if postscript or family name
						VString vFamilyName((const UniChar *)familyName);
						VString vinFamilyName(inFontFamilyName);
						vFamilyName.ExchangeAll('-',' ');
						vinFamilyName.ExchangeAll('-',' ');
						exists = wcsncmp(vinFamilyName.GetCPointer(), vFamilyName.GetCPointer(), familyNameLength) == 0 && (vinFamilyName.GetLength() == familyNameLength || vinFamilyName[familyNameLength] == ' ');
					}
									
					//JQ 02/04/2014: fixed ACI0086730 - special case for "Eras ITC" & "DINOT-Black" - for GDI family name is "DINOT-Black" and for D2D it is "DIN OT"...
					if (!exists)
					{
						if (inFontFamilyName.EqualToString("DINOT-Black"))
						{
							VString familyName2 = "DIN OT";
							exists = wcsncmp(familyName2.GetCPointer(), familyName, familyNameLength) == 0 && (familyName2.GetLength() == familyNameLength);
						}
						else if (inFontFamilyName.BeginsWith("Eras"))
							exists = wcsncmp(inFontFamilyName.GetCPointer(), familyName, 4) == 0 && (inFontFamilyName.GetLength() == 4 || inFontFamilyName[4] == ' ');
					}
				}
				else
				{
					WCHAR *familyName = new WCHAR[nameLength+1];
					font->GetImpl().GetDWriteTextFormat()->GetFontFamilyName( familyName, nameLength+1);
					//JQ 24/02/2014: fixed ACI0085448 - return true if passed font name starts with DWrite font family 
					size_t familyNameLength = wcslen(familyName);
					exists = wcsncmp(inFontFamilyName.GetCPointer(), familyName, familyNameLength) == 0 && (inFontFamilyName.GetLength() == familyNameLength || inFontFamilyName[familyNameLength] == ' ');
					if (!exists)
					{
						//JQ 22/05/2014: fixed ACI0086730: no matter if postscript or family name
						VString vFamilyName((const UniChar *)familyName);
						VString vinFamilyName(inFontFamilyName);
						vFamilyName.ExchangeAll('-',' ');
						vinFamilyName.ExchangeAll('-',' ');
						exists = wcsncmp(vinFamilyName.GetCPointer(), vFamilyName.GetCPointer(), familyNameLength) == 0 && (vinFamilyName.GetLength() == familyNameLength || vinFamilyName[familyNameLength] == ' ');
					}
					delete[] familyName;
				}
			}
			font->Release();
		}
 #endif
        else
        {
            //if gdiplus font family name is not equal to inFontFamilyName
            //gdiplus has returned a compatible font
            //so return true only if gdiplus font family name matches inFontFamilyName
            Gdiplus::FontFamily *family = new Gdiplus::FontFamily();
            Gdiplus::Status status = font->GetImpl().GetGDIPlusFont()->GetFamily(family);
            WCHAR familyName[LF_FACESIZE];
            family->GetFamilyName( familyName);
            
            //JQ 24/02/2014: fixed ACI0085448 - return true if passed font name starts with Gdiplus font family
            size_t familyNameLength = wcslen(familyName);
            exists = wcsncmp(inFontFamilyName.GetCPointer(), familyName, familyNameLength) == 0 && (inFontFamilyName.GetLength() == familyNameLength || inFontFamilyName[familyNameLength] == ' ');
 			if (!exists)
			{
				//JQ 22/05/2014: fixed ACI0086730: no matter if postscript or family name
				VString vFamilyName((const UniChar *)familyName);
				VString vinFamilyName(inFontFamilyName);
				vFamilyName.ExchangeAll('-',' ');
				vinFamilyName.ExchangeAll('-',' ');
				exists = wcsncmp(vinFamilyName.GetCPointer(), vFamilyName.GetCPointer(), familyNameLength) == 0 && (vinFamilyName.GetLength() == familyNameLength || vinFamilyName[familyNameLength] == ' ');
			}
           delete family;
            
			//JQ 02/04/2014: fixed ACI0086730 - special case for "Eras ITC" & "DINOT-Black" - for GDI family name is "DINOT-Black" and for D2D it is "DIN OT"...
			if (!exists)
			{
				if (inFontFamilyName.EqualToString("DINOT-Black"))
				{
					VString familyName2 = "DIN OT";
					exists = wcsncmp(familyName2.GetCPointer(), familyName, familyNameLength) == 0 && (familyName2.GetLength() == familyNameLength);
				}
				else if (inFontFamilyName.BeginsWith("Eras"))
					exists = wcsncmp(inFontFamilyName.GetCPointer(), familyName, 4) == 0 && (inFontFamilyName.GetLength() == 4 || inFontFamilyName[4] == ' ');
			}

            font->Release();            
        }
	}
#else
	VFont *font = RetainFont( inFontFamilyName, inFace, inSize, inDPI, true);
	if (font)
	{
		//ensure returned font has the requested family name (here we check only the family name but not the full name)
		//otherwise it is a font substitute
		CTFontRef ctfont = font->GetFontRef();
		CFStringRef name = CTFontCopyFamilyName(ctfont);
		VString xname;
		xname.MAC_FromCFString( name);
		CFRelease(name);
		VString inname( inFontFamilyName);
		inname.Truncate( xname.GetLength());
		exists = inname.EqualToString( xname);
	}
#endif
    if (!exists)
    {
        //fixed ACI0086991 (DEV)
        //if passed font name is not normalized (ascii english characters only as returned with CTFontCopyFamilyName)
        //we consider it is valid as checking localized name for all possible languages would hit performance
        //(as method is used only by SVG component for now it should not have side effects but for SVG and workaround is acceptable for SVG)
        
        const UniChar *c = inFontFamilyName.GetCPointer();
        bool isASCII = true;
        while (isASCII && *c)
        {
            if (*c >= 128)
                isASCII = false;
            else
                c++;
        }
        if (!isASCII)
            exists = true;
    }
    return exists;
}


GReal	VFont::GetPixelSize () const
{
#if VERSIONWIN
	if (fPixelSize != 0.0f)
		return fPixelSize;

#if ENABLE_D2D
	fPixelSize = D2D_PointToDIP( fSize);
#else
	fPixelSize = PointsToPixels( fSize);
#endif
	return fPixelSize;

#else
	return fSize;
#endif
}


VFont* VFont::DeriveFontSize(GReal inSize, const GReal inDPI) const
{
	VFont* result=sManager->RetainFont(fDesignName, fFace, inSize, inDPI, false);
	if(result && fFontIDValid)
		result->SetFontID(fFontID);
	return result;
}


VFont* VFont::DeriveFontFace(const VFontFace& inFace) const
{
	VFont* result = sManager->RetainFont(fDesignName, inFace, fSize, 0, false);
	if(result && fFontIDValid)
		result->SetFontID(fFontID);
	return result;
}


void VFont::DeInit()
{
	if(sManager != NULL)
	{
		delete sManager;
		sManager = NULL;
	}
}

VFont *VFont::RetainFontFromStream(VStream *inStream)
{	
	xbox_assert(sManager != NULL);
	
	VStr63 familyName;
	familyName.ReadFromStream(inStream);
	VFontFace face =(VFontFace) inStream->GetLong();
	GReal size = (GReal) inStream->GetReal();
	
	VFont *theFont;
	if(inStream->GetLastError() == VE_OK)
		theFont = sManager->RetainFont(familyName, face, size);
	else
		theFont = sManager->RetainStdFont(STDF_CAPTION);
	
	return theFont;
}


VError	VFont::WriteToStream(VStream *ioStream, sLONG /*inParam*/) const
{
	fDesignName.WriteToStream(ioStream);
	ioStream->PutLong(fFace);
	return ioStream->PutReal(fSize);
}



bool VFont::Match(const VString& inFontFamilyName, const VFontFace& inFace, GReal inSize) const
{
	return fDesignName == inFontFamilyName && inFace == fFace && inSize == fSize;
}


void VFont::ApplyToPort(PortRef inPort, GReal inScale) 
{
	fFont.ApplyToPort(inPort, inScale);
}


bool VFont::Match(StdFont inFont) const
{
	return fStdFont == inFont;
}


bool VFont::Init()
{
	if(sManager == NULL)
		sManager = new VFontMgr;

	return(sManager != NULL);
}


VFont::~VFont()
{
}


#pragma mark-

VFontMetrics::VFontMetrics( VFont* inFont, const VGraphicContext* inContext)
: fUseLegacyMetrics(false)
, fDesiredUseLegacyMetrics(false)
, fFont( inFont)
, fContext( inContext)
, fMetrics( this)
{
	// sc 11/05/2007 the graphic context is the owner of the VFontMetrics object, so never retain the graphic context.
	xbox_assert(inFont != NULL);

	fFont->Retain();
#if VERSIONWIN
	if (inContext)
	{
		if (!inContext->IsD2DImpl())
			fUseLegacyMetrics = true; //GDIPlus metrics = GDI metrics = legacy metrics
		else
		{
			fUseLegacyMetrics = !fFont->IsTrueTypeFont();	//by default, use GDI metrics for not truetype font, otherwise use D2D metrics
															//(because D2D impl draws not truetype font using legacy GDI as it cannot draw natively these kind of font)
			TextRenderingMode mode = inContext->GetTextRenderingMode();
			if (mode & TRM_LEGACY_ON)
				fUseLegacyMetrics = true; //force only GDI metrics (because text is rendered only with GDI)
			else if (mode & TRM_LEGACY_OFF)
				fUseLegacyMetrics = false; //force only impl metrics (because text is rendered only with D2D impl)
		}
	}
	else
		//you should always pass a context (otherwise metrics are set to 0) but just in case...
		fUseLegacyMetrics = true;
#else
	fUseLegacyMetrics = true;
	fDesiredUseLegacyMetrics = true;
#endif

	_GetMetrics( fAscent, fDescent, fInternalLeading, fExternalLeading,fLineHeight);
	if(fLineHeight==0)
		fLineHeight = fAscent + fDescent + fExternalLeading;

#if VERSIONMAC
	if ( inContext )
		fMetrics.GetTextFactory()->SetRenderingMode( inContext->GetTextRenderingMode());
#endif
}


void VFontMetrics::_GetMetrics( GReal &outAscent, GReal &outDescent, GReal &outInternalLeading, GReal &outExternalLeading , GReal &outLineHeight)
{
#if VERSIONWIN
	sLONG dpi = fContext ? (sLONG)fContext->GetDpiX() : (sLONG)VWinGDIGraphicContext::GetLogPixelsX();
	bool useLegacy = fUseLegacyMetrics || !fContext || !fContext->IsD2DImpl();
#elif VERSIONMAC
	sLONG dpi = fContext ? (sLONG)fContext->GetDpiX() : 72;
#else
	sLONG dpi = fContext ? (sLONG)fContext->GetDpiX() : 96; //use 96 as default on Linux ?
#endif

	VFont::MapOfCachedFontMetricsPerDPI::iterator itMetrics = fFont->fMapOfCachedFontMetrics.find(dpi);
	if (itMetrics != fFont->fMapOfCachedFontMetrics.end())
	{
		//there is a cached metrics entry for this dpi: check first if legacy or standard metrics are effectively stored yet
#if VERSIONWIN
#if ENABLE_D2D
		if (useLegacy)
#else
		if (true)
#endif
		{
			if (itMetrics->second.IsLegacyInitialized())
			{
				itMetrics->second.GetLegacy( outAscent, outDescent, outInternalLeading, outExternalLeading, outLineHeight);
				return;
			}
		}
		else
#endif
			if (itMetrics->second.IsInitialized())
			{
				itMetrics->second.Get( outAscent, outDescent, outInternalLeading, outExternalLeading,outLineHeight);
				return;
			}

		//if not, compute it
		fMetrics.GetMetrics( outAscent, outDescent, outInternalLeading, outExternalLeading,outLineHeight);

		//and store metrics in the cached metrics entry
#if VERSIONWIN
#if ENABLE_D2D
		if (useLegacy)
#else
		if (true)
#endif
			itMetrics->second.SetLegacy( outAscent, outDescent, outInternalLeading, outExternalLeading,outLineHeight);
		else
#endif
			itMetrics->second.Set( outAscent, outDescent, outInternalLeading, outExternalLeading,outLineHeight);

		return;
	}

	//there is none cached metrics for this dpi: compute metrics & store new entry in VFont internal map

	fMetrics.GetMetrics( outAscent, outDescent, outInternalLeading, outExternalLeading,outLineHeight);

	VCachedFontMetrics thisCachedmetrics;

#if VERSIONWIN
#if ENABLE_D2D
	if (useLegacy)
#else
	if (true)
#endif
		thisCachedmetrics.SetLegacy( outAscent, outDescent, outInternalLeading, outExternalLeading,outLineHeight);
	else
#endif
		thisCachedmetrics.Set( outAscent, outDescent, outInternalLeading, outExternalLeading,outLineHeight);

	fFont->fMapOfCachedFontMetrics[dpi] = thisCachedmetrics;
}


/** copy constructor */
VFontMetrics::VFontMetrics( const VFontMetrics& inFontMetrics):fMetrics( this)
{
	fContext = inFontMetrics.fContext;
	fFont = NULL;

	CopyRefCountable(&fFont, inFontMetrics.fFont);

	fAscent = inFontMetrics.fAscent;
	
	fDescent = inFontMetrics.fDescent;
	fInternalLeading = inFontMetrics.fInternalLeading;
	fExternalLeading = inFontMetrics.fExternalLeading;
	fUseLegacyMetrics = inFontMetrics.fUseLegacyMetrics;
	fDesiredUseLegacyMetrics = inFontMetrics.fDesiredUseLegacyMetrics;
}


/** use legacy (GDI) context metrics or actuel context metrics 
@remarks
	if actual context is D2D: if true, use GDI font metrics otherwise use Direct2D font metrics 
	any other context: use GDI metrics on Windows

	if context text rendering mode has TRM_LEGACY_ON, inUseLegacyMetrics is replaced by true
	if context text rendering mode has TRM_LEGACY_OFF, inUseLegacyMetrics is replaced by false
	(so context text rendering mode overrides this setting)
*/
void VFontMetrics::UseLegacyMetrics( bool inUseLegacyMetrics)
{
#if VERSIONWIN
	fDesiredUseLegacyMetrics = inUseLegacyMetrics;
	if ((!fContext) || (!fContext->IsD2DImpl()))
	{
		//GDIPlus metrics = GDI metrics = legacy metrics
		if (!fUseLegacyMetrics)
		{
			fUseLegacyMetrics = true;
			_GetMetrics( fAscent, fDescent, fInternalLeading, fExternalLeading,fLineHeight);
		}
		return;
	}

	TextRenderingMode mode = fContext->GetTextRenderingMode();
	if (mode & TRM_LEGACY_ON)
		inUseLegacyMetrics = true;
	else if (mode & TRM_LEGACY_OFF)
		inUseLegacyMetrics = false;
	if (fUseLegacyMetrics == inUseLegacyMetrics)
		return;

	fUseLegacyMetrics = inUseLegacyMetrics;
	_GetMetrics( fAscent, fDescent, fInternalLeading, fExternalLeading,fLineHeight);
#endif
}

VFontMetrics::~VFontMetrics()
{
	fFont->Release();
}


bool VFontMetrics::MousePosToTextPos(const VString& inString, GReal inMousePos, sLONG& ioTextPos, GReal& ioPixPos) 
{
	if (inString.IsEmpty())
		return false;
	
	return fMetrics.MousePosToTextPos( inString, inMousePos, ioTextPos, ioPixPos);
}


void VFontMetrics::MeasureText( const VString& inString, std::vector<GReal>& outOffsets, bool inRTL)
{
	outOffsets.clear();
	if (!inString.IsEmpty())
		fMetrics.MeasureText( inString, outOffsets, inRTL);
}


/** scale metrics by the specified factor */
void VFontMetrics::Scale( GReal inScaling)
{
	fAscent *= inScaling;
	fDescent *= inScaling;
	fInternalLeading *= inScaling;
	fExternalLeading *= inScaling;
	fLineHeight *= inScaling;
}

// donne la hauteur de la police en utilisant la meme (presque) metode d'arrondi quickdraw 

static sLONG _round(XBOX::GReal inValue)
{
	XBOX::GReal v1;
	XBOX::GReal v = std::modf(inValue,&v1);
	if(v>0.5)
		return std::ceil(inValue);
	else 
		return v1;
}

sLONG VFontMetrics::GetNormalizedTextHeight() const			
{ 
	return _round(fAscent) + _round(fDescent);
}
sLONG VFontMetrics::GetNormalizedLineHeight() const			
{ 
	return _round(fAscent) + _round(fDescent) + _round(fExternalLeading);
}


VCachedFontMetrics::VCachedFontMetrics(const VCachedFontMetrics& inMetrics)
{
#if VERSIONWIN
	fLegacyInitialized = inMetrics.fLegacyInitialized;
	if (fLegacyInitialized)
	{
		fLegacyMetrics.fAscent				= inMetrics.fLegacyMetrics.fAscent;
		fLegacyMetrics.fDescent				= inMetrics.fLegacyMetrics.fDescent;
		fLegacyMetrics.fInternalLeading 	= inMetrics.fLegacyMetrics.fInternalLeading;
		fLegacyMetrics.fExternalLeading 	= inMetrics.fLegacyMetrics.fExternalLeading;
		fLegacyMetrics.fLineHeight 			= inMetrics.fLegacyMetrics.fLineHeight;
	}
#endif				
	fInitialized = inMetrics.fInitialized;
	if (fInitialized)
	{
		fMetrics.fAscent			= inMetrics.fMetrics.fAscent;
		fMetrics.fDescent			= inMetrics.fMetrics.fDescent;
		fMetrics.fInternalLeading	= inMetrics.fMetrics.fInternalLeading;
		fMetrics.fExternalLeading	= inMetrics.fMetrics.fExternalLeading;
		fMetrics.fLineHeight		= inMetrics.fMetrics.fLineHeight;
	}
}

VCachedFontMetrics& VCachedFontMetrics::operator =(const VCachedFontMetrics& inMetrics)
{
	if (this == &inMetrics)
		return *this;
#if VERSIONWIN
	fLegacyInitialized = inMetrics.fLegacyInitialized;
	if (fLegacyInitialized)
	{
		fLegacyMetrics.fAscent				= inMetrics.fLegacyMetrics.fAscent;
		fLegacyMetrics.fDescent				= inMetrics.fLegacyMetrics.fDescent;
		fLegacyMetrics.fInternalLeading 	= inMetrics.fLegacyMetrics.fInternalLeading;
		fLegacyMetrics.fExternalLeading 	= inMetrics.fLegacyMetrics.fExternalLeading;
		fLegacyMetrics.fLineHeight		 	= inMetrics.fLegacyMetrics.fLineHeight;
	}
#endif				
	fInitialized = inMetrics.fInitialized;
	if (fInitialized)
	{
		fMetrics.fAscent			= inMetrics.fMetrics.fAscent;
		fMetrics.fDescent			= inMetrics.fMetrics.fDescent;
		fMetrics.fInternalLeading	= inMetrics.fMetrics.fInternalLeading;
		fMetrics.fExternalLeading	= inMetrics.fMetrics.fExternalLeading;
		fMetrics.fLineHeight		= inMetrics.fMetrics.fLineHeight;
	}
	return *this;
}

#if VERSIONWIN
void VCachedFontMetrics::SetLegacy( const GReal inAscent, const GReal inDescent, const GReal inInternalLeading, const GReal inExternalLeading, const GReal inLineHeight)
{	
	fLegacyMetrics.fAscent			= inAscent;
	fLegacyMetrics.fDescent			= inDescent;
	fLegacyMetrics.fInternalLeading = inInternalLeading;
	fLegacyMetrics.fExternalLeading = inExternalLeading;
	fLegacyMetrics.fLineHeight		= inLineHeight;
	fLegacyInitialized = true;
}
void VCachedFontMetrics::GetLegacy( GReal &outAscent, GReal &outDescent, GReal &outInternalLeading, GReal &outExternalLeading , GReal &outLineHeight) const
{
	outAscent						= fLegacyMetrics.fAscent;
	outDescent						= fLegacyMetrics.fDescent;
	outInternalLeading				= fLegacyMetrics.fInternalLeading;
	outExternalLeading				= fLegacyMetrics.fExternalLeading;
	outLineHeight					= fLegacyMetrics.fLineHeight;
}
#endif

void VCachedFontMetrics::Set( const GReal inAscent, const GReal inDescent, const GReal inInternalLeading, const GReal inExternalLeading,const GReal inLineHeight)
{	
	fMetrics.fAscent			= inAscent;
	fMetrics.fDescent			= inDescent;
	fMetrics.fInternalLeading	= inInternalLeading;
	fMetrics.fExternalLeading	= inExternalLeading;
	fMetrics.fLineHeight		= inLineHeight;
	fInitialized = true;
}
void VCachedFontMetrics::Get( GReal &outAscent, GReal &outDescent, GReal &outInternalLeading, GReal &outExternalLeading , GReal &outLineHeight) const
{
	outAscent						= fMetrics.fAscent;
	outDescent						= fMetrics.fDescent;
	outInternalLeading				= fMetrics.fInternalLeading;
	outExternalLeading				= fMetrics.fExternalLeading;
	outLineHeight					= fMetrics.fLineHeight;
}


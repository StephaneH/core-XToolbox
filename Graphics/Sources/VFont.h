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
#ifndef __VFont__
#define __VFont__

#include "Graphics/Sources/VFontMgr.h"

// Native declarations
#if VERSIONMAC
    #include "Graphics/Sources/XMacFont.h"
#elif VERSIONWIN
    #include "Graphics/Sources/XWinFont.h"
#endif

BEGIN_TOOLBOX_NAMESPACE

// Needed declarations
class VFont;
class VFontMetrics;
class VGraphicContext;

/** generic font type definition (as defined by CSS2 W3C: see http://www.w3.org/TR/CSS2/fonts.html#generic-font-families) */
typedef enum eGenericFont
{
	GENERIC_FONT_SERIF = 0,			//for instance "Times New Roman" (proportional)
	GENERIC_FONT_SANS_SERIF = 1,	//for instance "Arial"			(proportional)
	GENERIC_FONT_CURSIVE = 2,		//for instance "Adobe Poetica"	(handwritten-like)
	GENERIC_FONT_FANTASY = 3,		//for instance "Critter"	(mixing characters and decorations)
	GENERIC_FONT_MONOSPACE = 4		//for instance "Courier" (not proportional)

} eGenericFont;



/** this class is used to stored both precomputed standard & legacy font metrics - legacy (i.e. GDI) only for Windows - for a fixed DPI */
class VCachedFontMetrics: public VObject
{
public:
			VCachedFontMetrics():VObject() 
			{ 
#if VERSIONWIN
				fLegacyInitialized = false; 
#endif
				fInitialized = false; 
			}
	virtual	~VCachedFontMetrics() {}

			VCachedFontMetrics(const VCachedFontMetrics& inMetrics);

			VCachedFontMetrics& operator =(const VCachedFontMetrics& inMetrics);

#if VERSIONWIN
			bool				IsLegacyInitialized() const { return fLegacyInitialized; }
			void				SetLegacy( const GReal inAscent, const GReal inDescent, const GReal inInternalLeading, const GReal inExternalLeading,const GReal inLineHeight);
			void				GetLegacy( GReal &outAscent, GReal &outDescent, GReal &outInternalLeading, GReal &outExternalLeading,GReal &outLineHeight) const;
#endif
			bool				IsInitialized() const { return fInitialized; }
			void				Set( const GReal inAscent, const GReal inDescent, const GReal inInternalLeading, const GReal inExternalLeading,const GReal inLineHeight);
			void				Get( GReal &outAscent, GReal &outDescent, GReal &outInternalLeading, GReal &outExternalLeading, GReal &ouLineHeight) const;

protected:
			struct BaseFontMetrics
			{
			public:
						GReal				fAscent;
						GReal				fDescent;
						GReal				fInternalLeading;
						GReal				fExternalLeading;
						GReal				fLineHeight;
			};

#if VERSIONWIN
			bool				fLegacyInitialized;
			BaseFontMetrics		fLegacyMetrics;
#endif
			bool				fInitialized;
			BaseFontMetrics		fMetrics;
};

class XTOOLBOX_API VFont : public VObject, public IRefCountable
{
friend class VFontMgr;
friend class VFontMetrics;
#if VERSIONWIN
friend class XWinFont;
#endif

public:
	static	bool				Init ();
	static	void				DeInit ();

								VFont (const VString& inFontFamilyName, const VFontFace& inFace, GReal inSize, StdFont inStdFont = STDF_NONE, GReal inPixelSize = 0.0f);
	virtual						~VFont ();
	
			/** return true if the specified font exists on the current system 
			@remarks
				does not create font
			*/
	static	bool				FontExists(const VString& inFontFamilyName, const VFontFace& inFace, GReal inSize, const GReal inDPI = 0, const VGraphicContext *inGC = NULL);

			/** return the best existing font family name on the current system which matches the specified generic ID 
			@see
				eGenericFont
			*/
	static	void				GetGenericFontFamilyName(eGenericFont inGenericID, VString& outFontFamilyName, const VGraphicContext *inGC = NULL);

	// Creation 
	static	VFont*				RetainFont (const VString& inFontFamilyName, const VFontFace& inFace, GReal inSize, const GReal inDPI = 0, bool inReturnNULLIfNotExist = false) { assert(sManager != NULL); return sManager->RetainFont(inFontFamilyName, inFace, inSize, inDPI, inReturnNULLIfNotExist); };
	static	VFont*				RetainStdFont (StdFont inFont) { assert(sManager != NULL); return sManager->RetainStdFont(inFont); }

	static	VFont*				RetainFontFromStream ( VStream *inStream);
			VError				WriteToStream( VStream *ioStream, sLONG inParam = 0) const;

			VFont*				DeriveFontSize (GReal inSize, const GReal inDPI = 0) const;
			VFont*				DeriveFontFace (const VFontFace& inFace) const;

			void				ApplyToPort (PortRef inPort, GReal inScale);
	
			// Font matching
			bool				Match (const VString& inFontFamilyName, const VFontFace& inFace, GReal inSize) const;
			bool				Match (StdFont inFont) const;
	
			// Accessors
			const VString&		GetName () const		{ return fFamilyName; }
			const VFontFace&	GetFace () const		{ return fFace; }

			/** return size in point */
			GReal				GetSize () const		{ return fSize; }

			/** return size in pixel (according to impl dpi - which is res independent 72 on Mac and res dependent screen dpi on Windows)
			@remarks
				always equal to 4D form font size (because 4D form dpi = 72)

				on Windows = font size in point*screen dpi/72  = GetSize()*screen dpi/72
				on Mac = font size in point (as Quartz2D dpi = 72) = GetSize()
			*/
			GReal				GetPixelSize () const;

			bool				GetFontID(sWORD& outID)const	{outID=fFontID;return fFontIDValid;}
			void				SetFontID(sWORD inID)			{fFontID = inID;fFontIDValid=true;}

			
			FontRef				GetFontRef () const		{ return fFont.GetFontRef(); }
			FontRef				GetFontNative() const	{ return fFont.GetFontRef(); }
			
			Boolean				IsTrueTypeFont() { return fFont.IsTrueTypeFont();}
			
			// Native operator
			operator FontRef () const					{ return GetFontRef(); }
	
			// Utilities
	
	static	void				GetFontNameList (VectorOfVString& outFontNames, bool inWithScreenFonts = true) { xbox_assert(sManager != NULL); sManager->GetFontNameList(outFontNames, inWithScreenFonts); }
	static	void				GetFontFullNameList(VectorOfVString& outFontNames, bool inWithScreenFonts = true) { xbox_assert(sManager != NULL); sManager->GetFontFullNameList(outFontNames, inWithScreenFonts); }
	static	void				GetUserActiveFontNameList (VectorOfVString& outFontNames) { xbox_assert(sManager != NULL); sManager->GetUserActiveFontNameList(outFontNames); }
	static	void				GetFontNameListForContextualMenu (VectorOfVString& outFontNames) { xbox_assert(sManager != NULL); sManager->GetFontNameListForContextualMenu(outFontNames); }

								/** add font name to the list of last used font names */
	static	void				AddLastUsedFontName( const VString& inFontName) { xbox_assert(sManager != NULL); sManager->AddLastUsedFontName( inFontName); }		
								
								/** get last used font names */
	static	void				GetLastUsedFontNameList(VectorOfVString& outFontNames) { xbox_assert(sManager != NULL); sManager->GetLastUsedFontNameList( outFontNames); }
		
								/** clear list of last used font names */
	static	void				ClearLastUsedFontNameList() { xbox_assert(sManager != NULL); sManager->ClearLastUsedFontNameList(); }

                                /** localize font name (if font name is not localized) according to current system UI language */
    static  void                LocalizeFontName(VString& ioFontName) { xbox_assert(sManager != NULL); sManager->LocalizeFontName(ioFontName); }
    
			const XFontImpl&	GetImpl () const { return fFont; }


protected:
			typedef std::map<sLONG,VCachedFontMetrics> MapOfCachedFontMetricsPerDPI;

#if VERSIONWIN
			GReal				_GetPixelSize() const { return fPixelSize; }
#endif

	static VFontMgr*			sManager;
	
			VString				fFamilyName;
			VFontFace			fFace;
			GReal				fSize;
#if VERSIONWIN
	mutable GReal				fPixelSize;
#endif
			StdFont				fStdFont;
			
			// optimization pour l'ancien code qui utilsie tj des fontid quickdraw
			// pour eviter d'appler GetFnum trop souvent, on stock l'id quickdraw dans la vfont une fois pour toute.
			// Cet id est conservé lors de la derivation de la fonte, il n'est jamais utilisé en interne. 
			bool				fFontIDValid; 
			sWORD				fFontID;
			
			XFontImpl			fFont;

			MapOfCachedFontMetricsPerDPI fMapOfCachedFontMetrics;
};


/** font metrics
@remarks
	for D2D/DWrite impl, metrics are in DIP (1/96e inches)
	for GDIPlus & GDI, metrics are in pixels
	for Quartz2D, metrics are in pt = 1/72e inches
*/
class XTOOLBOX_API VFontMetrics : public VObject
{
public:
			
							VFontMetrics( VFont* inFont, const VGraphicContext* inContext); // Copies the TextRenderingMode set in VGraphicContext
	virtual					~VFontMetrics();
	
			/** copy constructor */
							VFontMetrics( const VFontMetrics& inFontMetrics);

			// Computation
			GReal			GetTextWidth( const VString& inString, bool inRTL = false)	{ return inString.IsEmpty() ? (GReal) 0.0f : fMetrics.GetTextWidth( inString, inRTL);}
			GReal			GetCharWidth( UniChar inChar)					{ return fMetrics.GetCharWidth( inChar);}

			// return true if the inMousePos is in the text
			// if the MousePos is in the text :
			//		ioTextPos is added by the character position nearest of inMousePos
			//		ioPixPos is added by this character position in pixels.
			// else
			//		ioTextPos is added by the len of the text in chars
			//		ioPixPos is added by the len of the text in pixels

			bool			MousePosToTextPos( const VString& inString, GReal inMousePos, sLONG& ioTextPos, GReal& ioPixPos);

			void			MeasureText( const VString& inString, std::vector<GReal>& outOffsets, bool inRTL = false);
	
			GReal			GetAscent() const				{ return fAscent;}
			GReal			GetDescent() const				{ return fDescent;}
			GReal			GetLeading() const				{ return fExternalLeading;}			
			GReal			GetInternalLeading() const		{ return fInternalLeading;} //remark: internal leading is included in ascent
			GReal			GetExternalLeading() const		{ return fExternalLeading;}

			GReal			GetTextHeight() const			{ return fAscent + fDescent;}
			GReal			GetLineHeight() const			{return fLineHeight;}
			
			// donne la hauteur de la police en utilisant la meme (presque) metode d'arrondi quickdraw 
			sLONG			GetNormalizedTextHeight() const;
			sLONG			GetNormalizedLineHeight() const	;
			
			// Accessors
			VFont*			GetFont() const					{ return fFont; }
			const VGraphicContext*	GetContext() const		{ return fContext; }
	
	
			/** scale metrics by the specified factor */
			void			Scale( GReal inScaling); 
	
			XFontMetricsImpl&	GetImpl()					{ return fMetrics; }

			/** use legacy (GDI) context metrics or actuel context metrics 
			@remarks
				if actual context is D2D: if true, use GDI font metrics otherwise use Direct2D font metrics 
				any other context: use GDI metrics on Windows

				if context text rendering mode has TRM_LEGACY_ON, inUseLegacyMetrics is replaced by true
				if context text rendering mode has TRM_LEGACY_OFF, inUseLegacyMetrics is replaced by false
				(so context text rendering mode overrides this setting)
			*/
			void			UseLegacyMetrics( bool inUseLegacyMetrics);

			bool			UseLegacyMetrics() const		{ return fUseLegacyMetrics; }

			bool			GetDesiredUseLegacyMetrics() const	{ return fDesiredUseLegacyMetrics; }
private:
			void			_GetMetrics( GReal &outAscent, GReal &outDescent, GReal &outInternalLeading, GReal &outExternalLeading ,GReal &outLineHeight);

			const VGraphicContext*	fContext;
			VFont*					fFont;
			GReal					fAscent;
			GReal					fDescent;
			GReal					fInternalLeading;
			GReal					fExternalLeading;
			GReal					fLineHeight;
			XFontMetricsImpl		fMetrics;
			bool					fDesiredUseLegacyMetrics;
			bool					fUseLegacyMetrics;
};


END_TOOLBOX_NAMESPACE

#endif

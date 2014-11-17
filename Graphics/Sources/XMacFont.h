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
#ifndef __XMacFont__
#define __XMacFont__

#include "VRect.h"
#include "VColor.h"

BEGIN_TOOLBOX_NAMESPACE

//set to 1 to use NS obj-c instances but CF C++ instances for CF objects which are not toll-free bridged
//(set to 1 if you want to enable RTF & HTML support 
// - in that case XMacFont.cpp & XMacStyledTextBox.cpp must be compiled as Obj-C++ source (FileType sourcecode.cpp.objcpp)
//   headers are still C++ only))
#define CT_USE_COCOA 1

// Needed declarations
class VFontMetrics;
#ifndef VGraphicContext
class VGraphicContext;
#endif

/** style range type definition */
typedef std::pair<SInt32,SInt32> CTUserStyleRange;

/** style range vector type definition */
typedef std::vector<CTUserStyleRange> CTUserStyleRangeVector;


/** Core Text helper class */
class XTOOLBOX_API CTHelper
{
public:
	/** create and retain Core Text string attributes using default attributes from the specified font, text rendering mode and layout mode */
	static CFMutableDictionaryRef CreateStringAttributes(  const VFont *inFont, const VColor& inColorFill, TextRenderingMode inMode = TRM_NORMAL, TextLayoutMode inLayoutMode = TLM_NORMAL, GReal inCharKerning = (GReal) 0.0);
	
	/** create and return attributed string from the specified text string and attributes */
	static CFAttributedStringRef CreateAttributedString( const VString& inText, CFDictionaryRef inAttributes, CFDictionaryRef inAttributesLatin = NULL, CTUserStyleRangeVector *inLatinStyleRanges = NULL);
	
	/** create Core Text line from the specified text string and string attributes */
	static CTLineRef CreateLine(const VString& inText, CFDictionaryRef inAttributes, CFDictionaryRef inAttributesLatin = NULL, CTUserStyleRangeVector *inLatinStyleRanges = NULL);
	
	/** create Core Text frame from the specified text string, string attributes and frame rect */
	static CTFrameRef CreateFrame(const VString& inText, CFDictionaryRef inAttributes, const VRect& inBounds = VRect(0,0,0,0), CFDictionaryRef inAttributesLatin = NULL, CTUserStyleRangeVector *inLatinStyleRanges = NULL);
	
	/** get line width 
	 @remarks
		here line is assumed to be drawed horizontally
	 */
	static GReal GetLineTypographicWidth( CTLineRef inLine);
	
	/** get line bounds 
	 @remarks
		here line is assumed to be drawed horizontally
	 */
	static void GetLineTypographicBounds( CTLineRef inLine, VRect& outBounds, bool inIncludeLeading = true);
	
	/** get frame typographic bounds 
	 @remarks
		note that it is not equal to frame path bounds but 
		to the union of lines typographic bounds
	 
	 @note
		Core Text CTFrame is buggy in vertical mode:
		line typographic height and so frame typographic height is wrong while using vertical-oriented glyphs
		so in vertical mode, it is recommended to estimate frame height by using frame count of lines
		- CTHelper::GetFrameLineCount - and font line height (= font ascent+descent+leading)
		which should work in most of the cases
		TODO: wait for a API fix from Apple
	 */
	static void GetFrameTypographicBounds( CTFrameRef inFrame, VRect& outBounds);
	
	/** get frame number of lines */
	static uint32_t GetFrameLineCount( CTFrameRef inFrame);
	
	/** get frame path bounds 
	 @remarks
		it is equal to the bounds of the path used to create the frame
	 */
	static void GetFramePathBounds( CTFrameRef inFrame, VRect& outBounds);
	
	/** set Core Text attribute of type Real */
	static void SetAttribute( CFMutableDictionaryRef inAttributes, CFStringRef inKey, GReal inValue);
	
	/** get Core Text attribute of type Real */
	static bool GetAttribute( CFDictionaryRef inAttributes, CFStringRef inKey, GReal &outValue);
	
	/** set Core Text attribute of type UINT32 */
	static void SetAttribute( CFMutableDictionaryRef inAttributes, CFStringRef inKey, uint32_t inValue);

	/** get Core Text attribute of type UINT32 */
	static bool GetAttribute( CFDictionaryRef inAttributes, CFStringRef inKey, uint32_t &outValue);

	/** set Core Text attribute of type SINT32 */
	static void SetAttribute( CFMutableDictionaryRef inAttributes, CFStringRef inKey, int32_t inValue);

	/** get Core Text attribute of type SINT32 */
	static bool GetAttribute( CFDictionaryRef inAttributes, CFStringRef inKey, int32_t &outValue);

	/** set Core Text attribute of type UINT8 */
	static void SetAttribute( CFMutableDictionaryRef inAttributes, CFStringRef inKey, uint8_t inValue);

	/** get Core Text attribute of type UINT8 */
	static bool GetAttribute( CFDictionaryRef inAttributes, CFStringRef inKey, uint8_t &outValue);

	/** set Core Text attribute of type SINT8 */
	static void SetAttribute( CFMutableDictionaryRef inAttributes, CFStringRef inKey, int8_t inValue);

	/** get Core Text attribute of type SINT8 */
	static bool GetAttribute( CFDictionaryRef inAttributes, CFStringRef inKey, int8_t &outValue);

	/** set Core Text attribute of type CFBoolean specified as bool */
	static void SetAttribute( CFMutableDictionaryRef inAttributes, CFStringRef inKey, bool inValue);

	/** get Core Text attribute of type CFBoolean as a bool */
	static bool GetAttribute( CFDictionaryRef inAttributes, CFStringRef inKey, bool &outValue);

	/** set Core Text attribute of type CFStringRef */
	static void SetAttribute( CFMutableDictionaryRef inAttributes, CFStringRef inKey, CFStringRef inValue);

	/** get Core Text attribute of type CFStringRef 
		@remarks
		caller does not own returned value
		(so do not call CFRelease on returned value)
	 */
	static bool GetAttribute( CFDictionaryRef inAttributes, CFStringRef inKey, CFStringRef &outValue);

	/** set Core Text attribute of type CFStringRef specified as a VString value */
	static void SetAttribute( CFMutableDictionaryRef inAttributes, CFStringRef inKey, const VString& inValue);

	/** get Core Text attribute of type CFStringRef as a VString value */
	static bool GetAttribute( CFDictionaryRef inAttributes, CFStringRef inKey, VString &outValue);

	/** set Core Text attribute of type CGColorRef specified as a VColor value */
	static void SetAttribute( CFMutableDictionaryRef inAttributes, CFStringRef inKey, const VColor& inValue);

	/** set Core Text attribute of type CGColorRef specified with red, green, blue and alpha components */
	static void SetAttribute( CFMutableDictionaryRef inAttributes, CFStringRef inKey, GReal inRed, GReal inGreen, GReal inBlue, GReal inAlpha = (GReal) 1.0);

	/** get Core Text attribute of type CGColorRef as a VColor value */
	static bool GetAttribute( CFDictionaryRef inAttributes, CFStringRef inKey, VColor &outValue);
	
	/** create NSColor * from VColor value  */
	static void *CreateNSColor( const VColor& inColor);
	
	/** get VColor from NSColor */
	static bool GetColor( void *inNSColor, VColor& outColor);
	
	/** convert a RTF stream to a CFAttributedStringRef */
	static CFMutableAttributedStringRef CreateAttributeStringFromRTF( CFDataRef inDataRTF);
	
	/** convert a HTML stream to a CFAttributedStringRef */
	static CFMutableAttributedStringRef CreateAttributeStringFromHTML( CFDataRef inDataHTML);
	
	/** convert a CFAttributedStringRef to a RTF stream */
	static CFDataRef CreateRTFFromAttributedString( CFAttributedStringRef inAttString);

#if CT_USE_COCOA	
	static bool UseCocoaAttributes() { return fUseCocoaAttributes; }
	
	/** enable Cocoa support for CoreText attributes 
	 @remarks
		after this method is called, CFAttributedString will be created from NSAttributedString & will store NSColor & NSParagraphStyle but CGColor & CTParagraphStyle
		(compatible with RTF & HTML NSAttributedString methods)
	 
		caution: caller must call DisableCocoa before caller method exits because this method lock/unlock current thread
	 */
	static void EnableCocoa();

	
	/** disable Cocoa support for CoreText attributes 
	 @remarks
		after this method is called, CFAttributedString will not be created from NSAttributedString & will store CGColor & CTParagraphStyle but NSColor & NSParagraphStyle
	 */
	static void DisableCocoa();
private:
	/** true for some tasks like using Cocoa RTF or HTML import/export 
	@remarks
		should be FALSE (default) if attributes are used for drawing in a CGContext (otherwise many context errors)
	 */
	static bool fUseCocoaAttributes;
	static VCriticalSection fMutex;
#endif
};


class XTOOLBOX_API XMacFont
{
public:
							XMacFont (VFont* inFont, const VString& inFontFamilyName, const VFontFace& inFace, GReal inSize);
							~XMacFont ()	
							{ 
								if (fCTStringAttributes)
									CFRelease(fCTStringAttributes); 
								if (fCTFontRef)
									CFRelease(fCTFontRef);  
							}
	
	// Context support
			void			ApplyToPort (PortRef inPort, GReal inScale);

	// Accessors
			bool			IsValid() const				{ return fCTFontRef != NULL; }
	
			FontRef			GetFontRef () const			{ return fCTFontRef; }
	
	// Native Accessors
	
			/** retain and return CTFont reference */
			CTFontRef		RetainCTFontRef () const;	
	
			/** retain and return Core Text default string attributes
				@remarks
					to draw a text using this font,
					apply first these attributes to the attributed string
	 
					caller must call CFRelease in order to release returned dictionary
			 */
			CFMutableDictionaryRef RetainStringAttributes() const; 
	
			void			GetPostScriptName (VString& outName) const;
	
			void			GetFamilyName (VString& outName) const;


	// metrics
			GReal			GetSize() const
			{
				xbox_assert(fCTFontRef);
				return ::CTFontGetSize( fCTFontRef);
			}
	
			GReal			GetAscent() const
			{
				xbox_assert(fCTFontRef);
				return ::CTFontGetAscent( fCTFontRef);
			}
			GReal			GetDescent() const
			{
				xbox_assert(fCTFontRef);
				return ::CTFontGetDescent( fCTFontRef);
			}
			GReal			GetLeading() const
			{
				xbox_assert(fCTFontRef);
				return ::CTFontGetLeading( fCTFontRef);
			}
	
			Boolean			IsTrueTypeFont() { return true;}
    
			/* return font name according to passed kind name (localized or not) */
			void			GetName (eFontNameKind inFontNameKind, VString& outName, bool inLocalized = true) const;

			/** font may be substituted: this method will return the real family name and not the one passed to RetainFont at font creation */
			void			GetRealFamilyName(VString& outName, const GraphicContextType inGCType = eDefaultGraphicContext, const TextRenderingMode inTRM = TRM_NORMAL) const;

	// Static Utils
	
	/** convert 4D font traits to Core Text font descriptor symbolic traits */
	static CTFontSymbolicTraits FontFaceToCTFontSymbolicTraits( const VFontFace& inFace);
	static VFontFace CTFontSymbolicTraitsToFontFace( const CTFontSymbolicTraits& inTraits);
	
	
	static	Style			FontFaceToStyle (const VFontFace& inFace);
	static	void			StyleToFontFace (Style inFace, VFontFace& outFace);
	
	static	FMFontStyle		FontFaceToFMFontStyle (const VFontFace& inFace);
	static	void			FMFontStyleToFontFace (FMFontStyle inFace, VFontFace& outFace);

#if WITH_QUICKDRAW
	static	sWORD			FontNameToFontNum (const VString& inFontName);
	static	void			FontNumToFontName (sWORD inFontFamilyID, VString& outFontName);
#endif

protected:
			VFont*					fFont;

			/** Core text font reference */
			CTFontRef				fCTFontRef;
	
			/** Core Text attributed string default attributes */
			CFMutableDictionaryRef	fCTStringAttributes;
};

typedef XMacFont XFontImpl;

/** Core Text factory class 
 @remarks
	help to create CTLine or CTFrame instances from a string based on text rendering mode and layout rendering mode
	help with text string metrics 
	
    class instance internally retains CTLine and CTFrame instances
    as long as the string, rendering mode or layout mode do not change
 */
class XMacFontMetrics;
class XTOOLBOX_API XMacTextFactory
{
public:
	XMacTextFactory (XMacFontMetrics* inMetrics, TextRenderingMode inMode = TRM_NORMAL, TextLayoutMode inLayoutMode = TLM_NORMAL):
	fMetrics( inMetrics),
	fRenderingMode(inMode),
	fLayoutMode(inLayoutMode),
	fTextAttributes(NULL),
	fTextLine(NULL),
	fTextFrame(NULL)	
	{;}
	
	~XMacTextFactory ()
	{
		if (fTextAttributes)
			::CFRelease( fTextAttributes);
		fTextAttributes = NULL;
		if (fTextLine)
			::CFRelease( fTextLine);
		fTextLine = NULL;
		if (fTextFrame)
			::CFRelease( fTextFrame);
		fTextFrame = NULL;
	}
	
	/** set text rendering mode */
	void			SetRenderingMode( TextRenderingMode inMode)
	{
		if (inMode != fRenderingMode)
		{
			fRenderingMode = inMode;
			SetDirty();
		}
	}
	
	/** set text layout mode */
	void			SetLayoutMode( TextLayoutMode inMode)
	{
		if (inMode != fLayoutMode)
		{
			fLayoutMode = inMode;
			SetDirty();
		}
	}
	
	TextRenderingMode		GetRenderingMode() const		{ return fRenderingMode;}
	TextLayoutMode			GetLayoutMode() const			{ return fLayoutMode;}
	
	
	/** get text line width */
	GReal			GetTextWidth (const VString& inText, bool inRTL = false);
	
	/** map mouse position to text line position */
	bool			MousePosToTextPos(const VString& inString, GReal inMousePos, sLONG& ioTextPos, GReal& ioPixPos) ;

	/** compute offsets of each char in the string */
	void			MeasureText( const VString& inString, std::vector<GReal>& outOffsets, bool inRTL = false);
	
	/** set dirty state for text attributes */ 
	void			SetDirty();
	
	
	/** create line from text, current attributes and current line layout 
	 @remarks
		CTLine object is internally retained 
		so if you call CreateLine first with some string, 
		then calling method again with the same string will return the same object
	 
		caller must release line
	 */
	CTLineRef		CreateLine( const VString& inText);
	
	/** create frame from text, current attributes and current frame layout 
	 @remarks
		CTFrame object is internally retained
		so if you call CreateFrame first with some string and bounds, 
		then calling method again with the same string and bounds will return the same object
	 
		caller must release frame
	 */
	CTFrameRef		CreateFrame( const VString& inText, const VRect& inBounds = VRect(0,0,0,0));

protected:	
	/** update text string attributes using font default attributes and text rendering+layout */ 
	void					_UpdateTextAttributes();
	
	/** return true if char is ideographic character 
		(CJK, katakana, hiragana, hangul, bopomofo or Yi unicode character) 
		or fullwidth latin character
	 
	 @remarks
		based on unicode 5.1 standard reference (UTF16 bytecodes only)
	 */
	bool					_IsUniCharIdeographicOrLatinFullWidth( UniChar inChar);
	
	/** build halfwidth latin character ranges and return retained attributes */
	CFDictionaryRef			_BuildHalfwidthLatinCharacterRanges();
		
	XMacFontMetrics*	fMetrics;
	
	TextRenderingMode	fRenderingMode;
	TextLayoutMode		fLayoutMode;
	
	/** current text string
	 
	 @remarks
		it is used with fTextLine and fTextFrame
		to optimize CoreText objects on a per-string basis
	 */
	VString			fTextString;
	
	/** current CTLine object */
	CTLineRef		fTextLine;
	
	/** current frame bounds */
	VRect			fTextFrameBounds;
	
	/** current CTFrame object */
	CTFrameRef		fTextFrame;
	
	/** Core text string attributes 
	 @remarks
		current text style attributes
	 */
	CFMutableDictionaryRef	fTextAttributes;
	
	/** halfwidth latin style range vector 
	 @remarks
		this is used to store halfwidth latin character ranges for vertical layout
	 */
	CTUserStyleRangeVector fLatinStyleRanges;
};


class XTOOLBOX_API XMacFontMetrics
{
public:
							XMacFontMetrics (VFontMetrics* inMetrics):fMetrics( inMetrics)	
							{
								fTextFactory = new XMacTextFactory( this);
							}
							~XMacFontMetrics ()
							{
								if (fTextFactory)
									delete fTextFactory;
								fTextFactory = NULL;
							}

			// Metrics support
			void			GetMetrics (GReal& outAscent, GReal& outDescent, GReal& outLeading) const;
			void			GetMetrics (GReal& outAscent, GReal& outDescent, GReal& outInternalLeading, GReal& outExternalLeading, GReal& outLineHeight) const;
	
			GReal			GetTextWidth (const VString& inText, bool inRTL = false)
			{
				return fTextFactory->GetTextWidth( inText, inRTL);
			}
			GReal			GetCharWidth (UniChar inChar);
	
			bool			MousePosToTextPos(const VString& inString, GReal inMousePos, sLONG& ioTextPos, GReal& ioPixPos)
			{
				return fTextFactory->MousePosToTextPos( inString, inMousePos, ioTextPos, ioPixPos);
			}

			void			MeasureText( const VString& inString, std::vector<GReal>& outOffsets, bool inRTL = false)
			{
				return fTextFactory->MeasureText( inString, outOffsets, inRTL);
			}
	
			/** get text factory */
			XMacTextFactory *GetTextFactory()
			{
				return fTextFactory;
			}
	
			VFontMetrics	*GetFontMetrics() const
			{
				return fMetrics;
			}

protected:
			/** parent metrics */
			VFontMetrics*	fMetrics;
	
			/** Core text factory */
			XMacTextFactory *fTextFactory;
};

typedef XMacFontMetrics XFontMetricsImpl;




END_TOOLBOX_NAMESPACE

#endif
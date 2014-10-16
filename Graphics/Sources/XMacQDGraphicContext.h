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
#ifndef __XMacQDGraphicContext__
#define __XMacQDGraphicContext__

#include "KernelIPC/VKernelIPC.h"
#include "Graphics/Sources/VGraphicContext.h"

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API VMacQDGraphicContext : public VGraphicContext
{
public:
			VMacQDGraphicContext (GrafPtr inContext);
			VMacQDGraphicContext (const VMacQDGraphicContext& inOriginal);
	virtual	~VMacQDGraphicContext ();
	
	// Pen management
	virtual void	SetFillColor (const VColor& inColor, VBrushFlags* ioFlags = NULL);
	virtual void	SetFillPattern (const VPattern* inPattern, VBrushFlags* ioFlags = NULL);
	virtual void	SetLineColor (const VColor& inColor, VBrushFlags* ioFlags = NULL);
	virtual void	SetLinePattern (const VPattern* inPattern, VBrushFlags* ioFlags = NULL);

	/** set line dash pattern
	@param inDashOffset
		offset into the dash pattern to start the line (user units)
	@param inDashPattern
		dash pattern (alternative paint and unpaint lengths in user units)
		for instance {3,2,4} will paint line on 3 user units, unpaint on 2 user units
							 and paint again on 4 user units and so on...
	*/
	virtual void	SetLineDashPattern(Real inDashOffset, const VLineDashPattern& inDashPattern, VBrushFlags* ioFlags = NULL) {}

	virtual void	SetLineWidth (Real inWidth, VBrushFlags* ioFlags = NULL);
	virtual void	SetLineStyle (CapStyle inCapStyle, JoinStyle inJoinStyle, VBrushFlags* ioFlags = NULL);
	virtual void	SetTextColor (const VColor& inColor, VBrushFlags* ioFlags = NULL);
	virtual void	GetTextColor (VColor& outColor);
	virtual void	SetBackColor (const VColor& inColor, VBrushFlags* ioFlags = NULL);
	virtual void	GetBrushPos (VPoint& outHwndPos) const { outHwndPos = fBrushPos; };
	virtual void	SetTransparency (Real inAlpha);	// Global Alpha - added to existing line or fill alpha
	virtual void	SetShadowValue (Real inHOffset, Real inVOffset, Real inBlurness);	// Need a call to SetDrawingMode to update changes

	virtual DrawingMode	SetDrawingMode (DrawingMode inMode, VBrushFlags* ioFlags = NULL);
	
	virtual void	EnableAntiAliasing ();
	virtual void	DisableAntiAliasing ();
	virtual bool	IsAntiAliasingAvailable ();
	
	// transform
	virtual void GetTransform(VAffineTransform &outTransform);
	virtual void SetTransform(const VAffineTransform &inTransform);
	virtual void ConcatTransform(const VAffineTransform &inTransform);
	virtual void RevertTransform(const VAffineTransform &inTransform);
	virtual void ResetTransform();
	
	// Text management
	virtual void	SetFont ( VFont *inFont, Real inScale = 1.0);
	virtual VFont*	GetFont();
	virtual void	SetTextPosBy (Real inHoriz, Real inVert);
	virtual void	SetTextPosTo (const VPoint& inHwndPos);
	virtual void	SetTextAngle (Real inAngle);
	virtual void	GetTextPos (VPoint& outHwndPos) const { outHwndPos = fTextPos; };
	
	virtual TextMeasuringMode	SetTextMeasuringMode(TextMeasuringMode inMode){return TMM_NORMAL;};
	virtual TextMeasuringMode	GetTextMeasuringMode(){return TMM_NORMAL;};


	virtual void	SetSpaceKerning (Real inSpaceExtra);
	virtual void	SetCharKerning (Real inSpaceExtra);
	
	virtual DrawingMode	SetTextDrawingMode (DrawingMode inMode, VBrushFlags* ioFlags = NULL);
	
	// Pixel management
	virtual uBYTE	SetAlphaBlend (uBYTE inAlphaBlend);
	virtual void	SetPixelBackColor (const VColor& inColor);
	virtual void	SetPixelForeColor (const VColor& inColor);
	
	// Graphic context storage
	virtual void	NormalizeContext ();
	virtual void	SaveContext ();
	virtual void	RestoreContext ();
	
	// Text measurement
	virtual Real	GetCharWidth (UniChar inChar) const;
	virtual Real	GetTextWidth (const VString& inString, bool inRTL = false) const;
	virtual Real	GetTextHeight (bool inIncludeExternalLeading = false) const;

	// Drawing primitives
	virtual void	DrawTextBox (const VString& inString, AlignStyle inHoriz, AlignStyle inVert, const VRect& inHwndBounds, TextLayoutMode inMode = TLM_NORMAL);
	virtual void	DrawText (const VString& inString, TextLayoutMode inMode = TLM_NORMAL, bool inRTLAnchorRight = false);
 
	//draw text at the specified position
	//
	//@param inString
	//	text string
	//@param inLayoutMode
	//  layout mode (here only TLM_VERTICAL and TLM_RIGHT_TO_LEFT are used)
	//
	//@remark: 
	//	this method does not make any layout formatting 
	//	text is drawed on a single line from the specified starting coordinate
	//	(which is relative to the first glyph horizontal or vertical baseline)
	//	useful when you want to position exactly a text at a specified coordinate
	//	without taking account extra spacing due to layout formatting (which is implementation variable)
	//@note
	//	this method is used by SVG component
	virtual void	DrawText (const VString& inString, const VPoint& inPos, TextLayoutMode inLayoutMode = TLM_NORMAL, bool inRTLAnchorRight = false);
	
	virtual void	DrawRect (const VRect& inHwndRect);
	virtual void	DrawOval (const VRect& inHwndRect);
	virtual void	DrawPolygon (const VPolygon& inHwndPolygon);
	virtual void	DrawRegion (const VRegion& inHwndRegion);
	virtual void	DrawBezier (VGraphicPath& inHwndBezier);
	virtual void	DrawPath (VGraphicPath& inHwndPath);

	virtual void	FillRect (const VRect& inHwndRect);
	virtual void	FillOval (const VRect& inHwndRect);
	virtual void	FillPolygon (const VPolygon& inHwndPolygon);
	virtual void	FillRegion (const VRegion& inHwndRegion);
	virtual void	FillBezier (VGraphicPath& inHwndBezier);
	virtual void	FillPath (VGraphicPath& inHwndPath);

	virtual void	FrameRect (const VRect& inHwndRect);
	virtual void	FrameOval (const VRect& inHwndRect);
	virtual void	FramePolygon (const VPolygon& inHwndPolygon);
	virtual void	FrameRegion (const VRegion& inHwndRegion);
	virtual void	FrameBezier (const VGraphicPath& inHwndBezier);
	virtual void	FramePath (const VGraphicPath& inHwndPath);

	virtual void	EraseRect (const VRect& inHwndRect);
	virtual void	EraseRegion (const VRegion& inHwndRegion);

	virtual void	InvertRect (const VRect& inHwndRect);
	virtual void	InvertRegion (const VRegion& inHwndRegion);

	virtual void	DrawLine (const VPoint& inHwndStart, const VPoint& inHwndEnd);
	virtual void	DrawLines (const Real* inCoords, sLONG inCoordCount);
	virtual void	DrawLineTo (const VPoint& inHwndEnd);
	virtual void	DrawLineBy (Real inWidth, Real inHeight);
	
	virtual void	MoveBrushTo (const VPoint& inHwndPos);
	virtual void	MoveBrushBy (Real inWidth, Real inHeight);
	
	virtual void	DrawPicture (const class VPicture& inPicture,const VRect& inHwndBounds,class VPictureDrawSettings *inSet=0);
	//virtual void	DrawPicture (const VOldPicture& inPicture, PictureMosaic inMosaicMode, const VRect& inHwndBounds, TransferMode inDrawingMode = TM_COPY);
	virtual void	DrawFileBitmap(const VFileBitmap *inFileBitmap, const VRect& inHwndBounds);
	virtual void	DrawPictureFile (const VFile& inFile, const VRect& inHwndBounds);
	virtual void	DrawIcon (const VIcon& inIcon, const VRect& inHwndBounds, PaintStatus inState = PS_NORMAL, Real inScale = 1.0); // JM 050507 ACI0049850 rajout inScale

	// Clipping
	virtual void	ClipRect (const VRect& inHwndRect, Boolean inAddToPrevious = false, Boolean inIntersectToPrevious = true);
	virtual void	ClipRegion (const VRegion& inHwndRgn, Boolean inAddToPrevious = false, Boolean inIntersectToPrevious = true);
	virtual void	GetClip (VRegion& outHwndRgn) const;
	
	virtual void	SaveClip ();
	virtual void	RestoreClip ();

	// Content bliting primitives
	virtual void	CopyContentTo (const VGraphicContext* inDestContext, const VRect* inSrcBounds = NULL, const VRect* inDestBounds = NULL, TransferMode inMode = TM_COPY, const VRegion* inMask = NULL);
	virtual void	Flush ();
	
	// Pixel drawing primitives
	virtual void	FillUniformColor (const VPoint& inContextPos, const VColor& inColor, const VPattern* inPattern = NULL);
	virtual void	SetPixelColor (const VPoint& inContextPos, const VColor& inColor);
	virtual VColor*	GetPixelColor (const VPoint& inContextPos) const;
	//virtual VOldPicture*	GetPicture () const { return NULL; };

	// Port setting and restoration (use before and after any drawing suite)
	virtual	void	BeginUsingContext (bool inNoDraw = false);
	virtual	void	EndUsingContext ();

	virtual	PortRef		GetParentPort () const { return fContext; };
	virtual	ContextRef	GetParentContext () const { return NULL; };
	
	// Utilities
	virtual	Boolean	UseEuclideanAxis () { return false; };
	virtual	Boolean	UseReversedAxis () { return true; };
	
	static void	_AdjustMLTEMinimalBounds (Rect& ioBounds);
	static void	_ApplyPattern (ContextRef inContext, PatternStyle inStyle, const VColor& inColor, PatternRef inPatternRef, FillRuleType inFillRule = FILLRULE_NONZERO);
	static Boolean	_GetStdPattern (PatternStyle inStyle, void* ioPattern);
	
	// Debug Utilities
	static void	RevealUpdate (WindowRef inWindow);
	static void	RevealClipping (PortRef inPort);
	static void	RevealBlitting (PortRef inPort, const RgnRef inRegion);
	static void	RevealInval (PortRef inPort, const RgnRef inRegion);

	// Class initialization
	static Boolean	Init ();
	static void 	DeInit ();
	
protected:
	GrafPtr			fContext;
	PortRef			fSavedPort;				// Context store/restore
	sLONG			fPortRefcon;
	VHandleStream*	fSavedContext;
	VArrayOf<RgnHandle>* fClipStack;
	RGBColor		fPixelForeColor;		// Some context settings
	RGBColor		fPixelBackColor;
	VPoint			fBrushPos;
	VPoint			fTextPos;
	Real			fTextAngle;
	uBYTE			fAlphaBlend;

private:
	// Initialization
	void	_Init ();
	void	_Dispose ();
};


class XTOOLBOX_API VMacQDOffscreenContext : public VMacQDGraphicContext
{
public:
			VMacQDOffscreenContext (GrafPtr inSourceContext);
			VMacQDOffscreenContext (const VMacQDOffscreenContext& inOriginal);
	virtual	~VMacQDOffscreenContext ();
	
	// Offscreen management
	virtual Boolean	CreatePicture (const VRect& inHwndBounds);
	virtual void	ReleasePicture ();
	
	// Inherited from VGraphicContext
	virtual void	CopyContentTo (const VGraphicContext* inDestContext, const VRect* inSrcBounds = NULL, const VRect* inDestBounds = NULL, TransferMode inMode = TM_COPY, const VRegion* inMask = NULL);

	// Accessors
	PicHandle	MAC_GetPicHandle () const;

protected:
	PicHandle	fPicture;
	Boolean		fPictureClosed;

private:
	// Initialization
	void	_Init ();
	void	_Dispose ();
};


class XTOOLBOX_API VMacQDBitmapContext : public VMacQDGraphicContext
{
public:
			VMacQDBitmapContext (GrafPtr inSourceContext);
			VMacQDBitmapContext (const VMacQDBitmapContext& inOriginal);
	virtual	~VMacQDBitmapContext ();
	
	// Offscreen management
	virtual Boolean	CreateBitmap (const VRect& inHwndBounds);
	virtual void	ReleaseBitmap ();
	
	// Accessors
	PicHandle	MAC_GetPicHandle () const;

protected:
	GrafPtr	fSrcContext;

private:
	// Initialization
	void	_Init ();
	void	_Dispose ();
};
	

// This class automate GetPort/SetPort calls
// Do not instantiate with new operator. You allways allocate in stack.
//
class XTOOLBOX_API StSetPort
{
public:
			StSetPort (WindowRef inWindow);
			StSetPort (PortRef inPort);
	virtual	~StSetPort ();

	operator PortRef () { return fCurrentPort; };

protected:
	PortRef		fCurrentPort;
	PortRef		fSavedPort;
	GDHandle	fSavedGD;
	
#if VERSIONDEBUG	// Don't allocate in heap !
	StSetPort () {};
#endif
};


// This class automate GetClipping/SetClipping calls
// Do not instantiate with new operator. You allways allocate in stack.
//
class XTOOLBOX_API StSetPortClipping
{
public:
			StSetPortClipping (WindowRef inWindow, RgnRef inNewClip);
			StSetPortClipping (WindowRef inWindow, RgnHandle inNewClip);
			StSetPortClipping (PortRef inPort, RgnRef inNewClip);
			StSetPortClipping (PortRef inPort, RgnHandle inNewClip);
	virtual	~StSetPortClipping ();

	operator RgnHandle () { return fSavedClip; };

protected:
	PortRef	fPort;
	RgnHandle	fSavedClip;
	
#if VERSIONDEBUG	// Don't allocate in heap !
	StSetPortClipping () {};
#endif
};

END_TOOLBOX_NAMESPACE

#endif

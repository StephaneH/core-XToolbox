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
#ifndef __XWinGDIPlusGraphicContext__
#define __XWinGDIPlusGraphicContext__

#include "Graphics/Sources/VGraphicContext.h"
#include <stack>
#include "VRect.h"

BEGIN_TOOLBOX_NAMESPACE

// Needed declarations
class VAffineTransform;
class VPattern;
class VAxialGradientPattern;
class VRadialGradientPattern;
class VGraphicFilterProcess;

#define GDIPLUS_RECT(x) Gdiplus::RectF(x.GetLeft(), x.GetTop(), x.GetWidth(), x.GetHeight())
#define GDIPLUS_POINT(x) Gdiplus::PointF(x.GetX(), x.GetY())

class XTOOLBOX_API VWinGDIPlusGraphicContext : public VGraphicContext
{

public:
				
				VWinGDIPlusGraphicContext (Gdiplus::Graphics* inGraphics,uBOOL inOwnGraphics=false);
				VWinGDIPlusGraphicContext (HDC inContext);
				VWinGDIPlusGraphicContext (HWND inWindow);

				/** create a bitmap graphic context from the specified bitmap
				@param inOwnIt
					true: graphic context owns the bitmap
					false: caller owns the bitmap

				@remarks
					if caller is bitmap owner, do not destroy bitmap before graphic context 

					use method GetGdiplusBitmap() to get bitmap ref
				*/
				VWinGDIPlusGraphicContext( Gdiplus::Bitmap *inBitmap, bool inOwnIt = true);

				/** create a bitmap graphic context from scratch
				@remarks
					use method GetGdiplusBitmap() to get bitmap ref
				*/
				VWinGDIPlusGraphicContext( sLONG inWidth, sLONG inHeight);
				VWinGDIPlusGraphicContext( sLONG inWidth, sLONG inHeight, Gdiplus::PixelFormat inPixelFormat);

				/** retain WIC bitmap  
				@remarks
					return NULL if graphic context is not a bitmap context or if context is GDI

					bitmap is still shared by the context so you should not modify it (!)
				*/
				IWICBitmap *RetainWICBitmap() const;

				/** get Gdiplus bitmap  
				@remarks
					return NULL if graphic context is not a bitmap context or if context is GDI

					bitmap is still owned by the context so you should not destroy it (!)
				*/
				Gdiplus::Bitmap *GetGdiplusBitmap() const { return fBitmap; }


	explicit	VWinGDIPlusGraphicContext (const VWinGDIPlusGraphicContext& inOriginal);
	virtual		~VWinGDIPlusGraphicContext ();
	
	virtual		GReal GetDpiX() const;
	virtual		GReal GetDpiY() const;

	virtual bool IsGdiPlusImpl() const { return true; }

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
	virtual void	SetLineDashPattern(GReal inDashOffset, const VLineDashPattern& inDashPattern, VBrushFlags* ioFlags = NULL);

	/** set fill rule */
	virtual void	SetFillRule( FillRuleType inFillRule);

	virtual void	SetLineWidth (GReal inWidth, VBrushFlags* ioFlags = NULL);
	virtual GReal	GetLineWidth () const;

	virtual void	SetLineStyle (CapStyle inCapStyle, JoinStyle inJoinStyle, VBrushFlags* ioFlags = NULL);
	virtual void	SetLineCap (CapStyle inCapStyle, VBrushFlags* ioFlags = NULL);
	virtual void	SetLineJoin(JoinStyle inJoinStyle, VBrushFlags* ioFlags = NULL); 
	/** set line miter limit */
	virtual void	SetLineMiterLimit( GReal inMiterLimit = (GReal) 0.0);
	virtual void	SetTextColor (const VColor& inColor, VBrushFlags* ioFlags = NULL);
	virtual void	GetTextColor (VColor& outColor) const;
	virtual void	SetBackColor (const VColor& inColor, VBrushFlags* ioFlags = NULL);
	virtual void	GetBrushPos (VPoint& outHwndPos) const;
	virtual GReal	SetTransparency (GReal inAlpha);	// Global Alpha - added to existing line or fill alpha
	virtual void	SetShadowValue (GReal inHOffset, GReal inVOffset, GReal inBlurness);	// Need a call to SetDrawingMode to update changes

	virtual DrawingMode	SetDrawingMode (DrawingMode inMode, VBrushFlags* ioFlags = NULL);
	
	virtual void	EnableAntiAliasing ();
	virtual void	DisableAntiAliasing ();
	virtual bool	IsAntiAliasingAvailable ();
	virtual	TextRenderingMode	SetTextRenderingMode( TextRenderingMode inMode);
	virtual TextRenderingMode	GetTextRenderingMode() const;

	// Text management
	virtual void	SetFont ( VFont *inFont, GReal inScale = (GReal) 1.0);
	virtual VFont*	RetainFont ();
	virtual void	SetTextPosBy (GReal inHoriz, GReal inVert);
	virtual void	SetTextPosTo (const VPoint& inHwndPos);
	virtual void	SetTextAngle (GReal inAngle);
	virtual void	GetTextPos (VPoint& outHwndPos) const;

	virtual void	SetSpaceKerning (GReal inRatio);
	virtual void	SetCharKerning (GReal inRatio);
	
	virtual DrawingMode	SetTextDrawingMode (DrawingMode inMode, VBrushFlags* ioFlags = NULL);
	
	/** return true if font glyphs are aligned to pixel boundary */
	virtual	bool IsFontPixelSnappingEnabled() const { return (GetTextRenderingMode() & TRM_LEGACY_ON) != 0; }

	virtual void GetTransform(VAffineTransform &outTransform);
	virtual void SetTransform(const VAffineTransform &inTransform);
	virtual void ConcatTransform(const VAffineTransform &inTransform);
	virtual void RevertTransform(const VAffineTransform &inTransform);
	virtual void ResetTransform();
	
	// Pixel management
	virtual uBYTE	SetAlphaBlend (uBYTE inAlphaBlend);
	virtual void	SetPixelBackColor (const VColor& inColor);
	virtual void	SetPixelForeColor (const VColor& inColor);
	
	// Graphic context storage
	virtual void	NormalizeContext ();
	virtual void	SaveContext ();
	virtual void	RestoreContext ();
		
	// Create a shadow layer graphic context
	// with specified bounds and transparency
	void BeginLayerShadow(const VRect& inBounds, const VPoint& inShadowOffset = VPoint(8,8), GReal inShadowBlurDeviation = (GReal) 1.0, const VColor& inShadowColor = VColor(0,0,0,255), GReal inAlpha = (GReal) 1.0);
	
	// Graphic layer management
	
	// Create a filter or alpha layer graphic context
	// with specified bounds and filter
	// @remark : retain the specified filter until EndLayer is called
protected:
			void				_Construct( sLONG inWidth, sLONG inHeight, Gdiplus::PixelFormat inPixelFormat);
	virtual bool				_BeginLayer(	 const VRect& inBounds, 
											 GReal inAlpha, 
											 VGraphicFilterProcess *inFilterProcess,
											 VImageOffScreenRef inBackBufferRef = NULL,
											 bool inInheritParentClipping = true,
											 bool inTransparent = true);
	virtual VImageOffScreenRef _EndLayer();
public:
	/** return true if offscreen layer needs to be cleared or resized on next call to DrawLayerOffScreen or BeginLayerOffScreen/EndLayerOffScreen
	@remarks
		return true if transformed input bounds size is not equal to the background layer size
	    so only transformed bounds translation is allowed.
		return true the offscreen layer is not compatible with the current gc
	 */
	virtual bool	ShouldClearLayerOffScreen(const VRect& inBounds, VImageOffScreenRef inBackBuffer);

	/** draw offscreen layer using the specified bounds 
	@remarks
		this is like calling BeginLayerOffScreen & EndLayerOffScreen without any drawing between
		so it is faster because only the offscreen bitmap is rendered
	 
		if transformed bounds size is not equal to background layer size, return false and do nothing 
	    so only transformed bounds translation is allowed.
		if inBackBuffer is NULL, return false and do nothing

		ideally if offscreen content is not dirty, you should first call this method
		and only call BeginLayerOffScreen & EndLayerOffScreen if it returns false
	 */
	virtual bool	DrawLayerOffScreen(const VRect& inBounds, VImageOffScreenRef inBackBuffer, bool inDrawAlways = false);
	
	// clear all layers and restore main graphic context
	virtual void	ClearLayers();
	
	/** return current clipping bounding rectangle 
	 @remarks
	 bounding rectangle is expressed in VGraphicContext normalized coordinate space 
	 (ie coordinate space with y axis pointing down - compliant with gdiplus and quickdraw coordinate spaces)
	 */  
	virtual void	GetClipBoundingBox( VRect& outBounds);
	
	virtual void	GetTransformToLayer(VAffineTransform &outTransform, sLONG inIndexLayer);

	//return current graphic context native reference
	virtual VGCNativeRef GetNativeRef() const
	{
		return (VGCNativeRef)fGDIPlusGraphics;
	}
	
	// Text measurement
	virtual GReal	GetCharWidth (UniChar inChar) const ;
	virtual GReal	GetTextWidth (const VString& inString, bool inRTL = false) const;
	virtual GReal	GetTextHeight (bool inIncludeExternalLeading = false) const;
	virtual sLONG	GetNormalizedTextHeight(bool inIncludeExternalLeading=false) const;
	virtual void	GetTextBounds( const VString& inString, VRect& oRect) const;

	// Drawing primitives
	virtual void	DrawTextBox (const VString& inString, AlignStyle inHoriz, AlignStyle inVert, const VRect& inHwndBounds, TextLayoutMode inMode = TLM_NORMAL);
	
	//return text box bounds
	//@remark:
	// if input bounds width is not null, use it as max line wrapping width (or max text height in vertical layout mode)
	// if input bounds height is not null, use it as max text height (or max line wrapping width in vertical layout mode)
	virtual void	GetTextBoxBounds( const VString& inString, VRect& ioHwndBounds, TextLayoutMode inMode = TLM_NORMAL);
	
	virtual void	DrawText (const VString& inString, TextLayoutMode inMode = TLM_NORMAL, bool inRTLAnchorRight = false);

	virtual TextMeasuringMode	SetTextMeasuringMode(TextMeasuringMode inMode);
	virtual TextMeasuringMode	GetTextMeasuringMode(){return fTextMeasuringMode;};

	//get text bounds 
	//
	//@param inString
	//	text string
	//@param oRect
	//	text bounds (out param)
	//@param inLayoutMode
	//  layout mode
	//
	//@remark
	//	this method return the typographic text line bounds 
	//@note
	//	this method is used by SVG component
	virtual void	GetTextBoundsTypographic( const VString& inString, VRect& oRect, TextLayoutMode inLayoutMode = TLM_NORMAL) const;
	

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
	virtual void	DrawLine(const GReal inHwndStartX,const GReal inHwndStartY, const GReal inHwndEndX,const GReal inHwndEndY);
	virtual void	DrawLines (const GReal* inCoords, sLONG inCoordCount);
	virtual void	DrawLineTo (const VPoint& inHwndEnd);
	virtual void	DrawLineBy (GReal inWidth, GReal inHeight);
	
	virtual void	MoveBrushTo (const VPoint& inHwndPos);
	virtual void	MoveBrushBy (GReal inWidth, GReal inHeight);
	
	virtual void	DrawPicture (const class VPicture& inPicture,const VRect& inHwndBounds,class VPictureDrawSettings *inSet=0);
	//virtual void	DrawPicture (const VOldPicture& inPicture, PictureMosaic inMosaicMode, const VRect& inHwndBounds, TransferMode inDrawingMode = TM_COPY);
	virtual void	DrawFileBitmap(const VFileBitmap *inFileBitmap, const VRect& inHwndBounds);
	virtual void	DrawPictureFile (const VFile& inFile, const VRect& inHwndBounds);
	virtual void	DrawIcon (const VIcon& inIcon, const VRect& inHwndBounds, PaintStatus inState = PS_NORMAL, GReal inScale = (GReal) 1.0); // JM 050507 ACI0049850 rajout inScale

	virtual void	QDFrameRect (const VRect& inHwndRect);
	virtual void	QDFrameRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight);
	virtual void	QDFrameOval (const VRect& inHwndRect);

	virtual void	QDDrawRect (const VRect& inHwndRect);
	virtual void	QDDrawRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight);
	virtual void	QDDrawOval (const VRect& inHwndRect);

	virtual void	QDFillRect (const VRect& inHwndRect);
	virtual void	QDFillRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight);
	virtual void	QDFillOval (const VRect& inHwndRect);

	virtual void	QDDrawLine(const GReal inHwndStartX,const GReal inHwndStartY, const GReal inHwndEndX,const GReal inHwndEndY);
	virtual void	QDMoveBrushTo (const VPoint& inHwndPos);
	virtual void	QDDrawLine (const VPoint& inHwndStart, const VPoint& inHwndEnd);
	virtual void	QDDrawLineTo (const VPoint& inHwndEnd);
	virtual void	QDDrawLines (const GReal* inCoords, sLONG inCoordCount);
	//draw picture data
	virtual void    DrawPictureData (const VPictureData *inPictureData, const VRect& inHwndBounds,VPictureDrawSettings *inSet=0);
	
	/** clear current graphic context area
	@remarks
		GDIPlus impl: inBounds can be NULL if and only if graphic context is a bitmap graphic context
	*/
	virtual void	Clear( const VColor& inColor = VColor(0,0,0,0), const VRect *inBounds = NULL, bool inBlendCopy = true);

	// Clipping
	virtual void	ClipRect (const VRect& inHwndRect, Boolean inAddToPrevious = false, Boolean inIntersectToPrevious = true);
	
	//add or intersect the specified clipping path with the current clipping path
	virtual void	ClipPath (const VGraphicPath& inPath, Boolean inAddToPrevious = false, Boolean inIntersectToPrevious = true);

	/** add or intersect the specified clipping path outline with the current clipping path 
	@remarks
		this will effectively create a clipping path from the path outline 
		(using current stroke brush) and merge it with current clipping path,
		making possible to constraint drawing to the path outline
	*/
	virtual void	ClipPathOutline (const VGraphicPath& inPath, Boolean inAddToPrevious = false, Boolean inIntersectToPrevious = true);

	virtual void	ClipRegion (const VRegion& inHwndRgn, Boolean inAddToPrevious = false, Boolean inIntersectToPrevious = true);
	virtual void	GetClip (VRegion& outHwndRgn) const;
	
	virtual void	SaveClip ();
	virtual void	RestoreClip ();

	// Content bliting primitives
	virtual void	CopyContentTo ( VGraphicContext* inDestContext, const VRect* inSrcBounds = NULL, const VRect* inDestBounds = NULL, TransferMode inMode = TM_COPY, const VRegion* inMask = NULL);
	virtual void	Flush ();
	virtual VPictureData* CopyContentToPictureData();
	
	// Pixel drawing primitives
	virtual void	FillUniformColor (const VPoint& inContextPos, const VColor& inColor, const VPattern* inPattern = NULL);
	virtual void	SetPixelColor (const VPoint& inContextPos, const VColor& inColor);
	virtual VColor*	GetPixelColor (const VPoint& inContextPos) const;
	//virtual VOldPicture*	GetPicture () const { return NULL; };

	// Port setting and restoration (use before and after any drawing suite)
	virtual	void	BeginUsingContext (bool inNoDraw = false);
	virtual	void	EndUsingContext ();

	// pp same as GetParentPort/ReleaseParentPort but special case for gdi+/gdi 
	// hdc clipregion is not sync with gdi+ clip region, to sync clip use this function
	virtual	ContextRef	BeginUsingParentContext()const;
	virtual	void	EndUsingParentContext(ContextRef inContextRef)const;

	virtual	PortRef	BeginUsingParentPort()const{return BeginUsingParentContext();};
	virtual	void	EndUsingParentPort(PortRef inPortRef)const{EndUsingParentContext(inPortRef);};


	virtual	PortRef		GetParentPort () const { return _GetHDC(); };
	virtual	ContextRef	GetParentContext () const { return _GetHDC(); };
	
	virtual	void	ReleaseParentPort (PortRef inPortRef) const {return _ReleaseHDC(inPortRef);};
	virtual	void	ReleaseParentContext (ContextRef inContextRef) const {return _ReleaseHDC(inContextRef);};
	
	// Utilities
	virtual	Boolean	UseEuclideanAxis () { return false; };
	virtual	Boolean	UseReversedAxis () { return true; };
	
	void	SelectBrushDirect (HGDIOBJ inBrush);
	void	SelectPenDirect (HGDIOBJ inBrush);
	void	SetTextCursorInfo (VString& inFont, sLONG inSize, Boolean inBold, Boolean inItalic, sLONG inX, sLONG inY);

	// Debug Utils
	static void	RevealUpdate (HWND inHwnd);
	static void	RevealClipping (ContextRef inContext);
	static void	RevealClipping (Gdiplus::Graphics* inContext);
	static void	RevealBlitting (ContextRef inContext, const RgnRef inHwndRegion);
	static void	RevealInval (ContextRef inContext, const RgnRef inHwndRegion);
	
	// Class initialization
	static Boolean	Init ();
	static void 	DeInit ();

	HDC _GetHDC()const;
	void _ReleaseHDC(HDC inDC)const;

protected:
	
	static ULONG_PTR	sGDIpTok;
	
	VIndex		fUseCount;
	HWND		fHwnd;
	HDC			fRefContext;	
	uBOOL		fOwnGraphics;

	std::stack <Gdiplus::GraphicsState> fSaveState;

	
	Gdiplus::Graphics		*fGDIPlusGraphics;
	Gdiplus::Graphics		*fGDIPlusDirectGraphics;

	/** bitmap source for bitmap graphic context or NULL if not a bitmap graphic context */
	Gdiplus::Bitmap			*fBitmap;	
	mutable IWICBitmap		*fWICBitmap; //WIC backing store used to cache WIC bitmap by RetainWICBitmap
	bool					fOwnBitmap;

	Gdiplus::Color			*fLineColor;
	Gdiplus::Pen			*fStrokePen;
	Gdiplus::Brush			*fLineBrush;
	Gdiplus::Brush			*fFillBrush;
	Gdiplus::Brush			*fTextBrush;
	VFont					*fTextFont;
	GReal					 fTextScale;
	VAffineTransform		*fTextScaleTransformBackup;
	mutable GReal					 fWhiteSpaceWidth;
	Gdiplus::RectF			*fTextRect;
	VPoint					fBrushPos;
	Gdiplus::SolidBrush				*fFillBrushSolid;

	Gdiplus::LinearGradientBrush	*fLineBrushLinearGradient;
	Gdiplus::PathGradientBrush		*fLineBrushRadialGradient;

	Gdiplus::LinearGradientBrush	*fFillBrushLinearGradient;
	Gdiplus::PathGradientBrush		*fFillBrushRadialGradient;

	/** brush used to paint area outside radial gradient path
	@remark 
		as gdiplus does not draw outside gradient path
		we need to paint first with solid color equal to last stop color
	*/
	Gdiplus::SolidBrush				*fLineBrushGradientClamp;
	Gdiplus::SolidBrush				*fFillBrushGradientClamp;
	
	/** gdiplus gradient path */
	Gdiplus::GraphicsPath			*fFillBrushGradientPath;

	VArrayOf<Gdiplus::Region*>	fClipStack;
	Gdiplus::Bitmap		*fMemBitmap;

	DrawingMode				fDrawingMode;
	TextRenderingMode		fTextRenderingMode;
	TextMeasuringMode		fTextMeasuringMode;
	
	Gdiplus::FillMode		fFillMode;

	GReal fGlobalAlpha;
	GReal fShadowHOffset;
	GReal fShadowVOffset;
	GReal fShadowBlurness;

private:
	// Initialization
	void	_Init ();
	void	_Dispose ();
	void	_InitGDIPlusObjects(bool inBeginUsingContext = true);
	void	_DisposeGDIPlusObjects();
	
	std::map<HICON, Gdiplus::Bitmap*> fCachedIcons;

	//create GDI+ linear gradient brush from the specified input pattern
	void _CreateLinearGradientBrush( const VPattern* inPattern, Gdiplus::Brush *&ioBrush, Gdiplus::LinearGradientBrush *&ioBrushLinear, Gdiplus::PathGradientBrush *&ioBrushRadial);

	//create GDI+ path gradient brush from the specified input pattern
	void _CreateRadialGradientBrush( const VPattern* inPattern, Gdiplus::Brush *&ioBrush, Gdiplus::LinearGradientBrush *&ioBrushLinear, Gdiplus::PathGradientBrush *&ioBrushRadial, Gdiplus::SolidBrush *&ioBrushClamp);

	/** apply text constant scaling */ 
	void	_ApplyTextScale( const VPoint& inTextOrig);
	/** revert text constant scaling */
	void	_RevertTextScale();

	/** determine current white space width 
	@remarks
		used for custom kerning
	*/
	void	_ComputeWhiteSpaceWidth() const;

	Gdiplus::Brush *_GetTextBrush() const
	{
		if (fUseShapeBrushForText)
		{
			if (fFillBrush)
				return fFillBrush;
			else
				return fTextBrush;
		}
		else
			return fTextBrush;
	}

	/** build native polygon */
	Gdiplus::PointF *_BuildPolygonNative( const VPolygon& inPolygon, Gdiplus::PointF *inBuffer = NULL, bool inFillOnly = false);

	// begin info for BeginUsingParentPort
	mutable sLONG				fHDCOpenCount;
	mutable HRGN				fHDCSavedClip;
	mutable	HFONT				fHDCFont;
	mutable	HFONT				fHDCSaveFont;
	mutable Gdiplus::Region*	fSavedClip;
	mutable HDC_TransformSetter* fHDCTransform;
	// end info for BeginUsingParentPort
};


class XTOOLBOX_API VWinGDIPlusOffscreenContext : public VWinGDIPlusGraphicContext
{
public:
			VWinGDIPlusOffscreenContext (HDC inSourceContext);
			VWinGDIPlusOffscreenContext (const VWinGDIPlusGraphicContext& inSourceContext);
			VWinGDIPlusOffscreenContext (const VWinGDIPlusOffscreenContext& inOriginal);
	virtual	~VWinGDIPlusOffscreenContext ();
	
	// Offscreen management
	virtual Boolean	CreatePicture (const VRect& inHwndBounds);
	virtual void	ReleasePicture ();
	
	// Inherited from VGraphicContext
	virtual void	CopyContentTo ( VGraphicContext* inDestContext, const VRect* inSrcBounds = NULL, const VRect* inDestBounds = NULL, TransferMode inMode = TM_COPY, const VRegion* inMask = NULL);

	// Accessors

protected:

private:
	// Initialization
	void	_Init ();
	void	_Dispose ();
};

class XTOOLBOX_API VWinGDIPlusBitmapContext : public VWinGDIPlusGraphicContext
{
public:
			VWinGDIPlusBitmapContext (HDC inSourceContext);
			VWinGDIPlusBitmapContext (const VWinGDIPlusGraphicContext& inSourceContext);
			VWinGDIPlusBitmapContext (const VWinGDIPlusBitmapContext& inOriginal);
	virtual	~VWinGDIPlusBitmapContext ();
	
	// Offscreen management
	virtual Boolean	CreateBitmap (const VRect& inHwndBounds);
	virtual void	ReleaseBitmap ();
		
	// Inherited from VGraphicContext
	virtual void	CopyContentTo ( VGraphicContext* inDestContext, const VRect* inSrcBounds = NULL, const VRect* inDestBounds = NULL, TransferMode inMode = TM_COPY, const VRegion* inMask = NULL);
	virtual VPictureData*	CopyContentToPictureData();
protected:
	HDC						fSrcContext;
	bool					fReleaseSrcContext;
	class VWinGDIBitmapContext	*fOffscreen;
private:
	// Initialization
	void	_Init ();
	void	_Dispose ();
};

#if 0
class XTOOLBOX_API VWinGDIPlusBitmapContext : public VWinGDIPlusGraphicContext
{
public:
			VWinGDIPlusBitmapContext (HDC inSourceContext);
			VWinGDIPlusBitmapContext (const VWinGDIPlusGraphicContext& inSourceContext);
			VWinGDIPlusBitmapContext (const VWinGDIPlusBitmapContext& inOriginal);
	virtual	~VWinGDIPlusBitmapContext ();
	
	// Offscreen management
	virtual Boolean	CreateBitmap (const VRect& inHwndBounds);
	virtual void	ReleaseBitmap ();
		
	// Accessors

protected:
	HDC		fSrcContext;
	Gdiplus::Bitmap*	fBitmap;
	HBITMAP	fSavedBitmap;

private:
	// Initialization
	void	_Init ();
	void	_Dispose ();
};
#endif


class XTOOLBOX_API StUseHDC : public VObject
{
	// petite class pour obtenir un HDC a partir d'un context GDIPlus, et sync les clip region (pas fait par l'api system)
	public:
	StUseHDC(Gdiplus::Graphics* inGraphics);
	virtual ~StUseHDC();
	
	operator HDC() { return fHDC ;};

	private:
	Gdiplus::Graphics*	fGDIPlusGraphics;
	HDC					fHDC;
	HRGN				fHDCSavedClip;
	Gdiplus::Region*	fSavedClip;
};

END_TOOLBOX_NAMESPACE

#endif

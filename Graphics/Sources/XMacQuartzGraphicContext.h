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
#ifndef __XMacQuartzGraphicContext__
#define __XMacQuartzGraphicContext__

#include "Graphics/Sources/VGraphicContext.h"

BEGIN_TOOLBOX_NAMESPACE

// Needed declarations
class VAxialGradientPattern;
class VRadialGradientPattern;
class VBitmapData;

// abstract class used to remove all ref to QD
// implementation is in GUI
class XTOOLBOX_API VMacQDPortBridge : public VObject
{
	friend class VMacQuartzGraphicContext;
public:
	VMacQDPortBridge(){};
	virtual ~VMacQDPortBridge(){};
protected:
	virtual void    BeginCGContext(CGContextRef&  outContext,CGRect& outQDPortBounds)=0;
	virtual void    EndCGContext()=0;
	virtual void    SetPort()=0;
	virtual PortRef GetPort()=0;
	virtual void    Flush()=0;
	virtual CGRect  GetPortBounds()=0;
    virtual void    ClipQDPort(const XBOX::VRegion& inClip)=0;
};


class XTOOLBOX_API VMacQuartzGraphicContext : public VGraphicContext
{
public:
			VMacQuartzGraphicContext();
			VMacQuartzGraphicContext (CGContextRef inContext, Boolean inReversed, const CGRect& inQDBounds,bool inForPrinting=false);

			VMacQuartzGraphicContext (CGContextRef inPrinterContext, const PMPageFormat inPageFormat); // for printing only

			VMacQuartzGraphicContext (VMacQDPortBridge* inQDPort, const CGRect& inQDBounds,bool inForPrinting=false);
			
			VMacQuartzGraphicContext (const VMacQuartzGraphicContext& inOriginal);
	virtual	~VMacQuartzGraphicContext ();

	virtual	bool	IsQuartz2DImpl() const { return true; }

	// legacy printing support
	virtual	bool BeginQDContext();
	virtual	void EndQDContext(bool inDontRestoreClip = false);
			void UseThisQDPortBridge(VMacQDPortBridge* inNewBridge,const CGRect& inQDBounds,bool inForPrinting=false);
	
	// cocoa printing support
	// Attach a new CGContext or NULL to release the current CGContext
	// To call after Opening or closing printer page
			void UpdatePrintingContext(CGContextRef inNewCGContext);
	
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

			/** force dash pattern unit to be equal to stroke width (on default it is equal to user unit) */
	virtual void	ShouldLineDashPatternUnitEqualToLineWidth( bool inValue);
	virtual bool	ShouldLineDashPatternUnitEqualToLineWidth() const { return fIsStrokeDashPatternUnitEqualToStrokeWidth; }

	/** set fill rule */
	virtual void	SetFillRule( FillRuleType inFillRule);

	virtual void	SetLineWidth (GReal inWidth, VBrushFlags* ioFlags = NULL);
	virtual GReal	GetLineWidth () const { return fLineWidth; }
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
	
	// transform
	virtual void GetTransform(VAffineTransform &outTransform);
	virtual void SetTransform(const VAffineTransform &inTransform);
	virtual void ConcatTransform(const VAffineTransform &inTransform);
	virtual void RevertTransform(const VAffineTransform &inTransform);
	virtual void ResetTransform();
	
	// Text management
	virtual void	SetFont ( VFont *inFont, GReal inScale = (GReal) 1.0);
	virtual VFont*	RetainFont();
	virtual void	SetTextPosBy (GReal inHoriz, GReal inVert);
	virtual void	SetTextPosTo (const VPoint& inHwndPos);
	virtual void	SetTextAngle (GReal inAngle);
	virtual void	GetTextPos (VPoint& outHwndPos) const;
	
	virtual void	SetSpaceKerning (GReal inSpaceExtra);
	virtual void	SetCharKerning (GReal inSpaceExtra);
	
	virtual DrawingMode	SetTextDrawingMode (DrawingMode inMode, VBrushFlags* ioFlags = NULL);

	virtual TextRenderingMode	SetTextRenderingMode( TextRenderingMode inMode);
	virtual TextRenderingMode	GetTextRenderingMode() const;
	
	// Pixel management
	virtual uBYTE	SetAlphaBlend (uBYTE inAlphaBlend);
	
	// Graphic context storage
	virtual void	NormalizeContext ();
	virtual void	SaveContext ();
	virtual void	RestoreContext ();
	
	// Graphic layer management
	
	// Create a shadow layer graphic context
	// with specified bounds and transparency
	void BeginLayerShadow(const VRect& inBounds, const VPoint& inShadowOffset = VPoint(8,8), GReal inShadowBlurDeviation = (GReal) 1.0, const VColor& inShadowColor = VColor(0,0,0,255), GReal inAlpha = (GReal) 1.0);
	
	// Create a filter or alpha layer graphic context
	// with specified bounds and filter
	// @remark : retain the specified filter until EndLayer is called
protected:
	virtual bool				_BeginLayer( const VRect& inBounds, 
										 GReal inAlpha, 
										 VGraphicFilterProcess *inFilterProcess,
										 VImageOffScreenRef inBackBufferRef = NULL, 
										 bool inInheritParentClipping = true,
										 bool inTransparent = true);
	virtual VImageOffScreenRef	_EndLayer();
public:
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
	
	/** return true if offscreen layer needs to be cleared or resized on next call to DrawLayerOffScreen or BeginLayerOffScreen/EndLayerOffScreen
	@remarks
		return true if transformed input bounds size is not equal to the background layer size
	    so only transformed bounds translation is allowed.
		return true the offscreen layer is not compatible with the current gc
	 */
	virtual bool	ShouldClearLayerOffScreen(const VRect& inBounds, VImageOffScreenRef inBackBuffer);

	// clear all layers and restore main graphic context
	virtual void	ClearLayers();
	
	/** return current clipping bounding rectangle 
	 @remarks
	 bounding rectangle is expressed in VGraphicContext normalized coordinate space 
	 (ie coordinate space with y axis pointing down - compliant with gdiplus and quickdraw coordinate spaces)
	 */  
	virtual void	GetClipBoundingBox( VRect& outBounds);
	
	/** return current graphic context transformation matrix
	 in VGraphicContext normalized coordinate space
	 (ie coordinate space with y axis pointing down - compliant with gdiplus and quickdraw coordinate spaces)
	 */
	virtual void GetTransformNormalized(VAffineTransform &outTransform);
	
	/** set current graphic context transformation matrix
	 in VGraphicContext normalized coordinate space
	 (ie coordinate space with y axis pointing down - compliant with gdiplus and quickdraw coordinate spaces)
	 */
	virtual void SetTransformNormalized(const VAffineTransform &inTransform);
	
	virtual void	GetTransformToLayer(VAffineTransform &outTransform, sLONG inIndexLayer);

	//return current graphic context native reference
	virtual VGCNativeRef GetNativeRef() const
	{
		return fContext;
	}
	
	//Text management
protected:	
	/** determine full layout mode from input layout mode and paragraph horizontal alignment */ 
	TextLayoutMode  _GetFullLayoutMode( TextLayoutMode inMode, AlignStyle inAlignHor = AL_LEFT);
public:
	virtual GReal			GetCharWidth (UniChar inChar) const;
	virtual GReal			GetTextWidth (const VString& inString, bool inRTL = false) const ;
	virtual void			GetTextBounds( const VString& inString, VRect& oRect) const;
	virtual GReal			GetTextHeight (bool inIncludeExternalLeading = false) const;
	virtual sLONG			GetNormalizedTextHeight (bool inIncludeExternalLeading = false) const;
			VFontMetrics*	GetFontMetrics() const;

	virtual TextMeasuringMode	SetTextMeasuringMode(TextMeasuringMode inMode){return TMM_NORMAL;};
	virtual TextMeasuringMode	GetTextMeasuringMode(){return TMM_NORMAL;};
	
	virtual void	DrawTextBox (const VString& inString, AlignStyle inHoriz, AlignStyle inVert, const VRect& inHwndBounds, TextLayoutMode inMode = TLM_NORMAL);

	//return text box bounds
	//@remark:
	// if input bounds width is not null, use it as max line wrapping width (or max text height in vertical layout mode)
	// if input bounds height is not null, use it as max text height (or max line wrapping width in vertical layout mode)
	virtual void	GetTextBoxBounds( const VString& inString, VRect& ioHwndBounds, TextLayoutMode inMode = TLM_NORMAL);
	
	virtual void	DrawText (const VString& inString, TextLayoutMode inMode = TLM_NORMAL, bool inRTLAnchorRight = false);

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
	
	// Multistyle text
	virtual void	DrawStyledText(  const VString& inText, VTreeTextStyle *inStyles, AlignStyle inHoriz, AlignStyle inVert, const VRect& inHwndBounds, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f);
	
	//return styled text box bounds
	//@remark:
	// if input bounds width is not null, use it as max line wrapping width (or max text height in vertical layout mode)
	// if input bounds height is not null, use it as max text height (or max line wrapping width in vertical layout mode)
	virtual void	GetStyledTextBoxBounds( const VString& inText, VTreeTextStyle *inStyles, VRect& ioBounds, const GReal inRefDocDPI = 72.0f, bool inForceMonoStyle = false);

	/** return styled text box run bounds from the specified range
	@remarks
		used only by some impl like Direct2D or CoreText in order to draw text run background
	*/
	virtual void	GetStyledTextBoxRunBoundsFromRange( const VString& inText, VTreeTextStyle *inStyles, const VRect& inBounds, std::vector<VRect>& outRunBounds, sLONG inStart = 0, sLONG inEnd = -1, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f);

	/** for the specified styled text box, return the text position at the specified coordinates
	@remarks
		output text position is 0-based

		input coordinates are in graphic context user space

		return true if input position is inside character bounds
		if method returns false, it returns the closest character index to the input position
	*/
	virtual bool	GetStyledTextBoxCharIndexFromCoord( const VString& inText, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VPoint& inPos, VIndex& outCharIndex, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f);

	/** for the specified styled text box, return the caret position & height of text at the specified text position
	@remarks
		text position is 0-based

		caret should be drawed from outCaretPos to VPoint(outCaretPos.x,OutCaretPos.y+outTextHeight)
	*/
	virtual void	GetStyledTextBoxCaretMetricsFromCharIndex( const VString& inText, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading = true, const bool inCaretUseCharMetrics = true, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f);

	//Drawing primitives
	
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
	virtual void	DrawLines (const GReal* inCoords, sLONG inCoordCount);	// Optimized version, allows use of JoinStyle
	virtual void	DrawLineTo (const VPoint& inHwndEnd);
	virtual void	DrawLineBy (GReal inWidth, GReal inHeight);
	
	virtual void	MoveBrushTo (const VPoint& inHwndPos);
	virtual void	MoveBrushBy (GReal inWidth, GReal inHeight);
	
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
	
	virtual void	DrawPicture (const class VPicture& inPicture,const VRect& inHwndBounds,class VPictureDrawSettings *inSet=0) ;
	//virtual void	DrawPicture (const VOldPicture& inPicture, PictureMosaic inMosaicMode, const VRect& inHwndBounds, TransferMode inDrawingMode = TM_COPY);
	virtual void	DrawFileBitmap(const VFileBitmap *inFileBitmap, const VRect& inHwndBounds);
	virtual void	DrawPictureFile (const VFile& inFile, const VRect& inHwndBounds);
	virtual void	DrawIcon (const VIcon& inIcon, const VRect& inHwndBounds, PaintStatus inState = PS_NORMAL, GReal inScale = (GReal) 1.0); // JM 050507 ACI0049850 rajout inScale

	//draw picture data
	virtual void    DrawPictureData (const VPictureData *inPictureData, const VRect& inHwndBounds,VPictureDrawSettings *inSet=0);
	
	/** clear current graphic context area
	@remarks
		Quartz2D impl: inBounds cannot be equal to NULL (do nothing if it is the case)
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
	virtual void	CopyContentTo (VGraphicContext* inDestContext, const VRect* inSrcBounds = NULL, const VRect* inDestBounds = NULL, TransferMode inMode = TM_COPY, const VRegion* inMask = NULL);
	virtual VPictureData* CopyContentToPictureData();
	virtual void	Flush ();
	
	// Pixel drawing priUseReversedAxismitives
	virtual void	FillUniformColor (const VPoint& inContextPos, const VColor& inColor, const VPattern* inPattern = NULL);
	virtual void	SetPixelColor (const VPoint& inContextPos, const VColor& inColor);
	virtual VColor*	GetPixelColor (const VPoint& inContextPos) const;
	//virtual VOldPicture*	GetPicture () const { return NULL; };

	// Port setting and restoration (use before and after any drawing suite)
	virtual	void	BeginUsingContext (bool inNoDraw = false);
	virtual	void	EndUsingContext () {};

	virtual	PortRef		GetParentPort () const { return fQDPort ? fQDPort->GetPort() : NULL; };
	virtual	ContextRef	GetParentContext () const { return fContext; };
	
#if VERSIONMAC	
	virtual void	GetParentPortBounds( VRect& outBounds);
#endif
	
	// Utilities
	virtual	Boolean	UseEuclideanAxis ()	{ _UseQuartzAxis(); return true; };
	virtual	Boolean	UseReversedAxis ()	{ _UseQDAxis(); return true; };


				/** transform point from QD coordinate space to Quartz2D coordinate space */
	virtual	void				QDToQuartz2D(VPoint &ioPos)
			{
				VRect portBounds;
				GetParentPortBounds(portBounds);
				ioPos.SetY(portBounds.GetHeight()-ioPos.GetY());
			}

			/** transform rect from QD coordinate space to Quartz2D coordinate space */
	virtual	void				QDToQuartz2D(VRect &ioRect)
			{
				VRect portBounds;
				GetParentPortBounds(portBounds);
				ioRect.SetY(portBounds.GetHeight()-(ioRect.GetY()+ioRect.GetHeight()));
			}

			/** transform point from Quartz2D coordinate space to QD coordinate space */
	virtual	void				Quartz2DToQD(VPoint &ioPos)
			{
				QDToQuartz2D(ioPos);
			}

			/** transform rect from Quartz2D coordinate space to QD coordinate space */
	virtual	void				Quartz2DToQD(VRect &ioRect)
			{
                QDToQuartz2D( ioRect);
			}
	
	void	SetQDBounds( const CGRect& inQDBounds);
	
	static PatternRef	_CreateStandardPattern (ContextRef inContext, PatternStyle inStyle);
	static void	_ReleasePattern (ContextRef inContext, PatternRef inPatternRef);
	static void	_ApplyPattern (ContextRef inContext, PatternStyle inStyle, const VColor& inColor, PatternRef inPatternRef, FillRuleType inFillRule = FILLRULE_NONZERO);
	static void	_CGPatternDrawPatternProc (void* inInfo, CGContextRef inContext);
	static void	_GetLineStyleElements (PatternStyle inStyle, uLONG inMaxCount, GReal* outElements, uLONG& outCount);
	
	static GradientRef	_CreateAxialGradient (ContextRef inContext, const VAxialGradientPattern* inGradient);
	static GradientRef	_CreateRadialGradient (ContextRef inContext, const VRadialGradientPattern* inGradient);
	static void	_ReleaseGradient (GradientRef inShading);
	static void	_ApplyGradient (ContextRef inContext, GradientRef inShading, FillRuleType inFillRule = FILLRULE_NONZERO);
	static void	_ApplyGradientWithTransform (ContextRef inContext, GradientRef inShading, const VAffineTransform& inMat, FillRuleType inFillRule = FILLRULE_NONZERO);
	static void	_CGShadingComputeValueProc (void* inInfo, const GReal* inValue, GReal* outValue);
	static void	_CGShadingTileComputeValueProc (void* inInfo, const GReal* inValue, GReal* outValue);
	static void	_CGShadingTileFlipComputeValueProc (void* inInfo, const GReal* inValue, GReal* outValue);
	static void	_CGShadingReleaseInfoProc (void* inInfo);
	
	static CGRect	QDRectToCGRect (const Rect& inQDRect);
	
	// Debug utils
	static void		RevealUpdate (WindowRef inWindow);
	static void		RevealClipping (ContextRef inContext);
	static void		RevealBlitting (ContextRef inContext, const RgnRef inRegion);
	static void		RevealInval (ContextRef inContext, const RgnRef inRegion);
	
	// Class initialization
	static Boolean	Init ();
	static void 	DeInit ();

protected:
			CGContextRef		fContext;
			VMacQDPortBridge*	fQDPort;
			CGRect				fQDBounds;	// qd origin used for apis that needs coordinates relative to the window
			
			CGRect				fPrinterAdjustedPageRect;
			CGRect				fPrinterAdjustedPaperRect;
			
			VRegion*			fQDContextSavedClip;

			VColor				fTextColor;		// text foreground color
			GReal				fTextAngle;		// Some context settings
			GReal				fShadowBlurness;
			CGSize				fShadowOffset;
			
			VColor				fLineColor;
			VColor				fFillColor;
			
			VPattern*			fFillPattern;
			std::vector<VPattern*>	fFillPatternStack;
			
			VPattern*			fLinePattern;
			std::vector<VPattern*>	fLinePatternStack;
			
			GReal				fDashPatternOffset; //current dash pattern start offset (always in user units)
			VLineDashPattern	fDashPattern; //current dash pattern (empty vector for no dash pattern) 

			bool				fIsStrokeDashPatternUnitEqualToStrokeWidth; //true -> dash pattern unit = stroke width; false (default) -> dash pattern unit = user unit
			std::vector<bool>	fIsStrokeDashPatternUnitEqualToStrokeWidthStack;

			GReal				fLineWidth;
			std::vector<GReal>	fLineWidthStack;

			bool				fUseLineDash;
			std::vector<bool>	fLineDashStack;

	mutable	Boolean				fAxisInverted;
			uBYTE				fAlphaBlend;
			VPoint				fTextPos;
			VFont*				fFont;
	mutable	VFontMetrics*		fFontMetrics;
			TextRenderingMode	fTextRenderingMode;
	mutable TextLayoutMode		fTextLayoutMode;

			GReal				fGlobalAlpha;
			std::vector<GReal>	fGlobalAlphaStack;

			std::vector<Boolean>*	fAxisInvertedStack;	// Additional context store/restore data
			sLONG				fSaveRestoreCount;
	
	// Utilities
	void	_CreatePathFromOval (const VRect& inHwndBounds);
	void	_CreatePathFromPolygon (const VPolygon& inHwndPolygon, bool inFillOnly = false);
	void	_CreatePathFromRegion (const VRegion& inHwndRegion);
	void	_CreatePathFromRect (const VRect& inHwndRect);
	
	void	_StrokePath ();
	void	_FillPath ();
	void	_FillStrokePath ();
	
	void	_UseQDAxis () const;
	void	_UseQuartzAxis () const;

	void	_InitFromQDBridge(VMacQDPortBridge* inQDPort, const CGRect& inQDBounds,bool inForPrinting);

	/** draw text layout 
	@remarks
		text layout left top origin is set to inTopLeft
		text layout width is determined automatically if inTextLayout->GetMaxWidth() == 0.0f, otherwise it is equal to inTextLayout->GetMaxWidth()
		text layout height is determined automatically if inTextLayout->GetMaxHeight() == 0.0f, otherwise it is equal to inTextLayout->GetMaxHeight()
	*/
	virtual void	_DrawTextLayout( VTextLayout *inTextLayout, const VPoint& inTopLeft = VPoint());
	
	/** return text layout bounds 
		@remark:
			text layout origin is set to inTopLeft
			on output, outBounds contains text layout bounds including any glyph overhangs

			on input, max width is determined by inTextLayout->GetMaxWidth() & max height by inTextLayout->GetMaxHeight()
			(if max width or max height is equal to 0.0f, it is assumed to be infinite)

			if text is empty and if inReturnCharBoundsForEmptyText == true, returned bounds will be set to a single char bounds 
			(useful while editing in order to compute initial text bounds which should not be empty in order to draw caret;
			 if you use text layout only for read-only text, you should let inReturnCharBoundsForEmptyText to default which is false)
	*/
	virtual void	_GetTextLayoutBounds( VTextLayout *inTextLayout, VRect& outBounds, const VPoint& inTopLeft = VPoint(), bool inReturnCharBoundsForEmptyText = false);


	/** return text layout run bounds from the specified range 
	@remarks
		text layout origin is set to inTopLeft
	*/
	virtual void	_GetTextLayoutRunBoundsFromRange( VTextLayout *inTextLayout, std::vector<VRect>& outRunBounds, const VPoint& inTopLeft = VPoint(), sLONG inStart = 0, sLONG inEnd = -1);


	/** return the caret position & height of text at the specified text position in the text layout
	@remarks
		text layout origin is set to inTopLeft

		text position is 0-based

		caret should be drawed from outCaretPos to VPoint(outCaretPos.x,OutCaretPos.y+outTextHeight)
		if text layout is drawed at inTopLeft

		by default inCaretUseCharMetrics = true, so output caret metrics are based on the run metrics of the input character (suitable to draw caret)
		but if inCaretUseCharMetrics = false, output caret metrics are based on the full line metrics (use this one to get line height)
	*/
	virtual void	_GetTextLayoutCaretMetricsFromCharIndex( VTextLayout *inTextLayout, const VPoint& inTopLeft, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading = true, const bool inCaretUseCharMetrics = true);


	/** return the text position at the specified coordinates
	@remarks
		text layout origin is set to inTopLeft (in gc user space)
		the hit coordinates are defined by inPos param (in gc user space)

		output text position is 0-based

		return true if input position is inside character bounds
		if method returns false, it returns the closest character index to the input position
	*/
	virtual bool _GetTextLayoutCharIndexFromPos( VTextLayout *inTextLayout, const VPoint& inTopLeft, const VPoint& inPos, VIndex& outCharIndex);

	/* update text layout
	@remarks
		build text layout according to layout settings & current graphic context settings

		this method is called only from VTextLayout::_UpdateTextLayout
	*/
	virtual void _UpdateTextLayout( VTextLayout *inTextLayout);

	/** internally called while replacing text in order to update impl-specific stuff */
	virtual void _PostReplaceText( VTextLayout *inTextLayout);

private:
	// Initialization
	void	_Init ();
};


class XTOOLBOX_API VMacQuartzOffscreenContext : public VMacQuartzGraphicContext
{
public:
			VMacQuartzOffscreenContext (CGContextRef inSourceContext, Boolean inReversed, const CGRect& inQDBounds);
			VMacQuartzOffscreenContext (const VMacQuartzGraphicContext& inSourceContext);
			VMacQuartzOffscreenContext (const VMacQuartzOffscreenContext& inOriginal);
	virtual	~VMacQuartzOffscreenContext ();
	
	// Offscreen management
	virtual Boolean	CreatePicture (const VRect& inHwndBounds);
	virtual void	ReleasePicture ();
	
	// Inherited from VGraphicContext
	virtual void	CopyContentTo (VGraphicContext* inDestContext, const VRect* inSrcBounds = NULL, const VRect* inDestBounds = NULL, TransferMode inMode = TM_COPY, const VRegion* inMask = NULL);
	
	// Accessors
	CGPDFDocumentRef	MAC_RetainPDFDocument () const;
	
protected:
	CGContextRef	fSrcContext;
	VFile*		fTempFile;
	Boolean		fPageClosed;

private:
	// Initialization
	void	_Init ();
	void	_Dispose ();
};


class XTOOLBOX_API VMacQuartzBitmapContext : public VMacQuartzGraphicContext
{
public:
			/** create bitmap context from scratch */
			VMacQuartzBitmapContext(const VRect& inBounds, Boolean inReversed,bool inTransparent=true);

			/** retain bitmap associated with graphic context

				inherited from VGraphicContext
			@remarks
				NULL if graphic context is not a bitmap graphic context
			*/
			VBitmapNativeRef RetainBitmap() const
			{ 
				return (VBitmapNativeRef)MAC_RetainCGImage();
			}

			VMacQuartzBitmapContext (CGContextRef inSourceContext, Boolean inReversed, const CGRect& inQDBounds);
			VMacQuartzBitmapContext (const VMacQuartzGraphicContext& inSourceContext);
			VMacQuartzBitmapContext (const VMacQuartzBitmapContext& inOriginal);
			VMacQuartzBitmapContext();
	virtual	~VMacQuartzBitmapContext ();
	
	// Offscreen management
	virtual Boolean	CreateBitmap (const VRect& inHwndBounds,bool inTransparent = true);
	virtual void	ReleaseBitmap ();
	
	// Inherited from VGraphicContext
	virtual void	CopyContentTo (VGraphicContext* inDestContext, const VRect* inSrcBounds = NULL, const VRect* inDestBounds = NULL, TransferMode inMode = TM_COPY, const VRegion* inMask = NULL);
	virtual VPictureData* CopyContentToPictureData();
	
	virtual void	FillUniformColor (const VPoint& inContextPos, const VColor& inColor, const VPattern* inPattern = NULL);
	virtual void	SetPixelColor (const VPoint& inContextPos, const VColor& inColor);
	virtual VColor*	GetPixelColor (const VPoint& inContextPos) const;

	// Accessors
	CGImageRef		MAC_RetainCGImage () const;

protected:
	CGContextRef	fSrcContext;
	uBYTE*			fBitmapData;
	mutable CGImageRef	fImageCache;
	
	void	_CreateImageCache () const;

private:
	// Initialization
	void	_Init ();
	void	_Dispose ();
};


typedef VMacQuartzGraphicContext	VNativeGraphicContext;
typedef VMacQuartzOffscreenContext	VOffscreenGraphicContext;
typedef VMacQuartzBitmapContext		VBitmapGraphicContext;

inline GReal	TwipsToPixels  (GReal x) { return (x / (GReal) 20.0); };
inline GReal	PixelsToTwips  (GReal x) { return (x * (GReal) 20.0); };
inline GReal	CmsToPixels    (GReal x) { return ((x * (GReal) 72.0) / (GReal) 2.54); };
inline GReal	PixelsToCms    (GReal x) { return ((x * (GReal) 2.54) / (GReal) 72.0); };
inline GReal	InchesToPixels (GReal x) { return (x * (GReal) 72.0); };
inline GReal	PixelsToInches (GReal x) { return (x / (GReal) 72.0); };
inline GReal	PointsToPixels (GReal x) { return x; };
inline GReal	PixelsToPoints (GReal x) { return x; };

END_TOOLBOX_NAMESPACE

#endif

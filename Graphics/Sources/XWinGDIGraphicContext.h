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
#ifndef __XWinGDIGraphicContext__
#define __XWinGDIGraphicContext__

#include "Graphics/Sources/VGraphicContext.h"

BEGIN_TOOLBOX_NAMESPACE

// Needed declarations
class VAffineTransform;
class VPattern;
class VAxialGradientPattern;
class VRadialGradientPattern;


class XTOOLBOX_API VWinGDIGraphicContext : public VGraphicContext
{
public:
				VWinGDIGraphicContext (HDC inContext);
				VWinGDIGraphicContext (HWND inWindow);
	explicit	VWinGDIGraphicContext (const VWinGDIGraphicContext& inOriginal);
	virtual		~VWinGDIGraphicContext ();
	
	virtual		bool IsGDIImpl() const { return true; }

	virtual		GReal GetDpiX() const;
	virtual		GReal GetDpiY() const;
	
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
	

	virtual void	SetLineWidth (GReal inWidth, VBrushFlags* ioFlags = NULL);
	virtual GReal	GetLineWidth () const { return fLineWidth; }
	virtual void	SetLineStyle (CapStyle inCapStyle, JoinStyle inJoinStyle, VBrushFlags* ioFlags = NULL);
	virtual void	SetLineCap (CapStyle inCapStyle, VBrushFlags* ioFlags = NULL);
	virtual void	SetLineJoin(JoinStyle inJoinStyle, VBrushFlags* ioFlags = NULL); 
	virtual void	SetTextColor (const VColor& inColor, VBrushFlags* ioFlags = NULL);
	virtual void	GetTextColor (VColor& outColor) const;
	virtual void	SetBackColor (const VColor& inColor, VBrushFlags* ioFlags = NULL);
	virtual void	GetBrushPos (VPoint& outHwndPos) const { outHwndPos = fBrushPos; };
	virtual GReal	SetTransparency (GReal inAlpha);	// Global Alpha - added to existing line or fill alpha
	virtual void	SetShadowValue (GReal inHOffset, GReal inVOffset, GReal inBlurness);	// Need a call to SetDrawingMode to update changes

	virtual DrawingMode	SetDrawingMode (DrawingMode inMode, VBrushFlags* ioFlags = NULL);
	
	virtual TextMeasuringMode	SetTextMeasuringMode(TextMeasuringMode inMode){return TMM_NORMAL;};
	virtual TextMeasuringMode	GetTextMeasuringMode(){return TMM_NORMAL;};

	virtual void	EnableAntiAliasing ();
	virtual void	DisableAntiAliasing ();
	virtual bool	IsAntiAliasingAvailable ();
	
			/** return true if font glyphs are aligned to pixel boundary */
	virtual	bool	IsFontPixelSnappingEnabled() const { return true; }

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
	virtual void	GetTextPos (VPoint& outHwndPos) const { outHwndPos = fTextPos; };
	
	virtual void	SetSpaceKerning (GReal inRatio);
	virtual void	SetCharKerning (GReal inRatio);
	
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
	virtual GReal	GetCharWidth (UniChar inChar) const;
	virtual GReal	GetTextWidth (const VString& inString, bool inRTL = false) const;
	virtual GReal	GetTextHeight (bool inIncludeExternalLeading = false) const;
	virtual sLONG	GetNormalizedTextHeight(bool inIncludeExternalLeading=false) const;
	
	// Drawing primitives
	virtual void	DrawTextBox (const VString& inString, AlignStyle inHoriz, AlignStyle inVert, const VRect& inHwndBounds, TextLayoutMode inMode = TLM_NORMAL);
	virtual void	GetTextBoxBounds( const VString& inString, VRect& ioHwndBounds, TextLayoutMode inMode = TLM_NORMAL);
	
	virtual void	DrawText (const VString& inString, TextLayoutMode inMode = TLM_NORMAL, bool inRTLAnchorRight = false);
			/**	draw text at the specified position
				
				@param inString
					text string
				@param inLayoutMode
				  layout mode (here only TLM_VERTICAL and TLM_RIGHT_TO_LEFT are used)
				
				@remark: 
					this method does not make any layout formatting 
					text is drawed on a single line from the specified starting coordinate
					(which is relative to the first glyph horizontal or vertical baseline)
					useful when you want to position exactly a text at a specified coordinate
					without taking account extra spacing due to layout formatting (which is implementation variable)
				@note
					this method is used by SVG component
			*/
	virtual void	DrawText (const VString& inString, const VPoint& inPos, TextLayoutMode inLayoutMode = TLM_NORMAL, bool inRTLAnchorRight = false);

    virtual void	GetStyledTextBounds( const VString& inText, VTreeTextStyle *inStyles, VRect& outBounds);


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
	virtual void	QDDrawLines (const GReal* inCoords, sLONG inCoordCount);	// Optimized version, allows use of JoinStyle
	
	virtual void	DrawPicture (const class VPicture& inPicture,const VRect& inHwndBounds,class VPictureDrawSettings *inSet=0) ;
	virtual void	DrawPictureData (const VPictureData *inPictureData, const VRect& inHwndBounds,class VPictureDrawSettings *inSet=0);
	virtual void	DrawFileBitmap(const VFileBitmap *inFileBitmap, const VRect& inHwndBounds);
	virtual void	DrawPictureFile (const VFile& inFile, const VRect& inHwndBounds);
	virtual void	DrawIcon (const VIcon& inIcon, const VRect& inHwndBounds, PaintStatus inState = PS_NORMAL, GReal inScale = (GReal) 1.0); // JM 050507 ACI0049850 rajout inScale

	// Clipping
	virtual void	ClipRect (const VRect& inHwndRect, Boolean inAddToPrevious = false, Boolean inIntersectToPrevious = true);
	virtual void	ClipRegion (const VRegion& inHwndRgn, Boolean inAddToPrevious = false, Boolean inIntersectToPrevious = true);
	virtual void	GetClip (VRegion& outHwndRgn) const;
	/** return current clipping bounding rectangle 
	 @remarks
		bounding rectangle is expressed in VGraphicContext normalized coordinate space 
	    (ie coordinate space with y axis pointing down - compliant with gdiplus and quickdraw coordinate spaces)
	 */  
	virtual void	GetClipBoundingBox( VRect& outBounds);
	
	virtual void	SaveClip ();
	virtual void	RestoreClip ();

	// Content bliting primitives
	virtual void	CopyContentTo (VGraphicContext* inDestContext, const VRect* inSrcBounds = NULL, const VRect* inDestBounds = NULL, TransferMode inMode = TM_COPY, const VRegion* inMask = NULL) ;
	virtual VPictureData* CopyContentToPictureData();
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
	virtual	ContextRef	GetParentContext () const { return fContext; };
	
	virtual	bool BeginQDContext();
	virtual	void EndQDContext(bool inDontRestoreClip = false);

	// Utilities
	virtual	Boolean	UseEuclideanAxis () { return false; };
	virtual	Boolean	UseReversedAxis () { return true; };
	
	
	void	SelectFillBrushDirect (HGDIOBJ inBrush);
	void	SelectLineBrushDirect (HGDIOBJ inBrush);
	void	SelectPenDirect (HGDIOBJ inBrush);
	void	SetTextCursorInfo (VString& inFont, sLONG inSize, Boolean inBold, Boolean inItalic, sLONG inX, sLONG inY);
	
	void	_AdjustLineCoords (GReal* ioCoords);
	void	_QDAdjustLineCoords(GReal* ioCoords);
	
	static PatternRef	_CreateStandardPattern (ContextRef inContext, PatternStyle inStyle, void* ioStorage);
	static void	_ReleasePattern (ContextRef inContext, PatternRef inPatternRef);
	static void	_ApplyPattern (ContextRef inContext, PatternStyle inStyle, const VColor& inColor, PatternRef inPatternRef);
	
	static GradientRef	_CreateAxialGradient (ContextRef inContext, const VAxialGradientPattern* inGradient);
	static GradientRef	_CreateRadialGradient (ContextRef inContext, const VRadialGradientPattern* inGradient);
	static Boolean	_GetStdPattern (PatternStyle inStyle, void* ioPattern);
	static void	_ReleaseGradient (GradientRef inShading);
	static void	_ApplyGradient (ContextRef inContext, GradientRef inShading);

	static void	_DrawShadow (ContextRef inContext, const VPoint& inOffset, const VRect& inHwndBounds);
	static void	_DrawShadow (ContextRef inContext, const VPoint& inOffset, const VPoint& inStartPoint, const VPoint& inEndPoint);
	static void	_DrawShadow (ContextRef inContext, const VPoint& inOffset, const GReal inStartPointX ,const GReal inStartPointY, const GReal inEndPointX,const GReal inEndPointY);

	static GReal	GetLogPixelsX () { return sLogPixelsX; };
	static GReal	GetLogPixelsY () { return sLogPixelsY; };

	// Debug Utils
	static void	RevealUpdate (HWND inWindow);
	static void	RevealClipping (ContextRef inContext);
	static void	RevealBlitting (ContextRef inContext, const RgnRef inHwndRegion);
	static void	RevealInval (ContextRef inContext, const RgnRef inHwndRegion);
	
	// Class initialization
	static Boolean	Init ();
	static void 	DeInit ();
	static long		DebugCheckHDC(HDC inDC);
	static void		DebugEnableCheckHDC(bool inSet);
	static void		DebugRegisterHDC(HDC inDC);
	static void		DebugUnRegisterHDC(HDC inDC);

	/** create a VRegion in gc local user space from the GDI region (device space) */
	void RegionGDIToLocal(HRGN inRgn, VRegion& outRegion, bool inOwnIt = true, bool useViewportOrg = true, bool inUseWorldTransform = true) const;
	
	/** create a GDI region (device space) from a region in local user space */
	HRGN RegionLocalToGDI(const VRegion& inRegion, bool useViewportOrg = true, bool inUseWorldTransform = true) const;

	HGDIOBJ	_CreatePen();

protected:
	virtual bool				_BeginLayer(	 const VRect& inBounds, 
											 GReal inAlpha, 
											 VGraphicFilterProcess *inFilterProcess,
											 VImageOffScreenRef inBackBufferRef = NULL,
											 bool inInheritParentClipping = true,
											 bool inTransparent = true);
	virtual VImageOffScreenRef _EndLayer();

	virtual bool				DrawLayerOffScreen(const VRect& inBounds, VImageOffScreenRef inBackBuffer, bool inDrawAlways = false);

	virtual bool				ShouldClearLayerOffScreen(const VRect& inBounds, VImageOffScreenRef inBackBuffer);

	/** offscreen parent hdc 
	@remarks
		if between BeginLayerOffScreen/EndLayerOffScreen
	*/
	HDC fOffScreenParent;

	/** offscreen use count 
	@remarks
		if between BeginLayerOffScreen/EndLayerOffScreen
	*/
	sLONG fOffScreenUseCount;

	/** current offscreen 
	@remarks
		if between BeginLayerOffScreen/EndLayerOffScreen
	*/
	VImageOffScreenRef fOffScreen;

	mutable GReal fDpiX, fDpiY;
	BOOL		fNeedReleaseDC;
	HDC			fContext;
	HWND		fHwnd;
	GReal		fImmPx;			// Some context settings
	GReal		fImmPy;
	VColor		fLineColor;
	VColor		fFillColor;
	GReal		fLineWidth;
	sLONG		fLineStyle;
	GReal		fTextAngle;
	VPoint		fTextPos;
	VPoint		fBrushPos;
	VColor		fPixelForeColor;
	VColor		fPixelBackColor;
	VPattern*	fFillPattern;
	VLineDashPattern fLineDashPattern;
	VPoint		fShadowOffset;
	Boolean		fDrawShadow;
	uBYTE		fAlphaBlend;
	sLONG		fContextRefcon;
	HGDIOBJ		fCurrentPen;	// Context store/restore
	HGDIOBJ		fCurrentBrush;
	
	HGDIOBJ		fFillBrush;

	ColorRef	fSavedTextColor;
	ColorRef	fSavedBackColor;
	HGDIOBJ		fSavedPen;
	HGDIOBJ		fSavedBrush;
	sLONG		fSavedRop;
	sLONG		fSavedBkMode;
	HFONT		fSavedFont;
	int			fSavedGraphicsMode;

	SIZE		fSaveViewPortExtend;
	SIZE		fSaveWindowExtend;
	int			fSaveMapMode;
	
	UINT		fSavedTextAlignment;
	
	typedef struct _PortSettings
	{
		int fSavedPortSettings;
		HGDIOBJ		fSavedPen;
		HGDIOBJ		fSavedBrush;
		sLONG		fSavedRop;
		sLONG		fSavedBkMode;
		HFONT		fSavedFont;
		UINT		fSavedTextAlignment;
		HGDIOBJ		fCurrentPen;
		HGDIOBJ		fCurrentBrush;
		HGDIOBJ		fFillBrush;
		VColor		fLineColor;
		VColor		fFillColor;
		GReal		fLineWidth;
		sLONG		fLineStyle;
		bool		fHairline;
		bool		fFillBrushDirty;
		bool		fLineBrushDirty;
	};
	
	std::vector<_PortSettings>	fSavedPortSettings;
	VHandleStream*		fSaveStream;
	VArrayOf<RgnRef>	fClipStack;

	bool				fQDContext_NeedsBeginUsing;

	static GReal	sLogPixelsX;
	static GReal	sLogPixelsY;
	static VColor*	sShadow1Color;
	static VColor*	sShadow2Color;
	static VColor*	sShadow3Color;

private:
	// Initialization
	void	_Init ();
	void	_Dispose ();
	
	void	_InitPrinterScale();

	void	_LPtoDP(VPoint &ioPoint);
	void	_LPtoDP(VRect &ioRect);
	
	void	_DPtoLP(VPoint &ioPoint);
	void	_DPtoLP(VRect &ioRect);
	
	void	_SelectBrushDirect (HGDIOBJ inBrush);
	BOOL	_DeleteObject(HGDIOBJ inObj) const;
	void	_UpdateLineBrush();
	bool	fFillBrushDirty;
	void	_UpdateFillBrush();
	bool	fLineBrushDirty;
};


class XTOOLBOX_API VWinGDIOffscreenContext : public VWinGDIGraphicContext
{
public:
			VWinGDIOffscreenContext (HDC inSourceContext);
			VWinGDIOffscreenContext (const VWinGDIGraphicContext& inSourceContext);
			VWinGDIOffscreenContext (const VWinGDIOffscreenContext& inOriginal);
	virtual	~VWinGDIOffscreenContext ();
	
	// Offscreen management
	virtual Boolean	CreatePicture (const VRect& inHwndBounds);
	virtual void	ReleasePicture ();
	
	// Inherited from VGraphicContext
	virtual void	CopyContentTo ( VGraphicContext* inDestContext, const VRect* inSrcBounds = NULL, const VRect* inDestBounds = NULL, TransferMode inMode = TM_COPY, const VRegion* inMask = NULL);

	// Accessors
	HENHMETAFILE	WIN_CreateEMFHandle ();

protected:
	HDC		fSrcContext;
	HENHMETAFILE	fPicture;
	
	void	_ClosePicture ();

private:
	// Initialization
	void	_Init ();
	void	_Dispose ();
};


class XTOOLBOX_API VWinGDIBitmapContext : public VWinGDIGraphicContext
{
public:
			VWinGDIBitmapContext (HDC inSourceContext);
			VWinGDIBitmapContext (const VWinGDIGraphicContext& inSourceContext);
			VWinGDIBitmapContext (const VWinGDIBitmapContext& inOriginal);
	virtual	~VWinGDIBitmapContext ();
	
	// Offscreen management
	virtual Boolean	CreateBitmap (const VRect& inHwndBounds);
	virtual void	ReleaseBitmap ();
	
	// Inherited from VGraphicContext
	virtual void	CopyContentTo ( VGraphicContext* inDestContext, const VRect* inSrcBounds = NULL, const VRect* inDestBounds = NULL, TransferMode inMode = TM_COPY, const VRegion* inMask = NULL);
	virtual VPictureData* CopyContentToPictureData();
	virtual// Accessors
	HENHMETAFILE	WIN_CreateEMFHandle () const;

protected:
	HDC		fSrcContext;
	HBITMAP	fBitmap;
	HBITMAP	fSavedBitmap;

private:
	// Initialization
	void	_Init ();
	void	_Dispose ();
};


/** static class used to reset GDI transform to identity (transform is restored on destruction) */
class StGDIResetTransform : public VObject
{
	public:
	StGDIResetTransform(HDC hdc);
	virtual ~StGDIResetTransform();

	private:
	HDC fHDC;
	int fGM;
	XFORM fCTM;
};


typedef VWinGDIGraphicContext	VNativeGraphicContext;
typedef VWinGDIOffscreenContext	VOffscreenGraphicContext;
typedef VWinGDIBitmapContext	VBitmapGraphicContext;

inline GReal	TwipsToPixels  (GReal x) { return (x * VWinGDIGraphicContext::GetLogPixelsX() / (GReal) 1440.0); };
inline GReal	PixelsToTwips  (GReal x) { return (x * (GReal) 1440.0 / VWinGDIGraphicContext::GetLogPixelsX()); };
inline GReal	CmsToPixels    (GReal x) { return (x * VWinGDIGraphicContext::GetLogPixelsX() / (GReal) 2.54); };
inline GReal	PixelsToCms    (GReal x) { return (x * (GReal) 2.54 / VWinGDIGraphicContext::GetLogPixelsX()); };
inline GReal	InchesToPixels (GReal x) { return (x * VWinGDIGraphicContext::GetLogPixelsX()); };
inline GReal	PixelsToInches (GReal x) { return (x / VWinGDIGraphicContext::GetLogPixelsX()); };
inline GReal	PointsToPixels (GReal x) { return (x * VWinGDIGraphicContext::GetLogPixelsX() / (GReal) 72.0); };
inline GReal	PixelsToPoints (GReal x) { return (x * (GReal) 72.0 / VWinGDIGraphicContext::GetLogPixelsX()); };

END_TOOLBOX_NAMESPACE

#endif

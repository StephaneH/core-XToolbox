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
#ifndef __XWinD2DGraphicContext__
#define __XWinD2DGraphicContext__

// !! HACK ALERT !!
// this header file is not part of VGraphics.h
// but is included directly by SVG Component

#undef DrawText 

#include <stack>

#pragma warning( push )
#pragma warning (disable: 4263)	//  member function does not override any base class virtual member function
#pragma warning (disable: 4264)

#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>
#include <wincodecsdk.h>
#pragma comment(lib, "WindowsCodecs.lib")
#include <winuser.h>
#include <atlcomcli.h>

#pragma warning ( pop)

#include "XWinGDIGraphicContext.h"

BEGIN_TOOLBOX_NAMESPACE

//set to 1 if D2D per-frame drawing is single-threaded
//(set to 1 for 4D because 4D GUI is single-threaded)
//@remarks
//	D2D factory can still be multi-threaded:
//  this setting disables only mutexs while drawing
#define D2D_GUI_SINGLE_THREADED 0
#if D2D_GUI_SINGLE_THREADED
#define D2D_GUI_RESOURCE_MUTEX_LOCK(type) 
#else
#define D2D_GUI_RESOURCE_MUTEX_LOCK(type) VTaskLock lock(&(fMutexResources[type]));
#endif

//Direct2D uses device independant unit (DIP) equal to 1/96 inches
//(1 DIP = 1 render target pixel only if render target DPI = 96 - default for render targets & bitmaps)
//default unit is DIP for Direct2D (for instance coordinates) & DWrite (for instance font metrics) 
#if D2D_GDI_COMPATIBLE
//use point to GDI screen pixel conversion (because 1 DIP = 1 GDI pixel if D2D_GDI_COMPATIBLE == 1)
//(otherwise DIP unit would not be consistent with GDI pixel)
inline GReal D2D_PointToDIP(GReal x)	{ return PointsToPixels(x); }
inline GReal D2D_DIPToPoint(GReal x)	{ return PixelsToPoints(x); }
#else
//use not GDI-compatible point to DIP conversion
inline GReal D2D_PointToDIP(GReal x)	{ return x*96.0f/72.0f; }
inline GReal D2D_DIPToPoint(GReal x)	{ return x*72.0f/96.0f; }
#endif

/** set to 1 in to draw/read with GDI in render target gdi-compatible dc (safer)
	set to 0 in order to draw/read with GDI in render target parent dc ie window or device dc 
												in which is presented the render target 
*/
#define D2D_RENDER_TARGET_USE_GDI_COMPATIBLE_DC D2D_GDI_COMPATIBLE


/** set to 1 in order to use GDIPlus bitmap hdc while using GDI within a transparent bitmap render target
@remarks
	this is a workaround while drawing with GDI over a transparent render target 
*/
#define D2D_GDI_USE_GDIPLUS_BITMAP_FOR_TRANSPARENCY D2D_GDI_COMPATIBLE

// Needed declarations
class VPattern;
class VGradientPattern;
class VAxialGradientPattern;
class VRadialGradientPattern;
class VGraphicFilterProcess;

#define D2D_RECT(x) D2D1::RectF(x.GetLeft(), x.GetTop(), x.GetRight(), x.GetBottom())
#define D2D_POINT(x) D2D1::Point2F(x.GetX(), x.GetY())
#define D2D_COLOR(color) D2D1::ColorF( color.GetRed()/255.0f, color.GetGreen()/255.0f, color.GetBlue()/255.0f, color.GetAlpha()/255.0f) 
#define VCOLOR_FROM_D2D_COLOR(color) VColor((uBYTE)(color.r*255),(uBYTE)(color.g*255),(uBYTE)(color.b*255),(uBYTE)(color.a*255))

#define D2D_CACHE_RESOURCE_HARDWARE TRUE
#define D2D_CACHE_RESOURCE_SOFTWARE FALSE

/** max size for layer texture cache
@remarks
	in Direct2D, layers are D3D textures so it is recommended
	to cache layers in order to avoid to re-allocate it at each frame
	
	there is one layer cache per resource domain (hardware & software)
	(layers can be shared by render targets belonging to the same resource domain)

	if more than D2D_LAYER_CACHE_MAX is used at one time, new layers will be allocated dynamically

	do not set this limit too high because layers can be quite video memory consuming
*/
#define D2D_LAYER_CACHE_MAX	4


/** max size for gradient stop collection texture cache 
@remarks
	in Direct2D, gradient stop collections are D3D textures so it is recommended
	to cache gradient stop collections in order to avoid to re-allocate it at each frame

	there is one gradient stop collection cache per render target

	as gradient stop collections are 1D textures you can set this limit quite higher than layer cache limit
*/
#define D2D_GRADIENT_STOP_COLLECTION_CACHE_MAX 50

/** when gradient stop collection cache is full, 
	internal cache will free the D2D_GRADIENT_STOP_COLLECTION_CACHE_FREE_COUNT oldest textures
	in the cache to allocate space for new gradient stop collections */
#define D2D_GRADIENT_STOP_COLLECTION_CACHE_FREE_COUNT 5

/** only gradient stop collections with stop count <= D2D_GRADIENT_STOP_COLLECTION_CACHE_MAX_STOP_COUNT will be cached 
	(in order to limit hash ID size)
*/
#define D2D_GRADIENT_STOP_COLLECTION_CACHE_MAX_STOP_COUNT 6


#define D2D_LAYERCLIP_DUMMY 1000.0f

/** layer/clipping element 
@remarks
	in Direct2D, layers & clipping are tightly bound together
*/
class VLayerClipElem : VObject
{
public:
	/** axis-aligned rect clipping or layer */
	VLayerClipElem( const VAffineTransform& inCTM, const VRect& inBounds, const VRegion* inRegion, const VRect& inPrevClipBounds, const VRegion *inPrevClipRegion,
					ID2D1Layer* inLayer, ID2D1Geometry* inGeom, const GReal inOpacity, 
					bool inHasClipOverride);
	/** offscreen layer */
	VLayerClipElem( const VAffineTransform& inCTM, const VRect& inBounds, const VRect& inPrevClipBounds, const VRegion *inPrevClipRegion,
					VImageOffScreen* inOffScreen, const GReal inOpacity, 
					bool inHasClipOverride, bool inOwnOffScreen = true);

	/** dummy constructor (used by SaveClip()) */
	VLayerClipElem( const VRect& inBounds, const VRect& inPrevClipBounds, const VRegion *inPrevClipRegion, bool inHasClipOverride);

	virtual ~VLayerClipElem();

	/** copy constructor */
	VLayerClipElem( const VLayerClipElem& inSource);

	/** transform accessor */
	const VAffineTransform& GetTransform() const { return fCTM; }

	/** layer accessor */
	ID2D1Layer *GetLayer() const { return fLayer; }

	/** retain layer */
	ID2D1Layer *RetainLayer() const 
	{ 
		ID2D1Layer *layer = fLayer; 
		layer->AddRef();
		return layer;
	}

	/** offscreen image accessor */
	VImageOffScreen *GetImageOffScreen() const { return fImageOffScreen; }

	/** retain offscreen image */
	VImageOffScreen *RetainImageOffScreen() const 
	{ 
		fImageOffScreen->Retain();
		return fImageOffScreen;
	}

	/** true if offscreen is owned by graphic context */
	bool OwnOffScreen() const { return fOwnOffScreen; }

	/** geom accessor */
	ID2D1Geometry *GetGeometry() const { return fGeom; }
		
	/** region accessor 
	@remarks
		transformed region
	*/
	const VRegion *GetRegion() const { return fRegion; }

	/** opacity accessor */
	GReal GetOpacity() const { return fOpacity; }

	/** true for layer */
	bool IsLayer() const { return fLayer != NULL; }

	/** true for bitmap render target */
	bool IsImageOffScreen() const { return fImageOffScreen != NULL; }

	/** true is it is dummy layer */
	bool IsDummy() const { return fOpacity == D2D_LAYERCLIP_DUMMY; }

	/** get transformed bounds (ie with CTM = identity) */
	const VRect& GetBounds() const { return fBounds; }

	/** get parent clipping transformed bounds */
	const VRect& GetPrevClipBounds() const { return fPrevClipBounds; }

	/** get parent transformed region */
	const VRegion* GetPrevClipRegion() const { return fPrevClipRegion; }

	/** get parent transformed region & take ownership */
	VRegion* GetOwnedPrevClipRegion() const
	{ 
		VRegion *region = fPrevClipRegion;
		fPrevClipRegion = NULL;
		return region; 
	}

	bool HasClipOverride() const { return fHasClipOverrideBackup; }
	void SetHasClipOverride( bool inHasClipOverride) { fHasClipOverrideBackup = inHasClipOverride; }

	VLayerClipElem&	operator = (const VLayerClipElem& inClip);
	Boolean		operator == (const VLayerClipElem& inClip) const;

protected:
	VAffineTransform fCTM;
	CComPtr<ID2D1Layer> fLayer;
	VImageOffScreen* fImageOffScreen;
	CComPtr<ID2D1Geometry> fGeom;
	GReal fOpacity;

	/** transformed bounds (ie with CTM = identity) */
	VRect fBounds; 

	/** parent clipping transformed bounds */
	VRect fPrevClipBounds;

	/** transformed region */
	VRegion *fRegion;

	/** parent clipping transformed region */
	mutable VRegion *fPrevClipRegion;
	
	bool fHasClipOverrideBackup;
	/** true if offscreen is owned by graphic context */
	bool fOwnOffScreen;
};

/** clipping stack type definition */
typedef std::vector<VLayerClipElem> VStackLayerClip;


class VWinD2DGraphicContext;

/** D2D drawing state class
@remarks
	this class stores D2D brushes, fonts & drawing state
*/
class VD2DDrawingState
{
public:
	/** build state from passed gc */
	VD2DDrawingState(const VWinD2DGraphicContext *inGC);
	virtual ~VD2DDrawingState();

	/** copy constructor */
	VD2DDrawingState( const VD2DDrawingState& inSource);

	/** restore state to passed gc */
	void Restore(const VWinD2DGraphicContext *inGC);

private:
	CComPtr<ID2D1DrawingStateBlock> fDrawingStateNative;

	bool		fMaxPerfFlag;
	bool		fIsHighQualityAntialiased;
	TextRenderingMode fHighQualityTextRenderingMode;
	FillRuleType fFillRule;
	GReal		fCharKerning; 
	GReal		fCharSpacing; 
	bool		fUseShapeBrushForText;
	bool		fShapeCrispEdgesEnabled;
	bool		fAllowCrispEdgesOnPath;
	bool		fAllowCrispEdgesOnBezier;

	CComPtr<ID2D1Brush>		fStrokeBrush;
	CComPtr<ID2D1StrokeStyle> fStrokeStyle;
	D2D1_STROKE_STYLE_PROPERTIES fStrokeStyleProps;
	VLineDashPattern fStrokeDashPatternInit;
	GReal					fStrokeWidth;
	bool					fIsStrokeDotPattern;
	bool					fIsStrokeDashPatternUnitEqualToStrokeWidth;

	CComPtr<ID2D1Brush>		fFillBrush;
	CComPtr<ID2D1Brush>		fTextBrush;

	CComPtr<ID2D1SolidColorBrush>		fStrokeBrushSolid;
	CComPtr<ID2D1SolidColorBrush>		fFillBrushSolid;
	CComPtr<ID2D1SolidColorBrush>		fTextBrushSolid;

	CComPtr<ID2D1LinearGradientBrush>	fStrokeBrushLinearGradient;
	CComPtr<ID2D1RadialGradientBrush>	fStrokeBrushRadialGradient;

	CComPtr<ID2D1LinearGradientBrush>	fFillBrushLinearGradient;
	CComPtr<ID2D1RadialGradientBrush>	fFillBrushRadialGradient;

	VFont			*fTextFont;
	VFontMetrics	*fTextFontMetrics;
	GReal			 fWhiteSpaceWidth;
	D2D1_RECT_F		 fTextRect;

	DrawingMode			fDrawingMode;
	TextRenderingMode	fTextRenderingMode;
	TextMeasuringMode	fTextMeasuringMode;

	GReal fGlobalAlpha;
	GReal fShadowHOffset;
	GReal fShadowVOffset;
	GReal fShadowBlurness;

	VPoint	fBrushPos;
};

typedef std::vector<VD2DDrawingState> VectorOfDrawingState;

/** D2D drawing context
@remarks
	this class is used to save/restore D2D context
	(this class save/restore brushes & font too because Direct2D does not save/restore it) 
*/
class VD2DDrawingContext : public VObject
{
public:
	VD2DDrawingContext():VObject() {}
	virtual ~VD2DDrawingContext() {}

	/** save drawing state from passed context */
	void Save(const VWinD2DGraphicContext *inGC);

	/** restore drawing state to passed context */
	void Restore(const VWinD2DGraphicContext *inGC);

	/** clear */
	void Clear()
	{
		fDrawingState.clear();
	}

	bool IsEmpty() const 
	{
		return fDrawingState.empty();
	}
private:
	VectorOfDrawingState fDrawingState;
};


class XTOOLBOX_API VWinD2DGraphicContext : public VGraphicContext
{
	friend class VD2DDrawingState;
	friend class VD2DDrawingContext;
public:
	/** derived from IRefCountable */
	virtual	sLONG		Release(const char* DebugInfo = 0) const;

	// Class initialization
	/** initialize D2D impl
		
	@param inUseHardwareIfPresent
		true: use hardware if present or if has sufficient video memory otherwise use software
			   (recommended setting for client - optimal speed)
		false: use software only (recommended setting for server - best stability)
	*/
	
	static Boolean	Init ( bool inUseHardwareIfPresent = true);
	static void 	DeInit ();

	static void	DisposeSharedResources(int inHardware = D2D_CACHE_RESOURCE_HARDWARE);

	static void		DesktopCompositionModeChanged();
	static bool		IsDesktopCompositionEnabled()
	{
		return sDesktopCompositionEnabled;
	}

	/** return true if Direct2D is available on the platform */
	static bool		IsAvailable()
	{
		return sD2DAvailable;
	}

	/** return true if Direct2D is enabled */ 
	static bool		IsEnabled() 
	{
		return sD2DEnabled;
	}

	/** return true if Direct2D should use hardware & software only if hardware is not available
		return false if Direct2D should always use software
	*/
	static bool		UseHardwareIfPresent()
	{
		return sUseHardwareIfPresent;
	}

	static void EnableHardware( bool inEnable)
	{
		if (!sUseHardwareIfPresent)
		{
			sHardwareEnabled = false;
			return;
		}
		if (sHardwareEnabled == inEnable)
			return;
		sHardwareEnabled = inEnable;
		
		//if hardware status has changed, reset all shared resources to ensure unused system/video memory is freed
		DisposeSharedResources( D2D_CACHE_RESOURCE_SOFTWARE);
		DisposeSharedResources( D2D_CACHE_RESOURCE_HARDWARE);
	}

	static bool IsHardwareEnabled() 
	{
		return sUseHardwareIfPresent && sHardwareEnabled;
	}

	/** set to true to force this graphic context to use software only
	@remarks
		if actual graphic context is hardware, 
		internal resources will be rebuilded 

		you should call this method outside BeginUsingContext/EndUsingContext
		otherwise method will do nothing
	@note
		actually this method is mandatory only for Direct2D impl
	*/
	void SetSoftwareOnly( bool inSoftwareOnly)
	{
		if (!testAssert(fUseCount <= 0))
			return;

		if (fIsRTSoftwareOnly == inSoftwareOnly)
			return;

		fIsRTSoftwareOnly = inSoftwareOnly;
		if (fRenderTarget)
		{
			//reset render target if appropriate
			if (fIsRTSoftwareOnly)
			{
				if (fIsRTHardware)
					_DisposeDeviceDependentObjects();
			}
			else
			{
				if (!fIsRTHardware && IsHardwareEnabled())
					_DisposeDeviceDependentObjects();
			}
		}
	}

	/** return true if this graphic context should use software only, false otherwise */
	bool IsSoftwareOnly() const { return fIsRTSoftwareOnly; }

	/** return true if graphic context uses hardware resources */
	bool IsHardware() const { return fRenderTarget != NULL && fIsRTHardware != FALSE; }

	/** set to true to speed up GDI over D2D context rendering 
	@remarks
		that mode optimizes GDI over D2D context rendering so that GDI over D2D context is preserved much longer
		but it means that some rendering is done by GDI but D2D 
		if shape rendering in both contexts is similar: 
		for instance pictures, simple shapes like lines & rect without gradient pattern, user dashes and opacity < 1
		might be rendered with GDI to preserve longer GDI context; 
		D2D is still used to render shapes which are not renderable with GDI

		note that clearing that mode forces a GDI flush if fast GDI was previously enabled

		for instance, you should enable it if you intend to render a set of primitives including legacy texts and optionally pictures and simple lines & rects
		in order to use only a single GDI over D2D context to render the set of primitives
		(because switching from D2D to GDI and GDI to D2D has a high perf hit)

		that mode is disabled on default: on default only legacy texts are rendered with GDI
		that mode does nothing in context but D2D
	*/
	virtual	void EnableFastGDIOverD2D( bool inEnable = true);
	virtual bool IsEnabledFastGDIOverD2D() const { return fGDI_Fast; }

	/** return true if font glyphs are aligned to pixel boundary */
	virtual	bool IsFontPixelSnappingEnabled() const { return (GetTextRenderingMode() & TRM_LEGACY_ON) != 0; }

	/** create a shared graphic context binded initially to the specified native context ref 
	@param inUserHandle
		user handle used to identify the shared graphic context
	@param inContextRef
		native context ref
		CAUTION: on Windows, Direct2D cannot draw directly to a printer HDC so while printing & if D2DIsEnabled()
				 you should either call CreateBitmapGraphicContext to draw in a bitmap graphic context
				 & then print the bitmap with DrawBitmapToHDC or use explicitly another graphic context like GDIPlus or GDI
				 (use the desired graphic context constructor for that)
	@param inBounds (used only by D2D impl - see VWinD2DGraphicContext)
		context area bounds 
	@param inPageScale (used only by D2D impl - see remarks below)
		desired page scaling 
		(default is 1.0f)
	@remarks
		if Direct2D is not available, method will behave the same than VGraphicContext::Create (param inUserHandle is ignored)
		so this method is mandatory only for Direct2D impl (but can be used safely for other impls so you can use same code in any platform)

		this method should be used if you want to re-use the same Direct2D graphic context with different GDI contexts:
		it creates a single Direct2D graphic context mapped to the passed user handle:
		- the first time you call this method, it allocates the graphic context, bind it to the input HDC & store it to a shared internal table (indexed by the user handle)
		- if you call this method again with the same user handle & with a different HDC or bounds, it rebinds the render target to the new HDC & bounds
		  which is more faster than to recreate the render target especially if new bounds are less or equal to the actual bounds

		caution: to release the returned graphic context, call as usual Release method of the graphic context
		
		if you want to remove the returned graphic context from the shared internal table too, call also RemoveShared 
		(actually you do not need to call this method because shared graphic contexts are auto-released by VGraphicContext::DeInit
		 & when needed for allocating video memory)

		For instance, in 4D this method should be called preferably than the previous one in order to avoid to create render target at each frame;

		if IsD2DImpl() return true, 

			all coordinates are expressed in device independent units (DIP) 
			where 1 DIP = 1 pixel if render target DPI = 96 so in order to avoid DPI scaling from 96 to DPI
			we force DPI equal to 96 by default (so 1 DIP = 1 Pixel by default).
			(It is because on Windows, 4D app is not dpi-aware and we must keep 
			this limitation for backward compatibility with previous impl)

			render target DPI is computed from inPageScale using the following formula:
			render target DPI = 96.0f*inPageScale 
			(yes render target DPI is a floating point value)

			bounds are expressed in DIP (so with DPI = 96): 
			D2D internally manages page scaling by scaling bounds & coordinates by DPI/96; 
	*/
	static VGraphicContext* CreateShared(sLONG inUserHandle, ContextRef inContextRef, const VRect& inBounds, const GReal inPageScale = 1.0f, const bool inTransparent = true, const bool inSoftwareOnly = false);

	/** release & remove the graphic context binded to the specified user handle from the internal global table	
	@see
		CreateShared
	*/
	static void RemoveShared( sLONG inUserHandle);

	/** create ID2D1DCRenderTarget render target binded to the specified HDC 
	@param inHDC
		gdi device context ref
	@param inBounds
		bounds (DIP unit: coordinate space equal to gdi coordinate space - see remarks below)
	@param inPageScale
		desired page scaling for render target (see remarks below)
		(default is 1.0f)
	@remarks
		caller can call BindHDC later to bind or rebind new dc 

		all coordinates are expressed in device independent units (DIP) 
		where 1 DIP = 1 pixel if render target DPI = 96 so in order to avoid DPI scaling from 96 to DPI
		we force DPI equal to 96 by default (so 1 DIP = 1 Pixel by default).
		(It is because on Windows, 4D app is not dpi-aware and we must keep 
		this limitation for backward compatibility with previous impl)

		render target DPI is computed from inPageScale using the following formula:
		render target DPI = 96.0f*inPageScale 
		(yes render target DPI is a floating point value)

		bounds are expressed in DIP (so with DPI = 96): 
		D2D internally manages page scaling by scaling bounds & coordinates by DPI/96; 
		while printing, just pass bounds like rendering on screen (gdi bounds)
		and set inPageScale equal to printer DPI/72 so that render target DPI = 96.0f*printer DPI/72
		(see usage in SVG component for an example)
	*/
	VWinD2DGraphicContext (HDC inHDC, const VRect& inBounds, const GReal inPageScale = 1.0f, const bool inTransparent = true);

	/** bind new HDC to the current render target
	@remarks
		do nothing and return false if current render target is not a ID2D1DCRenderTarget
	*/
	virtual bool BindHDC( HDC inHDC, const VRect& inBounds, bool inResetContext = true, bool inBindAlways = false) const;

	/** create ID2D1HwndRenderTarget render target binded to the specified HWND 
	@remarks
		ID2D1HwndRenderTarget render target is natively double-buffered so
		VWinD2DGraphicContext should be created at window creation 
		and destroyed only when window is destroyed 

		all coordinates are expressed in device independent units (DIP) 
		where 1 DIP = 1 pixel if render target DPI = 96 so in order to avoid DPI scaling from 96 to DPI
		we force DPI equal to 96 by default (so 1 DIP = 1 Pixel by default).
		(It is because on Windows, 4D app is not dpi-aware and we must keep 
		this limitation for backward compatibility with previous impl)

		render target DPI is computed from inPageScale using the following formula:
		render target DPI = 96.0f*inPageScale 
		(yes render target DPI is a floating point value)

		caller should call WndResize if window is resized
		(otherwise render target content is scaled to fit in window bounds)
	*/
	VWinD2DGraphicContext (HWND inHWND, const GReal inPageScale = 1.0f, const bool inTransparent = true);

	/** resize window render target 
	@remarks
		should be called whenever window is resized to prevent render target scaling

		do nothing if current render target is not a ID2D1HwndRenderTarget
	*/
	virtual void WndResize();

	/** build from input render target */
	VWinD2DGraphicContext (ID2D1RenderTarget* inRT, const VPoint& inOrigViewport = VPoint(0.0f,0.0f), uBOOL inOwnRT = true, const bool inTransparent = true);

	/** create WIC render target using provided WIC bitmap as source
	@remarks
		WIC Bitmap is retained until graphic context destruction

		inOrigViewport is expressed in DIP

		this render target is useful for software offscreen rendering
		(as it does not use hardware, render target content can never be lost: it can be used for permanent offscreen rendering)
	*/
	VWinD2DGraphicContext( IWICBitmap *inWICBitmap, const GReal inPageScale = 1.0f, const VPoint& inOrigViewport = VPoint(0.0f,0.0f));

	/** create WIC render target from scratch
	@remarks
		create a WIC render target bound to a internal WIC Bitmap using GUID_WICPixelFormat32bppPBGRA as pixel format
		(in order to be compatible with ID2D1Bitmap)

		width and height are expressed in DIP 
		(so bitmap size in pixel will be width*inPageScale x height*inPageScale)

		inOrigViewport is expressed in DIP too

		this render target is useful for software offscreen rendering
		(as it does not use hardware, render target content can never be lost: it can be used for permanent offscreen rendering)
	*/
	VWinD2DGraphicContext( sLONG inWidth, sLONG inHeight, const GReal inPageScale = 1.0f, const VPoint& inOrigViewport = VPoint(0.0f,0.0f), const bool inTransparent = true, const bool inTransparentCompatibleGDI = true);

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
	Gdiplus::Bitmap *GetGdiplusBitmap() const;


	/** create a D2D bitmap compatible with the specified render target from the current render target content
	@remarks
		call this method outside BeginUsingContext/EndUsingContext
		otherwise this method will fail and return NULL

		with this method, you can draw content from this render target to another one

		if current render target is a WIC bitmap render target, D2D bitmap is build directly from the internal WIC bitmap
	*/
	ID2D1Bitmap *CreateBitmap(ID2D1RenderTarget* inRT, const VRect *inBoundsTransformed = NULL)
	{
		return _CreateBitmap( inRT, inBoundsTransformed);
	}
	ID2D1Bitmap *_CreateBitmap(ID2D1RenderTarget* inRT, const VRect *inBoundsTransformed = NULL) const;


	/** draw the attached bitmap to a GDI device context 
	@remarks
		D2D does not support drawing directly to a printer HDC
		so for printing, you sould render first to a bitmap graphic context 
		and then call this method to draw bitmap to the printer HDC

		do nothing if current graphic context is not a bitmap graphic context
	*/
	void		DrawBitmapToHDC( HDC hdc, const VRect& inDestBounds, const Real inPageScale = 1.0f, const VRect* inSrcBounds = NULL);

	explicit	VWinD2DGraphicContext (const VWinD2DGraphicContext& inOriginal);
	virtual		~VWinD2DGraphicContext ();
	

	virtual		GReal GetDpiX() const;
	virtual		GReal GetDpiY() const;

	virtual bool IsD2DImpl() const { return true; }

	/** return true if render target is transparent
	@remarks
		ClearType can only be used on opaque render target
	*/
	bool		IsTransparent() const
	{
		return fIsTransparent;
	}

	/** change transparency status
	@remarks
		render target can be rebuilded if transparency status is changed
	*/
	void SetTransparent( bool inTransparent);

	/** set lock type 
	@remarks
		this method is mandatory only for Direct2D context: for all other contexts it does nothing.

		if context is Direct2D:

		if true, _Lock(), _GetHDC() and BeginUsingParentContext() will always lock & return parent context by default  
				ie the context in which is presented the render target (to which the render target is binded)
		if false (default), _Lock(), _GetHDC() and BeginUsingParentContext() will lock & return a GDI-compatible context created from the render target content
				if called between BeginUsingContext & EndUsingContext, otherwise will lock or return the context binded to the render target

		caution: please do NOT call between _GetHDC()/_ReleaseHDC() or BeginUsingParentContext/EndUsingParentContext

	@note
		locking the parent context has a very high performance cost due to the drawing context flushing & render target blitting
		so by default (false) we lock a GDI-compatible context over the render target if between BeginUsingContext/EndUsingContext
	*/
	void SetLockParentContext( bool inLockParentContext)
	{
		xbox_assert( fHDCOpenCount <= 0);
#if D2D_RENDER_TARGET_USE_GDI_COMPATIBLE_DC
		fLockParentContext = inLockParentContext;
#else
		fLockParentContext = true;
#endif
	}
	bool GetLockParentContext() const { return fLockParentContext; }
	
	/** enable/disable Direct2D implementation at runtime
	@remarks
		if set to false, IsD2DAvailable() will return false even if D2D is available on the platform
		if set to true, IsD2DAvailable() will return true if D2D is available on the platform
	*/
	static void D2DEnable( bool inD2DEnable = true);


	/** accessor on D2D factory */
	static const CComPtr<ID2D1Factory>& GetFactory() 
	{
#if D2D_FACTORY_MULTI_THREADED
		return fFactory;
#else
		return fFactory[VTask::GetCurrentID()];
#endif
	}

	/** accessor on D2D factory mutex */
	static VCriticalSection& GetMutexFactory()
	{
		return fMutexFactory;
	}

	/** accessor on DWrite factory */
	static const CComPtr<IDWriteFactory>& GetDWriteFactory() 
	{
		return fDWriteFactory;
	}

	/** accessor on DWrite factory mutex */
	static VCriticalSection& GetMutexDWriteFactory()
	{
		return fMutexDWriteFactory;
	}

#if !D2D_FACTORY_MULTI_THREADED
	static bool		_CreateSingleThreadedFactory();
#endif

	/** create or retain D2D cached bitmap mapped with the Gdiplus bitmap 
	@remarks
		this will create or retain a shared D2D bitmap using the frame 0 of the Gdiplus bitmap
	*/
	ID2D1Bitmap *CreateOrRetainBitmap( Gdiplus::Bitmap *inBmp, const VColor *inTransparentColor = NULL);

	/** release cached D2D bitmap associated with the specified Gdiplus bitmap */
	static void ReleaseBitmap( const Gdiplus::Bitmap *inBmp);
	
	// Pen management
	virtual void	SetFillColor (const VColor& inColor, VBrushFlags* ioFlags = NULL) { _SetFillColor( inColor, ioFlags); }
			void	_SetFillColor (const VColor& inColor, VBrushFlags* ioFlags = NULL) const;

	virtual void	SetFillPattern (const VPattern* inPattern, VBrushFlags* ioFlags = NULL) { _SetFillPattern( inPattern, ioFlags); }
			void	_SetFillPattern (const VPattern* inPattern, VBrushFlags* ioFlags = NULL) const;

	virtual void	SetLineColor (const VColor& inColor, VBrushFlags* ioFlags = NULL) { _SetLineColor( inColor, ioFlags); }
			void	_SetLineColor (const VColor& inColor, VBrushFlags* ioFlags = NULL) const;

	virtual void	SetLinePattern (const VPattern* inPattern, VBrushFlags* ioFlags = NULL) { _SetLinePattern( inPattern, ioFlags); }
			void	_SetLinePattern (const VPattern* inPattern, VBrushFlags* ioFlags = NULL) const;

	/** set line dash pattern
	@param inDashOffset
		offset into the dash pattern to start the line (user units)
	@param inDashPattern
		dash pattern (alternative paint and unpaint lengths in user units)
		for instance {3,2,4} will paint line on 3 user units, unpaint on 2 user units
							 and paint again on 4 user units and so on...
	*/
	virtual void	SetLineDashPattern(GReal inDashOffset, const VLineDashPattern& inDashPattern, VBrushFlags* ioFlags = NULL) { _SetLineDashPattern( inDashOffset, inDashPattern, ioFlags); }
			void	_SetLineDashPattern(GReal inDashOffset, const VLineDashPattern& inDashPattern, VBrushFlags* ioFlags = NULL) const;

			/** force dash pattern unit to be equal to stroke width (on default it is equal to user unit) */
	virtual void	ShouldLineDashPatternUnitEqualToLineWidth( bool inValue);
	virtual bool	ShouldLineDashPatternUnitEqualToLineWidth() const { return fIsStrokeDashPatternUnitEqualToStrokeWidth; }

	/** set fill rule */
	virtual void	SetFillRule( FillRuleType inFillRule);

	virtual void	SetLineWidth (GReal inWidth, VBrushFlags* ioFlags = NULL) { _SetLineWidth( inWidth, ioFlags); }
			void	_SetLineWidth (GReal inWidth, VBrushFlags* ioFlags = NULL) const;
	virtual GReal	GetLineWidth () const;

	virtual void	SetLineStyle (CapStyle inCapStyle, JoinStyle inJoinStyle, VBrushFlags* ioFlags = NULL);
	virtual void	SetLineCap (CapStyle inCapStyle, VBrushFlags* ioFlags = NULL);
	virtual void	SetLineJoin(JoinStyle inJoinStyle, VBrushFlags* ioFlags = NULL); 
	/** set line miter limit */
	virtual void	SetLineMiterLimit( GReal inMiterLimit = (GReal) 0.0);
	virtual void	SetTextColor (const VColor& inColor, VBrushFlags* ioFlags = NULL) { _SetTextColor( inColor, ioFlags); }
			void	_SetTextColor (const VColor& inColor, VBrushFlags* ioFlags = NULL) const;
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
	virtual void	SetFont ( VFont *inFont, GReal inScale = (GReal) 1.0) { _SetFont( inFont, inScale); }
			void	_SetFont ( VFont *inFont, GReal inScale = (GReal) 1.0) const;
	virtual VFont*	RetainFont ();
	virtual void	SetTextPosBy (GReal inHoriz, GReal inVert);
	virtual void	SetTextPosTo (const VPoint& inHwndPos);
	virtual void	SetTextAngle (GReal inAngle);
	virtual void	GetTextPos (VPoint& outHwndPos) const;

	virtual void	SetSpaceKerning (GReal inRatio);
	virtual void	SetCharKerning (GReal inRatio);
	
	virtual DrawingMode	SetTextDrawingMode (DrawingMode inMode, VBrushFlags* ioFlags = NULL);
	
	virtual void GetTransform(VAffineTransform &outTransform);
	virtual void _GetTransform(VAffineTransform &outTransform) const;
	virtual void SetTransform(const VAffineTransform &inTransform);
	virtual void _SetTransform(const VAffineTransform &inTransform) const;
	virtual void ConcatTransform(const VAffineTransform &inTransform);
	virtual void RevertTransform(const VAffineTransform &inTransform);
	virtual void ResetTransform();
	
	// Pixel management
	virtual uBYTE	SetAlphaBlend (uBYTE inAlphaBlend);
	virtual void	SetPixelBackColor (const VColor& inColor);
	virtual void	SetPixelForeColor (const VColor& inColor);
	
	// Graphic context storage
	typedef enum eDeferredRestoreType
	{
		eSave_Clipping,
		eSave_Context,
		ePush_Clipping
	} eDeferredRestoreType;

	virtual void	NormalizeContext ();
	virtual void	SaveContext () { _SaveContext(); }
	virtual void	RestoreContext () { _RestoreContext(); }
		
	void			_SaveContext () const;
	void			_RestoreContext() const;

	// Create a shadow layer graphic context
	// with specified bounds and transparency
	void BeginLayerShadow(const VRect& inBounds, const VPoint& inShadowOffset = VPoint(8,8), GReal inShadowBlurDeviation = (GReal) 1.0, const VColor& inShadowColor = VColor(0,0,0,255), GReal inAlpha = (GReal) 1.0);
	
	// Graphic layer management
	
	// Create a filter or alpha layer graphic context
	// with specified bounds and filter
	// @remark : retain the specified filter until EndLayer is called
protected:
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
	
	/** return index of current layer 
	@remarks
		index is zero-based
		return -1 if there is no layer
	*/
	virtual sLONG	GetIndexCurrentLayer() const;

    /** return true if current layer is a valid bitmap layer (otherwise might be a null layer - for clipping only - or shadow or transparency layer) */
	virtual	bool	IsCurrentLayerOffScreen() const;

	// clear all layers and restore main graphic context
	virtual void	ClearLayers();

	virtual bool	ShouldDrawTextOnTransparentLayer() const;


	/** return current clipping bounding rectangle 
	 @remarks
	 bounding rectangle is expressed in VGraphicContext normalized coordinate space 
	 (ie coordinate space with y axis pointing down - compliant with gdiplus and quickdraw coordinate spaces)
	 */  
	virtual void	GetClipBoundingBox( VRect& outBounds) { _GetClipBoundingBox( outBounds); }
	
	void	_GetClipBoundingBox( VRect& outBounds) const;

	/** return user space to main graphic context space transformation matrix
	@remarks
		unlike GetTransform which return current graphic context transformation 
		- which can be the current layer transformation if called between BeginLayer() and EndLayer() - 
		this method return current user space to the main graphic context space transformation
		taking account all intermediate layer transformations if any
		(of course useful only if you need to map user space to/from main graphic context space 
		 - main graphic context is the first one created or set with this class constructor)
	*/
	void	_GetTransformToScreen(VAffineTransform &outTransform) const
	{
		_GetTransformToLayer( outTransform, -1);
		if (fUseCount > 0 && (fHDCBounds.GetX() != 0.0f || fHDCBounds.GetY() != 0.0f))
			outTransform.Translate( fHDCBounds.GetX(), fHDCBounds.GetY(), VAffineTransform::MatrixOrderAppend);
	}
	virtual void	GetTransformToScreen(VAffineTransform &outTransform) 	{ _GetTransformToScreen( outTransform); }

	virtual void	GetTransformToLayer(VAffineTransform &outTransform, sLONG inIndexLayer)
	{
		_GetTransformToLayer(outTransform, inIndexLayer);
	}
	void	_GetTransformToLayer(VAffineTransform &outTransform, sLONG inIndexLayer) const;

	//return current graphic context native reference
	virtual VGCNativeRef GetNativeRef() const
	{
		return (VGCNativeRef)((ID2D1RenderTarget *)fRenderTarget);
	}
	
	// Text measurement

	virtual GReal	GetCharWidth (UniChar inChar) const;
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
protected:	
	IDWriteTextLayout *_GetTextBoundsTypographic( const VString& inString, VRect& oRect, TextLayoutMode inLayoutMode = TLM_NORMAL) const;
public:

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

	virtual void	DrawStyledText( const VString& inText, VTreeTextStyle *inStyles, AlignStyle inHoriz, AlignStyle inVert, const VRect& inHwndBounds, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f);
	
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

	//return styled text box bounds
	//@remark:
	// if input bounds width is not null, use it as max line wrapping width (or max text height in vertical layout mode)
	// if input bounds height is not null, use it as max text height (or max line wrapping width in vertical layout mode)
	virtual void	_GetStyledTextBoxBounds( const VString& inText, VTreeTextStyle *inStyles, AlignStyle inHAlign, AlignStyle inVAlign, TextLayoutMode inLayoutMode, VRect& ioBounds, IDWriteTextLayout **outLayout = NULL, const GReal inRefDocDPI = 72.0f, bool inEnableLegacy = true, bool inReturnLayoutDesignBounds = false, void *inTextMetrics = NULL, void *inTextOverhangMetrics = NULL, bool inForceMonoStyle = false);

	/** extract text lines & styles from a wrappable text using the specified passed max width
	@remarks 
		layout mode is assumed to be equal to default = TLM_NORMAL
	*/
	virtual void	GetTextBoxLines( const VString& inText, const GReal inMaxWidth, std::vector<VString>& outTextLines, VTreeTextStyle *inStyles = NULL, std::vector<VTreeTextStyle *> *outTextLinesStyles = NULL, const GReal inRefDocDPI = 72.0f, const bool inNoBreakWord = true, bool *outLinesOverflow = NULL);


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
		Direct2D impl: inBounds can be NULL (in that case the render target content is cleared)
	*/
	virtual void	Clear( const VColor& inColor = VColor(0,0,0,0), const VRect *inBounds = NULL, bool inBlendCopy = true);

	void	_ClipRect(const VRect& inHwndRect, Boolean inAddToPrevious = false, Boolean inIntersectToPrevious = true) const;

	// Clipping
	virtual void	ClipRect (const VRect& inHwndRect, Boolean inAddToPrevious = false, Boolean inIntersectToPrevious = true)
	{
		_ClipRect(inHwndRect, inAddToPrevious, inIntersectToPrevious);
	}


	/** add or intersect the specified clipping path with the current clipping path */
	virtual void	ClipPath (const VGraphicPath& inPath, Boolean inAddToPrevious = false, Boolean inIntersectToPrevious = true)
	{
		_ClipPath( inPath, NULL, inAddToPrevious, inIntersectToPrevious);
	}

	/** add or intersect the specified clipping path with the current clipping path */
	void	_ClipPath2(const VGraphicPath& inPath, Boolean inAddToPrevious = false, Boolean inIntersectToPrevious = true) const
	{
		_ClipPath( inPath, NULL, inAddToPrevious, inIntersectToPrevious);
	}

protected:
	/** add or intersect the specified clipping path with the current clipping path */
	virtual void	_ClipPath (const VGraphicPath& inPath, const VRegion *inRegion, Boolean inAddToPrevious = false, Boolean inIntersectToPrevious = true) const;
public:

	/** add or intersect the specified clipping path outline with the current clipping path 
	@remarks
		this will effectively create a clipping path from the path outline 
		(using current stroke brush) and merge it with current clipping path,
		making possible to constraint drawing to the path outline
	*/
	virtual void	ClipPathOutline (const VGraphicPath& inPath, Boolean inAddToPrevious = false, Boolean inIntersectToPrevious = true);

	virtual void	ClipRegion (const VRegion& inHwndRgn, Boolean inAddToPrevious = false, Boolean inIntersectToPrevious = true);
	virtual void	GetClip (VRegion& outHwndRgn) const;

	static void		GetClip (HDC inHDC, VRegion& outHwndRgn, bool inApplyOffsetViewport = true);
	static void		GetClip (HWND inHWND, VRegion& outHwndRgn, bool inApplyOffsetViewport = true);
	static void		GetClipBoundingBox (HDC inHDC, VRect& outBounds, bool inApplyOffsetViewport = true);
	static void		GetClipBoundingBox (HWND inHWND, VRect& outBounds, bool inApplyOffsetViewport = true);


	virtual void	SaveClip () 
	{
		if (fGDI_HDC)
		{
			fGDI_GC->SaveClip();
			fGDI_SaveClipCounter++;
			fGDI_LastPushedClipCounter.push_back( fGDI_PushedClipCounter);
			fGDI_PushedClipCounter = 0;
			fGDI_Restore.push_back( eSave_Clipping);
		}
		else
			_SaveClip();
		return;
	}

	virtual void	RestoreClip () 
	{ 
		if (fGDI_HDC)
		{
			if (fGDI_SaveClipCounter > 0)
			{
				while (fGDI_PushedClipCounter)
				{
					xbox_assert(fGDI_Restore.back() == ePush_Clipping);
					fGDI_ClipRect.pop_back();
					fGDI_Restore.pop_back();
					fGDI_PushedClipCounter--;
				}
				xbox_assert(fGDI_Restore.back() == eSave_Clipping);
				fGDI_Restore.pop_back();
				fGDI_PushedClipCounter = fGDI_LastPushedClipCounter.back();
				fGDI_LastPushedClipCounter.pop_back();
				fGDI_SaveClipCounter--;

				fGDI_GC->RestoreClip();
				return;
			}
			else 
				FlushGDI();
		}
		VImageOffScreenRef offscreen = _RestoreClip();
		if (offscreen)
			offscreen->Release();
	}

	virtual void	_SaveClip () const;
	virtual VImageOffScreenRef	_RestoreClip () const;

	/** push initial clipping */
	void			PushClipInit() const;

	// Content bliting primitives
	virtual void	CopyContentTo (VGraphicContext* inDestContext, const VRect* inSrcBounds = NULL, const VRect* inDestBounds = NULL, TransferMode inMode = TM_COPY, const VRegion* inMask = NULL)
	{
		_CopyContentTo (inDestContext, inSrcBounds, inDestBounds, inMode, inMask);
	}
	virtual void	_CopyContentTo (VGraphicContext* inDestContext, const VRect* inSrcBounds = NULL, const VRect* inDestBounds = NULL, TransferMode inMode = TM_COPY, const VRegion* inMask = NULL) const;
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

	/** flush actual GDI context (created from render target) */
	void			FlushGDI() const;

	virtual	ContextRef	BeginUsingParentContext()const;
	virtual	void	EndUsingParentContext(ContextRef inContextRef)const;

	virtual	PortRef	BeginUsingParentPort()const{return BeginUsingParentContext();};
	virtual	void	EndUsingParentPort(PortRef inPortRef)const{EndUsingParentContext(inPortRef);};


	virtual	PortRef		GetParentPort () const { return BeginUsingParentContext(); };
	virtual	ContextRef	GetParentContext () const { return BeginUsingParentContext(); };

	virtual	void	ReleaseParentPort (PortRef inPortRef) const {return EndUsingParentContext(inPortRef);};
	virtual	void	ReleaseParentContext (ContextRef inContextRef) const {return EndUsingParentContext(inContextRef);};

	// Utilities
	virtual	Boolean	UseEuclideanAxis () { return false; };
	virtual	Boolean	UseReversedAxis () { return true; };
	
	void	SelectBrushDirect (HGDIOBJ inBrush);
	void	SelectPenDirect (HGDIOBJ inBrush);
	void	SetTextCursorInfo (VString& inFont, sLONG inSize, Boolean inBold, Boolean inItalic, sLONG inX, sLONG inY);

	// Debug Utils
	static void	RevealUpdate (HWND inHwnd);
	static void	RevealClipping (ContextRef inContext);
	void	_RevealCurClipping () const;
	static void	RevealBlitting (ContextRef inContext, const RgnRef inHwndRegion);
	static void	RevealInval (ContextRef inContext, const RgnRef inHwndRegion);
	

	HDC _GetHDC() const;
	void _ReleaseHDC(HDC inDC, bool inDeferRelease = true) const;

	/** set to true to inform context returned by _GetHDC()/BeginUsingParentContext will not be used for drawing 
	@remarks
		used only by Direct2D impl
	*/
	void SetParentContextNoDraw(bool inStatus) const
	{
		fParentContextNoDraw = inStatus;
	}
	bool GetParentContextNoDraw() const { return fParentContextNoDraw; }
protected:
	/** draw text layout 
	@remarks
		text layout origin is set to inTopLeft
	*/
	virtual void	_DrawTextLayout( VTextLayout *inTextLayout, const VPoint& inTopLeft = VPoint());
	
	/** return text layout bounds 
		@remark:
			text layout origin is set to inTopLeft
			on output, outBounds contains text layout bounds including any glyph overhangs

			on input, max width is determined by inTextLayout->GetMaxWidth() & max height by inTextLayout->GetMaxHeight()
			(if max width or max height is equal to 0.0f, it is assumed to be infinite)

			if text is empty and if inReturnCharBoundsForEmptyText == true, returned bounds will be set to a single char bounds 
			(useful while editing in order to compute initial text bounds which should not be empty in order to draw caret)
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
	virtual bool	_GetTextLayoutCharIndexFromPos( VTextLayout *inTextLayout, const VPoint& inTopLeft, const VPoint& inPos, VIndex& outCharIndex);

	/* update text layout
	@remarks
		build text layout according to layout settings & current graphic context settings

		this method is called only from VTextLayout::_UpdateTextLayout
	*/
	virtual void	_UpdateTextLayout( VTextLayout *inTextLayout);


	virtual	void	_BuildRoundRectPath(const VRect inBounds,GReal inWidth,GReal inHeight,VGraphicPath& outPath, bool inFillOnly = false);

	/** test Aero Desktop Composition **/
	typedef HRESULT (WINAPI* DwmIsCompositionEnabledProc)(BOOL *pfEnabled);
	static HMODULE							sDwmApi;
	static DwmIsCompositionEnabledProc		fDwmIsCompositionEnabledProc;
	
	static bool sDesktopCompositionEnabled;
	
	/** true is D2D is available */
	static bool	sD2DAvailable;

	/** true is D2D is enabled */
	static bool sD2DEnabled;

	/** use hardware if present or only software */
	static bool sUseHardwareIfPresent;

	/** hardware enabling status */
	static bool sHardwareEnabled;

	/** D2D factory */
#if D2D_FACTORY_MULTI_THREADED
	static CComPtr<ID2D1Factory> fFactory;
#else
	typedef std::map<VTaskID, CComPtr<ID2D1Factory>> MapFactory;
	static MapFactory fFactory;
#endif
	static VCriticalSection fMutexFactory;

	mutable VCriticalSection fMutexDraw;

	/** DirectWrite factory */
	static CComPtr<IDWriteFactory> fDWriteFactory;
	static VCriticalSection fMutexDWriteFactory;

	/** task mutexs for hardware & software shared resources */
	static VCriticalSection fMutexResources[2];

	mutable VIndex		fUseCount;
#if VERSIONDEBUG
	mutable VIndex		fTaskUseCount;
#endif
	mutable VIndex		fLockCount;
	mutable VIndex		fLockSaveUseCount;
	mutable VAffineTransform fLockSaveTransform;
	mutable VStackLayerClip fLockSaveStackLayerClip;
	mutable bool			fLockSaveHasClipOverride;
	mutable VRect			fLockSaveClipBounds;
	mutable VRegion			*fLockSaveClipRegion;
#if VERSIONDEBUG
	mutable VIndex		fDebugLockGDIInteropCounter;
	mutable VIndex		fDebugLockGDIParentCounter;
#endif
	/** true if render target has alpha
	@remarks
		ClearType can only be used if render target is opaque
	*/
	mutable bool		fIsTransparent;

	mutable VIndex		fGlobalAlphaCount;

	mutable uBOOL		fOwnRT;

	mutable GReal		fDPI;

	mutable HDC			fHDC;

	/** temporary GDI device context
	@remarks
		this GDI context is created from the render target 
		between BeginUsingContext/EndUsingContext
		& kept alive as long as needed
	*/
	mutable HDC			fGDI_HDC;
	mutable bool		fGDI_HDC_FromBeginUsingParentContext;
	mutable bool		fGDI_IsReleasing;
	mutable bool		fGDI_Fast;
	mutable bool		fGDI_QDCompatible;

	/** temporary GDI gc
	@remarks
		this temporary gc is used while rendering with GDI over render target content
		(it is released as soon as we use a drawing method or EndUsingContext)
	*/
	mutable VWinGDIGraphicContext *fGDI_GC;

	mutable sLONG		fGDI_SaveClipCounter;
	mutable sLONG		fGDI_PushedClipCounter;
	mutable sLONG		fGDI_SaveContextCounter;
	mutable std::vector<eDeferredRestoreType> fGDI_Restore;
	mutable std::vector<sLONG> fGDI_LastPushedClipCounter;
	mutable std::vector<VRect> fGDI_ClipRect;
	mutable std::vector<VFont *> fGDI_TextFont;
	mutable std::vector<VColor>  fGDI_TextColor;
	mutable std::vector<VColor>  fGDI_StrokeColor;
	mutable std::vector<bool>	 fGDI_HasSolidStrokeColor;
	mutable std::vector<bool>	 fGDI_HasSolidStrokeColorInherit;
	mutable std::vector<GReal>	 fGDI_StrokeWidth;
	mutable std::vector<const VPattern *> fGDI_StrokePattern;
	mutable std::vector<bool>	 fGDI_HasStrokeCustomDashesInherit;
	mutable std::vector<VColor>  fGDI_FillColor;
	mutable std::vector<bool>	 fGDI_HasSolidFillColor;
	mutable std::vector<bool>	 fGDI_HasSolidFillColorInherit;
	mutable std::vector<const VPattern *> fGDI_FillPattern;

	mutable bool		fGDI_CurIsParentDC;
	mutable bool		fGDI_CurIsPreparedForTransparent;
	mutable VRect		fGDI_CurBoundsClipTransparentBmpSpace;
	mutable VRect		fGDI_CurBoundsClipTransparent;

	/** lock status: lock GDI parent context or lock GDI-compatible context created from the render target content
	@remarks
		if true, _Lock() & _GetHDC() will lock & return parent context by default  
				ie context in which is presented the render target
		if false, (default) _Lock() & _GetHDC() will lock & return a GDI-compatible context created from the render target content
	@note
		locking the parent context has a very high performance cost due to the drawing context flushing & render target blitting
		so by default (false) we lock a GDI-compatible context over the render target if between BeginUsingContext/EndUsingContext
	*/
	mutable bool		fLockParentContext;

	mutable bool		fUnlockNoRestoreDrawingContext;

	/** true if GDI context is or will be not used for drawing */
	mutable bool		fParentContextNoDraw;

	/** hdc bounds 
	@remarks
		these bounds are expressed in DIP.

		if fHDC and fHwnd are both NULL
		bounds are set to render target DIP size & viewport offset
		(useful for bitmap render target)
	*/
	mutable VRect		fHDCBounds;
	mutable CComPtr<ID2D1DCRenderTarget> fDCRenderTarget;
	mutable bool		fHDCBindRectDirty;
	mutable bool		fHDCBindDCDirty;
	mutable VRect		fHDCBindRectLast;

	HWND	fHwnd;
	mutable CComPtr<ID2D1HwndRenderTarget> fHwndRenderTarget;

	mutable CComPtr<IWICBitmap>	fWICBitmap;
	mutable Gdiplus::Bitmap		*fGdiplusBmp;
	static Gdiplus::Bitmap		*fGdiplusSharedBmp;
	static GReal				fGdiplusSharedBmpWidth;
	static GReal				fGdiplusSharedBmpHeight;
	static bool					fGdiplusSharedBmpInUse;
	mutable Gdiplus::Graphics   *fGdiplusGC;
	mutable VPoint				fViewportOffset;

	mutable Gdiplus::Bitmap		*fBackingStoreGdiplusBmp; //backing storage used to cache gdiplus bitmap if actual context bitmap is WIC (opaque surface)
	mutable CComPtr<IWICBitmap> fBackingStoreWICBmp; //backing storage used to cache WIC bitmap is actual context bitmap is Gdiplus (transparent surface)

	mutable CComPtr<ID2D1RenderTarget> fRenderTarget;

	/** true if this render target should always use software resource domain, false otherwise */
	mutable bool		fIsRTSoftwareOnly;

	/** true if render target is hardware */
	mutable BOOL		fIsRTHardware;

	/** shared resources render target (one for each resource domain - hardware & software)
	@remarks
		these render targets are used only to manage shared resources
		like layers or bitmaps which are shared by render targets of the same resource domain
		(they are never bound to a HDC)
	*/
	static CComPtr<ID2D1DCRenderTarget> fRscRenderTarget[2];

	typedef std::map<sLONG, VGraphicContext *> MapOfSharedGC;

	/** map of shared GC 
	@remarks
		map is indexed per user handle & per render target domain (hardware & software)
	*/
	static MapOfSharedGC fMapOfSharedGC[2];

	mutable bool		fIsShared;
	mutable sLONG		fUserHandle;

	/** initial clipping bounds 
	@remarks
		these bounds are intersection of gdi clipping bounds & render target size
		in render target identity coordinate space
		(so equal to render target DIP size if gdi bounds are infinite):
		used to ensure render target clipping inherit clipping from gdi parent context

		valid only betweeen BeginUsingContext & EndUsingContext
	*/
	mutable VRect		fClipBoundsInit;
	mutable VRegion*    fClipRegionInit;
	mutable VRect		fClipBoundsCur;
	mutable VRegion*    fClipRegionCur;

	mutable CComPtr<ID2D1Brush>		fStrokeBrush;
	mutable CComPtr<ID2D1StrokeStyle> fStrokeStyle;
	mutable D2D1_STROKE_STYLE_PROPERTIES fStrokeStyleProps;
	mutable VLineDashPattern fStrokeDashPatternInit;
	mutable FLOAT *fStrokeDashPattern;
	mutable UINT fStrokeDashCount;
	mutable UINT fStrokeDashCountInit;
	mutable GReal					fStrokeWidth;
	mutable bool					fIsStrokeDotPattern;
	mutable bool					fIsStrokeDashPatternUnitEqualToStrokeWidth;

	mutable CComPtr<ID2D1Brush>		fFillBrush;
	mutable CComPtr<ID2D1Brush>		fTextBrush;

	mutable CComPtr<ID2D1SolidColorBrush>		fStrokeBrushSolid;
	mutable CComPtr<ID2D1SolidColorBrush>		fFillBrushSolid;
	mutable CComPtr<ID2D1SolidColorBrush>		fTextBrushSolid;

	mutable CComPtr<ID2D1LinearGradientBrush>	fStrokeBrushLinearGradient;
	mutable CComPtr<ID2D1RadialGradientBrush>	fStrokeBrushRadialGradient;

	mutable CComPtr<ID2D1LinearGradientBrush>	fFillBrushLinearGradient;
	mutable CComPtr<ID2D1RadialGradientBrush>	fFillBrushRadialGradient;

	typedef std::vector<CComPtr<ID2D1Layer>> VectorOfLayer;
	/** layer cache (one for hardware render target & one for software render target) */
	static VectorOfLayer					fVectorOfLayer[2];

	typedef STL_EXT_NAMESPACE::hash_map<XBOX::VString, CComPtr<ID2D1GradientStopCollection>, STL_EXT_NAMESPACE::STL_HASH_FUNCTOR_NAME<XBOX::VString> > MapOfGradientStopCollection;
	/** texture cache for gradient stop collections */
	mutable MapOfGradientStopCollection			fMapOfGradientStopCollection;
	
	typedef std::deque<VString>			QueueOfGradientStopCollectionID;
	/** ID queue for gradient stop collections */
	mutable QueueOfGradientStopCollectionID		fQueueOfGradientStopCollectionID;

	mutable VFont			*fTextFont;
	mutable VFontMetrics	*fTextFontMetrics;
	mutable GReal			 fWhiteSpaceWidth;
	mutable D2D1_RECT_F		 fTextRect;

	mutable VD2DDrawingContext  fDrawingContext;
	mutable VStackLayerClip	fStackLayerClip;
	mutable bool			fHasClipOverride;

	/** offscreen layer count
	@remarks
		if the actual render target is a compatible render target (bound to a offscreen or filter layer), 
		this number is equal to the count of stacked offscreen layers (in fStackLayerClip)
		otherwise it is equal to 0.
	*/
	mutable int					fLayerOffScreenCount;


	mutable DrawingMode			fDrawingMode;
	mutable TextRenderingMode	fTextRenderingMode;
	mutable TextMeasuringMode	fTextMeasuringMode;

	mutable GReal fGlobalAlpha;
	mutable GReal fShadowHOffset;
	mutable GReal fShadowVOffset;
	mutable GReal fShadowBlurness;

	mutable VPoint	fBrushPos;

private:
	// Initialization
	void	_Init ();
	void	_Dispose ();
	void	_InitDeviceDependentObjects() const;
	void	_DisposeDeviceDependentObjects(bool inDisposeSharedResources = false) const;
	static void	_InitSharedResources();
	void	_DisposeSharedResources(bool inDisposeResourceRT = false) const;
	VImageOffScreenRef _EndLayerOffScreen() const;

	/** flush all drawing commands and allow temporary read/write access to render target gdi-compatible dc
		or to attached hdc or hwnd
	@remarks
		this is useful if you want to access to a render target between a BeginUsingContext/EndUsingContext
		call _Unlock to resume drawing

		do nothing if outside BeginUsingContext/EndUsingContext
	*/
	void	_Lock() const;

	/** unlock render target & eventually resume drawing
	@remarks
		if _Lock was called between BeginUsingContext/EndUsingContext,
		this method will resume drawing and restore clipping, layers, transform & context use count
		in order to properly resume drawing
	*/
	void	_Unlock() const;

	/** get next layer from internal cache or create new one */
	ID2D1Layer *_AllocateLayer() const;

	/** free layer: store it in internal cache if cache is not full */
	void _FreeLayer( ID2D1Layer *inLayer) const;

	/** clear layer cache */
	void _ClearLayers() const;

	/** create gradient stop collection from the specified gradient pattern */
	ID2D1GradientStopCollection *_CreateGradientStopCollection( const VGradientPattern *inPattern) const;

	/** create linear gradient brush from the specified pattern */
	void _CreateLinearGradientBrush( const VPattern* inPattern, CComPtr<ID2D1Brush> &ioBrush, CComPtr<ID2D1LinearGradientBrush> &ioBrushLinear, CComPtr<ID2D1RadialGradientBrush> &ioBrushRadial) const;

	/** create radial gradient brush from the specified pattern */
	void _CreateRadialGradientBrush( const VPattern* inPattern, CComPtr<ID2D1Brush> &ioBrush, CComPtr<ID2D1LinearGradientBrush> &ioBrushLinear, CComPtr<ID2D1RadialGradientBrush> &ioBrushRadial) const;

	void	_RestoreClipOverride() const;

	/** determine current white space width 
	@remarks
		used for custom kerning
	*/
	void	_ComputeWhiteSpaceWidth() const;

	/** draw single line text using custom kerning */
	void _DrawTextWithCustomKerning(const VString& inText, const VPoint& inPos, GReal inOffsetBaseline = 0.0f);

	/** build path from polygon */
	VGraphicPath *_BuildPathFromPolygon(const VPolygon& inPolygon, bool inFillOnly = false);

	DWRITE_LINE_METRICS *_GetLineMetrics( IDWriteTextLayout *inLayout, UINT32& outLineCount, bool& doDeleteLineMetrics);
	void _GetRangeBounds( IDWriteTextLayout *inLayout, const VIndex inStart, const uLONG inLength, VRect& outRangeBounds);
	void _GetRangeBounds( IDWriteTextLayout *inLayout, const VIndex inStart, const uLONG inLength, const VPoint& inOffset, std::vector<VRect>& outRangeBounds);
	void _GetLineBoundsFromCharIndex( IDWriteTextLayout *inLayout, const VIndex inCharIndex, VRect& outLineBounds);

	void _InitDefaultFont() const;

	/** icon cache (device independent) */
	static std::map<HICON, Gdiplus::Bitmap*> fCachedIcons;

	/** D2D icon cache (device dependent: one cache for hardware render targets & one cache for software render targets)*/
	static std::map<HICON, CComPtr<ID2D1Bitmap>> fCachedIconsD2D[2];

public:
	typedef std::map<const Gdiplus::Bitmap *, CComPtr<ID2D1Bitmap>> MapOfBitmap;
private:
	/** D2D bitmap cache (device dependent: one cache for hardware render targets & one cache for software render targets) 
	@remarks
		it is indexed per Gdiplus::Bitmap * (should be removed from cache when Gdiplus::Bitmap * is destroyed) 
	*/
	static MapOfBitmap fCachedBmp[2];


	/** D2D bitmap cache for Gdiplus transparent bitmaps which do not have alpha - ie use a fixed transparent color (device dependent: one cache for hardware render targets & one cache for software render targets) 
	@remarks
		it is indexed per Gdiplus::Bitmap * (should be removed from cache when Gdiplus::Bitmap * is destroyed) 
	*/
	static MapOfBitmap fCachedBmpTransparentNoAlpha[2];

	// begin info for BeginUsingParentPort
	mutable sLONG				fHDCOpenCount;
	mutable HRGN				fHDCSavedClip;
	mutable bool				fHDCNeedRestoreClip;
	mutable	HFONT				fHDCFont;
	mutable	HFONT				fHDCSaveFont;
	mutable HDC_TransformSetter* fHDCTransform;
	mutable POINT				fHDCSaveViewportOrg;

	// end info for BeginUsingParentPort

	mutable D2D1_GRADIENT_STOP	fTempStops[8];

#if D2D_GDI_USE_GDIPLUS_BITMAP_FOR_TRANSPARENCY
	/** Gdiplus Bitmap used to draw with GDI over a transparent surface
	@remarks
		this is a workaround while using GDI over a transparent surface

		it is safe to share only one bitmap here because
		we cannot call again _GetHDC() between _GetHDC() & _ReleaseHDC()
	*/
	//mutable Gdiplus::Bitmap				*fGDI_TransparentBmp;

	///** Gdiplus graphic context used to draw with GDI over a bitmap render target */
	//mutable Gdiplus::Graphics           *fGDI_TransparentGdiplusGC;

	//mutable HDC							fGDI_TransparentHDC;

	///** D2D RT & bmp used to transfer content from rt to bitmap */
	//CComPtr<ID2D1RenderTarget>			fGDI_TransparentD2DRT;	
	//CComPtr<ID2D1Bitmap>				fGDI_TransparentD2DBmp;

	//mutable VAffineTransform			fGDI_TransparentBackupTransform;
	//mutable VRect						fGDI_TransparentBackupClipBounds;
	//mutable VRegion*					fGDI_TransparentBackupClipRegion;
#endif
};



/** static class to get HDC from gdi-compatible ID2D1RenderTarget */
class StD2DUseHDC : public VObject
{
	public:
	StD2DUseHDC(ID2D1RenderTarget* inRT);
	virtual ~StD2DUseHDC();
	
	operator HDC() { return fHDC ;};

	private:
	CComPtr<ID2D1RenderTarget>	fRenderTarget;
	HDC					fHDC;
};

END_TOOLBOX_NAMESPACE

#endif

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
#ifndef __VGraphicContext__
#define __VGraphicContext__

#include "Graphics/Sources/VGraphicSettings.h"
#include "Graphics/Sources/VRect.h"
#include "Graphics/Sources/VBezier.h"
#if VERSIONMAC
#include "Graphics/Sources/V4DPictureIncludeBase.h"
#include "Graphics/Sources/V4DPictureTools.h"
#endif


BEGIN_TOOLBOX_NAMESPACE

// forward declarations

class	VAffineTransform;
class	VRect;
class	VRegion;
class	VIcon;
class 	VPolygon;
class 	VGraphicPath;
class 	VFileBitmap;
class 	VPictureData;
class 	VTreeTextStyle;
class 	VStyledTextBox;

class	VGraphicImage;
class	VGraphicFilterProcess;

class	XTOOLBOX_API VTextLayout;

class	VImageOffScreen;

#if VERSIONWIN
class	VGDIOffScreen;
#endif
class	XTOOLBOX_API VGCOffScreen;

typedef VImageOffScreen*	VImageOffScreenRef;

class	VMapOfComputingGC;

// type definitions

/** graphic context type definition */
typedef enum GraphicContextType
{
	//on Windows Vista+, D2D graphic context (if D2D is available & enabled) (hardware if available)
	//on Windows XP, Gdiplus graphic context
	//on Mac OS, Quartz2D graphic context
	eDefaultGraphicContext		= 0,

	//on Windows Vista+, D2D software graphic context (if D2D is available & enabled) 
	//on Windows XP, Gdiplus graphic context
	//on Mac OS, Quartz2D graphic context
	eSoftwareGraphicContext,

	//on Windows Vista+, D2D graphic context (if D2D is available & enabled) (hardware if available)
	//on Windows XP, Gdiplus graphic context
	//on Mac OS, Quartz2D graphic context
	//(will switch on software only graphic context if hardware is not available)
	eHardwareGraphicContext,

#if VERSIONWIN
	eWinGDIGraphicContext,
	eWinGDIPlusGraphicContext,
	eWinD2DSoftwareGraphicContext,
	eWinD2DHardwareGraphicContext,
	ePrinterGraphicContext = eWinGDIGraphicContext,
#endif
#if VERSIONMAC
	eMacQuartz2DGraphicContext = eDefaultGraphicContext,
	ePrinterGraphicContext = eDefaultGraphicContext,
#endif

	eCountGraphicContext

} GraphicContextType;


/** Brush synchronisation flags */
typedef struct VBrushFlags {

	uWORD	fTextModeChanged			: 1;	// Brush state flags (used by VGraphicContext
	uWORD	fDrawingModeChanged			: 1;	//	to specify which brushes have been overwritten)
	uWORD	fLineSizeChanged			: 1;
	uWORD	fLineBrushChanged			: 1;
	uWORD	fFillBrushChanged			: 1;
	uWORD	fTextBrushChanged			: 1;
	uWORD	fBrushPosChanged			: 1;
	uWORD	fFontChanged				: 1;
	uWORD	fKerningChanged				: 1;
	uWORD	fPixelTransfertChanged		: 1;
	uWORD	fTextPosChanged				: 1;
	uWORD	fTextRenderingModeChanged	: 1;
	uWORD	fTextMeasuringModeChanged	: 1;
	uWORD	fUnused						: 1;	// Reserved for future use (set to 0)
	uWORD	fCustomFlag1				: 1;	// Reserved for custom use. Those flags are never
	uWORD	fCustomFlag2				: 1;	//	modified by VGraphicContext
	uWORD	fCustomFlag3				: 1;

} VBrushFlags;

/** gtaphic context native ref type definition */
#if VERSIONWIN
//Gdiplus impl: Gdiplus::Graphics *
//Direct2D impl: ID2D1RenderTarget *
typedef	void*				VGCNativeRef;
#else
#if VERSIONMAC
typedef	CGContextRef		VGCNativeRef;
#endif
#endif


/** opaque type for impl-specific bitmap ref:
	D2D graphic context: IWICBitmap *
	Gdiplus graphic context: Gdiplus Bitmap *
	Quartz2D graphic context: CGImageRef
*/
typedef void*				VBitmapNativeRef;

/** line dash pattern type definition */
typedef std::vector<GReal>	VLineDashPattern;


class XTOOLBOX_API VGraphicContext : public VObject, public IRefCountable
{
public:
	friend class VTextLayout;

			/** class initialization */
	static	Boolean				Init (bool inEnableD2D = true, bool inEnableHardware = true);
	static	void 				DeInit ();

								VGraphicContext ();
								VGraphicContext (const VGraphicContext& inOriginal);
	virtual						~VGraphicContext ();
	
			/** return current graphic context native reference */
	virtual	VGCNativeRef		GetNativeRef() const { return NULL; }

	virtual	GReal				GetDpiX() const { return 72.0f; }
	virtual GReal				GetDpiY() const { return 72.0f; }
	
	static	bool				IsD2DAvailable();
	virtual	bool				IsD2DImpl() const { return false; }
	virtual	bool				IsGdiPlusImpl() const { return false; }
	virtual	bool				IsGDIImpl() const { return false; }
	virtual	bool				IsQuartz2DImpl() const { return false; }

	static	void				DesktopCompositionModeChanged();


			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//
			// Graphic context creators
			//
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			/** create graphic context binded to the specified native context ref:
				on default graphic context type is automatic (might be overidden with inType):
				on Vista+, create a D2D graphic context
				on XP, create a Gdiplus graphic context
				on Mac OS, create a Quartz2D graphic context
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
			@param inType
				desired graphic context type
			@remarks
				if IsD2DImpl() return true, 

					caller can call BindHDC later to bind or rebind new dc.

					all coordinates are expressed in device independent units (DIP) 
					where 1 DIP = 1 pixel if render target DPI = 96 so in order to avoid DPI scaling from 96 to DPI
					we force DPI equal to 96 by default (so 1 DIP = 1 Pixel by default).
					(It is because on Windows, 4D app is not dpi-aware and we must keep 
					this limitation for backward compatibility with previous impl)

					render target DPI is computed from inPageScale using the following formula:
					render target DPI = 96.0f*inPageScale 
					(yes render target DPI is a floating point value)

					bounds are expressed in DIP (so with DPI = 96): 
					D2D internally manages page scaling by scaling bounds & coordinates by DPI/96
			*/
	static	VGraphicContext*	Create(	ContextRef inContextRef, const VRect& inBounds, const GReal inPageScale = 1.0f, 
										const bool inTransparent = true, const bool inSoftwareOnly = false, const GraphicContextType inType = eDefaultGraphicContext);

			/** create a shared graphic context binded initially to the specified native context ref 
				on default graphic context type is automatic (might be overidden with inType):
				on Vista+, create a D2D graphic context
				on XP, create a Gdiplus graphic context
				on Mac OS, create a Quartz2D graphic context
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
			@param inTransparent
				true for D2D transparent surface (used only by D2D impl)
			@param inSoftwareOnly
				true for D2D software only surface (used only by D2D impl)
			@param inType
				desired graphic context type
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
	static	VGraphicContext*	CreateShared(	sLONG inUserHandle, ContextRef inContextRef, const VRect& inBounds, const GReal inPageScale = 1.0f, 
												const bool inTransparent = true, const bool inSoftwareOnly = false, const GraphicContextType inType = eDefaultGraphicContext);


			/** create a bitmap graphic context 
				on default graphic context type is automatic (might be overidden with inType):
				on Vista+, create a D2D WIC bitmap graphic context
				on XP, create a Gdiplus bitmap graphic context
				on Mac OS, create a Quartz2D bitmap graphic context
			@remarks
				use method RetainBitmap() to get bitmap source

				if IsD2DImpl() return true, 

					all coordinates are expressed in device independent units (DIP) 
					where 1 DIP = 1 pixel if render target DPI = 96 so in order to avoid DPI scaling from 96 to DPI
					we force DPI equal to 96 by default (so 1 DIP = 1 Pixel by default).
					(It is because on Windows, 4D app is not dpi-aware and we must keep 
					this limitation for backward compatibility with previous impl)

					render target DPI is computed from inPageScale using the following formula:
					render target DPI = 96.0f*inPageScale 
					(yes render target DPI is a floating point value)
			*/
	static	VGraphicContext*	CreateBitmapGraphicContext( sLONG inWidth, sLONG inHeight, const GReal inPageScale = 1.0f, 
															const bool inTransparent = true, const VPoint* inOrigViewport = NULL, 
															bool inTransparentCompatibleGDI = true, const GraphicContextType inType = eDefaultGraphicContext);

			
			/** return graphic context type */
			GraphicContextType	GetGraphicContextType() const;

			/** return true if current gc is compatible with the passed gc type 
			@param inType
				gc type
			@param inIgnoreHardware
				true: ignore hardware status (software and hardware gc are compatible if they have the same impl)
				false: take account hardware status (will return false if gc has not the same hardware status as the passed gc type)
			*/
			bool				IsCompatible( GraphicContextType inType, bool inIgnoreHardware = false);

			/** create and return gc compatible with this gc but used only for metrics */
			VGraphicContext*	CreateCompatibleForComputingMetrics() const;

			/** create a bitmap context for the specified type which is used only for computing metrics (i.e. not for drawing or blitting) 
			@remarks
				returned graphic context is shared & stored in a internal static global table

				method is thread-safe
			*/
	static	VGraphicContext*	CreateForComputingMetrics(const GraphicContextType inType = eDefaultGraphicContext);

			/** clear the shared bitmap context for metrics associated with the specified graphic context type 

				method is thread-safe
			*/
	static	void				ClearForComputingMetrics(const GraphicContextType inType = eDefaultGraphicContext);

			/** clear all shared bitmap context for metrics 
				
				method is thread-safe
			*/
	static	void				ClearAllForComputingMetrics();

#if VERSIONWIN
			/** create graphic context binded to the specified HWND:

				on Vista+, create a D2D graphic context
				on XP, create a Gdiplus graphic context
			@param inHWND
				window handle
			@param inPageScale (used only by D2D impl - see remarks below)
				desired page scaling 
				(default is 1.0f)
			@remarks
				FIXME: for now, it is not recommended to create directly a D2D graphic context from HWND
					   because ID2D1HwndRenderTarget hardware native double-buffering is conflicting with 4D GUI (especially due to Altura/QD wrapping)
					   so you should use either Create or CreateShared instead

				if IsD2DImpl() return true, 

					caller should call WndResize if window is resized
					(otherwise render target content is scaled to fit in window bounds)

					all coordinates are expressed in device independent units (DIP) 
					where 1 DIP = 1 pixel if render target DPI = 96 so in order to avoid DPI scaling from 96 to DPI
					we force DPI equal to 96 by default (so 1 DIP = 1 Pixel by default).
					(It is because on Windows, 4D app is not dpi-aware and we must keep 
					this limitation for backward compatibility with previous impl)

					render target DPI is computed from inPageScale using the following formula:
					render target DPI = 96.0f*inPageScale 
					(yes render target DPI is a floating point value)
			*/
	static	VGraphicContext*	Create(HWND inHWND, const GReal inPageScale = 1.0f, const bool inTransparent = true, const GraphicContextType inType = eDefaultGraphicContext);
#endif

			/** resize window render target 
			@remarks
				do nothing if IsD2DImpl() return false
				do nothing if current render target is not a ID2D1HwndRenderTarget
			*/
	virtual	void				WndResize() {}

			/** bind new HDC to the current render target
			@remarks
				do nothing and return false if not IsD2DImpl()
				do nothing and return false if current render target is not a ID2D1DCRenderTarget
			*/
	virtual	bool				BindContext( ContextRef inContextRef, const VRect& inBounds, bool inResetContext = true);

			/** release the shared graphic context binded to the specified user handle	
			@see
				CreateShared
			*/
	static	void				RemoveShared( sLONG inUserHandle);

#if VERSIONWIN
			/** retain WIC bitmap  
			@remarks
				return NULL if graphic context is not a bitmap context or if context is GDI

				bitmap is still shared by the context so you should not modify it (!)
			*/
	virtual	IWICBitmap*			RetainWICBitmap() const { return NULL; }

			/** get Gdiplus bitmap  
			@remarks
				return NULL if graphic context is not a bitmap context or if context is GDI

				bitmap is still owned by the context so you should not destroy it (!)
			*/
	virtual	Gdiplus::Bitmap*	GetGdiplusBitmap() const { return NULL; }

			/** retain or get bitmap associated with graphic context

				D2D graphic context: IWICBitmap * (retained WIC bitmap)
				Gdiplus graphic context: Gdiplus Bitmap * (Gdiplus bitmap)
				GDI context: return NULL

				Quartz2D graphic context: CGImageRef (retained CoreGraphics image)
			@remarks
				NULL if graphic context is not a bitmap graphic context

				bitmap is still shared by the context so you should not modify it (!)
				if it is retainable, you should release it after usage
				if it is not (Gdiplus bitmap), you should NOT destroy it because Gdiplus bitmap is owned by the context
			*/
	virtual	VBitmapNativeRef	RetainBitmap() const 
			{ 
				if (IsD2DImpl())
					return (VBitmapNativeRef)RetainWICBitmap();
				else if (IsGdiPlusImpl())
					return (VBitmapNativeRef)GetGdiplusBitmap();
				return NULL; 
			}
#else
			/** retain bitmap associated with graphic context

				D2D graphic context: IWICBitmap * (retained WIC bitmap)
				Gdiplus graphic context: Gdiplus Bitmap * (Gdiplus bitmap)
				GDI context: return NULL

				Quartz2D graphic context: CGImageRef (retained CoreGraphics bitmap)
			@remarks
				NULL if graphic context is not a bitmap graphic context

				bitmap is still shared by the context so you should not modify it (!)
				if it is retainable, you should release it after usage
				if it is not (Gdiplus bitmap), you should NOT destroy it because Gdiplus bitmap is owned by the context
			*/
	virtual	VBitmapNativeRef	RetainBitmap() const { return NULL; }
#endif

#if VERSIONWIN
			/** draw the attached bitmap to a GDI device context 
			@remarks
				this method works with printer HDC 

				D2D impl does not support drawing directly to a printer HDC
				so for printing with D2D, you sould render first to a bitmap graphic context 
				and then call this method to draw bitmap to the printer HDC

				do nothing if current graphic context is not a bitmap graphic context
			*/
	virtual	void				DrawBitmapToHDC( HDC hdc, const VRect& inDestBounds, const Real inPageScale = 1.0f, const VRect* inSrcBounds = NULL) {}
#endif

			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//
			// graphic context settings
			//
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			/** enable/disable hardware 
			@remarks
				actually only used by Direct2D impl
			*/
	static	void				EnableHardware( bool inEnable);

			/** hardware enabling status 
			@remarks
				actually only used by Direct2D impl
			*/
	static	bool				IsHardwareEnabled();

			/** return true if graphic context uses hardware resources */
	virtual	bool				IsHardware() const 
			{ 
#if VERSIONWIN
				return false;
#elif VERSIONMAC
				return true;
#else
				return false;
#endif
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
	virtual	void				SetSoftwareOnly( bool inSoftwareOnly) {}

			/** return true if this graphic context should use software only, false otherwise */
	virtual	bool				IsSoftwareOnly() const
			{
#if VERSIONWIN
				return true;
#elif VERSIONMAC
				return false;
#else
				return false;
#endif
			}

			/** change transparency status
			@remarks
				render target can be rebuilded if transparency status is changed
			*/
	virtual	void				SetTransparent( bool inTransparent) {}

			/** return true if render target is transparent
			@remarks
				ClearType can only be used on opaque render target
			*/
	virtual	bool				IsTransparent() const { return true; }

#if VERSIONWIN
			/** GDI helper method for transparent blit */
	static	void				gdiTransparentBlt( HDC hdcDest, const VRect& inDestBounds, HDC hdcSrc, const VRect& inSrcBounds, const VColor& inColorTransparent);
#endif

			//D2D enabling/disabling

			/** enable/disable Direct2D implementation at runtime
			@remarks
				if set to false, IsD2DAvailable() will return false even if D2D is available on the platform
				if set to true, IsD2DAvailable() will return true if D2D is available on the platform
			*/
	static	void				D2DEnable( bool inD2DEnable = true);

			/** return true if Direct2D impl is enabled at runtime, false otherwise */
	static	bool				D2DIsEnabled();

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

				that mode forces crisp edges too in order lines & rect rendering is similar with GDI & D2D
				that mode is disabled on default: on default only legacy texts are rendered with GDI
				that mode does nothing in context but D2D
			*/
	virtual	void				EnableFastGDIOverD2D( bool inEnable = true) {}
	virtual bool				IsEnabledFastGDIOverD2D() const { return false; }

			/** set lock type 
			@remarks
				this method is mandatory only for Direct2D context: for all other contexts it does nothing.

				if context is Direct2D:

				if true, _Lock(), _GetHDC() and BeginUsingParentContext() will always lock & return parent context by default  
						ie the context in which is presented the render target (to which the render target is binded)
				if false (default), _Lock(), _GetHDC() and BeginUsingParentContext() will lock & return a GDI-compatible context created from the render target content
						if called between BeginUsingContext & EndUsingContext, otherwise will lock or return the context binded to the render target

				caution: please do NOT call between _GetHDC()/_ReleaseHDC() or BeginUsingParentContext/EndUsingParentContext
				
			*/
	virtual	void				SetLockParentContext( bool inLockParentContext) {}
	virtual	bool				GetLockParentContext() const { return true; }


			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//
			// Drawing settings
			//
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	virtual	DrawingMode			SetDrawingMode (DrawingMode inMode, VBrushFlags* ioFlags = NULL) = 0;

	virtual void				SetBackColor (const VColor& inColor, VBrushFlags* ioFlags = NULL) = 0;
	virtual GReal				SetTransparency (GReal inAlpha) = 0;	// Global Alpha - added to existing line or fill alpha
	virtual void				SetShadowValue (GReal inHOffset, GReal inVOffset, GReal inBlurness) = 0;	// Need a call to SetDrawingMode to update changes
	
			void				SetMaxPerfFlag(bool inMaxPerfFlag);
			bool				GetMaxPerfFlag() const {return fMaxPerfFlag;}

			/** Anti-aliasing for everything but Text */
	virtual	void				EnableAntiAliasing () = 0;
	virtual void				DisableAntiAliasing () = 0;
	virtual bool				IsAntiAliasingAvailable () = 0;

			/** return current antialiasing status (dependent on max perf flag status) */
	virtual bool				IsAntiAliasingEnabled() const { return fIsHighQualityAntialiased && !fMaxPerfFlag; }

			/** set desired antialiasing status */
	virtual void				SetDesiredAntiAliasing( bool inEnable)	
			{ 
				if (inEnable)
					EnableAntiAliasing();
				else
					DisableAntiAliasing();
			}
			/** return desired antialiasing status (as requested by EnableAntiAliasing or DisableAntiAliasing) */
	virtual	bool				GetDesiredAntiAliasing() const { return fIsHighQualityAntialiased ; }
	
			/** enable crispEdges rendering mode
			@remarks
				when crispEdges status is enabled, shape floating point coordinates are modified in transformed space
				in order to align shape coordinates with device pixels 
				(according to pixel offset mode which should be half pixel offset for all impls):
				this mode ensures shapes are drawed in device space at the same pixel position
				on all platforms (actually this is not true for more complex shapes like paths with bezier curves 
				because for complex	shapes it is not possible to get the same rendering on any platform
				but it is true for shapes which are made mainly with lines);
				this mode helps too to reduce antialiasing artifacts on horizontal & vertical lines

				this mode has a performance cost & it can deform shape especially if device resolution is 
				not the same as shape resolution (ie if transform scale from shape user space to device space is not a integer value
				or if shape uses not integer values for coordinates) so default status is disabled;
				ideally it should be enabled while drawing at device resolution when platform compatibility is required
				(like in 4D forms for drawing controls)

				this rendering mode is not compatible with QD-compatible methods so you should not enable it
				while using these methods.
			*/
			bool				EnableShapeCrispEdges( bool inShapeCrispEdgesEnabled)
			{
				bool previous = fShapeCrispEdgesEnabled;
				fShapeCrispEdgesEnabled = inShapeCrispEdgesEnabled;
				return previous;
			}
	
			/** crisEdges rendering mode accessor */
			bool				IsShapeCrispEdgesEnabled() const 
			{
				return fShapeCrispEdgesEnabled;
			}

			/** allow crispEdges algorithm to be applied on paths 
			@remarks
				by default it is set to false

				by default crispEdges algorithm is applied only on lines, rects, round rects, ellipses & polygons;
				it is applied also on paths which contain only lines if fAllowCrispEdgesOnPaths if equal to true &
				on all paths if fAllowCrispEdgesOnPaths & fAllowCrispEdgesOnBezier are both equal to true.
			*/
			void				AllowCrispEdgesOnPath( bool inAllowCrispEdgesOnPath)
			{
				fAllowCrispEdgesOnPath = inAllowCrispEdgesOnPath;
			}

			/** return true if crispEdges can be applied on paths */
			bool				IsAllowedCrispEdgesOnPath() const 
			{
				return fAllowCrispEdgesOnPath;
			} 

			/** allow crispEdges algorithm to be applied on paths with bezier curves
			@remarks
				by default it is set to false because modifying bezier coordinates can result in bezier curve distortion 
				because of control points error discrepancies
				so caller should only enable it on a per-case basis

				by default crispEdges algorithm is applied only on lines, rects, round rects, ellipses & polygons;
				it is applied also on paths which contain only lines if fAllowCrispEdgesOnPaths if equal to true &
				on all paths if fAllowCrispEdgesOnPaths & fAllowCrispEdgesOnBezier are both equal to true.
			*/
			void				AllowCrispEdgesOnBezier( bool inAllowCrispEdgesOnBezier)
			{
				if (inAllowCrispEdgesOnBezier)
					fAllowCrispEdgesOnPath = true;
				fAllowCrispEdgesOnBezier = inAllowCrispEdgesOnBezier;
			}

			/** return true if crispEdges can be applied on paths with bezier curves */
			bool				IsAllowedCrispEdgesOnBezier() const 
			{
				return fAllowCrispEdgesOnPath && fAllowCrispEdgesOnBezier;
			} 

			/** adjust point coordinates in transformed space 
			@remarks
				this method should only be called if fShapeCrispEdgesEnabled is equal to true
			*/
			void				CEAdjustPointInTransformedSpace( VPoint& ioPos, bool inFillOnly = false, VPoint *outPosTransformed = NULL, GReal *outLineWidthTransformed = NULL);

			/** adjust rect coordinates in transformed space 
			@remarks
				this method should only be called if fShapeCrispEdgesEnabled is equal to true
			*/
			void				CEAdjustRectInTransformedSpace( VRect& ioRect, bool inFillOnly = false);

			/** return true if font glyphs are aligned to pixel boundary */
virtual		bool				IsFontPixelSnappingEnabled() const { return false; }

			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//
			// Printing
			//
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			bool				IsPrintingContext() const { return fPrinterScale; }
			bool				HasPrinterScale() const { return fPrinterScale; }

			bool				GetPrinterScale(GReal &outScaleX,GReal &outScaleY) const
			{
				if(fPrinterScale)
				{
					outScaleX=fPrinterScaleX;
					outScaleY=fPrinterScaleY;
				}
				return fPrinterScale;
			}

			void				SetPrinterScale(bool inEnable,GReal inScaleX,GReal inScaleY){fPrinterScale=inEnable;fPrinterScaleX=inScaleX;fPrinterScaleY=inScaleY;};
			
			/** automatically scale shapes (origin & size) with printer scaling 
			@remarks
				enabled on default: but should be disabled if caller manages printer scaling itself 
				(for instance if caller computes rendering at printer dpi & pass transformed coordinates yet)

				it is used only by GDI impl
			*/
			bool				ShouldScaleShapesWithPrinterScale( bool inEnable)
			{
				bool previous = fShouldScaleShapesWithPrinterScale;
				fShouldScaleShapesWithPrinterScale = inEnable;
				return previous;
			}
			bool				ShouldScaleShapesWithPrinterScale() const { return fShouldScaleShapesWithPrinterScale; }

			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//
			// Fill & Stroke management
			//
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			void				SetFillStdPattern (PatternStyle inStdPattern, VBrushFlags* ioFlags = NULL);
			void				SetLineStdPattern (PatternStyle inStdPattern, VBrushFlags* ioFlags = NULL);
	
	virtual	void				SetFillColor (const VColor& inColor, VBrushFlags* ioFlags = NULL) = 0;
	
	virtual	void				SetFillPattern (const VPattern* inPattern, VBrushFlags* ioFlags = NULL) = 0;
	virtual	void				SetLineColor (const VColor& inColor, VBrushFlags* ioFlags = NULL) = 0;
	virtual	void				SetLinePattern (const VPattern* inPattern, VBrushFlags* ioFlags = NULL) = 0;
	
			/** set line dash pattern

				@param inDashOffset
					offset into the dash pattern to start the line (user units)
				@param inDashPattern
					dash pattern (alternative paint and unpaint lengths in user units)
					for instance {3,2,4} will paint line on 3 user units, unpaint on 2 user units
										 and paint again on 4 user units and so on...
			*/
	virtual void				SetLineDashPattern(GReal inDashOffset, const VLineDashPattern& inDashPattern, VBrushFlags* ioFlags = NULL) = 0;

			/** clear line dash pattern */
			void				ClearLineDashPattern(VBrushFlags* ioFlags = NULL)
			{
				SetLineDashPattern( 0.0f, VLineDashPattern(), ioFlags);
			}

			/** force dash pattern unit to be equal to stroke width (on default it is equal to user unit) */
	virtual void				ShouldLineDashPatternUnitEqualToLineWidth( bool inValue) {}
	virtual bool				ShouldLineDashPatternUnitEqualToLineWidth() const { return false; }


			/** set fill rule */
	virtual void				SetFillRule( FillRuleType inFillRule) 
			{ 
				fFillRule = inFillRule; 
			}

			/** get fill rule */
			FillRuleType		GetFillRule() const
			{
				return fFillRule;
			}
	
	virtual	void				SetLineWidth (GReal inWidth, VBrushFlags* ioFlags = NULL) = 0;
	virtual GReal				GetLineWidth () const { return 1.0f; }
	
	virtual	bool				SetHairline(bool inSet);
	virtual	bool				GetHairLine(){return fHairline;}
	
	virtual	void				SetLineStyle (CapStyle inCapStyle, JoinStyle inJoinStyle, VBrushFlags* ioFlags = NULL) = 0;
	virtual void				SetLineCap (CapStyle inCapStyle, VBrushFlags* ioFlags = NULL) = 0;
	virtual void				SetLineJoin(JoinStyle inJoinStyle, VBrushFlags* ioFlags = NULL) = 0; 
			
			/** set line miter limit */
	virtual	void				SetLineMiterLimit( GReal inMiterLimit = (GReal) 0.0) {}

	virtual void				GetBrushPos (VPoint& outHwndPos) const = 0;


			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//
			// Graphic context state save & restore
			//
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	virtual void				NormalizeContext () = 0;
	virtual void				SaveContext () = 0;
	virtual void				RestoreContext () = 0;
	
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//
			// Transforms
			//
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	virtual	void				GetTransform(VAffineTransform &outTransform)=0;
	virtual	void				SetTransform(const VAffineTransform &inTransform)=0;
	virtual	void				ConcatTransform(const VAffineTransform &inTransform)=0;
	virtual	void				RevertTransform(const VAffineTransform &inTransform)=0;
	virtual	void				ResetTransform()=0;

			/** return current graphic context transformation matrix
				in VGraphicContext normalized coordinate space
				(ie coordinate space with y axis pointing down - compliant with gdiplus and quickdraw coordinate spaces)
			*/
	virtual void				GetTransformNormalized(VAffineTransform &outTransform) { GetTransform( outTransform); }
	
			/** set current graphic context transformation matrix
				in VGraphicContext normalized coordinate space
				(ie coordinate space with y axis pointing down - compliant with gdiplus and quickdraw coordinate spaces)
			*/
	virtual void				SetTransformNormalized(const VAffineTransform &inTransform) { SetTransform( inTransform); }
	
			/** return user space to main graphic context space transformation matrix
			@remarks
				unlike GetTransform which return current graphic context transformation 
				- which can be the current layer transformation if called between BeginLayer() and EndLayer() - 
				this method return current user space to the main graphic context space transformation
				taking account all intermediate layer transformations if any
				(of course useful only if you need to map user space to/from main graphic context space 
				 - main graphic context is the first one created or set with this class constructor)
			 
				returned matrix is compliant with VGraphicContext coordinate space
				(ie coordinate space with y axis pointing down - compliant with gdiplus and quickdraw coordinate spaces)
			*/
	virtual void				GetTransformToScreen(VAffineTransform &outTransform) 	{ GetTransformToLayer( outTransform, -1); }

	virtual void				GetTransformToLayer(VAffineTransform &outTransform, sLONG inIndexLayer) { GetTransform( outTransform); }


			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//
			// Graphic layer management
			//
			//
			//	@remarks
			//		layers are graphic containers which have their own graphic context
			//		so effects like transparency, shadow or custom filters can be applied globally on a set of graphic primitives;
			//		to define a layer, start a graphic container with BeginLayer...() - either the transparency/shadow or filter version - 
			//		draw any graphic primitives and then close layer with EndLayer(): 
			//		when you call EndLayer, group of primitives is drawed using transparency, shadow or filter you have defined in BeginLayer();
			//		you can define a layer inside another layer so you can call again
			//		BeginLayer...()/EndLayer() inside BeginLayer...()/EndLayer()
			// 
			//	@note
			//		on Windows OS, layers are implemented using a bitmap graphic context and software filters for transparency/shadow or filter layers
			//		on Mac OS, transparency/shadow layers are implemented as transparency/shadow quartz layers 
			//		but filter layers are implemented using a bitmap graphic context and CoreImage filters
			//		(because CoreImage filters need at least one bitmap graphic context)
			//
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			/** Create a transparent layer graphic context
				with specified bounds and transparency 
			*/
			void				BeginLayerTransparency(const VRect& inBounds, GReal inAlpha)
			{
				_BeginLayer( inBounds, inAlpha, NULL);
			}

			/** Create a filter layer graphic context
				with specified bounds and filter
				
			@remarks
				retain the specified filter until EndLayer is called 
			*/
			void				BeginLayerFilter(const VRect& inBounds, VGraphicFilterProcess *inFilterProcess)
			{
				_BeginLayer( inBounds, (GReal) 1.0, inFilterProcess);
			}

			/** Create a shadow layer graphic context with specified bounds and transparency */
	virtual	void				BeginLayerShadow(const VRect& inBounds, const VPoint& inShadowOffset = VPoint(8,8), GReal inShadowBlurDeviation = (GReal) 1.0, const VColor& inShadowColor = VColor(0,0,0,170), GReal inAlpha = (GReal) 1.0) {}
	
			/** create a background layer 
			@param inBounds
				layer bounds
			@param inBackBufferRef
				if NULL, create new background layer with size equal to current context transformed bounds size
						 
				if <>NULL, retain inBackBufferRef & resize it to current context transformed bounds size if needed
			@param inInheritParentClipping
				if true, offscreen layer bounds will be equal to the intersection of inBounds with gc clipping bounds (transformed by ctm)
				otherwise layer bounds will be equal to inBounds (transformed by ctm)
			@param inTransparent
				true for transparent layer, false otherwise
			@return
				true if new background layer is created or if inBackBufferRef is resized (you should redraw all the layer content)
				false if inBackBufferRef is not modified (you should redraw only layer modified content)
			@remarks
				this layer is suitable for offscreen compositing (for instance for SVG component)
				as background layer is owned by caller, caller should only redraw invalidated areas
				if method returns false 
			*/
	virtual bool				BeginLayerOffScreen(const VRect& inBounds, VImageOffScreenRef inBackBufferRef = NULL, bool inInheritParentClipping = true, bool inTransparent = true)
			{
				if (inBackBufferRef == NULL)
					inBackBufferRef = (VImageOffScreenRef)0xFFFFFFFF;
				return _BeginLayer( inBounds, 1.0f, NULL, inBackBufferRef, inInheritParentClipping, inTransparent);
			}

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
	virtual	bool				DrawLayerOffScreen(const VRect& inBounds, VImageOffScreenRef inBackBuffer, bool inDrawAlways = false) { return false; }
	
			/** return true if offscreen layer needs to be cleared or resized on next call to DrawLayerOffScreen or BeginLayerOffScreen/EndLayerOffScreen
				if using inBounds as dest bounds
			@remarks
				return true if transformed input bounds size is not equal to the background layer size
				so only transformed bounds translation is allowed.
				return true if the offscreen layer is not compatible with the current gc
			*/
	virtual	bool				ShouldClearLayerOffScreen(const VRect& inBounds, VImageOffScreenRef inBackBuffer) { return true; }

			/** return index of current layer 
			@remarks
				index is zero-based
				return -1 if there is no layer
			*/
	virtual sLONG				GetIndexCurrentLayer() const
			{
				if (fStackLayerGC.size() > 0)
					return (sLONG)(fStackLayerGC.size()-1);
				else 
					return -1;
			}
	
            /** return true if current layer is a valid bitmap layer (otherwise might be a null layer - for clipping only - or shadow or transparency layer) */
	virtual	bool                IsCurrentLayerOffScreen() const
            {
                return fStackLayerBackground.size() > 0 && fStackLayerBackground.back() != NULL;
            }
    
			/**
			   Draw the background image in the container graphic context
			   
			   @return
					background image
			   @remarks
					must only be called after BeginLayerOffScreen
			*/
	virtual VImageOffScreenRef	EndLayerOffScreen() 
			{
				return _EndLayer();
			}

			/** Draw the layer graphic context in the container graphic context 
				using the filter specified in BeginLayer
				(container graphic context can be either the main graphic context or another layer graphic context)
			*/
	virtual void				EndLayer();

			/** clear all layers and restore main graphic context */
	virtual	void				ClearLayers() {}

			/** get layer filter inflating offset 
			@remark
				filter bounds are inflated (in device space) by this offset 
				in order to reduce border artifacts with some filter effects
				(like gaussian blur)
			*/
	static	GReal				GetLayerFilterInflatingOffset()
			{
				return sLayerFilterInflatingOffset;
			}

	
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//
			// Text management
			//
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	virtual	void				SetFont( VFont *inFont, GReal inScale = (GReal) 1.0) = 0;
	virtual VFont*				RetainFont() = 0;
	virtual void				SetTextPosBy (GReal inHoriz, GReal inVert) = 0;
	virtual void				SetTextPosTo (const VPoint& inHwndPos) = 0;
	virtual void				SetTextAngle (GReal inAngle) = 0;
	virtual void				GetTextPos (VPoint& outHwndPos) const = 0;
	
	virtual	bool				ShouldDrawTextOnTransparentLayer() const { return false; }

			void				UseShapeBrushForText( bool inUseShapeBrushForText)
			{
				fUseShapeBrushForText = inUseShapeBrushForText;
			}
	
			bool				UseShapeBrushForText() const
			{
				return fUseShapeBrushForText;
			}

	virtual void				SetTextColor (const VColor& inColor, VBrushFlags* ioFlags = NULL) = 0;
	virtual void				GetTextColor (VColor& outColor) const = 0;

	virtual void				SetSpaceKerning (GReal inSpaceExtra)
			{
				fCharSpacing = inSpaceExtra;
			}
			GReal				GetSpaceKerning() const
			{
				return fCharSpacing;
			}

	virtual	void				SetCharKerning (GReal inSpaceExtra)
			{
				fCharKerning = inSpaceExtra;
			}
			GReal				GetCharKerning() const
			{
				return fCharKerning;
			}
	
			/** return sum of char kerning and spacing (actual kerning) */
			GReal				GetCharActualKerning() const
			{
				return fCharKerning+fCharSpacing;
			}
	
	virtual DrawingMode			SetTextDrawingMode (DrawingMode inMode, VBrushFlags* ioFlags = NULL) = 0;
	
			/** set anti-aliasing for Text, kerning, ligatures etc... */
	virtual TextRenderingMode	SetTextRenderingMode( TextRenderingMode inMode);
			/** return current text rendering mode (dependent on max perf flag status) */
	virtual TextRenderingMode	GetTextRenderingMode() const;

			/** return desired text rendering mode (as requested by SetTextRenderingMode) */
	virtual TextRenderingMode	GetDesiredTextRenderingMode() const { return fHighQualityTextRenderingMode; }

	virtual GReal				GetCharWidth (UniChar inChar) const  = 0;
	virtual GReal				GetTextWidth (const VString& inString, bool inRTL = false) const = 0;
	virtual TextMeasuringMode	SetTextMeasuringMode(TextMeasuringMode inMode) = 0;
	virtual TextMeasuringMode	GetTextMeasuringMode() = 0;

			/** return text bounds including additional spacing
				due to layout formatting
				@remark: if you want the real bounds use GetTextBoundsTypographic
			*/
	virtual	void				GetTextBounds( const VString& inString, VRect& oRect) const;

			/** Returns ascent + descent [+ external leading] */
	virtual GReal				GetTextHeight (bool inIncludeExternalLeading = false) const = 0;

			/** Returns ascent + descent [+ external leading] each info is normalized to int before operation*/
	virtual	sLONG				GetNormalizedTextHeight (bool inIncludeExternalLeading = false) const = 0;


			/** draw text box */
	virtual	void				DrawTextBox (const VString& inString, AlignStyle inHoriz, AlignStyle inVert, const VRect& inHwndBounds, TextLayoutMode inMode = TLM_NORMAL) = 0;
	
			/** return text box bounds
				@remark:
					if input bounds width is not null, use it as max line wrapping width (or max text height in vertical layout mode)
					if input bounds height is not null, use it as max text height (or max line wrapping width in vertical layout mode)
			*/
	virtual void				GetTextBoxBounds( const VString& inString, VRect& ioHwndBounds, TextLayoutMode inMode = TLM_NORMAL) {}
	
			/** draw text string from current start pos (relative to baseline - and text box left edge on default) 
			@remarks
				if inRTLAnchorRight == true, start position is relative to the string box right edge for right to left reading direction - only if (inMode & TLM_RIGHT_TO_LEFT) != 0)
				(default is false - RTL start pos is relative to the string box left edge - as for left to right direction for backward compatibility)
			*/
	virtual void				DrawText (const VString& inString, TextLayoutMode inMode = TLM_NORMAL, bool inRTLAnchorRight = false) = 0;

			/** return true if graphic context can natively draw styled text background color */
	virtual	bool				CanDrawStyledTextBackColor() const { return false; }

#if VERSIONWIN
	virtual void				DrawStyledText( const VString& inText, VTreeTextStyle *inStyles, AlignStyle inHoriz, AlignStyle inVert, const VRect& inHwndBounds, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f)
			{
				_DrawLegacyStyledText( inText, inStyles, inHoriz, inVert, inHwndBounds, inMode, inRefDocDPI);
			}
#else
	virtual void				DrawStyledText( const VString& inText, VTreeTextStyle *inStyles, AlignStyle inHoriz, AlignStyle inVert, const VRect& inHwndBounds, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f) {}
#endif

			/** return styled text box bounds
				@remark:
					if input bounds width is not null, use it as max line wrapping width (or max text height in vertical layout mode)
					if input bounds height is not null, use it as max text height (or max line wrapping width in vertical layout mode)
			*/
	virtual void				GetStyledTextSize( const VString& inText, VTreeTextStyle *inStyles, GReal& ioWidth, GReal &ioHeight)
			{
				VRect bounds(0,0,ioWidth,ioHeight);
				GetStyledTextBoxBounds( inText, inStyles, bounds);
				ioWidth = bounds.GetWidth();
				ioHeight = bounds.GetHeight();
			}

			/** return styled text box bounds
				@remark:
					if input bounds width is not null, use it as max line wrapping width (or max text height in vertical layout mode)
					if input bounds height is not null, use it as max text height (or max line wrapping width in vertical layout mode)

					inRefDocDPI is used only on Windows to adjust font size in point according to 4D form dpi which is 72 by default
						(otherwise real font size would be scaled to screen dpi/72 which is wrong in 4D form)
						it can be used to scale font size according to any reference DPI 
						it is only needed for styled text because otherwise it is assumed that current selected font is already scaled according to 4D form DPI
			*/
#if VERSIONWIN
	virtual void				GetStyledTextBoxBounds( const VString& inText, VTreeTextStyle *inStyles, VRect& ioBounds, const GReal inRefDocDPI = 72.0f,bool inForceMonoStyle=false)
			{
				_GetLegacyStyledTextBoxBounds( inText, inStyles, ioBounds, inRefDocDPI,inForceMonoStyle);
			}
#else
	virtual void				GetStyledTextBoxBounds( const VString& inText, VTreeTextStyle *inStyles, VRect& ioBounds, const GReal inRefDocDPI = 72.0f,bool inForceMonoStyle=false) {}
#endif

	/** return styled text box run bounds from the specified range */
#if VERSIONWIN
	virtual void				GetStyledTextBoxRunBoundsFromRange( const VString& inText, VTreeTextStyle *inStyles, const VRect& inBounds, std::vector<VRect>& outRunBounds, sLONG inStart = 0, sLONG inEnd = -1, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f)
			{
				_GetLegacyStyledTextBoxRunBoundsFromRange( inText, inStyles, inBounds, outRunBounds, inStart, inEnd, inHAlign, inVAlign, inMode, inRefDocDPI);
			}
#else
	virtual void				GetStyledTextBoxRunBoundsFromRange( const VString& inText, VTreeTextStyle *inStyles, const VRect& inBounds, std::vector<VRect>& outRunBounds, sLONG inStart = 0, sLONG inEnd = -1, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f) {}
#endif

			/** for the specified styled text box, return the caret position & height of text at the specified text position
			@remarks
				text position is 0-based

				caret should be drawed from outCaretPos to VPoint(outCaretPos.x,OutCaretPos.y+outTextHeight)

				by default inCaretUseCharMetrics = true, so output caret metrics are based on the run metrics of the input character (suitable to draw caret)
				but if inCaretUseCharMetrics = false, output caret metrics are based on the full line metrics (use this one to get line height)
			*/
#if VERSIONWIN
	virtual void				GetStyledTextBoxCaretMetricsFromCharIndex( const VString& inText, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading = true, const bool inCaretUseCharMetrics = true, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f)
			{
				_GetLegacyStyledTextBoxCaretMetricsFromCharIndex( inText, inStyles, inHwndBounds, inCharIndex, outCaretPos, outTextHeight, inCaretLeading, inCaretUseCharMetrics, inHAlign, inVAlign, inMode, inRefDocDPI);
			}
#else
	virtual void				GetStyledTextBoxCaretMetricsFromCharIndex( const VString& inText, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading = true, const bool inCaretUseCharMetrics = true, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f) {}
#endif

			/** for the specified text box, return the character bounds at the specified text position
			@remarks
				text position is 0-based

				caret should be drawed from outCaretPos to VPoint(outCaretPos.x,OutCaretPos.y+outTextHeight)

				by default inCaretUseCharMetrics = true, so output caret metrics are based on the run metrics of the input character (suitable to draw caret)
				but if inCaretUseCharMetrics = false, output caret metrics are based on the full line metrics (use this one to get line height)
			*/
	virtual void				GetTextBoxCaretMetricsFromCharIndex( const VString& inText, const VRect& inHwndBounds, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading = true, const bool inCaretUseCharMetrics = true, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f)
			{
				GetStyledTextBoxCaretMetricsFromCharIndex( inText, NULL, inHwndBounds, inCharIndex, outCaretPos, outTextHeight, inCaretLeading, inCaretUseCharMetrics, inHAlign, inVAlign, inMode, inRefDocDPI);
			}
	
			/** for the specified styled text box, return the text position at the specified coordinates
			@remarks
				output text position is 0-based

				input coordinates are in graphic context user space

				return true if input position is inside character bounds
				if method returns false, it returns the closest character index to the input position
			*/
#if VERSIONWIN
	virtual bool				GetStyledTextBoxCharIndexFromCoord( const VString& inText, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VPoint& inPos, VIndex& outCharIndex, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f)
			{
				return _GetLegacyStyledTextBoxCharIndexFromCoord( inText, inStyles, inHwndBounds, inPos, outCharIndex, inHAlign, inVAlign, inMode, inRefDocDPI);
			}
#else
	virtual bool				GetStyledTextBoxCharIndexFromCoord( const VString& inText, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VPoint& inPos, VIndex& outCharIndex, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f) { return false; }
#endif

	virtual bool				GetStyledTextBoxCharIndexFromPos( const VString& inText, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VPoint& inPos, VIndex& outCharIndex, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f)
			{
				return GetStyledTextBoxCharIndexFromCoord( inText, inStyles, inHwndBounds, inPos, outCharIndex, inHAlign, inVAlign, inMode, inRefDocDPI);
			}

			/** for the specified text box, return the text position at the specified coordinates
			@remarks
				output text position is 0-based

				input coordinates are in graphic context user space

				return true if input position is inside character bounds
				if method returns false, it returns the closest character index to the input position
			*/
	virtual bool				GetTextBoxCharIndexFromCoord( const VString& inText, const VRect& inHwndBounds, const VPoint& inPos, VIndex& outCharIndex, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f)
			{
				return GetStyledTextBoxCharIndexFromCoord( inText, NULL, inHwndBounds, inPos, outCharIndex, inHAlign, inVAlign, inMode, inRefDocDPI);
			}

			/**	get text bounds 
				
				@param inString
					text string
				@param oRect
					text layout bounds (out param)
				@param inLayoutMode
				  layout mode

				@remarks
					this method return the typographic text line bounds 
				@note
					this method is used by SVG component
			*/
	virtual void				GetTextBoundsTypographic( const VString& inString, VRect& oRect, TextLayoutMode inLayoutMode = TLM_NORMAL) const;

			/** extract text lines & styles from a wrappable text using the specified passed max width
			@param inText
				text to split into lines
			@param inMaxWidth
				text max width (used for automatic wrapping)
			@param outTextLines
				output text per line
			@param inStyles
				input styles
			@param outTextLinesStyles
				output text styles per line
			@param inRefDocDPI
				input DPI (default is 72 for 4D form compliancy)
			@param inNoBreakWord
				if true (default), words are not breakable so there will be at least one word per line: in that case text can overflow max width
					(here we need to make manually the wrapping because native API should eventually break words if width is too small) 
				if false, depending on native API, words can eventually be breaked into lines if max width is too small
			@param outLinesOverflow
				if inNoBreakWord is true: return true if some output lines width is greater than max width otherwise false
				if inNoBreakWord is false, return always false
			@remarks 
				layout mode is assumed to be equal to default = TLM_NORMAL
			*/
	virtual void				GetTextBoxLines( const VString& inText, const GReal inMaxWidth, std::vector<VString>& outTextLines, VTreeTextStyle *inStyles = NULL, std::vector<VTreeTextStyle *> *outTextLinesStyles = NULL, const GReal inRefDocDPI = 72.0f, const bool inNoBreakWord = true, bool *outLinesOverflow = NULL);
	

			/** return true if current graphic context or inStyles uses only true type font(s) */
	virtual bool				UseFontTrueTypeOnly( VTreeTextStyle *inStyles = NULL);

			/** return true if inStyles uses only true type font(s) */
	static	bool				UseFontTrueTypeOnlyRec( VTreeTextStyle *inStyles = NULL);

			/**	draw text at the specified position
				
				@param inString
					text string
				@param inLayoutMode
					layout mode (here only TLM_VERTICAL and TLM_RIGHT_TO_LEFT are used)
				@param inRTLAnchorRight
					if true, start position is relative to the string box right edge for right to left reading direction - so only if (inLayoutMode & TLM_RIGHT_TO_LEFT) != 0)
					(default is false - start pos is relative to the string box left edge - as for left to right reading direction for backward compatibility)
				@remark: 
					this method does not make any layout formatting 
					text is drawed on a single line from the specified starting coordinate
					(which is relative to the first glyph horizontal or vertical baseline)
					useful when you want to position exactly a text at a specified coordinate
					without taking account extra spacing due to layout formatting (which is implementation variable)
				@note
					this method is used by SVG component
			*/
	virtual void				DrawText (const VString& inString, const VPoint& inPos, TextLayoutMode inLayoutMode = TLM_NORMAL, bool inRTLAnchorRight = false){}

			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//
			// Basic Shapes
			//
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	virtual void				DrawRect (const VRect& inHwndRect) = 0;
	virtual void				DrawRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight);
	virtual void				DrawOval (const VRect& inHwndRect) = 0;
	virtual void				DrawPolygon (const VPolygon& inHwndPolygon) = 0;
	virtual void				DrawRegion (const VRegion& inHwndRegion) = 0;
	virtual void				DrawBezier (VGraphicPath& inHwndBezier) = 0;
	virtual void				DrawPath (VGraphicPath& inHwndPath) = 0;

	virtual void				FillRect (const VRect& inHwndRect) = 0;
	virtual void				FillRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight);
	virtual void				FillOval (const VRect& inHwndRect) = 0;
	virtual void				FillPolygon (const VPolygon& inHwndPolygon) = 0;
	virtual void				FillRegion (const VRegion& inHwndRegion) = 0;
	virtual void				FillBezier (VGraphicPath& inHwndBezier) = 0;
	virtual void				FillPath (VGraphicPath& inHwndPath) = 0;

	virtual void				FrameRect (const VRect& inHwndRect) = 0;
	virtual void				FrameRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight);
	virtual void				FrameOval (const VRect& inHwndRect) = 0;
	virtual void				FramePolygon (const VPolygon& inHwndPolygon) = 0;
	virtual void				FrameRegion (const VRegion& inHwndRegion) = 0;
	virtual void				FrameBezier (const VGraphicPath& inHwndBezier) = 0;
	virtual void				FramePath (const VGraphicPath& inHwndPath) = 0;

			/** create VGraphicPath object */
	virtual	VGraphicPath*		CreatePath(bool inComputeBoundsAccurate = false) const;

			/** paint arc stroke 
			@param inCenter
				arc ellipse center
			@param inStartDeg
				starting angle in degree
			@param inEndDeg
				ending angle in degree
			@param inRX
				arc ellipse radius along x axis
			@param inRY
				arc ellipse radius along y axis
				if inRY <= 0, inRY = inRX
			@param inDrawPie (default true)
				true: draw ellipse pie between start & end
				false: draw only ellipse arc between start & end
			@remarks
				shape is painted using counterclockwise direction
				(so if inStartDeg == 0 & inEndDeg == 90, method paint up right ellipse quarter)
			*/
	virtual void				FrameArc (const VPoint& inCenter, GReal inStartDeg, GReal inEndDeg, GReal inRX, GReal inRY = 0.0f, bool inDrawPie = true);

			/** paint a arc interior shape 
			@param inCenter
				arc ellipse center
			@param inStartDeg
				starting angle in degree
			@param inEndDeg
				ending angle in degree
			@param inRX
				arc ellipse radius along x axis
			@param inRY
				arc ellipse radius along y axis
				if inRY <= 0, inRY = inRX
			@param inDrawPie (default true)
				true: draw ellipse pie between start & end
				false: draw only ellipse arc between start & end
			@remarks
				shape is painted using counterclockwise direction
				(so if inStartDeg == 0 & inEndDeg == 90, method paint up right ellipse quarter)
			*/
	virtual void				FillArc (const VPoint& inCenter, GReal inStartDeg, GReal inEndDeg, GReal inRX, GReal inRY = 0.0f, bool inDrawPie = true);

			/** paint a arc (interior & stroke)
			@param inCenter
				arc ellipse center
			@param inStartDeg
				starting angle in degree
			@param inEndDeg
				ending angle in degree
			@param inRX
				arc ellipse radius along x axis
			@param inRY
				arc ellipse radius along y axis
				if inRY <= 0, inRY = inRX
			@param inDrawPie (default true)
				true: draw ellipse pie between start & end
				false: draw only ellipse arc between start & end
			@remarks
				shape is painted using counterclockwise direction
				(so if inStartDeg == 0 & inEndDeg == 90, method paint up right ellipse quarter)
			*/
	virtual void				DrawArc (const VPoint& inCenter, GReal inStartDeg, GReal inEndDeg, GReal inRX, GReal inRY = 0.0f, bool inDrawPie = true);

	virtual void				EraseRect (const VRect& inHwndRect) = 0;
	virtual void				EraseRegion (const VRegion& inHwndRegion) = 0;

	virtual void				InvertRect (const VRect& inHwndRect) = 0;
	virtual void				InvertRegion (const VRegion& inHwndRegion) = 0;

	virtual void				DrawLine (const VPoint& inHwndStart, const VPoint& inHwndEnd) = 0;
	virtual void				DrawLine(const GReal inHwndStartX,const GReal inHwndStartY, const GReal inHwndEndX,const GReal inHwndEndY) = 0;
	virtual void				DrawLines (const GReal* inCoords, sLONG inCoordCount) = 0;
	virtual void				DrawLineTo (const VPoint& inHwndEnd) = 0;
	virtual void				DrawLineBy (GReal inWidth, GReal inHeight) = 0;
	
			/** QD compatibile drawing primitive (pen mode inset) **/
	
	virtual void				QDFrameRect (const VRect& inHwndRect) = 0;
	virtual void				QDFrameRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight)=0;
	virtual void				QDFrameOval (const VRect& inHwndRect) = 0;
	
	virtual void				QDDrawRect (const VRect& inHwndRect) = 0;
	virtual void				QDDrawRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight)=0;
	virtual void				QDDrawOval (const VRect& inHwndRect) = 0;

	virtual void				QDFillRect (const VRect& inHwndRect) = 0;
	virtual void				QDFillRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight)=0;
	virtual void				QDFillOval (const VRect& inHwndRect) = 0;

	virtual void				QDDrawLine(const GReal inHwndStartX,const GReal inHwndStartY, const GReal inHwndEndX,const GReal inHwndEndY)=0;
	virtual void				QDMoveBrushTo (const VPoint& inHwndPos) = 0;
	virtual void				QDDrawLine (const VPoint& inHwndStart, const VPoint& inHwndEnd) = 0;
	virtual void				QDDrawLineTo (const VPoint& inHwndEnd) = 0;
	virtual void				QDDrawLines (const GReal* inCoords, sLONG inCoordCount) = 0;

	virtual void				MoveBrushTo (const VPoint& inHwndPos) = 0;
	virtual void				MoveBrushBy (GReal inWidth, GReal inHeight) = 0;
	
			/** clear current graphic context area
			@remarks
				Quartz2D impl: inBounds cannot be equal to NULL (do nothing if it is the case)
				GDIPlus impl: inBounds can be NULL if and only if graphic context is a bitmap graphic context
				Direct2D impl: inBounds can be NULL (in that case the render target content is cleared)
			*/
	virtual void				Clear( const VColor& inColor = VColor(0,0,0,0), const VRect *inBounds = NULL, bool inBlendCopy = true) {}

			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//
			// Pictures
			//
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	virtual void				DrawPicture (const class VPicture& inPicture,const VRect& inHwndBounds,class VPictureDrawSettings *inSet=0) = 0;
	virtual void				DrawFileBitmap(const VFileBitmap *inFileBitmap, const VRect& inHwndBounds) = 0;
	virtual void				DrawPictureFile (const VFile& inFile, const VRect& inHwndBounds) = 0;
	virtual void				DrawIcon (const VIcon& inIcon, const VRect& inHwndBounds, PaintStatus inState = PS_NORMAL, GReal inScale = (GReal) 1.0) = 0; 
	virtual void				DrawPictureData (const VPictureData *inPictureData, const VRect& inHwndBounds,VPictureDrawSettings *inSet=0) {}


			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//
			// Clipping
			//
			//	@remarks
			//		on default, new clipping region intersects current clipping region 
			//	@note
			//		MAC OS Quartz2D implementation is restricted to intersection (API constraint)
			//		(so Quartz2D assumes that inIntersectToPrevious is always true no matter what you set)
			//
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	virtual void				ClipRect (const VRect& inHwndRect, Boolean inAddToPrevious = false, Boolean inIntersectToPrevious = true) = 0;

			/** add or intersect the specified clipping path with the current clipping path */
	virtual void				ClipPath (const VGraphicPath& inPath, Boolean inAddToPrevious = false, Boolean inIntersectToPrevious = true) {}

			/** add or intersect the specified clipping path outline with the current clipping path 
			@remarks
				this will effectively create a clipping path from the path outline 
				(using current stroke brush) and merge it with current clipping path,
				making possible to constraint drawing to the path outline
			*/
	virtual void				ClipPathOutline (const VGraphicPath& inPath, Boolean inAddToPrevious = false, Boolean inIntersectToPrevious = true) {}

	virtual void				ClipRegion (const VRegion& inHwndRgn, Boolean inAddToPrevious = false, Boolean inIntersectToPrevious = true) = 0;
	virtual void				GetClip (VRegion& outHwndRgn) const = 0;
	
	virtual void				SaveClip () = 0;
	virtual void				RestoreClip () = 0;

			/** return current clipping bounding rectangle 
			 @remarks
				bounding rectangle is expressed in VGraphicContext normalized coordinate space 
				(ie coordinate space with y axis pointing down - compliant with gdiplus and quickdraw coordinate spaces)
			*/  
	virtual void				GetClipBoundingBox( VRect& outBounds) { outBounds.SetEmpty(); }

			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//
			// Content bliting primitives
			//
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	virtual uBYTE				SetAlphaBlend (uBYTE inAlphaBlend) = 0;
	virtual void				CopyContentTo (VGraphicContext* inDestContext, const VRect* inSrcBounds = NULL, const VRect* inDestBounds = NULL, TransferMode inMode = TM_COPY, const VRegion* inMask = NULL) = 0;
	virtual VPictureData*		CopyContentToPictureData()=0;
	virtual void				Flush () = 0;
	
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//
			// Pixel drawing primitives (supported only for offscreen contexts)
			//
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	virtual void				FillUniformColor (const VPoint& inPixelPos, const VColor& inColor, const VPattern* inPattern = NULL) = 0;
	virtual void				SetPixelColor (const VPoint& inPixelPos, const VColor& inColor) = 0;
	virtual VColor*				GetPixelColor (const VPoint& inPixelPos) const = 0;

			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//
			// Port setting and restoration (use before and after any drawing suite)
			//
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			
			/** begin using graphic context 
			@param inNoDraw
				true:	use context for other purpose than drawing (like get metrics etc, set or get font, set or get fill color, etc...)
						(for some impl like D2D & for better perfs, you should never set to false if you do not intend to draw between BeginUsingContext/EndUsingContext)
				false:	use context for drawing (default)
			*/
	virtual	void				BeginUsingContext (bool inNoDraw = false) = 0;
			
			/** end using graphic context */
	virtual	void				EndUsingContext () = 0;

#if VERSIONWIN
#if ENABLE_D2D
			/** if current gc is not a D2D gc & if D2D is available & enabled, 
				derive current gc to D2D gc & return it
				(return NULL if current gc is D2D gc yet)

				you should not release returned gc if not NULL: call EndUsingD2D to properly release it
			*/
			VGraphicContext*	BeginUsingD2D(const VRect& inBounds, bool inTransparent = true, bool inSoftwareOnly = false, sLONG inSharedUserHandle = 0);

			/** end using derived D2D gc & restore context 
			@remarks
				pass the gc returned by BeginUsingD2D (might be NULL)
			*/
			void				EndUsingD2D(VGraphicContext * inDerivedGC);
#endif
			/** begin using a derived Gdiplus context over render target 
			@remarks
				this is used for workaround some D2D or GDI constraints
			*/
			VGraphicContext*	BeginUsingGdiplus() const;

			/** end using derived Gdiplus context over render target */
			void				EndUsingGdiplus() const; 
#endif

			/** pp same as GetParentPort/ReleaseParentPort but special case for gdi+/gdi 
				hdc clipregion is not sync with gdi+ clip region, to sync clip use this function */
	virtual	ContextRef			BeginUsingParentContext() const { return GetParentContext(); } 
	virtual	void				EndUsingParentContext(ContextRef inContextRef) const { ReleaseParentContext(inContextRef); }
	
	virtual	PortRef				BeginUsingParentPort()const { return GetParentPort(); }
	virtual	void				EndUsingParentPort(PortRef inPortRef) const { ReleaseParentPort(inPortRef); }

#if VERSIONMAC	
	virtual void				GetParentPortBounds( VRect& outBounds) {}
#endif

	virtual	PortRef				GetParentPort () const = 0;
	virtual	ContextRef			GetParentContext () const = 0;
	
	virtual	void				ReleaseParentPort (PortRef inPortRef) const {;};
	virtual	void				ReleaseParentContext (ContextRef inContextRef) const {;};

			/** set to true to inform context returned by _GetHDC()/BeginUsingParentContext will not be used for drawing 
			@remarks
				used only by Direct2D impl
			*/
	virtual void				SetParentContextNoDraw(bool inStatus) const {}
	virtual bool				GetParentContextNoDraw() const { return false; }

			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//
			// For printing only
			//
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	virtual	bool BeginQDContext(){return false;}
	virtual	void EndQDContext(bool inDontRestoreClip = false){ ; }
	
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//
			// Quartz2D axis setting
			//
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	virtual	Boolean				UseEuclideanAxis () = 0;
	virtual	Boolean				UseReversedAxis () = 0;

			/** transform point from QD coordinate space to Quartz2D coordinate space */
	virtual	void				QDToQuartz2D(VPoint &ioPos) {}

			/** transform rect from QD coordinate space to Quartz2D coordinate space */
	virtual	void				QDToQuartz2D(VRect &ioRect) {}

			/** transform point from Quartz2D coordinate space to QD coordinate space */
	virtual	void				Quartz2DToQD(VPoint &ioPos) {}

			/** transform rect from Quartz2D coordinate space to QD coordinate space */
	virtual	void				Quartz2DToQD(VRect &ioRect) {}

			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//
			// Utilities
			//
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	static	sWORD				GetRowBytes (sWORD inWidth, sBYTE inDepth, sBYTE inPadding);
	static	sBYTE*				RotatePixels (GReal inRadian, sBYTE* ioBits, sWORD inRowBytes, sBYTE inDepth, sBYTE inPadding, const VRect& inSrcBounds, VRect& outDestBounds, uBYTE inFillByte = 0xFF);

			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//
			// Debug Utilities
			//
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			/**	This is a utility function used when debugging update
				mecanism. It try to remain as independant as possible
				from the framework. Use with care.
				TO BE MOVED INTO GUI
			*/
	#if VERSIONMAC
	static	void				RevealUpdate(WindowRef inWindow);
	#elif VERSIONWIN
	static	void				RevealUpdate(HWND inWindow);
	#endif

	static void					RevealClipping (ContextRef inContext);
	static void					RevealBlitting (ContextRef inContext, const RgnRef inRegion);
	static void					RevealInval (ContextRef inContext, const RgnRef inRegion);
	
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

public:	//public datas
	
			/** default filter layer inflating offset 
			@remarks
				filter bounds are inflated (in device space) by this offset 
				in order to reduce border artifacts with some filter effects
				(like gaussian blur)
			*/
	static	const GReal			sLayerFilterInflatingOffset;

	static	bool				sDebugNoClipping;		// Set to true to disable automatic clipping
	static	bool				sDebugNoOffscreen;		// Set to true to disable offscreen drawing
	static	bool				sDebugRevealClipping;	// Set to true to flash the clip region
	static	bool				sDebugRevealUpdate;		// Set to true to flash the update region
	static	bool				sDebugRevealBlitting;	// Set to true to flash the blit region
	static	bool				sDebugRevealInval;		// Set to true to flash every inval region

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

protected:	//protected methods

			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//
			// Basic shapes
			//
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	virtual	void				_BuildRoundRectPath(const VRect inBounds,GReal inWidth,GReal inHeight,VGraphicPath& outPath, bool inFillOnly = false);

			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//
			// layer management
			//
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			// Create a filter or alpha layer graphic context
			// with specified bounds and filter
			// @remark : retain the specified filter until EndLayer is called
	virtual	bool				_BeginLayer(	const VRect& inBounds, 
												GReal inAlpha, 
												VGraphicFilterProcess *inFilterProcess,
												VImageOffScreenRef inBackBufferRef = NULL,
												bool inInheritParentClipping = true,
												bool inTransparent = true) { return true; }

	virtual VImageOffScreenRef	_EndLayer() { return NULL; }

			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//
			// text management
			//
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			/** extract font style from VTextStyle */
			VFontFace			_TextStyleToFontFace( const VTextStyle *inStyle);

#if VERSIONWIN
	static	void				_DrawLegacyTextBox (HDC inHDC, const VString& inString, const VColor& inColor, AlignStyle inHoriz, AlignStyle inVert, const VRect& inLocalBounds, TextLayoutMode inMode = TLM_NORMAL);
	static	void				_GetLegacyTextBoxSize(HDC inHDC, const VString& inString, VRect& ioHwndBounds, TextLayoutMode inLayoutMode = TLM_NORMAL);

			void				_DrawLegacyTextBox (const VString& inString, AlignStyle inHoriz, AlignStyle inVert, const VRect& inLocalBounds, TextLayoutMode inMode = TLM_NORMAL);
			void				_GetLegacyTextBoxSize( const VString& inString, VRect& ioHwndBounds, TextLayoutMode inLayoutMode = TLM_NORMAL) const;

	static	void				_DrawLegacyStyledText( HDC inHDC, const VString& inText, VFont *inFont, const VColor& inColor, VTreeTextStyle *inStyles, AlignStyle inHoriz, AlignStyle inVert, const VRect& inHwndBounds, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f);
	static	void				_GetLegacyStyledTextBoxBounds( HDC inHDC, const VString& inText, VFont *inFont, VTreeTextStyle *inStyles, VRect& ioBounds, const GReal inRefDocDPI = 72.0f,bool inForceMonoStyle=false, const HDC inOutputRefDC=NULL);
	static	void				_GetLegacyStyledTextBoxCaretMetricsFromCharIndex( HDC inHDC, const VString& inText, VFont *inFont, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading = true, const bool inCaretUseCharMetrics = true, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f);
	static	bool				_GetLegacyStyledTextBoxCharIndexFromCoord( HDC inHDC, const VString& inText, VFont *inFont, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VPoint& inPos, VIndex& outCharIndex, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f);
	static	void				_GetLegacyStyledTextBoxRunBoundsFromRange( HDC inHDC, const VString& inText, VFont *inFont, VTreeTextStyle *inStyles, const VRect& inBounds, std::vector<VRect>& outRunBounds, sLONG inStart = 0, sLONG inEnd = -1, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f);

			void				_DrawLegacyStyledText( const VString& inText, VTreeTextStyle *inStyles, AlignStyle inHoriz, AlignStyle inVert, const VRect& inHwndBounds, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f);
			void				_GetLegacyStyledTextBoxBounds( const VString& inText, VTreeTextStyle *inStyles, VRect& ioBounds, const GReal inRefDocDPI = 72.0f,bool inForceMonoStyle=false);
			void				_GetLegacyStyledTextBoxCaretMetricsFromCharIndex( const VString& inText, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading = true, const bool inCaretUseCharMetrics = true, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f);
			bool				_GetLegacyStyledTextBoxCharIndexFromCoord( const VString& inText, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VPoint& inPos, VIndex& outCharIndex, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f);
			void				_GetLegacyStyledTextBoxRunBoundsFromRange( const VString& inText, VTreeTextStyle *inStyles, const VRect& inBounds, std::vector<VRect>& outRunBounds, sLONG inStart = 0, sLONG inEnd = -1, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f);

			/** draw text layout 
			@remarks
				text layout left top origin is set to inTopLeft
				text layout width is determined automatically if inTextLayout->GetMaxWidth() == 0.0f, otherwise it is equal to inTextLayout->GetMaxWidth()
				text layout height is determined automatically if inTextLayout->GetMaxHeight() == 0.0f, otherwise it is equal to inTextLayout->GetMaxHeight()
			*/
	virtual void				_DrawTextLayout( VTextLayout *inTextLayout, const VPoint& inTopLeft = VPoint());

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
	virtual void				_GetTextLayoutBounds( VTextLayout *inTextLayout, VRect& outBounds, const VPoint& inTopLeft = VPoint(), bool inReturnCharBoundsForEmptyText = false);

			/** return text layout run bounds from the specified range 
			@remarks
				text layout origin is set to inTopLeft
			*/
	virtual void				_GetTextLayoutRunBoundsFromRange( VTextLayout *inTextLayout, std::vector<VRect>& outRunBounds, const VPoint& inTopLeft = VPoint(), sLONG inStart = 0, sLONG inEnd = -1);


			/** return the caret position & height of text at the specified text position in the text layout
			@remarks
				text layout origin is set to inTopLeft

				text position is 0-based

				caret should be drawed from outCaretPos to VPoint(outCaretPos.x,OutCaretPos.y+outTextHeight)
				if text layout is drawed at inTopLeft

				by default inCaretUseCharMetrics = true, so output caret metrics are based on the run metrics of the input character (suitable to draw caret)
				but if inCaretUseCharMetrics = false, output caret metrics are based on the full line metrics (use this one to get line height)
			*/
	virtual void				_GetTextLayoutCaretMetricsFromCharIndex( VTextLayout *inTextLayout, const VPoint& inTopLeft, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading = true, const bool inCaretUseCharMetrics = true);

			/** return the text position at the specified coordinates
			@remarks
				text layout origin is set to inTopLeft (in gc user space)
				the hit coordinates are defined by inPos param (in gc user space)

				output text position is 0-based

				return true if input position is inside character bounds
				if method returns false, it returns the closest character index to the input position
			*/
	virtual bool				_GetTextLayoutCharIndexFromPos( VTextLayout *inTextLayout, const VPoint& inTopLeft, const VPoint& inPos, VIndex& outCharIndex);

			/* update text layout
			@remarks
				build text layout according to layout settings & current graphic context settings

				this method is called only from VTextLayout::_UpdateTextLayout
			*/
	virtual void				_UpdateTextLayout( VTextLayout *inTextLayout);

#else
			/** draw text layout 
			@remarks
				text layout left top origin is set to inTopLeft
				text layout width is determined automatically if inTextLayout->GetMaxWidth() == 0.0f, otherwise it is equal to inTextLayout->GetMaxWidth()
				text layout height is determined automatically if inTextLayout->GetMaxHeight() == 0.0f, otherwise it is equal to inTextLayout->GetMaxHeight()
			*/
	virtual void				_DrawTextLayout( VTextLayout *inTextLayout, const VPoint& inTopLeft = VPoint()) {}
	
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
	virtual void				_GetTextLayoutBounds( VTextLayout *inTextLayout, VRect& outBounds, const VPoint& inTopLeft = VPoint(), bool inReturnCharBoundsForEmptyText = false) {}


			/** return text layout run bounds from the specified range 
			@remarks
				text layout origin is set to inTopLeft
			*/
	virtual void				_GetTextLayoutRunBoundsFromRange( VTextLayout *inTextLayout, std::vector<VRect>& outRunBounds, const VPoint& inTopLeft = VPoint(), sLONG inStart = 0, sLONG inEnd = -1) {}


			/** return the caret position & height of text at the specified text position in the text layout
			@remarks
				text layout origin is set to inTopLeft

				text position is 0-based

				caret should be drawed from outCaretPos to VPoint(outCaretPos.x,OutCaretPos.y+outTextHeight)
				if text layout is drawed at inTopLeft

				by default inCaretUseCharMetrics = true, so output caret metrics are based on the run metrics of the input character (suitable to draw caret)
				but if inCaretUseCharMetrics = false, output caret metrics are based on the full line metrics (use this one to get line height)
			*/
	virtual void				_GetTextLayoutCaretMetricsFromCharIndex( VTextLayout *inTextLayout, const VPoint& inTopLeft, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading = true, const bool inCaretUseCharMetrics = true) {}


			/** return the text position at the specified coordinates
			@remarks
				text layout origin is set to inTopLeft (in gc user space)
				the hit coordinates are defined by inPos param (in gc user space)

				output text position is 0-based

				return true if input position is inside character bounds
				if method returns false, it returns the closest character index to the input position
			*/
	virtual bool				_GetTextLayoutCharIndexFromPos( VTextLayout *inTextLayout, const VPoint& inTopLeft, const VPoint& inPos, VIndex& outCharIndex) {return false;}

			/* update text layout
			@remarks
				build text layout according to layout settings & current graphic context settings

				this method is called only from VTextLayout::_UpdateTextLayout
			*/
	virtual void				_UpdateTextLayout( VTextLayout *inTextLayout) { xbox_assert(inTextLayout); }	
#endif

			/** internally called while replacing text in order to update impl-specific stuff */
	virtual void				_PostReplaceText( VTextLayout *inTextLayout) {}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

protected:	//protected datas
	
	mutable	bool				fMaxPerfFlag;
	
			/** high quality antialiasing mode */
	mutable	bool				fIsHighQualityAntialiased;

			/** high quality text rendering mode */
	mutable TextRenderingMode	fHighQualityTextRenderingMode;

			/** current fill rule (used to fill with solid color or patterns) */
	mutable	FillRuleType		fFillRule;
	
			/** char kerning override: 0.0 for auto kerning (default) 
			@remarks
				auto-kerning if fCharKerning+fCharSpacing == 0.0 (default)
				otherwise kerning is set to fCharKerning+fCharSpacing
			*/	
	mutable GReal				fCharKerning; 
	
			/** char spacing override: 0.0 for normal spacing (default) 
			@remarks
				auto-kerning if fCharKerning+fCharSpacing == 0.0 (default)
				otherwise kerning is set to fCharKerning+fCharSpacing
			*/	
	mutable GReal				fCharSpacing; 

			/** true: text will use current fill & stroke brush (fill brush for normal & stroke brush for outline)
				false: text will use its own brush (default for best compatibility: no outline support)
			@remarks
				must be set to true in order to apply a gradient to the text
				or to draw text outline	

				TODO: implement outlined text (but for now only SVG component might need it)
			*/
	mutable bool				fUseShapeBrushForText;

			/** crispEdges status 
			@remarks
				when crispEdges status is enabled, shape floating point coordinates are modified in transformed space
				in order to align shape coordinates with device pixels 
				(according to pixel offset mode which should be half pixel offset for all impls):
				this mode ensures shapes are drawed in device space at the same pixel position
				on all platforms (actually this is not true for more complex shapes like paths with bezier curves 
				because for complex	shapes it is not possible to get the same rendering on any platform
				but it is true for shapes which are made mainly with lines);
				this mode helps too to reduce antialiasing artifacts on horizontal & vertical lines

				this mode has a performance cost & it can deform shape especially if device resolution is 
				not the same as shape resolution (ie if transform scale from shape user space to device space is not a integer value
				or if shape uses not integer values for coordinates) so default status is disabled;
				ideally it should be enabled while drawing at device resolution when platform compatibility is required

				this rendering mode is not compatible with QD-compatible methods so you should not enable it
				while using these methods.
			*/
	mutable bool				fShapeCrispEdgesEnabled;

			/** allow crispEdges algorithm to be applied on paths 
			@remarks
				by default it is set to false

				by default crispEdges algorithm is applied only on lines, rects, round rects, ellipses & polygons;
				it is applied also on paths which contain only lines if fAllowCrispEdgesOnPaths if equal to true &
				on all paths if fAllowCrispEdgesOnPaths & fAllowCrispEdgesOnBezier are both equal to true.
			*/
	mutable bool				fAllowCrispEdgesOnPath;

			/** allow crispEdges algorithm to be applied on paths with bezier curves
			@remarks
				by default it is set to false because modifying bezier coordinates can result in bezier curve distortion 
				because of control points error discrepancies
				so caller should only enable it on a per-case basis

				by default crispEdges algorithm is applied only on lines, rects, round rects, ellipses & polygons;
				it is applied also on paths which contain only lines if fAllowCrispEdgesOnPaths if equal to true &
				on all paths if fAllowCrispEdgesOnPaths & fAllowCrispEdgesOnBezier are both equal to true.
			*/
	mutable bool				fAllowCrispEdgesOnBezier;

			bool				fHairline;

			bool				fPrinterScale;
			GReal				fPrinterScaleX;
			GReal				fPrinterScaleY;
			bool				fShouldScaleShapesWithPrinterScale;

			/** layer implementation */

			std::vector<VRect>						fStackLayerBounds;			//layer bounds stack
			std::vector<bool>						fStackLayerOwnBackground;	//layer back buffer owner
			std::vector<VImageOffScreenRef>			fStackLayerBackground;		//layer back buffer stack
			std::vector<VGCNativeRef>				fStackLayerGC;				//layer graphic context stack
			std::vector<GReal>						fStackLayerAlpha;			//layer alpha stack
			std::vector<VGraphicFilterProcess *>	fStackLayerFilter;			//layer filter stack

#if VERSIONWIN
#if ENABLE_D2D
			/** current parent HDC - valid only between BeginUsingD2D & EndUsingD2D */
			HDC					fD2DParentHDC;

			sLONG				fD2DUseCount;

			VGraphicContext*	fD2DCurGC;
#endif
			/** current parent HDC - valid only between BeginUsingGdiplus & EndUsingGdiplus */
	mutable HDC					fGDIPlusHDC;
	mutable VGraphicContext		*fGDIPlusGC;
	mutable int					fGDIPlusBackupGM;
	mutable VAffineTransform    fGDIPlusBackupCTM;
#endif

static		VMapOfComputingGC*	sMapOfComputingGC;
};


/** map of computing graphic contexts 
@remarks
	this map is used to store shared bitmap graphic contexts used only for computing metrics (so not for drawing or blitting)
*/
class VMapOfComputingGC 
{
public:
			VMapOfComputingGC() {}
	virtual	~VMapOfComputingGC() {}

			/** create a bitmap context for the specified type which is used only for computing metrics (i.e. not for drawing or blitting) 
			@remarks
				returned graphic context is shared & stored in a internal global table
			*/
			VGraphicContext*	Create(const GraphicContextType inType = eDefaultGraphicContext);

			/** clear the shared bitmap context for metrics associated with the specified graphic context type */
			void				Clear(const GraphicContextType inType = eDefaultGraphicContext);

			/** clear all shared bitmap context for metrics */
			void				ClearAll();
			

private:
			typedef std::map<sLONG,VRefPtr<VGraphicContext> > MapOfGCPerType;

			VCriticalSection	fMutex;
			MapOfGCPerType		fMapOfComputingGC;
};

	


#if VERSIONWIN

/** class used to encapsulate GDI offscreen DC rendering & the underlaying layer 
@remarks
	underlaying BITMAP layer is preserved until class destruction:
	it is resized or recreated only if needed to limit allocation/deallocation

	this class is used for instance to implement double-buffering for form contexts on platform where D2D is not/partially available - as on XP:
	actually double-buffering using GDI is still faster & less buggy if D2D is not available 
	especially because of GDI ClearType (GDIPlus context layers are a bit slower and buggy with texts because of GDI ClearType)
	so GDI is still better if D2D is not available

	recommended usage is to use Begin/End methods which take care of the offscreen logic

	it is encapsulated in class VImageOffScreen in order to be used by gc methods VGraphicContext::BeginLayerOffScreen/EndLayerOffScreen
*/
class VGDIOffScreen : public XBOX::VObject
{
public:
								VGDIOffScreen()
			{
				fHDCOffScreen = NULL;
				fHBMOffScreen = NULL;
				fOldFont = NULL;
				fOldBmp = NULL;
				fActive = false;
			}

	virtual						~VGDIOffScreen()
			{
				xbox_assert(!fActive && !fHDCOffScreen);
				Clear();
			}

			void				Clear()
			{
				xbox_assert(!fActive && fHDCOffScreen == NULL && fOldFont == NULL && fOldBmp == NULL);
				fMaxBounds.SetEmpty();
				if (fHBMOffScreen)
				{
					::DeleteBitmap(fHBMOffScreen);
					fHBMOffScreen = NULL;
				}
			}

			/** begin render offscreen 
			@remarks
				returns offscreen hdc or the passed parent hdc if failed 
			*/
			HDC					Begin(HDC inHDC, const XBOX::VRect& inBounds, bool *inNewBmp = false, bool inInheritParentClipping = false);

			/** end render offscreen & blit offscreen in parent gc */
			void				End(HDC inHDCOffScreen);

			/** return true if fHBMOffScreen needs to be cleared or resized if using inBounds as dest bounds on next call to Draw or Begin */
			bool				ShouldClear( HDC inHDC, const XBOX::VRect& inBounds);

			/** draw directly fHBMOffScreen to dest dc at the specified position 
			@remarks

				caution: blitting is done in device space so bounds are transformed to device space before blitting
			*/
			bool				Draw( HDC inHDC, const XBOX::VRect& inDestBounds, bool inNeedDeviceBoundsEqualToCurLayerBounds = false);

			/** get GDI bitmap 
			@remarks
				bitmap is owned by the class: do not destroy it...
				this will return a valid handle only if at least Begin has been called once before (the bitmap is created only if needed)
				you should not select the bitmap in a context between Begin/End because it is selected yet in the offscreen context

				normally you should not use this method but for debugging purpose:
				recommended usage is to use Begin/End methods which take care of the offscreen logic
			*/
			HBITMAP				GetHBITMAP() const { return fHBMOffScreen; }

private:
			bool				fActive;

			XBOX::VRect			fMaxBounds;	  //internal bitmap bounds (greater or equal to layer bounds)
			XBOX::VRect			fLayerBounds; //layer actual bounds in device space

			HDC					fHDCParent;

			HFONT				fOldFont;
			HBITMAP				fOldBmp;
			XBOX::VRect			fBounds;	  //current clip bounds (bounds used for blitting)
			bool				fNeedResetCTMParent;
			XFORM				fCTMParent;

			HBITMAP				fHBMOffScreen; 
			HDC					fHDCOffScreen;
};
#endif

/** gc offscreen image class */
class VImageOffScreen: public VObject, public IRefCountable
{
public:
			/** create offscreen image compatible with the specified gc */
								VImageOffScreen( VGraphicContext *inGC, GReal inWidth = 1.0f, GReal inHeight = 1.0f);

			const VRect			GetBounds( VGraphicContext *inGC) const;
#if VERSIONWIN
			/** GDIPlus gc compatible offscreen */

								VImageOffScreen(Gdiplus::Bitmap *inBmp);
			void				Set(Gdiplus::Bitmap *inBmp);

			bool				IsGDIPlusImpl() const { return fGDIPlusBmp != NULL; }

			operator Gdiplus::Bitmap *() const { return fGDIPlusBmp; }

			/** GDI gc compatible offscreen */

			bool				IsGDIImpl() const { return fGDIOffScreen.GetHBITMAP() != NULL; }
			VGDIOffScreen*		GetGDIOffScreen() { return &fGDIOffScreen; }
			HBITMAP				GetHBITMAP() const { return fGDIOffScreen.GetHBITMAP(); }

#if ENABLE_D2D
			/** D2D gc compatible offscreen */

								VImageOffScreen(ID2D1BitmapRenderTarget *inBmpRT, ID2D1RenderTarget *inParentRT = NULL);
			void				SetAndRetain(ID2D1BitmapRenderTarget *inBmpRT, ID2D1RenderTarget *inParentRT = NULL);

			bool				IsD2DImpl() const { return fD2DBmpRT != NULL; }

			ID2D1RenderTarget*	GetParentRenderTarget() const { return fD2DParentRT; }
			void				ClearParentRenderTarget();
			
			operator ID2D1BitmapRenderTarget *() const { return fD2DBmpRT; }
#else
			bool				IsD2DImpl() const { return false; }
#endif
#endif

#if VERSIONMAC
			/** Quartz2D gc compatible offscreen */

								VImageOffScreen( CGLayerRef inLayerRef);
			void				SetAndRetain(CGLayerRef inLayerRef);

			bool				IsD2DImpl() const { return false; }
			bool				IsGDIPlusImpl() const { return false; }

			operator CGLayerRef() const { return fLayer; }
#endif

	virtual						~VImageOffScreen();

			void				Clear();

private:
#if VERSIONWIN
			/** GDI gc compatible offscreen */
			VGDIOffScreen		fGDIOffScreen;
			/** GDIPlus gc compatible offscreen */
			Gdiplus::Bitmap*	fGDIPlusBmp;
#if ENABLE_D2D
			/** D2D gc compatible offscreen */
			ID2D1RenderTarget*	fD2DParentRT;
			ID2D1BitmapRenderTarget*	fD2DBmpRT;
#endif
#endif

#if VERSIONMAC
			/** Quartz2D gc compatible offscreen */
			CGLayerRef			fLayer;
#endif
};


/** class used to encapsulate VGraphicContext offscreen layer rendering & the underlaying layer
@remarks 
	underlaying layer is preserved until class destruction:
	it is resized or recreated only if needed to limit allocation/deallocation

	recommended usage is to use Begin/End methods which take care of the offscreen logic
*/
class XTOOLBOX_API VGCOffScreen : public XBOX::VObject
{
public:
			/** delegate to bind a alternate drawing derived gc to the offscreen 
			@param inGC
				parent gc (which is actually the gc bound to the offscreen)
			@param inBounds
				drawing clip bounds
			@param inTransparent
				needs transparency or not
			@param outContextParent
				if delegate returns a valid derived gc, it MUST return in outContextParent = inGC->BeginUsingParentContext() in order to lock (if appropriate) the inGC parent context
				(End() will automatically call inGC->EndUsingParentContext( outContextParent))
			@param inUserData
				user data passed to SetDrawingGCDelegate
			@remarks
				delegate should return NULL if it failed or does not want to derive the context
			*/
			typedef VGraphicContext* (*fnOffScreenDrawingGCDelegate)(XBOX::VGraphicContext *inGC, const XBOX::VRect& inBounds, bool inTransparent, XBOX::ContextRef &outContextParent, void *inUserData);

								VGCOffScreen()
			{
				fLayerOffScreen = NULL;
				fActive = false;
				fDrawingGCDelegate = NULL;
				fDrawingGCDelegateUserData = NULL;
				fDrawingGC = NULL;
			}

	virtual						~VGCOffScreen()
			{
				Clear();
			}

			void				Clear()
			{
				ReleaseRefCountable(&fDrawingGC);
				xbox_assert(!fActive);
				XBOX::ReleaseRefCountable(&fLayerOffScreen);
			}

			/** begin render offscreen 
			@remarks
				return true if a new layer is created, false if current layer is used
				(true for instance if current transformed region bounds have changed)

				it is caller responsibility to clear or not the layer after call to Begin
			*/
			bool				Begin(XBOX::VGraphicContext *inGC, const XBOX::VRect& inBounds, bool inTransparent = true, bool inLayerInheritParentClipping = false, const XBOX::VRegion *inNewLayerClipRegion = NULL, const XBOX::VRegion *inReUseLayerClipRegion = NULL);

			/** end render offscreen & blit offscreen in parent gc */
			void				End(XBOX::VGraphicContext *inGC);

			/** draw internal offscreen layer using the specified bounds 
			@remarks
				this is like calling Begin & End without any drawing between
				so it is faster because only the offscreen bitmap is rendered
			 
				if transformed bounds size is not equal to background layer size, return false and do nothing 
				so only transformed bounds translation is allowed.
				if the internal offscreen is NULL, return false and do nothing

				ideally if offscreen content is not dirty, you should first call this method
				and only call Begin & End if it returns false
			 */
			bool				Draw(XBOX::VGraphicContext *inGC, const VRect& inBounds);
	
			/** return true if the internal offscreen layer needs to be cleared or resized if using inBounds as dest bounds on next call to Draw or Begin/End
			@remarks
				return true if transformed input bounds size is not equal to the offscreen layer size
				so only transformed bounds translation is allowed.
				return true if the offscreen layer is not compatible with the passed gc
			 */
			bool				ShouldClear(XBOX::VGraphicContext *inGC, const VRect& inBounds);


			/** set a delegate to bind a alternate drawing derived gc to the offscreen 
			@remarks
				must be called before Begin()
			*/
			void				SetDrawingGCDelegate( fnOffScreenDrawingGCDelegate inDelegate, void *inUserData = NULL)
			{
				if (!testAssert(!fActive))
					return;

				if (fDrawingGCDelegate == inDelegate && fDrawingGCDelegateUserData == inUserData)
					return;

				fDrawingGCDelegate = inDelegate;
				fDrawingGCDelegateUserData = inUserData;
				ReleaseRefCountable(&fDrawingGC);
			}

			/** get current derived gc if any is bound to the offscreen 
			@remarks
				should be used for drawing between Begin/End if not NULL
			*/
			VGraphicContext*	GetDrawingGC()
			{
				if (fActive)
					return fDrawingGC;
				else
					return NULL;
			}

protected:
			bool				fActive;
			XBOX::VImageOffScreenRef	fLayerOffScreen;
			bool				fTransparent;

			/** optional delegate to bind a alternate drawing derived gc to the offscreen */
			fnOffScreenDrawingGCDelegate fDrawingGCDelegate;
			void*				fDrawingGCDelegateUserData;

			/** optional derived gc bound to the back buffer for drawing (lifetime is equal to the back buffer lifetime) */
			VGraphicContext*	fDrawingGC; 
			ContextRef			fDrawingGCParentContext;
			
			bool				fNeedRestoreClip;
};


#if VERSIONWIN

/** static class for GDI transform context save/restore 
@remarks
	used only by Windows impl GDIPlus & Direct2D 
	to ensure GDI context inherits GDIPlus/Direct2D context transform
*/
class HDC_TransformSetter
{
public:
			HDC_TransformSetter(HDC inDC,VAffineTransform& inMat);
			HDC_TransformSetter(HDC inDC);
	virtual	~HDC_TransformSetter();
			
			void				SetTransform(const VAffineTransform& inMat);
			void				ResetTransform();
	
private:

			HDC					fDC;
			DWORD				fOldMode;
			XFORM				fOldTransform;
};

class XTOOLBOX_API HDC_MapModeSaverSetter
{
public:

			HDC_MapModeSaverSetter(HDC inDC,int inMapMode=0);
	virtual	~HDC_MapModeSaverSetter();
	
private:

			HDC					fDC;
			int					fMapMode;
			SIZE				fWindowExtend;
			SIZE				fViewportExtend;
};


#endif

/** class used to deal gracefully with path crispEdges status */
class StUsePath
{
public:
			StUsePath( VGraphicContext *inGC, const VGraphicPath *inPath, bool inFillOnly = false)
			{
				fIsOwned = false;
				fPathConst = fPath = NULL;
				if (inGC->IsShapeCrispEdgesEnabled() && inGC->IsAllowedCrispEdgesOnPath())
				{
					fPath = inPath->CloneWithCrispEdges( inGC, inFillOnly);
					if (fPath)
					{
						fPathConst = fPath;
						fIsOwned = true;
					}
					else
						fPathConst = inPath;
				}
				else
					fPathConst = inPath;
			}
			StUsePath( VGraphicContext *inGC, VGraphicPath *inPath, bool inFillOnly = false)
			{
				fIsOwned = false;
				fPathConst = fPath = NULL;
				if (inGC->IsShapeCrispEdgesEnabled() && inGC->IsAllowedCrispEdgesOnPath())
				{
					fPath = inPath->CloneWithCrispEdges( inGC, inFillOnly);
					if (fPath)
						fIsOwned = true;
					else
						fPath = inPath;
				}
				else
					fPath = inPath;
				fPathConst = fPath;
			}
	virtual	~StUsePath()
			{
				if (fIsOwned)
					delete fPath;
			}

			const VGraphicPath*	Get() const
			{
				return fPathConst;
			}
			VGraphicPath*		GetModifiable() 
			{
				return fPath;
			}

private:
			const VGraphicPath*	fPathConst;
			VGraphicPath*		fPath;
			bool				fIsOwned;
};

// This class automate SaveContext/SaveContext calls.
// As for all 'St' (stack based) classes you don't instantiate with 'new' operator.
//
class StSaveContext
{
public:
			StSaveContext (const VGraphicContext* inContext)
			{
				fContext = const_cast<VGraphicContext*>(inContext);
				fContext->Retain(); 
				fContext->SaveContext();
			};
	
	virtual	~StSaveContext ()
			{
				fContext->RestoreContext();
				fContext->Release();
			};
	
private:
			VGraphicContext*	fContext;
};


// This class automate SaveContext/SaveContext calls.
// As for all 'St' (stack based) classes you don't instantiate with 'new' operator.
//
class StSaveContext_NoRetain
{
public:
			StSaveContext_NoRetain (const VGraphicContext* inContext)
			{
				fContext = const_cast<VGraphicContext*>(inContext);
				fContext->SaveContext();
			};
	
	virtual	~StSaveContext_NoRetain ()
			{
				fContext->RestoreContext();
			};
	
private:

			VGraphicContext*	fContext;
};


// This class automate SetClip/RestoreClip calls.
// As for all 'St' (stack based) classes you don't instantiate with 'new' operator.
//
class StSetClipping
{
public:
			StSetClipping (const VGraphicContext* inContext)
			{
				fContext = const_cast<VGraphicContext*>(inContext);
				if ( fContext )
				{
					fContext->Retain(); 
					fContext->SaveClip();
				}
			};
	
			StSetClipping (const VGraphicContext* inContext, const VRegion& inHwndRegion, Boolean inAddToPrevious = false)
			{
				fContext = const_cast<VGraphicContext*>(inContext);
				if ( fContext )
				{
					fContext->Retain(); 
					fContext->SaveClip();
					fContext->ClipRegion(inHwndRegion, inAddToPrevious);
				}
			};
	
			StSetClipping (const VGraphicContext* inContext, const VRect& inHwndRect, Boolean inAddToPrevious = false)
			{
				fContext = const_cast<VGraphicContext*>(inContext);
				if ( fContext )
				{
					fContext->Retain(); 
					fContext->SaveClip();
					fContext->ClipRect(inHwndRect, inAddToPrevious);
				}
			};
			
	virtual	~StSetClipping ()
			{
				if ( fContext )
				{
					fContext->RestoreClip();
					fContext->Release();
				}
			};
	
private:
			VGraphicContext*	fContext;
	
#if VERSIONDEBUG	// Don't allocate in heap !
			StSetClipping () {};
#endif
};



// Same as StSetContext but with no Retain/Release. Reserved for VGraphiccontext internal use.
//
class StUseContext_NoRetain
{
public:
			StUseContext_NoRetain (const VGraphicContext* inContext) { fContext = const_cast<VGraphicContext*>(inContext); fContext->BeginUsingContext(); };	
	virtual	~StUseContext_NoRetain () { fContext->EndUsingContext(); };
	
#if VERSIONMAC
			operator PortRef () { return fContext->GetParentPort(); };
#endif

			operator ContextRef () { return fContext->GetParentContext(); };
	
private:
			VGraphicContext*	fContext;
	
#if VERSIONDEBUG	// Don't allocate in heap !
			StUseContext_NoRetain () {};
#endif
};


// Same as StSetContext but with no Retain/Release. Reserved for VGraphiccontext internal use.
//
class StUseContext_NoRetain_NoDraw
{
public:
			StUseContext_NoRetain_NoDraw (const VGraphicContext* inContext) { fContext = const_cast<VGraphicContext*>(inContext); fContext->BeginUsingContext(true); };	
	virtual ~StUseContext_NoRetain_NoDraw () { fContext->EndUsingContext(); };
	
#if VERSIONMAC
			operator PortRef () { return fContext->GetParentPort(); };
#endif

			operator ContextRef () { return fContext->GetParentContext(); };
	
private:
			VGraphicContext*	fContext;
	
#if VERSIONDEBUG	// Don't allocate in heap !
			StUseContext_NoRetain_NoDraw () {};
#endif
};





// This class automate BeginUsingContext/EndUsingContext calls without saving the settings.
// As for all 'St' (stack based) classes you don't instantiate with 'new' operator.
//
class StUseContext
{
public:
			StUseContext (const VGraphicContext* inContext)
			{
				fContext = const_cast<VGraphicContext*>(inContext);
				fContext->Retain();
				fContext->BeginUsingContext();
			};
			
	virtual ~StUseContext ()
			{
				fContext->EndUsingContext();
				fContext->Release();
			};
	
	virtual VGraphicContext*	GetGraphicContext () { return fContext; };
	
#if VERSIONMAC
			operator PortRef () { return fContext->GetParentPort(); };
#endif

			operator ContextRef () { return fContext->GetParentContext(); };
	
private:
			VGraphicContext*	fContext;

#if VERSIONDEBUG	// Don't allocate in heap !
			StUseContext () {};
#endif
};


// This class automate BeginUsingContext/EndUsingContext calls without saving the settings.
// As for all 'St' (stack based) classes you don't instantiate with 'new' operator.
//
class StUseContext_NoDraw
{
public:
			StUseContext_NoDraw (const VGraphicContext* inContext)
			{
				fContext = const_cast<VGraphicContext*>(inContext);
				fContext->Retain();
				fContext->BeginUsingContext(true);
			};
			
	virtual	~StUseContext_NoDraw ()
			{
				fContext->EndUsingContext();
				fContext->Release();
			};
	
	virtual VGraphicContext*	GetGraphicContext () { return fContext; };
	
#if VERSIONMAC
			operator PortRef () { return fContext->GetParentPort(); };
#endif

			operator ContextRef () { return fContext->GetParentContext(); };
	
private:
			VGraphicContext*	fContext;

#if VERSIONDEBUG	// Don't allocate in heap !
			StUseContext_NoDraw () {};
#endif
};


#if VERSIONWIN
class StGDIUseGraphicsAdvanced
{
public:
			StGDIUseGraphicsAdvanced(HDC hdc, bool inSaveTransform = false)
			{
				fHDC = hdc;
				if (!fHDC)
					return;
				fOldMode = ::GetGraphicsMode( fHDC);
				if (fOldMode != GM_ADVANCED)
					::SetGraphicsMode( fHDC, GM_ADVANCED);

				fRestoreCTM = inSaveTransform;
				if (fRestoreCTM)
				{
					BOOL ok = ::GetWorldTransform(fHDC, &fCTM);
					if (!ok)
						fRestoreCTM = false;
				}
			}
	virtual	~StGDIUseGraphicsAdvanced()
			{
				if (!fHDC)
					return;
				if (fRestoreCTM)
				{
					BOOL ok = ::SetWorldTransform(fHDC, &fCTM);
					xbox_assert(ok == TRUE);
				}
				if (fOldMode != GM_ADVANCED)
					::SetGraphicsMode( fHDC, fOldMode);
			}

private:
			HDC					fHDC;
			int					fOldMode;
			bool				fRestoreCTM;
			XFORM				fCTM;
};
#endif



/** will force automatic legacy text rendering on Windows:
	if font is truetype, use impl native text rendering otherwise use GDI for text rendering
@remarks
	caution: you should use same setting both while getting metrics (text bounds) & rendering
*/
class StUseTextAutoLegacy
{
public:
#if VERSIONWIN
			StUseTextAutoLegacy(VGraphicContext *inGC)
			{
				xbox_assert(inGC);
				inGC->Retain();
				fGC = inGC;
				fTextRenderingMode = inGC->GetDesiredTextRenderingMode();
				fGC->SetTextRenderingMode( fTextRenderingMode & ~(TRM_LEGACY_OFF | TRM_LEGACY_ON));
			}
	virtual	~StUseTextAutoLegacy()
			{
				fGC->SetTextRenderingMode( fTextRenderingMode);
				fGC->Release();
			}
#else
			StUseTextAutoLegacy(VGraphicContext */*inGC*/)
			{
			}
	virtual ~StUseTextAutoLegacy()
			{
			}
#endif

private:
																
			VGraphicContext*	fGC;
			TextRenderingMode	fTextRenderingMode;
};

/** will force only GDI for text rendering on Windows 
@remarks
	caution: you should use same setting both while getting metrics (text bounds) & rendering
*/
class StUseTextLegacy
{
public:
#if VERSIONWIN
			StUseTextLegacy(VGraphicContext *inGC)
			{
				xbox_assert(inGC);
				inGC->Retain();
				fGC = inGC;
				fTextRenderingMode = inGC->GetDesiredTextRenderingMode();
				fGC->SetTextRenderingMode( (fTextRenderingMode & ~TRM_LEGACY_OFF) | TRM_LEGACY_ON);
			}
	virtual ~StUseTextLegacy()
			{
				fGC->SetTextRenderingMode( fTextRenderingMode);
				fGC->Release();
			}
#else
			StUseTextLegacy(VGraphicContext */*inGC*/)
			{
			}
	virtual ~StUseTextLegacy()
			{
			}
#endif

private:
			VGraphicContext*	fGC;
			TextRenderingMode	fTextRenderingMode;
};


/** will force only impl native text rendering on Windows (never use GDI for text rendering)
@remarks
	caution: you should use same setting both while getting metrics (text bounds) & rendering
*/
class StUseTextNoLegacy
{
public:
#if VERSIONWIN
			StUseTextNoLegacy(VGraphicContext *inGC)
			{
				xbox_assert(inGC);
				inGC->Retain();
				fGC = inGC;
				fTextRenderingMode = inGC->GetDesiredTextRenderingMode();
				fGC->SetTextRenderingMode( (fTextRenderingMode & ~TRM_LEGACY_ON) | TRM_LEGACY_OFF);
			}
	virtual ~StUseTextNoLegacy()
			{
				fGC->SetTextRenderingMode( fTextRenderingMode);
				fGC->Release();
			}
#else
			StUseTextNoLegacy(VGraphicContext */*inGC*/)
			{
			}
	virtual ~StUseTextNoLegacy()
			{
			}
#endif

private:
			VGraphicContext*	fGC;
			TextRenderingMode	fTextRenderingMode;
};


/** enable fast GDI over D2D rendering (see VGraphicContext::EnableFastGDIOverD2D) */
class StFastGDIOverD2D
{
public:
#if VERSIONWIN
			StFastGDIOverD2D(VGraphicContext *inGC)
			{
				xbox_assert(inGC);
				inGC->Retain();
				fGC = inGC;
				fFastGDI = fGC->IsEnabledFastGDIOverD2D();
				fGC->EnableFastGDIOverD2D(true);
			}
	virtual ~StFastGDIOverD2D()
			{
				fGC->EnableFastGDIOverD2D( fFastGDI);
				fGC->Release();
			}
#else
			StFastGDIOverD2D(VGraphicContext */*inGC*/)
			{
			}
	virtual ~StFastGDIOverD2D()
			{
			}
#endif

private:
			VGraphicContext*	fGC;
			bool				fFastGDI;
};

class StDisableShapeCrispEdges
{
public:
			StDisableShapeCrispEdges(VGraphicContext *inGC)
			{
				xbox_assert(inGC);
				inGC->Retain();
				fGC = inGC;
				
				fShapeCrispEdgesEnabled = fGC->EnableShapeCrispEdges( false);
			}
	virtual ~StDisableShapeCrispEdges()
			{
				fGC->EnableShapeCrispEdges( fShapeCrispEdgesEnabled);
				fGC->Release();
			}

private:
			VGraphicContext*	fGC;
			bool				fShapeCrispEdgesEnabled;
};

class StEnableShapeCrispEdges
{
public:
			StEnableShapeCrispEdges(VGraphicContext *inGC, bool inEnable = true)
			{
				xbox_assert(inGC);
				inGC->Retain();
				fGC = inGC;
				fShapeCrispEdgesEnabled = fGC->EnableShapeCrispEdges( inEnable);
				
			}
	virtual ~StEnableShapeCrispEdges()
			{
				fGC->EnableShapeCrispEdges( fShapeCrispEdgesEnabled);
				fGC->Release();
			}

private:
			VGraphicContext*	fGC;
			bool				fShapeCrispEdgesEnabled;
};

class StSetAntiAliasing
{
public:
			StSetAntiAliasing(VGraphicContext *inGC,bool inEnable)
			{
				xbox_assert(inGC);
				inGC->Retain();
				fGC = inGC;
				//we must save the desired antialiasing (which is not modified by max perf flag)
				fAntiAliasing = fGC->GetDesiredAntiAliasing();
				if(inEnable)
					fGC->EnableAntiAliasing();
				else 
					fGC->DisableAntiAliasing();
						
			}
	virtual ~StSetAntiAliasing()
			{
				if(fAntiAliasing)
					fGC->EnableAntiAliasing();
				else 
					fGC->DisableAntiAliasing();

				fGC->Release();
			}
	
private:
			VGraphicContext*	fGC;
			bool				fAntiAliasing;
};

class StSetAntiAliasing_NoRetain
{
public:
			StSetAntiAliasing_NoRetain(VGraphicContext *inGC,bool inEnable)
			{
				xbox_assert(inGC);
				
				fGC = inGC;
				//we must save the desired antialiasing (which is not modified by max perf flag)
				fAntiAliasing = fGC->GetDesiredAntiAliasing();
				if(inEnable)
					fGC->EnableAntiAliasing();
				else 
					fGC->DisableAntiAliasing();
				
			}
	virtual ~StSetAntiAliasing_NoRetain()
			{
				if(fAntiAliasing)
					fGC->EnableAntiAliasing();
				else 
					fGC->DisableAntiAliasing();
				
			}
	
private:
			VGraphicContext*	fGC;
			bool				fAntiAliasing;
};


/** static class used to inform next BeginUsingParentContext that the returned parent context will NOT be used for drawing */
class StParentContextNoDraw : public VObject
{
public:
			StParentContextNoDraw(const VGraphicContext* inGC)
			{
				fGC = inGC;
				fParentContextNoDraw = inGC->GetParentContextNoDraw();
				inGC->SetParentContextNoDraw(true);
			}
	virtual ~StParentContextNoDraw()
			{
				fGC->SetParentContextNoDraw(fParentContextNoDraw);
			}

private:
			const VGraphicContext*	fGC;
			bool					fParentContextNoDraw;
};


/** static class used to inform next BeginUsingParentContext that the returned parent context will be used for drawing */
class StParentContextDraw : public VObject
{
public:
			StParentContextDraw(const VGraphicContext* inGC, bool inDraw = true)
			{
				fGC = inGC;
				fParentContextNoDraw = inGC->GetParentContextNoDraw();
				inGC->SetParentContextNoDraw(!inDraw);
			}
	virtual ~StParentContextDraw()
			{
				fGC->SetParentContextNoDraw(fParentContextNoDraw);
			}

private:
			const VGraphicContext*	fGC;
			bool					fParentContextNoDraw;
};



inline		Real					CmsToTwips (Real x) { return (x * 1440.0 / 2.54); };
inline		Real					TwipsToCms (Real x) { return (x * 2.54 / 1440.0); };
inline		Real					InchesToTwips (Real x) { return (x * 1440.0); };
inline		Real					TwipsToInches (Real x) { return (x / 1440.0); };
inline		Real					PointsToTwips (Real x) { return (x * 20.0); };
inline		Real					TwipsToPoints (Real x) { return (x / 20.0); };

END_TOOLBOX_NAMESPACE

#if VERSIONMAC
#include "Graphics/Sources/XMacQuartzGraphicContext.h"
#endif

#if VERSIONWIN
#include "Graphics/Sources/XWinGDIGraphicContext.h"
#include "Graphics/Sources/XWinGDIPlusGraphicContext.h"
#endif

#include "Graphics/Sources/VTextLayout.h"

#endif

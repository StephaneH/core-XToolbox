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
#include "V4DPictureIncludeBase.h"
#include "VGraphicContext.h"
#if VERSIONWIN
#include "XWinStyledTextBox.h"
#if ENABLE_D2D
#include "XWinD2DGraphicContext.h"
#endif
#endif
#include "VStyledTextBox.h"
#include "VPattern.h"
#include "VRect.h"
#include "VBezier.h"
#include "VRegion.h"

// Those statics are allways available for Debug/Release code comptatibility
// However they're actually unused in Release code.
//
bool	VGraphicContext::sDebugNoClipping		= false;
bool	VGraphicContext::sDebugNoOffscreen		= false;
bool	VGraphicContext::sDebugRevealClipping	= false;
bool	VGraphicContext::sDebugRevealUpdate		= false;
bool	VGraphicContext::sDebugRevealBlitting	= false;
bool	VGraphicContext::sDebugRevealInval		= false;

/** initialize filter inflating offset */
const GReal VGraphicContext::sLayerFilterInflatingOffset = (GReal) 64.0;

VMapOfComputingGC* VGraphicContext::sMapOfComputingGC = NULL;

#if VERSIONWIN

static bool IsPrinterDC(const HDC inDC)
{
	int devicetechno = ::GetDeviceCaps( inDC, TECHNOLOGY );
	return (devicetechno == DT_RASPRINTER || devicetechno == DT_PLOTTER) 	;
}

HDC_TransformSetter::HDC_TransformSetter(HDC inDC,VAffineTransform& inMat)
{	
	BOOL oksetmode;
	BOOL oksetworld;
	fDC=inDC;
	fOldMode=::GetGraphicsMode (fDC);
	if(fOldMode==GM_ADVANCED)
	{
		::GetWorldTransform (fDC,&fOldTransform);
	}
	else
	{
		oksetmode=::SetGraphicsMode(fDC,GM_ADVANCED );
		::GetWorldTransform (fDC,&fOldTransform);
	}
	XFORM form;
	inMat.ToNativeMatrix(form);
	
	oksetworld=::SetWorldTransform (fDC,&form);
}
HDC_TransformSetter::HDC_TransformSetter(HDC inDC)
{
	fDC=inDC;
	fOldMode=::GetGraphicsMode (fDC);
	if(fOldMode==GM_ADVANCED)
	{
		::GetWorldTransform (fDC,&fOldTransform);
	}
	else
	{
		::SetGraphicsMode(fDC,GM_ADVANCED );
		::GetWorldTransform (fDC,&fOldTransform);
	}
}

HDC_TransformSetter::~HDC_TransformSetter()
{
	if(::GetGraphicsMode (fDC)!=GM_ADVANCED)
		::SetGraphicsMode(fDC,GM_ADVANCED );
		
	::SetWorldTransform (fDC,&fOldTransform);
	
	if(fOldMode!=GM_ADVANCED)
		::SetGraphicsMode(fDC,fOldMode );
}
void HDC_TransformSetter::SetTransform(const VAffineTransform& inMat)
{
	XFORM form;
	inMat.ToNativeMatrix(form);
	::SetWorldTransform (fDC,&form);
}
void HDC_TransformSetter::ResetTransform()
{
	::SetWorldTransform (fDC,&fOldTransform);
}


HDC_MapModeSaverSetter::HDC_MapModeSaverSetter(HDC inDC,int inMapMode)
{
	fDC=inDC;
	fMapMode = ::GetMapMode(inDC);
			
	::GetWindowExtEx(inDC,&fWindowExtend);
	::GetViewportExtEx(inDC,&fViewportExtend);
	if(inMapMode>=MM_MIN && inMapMode<=MM_MAX)
	{
		::SetMapMode(inDC,inMapMode);
		if(inMapMode == MM_ANISOTROPIC || inMapMode == MM_ISOTROPIC)
		{
			long logx = ::GetDeviceCaps( inDC, LOGPIXELSX );
			long logy = ::GetDeviceCaps( inDC, LOGPIXELSX );
			
			// pp on utilise se mode uniquement en impression, et en dehors des graphiccontext.
			// le graphic context GDI est tj en mm_text, le scaling lors de l'impression est fait directement dans les primitives dessin/clipping
			if(VWinGDIGraphicContext::GetLogPixelsX() != logx) 
			{
				long nPrnWidth = ::GetDeviceCaps(inDC,HORZRES); // Page size (X)
				long nPrnHeight = ::GetDeviceCaps(inDC,VERTRES); // Page size (Y)
				
				::SetMapMode(inDC,inMapMode);
			
				SetWindowExtEx(inDC,MulDiv(nPrnWidth, 72,logx) ,MulDiv(nPrnHeight, 72,logy),NULL);
				SetViewportExtEx(inDC,nPrnWidth,nPrnHeight,NULL);
			}
		}
		else
			::SetMapMode(inDC,inMapMode);
	}
}

HDC_MapModeSaverSetter::~HDC_MapModeSaverSetter()
{
	::SetMapMode(fDC,fMapMode);
	::SetWindowExtEx(fDC,fWindowExtend.cx,fWindowExtend.cy,NULL);
	::SetViewportExtEx(fDC,fViewportExtend.cx,fViewportExtend.cy,NULL);
}

#endif


/** create offscreen image compatible with the specified gc */
VImageOffScreen::VImageOffScreen( VGraphicContext *inGC, GReal inWidth, GReal inHeight)
{
	xbox_assert(inGC);
	if (inWidth < 1.0f)
		inWidth = 1.0f;
	if (inHeight < 1.0f)
		inHeight = 1.0f;

#if VERSIONWIN
	fGDIPlusBmp = NULL;
#if ENABLE_D2D
	fD2DBmpRT = NULL;
	fD2DParentRT = NULL;
#endif
#elif VERSIONMAC
	fLayer = NULL;
#endif

#if VERSIONWIN
	if (inGC->IsGdiPlusImpl())
	{
		fGDIPlusBmp = new Gdiplus::Bitmap(	inWidth, 
											inHeight, 
											(Gdiplus::PixelFormat)PixelFormat32bppARGB);
		//compatible layer resolution = reference gc resolution (otherwise layer would be scaled while rendering to ref gc)
		GReal dpiX = inGC->GetDpiX(), dpiY = inGC->GetDpiY();
		if (fGDIPlusBmp)
			fGDIPlusBmp->SetResolution(dpiX, dpiY);
	}
	else if (inGC->IsD2DImpl())
	{
		HRESULT hr = S_OK;
		ID2D1RenderTarget *rt = (ID2D1RenderTarget *)inGC->GetNativeRef();
		if (rt)
		{
#if D2D_RENDER_TARGET_USE_GDI_COMPATIBLE_DC
			D2D1_SIZE_F size = D2D1::SizeF(inWidth, inHeight);
			hr = rt->CreateCompatibleRenderTarget(
					&size, NULL,
					NULL,
					D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_GDI_COMPATIBLE,
					&fD2DBmpRT);
#else
			hr = rt->CreateCompatibleRenderTarget(
					size, &fD2DBmpRT);
#endif
		}
	}
#elif VERSIONMAC	
	VRect bounds(0,0,inWidth,inHeight);
	CGRect cgBounds = bounds;
	if (inGC->GetNativeRef())
		fLayer = CGLayerCreateWithContext((CGContextRef)inGC->GetNativeRef(), cgBounds.size, NULL);
#endif
}

const VRect VImageOffScreen::GetBounds(VGraphicContext *inGC) const
{
#if VERSIONWIN
	if (inGC->IsGDIImpl())
	{
		HBITMAP hbm = fGDIOffScreen.GetHBITMAP();
		if (hbm)
		{
			BITMAP bmp;
			::GetObject(hbm, sizeof(BITMAP), &bmp);
			return VRect(0, 0, bmp.bmWidth, bmp.bmHeight);
		}
	}
	else if (inGC->IsGdiPlusImpl())
	{
		if (fGDIPlusBmp)
			return VRect( 0, 0, fGDIPlusBmp->GetWidth(), fGDIPlusBmp->GetHeight());
	}
#if ENABLE_D2D
	else if (inGC->IsD2DImpl())
	{
		if (fD2DBmpRT)
		{
			D2D1_SIZE_U size = fD2DBmpRT->GetPixelSize();
			return VRect( 0, 0, size.width, size.height);
		}
	}
#endif
#endif
#if VERSIONMAC
	if (fLayer)
	{
		CGSize size = CGLayerGetSize(fLayer);
		return VRect( 0, 0, size.width, size.height);
	}
#endif
	return VRect();
}


#if VERSIONWIN

VImageOffScreen::VImageOffScreen(Gdiplus::Bitmap *inBmp):IRefCountable()
{
	fGDIPlusBmp = inBmp;
#if ENABLE_D2D
	fD2DBmpRT = NULL;
	fD2DParentRT = NULL;
#endif
}

void VImageOffScreen::Set(Gdiplus::Bitmap *inBmp)
{
	if (fGDIPlusBmp == inBmp)
	{
#if ENABLE_D2D
		if (fD2DBmpRT)
		{
			fD2DBmpRT->Release();
			fD2DBmpRT = NULL;
		}
		if (fD2DParentRT)
		{
			fD2DParentRT->Release();
			fD2DParentRT = NULL;
		}
#endif
		return;
	}
	Clear();
	fGDIPlusBmp = inBmp;
}

#if ENABLE_D2D
VImageOffScreen::VImageOffScreen(ID2D1BitmapRenderTarget *inBmpRT, ID2D1RenderTarget *inParentRT):IRefCountable()
{
	fGDIPlusBmp = NULL;
	if (inBmpRT)
		inBmpRT->AddRef();
	fD2DBmpRT = inBmpRT;
	if (inParentRT)
		inParentRT->AddRef();
	fD2DParentRT = inParentRT;
}

void VImageOffScreen::SetAndRetain(ID2D1BitmapRenderTarget *inBmpRT, ID2D1RenderTarget *inParentRT)
{
	fGDIOffScreen.Clear();
	if (fGDIPlusBmp)
	{
		delete fGDIPlusBmp;
		fGDIPlusBmp = NULL;
	}

	if (fD2DBmpRT != inBmpRT)
	{
		if (fD2DBmpRT)
			fD2DBmpRT->Release();
		if (inBmpRT)
			inBmpRT->AddRef();
		fD2DBmpRT = inBmpRT;
	}
	if (fD2DParentRT != inParentRT)
	{
		if (fD2DParentRT)
			fD2DParentRT->Release();
		if (inParentRT)
			inParentRT->AddRef();
		fD2DParentRT = inParentRT;
	}
}
void VImageOffScreen::ClearParentRenderTarget()
{
	if (fD2DParentRT)
	{
		fD2DParentRT->Release();
		fD2DParentRT = NULL;
	}
}
#endif

#endif

#if VERSIONMAC
VImageOffScreen::VImageOffScreen( CGLayerRef inLayerRef):IRefCountable()
{
	fLayer = inLayerRef;
	if (fLayer)
		CGLayerRetain(fLayer);
}

void VImageOffScreen::SetAndRetain(CGLayerRef inLayerRef)
{
	if (fLayer == inLayerRef)
		return;
	Clear();
	fLayer = inLayerRef;
	if (fLayer)
		CGLayerRetain(fLayer);
}
#endif

void VImageOffScreen::Clear()
{
#if VERSIONWIN
	fGDIOffScreen.Clear();
	if (fGDIPlusBmp)
	{
		delete fGDIPlusBmp;
		fGDIPlusBmp = NULL;
	}
#if ENABLE_D2D
	if (fD2DBmpRT)
	{
		fD2DBmpRT->Release();
		fD2DBmpRT = NULL;
	}
	if (fD2DParentRT)
	{
		fD2DParentRT->Release();
		fD2DParentRT = NULL;
	}
#endif
#endif

#if VERSIONMAC
	if (fLayer)
	{
		CGLayerRelease(fLayer);
		fLayer = NULL;
	}
#endif
}

VImageOffScreen::~VImageOffScreen()
{
	Clear();
}




Boolean VGraphicContext::Init(bool inEnableD2D, bool inEnableHardware)
{
	sMapOfComputingGC = new VMapOfComputingGC();
#if VERSIONMAC
	return VMacQuartzGraphicContext::Init();
#else
	VWinGDIGraphicContext::Init();
#if ENABLE_D2D
	if (inEnableD2D)
		VWinD2DGraphicContext::Init(inEnableHardware);
	return VWinGDIPlusGraphicContext::Init();
#else
	return VWinGDIPlusGraphicContext::Init();
#endif
#endif
}


void VGraphicContext::DeInit()
{
#if VERSIONMAC
	VMacQuartzGraphicContext::DeInit();
#else
#if ENABLE_D2D
	VWinD2DGraphicContext::DeInit();
#endif
	VWinGDIGraphicContext::DeInit();
//	VWinGDIPlusGraphicContext::DeInit();
#endif

	if (sMapOfComputingGC)
	{
		delete sMapOfComputingGC;
		sMapOfComputingGC = NULL;
	}
}

void VGraphicContext::SetMaxPerfFlag(bool inMaxPerfFlag) 
{
	if (inMaxPerfFlag == fMaxPerfFlag)
		return;

	fMaxPerfFlag = inMaxPerfFlag;

	if (fMaxPerfFlag)
	{
		//disable antialiasing for shapes & text

		//backup high quality status (inverse of max performance status)
		bool highQualityAntialiased = fIsHighQualityAntialiased;
		TextRenderingMode highQualityTextRenderingMode = fHighQualityTextRenderingMode;

		//here as fMaxPerfFlag == true, calling following methods will actually disable smoothing text & antialiasing
		SetTextRenderingMode( fHighQualityTextRenderingMode);
		DisableAntiAliasing();

		//keep high quality status for later restore (restored if max perf is disabled)
		fIsHighQualityAntialiased = highQualityAntialiased;
		fHighQualityTextRenderingMode = highQualityTextRenderingMode;
	}
	else
	{
		//restore high quality antialiasing as set with SetTextRenderingMode & EnableAntiAliasing

		SetTextRenderingMode( fHighQualityTextRenderingMode);
		if (fIsHighQualityAntialiased)
			EnableAntiAliasing();
		else 
			DisableAntiAliasing();
	}
}

GReal VGraphicContext::GetDpiScreen()
{
	//remark: on Mac OS, for consistency, we assume screen dpi = 72 as Quartz2D uses res-independent dpi = 72 
	//(it is Mac OS system responsibility to scale from Quartz2D dpi to device dpi - especially on high-dpi devices)
	GReal dpi = 72;

#if VERSIONWIN
	dpi = VWinGDIGraphicContext::GetLogPixelsX();
#endif
	return dpi;
}

bool VGraphicContext::IsD2DAvailable()
{
#if ENABLE_D2D
	return VWinD2DGraphicContext::IsAvailable() && VWinD2DGraphicContext::IsEnabled(); // && VWinD2DGraphicContext::IsDesktopCompositionEnabled();
#else
	return false;
#endif
}

/** enable/disable Direct2D implementation at runtime
@remarks
	if set to false, IsD2DAvailable() will return false even if D2D is available on the platform
	if set to true, IsD2DAvailable() will return true if D2D is available on the platform
*/
void VGraphicContext::D2DEnable( bool inD2DEnable)
{
#if ENABLE_D2D	
	bool isEnabled = D2DIsEnabled();
	VWinD2DGraphicContext::D2DEnable( inD2DEnable);
	if (isEnabled != D2DIsEnabled())
		//we need to clear shared computing graphic contexts is D2D status has changed
		ClearAllForComputingMetrics();
#endif
}

/** return true if Direct2D impl is enabled at runtime, false otherwise */
bool VGraphicContext::D2DIsEnabled()
{
#if ENABLE_D2D	
	return VWinD2DGraphicContext::IsEnabled();
#else
	return false;
#endif
}


/** enable/disable hardware 
@remarks
	actually only used by Direct2D impl
*/
void VGraphicContext::EnableHardware( bool inEnable)
{
	#if ENABLE_D2D
	VWinD2DGraphicContext::EnableHardware( inEnable);
	#endif
}

/** hardware enabling status 
@remarks
	actually only used by Direct2D impl
*/
bool VGraphicContext::IsHardwareEnabled() 
{
	#if ENABLE_D2D
	return VWinD2DGraphicContext::IsHardwareEnabled();
	#else
	#if VERSIONWIN
	return false;
	#elif VERSIONMAC
	return true;
	#else
	return false;
	#endif
	#endif
}


void VGraphicContext::DesktopCompositionModeChanged()
{
	#if ENABLE_D2D
	VWinD2DGraphicContext::DesktopCompositionModeChanged();
	#endif
}

/** create graphic context binded to the specified native context ref:
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
		D2D internally manages page scaling by scaling bounds & coordinates by DPI/96; 
*/
VGraphicContext* VGraphicContext::Create(ContextRef inContextRef, const VRect& inBounds, const GReal inPageScale, const bool inTransparent, const bool inSoftwareOnly, const GraphicContextType inType)
{
#if VERSIONWIN
	bool softwareOnly = inSoftwareOnly || !VGraphicContext::IsHardwareEnabled() || (inType == eSoftwareGraphicContext) || (inType == eWinD2DSoftwareGraphicContext);
	switch (inType)
	{
	case eDefaultGraphicContext:
	case eSoftwareGraphicContext:
	case eHardwareGraphicContext:
	case eWinD2DSoftwareGraphicContext:
	case eWinD2DHardwareGraphicContext:
		{
	#if ENABLE_D2D
		if (IsD2DAvailable() && (!softwareOnly || VSystem::IsSeven())) //fallback to GDIPlus if software & not seven
		{
			//Vista+
			VGraphicContext *gc = static_cast<VGraphicContext *>(new VWinD2DGraphicContext( inContextRef, inBounds, inPageScale, inTransparent));
			if (gc)
				gc->SetSoftwareOnly(softwareOnly);
			return gc;
		}
		else
	#endif
			//XP
			return static_cast<VGraphicContext *>(new VWinGDIPlusGraphicContext( inContextRef));
		}
		break;
	case eWinGDIPlusGraphicContext:
		return static_cast<VGraphicContext *>(new VWinGDIPlusGraphicContext( inContextRef));
		break;
	case eWinGDIGraphicContext:
		return static_cast<VGraphicContext *>(new VWinGDIGraphicContext( inContextRef));
		break;
	default:
		{
		xbox_assert(false);
		return static_cast<VGraphicContext *>(new VWinGDIGraphicContext( inContextRef));
		}
		break;
	}
#elif VERSIONMAC
	CGRect boundsNative;
	inBounds.MAC_ToCGRect( boundsNative);
	return static_cast<VGraphicContext *>(new VMacQuartzGraphicContext( inContextRef, true, boundsNative));
#else
	return NULL;
#endif
}

/** bind new HDC to the current render target
@remarks
	do nothing and return false if not IsD2DImpl()
	do nothing and return false if current render target is not a ID2D1DCRenderTarget
*/
bool VGraphicContext::BindContext( ContextRef inContextRef, const VRect& inBounds, bool inResetContext)
{
#if VERSIONWIN
#if ENABLE_D2D
	if (IsD2DImpl())
	{
		VWinD2DGraphicContext *gcNative = static_cast<VWinD2DGraphicContext *>(this);
		return gcNative->BindHDC( inContextRef, inBounds, inResetContext);
	}
#endif
#endif
	return false;
}

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
		D2D internally manages page scaling by scaling bounds & coordinates by DPI/96
*/
VGraphicContext* VGraphicContext::CreateShared(sLONG inUserHandle, ContextRef inContextRef, const VRect& inBounds, const GReal inPageScale, const bool inTransparent, const bool inSoftwareOnly, const GraphicContextType inType)
{
#if ENABLE_D2D
	if (IsD2DAvailable())
	{
		bool softwareOnly = inSoftwareOnly || !VGraphicContext::IsHardwareEnabled() || (inType == eSoftwareGraphicContext) || (inType == eWinD2DSoftwareGraphicContext);
		switch (inType)
		{
		case eDefaultGraphicContext:
		case eSoftwareGraphicContext:
		case eHardwareGraphicContext:
		case eWinD2DSoftwareGraphicContext:
		case eWinD2DHardwareGraphicContext:
			if (!softwareOnly || VSystem::IsSeven()) //fallback to GDIPlus on Vista if software only
				return VWinD2DGraphicContext::CreateShared(inUserHandle, inContextRef, inBounds, inPageScale, inTransparent, softwareOnly);
			else
				return static_cast<VGraphicContext *>(new VWinGDIPlusGraphicContext( inContextRef));
			break;
		case eWinGDIPlusGraphicContext:
			return static_cast<VGraphicContext *>(new VWinGDIPlusGraphicContext( inContextRef));
			break;
		case eWinGDIGraphicContext:
			return static_cast<VGraphicContext *>(new VWinGDIGraphicContext( inContextRef));
			break;
		default:
			{
			xbox_assert(false);
			return static_cast<VGraphicContext *>(new VWinGDIGraphicContext( inContextRef));
			}
			break;
		}
	}
#endif
	return VGraphicContext::Create( inContextRef, inBounds, inPageScale, inTransparent, inSoftwareOnly, inType);
}


/** release the shared graphic context binded to the specified user handle	
@see
	CreateShared
*/
void VGraphicContext::RemoveShared( sLONG inUserHandle)
{
#if ENABLE_D2D
	if (VWinD2DGraphicContext::IsAvailable())
		//Vista+
		return VWinD2DGraphicContext::RemoveShared(inUserHandle);
#endif
}



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
VGraphicContext* VGraphicContext::Create(HWND inHWND, const GReal inPageScale, const bool inTransparent, const GraphicContextType inType)
{
#if ENABLE_D2D
	if (IsD2DAvailable())
	{
		bool softwareOnly = !VGraphicContext::IsHardwareEnabled() || (inType == eSoftwareGraphicContext) || (inType == eWinD2DSoftwareGraphicContext);
		switch (inType)
		{
		case eDefaultGraphicContext:
		case eSoftwareGraphicContext:
		case eHardwareGraphicContext:
		case eWinD2DSoftwareGraphicContext:
		case eWinD2DHardwareGraphicContext:
			if (!softwareOnly || VSystem::IsSeven()) //fallback to GDIPlus on Vista if software only
			{
				VGraphicContext *gc = static_cast<VGraphicContext *>(new VWinD2DGraphicContext( inHWND, inPageScale, inTransparent));
				if (gc && softwareOnly)
					gc->SetSoftwareOnly(true);
				return gc;
			}
			else
				return static_cast<VGraphicContext *>(new VWinGDIPlusGraphicContext( inHWND));
			break;
		case eWinGDIPlusGraphicContext:
			return static_cast<VGraphicContext *>(new VWinGDIPlusGraphicContext( inHWND));
			break;
		case eWinGDIGraphicContext:
			return static_cast<VGraphicContext *>(new VWinGDIGraphicContext( inHWND));
			break;
		default:
			{
			xbox_assert(false);
			return static_cast<VGraphicContext *>(new VWinGDIGraphicContext( inHWND));
			}
			break;
		}
	}
#endif
	if (inType != eWinGDIGraphicContext)
		return static_cast<VGraphicContext *>(new VWinGDIPlusGraphicContext( inHWND));
	else
		return static_cast<VGraphicContext *>(new VWinGDIGraphicContext( inHWND));
}
#endif

/** create a bitmap graphic context 

	on Vista+, create a D2D WIC bitmap graphic context
	on XP, create a Gdiplus bitmap graphic context
	on Mac OS, create a Quartz2D bitmap graphic context
@remarks
	use method GetBitmap() to get bitmap source

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
#if VERSIONWIN
VGraphicContext *VGraphicContext::CreateBitmapGraphicContext( sLONG inWidth, sLONG inHeight, const GReal inPageScale, const bool inTransparent, const VPoint* inOrigViewport, bool inTransparentCompatibleGDI, const GraphicContextType inType)
#else
VGraphicContext *VGraphicContext::CreateBitmapGraphicContext( sLONG inWidth, sLONG inHeight, const GReal inPageScale, const bool inTransparent, const VPoint* inOrigViewport, bool, const GraphicContextType)
#endif
{
	if (inWidth == 0 || inHeight == 0 || inPageScale == 0.0f)
		return NULL;

#if VERSIONWIN
#if ENABLE_D2D
	if (IsD2DAvailable() && VSystem::IsSeven()) //D2D bitmap context is software only but D2D software is not available on Vista
	{
		//Seven+
		VPoint orig;
		if (inOrigViewport)
			orig = *inOrigViewport;

		switch (inType)
		{
		case eDefaultGraphicContext:
		case eSoftwareGraphicContext:
		case eHardwareGraphicContext:
		case eWinD2DSoftwareGraphicContext:
		case eWinD2DHardwareGraphicContext:
			return static_cast<VGraphicContext *>(new VWinD2DGraphicContext( inWidth, inHeight, inPageScale, orig, inTransparent, inTransparentCompatibleGDI));			
			break;
		case eWinGDIPlusGraphicContext:
			return static_cast<VGraphicContext *>(new VWinGDIPlusGraphicContext( inWidth, inHeight,inTransparent ? PixelFormat32bppARGB : PixelFormat32bppRGB ));
			break;
		case eWinGDIGraphicContext:
		default:
			{
			VWinGDIBitmapContext* gc = new VWinGDIBitmapContext(NULL);
			if (!gc->CreateBitmap(VRect(0,0,inWidth,inHeight)))
				ReleaseRefCountable(&gc);
			return static_cast<VGraphicContext *>(gc);
			}
			break;
		}
	}
#endif
	if (inType != eWinGDIGraphicContext)
		return static_cast<VGraphicContext *>(new VWinGDIPlusGraphicContext( inWidth, inHeight,inTransparent ? PixelFormat32bppARGB : PixelFormat32bppRGB ));
	else
	{
		VWinGDIBitmapContext* gc = new VWinGDIBitmapContext(NULL);
		if (!gc->CreateBitmap(VRect(0,0,inWidth,inHeight)))
			ReleaseRefCountable(&gc);
		return static_cast<VGraphicContext *>(gc);
	}
#elif VERSIONMAC
	return static_cast<VGraphicContext *>(new VMacQuartzBitmapContext( VRect(0, 0, inWidth, inHeight), true,inTransparent));
#else
	return NULL;
#endif
}


/** return graphic context type */
GraphicContextType	VGraphicContext::GetGraphicContextType() const
{
#if VERSIONWIN
	if (IsD2DImpl())
	{
		if (IsHardware())
			return eWinD2DHardwareGraphicContext;
		else
			return eWinD2DSoftwareGraphicContext;
	}
	else if (IsGdiPlusImpl())
		return eWinGDIPlusGraphicContext;
	else
		return eWinGDIGraphicContext;
#elif VERSIONMAC
	return eMacQuartz2DGraphicContext;
#else
	return eSoftwareGraphicContext;
#endif
}

/** return true if current gc is compatible with the passed gc type 
@param inType
	gc type
@param inIgnoreHardware
	true: ignore hardware status (software and hardware gc are compatible if they have the same impl)
	false: take account hardware status (will return false if gc has not the same hardware status as the passed gc type)
*/
bool VGraphicContext::IsCompatible( GraphicContextType inType, bool inIgnoreHardware)
{
	GraphicContextType thisType = GetGraphicContextType();
	if (thisType == inType)
		return true;
#if VERSIONWIN
	if (!inIgnoreHardware)
		return false;
	if (IsD2DImpl() 
		&&
		(inType == eWinD2DSoftwareGraphicContext || inType == eWinD2DHardwareGraphicContext)
		)
		return true;
#endif
	return false;
}

/** create and return gc compatible with this gc but used only for metrics */
VGraphicContext* VGraphicContext::CreateCompatibleForComputingMetrics() const
{
#if VERSIONWIN
	if (IsD2DImpl())
		return CreateForComputingMetrics( eWinD2DSoftwareGraphicContext); 
	else if (IsGdiPlusImpl())
		return CreateForComputingMetrics( eWinGDIPlusGraphicContext);
	else 
		return CreateForComputingMetrics( eWinGDIGraphicContext);
#else
	return CreateForComputingMetrics();
#endif
}


/** create a bitmap context for the specified type which is used only for computing metrics (i.e. not for drawing or blitting) 
@remarks
	returned graphic context is shared & stored in a internal static global table

	method is thread-safe
*/
VGraphicContext* VGraphicContext::CreateForComputingMetrics(const GraphicContextType inType)
{
	if (sMapOfComputingGC)
		return sMapOfComputingGC->Create( inType);
	else
		return NULL;
}

/** clear the shared bitmap context for metrics associated with the specified graphic context type 

	method is thread-safe
*/
void VGraphicContext::ClearForComputingMetrics(const GraphicContextType inType)
{
	if (sMapOfComputingGC)
		sMapOfComputingGC->Clear( inType);
}

/** clear all shared bitmap context for metrics 
	
	method is thread-safe
*/
void VGraphicContext::ClearAllForComputingMetrics()
{
	if (sMapOfComputingGC)
		sMapOfComputingGC->ClearAll();
}


/** create a bitmap context for the specified type which is used only for metrics (i.e. not for drawing) 
@remarks
	returned graphic context is shared & stored in a internal static global table

	it is caller responsibility to ensure returned gc is used tread-safe
	(as only GUI component & GUI thread should use VGraphicContext, it should not be a problem)
*/
VGraphicContext* VMapOfComputingGC::Create(const GraphicContextType inType)
{
	VTaskLock protect(&fMutex);

#if VERSIONWIN //only on Windows we might use different kind of gc
	sLONG type = (sLONG)inType;
#else
	sLONG type = eDefaultGraphicContext;
#endif

	bool addToMap = true;
	MapOfGCPerType::const_iterator it = fMapOfComputingGC.find(type);
	if (it != fMapOfComputingGC.end())
	{
		addToMap = false;
		if (it->second->GetRefCount() == 1) //only referenced by internal table
			return RetainRefCountable( it->second.Get());
	}

	VGraphicContext *gc = VGraphicContext::CreateBitmapGraphicContext( 1, 1, 1.0f, false, NULL, true, (GraphicContextType)type);  
	if (gc && addToMap)
		fMapOfComputingGC[type] = VRefPtr<VGraphicContext>(gc);

	return gc;
}

/** clear the shared bitmap context for metrics associated with the specified graphic context type */
void VMapOfComputingGC::Clear(const GraphicContextType inType)
{
	VTaskLock protect(&fMutex);

#if VERSIONWIN //only on Windows we might use different kind of gc
	sLONG type = (sLONG)inType;
#else
	sLONG type = eDefaultGraphicContext;
#endif

	MapOfGCPerType::iterator it = fMapOfComputingGC.find(type);
	if (it != fMapOfComputingGC.end())
		fMapOfComputingGC.erase(it);
}

/** clear all shared bitmap context for metrics */
void VMapOfComputingGC::ClearAll()
{
	VTaskLock protect(&fMutex);
	fMapOfComputingGC.clear();
}


VGraphicContext::VGraphicContext():VObject(),IRefCountable()
{
	fMaxPerfFlag = false;
	fFillRule = FILLRULE_EVENODD;
	fCharKerning = (GReal) 0.0;
	fCharSpacing = (GReal) 0.0;
	fUseShapeBrushForText = false;
	fShapeCrispEdgesEnabled = false;
	fAllowCrispEdgesOnBezier = false;
	fAllowCrispEdgesOnPath = false;
	fIsHighQualityAntialiased = true;
	fHighQualityTextRenderingMode = TRM_NORMAL;
	fPrinterScale=false;
	fPrinterScaleX=1.0;
	fPrinterScaleY=1.0;
	fShouldScaleShapesWithPrinterScale = true;
	fHairline=false;

#if VERSIONWIN
#if ENABLE_D2D
	fD2DCurGC = NULL;
	fD2DParentHDC = NULL;
	fD2DUseCount = 0;
#endif
	fGDIPlusGC = NULL;
#endif
}


VGraphicContext::VGraphicContext(const VGraphicContext& inOriginal):VObject(),IRefCountable()
{
	fMaxPerfFlag = inOriginal.fMaxPerfFlag;
	fFillRule = inOriginal.fFillRule;
	fCharKerning = inOriginal.fCharKerning;
	fCharSpacing = inOriginal.fCharSpacing;
	fUseShapeBrushForText = inOriginal.fUseShapeBrushForText;
	fShapeCrispEdgesEnabled = inOriginal.fShapeCrispEdgesEnabled;
	fAllowCrispEdgesOnBezier = inOriginal.fAllowCrispEdgesOnBezier;
	fAllowCrispEdgesOnPath = inOriginal.fAllowCrispEdgesOnPath;
	fIsHighQualityAntialiased = inOriginal.fIsHighQualityAntialiased;
	fHighQualityTextRenderingMode = inOriginal.fHighQualityTextRenderingMode;
	fPrinterScale=inOriginal.fPrinterScale;
	fPrinterScaleX=inOriginal.fPrinterScaleX;
	fPrinterScaleY=inOriginal.fPrinterScaleY;
	fHairline=inOriginal.fHairline;
	fShouldScaleShapesWithPrinterScale = inOriginal.fShouldScaleShapesWithPrinterScale;

#if VERSIONWIN
#if ENABLE_D2D
	fD2DCurGC = NULL;
	fD2DParentHDC = NULL;
	fD2DUseCount = 0;
#endif
	fGDIPlusGC = NULL;
#endif
}


VGraphicContext::~VGraphicContext()
{
#if VERSIONWIN
	xbox_assert(fGDIPlusGC == NULL);
#if ENABLE_D2D
	xbox_assert(fD2DCurGC == NULL);
#endif
#endif
}


VGraphicPath *VGraphicContext::CreatePath(bool inComputeBoundsAccurate) const
{	
	return new VGraphicPath(inComputeBoundsAccurate, fAllowCrispEdgesOnPath); 
}

TextRenderingMode VGraphicContext::SetTextRenderingMode( TextRenderingMode inMode)
{
	return TRM_NORMAL;
}


TextRenderingMode  VGraphicContext::GetTextRenderingMode() const
{
	return TRM_NORMAL;
}
void	VGraphicContext::SetFillStdPattern (PatternStyle inStdPattern, VBrushFlags* ioFlags)
{
	VPattern *pattern = VPattern::RetainStdPattern(inStdPattern);
	if(pattern)
	{
		SetFillPattern(pattern,ioFlags);
		pattern->Release();
	}
}
void	VGraphicContext::SetLineStdPattern (PatternStyle inStdPattern, VBrushFlags* ioFlags) 
{
	VPattern *pattern = VPattern::RetainStdPattern(inStdPattern);
	if(pattern)
	{
		SetLinePattern(pattern,ioFlags);
		pattern->Release();
	}
}

bool VGraphicContext::SetHairline(bool inSet)
{
	bool result=fHairline;
	fHairline = inSet;
	if(inSet)
	{
		SetLineWidth(1);
	}
	else
	{
		if(result!=inSet)
		{
			SetLineWidth(GetLineWidth());
		}
	}
	
	return result;
}


#if VERSIONMAC
void VGraphicContext::RevealUpdate(WindowRef inWindow)
{
	VMacQuartzGraphicContext::RevealUpdate(inWindow);
}
#elif VERSIONWIN
void VGraphicContext::RevealUpdate(HWND inWindow)
{
	VWinGDIPlusGraphicContext::RevealUpdate(inWindow);
}
#endif

// This is a utility function used when debugging clipping
// mecanism. It try to remain as independant as possible
// from the framework. Use with care.
//
void VGraphicContext::RevealClipping(ContextRef inContext)
{
#if VERSIONMAC
	VMacQuartzGraphicContext::RevealClipping(inContext);
#else
//	VWinGDIGraphicContext::RevealClipping(inContext);
	VWinGDIPlusGraphicContext::RevealClipping(inContext);
#endif
}


// This is a utility function used when debugging update
// mecanism. It try to remain as independant as possible
// from the framework. Use with care.
//
void VGraphicContext::RevealBlitting(ContextRef inContext, const RgnRef inRegion)
{
#if VERSIONMAC
	VMacQuartzGraphicContext::RevealBlitting(inContext, inRegion);
#else
//	VWinGDIGraphicContext::RevealBlitting(inContext, inRegion);
	VWinGDIPlusGraphicContext::RevealBlitting(inContext, inRegion);
#endif
}


// This is a utility function used when debugging update
// mecanism. It try to remain as independant as possible
// from the framework. Use with care.
//
void VGraphicContext::RevealInval(ContextRef inContext, const RgnRef inRegion)
{
#if VERSIONMAC
	VMacQuartzGraphicContext::RevealInval(inContext, inRegion);
#else
//	VWinGDIGraphicContext::RevealInval(inContext, inRegion);
	VWinGDIPlusGraphicContext::RevealInval(inContext, inRegion);
#endif
}


sWORD VGraphicContext::GetRowBytes(sWORD inWidth, sBYTE inDepth, sBYTE inPadding)
{
	sWORD	result = 0;
	
	switch (inDepth)
	{
		case 1:
			result = (inWidth + 7) >> 3;
			break;
			
		case 2:
			result = (inWidth + 3) >> 2;
			break;
			
		case 4:
			result = (inWidth + 1) >> 1;
			break;
			
		case 8:
			result = inWidth;
			break;
			
		case 16:
			result = inWidth << 1;
			break;
			
		case 24:
			result = inWidth * 3;
			break;
			
		case 32:
			result = inWidth << 2;
			break;
		
		default:
			assert(false);
			break;
	}
	
	switch (inPadding)
	{
		case 2:
			result = (result + 1) & 0xFFFE;
			break;
			
		case 4:
			result = (result + 3) & 0xFFFC;
			break;
			
		case 8:
			result = (result + 7) & 0xFFF8;
			break;
			
		case 16:
			result = (result + 15) & 0xFFF0;
			break;
		
		default:
			assert(false);
			break;
	}
	
	return result;
}


sBYTE* VGraphicContext::RotatePixels(GReal inRadian, sBYTE* ioBits, sWORD inRowBytes, sBYTE inDepth, sBYTE inPadding, const VRect& inSrcBounds, VRect& outDestBounds, uBYTE inFillByte)
{
	sWORD	srcleft = (sWORD)inSrcBounds.GetLeft();
	sWORD	srctop = (sWORD)inSrcBounds.GetTop();
	sWORD	srcwidth = (sWORD)inSrcBounds.GetWidth();
	sWORD	srcheight = (sWORD)inSrcBounds.GetHeight();
	sLONG	rotsize;
	GReal	rotwidth;
	GReal 	anglecos;
	GReal 	anglesin;
	GReal	temp;
	GReal	runh,runv,riseh,risev,rotx,roty,currotx,curroty; 
	sWORD	rotrowbytes;
	sBYTE*	rotbits;
	sBYTE*	maxrotbits;
	sBYTE*	cursrcbits;
	sBYTE*	currotbits;
	sWORD	rotrow,rotcol,cursrcrow,srcrow,srccol;	
	sWORD	srcright,srcbottom,dstleft,dstright,dsttop,dstbottom,dstwidth,dstheight,invleft,invright,invtop,invbottom,invwidth,invheight;
	sWORD	calcx1,calcx2,calcx3,calcx4,calcy1,calcy2,calcy3,calcy4,minx,miny,maxx,maxy,rothoff,rotvoff;
	sBYTE*	srcbyte;
	sBYTE*	dstbyte;
	sBYTE	srcpix;
	sWORD	stdangle;

	// first calculate dest rect
	inRadian = (GReal) ::fmod(inRadian, ((GReal) 2.0 * (GReal) PI));
	anglecos = (GReal) ::cos(inRadian);
	anglesin = (GReal) ::sin(inRadian);
	// because y axis is inverted use inverted y values and invert y results
	// calculate dest pos of top left corner
	srcright = srcleft + srcwidth;
	srcbottom = srctop + srcheight;
	temp = srcleft*anglecos - (-srctop)*anglesin;
	calcx1 = (sWORD)(temp + ((temp<0)?-0.5:0.5));
	temp = srcleft*anglesin + (-srctop)*anglecos;
	calcy1 = (sWORD)(temp + ((temp<0)?-0.5:0.5));
	minx = calcx1;
	maxx = calcx1;
	miny = calcy1;
	maxy = calcy1;
	// calculate dest pos of top right corner
	temp = (GReal)srcright*anglecos - (GReal)(-srctop)*anglesin;
	calcx2 = (sWORD)(temp + ((temp<0)?-0.5:0.5));
	temp = (GReal)srcright*anglesin + (GReal)(-srctop)*anglecos;
	calcy2 = (sWORD)(temp + ((temp<0)?-0.5:0.5));
	minx = Min(minx,calcx2);
	maxx = Max(maxx,calcx2);
	miny = Min(miny,calcy2);
	maxy = Max(miny,calcy2);
	// calculate dest pos of bottom left corner
	temp = (GReal)srcleft*anglecos - (GReal)(-srcbottom)*anglesin;
	calcx3 = (sWORD)(temp + ((temp<0)?-0.5:0.5));
	temp = (GReal)srcleft*anglesin + (GReal)(-srcbottom)*anglecos;
	calcy3 = (sWORD)(temp + ((temp<0)?-0.5:0.5));
	minx = Min(minx,calcx3);
	maxx = Max(maxx,calcx3);
	miny = Min(miny,calcy3);
	maxy = Max(miny,calcy3);
	// calculate dest pos of bottom right corner
	temp = (GReal)srcright*anglecos - (GReal)(-srcbottom)*anglesin;
	calcx4 = (sWORD)(temp + ((temp<0)?-0.5:0.5));
	temp = (GReal)srcright*anglesin + (GReal)(-srcbottom)*anglecos;
	calcy4 = (sWORD)(temp + ((temp<0)?-0.5:0.5));
	// now we have the dest rect
	dstleft = Min(minx,calcx4);
	dstright = Max(maxx,calcx4);
	// invert y results
	dsttop = -Max(maxy,calcy4); 
	dstbottom = -Min(miny,calcy4); 
	// update dest dimensions
	rotwidth = dstright-dstleft;
	outDestBounds.SetCoords(dstleft, dsttop, dstright-dstleft, dstbottom-dsttop);

	// now let's act as if the rotation center was the center of the source bitmap
	srcright = srcwidth>>1;
	srcleft = srcright-srcwidth;
	srcbottom = srcheight>>1;
	srctop = srcbottom-srcheight;
	
	if (Abs(inRadian - (PI / 2.0)) < 0.01) stdangle = 90;
	else if (Abs(inRadian - PI) < 0.01) stdangle = 180;
	else if (Abs(inRadian - (PI * 3.0 / 2.0)) < 0.01) stdangle = 270;
	else stdangle = 0;
	
	switch (stdangle)
	{
		case 90:
			dstleft = srctop;
			dstright = srcbottom;
			dsttop = -srcright;
			dstbottom = -srcleft;
			break;
		
		case 180:
			dstleft = srcleft;
			dstright = srcright;
			dsttop = srctop;
			dstbottom = srcbottom;
			break;
		
		case 270:
			dstleft = -srcbottom;
			dstright = -srctop;
			dsttop = srcleft;
			dstbottom = srcright;
			break;
		
		default:
			temp = (GReal)srcleft*anglecos - (GReal)(-srctop)*anglesin;
			calcx1 = (sWORD)(temp + ((temp<0)?-0.5:0.5));
			temp = (GReal)srcleft*anglesin + (GReal)(-srctop)*anglecos;
			calcy1 = (sWORD)(temp + ((temp<0)?-0.5:0.5));
			minx = calcx1;
			maxx = calcx1;
			miny = calcy1;
			maxy = calcy1;
			// calculate dest pos of top right corner
			temp = (GReal)srcright*anglecos - (GReal)(-srctop)*anglesin;
			calcx2 = (sWORD)(temp + ((temp<0)?-0.5:0.5));
			temp = (GReal)srcright*anglesin + (GReal)(-srctop)*anglecos;
			calcy2 = (sWORD)(temp + ((temp<0)?-0.5:0.5));
			minx = Min(minx,calcx2);
			maxx = Max(maxx,calcx2);
			miny = Min(miny,calcy2);
			maxy = Max(miny,calcy2);
			// calculate dest pos of bottom left corner
			temp = (GReal)srcleft*anglecos - (GReal)(-srcbottom)*anglesin;
			calcx3 = (sWORD)(temp + ((temp<0)?-0.5:0.5));
			temp = (GReal)srcleft*anglesin + (GReal)(-srcbottom)*anglecos;
			calcy3 = (sWORD)(temp + ((temp<0)?-0.5:0.5));
			minx = Min(minx,calcx3);
			maxx = Max(maxx,calcx3);
			miny = Min(miny,calcy3);
			maxy = Max(miny,calcy3);
			// calculate dest pos of bottom right corner
			temp = (GReal)srcright*anglecos - (GReal)(-srcbottom)*anglesin;
			calcx4 = (sWORD)(temp + ((temp<0)?-0.5:0.5));
			temp = (GReal)srcright*anglesin + (GReal)(-srcbottom)*anglecos;
			calcy4 = (sWORD)(temp + ((temp<0)?-0.5:0.5));
			// now we have the dest rect
			dstleft = Min(minx,calcx4);
			dstright = Max(maxx,calcx4);
			// invert y results
			dsttop = -Max(maxy,calcy4); 
			dstbottom = -Min(miny,calcy4); 
			break;
	}
	
	dstwidth = dstright-dstleft;
	dstheight = dstbottom-dsttop;
	// make dest bitmap	
	rotrowbytes = GetRowBytes(dstwidth, inDepth, inPadding);
	rotsize = dstheight * rotrowbytes;
	rotbits = (sBYTE*) VMemory::NewPtrClear(rotsize, 'rotb');
	if(!rotbits) return 0;  
	// update rotated pixels	
	maxrotbits = rotbits + rotsize;
	currotbits = rotbits;
	// start from top of rot pixmap
	rotrow = 0;
	
	switch (stdangle)
	{
		case 90:
			while(rotrow < dstheight)
			{
				cursrcbits = ioBits;
				for (rotcol = 0; rotcol < rotwidth; rotcol++)
				{
					srccol = dstheight-1-rotrow;
					switch(inDepth)
					{
					case 1:
						srcbyte = cursrcbits+(srccol>>3);
						srcpix = ((*srcbyte)>>((7-(srccol&0x00000007)))) & 0x01;
						#if VERSIONMAC // bw bitmaps are inverted on windows
						if(srcpix) 
						{
							dstbyte = currotbits+(rotcol>>3); // 8 bits per pixel
							*dstbyte |= (0x80>>(rotcol&0x00000007)); // set the pixel
						}
						#else
						if(!srcpix) 
						{
							dstbyte = currotbits+(rotcol>>3); // 8 bits per pixel
							*dstbyte &= ~(0x80>>(rotcol&0x00000007)); // clear the pixel
						}
						#endif
						break;
					case 2:
						srcbyte = cursrcbits+(srccol>>2); // 2 bits per pixel
						srcpix = ((*srcbyte)>>((srccol&0x00000003)<<1)) & 0x03;
						dstbyte = currotbits+(rotcol>>2); // 2 bits per pixel
						*dstbyte &= ~(0xC0>>((rotcol&0x00000003)<<1)); // clear the pixel
						if(srcpix) *dstbyte |= (srcpix<<(3-((rotcol&0x00000003)<<1))); // set it if needed
						break;
					case 4:
						srcbyte = cursrcbits+(srccol>>1); // 4 bits per pixel
						srcpix = ((*srcbyte)>>(srccol&0x00000001)) & 0x0F;
						dstbyte = currotbits+(rotcol>>1); // 4 bits per pixel
						*dstbyte &= ~(0xF0>>((rotcol&0x00000001)<<2)); // clear the pixel
						if(srcpix) *dstbyte |= (srcpix<<((rotcol&0x00000001)<<2)); // set it if needed
						break;
					case 8:
						*(currotbits+rotcol) = *(cursrcbits+srccol);
						break;
					case 16:
						*(sWORD*)(currotbits+(rotcol<<1)) = *(sWORD*)(cursrcbits+(srccol<<1));
						break;
					case 24:
						::CopyBlock(cursrcbits+(srccol*3),currotbits+(rotcol*3),3);
						break;
					case 32:
						*(sLONG*)(currotbits+(rotcol<<2)) = *(sLONG*)(cursrcbits+(srccol<<2));
						break;
					}
					cursrcbits += inRowBytes;
				}
				rotrow += 1;
				currotbits += rotrowbytes;
			}
			break;
		
		case 180:
			while (rotrow < dstheight)
			{
				cursrcbits = ioBits + ((srcheight-1)*inRowBytes);
				for (rotcol = 0; rotcol < rotwidth; rotcol++)
				{
					srccol = srcwidth - rotcol - 1;
					switch (inDepth)
					{
					case 1:
						srcbyte = cursrcbits+(srccol>>3);
						srcpix = ((*srcbyte)>>((7-(srccol&0x00000007)))) & 0x01;
					#if VERSIONMAC // bw bitmaps are inverted on windows
						if(srcpix) 
						{
							dstbyte = currotbits+(rotcol>>3); // 8 bits per pixel
							*dstbyte |= (0x80>>(rotcol&0x00000007)); // set the pixel
						}
					#else
						if(!srcpix) 
						{
							dstbyte = currotbits+(rotcol>>3); // 8 bits per pixel
							*dstbyte &= ~(0x80>>(rotcol&0x00000007)); // clear the pixel
						}
					#endif
						break;
						
					case 2:
						srcbyte = cursrcbits+(srccol>>2); // 2 bits per pixel
						srcpix = ((*srcbyte)>>((srccol&0x00000003)<<1)) & 0x03;
						dstbyte = currotbits+(rotcol>>2); // 2 bits per pixel
						*dstbyte &= ~(0xC0>>((rotcol&0x00000003)<<1)); // clear the pixel
						if(srcpix) *dstbyte |= (srcpix<<(3-((rotcol&0x00000003)<<1))); // set it if needed
						break;
						
					case 4:
						srcbyte = cursrcbits+(srccol>>1); // 4 bits per pixel
						srcpix = ((*srcbyte)>>(srccol&0x00000001)) & 0x0F;
						dstbyte = currotbits+(rotcol>>1); // 4 bits per pixel
						*dstbyte &= ~(0xF0>>((rotcol&0x00000001)<<2)); // clear the pixel
						if(srcpix) *dstbyte |= (srcpix<<((rotcol&0x00000001)<<2)); // set it if needed
						break;
						
					case 8:
						*(currotbits+rotcol) = *(cursrcbits+srccol);
						break;
						
					case 16:
						*(sWORD*)(currotbits+(rotcol<<1)) = *(sWORD*)(cursrcbits+(srccol<<1));
						break;
						
					case 24:
						::CopyBlock(cursrcbits+(srccol*3),currotbits+(rotcol*3),3);
						break;
						
					case 32:
						*(sLONG*)(currotbits+(rotcol<<2)) = *(sLONG*)(cursrcbits+(srccol<<2));
						break;
					}
					cursrcbits -= inRowBytes;
				}
				rotrow += 1;
				currotbits += rotrowbytes;
				cursrcbits -= inRowBytes;
			}
			break;
		
		case 270:
			while(rotrow<dstheight)
			{
				cursrcbits = ioBits + ((srcheight-1)*inRowBytes);
				for (rotcol = 0; rotcol < rotwidth; rotcol++)
				{
					srccol = rotrow;
					switch(inDepth)
					{
					case 1:
						srcbyte = cursrcbits+(srccol>>3);
						srcpix = ((*srcbyte)>>((7-(srccol&0x00000007)))) & 0x01;
					#if VERSIONMAC // bw bitmaps are inverted on windows
						if(srcpix) 
						{
							dstbyte = currotbits+(rotcol>>3); // 8 bits per pixel
							*dstbyte |= (0x80>>(rotcol&0x00000007)); // set the pixel
						}
					#else
						if(!srcpix) 
						{
							dstbyte = currotbits+(rotcol>>3); // 8 bits per pixel
							*dstbyte &= ~(0x80>>(rotcol&0x00000007)); // clear the pixel
						}
					#endif
						break;
						
					case 2:
						srcbyte = cursrcbits+(srccol>>2); // 2 bits per pixel
						srcpix = ((*srcbyte)>>((srccol&0x00000003)<<1)) & 0x03;
						dstbyte = currotbits+(rotcol>>2); // 2 bits per pixel
						*dstbyte &= ~(0xC0>>((rotcol&0x00000003)<<1)); // clear the pixel
						if (srcpix) *dstbyte |= (srcpix<<(3-((rotcol&0x00000003)<<1))); // set it if needed
						break;
						
					case 4:
						srcbyte = cursrcbits+(srccol>>1); // 4 bits per pixel
						srcpix = ((*srcbyte)>>(srccol&0x00000001)) & 0x0F;
						dstbyte = currotbits+(rotcol>>1); // 4 bits per pixel
						*dstbyte &= ~(0xF0>>((rotcol&0x00000001)<<2)); // clear the pixel
						if (srcpix) *dstbyte |= (srcpix<<((rotcol&0x00000001)<<2)); // set it if needed
						break;
						
					case 8:
						*(currotbits+rotcol) = *(cursrcbits+srccol);
						break;
						
					case 16:
						*(sWORD*)(currotbits+(rotcol<<1)) = *(sWORD*)(cursrcbits+(srccol<<1));
						break;
						
					case 24:
						::CopyBlock(cursrcbits+(srccol*3),currotbits+(rotcol*3),3);
						break;
						
					case 32:
						*(sLONG*)(currotbits+(rotcol<<2)) = *(sLONG*)(cursrcbits+(srccol<<2));
						break;
					}
					cursrcbits += inRowBytes;
				}
				rotrow += 1;
				currotbits += rotrowbytes;
			}
			break;
		
		default:
			// fill in the pixels with white
			while (currotbits<maxrotbits) *currotbits++ = inFillByte;
			
			currotbits = rotbits; 
			// in order to fill in the destination bitmap, we must scan all dest lines and cols
			// so get values for inverted rotation
			anglecos = (GReal) ::cos(-inRadian);
			anglesin = (GReal) ::sin(-inRadian);
			// because y axis is inverted always use inverted y values
			// calculate dest pos of top left corner
			temp = (GReal)dstleft*anglecos - (GReal)(-dsttop)*anglesin;
			calcx1 = (sWORD)(temp + ((temp<0)?-0.5:0.5));
			temp = (GReal)dstleft*anglesin + (GReal)(-dsttop)*anglecos;
			calcy1 = (sWORD)(temp + ((temp<0)?-0.5:0.5));
			minx = calcx1;
			maxx = calcx1;
			miny = calcy1;
			maxy = calcy1;
			// calculate dest pos of top right corner
			temp = (GReal)dstright*anglecos - (GReal)(-dsttop)*anglesin;
			calcx2 = (sWORD)(temp + ((temp<0)?-0.5:0.5));
			temp = (GReal)dstright*anglesin + (GReal)(-dsttop)*anglecos;
			calcy2 = (sWORD)(temp + ((temp<0)?-0.5:0.5));
			minx = Min(minx,calcx2);
			maxx = Max(maxx,calcx2);
			miny = Min(miny,calcy2);
			maxy = Max(miny,calcy2);
			// calculate dest pos of bottom left corner
			temp = (GReal)dstleft*anglecos - (GReal)(-dstbottom)*anglesin;
			calcx3 = (sWORD)(temp + ((temp<0)?-0.5:0.5));
			temp = (GReal)dstleft*anglesin + (GReal)(-dstbottom)*anglecos;
			calcy3 = (sWORD)(temp + ((temp<0)?-0.5:0.5));
			minx = Min(minx,calcx3);
			maxx = Max(maxx,calcx3);
			miny = Min(miny,calcy3);
			maxy = Max(miny,calcy3);
			// calculate dest pos of bottom right corner
			temp = (GReal)dstright*anglecos - (GReal)(-dstbottom)*anglesin;
			calcx4 = (sWORD)(temp + ((temp<0)?-0.5:0.5));
			temp = (GReal)dstright*anglesin + (GReal)(-dstbottom)*anglecos;
			calcy4 = (sWORD)(temp + ((temp<0)?-0.5:0.5));
			// now we have the invert rect
			invleft = Min(minx,calcx4);
			invright = Max(maxx,calcx4);
			// invert y results
			invtop = -Max(maxy,calcy4); 
			invbottom = -Min(miny,calcy4); 
			invwidth = invright - invleft;
			invheight = invbottom - invtop;
			// calculate reference point in mac Fixed types
			rotx = calcx1+((minx > 0) ? minx : -minx);
			roty = calcy1-((maxy > 0) ? maxy : -maxy);
			// calculate increments in mac Fixed types
			runh = (GReal)(calcx2-calcx1)/dstwidth; // when increasing horizontally 1 dest pixel, how much to increase horizontally the src pixel
			riseh = (GReal)(calcy2-calcy1)/dstwidth; // when increasing horizontally 1 dest pixel, how much to increase vertically the src pixel
			runv = (GReal)(calcx3-calcx1)/dstheight; // when increasing vertically 1 dest pixel, how much to increase horizontally the src pixel
			risev = (GReal)(calcy3-calcy1)/dstheight; // when increasing vertically 1 dest pixel, how much to increase vertically the src pixel
			// calc offset because bitmaps don't have the same origin
			rothoff = 1+((invwidth-srcwidth)>>1);
			rotvoff = 1+((invheight-srcheight)>>1);
			// and go
			cursrcrow = -1;
			while (rotrow < dstheight)
			{
				currotx = rotx;
				curroty = roty;
				for (rotcol = 0; rotcol < rotwidth; rotcol++)
				{
					if (curroty >= 0)
						srcrow = (sWORD)(-(curroty + 0.5) - rotvoff); 
					else
						srcrow = (sWORD)(-(curroty - 0.5) - rotvoff);
						
					if (srcrow >= 0 && srcrow < srcheight)
					{
						if (currotx >= 0)
							srccol = (sWORD)((currotx + 0.5) - rothoff);
						else
							srccol = (sWORD)((currotx - 0.5) - rothoff);
							
						if (srccol >= 0 && srccol < srcwidth)
						{
							if (srcrow != cursrcrow)
							{
								cursrcrow = srcrow;
								cursrcbits = ioBits + (cursrcrow*inRowBytes);
							}
							
							switch (inDepth)
							{
								case 1:
									srcbyte = cursrcbits+(srccol>>3);
									srcpix = ((*srcbyte)>>((7-(srccol&0x00000007)))) & 0x01;
								#if VERSIONMAC // bw bitmaps are inverted on windows
									if(srcpix) 
									{
										dstbyte = currotbits+(rotcol>>3); // 8 bits per pixel
										*dstbyte |= (0x80>>(rotcol&0x00000007)); // set the pixel
									}
								#else
									if(!srcpix) 
									{
										dstbyte = currotbits+(rotcol>>3); // 8 bits per pixel
										*dstbyte &= ~(0x80>>(rotcol&0x00000007)); // clear the pixel
									}
								#endif
									break;
									
								case 2:
									srcbyte = cursrcbits+(srccol>>2); // 2 bits per pixel
									srcpix = ((*srcbyte)>>((srccol&0x00000003)<<1)) & 0x03;
									dstbyte = currotbits+(rotcol>>2); // 2 bits per pixel
									*dstbyte &= ~(0xC0>>((rotcol&0x00000003)<<1)); // clear the pixel
									if(srcpix) *dstbyte |= (srcpix<<(3-((rotcol&0x00000003)<<1))); // set it if needed
									break;
									
								case 4:
									srcbyte = cursrcbits+(srccol>>1); // 4 bits per pixel
									srcpix = ((*srcbyte)>>(srccol&0x00000001)) & 0x0F;
									dstbyte = currotbits+(rotcol>>1); // 4 bits per pixel
									*dstbyte &= ~(0xF0>>((rotcol&0x00000001)<<2)); // clear the pixel
									if(srcpix) *dstbyte |= (srcpix<<((rotcol&0x00000001)<<2)); // set it if needed
									break;
									
								case 8:
									*(currotbits+rotcol) = *(cursrcbits+srccol);
									break;
									
								case 16:
									*(sWORD*)(currotbits+(rotcol<<1)) = *(sWORD*)(cursrcbits+(srccol<<1));
									break;
									
								case 24:
									::CopyBlock(cursrcbits+(srccol*3),currotbits+(rotcol*3),3);
									break;
									
								case 32:
									*(sLONG*)(currotbits+(rotcol<<2)) = *(sLONG*)(cursrcbits+(srccol<<2));
									break;
							}
						}
					}
					currotx += runh;
					curroty += riseh;
				}
				
				rotrow += 1;
				rotx += runv;
				roty += risev;
				currotbits += rotrowbytes;
			}
			break;
	}
	
	return rotbits;
}

void VGraphicContext::GetTextBounds( const VString& inString, VRect& oRect) const
{
	oRect = VRect(0, 0, 0, 0);
}



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
//	this method return the text bounds without the extra spacing 
//	due to layout formatting as with GetTextBounds
//@note
//	this method is used by SVG component
void VGraphicContext::GetTextBoundsTypographic( const VString& inString, VRect& oRect, TextLayoutMode inLayoutMode) const
{
	GetTextBounds( inString, oRect);
}

void VGraphicContext::DrawRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight)
{
	VGraphicPath pth;
	_BuildRoundRectPath(inHwndRect,inOvalWidth,inOvalHeight,pth);
	if (fShapeCrispEdgesEnabled)
	{
		//crispEdges has already been applied in _BuildRoundRectPath 
		fShapeCrispEdgesEnabled = false;
		DrawPath(pth);
		fShapeCrispEdgesEnabled = true;
	}
	else
		DrawPath(pth);
}
void VGraphicContext::FillRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight)
{
	VGraphicPath pth;
	_BuildRoundRectPath(inHwndRect,inOvalWidth,inOvalHeight,pth,true);
	if (fShapeCrispEdgesEnabled)
	{
		//crispEdges has already been applied in _BuildRoundRectPath 
		fShapeCrispEdgesEnabled = false;
		FillPath(pth);
		fShapeCrispEdgesEnabled = true;
	}
	else
		FillPath(pth);
}
void VGraphicContext::FrameRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight)
{
	VGraphicPath pth;
	_BuildRoundRectPath(inHwndRect,inOvalWidth,inOvalHeight,pth);
	if (fShapeCrispEdgesEnabled)
	{
		//crispEdges has already been applied in _BuildRoundRectPath 
		fShapeCrispEdgesEnabled = false;
		FramePath(pth);
		fShapeCrispEdgesEnabled = true;
	}
	else
		FramePath(pth);
}
void VGraphicContext::_BuildRoundRectPath(const VRect _inBounds,GReal inOvalWidth,GReal inOvalHeight,VGraphicPath& outPath, bool inFillOnly)
{
	VRect inBounds(_inBounds);
	if (fShapeCrispEdgesEnabled)
		CEAdjustRectInTransformedSpace(inBounds, inFillOnly);

	bool hline=true,vline=true;
	XBOX::VPoint cp1,cp2;
	GReal top,left,bottom,right,height,width;
	left=inBounds.GetX();
	right=inBounds.GetRight();
	top=inBounds.GetY();
	bottom=inBounds.GetBottom();
	const GReal	cpRatio = (GReal) 0.50;
	outPath.Clear();
 
	width = inOvalWidth/2;
	height = inOvalHeight/2;
 
	if(width*2>inBounds.GetWidth())
	{
		width=inBounds.GetWidth()/2;
		hline=false;
	}
	if(height*2>inBounds.GetHeight())
	{
		height=inBounds.GetHeight()/2;
		vline=false;
	}
 
	outPath.BeginSubPathAt(VPoint(left,bottom-height));
	cp1.SetPosTo(left, bottom - height + cpRatio * height);
	cp2.SetPosTo(left + width - cpRatio * width, bottom );
	outPath.AddBezierTo(cp1,VPoint(left + width, bottom),cp2);
	if(hline)
		outPath.AddLineTo(VPoint(right-width,bottom));
	cp1.SetPosTo(right- width + cpRatio * width, bottom);
	cp2.SetPosTo(right, bottom - height + cpRatio * height );
	outPath.AddBezierTo(cp1,VPoint(right, bottom - height),cp2);
	if(vline)
		outPath.AddLineTo(VPoint(right,top+height));
	cp1.SetPosTo(right, top + height - cpRatio * height);
	cp2.SetPosTo(right- width + cpRatio * width, top );
	outPath.AddBezierTo(cp1,VPoint(right - width, top),cp2);
	if(hline)
		outPath.AddLineTo(VPoint(left+width,top));
	cp1.SetPosTo(left + width - cpRatio * width, top);
	cp2.SetPosTo(left, top + height - cpRatio * height );
	outPath.AddBezierTo(cp1,VPoint(left, top + height),cp2);
	outPath.CloseSubPath();
	outPath.End();
}


/** adjust point coordinates in transformed space 
@remarks
	this method should only be called if fShapeCrispEdgesEnabled is equal to true
*/
void VGraphicContext::CEAdjustPointInTransformedSpace( VPoint& ioPos, bool inFillOnly, VPoint *outPosTransformed, GReal *outLineWidthTransformed)
{
#if VERSIONMAC
	UseReversedAxis();	//here in transformed space which is Quartz2D space, 
					//y axis is pointing up (on Windows impl, y axis is pointing down):
					//we need to take account y axis orientation while determining final pixel position
					//even if we use QD axis for input coordinates because we will modify
					//transformed coordinates which are bound to Quartz2D coordinate space
#endif

	VAffineTransform ctm;
	GetTransform( ctm);
	bool isIdentity = ctm.IsIdentity();

	//get transformed position 
	VPoint posTransformed;
	if (isIdentity)
		posTransformed = ioPos;
	else
		posTransformed = ctm.TransformPoint( ioPos);
	if (outPosTransformed)
		*outPosTransformed = posTransformed;

	GReal scaleX = fabs(ctm.GetScaleX());
	GReal scaleY = fabs(ctm.GetScaleY());
	GReal scaling = scaleX > scaleY ? scaleX : scaleY;
	GReal lineWidthTransformed =  GetLineWidth()*scaling;
	if (outLineWidthTransformed)
		*outLineWidthTransformed = lineWidthTransformed;

	//stick to rounded integer origin + 0.5 if tranformed line width is odd 
	//and rounded integer origin if transformed line width is even
	//(consistent with half pixel offset mode wich is used by all impls)
	//and take account impl y axis orientation to determine rounded value
	//so it is the same for all impls
#if VERSIONDEBUG
#if VERSIONWIN
	if (IsGdiPlusImpl())
	{
		//in order to be compliant with Quartz2D & Direct2D impl, 
		//Gdiplus pixel offset mode should always be PixelOffsetModeHalf (ie pixel center = 0.5,0.5)
		Gdiplus::Graphics *gcNative = reinterpret_cast<Gdiplus::Graphics *>(GetNativeRef());
		xbox_assert(gcNative->GetPixelOffsetMode() == Gdiplus::PixelOffsetModeHalf);
	}
#endif
#endif

	//adjust coords according to special frac value of 0.5f
	GReal x, y;
	x  = posTransformed.GetX();
	y  = posTransformed.GetY();

	//adjust coords according to pixel offset mode, vertical axis direction & line width
	if (!inFillOnly && (lineWidthTransformed <= 1.0f || (((int)floor(lineWidthTransformed+0.5f)) & 1)))
	{
		//line width is odd: stick to rounded integer origin + 0.5 or -0.5 depending if rounded value is lower or greater than the original value 
		
		GReal r = floor(x+0.5f);
		if (r > x)
			posTransformed.SetX( r-0.5f);
		else
			posTransformed.SetX( r+0.5f);

		r = floor(y+0.5f);
		if (r > y)
			posTransformed.SetY( r-0.5f);
		else
			posTransformed.SetY( r+0.5f);
	}
	else
	{
		//line width is even or we are filling: stick to rounded integer origin 

		//0.5 special case: ensure 0.5 will be sticked to 0 (or 1 depending on axis direction)

		GReal dx = x-floor(x);
		if (fabs(dx-0.5f) <= 0.01f)
			x  = floor(x);
		else
			x = floor(x+0.5f);
	#if VERSIONWIN
		GReal dy = y-floor(y);
		if (fabs(dy-0.5f) <= 0.01f)
			y  = floor(y);
		else
			y = floor(y+0.5f);
	#else
		//with Quartz2D, as transformed y axis is pointing up
		//we round to ceil(y) but floor(y) in order to map with same pixel value as with QD or GDI coord space
		//(only if frac value == 0.5)
		GReal dy = ceil(y)-y;
		if (fabs(dy-0.5f) <= 0.01f)
			y  = ceil(y);
		else
			y = floor(y+0.5f);
	#endif

		posTransformed.SetX( x);
		posTransformed.SetY( y);
	}

	//transformed back adjusted position to user space
	if (isIdentity)
		ioPos = posTransformed;
	else
		ioPos = ctm.Inverse().TransformPoint( posTransformed);
}


/** adjust rect coordinates in transformed space 
@remarks
	this method should only be called if fShapeCrispEdgesEnabled is equal to true
*/
void VGraphicContext::CEAdjustRectInTransformedSpace( VRect& ioRect, bool inFillOnly)
{
	//JQ 29/05/2013: reviewed in order contiguous rects do not overlap after coordinates have been adjusted

	//adjust first rect origin
	VPoint posTopLeft( ioRect.GetTopLeft());
	CEAdjustPointInTransformedSpace( posTopLeft, inFillOnly);

	VAffineTransform ctm;
	GetTransform( ctm);
	if (ctm.GetShearX() != 0.0f || ctm.GetShearY() != 0.0f)
	{
		//if rect is rotated, just modify left top origin
		ioRect.SetTopLeft( posTopLeft);
		return;
	}

	VPoint posBottomRight( ioRect.GetBotRight());
	CEAdjustPointInTransformedSpace( posBottomRight, inFillOnly);
	ioRect.SetTopLeft( posTopLeft);
	if (posBottomRight.GetX() >= posTopLeft.GetX())
		ioRect.SetWidth( posBottomRight.GetX()-posTopLeft.GetX());
	else
		ioRect.SetWidth(0);
	if (posBottomRight.GetY() >= posTopLeft.GetY())
		ioRect.SetHeight( posBottomRight.GetY()-posTopLeft.GetY());
	else
		ioRect.SetHeight(0);

	/*

	//we need to round width & height in transformed space in order it is integer
	//- because if transformed width & height are int values, 
	//  the 3 other corners will have same pixel offset in transformed space as the top left corner - 
	//to ensure all 4 corners are adjusted the same in transformed space
	//otherwise right or bottom border could be wrong depending on pixel offset
	//(we only do it if transformed rect is aligned with transformed space axis
	// otherwise it does not make sense because of rotation discrepancy)
	VAffineTransform ctm, ctmNoTrans;
	GetTransform( ctm);
	ctmNoTrans = ctm;
	ctmNoTrans.SetTranslation(0.0f,0.0f);

	if (ctmNoTrans.IsIdentity())
	{
		//rect is only translated so rect width/height in transformed space = rect width/height in user space:
		//here just round rect width/height in user space
		ioRect.SetPosTo( posTopLeft);
		if (ioRect.GetWidth() >= 0.5f)
			ioRect.SetWidth( floor( ioRect.GetWidth()+0.5f));
		if (ioRect.GetHeight() >= 0.5f)
			ioRect.SetHeight( floor( ioRect.GetHeight()+0.5f));
		return;
	}

	//there is rotation/shearing: we must round rect width/height in transformed space
	//							  & inverse transform it

	VPoint posBotLeftTransformed;
	posBotLeftTransformed = ctm.TransformPoint( ioRect.GetBotLeft());

	GReal widthTransformed,heightTransformed,scaleX,scaleY;
	
	if (fabs(posTopLeftTransformed.GetX() - posBotLeftTransformed.GetX()) <= 0.0001f)
	{
		//rotation 0° or rotation 180°

		scaleX = fabs(ctm.GetScaleX());
		scaleY = fabs(ctm.GetScaleY());
		if (scaleX == 0.0f || scaleY == 0.0f)
		{
			//prevent divide by 0 exception
			ioRect.SetPosTo( posTopLeft);
			return;
		}

		VPoint posTopRightTransformed;
		posTopRightTransformed = ctm.TransformPoint( ioRect.GetTopRight());

		widthTransformed = fabs(posTopRightTransformed.GetX()-posTopLeftTransformed.GetX());
		heightTransformed = fabs(posTopLeftTransformed.GetY()-posBotLeftTransformed.GetY());

		if (widthTransformed >= 0.5f)
		{
			//round width & inverse transform it
			widthTransformed = floor(widthTransformed+0.5f);
			ioRect.SetWidth( widthTransformed/scaleX);
		}
		if (heightTransformed >= 0.5f)
		{
			//round height & inverse transform it
			heightTransformed = floor(heightTransformed+0.5f);
			ioRect.SetHeight( heightTransformed/scaleY);
		}
	}
	else if (fabs(posTopLeftTransformed.GetY() - posBotLeftTransformed.GetY()) <= 0.0001f)
	{
		//rotation 90° or -90°

		//here width & height scaling are bound to shear coefs because of 90° rotation
		scaleX = fabs(ctm.GetShearX()); 
		scaleY = fabs(ctm.GetShearY());
		if (scaleX == 0.0f || scaleY == 0.0f)
		{
			//prevent divide by 0 exception
			ioRect.SetPosTo( posTopLeft);
			return;
		}

		VPoint posTopRightTransformed;
		posTopRightTransformed = ctm.TransformPoint( ioRect.GetTopRight());

		widthTransformed = fabs(posTopRightTransformed.GetY()-posTopLeftTransformed.GetY());
		heightTransformed = fabs(posTopLeftTransformed.GetX()-posBotLeftTransformed.GetX());

		if (widthTransformed >= 0.5f)
		{
			//round width & inverse transform it
			widthTransformed = floor(widthTransformed+0.5f);
			ioRect.SetWidth( widthTransformed/scaleY); //as transformed width is aligned on transformed y axis
													   //we must use scaleY but scaleX here 
		}
		if (heightTransformed >= 0.5f)
		{
			//round height & inverse transform it
			heightTransformed = floor(heightTransformed+0.5f);
			ioRect.SetHeight( heightTransformed/scaleX);
		}
	}
	ioRect.SetPosTo( posTopLeft);
	*/
}


#if VERSIONWIN
void VGraphicContext::_DrawLegacyTextBox (HDC inHDC, const VString& inString, const VColor& inColor, AlignStyle inHAlign, AlignStyle inVAlign, const VRect& inBounds, TextLayoutMode inMode)
{
	//bool restoreColor=false;

	//ensure we use advanced graphics mode (because we use untransformed coordinates)
	StGDIUseGraphicsAdvanced useAdvanced(inHDC);

	RECT bounds;
	inBounds.ToRectRef(bounds);
	
	UINT format=DT_NOPREFIX;

	if ((inMode & TLM_DONT_WRAP) != 0)
	{
		if ((inMode & TLM_LEGACY_MULTILINE) == 0)
		{
			format|=DT_SINGLELINE;
		}
	}
	else
		format|=DT_WORDBREAK;
	
	if ((inMode & TLM_TRUNCATE_MIDDLE_IF_NECESSARY) != 0)
		format|=DT_PATH_ELLIPSIS;
	else if ((inMode & TLM_TRUNCATE_END_IF_NECESSARY) != 0)
		format|=DT_END_ELLIPSIS;

	if (inMode & TLM_RIGHT_TO_LEFT)
		format |= DT_RTLREADING;

	switch(inHAlign)
	{
		case AL_DEFAULT:
		{
			format |= inMode & TLM_RIGHT_TO_LEFT ? DT_RIGHT : DT_LEFT;
			break;
		}
		case AL_LEFT:
		{
			format |= DT_LEFT;
			break;
		}
		case AL_CENTER:
		{
			format |= DT_CENTER;
			break;
		}
		case AL_RIGHT:
		{
			format |= DT_RIGHT;
			break;
		}
	}

	UINT oldAlign = ::SetTextAlign(inHDC, TA_LEFT | TA_TOP | TA_NOUPDATECP );
	UINT oldbkMode= ::SetBkMode(inHDC,TRANSPARENT);
	
	COLORREF oldCol=::SetTextColor(inHDC, inColor.WIN_ToCOLORREF());

	if (format & DT_SINGLELINE)
	{
		switch(inVAlign)
		{
			case AL_CENTER:
			{
				format|=DT_VCENTER;
				break;
			}
			case AL_BOTTOM:
			{
				format|=DT_BOTTOM;
				break;
			}
			default:
				format|=DT_TOP;
				break;
		}
	}
	else if (inVAlign == AL_CENTER || inVAlign == AL_BOTTOM)
	{
		format |= DT_CALCRECT;

		RECT boundsActual;
		boundsActual.left = bounds.left;
		boundsActual.top = bounds.top;
		boundsActual.right = bounds.right;
		boundsActual.bottom = bounds.top;
		::DrawTextW(inHDC, inString.GetCPointer(), -1, &boundsActual, format);

		switch(inVAlign)
		{
			case AL_CENTER:
			{
				bounds.top = (bounds.top+bounds.bottom)*0.5-(boundsActual.bottom-boundsActual.top)*0.5;
				break;
			}
			default: //AL_BOTTOM
				bounds.top = bounds.bottom-(boundsActual.bottom-boundsActual.top);
				break;
		}

		format &= ~DT_CALCRECT;
	}

	
	DrawTextW(inHDC,inString.GetCPointer(),-1,&bounds,format);
	
	::SetTextColor(inHDC,oldCol);
	::SetBkMode(inHDC,oldbkMode);
	::SetTextAlign(inHDC, oldAlign );
}

void VGraphicContext::_GetLegacyTextBoxSize(HDC inHDC, const VString& inString, VRect& ioHwndBounds, TextLayoutMode inLayoutMode) 
{
	//ensure we use advanced graphics mode (because we use untransformed coordinates)
	StGDIUseGraphicsAdvanced useAdvanced(inHDC);

	VPoint offset = ioHwndBounds.GetTopLeft();
	ioHwndBounds.SetPosTo(0.0f, 0.0f);

	if (ioHwndBounds.GetWidth() == 0)
		ioHwndBounds.SetWidth(1000000.0f); 
	if (ioHwndBounds.GetHeight() == 0)
		ioHwndBounds.SetHeight(1000000.0f);

	RECT bounds;
	ioHwndBounds.ToRectRef(bounds);
	UINT format=DT_NOPREFIX;
	
	if ((inLayoutMode & TLM_DONT_WRAP) != 0)
	{
		if ((inLayoutMode & TLM_LEGACY_MULTILINE) == 0)
		{
			format|=DT_SINGLELINE;
		}
	}
	else
		format|=DT_WORDBREAK;
	
	if (inLayoutMode & TLM_RIGHT_TO_LEFT)
		format |= DT_RTLREADING;

	format|= (DT_LEFT | DT_TOP | DT_CALCRECT);
	
	UINT oldAlign = ::SetTextAlign(inHDC, TA_LEFT | TA_TOP | TA_NOUPDATECP );
		
	DrawTextW(inHDC,inString.GetCPointer(),-1,&bounds,format);
	
	::SetTextAlign(inHDC, oldAlign );
	
	ioHwndBounds.FromRectRef(bounds);
	ioHwndBounds.SetPosBy(offset.GetX(), offset.GetY());
}

void VGraphicContext::_DrawLegacyTextBox(const VString& inString, AlignStyle inHAlign, AlignStyle inVAlign, const VRect& inBounds, TextLayoutMode inMode)
{
	if (inString.IsEmpty())
		return;	

	VColor color;
	GetTextColor( color);

	VRect bounds(inBounds);
	GReal pageScale = 1.0f;
	if (IsGdiPlusImpl())
	{
		//we need to scale bounds by gdiplus page scale (as gdiplus context is scaled -> we need to apply page scaling to GDI)
		Gdiplus::Graphics *gc = (Gdiplus::Graphics *)GetNativeRef();
		pageScale = gc->GetPageScale();
		if (pageScale != 1)
		{
			bounds.ScaleSizeBy(pageScale,pageScale);
			bounds.ScalePosBy(pageScale,pageScale);
		}
	}
	if(fPrinterScale)
	{
		bounds.ScaleSizeBy(fPrinterScaleX,fPrinterScaleY);
		bounds.ScalePosBy(fPrinterScaleX,fPrinterScaleY);
	}

	HDC dc=BeginUsingParentContext();

	_DrawLegacyTextBox( dc, inString, color, inHAlign, inVAlign, bounds, inMode);

	EndUsingParentContext(dc);
}

void VGraphicContext::_GetLegacyTextBoxSize( const VString& inString, VRect& ioHwndBounds, TextLayoutMode inLayoutMode) const
{
	if (inString.IsEmpty())
	{
		ioHwndBounds.SetWidth(0.0f);
		ioHwndBounds.SetHeight(0.0f);
		return;
	}
	HDC dc=BeginUsingParentContext();
	_GetLegacyTextBoxSize( dc, inString, ioHwndBounds, inLayoutMode);
	EndUsingParentContext(dc);
}



void VGraphicContext::_DrawLegacyStyledText( HDC inHDC, const VString& inText, VFont *inFont, const VColor& inColor, VTreeTextStyle *inStyles, AlignStyle inHoriz, AlignStyle inVert, const VRect& inHwndBounds, TextLayoutMode inMode, const GReal inRefDocDPI)
{
	if (inText.IsEmpty())
		return;
	//xbox_assert(inFont);

	//apply custom alignment 
	VStyledTextBox *textBox = new XWinStyledTextBox(inHDC, inHDC, inText, inStyles, inHwndBounds, inColor, inFont, inMode, inRefDocDPI);

	if (textBox)
	{
		textBox->SetTextAlign( inHoriz);
		textBox->SetParaAlign( inVert);

		//ensure we use advanced graphics mode (because we use untransformed coordinates)
		StGDIUseGraphicsAdvanced useAdvanced(inHDC);

		UINT oldbkMode= ::SetBkMode(inHDC,TRANSPARENT);
		textBox->Draw(inHwndBounds);
		::SetBkMode(inHDC,oldbkMode);
		
		textBox->Release();
	}
}

void VGraphicContext::_GetLegacyStyledTextBoxBounds( HDC inHDC, const VString& inText, VFont *inFont, VTreeTextStyle *inStyles, VRect& ioBounds, const GReal inRefDocDPI,bool inForceMonoStyle, const HDC inOutputRefDC)
{
	//xbox_assert(inFont);

	//ensure we use advanced graphics mode (because we use untransformed coordinates)
	StGDIUseGraphicsAdvanced useAdvanced(inHDC);
	StGDIUseGraphicsAdvanced useAdvancedOutput(inOutputRefDC);

	TextLayoutMode LayoutMode = TLM_NORMAL;

	if(inForceMonoStyle)
		LayoutMode |= TRM_LEGACY_MONOSTYLE;

	VRect bounds;
	if (ioBounds.GetWidth() == 0.0f)
		bounds.SetWidth( 100000.0f);
	else
		bounds.SetWidth( ioBounds.GetWidth());
	if (ioBounds.GetHeight() == 0.0f)
		bounds.SetHeight( 100000.0f);
	else
		bounds.SetHeight( ioBounds.GetHeight());

	GReal outWidth = bounds.GetWidth(), outHeight = 0;
	XWinStyledTextBox *textBox = new XWinStyledTextBox(inHDC, inOutputRefDC, inText, inStyles, bounds, VColor::sBlackColor, inFont, LayoutMode, inRefDocDPI);
	if (textBox)
	{
		textBox->GetSize(outWidth, outHeight, IsPrinterDC(inOutputRefDC));
		textBox->SetOutputRefDC(NULL);
		textBox->Release();
	}

	ioBounds.SetWidth( outWidth);
	ioBounds.SetHeight( outHeight);
}


void VGraphicContext::_GetLegacyStyledTextBoxCaretMetricsFromCharIndex( HDC inHDC, const VString& inText, VFont *inFont, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading, const bool inCaretUseCharMetrics, AlignStyle inHAlign, AlignStyle inVAlign, TextLayoutMode inMode, const GReal inRefDocDPI)
{
	//xbox_assert(inFont);

	//ensure we use advanced graphics mode (because we use untransformed coordinates)
	StGDIUseGraphicsAdvanced useAdvanced(inHDC);

	//ensure we return valid metrics if text is empty
	const VString *text = &inText;
	VString sEmpty("x");
	bool leading = inCaretLeading;
	if (inText.IsEmpty())
	{
		text = &sEmpty;
		leading = true;
	}
	VIndex charIndex = inCharIndex;
	bool doAdjustCaretAfterLastCR = false;
	if (charIndex >= text->GetLength())
	{
		charIndex = text->GetLength()-1;
		leading = false;
		if (text->GetUniChar(charIndex+1) == 13)
			doAdjustCaretAfterLastCR = true;
	}
	
	//apply custom alignment 
	XWinStyledTextBox *textBox = new XWinStyledTextBox(inHDC, inHDC, *text, inStyles, inHwndBounds, VColor::sBlackColor, inFont, inMode, inRefDocDPI);

	if (textBox)
	{
		textBox->SetTextAlign( inHAlign);
		textBox->SetParaAlign( inVAlign);

		leading = (text->GetUniChar(charIndex+1) == 13 || text->GetUniChar(charIndex+1) == 10) ? true : leading;
		
		/* VStyledTextEditView uses CR and not CRLF for compatibility with RichTextEdit so this code is now useless

		//convert input text line ending (CR, LF or CRLF) char index to RTE line ending (CR) char index 
		//NDJQ:
		//	Windows Rich Text Edit converts internally CRLF to CR and so we need to adjust character index 
		//	because RTE assumes end of line if CR for char index and so one unicode character only even if input text contains CRLF
		//	which screws up char indexs 
		//	(better way to handle that ?)
		UniChar charPrev = 0;
		const UniChar *c = text->GetCPointer();
		VIndex index = 0;
		VIndex newCharIndex = charIndex;
		while (*c && index < charIndex)
		{
			if (*c == 10 && charPrev == 13)
				newCharIndex--;

			charPrev = *c++;
			index++;
		}
		charIndex = newCharIndex;
		*/

		textBox->GetCaretMetricsFromCharIndex( inHwndBounds, charIndex, outCaretPos, outTextHeight, leading, inCaretUseCharMetrics);

		if (doAdjustCaretAfterLastCR)
		{
			//adjust manually caret position if caret is after last CR (better way to handle it ?)
			outCaretPos.SetPosBy(0.0f, outTextHeight);
			outCaretPos.SetX( inHwndBounds.GetX()+1);
		}
		textBox->Release();
	}
}

bool VGraphicContext::_GetLegacyStyledTextBoxCharIndexFromCoord( HDC inHDC, const VString& inText, VFont *inFont, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VPoint& inPos, VIndex& outCharIndex, AlignStyle inHAlign, AlignStyle inVAlign, TextLayoutMode inMode, const GReal inRefDocDPI)
{
	if (inText.IsEmpty())
	{
		outCharIndex = 0;
		return false;
	}

	//ensure we use advanced graphics mode (because we use untransformed coordinates)
	StGDIUseGraphicsAdvanced useAdvanced(inHDC);

	//xbox_assert(inFont);

	//apply custom alignment 
	XWinStyledTextBox *textBox = new XWinStyledTextBox(inHDC, inHDC, inText, inStyles, inHwndBounds, VColor::sBlackColor, inFont, inMode, inRefDocDPI);

	bool inside = false;
	if (textBox)
	{
		textBox->SetTextAlign( inHAlign);
		textBox->SetParaAlign( inVAlign);

		inside = textBox->GetCharIndexFromCoord( inHwndBounds, inPos, outCharIndex);

		/* VStyledTextEditView uses CR and not CRLF for compatibility with RichTextEdit so this code is now useless

		//convert RTE line ending (CR) char index to input text line ending (CR, LF or CRLF) char index
		//NDJQ:
		//	Windows Rich Text Edit converts internally CRLF to CR and so we need to adjust character index 
		//	because RTE assumes end of line if CR for char index and so one unicode character only even if input text contains CRLF
		//	which screws up char indexs 
		//	(better way to handle that ?)
		UniChar charPrev = 0;
		const UniChar *c = inText.GetCPointer();
		VIndex index = 0;
		VIndex newCharIndex = outCharIndex;
		while (*c && index <= outCharIndex)
		{
			if (*c == 10 && charPrev == 13)
				newCharIndex++; 

			charPrev = *c++;
			index++;
		}
		outCharIndex = newCharIndex;

		if (outCharIndex < inText.GetLength() && outCharIndex > 0)
		{
			if (inText.GetUniChar(outCharIndex+1) == 10 && inText.GetUniChar(outCharIndex) == 13)
				outCharIndex--;
		}
		*/
		textBox->Release();
	}
	return inside;
}

void VGraphicContext::_GetLegacyStyledTextBoxRunBoundsFromRange( HDC inHDC, const VString& inText, VFont *inFont, VTreeTextStyle *inStyles, const VRect& inBounds, std::vector<VRect>& outRunBounds, sLONG inStart, sLONG inEnd, AlignStyle inHAlign, AlignStyle inVAlign, TextLayoutMode inMode, const GReal inRefDocDPI)
{
	if (inText.IsEmpty())
	{
		outRunBounds.clear();
		return;
	}

	//ensure we use advanced graphics mode (because we use untransformed coordinates)
	StGDIUseGraphicsAdvanced useAdvanced(inHDC);

	//xbox_assert(inFont);

	//apply custom alignment 
	XWinStyledTextBox *textBox = new XWinStyledTextBox(inHDC, inHDC, inText, inStyles, inBounds, VColor::sBlackColor, inFont, inMode, inRefDocDPI);

	if (textBox)
	{
		textBox->SetTextAlign( inHAlign);
		textBox->SetParaAlign( inVAlign);
		textBox->GetRunBoundsFromRange( inBounds, outRunBounds, inStart, inEnd);
		textBox->Release();
	}
}


void VGraphicContext::_DrawLegacyStyledText( const VString& inText, VTreeTextStyle *inStyles, AlignStyle inHoriz, AlignStyle inVert, const VRect& inHwndBounds, TextLayoutMode inMode, const GReal inRefDocDPI)
{
	if (inText.IsEmpty())
		return;

	StUseContext_NoRetain	context(this);

	VRect txtBounds(inHwndBounds);
	VColor textColor;
	GetTextColor(textColor);
	VFont *font = RetainFont();

	GReal pageScale = 1.0f;
	if (IsGdiPlusImpl())
	{
		//we need to scale bounds & passed dpi by gdiplus page scale (as gdiplus context is scaled -> we need to apply page scaling to GDI)
		Gdiplus::Graphics *gc = (Gdiplus::Graphics *)GetNativeRef();
		pageScale = gc->GetPageScale();
		if (pageScale != 1)
		{
			txtBounds.ScaleSizeBy(pageScale,pageScale);
			txtBounds.ScalePosBy(pageScale,pageScale);
		}
	}
	if(fPrinterScale) 
	{
		txtBounds.ScaleSizeBy(fPrinterScaleX,fPrinterScaleY);
		txtBounds.ScalePosBy(fPrinterScaleX,fPrinterScaleY);
	}

	ContextRef contextRef = BeginUsingParentContext();
	
//#if VERSIONDEBUG
//	HBRUSH	brush = ::CreateSolidBrush(RGB(0, 255, 0));
//	RECT bounds;
//	bounds.left = txtBounds.GetX();
//	bounds.top = txtBounds.GetY();
//	bounds.right = txtBounds.GetRight();
//	bounds.bottom = txtBounds.GetBottom();
//	::FrameRect(contextRef, &bounds, brush);
//	::DeleteObject(brush);
//#endif

	
	//as fPrinterScaleY = printer DPI/72, we need to set text dpi to fPrinterScaleY*inRefDocDPI (so in 4D form as inRefDocDPI = 72, text dpi = (printer DPI/72)*72 = printer DPI)
	_DrawLegacyStyledText( contextRef, inText, font, textColor, inStyles, inHoriz, inVert, txtBounds, inMode, fPrinterScale ? inRefDocDPI*fPrinterScaleY*pageScale : inRefDocDPI*pageScale);

	EndUsingParentContext( contextRef);

	if (font)
		font->Release();
}

void VGraphicContext::_GetLegacyStyledTextBoxBounds( const VString& inText, VTreeTextStyle *inStyles, VRect& ioBounds, const GReal inRefDocDPI,bool inForceMonoStyle)
{
	if (inText.IsEmpty())
	{
		ioBounds.SetWidth( 0);
		ioBounds.SetHeight( 0);
		ioBounds.NormalizeToInt(true); //in GDI coord space, coordinates are absolute so stick to nearest pixel for consistency
	}
	StUseContext_NoRetain_NoDraw	context(this);

	VFont *font = RetainFont();
	
	ContextRef DestContextRef = BeginUsingParentContext();
	ContextRef contextRef = NULL; //force screen compatible dc for metrics only (VStyledTextBox assumes a ref dc for metrics compatible with the screen: it scales fonts metrics internally to inRefDocDPI (72 on default for 4D form compat))

	_GetLegacyStyledTextBoxBounds( contextRef, inText, font, inStyles, ioBounds, inRefDocDPI,inForceMonoStyle,DestContextRef);

	EndUsingParentContext(DestContextRef);
	if (font)
		font->Release();
}


void VGraphicContext::_GetLegacyStyledTextBoxCaretMetricsFromCharIndex( const VString& inText, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading, const bool inCaretUseCharMetrics, AlignStyle inHAlign, AlignStyle inVAlign, TextLayoutMode inMode, const GReal inRefDocDPI)
{	
	if (inText.IsEmpty())
	{
		outCaretPos.SetX( floor(inHwndBounds.GetX()+0.5f)); //in GDI coord space, coordinates are absolute so stick to nearest pixel for consistency
		outCaretPos.SetY( floor(inHwndBounds.GetY()+0.5f));
		outTextHeight = GetTextHeight(true);
	}
	StUseContext_NoRetain_NoDraw	context(this);

	VFont *font = RetainFont();
	//ContextRef contextRef = BeginUsingParentContext();
	ContextRef contextRef = NULL; //force screen compatible dc for metrics only (VStyledTextBox assumes a ref dc for metrics compatible with the screen: it scales fonts metrics internally to inRefDocDPI (72 on default for 4D form compat))

	_GetLegacyStyledTextBoxCaretMetricsFromCharIndex( contextRef, inText, font, inStyles, inHwndBounds, inCharIndex, outCaretPos, outTextHeight, inCaretLeading, inCaretUseCharMetrics, inHAlign, inVAlign, inMode, inRefDocDPI);
	
	//EndUsingParentContext(contextRef);
	
	if (outTextHeight <= 0.0f)
		outTextHeight = GetTextHeight(true);

	if (font)
		font->Release();
}

bool VGraphicContext::_GetLegacyStyledTextBoxCharIndexFromCoord( const VString& inText, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VPoint& inPos, VIndex& outCharIndex, AlignStyle inHAlign, AlignStyle inVAlign, TextLayoutMode inMode, const GReal inRefDocDPI)
{
	if (inText.IsEmpty())
	{
		outCharIndex = 0;
		return false;
	}

	StUseContext_NoRetain_NoDraw	context(this);  

	VFont *font = RetainFont();
	//ContextRef contextRef = BeginUsingParentContext();
	ContextRef contextRef = NULL; //force screen compatible dc for metrics only (VStyledTextBox assumes a ref dc for metrics compatible with the screen: it scales fonts metrics internally to inRefDocDPI (72 on default for 4D form compat))

	bool inside = _GetLegacyStyledTextBoxCharIndexFromCoord( contextRef, inText, font, inStyles, inHwndBounds, inPos, outCharIndex, inHAlign, inVAlign, inMode, inRefDocDPI);
	
	//EndUsingParentContext(contextRef);
	if (font)
		font->Release();
	return inside;
}

void VGraphicContext::_GetLegacyStyledTextBoxRunBoundsFromRange( const VString& inText, VTreeTextStyle *inStyles, const VRect& inBounds, std::vector<VRect>& outRunBounds, sLONG inStart, sLONG inEnd, AlignStyle inHAlign, AlignStyle inVAlign, TextLayoutMode inMode, const GReal inRefDocDPI)
{
	if (inText.IsEmpty())
	{
		outRunBounds.clear();
		return;
	}

	StUseContext_NoRetain_NoDraw	context(this);  

	VFont *font=RetainFont();
	//ContextRef contextRef = BeginUsingParentContext();
	ContextRef contextRef = NULL; //force screen compatible dc for metrics only (VStyledTextBox assumes a ref dc for metrics compatible with the screen: it scales fonts metrics internally to inRefDocDPI (72 on default for 4D form compat))

	_GetLegacyStyledTextBoxRunBoundsFromRange( contextRef, inText, font, inStyles, inBounds, outRunBounds, inStart, inEnd, inHAlign, inVAlign, inMode, inRefDocDPI);
	
	//EndUsingParentContext(contextRef);
	if (font)
		font->Release();
}

#endif


/**
*  Tests whether the given character is a valid space.
*/
static bool isSpace( UniChar c )
{
  if (c == 32)
	  return true;
  return ( c < 32 ) &&
        ( ( ( ( ( 1L << '\t' ) |
        ( 1L << '\n' ) |
        ( 1L << '\r' ) |
        ( 1L << '\f' ) ) >> c ) & 1L ) != 0 );
}

/** extract font style from VTextStyle */
VFontFace VGraphicContext::_TextStyleToFontFace( const VTextStyle *inStyle)
{
	VFontFace fontFace = KFS_NORMAL;
	if (inStyle->GetBold() == TRUE)
		fontFace |= KFS_BOLD;
	if (inStyle->GetItalic() == TRUE)
		fontFace |= KFS_ITALIC;
	if (inStyle->GetUnderline() == TRUE)
		fontFace |= KFS_UNDERLINE;
	if (inStyle->GetStrikeout() == TRUE)
		fontFace |= KFS_STRIKEOUT;
	return fontFace;
}


/** extract text lines & styles from a wrappable text using the specified passed max width
@remarks 
	layout mode is assumed to be equal to default = TLM_NORMAL
*/
void VGraphicContext::GetTextBoxLines( const VString& inText, const GReal inMaxWidth, std::vector<VString>& outTextLines, VTreeTextStyle *inStyles, std::vector<VTreeTextStyle *> *outTextLinesStyles, const GReal inRefDocDPI, const bool inNoBreakWord, bool *outLinesOverflow)
{
	xbox_assert(inStyles->GetParent() == NULL);

	if (outLinesOverflow)
		*outLinesOverflow = false;

	if (inText.IsEmpty())
	{
		outTextLines.push_back( VString(""));
		if (outTextLinesStyles)
			(*outTextLinesStyles).push_back( NULL);
		return;
	}

	VFont *fontBackup = RetainFont();
	xbox_assert(fontBackup);

	std::vector<VTextStyle *> styles;
	if (inStyles)
		//extract sequential uniform styles from cascading styles
		inStyles->BuildSequentialStylesFromCascadingStyles( styles, NULL, true);
	
	//parse text & break on word boundaries & compute width of contiguous text chunks with uniform styles
	const UniChar *c = inText.GetCPointer();
	VIndex lenText = inText.GetLength();

	VectorOfStringSlice words;
	VIntlMgr::GetDefaultMgr()->GetWordBoundariesWithOptions(inText, words, eKW_UseICU | eKW_UseLineBreakingRule);

	sLONG wordIndex = 0;
	sLONG wordStart, wordEnd;
	if (wordIndex < words.size())
	{
		wordStart = words[wordIndex].first-1;
		wordEnd = wordStart+words[wordIndex].second;
	}
	else
	{
		//no more word: make wordStart & wordEnd consistent
		wordStart = wordEnd = inText.GetLength()+2;
	}
	sLONG wordIndexBackup = 0;

	sLONG styleIndex = -1;
	sLONG styleStart, styleEnd;
	sLONG startLine = 0;
	sLONG endLine = 0;
	GReal lineWidth = 0.0f;
	VString textChunk; //contains current text with current uniform style

	sLONG styleIndexBackup = -1;
	sLONG endLineBackup = 0;
	sLONG lineWidthBackup = 0.0f;
	VString textChunkBackup;
	VFont *fontChunkBackup = NULL;

	if (inStyles)
	{
		if (!styles.empty())
		{
			styleIndex = 0;
			styles[0]->GetRange( styleStart, styleEnd);
			styleIndexBackup = 0;
		}
	}
	
	bool includeLeadingSpaces = true;
	//int counterEndOfLine = 1;
	for (int i = 0; i < lenText+1; i++, c++)
	{
		bool isValidForLineBreak = i <= wordStart || i >= wordEnd || *c == 13 || *c == 10;
		/* //we should not manage it as it is managed by ICU line breaking rule
		if (isValidForLineBreak && i == wordEnd)
		{
			if (wordIndex+1 >= words.size() || i < words[wordIndex+1].first-1)
				if (VIntlMgr::GetDefaultMgr()->IsPunctuation(*c))
					//ending punctuation: need to break after ending punctuation (standard text edit line breaking rule)
					isValidForLineBreak = false;
		}
		*/
		bool isSpaceChar = *c == 0 || VIntlMgr::GetDefaultMgr()->IsSpace( *c);

		//if (!isSpaceChar)
		//	counterEndOfLine = 0;

		if (styleIndex >= 0)
		{
			//apply current style if appropriate

			if (i == styleStart)
			{
				//start current style

				if (!textChunk.IsEmpty())
				{
					//compute actual text chunk width
					lineWidth += GetTextWidth(textChunk);
					textChunk.SetEmpty();
				}

				//set font to current style
				xbox_assert( !styles[styleIndex]->GetFontName().IsEmpty());
				VFont *font = VFont::RetainFont( styles[styleIndex]->GetFontName(),
												 _TextStyleToFontFace(styles[styleIndex]),
												 styles[styleIndex]->GetFontSize() > 0.0f ? styles[styleIndex]->GetFontSize() : 12,
												 inRefDocDPI); //force 72 DPI on default to ensure font size is correct on Windows in 4D form
				SetFont( font);
				font->Release();
			}
			if (i == styleEnd)
			{
				//end current style: switch to next style

				if (!textChunk.IsEmpty())
				{
					//compute actual text chunk width
					lineWidth += GetTextWidth(textChunk);
					textChunk.SetEmpty();
				}

				styleIndex++;
				if (styleIndex >= styles.size())
				{
					styleIndex = -1;
					SetFont( fontBackup);
				}
				else
				{
					styles[styleIndex]->GetRange( styleStart, styleEnd);
					if (i == styleStart)
					{
						//set font to style
						xbox_assert( !styles[styleIndex]->GetFontName().IsEmpty());
						VFont *font = VFont::RetainFont( styles[styleIndex]->GetFontName(),
														 _TextStyleToFontFace(styles[styleIndex]),
														 styles[styleIndex]->GetFontSize() > 0.0f ? styles[styleIndex]->GetFontSize() : 12,
														 inRefDocDPI); //force 72 DPI on default to ensure font size is correct on Windows in 4D form
						SetFont( font);
						font->Release();
					}
					else
						SetFont( fontBackup);
				}
			}
		}

		//build current word
		bool breakLine = false;
		if (!isSpaceChar && (*c != 0))
		{
			if (!includeLeadingSpaces)
			{
				//first character of first word of new line:
				//make start of line here
				startLine = i;
				includeLeadingSpaces = true;
			}
		}

		while (i >= wordEnd)
		{
			//end of word: iterate on next word
			wordIndex++;
			if (wordIndex < words.size())
			{
				wordStart = words[wordIndex].first-1;
				wordEnd = wordStart+words[wordIndex].second;
			}
			else
			{
				//no more word: make wordStart & wordEnd consistent
				wordStart = wordEnd = inText.GetLength()+2;
				break;
			}
		}

		if (isValidForLineBreak)
		{
			//line break ?

			//end of word
			if (!textChunk.IsEmpty())
			{
				//compute actual text chunk width
				lineWidth += GetTextWidth(textChunk);
				textChunk.SetEmpty();
			}
			if (lineWidth > inMaxWidth)
			{
				//end of line: break on previous word otherwise make line with current word
				if (endLineBackup-startLine > 0.0f)
				{
					//one word at least was inside bounds: restore to end of last word inside bounds
					endLine = endLineBackup;
					lineWidth = lineWidthBackup;
					styleIndex = styleIndexBackup;

					wordIndex = wordIndexBackup;
					if (wordIndex < words.size())
					{
						wordStart = words[wordIndex].first-1;
						wordEnd = wordStart+words[wordIndex].second;
					}
					else
					{
						//no more word: make wordStart & wordEnd consistent
						wordStart = wordEnd = inText.GetLength()+2;
					}
					if (styleIndex >= 0)
						styles[styleIndex]->GetRange( styleStart, styleEnd);
					textChunk = textChunkBackup;
					if (fontChunkBackup)
					{
						SetFont( fontChunkBackup);
						fontChunkBackup->Release();
						fontChunkBackup = NULL;
					}
					i = endLine;
					c = inText.GetCPointer()+endLine;
					isSpaceChar = *c == 0 || VIntlMgr::GetDefaultMgr()->IsSpace( *c);
				}
				else if (outLinesOverflow)
						*outLinesOverflow = true;

				//add text line
				VString line;
				inText.GetSubString( startLine+1, endLine-startLine, line);
				outTextLines.push_back( line);

				//add text line style
				if (outTextLinesStyles && endLine > startLine && inStyles)
				{
					VTreeTextStyle *lineStylesTree = inStyles->CreateTreeTextStyleOnRange( startLine, endLine);
					(*outTextLinesStyles).push_back( lineStylesTree);
				}
				else if (outTextLinesStyles)
					(*outTextLinesStyles).push_back( NULL);

				//start new line
				startLine = endLine = i;
				endLineBackup = endLine;
				lineWidth = 0.0f;
				breakLine = true;
				includeLeadingSpaces = false; //do not include next leading spaces
				//counterEndOfLine = 0;
			}
			else 
			{
				//we are still inside bounds: backup current line including last word (as we are line breakable here)
				endLineBackup = endLine;
				lineWidthBackup = lineWidth;
				styleIndexBackup = styleIndex;
				wordIndexBackup = wordIndex;
				textChunkBackup = textChunk;
				if (fontChunkBackup)
					fontChunkBackup->Release();
				fontChunkBackup = RetainFont();
			}
		}
		if (*c == 13 || *c == 10 || *c == 0)
		{
			//end of line

			xbox_assert( isSpaceChar);

			//counterEndOfLine++;

			//force a break line (if it is not just done)
			if (breakLine)
				includeLeadingSpaces = true; //for explicit break line, we need to include leading spaces for next line
			else
			{
				//add line
				VString line;
				//if (endLine > startLine || counterEndOfLine > 1) //if string is empty, do not store if first end of line 
				{
					inText.GetSubString( startLine+1, endLine-startLine, line);
					outTextLines.push_back( line);

					//add text line style
					if (outTextLinesStyles && endLine > startLine && inStyles)
					{
						VTreeTextStyle *lineStylesTree = inStyles->CreateTreeTextStyleOnRange( startLine, endLine);
						(*outTextLinesStyles).push_back( lineStylesTree);
					}
					else if (outTextLinesStyles)
						(*outTextLinesStyles).push_back( NULL);
					
					//if (endLine > startLine)
					//	counterEndOfLine = 0;
				}

				if (outLinesOverflow)
				{
					textChunk.RemoveWhiteSpaces( false, true);
					if (!textChunk.IsEmpty())
					{
						//compute actual text chunk width
						lineWidth += GetTextWidth(textChunk);
						textChunk.SetEmpty();
					}
					if (lineWidth > inMaxWidth)
						*outLinesOverflow = true;
				}
				
				//start new line
				startLine = endLine = i;
				endLineBackup = endLine;
				lineWidth = 0.0f;
			}

			if (*c == 0)
				break; //end of text
			//skip end of line (deal with Unix/Mac OS/Windows end of line)
			else if (*c == 13)
			{
				startLine++;
				if (*(c+1) == 10)
				{
					c++;i++;
					isSpaceChar = true;
					startLine++;
				}
			}
			else
				startLine++;
			endLine = startLine;
			endLineBackup = endLine;

			textChunk.SetEmpty();
		}
		else if (includeLeadingSpaces || !isSpaceChar)
			textChunk.AppendUniChar( *c);

		if (!isSpaceChar)
		{
			endLine = i+1;
			includeLeadingSpaces = true;
		}
	}

	if (fontChunkBackup)
		fontChunkBackup->Release();

	if (fontBackup)
	{
		SetFont(fontBackup);
		fontBackup->Release();
	}

	//free temp sequential uniform styles 
	std::vector<VTextStyle *>::iterator itStyle = styles.begin();
	for (;itStyle != styles.end(); itStyle++)
	{
		if (*itStyle)
			delete *itStyle;
	}
}


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
void VGraphicContext::FrameArc (const VPoint& inCenter, GReal inStartDeg, GReal inEndDeg, GReal inRX, GReal inRY, bool inDrawPie)
{
	VGraphicPath *path = CreatePath( true);
	if (!path)
		return;
	VPoint start, end;
	path->MeasureArc( inCenter, inStartDeg, inEndDeg, start, end, inRX, inRY);

	path->Begin();
	//ensure we draw counterclockwise
	while (inEndDeg < inStartDeg)
		inEndDeg += 360.0f;
	bool largeArc = inEndDeg-inStartDeg > 180;
	if (inDrawPie)
	{
		path->BeginSubPathAt( inCenter);
		path->AddLineTo( start);
		path->AddArcTo( inCenter, end, inRX, inRY, largeArc, 0.0f);
		path->CloseSubPath();
	}
	else
	{
		path->BeginSubPathAt( start);
		path->AddArcTo( inCenter, end, inRX, inRY, largeArc, 0.0f);
	}
	path->End();

	FramePath( *path);

	delete path;
}

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
void VGraphicContext::FillArc (const VPoint& inCenter, GReal inStartDeg, GReal inEndDeg, GReal inRX, GReal inRY, bool inDrawPie)
{
	VGraphicPath *path = CreatePath( true);
	if (!path)
		return;
	VPoint start, end;
	path->MeasureArc( inCenter, inStartDeg, inEndDeg, start, end, inRX, inRY);

	path->Begin();
	//ensure we draw counterclockwise
	while (inEndDeg < inStartDeg)
		inEndDeg += 360.0f;
	bool largeArc = inEndDeg-inStartDeg > 180;
	if (inDrawPie)
	{
		path->BeginSubPathAt( inCenter);
		path->AddLineTo( start);
		path->AddArcTo( inCenter, end, inRX, inRY, largeArc, 0.0f);
		path->CloseSubPath();
	}
	else
	{
		path->BeginSubPathAt( start);
		path->AddArcTo( inCenter, end, inRX, inRY, largeArc, 0.0f);
	}
	path->End();

	FillPath( *path);

	delete path;
}

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
void VGraphicContext::DrawArc (const VPoint& inCenter, GReal inStartDeg, GReal inEndDeg, GReal inRX, GReal inRY, bool inDrawPie)
{
	VGraphicPath *path = CreatePath( true);
	if (!path)
		return;
	VPoint start, end;
	path->MeasureArc( inCenter, inStartDeg, inEndDeg, start, end, inRX, inRY);

	path->Begin();
	//ensure we draw counterclockwise
	while (inEndDeg < inStartDeg)
		inEndDeg += 360.0f;
	bool largeArc = inEndDeg-inStartDeg > 180;
	if (inDrawPie)
	{
		path->BeginSubPathAt( inCenter);
		path->AddLineTo( start);
		path->AddArcTo( inCenter, end, inRX, inRY, largeArc, 0.0f);
		path->CloseSubPath();
	}
	else
	{
		path->BeginSubPathAt( start);
		path->AddArcTo( inCenter, end, inRX, inRY, largeArc, 0.0f);
	}
	path->End();

	FillPath( *path);
	FramePath( *path);

	delete path;
}


/** return true if current graphic context or inStyles use only true type font(s) */
bool VGraphicContext::UseFontTrueTypeOnly( VTreeTextStyle *inStyles)
{
#if VERSIONWIN
	VFont *font = RetainFont();
	if (font)
	{
		if (!font->IsTrueTypeFont())
		{
			ReleaseRefCountable(&font);
			return false;
		}
		else
			ReleaseRefCountable(&font);
	}
	if (inStyles)
		return UseFontTrueTypeOnlyRec( inStyles);
#endif
	return true;
}

bool VGraphicContext::UseFontTrueTypeOnlyRec( VTreeTextStyle *inStyles) 
{
#if VERSIONWIN
	if (inStyles->GetData())
	{
		if (!inStyles->GetData()->GetFontName().IsEmpty())
		{
			VFont *font = VFont::RetainFont( inStyles->GetData()->GetFontName(), 12.0f, 72.0f);
			if (font)
			{
				if (!font->IsTrueTypeFont())
				{
					ReleaseRefCountable(&font);
					return false;
				}
				else
					ReleaseRefCountable(&font);
			}	
		}
	}
	sLONG count = inStyles->GetChildCount();
	for (int i = 1; i <= count; i++)
	{
		if (!UseFontTrueTypeOnlyRec( inStyles->GetNthChild( i)))
			return false;
	}
#endif
	return true;
}


void VGraphicContext::EndLayer()
{
	VImageOffScreenRef offscreen = _EndLayer();
	if (offscreen)
		offscreen->Release();
}

#if VERSIONWIN
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// following methods are for legacy VTextLayout impl (used by both GDI & GDIPlus graphic contexts & by Direct2D graphic context in legacy rendering mode)
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** draw text layout 
@remarks
	text layout origin is set to inTopLeft
*/
void VGraphicContext::_DrawTextLayout( VTextLayout *inTextLayout, const VPoint& inTopLeft)
{
	if (inTextLayout->fTextLength == 0 && inTextLayout->fPlaceHolder.IsEmpty())
		return;

	StUseContext_NoRetain context(this);
	inTextLayout->BeginUsingContext(this); 
	if (!testAssert(inTextLayout->fLayoutIsValid))
	{
		inTextLayout->EndUsingContext();
		return;
	}
	if (!inTextLayout->_BeginDrawLayer( inTopLeft))
	{
		//text has been refreshed from layer: we do not need to redraw text content
		inTextLayout->_EndDrawLayer();
		inTextLayout->EndUsingContext();
		return;
	}

	GReal pageScaleX = 1.0f, pageScaleY = 1.0f;
	if (IsGdiPlusImpl())
	{
		//we need to scale bounds & passed dpi by gdiplus page scale (as gdiplus context is scaled -> we need to apply page scaling to GDI)
		Gdiplus::Graphics *gc = (Gdiplus::Graphics *)GetNativeRef();
		pageScaleX = pageScaleY = gc->GetPageScale();
	}
	if (fPrinterScale)
	{
		pageScaleX = fPrinterScaleX;
		pageScaleY = fPrinterScaleY;
	}

	ContextRef hdc = BeginUsingParentContext();

	if (pageScaleX != 1 || pageScaleY != 1)
	{
		//while printing, we fallback to classic DrawStyledText method
		StGDIUseGraphicsAdvanced useAdvanced(hdc);

		VRect bounds(	(inTopLeft.GetX()+inTextLayout->fMargin.left)*pageScaleX, 
						(inTopLeft.GetY()+inTextLayout->fMargin.top)*pageScaleY, 
						inTextLayout->_GetLayoutWidthMinusMargin()*pageScaleX, 
						inTextLayout->_GetLayoutHeightMinusMargin()*pageScaleY);
		inTextLayout->_BeginUsingStyles();
		//as fPrinterScaleY = printer DPI/72, we need to set text dpi to fPrinterScaleY*inTextLayout->fDPI (so in 4D form as inTextLayout->fDPI = 72, text dpi = (printer DPI/72)*72 = printer DPI)
		_DrawLegacyStyledText( hdc,		inTextLayout->GetText(), inTextLayout->fCurFont, inTextLayout->fCurTextColor, inTextLayout->fStyles, 
										inTextLayout->fHAlign, inTextLayout->fVAlign, bounds, inTextLayout->fLayoutMode, inTextLayout->fDPI*pageScaleY);
		inTextLayout->_EndUsingStyles();
	}
	else
	{
		//ensure we use advanced graphics mode (because we use untransformed coordinates)
		StGDIUseGraphicsAdvanced useAdvanced(hdc);

		UINT oldbkMode= ::SetBkMode(hdc,TRANSPARENT);
		VRect bounds(	inTopLeft.GetX()+inTextLayout->fMargin.left, 
						inTopLeft.GetY()+inTextLayout->fMargin.top, 
						inTextLayout->_GetLayoutWidthMinusMargin(), 
						inTextLayout->_GetLayoutHeightMinusMargin());

		inTextLayout->fTextBox->SetDrawContext( hdc);
		inTextLayout->fTextBox->Select( inTextLayout->fSelStart, inTextLayout->fSelEnd, inTextLayout->fSelIsActive);
		inTextLayout->fTextBox->Draw(bounds);

//#if VERSIONDEBUG
//		HBRUSH	brush = ::CreateSolidBrush(RGB(0, 255, 0));
//		RECT rect = bounds;
//		::FrameRect(hdc, &rect, brush);
//		::DeleteObject(brush);
//#endif

		::SetBkMode(hdc,oldbkMode);
	}
	
	EndUsingParentContext( hdc);

//#if VERSIONDEBUG
//	SaveContext();
//	SetFillColor(VColor(0,0,255,32));
//	VRect bounds( inTopLeft.GetX(), inTopLeft.GetY(), inTextLayout->fCurLayoutWidth, inTextLayout->fCurLayoutHeight);
//	FillRect(bounds);
//	RestoreContext();
//#endif

	inTextLayout->_EndDrawLayer();
	inTextLayout->EndUsingContext();
}
	
/** return text layout bounds 
	@remark:
		text layout origin is set to inTopLeft
		on output, outBounds contains text layout bounds including any glyph overhangs

		on input, max width is determined by inTextLayout->GetMaxWidth() & max height by inTextLayout->GetMaxHeight()
		(if max width or max height is equal to 0.0f, it is assumed to be infinite)

		if text is empty and if inReturnCharBoundsForEmptyText == true, returned bounds will be set to a single char bounds 
		(useful while editing in order to compute initial text bounds which should not be empty in order to draw caret)
*/
void VGraphicContext::_GetTextLayoutBounds( VTextLayout *inTextLayout, VRect& outBounds, const VPoint& inTopLeft, bool inReturnCharBoundsForEmptyText)
{
	if (inTextLayout->fTextLength == 0 && !inReturnCharBoundsForEmptyText)
	{
		outBounds.SetCoordsTo( inTopLeft.GetX(), inTopLeft.GetY(), 0.0f, 0.0f);
		return;
	}

	StUseContext_NoRetain_NoDraw context(this);
	inTextLayout->BeginUsingContext(this, true);

	outBounds.SetCoordsTo( inTopLeft.GetX(), inTopLeft.GetY(), 0.0f, 0.0f);
	if (testAssert(inTextLayout->fLayoutIsValid))
	{
		outBounds.SetWidth( inTextLayout->fCurLayoutWidth);
		outBounds.SetHeight( inTextLayout->fCurLayoutHeight);
	}
	inTextLayout->EndUsingContext();
}


/** return text layout run bounds from the specified range 
@remarks
	text layout origin is set to inTopLeft
*/
void VGraphicContext::_GetTextLayoutRunBoundsFromRange( VTextLayout *inTextLayout, std::vector<VRect>& outRunBounds, const VPoint& inTopLeft, sLONG inStart, sLONG inEnd)
{
	if (inTextLayout->fTextLength == 0)
	{
		outRunBounds.clear();
		return;
	}

	StUseContext_NoRetain_NoDraw	context(this);
	inTextLayout->BeginUsingContext(this, true);
	if (!testAssert(inTextLayout->fLayoutIsValid))
	{
		inTextLayout->EndUsingContext();
		return;
	}
	//ContextRef hdc = BeginUsingParentContext();
	ContextRef hdc = NULL; //force screen compatible dc for metrics only (VTextLayout assumes a ref dc for metrics compatible with the screen: it scales fonts internally to the desired dpi set by SetDPI (72 on default for 4D form compat))
	{
		//ensure we use advanced graphics mode (because we use untransformed coordinates)
		StGDIUseGraphicsAdvanced useAdvanced(hdc);

		VRect bounds(	inTopLeft.GetX()+inTextLayout->fMargin.left, 
						inTopLeft.GetY()+inTextLayout->fMargin.top, 
						inTextLayout->_GetLayoutWidthMinusMargin(), 
						inTextLayout->_GetLayoutHeightMinusMargin());
		inTextLayout->fTextBox->SetDrawContext( hdc);
		(static_cast<XWinStyledTextBox *>(inTextLayout->fTextBox))->GetRunBoundsFromRange( bounds, outRunBounds, inStart, inEnd);
	}
	//EndUsingParentContext( hdc);

	inTextLayout->EndUsingContext();
}


/** return the caret position & height of text at the specified text position in the text layout
@remarks
	text layout origin is set to inTopLeft

	text position is 0-based

	caret should be drawed from outCaretPos to VPoint(outCaretPos.x,OutCaretPos.y+outTextHeight)
	if text layout is drawed at inTopLeft

	by default inCaretUseCharMetrics = true, so output caret metrics are based on the run metrics of the input character (suitable to draw caret)
	but if inCaretUseCharMetrics = false, output caret metrics are based on the full line metrics (use this one to get line height)
*/
void VGraphicContext::_GetTextLayoutCaretMetricsFromCharIndex( VTextLayout *inTextLayout, const VPoint& inTopLeft, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading, const bool inCaretUseCharMetrics)
{
	StUseContext_NoRetain_NoDraw context(this);
	inTextLayout->BeginUsingContext(this, true);
	if (!testAssert(inTextLayout->fLayoutIsValid))
	{
		inTextLayout->EndUsingContext();
		outCaretPos = inTopLeft;
		outTextHeight = 0.0f;
		return;
	}

	//ContextRef hdc = BeginUsingParentContext();
	ContextRef hdc = NULL; //force screen compatible dc for metrics only (VTextLayout assumes a ref dc for metrics compatible with the screen: it scales fonts internally to the desired dpi set by SetDPI (72 on default for 4D form compat))
	{
		//ensure we use advanced graphics mode (because we use untransformed coordinates)
		StGDIUseGraphicsAdvanced useAdvanced(hdc);

		bool leading = inCaretLeading;
		VIndex charIndex = inCharIndex;
		bool doAdjustCaretAfterLastCR = false;
		if (inTextLayout->fTextLength == 0)
		{
			//for a empty text, adjust with dummy start character
			charIndex = 0;
			leading = true;
		}
		else if (charIndex >= inTextLayout->GetText().GetLength())
		{
			charIndex = inTextLayout->GetText().GetLength()-1;
			leading = false;
			if (inTextLayout->GetText().GetUniChar(charIndex+1) == 13)
				doAdjustCaretAfterLastCR = true;
		}
		
		leading = (charIndex < inTextLayout->fTextLength && inTextLayout->GetText().GetUniChar(charIndex+1) == 13) ? true : leading;

		VRect bounds(	inTopLeft.GetX()+inTextLayout->fMargin.left, 
						inTopLeft.GetY()+inTextLayout->fMargin.top, 
						inTextLayout->_GetLayoutWidthMinusMargin(), 
						inTextLayout->_GetLayoutHeightMinusMargin());
		inTextLayout->fTextBox->SetDrawContext( hdc); 
		(static_cast<XWinStyledTextBox *>(inTextLayout->fTextBox))->GetCaretMetricsFromCharIndex( bounds, charIndex, outCaretPos, outTextHeight, leading, inCaretUseCharMetrics);

		if (doAdjustCaretAfterLastCR)
		{
			//adjust manually caret position if caret is after last CR (better way to handle it ?)
			outCaretPos.SetPosBy(0.0f, outTextHeight);
			outCaretPos.SetX( bounds.GetX()+1);
		}
	}
	//EndUsingParentContext( hdc);

	inTextLayout->EndUsingContext();
}


/** return the text position at the specified coordinates
@remarks
	text layout origin is set to inTopLeft (in gc user space)
	the hit coordinates are defined by inPos param (in gc user space)

	output text position is 0-based

	return true if input position is inside character bounds
	if method returns false, it returns the closest character index to the input position
*/
bool VGraphicContext::_GetTextLayoutCharIndexFromPos( VTextLayout *inTextLayout, const VPoint& inTopLeft, const VPoint& inPos, VIndex& outCharIndex)
{
	if (inTextLayout->fTextLength == 0)
	{
		outCharIndex = 0;
		return false;
	}
	StUseContext_NoRetain_NoDraw context(this);
	inTextLayout->BeginUsingContext(this, true);
	if (!testAssert(inTextLayout->fLayoutIsValid))
	{
		inTextLayout->EndUsingContext();
		outCharIndex = 0;
		return false;
	}

	//ContextRef hdc = BeginUsingParentContext();
	ContextRef hdc = NULL; //force screen compatible dc for metrics only (VTextLayout assumes a ref dc for metrics compatible with the screen: it scales fonts internally to the desired dpi set by SetDPI (72 on default for 4D form compat))
	bool inside = false;
	{
		//ensure we use advanced graphics mode (because we use untransformed coordinates)
		StGDIUseGraphicsAdvanced useAdvanced(hdc);

		VRect bounds(	inTopLeft.GetX()+inTextLayout->fMargin.left, 
						inTopLeft.GetY()+inTextLayout->fMargin.top, 
						inTextLayout->_GetLayoutWidthMinusMargin(), 
						inTextLayout->_GetLayoutHeightMinusMargin());
		inTextLayout->fTextBox->SetDrawContext( hdc);
		inside = (static_cast<XWinStyledTextBox *>(inTextLayout->fTextBox))->GetCharIndexFromCoord( bounds, inPos, outCharIndex);
	}
	//EndUsingParentContext( hdc);

	inTextLayout->EndUsingContext();

	if (inTextLayout->fLayoutMode & TRM_LEGACY_MONOSTYLE) //TRM_LEGACY_MONOSTYLE -> XWinStyledTextBox uses standard text edit but rich text edit 
	{
		//workaround for buggy standard edit control (no pb with RichTextEdit)
		//(with standard text edit, char index from pos is sometimes buggy when line is empty)
		VPoint pos(inPos);
		GReal height = 0;
		_GetTextLayoutCaretMetricsFromCharIndex( inTextLayout, inTopLeft, outCharIndex, pos, height, true, false);
		if (inPos.GetY() >= pos.GetY()+height)
		{
			//move to next line or end of text
			if (outCharIndex < inTextLayout->fText.GetLength())
			{
				const UniChar *c = inTextLayout->fText.GetCPointer()+outCharIndex;
				while (*c && *c != 13)
				{
				 c++;
				 outCharIndex++;
				}
				if (*c == 13)
					outCharIndex++;
			}
		}
		else if (inPos.GetY() < pos.GetY())
		{
			//move to previous char or line
			if (outCharIndex > 0)
			{
				outCharIndex--;
				const UniChar *c = inTextLayout->fText.GetCPointer()+outCharIndex;
				if (!(inTextLayout->fLayoutMode & TLM_DONT_WRAP))
				{
					_GetTextLayoutCaretMetricsFromCharIndex( inTextLayout, inTopLeft, outCharIndex, pos, height, true, false);
					if (inPos.GetY() >= pos.GetY())
						return inside;
				}
				while (outCharIndex > 0 && *c != 13)
				{
				 c--;
				 outCharIndex--;
				}
			}
		}
	}

	return inside;
}


/* update text layout
@remarks
	build text layout according to layout settings & current graphic context settings

	this method is called only from VTextLayout::_UpdateTextLayout
*/
void VGraphicContext::_UpdateTextLayout( VTextLayout *inTextLayout)
{
	xbox_assert( inTextLayout->fGC == this);

#if ENABLE_D2D
	if (inTextLayout->fLayoutD2D)
	{
		inTextLayout->fLayoutD2D->Release();
		inTextLayout->fLayoutD2D = NULL;
	}
#endif

	if (inTextLayout->fTextBox)
	{
		if (inTextLayout->fLayoutBoundsDirty)
		{
			//only sync layout & formatted bounds

			XWinStyledTextBox *textBox = dynamic_cast<XWinStyledTextBox *>(inTextLayout->fTextBox);
			if (testAssert(textBox))
			{
				textBox->SetDrawContext(NULL); //force screen compatible context
				inTextLayout->fCurWidth = inTextLayout->fMaxWidth ? inTextLayout->fMaxWidth : 100000.0f;
				inTextLayout->fCurHeight = 0.0f;
				textBox->GetSize(inTextLayout->fCurWidth, inTextLayout->fCurHeight,fPrinterScale);
				//xbox_assert( fmod(inTextLayout->fCurWidth, 1.0f) == 0.0f && fmod(inTextLayout->fCurHeight, 1.0f) == 0.0f);
				if (inTextLayout->fMaxWidth != 0.0f && inTextLayout->fCurWidth > inTextLayout->fMaxWidth)
					inTextLayout->fCurWidth = std::floor(inTextLayout->fMaxWidth); //might be ~0 otherwise it is always rounded yet
				if (inTextLayout->fMaxHeight != 0.0f && inTextLayout->fCurHeight > inTextLayout->fMaxHeight)
					inTextLayout->fCurHeight = std::floor(inTextLayout->fMaxHeight); //might be ~0 otherwise it is always rounded yet
				if (inTextLayout->fMaxWidth == 0.0f)
					inTextLayout->fCurLayoutWidth = inTextLayout->fCurWidth;
				else
					inTextLayout->fCurLayoutWidth = std::floor(inTextLayout->fMaxWidth); //might be ~0 otherwise it is always rounded yet
				if (inTextLayout->fMaxHeight == 0.0f)
					inTextLayout->fCurLayoutHeight = inTextLayout->fCurHeight;
				else
					inTextLayout->fCurLayoutHeight = std::floor(inTextLayout->fMaxHeight); //might be ~0 otherwise it is always rounded yet
				if (inTextLayout->fMargin.left != 0.0f || inTextLayout->fMargin.right != 0.0f)
				{
					inTextLayout->fCurWidth += inTextLayout->fMargin.left+inTextLayout->fMargin.right;
					inTextLayout->fCurLayoutWidth += inTextLayout->fMargin.left+inTextLayout->fMargin.right;
				}
				if (inTextLayout->fMargin.top != 0.0f || inTextLayout->fMargin.bottom != 0.0f)
				{
					inTextLayout->fCurHeight += inTextLayout->fMargin.top+inTextLayout->fMargin.bottom;
					inTextLayout->fCurLayoutHeight += inTextLayout->fMargin.top+inTextLayout->fMargin.bottom;
				}
			}
			inTextLayout->fLayoutBoundsDirty = false;
		}
		return;
	}
	inTextLayout->fLayoutBoundsDirty = false;

	//create new text layout impl
	StUseContext_NoRetain_NoDraw	context(this);  

	TextRenderingMode trm = GetTextRenderingMode(); //depending on context, GetTextRenderingMode might be valid only outside BeginUsingParentContext/EndUsingParentContext

	ContextRef destDC = BeginUsingParentContext();
	ContextRef refDC = NULL;//force screen compatible dc for metrics only (VTextLayout assumes a ref dc for metrics compatible with the screen: it scales fonts internally to the desired dpi set by SetDPI (72 on default for 4D form compat))
	{
		//ensure we use advanced graphics mode (because we use untransformed coordinates)
		StGDIUseGraphicsAdvanced useAdvanced(refDC);
		StGDIUseGraphicsAdvanced useAdvancedOutput(destDC);
		//apply custom alignment 
		VRect bounds(0.0f, 0.0f, inTextLayout->fMaxWidth ? inTextLayout->fMaxWidth : 100000.0f, inTextLayout->fMaxHeight ? inTextLayout->fMaxHeight : 100000.0f);
		VStyledTextBox *textBox = VStyledTextBox::CreateImpl( refDC, destDC, *inTextLayout, &bounds, trm);

		xbox_assert(textBox);
		if (textBox)
		{
			XWinStyledTextBox *xtextBox = dynamic_cast<XWinStyledTextBox *>(textBox);
			
			inTextLayout->fCurWidth = bounds.GetWidth();
			inTextLayout->fCurHeight = 0.0f;
			textBox->GetSize(inTextLayout->fCurWidth, inTextLayout->fCurHeight,fPrinterScale);
			//xbox_assert( fmod(inTextLayout->fCurWidth, 1.0f) == 0.0f && fmod(inTextLayout->fCurHeight, 1.0f) == 0.0f);
			if (inTextLayout->fMaxWidth != 0.0f && inTextLayout->fCurWidth > inTextLayout->fMaxWidth)
				inTextLayout->fCurWidth = std::floor(inTextLayout->fMaxWidth); //might be ~0 otherwise it is always rounded yet
			if (inTextLayout->fMaxHeight != 0.0f && inTextLayout->fCurHeight > inTextLayout->fMaxHeight)
				inTextLayout->fCurHeight = std::floor(inTextLayout->fMaxHeight); //might be ~0 otherwise it is always rounded yet
			if (inTextLayout->fMaxWidth == 0.0f)
				inTextLayout->fCurLayoutWidth = inTextLayout->fCurWidth;
			else
				inTextLayout->fCurLayoutWidth = std::floor(inTextLayout->fMaxWidth); //might be ~0 otherwise it is always rounded yet
			if (inTextLayout->fMaxHeight == 0.0f)
				inTextLayout->fCurLayoutHeight = inTextLayout->fCurHeight;
			else
				inTextLayout->fCurLayoutHeight = std::floor(inTextLayout->fMaxHeight); //might be ~0 otherwise it is always rounded yet
			if (inTextLayout->fMargin.left != 0.0f || inTextLayout->fMargin.right != 0.0f)
			{
				inTextLayout->fCurWidth += inTextLayout->fMargin.left+inTextLayout->fMargin.right;
				inTextLayout->fCurLayoutWidth += inTextLayout->fMargin.left+inTextLayout->fMargin.right;
			}
			if (inTextLayout->fMargin.top != 0.0f || inTextLayout->fMargin.bottom != 0.0f)
			{
				inTextLayout->fCurHeight += inTextLayout->fMargin.top+inTextLayout->fMargin.bottom;
				inTextLayout->fCurLayoutHeight += inTextLayout->fMargin.top+inTextLayout->fMargin.bottom;
			}
			inTextLayout->fTextBox = textBox;
			xtextBox->SetOutputRefDC(NULL);
		}

	}
	EndUsingParentContext(destDC);
}

/** GDI helper method for transparent blit */
void VGraphicContext::gdiTransparentBlt( HDC hdcDest, const VRect& inDestBounds, HDC hdcSrc, const VRect& inSrcBounds, const VColor& inColorTransparent)
{
	::TransparentBlt(	hdcDest, floor(inDestBounds.GetX()), floor(inDestBounds.GetY()), ceil(inDestBounds.GetWidth()), ceil(inDestBounds.GetHeight()),
						hdcSrc, floor(inSrcBounds.GetX()), floor(inSrcBounds.GetY()), ceil(inSrcBounds.GetWidth()), ceil(inSrcBounds.GetHeight()), 
						inColorTransparent.WIN_ToCOLORREF());
}


#if ENABLE_D2D
/** if current gc is not a D2D gc & if D2D is available & enabled, 
	derive current gc to D2D gc & return it
	(return NULL if current gc is D2D gc yet)

	you should not release returned gc if not NULL: call EndUsingD2D to properly release it
*/
VGraphicContext* VGraphicContext::BeginUsingD2D(const VRect& inBounds, bool inTransparent, bool inSoftwareOnly, sLONG inSharedUserHandle) 
{
	if (fD2DUseCount)
	{
		fD2DUseCount++;
		return fD2DCurGC;
	}

	if (HasPrinterScale())
		return NULL;

	xbox_assert(fD2DParentHDC == NULL);

	if (IsD2DImpl())
		return NULL;

	if (!IsD2DAvailable())
		return NULL;

	xbox_assert(!fD2DCurGC);

	XBOX::VRect clipBounds;
	GetClipBoundingBox( clipBounds); //we intersect input bounds with parent gc current clipping bounds (to reduce rt size)
	clipBounds.Intersect( inBounds);

	//FIXME: temp fix for clipping bug - theme controls seem to not take account ctm while clipping 
	//									 (clipping is done assuming there is none ctm translation)
	//		 as a workaround we ensure render target origin is bound to parent context origin
	clipBounds.SetCoords( 0, 0, clipBounds.GetRight(), clipBounds.GetBottom()); 

	//get GDI parent context 
	fD2DParentHDC = BeginUsingParentContext(); 

	//create new D2D context 

	XBOX::VGraphicContext *gc;
	if (inSharedUserHandle)
		gc = VGraphicContext::CreateShared( inSharedUserHandle, fD2DParentHDC, clipBounds, 1.0f, inTransparent, inSoftwareOnly);
	else
		gc = VGraphicContext::Create( fD2DParentHDC, clipBounds, 1.0f, inTransparent, inSoftwareOnly);
	xbox_assert(gc);
	if (gc)
	{
		gc->BeginUsingContext();
		fD2DCurGC = gc;
		fD2DUseCount++;
	}
	else
	{
		//failed to create gc: release the GDI parent context
		EndUsingParentContext(fD2DParentHDC);
		fD2DParentHDC = NULL;
	}

	return gc;
}


/** end using derived D2D gc & restore context 
@remarks
	pass the gc returned by BeginUsingD2D (might be NULL)
*/
void VGraphicContext::EndUsingD2D(VGraphicContext * inDerivedGC) 
{
	if (!fD2DUseCount)
	{
		xbox_assert(fD2DCurGC == NULL && inDerivedGC == NULL);
		return;
	}
	fD2DUseCount--;
	if (fD2DUseCount > 0)
		return;

	xbox_assert(fD2DUseCount == 0 && fD2DCurGC && fD2DCurGC->IsD2DImpl() && fD2DCurGC == inDerivedGC);
	
	fD2DCurGC->EndUsingContext();
	XBOX::ReleaseRefCountable(&fD2DCurGC);

	EndUsingParentContext(fD2DParentHDC);
	fD2DParentHDC = NULL;
}
#endif


/** begin using a derived Gdiplus context over render target 
@remarks
	this is used for workaround some D2D constraints
*/
VGraphicContext *VGraphicContext::BeginUsingGdiplus() const
{
	xbox_assert(fGDIPlusGC == NULL);

	//get DC from render target or parent dc
	if (IsGDIImpl())
		//GDIPlus gc might modify GDI brushes etc... so save & restore context
		(const_cast<VGraphicContext *>(this))->SaveContext();

	(const_cast<VGraphicContext *>(this))->GetTransform( fGDIPlusBackupCTM);
	(const_cast<VGraphicContext *>(this))->SetTransform(VAffineTransform());

	fGDIPlusHDC = BeginUsingParentContext();
	if (!fGDIPlusHDC)
		return NULL;

	//POINT	offset;
	//::GetViewportOrgEx(fGDIPlusHDC, &offset);
	//::GetWindowOrgEx(fGDIPlusHDC, &offset);

	if (::GetGraphicsMode( fGDIPlusHDC) != GM_ADVANCED)
		fGDIPlusBackupGM = ::SetGraphicsMode( fGDIPlusHDC, GM_ADVANCED);
	else 
		fGDIPlusBackupGM = GM_ADVANCED;

	//create Gdiplus context from GDI context

	fGDIPlusGC = static_cast<VGraphicContext *>(new VWinGDIPlusGraphicContext( fGDIPlusHDC));
	if (!testAssert(fGDIPlusGC))
	{
		EndUsingParentContext( fGDIPlusHDC);
		fGDIPlusHDC = NULL;
		return NULL;
	}

	fGDIPlusGC->BeginUsingContext();
	fGDIPlusGC->SetTransform(fGDIPlusBackupCTM);

	if(fPrinterScale && fShouldScaleShapesWithPrinterScale)
	{
		Gdiplus::Graphics* gdip=(Gdiplus::Graphics*)fGDIPlusGC->GetNativeRef();
		gdip->SetPageUnit(Gdiplus::UnitPixel);
		gdip->SetPageScale(fPrinterScaleX);
	}
	else
	{
		Gdiplus::Graphics* gdip=(Gdiplus::Graphics*)fGDIPlusGC->GetNativeRef();
		if (gdip->GetPageUnit() != Gdiplus::UnitPixel || gdip->GetPageScale() != 1)
		{
			gdip->SetPageUnit(Gdiplus::UnitPixel);
			gdip->SetPageScale(1);
		}
	}

	//inherit settings from this context
	Gdiplus::Status status;

	fGDIPlusGC->ShouldScaleShapesWithPrinterScale(false); //as printer scaling if any is applied as gdiplus page scale, disable manual printer scale
	fGDIPlusGC->SetMaxPerfFlag( GetMaxPerfFlag());

	fGDIPlusGC->SetDesiredAntiAliasing( GetDesiredAntiAliasing());

	fGDIPlusGC->EnableShapeCrispEdges( IsShapeCrispEdgesEnabled());
	fGDIPlusGC->AllowCrispEdgesOnPath( IsAllowedCrispEdgesOnPath());
	fGDIPlusGC->AllowCrispEdgesOnBezier( IsAllowedCrispEdgesOnBezier());

	if (GetMaxPerfFlag() || (fHighQualityTextRenderingMode & TRM_WITHOUT_ANTIALIASING))
		fGDIPlusGC->SetTextRenderingMode( (fHighQualityTextRenderingMode & (TRM_CONSTANT_SPACING|TRM_LEGACY_ON|TRM_LEGACY_OFF)) | TRM_WITHOUT_ANTIALIASING);
	else
		fGDIPlusGC->SetTextRenderingMode( (fHighQualityTextRenderingMode & (TRM_CONSTANT_SPACING|TRM_LEGACY_ON|TRM_LEGACY_OFF)) | TRM_WITH_ANTIALIASING_NORMAL);

	fGDIPlusGC->SetSpaceKerning( GetSpaceKerning());
	fGDIPlusGC->SetCharKerning( GetCharKerning());

	fGDIPlusGC->UseShapeBrushForText( UseShapeBrushForText());
	VFont *font = (const_cast<VGraphicContext *>(this))->RetainFont(); //no choice here but no matter as font is not modified
	if (font)
	{
		fGDIPlusGC->SetFont( font);
		ReleaseRefCountable(&font);
	}
	VColor color;
	GetTextColor( color);
	fGDIPlusGC->SetTextColor( color);

	VPoint pos;
	GetTextPos( pos);
	fGDIPlusGC->SetTextPosTo( pos);

	return fGDIPlusGC;
}

/** end using derived Gdiplus context over render target */
void VGraphicContext::EndUsingGdiplus() const
{
	if (!fGDIPlusGC)
		return;

	fGDIPlusGC->EndUsingContext();
	fGDIPlusGC->Release();
	fGDIPlusGC = NULL;

	if (::GetGraphicsMode( fGDIPlusHDC) != fGDIPlusBackupGM)
		::SetGraphicsMode( fGDIPlusHDC, fGDIPlusBackupGM);

	EndUsingParentContext( fGDIPlusHDC);

	(const_cast<VGraphicContext *>(this))->SetTransform( fGDIPlusBackupCTM);
	fGDIPlusHDC = NULL;

	if (IsGDIImpl())
		(const_cast<VGraphicContext *>(this))->RestoreContext();
}

/** begin render offscreen */
HDC VGDIOffScreen::Begin(HDC inHDC, const VRect& inBounds, bool *inNewBmp, bool inInheritParentClipping)
{
	xbox_assert(!fActive && fHDCOffScreen == NULL);

	fHDCParent = inHDC;
	fBounds = inBounds;
	fBounds.NormalizeToInt();

	int gm = ::GetGraphicsMode(fHDCParent);
	fNeedResetCTMParent = false;
	if (gm == GM_ADVANCED)
	{
		::GetWorldTransform( fHDCParent, &fCTMParent);
		VAffineTransform ctm;
		ctm.FromNativeTransform( fCTMParent);
		if (!ctm.IsIdentity())
		{
			//transform to device bounds
			fNeedResetCTMParent = true;
			fBounds = ctm.TransformRect( inBounds);
			fBounds.NormalizeToInt();
		}
	}

	if (inNewBmp)
		*inNewBmp = fHBMOffScreen == NULL;

	if (	inNewBmp && fHBMOffScreen
			&&
			(
			fBounds.GetWidth() != fLayerBounds.GetWidth() 
			||
			fBounds.GetHeight() != fLayerBounds.GetHeight()
			))
		*inNewBmp = true; //we must return true if layer bounds size have changed (even if we do not create new bmp)
						  //for consistency with VGCOffScreen (because it might be used to clear and/or inval all the layer)

	fLayerBounds = VRect(0,0,fBounds.GetWidth(),fBounds.GetHeight());

	if (fBounds.GetWidth() == 0 || fBounds.GetHeight() == 0)
		return inHDC;

	HRGN rgnClip = NULL;
	VRect boundsClip = fBounds;

	if (inInheritParentClipping)
	{
		//create clip region from parent clip region
		rgnClip = ::CreateRectRgn(0,0,0,0);
		sLONG result;
		{
		StGDIResetTransform resetCTM(fHDCParent);
		result = ::GetClipRgn(fHDCParent, rgnClip);
		}
		xbox_assert(rgnClip != NULL);
		if (result == 0 || result == -1)
		{
			::DeleteObject(rgnClip);
			rgnClip = NULL;
		}
		else
		{
			//intersect clip region with input device bounds
			POINT viewportOffset;
			::GetViewportOrgEx( fHDCParent, &viewportOffset);
			::OffsetRgn( rgnClip, -viewportOffset.x, -viewportOffset.y); //transform to logical coord space

			VRegion rgnBounds( fBounds);
			result = ::CombineRgn( rgnClip, rgnClip, rgnBounds, RGN_AND);
			if (result == 0 || result == -1)
			{
				::DeleteObject(rgnClip);
				rgnClip = NULL;
			}
			if (result == NULLREGION) //do not draw through offscreen if region is empty
			{
				::DeleteObject(rgnClip);
				return inHDC;
			}

			//new clip bounds = clip region bounds
			RECT rect;
			::GetRgnBox(rgnClip, &rect);
			boundsClip.FromRectRef( rect);
			xbox_assert(boundsClip.GetWidth() > 0 && boundsClip.GetHeight() > 0); //because not NULLREGION
			if (boundsClip.GetWidth() == 0 || boundsClip.GetHeight() == 0) //just in case...
			{
				::DeleteObject(rgnClip);
				return inHDC;
			}
		}
	}

	//create compatible offscreen DC
	fHDCOffScreen = ::CreateCompatibleDC( fHDCParent);
	if (!fHDCOffScreen) //not raster device
		return inHDC;

	//create new bitmap or select actual bitmap 
	bool isNewHBM = false;
	if (fHBMOffScreen)
	{
		//we change the internal bitmap if and only if new bounds are greater than the actual bounds
		if (!(	fLayerBounds.GetWidth() <= fMaxBounds.GetWidth() 
				&&
				fLayerBounds.GetHeight() <= fMaxBounds.GetHeight()))
		{
			::DeleteBitmap(fHBMOffScreen);
			fHBMOffScreen = NULL;
		}
	}
	bool error = false;
	for (int pass = 0; pass < 2; pass++)
	{
		if (!fHBMOffScreen)
		{
			if (fLayerBounds.GetWidth() > fMaxBounds.GetWidth())
				fMaxBounds.SetWidth( ceil(fLayerBounds.GetWidth()));
			if (fLayerBounds.GetHeight() > fMaxBounds.GetHeight())
				fMaxBounds.SetHeight( ceil(fLayerBounds.GetHeight()));

			fHBMOffScreen = ::CreateCompatibleBitmap( fHDCParent, fMaxBounds.GetWidth(), fMaxBounds.GetHeight());
			isNewHBM = true;
			if (inNewBmp)
				*inNewBmp = true;

			if (!testAssert(fHBMOffScreen))
			{
				::DeleteDC(fHDCOffScreen);
				fHDCOffScreen = NULL;
				return inHDC;
			}
		}
		fOldBmp = SelectBitmap( fHDCOffScreen, fHBMOffScreen);
		error = !fOldBmp || fOldBmp == (HBITMAP)GDI_ERROR;
		if (!isNewHBM && error)
		{
			//maybe fHBM is no more compatible with fHDCOffScreen: we need to create it again
			::DeleteBitmap(fHBMOffScreen);
			fHBMOffScreen = NULL;
		}
		else 
			break;
	}
	if (error)
	{
		if (fHBMOffScreen)
		{
			::DeleteBitmap(fHBMOffScreen);
			fHBMOffScreen = NULL;
		}
		if (inNewBmp)
			*inNewBmp = true;
		::DeleteDC(fHDCOffScreen);
		fHDCOffScreen = NULL;
		return inHDC;
	}

	//now inherit context from parent

	HFONT font = (HFONT)GetCurrentObject(fHDCParent, OBJ_FONT);
	fOldFont = SelectFont( fHDCOffScreen, font);

	if (rgnClip)
	{
		::OffsetRgn(rgnClip, -fBounds.GetX(), -fBounds.GetY());
		BOOL result = ::SelectClipRgn(fHDCOffScreen, rgnClip);
		::DeleteObject(rgnClip);
	}
	else
	{
		VRect bounds( fBounds);
		bounds.SetPosBy( -fBounds.GetX(), -fBounds.GetY());
		VRegion rgn( bounds);
		BOOL result = ::SelectClipRgn(fHDCOffScreen, rgn);
	}
	::SetViewportOrgEx( fHDCOffScreen, -fBounds.GetX(), -fBounds.GetY(), NULL);

	::SetGraphicsMode(fHDCOffScreen, gm);
	if (gm == GM_ADVANCED)
		::SetWorldTransform( fHDCOffScreen, &fCTMParent);

	fBounds = boundsClip;
	fActive = true;

	return fHDCOffScreen;
}

/** end render offscreen & blit offscreen in parent gc (only the region defined in Begin()) */
void VGDIOffScreen::End(HDC inHDCOffScreen)
{
	if (!fActive)
		return;
	fActive = false;
	xbox_assert(inHDCOffScreen == fHDCOffScreen);

	//destroy clip region
	::SelectClipRgn(fHDCOffScreen, NULL);

	//restore context
	if (fOldFont)
	{
		SelectFont( fHDCOffScreen, fOldFont);
		fOldFont = NULL;
	}

	//blit to parent dc
	if (fNeedResetCTMParent)
	{
		//set parent transform to identity
		::ModifyWorldTransform( fHDCParent, NULL, MWT_IDENTITY);
	}

	::SetGraphicsMode(fHDCOffScreen, GM_COMPATIBLE); //ensure bitmap context transform is identity
	::SetViewportOrgEx( fHDCOffScreen, 0, 0, NULL);
	::BitBlt(	fHDCParent, fBounds.GetLeft(), fBounds.GetTop(), fBounds.GetWidth(), fBounds.GetHeight(), 
				fHDCOffScreen, 0, 0, SRCCOPY);

	if (fNeedResetCTMParent)
		//restore parent ctm
		::SetWorldTransform( fHDCParent, &fCTMParent);

	SelectBitmap( fHDCOffScreen, fOldBmp);
	fOldBmp = NULL;
	::DeleteDC( fHDCOffScreen);
	fHDCOffScreen = NULL;
}


/** return true if fHBMOffScreen needs to be cleared or resized on next call to Draw or Begin */
bool VGDIOffScreen::ShouldClear( HDC inHDC, const XBOX::VRect& inBounds)
{
	if (!fHBMOffScreen)
		return true;

	fBounds = inBounds;
	fBounds.NormalizeToInt();

	int gm = ::GetGraphicsMode(inHDC);
	VAffineTransform ctm;
	XFORM xformparent;
	if (gm == GM_ADVANCED)
	{
		::GetWorldTransform( inHDC, &xformparent);
		ctm.FromNativeTransform( xformparent);
		if (!ctm.IsIdentity())
		{
			//transform to device bounds
			fBounds = ctm.TransformRect( inBounds);
			fBounds.NormalizeToInt();
		}
	}

	if (	fBounds.GetWidth() != fLayerBounds.GetWidth() 
			||
			fBounds.GetHeight() != fLayerBounds.GetHeight())
		return true;

	//create compatible offscreen DC
	fHDCOffScreen = ::CreateCompatibleDC( inHDC);
	if (!fHDCOffScreen) //not raster device
		return true;

	//check bitmap compatibility with dc
	fOldBmp = SelectBitmap( fHDCOffScreen, fHBMOffScreen);
	bool error = !fOldBmp || fOldBmp == (HBITMAP)GDI_ERROR;
	if (error)
	{
		//maybe fHBM is no more compatible with fHDCOffScreen
		::DeleteDC(fHDCOffScreen);
		fHDCOffScreen = NULL;
		fOldBmp = NULL;
		return true;
	}
	
	SelectBitmap( fHDCOffScreen, fOldBmp);
	fOldBmp = NULL;
	::DeleteDC(fHDCOffScreen);
	fHDCOffScreen = NULL;
	return false;	
}


/** draw directly current fHBMOffScreen to dest dc at the specified position */
bool VGDIOffScreen::Draw( HDC inHDC, const XBOX::VRect& inDestBounds, bool inNeedDeviceBoundsEqualToCurLayerBounds)
{
	if (!fHBMOffScreen)
		return false;

	fHDCParent = inHDC;

	fBounds = inDestBounds;
	fBounds.NormalizeToInt();

	int gm = ::GetGraphicsMode(fHDCParent);
	VAffineTransform ctm;
	XFORM xformparent;
	bool doResetTransform = false;
	if (gm == GM_ADVANCED)
	{
		::GetWorldTransform( fHDCParent, &xformparent);
		ctm.FromNativeTransform( xformparent);
		if (!ctm.IsIdentity())
		{
			doResetTransform = true;
			fBounds = ctm.TransformRect( inDestBounds);
			fBounds.NormalizeToInt();
		}
	}

	if (inNeedDeviceBoundsEqualToCurLayerBounds)
		//only translation is allowed: if transform rotation or scale has changed since layer was build, do not draw
		if (	fBounds.GetWidth() != fLayerBounds.GetWidth() 
				||
				fBounds.GetHeight() != fLayerBounds.GetHeight())
			return false;

	//create compatible offscreen DC
	fHDCOffScreen = ::CreateCompatibleDC( inHDC);
	if (!fHDCOffScreen) //not raster device
		return false;

	fOldBmp = SelectBitmap( fHDCOffScreen, fHBMOffScreen);
	bool error = !fOldBmp || fOldBmp == (HBITMAP)GDI_ERROR;
	if (error)
	{
		//maybe fHBM is no more compatible with fHDCOffScreen
		::DeleteDC(fHDCOffScreen);
		fHDCOffScreen = NULL;
		return false;
	}
	
	if (doResetTransform)
	{
		//set parent transform to identity
		::ModifyWorldTransform( fHDCParent, NULL, MWT_IDENTITY);
	}

	//blit to parent dc
	::SetGraphicsMode( fHDCOffScreen, GM_COMPATIBLE); //ensure bitmap context transform is identity
	::SelectClipRgn( fHDCOffScreen, NULL);
	::SetViewportOrgEx( fHDCOffScreen, 0, 0, NULL);
	BOOL success = ::BitBlt(	fHDCParent, fBounds.GetLeft(), fBounds.GetTop(), fBounds.GetWidth(), fBounds.GetHeight(), 
								fHDCOffScreen, 0, 0, SRCCOPY);

	if (doResetTransform)
		//restore parent ctm
		::SetWorldTransform( fHDCParent, &xformparent);

	SelectBitmap( fHDCOffScreen, fOldBmp);
	fOldBmp = NULL;
	::DeleteDC( fHDCOffScreen);
	fHDCOffScreen = NULL;

	return success != FALSE;
}

#endif


/** begin render offscreen 
@remarks
	return true if a new layer is created, false if current layer is used
	(true for instance if current transformed region bounds have changed)

	it is caller responsibility to clear or not the layer after call to Begin
*/
bool VGCOffScreen::Begin(XBOX::VGraphicContext *inGC, const XBOX::VRect& inBounds, bool inTransparent, bool inLayerInheritParentClipping, const XBOX::VRegion *inNewLayerClipRegion, const XBOX::VRegion *inReUseLayerClipRegion)
{
	xbox_assert(!fActive);

	if (inBounds.IsEmpty())
		return true;

	//begin render offscreen
	fTransparent = inTransparent;
	bool newLayer = inGC->BeginLayerOffScreen( inBounds, fLayerOffScreen, inLayerInheritParentClipping, inTransparent); 
	XBOX::ReleaseRefCountable(&fLayerOffScreen);
	fActive = true;

	//set layer clipping
	const XBOX::VRegion *layerClip = newLayer ? inNewLayerClipRegion : inReUseLayerClipRegion;
	if (layerClip)
	{
		fNeedRestoreClip = true;
		inGC->SaveClip();
		if (inGC->IsD2DImpl())
			inGC->ClipRect( layerClip->GetBounds());
		else
			inGC->ClipRegion( *layerClip);
	}
	else
		fNeedRestoreClip = false;

	if (fDrawingGCDelegate)
	{
		fDrawingGC = (*fDrawingGCDelegate)( inGC, inBounds, inTransparent, fDrawingGCParentContext, fDrawingGCDelegateUserData);
		if (fDrawingGC)
			fDrawingGC->BeginUsingContext();
	}	

	return newLayer;
}


/** end render offscreen & blit offscreen in parent gc */
void VGCOffScreen::End(XBOX::VGraphicContext *inGC)
{
	if (!fActive)
		return;
	fActive = false;

	if (fDrawingGC)
	{
		fDrawingGC->EndUsingContext();
		ReleaseRefCountable(&fDrawingGC);
		inGC->EndUsingParentContext( fDrawingGCParentContext);
	}

	//restore layer clipping 
	if (fNeedRestoreClip)
		inGC->RestoreClip();

	//end render offscreen & blit offscreen in gc context
	xbox_assert(fLayerOffScreen == NULL);
	fLayerOffScreen = inGC->EndLayerOffScreen(); //keep offscreen for reuse on next Begin
}


/** draw offscreen layer using the specified bounds  */
bool VGCOffScreen::Draw(XBOX::VGraphicContext *inGC, const VRect& inBounds)
{
	if (!fLayerOffScreen)
		return false;
	return inGC->DrawLayerOffScreen( inBounds, fLayerOffScreen);
}

/** return true if offscreen layer needs to be cleared or resized on next call to Draw or Begin/End */
bool VGCOffScreen::ShouldClear(XBOX::VGraphicContext *inGC, const VRect& inBounds)
{
	if (!fLayerOffScreen)
		return true;
	return inGC->ShouldClearLayerOffScreen( inBounds, fLayerOffScreen);
}





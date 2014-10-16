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
#include "XWinGDIGraphicContext.h"
#include "XWinGDIPlusGraphicContext.h"
#include "XWinStyledTextBox.h"
#include "VRect.h"
#include "VRegion.h"
#include "VIcon.h"
#include "VPolygon.h"
#include "VPattern.h"
#include "VBezier.h"
#include "VGraphicImage.h"
#include "VGraphicFilter.h"
#include <stack>
#include "VFont.h"

#include <wincodec.h>
#include <wincodecsdk.h>
#pragma comment(lib, "WindowsCodecs.lib")
#include <winuser.h>
#include <atlcomcli.h>

//set to 1 to reveal layers 
#if VERSIONDEBUG
#define DEBUG_SHOW_LAYER	0
#else
#define DEBUG_SHOW_LAYER	0
#endif

#define SHADOW_ALPHA	255 /*60*/
#define SHADOW_R		128 /*0*/
#define SHADOW_G		128 /*0*/
#define SHADOW_B		128 /*0*/

// Class constants
const uLONG		kDEBUG_REVEAL_DELAY	= 75;

 
// Namespace
using namespace Gdiplus;

ULONG_PTR	VWinGDIPlusGraphicContext::sGDIpTok=0;
// Static
Boolean VWinGDIPlusGraphicContext::Init()
{
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	return Gdiplus::GdiplusStartup(&sGDIpTok, &gdiplusStartupInput, NULL) ==Gdiplus::Ok ;
}


void VWinGDIPlusGraphicContext::DeInit()
{
	Gdiplus::GdiplusShutdown(sGDIpTok);
}

static sLONG nbbitmap=0;
#pragma mark-

VWinGDIPlusGraphicContext::VWinGDIPlusGraphicContext(HDC inContext):VGraphicContext()
{
	_Init();
	fRefContext = inContext;
	_InitGDIPlusObjects( false);
}
VWinGDIPlusGraphicContext::VWinGDIPlusGraphicContext (Gdiplus::Graphics* inGraphics,uBOOL inOwnGraphics):VGraphicContext()
{
	_Init();
	fOwnGraphics=inOwnGraphics;
	fGDIPlusGraphics=inGraphics;
	if(!fOwnGraphics)
		SaveContext();
	_InitGDIPlusObjects( false);
}

VWinGDIPlusGraphicContext::VWinGDIPlusGraphicContext(HWND inWindow):VGraphicContext()
{
	_Init();
	fHwnd = inWindow;
	_InitGDIPlusObjects( false);
}

/** create a bitmap graphic context from the specified bitmap
@param inOwnIt
	true: graphic context owns the bitmap
	false: caller owns the bitmap

@remarks
	if caller is bitmap owner, do not destroy bitmap before graphic context 

	use method GetGdiplusBitmap() to get bitmap ref
*/
VWinGDIPlusGraphicContext::VWinGDIPlusGraphicContext( Gdiplus::Bitmap *inBitmap, bool inOwnIt)
{
	_Init();
	fBitmap = inBitmap;
	fOwnBitmap = inOwnIt;
	_InitGDIPlusObjects( false);
}


/** create a bitmap graphic context from scratch
@param inWidth
	bitmap width
@param inHeight
	bitmap height
@remarks
	use method GetGdiplusBitmap() to get bitmap ref
*/
VWinGDIPlusGraphicContext::VWinGDIPlusGraphicContext( sLONG inWidth, sLONG inHeight, Gdiplus::PixelFormat inPixelFormat)
{
	_Construct( inWidth, inHeight, inPixelFormat);
}


VWinGDIPlusGraphicContext::VWinGDIPlusGraphicContext( sLONG inWidth, sLONG inHeight)
{
	_Construct( inWidth, inHeight, PixelFormat32bppARGB);
}


void VWinGDIPlusGraphicContext::_Construct( sLONG inWidth, sLONG inHeight, Gdiplus::PixelFormat inPixelFormat)
{
	_Init();
	fBitmap = new Gdiplus::Bitmap(inWidth, inHeight, inPixelFormat);
	//get screen dpi
	int dpiX=VWinGDIGraphicContext::GetLogPixelsX();
	int dpiY=VWinGDIGraphicContext::GetLogPixelsY();
	//set mask bitmap dpi to screen dpi
	fBitmap->SetResolution((Gdiplus::REAL)dpiX,(Gdiplus::REAL)dpiY);
	fOwnBitmap = true;
	//clear bitmap
	BeginUsingContext();
	if (fGDIPlusGraphics)
		fGDIPlusGraphics->Clear( Gdiplus::Color(0,0,0,0));
	EndUsingContext();
}


/** retain WIC bitmap  
@remarks
	return NULL if graphic context is not a bitmap context or if context is GDI

	bitmap is still shared by the context so you should not modify it (!)
*/
IWICBitmap *VWinGDIPlusGraphicContext::RetainWICBitmap() const 
{ 
	if (!fBitmap)
		return NULL;
	xbox_assert( fUseCount == 0); //cannot access to bitmap while drawing
	if (fUseCount > 0) 
		return NULL;

	if (!fWICBitmap)
		fWICBitmap  = VPictureCodec_WIC::DecodeWIC( fBitmap);
	if (fWICBitmap)
	{
		fWICBitmap->AddRef();
		return fWICBitmap;
	}
	return NULL;
}



VWinGDIPlusGraphicContext::VWinGDIPlusGraphicContext(const VWinGDIPlusGraphicContext& inOriginal) : VGraphicContext(inOriginal)
{
	_Init();
	assert(false);
	if(inOriginal.fRefContext)
		fRefContext = inOriginal.fRefContext;
	else if(inOriginal.fHwnd)
		fHwnd=inOriginal.fHwnd;
	else if (inOriginal.fBitmap)
		fBitmap = inOriginal.fBitmap->Clone( 0, 0, inOriginal.fBitmap->GetWidth(), inOriginal.fBitmap->GetHeight(), inOriginal.fBitmap->GetPixelFormat());
	_InitGDIPlusObjects( false);
}


VWinGDIPlusGraphicContext::~VWinGDIPlusGraphicContext()
{
	_DisposeGDIPlusObjects();
	_Dispose();
}


GReal VWinGDIPlusGraphicContext::GetDpiX() const
{
	xbox_assert(fGDIPlusGraphics);
	return fGDIPlusGraphics->GetDpiX();
}

GReal VWinGDIPlusGraphicContext::GetDpiY() const 
{
	xbox_assert(fGDIPlusGraphics);
	return fGDIPlusGraphics->GetDpiY();
}

ContextRef	VWinGDIPlusGraphicContext::BeginUsingParentContext()const
{
	HDC result=NULL;
	fHDCOpenCount++;
	assert(fHDCOpenCount==1);
	if(fGDIPlusGraphics)
	{
		VAffineTransform transform;
		Gdiplus::Matrix mat;
		fGDIPlusGraphics->GetTransform(&mat);
		
		
		fHDCFont=NULL;
		fHDCSaveFont=NULL;
	
		if(fTextFont!=NULL)
		{
			//JQ 01/07/2010: fixed performance (why create again font ???)
			fHDCFont = fTextFont->GetFontRef();
		}
	
		// sync clip region
		fSavedClip=new Region();
	
		fGDIPlusGraphics->GetClip(fSavedClip);
		
		HRGN hdcclip=fSavedClip->GetHRGN(fGDIPlusGraphics);
		
		result=fGDIPlusGraphics->GetHDC();
		
		assert(result!=NULL);
		
		fHDCSavedClip=::CreateRectRgn(0,0,0,0);
		if(::GetClipRgn(result,fHDCSavedClip)==0)
		{
			DeleteObject(fHDCSavedClip);
			fHDCSavedClip=NULL;
		}	
		
		::SelectClipRgn(result,hdcclip);
		if(hdcclip)
			::DeleteObject(hdcclip);
		
		if(fHDCFont)
			fHDCSaveFont=(HFONT)::SelectObject(result,fHDCFont);
		//if(!mat.IsIdentity())
		//{
			transform.FromNativeTransform(mat);
			fHDCTransform = new HDC_TransformSetter(result,transform);
		//}
		//else
		//	fHDCTransform=NULL;
		RevealClipping(result);
	}
	else
	{
		//outside BeginUsingContext/EndUsingContext
		//we should return fRefContext or fHwnd context
		result = fRefContext;
		if (!result)
		{
			if (fHwnd)
				result = ::GetDC(fHwnd);
		}
	}

	return result;
};
void	VWinGDIPlusGraphicContext::EndUsingParentContext(ContextRef inContextRef)const
{
	fHDCOpenCount--;
	if(fGDIPlusGraphics)
	{
		if (fHDCTransform)
			delete fHDCTransform;

		::SelectClipRgn(inContextRef,fHDCSavedClip);
		if(fHDCSavedClip)
			::DeleteObject(fHDCSavedClip);
		
		if(fHDCFont)
		{
			::SelectObject(inContextRef,fHDCSaveFont);
			//JQ 01/07/2010: fixed performance 
			//::DeleteObject(fHDCFont);
		}
	
		fGDIPlusGraphics->ReleaseHDC(inContextRef);
		
		fGDIPlusGraphics->SetClip(fSavedClip,CombineModeReplace);
		delete fSavedClip;
		
		fHDCSavedClip=0;
		fSavedClip=0;
	}
	else if ((!fRefContext) && fHwnd)
		::ReleaseDC(fHwnd, inContextRef);
};

HDC VWinGDIPlusGraphicContext::_GetHDC()const
{
	HDC result=NULL;
	fHDCOpenCount++;
	if(fGDIPlusGraphics)
	{
		result=fGDIPlusGraphics->GetHDC();
		assert(result!=NULL);
	}
	else
	{
		//outside BeginUsingContext/EndUsingContext
		//we should return fRefContext or fHwnd context
		result = fRefContext;
		if (!result)
		{
			if (fHwnd)
				result = ::GetDC(fHwnd);
		}
	}
	
	return result;
}
void VWinGDIPlusGraphicContext::_ReleaseHDC(HDC inDC)const
{
	fHDCOpenCount--;
	if(fGDIPlusGraphics)
	{
		fGDIPlusGraphics->ReleaseHDC(inDC);
	}
	else if ((!fRefContext) && fHwnd)
		::ReleaseDC(fHwnd, inDC);
}


void VWinGDIPlusGraphicContext::_Init()
{
	fUseCount = 0;
	fHDCOpenCount=0;
	
	fHDCSavedClip=NULL;
	fSavedClip=NULL;
	
	fHwnd = NULL;
	fRefContext = NULL;
	fOwnGraphics=true;
	
	fBitmap = NULL;
	fWICBitmap = NULL;

	fGDIPlusGraphics = NULL;
	fLineColor = new Gdiplus::Color(0,0,0);
	fStrokePen = NULL;
	fFillBrush = NULL;
	fLineBrush = NULL;
	fTextBrush = NULL;
	fTextFont = NULL;
	fTextScale = 1.0f;
	fTextScaleTransformBackup = NULL;
	fWhiteSpaceWidth = 0.0f;
	fTextRect = NULL;

	fFillBrushSolid = NULL;
	fLineBrushLinearGradient = NULL;
	fLineBrushRadialGradient = NULL;
	fFillBrushLinearGradient = NULL;
	fFillBrushRadialGradient = NULL;

	fLineBrushGradientClamp = NULL;
	fFillBrushGradientClamp = NULL;
	fFillBrushGradientPath	= NULL;

	SetFillRule( fFillRule);

	fGlobalAlpha = 1.0;

	fShadowHOffset = 0.0;
	fShadowVOffset = 0.0;
	fShadowBlurness = 0.0;

	fDrawingMode = DM_NORMAL;
	fTextRenderingMode = fHighQualityTextRenderingMode = TRM_NORMAL;
	fTextMeasuringMode = TMM_NORMAL;
	
	fBrushPos.SetPos(0,0);
}
void VWinGDIPlusGraphicContext::_Dispose()
{
	//NDJQ: please do not move this code in _DisposeGdiplusObjects (!)
	//because it must survive outside BeginUsingContext/EndUsingContext
	if (fBitmap && fOwnBitmap)
		delete fBitmap;
	fBitmap = NULL;

	if (fWICBitmap) //WIC bitmap backing store is always owned 
		fWICBitmap->Release();
	fWICBitmap = NULL;

	//if (fHwnd != NULL)
	//	::ReleaseDC(fHwnd, fContext);
}

void VWinGDIPlusGraphicContext::_InitGDIPlusObjects(bool inBeginUsingContext)
{
	// Creation of GDI+ context if needed
	if (inBeginUsingContext)
	{
		if (fGDIPlusGraphics == NULL)
		{
			if(fRefContext)
			{
				fOwnGraphics=true;
				fGDIPlusGraphics = new Gdiplus::Graphics(fRefContext);
				fGDIPlusDirectGraphics=0;
				
				HDC refdc=GetDC(NULL);
				int logx=GetDeviceCaps(fRefContext,LOGPIXELSX);
				ReleaseDC(NULL,refdc);

				if(fGDIPlusGraphics->GetDpiX()!=logx)
				{
					fGDIPlusGraphics->SetPageUnit(Gdiplus::UnitPixel );
					fGDIPlusGraphics->SetPageScale(fGDIPlusGraphics->GetDpiX()/logx);
				}
			}
			else if(fHwnd)
			{
				fOwnGraphics=true;
				fGDIPlusGraphics = new Gdiplus::Graphics(fHwnd);
				fGDIPlusDirectGraphics=0;
			}
			else if (fBitmap)
			{
				fOwnGraphics=true;
				fGDIPlusGraphics = new Gdiplus::Graphics(static_cast<Gdiplus::Image *>(fBitmap));
				fGDIPlusDirectGraphics=0;
				fGDIPlusGraphics->SetPageScale( 1.0f);
				fGDIPlusGraphics->SetPageUnit(Gdiplus::UnitPixel);
			}
		}
		
		//fGDIPlusGraphics->SetSmoothingMode(SmoothingModeHighQuality);
		//fast antialiasing perf
		if (fGDIPlusGraphics)
			fGDIPlusGraphics->SetPixelOffsetMode( Gdiplus::PixelOffsetModeHalf);

		if (fTextFont == NULL)
		{
			//JQ 01/07/2010: fixed font initialization

			HDC hdc = _GetHDC();
			if (hdc)
			{
				LOGFONTW logFont;
				memset( &logFont, 0, sizeof(LOGFONTW));
				HGDIOBJ	fontObj = ::GetCurrentObject(hdc, OBJ_FONT);
				if (fontObj)
				{
					::GetObjectW(fontObj, sizeof(LOGFONTW), (void *)&logFont);
					_ReleaseHDC(hdc);

					VFontFace face = 0;
					if(logFont.lfItalic)
						face |= KFS_ITALIC;
					if(logFont.lfUnderline)
						face |= KFS_UNDERLINE;
					if(logFont.lfStrikeOut)
						face |= KFS_STRIKEOUT;
					if(logFont.lfWeight == FW_BOLD)
						face |= KFS_BOLD;

					GReal sizePoint = Abs(logFont.lfHeight); //as we assume 4D form 72 dpi 1pt = 1px

					fTextFont = VFont::RetainFont( VString(logFont.lfFaceName), face, sizePoint, 72, false);
				}
				else
				{
					_ReleaseHDC(hdc);
					fTextFont = VFont::RetainStdFont(STDF_CAPTION);
				}
			}
			else
			{
				_ReleaseHDC(hdc);
				fTextFont = VFont::RetainStdFont(STDF_CAPTION);
			}
			/*
			HDC dc=_GetHDC();
			fTextFont = new Gdiplus::Font(dc);
			fTextScale = 1.0f;
			_ReleaseHDC(dc);
			*/
		}
	}

	Color black_default_color(0,0,0);
	if (fStrokePen == NULL)
	{
		*fLineColor = black_default_color;
		fStrokePen = new Gdiplus::Pen(*fLineColor);
		fStrokePen->SetAlignment(Gdiplus::PenAlignmentCenter );
	}
	if (fFillBrushSolid == NULL)
		fFillBrushSolid = new Gdiplus::SolidBrush(black_default_color);
	if (fTextBrush == NULL)
		fTextBrush = new Gdiplus::SolidBrush(black_default_color);
	if (fTextRect == NULL)
		fTextRect = new Gdiplus::RectF(0,0,100,30);

	if (fFillBrush == NULL)
		fFillBrush = fFillBrushSolid;
}

void VWinGDIPlusGraphicContext::_DisposeGDIPlusObjects()
{
	//dispose layers
	ClearLayers();

	if (fGDIPlusGraphics != NULL)
	{
		if(!fOwnGraphics)
		{
			RestoreContext();
			fGDIPlusGraphics=0;
		}
		else
			delete fGDIPlusGraphics;
	}
	if (fStrokePen != NULL)
		delete fStrokePen;

	if (fFillBrush != fFillBrushSolid)
		if (fFillBrushSolid)
			delete fFillBrushSolid;

	if (fFillBrush != NULL)
		delete fFillBrush;

	if (fTextBrush != NULL)
		delete fTextBrush;

	if (fTextFont != NULL)
		fTextFont->Release();

	if (fTextRect != NULL)
		delete fTextRect;

	if (fLineBrush != NULL)
		delete fLineBrush;

	fGDIPlusGraphics = NULL;
	fStrokePen = NULL;
	fLineBrush = NULL;
	fFillBrush = NULL;
	fTextBrush = NULL;
	fTextFont = NULL;
	fTextScale = 1.0f;
	fWhiteSpaceWidth = 0.0f;
	if (fTextScaleTransformBackup)
		delete fTextScaleTransformBackup;
	fTextScaleTransformBackup = NULL;
	fTextRect = NULL;

	fFillBrushSolid = NULL;

	fLineBrushLinearGradient = NULL;
	fLineBrushRadialGradient = NULL;
	fFillBrushLinearGradient = NULL;
	fFillBrushRadialGradient = NULL;

	if (fLineBrushGradientClamp)
		delete fLineBrushGradientClamp;
	fLineBrushGradientClamp = NULL;

	if (fFillBrushGradientClamp)
		delete fFillBrushGradientClamp;
	fFillBrushGradientClamp = NULL;

	if (fFillBrushGradientPath)
		delete fFillBrushGradientPath;
	fFillBrushGradientPath = NULL;

	delete fLineColor;
}

void VWinGDIPlusGraphicContext::GetTransform(VAffineTransform &outTransform)
{
	StUseContext_NoRetain	context(this);
	Gdiplus::Matrix mat;
	fGDIPlusGraphics->GetTransform(&mat);
	outTransform.FromNativeTransform(mat);
}
void VWinGDIPlusGraphicContext::SetTransform(const VAffineTransform &inTransform)
{
	StUseContext_NoRetain	context(this);
	Gdiplus::Matrix mat;
	inTransform.ToNativeMatrix(mat);
	fGDIPlusGraphics->SetTransform(&mat); 
}
void VWinGDIPlusGraphicContext::ConcatTransform(const VAffineTransform &inTransform)
{
	StUseContext_NoRetain	context(this);
	Gdiplus::Matrix mat;
	inTransform.ToNativeMatrix(mat);
	fGDIPlusGraphics->MultiplyTransform(&mat); 
}
void VWinGDIPlusGraphicContext::RevertTransform(const VAffineTransform &inTransform)
{
	VAffineTransform inv=inTransform;
	inv.Inverse();
	ConcatTransform(inv);
}
void VWinGDIPlusGraphicContext::ResetTransform()
{
	StUseContext_NoRetain	context(this);
	fGDIPlusGraphics->ResetTransform(); 
}
void VWinGDIPlusGraphicContext::BeginUsingContext(bool inNoDraw)
{
	if (!inNoDraw && fBitmap)
	{
		//we are about to write on the bitmap: release WIC backing store
		if (fWICBitmap)
			fWICBitmap->Release();
		fWICBitmap = NULL;
	}

	fUseCount++;

	if(fRefContext)
		VWinGDIGraphicContext::DebugRegisterHDC(fRefContext);

	if (fUseCount==1)
	{
		_InitGDIPlusObjects();

		if(fHwnd)
		{
			
			RECT rect;
			GetClientRect(fHwnd, &rect);
			int nWidth = rect.right - rect.left + 1;
			int nHeight = rect.bottom - rect.top + 1;

			fMemBitmap = new Gdiplus::Bitmap(nWidth, nHeight);
			nbbitmap++;
			fGDIPlusDirectGraphics = fGDIPlusGraphics;
			fGDIPlusGraphics = Gdiplus::Graphics::FromImage(fMemBitmap);
			fGDIPlusGraphics->SetSmoothingMode(SmoothingModeHighQuality);
			//fast antialiasing perf
			fGDIPlusGraphics->SetPixelOffsetMode( Gdiplus::PixelOffsetModeHalf);
		}
		else
		{
			fMemBitmap=0;
			fGDIPlusDirectGraphics=0;
		}

		//reset text rendering mode
		fTextRenderingMode = ~fHighQualityTextRenderingMode;
		SetTextRenderingMode(fHighQualityTextRenderingMode);
	}
}


void VWinGDIPlusGraphicContext::EndUsingContext()
{
	fUseCount--;
	
	if(fRefContext)
		VWinGDIGraphicContext::DebugUnRegisterHDC(fRefContext);
	
	if (fUseCount==0)
	{
		// clear layers
		ClearLayers();

		// Delete Graphics
		if (fGDIPlusGraphics != NULL)
		{
			//Flush();
			if(fGDIPlusDirectGraphics && fMemBitmap)
			{
				fGDIPlusDirectGraphics->DrawImage(fMemBitmap, 0, 0);

				if(fOwnGraphics)
				{
					delete fGDIPlusGraphics;
					delete fGDIPlusDirectGraphics;
					fGDIPlusGraphics = NULL;
					fGDIPlusDirectGraphics=NULL;
				}
				else
				{
					delete fGDIPlusGraphics;
					fGDIPlusGraphics = fGDIPlusDirectGraphics;
					fGDIPlusDirectGraphics=0;
				}
				nbbitmap--;
				SetLastError(0);
				delete fMemBitmap;
				fMemBitmap=0;
			}
			else
			{
				// hdc ref ou graphics
				 if(fOwnGraphics)
				 {
					delete fGDIPlusGraphics;
					fGDIPlusGraphics=NULL;
				 }
			}
		}
	}
}

void VWinGDIPlusGraphicContext::Flush()
{
	if (fGDIPlusGraphics != NULL)
		fGDIPlusGraphics->Flush(FlushIntentionSync);
}




#pragma mark-


void VWinGDIPlusGraphicContext::SetFillColor(const VColor& inColor, VBrushFlags* ioFlags)
{
	Color color((inColor.GetAlpha()/255.0*fGlobalAlpha)*255, inColor.GetRed(),inColor.GetGreen(),inColor.GetBlue());
	fFillBrushSolid->SetColor(color);

	//if the current brush is a gradient, first free it 
	//before replacing with the solid color brush
	if (fFillBrush 
		&& 
		((fFillBrush->GetType() == BrushTypeLinearGradient) 
		 ||
		 (fFillBrush->GetType() == BrushTypePathGradient))
	    )
	{
		delete fFillBrush;
		fFillBrush = NULL;
		fFillBrushLinearGradient = NULL;
		fFillBrushRadialGradient = NULL;

		if (fFillBrushGradientClamp)
			delete fFillBrushGradientClamp;
		fFillBrushGradientClamp = NULL;
	}

	fFillBrush = fFillBrushSolid;

	if (ioFlags)
		ioFlags->fFillBrushChanged = true;

	if (fUseShapeBrushForText)
	{
		Gdiplus::SolidBrush *sb = dynamic_cast<Gdiplus::SolidBrush*>(fTextBrush);
		if (sb != NULL)
			sb->SetColor( color);

		if (ioFlags)
			ioFlags->fTextBrushChanged = true;
	}
}


//create GDI+ linear gradient brush from the specified input pattern
void VWinGDIPlusGraphicContext::_CreateLinearGradientBrush( const VPattern* inPattern, Gdiplus::Brush *&ioBrush, Gdiplus::LinearGradientBrush *&ioBrushLinear, Gdiplus::PathGradientBrush *&ioBrushRadial)
{
	const VAxialGradientPattern *p = static_cast<const VAxialGradientPattern*>(inPattern); 
	
	PointF p1 = GDIPLUS_POINT(p->GetStartPoint());
	PointF p2 = GDIPLUS_POINT(p->GetEndPoint());

	//release current brush if it is a linear gradient brush
	if (ioBrush 
		&& 
		((ioBrush->GetType() == BrushTypeLinearGradient) 
		 ||
		 (ioBrush->GetType() == BrushTypePathGradient))
	    )
	{
		delete ioBrush;
		ioBrush = NULL;
		ioBrushLinear = NULL;
		ioBrushRadial = NULL;
	}

	INT sizeExtra = 0;
	GReal startOffset = 0.0f;
	GReal endOffset = 1.0f;
	GradientStopVector tempStops;
	if (inPattern->GetWrapMode() == ePatternWrapClamp)
	{
		//gdiplus clamp wrapping mode is buggy:
		//as a workaround, extend gradient far beyond start and end point

		sizeExtra = 2;
		VPoint diff = p->GetEndPoint()-p->GetStartPoint();
		VPoint start = p->GetStartPoint()-diff*8/2;
		VPoint end = start + diff*8;
		startOffset = 0.5;
		endOffset = (8/2+1.0)/8;

		p1 = GDIPLUS_POINT(start);
		p2 = GDIPLUS_POINT(end);

		if (!((p->GetGradientStyle() == GRAD_STYLE_LUT 
			   ||
			   p->GetGradientStyle() == GRAD_STYLE_LUT_FAST)
			  &&
			  p->GetGradientStops().size() >= 2))
		{
			tempStops.push_back( GradientStop(0.0f, p->GetStartColor()));
			tempStops.push_back( GradientStop(1.0f, p->GetEndColor()));
		}
	}

	if (tempStops.size() >= 2
		||
		((p->GetGradientStyle() == GRAD_STYLE_LUT 
		  ||
		  p->GetGradientStyle() == GRAD_STYLE_LUT_FAST)
		  &&
		  p->GetGradientStops().size() >= 2))
	{
		//init multicolor gradient

		ioBrushLinear = new LinearGradientBrush(p1, p2, Color(), Color());
		const GradientStopVector& stops = tempStops.size() >= 2 ? tempStops : p->GetGradientStops();
		
		Gdiplus::Color *colors = new Gdiplus::Color[ stops.size()+sizeExtra];
		Gdiplus::REAL *offsets = new Gdiplus::REAL[ stops.size()+sizeExtra];

		GradientStopVector::const_iterator it = stops.begin();
		Gdiplus::Color *pcolor = colors+sizeExtra/2;
		Gdiplus::REAL *poffset = offsets+sizeExtra/2;
		for (;it != stops.end(); it++, pcolor++, poffset++)
		{
			//map [0,1] offset on [startOffset,endOffset] range
			*poffset = (*it).first*(endOffset-startOffset)+startOffset;
			VColor color = (*it).second;
			*pcolor = Gdiplus::Color( (BYTE)(color.GetAlpha()*fGlobalAlpha), color.GetRed(), color.GetGreen(), color.GetBlue());
		}
		//set clamp stops color+offset if appropriate
		if (sizeExtra != 0)
		{
			//set first stop color+offset
			pcolor = colors;
			poffset = offsets;
			*poffset	= 0.0;
			*pcolor		= *(pcolor+1);

			//set last stop color+offset
			pcolor = colors+stops.size()+1;
			poffset = offsets+stops.size()+1;
			*poffset	= 1.0;
			*pcolor		= *(pcolor-1);
		}
		ioBrushLinear->SetInterpolationColors( colors, offsets, ((INT)stops.size())+sizeExtra);

		delete [] colors;
		delete [] offsets;
	}
	else
	{
		//init standard gradient

		Color color1((BYTE)(p->GetStartColor().GetAlpha()*fGlobalAlpha), p->GetStartColor().GetRed(),p->GetStartColor().GetGreen(),p->GetStartColor().GetBlue());
		Color color2((BYTE)(p->GetEndColor().GetAlpha()*fGlobalAlpha), p->GetEndColor().GetRed(),p->GetEndColor().GetGreen(),p->GetEndColor().GetBlue());

		ioBrushLinear = new LinearGradientBrush(p1, p2, color1, color2);
	}

	//set wrap mode
	switch (inPattern->GetWrapMode())
	{
	case ePatternWrapClamp:
		ioBrushLinear->SetWrapMode( WrapModeTileFlipXY); //we use flip wrapping mode in order to reduce gdiplus clamp bug
		break;
	case ePatternWrapTile:
		ioBrushLinear->SetWrapMode( WrapModeTile);
		break;
	case ePatternWrapFlipX:
		ioBrushLinear->SetWrapMode( WrapModeTileFlipX);
		break;
	case ePatternWrapFlipY:
		ioBrushLinear->SetWrapMode( WrapModeTileFlipY);
		break;
	case ePatternWrapFlipXY:
		ioBrushLinear->SetWrapMode( WrapModeTileFlipXY);
		break;
	default:
		assert(false);
		break;
	}

	//set transform
	if (!p->GetTransform().IsIdentity())
	{
		Gdiplus::Matrix mat;
		p->GetTransform().ToNativeMatrix(mat);

		//remark: we must use MatrixOrderAppend here
		//		  because user transform must be applied after local gradient transform : 
		//		  doing this way make it possible for instance to shear the gradient 
		//		 
		//		  gdiplus will concat result transform before current transform
		//		  in order to draw gradient
		//			
		//		  (compliant with Quartz2D implementation - and also SVG 1.2 specification)
		ioBrushLinear->MultiplyTransform( &mat, MatrixOrderAppend);
	}

	ioBrush = static_cast<Gdiplus::Brush *>(ioBrushLinear);
}


//create GDI+ path gradient brush from the specified input pattern
void VWinGDIPlusGraphicContext::_CreateRadialGradientBrush( const VPattern* inPattern, Gdiplus::Brush *&ioBrush, Gdiplus::LinearGradientBrush *&ioBrushLinear, Gdiplus::PathGradientBrush *&ioBrushRadial, Gdiplus::SolidBrush *&ioBrushClamp)
{
	const VRadialGradientPattern *p = static_cast<const VRadialGradientPattern*>(inPattern); 

	bool useBrushClamp = true;
	if (&ioBrushRadial != &fFillBrushRadialGradient)
		useBrushClamp = false; //disable clamp brush for stroke 

	PatternWrapMode wrapMode = inPattern->GetWrapMode();

	bool usePrecomputed = wrapMode != ePatternWrapClamp;

	VPoint startPoint	= !usePrecomputed ? p->GetStartPoint() : p->GetStartPointTiling();
	VPoint endPoint		= !usePrecomputed ? p->GetEndPoint() : p->GetEndPointTiling();
	PointF p1 = GDIPLUS_POINT(startPoint);
	PointF p2 = GDIPLUS_POINT(endPoint);

	//release current brush if it is a linear gradient brush
	if (ioBrush 
		&& 
		((ioBrush->GetType() == BrushTypeLinearGradient) 
		 ||
		 (ioBrush->GetType() == BrushTypePathGradient))
	   )
	{
		delete ioBrush;
		ioBrush = NULL;
		ioBrushLinear = NULL;
		ioBrushRadial = NULL;
	}


	//create GDI+ path gradient 
	GReal endRadiusX;
	GReal endRadiusY;
	GReal endOffset = 1.0;

	//GDI+ tiling and reflect wrapping is buggy because it is applied on the gradient path and not 
	//on the gradient itself: as a workaround we can extend gradient path radius and manage
	//manually tiling and reflect wrapping modes with interpolation colors 
	if (!usePrecomputed)
	{
		endRadiusX = p->GetEndRadiusX();
		endRadiusY = p->GetEndRadiusY();
		endOffset = 1.0;
	}
	else
	{
		if (p->GetEndRadiusX() != 0.0 && p->GetEndRadiusY() != 0.0)
		{
			//as for tiling, end radius is stored in circular coordinate space
			//we need to map to ellipsoid coordinate space
			if (p->GetEndRadiusX() >= p->GetEndRadiusY())
			{
				endRadiusX = p->GetEndRadiusTiling();
				endRadiusY = endRadiusX*p->GetEndRadiusY()/p->GetEndRadiusX();
			}
			else
			{
				endRadiusY = p->GetEndRadiusTiling();
				endRadiusX = endRadiusY*p->GetEndRadiusX()/p->GetEndRadiusY();
			}
		}
		else
		{
			endRadiusX = endRadiusY = 0.0;
		}
		endOffset = 1.0/kPATTERN_GRADIENT_TILE_MAX_COUNT;
	}

	bool excludeGradientPath = false;
	VColor endColor;
	if  ((p->GetGradientStyle() == GRAD_STYLE_LUT 
		  ||
		  p->GetGradientStyle() == GRAD_STYLE_LUT_FAST)
		  &&
		  p->GetGradientStops().size() >= 2)
	{
			endColor = p->GetGradientStops().back().second;

			if (useBrushClamp)
			{
				//check if we need to exclude gradient path from clamp brush filling pass
				//(only if at least one stop color has opacity not equal to 1)
				if (fGlobalAlpha != 1.0)
					excludeGradientPath = true;
				else
				{
					const GradientStopVector& stops = p->GetGradientStops();			
					GradientStopVector::const_iterator it = stops.begin();
					for (;it != stops.end(); it++)
						if ((*it).second.GetAlpha() != 255)
						{
							excludeGradientPath = true;
							break;
						}
				}
			}
	}
	else
	{
			endColor = p->GetEndColor();

			if (useBrushClamp)
			{
				//check if we need to exclude gradient path from clamp brush filling pass
				//(only if at least one stop color has opacity not equal to 1)
				if (fGlobalAlpha != 1.0)
					excludeGradientPath = true;
				else
				{
					if (endColor.GetAlpha() != 255)
						excludeGradientPath = true;
					else if (p->GetStartColor().GetAlpha() != 255)
						excludeGradientPath = true;
				}
			}
	}

	if (useBrushClamp && ((BYTE)(endColor.GetAlpha()*fGlobalAlpha) != 0))
	{
		//while drawing a shape using this gradient 
		//use first this brush (set to gradient end color) to fill the shape 
		//(workaround for gdiplus nasty path gradient flood algorithm
		// which is constrained to path gradient shape)
		Color color((BYTE)(endColor.GetAlpha()*fGlobalAlpha), endColor.GetRed(),endColor.GetGreen(),endColor.GetBlue());

		if (ioBrushClamp == NULL)
			ioBrushClamp = new Gdiplus::SolidBrush( color);
		else
			ioBrushClamp->SetColor( color);
	}
	else
		if (ioBrushClamp != NULL)
		{
			delete ioBrushClamp;
			ioBrushClamp = NULL;
		}


	// Set the focus offset proportional to the ratio start radius/end radius
	GReal endRadius = xbox::Max(endRadiusX,endRadiusY);
	GReal focusOffset = endRadius != 0.0  ?  xbox::Max(p->GetStartRadiusX(),p->GetStartRadiusY())
										   /endRadius 
										 : 0.0;

	// Create a path that consists of a single circle that use the end point for center 
	// and the gradient end radius for the radius
	GraphicsPath path;
	RectF rect(	(REAL)(p2.X-endRadiusX), 
				(REAL)(p2.Y-endRadiusY), 
				(REAL)(2*endRadiusX), 
				(REAL)(2*endRadiusY));
	path.AddEllipse( rect);
	if (!p->GetTransform().IsIdentity())
	{
		Gdiplus::Matrix mat;
		p->GetTransform().ToNativeMatrix(mat);
		path.Transform( &mat);
	}

	if (ioBrushClamp && excludeGradientPath)
	{
		// store gradient path 
		// (it will be used later to fill shape with ioBrushClamp excluding gradient path
		//	in order to avoid opacity artifacts)

		if (fFillBrushGradientPath == NULL)
			fFillBrushGradientPath = new GraphicsPath();
		else
			fFillBrushGradientPath->Reset();
		fFillBrushGradientPath->AddEllipse( rect);
		if (!p->GetTransform().IsIdentity())
		{
			Gdiplus::Matrix mat;
			p->GetTransform().ToNativeMatrix( mat);
			fFillBrushGradientPath->Transform( &mat);
		}
	}
	else
	{
		if (fFillBrushGradientPath)
			delete fFillBrushGradientPath;
		fFillBrushGradientPath = NULL;
	}

	// Use the path to construct a brush.
	ioBrushRadial = new PathGradientBrush(&path);

	// Set the start point
	ioBrushRadial->SetCenterPoint( p1);

	bool bBrushDone = false;
	GradientStopVector tempStops;
	if (!((p->GetGradientStyle() == GRAD_STYLE_LUT 
		  ||
		  p->GetGradientStyle() == GRAD_STYLE_LUT_FAST)
		  &&
		  p->GetGradientStops().size() >= 2))
	{
		if (wrapMode == ePatternWrapClamp)
		{
			//init standard gradient
			
			Color color1((BYTE)(p->GetStartColor().GetAlpha()*fGlobalAlpha), p->GetStartColor().GetRed(),p->GetStartColor().GetGreen(),p->GetStartColor().GetBlue());
			Color color2((BYTE)(p->GetEndColor().GetAlpha()*fGlobalAlpha), p->GetEndColor().GetRed(),p->GetEndColor().GetGreen(),p->GetEndColor().GetBlue());

			//set the interpolation colors
			Color colors[] = {
			   color2,    // end color
			   color2,	  // yet end color
			   color1,	  // start radius color
			   color1};   // focus point color

			REAL pos[] = {
			   0.0,  // max radius 
			   1.0-endOffset, // real end radius
			   1.0-focusOffset, // real start radius
			   1.0}; // focus point
			              
			size_t sizeExtra = (focusOffset != 0.0 ? 1 : 0) + (endOffset != 1.0 ? 1 : 0);
			int offset = (endOffset != 1.0) ? 0 : 1;
			ioBrushRadial->SetInterpolationColors( colors+offset, pos+offset, (INT)(2+sizeExtra));

			bBrushDone = true;
		}
		else
		{
			//prepare multicolor gradient
			tempStops.push_back( GradientStop(0.0f, p->GetStartColor()));
			tempStops.push_back( GradientStop(1.0f, p->GetEndColor()));
		}
	}
	if (!bBrushDone)
	{
		if (wrapMode == ePatternWrapClamp)
		{
			//init multicolor clamp gradient
			const GradientStopVector& stops = p->GetGradientStops();			
			
			size_t sizeExtra = (focusOffset != 0.0 ? 1 : 0) + (endOffset != 1.0 ? 1 : 0);
			int offsetStart = (endOffset != 1.0 ? 1 : 0);

			Gdiplus::Color *colors = new Gdiplus::Color[ stops.size()+sizeExtra];
			Gdiplus::REAL *offsets = new Gdiplus::REAL[ stops.size()+sizeExtra];

			//set interpolation colors+offsets
			GradientStopVector::const_iterator it = stops.begin();
			Gdiplus::Color *pcolor = colors+offsetStart+stops.size()-1;
			Gdiplus::REAL *poffset = offsets+offsetStart+stops.size()-1;
			for (;it != stops.end(); it++, pcolor--, poffset--)
			{
				//map [0,1] input offset on range [1-focusOffset, 1-endOffset]
				*poffset = 1.0 - ((*it).first * (endOffset-focusOffset) + focusOffset);

				VColor color = (*it).second;
				*pcolor = Gdiplus::Color( (BYTE)(color.GetAlpha()*fGlobalAlpha), color.GetRed(), color.GetGreen(), color.GetBlue());
			}
			if (focusOffset != 0.0)
			{
				//set focus color and offset
				pcolor = colors+offsetStart+stops.size();
				poffset = offsets+offsetStart+stops.size();
				*pcolor = *(pcolor-1);
				*poffset = 1.0;
			}
			if (endOffset != 1.0)
			{
				//set max radius color and offset
				*colors = *(colors+1);
				*offsets = 0.0;
			}
			ioBrushRadial->SetInterpolationColors( colors, offsets, (INT)stops.size()+(INT)sizeExtra);

			delete [] colors;
			delete [] offsets;
		}
		else
		{
			//init multicolor tile or reflect gradient
			const GradientStopVector& stops = tempStops.size() > 0 ? tempStops : p->GetGradientStops();		
			
			size_t sizeExtra		= (focusOffset != 0.0 ? 1 : 0);
			size_t stopCount		= stops.size();
			size_t stopCountExtra	= stopCount+sizeExtra;

			Gdiplus::Color *colors = new Gdiplus::Color[ stopCountExtra*kPATTERN_GRADIENT_TILE_MAX_COUNT];
			Gdiplus::REAL *offsets = new Gdiplus::REAL[ stopCountExtra*kPATTERN_GRADIENT_TILE_MAX_COUNT];

			//set interpolation colors+offsets
			GradientStopVector::const_iterator it = stops.begin();
			Gdiplus::Color *pcolorLast = colors+stopCountExtra*kPATTERN_GRADIENT_TILE_MAX_COUNT-1;
			Gdiplus::REAL *poffsetLast = offsets+stopCountExtra*kPATTERN_GRADIENT_TILE_MAX_COUNT-1;
			Gdiplus::Color *pcolor = pcolorLast;
			Gdiplus::REAL *poffset = poffsetLast;
			bool reflect = wrapMode != ePatternWrapTile; 
			int indexStop = 0;
			for (;it != stops.end(); it++, indexStop++)
			{
				SmallReal offset = (*it).first * (endOffset-focusOffset) + focusOffset;
				VColor color = (*it).second;
				Gdiplus::Color colorStop = Gdiplus::Color( (BYTE)(color.GetAlpha()*fGlobalAlpha), color.GetRed(), color.GetGreen(), color.GetBlue());

				for (int i = 0; i < kPATTERN_GRADIENT_TILE_MAX_COUNT; i++)
				{
					int step;
					SmallReal offsetLocal;
					if (reflect && ((i & 1) == 1))
					{
						pcolor = pcolorLast-stopCountExtra*i-(stopCountExtra-1)+indexStop+sizeExtra;
						poffset = poffsetLast-stopCountExtra*i-(stopCountExtra-1)+indexStop+sizeExtra;
						offsetLocal = endOffset-offset;
					}
					else
					{
						pcolor = pcolorLast-stopCountExtra*i-indexStop-sizeExtra;
						poffset = poffsetLast-stopCountExtra*i-indexStop-sizeExtra;
						offsetLocal = offset;
					}

					*poffset = 1.0-i*endOffset-offsetLocal;
					*pcolor = colorStop;
				}
			}
			if (focusOffset != 0.0)
			{
				//set focus color and offset
				SmallReal offset = 0.0;
				VColor color = stops[0].second;
				Gdiplus::Color colorStop = Gdiplus::Color( (BYTE)(color.GetAlpha()*fGlobalAlpha), color.GetRed(), color.GetGreen(), color.GetBlue());

				for (int i = 0; i < kPATTERN_GRADIENT_TILE_MAX_COUNT; i++)
				{
					int step;
					SmallReal offsetLocal;
					if (reflect && ((i & 1) == 1))
					{
						pcolor = pcolorLast-stopCountExtra*i-(stopCountExtra-1);
						poffset = poffsetLast-stopCountExtra*i-(stopCountExtra-1);
						offsetLocal = endOffset-offset;
					}
					else 
					{
						pcolor = pcolorLast-stopCountExtra*i;
						poffset = poffsetLast-stopCountExtra*i;
						offsetLocal = offset;
					}

					*poffset = 1.0-i*endOffset-offsetLocal;
					*pcolor = colorStop;
				}
			}
			ioBrushRadial->SetInterpolationColors( colors, offsets, (INT)stopCountExtra*kPATTERN_GRADIENT_TILE_MAX_COUNT);

			delete [] colors;
			delete [] offsets;
		}
	}


	//set wrap mode
	//(as we manage manually gradient wrapping set it to default)
	ioBrushRadial->SetWrapMode( WrapModeClamp);

	ioBrush = static_cast<Gdiplus::Brush *>(ioBrushRadial);
}


void VWinGDIPlusGraphicContext::SetFillPattern(const VPattern* inPattern, VBrushFlags* ioFlags)
{
	if (inPattern == NULL || !inPattern->IsGradient())
	{
		//if the current brush is a gradient, first free it 
		//before replacing with the solid color brush
		if (fFillBrush 
			&& 
			((fFillBrush->GetType() == BrushTypeLinearGradient) 
			 ||
			 (fFillBrush->GetType() == BrushTypePathGradient))
			)
		{
			delete fFillBrush;
			fFillBrush = NULL;
			fFillBrushLinearGradient = NULL;
			fFillBrushRadialGradient = NULL;

			if (fFillBrushGradientClamp)
				delete fFillBrushGradientClamp;
			fFillBrushGradientClamp = NULL;
		}

		fFillBrush = fFillBrushSolid;
	}
	else
	{ 
		if (inPattern->GetKind() == 'axeP')
			_CreateLinearGradientBrush( inPattern, fFillBrush, fFillBrushLinearGradient, fFillBrushRadialGradient);
		else if (inPattern->GetKind() == 'radP')
			_CreateRadialGradientBrush( inPattern, fFillBrush, fFillBrushLinearGradient, fFillBrushRadialGradient, fFillBrushGradientClamp);
	}
	if (ioFlags)
		ioFlags->fFillBrushChanged = true;
	if (fUseShapeBrushForText)
	{
		if (ioFlags)
			ioFlags->fTextBrushChanged = true;
	}
}


void VWinGDIPlusGraphicContext::SetLineColor(const VColor& inColor, VBrushFlags* ioFlags)
{
	*fLineColor = Color( inColor.GetAlpha()*fGlobalAlpha, inColor.GetRed(), inColor.GetGreen(), inColor.GetBlue());
	fStrokePen->SetColor(*fLineColor);

	if (fLineBrush)
	{
		delete fLineBrush;
		fLineBrush = NULL;
		fLineBrushLinearGradient = NULL;
		fLineBrushRadialGradient = NULL;

		if (fLineBrushGradientClamp)
			delete fLineBrushGradientClamp;
		fLineBrushGradientClamp = NULL;
	}

	if (ioFlags)
		ioFlags->fLineBrushChanged = true;
}



void VWinGDIPlusGraphicContext::SetLinePattern(const VPattern* inPattern, VBrushFlags* ioFlags)
{
	if (inPattern != NULL) 
	{
		if (inPattern->GetKind() == 'axeP')
		{
			_CreateLinearGradientBrush( inPattern, fLineBrush, fLineBrushLinearGradient, fLineBrushRadialGradient);
			fStrokePen->SetBrush(fLineBrush);
			return;
		}	
		else if (inPattern->GetKind() == 'radP')
		{
			_CreateRadialGradientBrush( inPattern, fLineBrush, fLineBrushLinearGradient, fLineBrushRadialGradient, fLineBrushGradientClamp);
			fStrokePen->SetBrush(fLineBrush);
			return;
		}	
	}

	const VStandardPattern *stdp = dynamic_cast<const VStandardPattern*>(inPattern);
	if (stdp == NULL)
	{
		fStrokePen->SetDashStyle(Gdiplus::DashStyleSolid);
		fStrokePen->SetColor(*fLineColor);

		if (fLineBrush)
		{
			delete fLineBrush;
			fLineBrush = NULL;
			fLineBrushLinearGradient = NULL;
			fLineBrushRadialGradient = NULL;

			if (fLineBrushGradientClamp)
				delete fLineBrushGradientClamp;
			fLineBrushGradientClamp = NULL;
		}
	}
	else
	{
		PatternStyle ps = stdp->GetPatternStyle();
		switch(ps)
		{
			case PAT_LINE_DOT_MEDIUM:
			{
				fStrokePen->SetDashStyle(Gdiplus::DashStyleDash);
				break;
			}
			case PAT_LINE_DOT_MINI:
			case PAT_LINE_DOT_SMALL:
			{
				fStrokePen->SetDashStyle(Gdiplus::DashStyleDot);
				break;
			}
			default:
			{
				fStrokePen->SetDashStyle(Gdiplus::DashStyleSolid);
				break;
			}
		}
	}
	if (ioFlags)
		ioFlags->fLineBrushChanged = true;
}


/** set line dash pattern
@param inDashOffset
	offset into the dash pattern to start the line (user units)
@param inDashPattern
	dash pattern (alternative paint and unpaint lengths in user units)
	for instance {3,2,4} will paint line on 3 user units, unpaint on 2 user units
						 and paint again on 4 user units and so on...
*/
void VWinGDIPlusGraphicContext::SetLineDashPattern(GReal inDashOffset, const VLineDashPattern& inDashPattern, VBrushFlags* ioFlags)
{
	if (inDashPattern.size() < 2 || fStrokePen->GetWidth() == 0.0f)
		//clear dash pattern
		fStrokePen->SetDashStyle(Gdiplus::DashStyleSolid);
	else
	{
		//build dash pattern
		Gdiplus::REAL *pattern = new Gdiplus::REAL[inDashPattern.size()];
		VLineDashPattern::const_iterator it = inDashPattern.begin();
		Gdiplus::REAL *patternValue = pattern;
		for (;it != inDashPattern.end(); it++, patternValue++)
		{
			*patternValue = (Gdiplus::REAL)(*it) / fStrokePen->GetWidth(); //Gdiplus pattern unit is equal to pen width 
																		   //so as we use absolute user units divide first by pen width	
			//workaround for gdiplus dash pattern bug
			if (*patternValue == 0.0f)
				*patternValue = 0.0000001f;
		}
		//set dash pattern
		fStrokePen->SetDashStyle(Gdiplus::DashStyleCustom);
		fStrokePen->SetDashOffset( (Gdiplus::REAL) inDashOffset );
		fStrokePen->SetDashPattern( pattern, (INT)inDashPattern.size());

		delete [] pattern;
	}
	if (ioFlags)
		ioFlags->fLineBrushChanged = true;
}

/** set fill rule */
void VWinGDIPlusGraphicContext::SetFillRule( FillRuleType inFillRule)
{
	fFillRule = inFillRule;
	
	if (inFillRule == FILLRULE_NONZERO)
		fFillMode = Gdiplus::FillModeWinding;
	else
		fFillMode = Gdiplus::FillModeAlternate;
}


void VWinGDIPlusGraphicContext::SetLineWidth(GReal inWidth, VBrushFlags* ioFlags)
{
	fStrokePen->SetWidth(inWidth);
}

GReal VWinGDIPlusGraphicContext::GetLineWidth () const 
{ 
	if (fStrokePen)
		return fStrokePen->GetWidth();
	else
		return 1.0f; 
}


void VWinGDIPlusGraphicContext::SetLineCap (CapStyle inCapStyle, VBrushFlags* /*ioFlags*/)
{
	switch (inCapStyle)
	{
		case CS_BUTT:
			fStrokePen->SetLineCap( LineCapFlat, LineCapFlat, DashCapFlat);  
			break;
			
		case CS_ROUND:
			fStrokePen->SetLineCap( LineCapRound, LineCapRound, DashCapRound);  
			break;
			
		case CS_SQUARE:
			fStrokePen->SetLineCap( LineCapSquare, LineCapSquare, DashCapFlat);  
			break;
		
		default:
			assert(false);
			break;
	}
}

void VWinGDIPlusGraphicContext::SetLineJoin(JoinStyle inJoinStyle, VBrushFlags* /*ioFlags*/)
{
	switch (inJoinStyle)
	{
		case JS_MITER:
			fStrokePen->SetLineJoin( LineJoinMiter);
			break;
			
		case JS_ROUND:
			fStrokePen->SetLineJoin( LineJoinRound);
			break;
			
		case JS_BEVEL:
			fStrokePen->SetLineJoin( LineJoinBevel);
			break;
		
		default:
			assert(false);
			break;
	}
}

void VWinGDIPlusGraphicContext::SetLineMiterLimit( GReal inMiterLimit)
{
	fStrokePen->SetMiterLimit( inMiterLimit);
}


void VWinGDIPlusGraphicContext::SetLineStyle(CapStyle inCapStyle, JoinStyle inJoinStyle, VBrushFlags* /*ioFlags*/)
{
	SetLineCap( inCapStyle);
	SetLineJoin( inJoinStyle);
}





void VWinGDIPlusGraphicContext::SetTextColor(const VColor& inColor, VBrushFlags* ioFlags)
{
	if (fUseShapeBrushForText)
	{
		SetFillColor( inColor, ioFlags);
		return;
	}
	
	Gdiplus::SolidBrush *sb = dynamic_cast<Gdiplus::SolidBrush*>(fTextBrush);
	if (sb != NULL)
	{
		Color color((inColor.GetAlpha()/255.0*fGlobalAlpha)*255, inColor.GetRed(),inColor.GetGreen(),inColor.GetBlue());
		sb->SetColor(color);
	}
	if (ioFlags)
		ioFlags->fTextBrushChanged = true;
}

void VWinGDIPlusGraphicContext::GetTextColor (VColor& outColor) const
{
	Gdiplus::SolidBrush *sb = dynamic_cast<Gdiplus::SolidBrush*>(fTextBrush);
	if (sb != NULL)
	{
		Color color;
		sb->GetColor(&color);
		outColor.WIN_FromGdiplusColor( color);
	}
}


void VWinGDIPlusGraphicContext::SetBackColor(const VColor& inColor, VBrushFlags* ioFlags)
{

}


DrawingMode VWinGDIPlusGraphicContext::SetDrawingMode(DrawingMode inMode, VBrushFlags* ioFlags)
{
	DrawingMode toreturn = fDrawingMode;
	if (GetMaxPerfFlag())
		fDrawingMode = DM_NORMAL;
	else
		fDrawingMode = inMode;
	return toreturn;
}

void VWinGDIPlusGraphicContext::GetBrushPos (VPoint& outHwndPos) const
{
}

GReal VWinGDIPlusGraphicContext::SetTransparency(GReal inAlpha)
{
	GReal result = fGlobalAlpha;

	fGlobalAlpha = inAlpha;
	Gdiplus::SolidBrush *sbf = dynamic_cast<Gdiplus::SolidBrush*>(fFillBrush);
	if (sbf != NULL)
	{
		Color color;
		sbf->GetColor(&color);
		color.SetValue(color.MakeARGB((color.GetAlpha()/255.0*fGlobalAlpha)*255,color.GetRed(),color.GetGreen(),color.GetBlue()));
		sbf->SetColor(color);

		
	}
	Gdiplus::SolidBrush *sbt = dynamic_cast<Gdiplus::SolidBrush*>(fTextBrush);
	if (sbt != NULL)
	{
		Color color;
		sbt->GetColor(&color);
		color.SetValue(color.MakeARGB((color.GetAlpha()/255.0*fGlobalAlpha)*255,color.GetRed(),color.GetGreen(),color.GetBlue()));
		sbt->SetColor(color);
	}
	Gdiplus::Pen *p = dynamic_cast<Gdiplus::Pen*>(fStrokePen);
	if (p != NULL)
	{
		Color color;
		p->GetColor(&color);
		color.SetValue(color.MakeARGB((color.GetAlpha()/255.0*fGlobalAlpha)*255,color.GetRed(),color.GetGreen(),color.GetBlue()));
		p->SetColor(color);
	}
	return result;
}


void VWinGDIPlusGraphicContext::SetShadowValue(GReal inHOffset, GReal inVOffset, GReal inBlurness)
{
	fShadowHOffset = inHOffset;
	fShadowVOffset = inVOffset;
	fShadowBlurness = inBlurness;
}


void VWinGDIPlusGraphicContext::EnableAntiAliasing()
{
	fIsHighQualityAntialiased = true;
	if (GetMaxPerfFlag())
		fGDIPlusGraphics->SetSmoothingMode(SmoothingModeNone);
	else
		fGDIPlusGraphics->SetSmoothingMode(SmoothingModeHighQuality);
}


void VWinGDIPlusGraphicContext::DisableAntiAliasing()
{
	fIsHighQualityAntialiased = false;
	fGDIPlusGraphics->SetSmoothingMode(SmoothingModeNone);
}


bool VWinGDIPlusGraphicContext::IsAntiAliasingAvailable()
{
	return true;
}
TextMeasuringMode VWinGDIPlusGraphicContext::SetTextMeasuringMode(TextMeasuringMode inMode)
{
	TextMeasuringMode old=fTextMeasuringMode;
	fTextMeasuringMode = inMode;
	return old;
}

TextRenderingMode VWinGDIPlusGraphicContext::SetTextRenderingMode( TextRenderingMode inMode)
{
	TextRenderingMode oldMode = fTextRenderingMode;

	if (inMode & TRM_LEGACY_ON)
		inMode &= ~TRM_LEGACY_OFF; //TRM_LEGACY_ON & TRM_LEGACY_OFF are mutually exclusive 

	fHighQualityTextRenderingMode = inMode;
	if (fMaxPerfFlag)
		inMode = (inMode & (TRM_CONSTANT_SPACING|TRM_LEGACY_ON|TRM_LEGACY_OFF)) | TRM_WITHOUT_ANTIALIASING;

	if (fTextRenderingMode != inMode)
	{
		TextRenderingHint new_hint;

		if (inMode & TRM_WITH_ANTIALIASING_NORMAL)
			new_hint = TextRenderingHintAntiAlias;
		else if (inMode & TRM_WITH_ANTIALIASING_CLEARTYPE)
			new_hint = fStackLayerGC.size() > 0 ? TextRenderingHintAntiAlias : TextRenderingHintClearTypeGridFit;
		else if (inMode & TRM_WITHOUT_ANTIALIASING)
			new_hint = TextRenderingHintSingleBitPerPixel;
		else
			new_hint = fStackLayerGC.size() > 0 ? TextRenderingHintAntiAlias : TextRenderingHintClearTypeGridFit;

		fGDIPlusGraphics->SetTextRenderingHint( new_hint);
		fTextRenderingMode = inMode;

	}
	
	return oldMode;
}

TextRenderingMode  VWinGDIPlusGraphicContext::GetTextRenderingMode() const
{
	int textRenderingMode = TRM_WITHOUT_ANTIALIASING;
	TextRenderingHint textRenderingHint = fGDIPlusGraphics->GetTextRenderingHint();
	switch( textRenderingHint)
	{
	case TextRenderingHintSystemDefault:
		{
		//here we need to query system parameter info to get the actual text smoothing type

		BOOL fontSmoothing		= false;
		UINT fontSmoothingType	= 0;
		if (SystemParametersInfo(	SPI_GETFONTSMOOTHING,
									FALSE,
									&fontSmoothing,
									0))
		{
			if (fontSmoothing)
			{
				textRenderingMode = TRM_WITH_ANTIALIASING_NORMAL;
				#if(_WIN32_WINNT < 0x0501)
				#define SPI_GETFONTSMOOTHINGTYPE            0x200A
				#define FE_FONTSMOOTHINGSTANDARD            0x0001
				#define FE_FONTSMOOTHINGCLEARTYPE           0x0002
				#endif
				fontSmoothingType = FE_FONTSMOOTHINGSTANDARD;
				SystemParametersInfo(SPI_GETFONTSMOOTHINGTYPE,
									 0,
									 &fontSmoothingType,
									 0);
				if ((fontSmoothingType & FE_FONTSMOOTHINGCLEARTYPE) != 0)
					textRenderingMode = TRM_WITH_ANTIALIASING_CLEARTYPE;
				else
					textRenderingMode = TRM_WITH_ANTIALIASING_NORMAL;
			}
		}
		}
		break;
	case TextRenderingHintSingleBitPerPixelGridFit:
		textRenderingMode = TRM_WITHOUT_ANTIALIASING;
		break;
	case TextRenderingHintSingleBitPerPixel:
		textRenderingMode = TRM_WITHOUT_ANTIALIASING;
		break;
	case TextRenderingHintAntiAliasGridFit:
		textRenderingMode = TRM_WITH_ANTIALIASING_NORMAL;
		break;
	case TextRenderingHintAntiAlias:
		textRenderingMode = TRM_WITH_ANTIALIASING_NORMAL;
		break;
	case TextRenderingHintClearTypeGridFit:
		textRenderingMode = TRM_WITH_ANTIALIASING_CLEARTYPE;
		break;
	default:
		assert(false);
		break;
	}

	return (TextRenderingMode) (textRenderingMode | (fHighQualityTextRenderingMode & (TRM_CONSTANT_SPACING|TRM_LEGACY_ON|TRM_LEGACY_OFF)));
}

/** apply text constant scaling */ 
void VWinGDIPlusGraphicContext::_ApplyTextScale(const VPoint& inTextOrig)
{
	if (fTextScaleTransformBackup == NULL)
		fTextScaleTransformBackup = new VAffineTransform();
	GetTransform( *fTextScaleTransformBackup);
	VAffineTransform xform = *fTextScaleTransformBackup;
	xform.Translate( inTextOrig.GetX(), inTextOrig.GetY());
	xform.Scale( fTextScale, fTextScale);
	SetTransform( xform);
}

/** revert text constant scaling */
void VWinGDIPlusGraphicContext::_RevertTextScale()
{
	xbox_assert(fTextScaleTransformBackup);
	SetTransform( *fTextScaleTransformBackup);
}


void VWinGDIPlusGraphicContext::SetFont( VFont *inFont, GReal inScale)
{
#if 0	// sc 23/10/2006 gros pb de perf
	HDC dc=_GetHDC();
	inFont->ApplyToPort(dc, inScale);
	_ReleaseHDC(dc);
#endif
	//JQ 30/06/2010: slightly optimize SetFont

	if (inFont == fTextFont && inScale == 1.0f)
		return;

	if (inScale != 1.0f)
		CopyRefCountable( &fTextFont, inFont->DeriveFontSize( inFont->GetSize() * inScale));
	else
		CopyRefCountable( &fTextFont, inFont);

	fWhiteSpaceWidth = 0.0f;
#if VERSIONDEBUG
	//LOGFONTW logfont;
	//fTextFont->GetGDIPlusFont()->GetLogFontW( fGDIPlusGraphics, &logfont);
#endif

	/*
	VFontFace fontface = inFont->GetFace();

	int gdiplusface = FontStyleRegular;

	if (fontface == KFS_NORMAL)
		gdiplusface = FontStyleRegular;
	else
	{
		if (fontface & KFS_ITALIC)
		{
			if (fontface & KFS_BOLD)
				gdiplusface = FontStyleBoldItalic;
			else
				gdiplusface = FontStyleItalic;
		}
		else
		{
			if (fontface & KFS_BOLD)
				gdiplusface = FontStyleBold;
		}

		if (fontface & KFS_UNDERLINE)
			gdiplusface |= FontStyleUnderline;
		else
			if (fontface & KFS_STRIKEOUT)
				gdiplusface |= FontStyleStrikeout;
	}
	fTextFont = new Gdiplus::Font(inFont->GetName().GetCPointer(), inFont->GetSize()*inScale, gdiplusface, UnitPoint);
	fTextScale = 1.0f;
	*/

}


/** determine current white space width 
@remarks
	used for custom kerning
*/
void	VWinGDIPlusGraphicContext::_ComputeWhiteSpaceWidth() const
{
	if (fWhiteSpaceWidth != 0.0f)
		return;
	GReal kerning = fCharKerning + fCharSpacing;
	if (kerning == 0.0f)
		return;

	//precompute white space width 
	//(to deal with constant kerning because GetCharWidth return 0 if character is white space)
	VRect bounds1,bounds2;
	GReal kerningBackup = fCharKerning;
	GReal spacingBackup = fCharSpacing;
	fCharKerning = fCharSpacing = 0.0;
	GetTextBoundsTypographic("x x",bounds1);
	GetTextBoundsTypographic("xx", bounds2);
	fCharKerning = kerningBackup;
	fCharSpacing = spacingBackup;
	fWhiteSpaceWidth = (bounds1.GetWidth()-bounds2.GetWidth())/fTextScale;
}


VFont*	VWinGDIPlusGraphicContext::RetainFont()
{
	//JQ 30/06/2010: optimized font management
	if (fTextFont)
		fTextFont->Retain();
	else 
		//ensure we return a valid VFont if outside BeginUsingContext/EndUsingContext
		return VFont::RetainStdFont(STDF_CAPTION);
	return fTextFont;
}


void VWinGDIPlusGraphicContext::SetTextPosTo(const VPoint& inHwndPos)
{
	fTextRect->X = inHwndPos.GetX();
	fTextRect->Y = inHwndPos.GetY();
}


void VWinGDIPlusGraphicContext::SetTextPosBy(GReal inHoriz, GReal inVert)
{
	fTextRect->X += inHoriz;
	fTextRect->Y += inVert;
}


void VWinGDIPlusGraphicContext::SetTextAngle(GReal inAngle)
{
}

void VWinGDIPlusGraphicContext::GetTextPos (VPoint& outHwndPos) const
{
	outHwndPos.SetX(fTextRect->X);
	outHwndPos.SetY(fTextRect->Y);
}


DrawingMode VWinGDIPlusGraphicContext::SetTextDrawingMode(DrawingMode inMode, VBrushFlags* ioFlags)
{
		
	return DM_PLAIN;
}


void VWinGDIPlusGraphicContext::SetSpaceKerning(GReal inSpaceExtra)
{
	if (fCharSpacing == inSpaceExtra)
		return;
	fCharSpacing = inSpaceExtra;
	fWhiteSpaceWidth = 0.0f;
}


void VWinGDIPlusGraphicContext::SetCharKerning(GReal inSpaceExtra)
{
	if (fCharKerning == inSpaceExtra)
		return;
	fCharKerning = inSpaceExtra;
	fWhiteSpaceWidth = 0.0f;
}


uBYTE VWinGDIPlusGraphicContext::SetAlphaBlend(uBYTE inAlphaBlend)
{
	return 0;
}


void VWinGDIPlusGraphicContext::SetPixelForeColor(const VColor& inColor)
{
}


void VWinGDIPlusGraphicContext::SetPixelBackColor(const VColor& inColor)
{
}


#pragma mark-

GReal VWinGDIPlusGraphicContext::GetTextWidth(const VString& inString, bool inRTL) const 
{
	StUseContext_NoRetain	context(this);

	if ((fTextRenderingMode & TRM_LEGACY_ON) || ((!(fTextRenderingMode & TRM_LEGACY_OFF)) && (!fTextFont->IsTrueTypeFont())))
	{
		VRect bounds;
		_GetLegacyTextBoxSize(inString, bounds, TLM_NORMAL | (inRTL ? TLM_RIGHT_TO_LEFT : 0));
		return bounds.GetWidth();
	}

	PointF origin(0.0f, 0.0f);
	RectF boundRect;

	Gdiplus::StringFormat	format (fTextMeasuringMode==TMM_GENERICTYPO ? Gdiplus::StringFormat::GenericTypographic(): Gdiplus::StringFormat::GenericDefault());
	//const Gdiplus::StringFormat* format=Gdiplus::StringFormat::GenericDefault();
	//if(fTextMeasuringMode==TMM_GENERICTYPO)
	//	format = Gdiplus::StringFormat::GenericTypographic();
	format.SetLineAlignment(StringAlignmentNear);
	format.SetAlignment(StringAlignmentNear);
	if (inRTL)
		format.SetFormatFlags( format.GetFormatFlags() | StringFormatFlagsDirectionRightToLeft);

	if(fGDIPlusGraphics->MeasureString((LPCWSTR) inString.GetCPointer(),inString.GetLength(),fTextFont->GetImpl().GetGDIPlusFont(),origin,&format,&boundRect)!=Gdiplus::Ok)
		return 0;
	else
		return boundRect.Width*fTextScale;
}

void VWinGDIPlusGraphicContext::GetTextBounds( const VString& inString, VRect& oRect) const
{
	StUseContext_NoRetain	context(this);

	if ((fTextRenderingMode & TRM_LEGACY_ON) || ((!(fTextRenderingMode & TRM_LEGACY_OFF)) && (!fTextFont->IsTrueTypeFont())))
	{
		oRect.SetEmpty();
		_GetLegacyTextBoxSize(inString, oRect, TLM_NORMAL);
		return;
	}

	PointF origin(0.0f, 0.0f);
	RectF boundRect;

	const Gdiplus::StringFormat* format=Gdiplus::StringFormat::GenericDefault();
	if(fTextMeasuringMode==TMM_GENERICTYPO)
		format = Gdiplus::StringFormat::GenericTypographic();

	if(fGDIPlusGraphics->MeasureString((LPCWSTR) inString.GetCPointer(),inString.GetLength(),fTextFont->GetImpl().GetGDIPlusFont(),origin,format,&boundRect)!=Gdiplus::Ok)
		oRect = VRect(0, 0, 0, 0);
	else
		oRect = VRect(0 , 0, boundRect.Width*fTextScale, boundRect.Height*fTextScale);
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
//	this method return the typographic text line bounds 
//@note
//	this method is used by SVG component
void VWinGDIPlusGraphicContext::GetTextBoundsTypographic( const VString& inString, VRect& oRect, TextLayoutMode inLayoutMode) const
{
	sLONG	length = inString.GetLength();
	if (length == 0)
	{
		oRect = VRect(0,0,0,0);
		return;
	}

	StUseContext_NoRetain	context(this);
	
	if ((fTextRenderingMode & TRM_LEGACY_ON) || ((!(fTextRenderingMode & TRM_LEGACY_OFF)) && (!fTextFont->IsTrueTypeFont())))
	{
		oRect.SetEmpty();
		_GetLegacyTextBoxSize(inString, oRect, inLayoutMode);
		return;
	}


	Gdiplus::RectF	bounds; 
	Gdiplus::PointF pos(0,0); 
	StringFormat	*format = NULL;
	format = Gdiplus::StringFormat::GenericTypographic()->Clone();
	xbox_assert(format);
	format->SetFormatFlags(	Gdiplus::StringFormat::GenericTypographic()->GetFormatFlags()
							|
							StringFormatFlagsMeasureTrailingSpaces
							|
							((inLayoutMode & TLM_RIGHT_TO_LEFT) ? StringFormatFlagsDirectionRightToLeft : 0)
							|
							((inLayoutMode & TLM_VERTICAL) ? StringFormatFlagsDirectionVertical : 0));
	
	GReal kerning = (fCharKerning+fCharSpacing)/fTextScale;
	if (kerning 
		&&
		(!(inLayoutMode & TLM_RIGHT_TO_LEFT))
		)
	{
		//measure string width taking account custom kerning
		//(for now work only with left to right or top to bottom text)
		
		_ComputeWhiteSpaceWidth();
		const UniChar *c = inString.GetCPointer();
		while (*c)
		{
			if (inLayoutMode & TLM_VERTICAL)
				bounds.Height += ((*c) == 0x20 ? fWhiteSpaceWidth : GetCharWidth( *c)/fTextScale);
			else
				bounds.Width  += ((*c) == 0x20 ? fWhiteSpaceWidth : GetCharWidth( *c)/fTextScale);
			c++;
		}
		if (length > 1)
		{
			if (inLayoutMode & TLM_VERTICAL)
				bounds.Height += (length-1)*kerning;
			else
				bounds.Width += (length-1)*kerning;
		}
	}
	else
		if (fGDIPlusGraphics->MeasureString((LPCWSTR) inString.GetCPointer(),
											length,
											fTextFont->GetImpl().GetGDIPlusFont(),
											pos,
											format,
											&bounds)!=Gdiplus::Ok)
			bounds.Width = bounds.Height = 0.0f; 
	delete format;

	oRect = VRect( 0, 0, bounds.Width*fTextScale, bounds.Height*fTextScale);
}


GReal VWinGDIPlusGraphicContext::GetCharWidth(UniChar inChar) const
{
	VStr4	charString(inChar);
	return GetTextWidth(charString);
}


GReal VWinGDIPlusGraphicContext::GetTextHeight(bool inIncludeExternalLeading) const
{
	StUseContext_NoRetain	context(this);

	TEXTMETRICW	textMetrics;
	HDC dc=fGDIPlusGraphics->GetHDC();
	assert(dc!=NULL);
	::GetTextMetricsW(dc, &textMetrics);
	fGDIPlusGraphics->ReleaseHDC(dc);
	//JQ 21/06/2010: fixed text height 
	//(text height = ascent + descent only because internal leading is included in ascent & external leading is outside text cf MSDN)
	return (textMetrics.tmAscent + textMetrics.tmDescent + (inIncludeExternalLeading ? textMetrics.tmExternalLeading : 0))*fTextScale;
}

sLONG VWinGDIPlusGraphicContext::GetNormalizedTextHeight(bool inIncludeExternalLeading) const
{
	StUseContext_NoRetain	context(this);

	TEXTMETRICW	textMetrics;
	HDC dc=fGDIPlusGraphics->GetHDC();
	assert(dc!=NULL);
	::GetTextMetricsW(dc, &textMetrics);
	fGDIPlusGraphics->ReleaseHDC(dc);
	//JQ 21/06/2010: fixed text height 
	//(text height = ascent + descent only because internal leading is included in ascent & external leading is outside text cf MSDN)
	return (textMetrics.tmAscent + textMetrics.tmDescent + (inIncludeExternalLeading ? textMetrics.tmExternalLeading : 0))*fTextScale;
}


#pragma mark-

void VWinGDIPlusGraphicContext::NormalizeContext()
{
}


void VWinGDIPlusGraphicContext::SaveContext()
{
	fSaveState.push(fGDIPlusGraphics->Save());
}


void VWinGDIPlusGraphicContext::RestoreContext()
{
	Gdiplus::GraphicsState& cur = fSaveState.top();
	fGDIPlusGraphics->Restore(cur);
	fSaveState.pop();
}


#pragma mark-

void VWinGDIPlusGraphicContext::DrawTextBox(const VString& inString, AlignStyle inHAlign, AlignStyle inVAlign, const VRect& inHwndBounds, TextLayoutMode inOptions)
{
	if ((fTextRenderingMode & TRM_LEGACY_ON) || ((!(fTextRenderingMode & TRM_LEGACY_OFF)) && (!fTextFont->IsTrueTypeFont())))
	{
		_DrawLegacyTextBox(inString, inHAlign, inVAlign, inHwndBounds, inOptions);
		return;
	}
	GReal	x = inHwndBounds.GetX();
	GReal	y = inHwndBounds.GetY();
	GReal	w = inHwndBounds.GetWidth();
	GReal	h = inHwndBounds.GetHeight();
	
	if (w <= 0 || h <= 0)
		return;
	
	sLONG	length = inString.GetLength();
	if (length == 0)
		return;
	
	StUseContext_NoRetain	context(this);

	Gdiplus::StringFormat	format (fTextMeasuringMode==TMM_GENERICTYPO ? Gdiplus::StringFormat::GenericTypographic(): Gdiplus::StringFormat::GenericDefault());

	*fTextRect = Gdiplus::RectF(inHwndBounds.GetX(), inHwndBounds.GetY(), inHwndBounds.GetWidth(), inHwndBounds.GetHeight());
	
	switch (inHAlign)
	{
		case AL_LEFT:
			format.SetAlignment((inOptions & TLM_RIGHT_TO_LEFT) ? StringAlignmentFar : StringAlignmentNear);
			break;

		case AL_DEFAULT:
			format.SetAlignment(StringAlignmentNear);
			break;
			
		case AL_CENTER:
			format.SetAlignment(StringAlignmentCenter);
			break;
			
		case AL_JUST:
			// tbd
			break;
			
		case AL_RIGHT:
			format.SetAlignment((inOptions & TLM_RIGHT_TO_LEFT) ? StringAlignmentNear : StringAlignmentFar);
			break;
			
		default:
			assert(false);
			break;
	}
	
	switch (inVAlign)
	{
		case AL_LEFT:
		case AL_DEFAULT:
			format.SetLineAlignment(StringAlignmentNear);
			break;
			
		case AL_CENTER:
			format.SetLineAlignment(StringAlignmentCenter);
			break;
			
		case AL_JUST:
			// tbd
			break;
			
		case AL_RIGHT:
			format.SetLineAlignment(StringAlignmentFar);
			break;
			
		default:
			assert(false);
			break;
	}
	
	if ((inOptions & TLM_DONT_WRAP) != 0)
		format.SetFormatFlags(StringFormatFlagsNoWrap);

	if ((inOptions & TLM_TRUNCATE_END_IF_NECESSARY) != 0)
		format.SetTrimming(StringTrimmingEllipsisCharacter);
	else if ((inOptions & TLM_TRUNCATE_MIDDLE_IF_NECESSARY) != 0)
		format.SetTrimming(StringTrimmingEllipsisPath);
	else
		format.SetTrimming(StringTrimmingNone);

	if ((inOptions & TLM_VERTICAL) != 0)
		format.SetFormatFlags(format.GetFormatFlags() | StringFormatFlagsDirectionVertical | StringFormatFlagsDirectionRightToLeft);
	else if ((inOptions & TLM_RIGHT_TO_LEFT) != 0)
		format.SetFormatFlags(format.GetFormatFlags() | StringFormatFlagsDirectionRightToLeft);

	Status status;
	if (fTextScale != 1.0f)
	{
		VPoint pos = VPoint(fTextRect->X, fTextRect->Y);
		_ApplyTextScale( pos);

		//in order to ensure text will not be cropped, normalize height to ceil(height+font size) 
		Gdiplus::RectF bounds(0,0,fTextRect->Width/fTextScale, fTextRect->Height/fTextScale);

		SaveClip();
		ClipRect(VRect(0, 0, bounds.Width, bounds.Height));

		//in order to ensure text will not be cropped, normalize height to ceil(height+font size) 
		GReal newHeight = ceil(fTextFont->GetSize()+bounds.Height); //we need to add font size to ensure last line is not cropped
		if (inVAlign == AL_CENTER)
			bounds.Y -= (newHeight-bounds.Height)*0.5f;
		else if (inVAlign == AL_BOTTOM)
			bounds.Y = bounds.Y+bounds.Height-newHeight;
		bounds.Height = newHeight;
		status = fGDIPlusGraphics->DrawString((LPCWSTR) inString.GetCPointer(), length, fTextFont->GetImpl().GetGDIPlusFont(), bounds, &format, _GetTextBrush());

		RestoreClip();

		_RevertTextScale();
	}
	else
	{
		//in order to ensure text will not be cropped, normalize height to ceil(height+font size) 
		Gdiplus::RectF bounds(fTextRect->X, fTextRect->Y, fTextRect->Width, fTextRect->Height);

		SaveClip();
		ClipRect(VRect(bounds.X, bounds.Y, bounds.Width, bounds.Height));

		GReal newHeight = ceil(fTextFont->GetSize()+bounds.Height); //we need to add font size to ensure last line is not cropped
		if (inVAlign == AL_CENTER)
			bounds.Y -= (newHeight-bounds.Height)*0.5f;
		else if (inVAlign == AL_BOTTOM)
			bounds.Y = bounds.Y+bounds.Height-newHeight;
		bounds.Height = newHeight;
		status = fGDIPlusGraphics->DrawString((LPCWSTR) inString.GetCPointer(), length, fTextFont->GetImpl().GetGDIPlusFont(), bounds, &format, _GetTextBrush());

		RestoreClip();
	}
}



//return text box bounds
//@remark:
// if input bounds width is not null, use it as max line wrapping width (or max text height in vertical layout mode)
// if input bounds height is not null, use it as max text height (or max line wrapping width in vertical layout mode)
void VWinGDIPlusGraphicContext::GetTextBoxBounds( const VString& inString, VRect& ioHwndBounds, TextLayoutMode inOptions)
{
	if ((fTextRenderingMode & TRM_LEGACY_ON) || ((!(fTextRenderingMode & TRM_LEGACY_OFF)) && (!fTextFont->IsTrueTypeFont())))
	{
		_GetLegacyTextBoxSize(inString, ioHwndBounds, inOptions);
		return;
	}
	StringFormat	format (fTextMeasuringMode==TMM_GENERICTYPO ? Gdiplus::StringFormat::GenericTypographic(): Gdiplus::StringFormat::GenericDefault());
	INT cp,line;
	Gdiplus::RectF bounds = Gdiplus::RectF(	ioHwndBounds.GetX(), 
											ioHwndBounds.GetY(), 
											ioHwndBounds.GetWidth() != 0.0f ? ioHwndBounds.GetWidth() : 100000.0f, 
											ioHwndBounds.GetHeight() != 0.0f ? ioHwndBounds.GetHeight() : 100000.0f);
	
	if ((inOptions & TLM_DONT_WRAP) != 0)
		format.SetFormatFlags(StringFormatFlagsNoWrap);
	
	if ((inOptions & TLM_TRUNCATE_END_IF_NECESSARY) != 0)
		format.SetTrimming(StringTrimmingEllipsisCharacter);
	else if ((inOptions & TLM_TRUNCATE_MIDDLE_IF_NECESSARY) != 0)
		format.SetTrimming(StringTrimmingEllipsisPath);
	else
		format.SetTrimming(StringTrimmingNone);
	
	if ((inOptions & TLM_VERTICAL) != 0)
		format.SetFormatFlags(format.GetFormatFlags() | StringFormatFlagsDirectionVertical | StringFormatFlagsDirectionRightToLeft);
	else if ((inOptions & TLM_RIGHT_TO_LEFT) != 0)
		format.SetFormatFlags(format.GetFormatFlags() | StringFormatFlagsDirectionRightToLeft);
	
	Gdiplus::PointF pointTopLeft(ioHwndBounds.GetLeft(), ioHwndBounds.GetTop());
	Status status = fGDIPlusGraphics->MeasureString(	(LPCWSTR) inString.GetCPointer(), 
														inString.GetLength(), 
														fTextFont->GetImpl().GetGDIPlusFont(), 
														bounds,
														&format,
														&bounds
														,&cp,&line
														);
	//deal with math discrepancies: ensure last line of text is not clipped (so if height is 40.99 we convert it to 50)
	//(we cannot always round here to ceil(height) because if device has high scaling, diff between user space height & device space height would be too high
	// so to ensure GDIPlus will always draw the string we adjust height in DrawTextBox)
	if ((ceil(bounds.Height)-bounds.Height) <= 0.01)
		bounds.Height = ceil(bounds.Height);
	
	ioHwndBounds.SetX( bounds.X);
	ioHwndBounds.SetY( bounds.Y);
	ioHwndBounds.SetWidth( bounds.Width*fTextScale);
	ioHwndBounds.SetHeight( bounds.Height*fTextScale);
}



void VWinGDIPlusGraphicContext::DrawText(const VString& inString, TextLayoutMode inLayoutMode, bool inRTLAnchorRight)
{
	if ((fTextRenderingMode & TRM_LEGACY_ON) || ((!(fTextRenderingMode & TRM_LEGACY_OFF)) && (!fTextFont->IsTrueTypeFont())))
	{
		//legacy GDI drawing 

		DrawText( inString, VPoint(fTextRect->X, fTextRect->Y), inLayoutMode, inRTLAnchorRight);
		return;
	}

	DrawText( inString, VPoint( fTextRect->X, fTextRect->Y), inLayoutMode, inRTLAnchorRight);
	if (inLayoutMode & TLM_TEXT_POS_UPDATE_NOT_NEEDED)
		return;
	VRect bounds;
	GetTextBoundsTypographic( inString, bounds, inLayoutMode);
	if (inRTLAnchorRight && (inLayoutMode & TLM_RIGHT_TO_LEFT))
		fTextRect->X = fTextRect->X-bounds.GetWidth();
	else
		fTextRect->X = fTextRect->X+bounds.GetWidth();
}


//draw text at the specified position
//
//@param inString
//	text string
//@param inLayoutMode
//  layout mode (here only TLM_VERTICAL and TLM_RIGHT_TO_LEFT are used)
//
//@remark: this method does not make any layout formatting 
//		   text is drawed on a single line from the specified starting coordinate
//		   (which is relative to the first glyph horizontal or vertical baseline)
//		   useful when you want to position exactly a text at a specified coordinate
//		   without taking account extra spacing due to layout formatting (which is implementation variable)
//@note: this method is used by SVG component
void VWinGDIPlusGraphicContext::DrawText(const VString& inString, const VPoint& inPos, TextLayoutMode inLayoutMode, bool inRTLAnchorRight)
{
	GReal	x = inPos.GetX();
	GReal	y = inPos.GetY();
	
	sLONG	length = inString.GetLength();
	if (length == 0)
		return;
	
	StUseContext_NoRetain	context(this);

	if ((fTextRenderingMode & TRM_LEGACY_ON) || ((!(fTextRenderingMode & TRM_LEGACY_OFF)) && (!fTextFont->IsTrueTypeFont())))
	{
		//legacy GDI drawing 

		HDC hdc = BeginUsingParentContext();
		if (hdc)
		{
			VWinGDIGraphicContext *gcGDI = new VWinGDIGraphicContext( hdc);
			gcGDI->SetTextPosTo( inPos);
			gcGDI->DrawText( inString, inLayoutMode, inRTLAnchorRight);
			gcGDI->Release();
		}
		EndUsingParentContext(hdc);
		return;
	}


	//JQ 13/08/2008: fixed decorations
	//@remarks: DrawDriverString does not draw outline or strikeout decorations (GDI+ bug ?)
	//			so use DrawString instead and set manually baseline offset

	//adjust position with baseline offset 
	//(for vertical layout, we assume vertical baseline is at the middle of the cell size
	// which is compliant with ideographic or vertically displayed latin characters)
	Gdiplus::FontFamily *family = new Gdiplus::FontFamily();
	Gdiplus::Font *fontNative = fTextFont->GetImpl().GetGDIPlusFont();
	Status status = fontNative->GetFamily(family);
	GReal offset = (inLayoutMode & TLM_VERTICAL) != 0 ? 
						fontNative->GetSize() * 0.5f 
						: 
						fontNative->GetSize() / family->GetEmHeight(fontNative->GetStyle()) * family->GetCellAscent(fontNative->GetStyle());
	Gdiplus::PointF pos(x,y);
	if (fTextScale != 1.0f)
	{
		_ApplyTextScale( VPoint(x,y));
		pos.X = pos.Y = 0.0;
	}
	
	if ((inLayoutMode & TLM_VERTICAL) != 0)
		pos.X -= offset;
	else
		pos.Y -= offset;
	if (family)
		delete family;

	bool isAnchorRight = inRTLAnchorRight && (inLayoutMode & TLM_RIGHT_TO_LEFT) != 0;
	if (isAnchorRight)
	{
		VRect bounds;
		GetTextBoundsTypographic( inString, bounds, inLayoutMode);
		pos.X -= bounds.GetWidth();
	}

	//use typographic format in order to disable text box adjustment
	Gdiplus::StringFormat	*format = NULL;
	if ((inLayoutMode & (TLM_VERTICAL|TLM_RIGHT_TO_LEFT)) != 0)
	{
		format = Gdiplus::StringFormat::GenericTypographic()->Clone();
		if (format)
			format->SetFormatFlags(	Gdiplus::StringFormat::GenericTypographic()->GetFormatFlags() 
									| 
									((inLayoutMode & TLM_VERTICAL) ? StringFormatFlagsDirectionVertical : 0)
									| 
									((inLayoutMode & TLM_RIGHT_TO_LEFT) ? StringFormatFlagsDirectionRightToLeft : 0));
	}

	//draw string
	GReal kerning = (fCharKerning+fCharSpacing)/fTextScale;
	if (kerning 
		&&
		(!(inLayoutMode & TLM_RIGHT_TO_LEFT))
		&& 
		length > 1)
	{
		//draw string with custom char kerning
		//(for now work only with left to right or top to bottom text)
		_ComputeWhiteSpaceWidth();
		const UniChar *c = inString.GetCPointer();
		while (*c)
		{
			if (*c != 0x20)
				fGDIPlusGraphics->DrawString(
						(LPCWSTR) c, 
						1, 
						fontNative, 
						pos,
						format ? format : Gdiplus::StringFormat::GenericTypographic(),
						_GetTextBrush());
			if (inLayoutMode & TLM_VERTICAL)
				pos.Y += ((*c) == 0x20 ? fWhiteSpaceWidth : GetCharWidth( *c)/fTextScale)+kerning;
			else
				pos.X += ((*c) == 0x20 ? fWhiteSpaceWidth : GetCharWidth( *c)/fTextScale)+kerning;

			c++;
		}
	}
	else
		fGDIPlusGraphics->DrawString(
				(LPCWSTR) inString.GetCPointer(), 
				length, 
				fontNative, 
				pos,
				format ? format : Gdiplus::StringFormat::GenericTypographic(),
				_GetTextBrush());

	if (format)
		delete format;

	if (fTextScale != 1.0f)
		_RevertTextScale();
}

#pragma mark-

void VWinGDIPlusGraphicContext::FrameRect(const VRect& inHwndBounds)
{
	Status	status;
	StUseContext_NoRetain	context(this);

	if (fShapeCrispEdgesEnabled)
	{
		VRect bounds( inHwndBounds);
		CEAdjustRectInTransformedSpace( bounds);

		status = fGDIPlusGraphics->DrawRectangle(fStrokePen,GDIPLUS_RECT(bounds));
		return;
	}
	status = fGDIPlusGraphics->DrawRectangle(fStrokePen,GDIPLUS_RECT(inHwndBounds));
}


void VWinGDIPlusGraphicContext::FrameOval(const VRect& inHwndBounds)
{
	StUseContext_NoRetain	context(this);

	if (fShapeCrispEdgesEnabled)
	{
		VRect bounds( inHwndBounds);
		CEAdjustRectInTransformedSpace( bounds);

		Status	status = fGDIPlusGraphics->DrawEllipse(fStrokePen,GDIPLUS_RECT(bounds));
		return;
	}

	Status	status = fGDIPlusGraphics->DrawEllipse(fStrokePen,GDIPLUS_RECT(inHwndBounds));
}


void VWinGDIPlusGraphicContext::FrameRegion(const VRegion& inHwndRegion)
{
	//gdiplus cannot frame region so fill 
	FillRegion( inHwndRegion);
}

/** build native polygon */
Gdiplus::PointF *VWinGDIPlusGraphicContext::_BuildPolygonNative( const VPolygon& inPolygon, Gdiplus::PointF *inBuffer, bool inFillOnly)
{
	sLONG pc = 0;
	VPoint pt;
	Gdiplus::PointF *pf = NULL;
	pc = inPolygon.GetPointCount();
	pf = inBuffer ? inBuffer : new Gdiplus::PointF[pc];
	for(VIndex n=0;n<pc;n++) 
	{
		inPolygon.GetNthPoint( n+1, pt);
		if (fShapeCrispEdgesEnabled)
		{
			CEAdjustPointInTransformedSpace( pt, inFillOnly);
			pf[n] = Gdiplus::PointF( pt.GetX(), pt.GetY());
		}
		else
			pf[n] = Gdiplus::PointF(pt.x, pt.y);	
	}
	return pf;
}

void VWinGDIPlusGraphicContext::FramePolygon(const VPolygon& inHwndPolygon)
{
	StUseContext_NoRetain	context(this);

	sLONG pc = inHwndPolygon.GetPointCount();
	Gdiplus::PointF pointsMemStack[16];
	Gdiplus::PointF *pf = pc <= 16 ? _BuildPolygonNative( inHwndPolygon, pointsMemStack) : _BuildPolygonNative( inHwndPolygon);
	if (pf)
	{
		fGDIPlusGraphics->DrawPolygon(fStrokePen, pf, pc);
		if (pc > 16)
			delete [] pf;
	}
}


void VWinGDIPlusGraphicContext::FrameBezier(const VGraphicPath& inHwndBezier)
{
	FramePath( inHwndBezier);
}

void VWinGDIPlusGraphicContext::FramePath(const VGraphicPath& inHwndPath)
{
	StUsePath thePath( static_cast<VGraphicContext *>(this), &inHwndPath);

	if (fDrawingMode == DM_SHADOW)
	{
		const Gdiplus::GraphicsPath *path = *(thePath.Get());
		Gdiplus::GraphicsPath *shadow_path = path->Clone();

		Matrix m;
		m.Translate(fShadowHOffset, fShadowVOffset);
		shadow_path->Transform(&m);
        
		Gdiplus::Pen *pen = fStrokePen->Clone();
		pen->SetColor(Color((SHADOW_ALPHA/255.0*fGlobalAlpha)*255,SHADOW_R,SHADOW_G,SHADOW_B));
		fGDIPlusGraphics->DrawPath(pen, shadow_path);

		delete shadow_path;
		delete pen;
	}
	StUseContext_NoRetain	context(this);
	fGDIPlusGraphics->DrawPath(fStrokePen, *(thePath.Get()));
}



void VWinGDIPlusGraphicContext::FillRect(const VRect& _inHwndBounds)
{
	StUseContext_NoRetain	context(this);
	VRect inHwndBounds(_inHwndBounds);

	if (fShapeCrispEdgesEnabled)
		CEAdjustRectInTransformedSpace( inHwndBounds, true);

	if (fDrawingMode == DM_SHADOW)
	{
		const Gdiplus::RectF shadow_rect(inHwndBounds.GetLeft()+fShadowHOffset, inHwndBounds.GetTop()+fShadowVOffset, inHwndBounds.GetWidth(), inHwndBounds.GetHeight());

     	Gdiplus::SolidBrush brush(Color((SHADOW_ALPHA/255.0*fGlobalAlpha)*255,SHADOW_R,SHADOW_G,SHADOW_B));
		fGDIPlusGraphics->FillRectangle(&brush,shadow_rect);
	}

	if (fFillBrush->GetType() == BrushTypePathGradient
		&&
		fFillBrushGradientClamp)
	{
		//fill shape outside gradient path with end stop color
		if (fFillBrushGradientPath)
		{
			SaveClip();
			fGDIPlusGraphics->SetClip( fFillBrushGradientPath, Gdiplus::CombineModeExclude);
		}

		fGDIPlusGraphics->FillRectangle(fFillBrushGradientClamp,GDIPLUS_RECT(inHwndBounds));

		if (fFillBrushGradientPath)
			RestoreClip();
	}

	Status	status = fGDIPlusGraphics->FillRectangle(fFillBrush,GDIPLUS_RECT(inHwndBounds));
}


void VWinGDIPlusGraphicContext::FillOval(const VRect& _inHwndBounds)
{
	StUseContext_NoRetain	context(this);
	VRect inHwndBounds(_inHwndBounds);

	if (fShapeCrispEdgesEnabled)
		CEAdjustRectInTransformedSpace( inHwndBounds, true);

	if (fFillBrush->GetType() == BrushTypePathGradient
		&&
		fFillBrushGradientClamp)
	{
		//fill shape outside gradient path with end stop color
		if (fFillBrushGradientPath)
		{
			SaveClip();
			fGDIPlusGraphics->SetClip( fFillBrushGradientPath, Gdiplus::CombineModeExclude);
		}

		fGDIPlusGraphics->FillEllipse(fFillBrushGradientClamp,GDIPLUS_RECT(inHwndBounds));

		if (fFillBrushGradientPath)
			RestoreClip();
	}

	Status	status = fGDIPlusGraphics->FillEllipse(fFillBrush,GDIPLUS_RECT(inHwndBounds));
}


void VWinGDIPlusGraphicContext::FillRegion(const VRegion& inHwndRegion)
{
	StUseContext_NoRetain	context(this);
	Gdiplus::Region region(inHwndRegion);

	if (fFillBrush->GetType() == BrushTypePathGradient
		&&
		fFillBrushGradientClamp)
	{
		//fill shape outside gradient path with end stop color
		if (fFillBrushGradientPath)
		{
			SaveClip();
			fGDIPlusGraphics->SetClip( fFillBrushGradientPath, Gdiplus::CombineModeExclude);
		}

		fGDIPlusGraphics->FillRegion(fFillBrushGradientClamp, &region);

		if (fFillBrushGradientPath)
			RestoreClip();
	}

	fGDIPlusGraphics->FillRegion(fFillBrush, &region);
}


void VWinGDIPlusGraphicContext::FillPolygon(const VPolygon& inHwndPolygon)
{
	StUseContext_NoRetain	context(this);

	sLONG pc = inHwndPolygon.GetPointCount();
	Gdiplus::PointF pointsMemStack[16];
	Gdiplus::PointF *pf = pc <= 16 ? _BuildPolygonNative( inHwndPolygon, pointsMemStack, true) : _BuildPolygonNative( inHwndPolygon, NULL, true);
	if (pf)
	{
		if (fFillBrush->GetType() == BrushTypePathGradient
			&&
			fFillBrushGradientClamp)
		{
			//fill shape outside gradient path with end stop color
			if (fFillBrushGradientPath)
			{
				SaveClip();
				fGDIPlusGraphics->SetClip( fFillBrushGradientPath, Gdiplus::CombineModeExclude);
			}

			fGDIPlusGraphics->FillPolygon(fFillBrushGradientClamp, pf, pc, fFillMode);

			if (fFillBrushGradientPath)
				RestoreClip();
		}

		fGDIPlusGraphics->FillPolygon(fFillBrush, pf, pc, fFillMode);

		if (pc > 16)
			delete [] pf;
	}
}


void VWinGDIPlusGraphicContext::FillBezier(VGraphicPath& inHwndBezier)
{
	FillPath( inHwndBezier);
}

void VWinGDIPlusGraphicContext::FillPath(VGraphicPath& inHwndPath)
{
	StUseContext_NoRetain	context(this);
	StUsePath thePath( static_cast<VGraphicContext *>(this), &inHwndPath, true);

	const Gdiplus::GraphicsPath *path = *(thePath.Get());
	if (fDrawingMode == DM_SHADOW)
	{
		Gdiplus::GraphicsPath *shadow_path = path->Clone();

		Matrix m;
		m.Translate(fShadowHOffset, fShadowVOffset);
		shadow_path->Transform(&m);
        
		Gdiplus::SolidBrush brush(Color((SHADOW_ALPHA/255.0*fGlobalAlpha)*255,SHADOW_R,SHADOW_G,SHADOW_B));
		shadow_path->SetFillMode( fFillMode);
		fGDIPlusGraphics->FillPath(&brush, shadow_path);

		delete shadow_path;
	}
	FillRuleType fillRulePrevious = thePath.Get()->GetFillMode();
	if (fillRulePrevious != fFillRule)
		thePath.GetModifiable()->SetFillMode( fFillRule);

	if (fFillBrush->GetType() == BrushTypePathGradient
		&&
		fFillBrushGradientClamp)
	{
		//fill shape outside gradient path with end stop color
		if (fFillBrushGradientPath)
		{
			SaveClip();
			fGDIPlusGraphics->SetClip( fFillBrushGradientPath, Gdiplus::CombineModeExclude);
		}

		fGDIPlusGraphics->FillPath( fFillBrushGradientClamp, *(thePath.Get()));

		if (fFillBrushGradientPath)
			RestoreClip();
	}

	fGDIPlusGraphics->FillPath( fFillBrush, *(thePath.Get()));
	
	if (fillRulePrevious != fFillRule)
		thePath.GetModifiable()->SetFillMode( fillRulePrevious);
}


void VWinGDIPlusGraphicContext::DrawRect(const VRect& inHwndBounds)
{
	if (fShapeCrispEdgesEnabled)
	{
		StUseContext_NoRetain	context(this);

		VRect bounds( inHwndBounds);
		CEAdjustRectInTransformedSpace( bounds);

		fShapeCrispEdgesEnabled = false;
		FillRect(bounds);
		FrameRect(bounds);
		fShapeCrispEdgesEnabled = true;
	}
	else
	{
		FillRect(inHwndBounds);
		FrameRect(inHwndBounds);
	}
}


void VWinGDIPlusGraphicContext::DrawOval(const VRect& inHwndBounds)
{
	if (fShapeCrispEdgesEnabled)
	{
		StUseContext_NoRetain	context(this);

		VRect bounds( inHwndBounds);
		CEAdjustRectInTransformedSpace( bounds);

		fShapeCrispEdgesEnabled = false;
		FillOval(bounds);
		FrameOval(bounds);
		fShapeCrispEdgesEnabled = true;
	}
	else
	{
		FillOval(inHwndBounds);
		FrameOval(inHwndBounds);
	}
}


void VWinGDIPlusGraphicContext::DrawRegion(const VRegion& inHwndRegion)
{
	FillRegion(inHwndRegion);
	//FrameRegion(inHwndRegion); //gdiplus cannot frame region
}


void VWinGDIPlusGraphicContext::DrawPolygon(const VPolygon& inHwndPolygon)
{
	FillPolygon(inHwndPolygon);
	FramePolygon(inHwndPolygon);
}


void VWinGDIPlusGraphicContext::DrawBezier(VGraphicPath& inHwndBezier)
{
	DrawPath( inHwndBezier);
}

void VWinGDIPlusGraphicContext::DrawPath(VGraphicPath& inHwndPath)
{
	if (fShapeCrispEdgesEnabled)
	{
		StUseContext_NoRetain	context(this);
		StUsePath thePath( static_cast<VGraphicContext *>(this), &inHwndPath);
		fShapeCrispEdgesEnabled = false;
		FillPath(*(thePath.GetModifiable()));
		FramePath(*(thePath.Get()));
		fShapeCrispEdgesEnabled = true;
	}
	else
	{
		FillPath(inHwndPath);
		FramePath(inHwndPath);
	}
}

void VWinGDIPlusGraphicContext::EraseRect(const VRect& inHwndBounds)
{
	assert(false);	// Not supported
}


void VWinGDIPlusGraphicContext::EraseRegion(const VRegion& inHwndRegion)
{
	assert(false);	// Not supported
}


void VWinGDIPlusGraphicContext::InvertRect(const VRect& inHwndBounds)
{
	StUseContext_NoRetain	context(this);
	
	RECT r = inHwndBounds;
	HDC dc=_GetHDC();
	::InvertRect(dc, &r);
	_ReleaseHDC(dc);
}


void VWinGDIPlusGraphicContext::InvertRegion(const VRegion& inHwndRegion)
{
	StUseContext_NoRetain	context(this);
	HDC dc=_GetHDC();
	::InvertRgn(dc, inHwndRegion);
	_ReleaseHDC(dc);
}


void VWinGDIPlusGraphicContext::DrawLine(const VPoint& inHwndStart, const VPoint& inHwndEnd)
{
	StUseContext_NoRetain	context(this);
	
	if (fShapeCrispEdgesEnabled)
	{
		VPoint _start, _end;
		_start = inHwndStart;
		_end = inHwndEnd;
		CEAdjustPointInTransformedSpace( _start);
		CEAdjustPointInTransformedSpace( _end);

		if (fDrawingMode == DM_SHADOW)
		{
			Gdiplus::Pen *pen = fStrokePen->Clone();
			VPoint p1, p2;
			VPoint offset (fShadowHOffset, fShadowVOffset);
			p1 = _start + offset;
			p2 = _end + offset;
			pen->SetColor(Color((SHADOW_ALPHA/255.0*fGlobalAlpha)*255,SHADOW_R,SHADOW_G,SHADOW_B));
			fGDIPlusGraphics->DrawLine(pen,GDIPLUS_POINT(p1),GDIPLUS_POINT(p2));
			delete pen;
		}
		Status	status = fGDIPlusGraphics->DrawLine(fStrokePen,GDIPLUS_POINT(_start),GDIPLUS_POINT(_end));
		return;
	}

	if (fDrawingMode == DM_SHADOW)
	{
		Gdiplus::Pen *pen = fStrokePen->Clone();
		VPoint p1, p2;
		VPoint offset (fShadowHOffset, fShadowVOffset);
		p1 = inHwndStart + offset;
		p2 = inHwndEnd + offset;
		pen->SetColor(Color((SHADOW_ALPHA/255.0*fGlobalAlpha)*255,SHADOW_R,SHADOW_G,SHADOW_B));
		fGDIPlusGraphics->DrawLine(pen,GDIPLUS_POINT(p1),GDIPLUS_POINT(p2));
		delete pen;
	}
	Status	status = fGDIPlusGraphics->DrawLine(fStrokePen,GDIPLUS_POINT(inHwndStart),GDIPLUS_POINT(inHwndEnd));
}
void VWinGDIPlusGraphicContext::DrawLine(const GReal inHwndStartX,const GReal inHwndStartY, const GReal inHwndEndX,const GReal inHwndEndY)
{
	StUseContext_NoRetain	context(this);

	if (fShapeCrispEdgesEnabled)
	{
		VPoint _start( inHwndStartX, inHwndStartY);
		VPoint _end( inHwndEndX, inHwndEndY);
		CEAdjustPointInTransformedSpace( _start);
		CEAdjustPointInTransformedSpace( _end);

		if (fDrawingMode == DM_SHADOW)
		{
			Gdiplus::Pen *pen = fStrokePen->Clone();
			pen->SetColor(Color((SHADOW_ALPHA/255.0*fGlobalAlpha)*255,SHADOW_R,SHADOW_G,SHADOW_B));
			fGDIPlusGraphics->DrawLine(pen,(REAL)(_start.GetX()+fShadowHOffset),(REAL)(_start.GetY()+fShadowVOffset),(REAL)(_end.GetX()+fShadowHOffset),(REAL)(_end.GetY()+fShadowVOffset));
			delete pen;
		}
		Status	status = fGDIPlusGraphics->DrawLine(fStrokePen,_start.GetX(),_start.GetY(),_end.GetX(),_end.GetY());
		return;
	}

	if (fDrawingMode == DM_SHADOW)
	{
		Gdiplus::Pen *pen = fStrokePen->Clone();
		pen->SetColor(Color((SHADOW_ALPHA/255.0*fGlobalAlpha)*255,SHADOW_R,SHADOW_G,SHADOW_B));
		fGDIPlusGraphics->DrawLine(pen,(REAL)(inHwndStartX+fShadowHOffset),(REAL)(inHwndStartY+fShadowVOffset),(REAL)(inHwndEndX+fShadowHOffset),(REAL)(inHwndEndY+fShadowVOffset));
		delete pen;
	}
	Status	status = fGDIPlusGraphics->DrawLine(fStrokePen,inHwndStartX,inHwndStartY,inHwndEndX,inHwndEndY);
}

void VWinGDIPlusGraphicContext::DrawLines(const GReal* inCoords, sLONG inCoordCount)
{
	if (inCoordCount<4)
		return;

	if (fShapeCrispEdgesEnabled)
	{
		fShapeCrispEdgesEnabled = false;

		VPoint last( inCoords[0], inCoords[1]);
		CEAdjustPointInTransformedSpace( last);

		for(VIndex n=2; n<inCoordCount; n+=2)
		{
			VPoint cur( inCoords[n], inCoords[n+1]);
			CEAdjustPointInTransformedSpace( cur);

			DrawLine( last, cur);

			last = cur;
		}

		fShapeCrispEdgesEnabled = true;
		return;
	}

	GReal lastX = inCoords[0];
	GReal lastY = inCoords[1];
	for(VIndex n=2;n<inCoordCount;n+=2)
	{
		DrawLine(lastX,lastY, inCoords[n],inCoords[n+1]);
		lastX = inCoords[n];
		lastY = inCoords[n+1];
	}
}


void VWinGDIPlusGraphicContext::DrawLineTo(const VPoint& inHwndEnd)
{
	DrawLine(fBrushPos.GetX(),fBrushPos.GetY(),inHwndEnd.GetX(),inHwndEnd.GetY());
	fBrushPos=inHwndEnd;
}


void VWinGDIPlusGraphicContext::DrawLineBy(GReal inWidth, GReal inHeight)
{
	DrawLine(fBrushPos.GetX(),fBrushPos.GetY(),fBrushPos.GetX()+inWidth,fBrushPos.GetY()+inHeight);
	fBrushPos.SetPosBy( inWidth,inHeight);
}


void VWinGDIPlusGraphicContext::MoveBrushTo(const VPoint& inHwndPos)
{	
	fBrushPos = inHwndPos;
}


void VWinGDIPlusGraphicContext::MoveBrushBy(GReal inWidth, GReal inHeight)
{	
	fBrushPos.SetPosBy(inWidth,inHeight);
}


void VWinGDIPlusGraphicContext::DrawIcon(const VIcon& inIcon, const VRect& inHwndBounds, PaintStatus /*inState*/, GReal /*inScale*/)
{
	StUseContext_NoRetain	context(this);

	Bitmap *bitmap = fCachedIcons[inIcon];
	if (bitmap == NULL)
	{
		bitmap = new Gdiplus::Bitmap(inIcon);
		fCachedIcons[inIcon] = bitmap;
		// **************************************
		// LP: Quick and dirty alpha icon : BEGIN
		// **************************************
		BitmapData* bitmapData = new BitmapData;
		VRect vr = inIcon.GetBounds();
		Gdiplus::Rect rect(vr.GetLeft(), vr.GetTop(), vr.GetWidth(), vr.GetHeight());
		bitmap->LockBits(&rect,ImageLockModeRead,PixelFormat32bppARGB,bitmapData);

		UINT* pixels = (UINT*)bitmapData->Scan0;

		UINT p=0;
		UINT m = 0x00ffffff;
		UINT i = 0;
		UINT h = inIcon.GetHeight();
		UINT w = inIcon.GetWidth();
		UINT Z = bitmapData->Stride / 4;
		for(UINT row = 0; row < h; row++)
		{
			for(UINT col = 0; col < w; col++)
			{
				i = row * Z + col;
				p = pixels[i] & m;
				if (p == m)
					pixels[i] = p;
			}
		}
		bitmap->UnlockBits(bitmapData);
		delete bitmapData;
		// **************************************
		// LP: Quick and dirty alpha icon :   END
		// **************************************

	}
	VRect vr = inIcon.GetBounds();
	fGDIPlusGraphics->DrawImage(bitmap, GDIPLUS_RECT(inHwndBounds),0,0,vr.GetWidth(), vr.GetHeight(), UnitPixel,NULL,NULL,NULL);
}
void VWinGDIPlusGraphicContext::DrawPicture (const VPicture& inPicture,const VRect& inHwndBounds,VPictureDrawSettings *inSet)
{
	StUseContext_NoRetain	context(this);
	inPicture.Draw(fGDIPlusGraphics,inHwndBounds,inSet);
}



//draw picture data
void VWinGDIPlusGraphicContext::DrawPictureData (const VPictureData *inPictureData, const VRect& inHwndBounds,VPictureDrawSettings *inSet)
{
	xbox_assert(inPictureData);
	
	StUseContext_NoRetain	context(this);
	VPictureDrawSettings drawSettings;

	//save current tranformation matrix
	VAffineTransform ctm;
	GetTransform(ctm);
	
	if (inPictureData->IsKind( sCodec_svg))
		//@remark: better to call this method for svg codec to avoid to create new VWinGDIPlusGraphicContext instance
		inPictureData->DrawInVGraphicContext( *this, inHwndBounds, inSet?inSet:&drawSettings);
	else
		inPictureData->DrawInGDIPlusGraphics( fGDIPlusGraphics, inHwndBounds, inSet?inSet:&drawSettings);
	
	//restore current transformation matrix
	SetTransform(ctm);
}


#if 0
void VWinGDIPlusGraphicContext::DrawPicture(const VOldPicture& inPicture, PictureMosaic inMosaicMode, const VRect& inHwndBounds, TransferMode inDrawingMode)
{
	VOldImageData*	image = inPicture.GetImage();
	if (image == NULL) return;
	
	VOldEmfData*	data = (VOldEmfData*) image;
	if (!testAssert(image->GetImageKind() == IK_EMF)) return;

	BOOL	result;
	StUseContext_NoRetain	context(this);
	
	if (inMosaicMode == PM_TILE)
	{
		Boolean	drawOnScreen = true;
		GReal	tileWidth = inPicture.GetWidth();
		GReal	tileHeight = inPicture.GetHeight();
		
		VRect	tileBounds(inHwndBounds);
		tileBounds.SetSizeTo(tileWidth, tileHeight);
		
		GReal	hIndex, vIndex;
		sLONG	hTileCount = 1 + inHwndBounds.GetWidth() / tileWidth;
		sLONG	vTileCount = 1 + inHwndBounds.GetHeight() / tileHeight;
		
		HDC	offscreenDC = ::CreateCompatibleDC(fContext);
		if (offscreenDC != NULL)
		{
			HBITMAP	offscreenBM = ::CreateCompatibleBitmap(offscreenDC, tileWidth, tileHeight);
			if (offscreenBM != NULL)
			{
				HBITMAP	savedBM = (HBITMAP) ::SelectObject(offscreenDC, offscreenBM);
				RECT r = tileBounds;
				result = ::PlayEnhMetaFile(offscreenDC, (HENHMETAFILE) data->GetData(), &r);
				
				for (vIndex = 0; vIndex < vTileCount; vIndex++)
				{
					for (hIndex = 0; hIndex < hTileCount; hIndex++)
					{
						::BitBlt(fContext, tileBounds.GetLeft(), tileBounds.GetTop(), tileWidth, tileHeight, offscreenDC, 0, 0, SRCCOPY);
						tileBounds.SetPosBy(tileWidth, 0);
					}
					
					tileBounds.SetPosBy(-tileWidth * hTileCount, tileHeight);
				}
				
				savedBM = (HBITMAP) ::SelectObject(offscreenDC, savedBM);
				result = ::DeleteObject(offscreenBM);
				drawOnScreen = false;
			}
			
			result = ::DeleteDC(offscreenDC);
		}
		
		if (drawOnScreen)
		{
			for (vIndex = 0; vIndex < vTileCount; vIndex++)
			{
				for (hIndex = 0; hIndex < hTileCount; hIndex++)
				{
					RECT r = tileBounds;
					result = ::PlayEnhMetaFile(fContext, (HENHMETAFILE) data->GetData(), &r);
					tileBounds.SetPosBy(tileWidth, 0);
				}
				
				tileBounds.SetPosBy(-tileWidth * hTileCount, tileHeight);
			}
		}
	}
	else
	{
		RECT r = inHwndBounds;
		result = ::PlayEnhMetaFile(fContext, (HENHMETAFILE) data->GetData(), &r);
	}
	
}
#endif
void VWinGDIPlusGraphicContext::DrawFileBitmap(const VFileBitmap *inFileBitmap, const VRect& inHwndBounds)
{
	fGDIPlusGraphics->DrawImage(inFileBitmap->fBitmap, GDIPLUS_RECT(inHwndBounds));
}


void VWinGDIPlusGraphicContext::DrawPictureFile(const VFile& inFile, const VRect& inHwndBounds)
{
	assert(false);
}


#pragma mark-

void VWinGDIPlusGraphicContext::SaveClip()
{
	StUseContext_NoRetain	context(this);
	/*
	HRGN	rgn = ::CreateRectRgn(0,0,0,0);
	sLONG	result = ::GetClipRgn(fContext, rgn);
	assert(rgn != NULL);
	
	if (result == 0)
	{
		// No clipping
		::DeleteObject(rgn);
		rgn = NULL;
	}
	*/
	Gdiplus::Region *clip=new Gdiplus::Region();
	fGDIPlusGraphics->GetClip(clip);
	fClipStack.Push(clip);
}


void VWinGDIPlusGraphicContext::RestoreClip()
{
	StUseContext_NoRetain	context(this);

	Gdiplus::Region* rgn=fClipStack.Pop();
	fGDIPlusGraphics->SetClip(rgn,CombineModeReplace);
	if(rgn)
		delete rgn;
	/*HRGN	rgn = fClipStack.Pop();
	
	sLONG	result = ::SelectClipRgn(fContext, rgn);

	Gdiplus::Region grgn(rgn);
	fGDIPlusGraphics->SetClip(&grgn, CombineModeReplace);
//	fGDIPlusDirectGraphics->SetClip(&grgn, CombineModeReplace);

	if (rgn != NULL)
		result = ::DeleteObject(rgn);
	*/
}


//add or intersect the specified clipping path with the current clipping path
void VWinGDIPlusGraphicContext::ClipPath (const VGraphicPath& inPath, Boolean inAddToPrevious, Boolean inIntersectToPrevious)
{
#if VERSIONDEBUG
	if (sDebugNoClipping)
	{ 
		fGDIPlusGraphics->ResetClip();
		return;
	}
#endif
	StUseContext_NoRetain	context(this);
	fGDIPlusGraphics->SetClip( inPath, inAddToPrevious ? Gdiplus::CombineModeUnion : (inIntersectToPrevious ? Gdiplus::CombineModeIntersect : Gdiplus::CombineModeReplace) );

	RevealClipping(fGDIPlusGraphics);
}


/** add or intersect the specified clipping path outline with the current clipping path 
@remarks
	this will effectively create a clipping path from the path outline 
	(using current stroke brush) and merge it with current clipping path,
	making possible to constraint drawing to the path outline
*/
void VWinGDIPlusGraphicContext::ClipPathOutline (const VGraphicPath& inPath, Boolean inAddToPrevious, Boolean inIntersectToPrevious)
{
	if (fStrokePen == NULL)
		return;
#if VERSIONDEBUG
	if (sDebugNoClipping)
	{ 
		fGDIPlusGraphics->ResetClip();
		return;
	}
#endif
	//convert path to path outline
	const Gdiplus::GraphicsPath *pathSource = inPath;
	Gdiplus::GraphicsPath *pathOutline = pathSource->Clone();
	xbox_assert(pathOutline);
	Gdiplus::Status status = pathOutline->Widen( fStrokePen);
	status = pathOutline->Outline();

	//set clipping to new path
	StUseContext_NoRetain	context(this);
	fGDIPlusGraphics->SetClip( pathOutline, inAddToPrevious ? Gdiplus::CombineModeUnion : (inIntersectToPrevious ? Gdiplus::CombineModeIntersect : Gdiplus::CombineModeReplace) );

	delete pathOutline;

	RevealClipping(fGDIPlusGraphics);
}


void VWinGDIPlusGraphicContext::ClipRect(const VRect& inHwndBounds, Boolean inAddToPrevious, Boolean inIntersectToPrevious)
{
#if VERSIONDEBUG
	if (sDebugNoClipping)
	{ 
		fGDIPlusGraphics->ResetClip();
		return;
	}
#endif

	StUseContext_NoRetain	context(this);
	Gdiplus::RectF clipbounds(inHwndBounds.GetX(), inHwndBounds.GetY(), inHwndBounds.GetWidth(), inHwndBounds.GetHeight());
	Gdiplus::Region cliprgn(clipbounds);
	fGDIPlusGraphics->SetClip(&cliprgn,inAddToPrevious ? Gdiplus::CombineModeUnion : (inIntersectToPrevious ? Gdiplus::CombineModeIntersect : Gdiplus::CombineModeReplace));
#if 0
	
	if (inAddToPrevious)
	{
		BOOL	result = ::IntersectClipRect(dc, inHwndBounds.GetLeft(), inHwndBounds.GetTop(), inHwndBounds.GetRight(), inHwndBounds.GetBottom());
		if (!result) vThrowNativeError(::GetLastError());
	}
	else
	{
		RECT r = inHwndBounds;
		HRGN	clip = ::CreateRectRgnIndirect(&r);
		if (clip == NULL) vThrowNativeError(::GetLastError());
		
		sLONG	cs = ::SelectClipRgn(dc,clip);
		if (cs == ERROR) vThrowNativeError(::GetLastError());
		
		BOOL	result = ::DeleteObject(clip);
		if (!result) vThrowNativeError(::GetLastError());
	}

	VRegion region(inHwndBounds);
	Gdiplus::Region rgn(region);
	fGDIPlusGraphics->SetClip(&rgn, inAddToPrevious? CombineModeUnion:CombineModeReplace);
//	fGDIPlusDirectGraphics->SetClip(&rgn, inAddToPrevious? CombineModeUnion:CombineModeReplace/*:CombineModeIntersect*/);
#endif
	RevealClipping(fGDIPlusGraphics);
}


void VWinGDIPlusGraphicContext::ClipRegion(const VRegion& inHwndRegion, Boolean inAddToPrevious, Boolean inIntersectToPrevious)
{
#if VERSIONDEBUG
	if (sDebugNoClipping)
	{
		fGDIPlusGraphics->ResetClip();
		return;
	}
#endif

	StUseContext_NoRetain	context(this);

#if 0	// sc 23/10/2006, use GDI+ API
	// In case of offscreen drawing, port origin may have been moved
	// As SelectClip isn't aware of port origin, we offset the region manually
	VRegion	newRgn(inHwndRegion);

	POINT	offset;
	HDC dc=_GetHDC();
	::GetViewportOrgEx(dc, &offset);
	_ReleaseHDC(dc);
	newRgn.SetPosBy(offset.x, offset.y);

	if (inAddToPrevious)
	{
		BOOL	result = ::ExtSelectClipRgn(fContext, newRgn, RGN_AND);
		if (!result) vThrowNativeError(::GetLastError());
	}
	else
	{
		BOOL	result = ::SelectClipRgn(fContext, newRgn);
		if (!result) vThrowNativeError(::GetLastError());
	}
#endif
	RECT r;
	Gdiplus::RectF re;
	Gdiplus::Region *rgn;
	HRGN hrgn=inHwndRegion;
	
	if(GetRgnBox(hrgn,&r)==SIMPLEREGION)
	{
		if(r.right-r.left == kMAX_GDI_RGN_COORD && r.bottom-r.top == kMAX_GDI_RGN_COORD )
		{
			rgn=new Gdiplus::Region(hrgn);
			rgn->MakeInfinite();
		}
		else
			rgn=new Gdiplus::Region(hrgn);
	}
	else
		rgn=new Gdiplus::Region(hrgn);
	
	rgn->GetBounds(&re,fGDIPlusGraphics);
	
	fGDIPlusGraphics->SetClip(rgn, inAddToPrevious ? Gdiplus::CombineModeUnion : (inIntersectToPrevious ? Gdiplus::CombineModeIntersect : Gdiplus::CombineModeReplace));
	
	delete rgn;
	RevealClipping(fGDIPlusGraphics);
}


void VWinGDIPlusGraphicContext::GetClip(VRegion& outHwndRgn) const
{
	StUseContext_NoRetain	context(this);
	
	if(fGDIPlusGraphics->IsClipEmpty())
	{
		//outHwndRgn.FromRect(VRect(-kMAX_sLONG, -kMAX_sLONG, kMAX_sLONG, kMAX_sLONG));
		outHwndRgn.FromRect(VRect(0,0,0,0));
	}
	else
	{
		Gdiplus::Region clip;
		
		fGDIPlusGraphics->GetClip(&clip);
		
		if(clip.IsInfinite(fGDIPlusGraphics))
		{ // GetHRGN return NULL when the region is infinite
			outHwndRgn.FromRect(VRect(0, 0, kMAX_GDI_RGN_COORD, kMAX_GDI_RGN_COORD));
		}
		else
			outHwndRgn.FromRegionRef(clip.GetHRGN(fGDIPlusGraphics), true);	
	}

	#if 0
	HRGN	newRgn = ::CreateRectRgn(0, 0, 0, 0);
	sLONG	result = ::GetClipRgn(fContext, newRgn);
	assert(result != -1);
	
	if (result == 0)
	{
		outHwndRgn.FromRect(VRect(-kMAX_sLONG, -kMAX_sLONG, kMAX_sLONG, kMAX_sLONG));
		::DeleteObject(newRgn);
	}
	else
	{
		outHwndRgn.FromRegionRef(newRgn, true);

		// In case of offscreen drawing, port origin may have been moved
		// As SelectClip isn't aware of port origin, we offset the region manually
		POINT	offset;
		::GetViewportOrgEx(fContext, &offset);
		outHwndRgn.SetPosBy(-offset.x, -offset.y);
	}
	#endif
}


void VWinGDIPlusGraphicContext::RevealClipping(ContextRef inContext)
{
#if VERSIONDEBUG
	if (sDebugRevealClipping)
	{
		HRGN	clip = ::CreateRectRgn(0, 0, 0, 0);
		
		RECT	bounds;
		::GetClipRgn(inContext, clip);
		::GetRgnBox(clip, &bounds);
		
		HBRUSH	brush = ::CreateSolidBrush(RGB(210, 210, 0));
		::FrameRgn(inContext, clip, brush, 4, 4);
		
		::DeleteObject(brush);
		::DeleteObject(clip);
		::Sleep(kDEBUG_REVEAL_DELAY);
	}
#endif
}
void VWinGDIPlusGraphicContext::RevealClipping(Gdiplus::Graphics* inContext)
{
#if VERSIONDEBUG
	if (sDebugRevealClipping)
	{
		Gdiplus::Rect bounds;
		Gdiplus::Region gdipclip;
		inContext->GetClip(&gdipclip);
		gdipclip.GetBounds(&bounds,inContext);
		
		SolidBrush blackBrush(Color(255, 0, 0, 100));

		inContext->FillRegion(&blackBrush, &gdipclip);
		inContext->Flush();
		
		//HRGN hdcclip=gdipclip.GetHRGN(inContext);
		
		//HDC dc=inContext->GetHDC();
		
		//HBRUSH	brush = ::CreateSolidBrush(RGB(210, 210, 0));
		//::FrameRgn(dc, hdcclip, brush, 4, 4);
		
		//::DeleteObject(brush);
		//::DeleteObject(hdcclip);
		//inContext->ReleaseHDC(dc);
		::Sleep(kDEBUG_REVEAL_DELAY);
	}
#endif
}

void VWinGDIPlusGraphicContext::RevealUpdate(HWND inWindow)
{
// Please call this BEFORE BeginPaint
#if VERSIONDEBUG
	if (sDebugRevealUpdate)
	{
		HDC	curDC = ::GetDC(inWindow);
		
		RECT	bounds;
		HRGN	clip =::CreateRectRgn(0, 0, 0, 0);
		::GetUpdateRgn(inWindow, clip, false);
		::GetRgnBox(clip, &bounds);
		
		HBRUSH	brush = ::CreateSolidBrush(RGB(210, 210, 0));
		HRGN	saveClip = ::CreateRectRgn(0, 0, 0, 0);
		int restoreClip = ::GetClipRgn(curDC, saveClip);
		::SelectClipRgn(curDC, clip);
		
		::FillRgn(curDC, clip, brush);
		if (restoreClip)
			::SelectClipRgn(curDC, saveClip);
		else
			::SelectClipRgn(curDC, NULL);
		::DeleteObject(brush);
		::DeleteObject(saveClip);
		::DeleteObject(clip);
		
		::ReleaseDC(inWindow, curDC);
		::GdiFlush();
		
		::Sleep(kDEBUG_REVEAL_DELAY);
	}
#endif
}


void VWinGDIPlusGraphicContext::RevealBlitting(ContextRef inContext, const RgnRef inHwndRegion)
{
}


void VWinGDIPlusGraphicContext::RevealInval(ContextRef inContext, const RgnRef inHwndRegion)
{
}


#pragma mark-

void VWinGDIPlusGraphicContext::CopyContentTo(VGraphicContext* inDestContext, const VRect* inSrcBounds, const VRect* inDestBounds, TransferMode inMode, const VRegion* inMask)
{

}
VPictureData* VWinGDIPlusGraphicContext::CopyContentToPictureData()
{
	if (fBitmap)
		return static_cast<VPictureData *>(new VPictureData_GDIPlus( fBitmap, FALSE));

	return NULL;
}

void VWinGDIPlusGraphicContext::FillUniformColor(const VPoint& inContextPos, const VColor& inColor, const VPattern* inPattern)
{
	StUseContext_NoRetain	context(this);
	
	VColor*	pixelColor = GetPixelColor(inContextPos);
	COLORREF	winColor = 0x02000000 | pixelColor->GetRGBColor();
	
	SetFillColor(inColor);
	HDC dc=_GetHDC();
	::ExtFloodFill(dc, inContextPos.GetX(), inContextPos.GetY(), winColor, FLOODFILLSURFACE);
	_ReleaseHDC(dc);
}


void VWinGDIPlusGraphicContext::SetPixelColor(const VPoint& inContextPos, const VColor& inColor)
{
	if (fBitmap)
	{
		fBitmap->SetPixel( inContextPos.GetX(), inContextPos.GetY(), inColor.WIN_ToGdiplusColor());
		return;
	}
	HDC dc=_GetHDC();
	if (!dc)
		return;
	::SetPixel(dc, inContextPos.GetX(), inContextPos.GetY(), inColor.WIN_ToCOLORREF());
	_ReleaseHDC(dc);
}


VColor* VWinGDIPlusGraphicContext::GetPixelColor(const VPoint& inContextPos) const
{
	if (fBitmap)
	{
		Gdiplus::Color color;
		fBitmap->GetPixel( inContextPos.GetX(), inContextPos.GetY(), &color);
		return new VColor( color.GetRed(), color.GetGreen(), color.GetBlue(), color.GetAlpha());
	}
	HDC dc=_GetHDC();
	VColor* result = new VColor;
	if (result != NULL)
		result->WIN_FromCOLORREF( ::GetPixel(dc, inContextPos.GetX(), inContextPos.GetY()));
	_ReleaseHDC(dc);
	return result;
}



/** return current clipping bounding rectangle 
 @remarks
 bounding rectangle is expressed in VGraphicContext normalized coordinate space 
 (ie coordinate space with y axis pointing down - compliant with gdiplus and quickdraw coordinate spaces)
 */  
void VWinGDIPlusGraphicContext::GetClipBoundingBox( VRect& outBounds)
{
	Gdiplus::RectF rectClip;
	Gdiplus::Status status = fGDIPlusGraphics->GetClipBounds( &rectClip);
	outBounds.SetCoords( rectClip.X, rectClip.Y, rectClip.Width, rectClip.Height);
}


// Create a filter or alpha layer graphic context
// with specified bounds and filter
// @remark : retain the specified filter until EndLayer is called
bool VWinGDIPlusGraphicContext::_BeginLayer(const VRect& _inBounds, GReal inAlpha, VGraphicFilterProcess *inFilterProcess, VImageOffScreenRef inBackBuffer, bool inInheritParentClipping, bool inTransparent)
{
	//determine if backbuffer is owned by context (temporary layer) or by caller (re-usable layer)
	bool ownBackBuffer = inBackBuffer == NULL;
	if (inBackBuffer == (VImageOffScreenRef)0xFFFFFFFF)
		inBackBuffer = NULL;
	bool bNewBackground = true;
	Gdiplus::Bitmap *bmpLayer = inBackBuffer ? (Gdiplus::Bitmap *)(*inBackBuffer) : NULL;

	//compute transformed bbox
	VAffineTransform ctm;
	GetTransform( ctm);
	VRect bounds;
	VPoint ctmTranslate;
	VRect boundsClip;
	if (inInheritParentClipping)
	{
		Gdiplus::RectF boundsClipNative;
		fGDIPlusGraphics->GetClipBounds( &boundsClipNative);
		boundsClip.SetCoords( boundsClipNative.X, boundsClipNative.Y, boundsClipNative.Width, boundsClipNative.Height);
		boundsClip = ctm.TransformRect( boundsClip);
		boundsClip.NormalizeToInt();
	}
	if (ownBackBuffer)
	{
		//for temporary layer,
		//we draw directly in target device space

		bounds = _inBounds;
		bounds = ctm.TransformRect( bounds);
		if (inInheritParentClipping)
			//we can clip input bounds with current clipping bounds
			//because for temporary layer we will never draw outside current clipping bounds
			bounds.Intersect( boundsClip);
	}
	else
	{
		//for background drawing, in order to avoid discrepancies
		//we draw in target device space minus user to device transform translation
		//we cannot draw directly in screen user space otherwise we would have discrepancies
		//(because background layer is re-used by caller and layer origin will not match always the same pixel origin in screen space)
		ctmTranslate = ctm.GetTranslation();
		ctm.SetTranslation( 0.0f, 0.0f);
		bounds = ctm.TransformRect( _inBounds);
	}

	if (bounds.GetWidth() == 0.0 || bounds.GetHeight() == 0.0)
	{
		//draw area is empty: push empty layer
		
		fStackLayerBounds.push_back( VRect(0,0,0,0));  
		fStackLayerOwnBackground.push_back( ownBackBuffer);
		if (inBackBuffer)
			inBackBuffer->Retain();
		fStackLayerBackground.push_back( inBackBuffer);
		fStackLayerGC.push_back( NULL);
		fStackLayerAlpha.push_back( 1.0);
		fStackLayerFilter.push_back( NULL);
	}	
	else
	{
		//draw area is not empty:
		//build the layer back buffer and graphic context and stack it

		//inflate a little bit to deal with antialiasing 
		bounds.SetPosBy(-2.0f,-2.0f);
		bounds.SetSizeBy(4.0f,4.0f); 
		bounds.NormalizeToInt();

		//stack layer filter process
		GReal pageScaleParent = fGDIPlusGraphics->GetPageScale();
		if (inFilterProcess)
		{
			inFilterProcess->Retain();
			
			//inflate bounds to take account of filter (for blur filter for instance)
			VPoint offsetFilter;
			offsetFilter.x = ceil(inFilterProcess->GetInflatingOffset()*ctm.GetScaleX());
			offsetFilter.y = ceil(inFilterProcess->GetInflatingOffset()*ctm.GetScaleY());
			bounds.SetPosBy(-offsetFilter.x,-offsetFilter.y);
			bounds.SetSizeBy(offsetFilter.x*2.0f, offsetFilter.y*2.0f);

			GReal blurRadius = inFilterProcess->GetBlurMaxRadius();
			if (blurRadius != 0.0f)
			{
				//if (sLayerFilterInflatingOffset > blurRadius)
				//	blurRadius = VGraphicContext::sLayerFilterInflatingOffset;

				bounds.SetPosBy(-blurRadius,-blurRadius);
				bounds.SetSizeBy(blurRadius*2, blurRadius*2);
			}
		}
		fStackLayerFilter.push_back( inFilterProcess);

		//stack layer transformed bounds
		fStackLayerBounds.push_back( bounds);    

		//adjust bitmap bounds if parent graphic context has page scale not equal to 1
		//(while printing for instance, page scale can be 300/72 if dpi == 300
		// and to ensure bitmap will not be pixelized while printing, bitmap
		// must be scaled to printer device resolution)
		VRect boundsBitmap = bounds;
		if (pageScaleParent != 1.0f)
		{
			boundsBitmap.SetWidth( ceil(bounds.GetWidth() * pageScaleParent));
			boundsBitmap.SetHeight( ceil(bounds.GetHeight() * pageScaleParent));
		}

		//stack layer back buffer
		PixelFormat pfDesired = inTransparent ? PixelFormat32bppARGB : PixelFormat32bppRGB;
		if (bmpLayer == NULL)
			bmpLayer = new Gdiplus::Bitmap(	boundsBitmap.GetWidth(), 
											boundsBitmap.GetHeight(), 
											pfDesired);
		else
		{
			if ((bmpLayer->GetWidth() != boundsBitmap.GetWidth())
				||
				(bmpLayer->GetHeight() != boundsBitmap.GetHeight())
				||
				bmpLayer->GetPixelFormat() != pfDesired)
			{
				//reset back buffer
				inBackBuffer->Clear();
				bmpLayer = new Gdiplus::Bitmap(	boundsBitmap.GetWidth(), 
												boundsBitmap.GetHeight(), 
												pfDesired);
			}
			else
			{
				//use external back buffer
				bNewBackground = false;
			}
		}
		if (inBackBuffer)
		{
			inBackBuffer->Set( bmpLayer);
			inBackBuffer->Retain();
		}
		else
			inBackBuffer = new VImageOffScreen( bmpLayer);
		fStackLayerBackground.push_back( inBackBuffer);
		fStackLayerOwnBackground.push_back( ownBackBuffer);

		//stack current graphic context
		fStackLayerGC.push_back( fGDIPlusGraphics);
		
		//stack layer alpha transparency 
		fStackLayerAlpha.push_back( inAlpha);

		bmpLayer->SetResolution(fGDIPlusGraphics->GetDpiX(),fGDIPlusGraphics->GetDpiY());

		//set current graphic context to the layer graphic context
		Gdiplus::Graphics *gcLayer = new Gdiplus::Graphics( bmpLayer);

		//layer inherit from parent context settings
		Gdiplus::Status status;
		status = gcLayer->SetCompositingMode( CompositingModeSourceOver);
		status = gcLayer->SetCompositingQuality( fGDIPlusGraphics->GetCompositingQuality());
		status = gcLayer->SetSmoothingMode( fGDIPlusGraphics->GetSmoothingMode());
		status = gcLayer->SetInterpolationMode( fGDIPlusGraphics->GetInterpolationMode());
		if (inTransparent)
		{
			//fallback to text default antialias for layer graphic context if text cleartype antialias is used
			//to avoid graphic artifacts
			if (fTextRenderingMode & TRM_WITHOUT_ANTIALIASING)
				status = gcLayer->SetTextRenderingHint( TextRenderingHintSingleBitPerPixel);
			else
				status = gcLayer->SetTextRenderingHint( TextRenderingHintAntiAlias);
		}
		else
			status = gcLayer->SetTextRenderingHint( fGDIPlusGraphics->GetTextRenderingHint());
		status = gcLayer->SetPixelOffsetMode( fGDIPlusGraphics->GetPixelOffsetMode());
		status = gcLayer->SetTextContrast( fGDIPlusGraphics->GetTextContrast());
		status = gcLayer->SetPageUnit( fGDIPlusGraphics->GetPageUnit());
		status = gcLayer->SetPageScale( 1.0f);

		//ensure that there is no scaling between parent and layer context
		xbox_assert( gcLayer->GetDpiX() == fGDIPlusGraphics->GetDpiX());
		xbox_assert( gcLayer->GetDpiY() == fGDIPlusGraphics->GetDpiY());

		fGDIPlusGraphics = gcLayer;

		//inherit clipping from parent if appropriate
		if (inInheritParentClipping && (!ownBackBuffer))
		{
			//get parent clipping in layer local space
			boundsClip.SetPosBy(-boundsBitmap.GetX()-ctmTranslate.GetX(), -boundsBitmap.GetY()-ctmTranslate.GetY());
			boundsClip.SetWidth(ceil(boundsClip.GetWidth()*pageScaleParent));
			boundsClip.SetHeight(ceil(boundsClip.GetHeight()*pageScaleParent));
		}

		//set clipping
		SetTransform(VAffineTransform());
		VRect boundsClipLocal = boundsBitmap;
		boundsClipLocal.SetPosTo(0.0f, 0.0f);
		if (inInheritParentClipping && (!ownBackBuffer))
			boundsClipLocal.Intersect(boundsClip);
		fGDIPlusGraphics->SetClip( GDIPLUS_RECT(boundsClipLocal));

#if DEBUG_SHOW_LAYER
		fGDIPlusGraphics->SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
		Gdiplus::SolidBrush brushTransparent( Gdiplus::Color(128,0,0,255));
		fGDIPlusGraphics->FillRectangle(&brushTransparent,
											rectClip.GetX(),
											rectClip.GetY(),
											rectClip.GetWidth(),
											rectClip.GetHeight());
		fGDIPlusGraphics->SetCompositingMode(Gdiplus::CompositingModeSourceOver);
#endif

		//set layer transform
		ctm.Translate( -boundsBitmap.GetX(), -boundsBitmap.GetY(),
						VAffineTransform::MatrixOrderAppend);
		ctm.Scale( pageScaleParent, pageScaleParent, VAffineTransform::MatrixOrderAppend);
		SetTransform( ctm);
	}
	if (!ownBackBuffer)
		return bNewBackground;
	else
		return 	true;
}


// Create a shadow layer graphic context
// with specified bounds and transparency
void VWinGDIPlusGraphicContext::BeginLayerShadow(const VRect& _inBounds, const VPoint& inShadowOffset, GReal inShadowBlurDeviation, const VColor& inShadowColor, GReal inAlpha)
{
	//compute transformed bbox
	VAffineTransform ctm;
	GetTransform( ctm);
	
	//intersect current clipping rect with input bounds
	Gdiplus::RectF boundsClipNative;
	Gdiplus::Status status = fGDIPlusGraphics->GetClipBounds( &boundsClipNative);
	VRect boundsClip( boundsClipNative.X, boundsClipNative.Y, boundsClipNative.Width, boundsClipNative.Height);
	VRect bounds = _inBounds;
	bounds.Intersect( boundsClip);
	bounds = ctm.TransformRect( bounds);

	if (bounds.GetWidth() == 0.0 || bounds.GetHeight() == 0.0)
	{
		//draw area is empty: push empty layer
		
		fStackLayerBounds.push_back( VRect(0,0,0,0));    
		fStackLayerBackground.push_back( NULL);
		fStackLayerOwnBackground.push_back( true);
		fStackLayerGC.push_back( NULL);
		fStackLayerAlpha.push_back( 1.0);
		fStackLayerFilter.push_back( NULL);
	}	
	else
	{
		//draw area is not empty:
		//build the layer back buffer and graphic context and stack it
		bounds.NormalizeToInt();
		
		//create shadow filter sequence
		VGraphicFilterProcess *filterProcess = new VGraphicFilterProcess();	
		
		//add color matrix filter to set shadow color (if appropriate)
		VValueBag attributes;
		if (inShadowColor != VColor(0,0,0,255))
		{
			VGraphicFilter filterColor(eGraphicFilterType_ColorMatrix);
			attributes.SetReal(	VGraphicFilterBagKeys::in,	(sLONG)eGFIT_SOURCE_ALPHA);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM00,	0);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM01,	0);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM02,	0);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM03,	0);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM04,	inShadowColor.GetRed()*1.0/255.0);
			
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM10,	0);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM11,	0);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM12,	0);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM13,	0);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM14,	inShadowColor.GetGreen()*1.0/255.0);
			
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM20,	0);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM21,	0);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM22,	0);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM23,	0);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM24,	inShadowColor.GetBlue()*1.0/255.0);
			
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM30,	0);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM31,	0);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM32,	0);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM33,	inShadowColor.GetAlpha()*1.0/255.0);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM34,	0);
			filterColor.SetAttributes( attributes);
			filterProcess->Add( filterColor);	
		}
		
		//add blur filter
		attributes.Clear();
		VGraphicFilter filterBlur( eGraphicFilterType_Blur);
		if (inShadowColor == VColor(0,0,0,255))
			attributes.SetReal(	VGraphicFilterBagKeys::in,	(sLONG)eGFIT_SOURCE_ALPHA);
		else
			attributes.SetReal(	VGraphicFilterBagKeys::in,	(sLONG)eGFIT_LAST_RESULT);
		attributes.SetReal(	VGraphicFilterBagKeys::blurDeviation, inShadowBlurDeviation);
		filterBlur.SetAttributes( attributes);
		filterProcess->Add( filterBlur);	
			
		//add offset filter
		attributes.Clear();
		VGraphicFilter filterOffset( eGraphicFilterType_Offset);
		attributes.SetReal(	VGraphicFilterBagKeys::in,	(sLONG)eGFIT_LAST_RESULT);
		attributes.SetReal(	VGraphicFilterBagKeys::offsetX,	inShadowOffset.GetX());
		attributes.SetReal(	VGraphicFilterBagKeys::offsetY,	inShadowOffset.GetY());
		filterOffset.SetAttributes( attributes);
		filterProcess->Add( filterOffset);	
		
		//add blend filter
		attributes.Clear();
		VGraphicFilter filterBlend( eGraphicFilterType_Blend);
		attributes.SetReal( VGraphicFilterBagKeys::in,	(sLONG)eGFIT_SOURCE_GRAPHIC);
		attributes.SetReal( VGraphicFilterBagKeys::in2,	(sLONG)eGFIT_LAST_RESULT);
		attributes.SetReal( VGraphicFilterBagKeys::blendType, (sLONG)eGraphicImageBlendType_Over);
		filterBlend.SetAttributes( attributes);
		filterProcess->Add( filterBlend);	

		//add transparency filter if appropriate
		if (inAlpha != 1.0f)
		{
			attributes.Clear();
			VGraphicFilter filterColor(eGraphicFilterType_ColorMatrix);
			attributes.SetReal(	VGraphicFilterBagKeys::in,	(sLONG)eGFIT_LAST_RESULT);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM00,	1);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM01,	0);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM02,	0);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM03,	0);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM04,	0);
			
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM10,	0);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM11,	1);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM12,	0);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM13,	0);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM14,	0);
			
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM20,	0);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM21,	0);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM22,	1);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM23,	0);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM24,	0);
			
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM30,	0);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM31,	0);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM32,	0);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM33,	inAlpha);
			attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM34,	0);
			filterColor.SetAttributes( attributes);
			filterProcess->Add( filterColor);	
		}
			
		//stack layer filter
		fStackLayerFilter.push_back( filterProcess);

		//inflate bounds to take account of filter (for blur filter for instance)
		GReal pageScaleParent = fGDIPlusGraphics->GetPageScale();
		VPoint offsetFilter;
		offsetFilter.x = ceil(filterProcess->GetInflatingOffset()*ctm.GetScaleX());
		offsetFilter.y = ceil(filterProcess->GetInflatingOffset()*ctm.GetScaleY());
		bounds.SetPosBy(-offsetFilter.x,-offsetFilter.y);
		bounds.SetSizeBy(offsetFilter.x*2.0f, offsetFilter.y*2.0f);

		GReal blurRadius = filterProcess->GetBlurMaxRadius();
		if (blurRadius != 0.0f)
		{
			//if (sLayerFilterInflatingOffset > blurRadius)
			//	blurRadius = VGraphicContext::sLayerFilterInflatingOffset;

			bounds.SetPosBy(-blurRadius,-blurRadius);
			bounds.SetSizeBy(blurRadius*2, blurRadius*2);
		}

		//stack layer transformed bounds
		fStackLayerBounds.push_back( bounds);   

		//adjust bitmap bounds if parent graphic context has page scale not equal to 1
		//(while printing for instance, page scale can be 300/72 if dpi == 300
		// and to ensure bitmap will not be pixelized while printing, bitmap
		// must be scaled to printer device resolution)
		VRect boundsBitmap = bounds;
		if (pageScaleParent != 1.0f)
		{
			boundsBitmap.SetWidth( ceil(bounds.GetWidth() * pageScaleParent));
			boundsBitmap.SetHeight( ceil(bounds.GetHeight() * pageScaleParent));
		}
		
		//stack layer back buffer
		Gdiplus::Bitmap *bmpLayer = new Gdiplus::Bitmap(	boundsBitmap.GetWidth(), 
															boundsBitmap.GetHeight(), 
															(PixelFormat)PixelFormat32bppARGB);
		fStackLayerBackground.push_back( new VImageOffScreen(bmpLayer));
		fStackLayerOwnBackground.push_back( true);
		
		//stack current graphic context
		fStackLayerGC.push_back( fGDIPlusGraphics);
		
		//stack layer alpha transparency 
		fStackLayerAlpha.push_back( 1.0);
		
		bmpLayer->SetResolution(fGDIPlusGraphics->GetDpiX(),fGDIPlusGraphics->GetDpiY());
		
		//set current graphic context to the layer graphic context
		Gdiplus::Graphics *gcLayer = new Gdiplus::Graphics( bmpLayer);
		
		//layer inherit from parent context settings
		status = gcLayer->SetCompositingMode( CompositingModeSourceOver);
		status = gcLayer->SetCompositingQuality( fGDIPlusGraphics->GetCompositingQuality());
		status = gcLayer->SetSmoothingMode( fGDIPlusGraphics->GetSmoothingMode());
		status = gcLayer->SetInterpolationMode( fGDIPlusGraphics->GetInterpolationMode());
		//fallback to text default antialias for layer graphic context if text cleartype antialias is used
		//to avoid graphic artifacts
		if (fTextRenderingMode & TRM_WITHOUT_ANTIALIASING)
			status = gcLayer->SetTextRenderingHint( TextRenderingHintSingleBitPerPixel);
		else
			status = gcLayer->SetTextRenderingHint( TextRenderingHintAntiAlias);
		status = gcLayer->SetPixelOffsetMode( fGDIPlusGraphics->GetPixelOffsetMode());
		status = gcLayer->SetTextContrast( fGDIPlusGraphics->GetTextContrast());
		status = gcLayer->SetPageUnit( fGDIPlusGraphics->GetPageUnit());
		status = gcLayer->SetPageScale( 1.0f);
		
		//ensure that there is no scaling between parent and layer context
		xbox_assert( gcLayer->GetDpiX() == fGDIPlusGraphics->GetDpiX());
		xbox_assert( gcLayer->GetDpiY() == fGDIPlusGraphics->GetDpiY());
		
		//reset clip to layer bounds
		fGDIPlusGraphics = gcLayer;
		VRect rectClip = boundsBitmap;
		rectClip.SetPosBy( -boundsBitmap.GetX(), -boundsBitmap.GetY());
		fGDIPlusGraphics->SetClip( GDIPLUS_RECT(rectClip));
		
		//set layer transform
		ctm.Translate( -boundsBitmap.GetX(), -boundsBitmap.GetY(),
						VAffineTransform::MatrixOrderAppend);
		ctm.Scale( pageScaleParent, pageScaleParent, VAffineTransform::MatrixOrderAppend);
		SetTransform( ctm);
	}
}




// Draw the layer graphic context in the container graphic context 
// (which can be either the main graphic context or another layer graphic context)
VImageOffScreenRef VWinGDIPlusGraphicContext::_EndLayer( ) 
{
	if (fStackLayerGC.empty())
		return NULL;

	//pop layer stack... 

	//...pop layer destination bounds
	VRect bounds = fStackLayerBounds.back();
	fStackLayerBounds.pop_back();
	
	//...pop layer back buffer
	Gdiplus::Bitmap *bmpLayer = NULL;
	VImageOffScreenRef offscreen = fStackLayerBackground.back();
	if (offscreen)
		bmpLayer = (Gdiplus::Bitmap *)(*offscreen);
	fStackLayerBackground.pop_back();
	bool ownBackBuffer = fStackLayerOwnBackground.back();
	fStackLayerOwnBackground.pop_back();

	//...pop parent graphic context
	Gdiplus::Graphics *gcLayerParent = (Gdiplus::Graphics *)fStackLayerGC.back();
	fStackLayerGC.pop_back();

	//...pop layer alpha transparency
	GReal alpha = fStackLayerAlpha.back();
	fStackLayerAlpha.pop_back();

	//..pop layer filter
	VGraphicFilterProcess *filter = fStackLayerFilter.back();
	fStackLayerFilter.pop_back();

	if (gcLayerParent == NULL)
	{
		if (ownBackBuffer && offscreen)
		{
			offscreen->Release();
			return NULL;
		}
		return offscreen;
	}

	//destroy current layer graphic context 
	//in order to unlock the current layer back buffer
	delete fGDIPlusGraphics;

	//restore parent graphic context
	fGDIPlusGraphics = gcLayerParent;

	//reset ctm...
	//(because layer coordinate space rotation+scaling is equal to the parent graphic context coordinate space rotation+scaling)
	VAffineTransform ctm;
	GetTransform(ctm);
	bool bNeedInterpolation = false;
	if (ownBackBuffer)
		//...to identity for temporary layer
		//(layer coordinate space = target device coordinate space)
		SetTransform( VAffineTransform());
	else
	{
		//...to identity + parent graphic context translation for background layer
		//(layer coordinate space = target device coordinate space minus user to target device transform translation)
		VAffineTransform xform;
		VPoint ctmTranslation = ctm.GetTranslation();
		bNeedInterpolation = ctmTranslation.x != floor(ctmTranslation.x) || ctmTranslation.y != floor(ctmTranslation.y);
		xform.SetTranslation( ctmTranslation.x, ctmTranslation.y);
		SetTransform( xform);
	}

	//adjust bitmap bounds if parent graphic context has page scale not equal to 1
	//(while printing for instance, page scale can be 300/72 if dpi == 300
	// and to ensure bitmap will not be pixelized while printing, bitmap
	// must be scaled to printer device resolution)
	VRect boundsBitmap;
	GReal pageScaleParent = (GReal)fGDIPlusGraphics->GetPageScale();
	boundsBitmap.SetCoords(0,0,bounds.GetWidth() * pageScaleParent, bounds.GetHeight() * pageScaleParent);

	//set clipping
	//Gdiplus::Region clipRegion;
	//fGDIPlusGraphics->GetClip( &clipRegion);

	//VRect boundsClip = bounds;
	//fGDIPlusGraphics->SetClip( GDIPLUS_RECT(boundsClip), CombineModeIntersect);

	//here we can speed up layer drawing by disabling high quality compositing  
	//(we do not need high quality compositing for a simple bitmap alpha-blending 
	// without scaling and if page scale is equal to 1)
	Gdiplus::CompositingQuality cqBackup = fGDIPlusGraphics->GetCompositingQuality();
	Gdiplus::InterpolationMode imBackup;
	if (!bNeedInterpolation)
		imBackup = fGDIPlusGraphics->GetInterpolationMode();
	Gdiplus::SmoothingMode smBackup = fGDIPlusGraphics->GetSmoothingMode();
	if (fGDIPlusGraphics->GetPageScale() == 1.0f)
	{
		fGDIPlusGraphics->SetCompositingQuality( Gdiplus::CompositingQualityHighSpeed);
		if (!bNeedInterpolation) //if layer origin is aligned on target device pixel bounds
								 //we can disable interpolation
								 //otherwise we should enable it to avoid artifacts
			fGDIPlusGraphics->SetInterpolationMode( Gdiplus::InterpolationModeNearestNeighbor);
		fGDIPlusGraphics->SetSmoothingMode( Gdiplus::SmoothingModeNone);
	}

	if (filter != NULL)
	{
		//draw layer using filter
		VGraphicImage *image = new VGraphicImage( bmpLayer);
		filter->Apply( this, image, bounds, boundsBitmap, NULL, eGraphicImageBlendType_Over, ctm.GetScaling()*pageScaleParent);
		image->Release();
		filter->Release();
	}	
	else if (alpha != 1.0)
	{
		//transparent layer drawing:
		//set a color matrix in order to transform alpha
		
		ColorMatrix matColor;
		matColor.m[0][0] = 1.0;
		matColor.m[0][1] = 0.0;
		matColor.m[0][2] = 0.0;
		matColor.m[0][3] = 0.0;
		matColor.m[0][4] = 0.0;

		matColor.m[1][0] = 0.0;
		matColor.m[1][1] = 1.0;
		matColor.m[1][2] = 0.0;
		matColor.m[1][3] = 0.0;
		matColor.m[1][4] = 0.0;

		matColor.m[2][0] = 0.0;
		matColor.m[2][1] = 0.0;
		matColor.m[2][2] = 1.0;
		matColor.m[2][3] = 0.0;
		matColor.m[2][4] = 0.0;

		matColor.m[3][0] = 0.0;
		matColor.m[3][1] = 0.0;
		matColor.m[3][2] = 0.0;
		matColor.m[3][3] = alpha;
		matColor.m[3][4] = 0.0;

		matColor.m[4][0] = 0.0;
		matColor.m[4][1] = 0.0;
		matColor.m[4][2] = 0.0;
		matColor.m[4][3] = 0.0;
		matColor.m[4][4] = 1.0;

		// set a ImageAttributes object with the previous color matrix
		ImageAttributes imAttr;
		imAttr.SetColorMatrix( &matColor );

		// draw the layer back buffer image
		Gdiplus::RectF rectDst( bounds.GetLeft(), bounds.GetTop(), bounds.GetWidth(), bounds.GetHeight());
		Status status = fGDIPlusGraphics->DrawImage(	
								bmpLayer, 
								rectDst,
								0.0f, 0.0f,
								boundsBitmap.GetWidth(), boundsBitmap.GetHeight(),
								UnitPixel,
								&imAttr);
	}	
	else
	{
		//simple layer drawing

		Gdiplus::RectF rectDst( bounds.GetLeft(), bounds.GetTop(), bounds.GetWidth(), bounds.GetHeight());
		Status status = fGDIPlusGraphics->DrawImage(	
								bmpLayer, 
								rectDst,
								0.0f, 0.0f,
								boundsBitmap.GetWidth(), boundsBitmap.GetHeight(),
								UnitPixel);
	}

	//restore compositing quality
	if (fGDIPlusGraphics->GetPageScale() == 1.0f)
	{
		fGDIPlusGraphics->SetCompositingQuality( cqBackup);
		if (!bNeedInterpolation)
			fGDIPlusGraphics->SetInterpolationMode( imBackup);
		fGDIPlusGraphics->SetSmoothingMode( smBackup);
	}

	//restore previous clipping
	//fGDIPlusGraphics->SetClip( &clipRegion);

	//restore ctm
	SetTransform(ctm);

	if (ownBackBuffer && offscreen)
		//destroy layer back buffer 
		offscreen->Release();
	else
		return offscreen;

	return NULL;
}


/** return true if offscreen layer needs to be cleared or resized on next call to DrawLayerOffScreen or BeginLayerOffScreen/EndLayerOffScreen
@remarks
	return true if transformed input bounds size is not equal to the background layer size
    so only transformed bounds translation is allowed.
	return true the offscreen layer is not compatible with the current gc
 */
bool VWinGDIPlusGraphicContext::ShouldClearLayerOffScreen(const VRect& inBounds, VImageOffScreenRef inBackBuffer)
{
	if (!inBackBuffer)
		return true;
	Gdiplus::Bitmap *bmpLayer = (Gdiplus::Bitmap *)(*inBackBuffer);
	if (!bmpLayer)
		return true;

	//compute transformed bbox
	VAffineTransform ctm;
	GetTransform( ctm);
	VRect bounds;
	VPoint ctmTranslate;

	//for background drawing, in order to avoid discrepancies
	//we draw in target device space minus user to device transform translation
	//we cannot draw directly in screen user space otherwise we would have discrepancies
	//(because background layer is re-used by caller and layer origin will not match always the same pixel origin in screen space)
	VPoint ctmTranslation = ctm.GetTranslation();
	VAffineTransform xform = ctm;
	xform.SetTranslation( 0.0f, 0.0f);
	bounds = xform.TransformRect( inBounds);

	//inflate a little bit to deal with antialiasing 
	bounds.SetPosBy(-2.0f,-2.0f);
	bounds.SetSizeBy(4.0f,4.0f); 

	bounds.NormalizeToInt();

	if (bmpLayer->GetWidth() != (UINT)bounds.GetWidth()
		||
		bmpLayer->GetHeight() != (UINT)bounds.GetHeight())
		return true;

	return false;
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
bool VWinGDIPlusGraphicContext::DrawLayerOffScreen(const VRect& inBounds, VImageOffScreenRef inBackBuffer, bool inDrawAlways)
{
	if (!inBackBuffer)
		return false;
	Gdiplus::Bitmap *bmpLayer = (Gdiplus::Bitmap *)(*inBackBuffer);
	if (!bmpLayer)
		return false;

	//compute transformed bbox
	VAffineTransform ctm;
	GetTransform( ctm);
	VRect bounds;
	VPoint ctmTranslate;

	//for background drawing, in order to avoid discrepancies
	//we draw in target device space minus user to device transform translation
	//we cannot draw directly in screen user space otherwise we would have discrepancies
	//(because background layer is re-used by caller and layer origin will not match always the same pixel origin in screen space)
	VPoint ctmTranslation = ctm.GetTranslation();
	VAffineTransform xform = ctm;
	xform.SetTranslation( 0.0f, 0.0f);
	bounds = xform.TransformRect( inBounds);

	//inflate a little bit to deal with antialiasing 
	bounds.SetPosBy(-2.0f,-2.0f);
	bounds.SetSizeBy(4.0f,4.0f); 

	bool bNeedInterpolation = false;

	if (inDrawAlways)
		bNeedInterpolation = bounds.GetX() != floor(bounds.GetX()) || bounds.GetY() != floor(bounds.GetY());
	else
	{
		bounds.NormalizeToInt();

		if (bmpLayer->GetWidth() != (UINT)bounds.GetWidth()
			||
			bmpLayer->GetHeight() != (UINT)bounds.GetHeight())
			return false;
	}

	//reset ctm to identity + parent graphic context translation for background layer
	//(layer coordinate space = target device coordinate space minus user to target device transform translation)	
	if (!bNeedInterpolation)
		bNeedInterpolation = ctmTranslation.x != floor(ctmTranslation.x) || ctmTranslation.y != floor(ctmTranslation.y);
	xform.MakeIdentity();
	xform.SetTranslation( ctmTranslation.x, ctmTranslation.y);
	SetTransform( xform);

	//ensure there is no page scaling 
	xbox_assert(fGDIPlusGraphics->GetPageScale() == 1.0f);

	//set clipping
	//Gdiplus::Region clipRegion;
	//fGDIPlusGraphics->GetClip( &clipRegion);

	//VRect boundsClip = bounds;
	//fGDIPlusGraphics->SetClip( GDIPLUS_RECT(boundsClip), CombineModeIntersect);

	//here we can boost layer drawing by disabling high quality compositing  
	//(we do not need high quality compositing for a simple bitmap alpha-blending 
	// without scaling and if page scale is equal to 1)
	Gdiplus::CompositingQuality cqBackup = fGDIPlusGraphics->GetCompositingQuality();
	fGDIPlusGraphics->SetCompositingQuality( Gdiplus::CompositingQualityHighSpeed);

	Gdiplus::InterpolationMode imBackup;
	if (!bNeedInterpolation) //if layer origin is aligned on target device pixel bounds
							 //we can disable interpolation
							 //otherwise we should enable it to avoid artifacts
	{
		imBackup = fGDIPlusGraphics->GetInterpolationMode();
		fGDIPlusGraphics->SetInterpolationMode( Gdiplus::InterpolationModeNearestNeighbor);
	}
	Gdiplus::SmoothingMode smBackup = fGDIPlusGraphics->GetSmoothingMode();
	fGDIPlusGraphics->SetSmoothingMode( Gdiplus::SmoothingModeNone);

	//simple layer drawing
	Gdiplus::RectF rectDst( bounds.GetLeft(), bounds.GetTop(), bmpLayer->GetWidth(), bmpLayer->GetHeight());
	Status status = fGDIPlusGraphics->DrawImage(	
							static_cast<Gdiplus::Image *>(bmpLayer), 
							rectDst,
							0.0f, 0.0f,
							bmpLayer->GetWidth(), bmpLayer->GetHeight(),
							UnitPixel);

	//restore compositing quality
	fGDIPlusGraphics->SetCompositingQuality( cqBackup);
	if (!bNeedInterpolation)
		fGDIPlusGraphics->SetInterpolationMode( imBackup);
	fGDIPlusGraphics->SetSmoothingMode( smBackup);

	//restore previous clipping
	//fGDIPlusGraphics->SetClip( &clipRegion);

	//restore ctm
	SetTransform(ctm);

	return true;
}



// clear all layers and restore main graphic context
void VWinGDIPlusGraphicContext::ClearLayers()
{
	try
	{
	while (!fStackLayerGC.empty())
	{
		fStackLayerBounds.pop_back();
		
		Gdiplus::Graphics *gcLayer = (Gdiplus::Graphics *)fStackLayerGC.back();
		if (gcLayer)
		{
			delete fGDIPlusGraphics;
			fGDIPlusGraphics = gcLayer;
		}
		fStackLayerGC.pop_back();

		bool ownBackBuffer = fStackLayerOwnBackground.back();
		fStackLayerOwnBackground.pop_back();

		VImageOffScreenRef offscreen = fStackLayerBackground.back();
		if (offscreen)
			offscreen->Release();
		fStackLayerBackground.pop_back();

		fStackLayerAlpha.pop_back();

		VGraphicFilterProcess *filter = fStackLayerFilter.back();
		if (filter != NULL)
			filter->Release();
		fStackLayerFilter.pop_back();
	}
	}
	catch(...)
	{
		xbox_assert(false);
	}
}


void VWinGDIPlusGraphicContext::GetTransformToLayer(VAffineTransform &outTransform, sLONG inIndexLayer) 
{
	if (fStackLayerGC.size() == 0)
	{
		GetTransform(outTransform);
		return;
	}
	
	GetTransform(outTransform);
	std::vector<VRect>::const_reverse_iterator itBounds = fStackLayerBounds.rbegin();
	std::vector<bool>::const_reverse_iterator itOwnBackground = fStackLayerOwnBackground.rbegin();
	std::vector<VGCNativeRef>::reverse_iterator itGC = fStackLayerGC.rbegin();
	sLONG index = fStackLayerBounds.size()-1;
	for (;itBounds != fStackLayerBounds.rend(); itBounds++, itOwnBackground++, itGC++, index--)
	{
		if (inIndexLayer == index)
			break;

		VRect bounds = *itBounds;
		outTransform.Translate( bounds.GetX(), bounds.GetY(),
							   VAffineTransform::MatrixOrderAppend);
		if ((!(*itOwnBackground)) && (*itGC))
		{
			//for background layer we need to add parent transform translation
			Gdiplus::Graphics *oldGC = fGDIPlusGraphics;
			fGDIPlusGraphics = (Gdiplus::Graphics *)(*itGC);
			VAffineTransform ctm;
			GetTransform( ctm);
			outTransform.Translate( ctm.GetTranslateX(), ctm.GetTranslateY(),
								    VAffineTransform::MatrixOrderAppend);
			fGDIPlusGraphics = oldGC;
		}
	}
}

/** clear current graphic context area
@remarks
	GDIPlus impl: inBounds can be NULL if and only if graphic context is a bitmap graphic context
*/
void VWinGDIPlusGraphicContext::Clear( const VColor& inColor, const VRect *inBounds, bool inBlendCopy)
{
	if (inBlendCopy)
		fGDIPlusGraphics->SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
	if (inBounds == NULL)
		fGDIPlusGraphics->Clear( inColor.WIN_ToGdiplusColor());
	else
	{
		Gdiplus::SolidBrush brush( Gdiplus::Color(inColor.GetAlpha(), inColor.GetRed(), inColor.GetGreen(), inColor.GetBlue()));
		fGDIPlusGraphics->FillRectangle(	&brush,
											inBounds->GetX(),
											inBounds->GetY(),
											inBounds->GetWidth(),
											inBounds->GetHeight());
	}
	if (inBlendCopy)
		fGDIPlusGraphics->SetCompositingMode(Gdiplus::CompositingModeSourceOver);
}

void	VWinGDIPlusGraphicContext::QDFrameRect (const VRect& inHwndRect)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));

	VRect r=inHwndRect;
	REAL penSize = fStrokePen->GetWidth();
	if(penSize>1 || fPrinterScale)
	{
		r.SetPosBy((penSize/2.0),(penSize/2.0));
		r.SetSizeBy(-penSize,-penSize);
	}
	else
	{
		r.SetPosBy(0.5,0.5);
		r.SetSizeBy(-penSize,-penSize);
	}
	FrameRect(r);
}

void	VWinGDIPlusGraphicContext::QDFrameRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));

	VRect r=inHwndRect;
	REAL penSize = fStrokePen->GetWidth();
	if(penSize>1)
	{
		r.SetPosBy((penSize/2.0),(penSize/2.0));
		r.SetSizeBy(-penSize,-penSize);
	}
	else
	{
		r.SetPosBy(0.5,0.5);
		r.SetSizeBy(-penSize,-penSize);
	}
	FrameRoundRect(r,inOvalWidth,inOvalHeight);
}
void VWinGDIPlusGraphicContext::QDFrameOval (const VRect& inHwndRect)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));

	VRect r=inHwndRect;
	REAL penSize = fStrokePen->GetWidth();
	if(penSize>1)
	{
		r.SetPosBy((penSize/2.0),(penSize/2.0));
		r.SetSizeBy(-penSize,-penSize);
	}
	else
	{
		r.SetPosBy(0.5,0.5);
		r.SetSizeBy(-penSize,-penSize);
	}
	FrameOval(r);
}

void	VWinGDIPlusGraphicContext::QDDrawRect (const VRect& inHwndRect)
{
	QDFillRect(inHwndRect);
	QDFrameRect(inHwndRect);
}

void	VWinGDIPlusGraphicContext::QDDrawRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight)
{
	QDFillRoundRect (inHwndRect,inOvalWidth,inOvalHeight);
	QDFrameRoundRect (inHwndRect,inOvalWidth,inOvalHeight);
}

void	VWinGDIPlusGraphicContext::QDDrawOval (const VRect& inHwndRect)
{
	QDFillOval(inHwndRect);
	QDFrameOval(inHwndRect);
}

void	VWinGDIPlusGraphicContext::QDFillRect (const VRect& inHwndRect)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));

	FillRect(inHwndRect);
}
void	VWinGDIPlusGraphicContext::QDFillRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));

	FillRoundRect (inHwndRect,inOvalWidth,inOvalHeight);
}
void	VWinGDIPlusGraphicContext::QDFillOval (const VRect& inHwndRect)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));

	FillOval(inHwndRect);
}
#if 0

static void _QDAdjustLineCoords(GReal* ioCoords)
{
	// When fPenSize = 1 we use PS_ENDCAP_ROUND which adds a pixel at the end
	// this is because with other ENDCAPS and fPenSize = 1 the first point is not always drawn
	if (fLineWidth > 1)
	{
		// GDI does not include the end point when drawing the line
		// so we adjust the end point's coordinates by adding fPenSize
		// to behave like macintosh
		if (ioCoords[0] == ioCoords[2])
		{
			if (ioCoords[3] >= ioCoords[1])
				ioCoords[3]+=fLineWidth;
			else
				ioCoords[3]-=fLineWidth;
		}
		else if (ioCoords[3] == ioCoords[1])
		{
			if (ioCoords[2] >= ioCoords[0])
				ioCoords[2]+=fLineWidth;
			else
				ioCoords[2]-=fLineWidth;
		}
		else
		{
			// Add end point
			sLONG	hDiff = Abs(ioCoords[2] - ioCoords[0]);
			sLONG	vDiff = Abs(ioCoords[3] - ioCoords[1]);
			sLONG	moreX = (hDiff >= (vDiff / 2)) ? fLineWidth : 0;
			sLONG	moreY = (vDiff >= (hDiff / 2)) ? fLineWidth : 0;
	
			if (ioCoords[2] >= ioCoords[0])
				ioCoords[2] += moreX;
			else
				ioCoords[2] -= moreX;
				
			if (ioCoords[3] >= ioCoords[1])
				ioCoords[3] += moreY;
			else 
				ioCoords[3] -= moreY;
		}
	}
}

#endif

static bool go=false;
void VWinGDIPlusGraphicContext::QDDrawLine(const GReal inHwndStartX,const GReal inHwndStartY, const GReal inHwndEndX,const GReal inHwndEndY)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));

	GReal HwndStartX=inHwndStartX;
	GReal HwndStartY=inHwndStartY; 
	GReal HwndEndX=inHwndEndX;
	GReal HwndEndY=inHwndEndY;
	
	LineCap startCap,endCap; 
    DashCap dashCap;
                     
	startCap=fStrokePen->GetStartCap();
	endCap=fStrokePen->GetEndCap();
	dashCap=fStrokePen->GetDashCap();
	
	
	if(fStrokePen->GetDashPatternCount())
		fStrokePen->SetLineCap(LineCapFlat,LineCapFlat,DashCapFlat);
	else
		fStrokePen->SetLineCap(LineCapFlat,LineCapFlat,DashCapFlat);
	
	REAL penSize = fStrokePen->GetWidth();
	
	
	
	if (HwndStartX == HwndEndX)
	{
		HwndStartX += penSize / 2.0;
		HwndEndX += penSize / 2.0;
		if(penSize==1)
		{
			if (HwndEndY >= HwndStartY)
				HwndStartY++;
			else
				HwndStartY--;
		}
	}
	else if(HwndStartY == HwndEndY)
	{
		HwndStartY += penSize/2.0;
		HwndEndY += penSize/2.0;
		if(penSize==1)
		{
			if (HwndEndX >= HwndStartX)
				HwndStartX++;
			else
				HwndStartX--;
		}
	}
	
	DrawLine(HwndStartX,HwndStartY,HwndEndX,HwndEndY);
	
	fStrokePen->SetLineCap(startCap,endCap,dashCap);
	
}

void VWinGDIPlusGraphicContext::QDDrawLines(const GReal* inCoords, sLONG inCoordCount)
{
	if (inCoordCount<4)
		return;
	GReal lastX = inCoords[0];
	GReal lastY = inCoords[1];
	for(VIndex n=2;n<inCoordCount;n+=2)
	{
		QDDrawLine(lastX,lastY, inCoords[n],inCoords[n+1]);
		lastX = inCoords[n];
		lastY = inCoords[n+1];
	}
}

void VWinGDIPlusGraphicContext::QDMoveBrushTo (const VPoint& inHwndPos)
{
	MoveBrushTo (inHwndPos);
}
void VWinGDIPlusGraphicContext::QDDrawLine (const VPoint& inHwndStart, const VPoint& inHwndEnd)
{
	QDDrawLine (inHwndStart.GetX(),inHwndStart.GetY(),inHwndEnd.GetX(),inHwndEnd.GetY());
}
void VWinGDIPlusGraphicContext::QDDrawLineTo (const VPoint& inHwndEnd)
{
	QDDrawLine(fBrushPos,inHwndEnd);
	fBrushPos=inHwndEnd;
}

#pragma mark-

void VWinGDIPlusGraphicContext::SelectBrushDirect(HGDIOBJ inBrush)
{
}


void VWinGDIPlusGraphicContext::SelectPenDirect(HGDIOBJ inPen)
{
}


#pragma mark-

VWinGDIPlusOffscreenContext::VWinGDIPlusOffscreenContext(HDC inSourceContext) : VWinGDIPlusGraphicContext(inSourceContext)
{	
	_Init();
}


VWinGDIPlusOffscreenContext::VWinGDIPlusOffscreenContext(const VWinGDIPlusGraphicContext& inSourceContext) : VWinGDIPlusGraphicContext(inSourceContext)
{
	_Init();
}


VWinGDIPlusOffscreenContext::VWinGDIPlusOffscreenContext(const VWinGDIPlusOffscreenContext& inOriginal) : VWinGDIPlusGraphicContext(inOriginal)
{
	_Init();
}


VWinGDIPlusOffscreenContext::~VWinGDIPlusOffscreenContext()
{
	_Dispose();
}


void VWinGDIPlusOffscreenContext::_Init()
{
}


void VWinGDIPlusOffscreenContext::_Dispose()
{
}


void VWinGDIPlusOffscreenContext::CopyContentTo( VGraphicContext* inDestContext, const VRect* inSrcBounds, const VRect* inDestBounds, TransferMode inMode, const VRegion* inMask)
{
}


Boolean VWinGDIPlusOffscreenContext::CreatePicture(const VRect& inHwndBounds)
{
	return true;
}


void VWinGDIPlusOffscreenContext::ReleasePicture()
{
}


#pragma mark-

VWinGDIPlusBitmapContext::VWinGDIPlusBitmapContext(HDC inSourceContext) : VWinGDIPlusGraphicContext(inSourceContext)
{	
	_Init();
	fSrcContext=inSourceContext;
	if(!fSrcContext)
	{
		fSrcContext=::GetDC(NULL);
		fReleaseSrcContext=true;
	}
	fOffscreen = new VWinGDIBitmapContext(fSrcContext);
}

VWinGDIPlusBitmapContext::VWinGDIPlusBitmapContext(const VWinGDIPlusBitmapContext& inOriginal) : VWinGDIPlusGraphicContext(inOriginal)
{
	_Init();
	
	assert(false);
}


VWinGDIPlusBitmapContext::~VWinGDIPlusBitmapContext()
{
	_Dispose();
}

void VWinGDIPlusBitmapContext::_Init()
{
	fOffscreen=NULL;
	fSrcContext=NULL;
	fReleaseSrcContext=false;
}


void VWinGDIPlusBitmapContext::_Dispose()
{
	ReleaseBitmap();
	
	if(fGDIPlusGraphics)
		delete fGDIPlusGraphics;
	
	fOffscreen->Release();
	if(fReleaseSrcContext)
		::ReleaseDC(NULL,fSrcContext);
}


Boolean VWinGDIPlusBitmapContext::CreateBitmap(const VRect& inHwndBounds)
{
	Boolean result=false; 
	ReleaseBitmap();
	
	if(fGDIPlusGraphics)
		delete fGDIPlusGraphics;
	if(fOffscreen)
	{
		result = fOffscreen->CreateBitmap(inHwndBounds);
		if(result)
		{
			fOffscreen->BeginUsingContext();
			fRefContext = fOffscreen->GetParentContext();
		}
	}
	return result;
}


void VWinGDIPlusBitmapContext::ReleaseBitmap()
{
	if(fRefContext)
	{
		fOffscreen->EndUsingContext();
		fOffscreen->ReleaseBitmap();
	}
	fRefContext=NULL;
}

void VWinGDIPlusBitmapContext::CopyContentTo ( VGraphicContext* inDestContext, const VRect* inSrcBounds , const VRect* inDestBounds , TransferMode inMode , const VRegion* inMask )
{
	if(fOffscreen)
		fOffscreen->CopyContentTo (inDestContext, inSrcBounds , inDestBounds ,inMode , inMask );
}
VPictureData* VWinGDIPlusBitmapContext::CopyContentToPictureData()
{
	VPictureData* result=NULL;
	if(fOffscreen)
	{
		result=  fOffscreen->CopyContentToPictureData();
	}
	return result;
}

#if 0
VWinGDIPlusBitmapContext::VWinGDIPlusBitmapContext(HDC inSourceContext) : VWinGDIPlusGraphicContext(inSourceContext)
{	
	_Init();

	//fSrcContext = fContext;
	//fContext = NULL;
}

VWinGDIPlusBitmapContext::VWinGDIPlusBitmapContext(const VWinGDIPlusBitmapContext& inOriginal) : VWinGDIPlusGraphicContext(inOriginal)
{
	_Init();
	
	//fSrcContext = inOriginal.fSrcContext;
	//fContext = NULL;
}


VWinGDIPlusBitmapContext::~VWinGDIPlusBitmapContext()
{
	_Dispose();
}

void VWinGDIPlusBitmapContext::_Init()
{
	fSrcContext = NULL;
	fBitmap = NULL;
	//fSavedBitmap = NULL;
}


void VWinGDIPlusBitmapContext::_Dispose()
{
	ReleaseBitmap();
	if(fGDIPlusGraphics)
		delete fGDIPlusGraphics;
	fGDIPlusGraphics = NULL;
	//::SelectObject(fContext, fSavedBitmap);
	//::DeleteObject(fBitmap);
	//::DeleteDC(fContext);
}


Boolean VWinGDIPlusBitmapContext::CreateBitmap(const VRect& inHwndBounds)
{
	ReleaseBitmap();
	fBitmap=new Gdiplus::Bitmap(inHwndBounds.GetWidth(),inHwndBounds.GetHeight());
	if(!fGDIPlusGraphics)
	{
		fGDIPlusGraphics=new Gdiplus::Graphics(fBitmap);
	}
	else
	{
		fGDIPlusGraphics->FromImage(fBitmap);
	}
	fGDIPlusGraphics->Clear(Gdiplus::Color(0,0,0,0));
	return fGDIPlusGraphics->GetLastStatus()==Gdiplus::Ok;
	#if 0
	if (fContext == NULL)
	{
		fContext = ::CreateCompatibleDC(fSrcContext);
	}
	else
	{
		assert(fSavedBitmap != NULL);
		
		// Release the previous bitmap
		fSavedBitmap = (HBITMAP) ::SelectObject(fContext, fSavedBitmap);
		::DeleteObject(fSavedBitmap);
	}

	if (fContext != NULL)
	{
		// Create a new bitmap for the context
		fBitmap = ::CreateCompatibleBitmap(fSrcContext, inHwndBounds.GetWidth(), inHwndBounds.GetHeight());
		if (fBitmap != NULL)
		{
			fSavedBitmap = (HBITMAP) ::SelectObject(fContext, fBitmap);
			
			// Erase backgrounds
			::Rectangle(fContext, 0, 0, inHwndBounds.GetWidth(), inHwndBounds.GetHeight());
		}
		else
		{
			::DeleteDC(fContext);
			fContext = NULL;
		}
	}

	return (fContext != NULL);
	#endif
}


void VWinGDIPlusBitmapContext::ReleaseBitmap()
{
	if(fBitmap)
	{
		delete fBitmap;
		fBitmap=NULL;
	}
	/*if (fContext != NULL)
	{
		_Dispose();
		
		fContext = NULL;
		fSavedBitmap = NULL;
		fBitmap = NULL;
	}*/
}
#endif


StUseHDC::StUseHDC(Gdiplus::Graphics* inGraphics)
{
	fHDC=NULL;
	fHDCSavedClip=NULL;
	fSavedClip=NULL;
	if(inGraphics)
	{
		// sync clip region
		
		fGDIPlusGraphics = inGraphics;
		
		fSavedClip=new Region();
	
		fGDIPlusGraphics->GetClip(fSavedClip);
		
		HRGN hdcclip=fSavedClip->GetHRGN(fGDIPlusGraphics);
		
		fHDC=fGDIPlusGraphics->GetHDC();
		assert(fHDC!=NULL);
		fHDCSavedClip=::CreateRectRgn(0,0,0,0);
		if(::GetClipRgn(fHDC,fHDCSavedClip)==0)
		{
			DeleteObject(fHDCSavedClip);
			fHDCSavedClip=NULL;
		}	
		
		::SelectClipRgn(fHDC,hdcclip);
		if(hdcclip)
			::DeleteObject(hdcclip);
	}
}
StUseHDC::~StUseHDC()
{
	if(fGDIPlusGraphics)
	{
		::SelectClipRgn(fHDC,fHDCSavedClip);
		if(fHDCSavedClip)
			::DeleteObject(fHDCSavedClip);
			
		fGDIPlusGraphics->ReleaseHDC(fHDC);
			
		fGDIPlusGraphics->SetClip(fSavedClip,CombineModeReplace);
		delete fSavedClip;
			
		fHDCSavedClip=0;
		fSavedClip=0;
	}
}

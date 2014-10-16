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
#include "VRect.h"
#include "VRegion.h"
#include "VIcon.h"
#include "VPolygon.h"
#include "VPattern.h"
#include "VBezier.h"

#define	USE_DOUBLE_BUFFER	0


// Class constants
const uLONG		kDEBUG_REVEAL_DELAY	= 75;


// Class statics
GReal	VWinGDIGraphicContext::sLogPixelsX		= 90;
GReal	VWinGDIGraphicContext::sLogPixelsY		= 90;
VColor*	VWinGDIGraphicContext::sShadow1Color	= NULL;
VColor*	VWinGDIGraphicContext::sShadow2Color	= NULL;
VColor*	VWinGDIGraphicContext::sShadow3Color	= NULL;

class _StSelectObject
{
	public:
	_StSelectObject(HDC inDC,HGDIOBJ inObj)
	{
		fDC = inDC;
		fPrevious=::SelectObject(inDC,inObj);
		#if VERSIONDEBUG
		DWORD kind=::GetObjectType(fPrevious);
		assert(kind!=0);
		#endif
	}
	virtual ~_StSelectObject()
	{
		#if VERSIONDEBUG
		DWORD kind=::GetObjectType(fPrevious);
		DWORD err=GetLastError();
		if(kind==0)
		{
			LOGPEN pen;
			GetObject(fPrevious,sizeof(LOGPEN),(void*)&pen);
			err=GetLastError();
		}
		
		fPrevious=::SelectObject(fDC,fPrevious);
		err=GetLastError();
		assert(fPrevious!=NULL);
		#else
		::SelectObject(fDC,fPrevious);
		#endif
	}
	HDC		fDC;
	HGDIOBJ fPrevious;
};

// Static

typedef std::pair<HDC, sLONG> _HDCMap_Pair ;
typedef std::map<HDC, sLONG> _HDCMap ;

static _HDCMap sDebugHDCMap; 
static bool sDebugCheckHDC=false;

static void debuggdiobj(const char* s,VWinGDIGraphicContext* inGC,HGDIOBJ inobj)
{
	char buff [1024];
	sprintf(buff,"%s GC = %x OBJ = %x\r\n",s,(long)inGC,(long)inobj);
	OutputDebugString(buff);
}

long VWinGDIGraphicContext::DebugCheckHDC(HDC inDC)
{
	if(sDebugHDCMap.find(inDC)!=sDebugHDCMap.end())
		return sDebugHDCMap[inDC];
	else
		return 0;
}
void VWinGDIGraphicContext::DebugEnableCheckHDC(bool inSet)
{
	if(sDebugCheckHDC!=inSet)
		sDebugHDCMap.clear();
	sDebugCheckHDC=inSet;
}

void VWinGDIGraphicContext::DebugRegisterHDC(HDC inDC)
{
	if(sDebugCheckHDC && inDC!=NULL)
		sDebugHDCMap[inDC]++;
}
void VWinGDIGraphicContext::DebugUnRegisterHDC(HDC inDC)
{
	if(sDebugCheckHDC && inDC!=NULL)
	{
		long count = sDebugHDCMap[inDC]--;
		if(sDebugHDCMap[inDC]==0)
			sDebugHDCMap.erase(inDC);
	}
}

Boolean VWinGDIGraphicContext::Init()
{
	HDC	screenDC = ::GetDC(NULL);
	sLogPixelsX = ::GetDeviceCaps(screenDC, LOGPIXELSX);
	sLogPixelsY = ::GetDeviceCaps(screenDC, LOGPIXELSY);
	::ReleaseDC(NULL, screenDC);
	
	if (sShadow1Color == NULL)
	{
		sShadow1Color = new VColor(COL_GRAY50);
		sShadow2Color = new VColor(COL_GRAY30);
		sShadow3Color = new VColor(COL_GRAY10);
	}
	
	return true;
}


void VWinGDIGraphicContext::DeInit()
{
	delete sShadow1Color;
	sShadow1Color = NULL;
	
	delete sShadow2Color;
	sShadow2Color = NULL;
	
	delete sShadow3Color;
	sShadow3Color = NULL;
}


#pragma mark-

VWinGDIGraphicContext::VWinGDIGraphicContext(HDC inContext)
{
	_Init();
	
	fContext = inContext;
	
	_InitPrinterScale();

	//SaveContext();
}


VWinGDIGraphicContext::VWinGDIGraphicContext(HWND inWindow)
{
	_Init();
	fNeedReleaseDC=true;
	fHwnd = inWindow;
	fContext = ::GetDC(inWindow);
	
	//SaveContext();
}


VWinGDIGraphicContext::VWinGDIGraphicContext(const VWinGDIGraphicContext& inOriginal) : VGraphicContext(inOriginal)
{
	_Init();
	
	fContext = inOriginal.fContext;
	fImmPx = inOriginal.fImmPx;
	fImmPy = inOriginal.fImmPy;
	
	//SaveContext();
}


VWinGDIGraphicContext::~VWinGDIGraphicContext()
{
	//RestoreContext();
	
	_Dispose();
}


void VWinGDIGraphicContext::_Init()
{
	fQDContext_NeedsBeginUsing=false;
	fNeedReleaseDC=false;
	fContext = NULL;
	fDpiX = fDpiY = 0.0f;
	fHwnd = NULL;
	fImmPx = -1;
	fImmPy = -1;
	fLineWidth = 1.0;
	fLineStyle = PS_GEOMETRIC | PS_SOLID | PS_JOIN_MITER;
	fTextAngle = 0.0;
	fAlphaBlend = 0;
	fContextRefcon = 0;
	fCurrentPen = NULL;
	fCurrentBrush = NULL;
	fFillBrush = NULL;
	fFillBrushDirty=true;
	fLineBrushDirty=true;
	fSavedTextColor = CLR_INVALID;
	fSavedBackColor = CLR_INVALID;
	fSavedPen = NULL;
	fSavedBrush = NULL;
	fSavedRop = -1;
	fSavedBkMode = -1;
	fSavedTextAlignment = -1;
	fSaveStream = NULL;
	fSavedGraphicsMode = -1;
	fFillPattern = NULL;
//	fLinePattern = NULL;
	fDrawShadow = false;
	fSavedFont=NULL;
	fOffScreen = NULL;
	fOffScreenUseCount = 0;
	fOffScreenParent = NULL;
}

void VWinGDIGraphicContext::_InitPrinterScale()
{
	HDC screendc = ::GetDC(0);
	long reflogx = ::GetDeviceCaps( screendc, LOGPIXELSX );
	long reflogy = ::GetDeviceCaps( screendc, LOGPIXELSY );
		
	long logx = ::GetDeviceCaps( fContext, LOGPIXELSX );
	long logy = ::GetDeviceCaps( fContext, LOGPIXELSY );
		
	::ReleaseDC(NULL ,screendc );
	
	if(logx!=reflogx)
	{
		long nPrnWidth = ::GetDeviceCaps(fContext,HORZRES); // Page size (X)
		long nPrnHeight = ::GetDeviceCaps(fContext,VERTRES); // Page size (Y)
		
		fPrinterScale=true;
		fPrinterScaleX = nPrnWidth / (MulDiv(nPrnWidth, 72,logx) * 1.0);
		//JQ: there is math discrepancy because of division: if logx == logy, scaling should always be equal 
		if (logx == logy)
			fPrinterScaleY = fPrinterScaleX;
		else
			fPrinterScaleY = nPrnHeight / (MulDiv(nPrnHeight, 72,logy) * 1.0);
	}
	else
	{
		fPrinterScale=false;
		fPrinterScaleX = 1.0;
		fPrinterScaleY = 1.0;
	}
}
void VWinGDIGraphicContext::_Dispose()
{
	XBOX::ReleaseRefCountable(&fOffScreen);

	xbox_assert(fClipStack.IsEmpty());

	if (fNeedReleaseDC)
		::ReleaseDC(fHwnd, fContext);
	
	if (fFillPattern != NULL)
		fFillPattern->Release();
	
//	if (fLinePattern != NULL)
//		fLinePattern->Release();
}

GReal VWinGDIGraphicContext::GetDpiX() const
{
	if(fContext)
	{
		if (fDpiX == 0.0f)
			fDpiX = GetDeviceCaps(fContext,LOGPIXELSX);
		return fDpiX;
	}
	else
		return 72.0;
}
GReal VWinGDIGraphicContext::GetDpiY() const
{
	if(fContext)
	{
		if (fDpiY == 0.0f)
			fDpiY = GetDeviceCaps(fContext,LOGPIXELSY);
		return fDpiY;
	}
	else
		return 72.0;
}

void VWinGDIGraphicContext::GetTransform(VAffineTransform &outTransform)
{
	StUseContext_NoRetain	context(this);
	int mode=GetGraphicsMode (fContext);
	if(mode!=GM_ADVANCED)
	{
		SetGraphicsMode (fContext,GM_ADVANCED);
	}
	
	XFORM xform;
	::GetWorldTransform (fContext,&xform);
	
	outTransform.FromNativeTransform(xform);
	
}
void VWinGDIGraphicContext::SetTransform(const VAffineTransform &inTransform)
{
	StUseContext_NoRetain	context(this);
	int mode=GetGraphicsMode (fContext);
	if(mode!=GM_ADVANCED)
	{
		SetGraphicsMode (fContext,GM_ADVANCED);
	}
	XFORM xform;
	inTransform.ToNativeMatrix(xform);
	SetWorldTransform (fContext,&xform);
	
}
void VWinGDIGraphicContext::ConcatTransform(const VAffineTransform &inTransform)
{
	StUseContext_NoRetain	context(this);
	int mode=GetGraphicsMode (fContext);
	if(mode!=GM_ADVANCED)
	{
		SetGraphicsMode (fContext,GM_ADVANCED);
	}
	
	XFORM xform;
	inTransform.ToNativeMatrix(xform);
	ModifyWorldTransform (fContext,&xform,MWT_LEFTMULTIPLY);
}
void VWinGDIGraphicContext::RevertTransform(const VAffineTransform &inTransform)
{
	VAffineTransform inv=inTransform;
	inv.Inverse();
	ConcatTransform(inv);
}
void VWinGDIGraphicContext::ResetTransform()
{
	StUseContext_NoRetain	context(this);
	ModifyWorldTransform (fContext,NULL,MWT_IDENTITY);
}

#pragma mark-

void VWinGDIGraphicContext::SetFillColor(const VColor& inColor, VBrushFlags* ioFlags)
{
	HGDIOBJ	newBrush;
	if(fFillColor!=inColor || fFillBrush==NULL)
	{
		fFillColor = inColor;

		HGDIOBJ	newBrush = ::CreateSolidBrush(inColor.WIN_ToCOLORREF());
		if (newBrush == NULL) 
			vThrowNativeError(::GetLastError());
		if(fFillBrush!=fCurrentBrush)
		{
			HGDIOBJ	temp=fFillBrush;
			fFillBrush=NULL;
			_DeleteObject(temp);
			fFillBrush=newBrush;
		}
		else
			fFillBrush=NULL;
		
		SelectFillBrushDirect(newBrush);
		
		fFillBrush=newBrush;
	}
	else
	{
		SelectFillBrushDirect(fFillBrush);
	}
	
	if (ioFlags != NULL)
	{
		ioFlags->fLineBrushChanged = true;
		ioFlags->fLineSizeChanged = true;
		ioFlags->fTextBrushChanged = true;
	}
}
BOOL VWinGDIGraphicContext::_DeleteObject(HGDIOBJ inObj) const
{
	DWORD kind=::GetObjectType(inObj);
	switch(kind)
	{
		case OBJ_BRUSH:
		{
			assert(inObj!=fFillBrush);
			break;
		}
		case OBJ_EXTPEN:
		case OBJ_PEN:
		{
			assert(GetCurrentObject(fContext,OBJ_PEN)!=inObj);
			break;
		}
	}
	return ::DeleteObject(inObj);
}

void VWinGDIGraphicContext::SetFillPattern(const VPattern* inPattern, VBrushFlags* ioFlags)
{
	// Release previous brushes by setting NULL stock objects
	SelectPenDirect(::GetStockObject(NULL_PEN));
	
	if (fFillPattern != NULL)
		fFillPattern->ReleaseFromContext(this);

	VPattern*	pattern = (VPattern*)inPattern;
	if (pattern != NULL && pattern->GetKind() == VStandardPattern::Pattern_Kind && ((VStandardPattern*)pattern)->GetPatternStyle() == PAT_FILL_PLAIN)
		pattern = NULL;

	IRefCountable::Copy(&fFillPattern, pattern); 
	if (pattern != NULL)
	{
		SelectFillBrushDirect(::GetStockObject(NULL_BRUSH));
		pattern->ApplyToContext(this);
	}
	else
	{
		// FM, pour faire fonctionner FillRect...
	//	SelectBrushDirect(::GetStockObject(WHITE_BRUSH));
	}
	
	if (ioFlags != NULL)
	{
		ioFlags->fLineBrushChanged = true;
		ioFlags->fLineSizeChanged = true;
		ioFlags->fTextBrushChanged = true;
	}
}


void VWinGDIGraphicContext::SetLineColor(const VColor& inColor, VBrushFlags* ioFlags)
{
	
	fLineColor = inColor;
	
	
	
	HGDIOBJ	newPen = _CreatePen();
	
	if (newPen == NULL) vThrowNativeError(::GetLastError());

	//debuggdiobj("ExtCreatePen SetLineColor",this,newPen);

	SelectPenDirect( newPen);
	SelectLineBrushDirect(::GetStockObject(NULL_BRUSH));
	
	if (ioFlags != NULL)
		ioFlags->fLineBrushChanged = true;
}

HGDIOBJ	VWinGDIGraphicContext::_CreatePen()
{
	LOGBRUSH	brush;
	sLONG lineStyle = fLineStyle;
	sLONG count=fLineDashPattern.size();
	DWORD *style=NULL;
	brush.lbStyle = BS_SOLID;
	brush.lbColor = fLineColor.WIN_ToCOLORREF();
	brush.lbHatch = 0;
	GReal linewidth=fLineWidth;
	if(count>0)
	{
		lineStyle = lineStyle &~ PS_STYLE_MASK;
		lineStyle |= PS_USERSTYLE | PS_ENDCAP_FLAT;
		
		style=new DWORD[count];
		VLineDashPattern::const_iterator it = fLineDashPattern.begin();
		for (sLONG i=0;it != fLineDashPattern.end(); it++, i++)
		{
			style[i]=(DWORD)(*it);
			if(fPrinterScale && fShouldScaleShapesWithPrinterScale)
			{
				style[i]*=fPrinterScaleX;
			}
		}
	}
	else
	{
		if(fPrinterScale && fLineWidth==1)
		{
			//lineStyle = lineStyle &~ PS_ENDCAP_MASK;
			//lineStyle |= PS_ENDCAP_SQUARE;
		}
	}
	if(fPrinterScale && fShouldScaleShapesWithPrinterScale)
	{
		if(!fHairline)
			linewidth*=fPrinterScaleX;
	}
		
	HGDIOBJ	newPen = ::ExtCreatePen(lineStyle, linewidth, &brush, count, style);
	delete [] style;
	return newPen;
}


/** set line dash pattern
@param inDashOffset
	offset into the dash pattern to start the line (user units)
@param inDashPattern
	dash pattern (alternative paint and unpaint lengths in user units)
	for instance {3,2,4} will paint line on 3 user units, unpaint on 2 user units
						 and paint again on 4 user units and so on...
*/

void VWinGDIGraphicContext::SetLineDashPattern(GReal inDashOffset, const VLineDashPattern& inDashPattern, VBrushFlags* ioFlags)
{
	fLineDashPattern=inDashPattern;

	HGDIOBJ	newPen = _CreatePen();
	if (newPen == NULL) 
		vThrowNativeError(::GetLastError());
	
	//debuggdiobj("ExtCreatePen SetLinePattern",this,newPen);
	SelectPenDirect(newPen);
	SelectLineBrushDirect(::GetStockObject(NULL_BRUSH));
	
	if (ioFlags != NULL)
	{
		ioFlags->fFillBrushChanged = true;
		ioFlags->fTextBrushChanged = true;
	}

}

void VWinGDIGraphicContext::SetLinePattern(const VPattern* inPattern, VBrushFlags* ioFlags)
{
	// Line pattern is currently ignored.
	// Should use lbStyle = BS_HATCHED and move some code in _CreateStandardPattern/_ApplyPattern (see SetFillPattern)
	LOGBRUSH	brush;
	brush.lbStyle = BS_SOLID;
	brush.lbColor = fLineColor.WIN_ToCOLORREF();
	brush.lbHatch = 0;
	
	fLineDashPattern.clear();
	
	const VStandardPattern *stdp = dynamic_cast<const VStandardPattern*>(inPattern);
	if(stdp)
	{
		PatternStyle ps = stdp->GetPatternStyle();
		
		fLineStyle = fLineStyle &~ PS_STYLE_MASK;
		
		switch(ps)
		{
			case PAT_LINE_DOT_MEDIUM:
			{
				fLineStyle |= PS_DASH;
				break;
			}
			case PAT_LINE_DOT_MINI:
			case PAT_LINE_DOT_SMALL:
			{
				fLineStyle |= PS_DOT;
				break;
			}
			default:
			{
				fLineStyle |= BS_SOLID;
				break;
			}
		}
	}
	HGDIOBJ	newPen = _CreatePen();
	if (newPen == NULL) vThrowNativeError(::GetLastError());
	
	//debuggdiobj("ExtCreatePen SetLinePattern",this,newPen);
	SelectPenDirect(newPen);
	SelectLineBrushDirect(::GetStockObject(NULL_BRUSH));
	
	if (ioFlags != NULL)
	{
		ioFlags->fFillBrushChanged = true;
		ioFlags->fTextBrushChanged = true;
	}
}


void VWinGDIGraphicContext::SetLineWidth(GReal inWidth, VBrushFlags* ioFlags)
{
	fLineWidth = inWidth;
	
	// If you change this, make sure to synchronize DrawLine
	if (fLineWidth >= 1.0)
		fLineStyle |= PS_ENDCAP_FLAT;
	
		
	HGDIOBJ	newPen = _CreatePen();
	if (newPen == NULL) 
		vThrowNativeError(::GetLastError());
	//debuggdiobj("ExtCreatePen SetLineWidth",this,newPen);
	SelectPenDirect(newPen);
	SelectLineBrushDirect(::GetStockObject(NULL_BRUSH));
	
	if (ioFlags != NULL)
		ioFlags->fFillBrushChanged = true;
}

void VWinGDIGraphicContext::SetLineCap (CapStyle inCapStyle, VBrushFlags* /*ioFlags*/)
{
	switch (inCapStyle)
	{
		case CS_BUTT:
			break;
			
		case CS_ROUND:
			break;
			
		case CS_SQUARE:
			break;
		
		default:
			assert(false);
			break;
	}
}

void VWinGDIGraphicContext::SetLineJoin(JoinStyle inJoinStyle, VBrushFlags* /*ioFlags*/)
{
	switch (inJoinStyle)
	{
		case JS_MITER:
			break;
			
		case JS_ROUND:
			break;
			
		case JS_BEVEL:
			break;
		
		default:
			assert(false);
			break;
	}
}



void VWinGDIGraphicContext::SetLineStyle(CapStyle inCapStyle, JoinStyle inJoinStyle, VBrushFlags* ioFlags)
{
	fLineStyle = PS_GEOMETRIC | PS_SOLID | PS_JOIN_MITER;
	
	SetLineCap( inCapStyle);
	SetLineJoin( inJoinStyle);
	
	// If you change this, make sure to synchronize DrawLine
	if (fLineWidth > 1.0)
		fLineStyle |= PS_ENDCAP_FLAT;
		
	HGDIOBJ	newPen = _CreatePen();
	if (newPen == NULL) 
		vThrowNativeError(::GetLastError());
	
	//debuggdiobj("ExtCreatePen SetLineStyle",this,newPen);
	SelectPenDirect(newPen);
	SelectLineBrushDirect(::GetStockObject(NULL_BRUSH));
		
	if (ioFlags != NULL)
		ioFlags->fFillBrushChanged = true;
}


void VWinGDIGraphicContext::SetTextColor(const VColor& inColor, VBrushFlags* ioFlags)
{
	COLORREF	oldColor = ::SetTextColor(fContext, inColor.WIN_ToCOLORREF());
	if (fSavedTextColor == CLR_INVALID)
		fSavedTextColor = oldColor;
	
	if (ioFlags != NULL)
	{
		ioFlags->fLineBrushChanged = true;
		ioFlags->fFillBrushChanged = true;
	}
}

void VWinGDIGraphicContext::GetTextColor (VColor& outColor) const
{
	outColor.WIN_FromCOLORREF( ::GetTextColor(fContext));
}


void VWinGDIGraphicContext::SetBackColor(const VColor& inColor, VBrushFlags* ioFlags)
{
	COLORREF	oldColor = ::SetBkColor(fContext, inColor.WIN_ToCOLORREF());
	if (fSavedBackColor == CLR_INVALID)
		fSavedBackColor = oldColor;
	
	if (ioFlags != NULL)
	{
		ioFlags->fLineBrushChanged = true;
		ioFlags->fFillBrushChanged = true;
		ioFlags->fTextBrushChanged = true;
	}
}


DrawingMode VWinGDIGraphicContext::SetDrawingMode(DrawingMode inMode, VBrushFlags* ioFlags)
{
	sLONG	mode;
	
	switch (inMode)
	{
		case DM_NORMAL:
			mode = R2_COPYPEN;
			fDrawShadow = false;
			break;
			
		case DM_SHADOW:
			mode = R2_COPYPEN;
			fDrawShadow = true;
			break;
			
		case DM_CLIP:
			mode = R2_MASKPEN;
			fDrawShadow = false;
			break;
			
		case DM_HILITE:
			mode = R2_NOTXORPEN;
			fDrawShadow = false;
			break;
			
		default:
			mode = R2_COPYPEN;
			fDrawShadow = false;
			assert(false);
			break;
	}
	
	sLONG	oldRop = ::SetROP2(fContext, mode);
	if (fSavedRop == -1)
		fSavedRop = oldRop;
		
	sLONG	oldBkMode = ::SetBkMode(fContext, OPAQUE);
	if (fSavedBkMode == -1)
		fSavedBkMode = oldBkMode;
	
	if (ioFlags != NULL)
		ioFlags->fTextModeChanged = true;
	
	return DM_NORMAL;
}


GReal VWinGDIGraphicContext::SetTransparency(GReal inAlpha)
{
	// Not supported
	return 1.0f;
}


void VWinGDIGraphicContext::SetShadowValue(GReal inHOffset, GReal inVOffset, GReal /*inBlurness*/)
{
	// Partially supported (for rects and lines)
	// inBlurness isn't used - see _DrawShadow
	fShadowOffset.SetPosTo(inHOffset, inVOffset);
}


void VWinGDIGraphicContext::EnableAntiAliasing()
{
	// Not supported
}


void VWinGDIGraphicContext::DisableAntiAliasing()
{
	// Not supported
}


bool VWinGDIGraphicContext::IsAntiAliasingAvailable()
{
	return false;
}


void VWinGDIGraphicContext::SetFont( VFont *inFont, GReal inScale)
{
	inFont->ApplyToPort(fContext, inScale);
#if 0
	HFONT	scaledFont = inFont->GetScaledFontRef(inScale);
	HGDIOBJ	previous = ::SelectObject(inPort, scaledFont);

	if (previous != )
		_DeleteObject(previous);
#endif
}

VFont*	VWinGDIGraphicContext::RetainFont()
{
	VFont* result = NULL;
	if(fContext)
	{
		HFONT font = (HFONT)GetCurrentObject(fContext, OBJ_FONT);
		if(font)
		{
			LOGFONTW logfont;
			VFontFace face = KFS_NORMAL;
			GReal	size = 0;
			VString	fontName;


			GetObjectW( font, sizeof(LOGFONTW), &logfont);

			if(logfont.lfItalic)
				face |= KFS_ITALIC;
			if(logfont.lfUnderline)
				face |= KFS_UNDERLINE;
			if(logfont.lfStrikeOut)
				face |= KFS_STRIKEOUT;
			if(logfont.lfWeight == FW_BOLD)
				face |= KFS_BOLD;

			size = Abs(logfont.lfHeight); //as we assume 4D form 72 dpi 1pt = 1px

			fontName.FromUniCString(logfont.lfFaceName);

			result = VFont::RetainFont(fontName,face,size,72);
		}
	}

	return result;
}

void VWinGDIGraphicContext::SetTextPosTo(const VPoint& inHwndPos)
{
	fTextPos = inHwndPos;
}


void VWinGDIGraphicContext::SetTextPosBy(GReal inHoriz, GReal inVert)
{
	fTextPos.SetPosBy(inHoriz, inVert);
}


void VWinGDIGraphicContext::SetTextAngle(GReal inAngle)
{
	fTextAngle = inAngle;
}


DrawingMode VWinGDIGraphicContext::SetTextDrawingMode(DrawingMode inMode, VBrushFlags* ioFlags)
{
	sLONG	mode;
	
	switch (inMode)
	{
		case DM_PLAIN:
			mode = TRANSPARENT;
			break;
			
		default:
			// Other mode are not supported by GDI
			mode = OPAQUE;
			assert(false);
			break;
	}
	
	sLONG	oldRop = ::SetROP2(fContext, R2_COPYPEN);
	if (fSavedRop == -1)
		fSavedRop = oldRop;

	sLONG	oldBkMode = ::SetBkMode(fContext, mode);
	if (fSavedBkMode == -1)
		fSavedBkMode = oldBkMode;
	
	if (ioFlags != NULL)
		ioFlags->fDrawingModeChanged = true;

	return DM_PLAIN;
}


void VWinGDIGraphicContext::SetSpaceKerning(GReal inSpaceExtra)
{
}


void VWinGDIGraphicContext::SetCharKerning(GReal inSpaceExtra)
{
}


uBYTE VWinGDIGraphicContext::SetAlphaBlend(uBYTE inAlphaBlend)
{
	uBYTE	oldBlend = fAlphaBlend;
	fAlphaBlend = inAlphaBlend;
	return oldBlend;
}


void VWinGDIGraphicContext::SetPixelForeColor(const VColor& inColor)
{
	fPixelForeColor = inColor;
}


void VWinGDIGraphicContext::SetPixelBackColor(const VColor& inColor)
{
	fPixelBackColor = inColor;
}



#pragma mark-

GReal VWinGDIGraphicContext::GetTextWidth(const VString& inString, bool inRTL) const 
{
	uLONG	savedAlign = ::SetTextAlign(fContext, TA_BASELINE | (inRTL ? TA_RTLREADING : 0)); //for RTL, we need flag TA_RTLREADING for correct metrics

	SIZE	size;
	if (::GetTextExtentPoint32W(fContext, (LPCWSTR) inString.GetCPointer(), inString.GetLength(), &size) == 0)
		size.cx = 0;

	::SetTextAlign( fContext, savedAlign);

	return size.cx;
}


GReal VWinGDIGraphicContext::GetCharWidth(UniChar inChar) const
{
	VStr4	charString(inChar);
	return GetTextWidth(charString);
}


GReal VWinGDIGraphicContext::GetTextHeight(bool inIncludeExternalLeading) const
{
	TEXTMETRICA	textMetrics;
	::GetTextMetricsA(fContext, &textMetrics);

	//(text height = ascent + descent only because internal leading is included in ascent & external leading is outside text cf MSDN)
	return (textMetrics.tmAscent + textMetrics.tmDescent + (inIncludeExternalLeading ? textMetrics.tmExternalLeading : 0));
}

sLONG VWinGDIGraphicContext::GetNormalizedTextHeight(bool inIncludeExternalLeading) const
{
	TEXTMETRICA	textMetrics;
	::GetTextMetricsA(fContext, &textMetrics);

	//(text height = ascent + descent only because internal leading is included in ascent & external leading is outside text cf MSDN)
	return (textMetrics.tmAscent + textMetrics.tmDescent + (inIncludeExternalLeading ? textMetrics.tmExternalLeading : 0));
}

#pragma mark-

void VWinGDIGraphicContext::NormalizeContext()
{
}


void VWinGDIGraphicContext::SaveContext()
{
	_PortSettings set;
	
	set.fSavedPortSettings=::SaveDC( fContext);
	
	set.fSavedPen=fSavedPen;
	set.fCurrentPen=fCurrentPen;
	
	fSavedPen = NULL;
	fCurrentPen = NULL;
	
	set.fSavedBrush=fSavedBrush;
	set.fCurrentBrush=fCurrentBrush;
	set.fFillBrush=fFillBrush;
	
	fSavedBrush=NULL;
	fCurrentBrush=NULL;
	fFillBrush=NULL;
	
	set.fSavedFont=fSavedFont;
	
	set.fSavedRop=fSavedRop;
	set.fSavedBkMode=fSavedBkMode;
	set.fSavedTextAlignment=fSavedTextAlignment;
	set.fLineColor=fLineColor;
	set.fFillColor=fFillColor;
	set.fLineWidth=fLineWidth;
	set.fLineStyle=fLineStyle;
	set.fFillBrushDirty=fFillBrushDirty;
	set.fLineBrushDirty=fLineBrushDirty;
	set.fHairline=fHairline;
	if(fCurrentBrush)
	{
		assert(fCurrentBrush==::GetCurrentObject(fContext,OBJ_BRUSH));
	}
	if(fCurrentPen)
	{
		assert(fCurrentPen==::GetCurrentObject(fContext,OBJ_PEN));
	}
		
	fSavedPortSettings.push_back( set);
}


void VWinGDIGraphicContext::RestoreContext()
{
	_PortSettings set = fSavedPortSettings.back();
	fSavedPortSettings.pop_back();
	
	::RestoreDC( fContext, set.fSavedPortSettings);
	
	if(set.fFillBrush!=fFillBrush && fFillBrush!=NULL)
	{
		DeleteObject(fFillBrush);
		fFillBrush=NULL;
		set.fFillBrushDirty=true;
	}
	if(set.fCurrentBrush!=fCurrentBrush && fCurrentBrush!=NULL)
	{
		DeleteObject(fCurrentBrush);
		fCurrentBrush=NULL;
		set.fFillBrushDirty=true;
		set.fLineBrushDirty=true;
	}
	if(fCurrentPen!=NULL)
	{
		DeleteObject(fCurrentPen);
		fCurrentPen=NULL;
	}
	
	
	fSavedPen=set.fSavedPen;
	fSavedBrush=set.fSavedBrush;
	fSavedRop=set.fSavedRop;
	fSavedBkMode=set.fSavedBkMode;
	fSavedFont=set.fSavedFont;
	fSavedTextAlignment=set.fSavedTextAlignment;
	fCurrentPen=set.fCurrentPen;
	fCurrentBrush=set.fCurrentBrush;
	fLineColor=set.fLineColor;
	fFillColor=set.fFillColor;
	fLineWidth=set.fLineWidth;
	fLineStyle=set.fLineStyle;
	fFillBrushDirty=set.fFillBrushDirty;
	fLineBrushDirty=set.fLineBrushDirty;
	fFillBrush=set.fFillBrush;
	fHairline=set.fHairline;
	if(fCurrentBrush)
	{
		HGDIOBJ brush=::GetCurrentObject(fContext,OBJ_BRUSH);
		assert(fCurrentBrush==brush);
	}
	else
	{
		if(fSavedBrush!=NULL)
			assert(::GetCurrentObject(fContext,OBJ_BRUSH)!=NULL);
	}
	if(fCurrentPen)
	{
		HGDIOBJ pen=::GetCurrentObject(fContext,OBJ_PEN);
		assert(fCurrentPen==pen);
	}
}


#pragma mark-

void VWinGDIGraphicContext::DrawTextBox(const VString& inString, AlignStyle inHAlign, AlignStyle inVAlign, const VRect& inHwndBounds, TextLayoutMode inOptions)
{
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

	//if (!(inOptions & TLM_DONT_WRAP))
	{
		XBOX::VRect re(inHwndBounds);
		//NDJQ: it is done done by _DrawLegacyTextBox
		//_LPtoDP(re);
		_DrawLegacyTextBox( inString,  inHAlign, inVAlign, re, inOptions);
		return;
	}

	/*
	// Prepare rotation
	HDC		hdc = fContext;

	HFONT	oldFont, rotFont = NULL;

	if (Abs(fTextAngle) > kREAL_PIXEL_PRECISION)
	{
		LOGFONT	fnt;
		HGDIOBJ	fontObj = ::GetCurrentObject(hdc, OBJ_FONT);
		::GetObject(fontObj, sizeof(LOGFONT), (void*)&fnt);

		GReal	degAngle = 360.0 - (fTextAngle * 360.0) / (2.0 * PI);
		fnt.lfEscapement = degAngle * 10.0;
		fnt.lfOrientation = degAngle * 10.0;
		rotFont = CreateFontIndirect(&fnt);
		oldFont = (HFONT)::SelectObject(hdc, rotFont);
		inVAlign = AL_CENTER;
	}

	// Measure and truncate text
	SIZE	wordW;
	sWORD	usedW;
	sLONG	maxLength;
	VString	threeDots(L"...", -1);
	VStr<1>	ellipsis((UniChar) 0x2026); //CHAR_HORIZONTAL_ELLIPSIS
	VString*	suffix = NULL;

	::GetTextExtentExPointW(hdc, (LPCWSTR) inString.GetCPointer(), length, w, (int*) &maxLength, 0, &wordW);
	if(inOptions & TLM_DONT_WRAP)
	{	
		if (maxLength >= length)
		{
			usedW = wordW.cx;
		}
		else
		{
			// The ellipsis does not exists in all fonts, so the following test tries to detect this
			suffix = &ellipsis;
			SIZE ellipsisWidth;
			
			if (!::GetTextExtentPoint32W(hdc, (LPCWSTR) ellipsis.GetCPointer(), ellipsis.GetLength(), &ellipsisWidth) || ellipsisWidth.cx <= 4)
			{
				suffix = &threeDots;
				::GetTextExtentPoint32W(hdc, (LPCWSTR) threeDots.GetCPointer(), threeDots.GetLength(), &ellipsisWidth);
			}
			
			::GetTextExtentExPointW(hdc, (LPCWSTR) inString.GetCPointer(), length, w-ellipsisWidth.cx, (int*)&maxLength, 0, &wordW);
			length = maxLength;
			usedW = wordW.cx + ellipsisWidth.cx;
		}
	}
	else
	{
		usedW = wordW.cx;
	}
	// Align horizontaly and verticaly
	// Don't use wordW.cy to be able to align different strings centered vertically
	GReal   usedH = GetTextHeight();
	GReal	alignmentOffsetH  = 0.0;
	GReal	alignmentOffsetV  = 0.0;

	switch (inHAlign)
	{
		case AL_LEFT:
		case AL_DEFAULT:
			alignmentOffsetH = 1.0;
			break;
			
		case AL_CENTER:
			alignmentOffsetH = (w - usedW) / 2.0;
			break;
			
		case AL_JUST:
			alignmentOffsetH = 1.0;
			break;
			
		case AL_RIGHT:
			alignmentOffsetH = w - usedW;
			break;
			
		default:
			assert(false);
			break;
	}
	
	switch (inVAlign)
	{
		case AL_TOP:
		case AL_JUST:
			alignmentOffsetV = 0.0;
			break;
			
		case AL_DEFAULT:
		case AL_CENTER:
			alignmentOffsetV = (h - usedH) / 2.0;
			break;
			
		case AL_BOTTOM:
			alignmentOffsetV = h - usedH;
			break;
			
		default:
			assert(false);
			break;
	}

	// Adjust position for rotation
	if (Abs(fTextAngle) > kREAL_PIXEL_PRECISION)
	{
		VPoint	rCenter;
		VPoint	center(usedW / 2.0, usedH / 2.0);
		rCenter.SetX(cos(fTextAngle) * center.GetX() - sin(fTextAngle) * center.GetY());
		rCenter.SetY(sin(fTextAngle) * center.GetX() + cos(fTextAngle) * center.GetY());

		x -= rCenter.GetX() + alignmentOffsetV; //- (usedH / 2.0)
 		y -= rCenter.GetY() + (usedW / 2.0) + alignmentOffsetH;
	}
	else
	{
		x += alignmentOffsetH;
		y += alignmentOffsetV;
	}

	// Draw
	
	if(fPrinterScale)	
	{
		x*=fPrinterScaleX;
		y*=fPrinterScaleY;
	}
	
	sLONG	savedMode = ::SetBkMode(hdc, TRANSPARENT);
	if (suffix == NULL)
	{
		fSavedTextAlignment = ::SetTextAlign(hdc, TA_LEFT | TA_TOP);
		::ExtTextOutW(hdc, x, y, 0, NULL, (LPCWSTR) inString.GetCPointer(), length, NULL);
	}
	else
	{
		fSavedTextAlignment = ::SetTextAlign(hdc, TA_LEFT | TA_TOP | TA_UPDATECP);
		::MoveToEx(hdc, x, y, NULL);
		::ExtTextOutW(hdc, 0, 0, 0, NULL, (LPCWSTR) inString.GetCPointer(), length, NULL);
		::ExtTextOutW(hdc, 0, 0, 0, NULL, (LPCWSTR) suffix->GetCPointer(), suffix->GetLength(), NULL);
	}
	
	// Restore state
	::SetBkMode(hdc, savedMode);
	if (rotFont != NULL)
	{
		oldFont = (HFONT)::SelectObject(hdc, oldFont);
		_DeleteObject(rotFont);
	}
	*/
}

void VWinGDIGraphicContext::GetTextBoxBounds( const VString& inString, VRect& ioHwndBounds, TextLayoutMode inMode)
{
// pp pas tiptop mais la fonction n'existais pas. Ajouter rapidement afin d'activer les context gdi
	GReal	x = ioHwndBounds.GetX();
	GReal	y = ioHwndBounds.GetY();
	GReal	w = ioHwndBounds.GetWidth() != 0.0f ? ioHwndBounds.GetWidth() : 100000.0f;
	GReal	h = ioHwndBounds.GetHeight() != 0.0f ? ioHwndBounds.GetHeight() : 100000.0f ;
	
	sLONG	length = inString.GetLength();
	if (length == 0)
	{
		ioHwndBounds.SetWidth( 0);
		ioHwndBounds.SetHeight( 0);
		return;
	}
	
	StUseContext_NoRetain	context(this);

	//if (!(inMode & TLM_DONT_WRAP))
	{
		_GetLegacyTextBoxSize( inString,  ioHwndBounds, inMode);
		return;
	}

	/*
	// Prepare rotation
	HDC		hdc = fContext;
	HFONT	oldFont, rotFont = NULL;

	if (Abs(fTextAngle) > kREAL_PIXEL_PRECISION)
	{
		LOGFONT	fnt;
		HGDIOBJ	fontObj = ::GetCurrentObject(hdc, OBJ_FONT);
		::GetObject(fontObj, sizeof(LOGFONT), (void*)&fnt);

		GReal	degAngle = 360.0 - (fTextAngle * 360.0) / (2.0 * PI);
		fnt.lfEscapement = degAngle * 10.0;
		fnt.lfOrientation = degAngle * 10.0;
		rotFont = CreateFontIndirect(&fnt);
		oldFont = (HFONT)::SelectObject(hdc, rotFont);
	}

	// Measure and truncate text
	bool truncate=(inMode & TLM_TRUNCATE_MIDDLE_IF_NECESSARY) != 0;
	SIZE	wordW;
	sWORD	usedW,usedH;
	sLONG	maxLength;
	VString	threeDots(L"...", -1);
	VStr<1>	ellipsis((UniChar) 0x2026); //CHAR_HORIZONTAL_ELLIPSIS
	VString*	suffix = NULL;

	::GetTextExtentExPointW(hdc, (LPCWSTR) inString.GetCPointer(), length, w, (int*) &maxLength, 0, &wordW);
	if (maxLength >= length )
	{
		usedW = wordW.cx;
	}
	else
	{
		if(truncate)
		{
			// The ellipsis does not exists in all fonts, so the following test tries to detect this
			suffix = &ellipsis;
			SIZE ellipsisWidth;
			
			if (!::GetTextExtentPoint32W(hdc, (LPCWSTR) ellipsis.GetCPointer(), ellipsis.GetLength(), &ellipsisWidth) || ellipsisWidth.cx <= 4)
			{
				suffix = &threeDots;
				::GetTextExtentPoint32W(hdc, (LPCWSTR) threeDots.GetCPointer(), threeDots.GetLength(), &ellipsisWidth);
			}
			
			::GetTextExtentExPointW(hdc, (LPCWSTR) inString.GetCPointer(), length, w-ellipsisWidth.cx, (int*)&maxLength, 0, &wordW);
			length = maxLength;
			usedW = wordW.cx + ellipsisWidth.cx;
		}
		else
			usedW = wordW.cx;
	}
	usedH = wordW.cy;
	GReal	alignmentOffsetH  = 0.0;
	GReal	alignmentOffsetV  = 0.0;

	// Adjust position for rotation
	if (Abs(fTextAngle) > kREAL_PIXEL_PRECISION)
	{
		VPoint	rCenter;
		VPoint	center(usedW / 2.0, usedH / 2.0);
		rCenter.SetX(cos(fTextAngle) * center.GetX() - sin(fTextAngle) * center.GetY());
		rCenter.SetY(sin(fTextAngle) * center.GetX() + cos(fTextAngle) * center.GetY());

		x -= rCenter.GetX() +alignmentOffsetH; //- (usedH / 2.0)
 		y -= rCenter.GetY() + (usedW / 2.0) + alignmentOffsetV;
	}
	else
	{
		x += alignmentOffsetH;
		y += alignmentOffsetV;
	}
	
	ioHwndBounds.SetX( x);
	ioHwndBounds.SetY( y);
	ioHwndBounds.SetWidth( usedW);
	ioHwndBounds.SetHeight( usedH);
	
#if 0
	// Draw
	sLONG	savedMode = ::SetBkMode(hdc, TRANSPARENT);
	if (suffix == NULL)
	{
		fSavedTextAlignment = ::SetTextAlign(hdc, TA_LEFT | TA_TOP);
		::ExtTextOutW(hdc, x, y, 0, NULL, (LPCWSTR) inString.GetCPointer(), length, NULL);
	}
	else
	{
		fSavedTextAlignment = ::SetTextAlign(hdc, TA_LEFT | TA_TOP | TA_UPDATECP);
		::MoveToEx(hdc, x, y, NULL);
		::ExtTextOutW(hdc, 0, 0, 0, NULL, (LPCWSTR) inString.GetCPointer(), length, NULL);
		::ExtTextOutW(hdc, 0, 0, 0, NULL, (LPCWSTR) suffix->GetCPointer(), suffix->GetLength(), NULL);
	}
	
	// Restore state
	::SetBkMode(hdc, savedMode);
#endif

	if (rotFont != NULL)
	{
		oldFont = (HFONT)::SelectObject(hdc, oldFont);
		_DeleteObject(rotFont);
	}
	*/
}

void VWinGDIGraphicContext::DrawText(const VString& inString, TextLayoutMode inOptions, bool inRTLAnchorRight)
{
	StUseContext_NoRetain	context(this);
	
	if (inRTLAnchorRight && (inOptions & TLM_RIGHT_TO_LEFT))
		fTextPos.SetPosBy( -GetTextWidth( inString, true), 0);			

	uLONG	savedAlign = ::SetTextAlign(fContext, TA_BASELINE | TA_UPDATECP | ((inOptions & TLM_RIGHT_TO_LEFT) ? TA_RTLREADING : 0)); //for RTL, we need flag TA_RTLREADING for correct display
	if (fSavedTextAlignment == -1)
		fSavedTextAlignment = savedAlign;

	sLONG	oldMode = ::SetBkMode(fContext, TRANSPARENT);	// Temp fix until SetDrawingMode is fixed

	POINT	pos;
	::MoveToEx(fContext, fTextPos.GetX(), fTextPos.GetY(), &pos);
	
	BOOL	result = ::TextOutW(fContext, 0, 0, (LPCWSTR)inString.GetCPointer(), inString.GetLength());

	::MoveToEx(fContext, pos.x, pos.y, &pos);

	if (!(inRTLAnchorRight && (inOptions & TLM_RIGHT_TO_LEFT)))
		fTextPos.SetPosTo(pos.x, pos.y);
}

void VWinGDIGraphicContext::DrawText (const VString& inString, const VPoint& inPos, TextLayoutMode inLayoutMode, bool inRTLAnchorRight)
{
	SetTextPosTo( inPos);
	DrawText( inString, inLayoutMode, inRTLAnchorRight);
}


void VWinGDIGraphicContext::GetStyledTextBounds( const VString& inText, VTreeTextStyle *inStyles, VRect& outBounds)
{
	assert(false);
}

#pragma mark-

void VWinGDIGraphicContext::FrameRect(const VRect& inHwndBounds)
{
	StUseContext_NoRetain	context(this);
	
	// wants only the frame
	// msdn: "The rectangle that is drawn excludes the bottom and right edges."
	// "If a PS_NULL pen is used, the dimensions of the rectangle are 1 pixel less in height and 1 pixel less in width."
	_StSelectObject	pen(fContext,::GetStockObject(NULL_BRUSH));
	_UpdateLineBrush();
	
	VRect re(inHwndBounds);
	
	_LPtoDP(re);
	::Rectangle( fContext, re.GetLeft(), re.GetTop(), re.GetRight(), re.GetBottom());
	
	if (fDrawShadow)
		_DrawShadow(fContext, fShadowOffset, inHwndBounds);
}


void VWinGDIGraphicContext::FrameOval(const VRect& inHwndBounds)
{
	StUseContext_NoRetain	context(this);
	
	_StSelectObject	pen(fContext,::GetStockObject(NULL_BRUSH));
	_UpdateLineBrush();
	
	VRect re(inHwndBounds);
	
	_LPtoDP(re);
	
	::Ellipse(fContext, re.GetLeft(), re.GetTop(), re.GetRight(), re.GetBottom());
}


void VWinGDIGraphicContext::FrameRegion(const VRegion& inHwndRegion)
{
	StUseContext_NoRetain	context(this);
	
	_StSelectObject	pen(fContext,::GetStockObject(NULL_BRUSH));
	_UpdateLineBrush();
	
	::FrameRgn(fContext, inHwndRegion, (HBRUSH)fCurrentBrush, fLineWidth, fLineWidth);
	
}


void VWinGDIGraphicContext::FramePolygon(const VPolygon& inHwndPolygon)
{
	StUseContext_NoRetain	context(this);
	
	_StSelectObject	pen(fContext,::GetStockObject(NULL_BRUSH));
	_UpdateLineBrush();
	
	::Polyline(fContext, const_cast<VPolygon&>(inHwndPolygon).GetNativePoints(), inHwndPolygon.GetPointCount());
}


void VWinGDIGraphicContext::FrameBezier(const VGraphicPath& inHwndBezier)
{
	FramePath( inHwndBezier);
}

void VWinGDIGraphicContext::FramePath(const VGraphicPath& inHwndPath)
{
	StUseContext_NoRetain	context(this);

	//derive to GDIPlus context in order to draw arc in GDI context
	{
	VWinGDIPlusGraphicContext gc( fContext);
	gc.BeginUsingContext();
	
	if(fPrinterScale && fShouldScaleShapesWithPrinterScale)
	{
		Gdiplus::Graphics* gdip=(Gdiplus::Graphics*)gc.GetNativeRef();
		gdip->SetPageUnit(Gdiplus::UnitPixel );
		gdip->SetPageScale(fPrinterScaleX);
	}
	
	gc.SetMaxPerfFlag( GetMaxPerfFlag());
	gc.SetLineColor( fLineColor);
	gc.SetLineWidth( fLineWidth);

	if(fLineDashPattern.size())
	{
		gc.SetLineDashPattern(0,fLineDashPattern);
	}

	gc.FramePath( inHwndPath);

	gc.EndUsingContext();
	}
}


void VWinGDIGraphicContext::FillRect(const VRect& inHwndBounds)
{
	StUseContext_NoRetain	context(this);

	_StSelectObject	pen(fContext,::GetStockObject(NULL_PEN));
	
	_UpdateFillBrush();
	// wants only the filling
	// msdn: "The rectangle that is drawn excludes the bottom and right edges."
	// "If a PS_NULL pen is used, the dimensions of the rectangle are 1 pixel less in height and 1 pixel less in width."

	
	VRect re(inHwndBounds);
	
	_LPtoDP(re);
		
	::Rectangle(fContext, re.GetLeft(), re.GetTop(), re.GetRight(), re.GetBottom());
	
	if (fDrawShadow)
		_DrawShadow(fContext, fShadowOffset, inHwndBounds);
}


void VWinGDIGraphicContext::FillOval(const VRect& inHwndBounds)
{
	StUseContext_NoRetain	context(this);
	_StSelectObject	pen(fContext,::GetStockObject(NULL_PEN));
	_UpdateFillBrush();
	
	VRect re(inHwndBounds);
	
	_LPtoDP(re);
	
	::Ellipse(fContext, re.GetLeft(), re.GetTop(), re.GetRight() , re.GetBottom() );
}


void VWinGDIGraphicContext::FillRegion(const VRegion& inHwndRegion)
{
	StUseContext_NoRetain	context(this);
	_StSelectObject	pen(fContext,::GetStockObject(NULL_PEN));	
	_UpdateFillBrush();
	::PaintRgn(fContext, inHwndRegion);
}


void VWinGDIGraphicContext::FillPolygon(const VPolygon& inHwndPolygon)
{
	StUseContext_NoRetain	context(this);
	_StSelectObject	pen(fContext,::GetStockObject(NULL_PEN));
	_UpdateFillBrush();
	::Polygon(fContext, const_cast<VPolygon&>(inHwndPolygon).GetNativePoints(), inHwndPolygon.GetPointCount());
}


void VWinGDIGraphicContext::FillBezier(VGraphicPath& inHwndBezier)
{
	FillPath( inHwndBezier);
}

void VWinGDIGraphicContext::FillPath(VGraphicPath& inHwndPath)
{
	StUseContext_NoRetain	context(this);

	//derive to GDIPlus context in order to draw arc in GDI context
	{
	VWinGDIPlusGraphicContext gc( fContext);
	gc.BeginUsingContext();
	
	if(fPrinterScale && fShouldScaleShapesWithPrinterScale)
	{
		Gdiplus::Graphics* gdip=(Gdiplus::Graphics*)gc.GetNativeRef();
		gdip->SetPageUnit(Gdiplus::UnitPixel );
		gdip->SetPageScale(fPrinterScaleX);
	}
	
	
	gc.SetMaxPerfFlag( GetMaxPerfFlag());
	gc.SetFillColor( fFillColor);

	gc.FillPath( inHwndPath);

	gc.EndUsingContext();
	}
}


void VWinGDIGraphicContext::DrawRect(const VRect& inHwndBounds)
{
	StUseContext_NoRetain	context(this);
	
	// wants only the filling
	// msdn: "The rectangle that is drawn excludes the bottom and right edges."
	// "If a PS_NULL pen is used, the dimensions of the rectangle are 1 pixel less in height and 1 pixel less in width."

	VRect re(inHwndBounds);
	
	_LPtoDP(re);

	::Rectangle(fContext, re.GetLeft(), re.GetTop(), re.GetRight(), re.GetBottom());
	
	if (fDrawShadow)
		_DrawShadow(fContext, fShadowOffset, inHwndBounds);
}


void VWinGDIGraphicContext::DrawOval(const VRect& inHwndBounds)
{
	// GDI uses the same call for filling or framing a rectangle if the brushes are correct
	FillOval(inHwndBounds);
}


void VWinGDIGraphicContext::DrawRegion(const VRegion& inHwndRegion)
{
	FillRegion(inHwndRegion);
	FrameRegion(inHwndRegion);
}


void VWinGDIGraphicContext::DrawPolygon(const VPolygon& inHwndPolygon)
{
	FillPolygon(inHwndPolygon);
	FramePolygon(inHwndPolygon);
}


void VWinGDIGraphicContext::DrawBezier(VGraphicPath& inHwndBezier)
{
	DrawPath( inHwndBezier);
}

void VWinGDIGraphicContext::DrawPath(VGraphicPath& inHwndPath)
{
	StUseContext_NoRetain	context(this);

	//derive to GDIPlus context in order to draw arc in GDI context
	{
	VWinGDIPlusGraphicContext gc( fContext);
	gc.BeginUsingContext();
	
	if(fPrinterScale && fShouldScaleShapesWithPrinterScale)
	{
		Gdiplus::Graphics* gdip=(Gdiplus::Graphics*)gc.GetNativeRef();
		gdip->SetPageUnit(Gdiplus::UnitPixel );
		gdip->SetPageScale(fPrinterScaleX);
	}
	
	gc.SetMaxPerfFlag( GetMaxPerfFlag());
	gc.SetLineColor( fLineColor);
	gc.SetLineWidth( fLineWidth);
	gc.SetFillColor( fFillColor);

	gc.FillPath( inHwndPath);
	gc.FramePath( inHwndPath);

	gc.EndUsingContext();
	}
}


void VWinGDIGraphicContext::EraseRect(const VRect& inHwndBounds)
{
	assert(false);	// Not supported (GDI doesn't support empty context)
}


void VWinGDIGraphicContext::EraseRegion(const VRegion& inHwndRegion)
{
	assert(false);	// Not supported (GDI doesn't support empty context)
}


void VWinGDIGraphicContext::InvertRect(const VRect& inHwndBounds)
{
	StUseContext_NoRetain	context(this);
	
	VRect re(inHwndBounds);
	_LPtoDP(re);
	RECT r = re;
	
	::InvertRect(fContext, &r);
}


void VWinGDIGraphicContext::InvertRegion(const VRegion& inHwndRegion)
{
	StUseContext_NoRetain	context(this);
	
	::InvertRgn(fContext, inHwndRegion);
}


void VWinGDIGraphicContext::DrawLine(const VPoint& inHwndStart, const VPoint& inHwndEnd)
{
	StUseContext_NoRetain	context(this);
	
	_StSelectObject	pen(fContext,::GetStockObject(NULL_BRUSH));
	_UpdateLineBrush();
	
	VPoint start=inHwndStart,end=inHwndEnd;
	_LPtoDP(start);
	_LPtoDP(end);
	
	GReal	coords[4];
	coords[0] = start.GetX();
	coords[1] = start.GetY();
	coords[2] = end.GetX();
	coords[3] = end.GetY();

	_AdjustLineCoords(coords);

#if 1
	BOOL	result = ::MoveToEx(fContext, coords[0], coords[1], 0);
	result = ::LineTo(fContext, coords[2], coords[3]);
#else
	BOOL result = ::BeginPath( fContext);
	result = ::MoveToEx( fContext,coords[0], coords[1],NULL);
	result = ::LineTo( fContext,coords[2], coords[3]);
	result = ::EndPath( fContext);
	result = ::StrokePath( fContext);
#endif
	
	if (fDrawShadow)
		_DrawShadow(fContext, fShadowOffset, inHwndStart, inHwndEnd);
	
	if (!result) vThrowNativeError(::GetLastError());
}

void VWinGDIGraphicContext::DrawLine(const GReal inHwndStartX,const GReal inHwndStartY, const GReal inHwndEndX,const GReal inHwndEndY)
{
	StUseContext_NoRetain	context(this);
	
	_StSelectObject	pen(fContext,::GetStockObject(NULL_BRUSH));
	_UpdateLineBrush();
	
	VPoint start(inHwndStartX,inHwndStartY),end(inHwndEndX,inHwndEndY);
	_LPtoDP(start);
	_LPtoDP(end);
	
	GReal	coords[4];
	coords[0] = start.GetX();
	coords[1] = start.GetY();
	coords[2] = end.GetX();
	coords[3] = end.GetY();

	_AdjustLineCoords(coords);
	
	BOOL	result = ::MoveToEx(fContext, coords[0], coords[1], 0);
	result = ::LineTo(fContext, coords[2], coords[3]);
	
	if (fDrawShadow)
		_DrawShadow(fContext, fShadowOffset, inHwndStartX,inHwndStartY,inHwndEndX,inHwndEndY);
	
	if (!result) vThrowNativeError(::GetLastError());
}
void VWinGDIGraphicContext::DrawLines(const GReal* inCoords, sLONG inCoordCount)
{ 
	StUseContext_NoRetain	context(this);
	
	_StSelectObject	pen(fContext,::GetStockObject(NULL_BRUSH));
	_UpdateLineBrush();
	
	if (inCoordCount < 4 || (inCoordCount & 0x00000001) != 0) return;
	
	::MoveToEx(fContext, inCoords[0], inCoords[1], 0);
	
	for (sLONG index = 2; index < (inCoordCount - 1); index += 2)
		::LineTo(fContext, inCoords[index], inCoords[index + 1]);
}


void VWinGDIGraphicContext::DrawLineTo(const VPoint& inHwndEnd)
{
	DrawLine(fBrushPos, inHwndEnd);
	fBrushPos = inHwndEnd;
}


void VWinGDIGraphicContext::DrawLineBy(GReal inWidth, GReal inHeight)
{
	VPoint	end(fBrushPos);
	end.SetPosBy(inWidth, inHeight);
	
	DrawLine(fBrushPos, end);
	fBrushPos = end;
}


void VWinGDIGraphicContext::MoveBrushTo(const VPoint& inHwndPos)
{	
	fBrushPos = inHwndPos;
}


void VWinGDIGraphicContext::MoveBrushBy(GReal inWidth, GReal inHeight)
{	
	fBrushPos.SetPosBy(inWidth, inHeight);
}


void VWinGDIGraphicContext::DrawIcon(const VIcon& inIcon, const VRect& inHwndBounds, PaintStatus /*inState*/, GReal /*inScale*/)
{
	StUseContext_NoRetain	context(this);
	
	if (inIcon.IsValid())
		BOOL	succeed = ::DrawIconEx(fContext, inHwndBounds.GetX(), inHwndBounds.GetY(), inIcon, inHwndBounds.GetWidth(), inHwndBounds.GetHeight(), 0, NULL, DI_NORMAL);
}
void VWinGDIGraphicContext::DrawPicture (const VPicture& inPicture,const VRect& inHwndBounds,VPictureDrawSettings *inSet)
{	
	StUseContext_NoRetain	context(this);
	inPicture.Draw(fContext,inHwndBounds,inSet);
}

void VWinGDIGraphicContext::DrawPictureData (const VPictureData *inPictureData, const VRect& inHwndBounds,VPictureDrawSettings *inSet)
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
	else if (inPictureData->IsKind( sCodec_pict))
	{
		//@remark: if the mac picture contains text or font opcode, ALTURA may change the current font DC and delete a HFONT referenced in a VFONT
		HFONT saveFont=(HFONT)SelectObject(fContext,GetStockObject(SYSTEM_FONT));
		inPictureData->DrawInPortRef( fContext, inHwndBounds, inSet?inSet:&drawSettings);
		SelectObject(fContext,saveFont);
	}
	else
		inPictureData->DrawInPortRef( fContext, inHwndBounds, inSet?inSet:&drawSettings);

	
	//restore current transformation matrix
	SetTransform(ctm);
}

void VWinGDIGraphicContext::DrawFileBitmap(const VFileBitmap *inFileBitmap, const VRect& inHwndBounds)
{

}

void VWinGDIGraphicContext::DrawPictureFile(const VFile& inFile, const VRect& inHwndBounds)
{
	assert(false);
}


#pragma mark-

void VWinGDIGraphicContext::SaveClip()
{
	StUseContext_NoRetain	context(this);
	
	HRGN	rgn = ::CreateRectRgn(0,0,0,0);
	sLONG	result = ::GetClipRgn(fContext, rgn);
	xbox_assert(rgn != NULL);
	
	if (result == 0)
	{
		// No clipping
		_DeleteObject(rgn);
		rgn = NULL;
	}
	
	fClipStack.Push(rgn);
}


void VWinGDIGraphicContext::RestoreClip()
{
	StUseContext_NoRetain	context(this);
	
	HRGN	rgn = fClipStack.Pop();
	
	sLONG	result = ::SelectClipRgn(fContext, rgn);
	if (rgn != NULL)
		result = _DeleteObject(rgn);
}


void VWinGDIGraphicContext::ClipRect(const VRect& inHwndBounds, Boolean inAddToPrevious, Boolean inIntersectToPrevious)
{
#if VERSIONDEBUG
	if (sDebugNoClipping)
	{ 
		::SelectClipRgn(fContext,NULL);
		return;
	}
#endif
	BOOL result;
	StUseContext_NoRetain	context(this);

	// In case of offscreen drawing, port origin may have been moved
	// As SelectClip isn't aware of port origin, we offset the region manually
	VRect bounds(inHwndBounds);
	bounds.NormalizeToInt();

	POINT	offset;
	::GetViewportOrgEx(fContext, &offset);
	bounds.SetPosBy( offset.x, offset.y);
	
	_LPtoDP(bounds);
	
	RECT r=bounds;
	
	if (inAddToPrevious)
	{
		
		HRGN	newRgn = ::CreateRectRgnIndirect(&r);
		if (newRgn == NULL) vThrowNativeError(::GetLastError());
			
		result = ::ExtSelectClipRgn(fContext, newRgn, RGN_OR);
		if (!result) vThrowNativeError(::GetLastError());
		
		result = _DeleteObject(newRgn);
		if (!result) vThrowNativeError(::GetLastError());
	}
	else
	{
		if(inIntersectToPrevious)
		{
			result = ::IntersectClipRect(fContext, r.left, r.top, r.right, r.bottom);
			if (!result) vThrowNativeError(::GetLastError());
		}
		else
		{
			RECT r = bounds;
			HRGN	clip = ::CreateRectRgnIndirect(&r);
			if (clip == NULL) vThrowNativeError(::GetLastError());
			
			sLONG	cs = ::SelectClipRgn(fContext,clip);
			if (cs == ERROR) vThrowNativeError(::GetLastError());
			
			result = _DeleteObject(clip);
			if (!result) vThrowNativeError(::GetLastError());
		}
	}

	RevealClipping(fContext);
}


void VWinGDIGraphicContext::ClipRegion(const VRegion& inHwndRegion, Boolean inAddToPrevious, Boolean inIntersectToPrevious)
{
#if VERSIONDEBUG
	if (sDebugNoClipping)
	{
		::SelectClipRgn(fContext, NULL);
		return;
	}
#endif

	StUseContext_NoRetain	context(this);

	// In case of offscreen drawing, port origin may have been moved
	// As SelectClip isn't aware of port origin, we offset the region manually
	VRegion	newRgn(inHwndRegion);

	POINT	offset;
	::GetViewportOrgEx(fContext, &offset);
	newRgn.SetPosBy(offset.x, offset.y);
	
	if(fPrinterScale && fShouldScaleShapesWithPrinterScale)
	{
		newRgn.ScaleSizeBy(fPrinterScaleX,fPrinterScaleY);
		newRgn.ScalePosBy(fPrinterScaleX,fPrinterScaleY);
	}
	
	if (inAddToPrevious)
	{
		BOOL	result = ::ExtSelectClipRgn(fContext, newRgn, RGN_OR);
		if (!result) vThrowNativeError(::GetLastError());
	}
	else
	{
		if(inIntersectToPrevious)
		{
			BOOL	result = ::ExtSelectClipRgn(fContext, newRgn, RGN_AND);
			if (!result) vThrowNativeError(::GetLastError());
		}
		else
		{
			BOOL	result = ::SelectClipRgn(fContext, newRgn);
			if (!result) vThrowNativeError(::GetLastError());
		}
	}
	
#if VERSIONDEBUG
	RECT boxRect;
	::GetClipBox( fContext, &boxRect);
#endif

	RevealClipping(fContext);
}


void VWinGDIGraphicContext::GetClip(VRegion& outHwndRgn) const
{
	StUseContext_NoRetain	context(this);
	
	HRGN	newRgn = ::CreateRectRgn(0, 0, 0, 0);
	sLONG	result = ::GetClipRgn(fContext, newRgn);
	assert(result != -1);
	
	if (result == 0)
	{
		outHwndRgn.FromRect(VRect(-kMAX_sLONG, -kMAX_sLONG, kMAX_sLONG, kMAX_sLONG));
		_DeleteObject(newRgn);
	}
	else
		RegionGDIToLocal( newRgn, outHwndRgn, true, true, false);
}

/** create a VRegion in gc local user space from the GDI region (device space) */
void VWinGDIGraphicContext::RegionGDIToLocal(HRGN inRgn, VRegion& outRegion, bool inOwnIt, bool useViewportOrg, bool inUseWorldTransform) const
{
	VPoint translation;
	
	// In case of offscreen drawing, port origin may have been moved
	// As GDI clipping isn't aware of port origin, we offset the region manually
	if (useViewportOrg)
	{
		POINT	offset;
		::GetViewportOrgEx(fContext, &offset);
		translation.SetPos( -offset.x, -offset.y);
	}
	//if world transform is not identity, we need to inverse transform it too (because GDI regions are in device space & we want to get region in local user space)
	if (inUseWorldTransform && ::GetGraphicsMode( fContext) == GM_ADVANCED)
	{	
		VAffineTransform ctm;
		XFORM xform;
		::GetWorldTransform (fContext,&xform);
		ctm.FromNativeTransform(xform);

		if (!ctm.IsIdentity())
		{
			if (ctm.GetScaleX() == 1.0f && ctm.GetScaleY() == 1.0f
				&&
				ctm.GetShearX() == 0.0f && ctm.GetShearY() == 0.0f)
				//translation only
				translation.SetPosBy( -ctm.GetTranslateX(), -ctm.GetTranslateY());
			else 
			{
				//use Gdiplus region in order to properly convert from device space to vectorial local user space
				Gdiplus::Region region(inRgn);
				Gdiplus::Matrix ctmNative;
				ctm.Inverse().ToNativeMatrix( ctmNative);
				region.Transform( &ctmNative);
				Gdiplus::Graphics gc(fContext);
				HRGN hrgn = region.GetHRGN(&gc);
				outRegion.FromRegionRef(hrgn, true);
				if (translation.GetX() || translation.GetY())
					outRegion.SetPosBy(translation.GetX(), translation.GetY());
				if (inOwnIt)
					::DeleteRgn(inRgn);
				return;
			}
		}
	}

	outRegion.FromRegionRef(inRgn, inOwnIt);
	if (translation.GetX() || translation.GetY())
		outRegion.SetPosBy(translation.GetX(), translation.GetY());
		
	if(fPrinterScale && fShouldScaleShapesWithPrinterScale)
	{
		outRegion.ScaleSizeBy(1 / fPrinterScaleX,1 / fPrinterScaleY);
		outRegion.ScalePosBy(1 / fPrinterScaleX,1 / fPrinterScaleY);
	}
}

/** create a GDI region (device space) from a region in local user space */
HRGN VWinGDIGraphicContext::RegionLocalToGDI(const VRegion& inRegion, bool useViewportOrg, bool inUseWorldTransform) const
{
	VPoint translation;
	
	// In case of offscreen drawing, port origin may have been moved
	// As GDI clipping isn't aware of port origin, we offset the region manually
	if (useViewportOrg)
	{
		POINT	offset;
		::GetViewportOrgEx(fContext, &offset);
		translation.SetPos( offset.x, offset.y);
	}


	//if world transform is not identity, we need to inverse transform it too (because GDI regions are in device space & we want to get region in local user space)
	if (inUseWorldTransform && ::GetGraphicsMode(fContext) == GM_ADVANCED)
	{	
		VAffineTransform ctm;
		XFORM xform;
		::GetWorldTransform (fContext,&xform);
		ctm.FromNativeTransform(xform);

		if (!ctm.IsIdentity())
		{
			if (ctm.GetScaleX() == 1.0f && ctm.GetScaleY() == 1.0f
				&&
				ctm.GetShearX() == 0.0f && ctm.GetShearY() == 0.0f)
				//translation only
				translation.SetPosBy( ctm.GetTranslateX(), ctm.GetTranslateY());
			else 
			{
				//use Gdiplus region in order to properly convert to device space from vectorial local user space
				Gdiplus::Region region = inRegion;
				Gdiplus::Matrix ctmNative;
				ctm.ToNativeMatrix( ctmNative);
				region.Transform( &ctmNative);
				Gdiplus::Graphics gc(fContext);
				HRGN hrgn = region.GetHRGN(&gc);
				if (translation.GetX() || translation.GetY())
					::OffsetRgn(hrgn, translation.GetX(), translation.GetY());
				return hrgn;
			}
		}
	}

	const HRGN hrgnSrc = inRegion;
	if (hrgnSrc)
	{
		HRGN hrgn = ::CreateRectRgn(0, 0, 0, 0);
		::CopyRgn(hrgn, hrgnSrc);
		if (translation.GetX() || translation.GetY())
			::OffsetRgn(hrgn, translation.GetX(), translation.GetY());
		return hrgn;
	}
	else
		return NULL;
}


/** return current clipping bounding rectangle 
 @remarks
	bounding rectangle is expressed in VGraphicContext normalized coordinate space 
    (ie coordinate space with y axis pointing down - compliant with gdiplus and quickdraw coordinate spaces)
 */  
void VWinGDIGraphicContext::GetClipBoundingBox( VRect& outBounds)
{
	StUseContext_NoRetain	context(this);
	
	RECT boxRect;
	int type = ::GetClipBox( fContext, &boxRect);
	if (type == -1)
	{
		vThrowNativeError(::GetLastError());
		outBounds.SetCoords( -kMAX_sLONG, -kMAX_sLONG, kMAX_sLONG, kMAX_sLONG);
	}
	else if (type == ERROR || type == NULLREGION)
		outBounds.SetCoords( -kMAX_sLONG, -kMAX_sLONG, kMAX_sLONG, kMAX_sLONG);
	else
	{
		outBounds.FromRectRef( boxRect);
		
		POINT	offset;
		::GetViewportOrgEx(fContext, &offset);
		outBounds.SetPosBy(-offset.x, -offset.y);
	}
}


void VWinGDIGraphicContext::RevealClipping(ContextRef inContext)
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


void VWinGDIGraphicContext::RevealUpdate(HWND inWindow)
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


void VWinGDIGraphicContext::RevealBlitting(ContextRef inContext, const RgnRef inHwndRegion)
{
#if VERSIONDEBUG
	if (sDebugRevealBlitting)
	{
		HRGN	curClip = ::CreateRectRgn(0, 0, 0, 0);
		int	restoreClip = ::GetClipRgn(inContext, curClip);
		
		::SelectClipRgn(inContext, NULL);
		
		HBRUSH	brush = ::CreateSolidBrush(RGB(210, 155, 0));
		::FillRgn(inContext, inHwndRegion, brush);
		::DeleteObject(brush);
		
		if (restoreClip)
			::SelectClipRgn(inContext, curClip);
		else
			::SelectClipRgn(inContext, NULL);
			
		::DeleteObject(curClip);
		::Sleep(kDEBUG_REVEAL_DELAY);
	}
#endif
}


void VWinGDIGraphicContext::RevealInval(ContextRef inContext, const RgnRef inHwndRegion)
{
#if VERSIONDEBUG
	if (sDebugRevealInval)
	{
		HRGN	curClip = ::CreateRectRgn(0, 0, 0, 0);
		int	restoreClip = ::GetClipRgn(inContext, curClip);
		
		::SelectClipRgn(inContext, NULL);
		
		HBRUSH	brush = ::CreateSolidBrush(RGB(210, 255, 0));
		::FrameRgn(inContext, inHwndRegion, brush, 4, 4);
		::DeleteObject(brush);
		
		if (restoreClip)
			::SelectClipRgn(inContext, curClip);
		else
			::SelectClipRgn(inContext, NULL);
			
		::DeleteObject(curClip);
		::Sleep(kDEBUG_REVEAL_DELAY);
	}
#endif
}


#pragma mark-

void VWinGDIGraphicContext::CopyContentTo( VGraphicContext* inDestContext, const VRect* inSrcBounds, const VRect* inDestBounds, TransferMode inMode, const VRegion* inMask)
{

	if (inDestContext == NULL) return;
	
	HDC destDC;
	// Prepare the transfer
	DWORD	transMode;
	switch (inMode)
	{
		case TM_COPY:
			transMode = SRCCOPY;
			break;
			
		case TM_OR:
			transMode = SRCPAINT;
			break;
			
		case TM_XOR:
			transMode = SRCINVERT;
			break;
			
		case TM_ERASE:
			transMode = SRCERASE;
			break;
			
		case TM_NOT_COPY:
			transMode = NOTSRCCOPY;
			break;
			
		case TM_NOT_OR:
			transMode = MERGEPAINT;
			break;
			
		case TM_NOT_ERASE:
			transMode = NOTSRCERASE;
			break;
			
		case TM_AND:
			transMode = SRCAND;
			break;
			
		case TM_BLEND:
			break;	
			
		case TM_TRANSPARENT:
		case TM_NOT_XOR:
		default:
			assert(false);
			break;
	}
	
	StUseContext_NoRetain	srcContext(this);
	StUseContext	dstContext(inDestContext);
	
	// Set clipping
	if (inMask != NULL)
	{
		// Clipping will be restored, we can overide constness
		const_cast<VGraphicContext*>(inDestContext)->SaveClip();
		const_cast<VGraphicContext*>(inDestContext)->ClipRegion(inMask);
	}
	
	destDC=inDestContext->BeginUsingParentContext();
	
	// Prepare colors
	::SetTextColor(destDC, fPixelForeColor.WIN_ToCOLORREF());
	::SetBkColor(destDC, fPixelBackColor.WIN_ToCOLORREF());
	
	SIZE	contextSize;
	BOOL	result = ::GetViewportExtEx(fContext, &contextSize);
	
	sLONG	srcX = inSrcBounds != NULL ? inSrcBounds->GetX() : 0;
	sLONG	srcY = inSrcBounds != NULL ? inSrcBounds->GetY() : 0;
	sLONG	srcWidth = inSrcBounds != NULL ? inSrcBounds->GetWidth() : contextSize.cx;
	sLONG	srcHeight = inSrcBounds != NULL ? inSrcBounds->GetHeight() : contextSize.cy;
	sLONG	destX = inDestBounds != NULL ? inDestBounds->GetX() : 0;
	sLONG	destY = inDestBounds != NULL ? inDestBounds->GetY() : 0;
	sLONG	destWidth = inDestBounds != NULL ? inDestBounds->GetWidth() : contextSize.cx;
	sLONG	destHeight = inDestBounds != NULL ? inDestBounds->GetHeight() : contextSize.cy;
	
	if (inMode == TM_BLEND)
	{
		// Alpha blending
		BLENDFUNCTION	blendMode;
		blendMode.BlendOp = AC_SRC_OVER;
		blendMode.BlendFlags = 0;
		blendMode.SourceConstantAlpha = fAlphaBlend;
		blendMode.AlphaFormat = 0;
		::AlphaBlend(destDC, destX, destY, destWidth, destHeight, fContext, srcX, srcY, srcWidth, srcHeight, blendMode);
	}
	else if (srcWidth == destWidth && srcHeight == destHeight)
	{
		// Simple blit
		::BitBlt(destDC, destX, destY, destWidth, destHeight, fContext, srcX, srcY, transMode);
	}
	else
	{
		// Streched blit
		sLONG	destStretch = ::SetStretchBltMode(destDC, COLORONCOLOR);
		sLONG	srcSretch = ::SetStretchBltMode(fContext, COLORONCOLOR);
		::StretchBlt(destDC, destX, destY, destWidth, destHeight, fContext, srcX, srcY, srcWidth, srcHeight, transMode);
		::SetStretchBltMode(destDC, destStretch);
		::SetStretchBltMode(fContext, srcSretch);
	}
	inDestContext->EndUsingParentContext(destDC);
	// Restore clipping
	if (inMask != NULL)
		const_cast<VGraphicContext*>(inDestContext)->RestoreClip();
	
}

VPictureData* VWinGDIGraphicContext::CopyContentToPictureData()
{
	return NULL;
}

void VWinGDIGraphicContext::FillUniformColor(const VPoint& inContextPos, const VColor& inColor, const VPattern* inPattern)
{
	StUseContext_NoRetain	context(this);
	
	VColor*	pixelColor = GetPixelColor(inContextPos);
	COLORREF	winColor = 0x02000000 | pixelColor->GetRGBColor();
	
	SetFillColor(inColor);
	
	::ExtFloodFill(fContext, inContextPos.GetX(), inContextPos.GetY(), winColor, FLOODFILLSURFACE);
}


void VWinGDIGraphicContext::SetPixelColor(const VPoint& inContextPos, const VColor& inColor)
{
	::SetPixel(fContext, inContextPos.GetX(), inContextPos.GetY(), inColor.WIN_ToCOLORREF());
}


VColor* VWinGDIGraphicContext::GetPixelColor(const VPoint& inContextPos) const
{
	VColor *color = new VColor;
	if (color != NULL)
		color->WIN_FromCOLORREF( ::GetPixel(fContext, inContextPos.GetX(), inContextPos.GetY()));
	return color;
}


void VWinGDIGraphicContext::Flush()
{
	::GdiFlush();
}


#pragma mark-

void	VWinGDIGraphicContext::_LPtoDP(VPoint &ioPoint)
{
	if(fPrinterScale && fShouldScaleShapesWithPrinterScale)
	{
		ioPoint.Set(ioPoint.GetX()*fPrinterScaleX,ioPoint.GetY()*fPrinterScaleY);
	}
}

void	VWinGDIGraphicContext::_LPtoDP(VRect &ioRect)
{
	if(fPrinterScale && fShouldScaleShapesWithPrinterScale)
	{
		ioRect.ScaleSizeBy (fPrinterScaleX, fPrinterScaleY);
		ioRect.ScalePosBy (fPrinterScaleX, fPrinterScaleY);
	}
}
	
void	VWinGDIGraphicContext::_DPtoLP(VPoint &ioPoint)
{
	POINT p=ioPoint;
	DPtoLP(fContext,&p,1);
	ioPoint.Set(p.x,p.y);
}
void	VWinGDIGraphicContext::_DPtoLP(VRect &ioRect)
{
	RECT re=ioRect;
	DPtoLP(fContext,(LPPOINT)&re,2);
	ioRect.SetCoords(re.left,re.top,re.right-re.left,re.bottom-re.top);
}


bool VWinGDIGraphicContext::_BeginLayer(	 const VRect& inBounds, 
								 GReal /*inAlpha*/, 
								 VGraphicFilterProcess * /*inFilterProcess*/,
								 VImageOffScreenRef inBackBuffer,
								 bool inInheritParentClipping,
								 bool /*inTransparent*/)
{
	if (fOffScreenUseCount)
	{
		//with this context, we can only use one offscreen at a time
		fOffScreenUseCount++;
		return true;
	}
	xbox_assert(fOffScreen == NULL);
	fOffScreenParent = fContext;
	if (!inBackBuffer)
	{
		fOffScreenUseCount++;
		return true;
	}
	if (inBackBuffer == (VImageOffScreenRef)0xFFFFFFFF)
		inBackBuffer = NULL;
	if (fPrinterScale) //skip if printing
	{
		fOffScreenUseCount++;
		CopyRefCountable(&fOffScreen, inBackBuffer);
		return fOffScreen == NULL;
	}
	if (fContextRefcon <= 0)
	{
		//skip if printing (fPrinterScale is not valid yet)
		HDC screendc = ::GetDC(0);
		long reflogx = ::GetDeviceCaps( screendc, LOGPIXELSX );
		::ReleaseDC(NULL ,screendc );
		long logx = ::GetDeviceCaps( fContext, LOGPIXELSX );

		if (logx!=reflogx)
		{
			fOffScreenUseCount++;
			CopyRefCountable(&fOffScreen, inBackBuffer);
			return fOffScreen == NULL;
		}
	}

	//skip if region is empty
	VRect bounds;
	int mode = GetGraphicsMode (fContext);
	if(mode == GM_ADVANCED)
	{
		XFORM xform;
		::GetWorldTransform (fContext, &xform);
		VAffineTransform ctm;
		ctm.FromNativeTransform( xform);
		bounds = ctm.TransformRect( inBounds);
	}
	else 
		bounds.FromRect( inBounds);
	bounds.NormalizeToInt();

	if (bounds.GetWidth() == 0 || bounds.GetHeight() == 0)
	{
		fOffScreenUseCount++;
		CopyRefCountable(&fOffScreen, inBackBuffer);
		return true;
	}

	//create/update offscreen
	fOffScreen = inBackBuffer;
	if (fOffScreen)
		fOffScreen->Retain();
	else
		fOffScreen = new VImageOffScreen( static_cast<VGraphicContext *>(this));
	if (!fOffScreen)
	{
		fOffScreenUseCount++;
		return true;
	}

	//begin render in offscreen
	VGDIOffScreen *gdioffscreen = fOffScreen->GetGDIOffScreen();
	bool isNewBmp = false;
	fContext = gdioffscreen->Begin( fContext, inBounds, &isNewBmp, inInheritParentClipping);
	
	fOffScreenUseCount++;

	return isNewBmp;
}

VImageOffScreenRef VWinGDIGraphicContext::_EndLayer()
{
	if (fOffScreenUseCount <= 0)
	{
		xbox_assert(fOffScreen == NULL);
		return NULL;
	}
	if (fOffScreenUseCount > 1)
	{
		fOffScreenUseCount--;
		return NULL;
	}
	xbox_assert(fOffScreenUseCount == 1);
	
	//end render in offscreen & blit offscreen to parent context
	if (fOffScreen)
	{
		VGDIOffScreen *gdioffscreen = fOffScreen->GetGDIOffScreen();
		gdioffscreen->End( fContext);
	}
	fContext = fOffScreenParent;

	VImageOffScreen *imageOffScreen = RetainRefCountable(fOffScreen);
	XBOX::ReleaseRefCountable(&fOffScreen);
	fOffScreenUseCount = 0;
	return imageOffScreen;
}

bool VWinGDIGraphicContext::ShouldClearLayerOffScreen(const VRect& inBounds, VImageOffScreenRef inBackBuffer)
{
	if (!inBackBuffer)
		return true;

	if (!inBackBuffer->IsGDIImpl())
		return true;
	VGDIOffScreen *gdioffscreen = inBackBuffer->GetGDIOffScreen();
	return gdioffscreen->ShouldClear( fContext, inBounds);
}

bool VWinGDIGraphicContext::DrawLayerOffScreen(const VRect& inBounds, VImageOffScreenRef inBackBuffer, bool /*inDrawAlways*/)
{
	if (!inBackBuffer)
		return false;
	if (fPrinterScale) //skip if printing
		return false;
	if (fContextRefcon <= 0)
	{
		//skip if printing (fPrinterScale is not valid yet)
		HDC screendc = ::GetDC(0);
		long reflogx = ::GetDeviceCaps( screendc, LOGPIXELSX );
		::ReleaseDC(NULL ,screendc );
		long logx = ::GetDeviceCaps( fContext, LOGPIXELSX );

		if (logx!=reflogx)
			return false;
	}
	VGDIOffScreen *gdioffscreen = inBackBuffer->GetGDIOffScreen();
	return gdioffscreen->Draw( fContext, inBounds, true);
}


void VWinGDIGraphicContext::BeginUsingContext(bool /*inNoDraw*/)
{
	SIZE oldExtend,wExt,vExt;
	if (fContextRefcon == 0)
	{		
		// Using GDI, previous brushes are stored when new one are used.
		// EndUsingContext will restore the DC state using those values.
		//
		// Make sure to allways balance a BeginUsingContext call with EndUsingContext
		assert(fSavedTextColor == CLR_INVALID);
		assert(fSavedBackColor == CLR_INVALID);
		assert(fSavedPen == NULL);
		assert(fSavedBrush == NULL);
		assert(fSavedRop == -1);
		assert(fSavedBkMode == -1);
		assert(fSavedTextAlignment == -1);
		fSavedFont=(HFONT)::GetCurrentObject(fContext,OBJ_FONT);
		if(fSavedFont==NULL)
			fSavedFont = (HFONT)GetStockObject(SYSTEM_FONT);
		assert(fSavedGraphicsMode == -1);
		int mode = ::GetGraphicsMode(fContext);
		if (mode != GM_ADVANCED)
		{
			//ensure we take account world transform (especially if context is derived from another context...)
			::SetGraphicsMode(fContext, GM_ADVANCED);
			fSavedGraphicsMode = mode;
		}
		
		fSavedRop = ::SetROP2(fContext, R2_COPYPEN);
		fSavedBkMode = ::SetBkMode(fContext, OPAQUE);

		::GetWindowExtEx(fContext,&fSaveWindowExtend);
		::GetViewportExtEx(fContext,&fSaveViewPortExtend);
		fSaveMapMode =:: GetMapMode(fContext);
		fSavedTextAlignment=::GetTextAlign(fContext);

		_InitPrinterScale();
		
		if(fPrinterScale)
		{
#if VERSIONDEBUG
			POINT	offset;
			::GetViewportOrgEx(fContext, &offset);
#endif
			
			fSaveMapMode = SetMapMode(fContext,MM_TEXT);
		}
		
	}
	DebugRegisterHDC(fContext);
	fContextRefcon++;
}


void VWinGDIGraphicContext::EndUsingContext()
{
	fContextRefcon--;

	DebugUnRegisterHDC(fContext);

	if (fContextRefcon == 0)
	{
#if 0
		// See BeginUsingContext for explanations
		if (fSavedTextColor != CLR_INVALID)
		{
			::SetTextColor(fContext, fSavedTextColor);
			fSavedTextColor = CLR_INVALID;
		}
		
		if (fSavedBackColor != CLR_INVALID)
		{
			::SetBkColor(fContext, fSavedBackColor);
			fSavedBackColor = CLR_INVALID;
		}
		
		if (fSavedPen != NULL)
		{
			HGDIOBJ	curObj = ::SelectObject(fContext, fSavedPen);
			_DeleteObject(curObj);
			assert(curObj == fCurrentPen);
			
			fCurrentPen = NULL;
			fSavedPen = NULL;
		}
		
		if (fSavedBrush != NULL)
		{
			HGDIOBJ	curObj = ::SelectObject(fContext, fSavedBrush);
			_DeleteObject(curObj);
			assert(curObj == fCurrentBrush);
			
			fSavedBrush = NULL;
			fCurrentBrush = NULL;
		}
		
		if (fFillPattern != NULL)
		{
			fFillPattern->ReleaseFromContext(this);
			fFillPattern->Release();
			fFillPattern = NULL;
		}
		
//		if (fLinePattern != NULL)
//		{
//			fLinePattern->ReleaseFromContext(this);
//			fLinePattern->Release();
//			fLinePattern = NULL;
//		}
		
		if (fSavedRop != -1)
		{
			::SetROP2(fContext, fSavedRop);
			fSavedRop = -1;
		}
		
		if (fSavedBkMode != -1)
		{
			::SetBkMode(fContext, fSavedBkMode);
			fSavedBkMode = -1;
		}
		
		if (fSavedTextAlignment != -1 )
		{
			::SetTextAlign(fContext, fSavedTextAlignment);
			fSavedTextAlignment = -1;
		}
#else
		if (fSavedGraphicsMode != -1)
		{
			::SetGraphicsMode( fContext, fSavedGraphicsMode);
			fSavedGraphicsMode = -1;
		}
		HGDIOBJ oldobj;
		// on ne cherche pas a restaurer le contexte, RestoreContext() est la pour ca
		// See BeginUsingContext for explanations
		fSavedTextColor = CLR_INVALID;
		fSavedBackColor = CLR_INVALID;
		
		// don't leave our objects in context
		if (fCurrentPen != NULL)
		{
			
			if(fSavedPen!=NULL)
			{
				oldobj=SelectObject( fContext,fSavedPen);
				if(fCurrentPen!=fSavedPen)
					_DeleteObject(fCurrentPen);
			}
			else
			{
				oldobj = SelectObject( fContext,GetStockObject( NULL_PEN));
				_DeleteObject(fCurrentPen);
			}	
			
		}
		fCurrentPen = NULL;
		fSavedPen = NULL;
		
		if (fCurrentBrush != NULL)
		{
			if(fSavedBrush!=NULL)
			{
				oldobj=SelectObject( fContext,fSavedBrush);
				DeleteObject(fCurrentBrush);
			}
			else
			{
				oldobj = SelectObject( fContext,GetStockObject( NULL_BRUSH));
			}	
			DeleteObject(fCurrentBrush);
		}
		if(fFillBrush!=NULL && fFillBrush!=fCurrentBrush)
			DeleteObject(fFillBrush);
		
		fFillBrush=NULL;
		fCurrentBrush = NULL;
		fSavedBrush = NULL;
		fFillBrushDirty=true;
		fLineBrushDirty=true;
		
		if (fFillPattern != NULL)
		{
			fFillPattern->ReleaseFromContext(this);
			fFillPattern->Release();
			fFillPattern = NULL;
		}
	
		::SetMapMode(fContext,fSaveMapMode);
		::SetWindowExtEx(fContext,fSaveWindowExtend.cx,fSaveWindowExtend.cy,NULL);
		::SetViewportExtEx(fContext,fSaveViewPortExtend.cx,fSaveViewPortExtend.cy,NULL);
		
		if (fSavedTextAlignment != -1 )
		{
			::SetTextAlign(fContext, fSavedTextAlignment);
		}

		SelectObject(fContext,fSavedFont);
		
//		if (fLinePattern != NULL)
//		{
//			fLinePattern->ReleaseFromContext(this);
//			fLinePattern->Release();
//			fLinePattern = NULL;
//		}
		
		fSavedRop = -1;
		fSavedBkMode = -1;
		fSavedTextAlignment = -1;
#endif
	}
	
	assert(fContextRefcon >= 0);
}

bool VWinGDIGraphicContext::BeginQDContext()
{
	bool result=false;
	if(testAssert(fContextRefcon<=1))
	{
		if(fContextRefcon==1)
		{
			HRGN	newRgn = ::CreateRectRgn(0, 0, 0, 0);
			sLONG	result = ::GetClipRgn(fContext, newRgn);
			
			SaveClip();
			EndUsingContext();
			fQDContext_NeedsBeginUsing = true;

			::SelectClipRgn(fContext,newRgn);
			::DeleteObject(newRgn);
		}
		result=true;
	}
	return result;
}
void VWinGDIGraphicContext::EndQDContext(bool inDontRestoreClip) //pp inDontRestoreClip in not use on windows, mac only
{
	if(fQDContext_NeedsBeginUsing)
	{
		fQDContext_NeedsBeginUsing=false;
		BeginUsingContext();
		RestoreClip();
	}
}

#pragma mark-

void	VWinGDIGraphicContext::_UpdateFillBrush()
{
	if(fFillBrushDirty || GetCurrentObject(fContext,OBJ_BRUSH)!=fFillBrush)
	{
		SelectFillBrushDirect(fFillBrush);
		fFillBrushDirty=false;
	}
}

void	VWinGDIGraphicContext::_UpdateLineBrush()
{
	if(fLineBrushDirty || GetCurrentObject(fContext,OBJ_PEN)!=fCurrentPen )
	{
		HGDIOBJ	newPen = _CreatePen();
		SelectPenDirect( newPen);
		fLineBrushDirty=false;
	}
}

void	VWinGDIGraphicContext::SelectFillBrushDirect (HGDIOBJ inBrush)
{
		_SelectBrushDirect(inBrush);
		fFillBrushDirty=true;
}
void	VWinGDIGraphicContext::SelectLineBrushDirect (HGDIOBJ inBrush)
{
		_SelectBrushDirect(inBrush);
		fLineBrushDirty=true;
}

void VWinGDIGraphicContext::_SelectBrushDirect(HGDIOBJ inBrush)
{
	if (inBrush == NULL) return;
	
	if (fCurrentBrush != inBrush)
	{
		HGDIOBJ old = ::SelectObject( fContext, inBrush);
		xbox_assert( (fCurrentBrush == NULL) || (fCurrentBrush == old));
		if (fCurrentBrush == NULL)
		{
			// this was not our brush, save it to restore it in EndUsingContext
			xbox_assert( fSavedBrush == NULL);
			fSavedBrush = old;
		}
		else
		{
			if(old!=fFillBrush)
			{
				BOOL ok = _DeleteObject( old);
				xbox_assert( ok);
			}
		}
		fCurrentBrush = inBrush;
	}
	else
	{
		HGDIOBJ cur=::GetCurrentObject(fContext,OBJ_BRUSH);
		assert(cur==fCurrentBrush);
	}
}


void VWinGDIGraphicContext::SelectPenDirect(HGDIOBJ inPen)
{
	if (inPen == NULL) return;
	
	if (inPen != fCurrentPen)
	{
		HGDIOBJ old = ::SelectObject( fContext, inPen);
		xbox_assert( (fCurrentPen == NULL) || (fCurrentPen == old) );
		if (fCurrentPen == NULL)
		{
			// this was not our pen, save it to restore it in EndUsingContext
			xbox_assert( fSavedPen == NULL);
			fSavedPen = old;
		}
		else
		{
			BOOL ok = _DeleteObject( old);
			xbox_assert( ok);
		}
		fCurrentPen = inPen;
	}
	else
	{
		HGDIOBJ cur=::GetCurrentObject(fContext,OBJ_PEN);
		assert(cur==fCurrentPen);
	}
}


void VWinGDIGraphicContext::_AdjustLineCoords(GReal* ioCoords)
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
				ioCoords[3]+=1;
			else
				ioCoords[3]-=1;
		}
		else if (ioCoords[3] == ioCoords[1])
		{
			if (ioCoords[2] >= ioCoords[0])
				ioCoords[2]+=1;
			else
				ioCoords[2]-=1;
		}
		else
		{
			// Add end point
			sLONG	hDiff = Abs(ioCoords[2] - ioCoords[0]);
			sLONG	vDiff = Abs(ioCoords[3] - ioCoords[1]);
			sLONG	moreX = (hDiff >= (vDiff / 2)) ? 1 : 0;
			sLONG	moreY = (vDiff >= (hDiff / 2)) ? 1 : 0;
	
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


PatternRef VWinGDIGraphicContext::_CreateStandardPattern(ContextRef /*inContext*/, PatternStyle inStyle, void* ioStorage)
{
	Boolean	isFillPatten = _GetStdPattern(inStyle, ioStorage);	
	HBITMAP	bitMap = ::CreateBitmap(8, 8, 1, 1, ioStorage);
	PatternRef result = ::CreatePatternBrush(bitMap);
	::DeleteObject(bitMap);
	return result;
}


void VWinGDIGraphicContext::_ReleasePattern(ContextRef inContext, PatternRef inPatternRef)
{
	// Make sure inPatternRef is not used by inContext
	if (::GetCurrentObject( inContext, OBJ_BRUSH) != inPatternRef)
		::DeleteObject(inPatternRef);
}


void VWinGDIGraphicContext::_ApplyPattern(ContextRef inContext, PatternStyle inStyle, const VColor& inColor, PatternRef inPatternRef)
{
	if (inPatternRef == NULL) return;

	::SetTextColor(inContext, inColor.WIN_ToCOLORREF());
	//SelectFillBrushDirect(inPatternRef);
	// CAUTION: make sure SelectPenDirect and SelectBrushDirect have been called before (to reset current brushes).
	HGDIOBJ	prevBrush = ::SelectObject(inContext, inPatternRef);
	::DeleteObject(prevBrush);
}


GradientRef VWinGDIGraphicContext::_CreateAxialGradient(ContextRef /*inContext*/, const VAxialGradientPattern* inGradient)
{
	return NULL;
}


GradientRef VWinGDIGraphicContext::_CreateRadialGradient(ContextRef /*inContext*/, const VRadialGradientPattern* inGradient)
{
	return NULL;
}


void VWinGDIGraphicContext::_ReleaseGradient(GradientRef inShading)
{
}


void VWinGDIGraphicContext::_ApplyGradient(ContextRef inContext, GradientRef inShading)
{
}


Boolean VWinGDIGraphicContext::_GetStdPattern(PatternStyle inStyle, void* ioPattern)
{
	Boolean	isFillPattern = (inStyle <= PAT_FILL_LAST);
	
	switch (inStyle)
	{
		case PAT_PLAIN:
			((uLONG*) ioPattern)[0] = 0xFFFFFFFF;
			((uLONG*) ioPattern)[1] = 0xFFFFFFFF;
			break;
			
		case PAT_LITE_GRAY:
			((uLONG*) ioPattern)[0] = 0x88228822;
			((uLONG*) ioPattern)[1] = 0x88228822;
			break;
			
		case PAT_MED_GRAY:
			((uLONG*) ioPattern)[0] = 0xAA55AA55;
			((uLONG*) ioPattern)[1] = 0xAA55AA55;
			break;
			
		case PAT_DARK_GRAY:
			((uLONG*) ioPattern)[0] = 0x77DD77DD;
			((uLONG*) ioPattern)[1] = 0x77DD77DD;
			break;
			
		case PAT_H_HATCH:
			((uLONG*) ioPattern)[0] = 0xFF000000;
			((uLONG*) ioPattern)[1] = 0xFF000000;
			break;
			
		case PAT_V_HATCH:
			((uLONG*) ioPattern)[0] = 0x88888888;
			((uLONG*) ioPattern)[1] = 0x88888888;
			break;
			
		case PAT_HV_HATCH:
			((uLONG*) ioPattern)[0] = 0xFF888888;
			((uLONG*) ioPattern)[1] = 0xFF888888;
			break;
			
		case PAT_R_HATCH:
			((uLONG*) ioPattern)[0] = 0x01020408;
			((uLONG*) ioPattern)[1] = 0x10204080;
			break;
			
		case PAT_L_HATCH:
			((uLONG*) ioPattern)[0] = 0x80402010;
			((uLONG*) ioPattern)[1] = 0x08040201;
			break;
			
		case PAT_LR_HATCH:
			((uLONG*) ioPattern)[0] = 0x82442810;
			((uLONG*) ioPattern)[1] = 0x28448201;
			break;
		
		default:
			((uLONG*) ioPattern)[0] = 0xFFFFFFFF;
			((uLONG*) ioPattern)[1] = 0xFFFFFFFF;
			break;
	}
	
	return isFillPattern;
}


void VWinGDIGraphicContext::_DrawShadow(ContextRef inContext, const VPoint& inOffset, const VRect& inHwndBounds)
{
	// Shadow size may be large (3 pixels) or small (1 pixel)
	// Direcion and size are specified by inOffset
	if (inHwndBounds.GetWidth() < 0.0 || inHwndBounds.GetHeight() < 0.0) return;
	
	Boolean	isLargeHShadow = inOffset.GetX() > 2.0;
	Boolean	isLargeVShadow = inOffset.GetY() > 2.0;
	Boolean	isTopShadow =  inOffset.GetY() < 0;
	Boolean	isBottomShadow =  inOffset.GetY() > 0;
	Boolean	isLeftShadow =  inOffset.GetX() < 0;
	Boolean	isRightShadow =  inOffset.GetX() > 0;
	Boolean	fullCornerShadow = (isTopShadow && (isLeftShadow || isRightShadow)) || (isBottomShadow && (isLeftShadow || isRightShadow));
	
	_StSelectObject	pen(inContext,::GetStockObject(BLACK_PEN));
	
	COLORREF	oldColor = ::SetTextColor(inContext, sShadow1Color->WIN_ToCOLORREF());

	if (isRightShadow && !isTopShadow)
	{
		::MoveToEx(inContext, inHwndBounds.GetRight(), inHwndBounds.GetTop() + 1.0, 0);
		::LineTo(inContext, inHwndBounds.GetRight(), inHwndBounds.GetBottom());
		if (isBottomShadow)
			::LineTo(inContext, inHwndBounds.GetLeft() + 1.0, inHwndBounds.GetBottom());
		
		::SetTextColor(inContext, sShadow2Color->WIN_ToCOLORREF());
		if (isLargeHShadow)
		{
			::MoveToEx(inContext, inHwndBounds.GetRight() + 1.0, inHwndBounds.GetTop() + 2.0, 0);
			::LineTo(inContext, inHwndBounds.GetRight() + 1.0, inHwndBounds.GetBottom() - (isBottomShadow ? 0.0 : 1.0));
		}
		
		if (isLargeVShadow && isBottomShadow)
		{
			::MoveToEx(inContext, inHwndBounds.GetRight() - (isLargeHShadow ? 0.0 : 1.0), inHwndBounds.GetBottom() + 1.0, 0);
			::LineTo(inContext, inHwndBounds.GetLeft() + 2.0, inHwndBounds.GetBottom() + 1.0);
		}
		
		::SetTextColor(inContext, sShadow3Color->WIN_ToCOLORREF());
		if (isLargeHShadow)
		{
			::MoveToEx(inContext, inHwndBounds.GetRight() + 2.0, inHwndBounds.GetTop() + 3.0, 0);
			::LineTo(inContext, inHwndBounds.GetRight() + 2.0, inHwndBounds.GetBottom() - (isBottomShadow ? 0.0 : 2.0));
		}
		
		if (isLargeVShadow && isBottomShadow)
		{
			::MoveToEx(inContext, inHwndBounds.GetRight() - (isLargeHShadow ? 0.0 : 2.0), inHwndBounds.GetBottom() + 2.0, 0);
			::LineTo(inContext, inHwndBounds.GetLeft() + 3.0, inHwndBounds.GetBottom() + 2.0);
		}
	}
	else if (isLeftShadow && !isTopShadow)
	{
		::MoveToEx(inContext, inHwndBounds.GetLeft() - 1.0, inHwndBounds.GetTop() + 1.0, 0);
		::LineTo(inContext, inHwndBounds.GetLeft() - 1.0, inHwndBounds.GetBottom());
		if (isBottomShadow)
			::LineTo(inContext, inHwndBounds.GetRight(), inHwndBounds.GetBottom());
		
		::SetTextColor(inContext, sShadow2Color->WIN_ToCOLORREF());
		if (isLargeHShadow)
		{
			::MoveToEx(inContext, inHwndBounds.GetLeft() - 2.0, inHwndBounds.GetTop() + 2.0, 0);
			::LineTo(inContext, inHwndBounds.GetLeft() - 2.0, inHwndBounds.GetBottom() - (isBottomShadow ? 0.0 : 1.0));
		}
		
		if (isLargeVShadow && isBottomShadow)
		{
			::MoveToEx(inContext, inHwndBounds.GetRight() - 1.0, inHwndBounds.GetBottom() + 1.0, 0);
			::LineTo(inContext, inHwndBounds.GetLeft() + (isLargeHShadow ? -2.0 : 1.0), inHwndBounds.GetBottom() + 1.0);
		}
		
		::SetTextColor(inContext, sShadow3Color->WIN_ToCOLORREF());
		if (isLargeHShadow)
		{
			::MoveToEx(inContext, inHwndBounds.GetLeft() - 3.0, inHwndBounds.GetTop() + 3.0, 0);
			::LineTo(inContext, inHwndBounds.GetLeft() - 3.0, inHwndBounds.GetBottom() - (isBottomShadow ? 0.0 : 2.0));
		}
		
		if (isLargeVShadow && isBottomShadow)
		{
			::MoveToEx(inContext, inHwndBounds.GetRight() - 2.0, inHwndBounds.GetBottom() + 2.0, 0);
			::LineTo(inContext, inHwndBounds.GetLeft() + (isLargeHShadow ? -3.0 : 2.0), inHwndBounds.GetBottom() + 2.0);
		}
	}
	else
	{
		assert(false);
		//	TBD
	}
	
	::SetTextColor(inContext, oldColor);
	
}
void VWinGDIGraphicContext::_DrawShadow (ContextRef inContext, const VPoint& inOffset, const GReal inStartPointX ,const GReal inStartPointY, const GReal inEndPointX,const GReal inEndPointY)
{
	VPoint startPoint(inStartPointX,inStartPointY),endPoint(inEndPointX,inEndPointY);
	_DrawShadow(inContext, inOffset, startPoint,endPoint);
}

void VWinGDIGraphicContext::_DrawShadow(ContextRef inContext, const VPoint& inOffset, const VPoint& inStartPoint, const VPoint& inEndPoint)
{
	// Shadow size may be large (3 pixels) or small (1 pixel)
	// Direcion and size are specified by inOffset
	Boolean	isLargeHShadow = inOffset.GetX() > 2.0;
	Boolean	isLargeVShadow = inOffset.GetY() > 2.0;
	Boolean	isTopShadow = inOffset.GetY() < 0;
	Boolean	isBottomShadow = inOffset.GetY() > 0;
	Boolean	isLeftShadow = inOffset.GetX() < 0;
	Boolean	isRightShadow = inOffset.GetX() > 0;
	
	GReal	hDelta = inEndPoint.GetX() - inStartPoint.GetX();
	GReal	vDelta = inEndPoint.GetY() - inStartPoint.GetY();
	
	_StSelectObject	pen(inContext,::GetStockObject(BLACK_PEN));
	COLORREF	oldColor = ::SetTextColor(inContext, sShadow1Color->WIN_ToCOLORREF());
	
	if (hDelta == 0)
	{
		// Vertical only
		if (isLeftShadow)
		{
			::MoveToEx(inContext, inStartPoint.GetX() - 1.0, inStartPoint.GetY() + 1.0, 0);
			::LineTo(inContext, inEndPoint.GetX() - 1.0, inEndPoint.GetY() - 1.0);
			if (isLargeVShadow)
			{
				::SetTextColor(inContext, sShadow2Color->WIN_ToCOLORREF());
				::MoveToEx(inContext, inStartPoint.GetX() - 2.0, inStartPoint.GetY() + 2.0, 0);
				::LineTo(inContext, inEndPoint.GetX() - 2.0, inEndPoint.GetY() - 3.0);
				::SetTextColor(inContext, sShadow3Color->WIN_ToCOLORREF());
				::MoveToEx(inContext, inStartPoint.GetX() - 3.0, inStartPoint.GetY() + 3.0, 0);
				::LineTo(inContext, inEndPoint.GetX() - 3.0, inEndPoint.GetY() - 5.0);
			}
		}
		else if (isRightShadow)
		{
			::MoveToEx(inContext, inStartPoint.GetX(), inStartPoint.GetY() + 1.0, 0);
			::LineTo(inContext, inEndPoint.GetX(), inEndPoint.GetY() - 1.0);
			if (isLargeVShadow)
			{
				::SetTextColor(inContext, sShadow2Color->WIN_ToCOLORREF());
				::MoveToEx(inContext, inStartPoint.GetX() + 1.0, inStartPoint.GetY() + 2.0, 0);
				::LineTo(inContext, inEndPoint.GetX() + 1.0, inEndPoint.GetY() - 3.0);
				::SetTextColor(inContext, sShadow3Color->WIN_ToCOLORREF());
				::MoveToEx(inContext, inStartPoint.GetX() + 2.0, inStartPoint.GetY() + 3.0, 0);
				::LineTo(inContext, inEndPoint.GetX() + 2.0, inEndPoint.GetY() - 5.0);
			}
		}
	}
	else if (vDelta == 0)
	{
		// Horizontal only
		if (isTopShadow)
		{
			::MoveToEx(inContext, inStartPoint.GetX(), inStartPoint.GetY() + 1.0, 0);
			::LineTo(inContext, inEndPoint.GetX(), inEndPoint.GetY() - 1.0);
			if (isLargeHShadow)
			{
				::SetTextColor(inContext, sShadow2Color->WIN_ToCOLORREF());
				::MoveToEx(inContext, inStartPoint.GetX() + 2.0, inStartPoint.GetY() + 1.0, 0);
				::LineTo(inContext, inEndPoint.GetX() - 3.0, inEndPoint.GetY() + 1.0);
				::SetTextColor(inContext, sShadow3Color->WIN_ToCOLORREF());
				::MoveToEx(inContext, inStartPoint.GetX() + 3.0, inStartPoint.GetY() + 2.0, 0);
				::LineTo(inContext, inEndPoint.GetX() - 5.0, inEndPoint.GetY() + 2.0);
			}
		}
		else if (isBottomShadow)
		{
			::MoveToEx(inContext, inStartPoint.GetX() + 1.0, inStartPoint.GetY(), 0);
			::LineTo(inContext, inEndPoint.GetX() - 1.0, inEndPoint.GetY());
			if (isLargeHShadow)
			{
				::SetTextColor(inContext, sShadow2Color->WIN_ToCOLORREF());
				::MoveToEx(inContext, inStartPoint.GetX() + 2.0, inStartPoint.GetY() + 1.0, 0);
				::LineTo(inContext, inEndPoint.GetX() - 3.0, inEndPoint.GetY() + 1.0);
				::SetTextColor(inContext, sShadow3Color->WIN_ToCOLORREF());
				::MoveToEx(inContext, inStartPoint.GetX() + 3.0, inStartPoint.GetY() + 2.0, 0);
				::LineTo(inContext, inEndPoint.GetX() - 5.0, inEndPoint.GetY() + 2.0);
			}
		}
	}
	else
	{
		if (isTopShadow && isLeftShadow)
		{
			::MoveToEx(inContext, inStartPoint.GetX(), inStartPoint.GetY() + 1.0, 0);
			::LineTo(inContext, inEndPoint.GetX(), inEndPoint.GetY() - 1.0);
			if (isLargeHShadow || isLargeVShadow)
			{
				::SetTextColor(inContext, sShadow2Color->WIN_ToCOLORREF());
				::MoveToEx(inContext, inStartPoint.GetX() + 2.0, inStartPoint.GetY() + 1.0, 0);
				::LineTo(inContext, inEndPoint.GetX() - 3.0, inEndPoint.GetY() + 1.0);
				::SetTextColor(inContext, sShadow3Color->WIN_ToCOLORREF());
				::MoveToEx(inContext, inStartPoint.GetX() + 3.0, inStartPoint.GetY() + 2.0, 0);
				::LineTo(inContext, inEndPoint.GetX() - 5.0, inEndPoint.GetY() + 2.0);
			}
		}
		else if (isBottomShadow && isRightShadow)
		{
			::MoveToEx(inContext, inStartPoint.GetX() + 1.0, inStartPoint.GetY(), 0);
			::LineTo(inContext, inEndPoint.GetX() - 1.0, inEndPoint.GetY());
			if (isLargeHShadow || isLargeVShadow)
			{
				::SetTextColor(inContext, sShadow2Color->WIN_ToCOLORREF());
				::MoveToEx(inContext, inStartPoint.GetX() + 2.0, inStartPoint.GetY() + 1.0, 0);
				::LineTo(inContext, inEndPoint.GetX() - 3.0, inEndPoint.GetY() + 1.0);
				::SetTextColor(inContext, sShadow3Color->WIN_ToCOLORREF());
				::MoveToEx(inContext, inStartPoint.GetX() + 3.0, inStartPoint.GetY() + 2.0, 0);
				::LineTo(inContext, inEndPoint.GetX() - 5.0, inEndPoint.GetY() + 2.0);
			}
		}
	}
	
	::SetTextColor(inContext, oldColor);
	
}


void	VWinGDIGraphicContext::QDFrameRect (const VRect& inHwndRect)
{
	
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));
	
	VRect r=inHwndRect;
	if(fLineWidth>1 || fPrinterScale)
	{
		r.SetPosBy((fLineWidth/2.0),(fLineWidth/2.0));
		r.SetSizeBy(-fLineWidth,-fLineWidth);
	}
	else
		r.SetSizeBy(-fLineWidth,-fLineWidth);
	FrameRect(r);
}

void	VWinGDIGraphicContext::QDFrameRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));
	
	VRect r=inHwndRect;
	if(fLineWidth>1)
	{
		r.SetPosBy((fLineWidth/2.0),(fLineWidth/2.0));
		r.SetSizeBy(-fLineWidth,-fLineWidth);
	}
	else
	{
		r.SetPosBy(0.5,0.5);
		r.SetSizeBy(-fLineWidth,-fLineWidth);
	}
	FrameRoundRect(r,inOvalWidth,inOvalHeight);
}
void	VWinGDIGraphicContext::QDFrameOval (const VRect& inHwndRect)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));
	
	VRect r=inHwndRect;
	if(fLineWidth>1)
	{
		r.SetPosBy((fLineWidth/2.0),(fLineWidth/2.0));
		r.SetSizeBy(-fLineWidth,-fLineWidth);
	}
	else
		r.SetSizeBy(-fLineWidth,-fLineWidth);
	
	FrameOval(r);
}

void	VWinGDIGraphicContext::QDDrawRect (const VRect& inHwndRect)
{
	QDFillRect(inHwndRect);
	QDFrameRect(inHwndRect);
}

void	VWinGDIGraphicContext::QDDrawRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight)
{
	QDFillRoundRect(inHwndRect,inOvalWidth,inOvalHeight);
	QDFrameRoundRect(inHwndRect,inOvalWidth,inOvalHeight);
}

void	VWinGDIGraphicContext::QDDrawOval (const VRect& inHwndRect)
{
	QDFillOval(inHwndRect);
	QDFrameOval(inHwndRect);
}

void	VWinGDIGraphicContext::QDFillRect (const VRect& inHwndRect)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));
	
	FillRect (inHwndRect);
}
void	VWinGDIGraphicContext::QDFillRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));
	
	FillRoundRect(inHwndRect,inOvalWidth,inOvalHeight);
}
void	VWinGDIGraphicContext::QDFillOval (const VRect& inHwndRect)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));
	
	FillOval(inHwndRect);
}

void VWinGDIGraphicContext::_QDAdjustLineCoords(GReal* ioCoords)
{
	// When fPenSize = 1 we use PS_ENDCAP_ROUND which adds a pixel at the end
	// this is because with other ENDCAPS and fPenSize = 1 the first point is not always drawn
	if (fLineWidth > 1)
	{
		// GDI does not include the end point when drawing the line
		// so we adjust the end point's coordinates by adding fPenSize
		// to behave like macintosh
		/*
		coords[0] = start.GetX();
		coords[1] = start.GetY();
		coords[2] = end.GetX();
		coords[3] = end.GetY();
		*/
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

void VWinGDIGraphicContext::QDDrawLine (const VPoint& inHwndStart, const VPoint& inHwndEnd)
{
	QDDrawLine (inHwndStart.GetX(),inHwndStart.GetY(), inHwndEnd.GetX(),inHwndEnd.GetY());
}

void VWinGDIGraphicContext::QDDrawLine(const GReal inHwndStartX,const GReal inHwndStartY, const GReal inHwndEndX,const GReal inHwndEndY)
{
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));
	StUseContext_NoRetain	context(this);
	
	
	_StSelectObject	pen(fContext,::GetStockObject(NULL_BRUSH));
	_UpdateLineBrush();
	
	VPoint start(inHwndStartX,inHwndStartY),end(inHwndEndX,inHwndEndY);

	if (start.GetX() == end.GetX() ) 
	{
		if(fLineWidth==1)
		{
			if(fPrinterScale)
			{
				start.SetPosBy(fLineWidth / 2.0,0);
				end.SetPosBy(fLineWidth / 2.0,0);
				if(fHairline)
				{
					if (end.GetY() >= start.GetY())
					{
						start.SetPosBy(0,fLineWidth / 2.0);
						end.SetPosBy(0,-fLineWidth / 2.0);
					}
					else
					{
						start.SetPosBy(0,-fLineWidth / 2.0);
						end.SetPosBy(0,fLineWidth / 2.0);
					}
				}
			}
		}
		else
		{
			start.SetPosBy(fLineWidth / 2.0,0);
			end.SetPosBy(fLineWidth / 2.0,0);
		}
	}
    else if (start.GetY() == end.GetY()) 
	{
		if(fLineWidth==1)
		{
			if(fPrinterScale)
			{
				start.SetPosBy(0, fLineWidth/2.0);
				end.SetPosBy(0, fLineWidth/2.0);

				if(fHairline)
				{
					if (end.GetX() >= start.GetX())
					{
						start.SetPosBy(fLineWidth / 2.0,0);
						end.SetPosBy(-fLineWidth / 2.0,0);
					}
					else
					{
						start.SetPosBy(-fLineWidth / 2.0,0);
						end.SetPosBy(fLineWidth / 2.0,0);
					}
				}

			}
		}
		else
		{
			start.SetPosBy(0, fLineWidth/2.0);
			end.SetPosBy(0, fLineWidth/2.0);
		}
	}
	
	_LPtoDP(start);
	_LPtoDP(end);
	
	
	BOOL	result = ::MoveToEx(fContext, start.GetX(),start.GetY(), 0);
	result = ::LineTo(fContext,  end.GetX(), end.GetY());

	if (fDrawShadow)
		_DrawShadow(fContext, fShadowOffset, inHwndStartX, inHwndStartY,inHwndEndX,inHwndEndY);
	
	if (!result) vThrowNativeError(::GetLastError());
	
	
	
	#if 0
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));

	GReal HwndStartX=inHwndStartX;
	GReal HwndStartY=inHwndStartY; 
	GReal HwndEndX=inHwndEndX;
	GReal HwndEndY=inHwndEndY;

	if (HwndStartX == HwndEndX && fLineWidth > 1.0) 
	{
		HwndStartX += fLineWidth / 2.0;
		HwndEndX += fLineWidth / 2.0;
		HwndEndY += fLineWidth;
	}
    else if (HwndStartY == HwndEndY && fLineWidth > 1.0) 
	{
		HwndStartY += fLineWidth /2.0;
		HwndEndY += fLineWidth /2.0;
		HwndEndX += fLineWidth;
	}
	
	DrawLine(HwndStartX,HwndStartY, HwndEndX,HwndEndY);
	#endif
}

void	VWinGDIGraphicContext::QDDrawLines (const GReal* inCoords, sLONG inCoordCount)
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

void VWinGDIGraphicContext::QDMoveBrushTo (const VPoint& inHwndPos)
{
	MoveBrushTo (inHwndPos);
}

void VWinGDIGraphicContext::QDDrawLineTo (const VPoint& inHwndEnd)
{
	DrawLineTo (inHwndEnd);
}

#pragma mark-

VWinGDIOffscreenContext::VWinGDIOffscreenContext(HDC inSourceContext) : VWinGDIGraphicContext(inSourceContext)
{	
	_Init();

	fSrcContext = fContext;
	fContext = NULL;
}


VWinGDIOffscreenContext::VWinGDIOffscreenContext(const VWinGDIGraphicContext& inSourceContext) : VWinGDIGraphicContext(inSourceContext)
{
	_Init();

	fSrcContext = fContext;
	fContext = NULL;
}


VWinGDIOffscreenContext::VWinGDIOffscreenContext(const VWinGDIOffscreenContext& inOriginal) : VWinGDIGraphicContext(inOriginal)
{
	_Init();
	
	fSrcContext = inOriginal.fSrcContext;
	fContext = NULL;
}


VWinGDIOffscreenContext::~VWinGDIOffscreenContext()
{
	_Dispose();
}


void VWinGDIOffscreenContext::_Init()
{
	fSrcContext = NULL;
	fPicture = NULL;
}


void VWinGDIOffscreenContext::_Dispose()
{
	if (fContext != NULL && fPicture == NULL)
		fPicture = ::CloseEnhMetaFile(fContext);
	
	if (fPicture != NULL)
		::DeleteEnhMetaFile(fPicture);
}


void VWinGDIOffscreenContext::_ClosePicture()
{
	if (fContext != NULL && fPicture == NULL)
	{
		fPicture = ::CloseEnhMetaFile(fContext);
		fContext = NULL;
		fDpiX = fDpiY = 0.0f;
	}
}


void VWinGDIOffscreenContext::CopyContentTo( VGraphicContext* inDestContext, const VRect* inSrcBounds, const VRect* inDestBounds, TransferMode inMode, const VRegion* inMask)
{
	StUseContext	context(inDestContext);

	_ClosePicture();
	
	if (fPicture != NULL)
	{
		RECT	bounds;
		if (inDestBounds != NULL)
		{
			inDestBounds->ToRectRef(bounds);
		}
		else
		{
			SIZE	contextSize;
			BOOL	result = ::GetViewportExtEx(fSrcContext, &contextSize); 
			bounds.left = 0;
			bounds.top = 0;
			bounds.right = contextSize.cx;
			bounds.bottom = contextSize.cy;
		}

		BOOL	result = ::PlayEnhMetaFile(context, fPicture, &bounds);
	}
}


Boolean VWinGDIOffscreenContext::CreatePicture(const VRect& inHwndBounds)
{
	ReleasePicture();
	
	// Determine picture dimensions
	GReal	widthMM = ::GetDeviceCaps(fSrcContext, HORZSIZE); 
	GReal	heightMM = ::GetDeviceCaps(fSrcContext, VERTSIZE); 
	GReal	widthPels = ::GetDeviceCaps(fSrcContext, HORZRES); 
	GReal	heightPels = ::GetDeviceCaps(fSrcContext, VERTRES);
	 
	// Convert to .01-mm units
	RECT	bounds;
	bounds.left = (inHwndBounds.GetLeft() * widthMM * 100.0) / widthPels; 
	bounds.top = (inHwndBounds.GetTop() * heightMM * 100.0) / heightPels; 
	bounds.right = (inHwndBounds.GetRight() * widthMM * 100.0) / widthPels; 
	bounds.bottom = (inHwndBounds.GetBottom() * heightMM * 100.0) / heightPels;
	
	// Create EMF context
	fContext = ::CreateEnhMetaFile(fSrcContext, NULL, &bounds, "");
	fDpiX = fDpiY = 0.0f;
	if (fContext != NULL)
	{
		::SetGraphicsMode(fContext, GM_COMPATIBLE);
		::SetStretchBltMode(fContext, COLORONCOLOR);
		::SetMapMode(fContext, MM_TEXT);
	}
	
	return (fContext != NULL);
}


void VWinGDIOffscreenContext::ReleasePicture()
{
	_Dispose();
	
	fContext = NULL;
	fDpiX = fDpiY = 0.0f;
	fPicture = NULL;
}


HENHMETAFILE VWinGDIOffscreenContext::WIN_CreateEMFHandle ()
{
	_ClosePicture();
	return ::CopyEnhMetaFile(fPicture, "");
}


#pragma mark-

VWinGDIBitmapContext::VWinGDIBitmapContext(HDC inSourceContext) : VWinGDIGraphicContext(inSourceContext)
{	
	_Init();

	fSrcContext = fContext;
	fContext = NULL;
}


VWinGDIBitmapContext::VWinGDIBitmapContext(const VWinGDIGraphicContext& inSourceContext) : VWinGDIGraphicContext(inSourceContext)
{
	_Init();
	
	fSrcContext = fContext;
	fContext = NULL;
}


VWinGDIBitmapContext::VWinGDIBitmapContext(const VWinGDIBitmapContext& inOriginal) : VWinGDIGraphicContext(inOriginal)
{
	_Init();
	
	fSrcContext = inOriginal.fSrcContext;
	fContext = NULL;
}


VWinGDIBitmapContext::~VWinGDIBitmapContext()
{
	_Dispose();
}


void VWinGDIBitmapContext::_Init()
{
	fSrcContext = NULL;
	fBitmap = NULL;
	fSavedBitmap = NULL;
}


void VWinGDIBitmapContext::_Dispose()
{
	::SelectObject(fContext, fSavedBitmap);
	::DeleteObject(fBitmap);
	::DeleteDC(fContext);
}

// Inherited from VGraphicContext
void VWinGDIBitmapContext::CopyContentTo (VGraphicContext* inDestContext, const VRect* inSrcBounds, const VRect* inDestBounds, TransferMode inMode , const VRegion* inMask)
{

	VWinGDIGraphicContext::CopyContentTo (inDestContext,  inSrcBounds , inDestBounds ,  inMode , inMask );
}


VPictureData* VWinGDIBitmapContext::CopyContentToPictureData()
{
	VPictureData_GDIBitmap* result;
	SIZE s={0,0};
	HDC dstdc;
	HBITMAP dstbm,oldbm;
	
	if(fBitmap)
	{
		BITMAP bm;
		::GetObject(fBitmap,sizeof(BITMAP),&bm);
		
		dstdc=CreateCompatibleDC(fContext);
		dstbm=CreateCompatibleBitmap(fContext,bm.bmWidth,bm.bmHeight);
		oldbm=(HBITMAP)SelectObject(dstdc,dstbm);
		::BitBlt(dstdc,0,0,bm.bmWidth,bm.bmHeight,fContext,0,0,SRCCOPY);
		::SelectObject(dstdc,oldbm);
		::DeleteDC(dstdc);
		
		result=new VPictureData_GDIBitmap(dstbm);
	}
	return result;
}


Boolean VWinGDIBitmapContext::CreateBitmap(const VRect& inHwndBounds)
{
	if (fContext == NULL)
	{
		fContext = ::CreateCompatibleDC(fSrcContext);
		::SetGraphicsMode(fContext, GM_COMPATIBLE);
		::SetStretchBltMode(fContext, COLORONCOLOR);
		::SetMapMode(fContext, MM_TEXT);
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
	#if USE_DOUBLE_BUFFER
		sLONG	depth = ::GetDeviceCaps(fContext, BITSPIXEL);
		sLONG	nbColors = depth == 8 ? 256 : 0;
		
		BITMAPINFO*	info = (BITMAPINFO*) gMemory->NewPtr(sizeof(BITMAPINFO) + nbColors + sizeof(RGBQUAD));
		info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		info->bmiHeader.biWidth = inHwndBounds.GetWidth();
		info->bmiHeader.biHeight = -inHwndBounds.GetHeight();
		info->bmiHeader.biPlanes = 1;
		info->bmiHeader.biBitCount = depth;
		info->bmiHeader.biCompression = BI_RGB;
		info->bmiHeader.biSizeImage = 0;
		info->bmiHeader.biXPelsPerMeter = 2800;
		info->bmiHeader.biYPelsPerMeter = 2800;
		info->bmiHeader.biClrUsed = nbColors;
		info->bmiHeader.biClrImportant = 0;

		void*	fBitmapPixels = NULL;
		
		fBitmap = ::CreateDIBSection(fContext, info, DIB_RGB_COLORS, &fBitmapPixels, 0, 0);
		gMemory->DisposePtr((VPtr) info);
	#else
		fBitmap = ::CreateCompatibleBitmap(fSrcContext ? fSrcContext : fContext, inHwndBounds.GetWidth(), inHwndBounds.GetHeight());
	#endif
	
		if (fBitmap != NULL)
		{
			fSavedBitmap = (HBITMAP) ::SelectObject(fContext, fBitmap);
			
			// Erase backgrounds
			::SetDCBrushColor(fContext,RGB(255,0,0));
			::Rectangle(fContext, 0, 0, inHwndBounds.GetWidth(), inHwndBounds.GetHeight());
		}
		else
		{
			::DeleteDC(fContext);
			fContext = NULL;
			fDpiX = fDpiY = 0.0f;
		}
	}

	return (fContext != NULL);
}


void VWinGDIBitmapContext::ReleaseBitmap()
{
	if (fContext != NULL)
	{
		_Dispose();
		
		fContext = NULL;
		fDpiX = fDpiY = 0.0f;
		fSavedBitmap = NULL;
		fBitmap = NULL;
	}
}


HENHMETAFILE VWinGDIBitmapContext::WIN_CreateEMFHandle() const
{
	HENHMETAFILE	emfHandle = NULL;
	
	// Get bitmap size
	SIZE	contextSize;
	BOOL	result = ::GetViewportExtEx(fContext, &contextSize); 
	
	// Create EMF context
	RECT	bounds = { 0, 0, contextSize.cx, contextSize.cy };
	HDC	emfDC = ::CreateEnhMetaFile(fContext, NULL, &bounds, "");
	if (testAssert(emfDC != NULL))
	{
		// Set attributes
		::SetGraphicsMode(emfDC, GM_COMPATIBLE);
		::SetStretchBltMode(emfDC, COLORONCOLOR);
		::SetMapMode(emfDC, MM_TEXT);
		
		// Bit source context
		::BitBlt(emfDC, 0, 0, contextSize.cx, contextSize.cy, fContext, 0, 0, SRCCOPY);

		// Close image
		emfHandle = ::CloseEnhMetaFile(emfDC);
	}
	
	return emfHandle;
}


StGDIResetTransform::StGDIResetTransform(HDC hdc)
{
	fHDC = hdc;
	fGM = ::GetGraphicsMode( fHDC);
	if (fGM == GM_ADVANCED)
	{
		::GetWorldTransform(fHDC, &fCTM);
		::ModifyWorldTransform(fHDC, NULL, MWT_IDENTITY);
	}
}

StGDIResetTransform::~StGDIResetTransform()
{
	if (::GetGraphicsMode(fHDC) != fGM)
		::SetGraphicsMode(fHDC, fGM);
	if (fGM == GM_ADVANCED)
		::SetWorldTransform(fHDC, &fCTM);
}

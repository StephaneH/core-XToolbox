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
#include "XMacQuartzGraphicContext.h"
#include "XMacStyledTextBox.h"
#include "VRect.h"
#include "VRegion.h"
#include "VIcon.h"
#include "VPolygon.h"
#include "VPattern.h"
#include "VBezier.h"
#include "VGraphicImage.h"
#include "VGraphicFilter.h"
#include "VFont.h"
#include "XMacFont.h"

#define DEBUG_DRAWTEXTBOX_BOUNDS	0	// Set to "1" to show bounding frame of text
#define USE_CG_TEXTBOX				1	// Set to "1" to use CGContext with TXNDrawTextBox
#define USE_SELECT_FONT_API			1	// Set to "1" to use SelectFont/ShowText instead of SetFont/ShowGlyph
#define USE_ATSU_GET_BOUNDS_API		0	// Set to "1" to use ATSUGetUnjustifiedBounds


const RGBColor	kWHITE_COLOR	= { 0xFFFF, 0xFFFF, 0xFFFF };
const RGBColor	kBLACK_COLOR	= { 0x0000, 0x0000, 0x0000 };

const uLONG	kDEBUG_REVEAL_CLIPPING_DELAY	= 4;	// ticks
const uLONG	kDEBUG_REVEAL_BLITTING_DELAY	= 1;	// ticks
const uLONG	kDEBUG_REVEAL_INVAL_DELAY		= 1;	// ticks

const CGFloat	kPATTERN_DEFAULT_SIZE		= 16;
const uLONG	kMAX_LINE_STYLE_ELEMENTS	= 8;

const GReal	kSMALLEST_TXN_WIDTH		= 8;	// This value is in fact the size of the larger

/** get Core Text factory instance */
#define GetCoreTextFactory() GetFontMetrics()->GetImpl().GetTextFactory()


// Static

Boolean VMacQuartzGraphicContext::Init()
{
	return true;
}


void VMacQuartzGraphicContext::DeInit()
{
}


#pragma mark-

VMacQuartzGraphicContext::VMacQuartzGraphicContext():VGraphicContext()
{
	_Init();
}


VMacQuartzGraphicContext::VMacQuartzGraphicContext(CGContextRef inPrinterContext, const PMPageFormat inPageFormat)
:VGraphicContext()
{
	assert(inPrinterContext != NULL);
	
	_Init();
	
	fContext = ::CGContextRetain(inPrinterContext);
	fAxisInverted = false;
	
	PMRect aPaperRect,aPageRect;
	
	PMGetAdjustedPaperRect(inPageFormat, &aPaperRect);
	PMGetAdjustedPageRect(inPageFormat, &aPageRect);
	
	fPrinterAdjustedPaperRect = CGRectMake(aPaperRect.left,aPaperRect.top,aPaperRect.right-aPaperRect.left,aPaperRect.bottom-aPaperRect.top);
	fPrinterAdjustedPageRect = CGRectMake(aPageRect.left,aPageRect.top,aPageRect.right-aPageRect.left,aPageRect.bottom-aPageRect.top);
	fQDBounds=fPrinterAdjustedPageRect;
#if VERSIONDEBUG
	CGAffineTransform ctm = ::CGContextGetCTM( fContext);
#endif


	CGContextTranslateCTM(fContext, -fPrinterAdjustedPaperRect.origin.x ,(fPrinterAdjustedPaperRect.size.height + fPrinterAdjustedPaperRect.origin.y ) - fPrinterAdjustedPageRect.size.height );

	SetPrinterScale(true,1,1);

	CGContextSetFlatness(fContext, 2);
	CGContextSetInterpolationQuality(fContext, kCGInterpolationLow);
    CGContextSetAllowsAntialiasing( fContext, true);
    CGContextSetShouldAntialias(fContext, true);
    CGContextSetAllowsFontSmoothing( fContext, true);
    CGContextSetShouldSmoothFonts( fContext, true);
	SetLineWidth((GReal)1.0);
	
	SaveContext();
/*
_UseQDAxis();
{
	CGRect re=CGRectMake(0,0,200,200);;
	CGContextAddRect(inPrinterContext,re);
	
	CGColorSpaceRef	colorSpace = ::CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
	CGFloat componentsColor[4] = { 1,0,0,1};
	CGColorRef colorShadow = ::CGColorCreate( colorSpace, componentsColor);
	::CGContextSetStrokeColorWithColor(inPrinterContext,colorShadow);
	CGContextStrokePath(inPrinterContext);
	::CGColorRelease( colorShadow);
	::CGColorSpaceRelease(colorSpace);
}
_UseQuartzAxis();
{
	CGRect re=CGRectMake(0,0,200,200);;
	CGContextAddRect(inPrinterContext,re);
	
	CGColorSpaceRef	colorSpace = ::CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
	CGFloat componentsColor[4] = { 1,1,0,1};
	CGColorRef colorShadow = ::CGColorCreate( colorSpace, componentsColor);
	::CGContextSetStrokeColorWithColor(inPrinterContext,colorShadow);
	CGContextStrokePath(inPrinterContext);
	::CGColorRelease( colorShadow);
	::CGColorSpaceRelease(colorSpace);
}*/
}
VMacQuartzGraphicContext::VMacQuartzGraphicContext(CGContextRef inContext, Boolean inReversed, const CGRect& inQDBounds ,bool inForPrinting):VGraphicContext()
{
	assert(inContext != NULL);
	
	_Init();
	
	fContext = ::CGContextRetain(inContext);
	fAxisInverted = inReversed;
	fQDBounds = inQDBounds;
	fPrinterAdjustedPaperRect = fQDBounds;
	fPrinterAdjustedPageRect = fQDBounds;
	
#if VERSIONDEBUG
	CGAffineTransform ctm = ::CGContextGetCTM( fContext);
#endif

	SetPrinterScale(inForPrinting,1,1);

	CGContextSetFlatness(fContext, 2);
	CGContextSetInterpolationQuality(fContext, kCGInterpolationLow);
    CGContextSetAllowsAntialiasing( fContext, true);
    CGContextSetShouldAntialias(fContext, true);
    CGContextSetAllowsFontSmoothing( fContext, true);
    CGContextSetShouldSmoothFonts( fContext, true);
	SetLineWidth((GReal)1.0);
	SaveContext();
}


VMacQuartzGraphicContext::VMacQuartzGraphicContext( VMacQDPortBridge* inQDPort, const CGRect& inQDBounds,bool inForPrinting):VGraphicContext()
{
	_Init();
	
	_InitFromQDBridge(inQDPort,inQDBounds,inForPrinting);
}

void VMacQuartzGraphicContext::_InitFromQDBridge(VMacQDPortBridge* inQDPort, const CGRect& inQDBounds,bool inForPrinting)
{
	assert(inQDPort != NULL);
	CGRect outQDBounds;

	fQDPort = inQDPort;
	fQDBounds = inQDBounds;
	fPrinterAdjustedPaperRect = fQDBounds;
	fPrinterAdjustedPageRect = fQDBounds;
	
	inQDPort->BeginCGContext(fContext,outQDBounds);
	SaveContext();
	// move to desired origin
	// 0,0 in cgcontext is bottom,left in grafport.
	// but we need it to be the bottom,left of passed qd bounds.
	GReal offsetY = (outQDBounds.size.height) - (inQDBounds.origin.y + inQDBounds.size.height);
	::CGContextTranslateCTM( fContext, inQDBounds.origin.x, (float) offsetY);
	
	fQDBounds.origin.y = inQDBounds.origin.y+offsetY;
	fQDBounds.origin.x = inQDBounds.origin.x;
	
	CGContextSetFlatness(fContext, 2);
	CGContextSetInterpolationQuality(fContext, kCGInterpolationLow);
    CGContextSetAllowsAntialiasing( fContext, true);
    CGContextSetShouldAntialias(fContext, true);
    CGContextSetAllowsFontSmoothing( fContext, true);
    CGContextSetShouldSmoothFonts( fContext, true);
	SetLineWidth((GReal)1.0);
	
	SetPrinterScale(inForPrinting,1,1);
}


VMacQuartzGraphicContext::VMacQuartzGraphicContext(const VMacQuartzGraphicContext& inOriginal) : VGraphicContext(inOriginal)
{
	_Init();
	
	// Don't copy QD port to force a simple release of the context
	fContext = inOriginal.fContext;
	fTextAngle = inOriginal.fTextAngle;
	fQDBounds = inOriginal.fQDBounds;
	fShadowBlurness = inOriginal.fShadowBlurness;
	fShadowOffset = inOriginal.fShadowOffset;
	fFillPattern = inOriginal.fFillPattern;
	fLinePattern = inOriginal.fLinePattern;
	fAxisInverted = inOriginal.fAxisInverted;
	fAlphaBlend = inOriginal.fAlphaBlend;
	fTextRenderingMode = inOriginal.fTextRenderingMode;
	fTextLayoutMode = inOriginal.fTextLayoutMode;
	fTextColor = inOriginal.fTextColor;
	
	
	
	if (fFillPattern != NULL)
		fFillPattern->Retain();
		
	if (fLinePattern != NULL)
		fLinePattern->Retain();
	
	::CGContextRetain(fContext);
	
	CGContextSetFlatness(fContext, 2);
	CGContextSetInterpolationQuality(fContext, kCGInterpolationLow);
    CGContextSetAllowsAntialiasing( fContext, true);
    CGContextSetShouldAntialias(fContext, true);
    CGContextSetAllowsFontSmoothing( fContext, true);
    CGContextSetShouldSmoothFonts( fContext, true);
	SetLineWidth(inOriginal.fLineWidth);
	
	SaveContext();
}


VMacQuartzGraphicContext::~VMacQuartzGraphicContext()
{	
	if(fContext)
		RestoreContext();
	
	//clear all layers
	ClearLayers();
	
	ReleaseRefCountable( &fFillPattern);
	ReleaseRefCountable( &fLinePattern);
	ReleaseRefCountable( &fFont);

	delete fFontMetrics;
	
	if(fAxisInvertedStack)
	{
		assert(fAxisInvertedStack->empty());
		delete fAxisInvertedStack;
	}
	
	if (fQDPort != NULL)
	{
		delete fQDPort;
	}
	else
	{
		if(fContext)
			::CGContextRelease(fContext);
	}
}


void VMacQuartzGraphicContext::_Init()
{
	fSaveRestoreCount = 0;
	fQDContextSavedClip=NULL;
	fContext = NULL;
	fQDPort = NULL;
	fAlphaBlend = 0;
	fTextAngle = 0;
	fShadowBlurness = (GReal) 0.8;
	fShadowOffset.width = 2;
	fShadowOffset.height = -3;
	fQDBounds.origin.x = fQDBounds.origin.y = 0;
	fQDBounds.size.width = fQDBounds.size.height = 0;
	fFillPattern = NULL;
	fLinePattern = NULL;
	fAxisInverted = false;
	fAxisInvertedStack	= NULL;
	fLineWidth= (GReal) 1.0;
	fGlobalAlpha = 1.0f;

	fTextColor = VColor(0,0,0,255);
	fLineColor = fTextColor;
	fFillColor = fTextColor;

	fTextRenderingMode = fHighQualityTextRenderingMode = TRM_NORMAL;
	fTextLayoutMode	= TLM_NORMAL;
	fFont = NULL;
	fFontMetrics = NULL;

	fFillRule = FILLRULE_EVENODD; //init to alternate (compliant with default for all impl)
	fUseLineDash=false;

	fDashPatternOffset = 0;
	fIsStrokeDashPatternUnitEqualToStrokeWidth = false;
	
	fPrinterAdjustedPageRect=CGRectMake(0,0,0,0);
	fPrinterAdjustedPaperRect=CGRectMake(0,0,0,0);
}


bool VMacQuartzGraphicContext::BeginQDContext()
{
	bool result=false;
	if(fQDPort)
	{
		fQDContextSavedClip = new VRegion();
        GetClip(*fQDContextSavedClip);
		RestoreContext();
		
		assert(fSaveRestoreCount==0);

		fQDPort->EndCGContext();
		fContext=NULL;
        fQDPort->SetPort();
        fQDPort->ClipQDPort(*fQDContextSavedClip);
        result=true;
	}
	return result;
}

void VMacQuartzGraphicContext::EndQDContext(bool inDontRestoreClip)
{
	if(fQDPort)
	{
		VAffineTransform trans;
        CGRect outQDBounds;
		fQDPort->BeginCGContext(fContext,outQDBounds);
		
        SaveContext();

		// move to desired origin
		// 0,0 in cgcontext is bottom,left in grafport.
		// but we need it to be the bottom,left of passed qd bounds.
		GReal offsetY = (outQDBounds.size.height) - (fQDBounds.origin.y + fQDBounds.size.height);
		::CGContextTranslateCTM( fContext, fQDBounds.origin.x, (float) offsetY);
		
        if(fAxisInverted)
		{
			fAxisInverted=false;
			_UseQDAxis();
		}
		if (!inDontRestoreClip)
		{
			ClipRegion(*fQDContextSavedClip, false, false);
		}
		else
		{
			VRect clip(fQDBounds.origin.x, fQDBounds.origin.y, fQDBounds.size.width, fQDBounds.size.height);
			ClipRect(clip, false, false);
		}
        delete fQDContextSavedClip;
        fQDContextSavedClip=NULL;
        
		CGContextSetFlatness(fContext, 2);
		CGContextSetInterpolationQuality(fContext, kCGInterpolationLow);
		CGContextSetAllowsAntialiasing( fContext, true);
		CGContextSetShouldAntialias(fContext, true);
		CGContextSetAllowsFontSmoothing( fContext, true);
		CGContextSetShouldSmoothFonts( fContext, true);
	}
}

void VMacQuartzGraphicContext::UseThisQDPortBridge(VMacQDPortBridge* inNewBridge,const CGRect& inQDBounds,bool inForPrinting)
{
	if( fQDPort)
	{
		if(fContext)
        {
            RestoreContext();
		
            assert(fSaveRestoreCount==0);

            fQDPort->EndCGContext();
            fContext=NULL;
        }

		if(inNewBridge!=NULL)
		{
			delete fQDPort;
			fQDPort = NULL;

			_InitFromQDBridge(inNewBridge,inQDBounds,inForPrinting);
            
            if(fAxisInverted)
            {
                fAxisInverted=false;
                _UseQDAxis();
            }
		}
	}
}

void VMacQuartzGraphicContext::UpdatePrintingContext(CGContextRef inNewCGContext)
{
	
	assert(fPrinterScale);
	if(fContext)
	{
		RestoreContext();
		
        assert(fSaveRestoreCount==0);

		::CGContextRelease(fContext);

		fContext=NULL;
	}

	if(inNewCGContext!=NULL)
	{
        fContext = inNewCGContext;
		
		::CGContextRetain(fContext);
		
		CGContextSetFlatness(fContext, 2);
		CGContextSetInterpolationQuality(fContext, kCGInterpolationLow);
		CGContextSetAllowsAntialiasing( fContext, true);
		CGContextSetShouldAntialias(fContext, true);
		CGContextSetAllowsFontSmoothing( fContext, true);
		CGContextSetShouldSmoothFonts( fContext, true);
		SetLineWidth((GReal)1.0);
	
		SetPrinterScale(true,1,1);
		
		if(fAxisInverted)
        {
			fAxisInverted=false;
			_UseQDAxis();
		}
		SaveContext();
	}
	
}

#if VERSIONMAC	
void VMacQuartzGraphicContext::GetParentPortBounds( VRect& outBounds)
{
	outBounds.SetCoords(fQDBounds.origin.x, fQDBounds.origin.y, fQDBounds.size.width, fQDBounds.size.height);
}
#endif

void VMacQuartzGraphicContext::GetTransform(VAffineTransform &outTransform)
{
	CGAffineTransform mat;
	mat=CGContextGetCTM(fContext); 
	outTransform.FromNativeTransform(mat);
}

/** return current graphic context transformation matrix
	in VGraphicContext normalized coordinate space
	(ie coordinate space with y axis pointing down - compliant with gdiplus and quickdraw coordinate spaces)
 */
void VMacQuartzGraphicContext::GetTransformNormalized(VAffineTransform &outTransform) 
{
	_UseQDAxis();
	GetTransform( outTransform);
}

/** set current graphic context transformation matrix
	in VGraphicContext normalized coordinate space
	(ie coordinate space with y axis pointing down - compliant with gdiplus and quickdraw coordinate spaces)
 */
void VMacQuartzGraphicContext::SetTransformNormalized(const VAffineTransform &inTransform) 
{ 
	SetTransform( inTransform); 
	fAxisInverted = true;
}

void VMacQuartzGraphicContext::SetTransform(const VAffineTransform &inTransform)
{
	CGAffineTransform mat;
	inTransform.ToNativeMatrix(mat);
	bool axisInverted = fAxisInverted;
	ResetTransform();
	fAxisInverted = axisInverted;
	CGContextConcatCTM(fContext,mat);
}
void VMacQuartzGraphicContext::ConcatTransform(const VAffineTransform &inTransform)
{
	CGAffineTransform mat;
	inTransform.ToNativeMatrix(mat);
	CGContextConcatCTM(fContext,mat);
}
void VMacQuartzGraphicContext::RevertTransform(const VAffineTransform &inTransform)
{
	VAffineTransform inv=inTransform;
	inv.Inverse();
	ConcatTransform(inv);
}
void VMacQuartzGraphicContext::ResetTransform()
{
	CGAffineTransform mat;
	mat=CGContextGetCTM(fContext); 
	mat=CGAffineTransformInvert(mat);
	CGContextConcatCTM(fContext,mat);
	//as transform is set to identity we reset axis inverted status (default is Quartz2D space)
	fAxisInverted = false;
}


#pragma mark-


void VMacQuartzGraphicContext::BeginUsingContext (bool /*inNoDraw*/)
{
	if (fQDPort != NULL)
		fQDPort->SetPort();

	if (fFont == NULL)
	{
		//ensure font is not NULL for text methods
		fFont = VFont::RetainStdFont( STDF_TEXT);
		if (fFontMetrics)
			delete fFontMetrics;
		fFontMetrics = NULL;
	}
}


void VMacQuartzGraphicContext::SetQDBounds( const CGRect& inQDBounds)
{
	_UseQuartzAxis();
	fQDBounds = inQDBounds;
}


void VMacQuartzGraphicContext::SetFillColor(const VColor& inColor, VBrushFlags* ioFlags)
{
	fFillColor = inColor;
	::CGContextSetRGBFillColor(fContext, inColor.GetRed() / (GReal) 255.0, inColor.GetGreen() / (GReal) 255.0, inColor.GetBlue() / (GReal) 255.0, inColor.GetAlpha() / (GReal) 255.0);
	ReleaseRefCountable(&fFillPattern);
	
	if (ioFlags != NULL)
		ioFlags->fFillBrushChanged = true;

	if (fUseShapeBrushForText)
	{
		if (fFontMetrics)
			fFontMetrics->GetImpl().GetTextFactory()->SetDirty();
		
		if (ioFlags)
			ioFlags->fTextBrushChanged = true;
	}
}


void VMacQuartzGraphicContext::SetFillPattern(const VPattern* inPattern, VBrushFlags* ioFlags)
{
	// Keep a copy. Empty and line patterns are skipped.
	if (inPattern != NULL && !inPattern->IsPlain() && inPattern->IsFillPattern())
		CopyRefCountable(&fFillPattern, (VPattern *)inPattern);
	else
		ReleaseRefCountable(&fFillPattern);

	if (ioFlags != NULL)
		ioFlags->fFillBrushChanged = true;

	if (fUseShapeBrushForText)
	{
		if (ioFlags)
			ioFlags->fTextBrushChanged = true;
	}
}


void VMacQuartzGraphicContext::SetLineColor(const VColor& inColor, VBrushFlags* ioFlags)
{
	fLineColor = inColor;
	::CGContextSetRGBStrokeColor(fContext, inColor.GetRed() / (GReal) 255.0, inColor.GetGreen() / (GReal) 255.0, inColor.GetBlue() / (GReal) 255.0, inColor.GetAlpha() / (GReal) 255.0);
	if (fLinePattern && ((fLinePattern->GetKind() == 'axeP') || (fLinePattern->GetKind() == 'radP')))
		//reset gradient
		ReleaseRefCountable(&fLinePattern);

	if (ioFlags != NULL)
		ioFlags->fLineBrushChanged = true;
}



void VMacQuartzGraphicContext::SetLinePattern(const VPattern* inPattern, VBrushFlags* ioFlags)
{
	//gradient can be applied with dash pattern so please do not reset dash pattern if gradient is applied (used for instance by SVG component)

	if (inPattern && ((inPattern->GetKind() == 'axeP')||(inPattern->GetKind() == 'radP')))
	{
		CopyRefCountable(&fLinePattern, (VPattern *)inPattern);
		if (ioFlags != NULL)
			ioFlags->fLineBrushChanged = true;
		return;
	}

	//for any other line pattern, we need to reset dash pattern

	if (fUseLineDash)
	{
		fUseLineDash=false;
		fDashPatternOffset = 0;
		fDashPattern.clear();
		::CGContextSetLineDash( fContext, 0, NULL, 0);
	}

	// Keep a copy. Empty and fill patterns are skipped.
	Boolean	validPattern = inPattern != NULL && !inPattern->IsPlain() && inPattern->IsLinePattern();
	if (validPattern)
	{
		CopyRefCountable(&fLinePattern, (VPattern *)inPattern);
        fUseLineDash = true;
	}	else
		ReleaseRefCountable(&fLinePattern);
	if (ioFlags != NULL)
		ioFlags->fLineBrushChanged = true;
}

/** force dash pattern unit to be equal to stroke width (on default it is equal to user unit) */
void VMacQuartzGraphicContext::ShouldLineDashPatternUnitEqualToLineWidth( bool inValue)
{
	if (fIsStrokeDashPatternUnitEqualToStrokeWidth == inValue)
		return;

	fIsStrokeDashPatternUnitEqualToStrokeWidth = inValue;
	if (fUseLineDash && fDashPattern.size())
		//for custom dash pattern, we need to rebuild dash pattern
		SetLineDashPattern( fDashPatternOffset, fDashPattern);
}

/** set line dash pattern
@param inDashOffset
	offset into the dash pattern to start the line (user units)
@param inDashPattern
	dash pattern (alternative paint and unpaint lengths in user units)
	for instance {3,2,4} will paint line on 3 user units, unpaint on 2 user units
						 and paint again on 4 user units and so on...
*/
void VMacQuartzGraphicContext::SetLineDashPattern(GReal inDashOffset, const VLineDashPattern& inDashPattern, VBrushFlags* ioFlags)
{
	if (inDashPattern.size() < 2)
	{
		//clear dash pattern
		if (fUseLineDash)
		{
			::CGContextSetLineDash( fContext, 0, NULL, 0);
			fDashPatternOffset = 0;
			fDashPattern.clear();
			fUseLineDash=false;
		}
	}
	else
	{
		//build dash pattern

		fDashPatternOffset = inDashOffset;
		fDashPattern = inDashPattern;

		CGFloat *pattern = new CGFloat[inDashPattern.size()];
		VLineDashPattern::const_iterator it = inDashPattern.begin();
		CGFloat *patternValue = pattern;
		if (fIsStrokeDashPatternUnitEqualToStrokeWidth)
		{
			//we need to scale with stroke width as CGContextSetLineDash pattern unit is user unit
			for (;it != inDashPattern.end(); it++, patternValue++)
				*patternValue = (*it)*fLineWidth;
		}
		else
			//CGContextSetLineDash pattern unit = user unit so just copy the pattern value
			for (;it != inDashPattern.end(); it++, patternValue++)
				*patternValue = (*it);

		//set dash pattern
		::CGContextSetLineDash( fContext, inDashOffset, pattern, (size_t)inDashPattern.size());		

		delete [] pattern;
		
		fUseLineDash=true;
	}
	if (ioFlags != NULL)
		ioFlags->fLineBrushChanged = true;
}

/** set fill rule */
void VMacQuartzGraphicContext::SetFillRule( FillRuleType inFillRule)
{
	fFillRule = inFillRule;
}


void VMacQuartzGraphicContext::SetLineWidth(GReal inWidth, VBrushFlags* /*ioFlags*/)
{
	if(fHairline && fPrinterScale)
		::CGContextSetLineWidth(fContext, 0.01f); // pas trouv d'autre moyen pour faire du hairline
	else
		::CGContextSetLineWidth(fContext, inWidth);
	fLineWidth=inWidth;
    
	if (fUseLineDash && fDashPattern.size() && fIsStrokeDashPatternUnitEqualToStrokeWidth)
		//for custom dash pattern, we need to rebuild dash pattern
		SetLineDashPattern( fDashPatternOffset, fDashPattern);
}



void VMacQuartzGraphicContext::SetLineCap (CapStyle inCapStyle, VBrushFlags* ioFlags)
{
	switch (inCapStyle)
	{
		case CS_BUTT:
			::CGContextSetLineCap(fContext, kCGLineCapButt);
			break;
			
		case CS_ROUND:
			::CGContextSetLineCap(fContext, kCGLineCapRound);
			break;
			
		case CS_SQUARE:
			::CGContextSetLineCap(fContext, kCGLineCapSquare);
			break;
		
		default:
			assert(false);
			break;
	}
}

void VMacQuartzGraphicContext::SetLineJoin(JoinStyle inJoinStyle, VBrushFlags* ioFlags)
{
	switch (inJoinStyle)
	{
		case JS_MITER:
			::CGContextSetLineJoin(fContext, kCGLineJoinMiter);
			break;
			
		case JS_ROUND:
			::CGContextSetLineJoin(fContext, kCGLineJoinRound);
			break;
			
		case JS_BEVEL:
			::CGContextSetLineJoin(fContext, kCGLineJoinBevel);
			break;
		
		default:
			assert(false);
			break;
	}
}

void VMacQuartzGraphicContext::SetLineMiterLimit( GReal inMiterLimit)
{
	::CGContextSetMiterLimit( fContext, inMiterLimit); 	
}


void VMacQuartzGraphicContext::SetLineStyle(CapStyle inCapStyle, JoinStyle inJoinStyle, VBrushFlags* /*ioFlags*/)
{
	SetLineCap( inCapStyle);
	SetLineJoin( inJoinStyle);
}


void VMacQuartzGraphicContext::SetTextColor(const VColor& inColor, VBrushFlags* ioFlags)
{
	if (fUseShapeBrushForText)
	{
		SetFillColor( inColor, ioFlags);
		return;
	}

	fTextColor = inColor;
	if (fFontMetrics)
		fFontMetrics->GetImpl().GetTextFactory()->SetDirty();
	
	if (ioFlags)
		ioFlags->fTextBrushChanged = true;
}

void VMacQuartzGraphicContext::GetTextColor (VColor& outColor) const
{
	if (fUseShapeBrushForText)
		outColor = fFillColor;
	else
		outColor = fTextColor;
}

void VMacQuartzGraphicContext::SetBackColor(const VColor& /*inColor*/, VBrushFlags* /*ioFlags*/)
{
	// No meaning here
}


DrawingMode VMacQuartzGraphicContext::SetDrawingMode(DrawingMode inMode, VBrushFlags* /*ioFlags*/)
{	
	if (GetMaxPerfFlag())
		inMode = DM_NORMAL;
	switch (inMode)
	{
		case DM_NORMAL:
			::CGContextSetShadowWithColor(fContext, fShadowOffset, 0, NULL);
			break;
			
		case DM_HILITE:
			// Not supported
			break;
			
		case DM_SHADOW:
			::CGContextSetShadow(fContext, fShadowOffset, fShadowBlurness);	// Black color, 1/3 alpha
			break;
		
		default:
			assert(false);
			break;
	}
	
	return DM_NORMAL;
}


void VMacQuartzGraphicContext::GetBrushPos(VPoint& outHwndPos) const
{
	_UseQDAxis();
	CGPoint	curPos = ::CGContextGetPathCurrentPoint(fContext);
	outHwndPos.SetPosTo(curPos.x, curPos.y);
}


GReal VMacQuartzGraphicContext::SetTransparency(GReal inAlpha)
{
	GReal result = fGlobalAlpha;
	fGlobalAlpha = inAlpha;
	::CGContextSetAlpha(fContext, inAlpha);
	return result;
}


void VMacQuartzGraphicContext::SetShadowValue(GReal inHOffset, GReal inVOffset, GReal inBlurness)
{
	fShadowOffset.width = inHOffset;
	fShadowOffset.height = -inVOffset;
	fShadowBlurness = inBlurness;
}


void VMacQuartzGraphicContext::EnableAntiAliasing()
{
	fIsHighQualityAntialiased = true;
	if (GetMaxPerfFlag())
		::CGContextSetShouldAntialias(fContext, false);
	else
		::CGContextSetShouldAntialias(fContext, true);
}


void VMacQuartzGraphicContext::DisableAntiAliasing()
{
	fIsHighQualityAntialiased = false;
	::CGContextSetShouldAntialias(fContext, false);
}


bool VMacQuartzGraphicContext::IsAntiAliasingAvailable()
{
	return true;
}



void VMacQuartzGraphicContext::SetFont( VFont *inFont, GReal inScale)
{
	if (fFont != inFont || inScale != 1.0f)
	{
		if (inScale == 1)
			CopyRefCountable( &fFont, inFont);
		else
		{
			VString fontName	= inFont->GetName() ;
			VFontFace fontface	= inFont->GetFace();
			GReal fontSize		= inFont->GetSize()*inScale;
			
			ReleaseRefCountable(&fFont);
			//JQ 23/02/2011: optimization...
			//fFont = new VFont(fontName,fontface,fontSize);
			fFont = VFont::RetainFont( fontName,fontface,fontSize);
		}

		if (fFontMetrics)
			delete fFontMetrics;
		fFontMetrics = NULL;
	}
}

VFont*	VMacQuartzGraphicContext::RetainFont()
{
	return RetainRefCountable( fFont);
}


void VMacQuartzGraphicContext::SetTextPosTo( const VPoint& inHwndPos)
{
	fTextPos = inHwndPos;
}


void VMacQuartzGraphicContext::SetTextPosBy(GReal inHoriz, GReal inVert)
{
	fTextPos.SetPosBy( inHoriz, inVert);
}


void VMacQuartzGraphicContext::GetTextPos(VPoint& outHwndPos) const
{
	outHwndPos = fTextPos;
}


void VMacQuartzGraphicContext::SetTextAngle(GReal inAngle)
{
	fTextAngle = inAngle;
}


TextRenderingMode VMacQuartzGraphicContext::SetTextRenderingMode( TextRenderingMode inMode)
{
	TextRenderingMode oldMode = fTextRenderingMode;

	fHighQualityTextRenderingMode = inMode;
	if (fMaxPerfFlag)
		inMode = (inMode & TRM_CONSTANT_SPACING) | TRM_WITHOUT_ANTIALIASING;

	if (fTextRenderingMode != inMode)
	{
		fTextRenderingMode = inMode;
		
		if (fFontMetrics)
			GetCoreTextFactory()->SetRenderingMode( inMode);
		
        if (fTextRenderingMode & TRM_WITHOUT_ANTIALIASING)
			CGContextSetShouldSmoothFonts( fContext, false);
        else
            CGContextSetShouldSmoothFonts( fContext, true);
	}
	return oldMode;
}


TextRenderingMode  VMacQuartzGraphicContext::GetTextRenderingMode() const
{
	return fTextRenderingMode;
}


DrawingMode VMacQuartzGraphicContext::SetTextDrawingMode(DrawingMode inMode, VBrushFlags* /*ioFlags*/)
{
	CGTextDrawingMode	mode;
	
	switch (inMode)
	{
		case DM_PLAIN:
			mode = kCGTextFill;
			break;
			
		case DM_OUTLINED:
			mode = kCGTextStroke;
			break;
			
		case DM_FILLED:
			mode = kCGTextFillStroke;
			break;
			
		case DM_CLIP:
			mode = kCGTextClip;
			break;
		
		default:
			assert(false);
			break;
	}
	
	::CGContextSetTextDrawingMode(fContext, mode);
	return DM_PLAIN;
}


void VMacQuartzGraphicContext::SetSpaceKerning(GReal inSpaceExtra)
{
	if (fCharSpacing != inSpaceExtra)
	{
		fCharSpacing = inSpaceExtra;
		GReal kerningTotal = fCharKerning+fCharSpacing;

		if (kerningTotal != 0.0)
		{
			//use constant kerning space
			fTextRenderingMode |= TRM_CONSTANT_SPACING;
			fHighQualityTextRenderingMode |= TRM_CONSTANT_SPACING;
		}
		else
		{
			//enable auto kerning
			fTextRenderingMode &= ~TRM_CONSTANT_SPACING;
			fHighQualityTextRenderingMode &= ~TRM_CONSTANT_SPACING;
		}
	}
}


void VMacQuartzGraphicContext::SetCharKerning(GReal inSpaceExtra)
{
	if (fCharKerning != inSpaceExtra)
	{
		fCharKerning = inSpaceExtra;
		GReal kerningTotal = fCharKerning+fCharSpacing;

		if (kerningTotal != 0.0)
		{
			//use constant kerning space
			fTextRenderingMode |= TRM_CONSTANT_SPACING;
			fHighQualityTextRenderingMode |= TRM_CONSTANT_SPACING;
		}
		else
		{
			//enable auto kerning
			fTextRenderingMode &= ~TRM_CONSTANT_SPACING;
			fHighQualityTextRenderingMode &= ~TRM_CONSTANT_SPACING;
		}
	}
}


uBYTE VMacQuartzGraphicContext::SetAlphaBlend(uBYTE inAlphaBlend)
{
	uBYTE	oldBlend = fAlphaBlend;
	fAlphaBlend = inAlphaBlend;
	return oldBlend;
}



#pragma mark-




GReal VMacQuartzGraphicContext::GetTextWidth( const VString& inString, bool inRTL) const
{
	xbox_assert(fFont != NULL);
	if (fFont == NULL)
		return 0;

	fTextLayoutMode = TLM_NORMAL;
	return GetFontMetrics()->GetTextWidth( inString, inRTL);	
}


GReal VMacQuartzGraphicContext::GetCharWidth(UniChar inChar) const 
{
	VStr4	charString(inChar);
	return GetTextWidth(charString);
}



VFontMetrics *VMacQuartzGraphicContext::GetFontMetrics() const 
{
	if (fFontMetrics == NULL)
	{
		if (testAssert( fFont != NULL))
		{
			fFontMetrics = new VFontMetrics( fFont, static_cast<const VGraphicContext*>( this));
			XMacTextFactory	*textFactory = fFontMetrics->GetImpl().GetTextFactory();
			textFactory->SetRenderingMode(fTextRenderingMode);
			textFactory->SetLayoutMode(fTextLayoutMode);
		}
	}
	else
	{
		//update text factory pre-computed string attributes if text mode has changed
		
		XMacTextFactory	*textFactory = fFontMetrics->GetImpl().GetTextFactory();
		if (fTextRenderingMode != textFactory->GetRenderingMode())
			textFactory->SetRenderingMode(fTextRenderingMode);
		if (fTextLayoutMode != textFactory->GetLayoutMode())
			textFactory->SetLayoutMode(fTextLayoutMode);
	}
	
	return fFontMetrics;
}


GReal VMacQuartzGraphicContext::GetTextHeight(bool inIncludeExternalLeading) const
{
	VFontMetrics *metrics = GetFontMetrics();
	return (metrics != NULL) ? (inIncludeExternalLeading ? metrics->GetLineHeight() : metrics->GetTextHeight()) : 0;
}

sLONG VMacQuartzGraphicContext::GetNormalizedTextHeight(bool inIncludeExternalLeading) const
{
	VFontMetrics *metrics = GetFontMetrics();
	return (metrics != NULL) ? (inIncludeExternalLeading ? metrics->GetNormalizedLineHeight() : metrics->GetNormalizedTextHeight()) : 0;
}

void VMacQuartzGraphicContext::GetTextBounds( const VString& inString, VRect& oRect) const
{
	GetTextBoundsTypographic(inString, oRect, TLM_NORMAL);
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
void VMacQuartzGraphicContext::GetTextBoundsTypographic( const VString& inString, VRect& oRect, TextLayoutMode inLayoutMode) const
{
	xbox_assert(fFont != NULL);
	if (fFont != NULL)
	{
		fTextLayoutMode = inLayoutMode;
		CTLineRef line = GetCoreTextFactory()->CreateLine( inString);
		CTHelper::GetLineTypographicBounds( line, oRect, false);
		
		::CFRelease(line);
		
		if ((inLayoutMode & TLM_VERTICAL) != 0)
		{
			GReal width	= oRect.GetHeight();
			GReal height = oRect.GetWidth();
			oRect = VRect(0, 0, width, height);
		}
		return;
	}   
	oRect = VRect(0, 0, 0, 0); 
}



#pragma mark-

void VMacQuartzGraphicContext::NormalizeContext()
{
	fTextRenderingMode = TRM_NORMAL;
	fHighQualityTextRenderingMode = TRM_NORMAL;
	fTextLayoutMode = TLM_NORMAL;
}


void VMacQuartzGraphicContext::SaveContext()
{
	fSaveRestoreCount++;

	if (fAxisInvertedStack == NULL)
		fAxisInvertedStack = new std::vector<Boolean>();
	
	fAxisInvertedStack->push_back(fAxisInverted);
	fLineWidthStack.push_back(fLineWidth);
	fLineDashStack.push_back(fUseLineDash);
	
	fFillPatternStack.push_back(fFillPattern);
	RetainRefCountable(fFillPattern);
		
	fLinePatternStack.push_back(fLinePattern);
	RetainRefCountable(fLinePattern);
	
	fIsStrokeDashPatternUnitEqualToStrokeWidthStack.push_back( fIsStrokeDashPatternUnitEqualToStrokeWidth);

	fGlobalAlphaStack.push_back(fGlobalAlpha);

	::CGContextSaveGState(fContext);
}


void VMacQuartzGraphicContext::RestoreContext()
{
	::CGContextRestoreGState(fContext);
	
	fLineWidth=fLineWidthStack.back();
	fLineWidthStack.pop_back();
		
	VPattern* pat = fFillPatternStack.back();
	fFillPatternStack.pop_back();
	CopyRefCountable(&fFillPattern,pat);
	ReleaseRefCountable(&pat);
	
	
	pat=fLinePatternStack.back();
	fLinePatternStack.pop_back();
	CopyRefCountable(&fLinePattern,pat);
	ReleaseRefCountable(&pat);
	
	fAxisInverted = fAxisInvertedStack->back();
	fAxisInvertedStack->pop_back();

	if (fUseLineDash != fLineDashStack.back() || fIsStrokeDashPatternUnitEqualToStrokeWidth != fIsStrokeDashPatternUnitEqualToStrokeWidthStack.back())
	{
		if (!fLineDashStack.back() && fUseLineDash)
			//clear dash pattern
			SetLineDashPattern(0, VLineDashPattern());

		fUseLineDash = fLineDashStack.back();
		fIsStrokeDashPatternUnitEqualToStrokeWidth = fIsStrokeDashPatternUnitEqualToStrokeWidthStack.back();
		
		if (fUseLineDash && fDashPattern.size())
			SetLineDashPattern( fDashPatternOffset, fDashPattern);
	}
	fLineDashStack.pop_back();
	fIsStrokeDashPatternUnitEqualToStrokeWidthStack.pop_back();
	fSaveRestoreCount--;

	fGlobalAlpha = fGlobalAlphaStack.back();
	fGlobalAlphaStack.pop_back();
}


#pragma mark-


/** determine full layout mode from input layout mode and paragraph horizontal alignment */ 
TextLayoutMode  VMacQuartzGraphicContext::_GetFullLayoutMode( TextLayoutMode inMode, AlignStyle inAlignHor)
{
	switch( inAlignHor)
	{
		case AL_LEFT:
			inMode |= TLM_ALIGN_LEFT;
			break;
			
		case AL_RIGHT:
			inMode |= TLM_ALIGN_RIGHT;
			break;
			
		case AL_CENTER:
			inMode |= TLM_ALIGN_CENTER;
			break;
			
		case AL_JUST:
			inMode |= TLM_ALIGN_JUSTIFY;
			break;
			
		default:
			break;
	}
	return inMode;
}


void VMacQuartzGraphicContext::DrawTextBox( const VString& _inString, AlignStyle inHAlign, AlignStyle inVAlign, const VRect& _inHwndBounds, TextLayoutMode inLayoutMode)
{
	inLayoutMode = _GetFullLayoutMode(inLayoutMode, inHAlign);
	VRect inHwndBounds = _inHwndBounds;
	
	// check empty string
	if (_inString.IsEmpty())
		return;

	// check empty bounds
	if (inHwndBounds.GetWidth() <= 0 || inHwndBounds.GetHeight() <= 0)
		return;

	VString inString;
	inString = _inString;
	if (inLayoutMode & (TLM_TRUNCATE_MIDDLE_IF_NECESSARY|TLM_TRUNCATE_END_IF_NECESSARY))
	{
		//workaround for buggy CTFramesetterCreateFrame (bad access exception) while in truncating mode:
		//we must ensure to have enough space to draw at least "c...c" horizontally
		//(if not draw manually only "c...c" and disable truncating) 
		VRect bounds;
		GetTextBoundsTypographic(_inString, bounds, inLayoutMode & (~(TLM_TRUNCATE_MIDDLE_IF_NECESSARY|TLM_TRUNCATE_END_IF_NECESSARY)));
		
		if (_inHwndBounds.GetWidth() < bounds.GetWidth())
		{
			VString sTemp;
			sTemp.AppendUniChar(_inString.GetUniChar(1));
			sTemp.AppendCString("...");
			if (inLayoutMode & TLM_TRUNCATE_MIDDLE_IF_NECESSARY)
				sTemp.AppendUniChar(_inString.GetUniChar(_inString.GetLength()));
			GetTextBoundsTypographic(sTemp, bounds, inLayoutMode & (~(TLM_TRUNCATE_MIDDLE_IF_NECESSARY|TLM_TRUNCATE_END_IF_NECESSARY)));
			if (_inHwndBounds.GetWidth() < bounds.GetWidth())
			{
				inString = sTemp;
				inLayoutMode &= ~(TLM_TRUNCATE_MIDDLE_IF_NECESSARY|TLM_TRUNCATE_END_IF_NECESSARY);
			}
		}
	}

	//for strike-through style, fallback to DrawStyledText
	//(we use custom impl for strike-out style which is not supported by CoreText native impl)
	VFont *font = RetainFont();
	bool doDrawExternal = false;
	if (font && (font->GetFace() & KFS_STRIKEOUT))
	{
		DrawStyledText(inString, NULL, inHAlign, inVAlign, inHwndBounds, inLayoutMode, 72.0f);
		doDrawExternal = true;
	}
	ReleaseRefCountable(&font);
	if (doDrawExternal)
		return;
	
	// get metrics
	fTextLayoutMode = inLayoutMode;
	VFontMetrics *metrics = GetFontMetrics();
	if (metrics == NULL)
		return;
	
	StSetAntiAliasing antialias(static_cast<VGraphicContext *>(this), true); //force antialias
	SaveContext();
	_UseQDAxis();
	
	VAffineTransform ctm;
	VRect inHwndBoundsPrev = inHwndBounds;
	if (fTextAngle != 0.0)
	{
		//transform context in order to draw text box with fTextAngle rotation with origin set to textbox left/top
		inHwndBounds = VRect(0,0,inHwndBoundsPrev.GetWidth(),inHwndBoundsPrev.GetHeight());
		
		GetTransform( ctm);
		ctm.Translate(inHwndBoundsPrev.GetLeft(), inHwndBoundsPrev.GetTop(), VAffineTransform::MatrixOrderPrepend);
		ctm.Rotate( fTextAngle / (GReal) PI * (GReal) 180.0, VAffineTransform::MatrixOrderPrepend);
		SetTransform( ctm);
		
		inHwndBoundsPrev = inHwndBounds;
	}
	if ((inLayoutMode & TLM_VERTICAL) != 0)
	{
		//transform context in order to draw text box with 90¬∞ rotation with reversed width/height and origin set to textbox right/top
		inHwndBounds = VRect(0,0,inHwndBoundsPrev.GetHeight(),inHwndBoundsPrev.GetWidth());
		
		GetTransform( ctm);
		ctm.Translate(inHwndBoundsPrev.GetRight(), inHwndBoundsPrev.GetTop(), VAffineTransform::MatrixOrderPrepend);
		ctm.Rotate( 90, VAffineTransform::MatrixOrderPrepend);
		SetTransform( ctm);
	}
	ClipRect( inHwndBounds);
	//JQ 16/08/2010: fixed ACI0067891 (fixed textbox transform)
	_UseQuartzAxis();
	GReal yQD = inHwndBounds.GetY();
	inHwndBounds.SetY( 0);
		
	//create frame
	//JQ 15/02/2011: fixed ACI0068869 (ensure last text line is drawed even if bounds height is too small)
	VRect bounds = inHwndBounds;
	GReal heightMax = 10000.0f;
	bounds.SetHeight(heightMax);
	GetCoreTextFactory()->SetLayoutMode(inLayoutMode);
	CTFrameRef frame = GetCoreTextFactory()->CreateFrame( inString, bounds);
	
	//determine frame height
	GReal frameHeight = 0;
	if ((inLayoutMode & TLM_VERTICAL) != 0)
	{
		//Core Text CTFrame is buggy in vertical mode:
		//as a workaround, here we need to estimate frame bounds from count of line and font line height
		//otherwise typographic bounds would be wrong
		//TODO: wait for a API fix by Apple
	
		uint32_t numLine = CTHelper::GetFrameLineCount( frame);
		if (numLine == 1)
			frameHeight = metrics->GetTextHeight();
		else if (numLine > 1)
			frameHeight = (numLine-1)*metrics->GetLineHeight()+metrics->GetTextHeight();
	}
	else
	{
		VRect frameBounds;
		CTHelper::GetFrameTypographicBounds( frame, frameBounds);
		frameHeight = frameBounds.GetHeight();	
	}
	_UseQDAxis();
	
	//align frame vertically (horizontally if TLM_VERTICAL)
	GReal dy = 0, dx = 0;
	switch( inVAlign)
	{
		case AL_BOTTOM:
			dy = inHwndBounds.GetHeight()+heightMax-frameHeight;
			break;
			
		case AL_CENTER:
			dy = inHwndBounds.GetHeight()+heightMax-inHwndBounds.GetHeight()*0.5f-frameHeight*0.5f;
			break;
			
		case AL_DEFAULT:
		case AL_TOP:
			dy = heightMax;
			break;
			
		default:
			assert(false);
			break;
	}

	if ((inLayoutMode & TLM_VERTICAL) != 0)
		//Core Text CTFrame is buggy in vertical mode:
		//here we need to subtract ascent as a workaround
		//TODO: wait for a API fix by Apple
		dy += metrics->GetAscent();
	
	GetTransform( ctm);
	//here transform from Quartz2D to QuickDraw coordinate space
	//(we must do it locally in order to properly map user space with Quartz2D)
	ctm.Translate(dx, yQD+dy, VAffineTransform::MatrixOrderPrepend);
	ctm.Scale(1.0f, -1.0f, VAffineTransform::MatrixOrderPrepend);
	SetTransform( ctm);
	
	//draw frame
	CGContextSetTextMatrix( fContext, CGAffineTransformIdentity);
	CGContextSetTextPosition(fContext, 0, 0);
	
	CTFrameDraw( frame, fContext);
	CFRelease(frame);

#if DEBUG_DRAWTEXTBOX_BOUNDS
	::CGContextSetRGBStrokeColor(fContext, 1.0, 0.0, 0.0, 0.5);
	::CGContextStrokeRect(fContext,  inHwndBounds /*QDRectToCGRect(bounds)*/);
#endif
	
	RestoreContext();
}


//return text box bounds
//@remark:
// if input bounds width is not null, use it as max line wrapping width (or max text height in vertical layout mode)
// if input bounds height is not null, use it as max text height (or max line wrapping width in vertical layout mode)
void VMacQuartzGraphicContext::GetTextBoxBounds( const VString& inString, VRect& ioHwndBounds, TextLayoutMode inLayoutMode)
{
	// check empty string
	if (inString.IsEmpty())
	{
		ioHwndBounds.SetCoords(ioHwndBounds.GetX(), ioHwndBounds.GetY(), 0, 0);
		return;
	}
	
	// get metrics
	fTextLayoutMode = inLayoutMode;
	VFontMetrics *metrics = GetFontMetrics();
	if (metrics == NULL)
	{
		ioHwndBounds.SetCoords(ioHwndBounds.GetX(), ioHwndBounds.GetY(), 0, 0);
		return;
	}
	
	// check empty bounds
	if (ioHwndBounds.GetWidth() == 0)
		ioHwndBounds.SetWidth((GReal) 100000.0); //break line only on carriage return
	if (ioHwndBounds.GetHeight() == 0)
		ioHwndBounds.SetHeight((GReal) 100000.0); //draw as far as there are lines to draw

	GReal width = ioHwndBounds.GetWidth();
	GReal height = ioHwndBounds.GetHeight();
	if (inLayoutMode & TLM_VERTICAL)
	{
		width = ioHwndBounds.GetHeight();
		height = ioHwndBounds.GetWidth();
	}
	
	//create frame
	CTFrameRef frame = GetCoreTextFactory()->CreateFrame( inString, VRect(0,0,width,height));
	
	//determine frame bounds
	VRect frameBounds;
	CTHelper::GetFrameTypographicBounds( frame, frameBounds);
	CFRelease(frame);

	width = frameBounds.GetWidth();
	height = frameBounds.GetHeight();
	if (inLayoutMode & TLM_VERTICAL)
	{
		width = frameBounds.GetHeight();
		height = frameBounds.GetWidth();
		frameBounds.SetCoords(0,0,width,height);
	}
	
	if (fTextAngle != 0.0)
	{
		VAffineTransform xform;
		xform.Rotate( fTextAngle / (GReal) PI * (GReal) 180.0, VAffineTransform::MatrixOrderPrepend);
		frameBounds = xform.TransformRect( frameBounds);
	}
	frameBounds.SetPosBy(ioHwndBounds.GetX(),ioHwndBounds.GetY());
	ioHwndBounds = frameBounds;
}
	
	
	



void VMacQuartzGraphicContext::DrawText( const VString& inString, TextLayoutMode inLayoutMode, bool inRTLAnchorRight)
{
	DrawText( inString, fTextPos, inLayoutMode, inRTLAnchorRight);
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
void	VMacQuartzGraphicContext::DrawText (const VString& inString, const VPoint& inPos, TextLayoutMode inLayoutMode, bool inRTLAnchorRight)
{
	StSetAntiAliasing antialias(static_cast<VGraphicContext *>(this), true); //force antialias
	SaveContext();
	_UseQuartzAxis();
	
	VPoint pos = inPos;
	pos.SetY( fQDBounds.size.height  - pos.GetY());
	VAffineTransform ctm;
	VAffineTransform xform;
	if (fTextAngle != 0.0 
		||
		((inLayoutMode & TLM_VERTICAL) != 0))
	{
		if (fTextAngle != 0.0)
		{
			//transform context in order to draw text with fTextAngle rotation with origin set to text position
			xform.Rotate( -fTextAngle / (GReal) PI * (GReal) 180.0, VAffineTransform::MatrixOrderPrepend);
		}
		if ((inLayoutMode & TLM_VERTICAL) != 0)
		{
			//transform context in order to draw text box with 90¬∞ rotation and origin set to text position
			xform.Rotate( -90, VAffineTransform::MatrixOrderPrepend);
		}
		GetTransform( ctm);
		ctm.Translate(pos.GetX(), pos.GetY(), VAffineTransform::MatrixOrderPrepend);
		ctm.Multiply(xform, VAffineTransform::MatrixOrderPrepend);
		SetTransform(ctm);
		pos.Set(0,0);
	}
	
	fTextLayoutMode = inLayoutMode;
	
	//create line
	CTLineRef line = GetCoreTextFactory()->CreateLine(inString);
	
	VPoint offsetAnchor(0,0);
	if ((inLayoutMode & TLM_RIGHT_TO_LEFT) && inRTLAnchorRight)
	{
		//for right to left text, starting position must match text bounding box right border
		//but as Core Text draws text using as starting position text bounding box left border
		//we need to adjust text drawing position
		
		CGFloat ascent,descent,leading;
		double width = CTLineGetTypographicBounds(line, &ascent, &descent, &leading);
		offsetAnchor.x = (GReal) -(width-CTLineGetTrailingWhitespaceWidth(line)); //minus width without trailing spaces as alignment is left here & trailing spaces extend beyond left border
		pos = pos + offsetAnchor;
	}
	
	
	//draw line
	CGContextSetTextMatrix( fContext, CGAffineTransformIdentity);
	CGContextSetTextPosition(fContext, pos.GetX(), pos.GetY());
	CTLineDraw(line, fContext);
	
	//optionnaly custom draw for strike-out style
	if (fFont && (fFont->GetFace() & KFS_STRIKEOUT))
	{
		VFontMetrics *fontMetrics = GetFontMetrics();
		GReal ascent = fontMetrics ? fontMetrics->GetAscent() : fFont->GetSize()*0.5f;
		GReal width = CTLineGetOffsetForStringIndex(line, inString.GetLength(), NULL);

		CGContextSetLineCap(fContext, kCGLineCapSquare);
		CGContextSetLineDash(fContext, 0, NULL, 0);
			
		CGColorRef color = fUseShapeBrushForText ? fFillColor.MAC_CreateCGColorRef() : fTextColor.MAC_CreateCGColorRef();
		CGContextSetStrokeColorWithColor(fContext, color);
		CGColorRelease(color);
		
		CGContextSetLineWidth(fContext, fFont->GetSize()/12.0f);
				
		CGContextBeginPath(fContext);
		CGContextMoveToPoint(fContext, pos.GetX(), pos.GetY()+ascent*0.25f);
		CGContextAddLineToPoint(fContext, pos.GetX()+width, pos.GetY()+ascent*0.25f);
		CGContextStrokePath(fContext);
	}
	
	//update text cursor position
	if (!(inLayoutMode & TLM_TEXT_POS_UPDATE_NOT_NEEDED))
	{
		CGFloat primaryOffset = CTLineGetOffsetForStringIndex(line, inString.GetLength(), NULL);
		VPoint offset(primaryOffset+offsetAnchor.x,offsetAnchor.y);
		
		if (fTextAngle != 0.0 
			||
			((inLayoutMode & TLM_VERTICAL) != 0))
		{
			//apply rotation transform if any
			offset = xform.TransformPoint(offset);
			
			//transform offset from quartz to quickdraw space
			offset.y = -offset.y;
		}
		
		fTextPos = inPos + offset;
	}
	
	//release line
	CFRelease(line);
	
	RestoreContext();
}

//return styled text box bounds
//@remark:
// if input bounds width is not null, use it as max line wrapping width (or max text height in vertical layout mode)
// if input bounds height is not null, use it as max text height (or max line wrapping width in vertical layout mode)
void VMacQuartzGraphicContext::GetStyledTextBoxBounds( const VString& inText, VTreeTextStyle *inStyles, VRect& ioBounds, const GReal inRefDocDPI, bool /*inForceMonoStyle*/)
{
	if (inText.IsEmpty())
	{
		ioBounds.SetWidth(0.0f);
		ioBounds.SetHeight(0.0f);
		return;
	}
	
	const VString *text = &inText;
	VString textDummy;
	if (inText.GetUniChar(inText.GetLength()) == 13)
	{
		//if last character is a CR, we need to append some dummy whitespace character otherwise 
		//we cannot get correct text box bounds
		textDummy.FromString(inText);
		textDummy.AppendChar(' ');
		text = &textDummy;
	}
	
	VFont *font=RetainFont();
	ContextRef contextRef = BeginUsingParentContext();
	
	VRect bounds;
	if (ioBounds.GetWidth() == 0.0f)
		bounds.SetWidth( 100000.0f);
	else
		bounds.SetWidth( ioBounds.GetWidth());
	if (ioBounds.GetHeight() == 0.0f)
		bounds.SetHeight( 100000.0f);
	else
		bounds.SetHeight( ioBounds.GetHeight());

	GReal outWidth = bounds.GetWidth(), outHeight = bounds.GetHeight();
	VStyledTextBox *textBox = new XMacStyledTextBox(contextRef, *text, inStyles, bounds, VColor::sBlackColor, font, fTextRenderingMode, fTextLayoutMode, fCharKerning+fCharSpacing, inRefDocDPI);
	if (textBox)
	{
		textBox->GetSize(outWidth, outHeight);
		textBox->Release();
	}
	EndUsingParentContext(contextRef);
	font->Release();
	ioBounds.SetWidth( outWidth);
	ioBounds.SetHeight( outHeight);
}

void VMacQuartzGraphicContext::DrawStyledText( const VString& inText, VTreeTextStyle *inStyles, AlignStyle inHoriz, AlignStyle inVert, const VRect& inHwndBounds, TextLayoutMode inMode, const GReal inRefDocDPI)
{
	if (inText.IsEmpty())
		return;
	
	StSetAntiAliasing antialias(static_cast<VGraphicContext *>(this), true); //force antialias 

	VColor textColor;
	GetTextColor(textColor);
	VFont *font=RetainFont();
	bool axisInverted = fAxisInverted;
	VAffineTransform ctm;
	GetTransform(ctm);
	_UseQuartzAxis();
	ContextRef contextRef = BeginUsingParentContext();
	VRect newBounds;

	//JQ 20/01/2011
	//	  why override here antialiasing mode (pp ?): this is not consistent with Windows impl & with GetStyledTextBoxBounds
	//	  (bounds are not always the same with or without antialiasing & we should never override caller settings)
	//TextRenderingMode trm = SetTextRenderingMode(TRM_WITH_ANTIALIASING_NORMAL);
	//EnableAntiAliasing();

	newBounds.SetX(inHwndBounds.GetX());
	newBounds.SetY(fQDBounds.size.height - inHwndBounds.GetY() - inHwndBounds.GetHeight());
	newBounds.SetWidth(inHwndBounds.GetWidth());
	newBounds.SetHeight(inHwndBounds.GetHeight());

	//apply custom alignment 
	VStyledTextBox *textBox = new XMacStyledTextBox(contextRef, inText, inStyles, newBounds, textColor, font, fTextRenderingMode, inMode, fCharKerning+fCharSpacing, inRefDocDPI);

	if (textBox)
	{
		textBox->SetTextAlign( inHoriz);
		textBox->SetParaAlign( inVert);
		textBox->Draw(newBounds);
		textBox->Release();
	}
	font->Release();
	EndUsingParentContext(contextRef);
	
	//restore transformation matrix and y axe direction
	SetTransform(ctm);
	fAxisInverted = axisInverted;
	
	//SetTextRenderingMode(trm);
}

/** for the specified styled text box, return the text position at the specified coordinates
@remarks
	output text position is 0-based

	input coordinates are in graphic context user space

	return true if input position is inside character bounds
	if method returns false, it returns the closest character index to the input position
*/
bool VMacQuartzGraphicContext::GetStyledTextBoxCharIndexFromCoord( const VString& inText, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VPoint& inPos, VIndex& outCharIndex, AlignStyle inHoriz, AlignStyle inVert, TextLayoutMode inLayoutMode, const GReal inRefDocDPI)
{
	if (inText.IsEmpty())
	{
		outCharIndex = 0;
		return false;
	}
	
	outCharIndex = 0;
	
	const VString *text = &inText;
	VString textDummy;
	if (inText.GetUniChar(inText.GetLength()) == 13)
	{
		//if last character is a CR, we need to append some dummy whitespace character otherwise 
		//we cannot get correct caret index after last CR
		textDummy.FromString(inText);
		textDummy.AppendChar(' ');
		text = &textDummy;
	}
	
	VFont *font = RetainFont();
    bool doRestoreContext = false;
    if (fAxisInverted)
    {
        doRestoreContext = true;
        SaveContext();
        _UseQuartzAxis();
    }
    
	ContextRef contextRef = BeginUsingParentContext();
	
	VRect newBounds;
	newBounds.SetX(inHwndBounds.GetX());
	newBounds.SetY(fQDBounds.size.height - inHwndBounds.GetY() - inHwndBounds.GetHeight());
	newBounds.SetWidth(inHwndBounds.GetWidth());
	newBounds.SetHeight(inHwndBounds.GetHeight());

	VPoint pos( inPos);
	pos.SetY(fQDBounds.size.height-inPos.GetY());

	//apply custom alignment 
	XMacStyledTextBox *textBox = new XMacStyledTextBox(contextRef, *text, inStyles, newBounds, VColor::sBlackColor, font, fTextRenderingMode, inLayoutMode, fCharKerning+fCharSpacing, inRefDocDPI);

	if (textBox)
	{
		textBox->SetTextAlign( inHoriz);
		textBox->SetParaAlign( inVert);

		CFMutableAttributedStringRef attributedString = textBox->GetAttributedString();
		if (attributedString)
		{
			//create frame setter
			CTFramesetterRef frameSetter = CTFramesetterCreateWithAttributedString(attributedString);	
			
			//create path from input bounds
			CGMutablePathRef path = CGPathCreateMutable();
			CGRect bounds = CGRectMake(newBounds.GetX(), newBounds.GetY(), newBounds.GetWidth(), newBounds.GetHeight());
			CGPathAddRect(path, NULL, bounds);
			
			//create frame from frame setter
			CTFrameRef frame = CTFramesetterCreateFrame(frameSetter, CFRangeMake(0, 0), path, NULL);
			
			CGPathRef framePath = CTFrameGetPath(frame);
			
			CFArrayRef lines = CTFrameGetLines(frame);
			CFIndex numLines = CFArrayGetCount(lines);
			
			if (numLines)
			{
				//get line origins
				CGPoint lineOrigin[numLines];
				CTFrameGetLineOrigins(frame, CFRangeMake(0, 0), lineOrigin); //line origin is aligned on line baseline

				for(CFIndex i = 0; i < numLines; i++) //lines or ordered from the top line to the bottom line
				{
					CTLineRef line = (CTLineRef) CFArrayGetValueAtIndex(lines, i);
					CGFloat ascent, descent, leading;
					CTLineGetTypographicBounds(line, &ascent,  &descent, &leading);

					if (pos.GetY() >= newBounds.GetY()+lineOrigin[i].y+ascent+leading)
					{
						if (i > 0)
						{
							i--;
							line = (CTLineRef) CFArrayGetValueAtIndex(lines, i);
						}
					}
					else
						if (i < numLines-1)
							continue;


					//get line character index from mouse position
					CGPoint offset = { pos.GetX()-lineOrigin[i].x-newBounds.GetX(), pos.GetY()-lineOrigin[i].y-newBounds.GetY() };
					CFIndex indexChar = CTLineGetStringIndexForPosition(line, offset);
					CFRange lineRange = CTLineGetStringRange(line);
					if (indexChar >= inText.GetLength())
						indexChar = inText.GetLength();
					else if (indexChar >= lineRange.location+lineRange.length 
						&& indexChar > 0 
						&& indexChar-1 >= lineRange.location 
						&& inText[indexChar-1] == 13)
						indexChar--;
					if (indexChar == kCFNotFound)
						//text search is not supported
						indexChar = lineRange.location;
					
					outCharIndex = indexChar;
					break;
				}
			}
			CFRelease(path);
			CFRelease(frame);
			CFRelease(frameSetter);
		}

		textBox->Release();
	}

	font->Release();
	EndUsingParentContext(contextRef);
    
    if (doRestoreContext)
        RestoreContext();
	return true;
}

/** for the specified styled text box, return the caret position & height of text at the specified text position
@remarks
	text position is 0-based

	caret should be drawed from outCaretPos to VPoint(outCaretPos.x,OutCaretPos.y+outTextHeight)
*/
void VMacQuartzGraphicContext::GetStyledTextBoxCaretMetricsFromCharIndex( const VString& inText, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading, const bool inCaretUseCharMetrics, AlignStyle inHoriz, AlignStyle inVert, TextLayoutMode inLayoutMode, const GReal inRefDocDPI) 
{
	bool leading = inCaretLeading;
	VIndex charIndex = inCharIndex;
	VFont *font = RetainFont();
	
	if (inText.IsEmpty())
	{
		//for empty text, ensure we return valid caret metrics
		
		outCaretPos.SetY(inHwndBounds.GetTopLeft().GetY());
		if (inHwndBounds.GetWidth() >= 1.0f)
			outCaretPos.SetX(inHwndBounds.GetTopLeft().GetX()+1.0f);
		else 
			outCaretPos.SetX(inHwndBounds.GetTopLeft().GetX());
		outTextHeight = font->GetSize();
		font->Release();
		return;
	}
	
	const VString *text = &inText;
	VString textDummy;
	if (inText.GetUniChar(inText.GetLength()) == 13)
	{
		//if last character is a CR, we need to append some dummy whitespace character otherwise 
		//we cannot get correct caret metrics if character index is equal to text length
		textDummy.FromString(inText);
		textDummy.AppendChar(' ');
		text = &textDummy;
	}
	
	if (inCharIndex < inText.GetLength() && inText[inCharIndex] == 13)
		//fix for carriage return: ensure we return leading position 
		//(in CoreText, carriage return has a non-zero width which is very annoying because it is taken account too for text box measures:
		// there is no easy drawback but to ensure we do not draw caret on the trailing side) 
		leading = true;
	
    bool doRestoreContext = false;
    if (fAxisInverted)
    {
        doRestoreContext = true;
        SaveContext();
        _UseQuartzAxis();
    }

	outCaretPos.SetPos(inHwndBounds.GetTopLeft().GetX(), fQDBounds.size.height-inHwndBounds.GetTopLeft().GetY()); //default output caret pos in Quartz coord space
	outTextHeight = 0.0f;

	ContextRef contextRef = BeginUsingParentContext();
	
	VRect newBounds;
	newBounds.SetX(inHwndBounds.GetX());
	newBounds.SetY(fQDBounds.size.height - inHwndBounds.GetY() - inHwndBounds.GetHeight());
	newBounds.SetWidth(inHwndBounds.GetWidth());
	newBounds.SetHeight(inHwndBounds.GetHeight());

	//apply custom alignment 
	XMacStyledTextBox *textBox = new XMacStyledTextBox(contextRef, *text, inStyles, newBounds, VColor::sBlackColor, font, fTextRenderingMode, inLayoutMode, fCharKerning+fCharSpacing, inRefDocDPI);

	if (textBox)
	{
		textBox->SetTextAlign( inHoriz);
		textBox->SetParaAlign( inVert);

		CFMutableAttributedStringRef attributedString = textBox->GetAttributedString();
		if (attributedString)
		{
			//create frame setter
			CTFramesetterRef frameSetter = CTFramesetterCreateWithAttributedString(attributedString);	
			
			//create path from input bounds
			CGMutablePathRef path = CGPathCreateMutable();
			CGRect bounds = CGRectMake(newBounds.GetX(), newBounds.GetY(), newBounds.GetWidth(), newBounds.GetHeight());
			CGPathAddRect(path, NULL, bounds);
			
			//create frame from frame setter
			CTFrameRef frame = CTFramesetterCreateFrame(frameSetter, CFRangeMake(0, 0), path, NULL);
			
			CGPathRef framePath = CTFrameGetPath(frame);
			
			CFArrayRef lines = CTFrameGetLines(frame);
			CFIndex numLines = CFArrayGetCount(lines);
			
			if (numLines)
			{
				//get line origins
				CGPoint lineOrigin[numLines];
				CTFrameGetLineOrigins(frame, CFRangeMake(0, 0), lineOrigin); //line origin is aligned on line baseline

				for(CFIndex i = 0; i < numLines; i++) //lines or ordered from the top line to the bottom line
				{
					CTLineRef line = (CTLineRef) CFArrayGetValueAtIndex(lines, i);
					CFRange lineRange = CTLineGetStringRange(line);

					bool doGotIt = false;
					if (i == numLines-1) 
						doGotIt = true;
					else if (charIndex >= lineRange.location && charIndex < lineRange.location+lineRange.length)
						doGotIt = true;
					else if (charIndex < lineRange.location)
						{
							if (i > 0)
							{
								i--;
								line = (CTLineRef) CFArrayGetValueAtIndex(lines, i);
								lineRange = CTLineGetStringRange(line);
							}
							doGotIt = true;
						}
					if (doGotIt)
					{
						if (charIndex < lineRange.location)
						{
							charIndex = lineRange.location;
							leading = true;
						} else if (charIndex >= lineRange.location+lineRange.length)
						{
							charIndex = lineRange.location+lineRange.length;
							leading = true;
						}

						if ((!leading) && charIndex+1 <= inText.GetLength())
							charIndex++;
							
						CGFloat secondaryOffset;
						CGFloat primaryOffset = CTLineGetOffsetForStringIndex( line, charIndex, &secondaryOffset);
						
						outCaretPos.SetX( newBounds.GetX() + lineOrigin[i].x + primaryOffset);

						CGFloat ascent, descent, externalLeading;
						CTLineGetTypographicBounds(line, &ascent,  &descent, &externalLeading);

						outCaretPos.SetY( newBounds.GetY() + lineOrigin[i].y + ascent + externalLeading);

						outTextHeight = externalLeading+ascent+descent;
						break;
					}
				}
			}
			CFRelease(path);
			CFRelease(frame);
			CFRelease(frameSetter);
		}

		textBox->Release();
	}

	outCaretPos.SetY( fQDBounds.size.height-outCaretPos.GetY());

	font->Release();
	EndUsingParentContext(contextRef);
    
    if (doRestoreContext)
        RestoreContext();
}


/** return styled text box run bounds from the specified range
@remarks
	used only by some impl like Direct2D or CoreText in order to draw text run background
*/
void VMacQuartzGraphicContext::GetStyledTextBoxRunBoundsFromRange( const VString& inText, VTreeTextStyle *inStyles, const VRect& inHwndBounds, std::vector<VRect>& outRunBounds, sLONG inStart, sLONG inEnd, AlignStyle inHAlign, AlignStyle inVAlign, TextLayoutMode inMode, const GReal inRefDocDPI)
{
	if (inText.IsEmpty())
		return;
	if (inEnd <= inStart)
		return;
		
	VFont *font = RetainFont();
	
	if (inStart < inText.GetLength() && inText[inStart] == 13)
		inStart++;

    bool doRestoreContext = false;
    if (fAxisInverted)
    {
        doRestoreContext = true;
        SaveContext();
        _UseQuartzAxis();
    }

	ContextRef contextRef = BeginUsingParentContext();
	
	VRect newBounds;
	newBounds.SetX(inHwndBounds.GetX());
	newBounds.SetY(fQDBounds.size.height - inHwndBounds.GetY() - inHwndBounds.GetHeight());
	newBounds.SetWidth(inHwndBounds.GetWidth());
	newBounds.SetHeight(inHwndBounds.GetHeight());

	//apply custom alignment 
	XMacStyledTextBox *textBox = new XMacStyledTextBox(contextRef, inText, inStyles, newBounds, VColor::sBlackColor, font, fTextRenderingMode, inMode, fCharKerning+fCharSpacing, inRefDocDPI);

	if (textBox)
	{
		textBox->SetTextAlign( inHAlign);
		textBox->SetParaAlign( inVAlign);

		CFMutableAttributedStringRef attributedString = textBox->GetAttributedString();
		if (attributedString)
		{
			//create frame setter
			CTFramesetterRef frameSetter = CTFramesetterCreateWithAttributedString(attributedString);	
			
			//create path from input bounds
			CGMutablePathRef path = CGPathCreateMutable();
			CGRect bounds = CGRectMake(newBounds.GetX(), newBounds.GetY(), newBounds.GetWidth(), newBounds.GetHeight());
			CGPathAddRect(path, NULL, bounds);
			
			//create frame from frame setter
			CTFrameRef frame = CTFramesetterCreateFrame(frameSetter, CFRangeMake(0, 0), path, NULL);
			
			CGPathRef framePath = CTFrameGetPath(frame);
			
			CFArrayRef lines = CTFrameGetLines(frame);
			CFIndex numLines = CFArrayGetCount(lines);
			
			bool gotStart = false;
			bool gotEnd = false;
			GReal xStart = 0.0f, xEnd = 0.0f;
			if (numLines)
			{
				//get line origins
				CGPoint lineOrigin[numLines];
				CTFrameGetLineOrigins(frame, CFRangeMake(0, 0), lineOrigin); //line origin is aligned on line baseline

				for(CFIndex i = 0; i < numLines; i++) //lines or ordered from the top line to the bottom line
				{
					CTLineRef line = (CTLineRef) CFArrayGetValueAtIndex(lines, i);
					CFRange lineRange = CTLineGetStringRange(line);
					
					bool isStartLine = false;
					bool isEndLine = false;

					//is it start line ?
					if (!gotStart)
					{
						if (i == numLines-1 
							&&
							(inStart >= lineRange.location && inStart <= lineRange.location+lineRange.length)
							)
							gotStart = true;
						else if (inStart >= lineRange.location && inStart < lineRange.location+lineRange.length)
							gotStart = true;
						else if (inStart < lineRange.location)
						{
							if (i > 0)
							{
								i--;
								line = (CTLineRef) CFArrayGetValueAtIndex(lines, i);
								lineRange = CTLineGetStringRange(line);
							}
							gotStart = true;
						}
					
						if (gotStart)
						{
							if (inStart < lineRange.location)
							{
								inStart = lineRange.location;
							} else if (inStart > lineRange.location+lineRange.length)
							{
								inStart = lineRange.location+lineRange.length;
							}

							CGFloat secondaryOffset;
							CGFloat primaryOffset = CTLineGetOffsetForStringIndex( line, inStart, &secondaryOffset);
						
							xStart = newBounds.GetX() + lineOrigin[i].x + primaryOffset;
							isStartLine = true;
						}
					}
					
					//is it end line ?
					if (i == numLines-1 
						&&
						(inEnd >= lineRange.location && inEnd <= lineRange.location+lineRange.length)
						)
						gotEnd = true;
					else if (inEnd >= lineRange.location && inEnd < lineRange.location+lineRange.length)
						gotEnd = true;
					else if (inEnd < lineRange.location)
					{
						if (i > 0)
						{
							i--;
							line = (CTLineRef) CFArrayGetValueAtIndex(lines, i);
							lineRange = CTLineGetStringRange(line);
						}
						gotEnd = true;
					}
					
					if (gotEnd)
					{
						if (inEnd < lineRange.location)
						{
							inEnd = lineRange.location;
						} else if (inEnd > lineRange.location+lineRange.length)
						{
							inEnd = lineRange.location+lineRange.length;
						}

						CGFloat secondaryOffset;
						CGFloat primaryOffset = CTLineGetOffsetForStringIndex( line, inEnd, &secondaryOffset);
						
						xEnd = newBounds.GetX() + lineOrigin[i].x + primaryOffset;
						isEndLine = true;
					}
					
					if (gotStart)
					{
						
						CGFloat ascent, descent, externalLeading;
						GReal width = (GReal)CTLineGetTypographicBounds(line, &ascent,  &descent, &externalLeading);

						GReal y = newBounds.GetY() + lineOrigin[i].y + ascent + externalLeading;
						y = fQDBounds.size.height-y;
						GReal height = externalLeading+ascent+descent;
						
						GReal x = isStartLine ? xStart : newBounds.GetX() + lineOrigin[i].x;
						width = isEndLine ? xEnd-x : newBounds.GetX() + lineOrigin[i].x + width - x;
						
						outRunBounds.push_back( VRect(x, y, width, height));
						if (gotEnd)
							break;
					}
				}
			}
			CFRelease(path);
			CFRelease(frame);
			CFRelease(frameSetter);
		}

		textBox->Release();
	}

	font->Release();
	EndUsingParentContext(contextRef);
    
    if (doRestoreContext)
        RestoreContext();
}



#pragma mark-

void VMacQuartzGraphicContext::FrameRect(const VRect& inHwndBounds)
{
	if (fShapeCrispEdgesEnabled)
	{
		_UseQDAxis();
		VRect bounds( inHwndBounds);
		CEAdjustRectInTransformedSpace( bounds);

		_CreatePathFromRect(bounds);
		_StrokePath();
		return;
	}

	_UseQDAxis();
	_CreatePathFromRect(inHwndBounds);
	_StrokePath();
}


void VMacQuartzGraphicContext::FrameOval(const VRect& inHwndBounds)
{
	if (fShapeCrispEdgesEnabled)
	{
		_UseQDAxis();
		VRect bounds( inHwndBounds);
		CEAdjustRectInTransformedSpace( bounds);

		_CreatePathFromOval(bounds);
		_StrokePath();
		return;
	}

	_UseQDAxis();
	_CreatePathFromOval(inHwndBounds);
	_StrokePath();
}


void VMacQuartzGraphicContext::FrameRegion(const VRegion& inHwndRegion)
{
	_UseQDAxis();
	_CreatePathFromRegion(inHwndRegion);
	_StrokePath();
}


void VMacQuartzGraphicContext::FramePolygon(const VPolygon& inHwndPolygon)
{
	_UseQDAxis();
	
	// Save context because _CreatePathFromPolygon modify the CTM
	::CGContextSaveGState(fContext);
	_CreatePathFromPolygon(inHwndPolygon);
	_StrokePath();
	::CGContextRestoreGState(fContext);
}


void VMacQuartzGraphicContext::FrameBezier(const VGraphicPath& inHwndBezier)
{
	FramePath( inHwndBezier);
}

void VMacQuartzGraphicContext::FramePath(const VGraphicPath& inHwndPath)
{
	if (fShapeCrispEdgesEnabled)
	{
		StUsePath thePath( static_cast<VGraphicContext *>(this), &inHwndPath);
		_UseQDAxis();
		::CGContextSaveGState(fContext);
		::CGContextConcatCTM(fContext, *(thePath.Get()));
		::CGContextAddPath(fContext, *(thePath.Get()));
		_StrokePath();
		::CGContextRestoreGState(fContext);
		return;
	}
	_UseQDAxis();
	::CGContextSaveGState(fContext);
	::CGContextConcatCTM(fContext, inHwndPath);
	::CGContextAddPath(fContext, inHwndPath);
	_StrokePath();
	::CGContextRestoreGState(fContext);
}


void VMacQuartzGraphicContext::FillRect(const VRect& inHwndBounds)
{
	if (fShapeCrispEdgesEnabled)
	{
		_UseQDAxis();
		VRect bounds( inHwndBounds);
		CEAdjustRectInTransformedSpace( bounds, true);

		_CreatePathFromRect(bounds);
		_FillPath();
		return;
	}

	_UseQDAxis();
	_CreatePathFromRect(inHwndBounds);
	_FillPath();
}


void VMacQuartzGraphicContext::FillOval(const VRect& inHwndBounds)
{
	if (fShapeCrispEdgesEnabled)
	{
		_UseQDAxis();
		VRect bounds( inHwndBounds);
		CEAdjustRectInTransformedSpace( bounds, true);

		_CreatePathFromOval(bounds);
		_FillPath();
		return;
	}

	_UseQDAxis();
	_CreatePathFromOval(inHwndBounds);
	_FillPath();
}


void VMacQuartzGraphicContext::FillRegion(const VRegion& inHwndRegion)
{
	_UseQDAxis();
	_CreatePathFromRegion(inHwndRegion);
	_FillPath();
}


void VMacQuartzGraphicContext::FillPolygon(const VPolygon& inHwndPolygon)
{
	_UseQDAxis();
	
	// Save context because _CreatePathFromPolygon modify the CTM
	::CGContextSaveGState(fContext);
	_CreatePathFromPolygon(inHwndPolygon, true);
	_FillPath();
	::CGContextRestoreGState(fContext);
}


void VMacQuartzGraphicContext::FillBezier(VGraphicPath& inHwndBezier)
{
	FillPath( inHwndBezier);
}

void VMacQuartzGraphicContext::FillPath(VGraphicPath& inHwndPath)
{
	if (fShapeCrispEdgesEnabled)
	{
		StUsePath thePath( static_cast<VGraphicContext *>(this), &inHwndPath, true);
		_UseQDAxis();
		::CGContextSaveGState(fContext);
		::CGContextConcatCTM(fContext, *(thePath.Get()));
		::CGContextAddPath(fContext, *(thePath.Get()));
		_FillPath();
		::CGContextRestoreGState(fContext);
		return;
	}
	_UseQDAxis();
	::CGContextSaveGState(fContext);
	::CGContextConcatCTM(fContext, inHwndPath);
	::CGContextAddPath(fContext, inHwndPath);
	_FillPath();
	::CGContextRestoreGState(fContext);
}


void VMacQuartzGraphicContext::DrawRect(const VRect& inHwndBounds)
{
	if (fShapeCrispEdgesEnabled)
	{
		_UseQDAxis();
		VRect bounds( inHwndBounds);
		CEAdjustRectInTransformedSpace( bounds);

		_CreatePathFromRect(bounds);
		_FillStrokePath();
		return;
	}

	_UseQDAxis();
	_CreatePathFromRect(inHwndBounds);
	_FillStrokePath();
}


void VMacQuartzGraphicContext::DrawOval(const VRect& inHwndBounds)
{
	if (fShapeCrispEdgesEnabled)
	{
		_UseQDAxis();
		VRect bounds( inHwndBounds);
		CEAdjustRectInTransformedSpace( bounds);

		_CreatePathFromOval(bounds);
		_FillStrokePath();
		return;
	}

	_UseQDAxis();
	_CreatePathFromOval(inHwndBounds);
	_FillStrokePath();
}


void VMacQuartzGraphicContext::DrawRegion(const VRegion& inHwndRegion)
{
	_UseQDAxis();
	_CreatePathFromRegion(inHwndRegion);
	_FillStrokePath();
}


void VMacQuartzGraphicContext::DrawPolygon(const VPolygon& inHwndPolygon)
{
	_UseQDAxis();
	
	// Save context because _CreatePathFromPolygon modify the CTM
	::CGContextSaveGState(fContext);
	_CreatePathFromPolygon(inHwndPolygon);
	_FillStrokePath();
	::CGContextRestoreGState(fContext);
}


void VMacQuartzGraphicContext::DrawBezier(VGraphicPath& inHwndBezier)
{
	DrawPath( inHwndBezier);
}

void VMacQuartzGraphicContext::DrawPath(VGraphicPath& inHwndPath)
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



void VMacQuartzGraphicContext::EraseRect(const VRect& inHwndBounds)
{
	_UseQDAxis();
	::CGContextClearRect(fContext, inHwndBounds);
}


void VMacQuartzGraphicContext::EraseRegion(const VRegion& /*inHwndRegion*/)
{
	assert(false);	// Not supported
}


void VMacQuartzGraphicContext::InvertRect(const VRect& /*inHwndBounds*/)
{
	assert(false);	// Not supported
}


void VMacQuartzGraphicContext::InvertRegion(const VRegion& /*inHwndRegion*/)
{
	assert(false);	// Not supported
}


void VMacQuartzGraphicContext::DrawLine(const VPoint& inHwndStart, const VPoint& inHwndEnd)
{
	if (fShapeCrispEdgesEnabled)
	{
		_UseQDAxis();

		VPoint _start, _end;
		_start = inHwndStart;
		_end = inHwndEnd;
		CEAdjustPointInTransformedSpace( _start);
		CEAdjustPointInTransformedSpace( _end);

		::CGContextMoveToPoint(fContext, _start.GetX(), _start.GetY());
		::CGContextAddLineToPoint(fContext, _end.GetX(), _end.GetY());

		_StrokePath();
		return;
	}

	_UseQDAxis();
	::CGContextMoveToPoint(fContext, inHwndStart.GetX(), inHwndStart.GetY());
	::CGContextAddLineToPoint(fContext, inHwndEnd.GetX(), inHwndEnd.GetY());
	_StrokePath();
}

void VMacQuartzGraphicContext::DrawLine(const GReal inHwndStartX,const GReal inHwndStartY, const GReal inHwndEndX,const GReal inHwndEndY)
{
	if (fShapeCrispEdgesEnabled)
	{
		_UseQDAxis();

		VPoint _start( inHwndStartX, inHwndStartY);
		VPoint _end( inHwndEndX, inHwndEndY);
		CEAdjustPointInTransformedSpace( _start);
		CEAdjustPointInTransformedSpace( _end);

		::CGContextMoveToPoint(fContext, _start.GetX(), _start.GetY());
		::CGContextAddLineToPoint(fContext, _end.GetX(), _end.GetY());

		_StrokePath();
		return;
	}

	_UseQDAxis();
	::CGContextMoveToPoint(fContext, inHwndStartX, inHwndStartY);
	::CGContextAddLineToPoint(fContext, inHwndEndX, inHwndEndY);
	_StrokePath();
}

void VMacQuartzGraphicContext::DrawLines(const GReal* inCoords, sLONG inCoordCount)
{
	if (inCoordCount < 4 || (inCoordCount & 0x00000001) != 0) return;
	
	if (fShapeCrispEdgesEnabled)
	{
		_UseQDAxis();

		VPoint pos( inCoords[0], inCoords[1]);
		CEAdjustPointInTransformedSpace( pos);
		::CGContextMoveToPoint(fContext, pos.GetX(), pos.GetY());

		for (sLONG index = 2; index < (inCoordCount - 1); index += 2)
		{
			pos.SetPos( inCoords[index], inCoords[index + 1]);
			CEAdjustPointInTransformedSpace( pos);
			::CGContextAddLineToPoint(fContext, pos.GetX(), pos.GetY());
		}

		_StrokePath();
		return;
	}

	_UseQDAxis();
	
	::CGContextMoveToPoint(fContext, inCoords[0], inCoords[1]);
	
	for (sLONG index = 2; index < (inCoordCount - 1); index += 2)
		::CGContextAddLineToPoint(fContext, inCoords[index], inCoords[index + 1]);
		
	_StrokePath();
}


void VMacQuartzGraphicContext::DrawLineTo(const VPoint& inHwndEnd)
{
	if (fShapeCrispEdgesEnabled)
	{
		_UseQDAxis();

		VPoint _end = inHwndEnd;
		CEAdjustPointInTransformedSpace( _end);

		::CGContextAddLineToPoint(fContext, _end.GetX(), _end.GetY());
		_StrokePath();

		// Quartz resets the current path after drawing but the XToolbox
		//	expects to be able to call chained DrawLineTo. We reset a new path.
		::CGContextMoveToPoint(fContext, _end.GetX(), _end.GetY());
		return;
	}

	_UseQDAxis();
	::CGContextAddLineToPoint(fContext, inHwndEnd.GetX(), inHwndEnd.GetY());
	_StrokePath();
	
	// Quartz resets the current path after drawing but the XToolbox
	//	expects to be able to call chained DrawLineTo. We reset a new path.
	::CGContextMoveToPoint(fContext, inHwndEnd.GetX(), inHwndEnd.GetY());
}


void VMacQuartzGraphicContext::DrawLineBy(GReal inWidth, GReal inHeight)
{
	if (fShapeCrispEdgesEnabled)
	{
		_UseQDAxis();
		CGPoint	curPos = ::CGContextGetPathCurrentPoint(fContext);
		VPoint _end( curPos.x + inWidth, curPos.y + inHeight);
		CEAdjustPointInTransformedSpace( _end);

		::CGContextAddLineToPoint(fContext, _end.GetX(), _end.GetY());
		_StrokePath();

		// Quartz resets the current path after drawing but the XToolbox
		//	expects to be able to call chained DrawLineTo. We reset a new path.
		::CGContextMoveToPoint(fContext, _end.GetX(), _end.GetY());
		return;
	}

	_UseQDAxis();
	CGPoint	curPos = ::CGContextGetPathCurrentPoint(fContext);
	::CGContextAddLineToPoint(fContext, curPos.x + inWidth, curPos.y + inHeight);
	_StrokePath();
	
	// Quartz resets the current path after drawing but the XToolbox
	//	expects to be able to call chained DrawLineTo. We reset a new path.
	::CGContextMoveToPoint(fContext, curPos.x + inWidth, curPos.y + inHeight);
}


void VMacQuartzGraphicContext::MoveBrushTo(const VPoint& inHwndPos)
{
	if (fShapeCrispEdgesEnabled)
	{
		_UseQDAxis();
		VPoint _end = inHwndPos;
		CEAdjustPointInTransformedSpace( _end);

		::CGContextMoveToPoint(fContext, _end.GetX(), _end.GetY());
		return;
	}
	_UseQDAxis();
	::CGContextMoveToPoint(fContext, inHwndPos.GetX(), inHwndPos.GetY());
}


void VMacQuartzGraphicContext::MoveBrushBy(GReal inWidth, GReal inHeight)
{
	if (fShapeCrispEdgesEnabled)
	{
		_UseQDAxis();
		CGPoint	curPos = ::CGContextGetPathCurrentPoint(fContext);

		VPoint _end( curPos.x + inWidth, curPos.y + inHeight);
		CEAdjustPointInTransformedSpace( _end);

		::CGContextMoveToPoint(fContext, _end.GetX(), _end.GetY());
		return;
	}
	_UseQDAxis();
	CGPoint	curPos = ::CGContextGetPathCurrentPoint(fContext);
	::CGContextMoveToPoint(fContext, curPos.x + inWidth, curPos.y + inHeight);
}


void VMacQuartzGraphicContext::DrawIcon(const VIcon& inIcon, const VRect& inHwndBounds, PaintStatus /*inState*/, GReal inScale)
{
	_UseQuartzAxis();

	// il est important d'avoir des origines entieres sinon PlotIconRefInContext risque de ne pas prendre la bonne taille d'icone
	long int w, h, x, y;
	// JM 050507 ACI0049850 : il semblerait qu'un bug dans la fonction ::PlotIconRefInContext nous oblige ‚Äö√Ñ¬∞ multiplier
	// par 2 les dimensions de cgRect pour que cela fonctionne correctement...
	if (inScale > 1.0)
	{
		w = ::lrint( inHwndBounds.GetWidth()  * 2.0 );
		h = ::lrint( inHwndBounds.GetHeight() * 2.0 );
		x = ::lrint( inHwndBounds.GetLeft() - inHwndBounds.GetWidth() / 2.0);
		y = ::lrint( fQDBounds.size.height  - inHwndBounds.GetBottom() - inHwndBounds.GetHeight() / 2.0);
	}
	else
	{
		w = ::lrint( inHwndBounds.GetWidth());
		h = ::lrint( inHwndBounds.GetHeight());
		x = ::lrint( inHwndBounds.GetLeft());
		y = ::lrint( fQDBounds.size.height  - inHwndBounds.GetBottom());
	}
	
	CGRect	cgRect = { { static_cast<CGFloat>(x) , static_cast<CGFloat>(y) } , { static_cast<CGFloat>(w), static_cast<CGFloat>(h) } };
	::PlotIconRefInContext(fContext, &cgRect, inIcon.MAC_GetAlignmentType(), inIcon.MAC_GetTransformType(), NULL, kPlotIconRefNormalFlags, inIcon);
}

void VMacQuartzGraphicContext::DrawPicture (const VPicture& inPicture,const VRect& inHwndBounds,VPictureDrawSettings *inSet)
{
	VPictureDrawSettings set(inSet);
	StUseContext_NoRetain	context(this);
	
	//save current transformation matrix and current y axe direction 
	bool axisInverted = fAxisInverted;
	VAffineTransform ctm;
	GetTransform(ctm);
	
	_UseQuartzAxis();
	
	set.SetYAxisSwap(fQDBounds.size.height,true);
	inPicture.Draw(fContext,inHwndBounds,&set);
	
	//restore transformation matrix and y axe direction
	SetTransform(ctm);
	fAxisInverted = axisInverted;

	
}

//draw picture data
void VMacQuartzGraphicContext::DrawPictureData (const VPictureData *inPictureData, const VRect& inHwndBounds,VPictureDrawSettings *inSet)
{
	xbox_assert(inPictureData);
	
	VPictureDrawSettings drawSettings( inSet);
	
	//save current transformation matrix and current y axe direction 
	bool axisInverted = fAxisInverted;
	VAffineTransform ctm;
	GetTransform(ctm);
	
	if (inPictureData->IsKind( sCodec_svg))
		//@remark: better to call this method for svg codec to avoid to create new VMacQuartzGraphicContext instance
		inPictureData->DrawInVGraphicContext( *this, inHwndBounds, &drawSettings);
	else
	{
		drawSettings.SetYAxisSwap( fQDBounds.size.height,true); 
		_UseQuartzAxis();

		inPictureData->DrawInCGContext( fContext, inHwndBounds, &drawSettings);
	}
	
	//restore transformation matrix and y axe direction
	SetTransform(ctm);
	fAxisInverted = axisInverted;
}


#if 0
void VMacQuartzGraphicContext::DrawPicture(const VOldPicture& inPicture, PictureMosaic /*inMosaicMode*/, const VRect& inHwndBounds, TransferMode /*inDrawingMode*/)
{
	_UseQuartzAxis();

	VRect	tempBounds(inHwndBounds);
	tempBounds.SetPosBy(0.0, fQDBounds.size.height - inHwndBounds.GetBottom() - inHwndBounds.GetY());
	
	VOldImageData*	image = inPicture.GetImage();
	if (image == NULL) return;
	
	VOldPictData*	data = (VOldPictData*) image;
	if (!testAssert(image->GetImageKind() == IK_PICT)) return;

	Handle	pictHandle = (Handle) data->GetData();
    CGDataProviderRef	dataProvider = ::CGDataProviderCreateWithData(NULL, *pictHandle, ::GetHandleSize(pictHandle), NULL);
    QDPictRef	pictDataRef = ::QDPictCreateWithProvider(dataProvider);
    
    OSStatus	result = ::QDPictDrawToCGContext(fContext, tempBounds, pictDataRef);
	::QDPictRelease(pictDataRef);
    ::CGDataProviderRelease(dataProvider);
}
#endif
void VMacQuartzGraphicContext::DrawFileBitmap(const VFileBitmap *inFileBitmap, const VRect& inHwndBounds)
{
	_UseQuartzAxis();
	VRect	tempBounds(inHwndBounds);
	tempBounds.SetPosBy(0, fQDBounds.size.height - inHwndBounds.GetBottom() - inHwndBounds.GetY());
	::CGContextDrawImage(fContext, tempBounds, inFileBitmap->fBitmap);
}

void VMacQuartzGraphicContext::DrawPictureFile(const VFile& inFile, const VRect& inHwndBounds)
{	
	CGImageRef image = VPictureCodec_ImageIO::Decode(inFile);
	if (image)
	{
		_UseQuartzAxis();
		VRect	tempBounds(inHwndBounds);
		tempBounds.SetPosBy(0, fQDBounds.size.height - inHwndBounds.GetBottom() - inHwndBounds.GetY());
		
		::CGContextDrawImage(fContext, tempBounds, image);
		::CGImageRelease(image);
	}

	/*
	FSRef fileRef;
	if (!inFile.MAC_GetFSRef(&fileRef))
		return;
	
	_UseQuartzAxis();
	
	VRect	tempBounds(inHwndBounds);
	tempBounds.SetPosBy(0.0, fQDBounds.size.height - inHwndBounds.GetBottom() - inHwndBounds.GetY());

	OSType	dataType;
	Handle	dataRef = NULL;
	ComponentResult	result = ::QTNewDataReferenceFromFSRef(&fileRef, 0, &dataRef, &dataType);
	assert(result == noErr);
	
	if (dataRef != NULL)
	{
		GraphicsImportComponent	component = NULL;
		result = ::GetGraphicsImporterForDataRefWithFlags(dataRef, dataType, &component, 0);
		assert(result == noErr);
		
		CGImageRef	image = NULL;
		result = ::GraphicsImportCreateCGImage(component, &image, kGraphicsImportCreateCGImageUsingCurrentSettings);
		assert(result == noErr);

		if (image != NULL)
		{
			::CGContextDrawImage(fContext, tempBounds, image);
			::CGImageRelease(image);
		}
		
		::CloseComponent(component);
		::DisposeHandle(dataRef);
	}
	*/
}


#pragma mark-

void VMacQuartzGraphicContext::SaveClip()
{
	// Currently the only way for saving clipping
	SaveContext();
}


void VMacQuartzGraphicContext::RestoreClip()
{
	// Currently the only way for saving clipping
	RestoreContext();
}



//add or intersect the specified clipping path with the current clipping path
void VMacQuartzGraphicContext::ClipPath (const VGraphicPath& inPath, Boolean inAddToPrevious, Boolean inIntersectToPrevious)
{
#if VERSIONDEBUG
	if (sDebugNoClipping) return;
#endif
	_UseQDAxis();
	
	::CGContextBeginPath( fContext);
	::CGContextAddPath( fContext, inPath);
	::CGContextClosePath( fContext);

	//Quartz2D constraint: always intersect with current clipping path
	::CGContextClip(fContext);

	RevealClipping(fContext);
}

/** add or intersect the specified clipping path outline with the current clipping path 
@remarks
	this will effectively create a clipping path from the path outline 
	(using current stroke brush) and merge it with current clipping path,
	making possible to constraint drawing to the path outline
*/
void VMacQuartzGraphicContext::ClipPathOutline (const VGraphicPath& inPath, Boolean inAddToPrevious, Boolean inIntersectToPrevious)
{
#if VERSIONDEBUG
	if (sDebugNoClipping) return;
#endif
	_UseQDAxis();
	
	::CGContextBeginPath( fContext);
	::CGContextAddPath( fContext, inPath);
	::CGContextClosePath( fContext);

	::CGContextReplacePathWithStrokedPath(fContext); 

	//Quartz2D constraint: always intersect with current clipping path
	::CGContextClip(fContext);

	RevealClipping(fContext);
}


void VMacQuartzGraphicContext::ClipRect(const VRect& inHwndBounds, Boolean inAddToPrevious, Boolean inIntersectToPrevious)
{
#if VERSIONDEBUG
	if (sDebugNoClipping) return;
#endif
	if(fPrinterScale && inHwndBounds.IsEmpty())
    {
        long a=0;
        a++;
    }
	_UseQDAxis();
	
/*	if (inAddToPrevious)
	{
		::CGContextAddRect(fContext, inHwndBounds);
		::CGContextClip(fContext);
	}
	else*/
		//Quartz2D constraint: always intersect with current clipping path
		::CGContextClipToRect(fContext, inHwndBounds);
	RevealClipping(fContext);
}


void VMacQuartzGraphicContext::ClipRegion(const VRegion& inHwndRegion, Boolean inAddToPrevious, Boolean inIntersectToPrevious)
{
#if VERSIONDEBUG
	if (sDebugNoClipping) return;
#endif
	
	_UseQDAxis();

	//if (!inAddToPrevious)	// Not supported currently
	//	assert(false);
	
	// This may be used to reset previous clipping but is slow and destroy saved clipping
	//Rect	macRect;
	//::ClipCGContextToRegion(fContext, inHwndRegion.GetBounds().MAC_ToQDRect(macRect), inHwndRegion);

	//Quartz2D constraint: always intersect with current clipping path
    if(inHwndRegion.IsEmpty())
    {
        CGRect reclip = CGRectMake(0,0,0,0);
        ::CGContextClipToRect(fContext,reclip);
    }
    else
    {
        _CreatePathFromRegion(inHwndRegion);
        ::CGContextClip(fContext);
    }
	RevealClipping(fContext);
}


void VMacQuartzGraphicContext::GetClip(VRegion& outHwndRgn) const
{
	CGRect clipbounds=CGContextGetClipBoundingBox(fContext);
    outHwndRgn.FromRect(clipbounds);
}


void VMacQuartzGraphicContext::RevealUpdate(WindowRef inWindow)
{
#if VERSIONDEBUG && WITH_QUICKDRAW
//	if (sDebugRevealUpdate)
//		::HIViewFlashDirtyArea(inWindow);
#endif
}


void VMacQuartzGraphicContext::RevealClipping(ContextRef inContext)
{
#if VERSIONDEBUG
	if (sDebugRevealClipping)
	{
		// Fill clip region (translucent yellow)
		::CGContextSaveGState(inContext);
		::CGContextSetRGBFillColor(inContext,1, 1, (GReal) 0.1, (GReal) 0.3);
		::CGContextFillRect(inContext, CGRectMake(0, 0, 10000, 10000));
		::CGContextFlush(inContext);
		::CGContextRestoreGState(inContext);
		
		unsigned long	time;
		::Delay(kDEBUG_REVEAL_CLIPPING_DELAY, &time);
	}
#endif
}


void VMacQuartzGraphicContext::RevealBlitting(ContextRef inContext, const RgnRef inRegion)
{
#if VERSIONDEBUG
	if (sDebugRevealBlitting)
	{
		// Fill pattern (square pattern) with translucent yellow
		::CGContextSaveGState(inContext);
		
		CGAffineTransform	matrix = CGAffineTransformIdentity;
		CGPatternCallbacks	callbacks = { 0, _CGPatternDrawPatternProc, NULL };
		CGPatternRef	patternRef = ::CGPatternCreate((void*)PAT_FILL_CHECKWORK_8, CGRectMake(0, 0, kPATTERN_DEFAULT_SIZE, kPATTERN_DEFAULT_SIZE), CGAffineTransformIdentity, kPATTERN_DEFAULT_SIZE, kPATTERN_DEFAULT_SIZE, kCGPatternTilingConstantSpacing, false, &callbacks);
		
		CGColorSpaceRef	baseSpace = ::CGColorSpaceCreateDeviceRGB();
		CGColorSpaceRef	colorSpace = ::CGColorSpaceCreatePattern(baseSpace);
		::CGContextSetFillColorSpace(inContext, colorSpace);
		::CGColorSpaceRelease(colorSpace);
		::CGColorSpaceRelease(baseSpace);
		
		CGFloat	color[4] = { 1, 1, (CGFloat) 0.1, (CGFloat) 0.3 };
		::CGContextSetFillPattern(inContext, patternRef, color);
		::CGPatternRelease(patternRef);
		
		::HIShapeReplacePathInCGContext(inRegion, inContext);
		::CGContextDrawPath(inContext, kCGPathFill);
		::CGContextFlush(inContext);
		
		::CGContextRestoreGState(inContext);
		
		unsigned long	time;
		::Delay(kDEBUG_REVEAL_BLITTING_DELAY, &time);
	}
#endif
}


void VMacQuartzGraphicContext::RevealInval(ContextRef inContext, const RgnRef inRegion)
{
#if VERSIONDEBUG
	if (sDebugRevealInval)
	{
		// Fill pattern (striped pattern) with translucent yellow
		::CGContextSaveGState(inContext);
		
		CGAffineTransform	matrix = ::CGAffineTransformMakeRotation((GReal) PI / 4);
		CGPatternCallbacks	callbacks = { 0, _CGPatternDrawPatternProc, NULL };
		CGPatternRef	patternRef = ::CGPatternCreate((void*)PAT_FILL_L_HATCH_FAT_8, CGRectMake(0, 0, kPATTERN_DEFAULT_SIZE, kPATTERN_DEFAULT_SIZE), matrix, kPATTERN_DEFAULT_SIZE, kPATTERN_DEFAULT_SIZE, kCGPatternTilingConstantSpacing, false, &callbacks);
		
		CGColorSpaceRef	baseSpace = ::CGColorSpaceCreateDeviceRGB();
		CGColorSpaceRef	colorSpace = ::CGColorSpaceCreatePattern(baseSpace);
		::CGContextSetFillColorSpace(inContext, colorSpace);
		::CGColorSpaceRelease(colorSpace);
		::CGColorSpaceRelease(baseSpace);
		
		GReal	color[4] = { 1, 1, (GReal) 0.1, (GReal) 0.3 };
		::CGContextSetFillPattern(inContext, patternRef, color);
		::CGPatternRelease(patternRef);
		
		::HIShapeReplacePathInCGContext(inRegion, inContext);
		::CGContextDrawPath(inContext,  kCGPathFill);
		::CGContextFlush(inContext);
		
		::CGContextRestoreGState(inContext);
		
		unsigned long	time;
		::Delay(kDEBUG_REVEAL_INVAL_DELAY, &time);
	}
#endif
}


#pragma mark-

void VMacQuartzGraphicContext::CopyContentTo(VGraphicContext* /*inDestContext*/, const VRect* /*inSrcBounds*/, const VRect* /*inDestBounds*/, TransferMode /*inMode*/, const VRegion* /*inMask*/)
{
	assert(false);	// Not available for vectorial context. Use bitmaps intead.
}

VPictureData* VMacQuartzGraphicContext::CopyContentToPictureData()
{
	CGImageRef image = (CGImageRef)RetainBitmap();
	if (image)
	{
		VPictureData* pictData = new VPictureData_CGImage( image);
		CFRelease(image);
		return pictData;
	}
	return NULL;
}

void VMacQuartzGraphicContext::FillUniformColor(const VPoint& /*inContextPos*/, const VColor& /*inColor*/, const VPattern* /*inPattern*/)
{
	assert(false);	// Not available for vectorial context. Use bitmaps intead.
}


void VMacQuartzGraphicContext::SetPixelColor(const VPoint& /*inContextPos*/, const VColor& /*inColor*/)
{
	assert(false);	// Not available for vectorial context. Use bitmaps intead.
}


VColor* VMacQuartzGraphicContext::GetPixelColor(const VPoint& /*inContextPos*/) const
{
	assert(false);	// Not available for vectorial context. Use bitmaps intead.
	return new VColor;
}


void VMacQuartzGraphicContext::Flush()
{
	::CGContextFlush(fContext);

	if(fQDPort)
		fQDPort->Flush();

}


#pragma mark-

void VMacQuartzGraphicContext::_CreatePathFromOval(const VRect& inHwndBounds)
{
	const GReal	cpRatio = (GReal) 0.55;
	
	GReal	top = inHwndBounds.GetTop();
	GReal	left = inHwndBounds.GetLeft();
	GReal	bottom = inHwndBounds.GetBottom();
	GReal	right = inHwndBounds.GetRight();
	GReal	halfWidth = inHwndBounds.GetWidth() / 2;
	GReal	halfHeight = inHwndBounds.GetHeight() / 2;
	
	::CGContextMoveToPoint(fContext, left, top + halfHeight);
	::CGContextAddCurveToPoint(fContext, left, top + halfHeight + cpRatio * halfHeight, left + halfWidth - cpRatio * halfWidth, bottom, left + halfWidth, bottom);
	::CGContextAddCurveToPoint(fContext, left + halfWidth + cpRatio * halfWidth, bottom, right, top + halfHeight + cpRatio * halfHeight, right, top + halfHeight);
	::CGContextAddCurveToPoint(fContext, right, top + halfHeight - cpRatio * halfHeight, left + halfWidth + cpRatio * halfWidth, top, left + halfWidth, top);
	::CGContextAddCurveToPoint(fContext, left + halfWidth - cpRatio * halfWidth, top, left, top + halfHeight - cpRatio * halfHeight, left, top + halfHeight);
	::CGContextClosePath(fContext);
}


void VMacQuartzGraphicContext::_CreatePathFromPolygon(const VPolygon& inHwndPolygon, bool inFillOnly)
{
	::CGContextBeginPath(fContext);
	if (fShapeCrispEdgesEnabled)
	{
		const CGPoint *points = (const CGPoint *)inHwndPolygon.GetNativePoints();
		sLONG numPoint = inHwndPolygon.GetPointCount();
		if (numPoint >= 2)
		{
			CGPoint pointsMemStack[16];
			CGPoint *pointsCE = numPoint <= 16 ? pointsMemStack : new CGPoint [numPoint];

			const CGPoint *ptSrc = points;
			CGPoint *ptDst = pointsCE;
			for (int i = 0; i < numPoint; i++, ptSrc++, ptDst++)
			{
				VPoint pt( ptSrc->x, ptSrc->y);
				CEAdjustPointInTransformedSpace( pt, inFillOnly);
				ptDst->x = pt.GetX();
				ptDst->y = pt.GetY();
			}
			
			::CGContextAddLines(fContext, pointsCE, numPoint);

			if (numPoint > 16)
				delete [] pointsCE;
		}
	}
	else
		::CGContextAddLines(fContext, inHwndPolygon.GetNativePoints(), inHwndPolygon.GetPointCount());
	::CGContextClosePath(fContext);
}


void VMacQuartzGraphicContext::_CreatePathFromRegion(const VRegion& inHwndRegion)
{
	::HIShapeReplacePathInCGContext(inHwndRegion, fContext);
}


void VMacQuartzGraphicContext::_CreatePathFromRect(const VRect& inHwndRect)
{
	::CGContextAddRect(fContext, inHwndRect);
}


void VMacQuartzGraphicContext::_StrokePath()
{
	if (fLinePattern == NULL)
	{
		::CGContextDrawPath(fContext, kCGPathStroke);
	}
	else
	{
		//for axial & radial gradient patterns only, we paint the stroke interior
		if ((fLinePattern->GetKind() == 'axeP')||(fLinePattern->GetKind() == 'radP'))
			::CGContextReplacePathWithStrokedPath(fContext);	

		fLinePattern->ApplyToContext(this);
	}
}


void VMacQuartzGraphicContext::_FillPath()
{
	if (fFillPattern == NULL)
		::CGContextDrawPath(fContext, fFillRule == FILLRULE_NONZERO ? kCGPathFill : kCGPathEOFill );
	else
		fFillPattern->ApplyToContext(this);
}


void VMacQuartzGraphicContext::_FillStrokePath()
{
    if (fFillPattern == NULL && fLinePattern == NULL)
		::CGContextDrawPath(fContext, fFillRule == FILLRULE_NONZERO ? kCGPathFillStroke : kCGPathEOFillStroke);
	
	if (fFillPattern != NULL)
		fFillPattern->ApplyToContext(this);
	
	if (fLinePattern != NULL)
	{
		//for axial & radial gradient patterns only, we paint the stroke interior
		if ((fLinePattern->GetKind() == 'axeP')||(fLinePattern->GetKind() == 'radP'))
			::CGContextReplacePathWithStrokedPath(fContext);	
		
		fLinePattern->ApplyToContext(this);
	}
}


CGRect VMacQuartzGraphicContext::QDRectToCGRect(const Rect& inQDRect)
{
	CGRect	cgRect;
	cgRect.origin.x = inQDRect.left;
	cgRect.origin.y = inQDRect.top;
	cgRect.size.width = inQDRect.right - inQDRect.left;
	cgRect.size.height = inQDRect.bottom - inQDRect.top;
	
	return cgRect;
}


void VMacQuartzGraphicContext::_UseQDAxis() const
{
	if (!fAxisInverted)
	{
		if(IsPrintingContext() && ARCH_64)
		{
			::CGContextTranslateCTM(fContext,0, fPrinterAdjustedPageRect.size.height);
		}
		else
		{
			::CGContextTranslateCTM(fContext, 0, fQDBounds.size.height);
		}
		
		::CGContextScaleCTM(fContext, 1, -1);
		
		fAxisInverted = true;
	}
}


void VMacQuartzGraphicContext::_UseQuartzAxis() const
{
	if (fAxisInverted)
	{
		::CGContextScaleCTM(fContext, 1, -1);
		if(IsPrintingContext() && ARCH_64)
		{
			::CGContextTranslateCTM(fContext, 0, -fPrinterAdjustedPageRect.size.height);
		}
		else
		{
			::CGContextTranslateCTM(fContext, 0, -fQDBounds.size.height);
		}
		fAxisInverted = false;
	}
}


/** return current clipping bounding rectangle 
 @remarks
 bounding rectangle is expressed in VGraphicContext normalized coordinate space 
 (ie coordinate space with y axis pointing down - compliant with gdiplus and quickdraw coordinate spaces)
 */  
void VMacQuartzGraphicContext::GetClipBoundingBox( VRect& outBounds)
{
	_UseQDAxis();
	VRect bounds(::CGContextGetClipBoundingBox( fContext));
	outBounds = bounds;
}


PatternRef VMacQuartzGraphicContext::_CreateStandardPattern(ContextRef /*inContext*/, PatternStyle inStyle)
{
	if (inStyle > PAT_FILL_LAST) return NULL;
	
	VRect	bounds(0, 0, kPATTERN_DEFAULT_SIZE, kPATTERN_DEFAULT_SIZE);
	CGPatternCallbacks	callbacks = { 0, _CGPatternDrawPatternProc, NULL };
	CGPatternRef	patternRef = ::CGPatternCreate((void*)inStyle, bounds, CGAffineTransformIdentity, kPATTERN_DEFAULT_SIZE, kPATTERN_DEFAULT_SIZE, kCGPatternTilingNoDistortion, false, &callbacks);
	return patternRef;
}


void VMacQuartzGraphicContext::_ReleasePattern(ContextRef /*inContext*/, PatternRef inPatternRef)
{
	::CGPatternRelease(inPatternRef);
}


void VMacQuartzGraphicContext::_ApplyPattern(ContextRef inContext, PatternStyle inStyle, const VColor& inColor, PatternRef inPatternRef, FillRuleType inFillRule)
{
	if (inStyle > PAT_FILL_LAST)
	{
		uLONG	count;
		GReal	lengths[kMAX_LINE_STYLE_ELEMENTS];
		_GetLineStyleElements(inStyle, kMAX_LINE_STYLE_ELEMENTS, lengths, count);
		::CGContextSetLineDash(inContext, 0, lengths, count);
		::CGContextDrawPath(inContext, kCGPathStroke);
	}
	else if (testAssert(inPatternRef != NULL))
	{
		CGColorSpaceRef	baseSpace = ::CGColorSpaceCreateDeviceRGB();
		CGColorSpaceRef	colorSpace = ::CGColorSpaceCreatePattern(baseSpace);
		::CGContextSetFillColorSpace(inContext, colorSpace);
		::CGColorSpaceRelease(colorSpace);
		::CGColorSpaceRelease(baseSpace);
		
		GReal	color[4] = { inColor.GetRed() / (GReal) 255, inColor.GetGreen() / (GReal) 255, inColor.GetBlue() / (GReal) 255, inColor.GetAlpha() / (GReal) 255 };
		::CGContextSetFillPattern(inContext, inPatternRef, color);
		::CGContextDrawPath(inContext, inFillRule == FILLRULE_NONZERO ? kCGPathFill : kCGPathEOFill );
	}
}


void VMacQuartzGraphicContext::_CGPatternDrawPatternProc(void* inInfo, CGContextRef inContext)
{
	PatternStyle	style = *((PatternStyle*) &inInfo);
	switch (style)
	{ 
		case PAT_FILL_PLAIN:
			::CGContextFillRect(inContext, CGRectMake(0, 0, kPATTERN_DEFAULT_SIZE, kPATTERN_DEFAULT_SIZE));
			break;
			
		case PAT_FILL_EMPTY:
			break;
			
		case PAT_FILL_CHECKWORK_8:
		{
			CGRect	rects[] = { { 0, 0, 8, 8 },
								{ 8, 8, 8, 8 } };
			::CGContextFillRects(inContext, rects, sizeof(rects) / sizeof(CGRect));
			break;
		}
			
		case PAT_FILL_CHECKWORK_4:
			assert(false);	// TBD
			break;
			
		case PAT_FILL_GRID_8:
			assert(false);	// TBD
			break;
			
		case PAT_FILL_GRID_4:
			assert(false);	// TBD
			break;
			
		case PAT_FILL_GRID_FAT_8:
			assert(false);	// TBD
			break;
			
		case PAT_FILL_GRID_FAT_4:
			assert(false);	// TBD
			break;
			
		case PAT_FILL_H_HATCH_8:
			assert(false);	// TBD
			break;
			
		case PAT_FILL_V_HATCH_8:
			assert(false);	// TBD
			break;
			
		case PAT_FILL_HV_HATCH_8:
			assert(false);	// TBD
			break;
			
		case PAT_FILL_L_HATCH_8:
			assert(false);	// TBD
			break;
			
		case PAT_FILL_R_HATCH_8:
			assert(false);	// TBD
			break;
			
		case PAT_FILL_H_HATCH_FAT_8:
			assert(false);	// TBD
			break;
			
		case PAT_FILL_V_HATCH_FAT_8:
			assert(false);	// TBD
			break;
			
		case PAT_FILL_HV_HATCH_FAT_8:
			assert(false);	// TBD
			break;
		
		case PAT_FILL_L_HATCH_FAT_8:
		{
			CGRect	rects[] = { { 0, 0, kPATTERN_DEFAULT_SIZE, 4 },{ 0, 8, kPATTERN_DEFAULT_SIZE, 4 } };
			::CGContextFillRects(inContext, rects, sizeof(rects) / sizeof(CGRect));
			break;
		}

		default:
			assert(false);
			break;
	}
}


void VMacQuartzGraphicContext::_GetLineStyleElements(PatternStyle inStyle, uLONG inMaxCount, GReal* outElements, uLONG& outCount)
{
	switch (inStyle)
	{
		case PAT_LINE_PLAIN:
			outElements[0] = 10;
			outCount = 1;
			break;
			
		case PAT_LINE_DOT_LARGE:
			outElements[0] = 9;
			outElements[1] = 4;
			outCount = 2;
			break;
			
		case PAT_LINE_DOT_MEDIUM:
			outElements[0] = 5;
			outElements[1] = 3;
			outCount = 2;
			break;
			
		case PAT_LINE_DOT_SMALL:
			outElements[0] = 3;
			outElements[1] = 2;
			outCount = 2;
			break;
			
		case PAT_LINE_DOT_MINI:
			outElements[0] = 1;
			outElements[1] = 1;
			outElements[2] = 1;
			outElements[3] = 1;
			outElements[4] = 1;
			outElements[5] = 1;
			outCount = 6;
			break;
			
		case PAT_LINE_AXIS_MEDIUM:
			outElements[0] = 9;
			outElements[1] = 2;
			outElements[2] = 2;
			outElements[3] = 2;
			outCount = 4;
			break;
			
		case PAT_LINE_AXIS_SMALL:
			outElements[0] = 5;
			outElements[1] = 2;
			outElements[2] = 2;
			outElements[3] = 2;
			outCount = 4;
			break;
			
		case PAT_LINE_DBL_SMALL:
			outElements[0] = 5;
			outElements[1] = 2;
			outElements[2] = 2;
			outElements[3] = 2;
			outElements[4] = 2;
			outElements[5] = 2;
			outCount = 6;
			break;
			
		case PAT_LINE_DBL_MEDIUM:
			outElements[0] = 5;
			outElements[1] = 2;
			outElements[2] = 5;
			outElements[3] = 2;
			outElements[4] = 2;
			outElements[5] = 2;
			outCount = 6;
			break;

		default:
			assert(false);
			break;
	}
	
	assert(outCount < inMaxCount);
}




GradientRef VMacQuartzGraphicContext::_CreateAxialGradient(ContextRef /*inContext*/, const VAxialGradientPattern* inGradient)
{
	// Input/output values - see CGShadingCreateAxial doc
    const CGFloat	inputRange[2] = { 0, 1 };
    const CGFloat	outputRanges[8] = { 0, 1 , 0, 1, 0, 1 , 0, 1  };
	const CGFunctionCallbacks	callbacksClamp= { 0, _CGShadingComputeValueProc, _CGShadingReleaseInfoProc };
	const CGFunctionCallbacks	callbacksTile = { 0, _CGShadingTileComputeValueProc, _CGShadingReleaseInfoProc };
	const CGFunctionCallbacks	callbacksFlip = { 0, _CGShadingTileFlipComputeValueProc, _CGShadingReleaseInfoProc };
	
	// pp il ne faut surtout pas passer inGradient a la callback.
	// jq ok but must be static cast in VGradientPattern * because function wait for VGradientPattern *
	VGradientPattern* clone=static_cast<VGradientPattern *>(inGradient->Clone()); 

	CGColorSpaceRef	colorspace = ::CGColorSpaceCreateDeviceRGB();
	sLONG	components = ::CGColorSpaceGetNumberOfComponents(colorspace) + 1;

	//adjust starting and ending position according to the current wrapping mode
	CGFunctionRef	functionRef;
	VPoint startPos, endPos;
	startPos = inGradient->GetStartPoint();
	endPos   = inGradient->GetEndPoint();
	switch (inGradient->GetWrapMode())
	{
		case ePatternWrapClamp:
		{
			//clamp wrap mode
			functionRef = ::CGFunctionCreate((void*)clone, 1, inputRange, components, outputRanges, &callbacksClamp);
		}
		break;
		case ePatternWrapTile:
		{
			//normal tiling wrap mode
			VPoint delta = endPos-startPos;
			delta = delta * (kPATTERN_GRADIENT_TILE_MAX_COUNT/2);
			startPos = startPos-delta;
			delta	 = delta * 2;
			endPos	 = startPos+delta;
			functionRef = ::CGFunctionCreate((void*)clone , 1, inputRange, components, outputRanges, &callbacksTile);
		}
		break;
		case ePatternWrapFlipX:
		case ePatternWrapFlipY:
		case ePatternWrapFlipXY:
		default:
		{
			//flip tiling wrap mode
			VPoint delta = endPos-startPos;
			delta = delta * (kPATTERN_GRADIENT_TILE_MAX_COUNT/2);
			startPos = startPos-delta;
			delta	 = delta * 2;
			endPos	 = startPos+delta;
			functionRef = ::CGFunctionCreate((void*)clone , 1, inputRange, components, outputRanges, &callbacksFlip);
		}
		break;
	}
	GradientRef shading = ::CGShadingCreateAxial(colorspace, startPos, endPos, functionRef, true, true);

	::CGColorSpaceRelease(colorspace);
	::CGFunctionRelease(functionRef);
	
	// Increase Refcount
	//@remark: to prevent circular ref counting, we do not need to retain the VGradientPattern
	//         because the returned object is owned by the VGradientPattern
	
	return shading;
}


GradientRef VMacQuartzGraphicContext::_CreateRadialGradient(ContextRef /*inContext*/, const VRadialGradientPattern* inGradient)
{
	// Input/output values - see CGShadingCreateRadial doc
    const CGFloat	inputRange[2] = { 0, 1 };
    const CGFloat	outputRanges[8] = { 0, 1 , 0, 1, 0, 1 , 0, 1  };
	const CGFunctionCallbacks	callbacksClamp= { 0, _CGShadingComputeValueProc, _CGShadingReleaseInfoProc };
	const CGFunctionCallbacks	callbacksTile = { 0, _CGShadingTileComputeValueProc, _CGShadingReleaseInfoProc };
	const CGFunctionCallbacks	callbacksFlip = { 0, _CGShadingTileFlipComputeValueProc, _CGShadingReleaseInfoProc };
	
	// pp il ne faut surtout pas passer inGradient a la callback.
	// jq ok but must be static cast in VGradientPattern * because function wait for VGradientPattern *
	VGradientPattern* clone=static_cast<VGradientPattern *>(inGradient->Clone()); 
	
	CGColorSpaceRef	colorspace = ::CGColorSpaceCreateDeviceRGB();
	sLONG	components = ::CGColorSpaceGetNumberOfComponents(colorspace) + 1;
	CGFunctionRef	functionRef = ::CGFunctionCreate((void*)clone, 1, inputRange, components, outputRanges, 
													 inGradient->GetWrapMode() == ePatternWrapClamp ?	&callbacksClamp : 
													 (inGradient->GetWrapMode() == ePatternWrapTile ?	&callbacksTile :
																										&callbacksFlip
													 ));
	GReal startRadius;
	GReal endRadius;
	VPoint startPoint;
	VPoint endPoint;

	if (inGradient->GetWrapMode() == ePatternWrapClamp) 
	{
		startRadius = std::max( inGradient->GetStartRadiusX(),inGradient->GetStartRadiusY());
		endRadius = std::max( inGradient->GetEndRadiusX(),inGradient->GetEndRadiusY());
		startPoint = inGradient->GetStartPoint();
		endPoint = inGradient->GetEndPoint();
		
		//map focal coordinates from ellipsoid coordinate space to circular coordinate space centered on endpoint
		//(here gradient is assumed to be circular: ellipsoid transform is actually integrated in gradient transform)
		if (startPoint != endPoint
			&&
			inGradient->GetEndRadiusX() != inGradient->GetEndRadiusY()
			&&
			inGradient->GetEndRadiusX() != 0.0
			&& 
			inGradient->GetEndRadiusY() != 0.0)
		{
			VPoint focal = startPoint-endPoint;
			if (inGradient->GetEndRadiusX() >= inGradient->GetEndRadiusY())
				focal.y *= inGradient->GetEndRadiusX()/inGradient->GetEndRadiusY();
			else
				focal.x *= inGradient->GetEndRadiusY()/inGradient->GetEndRadiusX();
			
			startPoint = endPoint + focal;
		}
	}
	else
	{
		startRadius = std::max( inGradient->GetStartRadiusX(),inGradient->GetStartRadiusY());
		endRadius  =  inGradient->GetEndRadiusTiling();
		startPoint = inGradient->GetStartPointTiling(); //already precomputed in circular coordinate space centered on endpoint
		endPoint = inGradient->GetEndPointTiling();
	}


	CGShadingRef	shading = ::CGShadingCreateRadial(colorspace, startPoint, startRadius, endPoint, endRadius, 
													  functionRef, true, true);
	::CGColorSpaceRelease(colorspace);
	::CGFunctionRelease(functionRef);
	
	// Increase Refcount
	//@remark: to prevent circular ref counting, we do not need to retain the VGradientPattern
	//         because the returned object is owned by the VGradientPattern
	
	
	return shading;
}


void VMacQuartzGraphicContext::_ReleaseGradient(GradientRef inShading)
{
	::CGShadingRelease(inShading);
}


void VMacQuartzGraphicContext::_ApplyGradient(ContextRef inContext, GradientRef inShading, FillRuleType inFillRule)
{
	::CGContextSaveGState(inContext);
	if (inFillRule == FILLRULE_NONZERO)
		::CGContextClip(inContext);
	else
		::CGContextEOClip(inContext);
	::CGContextDrawShading(inContext, inShading);
	::CGContextRestoreGState(inContext);
}

void VMacQuartzGraphicContext::_ApplyGradientWithTransform (ContextRef inContext, GradientRef inShading, const VAffineTransform& inMat, FillRuleType inFillRule)
{
	::CGContextSaveGState(inContext);
	if (inFillRule == FILLRULE_NONZERO)
		::CGContextClip(inContext);
	else
		::CGContextEOClip(inContext);
	
	//apply local transformation matrix 
	CGAffineTransform mat;
	inMat.ToNativeMatrix( mat);
	::CGContextConcatCTM( inContext, mat); 
		
	//draw with shading
	::CGContextDrawShading(inContext, inShading);
	::CGContextRestoreGState(inContext);
}


//clamp wrapping mode per pixel callback function
void VMacQuartzGraphicContext::_CGShadingComputeValueProc(void* inInfo, const GReal* inValue, GReal* outValue)
{
	if (inInfo == NULL) return;
		
	((VGradientPattern*) inInfo)->ComputeValues(inValue, outValue);
}

//normal tiling wrapping mode per pixel callback function
void VMacQuartzGraphicContext::_CGShadingTileComputeValueProc (void* inInfo, const GReal* inValue, GReal* outValue)
{
	//remap input value to tile coordinate space
	GReal value = (*inValue) * kPATTERN_GRADIENT_TILE_MAX_COUNT;
	value = std::fmod(value, 1);

	((VGradientPattern*) inInfo)->ComputeValues(&value, outValue);

}

//flip tiling wrapping mode per pixel callback function
void VMacQuartzGraphicContext::_CGShadingTileFlipComputeValueProc (void* inInfo, const GReal* inValue, GReal* outValue)
{
	//remap input value to tile coordinate space
	GReal value = (*inValue) * kPATTERN_GRADIENT_TILE_MAX_COUNT;
	if ((((sLONG)value) & 1) != 0)
		//inverse alternatively input value
		value = 1-std::fmod(value, (GReal) 1);
	else
		value = std::fmod(value, (GReal) 1);

	((VGradientPattern*) inInfo)->ComputeValues(&value, outValue);
}


void VMacQuartzGraphicContext::_CGShadingReleaseInfoProc(void* inInfo)
{
	if (inInfo == NULL) 
		return;
	
	// pp c'est le CGShading object qui a l'ownership du VGradientPattern
	((VGradientPattern*) inInfo)->Release();
}




// Create a filter or alpha layer graphic context
// with specified bounds and filter
// @remark : retain the specified filter until EndLayer is called
bool VMacQuartzGraphicContext::_BeginLayer(const VRect& _inBounds, GReal inAlpha, VGraphicFilterProcess *inFilterProcess, VImageOffScreenRef inBackBuffer, bool inInheritParentClipping, bool /*inTransparent*/)
{
	//determine if backbuffer is owned by context (temporary layer) or by caller (re-usable layer)
	bool ownBackBuffer = inBackBuffer == NULL;
	if (inBackBuffer == (VImageOffScreenRef)0xFFFFFFFF)
		inBackBuffer = NULL;
	bool bNewBackground = true;
	CGLayerRef layer = inBackBuffer ? (CGLayerRef)(*inBackBuffer) : NULL;

	if (inFilterProcess == NULL && ownBackBuffer)
	{
		//begin new transparency layer
		_UseQDAxis(); 
		::CGContextSaveGState( fContext);

		if (inAlpha != 1.0)
		{
			::CGContextSetAlpha( fContext, inAlpha);
			::CGContextBeginTransparencyLayer ( fContext, NULL);	
		}
		
		fStackLayerBounds.push_back( VRect(0,0,0,0));    
		fStackLayerBackground.push_back( NULL);
		fStackLayerOwnBackground.push_back(true);
		fStackLayerGC.push_back( NULL);
		fStackLayerAlpha.push_back( inAlpha);
		fStackLayerFilter.push_back( NULL);
	}
	else
	{
		//begin new filter layer
		
		//compute transformed bounds
		_UseQDAxis(); 
		VAffineTransform ctm;
		GetTransform( ctm);
		VPoint ctmTranslate;
		VRect bounds;
		
		VRect boundsClip;
		if (inInheritParentClipping)
		{
			boundsClip.FromRect( VRect(::CGContextGetClipBoundingBox( fContext)));
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
			
			_UseQDAxis(); 
			::CGContextSaveGState( fContext);
	
			fStackLayerBounds.push_back( VRect(0,0,0,0));  
			fStackLayerBackground.push_back( NULL);
			fStackLayerOwnBackground.push_back( true);
			fStackLayerGC.push_back( NULL);
			fStackLayerAlpha.push_back( 1.0);
			fStackLayerFilter.push_back( NULL);
			return true;
		}	
		else
		{
			_UseQDAxis(); 

			//draw area is not empty:
			//build the layer back buffer and graphic context and stack it
			
			//inflate a little bit to deal with antialiasing 
			bounds.SetPosBy(-2.0f,-2.0f);
			bounds.SetSizeBy(4.0f,4.0f); 
			
			bounds.NormalizeToInt();
			
			//update bounds taking account filter
			if (inFilterProcess)
			{
				//inflate bounds to take account of filter (for blur filter for instance)
				VPoint offsetFilter;
				offsetFilter.x = std::ceil(inFilterProcess->GetInflatingOffset()*ctm.GetScaleX());
				offsetFilter.y = std::ceil(inFilterProcess->GetInflatingOffset()*ctm.GetScaleY());
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
		
			//stack layer back buffer
			CGRect cgBounds = bounds;
			if (layer == NULL)
			{	
				//create layer
                if (cgBounds.size.width < 8192 && cgBounds.size.height < 8192)
                    layer = CGLayerCreateWithContext(fContext, cgBounds.size, NULL);
			}
			else 
			{
				CGSize size = CGLayerGetSize(layer);
				if (size.width != cgBounds.size.width
					||
					size.height != cgBounds.size.height)
				{
					//release previous layer
					inBackBuffer->Clear();
					//create new layer
                    if (cgBounds.size.width < 8192 && cgBounds.size.height < 8192)
                        layer = CGLayerCreateWithContext(fContext, cgBounds.size, NULL);
                    else
                        layer = NULL;
				}
				else
				{
					//use external back buffer
					bNewBackground = false;
					CGLayerRetain(layer);
				}
			}
			if (!layer)
			{
				//if we fall here, probably video memory is full:
				//push empty layer
				::CGContextSaveGState( fContext);
		
				fStackLayerBounds.push_back( VRect(0,0,0,0));  
				fStackLayerBackground.push_back( NULL);
				fStackLayerOwnBackground.push_back( true);
				fStackLayerGC.push_back( NULL);
				fStackLayerAlpha.push_back( 1.0);
				fStackLayerFilter.push_back( NULL);
				return true;
			}
			::CGContextSaveGState( fContext);

			//stack filter process
			if (inFilterProcess)
				inFilterProcess->Retain();
			fStackLayerFilter.push_back( inFilterProcess);

			//stack layer transformed bounds
			fStackLayerBounds.push_back( bounds);    

			if (inBackBuffer)
			{
				inBackBuffer->SetAndRetain( layer);
				inBackBuffer->Retain();
			}
			else
				inBackBuffer = new VImageOffScreen( layer);
			if (layer)
				CGLayerRelease(layer);

			fStackLayerBackground.push_back( inBackBuffer);
			fStackLayerOwnBackground.push_back( ownBackBuffer);
			
			//stack current graphic context
			fStackLayerGC.push_back( fContext);
			
			//stack layer alpha transparency 
			fStackLayerAlpha.push_back( inAlpha);
			
			//set current graphic context to the layer graphic context
			xbox_assert(layer);
			CGContextRef gcLayer = CGLayerGetContext(layer);
			fContext = gcLayer;
			
			//inherit clipping from parent if appropriate
			if (inInheritParentClipping && (!ownBackBuffer))
				//get parent clipping in layer local space
				boundsClip.SetPosBy(-bounds.GetX()-ctmTranslate.GetX(), -bounds.GetY()-ctmTranslate.GetY());

			//set clipping
			SetTransform(VAffineTransform());
			VRect boundsClipLocal = bounds;
			boundsClipLocal.SetPosTo(0.0f, 0.0f);
			if (inInheritParentClipping && (!ownBackBuffer))
				boundsClipLocal.Intersect(boundsClip);
			::CGContextClipToRect( fContext, boundsClipLocal);

            CGContextSetAllowsAntialiasing( fContext, true);
            CGContextSetShouldAntialias(fContext, fIsHighQualityAntialiased && !GetMaxPerfFlag());
            CGContextSetAllowsFontSmoothing( fContext, true);
            if (fTextRenderingMode & TRM_WITHOUT_ANTIALIASING)
                CGContextSetShouldSmoothFonts( fContext, false);
            else
                CGContextSetShouldSmoothFonts( fContext, true);
           
			//set layer transform
			ctm.Translate( -bounds.GetX(), -bounds.GetY(),
						   VAffineTransform::MatrixOrderAppend);
			SetTransform( ctm);
		}
	}
	if (!ownBackBuffer)
		return bNewBackground;
	else
		return 	true;
}


// Create a shadow layer graphic context
// with specified bounds and transparency
void VMacQuartzGraphicContext::BeginLayerShadow(const VRect& _inBounds, const VPoint& inShadowOffset, GReal inShadowBlurDeviation, const VColor& inShadowColor, GReal inAlpha)
{
	_UseQDAxis(); 
	::CGContextSaveGState( fContext);
		
	//set alpha transparency
	::CGContextSetAlpha( fContext, inAlpha);
	
	//set shadow parameters

	//offset must be transformed to base space in order to feed CGContextSetShadowWithColor
	CGSize offset;
	offset.width = inShadowOffset.GetX();
	offset.height = inShadowOffset.GetY();
	VAffineTransform ctm;
	GetTransform( ctm);
	offset.width *= ctm.GetScaleX();
	offset.height *= ctm.GetScaleY();
	
	CGColorSpaceRef	colorSpace = ::CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
	CGFloat componentsColor[4] = { ((CGFloat)inShadowColor.GetRed())/255, ((CGFloat)inShadowColor.GetGreen())/255, ((CGFloat)inShadowColor.GetBlue())/255, ((CGFloat)inShadowColor.GetAlpha())/255};
	CGColorRef colorShadow = ::CGColorCreate( colorSpace, componentsColor);
	::CGContextSetShadowWithColor( fContext, offset, (float)inShadowBlurDeviation, colorShadow); 
	::CGColorRelease( colorShadow);
	::CGColorSpaceRelease(colorSpace);

	//begin transparency layer
	::CGContextBeginTransparencyLayer ( fContext, NULL);	
	
	fStackLayerBounds.push_back( VRect(0,0,0,0));    
	fStackLayerBackground.push_back( NULL);
	fStackLayerOwnBackground.push_back(true);
	fStackLayerGC.push_back( NULL);
	fStackLayerAlpha.push_back( (GReal) 0.999);	//@remark: we set a dummy alpha value different from 1.0 
												//to ensure that CGContextEndTransparencyLayer is called in EndLayer()
	fStackLayerFilter.push_back( NULL);
}



// Draw the layer graphic context in the container graphic context 
// (which can be either the main graphic context or another layer graphic context)
VImageOffScreenRef VMacQuartzGraphicContext::_EndLayer( ) 
{
	if (fStackLayerGC.empty())
		return NULL;
	
	//pop layer stack... 
	
	//...pop layer destination bounds
	VRect bounds = fStackLayerBounds.back();
	fStackLayerBounds.pop_back();
	
	//...pop layer back buffer
	CGLayerRef layer = NULL;
	VImageOffScreenRef offscreen = fStackLayerBackground.back();
	if (offscreen)
	{
		layer = (CGLayerRef)(*offscreen);
		xbox_assert(layer);
	}
	fStackLayerBackground.pop_back();
	bool ownBackBuffer = fStackLayerOwnBackground.back();
	fStackLayerOwnBackground.pop_back();
	
	//...pop parent graphic context
	CGContextRef gcLayerParent = fStackLayerGC.back();
	fStackLayerGC.pop_back();
	
	//...pop layer alpha transparency
	GReal alpha = fStackLayerAlpha.back();
	fStackLayerAlpha.pop_back();
	
	//..pop layer filter
	VGraphicFilterProcess *filterProcess = fStackLayerFilter.back();
	fStackLayerFilter.pop_back();
	
	if (filterProcess == NULL && ownBackBuffer)
	{
		//end transparency layer (shadow or alpha-blend only)

		if (alpha != 1)
		{
			::CGContextEndTransparencyLayer( fContext);
		}
		::CGContextRestoreGState( fContext);
		fAxisInverted = true; //stacked context is quickdraw compliant so restore inverted axis status  
		if (offscreen)
			offscreen->Release();
		return NULL;
	}

	if (gcLayerParent == NULL)
	{
		::CGContextRestoreGState( fContext);
		fAxisInverted = true; //stacked context is quickdraw compliant so restore inverted axis status  
		if (ownBackBuffer)
		{
			if (offscreen)
				offscreen->Release();
			return NULL;
		}
		else
			return offscreen;
	}
	
	//restore parent graphic context
	fContext = gcLayerParent;
	::CGContextRestoreGState( fContext);
	fAxisInverted = true; //stacked context is quickdraw compliant so restore inverted axis status  
	
	//ensure layer bounds size are consistent
	CGSize size = CGLayerGetSize(layer);
	xbox_assert(size.width == bounds.GetWidth()
				&&
				size.height == bounds.GetHeight());
	
	//reset ctm...
	//(because layer coordinate space scaling is equal to the parent graphic context coordinate space scaling)
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
	
	CGInterpolationQuality iqBackup;
	if (!bNeedInterpolation)
	{
		iqBackup = CGContextGetInterpolationQuality(fContext);
		CGContextSetInterpolationQuality(fContext, kCGInterpolationNone);
	}
	
	if (alpha < 1.0)
	{
		::CGContextSaveGState( fContext);
		
		::CGContextSetAlpha( fContext, alpha);
		::CGContextBeginTransparencyLayer ( fContext, NULL);	
	}
	
	if (filterProcess)
	{
		//draw layer using filter
		
		VGraphicImage *image = new VGraphicImage( layer);
		filterProcess->Apply( this, image, bounds, NULL, eGraphicImageBlendType_Over, ctm.GetScaling());
		image->Release();
		filterProcess->Release();
	}
	else 
		//draw layer
		CGContextDrawLayerAtPoint(fContext, CGPointMake(bounds.GetX(), bounds.GetY()), layer);
	
	if (alpha < 1.0)
	{
		::CGContextEndTransparencyLayer( fContext);
		
		::CGContextRestoreGState( fContext);
		fAxisInverted = true; //stacked context is quickdraw compliant so restore inverted axis status  
	}

	if (!bNeedInterpolation)
		CGContextSetInterpolationQuality(fContext, iqBackup);
	
	//restore ctm
	SetTransform(ctm);
	
	if (ownBackBuffer && offscreen)
		//destroy offscreen
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
bool VMacQuartzGraphicContext::ShouldClearLayerOffScreen(const VRect& inBounds, VImageOffScreenRef inBackBuffer)
{
	if (!inBackBuffer)
		return true;
	CGLayerRef layer = inBackBuffer ? (CGLayerRef)(*inBackBuffer) : NULL;
	if (!layer)
		return true;

	//compute transformed bounds
	_UseQDAxis(); 
	VAffineTransform ctm;
	GetTransform( ctm);
	VPoint ctmTranslate;
	VRect bounds;
	
	//for background drawing, in order to avoid discrepancies
	//we draw in target device space minus user to device transform translation
	//we cannot draw directly in screen user space otherwise we would have discrepancies
	//(because background layer is re-used by caller and layer origin will not match always the same pixel origin in screen space)
	VAffineTransform xform = ctm;
	ctmTranslate = xform.GetTranslation();
	xform.SetTranslation( 0.0f, 0.0f);
	bounds = xform.TransformRect( inBounds);
	
	//inflate a little bit to deal with antialiasing 
	bounds.SetPosBy(-2.0f,-2.0f);
	bounds.SetSizeBy(4.0f,4.0f); 
	bounds.NormalizeToInt();

	CGSize size = CGLayerGetSize(layer);
	if (size.width != bounds.GetWidth()
		||
		size.height != bounds.GetHeight())
		//graphic context rotation/scaling has probably changed since background layer was built
		//so do nothing and return false to inform caller background layer is dirty
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
bool VMacQuartzGraphicContext::DrawLayerOffScreen(const VRect& inBounds, VImageOffScreenRef inBackBuffer, bool inDrawAlways)
{
	if (!inBackBuffer)
		return false;
	CGLayerRef layer = inBackBuffer ? (CGLayerRef)(*inBackBuffer) : NULL;
	if (!layer)
		return false;

	//compute transformed bounds
	_UseQDAxis(); 
	VAffineTransform ctm;
	GetTransform( ctm);
	VPoint ctmTranslate;
	VRect bounds;
	
	//for background drawing, in order to avoid discrepancies
	//we draw in target device space minus user to device transform translation
	//we cannot draw directly in screen user space otherwise we would have discrepancies
	//(because background layer is re-used by caller and layer origin will not match always the same pixel origin in screen space)
	VAffineTransform xform = ctm;
	ctmTranslate = xform.GetTranslation();
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

		CGSize size = CGLayerGetSize(layer);
		if (size.width != bounds.GetWidth()
			||
			size.height != bounds.GetHeight())
			//graphic context rotation/scaling has probably changed since background layer was built
			//so do nothing and return false to inform caller background layer is dirty
			return false;
	}

	if (!bNeedInterpolation)
		bNeedInterpolation = ctmTranslate.x != floor(ctmTranslate.x) || ctmTranslate.y != floor(ctmTranslate.y);
	xform.MakeIdentity();
	xform.SetTranslation( ctmTranslate.x, ctmTranslate.y);
	SetTransform( xform);
	
	CGInterpolationQuality iqBackup;
	if (!bNeedInterpolation)
	{
		iqBackup = CGContextGetInterpolationQuality(fContext);
		CGContextSetInterpolationQuality(fContext, kCGInterpolationNone);
	}
	
	CGContextDrawLayerAtPoint(fContext, CGPointMake(bounds.GetX(), bounds.GetY()), layer);
	
	if (!bNeedInterpolation)
		CGContextSetInterpolationQuality(fContext, iqBackup);
	
	//restore ctm
	SetTransform(ctm);
	
	return true;
}
	
	

// clear all layers and restore main graphic context
void VMacQuartzGraphicContext::ClearLayers()
{
	while (!fStackLayerGC.empty())
	{
		fStackLayerBounds.pop_back();
		
		CGContextRef gcLayer = fStackLayerGC.back();
		if (gcLayer)
		{
			fContext = gcLayer;
			fAxisInverted = true; //stacked context is quickdraw compliant so restore inverted axis status  
		}
		fStackLayerGC.pop_back();
		
		bool ownBackBuffer = fStackLayerOwnBackground.back();
		fStackLayerOwnBackground.pop_back();
		
		VImageOffScreenRef offscreen = fStackLayerBackground.back();
		if (offscreen)
			offscreen->Release();
		fStackLayerBackground.pop_back();
		
		GReal alpha = fStackLayerAlpha.back();
		fStackLayerAlpha.pop_back();
		
		VGraphicFilterProcess *filter = fStackLayerFilter.back();
		if (filter == NULL && ownBackBuffer)
		{
			xbox_assert(gcLayer == NULL);
			if (alpha != 1)
				::CGContextEndTransparencyLayer( fContext);
			::CGContextRestoreGState( fContext);
			fAxisInverted = true; //stacked context is quickdraw compliant so restore inverted axis status  
		}
		else if (filter)
			filter->Release();
		fStackLayerFilter.pop_back();
	}
}


void VMacQuartzGraphicContext::GetTransformToLayer(VAffineTransform &outTransform, sLONG inIndexLayer) 
{
	_UseQDAxis();	//here we need to ensure that transform user space is quickdraw compliant
					//because VGraphicContext user uses quickdraw coordinates and not quartz coordinates
					//(for all graphic context methods, coordinates or bounds or expressed in quickdraw coordinate space
					// - ie y axis down - which is compliant too with other OS implementations)
	
	if (fStackLayerGC.size() == 0)
	{
		GetTransform(outTransform);
		return;
	}
	
	GetTransform(outTransform);
	std::vector<VRect>::reverse_iterator itBounds = fStackLayerBounds.rbegin();
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
			CGContextRef oldGC = fContext;
			fContext = *itGC;
			VAffineTransform ctm;
			GetTransform( ctm);
			outTransform.Translate( ctm.GetTranslateX(), ctm.GetTranslateY(),
								   VAffineTransform::MatrixOrderAppend);
			fContext = oldGC;
		}
	}
}

/** clear current graphic context area
@remarks
	Quartz2D impl: inBounds cannot be equal to NULL (do nothing if it is the case)
*/
void VMacQuartzGraphicContext::Clear( const VColor& inColor, const VRect *inBounds, bool inBlendCopy)
{
	//with Quartz, we need always to specify bounds
	_UseQDAxis();
	
	xbox_assert(inBounds);
	if (!inBounds)
		return;

	CGRect bounds;
	bounds.origin.x = inBounds->GetX();
	bounds.origin.y = inBounds->GetY();
	bounds.size.width = inBounds->GetWidth();
	bounds.size.height = inBounds->GetHeight();
	if (inBlendCopy)
		::CGContextSetBlendMode( fContext, kCGBlendModeCopy);
	if (inColor == VColor(0,0,0,0))
		::CGContextClearRect( fContext, bounds);
	else
	{
		::CGContextSaveGState( fContext);
		::CGContextSetRGBFillColor(fContext, inColor.GetRed() / (GReal) 255.0, inColor.GetGreen() / (GReal) 255.0, inColor.GetBlue() / (GReal) 255.0, inColor.GetAlpha() / (GReal) 255.0);
		::CGContextFillRect( fContext, bounds);
		::CGContextRestoreGState( fContext);
	}
	if (inBlendCopy)
		::CGContextSetBlendMode( fContext, kCGBlendModeNormal);
}

void VMacQuartzGraphicContext::QDFrameRect (const VRect& inHwndRect)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));

	VRect r=inHwndRect;
	
	if(fLineWidth>1 || fPrinterScale)
	{
		r.SetPosBy(fLineWidth/ (GReal)2.0,(fLineWidth/(GReal)2.0));
		r.SetSizeBy(-fLineWidth,-fLineWidth);
	}
	else
	{
		r.SetPosBy(0,1);
		r.SetSizeBy(-fLineWidth,-fLineWidth);
	}
	FrameRect(r);
}

void VMacQuartzGraphicContext::QDFrameRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));

	VRect r=inHwndRect;

	if(fLineWidth>1)
	{
		r.SetPosBy(fLineWidth/ (GReal)2.0,(fLineWidth/(GReal)2.0));
		r.SetSizeBy(-fLineWidth,-fLineWidth);
	}
	else
	{
		r.SetPosBy(0.5f,0.5f);
		r.SetSizeBy(-fLineWidth,-fLineWidth);
	}

	FrameRoundRect (r,inOvalWidth,inOvalHeight);
}
void VMacQuartzGraphicContext::QDFrameOval (const VRect& inHwndRect)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));

	VRect r=inHwndRect;
	
	if(fLineWidth>1)
	{
		r.SetPosBy(fLineWidth/(GReal)2.0,(fLineWidth/(GReal)2.0));
		r.SetSizeBy(-fLineWidth,-fLineWidth);
	}
	else
	{
		r.SetPosBy(0.5f,0.5f);
		r.SetSizeBy(-fLineWidth,-fLineWidth);
	}
	FrameOval(r);
}

void VMacQuartzGraphicContext::QDDrawRect (const VRect& inHwndRect)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));

	VRect r=inHwndRect;
	
	if(fLineWidth>1 || fPrinterScale)
	{
		r.SetPosBy(fLineWidth/ (GReal)2.0,(fLineWidth/(GReal)2.0));
		r.SetSizeBy(-fLineWidth,-fLineWidth);
	}
	else
	{
		r.SetPosBy(0,1);
		r.SetSizeBy(-fLineWidth,-fLineWidth);
	}
	
	if(!fUseLineDash)
	{
		DrawRect(r);
	}
	else 
	{
		FillRect(inHwndRect);
		FrameRect(r);
	}
	
	
}

void VMacQuartzGraphicContext::QDDrawRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));

	VRect r=inHwndRect;

	if(fLineWidth>1)
	{
		r.SetPosBy(fLineWidth/ (GReal)2.0,(fLineWidth/(GReal)2.0));
		r.SetSizeBy(-fLineWidth,-fLineWidth);
	}
	else
	{
		r.SetPosBy(0.5f,0.5f);
		r.SetSizeBy(-fLineWidth,-fLineWidth);
	}

	if(!fUseLineDash)
	{
		DrawRoundRect(r,inOvalHeight,inOvalHeight);
	}
	else 
	{
		FillRoundRect(inHwndRect,inOvalHeight,inOvalHeight);
		FrameRoundRect(r,inOvalHeight,inOvalHeight);
	}
	
	
}

void VMacQuartzGraphicContext::QDDrawOval (const VRect& inHwndRect)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));

	VRect r=inHwndRect;
	
	if(fLineWidth>1)
	{
		r.SetPosBy(fLineWidth/(GReal)2.0,(fLineWidth/(GReal)2.0));
		r.SetSizeBy(-fLineWidth,-fLineWidth);
	}
	else
	{
		r.SetPosBy(0.5f,0.5f);
		r.SetSizeBy(-fLineWidth,-fLineWidth);
	}
	if(!fUseLineDash)
	{
		DrawOval(r);
	}
	else 
	{
		FillOval(inHwndRect);
		FrameOval(r);
	}
}

void VMacQuartzGraphicContext::QDFillRect (const VRect& inHwndRect)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));

	VRect r=inHwndRect;

	//r.SetPosBy(-0.5f,-0.5f);
	
	FillRect(r);
}

void VMacQuartzGraphicContext::QDFillRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));
	
	VRect r=inHwndRect;
	
	//r.SetPosBy(-0.5f,-0.5f);
	
	FillRoundRect(r,inOvalWidth,inOvalHeight);
}

void VMacQuartzGraphicContext::QDFillOval (const VRect& inHwndRect)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));

	VRect r=inHwndRect;

	//r.SetPosBy(-0.5f,-0.5f);
	
	FillOval(r);
}

void VMacQuartzGraphicContext::QDDrawLine(const GReal inHwndStartX,const GReal inHwndStartY, const GReal inHwndEndX,const GReal inHwndEndY)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));
	
	GReal sx=inHwndStartX,ex=inHwndEndX,sy=inHwndStartY,ey=inHwndEndY;
	
	if(inHwndStartY == inHwndEndY)
	{
		sy += (fLineWidth/2);
		ey += (fLineWidth/2);
		if(fHairline && fPrinterScale)
		{
			sx += (fLineWidth/2);
			ex -= (fLineWidth/2);
		}
	} 
	else if(inHwndStartX == inHwndEndX)
	{
		sx += (fLineWidth/2);
		ex += (fLineWidth/2);
		if(fHairline && fPrinterScale)
		{
			sy += (fLineWidth/2);
			ey -= (fLineWidth/2);
		}
	}

	DrawLine(sx,sy, ex,ey);
}

void VMacQuartzGraphicContext::QDDrawLines(const GReal* inCoords, sLONG inCoordCount)
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

void VMacQuartzGraphicContext::QDMoveBrushTo (const VPoint& inHwndPos)
{
	MoveBrushTo (inHwndPos);
}
void VMacQuartzGraphicContext::QDDrawLine (const VPoint& inHwndStart, const VPoint& inHwndEnd)
{
	QDDrawLine (inHwndStart.GetX(),inHwndStart.GetY(),inHwndEnd.GetX(),inHwndEnd.GetY());
}
void VMacQuartzGraphicContext::QDDrawLineTo (const VPoint& inHwndEnd)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));

	DrawLineTo (inHwndEnd);
}


/** draw text layout 
@remarks
	text layout origin is set to inTopLeft
*/
void VMacQuartzGraphicContext::_DrawTextLayout( VTextLayout *inTextLayout, const VPoint& inTopLeft)
{
	if (inTextLayout->fTextLength == 0 && inTextLayout->fPlaceHolder.IsEmpty())
		return;

	inTextLayout->BeginUsingContext(this);
	if (!testAssert(inTextLayout->fLayoutIsValid))
	{
		inTextLayout->EndUsingContext();
		return;
	}

	xbox_assert(inTextLayout->fTextBox);
	if (!inTextLayout->_BeginDrawLayer( inTopLeft))
	{
		//text has been refreshed from layer: we do not need to redraw text content
		inTextLayout->_EndDrawLayer();
		inTextLayout->EndUsingContext();
		return;
	}
	
	
	//redraw layer text content or draw without layer
	SaveContext();
	_UseQuartzAxis();

	ContextRef contextRef = BeginUsingParentContext();

	VRect bounds(	inTopLeft.GetX()+inTextLayout->fMargin.left, 
					fQDBounds.size.height - (inTopLeft.GetY()+inTextLayout->fMargin.top+inTextLayout->_GetLayoutHeightMinusMargin()), 
					inTextLayout->_GetLayoutWidthMinusMargin(), 
					inTextLayout->_GetLayoutHeightMinusMargin());
	inTextLayout->fTextBox->SetDrawContext( contextRef);
	inTextLayout->fTextBox->Draw( bounds);

	EndUsingParentContext(contextRef);

	RestoreContext();

	inTextLayout->_EndDrawLayer();
	inTextLayout->EndUsingContext();
}
	
/** return text layout bounds 
	@remark:
		text layout origin is set to inTopLeft
		on output, outBounds contains text layout bounds including any glyph overhangs

		on input, max layout width is determined by inTextLayout->GetMaxWidth() & max layout height by inTextLayout->GetMaxHeight()
		(if max width or max height is equal to 0.0f, it is assumed to be infinite)

		if text is empty and if inReturnCharBoundsForEmptyText == true, returned bounds will be set to a single char bounds 
		(useful while editing in order to compute initial text bounds which should not be empty in order to draw caret)
*/
void VMacQuartzGraphicContext::_GetTextLayoutBounds( VTextLayout *inTextLayout, VRect& outBounds, const VPoint& inTopLeft, bool inReturnCharBoundsForEmptyText)
{
	if (inTextLayout->fTextLength == 0 && !inReturnCharBoundsForEmptyText)
	{
		outBounds.SetCoordsTo( inTopLeft.GetX(), inTopLeft.GetY(), 0.0f, 0.0f);
		return;
	}

	inTextLayout->BeginUsingContext(this);
	if (!testAssert(inTextLayout->fLayoutIsValid))
	{
		inTextLayout->EndUsingContext();
		return;
	}

	outBounds.SetCoordsTo( inTopLeft.GetX(), inTopLeft.GetY(), 0.0f, 0.0f);
	outBounds.SetWidth( inTextLayout->fCurLayoutWidth);
	outBounds.SetHeight( inTextLayout->fCurLayoutHeight);
	
	inTextLayout->EndUsingContext();
}


/** return text layout run bounds from the specified range 
@remarks
	text layout origin is set to inTopLeft
*/
void VMacQuartzGraphicContext::_GetTextLayoutRunBoundsFromRange( VTextLayout *inTextLayout, std::vector<VRect>& outRunBounds, const VPoint& inTopLeft, sLONG inStart, sLONG inEnd)
{
	if (inTextLayout->fTextLength == 0)
		return;

	inTextLayout->BeginUsingContext(this, true);
	if (!testAssert(inTextLayout->fLayoutIsValid))
	{
		inTextLayout->EndUsingContext();
		return;
	}

	xbox_assert(inTextLayout->fTextBox);
	XMacStyledTextBox *textBox = static_cast<XMacStyledTextBox *>(inTextLayout->fTextBox);

	if (inEnd < 0)
		inEnd = inTextLayout->fTextLength;

	if (inEnd <= inStart)
	{
		inTextLayout->EndUsingContext();
		return;
	}	
	
//	if (inStart < inTextLayout->fTextLength && inTextLayout->GetText()[inStart] == 13)
//		inStart++;

    bool doRestoreContext = false;
    if (fAxisInverted)
    {
        doRestoreContext = true;
        SaveContext();
        _UseQuartzAxis();
    }

	VPoint frameOrigin;
	frameOrigin.SetX(inTopLeft.GetX()+inTextLayout->fMargin.left);
	frameOrigin.SetY(fQDBounds.size.height - (inTopLeft.GetY()+inTextLayout->fMargin.top+inTextLayout->_GetLayoutHeightMinusMargin()));
		
	//get frame
	CTFrameRef frame = textBox->GetCurLayoutFrame( inTextLayout->_GetLayoutWidthMinusMargin(), inTextLayout->_GetLayoutHeightMinusMargin());
	frameOrigin += textBox->GetLayoutOffset();
 	
	//get frame path
	CGPathRef framePath = CTFrameGetPath(frame);
	
	//get lines
	CFArrayRef lines = CTFrameGetLines(frame);
	CFIndex numLines = CFArrayGetCount(lines);
	
	//extract run bounds while scanning lines
	bool gotStart = false;
	bool gotEnd = false;
	GReal xStart = 0.0f, xEnd = 0.0f;
	if (numLines)
	{
		//get line origins
		CGPoint lineOrigin[numLines];
		CTFrameGetLineOrigins(frame, CFRangeMake(0, 0), lineOrigin); //line origin is aligned on line baseline

		for(CFIndex i = 0; i < numLines; i++) //lines or ordered from the top line to the bottom line
		{
			CTLineRef line = (CTLineRef) CFArrayGetValueAtIndex(lines, i);
			CFRange lineRange = CTLineGetStringRange(line);
			
			bool isStartLine = false;
			bool isEndLine = false;

			//is it start line ?
			if (!gotStart)
			{
				if (i == numLines-1 
					&&
					(inStart >= lineRange.location && inStart <= lineRange.location+lineRange.length)
					)
					gotStart = true;
				else if (inStart >= lineRange.location && inStart < lineRange.location+lineRange.length)
					gotStart = true;
				else if (inStart < lineRange.location)
				{
					if (i > 0)
					{
						i--;
						line = (CTLineRef) CFArrayGetValueAtIndex(lines, i);
						lineRange = CTLineGetStringRange(line);
					}
					gotStart = true;
				}
			
				if (gotStart)
				{
					if (inStart < lineRange.location)
					{
						inStart = lineRange.location;
					} else if (inStart > lineRange.location+lineRange.length)
					{
						inStart = lineRange.location+lineRange.length;
					}

					CGFloat secondaryOffset;
					CGFloat primaryOffset = CTLineGetOffsetForStringIndex( line, inStart, &secondaryOffset);
				
					xStart = frameOrigin.GetX() + lineOrigin[i].x + primaryOffset;
					isStartLine = true;
				}
			}
			
			//is it end line ?
			if (i == numLines-1 
				&&
				(inEnd >= lineRange.location && inEnd <= lineRange.location+lineRange.length)
				)
				gotEnd = true;
			else if (inEnd >= lineRange.location && inEnd < lineRange.location+lineRange.length)
				gotEnd = true;
			else if (inEnd < lineRange.location)
			{
				if (i > 0)
				{
					i--;
					line = (CTLineRef) CFArrayGetValueAtIndex(lines, i);
					lineRange = CTLineGetStringRange(line);
				}
				gotEnd = true;
			}
			
			if (gotEnd)
			{
				if (inEnd < lineRange.location)
				{
					inEnd = lineRange.location;
				} else if (inEnd > lineRange.location+lineRange.length)
				{
					inEnd = lineRange.location+lineRange.length;
				}

				CGFloat secondaryOffset;
				CGFloat primaryOffset = CTLineGetOffsetForStringIndex( line, inEnd, &secondaryOffset);
				
				xEnd = frameOrigin.GetX() + lineOrigin[i].x + primaryOffset;
				isEndLine = true;
			}
			
			if (gotStart)
			{
				
				CGFloat ascent, descent, externalLeading;
				GReal width = (GReal)CTLineGetTypographicBounds(line, &ascent,  &descent, &externalLeading);

				GReal y = ceil(frameOrigin.GetY() + lineOrigin[i].y + ascent + externalLeading);
				GReal height = ceil(externalLeading+ascent+descent);

				if (i > 0)
				{
					CTLineRef linePrev = (CTLineRef) CFArrayGetValueAtIndex(lines, i-1);
					CTLineGetTypographicBounds(linePrev, &ascent,  &descent, &externalLeading);
					GReal extraLeading = ceil(frameOrigin.GetY() + lineOrigin[i-1].y + ascent + externalLeading)-ceil(ascent+descent+externalLeading)-y;
					if (extraLeading != 0.0f && height+extraLeading >= 0)
					{
						y += extraLeading;
						height += extraLeading;
					}
				}
				
				y = fQDBounds.size.height-y;
				
				GReal x = isStartLine ? xStart : frameOrigin.GetX() + lineOrigin[i].x;
				width = isEndLine ? xEnd-x : frameOrigin.GetX() + lineOrigin[i].x + width - x;
				
				outRunBounds.push_back( VRect(x, y, width, height));
				if (gotEnd)
					break;
			}
		}
	}

    if (doRestoreContext)
        RestoreContext();
    
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
void VMacQuartzGraphicContext::_GetTextLayoutCaretMetricsFromCharIndex( VTextLayout *inTextLayout, const VPoint& inTopLeft, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading, const bool inCaretUseCharMetrics)
{
	inTextLayout->BeginUsingContext(this, true);
	if (!testAssert(inTextLayout->fLayoutIsValid))
	{
		inTextLayout->EndUsingContext();
		outCaretPos = inTopLeft;
		outTextHeight = 0.0f;
		return;
	}

	xbox_assert(inTextLayout->fTextBox);
	
	bool leading = inCaretLeading;
	VIndex charIndex = inCharIndex;
	if (inCharIndex >= inTextLayout->fTextLength)
	{
		if (inTextLayout->fTextLength == 0)
			charIndex = 0;
		leading = true;
	}
	else if (inCharIndex < inTextLayout->fTextLength && (inTextLayout->GetText())[inCharIndex] == 13)
		//fix for carriage return: ensure we return leading position 
		//(in CoreText, carriage return has a non-zero width which is very annoying because it is taken account too for text box measures:
		// there is no easy drawback but to ensure we do not draw caret on the trailing side) 
		leading = true;
	
    bool doRestoreContext = false;
    if (fAxisInverted)
    {
        doRestoreContext = true;
        SaveContext();
        _UseQuartzAxis();
    }

	outCaretPos.SetPos(inTopLeft.GetX(), fQDBounds.size.height-inTopLeft.GetY()); //default output caret pos in Quartz coord space
	outTextHeight = 0.0f;

	VPoint frameOrigin;
	frameOrigin.SetX(inTopLeft.GetX()+inTextLayout->fMargin.left);
	frameOrigin.SetY(fQDBounds.size.height - (inTopLeft.GetY()+inTextLayout->fMargin.top+inTextLayout->_GetLayoutHeightMinusMargin()));
		
	XMacStyledTextBox *textBox = static_cast<XMacStyledTextBox *>(inTextLayout->fTextBox);
	if (textBox)
	{
		//get frame 
		CTFrameRef frame = textBox->GetCurLayoutFrame( inTextLayout->_GetLayoutWidthMinusMargin(), inTextLayout->_GetLayoutHeightMinusMargin());
        frameOrigin += textBox->GetLayoutOffset();
		
		CGPathRef framePath = CTFrameGetPath(frame);
		
		CFArrayRef lines = CTFrameGetLines(frame);
		CFIndex numLines = CFArrayGetCount(lines);
		
		if (numLines)
		{
			//get line origins
			CGPoint lineOrigin[numLines];
			CTFrameGetLineOrigins(frame, CFRangeMake(0, 0), lineOrigin); //line origin is aligned on line baseline

			for(CFIndex i = 0; i < numLines; i++) //lines or ordered from the top line to the bottom line
			{
				CTLineRef line = (CTLineRef) CFArrayGetValueAtIndex(lines, i);
				CFRange lineRange = CTLineGetStringRange(line);

				bool doGotIt = false;
				if (i == numLines-1) 
					doGotIt = true;
				else if (charIndex >= lineRange.location && charIndex < lineRange.location+lineRange.length)
					doGotIt = true;
				else if (charIndex < lineRange.location)
					{
						if (i > 0)
						{
							i--;
							line = (CTLineRef) CFArrayGetValueAtIndex(lines, i);
							lineRange = CTLineGetStringRange(line);
						}
						doGotIt = true;
					}
				if (doGotIt)
				{
					if (charIndex < lineRange.location)
					{
						charIndex = lineRange.location;
						leading = true;
					} else if (charIndex >= lineRange.location+lineRange.length)
					{
						charIndex = lineRange.location+lineRange.length;
						leading = true;
					}

					
					CGFloat secondaryOffset;
					CGFloat primaryOffset = (leading || (charIndex+1) > inTextLayout->fTextLength) ?  CTLineGetOffsetForStringIndex( line, charIndex, &secondaryOffset) : CTLineGetOffsetForStringIndex( line, charIndex+1, &secondaryOffset);
					
					outCaretPos.SetX( frameOrigin.GetX() + lineOrigin[i].x + primaryOffset);

					CGFloat ascent, descent, externalLeading;
					CTLineGetTypographicBounds(line, &ascent,  &descent, &externalLeading);
					if (inCaretUseCharMetrics)
						externalLeading = 0;

					outCaretPos.SetY( ceil(frameOrigin.GetY() + lineOrigin[i].y + ascent + externalLeading));

					outTextHeight = ceil(externalLeading+ascent+descent);
					
					if (!inCaretUseCharMetrics && i > 0)
					{
						CTLineRef linePrev = (CTLineRef) CFArrayGetValueAtIndex(lines, i-1);
						CTLineGetTypographicBounds(linePrev, &ascent,  &descent, &externalLeading);
						GReal extraLeading = ceil(frameOrigin.GetY() + lineOrigin[i-1].y + ascent + externalLeading)-ceil(ascent+descent+externalLeading)-outCaretPos.GetY();
						if (extraLeading != 0.0f && outTextHeight+extraLeading >= 0)
						{
							outCaretPos.SetY(outCaretPos.GetY()+extraLeading);
							outTextHeight += extraLeading;
						}
					}
					break;
				}
			}
		}
	}

	outCaretPos.SetY( fQDBounds.size.height-outCaretPos.GetY());

    if (doRestoreContext)
        RestoreContext();
    
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
bool VMacQuartzGraphicContext::_GetTextLayoutCharIndexFromPos( VTextLayout *inTextLayout, const VPoint& inTopLeft, const VPoint& inPos, VIndex& outCharIndex)
{
	if (inTextLayout->fTextLength == 0)
	{
		outCharIndex = 0;
		return false;
	}

	inTextLayout->BeginUsingContext(this, true);
	if (!testAssert(inTextLayout->fLayoutIsValid))
	{
		inTextLayout->EndUsingContext();
		outCharIndex = 0;
		return false;
	}

	xbox_assert(inTextLayout->fTextBox);
	
    bool doRestoreContext = false;
    if (fAxisInverted)
    {
        doRestoreContext = true;
        SaveContext();
        _UseQuartzAxis();
    }

	VPoint frameOrigin;
	frameOrigin.SetX(inTopLeft.GetX()+inTextLayout->fMargin.left);
	frameOrigin.SetY(fQDBounds.size.height - (inTopLeft.GetY()+inTextLayout->fMargin.top+inTextLayout->_GetLayoutHeightMinusMargin()));

	VPoint pos( inPos);
	pos.SetY(fQDBounds.size.height-inPos.GetY());

	outCharIndex = 0;

	XMacStyledTextBox *textBox = static_cast<XMacStyledTextBox *>(inTextLayout->fTextBox);
	if (textBox)
	{
		//get frame 
		CTFrameRef frame = textBox->GetCurLayoutFrame( inTextLayout->_GetLayoutWidthMinusMargin(), inTextLayout->_GetLayoutHeightMinusMargin());
        frameOrigin += textBox->GetLayoutOffset();

		CGPathRef framePath = CTFrameGetPath(frame);
		
		CFArrayRef lines = CTFrameGetLines(frame);
		CFIndex numLines = CFArrayGetCount(lines);
		
		if (numLines)
		{
			//get line origins
			CGPoint lineOrigin[numLines];
			CTFrameGetLineOrigins(frame, CFRangeMake(0, 0), lineOrigin); //line origin is aligned on line baseline
			CTLineRef linePrev = NULL;
			GReal yLinePrev;
			GReal heightLinePrev;
			
			for(CFIndex i = 0; i < numLines; i++) //lines or ordered from the top line to the bottom line
			{
				CTLineRef line = (CTLineRef) CFArrayGetValueAtIndex(lines, i);
				CGFloat ascent, descent, leading;
				CTLineGetTypographicBounds(line, &ascent,  &descent, &leading);

				GReal y = ceil(frameOrigin.GetY() + lineOrigin[i].y + ascent + leading);
				GReal height = ceil(leading+ascent+descent);
				
				if (linePrev)
				{
					GReal extraLeading = yLinePrev-heightLinePrev-y;
					if (extraLeading != 0.0f && height+extraLeading >= 0)
					{
						y += extraLeading;
						height += extraLeading;
					}
				}
				
				if (pos.GetY() > y)
				{
					if (i > 0)
					{
						i--;
						line = linePrev;
					}
				}
				else
				{
					linePrev = line;
					yLinePrev = y;
					heightLinePrev = height;

					if (i < numLines-1)
						continue;
				}

				//get line character index from mouse position
				CGPoint offset = { pos.GetX()-lineOrigin[i].x-frameOrigin.GetX(), pos.GetY()-lineOrigin[i].y-frameOrigin.GetY() };
				CFIndex indexChar = CTLineGetStringIndexForPosition(line, offset);
				CFRange lineRange = CTLineGetStringRange(line);
				if (indexChar > inTextLayout->fTextLength)
					indexChar = inTextLayout->fTextLength;
				else if (indexChar >= lineRange.location+lineRange.length 
					&& indexChar > 0 
					&& indexChar-1 >= lineRange.location 
					&& (inTextLayout->GetText())[indexChar-1] == 13)
					indexChar--;
				if (indexChar == kCFNotFound)
					//text search is not supported
					indexChar = lineRange.location;
				
				outCharIndex = indexChar;
				break;
			}
		}
	}

    if (doRestoreContext)
        RestoreContext();
    
	inTextLayout->EndUsingContext();
	return true;
}


/* update text layout
@remarks
	build text layout according to layout settings & current graphic context settings

	this method is called only from VTextLayout::_UpdateTextLayout
*/
void VMacQuartzGraphicContext::_UpdateTextLayout( VTextLayout *inTextLayout)
{
	xbox_assert( inTextLayout->fGC == static_cast<VGraphicContext *>(this));

	if (inTextLayout->fTextBox)
	{
		if (inTextLayout->fLayoutBoundsDirty)
		{
			//only sync layout & formatted bounds

			XMacStyledTextBox *textBox = dynamic_cast<XMacStyledTextBox *>(inTextLayout->fTextBox);
			if (testAssert(textBox))
			{
				VRect bounds(0.0f, 0.0f, inTextLayout->fMaxWidth ? inTextLayout->fMaxWidth : 100000.0f, inTextLayout->fMaxHeight ? inTextLayout->fMaxHeight : 100000.0f);
				GReal width = bounds.GetWidth(), height = bounds.GetHeight();
				textBox->GetSize(width, height);

				//store typo text metrics (formatted text metrics)
				inTextLayout->fCurWidth = width;
				inTextLayout->fCurHeight = height;

				//store text layout metrics (including overhangs)
				//here we ensure layout metrics are not lower than typo metrics
				inTextLayout->fCurLayoutWidth = inTextLayout->fMaxWidth  && inTextLayout->fMaxWidth > width ? std::floor(inTextLayout->fMaxWidth) : width; //might be ~0 otherwise it is always rounded yet
				inTextLayout->fCurLayoutHeight = inTextLayout->fMaxHeight ? std::floor(inTextLayout->fMaxHeight) : height; //might be ~0 otherwise it is always rounded yet

				//store text layout overhang metrics (with CoreText, overhangs are included in typo metrics yet so no need to deal with it here)
				inTextLayout->fCurOverhang.left = 0;
				inTextLayout->fCurOverhang.right = 0;
				inTextLayout->fCurOverhang.top = 0;
				inTextLayout->fCurOverhang.bottom = 0;

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
			return;
		}
	}
	ReleaseRefCountable(&(inTextLayout->fTextBox));

	inTextLayout->fLayoutBoundsDirty = false;

	//apply custom alignment 

	//compute layout & determine actual bounds
	VRect bounds(0.0f, 0.0f, inTextLayout->fMaxWidth ? inTextLayout->fMaxWidth : 100000.0f, inTextLayout->fMaxHeight ? inTextLayout->fMaxHeight : 100000.0f);

	ContextRef contextRef = BeginUsingParentContext();
	
	VStyledTextBox *textBox = VStyledTextBox::CreateImpl( contextRef, contextRef, *inTextLayout, &bounds, GetTextRenderingMode(), NULL, NULL, fCharKerning+fCharSpacing);
	
	EndUsingParentContext(contextRef);

	if (textBox)
	{
		inTextLayout->fTextBox = static_cast<VStyledTextBox *>(textBox);

		GReal width = bounds.GetWidth(), height = bounds.GetHeight();
		textBox->GetSize(width, height);

		//store typo text metrics (formatted text metrics)
		inTextLayout->fCurWidth = width;
		inTextLayout->fCurHeight = height;

		//store text layout metrics (including overhangs)
		//here we ensure layout metrics are not lower than typo metrics
		inTextLayout->fCurLayoutWidth = inTextLayout->fMaxWidth  && inTextLayout->fMaxWidth > width ? std::floor(inTextLayout->fMaxWidth) : width; //might be ~0 otherwise it is always rounded yet
        inTextLayout->fCurLayoutHeight = inTextLayout->fMaxHeight ? std::floor(inTextLayout->fMaxHeight) : height; //might be ~0 otherwise it is always rounded yet

		//store text layout overhang metrics (with CoreText, overhangs are included in typo metrics yet so no need to deal with it here)
		inTextLayout->fCurOverhang.left = 0;
		inTextLayout->fCurOverhang.right = 0;
		inTextLayout->fCurOverhang.top = 0;
		inTextLayout->fCurOverhang.bottom = 0;

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
}

/** internally called while replacing text in order to update impl-specific stuff */
void VMacQuartzGraphicContext::_PostReplaceText( VTextLayout *inTextLayout)
{
	if (inTextLayout->fTextLength != 0)
	{
        //appending a dummy CR seems to be a good workaround to avoid text wobble CoreText bug while mixing japanese/chinese text & latin:
        //as this last CR is ignored by CoreText layout metrics it does not change layout size too;
        //we use fTextLength to backup the actual text length
		inTextLayout->fText.AppendChar(13);
	}
}


#pragma mark-

VMacQuartzOffscreenContext::VMacQuartzOffscreenContext(const VMacQuartzGraphicContext& inSourceContext) : VMacQuartzGraphicContext(inSourceContext)
{
	_Init();
	
	fSrcContext = fContext;
	fContext = NULL;
}


VMacQuartzOffscreenContext::VMacQuartzOffscreenContext(CGContextRef inSourceContext, Boolean inReversed, const CGRect& inQDBounds) : VMacQuartzGraphicContext(inSourceContext, inReversed, inQDBounds)
{
	_Init();
	
	fSrcContext = fContext;
	fContext = NULL;
}


VMacQuartzOffscreenContext::VMacQuartzOffscreenContext(const VMacQuartzOffscreenContext& inOriginal) : VMacQuartzGraphicContext(inOriginal)
{
	_Init();
	
	fSrcContext = fContext;
	fContext = NULL;
}


VMacQuartzOffscreenContext::~VMacQuartzOffscreenContext()
{
	_Dispose();
}


void VMacQuartzOffscreenContext::_Init()
{
	fSrcContext = NULL;
	fTempFile = NULL;
   	fPageClosed = true;
}


void VMacQuartzOffscreenContext::_Dispose()
{	
	// May delete the file
	delete fTempFile;
	
	if (fContext != NULL)
		::CGContextRelease(fContext);
}


void VMacQuartzOffscreenContext::CopyContentTo(VGraphicContext* inDestContext, const VRect* inSrcBounds, const VRect* inDestBounds, TransferMode inMode, const VRegion* inMask)
{
	if (!testAssert(fContext != NULL)) return;
	if (!testAssert(inDestContext != NULL)) return;
	
	// Close the current page
	if (!fPageClosed)
	{
		::CGContextEndPage(fContext);
		::CGContextRelease(fContext);
		fPageClosed = true;
		fContext = NULL;
	}
	
	// Open the PDF document
	CGPDFDocumentRef	document = MAC_RetainPDFDocument();
	CGPDFPageRef	pageRef = ::CGPDFDocumentGetPage(document, 1);
	if (testAssert(pageRef != NULL))
	{
		// May need to scale drawing rectangle
		CGFloat	srcX = inSrcBounds != NULL ? inSrcBounds->GetX() : 0;
		CGFloat	srcY = inSrcBounds != NULL ? inSrcBounds->GetY() : 0;
		CGRect	pdfBounds = ::CGPDFPageGetBoxRect(pageRef, kCGPDFMediaBox);
		CGRect	drawBounds = CGRectMake(srcX, srcY, pdfBounds.size.width, pdfBounds.size.height);

		if (inDestBounds != NULL)
		{
			CGFloat	scaleX = inDestBounds->GetWidth() / (inSrcBounds != NULL ? inSrcBounds->GetWidth() : pdfBounds.size.width);
			CGFloat	scaleY = inDestBounds->GetHeight() / (inSrcBounds != NULL ? inSrcBounds->GetHeight() : pdfBounds.size.height);
			drawBounds.origin = CGPointMake(inDestBounds->GetX() - srcX, inDestBounds->GetY() - srcY);
			
			// May need to scale the rectangle
			if (Abs(scaleX - 1.0) > kREAL_PIXEL_PRECISION || Abs(scaleY - 1.0) > kREAL_PIXEL_PRECISION)
			{
				CGFloat	deltaX = inDestBounds->GetX() - srcX * scaleX;
				CGFloat	deltaY = inDestBounds->GetY() - srcY * scaleY;
				drawBounds = CGRectMake(deltaX, deltaY, pdfBounds.size.width * scaleX, pdfBounds.size.height * scaleY);
			}
		}

		// Clip and draw
		StUseContext_NoRetain	context(inDestContext);
		
		::CGContextSaveGState(context);
		
		if (inDestBounds != NULL)
			::CGContextClipToRect(context, *inDestBounds);
		
		if (inMask != NULL)
			const_cast<VGraphicContext*>(inDestContext)->ClipRegion(*inMask, true);	// const_cast because of CGContextSave/RestoreGState
		
		if (inMode == TM_BLEND)
			::CGContextSetAlpha(context, ((CGFloat) fAlphaBlend) / 255);
		
		CGAffineTransform	transform = ::CGPDFPageGetDrawingTransform(pageRef, kCGPDFMediaBox, drawBounds, 0, false);
		
		::CGContextConcatCTM(context, transform);
		::CGContextDrawPDFPage(context, pageRef);
		::CGContextRestoreGState(context);
		
		::CGPDFPageRelease(pageRef);
	}
	
	::CGPDFDocumentRelease(document);
}


Boolean VMacQuartzOffscreenContext::CreatePicture(const VRect& inHwndBounds)
{
	// Release previous if any
	if (fContext != NULL)
		::CGContextRelease(fContext);
		
	CFURLRef	tempURL = NULL;
	
	// Create a storage file in the system's 'temp' folder
	if (fTempFile == NULL)
	{
		//VFolder	tempFolder(eFK_WorkingFolder);
		VFolder	tempFolder(L"Sources:perforce:GoldFinger:Targets:Debug:");	// TEMP fix for a bug in VFolder
		VStr15	tempFileName;
		VUUID	randomID;
		
		do {
			randomID.Regenerate();
			randomID.GetString(tempFileName);
			tempFileName.Truncate(30);	// TEMP fix for a bug in VFolder
			fTempFile = new VFile(tempFolder, tempFileName);
		} while (fTempFile->Exists());
		
		fTempFile->Create( false);
	}
	
	// Open a PDF context using the file as document
	FSRef	fileRef;
	if (fTempFile != NULL && fTempFile->MAC_GetFSRef( &fileRef))
		tempURL = ::CFURLCreateFromFSRef(NULL, &fileRef);
	
	VRect	contextBounds(inHwndBounds);
	contextBounds.SetPosBy(-inHwndBounds.GetX(), -inHwndBounds.GetY());
	
    if (testAssert(tempURL != NULL))
	{
		CGRect r = contextBounds;
		fContext = ::CGPDFContextCreateWithURL(tempURL, &r, NULL);
		if (testAssert(fContext != NULL))
		{
	   		::CGContextBeginPage(fContext, &r);
			::CGContextTranslateCTM(fContext, -inHwndBounds.GetX(), -inHwndBounds.GetY());
	   		fPageClosed = false;
		}
		
   		::CFRelease(tempURL);
   	}
	
	return (fContext != NULL);
}


void VMacQuartzOffscreenContext::ReleasePicture()
{
	_Dispose();
	
	fContext = NULL;
	fTempFile = NULL;
	fPageClosed = true;
}


CGPDFDocumentRef VMacQuartzOffscreenContext::MAC_RetainPDFDocument () const
{
	CGPDFDocumentRef	documentRef = NULL;
	CFURLRef	tempURL = NULL;
	
	FSRef	fileRef;
	if (fTempFile != NULL && fTempFile->MAC_GetFSRef( &fileRef))
		tempURL = ::CFURLCreateFromFSRef(NULL, &fileRef);
	
    if (testAssert(tempURL != NULL))
	{
		documentRef = ::CGPDFDocumentCreateWithURL(tempURL);
   		::CFRelease(tempURL);
   	}
   	
	return documentRef;
}


#pragma mark-


/** create bitmap context from scratch */
VMacQuartzBitmapContext::VMacQuartzBitmapContext(const VRect& inBounds, Boolean inReversed,bool inTransparent):VMacQuartzGraphicContext()
{
	_Init();
	
	fMaxPerfFlag = false;
	fFillRule = FILLRULE_EVENODD;
	fCharKerning = (GReal) 0.0;
	fCharSpacing = (GReal) 0.0;
	
	CreateBitmap( inBounds,inTransparent);
	fSrcContext = fContext;
	
#if VERSIONDEBUG
	CGAffineTransform ctm = CGContextGetCTM(fContext);
#endif
	
	fAxisInverted = true;
	inBounds.MAC_ToCGRect( fQDBounds);
	CGContextSetFlatness(fContext, 2);
	CGContextSetInterpolationQuality(fContext, kCGInterpolationLow);
	
	if (!inReversed)
		_UseQuartzAxis();
}

VMacQuartzBitmapContext::VMacQuartzBitmapContext(const VMacQuartzGraphicContext& inSourceContext) : VMacQuartzGraphicContext(inSourceContext)
{
	_Init();
	
	fSrcContext = fContext;
	fContext = NULL;
}

VMacQuartzBitmapContext::VMacQuartzBitmapContext() : VMacQuartzGraphicContext()
{
	_Init();
	
	fSrcContext = NULL;
	fContext = NULL;
}

VMacQuartzBitmapContext::VMacQuartzBitmapContext(CGContextRef inSourceContext, Boolean inReversed, const CGRect& inQDBounds) : VMacQuartzGraphicContext(inSourceContext, inReversed, inQDBounds)
{
	_Init();
	
	fSrcContext = fContext;
	fContext = NULL;
}


VMacQuartzBitmapContext::VMacQuartzBitmapContext(const VMacQuartzBitmapContext& inOriginal) : VMacQuartzGraphicContext(inOriginal)
{
	_Init();
	
	fSrcContext = fContext;
	fContext = NULL;
}


VMacQuartzBitmapContext::~VMacQuartzBitmapContext()
{

	_Dispose();
	
	if(fSrcContext)
		fContext = fSrcContext;
}


void VMacQuartzBitmapContext::_Init()
{
	fSrcContext = NULL;
	fBitmapData = NULL;
	fImageCache = NULL;
}


void VMacQuartzBitmapContext::_Dispose()
{
	VMemory::DisposePtr((VPtr)fBitmapData);
	::CGImageRelease(fImageCache);
}


void VMacQuartzBitmapContext::_CreateImageCache() const
{
	// Create an image for the bitmap
	if (fContext != NULL && fImageCache == NULL)
	{
		#if 0
		size_t	width = ::CGBitmapContextGetWidth(fContext);
		size_t	height = ::CGBitmapContextGetHeight(fContext);
		size_t	bytesPerRow = ::CGBitmapContextGetBytesPerRow(fContext);
		size_t	totalBytes = height * bytesPerRow;
	
		CGDataProviderRef	provider = ::CGDataProviderCreateWithData(NULL, (void*) fBitmapData, totalBytes, NULL);
		if (testAssert(provider != NULL))
		{
			CGColorSpaceRef	colorSpace = ::CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
			fImageCache = ::CGImageCreate(width, height, 8, 32, bytesPerRow, colorSpace, kCGImageAlphaPremultipliedFirst, provider, NULL, TRUE, kCGRenderingIntentDefault);
			::CGColorSpaceRelease(colorSpace);
			::CGDataProviderRelease(provider);
		}
		#endif
		fImageCache = CGBitmapContextCreateImage(fContext);
	}
}


void VMacQuartzBitmapContext::CopyContentTo(VGraphicContext* inDestContext, const VRect* inSrcBounds, const VRect* inDestBounds, TransferMode inMode, const VRegion* inMask)
{
	if (!testAssert(fContext != NULL)) return;
	if (!testAssert(inDestContext != NULL)) return;
	
	_CreateImageCache();
	
	// Draw image in the destination context
	if (testAssert(fImageCache != NULL))
	{
		StUseContext_NoRetain	context(inDestContext);
		
		inDestContext->SaveContext();
		
				
		// Compute drawing rectangle
		CGFloat	imageWidth = ::CGImageGetWidth(fImageCache);
		CGFloat	imageHeight = ::CGImageGetHeight(fImageCache);
		CGFloat	srcX = inSrcBounds != NULL ? inSrcBounds->GetX() : 0;
		CGFloat	srcY = inSrcBounds != NULL ? inSrcBounds->GetY() : 0;
		CGRect	drawBounds = CGRectMake(srcX, srcY, imageWidth, imageHeight);

		if (inDestBounds != NULL)
		{
			CGFloat	scaleX = inDestBounds->GetWidth() / (inSrcBounds != NULL ? inSrcBounds->GetWidth() : imageWidth);
			CGFloat	scaleY = inDestBounds->GetHeight() / (inSrcBounds != NULL ? inSrcBounds->GetHeight() : imageHeight);
			drawBounds.origin = CGPointMake(inDestBounds->GetX() - srcX, inDestBounds->GetY() - srcY);
			
			// May need to scale the rectangle
			if (Abs(scaleX - 1) > kREAL_PIXEL_PRECISION || Abs(scaleY - 1.0) > kREAL_PIXEL_PRECISION)
			{
				CGFloat	deltaX = inDestBounds->GetX() - srcX * scaleX;
				CGFloat	deltaY = inDestBounds->GetY() - srcY * scaleY;
				drawBounds = CGRectMake(deltaX, deltaY, imageWidth * scaleX, imageHeight * scaleY);
			}
		}
		
		if(inDestBounds)
			inDestContext->ClipRect(*inDestBounds);
		
		VRect DrawBounds;
		
		DrawBounds.SetCoords(drawBounds.origin.x,drawBounds.origin.y,drawBounds.size.width,drawBounds.size.height);
		
		inDestContext->UseEuclideanAxis();
		
		VPictureData_CGImage pict(fImageCache);
		VPictureDrawSettings set;
		inDestContext->DrawPictureData(&pict,DrawBounds,&set);
		inDestContext->RestoreContext();
		CFRelease(fImageCache);
		fImageCache=NULL;
		/*
		
		// Clip and draw
		::CGContextSaveGState(context);
		
		if (inDestBounds != NULL)
		{
			VRect DestBounds=*inDestBounds;
			DestBounds.SetY( inDestContext->fQDBounds.size.height - (DestBounds.GetY() + DestBounds.GetHeight())); 
			::CGContextClipToRect(context,DestBounds);
		}
		//if (inMask != NULL)
		//	const_cast<VGraphicContext*>(inDestContext)->ClipRegion(*inMask, true);	// const_cast because of CGContextSave/RestoreGState
		
		if (inMode == TM_BLEND)
			::CGContextSetAlpha(context, ((CGFloat) fAlphaBlend) / 255);
			
		drawBounds.origin.y = inDestContext->fQDBounds.size.height -(drawBounds.origin.y + drawBounds.size);
		::CGContextDrawImage(context, drawBounds, fImageCache);
		context.RestoreContext();*/
	}
}
VPictureData* VMacQuartzBitmapContext::CopyContentToPictureData()
{
	VPictureData* result=NULL;
	_CreateImageCache();
	
	// Draw image in the destination context
	if (testAssert(fImageCache != NULL))
	{
		result = new VPictureData_CGImage(fImageCache);
		CFRelease(fImageCache);
		fImageCache=NULL;
	}
	return result;
}

void VMacQuartzBitmapContext::FillUniformColor(const VPoint& /*inContextPos*/, const VColor& /*inColor*/, const VPattern* /*inPattern*/)
{
	if (!testAssert(fContext != NULL)) return;
	
	// TBD
}


void VMacQuartzBitmapContext::SetPixelColor(const VPoint& inContextPos, const VColor& inColor)
{
	if (!testAssert(fContext != NULL)) return;
	
	assert(fBitmapData != NULL);
	
	size_t	width = (size_t)(inContextPos.GetX());
	size_t	height = (size_t)(inContextPos.GetY());
	if (width < 0 || height < 0) return;
	
	size_t	bytesPerRow = ::CGBitmapContextGetBytesPerRow(fContext);
	size_t	pixelOffset = height * bytesPerRow + width*4;
	uBYTE*	pixelPtr = fBitmapData + pixelOffset;
	
	assert(::CGBitmapContextGetAlphaInfo(fContext) == kCGImageAlphaPremultipliedFirst);
	
	*pixelPtr++ = inColor.GetAlpha();
	*pixelPtr++ = inColor.GetRed();
	*pixelPtr++ = inColor.GetGreen();
	*pixelPtr++ = inColor.GetBlue();
}


VColor* VMacQuartzBitmapContext::GetPixelColor(const VPoint& inContextPos) const
{
	if (!testAssert(fContext != NULL)) return NULL;
	
	assert(fBitmapData != NULL);
	
	size_t	width = (size_t)(inContextPos.GetX());
	size_t	height = (size_t)(inContextPos.GetY());
	if (width < 0 || height < 0) return NULL;
	
	size_t	bytesPerRow = ::CGBitmapContextGetBytesPerRow(fContext);
	size_t	pixelOffset = height * bytesPerRow + width*4;
	uBYTE*	pixelPtr = fBitmapData + pixelOffset;
	
	assert(::CGBitmapContextGetAlphaInfo(fContext) == kCGImageAlphaPremultipliedFirst);
	
	uBYTE	alpha = *pixelPtr++;
	uBYTE	red = *pixelPtr++;
	uBYTE	green = *pixelPtr++;
	uBYTE	blue = *pixelPtr++;
	
	return new VColor(red, green, blue, alpha);
}


Boolean VMacQuartzBitmapContext::CreateBitmap(const VRect& inHwndBounds,bool inTransparent)
{
	size_t	width = (size_t)(inHwndBounds.GetWidth());
	size_t	height = (size_t)(inHwndBounds.GetHeight());
	size_t	bytesPerRow = width * 4;
	size_t	totalBytes = height * bytesPerRow;
	
	// Try to keep existing buffer (speed optimization when size is increasing slightly)
	if (fBitmapData != NULL)
		VMemory::DisposePtr((VPtr)fBitmapData);
	
	fBitmapData = (uBYTE*) VMemory::NewPtr(totalBytes,'bitm');
	if(fBitmapData)
	{
		if(inTransparent)
			::memset(fBitmapData,0x00,totalBytes);
		else
			::memset(fBitmapData,0xff,totalBytes);
	}

	// If width or height changed, we cannot keep the old context
	if (fContext != NULL && (::CGBitmapContextGetWidth(fContext) != width || ::CGBitmapContextGetHeight(fContext) != height))
	{
		::CGImageRelease(fImageCache);
		fImageCache = NULL;
		
		RestoreContext();
		
		::CGContextRelease(fContext);
		fContext = NULL;
	}
	
	// Create the context if needed
	if (fContext == NULL)
	{
		CGColorSpaceRef	colorSpace = ::CGColorSpaceCreateDeviceRGB();
		fContext = ::CGBitmapContextCreate(fBitmapData, width, height, 8, bytesPerRow, colorSpace, inTransparent ? kCGImageAlphaPremultipliedFirst : kCGImageAlphaNoneSkipFirst);
		::CGColorSpaceRelease(colorSpace);
		if(fContext!=NULL)
			SaveContext();
	}
	
	inHwndBounds.MAC_ToCGRect( fQDBounds);
	fQDBounds.origin.x=0;
	fQDBounds.origin.y=0;
	fQDBounds.size.width+=inHwndBounds.GetX();
	fQDBounds.size.height+=inHwndBounds.GetY();
	UseReversedAxis();
	
	return (fContext != NULL);
}


void VMacQuartzBitmapContext::ReleaseBitmap()
{
	if (fContext != NULL)
	{
		::CGImageRelease(fImageCache);
		fImageCache = NULL;
		
		RestoreContext();
		
		::CGContextRelease(fContext);
		fContext = NULL;
		
		VMemory::DisposePtr((VPtr)fBitmapData);
		fBitmapData = NULL;
	}
	if(fSrcContext)
		fContext = fSrcContext;
}


CGImageRef VMacQuartzBitmapContext::MAC_RetainCGImage () const
{
	_CreateImageCache();
	
	if (fImageCache != NULL)
		::CGImageRetain(fImageCache);
	
	return fImageCache;
}



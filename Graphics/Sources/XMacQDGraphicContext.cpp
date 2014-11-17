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
#include "VQuickTimeSDK.h"
#include "V4DPictureIncludeBase.h"
#include "XMacQDGraphicContext.h"
#include "VRect.h"
#include "VRegion.h"
#include "VIcon.h"

#include "VPattern.h"
#include "VPolygon.h"
#include <Quicktime/ImageCompression.h>


const Real	kSMALLEST_TXN_WIDTH		= 8.0;	// This value is in fact the size of the larger
const Real	kSMALLEST_TXN_HEIGHT	= 8.0;	// letter to render with word-wrap enabled

const RGBColor	kWHITE_COLOR	= { 0xFFFF, 0xFFFF, 0xFFFF };
const RGBColor	kBLACK_COLOR	= { 0x0000, 0x0000, 0x0000 };

const sLONG	kREVEAL_CLIPPING_PATTERN		= 128;
const sLONG	kREVEAL_UPDATE_PATTERN			= 128;
const sLONG	kREVEAL_BLITTING_PATTERN		= 129;
const sLONG	kREVEAL_INVAL_PATTERN			= 130;

const uLONG	kDEBUG_REVEAL_CLIPPING_DELAY	= 10;
const uLONG	kDEBUG_REVEAL_UPDATE_DELAY		= 16;
const uLONG	kDEBUG_REVEAL_BLITTING_DELAY	= 16;
const uLONG	kDEBUG_REVEAL_INVAL_DELAY		= 10;


// Static
Boolean VMacQDGraphicContext::Init()
{
	::SetFractEnable(true);
	::SetPreserveGlyph(true);
	
	return true;
}


void VMacQDGraphicContext::DeInit()
{
}


#pragma mark-

VMacQDGraphicContext::VMacQDGraphicContext(GrafPtr inContext)
{
	_Init();
	
	fContext = inContext;
}


VMacQDGraphicContext::VMacQDGraphicContext(const VMacQDGraphicContext& inOriginal) : VGraphicContext(inOriginal)
{
	_Init();
	
	fContext = inOriginal.fContext;
}


VMacQDGraphicContext::~VMacQDGraphicContext()
{
	_Dispose();
}


void VMacQDGraphicContext::_Init()
{
	fContext = NULL;
	fAlphaBlend = 0;
	fTextAngle = 0.0;
	fPixelForeColor = kBLACK_COLOR;
	fPixelBackColor = kWHITE_COLOR;
	fSavedPort = NULL;
	fPortRefcon = 0;
	fSavedContext = NULL;
	fClipStack = NULL;
}


void VMacQDGraphicContext::_Dispose()
{
	assert(fPortRefcon == 0 /* You didn't restore previous port */);
	
	delete fClipStack;
	delete fSavedContext;
}

void VMacQDGraphicContext::GetTransform(VAffineTransform &outTransform)
{
	// not supported
}
void VMacQDGraphicContext::SetTransform(const VAffineTransform &inTransform)
{
	// not supported
}
void VMacQDGraphicContext::ConcatTransform(const VAffineTransform &inTransform)
{
	// not supported
}
void VMacQDGraphicContext::RevertTransform(const VAffineTransform &inTransform)
{
	// not supported
}
void VMacQDGraphicContext::ResetTransform()
{
	// not supported
}
#pragma mark-

void VMacQDGraphicContext::SetFillColor(const VColor& inColor, VBrushFlags* ioFlags)
{
	StUseContext_NoRetain	context(this);
	::RGBForeColor(inColor);
	
	if (ioFlags != NULL)
		ioFlags->fLineBrushChanged = true;
}


void VMacQDGraphicContext::SetFillPattern(const VPattern* inPattern, VBrushFlags* ioFlags)
{
	if (inPattern == NULL) return;
	assert(inPattern->IsFillPattern());
	
	StUseContext_NoRetain	context(this);
	inPattern->ApplyToContext(this);
	
	if (ioFlags != NULL)
		ioFlags->fLineBrushChanged = true;
}


void VMacQDGraphicContext::SetLineColor(const VColor& inColor, VBrushFlags* ioFlags)
{
	StUseContext_NoRetain	context(this);
	::RGBForeColor(inColor);
	
	if (ioFlags != NULL)
		ioFlags->fFillBrushChanged = true;
}


void VMacQDGraphicContext::SetLinePattern(const VPattern* inPattern, VBrushFlags* ioFlags)
{
	if (inPattern == NULL) return;
	assert(inPattern->IsLinePattern());
	
	StUseContext_NoRetain	context(this);
	inPattern->ApplyToContext(this);
	
	if (ioFlags != NULL)
		ioFlags->fFillBrushChanged = true;
}


void VMacQDGraphicContext::SetLineWidth(Real inWidth, VBrushFlags* /*ioFlags*/)
{
	StUseContext_NoRetain	context(this);
	::PenSize((short)inWidth, (short)inWidth);
}


void VMacQDGraphicContext::SetLineStyle(CapStyle /*inCapStyle*/, JoinStyle /*inJoinStyle*/, VBrushFlags* /*ioFlags*/)
{
	// Not supported
}


void VMacQDGraphicContext::SetTextColor(const VColor& inColor, VBrushFlags* ioFlags)
{
	StUseContext_NoRetain	context(this);
	::RGBForeColor(inColor);
	
	if (ioFlags != NULL)
		ioFlags->fLineBrushChanged = true;
}

void VMacQDGraphicContext::GetTextColor (VColor& outColor)
{
	StUseContext_NoRetain	context(this);
	
	RGBColor  foreColor;
	::GetPortForeColor(fContext, &foreColor);
	
	outColor.FromColorRef(foreColor);
}

void VMacQDGraphicContext::SetBackColor(const VColor& inColor, VBrushFlags* /*ioFlags*/)
{
	StUseContext_NoRetain	context(this);
	
	const Pattern kWHITE_PATTERN = { 0, 0, 0, 0, 0, 0, 0, 0 };
	::BackPat(&kWHITE_PATTERN);
	::RGBBackColor(inColor);
}


DrawingMode VMacQDGraphicContext::SetDrawingMode(DrawingMode inMode, VBrushFlags* ioFlags)
{
	StUseContext_NoRetain	context(this);
	
	sWORD	mode;
	
	switch (inMode)
	{
		case DM_NORMAL:
			mode = patCopy;
			break;
			
		case DM_CLIP:
			mode = srcOr;
			break;
			
		case DM_HILITE:
			mode = patCopy;
			break;
			
		case DM_SHADOW:
			mode = patCopy;	// Not supported
			break;
		
		default:
			assert(false);
			break;
	}
	
	::PenMode(mode);
	
	// Set hilite mode
	mode = ::LMGetHiliteMode();
	::LMSetHiliteMode((inMode == DM_HILITE) ? mode & 0x7F : mode | 0x80);
	
	if (ioFlags != NULL)
		ioFlags->fDrawingModeChanged = true;
	
	return DM_NORMAL;
}


void VMacQDGraphicContext::SetTransparency(Real inAlpha)
{
	// Not supported
}


void VMacQDGraphicContext::SetShadowValue(Real /*inHOffset*/, Real /*inVOffset*/, Real /*inBlurness*/)
{
	// Not supported
}


void VMacQDGraphicContext::EnableAntiAliasing()
{
	// Not supported
}


void VMacQDGraphicContext::DisableAntiAliasing()
{
	// Not supported
}


bool VMacQDGraphicContext::IsAntiAliasingAvailable()
{
	return false;
}


void VMacQDGraphicContext::SetFont( VFont *inFont, Real inScale)
{
	StUseContext_NoRetain	context(this);
	
	::TextFont(inFont->GetFontRef());
	::TextSize((sWORD)(inFont->GetSize() * inScale));
	::TextFace(XMacFont::FontFaceToStyle(inFont->GetFace()));
}

VFont*	VMacQDGraphicContext::GetFont()
{
	StUseContext_NoRetain	context(this);
	
	VFont* result = NULL;
	VString fontName;
	Real	fontSize = 0;
	VFontFace fontface = 0;
	
	FMFontFamily	family = ::GetPortTextFont(fContext);
	sWORD			fontStyle = ::GetPortTextFace(fContext);
	
	XMacFont::FontNumToFontName (family, fontName);
	XMacFont::StyleToFontFace(fontStyle, fontface);
	fontSize = ::GetPortTextSize(fContext);
	
	result = new VFont(fontName,fontface,fontSize);
	return result;
}


void VMacQDGraphicContext::SetTextPosTo(const VPoint& inHwndPos)
{
	StUseContext_NoRetain	context(this);
	
	::MoveTo((short)inHwndPos.GetX(), (short)inHwndPos.GetY());
	fTextPos = inHwndPos;
}


void VMacQDGraphicContext::SetTextPosBy(Real inHoriz, Real inVert)
{
	StUseContext_NoRetain	context(this);
	
	::Move((short)inHoriz, (short)inVert);
	fTextPos.SetPosBy(inHoriz, inVert);
}


void VMacQDGraphicContext::SetTextAngle(Real inAngle)
{
	fTextAngle = inAngle;
}


DrawingMode VMacQDGraphicContext::SetTextDrawingMode(DrawingMode inMode, VBrushFlags* /*ioFlags*/)
{
	StUseContext_NoRetain	context(this);
	
	sWORD	mode;
	
	switch (inMode)
	{
		case DM_PLAIN:
			mode = patCopy;
			break;
			
		case DM_CLIP:
			mode = srcOr;
			break;
		
		default:
			assert(false);
			break;
	}
	
	::TextMode(mode);
	return DM_PLAIN;
}


void VMacQDGraphicContext::SetSpaceKerning(Real inSpaceExtra)
{
	StUseContext_NoRetain	context(this);
	::SpaceExtra(::X2Fix(inSpaceExtra));
}


void VMacQDGraphicContext::SetCharKerning(Real inSpaceExtra)
{
	StUseContext_NoRetain	context(this);
	::CharExtra(::X2Fix(inSpaceExtra));
}


uBYTE VMacQDGraphicContext::SetAlphaBlend(uBYTE inAlphaBlend)
{
	uBYTE	oldBlend = fAlphaBlend;
	fAlphaBlend = inAlphaBlend;
	return oldBlend;
}


void VMacQDGraphicContext::SetPixelForeColor(const VColor& inColor)
{
	fPixelForeColor = inColor;
}


void VMacQDGraphicContext::SetPixelBackColor(const VColor& inColor)
{
	fPixelBackColor = inColor;
}



#pragma mark-

Real VMacQDGraphicContext::GetTextWidth(const VString& inString, bool inRTL) const
{
	StUseContext_NoRetain	context(this);
	VStringConvertBuffer	buffer(inString, VTC_MacOSAnsi);
	return ::TextWidth(buffer.GetCPointer(), 0, buffer.GetSize());
}


Real VMacQDGraphicContext::GetCharWidth(UniChar inChar) const
{
	VStr4	charString(inChar);
	return GetTextWidth(charString);
}


Real VMacQDGraphicContext::GetTextHeight(bool inIncludeExternalLeading) const
{
	StUseContext_NoRetain	context(this);
	
	FMFontFamily	family = ::GetPortTextFont(fContext);
	sWORD	fontSize = ::GetPortTextSize(fContext);
	sWORD	fontStyle = ::GetPortTextFace(fContext);
	
	FontInfo	fontInfo;
	OSErr	error = ::FetchFontInfo(family, fontSize, fontStyle, &fontInfo);
	assert(error == noErr);
	
	return (fontInfo.ascent + fontInfo.descent + (inIncludeExternalLeading ? fontInfo.leading : 0));
}


#pragma mark-

void VMacQDGraphicContext::NormalizeContext()
{
}


void VMacQDGraphicContext::SaveContext()
{
	if (fSavedContext == NULL)
		fSavedContext = new VHandleStream;
	
	if (fSavedContext != NULL)
	{
		StUseContext_NoRetain	context(this);
		
		// Optimize stream storing
		fSavedContext->OpenWriting();
		
		// Store pen state
		sLONG	size = sizeof(PenState) / sizeof(sWORD);
		
		PenState	pnState;
		::GetPenState(&pnState);
		fSavedContext->PutWords((sWORD*)&pnState, size);
		
		// Store fore and back colors
		size = sizeof(RGBColor) / sizeof(sWORD);
		
		RGBColor	color;
		::GetForeColor(&color);
		fSavedContext->PutWords((sWORD*)&color, size);
		
		::GetBackColor(&color);
		fSavedContext->PutWords((sWORD*)&color, size);
		
		// Store back pattern
//		PixPatHandle	bkPPat = ::NewPixPat();
//		::GetPortBackPixPat(GetParentPort(), bkPPat);
//		
//		VPattern	bkPat(bkPPat);
//		bkPat.WriteToStream(fSavedContext);
//		::DisposePixPat(bkPPat);
		
		// Store text mode
		sWORD	mode = ::GetPortTextMode(GetParentPort());
		fSavedContext->PutWord(mode);
		
		// Close and check for errors
		assert(fSavedContext->GetLastError() ==  VE_OK);
		fSavedContext->CloseWriting();
	}
}


void VMacQDGraphicContext::RestoreContext()
{
	if (fSavedContext != NULL)
	{
		StUseContext_NoRetain	context(this);
		
		fSavedContext->OpenReading();
		
		// Load pen state
		sLONG	size = sizeof(PenState) / sizeof(sWORD);
		
		PenState	pnState;
		fSavedContext->GetWords((sWORD*)&pnState, &size);
		::SetPenState(&pnState);
		
		// Load fore and back colors
		size = sizeof(RGBColor) / sizeof(sWORD);
		
		RGBColor	color;
		fSavedContext->GetWords((sWORD*)&color, &size);
		::RGBForeColor(&color);
		
		fSavedContext->GetWords((sWORD*)&color, &size);
		::RGBBackColor(&color);
		
		// Load back pattern
//		VPattern	bkPat;
//		if (bkPat.ReadFromStream(fSavedContext) == VE_OK)
//			::SetPortBackPixPat(GetParentPort(), bkPat);
		
		// Load text mode
		sWORD	mode = fSavedContext->GetWord();
		::TextMode(mode);
		
		// Close and check for errors
		assert(fSavedContext->GetLastError() == VE_OK);
		fSavedContext->CloseReading();
	}
}


#pragma mark-

void VMacQDGraphicContext::DrawTextBox(const VString& inString, AlignStyle inHAlign, AlignStyle inVAlign, const VRect& inHwndBounds, TextLayoutMode inOptions)
{
	// Rotation center is the topLeft corner of inHwndBounds
	Rect	bounds;
	inHwndBounds.MAC_ToQDRect(bounds);
	
	sWORD	totalW = (sWORD)inHwndBounds.GetWidth();
	sWORD	totalH = (sWORD)inHwndBounds.GetHeight();
	if (totalW <= 0 || totalH <= 0) return;
	
	sLONG	length = inString.GetLength();
	if (length == 0) return;
	
	StUseContext_NoRetain	context(this);
	
	// Temp fix for a crashing bug with MLTE
	_AdjustMLTEMinimalBounds(bounds);
	
	// Compute used bounds for drawing justified text
	TXNTextBoxOptionsData	options;
	options.optionTags = kTXNSetFlushnessMask | kTXNSetJustificationMask | kTXNUseFontFallBackMask | kTXNDontDrawTextMask | kTXNImageWithQDMask;
	options.rotation = 0;
	options.options = NULL;
	options.justification = (inHAlign == AL_JUST) ? kATSUFullJustification : kATSUNoJustification;
	options.flushness = kATSUStartAlignment;
	
	if (inOptions & TLM_DONT_WRAP)
		options.optionTags |= kTXNDontWrapTextMask;
		
	::TXNDrawUnicodeTextBox(inString.GetCPointer(), length, &bounds, NULL, &options);
	
	// Set default tags
	options.optionTags = kTXNSetFlushnessMask | kTXNSetJustificationMask | kTXNUseFontFallBackMask | kTXNDontUpdateBoxRectMask | kTXNImageWithQDMask;
	
	if (inOptions & TLM_DONT_WRAP)
		options.optionTags |= kTXNDontWrapTextMask;
		
	// Set angle
	Real	textAngle = fTextAngle / PI * 180.0;
	if (Abs(textAngle) > kREAL_PIXEL_PRECISION)
	{
		options.optionTags |= kTXNRotateTextMask;
		options.rotation = ::X2Fix(-textAngle);
	}
		
	// Set horizontal justification
	switch (inHAlign)
	{
		case AL_JUST:
			options.justification = kATSUFullJustification;
			options.flushness = kATSUStartAlignment;
			break;
			
		case AL_LEFT:
		case AL_DEFAULT:
			options.justification = kATSUNoJustification;
			options.flushness = kATSUStartAlignment;
			break;
			
		case AL_CENTER:
			options.justification = kATSUNoJustification;
			options.flushness = kATSUCenterAlignment;
			break;
			
		case AL_RIGHT:
			options.justification = kATSUNoJustification;
			options.flushness = kATSUEndAlignment;
			break;
			
		default:
			options.justification = kATSUNoJustification;
			options.flushness = kATSUStartAlignment;
			assert(false);
			break;
	}
	
	// Set vertical justification (by adjusting bounds' top and bottom)
	sWORD	textH = Min((sWORD)(bounds.bottom - bounds.top), totalH);
	switch (inVAlign)
	{
		case AL_BOTTOM:
			bounds.top += totalH - textH;
			break;
			
		case AL_CENTER:
			bounds.top += (totalH - textH) / 2;
			break;
			
		case AL_DEFAULT:
		case AL_TOP:
			break;
			
		default:
			assert(false);
			break;
	}
	
	bounds.bottom = bounds.top + textH;
	bounds.left = (short)inHwndBounds.GetLeft();
	bounds.right = (short)inHwndBounds.GetRight();
	
	// Temp fix for a crashing bug with MLTE
	VMacQDGraphicContext::_AdjustMLTEMinimalBounds(bounds);
	
#if DEBUG_DRAWTEXTBOX_BOUNDS
	::FrameRect(&bounds);
#endif

	::TXNDrawUnicodeTextBox(inString.GetCPointer(), length, &bounds, NULL, &options);
}


void VMacQDGraphicContext::DrawText(const VString& inString, TextLayoutMode inOptions, bool inRTLAnchorRight)
{
	VStringConvertBuffer macStr( inString, VTC_MacOSAnsi);
	
	StUseContext_NoRetain	context(this);
	
	::MoveTo((short)fTextPos.GetX(), (short)fTextPos.GetY());
	::DrawText(macStr.GetCPointer(), 0, macStr.GetSize());
	
	Point	pen;
	::GetPen(&pen);
	fTextPos.SetPosTo(pen.h, pen.v); 
}

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
void VMacQDGraphicContext::DrawText (const VString& inString, const VPoint& inPos, TextLayoutMode /*inLayoutMode*/, bool /*inRTLAnchorRight*/)
{
	VStringConvertBuffer macStr( inString, VTC_MacOSAnsi);
	
	StUseContext_NoRetain	context(this);
	
	::MoveTo((short)inPos.GetX(), (short)inPos.GetY());
	::DrawText(macStr.GetCPointer(), 0, macStr.GetSize());
}


#pragma mark-

void VMacQDGraphicContext::FrameRect(const VRect& inHwndBounds)
{
	StUseContext_NoRetain	context(this);
	
	Rect	macRect;
	::FrameRect(inHwndBounds.MAC_ToQDRect(macRect));
}

void VMacQDGraphicContext::FrameOval(const VRect& inHwndBounds)
{
	StUseContext_NoRetain	context(this);
	
	Rect	macRect;
	::FrameOval(inHwndBounds.MAC_ToQDRect(macRect));
}


void VMacQDGraphicContext::FrameRegion(const VRegion& inHwndRegion)
{
	StUseContext_NoRetain	context(this);
	
	RgnHandle	qdRgn = inHwndRegion.MAC_ToQDRgnHandle();
	::FrameRgn(qdRgn);
	::DisposeRgn(qdRgn);
}


void VMacQDGraphicContext::FramePolygon(const VPolygon& inHwndPolygon)
{
	StUseContext_NoRetain	context(this);
	
	PolyHandle	polygon = inHwndPolygon.MAC_ToQDPolyHandle();
	::FramePoly(polygon);
	::KillPoly(polygon);
}


void VMacQDGraphicContext::FrameBezier(const VGraphicPath& inHwndBezier)
{
	FramePath( inHwndBezier);
}

void VMacQDGraphicContext::FramePath(const VGraphicPath& inHwndPath)
{
	// Not supported
}


void VMacQDGraphicContext::FillRect(const VRect& inHwndBounds)
{
	StUseContext_NoRetain	context(this);
	
	Rect	macRect;
	::PaintRect(inHwndBounds.MAC_ToQDRect(macRect));
}


void VMacQDGraphicContext::FillOval(const VRect& inHwndBounds)
{
	StUseContext_NoRetain	context(this);
	
	Rect	macRect;
	::PaintOval(inHwndBounds.MAC_ToQDRect(macRect));
}


void VMacQDGraphicContext::FillRegion(const VRegion& inHwndRegion)
{
	StUseContext_NoRetain	context(this);
	
	RgnHandle	qdRgn = inHwndRegion.MAC_ToQDRgnHandle();
	::PaintRgn(qdRgn);
	::DisposeRgn(qdRgn);
}


void VMacQDGraphicContext::FillPolygon(const VPolygon& inHwndPolygon)
{
	StUseContext_NoRetain	context(this);
	
	PolyHandle	polygon = inHwndPolygon.MAC_ToQDPolyHandle();
	::PaintPoly(polygon);
	::KillPoly(polygon);
}


void VMacQDGraphicContext::FillBezier(VGraphicPath& inHwndBezier)
{
	FillPath( inHwndBezier);
}

void VMacQDGraphicContext::FillPath(VGraphicPath& inHwndPath)
{
	// Not supported
}



void VMacQDGraphicContext::DrawRect(const VRect& inHwndBounds)
{
	FillRect(inHwndBounds);
	FrameRect(inHwndBounds);
}


void VMacQDGraphicContext::DrawOval(const VRect& inHwndBounds)
{
	FillOval(inHwndBounds);
	FrameOval(inHwndBounds);
}


void VMacQDGraphicContext::DrawRegion(const VRegion& inHwndRegion)
{
	FillRegion(inHwndRegion);
	FrameRegion(inHwndRegion);
}


void VMacQDGraphicContext::DrawPolygon(const VPolygon& inHwndPolygon)
{
	FillPolygon(inHwndPolygon);
	FramePolygon(inHwndPolygon);
}


void VMacQDGraphicContext::DrawBezier(VGraphicPath& inHwndBezier)
{
	DrawPath( inHwndBezier);
}

void VMacQDGraphicContext::DrawPath(VGraphicPath& inHwndPath)
{
	FillPath(inHwndPath);
	FramePath(inHwndPath);
}

void VMacQDGraphicContext::EraseRect(const VRect& inHwndBounds)
{
	StUseContext_NoRetain	context(this);
	
	Rect	macRect;
	::EraseRect(inHwndBounds.MAC_ToQDRect(macRect));
}


void VMacQDGraphicContext::EraseRegion(const VRegion& inHwndRegion)
{
	StUseContext_NoRetain	context(this);
	
	RgnHandle	qdRgn = inHwndRegion.MAC_ToQDRgnHandle();
	::EraseRgn(qdRgn);
	::DisposeRgn(qdRgn);
}


void VMacQDGraphicContext::InvertRect(const VRect& inHwndBounds)
{
	StUseContext_NoRetain	context(this);
	
	Rect	macRect;
	::InvertRect(inHwndBounds.MAC_ToQDRect(macRect));
}


void VMacQDGraphicContext::InvertRegion(const VRegion& inHwndRegion)
{
	StUseContext_NoRetain	context(this);
	RgnHandle	qdRgn = inHwndRegion.MAC_ToQDRgnHandle();
	::InvertRgn(qdRgn);
	::DisposeRgn(qdRgn);
}


void VMacQDGraphicContext::DrawLine(const VPoint& inHwndStart, const VPoint& inHwndEnd)
{
	StUseContext_NoRetain	context(this);

	::MoveTo((short)inHwndStart.GetX(), (short)inHwndStart.GetY());
	::LineTo((short)inHwndEnd.GetX(), (short)inHwndEnd.GetY());
}


void VMacQDGraphicContext::DrawLines(const Real* inCoords, sLONG inCoordCount)
{
	if (inCoordCount < 4 || (inCoordCount & 0x00000001) != 0) return;
	
	::MoveTo((short)(inCoords[0]), (short)(inCoords[1]));
	
	for (sLONG index = 2; index < (inCoordCount - 1); index += 2)
		::LineTo((short)(inCoords[index]), (short)(inCoords[index + 1]));
}


void VMacQDGraphicContext::DrawLineTo(const VPoint& inHwndEnd)
{
	DrawLine(fBrushPos, inHwndEnd);
	fBrushPos = inHwndEnd;
}


void VMacQDGraphicContext::DrawLineBy(Real inWidth, Real inHeight)
{
	VPoint	end(fBrushPos);
	end.SetPosBy(inWidth, inHeight);
	
	DrawLine(fBrushPos, end);
	fBrushPos = end;
}


void VMacQDGraphicContext::MoveBrushTo(const VPoint& inHwndPos)
{
	StUseContext_NoRetain	context(this);
	
	fBrushPos = inHwndPos;
	::MoveTo((short)inHwndPos.GetX(), (short)inHwndPos.GetY());
}


void VMacQDGraphicContext::MoveBrushBy(Real inWidth, Real inHeight)
{
	StUseContext_NoRetain	context(this);
	
	fBrushPos.SetPosBy(inWidth, inHeight);
	::Move((short)inWidth, (short)inHeight);
}


void VMacQDGraphicContext::DrawIcon(const VIcon& inIcon, const VRect& inHwndBounds, PaintStatus /*inState*/, Real /*inScale*/)
{
	StUseContext_NoRetain	context(this);
	
	Rect	macRect;
	::PlotIconRef(inHwndBounds.MAC_ToQDRect(macRect), inIcon.MAC_GetAlignmentType(), inIcon.MAC_GetTransformType(), kIconServicesNormalUsageFlag, inIcon);
}

void VMacQDGraphicContext::DrawPicture (const VPicture& inPicture,const VRect& inHwndBounds,VPictureDrawSettings *inSet)
{
	StUseContext_NoRetain	context(this);
	
	inPicture.Draw(fContext,inHwndBounds,inSet);
	
}
#if 0
void VMacQDGraphicContext::DrawPicture(const VOldPicture& inPicture, PictureMosaic inMosaicMode, const VRect& inHwndBounds, TransferMode /*inDrawingMode*/)
{	
	VOldImageData*	image = inPicture.GetImage();
	if (image == NULL) return;
	
	VOldPictData*	data = (VOldPictData*) image;
	if (!testAssert(image->GetImageKind() == IK_PICT)) return;
	
	StUseContext_NoRetain	context(this);
	
	Rect	macRect;
	if (inMosaicMode == PM_TILE)
	{
		Real	tileWidth = inPicture.GetWidth();
		Real	tileHeight = inPicture.GetHeight();
		
		VRect	tileBounds(inHwndBounds);
		tileBounds.SetSizeTo(tileWidth, tileHeight);
		
		sLONG	hTileCount = (sLONG)(1 + inHwndBounds.GetWidth() / tileWidth);
		sLONG	vTileCount = (sLONG)(1 + inHwndBounds.GetHeight() / tileHeight);
		
		for (sLONG vIndex = 0; vIndex < vTileCount; vIndex++)
		{
			for (sLONG hIndex = 0; hIndex < hTileCount; hIndex++)
			{
				tileBounds.MAC_ToQDRect(macRect);
				//::NCMDrawMatchedPicture((PicHandle)data->GetData(), NULL, &macRect);
				::DrawPicture((PicHandle)data->GetData(), &macRect);
				tileBounds.SetPosBy(tileWidth, 0);
			}
			
			tileBounds.SetPosBy(-tileWidth * hTileCount, tileHeight);
		}
	}
	else
	{
		inHwndBounds.MAC_ToQDRect(macRect);
		//::NCMDrawMatchedPicture((PicHandle)data->GetData(), NULL, &macRect);
		::DrawPicture((PicHandle)data->GetData(), &macRect);
	}
}
#endif
void VMacQDGraphicContext::DrawFileBitmap(const VFileBitmap *inFileBitmap, const VRect& inHwndBounds)
{
}

void VMacQDGraphicContext::DrawPictureFile(const VFile& inFile, const VRect& inHwndBounds)
{
	StUseContext_NoRetain	context(this);
	VFileDesc *fileDesc;
	VError error = inFile.Open(FA_READ, &fileDesc);
	if (error == VE_OK || error == VE_STREAM_ALREADY_OPENED)
	{
		Rect	macRect;
		::DrawPictureFile(fileDesc->GetSystemRef(), inHwndBounds.MAC_ToQDRect(macRect), NULL);
		delete fileDesc;
	}
}


#pragma mark-

void VMacQDGraphicContext::SaveClip()
{
	StUseContext_NoRetain	context(this);
	
	RgnHandle	macRegion = ::NewRgn();
	::GetClip(macRegion);
	
	if (fClipStack == NULL)
		fClipStack = new VArrayOf<RgnHandle>;
		
	if (fClipStack != NULL)
		fClipStack->Push(macRegion);
}


void VMacQDGraphicContext::RestoreClip()
{
	if (fClipStack != NULL)
	{
		StUseContext_NoRetain	context(this);
		
		RgnHandle	macRegion = fClipStack->Pop();
		if (!macRegion) return;
		
		::SetClip(macRegion);
		::DisposeRgn(macRegion);
	}
}


void VMacQDGraphicContext::ClipRect(const VRect& inHwndBounds, Boolean inAddToPrevious, Boolean inIntersectToPrevious)
{
	StUseContext_NoRetain	context(this);
	
#if VERSIONDEBUG
	if (sDebugNoClipping) return;
#endif
	
	Rect	macRect;
	if (inAddToPrevious)
	{
		RgnHandle	clip = ::NewRgn();
		RgnHandle	newRgn = ::NewRgn();
		
		::RectRgn(newRgn, inHwndBounds.MAC_ToQDRect(macRect));
		::GetClip(clip);
		
		::SectRgn(clip, newRgn, clip);
		::SetClip(clip);
		
		::DisposeRgn(clip);
		::DisposeRgn(newRgn);
	}
	else
	{
		::ClipRect(inHwndBounds.MAC_ToQDRect(macRect));
	}
	
	RevealClipping(context);
}


void VMacQDGraphicContext::ClipRegion(const VRegion& inHwndRegion, Boolean inAddToPrevious, Boolean inIntersectToPrevious)
{
	StUseContext_NoRetain	context(this);
	
#if VERSIONDEBUG
	if (sDebugNoClipping) return;
#endif
	
	if (inAddToPrevious)
	{
		RgnHandle	clip = ::NewRgn();
		RgnHandle	qdRgn = inHwndRegion.MAC_ToQDRgnHandle();
		::GetClip(clip);
		::SectRgn(clip, qdRgn, clip);
		::SetClip(clip);
		::DisposeRgn(clip);
		::DisposeRgn(qdRgn);
	}
	else
	{
		RgnHandle	qdRgn = inHwndRegion.MAC_ToQDRgnHandle();
		::SetClip(qdRgn);
		::DisposeRgn(qdRgn);
	}
	
	RevealClipping(context);
}


void VMacQDGraphicContext::GetClip(VRegion& outHwndRgn) const
{
	StUseContext_NoRetain	context(this);
	
	RgnHandle	macRegion = ::NewRgn();
	::GetClip(macRegion);
	
	outHwndRgn.MAC_FromQDRgnHandle(macRegion, true);
}


void VMacQDGraphicContext::RevealClipping(PortRef inPort)
{
#if VERSIONDEBUG
	if (sDebugRevealClipping)
	{
		PixPatHandle	pixPat = ::GetPixPat(kREVEAL_CLIPPING_PATTERN);
		assert(pixPat != NULL);
	
		StSetPort	context(inPort);
		RgnHandle	clipRegion = ::NewRgn();
		::GetClip(clipRegion);
	
		// Draw a "ring" region (usually light yellow colored)
		RgnHandle	ringRgn = ::NewRgn();
		::CopyRgn(clipRegion, ringRgn);
		::InsetRgn(ringRgn, 4, 4);
		::DiffRgn(clipRegion, ringRgn, ringRgn);
		::DisposeRgn(clipRegion);
		
		::FillCRgn(ringRgn, pixPat);
		::QDFlushPortBuffer(context, ringRgn);
		
		::DisposePixPat(pixPat);
		::DisposeRgn(ringRgn);
		
		// Delay for displaying
		uLONG	result;
		::Delay(kDEBUG_REVEAL_CLIPPING_DELAY, &result);
	}
#endif
}


void VMacQDGraphicContext::RevealUpdate(WindowRef inWindow)
{
#if VERSIONDEBUG
	if (sDebugRevealUpdate)
	{
		PixPatHandle	pixPat = ::GetPixPat(kREVEAL_UPDATE_PATTERN);
		assert(pixPat != NULL);
		
		StSetPort	port(inWindow);
		RgnHandle	updateRgn = ::NewRgn();
		::GetWindowRegion(inWindow, kWindowUpdateRgn, updateRgn);
		
		// Convert to local coords
		Point	offset = { 0, 0 };
		::LocalToGlobal(&offset);
		::OffsetRgn(updateRgn, -offset.h, -offset.v);
		
		// Make sure the clipping region won't obstruct our region
		StSetPortClipping	clip(inWindow, updateRgn);
		
		// Fill with pattern (usually plain light yellow)
		::FillCRgn(updateRgn, pixPat);
		::QDFlushPortBuffer(port, updateRgn);
		
		::DisposePixPat(pixPat);
		::DisposeRgn(updateRgn);
		
		// Delay for displaying
		uLONG	result;
		::Delay(kDEBUG_REVEAL_UPDATE_DELAY, &result);
	}
#endif
}


void VMacQDGraphicContext::RevealBlitting(PortRef inPort, const RgnRef inRegion)
{
#if VERSIONDEBUG
	if (sDebugRevealBlitting)
	{
		PixPatHandle	pixPat = ::GetPixPat(kREVEAL_BLITTING_PATTERN);
		assert(pixPat != NULL);
		
		// Leave clipping as it _will_ have influence on copybits
		StSetPort	port(inPort);
		
		// Fill with pattern (usually square patterned yellow)
		RgnHandle	qdRgn = ::NewRgn();
		::HIShapeGetAsQDRgn(inRegion, qdRgn);
		::FillCRgn(qdRgn, pixPat);
		::DisposePixPat(pixPat);
		
		::QDFlushPortBuffer(port, qdRgn);
		::DisposeRgn(qdRgn);
		
		// Delay for displaying
		uLONG	result;
		::Delay(kDEBUG_REVEAL_BLITTING_DELAY, &result);
	}
#endif
}


void VMacQDGraphicContext::RevealInval(PortRef inPort, const RgnRef inRegion)
{
#if VERSIONDEBUG
	if (sDebugRevealInval)
	{
		PixPatHandle	pixPat = ::GetPixPat(kREVEAL_INVAL_PATTERN);
		assert(pixPat != NULL);
		
		// Make sure the clipping region won't obstruct our region
		StSetPort	port(inPort);
		StSetPortClipping	clip(inPort, inRegion);
		
		// Fill with pattern (usually striped yellow)
		RgnHandle	qdRgn = ::NewRgn();
		::HIShapeGetAsQDRgn(inRegion, qdRgn);
		::FillCRgn(qdRgn, pixPat);
		::DisposePixPat(pixPat);
		
		::QDFlushPortBuffer(inPort, qdRgn);
		::DisposeRgn(qdRgn);
		
		// Delay for displaying
		uLONG	result;
		::Delay(kDEBUG_REVEAL_INVAL_DELAY, &result);
	}
#endif
}


#pragma mark-

void VMacQDGraphicContext::CopyContentTo(const VGraphicContext* inDestContext, const VRect* inSrcBounds, const VRect* inDestBounds, TransferMode inMode, const VRegion* inMask)
{
	if (inDestContext == NULL) return;
	
	StUseContext	destContext(inDestContext);
	
	// Convert transfer mode
	sWORD	mode;
	RGBColor	savedOpColor;
	
	switch (inMode)
	{
		case TM_COPY:
			mode = srcCopy;
			break;
			
		case TM_OR:
			mode = srcOr;
			break;
			
		case TM_XOR:
			mode = srcXor;
			break;
			
		case TM_ERASE:
			mode = srcBic;
			break;
			
		case TM_NOT_COPY:
			mode = notSrcCopy;
			break;
			
		case TM_NOT_OR:
			mode = notSrcOr;
			break;
			
		case TM_NOT_XOR:
			mode = notSrcXor;
			break;
			
		case TM_NOT_ERASE:
			mode = notSrcBic;
			break;
			
		case TM_TRANSPARENT:
			mode = transparent;
			break;
			
		case TM_BLEND:
		{
			mode = blend;
			RGBColor	blendColor;
			blendColor.red = blendColor.green = blendColor.blue = ((fAlphaBlend * 0xFFFF) / 0xFF);
			::GetPortOpColor(destContext, &savedOpColor);
			::SetPortOpColor(destContext, &blendColor);
			break;
		}
		
		default:
			mode = inMode;
			break;
	}
		
	// Save colors
	RGBColor	savedForeColor, savedBackColor;
	::GetForeColor(&savedForeColor);
	::GetBackColor(&savedBackColor);
	
	// Normalize front and back colors
	::RGBBackColor(&fPixelBackColor);
	::RGBForeColor(&fPixelForeColor);
	
	RgnHandle	qdRgn = (inMask != NULL) ? inMask->MAC_ToQDRgnHandle() : NULL;
	
	Rect	macSrcRect, macDestRect;
	if (inSrcBounds != NULL)
		inSrcBounds->MAC_ToQDRect(macSrcRect);
	else
		::GetPortBounds(fContext, &macSrcRect);
	
	if (inDestBounds != NULL)
		inDestBounds->MAC_ToQDRect(macDestRect);
	else
		::GetPortBounds(destContext, &macDestRect);
	
	::CopyBits(::GetPortBitMapForCopyBits(fContext), ::GetPortBitMapForCopyBits(destContext), &macSrcRect, &macDestRect, mode, qdRgn);
	::DisposeRgn(qdRgn);
	
	if (mode == blend)
		::SetPortOpColor(destContext, &savedOpColor);
	
	// Restore colors
	::RGBForeColor(&savedForeColor);
	::RGBBackColor(&savedBackColor);
}


void VMacQDGraphicContext::FillUniformColor(const VPoint& inContextPos, const VColor& inColor, const VPattern* inPattern)
{
	StUseContext_NoRetain	context(this);
	
	Rect	bounds;
	::GetPortBounds(context, &bounds);
	
	PixMapHandle	srcPixMap = ::GetGWorldPixMap(context);
	if (::LockPixels(srcPixMap))
	{	
		// Create mask from uniform color
		BitMap	bwMask;
		bwMask.bounds = bounds;
		bwMask.rowBytes = (((bwMask.bounds.right - bwMask.bounds.left) +15) / 16) * 2;
		bwMask.baseAddr = ::NewPtrClear((sLONG)(bwMask.bounds.bottom - bwMask.bounds.top) * bwMask.rowBytes);
		if (bwMask.baseAddr != NULL)
		{
			::SeedCFill(::GetPortBitMapForCopyBits(context), &bwMask, &bounds, &bounds, (short)inContextPos.GetX(), (short)inContextPos.GetY(), 0, 0);
			
			// Prepare fore and back colors
			RGBColor	savedForeColor, savedBackColor;
			::GetForeColor(&savedForeColor);
			::GetBackColor(&savedBackColor);
			
			::RGBBackColor(&fPixelBackColor);
			::RGBForeColor(inColor);
			
			if (inPattern != NULL)
			{
				// Create mask from pattern if any
				GWorldPtr	patWorld = NULL;
				OSErr	error = ::NewGWorld(&patWorld, 1, &bounds, 0, 0, 0);
				if (error == noErr && ::MemError() == noErr)
				{
					{
						StSetPort	offscreen(patWorld);
						inPattern->ApplyToContext(NULL);	// Use current port
						::PaintRect(&bounds);
					}
					
					// Fill with patterned color
					::CopyMask(::GetPortBitMapForCopyBits(patWorld), &bwMask, ::GetPortBitMapForCopyBits(context), &bounds, &bounds, &bounds);
					::DisposeGWorld(patWorld);
				}
			}
			else
			{
				// Fill with uniform color
				::CopyMask(&bwMask, &bwMask, ::GetPortBitMapForCopyBits(context), &bounds, &bounds, &bounds);
			}
			
			// Restore colors
			::RGBForeColor(&savedForeColor);
			::RGBBackColor(&savedBackColor);
		
			::DisposePtr(bwMask.baseAddr);
		}
			
		::UnlockPixels(srcPixMap);
	}
}


void VMacQDGraphicContext::SetPixelColor(const VPoint& inContextPos, const VColor& inColor)
{
	StUseContext_NoRetain	context(this);
	
	sLONG	pixelX = (sLONG)inContextPos.GetX();
	sLONG	pixelY = (sLONG)inContextPos.GetY();
	
	::SetCPixel(pixelX, pixelY, inColor);
}


VColor* VMacQDGraphicContext::GetPixelColor(const VPoint& inContextPos) const
{
	VColor*	result = new VColor(COL_WHITE);

	StUseContext_NoRetain	context(this);
	PixMapHandle	pixMap = ::GetGWorldPixMap(context);
	
	if (::LockPixels(pixMap))
	{	
		sLONG	pixelX = (sLONG)inContextPos.GetX();
		sLONG	pixelY = (sLONG)inContextPos.GetY();
	
		if ((**pixMap).pixelSize == 1)
		{
			if (::GetPixel(pixelX, pixelY))
				result->FromStdColor(COL_BLACK);
			else
				result->FromStdColor(COL_WHITE);
		}
		else
		{
			RGBColor	rgb;
			::GetCPixel(pixelX, pixelY, &rgb);
			result->FromColorRef(rgb);
		}
		
		::UnlockPixels(pixMap);
	}
	
	return result;
}


void VMacQDGraphicContext::Flush()
{
	// Flushing has meaning only for a window port
	::QDFlushPortBuffer(fContext, NULL);
}


void VMacQDGraphicContext::_AdjustMLTEMinimalBounds(Rect& ioBounds)
{
	// MLTE doesn't like a too small rect
	if ((ioBounds.right - ioBounds.left) < kSMALLEST_TXN_WIDTH)
		ioBounds.right = ioBounds.left = 0;
}


void VMacQDGraphicContext::_ApplyPattern(ContextRef /*inContext*/, PatternStyle inStyle, const VColor& inColor, PatternRef /*inPatternRef*/, FillRuleType /*inFillRule*/)
{
	Pattern	pattern;
	Boolean	isFillPattern = _GetStdPattern(inStyle, &pattern);
	
	// Simply apply to current port
	::RGBForeColor(inColor);
	if (isFillPattern)
		::BackPat(&pattern);
	else
		::PenPat(&pattern);
}


Boolean VMacQDGraphicContext::_GetStdPattern(PatternStyle inStyle, void* ioPattern)
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


#pragma mark-

void VMacQDGraphicContext::BeginUsingContext(bool /*inNoDraw*/)
{
	if (fPortRefcon == 0)
	{
		GDHandle	gdHandle;
		::GetGWorld(&fSavedPort, &gdHandle);
		::SetGWorld(fContext, NULL);
	}
	
#if VERSIONDEBUG
	else
	{
		// This debug test make sure mecanism is secured
		GrafPtr	testPort;
		GDHandle	gdHandle;
		::GetGWorld(&testPort, &gdHandle);
		assert(fContext == testPort);
	}
#endif
	
	fPortRefcon++;
}


void VMacQDGraphicContext::EndUsingContext()
{
	fPortRefcon--;
	
	if (fPortRefcon == 0)
	{
		assert(fContext != NULL && fSavedPort != NULL);
		::SetGWorld(fSavedPort, NULL);
	}
	
	assert(fPortRefcon >= 0);
}


#pragma mark-

StSetPort::StSetPort(WindowRef inWindow)
{
	::GetGWorld(&fSavedPort, &fSavedGD);
	
	fCurrentPort = ::GetWindowPort(inWindow);
	::SetPortWindowPort(inWindow);
}


StSetPort::StSetPort(PortRef inPort)
{
	::GetGWorld(&fSavedPort, &fSavedGD);
	
	fCurrentPort = inPort;
	::SetGWorld(inPort, NULL);
}


StSetPort::~StSetPort()
{
	::SetGWorld(fSavedPort, fSavedGD);
}


#pragma mark-

StSetPortClipping::StSetPortClipping(WindowRef inWindow, RgnHandle inNewClip)
{
	fPort = ::GetWindowPort(inWindow);
	fSavedClip = ::NewRgn();
	::GetClip(fSavedClip);
	
	::SetPort(fPort);
	::SetClip(inNewClip);
}


StSetPortClipping::StSetPortClipping(WindowRef inWindow, RgnRef inNewClip)
{
	fPort = ::GetWindowPort(inWindow);
	fSavedClip = ::NewRgn();
	::GetClip(fSavedClip);
	
	::HIShapeSetQDClip(inNewClip, fPort);
}


StSetPortClipping::StSetPortClipping(PortRef inPort, RgnHandle inNewClip)
{
	fPort = inPort;
	fSavedClip = ::NewRgn();
	::GetClip(fSavedClip);
	
	::SetPort(fPort);
	::SetClip(inNewClip);
}


StSetPortClipping::StSetPortClipping(PortRef inPort, RgnRef inNewClip)
{
	fPort = inPort;
	fSavedClip = ::NewRgn();
	::GetClip(fSavedClip);
	
	::HIShapeSetQDClip(inNewClip, fPort);
}


StSetPortClipping::~StSetPortClipping()
{
	::SetClip(fSavedClip);
	::DisposeRgn(fSavedClip);
}


#pragma mark-

VMacQDOffscreenContext::VMacQDOffscreenContext(GrafPtr inSourceContext) : VMacQDGraphicContext(inSourceContext)
{	
	_Init();
}


VMacQDOffscreenContext::VMacQDOffscreenContext(const VMacQDOffscreenContext& inOriginal) : VMacQDGraphicContext(inOriginal)
{
	_Init();
}


VMacQDOffscreenContext::~VMacQDOffscreenContext()
{
	_Dispose();
}


void VMacQDOffscreenContext::_Init()
{
	fPicture = NULL;
}


void VMacQDOffscreenContext::_Dispose()
{
	::KillPicture(fPicture);
}


void VMacQDOffscreenContext::CopyContentTo(const VGraphicContext* inDestContext, const VRect* /*inSrcBounds*/, const VRect* inDestBounds, TransferMode /*inMode*/, const VRegion* /*inMask*/)
{
	StUseContext_NoRetain	context(inDestContext);
	
	Rect	macRect;
	if (inDestBounds != NULL)
		inDestBounds->MAC_ToQDRect(macRect);
	else
		::GetPortBounds(context, &macRect);
	
	::DrawPicture(fPicture, &macRect);
}


Boolean VMacQDOffscreenContext::CreatePicture(const VRect& inHwndBounds)
{
	ReleasePicture();
	
	OpenCPicParams	params;
	inHwndBounds.MAC_ToQDRect(params.srcRect);
	
	params.hRes = 0x00480000;
	params.vRes = 0x00480000;
	params.version = -2;
	params.reserved1 = 0;
	params.reserved2 = 0;
	
	fPicture = ::OpenCPicture(&params);
	fPictureClosed = false;
	
	if (fPicture != NULL)
	{
		StUseContext_NoRetain	context(this);
		::ClipRect(&params.srcRect);
		::EraseRect(&params.srcRect);
		
		::SetOrigin((short)inHwndBounds.GetLeft(), (short)inHwndBounds.GetTop());
	}

	return (fPicture != NULL);
}


void VMacQDOffscreenContext::ReleasePicture()
{
	_Dispose();
	fPicture = NULL;
}


PicHandle VMacQDOffscreenContext::MAC_GetPicHandle() const
{
	Handle	newPicture = (Handle) fPicture;
	::HandToHand(&newPicture);
	
	return (PicHandle) newPicture;
}


#pragma mark-

VMacQDBitmapContext::VMacQDBitmapContext(GrafPtr inSourceContext) : VMacQDGraphicContext(NULL)
{	
	_Init();
	
	fSrcContext = inSourceContext;
}


VMacQDBitmapContext::VMacQDBitmapContext(const VMacQDBitmapContext& inOriginal) : VMacQDGraphicContext(inOriginal)
{
	_Init();
	
	fSrcContext = inOriginal.fSrcContext;
	fContext = NULL;
}


VMacQDBitmapContext::~VMacQDBitmapContext()
{
	_Dispose();
}


void VMacQDBitmapContext::_Init()
{
	fSrcContext = NULL;
}


void VMacQDBitmapContext::_Dispose()
{
	::DisposeGWorld(fContext);
}


Boolean VMacQDBitmapContext::CreateBitmap(const VRect& inHwndBounds)
{
	Rect	macRect;
	if (fContext == NULL)
		::NewGWorld(&fContext, 0, inHwndBounds.MAC_ToQDRect(macRect), NULL, NULL, 0);
	else
		::UpdateGWorld(&fContext, 0, inHwndBounds.MAC_ToQDRect(macRect), NULL, NULL, 0);
	
	if (fContext != NULL)
	{
		::OffsetRect(&macRect, -macRect.left, -macRect.top);
		
		StUseContext_NoRetain	context(this);
		::ClipRect(&macRect);
		::EraseRect(&macRect);
		
		::SetOrigin((short)inHwndBounds.GetLeft(), (short)inHwndBounds.GetTop());
	}

	return (fContext != NULL);
}


void VMacQDBitmapContext::ReleaseBitmap()
{
	if (fContext != NULL)
	{
		_Dispose();
		fContext = NULL;
	}
}


PicHandle VMacQDBitmapContext::MAC_GetPicHandle() const
{
	OpenCPicParams	params;
	::GetPortBounds(fSrcContext, &params.srcRect);
	params.hRes = 0x00480000;
	params.hRes = 0x00480000;
	params.version = -2;
	params.reserved1 = 0;
	params.reserved2 = 0;
	
	StUseContext_NoRetain	context(this);
	
	PicHandle	newPicture = ::OpenCPicture(&params);
	const BitMap*	portBits = ::GetPortBitMapForCopyBits(fContext);
	::ClipRect(&params.srcRect);
	::CopyBits(portBits, portBits, &params.srcRect, &params.srcRect, srcCopy, NULL);
	::ClosePicture();
	
	return newPicture;
}
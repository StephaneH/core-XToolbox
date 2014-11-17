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
#include "VRect.h"

#if VERSIONDEBUG
#define DBGTESTOVERFLOW 0
#else
#define DBGTESTOVERFLOW 0
#endif

#if DBGTESTOVERFLOW
void _DbgTestOverflow(GReal inValue)
{
	xbox_assert(Abs(inValue) < 0xFFFFF);
}
#endif

VRect::VRect()
{
	fX = fY = 0;
	fWidth  = fHeight = 0;
}


VRect::VRect(GReal inX, GReal inY, GReal inWidth, GReal inHeight)
{
	fX = inX;
	fY = inY;
	fWidth = inWidth;
	fHeight = inHeight;
#if DBGTESTOVERFLOW
	_DbgTestOverflow(fX);
	_DbgTestOverflow(fY);
	_DbgTestOverflow(fWidth);
	_DbgTestOverflow(fHeight);
#endif
}


VRect::VRect (const VRect& inOriginal)
{
	FromRect(inOriginal);
}

#if !VERSION_LINUX
VRect::VRect(const RectRef& inNativeRect)
{
	FromRectRef(inNativeRect);
}
#endif

VRect::~VRect()
{
}

void VRect::SetPosBy (GReal inHoriz, GReal inVert)
{ 
	fX += inHoriz; 
	fY += inVert; 
#if DBGTESTOVERFLOW
	_DbgTestOverflow(fX);
	_DbgTestOverflow(fY);
#endif
}

void VRect::SetSizeBy (GReal inHoriz, GReal inVert) 
{ 
	fWidth += inHoriz; 
	fHeight += inVert;
#if DBGTESTOVERFLOW
	_DbgTestOverflow(fWidth);
	_DbgTestOverflow(fHeight);
#endif
}

void VRect::SetPosTo (const GReal inX,const GReal inY)
{
	fX = inX; 
	fY = inY;
#if DBGTESTOVERFLOW
	_DbgTestOverflow(fX);
	_DbgTestOverflow(fY);
#endif
}

void VRect::SetCoordsTo (GReal inX, GReal inY, GReal inWidth, GReal inHeight) 
{ 
	fX = inX; 
	fY = inY; 
	fWidth = inWidth; 
	fHeight = inHeight; 
#if DBGTESTOVERFLOW
	_DbgTestOverflow(fX);
	_DbgTestOverflow(fY);
	_DbgTestOverflow(fWidth);
	_DbgTestOverflow(fHeight);
#endif
}

void VRect::SetCoordsBy (GReal inX, GReal inY, GReal inWidth, GReal inHeight) 
{ 
	fX += inX; 
	fY += inY; 
	fWidth += inWidth; 
	fHeight += inHeight; 
#if DBGTESTOVERFLOW
	_DbgTestOverflow(fX);
	_DbgTestOverflow(fY);
	_DbgTestOverflow(fWidth);
	_DbgTestOverflow(fHeight);
#endif
}

void VRect::SetX (GReal inX) 
{ 
	fX = inX; 
#if DBGTESTOVERFLOW
	_DbgTestOverflow(fX);
#endif
}

void VRect::SetY (GReal inY) 
{ 
	fY = inY; 
#if DBGTESTOVERFLOW
	_DbgTestOverflow(fY);
#endif
}

void VRect::SetWidth (GReal inWidth) 
{ 
	fWidth = inWidth; 
#if DBGTESTOVERFLOW
	_DbgTestOverflow(fWidth);
#endif
}

void VRect::SetHeight (GReal inHeight) 
{ 
	fHeight = inHeight; 
#if DBGTESTOVERFLOW
	_DbgTestOverflow(fHeight);
#endif
}


void VRect::ScaleSizeBy (GReal inHorizScale, GReal inVertScale)
{
	SetSizeBy(GetWidth() * (inHorizScale - 1), GetHeight() * (inVertScale - 1));
}


void VRect::ScalePosBy (GReal inHorizScale, GReal inVertScale)
{
	// JM 281206 REF5 SetPosBy(GetWidth() * (inHorizScale - 1), GetHeight() * (inVertScale - 1));
	SetPosBy(GetLeft() * (inHorizScale - 1), GetTop() * (inVertScale - 1));
}


void VRect::SetPosTo (const VPoint& inPos)
{
	SetPosBy(inPos.GetX() - fX, inPos.GetY() - fY);
}


void VRect::SetSizeTo (GReal inWidth, GReal inHeight)
{
#if 0
	assert(inWidth >= -1);
	assert(inHeight >= -1);
#endif
	
	SetSizeBy((inWidth >= 0) ? inWidth - fWidth : 0, (inHeight >= 0) ? inHeight - fHeight : 0);
}


bool VRect::operator == (const VRect& inRect) const
{
	return (Abs(fX - inRect.fX) < kREAL_PIXEL_PRECISION
			&& Abs(fY - inRect.fY) < kREAL_PIXEL_PRECISION
			&& Abs(fWidth - inRect.fWidth) < kREAL_PIXEL_PRECISION
			&& Abs(fHeight - inRect.fHeight) < kREAL_PIXEL_PRECISION);
}


void VRect::Union (const VRect& inRect)
{
	if (IsEmpty())
	{
		fX = inRect.fX;
		fY = inRect.fY;
		fWidth = inRect.fWidth;
		fHeight = inRect.fHeight;
	}
	else if (!inRect.IsEmpty())
	{
		GReal	newLeft = Min(fX, inRect.fX);
		GReal	newTop = Min(fY, inRect.fY);
		GReal	newRight = Max(GetRight(), inRect.GetRight());
		GReal	newBottom = Max(GetBottom(), inRect.GetBottom());
		
		SetCoords(newLeft, newTop, newRight - newLeft, newBottom - newTop);
	}
}


void VRect::Intersect (const VRect& inRect)
{
	GReal	newLeft = Max(fX, inRect.GetX());
	GReal	newTop = Max(fY, inRect.GetY());
	GReal	newRight = Min(GetRight(), inRect.GetRight());
	GReal	newBottom = Min(GetBottom(), inRect.GetBottom());
	
	if (newRight > newLeft && newBottom > newTop)
		SetCoords(newLeft, newTop, newRight - newLeft, newBottom - newTop);
	else
		SetCoords(0, 0, 0, 0);
}


void VRect::Inset (GReal inHoriz, GReal inVert)
{
	fX += inHoriz;
	fY += inVert;
	fWidth -= 2 * inHoriz;
	fHeight -= 2 * inVert;
#if DBGTESTOVERFLOW
	_DbgTestOverflow(fX);
	_DbgTestOverflow(fY);
	_DbgTestOverflow(fWidth);
	_DbgTestOverflow(fHeight);
#endif
}


bool VRect::HitTest (const VPoint& inPoint) const
{
	return ((inPoint.GetX() - fX) > -kREAL_PIXEL_PRECISION
			&& (inPoint.GetX() - GetRight()) < kREAL_PIXEL_PRECISION
			&& (inPoint.GetY() - fY) > -kREAL_PIXEL_PRECISION
			&& (inPoint.GetY() - GetBottom()) < kREAL_PIXEL_PRECISION);
}


void VRect::FromRect(const VRect& inOriginal)
{
	fX = inOriginal.fX;
	fY = inOriginal.fY;
	fWidth = inOriginal.fWidth;
	fHeight = inOriginal.fHeight;
}


/** set rect to the smallest containing rect with integer coordinates 
@param inStickOriginToNearestInteger
	false (default): origin is set to floor(origin) (use it in order to get ideal GDI/QD clipping or invaliding rectangle which contains GDI/QD drawing rectangle)
	true: origin is rounded to the nearest int (use it to get ideal GDI/QD drawing rectangle)
*/
#if VERSIONDEBUG
uLONG sCounterNormalizeIsDone = 0;
#endif

void VRect::NormalizeToInt(bool inStickOriginToNearestInteger)
{
	GReal xRounded = std::floor(fX+0.5f);
	GReal yRounded = std::floor(fY+0.5f);

#if VERSIONDEBUG //this help to test cases where normalizing is necessary
	if (fX != std::floor(fX) || fY != std::floor(fY) || fWidth != std::ceil(fWidth) || fHeight != std::ceil(fHeight))
		sCounterNormalizeIsDone++;
#endif
	if (inStickOriginToNearestInteger)
	{
		fX = xRounded;
		fY = yRounded;
		fWidth = std::ceil(fWidth);
		fHeight = std::ceil(fHeight);
		return;
	}

	GReal dx = fX-std::floor(fX);
	GReal dy = fY-std::floor(fY);

	//caution: bounding rectangle (used for clipping or invalidating - inStickOriginToNearestInteger = false) should include drawing rectangle (inStickOriginToNearestInteger = true):
	//as drawing rectangle is snapped to nearest pixel, we need to add extra pixel here if rounded position is not equal to floor(position)
	//otherwise clipping rectangle might crop right or bottom drawing rectangle edge
	if (xRounded != std::floor(fX))
		dx = 1;
	if (yRounded != std::floor(fY))
		dy = 1;

	fX = std::floor(fX);
	fY = std::floor(fY);
	fWidth = std::ceil(fWidth+dx);
	fHeight = std::ceil(fHeight+dy);
}

#if !VERSION_LINUX
void VRect::FromRectRef(const RectRef& inNativeRect)
{
#if VERSIONMAC
	fX = inNativeRect.origin.x;
	fY = inNativeRect.origin.y;
	fWidth = inNativeRect.size.width;
	fHeight = inNativeRect.size.height;
#else
	fX = inNativeRect.left;
	fY = inNativeRect.top;
	fWidth = inNativeRect.right - inNativeRect.left;
	fHeight = inNativeRect.bottom - inNativeRect.top;
#if DBGTESTOVERFLOW
	_DbgTestOverflow(fX);
	_DbgTestOverflow(fY);
	_DbgTestOverflow(fWidth);
	_DbgTestOverflow(fHeight);
#endif
#endif
}


void VRect::ToRectRef(RectRef& outNativeRect) const
{
#if VERSIONMAC
	outNativeRect.origin.x = fX;
	outNativeRect.origin.y = fY;
	outNativeRect.size.width = fWidth;
	outNativeRect.size.height = fHeight;
#else
	outNativeRect.left = fX;
	outNativeRect.top = fY;
	outNativeRect.right = fX + fWidth;
	outNativeRect.bottom = fY + fHeight;
#endif
}
#endif


#if VERSIONMAC

void VRect::MAC_FromQDRect(const Rect& inQDRect)
{
	fX = inQDRect.left;
	fY = inQDRect.top;
	fWidth = inQDRect.right - inQDRect.left;
	fHeight = inQDRect.bottom - inQDRect.top;
}


Rect* VRect::MAC_ToQDRect(Rect& outQDRect) const
{
#if DEBUG_QD_LIMITS
	assert(fX >= kMIN_sWORD);
	assert(fY >= kMIN_sWORD);
	assert(fWidth <= kMAX_sWORD);
	assert(fHeight <= kMAX_sWORD);
#endif
	
	// Make sure doesn't reach QD limits
	if (fX < kMIN_sWORD || fY < kMIN_sWORD || fWidth > kMAX_sWORD || fHeight > kMAX_sWORD)
	{
		outQDRect.left = outQDRect.top = 0;
		outQDRect.right = outQDRect.bottom = 0;
	}
	else
	{
		outQDRect.left = (short)fX;
		outQDRect.top = (short)fY;
		outQDRect.right = (short)(fX + fWidth);
		outQDRect.bottom = (short)(fY + fHeight);
	}
	
	return &outQDRect;
}


void VRect::MAC_FromCGRect (const CGRect& inCGRect)
{
	fX = inCGRect.origin.x;
	fY = inCGRect.origin.y;
	fWidth = inCGRect.size.width;
	fHeight = inCGRect.size.height;
}
	
	
CGRect*	VRect::MAC_ToCGRect (CGRect& outCGRect) const
{
	outCGRect.origin.x = fX;
	outCGRect.origin.y = fY;
	outCGRect.size.width = fWidth;
	outCGRect.size.height = fHeight;
	
	return &outCGRect;
}

#endif	// VERSIONMAC


bool	VRect::Overlaps (const VRect& inRect) const
{
	VRect	intersect(inRect);
	intersect.Intersect(*this);
	
	return (intersect.fWidth > kREAL_PIXEL_PRECISION && intersect.fHeight > kREAL_PIXEL_PRECISION);
}


bool	VRect::Contains (const VRect& inRect) const
{
	return ((inRect.GetLeft() - GetLeft()) > -kREAL_PIXEL_PRECISION &&
			(inRect.GetRight() - GetRight()) < kREAL_PIXEL_PRECISION &&
			(inRect.GetTop() - GetTop()) > -kREAL_PIXEL_PRECISION &&
			(inRect.GetBottom() - GetBottom()) < kREAL_PIXEL_PRECISION);
}


bool	VRect::IsEmpty () const
{
	return (fWidth < kREAL_PIXEL_PRECISION || fHeight < kREAL_PIXEL_PRECISION);
}


VError VRect::ReadFromStream(VStream* ioStream, sLONG /*inParam*/)
{
	assert(ioStream != NULL);
	
	fX = (GReal) ioStream->GetReal();
	fY = (GReal) ioStream->GetReal();
	fWidth = (GReal) ioStream->GetReal();
	fHeight = (GReal) ioStream->GetReal();
	
#if DBGTESTOVERFLOW
	_DbgTestOverflow(fX);
	_DbgTestOverflow(fY);
	_DbgTestOverflow(fWidth);
	_DbgTestOverflow(fHeight);
#endif
	return ioStream->GetLastError();
}


VError VRect::WriteToStream(VStream* ioStream, sLONG /*inParam*/) const
{
	assert(ioStream != NULL);
	
	ioStream->PutReal(fX);
	ioStream->PutReal(fY);
	ioStream->PutReal(fWidth);
	ioStream->PutReal(fHeight);
	
	return ioStream->GetLastError();
}


VError VRect::SaveToBag(VValueBag& ioBag, VString& outKind) const
{
	ioBag.SetReal(L"left", fX);
	ioBag.SetReal(L"top", fY);
	ioBag.SetReal(L"width", fWidth);
	ioBag.SetReal(L"height", fHeight);
	
	outKind = L"rect";
	return VE_OK;
}


VError VRect::LoadFromBag(const VValueBag& inBag)
{
	bool	fullySpecified = false;
	GReal	tmp,right, bottom;
	
	fX = fY = fWidth = fHeight = 0;
	
	if (inBag.GetReal(L"left", fX))
	{
		if (inBag.GetReal(L"right", right))
		{
			fWidth = right - fX;
			fullySpecified = !inBag.AttributeExists(L"width");
		}
		else
		{
			fullySpecified = inBag.GetReal(L"width", fWidth);
		}
	}
	else if (inBag.GetReal(L"right", right))
	{
		fullySpecified = inBag.GetReal(L"width", fWidth);
		fX = right - fWidth;
	}

	if (inBag.GetReal(L"top", fY))
	{
		if (inBag.GetReal(L"bottom", bottom))
		{
			fHeight = bottom - fY;
			fullySpecified = !inBag.AttributeExists(L"height");
		}
		else
		{
			fullySpecified = inBag.GetReal(L"height", fHeight);
		}
	}
	else if (inBag.GetReal(L"bottom", bottom))
	{
		fullySpecified = inBag.GetReal(L"height", fHeight);
		fY = bottom - fHeight;
	}
	
	if (!fullySpecified)
		return VE_MALFORMED_XML_DESCRIPTION;

#if DBGTESTOVERFLOW
	_DbgTestOverflow(fX);
	_DbgTestOverflow(fY);
	_DbgTestOverflow(fWidth);
	_DbgTestOverflow(fHeight);
#endif
	return VE_OK;
}


VError VRect::LoadFromBagRelative(const VValueBag& inBag, const VRect& inParent)
{
	bool	fullySpecified = false;
	
	GReal left = 0;
	GReal right = 0;
	GReal width = 0;
	if (inBag.GetReal(L"left", left))
	{
		fX = inParent.fX + left;
		if (inBag.GetReal(L"right", right))
		{
			fWidth = inParent.fWidth - (right + left);
			xbox_assert(!inBag.AttributeExists(L"width"));
			fullySpecified = true;
		}
		else
		{
			fullySpecified = inBag.GetReal(L"width", fWidth);
		}
	}
	else
	{
		fullySpecified = inBag.GetReal(L"width", fWidth);
		
		if (inBag.GetReal(L"right", right))
		{
			fX = inParent.fX + inParent.fWidth - right - fWidth;
		}
		else
		{
			fullySpecified = false;
		}
	}

	GReal top = 0;
	GReal bottom = 0;
	GReal height = 0;
	if (inBag.GetReal(L"top", top))
	{
		fY = inParent.fY + top;
		if (inBag.GetReal(L"bottom", bottom))
		{
			fHeight = inParent.fHeight - (bottom + top);
			xbox_assert(!inBag.AttributeExists(L"height"));
			fullySpecified = true;
		}
		else
		{
			inBag.GetReal(L"height", fHeight);
		}
	}
	else
	{
		fullySpecified = inBag.GetReal(L"height", fHeight);
		
		if (inBag.GetReal(L"bottom", bottom))
		{
			fY = inParent.fY + inParent.fHeight - bottom - fHeight;
		}
		else
		{
			fullySpecified = false;
		}
	}
	
	if (!fullySpecified)
		return VE_MALFORMED_XML_DESCRIPTION;

#if DBGTESTOVERFLOW
	_DbgTestOverflow(fX);
	_DbgTestOverflow(fY);
	_DbgTestOverflow(fWidth);
	_DbgTestOverflow(fHeight);
#endif
	return VE_OK;
}

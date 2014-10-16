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
#include "VRegion.h"
#include "VPolygon.h"

#if ENABLE_D2D
#include "XWinD2DGraphicContext.h"
#endif


// Note GDI regions are centered on (0, 0) so maximal size if kMAX_GDI_RGN_COORD
// It could be kMAX_GDI_RGN_SIZE, if it was centered on (kMIN_GDI_RGN_COORD, kMIN_GDI_RGN_COORD)
// but the second solution would create problems with some operations (Inset, SetSizeBy,...)

#if USE_GDIPLUS
using namespace Gdiplus;
#endif


VRegion::VRegion(GReal inX, GReal inY, GReal inWidth, GReal inHeight)
{
	_Init();
	FromRect( VRect( inX, inY, inWidth, inHeight));
}


VRegion::~VRegion()
{
	_Release();
		
	if (fRegionBuffer != NULL)
		::DeleteObject(fRegionBuffer);
}


void VRegion::SetPosBy (GReal inHoriz, GReal inVert)
{
	if (fRegion == NULL)
		SetEmpty();
		
#if !USE_GDIPLUS
	fOffset.SetPosBy(inHoriz, inVert);
	fBounds.SetPosBy(inHoriz, inVert);
#else
	fRegion->Translate((sLONG)inHoriz,(sLONG)inVert);
#endif

	_ComputeBounds();
}


void VRegion::SetSizeBy (GReal inHoriz, GReal inVert)
{
	if (fRegion == NULL)
		SetEmpty();
		
#if USE_GDIPLUS
	Gdiplus::Matrix mat;
	mat.Scale((fBounds.GetWidth()+inHoriz)/(GReal)fBounds.GetWidth(),(fBounds.GetHeight()+inVert)/(GReal)fBounds.GetHeight());
	fRegion->Transform(&mat);
#else

	GReal	dstWidth = fBounds.GetWidth() + inHoriz;
	GReal	dstHeight = fBounds.GetHeight() + inVert;
	
#if DEBUG_GDI_LIMITS
	assert(dstWidth <= kMAX_GDI_RGN_COORD);
	assert(dstHeight <= kMAX_GDI_RGN_COORD);
#endif
	
	// Make sure the region isn't too big for Win32
	if (dstWidth > kMAX_GDI_RGN_COORD || dstHeight > kMAX_GDI_RGN_COORD) return;
		
	sLONG	rgnSize = ::GetRegionData(fRegion, sizeof(RGNDATA), 0);
	VHandle	vHandle = VMemory::NewHandle(rgnSize);
	VPtr	vPtr = VMemory::LockHandle(vHandle);
		
	rgnSize = ::GetRegionData(fRegion, rgnSize, (RGNDATA*)vPtr);
	
	// Create a scale matrix
	XFORM	xForm;
	xForm.eM11 = dstWidth / fBounds.GetWidth(); 
    xForm.eM12 = 0.0;   
    xForm.eM21 = 0.0;   
    xForm.eM22 = dstHeight / fBounds.GetHeight();
    xForm.eDx = 0.0;  
    xForm.eDy = 0.0;

	HRGN	tempRgn = ::ExtCreateRegion(&xForm, rgnSize, (RGNDATA*)vPtr);
	if (tempRgn != NULL)
	{
		_Release();
		fRegion = tempRgn;
		
		// Region scaling move origin, so reset it
		RECT	rgnRect = { 0, 0, 0, 0 };
		::GetRgnBox(fRegion, &rgnRect);
		::OffsetRgn(fRegion, -rgnRect.left, -rgnRect.top);
		fOffset.SetPosBy( rgnRect.left, rgnRect.top);
	}
	
	VMemory::DisposeHandle(vHandle);
	
#endif

	_ComputeBounds();
}


Boolean VRegion::HitTest (const VPoint& inPoint) const
{
	if (fRegion == NULL) return false;
		
#if !USE_GDIPLUS
	VPoint	testPoint(inPoint);
	testPoint -= fOffset;
	
	if (testPoint.GetX() > kMAX_GDI_RGN_COORD || testPoint.GetY() > kMAX_GDI_RGN_COORD)
		return false;
	else
		return (::PtInRegion(fRegion, testPoint.GetX(), testPoint.GetY()) != 0);
#else
	return fRegion->IsVisible((INT)inPoint.GetX(), (INT)inPoint.GetY());
#endif
}


void VRegion::Inset (GReal inHoriz, GReal inVert)
{
	if (fRegion == NULL)
		SetEmpty();
		
#if !USE_GDIPLUS

	GReal	dstRight = fBounds.GetRight() + inHoriz;
	GReal	dstLeft = fBounds.GetLeft() + inHoriz;
	GReal	dstTop = fBounds.GetTop() + inVert;
	GReal	dstBottom = fBounds.GetBottom() + inVert;
	
#if DEBUG_GDI_LIMITS
	assert(Abs(dstRight) <= kMAX_GDI_RGN_COORD);
	assert(Abs(dstLeft) <= kMIN_GDI_RGN_COORD);
	assert(Abs(dstTop) <= kMIN_GDI_RGN_COORD);
	assert(Abs(dstBottom) <= kMAX_GDI_RGN_COORD);
#endif
	
	// Make sure the region isn't too big for Win32
	if (dstRight > kMAX_GDI_RGN_COORD || dstLeft < kMIN_GDI_RGN_COORD
		|| dstBottom > kMAX_GDI_RGN_COORD || dstTop < kMIN_GDI_RGN_COORD) return;
		
	sLONG	rgnSize = ::GetRegionData(fRegion, sizeof(RGNDATA), 0);
	VHandle	vHandle = VMemory::NewHandle(rgnSize);
	VPtr	vPtr = VMemory::LockHandle(vHandle);
		
	// Create a translate matrix
	rgnSize = ::GetRegionData(fRegion, rgnSize, (RGNDATA*)vPtr);
	
	XFORM xForm;
	xForm.eM11 = inHoriz; 
    xForm.eM12 = 0.0;   
    xForm.eM21 = 0.0;   
    xForm.eM22 = inVert;
    xForm.eDx = 0.0;  
    xForm.eDy = 0.0;
    
	HRGN	tempRgn = ::ExtCreateRegion(&xForm, rgnSize, (RGNDATA*)vPtr);
	if (tempRgn != NULL)
	{
		_Release();
		fRegion = tempRgn;
	}
	
	VMemory::DisposeHandle(vHandle);
	
	_AdjustOrigin();
#else
	Gdiplus::Matrix mat;
	
	mat.Translate(inHoriz,inVert);
	mat.Scale((fBounds.GetWidth()-inHoriz)/(GReal)fBounds.GetWidth(),(fBounds.GetHeight()-inVert)/(GReal)fBounds.GetHeight());
	
	fRegion->Transform(&mat);
#endif

	_ComputeBounds();
}

	
void VRegion::SetEmpty()
{
#if !USE_GDIPLUS
	sLONG	test = ::DeleteObject(fRegion);
	
	fRegion = ::CreateRectRgn(0, 0, 0, 0);
	fOffset.SetPosTo(0, 0);
	fBounds.SetCoords(0.0, 0.0, 0.0, 0.0);
#else
	fRegion->MakeEmpty();
#endif
	fBounds.SetCoords(0.0, 0.0, 0.0, 0.0);
}


Boolean VRegion::IsEmpty() const
{
	if (fRegion == NULL) return true;
	
#if !USE_GDIPLUS
	RECT	bounds;
	return (::GetRgnBox(fRegion, &bounds) == NULLREGION);
#else
	Boolean	result = true;
	HDC	dc = ::GetDC(NULL);
	Graphics graf(dc);
	result = fRegion->IsEmpty(&graf);
	::ReleaseDC(NULL,dc);
	return result;
#endif
}


void VRegion::FromRegion (const VRegion& inOriginal)
{
	_Release();
	
#if !USE_GDIPLUS
	fRegion = ::CreateRectRgn(0, 0, 0, 0);
	sLONG	test = ::CombineRgn(fRegion, inOriginal.fRegion, 0, RGN_COPY);
	
	fOffset = inOriginal.fOffset;
#else
	fRegion = inOriginal.fRegion->Clone();
#endif

	_ComputeBounds();
}


void VRegion::FromRect (const VRect& inRect)
{
	_Release();
	

#if !USE_GDIPLUS

#if DEBUG_GDI_LIMITS
	assert(inRect.GetWidth() <= kMAX_GDI_RGN_COORD);
	assert(inRect.GetHeight() <= kMAX_GDI_RGN_COORD);
#endif
	VRect bounds(inRect);
	bounds.NormalizeToInt(); //normalize to prevent artifacts due to invalidating or clipping region being cropped down

	// Make sure region isn't too big for GDI
	sLONG	newWidth = Min((sLONG)bounds.GetWidth(), (sLONG)kMAX_GDI_RGN_COORD);
	sLONG	newHeight = Min((sLONG)bounds.GetHeight(), (sLONG)kMAX_GDI_RGN_COORD);
	fRegion = ::CreateRectRgn(0, 0, newWidth, newHeight);
	
	fOffset.SetPosTo(bounds.GetX(), bounds.GetY());
#else
	Gdiplus::RectF r(inRect.GetX(),inRect.GetY(),inRect.GetWidth(),inRect.GetHeight());
	fRegion = new Gdiplus::Region(r);
#endif

	_ComputeBounds();
}


void VRegion::FromPolygon (const VPolygon& inPolygon)
{
	_Release();
	
	VPolygon*	constPoly = (VPolygon*) &inPolygon;
	
	const POINT* firstPt = constPoly->GetNativePoints();
	fRegion = ::CreatePolygonRgn(firstPt, constPoly->GetPointCount(), WINDING);
	
	// Note: polygon doesnt actually use fOffset. Use AdjustOrigin instead.
	fOffset.SetPosTo(0, 0);
	_AdjustOrigin();

	_ComputeBounds();
}


void VRegion::FromRegionRef(RgnRef inRgn, Boolean inOwnIt)
{
	_Release();
	
	if (inOwnIt)
	{
	#if !USE_GDIPLUS
		fRegion = inRgn;
		fOffset.SetPosTo(0, 0);
		_AdjustOrigin();
	#else
		fRegion = new Gdiplus::Region(inRgn);
		::DeleteObject(inRgn);
	#endif
	}
	else
	{
	#if !USE_GDIPLUS
		if (fRegion == NULL)
			fRegion = ::CreateRectRgn(0, 0, 0, 0);
		::CombineRgn(fRegion, inRgn, 0, RGN_COPY);
		
		fOffset.SetPosTo(0, 0);
		_AdjustOrigin();
	#else
		fRegion = new Gdiplus::Region(inRgn);
	#endif
	}
	
	_ComputeBounds();
}


Boolean VRegion::operator == (const VRegion& inRgn) const
{
	Boolean	test = false;
	if (IsEmpty() || inRgn.IsEmpty())
	{
		test = (IsEmpty() && inRgn.IsEmpty());
	}
	else
	{
		VRegion regionXor(*this);
		regionXor.Xor( inRgn);
		return regionXor.IsEmpty();
		/*
		assert(false);
		test = true;
		test &= fOffset.GetX() == inRgn.fOffset.GetX();
		test &= fOffset.GetY() == inRgn.fOffset.GetY();
		*/
	}
	
	return test;
}


void VRegion::Xor(const VRegion& inRgn)
{
	if (fRegion == NULL)
	{
		FromRegion(inRgn);
		return;
	}
	
#if !USE_GDIPLUS
	sLONG	dX = inRgn.fOffset.GetX() - fOffset.GetX();
	sLONG	dY = inRgn.fOffset.GetY() - fOffset.GetY();
	
	if (Abs(dX) > kMAX_GDI_RGN_COORD || Abs(dY) > kMAX_GDI_RGN_COORD)
	{
		// If regions are too far, xor is same as union (no intersection)
		Union(inRgn);
		return;
	}
	else
	{
		::OffsetRgn(fRegion, -dX, -dY);
		fOffset.SetPosBy(dX, dY);
		
		sLONG	test = ::CombineRgn(fRegion, fRegion, inRgn.fRegion, RGN_XOR);
	}
	
	_AdjustOrigin();
#else
	fRegion->Xor(inRgn);
#endif

	_ComputeBounds();
}


void VRegion::Substract(const VRegion& inRgn)
{
	if (fRegion == NULL)
	{
		SetEmpty();
		return;
	}
	
#if !USE_GDIPLUS
	sLONG	dX = inRgn.fOffset.GetX() - fOffset.GetX();
	sLONG	dY = inRgn.fOffset.GetY() - fOffset.GetY();
	
	// If regions are too far, no difference
	if (Abs(dX) > kMAX_GDI_RGN_COORD || Abs(dY) > kMAX_GDI_RGN_COORD)
	{
		return;
	}
	else
	{
		::OffsetRgn(fRegion, -dX, -dY);
		fOffset.SetPosBy(dX, dY);
		
		sLONG	test = ::CombineRgn(fRegion, fRegion, inRgn.fRegion, RGN_DIFF);
	}
	
	_AdjustOrigin();
#else
	fRegion->Exclude(inRgn);
#endif

	_ComputeBounds();
}


void VRegion::Intersect(const VRegion& inRgn)
{
	if (fRegion == NULL)
	{
		SetEmpty();
		return;
	}
	
#if !USE_GDIPLUS
	sLONG	dX = inRgn.fOffset.GetX() - fOffset.GetX();
	sLONG	dY = inRgn.fOffset.GetY() - fOffset.GetY();
	
	// If regions are too far, intersection is empty
	if (Abs(dX) > kMAX_GDI_RGN_COORD || Abs(dY) > kMAX_GDI_RGN_COORD)
	{
		SetEmpty();
		return;
	}
	else
	{
		::OffsetRgn(fRegion, -dX, -dY);
		fOffset.SetPosBy(dX, dY);
		
		sLONG	test = ::CombineRgn(fRegion, fRegion, inRgn.fRegion, RGN_AND);
	}
	
	_AdjustOrigin();
#else
	fRegion->Intersect(inRgn);
#endif

	_ComputeBounds();
}


void VRegion::Union(const VRegion& inRgn)
{
	if (inRgn.IsEmpty())
		return;

	if (IsEmpty())
	{
		FromRegion(inRgn);
		return;
	}
	
#if !USE_GDIPLUS
	
	sLONG	dX = inRgn.fOffset.GetX() - fOffset.GetX();
	sLONG	dY = inRgn.fOffset.GetY() - fOffset.GetY();
	
	// If regions are too far, union is too big for GDI
	if (Abs(dX) > kMAX_GDI_RGN_COORD || Abs(dY) > kMAX_GDI_RGN_COORD)
	{
		// Leave as is
		return;
	}
	else
	{
		::OffsetRgn(fRegion, -dX, -dY);
		fOffset.SetPosBy(dX, dY);
		
		sLONG	test = ::CombineRgn(fRegion, fRegion, inRgn.fRegion, RGN_OR);
	}
#else
	fRegion->Union(inRgn);
#endif

	_ComputeBounds();
}


VRegion::operator const RgnRef () const
{
#if !USE_GDIPLUS
	if (fRegionBuffer == NULL)
		fRegionBuffer = ::CreateRectRgn(0, 0, 0, 0);
		
	if (fBounds.GetLeft() >= kMIN_GDI_RGN_COORD && fBounds.GetTop() >= kMIN_GDI_RGN_COORD &&
		fBounds.GetRight() <= kMAX_GDI_RGN_COORD && fBounds.GetBottom() <= kMAX_GDI_RGN_COORD &&
		fRegion != NULL)
	{
		// If rgn is out of GDI limits, leave empty
		::CombineRgn(fRegionBuffer, fRegion, 0, RGN_COPY);
		::OffsetRgn(fRegionBuffer, fOffset.GetX(), fOffset.GetY());
	}
	
#else
	if (fRegionBuffer != NULL)
		::DeleteObject(fRegionBuffer);
	
	if (fRegion != NULL)
	{
		HDC dc = ::GetDC(NULL);
		Graphics graf(dc);
		fRegionBuffer = fRegion->GetHRGN(&graf);
		::ReleaseDC(NULL,dc);
	}
	else
	{
		fRegionBuffer = ::CreateRectRgn(0, 0, 0, 0);
	}
#endif

	return fRegionBuffer;
}


#if USE_GDIPLUS

VRegion::operator const Gdiplus::Region& () const
{
	return *fRegion;	
}

VRegion::operator const Gdiplus::Region* () const
{
	return fRegion;	
}

#endif


void VRegion::_Init()
{
	fRegion = NULL;
	fRegionBuffer = NULL;
}


void VRegion::_ComputeBounds()
{
#if !USE_GDIPLUS
	RECT	winRect = {0, 0, 0, 0};
	if (fRegion != NULL)
		sLONG	test = ::GetRgnBox(fRegion, &winRect);
		
	fBounds.FromRectRef(winRect);
	fBounds.SetPosBy(fOffset.GetX(), fOffset.GetY());
#else
	Gdiplus::Rect	r(0.0, 0.0, 0.0, 0.0);
	if (fRegion != NULL)
	{
		HDC tmpdc = ::GetDC(NULL);
		Graphics	graf(tmpdc);
		fRegion->GetBounds(&r,&graf);
		::ReleaseDC(NULL,tmpdc);
	}
	
	fBounds.SetCoords(r.X, r.Y, r.Width, r.Height);
#endif
}


void VRegion::_AdjustOrigin()
{
	// Reset native region (0,0) and adjust fOffset
	RECT	rgnRect = { 0, 0, 0, 0 };
	if (fRegion != NULL)
	{
		::GetRgnBox(fRegion, &rgnRect);
		::OffsetRgn(fRegion, -rgnRect.left, -rgnRect.top);
		fOffset.SetPosBy(rgnRect.left, rgnRect.top);
	}
}


void VRegion::_Release()
{
#if !USE_GDIPLUS
	if (fRegion != NULL)
		::DeleteObject(fRegion);
#else
	if (fRegion != NULL)
		delete fRegion;
#endif

	fRegion = NULL;
}

/** return true if region is more than a single rectangle */
bool VRegion::IsComplex() const
{
	HRGN hrgn = (RgnRef)(*this);
	RECT rect;
	int type = ::GetRgnBox( hrgn, &rect);
	return (type == COMPLEXREGION);
}


#if ENABLE_D2D
/** get the complex path made from the internal complex region 
@remarks
	only for D2D impl because we can only clip with geometry paths
	& not directly with GDI region

	return NULL if region is a simple region (ie equal to region bounds)
*/
VGraphicPath *VRegion::GetComplexPath() const
{
	HRGN hrgn = (RgnRef)(*this);

	Gdiplus::Region region( hrgn);
	Gdiplus::Matrix matIdentity;
	UINT numRect = region.GetRegionScansCount(&matIdentity);
	if (numRect > 1)
	{
		VGraphicPath *path = new VGraphicPath();
		if (!path)
			return NULL;

		Gdiplus::Rect rects[8];
		Gdiplus::Rect *pRect = rects;
		if (numRect > 8)
			pRect = new Gdiplus::Rect[numRect];
		Gdiplus::Status status = region.GetRegionScans(&matIdentity, pRect, (INT *)&numRect);  
		if (status == Gdiplus::Ok)
		{
			path->Begin();
			Gdiplus::Rect *curRect = pRect;
			for (int i = 0; i < numRect; i++, curRect++)
			{
				/*
				if (i == 0)
				*/
					path->AddRect( VRect( curRect->GetLeft(), curRect->GetTop(), curRect->GetRight()-curRect->GetLeft(), curRect->GetBottom()-curRect->GetTop()));
				/*
				else
				{
					VGraphicPath pathRect;
					pathRect.Begin();
					pathRect.AddRect( VRect( curRect->GetLeft(), curRect->GetTop(), curRect->GetRight()-curRect->GetLeft(), curRect->GetBottom()-curRect->GetTop()));
					pathRect.End();

					path->Union( pathRect);
				}
				*/
			}
			path->End();
		}
		else
		{
			delete path;
			path = NULL;
		}
		if (numRect > 8)
			delete [] pRect;
		return path;
	}
	else
	   return NULL;
}
#endif


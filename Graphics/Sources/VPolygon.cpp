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
#include "VPolygon.h"

#undef min
#undef max


VPolygon::VPolygon() 
{
	fPtrNativePoints = NULL;
}


VPolygon::VPolygon(const VPolygon& inOriginal)
{
	fNativePointsInitialized = false;
	fPtrNativePoints = NULL;
	fPoints = inOriginal.fPoints;
	fBounds = inOriginal.fBounds;
}


VPolygon::VPolygon(const VectorOfPoint& inPoints, bool inClose) 
{
	fNativePointsInitialized = false;
	fPtrNativePoints = NULL;

	VectorOfPoint::const_iterator itPoint = inPoints.begin();
	for (;itPoint != inPoints.end(); itPoint++)
		AddPoint( *itPoint);

	if (inClose)
		Close();
}

VPolygon::VPolygon (const VRect& inRect)
{
	fPtrNativePoints = NULL;
	FromVRect(inRect);
}
void VPolygon::FromVRect(const VRect& inRect)
{
	Clear();
	AddPoint(VPoint(inRect.GetX(),inRect.GetY()));
	AddPoint(VPoint(inRect.GetX()+inRect.GetWidth(),inRect.GetY()));
	AddPoint(VPoint(inRect.GetX()+inRect.GetWidth(),inRect.GetY()+inRect.GetHeight()));
	AddPoint(VPoint(inRect.GetX(),inRect.GetY()+inRect.GetHeight()));
}


VPolygon::~VPolygon()
{
	_ClearNativePoints();
}

void VPolygon::AddPoint(const VPoint& inLocalPoint)
{
	fPoints.push_back( inLocalPoint);
	_ComputeBounds( inLocalPoint);
}


void VPolygon::Close()
{
	if (fPoints.size() > 1)
	{
		if (fPoints[0] != fPoints.back())
		{
			//JQ 30/10/2013: we should not pass a ref on fPoints entry here as AddPoint modifies fPoints
			VPoint pt = fPoints[0];
			AddPoint(pt);
		}
	}
}

void VPolygon::Clear()
{
	fPoints.clear();
	fBounds.SetEmpty();

	_ClearNativePoints();
}


void VPolygon::SetPosBy (GReal inHoriz, GReal inVert)
{
	VectorOfPoint::iterator itPoint = fPoints.begin();
	for (;itPoint != fPoints.end(); itPoint++)
	{
		itPoint->x += inHoriz;
		itPoint->y += inVert;
	}
	fBounds.SetPosBy(inHoriz, inVert);
}


void VPolygon::SetSizeBy (GReal inWidth, GReal inHeight)
{
	if (fBounds.GetWidth() == 0.0f || fBounds.GetHeight() == 0.0f)
		return;

	if (inWidth != 0 || inHeight != 0)
	{
		GReal	scaleX = (GReal)(inWidth + fBounds.GetWidth()) / (GReal)fBounds.GetWidth();
		GReal	scaleY = (GReal)(inHeight + fBounds.GetHeight()) / (GReal)fBounds.GetHeight();

		VectorOfPoint::iterator itPoint = fPoints.begin();
		for (;itPoint != fPoints.end(); itPoint++)
		{
			itPoint->x = fBounds.GetLeft() + (itPoint->x - fBounds.GetLeft()) * scaleX;
			itPoint->y = fBounds.GetTop() + (itPoint->y - fBounds.GetTop()) * scaleY;
		}

		fBounds.SetSizeTo(fBounds.GetWidth() * scaleX, fBounds.GetHeight() * scaleY);
	}
}


Boolean VPolygon::HitTest (const VPoint& inPoint) const
{
	// Should test is point is inside the closed polygon
	//	or on any polygon segment if opened.
	xbox_assert(false);
	return fBounds.HitTest(inPoint);
}


void VPolygon::Inset (GReal inHoriz, GReal inVert)
{
	SetPosBy(inHoriz, inVert);
	SetSizeBy(-2.0 * inHoriz, -2.0 * inVert);
}


sLONG VPolygon::GetPointCount () const
{
	return fPoints.size();
}


void VPolygon::GetNthPoint (sLONG inIndex, VPoint& outPoint) const
{
	if (!testAssert(inIndex > 0 && inIndex <= fPoints.size()))
		outPoint.SetPosTo(kMIN_sWORD, kMIN_sWORD);
	else
		outPoint = fPoints[inIndex-1];
}


VError VPolygon::ReadFromStream(VStream* ioStream, sLONG /*inParam*/)
{
	xbox_assert(ioStream != NULL);
	sLONG	sign = ioStream->GetLong();
		
	if (sign == 'POLY')
	{
		Clear();

		sWORD	i;
		sWORD	nbpoint = ioStream->GetWord();
		
		Real h, v;
		for (i = 0; i < nbpoint; i++)
		{
			h = ioStream->GetReal();
			v = ioStream->GetReal();
			AddPoint(VPoint((GReal)h, (GReal)v));
		}
	}
	
	return ioStream->GetLastError();
}


VError VPolygon::WriteToStream(VStream* ioStream, sLONG /*inParam*/) const
{
	xbox_assert(ioStream);
	
	ioStream->PutLong('POLY');
	ioStream->PutWord(GetPointCount());

	for (sLONG index = 0; index < fPoints.size(); index++)
	{
		ioStream->PutReal((Real)fPoints[index].x);
		ioStream->PutReal((Real)fPoints[index].y);
	}

	return ioStream->GetLastError();
}


void VPolygon::_ComputeBounds(const VPoint& inLastPoint)
{
	_ClearNativePoints();

	sLONG	count = fPoints.size();
	if (count == 0)
		fBounds.SetCoords(0.0, 0.0, 0.0, 0.0);
	else if (count == 1)
	{
		fBounds.SetPosTo(inLastPoint);
		fBounds.SetSizeTo(0.0, 0.0);
	}
	else
	{
		if (inLastPoint.GetX() < fBounds.GetLeft()) 
			fBounds.SetLeft(inLastPoint.GetX());
		else if (inLastPoint.GetX() > fBounds.GetRight()) 
			fBounds.SetRight(inLastPoint.GetX());
			
		if (inLastPoint.GetY() < fBounds.GetTop()) 
			fBounds.SetTop(inLastPoint.GetY());
		else if (inLastPoint.GetY() > fBounds.GetBottom()) 
			fBounds.SetBottom(inLastPoint.GetY());
	}
}


VPolygon& VPolygon::operator = (const VPolygon& inOriginal)
{
	if (this == &inOriginal)
		return *this;

	fPoints = inOriginal.fPoints;
	fBounds = inOriginal.fBounds;
	_ClearNativePoints();
	return *this;
}


Boolean VPolygon::operator == (const VPolygon& inPolygon) const
{
	if (fPoints.size() != inPolygon.fPoints.size())
		return false;

	Boolean isEqual = true;

	for (long index = 0; index < fPoints.size() && isEqual; index++)
	{
		isEqual = fPoints[index].x == inPolygon.fPoints[index].x && isEqual;
		isEqual = fPoints[index].y == inPolygon.fPoints[index].y && isEqual;
	}

	return isEqual;
}


const VPolygon::NativePoint* VPolygon::GetNativePoints() const
{
#if VERSIONWIN || VERSIONMAC
	if (fNativePointsInitialized)
		return fPtrNativePoints ? fPtrNativePoints : &(fNativePoints[0]);

	xbox_assert(fPtrNativePoints == NULL);

	NativePoint *points = fPoints.size() <= VPOLYGON_NATIVE_POINTS_FIXED_SIZE ? &(fNativePoints[0]) : NULL;
	if (points == NULL)
		//alloc dynamically
		points = fPtrNativePoints = (NativePoint *)VMemory::NewPtr( (VSize)(sizeof(NativePoint)*fPoints.size()), 'vpol');

	if (!testAssert(points))
	{
		//make it safe
		fPoints.resize( VPOLYGON_NATIVE_POINTS_FIXED_SIZE);
		points = &(fNativePoints[0]);
	}

	VectorOfPoint::iterator itPoint = fPoints.begin();
	for (;itPoint != fPoints.end(); itPoint++, points++)
	{
#if VERSIONWIN
		points->x = (LONG)floor(itPoint->x);
		points->y = (LONG)floor(itPoint->y);
#else
		points->x = itPoint->x;
		points->y = itPoint->y;
#endif
	}

	fNativePointsInitialized = true;
	
	return fPtrNativePoints ? fPtrNativePoints : &(fNativePoints[0]);
#else
	//linux (but useful only while painting) ?
	return NULL;
#endif
}

void VPolygon::_ClearNativePoints()
{
	fNativePointsInitialized = false;
	if (fPtrNativePoints)
	{
		VMemory::DisposePtr( fPtrNativePoints);
		fPtrNativePoints = NULL;
	}
}

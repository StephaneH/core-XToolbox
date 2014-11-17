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
#include "VBezier.h"
#include "VGraphicContext.h"

#define GP_BOUNDS_MAX_VALUE_DEFAULT	1000000.0f
#define GP_BOUNDS_MIN_DEFAULT VPoint( GP_BOUNDS_MAX_VALUE_DEFAULT,  GP_BOUNDS_MAX_VALUE_DEFAULT)
#define GP_BOUNDS_MAX_DEFAULT VPoint(-GP_BOUNDS_MAX_VALUE_DEFAULT, -GP_BOUNDS_MAX_VALUE_DEFAULT)

VGraphicPath::VGraphicPath(bool inComputeBoundsAccurate, bool inCreateStorageForCrispEdges)
{	
	fCreateStorageForCrispEdges = inCreateStorageForCrispEdges;
	if (fCreateStorageForCrispEdges)
		fDrawCmds.reserve(16); //slightly optimize allocation
	fHasBezier = false;

	fFillMode = FILLRULE_EVENODD;
	fTransform = CGAffineTransformIdentity;
	fPath = ::CGPathCreateMutable();

	fComputeBoundsAccurate = inComputeBoundsAccurate;
	if (fComputeBoundsAccurate)
	{
		fPathPrevPoint	= GP_BOUNDS_MIN_DEFAULT;
		fPathMin		= GP_BOUNDS_MIN_DEFAULT;
		fPathMax		= GP_BOUNDS_MAX_DEFAULT;
	}
}

VGraphicPath::VGraphicPath( const VPolygon& inPolygon)
{
	fCreateStorageForCrispEdges = true;
	fDrawCmds.reserve(16); //slightly optimize allocation
	fHasBezier = false;

	fFillMode = FILLRULE_EVENODD;
	fTransform = CGAffineTransformIdentity;
	fPath = ::CGPathCreateMutable();

	fComputeBoundsAccurate = false;

	Begin();
	sLONG count = inPolygon.GetPointCount();
	if (count >= 2)
	{
		VPoint pt;
		inPolygon.GetNthPoint( 1, pt);
		BeginSubPathAt( pt);
		VPoint ptStart = pt;
		for (sLONG index = 2; index <= count; index++)
		{
			inPolygon.GetNthPoint( index, pt);
			AddLineTo( pt);
		}
		if (pt != ptStart)
			CloseSubPath();
	}
	End();
}

VGraphicPath::VGraphicPath(const VGraphicPath& inOriginal)
{
	fFillMode = inOriginal.fFillMode;
	fTransform = inOriginal.fTransform;
	fPath = ::CGPathCreateMutableCopy(inOriginal.fPath);
	fPolygon = inOriginal.fPolygon;
	fBounds = inOriginal.fBounds;

	fComputeBoundsAccurate = inOriginal.fComputeBoundsAccurate;
	if (fComputeBoundsAccurate)
	{
		fPathMin		= inOriginal.fPathMin;		
		fPathMax		= inOriginal.fPathMax;		
	}

	fCreateStorageForCrispEdges = inOriginal.fCreateStorageForCrispEdges;
	if (fCreateStorageForCrispEdges)
	{
		fDrawCmds = inOriginal.fDrawCmds;
		fCurTransform = inOriginal.fCurTransform;
	}
	fHasBezier = inOriginal.fHasBezier;
}


VGraphicPath::~VGraphicPath()
{
	::CGPathRelease(fPath);
}


VGraphicPath *VGraphicPath::Clone() const
{
	return new VGraphicPath( *this);
}

/** return cloned graphic path with coordinates modified with crispEdges algorithm
@remarks
	depending on fCreateStorageForCrispEdges status & on VGraphicContext crispEdges status

	return NULL if crispEdges is disabled 
*/
VGraphicPath *VGraphicPath::CloneWithCrispEdges( VGraphicContext *inGC, bool inFillOnly) const
{
	if (inGC == NULL || !inGC->IsShapeCrispEdgesEnabled() || !inGC->IsAllowedCrispEdgesOnPath() || !fCreateStorageForCrispEdges || fDrawCmds.empty())
		return NULL;
	if (fHasBezier && !inGC->IsAllowedCrispEdgesOnBezier())
		return NULL;

	VGraphicPath *path = inGC->CreatePath();
	if (!path)
		return NULL;
	path->fCreateStorageForCrispEdges = false;
	path->Begin();
	VGraphicPathDrawCmds::const_iterator it = fDrawCmds.begin();
	bool isIdentity = fCurTransform.IsIdentity();
	VAffineTransform ctm;
	if (!isIdentity)
	{
		inGC->UseReversedAxis();
		inGC->GetTransform( ctm);
		inGC->SetTransform( fCurTransform * ctm);
	}
	for (;it != fDrawCmds.end(); it++)
	{
		switch (it->GetType())
		{
		case GPDCT_BEGIN_SUBPATH_AT:
			{
			VPoint pt(*(it->GetPoints()));
			inGC->UseReversedAxis();
			inGC->CEAdjustPointInTransformedSpace( pt, inFillOnly);
			path->BeginSubPathAt( pt);
			}
			break;
		case GPDCT_ADD_LINE_TO:
			{
			VPoint pt(*(it->GetPoints()));
			inGC->UseReversedAxis();
			inGC->CEAdjustPointInTransformedSpace( pt, inFillOnly);
			path->AddLineTo( pt);
			}
			break;
		case GPDCT_ADD_BEZIER_TO:
			{
			VPoint c1(*(it->GetPoints()));
			VPoint d(*(it->GetPoints()+1));
			VPoint c2(*(it->GetPoints()+2));

			inGC->UseReversedAxis();
			inGC->CEAdjustPointInTransformedSpace( c1, inFillOnly);
			inGC->CEAdjustPointInTransformedSpace( d, inFillOnly);
			inGC->CEAdjustPointInTransformedSpace( c2, inFillOnly);
			path->AddBezierTo( c1, d, c2);
			}
			break;
		case GPDCT_CLOSE_SUBPATH:
			path->CloseSubPath();
			break;
		case GPDCT_ADD_OVAL:
			{
			VRect rect(it->GetRect());
			inGC->UseReversedAxis();
			inGC->CEAdjustRectInTransformedSpace( rect, inFillOnly);
			path->AddOval( rect);
			}
			break;
		case GPDCT_SET_POS_BY:
			path->SetPosBy( it->GetPoints()->GetX(), it->GetPoints()->GetY());
			break;
		case GPDCT_SET_SIZE_BY:
			path->SetSizeBy( it->GetPoints()->GetX(), it->GetPoints()->GetY());
			break;
		default:
			xbox_assert(false);
			break;
		}
	}
	if (!isIdentity)
	{
		inGC->UseReversedAxis();
		inGC->SetTransform( ctm);
	}
	path->End();
	return path;
}


void VGraphicPath::BeginSubPathAt(const VPoint& inLocalPoint)
{
	::CGPathMoveToPoint(fPath, NULL, inLocalPoint.GetX(), inLocalPoint.GetY());
	fPolygon.AddPoint(inLocalPoint);

	if (fComputeBoundsAccurate)
		_addPointToBounds( VPoint( (SmallReal)inLocalPoint.GetX(), (SmallReal)inLocalPoint.GetY()));

	if (fCreateStorageForCrispEdges)
		fDrawCmds.push_back( VGraphicPathDrawCmd( GPDCT_BEGIN_SUBPATH_AT, inLocalPoint));
}


void VGraphicPath::AddLineTo(const VPoint& inDestPoint)
{
	::CGPathAddLineToPoint(fPath, NULL, inDestPoint.GetX(), inDestPoint.GetY());
	fPolygon.AddPoint(inDestPoint);

	if (fComputeBoundsAccurate)
		_addPointToBounds( VPoint( (SmallReal)inDestPoint.GetX(), (SmallReal)inDestPoint.GetY()));
	_ComputeBounds();

	if (fCreateStorageForCrispEdges)
		fDrawCmds.push_back( VGraphicPathDrawCmd( GPDCT_ADD_LINE_TO, inDestPoint));
}


void VGraphicPath::AddBezierTo(const VPoint& inStartControl, const VPoint& inDestPoint, const VPoint& inDestControl)
{
	::CGPathAddCurveToPoint(fPath, NULL, inStartControl.GetX(), inStartControl.GetY(), inDestControl.GetX(), inDestControl.GetY(), inDestPoint.GetX(), inDestPoint.GetY());
	fPolygon.AddPoint(inDestPoint);

	if (fComputeBoundsAccurate)
		_addBezierToBounds( VPoint( (SmallReal)inStartControl.GetX(), (SmallReal)inStartControl.GetY()),
							VPoint( (SmallReal)inDestControl.GetX(), (SmallReal)inDestControl.GetY()),
							VPoint( (SmallReal)inDestPoint.GetX(), (SmallReal)inDestPoint.GetY()));
	_ComputeBounds();

	fHasBezier = true;
	if (fCreateStorageForCrispEdges)
		fDrawCmds.push_back( VGraphicPathDrawCmd( GPDCT_ADD_BEZIER_TO, inStartControl, inDestPoint, inDestControl));
}


void VGraphicPath::CloseSubPath()
{
	::CGPathCloseSubpath(fPath);

	if (fCreateStorageForCrispEdges)
		fDrawCmds.push_back( VGraphicPathDrawCmd( GPDCT_CLOSE_SUBPATH));
}


void VGraphicPath::Clear()
{
	::CGPathRelease(fPath);
	fPath = ::CGPathCreateMutable();
	fPolygon.Clear();

	if (fComputeBoundsAccurate)
	{
		fPathPrevPoint	= GP_BOUNDS_MIN_DEFAULT;
		fPathMin		= GP_BOUNDS_MIN_DEFAULT;
		fPathMax		= GP_BOUNDS_MAX_DEFAULT;
	}
	fBounds			= VRect(0,0,0,0);

	fDrawCmds.clear();
	fCurTransform.MakeIdentity();
	fHasBezier = false;
}


void VGraphicPath::AddOval(const VRect& inBounds)
{
	if (fComputeBoundsAccurate)
		_addRectToBounds( inBounds);

#if MAC_OS_X_VERSION_MAX_ALLOWED > MAC_OS_X_VERSION_10_3
	if (CGPathAddEllipseInRect != NULL)
	{
		// Only available under 10.4
		::CGPathAddEllipseInRect(fPath, NULL, inBounds);
		_ComputeBounds();
		return;
	}
#endif

	const GReal	cpRatio = (GReal) 0.55;
	GReal	top = inBounds.GetTop();
	GReal	left = inBounds.GetLeft();
	GReal	bottom = inBounds.GetBottom();
	GReal	right = inBounds.GetRight();
	GReal	halfWidth = inBounds.GetWidth() / 2;
	GReal	halfHeight = inBounds.GetHeight() / 2;
	
	::CGPathMoveToPoint(fPath, NULL, left, top + halfHeight);
	::CGPathAddCurveToPoint(fPath, NULL, left, top + halfHeight + cpRatio * halfHeight, left + halfWidth - cpRatio * halfWidth, bottom, left + halfWidth, bottom);
	::CGPathAddCurveToPoint(fPath, NULL, left + halfWidth + cpRatio * halfWidth, bottom, right, top + halfHeight + cpRatio * halfHeight, right, top + halfHeight);
	::CGPathAddCurveToPoint(fPath, NULL, right, top + halfHeight - cpRatio * halfHeight, left + halfWidth + cpRatio * halfWidth, top, left + halfWidth, top);
	::CGPathAddCurveToPoint(fPath, NULL, left + halfWidth - cpRatio * halfWidth, top, left, top + halfHeight - cpRatio * halfHeight, left, top + halfHeight);
	::CGPathCloseSubpath(fPath);

	fPolygon.AddPoint(inBounds.GetTopLeft());
	fPolygon.AddPoint(inBounds.GetTopRight());
	fPolygon.AddPoint(inBounds.GetBotRight());
	fPolygon.AddPoint(inBounds.GetBotLeft());
	_ComputeBounds();

	if (fCreateStorageForCrispEdges)
		fDrawCmds.push_back( VGraphicPathDrawCmd( GPDCT_ADD_OVAL, inBounds));
}


void VGraphicPath::SetPosBy(GReal inHoriz, GReal inVert)
{
	if (inHoriz == 0.0 && inVert == 0.0) return;
	
	fTransform = ::CGAffineTransformTranslate(fTransform, inHoriz, inVert);
	fBounds.SetPosBy(inHoriz, inVert);
	fPolygon.SetPosBy(inHoriz, inVert);

	if (fComputeBoundsAccurate)
	{
		VPoint translate( (SmallReal)inHoriz, (SmallReal)inVert);

		if (fPathMin != GP_BOUNDS_MIN_DEFAULT)
			fPathMin += translate;
		if (fPathMax != GP_BOUNDS_MAX_DEFAULT)
			fPathMax += translate;
	}

	if (fCreateStorageForCrispEdges)
	{
		fDrawCmds.push_back( VGraphicPathDrawCmd( GPDCT_SET_POS_BY, inHoriz, inVert));
		fCurTransform.Translate( inHoriz, inVert, VAffineTransform::MatrixOrderAppend);
	}
}


void VGraphicPath::SetSizeBy(GReal inWidth, GReal inHeight)
{
	if (inWidth == 0 && inHeight == 0) return;
	
	fTransform = ::CGAffineTransformScale(fTransform, 1 + inWidth / fBounds.GetWidth(), 1 + inHeight / fBounds.GetHeight());
	fBounds.SetSizeBy(inWidth, inHeight);
	fPolygon.SetSizeBy(inWidth, inHeight);

	if (fComputeBoundsAccurate)
	{
		VPoint inflate( (SmallReal)inWidth, (SmallReal)inHeight);

		if (fPathMax != GP_BOUNDS_MAX_DEFAULT)
			fPathMax += inflate;
	}

	if (fCreateStorageForCrispEdges)
	{
		fDrawCmds.push_back( VGraphicPathDrawCmd( GPDCT_SET_SIZE_BY, inWidth, inHeight));
		fCurTransform.Scale( 1 + inWidth / fBounds.GetWidth(), 1 + inHeight / fBounds.GetHeight(), VAffineTransform::MatrixOrderAppend);
	}
}


Boolean VGraphicPath::HitTest(const VPoint& inPoint) const
{
#if MAC_OS_X_VERSION_MAX_ALLOWED > MAC_OS_X_VERSION_10_3

	VIndex m = fPolygon.GetPointCount();
	VPoint point;
	VPoint last;
	fPolygon.GetNthPoint(1,last);
	for(VIndex n = 2;n <=m ;n++)
	{
		fPolygon.GetNthPoint(n,point);
		if (Abs(point.GetX() - last.GetX()) <4) // 2 points aligned V
		{
			//Compare Y
			if (Abs(inPoint.GetX() - point.GetX())<4) // hit point also aligned V
				if (inPoint.GetY()>Min(point.GetY(),last.GetY())) // hit point in the gap ?
				if (inPoint.GetY()<Max(point.GetY(),last.GetY())) // hit point in the gap ?
					return true;
		}
		if (Abs(point.GetY() - last.GetY()) <4) // 2 points aligned H
		{
			//Compare X
			if (Abs(inPoint.GetY() - point.GetY())<4) // hit point also aligned V
				if (inPoint.GetX()>Min(point.GetX(),last.GetX())) // hit point in the gap ?
				if (inPoint.GetX()<Max(point.GetX(),last.GetX())) // hit point in the gap ?
				return true;
		}
		last = point;
	}
	return false;
/*	CGRect rect = CGPathGetBoundingBox(fPath);
	CGPathCloseSubpath(fPath);
	return (CGPathContainsPoint != NULL) ? ::CGPathContainsPoint(fPath, &fTransform, inPoint, false) : fBounds.HitTest(inPoint);*/
#else
	return fBounds.HitTest(inPoint);
#endif
}

Boolean VGraphicPath::ScaledHitTest(const VPoint& inPoint, GReal scale) const
{
	return HitTest(inPoint);
}


void VGraphicPath::Inset(GReal inHoriz, GReal inVert)
{
	SetPosBy(inHoriz, inVert);
	SetSizeBy(-2 * inHoriz, -2 * inVert);
}


void VGraphicPath::_ComputeBounds()
{
	if (fComputeBoundsAccurate)
	{
		if (fPathMin == GP_BOUNDS_MIN_DEFAULT
			||
			fPathMax == GP_BOUNDS_MAX_DEFAULT)
			fBounds = VRect(0,0,0,0);
		else
			fBounds.SetCoords(	(GReal)fPathMin.x, 
								(GReal)fPathMin.y, 
								(GReal)(fPathMax.x-fPathMin.x), 
								(GReal)(fPathMax.y-fPathMin.y));
	}
	else
	{
		CGRect	bounds = ::CGPathGetBoundingBox(fPath);
		fBounds.FromRectRef(bounds);
	}
}


VGraphicPath& VGraphicPath::operator = (const VGraphicPath& inOriginal)
{	
	fTransform = inOriginal.fTransform;
	fBounds = inOriginal.fBounds;
	fPath = ::CGPathCreateMutableCopy(inOriginal.fPath);

	fComputeBoundsAccurate = inOriginal.fComputeBoundsAccurate;
	if (fComputeBoundsAccurate)
	{
		fPathMin		= inOriginal.fPathMin;		
		fPathMax		= inOriginal.fPathMax;		
	}

	fCreateStorageForCrispEdges = inOriginal.fCreateStorageForCrispEdges;
	if (fCreateStorageForCrispEdges)
	{
		fDrawCmds = inOriginal.fDrawCmds;
		fCurTransform = inOriginal.fCurTransform;
	}
	fHasBezier = inOriginal.fHasBezier;

	return *this;
}


Boolean VGraphicPath::operator == (const VGraphicPath& inBezier) const
{	
	return	(::CGPathEqualToPath(fPath, inBezier.fPath)
			 && 
			 fComputeBoundsAccurate == inBezier.fComputeBoundsAccurate
			 &&
			 fCreateStorageForCrispEdges == inBezier.fCreateStorageForCrispEdges);
}


/* add arc to the specified graphic path
@param inCenter
	arc ellipse center
@param inDestPoint
	dest point of the arc
@param inRX
	arc ellipse radius along x axis
@param inRY
	arc ellipse radius along y axis
	if inRY <= 0, inRY = inRX
@param inLargeArcFlag (default false)
	true for draw largest arc
@param inXAxisRotation (default 0.0f)
	arc x axis rotation in degree
@remarks
	this method add a arc starting from current pos to the dest point 

	use MeasureArc method to get start & end point from ellipse start & end angle in degree
*/
void VGraphicPath::AddArcTo(const VPoint& inCenter, const VPoint& inDestPoint, GReal _inRX, GReal _inRY, bool inLargeArcFlag, GReal inXAxisRotation)
{
	//we need double precision here
	GReal inRx = _inRX, inRy = _inRY; 
	VPoint srcPoint;
	fPolygon.GetNthPoint( fPolygon.GetPointCount(), srcPoint);
	GReal inX = srcPoint.GetX(), inY = srcPoint.GetY();
	GReal inEX = inDestPoint.GetX(), inEY = inDestPoint.GetY();
	
	GReal sin_th, cos_th;
    GReal a00, a01, a10, a11;
    GReal x0, y0, x1, y1, xc, yc;
    GReal d, sfactor, sfactor_sq;
    GReal th0, th1, th_arc;
    sLONG i, n_segs;
    GReal dx, dy, dx1, dy1, Pr1, Pr2, Px, Py, check;

    inRx = std::fabs(inRx);
    inRy = std::fabs(inRy);

    sin_th = std::sin(inXAxisRotation * ((GReal) PI / 180));
    cos_th = std::cos(inXAxisRotation * ((GReal) PI / 180));

    dx = (inX - inEX) / 2;
    dy = (inY - inEY) / 2;
    dx1 =  cos_th * dx + sin_th * dy;
    dy1 = -sin_th * dx + cos_th * dy;
    Pr1 = inRx * inRx;
    Pr2 = inRy * inRy;
    Px = dx1 * dx1;
    Py = dy1 * dy1;
    /* Spec : check if radii are large enough */
    check = Px / Pr1 + Py / Pr2;
    if (check > 1) {
        inRx = inRx * std::sqrt(check);
        inRy = inRy * std::sqrt(check);
    }

    a00 =  cos_th / inRx;
    a01 =  sin_th / inRx;
    a10 = -sin_th / inRy;
    a11 =  cos_th / inRy;
    x0 = a00 * inX + a01 * inY;
    y0 = a10 * inX + a11 * inY;
    x1 = a00 * inEX + a01 * inEY;
    y1 = a10 * inEX + a11 * inEY;
    xc = a00 * inCenter.GetX() + a01 * inCenter.GetY();
    yc = a10 * inCenter.GetX() + a11 * inCenter.GetY();

    /* (x0, y0) is current point in transformed coordinate space.
       (x1, y1) is new point in transformed coordinate space.
       (xc, yc) is new center in transformed coordinate space.

       The arc fits a unit-radius circle in this space.
    */

	/*
    d = (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0);
    sfactor_sq = 1.0 / d - 0.25;
    if (sfactor_sq < 0) sfactor_sq = 0;
    sfactor = sqrt(sfactor_sq);
    if (inSweepFlag == inLargeArcFlag) sfactor = -sfactor;

    xc = 0.5 * (x0 + x1) - sfactor * (y1 - y0);
    yc = 0.5 * (y0 + y1) + sfactor * (x1 - x0);
	*/
    /* (xc, yc) is center of the circle. */

    th0 = std::atan2(y0 - yc, x0 - xc);
    th1 = std::atan2(y1 - yc, x1 - xc);

    th_arc = th1 - th0;
	if (th0 <= 0 && th1 > 0 && std::fabs(th_arc-PI) <= (GReal) 0.00001)
		th_arc = -th_arc;	//ensure we draw counterclockwise if angle delta = 180°
	else if (std::fabs(th_arc)-(GReal)PI > (GReal) 0.00001 && !inLargeArcFlag)
	{
		if (th_arc > 0)
			th_arc -= 2 * (GReal) PI;
		else
			th_arc += 2 * (GReal) PI;
	}
	else if (std::fabs(th_arc) <= (GReal) PI && inLargeArcFlag)
	{
		if (th_arc > 0)
			th_arc -= 2 * (GReal) PI;
		else
			th_arc += 2 * (GReal) PI;
	}

	//split arc in small arcs <= 90°

    n_segs = sLONG(std::ceil(fabs(th_arc / (PI * 0.5 + 0.001))));
    for (i = 0; i < n_segs; i++) 
	{
        _PathAddArcSegment(	xc, yc,
							th0 + i * th_arc / n_segs,
							th0 + (i + 1) * th_arc / n_segs,
							inRx, inRy, 
							inXAxisRotation);
    }
}


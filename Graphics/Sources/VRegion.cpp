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


VRegion::VRegion(const VRegion* inOriginal)
{
	_Init();
	
	// Don't create a region avoiding releasing it when the new one is set
	if (inOriginal != NULL)
		FromRegion(*inOriginal);
	else
		SetEmpty();	// important to properly initialize mac implementation
}


VRegion::VRegion(const VRegion& inOriginal)
{
	_Init();
	FromRegion(inOriginal);
}


VRegion::VRegion(RgnRef	inRgn, bool inOwnIt)
{
	_Init();
	FromRegionRef(inRgn, inOwnIt);
}


VRegion::VRegion(const VRect& inRect)
{
	_Init();
	FromRect(inRect);
}


VRegion::VRegion(const VPolygon& inPolygon)
{
	_Init();
	FromPolygon(inPolygon);
}


Boolean	VRegion::Overlap(const VRegion& inRgn)
{
	VRegion	tempRgn(this);
	tempRgn.Intersect(inRgn);
	return !tempRgn.IsEmpty();
}

/* ----- VSHAPE ----- */

VShape::VShape()
{
}

VShape::VShape(const VShape& inOriginal)
{
}

void VShape::SetTriangleTo(const VPoint& inPointA, const VPoint& inPointB, const VPoint& inPointC)
{
}

void VShape::SetCircleTo(const VPoint& inCenter, const GReal inSize)
{
}

VShape::~VShape()
{
}

/* ----- VTRIANGLE ----- */

VTriangle::VTriangle()
{
	fA = VPoint(0,0);
	fB = VPoint(0,0);
	fC = VPoint(0,0);
}

VTriangle::VTriangle(const VPoint& inPointA, const VPoint& inPointB, const VPoint& inPointC)
{
	SetTriangleTo(inPointA, inPointB, inPointC);
}

void VTriangle::SetTriangleTo(const VPoint& inPointA, const VPoint& inPointB, const VPoint& inPointC)
{
	fA = inPointA;
	fB = inPointB;
	fC = inPointC;
}

VTriangle::VTriangle(const VTriangle& inOriginal)
{
	fA = inOriginal.fA;
	fB = inOriginal.fB;
	fC = inOriginal.fC;
}

VTriangle::~VTriangle()
{
}

Boolean VTriangle::HitTest(const VPoint& inPoint) const
{
	// JM 271206 d'après code : http://forum.games-creators.org/archive/index.php/t-2419.html
	LINE_t ab, bc, ca;

	ab.a = fA; ab.b = fB;
	bc.a = fB; bc.b = fC;
	ca.a = fC; ca.b = fA;

	const int pab = Side( inPoint, &ab );
	const int pbc = Side( inPoint, &bc );
	const int pca = Side( inPoint, &ca );

	if ( (0 < pab) && (0 < pbc) && (0 < pca) )
	return true; 
	if ( (0 > pab) && (0 > pbc) && (0 > pca) )
	return true; 

	if ( (0 <= pab) && (0 <= pbc) && (0 <= pca) )
	return false;
	if ( (0 >= pab) && (0 >= pbc) && (0 >= pca) )
	return false; 

	return false;
}
 
int VTriangle::Side(const VPoint &p, LINE_t *e ) const
{
	VPoint p1 = p;
	VPoint p2 = e->a;
	VPoint p3 = e->b;

	const int n = p1.x * (p2.y - p3.y) + p2.x * (p3.y - p1.y) + p3.x * (p1.y - p2.y);
	if ( n > 0 ) return 1; 
	else if ( n < 0 ) return -1; 
	else return 0; 
}

VError VTriangle::ReadFromStream(VStream* ioStream, sLONG /*inParam*/)
{
	assert(ioStream != NULL);
	
	return VE_OK;
}

VError VTriangle::WriteToStream(VStream* ioStream, sLONG /*inParam*/) const
{
	assert(ioStream != NULL);
	
	return VE_OK;
}

/* ----- VCIRCLE ----- */

VCircle::VCircle()
{
}

VCircle::VCircle(const VPoint& inCenter, const GReal inSize)
{
	SetCircleTo(inCenter, inSize);
}

void VCircle::SetCircleTo(const VPoint& inCenter, const GReal inSize)
{
	fO = inCenter;
	fR = inSize;
	fR2 = fR * fR;
}

VCircle::VCircle(const VCircle& inOriginal)
{
	fO = inOriginal.fO;
	fR = inOriginal.fR;
	fR2 = fR * fR;
}

VCircle::~VCircle()
{
}

Boolean VCircle::HitTest(const VPoint& inPoint) const
{
	GReal d2 = (inPoint.x - fO.x) * (inPoint.x - fO.x) + (inPoint.y - fO.y) * (inPoint.y - fO.y);
	return (d2 <= fR2);
}

VError VCircle::ReadFromStream(VStream* ioStream, sLONG /*inParam*/)
{
	assert(ioStream != NULL);
	
	return VE_OK;
}

VError VCircle::WriteToStream(VStream* ioStream, sLONG /*inParam*/) const
{
	assert(ioStream != NULL);
	
	return VE_OK;
}

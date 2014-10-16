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

BEGIN_TOOLBOX_NAMESPACE

/*
	Because there's currently no way to scale an HIShape, we keep the tree of operations on HIRects to be able to scale them at will.
*/

class VRegionNode : public VObject
{
public:
	typedef enum VRegionOpCode
	{
		INTERSECT,
		UNION,
		SUBSTRACT
	} VRegionOpCode;

	virtual	void SetPosBy( GReal inHoriz, GReal inVert) {;}
	virtual void SetSizeBy( GReal inHoriz, GReal inVert) {;}
	virtual VRegionNode* Clone() { return new VRegionNode;}

	virtual	bool UnionRect( const VRect& inRegion)	{ return false;}
	
	virtual	bool IsRect( VRect& outRect)			{ outRect.SetEmpty(); return true;}
	
	virtual HIShapeRef	CreateShape()
	{
		return ::HIShapeCreateEmpty();
	}
};

class VRegionNode_rect : public VRegionNode
{
public:
	VRegionNode_rect( const CGRect& inBounds):fBounds( inBounds)	{;}

	virtual	void SetPosBy( GReal inHoriz, GReal inVert)
	{
		fBounds.origin.x += inHoriz;
		fBounds.origin.y += inVert;
	}

	virtual void SetSizeBy( GReal inHoriz, GReal inVert)
	{
		fBounds.size.width += inHoriz;
		fBounds.size.height += inVert;
	}

	virtual VRegionNode_rect* Clone()
	{
		return new VRegionNode_rect( fBounds);
	}

	virtual	bool IsRect( VRect& outRect)			{ outRect.MAC_FromCGRect( fBounds); return true;}

	virtual	bool UnionRect( const VRect& inRect)
	{
		CGRect r = inRect;

		if (CGRectIsEmpty( r) || CGRectContainsRect( fBounds, r))
			return true;
		
		bool ok;

		// check if union of two rectangles is a rectangle

		if ( (r.size.width == fBounds.size.width) && (r.origin.x == fBounds.origin.x) )
		{
			ok = ( ( (r.origin.y >= fBounds.origin.y) && (r.origin.y <= fBounds.origin.y + fBounds.size.height) )
				|| ( (r.origin.y + r.size.height >= fBounds.origin.y) && (r.origin.y + r.size.height <= fBounds.origin.y + fBounds.size.height) ) );
		}
		else if ( (r.size.height == fBounds.size.height) && (r.origin.y == fBounds.origin.y) )
		{
			ok = ( ( (r.origin.x >= fBounds.origin.x) && (r.origin.x <= fBounds.origin.x + fBounds.size.width) )
				|| ( (r.origin.x + r.size.width >= fBounds.origin.x) && (r.origin.x + r.size.width <= fBounds.origin.x + fBounds.size.width) ) );
		}
		else
		{
			ok = false;
		}
		
		if (ok)
			fBounds = CGRectUnion( fBounds, r);

		return ok;
	}

	virtual HIShapeRef	CreateShape()
	{
		return ::HIShapeCreateWithRect( &fBounds);
	}
	
	CGRect	fBounds;
};


class VRegionNode_op : public VRegionNode
{
public:
	VRegionNode_op( VRegionNode *inNode1, VRegionNode *inNode2, VRegionOpCode inOpCode)
		:fNode1( inNode1), fNode2( inNode2), fOpCode( inOpCode)
			{
			}

	~VRegionNode_op()
		{
			delete fNode1;
			delete fNode2;
		}

	virtual	void SetPosBy( GReal inHoriz, GReal inVert)
	{
		fNode1->SetPosBy( inHoriz, inVert);
		fNode2->SetPosBy( inHoriz, inVert);
	}

	virtual void SetSizeBy( GReal inHoriz, GReal inVert)
	{
		fNode1->SetSizeBy( inHoriz, inVert);
		fNode2->SetSizeBy( inHoriz, inVert);
	}

	virtual VRegionNode_op* Clone()
	{
		return new VRegionNode_op( fNode1->Clone(), fNode2->Clone(), fOpCode);
	}

	virtual	bool IsRect( VRect& outRect)			{ return false;}

	virtual	bool UnionRect( const VRect& inRect)
	{
		if (fOpCode == UNION)
			return fNode1->UnionRect( inRect) || fNode2->UnionRect( inRect);
		else
			return false;
	}

	virtual HIShapeRef	CreateShape()
	{
		HIShapeRef shape;
		HIShapeRef shape1 = fNode1->CreateShape();
		HIShapeRef shape2 = fNode2->CreateShape();
		switch( fOpCode)
		{
			case INTERSECT:	shape = ::HIShapeCreateIntersection( shape1, shape2); break;
			case UNION:		shape = ::HIShapeCreateUnion( shape1, shape2); break;
			case SUBSTRACT:	shape = ::HIShapeCreateDifference( shape1, shape2); break;
		}
		::CFRelease( shape1);
		::CFRelease( shape2);
		
		return shape;
	}
	
private:
	VRegionNode*	fNode1;
	VRegionNode*	fNode2;
	VRegionOpCode	fOpCode;
};

//====================================================================================


VRegion::VRegion(GReal inX, GReal inY, GReal inWidth, GReal inHeight)
{
	_Init();
	
	CGRect	macRect = { inX, inY, inWidth, inHeight };

	fNode = new VRegionNode_rect( macRect);
	
	fBounds.FromRectRef( macRect);
}


VRegion::~VRegion()
{
	_Release();
}


void VRegion::_Init()
{
	fShape = NULL;
	fNode = NULL;
}


void VRegion::_Release()
{
	_PurgeShape();
	delete fNode;
	fNode = NULL;
}


RgnRef VRegion::MAC_GetShape() const
{
	if (fShape == NULL)
		fShape = fNode->CreateShape();
	return fShape;
}


void VRegion::SetPosBy(GReal inHoriz, GReal inVert)
{
	fBounds.SetPosBy(inHoriz, inVert);
	
	fNode->SetPosBy(inHoriz, inVert);
}


void VRegion::SetSizeBy(GReal inHoriz, GReal inVert)
{
	fBounds.SetSizeBy(inHoriz, inVert);

	fNode->SetSizeBy(inHoriz, inVert);
}


Boolean VRegion::HitTest(const VPoint& inPoint) const
{
	CGPoint	point = inPoint;
	return ::HIShapeContainsPoint( MAC_GetShape(), &point);
}


void VRegion::Inset(GReal inHoriz, GReal inVert)
{
	SetPosBy(inHoriz, inVert);
	SetSizeBy((GReal) -2.0 * inHoriz, (GReal) -2.0 * inVert);

	fNode->SetPosBy(inHoriz, inVert);
	fNode->SetSizeBy((GReal) -2.0 * inHoriz, (GReal) -2.0 * inVert);
}

	
void VRegion::SetEmpty()
{
	_Release();

	fNode = new VRegionNode;
	
	fBounds.SetEmpty();
}


Boolean VRegion::IsEmpty() const
{
	return ::HIShapeIsEmpty(MAC_GetShape());
}


void VRegion::FromRect (const VRect& inRect)
{
	_Release();
	
	CGRect r = inRect;
	fNode = new VRegionNode_rect( r);

	fBounds = inRect;
}


void VRegion::FromRegion(const VRegion& inOriginal)
{
	_Release();
	
	fNode = inOriginal.fNode->Clone();

	fBounds = inOriginal.fBounds;
}


void VRegion::FromPolygon(const VPolygon& inPolygon)
{
	_Release();
	
	// Not suported - use bounds instead
	DebugMsg("Unsupported VRegion::FromPolygon call");

	CGRect r = inPolygon.GetBounds();

	fNode = new VRegionNode_rect( r);
	
	_ComputeBounds();
}


void VRegion::FromRegionRef(RgnRef inRgn, Boolean inOwnIt)
{
	_Release();
	
	if (inRgn == NULL)
	{
		fNode = new VRegionNode;
		fBounds.SetEmpty();
	}
	else
	{
		// brutal mais on n'a pas le choix
		CGRect r;
		::HIShapeGetBounds( inRgn, &r);
		fNode = new VRegionNode_rect( r);
		fBounds.FromRectRef( r);
		if (inOwnIt)
		{
			fShape = inRgn;
		}
	}
}


Boolean VRegion::operator == (const VRegion& inRgn) const
{	
	HIShapeRef	tempRegion = ::HIShapeCreateDifference( MAC_GetShape(), inRgn.MAC_GetShape());
	Boolean	isEqual = ::HIShapeIsEmpty(tempRegion);
	::CFRelease(tempRegion);
	
	return isEqual;
}


void VRegion::Xor(const VRegion& inRgn)
{
	_PurgeShape();

	VRegionNode_op *op_union = new VRegionNode_op( fNode, inRgn.fNode->Clone(), VRegionNode::UNION);	// keeps fNode
	VRegionNode_op *op_intersect = new VRegionNode_op( fNode->Clone(), inRgn.fNode->Clone(), VRegionNode::INTERSECT);

	fNode = new VRegionNode_op( op_union, op_intersect, VRegionNode::SUBSTRACT);

	_ComputeBounds();
}


void VRegion::Substract(const VRegion& inRgn)
{
	_PurgeShape();

	fNode = new VRegionNode_op( fNode, inRgn.fNode->Clone(), VRegionNode::SUBSTRACT);
	
	_ComputeBounds();
}


void VRegion::Intersect(const VRegion& inRgn)
{
	_PurgeShape();

	fNode = new VRegionNode_op( fNode, inRgn.fNode->Clone(), VRegionNode::INTERSECT);
	
	_ComputeBounds();
}


void VRegion::Union(const VRegion& inRgn)
{
	_PurgeShape();

	// simple and frequent case: union with connected rectangles produces a rectangle
	VRect r;
	if (!inRgn.fNode->IsRect( r) || !fNode->UnionRect( r))
		fNode = new VRegionNode_op( fNode, inRgn.fNode->Clone(), VRegionNode::UNION);
	
	fBounds.Union( inRgn.fBounds);
}


void VRegion::_ComputeBounds()
{
	CGRect	rgnBox = { 0, 0, 0, 0 };
	::HIShapeGetBounds(MAC_GetShape(), &rgnBox);
		
	fBounds.FromRectRef(rgnBox);
}

END_TOOLBOX_NAMESPACE


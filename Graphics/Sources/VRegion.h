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
#ifndef __VRegion__
#define __VRegion__

#include "Graphics/Sources/VRect.h"

BEGIN_TOOLBOX_NAMESPACE

// Needed declarations
class VPolygon;
class VGraphicPath;

#if VERSIONMAC
class VRegionNode;
#endif

class XTOOLBOX_API VRegion : public VObject
{
public:
				VRegion (const VRegion* inRgn = NULL);
	explicit	VRegion (RgnRef	inRgn, bool inOwnIt);
	explicit	VRegion (const VRegion& inOriginal);
	explicit	VRegion (const VRect& inRect);
	explicit	VRegion (const VPolygon& inPolygon);
	explicit	VRegion (GReal inX, GReal inY, GReal inWidth, GReal inHeight);
	virtual		~VRegion ();
	
	void	SetPosBy (GReal inHoriz, GReal inVert);
	void	SetSizeBy (GReal inHoriz, GReal inVert);

	void	SetPosTo (const VPoint& inPos)				{ SetPosBy(inPos.GetX() - fBounds.GetX(), inPos.GetY() - fBounds.GetY()); }
	void	SetSizeTo (GReal inWidth, GReal inHeight)	{ xbox_assert(inWidth > -2); xbox_assert(inHeight > -2); SetSizeBy((inWidth != -1) ? inWidth - fBounds.GetWidth() : 0, (inHeight != -1) ? inHeight - fBounds.GetHeight() : 0); }
	
	void	ScaleSizeBy (GReal inHorizScale, GReal inVertScale)	{ SetSizeBy(fBounds.GetWidth() * (inHorizScale - 1), fBounds.GetHeight() * (inVertScale - 1));}
	void	ScalePosBy (GReal inHorizScale, GReal inVertScale)	{ SetPosBy(fBounds.GetX() * (inHorizScale - 1), fBounds.GetY() * (inVertScale - 1));}
	
	Boolean	HitTest (const VPoint& inPoint) const;
	void	Inset (GReal inHoriz, GReal inVert);

	const VRect&	GetBounds() const { return fBounds; }
	
	void	SetEmpty ();
	Boolean	IsEmpty () const;
	
	// Copy operators
	void	FromRegion (const VRegion& inOriginal);
	void	FromRect (const VRect& inRect);
	void	FromPolygon (const VPolygon& inPolygon);
	void	FromRegionRef (RgnRef inRgn, Boolean inOwnIt = true);
	
	// Logical operators
	VRegion&	operator = (const VRegion& inRgn) { FromRegion(inRgn); return *this; };
	VRegion&	operator += (const VRegion& inRgn) { Union(inRgn); return *this; };
	VRegion&	operator -= (const VRegion& inRgn) { Substract(inRgn); return *this; };
	VRegion&	operator &= (const VRegion& inRgn) { Intersect(inRgn); return *this; };
	Boolean		operator != (const VRegion& inRgn) const { return !(*this == inRgn); };
	Boolean		operator == (const VRegion& inRgn) const;
	
	// Native operators

#if VERSIONWIN
	/** return true if region is more than a single rectangle */
	bool IsComplex() const;
#endif

#if ENABLE_D2D
	/** get the complex path made from the internal complex region 
	@remarks
		only for D2D impl because we can only clip with geometry paths
		& not directly with GDI region

		return NULL if region is a simple region (ie equal to region bounds)
	*/
	VGraphicPath *GetComplexPath() const;
#endif

#if VERSIONMAC
	operator const RgnRef () const	{ return MAC_GetShape();}
#else
	operator const RgnRef () const;
#endif

#if USE_GDIPLUS
	operator const Gdiplus::Region& () const;
	operator const Gdiplus::Region* () const;
#endif

#if VERSIONMAC
		
	// guaranteed valid until next operation on this VRegion
	RgnRef		MAC_GetShape() const;
#endif

	// Utilities
	void	Intersect (const VRegion& inRgn);
	void	Union (const VRegion& inRgn);
	void	Substract (const VRegion& inRgn);
	
	void	Xor (const VRegion& inRgn);		// Much slower than the 3 basic operations
	Boolean	Overlap (const VRegion& inRgn);	// Much slower than the 3 basic operations
	
protected:
	VRect	fBounds;			// Cached bounds for the shape
#if VERSIONWIN
#if USE_GDIPLUS
	Gdiplus::Region*	fRegion;
#else
	RgnRef	fRegion;
#endif
	VPoint	fOffset;
	mutable RgnRef	fRegionBuffer;
#endif
	

#if VERSIONMAC
	mutable	RgnRef			fShape;
			VRegionNode*	fNode;

			void			_PurgeShape() const		{ if (fShape != NULL) ::CFRelease(fShape); fShape = NULL;}
#endif
	
			void			_Init ();
			void			_Release ();
			void			_ComputeBounds ();
	
#if VERSIONWIN
			void			_AdjustOrigin ();
#endif
};

// JM 271206
class XTOOLBOX_API VShape : public VObject, public IStreamable
{
 public:
			VShape();
			VShape (const VShape& inOriginal);
	virtual	~VShape();

	// Operators
	virtual void SetTriangleTo(const VPoint& inPointA, const VPoint& inPointB, const VPoint& inPointC);
	virtual void SetCircleTo (const VPoint& inCenter, const GReal inSize);
	
	// Accessors

	// Utilities
	virtual Boolean	HitTest (const VPoint& inPoint) const = 0;
	
	// Copy operators
	
	// Native operators
	
	// Inherited from VObject

protected:
};

typedef struct LINE_tag
{
	VPoint a;
	VPoint b;
} LINE_t;

typedef struct TRIANGLE_tag
{
	VPoint a;
	VPoint b;
	VPoint c;
} TRIANGLE_t;

class XTOOLBOX_API VTriangle : public VShape
{
 public:
			VTriangle ();
			VTriangle (const VPoint& inPointA, const VPoint& inPointB, const VPoint& inPointC);
			VTriangle (const VTriangle& inOriginal);
	virtual	~VTriangle();

	// Operators
	
	// Accessors
	void SetTriangleTo (const VPoint& inPointA, const VPoint& inPointB, const VPoint& inPointC);

	// Utilities
	virtual Boolean	HitTest (const VPoint& inPoint) const;
	
	// Copy operators
	
	// Native operators
	
	// Inherited from VObject
	virtual VError	ReadFromStream (VStream* ioStream, sLONG inParam = 0);
	virtual VError	WriteToStream (VStream* ioStream, sLONG inParam = 0) const;

protected:
	VPoint	fA;
	VPoint	fB;
	VPoint	fC;

private:
	// Utilities
	int Side(const VPoint &p, LINE_t *e ) const;
};

class XTOOLBOX_API VCircle : public VShape
{
 public:
			VCircle ();
			VCircle (const VPoint& inCenter, const GReal inSize);
			VCircle (const VCircle& inOriginal);
	virtual	~VCircle();

	// Operators
	
	// Accessors
	void SetCircleTo (const VPoint& inCenter, const GReal inSize);

	// Utilities
	virtual Boolean	HitTest (const VPoint& inPoint) const;
	
	// Copy operators
	
	// Native operators
	
	// Inherited from VObject
	virtual VError	ReadFromStream (VStream* ioStream, sLONG inParam = 0);
	virtual VError	WriteToStream (VStream* ioStream, sLONG inParam = 0) const;

protected:
	VPoint	fO;
	GReal	fR;

private:
	GReal	fR2; // Le rayon du cercle au carré
};

END_TOOLBOX_NAMESPACE

#endif
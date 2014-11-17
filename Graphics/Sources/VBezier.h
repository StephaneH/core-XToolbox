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
#ifndef __VBezier__
#define __VBezier__

#include "Graphics/Sources/IBoundedShape.h"
#include "Graphics/Sources/VPolygon.h"	// Needed for Windows
#include "Graphics/Sources/VAffineTransform.h"	

BEGIN_TOOLBOX_NAMESPACE

const GReal	GPI = (GReal) 3.1415926535897932384626433832795;

/** number of points calculated on bezier curve to estimate bounding rectangle 
	using fast de Casteljau's algorithm */
#define GP_BEZIER_DECASTELJAU_SAMPLING_COUNT	20

typedef enum eGraphicPathDrawCmdType
{
	GPDCT_BEGIN_SUBPATH_AT,
	GPDCT_ADD_LINE_TO,
	GPDCT_ADD_BEZIER_TO,
	GPDCT_CLOSE_SUBPATH,
	GPDCT_ADD_OVAL,
	GPDCT_SET_POS_BY,
	GPDCT_SET_SIZE_BY

} eGraphicPathDrawCmdType;

/* graphic path drawing command 
@remarks
   used only by crispEdges rendering mode 
   (in order to rebuild path on the fly with adjusted coordinates)
**/
class VGraphicPathDrawCmd 
{
protected:
	eGraphicPathDrawCmdType fType;
	
	VPoint fPoints[3];
	VRect fRect;
public:
	VGraphicPathDrawCmd( eGraphicPathDrawCmdType inType)
	{
		xbox_assert(inType == GPDCT_CLOSE_SUBPATH);
		fType = inType;
	}
	VGraphicPathDrawCmd( eGraphicPathDrawCmdType inType, const VPoint& inPos)
	{
		fType = inType;
		switch (inType)
		{
		case GPDCT_BEGIN_SUBPATH_AT:
		case GPDCT_ADD_LINE_TO:
			fPoints[0] = inPos;
			break;
		default:
			xbox_assert(false);
			break;
		}
	}
	VGraphicPathDrawCmd( eGraphicPathDrawCmdType inType, const VPoint& inStartControl, const VPoint& inDestPoint, const VPoint& inDestControl)
	{
		xbox_assert(inType == GPDCT_ADD_BEZIER_TO);
		fType = inType;

		fPoints[0] = inStartControl;
		fPoints[1] = inDestPoint;
		fPoints[2] = inDestControl;
	}

	VGraphicPathDrawCmd( eGraphicPathDrawCmdType inType, const VRect& inRect)
	{
		xbox_assert(inType == GPDCT_ADD_OVAL);
		fType = inType;

		fRect = inRect;
	}

	VGraphicPathDrawCmd( eGraphicPathDrawCmdType inType, const GReal inX, const GReal inY)
	{
		xbox_assert(inType == GPDCT_SET_POS_BY || inType == GPDCT_SET_SIZE_BY);
		fType = inType;

		fPoints[0].SetPosTo( inX, inY);
	}

	VGraphicPathDrawCmd (const VGraphicPathDrawCmd& inOriginal)
	{
		fType = inOriginal.fType;
		fPoints[0] = inOriginal.fPoints[0];
		fPoints[1] = inOriginal.fPoints[1];
		fPoints[2] = inOriginal.fPoints[2];
		fRect = inOriginal.fRect;
	}

	VGraphicPathDrawCmd&	operator = (const VGraphicPathDrawCmd& inOriginal)
	{
		if (&inOriginal == this)
			return *this;

		fType = inOriginal.fType;
		fPoints[0] = inOriginal.fPoints[0];
		fPoints[1] = inOriginal.fPoints[1];
		fPoints[2] = inOriginal.fPoints[2];
		fRect = inOriginal.fRect;

		return *this;
	}

	eGraphicPathDrawCmdType GetType() const
	{
		return fType;
	}

	const VRect& GetRect() const
	{
		return fRect;
	}

	const VPoint *GetPoints() const 
	{
		return &(fPoints[0]);
	}
};

typedef std::vector<VGraphicPathDrawCmd> VGraphicPathDrawCmds;

#ifndef VGraphicContext
class VGraphicContext;
#endif


/* graphic path */
class XTOOLBOX_API VGraphicPath : public VObject, public IBoundedShape
{
public:
				/** default constructor
				@param inComputeBoundsAccurate
					true: slower but compute accurate path drawing bounds using de Casteljau's algorithm to estimate bezier curves real bounds
					false: use API built-in method : faster but depending on API, bounds can be somewhat larger than the shape
						   (GdiPlus or Quartz2D approximate bezier curve with the polygon made of the 4 control points
						    to determine bezier curve bounding rectangle so actually this is not the real shape bounding rectangle)	
				@param inCreateStorageForCrispEdges
					true: create internal storage which will be used for crispEdges rendering mode (see also VGraphicContext::fAllowCrispEdgesOnPath)
						  (crispEdges can only be applied on paths created with inCreateStorageForCrispEdges = true)
				    false: disable crispEdges for this path (default)
				@remarks
					set inComputeBoundsAccurate to true only if you use and need accurate bounds
					(SVG component needs real shape bounds for instance in order 
					 to draw gradients, patterns or filters which are relative to shape bounding rect)
				@note
					default is false for backward compatibility	
				*/
				VGraphicPath (bool inComputeBoundsAccurate = false, bool inCreateStorageForCrispEdges = false);

				VGraphicPath( const VPolygon& inPolygon);

	explicit	VGraphicPath (const VGraphicPath& inOriginal);

	virtual		~VGraphicPath ();
	
	VGraphicPath *Clone() const;

	/** return cloned graphic path with coordinates modified with crispEdges algorithm
	@remarks
		depending on fCreateStorageForCrispEdges status & on VGraphicContext crispEdges status

		return NULL if crispEdges is disabled 
	*/
	VGraphicPath *CloneWithCrispEdges( VGraphicContext *inGC, bool inFillOnly = false) const;

	/** intersect current path with the specified path
	@remarks
		does not update internal polygon
		used only by D2D impl for path clipping implementation
	*/
	void Intersect( const VGraphicPath& inPath)
	{
#if ENABLE_D2D
		_Combine( inPath, true);
#endif
	}

	/** union current path with the specified path
	@remarks
		does not update internal polygon
		used only by D2D impl for path clipping implementation
	*/
	void Union( const VGraphicPath& inPath)
	{
#if ENABLE_D2D
		_Combine( inPath, false);
#endif
	}

#if ENABLE_D2D
	void	Widen(const GReal inStrokeWidth, ID2D1StrokeStyle* inStrokeStyle);
	void	Outline();
#endif

	// VBezier construction
#if VERSIONMAC
	void	Begin() {}
	void	End() {}
#else
	void	Begin();
	void	End();
#endif
	bool	IsBuilded() const
	{
#if ENABLE_D2D
		return fGeomSink == NULL;
#else
		return true;
#endif
	}
	void	BeginSubPathAt (const VPoint& inLocalPoint);
	void	AddLineTo (const VPoint& inDestPoint);
	void	AddBezierTo(const VPoint& inStartControl, const VPoint& inDestPoint, const VPoint& inDestControl);
	void	CloseSubPath ();
	void	Clear();
	
	void			AddRect( const VRect& inBounds)
	{
		BeginSubPathAt( inBounds.GetTopLeft());
		AddLineTo( inBounds.GetTopRight());
		AddLineTo( inBounds.GetBotRight());
		AddLineTo( inBounds.GetBotLeft());
		CloseSubPath();
	}

	void	AddOval (const VRect& inBounds);	// Similar to creating a closed subpath with 4 curves
	
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
	void	AddArcTo(const VPoint& inCenter, const VPoint& inDestPoint, GReal inRX, GReal inRY = 0.0f, bool inLargeArcFlag = false, GReal inXAxisRotation = 0.0f);

	/** measure a arc 
	@param inCenter
		arc ellipse center
	@param inStartDeg
		starting angle in degree
	@param inEndDeg
		ending angle in degree
	@param outStartPoint
		computed start point
	@param outEndPoint
		computed end point
	@param inRX
		arc ellipse radius along x axis
	@param inRY
		arc ellipse radius along y axis
		if inRY <= 0, inRY = inRX
	@remarks
		use this method to get start & end point from start & end angle in degree
		which you can pass then to AddArc
	*/
	static void MeasureArc (const VPoint& inCenter, GReal inStartDeg, GReal inEndDeg, VPoint& outStartPoint, VPoint& outEndPoint, GReal inRX, GReal inRY = 0.0f)
	{
		if (inRY <= 0.0f)
			inRY = inRX;
		if (inRX <= 0.0f || inRY <= 0.0f)
		{
			outStartPoint = inCenter;
			outEndPoint = inCenter;
			return;
		}
		GReal x0 = std::cos( inStartDeg*GPI/180);
		GReal y0 = -std::sin( inStartDeg*GPI/180);

		GReal x1 = std::cos( inEndDeg*GPI/180);
		GReal y1 = -std::sin( inEndDeg*GPI/180);

		x0 = inCenter.GetX() + x0*inRX;
		y0 = inCenter.GetY() + y0*inRY;

		x1 = inCenter.GetX() + x1*inRX;
		y1 = inCenter.GetY() + y1*inRY;

		outStartPoint.SetPos( x0, y0);
		outEndPoint.SetPos( x1, y1);
	}

	// IBoundedShape API
	void	SetPosBy (GReal inHoriz, GReal inVert);
	void	SetSizeBy (GReal inWidth, GReal inHeight);
	
	Boolean	HitTest (const VPoint& inPoint) const;
	Boolean ScaledHitTest(const VPoint& inPoint, GReal scale) const; // JM 201206
	void	Inset (GReal inHoriz, GReal inVert);
	
	// Operators
	VGraphicPath&	operator = (const VGraphicPath& inOriginal);
	Boolean		operator == (const VGraphicPath& inBezier) const;

#if VERSIONMAC
	operator const CGPathRef () const { return fPath; }
	operator const CGAffineTransform& () const { return fTransform; }
	
	void SetFillMode( FillRuleType inFillMode) { fFillMode = inFillMode; }
	FillRuleType GetFillMode() const { return fFillMode; }
#endif

#if VERSIONWIN
	operator const Gdiplus::GraphicsPath* () const 
	{ 
		xbox_assert(fPath); 
		return (Gdiplus::GraphicsPath *)fPath; 
	}
	operator const Gdiplus::GraphicsPath& () const 
	{  
		xbox_assert(fPath); 
		return *((Gdiplus::GraphicsPath *)fPath); 
	}

	void SetFillMode( FillRuleType inFillMode);
	FillRuleType GetFillMode() const;

#if ENABLE_D2D
	ID2D1Geometry* GetImplPathD2D() const 
	{ 
		return fPathD2D;
	}
#endif
#endif

	operator const VPolygon& () const { return fPolygon; }
	const VPolygon& GetPolygon () const { return fPolygon; }

protected:
	FillRuleType			fFillMode;

#if VERSIONMAC
	CGAffineTransform	fTransform;
	CGMutablePathRef 	fPath;
#else
	Gdiplus::GraphicsPath  *fPath;
	VPoint					fLastPoint;

#if ENABLE_D2D
	ID2D1GeometrySink*		fGeomSink;
	ID2D1Geometry*			fPathD2D;
	
	bool					fFigureIsClosed;

	void					_Combine( const VGraphicPath& inPath, bool inIntersect = true);
#endif
#endif

	VPolygon	fPolygon;

	/**	true: slower but compute accurate path drawing bounds using de Casteljau's algorithm to estimate bezier curves real bounds
		false: use API built-in method : faster but depending on API, bounds can be somewhat larger than the shape
			   (GdiPlus or Quartz2D approximate bezier curve with the polygon made of the 4 control points
			    to determine bezier curve bounding rectangle so actually this is not the real shape bounding rectangle)
	*/
	bool	fComputeBoundsAccurate;

	/** path last point */
	VPoint	fPathPrevPoint;	

	/** path minimum bounds */
	VPoint	fPathMin;

	/** path maximum bounds */
	VPoint	fPathMax;

	/** path drawing commands 
	@remarks
	   used only by crispEdges rendering mode 
	   (in order to rebuild path on the fly with adjusted coordinates)
	*/
	VGraphicPathDrawCmds fDrawCmds;

	bool fCreateStorageForCrispEdges;

	/** current path transform 
	@remarks
		maintained only if fCreateStorageForCrispEdges is equal to true
		(we need it to adjust coords in transformed space)
	*/
	VAffineTransform fCurTransform;

	/** true if path contains bezier curves */
	bool fHasBezier;

#if VERSIONWIN
	void _Init( bool inComputeBoundsAccurate = false, bool inCreateStorageForCrispEdges = false);
#endif

	/** add point to path bounds 
	@remarks
		used only if fComputeBoundsAccurate is set to true
	*/
	void _addPointToBounds( const VPoint& p0)
	{
		if (p0.x < fPathMin.x)
			fPathMin.x = p0.x;
		if (p0.x > fPathMax.x)
			fPathMax.x = p0.x;
		if (p0.y < fPathMin.y)
			fPathMin.y = p0.y;
		if (p0.y > fPathMax.y)
			fPathMax.y = p0.y;

		fPathPrevPoint = p0;
	}


	/** add rect to path bounds 
	@remarks
		used only if fComputeBoundsAccurate is set to true
	*/
	void _addRectToBounds( const VRect& inBounds)
	{
		_addPointToBounds( VPoint((SmallReal)inBounds.GetLeft(), (SmallReal)inBounds.GetTop()));
		_addPointToBounds( VPoint((SmallReal)inBounds.GetRight(), (SmallReal)inBounds.GetBottom()));
	}


	/** add bezier curve bounds using de Casteljau's algorithm 
	@param p1
		first control point
	@param p2
		second control point
	@param p3
		end point
	@remarks
		compute GP_BEZIER_DECASTELJAU_SAMPLING_COUNT points on curve
		to estimate real bezier curve bounding rectangle
		and update fPathMin and fPathMax
	@note
		use fPathPrevPoint as start point
	*/
	void  _addBezierToBounds( const VPoint& p1, const VPoint& p2, const VPoint &p3)
	{
		const VPoint& p0 = fPathPrevPoint;

		//iterate linearly from 0 to 1
		SmallReal step = 1.0f/GP_BEZIER_DECASTELJAU_SAMPLING_COUNT;
		for (SmallReal t = step; t < 1.0f; t += step)
		{
			//de Casteljau's algorithm first subdivision
			VPoint c0 = p0*(1.0f-t)+p1*t;
			VPoint c1 = p1*(1.0f-t)+p2*t;
			VPoint c2 = p2*(1.0f-t)+p3*t;
			
			//de Casteljau's algorithm second subdivision
			VPoint d0 = c0*(1.0f-t)+c1*t;
			VPoint d1 = c1*(1.0f-t)+c2*t;
			
			//de Casteljau's algorithm third subdivision
			VPoint e0 = d0*(1.0f-t)+d1*t;

			if (e0.x < fPathMin.x)
				fPathMin.x = e0.x;
			if (e0.x > fPathMax.x)
				fPathMax.x = e0.x;
			if (e0.y < fPathMin.y)
				fPathMin.y = e0.y;
			if (e0.y > fPathMax.y)
				fPathMax.y = e0.y;
		}
		_addPointToBounds( p3);
	}

	/** add a arc segment (<= 90°) */
	void _PathAddArcSegment(	GReal xc, GReal yc,
								GReal th0, GReal th1,
								GReal rx, GReal ry, 
								GReal xAxisRotation)
	{
		GReal sinTh, cosTh;
		GReal a00, a01, a10, a11;
		GReal x1, y1, x2, y2, x3, y3;
		GReal t;
		GReal thHalf;

		sinTh = std::sin(xAxisRotation * (GPI / 180));
		cosTh = std::cos(xAxisRotation * (GPI / 180));

		a00 =  cosTh * rx;
		a01 = -sinTh * ry;
		a10 =  sinTh * rx;
		a11 =  cosTh * ry;

		thHalf = (GReal) 0.5 * (th1 - th0);
		t = ((GReal) 8 / (GReal) 3) * std::sin(thHalf * (GReal) 0.5) * std::sin(thHalf * (GReal) 0.5) / std::sin(thHalf);
		x1 = xc + std::cos(th0) - t * std::sin(th0);
		y1 = yc + std::sin(th0) + t * std::cos(th0);
		x3 = xc + std::cos(th1);
		y3 = yc + std::sin(th1);
		x2 = x3 + t * std::sin(th1);
		y2 = y3 - t * std::cos(th1);

		AddBezierTo(	VPoint(a00 * x1 + a01 * y1, a10 * x1 + a11 * y1),
						VPoint(a00 * x3 + a01 * y3, a10 * x3 + a11 * y3),
						VPoint(a00 * x2 + a01 * y2, a10 * x2 + a11 * y2));
	}

	virtual void	_ComputeBounds ();
};


#define VBezier VGraphicPath

END_TOOLBOX_NAMESPACE

#endif

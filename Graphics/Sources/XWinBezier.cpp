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

#if ENABLE_D2D
#include "V4DPictureIncludeBase.h"
#include "XWinD2DGraphicContext.h"
#endif

#define GP_BOUNDS_MAX_VALUE_DEFAULT	1000000.0f
#define GP_BOUNDS_MIN_DEFAULT VPoint( GP_BOUNDS_MAX_VALUE_DEFAULT,  GP_BOUNDS_MAX_VALUE_DEFAULT)
#define GP_BOUNDS_MAX_DEFAULT VPoint(-GP_BOUNDS_MAX_VALUE_DEFAULT, -GP_BOUNDS_MAX_VALUE_DEFAULT)


VGraphicPath::VGraphicPath(bool inComputeBoundsAccurate, bool inCreateStorageForCrispEdges)
{
	_Init( inComputeBoundsAccurate, inCreateStorageForCrispEdges);
}

void VGraphicPath::_Init( bool inComputeBoundsAccurate, bool inCreateStorageForCrispEdges)
{
	fCreateStorageForCrispEdges = inCreateStorageForCrispEdges;
	if (fCreateStorageForCrispEdges)
		fDrawCmds.reserve(16); //slightly optimize allocation
	fHasBezier = false;

	fPath = new Gdiplus::GraphicsPath( Gdiplus::FillModeAlternate);
	
	fLastPoint.x = 0.0f;
	fLastPoint.y = 0.0f;

	fComputeBoundsAccurate = inComputeBoundsAccurate;
	if (fComputeBoundsAccurate)
	{
		fPathPrevPoint	= GP_BOUNDS_MIN_DEFAULT;
		fPathMin		= GP_BOUNDS_MIN_DEFAULT;
		fPathMax		= GP_BOUNDS_MAX_DEFAULT;
	}

	fFillMode = FILLRULE_EVENODD;

#if ENABLE_D2D

	fPathD2D = NULL;
	if (VWinD2DGraphicContext::IsAvailable())
	{
#if !D2D_FACTORY_MULTI_THREADED
		VWinD2DGraphicContext::_CreateSingleThreadedFactory();
#endif		
		VWinD2DGraphicContext::GetMutexFactory().Lock();
		HRESULT hr = VWinD2DGraphicContext::GetFactory()->CreatePathGeometry( (ID2D1PathGeometry **)&fPathD2D);
		xbox_assert(SUCCEEDED(hr));
		VWinD2DGraphicContext::GetMutexFactory().Unlock();
	}
	fGeomSink = NULL;
	fFigureIsClosed = true;
#endif
}


VGraphicPath::VGraphicPath( const VPolygon& inPolygon)
{
	_Init( false, true);
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
#if ENABLE_D2D
#if !D2D_FACTORY_MULTI_THREADED
		VWinD2DGraphicContext::_CreateSingleThreadedFactory();
#endif		
#endif

	fPath = NULL;
#if ENABLE_D2D
	fGeomSink = NULL;
	fPathD2D = NULL;
#endif

	*this = inOriginal;
}

VGraphicPath& VGraphicPath::operator = (const VGraphicPath& inOriginal)
{	
	End();

	fBounds = inOriginal.fBounds;
	fPolygon = inOriginal.fPolygon;

	fPath = inOriginal.fPath->Clone();

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

	fFillMode = inOriginal.GetFillMode();

#if ENABLE_D2D
	if (fGeomSink != NULL)
	{
		fGeomSink->Release();
		fGeomSink = NULL;
	}
	if (fPathD2D != NULL)
	{
		fPathD2D->Release();
		fPathD2D = NULL;
	}

	fFigureIsClosed = true;

	if (VWinD2DGraphicContext::IsAvailable() && inOriginal.IsBuilded())
	{
		//copy from source path
		//(just retain source path because D2D paths are immutable)
		ID2D1Geometry *pathSource = inOriginal.GetImplPathD2D();
		xbox_assert(pathSource);
#if !D2D_FACTORY_MULTI_THREADED
#if VERSIONDEBUG
		{
		VWinD2DGraphicContext::GetMutexFactory().Lock();
		CComPtr<ID2D1Factory> factorySource;
		pathSource->GetFactory(&factorySource);
		xbox_assert(factorySource == VWinD2DGraphicContext::GetFactory());
		VWinD2DGraphicContext::GetMutexFactory().Unlock();
		}
#endif
#endif
		pathSource->AddRef();
		fPathD2D = pathSource;
	}
#endif
	return *this;
}


Boolean VGraphicPath::operator == (const VGraphicPath& inBezier) const
{	
	return (fPath == inBezier.fPath 
#if ENABLE_D2D
			&&
			fPathD2D == inBezier.fPathD2D
#endif
			&& 
			fFillMode == inBezier.fFillMode
			&&
			fComputeBoundsAccurate == inBezier.fComputeBoundsAccurate
			&&
			fCreateStorageForCrispEdges == inBezier.fCreateStorageForCrispEdges);
}


VGraphicPath::~VGraphicPath()
{
	if (fPath != NULL)
		delete fPath;
#if ENABLE_D2D
	if (fGeomSink != NULL)
	{
		fGeomSink->Release();
		fGeomSink = NULL;
	}
	if (fPathD2D != NULL)
	{
		fPathD2D->Release();
		fPathD2D = NULL;
	}
#endif
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
			inGC->CEAdjustPointInTransformedSpace( pt, inFillOnly);
			path->BeginSubPathAt( pt);
			}
			break;
		case GPDCT_ADD_LINE_TO:
			{
			VPoint pt(*(it->GetPoints()));
			inGC->CEAdjustPointInTransformedSpace( pt, inFillOnly);
			path->AddLineTo( pt);
			}
			break;
		case GPDCT_ADD_BEZIER_TO:
			{
			VPoint c1(*(it->GetPoints()));
			VPoint d(*(it->GetPoints()+1));
			VPoint c2(*(it->GetPoints()+2));

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
		inGC->SetTransform( ctm);
	path->End();
	return path;
}


#if ENABLE_D2D

/** combine current path with the specified path
@remarks
	does not update internal polygon
	used only by D2D impl for path clipping implementation
*/
void VGraphicPath::_Combine( const VGraphicPath& inPath, bool inIntersect)
{
	End();

	//ensure proper combination if current path or input path is empty
	if (inPath.GetBounds().GetWidth() == 0.0f || inPath.GetBounds().GetHeight() == 0.0f)
	{
		//input path is empty
		if (inIntersect)
			Clear();
		return;
	}
	if (GetBounds().GetWidth() == 0.0f || GetBounds().GetHeight() == 0.0f)
	{
		//current path is empty
		if (inIntersect)
			return;
		else
			*this = inPath;
		return;
	}

	//combine D2D path if appropriate
	//TODO: make it work for Gdiplus::GraphicPath too ?
	if (fPathD2D != NULL && inPath.GetImplPathD2D())
	{
		ID2D1Geometry *oldPath = fPathD2D;

		//create new path
		ID2D1PathGeometry *newPath = NULL;
		VWinD2DGraphicContext::GetMutexFactory().Lock();
		HRESULT hr = VWinD2DGraphicContext::GetFactory()->CreatePathGeometry( &newPath);
		xbox_assert(SUCCEEDED(hr));
		VWinD2DGraphicContext::GetMutexFactory().Unlock();

		//combine current path with input path
		CComPtr<ID2D1GeometrySink> thisGeomSink;
		if (SUCCEEDED(newPath->Open(&thisGeomSink)))
		{
			thisGeomSink->SetFillMode( fFillMode == FILLRULE_EVENODD ? D2D1_FILL_MODE_ALTERNATE : D2D1_FILL_MODE_WINDING);

			if (!SUCCEEDED(oldPath->CombineWithGeometry(
							inPath.GetImplPathD2D(),
							inIntersect ? D2D1_COMBINE_MODE_INTERSECT : D2D1_COMBINE_MODE_UNION,
							NULL,
							0.0f,
							thisGeomSink
							)))
			{
				newPath->Release();
				return;
			}
			thisGeomSink->Close();
		}

		fPathD2D = newPath;
		oldPath->Release();

		if (inIntersect)
			fBounds.Intersect( inPath.GetBounds());
		else
			fBounds.Union( inPath.GetBounds());

		fCreateStorageForCrispEdges = false;
		fHasBezier = false;
		fDrawCmds.clear();
		fCurTransform.MakeIdentity();
	}
}

void VGraphicPath::Widen(const GReal inStrokeWidth, ID2D1StrokeStyle* inStrokeStyle)
{
	xbox_assert(inStrokeStyle);
	End();

	if (fPathD2D != NULL)
	{
		CComPtr<ID2D1StrokeStyle> strokeStyle = (ID2D1StrokeStyle *)inStrokeStyle;
		ID2D1Geometry *oldPath = fPathD2D;

		//create new path
		ID2D1PathGeometry *newPath = NULL;
		VWinD2DGraphicContext::GetMutexFactory().Lock();
		HRESULT hr = VWinD2DGraphicContext::GetFactory()->CreatePathGeometry( &newPath);
		xbox_assert(SUCCEEDED(hr));
		VWinD2DGraphicContext::GetMutexFactory().Unlock();

		//widen current path
		CComPtr<ID2D1GeometrySink> thisGeomSink;
		if (SUCCEEDED(newPath->Open(&thisGeomSink)))
		{
			thisGeomSink->SetFillMode( fFillMode == FILLRULE_EVENODD ? D2D1_FILL_MODE_ALTERNATE : D2D1_FILL_MODE_WINDING);

			if (!SUCCEEDED(oldPath->Widen(
							inStrokeWidth,
							strokeStyle,
							NULL,
							0.0f,
							thisGeomSink)))
			{
				newPath->Release();
				return;
			}
			thisGeomSink->Close();
		}

		fPathD2D = newPath;
		oldPath->Release();

		fCreateStorageForCrispEdges = false;
		fHasBezier = false;
		fDrawCmds.clear();
		fCurTransform.MakeIdentity();
	}
}

void VGraphicPath::Outline()
{
	End();

	if (fPathD2D != NULL)
	{
		ID2D1Geometry *oldPath = fPathD2D;

		//create new path
		ID2D1PathGeometry *newPath = NULL;
		VWinD2DGraphicContext::GetMutexFactory().Lock();
		HRESULT hr = VWinD2DGraphicContext::GetFactory()->CreatePathGeometry( &newPath);
		xbox_assert(SUCCEEDED(hr));
		VWinD2DGraphicContext::GetMutexFactory().Unlock();

		//outline current path
		CComPtr<ID2D1GeometrySink> thisGeomSink;
		if (SUCCEEDED(newPath->Open(&thisGeomSink)))
		{
			thisGeomSink->SetFillMode( fFillMode == FILLRULE_EVENODD ? D2D1_FILL_MODE_ALTERNATE : D2D1_FILL_MODE_WINDING);

			if (!SUCCEEDED(oldPath->Outline(
							NULL,
							0.0f,
							thisGeomSink)))
			{
				newPath->Release();
				return;
			}
			thisGeomSink->Close();
		}

		fPathD2D = newPath;
		oldPath->Release();

		fCreateStorageForCrispEdges = false;
		fHasBezier = false;
		fDrawCmds.clear();
		fCurTransform.MakeIdentity();
	}
}

#endif

void VGraphicPath::Begin()
{
#if ENABLE_D2D
	//start to populate geometry
	if (fGeomSink)
		return;
	if (fPathD2D == NULL)
		return;

	ID2D1PathGeometry *thisPath = NULL;
	if (FAILED(fPathD2D->QueryInterface( __uuidof(ID2D1PathGeometry), (void **)&thisPath)))
		return; //no more a ID2D1PathGeometry so skip
	ID2D1GeometrySink *thisGeomSink;
	if (SUCCEEDED(thisPath->Open(&thisGeomSink)))
	{
		fGeomSink = thisGeomSink;
		thisGeomSink->SetFillMode( fFillMode == FILLRULE_EVENODD ? D2D1_FILL_MODE_ALTERNATE : D2D1_FILL_MODE_WINDING);
		fFigureIsClosed = true;
	}
	thisPath->Release();
#endif
}

void VGraphicPath::End()
{
#if ENABLE_D2D
	if (fGeomSink)
	{
		if (!fFigureIsClosed)
			fGeomSink->EndFigure( D2D1_FIGURE_END_OPEN);
		fFigureIsClosed = true;
		fGeomSink->Close();
		fGeomSink->Release();
		fGeomSink = NULL;
		_ComputeBounds();
	}
#endif
}
void VGraphicPath::BeginSubPathAt(const VPoint& inLocalPoint)
{
	fLastPoint = VPoint((GReal)inLocalPoint.GetX(), (GReal)inLocalPoint.GetY());
	fPath->StartFigure();

#if ENABLE_D2D
	if (VWinD2DGraphicContext::IsAvailable())
	{
		if (!fGeomSink)
		{
			Begin();
			if (!fGeomSink)
				return;
		}

		if (!fFigureIsClosed)
			//do not forget to close current figure before opening new figure to prevent really nasty artifacts...
			fGeomSink->EndFigure( D2D1_FIGURE_END_OPEN);
		fGeomSink->BeginFigure( D2D1::Point2F( inLocalPoint.GetX(), inLocalPoint.GetY()),
								   D2D1_FIGURE_BEGIN_FILLED);
		fFigureIsClosed = false;
	}
#endif

	fPolygon.AddPoint(inLocalPoint);

	if (fComputeBoundsAccurate)
		_addPointToBounds( inLocalPoint);
	_ComputeBounds();

	if (fCreateStorageForCrispEdges)
		fDrawCmds.push_back( VGraphicPathDrawCmd( GPDCT_BEGIN_SUBPATH_AT, inLocalPoint));
}


void VGraphicPath::AddLineTo(const VPoint& inDestPoint)
{
	Gdiplus::PointF d = Gdiplus::PointF(inDestPoint.GetX(), inDestPoint.GetY());
	fPath->AddLine(Gdiplus::PointF(fLastPoint.x,fLastPoint.y), d);
	fLastPoint.x = d.X;
	fLastPoint.y = d.Y;

#if ENABLE_D2D
	if (VWinD2DGraphicContext::IsAvailable())
	{
		if (!fGeomSink)
		{
			Begin();
			if (!fGeomSink)
				return;
		}

		fGeomSink->AddLine( D2D1::Point2F( inDestPoint.GetX(), inDestPoint.GetY()));
	}
#endif

	fPolygon.AddPoint(inDestPoint);

	if (fComputeBoundsAccurate)
		_addPointToBounds( inDestPoint);
	_ComputeBounds();

	if (fCreateStorageForCrispEdges)
		fDrawCmds.push_back( VGraphicPathDrawCmd( GPDCT_ADD_LINE_TO, inDestPoint));
}


void VGraphicPath::AddBezierTo(const VPoint& inStartControl, const VPoint& inDestPoint, const VPoint& inDestControl)
{
	Gdiplus::PointF d = Gdiplus::PointF(inDestPoint.GetX(), inDestPoint.GetY());
	Gdiplus::PointF c1 = Gdiplus::PointF(inStartControl.GetX(), inStartControl.GetY());
	Gdiplus::PointF c2 = Gdiplus::PointF(inDestControl.GetX(), inDestControl.GetY());
		
	fPath->AddBezier(Gdiplus::PointF(fLastPoint.x,fLastPoint.y), c1, c2, d);
	fLastPoint.x = d.X;
	fLastPoint.y = d.Y;

#if ENABLE_D2D
	if (VWinD2DGraphicContext::IsAvailable())
	{
		if (!fGeomSink)
		{
			Begin();
			if (!fGeomSink)
				return;
		}

		D2D1_POINT_2F c1 = D2D1::Point2F(inStartControl.GetX(), inStartControl.GetY());
		D2D1_POINT_2F c2 = D2D1::Point2F(inDestControl.GetX(), inDestControl.GetY());
		D2D1_POINT_2F d = D2D1::Point2F(inDestPoint.GetX(), inDestPoint.GetY());

		fGeomSink->AddBezier( D2D1::BezierSegment(c1, c2, d));
	}
#endif

	fPolygon.AddPoint(inDestPoint);

	if (fComputeBoundsAccurate)
		_addBezierToBounds( inStartControl, inDestControl, inDestPoint);
	_ComputeBounds();

	fHasBezier = true;
	if (fCreateStorageForCrispEdges)
		fDrawCmds.push_back( VGraphicPathDrawCmd( GPDCT_ADD_BEZIER_TO, inStartControl, inDestPoint, inDestControl));
}


void VGraphicPath::AddOval(const VRect& inBounds)
{
	Gdiplus::RectF rect(inBounds.GetLeft(), inBounds.GetTop(), inBounds.GetWidth(), inBounds.GetHeight());
	fPath->AddEllipse(rect);

#if ENABLE_D2D
	if (VWinD2DGraphicContext::IsAvailable())
	{
		if (!fGeomSink)
		{
			Begin();
			if (!fGeomSink)
				return;
		}

		if (!fFigureIsClosed)
			fGeomSink->EndFigure( D2D1_FIGURE_END_OPEN);
		fFigureIsClosed = true;

		GReal radiusX = inBounds.GetWidth()*0.5f;
		GReal radiusY = inBounds.GetHeight()*0.5f;
		D2D1_POINT_2F center = D2D1::Point2F( inBounds.GetLeft()+radiusX, inBounds.GetTop()+radiusY);
		D2D1_POINT_2F s = D2D1::Point2F(center.x-radiusX, center.y);
		D2D1_POINT_2F d = D2D1::Point2F(center.x+radiusX, center.y);

		fGeomSink->BeginFigure( s, D2D1_FIGURE_BEGIN_FILLED);
		fGeomSink->AddArc( D2D1::ArcSegment( d, D2D1::SizeF(radiusX, radiusY), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
		fGeomSink->AddArc( D2D1::ArcSegment( s, D2D1::SizeF(radiusX, radiusY), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
		fGeomSink->EndFigure( D2D1_FIGURE_END_OPEN);
	}
#endif

	fPolygon.AddPoint(inBounds.GetTopLeft());
	fPolygon.AddPoint(inBounds.GetTopRight());
	fPolygon.AddPoint(inBounds.GetBotRight());
	fPolygon.AddPoint(inBounds.GetBotLeft());

	if (fComputeBoundsAccurate)
		_addRectToBounds( inBounds);
	_ComputeBounds();

	if (fCreateStorageForCrispEdges)
		fDrawCmds.push_back( VGraphicPathDrawCmd( GPDCT_ADD_OVAL, inBounds));
}


void VGraphicPath::CloseSubPath()
{
	fPath->CloseFigure();

#if ENABLE_D2D
	if (VWinD2DGraphicContext::IsAvailable())
	{
		if (fGeomSink)
		{
			fGeomSink->EndFigure( D2D1_FIGURE_END_CLOSED);
			fFigureIsClosed = true;
		}
	}
#endif
	fPolygon.Close();

	if (fCreateStorageForCrispEdges)
		fDrawCmds.push_back( VGraphicPathDrawCmd( GPDCT_CLOSE_SUBPATH));
}


void VGraphicPath::Clear()
{
	fPath->Reset();
	fLastPoint.x = 0.0f;
	fLastPoint.y = 0.0f;

#if ENABLE_D2D
	if (VWinD2DGraphicContext::IsAvailable())
	{
		End();

		if (fPathD2D != NULL)
		{
			fPathD2D->Release();
			fPathD2D = NULL;
		}
		VWinD2DGraphicContext::GetMutexFactory().Lock();
		HRESULT hr = VWinD2DGraphicContext::GetFactory()->CreatePathGeometry( (ID2D1PathGeometry **)&fPathD2D);
		xbox_assert(SUCCEEDED(hr));
		VWinD2DGraphicContext::GetMutexFactory().Unlock();
	}
#endif

	if (fComputeBoundsAccurate)
	{
		fPathPrevPoint	= GP_BOUNDS_MIN_DEFAULT;
		fPathMin		= GP_BOUNDS_MIN_DEFAULT;
		fPathMax		= GP_BOUNDS_MAX_DEFAULT;
	}
	fBounds			= VRect(0,0,0,0);

	fPolygon.Clear();

	fDrawCmds.clear();
	fCurTransform.MakeIdentity();
	fHasBezier = false;
}


void VGraphicPath::SetPosBy(GReal inHoriz, GReal inVert)
{
	Gdiplus::Matrix matrix;
	matrix.Translate(inHoriz, inVert);
	fPath->Transform(&matrix);

#if ENABLE_D2D
	if (VWinD2DGraphicContext::IsAvailable() && fPathD2D)
	{
		End();
		
		VAffineTransform mat;
		mat.SetTranslation( inHoriz, inVert);
		D2D1_MATRIX_3X2_F matNative;
		mat.ToNativeMatrix((D2D_MATRIX_REF)&matNative);

		ID2D1Geometry *sourcePath = fPathD2D;
		
		ID2D1TransformedGeometry *thisPath = NULL;
		VWinD2DGraphicContext::GetMutexFactory().Lock();
		bool ok = SUCCEEDED(VWinD2DGraphicContext::GetFactory()->CreateTransformedGeometry( sourcePath, &matNative, &thisPath));
		VWinD2DGraphicContext::GetMutexFactory().Unlock();
		xbox_assert(ok);
		if (ok)
		{
			fPathD2D = thisPath;
			sourcePath->Release();
		}
		else 
			return;
	}
#endif

	fPolygon.SetPosBy(inHoriz, inVert);

	if (fComputeBoundsAccurate)
	{
		VPoint translate( (SmallReal)inHoriz, (SmallReal)inVert);

		if (fPathMin != GP_BOUNDS_MIN_DEFAULT)
			fPathMin += translate;
		if (fPathMax != GP_BOUNDS_MAX_DEFAULT)
			fPathMax += translate;
	}
	_ComputeBounds();

	if (fCreateStorageForCrispEdges)
	{
		fDrawCmds.push_back( VGraphicPathDrawCmd( GPDCT_SET_POS_BY, inHoriz, inVert));
		fCurTransform.Translate( inHoriz, inVert, VAffineTransform::MatrixOrderAppend);
	}
}


void VGraphicPath::SetSizeBy(GReal inWidth, GReal inHeight)
{
	Gdiplus::Matrix matrix;
	matrix.Scale(1.0 + inWidth / fBounds.GetWidth(), 1.0 + inHeight / fBounds.GetHeight());
	fPath->Transform(&matrix);

#if ENABLE_D2D
	if (VWinD2DGraphicContext::IsAvailable() && fPathD2D)
	{
		End();
		
		VAffineTransform mat;
		mat.SetScaling( 1.0 + inWidth / fBounds.GetWidth(), 1.0 + inHeight / fBounds.GetHeight());
		D2D1_MATRIX_3X2_F matNative;
		mat.ToNativeMatrix((D2D_MATRIX_REF)&matNative);

		ID2D1Geometry *sourcePath = fPathD2D;
		
		ID2D1TransformedGeometry *thisPath = NULL;
		VWinD2DGraphicContext::GetMutexFactory().Lock();
		bool ok = SUCCEEDED(VWinD2DGraphicContext::GetFactory()->CreateTransformedGeometry( sourcePath, &matNative, &thisPath));
		VWinD2DGraphicContext::GetMutexFactory().Unlock();
		xbox_assert(ok);
		if (ok)
		{
			fPathD2D = thisPath;
			sourcePath->Release();
		}
		else 
			return;
	}
#endif

	fPolygon.SetSizeBy(inWidth, inHeight);

	if (fComputeBoundsAccurate)
	{
		VPoint inflate( (SmallReal)inWidth, (SmallReal)inHeight);

		if (fPathMax != GP_BOUNDS_MAX_DEFAULT)
			fPathMax += inflate;
	}
	_ComputeBounds();

	if (fCreateStorageForCrispEdges)
	{
		fDrawCmds.push_back( VGraphicPathDrawCmd( GPDCT_SET_SIZE_BY, inWidth, inHeight));
		fCurTransform.Scale( 1 + inWidth / fBounds.GetWidth(), 1 + inHeight / fBounds.GetHeight(), VAffineTransform::MatrixOrderAppend);
	}
}


Boolean VGraphicPath::HitTest(const VPoint& inPoint) const
{
#if ENABLE_D2D
	if (fPathD2D != NULL)
	{
		BOOL bContains = FALSE;
		fPathD2D->StrokeContainsPoint( D2D1::Point2F(inPoint.GetX(), inPoint.GetY()),
									   4,
									   NULL,
									   NULL,
									   &bContains);
		return bContains == TRUE;
	}
#endif

	Gdiplus::Color black_default_color(0,0,0);
	Gdiplus::Pen p(black_default_color);
	p.SetWidth(4);
	return fPath->IsOutlineVisible((Gdiplus::REAL)inPoint.GetX(), (Gdiplus::REAL)inPoint.GetY(), &p );
}

// JM 201206
Boolean VGraphicPath::ScaledHitTest(const VPoint& inPoint, GReal scale) const
{
#if ENABLE_D2D
	if (fPathD2D != NULL)
	{
		BOOL bContains = FALSE;
		fPathD2D->StrokeContainsPoint( D2D1::Point2F(inPoint.GetX(), inPoint.GetY()),
									   4*scale,
									   NULL,
									   NULL,
									   &bContains);
		return bContains == TRUE;
	}
#endif

	Gdiplus::Color black_default_color(0,0,0);
	Gdiplus::Pen p(black_default_color);
	p.SetWidth(4 * scale);
	return fPath->IsOutlineVisible((Gdiplus::REAL)inPoint.GetX(), (Gdiplus::REAL)inPoint.GetY(), &p );
}


void VGraphicPath::Inset(GReal inHoriz, GReal inVert)
{
	SetPosBy(inHoriz, inVert);
	SetSizeBy(-2.0 * inHoriz, -2.0 * inVert);
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
#if ENABLE_D2D
		if (fPathD2D != NULL)
		{
			if (fGeomSink == NULL)
			{
				D2D1_RECT_F bounds;
				fPathD2D->GetBounds( NULL, &bounds);
				fBounds.SetCoords( bounds.left, bounds.top, bounds.right-bounds.left, bounds.bottom-bounds.top);
				return;
			}
		}
#endif

		Gdiplus::RectF bounds;
		fPath->GetBounds( &bounds);
		fBounds = VRect ( bounds.GetLeft(), bounds.GetTop(), bounds.GetRight()-bounds.GetLeft(), bounds.GetBottom()-bounds.GetTop());
	}
}



void VGraphicPath::SetFillMode( FillRuleType inFillMode)
{
	fFillMode = inFillMode;

	if (inFillMode == FILLRULE_EVENODD)
		fPath->SetFillMode( Gdiplus::FillModeAlternate);
	else
		fPath->SetFillMode( Gdiplus::FillModeWinding);

#if ENABLE_D2D
	if (fGeomSink != NULL)
		fGeomSink->SetFillMode( inFillMode == FILLRULE_EVENODD ? D2D1_FILL_MODE_ALTERNATE : D2D1_FILL_MODE_WINDING);
#endif
}

FillRuleType VGraphicPath::GetFillMode() const
{
	return fFillMode;
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
	Real inRx = _inRX, inRy = _inRY; 
	Real inX = fLastPoint.x, inY = fLastPoint.y;
	Real inEX = inDestPoint.GetX(), inEY = inDestPoint.GetY();
	
	Real sin_th, cos_th;
    Real a00, a01, a10, a11;
    Real x0, y0, x1, y1, xc, yc;
    Real d, sfactor, sfactor_sq;
    Real th0, th1, th_arc;
    sLONG i, n_segs;
    Real dx, dy, dx1, dy1, Pr1, Pr2, Px, Py, check;

    inRx = fabs(inRx);
    inRy = fabs(inRy);

    sin_th = sin(inXAxisRotation * (PI / 180.0));
    cos_th = cos(inXAxisRotation * (PI / 180.0));

    dx = (inX - inEX) / 2.0;
    dy = (inY - inEY) / 2.0;
    dx1 =  cos_th * dx + sin_th * dy;
    dy1 = -sin_th * dx + cos_th * dy;
    Pr1 = inRx * inRx;
    Pr2 = inRy * inRy;
    Px = dx1 * dx1;
    Py = dy1 * dy1;
    /* Spec : check if radii are large enough */
    check = Px / Pr1 + Py / Pr2;
    if (check > 1) {
        inRx = inRx * sqrt(check);
        inRy = inRy * sqrt(check);
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

    th0 = atan2(y0 - yc, x0 - xc);
    th1 = atan2(y1 - yc, x1 - xc);

    th_arc = th1 - th0;
	if (th0 <= 0.0 && th1 > 0.0 && fabs(th_arc-PI) <= 0.00001)
		th_arc = -th_arc;	//ensure we draw counterclockwise if angle delta = 180°
	else if (fabs(th_arc)-PI > 0.00001 && !inLargeArcFlag)
	{
		if (th_arc > 0)
			th_arc -= 2 * PI;
		else
			th_arc += 2 * PI;
	}
	else if (fabs(th_arc) <= PI && inLargeArcFlag)
	{
		if (th_arc > 0)
			th_arc -= 2 * PI;
		else
			th_arc += 2 * PI;
	}

	//split arc in small arcs <= 90°

    n_segs = int(ceil(fabs(th_arc / (PI * 0.5 + 0.001))));
    for (i = 0; i < n_segs; i++) 
	{
        _PathAddArcSegment(	xc, yc,
							th0 + i * th_arc / n_segs,
							th0 + (i + 1) * th_arc / n_segs,
							inRx, inRy, 
							inXAxisRotation);
    }
}





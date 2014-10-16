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
#ifndef __VPolygon__
#define __VPolygon__

#include "Graphics/Sources/IBoundedShape.h"

BEGIN_TOOLBOX_NAMESPACE

#define VPOLYGON_NATIVE_POINTS_FIXED_SIZE 12  //if more points than this value, native points will be allocated dynamically otherwise stored locally

class XTOOLBOX_API VPolygon : public VObject, public IBoundedShape, public IStreamable
{
public:
	/** platform-independent point vector */
	typedef std::vector<VPoint> VectorOfPoint;

	/** native point type definition */
#if VERSIONWIN
	typedef POINT NativePoint;
#elif VERSIONMAC
	typedef CGPoint NativePoint;
#else
	//linux ?
	typedef VPoint NativePoint;
#endif

						VPolygon ();
						VPolygon (const VPolygon& inOriginal);
						VPolygon (const VectorOfPoint& inPoints, bool inClose = true);
						VPolygon (const VRect& inRect);

	virtual				~VPolygon ();
			void		FromVRect(const VRect& inRect);
	
	// Polygon construction
			void		AddPoint (const VPoint& inLocalPoint);
			void		Close ();
			void		Clear ();
	
	// IBoundedShape API
	virtual void		SetPosBy (GReal inHoriz, GReal inVert);
	virtual void		SetSizeBy (GReal inWidth, GReal inHeight);
	
	virtual Boolean		HitTest (const VPoint& inPoint) const;
	virtual void		Inset (GReal inHoriz, GReal inVert);
	
	// Stream API
	virtual VError		WriteToStream (VStream* ioStream, sLONG inParam) const;
	virtual VError		ReadFromStream (VStream* ioStream, sLONG inParam);
	
	// Accessors
			sLONG		GetPointCount () const;
			void		GetNthPoint (sLONG inIndex, VPoint& outPoint) const;
	
	// Operators
			VPolygon&	operator = (const VPolygon& inOriginal);
			Boolean		operator == (const VPolygon& inPolygon) const;

			const VPolygon::NativePoint* GetNativePoints() const;

protected:
			void		_ClearNativePoints();
			void		_ComputeBounds (const VPoint& inLastPoint);

			friend	class	VRegion;
	
	mutable VectorOfPoint	fPoints;

			/** native point table (stored locally to avoid to create it each time it is requested) 
			@remarks
				it is created locally on demand i.e. if native platform requests for vector of native points (normally only if polygon is painted)
			*/
	mutable	NativePoint*	fPtrNativePoints;
	mutable	NativePoint		fNativePoints[VPOLYGON_NATIVE_POINTS_FIXED_SIZE];
	mutable	bool			fNativePointsInitialized;

};


END_TOOLBOX_NAMESPACE

#endif

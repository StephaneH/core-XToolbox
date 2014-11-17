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
#ifndef __VPattern__
#define __VPattern__

#include "Graphics/Sources/VGraphicContext.h"
#include "Graphics/Sources/VRect.h"
#include "Graphics/Sources/VPolygon.h"
#include "Graphics/Sources/VAffineTransform.h"

BEGIN_TOOLBOX_NAMESPACE

//patter wrapping mode
typedef enum PatternWrapMode
{
	ePatternWrapClamp,	//pattern is not repeated
	ePatternWrapTile,	//pattern is repeated 
	ePatternWrapFlipX,	//pattern is repeated and alternatively reflected in X direction
	ePatternWrapFlipY,  //pattern is repeated and alternatively reflected in Y direction
	ePatternWrapFlipXY  //pattern is repeated and alternatively reflected in X and Y direction
} PatternWrapMode;


class XTOOLBOX_API VFileBitmap : public VObject
{
private:
	VString	fPath;
	void BuildBitmap();

public:
	VFileBitmap(const VString &inPath);
	VFileBitmap(const VFileBitmap &inFrom);
	virtual	~VFileBitmap ();

	void GetBounds(VRect &outRect) const;
	void GetPath(VString &outPath) const;

#if VERSIONWIN
	Gdiplus::Bitmap	*fBitmap;
#endif
#if VERSIONMAC
	CGImageRef	fBitmap;
#endif
};

class XTOOLBOX_API VPattern : public VObject, public IStreamable, public IRefCountable
{
public:
	enum	{ Pattern_Kind = 'nulP' };
	
			VPattern ();
			VPattern (const VPattern& inOriginal);
	virtual	~VPattern ();
	
	// Context handling
	virtual void	ApplyToContext (VGraphicContext* inContext) const;
	virtual void	ReleaseFromContext (VGraphicContext* inContext) const;
	
	// Pattern description
	virtual Boolean	IsLinePattern () const = 0;
	virtual Boolean	IsFillPattern () const = 0;
	virtual Boolean	IsPlain () const = 0;
	virtual Boolean	IsEmpty () const = 0;
	virtual Boolean IsGradient() const = 0;
	
	// wrap mode accessors
	virtual void SetWrapMode( PatternWrapMode inWrapMode)
	{
		fWrapMode = inWrapMode;
		_ComputeFactors();
	}
	virtual PatternWrapMode GetWrapMode() const
	{
		return fWrapMode;
	}

	// Cloning
	virtual VPattern*	Clone () const { return NULL; };
	virtual OsType	GetKind () const { return Pattern_Kind; };

	// Inherited from IStreamable
	virtual VError	ReadFromStream (VStream* inStream, sLONG inParam = 0);
	virtual VError	WriteToStream (VStream* inStream, sLONG inParam = 0) const;
	
	// Class Utilities (equivalent to VFont APIs)
	static VPattern*	RetainStdPattern (PatternStyle inStyle);
	static VPattern*	RetainPatternFromStream (VStream* inStream, OsType inKind);
	
protected:
	// Override to apply the pattern (NULL means current port/context)
	virtual void	DoApplyPattern (ContextRef inContext, FillRuleType inFillRule = FILLRULE_NONZERO) const;
	virtual void	DoReleasePattern (ContextRef inContext) const;

	virtual void	_ComputeFactors() {}
	
	
	//current wrapping mode
	PatternWrapMode fWrapMode;
};


class XTOOLBOX_API VStandardPattern : public VPattern
{
public:
	enum	{ Pattern_Kind = 'stdP' };
	
			VStandardPattern (PatternStyle inStyle = PAT_FILL_PLAIN, const VColor* inColor = NULL);
			VStandardPattern (const VStandardPattern& inOriginal);
	virtual	~VStandardPattern ();
	
	// Accessors
	virtual void	SetPatternStyle (PatternStyle inStyle);
	PatternStyle	GetPatternStyle () const { return fPatternStyle; };
	
	virtual void	SetPatternColor (const VColor& inColor) { fPatternColor = inColor; };
	const VColor&	GetPatternColor () const { return fPatternColor; };
	
	// Inherited from VPattern
	virtual Boolean	IsLinePattern () const { return (fPatternStyle > PAT_FILL_LAST); };
	virtual Boolean	IsFillPattern () const { return (fPatternStyle <= PAT_FILL_LAST); };
	virtual Boolean	IsPlain () const { return (fPatternStyle == PAT_FILL_PLAIN || fPatternStyle == PAT_LINE_PLAIN); };
	virtual Boolean	IsEmpty () const { return (fPatternStyle == PAT_FILL_EMPTY); };
	virtual Boolean IsGradient() const { return false; }
	
	// Cloning
	virtual VPattern*	Clone () const { return new VStandardPattern(*this); };
	VStandardPattern& operator = (const VStandardPattern& inOriginal);
	virtual OsType	GetKind () const { return Pattern_Kind; };

	// Inherited from IStreamable
	virtual VError	ReadFromStream (VStream* inStream, sLONG inParam = 0);
	virtual VError	WriteToStream (VStream* inStream, sLONG inParam = 0) const;
	
	// Platform-related Utilities

#if VERSIONWIN
	HBRUSH	WIN_CreateBrush () const;
	operator	HBRUSH () const { return WIN_CreateBrush(); };
#endif

#if USE_OBSOLETE_API
	void	FromStdPattern (PatternStyle inStdPattern) { SetPatternStyle(inStdPattern); };
#endif

protected:
	PatternStyle	fPatternStyle;	// Standard pattern style
	VColor			fPatternColor;	// Pattern color
	mutable PatternRef	fPatternRef;// Store platform-specific data
	
#if VERSIONWIN
	sBYTE	fBuffer[16];
#endif

	virtual void	DoApplyPattern (ContextRef inContext, FillRuleType inFillRule = FILLRULE_NONZERO) const;
	virtual void	DoReleasePattern (ContextRef inContext) const;

private:
	void	_Init ();
	void	_Release ();
};


/** gradient stop color */
typedef std::pair<GReal,VColor> GradientStop;

/** gradient stop color vector */
typedef std::vector<GradientStop> GradientStopVector;

/** gradient lookup table color size */
#define GRADIENT_LUT_SIZE	128

/** max gradient repeat count
 @remarks
	this is used to implement repeat and reflect wrapping modes for axial and radial gradients 
 */
#if VERSIONMAC
const int	kPATTERN_GRADIENT_TILE_MAX_COUNT = 16;
#else
/** actually in Windows implementation, we use the same technique than on Mac OS 
	(enlarging radius * kPATTERN_GRADIENT_TILE_MAX_COUNT and adjust exterior radius center if focal is not centered)
	but here as gdiplus lut is used on the total radius range (and not only on the tile range as for Mac OS implementation)
	we need to lower tile count in order to reduce color interpolation discrepancies
	between pad and tiling wrapping modes (otherwise reflect or repeat rendering would be blurry)
*/
const int	kPATTERN_GRADIENT_TILE_MAX_COUNT = 8;
#endif


class XTOOLBOX_API VGradientPattern : public VPattern
{
public:
	enum	{ Pattern_Kind = 'graP' };

protected:	
			VGradientPattern ();
			VGradientPattern (GradientStyle inStyle, const VPoint& inStartPoint, const VPoint& inEndPoint, const VColor& inStartColor, const VColor& inEndColor);
			//multicolor gradient constructor
			VGradientPattern (GradientStyle inStyle, const VPoint& inStartPoint, const VPoint& inEndPoint, const GradientStopVector& inGradStops, const VAffineTransform& inTransform = VAffineTransform());
			VGradientPattern (const VGradientPattern& inOriginal);
public:
	virtual	~VGradientPattern ();
	
	
	// Gradient support
	virtual void	ComputeValues (const GReal* inValue, GReal* outValue);

	// Accessors
	virtual void	SetGradientStyle (GradientStyle inStyle);
	GradientStyle	GetGradientStyle () const { return fGradientStyle; };
	
	virtual void	SetStartPoint (const VPoint& inPoint);
	const VPoint&	GetStartPoint () const { return fStartPoint; };
	
	virtual void	SetEndPoint (const VPoint& inPoint);
	const VPoint&	GetEndPoint () const { return fEndPoint; };
	
	virtual void	SetStartColor (const VColor& inColor) { fStartColor = inColor; };
	const VColor&	GetStartColor () const { return fStartColor; };
	
	virtual void	SetEndColor (const VColor& inColor) { fEndColor = inColor; };
	const VColor&	GetEndColor () const { return fEndColor; };
	

	/** define a gradient with more than 2 colors
	@remarks
		each stop is defined by a pair <offset,color> where offset is [0-1] value 
		which is mapped to [start point-end point] position
		(so gradient can start farer than start point and finish before end point)

		if style is not set yet to GRAD_STYLE_LUT or GRAD_STYLE_LUT_FAST
		it will be set to GRAD_STYLE_LUT
	*/
	virtual void	SetGradientStops( const GradientStopVector& inGradStops);

	/** const read accessor on gradient stops */
	const GradientStopVector&  GetGradientStops() const
	{
		return fGradientStops;
	}

	/** set gradient transform */
	void SetTransform( const VAffineTransform& inTransform)
	{
		fTransform = inTransform;
		_ComputeFactors();
	}
	
	/** get gradient transform */
	const VAffineTransform& GetTransform() const
	{
		return fTransform;
	}
	
	
	// Cloning
	VGradientPattern& operator = (const VGradientPattern& inOriginal);
	virtual OsType	GetKind () const { return Pattern_Kind; };
	
	// Inherited from VPattern
	virtual Boolean	IsLinePattern () const { return true; };
	virtual Boolean	IsFillPattern () const { return true; };
	virtual Boolean	IsPlain () const { return false; };
	virtual Boolean	IsEmpty () const { return false; };
	virtual Boolean IsGradient() const { return true; }

	// Inherited from IStreamable
	virtual VError	ReadFromStream (VStream* inStream, sLONG inParam = 0);
	virtual VError	WriteToStream (VStream* inStream, sLONG inParam = 0) const;

protected:
	GradientStyle	fGradientStyle;	// Standard gradient style
	mutable GradientRef		fGradientRef;	// Store platform-specific data
	VPoint	fStartPoint;	// Start coords for gradient
	VPoint	fEndPoint;		// End coords for gradient
	VColor	fStartColor;	// Color for start point
	VColor	fEndColor;		// Color for end point
	
	/** gradient transformation matrix */
	VAffineTransform fTransform;
		
	/** gradient stop color vector 
	@remarks
		used only if gradient style is GRAD_STYLE_LUT or GRAD_STYLE_LUT_FAST
	*/
	GradientStopVector fGradientStops;
#if VERSIONMAC
	/** gradient lookup table 
	@remarks
		lut is used only if gradient is defined with more than 2 colors
		(to speed up color picking and linear interpolation)
	*/
	GReal fLUT[GRADIENT_LUT_SIZE][4];
#endif

	// Gradient computing
	virtual void	DoComputeValues (const GReal* inValue, GReal* outValue) = 0;

private:
	void	_Init ();
	void	_Release ();
};


class XTOOLBOX_API VAxialGradientPattern : public VGradientPattern
{
public:
	enum	{ Pattern_Kind = 'axeP' };
	
			VAxialGradientPattern ();
			VAxialGradientPattern (GradientStyle inStyle, const VPoint& inStartPoint, const VPoint& inEndPoint, const VColor& inStartColor, const VColor& inEndColor);
			VAxialGradientPattern (GradientStyle inStyle, const VPoint& inStartPoint, const VPoint& inCentralPoint, const VPoint& inEndPoint, const VColor& inStartColor, const VColor& inCentralColor, const VColor& inEndColor);
			//multicolor gradient constructor
			VAxialGradientPattern (GradientStyle inStyle, const VPoint& inStartPoint, const VPoint& inEndPoint, const GradientStopVector& inGradStops, const VAffineTransform& inTransform = VAffineTransform());
			
			VAxialGradientPattern (const VAxialGradientPattern& inOriginal);
	virtual	~VAxialGradientPattern ();
	
	// Accessors
	virtual void	SetStartColor (const VColor& inColor) { VGradientPattern::SetStartColor(inColor); _ComputeFactors(); }; // JM 191006 il faut mettre à jour
	virtual void	SetEndColor (const VColor& inColor) { VGradientPattern::SetEndColor(inColor); _ComputeFactors(); };	    // les coefficients du gradient...

	virtual void	SetCentralColor (const VColor& inColor) { fCentralColor = inColor; _ComputeFactors();};
	const VColor&	GetCentralColor () const { return fCentralColor; };
	
	virtual void	SetCentralPoint (const VPoint& inPoint) { fCentralPoint = inPoint; _ComputeFactors();};
	const VPoint&	GetCentralPoint () const { return fCentralPoint; };
	
	virtual void	SetBilinear (Boolean inSet) { fBilinear = inSet; _ComputeFactors();};
	Boolean	IsBilinear () const { return fBilinear; };
	
	// Cloning
	virtual VPattern*	Clone () const { return new VAxialGradientPattern(*this); };
	VAxialGradientPattern& operator = (const VAxialGradientPattern& inOriginal);
	virtual OsType	GetKind () const { return Pattern_Kind; };

	// Inherited from IStreamable
	virtual VError	ReadFromStream (VStream* inStream, sLONG inParam = 0);
	virtual VError	WriteToStream (VStream* inStream, sLONG inParam = 0) const;
	
protected:
			void	_ComputeFactors();
	
	VPoint		fCentralPoint;	// Additional color point (between start and end)
	VColor		fCentralColor;	// Color for additional point
	Boolean		fBilinear;		// Specify if uses additional point

	// pre-computed factors to speed up DoComputeValues
	GReal	fSpreadFactor[4];
	GReal	fStartComponent[4];
	GReal	fCentralValue;
	
	// Gradient computing
	virtual void	DoComputeValues (const GReal* inValue, GReal* outValue);
	
	// Inherited from VPattern
	virtual void	DoApplyPattern (ContextRef inContext, FillRuleType inFillRule = FILLRULE_NONZERO) const;

private:
	void	_Init ();
};


class XTOOLBOX_API VRadialGradientPattern : public VGradientPattern
{
public:
	enum	{ Pattern_Kind = 'radP' };
	
			VRadialGradientPattern ();
			//circular gradient constructor
			VRadialGradientPattern (GradientStyle inStyle, const VPoint& inStartPoint, const VPoint& inEndPoint, const VColor& inStartColor, const VColor& inEndColor, GReal inStartRadius, GReal inEndRadius);
			//ellipsoid gradient constructor
			VRadialGradientPattern(GradientStyle inStyle, const VPoint& inStartPoint, const VPoint& inEndPoint, const VColor& inStartColor, const VColor& inEndColor, GReal inStartRadiusX, GReal inStartRadiusY, GReal inEndRadiusX, GReal inEndRadiusY);
			//multicolor circular gradient constructor
			VRadialGradientPattern (GradientStyle inStyle, const VPoint& inStartPoint, const VPoint& inEndPoint, GReal inStartRadius, GReal inEndRadius, const GradientStopVector& inGradStops, const VAffineTransform& inTransform = VAffineTransform());
			//multicolor ellipsoid gradient constructor
			VRadialGradientPattern (GradientStyle inStyle, const VPoint& inStartPoint, const VPoint& inEndPoint, GReal inStartRadiusX, GReal inStartRadiusY, GReal inEndRadiusX, GReal inEndRadiusY, const GradientStopVector& inGradStops, const VAffineTransform& inTransform = VAffineTransform());

			//copy constructor
			VRadialGradientPattern (const VRadialGradientPattern& inOriginal);

	virtual	~VRadialGradientPattern ();
	
	void	SetStartRadius (GReal inRadiusX, GReal inRadiusY = (GReal) -1.0) 
	{ 
		fStartRadiusX = inRadiusX; 
		if (inRadiusY < 0)
			fStartRadiusY = inRadiusX;
		else
			fStartRadiusY = inRadiusY;
	};
	GReal	GetStartRadius () const { return fStartRadiusX; };
	GReal	GetStartRadiusX () const { return fStartRadiusX; };
	GReal	GetStartRadiusY () const { return fStartRadiusY; };
	
	void	SetEndRadius (GReal inRadiusX, GReal inRadiusY = (GReal) -1.0) 
	{ 
		fEndRadiusX = inRadiusX; 
		if (inRadiusY < 0)
			fEndRadiusY = inRadiusX;
		else
			fEndRadiusY = inRadiusY;
	};
	GReal	GetEndRadius () const { return fEndRadiusX; };
	GReal	GetEndRadiusX () const { return fEndRadiusX; };
	GReal	GetEndRadiusY () const { return fEndRadiusY; };

	/** accessors on precomputed radius, start and end point used for tiling modes */
	GReal	GetEndRadiusTiling () const { return fEndRadiusTiling; };
	const VPoint&	GetStartPointTiling () const { return fStartPointTiling; };
	const VPoint&	GetEndPointTiling () const { return fEndPointTiling; };
	
	// Cloning
	virtual VPattern*	Clone () const { return new VRadialGradientPattern(*this); };
	VRadialGradientPattern& operator = (const VRadialGradientPattern& inOriginal);
	virtual OsType	GetKind () const { return Pattern_Kind; };

	// Inherited from IStreamable
	virtual VError	ReadFromStream (VStream* inStream, sLONG inParam = 0);
	virtual VError	WriteToStream (VStream* inStream, sLONG inParam = 0) const;
	
protected:
	GReal	fStartRadiusX;	// Radius X for start point
	GReal	fEndRadiusX;	// Radius X for end point
	GReal	fStartRadiusY;	// Radius Y for start point (for ellipsoid radial gradients)
	GReal	fEndRadiusY;	// Radius Y for end point (for ellipsoid radial gradients)

	GReal	fEndRadiusTiling;	//radius used for reflect/repeat wrapping modes (stored in circular coordinate space centered on end point)
	VPoint	fStartPointTiling;	//start point used for reflect/repeat wrapping modes (stored in circular coordinate space centered on end point)
	VPoint  fEndPointTiling;    //end point used for reflect/repeat wrapping modes (origin in user space for circular coordinate space)
	
	// pre-computed factors to speed up DoComputeValues
#if VERSIONMAC
	GReal	fSpreadFactor[4];
	GReal	fStartComponent[4];
	VAffineTransform fMat;  // local transformation matrix (used to remap input coordinates if the gradient is ellipsoid)
#endif
	
	void	_ComputeFactors();
	
	// Gradient computing
	virtual void	DoComputeValues (const GReal* inValue, GReal* outValue);
	
	// Inherited from VPattern
	virtual void	DoApplyPattern (ContextRef inContext, FillRuleType inFillRule = FILLRULE_NONZERO) const;

private:
	void	_Init ();
};


END_TOOLBOX_NAMESPACE

#endif
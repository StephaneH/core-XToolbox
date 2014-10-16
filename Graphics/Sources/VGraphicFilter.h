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
#ifndef __VGraphicFilter__
#define __VGraphicFilter__

#include "Graphics/Sources/VGraphicImage.h"


BEGIN_TOOLBOX_NAMESPACE

/* 
///////////////////////////
//
//filter usage guidelines:
//
///////////////////////////
 
//first create the filters (one VGraphicFilter instance by filter)

//blur filter example: 
VGraphicFilter filterBlur( eGraphicFilterType_Blur);

//set filter attributes (see VGraphicFilterBagKeys for detail)...
VValueBag filterAttributes;

//...first set primary source
//(use eGFIT_SOURCE_GRAPHIC for the base image source,
//     eGFIT_SOURCE_ALPHA for the alpha only image source (image source with RGB set to 0),
//     eGFIT_LAST_RESULT to use the last filter result,
//	   or specify directly a filter result index from eGFIT_RESULT_BASE+0 to eGFIT_RESULT_BASE+filter sequence size-1
//	   (you can use utility method VGraphicFilterProcess::GetFilterOutputImageIndex(const VString& inNameFilter)
//		to get result image index of previously added filters)
//     (ex: eGFIT_RESULT_BASE+2 to use the result image of the 3nd filter))
filterAttributes.SetLong( VGraphicFilterBagKeys::in, eGFIT_SOURCE_GRAPHIC); 
//...and eventually secondary image source
//filterAttributes.SetLong( VGraphicFilterBagKeys::in2, eGFIT_LAST_RESULT);

//...and specific filter attributes (see VGraphicFilterBagKeys for detail)
filterAttributes.SetReal( VGraphicFilterBagKeys::blurRadius, 3);

//...apply new attributes
filterBlur.SetAttributes(filterAttributes);

//then create the filter process instance...
VGraphicFilterProcess filterProcess;

//...add filters in the order you want to apply them
filterProcess.Add( filterBlur); //here we have only one filter

//...then you have three methods to use the filter sequence:

//the first one is to use directly the Apply method of VGraphicFilterProcess to draw in a VGraphicContext instance:
//(here inImageSource is a VGraphicImage instance: you can instantiate a VGraphicImage object 
// from a VBitmapData, a Gdiplus::Bitmap (Windows OS) or a CGImageRef or a CIImageRef (Mac OS)) 
filterProcess.Apply( inGC,	
					 inImageSource, 
					 VRect(0,0,0,0),
					 NULL); 

//or you can create a new VGraphicImage which is the result of applying the filter sequence on the source image
VGraphicImage *outImageDest = NULL;
filterProcess.Apply (NULL,
					 inImageSource,
					 VRect(0,0,0,0),
					 &outImageDest);

//or finally you can use VGraphicContext::BeginLayerFilter and VGraphicContext::EndLayer methods
//if you want to apply the filter on a layer

gc->BeginLayerFilter( inLayerBounds, filterProcess);

//... draw what you want

gc->EndLayer(); //here the filter will be applied on the layer clipped by inLayerBounds and build from the graphic primitives added from BeginLayer

*/

//graphic filter type enumerates
//@remark: a filter can have up to 2 input images (in1 and in2) and has always one output image (out)
typedef enum eGraphicFilterType 
{
    eGraphicFilterType_Default				= 0,		//default filter: do nothing

	eGraphicFilterType_Blend				,			//out = in1 + in2 (with alpha compositing)
	eGraphicFilterType_Composite			,			//out = in1 + in2 (with color compositing)
	eGraphicFilterType_Blur					,			//out = in1 clone with gaussian blur effect
	eGraphicFilterType_ColorMatrix			,			//out = in1 clone * color matrix
	eGraphicFilterType_Offset				,			//out = in1 clone translated by offset
 
	eGraphicFilterType_Copy					,			//out = in1 clone

	eGraphicFilterType_Count			

} eGraphicFilterType;


//graphic image table 
typedef std::vector<VGraphicImageRef>	VGraphicImageVector;

class VGraphicFilterImpl;
class VGraphicFilterProcess;

//graphic filter
class XTOOLBOX_API VGraphicFilter : public VObject, public IStreamable
{
public:
	friend class VGraphicFilterProcess;

	//lifetime methods

			//build default filter: do nothing
			VGraphicFilter ();

			//build filter from filter type
			VGraphicFilter (eGraphicFilterType inFilterType);

			//copy constructor
			VGraphicFilter (const VGraphicFilter &inFilter);

	virtual	~VGraphicFilter ();

	// Operators
	VGraphicFilter& operator = (const VGraphicFilter &inVGraphicFilter);
	
	//filter type accessor
	eGraphicFilterType GetType() const 
	{
		return fType;
	}

	//filter custom name accessors
	void SetName( const VString& inName)
	{
		fName = inName;
	}
	const VString& GetName() const
	{
		return fName;
	}

	//set attributes
	//@remark: see VGraphicFilterImplxxx for attribute filter details
	void SetAttributes(const VValueBag& inValue);
	const VValueBag& Attributes() const
	{
		return fAttributes;
	}

	//apply filter 
	//
	//@param ioImageVector
	//	filter image list
	//@param inFilterIndex
	//  filter index 
	//@param inBounds
	//	filterprocess destination bounds (can be not equal to image bounds: used to resolve relative coordinates)
	//@param bIsRelative
	//	if true filter internal coordinates if any are relative to inBounds (for instance like offset for offset filter)
	//@param inScaleUserToDeviceUnits
	//  if filter image space scaling is equal to device space scaling (for instance for layers to avoid artifacts)
	//  in order to transform from user space to device space filter-related coordinates (like for instance offset filter offset)
	//  this parameter need to be set to user space to device space scaling factor */
	//
	//@remark: see VGraphicFilterImplxxx for attribute filter details
	//
	void Apply( VGraphicImageVector& ioImageVector, sLONG inFilterIndex, const VRect& inBounds, bool bIsRelative = false, VPoint inScaleUserToDeviceUnits = VPoint(1.0f,1.0f));

	// Stream storage
	VError	ReadFromStream (VStream* inStream, sLONG inParam = 0);
	VError	WriteToStream (VStream* inStream, sLONG inParam = 0) const;

protected:
	//filter type
	eGraphicFilterType fType;

	//filter name
	VString fName;

	//filter attributes
	VValueBag fAttributes;

	//filter implementation
	VGraphicFilterImpl *fImpl; 

protected:
	//create VGraphicFilterImplxxx from current filter name
	void _CreateImpl();

	//synchronize attributes 
	void _SyncAttributes();
};



//graphic filter process
class XTOOLBOX_API VGraphicFilterProcess : public VObject, public IStreamable, public IRefCountable
{
public:
	// lifetime methods
			VGraphicFilterProcess ();
	virtual	~VGraphicFilterProcess ();

	//relative coordinates status accessors
	void SetCoordsRelative( bool bIsRelative)
	{
		fCoordsRelative = bIsRelative;
	}
	bool GetCoordsRelative()
	{
		return fCoordsRelative;
	}


	//add new filter
	void Add(const VGraphicFilter& inFilter);

	//clear all filters
	void Clear();

	/** read accessor on inflating offset 
	@remarks
		filter user must use this offset to inflate filter bounds 
		for instance while using layers to ensure filters like gaussian blur will work well
		(otherwise blur filter can be cropped)
	*/
	GReal GetInflatingOffset() const 
	{
		return fInflatingOffset;
	}

	GReal GetBlurMaxRadius() const
	{
		return fBlurMaxRadius;
	}

	bool IsEmpty()
	{
		return fSeq.empty();
	}

	/** return number of filters */
	VSize GetNumFilter() const
	{
		return (VSize)fSeq.size();
	}

	/** const accessor on filter 
	@remarks 
		index range is [0..GetNumFilter()-1]
	*/
	const VGraphicFilter* GetFilter( VIndex inIndex) const
	{
		if (inIndex >= 0 && inIndex < fSeq.size())
			return &(fSeq[inIndex]);
		else
			return NULL;
	}

	/** return index of the filter output image 
	@remarks
		can be used as index for input image for any following filter
	*/
	VIndex GetFilterOutputImageIndex(const VString& inNameFilter) const;

	/** apply current filter sequence on input image source 
	//  draw the result in the specified graphic context if inGC is not NULL
	//  return retained result image in *outResult if outResult is not NULL
	//
	//@param inGC 
	//	graphic context in which to draw the filter result (can be NULL)
	//@param inImageSource
	//	filter image source
	//@param inBoundsDst					
	//	destination bounds
	//@param inBoundsSrc					
	//	source bounds
	//@param outResult				
	//	if not null, will contain the retained filter result image
	//@param inScaleUserToDeviceUnits
	//  if filter image space scaling is equal to device space scaling (for instance for layers to avoid artifacts)
	//  in order to transform from user space to device space filter-related coordinates (like for instance offset filter offset)
	//  this parameter need to be set to user space to device space scaling factor */
	void Apply( VGraphicContext *inGC, VGraphicImageRef inImageSource, const VRect& inBoundsDst, const VRect& inBoundsSrc, VGraphicImageRef *outResult = NULL, eGraphicImageBlendType inBlendType = eGraphicImageBlendType_Over, VPoint inScaleUserToDeviceUnits = VPoint(1.0f,1.0f));
	
	/** apply current filter sequence on input image source 
	//  draw the result in the specified graphic context if inGC is not NULL
	//  return retained result image in *outResult if outResult is not NULL
	//
	//@param inGC 
	//	graphic context in which to draw the filter result (can be NULL)
	//@param inImageSource
	//	filter image source
	//@param inBoundsDst					
	//	destination bounds
	//@param outResult				
	//	if not null, will contain the retained filter result image
	//@param inScaleUserToDeviceUnits
	//  if filter image space scaling is equal to device space scaling (for instance for layers to avoid artifacts)
	//  in order to transform from user space to device space filter-related coordinates (like for instance offset filter offset)
	//  this parameter need to be set to user space to device space scaling factor */
	void Apply( VGraphicContext *inGC, VGraphicImageRef inImageSource, const VRect& inBoundsDst, VGraphicImageRef *outResult = NULL, eGraphicImageBlendType inBlendType = eGraphicImageBlendType_Over, VPoint inScaleUserToDeviceUnits = VPoint(1.0f,1.0f));

	// Stream storage
	virtual VError	ReadFromStream (VStream* inStream, sLONG inParam = 0);
	virtual VError	WriteToStream (VStream* inStream, sLONG inParam = 0) const;

private:
	/** inflating offset 
	@remarks
		filter user must use this offset to inflate filter bounds 
		for instance while using layers to ensure filters like gaussian blur will work well
	*/
	GReal fInflatingOffset;				

	GReal fBlurMaxRadius;

	bool fCoordsRelative;				//true if primitive coordinates are relative

	std::vector<VGraphicFilter> fSeq;	//filter sequence
};



//graphic filter image indexs
typedef enum eGraphicFilterImageType 
{
    eGFIT_SOURCE_GRAPHIC	= 0,			//filter graphic image source (RGB color + Alpha)
    eGFIT_SOURCE_ALPHA		= 1,			//filter graphic alpha image source (Alpha)

	eGFIT_RESULT_BASE		= 2,			//filter graphic image result base index

	eGFIT_LAST_RESULT		= -1			//filter graphic image last result index
} eGraphicFilterImageType;


//default filter blur gaussian convolution kernel radius
#define kFILTER_BLUR_RADIUS_DEFAULT			3

//default filter blur gaussian deviation
#define kFILTER_BLUR_DEVIATION_DEFAULT		((GReal)0.2*3)

//default alpha blur state
#define kFILTER_BLUR_ALPHA_DEFAULT			((Boolean)0)


//color matrix enumeration type definition
typedef enum eGraphicFilterColorMatrix
{
	eGraphicFilterColorMatrix_Identity,				//do nothing
	eGraphicFilterColorMatrix_Saturate,				//saturate colors: set VGraphicFilterBagKeys::colorMatrixSaturate attribute to [0.0,1.0] range
	eGraphicFilterColorMatrix_HueRotate,			//rotate colors: set VGraphicFilterBagKeys::colorMatrixHueRotateAngle to de
	eGraphicFilterColorMatrix_Grayscale,			//make grayscale image
	eGraphicFilterColorMatrix_LuminanceToAlpha,		//make (000A) image where A (alpha channel) is equal to source image luminance
	eGraphicFilterColorMatrix_AlphaMask,			//make (000A) image where A (alpha channel) is equal to source image alpha
	eGraphicFilterColorMatrix_Matrix				//apply 5x5 color matrix : set VGraphicFilterBagKeys::colorMatrixM?? attribute)

} eGraphicFilterColorMatrix;


//graphic filter composite type definition
typedef enum eGraphicFilterCompositeType
{
	eGraphicFilterCompositeType_Over,
	eGraphicFilterCompositeType_In,					//TODO: not implemented yet on Windows OS
	eGraphicFilterCompositeType_Out,				//TODO: not implemented yet on Windows OS
	eGraphicFilterCompositeType_Atop,				//TODO: not implemented yet on Windows OS
	eGraphicFilterCompositeType_Xor,				//TODO: not implemented yet 
	eGraphicFilterCompositeType_Arithmetic			//TODO: not implemented yet
} eGraphicFilterCompositeType;


//filter implementation attributes
namespace VGraphicFilterBagKeys
{
	//general properties: filter input
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(in, XBOX::VLong, sLONG, eGFIT_LAST_RESULT);		//primary input source
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(in2, XBOX::VLong, sLONG, eGFIT_LAST_RESULT);		//secondary input source

	//blur filter properties
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(blurRadius, XBOX::VReal, GReal, kFILTER_BLUR_RADIUS_DEFAULT);			//gaussian blur radius
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(blurDeviation, XBOX::VReal, GReal, kFILTER_BLUR_DEVIATION_DEFAULT);	//gaussian blur deviation
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(blurAlpha, XBOX::VBoolean, Boolean, kFILTER_BLUR_ALPHA_DEFAULT);		//if set to 1, gaussian blur is applied on alpha channel only

	//color matrix filter properties
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(colorMatrixType, XBOX::VLong, sLONG, eGraphicFilterColorMatrix_Matrix);	//color matrix type

	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(colorMatrixSaturate, XBOX::VReal, GReal, 1);	//saturation value (range is [0,1]) (only for eGraphicFilterColorMatrix_Saturate)
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(colorMatrixHueRotateAngle, XBOX::VReal, GReal, 0);	//angle in degrees (only for eGraphicFilterColorMatrix_HueRotate)

	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(colorMatrixM00, XBOX::VReal, GReal, 1);		//matrix values (only for eGraphicFilterColorMatrix_Matrix)
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(colorMatrixM01, XBOX::VReal, GReal, 0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(colorMatrixM02, XBOX::VReal, GReal, 0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(colorMatrixM03, XBOX::VReal, GReal, 0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(colorMatrixM04, XBOX::VReal, GReal, 0);

	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(colorMatrixM10, XBOX::VReal, GReal, 0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(colorMatrixM11, XBOX::VReal, GReal, 1);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(colorMatrixM12, XBOX::VReal, GReal, 0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(colorMatrixM13, XBOX::VReal, GReal, 0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(colorMatrixM14, XBOX::VReal, GReal, 0);

	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(colorMatrixM20, XBOX::VReal, GReal, 0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(colorMatrixM21, XBOX::VReal, GReal, 0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(colorMatrixM22, XBOX::VReal, GReal, 1);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(colorMatrixM23, XBOX::VReal, GReal, 0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(colorMatrixM24, XBOX::VReal, GReal, 0);

	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(colorMatrixM30, XBOX::VReal, GReal, 0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(colorMatrixM31, XBOX::VReal, GReal, 0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(colorMatrixM32, XBOX::VReal, GReal, 0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(colorMatrixM33, XBOX::VReal, GReal, 1);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(colorMatrixM34, XBOX::VReal, GReal, 0);

	//offset filter properties
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(offsetX, XBOX::VReal, GReal, 0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(offsetY, XBOX::VReal, GReal, 0);

	//blend filter properties
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(blendType, XBOX::VLong, sLONG, eGraphicImageBlendType_Over);

	//composite filter properties
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(compositeType, XBOX::VLong, sLONG, eGraphicFilterCompositeType_Over);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(compositeK1, XBOX::VReal, GReal, 0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(compositeK2, XBOX::VReal, GReal, 0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(compositeK3, XBOX::VReal, GReal, 0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(compositeK4, XBOX::VReal, GReal, 0);
};



//abstract graphic filter implementation
class VGraphicFilterImpl : public VObject
{
public:
	friend class VGraphicFilterProcess;

	//lifetime methods
			VGraphicFilterImpl ();
	virtual	~VGraphicFilterImpl ();

	//synchronize filter properties with bag attributes
	virtual void Sync( const VValueBag& inValue);

	//apply filter 
	//@remark: see VGraphicFilterImplxxx for attribute filter details
	virtual void Apply( VGraphicImageVector& ioImageVector, sLONG inFilterIndex, const VRect& inBounds, bool bIsRelative = false, VPoint inScaleUserToDeviceUnits = VPoint(1.0f,1.0f)) = 0;

protected:
	sLONG fIn;	//graphic image table first input index (relative from current filter index if < 0)
	sLONG fIn2;	//graphic image table second input index (relative from current filter index if < 0)

	//runtime temporary variables

	sLONG _fIn;	//graphic image table first input absolute index 
	sLONG _fIn2;//graphic image table second input absolute index 

	//create temporary indexed image 
	//@remark: do nothing if the indexed image is created yet 
	void _CreateImage( VGraphicImageVector& ioImageVector, sLONG inFilterIndex = 0);

	//convert relative image indexs to absolute image indexs
	void _Normalize( VGraphicImageVector& ioImageVector, sLONG inFilterIndex);

	/** inflating offset 
	@see 
		VGraphicFilterProcess::GetInflatingOffset()
	*/
	GReal fInflatingOffset;				
};


//default graphic filter : result = source1
class VGraphicFilterDefault : public VGraphicFilterImpl
{
public:
	//lifetime methods
			VGraphicFilterDefault ();
	virtual	~VGraphicFilterDefault ();

	//apply filter 
	//@remark: see VGraphicFilterImplxxx for attribute filter details
	void Apply( VGraphicImageVector& ioImageVector, sLONG inFilterIndex, const VRect& inBounds, bool bIsRelative = false, VPoint inScaleUserToDeviceUnits = VPoint(1.0f,1.0f));
};



//copy graphic filter : out = in1 copy
class VGraphicFilterCopy : public VGraphicFilterImpl
{
public:
	//lifetime methods
			VGraphicFilterCopy ();
	virtual	~VGraphicFilterCopy ();
	
	//apply filter 
	void Apply( VGraphicImageVector& ioImageVector, sLONG inFilterIndex, const VRect& inBounds, bool bIsRelative = false, VPoint inScaleUserToDeviceUnits = VPoint(1.0f,1.0f));
};




//blur graphic filter : out = in1 modified with gaussian blur effect
class VGraphicFilterBlur : public VGraphicFilterImpl
{
public:
	//lifetime methods
			VGraphicFilterBlur ();
	virtual	~VGraphicFilterBlur ();
	
	//synchronize filter properties with bag attributes
	void Sync( const VValueBag& inValue);

	//apply filter 
	void Apply( VGraphicImageVector& ioImageVector, sLONG inFilterIndex, const VRect& inBounds, bool bIsRelative = false, VPoint inScaleUserToDeviceUnits = VPoint(1,1));

private:
	GReal fRadius;		//gaussian convolution kernel radius
	GReal fDeviation;	//gaussian deviation
	bool fAlphaOnly;	//blur alpha channel only 
};




//color matrix graphic filter : out = in1 * color matrix
class VGraphicFilterColorMatrix : public VGraphicFilterImpl
{
public:
	//lifetime methods
			VGraphicFilterColorMatrix ();
	virtual	~VGraphicFilterColorMatrix ();
	
	//synchronize filter properties with bag attributes
	void Sync( const VValueBag& inValue);

	//apply filter 
	void Apply( VGraphicImageVector& ioImageVector, sLONG inFilterIndex, const VRect& inBounds, bool bIsRelative = false, VPoint inScaleUserToDeviceUnits = VPoint(1.0f,1.0f));

private:
	//color matrix
	VColorMatrix fMatrix;
};


//offset graphic filter : out = in1 clone translated by offset
class VGraphicFilterOffset : public VGraphicFilterImpl
{
public:
	//lifetime methods
			VGraphicFilterOffset ();
	virtual	~VGraphicFilterOffset ();
	
	//synchronize filter properties with bag attributes
	void Sync( const VValueBag& inValue);

	//apply filter 
	void Apply( VGraphicImageVector& ioImageVector, sLONG inFilterIndex, const VRect& inBounds, bool bIsRelative = false, VPoint inScaleUserToDeviceUnits = VPoint(1.0f,1.0f));

private:
	//offset vector
	VPoint fOffset;
};


//blend graphic filter : out = in1 clone with in2 blended with one alpha-blending method
class VGraphicFilterBlend : public VGraphicFilterImpl
{
public:
	//lifetime methods
			VGraphicFilterBlend ();
	virtual	~VGraphicFilterBlend ();
	
	//synchronize filter properties with bag attributes
	void Sync( const VValueBag& inValue);

	//apply filter 
	void Apply( VGraphicImageVector& ioImageVector, sLONG inFilterIndex, const VRect& inBounds, bool bIsRelative = false, VPoint inScaleUserToDeviceUnits = VPoint(1.0f,1.0f));

private:
	//blend type enumeration
	eGraphicImageBlendType fBlendType;
};


//composite graphic filter : out = in1 clone with in2 composited 
class VGraphicFilterComposite : public VGraphicFilterImpl
{
public:
	//lifetime methods
			VGraphicFilterComposite ();
	virtual	~VGraphicFilterComposite ();
	
	//synchronize filter properties with bag attributes
	void Sync( const VValueBag& inValue);

	//apply filter 
	void Apply( VGraphicImageVector& ioImageVector, sLONG inFilterIndex, const VRect& inBounds, bool bIsRelative = false, VPoint inScaleUserToDeviceUnits = VPoint(1.0f,1.0f));

private:
	//composite type enumeration
	eGraphicFilterCompositeType fCompositeType;

	//arithmetic coefficients
	GReal fK1, fK2, fK3, fK4;
};



END_TOOLBOX_NAMESPACE

#endif
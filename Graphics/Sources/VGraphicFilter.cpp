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
#include "VGraphicFilter.h"
#undef max
#undef min

using namespace std;

///////////////////////////
//
// implementation methods
//
///////////////////////////


//lifetime methods

VGraphicFilter::VGraphicFilter():VObject()
{
	fType = eGraphicFilterType_Default;
	fImpl = NULL;
}

VGraphicFilter::VGraphicFilter (eGraphicFilterType inFilterType):VObject()
{
	fType = inFilterType;
	fImpl = NULL;

	_CreateImpl();
	_SyncAttributes();
}

VGraphicFilter::VGraphicFilter(const VGraphicFilter& inOriginal):VObject()
{
	if (this == &inOriginal)
		return;

	fType = eGraphicFilterType_Default;
	fImpl = NULL;

	*this = inOriginal;
}

VGraphicFilter::~VGraphicFilter()
{
	if (fImpl)
		delete fImpl;
	fImpl = NULL;
}


VGraphicFilter& VGraphicFilter::operator = (const VGraphicFilter &inFilter)
{
	if (this == &inFilter)
		return *this;

	if (fType == inFilter.fType && fImpl != NULL)
	{
		fName		= inFilter.fName;
		fAttributes = inFilter.fAttributes;
		_SyncAttributes();
	}	
	else
	{
		fType		= inFilter.fType;
		fName		= inFilter.fName;
		fAttributes = inFilter.fAttributes;
	
		_CreateImpl();
		_SyncAttributes();
	}
	return *this;
}


//create VGraphicFilterImplxxx from current filter name
void  VGraphicFilter::_CreateImpl()
{
	if (fImpl)
		delete fImpl;
	fImpl = NULL;
	
	switch (fType)
	{
	case eGraphicFilterType_Default:
		fImpl = static_cast<VGraphicFilterImpl *>(new VGraphicFilterDefault());
		break;

	case eGraphicFilterType_Blur:
		fImpl = static_cast<VGraphicFilterImpl *>(new VGraphicFilterBlur());
		break;
										
	case eGraphicFilterType_ColorMatrix:
		fImpl = static_cast<VGraphicFilterImpl *>(new VGraphicFilterColorMatrix());
		break;
										
	case eGraphicFilterType_Blend:
		fImpl = static_cast<VGraphicFilterImpl *>(new VGraphicFilterBlend());
		break;
										
	case eGraphicFilterType_Copy:
		fImpl = static_cast<VGraphicFilterImpl *>(new VGraphicFilterCopy());
		break;

	case eGraphicFilterType_Offset:
		fImpl = static_cast<VGraphicFilterImpl *>(new VGraphicFilterOffset());
		break;
										
	case eGraphicFilterType_Composite:
		fImpl = static_cast<VGraphicFilterImpl *>(new VGraphicFilterComposite());
		break;

	default:
		fImpl = static_cast<VGraphicFilterImpl *>(new VGraphicFilterDefault());
		break;
	}
	xbox_assert(fImpl != NULL);
}


//reset VGraphicFilterImplxxx
void VGraphicFilter::_SyncAttributes()
{
	xbox_assert(fImpl != NULL);
	fImpl->Sync( fAttributes);
}


//apply filter 
//@remark: see VGraphicFilterImplxxx for attribute filter details
void VGraphicFilter::Apply( VGraphicImageVector& ioImageVector, sLONG inFilterIndex, const VRect& inBounds, bool bIsRelative, VPoint inScaleUserToDeviceUnits)
{
	xbox_assert(fImpl != NULL);
	fImpl->Apply(ioImageVector, inFilterIndex, inBounds, bIsRelative, inScaleUserToDeviceUnits);
}


//IStreamable overriden methods

VError VGraphicFilter::ReadFromStream(VStream* inStream, sLONG /*inParam*/)
{
	if (inStream == NULL) return VE_INVALID_PARAMETER;

	sLONG type = (sLONG)eGraphicFilterType_Default;
	VError error = inStream->GetLong( type);
	if (type >= eGraphicFilterType_Count || type < 0)
		type = (sLONG)eGraphicFilterType_Default;
	fType = (eGraphicFilterType)type;

	fAttributes.Destroy();

	if (error == VE_OK)
	{
		error = fName.ReadFromStream( inStream);
		if (error == VE_OK)
			error = fAttributes.ReadFromStream( inStream);
	}

	_CreateImpl();
	_SyncAttributes();

	return inStream->GetLastError();
}


VError VGraphicFilter::WriteToStream(VStream* inStream, sLONG /*inParam*/) const
{
	if (inStream == NULL) return VE_INVALID_PARAMETER;

	VError error = inStream->PutLong( (sLONG)fType);
	if (error == VE_OK)
	{
		error = fName.WriteToStream( inStream);
		if (error == VE_OK)
			error = fAttributes.WriteToStream( inStream);
	}
	
	return inStream->GetLastError();
}


//set attributes
//@remark: see VGraphicFilterImplxxx for attribute filter details
void VGraphicFilter::SetAttributes(const VValueBag& inValue)
{
	fAttributes = inValue;

	_SyncAttributes();
}

////////////////////////////////////////////////////
//
// abstract graphic filter implementation
//
////////////////////////////////////////////////////

VGraphicFilterImpl::VGraphicFilterImpl ():VObject()
{
	fIn		= eGFIT_LAST_RESULT;
	fIn2	= eGFIT_LAST_RESULT;
	fInflatingOffset = 0.0f;
}

VGraphicFilterImpl::~VGraphicFilterImpl()
{
}


//synchronize filter properties with bag attributes
void VGraphicFilterImpl::Sync( const VValueBag& inBag)
{
	fIn		= VGraphicFilterBagKeys::in.Get(&inBag);
	fIn2	= VGraphicFilterBagKeys::in2.Get(&inBag);
}


//create temporary indexed image 
//@remark: do nothing if the indexed image is created yet 
void VGraphicFilterImpl::_CreateImage( VGraphicImageVector& ioImageVector, sLONG inFilterIndex)
{
	xbox_assert(inFilterIndex > (sLONG)eGFIT_SOURCE_GRAPHIC && inFilterIndex < (sLONG)ioImageVector.size());
	xbox_assert(ioImageVector[eGFIT_SOURCE_GRAPHIC] != NULL);
	if (ioImageVector[ inFilterIndex] == NULL)
	{
		//create new image with same size and pixel format as the primary image source
		ioImageVector[ inFilterIndex] = new VGraphicImage(	ioImageVector[eGFIT_SOURCE_GRAPHIC]->GetWidth(),
															ioImageVector[eGFIT_SOURCE_GRAPHIC]->GetHeight(),
															ioImageVector[eGFIT_SOURCE_GRAPHIC]->GetPixelFormat());
	}
}

//convert relative image indexs to absolute image indexs
void VGraphicFilterImpl::_Normalize( VGraphicImageVector& ioImageVector, sLONG inFilterIndex)
{
	if (fIn < 0)
	{
		_fIn = eGFIT_RESULT_BASE+inFilterIndex+fIn;
		if (_fIn < eGFIT_RESULT_BASE)
			_fIn = eGFIT_SOURCE_GRAPHIC;
	}
	else
		_fIn = fIn;

	if (fIn2 < 0)
	{
		_fIn2 = eGFIT_RESULT_BASE+inFilterIndex+fIn2;
		if (_fIn2 < eGFIT_RESULT_BASE)
			_fIn2 = eGFIT_SOURCE_GRAPHIC;
	}
	else
		_fIn2 = fIn2;

	xbox_assert(_fIn >= 0 && _fIn < (sLONG)ioImageVector.size());

	sLONG result = eGFIT_RESULT_BASE+inFilterIndex;
	xbox_assert(result >= 0 && result < (sLONG)ioImageVector.size());
	xbox_assert(_fIn < result);
	xbox_assert(_fIn2 >= 0 && _fIn2 < (sLONG)ioImageVector.size());
	xbox_assert(_fIn2 < result);

	sLONG in = _fIn;
	for (int iSource = 1; iSource <= 2; iSource++) 
	{
		if ((in == eGFIT_SOURCE_ALPHA) && (ioImageVector[in] == NULL))
		{
			//generate graphic source alpha
			VGraphicFilter filter( eGraphicFilterType_ColorMatrix);
			VValueBag bagAttributes;
			bagAttributes.SetLong( VGraphicFilterBagKeys::in, eGFIT_SOURCE_GRAPHIC);
			bagAttributes.SetLong( VGraphicFilterBagKeys::colorMatrixType, eGraphicFilterColorMatrix_AlphaMask);
			filter.SetAttributes(bagAttributes);

			VGraphicFilterProcess filterProcess;
			filterProcess.Add( filter);
			VGraphicImageRef imageSourceAlpha = NULL;
			filterProcess.Apply( NULL,	
								 ioImageVector[eGFIT_SOURCE_GRAPHIC], 
								 VRect(0, 0, 0, 0),
								 &imageSourceAlpha); 

			ioImageVector[eGFIT_SOURCE_ALPHA] = imageSourceAlpha;
		}
		if (iSource == 1)
			in = _fIn2;
	}
}



////////////////////////////////////////////////////
//
// default graphic filter : do nothing
//
////////////////////////////////////////////////////

VGraphicFilterDefault::VGraphicFilterDefault ():VGraphicFilterImpl()
{
}

VGraphicFilterDefault::~VGraphicFilterDefault()
{
}


//apply filter 
//@remark: see VGraphicFilterImplxxx for attribute filter details
void VGraphicFilterDefault::Apply( VGraphicImageVector& ioImageVector, sLONG inFilterIndex, const VRect& inBounds, bool bIsRelative, VPoint inScaleUserToDeviceUnits)
{
	_Normalize( ioImageVector, inFilterIndex);

	//get source image index
	sLONG result = eGFIT_RESULT_BASE+inFilterIndex;
	xbox_assert(result < (sLONG)ioImageVector.size());

	//set result image as equal to source image
	CopyRefCountable(&ioImageVector[ result], ioImageVector[_fIn]);
}



////////////////////////////////////////////////////
//
// copy graphic filter : out = in1 clone
//
////////////////////////////////////////////////////

VGraphicFilterCopy::VGraphicFilterCopy ():VGraphicFilterImpl()
{
}

VGraphicFilterCopy::~VGraphicFilterCopy()
{
}


//apply filter 
void VGraphicFilterCopy::Apply( VGraphicImageVector& ioImageVector, sLONG inFilterIndex, const VRect& inBounds, bool bIsRelative, VPoint inScaleUserToDeviceUnits)
{
	_Normalize( ioImageVector, inFilterIndex);

	//get source image index
	sLONG result = eGFIT_RESULT_BASE+inFilterIndex;
	xbox_assert(result < (sLONG)ioImageVector.size());

	//set result image as source image clone
	xbox_assert(ioImageVector[result] == NULL);
	ioImageVector[ result] = ioImageVector[_fIn]->Clone();
}







////////////////////////////////////////////////////////////
//
// blur graphic filter : result = in1 clone with blur filter
//
////////////////////////////////////////////////////////////

VGraphicFilterBlur::VGraphicFilterBlur ():VGraphicFilterImpl()
{
	fRadius = kFILTER_BLUR_RADIUS_DEFAULT;
	fDeviation = kFILTER_BLUR_DEVIATION_DEFAULT;
	fAlphaOnly = kFILTER_BLUR_ALPHA_DEFAULT;
	fInflatingOffset = fRadius;
}

VGraphicFilterBlur::~VGraphicFilterBlur()
{
}


//synchronize filter properties with bag attributes
void VGraphicFilterBlur::Sync( const VValueBag& inBag)
{
	VGraphicFilterImpl::Sync( inBag);

	//ensure consistency between deviation & blur radius (blur radius is kernel radius)
	//(radius = 5 * deviation which is consistent with gaussian blur)
	if (inBag.AttributeExists( VGraphicFilterBagKeys::blurDeviation))
		fDeviation	= VGraphicFilterBagKeys::blurDeviation.Get(&inBag);
	else
	{
		fRadius		= ceil(VGraphicFilterBagKeys::blurRadius.Get(&inBag));
		fDeviation  = 0.2f*fRadius;
	}
	fAlphaOnly	= VGraphicFilterBagKeys::blurAlpha.Get(&inBag) != 0;

	if (fDeviation < (GReal) 0.2)
		fDeviation = (GReal) 0.2;
	fRadius = fDeviation * 5;

	if (fRadius > VGraphicContext::sLayerFilterInflatingOffset)
	{
		fRadius = VGraphicContext::sLayerFilterInflatingOffset;
		fDeviation = (GReal) 0.2f*fRadius;
	}
	fInflatingOffset = ceil(fRadius);
}



///////////////////////////////////////////////////////////////
//
// color matrix graphic filter : out = in1 clone * color matrix
//
///////////////////////////////////////////////////////////////

VGraphicFilterColorMatrix::VGraphicFilterColorMatrix ():VGraphicFilterImpl()
{
	fMatrix.m[0][0] = 1;
	fMatrix.m[0][1] = 0;
	fMatrix.m[0][2] = 0;
	fMatrix.m[0][3] = 0;
	fMatrix.m[0][4] = 0;

	fMatrix.m[1][0] = 0;
	fMatrix.m[1][1] = 1;
	fMatrix.m[1][2] = 0;
	fMatrix.m[1][3] = 0;
	fMatrix.m[1][4] = 0;

	fMatrix.m[2][0] = 0;
	fMatrix.m[2][1] = 0;
	fMatrix.m[2][2] = 1;
	fMatrix.m[2][3] = 0;
	fMatrix.m[2][4] = 0;

	fMatrix.m[3][0] = 0;
	fMatrix.m[3][1] = 0;
	fMatrix.m[3][2] = 0;
	fMatrix.m[3][3] = 1;
	fMatrix.m[3][4] = 0;

	fMatrix.m[4][0] = 0;
	fMatrix.m[4][1] = 0;
	fMatrix.m[4][2] = 0;
	fMatrix.m[4][3] = 0;
	fMatrix.m[4][4] = 1;
}

VGraphicFilterColorMatrix::~VGraphicFilterColorMatrix()
{
}


//synchronize filter properties with bag attributes
void VGraphicFilterColorMatrix::Sync( const VValueBag& inBag)
{
	VGraphicFilterImpl::Sync( inBag);

	eGraphicFilterColorMatrix type	= (eGraphicFilterColorMatrix) VGraphicFilterBagKeys::colorMatrixType.Get(&inBag);
	switch( type)
	{
	case eGraphicFilterColorMatrix_Saturate:
		{
		//build saturation color matrix

		GReal s = VGraphicFilterBagKeys::colorMatrixSaturate.Get(&inBag);
	
		fMatrix.m[0][0] = (GReal) 0.213+(GReal) 0.787*s;
		fMatrix.m[0][1] = (GReal) 0.715-(GReal) 0.715*s;
		fMatrix.m[0][2] = (GReal) 0.072-(GReal) 0.072*s;
		fMatrix.m[0][3] = 0;
		fMatrix.m[0][4] = 0;

		fMatrix.m[1][0] = (GReal) 0.213-(GReal) 0.213*s;
		fMatrix.m[1][1] = (GReal) 0.715+(GReal) 0.285*s;
		fMatrix.m[1][2] = (GReal) 0.072-(GReal) 0.072*s;
		fMatrix.m[1][3] = 0;
		fMatrix.m[1][4] = 0;

		fMatrix.m[2][0] = (GReal) 0.213-(GReal) 0.213*s;
		fMatrix.m[2][1] = (GReal) 0.715-(GReal) 0.715*s;
		fMatrix.m[2][2] = (GReal) 0.072+(GReal) 0.928*s;
		fMatrix.m[2][3] = 0;
		fMatrix.m[2][4] = 0;

		fMatrix.m[3][0] = 0;
		fMatrix.m[3][1] = 0;
		fMatrix.m[3][2] = 0;
		fMatrix.m[3][3] = 1;
		fMatrix.m[3][4] = 0;
		}
		break;
	case eGraphicFilterColorMatrix_HueRotate:
		{
		//build hue rotate color matrix
		
		GReal angle		= VGraphicFilterBagKeys::colorMatrixHueRotateAngle.Get(&inBag)* (GReal)XBOX::PI / 180;
		GReal sinus		= sin( angle);
		GReal cosinus	= cos( angle);
		
		fMatrix.m[0][0] = (GReal) 0.213+cosinus*(GReal) 0.787+sinus*(GReal) -0.213;
		fMatrix.m[0][1] = (GReal) 0.715+cosinus*(GReal) -0.715+sinus*(GReal) -0.715;
		fMatrix.m[0][2] = (GReal) 0.072+cosinus*(GReal) -0.072+sinus*(GReal) 0.928;
		fMatrix.m[0][3] = 0;
		fMatrix.m[0][4] = 0;

		fMatrix.m[1][0] = (GReal) 0.213+cosinus*(GReal)-0.213+sinus*(GReal) +0.143;
		fMatrix.m[1][1] = (GReal) 0.715+cosinus*(GReal)0.285+sinus*(GReal) 0.140;
		fMatrix.m[1][2] = (GReal) 0.072+cosinus*(GReal)-0.072+sinus*(GReal) -0.283;
		fMatrix.m[1][3] = 0;
		fMatrix.m[1][4] = 0;

		fMatrix.m[2][0] = (GReal) 0.213+cosinus*(GReal) -0.213+sinus*(GReal) -0.787;
		fMatrix.m[2][1] = (GReal) 0.715+cosinus*(GReal) -0.715+sinus*(GReal) 0.715;
		fMatrix.m[2][2] = (GReal) 0.072+cosinus*(GReal) 0.928+sinus*(GReal) +0.072;
		fMatrix.m[2][3] = 0;
		fMatrix.m[2][4] = 0;

		fMatrix.m[3][0] = 0;
		fMatrix.m[3][1] = 0;
		fMatrix.m[3][2] = 0;
		fMatrix.m[3][3] = 1;
		fMatrix.m[3][4] = 0;
		}
		break;
	case eGraphicFilterColorMatrix_LuminanceToAlpha:
		{
		//build luminance to alpha color matrix
		
		fMatrix.m[0][0] = 0;
		fMatrix.m[0][1] = 0;
		fMatrix.m[0][2] = 0;
		fMatrix.m[0][3] = 0;
		fMatrix.m[0][4] = 0;

		fMatrix.m[1][0] = 0;
		fMatrix.m[1][1] = 0;
		fMatrix.m[1][2] = 0;
		fMatrix.m[1][3] = 0;
		fMatrix.m[1][4] = 0;

		fMatrix.m[2][0] = 0;
		fMatrix.m[2][1] = 0;
		fMatrix.m[2][2] = 0;
		fMatrix.m[2][3] = 0;
		fMatrix.m[2][4] = 0;

		fMatrix.m[3][0] = (GReal) 0.2125;
		fMatrix.m[3][1] = (GReal) 0.7154;
		fMatrix.m[3][2] = (GReal) 0.0721;
		fMatrix.m[3][3] = 0;
		fMatrix.m[3][4] = 0;
		}
		break;
	case eGraphicFilterColorMatrix_AlphaMask:
		{
		//build black image while preserving alpha channel
		
		fMatrix.m[0][0] = 0;
		fMatrix.m[0][1] = 0;
		fMatrix.m[0][2] = 0;
		fMatrix.m[0][3] = 0;
		fMatrix.m[0][4] = 0;

		fMatrix.m[1][0] = 0;
		fMatrix.m[1][1] = 0;
		fMatrix.m[1][2] = 0;
		fMatrix.m[1][3] = 0;
		fMatrix.m[1][4] = 0;

		fMatrix.m[2][0] = 0;
		fMatrix.m[2][1] = 0;
		fMatrix.m[2][2] = 0;
		fMatrix.m[2][3] = 0;
		fMatrix.m[2][4] = 0;

		fMatrix.m[3][0] = 0;
		fMatrix.m[3][1] = 0;
		fMatrix.m[3][2] = 0;
		fMatrix.m[3][3] = 1;
		fMatrix.m[3][4] = 0;
		}
		break;
	case eGraphicFilterColorMatrix_Matrix:
		{
		//build custom color matrix
		
		fMatrix.m[0][0] = VGraphicFilterBagKeys::colorMatrixM00.Get(&inBag);
		fMatrix.m[0][1] = VGraphicFilterBagKeys::colorMatrixM01.Get(&inBag);
		fMatrix.m[0][2] = VGraphicFilterBagKeys::colorMatrixM02.Get(&inBag);
		fMatrix.m[0][3] = VGraphicFilterBagKeys::colorMatrixM03.Get(&inBag);
		fMatrix.m[0][4] = VGraphicFilterBagKeys::colorMatrixM04.Get(&inBag);

		fMatrix.m[1][0] = VGraphicFilterBagKeys::colorMatrixM10.Get(&inBag);
		fMatrix.m[1][1] = VGraphicFilterBagKeys::colorMatrixM11.Get(&inBag);
		fMatrix.m[1][2] = VGraphicFilterBagKeys::colorMatrixM12.Get(&inBag);
		fMatrix.m[1][3] = VGraphicFilterBagKeys::colorMatrixM13.Get(&inBag);
		fMatrix.m[1][4] = VGraphicFilterBagKeys::colorMatrixM14.Get(&inBag);

		fMatrix.m[2][0] = VGraphicFilterBagKeys::colorMatrixM20.Get(&inBag);
		fMatrix.m[2][1] = VGraphicFilterBagKeys::colorMatrixM21.Get(&inBag);
		fMatrix.m[2][2] = VGraphicFilterBagKeys::colorMatrixM22.Get(&inBag);
		fMatrix.m[2][3] = VGraphicFilterBagKeys::colorMatrixM23.Get(&inBag);
		fMatrix.m[2][4] = VGraphicFilterBagKeys::colorMatrixM24.Get(&inBag);

		fMatrix.m[3][0] = VGraphicFilterBagKeys::colorMatrixM30.Get(&inBag);
		fMatrix.m[3][1] = VGraphicFilterBagKeys::colorMatrixM31.Get(&inBag);
		fMatrix.m[3][2] = VGraphicFilterBagKeys::colorMatrixM32.Get(&inBag);
		fMatrix.m[3][3] = VGraphicFilterBagKeys::colorMatrixM33.Get(&inBag);
		fMatrix.m[3][4] = VGraphicFilterBagKeys::colorMatrixM34.Get(&inBag);
		}
		break;
	case eGraphicFilterColorMatrix_Grayscale:
		{
		//build grayscale image matrix
		
		fMatrix.m[0][0] = (GReal) 0.33;
		fMatrix.m[0][1] = (GReal) 0.33;
		fMatrix.m[0][2] = (GReal) 0.33;
		fMatrix.m[0][3] = 0;
		fMatrix.m[0][4] = 0;
		
		fMatrix.m[1][0] = (GReal) 0.33;
		fMatrix.m[1][1] = (GReal) 0.33;
		fMatrix.m[1][2] = (GReal) 0.33;
		fMatrix.m[1][3] = 0;
		fMatrix.m[1][4] = 0;
		
		fMatrix.m[2][0] = (GReal) 0.33;
		fMatrix.m[2][1] = (GReal) 0.33;
		fMatrix.m[2][2] = (GReal) 0.33;
		fMatrix.m[2][3] = 0;
		fMatrix.m[2][4] = 0;
		
		fMatrix.m[3][0] = 0;
		fMatrix.m[3][1] = 0;
		fMatrix.m[3][2] = 0;
		fMatrix.m[3][3] = 1;
		fMatrix.m[3][4] = 0;
		}
		break;
	case eGraphicFilterColorMatrix_Identity:
	default:
		break;		
	}
}


///////////////////////////////////////////////////////////////
//
// offset graphic filter : out = in1 clone translated by offset
//
///////////////////////////////////////////////////////////////

VGraphicFilterOffset::VGraphicFilterOffset ():VGraphicFilterImpl()
{
	fOffset = VPoint(0,0);
}

VGraphicFilterOffset::~VGraphicFilterOffset()
{
}


//synchronize filter properties with bag attributes
void VGraphicFilterOffset::Sync( const VValueBag& inBag)
{
	VGraphicFilterImpl::Sync( inBag);

	fOffset = VPoint(	VGraphicFilterBagKeys::offsetX.Get(&inBag),
						VGraphicFilterBagKeys::offsetY.Get(&inBag));
	fInflatingOffset = ceil(std::max(fOffset.x, fOffset.y));
}



//////////////////////////////////////////////////////////////////////////////////////////
//
// blend graphic filter : out = in1 clone with in2 alpha-blended 
//
//////////////////////////////////////////////////////////////////////////////////////////

VGraphicFilterBlend::VGraphicFilterBlend ():VGraphicFilterImpl()
{
	fBlendType = eGraphicImageBlendType_Over;
}

VGraphicFilterBlend::~VGraphicFilterBlend()
{
}


//synchronize filter properties with bag attributes
void VGraphicFilterBlend::Sync( const VValueBag& inBag)
{
	VGraphicFilterImpl::Sync( inBag);

	fBlendType = (eGraphicImageBlendType) VGraphicFilterBagKeys::blendType.Get(&inBag);
}



//////////////////////////////////////////////////////////////////////////////////////////
//
// composite graphic filter : out = in1 clone with in2 color composited
//
//////////////////////////////////////////////////////////////////////////////////////////

VGraphicFilterComposite::VGraphicFilterComposite ():VGraphicFilterImpl()
{
	fCompositeType = eGraphicFilterCompositeType_Over;
	fK1 = fK2 = fK3 = fK4 = 0;
}

VGraphicFilterComposite::~VGraphicFilterComposite()
{
}


//synchronize filter properties with bag attributes
void VGraphicFilterComposite::Sync( const VValueBag& inBag)
{
	VGraphicFilterImpl::Sync( inBag);

	fCompositeType = (eGraphicFilterCompositeType) VGraphicFilterBagKeys::compositeType.Get(&inBag);
	fK1 = VGraphicFilterBagKeys::compositeK1.Get(&inBag);
	fK2 = VGraphicFilterBagKeys::compositeK2.Get(&inBag);
	fK3 = VGraphicFilterBagKeys::compositeK3.Get(&inBag);
	fK4 = VGraphicFilterBagKeys::compositeK4.Get(&inBag);
}





////////////////////////////////////////////////////
//
// graphic filter process
//
////////////////////////////////////////////////////

VGraphicFilterProcess::VGraphicFilterProcess():VObject(),IRefCountable()
{
	fCoordsRelative = false;
	fInflatingOffset = 0.0f;
	fBlurMaxRadius = 0.0f;
}

VGraphicFilterProcess::~VGraphicFilterProcess()
{
}


//IStreamable overriden methods

VError VGraphicFilterProcess::ReadFromStream(VStream* inStream, sLONG /*inParam*/)
{
	if (inStream == NULL) 
		return VE_INVALID_PARAMETER;

	fSeq.clear();
	fInflatingOffset = 0.0f;
	fBlurMaxRadius = 0.0f;

	sLONG count = 0;
	VError error = inStream->GetLong( count);
	if (error == VE_OK)
	{
		for (int iFilter = 0; iFilter < count; iFilter++)
		{
			VGraphicFilter filter;
			error = filter.ReadFromStream( inStream);
			if (error != VE_OK)
				break;
			fSeq.push_back( filter);
			if (filter.GetType() == eGraphicFilterType_Blur)
			{
				if (filter.fImpl->fInflatingOffset > fBlurMaxRadius)
					fBlurMaxRadius = filter.fImpl->fInflatingOffset;
			}
			else
				fInflatingOffset = fInflatingOffset + filter.fImpl->fInflatingOffset;
		}
	}

	return inStream->GetLastError();
}


VError VGraphicFilterProcess::WriteToStream(VStream* inStream, sLONG /*inParam*/) const
{
	if (inStream == NULL) return VE_INVALID_PARAMETER;
	
	inStream->PutLong( (sLONG)fSeq.size());
	std::vector<VGraphicFilter>::const_iterator it = fSeq.begin();
	while (it != fSeq.end())
	{
		//apply filter
		(*it).WriteToStream( inStream);

		++it;
	}

	return inStream->GetLastError();
}


//add new filter
void VGraphicFilterProcess::Add(const VGraphicFilter& inFilter)
{
	//add new filter
	fSeq.push_back( inFilter);
	if (inFilter.GetType() == eGraphicFilterType_Blur)
	{
		if (inFilter.fImpl->fInflatingOffset > fBlurMaxRadius)
			fBlurMaxRadius = inFilter.fImpl->fInflatingOffset;
	}
	else
		fInflatingOffset = fInflatingOffset + inFilter.fImpl->fInflatingOffset;
}


//clear all filters
void VGraphicFilterProcess::Clear()
{
	fSeq.clear();
	fBlurMaxRadius = fInflatingOffset = 0.0f;
}


/** return index of the filter output image 
@remarks
	can be used as index for input image for any following filter
*/
VIndex VGraphicFilterProcess::GetFilterOutputImageIndex(const VString& inNameFilter) const
{
	//search for filter from name
	std::vector<VGraphicFilter>::const_iterator it = fSeq.begin();
	VIndex iFilter = 0;
	while (it != fSeq.end())
	{
		const VGraphicFilter& filter = *it;
		if (filter.GetName().EqualToString( inNameFilter, true))
			break;

		++iFilter;
		++it;
	}
	if (it == fSeq.end())
		return eGFIT_SOURCE_GRAPHIC;

	//determine output image index
	iFilter += (VIndex)eGFIT_RESULT_BASE;
	return iFilter;
}


//apply current filter sequence on input image source and blend the result in the specified graphic context
void VGraphicFilterProcess::Apply( VGraphicContext *inGC, VGraphicImageRef inImageSource, const VRect& inBoundsDst, const VRect& inBoundsSrc, VGraphicImageRef *outResult, eGraphicImageBlendType inBlendType, VPoint inScaleUserToDeviceUnits)
{
	xbox_assert(inImageSource != NULL);

	//init temporary graphic image table

	VGraphicImageVector fImageVector;	//graphic image vector

	inImageSource->Retain();
	fImageVector.push_back( inImageSource);	//eGFIT_SOURCE_GRAPHIC
	fImageVector.push_back( NULL);			//eGFIT_SOURCE_ALPHA		(will be later created on demand only)
	for (int iImage = 0; iImage < (int)fSeq.size(); iImage++)
		fImageVector.push_back( NULL);		//eGFIT_RESULT_BASE+0 ... eGFIT_RESULT_BASE+fSeq.size()-1

	//iterate on filters

	std::vector<VGraphicFilter>::iterator it = fSeq.begin();
	sLONG iFilter = 0;
	while (it != fSeq.end())
	{
		//apply filter
		VGraphicFilter& filter = *it;
		filter.Apply( fImageVector, iFilter, inBoundsSrc, GetCoordsRelative(), inScaleUserToDeviceUnits);

		++iFilter;
		++it;
	}

	VGraphicImageRef imageResult;
	if (fSeq.size())
		imageResult = fImageVector[eGFIT_RESULT_BASE+fSeq.size()-1];
	else
		imageResult = inImageSource;
	if (outResult != NULL && imageResult != NULL)
	{
		//return retained result 

		*outResult = imageResult;
		imageResult->Retain();
	}
	if (imageResult != NULL && inGC != NULL)
	{
		//blend result in graphic context

		imageResult->Draw( inGC, inBoundsDst, inBoundsSrc, inBlendType);
	}

	//release temporary graphic image table
	sLONG size = (sLONG)fImageVector.size();
	for (sLONG iImage = 0; iImage < size; iImage++)
		ReleaseRefCountable(&fImageVector[ iImage]);
}


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
void VGraphicFilterProcess::Apply( VGraphicContext *inGC, VGraphicImageRef inImageSource, const VRect& inBoundsDst, VGraphicImageRef *outResult, eGraphicImageBlendType inBlendType, VPoint inScaleUserToDeviceUnits)
{
	xbox_assert(inImageSource != NULL);
	VRect boundsSrc(0, 0, inImageSource->GetWidth(), inImageSource->GetHeight());
	Apply( inGC, inImageSource, inBoundsDst, boundsSrc, outResult, inBlendType, inScaleUserToDeviceUnits);
}


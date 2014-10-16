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


////////////////////////////////
//
// filter implementation methods
//
////////////////////////////////



/////////////////////////////////////////////////////////////
//
// blur graphic filter : result = in1 clone with blur filter
//
/////////////////////////////////////////////////////////////


//apply filter 
void VGraphicFilterBlur::Apply(VGraphicImageVector& ioImageVector, sLONG inFilterIndex, const VRect& inBounds, bool bIsRelative, VPoint inScaleUserToDeviceUnits)
{
	_Normalize( ioImageVector, inFilterIndex);

	//get source image index
	sLONG result = eGFIT_RESULT_BASE+inFilterIndex;
	xbox_assert(result < (sLONG)ioImageVector.size());

	//init result image as source image copy
	xbox_assert(ioImageVector[result] == NULL);
	ioImageVector[ result] = ioImageVector[_fIn]->Clone();

	if (fRadius == 0.0 || fDeviation == 0.0)
		return;

	xbox_assert((ioImageVector[ result]->GetPixelFormat() == eGraphicImagePixelFormat32));

	//get radius and deviation
	int radius = (int)fRadius;
	GReal sigma = fDeviation;

	//@remark: here we blur only alpha channel if source is eGFIT_SOURCE_ALPHA
	bool alphaOnly = (_fIn == eGFIT_SOURCE_ALPHA)?true:fAlphaOnly;

	//compute gaussian kernel
	GReal *pKernelReal = new GReal[2*radius+1];
	GReal *pCoefReal = pKernelReal;
	GReal sumReal = 0;
	GReal denom = sqrt(2*PI*sigma*sigma);
	for (int n = -radius; n <= radius; n++,pCoefReal++)
	{
		*pCoefReal = exp(-(n*n)/(2*sigma*sigma))/denom;
		sumReal += *pCoefReal;
	}
	pCoefReal = pKernelReal;
	if (sumReal > 0.00000001f)
		for (int n = -radius; n <= radius; n++,pCoefReal++)
		{
			*pCoefReal = (*pCoefReal)/sumReal;
		}
	uLONG sum = 65536;
	uLONG *pKernel = new uLONG[2*radius+1];
	pCoefReal = pKernelReal;
	uLONG *pCoef = pKernel;
	for (int n = -radius; n <= radius; n++,pCoef++,pCoefReal++)
	{
		*pCoef = (*pCoefReal)*sum;
	}

	//apply gaussian kernel on pixel data
 	uBYTE *pPixBuf = (uBYTE *)ioImageVector[result]->GetPixelBuffer();	
	int rowOffset = (ioImageVector[result]->GetStride()/4-ioImageVector[result]->GetWidth()+2*radius)*4;
	int stride = ioImageVector[result]->GetStride();
	int width = ioImageVector[result]->GetWidth();
	int height = ioImageVector[result]->GetHeight();
	int radiusMul4 = radius*4;
	int kMin = alphaOnly?3:0;
	try
	{
		//iterate horizontally on image pixel buffer
		uBYTE *pColor = pPixBuf+kMin;
		int j,i;
		for (int k = kMin; k < 4; k++, pColor = pPixBuf+k) //iterate for each b,g,r,a component
			for (j = radius, pColor += radius*stride+radiusMul4; j < height-radius; j++, pColor+=rowOffset)
			{
				for (i = radius; i < width-radius; i++, pColor+=4)
				{
					//apply one-dimensional gaussian kernel
					pCoef = pKernel;
					uBYTE *p = pColor-radiusMul4;
					uLONG val = 0;
					for (int r = -radius; r <= radius; r++, p+=4, pCoef++)
						val += (*p)*(*pCoef);
					val >>= 16;

					*pColor = (uBYTE)val;
				}
			}

		//iterate vertically on image pixel buffer
		pColor = pPixBuf+kMin;
		for (int k = kMin; k < 4; k++, pColor = pPixBuf+k) //iterate for each b,g,r,a component
			for (i = radius, pColor += radiusMul4+radius*stride; i < width-radius; i++, pColor+=4-(height-2*radius)*stride)
				for (j = radius; j < height-radius; j++, pColor+=stride)
				{
					//apply one-dimensional gaussian kernel
					pCoef = pKernel;
					uBYTE *p = pColor-radius*stride;
					uLONG val = 0;
					for (int r = -radius; r <= radius; r++, p+=stride, pCoef++)
						val += (*p)*(*pCoef);
					val >>= 16;
					
					*pColor = (uBYTE)val;
				}
	}
	catch(...)
	{
		xbox_assert(false);
	}

	delete [] pKernel;
	delete [] pKernelReal;
}


///////////////////////////////////////////////////////////////////
//
// color matrix graphic filter : result = in1 clone * color matrix
//
///////////////////////////////////////////////////////////////////


//apply filter 
void VGraphicFilterColorMatrix::Apply(VGraphicImageVector& ioImageVector, sLONG inFilterIndex, const VRect& inBounds, bool bIsRelative, VPoint inScaleUserToDeviceUnits)
{
	_Normalize( ioImageVector, inFilterIndex);

	//get source image index
	sLONG result = eGFIT_RESULT_BASE+inFilterIndex;
	xbox_assert(result < (sLONG)ioImageVector.size());

	//set result image as source image clone
	xbox_assert(ioImageVector[result] == NULL);
	VRect bounds( 0, 0, ioImageVector[_fIn]->GetWidth(), ioImageVector[_fIn]->GetHeight());
	ioImageVector[ result] = new VGraphicImage( bounds.GetWidth(), bounds.GetHeight());

	//draw source image in destination image using color matrix
	ioImageVector[ result]->DrawImage( ioImageVector[_fIn], bounds, bounds, &fMatrix, eGraphicImageBlendType_Replace); 
}


///////////////////////////////////////////////////////////////
//
// offset graphic filter : out = in1 clone translated by offset
//
///////////////////////////////////////////////////////////////

void VGraphicFilterOffset::Apply(VGraphicImageVector& ioImageVector, sLONG inFilterIndex, const VRect& inBounds, bool bIsRelative, VPoint inScaleUserToDeviceUnits)
{
	_Normalize( ioImageVector, inFilterIndex);

	//get source image index
	sLONG result = eGFIT_RESULT_BASE+inFilterIndex;
	xbox_assert(result < (sLONG)ioImageVector.size());

	//set result image as source image clone
	xbox_assert(ioImageVector[result] == NULL);
	VRect rectSrc( 0, 0, ioImageVector[_fIn]->GetWidth(), ioImageVector[_fIn]->GetHeight());
	VRect rectDst = rectSrc;

	//compute absolute coordinates if appropriate
	VPoint offset = fOffset;
	if (bIsRelative)
	{
		offset.SetX( fOffset.GetX() * inBounds.GetWidth() * inScaleUserToDeviceUnits.GetX());
		offset.SetY( fOffset.GetY() * inBounds.GetHeight() * inScaleUserToDeviceUnits.GetY());
	}
	else
		offset.Set(offset.GetX() * inScaleUserToDeviceUnits.GetX(),
				   offset.GetY() * inScaleUserToDeviceUnits.GetY());
	rectDst.SetPosBy( offset.GetX(), offset.GetY());

	ioImageVector[ result] = new VGraphicImage( rectSrc.GetWidth(), rectSrc.GetHeight());

	//draw source image in destination image using color matrix
	ioImageVector[ result]->DrawImage( ioImageVector[_fIn], rectDst, rectSrc, NULL, eGraphicImageBlendType_Replace); 
}



//////////////////////////////////////////////////////////////////////////////////////////
//
// blend graphic filter : out = in1 clone with in2 blended with one alpha-blending method
//
//////////////////////////////////////////////////////////////////////////////////////////

void VGraphicFilterBlend::Apply(VGraphicImageVector& ioImageVector, sLONG inFilterIndex, const VRect& inBounds, bool bIsRelative, VPoint inScaleUserToDeviceUnits)
{
	_Normalize( ioImageVector, inFilterIndex);

	//get source image index
	sLONG result = eGFIT_RESULT_BASE+inFilterIndex;
	xbox_assert(result < (sLONG)ioImageVector.size());

	//set result image as source image clone
	xbox_assert(ioImageVector[result] == NULL);
	VRect rect( 0, 0, ioImageVector[_fIn2]->GetWidth(), ioImageVector[_fIn2]->GetHeight());

	ioImageVector[ result] = ioImageVector[_fIn2]->Clone();

	//draw source image in destination image using color matrix
	ioImageVector[ result]->DrawImage( ioImageVector[_fIn], rect, rect, NULL, fBlendType); 
}
 


//////////////////////////////////////////////////////////////////////////////////////////
//
// composite graphic filter : out = in1 clone with in2 composited
//
//////////////////////////////////////////////////////////////////////////////////////////

void VGraphicFilterComposite::Apply(VGraphicImageVector& ioImageVector, sLONG inFilterIndex, const VRect& inBounds, bool bIsRelative, VPoint inScaleUserToDeviceUnits)
{
	_Normalize( ioImageVector, inFilterIndex);

	//get source image index
	sLONG result = eGFIT_RESULT_BASE+inFilterIndex;
	xbox_assert(result < (sLONG)ioImageVector.size());

	//set result image as source image clone
	xbox_assert(ioImageVector[result] == NULL);
	VRect rect( 0, 0, ioImageVector[_fIn2]->GetWidth(), ioImageVector[_fIn2]->GetHeight());

	ioImageVector[ result] = ioImageVector[_fIn2]->Clone();

	switch( fCompositeType)
	{
	case eGraphicFilterCompositeType_Over:
		//draw source image in destination image using color matrix
		ioImageVector[ result]->DrawImage( ioImageVector[_fIn], rect, rect, NULL, eGraphicImageBlendType_Over); 
		break;
	case eGraphicFilterCompositeType_In:
		//TODO: not implemented yet
		ioImageVector[ result]->DrawImage( ioImageVector[_fIn], rect, rect, NULL, eGraphicImageBlendType_Over); 
		break;
	case eGraphicFilterCompositeType_Out:
		//TODO: not implemented yet
		ioImageVector[ result]->DrawImage( ioImageVector[_fIn], rect, rect, NULL, eGraphicImageBlendType_Over); 
		break;
	case eGraphicFilterCompositeType_Atop:
		//TODO: not implemented yet
		ioImageVector[ result]->DrawImage( ioImageVector[_fIn], rect, rect, NULL, eGraphicImageBlendType_Over); 
		break;
	case eGraphicFilterCompositeType_Xor:
		//TODO: not implemented yet
		ioImageVector[ result]->DrawImage( ioImageVector[_fIn], rect, rect, NULL, eGraphicImageBlendType_Over); 
		break;
	case eGraphicFilterCompositeType_Arithmetic:
	default:
		//TODO: not implemented yet
		ioImageVector[ result]->DrawImage( ioImageVector[_fIn], rect, rect, NULL, eGraphicImageBlendType_Over); 
		break;
	}
}


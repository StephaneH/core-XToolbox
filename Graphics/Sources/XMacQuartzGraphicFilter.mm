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
#undef CIImage
#include "QuartzCore/CIContext.h"
#include "QuartzCore/CIImage.h"
#include "QuartzCore/CIFilter.h"
#include "QuartzCore/CIVector.h"
#include "Foundation/NSValue.h"
#include "Foundation/NSObject.h"
#include "Foundation/NSAutoreleasePool.h"
#include "Foundation/NSAffineTransform.h"


//////////////////////////////////////////////////////////
//
// blur graphic filter : result = source1 with blur filter
//
//////////////////////////////////////////////////////////

//@remark: here we need to declare setValue and valueForKey methods to remove warnings
@interface CIFilter (Extension)
- (void)setValue:(id)value forKey:(NSString *)aString;
- (id)valueForKey:(NSString *)aString;
@end

//apply filter 
void VGraphicFilterBlur::Apply( VGraphicImageVector& ioImageVector, sLONG inFilterIndex, const VRect& inBounds, bool bIsRelative, VPoint inScaleUserToDeviceUnits)
{
	_Normalize( ioImageVector, inFilterIndex);

	//get source image index
	sLONG result = eGFIT_RESULT_BASE+inFilterIndex;
	xbox_assert(result < ioImageVector.size());
	xbox_assert(ioImageVector[result] == NULL);
	
	//stack new autorelease pool
	//@remark: here we need to create a new autorelease pool 
	//		   to ensure that autoreleased objects are destroyed at the end of the method
	NSAutoreleasePool *innerPool = [[NSAutoreleasePool alloc] init];
	
	//get source image as CIImage object 
	CIImage *imageNativeCI =  (CIImage *)ioImageVector[ _fIn]->GetCIImage();
	xbox_assert(imageNativeCI);
	if (imageNativeCI && fDeviation > 0.0)
	{
		//create gaussian blur filter
		
		CIFilter *blurFilter = NULL;
		blurFilter = [CIFilter	filterWithName:@"CIGaussianBlur"]; 
		if (blurFilter)
		{
			[blurFilter setDefaults]; 
			[blurFilter setValue: imageNativeCI forKey: @"inputImage"];  
			[blurFilter setValue: [NSNumber numberWithFloat:fDeviation] forKey: @"inputRadius"];
			CIImage *outImage = NULL;
			outImage = [blurFilter valueForKey: @"outputImage"]; 
			if (outImage)
				ioImageVector[result] = new VGraphicImage( (VCacheImageNativeRef)outImage);
			else
				CopyRefCountable(&ioImageVector[result],ioImageVector[ _fIn]);	
		}	
		else
			CopyRefCountable(&ioImageVector[result],ioImageVector[ _fIn]);
	}	else
		CopyRefCountable(&ioImageVector[result],ioImageVector[ _fIn]);
	
	//close autorelease pool
	if (innerPool)
		[innerPool release];	
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
	xbox_assert(result < ioImageVector.size());
	xbox_assert(ioImageVector[result] == NULL);
	
	//stack new autorelease pool
	//@remark: here we need to create a new autorelease pool 
	//		   to ensure that autoreleased objects are destroyed at the end of the method
	NSAutoreleasePool *innerPool = [[NSAutoreleasePool alloc] init];
	
	//get source image as CIImage object 
	CIImage *imageNativeCI =  (CIImage *)ioImageVector[ _fIn]->GetCIImage();
	xbox_assert(imageNativeCI);
	if (imageNativeCI)
	{
		//create colormatrix filter
		
		CIFilter *colorMatrixFilter = NULL;
		colorMatrixFilter = [CIFilter	filterWithName:@"CIColorMatrix"]; 
		if (colorMatrixFilter)
		{
			[colorMatrixFilter setDefaults]; 
			[colorMatrixFilter setValue: imageNativeCI forKey: @"inputImage"];
			
			CIVector *inRVector = (CIVector *)[CIVector vectorWithX:fMatrix.m[0][0] Y:fMatrix.m[0][1] Z:fMatrix.m[0][2] W:fMatrix.m[0][3]];
			CIVector *inGVector = (CIVector *)[CIVector vectorWithX:fMatrix.m[1][0] Y:fMatrix.m[1][1] Z:fMatrix.m[1][2] W:fMatrix.m[1][3]];
			CIVector *inBVector = (CIVector *)[CIVector vectorWithX:fMatrix.m[2][0] Y:fMatrix.m[2][1] Z:fMatrix.m[2][2] W:fMatrix.m[2][3]];
			CIVector *inAVector = (CIVector *)[CIVector vectorWithX:fMatrix.m[3][0] Y:fMatrix.m[3][1] Z:fMatrix.m[3][2] W:fMatrix.m[3][3]];
			CIVector *inBiasVector = (CIVector *)[CIVector vectorWithX:fMatrix.m[0][4] Y:fMatrix.m[1][4] Z:fMatrix.m[2][4] W:fMatrix.m[3][4]];
			[colorMatrixFilter setValue:inRVector forKey: @"inputRVector"];
			[colorMatrixFilter setValue:inGVector forKey: @"inputGVector"];
			[colorMatrixFilter setValue:inBVector forKey: @"inputBVector"];
			[colorMatrixFilter setValue:inAVector forKey: @"inputAVector"];
			[colorMatrixFilter setValue:inBiasVector forKey: @"inputBiasVector"];

			CIImage *outImage = NULL;
			outImage = [colorMatrixFilter valueForKey: @"outputImage"];
			
			if (outImage)
				ioImageVector[result] = new VGraphicImage( (VCacheImageNativeRef)outImage);
			else
				CopyRefCountable(&ioImageVector[result],ioImageVector[ _fIn]);
		}	
		else
			CopyRefCountable(&ioImageVector[result],ioImageVector[ _fIn]);
	}	else
		CopyRefCountable(&ioImageVector[result],ioImageVector[ _fIn]);

	//close autorelease pool
	if (innerPool)
		[innerPool release];	
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
	xbox_assert(result < ioImageVector.size());
	xbox_assert(ioImageVector[result] == NULL);
	
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
	
	//stack new autorelease pool
	//@remark: here we need to create a new autorelease pool 
	//		   to ensure that autoreleased objects are destroyed at the end of the method
	NSAutoreleasePool *innerPool = [[NSAutoreleasePool alloc] init];
	
	//get source image as CIImage object 
	CIImage *imageNativeCI =  (CIImage *)ioImageVector[ _fIn]->GetCIImage();
	xbox_assert(imageNativeCI);
	if (imageNativeCI)
	{
		//create offset filter
		
		CIFilter *offsetFilter = NULL;
		offsetFilter = [CIFilter filterWithName:@"CIAffineTransform"]; 
		if (offsetFilter)
		{
			NSAffineTransform *mat = [NSAffineTransform transform];
			[mat translateXBy:offset.GetX() yBy:offset.GetY()];
			
			[offsetFilter setDefaults]; 
			[offsetFilter setValue: imageNativeCI forKey: @"inputImage"];  
			[offsetFilter setValue: mat forKey: @"inputTransform"];
			CIImage *outImage = NULL;
			outImage = [offsetFilter valueForKey: @"outputImage"]; 
			if (outImage)
				ioImageVector[result] = new VGraphicImage( (VCacheImageNativeRef)outImage);
			else
				CopyRefCountable(&ioImageVector[result],ioImageVector[ _fIn]);	
		}	
		else
			CopyRefCountable(&ioImageVector[result],ioImageVector[ _fIn]);
	}	else
		CopyRefCountable(&ioImageVector[result],ioImageVector[ _fIn]);
	
	//close autorelease pool
	if (innerPool)
		[innerPool release];	
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
	xbox_assert(result < ioImageVector.size());
	xbox_assert(ioImageVector[result] == NULL);
	
	ioImageVector[result] = ioImageVector[ _fIn2]->Clone();
	
	//stack new autorelease pool
	//@remark: here we need to create a new autorelease pool 
	//		   to ensure that autoreleased objects are destroyed at the end of the method
	NSAutoreleasePool *innerPool = [[NSAutoreleasePool alloc] init];
	
	//get source image as CIImage object 
	CIImage *imageSource =  (CIImage *)ioImageVector[ _fIn]->GetCIImage();
	CIImage *imageBackground =  (CIImage *)ioImageVector[ result]->GetCIImage();
	xbox_assert(imageSource && imageBackground);
	if (imageSource)
	{
		//create blend compositing filter
		
		CIFilter *filter = NULL;
		switch(fBlendType)
		{
			case eGraphicImageBlendType_Replace:
				//do nothing: just source image copy
			break;
			case eGraphicImageBlendType_Over:
				filter = [CIFilter filterWithName:@"CISourceOverCompositing"]; 
				break;
			case eGraphicImageBlendType_Multiply:
				filter = [CIFilter filterWithName:@"CIMultiplyBlendMode"]; 
				break;
			case eGraphicImageBlendType_Screen:
				filter = [CIFilter filterWithName:@"CIScreenBlendMode"]; 
				break;
			case eGraphicImageBlendType_Darken:
				filter = [CIFilter filterWithName:@"CIDarkenBlendMode"]; 
				break;
			case eGraphicImageBlendType_Lighten:
				filter = [CIFilter filterWithName:@"CILightenBlendMode"]; 
				break;
			default:
				xbox_assert(false);
			break;
		}
			
		if (filter)
		{
			[filter setDefaults]; 
			[filter setValue: imageSource forKey: @"inputImage"];  
			[filter setValue: imageBackground forKey: @"inputBackgroundImage"];  
			CIImage *outImage = NULL;
			outImage = [filter valueForKey: @"outputImage"]; 
			if (outImage)
			{
				ReleaseRefCountable(&ioImageVector[result]);
				ioImageVector[result] = new VGraphicImage( (VCacheImageNativeRef)outImage);
			}
		}	
	}		
	//close autorelease pool
	if (innerPool)
		[innerPool release];	
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
	xbox_assert(result < ioImageVector.size());
	xbox_assert(ioImageVector[result] == NULL);
	
	ioImageVector[result] = ioImageVector[ _fIn2]->Clone();
	
	//stack new autorelease pool
	//@remark: here we need to create a new autorelease pool 
	//		   to ensure that autoreleased objects are destroyed at the end of the method
	NSAutoreleasePool *innerPool = [[NSAutoreleasePool alloc] init];
	
	//get source image as CIImage object 
	CIImage *imageSource =  (CIImage *)ioImageVector[ _fIn]->GetCIImage();
	CIImage *imageBackground =  (CIImage *)ioImageVector[ result]->GetCIImage();
	xbox_assert(imageSource && imageBackground);
	if (imageSource)
	{
		//create compositing filter
		
		CIFilter *filter = NULL;
		switch(fCompositeType)
		{
			case eGraphicFilterCompositeType_Over:
				filter = [CIFilter filterWithName:@"CISourceOverCompositing"]; 
			break;
			case eGraphicFilterCompositeType_In:
				filter = [CIFilter filterWithName:@"CISourceInCompositing"]; 
				break;
			case eGraphicFilterCompositeType_Out:
				filter = [CIFilter filterWithName:@"CISourceOutCompositing"]; 
				break;
			case eGraphicFilterCompositeType_Atop:
				filter = [CIFilter filterWithName:@"CISourceAtopCompositing"]; 
				break;
			case eGraphicFilterCompositeType_Xor:
				//TODO: not implemented yet
				filter = [CIFilter filterWithName:@"CISourceOverCompositing"]; 
				break;
			case eGraphicFilterCompositeType_Arithmetic:
				//TODO: not implemented yet
				filter = [CIFilter filterWithName:@"CISourceOverCompositing"]; 
				break;
			default:
				xbox_assert(false);
			break;
		}
			
		if (filter)
		{
			[filter setDefaults]; 
			[filter setValue: imageSource forKey: @"inputImage"];  
			[filter setValue: imageBackground forKey: @"inputBackgroundImage"];  
			CIImage *outImage = NULL;
			outImage = [filter valueForKey: @"outputImage"]; 
			if (outImage)
			{
				ReleaseRefCountable(&ioImageVector[result]);
				ioImageVector[result] = new VGraphicImage( (VCacheImageNativeRef)outImage);
			}
		}	
	}		
	//close autorelease pool
	if (innerPool)
		[innerPool release];	
}




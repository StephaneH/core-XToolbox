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
#include "VGraphicImage.h"
#include "VGraphicFilter.h"

#include "QuartzCore/CIContext.h"
#include "QuartzCore/CIImage.h"
#include "QuartzCore/CIFilter.h"
#include "QuartzCore/CIImageAccumulator.h"

#import <Cocoa/Cocoa.h>

VGraphicImage::VGraphicImage():VObject(),IRefCountable()
{
	fImage		= NULL;
}

VGraphicImage::~VGraphicImage()
{
	Clear();
}

VGraphicImage::VGraphicImage( VCacheImageNativeRef inImage):VObject(),IRefCountable()
{
	xbox_assert( inImage != NULL);
	
	CIImage *imageCI = (CIImage *)inImage;
	[imageCI retain];
	
	fImage		= inImage;
}


//create a CoreImage image with the specified dimensions, pixel format and color
CIImage *CreateCIImage( sLONG inWidth, sLONG inHeight, eGraphicImagePixelFormat inPixelFormat, VColor inColor)
{
	CIImage *imageCI = NULL;
	
	switch ( inPixelFormat)
	{
		case eGraphicImagePixelFormat32:
		{
			//create temporary CGImage
			VBitmapData *data = new VBitmapData( inWidth, inHeight, VBitmapData::VPixelFormat32bppPARGB);
			data->Clear( inColor);
			
			CGImageRef image = data->CreateNativeBitmap();
			delete data;
			if (image)
			{
				//create CIImage from CGImage
				imageCI = [[CIImage alloc] initWithCGImage: image];
				
				CGImageRelease(image);
			}
		}
			break;
		default:
			xbox_assert(false);
	}
	return imageCI;
}


VGraphicImage::VGraphicImage( sLONG inWidth, sLONG inHeight, eGraphicImagePixelFormat inPixelFormat, const VColor& inColor):VObject(),IRefCountable()
{
	fImage = NULL;
	switch ( inPixelFormat)
	{
		case eGraphicImagePixelFormat32:
			fImage = (VCacheImageNativeRef)CreateCIImage( inWidth, inHeight, inPixelFormat, inColor);
			break;
		default:
			xbox_assert(false);
			break;
	}
}

//create image with the specified image data reference
//@remark: the image data is not owned by this object: caller can destroy it
VGraphicImage::VGraphicImage( VBitmapData *inBitmapData):VObject(),IRefCountable()
{
	xbox_assert(inBitmapData);
	
	fImage = NULL;
	CGImageRef image = inBitmapData->CreateNativeBitmap();
	if (image)
	{
		//create CIImage from CGImage
		fImage = (VCacheImageNativeRef) [[CIImage alloc] initWithCGImage: image];
		
		CGImageRelease(image);
	}
}


VGraphicImage::VGraphicImage( VImageNativeRef inImageNative):VObject(),IRefCountable()
{
	fImage = NULL;
	if (inImageNative != NULL)
	{
		//create CIImage from CGImage
		CIImage *imageCI = [[CIImage alloc] initWithCGImage: inImageNative];
		fImage = (void *)imageCI;
	}
}


//create image from the specified CG layer
//@remark: native layer is not further referenced so caller can destroy it
VGraphicImage::VGraphicImage( CGLayerRef inImageNative):VObject(),IRefCountable()
{
	fImage = NULL;
	if (inImageNative != NULL)
	{
		//create CIImage from CGLayer
		
		//default CIImage colorspace if GenericRGB so we need to set explicitly layer colorspace to DeviceRGB
		//because layers are created from device gc & CoreImage cannot get the info from CGLayer
		//(for initWithCGImage it is not necessary because CGImage is bound with a colorspace & so CoreImage will use it)
		NSMutableDictionary *opt = [[NSMutableDictionary alloc] init];
		CGColorSpaceRef colorspace = CGColorSpaceCreateDeviceRGB();
		[opt setObject:(id)colorspace forKey:kCIImageColorSpace];
		CGColorSpaceRelease( colorspace);
		
		CIImage *imageCI = [[CIImage alloc] initWithCGLayer:inImageNative options:opt];
		
		[opt release];
		
		fImage = (void *)imageCI;
	}
}


//native image accessor
VImageNativeRef VGraphicImage::CreateNative()
{
	return NULL;
}


//width, height and pixel format accessors
uLONG VGraphicImage::GetWidth()
{
	if (fImage == NULL)
		return 0;
	
	CIImage *imageCI = (CIImage *)fImage;
	CGRect bounds = [imageCI extent];
	return (uLONG)bounds.size.width;
	
}

uLONG VGraphicImage::GetHeight()
{
	if (fImage == NULL)
		return 0;
	
	CIImage *imageCI = (CIImage *)fImage;
	CGRect bounds = [imageCI extent];
	return (uLONG)bounds.size.height;
}

uLONG VGraphicImage::GetStride()
{
	return GetWidth()*4;
}


eGraphicImagePixelFormat VGraphicImage::GetPixelFormat()
{
	if (fImage)
	{
		return eGraphicImagePixelFormat32;
	}	
	else
	{
		return eGraphicImagePixelFormatUnknown;
	}
}



//clear image with the specified color
void VGraphicImage::Fill(const VColor& inColor)
{
	if (fImage == NULL)
		return;
	
	CIImage *imageCI = (CIImage *)fImage;
	CGRect bounds = [imageCI extent];
	[imageCI release];
	
	fImage = (VCacheImageNativeRef)CreateCIImage( bounds.size.width, bounds.size.height, eGraphicImagePixelFormat32, inColor);
}


//clear image data
void VGraphicImage::Clear()
{
	if (fImage == NULL)
		return;
	
	CIImage *imageCI = (CIImage *)fImage;
	[imageCI release];
	fImage = NULL;
	
}

//clone image
VGraphicImage *VGraphicImage::Clone()
{
	if (fImage == NULL)
		return NULL;
	
	CIImage *imageCI = (CIImage *)fImage;
	
	CIImage *cloneImageCI =  [imageCI copy];
	VGraphicImage *image = new VGraphicImage( (VCacheImageNativeRef)cloneImageCI);
	[cloneImageCI release];
	return image;
}


//draw image in the destination context using the specified destination and source rectangles
void VGraphicImage::Draw( CGContextRef inGC, const VRect& inDstRect, const VRect& inSrcRect, eGraphicImageBlendType inBlendType)
{
	if (fImage == NULL)
		return;
	CIImage *imageCI = (CIImage *)fImage;
	
	if (inGC == NULL)
		return;
		
	//apply blend compositing mode
	switch (inBlendType)
	{
		case eGraphicImageBlendType_Replace:
			CGContextSetBlendMode( inGC, kCGBlendModeNormal);
			break;
		case eGraphicImageBlendType_Over:
			CGContextSetBlendMode( inGC, kCGBlendModeNormal);
			break;
		case eGraphicImageBlendType_Multiply:
			CGContextSetBlendMode( inGC, kCGBlendModeMultiply);
			break;
		case eGraphicImageBlendType_Screen:
			CGContextSetBlendMode( inGC, kCGBlendModeScreen);
			break;
		case eGraphicImageBlendType_Darken:
			CGContextSetBlendMode( inGC, kCGBlendModeDarken);
			break;
		case eGraphicImageBlendType_Lighten:
			CGContextSetBlendMode( inGC, kCGBlendModeLighten);
			break;
		default:
			xbox_assert(false);
			break;
	}
	
	CIImageAccumulator *imageAccu = [[CIImageAccumulator alloc] initWithExtent:[imageCI extent] format:kCIFormatARGB8];
	[imageAccu setImage:imageCI];
	
	NSAutoreleasePool *innerPool = [[NSAutoreleasePool alloc] init];
	
	CGRect  dstRect = inDstRect;
	CGRect  srcRect = inSrcRect;
	
	//save current Cocoa context
	NSGraphicsContext *gcNSBackup = [NSGraphicsContext currentContext];
	
	//create new Cocoa context from cg context & set it as current context
	NSGraphicsContext *gcNS = [[NSGraphicsContext graphicsContextWithGraphicsPort:inGC flipped:NO ] retain];
	[NSGraphicsContext setCurrentContext:gcNS];
	
	//allocate CoreImage autorelease pool
	NSAutoreleasePool *innerPool2 = [[NSAutoreleasePool alloc] init];
	
	//create CoreImage context from Cocoa context
	CIContext* gcCI = [[gcNS CIContext] retain];
	
	//apply filter to the source image
	[gcCI drawImage:[imageAccu image] inRect:dstRect fromRect:srcRect];
	
	//release CoreImage context & autorelease pool
	[gcCI release];
	if (innerPool2)
		[innerPool2 release];
	
	//flush & release Cocoa context
	[gcNS release];
	
	//restore Cocoa context
	[NSGraphicsContext setCurrentContext:gcNSBackup];
	
	//close Cocoa autorelease pool
	if (innerPool)
		[innerPool release];	
	
	[imageAccu release];
	
	//restore blend compositing mode
	CGContextSetBlendMode( inGC, kCGBlendModeNormal);
}

//draw image in the destination context using the specified destination and source rectangles
void VGraphicImage::Draw( VGraphicContext *_inGC, const VRect& inDstRect, const VRect& inSrcRect, eGraphicImageBlendType inBlendType)
{
	if (_inGC == NULL)
		return;

	_inGC->SaveContext();
	CGContextRef inGC = _inGC->GetNativeRef();
	Draw( inGC, inDstRect, inSrcRect, inBlendType);
	_inGC->RestoreContext();
}	


//draw the specified image using the specified destination and source rectangles
//@remark: this remains abstract until the image is drawed in a VGraphicContext 
//		   because we use CIImage (CoreImage abstract image class) in this implementation 
void VGraphicImage::DrawImage( VGraphicImage *inImageSource, const VRect& inDstRect, const VRect& inSrcRect, const VColorMatrix *inColorMat, eGraphicImageBlendType inBlendType)
{
	if (!testAssert(inImageSource))
		return;
	if (!fImage)
		return;
	
	//@remark: here we only manage translation 
	//		   TODO: add filter for full affine transform management ?
	xbox_assert((inDstRect.GetWidth() == inSrcRect.GetWidth())
				&&
				(inDstRect.GetHeight() == inSrcRect.GetHeight()));
		
	VGraphicFilterProcess filterProcess;
	if (inColorMat)
	{
		if ((inDstRect.GetX() != inSrcRect.GetX())
			||
			(inDstRect.GetY() != inSrcRect.GetY()))
		{
			//apply translation
			
			VGraphicFilter filter( eGraphicFilterType_Offset);

			VValueBag bagAttributes;
			bagAttributes.SetLong( VGraphicFilterBagKeys::in, eGFIT_SOURCE_GRAPHIC);
			bagAttributes.SetReal( VGraphicFilterBagKeys::offsetX, inDstRect.GetX()-inSrcRect.GetX());
			bagAttributes.SetReal( VGraphicFilterBagKeys::offsetY, inDstRect.GetY()-inSrcRect.GetY());
			
			filter.SetAttributes(bagAttributes);
			filterProcess.Add( filter);
		}
		if (inColorMat)
		{
			//apply color matrix
			
			VGraphicFilter filter( eGraphicFilterType_ColorMatrix);

			VValueBag bagAttributes;
			bagAttributes.SetLong( VGraphicFilterBagKeys::in, eGFIT_LAST_RESULT);
			bagAttributes.SetLong( VGraphicFilterBagKeys::colorMatrixType, eGraphicFilterColorMatrix_Matrix);
			
			bagAttributes.SetReal( VGraphicFilterBagKeys::colorMatrixM00, inColorMat->m[0][0]);
			bagAttributes.SetReal( VGraphicFilterBagKeys::colorMatrixM01, inColorMat->m[0][1]);
			bagAttributes.SetReal( VGraphicFilterBagKeys::colorMatrixM02, inColorMat->m[0][2]);
			bagAttributes.SetReal( VGraphicFilterBagKeys::colorMatrixM03, inColorMat->m[0][3]);
			bagAttributes.SetReal( VGraphicFilterBagKeys::colorMatrixM04, inColorMat->m[0][4]);
			
			bagAttributes.SetReal( VGraphicFilterBagKeys::colorMatrixM10, inColorMat->m[1][0]);
			bagAttributes.SetReal( VGraphicFilterBagKeys::colorMatrixM11, inColorMat->m[1][1]);
			bagAttributes.SetReal( VGraphicFilterBagKeys::colorMatrixM12, inColorMat->m[1][2]);
			bagAttributes.SetReal( VGraphicFilterBagKeys::colorMatrixM13, inColorMat->m[1][3]);
			bagAttributes.SetReal( VGraphicFilterBagKeys::colorMatrixM14, inColorMat->m[1][4]);
			
			bagAttributes.SetReal( VGraphicFilterBagKeys::colorMatrixM20, inColorMat->m[2][0]);
			bagAttributes.SetReal( VGraphicFilterBagKeys::colorMatrixM21, inColorMat->m[2][1]);
			bagAttributes.SetReal( VGraphicFilterBagKeys::colorMatrixM22, inColorMat->m[2][2]);
			bagAttributes.SetReal( VGraphicFilterBagKeys::colorMatrixM23, inColorMat->m[2][3]);
			bagAttributes.SetReal( VGraphicFilterBagKeys::colorMatrixM24, inColorMat->m[2][4]);
			
			bagAttributes.SetReal( VGraphicFilterBagKeys::colorMatrixM30, inColorMat->m[3][0]);
			bagAttributes.SetReal( VGraphicFilterBagKeys::colorMatrixM31, inColorMat->m[3][1]);
			bagAttributes.SetReal( VGraphicFilterBagKeys::colorMatrixM32, inColorMat->m[3][2]);
			bagAttributes.SetReal( VGraphicFilterBagKeys::colorMatrixM33, inColorMat->m[3][3]);
			bagAttributes.SetReal( VGraphicFilterBagKeys::colorMatrixM34, inColorMat->m[3][4]);
			
			filter.SetAttributes(bagAttributes);
			filterProcess.Add( filter);
		}
		
		//create new transformed image 
		VGraphicImage *imageFinal = NULL;
		filterProcess.Apply( NULL,	
							 inImageSource, 
							 VRect(0, 0, 0, 0),
							 &imageFinal,
							 inBlendType);
		if (imageFinal)
		{
			//'catch' filter result CIImage 

			CIImage *imageCI = (CIImage *)fImage;
			[imageCI release];
			fImage = NULL;
			
			fImage = (VCacheImageNativeRef) (imageCI = (CIImage *)imageFinal->GetCIImage());			
			if (imageCI)
				[imageCI retain];
				
			imageFinal->Release();	
		}
	}
}







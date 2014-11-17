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
#ifndef __VGraphicImage__
#define __VGraphicImage__

#include "Graphics/Sources/V4DPictureIncludeBase.h"
#include "Graphics/Sources/VGraphicContext.h"
#include "Graphics/Sources/VRect.h"
#include "Graphics/Sources/V4DPictureTools.h"

BEGIN_TOOLBOX_NAMESPACE

#if VERSIONWIN
//GDIPLUS implementation: use Gdiplus Bitmap as native image
//Direct2D implementation: use ID2D1Bitmap as native image
typedef void *VImageNativeRef; 

//use VBitmapData as internal stored image
typedef VBitmapData *VCacheImageNativeRef;	
#else
#if VERSIONMAC
//Quartz2D implementation

//use CoreGraphics Image as native image
typedef CGImageRef VImageNativeRef;

//use CoreImage Image as internal stored image
typedef void *VCacheImageNativeRef;	//here we need to use opaque type for CIImage * because CIImage is pure Objective-C
#endif
#endif

typedef enum eGraphicImagePixelFormat 
{
    eGraphicImagePixelFormat32,		//32bpp pixel format 
									//	PC OS Gdiplus:		ARGB  not premultiplied alpha
									//  MAC OS Quartz2D:	PARGB premultiplied alpha (needed to use CoreImage filters)
	eGraphicImagePixelFormatUnknown

}	eGraphicImagePixelFormat;


//color matrix type definition
typedef struct VColorMatrix
{
    GReal m[5][5];
} VColorMatrix;


//blend type enumeration 
typedef enum eGraphicImageBlendType
{
	eGraphicImageBlendType_Replace,
	eGraphicImageBlendType_Over,
	eGraphicImageBlendType_Multiply,	//TODO: not implemented yet on Windows OS
	eGraphicImageBlendType_Screen,		//TODO: not implemented yet on Windows OS
	eGraphicImageBlendType_Darken,		//TODO: not implemented yet on Windows OS
	eGraphicImageBlendType_Lighten		//TODO: not implemented yet on Windows OS
} eGraphicImageBlendType;


//graphic image type
class VGraphicImage;

class VGraphicImage: public VObject, public IRefCountable
{
public:
	//lifetime methods
	VGraphicImage();
	virtual ~VGraphicImage();

	//create image from specified width, height and pixel format
	VGraphicImage( sLONG inWidth, sLONG inHeight, eGraphicImagePixelFormat inPixelFormat = eGraphicImagePixelFormat32, const VColor& inColor = VColor(0,0,0,0));

	//create image from the specified native image reference
	//@remark: native image is not further referenced so caller can destroy it
	VGraphicImage( VImageNativeRef inImageNative);

	//create image with the specified image data reference
	//@remark: on Windows OS, the image data is owned by this object: caller must not destroy it
	//@remark: on Mac OS, the image data is not owned by this object: caller can destroy it
	VGraphicImage( VBitmapData *inBitmapData);
	
#if VERSIONWIN
	//pixel buffer accessor
	void *GetPixelBuffer();
#endif
#if VERSIONMAC
	//create image with the specified CoreImage image reference
	//@remark: the CoreImage image is retained until object destruction
	VGraphicImage( VCacheImageNativeRef inImage);
	
	//create image from the specified CG layer
	//@remark: native layer is not further referenced so caller can destroy it
	VGraphicImage( CGLayerRef inImageNative);
	
	//CoreImage image accessor
	VCacheImageNativeRef GetCIImage()
	{
		return fImage;
	}
#endif
	
	//create and return native image
	//@remark: caller must destroy it
	VImageNativeRef CreateNative();

	//width, height and pixel format accessors
	uLONG GetWidth();
	uLONG GetHeight();
	uLONG GetStride();
	eGraphicImagePixelFormat GetPixelFormat();

	//clear image with the specified color
	void Fill( const VColor& inColor);
	
	//clear image data
	void Clear();

	//clone image
	VGraphicImage *Clone();
	
	//draw image in the destination context using the specified destination and source rectangles
	void Draw( VGraphicContext *inGC, const VRect& inDstRect, const VRect& inSrcRect, eGraphicImageBlendType inBlendType = eGraphicImageBlendType_Over);
	
	//draw image in the destination context using the specified destination and source rectangles
#if VERSIONMAC
	void Draw( CGContextRef inGC, const VRect& inDstRect, const VRect& inSrcRect, eGraphicImageBlendType inBlendType = eGraphicImageBlendType_Over);
#endif
	
	//draw the specified image using the specified destination and source rectangles
	void DrawImage( VGraphicImage *inImageSource, const VRect& inDstRect, const VRect& inSrcRect, const VColorMatrix *inColorMat = NULL, eGraphicImageBlendType inBlendType = eGraphicImageBlendType_Over);

private:
	//stored image data
	VCacheImageNativeRef fImage;
};

//graphic image opaque type reference
typedef VGraphicImage	*VGraphicImageRef;



END_TOOLBOX_NAMESPACE

#endif
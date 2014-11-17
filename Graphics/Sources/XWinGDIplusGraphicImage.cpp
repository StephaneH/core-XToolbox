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

VGraphicImage::VGraphicImage():VObject(),IRefCountable()
{
	fImage		= NULL;
}

VGraphicImage::~VGraphicImage()
{
	if (fImage)
		delete fImage;
	fImage = NULL;
}

VGraphicImage::VGraphicImage( VBitmapData *inBitmapData):VObject(),IRefCountable()
{
	xbox_assert( inBitmapData != NULL);
	fImage = inBitmapData;
}


VGraphicImage::VGraphicImage( sLONG inWidth, sLONG inHeight, eGraphicImagePixelFormat inPixelFormat, const VColor& inColor):VObject(),IRefCountable()
{
	fImage = NULL;
	switch ( inPixelFormat)
	{
	case eGraphicImagePixelFormat32:
		{
			fImage = new VBitmapData( inWidth, inHeight, VBitmapData::VPixelFormat32bppARGB);
			xbox_assert(fImage);
			if (fImage)
				fImage->Clear(inColor);
		}
		break;
	default:
		xbox_assert(false);
	}
}

VGraphicImage::VGraphicImage( VImageNativeRef inImageNative):VObject(),IRefCountable()
{
	fImage = NULL;
	if (inImageNative != NULL)
		fImage = new VBitmapData( *((Gdiplus::Bitmap *)inImageNative));
}


//native image accessor
VImageNativeRef VGraphicImage::CreateNative()
{
	if (fImage != NULL)
		return (VImageNativeRef) fImage->CreateNativeBitmap();
	else
		return NULL;
}


//width, height and pixel format accessors
uLONG VGraphicImage::GetWidth()
{
	if (fImage)
		return (uLONG)fImage->GetWidth();
	else
		return 0;
}

uLONG VGraphicImage::GetHeight()
{
	if (fImage)
		return (uLONG)fImage->GetHeight();
	else
		return 0;
}

uLONG VGraphicImage::GetStride()
{
	if (fImage)
		return (uLONG)fImage->GetRowByte();
	else
		return 0;
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
	if (fImage)
		fImage->Clear( inColor);
}


//clear image data
void VGraphicImage::Clear()
{
	if (fImage)
		delete fImage;
	fImage = NULL;
}

//clone image
VGraphicImage *VGraphicImage::Clone()
{
	if (fImage)
	{
		VBitmapData *cloneBmpData = NULL;
		try
		{
			cloneBmpData = new VBitmapData( fImage->GetWidth(), 
											fImage->GetHeight(),
											fImage->GetPixelsFormat());
			int height = cloneBmpData->GetHeight();
			int stride =  min(((uLONG)cloneBmpData->GetRowByte()), GetStride());

			int strideSrcDiv4 = GetStride()/4;
			uLONG *pSrc = (uLONG *)GetPixelBuffer();

			int strideDstDiv4 = cloneBmpData->GetRowByte()/4;
			uLONG *pDst = (uLONG *)cloneBmpData->GetPixelBuffer();

			for (int j = 0; j < height; j++, pSrc+=strideSrcDiv4, pDst+=strideDstDiv4)
				memcpy( (void *)(pDst),
						(const void *)(pSrc),
						(size_t)(stride));
		}
		catch(...)
		{
			if (cloneBmpData)
				delete cloneBmpData;
			return NULL;
		}

		if (cloneBmpData)
			return new VGraphicImage( cloneBmpData);
	}	
	return NULL;
}

//pixel buffer accessor
void* VGraphicImage::GetPixelBuffer()
{
	if (fImage)
		return fImage->GetPixelBuffer();
	else
		return NULL;
}


//draw image in the destination context using the specified destination and source rectangles
void VGraphicImage::Draw( VGraphicContext *_inGC, const VRect& inDstRect, const VRect& inSrcRect, eGraphicImageBlendType inBlendType)
{
	if (_inGC == NULL)
		return;
	Gdiplus::Graphics* inGC = (Gdiplus::Graphics *)_inGC->GetNativeRef();
	
	if (testAssert(fImage != NULL))
	{
		Gdiplus::Bitmap *image = (Gdiplus::Bitmap *)CreateNative();
		if (image != NULL)
		{
			Gdiplus::RectF rectDst(	inDstRect.GetLeft(), 
									inDstRect.GetTop(), 
									inDstRect.GetWidth(), 
									inDstRect.GetHeight());

			//set compositing mode
			Gdiplus::CompositingMode compositingMode = inGC->GetCompositingMode();
			switch (inBlendType)
			{
			case eGraphicImageBlendType_Replace:
				//copy source
				{
				if (compositingMode != Gdiplus::CompositingModeSourceCopy)
					inGC->SetCompositingMode( Gdiplus::CompositingModeSourceCopy);
				}
				break;
			case eGraphicImageBlendType_Over:
			default:
				{
				//blend source on destination using normal alpha-blend composition
				if (compositingMode != Gdiplus::CompositingModeSourceOver)
					inGC->SetCompositingMode( Gdiplus::CompositingModeSourceOver);
				}
				break;
			}

			Gdiplus::Status status = inGC->DrawImage(	
												static_cast<Gdiplus::Image *>(image), 
												rectDst,
												inSrcRect.GetLeft(), inSrcRect.GetTop(),
												inSrcRect.GetWidth(), inSrcRect.GetHeight(),
												Gdiplus::UnitPixel);

			//restore previous compositing mode
			if (inGC->GetCompositingMode() != compositingMode)
				inGC->SetCompositingMode( compositingMode);

			delete image;
		}
	}
}


//draw the specified image using the specified destination and source rectangles
void VGraphicImage::DrawImage( VGraphicImage *inImageSource, const VRect& inDstRect, const VRect& inSrcRect, const VColorMatrix *inColorMat, eGraphicImageBlendType inBlendType)
{
	if (!testAssert(inImageSource && (this != inImageSource)))
		return;

	Gdiplus::ColorMatrix mat;
	Gdiplus::ImageAttributes imAttr;
	if (inColorMat)
	{
		//set native color matrix 
		for (int j = 0; j < 5; j++)
			for (int i = 0; i < 5; i++)
				mat.m[i][j] = inColorMat->m[j][i];

		// set a ImageAttributes object with the previous color matrix
		imAttr.SetColorMatrix( &mat );
	}

	// draw the source image in the result image using image attributes
	Gdiplus::Bitmap *bmpDest = (Gdiplus::Bitmap *)CreateNative();
	Gdiplus::Bitmap *bmpSrc  = (Gdiplus::Bitmap *)inImageSource->CreateNative();

	if (testAssert(bmpDest && bmpSrc))
	{
		Gdiplus::Graphics *gcDest = new Gdiplus::Graphics( bmpDest);
		if (testAssert(gcDest))
		{
			Gdiplus::Rect rectDst(	(INT)inDstRect.GetX(), 
									(INT)inDstRect.GetY(), 
									(INT)inDstRect.GetWidth(), 
									(INT)inDstRect.GetHeight());


			//set compositing mode
			Gdiplus::CompositingMode compositingMode = gcDest->GetCompositingMode();
			switch (inBlendType)
			{
			case eGraphicImageBlendType_Replace:
				//copy source
				{
				if (compositingMode != Gdiplus::CompositingModeSourceCopy)
					gcDest->SetCompositingMode( Gdiplus::CompositingModeSourceCopy);
				}
				break;
			case eGraphicImageBlendType_Over:
			default:
				{
				//blend source on destination using normal alpha-blend composition
				if (compositingMode != Gdiplus::CompositingModeSourceOver)
					gcDest->SetCompositingMode( Gdiplus::CompositingModeSourceOver);
				}
				break;
			}

			Gdiplus::Status status;
			if (inColorMat)
				status = gcDest->DrawImage(	
											static_cast<Gdiplus::Image *>(bmpSrc), 
											rectDst,
											(INT)inSrcRect.GetX(), (INT)inSrcRect.GetY(),
											(INT)inSrcRect.GetWidth(), (INT)inSrcRect.GetHeight(),
											Gdiplus::UnitPixel,
											&imAttr);
			else
				status = gcDest->DrawImage(	
											static_cast<Gdiplus::Image *>(bmpSrc), 
											rectDst,
											(INT)inSrcRect.GetX(), (INT)inSrcRect.GetY(),
											(INT)inSrcRect.GetWidth(), (INT)inSrcRect.GetHeight(),
											Gdiplus::UnitPixel);

			delete gcDest;

			if (fImage)
				delete fImage;
			fImage = new VBitmapData( *bmpDest);
		}
		delete bmpSrc;
		delete bmpDest;
	}
}






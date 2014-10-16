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
#include "V4DPictureIncludeBase.h"
namespace V4DPicturesSettingsBagKeys
{
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( vers,VLong,sLONG,1);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( peX,VLong,sLONG,0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( peY,VLong,sLONG,0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( peMode,VLong,sLONG,0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( frH,VLong,sLONG,1); // old for compatibility only
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( frV,VLong,sLONG,1); // old for compatibility only
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( frLine,VLong,sLONG,1);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( frCol,VLong,sLONG,1);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( frSplit,VLong,sLONG,0);
	CREATE_BAGKEY(affine);
	CREATE_BAGKEY(transcol);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( alpha,VLong,sLONG,100);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( drmode,VLong,sLONG,0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( scalemode,VLong,sLONG,PM_SCALE_TO_FIT);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( devicecanuselayer,VLong,sLONG,1);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( deviceprinter,VLong,sLONG,0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( devicecanusecleartype,VLong,sLONG,1);
}
void VPictureDrawSettings::GetDrawingMatrix(VAffineTransform& outMat,bool inAutoSwapAxis)
{
	VAffineTransform tmp(fDrawingMatrix);
	
	outMat=fDrawingMatrix;
	if(inAutoSwapAxis && (fYAxisSwap || fXAxisSwap))
	{
		GReal tx,ty;
		
		tx=outMat.GetTranslateX();
		ty=outMat.GetTranslateY();
		
		if(fXAxisSwap)
			tx *= -1;
		if(fYAxisSwap)
			ty *= -1;
			
		outMat.SetTranslation(tx,ty);
	}
}
VError	VPictureDrawSettings::LoadFromBag( const VValueBag& inBag)
{
	VError result=VE_UNKNOWN_ERROR;
	result=inherited::LoadFromBag(inBag);
	if(result==VE_OK)
	{
		fScaleMode=(PictureMosaic)V4DPicturesSettingsBagKeys::scalemode.Get(&inBag);
		fDeviceCanUseLayers=(V4DPicturesSettingsBagKeys::devicecanuselayer.Get(&inBag)==1);
		fDevicePrinter=(V4DPicturesSettingsBagKeys::deviceprinter.Get(&inBag)==1);
		fDeviceCanUseClearType=(V4DPicturesSettingsBagKeys::devicecanusecleartype.Get(&inBag)==1);
	}
	return result;
}
VError	VPictureDrawSettings::SaveToBag( VValueBag& ioBag, VString& outKind) const
{
	VError result=VE_UNKNOWN_ERROR;
	result=inherited::SaveToBag(ioBag,outKind);
	if(result==VE_OK)
	{
		ioBag.SetLong(V4DPicturesSettingsBagKeys::scalemode,fScaleMode);
		ioBag.SetLong(V4DPicturesSettingsBagKeys::devicecanuselayer,fDeviceCanUseLayers);
		ioBag.SetLong(V4DPicturesSettingsBagKeys::deviceprinter,fDevicePrinter);
		ioBag.SetLong(V4DPicturesSettingsBagKeys::devicecanusecleartype,fDeviceCanUseClearType);
	}
	return result;	
}

VError	VPictureDrawSettings_Base::LoadFromBag( const VValueBag& inBag)
{
	sLONG vers;
	VError result=VE_UNKNOWN_ERROR;
	if(inBag.GetLong(V4DPicturesSettingsBagKeys::vers,vers))
	{
		VLong tmplong;
		const VValueBag* tmp;
		fAlphaBlendFactor=V4DPicturesSettingsBagKeys::alpha.Get(&inBag);
		
		if(inBag.GetAttribute( V4DPicturesSettingsBagKeys::frLine, tmplong))
		{
			fLine=V4DPicturesSettingsBagKeys::frLine.Get(&inBag);
			fCol=V4DPicturesSettingsBagKeys::frCol.Get(&inBag);
		}
		else
		{
			fCol=V4DPicturesSettingsBagKeys::frH.Get(&inBag);
			fLine=V4DPicturesSettingsBagKeys::frV.Get(&inBag);
		}
		if(fCol<1)
			fCol=1;
		if(fLine<1)
			fLine=1;
		
		fSplit=V4DPicturesSettingsBagKeys::frSplit.Get(&inBag)==1;
		fDrawingMode=V4DPicturesSettingsBagKeys::drmode.Get(&inBag);

		fPicEnd_PosX=V4DPicturesSettingsBagKeys::peX.Get(&inBag);
		fPicEnd_PosY=V4DPicturesSettingsBagKeys::peY.Get(&inBag);
		fPicEnd_TransfertMode=V4DPicturesSettingsBagKeys::peMode.Get(&inBag);
		
		tmp=inBag.RetainUniqueElement(V4DPicturesSettingsBagKeys::transcol);
		
		if(tmp)
		{
			fTransparentColor.LoadFromBag(*tmp);
			tmp->Release();
		}
		tmp=inBag.RetainUniqueElement(V4DPicturesSettingsBagKeys::affine);
		if(tmp)
		{
			fPictureMatrix.LoadFromBag(*tmp);
			tmp->Release();
		}
		result=XBOX::VE_OK;
	}
	return result;
}
VError	VPictureDrawSettings_Base::SaveToBag( VValueBag& ioBag, VString& /*outKind*/) const
{
	VError result=VE_UNKNOWN_ERROR;
	if(!fPictureMatrix.IsIdentity())
	{
		VString matkind;
		VValueBag *matbag=new VValueBag();
		fPictureMatrix.SaveToBag(*matbag,matkind);
		ioBag.AddElement(V4DPicturesSettingsBagKeys::affine,matbag);
		matbag->Release();
	}	
	
	if(fAlphaBlendFactor!=100)
		ioBag.SetLong(V4DPicturesSettingsBagKeys::alpha,fAlphaBlendFactor);
	
	if(fTransparentColor!=VColor::sWhiteColor)
	{
		VString colkind;
		VValueBag *colbag=new VValueBag();
		fTransparentColor.SaveToBag(*colbag,colkind);
		ioBag.AddElement(V4DPicturesSettingsBagKeys::transcol,colbag);
		colbag->Release();
	}
	
	if(fDrawingMode!=0)
	{
		ioBag.SetLong(V4DPicturesSettingsBagKeys::drmode,fDrawingMode);
	}
	
	ioBag.SetLong(V4DPicturesSettingsBagKeys::frCol,fCol);	
	
	ioBag.SetLong(V4DPicturesSettingsBagKeys::frLine,fLine);
			
	ioBag.SetLong(V4DPicturesSettingsBagKeys::frSplit,fSplit);
	
	ioBag.SetLong(V4DPicturesSettingsBagKeys::vers,1);
	
	if(fPicEnd_PosX!=0)
		ioBag.SetLong(V4DPicturesSettingsBagKeys::peX,fPicEnd_PosX);
	if(fPicEnd_PosY!=0 || fPicEnd_TransfertMode!=0)
		ioBag.SetLong(V4DPicturesSettingsBagKeys::peY,fPicEnd_PosY);
	if(fPicEnd_TransfertMode!=0)
		ioBag.SetLong(V4DPicturesSettingsBagKeys::peMode,fPicEnd_TransfertMode);

	
	result=XBOX::VE_OK;	
	
	return result;
}
VPictureDrawSettings_Base::VPictureDrawSettings_Base(const VPicture* inPicture)
{
	if(!inPicture)
	{
		_Reset();
	}
	else
	{
		_FromVPictureDrawSettings(inPicture->GetSettings());
	}
}

bool VPictureDrawSettings_Base::IsDefaultSettings()
{
	return		fInterpolationMode == 1 
			&&	fDrawingMode==0 
			&&	fTransparentColor==VColor(0xff,0xff,0xff) 
			&&	fBackgroundColor==VColor(0xff,0xff,0xff)
			&&	fAlphaBlendFactor==100
			&&	fFrameNumber==0
			&&	fX==0
			&&	fY==0
			&&	fPictureMatrix.IsIdentity();
}


/***********************************************************************************/
void VBitmapData::_Init()
{
	fInited=0;
	fOwnBuffer=false;
	fPixBuffer=0;
	fWidth=0;
	fHeight=0;
	fBytePerRow=0;
	fPixelsFormat=VPixelFormatInvalide;
}
VBitmapData::VBitmapData(const VPictureData& inData,VPictureDrawSettings* inSet)
{
	_Init();
}
#if VERSIONWIN
VBitmapData::VBitmapData(Gdiplus::Bitmap& inBitmap,Gdiplus::Bitmap* inMask)
{
	_Init();
	
	UINT gdipformat=inBitmap.GetPixelFormat();
	if(inMask)
	{
		gdipformat=PixelFormat32bppRGB;
		fPixelsFormat=VPixelFormat32bppARGB;
	}
	else
	{
		if(Gdiplus::IsIndexedPixelFormat(gdipformat))
		{
			// tdb create a color palette
			gdipformat=PixelFormat32bppRGB;//PixelFormat8bppIndexed;
			fPixelsFormat=VPixelFormat32bppRGB;//VPixelFormat8bppIndexed;
		}
		else
		{
			if(Gdiplus::IsAlphaPixelFormat(gdipformat))
			{
				//JQ 04/08/2010: fixed Pre-alpha pixel format 
				if (gdipformat & PixelFormatPAlpha)
				{
					gdipformat=PixelFormat32bppPARGB;
					fPixelsFormat=VPixelFormat32bppPARGB;
				}
				else
				{
					gdipformat=PixelFormat32bppARGB;
					fPixelsFormat=VPixelFormat32bppARGB;
				}
			}
			else
			{
				gdipformat=PixelFormat32bppRGB;
				fPixelsFormat=VPixelFormat32bppRGB;
			}
		}
	}
	Gdiplus::BitmapData bmData;
	Gdiplus::Rect r;
	r.X=0;
	r.Y=0;
	r.Width=inBitmap.GetWidth();
	r.Height=inBitmap.GetHeight();
	
	if(inBitmap.LockBits(&r,Gdiplus::ImageLockModeRead,gdipformat,&bmData)==Gdiplus::Ok)
	{
		fWidth=bmData.Width;
		fHeight=bmData.Height;
		fBytePerRow=abs(bmData.Stride);
		
		fPixBuffer=malloc(fHeight*fBytePerRow);
		if(fPixBuffer)
		{
			if(bmData.Stride<0)
			{
				char *src,*dst;
				src=(char*)bmData.Scan0;
				dst=(char*)fPixBuffer;
				for(long i=0;i<fHeight;src-=fBytePerRow,dst+=fBytePerRow,i++)
					memcpy(dst,src,fBytePerRow);
			}
			else
				memcpy(fPixBuffer,bmData.Scan0,fHeight*fBytePerRow);
			fOwnBuffer=true;
			fInited=true;
			if(inMask)
			{
				VBitmapData mask(*inMask);
				MergeMask(mask);
			}
		}
		inBitmap.UnlockBits(&bmData);
	}
}
#elif VERSIONMAC

VBitmapData::VBitmapData(CGImageRef inCGImage,CGImageRef inMask)
{
	_Init();
	fOwnBuffer=false;
	fPixBuffer=0;
	fWidth=0;
	fHeight=0;
	fBytePerRow=0;
	fPixelsFormat=VPixelFormatInvalide;
	if(inCGImage)
	{
		CGContextRef    context = NULL;
		CGColorSpaceRef colorSpace;
		int             bitmapByteCount;
		CGImageAlphaInfo alphainfo;
		fWidth=CGImageGetWidth(inCGImage);
		fHeight=CGImageGetHeight(inCGImage);
		fBytePerRow   = ( 4 * fWidth + 15 ) & ~15;
		bitmapByteCount     = (fBytePerRow * fHeight);
		
		alphainfo=CGImageGetAlphaInfo(inCGImage);
		if(inMask)
		{
			alphainfo=kCGImageAlphaNoneSkipLast;
			fPixelsFormat=VPixelFormat32bppARGB;
		}
		else
		{
			switch(alphainfo)
			{
				case kCGImageAlphaNoneSkipLast:
				case kCGImageAlphaNoneSkipFirst:
				case kCGImageAlphaNone:
				case kCGImageAlphaOnly:
				{
					alphainfo=kCGImageAlphaNoneSkipLast;
					fPixelsFormat=VPixelFormat32bppRGB;
				}
				case kCGImageAlphaPremultipliedLast:
				case kCGImageAlphaPremultipliedFirst:
				{
					alphainfo=kCGImageAlphaPremultipliedLast;
					fPixelsFormat=VPixelFormat32bppPARGB;
					break;
				}
				case kCGImageAlphaLast:
				case kCGImageAlphaFirst:
				{
					alphainfo=kCGImageAlphaLast;
					fPixelsFormat=VPixelFormat32bppARGB;
					break;
				}
			}
		}
								
		fPixBuffer = malloc( fBytePerRow * fHeight );
		if(fPixBuffer)
		{
			colorSpace = CGColorSpaceCreateDeviceRGB();
			fOwnBuffer=true;
			context = CGBitmapContextCreate (fPixBuffer,
								fWidth,
								fHeight,
								8,      // bits per component
								fBytePerRow,
								colorSpace,
								alphainfo | kCGBitmapByteOrder32Host);
			if (context) 
			{
				CGRect vr;
				vr.origin.x= 0;
				vr.origin.y=  0;
				vr.size.width=fWidth;
				vr.size.height=fHeight;
				::CGContextClearRect(context, vr);
				CGContextDrawImage(context,vr,inCGImage);
				CGContextRelease(context);
				fInited=true;
				if(inMask)
				{
					VBitmapData mask(inMask,0);
					MergeMask(mask);
				}
				
			}
			CGColorSpaceRelease( colorSpace );	
		}
	}
}
#endif
VBitmapData::VBitmapData(void* pixbuffer,sLONG width,sLONG height,sLONG byteperrow,VBitmapData::VPixelFormat pixelsformat,std::vector<VColor> *inColortable)
{
	_Init();
	if(pixbuffer)
	{
		fWidth=width;
		fHeight=height;
		fBytePerRow=byteperrow;
		fPixelsFormat=pixelsformat;
		
		fPixBuffer=malloc(fBytePerRow*fHeight);
		if(fPixBuffer)
		{
			memcpy(fPixBuffer,pixbuffer,fBytePerRow*fHeight);
		}
		if(inColortable)
		{
			fColorTable=*inColortable;
		}
		fOwnBuffer=true;
	}
	fInited=fPixBuffer!=0;
}
VBitmapData::VBitmapData(sLONG width,sLONG height,VBitmapData::VPixelFormat  pixelsformat,std::vector<VColor> *inColortable, bool inClearColorTransparent)
{
	_Init();
	if(_AllocateBuffer(width,height,pixelsformat,inClearColorTransparent))
	{
		if(inColortable)
		{
			fColorTable=*inColortable;
		}
		fInited=true;
	}
	else
		fInited=false;
}
VBitmapData::~VBitmapData()
{
	if(fOwnBuffer && fInited && fPixBuffer)
		free(fPixBuffer);
}

bool VBitmapData::_AllocateBuffer(sLONG inWidth,sLONG inHeight,VBitmapData::VPixelFormat inFormat, bool inClearColorTransparent)
{
	assert(!fInited);
	fPixBuffer=0;
	if(inHeight>0 && inWidth>0)
	{
		fWidth=inWidth;
		fHeight=inHeight;
		fPixelsFormat=inFormat;
		GReal r= GetPixelFormatSize()/(GReal) 8;
		uLONG l= (r*fWidth)+15;
		fBytePerRow= l & ~15;
	
		fPixBuffer=malloc(fBytePerRow*fHeight);
		if (inClearColorTransparent)
			memset(fPixBuffer,0,fBytePerRow*fHeight);
		else
			memset(fPixBuffer,0xff,fBytePerRow*fHeight);
		fOwnBuffer=true;
	}
	return fPixBuffer!=0;
}
const VColor& VBitmapData::GetIndexedColor(sLONG inIndex)
{
	assert(IsIndexedPixelFormat());
	if(inIndex<(sLONG)fColorTable.size())
	{
		return fColorTable[inIndex];
	}
	else
		return fFakeColor;
}
void VBitmapData::MergeMask(VBitmapData& inMask)
{
	if(fInited && inMask.fInited && GetPixelFormatSize()==32)
	{
		if(GetWidth() == inMask.GetWidth() && GetHeight() == inMask.GetHeight())
		{
			if(inMask.GetPixelFormatSize()==32)
			{
				for(long i=0;i<GetHeight();i++)
				{
					_ARGBPix* pixbuf=(_ARGBPix*)GetLinePtr(i);
					_ARGBPix* maskbuf=(_ARGBPix*)inMask.GetLinePtr(i);
					for(long ii=0;ii<GetWidth();ii++,pixbuf++,maskbuf++)
					{
						sWORD alpha=255-maskbuf->variant.col.R;
						pixbuf->variant.col.A=alpha;
						if(IsPreAlphaPixelFormat())
						{
							pixbuf->variant.col.R=(pixbuf->variant.col.R * alpha)/255;
							pixbuf->variant.col.G=(pixbuf->variant.col.G * alpha )/255;
							pixbuf->variant.col.B=(pixbuf->variant.col.B * alpha )/255;
						}
					}
				}
			}
			else
			{
				VColor col;
				for(long i=0;i<GetHeight();i++)
				{
					_ARGBPix* pixbuf=(_ARGBPix*)GetLinePtr(i);
					unsigned char* maskbuf=(unsigned char*)inMask.GetLinePtr(i);
					for(long ii=0;ii<GetWidth();ii++,pixbuf++,maskbuf++)
					{
						col=inMask.GetIndexedColor(*maskbuf);
						sWORD alpha=255-col.GetRed();
						pixbuf->variant.col.A=alpha;
						if(IsPreAlphaPixelFormat())
						{
							pixbuf->variant.col.R=(pixbuf->variant.col.R * alpha)/255;
							pixbuf->variant.col.G=(pixbuf->variant.col.G * alpha )/255;
							pixbuf->variant.col.B=(pixbuf->variant.col.B * alpha )/255;
						}
					}
				}
			}
		}
	}
}


/** compare this bitmap data with the input bitmap data
@param inBmpData
	the input bitmap data to compare with
@outMask
	if not NULL, *outMask will contain a bitmap data where different pixels (pixels not equal in both pictures) have alpha = 0.0f
	and equal pixels have alpha = 1.0f
@remarks
	return true if bitmap datas are equal, false otherwise

	if the bitmap datas have not the same width or height, return false & optionally set *outMask to NULL
*/
bool VBitmapData::Compare( const VBitmapData *inBmpData, VBitmapData **outMask) const
{
	if (GetPixelFormatSize() != 32 || inBmpData->GetPixelFormatSize() != 32)
	{
		if (outMask)
			*outMask = NULL;
		return false;
	}

	xbox_assert(fInited && inBmpData->fInited);
	xbox_assert(inBmpData);
	sLONG width = GetWidth();
	sLONG height = GetHeight();
	if (width != inBmpData->GetWidth() || height != inBmpData->GetHeight())
	{
		if (outMask)
			*outMask = NULL;
		return false;
	}
	if (!width || !height)
	{
		if (outMask)
			*outMask = new VBitmapData( *this);
		return true;
	}

	if (outMask)
	{
		*outMask = new VBitmapData(width, height, VBitmapData::VPixelFormat32bppARGB);
		if (*outMask == NULL)
			outMask = NULL;
	}

	_ARGBPix colorEqual;
	colorEqual.variant.col.R = colorEqual.variant.col.G = colorEqual.variant.col.B = 0;
	colorEqual.variant.col.A = 0xff;

	_ARGBPix colorNotEqual;
	colorNotEqual.variant.col.R = colorEqual.variant.col.G = colorEqual.variant.col.B = 0;
	colorNotEqual.variant.col.A = 0;

	bool isEqual = true;
	if (outMask)
		for(long i=0;i<GetHeight();i++)
		{
			_ARGBPix* pixbuf=(_ARGBPix*)GetLinePtr(i);
			_ARGBPix* pixbuf2=(_ARGBPix*)inBmpData->GetLinePtr(i);

			_ARGBPix* maskbuf= (_ARGBPix*)(*outMask)->GetLinePtr(i);

			for(long ii = 0; ii < GetWidth(); ii++, pixbuf++, pixbuf2++, maskbuf++)
			{
				if (pixbuf->variant.ARGB == pixbuf2->variant.ARGB)
					*maskbuf = colorEqual;
				else
				{
					*maskbuf = colorNotEqual;
					isEqual = false;
				}
			}
		}
	else
		for(long i=0;i<GetHeight();i++)
		{
			_ARGBPix* pixbuf=(_ARGBPix*)GetLinePtr(i);
			_ARGBPix* pixbuf2=(_ARGBPix*)inBmpData->GetLinePtr(i);
			for(long ii = 0; ii < GetWidth(); ii++, pixbuf++, pixbuf2++)
			{
				if (pixbuf->variant.ARGB != pixbuf2->variant.ARGB)
				{
					isEqual = false;
					break;
				}
			}
		}
	return isEqual;
}


void VBitmapData::ApplyColor(const VColor &inColor)
{
	sWORD Hue,Saturation,Luminosity;
	sWORD tHue,tSaturation,tLuminosity;
	VColor c;
	inColor.GetHSL (Hue,Saturation,Luminosity);
	c.FromHSL(Hue,Saturation,Luminosity);
	if(!fInited)
		return;
	if(IsIndexedPixelFormat())
	{
		for(size_t i=1;i<=fColorTable.size();i++)
		{
			fColorTable[i].GetHSL (tHue,tSaturation,tLuminosity);
			fColorTable[i].FromHSL(Hue,Saturation,tLuminosity);
		}
	}
	else
	{
		for(long i=0;i<GetHeight();i++)
		{
			_ARGBPix* pixbuf=(_ARGBPix*)GetLinePtr(i);
			for(long ii=0;ii<GetWidth();ii++,pixbuf++)
			{
				if(!IsPreAlphaPixelFormat())
				{
					c.FromRGBAColor(pixbuf->variant.col.R,pixbuf->variant.col.G,pixbuf->variant.col.B);
					c.GetHSL (tHue,tSaturation,tLuminosity);
					c.FromHSL(Hue,Saturation,tLuminosity);
					pixbuf->variant.col.R=c.GetRed();
					pixbuf->variant.col.G=c.GetGreen();
					pixbuf->variant.col.B=c.GetBlue();
				}
				else
				{
					_ARGBPix tmppix=*pixbuf;
					if(tmppix.variant.col.A==0)
					{
						tmppix.variant.col.R=tmppix.variant.col.G=tmppix.variant.col.B=0;
					}
					else
					{
						tmppix.variant.col.R= (tmppix.variant.col.R*255)/tmppix.variant.col.A;
						tmppix.variant.col.G= (tmppix.variant.col.G*255)/tmppix.variant.col.A;
						tmppix.variant.col.B= (tmppix.variant.col.B*255)/tmppix.variant.col.A;
					}
					
					c.FromRGBAColor(tmppix.variant.col.R,tmppix.variant.col.G,tmppix.variant.col.B);
					c.GetHSL (tHue,tSaturation,tLuminosity);
					c.FromHSL(Hue,Saturation,tLuminosity);
					
					pixbuf->variant.col.R=(c.GetRed()*pixbuf->variant.col.A)/255;
					pixbuf->variant.col.G=(c.GetGreen()*pixbuf->variant.col.A)/255;;
					pixbuf->variant.col.B=(c.GetBlue()*pixbuf->variant.col.A)/255;;
				}
			}
		}
	}
}

void VBitmapData::Clear(const VColor &inColor)
{
	if(!fInited)
		return;
	if(IsIndexedPixelFormat())
	{
		//fill color table
		for(size_t i=1;i<=fColorTable.size();i++)
			fColorTable[i] = inColor;
	}
	else
	{
		//compute new pixel color value
		_ARGBPix color;
		if(!IsPreAlphaPixelFormat())
		{
			color.variant.col.R=inColor.GetRed();
			color.variant.col.G=inColor.GetGreen();
			color.variant.col.B=inColor.GetBlue();

			if (IsAlphaPixelFormat())
				color.variant.col.A = inColor.GetAlpha();
			else
				color.variant.col.A = (uBYTE)255;
		}
		else
		{
			color.variant.col.R=(inColor.GetRed()*inColor.GetAlpha())/255;
			color.variant.col.G=(inColor.GetGreen()*inColor.GetAlpha())/255;
			color.variant.col.B=(inColor.GetBlue()*inColor.GetAlpha())/255;
			color.variant.col.A=inColor.GetAlpha();
		}

		for(long i=0;i<GetHeight();i++)
		{
			_ARGBPix* pixbuf=(_ARGBPix*)GetLinePtr(i);
			for(long ii=0;ii<GetWidth();ii++,pixbuf++)
			{
				*pixbuf = color;
			}
		}
	}
}
VBitmapData* VBitmapData::MakeTransparent(const VColor& inTransparentColor)
{
	long i,ii;
	VBitmapData* result=NULL;
	_ARGBPix* pixbuf=NULL;
	_ARGBPix* destbuf=NULL;
	_ARGBPix trans={0};
	
	uBYTE r=inTransparentColor.GetRed();
	uBYTE g=inTransparentColor.GetGreen();
	uBYTE b=inTransparentColor.GetBlue();
	uBYTE a=0xff;

	if(IsIndexedPixelFormat() || !IsAlphaPixelFormat())
	{
		result = new VBitmapData(GetWidth(),GetHeight(),VPixelFormat32bppARGB);
		if(result)
		{		
			
			if(IsIndexedPixelFormat())
			{
				unsigned char* src;
			
				for(i=0;i<GetHeight();i++)
				{
					src=(unsigned char*)GetLinePtr(i);
					destbuf=(_ARGBPix*)result->GetLinePtr(i);
				
					for(ii=0;ii<GetWidth();ii++,src++,destbuf++)
					{
						if(fColorTable[*src]==inTransparentColor)
						{
							*destbuf = trans;
						}
						else
						{
							destbuf->variant.col.R = fColorTable[*src].GetRed();
							destbuf->variant.col.G = fColorTable[*src].GetGreen();
							destbuf->variant.col.B = fColorTable[*src].GetBlue();
							destbuf->variant.col.A = 0xff;
						}
					
					}
				}
			}
			else if(!IsAlphaPixelFormat())
			{
				for(i=0;i<GetHeight();i++)
				{
					pixbuf=(_ARGBPix*)GetLinePtr(i);
					destbuf=(_ARGBPix*)result->GetLinePtr(i);
					for(ii=0;ii<GetWidth();ii++,pixbuf++,destbuf++)
					{
						if(pixbuf->variant.col.R==r && pixbuf->variant.col.G==g && pixbuf->variant.col.B==b)
						{
							*destbuf = trans;
						}
						else
						{
							*destbuf = *pixbuf;
							destbuf->variant.col.A = 0xff;
						}
					}
				}
			}
		}
	}
	else
	{
		if(!IsPreAlphaPixelFormat())
        {
            result = new VBitmapData(GetWidth(),GetHeight(),VPixelFormat32bppARGB);
            if(result)
            {
                for(i=0;i<GetHeight();i++)
                {
                    pixbuf=(_ARGBPix*)GetLinePtr(i);
                    destbuf=(_ARGBPix*)result->GetLinePtr(i);
                    for(ii=0;ii<GetWidth();ii++,pixbuf++,destbuf++)
                    {
                       
                        if(pixbuf->variant.col.R==r && pixbuf->variant.col.G==g && pixbuf->variant.col.B==b && pixbuf->variant.col.A==a)
                        {
                               *destbuf = trans;
                        }
                        else
                        {
                               *destbuf = *pixbuf;
                        }
                    }
                }
            }
        }
        else
        {
            if(a!=0)
            {
                r= (r*a)/255;
                g= (g*a)/255;
                b= (b*a)/255;
            }
            result = new VBitmapData(GetWidth(),GetHeight(),VPixelFormat32bppPARGB);
            if(result)
            {
                for(i=0;i<GetHeight();i++)
                {
                    pixbuf=(_ARGBPix*)GetLinePtr(i);
                    destbuf=(_ARGBPix*)result->GetLinePtr(i);
                    for(ii=0;ii<GetWidth();ii++,pixbuf++,destbuf++)
                    {
                        
                        _ARGBPix tmppix=*pixbuf;
						
						if(tmppix.variant.col.R==r && tmppix.variant.col.G==g && tmppix.variant.col.B==b && tmppix.variant.col.A==a)
						{
							*destbuf = trans;
						}
						else
						{
							*destbuf = tmppix;
						}

                    }
                }
            }

        }
        
	}
	return result;
}
void VBitmapData::ConvertToGray()
{
	if(!fInited)
		return;
	if(IsIndexedPixelFormat())
	{
		VColor c;
		for(size_t i=1;i<=fColorTable.size();i++)
		{
			fColorTable[i].ToGrey();
		}
	}
	else
	{
		for(long i=0;i<GetHeight();i++)
		{
			_ARGBPix* pixbuf=(_ARGBPix*)GetLinePtr(i);
			for(long ii=0;ii<GetWidth();ii++,pixbuf++)
			{
				if(!IsPreAlphaPixelFormat())
				{
					if(pixbuf->variant.col.R != pixbuf->variant.col.G || pixbuf->variant.col.G!=pixbuf->variant.col.B)
					{
						uBYTE grey=(uBYTE)(0.299 * ((double) pixbuf->variant.col.R) + 0.587 * ((double) pixbuf->variant.col.G) + 0.114 * ((double) pixbuf->variant.col.B));
						pixbuf->variant.col.R=grey;
						pixbuf->variant.col.G=grey;
						pixbuf->variant.col.B=grey;
					}
				}
				else
				{
					_ARGBPix tmppix=*pixbuf;
					if(tmppix.variant.col.A)
					{
						tmppix.variant.col.R= (tmppix.variant.col.R*255)/tmppix.variant.col.A;
						tmppix.variant.col.G= (tmppix.variant.col.G*255)/tmppix.variant.col.A;
						tmppix.variant.col.B= (tmppix.variant.col.B*255)/tmppix.variant.col.A;
					}
					if(tmppix.variant.col.R != tmppix.variant.col.G || tmppix.variant.col.G!=tmppix.variant.col.B)
					{
						uBYTE grey=(uBYTE)(0.299 * ((double) tmppix.variant.col.R) + 0.587 * ((double) tmppix.variant.col.G) + 0.114 * ((double) tmppix.variant.col.B));
						grey=(grey*pixbuf->variant.col.A)/255;
						pixbuf->variant.col.R=grey;
						pixbuf->variant.col.G=grey;
						pixbuf->variant.col.B=grey;
					}
				}
			}
		}
	}
}

#if VERSIONWIN
Gdiplus::Bitmap* VBitmapData::CreateNativeBitmap()
{
	Gdiplus::Bitmap* result;
	Gdiplus::Rect r(0,0,GetWidth(),GetHeight());
	if(!fInited)
		return NULL;
	if(IsIndexedPixelFormat())
	{
		Gdiplus::Bitmap bm(GetWidth(),GetHeight(),GetRowByte(),PixelFormat8bppIndexed,(BYTE*)GetPixelBuffer());
		result=bm.Clone(0,0,bm.GetWidth(),bm.GetHeight(),bm.GetPixelFormat());
		if(result)
		{
			// IMPORTANT il faut ouvrir la bitmap en mode write pour forcer gdiplus a dupliqué le buffer
			// sans le clone pointe tj vers le buffer du VBitmapData
			Gdiplus::BitmapData bmData;
			result->LockBits(&r,Gdiplus::ImageLockModeWrite,bm.GetPixelFormat(),&bmData);
			result->UnlockBits(&bmData);
			
			// build the color table
			Gdiplus::ColorPalette* pal;
			pal=(Gdiplus::ColorPalette*)malloc(sizeof(Gdiplus::ColorPalette)+fColorTable.size()-1);
			if(pal)
			{
				pal->Flags=0;
				pal->Count=(UINT)fColorTable.size();
				for(size_t i=1;i<=fColorTable.size();i++)
				{
					pal->Entries[i]=fColorTable[i].GetRGBAColor();
				}
				result->SetPalette(pal);
				free((void*)pal);
			}
		}
	}
	else
	{
		Gdiplus::PixelFormat gdipformat;
		if(IsPreAlphaPixelFormat())
			gdipformat=PixelFormat32bppPARGB;
		else if(!IsAlphaPixelFormat())
			gdipformat=PixelFormat32bppRGB;
		else
			gdipformat=PixelFormat32bppARGB;
		
		Gdiplus::Bitmap bm(GetWidth(),GetHeight(),GetRowByte(),gdipformat,(BYTE*)GetPixelBuffer());
		result=bm.Clone(0,0,bm.GetWidth(),bm.GetHeight(),bm.GetPixelFormat());
		if(result)
		{
			// IMPORTANT il faut ouvrir la bitmap en mode write pour forcer gdiplus a dupliqué le buffer
			// sans le clone pointe tj vers le buffer du VBitmapData
			Gdiplus::BitmapData bmData;
			result->LockBits(&r,Gdiplus::ImageLockModeWrite,bm.GetPixelFormat(),&bmData);
			result->UnlockBits(&bmData);
		}
	}
	return result;
}
#elif VERSIONMAC
CGImageRef VBitmapData::CreateNativeBitmap()
{
	CGImageRef result=0;
	CGColorSpaceRef baseColorSpace = CGColorSpaceCreateDeviceRGB();
	CGDataProviderRef dataprov= 0;
	switch(fPixelsFormat)
	{
		case VPixelFormat8bppIndexed:
		{
			#if _WIN32 || __GNUC__
			#pragma pack(push,1)
			#else
			#pragma options align = power 
			#endif
			typedef struct xRGB
			{
				uBYTE r;
				uBYTE g;
				uBYTE b;
			}xRGB;
			#if _WIN32 || __GNUC__
			#pragma pack(pop)
			#else
			#pragma options align = reset
			#endif
			
			CGColorSpaceRef indColorSpace=0;
			// build the color table
			xRGB* pal;
			pal=(xRGB*)malloc(sizeof(xRGB)+fColorTable.size());
			if(pal)
			{
				for(long i=1;i<=fColorTable.size();i++)
				{
					pal[i].r=fColorTable[i].GetRed();
					pal[i].g=fColorTable[i].GetGreen();
					pal[i].b=fColorTable[i].GetBlue();
				}
				indColorSpace = CGColorSpaceCreateIndexed(baseColorSpace,fColorTable.size(),(uBYTE*)pal);
				free((void*)pal);
				
				dataprov= xV4DPicture_MemoryDataProvider::CGDataProviderCreate((char*)GetPixelBuffer(),GetHeight()*GetRowByte(),true);
				
				result=CGImageCreate(GetWidth(),GetHeight(),8,8,GetRowByte(),indColorSpace,kCGImageAlphaNone,dataprov,0,0,kCGRenderingIntentDefault);
				
			}
			break;
		}
		case VPixelFormat32bppRGB:
		{
			dataprov= xV4DPicture_MemoryDataProvider::CGDataProviderCreate((char*)GetPixelBuffer(),GetHeight()*GetRowByte(),true);
			result=CGImageCreate(GetWidth(),GetHeight(),8,32,GetRowByte(),baseColorSpace,kCGImageAlphaNone | kCGBitmapByteOrder32Host,dataprov,0,0,kCGRenderingIntentDefault);
			break;
		}
		case VPixelFormat32bppARGB:
		{
			dataprov= xV4DPicture_MemoryDataProvider::CGDataProviderCreate((char*)GetPixelBuffer(),GetHeight()*GetRowByte(),true);
			result=CGImageCreate(GetWidth(),GetHeight(),8,32,GetRowByte(),baseColorSpace,kCGImageAlphaLast | kCGBitmapByteOrder32Host,dataprov,0,0,kCGRenderingIntentDefault);
			break;
		}
		case VPixelFormat32bppPARGB:
		{
			dataprov= xV4DPicture_MemoryDataProvider::CGDataProviderCreate((char*)GetPixelBuffer(),GetHeight()*GetRowByte(),true);
			result=CGImageCreate(GetWidth(),GetHeight(),8,32,GetRowByte(),baseColorSpace,kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Host,dataprov,0,0,kCGRenderingIntentDefault);
			break;
		}
		
	}
	if(dataprov)
		CGDataProviderRelease(dataprov);
	CGColorSpaceRelease(baseColorSpace);
	return result;
}

//create a bitmap graphic context
//@remark: caller is context owner
//		   caller must not release VBitmapData before the context
//		   because context uses VBitmapData pixel buffer
CGContextRef VBitmapData::CreateBitmapContext()
{
	CGBitmapInfo bmpInfo = 0;
	switch(fPixelsFormat)
	{
		case VPixelFormat8bppIndexed:
		{
			return NULL;
			break;
		}
		case VPixelFormat32bppRGB:
		{
			bmpInfo = kCGImageAlphaNone | kCGBitmapByteOrder32Host;
			break;
		}
		case VPixelFormat32bppARGB:
		{
			bmpInfo = kCGImageAlphaLast | kCGBitmapByteOrder32Host;
			break;
		}
		case VPixelFormat32bppPARGB:
		{
			bmpInfo = kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Host;
			break;
		}
		default:
			xbox_assert(false);
			break;
	}
	
	
	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
	
	CGContextRef context = CGBitmapContextCreate (fPixBuffer,
									 fWidth,
									 fHeight,
									 8,      // bits per component
									 fBytePerRow,
									 colorSpace,
									 bmpInfo);
	xbox_assert(context);
	
	CGColorSpaceRelease(colorSpace);
	
	return (context);
}


#endif

VPictureData* VBitmapData::CreatePictureData()
{
	VPictureData* result=0;
	if(fInited)
	{
		#if VERSIONWIN
		Gdiplus::Bitmap* bm=CreateNativeBitmap();
		if(bm)
		{
			result=new VPictureData_GDIPlus(bm,true);
		}
		#elif VERSIONMAC
		CGImageRef bm=CreateNativeBitmap();
		if(bm)
		{
			result=new VPictureData_CGImage(bm);
			CFRelease(bm);
		}
		#endif
	}
	return result;
}

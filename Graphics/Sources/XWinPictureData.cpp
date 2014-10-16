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
#include "Kernel/VKernel.h"

#include <vector>
#include "V4DPictureIncludeBase.h"
#if ENABLE_D2D
#include "XWinD2DGraphicContext.h"
#endif



class xgdiplusbitmap : public Gdiplus::Bitmap
{
	public:
	xgdiplusbitmap(HBITMAP hbm,HPALETTE hpal)
	:Bitmap(hbm,hpal)
	{
		fBM=hbm;
	}
	virtual ~xgdiplusbitmap()
	{
		if(fBM)
			DeleteObject(fBM);
	}
	private:
	HBITMAP fBM;
};

 
/*********************************************************************************/

xGifInfo_GDIPlus::xGifInfo_GDIPlus(Gdiplus::Bitmap& inPicture)
:xGifInfo()
{
	xGifFrame* fr;
	sLONG nbframe=inPicture.GetFrameCount(&Gdiplus::FrameDimensionTime);
	long propsize;
	Gdiplus::PropertyItem* prop=0;
	
	fLoopCount=0;
	for(long i=0;i<nbframe;i++)
	{
		
		xGif89FrameInfo inInfo;
		sLONG framedelay;
		sBYTE transindex;
		
		inInfo.transparent=0;
		inInfo.delayTime=0;
		inInfo.inputFlag=0;
		inInfo.disposal=0;
		inInfo.transparent_index=0;
		
		
		inPicture.SelectActiveFrame(&Gdiplus::FrameDimensionTime,i);
		fr=new xGifFrame(0,0,inPicture.GetWidth(),inPicture.GetHeight(),false);
		
		
		if(propsize=inPicture.GetPropertyItemSize(PropertyTagFrameDelay))
		{
			prop=(Gdiplus::PropertyItem*)malloc(propsize);
			if(inPicture.GetPropertyItem(PropertyTagFrameDelay,propsize,prop)==Gdiplus::Ok)
			{
				inInfo.delayTime=*((long*)prop->value);
			}
			free((void*)prop);
		}
		if(propsize=inPicture.GetPropertyItemSize(0x5104))
		{
			prop=(Gdiplus::PropertyItem*)malloc(propsize);
			if(inPicture.GetPropertyItem(0x5104,propsize,prop)==Gdiplus::Ok)
			{
				inInfo.transparent_index=*((sBYTE*)prop->value);
			}
			free((void*)prop);
		}
		
		inInfo.transparent=inInfo.transparent_index!=-1;
		
		fr->SetGif89FrameInfo(inInfo);
		fFrameList.push_back(fr);
	}
	if(propsize=inPicture.GetPropertyItemSize(PropertyTagLoopCount))
	{
		prop=(Gdiplus::PropertyItem*)malloc(propsize);
		if(inPicture.GetPropertyItem(PropertyTagLoopCount,propsize,prop)==Gdiplus::Ok)
		{
			fLoopCount=*((short*)prop->value);
		}
		free((void*)prop);
	}
}


xGifInfo_GDIPlus::~xGifInfo_GDIPlus()
{
	
}
	


VPictureData_GDIBitmap::VPictureData_GDIBitmap()
:VPictureData_Bitmap()
{
	_Init();
}
VPictureData_GDIBitmap::VPictureData_GDIBitmap(VPictureDataProvider* inDataSource,_VPictureAccumulator* inRecorder)
:VPictureData_Bitmap(inDataSource,inRecorder)
{
	_Init();
}
VPictureData_GDIBitmap::VPictureData_GDIBitmap(const VPictureData_GDIBitmap& inData)
:VPictureData_Bitmap(inData)
{
	_Init();
}

VPictureData_GDIBitmap::~VPictureData_GDIBitmap()
{
	if(fDataProvider)
		fDataProvider->EndDirectAccess();
}

void VPictureData_GDIBitmap::_Init()
{
	fDIBInfo=0;
	fDIBData=0;
	fBitmap=NULL;
}
void VPictureData_GDIBitmap::_DoReset()const
{
	
}
void VPictureData_GDIBitmap::_DoLoad()const
{
	if(fDataProvider)
	{
		BITMAPINFO* bm=(BITMAPINFO*)fDataProvider->BeginDirectAccess();
		_InitDIBData(bm);
	}
}

PBITMAPINFO CreateBitmapInfoStruct(HBITMAP hBmp)
{ 
    BITMAP bmp; 
    PBITMAPINFO pbmi; 
    WORD    cClrBits; 

    // Retrieve the bitmap color format, width, and height. 
    if (!GetObject(hBmp, sizeof(BITMAP), (LPSTR)&bmp)) 
        return NULL;

    // Convert the color format to a count of bits. 
    cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel); 
    if (cClrBits == 1) 
        cClrBits = 1; 
    else if (cClrBits <= 4) 
        cClrBits = 4; 
    else if (cClrBits <= 8) 
        cClrBits = 8; 
    else if (cClrBits <= 16) 
        cClrBits = 16; 
    else if (cClrBits <= 24) 
        cClrBits = 24; 
    else cClrBits = 32; 

    // Allocate memory for the BITMAPINFO structure. (This structure 
    // contains a BITMAPINFOHEADER structure and an array of RGBQUAD 
    // data structures.) 

     if (cClrBits != 24) 
         pbmi = (PBITMAPINFO) LocalAlloc(LPTR, 
                    sizeof(BITMAPINFOHEADER) + 
                    sizeof(RGBQUAD) * (size_t) (1<< cClrBits)); 

     // There is no RGBQUAD array for the 24-bit-per-pixel format. 

     else 
         pbmi = (PBITMAPINFO) LocalAlloc(LPTR, 
                    sizeof(BITMAPINFOHEADER)); 

    // Initialize the fields in the BITMAPINFO structure. 

    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER); 
    pbmi->bmiHeader.biWidth = bmp.bmWidth; 
    pbmi->bmiHeader.biHeight = bmp.bmHeight; 
    pbmi->bmiHeader.biPlanes = bmp.bmPlanes; 
    pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel; 
    if (cClrBits < 24) 
        pbmi->bmiHeader.biClrUsed = (1<<cClrBits); 

    // If the bitmap is not compressed, set the BI_RGB flag. 
    pbmi->bmiHeader.biCompression = BI_RGB; 

    // Compute the number of bytes in the array of color 
    // indices and store the result in biSizeImage. 
    // For Windows NT, the width must be DWORD aligned unless 
    // the bitmap is RLE compressed. This example shows this. 
    // For Windows 95/98/Me, the width must be WORD aligned unless the 
    // bitmap is RLE compressed.
    pbmi->bmiHeader.biSizeImage = ((pbmi->bmiHeader.biWidth * cClrBits +31) & ~31) /8
                                  * pbmi->bmiHeader.biHeight; 
    // Set biClrImportant to 0, indicating that all of the 
    // device colors are important. 
     pbmi->bmiHeader.biClrImportant = 0; 
     
     return pbmi; 
 }


VError VPictureData_GDIBitmap::Save(VBlob* inData,VIndex inOffset,VSize& outSize,_VPictureAccumulator* /*inRecorder*/)const
{
	if(fDataProvider)
	{
		return inherited::Save(inData,inOffset,outSize);
	}
	else
	{
		if(fBitmap)
		{
			DIBSECTION sec;
			if(GetObject(fBitmap,sizeof(DIBSECTION),&sec))
			{
			} 
			else
			{
				BITMAP bm;
				if(GetObject(fBitmap,sizeof(BITMAP),&bm))
				{
					
				}
			}
		}
	}
	return VE_UNIMPLEMENTED;
}
void VPictureData_GDIBitmap::DrawInPortRef(PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet)const
{
	xDraw(GetHBitmap(),inPortRef,r,inSet);
}
#if ENABLE_D2D
void VPictureData_GDIBitmap::DrawInD2DGraphicContext(VWinD2DGraphicContext* inDC,const VRect& r,VPictureDrawSettings* inSet)const
{
	Gdiplus::Bitmap bm(GetHBitmap(),0);
	xDraw(&bm,inDC,r,inSet,false);
}
#endif
void VPictureData_GDIBitmap::DrawInGDIPlusGraphics(Gdiplus::Graphics* inDC,const VRect& r,VPictureDrawSettings* inSet)const
{
	xDraw(GetHBitmap(),inDC,r,inSet);
}
VPictureData_GDIBitmap::VPictureData_GDIBitmap(HBITMAP inBitmap)
{
	_Init();
	fBitmap=inBitmap;
	if(fBitmap)
	{
		BITMAP bm;
		::GetObject(fBitmap,sizeof(BITMAP),&bm);
		fBounds.SetCoords(0,0,bm.bmWidth,bm.bmHeight);
	}
	_SetSafe();
}

VPictureData* VPictureData_GDIBitmap::Clone()const
{
	return new VPictureData_GDIBitmap(*this);
}

void VPictureData_GDIBitmap::_InitDIBData(BITMAPINFO* inInfo)const
{
	fDIBInfo=inInfo;
	long palettesize=0;
	if(!fDIBInfo)
		return;
	if(fDIBInfo->bmiHeader.biClrUsed)
	{
		palettesize = fDIBInfo->bmiHeader.biClrUsed*sizeof(RGBQUAD);
	}
	else
	{
		switch(fDIBInfo->bmiHeader.biBitCount)
		{
			case 1:
				palettesize = 2*sizeof(RGBQUAD);
				break;
			case 4:
				palettesize = 16*sizeof(RGBQUAD);
				break;
			case 8:
				palettesize = 256*sizeof(RGBQUAD);
				break;
			case 16:
			case 32:
				if(fDIBInfo->bmiHeader.biCompression==BI_RGB)
					palettesize = 0;
				else
					palettesize = 3*sizeof(DWORD);
				break;
			case 24:
				palettesize = 0;
				break;
		}
	}
	fDIBData = (void*)( ((char*)fDIBInfo) + fDIBInfo->bmiHeader.biSize + palettesize );
	fBounds.SetCoords(0,0,fDIBInfo->bmiHeader.biWidth,fDIBInfo->bmiHeader.biHeight);
}

HBITMAP			VPictureData_GDIBitmap::GetHBitmap()const
{
	if(fBitmap==NULL && fDIBInfo)
	{
		HDC refdc=GetDC(NULL);
		fBitmap=CreateDIBitmap(refdc,&fDIBInfo->bmiHeader,CBM_INIT,fDIBData,fDIBInfo,DIB_RGB_COLORS);
		ReleaseDC(NULL,refdc);
	}
	return fBitmap;
}

/*
VPictureData* VPictureData_GDIBitmap::BuildThumbnail(sLONG inWidth,sLONG inHeight,PictureMosaic inMode,VPictureDrawSettings* inSet)
{
	assert(false);
	return NULL;
}
*/
/**************************************************************************************************/

/*********************************************************************************/


VPictureData_GDIPlus::VPictureData_GDIPlus()
:VPictureData_Bitmap()
{
	_Init();
}
VPictureData_GDIPlus::VPictureData_GDIPlus(HICON inIcon,sLONG inWantedSize)
{
	_Init();
	ICONINFO info;
	if(GetIconInfo(inIcon,&info))
	{
		BITMAP bitmapData;
		GetObject(info.hbmColor, sizeof(BITMAP), &bitmapData); 
		if (bitmapData.bmBitsPixel!=32) 
        { 
           fBitmap=new Gdiplus::Bitmap(inIcon);
        } 
        else
        {
			Gdiplus::Bitmap tmpbm(info.hbmColor,NULL);
			Gdiplus::Bitmap maskbm(info.hbmMask,NULL);
			VBitmapData bmdata(tmpbm);
			VBitmapData maskdata(maskbm);
			bool alphafound=false;
			for(long i=0;i<bmdata.GetHeight() && !alphafound ;i++)
			{
				VBitmapData::_ARGBPix* pixbuf=(VBitmapData::_ARGBPix*)bmdata.GetLinePtr(i);
				for(long ii=0;ii<bmdata.GetWidth();ii++,pixbuf++)
				{
					if(pixbuf->variant.col.A!=0)
					{
						alphafound=true;
						break;
					}
				}
			}
			if(!alphafound)
			{
				for(long i=0;i<bmdata.GetHeight() ;i++)
				{
					VBitmapData::_ARGBPix* pixbuf=(VBitmapData::_ARGBPix*)bmdata.GetLinePtr(i);
					VBitmapData::_ARGBPix* maskbuf=(VBitmapData::_ARGBPix*)maskdata.GetLinePtr(i);
					for(long ii=0;ii<bmdata.GetWidth();ii++,pixbuf++,maskbuf++)
					{
						pixbuf->variant.col.A=255-maskbuf->variant.col.R;
					}
				}
			}
			VBitmapData alphabm(bmdata.GetPixelBuffer(),bmdata.GetWidth(),bmdata.GetHeight(),bmdata.GetRowByte(),VBitmapData::VPixelFormat32bppARGB);
			fBitmap=alphabm.CreateNativeBitmap();
        }
		if(info.hbmColor)
			DeleteObject(info.hbmColor); 
		if(info.hbmMask)
			DeleteObject(info.hbmMask); 
	}
	
	/*
	Gdiplus::Bitmap tmpbm(inIcon);
	if(tmpbm.GetLastStatus()==Gdiplus::Ok)
	{
		fBitmap=new Gdiplus::Bitmap(tmpbm.GetWidth(),tmpbm.GetHeight(),PixelFormat32bppPARGB);
		Gdiplus::Graphics graph(fBitmap);
		HDC dc=graph.GetHDC();
		DrawIconEx( dc,0,0,inIcon,tmpbm.GetWidth(),tmpbm.GetHeight(),0,NULL,DI_NORMAL);
		graph.ReleaseHDC(dc);
	}
	
	if(fBitmap->GetLastStatus()==Gdiplus::Ok)
	{
		fBounds.SetCoords(0,0,fBitmap->GetWidth(),fBitmap->GetHeight());
	}
	else
	{
		fBounds.SetCoords(0,0,0,0);
	}
	*/
	if(fBitmap && fBitmap->GetLastStatus()==Gdiplus::Ok)
	{
		if(inWantedSize!=0)
		{
			if(inWantedSize!=fBitmap->GetWidth())
			{
				Gdiplus::Rect newbounds(0,0,inWantedSize,inWantedSize);
				Gdiplus::Bitmap* tmp=new Gdiplus::Bitmap(inWantedSize,inWantedSize,PixelFormat32bppPARGB);
				Gdiplus::Graphics graph(tmp);
				graph.DrawImage(fBitmap,newbounds);
				delete fBitmap;
				fBitmap=tmp;
			}
		}
		fBounds.SetCoords(0,0,fBitmap->GetWidth(),fBitmap->GetHeight());
	}
	_SetSafe();
}
VPictureData_GDIPlus::VPictureData_GDIPlus(Gdiplus::Bitmap* inPicture,BOOL inOwnedPicture)
{
	_Init();
	if(inOwnedPicture)
		fBitmap=inPicture;
	else
	{
		// IMPORTANT il faut ouvrir la bitmap en mode write pour forcer gdiplus a dupliqué le buffer
		// sans le clone pointe tj vers le buffer de inPicture
		fBitmap=inPicture->Clone(0,0,inPicture->GetWidth(),inPicture->GetHeight(),inPicture->GetPixelFormat());
		if(fBitmap)
		{
			Gdiplus::BitmapData bmData;
			Gdiplus::Rect r(0,0,fBitmap->GetWidth(),fBitmap->GetHeight());
			fBitmap->LockBits(&r,Gdiplus::ImageLockModeWrite,fBitmap->GetPixelFormat(),&bmData);
			fBitmap->UnlockBits(&bmData);
		}
	}
	fBounds.SetCoords(0,0,fBitmap->GetWidth(),fBitmap->GetHeight());
	_SetDecoderByExtension("png");	
	_SetSafe();
}
VPictureData_GDIPlus::VPictureData_GDIPlus(VPictureDataProvider* inDataSource,_VPictureAccumulator* inRecorder)
:VPictureData_Bitmap(inDataSource,inRecorder)
{
	_Init();
}
VPictureData_GDIPlus::VPictureData_GDIPlus(const VPictureData_GDIPlus& inData)
:VPictureData_Bitmap(inData)
{
	_Init();
}

VPictureData_GDIPlus::~VPictureData_GDIPlus()
{
	if(fBitmap)
	{
#if ENABLE_D2D
		VWinD2DGraphicContext::ReleaseBitmap( fBitmap);
#endif
		delete fBitmap;
		fBitmap = NULL;
	}
}
VPictureData_GDIPlus::VPictureData_GDIPlus(PortRef inPortRef,VRect inBounds)
{
	HDC dstdc;
	HBITMAP dstbm,olbdm;
	
	dstdc=CreateCompatibleDC(inPortRef);
	dstbm=CreateCompatibleBitmap(inPortRef,inBounds.GetWidth(),inBounds.GetHeight());
	olbdm=(HBITMAP)SelectObject(dstdc,dstbm);
	::BitBlt(dstdc,0,0,inBounds.GetWidth(),inBounds.GetHeight(),inPortRef,0,0,SRCCOPY);
	::SelectObject(dstdc,olbdm);
	::DeleteDC(dstdc);
	
	fBitmap=new xgdiplusbitmap(dstbm,NULL);
	fBounds.SetCoords(0,0,fBitmap->GetWidth(),fBitmap->GetHeight());
	_SetSafe();
}
VPictureData* VPictureData_GDIPlus::Clone()const
{
	return new VPictureData_GDIPlus(*this);
}

void VPictureData_GDIPlus::_Init()
{
	fBitmap=NULL;
}

VError VPictureData_GDIPlus::Save(VBlob* inData,VIndex inOffset,VSize& outSize,_VPictureAccumulator* /*inRecorder*/)const
{
	VError result=VE_UNKNOWN_ERROR;
	if(fDataProvider)
	{
		return inherited::Save(inData,inOffset,outSize);
	}
	else
	{
		if(fBitmap)
		{
			VBlobWithPtr blob;
				
			bool savedone=false;
			const VPictureCodec* deco=RetainDecoder();
			if(deco)
			{
				if(!deco->IsEncoder())
				{
					ReleaseRefCountable(&deco);
				}
			}
			if(!deco)
			{
				VPictureCodecFactoryRef fact;
				deco=fact->RetainDecoderByIdentifier(".png");
			}
			if(deco)
			{
					
				result = deco->Encode(this,NULL,blob); 
					
				if(result==VE_OK)
				{
					outSize=blob.GetSize();
					result=inData->PutData(blob.GetDataPtr(),outSize,inOffset);
				}
			}
		}

		/*
		if(fBitmap)
		{
			DWORD err;
			IStream* stream=NULL; 
			err= ::CreateStreamOnHGlobal(NULL,true,&stream);
			if(err == S_OK)
			{
				VPictureCodecFactoryRef fact;
				const xGDIPlusDecoderList& encoderlist=fact->xGetGDIPlus_Decoder();
				
				Gdiplus::Status stat = Gdiplus::GenericError;

				CLSID encoder;
				
				
				if(encoderlist.FindClassID(*fBitmap,encoder))
				{
					stat = fBitmap->Save(stream,&encoder,NULL);
				
					HGLOBAL glob;
					GetHGlobalFromStream(stream,&glob);
					char* c=(char*)GlobalLock(glob);
					outSize=GlobalSize(glob);
					result = inData->PutData(c,outSize,inOffset);
					GlobalUnlock(glob);
					stream->Release();
				}
				else
				{
					assert(false);
				}
			}
		}
		*/
	}
	return result;
}
void VPictureData_GDIPlus::BuildTimeLine(std::vector<sLONG>& /*inTimeLine*/,sLONG &outLoopCount)const
{
	
}
sLONG VPictureData_GDIPlus::GetFrameCount() const
{
	_Load();
	if(fBitmap)
	{
		return fBitmap->GetFrameCount(&Gdiplus::FrameDimensionTime);
	}
	else
		return 1;
}
void VPictureData_GDIPlus::DrawInPortRef(PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet)const
{
	_Load();
	if(fBitmap)
	{
		xDraw(fBitmap,inPortRef,r,inSet);
	}
}
#if ENABLE_D2D
void VPictureData_GDIPlus::DrawInD2DGraphicContext(VWinD2DGraphicContext* inDC,const VRect& r,VPictureDrawSettings* inSet)const
{
	_Load();
	if(fBitmap)
		xDraw(fBitmap,inDC,r,inSet);
}
#endif
void VPictureData_GDIPlus::DrawInGDIPlusGraphics(Gdiplus::Graphics* inDC,const VRect& r,VPictureDrawSettings* inSet)const
{
	_Load();
	if(fBitmap)
		xDraw(fBitmap,inDC,r,inSet);
}
Gdiplus::Bitmap* VPictureData_GDIPlus::GetGDIPlus_Bitmap()const
{
	_Load();
	return fBitmap;
}
/** return bitmap & give ownership to caller: caller becomes the owner & internal bitmap reference is set to NULL to prevent bitmap to be released 
@remarks
	caller should release this picture data just after this call because picture data is no longer valid
	it should be used only when caller needs to take ownership of the internal bitmap (in order to modify it without copying it) & 
	if picture data is released after
*/
Gdiplus::Bitmap* VPictureData_GDIPlus::GetAndOwnGDIPlus_Bitmap()const
{
	if(fBitmap)
	{
#if ENABLE_D2D
		VWinD2DGraphicContext::ReleaseBitmap( fBitmap);
#endif
		Gdiplus::Bitmap *bmp = fBitmap;
		fBitmap = NULL;
		return bmp;
	}
	return NULL;
}


void VPictureData_GDIPlus::_DoReset()const
{
	if(fBitmap)
	{
#if ENABLE_D2D
		VWinD2DGraphicContext::ReleaseBitmap( fBitmap);
#endif
		delete fBitmap;
		fBitmap=0;
	}
}


/** load metadatas */
void VPictureData_GDIPlus::_DoLoadMetadatas()const
{
	if(fDataProvider)
	{
		//prepare metadatas bag
		if (fMetadatas == NULL)
			fMetadatas = new VValueBag();
		else
			fMetadatas->Destroy();
		
		//copy metadatas to the provided bag
		VPictureCodec_WIC::GetMetadatas(*fDataProvider, GUID_NULL, fMetadatas); 
	}
}

void VPictureData_GDIPlus::_DoLoad()const
{
	if(fDataProvider)
	{
		//try to decode with WIC decoder first
		xbox_assert(fBitmap == NULL);
		fBitmap = VPictureCodec_WIC::Decode(*fDataProvider);
		if (fBitmap)
		{
			fBounds.SetCoords(0,0,fBitmap->GetWidth(),fBitmap->GetHeight());
			return;
		}

		//backfall to gdiplus decoder if failed
		VPictureDataProvider_Stream* st=new VPictureDataProvider_Stream(fDataProvider);
		fBitmap=new Gdiplus::Bitmap(st);
		//JQ 10/07/2012: fixed ACI0077506
		if (VSystem::IsVista() && XWinSystem::GetSystemVersion()!=WIN_SERVER_2008) // pp incident 125661. le changement de resolution ne fonctionne pas sous 2008 server r1
			fBitmap->SetResolution( 96.0f, 96.0f); //JQ 16/04/2012: fixed ACI0076335 - ensure dpi=96
		st->Release();
		fBounds.SetCoords(0,0,fBitmap->GetWidth(),fBitmap->GetHeight());
	}
}
/*
VPictureData* VPictureData_GDIPlus::BuildThumbnail(sLONG inWidth,sLONG inHeight,PictureMosaic inMode,VPictureDrawSettings* inSet)
{
	VPictureData* result=NULL;
	_Load();
	if(fBitmap)
	{
		VPictureDrawSettings set(inSet);
		VRect dstrect;
		Gdiplus::Bitmap* bm_thumbnail;
		
		CalcThumbRect(inWidth,inHeight,inMode,dstrect);
		
		bm_thumbnail=new Gdiplus::Bitmap(inWidth,inHeight,PixelFormat32bppARGB);
		
		Gdiplus::Graphics graph(bm_thumbnail);
		graph.Clear(Gdiplus::Color(0,0,0,0));
		graph.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
		
		set.SetScaleMode(PM_SCALE_TO_FIT);
		Draw(&graph,dstrect,&set);
		//graph.DrawImage(fBitmap,(int)dstrect.GetX(),(int)dstrect.GetY(),(int)dstrect.GetWidth(),(int)dstrect.GetHeight()); 
		
		result=new VPictureData_GDIPlus(bm_thumbnail,true);
	}
	return result;
}
*/
/***********************************************************************************************/

VPictureData_EMF::VPictureData_EMF()
:inherited()
{
	_Init();
}
void VPictureData_EMF::_Init()
{
	_SetDecoderByExtension("emf");
	fPicHandle=NULL;
	fMetaFile=NULL;
}
VPictureData_EMF::VPictureData_EMF(HMETAFILEPICT inMetaPict)
{
	_Init();
	FromEnhMetaFile((HENHMETAFILE)inMetaPict);
	_SetSafe();
	
}
VPictureData_EMF::VPictureData_EMF(HENHMETAFILE inMetaFile)
{
	_Init();
	FromEnhMetaFile(inMetaFile);
	_SetSafe();
}

VPictureData_EMF::VPictureData_EMF(VPictureDataProvider* inDataSource,_VPictureAccumulator* inRecorder)
:inherited(inDataSource,inRecorder)
{
	_Init();
}
VPictureData_EMF::VPictureData_EMF(const VPictureData_EMF& inData)
:inherited(inData)
{
	_Init();
	if(!fDataProvider && inData.fPicHandle)
	{
		VSize datasize;
		datasize=GetMacAllocator()->GetSize(inData.fPicHandle);
		fPicHandle=GetMacAllocator()->Allocate(datasize);
		if(fPicHandle)
		{
			GetMacAllocator()->Lock(fPicHandle);
			GetMacAllocator()->Lock(inData.fPicHandle);
			CopyBlock(*(char**)inData.fPicHandle,*(char**)fPicHandle,datasize);
			GetMacAllocator()->Unlock(fPicHandle);
			GetMacAllocator()->Unlock(inData.fPicHandle);
		}
		_SetSafe();
	}
}

VPictureData_EMF::~VPictureData_EMF()
{
	if(fPicHandle)
	{
		GetMacAllocator()->Free(fPicHandle);
	}
	_DisposeMetaFile();
}
void VPictureData_EMF::_DoReset()const
{
	_DisposeMetaFile();
}
void VPictureData_EMF::_DoLoad()const
{
	_DisposeMetaFile();
	if(fDataProvider)
	{
		fMetaFile=::SetEnhMetaFileBits((UINT)fDataProvider->GetDataSize(),(uBYTE*)fDataProvider->BeginDirectAccess());
		fDataProvider->EndDirectAccess();
		if(fMetaFile)
		{
			ENHMETAHEADER header;
			if(GetEnhMetaFileHeader(fMetaFile,sizeof(ENHMETAHEADER),&header))
			{
				fBounds.SetCoords(0,0,header.rclBounds.right-header.rclBounds.left,header.rclBounds.bottom-header.rclBounds.top);
			}
		}
	}
}

VPictureData* VPictureData_EMF::Clone()const
{
	VPictureData_EMF* clone;
	clone= new VPictureData_EMF(*this);
	return clone;
}

VError VPictureData_EMF::Save(VBlob* inData,VIndex inOffset,VSize& outSize,_VPictureAccumulator* /*inRecorder*/)const
{
	VError result=VE_OK;
	if(fDataProvider && fDataProvider->GetDataSize())
	{
		return inherited::Save(inData,inOffset,outSize);
	}
	else
	{
		if(fMetaFile)
		{
			long buffsize=GetEnhMetaFileBits(fMetaFile,0,NULL);
			if(buffsize)
			{
				uBYTE* buffer=(uBYTE*)new char[buffsize];
				if(buffer)
				{
					if(GetEnhMetaFileBits(fMetaFile,buffsize,buffer))
					{
						outSize=buffsize;
						result=inData->PutData(buffer,outSize,inOffset);
					}
					delete[] buffer;
				}
			}
		}
	}
	return result;
}
void VPictureData_EMF::_DisposeMetaFile()const
{
	#if VERSIONWIN
	if(fMetaFile)
	{
		DeleteEnhMetaFile(fMetaFile);
		fMetaFile=NULL;
	}
	#endif
}
void VPictureData_EMF::FromMacHandle(void* inMacHandle)
{
	_ReleaseDataProvider();
	_DisposeMetaFile();
	if(inMacHandle)
	{
		VSize datasize=GetMacAllocator()->GetSize(inMacHandle);
		if(datasize)
		{
			fPicHandle=GetMacAllocator()->Allocate(GetMacAllocator()->GetSize(inMacHandle));
			if(fPicHandle)
			{
				GetMacAllocator()->Lock(fPicHandle);
				GetMacAllocator()->Lock(inMacHandle);
				CopyBlock(*(char**)inMacHandle,*(char**)fPicHandle,datasize);
				GetMacAllocator()->Unlock(fPicHandle);
				GetMacAllocator()->Unlock(inMacHandle);
			}
		}
		else
			fPicHandle=NULL;
	}
	else
	{
		fPicHandle=NULL;
	}
}

#if VERSIONWIN
HENHMETAFILE	VPictureData_EMF::GetMetaFile()const
{
	_Load();
	return fMetaFile;
}

#endif

xMacPictureHandle VPictureData_EMF::GetPicHandle()const
{
	if(!fPicHandle)
	{
		if(fDataProvider && fDataProvider->GetDataSize())
			fPicHandle=GetMacAllocator()->Allocate(fDataProvider->GetDataSize());
		if(fPicHandle)
		{
			//VPtr p=fDataProvider->BeginDirectAccess();
			GetMacAllocator()->Lock(fPicHandle);
			fDataProvider->GetData(*(char**)fPicHandle,0,fDataProvider->GetDataSize());
			//CopyBlock(p,*(char**)fPicHandle,fDataProvider->GetDataSize());
#if VERSIONWIN
			ByteSwapWordArray(*(sWORD**)fPicHandle,5);
#endif
			GetMacAllocator()->Unlock(fPicHandle);
			//fDataProvider->EndDirectA();
		}
	}
	
	if(fPicHandle)
	{
		assert(GetMacAllocator()->CheckBlock(fPicHandle));
	}
	return (xMacPictureHandle)fPicHandle;
}


void VPictureData_EMF::FromMetaFilePict(METAFILEPICT* inMetaPict)
{
	HENHMETAFILE henh;
	_ReleaseDataProvider();
	_DisposeMetaFile();
	
	void*        lpWinMFBits;
	UINT         uiSizeBuf;

	uiSizeBuf = GetMetaFileBitsEx(inMetaPict->hMF, 0, NULL);
	lpWinMFBits = GlobalAllocPtr(GHND, uiSizeBuf);
	GetMetaFileBitsEx(inMetaPict->hMF, uiSizeBuf, (LPVOID)lpWinMFBits);
	henh = SetWinMetaFileBits(uiSizeBuf, (LPBYTE)lpWinMFBits, NULL, inMetaPict);
	GlobalFreePtr(lpWinMFBits);


	FromEnhMetaFile(henh);
}
void VPictureData_EMF::FromEnhMetaFile(HENHMETAFILE inMetaFile)
{
	_ReleaseDataProvider();
	_DisposeMetaFile();
	fMetaFile=inMetaFile;
	ENHMETAHEADER header;
	if(GetEnhMetaFileHeader(fMetaFile,sizeof(ENHMETAHEADER),&header))
	{
		fBounds.SetCoords(0,0,header.rclBounds.right-header.rclBounds.left,header.rclBounds.bottom-header.rclBounds.top);
	}
}


void VPictureData_EMF::DrawInPortRef(PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet)const
{
	_Load();
	if(fMetaFile)
		xDraw(fMetaFile,inPortRef,r,inSet);
}

#if ENABLE_D2D
void VPictureData_EMF::DrawInD2DGraphicContext(VWinD2DGraphicContext* inDC,const VRect& r,VPictureDrawSettings* inSet)const
{
	_Load();
	if(fMetaFile)
	{
		//here we need to get a HDC from D2D render target 
		//(if called between BeginUsingContext/EndUsingContext,
		// this will force a EndDraw() before getting hdc and a BeginDraw() to resume drawing after releasing hdc)				
		HDC hdc = inDC->BeginUsingParentContext();
		if (hdc)
			xDraw(fMetaFile,hdc,r,inSet);
		inDC->EndUsingParentContext( hdc);
	}
}
#endif
void VPictureData_EMF::DrawInGDIPlusGraphics(Gdiplus::Graphics* inDC,const VRect& r,VPictureDrawSettings* inSet)const
{
	_Load();
	if(fMetaFile)
		xDraw(fMetaFile,inDC,r,inSet);
}
/*
VPictureData* VPictureData_EMF::BuildThumbnail(sLONG inWidth,sLONG inHeight,PictureMosaic inMode,VPictureDrawSettings* inSet)
{
	assert(false);
	return NULL;
}
*/
/**************************************************************************************/

VPictureData_GDIPlus_Vector::VPictureData_GDIPlus_Vector()
:inherited()
{
	_Init();
}
VPictureData_GDIPlus_Vector::VPictureData_GDIPlus_Vector(VPictureDataProvider* inDataSource,_VPictureAccumulator* inRecorder)
:inherited(inDataSource,inRecorder)
{
	_Init();
}
VPictureData_GDIPlus_Vector::VPictureData_GDIPlus_Vector(const VPictureData_GDIPlus_Vector& inData)
:inherited(inData)
{
	_Init();
}

VPictureData_GDIPlus_Vector::VPictureData_GDIPlus_Vector(METAFILEPICT* inMetaPict)
{
	_Init();
	_FromMetaFilePict(inMetaPict);
	_SetSafe();
}
VPictureData_GDIPlus_Vector::VPictureData_GDIPlus_Vector(HENHMETAFILE inMetaFile)
{
	_Init();
	_FromEnhMetaFile(inMetaFile);
	_SetSafe();
}
VPictureData_GDIPlus_Vector::VPictureData_GDIPlus_Vector(Gdiplus::Metafile* inMetafile)
{
	_Init();
	_SetDecoderByExtension("emf");
	fMetafile=inMetafile;
	_InitSize();
	_SetSafe();
}

VPictureData_GDIPlus_Vector::~VPictureData_GDIPlus_Vector()
{
	if(fMetafile)
		delete fMetafile;
}

VPictureData* VPictureData_GDIPlus_Vector::Clone()const
{
	return new VPictureData_GDIPlus_Vector(*this);
}

void VPictureData_GDIPlus_Vector::_Init()
{
	fMetafile=NULL;
}



VError VPictureData_GDIPlus_Vector::Save(VBlob* inData,VIndex inOffset,VSize& outSize,_VPictureAccumulator* /*inRecorder*/)const
{
	VError result=VE_UNKNOWN_ERROR;
	if(fDataProvider)
	{
		result=inherited::Save(inData,inOffset,outSize);
	}
	else
	{
		if(fMetafile)
		{
			HENHMETAFILE meta;
			Gdiplus::Metafile* clone=(Gdiplus::Metafile*)fMetafile->Clone();
			meta=clone->GetHENHMETAFILE();
			if(meta)
			{
				long buffsize=GetEnhMetaFileBits(meta,0,NULL);
				if(buffsize)
				{
					uBYTE* buffer=(uBYTE*)new char[buffsize];
					if(buffer)
					{
						if(GetEnhMetaFileBits(meta,buffsize,buffer))
						{
							outSize=buffsize;
							result=inData->PutData(buffer,outSize,inOffset);
						}
						delete[] buffer;
					}
				}
				DeleteEnhMetaFile(meta);
			}
			delete clone;
		}
	}
	return result; 
}
#if ENABLE_D2D
void VPictureData_GDIPlus_Vector::DrawInD2DGraphicContext(VWinD2DGraphicContext* inDC,const VRect& r,VPictureDrawSettings* inSet)const
{
	_Load();
	if(fMetafile)
	{
		//here we need to get a HDC from D2D render target 
		//(if called between BeginUsingContext/EndUsingContext,
		// this will force a EndDraw() before getting hdc and a BeginDraw() to resume drawing after releasing hdc)				
		StSaveContext_NoRetain saveCtx(inDC);
		HDC hdc = inDC->BeginUsingParentContext();
		if (hdc)
			xDraw(fMetafile,hdc,r,inSet);
		inDC->EndUsingParentContext( hdc);
	}
}
#endif
void VPictureData_GDIPlus_Vector::DrawInGDIPlusGraphics(Gdiplus::Graphics* inDC,const VRect& r,VPictureDrawSettings* inSet)const
{
	_Load();
	if(fMetafile)
		xDraw(fMetafile,inDC,r,inSet);
}
void VPictureData_GDIPlus_Vector::DrawInPortRef(PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet)const
{
	_Load();
	if(fMetafile)
		xDraw(fMetafile,inPortRef,r,inSet);
}

Gdiplus::Metafile*	VPictureData_GDIPlus_Vector::GetGDIPlus_Metafile()const
{
	_Load();
	return fMetafile;
}
void VPictureData_GDIPlus_Vector::_DoReset()const
{
	_DisposeMetaFile();
}
void VPictureData_GDIPlus_Vector::_InitSize()const
{
	if(fMetafile)
	{
		Gdiplus::Rect rect;
		Gdiplus::MetafileHeader gh;
		fMetafile->GetMetafileHeader(&gh);
		
		//sLONG width  = (short) ( ( (long) gh.EmfHeader.rclFrame.right * 96L ) / 2540L );
		//sLONG height = (short) ( ( (long) gh.EmfHeader.rclFrame.bottom * 96L ) / 2540L );
		
		/*re1.Width= gh.EmfHeader.rclFrame.right * gh.EmfHeader.szlDevice.cx / (gh.EmfHeader.szlMillimeters.cx*100);
		re1.Height= gh.EmfHeader.rclFrame.bottom * gh.EmfHeader.szlDevice.cy / (gh.EmfHeader.szlMillimeters.cy*100);
		re1.Width=re1.Width * (96.0/gh.GetDpiX());
		re1.Height=re1.Height * (96.0/gh.GetDpiY());*/
		
		gh.GetBounds(&rect);
		fBounds.SetCoords(rect.GetLeft(),rect.GetTop(),rect.GetRight()-rect.GetLeft(),rect.GetBottom()-rect.GetTop());
	}
}
void VPictureData_GDIPlus_Vector::_DoLoad()const
{
	_DisposeMetaFile();
	if(fDataProvider)
	{
		const VPictureCodec* decoder=fDataProvider->RetainDecoder();
		if(decoder)
		{
			fMetafile=decoder->_CreateGDIPlus_Metafile(*fDataProvider);
			_InitSize();
		}
	}
}
/*
void VPictureData_GDIPlus_Vector::FromVFile(VFile& inFile)
{
	Gdiplus::Rect rect;
	SetDataSource(NULL,true);
	_DisposeMetaFile();
	_SetDecoderByExtension("emf");
	
	VString path;
	inFile.GetPath(path);
	fMetafile=new Gdiplus::Metafile(path.GetCPointer());
	_InitSize();
}
*/
void VPictureData_GDIPlus_Vector::_FromMetaFilePict(METAFILEPICT* inMetaPict)
{
	HENHMETAFILE henh;
	void*        lpWinMFBits;
	UINT         uiSizeBuf;

	uiSizeBuf = GetMetaFileBitsEx(inMetaPict->hMF, 0, NULL);
	lpWinMFBits = GlobalAllocPtr(GHND, uiSizeBuf);
	GetMetaFileBitsEx(inMetaPict->hMF, uiSizeBuf, (LPVOID)lpWinMFBits);
	henh = SetWinMetaFileBits(uiSizeBuf, (LPBYTE)lpWinMFBits, NULL, inMetaPict);
	GlobalFreePtr(lpWinMFBits);

	_FromEnhMetaFile(henh);
}
void VPictureData_GDIPlus_Vector::_FromEnhMetaFile(HENHMETAFILE inMetaFile)
{
	SetDataProvider(0,false);
	_DisposeMetaFile();
	
	_SetDecoderByExtension("emf");
	
	fMetafile=new Gdiplus::Metafile(inMetaFile,true);
	_InitSize();
}

void VPictureData_GDIPlus_Vector::_DisposeMetaFile()const
{
	if(fMetafile)
	{
		delete fMetafile;
		fMetafile=NULL;
	}
}
/*
VPictureData* VPictureData_GDIPlus_Vector::BuildThumbnail(sLONG inWidth,sLONG inHeight,PictureMosaic inMode,VPictureDrawSettings* inSet)
{
	VPictureData* result=NULL;
	_Load();
	if(fMetafile)
	{
		VRect dstrect;
		VPictureDrawSettings set(inSet);
		Gdiplus::Bitmap* bm_thumbnail;
		
		CalcThumbRect(inWidth,inHeight,inMode,dstrect);
		
		bm_thumbnail=new Gdiplus::Bitmap(inWidth,inHeight,PixelFormat32bppARGB);
		
		Gdiplus::Graphics graph(bm_thumbnail);
		graph.Clear(Gdiplus::Color(0,0,0,0));
		graph.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
		
		set.SetScaleMode(PM_SCALE_TO_FIT);
		Draw(&graph,dstrect,&set);
		
		//graph.DrawImage(fMetafile,(int)dstrect.GetX(),(int)dstrect.GetY(),(int)dstrect.GetWidth(),(int)dstrect.GetHeight()); 
		result=new VPictureData_GDIPlus(bm_thumbnail,true);
	}
	return result;
}
*/

/****** gif ******/

VPictureData_GIF::VPictureData_GIF()
:inherited()
{
	_Init();
}
VPictureData_GIF::VPictureData_GIF(VPictureDataProvider* inDataSource,_VPictureAccumulator* inRecorder)
:inherited(inDataSource,inRecorder)
{
	_Init();
}
VPictureData_GIF::VPictureData_GIF(const VPictureData_GIF& inData)
:inherited(inData)
{
	_Init();
}

VPictureData_GIF::~VPictureData_GIF()
{
	delete fGifInfo;
}

sLONG VPictureData_GIF::GetFrameCount() const
{
	return inherited::GetFrameCount();
}

void VPictureData_GIF::BuildTimeLine(std::vector<sLONG>& inTimeLine,sLONG &outLoopCount)const
{
	_Load();
	inTimeLine.clear();
	outLoopCount=0;
	if(fGifInfo)
	{
		for( xGifInfo::xGifFrameList::iterator i = fGifInfo->fFrameList.begin() ; i != fGifInfo->fFrameList.end() ; ++i)
		{
			xGifFrame* f = *i;
			if(f->isGif89)
				inTimeLine.push_back(f->fGif89.delayTime*10);
			else
				inTimeLine.push_back(100);
		}
		outLoopCount=fGifInfo->GetLoopCount();
	}
}
void VPictureData_GIF::_DoReset()const
{
	inherited::_DoReset();
	
	delete fGifInfo;
	fGifInfo=0;
}
void VPictureData_GIF::_DoLoad()const
{
	if(fGifInfo)
	{
		delete fGifInfo;
		fGifInfo=0;
	}

	inherited::_DoLoad();
	if(fBitmap)
	{
		fGifInfo = new xGifInfo_GDIPlus(*fBitmap);
	}
}

void VPictureData_GIF::_Init()
{
	fGifInfo=0;
}

VPictureData* VPictureData_GIF::Clone()const
{
	return new VPictureData_GIF(*this);
}
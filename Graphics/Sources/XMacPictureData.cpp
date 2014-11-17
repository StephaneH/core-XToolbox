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

VPictureData_CGImage::VPictureData_CGImage()
:VPictureData_Bitmap()
{
	_Init();
}

VPictureData_CGImage::VPictureData_CGImage(CGImageRef inPicture)
:VPictureData_Bitmap()
{
	_Init();
	fBitmap=inPicture;
	_SetDecoderByExtension("png");
	if(inPicture)
	{
		CGImageRetain(inPicture);
		fBounds.SetCoords(0,0,CGImageGetWidth(fBitmap),CGImageGetHeight(fBitmap));
	}
}

VPictureData_CGImage::VPictureData_CGImage(IconRef inIcon,sLONG inWantedSize)
{
	_Init();
	_SetDecoderByExtension("png");
	if(inIcon)
	{
		sLONG bpr   = ( 4 * inWantedSize + 15 ) & ~15;
		sLONG bitmapByteCount     = (bpr * inWantedSize);
		void *PixBuffer = malloc( bpr * inWantedSize );
		if(PixBuffer)
		{
			CGContextRef    context = NULL;
			CGColorSpaceRef colorSpace;
			colorSpace = CGColorSpaceCreateDeviceRGB();
			
			context = CGBitmapContextCreate (PixBuffer,
								inWantedSize,
								inWantedSize,
								8,      // bits per component
								bpr,
								colorSpace,
								kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Host);
			if (context) 
			{
				CGRect vr;
				vr.origin.x= 0;
				vr.origin.y=  0;
				vr.size.width=inWantedSize;
				vr.size.height=inWantedSize;
				::CGContextClearRect(context, vr);
				
				PlotIconRefInContext(context,&vr,kAlignAbsoluteCenter,kTransformNone,0,kPlotIconRefNormalFlags,inIcon);
				
				CGContextRelease(context);
				
				CGDataProviderRef dataprov= xV4DPicture_MemoryDataProvider::CGDataProviderCreate((char*)PixBuffer,inWantedSize*bpr,true);
				fBitmap=CGImageCreate(inWantedSize,inWantedSize,8,32,bpr,colorSpace,kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Host,dataprov,0,0,kCGRenderingIntentDefault);
				CGDataProviderRelease(dataprov);
				fBounds.SetCoords(0,0,CGImageGetWidth(fBitmap),CGImageGetHeight(fBitmap));
			}
			free(PixBuffer)	;
			CGColorSpaceRelease( colorSpace );	
		}

		
	}
}
VPictureData_CGImage::VPictureData_CGImage(VPictureDataProvider* inDataProvider,_VPictureAccumulator* inRecorder)
:VPictureData_Bitmap(inDataProvider,inRecorder)
{
	_Init();
}

VPictureData_CGImage::VPictureData_CGImage(const VPictureData_CGImage& inData)
:VPictureData_Bitmap(inData)
{
	_Init();
	
}

VPictureData_CGImage::~VPictureData_CGImage()
{
	_DoReset();
}

VPictureData_CGImage::VPictureData_CGImage(PortRef inPortRef,const VRect& inBounds)
{
	_Init();
}

VPictureData* VPictureData_CGImage::Clone()const
{
	return new VPictureData_CGImage(*this);
}

void VPictureData_CGImage::_Init()
{
	fBitmap=NULL;
	fTransBitmap=NULL;
}

VError VPictureData_CGImage::Save(VBlob* inData,VIndex inOffset,VSize& outSize,_VPictureAccumulator* inRecorder)const
{
	VError result=VE_UNKNOWN_ERROR;
	if(fDataProvider)
	{
		return inherited::Save(inData,inOffset,outSize,inRecorder);
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
	}
	return result;
}

void VPictureData_CGImage::DrawInPortRef(PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet)const
{
	_Load();
	if(fBitmap)
		xDraw(fBitmap, inPortRef,r,inSet);
}

void VPictureData_CGImage::DrawInCGContext(CGContextRef inDC,const VRect& r,VPictureDrawSettings* inSet)const
{
	_Load();
	if(fBitmap)
		xDraw(fBitmap, inDC,r,inSet);

}
void VPictureData_CGImage::_DoReset()const
{
	if(fBitmap)
		CGImageRelease(fBitmap);
	if(fTransBitmap)
		CGImageRelease(fTransBitmap);

	fBitmap=0;
	fTransBitmap=0;
}

/** load metadatas */
void VPictureData_CGImage::_DoLoadMetadatas()const
{
	VPictureDataProvider *datasrc=GetDataProvider();
	if(datasrc!=NULL)
	{
		//prepare metadatas bag
		if (fMetadatas == NULL)
			fMetadatas = new VValueBag();
		else
			fMetadatas->Destroy();
		
		//copy metadatas to the provided bag
		VPictureCodec_ImageIO::GetMetadatas(*datasrc, fMetadatas);
	}
}

void VPictureData_CGImage::_DoLoad()const
{
	VPictureDataProvider *datasrc=GetDataProvider();
	if(datasrc!=NULL)
	{
		//decode image from data source 
		fBitmap = VPictureCodec_ImageIO::Decode(*datasrc);
		if (fBitmap)
		{
			fBounds.SetCoords(0,0,CGImageGetWidth(fBitmap),CGImageGetHeight(fBitmap));
		}
		/*
		::GraphicsImportComponent importer = 0;
		
		xQTPointerDataRef dataref(datasrc);
				
		::GetGraphicsImporterForDataRef(dataref,dataref.GetKind(),&importer);
		
		if(importer)
		{
			GraphicsImportSetFlags(importer,kGraphicsImporterDontDoGammaCorrection | kGraphicsImporterDontUseColorMatching);
			OSErr err = GraphicsImportCreateCGImage( importer, &fBitmap, kGraphicsImportCreateCGImageUsingCurrentSettings );
			
			if(err==0)
			{
				fBounds.SetCoords(0,0,CGImageGetWidth(fBitmap),CGImageGetHeight(fBitmap));
			}
			CloseComponent( importer );
		}
		*/
	}
}

void VPictureData_CGImage::FromVFile(VFile& inFile)
{
	_ReleaseDataProvider();
	if(fBitmap)
		CGImageRelease(fBitmap);
		
	VPictureDataProvider* datasrc=VPictureDataProvider::Create(inFile);
	if(datasrc)
	{
		SetDataProvider(datasrc);
		datasrc->Release();
		_Load();
	}
}

CGImageRef	VPictureData_CGImage::GetCGImage()const
{
	_Load();
	return fBitmap;
}
#if 0
VPictureData* VPictureData_CGImage::BuildThumbnail(sLONG inWidth,sLONG inHeight,PictureMosaic inMode,VPictureDrawSettings* inSet)
{
	VPictureData* result=0;
	if(GetCGImage())
	{
		VRect dstrect;
		CalcThumbRect(inWidth,inHeight,inMode,dstrect);
		
		CGContextRef    context = NULL;
		CGColorSpaceRef colorSpace;
		void *          bitmapData;
		int             bitmapByteCount;
		int             bitmapBytesPerRow;
		
		bitmapBytesPerRow   = ( 4*inWidth + 15 ) & ~15;

		bitmapByteCount     = (bitmapBytesPerRow * inHeight);
		
		colorSpace = CGColorSpaceCreateDeviceRGB();

		bitmapData = malloc( bitmapByteCount );
		
		if(bitmapData)
		{
			memset(bitmapData,0,bitmapByteCount);
			context = CGBitmapContextCreate (bitmapData,
                                    inWidth,
                                    inHeight,
                                    8,      // bits per component
                                    bitmapBytesPerRow,
                                    colorSpace,
                                    kCGImageAlphaPremultipliedLast);
			 if (context)
			 {
				CGRect vr;
				vr.origin.x= (inWidth/2) - (dstrect.GetWidth()/2);
				vr.origin.y=  (inHeight/2) - (dstrect.GetHeight()/2);
				vr.size.width=dstrect.GetWidth();
				vr.size.height=dstrect.GetHeight();
				
				::CGContextClearRect(context, vr); 
				
				::CGContextDrawImage(context,vr,GetCGImage());
				::CGContextFlush(context);
				CGImageRef im;
				
				CGDataProviderRef dataprov= xV4DPicture_MemoryDataProvider::CGDataProviderCreate((char*)bitmapData,bitmapByteCount,true);
				
				im=CGImageCreate(inWidth,inHeight,8,32,bitmapBytesPerRow,colorSpace,kCGImageAlphaPremultipliedLast,dataprov,0,0,kCGRenderingIntentDefault);
				CGDataProviderRelease(dataprov);
				if(im)
				{
					result=new VPictureData_CGImage(im);
					CGImageRelease(im);
				}
				CGContextRelease(context);
			 }
			free(bitmapData);
		}
		CGColorSpaceRelease( colorSpace );
	}
	return result;
}
#endif
/******************************************************/

xGifInfo_QTImage::~xGifInfo_QTImage()
{
	for(long i=0;i<fCGImageList.size();i++)
	{
		CFRelease(fCGImageList[i]);
	}
}

xGifInfo_QTImage::xGifInfo_QTImage(VStream* inStream)
{
	FromVStream(inStream);
}

void xGifInfo_QTImage::FromVStream(VStream* inStream)
{
	CGImageRef prev=0;
	// creation des frames
	inherited::FromVStream(inStream);
	for(long i=0;i<fFrameList.size();i++)
	{
		xGifFrame* im=fFrameList[i];
		AddFrame(im,prev);
		im->FreePixelBuffer();
		prev=fCGImageList[i];
	}
}

void xGifInfo_QTImage::AddFrame(xGifFrame* inFrame,CGImageRef inPrev)
{
	CGImageRef frame=0;
	sLONG width,height;
	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();

	
	width = inFrame->fWidth;
	height = inFrame->fHeight;
	
	sLONG bitmapBytesPerRow   = ( 4*fWidth + 15 ) & ~15;
	sLONG bitmapByteCount     = (bitmapBytesPerRow * fHeight);
	
	char* bitmapData = (char*)malloc( bitmapByteCount );
		
	if(bitmapData)
	{
		
		memset(bitmapData,0,bitmapByteCount);
		if(inFrame->isGif89 && (inFrame->fGif89.disposal!=2) && inPrev)
		{
			CGContextRef context;
			// il faut blitter la frame precedente
			context = CGBitmapContextCreate (bitmapData,
                                    fWidth,
                                    fHeight,
                                    8,      // bits per component
                                    bitmapBytesPerRow,
                                    colorSpace,
                                    kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host);
			if(context)
			{
				CGRect vr;
				vr.origin.x= 0;
				vr.origin.y=  0;
				vr.size.width=fWidth;
				vr.size.height=fHeight;

				::CGContextClearRect(context, vr);
				
				::CGContextDrawImage(context,vr,inPrev);
				::CGContextFlush(context);
				CGContextRelease(context); 
			}

		}
		
		for(long y=0;y<height;y++)
		{
			unsigned char* src_lineptr=inFrame->fPixelsBuffer + (y*width);
			uLONG* dst_lineptr= (uLONG*)(bitmapData + ((y + inFrame->fY)*bitmapBytesPerRow));
			if(y + inFrame->fY < fHeight)
			{
				for(long x=0;x<width;x++)
				{
					uLONG lcol,lcol1;
					VColor col;
					unsigned char* src_pixptr=&src_lineptr[x];
					if(x + inFrame->fX < fWidth)
					{
						uLONG* dst_pixptr= &dst_lineptr[x + inFrame->fX];
						col=inFrame->GetPaletteEntry(*src_pixptr);
						if(col.GetAlpha())
							*dst_pixptr=col.GetRGBAColor(true);
					}
				}
			}
		}
		CGDataProviderRef dataprov= xV4DPicture_MemoryDataProvider::CGDataProviderCreate((char*)bitmapData,bitmapByteCount,true);
		frame=CGImageCreate(fWidth,fHeight,8,32,bitmapBytesPerRow,colorSpace,kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host ,dataprov,0,0,kCGRenderingIntentDefault);
		CGDataProviderRelease(dataprov);
		free((void*)bitmapData);		
	}
	CGColorSpaceRelease( colorSpace );
	fCGImageList.push_back(frame);
}
CGImageRef xGifInfo_QTImage::GetNthFrame(sLONG inIndex)
{
	sLONG nbframe=fCGImageList.size();
	
	if(inIndex<nbframe)
		return fCGImageList[inIndex];
	else
		return 0;
}

VPictureData_GIF::VPictureData_GIF()
:VPictureData_CGImage()
{
	_Init();	
}
VPictureData_GIF::VPictureData_GIF(VPictureDataProvider* inDataProvider,_VPictureAccumulator* inRecorder)
:VPictureData_CGImage(inDataProvider,inRecorder)
{
	_Init();
}
VPictureData_GIF::VPictureData_GIF(const VPictureData_GIF& inData)
:VPictureData_CGImage(inData)
{
	_Init();
}

VPictureData_GIF::~VPictureData_GIF()
{
	delete fGifInfo;
}

sLONG VPictureData_GIF::GetFrameCount()const
{
	if(fGifInfo)
		return fGifInfo->GetFrameCount();
	else
		return 1;
}

void VPictureData_GIF::_Init()
{
	fGifInfo=0;
}
void VPictureData_GIF::_DoReset()const
{
	delete fGifInfo;
	fGifInfo=0;
}
VError VPictureData_GIF::Save(VBlob* inData,VIndex inOffset,VSize& outSize,_VPictureAccumulator* inRecorder)const
{
	return inherited::Save(inData,inOffset,outSize,inRecorder);
}
VPictureData* VPictureData_GIF::Clone()const
{
	return new VPictureData_GIF(*this);
}
CGImageRef	VPictureData_GIF::GetCGImage()const
{
	_Load();
	if(fGifInfo)
	{
		return fGifInfo->GetNthFrame(0);
	}
	else
		return 0;
}
void VPictureData_GIF::_DoLoad()const
{
	if(!GetDataProvider())
		return;
	
	VPictureDataStream st(GetDataProvider());
	fGifInfo=new xGifInfo_QTImage(&st);
	fBounds.SetCoords(0,0,fGifInfo->fWidth,fGifInfo->fHeight);
}

void VPictureData_GIF::BuildTimeLine(std::vector<sLONG>& inTimeLine,sLONG &outLoopCount)const
{
	_Load();
	if(fGifInfo)
	{
		inTimeLine.clear();
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
	else
	{
		inTimeLine.clear();
		outLoopCount=0;
	}
}

void VPictureData_GIF::DrawInPortRef(PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet)const
{
	_Load();	// sc 09/09/2008 ACI0058950
	if(fGifInfo && inSet)
	{
		CGImageRef cgim=fGifInfo->GetNthFrame(inSet->GetFrameNumber());
		if(cgim)
		{
			xDraw(cgim,inPortRef,r,inSet);
		}
	}
}

void VPictureData_GIF::DrawInCGContext(CGContextRef inDC,const VRect& r,VPictureDrawSettings* inSet)const
{
	_Load();	// sc 09/09/2008 ACI0058950
	if(fGifInfo && inSet)
	{
		CGImageRef cgim=fGifInfo->GetNthFrame(inSet->GetFrameNumber());
		if(cgim)
		{
			xDraw(cgim,inDC,r,inSet);
		}
	}
}


/********************************************/

VPictureData_PDF::VPictureData_PDF()
:VPictureData_Vector()
{
	_SetDecoderByExtension("4DMemoryVector");
	_Init();
}


VPictureData_PDF::VPictureData_PDF(VPictureDataProvider* inDataProvider,_VPictureAccumulator* inRecorder)
:VPictureData_Vector(inDataProvider,inRecorder)
{
	_Init();
}

VPictureData_PDF::VPictureData_PDF(const VPictureData_PDF& inData)
:VPictureData_Vector(inData)
{
	_Init();
}


VPictureData_PDF::~VPictureData_PDF()
{
	_DoReset();
}

VPictureData_PDF::VPictureData_PDF(PortRef inPortRef,const VRect& inBounds)
{
	_SetDecoderByExtension("4DMemoryVector");
	_Init();
}

VPictureData* VPictureData_PDF::Clone()const
{
	return new VPictureData_PDF(*this);
}

void VPictureData_PDF::_Init()
{
	fPDFDocRef=NULL;
}

VError VPictureData_PDF::Save(VBlob* inData,VIndex inOffset,VSize& outSize,_VPictureAccumulator* inRecorder)const
{
	VError result=VE_UNKNOWN_ERROR;
	if(fDataProvider)
	{
		result= inherited::Save(inData,inOffset,outSize,inRecorder);
	}
	else
	{
		if(fPDFDocRef)
		{
			// pas de datasource, on sauve en pdf
			VPictureCodecFactoryRef fact;
			VBlobWithPtr blob;
			result=fact->EncodeToBlob(*this,"application/pdf",blob);
			if(result==VE_OK)
			{
				outSize=blob.GetSize();
				result=inData->PutData(blob.GetDataPtr(),outSize,inOffset);
			}
		}
	}
	return result;
}

void VPictureData_PDF::DrawInCGContext(CGContextRef inDC,const VRect& r,VPictureDrawSettings* inSet)const
{
	_Load();
	if(fPDFDocRef)
	{
		xDraw(fPDFDocRef, inDC,r,inSet);
	}
}

void VPictureData_PDF::DrawInPortRef(PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet)const
{
	_Load();
	if(fPDFDocRef)
	{
		xDraw(fPDFDocRef, inPortRef,r,inSet);
	}
}

void VPictureData_PDF::_DoReset()const
{
	if(fPDFDocRef)
		CGPDFDocumentRelease(fPDFDocRef);
	
	fPDFDocRef=0;

}
void VPictureData_PDF::_DoLoad()const
{

	if(fPDFDocRef)
		return;
	VPictureDataProvider *datasrc=GetDataProvider();
	if(datasrc!=NULL)
	{
		CGDataProviderRef dataref=xV4DPicture_MemoryDataProvider::CGDataProviderCreate(datasrc);
		fPDFDocRef=CGPDFDocumentCreateWithProvider(dataref);
		CGDataProviderRelease(dataref);
		if(fPDFDocRef)
		{
			CGRect cgr;
			CGPDFPageRef page = CGPDFDocumentGetPage (fPDFDocRef,1);
			if(page)
			{
				cgr = CGPDFPageGetBoxRect(page,kCGPDFMediaBox);
				cgr = CGPDFPageGetBoxRect(page,kCGPDFCropBox);
				cgr = CGPDFPageGetBoxRect(page,kCGPDFBleedBox);
				cgr = CGPDFPageGetBoxRect(page,kCGPDFTrimBox);
				cgr = CGPDFPageGetBoxRect(page,kCGPDFArtBox);
				fBounds.SetCoords(0,0,cgr.size.width,cgr.size.height);
			}
		}
	}
}

void VPictureData_PDF::FromVFile(VFile& inFile)
{
	_ReleaseDataProvider();
	if(fPDFDocRef)
		CGPDFDocumentRelease(fPDFDocRef);
	
	VPictureDataProvider* datasrc=VPictureDataProvider::Create(inFile);
	if(datasrc)
	{
		SetDataProvider(datasrc);
		datasrc->Release();
		_Load();
	}
}

CGPDFDocumentRef	VPictureData_PDF::GetPDF()const
{
	_Load();
	return fPDFDocRef;
}
#if 0 
VPictureData* VPictureData_PDF::BuildThumbnail(sLONG inWidth,sLONG inHeight,PictureMosaic inMode,VPictureDrawSettings* inSet)
{
	return 0;
}
#endif

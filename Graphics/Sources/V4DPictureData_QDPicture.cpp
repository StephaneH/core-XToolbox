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

#if ENABLE_D2D
#include "XWinD2DGraphicContext.h"
#endif

#if VERSIONMAC && !WITH_QUICKDRAW && !ARCH_64

// some QD apis removed from OSX sdk 10.7
extern "C" CGRect QDPictGetBounds(struct QDPict* pictRef);
extern "C" void QDPictRelease(struct QDPict* pictRef);
extern "C" Rect *QDGetPictureBounds(Picture** picH, Rect *outRect);
extern "C" struct QDPict* QDPictCreateWithProvider(CGDataProviderRef provider);
extern "C" OSStatus QDPictDrawToCGContext(CGContextRef ctx, CGRect rect, struct QDPict* pictRef);

#endif


#if VERSIONWIN || (VERSIONMAC && ARCH_32)

VPictureData_MacPicture::VPictureData_MacPicture()
:inherited()
{
	_SetDecoderByExtension(sCodec_pict);
	fPicHandle = NULL;
	fMetaFile = NULL;
#if VERSIONWIN
	fGdiplusMetaFile = NULL;
#endif
	fTrans = NULL;
}

VPictureData_MacPicture::VPictureData_MacPicture(VPictureDataProvider* inDataProvider, _VPictureAccumulator* inRecorder)
:inherited(inDataProvider, inRecorder)
{
	_SetDecoderByExtension(sCodec_pict);
	fPicHandle = NULL;
	fMetaFile = NULL;
	fTrans = NULL;
#if VERSIONWIN
	fGdiplusMetaFile = NULL;
#endif
}
VPictureData_MacPicture::VPictureData_MacPicture(const VPictureData_MacPicture& inData)
:inherited(inData)
{
	_SetDecoderByExtension(sCodec_pict);
	fPicHandle = NULL;
	fMetaFile = NULL;
	fTrans = NULL;
#if VERSIONWIN
	fGdiplusMetaFile = NULL;
#endif
	if (!fDataProvider && inData.fPicHandle && GetMacAllocator())
	{
		fPicHandle = GetMacAllocator()->Duplicate(inData.fPicHandle);
	}
}

VPictureData_MacPicture::~VPictureData_MacPicture()
{
	if (fPicHandle && GetMacAllocator())
	{
		if (GetMacAllocator()->CheckBlock(fPicHandle))
			GetMacAllocator()->Free(fPicHandle);
	}
	_DisposeMetaFile();
}

#if VERSIONWIN || WITH_QUICKDRAW
VPictureData_MacPicture::VPictureData_MacPicture(PortRef inPortRef, const VRect& inBounds)
{
	_SetDecoderByExtension(sCodec_pict);
	fPicHandle = NULL;
	fMetaFile = NULL;
	fTrans = NULL;
#if VERSIONWIN
	inPortRef; // pp pour warning
	inBounds;
	fGdiplusMetaFile = NULL;
#endif
#if VERSIONMAC
	GrafPtr oldPort;
	GetPort(&oldPort);
	SetPort(inPortRef);
	RGBColor saveFore, saveBack, white = { 0xffff, 0xffff, 0xffff }, black = { 0, 0, 0 };
	Rect r;

	inBounds.MAC_ToQDRect(r);

	GetBackColor(&saveFore);
	GetForeColor(&saveFore);

	RGBBackColor(&white);
	RGBForeColor(&black);

	PicHandle	newPicture = ::OpenPicture(&r);
	const BitMap*	portBits = ::GetPortBitMapForCopyBits(inPortRef);

	::ClipRect(&r);
	::CopyBits(portBits, portBits, &r, &r, srcCopy, NULL);
	::ClosePicture();

	RGBBackColor(&saveFore);
	RGBForeColor(&saveFore);

	SetPort(oldPort);
	fPicHandle = newPicture;

#if VERSIONWIN
	qtime::Rect rect;
	QDGetPictureBounds((qtime::Picture**)fPicHandle, &rect);
	fBounds.SetCoords(0, 0, rect.right - rect.left, rect.bottom - rect.top);
#else
	Rect rect;
	QDGetPictureBounds((Picture**) fPicHandle, &rect);
	fBounds.SetCoords(0, 0, rect.right - rect.left, rect.bottom - rect.top);
#endif

#if VERSIONMAC
	if (fPicHandle)
	{
		GetMacAllocator()->Lock(fPicHandle);
		CGDataProviderRef dataprov = xV4DPicture_MemoryDataProvider::CGDataProviderCreate(*(char**) fPicHandle, GetMacAllocator()->GetSize(fPicHandle), true);
		GetMacAllocator()->Unlock(fPicHandle);
		fMetaFile = QDPictCreateWithProvider(dataprov);
		CGDataProviderRelease(dataprov);
	}
#endif
#endif
}
#endif // VERSIONWIN || WITH_QUICKDRAW


#if VERSIONWIN
Gdiplus::Metafile*	VPictureData_MacPicture::GetGDIPlus_Metafile() const
{
	_Load();
	return fGdiplusMetaFile;
}
HENHMETAFILE	VPictureData_MacPicture::GetMetafile() const
{
	_Load();
	return fMetaFile;
}

#if ENABLE_D2D
void VPictureData_MacPicture::DrawInD2DGraphicContext(VWinD2DGraphicContext* inDC, const VRect& inBounds, VPictureDrawSettings* inSet)const
{
	VPictureDrawSettings drset(inSet);

	_Load();
	bool istrans = false;
	if (inSet && (inSet->GetDrawingMode() == 1 || (inSet->GetDrawingMode() == 2 && inSet->GetAlpha() != 100)))
		istrans = true;

	// pp on utilsie pas l'emf pour l'impression (bug hair line)
	// sauf !!! si l'appel viens du write ou view
	// Dans ce cas, l'impression ne passe plus du tout par altura, donc impossible d'utiliser drawpicture
	if (fMetaFile && (!drset.IsDevicePrinter() || drset.CanUseEMFForMacPicture()))
	{
		drset.SetInterpolationMode(0);
		if (istrans)
		{
			if (!fTrans)
			{
				_CreateTransparent(&drset);
			}
			if (fTrans)
			{
				xDraw(fTrans, inDC, inBounds, &drset);
			}
			else
			{
				//here we need to get a HDC from D2D render target 
				//because otherwise D2D render target does not support windows metafiles
				//(if called between BeginUsingContext/EndUsingContext,
				// this will force a EndDraw() before getting hdc and a BeginDraw() to resume drawing after releasing hdc)				
				StSaveContext_NoRetain saveCtx(inDC);
				HDC hdc = inDC->BeginUsingParentContext();
				if (hdc)
					xDraw(fMetaFile, hdc, inBounds, &drset);
				inDC->EndUsingParentContext(hdc);
			}
		}
		else
		{
			//here we need to get a HDC from D2D render target 
			//because otherwise D2D render target does not support windows metafiles
			//(if called between BeginUsingContext/EndUsingContext,
			// this will force a EndDraw() before getting hdc and a BeginDraw() to resume drawing after releasing hdc)				
			StSaveContext_NoRetain saveCtx(inDC);
			HDC hdc = inDC->BeginUsingParentContext();
			if (hdc)
				xDraw(fMetaFile, hdc, inBounds, &drset);
			inDC->EndUsingParentContext(hdc);
		}
		return;
	}
	else if (GetPicHandle() && sQDBridge)
	{
		VRect dstRect, srcRect;
		_CalcDestRect(inBounds, dstRect, srcRect, drset);

		xMacRect r;
		r.left = dstRect.GetX();
		r.top = dstRect.GetY();
		r.right = dstRect.GetX() + dstRect.GetWidth();
		r.bottom = dstRect.GetY() + dstRect.GetHeight();

		if (drset.GetScaleMode() == PM_TOPLEFT)
		{
			sLONG ox, oy;
			drset.GetOrigin(ox, oy);
			r.left -= ox;
			r.top -= oy;
			r.right = r.left + GetWidth();
			r.bottom = r.top + GetHeight();
		}
		{
			//here we need to get a HDC from D2D render target 
			//(if called between BeginUsingContext/EndUsingContext,
			// this will force a EndDraw() before getting hdc and a BeginDraw() to resume drawing after releasing hdc)				
			StSaveContext_NoRetain saveCtx(inDC);
			HDC hdc = inDC->BeginUsingParentContext();
			if (hdc)
				sQDBridge->DrawInMacPort(hdc, *this, (xMacRect*) &r, drset.GetDrawingMode() == 1, drset.GetScaleMode() == PM_TILE);
			inDC->EndUsingParentContext(hdc);
		}
	}
}
#endif

void VPictureData_MacPicture::DrawInGDIPlusGraphics(Gdiplus::Graphics* inDC, const VRect& inBounds, VPictureDrawSettings* inSet) const
{
	VPictureDrawSettings drset(inSet);

	_Load();
	bool istrans = false;
	if (inSet && (inSet->GetDrawingMode() == 1 || (inSet->GetDrawingMode() == 2 && inSet->GetAlpha() != 100)))
		istrans = true;

	// pp on utilsie pas l'emf pour l'impression (bug hair line)
	// sauf !!! si l'appel viens du write ou view
	// Dans ce cas, l'impression ne passe plus du tout par altura, donc impossible d'utiliser drawpicture
	if (fMetaFile && (!drset.IsDevicePrinter() || drset.CanUseEMFForMacPicture()))
	{
		drset.SetInterpolationMode(0);
		if (istrans)
		{
			if (!fTrans)
			{
				_CreateTransparent(&drset);
			}
			if (fTrans)
			{
				xDraw(fTrans, inDC, inBounds, &drset);
			}
			else
				xDraw(fMetaFile, inDC, inBounds, &drset);
		}
		else
		{
			xDraw(fMetaFile, inDC, inBounds, &drset);
		}
		return;
	}
	else if (GetPicHandle() && sQDBridge)
	{
		VRect dstRect, srcRect;
		_CalcDestRect(inBounds, dstRect, srcRect, drset);

		xMacRect r;
		r.left = dstRect.GetX();
		r.top = dstRect.GetY();
		r.right = dstRect.GetX() + dstRect.GetWidth();
		r.bottom = dstRect.GetY() + dstRect.GetHeight();


		if (drset.GetScaleMode() == PM_TOPLEFT)
		{
			sLONG ox, oy;
			drset.GetOrigin(ox, oy);
			r.left -= ox;
			r.top -= oy;
			r.right = r.left + GetWidth();
			r.bottom = r.top + GetHeight();
		}
		{
			StUseHDC dc(inDC);
			sQDBridge->DrawInMacPort(dc, *this, (xMacRect*) &r, drset.GetDrawingMode() == 1, drset.GetScaleMode() == PM_TILE);
		}
	}
}
void		VPictureData_MacPicture::_CreateTransparent(VPictureDrawSettings* /*inSet*/)const
{
	if (fTrans)
		return;
	if (fMetaFile)
	{
		fTrans = new Gdiplus::Bitmap(GetWidth(), GetHeight(), PixelFormat32bppRGB);
		if (fTrans)
		{
			Gdiplus::Metafile gdipmeta(fMetaFile, false);
			fTrans->SetResolution(gdipmeta.GetHorizontalResolution(), gdipmeta.GetVerticalResolution());
			{
				Gdiplus::Graphics graph(fTrans);
				graph.Clear(Gdiplus::Color(0xff, 0xff, 0xff));
				graph.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
				graph.DrawImage(&gdipmeta, 0, 0, 0, 0, GetWidth(), GetHeight(), Gdiplus::UnitPixel);
			}
		}
	}
}
#elif VERSIONMAC
void		VPictureData_MacPicture::_CreateTransparent(VPictureDrawSettings* inSet)const
{
	CGImageRef result = 0;

	if (fTrans)
		return;
	VPictureDrawSettings set(inSet);
	if (GetWidth() && GetHeight())
	{
		if (fMetaFile)
		{

			CGContextRef    context = NULL;
			CGColorSpaceRef colorSpace;
			void *          bitmapData;
			int             bitmapByteCount;
			int             bitmapBytesPerRow;
			sLONG width, height;

			CGRect cgr = QDPictGetBounds(fMetaFile);

			width = cgr.size.width;
			height = cgr.size.height;
			bitmapBytesPerRow = (4 * width + 15) & ~15;
			bitmapByteCount = (bitmapBytesPerRow * height);

			bitmapData = malloc(bitmapByteCount);
			if (bitmapData)
			{
				colorSpace = CGColorSpaceCreateDeviceRGB();//CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);

				memset(bitmapData, 0, bitmapByteCount);
				context = CGBitmapContextCreate(bitmapData,
					width,
					height,
					8,      // bits per component
					bitmapBytesPerRow,
					colorSpace,
					kCGImageAlphaNoneSkipLast);
				if (context)
				{
					CGRect vr;
					vr.origin.x = 0;
					vr.origin.y = 0;
					vr.size.width = width;
					vr.size.height = height;

					VRect pictrect(0, 0, width, height);
					set.SetScaleMode(PM_SCALE_TO_FIT);
					set.SetYAxisSwap(height, true);
					set.SetPictureMatrix(VAffineTransform());
					CGContextSaveGState(context);

					CGContextSetRGBFillColor(context, 1, 1, 1, 1);
					CGContextFillRect(context, vr);

					xDraw(fMetaFile, context, pictrect, &set);

					CGContextRestoreGState(context);
					CGDataProviderRef dataprov = xV4DPicture_MemoryDataProvider::CGDataProviderCreate((char*) bitmapData, bitmapByteCount, true);

					result = CGImageCreate(width, height, 8, 32, bitmapBytesPerRow, colorSpace, kCGImageAlphaNoneSkipLast, dataprov, 0, 0, kCGRenderingIntentDefault);
					CGContextRelease(context);
					CGDataProviderRelease(dataprov);
					if (set.GetDrawingMode() == 1)
					{
						const VColor& col = set.GetTransparentColor();
						CGFloat MaskingColors[6];
						MaskingColors[0] = col.GetRed();
						MaskingColors[1] = col.GetGreen();
						MaskingColors[2] = col.GetBlue();
						MaskingColors[3] = col.GetRed();
						MaskingColors[4] = col.GetGreen();
						MaskingColors[5] = col.GetBlue();

						CGImageRef trans = CGImageCreateWithMaskingColors(result, MaskingColors);
						if (trans)
						{
							CFRelease(result);
							fTrans = trans;
						}
					}
				}
				CGColorSpaceRelease(colorSpace);
			}
			free(bitmapData);
		}
	}
}


void VPictureData_MacPicture::DrawInCGContext(CGContextRef inDC, const VRect& r, VPictureDrawSettings* inSet) const
{
	_Load();
	if (fMetaFile)
	{
#if VERSIONWIN
		xDraw(fMetaFile, inDC, r, inSet);
#else
		if (inSet && inSet->GetDrawingMode() == 1)
		{
			if (!fTrans)
			{
				_CreateTransparent(inSet);
			}
			if (fTrans)
				xDraw(fTrans, inDC, r, inSet);
			else
				xDraw(fMetaFile, inDC, r, inSet);

		}
		else
			xDraw(fMetaFile, inDC, r, inSet);
#endif
	}
	else if (GetPicHandle())
		xDraw(GetPicHandle(), inDC, r, inSet);
}

#endif

void VPictureData_MacPicture::DrawInPortRef(PortRef inPortRef, const VRect& inBounds, VPictureDrawSettings* inSet) const
{
	VPictureDrawSettings drset(inSet);

	_Load();
	bool istrans = false;
	if (inSet && (inSet->GetDrawingMode() == 1 || (inSet->GetDrawingMode() == 2 && inSet->GetAlpha() != 100)))
		istrans = true;

	// pp on utilsie pas l'emf pour l'impression (bug hair line)
	// sauf !!! si l'appel viens du write ou view
	// Dans ce cas, l'impression ne passe plus du tout par altura, donc impossible d'utiliser drawpicture
	if (fMetaFile && (!drset.IsDevicePrinter() || drset.CanUseEMFForMacPicture()))
	{
		drset.SetInterpolationMode(0);
		if (istrans)
		{
			if (!fTrans)
			{
				_CreateTransparent(&drset);
			}
			if (fTrans)
			{
				xDraw(fTrans, inPortRef, inBounds, &drset);
			}
			else
				xDraw(fMetaFile, inPortRef, inBounds, &drset);
		}
		else
		{
			xDraw(fMetaFile, inPortRef, inBounds, &drset);
		}
		return;
	}
	else if (GetPicHandle() && sQDBridge)
	{
		VRect dstRect, srcRect;
		_CalcDestRect(inBounds, dstRect, srcRect, drset);

		xMacRect r;
		r.left = dstRect.GetX();
		r.top = dstRect.GetY();
		r.right = dstRect.GetX() + dstRect.GetWidth();
		r.bottom = dstRect.GetY() + dstRect.GetHeight();


		if (drset.GetScaleMode() == PM_TOPLEFT)
		{
			sLONG ox, oy;
			drset.GetOrigin(ox, oy);
			r.left -= ox;
			r.top -= oy;
			r.right = r.left + GetWidth();
			r.bottom = r.top + GetHeight();
		}
		sQDBridge->DrawInMacPort(inPortRef, *this, (xMacRect*) &r, drset.GetDrawingMode() == 1, drset.GetScaleMode() == PM_TILE);
	}
}

VPictureData* VPictureData_MacPicture::Clone()const
{
	VPictureData_MacPicture* clone;
	clone = new VPictureData_MacPicture(*this);
	return clone;
}

VSize VPictureData_MacPicture::GetDataSize(_VPictureAccumulator* /*inRecorder*/) const
{
	if (fDataProvider)
		return (sLONG) fDataProvider->GetDataSize();
	else if (fPicHandle && GetMacAllocator())
	{
		return (sLONG) GetMacAllocator()->GetSize(fPicHandle);
	}
	return 0;
}

VError VPictureData_MacPicture::Save(VBlob* inData, VIndex inOffset, VSize& outSize, _VPictureAccumulator* inRecorder)const
{
	VError result = VE_OK;
	VSize headersize = 0;
	if (fDataProvider)
	{
		if (fDataProvider->IsFile())
		{
			if (inData->IsOutsidePath())
			{
				result = inherited::Save(inData, inOffset, outSize, inRecorder);
			}
			else
			{
				assert(fDataProvider->GetDataSize()>0x200);
				VPtr dataptr = fDataProvider->BeginDirectAccess();
				if (dataptr)
				{
					dataptr += 0x200;
					result = inData->PutData(dataptr, fDataProvider->GetDataSize() - 0x200, inOffset);
					fDataProvider->EndDirectAccess();
				}
				else
					fDataProvider->GetLastError();
			}
		}
		else
		{
			if (inData->IsOutsidePath())
			{
				char buff[0x200];
				headersize = 0x200;
				memset(buff, 0, headersize);
				result = inData->PutData(buff, headersize, inOffset);
				inOffset += headersize;

				inherited::Save(inData, inOffset, outSize, inRecorder);

				outSize += headersize;
			}
			else
			{
				result = inherited::Save(inData, inOffset, outSize, inRecorder);
			}
		}
	}
	else
	{
		if (GetMacAllocator())
		{
			if (fPicHandle)
			{
				GetMacAllocator()->Lock(fPicHandle);
				VPtr p = *(char**) fPicHandle;
				if (p)
				{

					outSize = GetMacAllocator()->GetSize(fPicHandle);

					if (inData->IsOutsidePath())
					{
						char buff[0x200];
						headersize = 0x200;
						memset(buff, 0, headersize);
						result = inData->PutData(buff, headersize, inOffset);
						inOffset += headersize;
					}

#if VERSIONWIN
					ByteSwapWordArray((sWORD*) p, 5);
#endif
					result = inData->PutData(p, outSize, inOffset);
#if VERSIONWIN
					ByteSwapWordArray((sWORD*) p, 5);
#endif
					outSize += headersize;
				}
				GetMacAllocator()->Unlock(fPicHandle);
			}
		}
		else
			result = VE_UNIMPLEMENTED;
	}
	return result;
}

void VPictureData_MacPicture::_DisposeMetaFile()const
{
	if (fMetaFile)
	{
#if VERSIONWIN
		DeleteEnhMetaFile(fMetaFile);
#else
		QDPictRelease(fMetaFile);
#endif
		fMetaFile = NULL;
	}
	if (fTrans)
	{
#if VERSIONWIN
#if ENABLE_D2D
		VWinD2DGraphicContext::ReleaseBitmap(fTrans);
#endif
		delete fTrans;
#elif VERSIONMAC
		CFRelease(fTrans);
#endif
		fTrans = NULL;
	}
#if VERSIONWIN
	if (fGdiplusMetaFile)
		delete fGdiplusMetaFile;
	fGdiplusMetaFile = 0;
#endif
}
void VPictureData_MacPicture::DoDataSourceChanged()
{
	if (fPicHandle && GetMacAllocator())
	{
		GetMacAllocator()->Free(fPicHandle);
		fPicHandle = 0;
	}
	_DisposeMetaFile();
}

VPictureData_MacPicture::VPictureData_MacPicture(xMacPictureHandle inMacPicHandle)
:VPictureData_Vector()
{
	_SetDecoderByExtension(sCodec_pict);
	fDataSourceDirty = true;
	fDataSourceMetadatasDirty = true;
	fPicHandle = NULL;
	fMetaFile = NULL;
	fTrans = NULL;
#if VERSIONWIN
	fGdiplusMetaFile = 0;
#endif

	if (inMacPicHandle)
	{

#if VERSIONWIN // pp c'est une resource, sous windows elle est deja swap√©
		ByteSwapWordArray((sWORD*) (*inMacPicHandle), 5);
#endif

		VPictureDataProvider *prov = VPictureDataProvider::Create((xMacHandle) inMacPicHandle, false);

#if VERSIONWIN // pp c'est une resource, sous windows elle est deja swap√©
		ByteSwapWordArray((sWORD*) (*inMacPicHandle), 5);
#endif

		if (prov)
		{
			prov->SetDecoder(fDecoder);
			SetDataProvider(prov, true);
			prov->Release();
		}

	}
}

void VPictureData_MacPicture::_DoReset()const
{
	_DisposeMetaFile();
	if (fPicHandle && GetMacAllocator())
	{
		GetMacAllocator()->Free(fPicHandle);
		fPicHandle = 0;
	}
}

void* VPictureData_MacPicture::_BuildPicHandle()const
{
	void* outPicHandle = NULL;
	VIndex offset = 0;
	if (GetMacAllocator() && fDataProvider && fDataProvider->GetDataSize())
	{
		if (fDataProvider->IsFile())
		{
			outPicHandle = GetMacAllocator()->Allocate(fDataProvider->GetDataSize() - 0x200);
			offset = 0x200;
		}
		else
			outPicHandle = GetMacAllocator()->Allocate(fDataProvider->GetDataSize());
	}
	if (outPicHandle)
	{
		VSize datasize = fDataProvider->GetDataSize();
		if (fDataProvider->IsFile())
		{
			datasize -= 0x200;
		}
		GetMacAllocator()->Lock(outPicHandle);
		fDataProvider->GetData(*(char**) outPicHandle, offset, datasize);
#if VERSIONWIN
		ByteSwapWordArray(*(sWORD**) outPicHandle, 5);
#endif
		GetMacAllocator()->Unlock(outPicHandle);
	}
	return outPicHandle;
}

void VPictureData_MacPicture::_DoLoad()const
{
	if (fMetaFile)
		return;
	VIndex offset = 0;
	if (fDataProvider && fDataProvider->GetDataSize() && !fPicHandle)
	{
		fPicHandle = _BuildPicHandle();
	}
	if (fPicHandle)
	{
#if VERSIONWIN
		xMacRect rect;
		rect = (*((xMacPicture**) fPicHandle))->picFrame;
		//QDGetPictureBounds((qtime::Picture**)fPicHandle,&rect);
		fBounds.SetCoords(0, 0, rect.right - rect.left, rect.bottom - rect.top);
#elif VERSIONMAC
		Rect rect;
		QDGetPictureBounds((Picture**) fPicHandle, &rect);
		fBounds.SetCoords(0, 0, rect.right - rect.left, rect.bottom - rect.top);
#endif
	}
	if (fPicHandle)
	{
#if VERSIONMAC
		GetMacAllocator()->Lock(fPicHandle);
		CGDataProviderRef dataprov = xV4DPicture_MemoryDataProvider::CGDataProviderCreate(*(char**) fPicHandle, GetMacAllocator()->GetSize(fPicHandle), true);
		GetMacAllocator()->Unlock(fPicHandle);
		fMetaFile = QDPictCreateWithProvider(dataprov);
		CGDataProviderRelease(dataprov);
#elif VERSIONWIN
		if (sQDBridge)
			fMetaFile = sQDBridge->ToMetaFile(fPicHandle);
#endif
	}
}

VError VPictureData_MacPicture::SaveToFile(VFile& inFile)const
{
	VError result = VE_OK;
	if (fDataProvider && fDataProvider->GetDataSize())
	{
		VPtr p = fDataProvider->BeginDirectAccess();
		if (p)
		{
			result = inFile.Create();
			if (result == VE_OK)
			{
				VFileDesc* inFileDesc;
				result = inFile.Open(FA_READ_WRITE, &inFileDesc);
				if (result == VE_OK)
				{
					char buff[0x200];
					memset(buff, 0, 0x200);
					result = inFileDesc->PutData(buff, 0x200, 0);
					if (result == VE_OK)
					{
						result = inFileDesc->PutData(p, fDataProvider->GetDataSize(), 0x200);
					}
					delete inFileDesc;
				}
			}
			fDataProvider->EndDirectAccess();
		}
		else
			fDataProvider->ThrowLastError();

	}
	else
	{
		VBlobWithPtr blob;
		VSize outsize;
		result = Save(&blob, 0, outsize);
		if (result == VE_OK)
		{
			result = inFile.Create();
			if (result == VE_OK)
			{
				VFileDesc* inFileDesc;
				result = inFile.Open(FA_READ_WRITE, &inFileDesc);
				if (result == VE_OK)
				{
					char buff[0x200];
					memset(buff, 0, 0x200);
					result = inFileDesc->PutData(buff, 0x200, 0);
					if (result == VE_OK)
					{
						result = inFileDesc->PutData(blob.GetDataPtr(), blob.GetSize(), 0x200);
					}
					delete inFileDesc;
				}
			}
		}
	}
	return result;
}

#if WITH_VMemoryMgr
xMacPictureHandle	VPictureData_MacPicture::CreatePicHandle(VPictureDrawSettings* inSet, bool& outCanAddPicEnd) const
{
	void* result = NULL;
	VPictureDrawSettings set(inSet);
	outCanAddPicEnd = true;
	set.SetBackgroundColor(VColor(0xff, 0xff, 0xff));// pp no alpha
	if (set.IsIdentityMatrix())
	{
		result = _BuildPicHandle();
	}
	if (sQDBridge)
	{
		if (!result) // YT & PP - 24-Nov-2008 - ACI0059927 & ACI0059923
#if VERSIONWIN
		{
			Gdiplus::Bitmap* bm = CreateGDIPlus_Bitmap(&set);
			if (bm)
			{
				HBITMAP hbm;
				bm->GetHBITMAP(Gdiplus::Color(0xff, 0xff, 0xff), &hbm);
				if (hbm)
				{
					result = sQDBridge->HBitmapToPicHandle(hbm);
					DeleteObject(hbm);
				}
				delete bm;
			}
		}
#elif VERSIONMAC
		{
			result = sQDBridge->VPictureDataToPicHandle(*this, inSet);
		}
#endif
	}
	return (xMacPictureHandle) result;
}
#endif

#if WITH_VMemoryMgr
xMacPictureHandle VPictureData_MacPicture::GetPicHandle()const
{
	_Load();
	if (fPicHandle)
	{
		assert(GetMacAllocator()->CheckBlock(fPicHandle));
	}
	return (xMacPictureHandle) fPicHandle;
}
#endif
#endif // end version 64 bit
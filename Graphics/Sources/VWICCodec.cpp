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
#include "ImageMeta.h"

#include <wincodec.h>
#include <wincodecsdk.h>
#pragma comment(lib, "WindowsCodecs.lib")
#include <winuser.h>
#include <atlcomcli.h>

#if VERSIONDEBUG
#define TEST_METADATAS 1
#else
#define TEST_METADATAS 0
#endif


XBOX::VCriticalSection VPictureCodec_WIC::sLock;

//uncomment to read only XMP metadatas
//(by default, standard metadatas are browsed too for best compatibility)
//#define DEBUG_XMP 1


/** WIC gdiplus bitmap source class

	can be used to encode directly a gdiplus bitmap in a frame with IWICBitmapFrameEncode::WriteSource
*/
class VWICBitmapSource_Gdiplus : public IWICBitmapSource 
{
protected:
	/** QueryInerface ref count */
    LONG fcRef; 

	/** gdiplus bitmap ref */
	Gdiplus::Bitmap *fBmp;
public:
	VWICBitmapSource_Gdiplus( Gdiplus::Bitmap *inBmp);
	virtual ~VWICBitmapSource_Gdiplus();

    HRESULT STDMETHODCALLTYPE QueryInterface( 
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ __RPC__deref_out void __RPC_FAR *__RPC_FAR *ppvObject);

    ULONG STDMETHODCALLTYPE AddRef( void);

    ULONG STDMETHODCALLTYPE Release( void);

    HRESULT STDMETHODCALLTYPE GetSize( 
        /* [out] */ __RPC__out UINT *puiWidth,
        /* [out] */ __RPC__out UINT *puiHeight);
    
    HRESULT STDMETHODCALLTYPE GetPixelFormat( 
        /* [out] */ __RPC__out WICPixelFormatGUID *pPixelFormat);
    
    HRESULT STDMETHODCALLTYPE GetResolution( 
        /* [out] */ __RPC__out double *pDpiX,
        /* [out] */ __RPC__out double *pDpiY);
    
    HRESULT STDMETHODCALLTYPE CopyPalette( 
        /* [in] */ __RPC__in_opt IWICPalette *pIPalette);
    
    HRESULT STDMETHODCALLTYPE CopyPixels( 
        /* [unique][in] */ __RPC__in_opt const WICRect *prc,
        /* [in] */ UINT cbStride,
        /* [in] */ UINT cbBufferSize,
        /* [size_is][out] */ __RPC__out_ecount_full(cbBufferSize) BYTE *pbBuffer);
};


VWICBitmapSource_Gdiplus::VWICBitmapSource_Gdiplus( Gdiplus::Bitmap *inBmp)
{
    fcRef = 1;
	xbox_assert(inBmp);
	fBmp = inBmp;
}


VWICBitmapSource_Gdiplus::~VWICBitmapSource_Gdiplus()
{
}


HRESULT STDMETHODCALLTYPE VWICBitmapSource_Gdiplus::QueryInterface( REFIID riid,void **ppvObject)
{
    if (riid == IID_IWICBitmapSource || 
        riid == IID_IUnknown)
    {
        InterlockedIncrement(&fcRef);
        *ppvObject = this;
        return S_OK;
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
}
    
ULONG STDMETHODCALLTYPE VWICBitmapSource_Gdiplus::AddRef()
{
    return InterlockedIncrement(&fcRef);
}
    
ULONG STDMETHODCALLTYPE VWICBitmapSource_Gdiplus::Release()
{
    LONG cRef = InterlockedDecrement(&fcRef);
    if (cRef == 0)
        delete this;
    return cRef;
}


HRESULT STDMETHODCALLTYPE VWICBitmapSource_Gdiplus::GetSize( 
    /* [out] */ __RPC__out UINT *puiWidth,
    /* [out] */ __RPC__out UINT *puiHeight)
{
	xbox_assert(fBmp);
	*puiWidth	= (UINT)fBmp->GetWidth();
	*puiHeight	= (UINT)fBmp->GetHeight();

	return S_OK;
}

HRESULT STDMETHODCALLTYPE VWICBitmapSource_Gdiplus::GetPixelFormat( 
    /* [out] */ __RPC__out WICPixelFormatGUID *pPixelFormat)
{
	xbox_assert(fBmp);
	Gdiplus::PixelFormat pf = fBmp->GetPixelFormat();
	*pPixelFormat = VPictureCodec_WIC::FromGdiplusPixelFormat( pf);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE VWICBitmapSource_Gdiplus::GetResolution( 
    /* [out] */ __RPC__out double *pDpiX,
    /* [out] */ __RPC__out double *pDpiY)
{
	xbox_assert(fBmp);
	*pDpiX = (double)fBmp->GetHorizontalResolution();
	*pDpiY = (double)fBmp->GetVerticalResolution();

	return S_OK;
}

HRESULT STDMETHODCALLTYPE VWICBitmapSource_Gdiplus::CopyPalette( 
    /* [in] */ __RPC__in_opt IWICPalette *pIPalette)
{
	HRESULT result = E_NOTIMPL;
	xbox_assert(sizeof(WICColor) == sizeof(Gdiplus::ARGB));
	xbox_assert(fBmp);

	//check if gdiplus bitmap has a palette
	if ((fBmp->GetPixelFormat() & PixelFormatIndexed) == 0)
		return E_NOTIMPL;

	//read gdiplus palette
	INT sizePalette = fBmp->GetPaletteSize();
	if (sizePalette)
	{
		char *bufPalette = new char [sizePalette];
		Gdiplus::ColorPalette *palette = (Gdiplus::ColorPalette *)bufPalette;

		Gdiplus::Status status = fBmp->GetPalette( palette, sizePalette);
		if (status == Gdiplus::Ok)
		{
			//copy gdiplus palette to WIC palette
			result = pIPalette->InitializeCustom( (WICColor *)&(palette->Entries[0]), palette->Count);
		}
		delete [] bufPalette;
	}
	return result;
}

HRESULT STDMETHODCALLTYPE VWICBitmapSource_Gdiplus::CopyPixels( 
    /* [unique][in] */ __RPC__in_opt const WICRect *inBounds,
    /* [in] */ UINT inBufferStride,
    /* [in] */ UINT inBufferSize,
    /* [size_is][out] */ __RPC__out_ecount_full(cbBufferSize) BYTE *inBuffer)
{
	xbox_assert(inBuffer);
	Gdiplus::BitmapData bmData;
	try
	{
		//determine source rect
		Gdiplus::Rect bounds;
		if (inBounds != NULL)
		{
			if (inBounds->X < 0)
				return E_INVALIDARG;
			if (inBounds->Y < 0)
				return E_INVALIDARG;
			if (inBounds->X+inBounds->Width > fBmp->GetWidth())
				return E_INVALIDARG;
			if (inBounds->Y+inBounds->Height > fBmp->GetHeight())
				return E_INVALIDARG;
			bounds.X		= inBounds->X;
			bounds.Y		= inBounds->Y;
			bounds.Width	= inBounds->Width;
			bounds.Height	= inBounds->Height;
		}
		else
		{
			bounds.X		= 0;
			bounds.Y		= 0;
			bounds.Width	= fBmp->GetWidth();
			bounds.Height	= fBmp->GetHeight();
		}

		//check buffer size
		uLONG sizeBufferRequested = inBufferStride*bounds.Height;
		if (inBufferSize < sizeBufferRequested)
			return E_INVALIDARG;

		//copy pixels from source rectangle to destination buffer
		bmData.PixelFormat	= fBmp->GetPixelFormat();
		bmData.Scan0		= (VOID *)inBuffer;
		bmData.Stride		= (INT)inBufferStride;
		bmData.Width		= (UINT)bounds.Width;
		bmData.Height		= (UINT)bounds.Height;
		fBmp->LockBits(	&bounds,
						Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeUserInputBuf,
						fBmp->GetPixelFormat(),
						&bmData);
		fBmp->UnlockBits(&bmData);

		return S_OK;
	}
	catch(...)
	{
		fBmp->UnlockBits(&bmData);
		xbox_assert(false);
		return E_ACCESSDENIED;
	}
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////




/** Image source class */
class XTOOLBOX_API stImageSource : public VObject, public IHelperMetadatas
{
public:
	/** metadata tag types */
	typedef enum eTagType
	{
		TT_AUTO,				//guess tag type based on PROPVARIANT type (metadata reading only)

		TT_BLOB_ASCII,			//blob with only ASCII characters (not including the NULL terminating character)
		TT_BLOB_EXIF_STRING,	//blob containing a Exif-compliant string (8-bytes header for ASCII charset string followed by the text)
		
		TT_EXIF_DATETIME,		//Exif datetime
		TT_XMP_DATETIME,		//XMP datetime

		TT_GPSVERSIONID,		//GPSVersionID tag type (use XMP format for bag storage)

		TT_XMP_SEQ,				//XMP sequence of string values 
		TT_XMP_BAG,				//XMP bag of string values 

		//following tags types are used only for metadatas writing
		//(for best compatibility, type of tag while writing is determined
		// using IPTC IIMv4/TIFF 6.0/EXIF 2.2/XMP specifications)

		TT_BLOB,				//blob
		TT_LPWSTR,				//string
		TT_UI1,					//unsigned char
		TT_UI2,					//unsigned short 
		TT_UI4,					//unsigned long
		TT_I1,					//signed char
		TT_I2,					//signed short 
		TT_I4,					//signed long
		TT_BOOL,				//boolean
		TT_UI8,					//unsigned rational
		TT_I8,					//signed rational

		TT_ARRAY_LPWSTR,		//array of strings
		TT_ARRAY_UI1,			//array of unsigned char
		TT_ARRAY_UI2,			//array of unsigned short 
		TT_ARRAY_UI4,			//array of unsigned long
		TT_ARRAY_I1,			//array of signed char
		TT_ARRAY_I2,			//array of signed short 
		TT_ARRAY_I4,			//array of signed long
		TT_ARRAY_BOOL,			//array of boolean
		TT_ARRAY_UI8,			//array of unsigned rational
		TT_ARRAY_I8,			//array of signed rational

		TT_XMP_SEQ_UI2,			//XMP sequence of unsigned short values 
		TT_XMP_SEQ_UI8,			//XMP sequence of unsigned rational values 

		TT_XMP_BAG_UI2,			//XMP bag of unsigned short values 
		TT_XMP_BAG_UI8,			//XMP bag of unsigned rational values 

		TT_XMP_LANG_ALT			//XMP lang alt value (we write only 'x-default' child element)


	} eTagType;
public:
	/** initialize image source from the specified data provider 
	@remarks
		if inGUIDFormatContainer is GUID_NULL,
			try to find a image decoder which can parse the input data source
		else
			it is assumed that decoder validation has been done yet
			so that we can use directly decoder associated with the specified GUID
	*/
	stImageSource(CComPtr<IWICImagingFactory>& inFactory, VPictureDataProvider* inDataProvider, const GUID& inGUIDFormatContainer);

	virtual	~stImageSource();
	
	stImageSource(CComPtr<IWICImagingFactory>& inFactory, const CComPtr<IWICBitmapSource>& inBitmapSource);

	/** true if image data provider is valid 
	 @remarks
		false if source data provider is not a valid image
	 */
	bool IsValid() const;
	
	/** convert bitmap source to the desired pixel format 
	@remarks
		this remains abstract until image source is effectively used
	*/
	void ConvertToPixelFormat( const Gdiplus::PixelFormat& inPixelFormat);

	/** convert bitmap source to the desired pixel format 
	@remarks
		this remains abstract until image source is effectively used
	*/
	void ConvertToPixelFormat( const WICPixelFormatGUID& inPixelFormat);

	/** convert colorspace to sRGB colorspace 
	@remarks
		must be used before using CopyPixels with gdiplus pixel buffer to ensure proper color conversion
	*/
	void ConvertToRGBColorSpace();

	/** get bitmap frame decoder */
	bool GetBitmapFrameDecoder( CComPtr<IWICBitmapFrameDecode>& outBitmapFrameDecoder)
	{
		if (!IsValid())
			return false;
		outBitmapFrameDecoder = fDecoderFrameWIC;
		return true;
	}

	/** get bitmap source */
	bool GetBitmapSource( CComPtr<IWICBitmapSource>& outBitmapSource)
	{
		if (!IsValid())
			return false;
		outBitmapSource = fBitmapSourceWIC;
		return true;
	}


	/** get image width */
	uLONG GetWidth() const;
	
	/** get image height */
	uLONG GetHeight() const;

	/** get image resolution */
	void GetResolution( Real& outDpiX, Real& outDpiY);

	/** get image source container format */
	const GUID& GetContainerFormat() const { return fImageContainerGUID; }

	/** get WIC pixel format */
	void GetPixelFormat( WICPixelFormatGUID& outPF) const;

	/** get Gdiplus pixel format 
	@remarks
		return PixelFormatUndefined if image source format is not supported by Gdiplus
	*/
	Gdiplus::PixelFormat GetPixelFormat() const;

	/** get row length in bytes */
	uLONG GetStride() const;

	/** get number of bits per pixel */
	uLONG GetBitDepth() const;

	/** true if image has alpha channel */
	bool hasAlpha() const;

	/** true if image has palette */
	bool hasPalette() const;

	/** copy image source palette to gdiplus bitmap palette */
	bool CopyPalette( Gdiplus::Bitmap *inBmp);

	/** decode and copy image source pixels to destination buffer */
	bool CopyPixels( void *inBuffer, uLONG inBufSize, uLONG inBufStride = 0);

	/** true if input pixel format has alpha channel */
	static bool HasPixelFormatAlpha( const WICPixelFormatGUID& inPF);

	/** true if input pixel format is indexed */
	static bool IsPixelFormatIndexed( const WICPixelFormatGUID& inPF);

	/** return count of bits per pixel for input pixel format */
	static uLONG GetPixelFormatBitDepth( const WICPixelFormatGUID& inPF);

	/** return row length in bytes using row width in pixels and number of bits per pixel */
	static uLONG GetStride( uLONG inWidth, uLONG inBPP);

	/** read image source metadatas */
	void ReadMetadatas( VValueBag *outMetadatas);

	/** direct copy image source metadatas to metadatas query writer 
	@remarks
		this is possible only if source image GUID == dest image GUID
	*/
	void CopyMetadatas( CComPtr<IWICMetadataQueryWriter>& inMetaWriter);

	/** write metadatas to the destination frame 
	@remarks
		if inImageSrcDataProvider != NULL, 
		first direct copy image source metadatas to image dest
	*/
	static bool WriteMetadatas(CComPtr<IWICImagingFactory>& ioFactory,
							   CComPtr<IWICBitmapFrameEncode>& ioFrame,
							   const VValueBag *inBagMetas,
							   const GUID& inGUIDFormatContainer,
							   uLONG inImageWidth,
							   uLONG inImageHeight,
							   Real inXResolution,
							   Real inYResolution,
							   VPictureDataProvider *inImageSrcDataProvider = NULL,
							   bool inEncodeImageFrames = true);

protected:

	/** convert from WIC metadata key-value pair to metadata bag key-value pair 
	@remarks
		WIC metadatas tag types are converted to bag values types as follow:

		VT_I1,VT_UI1,VT_I2,VT_UI2,VT_I4,VT_UI4,VT_BOOL <-> VLong 
		VT_UI8, VT_I8, VT_R4, VT_R8 <-> VReal
		VT_LPSTR,VT_LPWSTR <-> VString
		VT_BLOB <->	VString (encoding depending on size of VT_BLOB and if VT_BLOB contains only ASCII characters
							 or a Exif-compliant string - any number of characters preceded by a 8-bytes ASCII header which identifies character code)
		VT_UNKNOWN <-> VString (either a XMP bag or seq: rdf:li child elements values are encoded
								in a string separated by ';')

		if tag repeat count > 1 (prop variant type & VT_VECTOR != 0), 
		tag values are string-encoded (in a VString value) using ';' as separator
	@note
		WIC uses VT_UI8 and VT_I8 for RATIONAL and SRATIONAL
		(where real value is equal to lowPart/HighPart)
	*/
	static bool _WICKeyValuePairToBagKeyValuePair( 
											const CComPtr<IWICMetadataQueryReader>& inMetaReader,
											PROPVARIANT& ioValue,
											const VString& inWICTagItem,
											VValueBag *ioBagMeta, const XBOX::VValueBag::StKey& inBagKey,
											eTagType inTagType = TT_AUTO,
											const VSize inMaxSizeBagOrSeq = 8);

	/** convert from metadata bag key-value pair to WIC metadata key-value pair
	@remarks
		for best compatibility, PROPVARIANT type of tag while writing is determined
		using IPTC IIMv4/TIFF 6.0/EXIF 2.2/XMP specifications
	@see
		_WICKeyValuePairToBagKeyValuePair
	*/
	static bool _WICKeyValuePairFromBagKeyValuePair(	
												CComPtr<IWICMetadataQueryWriter>& ioMetaWriter,
												PROPVARIANT& ioValue,
												const VString& inWICTagItem,
												const VValueBag *inBagMeta, const XBOX::VValueBag::StKey& inBagKey,
												eTagType inTagType = TT_LPWSTR,
												CComPtr<IWICImagingFactory> ioFactory = NULL);

	/** convert from VValueSingle value to WIC metadata key-value pair
	@see
		_WICKeyValuePairToBagKeyValuePair
	*/
	static bool _WICKeyValuePairFromVValueSingle(	
												CComPtr<IWICMetadataQueryWriter>& ioMetaWriter,
												PROPVARIANT& ioValue,
												const VString& inWICTagItem,
												const VValueSingle *inValueSingle,
												eTagType inTagType = TT_LPWSTR,
												CComPtr<IWICImagingFactory> ioFactory = NULL);


private:
	VPictureDataProvider_Stream		*fStream4D;
	CComPtr<IWICImagingFactory>		fFactoryWIC;
	CComPtr<IWICBitmapDecoder>		fDecoderWIC;
	CComPtr<IWICBitmapFrameDecode>	fDecoderFrameWIC;
	CComPtr<IWICBitmapSource>		fBitmapSourceWIC;

	/** image source container format */
	GUID fImageContainerGUID;
};





/** initialize image source from the specified data provider 
@remarks
	if inGUIDFormatContainer is GUID_NULL,
		try to find a image decoder which can parse the input data source
	else
		it is assumed that data source validation has been done yet
		so we can use directly decoder associated with the specified GUID
*/
stImageSource::stImageSource(CComPtr<IWICImagingFactory>& inFactory, VPictureDataProvider* inDataProvider, const GUID& inGUIDFormatContainer)
{
	fFactoryWIC			= inFactory;
	fStream4D			= NULL;
	fImageContainerGUID = inGUIDFormatContainer;

	if (!inDataProvider)
		return;
	if (!inDataProvider->IsValid())
		return;

	//create stream from data provider
	fStream4D = new VPictureDataProvider_Stream(inDataProvider);
	if (fStream4D)
	{
		if (inGUIDFormatContainer == GUID_NULL)
		{
			//create decoder from stream (if failed, source data is not a valid image source)
			if (SUCCEEDED(inFactory->CreateDecoderFromStream(
							static_cast<IStream *>(fStream4D),
							0, // vendor
							WICDecodeMetadataCacheOnDemand,
							&fDecoderWIC)))
			{
				//create frame decoder 
				UINT numFrame = 0;
				fDecoderWIC->GetFrameCount(&numFrame);
				if (numFrame >= 1)
				{
					if (SUCCEEDED(fDecoderWIC->GetFrame(0, &fDecoderFrameWIC)))
					{
						HRESULT hr = fDecoderFrameWIC->QueryInterface( __uuidof(IWICBitmapSource), (void **)&fBitmapSourceWIC);
						xbox_assert(SUCCEEDED(hr));
						if (SUCCEEDED(fDecoderWIC->GetContainerFormat(&fImageContainerGUID)))
							return;
					}
				}
			}
		}
		else
		{
			//create decoder from GUID
			if (SUCCEEDED(inFactory->CreateDecoder(
							inGUIDFormatContainer,
							0, // vendor
							&fDecoderWIC)))
			{
				//initialize decoder with stream
				if (SUCCEEDED(fDecoderWIC->Initialize(fStream4D, WICDecodeMetadataCacheOnDemand)))
				{
					//create frame decoder 
					UINT numFrame = 0;
					fDecoderWIC->GetFrameCount(&numFrame);
					if (numFrame >= 1)
					{
						if (SUCCEEDED(fDecoderWIC->GetFrame(0, &fDecoderFrameWIC)))
						{
							HRESULT hr = fDecoderFrameWIC->QueryInterface( __uuidof(IWICBitmapSource), (void **)&fBitmapSourceWIC);
							xbox_assert(SUCCEEDED(hr));
							return;
						}
					}
				}
			}
		}

		fBitmapSourceWIC = (IWICBitmapSource *)NULL;
		fDecoderFrameWIC = (IWICBitmapFrameDecode *)NULL;
		fDecoderWIC = (IWICBitmapDecoder *)NULL;

		fStream4D->Release();
		fStream4D = NULL;
	}
}

stImageSource::stImageSource(CComPtr<IWICImagingFactory>& inFactory, const CComPtr<IWICBitmapSource>& inBitmapSource)
{
	fFactoryWIC			= inFactory;
	fStream4D			= NULL;
	fImageContainerGUID = GUID_NULL;
	fBitmapSourceWIC	= inBitmapSource;
}


stImageSource::~stImageSource()
{
	fBitmapSourceWIC = (IWICBitmapSource *)NULL;
	fDecoderFrameWIC = (IWICBitmapFrameDecode *)NULL;
	fDecoderWIC = (IWICBitmapDecoder *)NULL;

	if (fStream4D)
		fStream4D->Release();
}



/** true if image data provider is valid 
 @remarks
	false if source data provider is not a valid image
 */
bool stImageSource::IsValid() const
{
	return fBitmapSourceWIC != NULL;
}

/** convert bitmap source to the desired pixel format 
@remarks
	this remains abstract until image source is effectively used
*/
void stImageSource::ConvertToPixelFormat( const Gdiplus::PixelFormat& inPixelFormat)
{
	WICPixelFormatGUID pf = VPictureCodec_WIC::FromGdiplusPixelFormat( inPixelFormat);
	ConvertToPixelFormat( pf);
}


/** convert bitmap source to the desired pixel format 
@remarks
	this remains abstract until image source is effectively used
*/
void stImageSource::ConvertToPixelFormat( const WICPixelFormatGUID& inPixelFormat)
{
	if (!IsValid())
		return;

	if (inPixelFormat == GUID_WICPixelFormatUndefined)
		return;

	CComPtr<IWICFormatConverter> converter;
	if (!SUCCEEDED(fFactoryWIC->CreateFormatConverter(&converter)))
		return;
	if (!SUCCEEDED(converter->Initialize(	fBitmapSourceWIC, 
											inPixelFormat,  
											WICBitmapDitherTypeNone,
											NULL,
											0.0f,
											WICBitmapPaletteTypeMedianCut)))
		return;
	fBitmapSourceWIC = NULL;
	HRESULT hr = converter->QueryInterface(__uuidof(IWICBitmapSource), (void **)&fBitmapSourceWIC);
	xbox_assert(SUCCEEDED(hr));
}

/** convert colorspace to sRGB colorspace 
@remarks
	must be used before using CopyPixels with gdiplus pixel buffer to ensure proper color conversion
*/
void stImageSource::ConvertToRGBColorSpace()
{
	if (!IsValid())
		return;

	if (!fDecoderFrameWIC)
		return;

	//get source color context
	IWICColorContext *srcColorContexts[8];
	for (int i = 0; i < 8; i++)
		srcColorContexts[i] = 0;
	CComPtr<IWICColorContext> srcColorContext;
	UINT numColorContext = 0;
	HRESULT hr = fDecoderFrameWIC->GetColorContexts(0, &(srcColorContexts[0]), &numColorContext);
	if (FAILED(hr) || numColorContext <= 0)
		return;
	for (int i = 0; i < numColorContext; i++)
	{
		hr = fFactoryWIC->CreateColorContext(&(srcColorContexts[i]));
		if (FAILED(hr))
			return;
	}
	srcColorContext = NULL;
	UINT numColorContext2 = numColorContext;
	numColorContext = 0;
	hr = fDecoderFrameWIC->GetColorContexts(numColorContext2, &(srcColorContexts[0]), &numColorContext);
	if (FAILED(hr) || numColorContext <= 0)
		return;
	srcColorContext = srcColorContexts[0]; //retain only the first one
	for (int i = 0; i < numColorContext; i++)
		srcColorContexts[i]->Release();
	WICColorContextType type = WICColorContextUninitialized;
	srcColorContext->GetType(&type);
	if (type == WICColorContextExifColorSpace)
	{
		UINT exifColorSpace = 0;
		srcColorContext->GetExifColorSpace(&exifColorSpace);
		if (exifColorSpace == 1)
			return;
	}

	//set dest color context (sRGB)
	CComPtr<IWICColorContext> dstColorContext;
	if (!SUCCEEDED(fFactoryWIC->CreateColorContext(&dstColorContext)))
		return;
	dstColorContext->InitializeFromExifColorSpace( 1);

	//create color transformer
	CComPtr<IWICColorTransform> converter;
	if (!SUCCEEDED(fFactoryWIC->CreateColorTransformer(&converter)))
		return;

	//initialize colorspace transform (it remains abstract until CopyPixels is called)
	WICPixelFormatGUID pf;
	fBitmapSourceWIC->GetPixelFormat(&pf);
	if (!SUCCEEDED(converter->Initialize(	fBitmapSourceWIC, 
											srcColorContext,  
											dstColorContext,
											pf)))
		return;
	
	fBitmapSourceWIC = NULL;
	hr = converter->QueryInterface(__uuidof(IWICBitmapSource), (void **)&fBitmapSourceWIC);
	xbox_assert(SUCCEEDED(hr));
}


/** get image width */
uLONG stImageSource::GetWidth() const
{
	if (!IsValid())
		return 0;

    UINT width = 0;
    UINT height = 0;
    if (SUCCEEDED(fBitmapSourceWIC->GetSize(&width, &height)))
	{
		return ((uLONG)width);
	}
	return 0;
}

/** get image height */
uLONG stImageSource::GetHeight() const
{
	if (!IsValid())
		return 0;

    UINT width = 0;
    UINT height = 0;
    if (SUCCEEDED(fBitmapSourceWIC->GetSize(&width, &height)))
	{
		return ((uLONG)height);
	}
	return 0;
}

/** get image resolution */
void stImageSource::GetResolution( Real& outDpiX, Real& outDpiY)
{
	if (!IsValid())
	{
		outDpiX = outDpiY = 96.0;
		return;
	}

    fBitmapSourceWIC->GetResolution(&outDpiX, &outDpiY);
}

/** get WIC pixel format */
void stImageSource::GetPixelFormat( WICPixelFormatGUID& outPF) const
{
	if (!IsValid())
		outPF = GUID_WICPixelFormatUndefined;

    if (!SUCCEEDED(fBitmapSourceWIC->GetPixelFormat(&outPF)))
		outPF = GUID_WICPixelFormatUndefined;
}


/** get Gdiplus pixel format 
@remarks
	return PixelFormatUndefined if image source format is not supported by Gdiplus
*/
Gdiplus::PixelFormat stImageSource::GetPixelFormat() const
{
	WICPixelFormatGUID pfWIC;
	GetPixelFormat(pfWIC);
	
	Gdiplus::PixelFormat pf = VPictureCodec_WIC::FromWICPixelFormatGUID( pfWIC);
	return pf;
}

/** get row length in bytes */
uLONG stImageSource::GetStride() const
{
	return GetStride( GetWidth(), GetBitDepth());
}


/** get number of bits per pixel */
uLONG stImageSource::GetBitDepth() const
{
	WICPixelFormatGUID pfWIC;
	GetPixelFormat(pfWIC);

	return stImageSource::GetPixelFormatBitDepth( pfWIC);
}

/** true if input pixel format has alpha channel */
bool stImageSource::HasPixelFormatAlpha( const WICPixelFormatGUID& inPF)
{
	if (inPF == GUID_WICPixelFormat32bppBGRA) 
		return true;
	if (inPF == GUID_WICPixelFormat32bppPBGRA) 
		return true;
	if (inPF == GUID_WICPixelFormat64bppRGBA) 
		return true;
	if (inPF == GUID_WICPixelFormat64bppPRGBA) 
		return true;
	if (inPF == GUID_WICPixelFormat128bppRGBAFloat) 
		return true;
	if (inPF == GUID_WICPixelFormat128bppPRGBAFloat) 
		return true;
	if (inPF == GUID_WICPixelFormat64bppRGBAFixedPoint) 
		return true;
	if (inPF == GUID_WICPixelFormat128bppRGBAFixedPoint) 
		return true;
	if (inPF == GUID_WICPixelFormat64bppRGBAHalf) 
		return true;
	if (inPF == GUID_WICPixelFormat40bppCMYKAlpha) 
		return true;
	if (inPF == GUID_WICPixelFormat80bppCMYKAlpha) 
		return true;
	if (inPF == GUID_WICPixelFormat32bpp3ChannelsAlpha) 
		return true;
	if (inPF == GUID_WICPixelFormat40bpp4ChannelsAlpha) 
		return true;
	if (inPF == GUID_WICPixelFormat48bpp5ChannelsAlpha) 
		return true;
	if (inPF == GUID_WICPixelFormat56bpp6ChannelsAlpha) 
		return true;
	if (inPF == GUID_WICPixelFormat64bpp7ChannelsAlpha) 
		return true;
	if (inPF == GUID_WICPixelFormat72bpp8ChannelsAlpha) 
		return true;
	if (inPF == GUID_WICPixelFormat64bpp3ChannelsAlpha) 
		return true;
	if (inPF == GUID_WICPixelFormat80bpp4ChannelsAlpha) 
		return true;
	if (inPF == GUID_WICPixelFormat96bpp5ChannelsAlpha) 
		return true;
	if (inPF == GUID_WICPixelFormat112bpp6ChannelsAlpha) 
		return true;
	if (inPF == GUID_WICPixelFormat128bpp7ChannelsAlpha) 
		return true;
	if (inPF == GUID_WICPixelFormat144bpp8ChannelsAlpha) 
		return true;
	return false;
}

/** true if input pixel format is indexed */
bool stImageSource::IsPixelFormatIndexed( const WICPixelFormatGUID& inPF)
{
	if (inPF == GUID_WICPixelFormat1bppIndexed) 
		return true;
	if (inPF == GUID_WICPixelFormat2bppIndexed) 
		return true;
	if (inPF == GUID_WICPixelFormat4bppIndexed) 
		return true;
	if (inPF == GUID_WICPixelFormat8bppIndexed) 
		return true;
	return false;
}


/** return count of bits per pixel for input pixel format */
uLONG stImageSource::GetPixelFormatBitDepth( const WICPixelFormatGUID& inPF)
{
	if (inPF == GUID_WICPixelFormat1bppIndexed) 
		return 1;
	if (inPF == GUID_WICPixelFormat2bppIndexed) 
		return 2;
	if (inPF == GUID_WICPixelFormat4bppIndexed) 
		return 4;
	if (inPF == GUID_WICPixelFormat8bppIndexed) 
		return 8;
	if (inPF == GUID_WICPixelFormatBlackWhite) 
		return 1;
	if (inPF == GUID_WICPixelFormat2bppGray) 
		return 2;
	if (inPF == GUID_WICPixelFormat4bppGray) 
		return 4;
	if (inPF == GUID_WICPixelFormat8bppGray) 
		return 8;
	if (inPF == GUID_WICPixelFormat16bppBGR555) 
		return 16;
	if (inPF == GUID_WICPixelFormat16bppBGR565) 
		return 16;
	if (inPF == GUID_WICPixelFormat16bppGray) 
		return 16;
	if (inPF == GUID_WICPixelFormat24bppBGR) 
		return 24;
	if (inPF == GUID_WICPixelFormat24bppRGB) 
		return 24;
	if (inPF == GUID_WICPixelFormat32bppBGR) 
		return 32;
	if (inPF == GUID_WICPixelFormat32bppBGRA) 
		return 32;
	if (inPF == GUID_WICPixelFormat32bppPBGRA) 
		return 32;
	if (inPF == GUID_WICPixelFormat32bppGrayFloat) 
		return 32;
	if (inPF == GUID_WICPixelFormat48bppRGBFixedPoint) 
		return 48;
	if (inPF == GUID_WICPixelFormat16bppGrayFixedPoint) 
		return 16;
	if (inPF == GUID_WICPixelFormat32bppBGR101010) 
		return 32;
	if (inPF == GUID_WICPixelFormat48bppRGB) 
		return 48;
	if (inPF == GUID_WICPixelFormat64bppRGBA) 
		return 64;
	if (inPF == GUID_WICPixelFormat64bppPRGBA) 
		return 64;
	if (inPF == GUID_WICPixelFormat96bppRGBFixedPoint) 
		return 96;
	if (inPF == GUID_WICPixelFormat128bppRGBAFloat) 
		return 128;
	if (inPF == GUID_WICPixelFormat128bppPRGBAFloat) 
		return 128;
	if (inPF == GUID_WICPixelFormat128bppRGBFloat) 
		return 128;
	if (inPF == GUID_WICPixelFormat32bppCMYK) 
		return 32;
	if (inPF == GUID_WICPixelFormat64bppRGBAFixedPoint) 
		return 64;
	if (inPF == GUID_WICPixelFormat64bppRGBFixedPoint) 
		return 64;
	if (inPF == GUID_WICPixelFormat128bppRGBAFixedPoint) 
		return 128;
	if (inPF == GUID_WICPixelFormat128bppRGBFixedPoint) 
		return 128;
	if (inPF == GUID_WICPixelFormat64bppRGBAHalf) 
		return 64;
	if (inPF == GUID_WICPixelFormat64bppRGBHalf) 
		return 64;
	if (inPF == GUID_WICPixelFormat48bppRGBHalf) 
		return 48;
	if (inPF == GUID_WICPixelFormat32bppRGBE) 
		return 32;
	if (inPF == GUID_WICPixelFormat16bppGrayHalf) 
		return 16;
	if (inPF == GUID_WICPixelFormat32bppGrayFixedPoint) 
		return 32;
	if (inPF == GUID_WICPixelFormat64bppCMYK) 
		return 64;
	if (inPF == GUID_WICPixelFormat24bpp3Channels) 
		return 24;
	if (inPF == GUID_WICPixelFormat32bpp4Channels) 
		return 32;
	if (inPF == GUID_WICPixelFormat40bpp5Channels) 
		return 40;
	if (inPF == GUID_WICPixelFormat48bpp6Channels) 
		return 48;
	if (inPF == GUID_WICPixelFormat56bpp7Channels) 
		return 56;
	if (inPF == GUID_WICPixelFormat64bpp8Channels) 
		return 64;
	if (inPF == GUID_WICPixelFormat48bpp3Channels) 
		return 48;
	if (inPF == GUID_WICPixelFormat64bpp4Channels) 
		return 64;
	if (inPF == GUID_WICPixelFormat80bpp5Channels) 
		return 80;
	if (inPF == GUID_WICPixelFormat96bpp6Channels) 
		return 96;
	if (inPF == GUID_WICPixelFormat112bpp7Channels) 
		return 112;
	if (inPF == GUID_WICPixelFormat128bpp8Channels) 
		return 128;
	if (inPF == GUID_WICPixelFormat40bppCMYKAlpha) 
		return 40;
	if (inPF == GUID_WICPixelFormat80bppCMYKAlpha) 
		return 80;
	if (inPF == GUID_WICPixelFormat32bpp3ChannelsAlpha) 
		return 32;
	if (inPF == GUID_WICPixelFormat40bpp4ChannelsAlpha) 
		return 40;
	if (inPF == GUID_WICPixelFormat48bpp5ChannelsAlpha) 
		return 48;
	if (inPF == GUID_WICPixelFormat56bpp6ChannelsAlpha) 
		return 56;
	if (inPF == GUID_WICPixelFormat64bpp7ChannelsAlpha) 
		return 64;
	if (inPF == GUID_WICPixelFormat72bpp8ChannelsAlpha) 
		return 72;
	if (inPF == GUID_WICPixelFormat64bpp3ChannelsAlpha) 
		return 64;
	if (inPF == GUID_WICPixelFormat80bpp4ChannelsAlpha) 
		return 80;
	if (inPF == GUID_WICPixelFormat96bpp5ChannelsAlpha) 
		return 96;
	if (inPF == GUID_WICPixelFormat112bpp6ChannelsAlpha) 
		return 112;
	if (inPF == GUID_WICPixelFormat128bpp7ChannelsAlpha) 
		return 128;
	if (inPF == GUID_WICPixelFormat144bpp8ChannelsAlpha) 
		return 144;
	return 0;
}


/** return row length in bytes using row width in pixels and number of bits per pixel */
uLONG stImageSource::GetStride( uLONG inWidth, uLONG inBPP)
{
	uLONG stride = (inWidth * inBPP + 7) / 8;
	return stride;
}

/** true if image has alpha channel */
bool stImageSource::hasAlpha() const
{
	if (!IsValid())
		return false;

	bool hasAlpha = false;
	GUID pf;
    if (SUCCEEDED(fBitmapSourceWIC->GetPixelFormat(&pf)))
		hasAlpha = HasPixelFormatAlpha(pf);
	if (!hasAlpha)
	{
		if (hasPalette())
		{
			//get palette alpha status
			CComPtr<IWICPalette> palette;
			if (SUCCEEDED(fFactoryWIC->CreatePalette( &palette)))
				if (SUCCEEDED(fBitmapSourceWIC->CopyPalette( palette)))
				{
					BOOL alpha = 0;
					palette->HasAlpha(&alpha);
					return alpha != 0;
				}
		}
	}
	return hasAlpha;
}


/** true if image has palette */
bool stImageSource::hasPalette() const
{
	if (!IsValid())
		return false;
	
	WICPixelFormatGUID pf;
	GetPixelFormat(pf);
	return IsPixelFormatIndexed(pf);
}

/** copy image source palette to gdiplus bitmap palette 
@remarks
	destination bitmap pixel format must be indexed
*/
bool stImageSource::CopyPalette( Gdiplus::Bitmap *inBmp)
{
	xbox_assert(inBmp);
	if (!IsValid())
		return false;

	CComPtr<IWICPalette> palette;
	if (SUCCEEDED(fFactoryWIC->CreatePalette( &palette)))
		if (SUCCEEDED(fBitmapSourceWIC->CopyPalette( palette)))
		{
			UINT numColor = 0;
			palette->GetColorCount( &numColor);

			if (numColor >= 1)
			{
				xbox_assert(sizeof(WICColor) == sizeof(Gdiplus::ARGB));

				char *paletteBuffer = new char [sizeof(Gdiplus::ColorPalette)+(numColor-1)*sizeof(Gdiplus::ARGB)];
				Gdiplus::ColorPalette *paletteDest = (Gdiplus::ColorPalette *)paletteBuffer;

				paletteDest->Count = numColor;
				paletteDest->Flags = 0;
				
				//copy alpha status
				BOOL hasAlpha = 0;
				palette->HasAlpha(&hasAlpha);
				if (hasAlpha)
					paletteDest->Flags |= Gdiplus::PaletteFlagsHasAlpha;
				
				//copy grayscale status
				BOOL isGrayscale = 0;
				palette->IsGrayscale( &isGrayscale);
				if (isGrayscale)
					paletteDest->Flags |= Gdiplus::PaletteFlagsGrayScale;

				//copy colors to gdiplus palette
				UINT numColorRead = 0;
				if (SUCCEEDED(palette->GetColors( numColor, (WICColor *)&(paletteDest->Entries[0]), &numColorRead)))
				{
					xbox_assert(numColorRead <= numColor);
					if (Gdiplus::Ok == inBmp->SetPalette(paletteDest))
					{
						delete [] paletteBuffer;
						return true;
					}
				}
				delete [] paletteBuffer;
			}
		}
	return false;
}


/** decode and copy image source to destination buffer */
bool stImageSource::CopyPixels( void *inBuffer, uLONG inBufSize, uLONG inBufStride)
{
	if (!IsValid())
		return false;

	if (inBufStride == 0)
		inBufStride = GetStride();

	return (SUCCEEDED(fBitmapSourceWIC->CopyPixels( NULL, (UINT)inBufStride, (UINT)inBufSize, (BYTE *)inBuffer)));
}



/** direct copy image source metadatas to metadatas query writer 
@remarks
	this is possible only if source image GUID == dest image GUID
*/
void stImageSource::CopyMetadatas( CComPtr<IWICMetadataQueryWriter>& inMetaWriter)
{
	if (!IsValid()) 
		return;

	if (fDecoderFrameWIC == NULL)
		return;

	CComPtr<IWICMetadataQueryReader> metaReader;
	if (!SUCCEEDED(fDecoderFrameWIC->GetMetadataQueryReader(
				   &metaReader)))
		return;

	PROPVARIANT value;
    PropVariantInit(&value);

    value.vt		=	VT_UNKNOWN;
    value.punkVal	=	(IWICMetadataQueryReader *)metaReader;
    value.punkVal->AddRef();
    inMetaWriter->SetMetadataByName( L"/", &value);

    PropVariantClear(&value);
}


/** read metadatas */
void stImageSource::ReadMetadatas( VValueBag *outMetadatas)
{
	if (!IsValid()) 
		return;

	if (fDecoderFrameWIC == NULL)
		return;

#if TEST_METADATAS
	//create manually default metas bag
	if (::GetAsyncKeyState(VK_LSHIFT) & 0x8000)
	{
		ImageMeta::stWriter writer( outMetadatas);

		//set default IPTC tags
		writer.SetIPTCActionAdvised();

		VectorOfVString bylines;
		bylines.push_back( "byline1");
		bylines.push_back( "byline2");
		writer.SetIPTCByline( bylines);

		VectorOfVString bylinetitles;
		bylinetitles.push_back( "bylinetitle1");
		bylinetitles.push_back( "bylinetitle2");
		writer.SetIPTCBylineTitle( bylinetitles);

		writer.SetIPTCCaptionAbstract( "caption abstract");

		writer.SetIPTCCategory( "category");

		writer.SetIPTCCity( "city");

		VectorOfVString contacts;
		contacts.push_back( "contact1");
		contacts.push_back( "contact2");
		writer.SetIPTCContact( contacts);

		VectorOfVString countrycodes;
		countrycodes.push_back( "FRA");
		countrycodes.push_back( "USA");
		writer.SetIPTCContentLocationCode( countrycodes);

		VectorOfVString countrynames;
		countrynames.push_back( "France");
		countrynames.push_back( "Etats-Unis");
		writer.SetIPTCContentLocationName( countrynames);

		writer.SetIPTCCopyrightNotice( "copyright notice");

		writer.SetIPTCCountryPrimaryLocationCode( "FRA");

		writer.SetIPTCCountryPrimaryLocationName( "France");

		writer.SetIPTCCredit( "credit");

		VTime time;
		time.FromSystemTime();
		writer.SetIPTCDateTimeCreated( time);

		writer.SetIPTCDigitalCreationDateTime( time);

		writer.SetIPTCEditorialUpdate("editorial update");

		writer.SetIPTCEditStatus( "edit status");

		writer.SetIPTCExpirationDateTime( time);

		writer.SetIPTCFixtureIdentifier( "fixture identifier");

		writer.SetIPTCHeadline( "headline");

		writer.SetIPTCImageOrientation();

		writer.SetIPTCImageType();

		VectorOfVString keywords;
		keywords.push_back( "keyword1");
		keywords.push_back( "keyword2");
		writer.SetIPTCKeywords( keywords);

		writer.SetIPTCLanguageIdentifier( "fr");

		writer.SetIPTCObjectAttributeReference( "wonder");

		writer.SetIPTCObjectCycle();

		writer.SetIPTCObjectName( "object name");

		writer.SetIPTCObjectTypeReference( "object type ref");

		writer.SetIPTCOriginalTransmissionReference( "original transmission ref");

		writer.SetIPTCOriginatingProgram( "originating program");

		writer.SetIPTCProgramVersion( "program version");

		writer.SetIPTCProvinceState( "province state");

		/*
		std::vector<uLONG> refnumbers;
		refnumbers.push_back( 1);
		refnumbers.push_back( 2);
		writer.SetIPTCReferenceNumber( refnumbers);

		std::vector<VTime> times;
		times.push_back(time);
		VTime time2 = time;
		time2.AddDays(1);
		times.push_back( time2);
		writer.SetIPTCReferenceDateTime( times);

		VectorOfVString refservices;
		refservices.push_back( "refservice1");
		refservices.push_back( "refservice2");
		writer.SetIPTCReferenceService( refservices);
		*/

		writer.SetIPTCReleaseDateTime( time);

		std::vector<ImageMetaIPTC::eScene> scenes;
		scenes.push_back( ImageMetaIPTC::kScene_ExteriorView);
		scenes.push_back( ImageMetaIPTC::kScene_PanoramicView);
		writer.SetIPTCScene( scenes);

		writer.SetIPTCSource( "source");

		writer.SetIPTCSpecialInstructions( "special instructions");

		writer.SetIPTCStarRating( 4);

		VectorOfVString subjectrefs;
		subjectrefs.push_back( "01013000");
		writer.SetIPTCSubjectReference( subjectrefs);

		writer.SetIPTCSubLocation( "sub-location");

		VectorOfVString supcats;
		supcats.push_back( "supcat1");
		supcats.push_back( "supcat2");
		writer.SetIPTCSupplementalCategory( supcats);

		writer.SetIPTCUrgency( 5);

		VectorOfVString writereditors;
		writereditors.push_back( "writereditor1");
		writereditors.push_back( "writereditor2");
		writer.SetIPTCWriterEditor( writereditors);

		//set default TIFF tags
		writer.SetTIFFArtist( "artist");
		writer.SetTIFFCopyright( "copyright");
		writer.SetTIFFDateTime( time);
		writer.SetTIFFDocumentName( "document name");
		writer.SetTIFFHostComputer( "host computer");
		writer.SetTIFFImageDescription( "image description");
		writer.SetTIFFMake( "make");
		writer.SetTIFFModel( "model");
		writer.SetTIFFOrientation();
		writer.SetTIFFPhotometricInterpretation();

		//writer.SetTIFFPrimaryChromaticities();

		writer.SetTIFFSoftware( "software");

		//writer.SetTIFFWhitePoint();

		writer.SetTIFFXResolution( 92.0);
		writer.SetTIFFYResolution( 92.0);
		writer.SetTIFFResolutionUnit( ImageMetaTIFF::kResolutionUnit_Inches);

		//std::vector<uLONG> transferFunc;
		//for (int i = 0; i < 3*256; i++)
		//	transferFunc.push_back(1.0);
		//writer.SetTIFFTransferFunction( transferFunc);

		//set default EXIF tags
		writer.SetEXIFApertureValue( 0.7);
		writer.SetEXIFBrightnessValue( 1.0);
		
		writer.SetEXIFColorSpace();

		writer.SetEXIFComponentsConfiguration();

		writer.SetEXIFCompressedBitsPerPixel( 1.6);
		writer.SetEXIFContrast( ImageMetaEXIF::kPower_Normal);
		writer.SetEXIFCustomRendered( true);
		writer.SetEXIFDateTimeDigitized( time);
		writer.SetEXIFDateTimeOriginal( time);

		writer.SetEXIFDigitalZoomRatio( 1.0);
		writer.SetEXIFExposureBiasValue( 0.0);
		writer.SetEXIFExposureIndex( 0.0);
		writer.SetEXIFExposureMode();
		writer.SetEXIFExposureProgram();
		writer.SetEXIFExposureTime( 0.5);
		writer.SetEXIFFileSource();
		writer.SetEXIFFlash( true);
		writer.SetEXIFFlashEnergy( 1.0);
		writer.SetEXIFFlashPixVersion();
		writer.SetEXIFFNumber( 7.1);
		writer.SetEXIFFocalLength( 5.8);
		writer.SetEXIFFocalLenIn35mmFilm( 35);
		writer.SetEXIFFocalPlaneResolutionUnit();
		writer.SetEXIFFocalPlaneXResolution( 72.0);
		writer.SetEXIFFocalPlaneYResolution( 72.0);
		writer.SetEXIFGainControl();
		writer.SetEXIFGamma( 2.2);
		writer.SetEXIFImageUniqueID( "ImageUniqueID");
		std::vector<uLONG> isoSpeedRatings;
		isoSpeedRatings.push_back(50);
		writer.SetEXIFISOSpeedRatings( isoSpeedRatings);
		writer.SetEXIFLightSource();
		writer.SetEXIFMakerNote( "maker note");
		writer.SetEXIFMaxApertureValue( 1.0);
		writer.SetEXIFMeteringMode();
		writer.SetEXIFPixelXDimension( 1024);
		writer.SetEXIFPixelYDimension( 768);
		writer.SetEXIFRelatedSoundFile( "related sound file");
		writer.SetEXIFSaturation( ImageMetaEXIF::kPower_Low);
		writer.SetEXIFSceneCaptureType();
		writer.SetEXIFSceneType( true);
		writer.SetEXIFSensingMethod();
		writer.SetEXIFSharpness( ImageMetaEXIF::kPower_High);
		writer.SetEXIFShutterSpeedValue( 5.0);
		writer.SetEXIFSpectralSensitivity( "SpectralSensitivity");
		writer.SetEXIFSubjectAreaRectangle( 100, 50, 25, 15);
		writer.SetEXIFSubjectDistance( 100.0);
		writer.SetEXIFSubjectDistRange();
		writer.SetEXIFSubjectLocation( 100, 50);
		writer.SetEXIFUserComment( "UserComment");
		writer.SetEXIFVersion();
		writer.SetEXIFWhiteBalance( true);

		//set default GPS tags
		writer.SetGPSAltitude( 100.0);
		writer.SetGPSAreaInformation( "AreaInformation"); 
		writer.SetGPSDateTime( time);
		writer.SetGPSDestBearing( 75.0);
		writer.SetGPSDestDistance( 2.5);
		writer.SetGPSDestLatitude( 25, 54, 20, true);
		writer.SetGPSDestLongitude( 100, 32, 50, false);
		writer.SetGPSDifferential( true);
		writer.SetGPSDOP( 1.0);
		writer.SetGPSImgDirection( 180.0);
		writer.SetGPSLatitude( 32, 54, 20, false);
		writer.SetGPSLongitude( 120, 10, 43.32, true);
		writer.SetGPSMapDatum( "MapDatum");
		writer.SetGPSMeasureMode();
		writer.SetGPSProcessingMethod( "ProcessingMethod");
		writer.SetGPSSatellites( "satellites");
		writer.SetGPSSpeed( 30);
		writer.SetGPSStatus();
		writer.SetGPSTrack( 100.0);
		writer.SetGPSVersionID();
		return;
	}
#endif

	PROPVARIANT value;
	PropVariantInit(&value);

	CComPtr<IWICMetadataQueryReader> metaReader;
	if (SUCCEEDED(fDecoderFrameWIC->GetMetadataQueryReader( &metaReader)))
	{
		//search first for XMP metadatas if any
		bool hasXMP = false;
		CComPtr<IWICMetadataQueryReader> metaReaderXMP;
		{
			VString pathXMP = "/xmp";
			bool ok = SUCCEEDED(metaReader->GetMetadataByName(pathXMP.GetCPointer(), &value));
			if (!ok)
			{
				//try alternative path
				pathXMP = "/ifd/xmp";
				ok = SUCCEEDED(metaReader->GetMetadataByName(pathXMP.GetCPointer(), &value));
			}
			if (ok)
			{
				if (value.vt == VT_UNKNOWN)
					ok = SUCCEEDED(value.punkVal->QueryInterface(IID_IWICMetadataQueryReader, (void **)&metaReaderXMP));
				if (ok)
					hasXMP = true;
				PropVariantClear(&value); 
			}
		}

		{
			//read IPTC metadatas if any
			CComPtr<IWICMetadataQueryReader> metaReaderIPTC;

			bool ok = false;
			//look first for IPTC IIMv4 metadatas
			VString pathIPTC = "/iptc";
#ifndef DEBUG_XMP 
			ok = SUCCEEDED(metaReader->GetMetadataByName(pathIPTC.GetCPointer(), &value));
			if (!ok)
			{
				//try alternative path (TIFF-compliant path)
				pathIPTC = "/ifd/iptc";
				ok = SUCCEEDED(metaReader->GetMetadataByName(pathIPTC.GetCPointer(), &value));
			}
			if (!ok)
			{
				//try alternative path (JPEG-compliant path)
				pathIPTC = "/app1/ifd/iptc";
				ok = SUCCEEDED(metaReader->GetMetadataByName(pathIPTC.GetCPointer(), &value));
			}
			if (!ok)
			{
				//try alternative path (JPEG-compliant path)
				pathIPTC = "/app13/irb/8bimiptc/iptc";
				ok = SUCCEEDED(metaReader->GetMetadataByName(pathIPTC.GetCPointer(), &value));
			}
#endif
			if (ok || hasXMP)
			{
				if (ok)
				{
					ok = false;
					if (value.vt == VT_UNKNOWN)
						ok = SUCCEEDED(value.punkVal->QueryInterface(IID_IWICMetadataQueryReader, (void **)&metaReaderIPTC));
					PropVariantClear(&value); 
				}

				//read metadatas 
				//@remarks
				//	IPTC metadatas tag type is always ASCII string (can be repeatable)

				VValueBag *bagMetaIPTC = new VValueBag();

				if (hasXMP)
				{
					//IPTC XMP tags (IIMv4 tags in comment or not supported in XMP)
					//@remarks
					//	for XMP tags of type Lang Alt - like 'dc:title',
					//  we get actually the '/x-default' metadata child element
					//  (which should be the XMP 'rdf:Alt/rdf:li' element with attribute xml:lang="x-default")

					//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/{ushort=3}", 
					//									bagMetaIPTC, ImageMetaIPTC::ObjectTypeReference);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/iptccore:IntellectualGenre", 
														bagMetaIPTC, ImageMetaIPTC::ObjectAttributeReference);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/dc:title/x-default", 
														bagMetaIPTC, ImageMetaIPTC::ObjectName);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/mediapro:Status", 
														bagMetaIPTC, ImageMetaIPTC::EditStatus);
					//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/{ushort=8}", 
					//									bagMetaIPTC, ImageMetaIPTC::EditorialUpdate);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/photoshop:Urgency", 
														bagMetaIPTC, ImageMetaIPTC::Urgency);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/iptccore:SubjectCode", 
														bagMetaIPTC, ImageMetaIPTC::SubjectReference,
														TT_XMP_BAG);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/photoshop:Category", 
														bagMetaIPTC, ImageMetaIPTC::Category);
					if (!_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/photoshop:SupplementalCategory", 
															bagMetaIPTC, ImageMetaIPTC::SupplementalCategory,
															TT_XMP_SEQ))
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/photoshop:SupplementalCategories", 
																					bagMetaIPTC, ImageMetaIPTC::SupplementalCategory,
																					TT_XMP_SEQ);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/mediapro:Event", 
														bagMetaIPTC, ImageMetaIPTC::FixtureIdentifier);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/dc:subject", 
														bagMetaIPTC, ImageMetaIPTC::Keywords,
														TT_XMP_BAG);
					//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/{ushort=26}", 
					//									bagMetaIPTC, ImageMetaIPTC::ContentLocationCode);
					//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/{ushort=27}", 
					//									bagMetaIPTC, ImageMetaIPTC::ContentLocationName);
					//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/{ushort=30}", 
					//									bagMetaIPTC, ImageMetaIPTC::ReleaseDate);
					//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/{ushort=35}", 
					//									bagMetaIPTC, ImageMetaIPTC::ReleaseTime);
					//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/{ushort=37}", 
					//									bagMetaIPTC, ImageMetaIPTC::ExpirationDate);
					//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/{ushort=38}", 
					//									bagMetaIPTC, ImageMetaIPTC::ExpirationTime);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/photoshop:Instructions", 
														bagMetaIPTC, ImageMetaIPTC::SpecialInstructions);
					//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/{ushort=42}", 
					//									bagMetaIPTC, ImageMetaIPTC::ActionAdvised);
					//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/{ushort=45}", 
					//									bagMetaIPTC, ImageMetaIPTC::ReferenceService);
					//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/{ushort=47}", 
					//									bagMetaIPTC, ImageMetaIPTC::ReferenceDate);
					//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/{ushort=50}", 
					//									bagMetaIPTC, ImageMetaIPTC::ReferenceNumber);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/photoshop:DateCreated", 
														bagMetaIPTC, ImageMetaIPTC::DateTimeCreated,
														TT_XMP_DATETIME);
					//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/{ushort=60}", 
					//									bagMetaIPTC, ImageMetaIPTC::TimeCreated);
					//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/{ushort=62}", 
					//									bagMetaIPTC, ImageMetaIPTC::DigitalCreationDate);
					//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/{ushort=63}", 
					//									bagMetaIPTC, ImageMetaIPTC::DigitalCreationTime);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/xmp:creatortool", 
														bagMetaIPTC, ImageMetaIPTC::OriginatingProgram);
					//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/{ushort=70}", 
					//									bagMetaIPTC, ImageMetaIPTC::ProgramVersion);
					//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/{ushort=75}", 
					//									bagMetaIPTC, ImageMetaIPTC::ObjectCycle);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/dc:creator", 
														bagMetaIPTC, ImageMetaIPTC::Byline,
														TT_XMP_SEQ);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/photoshop:AuthorsPosition", 
														bagMetaIPTC, ImageMetaIPTC::BylineTitle);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/photoshop:City", 
														bagMetaIPTC, ImageMetaIPTC::City);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/iptccore:Location", 
														bagMetaIPTC, ImageMetaIPTC::SubLocation);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/photoshop:State", 
														bagMetaIPTC, ImageMetaIPTC::ProvinceState);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/iptccore:CountryCode", 
														bagMetaIPTC, ImageMetaIPTC::CountryPrimaryLocationCode);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/photoshop:Country", 
														bagMetaIPTC, ImageMetaIPTC::CountryPrimaryLocationName);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/photoshop:TransmissionReference", 
														bagMetaIPTC, ImageMetaIPTC::OriginalTransmissionReference);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/iptccore:Scene", 
														bagMetaIPTC, ImageMetaIPTC::Scene,
														TT_XMP_BAG);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/photoshop:Headline", 
														bagMetaIPTC, ImageMetaIPTC::Headline);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/photoshop:Credit", 
														bagMetaIPTC, ImageMetaIPTC::Credit);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/photoshop:Source", 
														bagMetaIPTC, ImageMetaIPTC::Source);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/dc:rights/x-default", 
														bagMetaIPTC, ImageMetaIPTC::CopyrightNotice);
					//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/{ushort=118}", 
					//									bagMetaIPTC, ImageMetaIPTC::Contact);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/dc:description/x-default", 
														bagMetaIPTC, ImageMetaIPTC::CaptionAbstract);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/photoshop:CaptionWriter", 
														bagMetaIPTC, ImageMetaIPTC::WriterEditor);
					//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/{ushort=130}", 
					//									bagMetaIPTC, ImageMetaIPTC::ImageType);
					//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/{ushort=131}", 
					//									bagMetaIPTC, ImageMetaIPTC::ImageOrientation);
					//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/{ushort=135}", 
					//									bagMetaIPTC, ImageMetaIPTC::LanguageIdentifier);
					//if (!_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/MicrosoftPhoto:Rating", 
					//									bagMetaIPTC, ImageMetaIPTC::StarRating))
					_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/xmp:Rating", 
														bagMetaIPTC, ImageMetaIPTC::StarRating);
				}
				if (ok)
				{
					//IPTC IIMv4 tags

#if ENABLE_ALL_METAS
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Object Type Reference}", 
														bagMetaIPTC, ImageMetaIPTC::ObjectTypeReference);
#endif
					//_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Object Attribute Reference}", 
					//									bagMetaIPTC, ImageMetaIPTC::ObjectAttributeReference);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Object Name}", 
														bagMetaIPTC, ImageMetaIPTC::ObjectName);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Edit Status}", 
														bagMetaIPTC, ImageMetaIPTC::EditStatus);
#if ENABLE_ALL_METAS
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Editorial Update}", 
														bagMetaIPTC, ImageMetaIPTC::EditorialUpdate);
#endif
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Urgency}", 
														bagMetaIPTC, ImageMetaIPTC::Urgency);
					//_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Subject Reference}", 
					//									bagMetaIPTC, ImageMetaIPTC::SubjectReference);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Category}", 
														bagMetaIPTC, ImageMetaIPTC::Category);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Supplemental Category}", 
														bagMetaIPTC, ImageMetaIPTC::SupplementalCategory);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Fixture Identifier}", 
														bagMetaIPTC, ImageMetaIPTC::FixtureIdentifier);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Keywords}", 
														bagMetaIPTC, ImageMetaIPTC::Keywords);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Content Location Code}", 
														bagMetaIPTC, ImageMetaIPTC::ContentLocationCode);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Content Location Name}", 
														bagMetaIPTC, ImageMetaIPTC::ContentLocationName);
					bool datetime = false;
					if (!bagMetaIPTC->AttributeExists( ImageMetaIPTC::ReleaseDateTime))
					{
						datetime =		_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Release Date}", 
																			bagMetaIPTC, ImageMetaIPTC::ReleaseDate);
						datetime =		_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Release Time}", 
																			bagMetaIPTC, ImageMetaIPTC::ReleaseTime) || datetime;
						if (datetime)
						{
							//convert to XML datetime

							VString dateIPTC, timeIPTC, datetimeXML;
							bagMetaIPTC->GetString( ImageMetaIPTC::ReleaseDate, dateIPTC);
							bagMetaIPTC->GetString( ImageMetaIPTC::ReleaseTime, timeIPTC);
							DateTimeFromIPTCToXML( dateIPTC, timeIPTC, datetimeXML);

							bagMetaIPTC->SetString( ImageMetaIPTC::ReleaseDateTime, datetimeXML);
							bagMetaIPTC->RemoveAttribute( ImageMetaIPTC::ReleaseDate);
							bagMetaIPTC->RemoveAttribute( ImageMetaIPTC::ReleaseTime);
						}
					}
					
					if (!bagMetaIPTC->AttributeExists( ImageMetaIPTC::ExpirationDateTime))
					{
						datetime = _WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Expiration Date}", 
																		bagMetaIPTC, ImageMetaIPTC::ExpirationDate);
						datetime = _WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Expiration Time}", 
																		bagMetaIPTC, ImageMetaIPTC::ExpirationTime) || datetime;
						if (datetime)
						{
							//convert to XML datetime

							VString dateIPTC, timeIPTC, datetimeXML;
							bagMetaIPTC->GetString( ImageMetaIPTC::ExpirationDate, dateIPTC);
							bagMetaIPTC->GetString( ImageMetaIPTC::ExpirationTime, timeIPTC);
							DateTimeFromIPTCToXML( dateIPTC, timeIPTC, datetimeXML);

							bagMetaIPTC->SetString( ImageMetaIPTC::ExpirationDateTime, datetimeXML);
							bagMetaIPTC->RemoveAttribute( ImageMetaIPTC::ExpirationDate);
							bagMetaIPTC->RemoveAttribute( ImageMetaIPTC::ExpirationTime);
						}
					}
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Special Instructions}", 
														bagMetaIPTC, ImageMetaIPTC::SpecialInstructions);
#if ENABLE_ALL_METAS
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Action Advised}", 
														bagMetaIPTC, ImageMetaIPTC::ActionAdvised);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Reference Service}", 
														bagMetaIPTC, ImageMetaIPTC::ReferenceService);

					if (!bagMetaIPTC->AttributeExists( ImageMetaIPTC::ReferenceDateTime))
					{
						datetime = _WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Reference Date}", 
																		bagMetaIPTC, ImageMetaIPTC::ReferenceDate);
						if (datetime)
						{
							VString dateIPTC, datetimeXML, datetimesXML;

							//get array of IPTC reference dates
							VectorOfVString vDateIPTC;
							bagMetaIPTC->GetString( ImageMetaIPTC::ReferenceDate, dateIPTC);
							dateIPTC.GetSubStrings(";", vDateIPTC);
							//iterate on IPTC reference dates
							VectorOfVString::const_iterator it = vDateIPTC.begin();
							for (;it != vDateIPTC.end();it++)
							{
								//convert single IPTC reference date to XML datetime
								DateTimeFromIPTCToXML( *it, VString(""), datetimeXML);
								if (datetimesXML.IsEmpty())
									datetimesXML = datetimeXML;
								else
								{
									datetimesXML.AppendUniChar(';');
									datetimesXML.AppendString( datetimeXML);
								}
							}
							bagMetaIPTC->SetString( ImageMetaIPTC::ReferenceDateTime, datetimesXML);
							bagMetaIPTC->RemoveAttribute( ImageMetaIPTC::ReferenceDate);
						}
					}

					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Reference Number}", 
														bagMetaIPTC, ImageMetaIPTC::ReferenceNumber);
#endif
					if (!bagMetaIPTC->AttributeExists( ImageMetaIPTC::DateTimeCreated))
					{
						datetime = _WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Date Created}", 
																		bagMetaIPTC, ImageMetaIPTC::DateCreated);
						datetime = _WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Time Created}", 
																		bagMetaIPTC, ImageMetaIPTC::TimeCreated) || datetime;
						if (datetime)
						{
							//convert to XML datetime

							VString dateIPTC, timeIPTC, datetimeXML;
							bagMetaIPTC->GetString( ImageMetaIPTC::DateCreated, dateIPTC);
							bagMetaIPTC->GetString( ImageMetaIPTC::TimeCreated, timeIPTC);
							DateTimeFromIPTCToXML( dateIPTC, timeIPTC, datetimeXML);

							bagMetaIPTC->SetString( ImageMetaIPTC::DateTimeCreated, datetimeXML);
							bagMetaIPTC->RemoveAttribute( ImageMetaIPTC::DateCreated);
							bagMetaIPTC->RemoveAttribute( ImageMetaIPTC::TimeCreated);
						}
					}

					if (!bagMetaIPTC->AttributeExists( ImageMetaIPTC::DigitalCreationDateTime))
					{
						datetime = _WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Digital Creation Date}", 
																		bagMetaIPTC, ImageMetaIPTC::DigitalCreationDate);
						datetime = _WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Digital Creation Time}", 
																		bagMetaIPTC, ImageMetaIPTC::DigitalCreationTime) || datetime;
						if (datetime)
						{
							//convert to XML datetime

							VString dateIPTC, timeIPTC, datetimeXML;
							bagMetaIPTC->GetString( ImageMetaIPTC::DigitalCreationDate, dateIPTC);
							bagMetaIPTC->GetString( ImageMetaIPTC::DigitalCreationTime, timeIPTC);
							DateTimeFromIPTCToXML( dateIPTC, timeIPTC, datetimeXML);

							bagMetaIPTC->SetString( ImageMetaIPTC::DigitalCreationDateTime, datetimeXML);
							bagMetaIPTC->RemoveAttribute( ImageMetaIPTC::DigitalCreationDate);
							bagMetaIPTC->RemoveAttribute( ImageMetaIPTC::DigitalCreationTime);
						}
					}

					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Originating Program}", 
														bagMetaIPTC, ImageMetaIPTC::OriginatingProgram);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Program Version}", 
														bagMetaIPTC, ImageMetaIPTC::ProgramVersion);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Object Cycle}", 
														bagMetaIPTC, ImageMetaIPTC::ObjectCycle);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=By-line}", 
														bagMetaIPTC, ImageMetaIPTC::Byline);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=By-line Title}", 
														bagMetaIPTC, ImageMetaIPTC::BylineTitle);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=City}", 
														bagMetaIPTC, ImageMetaIPTC::City);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Sub-location}", 
														bagMetaIPTC, ImageMetaIPTC::SubLocation);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Province/State}", 
														bagMetaIPTC, ImageMetaIPTC::ProvinceState);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Country/Primary Location Code}", 
														bagMetaIPTC, ImageMetaIPTC::CountryPrimaryLocationCode);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Country/Primary Location Name}", 
														bagMetaIPTC, ImageMetaIPTC::CountryPrimaryLocationName);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Original Transmission Reference}", 
														bagMetaIPTC, ImageMetaIPTC::OriginalTransmissionReference);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Headline}", 
														bagMetaIPTC, ImageMetaIPTC::Headline);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Credit}", 
														bagMetaIPTC, ImageMetaIPTC::Credit);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Source}", 
														bagMetaIPTC, ImageMetaIPTC::Source);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Copyright Notice}", 
														bagMetaIPTC, ImageMetaIPTC::CopyrightNotice);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Contact}", 
														bagMetaIPTC, ImageMetaIPTC::Contact);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Caption/Abstract}", 
														bagMetaIPTC, ImageMetaIPTC::CaptionAbstract);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Writer/Editor}", 
														bagMetaIPTC, ImageMetaIPTC::WriterEditor);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Image Type}", 
														bagMetaIPTC, ImageMetaIPTC::ImageType);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Image Orientation}", 
														bagMetaIPTC, ImageMetaIPTC::ImageOrientation);
					_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Language Identifier}", 
														bagMetaIPTC, ImageMetaIPTC::LanguageIdentifier);
					//_WICKeyValuePairToBagKeyValuePair(	metaReaderIPTC, value, "/{str=Star Rating}", 
					//									bagMetaIPTC, ImageMetaIPTC::StarRating);
				}
				if (bagMetaIPTC->GetAttributesCount() > 0)
					outMetadatas->AddElement( ImageMetaIPTC::IPTC, bagMetaIPTC);
				bagMetaIPTC->Release();
			}
		}

		//read TIFF metadatas if any
		{
			CComPtr<IWICMetadataQueryReader> metaReaderTIFF;

			bool ok = false;
			VString pathTIFF = "/ifd";
#ifndef DEBUG_XMP 
			ok = SUCCEEDED(metaReader->GetMetadataByName(pathTIFF.GetCPointer(), &value));
			if (!ok)
			{
				//try alternative path (JPEG-compliant path)
				pathTIFF = "/app1/ifd";
				ok = SUCCEEDED(metaReader->GetMetadataByName(pathTIFF.GetCPointer(), &value));
			}
#endif
			if (ok || hasXMP)
			{
				if (ok)
				{
					ok = false;
					if (value.vt == VT_UNKNOWN)
						ok = SUCCEEDED(value.punkVal->QueryInterface(IID_IWICMetadataQueryReader, (void **)&metaReaderTIFF));
					PropVariantClear(&value); 
				}

				{
					VValueBag *bagMetaTIFF = new VValueBag();

					if (hasXMP)
					{
						//XMP TIFF tags (TIFF 6.0)

						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/tiff:Compression", 
															bagMetaTIFF, ImageMetaTIFF::Compression);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/tiff:PhotometricInterpretation", 
															bagMetaTIFF, ImageMetaTIFF::PhotometricInterpretation);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/tiff:DocumentName", 
															bagMetaTIFF, ImageMetaTIFF::DocumentName);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/tiff:ImageDescription", 
															bagMetaTIFF, ImageMetaTIFF::ImageDescription);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/tiff:Make", 
															bagMetaTIFF, ImageMetaTIFF::Make);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/tiff:Model", 
															bagMetaTIFF, ImageMetaTIFF::Model);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/tiff:Orientation", 
															bagMetaTIFF, ImageMetaTIFF::Orientation);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/tiff:XResolution", 
															bagMetaTIFF, ImageMetaTIFF::XResolution);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/tiff:YResolution", 
															bagMetaTIFF, ImageMetaTIFF::YResolution);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/tiff:ResolutionUnit", 
															bagMetaTIFF, ImageMetaTIFF::ResolutionUnit);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/tiff:Software", 
															bagMetaTIFF, ImageMetaTIFF::Software);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/tiff:DateTime", 
															bagMetaTIFF, ImageMetaTIFF::DateTime,
															TT_XMP_DATETIME);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/tiff:Artist", 
															bagMetaTIFF, ImageMetaTIFF::Artist);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/tiff:HostComputer", 
															bagMetaTIFF, ImageMetaTIFF::HostComputer);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/tiff:Copyright", 
															bagMetaTIFF, ImageMetaTIFF::Copyright);
#if ENABLE_ALL_METAS
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/tiff:WhitePoint", 
															bagMetaTIFF, ImageMetaTIFF::WhitePoint,
															TT_XMP_SEQ);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/tiff:PrimaryChromaticities", 
															bagMetaTIFF, ImageMetaTIFF::PrimaryChromaticities,
															TT_XMP_SEQ);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/tiff:TransferFunction", 
															bagMetaTIFF, ImageMetaTIFF::TransferFunction,
															TT_XMP_SEQ,
															3*256);
#endif
					}
					if (ok)
					{
						//TIFF tags (TIFF 6.0)

						_WICKeyValuePairToBagKeyValuePair(	metaReaderTIFF, value, "/{ushort=259}", 
															bagMetaTIFF, ImageMetaTIFF::Compression);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderTIFF, value, "/{ushort=262}", 
															bagMetaTIFF, ImageMetaTIFF::PhotometricInterpretation);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderTIFF, value, "/{ushort=269}", 
															bagMetaTIFF, ImageMetaTIFF::DocumentName);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderTIFF, value, "/{ushort=270}", 
															bagMetaTIFF, ImageMetaTIFF::ImageDescription);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderTIFF, value, "/{ushort=271}", 
															bagMetaTIFF, ImageMetaTIFF::Make);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderTIFF, value, "/{ushort=272}", 
															bagMetaTIFF, ImageMetaTIFF::Model);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderTIFF, value, "/{ushort=274}", 
															bagMetaTIFF, ImageMetaTIFF::Orientation);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderTIFF, value, "/{ushort=282}", 
															bagMetaTIFF, ImageMetaTIFF::XResolution);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderTIFF, value, "/{ushort=283}", 
															bagMetaTIFF, ImageMetaTIFF::YResolution);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderTIFF, value, "/{ushort=296}", 
															bagMetaTIFF, ImageMetaTIFF::ResolutionUnit);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderTIFF, value, "/{ushort=305}", 
															bagMetaTIFF, ImageMetaTIFF::Software);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderTIFF, value, "/{ushort=306}", 
															bagMetaTIFF, ImageMetaTIFF::DateTime,
															TT_EXIF_DATETIME);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderTIFF, value, "/{ushort=315}", 
															bagMetaTIFF, ImageMetaTIFF::Artist);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderTIFF, value, "/{ushort=316}", 
															bagMetaTIFF, ImageMetaTIFF::HostComputer);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderTIFF, value, "/{ushort=33432}", 
															bagMetaTIFF, ImageMetaTIFF::Copyright);
#if ENABLE_ALL_METAS
						_WICKeyValuePairToBagKeyValuePair(	metaReaderTIFF, value, "/{ushort=318}", 
															bagMetaTIFF, ImageMetaTIFF::WhitePoint);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderTIFF, value, "/{ushort=319}", 
															bagMetaTIFF, ImageMetaTIFF::PrimaryChromaticities);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderTIFF, value, "/{ushort=301}", 
															bagMetaTIFF, ImageMetaTIFF::TransferFunction);
#endif
					}

					if (bagMetaTIFF->GetAttributesCount() > 0)
						outMetadatas->AddElement( ImageMetaTIFF::TIFF, bagMetaTIFF);
					bagMetaTIFF->Release();
				}
			}
		}

		//read EXIF metadatas if any
		{
			VString pathEXIF = "/exif";
			CComPtr<IWICMetadataQueryReader> metaReaderEXIF;

			bool ok = false;
#ifndef DEBUG_XMP 
			ok = SUCCEEDED(metaReader->GetMetadataByName(pathEXIF.GetCPointer(), &value));
			if (!ok)
			{
				//try alternative path (TIFF-compliant path)
				pathEXIF = "/ifd/exif";
				ok = SUCCEEDED(metaReader->GetMetadataByName(pathEXIF.GetCPointer(), &value));
			}
			if (!ok)
			{
				//try alternative path (JPEG-compliant path)
				pathEXIF = "/app1/ifd/exif";
				ok = SUCCEEDED(metaReader->GetMetadataByName(pathEXIF.GetCPointer(), &value));
			}
#endif
			if (ok || hasXMP)
			{
				if (ok)
				{
					ok = false;
					if (value.vt == VT_UNKNOWN)
						ok = SUCCEEDED(value.punkVal->QueryInterface(IID_IWICMetadataQueryReader, (void **)&metaReaderEXIF));
					PropVariantClear(&value); 
				}

				{
					VValueBag *bagMetaEXIF = new VValueBag();

					if (hasXMP)
					{
						//XMP EXIF tags (Exif 2.2)

						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:ExposureTime", 
															bagMetaEXIF, ImageMetaEXIF::ExposureTime);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:FNumber", 
															bagMetaEXIF, ImageMetaEXIF::FNumber);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:ExposureProgram", 
															bagMetaEXIF, ImageMetaEXIF::ExposureProgram);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:SpectralSensitivity", 
															bagMetaEXIF, ImageMetaEXIF::SpectralSensitivity);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:ISOSpeedRatings", 
															bagMetaEXIF, ImageMetaEXIF::ISOSpeedRatings,
															TT_XMP_SEQ);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:ExifVersion", 
															bagMetaEXIF, ImageMetaEXIF::ExifVersion);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:DateTimeOriginal", 
															bagMetaEXIF, ImageMetaEXIF::DateTimeOriginal,
															TT_XMP_DATETIME);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:DateTimeDigitized", 
															bagMetaEXIF, ImageMetaEXIF::DateTimeDigitized,
															TT_XMP_DATETIME);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:ComponentsConfiguration", 
															bagMetaEXIF, ImageMetaEXIF::ComponentsConfiguration,
															TT_XMP_SEQ);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:CompressedBitsPerPixel", 
															bagMetaEXIF, ImageMetaEXIF::CompressedBitsPerPixel);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:ShutterSpeedValue", 
															bagMetaEXIF, ImageMetaEXIF::ShutterSpeedValue);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:ApertureValue", 
															bagMetaEXIF, ImageMetaEXIF::ApertureValue);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:BrightnessValue", 
															bagMetaEXIF, ImageMetaEXIF::BrightnessValue);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:ExposureCompensation", 
															bagMetaEXIF, ImageMetaEXIF::ExposureBiasValue);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:MaxApertureValue", 
															bagMetaEXIF, ImageMetaEXIF::MaxApertureValue);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:SubjectDistance", 
															bagMetaEXIF, ImageMetaEXIF::SubjectDistance);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:MeteringMode", 
															bagMetaEXIF, ImageMetaEXIF::MeteringMode);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:LightSource", 
															bagMetaEXIF, ImageMetaEXIF::LightSource);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:Flash", 
															bagMetaEXIF, ImageMetaEXIF::Flash);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:FocalLength", 
															bagMetaEXIF, ImageMetaEXIF::FocalLength);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:SubjectArea", 
															bagMetaEXIF, ImageMetaEXIF::SubjectArea,
															TT_XMP_SEQ);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:FlashPixVersion", 
															bagMetaEXIF, ImageMetaEXIF::FlashPixVersion);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:ColorSpace", 
															bagMetaEXIF, ImageMetaEXIF::ColorSpace);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:PixelXDimension", 
															bagMetaEXIF, ImageMetaEXIF::PixelXDimension);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:PixelYDimension", 
															bagMetaEXIF, ImageMetaEXIF::PixelYDimension);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:RelatedSoundFile", 
															bagMetaEXIF, ImageMetaEXIF::RelatedSoundFile);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:FlashEnergy", 
															bagMetaEXIF, ImageMetaEXIF::FlashEnergy);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:FocalPlaneXResolution", 
															bagMetaEXIF, ImageMetaEXIF::FocalPlaneXResolution);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:FocalPlaneYResolution", 
															bagMetaEXIF, ImageMetaEXIF::FocalPlaneYResolution);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:FocalPlaneResolutionUnit", 
															bagMetaEXIF, ImageMetaEXIF::FocalPlaneResolutionUnit);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:SubjectLocation", 
															bagMetaEXIF, ImageMetaEXIF::SubjectLocation,
															TT_XMP_SEQ);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:ExposureIndex", 
															bagMetaEXIF, ImageMetaEXIF::ExposureIndex);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:SensingMethod", 
															bagMetaEXIF, ImageMetaEXIF::SensingMethod);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:FileSource", 
															bagMetaEXIF, ImageMetaEXIF::FileSource);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:SceneType", 
															bagMetaEXIF, ImageMetaEXIF::SceneType);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:CustomRendered", 
															bagMetaEXIF, ImageMetaEXIF::CustomRendered);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:ExposureMode", 
															bagMetaEXIF, ImageMetaEXIF::ExposureMode);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:WhiteBalance", 
															bagMetaEXIF, ImageMetaEXIF::WhiteBalance);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:DigitalZoomRatio", 
															bagMetaEXIF, ImageMetaEXIF::DigitalZoomRatio);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:FocalLenIn35mmFilm", 
															bagMetaEXIF, ImageMetaEXIF::FocalLenIn35mmFilm);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:SceneCaptureType", 
															bagMetaEXIF, ImageMetaEXIF::SceneCaptureType);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GainControl", 
															bagMetaEXIF, ImageMetaEXIF::GainControl);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:Contrast", 
															bagMetaEXIF, ImageMetaEXIF::Contrast);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:Saturation", 
															bagMetaEXIF, ImageMetaEXIF::Saturation);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:Sharpness", 
															bagMetaEXIF, ImageMetaEXIF::Sharpness);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:SubjectDistRange", 
															bagMetaEXIF, ImageMetaEXIF::SubjectDistRange);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:ImageUniqueID", 
															bagMetaEXIF, ImageMetaEXIF::ImageUniqueID);
						//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:", 
						//									bagMetaEXIF, ImageMetaEXIF::Gamma);

						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:MakerNote/x-default", 
															bagMetaEXIF, ImageMetaEXIF::MakerNote);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:UserComment/x-default", 
															bagMetaEXIF, ImageMetaEXIF::UserComment);
#if ENABLE_ALL_METAS
						//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:SubsecTime", 
						//									bagMetaEXIF, ImageMetaEXIF::SubsecTime);
						//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:SubsecTimeOriginal", 
						//									bagMetaEXIF, ImageMetaEXIF::SubsecTimeOriginal);
						//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:SubsecTimeDigitized", 
						//									bagMetaEXIF, ImageMetaEXIF::SubsecTimeDigitized);
						//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:SpatialFrequencyResponse", 
						//									bagMetaEXIF, ImageMetaEXIF::SpatialFrequencyResponse);
						//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:OECF", 
						//									bagMetaEXIF, ImageMetaEXIF::OECF);
						//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:CFAPattern", 
						//									bagMetaEXIF, ImageMetaEXIF::CFAPattern);
						//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:DeviceSettingDescription", 
						//									bagMetaEXIF, ImageMetaEXIF::DeviceSettingDescription);
#endif
					}
					if (ok)
					{
						//EXIF tags (Exif 2.2)

						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=33434}", 
															bagMetaEXIF, ImageMetaEXIF::ExposureTime);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=33437}", 
															bagMetaEXIF, ImageMetaEXIF::FNumber);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=34850}", 
															bagMetaEXIF, ImageMetaEXIF::ExposureProgram);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=34852}", 
															bagMetaEXIF, ImageMetaEXIF::SpectralSensitivity);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=34855}", 
															bagMetaEXIF, ImageMetaEXIF::ISOSpeedRatings);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=36864}", 
															bagMetaEXIF, ImageMetaEXIF::ExifVersion,
															TT_BLOB_ASCII);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=36867}", 
															bagMetaEXIF, ImageMetaEXIF::DateTimeOriginal,
															TT_EXIF_DATETIME);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=36868}", 
															bagMetaEXIF, ImageMetaEXIF::DateTimeDigitized,
															TT_EXIF_DATETIME);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=37121}", 
															bagMetaEXIF, ImageMetaEXIF::ComponentsConfiguration);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=37122}", 
															bagMetaEXIF, ImageMetaEXIF::CompressedBitsPerPixel);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=37377}", 
															bagMetaEXIF, ImageMetaEXIF::ShutterSpeedValue);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=37378}", 
															bagMetaEXIF, ImageMetaEXIF::ApertureValue);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=37379}", 
															bagMetaEXIF, ImageMetaEXIF::BrightnessValue);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=37380}", 
															bagMetaEXIF, ImageMetaEXIF::ExposureBiasValue);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=37381}", 
															bagMetaEXIF, ImageMetaEXIF::MaxApertureValue);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=37382}", 
															bagMetaEXIF, ImageMetaEXIF::SubjectDistance);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=37383}", 
															bagMetaEXIF, ImageMetaEXIF::MeteringMode);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=37384}", 
															bagMetaEXIF, ImageMetaEXIF::LightSource);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=37385}", 
															bagMetaEXIF, ImageMetaEXIF::Flash);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=37386}", 
															bagMetaEXIF, ImageMetaEXIF::FocalLength);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=37396}", 
															bagMetaEXIF, ImageMetaEXIF::SubjectArea);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=40960}", 
															bagMetaEXIF, ImageMetaEXIF::FlashPixVersion,
															TT_BLOB_ASCII);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=40961}", 
															bagMetaEXIF, ImageMetaEXIF::ColorSpace);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=40962}", 
															bagMetaEXIF, ImageMetaEXIF::PixelXDimension);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=40963}", 
															bagMetaEXIF, ImageMetaEXIF::PixelYDimension);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=40964}", 
															bagMetaEXIF, ImageMetaEXIF::RelatedSoundFile);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=41483}", 
															bagMetaEXIF, ImageMetaEXIF::FlashEnergy);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=41486}", 
															bagMetaEXIF, ImageMetaEXIF::FocalPlaneXResolution);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=41487}", 
															bagMetaEXIF, ImageMetaEXIF::FocalPlaneYResolution);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=41488}", 
															bagMetaEXIF, ImageMetaEXIF::FocalPlaneResolutionUnit);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=41492}", 
															bagMetaEXIF, ImageMetaEXIF::SubjectLocation);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=41493}", 
															bagMetaEXIF, ImageMetaEXIF::ExposureIndex);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=41495}", 
															bagMetaEXIF, ImageMetaEXIF::SensingMethod);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=41728}", 
															bagMetaEXIF, ImageMetaEXIF::FileSource);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=41729}", 
															bagMetaEXIF, ImageMetaEXIF::SceneType);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=41985}", 
															bagMetaEXIF, ImageMetaEXIF::CustomRendered);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=41986}", 
															bagMetaEXIF, ImageMetaEXIF::ExposureMode);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=41987}", 
															bagMetaEXIF, ImageMetaEXIF::WhiteBalance);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=41988}", 
															bagMetaEXIF, ImageMetaEXIF::DigitalZoomRatio);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=41989}", 
															bagMetaEXIF, ImageMetaEXIF::FocalLenIn35mmFilm);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=41990}", 
															bagMetaEXIF, ImageMetaEXIF::SceneCaptureType);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=41991}", 
															bagMetaEXIF, ImageMetaEXIF::GainControl);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=41992}", 
															bagMetaEXIF, ImageMetaEXIF::Contrast);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=41993}", 
															bagMetaEXIF, ImageMetaEXIF::Saturation);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=41994}", 
															bagMetaEXIF, ImageMetaEXIF::Sharpness);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=41996}", 
															bagMetaEXIF, ImageMetaEXIF::SubjectDistRange);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=42016}", 
															bagMetaEXIF, ImageMetaEXIF::ImageUniqueID);
						//_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=42016}", 
						//									bagMetaEXIF, ImageMetaEXIF::Gamma);

						//_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=37500}", 
						//									bagMetaEXIF, ImageMetaEXIF::MakerNote);
						//_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=37510}", 
						//									bagMetaEXIF, ImageMetaEXIF::UserComment,
						//									TT_BLOB_EXIF_STRING);
#if ENABLE_ALL_METAS
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=37520}", 
															bagMetaEXIF, ImageMetaEXIF::SubsecTime);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=37521}", 
															bagMetaEXIF, ImageMetaEXIF::SubsecTimeOriginal);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=37522}", 
															bagMetaEXIF, ImageMetaEXIF::SubsecTimeDigitized);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=41484}", 
															bagMetaEXIF, ImageMetaEXIF::SpatialFrequencyResponse);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=34856}", 
															bagMetaEXIF, ImageMetaEXIF::OECF);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=41730}", 
															bagMetaEXIF, ImageMetaEXIF::CFAPattern);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderEXIF, value, "/{ushort=41995}", 
															bagMetaEXIF, ImageMetaEXIF::DeviceSettingDescription);
#endif
					}

					//synchronize IPTC datetimes with Exif datetimes 
					//(workaround for IPTC time tag bug)
					if (bagMetaEXIF->AttributeExists( ImageMetaEXIF::DateTimeOriginal))
					{
						VValueBag *bagIPTC = outMetadatas->GetUniqueElement( ImageMetaIPTC::IPTC);
						if (bagIPTC)
						{
							VString datetime;
							bagMetaEXIF->GetString( ImageMetaEXIF::DateTimeOriginal, 
													datetime);		
							bagIPTC->SetString( ImageMetaIPTC::DateTimeCreated,
												datetime);
						}
					}
					if (bagMetaEXIF->AttributeExists( ImageMetaEXIF::DateTimeDigitized))
					{
						VValueBag *bagIPTC = outMetadatas->GetUniqueElement( ImageMetaIPTC::IPTC);
						if (bagIPTC)
						{
							VString datetime;
							bagMetaEXIF->GetString( ImageMetaEXIF::DateTimeDigitized, 
													datetime);		
							bagIPTC->SetString( ImageMetaIPTC::DigitalCreationDateTime,
												datetime);
						}
					}

					if (bagMetaEXIF->GetAttributesCount() > 0)
						outMetadatas->AddElement( ImageMetaEXIF::EXIF, bagMetaEXIF);
					bagMetaEXIF->Release();
				}
			}
		}

		//read GPS metadatas if any
		{
			VString pathGPS = "/gps";
			CComPtr<IWICMetadataQueryReader> metaReaderGPS;

			bool ok = false;
#ifndef DEBUG_XMP 
			ok = SUCCEEDED(metaReader->GetMetadataByName(pathGPS.GetCPointer(), &value));
			if (!ok)
			{
				//try alternative path (TIFF-compliant path)
				pathGPS = "/ifd/gps";
				ok = SUCCEEDED(metaReader->GetMetadataByName(pathGPS.GetCPointer(), &value));
			}
			if (!ok)
			{
				//try alternative path (JPEG-compliant path)
				pathGPS = "/app1/ifd/gps";
				ok = SUCCEEDED(metaReader->GetMetadataByName(pathGPS.GetCPointer(), &value));
			}
#endif
			if (ok || hasXMP)
			{
				if (ok)
				{
					ok = false;
					if (value.vt == VT_UNKNOWN)
						ok = SUCCEEDED(value.punkVal->QueryInterface(IID_IWICMetadataQueryReader, (void **)&metaReaderGPS));
					PropVariantClear(&value); 
				}

				{
					VValueBag *bagMetaGPS = new VValueBag();

					if (hasXMP)
					{
						//XMP GPS tags (Exif 2.2)

						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSVersionID", 
															bagMetaGPS, ImageMetaGPS::VersionID);
						//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSLatitudeRef", 
						//									bagMetaGPS, ImageMetaGPS::LatitudeRef);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSLatitude", 
															bagMetaGPS, ImageMetaGPS::Latitude);
						//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSLongitudeRef", 
						//									bagMetaGPS, ImageMetaGPS::LongitudeRef);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSLongitude", 
															bagMetaGPS, ImageMetaGPS::Longitude);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSAltitudeRef", 
															bagMetaGPS, ImageMetaGPS::AltitudeRef);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSAltitude", 
															bagMetaGPS, ImageMetaGPS::Altitude);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSSatellites", 
															bagMetaGPS, ImageMetaGPS::Satellites);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSStatus", 
															bagMetaGPS, ImageMetaGPS::Status);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSMeasureMode", 
															bagMetaGPS, ImageMetaGPS::MeasureMode);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSDOP", 
															bagMetaGPS, ImageMetaGPS::DOP);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSSpeedRef", 
															bagMetaGPS, ImageMetaGPS::SpeedRef);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSSpeed", 
															bagMetaGPS, ImageMetaGPS::Speed);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSTrackRef", 
															bagMetaGPS, ImageMetaGPS::TrackRef);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSTrack", 
															bagMetaGPS, ImageMetaGPS::Track);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSImgDirectionRef", 
															bagMetaGPS, ImageMetaGPS::ImgDirectionRef);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSImgDirection", 
															bagMetaGPS, ImageMetaGPS::ImgDirection);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSMapDatum", 
															bagMetaGPS, ImageMetaGPS::MapDatum);
						//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSDestLatitudeRef", 
						//									bagMetaGPS, ImageMetaGPS::DestLatitudeRef);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSDestLatitude", 
															bagMetaGPS, ImageMetaGPS::DestLatitude);
						//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSDestLongitudeRef", 
						//									bagMetaGPS, ImageMetaGPS::DestLongitudeRef);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSDestLongitude", 
															bagMetaGPS, ImageMetaGPS::DestLongitude);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSDestBearingRef", 
															bagMetaGPS, ImageMetaGPS::DestBearingRef);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSDestBearing", 
															bagMetaGPS, ImageMetaGPS::DestBearing);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSDestDistanceRef", 
															bagMetaGPS, ImageMetaGPS::DestDistanceRef);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSDestDistance", 
															bagMetaGPS, ImageMetaGPS::DestDistance);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSProcessingMethod", 
															bagMetaGPS, ImageMetaGPS::ProcessingMethod);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSAreaInformation", 
															bagMetaGPS, ImageMetaGPS::AreaInformation);
						//_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSDateStamp", 
						//									bagMetaGPS, ImageMetaGPS::DateStamp);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSTimeStamp", 
															bagMetaGPS, ImageMetaGPS::DateTime,
															TT_XMP_DATETIME);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderXMP, value, "/exif:GPSDifferential", 
															bagMetaGPS, ImageMetaGPS::Differential);
					}
					if (ok)
					{
						//GPS tags (Exif 2.2)

						_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=0}", 
															bagMetaGPS, ImageMetaGPS::VersionID,
															TT_GPSVERSIONID);
						if (!bagMetaGPS->AttributeExists( ImageMetaGPS::Latitude))
						{
							_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=1}", 
																bagMetaGPS, ImageMetaGPS::LatitudeRef);
							_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=2}", 
																bagMetaGPS, ImageMetaGPS::Latitude);
							if (bagMetaGPS->AttributeExists( ImageMetaGPS::Latitude)
								&&
								bagMetaGPS->AttributeExists( ImageMetaGPS::LatitudeRef))
							{
								//convert to XMP latitude 
								VString latitude, latitudeRef;
								bagMetaGPS->GetString( ImageMetaGPS::Latitude, latitude);
								bagMetaGPS->GetString( ImageMetaGPS::LatitudeRef, latitudeRef);
								VString latitudeXMP;
								GPSCoordsFromExifToXMP( latitude, latitudeRef, latitudeXMP);
								bagMetaGPS->SetString( ImageMetaGPS::Latitude, latitudeXMP);
								bagMetaGPS->RemoveAttribute( ImageMetaGPS::LatitudeRef);
							}
							else
							{
								bagMetaGPS->RemoveAttribute( ImageMetaGPS::Latitude);
								bagMetaGPS->RemoveAttribute( ImageMetaGPS::LatitudeRef);
							}
						}
						if (!bagMetaGPS->AttributeExists( ImageMetaGPS::Longitude))
						{
							_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=3}", 
																bagMetaGPS, ImageMetaGPS::LongitudeRef);
							_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=4}", 
																bagMetaGPS, ImageMetaGPS::Longitude);
							if (bagMetaGPS->AttributeExists( ImageMetaGPS::Longitude)
								&&
								bagMetaGPS->AttributeExists( ImageMetaGPS::LongitudeRef))
							{
								//convert to XMP latitude 
								VString longitude, longitudeRef;
								bagMetaGPS->GetString( ImageMetaGPS::Longitude, longitude);
								bagMetaGPS->GetString( ImageMetaGPS::LongitudeRef, longitudeRef);
								VString longitudeXMP;
								GPSCoordsFromExifToXMP( longitude, longitudeRef, longitudeXMP);
								bagMetaGPS->SetString( ImageMetaGPS::Longitude, longitudeXMP);
								bagMetaGPS->RemoveAttribute( ImageMetaGPS::LongitudeRef);
							}
							else
							{
								bagMetaGPS->RemoveAttribute( ImageMetaGPS::Longitude);
								bagMetaGPS->RemoveAttribute( ImageMetaGPS::LongitudeRef);
							}
						}
						_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=5}", 
															bagMetaGPS, ImageMetaGPS::AltitudeRef);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=6}", 
															bagMetaGPS, ImageMetaGPS::Altitude);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=8}", 
															bagMetaGPS, ImageMetaGPS::Satellites);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=9}", 
															bagMetaGPS, ImageMetaGPS::Status);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=10}", 
															bagMetaGPS, ImageMetaGPS::MeasureMode);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=11}", 
															bagMetaGPS, ImageMetaGPS::DOP);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=12}", 
															bagMetaGPS, ImageMetaGPS::SpeedRef);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=13}", 
															bagMetaGPS, ImageMetaGPS::Speed);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=14}", 
															bagMetaGPS, ImageMetaGPS::TrackRef);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=15}", 
															bagMetaGPS, ImageMetaGPS::Track);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=16}", 
															bagMetaGPS, ImageMetaGPS::ImgDirectionRef);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=17}", 
															bagMetaGPS, ImageMetaGPS::ImgDirection);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=18}", 
															bagMetaGPS, ImageMetaGPS::MapDatum);
						if (!bagMetaGPS->AttributeExists( ImageMetaGPS::DestLatitude))
						{
							_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=19}", 
																bagMetaGPS, ImageMetaGPS::DestLatitudeRef);
							_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=20}", 
																bagMetaGPS, ImageMetaGPS::DestLatitude);
							if (bagMetaGPS->AttributeExists( ImageMetaGPS::DestLatitude)
								&&
								bagMetaGPS->AttributeExists( ImageMetaGPS::DestLatitudeRef))
							{
								//convert to XMP latitude 
								VString latitude, latitudeRef;
								bagMetaGPS->GetString( ImageMetaGPS::DestLatitude, latitude);
								bagMetaGPS->GetString( ImageMetaGPS::DestLatitudeRef, latitudeRef);
								VString latitudeXMP;
								GPSCoordsFromExifToXMP( latitude, latitudeRef, latitudeXMP);
								bagMetaGPS->SetString( ImageMetaGPS::DestLatitude, latitudeXMP);
								bagMetaGPS->RemoveAttribute( ImageMetaGPS::DestLatitudeRef);
							}
							else
							{
								bagMetaGPS->RemoveAttribute( ImageMetaGPS::DestLatitude);
								bagMetaGPS->RemoveAttribute( ImageMetaGPS::DestLatitudeRef);
							}
						}
						if (!bagMetaGPS->AttributeExists( ImageMetaGPS::DestLongitude))
						{
							_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=21}", 
																bagMetaGPS, ImageMetaGPS::DestLongitudeRef);
							_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=22}", 
																bagMetaGPS, ImageMetaGPS::DestLongitude);
							if (bagMetaGPS->AttributeExists( ImageMetaGPS::DestLongitude)
								&&
								bagMetaGPS->AttributeExists( ImageMetaGPS::DestLongitudeRef))
							{
								//convert to XMP latitude 
								VString longitude, longitudeRef;
								bagMetaGPS->GetString( ImageMetaGPS::DestLongitude, longitude);
								bagMetaGPS->GetString( ImageMetaGPS::DestLongitudeRef, longitudeRef);
								VString longitudeXMP;
								GPSCoordsFromExifToXMP( longitude, longitudeRef, longitudeXMP);
								bagMetaGPS->SetString( ImageMetaGPS::DestLongitude, longitudeXMP);
								bagMetaGPS->RemoveAttribute( ImageMetaGPS::DestLongitudeRef);
							}
							else
							{
								bagMetaGPS->RemoveAttribute( ImageMetaGPS::DestLongitude);
								bagMetaGPS->RemoveAttribute( ImageMetaGPS::DestLongitudeRef);
							}
						}
						_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=23}", 
															bagMetaGPS, ImageMetaGPS::DestBearingRef);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=24}", 
															bagMetaGPS, ImageMetaGPS::DestBearing);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=25}", 
															bagMetaGPS, ImageMetaGPS::DestDistanceRef);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=26}", 
															bagMetaGPS, ImageMetaGPS::DestDistance);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=27}", 
															bagMetaGPS, ImageMetaGPS::ProcessingMethod,
															TT_BLOB_EXIF_STRING);
						_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=28}", 
															bagMetaGPS, ImageMetaGPS::AreaInformation,
															TT_BLOB_EXIF_STRING);
						if (!bagMetaGPS->AttributeExists( ImageMetaGPS::DateTime))
						{
							_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=29}", 
																bagMetaGPS, ImageMetaGPS::DateStamp);
							_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=7}", 
																bagMetaGPS, ImageMetaGPS::TimeStamp);
							if (bagMetaGPS->AttributeExists( ImageMetaGPS::DateStamp)
								||
								bagMetaGPS->AttributeExists( ImageMetaGPS::TimeStamp))
							{
								//convert to XML datetime

								VString dateGPS, timeGPS, datetimeXML;
								bagMetaGPS->GetString( ImageMetaGPS::DateStamp, dateGPS);
								bagMetaGPS->GetString( ImageMetaGPS::TimeStamp, timeGPS);
								DateTimeFromGPSToXML( dateGPS, timeGPS, datetimeXML);

								bagMetaGPS->SetString( ImageMetaGPS::DateTime, datetimeXML);
								bagMetaGPS->RemoveAttribute( ImageMetaGPS::DateStamp);
								bagMetaGPS->RemoveAttribute( ImageMetaGPS::TimeStamp);
							}
						}
						_WICKeyValuePairToBagKeyValuePair(	metaReaderGPS, value, "/{ushort=30}", 
															bagMetaGPS, ImageMetaGPS::Differential);
					}

					if (bagMetaGPS->GetAttributesCount() > 0)
						outMetadatas->AddElement( ImageMetaGPS::GPS, bagMetaGPS);
					bagMetaGPS->Release();
				}
			}
		}
	}

#if VERSIONDEBUG	
	VString bagDump;
	outMetadatas->DumpXML(bagDump, VString("Metadatas"), true);
#endif
}


/** convert from WIC metadata key-value pair to metadata bag key-value pair 
@remarks
	WIC metadatas tag types are converted to bag values types as follow:

	VT_I1,VT_UI1,VT_I2,VT_UI2,VT_I4,VT_UI4,VT_BOOL <-> VLong 
	VT_UI8, VT_I8, VT_R4, VT_R8 <-> VReal
	VT_LPSTR,VT_LPWSTR <-> VString
	VT_BLOB <->	VString (encoding depending on size of VT_BLOB and if VT_BLOB contains only ASCII characters
						 or a Exif-compliant string - any number of characters preceded by a 8-bytes ASCII header which identifies character code)
	VT_UNKNOWN <-> VString (either a XMP bag or seq: child elements values are encoded
							in a string separated by ';')

	if tag repeat count > 1 (prop variant type & VT_VECTOR != 0), 
	tag values are string-encoded (in a VString value) using ';' as separator
@note
	WIC uses VT_UI8 and VT_I8 for RATIONAL and SRATIONAL
	(where real value is equal to lowPart/HighPart)
*/
bool stImageSource::_WICKeyValuePairToBagKeyValuePair(	const CComPtr<IWICMetadataQueryReader>& inMetaReader,
														PROPVARIANT& ioValue,
														const VString& inWICTagItem,
														VValueBag *ioBagMeta, const XBOX::VValueBag::StKey& inBagKey,
														eTagType inTagType,
														const VSize inMaxSizeBagOrSeq)
{
	//skip if attribute exists yet (ie if attribute have been initialized from XML tag)
	if (ioBagMeta->AttributeExists( inBagKey))
		return true;

	xbox_assert(sizeof(WCHAR) == sizeof(UniChar));
    if (SUCCEEDED(inMetaReader->GetMetadataByName((LPCWSTR)inWICTagItem.GetCPointer(), &ioValue)))
	{
		if (ioValue.vt == VT_UNKNOWN)
		{
			if (inTagType == TT_XMP_BAG || inTagType == TT_XMP_SEQ)
			{
				//tag is a xmp complex type: bag or seq
				//build string from xmp child elements separated with ';'
				//(no matter here if it is seq or bag: xmp child elements are the same in WIC implementation:
				// WIC stores bag or seq childs elements in tags {ulong=0},...,{ulong=n-1}
				// where n is the count of child elements)
				
				CComPtr<IWICMetadataQueryReader> metaReaderBag;
				if (SUCCEEDED(ioValue.punkVal->QueryInterface(IID_IWICMetadataQueryReader, (void **)&metaReaderBag)))
				{
					PropVariantClear(&ioValue);

					//build temp bag with XMP child values stored as attributes
					VValueBag *bagValues = new VValueBag();
					for (sLONG count = 0; count < inMaxSizeBagOrSeq; count++) 
					{
						VString keyString;
						keyString.FromLong( count);
						VValueBag::StKey key(keyString);
						if (!_WICKeyValuePairToBagKeyValuePair(metaReaderBag, 
															  ioValue,
															  "/{ulong="+keyString+"}",
															  bagValues, key))
							break;
					}

					//build string value from temp bag attributes values
					//using ';' as separator
					VString value;
					if (bagValues->GetAttributesCount() >= 1)
					{
						const VValueSingle *valueSingle = bagValues->GetNthAttribute(1, NULL);
						valueSingle->GetString( value);
					
						for (sLONG index = 2; index <= bagValues->GetAttributesCount(); index++)
						{
							value.AppendUniChar(';');
							const VValueSingle *valueSingle = bagValues->GetNthAttribute(index, NULL);
							VString valueString;
							valueSingle->GetString( valueString);
							value.AppendString( valueString);
						}

						ioBagMeta->SetString( inBagKey, value);
						delete bagValues;
						return true;
					}
					else
					{
						delete bagValues;
						return false;
					}
				}
			}

			PropVariantClear(&ioValue);
			return false;
		}
		else if (ioValue.vt == VT_BLOB)
		{
			if (ioValue.blob.cbSize < 1)
			{
				PropVariantClear(&ioValue); 
				return false;
			}
			else if (inTagType == TT_BLOB_ASCII)
			{
				//tag value contains only ASCII characters
				//so build string directly from blob 

				char *text = new char [ioValue.blob.cbSize+1];
				memcpy(text,ioValue.blob.pBlobData,ioValue.blob.cbSize);
				text[ioValue.blob.cbSize] = 0;
				VString value((const char *)text);
				delete [] text;

				ioBagMeta->SetString( inBagKey, value);
			}
			else if (inTagType == TT_BLOB_EXIF_STRING)
			{
				if (ioValue.blob.cbSize > 8)
				{
					//blob is a Exif-compliant string ie
					//a block of characters preceeded by a 8-bytes ASCII header 
					//which identifies character code

					//get character code string
					char charCode[8+1];
					memcpy(charCode,ioValue.blob.pBlobData,8);
					charCode[8] = 0;
					VString sCharCode((const char *)charCode);
					CharSet charSet;
					if (sCharCode.EqualToString("ASCII",true))
						charSet = VTC_Win32Ansi;
					else if (sCharCode.EqualToString("JIS",true))
						charSet = VTC_JIS;
					else if (sCharCode.EqualToString("UNICODE",true))
						charSet = VTC_UTF_16;
					else 
						charSet = VTC_UNKNOWN;

					//read block of characters
					VString value;
					value.FromBlock(ioValue.blob.pBlobData+8,ioValue.blob.cbSize-8,charSet);

					if (!value.IsEmpty())
						ioBagMeta->SetString( inBagKey, value);
					else
					{
						PropVariantClear(&ioValue); 
						return false;
					}
				}
				else
				{
					PropVariantClear(&ioValue); 
					return false;
				}
			}
			else
			{
				VString value;
				//encode as blob
				if (ioValue.blob.cbSize >= 1)
				{
					VBlobWithPtr blob;
					blob.PutData(ioValue.blob.pBlobData, (VSize)ioValue.blob.cbSize);
					ImageMeta::stWriter::SetBlob( ioBagMeta, inBagKey, *(static_cast<VBlob *>(&blob)));
				}
			}
			PropVariantClear(&ioValue); 
			return true;
		}

		//is value repeat count > 1 ?
		if ((ioValue.vt & VT_VECTOR) != 0)
		{
			VARTYPE vt = ioValue.vt & (~VT_VECTOR);

			//is value of type integer ?
			bool isInteger = true;
			VString value;
			switch (vt)
			{
			case VT_I1:
				{
					CAC *t = &(ioValue.cac);
					for (int i = 0; i < t->cElems; i++)
					{
						if (value.IsEmpty())
							value.FromLong( (sLONG)t->pElems[i]);
						else
						{
							value.AppendUniChar(';');
							value.AppendLong( (sLONG)t->pElems[i]);
						}
					}
				}
				break;
			case VT_UI1:
				{
					CAUB *t = &(ioValue.caub);
					for (int i = 0; i < t->cElems; i++)
					{
						if (value.IsEmpty())
							value.FromLong( (sLONG)t->pElems[i]);
						else
						{
							value.AppendUniChar(';');
							value.AppendLong( (sLONG)t->pElems[i]);
						}
					}
				}
				break;
			case VT_I2:
				{
					CAI *t = &(ioValue.cai);
					for (int i = 0; i < t->cElems; i++)
					{
						if (value.IsEmpty())
							value.FromLong( (sLONG)t->pElems[i]);
						else
						{
							value.AppendUniChar(';');
							value.AppendLong( (sLONG)t->pElems[i]);
						}
					}
				}
				break;
			case VT_UI2: 
				{
					CAUI *t = &(ioValue.caui);
					for (int i = 0; i < t->cElems; i++)
					{
						if (value.IsEmpty())
							value.FromLong( (sLONG)t->pElems[i]);
						else
						{
							value.AppendUniChar(';');
							value.AppendLong( (sLONG)t->pElems[i]);
						}
					}
				}
				break;
			case VT_I4:
				{
					CAL *t = &(ioValue.cal);
					for (int i = 0; i < t->cElems; i++)
					{
						if (value.IsEmpty())
							value.FromLong( (sLONG)t->pElems[i]);
						else
						{
							value.AppendUniChar(';');
							value.AppendLong( (sLONG)t->pElems[i]);
						}
					}
				}
				break;
			case VT_UI4:
				{
					CAUL *t = &(ioValue.caul);
					for (int i = 0; i < t->cElems; i++)
					{
						if (value.IsEmpty())
							value.FromLong( (sLONG)t->pElems[i]);
						else
						{
							value.AppendUniChar(';');
							value.AppendLong( (sLONG)t->pElems[i]);
						}
					}
				}
				break;
			case VT_BOOL:
				{
					CABOOL *t = &(ioValue.cabool);
					for (int i = 0; i < t->cElems; i++)
					{
						if (value.IsEmpty())
							value.FromLong( (sLONG)(t->pElems[i] != 0 ? 1 : 0));
						else
						{
							value.AppendUniChar(';');
							value.AppendLong( (sLONG)(t->pElems[i] != 0 ? 1 : 0));
						}
					}
				}
				break;
			default:
				isInteger = false;
				break;
			}
			if (isInteger)
			{
				if (inTagType == TT_GPSVERSIONID)
					//use XMP separator
					value.ExchangeAll(';','.');
				ioBagMeta->SetString( inBagKey, value);
			}
			else
			{
				//is value of type real ?
				bool isReal = true;
				switch (vt)
				{
				case VT_I8:
					{
						CAH *t = &(ioValue.cah);
						for (int i = 0; i < t->cElems; i++)
						{
							if (value.IsEmpty())
								value.FromReal( t->pElems[i].HighPart != 0 ? 
												((Real)((sLONG)t->pElems[i].LowPart))/((sLONG)t->pElems[i].HighPart) : 
												((Real)((sLONG)t->pElems[i].LowPart)) );
							else
							{
								value.AppendUniChar(';');
								value.AppendReal(	t->pElems[i].HighPart != 0 ? 
													((Real)((sLONG)t->pElems[i].LowPart))/((sLONG)t->pElems[i].HighPart) : 
													((Real)((sLONG)t->pElems[i].LowPart)) );
							}
						}
					}
					break;
				case VT_UI8:
					{
						CAUH *t = &(ioValue.cauh);
						for (int i = 0; i < t->cElems; i++)
						{
							if (value.IsEmpty())
								value.FromReal( t->pElems[i].HighPart != 0 ? 
												((Real)t->pElems[i].LowPart)/t->pElems[i].HighPart : 
												((Real)t->pElems[i].LowPart) );
							else
							{
								value.AppendUniChar(';');
								value.AppendReal( t->pElems[i].HighPart != 0 ? 
												  ((Real)t->pElems[i].LowPart)/t->pElems[i].HighPart : 
												  ((Real)t->pElems[i].LowPart) );
							}
						}
					}
					break;
				case VT_R4:
					{
						CAFLT *t = &(ioValue.caflt);
						for (int i = 0; i < t->cElems; i++)
						{
							if (value.IsEmpty())
								value.FromReal( (Real)t->pElems[i]);
							else
							{
								value.AppendUniChar(';');
								value.AppendReal( (Real)t->pElems[i]);
							}
						}
					}
					break;
				case VT_R8:
					{
						CADBL *t = &(ioValue.cadbl);
						for (int i = 0; i < t->cElems; i++)
						{
							if (value.IsEmpty())
								value.FromReal( (Real)t->pElems[i]);
							else
							{
								value.AppendUniChar(';');
								value.AppendReal( (Real)t->pElems[i]);
							}
						}
					}
					break;
				default:
					isReal = false;
					break;
				}
				if (isReal)
					ioBagMeta->SetString( inBagKey, value);
				else
				{
					//is value of type string ?
					bool isString = true;
					VString valString;
					switch (vt)
					{
					case VT_LPSTR:
						{
							CALPSTR *t = &(ioValue.calpstr);
							for (int i = 0; i < t->cElems; i++)
							{
								if (value.IsEmpty())
									value = (const char *)t->pElems[i];
								else
								{
									value.AppendUniChar(';');
									value.AppendCString( (const char *)t->pElems[i]);
								}
							}
						}
						break;
					case VT_LPWSTR:
						{
							xbox_assert(sizeof(WCHAR) == sizeof(UniChar));

							CALPWSTR *t = &(ioValue.calpwstr);
							for (int i = 0; i < t->cElems; i++)
							{
								if (value.IsEmpty())
									value = (const UniChar *)t->pElems[i];
								else
								{
									value.AppendUniChar(';');
									value.AppendUniCString( (const UniChar *)t->pElems[i]);
								}
							}
						}
						break;
					default:
						isString = false;
						break;
					}
					if (isString)
						ioBagMeta->SetString( inBagKey, value);
					else
					{
						xbox_assert(false);
						PropVariantClear(&ioValue); 
						return false;
					}
				}
			}
			PropVariantClear(&ioValue); 
			return true;
		}

		//value repeat count is 1:

		//is value of type integer ?
		bool isInteger = true;
		sLONG valInt = 0;
		switch (ioValue.vt)
		{
		case VT_I1:
			valInt = (sLONG)ioValue.cVal;
			break;
		case VT_UI1:
			valInt = (sLONG)ioValue.bVal;
			break;
		case VT_I2:
			valInt = (sLONG)ioValue.iVal;
			break;
		case VT_UI2:
			valInt = (sLONG)ioValue.uiVal;
			break;
		case VT_I4:
			valInt = (sLONG)ioValue.lVal;
			break;
		case VT_UI4:
			valInt = (sLONG)ioValue.ulVal;
			break;
		case VT_BOOL:
			valInt = ioValue.boolVal != 0 ? 1 : 0;
			break;
		default:
			isInteger = false;
			break;
		}
		if (isInteger)
			ioBagMeta->SetLong( inBagKey, valInt);
		else
		{
			//is value of type real ?
			bool isReal = true;
			Real valReal = 0.0;
			switch (ioValue.vt)
			{
			case VT_I8:
				valReal =	ioValue.hVal.HighPart != 0 ? 
							((Real)((sLONG)ioValue.hVal.LowPart))/((sLONG)ioValue.hVal.HighPart) : 
							((Real)((sLONG)ioValue.hVal.LowPart));
				break;
			case VT_UI8:
				valReal =	ioValue.uhVal.HighPart != 0 ? 
							((Real)ioValue.uhVal.LowPart)/ioValue.uhVal.HighPart : 
							((Real)ioValue.uhVal.LowPart);
				break;
			case VT_R4:
				valReal = (Real)ioValue.fltVal;
				break;
			case VT_R8:
				valReal = (Real)ioValue.dblVal;
				break;
			default:
				isReal = false;
				break;
			}
			if (isReal)
				ioBagMeta->SetReal( inBagKey, valReal);
			else
			{
				//is value of type string ?
				bool isString = true;
				VString valString;
				switch (ioValue.vt)
				{
				case VT_LPSTR:
					valString = (const char *)ioValue.pszVal;
					break;
				case VT_LPWSTR:
					{
					xbox_assert(sizeof(WCHAR) == sizeof(UniChar));
					valString = (const UniChar *)ioValue.pwszVal;
					}
					break;
				default:
					isString = false;
					break;
				}
				if (isString)
				{
					if (inTagType == TT_XMP_DATETIME)
					{
						//convert to XML datetime
						VString valStringXMP = valString;
						DateTimeFromXMPToXML( valStringXMP, valString);
					}
					else if (inTagType == TT_EXIF_DATETIME)
					{
						//convert to XML datetime
						VString valStringEXIF = valString;
						DateTimeFromExifToXML( valStringEXIF, valString);
					}

					ioBagMeta->SetString( inBagKey, valString);
				}
				else
				{
					xbox_assert(false);
					PropVariantClear(&ioValue); 
					return false;
				}
			}
		}
		PropVariantClear(&ioValue); 
		return true;
	}
	return false;
}


/** write metadatas to the destination frame */
bool stImageSource::WriteMetadatas(
					CComPtr<IWICImagingFactory>& ioFactory,
					CComPtr<IWICBitmapFrameEncode>& ioFrame,
					const VValueBag *inBagMetas,
					const GUID& inGUIDFormatContainer,
					uLONG inImageWidth,
					uLONG inImageHeight,
					Real inXResolution,
					Real inYResolution,
 					VPictureDataProvider *inImageSrcDataProvider,
					bool inEncodeImageFrames
					)
{
	//will return false only if we failed to encode all modified metadatas
	//(ie if we cannot encode any new metas and if we are called from 4D command SET PICTURE METADATAS)
	bool result = inBagMetas == NULL || inBagMetas->IsEmpty();

	//skip formats which do not support IPTC/EXIF/TIFF or XMP metadatas
	if (inGUIDFormatContainer == GUID_ContainerFormatBmp)
		return result;
	else if (inGUIDFormatContainer == GUID_ContainerFormatIco)
		return result;
	else if (inGUIDFormatContainer == GUID_ContainerFormatGif)
		return result;

	stImageSource imageSrc( ioFactory, inImageSrcDataProvider, GUID_NULL);
	{

	//get root metadatas writer
	CComPtr<IWICMetadataQueryWriter> metaWriter;
	if (!SUCCEEDED(ioFrame->GetMetadataQueryWriter(
				   &metaWriter)))
		return result;

	if (inImageSrcDataProvider)
	{
		//copy image src metadatas to image dest
		//(can occur only if image src GUID == image dest GUID)

		imageSrc.CopyMetadatas( metaWriter);
	}

	//determine XMP block path
	VString pathXMP;
	if (inGUIDFormatContainer == GUID_ContainerFormatJpeg)
		pathXMP = "/xmp";
	else if (inGUIDFormatContainer == GUID_ContainerFormatTiff)
		pathXMP = "/ifd/xmp";
	else if (inGUIDFormatContainer == GUID_ContainerFormatWmp)
		pathXMP = "/ifd/xmp";
	else
		//unknown format: try with TIFF-compliant format
		pathXMP = "/ifd/xmp";

	//determine TIFF block path
	VString pathTIFF;
	if (inGUIDFormatContainer == GUID_ContainerFormatJpeg)
		pathTIFF = "/app1/ifd";
	else if (inGUIDFormatContainer == GUID_ContainerFormatTiff)
		pathTIFF = "/ifd";
	else if (inGUIDFormatContainer == GUID_ContainerFormatWmp)
		pathTIFF = "/ifd";
	else
		//unknown format: try with TIFF-compliant format
		pathTIFF = "/ifd";

	if (inEncodeImageFrames)
	{
		//as frame is encoded from gdiplus image and not copied from the data provider,
		//we need to purge some metas
		metaWriter->RemoveMetadataByName(VString(pathXMP+"/tiff:Compression").GetCPointer());
		metaWriter->RemoveMetadataByName(VString(pathTIFF+"/{ushort=259}").GetCPointer());

		metaWriter->RemoveMetadataByName(VString(pathXMP+"/tiff:TransferFunction").GetCPointer());
		metaWriter->RemoveMetadataByName(VString(pathTIFF+"/{ushort=301}").GetCPointer());

		PROPVARIANT value;
		PropVariantInit(&value);

		//use actual image resolution
		VReal xres(inXResolution); 
		VReal yres(inYResolution);
		VLong resUnit(2);
		_WICKeyValuePairFromVValueSingle(	metaWriter, value, pathXMP+"/tiff:XResolution", 
											static_cast<const VValueSingle *>(&xres),
											TT_UI8);
		_WICKeyValuePairFromVValueSingle(	metaWriter, value, pathXMP+"/tiff:YResolution", 
											static_cast<const VValueSingle *>(&yres),
											TT_UI8);
		_WICKeyValuePairFromVValueSingle(	metaWriter, value, pathXMP+"/tiff:ResolutionUnit", 
											static_cast<const VValueSingle *>(&resUnit),
											TT_UI2);

		_WICKeyValuePairFromVValueSingle(	metaWriter, value, pathTIFF+"/{ushort=282}", 
											static_cast<const VValueSingle *>(&xres),
											TT_UI8);
		_WICKeyValuePairFromVValueSingle(	metaWriter, value, pathTIFF+"/{ushort=283}", 
											static_cast<const VValueSingle *>(&yres),
											TT_UI8);
		_WICKeyValuePairFromVValueSingle(	metaWriter, value, pathTIFF+"/{ushort=296}", 
											static_cast<const VValueSingle *>(&resUnit),
											TT_UI2);
	}

	const VValueBag *bagMetaTIFF = inBagMetas ? inBagMetas->RetainUniqueElement(ImageMetaTIFF::TIFF) : NULL;
	if (bagMetaTIFF)
	{
		PROPVARIANT value;
		PropVariantInit(&value);

		//write XMP TIFF metadatas (TIFF 6.0)

		//_WICKeyValuePairFromBagKeyValuePair(metaWriter, value, "/tiff:Compression", 
		//									bagMetaTIFF, ImageMetaTIFF::Compression,
		//									TT_UI2);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/tiff:XResolution", 
											bagMetaTIFF, ImageMetaTIFF::XResolution,
											TT_UI8);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/tiff:YResolution", 
											bagMetaTIFF, ImageMetaTIFF::YResolution,
											TT_UI8);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/tiff:ResolutionUnit", 
											bagMetaTIFF, ImageMetaTIFF::ResolutionUnit,
											TT_UI2);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/tiff:PhotometricInterpretation", 
											bagMetaTIFF, ImageMetaTIFF::PhotometricInterpretation,
											TT_UI2);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/tiff:DocumentName", 
											bagMetaTIFF, ImageMetaTIFF::DocumentName,
											TT_LPWSTR);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/tiff:ImageDescription", 
											bagMetaTIFF, ImageMetaTIFF::ImageDescription,
											TT_LPWSTR);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/tiff:Make", 
											bagMetaTIFF, ImageMetaTIFF::Make,
											TT_LPWSTR);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/tiff:Model", 
											bagMetaTIFF, ImageMetaTIFF::Model,
											TT_LPWSTR);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/tiff:Orientation", 
											bagMetaTIFF, ImageMetaTIFF::Orientation,
											TT_UI2);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/tiff:Software", 
											bagMetaTIFF, ImageMetaTIFF::Software,
											TT_LPWSTR);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/tiff:DateTime", 
											bagMetaTIFF, ImageMetaTIFF::DateTime,
											TT_XMP_DATETIME);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/tiff:Artist", 
											bagMetaTIFF, ImageMetaTIFF::Artist,
											TT_LPWSTR);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/tiff:HostComputer", 
											bagMetaTIFF, ImageMetaTIFF::HostComputer,
											TT_LPWSTR);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/tiff:Copyright", 
											bagMetaTIFF, ImageMetaTIFF::Copyright,
											TT_LPWSTR);
#if ENABLE_ALL_METAS
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/tiff:WhitePoint", 
											bagMetaTIFF, ImageMetaTIFF::WhitePoint,
											TT_XMP_SEQ_UI8,
											ioFactory);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/tiff:PrimaryChromaticities", 
											bagMetaTIFF, ImageMetaTIFF::PrimaryChromaticities,
											TT_XMP_SEQ_UI8,
											ioFactory);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/tiff:TransferFunction", 
											bagMetaTIFF, ImageMetaTIFF::TransferFunction,
											TT_XMP_SEQ_UI2,
											ioFactory);
#endif
		{
			//write TIFF metadatas

			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathTIFF+"/{ushort=282}", 
												bagMetaTIFF, ImageMetaTIFF::XResolution,
												TT_UI8);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathTIFF+"/{ushort=283}", 
												bagMetaTIFF, ImageMetaTIFF::YResolution,
												TT_UI8);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathTIFF+"/{ushort=296}", 
												bagMetaTIFF, ImageMetaTIFF::ResolutionUnit,
												TT_UI2);

			//result = result || _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathTIFF+"/{ushort=259}", 
			//									bagMetaTIFF, ImageMetaTIFF::Compression,
			//									TT_UI2);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathTIFF+"/{ushort=262}", 
												bagMetaTIFF, ImageMetaTIFF::PhotometricInterpretation,
												TT_UI2);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathTIFF+"/{ushort=269}", 
												bagMetaTIFF, ImageMetaTIFF::DocumentName,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathTIFF+"/{ushort=270}", 
												bagMetaTIFF, ImageMetaTIFF::ImageDescription,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathTIFF+"/{ushort=271}", 
												bagMetaTIFF, ImageMetaTIFF::Make,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathTIFF+"/{ushort=272}", 
												bagMetaTIFF, ImageMetaTIFF::Model,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathTIFF+"/{ushort=274}", 
												bagMetaTIFF, ImageMetaTIFF::Orientation,
												TT_UI2);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathTIFF+"/{ushort=305}", 
												bagMetaTIFF, ImageMetaTIFF::Software,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathTIFF+"/{ushort=306}", 
												bagMetaTIFF, ImageMetaTIFF::DateTime,
												TT_EXIF_DATETIME);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathTIFF+"/{ushort=315}", 
												bagMetaTIFF, ImageMetaTIFF::Artist,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathTIFF+"/{ushort=316}", 
												bagMetaTIFF, ImageMetaTIFF::HostComputer,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathTIFF+"/{ushort=33432}", 
												bagMetaTIFF, ImageMetaTIFF::Copyright,
												TT_LPWSTR);
#if ENABLE_ALL_METAS
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathTIFF+"/{ushort=318}", 
												bagMetaTIFF, ImageMetaTIFF::WhitePoint,
												TT_ARRAY_UI8);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathTIFF+"/{ushort=319}", 
												bagMetaTIFF, ImageMetaTIFF::PrimaryChromaticities,
												TT_ARRAY_UI8);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathTIFF+"/{ushort=301}", 
												bagMetaTIFF, ImageMetaTIFF::TransferFunction,
												TT_ARRAY_UI2);
#endif
		}

		bagMetaTIFF->Release();
	}

	const VValueBag *bagMetaIPTC = inBagMetas ? inBagMetas->RetainUniqueElement(ImageMetaIPTC::IPTC) : NULL;
	if (bagMetaIPTC)
	{
		PROPVARIANT value; 
		PropVariantInit(&value);

		//write IPTC XMP tags (IIMv4 tags in comment or not supported in XMP)
		//@remarks
		//	for XMP tags of type Lang Alt - like 'dc:title',
		//  we write actually the '/x-default' metadata child element
		//  (which should be the XMP 'rdf:Alt/rdf:li' element with attribute xml:lang="x-default")

		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/{ushort=3}", 
		//									bagMetaIPTC, ImageMetaIPTC::ObjectTypeReference);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/iptccore:IntellectualGenre", 
											bagMetaIPTC, ImageMetaIPTC::ObjectAttributeReference);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/dc:title", 
											bagMetaIPTC, ImageMetaIPTC::ObjectName,
											TT_XMP_LANG_ALT,
											ioFactory);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/mediapro:Status", 
											bagMetaIPTC, ImageMetaIPTC::EditStatus);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/{ushort=8}", 
		//									bagMetaIPTC, ImageMetaIPTC::EditorialUpdate);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/photoshop:Urgency", 
											bagMetaIPTC, ImageMetaIPTC::Urgency,
											TT_LPWSTR);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/iptccore:SubjectCode", 
											bagMetaIPTC, ImageMetaIPTC::SubjectReference,
											TT_XMP_BAG,
											ioFactory);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/photoshop:Category", 
											bagMetaIPTC, ImageMetaIPTC::Category,
											TT_LPWSTR);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/photoshop:SupplementalCategory", 
											bagMetaIPTC, ImageMetaIPTC::SupplementalCategory,
											TT_XMP_SEQ,
											ioFactory);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/photoshop:SupplementalCategories", 
											bagMetaIPTC, ImageMetaIPTC::SupplementalCategory,
											TT_XMP_SEQ,
											ioFactory);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/mediapro:Event", 
											bagMetaIPTC, ImageMetaIPTC::FixtureIdentifier);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/dc:subject", 
											bagMetaIPTC, ImageMetaIPTC::Keywords,
											TT_XMP_BAG,
											ioFactory);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/{ushort=26}", 
		//									bagMetaIPTC, ImageMetaIPTC::ContentLocationCode);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/{ushort=27}", 
		//									bagMetaIPTC, ImageMetaIPTC::ContentLocationName);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/{ushort=30}", 
		//									bagMetaIPTC, ImageMetaIPTC::ReleaseDate);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/{ushort=35}", 
		//									bagMetaIPTC, ImageMetaIPTC::ReleaseTime);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/{ushort=37}", 
		//									bagMetaIPTC, ImageMetaIPTC::ExpirationDate);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/{ushort=38}", 
		//									bagMetaIPTC, ImageMetaIPTC::ExpirationTime);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/photoshop:Instructions", 
											bagMetaIPTC, ImageMetaIPTC::SpecialInstructions,
											TT_LPWSTR);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/{ushort=42}", 
		//									bagMetaIPTC, ImageMetaIPTC::ActionAdvised);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/{ushort=45}", 
		//									bagMetaIPTC, ImageMetaIPTC::ReferenceService);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/{ushort=47}", 
		//									bagMetaIPTC, ImageMetaIPTC::ReferenceDate);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/{ushort=50}", 
		//									bagMetaIPTC, ImageMetaIPTC::ReferenceNumber);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/photoshop:DateCreated", 
											bagMetaIPTC, ImageMetaIPTC::DateTimeCreated,
											TT_XMP_DATETIME);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/{ushort=60}", 
		//									bagMetaIPTC, ImageMetaIPTC::TimeCreated);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/{ushort=62}", 
		//									bagMetaIPTC, ImageMetaIPTC::DigitalCreationDate);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/{ushort=63}", 
		//									bagMetaIPTC, ImageMetaIPTC::DigitalCreationTime);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/xmp:creatortool", 
											bagMetaIPTC, ImageMetaIPTC::OriginatingProgram);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/{ushort=70}", 
		//									bagMetaIPTC, ImageMetaIPTC::ProgramVersion);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/{ushort=75}", 
		//									bagMetaIPTC, ImageMetaIPTC::ObjectCycle);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/dc:creator", 
											bagMetaIPTC, ImageMetaIPTC::Byline,
											TT_XMP_SEQ,
											ioFactory);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/photoshop:AuthorsPosition", 
											bagMetaIPTC, ImageMetaIPTC::BylineTitle,
											TT_LPWSTR);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/photoshop:City", 
											bagMetaIPTC, ImageMetaIPTC::City,
											TT_LPWSTR);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/iptccore:Location", 
											bagMetaIPTC, ImageMetaIPTC::SubLocation);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/photoshop:State", 
											bagMetaIPTC, ImageMetaIPTC::ProvinceState,
											TT_LPWSTR);
		result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/iptccore:CountryCode", 
											bagMetaIPTC, ImageMetaIPTC::CountryPrimaryLocationCode);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/photoshop:Country", 
											bagMetaIPTC, ImageMetaIPTC::CountryPrimaryLocationName,
											TT_LPWSTR);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/photoshop:TransmissionReference", 
											bagMetaIPTC, ImageMetaIPTC::OriginalTransmissionReference,
											TT_LPWSTR);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/iptccore:Scene", 
											bagMetaIPTC, ImageMetaIPTC::Scene,
											TT_XMP_BAG,
											ioFactory);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/photoshop:Headline", 
											bagMetaIPTC, ImageMetaIPTC::Headline,
											TT_LPWSTR);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/photoshop:Credit", 
											bagMetaIPTC, ImageMetaIPTC::Credit,
											TT_LPWSTR);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/photoshop:Source", 
											bagMetaIPTC, ImageMetaIPTC::Source,
											TT_LPWSTR);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/dc:rights", 
											bagMetaIPTC, ImageMetaIPTC::CopyrightNotice,
											TT_XMP_LANG_ALT,
											ioFactory);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/{ushort=118}", 
		//									bagMetaIPTC, ImageMetaIPTC::Contact);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/dc:description", 
											bagMetaIPTC, ImageMetaIPTC::CaptionAbstract,
											TT_XMP_LANG_ALT,
											ioFactory);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/photoshop:CaptionWriter", 
											bagMetaIPTC, ImageMetaIPTC::WriterEditor,
											TT_LPWSTR);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/{ushort=130}", 
		//									bagMetaIPTC, ImageMetaIPTC::ImageType);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/{ushort=131}", 
		//									bagMetaIPTC, ImageMetaIPTC::ImageOrientation);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/{ushort=135}", 
		//									bagMetaIPTC, ImageMetaIPTC::LanguageIdentifier);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/MicrosoftPhoto:Rating", 
		//									bagMetaIPTC, ImageMetaIPTC::StarRating,
		//									TT_LPWSTR);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/xmp:Rating", 
											bagMetaIPTC, ImageMetaIPTC::StarRating,
											TT_LPWSTR);
		//Create IPTC metadatas block writer.
		if (inGUIDFormatContainer != GUID_ContainerFormatWmp) //HDPhoto does not support IPTC metadatas block
		{
			//write IPTC IIMv4 metadatas
			VString pathIPTC;
			if (inGUIDFormatContainer == GUID_ContainerFormatJpeg)
			{
				pathIPTC = "/app13/irb/8bimiptc/iptc";
				if (imageSrc.IsValid())
				{
					//JQ 01/12/2010: fixed ACI0069005

					//while encoding with JPEG codec, if image source has a empty IPTC metadata block, 
					//rebuild image dest IPTC block from scratch
					//(otherwise WIC fails to encode any metas & WriteSource returns a nasty stream error)

					CComPtr<IWICBitmapFrameDecode> bmpFrameDecoder;
					if (imageSrc.GetBitmapFrameDecoder( bmpFrameDecoder))
					{
						CComPtr<IWICMetadataQueryReader> metaReader;
						if (SUCCEEDED(bmpFrameDecoder->GetMetadataQueryReader( &metaReader)))
						{			
							PROPVARIANT value;
							PropVariantInit(&value);
							if (SUCCEEDED(metaReader->GetMetadataByName(pathIPTC.GetCPointer(), &value)))
							{
								CComPtr<IWICMetadataQueryReader> metaReaderIPTC;
								if (value.vt == VT_UNKNOWN)
								{
									if (SUCCEEDED(value.punkVal->QueryInterface(IID_IWICMetadataQueryReader, (void **)&metaReaderIPTC)))
									{
										CComPtr<IEnumString> enumerator;
										if (SUCCEEDED(metaReaderIPTC->GetEnumerator( &enumerator)))
										{	
											LPOLESTR nextitem = NULL;
											ULONG count = 0;
											HRESULT hr = enumerator->Next( 1, &nextitem, &count);
											if (FAILED(hr))
												//image source metadata IPTC block is empty:
												//rebuild metadata IPTC block from scratch
												hr = metaWriter->RemoveMetadataByName( pathIPTC.GetCPointer());
										}
									}
								}
							}
							PropVariantClear(&value);
						}
					}
				}
			}
			else if (inGUIDFormatContainer == GUID_ContainerFormatTiff)
				pathIPTC = "/ifd/iptc";
			else
				//unknown format: try with TIFF-compliant format
				pathIPTC = "/ifd/iptc";

#if ENABLE_ALL_METAS
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Object Type Reference}", 
												bagMetaIPTC, ImageMetaIPTC::ObjectTypeReference,
												TT_LPWSTR);
#endif
			//result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Object Attribute Reference}", 
			//									bagMetaIPTC, ImageMetaIPTC::ObjectAttributeReference,
			//									TT_ARRAY_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Object Name}", 
												bagMetaIPTC, ImageMetaIPTC::ObjectName,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Edit Status}", 
												bagMetaIPTC, ImageMetaIPTC::EditStatus,
												TT_LPWSTR);
#if ENABLE_ALL_METAS
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Editorial Update}", 
												bagMetaIPTC, ImageMetaIPTC::EditorialUpdate,
												TT_LPWSTR);
#endif
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Urgency}", 
												bagMetaIPTC, ImageMetaIPTC::Urgency,
												TT_LPWSTR);
			//result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Subject Reference}", 
			//									bagMetaIPTC, ImageMetaIPTC::SubjectReference,
			//									TT_ARRAY_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Category}", 
												bagMetaIPTC, ImageMetaIPTC::Category,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Supplemental Category}", 
												bagMetaIPTC, ImageMetaIPTC::SupplementalCategory,
												TT_ARRAY_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Fixture Identifier}", 
												bagMetaIPTC, ImageMetaIPTC::FixtureIdentifier,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Keywords}", 
												bagMetaIPTC, ImageMetaIPTC::Keywords,
												TT_ARRAY_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Content Location Code}", 
												bagMetaIPTC, ImageMetaIPTC::ContentLocationCode,
												TT_ARRAY_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Content Location Name}", 
												bagMetaIPTC, ImageMetaIPTC::ContentLocationName,
												TT_ARRAY_LPWSTR);

			if (bagMetaIPTC->AttributeExists( ImageMetaIPTC::ReleaseDateTime))
			{
				//convert to IPTC date and time tags
				VString dateIPTC, timeIPTC, datetimeXML;
				const VValueSingle *valueSingle = bagMetaIPTC->GetAttribute( ImageMetaIPTC::ReleaseDateTime);
				valueSingle->GetXMLString( datetimeXML, XSO_Time_Local);
				DateTimeFromXMLToIPTC( datetimeXML, dateIPTC, timeIPTC);

				result = result | _WICKeyValuePairFromVValueSingle(	metaWriter, value, pathIPTC+"/{str=Release Date}", 
													static_cast<const VValueSingle *>(&dateIPTC),
													TT_LPWSTR);
				result = result | _WICKeyValuePairFromVValueSingle(	metaWriter, value, pathIPTC+"/{str=Release Time}", 
													static_cast<const VValueSingle *>(&timeIPTC),
													TT_LPWSTR);
			}

			if (bagMetaIPTC->AttributeExists( ImageMetaIPTC::ExpirationDateTime))
			{
				//convert to IPTC date and time tags
				VString dateIPTC, timeIPTC, datetimeXML;
				const VValueSingle *valueSingle = bagMetaIPTC->GetAttribute( ImageMetaIPTC::ExpirationDateTime);
				valueSingle->GetXMLString( datetimeXML, XSO_Time_Local);
				DateTimeFromXMLToIPTC( datetimeXML, dateIPTC, timeIPTC);

				result = result | _WICKeyValuePairFromVValueSingle(	metaWriter, value, pathIPTC+"/{str=Expiration Date}", 
													static_cast<const VValueSingle *>(&dateIPTC),
													TT_LPWSTR);
				result = result | _WICKeyValuePairFromVValueSingle(	metaWriter, value, pathIPTC+"/{str=Expiration Time}", 
													static_cast<const VValueSingle *>(&timeIPTC),
													TT_LPWSTR);
			}

			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Special Instructions}", 
												bagMetaIPTC, ImageMetaIPTC::SpecialInstructions,
												TT_LPWSTR);
#if ENABLE_ALL_METAS
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Action Advised}", 
												bagMetaIPTC, ImageMetaIPTC::ActionAdvised,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Reference Service}", 
												bagMetaIPTC, ImageMetaIPTC::ReferenceService,
												TT_ARRAY_LPWSTR);

			if (bagMetaIPTC->AttributeExists( ImageMetaIPTC::ReferenceDateTime))
			{
				//convert to IPTC reference date tag
				VString dateIPTC, timeIPTC, datesIPTC, datetimeXML, datetimesXML;

				//get array of XML datetime
				const VValueSingle *valueSingle = bagMetaIPTC->GetAttribute( ImageMetaIPTC::ReferenceDateTime);
				if (valueSingle->GetValueKind() == VK_TIME)
					valueSingle->GetXMLString( datetimesXML, XSO_Time_UTC);
				else
					valueSingle->GetString( datetimesXML);
				VectorOfVString vDateTimeXML;
				datetimesXML.GetSubStrings(";", vDateTimeXML);

				//iterate on XML datetime values
				VectorOfVString::const_iterator it = vDateTimeXML.begin();
				for (;it != vDateTimeXML.end(); it++)
				{
					DateTimeFromXMLToIPTC( *it, dateIPTC, timeIPTC);
					if (datesIPTC.IsEmpty())
						datesIPTC = dateIPTC;
					else
					{
						datesIPTC.AppendUniChar(';');
						datesIPTC.AppendString( dateIPTC);
					}
				}
				//write WIC key-value pair
				result = result | _WICKeyValuePairFromVValueSingle(	metaWriter, value, pathIPTC+"/{str=Reference Date}", 
													static_cast<const VValueSingle *>(&datesIPTC),
													TT_ARRAY_LPWSTR);
			}

			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Reference Number}", 
												bagMetaIPTC, ImageMetaIPTC::ReferenceNumber,
												TT_ARRAY_LPWSTR);
#endif
			if (bagMetaIPTC->AttributeExists( ImageMetaIPTC::DateTimeCreated))
			{
				//convert to IPTC date and time tags
				VString dateIPTC, timeIPTC, datetimeXML;
				const VValueSingle *valueSingle = bagMetaIPTC->GetAttribute( ImageMetaIPTC::DateTimeCreated);
				valueSingle->GetXMLString( datetimeXML, XSO_Time_Local);
				DateTimeFromXMLToIPTC( datetimeXML, dateIPTC, timeIPTC);

				result = result | _WICKeyValuePairFromVValueSingle(	metaWriter, value, pathIPTC+"/{str=Date Created}", 
													static_cast<const VValueSingle *>(&dateIPTC),
													TT_LPWSTR);
				result = result | _WICKeyValuePairFromVValueSingle(	metaWriter, value, pathIPTC+"/{str=Time Created}", 
													static_cast<const VValueSingle *>(&timeIPTC),
													TT_LPWSTR);
			}

			if (bagMetaIPTC->AttributeExists( ImageMetaIPTC::DigitalCreationDateTime))
			{
				//convert to IPTC date and time tags
				VString dateIPTC, timeIPTC, datetimeXML;
				const VValueSingle *valueSingle = bagMetaIPTC->GetAttribute( ImageMetaIPTC::DigitalCreationDateTime);
				valueSingle->GetXMLString( datetimeXML, XSO_Time_Local);
				DateTimeFromXMLToIPTC( datetimeXML, dateIPTC, timeIPTC);

				result = result | _WICKeyValuePairFromVValueSingle(	metaWriter, value, pathIPTC+"/{str=Digital Creation Date}", 
													static_cast<const VValueSingle *>(&dateIPTC),
													TT_LPWSTR);
				result = result | _WICKeyValuePairFromVValueSingle(	metaWriter, value, pathIPTC+"/{str=Digital Creation Time}", 
													static_cast<const VValueSingle *>(&timeIPTC),
													TT_LPWSTR);
			}

			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Originating Program}", 
												bagMetaIPTC, ImageMetaIPTC::OriginatingProgram,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Program Version}", 
												bagMetaIPTC, ImageMetaIPTC::ProgramVersion,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Object Cycle}", 
												bagMetaIPTC, ImageMetaIPTC::ObjectCycle,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=By-line}", 
												bagMetaIPTC, ImageMetaIPTC::Byline,
												TT_ARRAY_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=By-line Title}", 
												bagMetaIPTC, ImageMetaIPTC::BylineTitle,
												TT_ARRAY_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=City}", 
												bagMetaIPTC, ImageMetaIPTC::City,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Sub-location}", 
												bagMetaIPTC, ImageMetaIPTC::SubLocation,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Province/State}", 
												bagMetaIPTC, ImageMetaIPTC::ProvinceState,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Country/Primary Location Code}", 
												bagMetaIPTC, ImageMetaIPTC::CountryPrimaryLocationCode,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Country/Primary Location Name}", 
												bagMetaIPTC, ImageMetaIPTC::CountryPrimaryLocationName,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Original Transmission Reference}", 
												bagMetaIPTC, ImageMetaIPTC::OriginalTransmissionReference,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Headline}", 
												bagMetaIPTC, ImageMetaIPTC::Headline,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Credit}", 
												bagMetaIPTC, ImageMetaIPTC::Credit,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Source}", 
												bagMetaIPTC, ImageMetaIPTC::Source,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Copyright Notice}", 
												bagMetaIPTC, ImageMetaIPTC::CopyrightNotice,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Contact}", 
												bagMetaIPTC, ImageMetaIPTC::Contact,
												TT_ARRAY_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Caption/Abstract}", 
												bagMetaIPTC, ImageMetaIPTC::CaptionAbstract,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Writer/Editor}", 
												bagMetaIPTC, ImageMetaIPTC::WriterEditor,
												TT_ARRAY_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Image Type}", 
												bagMetaIPTC, ImageMetaIPTC::ImageType,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Image Orientation}", 
												bagMetaIPTC, ImageMetaIPTC::ImageOrientation,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathIPTC+"/{str=Language Identifier}", 
												bagMetaIPTC, ImageMetaIPTC::LanguageIdentifier,
												TT_LPWSTR);
			//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathIPTC+"/{str=Star Rating}", 
			//									bagMetaIPTC, ImageMetaIPTC::StarRating);
		}

		bagMetaIPTC->Release();
	}

	//determine EXIF block path
	VString pathEXIF;
	if (inGUIDFormatContainer == GUID_ContainerFormatJpeg)
		pathEXIF = "/app1/ifd/exif";
	else if (inGUIDFormatContainer == GUID_ContainerFormatTiff)
		pathEXIF = "/ifd/exif";
	else if (inGUIDFormatContainer == GUID_ContainerFormatWmp)
		pathEXIF = "/ifd/exif";
	else
		//unknown format: try with TIFF-compliant format
		pathEXIF = "/ifd/exif";

	if (inEncodeImageFrames)
	{
		//here as frame is encoded from gdiplus image and not copied from the data provider,
		//we need to purge some metas

		metaWriter->RemoveMetadataByName(VString(pathXMP+"/exif:CustomRendered").GetCPointer());
		metaWriter->RemoveMetadataByName(VString(pathEXIF+"/{ushort=41985}").GetCPointer());
		
		metaWriter->RemoveMetadataByName(VString(pathXMP+"/exif:GainControl").GetCPointer());
		metaWriter->RemoveMetadataByName(VString(pathEXIF+"/{ushort=41991}").GetCPointer());

		metaWriter->RemoveMetadataByName(VString(pathXMP+"/exif:Contrast").GetCPointer());
		metaWriter->RemoveMetadataByName(VString(pathEXIF+"/{ushort=41992}").GetCPointer());

		metaWriter->RemoveMetadataByName(VString(pathXMP+"/exif:Saturation").GetCPointer());
		metaWriter->RemoveMetadataByName(VString(pathEXIF+"/{ushort=41993}").GetCPointer());

		metaWriter->RemoveMetadataByName(VString(pathXMP+"/exif:Sharpness").GetCPointer());
		metaWriter->RemoveMetadataByName(VString(pathEXIF+"/{ushort=41994}").GetCPointer());

		metaWriter->RemoveMetadataByName(VString(pathXMP+"/exif:CompressedBitsPerPixel").GetCPointer());
		metaWriter->RemoveMetadataByName(VString(pathEXIF+"/{ushort=37122}").GetCPointer());
		
		metaWriter->RemoveMetadataByName(VString(pathXMP+"/exif:ColorSpace").GetCPointer());
		metaWriter->RemoveMetadataByName(VString(pathEXIF+"/{ushort=40961}").GetCPointer());

		//set actual image dimensions
		VLong width((sLONG)inImageWidth);
		VLong height((sLONG)inImageHeight);

		PROPVARIANT value;
		PropVariantInit(&value);

		_WICKeyValuePairFromVValueSingle(	metaWriter, value, pathXMP+"/exif:PixelXDimension", 
											static_cast<const VValueSingle *>(&width),
											TT_UI4);
		_WICKeyValuePairFromVValueSingle(	metaWriter, value, pathXMP+"/exif:PixelYDimension", 
											static_cast<const VValueSingle *>(&height),
											TT_UI4);

		_WICKeyValuePairFromVValueSingle(	metaWriter, value, pathEXIF+"/{ushort=40962}", 
											static_cast<const VValueSingle *>(&width),
											TT_UI4);
		_WICKeyValuePairFromVValueSingle(	metaWriter, value, pathEXIF+"/{ushort=40963}", 
											static_cast<const VValueSingle *>(&height),
											TT_UI4);
	}

	const VValueBag *bagMetaEXIF = inBagMetas ? inBagMetas->RetainUniqueElement(ImageMetaEXIF::EXIF) : NULL;
	if (bagMetaEXIF)
	{
		PROPVARIANT value;
		PropVariantInit(&value);

		//write XMP EXIF metadatas (Exif 2.2)

		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:ExposureTime", 
											bagMetaEXIF, ImageMetaEXIF::ExposureTime,
											TT_UI8);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:FNumber", 
											bagMetaEXIF, ImageMetaEXIF::FNumber,
											TT_UI8);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:ExposureProgram", 
											bagMetaEXIF, ImageMetaEXIF::ExposureProgram,
											TT_UI2);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:SpectralSensitivity", 
											bagMetaEXIF, ImageMetaEXIF::SpectralSensitivity);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:ISOSpeedRatings", 
											bagMetaEXIF, ImageMetaEXIF::ISOSpeedRatings,
											TT_XMP_SEQ_UI2,
											ioFactory);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/exif:OECF", 
		//									bagMetaEXIF, ImageMetaEXIF::OECF);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:ExifVersion", 
											bagMetaEXIF, ImageMetaEXIF::ExifVersion);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:DateTimeOriginal", 
											bagMetaEXIF, ImageMetaEXIF::DateTimeOriginal,
											TT_XMP_DATETIME);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:DateTimeDigitized", 
											bagMetaEXIF, ImageMetaEXIF::DateTimeDigitized,
											TT_XMP_DATETIME);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:ComponentsConfiguration", 
											bagMetaEXIF, ImageMetaEXIF::ComponentsConfiguration,
											TT_XMP_SEQ_UI2,
											ioFactory);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:CompressedBitsPerPixel", 
											bagMetaEXIF, ImageMetaEXIF::CompressedBitsPerPixel,
											TT_UI8);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:ShutterSpeedValue", 
											bagMetaEXIF, ImageMetaEXIF::ShutterSpeedValue,
											TT_I8);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:ApertureValue", 
											bagMetaEXIF, ImageMetaEXIF::ApertureValue,
											TT_UI8);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:BrightnessValue", 
											bagMetaEXIF, ImageMetaEXIF::BrightnessValue,
											TT_I8);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:ExposureCompensation", 
											bagMetaEXIF, ImageMetaEXIF::ExposureBiasValue,
											TT_I8);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:MaxApertureValue", 
											bagMetaEXIF, ImageMetaEXIF::MaxApertureValue,
											TT_UI8);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:SubjectDistance", 
											bagMetaEXIF, ImageMetaEXIF::SubjectDistance,
											TT_UI8);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:MeteringMode", 
											bagMetaEXIF, ImageMetaEXIF::MeteringMode,
											TT_UI2);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:LightSource", 
											bagMetaEXIF, ImageMetaEXIF::LightSource,
											TT_UI2);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:Flash", 
											bagMetaEXIF, ImageMetaEXIF::Flash,
											TT_UI2);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:FocalLength", 
											bagMetaEXIF, ImageMetaEXIF::FocalLength,
											TT_UI8);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:SubjectArea", 
											bagMetaEXIF, ImageMetaEXIF::SubjectArea,
											TT_XMP_SEQ_UI2,
											ioFactory);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/exif:SubsecTime", 
		//									bagMetaEXIF, ImageMetaEXIF::SubsecTime);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/exif:SubsecTimeOriginal", 
		//									bagMetaEXIF, ImageMetaEXIF::SubsecTimeOriginal);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/exif:SubsecTimeDigitized", 
		//									bagMetaEXIF, ImageMetaEXIF::SubsecTimeDigitized);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:FlashPixVersion", 
											bagMetaEXIF, ImageMetaEXIF::FlashPixVersion);
		/* //following tag is set by WIC and should not be modified
		{
			//force sRGB colorspace as image source is Gdiplus image
			VLong sRGB(1);
			result = result | _WICKeyValuePairFromVValueSingle(	metaWriter, value, pathXMP+"/exif:ColorSpace", 
												static_cast<const VValueSingle *>(&sRGB),
												TT_UI2);
		}
		*/
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:RelatedSoundFile", 
											bagMetaEXIF, ImageMetaEXIF::RelatedSoundFile,
											TT_LPWSTR);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:FlashEnergy", 
											bagMetaEXIF, ImageMetaEXIF::FlashEnergy,
											TT_UI8);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/exif:SpatialFrequencyResponse", 
		//									bagMetaEXIF, ImageMetaEXIF::SpatialFrequencyResponse);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:FocalPlaneXResolution", 
											bagMetaEXIF, ImageMetaEXIF::FocalPlaneXResolution,
											TT_UI8);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:FocalPlaneYResolution", 
											bagMetaEXIF, ImageMetaEXIF::FocalPlaneYResolution,
											TT_UI8);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:FocalPlaneResolutionUnit", 
											bagMetaEXIF, ImageMetaEXIF::FocalPlaneResolutionUnit,
											TT_UI2);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:SubjectLocation", 
											bagMetaEXIF, ImageMetaEXIF::SubjectLocation,
											TT_XMP_SEQ_UI2,
											ioFactory);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:ExposureIndex", 
											bagMetaEXIF, ImageMetaEXIF::ExposureIndex,
											TT_UI8);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:SensingMethod", 
											bagMetaEXIF, ImageMetaEXIF::SensingMethod,
											TT_UI2);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:FileSource", 
											bagMetaEXIF, ImageMetaEXIF::FileSource,
											TT_UI1);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:SceneType", 
											bagMetaEXIF, ImageMetaEXIF::SceneType,
											TT_UI1);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/exif:CFAPattern", 
		//									bagMetaEXIF, ImageMetaEXIF::CFAPattern);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:CustomRendered", 
											bagMetaEXIF, ImageMetaEXIF::CustomRendered,
											TT_UI2);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:ExposureMode", 
											bagMetaEXIF, ImageMetaEXIF::ExposureMode,
											TT_UI2);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:WhiteBalance", 
											bagMetaEXIF, ImageMetaEXIF::WhiteBalance,
											TT_UI2);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:DigitalZoomRatio", 
											bagMetaEXIF, ImageMetaEXIF::DigitalZoomRatio,
											TT_UI8);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:FocalLenIn35mmFilm", 
											bagMetaEXIF, ImageMetaEXIF::FocalLenIn35mmFilm,
											TT_UI2);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:SceneCaptureType", 
											bagMetaEXIF, ImageMetaEXIF::SceneCaptureType,
											TT_UI2);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:GainControl", 
											bagMetaEXIF, ImageMetaEXIF::GainControl,
											TT_UI8);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:Contrast", 
											bagMetaEXIF, ImageMetaEXIF::Contrast,
											TT_UI2);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:Saturation", 
											bagMetaEXIF, ImageMetaEXIF::Saturation,
											TT_UI2);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:Sharpness", 
											bagMetaEXIF, ImageMetaEXIF::Sharpness,
											TT_UI2);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/exif:DeviceSettingDescription", 
		//									bagMetaEXIF, ImageMetaEXIF::DeviceSettingDescription);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:SubjectDistRange", 
											bagMetaEXIF, ImageMetaEXIF::SubjectDistRange,
											TT_UI2);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:ImageUniqueID", 
											bagMetaEXIF, ImageMetaEXIF::ImageUniqueID,
											TT_LPWSTR);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/exif:", 
		//									bagMetaEXIF, ImageMetaEXIF::Gamma);

		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:MakerNote", 
											bagMetaEXIF, ImageMetaEXIF::MakerNote,
											TT_XMP_LANG_ALT,
											ioFactory);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:UserComment", 
											bagMetaEXIF, ImageMetaEXIF::UserComment,
											TT_XMP_LANG_ALT,
											ioFactory);

		{
			//write EXIF tags (Exif 2.2)

			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=33434}", 
												bagMetaEXIF, ImageMetaEXIF::ExposureTime,
												TT_UI8);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=33437}", 
												bagMetaEXIF, ImageMetaEXIF::FNumber,
												TT_UI8);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=34850}", 
												bagMetaEXIF, ImageMetaEXIF::ExposureProgram,
												TT_UI2);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=34852}", 
												bagMetaEXIF, ImageMetaEXIF::SpectralSensitivity,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=34855}", 
												bagMetaEXIF, ImageMetaEXIF::ISOSpeedRatings,
												TT_ARRAY_UI2);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=36864}", 
												bagMetaEXIF, ImageMetaEXIF::ExifVersion,
												TT_BLOB_ASCII);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=36867}", 
												bagMetaEXIF, ImageMetaEXIF::DateTimeOriginal,
												TT_EXIF_DATETIME);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=36868}", 
												bagMetaEXIF, ImageMetaEXIF::DateTimeDigitized,
												TT_EXIF_DATETIME);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=37121}", 
												bagMetaEXIF, ImageMetaEXIF::ComponentsConfiguration,
												TT_BLOB);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=37122}", 
												bagMetaEXIF, ImageMetaEXIF::CompressedBitsPerPixel,
												TT_UI8);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=37377}", 
												bagMetaEXIF, ImageMetaEXIF::ShutterSpeedValue,
												TT_I8);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=37378}", 
												bagMetaEXIF, ImageMetaEXIF::ApertureValue,
												TT_UI8);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=37379}", 
												bagMetaEXIF, ImageMetaEXIF::BrightnessValue,
												TT_I8);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=37380}", 
												bagMetaEXIF, ImageMetaEXIF::ExposureBiasValue,
												TT_I8);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=37381}", 
												bagMetaEXIF, ImageMetaEXIF::MaxApertureValue,
												TT_UI8);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=37382}", 
												bagMetaEXIF, ImageMetaEXIF::SubjectDistance,
												TT_UI8);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=37383}", 
												bagMetaEXIF, ImageMetaEXIF::MeteringMode,
												TT_UI2);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=37384}", 
												bagMetaEXIF, ImageMetaEXIF::LightSource,
												TT_UI2);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=37385}", 
												bagMetaEXIF, ImageMetaEXIF::Flash,
												TT_UI2);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=37386}", 
												bagMetaEXIF, ImageMetaEXIF::FocalLength,
												TT_UI8);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=37396}", 
												bagMetaEXIF, ImageMetaEXIF::SubjectArea,
												TT_ARRAY_UI2);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=40960}", 
												bagMetaEXIF, ImageMetaEXIF::FlashPixVersion,
												TT_BLOB_ASCII);
			/* //following tag is set by WIC and should not be modified
			{
				//force sRGB colorspace as image source is Gdiplus image
				VLong sRGB(1);
				result = result | _WICKeyValuePairFromVValueSingle(	metaWriter, value, pathEXIF+"/{ushort=40961}", 
													static_cast<const VValueSingle *>(&sRGB),
													TT_UI2);
			}
			*/
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=40964}", 
												bagMetaEXIF, ImageMetaEXIF::RelatedSoundFile,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=41483}", 
												bagMetaEXIF, ImageMetaEXIF::FlashEnergy,
												TT_UI8);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=41486}", 
												bagMetaEXIF, ImageMetaEXIF::FocalPlaneXResolution,
												TT_UI8);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=41487}", 
												bagMetaEXIF, ImageMetaEXIF::FocalPlaneYResolution,
												TT_UI8);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=41488}", 
												bagMetaEXIF, ImageMetaEXIF::FocalPlaneResolutionUnit,
												TT_UI2);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=41492}", 
												bagMetaEXIF, ImageMetaEXIF::SubjectLocation,
												TT_ARRAY_UI2);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=41493}", 
												bagMetaEXIF, ImageMetaEXIF::ExposureIndex,
												TT_UI8);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=41495}", 
												bagMetaEXIF, ImageMetaEXIF::SensingMethod,
												TT_UI2);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=41728}", 
												bagMetaEXIF, ImageMetaEXIF::FileSource,
												TT_UI1);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=41729}", 
												bagMetaEXIF, ImageMetaEXIF::SceneType,
												TT_UI1);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=41985}", 
												bagMetaEXIF, ImageMetaEXIF::CustomRendered,
												TT_UI2);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=41986}", 
												bagMetaEXIF, ImageMetaEXIF::ExposureMode,
												TT_UI2);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=41987}", 
												bagMetaEXIF, ImageMetaEXIF::WhiteBalance,
												TT_UI2);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=41988}", 
												bagMetaEXIF, ImageMetaEXIF::DigitalZoomRatio,
												TT_UI8);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=41989}", 
												bagMetaEXIF, ImageMetaEXIF::FocalLenIn35mmFilm,
												TT_UI2);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=41990}", 
												bagMetaEXIF, ImageMetaEXIF::SceneCaptureType,
												TT_UI2);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=41991}", 
												bagMetaEXIF, ImageMetaEXIF::GainControl,
												TT_UI8);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=41992}", 
												bagMetaEXIF, ImageMetaEXIF::Contrast,
												TT_UI2);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=41993}", 
												bagMetaEXIF, ImageMetaEXIF::Saturation,
												TT_UI2);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=41994}", 
												bagMetaEXIF, ImageMetaEXIF::Sharpness,
												TT_UI2);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=41996}", 
												bagMetaEXIF, ImageMetaEXIF::SubjectDistRange,
												TT_UI2);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=42016}", 
												bagMetaEXIF, ImageMetaEXIF::ImageUniqueID,
												TT_LPWSTR);
			//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathEXIF+"/{ushort=42016}", 
			//									bagMetaEXIF, ImageMetaEXIF::Gamma);

			//result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=37500}", 
			//									bagMetaEXIF, ImageMetaEXIF::MakerNote,
			//									TT_BLOB);
			//result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=37510}", 
			//									bagMetaEXIF, ImageMetaEXIF::UserComment,
			//									TT_BLOB_EXIF_STRING);

#if ENABLE_ALL_METAS
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=37520}", 
												bagMetaEXIF, ImageMetaEXIF::SubsecTime,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=37521}", 
												bagMetaEXIF, ImageMetaEXIF::SubsecTimeOriginal,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=37522}", 
												bagMetaEXIF, ImageMetaEXIF::SubsecTimeDigitized,
												TT_LPWSTR);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=41484}", 
												bagMetaEXIF, ImageMetaEXIF::SpatialFrequencyResponse,
												TT_BLOB);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=34856}", 
												bagMetaEXIF, ImageMetaEXIF::OECF,
												TT_BLOB);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=41730}", 
												bagMetaEXIF, ImageMetaEXIF::CFAPattern,
												TT_BLOB);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathEXIF+"/{ushort=41995}", 
												bagMetaEXIF, ImageMetaEXIF::DeviceSettingDescription,
												TT_BLOB);
#endif
		}

		bagMetaEXIF->Release();
	}

	const VValueBag *bagMetaGPS = inBagMetas ? inBagMetas->RetainUniqueElement(ImageMetaGPS::GPS) : NULL;
	if (bagMetaGPS)
	{
		PROPVARIANT value;
		PropVariantInit(&value);

		//write XMP GPS tags (Exif 2.2)

		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:GPSVersionID", 
											bagMetaGPS, ImageMetaGPS::VersionID);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/exif:GPSLatitudeRef", 
		//									bagMetaGPS, ImageMetaGPS::LatitudeRef);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:GPSLatitude", 
											bagMetaGPS, ImageMetaGPS::Latitude);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/exif:GPSLongitudeRef", 
		//									bagMetaGPS, ImageMetaGPS::LongitudeRef);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:GPSLongitude", 
											bagMetaGPS, ImageMetaGPS::Longitude);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:GPSAltitudeRef", 
											bagMetaGPS, ImageMetaGPS::AltitudeRef,
											TT_UI1);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:GPSAltitude", 
											bagMetaGPS, ImageMetaGPS::Altitude,
											TT_UI8);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:GPSSatellites", 
											bagMetaGPS, ImageMetaGPS::Satellites);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:GPSStatus", 
											bagMetaGPS, ImageMetaGPS::Status);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:GPSMeasureMode", 
											bagMetaGPS, ImageMetaGPS::MeasureMode);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:GPSDOP", 
											bagMetaGPS, ImageMetaGPS::DOP,
											TT_UI8);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:GPSSpeedRef", 
											bagMetaGPS, ImageMetaGPS::SpeedRef);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:GPSSpeed", 
											bagMetaGPS, ImageMetaGPS::Speed,
											TT_UI8);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:GPSTrackRef", 
											bagMetaGPS, ImageMetaGPS::TrackRef);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:GPSTrack", 
											bagMetaGPS, ImageMetaGPS::Track,
											TT_UI8);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:GPSImgDirectionRef", 
											bagMetaGPS, ImageMetaGPS::ImgDirectionRef);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:GPSImgDirection", 
											bagMetaGPS, ImageMetaGPS::ImgDirection,
											TT_UI8);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:GPSMapDatum", 
											bagMetaGPS, ImageMetaGPS::MapDatum);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/exif:GPSDestLatitudeRef", 
		//									bagMetaGPS, ImageMetaGPS::DestLatitudeRef);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:GPSDestLatitude", 
											bagMetaGPS, ImageMetaGPS::DestLatitude);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/exif:GPSDestLongitudeRef", 
		//									bagMetaGPS, ImageMetaGPS::DestLongitudeRef);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:GPSDestLongitude", 
											bagMetaGPS, ImageMetaGPS::DestLongitude);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:GPSDestBearingRef", 
											bagMetaGPS, ImageMetaGPS::DestBearingRef);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:GPSDestBearing", 
											bagMetaGPS, ImageMetaGPS::DestBearing,
											TT_UI8);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:GPSDestDistanceRef", 
											bagMetaGPS, ImageMetaGPS::DestDistanceRef);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:GPSDestDistance", 
											bagMetaGPS, ImageMetaGPS::DestDistance,
											TT_UI8);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:GPSProcessingMethod", 
											bagMetaGPS, ImageMetaGPS::ProcessingMethod);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:GPSAreaInformation", 
											bagMetaGPS, ImageMetaGPS::AreaInformation);
		//result = result | _WICKeyValuePairFromBagKeyValuePair(	metaWriter, value, pathXMP+"/exif:GPSDateStamp", 
		//									bagMetaGPS, ImageMetaGPS::DateStamp);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:GPSTimeStamp", 
											bagMetaGPS, ImageMetaGPS::DateTime,
											TT_XMP_DATETIME);
		result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathXMP+"/exif:GPSDifferential", 
											bagMetaGPS, ImageMetaGPS::Differential,
											TT_UI2);

		{
			//write GPS tags (Exif 2.2)

			VString pathGPS;
			if (inGUIDFormatContainer == GUID_ContainerFormatJpeg)
				pathGPS = "/app1/ifd/gps";
			else if (inGUIDFormatContainer == GUID_ContainerFormatTiff)
				pathGPS = "/ifd/gps";
			else if (inGUIDFormatContainer == GUID_ContainerFormatWmp)
				pathGPS = "/ifd/gps";
			else
				//unknown format: try with TIFF-compliant format
				pathGPS = "/ifd/gps";

			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathGPS+"/{ushort=0}", 
												bagMetaGPS, ImageMetaGPS::VersionID,
												TT_GPSVERSIONID);
			if (bagMetaGPS->AttributeExists( ImageMetaGPS::Latitude))
			{
				//convert to GPS latitude+latitudeRef 
				VString latitudeXMP;
				bagMetaGPS->GetString( ImageMetaGPS::Latitude, latitudeXMP);
				VString latitude, latitudeRef;
				GPSCoordsFromXMPToExif( latitudeXMP, latitude, latitudeRef);
				result = result | _WICKeyValuePairFromVValueSingle(	metaWriter, value, pathGPS+"/{ushort=1}",
													static_cast<const VValueSingle *>(&latitudeRef),
													TT_LPWSTR);
				result = result | _WICKeyValuePairFromVValueSingle(	metaWriter, value, pathGPS+"/{ushort=2}",
													static_cast<const VValueSingle *>(&latitude),
													TT_ARRAY_UI8);
			}
			if (bagMetaGPS->AttributeExists( ImageMetaGPS::Longitude))
			{
				//convert to GPS longitude+longitudeRef 
				VString longitudeXMP;
				bagMetaGPS->GetString( ImageMetaGPS::Longitude, longitudeXMP);
				VString longitude, longitudeRef;
				GPSCoordsFromXMPToExif( longitudeXMP, longitude, longitudeRef);
				result = result | _WICKeyValuePairFromVValueSingle(	metaWriter, value, pathGPS+"/{ushort=3}",
													static_cast<const VValueSingle *>(&longitudeRef),
													TT_LPWSTR);
				result = result | _WICKeyValuePairFromVValueSingle(	metaWriter, value, pathGPS+"/{ushort=4}",
													static_cast<const VValueSingle *>(&longitude),
													TT_ARRAY_UI8);
			}
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathGPS+"/{ushort=5}", 
												bagMetaGPS, ImageMetaGPS::AltitudeRef,
												TT_UI1);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathGPS+"/{ushort=6}", 
												bagMetaGPS, ImageMetaGPS::Altitude,
												TT_UI8);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathGPS+"/{ushort=8}", 
												bagMetaGPS, ImageMetaGPS::Satellites);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathGPS+"/{ushort=9}", 
												bagMetaGPS, ImageMetaGPS::Status);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathGPS+"/{ushort=10}", 
												bagMetaGPS, ImageMetaGPS::MeasureMode);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathGPS+"/{ushort=11}", 
												bagMetaGPS, ImageMetaGPS::DOP,
												TT_UI8);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathGPS+"/{ushort=12}", 
												bagMetaGPS, ImageMetaGPS::SpeedRef);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathGPS+"/{ushort=13}", 
												bagMetaGPS, ImageMetaGPS::Speed,
												TT_UI8);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathGPS+"/{ushort=14}", 
												bagMetaGPS, ImageMetaGPS::TrackRef);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathGPS+"/{ushort=15}", 
												bagMetaGPS, ImageMetaGPS::Track,
												TT_UI8);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathGPS+"/{ushort=16}", 
												bagMetaGPS, ImageMetaGPS::ImgDirectionRef);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathGPS+"/{ushort=17}", 
												bagMetaGPS, ImageMetaGPS::ImgDirection,
												TT_UI8);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathGPS+"/{ushort=18}", 
												bagMetaGPS, ImageMetaGPS::MapDatum);
			if (bagMetaGPS->AttributeExists( ImageMetaGPS::DestLatitude))
			{
				//convert to GPS latitude+latitudeRef 
				VString latitudeXMP;
				bagMetaGPS->GetString( ImageMetaGPS::DestLatitude, latitudeXMP);
				VString latitude, latitudeRef;
				GPSCoordsFromXMPToExif( latitudeXMP, latitude, latitudeRef);
				result = result | _WICKeyValuePairFromVValueSingle(	metaWriter, value, pathGPS+"/{ushort=19}",
													static_cast<const VValueSingle *>(&latitudeRef),
													TT_LPWSTR);
				result = result | _WICKeyValuePairFromVValueSingle(	metaWriter, value, pathGPS+"/{ushort=20}",
													static_cast<const VValueSingle *>(&latitude),
													TT_ARRAY_UI8);
			}
			if (bagMetaGPS->AttributeExists( ImageMetaGPS::DestLongitude))
			{
				//convert to GPS longitude+longitudeRef 
				VString longitudeXMP;
				bagMetaGPS->GetString( ImageMetaGPS::DestLongitude, longitudeXMP);
				VString longitude, longitudeRef;
				GPSCoordsFromXMPToExif( longitudeXMP, longitude, longitudeRef);
				result = result | _WICKeyValuePairFromVValueSingle(	metaWriter, value, pathGPS+"/{ushort=21}",
													static_cast<const VValueSingle *>(&longitudeRef),
													TT_LPWSTR);
				result = result | _WICKeyValuePairFromVValueSingle(	metaWriter, value, pathGPS+"/{ushort=22}",
													static_cast<const VValueSingle *>(&longitude),
													TT_ARRAY_UI8);
			}
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathGPS+"/{ushort=23}", 
												bagMetaGPS, ImageMetaGPS::DestBearingRef);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathGPS+"/{ushort=24}", 
												bagMetaGPS, ImageMetaGPS::DestBearing,
												TT_UI8);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathGPS+"/{ushort=25}", 
												bagMetaGPS, ImageMetaGPS::DestDistanceRef);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathGPS+"/{ushort=26}", 
												bagMetaGPS, ImageMetaGPS::DestDistance,
												TT_UI8);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathGPS+"/{ushort=27}", 
												bagMetaGPS, ImageMetaGPS::ProcessingMethod,
												TT_BLOB_EXIF_STRING);
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathGPS+"/{ushort=28}", 
												bagMetaGPS, ImageMetaGPS::AreaInformation,
												TT_BLOB_EXIF_STRING);
			if (bagMetaGPS->AttributeExists( ImageMetaGPS::DateTime))
			{
				//convert to GPS date+time

				VString datetimeXML, dateGPS, timeGPS;
				const VValueSingle *valueSingle = bagMetaGPS->GetAttribute( ImageMetaGPS::DateTime);
				if (valueSingle->GetValueKind() == VK_TIME)
					valueSingle->GetXMLString( datetimeXML, XSO_Time_UTC);
				else
					valueSingle->GetString( datetimeXML);

				DateTimeFromXMLToGPS( datetimeXML, dateGPS, timeGPS);

				result = result | _WICKeyValuePairFromVValueSingle(	metaWriter, value, pathGPS+"/{ushort=29}",
													static_cast<const VValueSingle *>(&dateGPS),
													TT_LPWSTR);
				result = result | _WICKeyValuePairFromVValueSingle(	metaWriter, value, pathGPS+"/{ushort=7}",
													static_cast<const VValueSingle *>(&timeGPS),
													TT_ARRAY_UI8);
			}
			result = result | _WICKeyValuePairFromBagKeyValuePair(metaWriter, value, pathGPS+"/{ushort=30}", 
												bagMetaGPS, ImageMetaGPS::Differential,
												TT_UI2);
		}

		bagMetaGPS->Release();
	}

	}
	return result;
}


/** convert from metadata bag key-value pair to WIC metadata key-value pair
@see
	_WICKeyValuePairToBagKeyValuePair
*/
bool stImageSource::_WICKeyValuePairFromBagKeyValuePair(	
											CComPtr<IWICMetadataQueryWriter>& ioMetaWriter,
											PROPVARIANT& ioValue,
											const VString& inWICTagItem,
											const VValueBag *inBagMeta, const XBOX::VValueBag::StKey& inBagKey,
											eTagType inTagType,
											CComPtr<IWICImagingFactory> ioFactory)
{
	const VValueSingle *valueSingle = inBagMeta->GetAttribute( inBagKey);
	if (!valueSingle)
		return false;

	if (inTagType == TT_BLOB)
	{
		//blob (UNDEFINED values)

		if (valueSingle->GetValueKind() == VK_STRING)
		{
			if (static_cast<const VString *>(valueSingle)->IsEmpty())
			{
				//if value is a empty string, remove metadata
				ioMetaWriter->RemoveMetadataByName( inWICTagItem.GetCPointer());
				return true;
			}
		}

		VBlobWithPtr blob;
		if (ImageMeta::stReader::GetBlob( inBagMeta, inBagKey, *(static_cast<VBlob *>(&blob))))
		{
			ioValue.vt = VT_BLOB;
			ioValue.blob.cbSize = blob.GetSize();
			ioValue.blob.pBlobData = (BYTE *)blob.GetDataPtr();

			//set WIC tag value
			bool ok = SUCCEEDED(ioMetaWriter->SetMetadataByName( inWICTagItem.GetCPointer(), &ioValue));
			PropVariantInit(&ioValue); //caller is owner of allocated data so do not call PropVariantClear here
			return ok;
		}
		else
			return false;
	}

	return _WICKeyValuePairFromVValueSingle( ioMetaWriter,
											 ioValue,
											 inWICTagItem,
											 valueSingle,
											 inTagType,
											 ioFactory);
}

/** convert from VValueSingle value to WIC metadata key-value pair
@see
	_WICKeyValuePairToBagKeyValuePair
	stImageSource::eTagType
*/
bool stImageSource::_WICKeyValuePairFromVValueSingle(	
											CComPtr<IWICMetadataQueryWriter>& ioMetaWriter,
											PROPVARIANT& ioValue,
											const VString& inWICTagItem,
											const VValueSingle *inValueSingle,
											eTagType inTagType,
											CComPtr<IWICImagingFactory> ioFactory)
{
	if (inValueSingle->GetValueKind() == VK_STRING)
	{
		if (static_cast<const VString *>(inValueSingle)->IsEmpty())
		{
			//if value is a empty string, remove metadata
			ioMetaWriter->RemoveMetadataByName( inWICTagItem.GetCPointer());
			return true;
		}
	}

	BYTE *valueBlob = NULL;
	VBlobWithPtr valueBlob4D;
	VString valueString;
	VectorOfVString valueStringVector;

	switch (inTagType)
	{
	case TT_BLOB_ASCII:
		//blob with only ASCII characters (not including the NULL terminating character)
		{
			VString text;
			inValueSingle->GetString( text);
			if (text.IsEmpty())
				return false;

			ioValue.vt = VT_BLOB;
			ioValue.blob.cbSize = text.GetLength();
			valueBlob = new BYTE [ioValue.blob.cbSize];
			ioValue.blob.pBlobData = valueBlob;
			text.ToBlock( ioValue.blob.pBlobData, ioValue.blob.cbSize, VTC_Win32Ansi, false, false);
		}
		break;
	case TT_BLOB_EXIF_STRING:	
		//blob containing a Exif-compliant string (8-bytes header for ASCII charset string followed by the text)
		{
			VString text;
			inValueSingle->GetString( text);
			if (text.IsEmpty())
				return false;
			
			ioValue.vt = VT_BLOB;
			ioValue.blob.cbSize = text.GetLength()*sizeof(UniChar)+8;
			valueBlob = new BYTE [ioValue.blob.cbSize];
			ioValue.blob.pBlobData = valueBlob;

			//write ASCII character set  
			memcpy(ioValue.blob.pBlobData, "UNICHAR", 7);
			ioValue.blob.pBlobData[7] = 0;

			//write string with UNICHAR charset
			text.ToBlock( ioValue.blob.pBlobData+8, ioValue.blob.cbSize-8, VTC_UTF_16, false, false);
		}
		break;
	case TT_XMP_LANG_ALT:
		//XMP lang alt element
		{
			inValueSingle->GetString( valueString);
			if (valueString.IsEmpty())
				return false;

			CComPtr<IWICMetadataQueryWriter> metaWriterLangAlt;
			if (SUCCEEDED(ioFactory->CreateQueryWriter(
					GUID_MetadataFormatXMPAlt,
					NULL,
					&metaWriterLangAlt)))
			{
				VString xdefault = "/x-default";
				ioValue.vt = VT_LPWSTR;
				ioValue.pwszVal = (LPWSTR)valueString.GetCPointer();
				bool ok = SUCCEEDED(metaWriterLangAlt->SetMetadataByName( xdefault.GetCPointer(), &ioValue));
				PropVariantInit(&ioValue); //caller is owner of allocated data so do not call PropVariantClear here
				if (ok)
				{
					PROPVARIANT value;
					PropVariantInit(&value);
					value.vt = VT_UNKNOWN;
					value.punkVal = (IWICMetadataQueryWriter *)metaWriterLangAlt;
					value.punkVal->AddRef();

					xbox_assert(sizeof(UniChar) == sizeof(WCHAR));
					ok = SUCCEEDED(ioMetaWriter->SetMetadataByName(inWICTagItem.GetCPointer(), &value));
					PropVariantClear(&value);
				}
				return ok;
			}
			else
				return false;
		}
		break;
	case TT_EXIF_DATETIME:
		{
			xbox_assert(sizeof(UniChar) == sizeof(WCHAR));

			//get XML UTC datetime
			VString datetimeXML;
			if (inValueSingle->GetValueKind() == VK_TIME)
				inValueSingle->GetXMLString( datetimeXML, XSO_Time_UTC);
			else
				inValueSingle->GetString( datetimeXML);

			//convert to Exif datetime
			DateTimeFromXMLToExif( datetimeXML, valueString);
			
			ioValue.vt = VT_LPWSTR;
			ioValue.pwszVal = (LPWSTR)valueString.GetCPointer();
		}
		break;
	case TT_XMP_DATETIME:
		{
			xbox_assert(sizeof(UniChar) == sizeof(WCHAR));

			//get XML UTC datetime
			VString datetimeXML;
			if (inValueSingle->GetValueKind() == VK_TIME)
				inValueSingle->GetXMLString( datetimeXML, XSO_Time_UTC);
			else
				inValueSingle->GetString( datetimeXML);

			//convert to XMP datetime
			DateTimeFromXMLToXMP( datetimeXML, valueString);
			
			ioValue.vt = VT_LPWSTR;
			ioValue.pwszVal = (LPWSTR)valueString.GetCPointer();
		}
		break;
	case TT_LPWSTR:
		{
			inValueSingle->GetString( valueString);
			ioValue.vt = VT_LPWSTR;
			ioValue.pwszVal = (LPWSTR)valueString.GetCPointer();
		}
		break;
	case TT_UI1:
		{
			ioValue.vt = VT_UI1;
			ioValue.bVal = (UCHAR)inValueSingle->GetLong();
		}
		break;
	case TT_I1:
		{
			ioValue.vt = VT_I1;
			ioValue.cVal = (CHAR)inValueSingle->GetLong();
		}
		break;
	case TT_UI2:
		{
			ioValue.vt = VT_UI2;
			ioValue.uiVal = (USHORT)inValueSingle->GetLong();
		}
		break;
	case TT_I2:
		{
			ioValue.vt = VT_I2;
			ioValue.iVal = (SHORT)inValueSingle->GetLong();
		}
		break;
	case TT_UI4:
		{
			ioValue.vt = VT_UI4;
			ioValue.ulVal = (ULONG)inValueSingle->GetLong();
		}
		break;
	case TT_I4:
		{
			ioValue.vt = VT_I4;
			ioValue.lVal = (LONG)inValueSingle->GetLong();
		}
		break;
	case TT_BOOL:
		{
			VARIANT_BOOL valueBool; 
			if (inValueSingle->GetValueKind() == VK_STRING)
			{
				inValueSingle->GetString( valueString);
				valueBool = valueString.EqualToString( "true", false)
							||
							valueString.EqualToString( "1", false) ? -1 : 0; 
			}
			else
				valueBool = inValueSingle->GetBoolean() != 0 ? -1 : 0;
			ioValue.vt = VT_BOOL;
			ioValue.boolVal = valueBool;
		}
		break;
	case TT_UI8:
		{
			//5 digits precision is enough for metadatas
			Real valueReal = fabs(inValueSingle->GetReal());
			ioValue.vt = VT_UI8;
			ioValue.uhVal.LowPart	= (DWORD)(valueReal*100000);
			ioValue.uhVal.HighPart	= 100000;
		}
		break;
	case TT_I8:
		{
			//5 digits precision is enough for metadatas
			Real valueReal = inValueSingle->GetReal();
			ioValue.vt = VT_I8;
			ioValue.hVal.LowPart	= (DWORD)(fabs(valueReal)*100000);
			ioValue.hVal.HighPart	= 100000*(valueReal >= 0.0 ? 1 : -1);
		}
		break;
	case TT_ARRAY_I1:
	case TT_GPSVERSIONID:
		{
			inValueSingle->GetString( valueString);
			if (valueString.IsEmpty())
				return false;

			//decode from list of char string-formatted values
			VectorOfVString vValues;
			valueString.GetSubStrings(inTagType == TT_GPSVERSIONID ? '.' : ';',vValues);
			xbox_assert(vValues.size() > 0);
			
			ioValue.vt = VT_I1|VT_VECTOR;
			ioValue.cac.cElems = vValues.size();
			valueBlob = new BYTE [ioValue.cac.cElems*sizeof(CHAR)];
			ioValue.cac.pElems = (CHAR *)valueBlob;

			CHAR *p = ioValue.cac.pElems;
			VectorOfVString::const_iterator it = vValues.begin();
			for (;it != vValues.end(); it++,p++)
				*p = (CHAR)((*it).GetLong());
		}
		break;
	case TT_ARRAY_UI1:
		{
			inValueSingle->GetString( valueString);
			if (valueString.IsEmpty())
				return false;

			//decode from list of uchar string-formatted values
			VectorOfVString vValues;
			valueString.GetSubStrings(';',vValues);
			xbox_assert(vValues.size() > 0);
			
			ioValue.vt = VT_UI1|VT_VECTOR;
			ioValue.caub.cElems = vValues.size();
			valueBlob = new BYTE [ioValue.caub.cElems*sizeof(UCHAR)];
			ioValue.caub.pElems = (UCHAR *)valueBlob;

			UCHAR *p = ioValue.caub.pElems;
			VectorOfVString::const_iterator it = vValues.begin();
			for (;it != vValues.end(); it++,p++)
				*p = (UCHAR)((*it).GetLong());
		}
		break;
	case TT_ARRAY_I2:
		{
			inValueSingle->GetString( valueString);
			if (valueString.IsEmpty())
				return false;

			//decode from list of short string-formatted values
			VectorOfVString vValues;
			valueString.GetSubStrings(';',vValues);
			xbox_assert(vValues.size() > 0);
			
			ioValue.vt = VT_I2|VT_VECTOR;
			ioValue.cai.cElems = vValues.size();
			valueBlob = new BYTE [ioValue.cai.cElems*sizeof(SHORT)];
			ioValue.cai.pElems = (SHORT *)valueBlob;

			SHORT *p = ioValue.cai.pElems;
			VectorOfVString::const_iterator it = vValues.begin();
			for (;it != vValues.end(); it++,p++)
				*p = (SHORT)((*it).GetLong());
		}
		break;
	case TT_ARRAY_UI2: 
		{
			inValueSingle->GetString( valueString);
			if (valueString.IsEmpty())
				return false;

			//decode from list of ushort string-formatted values
			VectorOfVString vValues;
			valueString.GetSubStrings(';',vValues);
			xbox_assert(vValues.size() > 0);
			
			ioValue.vt = VT_UI2|VT_VECTOR;
			ioValue.caui.cElems = vValues.size();
			valueBlob = new BYTE [ioValue.caui.cElems*sizeof(USHORT)];
			ioValue.caui.pElems = (USHORT *)valueBlob;

			USHORT *p = ioValue.caui.pElems;
			VectorOfVString::const_iterator it = vValues.begin();
			for (;it != vValues.end(); it++,p++)
				*p = (USHORT)((*it).GetLong());
		}
		break;
	case TT_ARRAY_I4:
		{
			inValueSingle->GetString( valueString);
			if (valueString.IsEmpty())
				return false;

			//decode from list of long string-formatted values
			VectorOfVString vValues;
			valueString.GetSubStrings(';',vValues);
			xbox_assert(vValues.size() > 0);
			
			ioValue.vt = VT_I4|VT_VECTOR;
			ioValue.cal.cElems = vValues.size();
			valueBlob = new BYTE [ioValue.cal.cElems*sizeof(LONG)];
			ioValue.cal.pElems = (LONG *)valueBlob;

			LONG *p = ioValue.cal.pElems;
			VectorOfVString::const_iterator it = vValues.begin();
			for (;it != vValues.end(); it++,p++)
				*p = (LONG)((*it).GetLong());
		}
		break;
	case TT_ARRAY_UI4:
		{
			inValueSingle->GetString( valueString);
			if (valueString.IsEmpty())
				return false;

			//decode from list of ulong string-formatted values
			VectorOfVString vValues;
			valueString.GetSubStrings(';',vValues);
			xbox_assert(vValues.size() > 0);
			
			ioValue.vt = VT_UI4|VT_VECTOR;
			ioValue.caul.cElems = vValues.size();
			valueBlob = new BYTE [ioValue.caul.cElems*sizeof(ULONG)];
			ioValue.caul.pElems = (ULONG *)valueBlob;

			ULONG *p = ioValue.caul.pElems;
			VectorOfVString::const_iterator it = vValues.begin();
			for (;it != vValues.end(); it++,p++)
				*p = (ULONG)((*it).GetLong());
		}
		break;
	case TT_ARRAY_BOOL:
		{
			inValueSingle->GetString( valueString);
			if (valueString.IsEmpty())
				return false;

			//decode from list of ulong string-formatted values
			VectorOfVString vValues;
			valueString.GetSubStrings(';',vValues);
			xbox_assert(vValues.size() > 0);
			
			ioValue.vt = VT_BOOL|VT_VECTOR;
			ioValue.cabool.cElems = vValues.size();
			valueBlob = new BYTE [ioValue.cabool.cElems*sizeof(VARIANT_BOOL)];
			ioValue.cabool.pElems = (VARIANT_BOOL *)valueBlob;

			VARIANT_BOOL *p = ioValue.cabool.pElems;
			VectorOfVString::const_iterator it = vValues.begin();
			for (;it != vValues.end(); it++,p++)
			{
				*p = (*it).EqualToString( "true", false)
					 ||
					 (*it).EqualToString( "1", false) ? -1 : 0; 
			}
		}
		break;
	case TT_ARRAY_I8:
		{
			inValueSingle->GetString( valueString);
			if (valueString.IsEmpty())
				return false;

			//decode from list of real string-formatted values
			VectorOfVString vValues;
			valueString.GetSubStrings(';',vValues);
			xbox_assert(vValues.size() > 0);
			
			ioValue.vt = VT_I8|VT_VECTOR;
			ioValue.cah.cElems = vValues.size();
			valueBlob = new BYTE [ioValue.cah.cElems*sizeof(LARGE_INTEGER)];
			ioValue.cah.pElems = (LARGE_INTEGER *)valueBlob;

			LARGE_INTEGER *p = ioValue.cah.pElems;
			VectorOfVString::const_iterator it = vValues.begin();
			for (;it != vValues.end(); it++,p++)
			{
				//5 digits precision is enough for metadatas
				Real valueReal = (*it).GetReal();
				(*p).LowPart	= (DWORD)(fabs(valueReal)*100000);
				(*p).HighPart	= ((LONG)100000)*(valueReal >= 0.0 ? 1 : -1);
			}
		}
		break;
	case TT_ARRAY_UI8:
		{
			inValueSingle->GetString( valueString);
			if (valueString.IsEmpty())
				return false;

			//decode from list of real string-formatted values
			VectorOfVString vValues;
			valueString.GetSubStrings(';',vValues);
			xbox_assert(vValues.size() > 0);
			
			ioValue.vt = VT_UI8|VT_VECTOR;
			ioValue.cauh.cElems = vValues.size();
			valueBlob = new BYTE [ioValue.cauh.cElems*sizeof(ULARGE_INTEGER)];
			ioValue.cauh.pElems = (ULARGE_INTEGER *)valueBlob;

			ULARGE_INTEGER *p = ioValue.cauh.pElems;
			VectorOfVString::const_iterator it = vValues.begin();
			for (;it != vValues.end(); it++,p++)
			{
				//5 digits precision is enough for metadatas
				Real valueReal = fabs((*it).GetReal());
				(*p).LowPart	= (DWORD)(valueReal*100000);
				(*p).HighPart	= (DWORD)100000;
			}
		}
		break;
	case TT_ARRAY_LPWSTR:
		{
			inValueSingle->GetString( valueString);
			if (valueString.IsEmpty())
				return false;

			//decode from list of ulong string-formatted values
			VectorOfVString vValues;
			valueString.GetSubStrings(';',valueStringVector);
			xbox_assert(valueStringVector.size() > 0);
			
			ioValue.vt = VT_LPWSTR|VT_VECTOR;
			ioValue.calpwstr.cElems = valueStringVector.size();
			valueBlob = new BYTE [ioValue.cabool.cElems*sizeof(LPWSTR)];
			ioValue.calpwstr.pElems = (LPWSTR *)valueBlob;

			LPWSTR *p = ioValue.calpwstr.pElems;
			VectorOfVString::const_iterator it = valueStringVector.begin();
			for (;it != valueStringVector.end(); it++,p++)
			{
				*p = (LPWSTR)(*it).GetCPointer();
			}
		}
		break;
	case TT_XMP_BAG:
	case TT_XMP_BAG_UI2:
	case TT_XMP_BAG_UI8:
		{
			inValueSingle->GetString( valueString);
			if (valueString.IsEmpty())
				return false;

			//decode from list of string-formatted values
			VectorOfVString vValues;
			valueString.GetSubStrings(';',vValues);
			xbox_assert(vValues.size() > 0);
			
			CComPtr<IWICMetadataQueryWriter> metaWriterBAG;
			if (SUCCEEDED(ioFactory->CreateQueryWriter(
					GUID_MetadataFormatXMPBag,
					NULL,
					&metaWriterBAG)))
			{
				VectorOfVString::const_iterator it = vValues.begin();
				int index = 0;
				for (;it != vValues.end(); it++, index++)
				{
					switch (inTagType)
					{
					case TT_XMP_BAG_UI2:
						{
						ioValue.vt		= VT_UI2;
						ioValue.uiVal	= (USHORT)(*it).GetLong();
						}
						break;
					case TT_XMP_BAG_UI8:
						{
						Real valueReal = fabs((*it).GetReal());
						ioValue.vt = VT_UI8;
						ioValue.uhVal.LowPart	= (DWORD)(valueReal*100000);
						ioValue.uhVal.HighPart	= 100000;
						}
						break;
					default:
						{
						ioValue.vt		= VT_LPWSTR;
						ioValue.pwszVal	= (LPWSTR)(*it).GetCPointer();
						}
						break;
					}
					//set WIC tag value
					VString keyString;
					keyString.FromLong( index);
					keyString = "/{ulong="+keyString+"}";
					bool ok = SUCCEEDED(metaWriterBAG->SetMetadataByName( keyString.GetCPointer(), 
																		  &ioValue));
					PropVariantInit(&ioValue); //caller is owner of allocated data so do not call PropVariantClear here
					if (!ok)
						return false;
				}

				PROPVARIANT value;
				PropVariantInit(&value);
				value.vt = VT_UNKNOWN;
				value.punkVal = (IWICMetadataQueryWriter *)metaWriterBAG;
				value.punkVal->AddRef();

				xbox_assert(sizeof(UniChar) == sizeof(WCHAR));
				bool ok = SUCCEEDED(ioMetaWriter->SetMetadataByName(inWICTagItem.GetCPointer(), &value));
				PropVariantClear(&value);

				return ok;
			}
			else
				return false;
		}
		break;
	case TT_XMP_SEQ:
	case TT_XMP_SEQ_UI2:
	case TT_XMP_SEQ_UI8:
		{
			inValueSingle->GetString( valueString);
			if (valueString.IsEmpty())
				return false;

			//decode from list of string-formatted values
			VectorOfVString vValues;
			valueString.GetSubStrings(';',vValues);
			xbox_assert(vValues.size() > 0);
			
			CComPtr<IWICMetadataQueryWriter> metaWriterSEQ;
			if (SUCCEEDED(ioFactory->CreateQueryWriter(
					GUID_MetadataFormatXMPSeq,
					NULL,
					&metaWriterSEQ)))
			{
				VectorOfVString::const_iterator it = vValues.begin();
				int index = 0;
				for (;it != vValues.end(); it++, index++)
				{
					switch (inTagType)
					{
					case TT_XMP_SEQ_UI2:
						{
						ioValue.vt		= VT_UI2;
						ioValue.uiVal	= (USHORT)(*it).GetLong();
						}
						break;
					case TT_XMP_SEQ_UI8:
						{
						Real valueReal = fabs((*it).GetReal());
						ioValue.vt = VT_UI8;
						ioValue.uhVal.LowPart	= (DWORD)(valueReal*100000);
						ioValue.uhVal.HighPart	= 100000;
						}
						break;
					default:
						{
						ioValue.vt		= VT_LPWSTR;
						ioValue.pwszVal	= (LPWSTR)(*it).GetCPointer();
						}
						break;
					}
					
					//set WIC tag value
					VString keyString;
					keyString.FromLong( index);
					keyString = "/{ulong="+keyString+"}";
					bool ok = SUCCEEDED(metaWriterSEQ->SetMetadataByName( keyString.GetCPointer(), 
																		  &ioValue));
					PropVariantInit(&ioValue); //caller is owner of allocated data so do not call PropVariantClear here
					if (!ok)
						return false;
				}

				PROPVARIANT value;
				PropVariantInit(&value);
				value.vt = VT_UNKNOWN;
				value.punkVal = (IWICMetadataQueryWriter *)metaWriterSEQ;
				value.punkVal->AddRef();

				xbox_assert(sizeof(UniChar) == sizeof(WCHAR));
				bool ok = SUCCEEDED(ioMetaWriter->SetMetadataByName(inWICTagItem.GetCPointer(), &value));
				PropVariantClear(&value);

				return ok;
			}
			else
				return false;
		}
		break;
	default:
		return false;
		break;
	}

	//set WIC tag value
	HRESULT hr = ioMetaWriter->SetMetadataByName( inWICTagItem.GetCPointer(), &ioValue);
	bool ok = SUCCEEDED(hr);
	PropVariantInit(&ioValue); //caller is owner of allocated data so do not call PropVariantClear here
	if (valueBlob)
		delete [] valueBlob;
	return ok;
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////


VWICCodecInfo::VWICCodecInfo(const VWICCodecInfo& inRef):VObject()
{
	fUTI			= inRef.fUTI;
	fGUID			= inRef.fGUID;
	fDisplayName	= inRef.fDisplayName;
	fCanEncode		= inRef.fCanEncode;
	fMimeTypeList	= inRef.fMimeTypeList;
	fExtensionList	= inRef.fExtensionList;
}

VWICCodecInfo::VWICCodecInfo (  const VString& inUTI, 
								const GUID& inGUID,
								const VString& inDisplayName,
								const std::vector<VString>& inMimeTypeList,
								const std::vector<VString>& inExtensionList,
								bool inCanEncode)
{
	fUTI			= inUTI;
	fGUID			= inGUID;
	fDisplayName	= inDisplayName;
	fCanEncode		= inCanEncode;
	fMimeTypeList	= inMimeTypeList;
	fExtensionList	= inExtensionList;
}



uBOOL VWICCodecInfo::FindExtension(const VString& inExt) const
{
	for( std::vector<VString>::const_iterator i = fExtensionList.begin() ; i != fExtensionList.end() ; ++i)
	{
		if(*i == inExt)
			return true;
	}
	return false;
}


uBOOL VWICCodecInfo::FindMimeType(const VString& inMime) const
{
	for( std::vector<VString>::const_iterator i = fMimeTypeList.begin() ; i != fMimeTypeList.end() ; ++i)
	{
		if (*i == inMime)
			return true;
	}
	return false;
}


VWICCodecInfo::~VWICCodecInfo()
{
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////


/** append WIC codecs to list of codecs */
void VPictureCodec_WIC::AppendWICCodecs( VectorOfPictureCodec& outCodecs,  VPictureCodecFactory* inFactory)
{
	//VTaskLock lock(&sLock);

	//retain factory instance (use com smart pointer for auto-release)
	CComPtr<IWICImagingFactory> factory;
	HRESULT result = factory.CoCreateInstance(CLSID_WICImagingFactory);
	if (FAILED(result))
		//failed: probably WIC component is not installed
		//(WIC requires Windows XP SP3 - or SP2 with Microsoft .NET 3.0 - or Vista)
		return;
	
	//browse encoders and decoders
    CComPtr<IEnumUnknown> e;
	for (int numComponent = 0; numComponent < 2; numComponent++)
	{
		//first pass: add codecs which can encode 
		//second pass: add codecs which can only decode
		bool enumEncoder = numComponent == 0;

		result = factory->CreateComponentEnumerator(enumEncoder ? WICEncoder : WICDecoder, WICComponentEnumerateRefresh, &e);
		if (SUCCEEDED(result))
		{
			ULONG num = 0;
			IUnknown* unknown;

			while ((S_OK == e->Next(1, &unknown, &num)) 
					&& 
					(1 == num))
			{
				CComQIPtr<IWICBitmapCodecInfo> codecInfo = unknown;
				
				//browse file extensions
				std::vector<VString> vFileExtensions;
				UINT strLen = 0;
				if (SUCCEEDED(codecInfo->GetFileExtensions(0,0,&strLen)))
				{
					wchar_t* pbuf = new wchar_t[strLen];
					codecInfo->GetFileExtensions(strLen, pbuf, &strLen);
					
					xbox::VString fileExtensions;
	#if WCHAR_IS_UNICHAR
					fileExtensions.FromUniCString( (const UniChar *)pbuf);
	#else
					fileExtensions.FromUniCString( pbuf);
	#endif
					fileExtensions.GetSubStrings( ',',vFileExtensions);
					
					std::vector<VString>::iterator it = vFileExtensions.begin();
					for (;it != vFileExtensions.end(); it++)
					{
						if (!(*it).IsEmpty())
							if ((*it).GetUniChar(1) == '.')
								(*it).Remove(1,1);
					}
					delete [] pbuf;
				}

				//check if a existing codec uses same extension 
				bool extensionFound = false;
				std::vector<VString>::iterator it = vFileExtensions.begin();
				for (;it != vFileExtensions.end(); it++)
				{
					extensionFound = inFactory->_ExtensionExist( *it);
					if (extensionFound)
						break;
				}
				if (!extensionFound && vFileExtensions.size())
				{
					//get display name
					VString name;
					/*
					//@remarks
					//	 obsolete (Windows OS only): display name is set by UpdateDisplayNameWithSystemInformation()

					UINT strLen = 0;
					if(SUCCEEDED(codecInfo->GetFriendlyName(0,0,&strLen)))
					{
						wchar_t* pbuf = new wchar_t[strLen];
						codecInfo->GetFriendlyName(strLen, pbuf, &strLen);
		#if WCHAR_IS_UNICHAR
						name.FromUniCString( (const UniChar *)pbuf);
		#else
						name.FromUniCString( pbuf);
		#endif
						delete [] pbuf;
					}
					*/

					//browse mime types
					std::vector<VString> vMimeTypes;
					strLen = 0;
					if (SUCCEEDED(codecInfo->GetMimeTypes(0,0,&strLen)))
					{
						wchar_t* pbuf = new wchar_t[strLen];
						codecInfo->GetMimeTypes(strLen, pbuf, &strLen);
						
						xbox::VString mimeTypes;
		#if WCHAR_IS_UNICHAR
						mimeTypes.FromUniCString( (const UniChar *)pbuf);
		#else
						mimeTypes.FromUniCString( pbuf);
		#endif
						mimeTypes.GetSubStrings( ',',vMimeTypes);
						
						delete [] pbuf;
					}


					//get image container format GUID
					GUID guidContainerFormat = { 0 };
					codecInfo->GetContainerFormat(&guidContainerFormat);
					
					//determine UTI
					VString uti;
					if (guidContainerFormat == GUID_ContainerFormatBmp)
						uti = "com.microsoft.bmp";
					else if (guidContainerFormat == GUID_ContainerFormatPng)
						uti = "public.png";
					else if (guidContainerFormat == GUID_ContainerFormatIco)
						uti = "com.microsoft.ico";
					else if (guidContainerFormat == GUID_ContainerFormatJpeg)
						uti = "public.jpeg";
					else if (guidContainerFormat == GUID_ContainerFormatTiff)
						uti = "public.tiff";
					else if (guidContainerFormat == GUID_ContainerFormatGif)
						uti = "com.compuserve.gif";
					else if (guidContainerFormat == GUID_ContainerFormatWmp)
						uti = "com.microsoft.wdp";

					//build codec info 
					VWICCodecInfo info( uti, guidContainerFormat, name, vMimeTypes, vFileExtensions, enumEncoder);

					//append WIC codec to the list of codecs
					outCodecs.push_back(static_cast<VPictureCodec *>(new VPictureCodec_WIC(info)));
				}
			}
		}
		e.Release();
	}


	//initialize metadatas static datas
	ImageMeta::stReader::InitializeGlobals();
}


VPictureCodec_WIC::VPictureCodec_WIC(const VWICCodecInfo& inWICCodecInfo)
:VPictureCodec()
{
	fUTI				= inWICCodecInfo.GetUTI();
	fGUID				= inWICCodecInfo.GetGUID();

	/* 
	//@remarks
	//	 obsolete (Windows OS only): display name is set by UpdateDisplayNameWithSystemInformation()

	if (!inWICCodecInfo.GetDisplayName().IsEmpty())
		SetDisplayName( inWICCodecInfo.GetDisplayName());
	else 
	{	
		//extract display name from first extension:
		
		if (inWICCodecInfo.CountExtension())
		{
			VString name;
			inWICCodecInfo.GetNthExtension(0, name);

			//set first char to uppercase
			UniChar firstChar = name.GetUniChar(1);
			if (firstChar >= 'a' && firstChar <= 'z')
			{
				firstChar -= 'a'-'A';
				name.SetUniChar(1, firstChar);
			}

			SetDisplayName(name);
		}
	}
	*/
	
	SetPlatform(2); 
	SetBuiltIn(false);
	
	for(long nbmime=0;nbmime < inWICCodecInfo.CountMimeType();nbmime++)
	{
		VString mime;
		inWICCodecInfo.GetNthMimeType(nbmime,mime);
		AppendMimeType(mime);
	}
	for(long nbext=0;nbext < inWICCodecInfo.CountExtension();nbext++)
	{
		VString ext;
		inWICCodecInfo.GetNthExtension(nbext,ext);
		AppendExtension(ext);
	}
	
	SetEncode(inWICCodecInfo.CanEncode());
	SetCanValidateData(true); 
}

/** return true if WIC pixel format has alpha component */
bool VPictureCodec_WIC::HasPixelFormatAlpha( const GUID& inWICPixelFormat)
{
	return stImageSource::HasPixelFormatAlpha( inWICPixelFormat);
}


/** convert WIC pixel format to gdiplus pixel format
@remarks
	return PixelFormatUndefined if WIC pixel format is not supported by gdiplus
*/
Gdiplus::PixelFormat VPictureCodec_WIC::FromWICPixelFormatGUID( const GUID& inPF)
{
	//caution:
	//	WIC naming convention: least significant byte first
	//	gdiplus naming convention: highest significant byte first (so WIC reverse) 
	//  (for instance if pixel byte stream order is b, g, r, a
	//   it is BGRA for WIC and ARGB for gdiplus)

	if (inPF == GUID_WICPixelFormat1bppIndexed) 
		return PixelFormat1bppIndexed;
	if (inPF == GUID_WICPixelFormat4bppIndexed) 
		return PixelFormat4bppIndexed;
	if (inPF == GUID_WICPixelFormat8bppIndexed) 
		return PixelFormat8bppIndexed;
	if (inPF == GUID_WICPixelFormat16bppGray) 
		return PixelFormat16bppGrayScale;
	if (inPF == GUID_WICPixelFormat16bppBGR555) 
		return PixelFormat16bppRGB555; 
	if (inPF == GUID_WICPixelFormat16bppBGR565) 
		return PixelFormat16bppRGB565; 
	if (inPF == GUID_WICPixelFormat24bppBGR) 
		return PixelFormat24bppRGB;
	if (inPF == GUID_WICPixelFormat32bppBGR) 
		return PixelFormat32bppRGB;
	if (inPF == GUID_WICPixelFormat32bppBGRA) 
		return PixelFormat32bppARGB; 
	if (inPF == GUID_WICPixelFormat32bppPBGRA) 
		return PixelFormat32bppPARGB; 
	
	//TODO: check following formats if we need to reverse R and B channels
	if (inPF == GUID_WICPixelFormat48bppRGB) 
		return PixelFormat48bppRGB; 
	if (inPF == GUID_WICPixelFormat64bppRGBA) 
		return PixelFormat64bppARGB; 
	if (inPF == GUID_WICPixelFormat64bppPRGBA) 
		return PixelFormat64bppPARGB; 

	if (inPF == GUID_WICPixelFormat32bppCMYK) 
		return PixelFormat32bppCMYK; 
	return PixelFormatUndefined;
}

/** convert gdiplus pixel format to WIC pixel format
@remarks
	return GUID_WICPixelFormatUndefined if gdiplus pixel format is not supported by WIC
*/
const GUID& VPictureCodec_WIC::FromGdiplusPixelFormat( Gdiplus::PixelFormat inPF)
{
	//caution:
	//	WIC naming convention: least significant byte first
	//	gdiplus naming convention: highest significant byte first (so WIC reverse) 
	//  (for instance if pixel byte stream order is b, g, r, a
	//   it is BGRA for WIC and ARGB for gdiplus)

	if (inPF == PixelFormat1bppIndexed) 
		return GUID_WICPixelFormat1bppIndexed;
	if (inPF == PixelFormat4bppIndexed) 
		return GUID_WICPixelFormat4bppIndexed;
	if (inPF == PixelFormat8bppIndexed) 
		return GUID_WICPixelFormat8bppIndexed;
	if (inPF == PixelFormat16bppGrayScale) 
		return GUID_WICPixelFormat16bppGray;
	if (inPF == PixelFormat16bppRGB555) 
		return GUID_WICPixelFormat16bppBGR555; 
	if (inPF == PixelFormat16bppRGB565) 
		return GUID_WICPixelFormat16bppBGR565; 
	if (inPF == PixelFormat24bppRGB) 
		return GUID_WICPixelFormat24bppBGR;  
	if (inPF == PixelFormat32bppRGB) 
		return GUID_WICPixelFormat32bppBGR;  
	if (inPF == PixelFormat32bppARGB) 
		return GUID_WICPixelFormat32bppBGRA; 
	if (inPF == PixelFormat32bppPARGB) 
		return GUID_WICPixelFormat32bppPBGRA;  
	
	//TODO: check following formats if we need to reverse R and B channels
	if (inPF == PixelFormat48bppRGB) 
		return GUID_WICPixelFormat48bppRGB; 
	if (inPF == PixelFormat64bppARGB) 
		return GUID_WICPixelFormat64bppRGBA; 
	if (inPF == PixelFormat64bppPARGB) 
		return GUID_WICPixelFormat64bppPRGBA; 

	if (inPF == PixelFormat32bppCMYK) 
		return GUID_WICPixelFormat32bppCMYK; 

	return GUID_WICPixelFormatUndefined;
}



VPictureData* VPictureCodec_WIC::_CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder) const
{
	return static_cast<VPictureData *>(new VPictureData_GDIPlus(&inDataProvider, inRecorder));
}

bool VPictureCodec_WIC::ValidateData(VFile& inFile)const
{
	if (!inFile.Exists())
		return false;
	
	VPictureDataProvider *dataProvider = VPictureDataProvider::Create( inFile);
	if (dataProvider)
	{
		bool ok = ValidateData(*dataProvider);
		dataProvider->Release();
		return ok;
	}
	return false;
}
bool VPictureCodec_WIC::ValidateData(VPictureDataProvider& inDataProvider)const
{
	//retain factory instance (use com smart pointer for auto-release)
	CComPtr<IWICImagingFactory> factory;
	HRESULT result = factory.CoCreateInstance(CLSID_WICImagingFactory);
	if (FAILED(result))
		//failed: probably WIC component is not installed
		//(WIC requires Windows XP SP3 - or SP2 with Microsoft .NET 3.0 - or Vista)
		return false;

	//create decoder instance from decoder container format GUID
	CComPtr<IWICBitmapDecoder> decoder;
	if (!SUCCEEDED(factory->CreateDecoder( fGUID, NULL, &decoder)))
		return false;

	//get decoder info
	CComPtr<IWICBitmapDecoderInfo> decoderInfo;
	if (!SUCCEEDED(decoder->GetDecoderInfo( &decoderInfo)))
		return false;

	//create stream from data provider
	VPictureDataProvider_Stream	*stream = new VPictureDataProvider_Stream(&inDataProvider);
	if (!stream)
		return false;

	
	//check if stream match decoder pattern(s)
	BOOL match = 0;
	if (!SUCCEEDED(decoderInfo->MatchesPattern( stream, &match)))
		match = 0;

	if(match!=0)
	{
		// certain codec (.cr2) ont une pattern qui valide tout les type de data, car il ny a pas de header qui permette une validation
		// afin de ne pas valider des données incorrect, on check QueryCapability.
		
		// pp ne pas appeler QueryCapability sur le coded ico, bug microsoft, le codec ne release pas la stream
		if(!::IsEqualGUID(fGUID,GUID_ContainerFormatIco))
		{
			HRESULT hr;
			DWORD capabilities;
			hr = decoder->QueryCapability(stream, &capabilities);
			match = SUCCEEDED(hr);
		}
	}

	stream->Release();
	return match != 0;
}

VError VPictureCodec_WIC::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet)const
{
	VError result=VE_UNKNOWN_ERROR;
	result = _Encode( static_cast<const VPictureCodec *>(this), inData, inSettings, outBlob, inSet);
	return result;
}


VError VPictureCodec_WIC::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet)const
{
	VError result=VE_OK;
	VPictureDrawSettings set(inSet);
	VBlobWithPtr blob;
	result = DoEncode(inData, inSettings, blob, inSet);
	if(result==VE_OK)
	{
		result=CreateFileWithBlob(blob,inFile);
	}
	return result;
}


/** encode input picture data with the specified codec */
VError VPictureCodec_WIC::_Encode(	 const VPictureCodec *inPictureCodec,
									 const VPictureData& inData,
									 const VValueBag* inSettings,
									 VBlob& outBlob,
									 VPictureDrawSettings* inSet
									 )
{ 
	//TEST METADATAS 
	/*
	VValueBag* inSettings = NULL;
	if (_inSettings)
		inSettings = _inSettings->Clone();
	else
		inSettings = new VValueBag();
	if (inSettings)
		if (::GetAsyncKeyState(VK_LSHIFT) & 0x8000)
		{
			VValueBag *metas = new VValueBag();
			{
				ImageMeta::stWriter metaWriter( metas);
				metaWriter.SetIPTCObjectName( "testObjectName");
				metaWriter.SetEXIFFNumber( 10.5);
				metaWriter.SetTIFFImageDescription( "TIFFImageDescription");
			}
			{
				ImageEncoding::stWriter writer(inSettings);
				VValueBag *bagMetas = writer.CreateOrRetainMetadatas( metas);
				if (bagMetas)
					bagMetas->Release();
			}
			metas->Release();
		}
	*/

	xbox_assert(inPictureCodec);

	VError result = VE_OK;

	//determine if we need to re-encode VPictureData image frames or not
	//
	//we can copy directly image frames from VPictureData if:
	//- image source format is equal to image dest format 
	//and
	//- image has no picture transform
	//and
	//- image encoding quality is 1 (or if property is not present)
	//(otherwise we need to re-encode frames)
	bool bEncodeImageFrames = true;
	bool bFormatEqual = false;
	{
		VPictureCodecFactoryRef pictureCodecFactory;
		const VPictureCodec *decoder = pictureCodecFactory->RetainDecoderByIdentifier( inData.GetEncoderID());
		if (decoder)
		{
			bFormatEqual = IsEqualGUID(decoder->GetGUID(), inPictureCodec->GetGUID()) != 0;
			decoder->Release();
		}
	}
	if (bFormatEqual && inData.GetDataProvider()) 
	{
		if ((!inSet) || inSet->IsIdentityMatrix())
		{
			if (inSettings)
			{
				if (inSettings->AttributeExists(ImageEncoding::ImageQuality))
				{
					ImageEncoding::stReader reader(inSettings);
					bEncodeImageFrames = reader.GetImageQuality() < 1.0;
				}
				else
					//remark: if ImageQuality attribute is not present, we assume that frame does not need to be re-encoded
					bEncodeImageFrames = false;
			}
			else
				//remark: if ImageQuality attribute is not present, we assume that frame does not need to be re-encoded
				bEncodeImageFrames = false;
		}
	}

	//composite image on opaque background if codec cannot encode alpha
	VPictureDrawSettings set(inSet);
	bool isTransparent = set.GetDrawingMode() != 3;

	//create Gdiplus bitmap
	Gdiplus::Bitmap *src = bEncodeImageFrames ? inData.CreateGDIPlus_Bitmap(&set) : NULL;
	if (bEncodeImageFrames && src == NULL)
		return VE_UNKNOWN_ERROR;

	//retain factory instance (use com smart pointer for auto-release)
	CComPtr<IWICImagingFactory> factory;
	if (SUCCEEDED(factory.CoCreateInstance(CLSID_WICImagingFactory)))
	{	
		//determinate frame size, resolution and pixel format
		UINT width, height;
		double dpiX, dpiY;
		WICPixelFormatGUID pixelFormat;
		stImageSource imageSrc( factory, inData.GetDataProvider(), inPictureCodec->GetGUID());

		{ //ensure IWICBitmapSource instance will be released before imageSrc
		//create bitmap source 
		CComPtr<IWICBitmapSource> bmpSource;

		if (bEncodeImageFrames)
		{
			//create bitmap source from gdiplus bitmap

			bmpSource = static_cast<IWICBitmapSource *>(new VWICBitmapSource_Gdiplus( src));
			//JQ 26/07/2010: fixed leak memory (here ref count if 2 because of CCompPtr assign+new VWICBitmapSource_Gdiplus)
			((IWICBitmapSource *)bmpSource)->Release();
		}
		else
		{
			//create bitmap source from data provider
			if (!imageSrc.IsValid())
				//data source is not a valid image source
				return VE_UNKNOWN_ERROR;
			if (!imageSrc.GetBitmapSource( bmpSource))
				return VE_UNKNOWN_ERROR;
		}
		if (bmpSource == (IWICBitmapSource *)NULL)
		{
			if (src) delete src;
			return VE_UNKNOWN_ERROR;
		}

		bmpSource->GetSize( &width, &height);
		bmpSource->GetResolution( &dpiX, &dpiY);
		bmpSource->GetPixelFormat(&pixelFormat);
		if (pixelFormat == GUID_WICPixelFormatUndefined)
		{
			//encoder cannot encode with unknown format
			if (src) delete src;
			return VE_UNIMPLEMENTED;
		}


		//create encoder
		CComPtr<IWICBitmapEncoder> encoder;
		if (!SUCCEEDED(factory->CreateEncoder(
							inPictureCodec->GetGUID(),
							0, // vendor
							&encoder)))
		{
			if (src) delete src;
			return VE_UNKNOWN_ERROR;
		}
		//create encoder output stream
		CComPtr<IStream> stream;
		if (!SUCCEEDED(::CreateStreamOnHGlobal(NULL,true,&stream)))
		{
			if (src) delete src;
			return VE_UNKNOWN_ERROR;
		}

		//tell encoder to use the stream
		if (!SUCCEEDED(encoder->Initialize(
						stream,
						WICBitmapEncoderNoCache)))
		{
			if (src) delete src;
			return VE_UNKNOWN_ERROR;
		}

		//create frame encoder 
		CComPtr<IWICBitmapFrameEncode> frame;
		CComPtr<IPropertyBag2> properties;

		if (!SUCCEEDED(encoder->CreateNewFrame(
						&frame,
						bEncodeImageFrames ? &properties : NULL)))
		{
			if (src) delete src;
			return VE_UNKNOWN_ERROR;
		}

		if (bEncodeImageFrames)
		{
			//get encoding properties
			Real imageQuality;
			Real imageCompression;
			{
				ImageEncoding::stReader reader( inSettings);
				
				imageQuality		= reader.GetImageQuality();
				imageCompression	= reader.GetImageCompression();
			}
			
			PROPBAG2 name = { 0 };
			name.dwType = PROPBAG2_TYPE_DATA;
			CComVariant value;

			if (inPictureCodec->GetGUID() != GUID_ContainerFormatTiff)
			{
				//set image quality (0 : worst quality, 1: best quality)
				name.vt = VT_R4;
				name.pstrName = L"ImageQuality";
				
				value = (FLOAT)imageQuality;

				properties->Write(
							1, // property count
							&name,
							&value);
			}

			//set image compression (0 : worst/fastest compression, 1: best/slowest compression)
			name.vt = VT_R4;
			name.pstrName = L"CompressionQuality";

			value = (FLOAT)imageCompression;

			properties->Write(
						1, // property count
						&name,
						&value);
		}

		/*
		if (inPictureCodec->GetGUID() == GUID_ContainerFormatTiff)
	    {
			//auto-determine suitable algorithm
			name.vt = VT_UI1;
			name.pstrName = L"TiffCompressionMethod";
			value = (BYTE)WICTiffCompressionLZW;
			properties->Write(
						1, // property count
						&name,
						&value);
		}
		*/

		if (!SUCCEEDED(frame->Initialize( bEncodeImageFrames ? properties : NULL)))
		{
			if (src) delete src;
			return VE_UNKNOWN_ERROR;
		}

		//set frame size
		frame->SetSize(width, height);

		//set frame resolution
		frame->SetResolution( dpiX, dpiY);

		//set frame pixel format: try to use gdiplus bitmap pixel format
		//(if failed, encoder will choose automatically a compatible pixel format
		// and WriteSource will make gracefully pixel format conversion
		// from gdiplus pixel format to codec supported pixel format)
		if (inPictureCodec->GetGUID() == GUID_ContainerFormatBmp) //WIC use RGBA even for BMP but BMP cannot encode alpha
			pixelFormat = GUID_WICPixelFormat32bppBGR;
		frame->SetPixelFormat(&pixelFormat);

		if (bEncodeImageFrames && isTransparent && !stImageSource::HasPixelFormatAlpha( pixelFormat))
		{
			//image source is transparent but destination pixel format cannot encode alpha:
			//we need to composite image source on default background

			Gdiplus::Bitmap *bmpOpaque = new Gdiplus::Bitmap(	src->GetWidth(), 
																src->GetHeight(), 
																PixelFormat32bppRGB);
			Gdiplus::Graphics *gc = new Gdiplus::Graphics( bmpOpaque);
			xbox_assert(gc);

			gc->SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
			Gdiplus::Color colorBackground(	ENCODING_BACKGROUND_COLOR.GetRed(),
											ENCODING_BACKGROUND_COLOR.GetGreen(),
											ENCODING_BACKGROUND_COLOR.GetBlue(),
											ENCODING_BACKGROUND_COLOR.GetAlpha());
			gc->Clear(colorBackground);
			gc->SetCompositingMode(Gdiplus::CompositingModeSourceOver);

			//ensure we disable composition, smoothing mode & interpolation
			//as we do not want image to be modified but just composed as it is
			//(otherwise for instance antialiased text would be antialiased again)
			gc->SetCompositingQuality( Gdiplus::CompositingQualityHighSpeed);
			gc->SetSmoothingMode( Gdiplus::SmoothingModeNone);
			gc->SetInterpolationMode( Gdiplus::InterpolationModeNearestNeighbor);

			gc->DrawImage( src, 0, 0);
			delete gc;

			//reset WIC bitmap source
			delete src;
			src = bmpOpaque;

			bmpSource = static_cast<IWICBitmapSource *>(new VWICBitmapSource_Gdiplus( src));
			//JQ 26/07/2010: fixed leak memory (here ref count if 2 because of CCompPtr assign+new VWICBitmapSource_Gdiplus)
			((IWICBitmapSource *)bmpSource)->Release();
			bmpSource->GetPixelFormat(&pixelFormat);
			if (pixelFormat == GUID_WICPixelFormatUndefined)
			{
				//encoder cannot encode gdiplus bitmap if gdiplus bitmap use unknown format
				delete src;
				return VE_UNIMPLEMENTED;
			}
			frame->SetPixelFormat(&pixelFormat);
		}
		
		//eventually encode metadatas
		//(copy modified metadatas stored in inSettings ImageMeta::Metadatas element
		// - so caller must have stored the bag of modified metadatas in inSettings - 
		// and/or copy the unmodified metadatas)
		{
			ImageEncoding::stReader reader( inSettings);
			if (reader.WriteMetadatas())
			{	
				//if source image has same GUID as dest image:
				//copy unmodified image source metadatas to dest image 
				//to ensure to preserve source image metadatas
				//(which is not possible when converting from one format to another one)
				VPictureDataProvider *imageSrcDataProvider = bFormatEqual ? inData.GetDataProvider() : NULL;

				const VValueBag *metas = NULL;
				if (!bFormatEqual)
				{
					//dest GUID != src GUID => decode metadatas from source image and re-encode to dest image  
					metas = inData.RetainMetadatas(); //decode source image metas if not done yet
					if (metas)
					{
						//encode metadatas to dest image
						stImageSource::WriteMetadatas(factory, frame, metas, inPictureCodec->GetGUID(),
													  width, height,
													  dpiX,
													  dpiY,
													  NULL,
													  false);
						metas->Release();
					}
				}

				metas = reader.RetainMetadatas();
				if (metas || imageSrcDataProvider || bEncodeImageFrames)
				{
					//encode modified metadatas 
					if (!stImageSource::WriteMetadatas(	factory, frame, metas, inPictureCodec->GetGUID(),
													width, height,
													dpiX,
													dpiY,
													imageSrcDataProvider, //if imageSrcDataProvider is not NULL, dest GUID == src GUID
													bEncodeImageFrames))  //so copy unmodified metadatas from source
						//return error only if modified metadatas could not be set 
						//(because if we fall here, we come from 4D command SET PICTURE METADATA
						// and we must return a error to caller in order var OK is properly set to 0
						// - but we do not throw a error cf spec)
					{
						if (metas)
							metas->Release();
						if (src) delete src;
						return VE_INVALID_PARAMETER;
					}
				}
				if (metas)
					metas->Release();
			}
		}
		
		//copy pixels
		if (bEncodeImageFrames)
		{
			WICRect bounds;
			bounds.X = 0;
			bounds.Y = 0;
			bounds.Width = width;
			bounds.Height = height;
			HRESULT hr = frame->WriteSource(bmpSource, &bounds);
			if (FAILED(hr))
			{
				if (src) delete src;
				return VE_UNKNOWN_ERROR;
			}
		}
		else
		{
			HRESULT hr = frame->WriteSource(bmpSource, NULL);
			if (FAILED(hr))
			{
				return VE_UNKNOWN_ERROR;
			}
		}
		
		//finalize frame and container
		if (!SUCCEEDED(frame->Commit()))
		{
			if (src) delete src;
			return VE_UNKNOWN_ERROR;
		}
		if (!SUCCEEDED(encoder->Commit()))
		{
			if (src) delete src;
			return VE_UNKNOWN_ERROR;
		}

		//copy stream to output blob
		HGLOBAL hStream;
		GetHGlobalFromStream(stream, &hStream);
		const void* pData=(const void*)::GlobalLock(hStream);
		size_t outSize = ::GlobalSize(hStream);
		result = outBlob.PutData(pData, outSize, 0);
		::GlobalUnlock(hStream);

		} //release imageSrc
	}
	else
		result=VE_UNKNOWN_ERROR;
	if (src) delete src;
	return result;
}

/** create a Gdiplus bitmap from a WIC bitmap source */
Gdiplus::Bitmap *VPictureCodec_WIC::FromWIC( IWICBitmapSource *inWICBmpSrc, Gdiplus::PixelFormat *inPixelFormat)
{
	//retain factory instance (use com smart pointer for auto-release)
	CComPtr<IWICImagingFactory> factory;
	HRESULT result = factory.CoCreateInstance(CLSID_WICImagingFactory);
	if (FAILED(result))
		//failed: probably WIC component is not installed
		//(WIC requires Windows XP SP3 - or SP2 with Microsoft .NET 3.0 - or Vista)
		return NULL;

	CComPtr<IWICBitmapSource> bmpSrc = inWICBmpSrc;
	stImageSource imageSrc( factory, bmpSrc);

	if (!imageSrc.IsValid())
		//data source is not a valid image source
		return NULL;

	//convert pixel format to desired pixel format
	Gdiplus::PixelFormat pfSource = imageSrc.GetPixelFormat();
	Gdiplus::PixelFormat pf = inPixelFormat ? *inPixelFormat : pfSource;
	if (pf == PixelFormatUndefined)
		//if dest pixel format is not supported by gdiplus, convert to supported pixel format
		pf = PixelFormat32bppARGB;
	
	if(pfSource==PixelFormat32bppCMYK) // pp bug gdi+/wic avec certaine jpeg en CMYK : on converti en 32rgb
	{
		imageSrc.ConvertToPixelFormat( PixelFormat32bppRGB);
		pfSource = PixelFormat32bppRGB;
		if(!inPixelFormat)
			pf = pfSource;
	}
	
	if (pf != pfSource)
		imageSrc.ConvertToPixelFormat( pf);

	//fixed ACI0076335: ensure color space is sRGB
	imageSrc.ConvertToRGBColorSpace();

	//create gdiplus bitmap 
	INT width, height;
	width = imageSrc.GetWidth();
	height = imageSrc.GetHeight();
	Gdiplus::Bitmap *bmp = new Gdiplus::Bitmap( width, height, pf);
	
	//copy image source palette if appropriate
	if (imageSrc.hasPalette())
		imageSrc.CopyPalette( bmp);

	//copy image source pixels to gdiplus bitmap pixels
	Gdiplus::BitmapData bmData;
	Gdiplus::Rect bounds(0,0,width,height);
	bmp->LockBits(&bounds,Gdiplus::ImageLockModeWrite,pf,&bmData);
	xbox_assert(bmData.Width == width);
	xbox_assert(bmData.Height == height);
	xbox_assert(bmData.PixelFormat == pf);
	
	if (imageSrc.CopyPixels( bmData.Scan0, (uLONG)(bmData.Stride*bmData.Height), (uLONG)bmData.Stride))
	{
		bmp->UnlockBits(&bmData);
		return bmp;
	}
	else
	{
		bmp->UnlockBits(&bmData);
		delete bmp;
		return NULL;
	}
}


/** create gdiplus bitmap from input data provider 
	using decoder with the specified GUID format container
@remarks
	if guid is GUID_NULL, use first decoder which can decode data source
	if outMetadatas is not NULL, read metatadatas in outMetadatas
@see
	kImageMetadata
*/
Gdiplus::Bitmap *VPictureCodec_WIC::Decode( VPictureDataProvider& inDataProvider, 
											const GUID& inGUIDFormatContainer, 
											VValueBag *outMetadatas, 
											Gdiplus::PixelFormat *inPixelFormat) 
{
	if (inGUIDFormatContainer == GUID_ContainerFormatGif)
		return NULL; //use gdiplus decoder for Gif

	//VTaskLock lock(&sLock);

	//retain factory instance (use com smart pointer for auto-release)
	CComPtr<IWICImagingFactory> factory;
	HRESULT result = factory.CoCreateInstance(CLSID_WICImagingFactory);
	if (FAILED(result))
		//failed: probably WIC component is not installed
		//(WIC requires Windows XP SP3 - or SP2 with Microsoft .NET 3.0 - or Vista)
		return NULL;

	//get image source
	stImageSource imageSrc( factory, &inDataProvider, inGUIDFormatContainer);
	if (!imageSrc.IsValid())
		//data source is not a valid image source
		return NULL;
	if (imageSrc.GetContainerFormat() == GUID_ContainerFormatGif)
		return NULL; //use gdiplus decoder for Gif

	//convert pixel format to desired pixel format
	Gdiplus::PixelFormat pfSource = imageSrc.GetPixelFormat();
	Gdiplus::PixelFormat pf = inPixelFormat ? *inPixelFormat : pfSource;
	if (pf == PixelFormatUndefined)
		//if dest pixel format is not supported by gdiplus, convert to supported pixel format
		pf = PixelFormat32bppARGB;
	
	if(pfSource==PixelFormat32bppCMYK || pfSource==PixelFormat16bppGrayScale) // pp bug gdi+/wic avec certaine jpeg en CMYK ou niveau de gris : on converti en 32rgb
	{
		imageSrc.ConvertToPixelFormat( PixelFormat32bppRGB);
		pfSource = PixelFormat32bppRGB;
		if(!inPixelFormat)
			pf = pfSource;
	}
	
	if (pf != pfSource)
		imageSrc.ConvertToPixelFormat( pf);

	//fixed ACI0076335: ensure color space is sRGB
	imageSrc.ConvertToRGBColorSpace();

	//create gdiplus bitmap 
	INT width, height;
	width = imageSrc.GetWidth();
	height = imageSrc.GetHeight();
	Gdiplus::Bitmap *bmp = new Gdiplus::Bitmap( width, height, pf);
	
	//copy image source palette if appropriate
	if (imageSrc.hasPalette())
		imageSrc.CopyPalette( bmp);

	//copy image source pixels to gdiplus bitmap pixels
	Gdiplus::BitmapData bmData;
	Gdiplus::Rect bounds(0,0,width,height);
	bmp->LockBits(&bounds,Gdiplus::ImageLockModeWrite,pf,&bmData);
	xbox_assert(bmData.Width == width);
	xbox_assert(bmData.Height == height);
	xbox_assert(bmData.PixelFormat == pf);
	
	if (imageSrc.CopyPixels( bmData.Scan0, (uLONG)(bmData.Stride*bmData.Height), (uLONG)bmData.Stride))
	{
		bmp->UnlockBits(&bmData);
		return bmp;
	}
	else
	{
		bmp->UnlockBits(&bmData);
		delete bmp;
		return NULL;
	}
}

/** create gdiplus bitmap from input resource
	using decoder with the specified GUID format container
@remarks
	if guid is GUID_NULL, use first decoder which can decode data source
	if outMetadatas is not NULL, read metatadatas in outMetadatas
@see
	kImageMetadata
*/
Gdiplus::Bitmap *VPictureCodec_WIC::Decode( HRSRC hResource, 
											const GUID& inGUIDFormatContainer, 
											VValueBag *outMetadatas, 
											Gdiplus::PixelFormat *inPixelFormat) 
{
	//load resource
	HGLOBAL handle = ::LoadResource(NULL, hResource);
	if (!handle)
		return NULL;

	LPVOID data = ::LockResource( handle);
	if (!data) 
		return NULL;

	DWORD size = ::SizeofResource(NULL, hResource);
	if (!size)
		return NULL;

	//create data provider
	VPictureDataProvider *dataProvider = VPictureDataProvider::Create((VPtr)data, FALSE, size);
	if (!dataProvider)
		return NULL;

	Gdiplus::Bitmap *bmp = Decode( *dataProvider, inGUIDFormatContainer, outMetadatas, inPixelFormat);
	dataProvider->Release();
	return bmp;
}

/** create gdiplus bitmap from input file */
Gdiplus::Bitmap *VPictureCodec_WIC::Decode( const VFile& inFile, VValueBag *outMetadatas, Gdiplus::PixelFormat *inPixelFormat)
{
	if (!inFile.Exists())
		return NULL;
	
	VPictureDataProvider *dataProvider = VPictureDataProvider::Create( inFile);
	if (dataProvider)
	{
		Gdiplus::Bitmap *bmp = Decode(*dataProvider, outMetadatas, inPixelFormat);
		dataProvider->Release();
		return bmp;
	}
	return NULL;
}

#if ENABLE_D2D
/** create ID2D1Bitmap from input data provider 
	using decoder with the specified GUID format container
@remarks
	pixel format is GUID_WICPixelFormat32bppPBGRA (the only one supported by ID2D1Bitmap)

	if guid is GUID_NULL, use first decoder which can decode data source
	otherwise use decoder associated with image container guid
	if outMetadatas is not NULL, read metatadatas in outMetadatas
@see
	kImageMetadata
*/
ID2D1Bitmap* VPictureCodec_WIC::DecodeD2D( ID2D1RenderTarget* inRT, VPictureDataProvider& inDataProvider, const GUID& inGUIDFormatContainer, VValueBag *outMetadatas)
{
	xbox_assert(inRT);
	//VTaskLock lock(&sLock);

	//retain factory instance (use com smart pointer for auto-release)
	CComPtr<IWICImagingFactory> factory;
	HRESULT result = factory.CoCreateInstance(CLSID_WICImagingFactory);
	if (FAILED(result))
		//failed: probably WIC component is not installed
		//(WIC requires Windows XP SP3 - or SP2 with Microsoft .NET 3.0 - or Vista)
		return NULL;

	//get image source
	stImageSource imageSrc( factory, &inDataProvider, inGUIDFormatContainer);
	if (!imageSrc.IsValid())
		//data source is not a valid image source
		return NULL;

	//convert pixel format to desired pixel format
	WICPixelFormatGUID pfSource;
	imageSrc.GetPixelFormat( pfSource);
	//ensure we use 32bpp PBGRA as dest format (needed for ID2D1Bitmap)
	WICPixelFormatGUID pf = GUID_WICPixelFormat32bppPBGRA;
	if (pf != pfSource)
		imageSrc.ConvertToPixelFormat( pf);

	//create D2D Bitmap from WIC bitmap source
	CComPtr<ID2D1RenderTarget> rt;
	rt = (ID2D1RenderTarget *)inRT;
	ID2D1Bitmap *bitmap = NULL;
	CComPtr<IWICBitmapSource> bmpSource;
	imageSrc.GetBitmapSource( bmpSource);
	if (SUCCEEDED(rt->CreateBitmapFromWicBitmap(
			bmpSource,
			&bitmap)))
		return bitmap;
	else
		return NULL;
}

/** create D2D-compatible IWICBitmap from input data provider (device independent)
	using decoder with the specified GUID format container
@remarks
	pixel format is GUID_WICPixelFormat32bppPBGRA (the only one supported by ID2D1Bitmap)

	if guid is GUID_NULL, use first decoder which can decode data source
	otherwise use decoder associated with image container guid
	if outMetadatas is not NULL, read metatadatas in outMetadatas
@see
	kImageMetadata
*/
IWICBitmap* VPictureCodec_WIC::DecodeWIC( VPictureDataProvider& inDataProvider, const GUID& inGUIDFormatContainer, VValueBag *outMetadatas)
{
	//VTaskLock lock(&sLock);

	//retain factory instance (use com smart pointer for auto-release)
	CComPtr<IWICImagingFactory> factory;
	HRESULT result = factory.CoCreateInstance(CLSID_WICImagingFactory);
	if (FAILED(result))
		//failed: probably WIC component is not installed
		//(WIC requires Windows XP SP3 - or SP2 with Microsoft .NET 3.0 - or Vista)
		return NULL;

	//get image source
	stImageSource imageSrc( factory, &inDataProvider, inGUIDFormatContainer);
	if (!imageSrc.IsValid())
		//data source is not a valid image source
		return NULL;

	//convert pixel format to desired pixel format
	WICPixelFormatGUID pfSource;
	imageSrc.GetPixelFormat( pfSource);
	//ensure we use 32bpp PBGRA as dest format (needed for ID2D1Bitmap)
	WICPixelFormatGUID pf = GUID_WICPixelFormat32bppPBGRA;
	if (pf != pfSource)
		imageSrc.ConvertToPixelFormat( pf);

	//create WIC Bitmap from WIC bitmap source
	IWICBitmap *bitmap = NULL;
	CComPtr<IWICBitmapSource> bmpSource;
	imageSrc.GetBitmapSource( bmpSource);
	if (SUCCEEDED(factory->CreateBitmapFromSource(
			bmpSource,
			WICBitmapCacheOnDemand,
			&bitmap)))
		return bitmap;
	else
		return NULL;
}


/** create ID2D1Bitmap from resource 
	using decoder with the specified GUID format container
@remarks
	pixel format is GUID_WICPixelFormat32bppPBGRA (the only one supported by ID2D1Bitmap)

	if guid is GUID_NULL, use first decoder which can decode data source
	otherwise use decoder associated with image container guid
	if outMetadatas is not NULL, read metatadatas in outMetadatas
@see
	kImageMetadata
*/
ID2D1Bitmap* VPictureCodec_WIC::DecodeD2D( ID2D1RenderTarget* inRT, HRSRC hResource, const GUID& inGUIDFormatContainer, VValueBag *outMetadatas)
{
	//load resource
	HGLOBAL handle = ::LoadResource(NULL, hResource);
	if (!handle)
		return NULL;

	LPVOID data = ::LockResource( handle);
	if (!data) 
		return NULL;

	DWORD size = ::SizeofResource(NULL, hResource);
	if (!size)
		return NULL;

	//create data provider
	VPictureDataProvider *dataProvider = VPictureDataProvider::Create((VPtr)data, FALSE, size);
	if (!dataProvider)
		return NULL;

	ID2D1Bitmap* bitmap = DecodeD2D( inRT, *dataProvider, inGUIDFormatContainer, outMetadatas);
	dataProvider->Release();
	return bitmap;
}

/** create D2D-compatible IWICBitmap from resource (device independent)
	using decoder with the specified GUID format container
@remarks
	pixel format is GUID_WICPixelFormat32bppPBGRA (in order to be compatible with D2D)

	if guid is GUID_NULL, use first decoder which can decode data source
	otherwise use decoder associated with image container guid
	if outMetadatas is not NULL, read metatadatas in outMetadatas
@see
	kImageMetadata
*/
IWICBitmap* VPictureCodec_WIC::DecodeWIC( HRSRC hResource, const GUID& inGUIDFormatContainer, VValueBag *outMetadatas)
{
	//load resource
	HGLOBAL handle = ::LoadResource(NULL, hResource);
	if (!handle)
		return NULL;

	LPVOID data = ::LockResource( handle);
	if (!data) 
		return NULL;

	DWORD size = ::SizeofResource(NULL, hResource);
	if (!size)
		return NULL;

	//create data provider
	VPictureDataProvider *dataProvider = VPictureDataProvider::Create((VPtr)data, FALSE, size);
	if (!dataProvider)
		return NULL;

	IWICBitmap* bitmap = DecodeWIC( *dataProvider, inGUIDFormatContainer, outMetadatas);
	dataProvider->Release();
	return bitmap;
}

/** create ID2D1Bitmap from Gdiplus Bitmap 
@remarks
	dest pixel format is GUID_WICPixelFormat32bppPBGRA (the only one supported by ID2D1Bitmap)
*/
ID2D1Bitmap* VPictureCodec_WIC::DecodeD2D( ID2D1RenderTarget* inRT, Gdiplus::Bitmap *inBmpSource)
{
	//retain factory instance (use com smart pointer for auto-release)
	CComPtr<IWICImagingFactory> factory;
	HRESULT result = factory.CoCreateInstance(CLSID_WICImagingFactory);
	if (FAILED(result))
		//failed: probably WIC component is not installed
		//(WIC requires Windows XP SP3 - or SP2 with Microsoft .NET 3.0 - or Vista)
		return NULL;

	//create WIC bitmap source from Gdiplus Bitmap
	CComPtr<IWICBitmapSource> bmpSource;
	bmpSource = static_cast<IWICBitmapSource *>(new VWICBitmapSource_Gdiplus( inBmpSource));
	xbox_assert(bmpSource);
	//prevent leak memory: here ref count if 2 because of CCompPtr assign+new VWICBitmapSource_Gdiplus)
	((IWICBitmapSource *)bmpSource)->Release();
	stImageSource imageSrc(factory, bmpSource);

	//convert pixel format to the desired pixel format
	WICPixelFormatGUID pfSource;
	imageSrc.GetPixelFormat( pfSource);
	//ensure we use 32bpp PBGRA as dest format (needed for ID2D1Bitmap)
	WICPixelFormatGUID pf = GUID_WICPixelFormat32bppPBGRA;
	if (pf != pfSource)
		imageSrc.ConvertToPixelFormat( pf);

	//create D2D Bitmap from WIC bitmap source
	CComPtr<ID2D1RenderTarget> rt;
	rt = (ID2D1RenderTarget *)inRT;
	ID2D1Bitmap *bitmap = NULL;
	bmpSource = NULL;
	imageSrc.GetBitmapSource( bmpSource);
	if (SUCCEEDED(rt->CreateBitmapFromWicBitmap(
			bmpSource,
			&bitmap)))
		return bitmap;
	else
		return NULL;
}

/** create IWICBitmap from Gdiplus Bitmap
@remarks
	dest format inherits gdiplus format if inWICPixelFormat is not specified
*/
IWICBitmap* VPictureCodec_WIC::DecodeWIC( Gdiplus::Bitmap *inBmpSource, const VRect *inBounds, const GUID *inWICPixelFormat)
{
	//retain factory instance (use com smart pointer for auto-release)
	CComPtr<IWICImagingFactory> factory;
	HRESULT result = factory.CoCreateInstance(CLSID_WICImagingFactory);
	if (FAILED(result))
		//failed: probably WIC component is not installed
		//(WIC requires Windows XP SP3 - or SP2 with Microsoft .NET 3.0 - or Vista)
		return NULL;

	//create WIC bitmap source from Gdiplus Bitmap
	CComPtr<IWICBitmapSource> bmpSource;
	bmpSource = static_cast<IWICBitmapSource *>(new VWICBitmapSource_Gdiplus( inBmpSource));
	xbox_assert(bmpSource);
	//prevent leak memory: here ref count if 2 because of CCompPtr assign+new VWICBitmapSource_Gdiplus)
	((IWICBitmapSource *)bmpSource)->Release();

	if (inWICPixelFormat)
	{
		//convert pixel format to the desired pixel format
		stImageSource imageSrc(factory, bmpSource);
		
		WICPixelFormatGUID pfSource;
		imageSrc.GetPixelFormat( pfSource);
		if (!imageSrc.IsValid() || pfSource == GUID_WICPixelFormatUndefined)
		{
			//unknown format: return NULL
			xbox_assert(false);
			return NULL;
		}

		if (*inWICPixelFormat != pfSource)
		{
			imageSrc.ConvertToPixelFormat( *inWICPixelFormat);
			imageSrc.GetBitmapSource( bmpSource);
		}
	}

	IWICBitmap *bmp = NULL;
	HRESULT hr;
	if (inBounds)
	{
		VRect bounds(*inBounds);
		bounds.NormalizeToInt();
		hr = factory->CreateBitmapFromSourceRect(bmpSource, bounds.GetX(), bounds.GetY(), bounds.GetWidth(), bounds.GetHeight(), &bmp);
	}
	else
		hr = factory->CreateBitmapFromSource(bmpSource, WICBitmapCacheOnDemand, &bmp);
	xbox_assert(SUCCEEDED(hr));
	return bmp;
}
#endif


/** read metadatas from input data provider
@remarks
	if guid is GUID_NULL, use first decoder which can decode data source
	otherwise use decoder associated with image container guid
@see
	kImageMetadata
*/
void VPictureCodec_WIC::GetMetadatas( VPictureDataProvider& inDataProvider, const GUID& inGUIDFormatContainer, VValueBag *outMetadatas)
{
	xbox_assert(outMetadatas);

	//retain factory instance (use com smart pointer for auto-release)
	CComPtr<IWICImagingFactory> factory;
	HRESULT result = factory.CoCreateInstance(CLSID_WICImagingFactory);
	if (FAILED(result))
		//failed: probably WIC component is not installed
		//(WIC requires Windows XP SP3 - or SP2 with Microsoft .NET 3.0 - or Vista)
		return;

	//get image source
	stImageSource imageSrc( factory, &inDataProvider, inGUIDFormatContainer);
	if (!imageSrc.IsValid())
		//data source is not a valid image source
		return;

	imageSrc.ReadMetadatas( outMetadatas);
}


/** return true if codec can encode metadatas */
bool VPictureCodec_WIC::CanEncodeMetas() const	
{ 
	return	fGUID != GUID_NULL
			&&
			fGUID != GUID_ContainerFormatBmp
			&&
			fGUID != GUID_ContainerFormatIco
			&&
			fGUID != GUID_ContainerFormatGif
			&&
			fGUID != GUID_ContainerFormatPng;
}

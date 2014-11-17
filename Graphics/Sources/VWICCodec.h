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
#ifndef __VWICCodec__
#define __VWICCodec__

BEGIN_TOOLBOX_NAMESPACE


/** WIC codec info class */
class XTOOLBOX_API VWICCodecInfo : public VObject 
{
public:
	VWICCodecInfo ( const VString& inUTI, 
					const GUID& inGUID,
					const VString& inDisplayName,
					const std::vector<VString>& inMimeTypeList,
					const std::vector<VString>& inExtensionList,
					bool inCanEncode = true);
	VWICCodecInfo (const VWICCodecInfo& inRef);
	virtual	~VWICCodecInfo ();
	
	/** get accessor on codec UTI */ 
	const VString&	GetUTI () const { return fUTI; };

	/** get accessor on codec container format GUID */ 
	const GUID&	GetGUID () const { return fGUID; };

	/** get accessor on codec display name */ 
	const VString& GetDisplayName () const { return fDisplayName; };

	/** return true if codec can encode */
	bool CanEncode() const { return fCanEncode; }
	
	uBOOL	FindMimeType(const VString& inMime) const;
	uBOOL	FindExtension(const VString& inExt) const;
	
	sLONG CountMimeType() const										{return (sLONG)fMimeTypeList.size();};
	sLONG CountExtension() const									{return (sLONG)fExtensionList.size();};
	
	void GetNthMimeType(sLONG inIndex,VString& outMime)	const		{outMime		=fMimeTypeList[inIndex];};
	void GetNthExtension(sLONG inIndex,VString& outExtension) const	{outExtension	=fExtensionList[inIndex];};
	
protected:
	/** WIC codec UTI */
	VString fUTI;
	
	/** WIC codec container format GUID */
	GUID fGUID;

	/** display name */
	VString fDisplayName;

	/** true if codec can encode */
	bool fCanEncode;
	
	/** codec mime type list */
	std::vector<VString> fMimeTypeList;
	
	/** codec extension type list */
	std::vector<VString> fExtensionList;
};



/** WIC codec class */
class XTOOLBOX_API VPictureCodec_WIC : public VPictureCodec
{
public:
	VPictureCodec_WIC( const VWICCodecInfo& inWICCodecInfo);
	virtual	~VPictureCodec_WIC() {}
	
	virtual bool ValidateData(VFile& inFile)const;
	virtual bool ValidateData(VPictureDataProvider& inDataProvider)const;
	
	virtual VPictureData* _CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder=0) const;

	/** append WIC codecs to list of codecs */
	static void AppendWICCodecs( VectorOfPictureCodec& outCodecs, VPictureCodecFactory* inFactory);

	/** encode input picture data with the specified codec 
	@remarks
		if inSettings is not NULL,
		encoding properties are stored in inSettings attributes
		and optionally metadatas in first element with name ImageMeta::Metadatas (value is a VValueBag)
	@see
		kImageMetadata
	*/
	static VError _Encode(const VPictureCodec *inPictureCodec,
						 const VPictureData& inData,
						 const VValueBag* inSettings,
						 VBlob& outBlob,
						 VPictureDrawSettings* inSet = NULL);
	
	/** return true if codec can encode metadatas */
	bool CanEncodeMetas() const;		


	/** create gdiplus bitmap from input data provider 
	@remarks
		if outMetadatas is not NULL, read metatadatas in outMetadatas
	@see
		kImageMetadata
	*/
	static Gdiplus::Bitmap *Decode( VPictureDataProvider& inDataProvider, VValueBag *outMetadatas = NULL, Gdiplus::PixelFormat *inPixelFormat = NULL)
	{
		return Decode( inDataProvider, GUID_NULL, outMetadatas, inPixelFormat);
	}

#if ENABLE_D2D
	/** create ID2D1Bitmap from data provider (device dependent)
	@remarks
		pixel format is GUID_WICPixelFormat32bppPBGRA (the only one supported by ID2D1Bitmap)

		if outMetadatas is not NULL, read metatadatas in outMetadatas
	@see
		kImageMetadata
	*/
	static ID2D1Bitmap* DecodeD2D( ID2D1RenderTarget* inRT, VPictureDataProvider& inDataProvider, VValueBag *outMetadatas = NULL)
	{
		return DecodeD2D( inRT, inDataProvider, GUID_NULL, outMetadatas);
	}

	/** create D2D-compatible IWICBitmap from data provider (device independent)
	@remarks
		pixel format is GUID_WICPixelFormat32bppPBGRA (the only one supported by D2D)

		if outMetadatas is not NULL, read metatadatas in outMetadatas
	@see
		kImageMetadata
	*/
	static IWICBitmap* DecodeWIC( VPictureDataProvider& inDataProvider, VValueBag *outMetadatas = NULL)
	{
		return DecodeWIC( inDataProvider, GUID_NULL, outMetadatas);
	}
#endif

	/** create gdiplus bitmap from resource
	@remarks
		if outMetadatas is not NULL, read metatadatas in outMetadatas
	@see
		kImageMetadata
	*/
	static Gdiplus::Bitmap *Decode( HRSRC hResource, VValueBag *outMetadatas = NULL, Gdiplus::PixelFormat *inPixelFormat = NULL)
	{
		return Decode( hResource, GUID_NULL, outMetadatas, inPixelFormat);
	}

#if ENABLE_D2D
	/** create ID2D1Bitmap from resource (device dependent)
	@remarks
		pixel format is GUID_WICPixelFormat32bppPBGRA (the only one supported by ID2D1Bitmap)

		if outMetadatas is not NULL, read metatadatas in outMetadatas
	@see
		kImageMetadata
	*/
	static ID2D1Bitmap* DecodeD2D( ID2D1RenderTarget* inRT, HRSRC hResource, VValueBag *outMetadatas = NULL)
	{
		return DecodeD2D( inRT, hResource, GUID_NULL, outMetadatas);
	}

	/** create D2D-compatible IWICBitmap from resource (device independent)
	@remarks
		pixel format is GUID_WICPixelFormat32bppPBGRA (the only one supported by D2D)

		if outMetadatas is not NULL, read metatadatas in outMetadatas
	@see
		kImageMetadata
	*/
	static IWICBitmap* DecodeWIC( HRSRC hResource, VValueBag *outMetadatas = NULL)
	{
		return DecodeWIC( hResource, GUID_NULL, outMetadatas);
	}
#endif

	/** create gdiplus bitmap from input data provider 
		using decoder with the specified GUID format container
	@remarks
		if guid is GUID_NULL, use first decoder which can decode data source
		otherwise use decoder associated with image container guid
		if outMetadatas is not NULL, read metatadatas in outMetadatas
	@see
		kImageMetadata
	*/
	static Gdiplus::Bitmap *Decode( VPictureDataProvider& inDataProvider, const GUID& inGUIDFormatContainer, VValueBag *outMetadatas = NULL, Gdiplus::PixelFormat *inPixelFormat = NULL); 
	
#if ENABLE_D2D
	/** create ID2D1Bitmap from input data provider (device dependent)
		using decoder with the specified GUID format container
	@remarks
		pixel format is GUID_WICPixelFormat32bppPBGRA (the only one supported by ID2D1Bitmap)

		if guid is GUID_NULL, use first decoder which can decode data source
		otherwise use decoder associated with image container guid
		if outMetadatas is not NULL, read metatadatas in outMetadatas
	@see
		kImageMetadata
	*/
	static ID2D1Bitmap* DecodeD2D( ID2D1RenderTarget* inRT, VPictureDataProvider& inDataProvider, const GUID& inGUIDFormatContainer, VValueBag *outMetadatas = NULL);

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
	static IWICBitmap* DecodeWIC( VPictureDataProvider& inDataProvider, const GUID& inGUIDFormatContainer, VValueBag *outMetadatas = NULL);
#endif

	/** create gdiplus bitmap from resource 
		using decoder with the specified GUID format container
	@remarks
		if guid is GUID_NULL, use first decoder which can decode data source
		otherwise use decoder associated with image container guid
		if outMetadatas is not NULL, read metatadatas in outMetadatas
	@see
		kImageMetadata
	*/
	static Gdiplus::Bitmap *Decode( HRSRC hResource, const GUID& inGUIDFormatContainer, VValueBag *outMetadatas = NULL, Gdiplus::PixelFormat *inPixelFormat = NULL); 

#if ENABLE_D2D
	/** create ID2D1Bitmap from resource (device dependent)
		using decoder with the specified GUID format container
	@remarks
		pixel format is GUID_WICPixelFormat32bppPBGRA (the only one supported by ID2D1Bitmap)

		if guid is GUID_NULL, use first decoder which can decode data source
		otherwise use decoder associated with image container guid
		if outMetadatas is not NULL, read metatadatas in outMetadatas
	@see
		kImageMetadata
	*/
	static ID2D1Bitmap* DecodeD2D( ID2D1RenderTarget* inRT, HRSRC hResource, const GUID& inGUIDFormatContainer, VValueBag *outMetadatas = NULL);

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
	static IWICBitmap* DecodeWIC( HRSRC hResource, const GUID& inGUIDFormatContainer, VValueBag *outMetadatas = NULL);

	/** create ID2D1Bitmap from Gdiplus Bitmap 
	@remarks
		dest pixel format is GUID_WICPixelFormat32bppPBGRA (the only one supported by ID2D1Bitmap)
	*/
	static ID2D1Bitmap* DecodeD2D( ID2D1RenderTarget* inRT, Gdiplus::Bitmap *inBmpSource);

	/** create IWICBitmap from Gdiplus Bitmap
	@remarks
		dest format inherits gdiplus format if inWICPixelFormat is not specified
	*/
	static IWICBitmap* DecodeWIC( Gdiplus::Bitmap *inBmpSource, const VRect *inBounds = NULL, const GUID *inWICPixelFormat = NULL);
#endif

	/** create a Gdiplus bitmap from a WIC bitmap source */
	static Gdiplus::Bitmap *FromWIC( IWICBitmapSource *inWICBmpSrc, Gdiplus::PixelFormat *inPixelFormat = NULL);


	/** read metadatas from input data provider
	@remarks
		if guid is GUID_NULL, use first decoder which can decode data source
		otherwise use decoder associated with image container guid
	@see
		kImageMetadata
	*/
	static void GetMetadatas( VPictureDataProvider& inDataProvider, const GUID& inGUIDFormatContainer, VValueBag *outMetadatas); 

	/** create gdiplus bitmap from input file 
	@remarks
		if outMetadatas is not NULL, read metatadatas in outMetadatas
	@see
		kImageMetadata
	*/
	static Gdiplus::Bitmap *Decode( const VFile& inFile, VValueBag *outMetadatas = NULL, Gdiplus::PixelFormat *inPixelFormat = NULL); 

	/** convert WIC pixel format to gdiplus pixel format
	@remarks
		return PixelFormatUndefined if WIC pixel format is not supported by gdiplus
	*/
	static Gdiplus::PixelFormat FromWICPixelFormatGUID( const GUID& inWICPixelFormat);

	/** convert gdiplus pixel format to WIC pixel format
	@remarks
		return GUID_WICPixelFormatUndefined if gdiplus pixel format is not supported by WIC
	*/
	static const GUID& FromGdiplusPixelFormat( Gdiplus::PixelFormat inPF);

	/** return true if WIC pixel format has alpha component */
	static bool HasPixelFormatAlpha( const GUID& inWICPixelFormat);

protected:
	virtual VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet=NULL) const;
	virtual VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet=NULL) const;

	/** mutex to make com interface thread-safe */
	static	XBOX::VCriticalSection sLock;
};


END_TOOLBOX_NAMESPACE

#endif


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
#ifndef __VImageIOCodec__
#define __VImageIOCodec__

BEGIN_TOOLBOX_NAMESPACE

/** metadata tag types */
typedef enum eMetaTagType
{
	TT_EXIF_VERSION,		//Exif Version ASCII string
		
	TT_EXIF_DATETIME,		//Exif datetime
		
	TT_GPSVERSIONID,		//GPSVersionID tag (use XMP format for bag storage)

	TT_BLOB,				//undefined tag (tags of type UNDEFINED are stored as blobs in metadatas bag)
	
	//following tag types are used only for reading
	
	TT_AUTO,				//guess tag type based on CFDictionary value type 
	
	//following tag types are used only for writing
	
	TT_STRING,				//tag destination value type is string (default)
	TT_INT,					//tag destination value type is integer 
	TT_REAL,				//tag destination value type is real
	
	TT_ARRAY_STRING,		//tag destination value type is string array
	TT_ARRAY_INT,			//tag destination value type is integer array
	TT_ARRAY_REAL			//tag destination value type is real array
	
} eMetaTagType;
	

/** ImageIO codec info class */
class XTOOLBOX_API VImageIOCodecInfo : public VObject 
{
public:
	VImageIOCodecInfo ( const VString& inUTI, bool inCanEncode);
	VImageIOCodecInfo (const VImageIOCodecInfo& inRef);
	virtual	~VImageIOCodecInfo ();
	
	/** get accessor on codec UTI */ 
	const VString&	GetUTI () const { return fUTI; };
	
	/** return true if codec can encode */
	bool CanEncode() const { return fCanEncode; }
	
	uBOOL	FindMimeType(const VString& inMime) const;
	uBOOL	FindExtension(const VString& inExt) const;
	uBOOL	FindMacType(const OsType) const;
	
	sLONG CountMimeType() const										{return (sLONG)fMimeTypeList.size();};
	sLONG CountExtension() const									{return (sLONG)fExtensionList.size();};
	sLONG CountMacType() const										{return (sLONG)fMacTypeList.size();};
	
	void GetNthMimeType(sLONG inIndex,VString& outMime)	const		{outMime		=fMimeTypeList[inIndex];};
	void GetNthExtension(sLONG inIndex,VString& outExtension) const	{outExtension	=fExtensionList[inIndex];};
	void GetNthMacType(sLONG inIndex,OsType& outType) const			{outType		=fMacTypeList[inIndex];};
	
protected:
	/** add values from a CFArray or CFString reference */
	void _addValuesFromCFArrayOrCFString( CFTypeRef inValues, std::vector<VString>& outValues); 
	
	/** ImageIO codec UTI */
	VString fUTI;
	
	/** true if codec can encode */
	bool fCanEncode;
	
	/** codec mime type list */
	std::vector<VString> fMimeTypeList;
	
	/** codec extension type list */
	std::vector<VString> fExtensionList;
	
	/** codec mac type list
	 @remarks
	 obsolete but for compatibility with older API	 
	 */
	std::vector<OsType> fMacTypeList;
};


/** Image data provider class */
class XTOOLBOX_API stImageSource : public VObject
{
public:
	stImageSource(VPictureDataProvider& inDataProvider, bool inGetProperties = true);
	virtual	~stImageSource();
	
	/** true if image data provider is valid 
	 @remarks
		false if source data provider is not a valid image
	 */
	bool IsValid() const;
	
	/** get image width */
	uLONG GetWidth() const;
	
	/** get image height */
	uLONG GetHeight() const;
	
	/** get image color bit depth */
	uLONG GetBitDepth() const;
	
	/** true if image has alpha channel */
	bool hasAlpha() const;

	/** copy image metadatas to the provided prop container
	 @remarks
		copy all image metadatas and not only the ones supported in ImageMeta.h
	*/
	void CopyMetadatas(CFMutableDictionaryRef ioProps);
	
	/** copy image metadatas to the metadata bag */
	void ReadMetadatas(VValueBag *ioValueBag);
	
	/** retain image source reference */
	CGImageSourceRef RetainImageSourceRef();
protected:
	/** convert a CFDictionnary key-value pair to a VValueBag attribute */
	bool _FromCFDictionnaryKeyValuePairToBagKeyValuePair(	CFDictionaryRef inCFDict, CFStringRef inCFDictKey, 
															VValueBag& ioBag, const XBOX::VValueBag::StKey& inBagKey,
															eMetaTagType inTagType = TT_AUTO); 
	
private:
	/** ImageIO image source reference */
	CGImageSourceRef fImageSource;
	
	/** ImageIO image source properties */
	CFDictionaryRef fProperties;	
};


/** ImageIO codec class */
class XTOOLBOX_API VPictureCodec_ImageIO : public VPictureCodec
{
public:
	VPictureCodec_ImageIO( const VImageIOCodecInfo& inImageIOCodecInfo);
	virtual	~VPictureCodec_ImageIO() {}
	
	virtual bool ValidateData(VFile& inFile)const;
	virtual bool ValidateData(VPictureDataProvider& inDataProvider)const;
	
	virtual VPictureData* _CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder=0) const;
	
	/** encode input picture data with the specified image UTI 
	 @remarks
		if inSettings is not NULL, encoding properties are stored in inSettings attributes (see kImageEncoding) and eventually 
		modified metadatas are stored in a unique metatadas element of inSettings with key ImageMeta::Metadatas;
		if inSettings is NULL or metadatas element is not present, encoder will use image source initial metadatas stored in inData if any
	 @note
		only image source metadatas which are compatible with image destination metadatas will be encoded
	 */
	static VError _Encode(const VString& inUTI,
						 const VPictureData& inData,
						 const VValueBag* inSettings,
						 VBlob& outBlob,
						 VPictureDrawSettings* inSet = NULL);
	
	/** return true if codec can encode metadatas */
	bool CanEncodeMetas() const;

	/** create CoreGraphics image from input data provider */
	static CGImageRef Decode( VPictureDataProvider& inDataProvider, VValueBag *outMetadatas = NULL); 
	
	/** create CoreGraphics image from input file */
	static CGImageRef Decode( const VFile& inFile, VValueBag *outMetadatas = NULL); 
	
	/** get metadatas from input data provider */
	static void GetMetadatas( VPictureDataProvider& inDataProvider, VValueBag *outMetadatas); 
	
	/** write metadatas to the destination properties dictionnary */
	static void WriteMetadatas(CFMutableDictionaryRef ioProps, const VValueBag *inBagMetas,
							   uLONG inImageWidth,
							   uLONG inImageHeight,
							   VPictureDataProvider *inImageSrcDataProvider = NULL,
							   bool inEncodeImageFrames = true);
		
protected:
	virtual VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet=NULL) const;
	virtual VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet=NULL) const;
	
	
	/** convert a VValueBag attribute key-value pair to a CFDictionnary key-value pair */
	static bool _FromBagKeyValuePairToCFDictionnaryKeyValuePair(	CFMutableDictionaryRef ioCFDict, CFStringRef inCFDictKey, 
																	const VValueBag& inBag, const XBOX::VValueBag::StKey& inBagKey,
																	eMetaTagType inTagType = TT_STRING); 
	
	/** convert a VValueBag attribute to a CFDictionnary key-value pair */
	static bool _FromVValueSingleToCFDictionnaryKeyValuePair(		CFMutableDictionaryRef ioCFDict, 
																	CFStringRef inCFDictKey, 
																	const VValueSingle *att,																			
																	eMetaTagType inTagType =  TT_STRING);
	
};


END_TOOLBOX_NAMESPACE

#endif


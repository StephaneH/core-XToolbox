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
#include "VFont.h"
#include "V4DPictureIncludeBase.h"
#include "ImageMeta.h"

#import <ApplicationServices/ApplicationServices.h>

#if VERSIONDEBUG
#define TEST_METADATAS 0
#else
#define TEST_METADATAS 0
#endif

VImageIOCodecInfo::VImageIOCodecInfo(const VImageIOCodecInfo& inRef):VObject()
{
	fUTI			= inRef.fUTI;
	fCanEncode		= inRef.fCanEncode;
	fMimeTypeList	= inRef.fMimeTypeList;
	fExtensionList	= inRef.fExtensionList;
	fMacTypeList	= inRef.fMacTypeList;
}

VImageIOCodecInfo::VImageIOCodecInfo(const VString& inUTI, bool inCanEncode):VObject()
{
	fUTI = inUTI;
	fCanEncode = inCanEncode; 
	
	CFStringRef uti = inUTI.MAC_RetainCFStringCopy();
	if(UTTypeConformsTo(uti, kUTTypeImage)
	   ||
	   UTTypeConformsTo(uti, kUTTypePDF)) //PDF is not conforming to public.image type because it is mixed document
	{
		//copy the declaration for the UTI (if it exists)
		CFDictionaryRef declaration = UTTypeCopyDeclaration(uti);
		if(declaration != NULL)
		{
			//grab the tags for this UTI, which includes extensions, OSTypes and MIME types.
			CFDictionaryRef tags = (CFDictionaryRef)CFDictionaryGetValue(declaration, kUTTypeTagSpecificationKey);
			if(tags != NULL)
			{
				//browse extensions
				CFTypeRef filenameExtensions = CFDictionaryGetValue(tags, kUTTagClassFilenameExtension);
				_addValuesFromCFArrayOrCFString( filenameExtensions, fExtensionList);
				
				//browse mime types
				CFTypeRef mimeTypes = CFDictionaryGetValue(tags, kUTTagClassMIMEType);
				_addValuesFromCFArrayOrCFString( mimeTypes, fMimeTypeList);
				
				//browse OSTypes 
				std::vector<VString> vOSTypes;
				CFTypeRef osTypes = CFDictionaryGetValue(tags, kUTTagClassOSType);
				_addValuesFromCFArrayOrCFString( osTypes, vOSTypes);
				std::vector<VString>::iterator it = vOSTypes.begin();
				for (;it != vOSTypes.end(); it++)
				{
					CFStringRef osType;
					osType = (*it).MAC_RetainCFStringCopy();
					OSType type = UTGetOSTypeFromString( osType);
					if (type)
						fMacTypeList.push_back( type);
					CFRelease(osType);
				}
			}
			CFRelease(declaration);
		}
	}
	CFRelease(uti);
}


/** add values from a CFArray or CFString reference */
void VImageIOCodecInfo::_addValuesFromCFArrayOrCFString( CFTypeRef inValues, std::vector<VString>& outValues)
{
#if VERSIONDEBUG
//	CFShow( inValues);
#endif
	if(inValues != NULL)
	{
		CFTypeID type = CFGetTypeID(inValues);
		if(type == CFStringGetTypeID())
		{
			//add single value
			VString xvalue;
			xvalue.MAC_FromCFString((CFStringRef)inValues);
			outValues.push_back(xvalue);
		}
		else if (type == CFArrayGetTypeID())
		{
			//add multiple values
			CFArrayRef values = (CFArrayRef)inValues;
			sLONG count = CFArrayGetCount( values);
			for (int i = 0; i < count; i++)
			{
				CFStringRef value = (CFStringRef)CFArrayGetValueAtIndex( values, i);
				VString xvalue;
				xvalue.MAC_FromCFString(value);
				outValues.push_back(xvalue);
			}
		}
	}
}


uBOOL VImageIOCodecInfo::FindExtension(const VString& inExt) const
{
	for( std::vector<VString>::const_iterator i = fExtensionList.begin() ; i != fExtensionList.end() ; ++i)
	{
		if(*i == inExt)
			return true;
	}
	return false;
}

uBOOL VImageIOCodecInfo::FindMacType(const OsType inType) const
{
	for( std::vector<OsType>::const_iterator i = fMacTypeList.begin() ; i != fMacTypeList.end() ; ++i)
	{
		if(*i == inType)
			return true;
	}
	return false;
}

uBOOL VImageIOCodecInfo::FindMimeType(const VString& inMime) const
{
	for( std::vector<VString>::const_iterator i = fMimeTypeList.begin() ; i != fMimeTypeList.end() ; ++i)
	{
		if (*i == inMime)
			return true;
	}
	return false;
}


VImageIOCodecInfo::~VImageIOCodecInfo()
{
}




VPictureCodec_ImageIO::VPictureCodec_ImageIO(const VImageIOCodecInfo& inImageIOCodecInfo)
:VPictureCodec()
{
	fUTI				= inImageIOCodecInfo.GetUTI();
	
	if (!fUTI.IsEmpty())
	{	
		//extract display name from UTI:
		
		//extract last tag
		VString name = fUTI;
		VIndex posComma = name.FindUniChar( '.', name.GetLength(), true);
		if (posComma > 0)
			name.Remove(1,posComma);
		if (!name.IsEmpty())
		{
			//set first char to uppercase
			UniChar firstChar = name.GetUniChar(1);
			if (firstChar >= 'a' && firstChar <= 'z')
			{
				firstChar -= 'a'-'A';
				name.SetUniChar(1, firstChar);
			}
			
			//remove "-image" part
			name.Exchange( "-image", ""); 
			
			SetDisplayName(name);
		}
	}
	
	SetPlatform(1); 
	SetBuiltIn(false);
	
	for(long nbmime=0;nbmime < inImageIOCodecInfo.CountMimeType();nbmime++)
	{
		VString mime;
		inImageIOCodecInfo.GetNthMimeType(nbmime,mime);
		AppendMimeType(mime);
	}
	for(long nbext=0;nbext < inImageIOCodecInfo.CountExtension();nbext++)
	{
		VString ext;
		inImageIOCodecInfo.GetNthExtension(nbext,ext);
		AppendExtension(ext);
	}
	for(long nbext=0;nbext < inImageIOCodecInfo.CountMacType();nbext++)
	{
		OsType ext;
		inImageIOCodecInfo.GetNthMacType(nbext,ext);
		AppendMacType((sLONG)ext);
	}
	
	SetEncode(inImageIOCodecInfo.CanEncode());
	SetCanValidateData(true); //TODO: check if we can validate data with ImageIO function ?
}
VPictureData* VPictureCodec_ImageIO::_CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder) const
{
	return new VPictureData_CGImage(&inDataProvider, inRecorder);
}

bool VPictureCodec_ImageIO::ValidateData(VFile& inFile)const
{
	bool result=false;
	VPictureDataProvider* ds=VPictureDataProvider::Create(inFile,false);
	if(ds)
	{
		result=ValidateData(*ds);
		ds->Release();
	}
	return result;
}
bool VPictureCodec_ImageIO::ValidateData(VPictureDataProvider& inDataProvider)const
{
	bool result=false;
		
	if(!fUTI.IsEmpty())
	{
		CFStringRef uti=fUTI.MAC_RetainCFStringCopy();
		
		CGDataProviderRef dataProvider = xV4DPicture_MemoryDataProvider::CGDataProviderCreate(&inDataProvider);
		
		CGImageSourceRef test = CGImageSourceCreateWithDataProvider(dataProvider,NULL);
		
		if (test!=NULL)
		{
			if(CGImageSourceGetStatus(test) == kCGImageStatusComplete)
			{
				CFStringRef testuti=CGImageSourceGetType(test);
				if(testuti)
				{
					if(UTTypeEqual(uti, testuti))
						result=true;
					CFRelease(testuti);
				}
				
			}
			CFRelease(test);
		}
		
		CFRelease(uti);
		CFRelease(dataProvider);
	}
	return result;
}

VError VPictureCodec_ImageIO::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet)const
{
	VError result=VE_UNKNOWN_ERROR;
	result = _Encode( fUTI, inData, inSettings, outBlob, inSet);
	return result;
}


VError VPictureCodec_ImageIO::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet)const
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


/** encode input picture data with the specified image UTI */
VError VPictureCodec_ImageIO::_Encode(const VString& inUTI,
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
	{
		VValueBag *metas = new VValueBag();
		{
			ImageMeta::stWriter metaWriter( metas);
			metaWriter.SetIPTCObjectName( "IPTCObjectName");
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
	
	VError result = VE_OK;
	
	//create image data buffer
	CFMutableDataRef dataImage = CFDataCreateMutable( kCFAllocatorDefault, 0);
	xbox_assert(dataImage != NULL);
	
	//create ImageIO destination reference
	CFStringRef uti = inUTI.MAC_RetainCFStringCopy();
	CGImageDestinationRef imageDestRef = CGImageDestinationCreateWithData( dataImage, uti, 1, NULL);
	CFRelease(uti);
	if (imageDestRef == NULL)
	{
		result = VE_INVALID_PARAMETER;
		CFRelease(dataImage);
		return result;
	}
	
	//determine if we need to re-encode VPictureData image frames or not
	//
	//we can only copy directly image frames from VPictureData if:
	//- image source format is equal to image dest format 
	//and
	//- image has no picture transform
	//and
	//- image encoding quality is 1 
	//(otherwise we need to re-encode frames)
	bool bEncodeImageFrames = true;
	if (inData.GetDataProvider()) 
	{
		if ((!inSet) || inSet->IsIdentityMatrix())
		{
			//remark: if ImageQuality attribute is not present, we assume that frame does not need to be re-encoded
			if (inSettings)
			{
				if (inSettings->AttributeExists(ImageEncoding::ImageQuality))
				{
					ImageEncoding::stReader reader(inSettings);
					bEncodeImageFrames = reader.GetImageQuality() < 1.0;
				}
				else
					bEncodeImageFrames = false;
			}
			else
				bEncodeImageFrames = false;
			
			if (!bEncodeImageFrames)
			{
				//check source and dest image formats
				VPictureCodecFactoryRef pictureCodecFactory;
				const VPictureCodec *decoder = pictureCodecFactory->RetainDecoderByIdentifier( inData.GetEncoderID());
				if (decoder)
				{
					if (decoder->GetUTI() != inUTI)
						bEncodeImageFrames = true;
					decoder->Release();
				}
				else
					bEncodeImageFrames = true;
			}
		}
	}
	
	//remark: as kCGImageDestinationBackgroundColor ImageIO property is buggy,
	//we need to manually composite image on background if image destination format cannot support alpha component
	VPictureDrawSettings set(inSet);
	if (inUTI.EqualToString("com.microsoft.bmp")
		||
		inUTI.EqualToString("public.jpeg")
		||
		inUTI.EqualToString("public.jpeg-2000"))
		set.SetBackgroundColor( ENCODING_BACKGROUND_COLOR);
		
	//create CoreGraphics image
	CGImageRef src = bEncodeImageFrames ? inData.CreateCGImage(&set) : NULL;
	if(src || (!bEncodeImageFrames))
	{
		//encode image destination properties
		CFMutableDictionaryRef props = CFDictionaryCreateMutable( kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
		xbox_assert(props);
		
		//set background color (used only if destination image format does not support alpha component)
		
		//remark: kCGImageDestinationBackgroundColor property is buggy so composite image on background manually - see above
		/*
		CGColorSpaceRef rgbColorSpace = CGColorSpaceCreateDeviceRGB();
		CGFloat components[] = { ((Real)inBgColor.GetRed())/255.0f, ((Real)inBgColor.GetGreen())/255.0f, ((Real)inBgColor.GetBlue())/255.0f, ((Real)inBgColor.GetAlpha())/255.0f };
		CGColorRef colorValue = CGColorCreate(rgbColorSpace, components);
		xbox_assert(colorValue);
		CGColorSpaceRelease(rgbColorSpace);
		CFDictionarySetValue(props, kCGImageDestinationBackgroundColor, colorValue);
		CGColorRelease(colorValue);
		*/
		
		{
			ImageEncoding::stReader reader(inSettings);
			
			//set image compression quality
			Real ImageQuality = reader.GetImageQuality();
			
			CFNumberRef number = CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &ImageQuality);
			xbox_assert(number);
			CFDictionarySetValue(props, kCGImageDestinationLossyCompressionQuality, number);
			CFRelease(number);
			
			//eventually encode metadatas
			if (reader.WriteMetadatas())
			{
				const VValueBag *metas = reader.RetainMetadatas();
				if (metas || inData.GetDataProvider())
				{
					//encode modified metadatas
					uLONG width = CGImageGetWidth(src);
					uLONG height = CGImageGetHeight(src);
					WriteMetadatas(props, metas, width, height, inData.GetDataProvider(), bEncodeImageFrames);
				}
				if (metas)
					metas->Release();
			}
		}
		
		//add image to image destination container
		if (bEncodeImageFrames)
			CGImageDestinationAddImage( imageDestRef, src, props);
		else
		{
			//direct copy frame from image source 
			stImageSource imageSrc( *inData.GetDataProvider(), false);
			CGImageSourceRef imageSrcRef = imageSrc.RetainImageSourceRef();
			if (imageSrcRef)
			{
				CFDictionarySetValue(props, kCGImageDestinationLossyCompressionQuality, kCFNull); //ensure we do not compress further
				CGImageDestinationAddImageFromSource( imageDestRef, imageSrcRef, 0, props);
			
				CFRelease(imageSrcRef); 
			}
		}
		
		//finalize image
		bool success = false;
		try
		{
			success = CGImageDestinationFinalize( imageDestRef);
		}
		catch(...)
		{
			success = false;
		}
		
		if (success)
			//copy image data to blob
			outBlob.PutData( CFDataGetMutableBytePtr( dataImage), CFDataGetLength( dataImage), 0);
		else
			result = VE_UNKNOWN_ERROR;

		//release properties
		CFRelease(props);
	
		//release coregraphics image
		if (src)
			CFRelease(src);
	}
	
	//release ImageIO dest reference
	CFRelease( imageDestRef);
	
	//release image data buffer
	CFRelease( dataImage);								 
	return result;
}

/** return true if codec can encode metadatas */
bool VPictureCodec_ImageIO::CanEncodeMetas() const
{
	if (fUTI.EqualToString("public.png")
		||
		fUTI.EqualToString("com.microsoft.bmp")
		||
		fUTI.EqualToString("com.compuserve.gif")
		||
		fUTI.EqualToString("com.microsoft.ico"))
		return false;
	return true;
}

/** write metadatas to the destination properties dictionnary */
void VPictureCodec_ImageIO::WriteMetadatas(CFMutableDictionaryRef ioProps, const VValueBag *inBagMetas,
										   uLONG inImageWidth,
										   uLONG inImageHeight,
										   VPictureDataProvider *inImageSrcDataProvider,
										   bool inEncodeImageFrames)
{
	if (inImageSrcDataProvider)
	{
		//copy image src metadatas to image dest
		stImageSource imageSrc( *inImageSrcDataProvider);
		if (imageSrc.IsValid())
			imageSrc.CopyMetadatas( ioProps);
		else
			inImageSrcDataProvider = NULL;
	}
	
	
	//write TIFF metadatas 
	const VValueBag *bagMetaTIFF = inBagMetas ? inBagMetas->GetUniqueElement( ImageMetaTIFF::TIFF) : NULL;
	if (bagMetaTIFF || inImageSrcDataProvider)
	{
		CFDictionaryRef metaSrc = NULL;
		CFMutableDictionaryRef metaTIFF = NULL;
		if (CFDictionaryGetValueIfPresent(ioProps, kCGImagePropertyTIFFDictionary, (const void**)&metaSrc))
		{
			metaTIFF = CFDictionaryCreateMutableCopy( kCFAllocatorDefault, 0, metaSrc);
			if (inEncodeImageFrames)
			{
				//following metas are related to the data provider but they are not related to the CGImageRef build from the data provider 
				//so we need to remove following tags which have been used yet by ImageIO
				//to generate a CGImageRef from the data provider
				CFDictionaryRemoveValue( metaTIFF, kCGImagePropertyTIFFCompression);
				CFDictionaryRemoveValue( metaTIFF, kCGImagePropertyTIFFTransferFunction);
			}
		}
		else
			metaTIFF = CFDictionaryCreateMutable( kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
		
		if (bagMetaTIFF)
		{
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaTIFF,		kCGImagePropertyTIFFXResolution,
															*bagMetaTIFF,	ImageMetaTIFF::XResolution,
															TT_REAL);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaTIFF,		kCGImagePropertyTIFFYResolution,
															*bagMetaTIFF,	ImageMetaTIFF::YResolution,
															TT_REAL);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaTIFF,		kCGImagePropertyTIFFResolutionUnit,
															*bagMetaTIFF,	ImageMetaTIFF::ResolutionUnit,
															TT_INT);
			/* //remark: this tag is set by ImageIO (TIFF only) and must not be overriden
			 
			 _FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaTIFF,		kCGImagePropertyTIFFCompression,
			 *bagMetaTIFF,	ImageMetaTIFF::Compression,
			 TT_INT);
			 */
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaTIFF,		kCGImagePropertyTIFFPhotometricInterpretation,
															*bagMetaTIFF,	ImageMetaTIFF::PhotometricInterpretation,
															TT_INT);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaTIFF,		kCGImagePropertyTIFFDocumentName,
															*bagMetaTIFF,	ImageMetaTIFF::DocumentName);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaTIFF,		kCGImagePropertyTIFFImageDescription,
															*bagMetaTIFF,	ImageMetaTIFF::ImageDescription);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaTIFF,		kCGImagePropertyTIFFMake,
															*bagMetaTIFF,	ImageMetaTIFF::Make);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaTIFF,		kCGImagePropertyTIFFModel,
															*bagMetaTIFF,	ImageMetaTIFF::Model);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaTIFF,		kCGImagePropertyTIFFOrientation,
															*bagMetaTIFF,	ImageMetaTIFF::Orientation);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaTIFF,		kCGImagePropertyTIFFSoftware,
															*bagMetaTIFF,	ImageMetaTIFF::Software);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaTIFF,		kCGImagePropertyTIFFDateTime,
															*bagMetaTIFF,	ImageMetaTIFF::DateTime,
															TT_EXIF_DATETIME);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaTIFF,		kCGImagePropertyTIFFArtist,
															*bagMetaTIFF,	ImageMetaTIFF::Artist);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaTIFF,		kCGImagePropertyTIFFHostComputer,
															*bagMetaTIFF,	ImageMetaTIFF::HostComputer);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaTIFF,		kCGImagePropertyTIFFCopyright,
															*bagMetaTIFF,	ImageMetaTIFF::Copyright);
#if ENABLE_ALL_METAS
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaTIFF,		kCGImagePropertyTIFFWhitePoint,
															*bagMetaTIFF,	ImageMetaTIFF::WhitePoint, TT_ARRAY_REAL);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaTIFF,		kCGImagePropertyTIFFPrimaryChromaticities,
															*bagMetaTIFF,	ImageMetaTIFF::PrimaryChromaticities, TT_ARRAY_REAL);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaTIFF,		kCGImagePropertyTIFFTransferFunction,
															*bagMetaTIFF,	ImageMetaTIFF::TransferFunction, TT_ARRAY_INT);
#endif
		}
		
		if (CFDictionaryGetCount(metaTIFF) > 0)
			CFDictionarySetValue(ioProps, kCGImagePropertyTIFFDictionary, metaTIFF);
		else
		{
			if (inEncodeImageFrames)
				CFDictionaryRemoveValue(ioProps, kCGImagePropertyTIFFDictionary);
			else
				CFDictionarySetValue(ioProps, kCGImagePropertyTIFFDictionary, kCFNull); //set no NULL in order to remove from input image source if present
		}
		CFRelease(metaTIFF);
	}

	
	//write EXIF metadatas 
	const VValueBag *bagMetaEXIF = inBagMetas ? inBagMetas->GetUniqueElement( ImageMetaEXIF::EXIF) : NULL;
	if (bagMetaEXIF || inImageSrcDataProvider)
	{
		CFDictionaryRef metaSrc = NULL;
		CFMutableDictionaryRef metaEXIF = NULL;
		if (CFDictionaryGetValueIfPresent(ioProps, kCGImagePropertyExifDictionary, (const void**)&metaSrc))
		{
			metaEXIF = CFDictionaryCreateMutableCopy( kCFAllocatorDefault, 0, metaSrc);
			if (inEncodeImageFrames)
			{
				//following metas are related to the data provider but they are not related to the CGImageRef build from the data provider 
				//so we need to remove following tags - which have been used yet by ImageIO to generate a CGImageRef from the data provider
				//(otherwise following image adjustments would be applied twice on the image from data provider)
				CFDictionaryRemoveValue( metaEXIF, kCGImagePropertyExifCustomRendered);
				CFDictionaryRemoveValue( metaEXIF, kCGImagePropertyExifGainControl);
				CFDictionaryRemoveValue( metaEXIF, kCGImagePropertyExifContrast);
				CFDictionaryRemoveValue( metaEXIF, kCGImagePropertyExifSaturation);
				CFDictionaryRemoveValue( metaEXIF, kCGImagePropertyExifSharpness);
				CFDictionaryRemoveValue( metaEXIF, kCGImagePropertyExifGamma);
				CFDictionaryRemoveValue( metaEXIF, kCGImagePropertyExifCompressedBitsPerPixel);
				CFDictionaryRemoveValue( metaEXIF, kCGImagePropertyExifColorSpace);
				CFDictionaryRemoveValue( metaEXIF, kCGImagePropertyExifPixelXDimension);
				CFDictionaryRemoveValue( metaEXIF, kCGImagePropertyExifPixelYDimension);
			}
		}
		else
			metaEXIF = CFDictionaryCreateMutable( kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
		
		if (bagMetaEXIF)
		{
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifExposureTime,
															*bagMetaEXIF,	ImageMetaEXIF::ExposureTime,
															TT_REAL);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifFNumber,
															*bagMetaEXIF,	ImageMetaEXIF::FNumber,
															TT_REAL);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifExposureProgram,
															*bagMetaEXIF,	ImageMetaEXIF::ExposureProgram,
															TT_INT);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifSpectralSensitivity,
															*bagMetaEXIF,	ImageMetaEXIF::SpectralSensitivity);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifISOSpeedRatings,
															*bagMetaEXIF,	ImageMetaEXIF::ISOSpeedRatings, TT_ARRAY_INT);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifVersion,
															*bagMetaEXIF,	ImageMetaEXIF::ExifVersion, TT_EXIF_VERSION);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifDateTimeOriginal,
															*bagMetaEXIF,	ImageMetaEXIF::DateTimeOriginal, 
															TT_EXIF_DATETIME);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifDateTimeDigitized,
															*bagMetaEXIF,	ImageMetaEXIF::DateTimeDigitized, 
															TT_EXIF_DATETIME);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifComponentsConfiguration,
															*bagMetaEXIF,	ImageMetaEXIF::ComponentsConfiguration, TT_ARRAY_INT);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifCompressedBitsPerPixel,
															*bagMetaEXIF,	ImageMetaEXIF::CompressedBitsPerPixel, 
															TT_REAL);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifShutterSpeedValue,
															*bagMetaEXIF,	ImageMetaEXIF::ShutterSpeedValue, 
															TT_REAL);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifApertureValue,
															*bagMetaEXIF,	ImageMetaEXIF::ApertureValue, 
															TT_REAL);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifBrightnessValue,
															*bagMetaEXIF,	ImageMetaEXIF::BrightnessValue, 
															TT_REAL);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifExposureBiasValue,
															*bagMetaEXIF,	ImageMetaEXIF::ExposureBiasValue, 
															TT_REAL);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifMaxApertureValue,
															*bagMetaEXIF,	ImageMetaEXIF::MaxApertureValue, 
															TT_REAL);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifSubjectDistance,
															*bagMetaEXIF,	ImageMetaEXIF::SubjectDistance, 
															TT_REAL);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifMeteringMode,
															*bagMetaEXIF,	ImageMetaEXIF::MeteringMode, 
															TT_INT);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifLightSource,
															*bagMetaEXIF,	ImageMetaEXIF::LightSource, 
															TT_INT);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifFlash,
															*bagMetaEXIF,	ImageMetaEXIF::Flash, 
															TT_INT);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifFocalLength,
															*bagMetaEXIF,	ImageMetaEXIF::FocalLength, 
															TT_REAL);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifSubjectArea,
															*bagMetaEXIF,	ImageMetaEXIF::SubjectArea, TT_ARRAY_INT);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifFlashPixVersion,
															*bagMetaEXIF,	ImageMetaEXIF::FlashPixVersion, TT_EXIF_VERSION);
			
			/* //remark: following tags are set by ImageIO and must not be overriden (!)
			 {
			 //force sRGB colorspace 
			 VLong sRGB(1);
			 _FromVValueSingleToCFDictionnaryKeyValuePair(	metaEXIF,	kCGImagePropertyExifColorSpace,
			 static_cast<const VValueSingle *>(&sRGB),
			 TT_INT);
			 }
			 {
			 //set actual image dimensions
			 VLong width((sLONG)inImageWidth);
			 VLong height((sLONG)inImageHeight);
			 _FromVValueSingleToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifPixelXDimension,
			 static_cast<const VValueSingle *>(&width),
			 TT_INT);
			 _FromVValueSingleToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifPixelYDimension,
			 static_cast<const VValueSingle *>(&height),
			 TT_INT);
			 }
			 */
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifRelatedSoundFile,
															*bagMetaEXIF,	ImageMetaEXIF::RelatedSoundFile);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifFlashEnergy,
															*bagMetaEXIF,	ImageMetaEXIF::FlashEnergy,
															TT_REAL);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifFocalPlaneXResolution,
															*bagMetaEXIF,	ImageMetaEXIF::FocalPlaneXResolution,
															TT_REAL);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifFocalPlaneYResolution,
															*bagMetaEXIF,	ImageMetaEXIF::FocalPlaneYResolution,
															TT_REAL);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifFocalPlaneResolutionUnit,
															*bagMetaEXIF,	ImageMetaEXIF::FocalPlaneResolutionUnit,
															TT_INT);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifSubjectLocation,
															*bagMetaEXIF,	ImageMetaEXIF::SubjectLocation, TT_ARRAY_INT);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifExposureIndex,
															*bagMetaEXIF,	ImageMetaEXIF::ExposureIndex,
															TT_REAL);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifSensingMethod,
															*bagMetaEXIF,	ImageMetaEXIF::SensingMethod,
															TT_INT);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifFileSource,
															*bagMetaEXIF,	ImageMetaEXIF::FileSource,
															TT_INT);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifSceneType,
															*bagMetaEXIF,	ImageMetaEXIF::SceneType,
															TT_INT);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifCustomRendered,
															*bagMetaEXIF,	ImageMetaEXIF::CustomRendered,
															TT_INT);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifExposureMode,
															*bagMetaEXIF,	ImageMetaEXIF::ExposureMode,
															TT_INT);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifWhiteBalance,
															*bagMetaEXIF,	ImageMetaEXIF::WhiteBalance,
															TT_INT);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifDigitalZoomRatio,
															*bagMetaEXIF,	ImageMetaEXIF::DigitalZoomRatio,
															TT_REAL);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifFocalLenIn35mmFilm,
															*bagMetaEXIF,	ImageMetaEXIF::FocalLenIn35mmFilm,
															TT_INT);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifSceneCaptureType,
															*bagMetaEXIF,	ImageMetaEXIF::SceneCaptureType,
															TT_INT);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifGainControl,
															*bagMetaEXIF,	ImageMetaEXIF::GainControl,
															TT_REAL);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifContrast,
															*bagMetaEXIF,	ImageMetaEXIF::Contrast,
															TT_INT);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifSaturation,
															*bagMetaEXIF,	ImageMetaEXIF::Saturation,
															TT_INT);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifSharpness,
															*bagMetaEXIF,	ImageMetaEXIF::Sharpness,
															TT_INT);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifSubjectDistRange,
															*bagMetaEXIF,	ImageMetaEXIF::SubjectDistRange,
															TT_INT);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifImageUniqueID,
															*bagMetaEXIF,	ImageMetaEXIF::ImageUniqueID);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifGamma,
															*bagMetaEXIF,	ImageMetaEXIF::Gamma,
															TT_REAL);
			
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifMakerNote,
															*bagMetaEXIF,	ImageMetaEXIF::MakerNote);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifUserComment,
															*bagMetaEXIF,	ImageMetaEXIF::UserComment);
#if ENABLE_ALL_METAS
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifSubsecTime,
															*bagMetaEXIF,	ImageMetaEXIF::SubsecTime);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifSubsecTimeOrginal,
															*bagMetaEXIF,	ImageMetaEXIF::SubsecTimeOriginal);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifSubsecTimeDigitized,
															*bagMetaEXIF,	ImageMetaEXIF::SubsecTimeDigitized);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifSpatialFrequencyResponse,
															*bagMetaEXIF,	ImageMetaEXIF::SpatialFrequencyResponse, TT_BLOB);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifOECF,
															*bagMetaEXIF,	ImageMetaEXIF::OECF, TT_BLOB);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifCFAPattern,
															*bagMetaEXIF,	ImageMetaEXIF::CFAPattern, TT_BLOB);
			_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaEXIF,		kCGImagePropertyExifDeviceSettingDescription,
															*bagMetaEXIF,	ImageMetaEXIF::DeviceSettingDescription, TT_BLOB);
#endif
		}
		
		if (CFDictionaryGetCount(metaEXIF) > 0)
			CFDictionarySetValue(ioProps, kCGImagePropertyExifDictionary, metaEXIF);
		else
		{
			if (inEncodeImageFrames)
				CFDictionaryRemoveValue(ioProps, kCGImagePropertyExifDictionary);
			else
				CFDictionarySetValue(ioProps, kCGImagePropertyExifDictionary, kCFNull);
		}
		CFRelease(metaEXIF);
	}
	
	if (inBagMetas == NULL)
		return;
	
	//write IPTC metadatas 
	const VValueBag *bagMetaIPTC = inBagMetas->GetUniqueElement( ImageMetaIPTC::IPTC);
	if (bagMetaIPTC)
	{
		CFDictionaryRef metaSrc = NULL;
		CFMutableDictionaryRef metaIPTC = NULL;
		if (CFDictionaryGetValueIfPresent(ioProps, kCGImagePropertyIPTCDictionary, (const void**)&metaSrc))
			metaIPTC = CFDictionaryCreateMutableCopy( kCFAllocatorDefault, 0, metaSrc);
		else
			metaIPTC = CFDictionaryCreateMutable( kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
		
#if ENABLE_ALL_METAS
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCObjectTypeReference,
														*bagMetaIPTC,	ImageMetaIPTC::ObjectTypeReference);
#endif
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCObjectAttributeReference,
														*bagMetaIPTC,	ImageMetaIPTC::ObjectAttributeReference, TT_ARRAY_STRING);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCObjectName,
														*bagMetaIPTC,	ImageMetaIPTC::ObjectName);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCEditStatus,
														*bagMetaIPTC,	ImageMetaIPTC::EditStatus);
#if ENABLE_ALL_METAS
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCEditorialUpdate,
														*bagMetaIPTC,	ImageMetaIPTC::EditorialUpdate);
#endif
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCUrgency,
														*bagMetaIPTC,	ImageMetaIPTC::Urgency);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCSubjectReference,
														*bagMetaIPTC,	ImageMetaIPTC::SubjectReference, TT_ARRAY_STRING);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCCategory,
														*bagMetaIPTC,	ImageMetaIPTC::Category);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCSupplementalCategory,
														*bagMetaIPTC,	ImageMetaIPTC::SupplementalCategory, TT_ARRAY_STRING);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCFixtureIdentifier,
														*bagMetaIPTC,	ImageMetaIPTC::FixtureIdentifier);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCKeywords,
														*bagMetaIPTC,	ImageMetaIPTC::Keywords, TT_ARRAY_STRING);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCContentLocationCode,
														*bagMetaIPTC,	ImageMetaIPTC::ContentLocationCode, TT_ARRAY_STRING);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCContentLocationName,
														*bagMetaIPTC,	ImageMetaIPTC::ContentLocationName, TT_ARRAY_STRING);
		if (bagMetaIPTC->AttributeExists( ImageMetaIPTC::ReleaseDateTime))
		{
			//convert to IPTC date and time tags
			VString dateIPTC, timeIPTC, datetimeXML;
			const VValueSingle *valueSingle = bagMetaIPTC->GetAttribute( ImageMetaIPTC::ReleaseDateTime);
			valueSingle->GetXMLString( datetimeXML, XSO_Time_Local);
			IHelperMetadatas::DateTimeFromXMLToIPTC( datetimeXML, dateIPTC, timeIPTC);
			
			_FromVValueSingleToCFDictionnaryKeyValuePair(metaIPTC,	kCGImagePropertyIPTCReleaseDate,
														 static_cast<const VValueSingle *>(&dateIPTC));
			_FromVValueSingleToCFDictionnaryKeyValuePair(metaIPTC,	kCGImagePropertyIPTCReleaseTime,
														 static_cast<const VValueSingle *>(&timeIPTC));
		}
		
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCExpirationDate,
														*bagMetaIPTC,	ImageMetaIPTC::ExpirationDate);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCExpirationTime,
														*bagMetaIPTC,	ImageMetaIPTC::ExpirationTime);
		if (bagMetaIPTC->AttributeExists( ImageMetaIPTC::ExpirationDateTime))
		{
			//convert to IPTC date and time tags
			VString dateIPTC, timeIPTC, datetimeXML;
			const VValueSingle *valueSingle = bagMetaIPTC->GetAttribute( ImageMetaIPTC::ExpirationDateTime);
			valueSingle->GetXMLString( datetimeXML, XSO_Time_Local);
			IHelperMetadatas::DateTimeFromXMLToIPTC( datetimeXML, dateIPTC, timeIPTC);
			
			_FromVValueSingleToCFDictionnaryKeyValuePair(metaIPTC,	kCGImagePropertyIPTCExpirationDate,
														 static_cast<const VValueSingle *>(&dateIPTC));
			_FromVValueSingleToCFDictionnaryKeyValuePair(metaIPTC,	kCGImagePropertyIPTCExpirationTime,
														 static_cast<const VValueSingle *>(&timeIPTC));
		}
		
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCSpecialInstructions,
														*bagMetaIPTC,	ImageMetaIPTC::SpecialInstructions);
#if ENABLE_ALL_METAS
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCActionAdvised,
														*bagMetaIPTC,	ImageMetaIPTC::ActionAdvised);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCReferenceService,
														*bagMetaIPTC,	ImageMetaIPTC::ReferenceService, TT_ARRAY_STRING);
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
				IHelperMetadatas::DateTimeFromXMLToIPTC( *it, dateIPTC, timeIPTC);
				if (datesIPTC.IsEmpty())
					datesIPTC = dateIPTC;
				else
				{
					datesIPTC.AppendUniChar(';');
					datesIPTC.AppendString( dateIPTC);
				}
			}
			//write ImageIO key-value pair
			_FromVValueSingleToCFDictionnaryKeyValuePair(metaIPTC,	kCGImagePropertyIPTCReferenceDate,
														 static_cast<const VValueSingle *>(&datesIPTC),
														 TT_ARRAY_STRING);
			
		}
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCReferenceNumber,
														*bagMetaIPTC,	ImageMetaIPTC::ReferenceNumber, TT_ARRAY_STRING);
#endif
		if (bagMetaIPTC->AttributeExists( ImageMetaIPTC::DateTimeCreated))
		{
			//convert to IPTC date and time tags
			VString dateIPTC, timeIPTC, datetimeXML;
			const VValueSingle *valueSingle = bagMetaIPTC->GetAttribute( ImageMetaIPTC::DateTimeCreated);
			valueSingle->GetXMLString( datetimeXML, XSO_Time_Local);
			IHelperMetadatas::DateTimeFromXMLToIPTC( datetimeXML, dateIPTC, timeIPTC);
			
			_FromVValueSingleToCFDictionnaryKeyValuePair(metaIPTC,	kCGImagePropertyIPTCDateCreated,
														 static_cast<const VValueSingle *>(&dateIPTC));
			_FromVValueSingleToCFDictionnaryKeyValuePair(metaIPTC,	kCGImagePropertyIPTCTimeCreated,
														 static_cast<const VValueSingle *>(&timeIPTC));
		}
		if (bagMetaIPTC->AttributeExists( ImageMetaIPTC::DigitalCreationDateTime))
		{
			//convert to IPTC date and time tags
			VString dateIPTC, timeIPTC, datetimeXML;
			const VValueSingle *valueSingle = bagMetaIPTC->GetAttribute( ImageMetaIPTC::DigitalCreationDateTime);
			valueSingle->GetXMLString( datetimeXML, XSO_Time_Local);
			IHelperMetadatas::DateTimeFromXMLToIPTC( datetimeXML, dateIPTC, timeIPTC);
			
			_FromVValueSingleToCFDictionnaryKeyValuePair(metaIPTC,	kCGImagePropertyIPTCDigitalCreationDate,
														 static_cast<const VValueSingle *>(&dateIPTC));
			_FromVValueSingleToCFDictionnaryKeyValuePair(metaIPTC,	kCGImagePropertyIPTCDigitalCreationTime,
														 static_cast<const VValueSingle *>(&timeIPTC));
		}
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCOriginatingProgram,
														*bagMetaIPTC,	ImageMetaIPTC::OriginatingProgram);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCProgramVersion,
														*bagMetaIPTC,	ImageMetaIPTC::ProgramVersion);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCObjectCycle,
														*bagMetaIPTC,	ImageMetaIPTC::ObjectCycle);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCByline,
														*bagMetaIPTC,	ImageMetaIPTC::Byline, TT_ARRAY_STRING);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCBylineTitle,
														*bagMetaIPTC,	ImageMetaIPTC::BylineTitle, TT_ARRAY_STRING);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCCity,
														*bagMetaIPTC,	ImageMetaIPTC::City);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCSubLocation,
														*bagMetaIPTC,	ImageMetaIPTC::SubLocation);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCProvinceState,
														*bagMetaIPTC,	ImageMetaIPTC::ProvinceState);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCCountryPrimaryLocationCode,
														*bagMetaIPTC,	ImageMetaIPTC::CountryPrimaryLocationCode);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCCountryPrimaryLocationName,
														*bagMetaIPTC,	ImageMetaIPTC::CountryPrimaryLocationName);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCOriginalTransmissionReference,
														*bagMetaIPTC,	ImageMetaIPTC::OriginalTransmissionReference);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCHeadline,
														*bagMetaIPTC,	ImageMetaIPTC::Headline);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCCredit,
														*bagMetaIPTC,	ImageMetaIPTC::Credit);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCSource,
														*bagMetaIPTC,	ImageMetaIPTC::Source);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCCopyrightNotice,
														*bagMetaIPTC,	ImageMetaIPTC::CopyrightNotice);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCContact,
														*bagMetaIPTC,	ImageMetaIPTC::Contact, TT_ARRAY_STRING);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCCaptionAbstract,
														*bagMetaIPTC,	ImageMetaIPTC::CaptionAbstract);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCWriterEditor,
														*bagMetaIPTC,	ImageMetaIPTC::WriterEditor, TT_ARRAY_STRING);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCImageType,
														*bagMetaIPTC,	ImageMetaIPTC::ImageType);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCImageOrientation,
														*bagMetaIPTC,	ImageMetaIPTC::ImageOrientation);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCLanguageIdentifier,
														*bagMetaIPTC,	ImageMetaIPTC::LanguageIdentifier);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaIPTC,		kCGImagePropertyIPTCStarRating,
														*bagMetaIPTC,	ImageMetaIPTC::StarRating);
		if (CFDictionaryGetCount(metaIPTC) > 0)
			CFDictionarySetValue(ioProps, kCGImagePropertyIPTCDictionary, metaIPTC);
		else
		{
			if (inEncodeImageFrames)
				CFDictionaryRemoveValue(ioProps, kCGImagePropertyIPTCDictionary);
			else
				CFDictionarySetValue(ioProps, kCGImagePropertyIPTCDictionary, kCFNull);
		}
		CFRelease(metaIPTC);
	}
	
	
	//write GPS metadatas 
	const VValueBag *bagMetaGPS = inBagMetas->GetUniqueElement( ImageMetaGPS::GPS);
	if (bagMetaGPS)
	{
		CFDictionaryRef metaSrc = NULL;
		CFMutableDictionaryRef metaGPS = NULL;
		if (CFDictionaryGetValueIfPresent(ioProps, kCGImagePropertyGPSDictionary, (const void**)&metaSrc))
			metaGPS = CFDictionaryCreateMutableCopy( kCFAllocatorDefault, 0, metaSrc);
		else
			metaGPS = CFDictionaryCreateMutable( kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
		
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSVersion,
														*bagMetaGPS,	ImageMetaGPS::VersionID,
														TT_GPSVERSIONID);
		if (bagMetaGPS->AttributeExists( ImageMetaGPS::Latitude)) 
		{
			//convert to GPS latitude+latitudeRef 
			VString latitudeXMP;
			bagMetaGPS->GetString( ImageMetaGPS::Latitude, latitudeXMP);
			VString latitude, latitudeRef;
			IHelperMetadatas::GPSCoordsFromXMPToExif( latitudeXMP, latitude, latitudeRef);
			
			_FromVValueSingleToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSLatitudeRef,
														 static_cast<const VValueSingle *>(&latitudeRef));
			_FromVValueSingleToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSLatitude,
														 static_cast<const VValueSingle *>(&latitude), TT_REAL);
		}
		if (bagMetaGPS->AttributeExists( ImageMetaGPS::Longitude))
		{
			//convert to GPS longitude+longitudeRef 
			VString longitudeXMP;
			bagMetaGPS->GetString( ImageMetaGPS::Longitude, longitudeXMP);
			VString longitude, longitudeRef;
			IHelperMetadatas::GPSCoordsFromXMPToExif( longitudeXMP, longitude, longitudeRef);
			
			_FromVValueSingleToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSLongitudeRef,
														 static_cast<const VValueSingle *>(&longitudeRef));
			_FromVValueSingleToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSLongitude,
														 static_cast<const VValueSingle *>(&longitude), TT_REAL);
		}
		
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSAltitudeRef,
														*bagMetaGPS,	ImageMetaGPS::AltitudeRef,
														TT_INT);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSAltitude,
														*bagMetaGPS,	ImageMetaGPS::Altitude,
														TT_REAL);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSSatellites,
														*bagMetaGPS,	ImageMetaGPS::Satellites);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSStatus,
														*bagMetaGPS,	ImageMetaGPS::Status);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSMeasureMode,
														*bagMetaGPS,	ImageMetaGPS::MeasureMode);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSDOP,
														*bagMetaGPS,	ImageMetaGPS::DOP,
														TT_REAL);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSSpeedRef,
														*bagMetaGPS,	ImageMetaGPS::SpeedRef);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSSpeed,
														*bagMetaGPS,	ImageMetaGPS::Speed,
														TT_REAL);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSTrackRef,
														*bagMetaGPS,	ImageMetaGPS::TrackRef);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSTrack,
														*bagMetaGPS,	ImageMetaGPS::Track,
														TT_REAL);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSImgDirectionRef,
														*bagMetaGPS,	ImageMetaGPS::ImgDirectionRef);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSImgDirection,
														*bagMetaGPS,	ImageMetaGPS::ImgDirection,
														TT_REAL);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSMapDatum,
														*bagMetaGPS,	ImageMetaGPS::MapDatum);
		if (bagMetaGPS->AttributeExists( ImageMetaGPS::DestLatitude))
		{
			//convert to GPS latitude+latitudeRef 
			VString latitudeXMP;
			bagMetaGPS->GetString( ImageMetaGPS::DestLatitude, latitudeXMP);
			VString latitude, latitudeRef;
			IHelperMetadatas::GPSCoordsFromXMPToExif( latitudeXMP, latitude, latitudeRef);
			
			_FromVValueSingleToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSDestLatitudeRef,
														 static_cast<const VValueSingle *>(&latitudeRef));
			_FromVValueSingleToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSDestLatitude,
														 static_cast<const VValueSingle *>(&latitude), TT_REAL);
		}
		if (bagMetaGPS->AttributeExists( ImageMetaGPS::DestLongitude))
		{
			//convert to GPS longitude+longitudeRef 
			VString longitudeXMP;
			bagMetaGPS->GetString( ImageMetaGPS::DestLongitude, longitudeXMP);
			VString longitude, longitudeRef;
			IHelperMetadatas::GPSCoordsFromXMPToExif( longitudeXMP, longitude, longitudeRef);
			
			_FromVValueSingleToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSDestLongitudeRef,
														 static_cast<const VValueSingle *>(&longitudeRef));
			_FromVValueSingleToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSDestLongitude,
														 static_cast<const VValueSingle *>(&longitude), TT_REAL);
		}
		
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSDestBearingRef,
														*bagMetaGPS,	ImageMetaGPS::DestBearingRef);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSDestBearing,
														*bagMetaGPS,	ImageMetaGPS::DestBearing,
														TT_REAL);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSDestDistanceRef,
														*bagMetaGPS,	ImageMetaGPS::DestDistanceRef);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSDestDistance,
														*bagMetaGPS,	ImageMetaGPS::DestDistance,
														TT_REAL);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSProcessingMethod,
														*bagMetaGPS,	ImageMetaGPS::ProcessingMethod);
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSAreaInformation,
														*bagMetaGPS,	ImageMetaGPS::AreaInformation);
		if (bagMetaGPS->AttributeExists( ImageMetaGPS::DateTime))
		{
			//convert to GPS date+time
			
			VString datetimeXML, dateGPS, timeGPS;
			const VValueSingle *valueSingle = bagMetaGPS->GetAttribute( ImageMetaGPS::DateTime);
			if (valueSingle->GetValueKind() == VK_TIME)
				valueSingle->GetXMLString( datetimeXML, XSO_Time_UTC);
			else
				valueSingle->GetString( datetimeXML);
			
			IHelperMetadatas::DateTimeFromXMLToGPS( datetimeXML, dateGPS, timeGPS);
			
			_FromVValueSingleToCFDictionnaryKeyValuePair(metaGPS,	kCGImagePropertyGPSDateStamp,
														 static_cast<const VValueSingle *>(&dateGPS));
			_FromVValueSingleToCFDictionnaryKeyValuePair(metaGPS,	kCGImagePropertyGPSTimeStamp,
														 static_cast<const VValueSingle *>(&timeGPS));
		}
		_FromBagKeyValuePairToCFDictionnaryKeyValuePair(metaGPS,		kCGImagePropertyGPSDifferental,
														*bagMetaGPS,	ImageMetaGPS::Differential,
														TT_INT);
		
		if (CFDictionaryGetCount(metaGPS) > 0)
			CFDictionarySetValue(ioProps, kCGImagePropertyGPSDictionary, metaGPS);
		else
		{
			if (inEncodeImageFrames)
				CFDictionaryRemoveValue(ioProps, kCGImagePropertyGPSDictionary);
			else
				CFDictionarySetValue(ioProps, kCGImagePropertyGPSDictionary, kCFNull);
		}
		CFRelease(metaGPS);
	}
}

/** convert from metadata bag key-value pair to CFDictionnary metadata key-value pair */
bool VPictureCodec_ImageIO::_FromBagKeyValuePairToCFDictionnaryKeyValuePair(	CFMutableDictionaryRef ioCFDict, 
																				CFStringRef inCFDictKey, 
																				const VValueBag& inBag, 
																				const XBOX::VValueBag::StKey& inBagKey,
																				eMetaTagType inTagType)
{
	const VValueSingle *valueSingle = inBag.GetAttribute( inBagKey);
	if (!valueSingle)
		return false;

	if (inTagType == TT_BLOB) //undefined type tags are encoded in bag as blobs
	{
		if (valueSingle->GetValueKind() == VK_STRING)
		{
			if (static_cast<const VString *>(valueSingle)->IsEmpty())
			{
				//if value is a empty string, remove metadata
				CTHelper::SetAttribute( ioCFDict, inCFDictKey, kCFNull);
				return true;
			}
		}

		//get blob as CFArray
		CFMutableArrayRef arrayValues;	
		bool ok = IHelperMetadatas::GetCFArrayChar( &inBag, inBagKey, arrayValues);
		if (ok)
		{
			//set dictionnary key/value pair
			CFDictionarySetValue(ioCFDict, inCFDictKey, arrayValues);
			CFRelease(arrayValues);
			return true;
		}
		return false;
	}
	
	return _FromVValueSingleToCFDictionnaryKeyValuePair( ioCFDict,
														 inCFDictKey,
														 valueSingle,
														 inTagType);
}


/** convert a VValueBag attribute to a CFDictionnary key-value pair */
bool VPictureCodec_ImageIO::_FromVValueSingleToCFDictionnaryKeyValuePair(	CFMutableDictionaryRef ioCFDict, 
																			CFStringRef inCFDictKey, 
																			const VValueSingle *att,																			
																			eMetaTagType inTagType)
{
	if (att->GetValueKind() == VK_STRING)
	{
		if (static_cast<const VString *>(att)->IsEmpty())
		{
			//if value is a empty string, remove metadata
			CTHelper::SetAttribute( ioCFDict, inCFDictKey, kCFNull);
			return true;
		}
	}

	xbox_assert(att);
	if (inTagType == TT_EXIF_DATETIME)
	{
		//get XML UTC datetime
		VString datetimeXML;
		if (att->GetValueKind() == VK_TIME)
			att->GetXMLString( datetimeXML, XSO_Time_UTC);
		else
			att->GetString( datetimeXML);
		
		//convert to Exif datetime
		VString valueString;
		IHelperMetadatas::DateTimeFromXMLToExif( datetimeXML, valueString);
		
		CTHelper::SetAttribute( ioCFDict, inCFDictKey, valueString);
		return true;
	}
	else if (inTagType == TT_INT)
	{
		//value is of type integer: store as sInt32 
		CTHelper::SetAttribute( ioCFDict, inCFDictKey, (int32_t) att->GetLong());
		return true;
	}
	else if (inTagType == TT_REAL)
	{
		//value is of type float: store as double 
		CTHelper::SetAttribute( ioCFDict, inCFDictKey, (GReal) att->GetReal());
		return true;
	}
	else if (inTagType == TT_STRING)
	{
		//value is of type string: store as CFString 
		VString text;
		att->GetString( text);
		CTHelper::SetAttribute( ioCFDict, inCFDictKey, text);
		return true;
	}
	else if (att->GetValueKind() == VK_STRING)
	{
		const VString *text = static_cast<const VString *>(att);
		xbox_assert(text);
		{
			//value is composite: 
			//store in dest dictionnary as a array of typed values
			VectorOfVString items;  
			if (inTagType == TT_EXIF_VERSION)
			{
				//XMP exif version (for instance "0210"): extract version numbers from string
				
				if (text->GetLength() >= 4)
				{
					int start = 1, end = 4; 
					if (text->GetUniChar(1) == '0' && text->GetUniChar(4) == '0')
					{
						start = 2;
						end = 3;
					}
					for (int i = start; i <= end; i++)
					{
						VString temp;
						temp.AppendUniChar(text->GetUniChar(i));
						items.push_back( temp);
					}
				}
				else
					return false;
			}
			else
				//attribute value string is a list of string-formatted values: 				
				text->GetSubStrings(inTagType == TT_GPSVERSIONID ? '.' : ';',
									items);
			if (items.size() > 0)
			{
				CFMutableArrayRef arrayItems = CFArrayCreateMutable( kCFAllocatorDefault, items.size(), &kCFTypeArrayCallBacks);
				switch (inTagType)
				{
					case TT_EXIF_VERSION:						
					case TT_GPSVERSIONID:	
					case TT_ARRAY_INT:
					{
						//store as a array of sInt32 values
						for (int i = 0; i < items.size(); i++)
						{
							int32_t value = (int32_t)items[i].GetLong();
							CFNumberRef number = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &value);
							CFArrayAppendValue( arrayItems, number);
							CFRelease(number);
						}
					}
					break;
						
					case TT_ARRAY_REAL:
					{
						//store as a array of double values
						for (int i = 0; i < items.size(); i++)
						{
							Real value = items[i].GetReal();
							CFNumberRef number = CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &value);
							CFArrayAppendValue( arrayItems, number);
							CFRelease(number);
						}
					}
					break;
						
					default:
					{
						//store as a array of strings
						for (int i = 0; i < items.size(); i++)
						{
							CFStringRef textCF = items[i].MAC_RetainCFStringCopy();
							CFArrayAppendValue( arrayItems, textCF);
							CFRelease(textCF);
						}
					}
					break;
				}
				CFDictionarySetValue(ioCFDict, inCFDictKey, arrayItems);
				CFRelease(arrayItems);
			}
		}
		return true;
	}
	return false;
}

/** create CoreGraphics image from input data provider */
CGImageRef VPictureCodec_ImageIO::Decode( VPictureDataProvider& inDataProvider, VValueBag *outMetadatas)
{
	stImageSource imageSource( inDataProvider);
	if (imageSource.IsValid())
	{
		CGImageSourceRef imageSourceRef = imageSource.RetainImageSourceRef();
		if (imageSourceRef)
		{
			CGImageRef image = CGImageSourceCreateImageAtIndex(imageSourceRef, 0, NULL);
			CFRelease(imageSourceRef);
			
			//copy metadatas
			if (outMetadatas)
				imageSource.ReadMetadatas(outMetadatas);
			
			return image;
		}
	}
	return NULL;
}


/** create CoreGraphics image from input file */
CGImageRef VPictureCodec_ImageIO::Decode( const VFile& inFile, VValueBag *outMetadatas)
{
	if (!inFile.Exists())
		return NULL;
	
	VPictureDataProvider *dataProvider = VPictureDataProvider::Create( inFile);
	if (dataProvider)
	{
		CGImageRef image = Decode(*dataProvider, outMetadatas);
		delete dataProvider;
		return image;
	}
	return NULL;
}


/** get metadatas from input data provider */
void VPictureCodec_ImageIO::GetMetadatas( VPictureDataProvider& inDataProvider, VValueBag *outMetadatas)
{
	xbox_assert(outMetadatas);
	
	stImageSource imageSource( inDataProvider);
	if (imageSource.IsValid())
	{
		//copy metadatas
		imageSource.ReadMetadatas(outMetadatas);
	}
}


stImageSource::stImageSource(VPictureDataProvider& inDataProvider, bool inGetProperties)
{
	fImageSource = NULL;
	fProperties = NULL;
	if (!inDataProvider.IsValid())
		return;
	
	//create image data buffer from data provider
	CGDataProviderRef dataProvider = xV4DPicture_MemoryDataProvider::CGDataProviderCreate(&inDataProvider);
	
	//create ImageIO source reference
	fImageSource = CGImageSourceCreateWithDataProvider(dataProvider, NULL);
	if (fImageSource == NULL || CGImageSourceGetStatus(fImageSource) != kCGImageStatusComplete || CGImageSourceGetCount(fImageSource) < 1)
	{
		if (fImageSource)
			CFRelease(fImageSource);
		fImageSource = NULL;
		
		CFRelease(dataProvider);
		return;
	}
	
	if (inGetProperties)
		fProperties = CGImageSourceCopyPropertiesAtIndex( fImageSource, 0, NULL);
	
	CFRelease(dataProvider);
}

stImageSource::~stImageSource()
{
	if (fProperties)
		CFRelease(fProperties);
	fProperties = NULL;
	if (fImageSource)
		CFRelease(fImageSource);
	fImageSource = NULL;
}


/** retain image source reference */
CGImageSourceRef stImageSource::RetainImageSourceRef() 
{
	if (fImageSource == NULL)
		return NULL;
	
	CFRetain(fImageSource);
	return fImageSource;
}


/** true if image data provider is valid 
 @remarks
 false if source data provider is not a valid image
 */
bool stImageSource::IsValid() const
{
	return fImageSource != NULL;
}

/** get image width */
uLONG stImageSource::GetWidth() const
{
	if (fProperties == NULL)
		return 0;
	
	uLONG width = 0;
	CFNumberRef number = (CFNumberRef)CFDictionaryGetValue(fProperties, kCGImagePropertyPixelWidth);
	if (number)
		CFNumberGetValue(number, kCFNumberLongType, &width);
	return width;
}

/** get image height */
uLONG stImageSource::GetHeight() const
{
	if (fProperties == NULL)
		return 0;
	
	uLONG height = 0;
	CFNumberRef number = (CFNumberRef)CFDictionaryGetValue(fProperties, kCGImagePropertyPixelHeight);
	if (number)
		CFNumberGetValue(number, kCFNumberLongType, &height);
	return height;
}


/** get image color bit depth */
uLONG stImageSource::GetBitDepth() const
{
	if (fProperties == NULL)
		return 0;
	
	uLONG depth = 0;
	CFNumberRef number = (CFNumberRef)CFDictionaryGetValue(fProperties, kCGImagePropertyDepth);
	if (number)
		CFNumberGetValue(number, kCFNumberLongType, &depth);
	return depth;
}

/** true if image has alpha channel */
bool stImageSource::hasAlpha() const
{
	if (fProperties == NULL)
		return false;
	
	return CFDictionaryGetValue(fProperties, kCGImagePropertyHasAlpha) == kCFBooleanTrue;
}


/** copy image metadatas to the provided prop container
 @remarks
	copy all image metadatas and not only the ones supported in ImageMeta.h
 */
void stImageSource::CopyMetadatas(CFMutableDictionaryRef ioProps)
{
	if (fProperties == NULL)
		return;
	
	//first copy metadatas which are supported in ImageMeta.h (and so modifiable and serializable in VValueBag)
	CFDictionaryRef metas = NULL;
	if (CFDictionaryGetValueIfPresent(fProperties, kCGImagePropertyIPTCDictionary, (const void**)&metas))
		CFDictionarySetValue( ioProps, kCGImagePropertyIPTCDictionary, metas);
	
	if (CFDictionaryGetValueIfPresent(fProperties, kCGImagePropertyTIFFDictionary, (const void**)&metas))
		CFDictionarySetValue( ioProps, kCGImagePropertyTIFFDictionary, metas);
	
	if (CFDictionaryGetValueIfPresent(fProperties, kCGImagePropertyExifDictionary, (const void**)&metas))
		CFDictionarySetValue( ioProps, kCGImagePropertyExifDictionary, metas);
	
	if (CFDictionaryGetValueIfPresent(fProperties, kCGImagePropertyGPSDictionary, (const void**)&metas))
		CFDictionarySetValue( ioProps, kCGImagePropertyGPSDictionary, metas);
	
	//copy also metadatas which are not supported in ImageMeta.h (in order to preserve all metadatas even those which are not modifiable)
	if (CFDictionaryGetValueIfPresent(fProperties, kCGImagePropertyGIFDictionary, (const void**)&metas))
		CFDictionarySetValue( ioProps, kCGImagePropertyGIFDictionary, metas);

	if (CFDictionaryGetValueIfPresent(fProperties, kCGImagePropertyJFIFDictionary, (const void**)&metas))
		CFDictionarySetValue( ioProps, kCGImagePropertyJFIFDictionary, metas);
	
	if (CFDictionaryGetValueIfPresent(fProperties, kCGImagePropertyPNGDictionary, (const void**)&metas))
		CFDictionarySetValue( ioProps, kCGImagePropertyPNGDictionary, metas);
	
	if (CFDictionaryGetValueIfPresent(fProperties, kCGImagePropertyRawDictionary, (const void**)&metas))
		CFDictionarySetValue( ioProps, kCGImagePropertyRawDictionary, metas);
	
	if (CFDictionaryGetValueIfPresent(fProperties, kCGImagePropertyCIFFDictionary, (const void**)&metas))
		CFDictionarySetValue( ioProps, kCGImagePropertyCIFFDictionary, metas);
	
	if (CFDictionaryGetValueIfPresent(fProperties, kCGImagePropertyMakerCanonDictionary, (const void**)&metas))
		CFDictionarySetValue( ioProps, kCGImagePropertyMakerCanonDictionary, metas);
	
	if (CFDictionaryGetValueIfPresent(fProperties, kCGImagePropertyMakerNikonDictionary, (const void**)&metas))
		CFDictionarySetValue( ioProps, kCGImagePropertyMakerNikonDictionary, metas);
	
	if (CFDictionaryGetValueIfPresent(fProperties, kCGImageProperty8BIMDictionary, (const void**)&metas))
		CFDictionarySetValue( ioProps, kCGImageProperty8BIMDictionary, metas);
	
	if (CFDictionaryGetValueIfPresent(fProperties, kCGImagePropertyExifAuxDictionary, (const void**)&metas))
		CFDictionarySetValue( ioProps, kCGImagePropertyExifAuxDictionary, metas);
	
	//TODO: following dictionnaries seem not to be implemented in 10.5 although doc tell it is ???
	//		(using these tags throw compilation errors so disable it for now: can uncomment when fix is available)
	
	//	if (CFDictionaryGetValueIfPresent(fProperties, kCGImagePropertyMakerMinoltaDictionary, (const void**)&metas))
	//		CFDictionarySetValue( ioProps, kCGImagePropertyMakerMinoltaDictionary, metas);
	
	//	if (CFDictionaryGetValueIfPresent(fProperties, kCGImagePropertyMakerFujiDictionary, (const void**)&metas))
	//		CFDictionarySetValue( ioProps, kCGImagePropertyMakerFujiDictionary, metas);
	
	//	if (CFDictionaryGetValueIfPresent(fProperties, kCGImagePropertyMakerOlympusDictionary, (const void**)&metas))
	//		CFDictionarySetValue( ioProps, kCGImagePropertyMakerOlympusDictionary, metas);
	
	//	if (CFDictionaryGetValueIfPresent(fProperties, kCGImagePropertyMakerPentaxDictionary, (const void**)&metas))
	//		CFDictionarySetValue( ioProps, kCGImagePropertyMakerPentaxDictionary, metas);
	
	//	if (CFDictionaryGetValueIfPresent(fProperties, kCGImagePropertyDNGDictionary, (const void**)&metas))
	//		CFDictionarySetValue( ioProps, kCGImagePropertyDNGDictionary, metas);
}	


/** copy image metadatas to the metadata bag 
 @remarks
	bag value type is determined from metadata tag type as follow:
	ASCII tag value <-> VString bag value if repeat count is 1, otherwise encode tag string values in a VString using ';' as separator
	UNDEFINED, BYTE, SHORT, LONG or SLONG tag value <-> VLong bag value if repeat count is 1, otherwise encode tag values in a VString using ';' as separator
	RATIONAL, SRATIONAL tag value <-> VReal bag value if repeat count is 1, otherwise encode tag values in a VString using ';' as separator
*/
void stImageSource::ReadMetadatas(VValueBag *ioValueBag)
{
 	if (fProperties == NULL || ioValueBag == NULL)
		return;
	
#if TEST_METADATAS
	//create manually default metas bag
	{
		ImageMeta::stWriter writer( ioValueBag);
		
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
		
		writer.SetIPTCObjectAttributeReference( "001:attribute reference");
		
		writer.SetIPTCObjectCycle();
		
		writer.SetIPTCObjectName( "object name");
		
		writer.SetIPTCObjectTypeReference( "object type ref");
		
		writer.SetIPTCOriginalTransmissionReference( "original transmission ref");
		
		writer.SetIPTCOriginatingProgram( "originating program");
		
		writer.SetIPTCProgramVersion( "program version");
		
		writer.SetIPTCProvinceState( "province state");
		
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
		
		writer.SetIPTCReleaseDateTime( time);
		
		std::vector<ImageMetaIPTC::eScene> scenes;
		scenes.push_back( ImageMetaIPTC::kScene_ExteriorView);
		scenes.push_back( ImageMetaIPTC::kScene_PanoramicView);
		writer.SetIPTCScene( scenes);
		
		writer.SetIPTCSource( "source");
		
		writer.SetIPTCSpecialInstructions( "special instructions");
		
		writer.SetIPTCStarRating( 4);
		
		VectorOfVString subjectrefs;
		subjectrefs.push_back( "subjectref1");
		subjectrefs.push_back( "subjectref2");
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
		
		writer.SetTIFFPrimaryChromaticities();
		
		writer.SetTIFFSoftware( "software");
		
		writer.SetTIFFWhitePoint();
		
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
		{
			VBlobWithPtr blob;
			char buffer[] = "CFAPattern";
			blob.PutData( buffer, sizeof(buffer));
			writer.SetEXIFCFAPattern( blob);
		}
		
		writer.SetEXIFColorSpace();
		
		writer.SetEXIFComponentsConfiguration();
		
		writer.SetEXIFCompressedBitsPerPixel( 1.6);
		writer.SetEXIFContrast( ImageMetaEXIF::kPower_Normal);
		writer.SetEXIFCustomRendered( true);
		writer.SetEXIFDateTimeDigitized( time);
		writer.SetEXIFDateTimeOriginal( time);
		
		{
			VBlobWithPtr blob;
			char buffer[] = "DeviceSettingDescription";
			blob.PutData( buffer, sizeof(buffer));
			writer.SetEXIFDeviceSettingDescription( blob);
		}
		
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
		{
			VBlobWithPtr blob;
			char buffer[] = "OECF";
			blob.PutData( buffer, sizeof(buffer));
			writer.SetEXIFOECF( blob);
		}
		writer.SetEXIFPixelXDimension( 1024);
		writer.SetEXIFPixelYDimension( 768);
		writer.SetEXIFRelatedSoundFile( "related sound file");
		writer.SetEXIFSaturation( ImageMetaEXIF::kPower_Low);
		writer.SetEXIFSceneCaptureType();
		writer.SetEXIFSceneType( true);
		writer.SetEXIFSensingMethod();
		writer.SetEXIFSharpness( ImageMetaEXIF::kPower_High);
		writer.SetEXIFShutterSpeedValue( 5.0);
		{
			VBlobWithPtr blob;
			char buffer[] = "SpatialFrequencyResponse";
			blob.PutData( buffer, sizeof(buffer));
			writer.SetEXIFSpatialFrequencyResponse( blob);
		}
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
		writer.SetGPSProcessingMethod("ProcessingMethod");
		writer.SetGPSSatellites( "satellites");
		writer.SetGPSSpeed( 30);
		writer.SetGPSStatus();
		writer.SetGPSTrack( 100.0);
		writer.SetGPSVersionID();
		return;
	}
#endif
	
	
	//read IPTC metadatas 
	CFDictionaryRef metaIPTC = NULL;
	if (CFDictionaryGetValueIfPresent(fProperties, kCGImagePropertyIPTCDictionary, (const void**)&metaIPTC))
	{
		VValueBag *bagMetaIPTC = new VValueBag();
#if ENABLE_ALL_METAS
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCObjectTypeReference,
														*bagMetaIPTC,	ImageMetaIPTC::ObjectTypeReference);
#endif
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCObjectAttributeReference,
														*bagMetaIPTC,	ImageMetaIPTC::ObjectAttributeReference);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCObjectName,
														*bagMetaIPTC,	ImageMetaIPTC::ObjectName);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCEditStatus,
														*bagMetaIPTC,	ImageMetaIPTC::EditStatus);
#if ENABLE_ALL_METAS
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCEditorialUpdate,
														*bagMetaIPTC,	ImageMetaIPTC::EditorialUpdate);
#endif
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCUrgency,
														*bagMetaIPTC,	ImageMetaIPTC::Urgency);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCSubjectReference,
														*bagMetaIPTC,	ImageMetaIPTC::SubjectReference);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCCategory,
														*bagMetaIPTC,	ImageMetaIPTC::Category);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCSupplementalCategory,
														*bagMetaIPTC,	ImageMetaIPTC::SupplementalCategory);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCFixtureIdentifier,
														*bagMetaIPTC,	ImageMetaIPTC::FixtureIdentifier);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCKeywords,
														*bagMetaIPTC,	ImageMetaIPTC::Keywords);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCContentLocationCode,
														*bagMetaIPTC,	ImageMetaIPTC::ContentLocationCode);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCContentLocationName,
														*bagMetaIPTC,	ImageMetaIPTC::ContentLocationName);
		bool datetime = false;
		{
			datetime =		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(	metaIPTC, kCGImagePropertyIPTCReleaseDate, 
																				*bagMetaIPTC, ImageMetaIPTC::ReleaseDate);
			datetime =		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(	metaIPTC, kCGImagePropertyIPTCReleaseTime, 
																				*bagMetaIPTC, ImageMetaIPTC::ReleaseTime) || datetime;
			if (datetime)
			{
				//convert to XML datetime
				
				VString dateIPTC, timeIPTC, datetimeXML;
				bagMetaIPTC->GetString( ImageMetaIPTC::ReleaseDate, dateIPTC);
				bagMetaIPTC->GetString( ImageMetaIPTC::ReleaseTime, timeIPTC);
				IHelperMetadatas::DateTimeFromIPTCToXML( dateIPTC, timeIPTC, datetimeXML);
				
				bagMetaIPTC->SetString( ImageMetaIPTC::ReleaseDateTime, datetimeXML);
				bagMetaIPTC->RemoveAttribute( ImageMetaIPTC::ReleaseDate);
				bagMetaIPTC->RemoveAttribute( ImageMetaIPTC::ReleaseTime);
			}
		}
		
		
		{
			datetime = _FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCExpirationDate,
																	   *bagMetaIPTC,	ImageMetaIPTC::ExpirationDate);
			datetime = _FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCExpirationTime,
																	   *bagMetaIPTC,	ImageMetaIPTC::ExpirationTime) || datetime;
			if (datetime)
			{
				//convert to XML datetime
				
				VString dateIPTC, timeIPTC, datetimeXML;
				bagMetaIPTC->GetString( ImageMetaIPTC::ExpirationDate, dateIPTC);
				bagMetaIPTC->GetString( ImageMetaIPTC::ExpirationTime, timeIPTC);
				IHelperMetadatas::DateTimeFromIPTCToXML( dateIPTC, timeIPTC, datetimeXML);
				
				bagMetaIPTC->SetString( ImageMetaIPTC::ExpirationDateTime, datetimeXML);
				bagMetaIPTC->RemoveAttribute( ImageMetaIPTC::ExpirationDate);
				bagMetaIPTC->RemoveAttribute( ImageMetaIPTC::ExpirationTime);
			}
		}
		
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCSpecialInstructions,
														*bagMetaIPTC,	ImageMetaIPTC::SpecialInstructions);
#if ENABLE_ALL_METAS
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCActionAdvised,
														*bagMetaIPTC,	ImageMetaIPTC::ActionAdvised);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCReferenceService,
														*bagMetaIPTC,	ImageMetaIPTC::ReferenceService);
		{
			datetime = _FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCReferenceDate,
																	   *bagMetaIPTC,	ImageMetaIPTC::ReferenceDate);
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
					IHelperMetadatas::DateTimeFromIPTCToXML( *it, VString(""), datetimeXML);
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
		
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCReferenceNumber,
														*bagMetaIPTC,	ImageMetaIPTC::ReferenceNumber);
#endif
		{
			datetime = _FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCDateCreated,
																	   *bagMetaIPTC,	ImageMetaIPTC::DateCreated);
			datetime = _FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCTimeCreated,
																	   *bagMetaIPTC,	ImageMetaIPTC::TimeCreated) || datetime;
			if (datetime)
			{
				//convert to XML datetime
				
				VString dateIPTC, timeIPTC, datetimeXML;
				bagMetaIPTC->GetString( ImageMetaIPTC::DateCreated, dateIPTC);
				bagMetaIPTC->GetString( ImageMetaIPTC::TimeCreated, timeIPTC);
				IHelperMetadatas::DateTimeFromIPTCToXML( dateIPTC, timeIPTC, datetimeXML);
				
				bagMetaIPTC->SetString( ImageMetaIPTC::DateTimeCreated, datetimeXML);
				bagMetaIPTC->RemoveAttribute( ImageMetaIPTC::DateCreated);
				bagMetaIPTC->RemoveAttribute( ImageMetaIPTC::TimeCreated);
			}
		}
		
		{
			datetime = _FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCDigitalCreationDate,
																	   *bagMetaIPTC,	ImageMetaIPTC::DigitalCreationDate);
			datetime = _FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCDigitalCreationTime,
																	   *bagMetaIPTC,	ImageMetaIPTC::DigitalCreationTime) || datetime;
			if (datetime)
			{
				//convert to XML datetime
				
				VString dateIPTC, timeIPTC, datetimeXML;
				bagMetaIPTC->GetString( ImageMetaIPTC::DigitalCreationDate, dateIPTC);
				bagMetaIPTC->GetString( ImageMetaIPTC::DigitalCreationTime, timeIPTC);
				IHelperMetadatas::DateTimeFromIPTCToXML( dateIPTC, timeIPTC, datetimeXML);
				
				bagMetaIPTC->SetString( ImageMetaIPTC::DigitalCreationDateTime, datetimeXML);
				bagMetaIPTC->RemoveAttribute( ImageMetaIPTC::DigitalCreationDate);
				bagMetaIPTC->RemoveAttribute( ImageMetaIPTC::DigitalCreationTime);
			}
		}
		
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCOriginatingProgram,
														*bagMetaIPTC,	ImageMetaIPTC::OriginatingProgram);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCProgramVersion,
														*bagMetaIPTC,	ImageMetaIPTC::ProgramVersion);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCObjectCycle,
														*bagMetaIPTC,	ImageMetaIPTC::ObjectCycle);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCByline,
														*bagMetaIPTC,	ImageMetaIPTC::Byline);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCBylineTitle,
														*bagMetaIPTC,	ImageMetaIPTC::BylineTitle);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCCity,
														*bagMetaIPTC,	ImageMetaIPTC::City);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCSubLocation,
														*bagMetaIPTC,	ImageMetaIPTC::SubLocation);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCProvinceState,
														*bagMetaIPTC,	ImageMetaIPTC::ProvinceState);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCCountryPrimaryLocationCode,
														*bagMetaIPTC,	ImageMetaIPTC::CountryPrimaryLocationCode);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCCountryPrimaryLocationName,
														*bagMetaIPTC,	ImageMetaIPTC::CountryPrimaryLocationName);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCOriginalTransmissionReference,
														*bagMetaIPTC,	ImageMetaIPTC::OriginalTransmissionReference);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCHeadline,
														*bagMetaIPTC,	ImageMetaIPTC::Headline);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCCredit,
														*bagMetaIPTC,	ImageMetaIPTC::Credit);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCSource,
														*bagMetaIPTC,	ImageMetaIPTC::Source);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCCopyrightNotice,
														*bagMetaIPTC,	ImageMetaIPTC::CopyrightNotice);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCContact,
														*bagMetaIPTC,	ImageMetaIPTC::Contact);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCCaptionAbstract,
														*bagMetaIPTC,	ImageMetaIPTC::CaptionAbstract);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCWriterEditor,
														*bagMetaIPTC,	ImageMetaIPTC::WriterEditor);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCImageType,
														*bagMetaIPTC,	ImageMetaIPTC::ImageType);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCImageOrientation,
														*bagMetaIPTC,	ImageMetaIPTC::ImageOrientation);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCLanguageIdentifier,
														*bagMetaIPTC,	ImageMetaIPTC::LanguageIdentifier);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaIPTC,		kCGImagePropertyIPTCStarRating,
														*bagMetaIPTC,	ImageMetaIPTC::StarRating);
		if (bagMetaIPTC->GetAttributesCount() > 0)
			ioValueBag->AddElement( ImageMetaIPTC::IPTC, bagMetaIPTC);
		bagMetaIPTC->Release();
	}
	
	//read TIFF metadatas 
	CFDictionaryRef metaTIFF = NULL;
	if (CFDictionaryGetValueIfPresent(fProperties, kCGImagePropertyTIFFDictionary, (const void**)&metaTIFF))
	{
		VValueBag *bagMetaTIFF = new VValueBag();
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaTIFF,		kCGImagePropertyTIFFCompression,
														*bagMetaTIFF,	ImageMetaTIFF::Compression);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaTIFF,		kCGImagePropertyTIFFPhotometricInterpretation,
														*bagMetaTIFF,	ImageMetaTIFF::PhotometricInterpretation);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaTIFF,		kCGImagePropertyTIFFDocumentName,
														*bagMetaTIFF,	ImageMetaTIFF::DocumentName);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaTIFF,		kCGImagePropertyTIFFImageDescription,
														*bagMetaTIFF,	ImageMetaTIFF::ImageDescription);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaTIFF,		kCGImagePropertyTIFFMake,
														*bagMetaTIFF,	ImageMetaTIFF::Make);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaTIFF,		kCGImagePropertyTIFFModel,
														*bagMetaTIFF,	ImageMetaTIFF::Model);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaTIFF,		kCGImagePropertyTIFFOrientation,
														*bagMetaTIFF,	ImageMetaTIFF::Orientation);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaTIFF,		kCGImagePropertyTIFFXResolution,
														*bagMetaTIFF,	ImageMetaTIFF::XResolution);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaTIFF,		kCGImagePropertyTIFFYResolution,
														*bagMetaTIFF,	ImageMetaTIFF::YResolution);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaTIFF,		kCGImagePropertyTIFFResolutionUnit,
														*bagMetaTIFF,	ImageMetaTIFF::ResolutionUnit);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaTIFF,		kCGImagePropertyTIFFSoftware,
														*bagMetaTIFF,	ImageMetaTIFF::Software);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaTIFF,		kCGImagePropertyTIFFDateTime,
														*bagMetaTIFF,	ImageMetaTIFF::DateTime,
														TT_EXIF_DATETIME);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaTIFF,		kCGImagePropertyTIFFArtist,
														*bagMetaTIFF,	ImageMetaTIFF::Artist);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaTIFF,		kCGImagePropertyTIFFHostComputer,
														*bagMetaTIFF,	ImageMetaTIFF::HostComputer);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaTIFF,		kCGImagePropertyTIFFCopyright,
														*bagMetaTIFF,	ImageMetaTIFF::Copyright);
#if ENABLE_ALL_METAS
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaTIFF,		kCGImagePropertyTIFFWhitePoint,
														*bagMetaTIFF,	ImageMetaTIFF::WhitePoint);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaTIFF,		kCGImagePropertyTIFFPrimaryChromaticities,
														*bagMetaTIFF,	ImageMetaTIFF::PrimaryChromaticities);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaTIFF,		kCGImagePropertyTIFFTransferFunction,
														*bagMetaTIFF,	ImageMetaTIFF::TransferFunction);
#endif		
		if (bagMetaTIFF->GetAttributesCount() > 0)
			ioValueBag->AddElement( ImageMetaTIFF::TIFF, bagMetaTIFF);
		bagMetaTIFF->Release();
	}
	
	//read EXIF metadatas 
	CFDictionaryRef metaEXIF = NULL;
	if (CFDictionaryGetValueIfPresent(fProperties, kCGImagePropertyExifDictionary, (const void**)&metaEXIF))
	{
		VValueBag *bagMetaEXIF = new VValueBag();
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifExposureTime,
														*bagMetaEXIF,	ImageMetaEXIF::ExposureTime);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifFNumber,
														*bagMetaEXIF,	ImageMetaEXIF::FNumber);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifExposureProgram,
														*bagMetaEXIF,	ImageMetaEXIF::ExposureProgram);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifSpectralSensitivity,
														*bagMetaEXIF,	ImageMetaEXIF::SpectralSensitivity);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifISOSpeedRatings,
														*bagMetaEXIF,	ImageMetaEXIF::ISOSpeedRatings);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifVersion,
														*bagMetaEXIF,	ImageMetaEXIF::ExifVersion,
														TT_EXIF_VERSION);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifDateTimeOriginal,
														*bagMetaEXIF,	ImageMetaEXIF::DateTimeOriginal,
														TT_EXIF_DATETIME);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifDateTimeDigitized,
														*bagMetaEXIF,	ImageMetaEXIF::DateTimeDigitized,
														TT_EXIF_DATETIME);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifComponentsConfiguration,
														*bagMetaEXIF,	ImageMetaEXIF::ComponentsConfiguration);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifCompressedBitsPerPixel,
														*bagMetaEXIF,	ImageMetaEXIF::CompressedBitsPerPixel);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifShutterSpeedValue,
														*bagMetaEXIF,	ImageMetaEXIF::ShutterSpeedValue);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifApertureValue,
														*bagMetaEXIF,	ImageMetaEXIF::ApertureValue);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifBrightnessValue,
														*bagMetaEXIF,	ImageMetaEXIF::BrightnessValue);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifExposureBiasValue,
														*bagMetaEXIF,	ImageMetaEXIF::ExposureBiasValue);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifMaxApertureValue,
														*bagMetaEXIF,	ImageMetaEXIF::MaxApertureValue);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifSubjectDistance,
														*bagMetaEXIF,	ImageMetaEXIF::SubjectDistance);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifMeteringMode,
														*bagMetaEXIF,	ImageMetaEXIF::MeteringMode);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifLightSource,
														*bagMetaEXIF,	ImageMetaEXIF::LightSource);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifFlash,
														*bagMetaEXIF,	ImageMetaEXIF::Flash);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifFocalLength,
														*bagMetaEXIF,	ImageMetaEXIF::FocalLength);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifSubjectArea,
														*bagMetaEXIF,	ImageMetaEXIF::SubjectArea);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifFlashPixVersion,
														*bagMetaEXIF,	ImageMetaEXIF::FlashPixVersion,
														TT_EXIF_VERSION);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifColorSpace,
														*bagMetaEXIF,	ImageMetaEXIF::ColorSpace);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifPixelXDimension,
														*bagMetaEXIF,	ImageMetaEXIF::PixelXDimension);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifPixelYDimension,
														*bagMetaEXIF,	ImageMetaEXIF::PixelYDimension);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifRelatedSoundFile,
														*bagMetaEXIF,	ImageMetaEXIF::RelatedSoundFile);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifFlashEnergy,
														*bagMetaEXIF,	ImageMetaEXIF::FlashEnergy);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifFocalPlaneXResolution,
														*bagMetaEXIF,	ImageMetaEXIF::FocalPlaneXResolution);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifFocalPlaneYResolution,
														*bagMetaEXIF,	ImageMetaEXIF::FocalPlaneYResolution);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifFocalPlaneResolutionUnit,
														*bagMetaEXIF,	ImageMetaEXIF::FocalPlaneResolutionUnit);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifSubjectLocation,
														*bagMetaEXIF,	ImageMetaEXIF::SubjectLocation);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifExposureIndex,
														*bagMetaEXIF,	ImageMetaEXIF::ExposureIndex);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifSensingMethod,
														*bagMetaEXIF,	ImageMetaEXIF::SensingMethod);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifFileSource,
														*bagMetaEXIF,	ImageMetaEXIF::FileSource);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifSceneType,
														*bagMetaEXIF,	ImageMetaEXIF::SceneType);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifCustomRendered,
														*bagMetaEXIF,	ImageMetaEXIF::CustomRendered);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifExposureMode,
														*bagMetaEXIF,	ImageMetaEXIF::ExposureMode);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifWhiteBalance,
														*bagMetaEXIF,	ImageMetaEXIF::WhiteBalance);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifDigitalZoomRatio,
														*bagMetaEXIF,	ImageMetaEXIF::DigitalZoomRatio);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifFocalLenIn35mmFilm,
														*bagMetaEXIF,	ImageMetaEXIF::FocalLenIn35mmFilm);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifSceneCaptureType,
														*bagMetaEXIF,	ImageMetaEXIF::SceneCaptureType);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifGainControl,
														*bagMetaEXIF,	ImageMetaEXIF::GainControl);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifContrast,
														*bagMetaEXIF,	ImageMetaEXIF::Contrast);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifSaturation,
														*bagMetaEXIF,	ImageMetaEXIF::Saturation);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifSharpness,
														*bagMetaEXIF,	ImageMetaEXIF::Sharpness);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifSubjectDistRange,
														*bagMetaEXIF,	ImageMetaEXIF::SubjectDistRange);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifImageUniqueID,
														*bagMetaEXIF,	ImageMetaEXIF::ImageUniqueID);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifGamma,
														*bagMetaEXIF,	ImageMetaEXIF::Gamma);
		
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifMakerNote,
														*bagMetaEXIF,	ImageMetaEXIF::MakerNote);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifUserComment,
														*bagMetaEXIF,	ImageMetaEXIF::UserComment);
#if ENABLE_ALL_METAS
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifSubsecTime,
														*bagMetaEXIF,	ImageMetaEXIF::SubsecTime);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifSubsecTimeOrginal,
														*bagMetaEXIF,	ImageMetaEXIF::SubsecTimeOriginal);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifSubsecTimeDigitized,
														*bagMetaEXIF,	ImageMetaEXIF::SubsecTimeDigitized);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifSpatialFrequencyResponse,
														*bagMetaEXIF,	ImageMetaEXIF::SpatialFrequencyResponse,
														TT_BLOB);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifOECF,
														*bagMetaEXIF,	ImageMetaEXIF::OECF,
														TT_BLOB);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifCFAPattern,
														*bagMetaEXIF,	ImageMetaEXIF::CFAPattern,
														TT_BLOB);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaEXIF,		kCGImagePropertyExifDeviceSettingDescription,
														*bagMetaEXIF,	ImageMetaEXIF::DeviceSettingDescription,
														TT_BLOB);
#endif
		//synchronize IPTC datetimes with Exif datetimes 
		//(workaround for IPTC time tag bug)
		if (bagMetaEXIF->AttributeExists( ImageMetaEXIF::DateTimeOriginal))
		{
			VValueBag *bagIPTC = ioValueBag->GetUniqueElement( ImageMetaIPTC::IPTC);
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
			VValueBag *bagIPTC = ioValueBag->GetUniqueElement( ImageMetaIPTC::IPTC);
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
			ioValueBag->AddElement( ImageMetaEXIF::EXIF, bagMetaEXIF);
		bagMetaEXIF->Release();
	}
	
	//read GPS metadatas 
	CFDictionaryRef metaGPS = NULL;
	if (CFDictionaryGetValueIfPresent(fProperties, kCGImagePropertyGPSDictionary, (const void**)&metaGPS))
	{
		VValueBag *bagMetaGPS = new VValueBag();
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSVersion,
														*bagMetaGPS,	ImageMetaGPS::VersionID,
														TT_GPSVERSIONID);
		{
			_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSLatitudeRef,
															*bagMetaGPS,	ImageMetaGPS::LatitudeRef);
			_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSLatitude,
															*bagMetaGPS,	ImageMetaGPS::Latitude);
			if (bagMetaGPS->AttributeExists( ImageMetaGPS::Latitude)
				&&
				bagMetaGPS->AttributeExists( ImageMetaGPS::LatitudeRef))
			{
				//convert to XMP latitude 
				VString latitude, latitudeRef;
				bagMetaGPS->GetString( ImageMetaGPS::Latitude, latitude);
				bagMetaGPS->GetString( ImageMetaGPS::LatitudeRef, latitudeRef);
				VString latitudeXMP;
				IHelperMetadatas::GPSCoordsFromExifToXMP( latitude, latitudeRef, latitudeXMP);
				bagMetaGPS->SetString( ImageMetaGPS::Latitude, latitudeXMP);
				bagMetaGPS->RemoveAttribute( ImageMetaGPS::LatitudeRef);
			}
			else
			{
				bagMetaGPS->RemoveAttribute( ImageMetaGPS::Latitude);
				bagMetaGPS->RemoveAttribute( ImageMetaGPS::LatitudeRef);
			}
		}
		
		{
			_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSLongitudeRef,
															*bagMetaGPS,	ImageMetaGPS::LongitudeRef);
			_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSLongitude,
															*bagMetaGPS,	ImageMetaGPS::Longitude);
			if (bagMetaGPS->AttributeExists( ImageMetaGPS::Longitude)
				&&
				bagMetaGPS->AttributeExists( ImageMetaGPS::LongitudeRef))
			{
				//convert to XMP latitude 
				VString longitude, longitudeRef;
				bagMetaGPS->GetString( ImageMetaGPS::Longitude, longitude);
				bagMetaGPS->GetString( ImageMetaGPS::LongitudeRef, longitudeRef);
				VString longitudeXMP;
				IHelperMetadatas::GPSCoordsFromExifToXMP( longitude, longitudeRef, longitudeXMP);
				bagMetaGPS->SetString( ImageMetaGPS::Longitude, longitudeXMP);
				bagMetaGPS->RemoveAttribute( ImageMetaGPS::LongitudeRef);
			}
			else
			{
				bagMetaGPS->RemoveAttribute( ImageMetaGPS::Longitude);
				bagMetaGPS->RemoveAttribute( ImageMetaGPS::LongitudeRef);
			}
		}
		
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSAltitudeRef,
														*bagMetaGPS,	ImageMetaGPS::AltitudeRef);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSAltitude,
														*bagMetaGPS,	ImageMetaGPS::Altitude);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSSatellites,
														*bagMetaGPS,	ImageMetaGPS::Satellites);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSStatus,
														*bagMetaGPS,	ImageMetaGPS::Status);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSMeasureMode,
														*bagMetaGPS,	ImageMetaGPS::MeasureMode);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSDOP,
														*bagMetaGPS,	ImageMetaGPS::DOP);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSSpeedRef,
														*bagMetaGPS,	ImageMetaGPS::SpeedRef);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSSpeed,
														*bagMetaGPS,	ImageMetaGPS::Speed);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSTrackRef,
														*bagMetaGPS,	ImageMetaGPS::TrackRef);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSTrack,
														*bagMetaGPS,	ImageMetaGPS::Track);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSImgDirectionRef,
														*bagMetaGPS,	ImageMetaGPS::ImgDirectionRef);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSImgDirection,
														*bagMetaGPS,	ImageMetaGPS::ImgDirection);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSMapDatum,
														*bagMetaGPS,	ImageMetaGPS::MapDatum);
		{
			_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSDestLatitudeRef,
															*bagMetaGPS,	ImageMetaGPS::DestLatitudeRef);
			_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSDestLatitude,
															*bagMetaGPS,	ImageMetaGPS::DestLatitude);
			if (bagMetaGPS->AttributeExists( ImageMetaGPS::DestLatitude)
				&&
				bagMetaGPS->AttributeExists( ImageMetaGPS::DestLatitudeRef))
			{
				//convert to XMP latitude 
				VString latitude, latitudeRef;
				bagMetaGPS->GetString( ImageMetaGPS::DestLatitude, latitude);
				bagMetaGPS->GetString( ImageMetaGPS::DestLatitudeRef, latitudeRef);
				VString latitudeXMP;
				IHelperMetadatas::GPSCoordsFromExifToXMP( latitude, latitudeRef, latitudeXMP);
				bagMetaGPS->SetString( ImageMetaGPS::DestLatitude, latitudeXMP);
				bagMetaGPS->RemoveAttribute( ImageMetaGPS::DestLatitudeRef);
			}
			else
			{
				bagMetaGPS->RemoveAttribute( ImageMetaGPS::DestLatitude);
				bagMetaGPS->RemoveAttribute( ImageMetaGPS::DestLatitudeRef);
			}
		}
		
		{
			_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSDestLongitudeRef,
															*bagMetaGPS,	ImageMetaGPS::DestLongitudeRef);
			_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSDestLongitude,
															*bagMetaGPS,	ImageMetaGPS::DestLongitude);
			if (bagMetaGPS->AttributeExists( ImageMetaGPS::DestLongitude)
				&&
				bagMetaGPS->AttributeExists( ImageMetaGPS::DestLongitudeRef))
			{
				//convert to XMP latitude 
				VString longitude, longitudeRef;
				bagMetaGPS->GetString( ImageMetaGPS::DestLongitude, longitude);
				bagMetaGPS->GetString( ImageMetaGPS::DestLongitudeRef, longitudeRef);
				VString longitudeXMP;
				IHelperMetadatas::GPSCoordsFromExifToXMP( longitude, longitudeRef, longitudeXMP);
				bagMetaGPS->SetString( ImageMetaGPS::DestLongitude, longitudeXMP);
				bagMetaGPS->RemoveAttribute( ImageMetaGPS::DestLongitudeRef);
			}
			else
			{
				bagMetaGPS->RemoveAttribute( ImageMetaGPS::DestLongitude);
				bagMetaGPS->RemoveAttribute( ImageMetaGPS::DestLongitudeRef);
			}
		}
		
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSDestBearingRef,
														*bagMetaGPS,	ImageMetaGPS::DestBearingRef);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSDestBearing,
														*bagMetaGPS,	ImageMetaGPS::DestBearing);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSDestDistanceRef,
														*bagMetaGPS,	ImageMetaGPS::DestDistanceRef);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSDestDistance,
														*bagMetaGPS,	ImageMetaGPS::DestDistance);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSProcessingMethod,
														*bagMetaGPS,	ImageMetaGPS::ProcessingMethod);
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSAreaInformation,
														*bagMetaGPS,	ImageMetaGPS::AreaInformation);
		{
			_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSDateStamp,
															*bagMetaGPS,	ImageMetaGPS::DateStamp);
			_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSTimeStamp,
															*bagMetaGPS,	ImageMetaGPS::TimeStamp);
			if (bagMetaGPS->AttributeExists( ImageMetaGPS::DateStamp)
				||
				bagMetaGPS->AttributeExists( ImageMetaGPS::TimeStamp))
			{
				//convert to XML datetime
				
				VString dateGPS, timeGPS, datetimeXML;
				bagMetaGPS->GetString( ImageMetaGPS::DateStamp, dateGPS);
				bagMetaGPS->GetString( ImageMetaGPS::TimeStamp, timeGPS);
				
				IHelperMetadatas::DateTimeFromGPSToXML( dateGPS, timeGPS, datetimeXML);
				
				bagMetaGPS->SetString( ImageMetaGPS::DateTime, datetimeXML);
				bagMetaGPS->RemoveAttribute( ImageMetaGPS::DateStamp);
				bagMetaGPS->RemoveAttribute( ImageMetaGPS::TimeStamp);
			}
		}
		_FromCFDictionnaryKeyValuePairToBagKeyValuePair(metaGPS,		kCGImagePropertyGPSDifferental,
														*bagMetaGPS,	ImageMetaGPS::Differential);
		
		if (bagMetaGPS->GetAttributesCount() > 0)
			ioValueBag->AddElement( ImageMetaGPS::GPS, bagMetaGPS);
		bagMetaGPS->Release();
	}
	
#if VERSIONDEBUG	
	//VString bagDump;
	//ioValueBag->DumpXML(bagDump, VString("Metadatas"), true);
	//CFStringRef bagDumpCF = bagDump.MAC_RetainCFStringCopy();
	//CFShow(bagDumpCF);
	//CFRelease(bagDumpCF);
#endif
}



/** convert a CFDictionnary key-value pair to a VValueBag attribute 
 @remarks
	bag value type is determined from metadata tag type as follow:
	ASCII tag value <-> VString bag value if repeat count is 1, otherwise encode tag string values in a VString using ';' as separator
	UNDEFINED, BYTE, SHORT, LONG or SLONG tag value <-> VLong bag value if repeat count is 1, otherwise encode tag values in a VString using ';' as separator
	RATIONAL, SRATIONAL tag value <-> VReal bag value if repeat count is 1, otherwise encode tag values in a VString using ';' as separator
*/
bool stImageSource::_FromCFDictionnaryKeyValuePairToBagKeyValuePair(	CFDictionaryRef inCFDict, CFStringRef inCFDictKey, 
																		VValueBag& ioBag, const XBOX::VValueBag::StKey& inBagKey,
																		eMetaTagType inTagType)
{
	CFTypeRef value;
	value = CFDictionaryGetValue(inCFDict, inCFDictKey);
	if (value == NULL)
		return false;
#if VERSIONDEBUG
	//CFShow(inCFDictKey);
	//CFShow(value);
#endif
	CFTypeID type = CFGetTypeID(value);
	if(type == CFStringGetTypeID())
	{
		//get as a string value
		VString valString;
		valString.MAC_FromCFString((CFStringRef)value);
		
		if (inTagType == TT_EXIF_DATETIME)
		{
			//convert to XML datetime
			VString valStringEXIF = valString;
			IHelperMetadatas::DateTimeFromExifToXML( valStringEXIF, valString);
		}
		
		ioBag.SetString( inBagKey, valString);
	} 
	else if (type == CFNumberGetTypeID())
	{
		//get as a number value
		CFNumberRef number = (CFNumberRef)value;
		if (CFNumberIsFloatType(number))
		{
			//get as a double
			double numberDouble;
			CFNumberGetValue(number, kCFNumberDoubleType, &numberDouble);
			ioBag.SetReal( inBagKey, (Real)numberDouble);
		}
		else
		{
			//get as a signed integer
			int32_t numberInt;
			CFNumberGetValue(number, kCFNumberSInt32Type, &numberInt);
			ioBag.SetLong( inBagKey, (sLONG)numberInt);
		}
	}
	else if (type == CFArrayGetTypeID())
	{
		//get as an array of values
		CFArrayRef arrayValues = (CFArrayRef)value;
		if (inTagType == TT_BLOB) //undefined type tags are encoded in bag as blobs
		{
			IHelperMetadatas::SetCFArrayChar( &ioBag, inBagKey, arrayValues);
			return true;
		}
		
		//build string with ';'-separated list of sub-values 
		//(no other way to deal with attributes which can have multiple values
		// - can occur with IPTC Keywords tag for instance -
		// and we use ';' as separator instead of ',' to avoid conflicting with floating point numbers formatted as string)
		VString valuePacked;
		CFIndex count = CFArrayGetCount(arrayValues);
		for (CFIndex i = 0; i < count; i++)
		{
			value = CFArrayGetValueAtIndex(arrayValues, i);
#if VERSIONDEBUG
			//CFShow(value);
#endif
			type = CFGetTypeID(value);
			if(type == CFStringGetTypeID())
			{
				//get as a string value
				VString xvalue;
				xvalue.MAC_FromCFString((CFStringRef)value);
				if (valuePacked.IsEmpty())
					valuePacked = xvalue;
				else
				{
					valuePacked.AppendUniChar(';');
					valuePacked.AppendString(xvalue);
				}
			} 
			else if (type == CFNumberGetTypeID())
			{
				//get as a number value
				CFNumberRef number = (CFNumberRef)value;
				if (CFNumberIsFloatType(number))
				{
					//get as a double
					double numberDouble;
					CFNumberGetValue(number, kCFNumberDoubleType, &numberDouble);
					
					if (valuePacked.IsEmpty())
						valuePacked.FromReal(numberDouble);
					else
					{
						valuePacked.AppendUniChar(';');
						valuePacked.AppendReal(numberDouble);
					}
				}
				else
				{
					//get as a signed integer
					int32_t numberInt;
					CFNumberGetValue(number, kCFNumberSInt32Type, &numberInt);
					if (valuePacked.IsEmpty())
						valuePacked.FromLong((sLONG)numberInt);
					else
					{
						valuePacked.AppendUniChar(';');
						valuePacked.AppendLong((sLONG)numberInt);
					}
				}
			}
		}
		if (inTagType == TT_GPSVERSIONID)
			//use XMP separator
			valuePacked.ExchangeAll(';','.');
		else if (inTagType == TT_EXIF_VERSION)
		{
			//use XMP exif version string
			if (valuePacked.IsEmpty())
				return false;
			
			VectorOfVString vValues;
			valuePacked.GetSubStrings((UniChar)';',vValues); 
			if (vValues.size() == 1)
				valuePacked = "0"+vValues[0]+"00";
			else if (vValues.size() == 2)
				valuePacked = "0"+vValues[0]+vValues[1]+"0";
			else if (vValues.size() == 3)
				valuePacked = "0"+vValues[0]+vValues[1]+vValues[2];
			else
				valuePacked = vValues[0]+vValues[1]+vValues[2]+vValues[3];
		}
		ioBag.SetString( inBagKey, valuePacked);
	}
	return true;
}





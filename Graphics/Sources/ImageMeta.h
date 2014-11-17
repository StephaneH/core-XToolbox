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

#ifndef __ImageMeta__
#define __ImageMeta__

#include "VPoint.h"

BEGIN_TOOLBOX_NAMESPACE

//eventually allow ENABLE_ALL_METAS only for 4D internal usage (debug)
//(if 0 allow only 4D metas available for 4D developers)
#if VERSIONDEBUG
#define ENABLE_ALL_METAS 0
#else
#define ENABLE_ALL_METAS 0
#endif


//encoding and metadata properties

//encoding background color (only used if destination format has no alpha channel)
//@remarks
//	this property cannot be overriden
#define ENCODING_BACKGROUND_COLOR		VColor(0xff,0xff,0xff)


template<class T>
inline void clampValue(T& val, const T min, const T max)
{
	if (val < min) 
		val = min;
	else if (val > max)
		val = max;
};

#define saturateValue(val)				clampValue(val,0.0,1.0)

/** metadatas bag 
 @remarks
	all metadatas are stored in a single metadatas bag:
	this bag has one element per metadata block (IPTC, TIFF, EXIF, or GPS)

	please use classes ImageMeta::stReader and ImageMeta::stWriter to read or write metadatas
	(these classes must be constructed with the metadatas bag)
 @note
	we need to take in account IPTC/TIFF/EXIF tags as well as XMP tags:
	in order to properly synchronize IPTC/TIFF/EXIF tags with XMP tags,
	if IPTC/TIFF/EXIF tag type is not the same as the associated XMP tag type,
	we choose the most compatible type to store the metadata in the bag
	(for instance date, time and datetimes tags are all stored as XML datetimes VString or VTime,
	 GPS latitude and longitude as XMP formatted latitude and longitude, etc...)
 @see
	ImageMetaIPTC
	ImageMetaTIFF
	ImageMetaEXIF
	ImageMetaGPS

	see metadata tags comments for per-tag detailed information 
 */
namespace ImageMeta
{
	//metadatas bag name
	XTOOLBOX_API EXTERN_BAGKEY(Metadatas);	
}	


/** encoding properties
@remarks
	encoding properties bag contains both encoding properties as bag attributes
	and optionally modified metadatas bag as unique element with name ImageMeta::Metadatas

	please use ImageEncoding::stReader and ImageEncoding::stWriter
	to read or write encoding properties or retain/release the optional metadatas bag
*/

#if !VERSION_LINUX
namespace ImageEncoding
{
	//image quality (0 : worst quality, 1: best quality = lossless if codec support it)
	XTOOLBOX_API EXTERN_BAGKEY_WITH_DEFAULT_SCALAR(ImageQuality,	XBOX::VReal, Real);	

	//image lossless compression (0 : worst/fastest compression, 1: best/slowest compression)
	//@remarks
	//	used only for TIFF
	XTOOLBOX_API EXTERN_BAGKEY_WITH_DEFAULT_SCALAR(ImageCompression,	XBOX::VReal, Real);		

	//metadatas encoding status (1 : encode with metadatas, 0: do not encode with metadatas)
	//@remark
	//	by default, if attribute is not defined, metadatas are encoded
	XTOOLBOX_API EXTERN_BAGKEY_WITH_DEFAULT_SCALAR(EncodeMetadatas, XBOX::VBoolean, Boolean);		
	
	// extension or mime 
	XTOOLBOX_API EXTERN_BAGKEY_WITH_DEFAULT_SCALAR(CodecIdentifier,XBOX::VString, XBOX::VString);

	//encoding properties reader class
	class XTOOLBOX_API stReader
	{
	public:
		stReader(const VValueBag *inBagEncoding)
		{
			if (inBagEncoding == NULL)
				//set default to enable caller to request default encoding properties
				fBagRead = new VValueBag();
			else
			{
				inBagEncoding->Retain();
				fBagRead = inBagEncoding;
			}
		}
		virtual ~stReader()
		{
			fBagRead->Release();
		}

		/** get extension */
		VString GetCodecIdentifier() const
		{
			VString value;
			fBagRead->GetString( CodecIdentifier, value);
			return value;
		}

		/** get image quality (0 : worst quality, 1: best quality = lossless if codec support it) */
		Real GetImageQuality() const
		{
			Real value;
			if (fBagRead->GetReal( ImageQuality, value))
			{
				saturateValue(value);
				return value;
			}
			else
				return 1.0;
		}

		/** get image lossless compression (0 : worst/fastest compression, 1: best/slowest compression) */
		Real GetImageCompression() const
		{
			Real value;
			if (fBagRead->GetReal( ImageCompression, value))
			{
				saturateValue(value);
				return value;
			}
			else
				return 0.8;
		}

		
		/** get metadatas encoding status (1 : encode with metadatas, 0: do not encode with metadatas) */
		bool WriteMetadatas() const
		{
			Boolean value;
			if (fBagRead->GetBoolean( EncodeMetadatas, value))
				return value != 0;
			else
				return true;
		}
		
		/** retain read-only modified metadatas bag */
		const VValueBag *RetainMetadatas() const
		{
			return fBagRead->RetainUniqueElement( ImageMeta::Metadatas);
		}

		/** re-encoding status */
		bool NeedReEncode() const
		{
			if (fBagRead->AttributeExists(ImageEncoding::ImageQuality)) 
				if (GetImageQuality() < 1.0)
					return true;
			if (fBagRead->AttributeExists(ImageEncoding::ImageCompression))
				return true;
			if (fBagRead->AttributeExists(ImageEncoding::EncodeMetadatas))
				if (!WriteMetadatas())
					return true;
			if (fBagRead->GetUniqueElement(ImageMeta::Metadatas) != NULL)
				return true;
			return false;
		}

	protected:
		const VValueBag *fBagRead;
	};

	//encoding properties writer class
	class XTOOLBOX_API stWriter 
	{
	public:
		stWriter(VValueBag *ioBagEncoding)
		{
			xbox_assert(ioBagEncoding);
			ioBagEncoding->Retain();
			fBagWrite = ioBagEncoding;
		}
		virtual ~stWriter()
		{
			fBagWrite->Release();
		}

		/** set codec identifier */
		void SetCodecIdentifier( const VString& inCodecIdentifier)
		{
			fBagWrite->SetString( CodecIdentifier, inCodecIdentifier);
		}
		/** set image quality (0 : worst quality, 1: best quality = lossless if codec support it) */
		void SetImageQuality( Real inQuality)
		{
			saturateValue(inQuality);
			fBagWrite->SetReal( ImageQuality, inQuality);
		}

		/** set metadatas encoding status (1 : encode with metadatas, 0: do not encode with metadatas) */
		void WriteMetadatas( bool inWriteMetadatas)
		{
			fBagWrite->SetBoolean( EncodeMetadatas, (Boolean)(inWriteMetadatas ? 1 : 0));
		}

		/** set image lossless compression (0 : worst/fastest compression, 1: best/slowest compression) */
		void SetImageCompression( Real inCompression)
		{
			saturateValue(inCompression);
			fBagWrite->SetReal( ImageCompression, inCompression);
		}
		
		/** retain read-write modified metadatas bag */
		VValueBag *CreateOrRetainMetadatas(VValueBag *inBagMetas = NULL)
		{
			VValueBag *bag = fBagWrite->RetainUniqueElement( ImageMeta::Metadatas);
			if (bag == NULL)
			{
				if (inBagMetas)
				{
					inBagMetas->Retain();
					bag = inBagMetas;
				}
				else
					bag = new VValueBag();
				if (bag)
					fBagWrite->AddElement( ImageMeta::Metadatas, bag);
			}
			return bag;	
		}

		/** destroy modified metadatas */
		void DestroyMetadatas()
		{
			fBagWrite->RemoveElements( ImageMeta::Metadatas);
		}
	protected:
		VValueBag *fBagWrite;
	};
}
#endif


//IPTC metadatas properties (based on IIM v4.1: see http://www.iptc.org/std/IIM/4.1/specification/IIMV4.1.pdf)
namespace ImageMetaIPTC
{
	//bag name
	XTOOLBOX_API EXTERN_BAGKEY(IPTC);

	//bag attributes
	EXTERN_BAGKEY(ObjectTypeReference);			//VString 
	EXTERN_BAGKEY(ObjectAttributeReference);	//VString (alias Iptc4xmpCore:IntellectualGenre)
	EXTERN_BAGKEY(ObjectName);					//VString
	EXTERN_BAGKEY(EditStatus);					//VString
	EXTERN_BAGKEY(EditorialUpdate);				//VString
	EXTERN_BAGKEY(Urgency);						//VLong || VString (VLong formatted as VString)
												//	0 (reserved) 
												//	1 (most urgent) 
												//	2 
												//	3 
												//	4 
												//	5 (normal urgency) 
												//	6 
												//	7 
												//	8 (least urgent) 
												//	9 (user-defined priority)
	EXTERN_BAGKEY(SubjectReference);			//VLong or VString (if more than one, individual subjects codes are separated by ';')
												//(alias Iptc4xmpCore:SubjectCode)
	EXTERN_BAGKEY(Category);					//VString  
	EXTERN_BAGKEY(SupplementalCategory);		//VString (if more than one, categories are separated by ';')
	EXTERN_BAGKEY(FixtureIdentifier);			//VString
	EXTERN_BAGKEY(Keywords);					//VString (if more than one, keywords are separated by ';')
	EXTERN_BAGKEY(ContentLocationCode);			//VString (if more than one, keywords are separated by ';')
	EXTERN_BAGKEY(ContentLocationName);			//VString (if more than one, keywords are separated by ';')
	EXTERN_BAGKEY(ReleaseDateTime);				//VTime or VString with XML datetime ("YYYY-MM-DDTHH:MM:SS[+|-]HH:MM")
												//(build from IPTC 'ReleaseDate' and 'ReleaseTime' tags)
	EXTERN_BAGKEY(ExpirationDateTime);			//VTime or VString with XML datetime ("YYYY-MM-DDTHH:MM:SS[+|-]HH:MM")
												//(build from IPTC 'ExpirationDate' and 'ExpirationTime' tags)
	EXTERN_BAGKEY(SpecialInstructions);			//VString
	EXTERN_BAGKEY(ActionAdvised);				//VLong or VString (two numeric characters)
												//	"01" = Object Kill 
												//	"02" = Object Replace 
												//	"03" = Ojbect Append 
												//	"04" = Object Reference
	/** IPTC Action Advised enum */
	typedef enum eActionAdvised
	{
		kActionAdvised_Min			= 1,

		kActionAdvised_Kill			= 1,
		kActionAdvised_Replace		= 2,
		kActionAdvised_Append		= 3,
		kActionAdvised_Reference	= 4,

		kActionAdvised_Max			= 4
	} eActionAdvised;

	EXTERN_BAGKEY(ReferenceService);			//VString (if more than one, values are separated by ';')
	EXTERN_BAGKEY(ReferenceDateTime);			//VTime or VString with XML datetime ("YYYY-MM-DDTHH:MM:SS[+|-]HH:MM")
												//(if more than one, string-list of XML datetimes separated by ';') 
												//(build from IPTC 'ReferenceDate' tag)
	EXTERN_BAGKEY(ReferenceNumber);				//VLong or VString (if more than one, VString with values separated by ';')
	EXTERN_BAGKEY(DateTimeCreated);				//VString with XML datetime ("YYYY-MM-DDTHH:MM:SS[+|-]HH:MM")
												//(build from IPTC 'DateCreated' and 'TimeCreated' tags)
	EXTERN_BAGKEY(DigitalCreationDateTime);		//VTime or VString with XML datetime ("YYYY-MM-DDTHH:MM:SS[+|-]HH:MM")
												//(build from IPTC 'DigitalCreationDate' and 'DigitalCreationTime' tags)
	EXTERN_BAGKEY(OriginatingProgram);			//VString
	EXTERN_BAGKEY(ProgramVersion);				//VString
	EXTERN_BAGKEY(ObjectCycle);					//VString (one character)
												//	'a' = morning
												//	'p' = evening
												//	'b' = both

	/** IPTC Object Cycle enum */
	typedef enum eObjectCycle
	{
		kObjectCycle_Min		= 0,

		kObjectCycle_Morning	= 0,
		kObjectCycle_Evening	= 1,
		kObjectCycle_Both		= 2,

		kObjectCycle_Max		= 2

	} eObjectCycle;

	EXTERN_BAGKEY(Byline);						//VString (if more than one, values are separated by ';')
	EXTERN_BAGKEY(BylineTitle);					//VString (if more than one, values are separated by ';')
	EXTERN_BAGKEY(City);						//VString
	EXTERN_BAGKEY(SubLocation);					//VString
	EXTERN_BAGKEY(ProvinceState);				//VString
	EXTERN_BAGKEY(CountryPrimaryLocationCode);	//VString
	EXTERN_BAGKEY(CountryPrimaryLocationName);	//VString
	EXTERN_BAGKEY(OriginalTransmissionReference);	//VString
	EXTERN_BAGKEY(Scene);						//VString (IPTC scene code - 6 digits string: see http://www.newscodes.org:
												//         if more than one, scene codes are separated by ';')
	/** IPTC Scene enum 
	@see
		http://cv.iptc.org/newscodes/scene/
	*/
	typedef enum eScene
	{
		kScene_Min				= 10100,

		kScene_Headshot			= 10100,
		kScene_HalfLength		= 10200,
		kScene_FullLength		= 10300,
		kScene_Profile			= 10400,
		kScene_RearView			= 10500,
		kScene_Single			= 10600,
		kScene_Couple			= 10700,
		kScene_Two				= 10800,
		kScene_Group			= 10900,
		kScene_GeneralView		= 11000,
		kScene_PanoramicView	= 11100,
		kScene_AerialView		= 11200,
		kScene_UnderWater		= 11300,
		kScene_NightScene		= 11400,
		kScene_Satellite		= 11500,
		kScene_ExteriorView		= 11600,
		kScene_InteriorView		= 11700,
		kScene_CloseUp			= 11800,
		kScene_Action			= 11900,
		kScene_Performing		= 12000,
		kScene_Posing			= 12100,
		kScene_Symbolic			= 12200,
		kScene_OffBeat			= 12300,
		kScene_MovieScene		= 12400,

		kScene_Max				= 12400

	} eScene;

	EXTERN_BAGKEY(Headline);					//VString
	EXTERN_BAGKEY(Credit);						//VString
	EXTERN_BAGKEY(Source);						//VString
	EXTERN_BAGKEY(CopyrightNotice);				//VString
	EXTERN_BAGKEY(Contact);						//VString (if more than one, contacts are separated by ';')
	EXTERN_BAGKEY(CaptionAbstract);				//VString
	EXTERN_BAGKEY(WriterEditor);				//VString (if more than one, writers/editors are separated by ';')
	EXTERN_BAGKEY(ImageType);					//VString : two characters only: first is numeric, second is alphabetic
												//	first character:
												//	'0' = No objectdata.
												//	'1' = Single component, e.g. black and white or one component of a colour project.
												//	'2', '3', '4' = Multiple components for a colour project.
												//	'9' = Supplemental objects related to other objectdata
												//	second character:
												//	'W' = Monochrome.
												//	'Y' = Yellow component.
												//	'M' = Magenta component.
												//	'C' = Cyan component.
												//	'K' = Black component.
												//	'R' = Red component.
												//	'G' = Green component.
												//	'B' = Blue component.
												//	'T' = Text only.
												//	'F' = Full colour composite, frame sequential.
												//	'L' = Full colour composite, line sequential.
												//	'P' = Full colour composite, pixel sequential.
												//	'S' = Full colour composite, special interleaving.
	/** IPTC image type colour composition */
	typedef enum eImageType
	{
		kImageType_Min		  = 0,

		kImageType_Monochrome = 0,
		kImageType_Yellow	  = 1,
		kImageType_Magenta	  = 2,
		kImageType_Cyan		  = 3,
		kImageType_Black	  = 4,
		kImageType_Red		  = 5,
		kImageType_Green	  = 6,
		kImageType_Blue		  = 7,
		kImageType_TextOnly	  = 8,
		kImageType_FullColourCompositeFrameSequential		= 9,
		kImageType_FullColourCompositeLineSequential		= 10,
		kImageType_FullColourCompositePixelSequential		= 11,
		kImageType_FullColourCompositeSpecialInterleaving	= 12,

		kImageType_Max		  = 12

	} eImageType;

	EXTERN_BAGKEY(ImageOrientation);			//VString: one character only
												//	'L' = Landscape 
												//	'P' = Portrait 
												//	'S' = Square
	/** IPTC image orientation */
	typedef enum eImageOrientation
	{
		kImageOrientation_Min		  = 0,

		kImageOrientation_Landscape	  = 0,
		kImageOrientation_Portrait	  = 1,
		kImageOrientation_Square	  = 2,

		kImageOrientation_Max		  = 2,

	} eImageOrientation;

	EXTERN_BAGKEY(LanguageIdentifier);			//VString: two or three alphabetic characters 
												//(ISO 639:1988 national language identifier)
	EXTERN_BAGKEY(StarRating);					//VLong || VString (VLong formatted as VString)
												//(normal Rating values of 1,2,3,4 and 5 stars)

	//internal usage only: 
	//following tags are used only to read/write metadatas 
	//(bag datetime format is XML datetime format)
	EXTERN_BAGKEY(ReleaseDate);	
	EXTERN_BAGKEY(ReleaseTime);	
	EXTERN_BAGKEY(ExpirationDate);	
	EXTERN_BAGKEY(ExpirationTime);	
	EXTERN_BAGKEY(ReferenceDate);	
	EXTERN_BAGKEY(DateCreated);	
	EXTERN_BAGKEY(TimeCreated);	
	EXTERN_BAGKEY(DigitalCreationDate);	
	EXTERN_BAGKEY(DigitalCreationTime);	
}


//TIFF metadatas properties (based on Exif 2.2 (IFD TIFF tags): see http://exif.org/Exif2-2.PDF)
namespace ImageMetaTIFF
{
	//bag name
	XTOOLBOX_API EXTERN_BAGKEY(TIFF);	
	
	//bag attributes
	EXTERN_BAGKEY(Compression);					//VLong 
												//	1 = Uncompressed 
												//	2 = CCITT 1D 
												//	3 = T4/Group 3 Fax 
												//	4 = T6/Group 4 Fax 
												//	5 = LZW 
												//	6 = JPEG (old-style) 
												//	7 = JPEG 
												//	8 = Adobe Deflate 
												//	9 = JBIG B&W 
												//	10 = JBIG Color 
												//	99 = JPEG 
												//	262 = Kodak 262 
												//	32766 = Next 
												//	32767 = Sony ARW Compressed 
												//	32769 = Epson ERF Compressed 
												//	32771 = CCIRLEW 
												//	32773 = PackBits 
												//	32809 = Thunderscan 
												//	32867 = Kodak KDC Compressed 
												//	32895 = IT8CTPAD 
												//	32896 = IT8LW 
												//	32897 = IT8MP 
												//	32898 = IT8BL 
												//	32908 = PixarFilm 
												//	32909 = PixarLog 
												//	32946 = Deflate 
												//	32947 = DCS 
												//	34661 = JBIG 
												//	34676 = SGILog 
												//	34677 = SGILog24 
												//	34712 = JPEG 2000 
												//	34713 = Nikon NEF Compressed 
												//	34718 = Microsoft Document Imaging (MDI) Binary Level Codec 
												//	34719 = Microsoft Document Imaging (MDI) Progressive Transform Codec 
												//	34720 = Microsoft Document Imaging (MDI) Vector 
												//	65000 = Kodak DCR Compressed 
												//	65535 = Pentax PEF Compressed
	/** TIFF compression type */
	typedef enum eCompression
	{
		kCompression_Min 							= 1		,

		kCompression_Uncompressed 					= 1		,
		kCompression_CCITT_1D 						= 2		,
		kCompression_T4Group_3_Fax					= 3		,
		kCompression_T6Group_4_Fax 					= 4		,
		kCompression_LZW 							= 5		,
		kCompression_JPEG_OldStyle 					= 6		,
		kCompression_JPEG 							= 7		,
		kCompression_Adobe_Deflate 					= 8		,
		kCompression_JBIG_BW 						= 9		,
		kCompression_JBIG_Color 					= 10	,
		kCompression_JPEG_Bis 						= 99	,
		kCompression_Kodak_262 						= 262	,
		kCompression_Next							= 32766 ,
		kCompression_Sony_ARW_Compressed 			= 32767 ,
		kCompression_Epson_ERF_Compressed 			= 32769 ,
		kCompression_CCIRLEW 						= 32771 ,
		kCompression_PackBits 						= 32773 ,
		kCompression_Thunderscan 					= 32809 ,
		kCompression_Kodak_KDC_Compressed 			= 32867 ,
		kCompression_IT8CTPAD 						= 32895 ,
		kCompression_IT8LW 							= 32896 ,
		kCompression_IT8MP 							= 32897 ,
		kCompression_IT8BL 							= 32898 ,
		kCompression_PixarFilm 						= 32908 ,
		kCompression_PixarLog 						= 32909 ,
		kCompression_Deflate 						= 32946 ,
		kCompression_DCS 							= 32947 ,
		kCompression_JBIG 							= 34661 ,
		kCompression_SGILog 						= 34676 ,
		kCompression_SGILog24 						= 34677 ,
		kCompression_JPEG2000						= 34712 ,
		kCompression_Nikon_NEF_Compressed 			= 34713 ,
		kCompression_MDI_BinaryLevelCodec 			= 34718 ,
		kCompression_MDI_ProgressiveTransformCodec 	= 34719 ,
		kCompression_MDI_Vector 					= 34720 ,
		kCompression_Kodak_DCR_Compressed 			= 65000 ,
		kCompression_Pentax_PEF_Compressed			= 65535 ,

		kCompression_Max 							= 65535

	} eCompression;

	EXTERN_BAGKEY(PhotometricInterpretation);	//VLong
												//	0 = WhiteIsZero 
												//	1 = BlackIsZero 
												//	2 = RGB 
												//	3 = RGB Palette 
												//	4 = Transparency Mask 
												//	5 = CMYK 
												//	6 = YCbCr 
												//	8 = CIELab 
												//	9 = ICCLab 
												//	10 = ITULab 
												//	32803 = Color Filter Array 
												//	32844 = Pixar LogL 
												//	32845 = Pixar LogLuv 
												//	34892 = Linear Raw
	/** TIFF Photometric Interpretation */
	typedef enum ePhotometricInterpretation
	{
		kPhotometricInterpretation_Min		 		= 0		,

		kPhotometricInterpretation_WhiteIsZero 		= 0		,
		kPhotometricInterpretation_BlackIsZero 		= 1 	,
		kPhotometricInterpretation_RGB 				= 2 	,
		kPhotometricInterpretation_RGBPalette 		= 3 	,
		kPhotometricInterpretation_TransparencyMask = 4 	,
		kPhotometricInterpretation_CMYK 			= 5 	,
		kPhotometricInterpretation_YCbCr 			= 6 	,
		kPhotometricInterpretation_CIELab 			= 8 	,
		kPhotometricInterpretation_ICCLab 			= 9 	,
		kPhotometricInterpretation_ITULab 			= 10	,
		kPhotometricInterpretation_ColorFilterArray = 32803 ,
		kPhotometricInterpretation_PixarLogL 		= 32844 ,
		kPhotometricInterpretation_PixarLogLuv 		= 32845 ,
		kPhotometricInterpretation_LinearRaw		= 34892 ,

		kPhotometricInterpretation_Max		 		= 34892	

	} ePhotometricInterpretation;

	EXTERN_BAGKEY(DocumentName);				//VString
	EXTERN_BAGKEY(ImageDescription);			//VString
	EXTERN_BAGKEY(Make);						//VString
	EXTERN_BAGKEY(Model);						//VString
	EXTERN_BAGKEY(Orientation);					//VLong
												//	1 = Horizontal (normal) 
												//	2 = Mirror horizontal 
												//	3 = Rotate 180 
												//	4 = Mirror vertical 
												//	5 = Mirror horizontal and rotate 270 CW 
												//	6 = Rotate 90 CW 
												//	7 = Mirror horizontal and rotate 90 CW 
												//	8 = Rotate 270 CW
	/** TIFF Orientation */
	typedef enum eOrientation
	{
		kOrientation_Min							= 1,

		kOrientation_Horizontal						= 1,
		kOrientation_MirrorHorizontal				= 2,
		kOrientation_Rotate180						= 3,
		kOrientation_MirrorVertical					= 4,
		kOrientation_MirrorHorizontalRotate270CW	= 5,
		kOrientation_Rotate90CW						= 6,
		kOrientation_MirrorHorizontalRotate90CW		= 7,
		kOrientation_Rotate270CW					= 8,

		kOrientation_Max							= 8,

	} eOrientation;

	EXTERN_BAGKEY(XResolution);					//VReal
	EXTERN_BAGKEY(YResolution);					//VReal
	EXTERN_BAGKEY(ResolutionUnit);				//VLong
												//	1 = None 
												//	2 = inches 
												//	3 = cm 
												//	4 = mm 
												//	5 = um
	typedef enum eResolutionUnit
	{
		kResolutionUnit_Min		= 1,

		kResolutionUnit_None 	= 1,
		kResolutionUnit_Inches	= 2,
		kResolutionUnit_Cm		= 3,
		kResolutionUnit_Mm		= 4,
		kResolutionUnit_Um		= 5,

		kResolutionUnit_Max		= 5,

	} eResolutionUnit;

	EXTERN_BAGKEY(Software);					//VString
	EXTERN_BAGKEY(TransferFunction);			//VString (list of string-formatted integer values separated by ';')
	EXTERN_BAGKEY(DateTime);					//VTime or VString with XML datetime ("YYYY-MM-DDTHH:MM:SSZ")
	EXTERN_BAGKEY(Artist);						//VString 
	EXTERN_BAGKEY(HostComputer);				//VString
	EXTERN_BAGKEY(Copyright);					//VString
	EXTERN_BAGKEY(WhitePoint);					//VString (list of string-formatted real values separated by ';')
	EXTERN_BAGKEY(PrimaryChromaticities);		//VString (list of string-formatted real values separated by ';')
}


//EXIF metadatas properties (based on Exif 2.2 (IFD EXIF tags): see http://exif.org/Exif2-2.PDF)
namespace ImageMetaEXIF
{
	//bag name
	XTOOLBOX_API EXTERN_BAGKEY(EXIF);	
	
	//bag attributes
	EXTERN_BAGKEY(ExposureTime);			//VReal
	EXTERN_BAGKEY(FNumber);					//VReal
	EXTERN_BAGKEY(ExposureProgram);			//VLong
											//	1 = Manual 
											//	2 = Program AE 
											//	3 = Aperture-priority AE 
											//	4 = Shutter speed priority AE 
											//	5 = Creative (Slow speed) 
											//	6 = Action (High speed) 
											//	7 = Portrait 
											//	8 = Landscape
	/** EXIF Exposure Program */
	typedef enum eExposureProgram 
	{
		kExposureProgram_Min						= 1,

		kExposureProgram_Manual						= 1,
		kExposureProgram_ProgramAE					= 2,
		kExposureProgram_AperturePriorityAE			= 3,
		kExposureProgram_ShutterSpeedPriorityAE		= 4,
		kExposureProgram_Creative					= 5,
		kExposureProgram_Action						= 6,
		kExposureProgram_Portrait					= 7,
		kExposureProgram_Landscape					= 8,

		kExposureProgram_Max						= 8

	} eExposureProgram;

	EXTERN_BAGKEY(SpectralSensitivity);		//VString
	EXTERN_BAGKEY(ISOSpeedRatings);			//VString (list of string-formatted integer values separated by ';')
	EXTERN_BAGKEY(OECF);					//VString 
											// base64-encoded binary blob 
											// or 
											// list of string-formatted uchar values separated by ';'
	EXTERN_BAGKEY(ExifVersion);				//VString (4 characters: "0220" for instance)					
	EXTERN_BAGKEY(DateTimeOriginal);		//VTime or VString with XML datetime ("YYYY-MM-DDTHH:MM:SSZ")
	EXTERN_BAGKEY(DateTimeDigitized);		//VTime or VString with XML datetime ("YYYY-MM-DDTHH:MM:SSZ")
	EXTERN_BAGKEY(ComponentsConfiguration);	//VString (list of 4 string-formatted integer values separated by ';')
											//  meaning for single integer value:
											//	0 = does not exist
											//	1 = Y 
											//	2 = Cb 
											//	3 = Cr 
											//	4 = R 
											//	5 = G 
											//	6 = B
											//  for instance "4;5;6;0" is RGB uncompressed
	/** EXIF component type */
	typedef enum eComponentType
	{
		kComponentType_Min		= 0,

		kComponentType_Unused	= 0,
		kComponentType_Y		= 1,
		kComponentType_Cb		= 2,
		kComponentType_Cr		= 3,
		kComponentType_R		= 4,
		kComponentType_G		= 5,
		kComponentType_B		= 6,

		kComponentType_Max		= 6

	} eComponentType;

	EXTERN_BAGKEY(CompressedBitsPerPixel);	//VReal
	EXTERN_BAGKEY(ShutterSpeedValue);		//VReal
	EXTERN_BAGKEY(ApertureValue);			//VReal
	EXTERN_BAGKEY(BrightnessValue);			//VReal
	EXTERN_BAGKEY(ExposureBiasValue);		//VReal
	EXTERN_BAGKEY(MaxApertureValue);		//VReal
	EXTERN_BAGKEY(SubjectDistance);			//VReal
	EXTERN_BAGKEY(MeteringMode);			//VLong
											//	1 	= Average 
											//	2 	= Center-weighted average 
											//	3 	= Spot 
											//	4 	= Multi-spot 
											//	5 	= Multi-segment 
											//	6 	= Partial 
											//	255 = Other

	/** EXIF Metering Mode */
	typedef enum eMeteringMode
	{
		kMeteringMode_Min						= 1  ,

		kMeteringMode_Average					= 1  ,
		kMeteringMode_CenterWeightedAverage		= 2  ,
		kMeteringMode_Spot						= 3  ,
		kMeteringMode_MultiSpot					= 4  ,
		kMeteringMode_MultiSegment				= 5  ,
		kMeteringMode_Partial					= 6  ,
		kMeteringMode_Other						= 255,

		kMeteringMode_Max						= 255

	} eMeteringMode;

	EXTERN_BAGKEY(LightSource);				//VLong
											//	0 	= Unknown 
											//	1 	= Daylight 
											//	2 	= Fluorescent 
											//	3 	= Tungsten 
											//	4 	= Flash 
											//	9 	= Fine Weather 
											//	10 	= Cloudy 
											//	11 	= Shade 
											//	12 	= Daylight Fluorescent 
											//	13 	= Day White Fluorescent 
											//	14 	= Cool White Fluorescent 
											//	15 	= White Fluorescent 
											//	17 	= Standard Light A 
											//	18 	= Standard Light B 
											//	19 	= Standard Light C 
											//	20 	= D55 
											//	21 	= D65 
											//	22 	= D75 
											//	23 	= D50 
											//	24 	= ISO Studio Tungsten 
											//	255 = Other
	/** EXIF Light Source Type */
	typedef enum eLightSource
	{
		kLightSource_Min						= 0	,

		kLightSource_Unknown					= 0	,
		kLightSource_Daylight					= 1	,
		kLightSource_Fluorescent				= 2	,
		kLightSource_Tungsten					= 3	,
		kLightSource_Flash						= 4 ,
		kLightSource_FineWeather				= 9	,
		kLightSource_Cloudy						= 10,
		kLightSource_Shade						= 11,
		kLightSource_DaylightFluorescent		= 12,
		kLightSource_DayWhiteFluorescent		= 13,
		kLightSource_CoolWhiteFluorescent		= 14,
		kLightSource_WhiteFluorescent			= 15,
		kLightSource_StandardLightA				= 17,
		kLightSource_StandardLightB				= 18,
		kLightSource_StandardLightC				= 19,
		kLightSource_D55						= 20,
		kLightSource_D65						= 21,
		kLightSource_D75						= 22,
		kLightSource_D50						= 23,
		kLightSource_ISOStudioTungsten			= 24,
		kLightSource_Other						= 255,

		kLightSource_Max						= 255

	} eLightSource;

	EXTERN_BAGKEY(Flash);					//VLong
											//	0000.H = Flash did not fire.
											//	0001.H = Flash fired.
											//	0005.H = Strobe return light not detected.
											//	0007.H = Strobe return light detected.
											//	0009.H = Flash fired, compulsory flash mode
											//	000D.H = Flash fired, compulsory flash mode, return light not detected
											//	000F.H = Flash fired, compulsory flash mode, return light detected
											//	0010.H = Flash did not fire, compulsory flash mode
											//	0018.H = Flash did not fire, auto mode
											//	0019.H = Flash fired, auto mode
											//	001D.H = Flash fired, auto mode, return light not detected
											//	001F.H = Flash fired, auto mode, return light detected
											//	0020.H = No flash function
											//	0041.H = Flash fired, red-eye reduction mode
											//	0045.H = Flash fired, red-eye reduction mode, return light not detected
											//	0047.H = Flash fired, red-eye reduction mode, return light detected
											//	0049.H = Flash fired, compulsory flash mode, red-eye reduction mode
											//	004D.H = Flash fired, compulsory flash mode, red-eye reduction mode, return light not detected
											//	004F.H = Flash fired, compulsory flash mode, red-eye reduction mode, return light detected
											//	0059.H = Flash fired, auto mode, red-eye reduction mode
											//	005D.H = Flash fired, auto mode, return light not detected, red-eye reduction mode
											//	005F.H = Flash fired, auto mode, return light detected, red-eye reduction mode
											//	Other = reserved
	/** EXIF status of the returned light */ 
	typedef enum eFlashReturn
	{
		kFlashReturn_Min					= 0,

		kFlashReturn_NoDetectionFunction	= 0,
		kFlashReturn_Reserved				= 1,
		kFlashReturn_NotDetected			= 2,
		kFlashReturn_Detected				= 3,

		kFlashReturn_Max					= 3

	} eFlashReturn;	

	/** EXIF flash mode */
	typedef enum eFlashMode
	{
		kFlashMode_Min							= 0,

		kFlashMode_Unknown						= 0,
		kFlashMode_CompulsoryFlashFiring		= 1,
		kFlashMode_CompulsoryFlashSuppression	= 2,
		kFlashMode_AutoMode						= 3,

		kFlashMode_Max							= 3

	} eFlashMode;

	EXTERN_BAGKEY(FocalLength);				//VReal
	EXTERN_BAGKEY(SubjectArea);				//VString (list of 2 up to 4 string-formatted integer values separated by ';')
											//	The subject location and area are defined by number of values as follows.
											//	number of values = 2 
											//		Indicates the location of the main subject as coordinates. The first value is the X coordinate and the
											//		second is the Y coordinate.
											//	number of values = 3 
											//		The area of the main subject is given as a circle. The circular area is expressed as center coordinates
											//		and diameter. The first value is the center X coordinate, the second is the center Y coordinate, and the
											//		third is the diameter. 
											//	number of values = 4 
											//		The area of the main subject is given as a rectangle. The rectangular area is expressed as center
											//		coordinates and area dimensions. The first value is the center X coordinate, the second is the center Y
											//		coordinate, the third is the width of the area, and the fourth is the height of the area. 

	/** EXIF Subject Area Type */
	typedef enum eSubjectArea
	{
		kSubjectArea_Min			= 0,

		kSubjectArea_Coordinates	= 0,
		kSubjectArea_Circle			= 1,
		kSubjectArea_Rectangle		= 2,

		kSubjectArea_Max			= 2
	} eSubjectArea;

	EXTERN_BAGKEY(MakerNote);				//VString
	EXTERN_BAGKEY(UserComment);				//VString
	EXTERN_BAGKEY(SubsecTime);				//VString (subsec digits)
	EXTERN_BAGKEY(SubsecTimeOriginal);		//VString (subsec digits)
	EXTERN_BAGKEY(SubsecTimeDigitized);		//VString (subsec digits)
	EXTERN_BAGKEY(FlashPixVersion);			//VString (4 characters only: for instance "0100")
	EXTERN_BAGKEY(ColorSpace);				//VLong
											//	1			= sRGB 
											//	2			= Adobe RGB 
											//	65535		= Uncalibrated 
											//	4294967295	= Uncalibrated
	/** EXIF color space */
	typedef enum eColorSpace
	{
		kColorSpace_Min				= -1,

		kColorSpace_Uncalibrated	= -1,
		kColorSpace_Unknown			=  0,
		kColorSpace_sRGB			=  1,
		kColorSpace_AdobeRGB		=  2,

		kColorSpace_Max				=  2

	} eColorSpace;

	EXTERN_BAGKEY(PixelXDimension);			//VLong
	EXTERN_BAGKEY(PixelYDimension);			//VLong
	EXTERN_BAGKEY(RelatedSoundFile);		//VString
	EXTERN_BAGKEY(FlashEnergy);				//VReal
	EXTERN_BAGKEY(SpatialFrequencyResponse);//VString 
											// base64-encoded binary blob 
											// or 
											// list of string-formatted uchar values separated by ';'
	EXTERN_BAGKEY(FocalPlaneXResolution);	//VReal
	EXTERN_BAGKEY(FocalPlaneYResolution);	//VReal
	EXTERN_BAGKEY(FocalPlaneResolutionUnit);//VLong
											//	1 = None 
											//	2 = inches 
											//	3 = cm 
											//	4 = mm 
											//	5 = um

	EXTERN_BAGKEY(SubjectLocation);			//VString (2 string-formatted integers separated by ';')
											// xy coordinates of the subject in the image
	EXTERN_BAGKEY(ExposureIndex);			//VReal
	EXTERN_BAGKEY(SensingMethod);			//VLong
											//	1 = Not defined 
											//	2 = One-chip color area 
											//	3 = Two-chip color area 
											//	4 = Three-chip color area 
											//	5 = Color sequential area 
											//	7 = Trilinear 
											//	8 = Color sequential linear
	/** EXIF Sensing Method */
	typedef enum eSensingMethod 
	{
		kSensingMethod_Min						= 1,

		kSensingMethod_NotDefined				= 1,
		kSensingMethod_OneChipColorArea			= 2,
		kSensingMethod_TwoChipColorArea			= 3,
		kSensingMethod_ThreeChipColorArea		= 4,
		kSensingMethod_ColorSequentialArea		= 5,
		kSensingMethod_Trilinear				= 7,
		kSensingMethod_ColorSequentialLinear	= 8,

		kSensingMethod_Max						= 8
	} eSensingMethod; 

	EXTERN_BAGKEY(FileSource);				//VLong
											//	1 = Film Scanner 
											//	2 = Reflection Print Scanner 
											//	3 = Digital Camera
	/** EXIF File Source */
	typedef enum eFileSource
	{
		kFileSource_Min						= 1,

		kFileSource_FilmScanner				= 1,
		kFileSource_ReflectionPrintScanner  = 2,
		kFileSource_DigitalCamera			= 3,

		kFileSource_Max						= 3
	} eFileSource;

	EXTERN_BAGKEY(SceneType);				//VLong
											//	1 = Directly photographed
	EXTERN_BAGKEY(CFAPattern);				//VString 
											// base64-encoded binary blob 
											// or 
											// list of string-formatted uchar values separated by ';'
	EXTERN_BAGKEY(CustomRendered);			//VLong
											//	0 = Normal 
											//	1 = Custom
	EXTERN_BAGKEY(ExposureMode);			//VLong
											//	0 = Auto 
											//	1 = Manual 
											//	2 = Auto bracket
	/** EXIF Exposure Mode */
	typedef enum eExposureMode
	{
		kExposureMode_Min			= 0,

		kExposureMode_Auto			= 0,
		kExposureMode_Manual		= 1,
		kExposureMode_AutoBracket	= 2,

		kExposureMode_Max			= 2
	} eExposureMode;

	EXTERN_BAGKEY(WhiteBalance);			//VLong
											//	0 = Auto 
											//	1 = Manual
	EXTERN_BAGKEY(DigitalZoomRatio);		//VReal
	EXTERN_BAGKEY(FocalLenIn35mmFilm);		//VLong
	EXTERN_BAGKEY(SceneCaptureType);		//VLong
											//	0 = Standard 
											//	1 = Landscape 
											//	2 = Portrait 
											//	3 = Night
	/** EXIF Scene Capture Type */
	typedef enum eSceneCaptureType
	{
		kSceneCaptureType_Min		= 0,

		kSceneCaptureType_Standard  = 0,
		kSceneCaptureType_Landscape = 1,
		kSceneCaptureType_Portrait  = 2,
		kSceneCaptureType_Night		= 3,

		kSceneCaptureType_Max		= 3
	} eSceneCaptureType;

	EXTERN_BAGKEY(GainControl);				//VLong
											//	0 = None 
											//	1 = Low gain up 
											//	2 = High gain up 
											//	3 = Low gain down 
											//	4 = High gain down
	/** EXIF Gain Control */
	typedef enum eGainControl
	{
		kGainControl_Min			= 0,

		kGainControl_None			= 0,
		kGainControl_LowGainUp		= 1,
		kGainControl_HighGainUp		= 2,
		kGainControl_LowGainDown	= 3,
		kGainControl_HighGainDown	= 4,

		kGainControl_Max			= 4
	} eGainControl;

	EXTERN_BAGKEY(Contrast);				//VLong
											//	0 = Normal 
											//	1 = Low 
											//	2 = High
	EXTERN_BAGKEY(Saturation);				//VLong
											//	0 = Normal 
											//	1 = Low 
											//	2 = High
	EXTERN_BAGKEY(Sharpness);				//VLong
											//	0 = Normal 
											//	1 = Low 
											//	2 = High
	/** EXIF Contrast/Saturation/Sharpness power */
	typedef enum ePower
	{
		kPower_Min		= 0,

		kPower_Normal	= 0,
		kPower_Low		= 1,
		kPower_High		= 2,

		kPower_Max		= 2
	} ePower;


	EXTERN_BAGKEY(DeviceSettingDescription);//VString 
											// base64-encoded binary blob 
											// or 
											// list of string-formatted uchar values separated by ';'
	EXTERN_BAGKEY(SubjectDistRange);		//VLong
											//	0 = Unknown 
											//	1 = Macro 
											//	2 = Close 
											//	3 = Distant
	/** EXIF Subject Distance Range */
	typedef enum eSubjectDistRange
	{
		kSubjectDistRange_Min		= 0,

		kSubjectDistRange_Unknown	= 0,
		kSubjectDistRange_Macro		= 1,
		kSubjectDistRange_Close		= 2,
		kSubjectDistRange_Distant	= 3,

		kSubjectDistRange_Max		= 3
	} eSubjectDistRange;

	EXTERN_BAGKEY(ImageUniqueID);			//VString
	EXTERN_BAGKEY(Gamma);					//VReal

	//pseudo-tags

	//tags used to read/write exif Flash tag sub-values
	//(for instance 'SET PICTURE META(vPicture;"EXIF/Flash/Fired";true;"EXIF/Flash/Mode";3)'
	//				alias for
	//				'SET PICTURE META(vPicture;"EXIF/Flash";25)'
	//)
	EXTERN_BAGKEY(Fired);					//VBoolean : flash fired status
	EXTERN_BAGKEY(ReturnLight);				//VLong : flash return light status
	EXTERN_BAGKEY(Mode);					//VLong : flash mode
	EXTERN_BAGKEY(FunctionPresent);			//VBoolean : flash function status
	EXTERN_BAGKEY(RedEyeReduction);			//VBoolean : red-eye reduction status

}

//GPS metadatas properties (based on Exif 2.2 (IFD GPS tags): see http://exif.org/Exif2-2.PDF)
namespace ImageMetaGPS
{
	//bag name
	XTOOLBOX_API EXTERN_BAGKEY(GPS);	
	
	//bag attributes
	EXTERN_BAGKEY(VersionID);				//VString (4 uchar string-formatted separated by '.' (XMP format): '2.2.0.0' for instance)
	EXTERN_BAGKEY(Latitude);				//VString with XMP-formatted latitude:
											//	3 string-formatted reals (degrees, minutes and seconds respectively)
											//	separated by ',' (not ';' here because XMP-compliant format)
											//	and followed by 'N' or 'S'
											//  (for instance "10,54,0N")
	EXTERN_BAGKEY(Longitude);				//VString with XMP-formatted longitude: 
											//	3 string-formatted reals (degrees, minutes and seconds respectively)
											//	separated by ',' (not ';' here because XMP-compliant format)
											//  and followed by 'W' or 'E'
											//  (for instance "30,20,0E")
	EXTERN_BAGKEY(AltitudeRef);				//VLong
											//	0 = Above Sea Level 
											//	1 = Below Sea Level
	/** GPS Altitude Ref */
	typedef enum eAltitudeRef
	{
		kAltitudeRef_Min		   = 0,

		kAltitudeRef_AboveSeaLevel = 0,
		kAltitudeRef_BelowSeaLevel = 1,

		kAltitudeRef_Max		   = 1
	} eAltitudeRef;

	EXTERN_BAGKEY(Altitude);				//VReal (altitude in meters depending on reference set by AltitudeRef)
	EXTERN_BAGKEY(Satellites);				//VString 
	EXTERN_BAGKEY(Status);					//VString (one character only)
											//	'A' = Measurement in progress
											//	'V' = Measurement Interoperability
	/** GPS Status */
	typedef enum eStatus
	{
		kStatus_Min							= 0,

		kStatus_MeasurementInProgress		= 0,
		kStatus_MeasurementInteroperability = 1,

		kStatus_Max							= 1
	} eStatus;

	EXTERN_BAGKEY(MeasureMode);				//VLong
											//	2 = 2-Dimensional 
											//	3 = 3-Dimensional
	/** GPS Measure Mode */
	typedef enum eMeasureMode
	{
		kMeasureMode_Min = 2,

		kMeasureMode_2D = 2,
		kMeasureMode_3D = 3,

		kMeasureMode_Max = 3
	} eMeasureMode;

	EXTERN_BAGKEY(DOP);						//VReal
	EXTERN_BAGKEY(SpeedRef);				//VString (one character only)
											//	'K' = km/h 
											//	'M' = miles/h 
											//	'N' = knots
	/** GPS Distance Ref */
	typedef enum eDistanceRef
	{
		kDistanceRef_Min				= 0,

		kDistanceRef_Km					= 0,
		kDistanceRef_Miles				= 1,
		kDistanceRef_NauticalMiles		= 2,

		kDistanceRef_Max				= 2
	} eDistanceRef;

	EXTERN_BAGKEY(Speed);					//VReal
	EXTERN_BAGKEY(TrackRef);				//VString (one character only)
											//	'M' = Magnetic North 
											//	'T' = True North

	/** GPS North Type */
	typedef enum eNorthType
	{
		kNorthType_Min		= 0,

		kNorthType_Magnetic = 0,
		kNorthType_True		= 1,

		kNorthType_Max		= 1
	} eNorthType;

	EXTERN_BAGKEY(Track);					//VReal
	EXTERN_BAGKEY(ImgDirectionRef);			//VString (one character only)
											//	'M' = Magnetic North 
											//	'T' = True North
	EXTERN_BAGKEY(ImgDirection);			//VReal (0.00 to 359.99)
	EXTERN_BAGKEY(MapDatum);				//VString
	EXTERN_BAGKEY(DestLatitude);			//VString with XMP-formatted latitude:
											//	3 string-formatted reals (degrees, minutes and seconds respectively)
											//	separated by ',' (not ';' here because XMP-compliant)
											//	and followed by 'N' or 'S'
											//  (for instance "10,54,0N")
	EXTERN_BAGKEY(DestLongitude);			//VString with XMP-formatted longitude: 
											//	3 string-formatted reals (degrees, minutes and seconds respectively)
											//	separated by ',' (not ';' here because XMP-compliant)
											//  and followed by 'W' or 'E'
											//  (for instance "30,20,0E")
	EXTERN_BAGKEY(DestBearingRef);			//VString (one character only)
											//	'M' = Magnetic North 
											//	'T' = True North
	EXTERN_BAGKEY(DestBearing);				//VReal
	EXTERN_BAGKEY(DestDistanceRef);			//VString (one character only)
											//	'K' = Kilometers 
											//	'M' = Miles 
											//	'N' = Nautical Miles
	EXTERN_BAGKEY(DestDistance);			//VReal	
	EXTERN_BAGKEY(ProcessingMethod);		//VString 
	EXTERN_BAGKEY(AreaInformation);			//VString
	EXTERN_BAGKEY(Differential);			//VLong
											//	0 = No Correction 
											//	1 = Differential Corrected
	EXTERN_BAGKEY(DateTime);				//VTime or VString with XML datetime ("YYYY-MM-DDTHH:MM:SSZ")

	//internal usage only: 
	//following tags are used only to read/write metadatas 
	//(bag datetime type is XML datetime string or VTime
	// and bag latitude and longitude format is XMP format)
	EXTERN_BAGKEY(DateStamp);	
	EXTERN_BAGKEY(TimeStamp);	

	EXTERN_BAGKEY(LatitudeRef);	
	EXTERN_BAGKEY(LongitudeRef);	
	EXTERN_BAGKEY(DestLatitudeRef);	
	EXTERN_BAGKEY(DestLongitudeRef);	

	//pseudo-tags

	//pseudo-tags used to read/write latitude or longitude type tags
	//(for instance 'SET PICTURE METAS(vPicture;"GPS/Latitude/Deg";100;"GPS/Latitude/Min";54;"GPS/Latitude/Sec";32;"GPS/Latitude/Dir";"N")')
	EXTERN_BAGKEY(Deg);		//VReal or VString: normalized degrees (latitude or longitude type tag only)
	EXTERN_BAGKEY(Min);		//VReal or VString: normalized minutes (latitude or longitude type tag only)
	EXTERN_BAGKEY(Sec);		//VReal or VString: normalized seconds (latitude or longitude type tag only)
	EXTERN_BAGKEY(Dir);		//VString: direction ("N", "S", "W" or "E")(latitude or longitude type tag only)
}

/** metadatas helper class */
class IHelperMetadatas
{
protected:
	bool GetString( const VValueBag *inBag, const XBOX::VValueBag::StKey& inBagKey, VString& outValue, 
					VSize inMaxLength = kMAX_VSize) const;
	void SetString( VValueBag *ioBag, const XBOX::VValueBag::StKey& inBagKey, const VString& inValue, 
					VSize inMaxLength = kMAX_VSize);

	template<class T>
	bool GetLong( const VValueBag *inBag, const XBOX::VValueBag::StKey& inBagKey, 
				  T& outValue, 
				  sLONG inMin = kMIN_sLONG,
				  sLONG inMax = kMAX_sLONG,
				  sLONG inDefault = 0) const
	{
		sLONG valueLong;
		if (inBag)
		{
			if (inBag->GetLong( inBagKey, valueLong))
			{
				if (valueLong < inMin || valueLong > inMax)
					valueLong = inDefault;
				try
				{
					outValue = static_cast<T>(valueLong);
				}
				catch(...)
				{
					outValue = static_cast<T>(inDefault); 
					return true;
				}
				return true;
			}	
			else
				return false;
		}
		else
			return false;
	}

	template<class T>
	void SetLong( VValueBag *ioBag, const XBOX::VValueBag::StKey& inBagKey, 
				  const T inValue, 
				  sLONG inMin = kMIN_sLONG,
				  sLONG inMax = kMAX_sLONG,
				  sLONG inDefault = 0) 
	{
		if (ioBag == NULL)
			return;
		sLONG value = static_cast<sLONG>(inValue);
		if (value < inMin || value > inMax)
			value = inDefault;
		ioBag->SetLong( inBagKey, value);
	}

	static bool IsDateTime( const XBOX::VValueBag::StKey& inBagKey)
	{
		XBOX::VString key(inBagKey.GetKeyAdress());
		return key.Find("DateTime") >= 1;
	}

	bool GetDateTime( const VValueBag *inBag, const XBOX::VValueBag::StKey& inBagKey, VTime& outValue) const;
	void SetDateTime( VValueBag *ioBag, const XBOX::VValueBag::StKey& inBagKey, const VTime& inValue);

	bool GetArrayDateTime( const VValueBag *inBag, const XBOX::VValueBag::StKey& inBagKey, std::vector<VTime>& outValue) const;
	void SetArrayDateTime( VValueBag *ioBag, const XBOX::VValueBag::StKey& inBagKey, const std::vector<VTime>& inValue, const XMLStringOptions inXMLOptions = XSO_Default);

	bool GetReal( const VValueBag *inBag, const XBOX::VValueBag::StKey& inBagKey, 
		  Real& outValue, 
		  Real inMin = kMIN_Real,
		  Real inMax = kMAX_Real) const;
	void SetReal( VValueBag *ioBag, const XBOX::VValueBag::StKey& inBagKey, 
		  Real inValue, 
		  Real inMin = kMIN_Real,
		  Real inMax = kMAX_Real);

	bool GetArrayReal( const VValueBag *inBag, const XBOX::VValueBag::StKey& inBagKey, std::vector<Real>& outValue) const;
	void SetArrayReal( VValueBag *ioBag, const XBOX::VValueBag::StKey& inBagKey, const std::vector<Real>& inValue);

	/** get Exif/FlashPix/gps version as a vector of 4 unsigned chars (for instance for exif { 0, 2, 2, 0}) */
	bool GetExifVersion( const VValueBag *inBag, const XBOX::VValueBag::StKey& inBagKey, 
						 std::vector<uCHAR>& outValue) const;
	/** set Exif/FlashPix/gps version as a vector of 4 unsigned chars (for instance for exif { 0, 2, 2, 0}) */
	void SetExifVersion( VValueBag *ioBag, const XBOX::VValueBag::StKey& inBagKey, 
						 const std::vector<uCHAR>& inValue);

	/** get latitude as degree, minute, second and latidude ref */
	bool GetLatitude(	const VString& inValue, 
						Real& outDegree, Real& outMinute, Real& outSecond, bool& outIsNorth) const;

	/** get latitude as degree, minute, second and latidude ref */
	bool GetLatitude(	const VValueBag *inBag, const XBOX::VValueBag::StKey& inBagKey, 
						Real& outDegree, Real& outMinute, Real& outSecond, bool& outIsNorth) const;

	/** set latitude as degree, minute, second and latidude ref */
	void SetLatitude(	VValueBag *ioBag, const XBOX::VValueBag::StKey& inBagKey, 
						const Real inDegree, const Real inMinute, const Real inSecond, const bool inIsNorth);

	/** get latitude as degree, minute, second and latidude ref */
	bool GetLongitude(	const VString& inValue, 
						Real& outDegree, Real& outMinute, Real& outSecond, bool& outIsWest) const;

	/** get longitude as degree, minute, second and longitude ref */
	bool GetLongitude(	const VValueBag *inBag, const XBOX::VValueBag::StKey& inBagKey, 
						Real& outDegree, Real& outMinute, Real& outSecond, bool& outIsWest) const;

	/** set longitude as degree, minute, second and longitude ref */
	void SetLongitude(	VValueBag *ioBag, const XBOX::VValueBag::StKey& inBagKey, 
						const Real inDegree, const Real inMinute, const Real inSecond, const bool inIsWest);
public:
	static bool GetArrayString( const VValueBag *inBag, const XBOX::VValueBag::StKey& inBagKey, VectorOfVString& outValue);
	static void SetArrayString( VValueBag *ioBag, const XBOX::VValueBag::StKey& inBagKey, const VectorOfVString& inValue); 

	template<class SLOT_TYPE>
	static bool GetArrayLong( const VValueBag *inBag, const XBOX::VValueBag::StKey& inBagKey, std::vector<SLOT_TYPE>& outValue, const UniChar inSep = ';') 
	{
		if (inBag)
		{
			VString values;
			if (inBag->GetString( inBagKey, values))
			{
				VectorOfVString vNumber;
				values.GetSubStrings(inSep, vNumber);
				if (vNumber.size() > 0)
				{
					VectorOfVString::const_iterator it = vNumber.begin();
					try
					{
						for(;it != vNumber.end(); it++)
							outValue.push_back( static_cast<SLOT_TYPE>((*it).GetLong()));
					}
					catch(...)
					{
						//invalid value for type T
						return false;
					}
					return true;
				}
				else
					return false;
			}
			else
				return false;
		}
		else
			return false;
	}

	template<class SLOT_TYPE>
	static void SetArrayLong( VValueBag *ioBag, const XBOX::VValueBag::StKey& inBagKey, const std::vector<SLOT_TYPE>& inValue, const UniChar inSep = ';')
	{
		if (ioBag == NULL)
			return;
		if (inValue.size() >= 1)
		{
			VString value;
			value.FromLong(static_cast<sLONG>(inValue[0]));
	#if VERSIONMAC || VERSION_LINUX		
			typename std::vector<SLOT_TYPE>::const_iterator it = inValue.begin();
	#else
			std::vector<SLOT_TYPE>::const_iterator it = inValue.begin();
	#endif
			it++;
			for (;it != inValue.end(); it++)
			{
				value.AppendUniChar(inSep);
				value.AppendLong( static_cast<sLONG>(*it));
			}
			ioBag->SetString( inBagKey, value);
		}
	}
	static bool GetBlob(	const VValueBag *inBag, const XBOX::VValueBag::StKey& inBagKey, 
							VBlob& outValue, bool inBase64Only = false);
	static void SetBlob(	VValueBag *ioBag, const XBOX::VValueBag::StKey& inBagKey, 
							const VBlob& inValue, bool inBase64Only = false);

#if VERSIONMAC
	static bool GetCFArrayChar(	const VValueBag *inBag, const XBOX::VValueBag::StKey& inBagKey, 
							    CFMutableArrayRef& outValue, bool inBase64Only = false);
	static void SetCFArrayChar(	VValueBag *ioBag, const XBOX::VValueBag::StKey& inBagKey, 
							    CFArrayRef inValue, bool inBase64Only = false);
#endif
	
	/** read GPS coordinates from string as degree, minute, second */
	static bool GPSScanCoords(	const VString& inValue, 
						Real& outDegree, Real& outMinute, Real& outSecond,
						const UniChar inSep = (UniChar)',');

	/** write GPS coordinates to string from degree, minute, second */
	static void GPSFormatCoords(	VString& outValue, 
						const Real inDegree, const Real inMinute, const Real inSecond,
						const UniChar inSep = (UniChar)',');

	/* convert Exif 2.2 GPS coordinates value to XMP GPS coordinates value */
	static void GPSCoordsFromExifToXMP(const VString& inExifCoords, const VString& inExifCoordsRef, VString& outXMPCoords);

	/* convert  XMP GPS coordinates value to Exif 2.2 GPS coordinates value */
	static void GPSCoordsFromXMPToExif(const VString& inXMPCoords, VString& outExifCoords, VString& outExifCoordsRef);

	/** convert GPS date+time values to XML datetime value */
	static void DateTimeFromGPSToXML(const VString& inGPSDate, const VString& inGPSTime, VString& outXMLDateTime);

	/** convert XML datetime value to GPS date+time values */
	static void DateTimeFromXMLToGPS(const VString& inXMLDateTime, VString& outGPSDate, VString& outGPSTime);

	/** convert IPTC date+time values to XML datetime value */
	static void DateTimeFromIPTCToXML(const VString& inIPTCDate, const VString& inIPTCTime, VString& outXMLDateTime);

	/** convert XML datetime value to IPTC date+time values */
	static void DateTimeFromXMLToIPTC(const VString& inXMLDateTime, VString& outIPTCDate, VString& outIPTCTime);

	/** convert VTime value to IPTC date+time values */
	static void DateTimeFromVTimeToIPTC(const VTime& inTime, VString& outIPTCDate, VString& outIPTCTime);

	/** convert Exif datetime value to XML datetime value */
	static void DateTimeFromExifToXML(const VString& inExifDateTime, VString& outXMLDateTime);

	/** convert XML datetime value to Exif datetime value */
	static void DateTimeFromXMLToExif(const VString& inXMLDateTime, VString& outExifDateTime);

	/** convert VTime value to Exif datetime value */
	static void DateTimeFromVTimeToExif(const VTime& inTime, VString& outExifDateTime);

	/** convert XMP datetime value to XML datetime value */
	static void DateTimeFromXMPToXML(const VString& inXMPDateTime, VString& outXMLDateTime)
	{
		//XMP datetime format is same as Exif datetime format
		DateTimeFromExifToXML( inXMPDateTime, outXMLDateTime);
	}

	/** convert XML datetime value to XMP datetime value */
	static void DateTimeFromXMLToXMP(const VString& inXMLDateTime, VString& outXMPDateTime) 
	{
		//XMP datetime format is same as Exif datetime format
		DateTimeFromXMLToExif( inXMLDateTime, outXMPDateTime);
	}

	/** convert VTime value to XMP datetime value */
	static void DateTimeFromVTimeToXMP(const VTime& inTime, VString& outXMPDateTime)  
	{
		//XMP datetime format is same as Exif datetime format
		DateTimeFromVTimeToExif( inTime, outXMPDateTime);
	}
};


//metadatas reader/writer classes
namespace ImageMeta 
{
	XTOOLBOX_API extern VValueBag sMapIPTCObjectCycleFromString;
	XTOOLBOX_API extern std::map<sLONG,VString> sMapIPTCObjectCycleFromLong;

	XTOOLBOX_API extern VValueBag sMapIPTCImageTypeFromString;
	XTOOLBOX_API extern std::map<sLONG,VString> sMapIPTCImageTypeFromLong;

	XTOOLBOX_API extern VValueBag sMapIPTCImageOrientationFromString;
	XTOOLBOX_API extern std::map<sLONG,VString> sMapIPTCImageOrientationFromLong;

	XTOOLBOX_API extern VValueBag sMapGPSStatusFromString;
	XTOOLBOX_API extern std::map<sLONG,VString> sMapGPSStatusFromLong;

	XTOOLBOX_API extern VValueBag sMapGPSDistanceRefFromString;
	XTOOLBOX_API extern std::map<sLONG,VString> sMapGPSDistanceRefFromLong;

	XTOOLBOX_API extern VValueBag sMapGPSNorthTypeFromString;
	XTOOLBOX_API extern std::map<sLONG,VString> sMapGPSNorthTypeFromLong;

	/** metadatas reader class
	@remarks
		accessor methods Get...(...) return true if metadata tag exists and if metadata tag value is valid
	*/
	class XTOOLBOX_API stReader : public IHelperMetadatas
	{
	public:
		stReader(const VValueBag *inBagMetas)
		{
			xbox_assert(inBagMetas);
			inBagMetas->Retain();
			fBagRead = inBagMetas;

			fBagReadIPTC = fBagRead->GetUniqueElement( ImageMetaIPTC::IPTC);
			fBagReadTIFF = fBagRead->GetUniqueElement( ImageMetaTIFF::TIFF);
			fBagReadEXIF = fBagRead->GetUniqueElement( ImageMetaEXIF::EXIF);
			fBagReadGPS  = fBagRead->GetUniqueElement( ImageMetaGPS::GPS);
		}
		virtual ~stReader()
		{
			fBagRead->Release();
		}

		static void InitializeGlobals()
		{
			sMapIPTCObjectCycleFromString.SetLong( "a", ImageMetaIPTC::kObjectCycle_Morning);
			sMapIPTCObjectCycleFromString.SetLong( "p", ImageMetaIPTC::kObjectCycle_Evening);
			sMapIPTCObjectCycleFromString.SetLong( "b", ImageMetaIPTC::kObjectCycle_Both);

			sMapIPTCObjectCycleFromLong[ ImageMetaIPTC::kObjectCycle_Morning] = "a";
			sMapIPTCObjectCycleFromLong[ ImageMetaIPTC::kObjectCycle_Evening] = "p";
			sMapIPTCObjectCycleFromLong[ ImageMetaIPTC::kObjectCycle_Both	] = "b";

			sMapIPTCImageTypeFromString.SetLong( "W", ImageMetaIPTC::kImageType_Monochrome	);
			sMapIPTCImageTypeFromString.SetLong( "Y", ImageMetaIPTC::kImageType_Yellow		);
			sMapIPTCImageTypeFromString.SetLong( "M", ImageMetaIPTC::kImageType_Magenta		);
			sMapIPTCImageTypeFromString.SetLong( "C", ImageMetaIPTC::kImageType_Cyan		);
			sMapIPTCImageTypeFromString.SetLong( "K", ImageMetaIPTC::kImageType_Black		);
			sMapIPTCImageTypeFromString.SetLong( "R", ImageMetaIPTC::kImageType_Red			);
			sMapIPTCImageTypeFromString.SetLong( "G", ImageMetaIPTC::kImageType_Green		);
			sMapIPTCImageTypeFromString.SetLong( "B", ImageMetaIPTC::kImageType_Blue		);
			sMapIPTCImageTypeFromString.SetLong( "T", ImageMetaIPTC::kImageType_TextOnly	);
			sMapIPTCImageTypeFromString.SetLong( "F", ImageMetaIPTC::kImageType_FullColourCompositeFrameSequential		);
			sMapIPTCImageTypeFromString.SetLong( "L", ImageMetaIPTC::kImageType_FullColourCompositeLineSequential		);
			sMapIPTCImageTypeFromString.SetLong( "P", ImageMetaIPTC::kImageType_FullColourCompositePixelSequential		);
			sMapIPTCImageTypeFromString.SetLong( "S", ImageMetaIPTC::kImageType_FullColourCompositeSpecialInterleaving	);

			sMapIPTCImageTypeFromLong[ ImageMetaIPTC::kImageType_Monochrome		] = "W";
			sMapIPTCImageTypeFromLong[ ImageMetaIPTC::kImageType_Yellow			] = "Y";
			sMapIPTCImageTypeFromLong[ ImageMetaIPTC::kImageType_Magenta		] = "M";
			sMapIPTCImageTypeFromLong[ ImageMetaIPTC::kImageType_Cyan			] = "C";
			sMapIPTCImageTypeFromLong[ ImageMetaIPTC::kImageType_Black			] = "K";
			sMapIPTCImageTypeFromLong[ ImageMetaIPTC::kImageType_Red			] = "R";
			sMapIPTCImageTypeFromLong[ ImageMetaIPTC::kImageType_Green			] = "G";
			sMapIPTCImageTypeFromLong[ ImageMetaIPTC::kImageType_Blue			] = "B";
			sMapIPTCImageTypeFromLong[ ImageMetaIPTC::kImageType_TextOnly		] = "T";
			sMapIPTCImageTypeFromLong[ ImageMetaIPTC::kImageType_FullColourCompositeFrameSequential	]	 = "F";
			sMapIPTCImageTypeFromLong[ ImageMetaIPTC::kImageType_FullColourCompositeLineSequential	]	 = "L";
			sMapIPTCImageTypeFromLong[ ImageMetaIPTC::kImageType_FullColourCompositePixelSequential	]	 = "P";
			sMapIPTCImageTypeFromLong[ ImageMetaIPTC::kImageType_FullColourCompositeSpecialInterleaving] = "S";
									   
			sMapIPTCImageOrientationFromString.SetLong( "L", ImageMetaIPTC::kImageOrientation_Landscape);
			sMapIPTCImageOrientationFromString.SetLong( "P", ImageMetaIPTC::kImageOrientation_Portrait);
			sMapIPTCImageOrientationFromString.SetLong( "S", ImageMetaIPTC::kImageOrientation_Square);

			sMapIPTCImageOrientationFromLong[ ImageMetaIPTC::kImageOrientation_Landscape] = "L";
			sMapIPTCImageOrientationFromLong[ ImageMetaIPTC::kImageOrientation_Portrait ] = "P";
			sMapIPTCImageOrientationFromLong[ ImageMetaIPTC::kImageOrientation_Square	] = "S";

			sMapGPSStatusFromString.SetLong( "A", ImageMetaGPS::kStatus_MeasurementInProgress	   );
			sMapGPSStatusFromString.SetLong( "V", ImageMetaGPS::kStatus_MeasurementInteroperability);

			sMapGPSStatusFromLong[ ImageMetaGPS::kStatus_MeasurementInProgress			] = "A";
			sMapGPSStatusFromLong[ ImageMetaGPS::kStatus_MeasurementInteroperability	] = "V";

			sMapGPSDistanceRefFromString.SetLong( "K", ImageMetaGPS::kDistanceRef_Km			);
			sMapGPSDistanceRefFromString.SetLong( "M", ImageMetaGPS::kDistanceRef_Miles			);
			sMapGPSDistanceRefFromString.SetLong( "N", ImageMetaGPS::kDistanceRef_NauticalMiles	);

			sMapGPSDistanceRefFromLong[ ImageMetaGPS::kDistanceRef_Km				] = "K";
			sMapGPSDistanceRefFromLong[ ImageMetaGPS::kDistanceRef_Miles			] = "M";
			sMapGPSDistanceRefFromLong[ ImageMetaGPS::kDistanceRef_NauticalMiles	] = "N";

			sMapGPSNorthTypeFromString.SetLong( "M", ImageMetaGPS::kNorthType_Magnetic);
			sMapGPSNorthTypeFromString.SetLong( "T", ImageMetaGPS::kNorthType_True	  );

			sMapGPSNorthTypeFromLong[ ImageMetaGPS::kNorthType_Magnetic] = "M";
			sMapGPSNorthTypeFromLong[ ImageMetaGPS::kNorthType_True	  ]  = "T";
		}
		
	public:
		/** get metadata value as XML-formatted string value */
		bool GetMeta(	const XBOX::VValueBag::StKey& inKeyBlock, 
						const XBOX::VValueBag::StKey& inKeyTag, 
						VString& outValue);

		/** get metadata value as a XML-formatted string array value */
		bool GetMeta(	const XBOX::VValueBag::StKey& inKeyBlock, 
						const XBOX::VValueBag::StKey& inKeyTag, 
						VectorOfVString& outValue);

		/** get metadata value as XML-formatted string value 
		@remarks
			return sub-value associated with pseudo-tag
			(for instance for ImageMetaGPS:Min, return normalized minutes from latitude or longitude type tag) 
		*/
		bool GetMeta(	const XBOX::VValueBag::StKey& inKeyBlock, 
						const XBOX::VValueBag::StKey& inKeyTag, 
						const XBOX::VValueBag::StKey& inKeyPseudoTag, 
						VString& outValue);

		//IPTC reader methods
		bool hasMetaIPTC() const
		{
			return fBagReadIPTC != NULL;
		}

		bool GetIPTCObjectTypeReference( VString& outValue) const
		{
			return GetString( fBagReadIPTC, ImageMetaIPTC::ObjectTypeReference, outValue);
		}

		bool GetIPTCObjectAttributeReference( VString& outValue) const
		{
			return GetString( fBagReadIPTC, ImageMetaIPTC::ObjectAttributeReference, outValue);
		}
		
		bool GetIPTCObjectName( VString& outValue) const
		{
			return GetString( fBagReadIPTC, ImageMetaIPTC::ObjectName, outValue);
		}

		bool GetIPTCEditStatus( VString& outValue) const
		{
			return GetString( fBagReadIPTC, ImageMetaIPTC::EditStatus, outValue);
		}

		bool GetIPTCEditorialUpdate( VString& outValue) const
		{
			return GetString( fBagReadIPTC, ImageMetaIPTC::EditorialUpdate, outValue);
		}

		bool GetIPTCUrgency( uLONG& outValue) const
		{
			return GetLong( fBagReadIPTC, ImageMetaIPTC::Urgency, outValue, 0, 9, 5);
		}

		bool GetIPTCSubjectReference( VectorOfVString& outValue) const
		{
			return GetArrayString( fBagReadIPTC, ImageMetaIPTC::SubjectReference, outValue);
		}

		bool GetIPTCSubjectReference( std::vector<sLONG>& outValue) const
		{
			return GetArrayLong( fBagReadIPTC, ImageMetaIPTC::Scene, outValue);
		}

		bool GetIPTCCategory( VString& outValue) const
		{
			return GetString( fBagReadIPTC, ImageMetaIPTC::Category, outValue);
		}

		bool GetIPTCSupplementalCategory( VectorOfVString& outValue) const
		{
			return GetArrayString( fBagReadIPTC, ImageMetaIPTC::SupplementalCategory, outValue);
		}

		bool GetIPTCFixtureIdentifier( VString& outValue) const
		{
			return GetString( fBagReadIPTC, ImageMetaIPTC::FixtureIdentifier, outValue);
		}

		bool GetIPTCKeywords( VectorOfVString& outValue) const
		{
			return GetArrayString( fBagReadIPTC, ImageMetaIPTC::Keywords, outValue);
		}

		bool GetIPTCContentLocationCode( VectorOfVString& outValue) const
		{
			return GetArrayString( fBagReadIPTC, ImageMetaIPTC::ContentLocationCode, outValue);
		}

		bool GetIPTCContentLocationName( VectorOfVString& outValue) const
		{
			return GetArrayString( fBagReadIPTC, ImageMetaIPTC::ContentLocationName, outValue);
		}

		bool GetIPTCReleaseDateTime( VTime& outValue) const
		{
			return GetDateTime( fBagReadIPTC, ImageMetaIPTC::ReleaseDateTime, outValue);
		}

		bool GetIPTCExpirationDateTime( VTime& outValue) const
		{
			return GetDateTime( fBagReadIPTC, ImageMetaIPTC::ExpirationDateTime, outValue);
		}

		bool GetIPTCSpecialInstructions( VString& outValue) const
		{
			return GetString( fBagReadIPTC, ImageMetaIPTC::SpecialInstructions, outValue);
		}

		bool GetIPTCActionAdvised( ImageMetaIPTC::eActionAdvised& outValue) const
		{
			return GetLong( fBagReadIPTC, ImageMetaIPTC::ActionAdvised, outValue, 
							ImageMetaIPTC::kActionAdvised_Min, ImageMetaIPTC::kActionAdvised_Max,
							ImageMetaIPTC::kActionAdvised_Append);
		}

		bool GetIPTCReferenceService( VectorOfVString& outValue) const
		{
			return GetArrayString( fBagReadIPTC, ImageMetaIPTC::ReferenceService, outValue);
		}

		bool GetIPTCReferenceDateTime( std::vector<VTime>& outValue) const
		{
			return GetArrayDateTime( fBagReadIPTC, ImageMetaIPTC::ReferenceDateTime, outValue);
		}

		bool GetIPTCReferenceNumber( std::vector<uLONG>& outValue) const
		{
			return GetArrayLong( fBagReadIPTC, ImageMetaIPTC::ReferenceNumber, outValue);
		}

		bool GetIPTCDateTimeCreated( VTime& outValue) const
		{
			return GetDateTime( fBagReadIPTC, ImageMetaIPTC::DateTimeCreated, outValue);
		}

		bool GetIPTCDigitalCreationDateTime( VTime& outValue) const
		{
			return GetDateTime( fBagReadIPTC, ImageMetaIPTC::DigitalCreationDateTime, outValue);
		}

		bool GetIPTCOriginatingProgram( VString& outValue) const
		{
			return GetString( fBagReadIPTC, ImageMetaIPTC::OriginatingProgram, outValue);
		}

		bool GetIPTCProgramVersion( VString& outValue) const
		{
			return GetString( fBagReadIPTC, ImageMetaIPTC::ProgramVersion, outValue);
		}

		bool GetIPTCObjectCycle( VString& outValue) const
		{
			return GetString( fBagReadIPTC, ImageMetaIPTC::ObjectCycle, outValue, 1);
		}
		bool GetIPTCObjectCycle( ImageMetaIPTC::eObjectCycle& outValue) const
		{
			VString objectCycle;
			if (GetString( fBagReadIPTC, ImageMetaIPTC::ObjectCycle, objectCycle, 1))
				return sMapIPTCObjectCycleFromString.GetLong( objectCycle, outValue);
			else
				return false;
		}

		bool GetIPTCByline( VectorOfVString& outValue) const
		{
			return GetArrayString( fBagReadIPTC, ImageMetaIPTC::Byline, outValue);
		}
		bool GetIPTCBylineTitle( VectorOfVString& outValue) const
		{
			return GetArrayString( fBagReadIPTC, ImageMetaIPTC::BylineTitle, outValue);
		}

		bool GetIPTCCity( VString& outValue) const
		{
			return GetString( fBagReadIPTC, ImageMetaIPTC::City, outValue);
		}

		bool GetIPTCSubLocation( VString& outValue) const
		{
			return GetString( fBagReadIPTC, ImageMetaIPTC::SubLocation, outValue);
		}

		bool GetIPTCProvinceState( VString& outValue) const
		{
			return GetString( fBagReadIPTC, ImageMetaIPTC::ProvinceState, outValue);
		}

		bool GetIPTCCountryPrimaryLocationCode( VString& outValue) const
		{
			return GetString( fBagReadIPTC, ImageMetaIPTC::CountryPrimaryLocationCode, outValue);
		}

		bool GetIPTCCountryPrimaryLocationName( VString& outValue) const
		{
			return GetString( fBagReadIPTC, ImageMetaIPTC::CountryPrimaryLocationName, outValue);
		}

		bool GetIPTCOriginalTransmissionReference( VString& outValue) const
		{
			return GetString( fBagReadIPTC, ImageMetaIPTC::OriginalTransmissionReference, outValue);
		}

		bool GetIPTCScene( std::vector<ImageMetaIPTC::eScene>& outValue) const
		{
			return GetArrayLong( fBagReadIPTC, ImageMetaIPTC::Scene, outValue);
		}

		bool GetIPTCHeadline( VString& outValue) const
		{
			return GetString( fBagReadIPTC, ImageMetaIPTC::Headline, outValue);
		}

		bool GetIPTCCredit( VString& outValue) const
		{
			return GetString( fBagReadIPTC, ImageMetaIPTC::Credit, outValue);
		}

		bool GetIPTCSource( VString& outValue) const
		{
			return GetString( fBagReadIPTC, ImageMetaIPTC::Source, outValue);
		}

		bool GetIPTCCopyrightNotice( VString& outValue) const
		{
			return GetString( fBagReadIPTC, ImageMetaIPTC::CopyrightNotice, outValue);
		}

		bool GetIPTCContact( VectorOfVString& outValue) const
		{
			return GetArrayString( fBagReadIPTC, ImageMetaIPTC::Contact, outValue);
		}

		bool GetIPTCCaptionAbstract( VString& outValue) const
		{
			return GetString( fBagReadIPTC, ImageMetaIPTC::CaptionAbstract, outValue);
		}

		bool GetIPTCWriterEditor( VectorOfVString& outValue) const
		{
			return GetArrayString( fBagReadIPTC, ImageMetaIPTC::WriterEditor, outValue);
		}

		bool GetIPTCImageType( VSize& outNumComponent, ImageMetaIPTC::eImageType& outImageTypeColourComposition) const
		{
			VString imageType;
			if (GetString( fBagReadIPTC, ImageMetaIPTC::ImageType, imageType))
			{
				if (imageType.GetLength() != 2)
					return false;
				VString numComponent;
				numComponent.AppendUniChar(imageType.GetUniChar(1));
				outNumComponent = numComponent.GetLong();

				VString imageTypeColourComposition;
				imageTypeColourComposition.AppendUniChar(imageType.GetUniChar(2));
				return sMapIPTCImageTypeFromString.GetLong( imageTypeColourComposition, outImageTypeColourComposition);
			}
			else
				return false;
		}

		bool GetIPTCImageOrientation( ImageMetaIPTC::eImageOrientation& outValue) const
		{
			VString imageOrientation;
			if (GetString( fBagReadIPTC, ImageMetaIPTC::ImageOrientation, imageOrientation, 1))
				return sMapIPTCImageOrientationFromString.GetLong( imageOrientation, outValue);
			else
				return false;
		}

		bool GetIPTCLanguageIdentifier( VString& outValue) const
		{
			return GetString( fBagReadIPTC, ImageMetaIPTC::LanguageIdentifier, outValue);
		}

		bool GetIPTCStarRating( uLONG& outValue) const
		{
			return GetLong( fBagReadIPTC, ImageMetaIPTC::StarRating, outValue, 0, 5, 0);
		}


		//TIFF reader methods
		bool hasMetaTIFF() const
		{
			return fBagReadTIFF != NULL;
		}

		bool GetTIFFCompression( ImageMetaTIFF::eCompression& outValue) const
		{
			return GetLong( fBagReadTIFF, ImageMetaTIFF::Compression, outValue, 
							ImageMetaTIFF::kCompression_Min, ImageMetaTIFF::kCompression_Max,
							ImageMetaTIFF::kCompression_Min);
		}

		bool GetTIFFPhotometricInterpretation( ImageMetaTIFF::ePhotometricInterpretation& outValue) const
		{
			return GetLong( fBagReadTIFF, ImageMetaTIFF::PhotometricInterpretation, outValue, 
							ImageMetaTIFF::kPhotometricInterpretation_Min, ImageMetaTIFF::kPhotometricInterpretation_Max,
							ImageMetaTIFF::kPhotometricInterpretation_RGB);
		}

		bool GetTIFFDocumentName( VString& outValue) const
		{
			return GetString( fBagReadTIFF, ImageMetaTIFF::DocumentName, outValue);
		}

		bool GetTIFFImageDescription( VString& outValue) const
		{
			return GetString( fBagReadTIFF, ImageMetaTIFF::ImageDescription, outValue);
		}

		bool GetTIFFMake( VString& outValue) const
		{
			return GetString( fBagReadTIFF, ImageMetaTIFF::Make, outValue);
		}

		bool GetTIFFModel( VString& outValue) const
		{
			return GetString( fBagReadTIFF, ImageMetaTIFF::Model, outValue);
		}

		bool GetTIFFOrientation( ImageMetaTIFF::eOrientation& outValue) const
		{
			return GetLong( fBagReadTIFF, ImageMetaTIFF::Orientation, outValue, 
							ImageMetaTIFF::kOrientation_Min, ImageMetaTIFF::kOrientation_Max,
							ImageMetaTIFF::kOrientation_Horizontal);
		}

		bool GetTIFFXResolution( Real& outValue) const
		{
			return GetReal( fBagReadTIFF, ImageMetaTIFF::XResolution, outValue);
		}

		bool GetTIFFYResolution( Real& outValue) const
		{
			return GetReal( fBagReadTIFF, ImageMetaTIFF::YResolution, outValue);
		}

		bool GetTIFFResolutionUnit( ImageMetaTIFF::eResolutionUnit& outValue) const
		{
			return GetLong( fBagReadTIFF, ImageMetaTIFF::ResolutionUnit, outValue, 
							ImageMetaTIFF::kResolutionUnit_Min, ImageMetaTIFF::kResolutionUnit_Max,
							ImageMetaTIFF::kResolutionUnit_Inches);
		}

		bool GetTIFFSoftware( VString& outValue) const
		{
			return GetString( fBagReadTIFF, ImageMetaTIFF::Software, outValue);
		}

		bool GetTIFFTransferFunction( std::vector<uLONG>& outValue) const
		{
			bool ok = GetArrayLong( fBagReadTIFF, ImageMetaTIFF::TransferFunction, outValue);
			if (!ok) 
				return false;
			return outValue.size() == 3*256;
		}

		bool GetTIFFDateTime( VTime& outValue) const
		{
			return GetDateTime( fBagReadTIFF, ImageMetaTIFF::DateTime, outValue);
		}

		bool GetTIFFArtist( VString& outValue) const
		{
			return GetString( fBagReadTIFF, ImageMetaTIFF::Artist, outValue);
		}

		bool GetTIFFHostComputer( VString& outValue) const
		{
			return GetString( fBagReadTIFF, ImageMetaTIFF::HostComputer, outValue);
		}

		bool GetTIFFCopyright( VString& outValue) const
		{
			return GetString( fBagReadTIFF, ImageMetaTIFF::Copyright, outValue);
		}

		bool GetTIFFWhitePoint( VPoint& outWhitePointChromaticity) const
		{
			std::vector<Real> values;
			bool ok = GetArrayReal( fBagReadTIFF, ImageMetaTIFF::WhitePoint, values);
			if (!ok)
				return false;
			if (values.size() == 2)
			{
				outWhitePointChromaticity.x = (GReal) values[0];
				outWhitePointChromaticity.y = (GReal) values[1];
				return true;
			}
			else
				return false;
		}

		bool GetTIFFPrimaryChromaticities(  VPoint& outRedChromaticity,
										    VPoint& outGreenChromaticity,
										    VPoint& outBlueChromaticity) const
		{
			std::vector<Real> values;
			bool ok = GetArrayReal( fBagReadTIFF, ImageMetaTIFF::PrimaryChromaticities, values);
			if (!ok)
				return false;
			if (values.size() == 6)
			{
				outRedChromaticity.x	= (GReal) values[0];
				outRedChromaticity.y	= (GReal) values[1];
				outGreenChromaticity.x	= (GReal) values[2];
				outGreenChromaticity.y	= (GReal) values[3];
				outBlueChromaticity.x	= (GReal) values[4];
				outBlueChromaticity.y	= (GReal) values[5];
				return true;
			}
			else
				return false;
		}

		//EXIF reader methods
		bool hasMetaEXIF() const
		{
			return fBagReadEXIF != NULL;
		}

		bool GetEXIFExposureTime( Real& outValue) const
		{
			return GetReal( fBagReadEXIF, ImageMetaEXIF::ExposureTime, outValue);
		}

		bool GetEXIFFNumber( Real& outValue) const
		{
			return GetReal( fBagReadEXIF, ImageMetaEXIF::FNumber, outValue);
		}

		bool GetEXIFExposureProgram( ImageMetaEXIF::eExposureProgram& outValue) const
		{
			return GetLong( fBagReadEXIF, ImageMetaEXIF::ExposureProgram, outValue, 
							ImageMetaEXIF::kExposureProgram_Min, 
							ImageMetaEXIF::kExposureProgram_Max,
							ImageMetaEXIF::kExposureProgram_Portrait);
		}

		bool GetEXIFSpectralSensitivity( VString& outValue) const
		{
			return GetString( fBagReadEXIF, ImageMetaEXIF::SpectralSensitivity, outValue);
		}

		bool GetEXIFISOSpeedRatings( std::vector<uLONG>& outValue) const
		{
			return GetArrayLong( fBagReadEXIF, ImageMetaEXIF::ISOSpeedRatings, outValue);
		}

		bool GetEXIFOECF( VBlob& outValue) const
		{
			return GetBlob( fBagReadEXIF, ImageMetaEXIF::OECF, outValue);
		}

		bool GetEXIFVersion( uCHAR& outNumber1,
							 uCHAR& outNumber2,
							 uCHAR& outNumber3,
							 uCHAR& outNumber4) const 
		{
			std::vector<uCHAR> values;
			bool ok = GetExifVersion( fBagReadEXIF, ImageMetaEXIF::ExifVersion, values);
			if (!ok)
				return false;
			if (values.size() != 4)
				return false;
			outNumber1 = values[0];
			outNumber2 = values[1];
			outNumber3 = values[2];
			outNumber4 = values[3];
			return true;
		}

		bool GetEXIFDateTimeOriginal( VTime& outValue) const
		{
			return GetDateTime( fBagReadEXIF, ImageMetaEXIF::DateTimeOriginal, outValue);
		}

		bool GetEXIFDateTimeDigitized( VTime& outValue) const
		{
			return GetDateTime( fBagReadEXIF, ImageMetaEXIF::DateTimeDigitized, outValue);
		}

		bool GetEXIFComponentsConfiguration( ImageMetaEXIF::eComponentType& outChannel1, 
											 ImageMetaEXIF::eComponentType& outChannel2, 
											 ImageMetaEXIF::eComponentType& outChannel3,
											 ImageMetaEXIF::eComponentType& outChannel4) const
		{
			std::vector<ImageMetaEXIF::eComponentType> values;
			bool ok = GetArrayLong( fBagReadEXIF, ImageMetaEXIF::ComponentsConfiguration, values);
			if (!ok) 
				return false;
			if (values.size() != 4)
				return false;
			outChannel1 = values[0];
			outChannel2 = values[1];
			outChannel3 = values[2];
			outChannel4 = values[3];
			return true;
		}

		bool GetEXIFCompressedBitsPerPixel( Real& outValue) const
		{
			return GetReal( fBagReadEXIF, ImageMetaEXIF::CompressedBitsPerPixel, outValue);
		}

		bool GetEXIFShutterSpeedValue( Real& outValue) const
		{
			return GetReal( fBagReadEXIF, ImageMetaEXIF::ShutterSpeedValue, outValue);
		}

		bool GetEXIFApertureValue( Real& outValue) const
		{
			return GetReal( fBagReadEXIF, ImageMetaEXIF::ApertureValue, outValue);
		}

		bool GetEXIFBrightnessValue( Real& outValue) const
		{
			return GetReal( fBagReadEXIF, ImageMetaEXIF::BrightnessValue, outValue);
		}

		bool GetEXIFExposureBiasValue( Real& outValue) const
		{
			return GetReal( fBagReadEXIF, ImageMetaEXIF::ExposureBiasValue, outValue);
		}

		bool GetEXIFMaxApertureValue( Real& outValue) const
		{
			return GetReal( fBagReadEXIF, ImageMetaEXIF::MaxApertureValue, outValue);
		}

		bool GetEXIFSubjectDistance( Real& outValue) const
		{
			return GetReal( fBagReadEXIF, ImageMetaEXIF::SubjectDistance, outValue);
		}

		bool GetEXIFMeteringMode( ImageMetaEXIF::eMeteringMode& outValue) const
		{
			return GetLong( fBagReadEXIF, ImageMetaEXIF::MeteringMode, outValue, 
							ImageMetaEXIF::kMeteringMode_Min, ImageMetaEXIF::kMeteringMode_Max,
							ImageMetaEXIF::kMeteringMode_Other);
		}

		bool GetEXIFLightSource( ImageMetaEXIF::eLightSource& outValue) const
		{
			return GetLong( fBagReadEXIF, ImageMetaEXIF::LightSource, outValue, 
							ImageMetaEXIF::kLightSource_Min, ImageMetaEXIF::kLightSource_Max,
							ImageMetaEXIF::kLightSource_Other);
		}

		/** get EXIF flash status
		@param outFired
				true if flash is fired
		@param outReturn (optional)
				flash return mode
		@param outMode (optional)
				flash mode
		@param outFunctionPresent (optional)
				true if presence of a flash function
		@param outRedEyeReduction (optional)
				true if red-eye reduction is supported
		*/
		bool GetEXIFFlash( bool& outFired, 
						   ImageMetaEXIF::eFlashReturn *outReturn = NULL,  
						   ImageMetaEXIF::eFlashMode *outMode = NULL,
						   bool *outFunctionPresent = NULL,
						   bool *outRedEyeReduction = NULL) const;


		bool GetEXIFFocalLength( Real& outValue) const
		{
			return GetReal( fBagReadEXIF, ImageMetaEXIF::FocalLength, outValue);
		}

		bool GetEXIFSubjectAreaType(		ImageMetaEXIF::eSubjectArea& outValue) const;
		bool GetEXIFSubjectAreaCenter(		uLONG& outCenterX, uLONG& outCenterY) const;
		bool GetEXIFSubjectAreaCircle(		uLONG& outCenterX, uLONG& outCenterY, 
											uLONG& outDiameter) const;
		bool GetEXIFSubjectAreaRectangle(	uLONG& outCenterX, uLONG& outCenterY, 
											uLONG& outWidth, uLONG& outHeight) const;

		bool GetEXIFMakerNote( VString& outValue) const
		{
			return GetString( fBagReadEXIF, ImageMetaEXIF::MakerNote, outValue);
		}

		bool GetEXIFUserComment( VString& outValue) const
		{
			return GetString( fBagReadEXIF, ImageMetaEXIF::UserComment, outValue);
		}

		bool GetEXIFFlashPixVersion( uCHAR& outNumber1,
									 uCHAR& outNumber2,
									 uCHAR& outNumber3,
									 uCHAR& outNumber4) const 
		{
			std::vector<uCHAR> values;
			bool ok = GetExifVersion( fBagReadEXIF, ImageMetaEXIF::FlashPixVersion, values);
			if (!ok)
				return false;
			if (values.size() != 4)
				return false;
			outNumber1 = values[0];
			outNumber2 = values[1];
			outNumber3 = values[2];
			outNumber4 = values[3];
			return true;
		}

		bool GetEXIFColorSpace( ImageMetaEXIF::eColorSpace& outValue) const
		{
			sLONG value;
			if (!GetLong( fBagReadEXIF, ImageMetaEXIF::ColorSpace, value))
				return false;
			if (value == 0xffff) //because it can be read from a ushort
				outValue = ImageMetaEXIF::kColorSpace_Uncalibrated;
			else if (value == 0xffffffff) 
				outValue = ImageMetaEXIF::kColorSpace_Uncalibrated;
			else if (value >= ImageMetaEXIF::kColorSpace_Min && value <= ImageMetaEXIF::kColorSpace_Max)
				outValue = static_cast<ImageMetaEXIF::eColorSpace>(value);
			else
				outValue = ImageMetaEXIF::kColorSpace_Unknown;
			return true;
		}

		bool GetEXIFPixelXDimension( uLONG& outValue) const
		{
			return GetLong( fBagReadEXIF, ImageMetaEXIF::PixelXDimension, outValue);
		}

		bool GetEXIFPixelYDimension( uLONG& outValue) const
		{
			return GetLong( fBagReadEXIF, ImageMetaEXIF::PixelYDimension, outValue);
		}

		bool GetEXIFRelatedSoundFile( VString& outValue) const
		{
			return GetString( fBagReadEXIF, ImageMetaEXIF::RelatedSoundFile, outValue);
		}

		bool GetEXIFFlashEnergy( Real& outValue) const
		{
			return GetReal( fBagReadEXIF, ImageMetaEXIF::FlashEnergy, outValue);
		}

		bool GetEXIFSpatialFrequencyResponse( VBlob& outValue) const
		{
			return GetBlob( fBagReadEXIF, ImageMetaEXIF::SpatialFrequencyResponse, outValue);
		}

		bool GetEXIFFocalPlaneXResolution( Real& outValue) const
		{
			return GetReal( fBagReadEXIF, ImageMetaEXIF::FocalPlaneXResolution, outValue);
		}

		bool GetEXIFFocalPlaneYResolution( Real& outValue) const
		{
			return GetReal( fBagReadEXIF, ImageMetaEXIF::FocalPlaneYResolution, outValue);
		}

		bool GetEXIFFocalPlaneResolutionUnit( ImageMetaTIFF::eResolutionUnit& outValue) const
		{
			return GetLong( fBagReadEXIF, ImageMetaEXIF::FocalPlaneResolutionUnit, outValue, 
							ImageMetaTIFF::kResolutionUnit_Min, ImageMetaTIFF::kResolutionUnit_Max,
							ImageMetaTIFF::kResolutionUnit_Inches);
		}

		bool GetEXIFSubjectLocation( uLONG& outCenterX, uLONG& outCenterY) const;

		bool GetEXIFExposureIndex( Real& outValue) const
		{
			return GetReal( fBagReadEXIF, ImageMetaEXIF::ExposureIndex, outValue);
		}

		bool GetEXIFSensingMethod( ImageMetaEXIF::eSensingMethod& outValue) const
		{
			return GetLong( fBagReadEXIF, ImageMetaEXIF::SensingMethod, outValue, 
							ImageMetaEXIF::kSensingMethod_Min, ImageMetaEXIF::kSensingMethod_Max,
							ImageMetaEXIF::kSensingMethod_NotDefined);
		}

		bool GetEXIFFileSource( ImageMetaEXIF::eFileSource& outValue) const
		{
			return GetLong( fBagReadEXIF, ImageMetaEXIF::FileSource, outValue, 
							ImageMetaEXIF::kFileSource_Min, ImageMetaEXIF::kFileSource_Max,
							ImageMetaEXIF::kFileSource_DigitalCamera);
		}

		bool GetEXIFSceneType( bool& outDirectlyPhotographed) const
		{
			sLONG sceneType;
			if (!GetLong( fBagReadEXIF, ImageMetaEXIF::SceneType, sceneType))
				return false;
			outDirectlyPhotographed = sceneType == 1;
			return true;
		}

		bool GetEXIFCFAPattern( VBlob& outValue) const
		{
			return GetBlob( fBagReadEXIF, ImageMetaEXIF::CFAPattern, outValue);
		}

		bool GetEXIFCustomRendered( bool& outCustomRendered) const
		{
			sLONG customRendered;
			if (!GetLong( fBagReadEXIF, ImageMetaEXIF::CustomRendered, customRendered))
				return false;
			outCustomRendered = customRendered == 1;
			return true;
		}

		bool GetEXIFExposureMode( ImageMetaEXIF::eExposureMode& outValue) const
		{
			return GetLong( fBagReadEXIF, ImageMetaEXIF::ExposureMode, outValue, 
							ImageMetaEXIF::kExposureMode_Min, ImageMetaEXIF::kExposureMode_Max,
							ImageMetaEXIF::kExposureMode_Auto);
		}

		bool GetEXIFWhiteBalance( bool& outIsManual) const
		{
			sLONG whiteBalance;
			if (!GetLong( fBagReadEXIF, ImageMetaEXIF::WhiteBalance, whiteBalance))
				return false;
			outIsManual = whiteBalance == 1;
			return true;
		}

		bool GetEXIFDigitalZoomRatio( Real& outValue) const
		{
			return GetReal( fBagReadEXIF, ImageMetaEXIF::DigitalZoomRatio, outValue);
		}

		bool GetEXIFFocalLenIn35mmFilm( uLONG& outValue) const
		{
			return GetLong( fBagReadEXIF, ImageMetaEXIF::FocalLenIn35mmFilm, outValue);
		}

		bool GetEXIFSceneCaptureType( ImageMetaEXIF::eSceneCaptureType& outValue) const
		{
			return GetLong( fBagReadEXIF, ImageMetaEXIF::SceneCaptureType, outValue, 
							ImageMetaEXIF::kSceneCaptureType_Min, ImageMetaEXIF::kSceneCaptureType_Max,
							ImageMetaEXIF::kSceneCaptureType_Standard);
		}

		bool GetEXIFGainControl( ImageMetaEXIF::eGainControl& outValue) const
		{
			return GetLong( fBagReadEXIF, ImageMetaEXIF::GainControl, outValue, 
							ImageMetaEXIF::kGainControl_Min, ImageMetaEXIF::kGainControl_Max,
							ImageMetaEXIF::kGainControl_None);
		}

		bool GetEXIFContrast( ImageMetaEXIF::ePower& outValue) const
		{
			return GetLong( fBagReadEXIF, ImageMetaEXIF::Contrast, outValue, 
							ImageMetaEXIF::kPower_Min, ImageMetaEXIF::kPower_Max,
							ImageMetaEXIF::kPower_Normal);
		}

		bool GetEXIFSaturation( ImageMetaEXIF::ePower& outValue) const
		{
			return GetLong( fBagReadEXIF, ImageMetaEXIF::Saturation, outValue, 
							ImageMetaEXIF::kPower_Min, ImageMetaEXIF::kPower_Max,
							ImageMetaEXIF::kPower_Normal);
		}

		bool GetEXIFSharpness( ImageMetaEXIF::ePower& outValue) const
		{
			return GetLong( fBagReadEXIF, ImageMetaEXIF::Sharpness, outValue, 
							ImageMetaEXIF::kPower_Min, ImageMetaEXIF::kPower_Max,
							ImageMetaEXIF::kPower_Normal);
		}

		bool GetEXIFDeviceSettingDescription( VBlob& outValue) const
		{
			return GetBlob( fBagReadEXIF, ImageMetaEXIF::DeviceSettingDescription, outValue);
		}

		bool GetEXIFSubjectDistRange( ImageMetaEXIF::eSubjectDistRange& outValue) const
		{
			return GetLong( fBagReadEXIF, ImageMetaEXIF::SubjectDistRange, outValue, 
							ImageMetaEXIF::kSubjectDistRange_Min, ImageMetaEXIF::kSubjectDistRange_Max,
							ImageMetaEXIF::kSubjectDistRange_Unknown);
		}

		bool GetEXIFImageUniqueID( VString& outValue) const
		{
			return GetString( fBagReadEXIF, ImageMetaEXIF::ImageUniqueID, outValue);
		}

		bool GetEXIFGamma( Real& outValue) const
		{
			return GetReal( fBagReadEXIF, ImageMetaEXIF::Gamma, outValue);
		}

		//GPS reader methods
		bool hasMetaGPS() const
		{
			return fBagReadGPS != NULL;
		}

		bool GetGPSVersionID( uCHAR& outNumber1,
							  uCHAR& outNumber2,
							  uCHAR& outNumber3,
							  uCHAR& outNumber4) const 
		{
			std::vector<uCHAR> values;
			bool ok = GetExifVersion( fBagReadGPS, ImageMetaGPS::VersionID, values);
			if (!ok)
				return false;
			if (values.size() != 4)
				return false;
			outNumber1 = values[0];
			outNumber2 = values[1];
			outNumber3 = values[2];
			outNumber4 = values[3];
			return true;
		}

		bool GetGPSLatitude( Real& outDegree, Real& outMinute, Real& outSecond, bool& outIsNorth) const
		{
			return GetLatitude( fBagReadGPS, ImageMetaGPS::Latitude, outDegree, outMinute, outSecond, outIsNorth);
		}

		bool GetGPSLongitude( Real& outDegree, Real& outMinute, Real& outSecond, bool& outIsWest) const
		{
			return GetLongitude( fBagReadGPS, ImageMetaGPS::Longitude, outDegree, outMinute, outSecond, outIsWest);
		}

		bool GetGPSAltitude( Real& outAltitude, ImageMetaGPS::eAltitudeRef& outAltitudeRef) const
		{
			if (!GetReal( fBagReadGPS, ImageMetaGPS::Altitude, outAltitude))
				return false;
			return GetLong( fBagReadGPS, ImageMetaGPS::AltitudeRef, outAltitudeRef,
							ImageMetaGPS::kAltitudeRef_Min, ImageMetaGPS::kAltitudeRef_Max,
							ImageMetaGPS::kAltitudeRef_AboveSeaLevel);
		}	

		bool GetGPSSatellites( VString& outValue) const
		{
			return GetString( fBagReadGPS, ImageMetaGPS::Satellites, outValue);
		}

		bool GetGPSStatus( ImageMetaGPS::eStatus& outValue) const
		{
			VString value;
			if (!GetString( fBagReadGPS, ImageMetaGPS::Status, value))
				return false;
			return sMapGPSStatusFromString.GetLong( value, outValue);
		}

		bool GetGPSMeasureMode( ImageMetaGPS::eMeasureMode& outValue) const
		{
			return GetLong( fBagReadGPS, ImageMetaGPS::MeasureMode, outValue,
							ImageMetaGPS::kMeasureMode_Min, ImageMetaGPS::kMeasureMode_Max,
							ImageMetaGPS::kMeasureMode_2D);
		}

		bool GetGPSDOP( Real& outValue) const
		{
			return GetReal( fBagReadGPS, ImageMetaGPS::DOP, outValue);
		}

		bool GetGPSSpeed( Real& outSpeed, ImageMetaGPS::eDistanceRef& outSpeedRef) const
		{
			if (!GetReal( fBagReadGPS, ImageMetaGPS::Speed, outSpeed))
				return false;
			VString value;
			if (!GetString( fBagReadGPS, ImageMetaGPS::SpeedRef, value))
				return false;
			return sMapGPSDistanceRefFromString.GetLong( value, outSpeedRef);
		}	

		bool GetGPSTrack( Real& outTrack, ImageMetaGPS::eNorthType& outTrackRef) const
		{
			if (!GetReal( fBagReadGPS, ImageMetaGPS::Track, outTrack))
				return false;
			VString value;
			if (!GetString( fBagReadGPS, ImageMetaGPS::TrackRef, value))
				return false;
			return sMapGPSNorthTypeFromString.GetLong( value, outTrackRef);
		}	

		bool GetGPSImgDirection( Real& outImgDirection, ImageMetaGPS::eNorthType& outImgDirectionRef) const
		{
			if (!GetReal( fBagReadGPS, ImageMetaGPS::ImgDirection, outImgDirection))
				return false;
			VString value;
			if (!GetString( fBagReadGPS, ImageMetaGPS::ImgDirectionRef, value))
				return false;
			return sMapGPSNorthTypeFromString.GetLong( value, outImgDirectionRef);
		}	

		bool GetGPSMapDatum( VString& outValue) const
		{
			return GetString( fBagReadGPS, ImageMetaGPS::MapDatum, outValue);
		}

		bool GetGPSDestLatitude( Real& outDegree, Real& outMinute, Real& outSecond, bool& outIsNorth) const
		{
			return GetLatitude( fBagReadGPS, ImageMetaGPS::DestLatitude, outDegree, outMinute, outSecond, outIsNorth);
		}

		bool GetGPSDestLongitude( Real& outDegree, Real& outMinute, Real& outSecond, bool& outIsWest) const
		{
			return GetLongitude( fBagReadGPS, ImageMetaGPS::DestLongitude, outDegree, outMinute, outSecond, outIsWest);
		}

		bool GetGPSDestBearing( Real& outDestBearing, ImageMetaGPS::eNorthType& outDestBearingRef) const
		{
			if (!GetReal( fBagReadGPS, ImageMetaGPS::DestBearing, outDestBearing))
				return false;
			VString value;
			if (!GetString( fBagReadGPS, ImageMetaGPS::DestBearingRef, value))
				return false;
			return sMapGPSNorthTypeFromString.GetLong( value, outDestBearingRef);
		}	

		bool GetGPSDestDistance( Real& outDestDistance, ImageMetaGPS::eDistanceRef& outDestDistanceRef) const
		{
			if (!GetReal( fBagReadGPS, ImageMetaGPS::DestDistance, outDestDistance))
				return false;
			VString value;
			if (!GetString( fBagReadGPS, ImageMetaGPS::DestDistanceRef, value))
				return false;
			return sMapGPSDistanceRefFromString.GetLong( value, outDestDistanceRef);
		}	

		bool GetGPSProcessingMethod( VString& outValue) const
		{
			return GetString( fBagReadGPS, ImageMetaGPS::ProcessingMethod, outValue);
		}

		bool GetGPSAreaInformation( VString& outValue) const
		{
			return GetString( fBagReadGPS, ImageMetaGPS::AreaInformation, outValue);
		}

		bool GetGPSDifferential( bool& outDifferentialCorrected) const
		{
			sLONG value;
			if (!GetLong( fBagReadGPS, ImageMetaGPS::Differential, value))
				return false;
			outDifferentialCorrected = value == 1;
			return true;
		}

		bool GetGPSDateTime( VTime& outValue) const
		{
			return GetDateTime( fBagReadGPS, ImageMetaGPS::DateTime, outValue);
		}

	protected:
		mutable const VValueBag *fBagRead;
		const VValueBag *fBagReadIPTC;
		const VValueBag *fBagReadTIFF;
		const VValueBag *fBagReadEXIF;
		const VValueBag *fBagReadGPS;
	};

	//metadatas writing helper class
	class XTOOLBOX_API stWriter : public IHelperMetadatas
	{
	public:
		stWriter(VValueBag *inBagMetas)
		{
			xbox_assert(inBagMetas);
			inBagMetas->Retain();
			fBagWrite = inBagMetas;

			fBagWriteIPTC = fBagWrite->GetUniqueElement( ImageMetaIPTC::IPTC);
			fBagWriteTIFF = fBagWrite->GetUniqueElement( ImageMetaTIFF::TIFF);
			fBagWriteEXIF = fBagWrite->GetUniqueElement( ImageMetaEXIF::EXIF);
			fBagWriteGPS  = fBagWrite->GetUniqueElement( ImageMetaGPS::GPS);
		}
		virtual ~stWriter()
		{
			fBagWrite->Release();
		}

		/** set metadata value from a VValueSingle value */
		bool SetMeta(	const XBOX::VValueBag::StKey& inKeyBlock, 
						const XBOX::VValueBag::StKey& inKeyTag, 
						const VValueSingle& inValue);

		/** set metadata value from a XML-formatted string value */
		bool SetMeta(	const XBOX::VValueBag::StKey& inKeyBlock, 
						const XBOX::VValueBag::StKey& inKeyTag, 
						const VString& inValue);

		/** set metadata value from a XML-formatted string array value */
		bool SetMeta(	const XBOX::VValueBag::StKey& inKeyBlock, 
						const XBOX::VValueBag::StKey& inKeyTag, 
						const VectorOfVString& inValue);

		/** remove metadata */
		bool RemoveMeta(const XBOX::VValueBag::StKey& inKeyBlock, 
						const XBOX::VValueBag::StKey& inKeyTag);

		/** set metadata value from a VValueSingle value 
		@remarks
			set only sub-value associated with pseudo-tag
			(for instance for ImageMetaGPS:Min, update only normalized minutes from latitude or longitude type tag) 
		*/
		bool SetMeta(	const XBOX::VValueBag::StKey& inKeyBlock, 
						const XBOX::VValueBag::StKey& inKeyTag, 
						const XBOX::VValueBag::StKey& inKeyPseudoTag, 
						const VValueSingle& inValue,
						const VValueBag *inMetasSource = NULL);

		/** set metadata value from a XML-formatted string value 
		@remarks
			set only sub-value associated with pseudo-tag
			(for instance for ImageMetaGPS:Min, update only normalized minutes from latitude or longitude type tag) 
		*/
		bool SetMeta(	const XBOX::VValueBag::StKey& inKeyBlock, 
						const XBOX::VValueBag::StKey& inKeyTag, 
						const XBOX::VValueBag::StKey& inKeyPseudoTag, 
						const VString& inValue,
						const VValueBag *inMetasSource = NULL);

		//IPTC writer methods
	protected:
		void addMetaIPTC() 
		{
			//ensure IPTC element exists

			if (fBagWriteIPTC != NULL)
				return;
			fBagWriteIPTC = fBagWrite->GetUniqueElement( ImageMetaIPTC::IPTC);
			if (fBagWriteIPTC != NULL)
				return;
			VValueBag *bagIPTC = new VValueBag();
			fBagWrite->AddElement( ImageMetaIPTC::IPTC, bagIPTC);
			fBagWriteIPTC = bagIPTC;
			bagIPTC->Release();
		}
	public:
		void SetIPTCObjectTypeReference( const VString& inValue)
		{
			addMetaIPTC();
			SetString( fBagWriteIPTC, ImageMetaIPTC::ObjectTypeReference, inValue);
		}

		void SetIPTCObjectAttributeReference( const VString& inValue)
		{
			addMetaIPTC();
			SetString( fBagWriteIPTC, ImageMetaIPTC::ObjectAttributeReference, inValue);
		}
		
		void SetIPTCObjectName( const VString& inValue)
		{
			addMetaIPTC();
			SetString( fBagWriteIPTC, ImageMetaIPTC::ObjectName, inValue);
		}

		void SetIPTCEditStatus( const VString& inValue)
		{
			addMetaIPTC();
			SetString( fBagWriteIPTC, ImageMetaIPTC::EditStatus, inValue);
		}

		void SetIPTCEditorialUpdate( const VString& inValue)
		{
			addMetaIPTC();
			SetString( fBagWriteIPTC, ImageMetaIPTC::EditorialUpdate, inValue);
		}

		bool SetIPTCUrgency( const uLONG inValue = 5)
		{
			addMetaIPTC();
			bool ok = /*inValue >= 0 && */ inValue <= 9;
			if (ok)
				SetLong( fBagWriteIPTC, ImageMetaIPTC::Urgency, inValue);
			return ok;
		}

		bool SetIPTCSubjectReference( const VectorOfVString& inValue);
		bool SetIPTCSubjectReference( const std::vector<sLONG>& inValue);

		void SetIPTCCategory( const VString& inValue)
		{
			addMetaIPTC();
			SetString( fBagWriteIPTC, ImageMetaIPTC::Category, inValue);
		}

		void SetIPTCSupplementalCategory( const VectorOfVString& inValue)
		{
			addMetaIPTC();
			SetArrayString( fBagWriteIPTC, ImageMetaIPTC::SupplementalCategory, inValue);
		}

		void SetIPTCFixtureIdentifier( const VString& inValue)
		{
			addMetaIPTC();
			SetString( fBagWriteIPTC, ImageMetaIPTC::FixtureIdentifier, inValue);
		}

		void SetIPTCKeywords( const VectorOfVString& inValue)
		{
			addMetaIPTC();
			SetArrayString( fBagWriteIPTC, ImageMetaIPTC::Keywords, inValue);
		}

		void SetIPTCContentLocationCode( const VectorOfVString& inValue)
		{
			addMetaIPTC();
			SetArrayString( fBagWriteIPTC, ImageMetaIPTC::ContentLocationCode, inValue);
		}

		void SetIPTCContentLocationName( const VectorOfVString& inValue)
		{
			addMetaIPTC();
			SetArrayString( fBagWriteIPTC, ImageMetaIPTC::ContentLocationName, inValue);
		}

		void SetIPTCReleaseDateTime( const VTime& inValue)
		{
			addMetaIPTC();
			SetDateTime( fBagWriteIPTC, ImageMetaIPTC::ReleaseDateTime, inValue);
		}

		void SetIPTCExpirationDateTime( const VTime& inValue)
		{
			addMetaIPTC();
			SetDateTime( fBagWriteIPTC, ImageMetaIPTC::ExpirationDateTime, inValue);
		}

		void SetIPTCSpecialInstructions( const VString& inValue)
		{
			addMetaIPTC();
			SetString( fBagWriteIPTC, ImageMetaIPTC::SpecialInstructions, inValue);
		}

		bool SetIPTCActionAdvised( sLONG inValue)
		{
			bool ok = inValue >= (sLONG)ImageMetaIPTC::kActionAdvised_Min && inValue <= (sLONG)ImageMetaIPTC::kActionAdvised_Max;
			if (ok)
				SetIPTCActionAdvised( (ImageMetaIPTC::eActionAdvised)inValue);
			return ok;
		}
		void SetIPTCActionAdvised( const ImageMetaIPTC::eActionAdvised inValue = ImageMetaIPTC::kActionAdvised_Append);

		void SetIPTCReferenceService( const VectorOfVString& inValue)
		{
			addMetaIPTC();
			SetArrayString( fBagWriteIPTC, ImageMetaIPTC::ReferenceService, inValue);
		}

		void SetIPTCReferenceDateTime( const std::vector<VTime>& inValue)
		{
			addMetaIPTC();
			SetArrayDateTime( fBagWriteIPTC, ImageMetaIPTC::ReferenceDateTime, inValue);
		}

		void SetIPTCReferenceNumber( const std::vector<uLONG>& inValue)
		{
			addMetaIPTC();
			SetArrayLong( fBagWriteIPTC, ImageMetaIPTC::ReferenceNumber, inValue);
		}

		void SetIPTCDateTimeCreated( const VTime& inValue)
		{
			addMetaIPTC();
			SetDateTime( fBagWriteIPTC, ImageMetaIPTC::DateTimeCreated, inValue);
		}

		void SetIPTCDigitalCreationDateTime( const VTime& inValue)
		{
			addMetaIPTC();
			SetDateTime( fBagWriteIPTC, ImageMetaIPTC::DigitalCreationDateTime, inValue);
		}

		void SetIPTCOriginatingProgram( const VString& inValue)
		{
			addMetaIPTC();
			SetString( fBagWriteIPTC, ImageMetaIPTC::OriginatingProgram, inValue);
		}

		void SetIPTCProgramVersion( const VString& inValue)
		{
			addMetaIPTC();
			SetString( fBagWriteIPTC, ImageMetaIPTC::ProgramVersion, inValue);
		}

		bool SetIPTCObjectCycle( const VString& inValue)
		{
			addMetaIPTC();
			bool ok = sMapIPTCObjectCycleFromString.AttributeExists( inValue);
			if (ok)
				SetString( fBagWriteIPTC, ImageMetaIPTC::ObjectCycle, inValue);
			return ok;
		}
		void SetIPTCObjectCycle( const ImageMetaIPTC::eObjectCycle inValue = ImageMetaIPTC::kObjectCycle_Both)
		{
			addMetaIPTC();
			VString objectCycle = sMapIPTCObjectCycleFromLong[ inValue];
			SetString( fBagWriteIPTC, ImageMetaIPTC::ObjectCycle, objectCycle);
		}

		void SetIPTCByline( const VectorOfVString& inValue)
		{
			addMetaIPTC();
			SetArrayString( fBagWriteIPTC, ImageMetaIPTC::Byline, inValue);
		}
		void SetIPTCBylineTitle( const VectorOfVString& inValue)
		{
			addMetaIPTC();
			SetArrayString( fBagWriteIPTC, ImageMetaIPTC::BylineTitle, inValue);
		}

		void SetIPTCCity( const VString& inValue)
		{
			addMetaIPTC();
			SetString( fBagWriteIPTC, ImageMetaIPTC::City, inValue);
		}

		void SetIPTCSubLocation( const VString& inValue)
		{
			addMetaIPTC();
			SetString( fBagWriteIPTC, ImageMetaIPTC::SubLocation, inValue);
		}

		void SetIPTCProvinceState( const VString& inValue)
		{
			addMetaIPTC();
			SetString( fBagWriteIPTC, ImageMetaIPTC::ProvinceState, inValue);
		}

		void SetIPTCCountryPrimaryLocationCode( const VString& inValue)
		{
			addMetaIPTC();
			SetString( fBagWriteIPTC, ImageMetaIPTC::CountryPrimaryLocationCode, inValue);
		}

		void SetIPTCCountryPrimaryLocationName( const VString& inValue)
		{
			addMetaIPTC();
			SetString( fBagWriteIPTC, ImageMetaIPTC::CountryPrimaryLocationName, inValue);
		}

		void SetIPTCOriginalTransmissionReference( const VString& inValue)
		{
			addMetaIPTC();
			SetString( fBagWriteIPTC, ImageMetaIPTC::OriginalTransmissionReference, inValue);
		}

		bool SetIPTCScene( const VectorOfVString& inValue);
		bool SetIPTCScene( const std::vector<sLONG>& inValue);
		void SetIPTCScene( const std::vector<ImageMetaIPTC::eScene>& inValue);

		void SetIPTCHeadline( const VString& inValue)
		{
			addMetaIPTC();
			SetString( fBagWriteIPTC, ImageMetaIPTC::Headline, inValue);
		}

		void SetIPTCCredit( const VString& inValue)
		{
			addMetaIPTC();
			SetString( fBagWriteIPTC, ImageMetaIPTC::Credit, inValue);
		}

		void SetIPTCSource( const VString& inValue)
		{
			addMetaIPTC();
			SetString( fBagWriteIPTC, ImageMetaIPTC::Source, inValue);
		}

		void SetIPTCCopyrightNotice( const VString& inValue)
		{
			addMetaIPTC();
			SetString( fBagWriteIPTC, ImageMetaIPTC::CopyrightNotice, inValue);
		}

		void SetIPTCContact( const VectorOfVString& inValue)
		{
			addMetaIPTC();
			SetArrayString( fBagWriteIPTC, ImageMetaIPTC::Contact, inValue);
		}

		void SetIPTCCaptionAbstract( const VString& inValue)
		{
			addMetaIPTC();
			SetString( fBagWriteIPTC, ImageMetaIPTC::CaptionAbstract, inValue);
		}

		void SetIPTCWriterEditor( const VectorOfVString& inValue)
		{
			addMetaIPTC();
			SetArrayString( fBagWriteIPTC, ImageMetaIPTC::WriterEditor, inValue);
		}

		bool SetIPTCImageType( const VString& inValue)
		{
			if (inValue.GetLength() == 2)
			{
				VString imageType;
				imageType.AppendUniChar( inValue.GetUniChar(2));
				if (isdigit((int)inValue.GetUniChar(1)) && sMapIPTCImageTypeFromString.AttributeExists(  imageType))
				{
					addMetaIPTC();
					SetString( fBagWriteIPTC, ImageMetaIPTC::ImageType, inValue);
					return true;
				}
				else
					return false;
			}
			else 
				return false;
		}

		void SetIPTCImageType( const VSize inNumComponent = 4, const ImageMetaIPTC::eImageType inImageTypeColourComposition = ImageMetaIPTC::kImageType_FullColourCompositePixelSequential)
		{
			addMetaIPTC();

			VString value;
			value.FromLong( (sLONG)inNumComponent);
			value.Truncate(1);
			value.AppendString( sMapIPTCImageTypeFromLong[ inImageTypeColourComposition]);

			SetString( fBagWriteIPTC, ImageMetaIPTC::ImageType, value);
		}

		bool SetIPTCImageOrientation( const VString& inValue)
		{
			bool ok  = sMapIPTCImageOrientationFromString.AttributeExists( inValue);
			if (ok)
			{
				addMetaIPTC();
				SetString( fBagWriteIPTC, ImageMetaIPTC::ImageOrientation, inValue);
			}
			return ok;
		}
		void SetIPTCImageOrientation( const ImageMetaIPTC::eImageOrientation inValue = ImageMetaIPTC::kImageOrientation_Landscape)
		{
			addMetaIPTC();
			SetString( fBagWriteIPTC, ImageMetaIPTC::ImageOrientation, sMapIPTCImageOrientationFromLong[ inValue]);
		}

		void SetIPTCLanguageIdentifier( const VString& inValue)
		{
			addMetaIPTC();
			SetString( fBagWriteIPTC, ImageMetaIPTC::LanguageIdentifier, inValue);
		}

		void SetIPTCStarRating( const uLONG inValue = 3)
		{
			addMetaIPTC();
			SetLong( fBagWriteIPTC, ImageMetaIPTC::StarRating, inValue, 0, 5, 0);
		}


		//TIFF writer methods
	protected:
		void addMetaTIFF() 
		{
			//ensure TIFF element exists

			if (fBagWriteTIFF != NULL)
				return;
			fBagWriteTIFF = fBagWrite->GetUniqueElement( ImageMetaTIFF::TIFF);
			if (fBagWriteTIFF != NULL)
				return;
			VValueBag *bagTIFF = new VValueBag();
			fBagWrite->AddElement( ImageMetaTIFF::TIFF, bagTIFF);
			fBagWriteTIFF = bagTIFF;
			bagTIFF->Release();
		}
	public:
		void SetTIFFCompression( const ImageMetaTIFF::eCompression inValue) 
		{
			addMetaTIFF();
			SetLong(	fBagWriteTIFF, ImageMetaTIFF::Compression, inValue, 
						ImageMetaTIFF::kCompression_Min, ImageMetaTIFF::kCompression_Max,
						ImageMetaTIFF::kCompression_Min);
		}

		void SetTIFFPhotometricInterpretation( const ImageMetaTIFF::ePhotometricInterpretation inValue = ImageMetaTIFF::kPhotometricInterpretation_RGB) 
		{
			addMetaTIFF();
			SetLong(	fBagWriteTIFF, ImageMetaTIFF::PhotometricInterpretation, inValue, 
						ImageMetaTIFF::kPhotometricInterpretation_Min, ImageMetaTIFF::kPhotometricInterpretation_Max,
						ImageMetaTIFF::kPhotometricInterpretation_RGB);
		}

		void SetTIFFDocumentName( const VString& inValue) 
		{
			addMetaTIFF();
			SetString( fBagWriteTIFF, ImageMetaTIFF::DocumentName, inValue);
		}

		void SetTIFFImageDescription( const VString& inValue) 
		{
			addMetaTIFF();
			SetString( fBagWriteTIFF, ImageMetaTIFF::ImageDescription, inValue);
		}

		void SetTIFFMake( const VString& inValue) 
		{
			addMetaTIFF();
			SetString( fBagWriteTIFF, ImageMetaTIFF::Make, inValue);
		}

		void SetTIFFModel( const VString& inValue) 
		{
			addMetaTIFF();
			SetString( fBagWriteTIFF, ImageMetaTIFF::Model, inValue);
		}

		bool SetTIFFOrientation( const sLONG inValue) 
		{
			bool ok = inValue >= ImageMetaTIFF::kOrientation_Min && inValue <= ImageMetaTIFF::kOrientation_Max;
			if (ok)
			{
				addMetaTIFF();
				SetLong( fBagWriteTIFF, ImageMetaTIFF::Orientation, inValue);
			}
			return ok;
		}
		void SetTIFFOrientation( const ImageMetaTIFF::eOrientation inValue = ImageMetaTIFF::kOrientation_Horizontal) 
		{
			addMetaTIFF();
			SetLong( fBagWriteTIFF, ImageMetaTIFF::Orientation, inValue, 
					 ImageMetaTIFF::kOrientation_Min, ImageMetaTIFF::kOrientation_Max,
					 ImageMetaTIFF::kOrientation_Horizontal);
		}

		void SetTIFFXResolution( Real inValue = 72.0) 
		{
			addMetaTIFF();
			SetReal( fBagWriteTIFF, ImageMetaTIFF::XResolution, inValue);
		}

		void SetTIFFYResolution( Real inValue = 72.0) 
		{
			addMetaTIFF();
			SetReal( fBagWriteTIFF, ImageMetaTIFF::YResolution, inValue);
		}

		bool SetTIFFResolutionUnit( const sLONG inValue) 
		{
			bool ok = inValue >= ImageMetaTIFF::kResolutionUnit_Min && inValue <= ImageMetaTIFF::kResolutionUnit_Max;
			if (ok)
			{
				addMetaTIFF();
				SetLong( fBagWriteTIFF, ImageMetaTIFF::ResolutionUnit, inValue);
			}
			return ok;
		}
		void SetTIFFResolutionUnit( const ImageMetaTIFF::eResolutionUnit inValue = ImageMetaTIFF::kResolutionUnit_Inches) 
		{
			addMetaTIFF();
			SetLong( fBagWriteTIFF, ImageMetaTIFF::ResolutionUnit, inValue, 
					 ImageMetaTIFF::kResolutionUnit_Min, ImageMetaTIFF::kResolutionUnit_Max,
					 ImageMetaTIFF::kResolutionUnit_Inches);
		}

		void SetTIFFSoftware( const VString& inValue) 
		{
			addMetaTIFF();
			SetString( fBagWriteTIFF, ImageMetaTIFF::Software, inValue);
		}

		bool SetTIFFTransferFunction( const VectorOfVString& inValue) 
		{
			if (inValue.size() != 3*256)
				return false;
			addMetaTIFF();
			SetArrayString( fBagWriteTIFF, ImageMetaTIFF::TransferFunction, inValue);
			return true;
		}
		bool SetTIFFTransferFunction( const std::vector<uLONG>& inValue) 
		{
			if (inValue.size() != 3*256)
				return false;
			addMetaTIFF();
			SetArrayLong( fBagWriteTIFF, ImageMetaTIFF::TransferFunction, inValue);
			return true;
		}

		void SetTIFFDateTime( const VTime& inValue) 
		{
			addMetaTIFF();
			SetDateTime( fBagWriteTIFF, ImageMetaTIFF::DateTime, inValue);
		}

		void SetTIFFArtist( const VString& inValue) 
		{
			addMetaTIFF();
			SetString( fBagWriteTIFF, ImageMetaTIFF::Artist, inValue);
		}

		void SetTIFFHostComputer( const VString& inValue) 
		{
			addMetaTIFF();
			SetString( fBagWriteTIFF, ImageMetaTIFF::HostComputer, inValue);
		}

		void SetTIFFCopyright( const VString& inValue) 
		{
			addMetaTIFF();
			SetString( fBagWriteTIFF, ImageMetaTIFF::Copyright, inValue);
		}

		bool SetTIFFWhitePoint( const VectorOfVString& inValue) 
		{
			if (inValue.size() != 2)
				return false;
			addMetaTIFF();
			SetArrayString( fBagWriteTIFF, ImageMetaTIFF::WhitePoint, inValue);
			return true;
		}

		void SetTIFFWhitePoint( const VPoint& inWhitePointChromaticity = VPoint((GReal) 0.3127,(GReal) 0.3290)) 
		{
			addMetaTIFF();
			std::vector<Real> values;
			values.push_back(inWhitePointChromaticity.x);
			values.push_back(inWhitePointChromaticity.y);
			SetArrayReal( fBagWriteTIFF, ImageMetaTIFF::WhitePoint, values);
		}

		bool SetTIFFPrimaryChromaticities( const VectorOfVString& inValue) 
		{
			if (inValue.size() != 6)
				return false;
			addMetaTIFF();
			SetArrayString( fBagWriteTIFF, ImageMetaTIFF::PrimaryChromaticities, inValue);
			return true;
		}
		void SetTIFFPrimaryChromaticities( const VPoint& inRedChromaticity		= VPoint((GReal) 0.68, (GReal) 0.32),
										   const VPoint& inGreenChromaticity	= VPoint((GReal) 0.28, (GReal) 0.60),
										   const VPoint& inBlueChromaticity		= VPoint((GReal) 0.15, (GReal) 0.07)) 
		{
			addMetaTIFF();
			std::vector<Real> values;
			values.push_back(inRedChromaticity.x);
			values.push_back(inRedChromaticity.y);
			values.push_back(inGreenChromaticity.x);
			values.push_back(inGreenChromaticity.y);
			values.push_back(inBlueChromaticity.x);
			values.push_back(inBlueChromaticity.y);
			SetArrayReal( fBagWriteTIFF, ImageMetaTIFF::PrimaryChromaticities, values);
		}


		//EXIF writer methods
	protected:
		void addMetaEXIF() 
		{
			//ensure EXIF element exists

			if (fBagWriteEXIF != NULL)
				return;
			fBagWriteEXIF = fBagWrite->GetUniqueElement( ImageMetaEXIF::EXIF);
			if (fBagWriteEXIF != NULL)
				return;
			VValueBag *bagEXIF = new VValueBag();
			fBagWrite->AddElement( ImageMetaEXIF::EXIF, bagEXIF);
			fBagWriteEXIF = bagEXIF;
			bagEXIF->Release();
		}
	public:
		void SetEXIFExposureTime( Real inValue) 
		{
			addMetaEXIF();
			SetReal( fBagWriteEXIF, ImageMetaEXIF::ExposureTime, inValue);
		}

		void SetEXIFFNumber( Real inValue) 
		{
			addMetaEXIF();
			SetReal( fBagWriteEXIF, ImageMetaEXIF::FNumber, inValue);
		}

		bool SetEXIFExposureProgram( const sLONG inValue) 
		{
			bool ok = inValue >= ImageMetaEXIF::kExposureProgram_Min && inValue <= ImageMetaEXIF::kExposureProgram_Max;
			if (ok)
			{
				addMetaEXIF();
				SetLong(	fBagWriteEXIF, ImageMetaEXIF::ExposureProgram, inValue);
			}
			return ok;
		}
		void SetEXIFExposureProgram( const ImageMetaEXIF::eExposureProgram inValue = ImageMetaEXIF::kExposureProgram_Landscape) 
		{
			addMetaEXIF();
			SetLong(		fBagWriteEXIF, 
							ImageMetaEXIF::ExposureProgram, inValue, 
							ImageMetaEXIF::kExposureProgram_Min, 
							ImageMetaEXIF::kExposureProgram_Max,
							ImageMetaEXIF::kExposureProgram_Portrait);
		}

		void SetEXIFSpectralSensitivity( const VString& inValue) 
		{
			addMetaEXIF();
			SetString( fBagWriteEXIF, ImageMetaEXIF::SpectralSensitivity, inValue);
		}

		void SetEXIFISOSpeedRatings( const VectorOfVString& inValue) 
		{
			addMetaEXIF();
			SetArrayString( fBagWriteEXIF, ImageMetaEXIF::ISOSpeedRatings, inValue);
		}
		void SetEXIFISOSpeedRatings( const std::vector<uLONG>& inValue) 
		{
			addMetaEXIF();
			SetArrayLong( fBagWriteEXIF, ImageMetaEXIF::ISOSpeedRatings, inValue);
		}

		void SetEXIFOECF( const VBlob& inValue) 
		{
			addMetaEXIF();
			SetBlob( fBagWriteEXIF, ImageMetaEXIF::OECF, inValue);
		}

		bool SetEXIFVersion( const VString& inValue) 
		{
			if (inValue.GetLength() != 4)
				return false;
			if (!isdigit((int)inValue.GetUniChar(1)))
				return false;
			if (!isdigit((int)inValue.GetUniChar(2)))
				return false;
			if (!isdigit((int)inValue.GetUniChar(3)))
				return false;
			if (!isdigit((int)inValue.GetUniChar(4)))
				return false;
			
			addMetaEXIF();
			SetString( fBagWriteEXIF, ImageMetaEXIF::ExifVersion, inValue);
			return true;
		}
		void SetEXIFVersion( const uCHAR inNumber1 = 0,
							 const uCHAR inNumber2 = 2,
							 const uCHAR inNumber3 = 2,
							 const uCHAR inNumber4 = 0) 
		{
			addMetaEXIF();
			std::vector<uCHAR> values;
			values.push_back( inNumber1);
			values.push_back( inNumber2);
			values.push_back( inNumber3);
			values.push_back( inNumber4);
			SetExifVersion( fBagWriteEXIF, ImageMetaEXIF::ExifVersion, values);
		}

		void SetEXIFDateTimeOriginal( const VTime& inValue) 
		{
			addMetaEXIF();
			SetDateTime( fBagWriteEXIF, ImageMetaEXIF::DateTimeOriginal, inValue);
		}

		void SetEXIFDateTimeDigitized( const VTime& inValue) 
		{
			addMetaEXIF();
			SetDateTime( fBagWriteEXIF, ImageMetaEXIF::DateTimeDigitized, inValue);
		}

		bool SetEXIFComponentsConfiguration( const VectorOfVString& inValue) 
		{
			if (inValue.size() != 4)
				return false;
			for (int i = 0; i < 4; i++)
			{
				sLONG componentType = inValue[i].GetLong();
				if (componentType < ImageMetaEXIF::kComponentType_Min 
					|| 
					componentType > ImageMetaEXIF::kComponentType_Max)
					return false;
			}
			addMetaEXIF();
			SetArrayString( fBagWriteEXIF, ImageMetaEXIF::ComponentsConfiguration, inValue);
			return true;
		}
		void SetEXIFComponentsConfiguration( const ImageMetaEXIF::eComponentType inChannel1 = ImageMetaEXIF::kComponentType_R,
											 const ImageMetaEXIF::eComponentType inChannel2 = ImageMetaEXIF::kComponentType_G,
											 const ImageMetaEXIF::eComponentType inChannel3 = ImageMetaEXIF::kComponentType_B,
											 const ImageMetaEXIF::eComponentType inChannel4 = ImageMetaEXIF::kComponentType_Unused) 
		{
			addMetaEXIF();
			std::vector<ImageMetaEXIF::eComponentType> values;
			values.push_back( inChannel1);
			values.push_back( inChannel2);
			values.push_back( inChannel3);
			values.push_back( inChannel4);
			SetArrayLong( fBagWriteEXIF, ImageMetaEXIF::ComponentsConfiguration, values);
		}

		void SetEXIFCompressedBitsPerPixel( Real inValue) 
		{
			addMetaEXIF();
			SetReal( fBagWriteEXIF, ImageMetaEXIF::CompressedBitsPerPixel, inValue);
		}

		void SetEXIFShutterSpeedValue( Real inValue) 
		{
			addMetaEXIF();
			SetReal( fBagWriteEXIF, ImageMetaEXIF::ShutterSpeedValue, inValue);
		}

		void SetEXIFApertureValue( Real inValue) 
		{
			addMetaEXIF();
			SetReal( fBagWriteEXIF, ImageMetaEXIF::ApertureValue, inValue);
		}

		void SetEXIFBrightnessValue( Real inValue) 
		{
			addMetaEXIF();
			SetReal( fBagWriteEXIF, ImageMetaEXIF::BrightnessValue, inValue);
		}

		void SetEXIFExposureBiasValue( Real inValue) 
		{
			addMetaEXIF();
			SetReal( fBagWriteEXIF, ImageMetaEXIF::ExposureBiasValue, inValue);
		}

		void SetEXIFMaxApertureValue( Real inValue) 
		{
			addMetaEXIF();
			SetReal( fBagWriteEXIF, ImageMetaEXIF::MaxApertureValue, inValue);
		}

		void SetEXIFSubjectDistance( Real inValue) 
		{
			addMetaEXIF();
			SetReal( fBagWriteEXIF, ImageMetaEXIF::SubjectDistance, inValue);
		}

		void SetEXIFMeteringMode( const ImageMetaEXIF::eMeteringMode inValue = ImageMetaEXIF::kMeteringMode_Average) 
		{
			addMetaEXIF();
			SetLong(fBagWriteEXIF, 
					ImageMetaEXIF::MeteringMode, 
					inValue, 
					ImageMetaEXIF::kMeteringMode_Min, ImageMetaEXIF::kMeteringMode_Max,
					ImageMetaEXIF::kMeteringMode_Other
					);
		}

		void SetEXIFLightSource( const ImageMetaEXIF::eLightSource inValue = ImageMetaEXIF::kLightSource_Daylight) 
		{
			addMetaEXIF();
			SetLong(fBagWriteEXIF, 
					ImageMetaEXIF::LightSource, 
					inValue, 
					ImageMetaEXIF::kLightSource_Min, ImageMetaEXIF::kLightSource_Max,
					ImageMetaEXIF::kLightSource_Other
					);
		}

		/** set EXIF flash status
		@param inFired
				true if flash is fired
		@param inReturn 
				flash return mode
		@param inMode 
				flash mode
		@param inFunctionPresent 
				true if presence of a flash function
		@param inRedEyeReduction 
				true if red-eye reduction is supported
		*/
		void SetEXIFFlash( const bool inFired = false, 
						   const ImageMetaEXIF::eFlashReturn inReturn = ImageMetaEXIF::kFlashReturn_Detected,  
						   const ImageMetaEXIF::eFlashMode inMode = ImageMetaEXIF::kFlashMode_AutoMode,
						   const bool inFunctionPresent = true,
						   bool  inRedEyeReduction = false);


		void SetEXIFFocalLength( Real inValue) 
		{
			addMetaEXIF();
			SetReal( fBagWriteEXIF, ImageMetaEXIF::FocalLength, inValue);
		}

		bool SetEXIFSubjectArea( const VectorOfVString& inValue)
		{
			if (inValue.size() < 2 || inValue.size() > 4)
				return false;
			addMetaEXIF();
			SetArrayString( fBagWriteEXIF, ImageMetaEXIF::SubjectArea, inValue);
			return true;
		}

		void SetEXIFSubjectAreaCenter(		const uLONG inCenterX, const uLONG inCenterY);
		void SetEXIFSubjectAreaCircle(		const uLONG inCenterX, const uLONG inCenterY, 
											const uLONG inDiameter);
		void SetEXIFSubjectAreaRectangle(	const uLONG inCenterX, const uLONG inCenterY, 
											const uLONG inWidth, const uLONG inHeight);

		void SetEXIFMakerNote( const VString& inValue) 
		{
			addMetaEXIF();
			SetString( fBagWriteEXIF, ImageMetaEXIF::MakerNote, inValue);
		}

		void SetEXIFUserComment( const VString& inValue) 
		{
			addMetaEXIF();
			SetString( fBagWriteEXIF, ImageMetaEXIF::UserComment, inValue);
		}

		bool SetEXIFFlashPixVersion( const VString& inValue) 
		{
			if (inValue.GetLength() != 4)
				return false;
			if (!isdigit((int)inValue.GetUniChar(1)))
				return false;
			if (!isdigit((int)inValue.GetUniChar(2)))
				return false;
			if (!isdigit((int)inValue.GetUniChar(3)))
				return false;
			if (!isdigit((int)inValue.GetUniChar(4)))
				return false;
			
			addMetaEXIF();
			SetString( fBagWriteEXIF, ImageMetaEXIF::FlashPixVersion, inValue);
			return true;
		}
		void SetEXIFFlashPixVersion( const uCHAR inNumber1 = 0,
									 const uCHAR inNumber2 = 2,
									 const uCHAR inNumber3 = 2,
									 const uCHAR inNumber4 = 0) 
		{
			addMetaEXIF();
			std::vector<uCHAR> values;
			values.push_back( inNumber1);
			values.push_back( inNumber2);
			values.push_back( inNumber3);
			values.push_back( inNumber4);
			SetExifVersion( fBagWriteEXIF, ImageMetaEXIF::FlashPixVersion, values);
		}

		void SetEXIFColorSpace( const ImageMetaEXIF::eColorSpace inValue = ImageMetaEXIF::kColorSpace_sRGB) 
		{
			addMetaEXIF();
			SetLong(fBagWriteEXIF, 
					ImageMetaEXIF::ColorSpace, inValue, 
					ImageMetaEXIF::kColorSpace_Min, ImageMetaEXIF::kColorSpace_Max,
					ImageMetaEXIF::kColorSpace_Unknown
					);
		}

		void SetEXIFPixelXDimension( const uLONG inValue) 
		{
			addMetaEXIF();
			SetLong( fBagWriteEXIF, ImageMetaEXIF::PixelXDimension, inValue);
		}

		void SetEXIFPixelYDimension( const uLONG inValue) 
		{
			addMetaEXIF();
			SetLong( fBagWriteEXIF, ImageMetaEXIF::PixelYDimension, inValue);
		}

		void SetEXIFRelatedSoundFile( const VString& inValue) 
		{
			addMetaEXIF();
			SetString( fBagWriteEXIF, ImageMetaEXIF::RelatedSoundFile, inValue);
		}

		void SetEXIFFlashEnergy( Real inValue) 
		{
			addMetaEXIF();
			SetReal( fBagWriteEXIF, ImageMetaEXIF::FlashEnergy, inValue);
		}

		void SetEXIFSpatialFrequencyResponse( const VBlob& inValue) 
		{
			addMetaEXIF();
			SetBlob( fBagWriteEXIF, ImageMetaEXIF::SpatialFrequencyResponse, inValue);
		}

		void SetEXIFFocalPlaneXResolution( Real inValue) 
		{
			addMetaEXIF();
			SetReal( fBagWriteEXIF, ImageMetaEXIF::FocalPlaneXResolution, inValue);
		}

		void SetEXIFFocalPlaneYResolution( Real inValue) 
		{
			addMetaEXIF();
			SetReal( fBagWriteEXIF, ImageMetaEXIF::FocalPlaneYResolution, inValue);
		}

		bool SetEXIFFocalPlaneResolutionUnit( const sLONG inValue) 
		{
			bool ok = inValue >= ImageMetaTIFF::kResolutionUnit_Min && inValue <= ImageMetaTIFF::kResolutionUnit_Max;
			if (ok)
			{
				addMetaEXIF();
				SetLong( fBagWriteEXIF, ImageMetaEXIF::FocalPlaneResolutionUnit, inValue);
			}
			return ok;
		}
		void SetEXIFFocalPlaneResolutionUnit( const ImageMetaTIFF::eResolutionUnit inValue = ImageMetaTIFF::kResolutionUnit_Inches) 
		{
			addMetaEXIF();
			SetLong(fBagWriteEXIF, 
					ImageMetaEXIF::FocalPlaneResolutionUnit, inValue, 
					ImageMetaTIFF::kResolutionUnit_Min, ImageMetaTIFF::kResolutionUnit_Max,
					ImageMetaTIFF::kResolutionUnit_Inches);
		}

		void SetEXIFSubjectLocation( const uLONG inCenterX, const uLONG inCenterY);

		void SetEXIFExposureIndex( Real inValue) 
		{
			addMetaEXIF();
			SetReal( fBagWriteEXIF, ImageMetaEXIF::ExposureIndex, inValue);
		}

		void SetEXIFSensingMethod( const ImageMetaEXIF::eSensingMethod inValue = ImageMetaEXIF::kSensingMethod_ColorSequentialLinear) 
		{
			addMetaEXIF();
			SetLong(fBagWriteEXIF, 
					ImageMetaEXIF::SensingMethod, inValue, 
					ImageMetaEXIF::kSensingMethod_Min, ImageMetaEXIF::kSensingMethod_Max,
					ImageMetaEXIF::kSensingMethod_NotDefined);
		}

		void SetEXIFFileSource( const ImageMetaEXIF::eFileSource inValue = ImageMetaEXIF::kFileSource_DigitalCamera) 
		{
			addMetaEXIF();
			SetLong(fBagWriteEXIF, 
					ImageMetaEXIF::FileSource, inValue, 
					ImageMetaEXIF::kFileSource_Min, ImageMetaEXIF::kFileSource_Max,
					ImageMetaEXIF::kFileSource_DigitalCamera);
		}

		void SetEXIFSceneType( const bool inDirectlyPhotographed = true) 
		{
			addMetaEXIF();

			sLONG sceneType = inDirectlyPhotographed ? 1 : 0;
			SetLong( fBagWriteEXIF, ImageMetaEXIF::SceneType, sceneType);
		}

		void SetEXIFCFAPattern( const VBlob& inValue) 
		{
			addMetaEXIF();
			SetBlob( fBagWriteEXIF, ImageMetaEXIF::CFAPattern, inValue);
		}

		void SetEXIFCustomRendered( const bool inCustomRendered = true) 
		{
			addMetaEXIF();

			sLONG customRendered = inCustomRendered ? 1 : 0;
			SetLong( fBagWriteEXIF, ImageMetaEXIF::CustomRendered, customRendered);
		}

		bool SetEXIFExposureMode( const sLONG inValue) 
		{
			if (inValue >= ImageMetaEXIF::kExposureMode_Min 
				&&
				inValue <= ImageMetaEXIF::kExposureMode_Max)
			{
				addMetaEXIF();
				SetLong(fBagWriteEXIF, ImageMetaEXIF::ExposureMode, inValue);
				return true;
			}
			else
				return false;
		}

		void SetEXIFExposureMode( const ImageMetaEXIF::eExposureMode inValue = ImageMetaEXIF::kExposureMode_Auto) 
		{
			addMetaEXIF();
			SetLong(fBagWriteEXIF, 
					ImageMetaEXIF::ExposureMode, inValue, 
					ImageMetaEXIF::kExposureMode_Min, ImageMetaEXIF::kExposureMode_Max,
					ImageMetaEXIF::kExposureMode_Auto);
		}

		void SetEXIFWhiteBalance( const bool inIsManual = true) 
		{
			addMetaEXIF();

			sLONG whiteBalance = inIsManual ? 1 : 0;
			SetLong( fBagWriteEXIF, ImageMetaEXIF::WhiteBalance, whiteBalance);
		}

		void SetEXIFDigitalZoomRatio( Real inValue) 
		{
			addMetaEXIF();
			SetReal( fBagWriteEXIF, ImageMetaEXIF::DigitalZoomRatio, inValue);
		}

		void SetEXIFFocalLenIn35mmFilm( const uLONG inValue) 
		{
			addMetaEXIF();
			SetLong( fBagWriteEXIF, ImageMetaEXIF::FocalLenIn35mmFilm, inValue);
		}

		bool SetEXIFSceneCaptureType( const sLONG inValue) 
		{
			if (inValue >= ImageMetaEXIF::kSceneCaptureType_Min
				&&
				inValue <= ImageMetaEXIF::kSceneCaptureType_Max)
			{
				addMetaEXIF();
				SetLong(fBagWriteEXIF, ImageMetaEXIF::SceneCaptureType, inValue);
				return true;
			}
			else
				return false;
		}

		void SetEXIFSceneCaptureType( const ImageMetaEXIF::eSceneCaptureType inValue = ImageMetaEXIF::kSceneCaptureType_Standard) 
		{
			addMetaEXIF();
			SetLong(fBagWriteEXIF, 
					ImageMetaEXIF::SceneCaptureType, inValue, 
					ImageMetaEXIF::kSceneCaptureType_Min, ImageMetaEXIF::kSceneCaptureType_Max,
					ImageMetaEXIF::kSceneCaptureType_Standard);
		}

		bool SetEXIFGainControl( const sLONG inValue)
		{
			if (inValue >= ImageMetaEXIF::kGainControl_Min
				&&
				inValue <= ImageMetaEXIF::kGainControl_Max)
			{
				addMetaEXIF();
				SetLong(fBagWriteEXIF, ImageMetaEXIF::GainControl, inValue);
				return true;
			}
			else
				return false;
		}

		void SetEXIFGainControl( const ImageMetaEXIF::eGainControl inValue = ImageMetaEXIF::kGainControl_None) 
		{
			addMetaEXIF();
			SetLong(fBagWriteEXIF, ImageMetaEXIF::GainControl, inValue, 
					ImageMetaEXIF::kGainControl_Min, ImageMetaEXIF::kGainControl_Max,
					ImageMetaEXIF::kGainControl_None);
		}

		bool SetEXIFContrast( const sLONG inValue) 
		{
			if (inValue >= ImageMetaEXIF::kPower_Min 
				&&
				inValue <= ImageMetaEXIF::kPower_Max)
			{
				addMetaEXIF();
				SetLong(fBagWriteEXIF, ImageMetaEXIF::Contrast, inValue);
				return true;
			}
			else
				return false;
		}

		void SetEXIFContrast( const ImageMetaEXIF::ePower inValue = ImageMetaEXIF::kPower_Normal) 
		{
			addMetaEXIF();
			SetLong(fBagWriteEXIF, 
					ImageMetaEXIF::Contrast, inValue, 
					ImageMetaEXIF::kPower_Min, ImageMetaEXIF::kPower_Max,
					ImageMetaEXIF::kPower_Normal);
		}

		bool SetEXIFSaturation( const sLONG inValue) 
		{
			if (inValue >= ImageMetaEXIF::kPower_Min 
				&&
				inValue <= ImageMetaEXIF::kPower_Max)
			{
				addMetaEXIF();
				SetLong(fBagWriteEXIF, ImageMetaEXIF::Saturation, inValue);
				return true;
			}
			else
				return false;
		}

		void SetEXIFSaturation( const ImageMetaEXIF::ePower inValue = ImageMetaEXIF::kPower_Normal) 
		{
			addMetaEXIF();
			SetLong(fBagWriteEXIF, 
					ImageMetaEXIF::Saturation, inValue, 
					ImageMetaEXIF::kPower_Min, ImageMetaEXIF::kPower_Max,
					ImageMetaEXIF::kPower_Normal);
		}

		bool SetEXIFSharpness( const sLONG inValue) 
		{
			if (inValue >= ImageMetaEXIF::kPower_Min 
				&&
				inValue <= ImageMetaEXIF::kPower_Max)
			{
				addMetaEXIF();
				SetLong(fBagWriteEXIF, ImageMetaEXIF::Sharpness, inValue);
				return true;
			}
			else
				return false;
		}

		void SetEXIFSharpness( const ImageMetaEXIF::ePower inValue = ImageMetaEXIF::kPower_Normal) 
		{
			addMetaEXIF();
			SetLong(fBagWriteEXIF, 
					ImageMetaEXIF::Sharpness, inValue, 
					ImageMetaEXIF::kPower_Min, ImageMetaEXIF::kPower_Max,
					ImageMetaEXIF::kPower_Normal);
		}

		void SetEXIFDeviceSettingDescription( const VBlob& inValue) 
		{
			addMetaEXIF();
			SetBlob( fBagWriteEXIF, ImageMetaEXIF::DeviceSettingDescription, inValue);
		}

		bool SetEXIFSubjectDistRange( const sLONG inValue) 
		{
			if (inValue >= ImageMetaEXIF::kSubjectDistRange_Min 
				&&
				inValue <= ImageMetaEXIF::kSubjectDistRange_Max)
			{
				addMetaEXIF();
				SetLong(fBagWriteEXIF, ImageMetaEXIF::SubjectDistRange, inValue);
				return true;
			}
			else
				return false;
		}

		void SetEXIFSubjectDistRange( const ImageMetaEXIF::eSubjectDistRange inValue = ImageMetaEXIF::kSubjectDistRange_Close) 
		{
			addMetaEXIF();
			SetLong(fBagWriteEXIF, 
					ImageMetaEXIF::SubjectDistRange, inValue, 
					ImageMetaEXIF::kSubjectDistRange_Min, ImageMetaEXIF::kSubjectDistRange_Max,
					ImageMetaEXIF::kSubjectDistRange_Unknown);
		}

		void SetEXIFImageUniqueID( const VString& inValue) 
		{
			addMetaEXIF();
			SetString( fBagWriteEXIF, ImageMetaEXIF::ImageUniqueID, inValue);
		}

		void SetEXIFGamma( Real inValue) 
		{
			addMetaEXIF();
			SetReal( fBagWriteEXIF, ImageMetaEXIF::Gamma, inValue);
		}

		//GPS writer methods
	protected:
		void addMetaGPS() 
		{
			//ensure GPS element exists

			if (fBagWriteGPS != NULL)
				return;
			fBagWriteGPS = fBagWrite->GetUniqueElement( ImageMetaGPS::GPS);
			if (fBagWriteGPS != NULL)
				return;
			VValueBag *bagGPS = new VValueBag();
			fBagWrite->AddElement( ImageMetaGPS::GPS, bagGPS);
			fBagWriteGPS = bagGPS;
			bagGPS->Release();
		}
	public:

		bool SetGPSVersionID( const VString& inValue) 
		{
			VectorOfVString values;
			inValue.GetSubStrings((UniChar)'.', values);
			if (values.size() != 4)
				return false;
			SetGPSVersionID((uCHAR)values[0].GetLong(),
							(uCHAR)values[1].GetLong(),
							(uCHAR)values[2].GetLong(),
							(uCHAR)values[3].GetLong());
			return true;
		}
		void SetGPSVersionID( const uCHAR inNumber1 = 1,
							  const uCHAR inNumber2 = 1,
							  const uCHAR inNumber3 = 0,
							  const uCHAR inNumber4 = 0) 
		{
			addMetaGPS();
			std::vector<uCHAR> values;
			values.push_back( inNumber1);
			values.push_back( inNumber2);
			values.push_back( inNumber3);
			values.push_back( inNumber4);
			SetExifVersion( fBagWriteGPS, ImageMetaGPS::VersionID, values);
		}

		bool SetGPSLatitude( const VString& inValue) 
		{
			Real degree, minute, second;
			bool isNorth;
			if (!GetLatitude( inValue, degree, minute, second, isNorth))
				return false;
			SetGPSLatitude( degree, minute, second, isNorth);
			return true;
		}

		void SetGPSLatitude( const Real inDegree, const Real inMinute, const Real inSecond, const bool inIsNorth) 
		{
			addMetaGPS();
			SetLatitude( fBagWriteGPS, ImageMetaGPS::Latitude, inDegree, inMinute, inSecond, inIsNorth);
		}


		bool SetGPSLongitude( const VString& inValue) 
		{
			Real degree, minute, second;
			bool isWest;
			if (!GetLongitude( inValue, degree, minute, second, isWest))
				return false;
			SetGPSLongitude( degree, minute, second, isWest);
			return true;
		}

		void SetGPSLongitude( const Real inDegree, const Real inMinute, const Real inSecond, const bool inIsWest) 
		{
			addMetaGPS();
			SetLongitude( fBagWriteGPS, ImageMetaGPS::Longitude, inDegree, inMinute, inSecond, inIsWest);
		}

		bool SetGPSAltitudeRef( const sLONG inValue) 
		{
			if (inValue >= ImageMetaGPS::kAltitudeRef_Min 
				&&
				inValue <= ImageMetaGPS::kAltitudeRef_Max)
			{
				addMetaGPS();
				SetLong(fBagWriteGPS, ImageMetaGPS::AltitudeRef, inValue);
				return true;
			}
			else
				return false;
		}

		void SetGPSAltitude( Real inAltitude, const ImageMetaGPS::eAltitudeRef inAltitudeRef = ImageMetaGPS::kAltitudeRef_AboveSeaLevel) 
		{
			addMetaGPS();
			SetReal( fBagWriteGPS, ImageMetaGPS::Altitude, inAltitude);
			SetLong( fBagWriteGPS, ImageMetaGPS::AltitudeRef, inAltitudeRef,
					 ImageMetaGPS::kAltitudeRef_Min, ImageMetaGPS::kAltitudeRef_Max,
					 ImageMetaGPS::kAltitudeRef_AboveSeaLevel);
		}	

		void SetGPSSatellites( const VString& inValue) 
		{
			addMetaGPS();
			SetString( fBagWriteGPS, ImageMetaGPS::Satellites, inValue);
		}

		bool SetGPSStatus( const VString& inValue) 
		{
			addMetaGPS();
			if (!sMapGPSStatusFromString.AttributeExists(inValue))
				return false;
			SetString( fBagWriteGPS, ImageMetaGPS::Status, inValue);
			return true;
		}

		void SetGPSStatus( const ImageMetaGPS::eStatus inValue = ImageMetaGPS::kStatus_MeasurementInProgress) 
		{
			addMetaGPS();
			VString value = sMapGPSStatusFromLong[inValue];
			SetString( fBagWriteGPS, ImageMetaGPS::Status, value);
		}

		bool SetGPSMeasureMode( const sLONG inValue) 
		{
			if (inValue >= ImageMetaGPS::kMeasureMode_Min 
				&&
				inValue <= ImageMetaGPS::kMeasureMode_Max)
			{
				addMetaGPS();
				SetLong(fBagWriteGPS, ImageMetaGPS::MeasureMode, inValue);
				return true;
			}
			else
				return false;
		}

		void SetGPSMeasureMode( const ImageMetaGPS::eMeasureMode inValue = ImageMetaGPS::kMeasureMode_2D) 
		{
			addMetaGPS();
			SetLong( fBagWriteGPS, ImageMetaGPS::MeasureMode, inValue,
					 ImageMetaGPS::kMeasureMode_Min, ImageMetaGPS::kMeasureMode_Max,
					 ImageMetaGPS::kMeasureMode_2D);
		}

		void SetGPSDOP( Real inValue) 
		{
			addMetaGPS();
			SetReal( fBagWriteGPS, ImageMetaGPS::DOP, inValue);
		}

		bool SetGPSSpeedRef( const VString& inValue) 
		{
			addMetaGPS();
			if (!sMapGPSDistanceRefFromString.AttributeExists(inValue))
				return false;
			SetString( fBagWriteGPS, ImageMetaGPS::SpeedRef, inValue);
			return true;
		}

		void SetGPSSpeed( Real inSpeed, const ImageMetaGPS::eDistanceRef inSpeedRef = ImageMetaGPS::kDistanceRef_Km) 
		{
			addMetaGPS();
			SetReal( fBagWriteGPS, ImageMetaGPS::Speed, inSpeed);
			VString value = sMapGPSDistanceRefFromLong[inSpeedRef];
			SetString( fBagWriteGPS, ImageMetaGPS::SpeedRef, value);
		}	

		bool SetGPSTrackRef( const VString& inValue) 
		{
			addMetaGPS();
			if (!sMapGPSNorthTypeFromString.AttributeExists(inValue))
				return false;
			SetString( fBagWriteGPS, ImageMetaGPS::TrackRef, inValue);
			return true;
		}

		void SetGPSTrack( Real inTrack, const ImageMetaGPS::eNorthType inTrackRef = ImageMetaGPS::kNorthType_Magnetic) 
		{
			addMetaGPS();
			SetReal( fBagWriteGPS, ImageMetaGPS::Track, inTrack);
			VString value = sMapGPSNorthTypeFromLong[inTrackRef];
			SetString( fBagWriteGPS, ImageMetaGPS::TrackRef, value);
		}	

		bool SetGPSImgDirectionRef( const VString& inValue) 
		{
			addMetaGPS();
			if (!sMapGPSNorthTypeFromString.AttributeExists(inValue))
				return false;
			SetString( fBagWriteGPS, ImageMetaGPS::ImgDirectionRef, inValue);
			return true;
		}

		void SetGPSImgDirection( Real inImgDirection, const ImageMetaGPS::eNorthType inImgDirectionRef = ImageMetaGPS::kNorthType_Magnetic) 
		{
			addMetaGPS();
			SetReal( fBagWriteGPS, ImageMetaGPS::ImgDirection, inImgDirection);
			VString value = sMapGPSNorthTypeFromLong[inImgDirectionRef];
			SetString( fBagWriteGPS, ImageMetaGPS::ImgDirectionRef, value);
		}	

		void SetGPSMapDatum( const VString& inValue) 
		{
			addMetaGPS();
			SetString( fBagWriteGPS, ImageMetaGPS::MapDatum, inValue);
		}

		bool SetGPSDestLatitude( const VString& inValue) 
		{
			Real degree, minute, second;
			bool isNorth;
			if (!GetLatitude( inValue, degree, minute, second, isNorth))
				return false;
			SetGPSDestLatitude( degree, minute, second, isNorth);
			return true;
		}

		void SetGPSDestLatitude( const Real inDegree, const Real inMinute, const Real inSecond, const bool inIsNorth) 
		{
			addMetaGPS();
			SetLatitude( fBagWriteGPS, ImageMetaGPS::DestLatitude, inDegree, inMinute, inSecond, inIsNorth);
		}

		bool SetGPSDestLongitude( const VString& inValue) 
		{
			Real degree, minute, second;
			bool isWest;
			if (!GetLongitude( inValue, degree, minute, second, isWest))
				return false;
			SetGPSDestLongitude( degree, minute, second, isWest);
			return true;
		}

		void SetGPSDestLongitude( const Real inDegree, const Real inMinute, const Real inSecond, const bool inIsWest) 
		{
			addMetaGPS();
			SetLongitude( fBagWriteGPS, ImageMetaGPS::DestLongitude, inDegree, inMinute, inSecond, inIsWest);
		}

		bool SetGPSDestBearingRef( const VString& inValue) 
		{
			addMetaGPS();
			if (!sMapGPSNorthTypeFromString.AttributeExists(inValue))
				return false;
			SetString( fBagWriteGPS, ImageMetaGPS::DestBearingRef, inValue);
			return true;
		}

		void SetGPSDestBearing( Real inDestBearing, const ImageMetaGPS::eNorthType inDestBearingRef = ImageMetaGPS::kNorthType_Magnetic) 
		{
			addMetaGPS();
			SetReal( fBagWriteGPS, ImageMetaGPS::DestBearing, inDestBearing);
			VString value = sMapGPSNorthTypeFromLong[inDestBearingRef];
			SetString( fBagWriteGPS, ImageMetaGPS::DestBearingRef, value);
		}	

		bool SetGPSDestDistanceRef( const VString& inValue) 
		{
			addMetaGPS();
			if (!sMapGPSDistanceRefFromString.AttributeExists(inValue))
				return false;
			SetString( fBagWriteGPS, ImageMetaGPS::DestDistanceRef, inValue);
			return true;
		}

		void SetGPSDestDistance( Real inDestDistance, const ImageMetaGPS::eDistanceRef inDestDistanceRef = ImageMetaGPS::kDistanceRef_Km) 
		{
			addMetaGPS();
			SetReal( fBagWriteGPS, ImageMetaGPS::DestDistance, inDestDistance);
			VString value = sMapGPSDistanceRefFromLong[inDestDistanceRef];
			SetString( fBagWriteGPS, ImageMetaGPS::DestDistanceRef, value);
		}	

		void SetGPSProcessingMethod( const VString& inValue) 
		{
			addMetaGPS();
			SetString( fBagWriteGPS, ImageMetaGPS::ProcessingMethod, inValue);
		}

		void SetGPSAreaInformation( const VString& inValue) 
		{
			addMetaGPS();
			SetString( fBagWriteGPS, ImageMetaGPS::AreaInformation, inValue);
		}

		void SetGPSDifferential( const bool inDifferentialCorrected = true) 
		{
			addMetaGPS();
			sLONG value = inDifferentialCorrected ? 1 : 0;
			SetLong( fBagWriteGPS, ImageMetaGPS::Differential, value);
		}

		void SetGPSDateTime( const VTime& inValue) 
		{
			addMetaGPS();
			SetDateTime( fBagWriteGPS, ImageMetaGPS::DateTime, inValue);
		}

	protected:
		VValueBag *fBagWrite;
		VValueBag *fBagWriteIPTC;
		VValueBag *fBagWriteTIFF;
		VValueBag *fBagWriteEXIF;
		VValueBag *fBagWriteGPS;
	};
}	


END_TOOLBOX_NAMESPACE


//
// This class provides encode/decode for RFC 2045 Base64 as
// defined by RFC 2045, N. Freed and N. Borenstein.
// RFC 2045: Multipurpose Internet Mail Extensions (MIME)
// Part One: Format of Internet Message Bodies. Reference
// 1996 Available at: http://www.ietf.org/rfc/rfc2045.txt
// This class is used by XML Schema binary format validation
//
//
// slightly modified from xerces 2.8.0 by Jacques Quidu
// (in order to avoid to link Graphics dll with xerces)

namespace xerces
{

typedef unsigned char    XMLByte;
typedef unsigned short   XMLCh;

class Base64
{
public :

    enum Conformance
    {
        Conf_RFC2045
      , Conf_Schema
    };


    /**
     * Encodes input blob into Base64 VString
	  
	   return true if blob is successfully encoded
     */
    static bool encode(const class VBlob& inBlob,
					   class VString& outTextBase64);

    /**
     * Decodes input Base64 VString into output blob
     */
    static bool decode(	 const VString& inTextBase64,
						 VBlob& outBlob,	
                         Conformance  inConform = Conf_RFC2045
                       );


private :

    static void init();

    static bool isData(const XMLByte& octet);
    static bool isPad(const XMLByte& octet);

    static XMLByte set1stOctet(const XMLByte&, const XMLByte&);
    static XMLByte set2ndOctet(const XMLByte&, const XMLByte&);
    static XMLByte set3rdOctet(const XMLByte&, const XMLByte&);

    static void split1stOctet(const XMLByte&, XMLByte&, XMLByte&);
    static void split2ndOctet(const XMLByte&, XMLByte&, XMLByte&);
    static void split3rdOctet(const XMLByte&, XMLByte&, XMLByte&);

	static bool isWhiteSpace( const XMLByte octet)
	{
		return octet == 0x20 || octet == 0x09 || octet == 0x0D || octet == 0x0A;
	}

    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    Base64();
    Base64(const Base64&);

    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  base64Alphabet
    //     The Base64 alphabet (see RFC 2045).
    //
    //  base64Padding
    //     Padding character (see RFC 2045).
    //
    //  base64Inverse
    //     Table used in decoding base64.
    //
    //  isInitialized
    //     Set once base64Inverse is initalized.
    //
    //  quadsPerLine
    //     Number of quadruplets per one line. The encoded output
    //     stream must be represented in lines of no more
    //     than 19 quadruplets each.
    //
    // -----------------------------------------------------------------------

    static const XMLByte  base64Alphabet[];
    static const XMLByte  base64Padding;

    static XMLByte  base64Inverse[];
    static bool  isInitialized;

    static const unsigned int  quadsPerLine;
};


// -----------------------------------------------------------------------
//  Helper methods
// -----------------------------------------------------------------------
inline bool Base64::isPad(const XMLByte& octet)
{
    return ( octet == base64Padding );
}

inline XMLByte Base64::set1stOctet(const XMLByte& b1, const XMLByte& b2)
{
    return (( b1 << 2 ) | ( b2 >> 4 ));
}

inline XMLByte Base64::set2ndOctet(const XMLByte& b2, const XMLByte& b3)
{
    return (( b2 << 4 ) | ( b3 >> 2 ));
}

inline XMLByte Base64::set3rdOctet(const XMLByte& b3, const XMLByte& b4)
{
    return (( b3 << 6 ) | b4 );
}

inline void Base64::split1stOctet(const XMLByte& ch, XMLByte& b1, XMLByte& b2) {
    b1 = ch >> 2;
    b2 = ( ch & 0x3 ) << 4;
}

inline void Base64::split2ndOctet(const XMLByte& ch, XMLByte& b2, XMLByte& b3) {
    b2 |= ch >> 4;  // combine with previous value
    b3 = ( ch & 0xf ) << 2;
}

inline void Base64::split3rdOctet(const XMLByte& ch, XMLByte& b3, XMLByte& b4) {
    b3 |= ch >> 6;  // combine with previous value
    b4 = ( ch & 0x3f );
}

}


#endif

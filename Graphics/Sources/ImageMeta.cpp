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
#include "ImageMeta.h"

BEGIN_TOOLBOX_NAMESPACE

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
	CREATE_BAGKEY(Metadatas);	

	VValueBag sMapIPTCObjectCycleFromString;
	std::map<sLONG,VString> sMapIPTCObjectCycleFromLong;

	VValueBag sMapIPTCImageTypeFromString;
	std::map<sLONG,VString> sMapIPTCImageTypeFromLong;

	VValueBag sMapIPTCImageOrientationFromString;
	std::map<sLONG,VString> sMapIPTCImageOrientationFromLong;

	VValueBag sMapGPSStatusFromString;
	std::map<sLONG,VString> sMapGPSStatusFromLong;

	VValueBag sMapGPSDistanceRefFromString;
	std::map<sLONG,VString> sMapGPSDistanceRefFromLong;

	VValueBag sMapGPSNorthTypeFromString;
	std::map<sLONG,VString> sMapGPSNorthTypeFromLong;
}


/** encoding properties
@remarks
	encoding properties bag contains both encoding properties as bag attributes
	and optionally modified metadatas bag as unique element with name ImageMeta::Metadatas

	please use ImageEncoding::stReader and ImageEncoding::stWriter
	to read or write encoding properties or retain/release the optional metadatas bag
*/
namespace ImageEncoding
{
	//image quality (0 : worst quality, 1: best quality = lossless if codec support it)
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(ImageQuality,	XBOX::VReal, Real, 1);	

	//image lossless compression (0 : worst/fastest compression, 1: best/slowest compression)
	//@remarks
	//	used only for TIFF
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(ImageCompression,	XBOX::VReal, Real, 0.8);		

	//metadatas encoding status (1 : encode with metadatas, 0: do not encode with metadatas)
	//@remark
	//	by default, if attribute is not defined, metadatas are encoded
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(EncodeMetadatas, XBOX::VBoolean, Boolean, 1);		
	
	// extension or mime 
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR(CodecIdentifier,XBOX::VString, XBOX::VString, "");
}	



//IPTC metadatas properties (based on IIM v4.1: see http://www.iptc.org/std/IIM/4.1/specification/IIMV4.1.pdf)
namespace ImageMetaIPTC
{
	//bag name
	CREATE_BAGKEY(IPTC);	

	//bag attributes
	CREATE_BAGKEY(ObjectTypeReference);			//VString 
	CREATE_BAGKEY(ObjectAttributeReference);	//VString (alias Iptc4xmpCore:IntellectualGenre)
	CREATE_BAGKEY(ObjectName);					//VString
	CREATE_BAGKEY(EditStatus);					//VString
	CREATE_BAGKEY(EditorialUpdate);				//VString
	CREATE_BAGKEY(Urgency);						//VLong || VString (VLong formatted as VString)
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
	CREATE_BAGKEY(SubjectReference);			//VLong or VString (if more than one, individual subjects codes are separated by ';')
												//(alias Iptc4xmpCore:SubjectCode)
	CREATE_BAGKEY(Category);					//VString  
	CREATE_BAGKEY(SupplementalCategory);		//VString (if more than one, categories are separated by ';')
	CREATE_BAGKEY(FixtureIdentifier);			//VString
	CREATE_BAGKEY(Keywords);					//VString (if more than one, keywords are separated by ';')
	CREATE_BAGKEY(ContentLocationCode);			//VString (if more than one, keywords are separated by ';')
	CREATE_BAGKEY(ContentLocationName);			//VString (if more than one, keywords are separated by ';')
	CREATE_BAGKEY(ReleaseDateTime);				//VTime or VString with XML datetime ("YYYY-MM-DDTHH:MM:SS[+|-]HH:MM")
												//(build from IPTC 'ReleaseDate' and 'ReleaseTime' tags)
	CREATE_BAGKEY(ExpirationDateTime);			//VTime or VString with XML datetime ("YYYY-MM-DDTHH:MM:SS[+|-]HH:MM")
												//(build from IPTC 'ExpirationDate' and 'ExpirationTime' tags)
	CREATE_BAGKEY(SpecialInstructions);			//VString
	CREATE_BAGKEY(ActionAdvised);				//VLong or VString (two numeric characters)
												//	"01" = Object Kill 
												//	"02" = Object Replace 
												//	"03" = Ojbect Append 
												//	"04" = Object Reference

	CREATE_BAGKEY(ReferenceService);			//VString (if more than one, values are separated by ';')
	CREATE_BAGKEY(ReferenceDateTime);			//VTime or VString with XML datetime ("YYYY-MM-DDTHH:MM:SS[+|-]HH:MM")
												//(if more than one, string-list of XML datetimes separated by ';') 
												//(build from IPTC 'ReferenceDate' tag)
	CREATE_BAGKEY(ReferenceNumber);				//VLong or VString (if more than one, VString with values separated by ';')
	CREATE_BAGKEY(DateTimeCreated);				//VString with XML datetime ("YYYY-MM-DDTHH:MM:SS[+|-]HH:MM")
												//(build from IPTC 'DateCreated' and 'TimeCreated' tags)
	CREATE_BAGKEY(DigitalCreationDateTime);		//VTime or VString with XML datetime ("YYYY-MM-DDTHH:MM:SS[+|-]HH:MM")
												//(build from IPTC 'DigitalCreationDate' and 'DigitalCreationTime' tags)
	CREATE_BAGKEY(OriginatingProgram);			//VString
	CREATE_BAGKEY(ProgramVersion);				//VString
	CREATE_BAGKEY(ObjectCycle);					//VString (one character)
												//	'a' = morning
												//	'p' = evening
												//	'b' = both


	CREATE_BAGKEY(Byline);						//VString (if more than one, values are separated by ';')
	CREATE_BAGKEY(BylineTitle);					//VString (if more than one, values are separated by ';')
	CREATE_BAGKEY(City);						//VString
	CREATE_BAGKEY(SubLocation);					//VString
	CREATE_BAGKEY(ProvinceState);				//VString
	CREATE_BAGKEY(CountryPrimaryLocationCode);	//VString
	CREATE_BAGKEY(CountryPrimaryLocationName);	//VString
	CREATE_BAGKEY(OriginalTransmissionReference);	//VString
	CREATE_BAGKEY(Scene);						//VString (IPTC scene code - 6 digits string: see http://www.newscodes.org:
												//         if more than one, scene codes are separated by ';')

	CREATE_BAGKEY(Headline);					//VString
	CREATE_BAGKEY(Credit);						//VString
	CREATE_BAGKEY(Source);						//VString
	CREATE_BAGKEY(CopyrightNotice);				//VString
	CREATE_BAGKEY(Contact);						//VString (if more than one, contacts are separated by ';')
	CREATE_BAGKEY(CaptionAbstract);				//VString
	CREATE_BAGKEY(WriterEditor);				//VString (if more than one, writers/editors are separated by ';')
	CREATE_BAGKEY(ImageType);					//VString : two characters only: first is numeric, second is alphabetic
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

	CREATE_BAGKEY(ImageOrientation);			//VString: one character only
												//	'L' = Landscape 
												//	'P' = Portrait 
												//	'S' = Square

	CREATE_BAGKEY(LanguageIdentifier);			//VString: two or three alphabetic characters 
												//(ISO 639:1988 national language identifier)
	CREATE_BAGKEY(StarRating);					//VLong || VString (VLong formatted as VString)
												//(normal Rating values of 1,2,3,4 and 5 stars)

	//internal usage only: 
	//following tags are used only to read/write metadatas 
	//(bag datetime format is XML datetime format)
	CREATE_BAGKEY(ReleaseDate);	
	CREATE_BAGKEY(ReleaseTime);	
	CREATE_BAGKEY(ExpirationDate);	
	CREATE_BAGKEY(ExpirationTime);	
	CREATE_BAGKEY(ReferenceDate);	
	CREATE_BAGKEY(DateCreated);	
	CREATE_BAGKEY(TimeCreated);	
	CREATE_BAGKEY(DigitalCreationDate);	
	CREATE_BAGKEY(DigitalCreationTime);	
}


//TIFF metadatas properties (based on Exif 2.2 (IFD TIFF tags): see http://exif.org/Exif2-2.PDF)
namespace ImageMetaTIFF
{
	//bag name
	CREATE_BAGKEY(TIFF);	
	
	//bag attributes
	CREATE_BAGKEY(Compression);					//VLong 
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

	CREATE_BAGKEY(PhotometricInterpretation);	//VLong
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

	CREATE_BAGKEY(DocumentName);				//VString
	CREATE_BAGKEY(ImageDescription);			//VString
	CREATE_BAGKEY(Make);						//VString
	CREATE_BAGKEY(Model);						//VString
	CREATE_BAGKEY(Orientation);					//VLong
												//	1 = Horizontal (normal) 
												//	2 = Mirror horizontal 
												//	3 = Rotate 180 
												//	4 = Mirror vertical 
												//	5 = Mirror horizontal and rotate 270 CW 
												//	6 = Rotate 90 CW 
												//	7 = Mirror horizontal and rotate 90 CW 
												//	8 = Rotate 270 CW

	CREATE_BAGKEY(XResolution);					//VReal
	CREATE_BAGKEY(YResolution);					//VReal
	CREATE_BAGKEY(ResolutionUnit);				//VLong
												//	1 = None 
												//	2 = inches 
												//	3 = cm 
												//	4 = mm 
												//	5 = um

	CREATE_BAGKEY(Software);					//VString
	CREATE_BAGKEY(TransferFunction);			//VString (list of string-formatted integer values separated by ';')
	CREATE_BAGKEY(DateTime);					//VTime or VString with XML datetime ("YYYY-MM-DDTHH:MM:SSZ")
	CREATE_BAGKEY(Artist);						//VString 
	CREATE_BAGKEY(HostComputer);				//VString
	CREATE_BAGKEY(Copyright);					//VString
	CREATE_BAGKEY(WhitePoint);					//VString (list of string-formatted real values separated by ';')
	CREATE_BAGKEY(PrimaryChromaticities);		//VString (list of string-formatted real values separated by ';')
}


//EXIF metadatas properties (based on Exif 2.2 (IFD EXIF tags): see http://exif.org/Exif2-2.PDF)
namespace ImageMetaEXIF
{
	//bag name
	CREATE_BAGKEY(EXIF);	
	
	//bag attributes
	CREATE_BAGKEY(ExposureTime);			//VReal
	CREATE_BAGKEY(FNumber);					//VReal
	CREATE_BAGKEY(ExposureProgram);			//VLong
											//	1 = Manual 
											//	2 = Program AE 
											//	3 = Aperture-priority AE 
											//	4 = Shutter speed priority AE 
											//	5 = Creative (Slow speed) 
											//	6 = Action (High speed) 
											//	7 = Portrait 
											//	8 = Landscape

	CREATE_BAGKEY(SpectralSensitivity);		//VString
	CREATE_BAGKEY(ISOSpeedRatings);			//VString (list of string-formatted integer values separated by ';')
	CREATE_BAGKEY(OECF);					//VString 
											// base64-encoded binary blob 
											// or 
											// list of string-formatted uchar values separated by ';'
	CREATE_BAGKEY(ExifVersion);				//VString (4 characters: "0220" for instance)					
	CREATE_BAGKEY(DateTimeOriginal);		//VTime or VString with XML datetime ("YYYY-MM-DDTHH:MM:SSZ")
	CREATE_BAGKEY(DateTimeDigitized);		//VTime or VString with XML datetime ("YYYY-MM-DDTHH:MM:SSZ")
	CREATE_BAGKEY(ComponentsConfiguration);	//VString (list of 4 string-formatted integer values separated by ';')
											//  meaning for single integer value:
											//	0 = does not exist
											//	1 = Y 
											//	2 = Cb 
											//	3 = Cr 
											//	4 = R 
											//	5 = G 
											//	6 = B
											//  for instance "4;5;6;0" is RGB uncompressed

	CREATE_BAGKEY(CompressedBitsPerPixel);	//VReal
	CREATE_BAGKEY(ShutterSpeedValue);		//VReal
	CREATE_BAGKEY(ApertureValue);			//VReal
	CREATE_BAGKEY(BrightnessValue);			//VReal
	CREATE_BAGKEY(ExposureBiasValue);		//VReal
	CREATE_BAGKEY(MaxApertureValue);		//VReal
	CREATE_BAGKEY(SubjectDistance);			//VReal
	CREATE_BAGKEY(MeteringMode);			//VLong
											//	1 	= Average 
											//	2 	= Center-weighted average 
											//	3 	= Spot 
											//	4 	= Multi-spot 
											//	5 	= Multi-segment 
											//	6 	= Partial 
											//	255 = Other


	CREATE_BAGKEY(LightSource);				//VLong
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

	CREATE_BAGKEY(Flash);					//VLong
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

	CREATE_BAGKEY(FocalLength);				//VReal
	CREATE_BAGKEY(SubjectArea);				//VString (list of 2 up to 4 string-formatted integer values separated by ';')
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


	CREATE_BAGKEY(MakerNote);				//VString
	CREATE_BAGKEY(UserComment);				//VString
	CREATE_BAGKEY(SubsecTime);				//VString (subsec digits)
	CREATE_BAGKEY(SubsecTimeOriginal);		//VString (subsec digits)
	CREATE_BAGKEY(SubsecTimeDigitized);		//VString (subsec digits)
	CREATE_BAGKEY(FlashPixVersion);			//VString (4 characters only: for instance "0100")
	CREATE_BAGKEY(ColorSpace);				//VLong
											//	1			= sRGB 
											//	2			= Adobe RGB 
											//	65535		= Uncalibrated 
											//	4294967295	= Uncalibrated

	CREATE_BAGKEY(PixelXDimension);			//VLong
	CREATE_BAGKEY(PixelYDimension);			//VLong
	CREATE_BAGKEY(RelatedSoundFile);		//VString
	CREATE_BAGKEY(FlashEnergy);				//VReal
	CREATE_BAGKEY(SpatialFrequencyResponse);//VString 
											// base64-encoded binary blob 
											// or 
											// list of string-formatted uchar values separated by ';'
	CREATE_BAGKEY(FocalPlaneXResolution);	//VReal
	CREATE_BAGKEY(FocalPlaneYResolution);	//VReal
	CREATE_BAGKEY(FocalPlaneResolutionUnit);//VLong
											//	1 = None 
											//	2 = inches 
											//	3 = cm 
											//	4 = mm 
											//	5 = um

	CREATE_BAGKEY(SubjectLocation);			//VString (2 string-formatted integers separated by ';')
											// xy coordinates of the subject in the image
	CREATE_BAGKEY(ExposureIndex);			//VReal
	CREATE_BAGKEY(SensingMethod);			//VLong
											//	1 = Not defined 
											//	2 = One-chip color area 
											//	3 = Two-chip color area 
											//	4 = Three-chip color area 
											//	5 = Color sequential area 
											//	7 = Trilinear 
											//	8 = Color sequential linear

	CREATE_BAGKEY(FileSource);				//VLong
											//	1 = Film Scanner 
											//	2 = Reflection Print Scanner 
											//	3 = Digital Camera

	CREATE_BAGKEY(SceneType);				//VLong
											//	1 = Directly photographed
	CREATE_BAGKEY(CFAPattern);				//VString 
											// base64-encoded binary blob 
											// or 
											// list of string-formatted uchar values separated by ';'
	CREATE_BAGKEY(CustomRendered);			//VLong
											//	0 = Normal 
											//	1 = Custom
	CREATE_BAGKEY(ExposureMode);			//VLong
											//	0 = Auto 
											//	1 = Manual 
											//	2 = Auto bracket

	CREATE_BAGKEY(WhiteBalance);			//VLong
											//	0 = Auto 
											//	1 = Manual
	CREATE_BAGKEY(DigitalZoomRatio);		//VReal
	CREATE_BAGKEY(FocalLenIn35mmFilm);		//VLong
	CREATE_BAGKEY(SceneCaptureType);		//VLong
											//	0 = Standard 
											//	1 = Landscape 
											//	2 = Portrait 
											//	3 = Night

	CREATE_BAGKEY(GainControl);				//VLong
											//	0 = None 
											//	1 = Low gain up 
											//	2 = High gain up 
											//	3 = Low gain down 
											//	4 = High gain down

	CREATE_BAGKEY(Contrast);				//VLong
											//	0 = Normal 
											//	1 = Low 
											//	2 = High
	CREATE_BAGKEY(Saturation);				//VLong
											//	0 = Normal 
											//	1 = Low 
											//	2 = High
	CREATE_BAGKEY(Sharpness);				//VLong
											//	0 = Normal 
											//	1 = Low 
											//	2 = High


	CREATE_BAGKEY(DeviceSettingDescription);//VString 
											// base64-encoded binary blob 
											// or 
											// list of string-formatted uchar values separated by ';'
	CREATE_BAGKEY(SubjectDistRange);		//VLong
											//	0 = Unknown 
											//	1 = Macro 
											//	2 = Close 
											//	3 = Distant

	CREATE_BAGKEY(ImageUniqueID);			//VString
	CREATE_BAGKEY(Gamma);					//VReal

	//pseudo-tags

	//tags used to read/write exif Flash tag sub-values
	//(for instance 'SET PICTURE META(vPicture;"EXIF/Flash/Fired";true;"EXIF/Flash/Mode";3)'
	//				alias for
	//				'SET PICTURE META(vPicture;"EXIF/Flash";25)'
	//)
	CREATE_BAGKEY(Fired);					//VBoolean : flash fired status
	CREATE_BAGKEY(ReturnLight);				//VLong : flash return light status
	CREATE_BAGKEY(Mode);					//VLong : flash mode
	CREATE_BAGKEY(FunctionPresent);			//VBoolean : flash function status
	CREATE_BAGKEY(RedEyeReduction);			//VBoolean : red-eye reduction status

}

//GPS metadatas properties (based on Exif 2.2 (IFD GPS tags): see http://exif.org/Exif2-2.PDF)
namespace ImageMetaGPS
{
	//bag name
	CREATE_BAGKEY(GPS);	
	
	//bag attributes
	CREATE_BAGKEY(VersionID);				//VString (4 uchar string-formatted separated by '.' (XMP format): '2.2.0.0' for instance)
	CREATE_BAGKEY(Latitude);				//VString with XMP-formatted latitude:
											//	3 string-formatted reals (degrees, minutes and seconds respectively)
											//	separated by ',' (not ';' here because XMP-compliant format)
											//	and followed by 'N' or 'S'
											//  (for instance "10,54,0N")
	CREATE_BAGKEY(Longitude);				//VString with XMP-formatted longitude: 
											//	3 string-formatted reals (degrees, minutes and seconds respectively)
											//	separated by ',' (not ';' here because XMP-compliant format)
											//  and followed by 'W' or 'E'
											//  (for instance "30,20,0E")
	CREATE_BAGKEY(AltitudeRef);				//VLong
											//	0 = Above Sea Level 
											//	1 = Below Sea Level

	CREATE_BAGKEY(Altitude);				//VReal (altitude in meters depending on reference set by AltitudeRef)
	CREATE_BAGKEY(Satellites);				//VString 
	CREATE_BAGKEY(Status);					//VString (one character only)
											//	'A' = Measurement in progress
											//	'V' = Measurement Interoperability

	CREATE_BAGKEY(MeasureMode);				//VLong
											//	2 = 2-Dimensional 
											//	3 = 3-Dimensional

	CREATE_BAGKEY(DOP);						//VReal
	CREATE_BAGKEY(SpeedRef);				//VString (one character only)
											//	'K' = km/h 
											//	'M' = miles/h 
											//	'N' = knots

	CREATE_BAGKEY(Speed);					//VReal
	CREATE_BAGKEY(TrackRef);				//VString (one character only)
											//	'M' = Magnetic North 
											//	'T' = True North


	CREATE_BAGKEY(Track);					//VReal
	CREATE_BAGKEY(ImgDirectionRef);			//VString (one character only)
											//	'M' = Magnetic North 
											//	'T' = True North
	CREATE_BAGKEY(ImgDirection);			//VReal (0.00 to 359.99)
	CREATE_BAGKEY(MapDatum);				//VString
	CREATE_BAGKEY(DestLatitude);			//VString with XMP-formatted latitude:
											//	3 string-formatted reals (degrees, minutes and seconds respectively)
											//	separated by ',' (not ';' here because XMP-compliant)
											//	and followed by 'N' or 'S'
											//  (for instance "10,54,0N")
	CREATE_BAGKEY(DestLongitude);			//VString with XMP-formatted longitude: 
											//	3 string-formatted reals (degrees, minutes and seconds respectively)
											//	separated by ',' (not ';' here because XMP-compliant)
											//  and followed by 'W' or 'E'
											//  (for instance "30,20,0E")
	CREATE_BAGKEY(DestBearingRef);			//VString (one character only)
											//	'M' = Magnetic North 
											//	'T' = True North
	CREATE_BAGKEY(DestBearing);				//VReal
	CREATE_BAGKEY(DestDistanceRef);			//VString (one character only)
											//	'K' = Kilometers 
											//	'M' = Miles 
											//	'N' = Nautical Miles
	CREATE_BAGKEY(DestDistance);			//VReal	
	CREATE_BAGKEY(ProcessingMethod);		//VString 
	CREATE_BAGKEY(AreaInformation);			//VString
	CREATE_BAGKEY(Differential);			//VLong
											//	0 = No Correction 
											//	1 = Differential Corrected
	CREATE_BAGKEY(DateTime);				//VTime or VString with XML datetime ("YYYY-MM-DDTHH:MM:SSZ")

	//internal usage only: 
	//following tags are used only to read/write metadatas 
	//(bag datetime type is XML datetime string or VTime
	// and bag latitude and longitude format is XMP format)
	CREATE_BAGKEY(DateStamp);	
	CREATE_BAGKEY(TimeStamp);	

	CREATE_BAGKEY(LatitudeRef);	
	CREATE_BAGKEY(LongitudeRef);	
	CREATE_BAGKEY(DestLatitudeRef);	
	CREATE_BAGKEY(DestLongitudeRef);	

	//pseudo-tags

	//pseudo-tags used to read/write latitude or longitude type tags
	//(for instance 'SET PICTURE METAS(vPicture;"GPS/Latitude/Deg";100;"GPS/Latitude/Min";54;"GPS/Latitude/Sec";32;"GPS/Latitude/Dir";"N")')
	CREATE_BAGKEY(Deg);		//VReal or VString: normalized degrees (latitude or longitude type tag only)
	CREATE_BAGKEY(Min);		//VReal or VString: normalized minutes (latitude or longitude type tag only)
	CREATE_BAGKEY(Sec);		//VReal or VString: normalized seconds (latitude or longitude type tag only)
	CREATE_BAGKEY(Dir);		//VString: direction ("N", "S", "W" or "E")(latitude or longitude type tag only)
}

END_TOOLBOX_NAMESPACE

USING_TOOLBOX_NAMESPACE

bool IHelperMetadatas::GetString(	const VValueBag *inBag, 
									const XBOX::VValueBag::StKey& inBagKey, VString& outValue, 
									VSize inMaxLength) const
{
	if (inBag)
	{
		if (inBag->GetString( inBagKey, outValue))
		{
			if (outValue.GetLength() > inMaxLength)
				outValue.Truncate( inMaxLength);
			return true;
		}
		else
			return false;
	}
	else
		return false;
}
void IHelperMetadatas::SetString(	VValueBag *ioBag, 
									const XBOX::VValueBag::StKey& inBagKey, const VString& inValue, 
									VSize inMaxLength) 
{
	if (ioBag == NULL)
		return;

	if (inValue.GetLength() > inMaxLength)
	{
		VString value = inValue;
		value.Truncate( inMaxLength);
		ioBag->SetString( inBagKey, value);
	}
	else
		ioBag->SetString( inBagKey, inValue);
}

bool IHelperMetadatas::GetArrayString(	const VValueBag *inBag, 
										const XBOX::VValueBag::StKey& inBagKey, 
										VectorOfVString& outValue) 
{
	if (inBag)
	{
		VString values;
		if (inBag->GetString( inBagKey, values))
		{
			values.GetSubStrings(';', outValue, false, true);
			return outValue.size() > 0;
		}
		else
			return false;
	}
	else
		return false;
}
void IHelperMetadatas::SetArrayString(	VValueBag *ioBag, 
										const XBOX::VValueBag::StKey& inBagKey, 
										const VectorOfVString& inValue) 
{
	if (ioBag == NULL)
		return;
	if (inValue.size() >= 1)
	{
		VString value;
		VectorOfVString::const_iterator it = inValue.begin();
		for (;it != inValue.end(); it++)
		{
			if ((*it).IsEmpty())
				continue;
			if (value.IsEmpty())
				value = *it;
			else
			{
				value.AppendUniChar((UniChar)';');
				value.AppendString( *it);
			}
		}
		ioBag->SetString( inBagKey, value);
	}
	else
		ioBag->SetString( inBagKey, "");
}



bool IHelperMetadatas::GetDateTime(const VValueBag *inBag, 
								   const XBOX::VValueBag::StKey& inBagKey, 
								   VTime& outValue) const 
{
	if (inBag)
	{
		const VValueSingle *value = inBag->GetAttribute( inBagKey);
		if (value)
		{
			if (value->GetValueKind() == VK_TIME)
				outValue = *static_cast<const VTime *>(value);
			else if (value->GetValueKind() == VK_STRING)
				return outValue.FromXMLString( *static_cast<const VString *>(value));
			else
				return false;
			return true;
		}
		else
			return false;
	}
	else
		return false;
}
void IHelperMetadatas::SetDateTime(VValueBag *ioBag, 
								   const XBOX::VValueBag::StKey& inBagKey, 
								   const VTime& inValue)
{
	if (ioBag == NULL)
		return;
	ioBag->SetTime( inBagKey, inValue);		
}

bool IHelperMetadatas::GetArrayDateTime(const VValueBag *inBag, 
										const XBOX::VValueBag::StKey& inBagKey, 
										std::vector<VTime>& outValue) const 
{
	if (inBag == NULL)
		return false;
	const VValueSingle *value = inBag->GetAttribute( inBagKey);
	if (value)
	{
		if (value->GetValueKind() == VK_TIME)
		{
			outValue.push_back( *static_cast<const VTime *>(value));
			return true;
		}
		else
		{
			VectorOfVString times;
			if (GetArrayString( inBag, inBagKey, times))
			{
				VectorOfVString::const_iterator it = times.begin();
				for (;it != times.end(); it++)
				{
					VTime time;
					time.FromXMLString( *it);
					outValue.push_back( time);
				}
				return true;
			}
			else
				return false;
		}
	}
	else
		return false;
}
void IHelperMetadatas::SetArrayDateTime(VValueBag *ioBag, 
										const XBOX::VValueBag::StKey& inBagKey, 
										const std::vector<VTime>& inValue, 
										const XMLStringOptions inXMLOptions)
{
	if (ioBag == NULL)
		return;
	if (inValue.size() == 1)
	{
		ioBag->SetTime( inBagKey, inValue[0]);
	}
	else if (inValue.size() > 1)
	{
		VString value;
		inValue[0].GetXMLString( value, inXMLOptions);
		std::vector<VTime>::const_iterator it = inValue.begin();
		it++;
		for (;it != inValue.end(); it++)
		{
			value.AppendUniChar((UniChar)';');
			VString valueTime;
			(*it).GetXMLString( valueTime, inXMLOptions);
			value.AppendString( valueTime);
		}
		ioBag->SetString( inBagKey, value);
	}
}

bool IHelperMetadatas::GetReal( const VValueBag *inBag, 
								const XBOX::VValueBag::StKey& inBagKey, 
								Real& outValue, 
								Real inMin,
								Real inMax) const
{
	if (inBag)
	{
		if (inBag->GetReal( inBagKey, outValue))
		{
			clampValue(outValue,inMin,inMax); 
			return true;
		}	
		else
			return false;
	}
	else
		return false;
}
void IHelperMetadatas::SetReal( VValueBag *ioBag, 
								const XBOX::VValueBag::StKey& inBagKey, 
								Real inValue, 
								Real inMin,
								Real inMax) 
{
	clampValue(inValue, inMin, inMax);
	ioBag->SetReal( inBagKey, inValue);
}

bool IHelperMetadatas::GetArrayReal(const VValueBag *inBag, 
									const XBOX::VValueBag::StKey& inBagKey, 
									std::vector<Real>& outValue) const
{
	if (inBag)
	{
		VString values;
		if (inBag->GetString( inBagKey, values))
		{
			VectorOfVString vNumber;
			values.GetSubStrings(';', vNumber);
			if (vNumber.size() > 0)
			{
				VectorOfVString::const_iterator it = vNumber.begin();
				for(;it != vNumber.end(); it++)
					outValue.push_back( (*it).GetReal());
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
void IHelperMetadatas::SetArrayReal(VValueBag *ioBag, 
									const XBOX::VValueBag::StKey& inBagKey, 
									const std::vector<Real>& inValue)
{
	if (ioBag == NULL)
		return;
	if (inValue.size() >= 1)
	{
		VString value;
		value.FromReal(inValue[0]);
		std::vector<Real>::const_iterator it = inValue.begin();
		it++;
		for (;it != inValue.end(); it++)
		{
			value.AppendUniChar((UniChar)';');
			value.AppendReal( *it);
		}
		ioBag->SetString( inBagKey, value);
	}
}

bool IHelperMetadatas::GetBlob(	const VValueBag *inBag, 
							    const XBOX::VValueBag::StKey& inBagKey, 
								VBlob& outValue, bool inBase64Only)  
{
	if (inBag == NULL)
		return false;

	//try to decode from base64
	VString value;
	if (!inBag->GetString( inBagKey, value))
		return false;
	//skip header if any
	VIndex lenHeader;
	if ((lenHeader = value.FindUniChar((UniChar)',')) >= 1)
		value.Remove(1,lenHeader);
	if (xerces::Base64::decode( value, outValue))
		return true;

	if (inBase64Only)
		return false;

	//decode from list of string-formatted char values
	std::vector<uCHAR> vChar;
	if (!GetArrayLong(inBag, inBagKey, vChar))
		return false;
	if (vChar.size() == 0)
		return false;
	uCHAR *buffer = new uCHAR[vChar.size()];
	uCHAR *p = buffer;
	std::vector<uCHAR>::const_iterator it = vChar.begin();
	for (;it != vChar.end(); it++, p++)
		*p = *it;

	bool ok = outValue.PutData( buffer, vChar.size()) == VE_OK;

	delete [] buffer;

	return ok;
}

void IHelperMetadatas::SetBlob(	VValueBag *ioBag, 
								const XBOX::VValueBag::StKey& inBagKey, 
								const VBlob& inValue, bool inBase64Only)
{
	VSize blobSize = inValue.GetSize();
	if (blobSize == 0)
		return;
	
	VString value;
	if (blobSize <= 8 && (!inBase64Only))
	{
		uCHAR buffer[8];
		if (inValue.GetData(buffer, blobSize) != VE_OK)
			return;

		//for a very small blob,
		//encode in a string-list of string-formatted uchar values

		uCHAR *p = buffer;
		value.FromLong((sLONG)(*p));
		p++;
		for (int i = 1; i < blobSize; i++,p++)
		{
			value.AppendUniChar(';');
			value.AppendLong((sLONG)(*p));
		}
	}
	else
	{
		//encode using base64 encoding
		if (!xerces::Base64::encode( inValue, value))
			return;
	}
	ioBag->SetString( inBagKey, value);
}


#if VERSIONMAC
bool IHelperMetadatas::GetCFArrayChar(	const VValueBag *inBag, const XBOX::VValueBag::StKey& inBagKey, 
										CFMutableArrayRef& outValue, bool inBase64Only)
{
	//parse as blob
	VBlobWithPtr blob;
	if (!GetBlob( inBag, inBagKey, *(static_cast<VBlob *>(&blob)), inBase64Only))
		return false;
	if (blob.IsNull() || blob.IsEmpty())
		return false;

	//convert blob to CFArray (tags of type undefined are managed as array of SInt32 by ImageIO)
	char *buffer = (char *)blob.GetDataPtr();
	char *p = buffer;
	CFIndex count = (CFIndex)blob.GetSize();
	outValue = CFArrayCreateMutable( kCFAllocatorDefault, count, &kCFTypeArrayCallBacks);
	if (outValue == NULL)
		return false;
	for (int i = 0; i < count; i++, p++)
	{
		int32_t value = (int32_t)(*p);
		CFNumberRef number = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &value);
		CFArrayAppendValue( outValue, number);
		CFRelease(number);
	}
	return true;
}
void IHelperMetadatas::SetCFArrayChar(	VValueBag *ioBag, const XBOX::VValueBag::StKey& inBagKey, 
										CFArrayRef inValue, bool inBase64Only)
{
	//convert CFArrayRef to blob
	xbox_assert(inValue);
	CFTypeRef value;
	CFIndex count = CFArrayGetCount(inValue);
	char *buffer = new char [count];
	char *p = buffer;
	for (CFIndex i = 0; i < count; i++, p++)
	{
		value = CFArrayGetValueAtIndex(inValue, i);
		xbox_assert(CFGetTypeID(value) == CFNumberGetTypeID());
		{
			//get as a number value
			CFNumberRef number = (CFNumberRef)value;
			//get as a signed integer (because tags of type undefined are managed as array of SInt32 by ImageIO)
			int32_t numberInt;
			CFNumberGetValue(number, kCFNumberSInt32Type, &numberInt);
			*p = (char)numberInt;
		}
	}
	
	//set as blob 
	VBlobWithPtr blob;
	blob.PutData( (void *)buffer, count, 0);
	delete [] buffer;
	SetBlob( ioBag, inBagKey, *(static_cast<VBlob *>(&blob)), inBase64Only);
}
#endif




/** get Exif/FlashPix/gps version as a vector of 4 unsigned chars (for instance for exif { 0, 2, 2, 0}) */
bool IHelperMetadatas::GetExifVersion(	const VValueBag *inBag, const XBOX::VValueBag::StKey& inBagKey, 
										std::vector<uCHAR>& outValue) const
{
	if (inBag == NULL)
		return false;
	outValue.clear();
	if (inBagKey.Equal( ImageMetaGPS::VersionID))
	{
		//GPSVersionID tag (for instance "1.1.0.0")

		VString value;
		if (!GetArrayLong(inBag, inBagKey, outValue, (UniChar)'.'))
			return false;
		//ensure vector size is 4
		if (outValue.size() > 4)
			outValue.resize(4);
		for (int i = outValue.size(); i < 4; i++)
			outValue.push_back(0);
		return true;
	}
	else
	{
		//ExifVersion of FlashPixVersion tags (for instance "0220")

		VString value;
		if (!GetString(inBag, inBagKey, value))
			return false;
		if (value.GetLength() != 4)
			return false;
		outValue.push_back((uCHAR)value.GetUniChar(1));
		outValue.push_back((uCHAR)value.GetUniChar(2));
		outValue.push_back((uCHAR)value.GetUniChar(3));
		outValue.push_back((uCHAR)value.GetUniChar(4));
		return true;
	}
}

/** set Exif/FlashPix/gps version as a vector of 4 unsigned chars (for instance for exif { 0, 2, 2, 0}) */
void IHelperMetadatas::SetExifVersion(	VValueBag *ioBag, const XBOX::VValueBag::StKey& inBagKey, 
										const std::vector<uCHAR>& inValue) 
{
	if (ioBag == NULL || inValue.size() == 0)
		return;

	uCHAR version[4];
	VSize size = xbox::Min((unsigned int)inValue.size(),(unsigned int)4);
	for (int i = 0; i < size; i++)
		version[i] = inValue[i];
	for (int i = size; i < 4; i++)
		version[i] = 0;

	VString value;
	if (inBagKey.Equal( ImageMetaGPS::VersionID))
	{
		//GPSVersionID tag (for instance "1.1.0.0")

		char buffer[32];

		#if COMPIL_VISUAL
		sprintf( buffer, "%01d.%01d.%01d.%01d", (sLONG)version[0], (sLONG)version[1], (sLONG)version[2], (sLONG)version[3]);
		#else
		snprintf( buffer, sizeof( buffer), "%01d.%01d.%01d.%01d", (sLONG)version[0], (sLONG)version[1], (sLONG)version[2], (sLONG)version[3]);
		#endif

		value.FromCString( buffer);
	}
	else
	{
		//ExifVersion of FlashPixVersion tags (for instance "0220")

		char buffer[32];

		#if COMPIL_VISUAL
		sprintf( buffer, "%01d%01d%01d%01d", (sLONG)version[0], (sLONG)version[1], (sLONG)version[2], (sLONG)version[3]);
		#else
		snprintf( buffer, sizeof( buffer), "%01d%01d%01d%01d", (sLONG)version[0], (sLONG)version[1], (sLONG)version[2], (sLONG)version[3]);
		#endif

		value.FromCString( buffer);
	}
	ioBag->SetString( inBagKey, value);
}


/** read GPS coordinates from string as degree, minute, second */
bool IHelperMetadatas::GPSScanCoords(	
					const VString& inValue, 
					Real& outDegree, Real& outMinute, Real& outSecond,
					const UniChar inSep) 
{
	VectorOfVString vCoords;
	inValue.GetSubStrings(inSep, vCoords);
	if (vCoords.size() == 0)
		return false;
	//normalize degree, minute, and second
	Real degreeTotal = 0.0;
	if (vCoords.size() >= 1)
		degreeTotal = fabs(vCoords[0].GetReal());
	if (vCoords.size() >= 2)
		degreeTotal += fabs(vCoords[1].GetReal())/60.0;
	if (vCoords.size() >= 3)
		degreeTotal += fabs(vCoords[2].GetReal())/60.0/60.0;

	outDegree = floor(degreeTotal);
	outMinute = floor((degreeTotal-outDegree)*60.0+0.5);
	outSecond = ((degreeTotal-outDegree)*60-outMinute)*60.0;
	if (outSecond <= 0.00001)
		outSecond = 0;
	return true;
}

/** write GPS coordinates to string from degree, minute, second */
void IHelperMetadatas::GPSFormatCoords(	
					VString& outValue, 
					const Real inDegree, const Real inMinute, const Real inSecond,
					const UniChar inSep) 
{
	//normalize degree, minute, and second
	Real degreeTotal = 0.0;
	degreeTotal = fabs(inDegree);
	degreeTotal += fabs(inMinute)/60.0;
	degreeTotal += fabs(inSecond)/60.0/60.0;
	Real degree, minute, second;
	degree = floor(degreeTotal);
	minute = floor((degreeTotal-degree)*60.0+0.5);
	second = ((degreeTotal-degree)*60-minute)*60.0;
	if (second <= 0.00001)
		second = 0;

	char buffer[255];
	char sep = (char)inSep;

	#if COMPIL_VISUAL
	sprintf( buffer, "%02d%c%02d%c%.4G", (sLONG)degree, sep, (sLONG)minute, sep, second);
	outValue.FromCString( buffer);
	#else
	snprintf( buffer, sizeof( buffer), "%02d%c%02d%c%.4G", (sLONG)degree, sep, (sLONG)minute, sep, second);
	outValue.FromCString( buffer);
	#endif
}

/** get latitude as degree, minute, second and latidude ref */
bool IHelperMetadatas::GetLatitude(	const VString& inValue, 
									Real& outDegree, Real& outMinute, Real& outSecond, bool& outIsNorth) const 
{
	VString coords, ref;
	if (inValue.GetLength() >= 1)
	{
		ref.AppendUniChar(inValue.GetUniChar(inValue.GetLength()));
		inValue.GetSubString(1,inValue.GetLength()-1,coords);
		if (!GPSScanCoords( coords, outDegree, outMinute, outSecond))
			return false;
		outIsNorth = ref.EqualToString( CVSTR("N"), false);
		if (!outIsNorth)
			if (!ref.EqualToString( CVSTR("S"), false))
				return false;
		return true;
	}
	else
		return false;
}

/** get latitude as degree, minute, second and latidude ref */
bool IHelperMetadatas::GetLatitude(	const VValueBag *inBag, const XBOX::VValueBag::StKey& inBagKey, 
									Real& outDegree, Real& outMinute, Real& outSecond, bool& outIsNorth) const 
{
	VString value;
	if (!GetString( inBag, inBagKey, value))
		return false;
	return GetLatitude( value, outDegree, outMinute, outSecond, outIsNorth);
}


/** set latitude as degree, minute, second and latidude ref */
void IHelperMetadatas::SetLatitude(	VValueBag *ioBag, const XBOX::VValueBag::StKey& inBagKey, 
									const Real inDegree, const Real inMinute, const Real inSecond, const bool inIsNorth)
{
	VString latitude;
	GPSFormatCoords( latitude, inDegree, inMinute, inSecond);

	if (inIsNorth)
		latitude.AppendCString( "N");
	else
		latitude.AppendCString( "S");

	SetString( ioBag, inBagKey, latitude);
}

/** get longitude as degree, minute, second and longitude ref */
bool IHelperMetadatas::GetLongitude(const VString& inValue, 
									Real& outDegree, Real& outMinute, Real& outSecond, bool& outIsWest) const 
{
	VString coords, ref;
	if (inValue.GetLength() >= 1)
	{
		ref.AppendUniChar(inValue.GetUniChar(inValue.GetLength()));
		inValue.GetSubString(1,inValue.GetLength()-1,coords);
		if (!GPSScanCoords( coords, outDegree, outMinute, outSecond))
			return false;
		outIsWest = ref.EqualToString( CVSTR("W"), false);
		if (!outIsWest)
			if (!ref.EqualToString( CVSTR("E"), false))
				return false;
		return true;
	}
	else
		return false;
}

/** get longitude as degree, minute, second and longitude ref */
bool IHelperMetadatas::GetLongitude(const VValueBag *inBag, const XBOX::VValueBag::StKey& inBagKey, 
									Real& outDegree, Real& outMinute, Real& outSecond, bool& outIsWest) const 
{
	VString value;
	if (!GetString( inBag, inBagKey, value))
		return false;
	return GetLongitude( value, outDegree, outMinute, outSecond, outIsWest);
}

/** set longitude as degree, minute, second and longitude ref */
void IHelperMetadatas::SetLongitude(	VValueBag *ioBag, const XBOX::VValueBag::StKey& inBagKey, 
										const Real inDegree, const Real inMinute, const Real inSecond, 
										const bool inIsWest)
{
	VString longitude;
	GPSFormatCoords( longitude, inDegree, inMinute, inSecond);

	if (inIsWest)
		longitude.AppendCString( "W");
	else
		longitude.AppendCString( "E");

	SetString( ioBag, inBagKey, longitude);
}




/* convert Exif 2.2 GPS coordinates value to XMP GPS coordinates value */
void IHelperMetadatas::GPSCoordsFromExifToXMP(const VString& inExifCoords, const VString& inExifCoordsRef, VString& outXMPCoords) 
{
	outXMPCoords.SetEmpty();
	Real degree, minute, second;

	GPSScanCoords( inExifCoords, degree, minute, second, (UniChar)';');
	GPSFormatCoords( outXMPCoords, degree, minute, second, (UniChar)',');

	outXMPCoords.AppendString(inExifCoordsRef);
}


/* convert  XMP GPS coordinates value to Exif 2.2 GPS coordinates value */
void IHelperMetadatas::GPSCoordsFromXMPToExif(const VString& inXMPCoords, VString& outExifCoords, VString& outExifCoordsRef) 
{
	outExifCoordsRef.SetEmpty();
	outExifCoords.SetEmpty();

	if (inXMPCoords.GetLength() >= 1)
	{

		outExifCoordsRef.AppendUniChar(inXMPCoords.GetUniChar(inXMPCoords.GetLength()));

		VString coords;
		inXMPCoords.GetSubString(1,inXMPCoords.GetLength()-1,coords);

		Real degree, minute, second;
		GPSScanCoords( coords, degree, minute, second, (UniChar)',');

#if VERSIONWIN		
		//WIC stores GPS coordinates as a array of 3 real values (for degree, minutes and seconds)

		GPSFormatCoords( outExifCoords, degree, minute, second, (UniChar)';');
#else
		//ImageIO stores GPS coordinates as a single double value 

		Real degreeTotal = 0.0;
		degreeTotal = fabs(degree);
		degreeTotal += fabs(minute)/60.0;
		degreeTotal += fabs(second)/60.0/60.0;

		VReal valueReal( degreeTotal);
		valueReal.GetString(outExifCoords);
#endif
	}
}


/** convert GPS date+time values to XML datetime value */
void IHelperMetadatas::DateTimeFromGPSToXML(const VString& inGPSDate, const VString& inGPSTime, VString& outXMLDateTime) 
{
	if (inGPSDate.GetLength() < 10)
		outXMLDateTime = "0000-00-00T";
	else
	{
		outXMLDateTime = inGPSDate;
		outXMLDateTime.SetUniChar(5,(UniChar)'-');
		outXMLDateTime.SetUniChar(8,(UniChar)'-');
		outXMLDateTime += "T";
	}

	VectorOfVString vTime;
#if VERSIONWIN	
	inGPSTime.GetSubStrings( ';', vTime, false, true); 
#else
	inGPSTime.GetSubStrings( ':', vTime, false, true);
#endif
	sLONG hour = 0, minute = 0, second = 0;
	if (vTime.size() >= 1)
		hour = vTime[0].GetLong();
	if (vTime.size() >= 2)
		minute = vTime[1].GetLong();
	if (vTime.size() >= 3)
		second = (sLONG)vTime[2].GetReal();
	
	char buffer[255];

	#if COMPIL_VISUAL
	sprintf( buffer, "%02d:%02d:%02dZ", hour, minute, second);
	outXMLDateTime.AppendCString( buffer);
	#else
	snprintf( buffer, sizeof( buffer), "%02d:%02d:%02dZ", hour, minute, second);
	outXMLDateTime.AppendCString( buffer);
	#endif
}

/** convert XML datetime value to GPS date+time values */
void IHelperMetadatas::DateTimeFromXMLToGPS(const VString& inXMLDateTime, VString& outGPSDate, VString& outGPSTime) 
{
	if (inXMLDateTime.IsEmpty())
	{
		outGPSDate.SetEmpty();
		outGPSTime.SetEmpty();
		return;
	}

	if (inXMLDateTime.GetLength() == 20)
	{
		//xml UTC datetime ("YYYY-MM-DDTHH:MM:SSZ"): convert directly to GPS date "YYYY:MM:DD" and GPS time "HH;MM;SS" format

		inXMLDateTime.GetSubString(1,10,outGPSDate);
		outGPSDate.SetUniChar(5,(UniChar)':');
		outGPSDate.SetUniChar(8,(UniChar)':');

		inXMLDateTime.GetSubString(12,8,outGPSTime);
#if VERSIONWIN		
		outGPSTime.SetUniChar(3,(UniChar)';');
		outGPSTime.SetUniChar(6,(UniChar)';');
#else
		outGPSTime.SetUniChar(3,(UniChar)':');
		outGPSTime.SetUniChar(6,(UniChar)':');
		outGPSTime.AppendString(".00");
#endif
	}
	else
	{
		VTime time;
		time.FromXMLString( inXMLDateTime);

		sWORD year, month, day, hour, minute, second, millisecond;
		char buffer[255];

		//datetime with GMT+0 timezone
		time.GetUTCTime(year, month, day, hour, minute, second, millisecond);
		#if COMPIL_VISUAL
		sprintf( buffer, "%04d:%02d:%02d", year, month, day);
		outGPSDate.FromCString( buffer);

		sprintf( buffer, "%02d;%02d;%02d", hour, minute, second);
		outGPSTime.FromCString( buffer);
		#else
		snprintf( buffer, sizeof( buffer), "%04d:%02d:%02d", year, month, day);
		outGPSDate.FromCString( buffer);
		#if VERSIONWIN
		snprintf( buffer, sizeof( buffer), "%02d;%02d;%02d", hour, minute, second);
		outGPSTime.FromCString( buffer);
		#else
		snprintf( buffer, sizeof( buffer), "%02d:%02d:%02d", hour, minute, second);
		outGPSTime.FromCString( buffer);
		outGPSTime.AppendString(".00");
		#endif
		#endif
	}
}





/** convert IPTC date+time values to XML datetime value */
void IHelperMetadatas::DateTimeFromIPTCToXML(const VString& inIPTCDate, const VString& inIPTCTime, VString& outXMLDateTime) 
{
	if (inIPTCDate.GetLength() < 8)
		outXMLDateTime = "0000-00-00T";
	else
	{
		VString sYear, sMonth, sDay;
		inIPTCDate.GetSubString( 1, 4, sYear); 
		inIPTCDate.GetSubString( 5, 2, sMonth); 
		inIPTCDate.GetSubString( 7, 2, sDay); 
		outXMLDateTime = sYear+"-"+sMonth+"-"+sDay+"T";
	}

	if (inIPTCTime.GetLength() < 11)
		outXMLDateTime += "00:00:00Z";
	else
	{
		VString sHour, sMinute, sSecond, sTZSign, sTZHour, sTZMinute;
		inIPTCTime.GetSubString( 1, 2, sHour); 
		inIPTCTime.GetSubString( 3, 2, sMinute); 
		inIPTCTime.GetSubString( 5, 2, sSecond); 
		inIPTCTime.GetSubString( 7, 1, sTZSign); 
		inIPTCTime.GetSubString( 8, 2, sTZHour); 
		inIPTCTime.GetSubString( 10, 2, sTZMinute); 
		outXMLDateTime += sHour+":"+sMinute+":"+sSecond+sTZSign+sTZHour+":"+sTZMinute;
		
		//normalize xml datetime to UTC (in order IPTC datetimes to be compliant with EXIF datetimes)
		VTime time;
		time.FromXMLString(outXMLDateTime);
		time.GetXMLString(outXMLDateTime, XSO_Time_UTC);
	}
}

/** convert XML datetime value to IPTC date+time values */
void IHelperMetadatas::DateTimeFromXMLToIPTC(const VString& inXMLDateTime, VString& outIPTCDate, VString& outIPTCTime) 
{
	if (inXMLDateTime.IsEmpty())
	{
		outIPTCDate.SetEmpty();
		outIPTCTime.SetEmpty();
		return;
	}

	if (inXMLDateTime.GetLength() == 25)
	{
		//xml datetime with timezone : convert directly to IPTC

		VString sYear, sMonth, sDay;
		inXMLDateTime.GetSubString( 1, 4, sYear); 
		inXMLDateTime.GetSubString( 6, 2, sMonth); 
		inXMLDateTime.GetSubString( 9, 2, sDay); 
		outIPTCDate = sYear+sMonth+sDay;

		VString sHour, sMinute, sSecond, sTZSign, sTZHour, sTZMinute;
		inXMLDateTime.GetSubString( 12, 2, sHour); 
		inXMLDateTime.GetSubString( 15, 2, sMinute); 
		inXMLDateTime.GetSubString( 18, 2, sSecond); 
		inXMLDateTime.GetSubString( 20, 1, sTZSign); 
		inXMLDateTime.GetSubString( 21, 2, sTZHour); 
		inXMLDateTime.GetSubString( 24, 2, sTZMinute); 
		outIPTCTime = sHour+sMinute+sSecond+sTZSign+sTZHour+sTZMinute;
	}
	else
	{
		VTime time;
		time.FromXMLString( inXMLDateTime);

		DateTimeFromVTimeToIPTC( time, outIPTCDate, outIPTCTime);
	}
}

/** convert VTime value to IPTC date+time values */
void IHelperMetadatas::DateTimeFromVTimeToIPTC(const VTime& inTime, VString& outIPTCDate, VString& outIPTCTime) 
{
	sWORD year, month, day, hour, minute, second, millisecond;
	char buffer[255];

	//datetime with GMT+0 timezone
	inTime.GetUTCTime(year, month, day, hour, minute, second, millisecond);
	#if COMPIL_VISUAL
	sprintf( buffer, "%04d%02d%02d", year, month, day);
	outIPTCDate.FromCString( buffer);

	sprintf( buffer, "%02d%02d%02d+0000", hour, minute, second);
	outIPTCTime.FromCString( buffer);
	#else
	snprintf( buffer, sizeof( buffer), "%04d%02d%02d", year, month, day);
	outIPTCDate.FromCString( buffer);

	snprintf( buffer, sizeof( buffer), "%02d%02d%02d+0000", hour, minute, second);
	outIPTCTime.FromCString( buffer);
	#endif
}


/** convert Exif datetime value to XML datetime value */
void IHelperMetadatas::DateTimeFromExifToXML(const VString& inExifDateTime, VString& outXMLDateTime) 
{
	if (inExifDateTime.GetLength() == 19)
	{
		//exif datetime format = "YYYY:MM:DD HH:MM:SS"
		//XML datetime format = "YYYY-MM-DDTHH:MM:SSZ"

		outXMLDateTime = inExifDateTime;
		outXMLDateTime.SetUniChar(5,(UniChar)'-');
		outXMLDateTime.SetUniChar(8,(UniChar)'-');
		outXMLDateTime.SetUniChar(11,(UniChar)'T');
		outXMLDateTime += "Z";
	}
	else if (inExifDateTime.GetLength() == 10)
	{
		//exif date format = "YYYY:MM:DD"
		//XML datetime format = "YYYY-MM-DDT00:00:00Z"

		outXMLDateTime = inExifDateTime;
		outXMLDateTime.SetUniChar(5,(UniChar)'-');
		outXMLDateTime.SetUniChar(8,(UniChar)'-');
		outXMLDateTime += "T00:00:00Z";
	}
	else
		outXMLDateTime = "0000-00-00T00:00:00Z";
}

/** convert XML datetime value to Exif datetime value */
void IHelperMetadatas::DateTimeFromXMLToExif(const VString& inXMLDateTime, VString& outExifDateTime) 
{
	if (inXMLDateTime.IsEmpty())
	{
		outExifDateTime.SetEmpty();
		return;
	}

	if (inXMLDateTime.GetLength() == 20)
	{
		//xml UTC datetime ("YYYY-MM-DDTHH:MM:SSZ"): convert directly to Exif "YYYY:MM:DD HH:MM:SS" format

		outExifDateTime = inXMLDateTime;
		outExifDateTime.Truncate(inXMLDateTime.GetLength()-1);
		outExifDateTime.SetUniChar(5,(UniChar)':');
		outExifDateTime.SetUniChar(8,(UniChar)':');
		outExifDateTime.SetUniChar(11,(UniChar)' ');
	}
	else
	{
		VTime time;
		time.FromXMLString( inXMLDateTime);

		DateTimeFromVTimeToExif( time, outExifDateTime);
	}
}

/** convert VTime value to Exif datetime value */
void IHelperMetadatas::DateTimeFromVTimeToExif(const VTime& inTime, VString& outExifDateTime) 
{
	sWORD year, month, day, hour, minute, second, millisecond;
	char buffer[255];

	//datetime with GMT+0 timezone
	inTime.GetUTCTime(year, month, day, hour, minute, second, millisecond);
	#if COMPIL_VISUAL
	sprintf( buffer, "%04d:%02d:%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
	outExifDateTime.FromCString( buffer);
	#else
	snprintf( buffer, sizeof( buffer), "%04d:%02d:%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
	outExifDateTime.FromCString( buffer);
	#endif
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
bool ImageMeta::stReader::GetEXIFFlash(bool& outFired, 
									   ImageMetaEXIF::eFlashReturn *outReturn,  
									   ImageMetaEXIF::eFlashMode *outMode,
									   bool *outFunctionPresent,
									   bool *outRedEyeReduction) const
{
	uLONG flash;
	if (!GetLong( fBagReadEXIF, ImageMetaEXIF::Flash, flash))
		return false;
	outFired = (flash & 1) != 0;
	if (outReturn)
		*outReturn = (ImageMetaEXIF::eFlashReturn)((flash>>1)&3);
	if (outMode)
		*outMode = (ImageMetaEXIF::eFlashMode)((flash>>3)&3);
	if (outFunctionPresent)
		*outFunctionPresent = (flash & (1<<5)) == 0;
	if (outRedEyeReduction)
		*outRedEyeReduction = (flash & (1<<6)) != 0;
	return true;
}


bool ImageMeta::stReader::GetEXIFSubjectAreaType( ImageMetaEXIF::eSubjectArea& outValue) const
{
	std::vector<uLONG> area;
	if (!GetArrayLong( fBagReadEXIF, ImageMetaEXIF::SubjectArea, area))
		return false;
	if (area.size() == 2)
		outValue = ImageMetaEXIF::kSubjectArea_Coordinates;
	else if (area.size() == 3)
		outValue = ImageMetaEXIF::kSubjectArea_Circle;
	else if (area.size() == 4)
		outValue = ImageMetaEXIF::kSubjectArea_Rectangle;
	else
		return false;
	return true;
}

bool ImageMeta::stReader::GetEXIFSubjectAreaCenter( uLONG& outCenterX, uLONG& outCenterY) const
{
	std::vector<uLONG> area;
	if (!GetArrayLong( fBagReadEXIF, ImageMetaEXIF::SubjectArea, area))
		return false;
	if (area.size() < 2)
		return false;
	outCenterX = area[0];
	outCenterY = area[1];
	return true;
}


void ImageMeta::stWriter::SetEXIFSubjectAreaCenter(	const uLONG inCenterX, const uLONG inCenterY)
{
	addMetaEXIF();

	std::vector<uLONG> area;
	area.push_back(inCenterX);
	area.push_back(inCenterY);
	SetArrayLong( fBagWriteEXIF, ImageMetaEXIF::SubjectArea, area);
}


bool ImageMeta::stReader::GetEXIFSubjectAreaCircle( uLONG& outCenterX, uLONG& outCenterY, uLONG& outDiameter) const
{
	std::vector<uLONG> area;
	if (!GetArrayLong( fBagReadEXIF, ImageMetaEXIF::SubjectArea, area))
		return false;
	if (area.size() != 3)
		return false;
	outCenterX	= area[0];
	outCenterY	= area[1];
	outDiameter = area[2];
	return true;
}


void ImageMeta::stWriter::SetEXIFSubjectAreaCircle(		const uLONG inCenterX, const uLONG inCenterY, 
														const uLONG inDiameter)
{
	addMetaEXIF();

	std::vector<uLONG> area;
	area.push_back(inCenterX);
	area.push_back(inCenterY);
	area.push_back(inDiameter);
	SetArrayLong( fBagWriteEXIF, ImageMetaEXIF::SubjectArea, area);
}

bool ImageMeta::stReader::GetEXIFSubjectAreaRectangle( uLONG& outCenterX, uLONG& outCenterY, uLONG& outWidth, uLONG& outHeight) const
{
	std::vector<uLONG> area;
	if (!GetArrayLong( fBagReadEXIF, ImageMetaEXIF::SubjectArea, area))
		return false;
	if (area.size() != 4)
		return false;
	outCenterX	= area[0];
	outCenterY	= area[1];
	outWidth	= area[2];
	outHeight	= area[3];
	return true;
}

void ImageMeta::stWriter::SetEXIFSubjectAreaRectangle(	const uLONG inCenterX, const uLONG inCenterY, 
														const uLONG inWidth, const uLONG inHeight)
{
	addMetaEXIF();

	std::vector<uLONG> area;
	area.push_back(inCenterX);
	area.push_back(inCenterY);
	area.push_back(inWidth);
	area.push_back(inHeight);
	SetArrayLong( fBagWriteEXIF, ImageMetaEXIF::SubjectArea, area);
}


bool ImageMeta::stReader::GetEXIFSubjectLocation( uLONG& outCenterX, uLONG& outCenterY) const
{
	std::vector<uLONG> area;
	if (!GetArrayLong( fBagReadEXIF, ImageMetaEXIF::SubjectLocation, area))
		return false;
	if (area.size() != 2)
		return false;
	outCenterX = area[0];
	outCenterY = area[1];
	return true;
}

void ImageMeta::stWriter::SetEXIFSubjectLocation( const uLONG inCenterX, const uLONG inCenterY)
{
	addMetaEXIF();

	std::vector<uLONG> area;
	area.push_back(inCenterX);
	area.push_back(inCenterY);
	SetArrayLong( fBagWriteEXIF, ImageMetaEXIF::SubjectLocation, area);
}

void ImageMeta::stWriter::SetIPTCActionAdvised( const ImageMetaIPTC::eActionAdvised inValue)
{
	addMetaIPTC();

	char buffer[32];
	VString value;

	#if COMPIL_VISUAL
	sprintf( buffer, "%02d", (sLONG)inValue);
	#else
	snprintf( buffer, sizeof( buffer), "%02d", (sLONG)inValue);
	#endif
	value.FromCString( buffer);

	SetString( fBagWriteIPTC, ImageMetaIPTC::ActionAdvised, value); 
}

bool ImageMeta::stWriter::SetIPTCSubjectReference( const std::vector<sLONG>& inValue)
{
	char buffer[32];
	VString value;

	std::vector<sLONG>::const_iterator it = inValue.begin();
	for (;it != inValue.end(); it++)
	{
		sLONG subjectCode = *it;

		//ensure that subject code is encoded as 8 digits string
		#if COMPIL_VISUAL
		sprintf( buffer, "%08d", subjectCode);
		#else
		snprintf( buffer, sizeof( buffer), "%08d", subjectCode);
		#endif

		if (value.IsEmpty())
			value.FromCString( buffer);
		else
		{
			value.AppendUniChar((UniChar)';');
			value.AppendCString( buffer);
		}
	}
	addMetaIPTC();
	fBagWriteIPTC->SetString( ImageMetaIPTC::SubjectReference, value);
	return true;
}

bool ImageMeta::stWriter::SetIPTCSubjectReference( const VectorOfVString& inValue)
{
	std::vector<sLONG> subjectCodes;
	VectorOfVString::const_iterator it = inValue.begin();
	for (;it != inValue.end(); it++)
		subjectCodes.push_back( (*it).GetLong());
	return SetIPTCSubjectReference( subjectCodes);
}


bool ImageMeta::stWriter::SetIPTCScene( const std::vector<sLONG>& inValue)
{
	char buffer[32];
	VString value;

	std::vector<sLONG>::const_iterator it = inValue.begin();
	for (;it != inValue.end(); it++)
	{
		sLONG scene = *it;
		if (scene < ImageMetaIPTC::kScene_Min || scene > ImageMetaIPTC::kScene_Max)
			return false;

		//ensure that scene code is encoded as 6 digits string
		#if COMPIL_VISUAL
		sprintf( buffer, "%06d", scene);
		#else
		snprintf( buffer, sizeof( buffer), "%06d", scene);
		#endif

		if (value.IsEmpty())
			value.FromCString( buffer);
		else
		{
			value.AppendUniChar((UniChar)';');
			value.AppendCString( buffer);
		}
	}
	addMetaIPTC();
	fBagWriteIPTC->SetString( ImageMetaIPTC::Scene, value);
	return true;
}


void ImageMeta::stWriter::SetIPTCScene( const std::vector<ImageMetaIPTC::eScene>& inValue)
{
	addMetaIPTC();

	if (fBagWriteIPTC == NULL)
		return;
	if (inValue.size() >= 1)
	{
		char buffer[32];
		VString value;

		//ensure that scene code is encoded as 6 digits string
		#if COMPIL_VISUAL
		sprintf( buffer, "%06d", (sLONG)inValue[0]);
		#else
		snprintf( buffer, sizeof( buffer), "%06d", (sLONG)inValue[0]);
		#endif
		value.FromCString( buffer);

		std::vector<ImageMetaIPTC::eScene>::const_iterator it = inValue.begin();
		it++;
		for (;it != inValue.end(); it++)
		{
			value.AppendUniChar((UniChar)';');
			//ensure that scene code is encoded as 6 digits string
			#if COMPIL_VISUAL
			sprintf( buffer, "%06d", (sLONG)(*it));
			#else
			snprintf( buffer, sizeof( buffer), "%06d", (sLONG)(*it));
			#endif
			value.AppendCString( buffer);
		}
		fBagWriteIPTC->SetString( ImageMetaIPTC::Scene, value);
	}
	else
		fBagWriteIPTC->SetString( ImageMetaIPTC::Scene, "");
}

bool ImageMeta::stWriter::SetIPTCScene( const VectorOfVString& inValue)
{
	std::vector<sLONG> scenes;
	VectorOfVString::const_iterator it = inValue.begin();
	for (;it != inValue.end(); it++)
		scenes.push_back( (*it).GetLong());
	return SetIPTCScene( scenes);
}



/** set EXIF flash status
@param inFired
		true if flash is fired
@param inReturn (optional)
		flash return mode
@param inMode (optional)
		flash mode
@param inFunctionPresent (optional)
		true if presence of a flash function
@param inRedEyeReduction (optional)
		true if red-eye reduction is supported
*/
void ImageMeta::stWriter::SetEXIFFlash( const bool inFired, 
				   const ImageMetaEXIF::eFlashReturn inReturn,  
				   const ImageMetaEXIF::eFlashMode inMode,
				   const bool inFunctionPresent,
				   const bool inRedEyeReduction)
{
	addMetaEXIF();

	uLONG flash;
	flash = inFired ? 1 : 0;
	flash |= (((uLONG)inReturn)&3)<<1;
	flash |= (((uLONG)inMode)&3)<<3;
	flash |= inFunctionPresent ? 0 : 1<<5;
	flash |= inRedEyeReduction ? 1<<6 : 0;

	fBagWriteEXIF->SetLong( ImageMetaEXIF::Flash, flash);
}

/** get metadata value as a XML-formatted string value */
bool ImageMeta::stReader::GetMeta( const XBOX::VValueBag::StKey& inKeyBlock, const XBOX::VValueBag::StKey& inKeyTag, VString& outValue)
{
	const XBOX::VValueBag *bag = NULL;
	if (hasMetaIPTC() && inKeyBlock.Equal( ImageMetaIPTC::IPTC))
		bag = fBagReadIPTC;
	else if (hasMetaTIFF() && inKeyBlock.Equal( ImageMetaTIFF::TIFF))
		bag = fBagReadTIFF;
	else if (hasMetaEXIF() && inKeyBlock.Equal( ImageMetaEXIF::EXIF))
		bag = fBagReadEXIF;
	else if (hasMetaGPS() && inKeyBlock.Equal( ImageMetaGPS::GPS))
		bag = fBagReadGPS;
	if (bag)
	{
		const VValueSingle *value = bag->GetAttribute( inKeyTag);
		if (value)
		{
			if (value->GetValueKind() == VK_STRING)
				//we want the untranslated string value so do not call GetXMLString here
				//but get directly the string value
				outValue.FromString( *static_cast<const VString *>(value));
			else
				value->GetXMLString( outValue, XSO_Default);
			return true;
		}
	}

	outValue.SetEmpty();
	return false;
}

/** get metadata value as a XML-formatted string array value */
bool ImageMeta::stReader::GetMeta(	const XBOX::VValueBag::StKey& inKeyBlock, 
									const XBOX::VValueBag::StKey& inKeyTag, 
									VectorOfVString& outValue)
{
	if (inKeyBlock.Equal( ImageMetaEXIF::EXIF))
	{
		if (inKeyTag.Equal( ImageMetaEXIF::ExifVersion))
		{
			uCHAR version[4];
			bool ok = GetEXIFVersion( version[0], version[1], version[2], version[3]);
			if (!ok)
				return false;
			for (int i = 0; i < 4; i++)
			{
				VString value;
				value.FromByte( (sBYTE)(version[i]-48));
				outValue.push_back( value);
			}
			return true;
		}
		else if (inKeyTag.Equal( ImageMetaEXIF::FlashPixVersion))
		{
			uCHAR version[4];
			bool ok = GetEXIFFlashPixVersion( version[0], version[1], version[2], version[3]);
			if (!ok)
				return false;
			for (int i = 0; i < 4; i++)
			{
				VString value;
				value.FromByte( (sBYTE)(version[i]-48));
				outValue.push_back( value);
			}
			return true;
		}
	}
	else if (inKeyBlock.Equal( ImageMetaGPS::GPS))
	{
		if (inKeyTag.Equal( ImageMetaGPS::VersionID))
		{
			uCHAR version[4];
			bool ok = GetGPSVersionID( version[0], version[1], version[2], version[3]);
			if (!ok)
				return false;
			for (int i = 0; i < 4; i++)
			{
				VString value;
				value.FromByte( (sBYTE)(version[i]-48));
				outValue.push_back( value);
			}
			return true;
		}
	}

	VString value;
	if (!GetMeta( inKeyBlock, inKeyTag, value))
		return false;
	value.GetSubStrings( (UniChar)';', outValue, false, true);
	return outValue.size() > 0;
}


/** get metadata value as XML-formatted string value 
@remarks
	return sub-value associated with pseudo-tag
	(for instance for ImageMetaGPS::Min, return normalized minutes from latitude or longitude type tag) 
*/
bool ImageMeta::stReader::GetMeta(	const XBOX::VValueBag::StKey& inKeyBlock, 
									const XBOX::VValueBag::StKey& inKeyTag, 
									const XBOX::VValueBag::StKey& inKeyPseudoTag, 
									VString& outValue)
{
	//get EXIF pseudo-tags
	if (hasMetaEXIF())
	if (inKeyPseudoTag.Equal(ImageMetaEXIF::Fired)
		||
		inKeyPseudoTag.Equal(ImageMetaEXIF::ReturnLight)
		||
		inKeyPseudoTag.Equal(ImageMetaEXIF::Mode)
		||
		inKeyPseudoTag.Equal(ImageMetaEXIF::FunctionPresent)
		||
		inKeyPseudoTag.Equal(ImageMetaEXIF::RedEyeReduction))
	{
		//get Flash pseudo-tags

		bool flashFired = false;
		ImageMetaEXIF::eFlashReturn flashReturn = ImageMetaEXIF::kFlashReturn_NoDetectionFunction;
		ImageMetaEXIF::eFlashMode flashMode = ImageMetaEXIF::kFlashMode_Unknown;
		bool flashFunctionPresent = true;
		bool flashRedEyeReduction = false;

		if (!GetEXIFFlash( flashFired, &flashReturn, &flashMode, &flashFunctionPresent, &flashRedEyeReduction))
			return false;

		if (inKeyPseudoTag.Equal(ImageMetaEXIF::Fired))
			outValue.FromBoolean( flashFired ? 1 : 0);
		else if (inKeyPseudoTag.Equal(ImageMetaEXIF::ReturnLight))
			outValue.FromLong( (sLONG)flashReturn);
		else if (inKeyPseudoTag.Equal(ImageMetaEXIF::Mode))
			outValue.FromLong( (sLONG)flashMode);
		else if (inKeyPseudoTag.Equal(ImageMetaEXIF::FunctionPresent))
			outValue.FromBoolean( flashFunctionPresent ? 1 : 0);
		else 
			outValue.FromBoolean( flashRedEyeReduction ? 1 : 0);
		return true;
	}

	//get GPS pseudo-tags
	if (!hasMetaGPS())
		return false;

	if (inKeyPseudoTag.Equal(ImageMetaGPS::Deg)
		||
		inKeyPseudoTag.Equal(ImageMetaGPS::Min)
		||
		inKeyPseudoTag.Equal(ImageMetaGPS::Sec)
		||
		inKeyPseudoTag.Equal(ImageMetaGPS::Dir))
	{
		if (inKeyTag.Equal(ImageMetaGPS::Latitude)
			|| 
			inKeyTag.Equal(ImageMetaGPS::DestLatitude))
		{
			Real degree = 0, minute = 0, second = 0;
			bool isNorth = true;
			bool found;
			if (inKeyTag.Equal(ImageMetaGPS::Latitude))
				found = GetGPSLatitude( degree, minute, second, isNorth);
			else
				found = GetGPSDestLatitude( degree, minute, second, isNorth);
			if (!found)
				return false;

			if (inKeyPseudoTag.Equal(ImageMetaGPS::Deg))
				outValue.FromReal( degree);
			else if (inKeyPseudoTag.Equal(ImageMetaGPS::Min))
				outValue.FromReal( minute);
			else if (inKeyPseudoTag.Equal(ImageMetaGPS::Sec))
				outValue.FromReal( second);
			else 
				outValue = isNorth ? "N" : "S";
			return true;
		}
		else if (inKeyTag.Equal(ImageMetaGPS::Longitude)
				 ||
				 inKeyTag.Equal(ImageMetaGPS::DestLongitude))
		{
			Real degree = 0, minute = 0, second = 0;
			bool isWest = true;
			bool found;
			if (inKeyTag.Equal(ImageMetaGPS::Longitude))
				found = GetGPSLongitude( degree, minute, second, isWest);
			else
				found = GetGPSDestLongitude( degree, minute, second, isWest);
			if (!found)
				return false;

			if (inKeyPseudoTag.Equal(ImageMetaGPS::Deg))
				outValue.FromReal( degree);
			else if (inKeyPseudoTag.Equal(ImageMetaGPS::Min))
				outValue.FromReal( minute);
			else if (inKeyPseudoTag.Equal(ImageMetaGPS::Sec))
				outValue.FromReal( second);
			else 
				outValue = isWest ? "W" : "E";
			return true;
		}
	}
	return false;
}


/** set metadata value from a XML-formatted string value */
bool ImageMeta::stWriter::SetMeta(const XBOX::VValueBag::StKey& inKeyBlock, 
								  const XBOX::VValueBag::StKey& inKeyTag, 
								  const VString& inValue)
{
	return SetMeta( inKeyBlock, inKeyTag, *static_cast<const VValueSingle *>(&inValue));
}


/** set metadata value from a XML-formatted string array value */
bool ImageMeta::stWriter::SetMeta(	const XBOX::VValueBag::StKey& inKeyBlock, 
									const XBOX::VValueBag::StKey& inKeyTag, 
									const VectorOfVString& inValue)
{
	//filter meta tags which can have more than one value
	//and call appropriate method if any
	if (inKeyBlock.Equal( ImageMetaIPTC::IPTC))
	{
		//check tag value syntax when appropriate
		if (inKeyTag.Equal( ImageMetaIPTC::SubjectReference))
			return SetIPTCSubjectReference( inValue);
		else if (inKeyTag.Equal( ImageMetaIPTC::Scene))
			return SetIPTCScene( inValue);
	}
	else if (inKeyBlock.Equal( ImageMetaTIFF::TIFF))
	{
		if (inKeyTag.Equal( ImageMetaTIFF::TransferFunction))
			return SetTIFFTransferFunction( inValue);
		if (inKeyTag.Equal( ImageMetaTIFF::WhitePoint))
			return SetTIFFWhitePoint( inValue);
		if (inKeyTag.Equal( ImageMetaTIFF::PrimaryChromaticities))
			return SetTIFFPrimaryChromaticities( inValue);

	}
	else if (inKeyBlock.Equal( ImageMetaEXIF::EXIF))
	{
		if (inKeyTag.Equal( ImageMetaEXIF::ISOSpeedRatings))
		{
			SetEXIFISOSpeedRatings( inValue);
			return true;
		}
		else if (inKeyTag.Equal( ImageMetaEXIF::ComponentsConfiguration))
			return SetEXIFComponentsConfiguration( inValue);
		else if (inKeyTag.Equal( ImageMetaEXIF::SubjectArea))
			return SetEXIFSubjectArea( inValue);
	}

	//either meta can have only one value or
	//meta has not dedicated method which can deal with VectorOfVString
	//so convert to VString
	VString values;
	if (inValue.size() > 0)
	{
		VectorOfVString::const_iterator it = inValue.begin();
		for (;it != inValue.end(); it++)
		{
			if ((*it).IsEmpty())
				continue;
			if (values.IsEmpty())
				values = *it;
			else
			{
				values += ";";
				values += *it;
			}
		}
	}
	return SetMeta( inKeyBlock, inKeyTag, values);
}

/** remove metadata */
bool ImageMeta::stWriter::RemoveMeta(	const XBOX::VValueBag::StKey& inKeyBlock, 
										const XBOX::VValueBag::StKey& inKeyTag)
{
	return SetMeta( inKeyBlock, inKeyTag, VString(""));
}


/** set metadata value from a VValueSingle value */
bool ImageMeta::stWriter::SetMeta(const XBOX::VValueBag::StKey& inKeyBlock, 
								  const XBOX::VValueBag::StKey& inKeyTag, 
								  const XBOX::VValueSingle& inValue)
{
	bool bRemoveMeta = false;
	if (inValue.GetValueKind() == VK_STRING)
	{
		if (static_cast<const VString *>(&inValue)->IsEmpty())
			bRemoveMeta = true;
	}

	XBOX::VValueBag *bag = NULL;

	if (inKeyBlock.Equal( ImageMetaIPTC::IPTC))
	{
		//check tag value syntax when appropriate
		if (!bRemoveMeta)
		{
			if (inKeyTag.Equal( ImageMetaIPTC::Urgency))
				return SetIPTCUrgency( inValue.GetLong());
			else if (inKeyTag.Equal( ImageMetaIPTC::ActionAdvised))
				return SetIPTCActionAdvised( inValue.GetLong());
			else if (inKeyTag.Equal( ImageMetaIPTC::ObjectCycle))
			{
				VString value;
				inValue.GetString( value); 
				return SetIPTCObjectCycle( value);
			}
			else if (inKeyTag.Equal( ImageMetaIPTC::SubjectReference))
			{
				VString value;
				inValue.GetString( value); 
				VectorOfVString values;
				value.GetSubStrings( (UniChar)';', values, false, true);
				return SetIPTCSubjectReference( values);
			}
			else if (inKeyTag.Equal( ImageMetaIPTC::Scene))
			{
				VString value;
				inValue.GetString( value); 
				VectorOfVString values;
				value.GetSubStrings( (UniChar)';', values, false, true);
				return SetIPTCScene( values);
			}
			else if (inKeyTag.Equal( ImageMetaIPTC::ImageType))
			{
				VString value;
				inValue.GetString( value); 
				return SetIPTCImageType( value);
			}
			else if (inKeyTag.Equal( ImageMetaIPTC::ImageOrientation))
			{
				VString value;
				inValue.GetString( value); 
				return SetIPTCImageOrientation( value);
			}
		}

		addMetaIPTC();
		bag = fBagWriteIPTC;
	}
	else if (inKeyBlock.Equal( ImageMetaTIFF::TIFF))
	{
		//check tag value syntax when appropriate
		if (!bRemoveMeta)
		{
			if (inKeyTag.Equal( ImageMetaTIFF::Orientation))
				return SetTIFFOrientation( inValue.GetLong());
			else if (inKeyTag.Equal( ImageMetaTIFF::ResolutionUnit))
				return SetTIFFResolutionUnit( inValue.GetLong());
			else if (inKeyTag.Equal( ImageMetaTIFF::TransferFunction))
			{
				VString value;
				inValue.GetString( value); 
				VectorOfVString values;
				value.GetSubStrings( (UniChar)';', values, false, true);
				if (values.size() != 3*256)
					return false;
			}
			else if (inKeyTag.Equal( ImageMetaTIFF::WhitePoint))
			{
				VString value;
				inValue.GetString( value); 
				VectorOfVString values;
				value.GetSubStrings( (UniChar)';', values, false, true);
				if (values.size() != 2)
					return false;
			}
			else if (inKeyTag.Equal( ImageMetaTIFF::PrimaryChromaticities))
			{
				VString value;
				inValue.GetString( value); 
				VectorOfVString values;
				value.GetSubStrings( (UniChar)';', values, false, true);
				if (values.size() != 6)
					return false;
			}
		}
		addMetaTIFF();
		bag = fBagWriteTIFF;
	}
	else if (inKeyBlock.Equal( ImageMetaEXIF::EXIF))
	{
		//check tag value syntax when appropriate
		if (!bRemoveMeta)
		{
			if (inKeyTag.Equal( ImageMetaEXIF::ExposureProgram))
				return SetEXIFExposureProgram( inValue.GetLong());
			else if (inKeyTag.Equal( ImageMetaEXIF::ExifVersion))
			{
				VString value;
				inValue.GetString( value);
				value.ExchangeAll(CVSTR(";"),CVSTR("")); //remove seps if set from a array of values
				return SetEXIFVersion( value);
			}
			else if (inKeyTag.Equal( ImageMetaEXIF::ComponentsConfiguration))
			{
				VString value;
				inValue.GetString( value); 
				VectorOfVString values;
				value.GetSubStrings( (UniChar)';', values, false, true);
				return SetEXIFComponentsConfiguration( values);
			}
			else if (inKeyTag.Equal( ImageMetaEXIF::SubjectArea))
			{
				VString value;
				inValue.GetString( value); 
				VectorOfVString values;
				value.GetSubStrings( (UniChar)';', values, false, true);
				return SetEXIFSubjectArea( values);
			}
			else if (inKeyTag.Equal( ImageMetaEXIF::FlashPixVersion))
			{
				VString value;
				inValue.GetString( value);
				value.ExchangeAll(CVSTR(";"),CVSTR("")); //remove seps if set from a array of values
				return SetEXIFFlashPixVersion( value);
			}
			if (inKeyTag.Equal( ImageMetaEXIF::FocalPlaneResolutionUnit))
				return SetEXIFFocalPlaneResolutionUnit( inValue.GetLong());
			else if (inKeyTag.Equal( ImageMetaEXIF::CustomRendered))
			{
				SetEXIFCustomRendered( inValue.GetLong() != 0);
				return true;
			}
			else if (inKeyTag.Equal( ImageMetaEXIF::ExposureMode))
				return SetEXIFExposureMode( inValue.GetLong());
			else if (inKeyTag.Equal( ImageMetaEXIF::WhiteBalance))
			{
				SetEXIFWhiteBalance( inValue.GetLong() != 0);
				return true;
			}
			else if (inKeyTag.Equal( ImageMetaEXIF::SceneCaptureType))
				return SetEXIFSceneCaptureType( inValue.GetLong());
			else if (inKeyTag.Equal( ImageMetaEXIF::GainControl))
				return SetEXIFGainControl( inValue.GetLong());
			else if (inKeyTag.Equal( ImageMetaEXIF::Contrast))
				return SetEXIFContrast( inValue.GetLong());
			else if (inKeyTag.Equal( ImageMetaEXIF::Saturation))
				return SetEXIFSaturation( inValue.GetLong());
			else if (inKeyTag.Equal( ImageMetaEXIF::Sharpness))
				return SetEXIFSharpness( inValue.GetLong());
			else if (inKeyTag.Equal( ImageMetaEXIF::SubjectDistRange))
				return SetEXIFSubjectDistRange( inValue.GetLong());
		}
		addMetaEXIF();
		bag = fBagWriteEXIF;
	}
	else if (inKeyBlock.Equal( ImageMetaGPS::GPS))
	{
		//check tag value syntax when appropriate
		if (!bRemoveMeta)
		{
			if (inKeyTag.Equal( ImageMetaGPS::VersionID))
			{
				VString value;
				inValue.GetString( value);
				value.ExchangeAll((UniChar)';',(UniChar)'.'); //replace seps if set from a array of values
				return SetGPSVersionID( value);
			}
			else if (inKeyTag.Equal( ImageMetaGPS::Latitude))
			{
				VString value;
				inValue.GetString( value);
				return SetGPSLatitude( value);
			}
			else if (inKeyTag.Equal( ImageMetaGPS::DestLatitude))
			{
				VString value;
				inValue.GetString( value);
				return SetGPSDestLatitude( value);
			}
			else if (inKeyTag.Equal( ImageMetaGPS::Longitude))
			{
				VString value;
				inValue.GetString( value);
				return SetGPSLongitude( value);
			}
			else if (inKeyTag.Equal( ImageMetaGPS::DestLongitude))
			{
				VString value;
				inValue.GetString( value);
				return SetGPSDestLongitude( value);
			}
			else if (inKeyTag.Equal( ImageMetaGPS::AltitudeRef))
				return SetGPSAltitudeRef( inValue.GetLong());
			else if (inKeyTag.Equal( ImageMetaGPS::MeasureMode))
				return SetGPSMeasureMode( inValue.GetLong());
			else if (inKeyTag.Equal( ImageMetaGPS::Differential))
			{
				SetGPSDifferential( inValue.GetLong() != 0);
				return true;
			}
			else if (inKeyTag.Equal( ImageMetaGPS::Status))
			{
				VString value;
				inValue.GetString( value);
				return SetGPSStatus( value);
			}
			else if (inKeyTag.Equal( ImageMetaGPS::SpeedRef))
			{
				VString value;
				inValue.GetString( value);
				return SetGPSSpeedRef( value);
			}
			else if (inKeyTag.Equal( ImageMetaGPS::TrackRef))
			{
				VString value;
				inValue.GetString( value);
				return SetGPSTrackRef( value);
			}
			else if (inKeyTag.Equal( ImageMetaGPS::ImgDirectionRef))
			{
				VString value;
				inValue.GetString( value);
				return SetGPSImgDirectionRef( value);
			}
			else if (inKeyTag.Equal( ImageMetaGPS::DestBearingRef))
			{
				VString value;
				inValue.GetString( value);
				return SetGPSDestBearingRef( value);
			}
			else if (inKeyTag.Equal( ImageMetaGPS::DestDistanceRef))
			{
				VString value;
				inValue.GetString( value);
				return SetGPSDestDistanceRef( value);
			}
		}

		addMetaGPS();
		bag = fBagWriteGPS;
	}
	if (bag)
	{
		bag->SetAttribute( inKeyTag, inValue);
		return true;
	}
	else
		return false;
}

/** set metadata value from XML-formatted string value 
@remarks
	set only sub-value associated with pseudo-tag
	(for instance for ImageMetaGPS::Min, update only normalized minutes from latitude or longitude type tag) 
*/
bool ImageMeta::stWriter::SetMeta(	const XBOX::VValueBag::StKey& inKeyBlock, 
									const XBOX::VValueBag::StKey& inKeyTag, 
									const XBOX::VValueBag::StKey& inKeyPseudoTag, 
									const VString& inValue,
									const VValueBag *inMetasSource)
{
	return SetMeta( inKeyBlock, inKeyTag, inKeyPseudoTag, *static_cast<const VValueSingle *>(&inValue), inMetasSource);
}

/** set metadata value from a VValueSingle value 
@remarks
	set only sub-value associated with pseudo-tag
	(for instance for ImageMetaGPS::Min, update only normalized minutes from latitude or longitude type tag) 
*/
bool ImageMeta::stWriter::SetMeta(	const XBOX::VValueBag::StKey& inKeyBlock, 
									const XBOX::VValueBag::StKey& inKeyTag, 
									const XBOX::VValueBag::StKey& inKeyPseudoTag, 
									const VValueSingle& inValue,
									const VValueBag *inMetasSource)
{
	bool bRemoveMeta = false;
	if (inValue.GetValueKind() == VK_STRING)
	{
		if (static_cast<const VString *>(&inValue)->IsEmpty())
			bRemoveMeta = true;
	}
	if (bRemoveMeta)
		return SetMeta(inKeyBlock,inKeyTag,inValue);

	//set EXIF Flash pseudo-tags
	if (inKeyPseudoTag.Equal(ImageMetaEXIF::Fired)
		||
		inKeyPseudoTag.Equal(ImageMetaEXIF::ReturnLight)
		||
		inKeyPseudoTag.Equal(ImageMetaEXIF::Mode)
		||
		inKeyPseudoTag.Equal(ImageMetaEXIF::FunctionPresent)
		||
		inKeyPseudoTag.Equal(ImageMetaEXIF::RedEyeReduction))
	{
		bool flashFired = false;
		ImageMetaEXIF::eFlashReturn flashReturn = ImageMetaEXIF::kFlashReturn_NoDetectionFunction;
		ImageMetaEXIF::eFlashMode flashMode = ImageMetaEXIF::kFlashMode_Unknown;
		bool flashFunctionPresent = true;
		bool flashRedEyeReduction = false;

		{
			ImageMeta::stReader reader( fBagWrite);
			if (!reader.GetEXIFFlash( flashFired, &flashReturn, &flashMode, &flashFunctionPresent, &flashRedEyeReduction))
			{
				if (inMetasSource)
				{
					ImageMeta::stReader readerSource( inMetasSource);
					readerSource.GetEXIFFlash( flashFired, &flashReturn, &flashMode, &flashFunctionPresent, &flashRedEyeReduction);
				}
			}
		}

		if (inKeyPseudoTag.Equal(ImageMetaEXIF::Fired))
			flashFired = inValue.GetBoolean() != 0;
		else if (inKeyPseudoTag.Equal(ImageMetaEXIF::ReturnLight))
		{
			sLONG returnLight = inValue.GetLong();
			if (returnLight < ImageMetaEXIF::kFlashReturn_Min || returnLight > ImageMetaEXIF::kFlashReturn_Max)
				return false;
			flashReturn = (ImageMetaEXIF::eFlashReturn)returnLight;
		}
		else if (inKeyPseudoTag.Equal(ImageMetaEXIF::Mode))
		{
			sLONG mode = inValue.GetLong();
			if (mode < ImageMetaEXIF::kFlashMode_Min || mode > ImageMetaEXIF::kFlashMode_Max)
				return false;
			flashMode = (ImageMetaEXIF::eFlashMode)mode;
		}
		else if (inKeyPseudoTag.Equal(ImageMetaEXIF::FunctionPresent))
			flashFunctionPresent = inValue.GetBoolean() != 0;
		else 
			flashRedEyeReduction = inValue.GetBoolean() != 0;

		SetEXIFFlash( flashFired, flashReturn, flashMode, flashFunctionPresent, flashRedEyeReduction);
		return true;
	}
	//set GPS pseudo-tags
	else if (inKeyPseudoTag.Equal(ImageMetaGPS::Deg)
			||
			inKeyPseudoTag.Equal(ImageMetaGPS::Min)
			||
			inKeyPseudoTag.Equal(ImageMetaGPS::Sec)
			||
			inKeyPseudoTag.Equal(ImageMetaGPS::Dir))
	{
		if (inKeyTag.Equal(ImageMetaGPS::Latitude) 
			|| 
			inKeyTag.Equal(ImageMetaGPS::DestLatitude))
		{
			Real degree = 0, minute = 0, second = 0;
			bool isNorth = true;
			{
				ImageMeta::stReader reader( fBagWrite);
				bool ok;
				if (inKeyTag.Equal(ImageMetaGPS::Latitude))
					ok = reader.GetGPSLatitude( degree, minute, second, isNorth);
				else
					ok = reader.GetGPSDestLatitude( degree, minute, second, isNorth);
				if (!ok)
				{
					if (inMetasSource)
					{
						ImageMeta::stReader readerSource( inMetasSource);
						if (inKeyTag.Equal(ImageMetaGPS::Latitude))
							readerSource.GetGPSLatitude( degree, minute, second, isNorth);
						else
							readerSource.GetGPSDestLatitude( degree, minute, second, isNorth);
					}
				}
			}

			if (inKeyPseudoTag.Equal(ImageMetaGPS::Deg))
				degree = inValue.GetReal();
			else if (inKeyPseudoTag.Equal(ImageMetaGPS::Min))
				minute = inValue.GetReal();
			else if (inKeyPseudoTag.Equal(ImageMetaGPS::Sec))
				second = inValue.GetReal();
			else 
			{
				VString direction;
				inValue.GetString( direction);
				if (direction.GetLength() > 0 && isdigit( (int) direction.GetUniChar(1)))
					isNorth = direction.GetLong() != 0;
				else
					isNorth = direction.EqualToString( "N");
			}
	
			if (inKeyTag.Equal(ImageMetaGPS::Latitude))
				SetGPSLatitude( degree, minute, second, isNorth);
			else
				SetGPSDestLatitude( degree, minute, second, isNorth);
			return true;
		}
		else if (inKeyTag.Equal(ImageMetaGPS::Longitude)
				 ||
				 inKeyTag.Equal(ImageMetaGPS::DestLongitude))
		{
			Real degree = 0, minute = 0, second = 0;
			bool isWest = true;
			{
				ImageMeta::stReader reader( fBagWrite);
				bool ok;
				if (inKeyTag.Equal(ImageMetaGPS::Longitude))
					ok = reader.GetGPSLongitude( degree, minute, second, isWest);
				else
					ok = reader.GetGPSDestLongitude( degree, minute, second, isWest);
				if (!ok)
				{
					if (inMetasSource)
					{
						ImageMeta::stReader readerSource( inMetasSource);
						if (inKeyTag.Equal(ImageMetaGPS::Longitude))
							readerSource.GetGPSLongitude( degree, minute, second, isWest);
						else
							readerSource.GetGPSDestLongitude( degree, minute, second, isWest);
					}
				}
			}

			if (inKeyPseudoTag.Equal(ImageMetaGPS::Deg))
				degree = inValue.GetReal();
			else if (inKeyPseudoTag.Equal(ImageMetaGPS::Min))
				minute = inValue.GetReal();
			else if (inKeyPseudoTag.Equal(ImageMetaGPS::Sec))
				second = inValue.GetReal();
			else 
			{
				VString direction;
				inValue.GetString( direction);
				if (direction.GetLength() > 0 && isdigit( (int) direction.GetUniChar(1)))
					isWest = direction.GetLong() != 0;
				else
					isWest = direction.EqualToString( "W");
			}
	
			if (inKeyTag.Equal(ImageMetaGPS::Longitude))
				SetGPSLongitude( degree, minute, second, isWest);
			else
				SetGPSDestLongitude( degree, minute, second, isWest);
			return true;
		}
	}
	return false;
}


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
// (to avoid to link Graphics dll with xerces)

namespace xerces
{

// ---------------------------------------------------------------------------
//  constants
// ---------------------------------------------------------------------------
static const int BASELENGTH = 255;
static const int FOURBYTE   = 4;

const XMLCh chSpace					= 0x20;
const XMLCh chPlus                  = 0x2B;
const XMLCh chForwardSlash          = 0x2F;
const XMLCh chNull                  = 0x00;
const XMLCh chEqual                 = 0x3D;
const XMLCh chLF                    = 0x0A;

const XMLCh chDigit_0               = 0x30;
const XMLCh chDigit_1               = 0x31;
const XMLCh chDigit_2               = 0x32;
const XMLCh chDigit_3               = 0x33;
const XMLCh chDigit_4               = 0x34;
const XMLCh chDigit_5               = 0x35;
const XMLCh chDigit_6               = 0x36;
const XMLCh chDigit_7               = 0x37;
const XMLCh chDigit_8               = 0x38;
const XMLCh chDigit_9               = 0x39;

const XMLCh chLatin_A               = 0x41;
const XMLCh chLatin_B               = 0x42;
const XMLCh chLatin_C               = 0x43;
const XMLCh chLatin_D               = 0x44;
const XMLCh chLatin_E               = 0x45;
const XMLCh chLatin_F               = 0x46;
const XMLCh chLatin_G               = 0x47;
const XMLCh chLatin_H               = 0x48;
const XMLCh chLatin_I               = 0x49;
const XMLCh chLatin_J               = 0x4A;
const XMLCh chLatin_K               = 0x4B;
const XMLCh chLatin_L               = 0x4C;
const XMLCh chLatin_M               = 0x4D;
const XMLCh chLatin_N               = 0x4E;
const XMLCh chLatin_O               = 0x4F;
const XMLCh chLatin_P               = 0x50;
const XMLCh chLatin_Q               = 0x51;
const XMLCh chLatin_R               = 0x52;
const XMLCh chLatin_S               = 0x53;
const XMLCh chLatin_T               = 0x54;
const XMLCh chLatin_U               = 0x55;
const XMLCh chLatin_V               = 0x56;
const XMLCh chLatin_W               = 0x57;
const XMLCh chLatin_X               = 0x58;
const XMLCh chLatin_Y               = 0x59;
const XMLCh chLatin_Z               = 0x5A;

const XMLCh chLatin_a               = 0x61;
const XMLCh chLatin_b               = 0x62;
const XMLCh chLatin_c               = 0x63;
const XMLCh chLatin_d               = 0x64;
const XMLCh chLatin_e               = 0x65;
const XMLCh chLatin_f               = 0x66;
const XMLCh chLatin_g               = 0x67;
const XMLCh chLatin_h               = 0x68;
const XMLCh chLatin_i               = 0x69;
const XMLCh chLatin_j               = 0x6A;
const XMLCh chLatin_k               = 0x6B;
const XMLCh chLatin_l               = 0x6C;
const XMLCh chLatin_m               = 0x6D;
const XMLCh chLatin_n               = 0x6E;
const XMLCh chLatin_o               = 0x6F;
const XMLCh chLatin_p               = 0x70;
const XMLCh chLatin_q               = 0x71;
const XMLCh chLatin_r               = 0x72;
const XMLCh chLatin_s               = 0x73;
const XMLCh chLatin_t               = 0x74;
const XMLCh chLatin_u               = 0x75;
const XMLCh chLatin_v               = 0x76;
const XMLCh chLatin_w               = 0x77;
const XMLCh chLatin_x               = 0x78;
const XMLCh chLatin_y               = 0x79;
const XMLCh chLatin_z               = 0x7A;


// ---------------------------------------------------------------------------
//  class data member
// ---------------------------------------------------------------------------

// the base64 alphabet according to definition in RFC 2045
const XMLByte Base64::base64Alphabet[] = {
    chLatin_A, chLatin_B, chLatin_C, chLatin_D, chLatin_E,
    chLatin_F, chLatin_G, chLatin_H, chLatin_I, chLatin_J,
    chLatin_K, chLatin_L, chLatin_M, chLatin_N, chLatin_O,
    chLatin_P, chLatin_Q, chLatin_R, chLatin_S, chLatin_T,
    chLatin_U, chLatin_V, chLatin_W, chLatin_X, chLatin_Y, chLatin_Z,
	chLatin_a, chLatin_b, chLatin_c, chLatin_d, chLatin_e,
	chLatin_f, chLatin_g, chLatin_h, chLatin_i, chLatin_j,
	chLatin_k, chLatin_l, chLatin_m, chLatin_n, chLatin_o,
	chLatin_p, chLatin_q, chLatin_r, chLatin_s, chLatin_t,
	chLatin_u, chLatin_v, chLatin_w, chLatin_x, chLatin_y, chLatin_z,
	chDigit_0, chDigit_1, chDigit_2, chDigit_3, chDigit_4,
	chDigit_5, chDigit_6, chDigit_7, chDigit_8, chDigit_9,
	chPlus, chForwardSlash, chNull
};

// This is an inverse table for base64 decoding.  So, if
// base64Alphabet[17] = 'R', then base64Inverse['R'] = 17.
//
// For characters not in base64Alphabet then
// base64Inverse[char] = 0xFF.
XMLByte Base64::base64Inverse[BASELENGTH] =
{
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3E, 0xFF, 0xFF, 0xFF, 0x3F, 
    0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 
    0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 
    0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};


const XMLByte Base64::base64Padding = chEqual;

// number of quadruplets per one line ( must be >1 and <19 )
const unsigned int Base64::quadsPerLine = 15;

bool Base64::isInitialized = true;


/**
 * Encodes input blob into Base64 VString
  
   return true if blob is successfully encoded
 */
bool Base64::encode(const VBlob& inBlob,
					VString& outTextBase64)
{
	if (inBlob.IsNull() || inBlob.GetSize() == 0)
		return false;
	VSize inputLength = inBlob.GetSize();
    int quadrupletCount = ( inputLength + 2 ) / 3;
    if (quadrupletCount == 0)
		return false;

	XMLByte* inputData = new XMLByte [inputLength];
	XBOX::VError error = inBlob.GetData(inputData, inputLength);
	if (error != VE_OK)
	{
		delete [] inputData;
		return false;
	}


    // number of rows in encoded stream ( including the last one )
    int lineCount = ( quadrupletCount + quadsPerLine-1 ) / quadsPerLine;

	//
    // convert the triplet(s) to quadruplet(s)
    //
    XMLByte  b1, b2, b3, b4;  // base64 binary codes ( 0..63 )

    unsigned int inputIndex = 0;
    unsigned int outputIndex = 0;
    XMLByte *encodedData = new XMLByte [quadrupletCount*FOURBYTE+lineCount+1];

    //
    // Process all quadruplet(s) except the last
    //
    int quad = 1;
    for (; quad <= quadrupletCount-1; quad++ )
    {
        // read triplet from the input stream
        split1stOctet( inputData[ inputIndex++ ], b1, b2 );
        split2ndOctet( inputData[ inputIndex++ ], b2, b3 );
        split3rdOctet( inputData[ inputIndex++ ], b3, b4 );

        // write quadruplet to the output stream
        encodedData[ outputIndex++ ] = base64Alphabet[ b1 ];
        encodedData[ outputIndex++ ] = base64Alphabet[ b2 ];
        encodedData[ outputIndex++ ] = base64Alphabet[ b3 ];
        encodedData[ outputIndex++ ] = base64Alphabet[ b4 ];

        if (( quad % quadsPerLine ) == 0 )
            encodedData[ outputIndex++ ] = chLF;
    }

    //
    // process the last Quadruplet
    //
    // first octet is present always, process it
    split1stOctet( inputData[ inputIndex++ ], b1, b2 );
    encodedData[ outputIndex++ ] = base64Alphabet[ b1 ];

    if( inputIndex < inputLength )
    {
        // second octet is present, process it
        split2ndOctet( inputData[ inputIndex++ ], b2, b3 );
        encodedData[ outputIndex++ ] = base64Alphabet[ b2 ];

        if( inputIndex < inputLength )
        {
            // third octet present, process it
            // no PAD e.g. 3cQl
            split3rdOctet( inputData[ inputIndex++ ], b3, b4 );
            encodedData[ outputIndex++ ] = base64Alphabet[ b3 ];
            encodedData[ outputIndex++ ] = base64Alphabet[ b4 ];
        }
        else
        {
            // third octet not present
            // one PAD e.g. 3cQ=
            encodedData[ outputIndex++ ] = base64Alphabet[ b3 ];
            encodedData[ outputIndex++ ] = base64Padding;
        }
    }
    else
    {
        // second octet not present
        // two PADs e.g. 3c==
        encodedData[ outputIndex++ ] = base64Alphabet[ b2 ];
        encodedData[ outputIndex++ ] = base64Padding;
        encodedData[ outputIndex++ ] = base64Padding;
    }

    // write out end of the last line
    //encodedData[ outputIndex++ ] = chLF;
    // write out end of string
    encodedData[ outputIndex ] = 0;

	outTextBase64.SetEmpty();
	outTextBase64.EnsureSize(outputIndex);
	for (int i = 0; i < outputIndex; i++)
		outTextBase64.AppendUniChar( (UniChar)encodedData[i]);

	delete [] encodedData;
	delete [] inputData;
	return true;
}

/**
 * Decodes input Base64 VString into output blob
 */
bool Base64::decode( const VString& inTextBase64,
					 VBlob& outBlob,	
                     Conformance  inConform
                   )
{
    int inputLength = inTextBase64.GetLength();
	if (inputLength == 0)
		return false;

	//copy input string to XMLByte table
	XMLByte *rawInputData = new XMLByte [inputLength+1];
	const UniChar *inputData = inTextBase64.GetCPointer();

    //
    // remove all XML whitespaces from the base64Data
    //
    int inputIndex = 0;
    int rawInputLength = 0;
    bool inWhiteSpace = false;

    switch (inConform)
    {
    case Conf_RFC2045:
        while ( inputIndex < inputLength )
        {
            if (!isWhiteSpace((XMLByte)inputData[inputIndex]))
            {
                rawInputData[ rawInputLength++ ] = (XMLByte)inputData[ inputIndex ];
            }
            // RFC2045 does not explicitly forbid more than ONE whitespace 
            // before, in between, or after base64 octects.
            // Besides, S? allows more than ONE whitespace as specified in the production 
            // [3]   S   ::=   (#x20 | #x9 | #xD | #xA)+
            // therefore we do not detect multiple ws

            inputIndex++;
        }

        break;
    case Conf_Schema:
        // no leading #x20
        if (chSpace == inputData[inputIndex])
		{
			delete [] rawInputData;
            return false;
		}
        while ( inputIndex < inputLength )
        {
            if (chSpace != inputData[inputIndex])
            {
                rawInputData[ rawInputLength++ ] = (XMLByte)inputData[ inputIndex ];
                inWhiteSpace = false;
            }
            else
            {
                if (inWhiteSpace)
				{
					delete [] rawInputData;
                    return false; // more than 1 #x20 encountered
				}
                else
                    inWhiteSpace = true;
            }

            inputIndex++;
        }

        // no trailing #x20
        if (inWhiteSpace)
		{
			delete [] rawInputData;
            return false;
		}
        break;

    default:
        break;
    }

    //now rawInputData contains canonical representation 
    //if the data is valid Base64
    rawInputData[ rawInputLength ] = 0;

    // the length of raw data should be divisible by four
    if (( rawInputLength % FOURBYTE ) != 0 )
	{
		delete [] rawInputData;
        return false;
	}

    int quadrupletCount = rawInputLength / FOURBYTE;
    if ( quadrupletCount == 0 )
	{
		delete [] rawInputData;
        return false;
	}

    //
    // convert the quadruplet(s) to triplet(s)
    //
    XMLByte  d1, d2, d3, d4;  // base64 characters
    XMLByte  b1, b2, b3, b4;  // base64 binary codes ( 0..64 )

    int rawInputIndex  = 0;
    int outputIndex    = 0;
	XMLByte *decodedData = new XMLByte [ quadrupletCount*3+1 ];

    //
    // Process all quadruplet(s) except the last
    //
    int quad = 1;
    for (; quad <= quadrupletCount-1; quad++ )
    {
        // read quadruplet from the input stream
        if (!isData( (d1 = rawInputData[ rawInputIndex++ ]) ) ||
            !isData( (d2 = rawInputData[ rawInputIndex++ ]) ) ||
            !isData( (d3 = rawInputData[ rawInputIndex++ ]) ) ||
            !isData( (d4 = rawInputData[ rawInputIndex++ ]) ))
		{
			delete [] decodedData;
			delete [] rawInputData;
			return false;
		}

        b1 = base64Inverse[ d1 ];
        b2 = base64Inverse[ d2 ];
        b3 = base64Inverse[ d3 ];
        b4 = base64Inverse[ d4 ];

        // write triplet to the output stream
        decodedData[ outputIndex++ ] = set1stOctet(b1, b2);
        decodedData[ outputIndex++ ] = set2ndOctet(b2, b3);
        decodedData[ outputIndex++ ] = set3rdOctet(b3, b4);
    }

    //
    // process the last Quadruplet
    //
    // first two octets are present always, process them
    if (!isData( (d1 = rawInputData[ rawInputIndex++ ]) ) ||
        !isData( (d2 = rawInputData[ rawInputIndex++ ]) ))
	{
		delete [] decodedData;
		delete [] rawInputData;
        return false;
	}

    b1 = base64Inverse[ d1 ];
    b2 = base64Inverse[ d2 ];

    // try to process last two octets
    d3 = rawInputData[ rawInputIndex++ ];
    d4 = rawInputData[ rawInputIndex++ ];

    if (!isData( d3 ) || !isData( d4 ))
    {
        // check if last two are PAD characters
        if (isPad( d3 ) && isPad( d4 ))
        {
            // two PAD e.g. 3c==
            if ((b2 & 0xf) != 0) // last 4 bits should be zero
			{
				delete [] decodedData;
				delete [] rawInputData;
				return false;
			}

            decodedData[ outputIndex++ ] = set1stOctet(b1, b2);
        }
        else if (!isPad( d3 ) && isPad( d4 ))
        {
            // one PAD e.g. 3cQ=
            b3 = base64Inverse[ d3 ];
            if (( b3 & 0x3 ) != 0 ) // last 2 bits should be zero
			{
				delete [] decodedData;
				delete [] rawInputData;
				return false;
			}

            decodedData[ outputIndex++ ] = set1stOctet( b1, b2 );
            decodedData[ outputIndex++ ] = set2ndOctet( b2, b3 );
        }
        else
        {
            // an error like "3c[Pad]r", "3cdX", "3cXd", "3cXX" where X is non data
			delete [] decodedData;
			delete [] rawInputData;
			return false;
		}
    }
    else
    {
        // no PAD e.g 3cQl
        b3 = base64Inverse[ d3 ];
        b4 = base64Inverse[ d4 ];
        decodedData[ outputIndex++ ] = set1stOctet( b1, b2 );
        decodedData[ outputIndex++ ] = set2ndOctet( b2, b3 );
        decodedData[ outputIndex++ ] = set3rdOctet( b3, b4 );
    }

	bool ok = outBlob.PutData(decodedData, outputIndex) == VE_OK;

	delete [] decodedData;
	delete [] rawInputData;
	return ok;
}

void Base64::init()
{
}

bool Base64::isData(const XMLByte& octet)
{
    return (base64Inverse[octet]!=(XMLByte)-1);
}

}
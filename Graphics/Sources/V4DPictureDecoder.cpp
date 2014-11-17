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
#include "VGifEncoder.h"
#include "ImageMeta.h"

#if VERSIONMAC
#import <ApplicationServices/ApplicationServices.h>
#elif VERSIONWIN
#include <wincodec.h>
#endif

BEGIN_TOOLBOX_NAMESPACE

const VString sCodec_none="????";
const VString sCodec_membitmap=".4DMemoryBitmap";
const VString sCodec_memvector=".4DMemoryVector";
const VString sCodec_4pct=".4pct";
const VString sCodec_4dmeta=".4DMetaPict";
const VString sCodec_png=".png";
const VString sCodec_bmp=".bmp";
const VString sCodec_jpg=".jpg";
const VString sCodec_tiff=".tiff";
const VString sCodec_gif=".gif";
const VString sCodec_emf=".emf";
const VString sCodec_pdf=".pdf";
const VString sCodec_pict=".pic";
const VString sCodec_svg=".svg";
const VString sCodec_blob=".4DBlob";

const VString sCodec_4DMeta_sign1="4MTA";
const VString sCodec_4DMeta_sign2="4DMetaPict";
const VString sCodec_4DMeta_sign3="4MTA";

END_TOOLBOX_NAMESPACE


VQTSpatialSettingsBag::VQTSpatialSettingsBag()
{
	fCodecType=0;
	fDepth=0;
	fSpatialQuality=0;
}

VQTSpatialSettingsBag::VQTSpatialSettingsBag(OsType inCodecType,sWORD depth,uLONG inSpatialQuality)
{
	fCodecType=inCodecType;
	fDepth=depth;
	fSpatialQuality=inSpatialQuality;
}
void VQTSpatialSettingsBag::Reset()
{
	fCodecType=0;
	fDepth=0;
	fSpatialQuality=0;
}

VError	VQTSpatialSettingsBag::LoadFromBag( const VValueBag& inBag)
{
	VError err;
	VString codec_type;
	sLONG depth;
	uLONG spatialQuality;
	
	if ( inBag.GetString( L"codec_type", codec_type)
		&& inBag.GetLong( "depth", depth)
		&& inBag.GetLong( "spatial_quality", spatialQuality) 
		&& (codec_type.GetLength() == 4)
		&& (depth >= -32768 && depth <= 32767)
		)
	{
		fCodecType = codec_type.GetOsType();
		fDepth = (short) depth;
		fSpatialQuality = (uLONG) spatialQuality;
	
		err = VE_OK;
	}
	else
	{
		err = VE_INVALID_PARAMETER;
	}
	return err;
}
VError	VQTSpatialSettingsBag::SaveToBag( VValueBag& ioBag, VString& outKind) const
{
	VString codec_type;
	codec_type.FromOsType( (OsType) fCodecType);
	ioBag.SetString( L"codec_type", codec_type);
	
	ioBag.SetLong( L"depth", fDepth);
	ioBag.SetLong( L"spatial_quality", fSpatialQuality);
	
	outKind = L"qt_spatial_settings";
	
	return VE_OK;
}
VQTSpatialSettingsBag::~VQTSpatialSettingsBag()
{

}

void VQTSpatialSettingsBag::SetCodecType(OsType inCodecType)
{
	fCodecType = inCodecType;
}

void VQTSpatialSettingsBag::SetDepth(sWORD inDepth)
{
	fDepth = inDepth;
}
void VQTSpatialSettingsBag::SetSpatialQuality(uLONG inSpatialQuality)
{
	fSpatialQuality = inSpatialQuality;
}

OsType	VQTSpatialSettingsBag::GetCodecType() const
{
	return fCodecType;
}

sWORD VQTSpatialSettingsBag::GetDepth() const
{
	return fDepth;
}
OsType VQTSpatialSettingsBag::GetSpatialQuality() const
{
	return  fSpatialQuality;
}


/********************************************************************/
// temp picturedataproovider
/********************************************************************/
class XTOOLBOX_API xTempPictureDataProvider_VPtr : public VPictureDataProvider_VPtr
{
	public:
	xTempPictureDataProvider_VPtr(void* inDataPtr,VSize inNbBytes,VIndex inOffset=0)
	:VPictureDataProvider_VPtr(kVPtr)
	{
		_InitFromVPtr((VPtr)inDataPtr,false,inNbBytes,inOffset);
	}
	~xTempPictureDataProvider_VPtr()
	{
	}
};

/********************************************************************/
// decoder info
/********************************************************************/

VPictureCodecFactory* VPictureCodecFactory::sDecoderFactory = NULL;
VPictureCodecFactory::VPictureCodecFactory()
{
	
	_AppendRetained_BuiltIn(new VPictureCodec_InMemBitmap());
	_AppendRetained_BuiltIn(new VPictureCodec_InMemVector());
	
	_AppendRetained_BuiltIn(new VPictureCodec_VPicture());
	_AppendRetained_BuiltIn(new VPictureCodec_4DMeta());
	_AppendRetained_BuiltIn(new VPictureCodec_JPEG());
	_AppendRetained_BuiltIn(new VPictureCodec_PNG());
	_AppendRetained_BuiltIn(new VPictureCodec_BMP());
	_AppendRetained_BuiltIn(new VPictureCodec_GIF());
	_AppendRetained_BuiltIn(new VPictureCodec_TIFF());
	_AppendRetained_BuiltIn(new VPictureCodec_EMF());
	_AppendRetained_BuiltIn(new VPictureCodec_PICT());
	_AppendRetained_BuiltIn(new VPictureCodec_PDF());

	_AppendRetained_BuiltIn(new VPictureCodec_4DVarBlob());
	
	_AppendRetained_Dyn(new VPictureCodec_ITKBlobPict(".ITKBlobPict"));
	_AppendRetained_Dyn(new VPictureCodec_UnknownData(sCodec_blob));

#if VERSIONMAC
	_AppendImageIOCodecs();	
#elif VERSIONWIN
	_AppendWICCodecs();
#endif
	
	_AppendRetained_BuiltIn(new VPictureCodec_FakeSVG());

	
	_UpdateDisplayNameWithSystemInformation();
}

VPictureCodecFactory::~VPictureCodecFactory()
{
	VSize i=0;
	
	sDecoderFactory=NULL;

	for(i=0;i<fBuiltInCodec.size();i++)
	{
		fBuiltInCodec[i]->Release();
	}
	fBuiltInCodec.clear();
	
	for(i=0;i<fPlatformCodec.size();i++)
	{
		fPlatformCodec[i]->Release();
	}
	fPlatformCodec.clear();
	
	for(i=0;i<fQTCodec.size();i++)
	{
		fQTCodec[i]->Release();
	}
	fQTCodec.clear();
	
	for(i=0;i<fDynamicCodec.size();i++)
	{
		fDynamicCodec[i]->Release();
	}
	fDynamicCodec.clear();
}
void VPictureCodecFactory::_UpdateDisplayNameWithSystemInformation()
{
	VSize i=0;

	for(i=0;i<fBuiltInCodec.size();i++)
	{
		fBuiltInCodec[i]->UpdateDisplayNameWithSystemInformation();
	}
	for(i=0;i<fPlatformCodec.size();i++)
	{
		fPlatformCodec[i]->UpdateDisplayNameWithSystemInformation();
	}
	for(i=0;i<fQTCodec.size();i++)
	{
		fQTCodec[i]->UpdateDisplayNameWithSystemInformation();
	}
	
	for(i=0;i<fDynamicCodec.size();i++)
	{
		fDynamicCodec[i]->UpdateDisplayNameWithSystemInformation();
	}
}
void VPictureCodecFactory::AutoWash(bool inForce)
{
	for(VSize i=0;i<fBuiltInCodec.size();i++)
	{
		fBuiltInCodec[i]->AutoWash();
	}
	// pas d'autowash pour les codec qt & dyn
}
void VPictureCodecFactory::RegisterCodec(VPictureCodec* inCodec)
{
	// pas thread safe !!!!!
	if(inCodec)
	{
		inCodec->Retain();
		if(inCodec->CheckIdentifier(sCodec_svg))
		{
			const VPictureCodec* fakesvg = RetainDecoderByIdentifier(sCodec_svg);
			if(fakesvg)
			{
				fakesvg->Release();
				VectorOfPictureCodec_Iterator it = std::find(fBuiltInCodec.begin(), fBuiltInCodec.end(), fakesvg);
				if(it!=fBuiltInCodec.end())
				{
					fBuiltInCodec.erase(it);
					fakesvg->Release();
				}
				
			}
		}
		_AppendRetained_BuiltIn(inCodec);
		inCodec->UpdateDisplayNameWithSystemInformation();
	}
}

const VPictureCodec* VPictureCodecFactory::RegisterAndRetainUnknownDataCodec(const VString& inIdentifier)
{
	if(!inIdentifier.IsEmpty())
	{
		VectorOfVString signArray;
		inIdentifier.GetSubStrings( ";",signArray);

		const VPictureCodec* codec;

		for(VectorOfVString::iterator it=signArray.begin();it!=signArray.end();it++)
		{
			codec=RetainDecoderByIdentifier((*it));
			if(codec)
				break;
		}
		if(!codec)
		{
			VTaskLock	accessLocker(&fLock);

			VPictureCodec* _codec=new VPictureCodec_UnknownData(inIdentifier);
			_AppendRetained_Dyn(_codec);
			_codec->Retain();
			_codec->UpdateDisplayNameWithSystemInformation();
			codec=_codec;
		}
		return codec;
	}
	else
		return NULL;
}

sLONG VPictureCodecFactory::CountEncoder()const
{
	sLONG result=0;
	VSize i;
	const VPictureCodec* codec;

	VTaskLock	accessLocker(&fLock);

	for(i=0;i<fBuiltInCodec.size();i++)
	{
		codec=fBuiltInCodec[i];
		if(codec->IsEncoder() && !codec->IsPrivateCodec())
			result++;
	}
	for(i=0;i<fPlatformCodec.size();i++)
	{
		codec=fPlatformCodec[i];
		if(codec->IsEncoder() && !codec->IsPrivateCodec())
			result++;
	}
	for(i=0;i<fQTCodec.size();i++)
	{
		codec=fQTCodec[i];
		if(codec->IsEncoder() && !codec->IsPrivateCodec())
			result++;
	}
	for(i=0;i<fDynamicCodec.size();i++)
	{
		codec=fDynamicCodec[i];
		if(codec->IsEncoder() && !codec->IsPrivateCodec())
			result++;
	}
	
	return result;
}
sLONG VPictureCodecFactory::CountDecoder()const
{
	const VPictureCodec* codec;
	sLONG result=0;
	VSize i;

	VTaskLock	accessLocker(&fLock);

	for(i=0;i<fBuiltInCodec.size();i++)
	{
		codec=fBuiltInCodec[i];
		if(!codec->IsPrivateCodec())
			result++;
	}
	for(i=0;i<fPlatformCodec.size();i++)
	{
		codec=fPlatformCodec[i];
		if(!codec->IsPrivateCodec())
			result++;
	}
	for(i=0;i<fQTCodec.size();i++)
	{
		codec=fQTCodec[i];
		if(!codec->IsPrivateCodec())
			result++;
	}
	for(i=0;i<fDynamicCodec.size();i++)
	{
		codec=fDynamicCodec[i];
		if(!codec->IsPrivateCodec())
			result++;
	}
	
	return result;
}

void VPictureCodecFactory::GetCodecList(VectorOfVPictureCodecConstRef &outList,bool inEncoderOnly,bool inIncludePrivate)
{
	VSize i;
	VTaskLock	accessLocker(&fLock);
	
	outList.clear();

	for(i=0;i<fBuiltInCodec.size();i++)
	{
		if(!fBuiltInCodec[i]->IsPrivateCodec() || inIncludePrivate)
		{
			if((inEncoderOnly && fBuiltInCodec[i]->IsEncoder()) || !inEncoderOnly)
				outList.push_back(VPictureCodecConstRef(fBuiltInCodec[i]));
		}
	}
	for(i=0;i<fPlatformCodec.size();i++)
	{
		if(!fPlatformCodec[i]->IsPrivateCodec() || inIncludePrivate)
		{
			if((inEncoderOnly && fPlatformCodec[i]->IsEncoder()) || !inEncoderOnly)
				outList.push_back(VPictureCodecConstRef(fPlatformCodec[i]));
		}
	}
	for(i=0;i<fQTCodec.size();i++)
	{
		if(!fQTCodec[i]->IsPrivateCodec() || inIncludePrivate)
		{
			if((inEncoderOnly && fQTCodec[i]->IsEncoder()) || !inEncoderOnly)
				outList.push_back(VPictureCodecConstRef(fQTCodec[i]));
		}
	}
	for(i=0;i<fDynamicCodec.size();i++)
	{
		if(!fDynamicCodec[i]->IsPrivateCodec() || inIncludePrivate)
		{
			if((inEncoderOnly && fDynamicCodec[i]->IsEncoder()) || !inEncoderOnly)
				outList.push_back(VPictureCodecConstRef(fDynamicCodec[i]));
		}
	}
}

void VPictureCodecFactory::_AppendRetained_BuiltIn(VPictureCodec* inInfo)
{
	fBuiltInCodec.push_back(inInfo);
}

void VPictureCodecFactory::_AppendRetained_Dyn(VPictureCodec* inInfo)
{
	VTaskLock	accessLocker(&fLock);
	fDynamicCodec.push_back(inInfo);
}

VError VPictureCodecFactory::Encode(const VPicture& inPict,const VFileKind& inKind,VFile& inFile,const VValueBag* inEncoderParameter)
{
	VError result=0;
	const VPictureCodec* encoder=RetainDecoderForVFileKind(inKind);
	if(encoder)
	{
		result=encoder->Encode(inPict,inEncoderParameter,inFile);
		encoder->Release();
	}
	return result;
}
VError VPictureCodecFactory::Encode(const VPictureData& inPict,const VFileKind& inKind,VFile& inFile,const VValueBag* inEncoderParameter)
{
	VError result=0;
	const VPictureCodec* encoder=RetainEncoderForVFileKind(inKind);
	if(encoder)
	{
		inPict.Retain();
		result=encoder->Encode(inPict,inEncoderParameter,inFile,NULL);
		inPict.Release();
		encoder->Release();
	}
	return result;
}

VError VPictureCodecFactory::Encode(const VPicture& inPict,VFile& inFile,const VValueBag* inEncoderParameter)
{
	XBOX::VString ext;
	VError result=0;

	inFile.GetExtension(ext);
	const VPictureCodec* encoder=RetainEncoderForExtension(ext);
	if(encoder)
	{
		result=encoder->Encode(inPict,inEncoderParameter,inFile);
		encoder->Release();
	}
	else
		result=VE_FILE_BAD_KIND;

	return result;
}
VError VPictureCodecFactory::Encode(const VPictureData& inPict,VFile& inFile,const VValueBag* inEncoderParameter)
{
	VError result=0;
	XBOX::VString ext;
	
	inFile.GetExtension(ext);
	const VPictureCodec* encoder=RetainEncoderForExtension(ext);
	if(encoder)
	{
		result=encoder->Encode(inPict,inEncoderParameter,inFile);
		encoder->Release();
	}
	else
		result=VE_INVALID_PARAMETER;

	return result;
}

VError VPictureCodecFactory::EncodeToBlob(const VPicture& inPict,const VString& inID,VBlob& inBlob,const VValueBag* inEncoderParameter)
{
	VError result=VE_INVALID_PARAMETER;
	const VPictureCodec* encoder=RetainEncoderByIdentifier(inID);
	if(encoder)
	{
		result=encoder->Encode(inPict,inEncoderParameter,inBlob);
		encoder->Release();
	}
	return result;
}
VError VPictureCodecFactory::EncodeToBlob(const VPictureData& inPict,const VString& inID,VBlob& inBlob,const VValueBag* inEncoderParameter)
{
	VError result=VE_INVALID_PARAMETER;
	const VPictureCodec* encoder=RetainEncoderByIdentifier(inID);
	if(encoder)
	{ 
		inPict.Retain();
		result = encoder->Encode(inPict,inEncoderParameter,inBlob,NULL);
		inPict.Release();
		encoder->Release();
	}
	return result;
}
VPictureData* VPictureCodecFactory::Encode(const VPicture& inPict,const VString& inID,const VValueBag* inEncoderParameter)
{
	VError err;
	VPictureData* result=0;
	VBlobWithPtr* blob=new VBlobWithPtr();
	err=EncodeToBlob(inPict,inID,*blob,inEncoderParameter);
	if(err==VE_OK && blob->GetSize()>0)
	{
		const VPictureCodec* encoder=RetainEncoderByIdentifier(inID);
		if(encoder)
		{
			VPictureDataProvider* dataprovider=VPictureDataProvider::Create(blob,true);
			if(dataprovider)
			{
				result=encoder->CreatePictDataFromDataProvider(*dataprovider);
				dataprovider->Release();
			}
			encoder->Release();
		}
	}
	return result;
}
VError VPictureCodecFactory::Encode(const VPicture& inPict,const VString& inID,VPicture& outPict,const VValueBag* inEncoderParameter)
{
	bool done=false;
	VError err=VE_OK;
	const VPictureData* data=inPict.RetainPictDataByIdentifier(inID);
	if(data)
	{
		const VPictureCodec* codec = RetainDecoderByIdentifier(inID);
		if(codec)
		{
			if(!codec->NeedReEncode(*data,inEncoderParameter,NULL))
			{
				outPict.SetDrawingSettings(inPict.GetSettings());
				outPict.AppendPictData(data,true,true);
				done=true;
			}
			codec->Release();
		}
		data->Release();
	}
	if(!done)
	{
		// on a pas trouver le type demander, on le cree sans transformation
		VPicture tmpPict;
		tmpPict.FromVPicture_Retain(inPict,false);
		tmpPict.ResetMatrix();
		
		data=Encode(tmpPict,inID,inEncoderParameter);
		if(data)
		{
			outPict.SetDrawingSettings(inPict.GetSettings());
			outPict.AppendPictData(data,true,true);
			data->Release();
		}
		else
			err=VE_INVALID_PARAMETER;
	}
	return err;
}

VPictureData* VPictureCodecFactory::Encode(const VPictureData& inPict,const VString& inID,const VValueBag* inEncoderParameter)
{
	VError err;
	VPictureData* result=0;
	VBlobWithPtr *blob=new VBlobWithPtr();
	inPict.Retain();
	err=EncodeToBlob(inPict,inID,*blob,inEncoderParameter);
	if(err==VE_OK && blob->GetSize()>0)
	{
		const VPictureCodec* encoder=RetainEncoderByIdentifier(inID);
		if(encoder)
		{
			VPictureDataProvider* dataprovider=VPictureDataProvider::Create(blob,true);
			if(dataprovider)
			{
				result=encoder->CreatePictDataFromDataProvider(*dataprovider);
				dataprovider->Release();
			}
			encoder->Release();
		}
	}
	inPict.Release();
	return result;
}


bool VPictureCodecFactory::IsQuicktimeCodec(const VPictureCodec* inCodec)
{
	return std::find( fQTCodec.begin(), fQTCodec.end(), inCodec) != fQTCodec.end();
}

void VPictureCodecFactory::RegisterQuicktimeDecoder(VPictureCodec* inCodec)
{
	if(inCodec!=NULL)
	{
		bool extensionfound=false;
		if(inCodec->CountExtensions()>0)
		{
			for(long nbext=0;nbext<inCodec->CountExtensions();nbext++)
			{
				VString ext;
				inCodec->GetNthExtension(nbext,ext);
				extensionfound=_ExtensionExist(ext);
				if(extensionfound)
					break;
			}
			if(!extensionfound)
			{
				inCodec->Retain();
				fQTCodec.push_back(inCodec);
				inCodec->UpdateDisplayNameWithSystemInformation();
			}
		}	
	}
}

void VPictureCodecFactory::UnRegisterQuicktimeDecoder()
{
	for(sLONG i=0;i<fQTCodec.size();i++)
	{
		fQTCodec[i]->Release();
	}
	fQTCodec.clear();
}

#if VERSIONMAC
/** append ImageIO codecs */
void VPictureCodecFactory::_AppendImageIOCodecs()
{
	CFArrayRef decoderUTIs = CGImageSourceCopyTypeIdentifiers();
#if VERSIONDEBUG
	CFShow( decoderUTIs);
#endif
	CFArrayRef encoderUTIs = CGImageDestinationCopyTypeIdentifiers();
#if VERSIONDEBUG
	CFShow( encoderUTIs);
#endif
	sLONG count = CFArrayGetCount( decoderUTIs);
	for (int i = 0; i < count; i++)
	{
		CFStringRef uti = (CFStringRef)CFArrayGetValueAtIndex( decoderUTIs, i);
		
		//check if ImageIO can encode with codec
		bool canEncode = false;
		sLONG countEncode = CFArrayGetCount( encoderUTIs);
		for (int j = 0; j < countEncode; j++)
		{
			CFStringRef utiEncode = (CFStringRef)CFArrayGetValueAtIndex( encoderUTIs, j);
			if (CFStringCompare(uti, utiEncode, 0) == kCFCompareEqualTo)
			{
				canEncode = true;
				break;
			}
		}
		
		//extract ImageIO codec information
		VString xuti;
		xuti.MAC_FromCFString(uti);
		VImageIOCodecInfo info( xuti, canEncode);
		
		//check if a existing codec uses same extension 
		bool extensionFound = false;
		for(long numExt = 0; numExt < info.CountExtension(); numExt++)
		{
			VString ext;
			info.GetNthExtension( numExt, ext);
			extensionFound = _ExtensionExist( ext);
			if (extensionFound)
				break;
		}
		if(!extensionFound && info.CountExtension())
			//append ImageIO codec to the list of codecs
			fPlatformCodec.push_back(static_cast<VPictureCodec *>(new VPictureCodec_ImageIO(info)));
	}
	CFRelease(encoderUTIs);
	CFRelease(decoderUTIs);
	
	ImageMeta::stReader::InitializeGlobals();
}
#elif VERSIONWIN
/** append WIC codecs */
void VPictureCodecFactory::_AppendWICCodecs()
{
	VPictureCodec_WIC::AppendWICCodecs( fPlatformCodec, this);
}
#endif


bool VPictureCodecFactory::_ExtensionExist_Dyn(const VString& inExt)
{
	VTaskLock	accessLocker(&fLock);
	for(VSize i=0;i<fDynamicCodec.size();i++)
	{
		if(fDynamicCodec[i]->CheckExtension(inExt))
			return true;
	}
	return false;
}
bool VPictureCodecFactory::_MimeTypeExist_Dyn(const VString& inMime)
{
	VTaskLock	accessLocker(&fLock);
	for(VSize i=0;i<fDynamicCodec.size();i++)
	{
		if(fDynamicCodec[i]->CheckMimeType(inMime))
			return true;
	}
	return false;
}

bool VPictureCodecFactory::_ExtensionExist(const VString& inExt)
{
	bool res=false;
	VSize i=0;
	
	for(i=0;i<fBuiltInCodec.size();i++)
	{
		if(fBuiltInCodec[i]->CheckExtension(inExt))
			return true;
	}
	for(i=0;i<fPlatformCodec.size();i++)
	{
		if(fPlatformCodec[i]->CheckExtension(inExt))
			return true;
	}
	for(i=0;i<fQTCodec.size();i++)
	{
		if(fBuiltInCodec[i]->CheckExtension(inExt))
			return true;
	}

	return _ExtensionExist_Dyn(inExt);
}
bool VPictureCodecFactory::_MimeTypeExist(const VString& inMime)
{
	VSize i=0;
	
	for(i=0;i<fBuiltInCodec.size();i++)
	{
		if(fBuiltInCodec[i]->CheckMimeType(inMime))
			return true;
	}
	for(i=0;i<fPlatformCodec.size();i++)
	{
		if(fPlatformCodec[i]->CheckMimeType(inMime))
			return true;
	}
	for(i=0;i<fQTCodec.size();i++)
	{
		if(fQTCodec[i]->CheckMimeType(inMime))
			return true;
	}

	return _MimeTypeExist_Dyn(inMime);
}

bool VPictureCodecFactory::CheckData(VFile& inFile)
{
	bool result=false;
	
	const VPictureCodec* inf=RetainDecoder(inFile,true);
	if(inf)
	{
		inf->Release();
		result=true;
	}
	return result;
}
const VPictureCodec* VPictureCodecFactory::RetainEncoderForVFileKind(const VFileKind& inKind)
{
	const VPictureCodec* enco=RetainDecoderForVFileKind(inKind);
	if(enco && !enco->IsEncoder())
	{
		enco->Release();
		enco=0;
	}
	return enco;
}
const VPictureCodec* VPictureCodecFactory::RetainEncoderForMacType(const sLONG inMacType)
{
	const VPictureCodec* enco=RetainDecoderForMacType(inMacType);
	if(enco && !enco->IsEncoder())
	{
		enco->Release();
		enco=0;
	}
	return enco;
}
const VPictureCodec* VPictureCodecFactory::RetainEncoderForMimeType(const VString& inMime)
{
	const VPictureCodec* enco=RetainDecoderForMimeType(inMime);
	if(enco && !enco->IsEncoder())
	{
		enco->Release();
		enco=0;
	}
	return enco;
}
const VPictureCodec* VPictureCodecFactory::RetainEncoderForExtension(const VString& inExt)
{
	const VPictureCodec* enco=RetainDecoderForExtension(inExt);
	if(enco && !enco->IsEncoder())
	{
		enco->Release();
		enco=0;
	}
	return enco;
}
const VPictureCodec* VPictureCodecFactory::RetainEncoderByIdentifier(const VString& inExt)
{
	const VPictureCodec* enco=RetainDecoderByIdentifier(inExt);
	if(enco && !enco->IsEncoder())
	{
		enco->Release();
		enco=0;
	}
	return enco;
}
const VPictureCodec* VPictureCodecFactory::RetainEncoderForQTExporterType(const sLONG inQTType)
{
	const VPictureCodec* enco=RetainDecoderForQTExporterType(inQTType);
	if(enco && !enco->IsEncoder())
	{
		enco->Release();
		enco=0;
	}
	return enco;
}

const VPictureCodec* VPictureCodecFactory::RetainEncoderForUTI(const VString& inExt)
{
	const VPictureCodec* enco=RetainDecoderForUTI(inExt);
	if(enco && !enco->IsEncoder())
	{
		enco->Release();
		enco=0;
	}
	return enco;
}

#if VERSIONWIN
const VPictureCodec* VPictureCodecFactory::RetainEncoderForGUID(const GUID& guid)
{
	const VPictureCodec* enco=RetainDecoderForGUID(guid);
	if(enco && !enco->IsEncoder())
	{
		enco->Release();
		enco=0;
	}
	return enco;
}
#endif

bool VPictureCodecFactory::CheckData(VPictureDataProvider& inDataProvider)
{
	bool result=false;
	const VPictureCodec* inf=RetainDecoder(inDataProvider);
	if(inf)
	{
		inf->Release();
		result=true;
	}
	return result;
}

const VPictureCodec* VPictureCodecFactory::RetainDecoderForVFileKind(const VFileKind& inKind)
{
	VString ext;
	if(inKind.GetPreferredExtension(ext))
	{
		return RetainDecoderForExtension(ext);
	}
	return 0;
}

const VPictureCodec* VPictureCodecFactory::RetainDecoderForQTExporterType(const sLONG inQTType)
{
	const VPictureCodec *result=0;
	
	result=_RetainDecoderForQTExporterType(fBuiltInCodec,inQTType);
	if(!result)
	{
		result=_RetainDecoderForQTExporterType(fPlatformCodec,inQTType);
	}
	if(!result)
	{
		result=_RetainDecoderForQTExporterType(fQTCodec,inQTType);
	}
	return result;
}

const VPictureCodec* VPictureCodecFactory::RetainDecoderForMimeType(const VString& inMime)
{
	const VPictureCodec *result=0;
	
	result=_RetainDecoderForMimeType(fBuiltInCodec,inMime);
	if(!result)
	{
		result=_RetainDecoderForMimeType(fPlatformCodec,inMime);
		if (!result)
			result=_RetainDecoderForMimeType(fQTCodec,inMime);
		if(!result)
		{
			VTaskLock	accessLocker(&fLock);
			result=_RetainDecoderForMimeType(fDynamicCodec,inMime);
		}
	}
	return result;
}
const VPictureCodec* VPictureCodecFactory::RetainDecoderForExtension(const VString& inExt)
{
	const VPictureCodec *result=0;
	
	result = _RetainDecoderForExtension(fBuiltInCodec,inExt);
	if(!result)
	{
		result = _RetainDecoderForExtension(fPlatformCodec,inExt);
		if(!result)
			result=_RetainDecoderForExtension(fQTCodec,inExt);
		if(!result)
		{
			VTaskLock	accessLocker(&fLock);
			result=_RetainDecoderForExtension(fDynamicCodec,inExt);
		}
	}
	return result;
}
const VPictureCodec* VPictureCodecFactory::RetainDecoderForUTI(const VString& inUTI)
{
	const VPictureCodec *result=0;
	
	result = _RetainDecoderForUTI(fBuiltInCodec,inUTI);
	if(!result)
	{
		result = _RetainDecoderForUTI(fPlatformCodec,inUTI);
		if(!result)
		{
			VTaskLock	accessLocker(&fLock);
			result=_RetainDecoderForUTI(fDynamicCodec,inUTI);
		}
	}
	return result;
}

#if VERSIONWIN
const VPictureCodec* VPictureCodecFactory::RetainDecoderForGUID(const GUID& guid)
{
	const VPictureCodec *result=0;
	
	result = _RetainDecoderForGUID(fBuiltInCodec,guid);
	if(!result)
	{
		result = _RetainDecoderForGUID(fPlatformCodec,guid);
		if(!result)
		{
			VTaskLock	accessLocker(&fLock);
			result=_RetainDecoderForGUID(fDynamicCodec,guid);
		}
	}
	return result;
}
#endif

const VPictureCodec* VPictureCodecFactory::_RetainDecoderForMacType(const VectorOfPictureCodec& inList,const sLONG inMacType)
{
	const VPictureCodec *codec,*result=0;
	VSize i;
	
	for(i=0;i<inList.size();i++)
	{
		codec=inList[i];
		if(codec->CheckMacType(inMacType))
		{
			codec->Retain();
			result=codec;
			break;
		}
	}
	return result;
}

const VPictureCodec*  VPictureCodecFactory::_RetainDecoderForMimeType(const VectorOfPictureCodec& inList,const VString& inMime)
{
	const VPictureCodec *codec,*result=0;
	VSize i;
	
	for(i=0;i<inList.size();i++)
	{
		codec=inList[i];
		if(codec->CheckMimeType(inMime))
		{
			codec->Retain();
			result=codec;
			break;
		}
	}
	return result;
}
const VPictureCodec*  VPictureCodecFactory::_RetainDecoderForExtension(const VectorOfPictureCodec& inList,const VString& inExt)
{
	const VPictureCodec *codec,*result=0;
	VSize i;
	
	for(i=0;i<inList.size();i++)
	{
		codec=inList[i];
		if(codec->CheckExtension(inExt))
		{
			codec->Retain();
			result=codec;
			break;
		}
	}
	return result;
}
const VPictureCodec*  VPictureCodecFactory::_RetainDecoderForQTExporterType(const VectorOfPictureCodec& inList,const sLONG inQTType)
{
	const VPictureCodec *codec,*result=0;
	VSize i;
	
	for(i=0;i<inList.size();i++)
	{
		codec=inList[i];
		if(codec->CheckQTExporterComponentType(inQTType))
		{
			codec->Retain();
			result=codec;
			break;
		}
	}
	return result;
}

const VPictureCodec*  VPictureCodecFactory::_RetainDecoderForUTI(const VectorOfPictureCodec& inList,const VString& inUTI)
{
	const VPictureCodec *codec,*result=0;
	VSize i;
	
	for(i=0;i<inList.size();i++)
	{
		codec=inList[i];
		if(codec->CheckUTI(inUTI))
		{
			codec->Retain();
			result=codec;
			break;
		}
	}
	return result;
}

#if VERSIONWIN
const VPictureCodec*  VPictureCodecFactory::_RetainDecoderForGUID(const VectorOfPictureCodec& inList,const GUID& guid)
{
	const VPictureCodec *codec,*result=0;
	VSize i;
	
	for(i=0;i<inList.size();i++)
	{
		codec=inList[i];
		if(codec->GetGUID() == guid)
		{
			codec->Retain();
			result=codec;
			break;
		}
	}
	return result;
}
#endif

const VPictureCodec* VPictureCodecFactory::RetainDecoderForMacType(const sLONG inMacType)
{
	const VPictureCodec *result=0;

	result=_RetainDecoderForMacType(fBuiltInCodec,inMacType);
	if(!result)
	{
		result=_RetainDecoderForMacType(fPlatformCodec,inMacType);
		if(!result)
			result=_RetainDecoderForMacType(fQTCodec,inMacType);
		if(!result)
		{
			VTaskLock	accessLocker(&fLock);
			result=_RetainDecoderForMacType(fDynamicCodec,inMacType);
		}
	}
	return result;
}

const VPictureCodec* VPictureCodecFactory::RetainDecoderByIdentifier(const VString& inExt)
{
	const VPictureCodec *info=0,*result=0;
	
	if (inExt.IsEmpty())
		return info;
		
	const UniChar *p = inExt.GetCPointer();
	if(*p == '.')
	{
		result=RetainDecoderForExtension(inExt);
	}
	else
	{
		if(inExt.GetLength()==4)
		{
			result=RetainDecoderForMacType(inExt.GetOsType());
			if(!result)
			{
				result=RetainEncoderForQTExporterType(inExt.GetOsType());
			}
		}
		else
		{
			result=RetainDecoderForMimeType(inExt);
			if (!result)
				result = RetainDecoderForUTI(inExt);
		}
	}
	return result;
}

const VPictureCodec* VPictureCodecFactory::RetainDecoder( const VFile& inFile,bool inCheckData)
{
	const VPictureCodec* codec=NULL;
	if(inFile.Exists())
	{
		VString ext;
		inFile.GetExtension(ext);
		
		if(!ext.IsEmpty())
		{
			codec=RetainDecoderForExtension(ext);
			if(codec)
			{
				if(inCheckData)
				{
					if(!codec->CanValidateData())
					{
						// le codec ne sais pas valider les donnes, on assume que le contenue du fichier est correct
					}
					else
					{
						if(!codec->ValidateData(const_cast<VFile&>( inFile)))
						{
							// le contenue du fichier ne correspond pas au codec, on cherche un codec qui valide le data
							codec->Release();
							codec=NULL;
						}
					}
				}
			}
		}
		if(!codec && inCheckData)
		{
			for(VSize i=0;i<fBuiltInCodec.size();i++)
			{
				if(fBuiltInCodec[i]->ValidateData(const_cast<VFile&>( inFile)))
				{
					codec=fBuiltInCodec[i];
					codec->Retain();
					break;
				}
			}
			if(!codec)
			{
				for(VSize i=0;i<fPlatformCodec.size();i++)
				{
					if(fPlatformCodec[i]->ValidateData(const_cast<VFile&>( inFile)))
					{
						codec=fPlatformCodec[i];
						codec->Retain();
						break;
					}
				}
				if(!codec)
				{
					for(VSize i=0;i<fQTCodec.size();i++)
					{
						if(fQTCodec[i]->ValidateData(const_cast<VFile&>( inFile)))
						{
							codec=fQTCodec[i];
							codec->Retain();
							break;
						}
					}
					if(!codec)
					{
						VTaskLock	accessLocker(&fLock);
						for(VSize i=0;i<fDynamicCodec.size();i++)
						{
							if(fDynamicCodec[i]->ValidateData(const_cast<VFile&>( inFile)))
							{
								codec=fDynamicCodec[i];
								codec->Retain();
								break;
							}
						}
					}
				}
			}
		}
	}
	return codec;
}

const VPictureCodec* VPictureCodecFactory::RetainDecoder(void* inData,VSize inDataSize)
{
	xTempPictureDataProvider_VPtr ds(inData,inDataSize);
	return RetainDecoder(ds);
}

const VPictureCodec* VPictureCodecFactory::RetainDecoder(VPictureDataProvider& inDataProvider)
{
	VSize i;
	const VPictureCodec* codec=0;
	
	for(i=0;i<fBuiltInCodec.size();i++)
	{
		if(fBuiltInCodec[i]->ValidateData(inDataProvider))
		{
			codec=fBuiltInCodec[i];
			codec->Retain();
			break;
		}
	}
	if(!codec)
	{
		for(i=0;i<fPlatformCodec.size();i++)
		{
			if(fPlatformCodec[i]->ValidateData(inDataProvider))
			{
				codec=fPlatformCodec[i];
				codec->Retain();
				break;
			}
		}
		if(!codec)
		{
			for(i=0;i<fQTCodec.size();i++)
			{
				if(fQTCodec[i]->ValidateData(inDataProvider))
				{
					codec=fQTCodec[i];
					codec->Retain();
					break;
				}
			}
			if(!codec)
			{
				VTaskLock	accessLocker(&fLock);
				for(i=0;i<fDynamicCodec.size();i++)
				{
					if(fDynamicCodec[i]->ValidateData(inDataProvider))
					{
						codec=fDynamicCodec[i];
						codec->Retain();
						break;
					}
				}
			}
		}
	}
	return codec;
}

void VPictureCodecFactory::BuildPutFileFilter(VectorOfVFileKind& outList,const VPictureCodec* inForThisCodec)
{
	VectorOfVFileKind list;
	VPictureCodecConstRef codecref;
	VectorOfVPictureCodecConstRef codeclist;
	outList.clear();
	
	if(!inForThisCodec)
	{
		GetCodecList(codeclist);
		
		for(VSize i=0;i<codeclist.size();i++)
		{
			codecref=codeclist[i];
			
			if(codecref->IsCross() || (codecref->IsWin() && VERSIONWIN) || (codecref->IsMac() && VERSIONMAC))
			{
				if(!codecref->IsPrivateCodec() && codecref->IsEncoder()	)
				{
					list.clear();
					codecref->RetainFileKind(list);
					if (!list.empty())
					{
						for(sLONG ii = 0;ii<list.size();ii++)
						{
							outList.push_back( list[ii]);
						}
					}
				}
			}
		}
	}
	else
	{
		list.clear();
		inForThisCodec->RetainFileKind(list);
		if (!list.empty())
		{
			for(sLONG ii = 0;ii<list.size();ii++)
			{
				outList.push_back( list[ii]);
			}
		}
	}
}

void VPictureCodecFactory::BuildGetFileFilter(VectorOfVFileKind& outList)
{
	VPictureCodecConstRef codecref;
	VectorOfVPictureCodecConstRef codeclist;
	VectorOfVFileKind list;
	
	outList.clear();

	GetCodecList(codeclist);

	for(VSize i=0;i<codeclist.size();i++)
	{
		codecref = codeclist[i];
		
		if(codecref->IsCross() || (codecref->IsWin() && VERSIONWIN) || (codecref->IsMac() && VERSIONMAC))
		{
			if(!codecref->IsPrivateCodec())
			{
				list.clear();
				codecref->RetainFileKind(list);
				if (!list.empty())
				{
					for(sLONG ii = 0;ii<list.size();ii++)
					{
						outList.push_back( list[ii]);
					}
				}
			}
		}
	}
}
VPictureData* VPictureCodecFactory::CreatePictureDataFromIdentifier(const VString& inID,VPictureDataProvider* inDataProvider,_VPictureAccumulator* inRecorder)
{
	VPictureData *result=0;
	const VPictureCodec* deco=RetainDecoderByIdentifier(inID);
	if(deco)
	{
		result=deco->CreatePictDataFromDataProvider(*inDataProvider,inRecorder);
		deco->Release();
	}
	return result;
}
VPictureData* VPictureCodecFactory::CreatePictureDataFromExtension(const VString& inExt,VPictureDataProvider* inDataProvider,_VPictureAccumulator* inRecorder)
{
	VPictureData *result=0;
	if(inDataProvider)
	{
		const VPictureCodec *codec=RetainDecoderForExtension(inExt);
		if(codec)
		{
			result = codec->CreatePictDataFromDataProvider(*inDataProvider,inRecorder);
			codec->Release();
		}
	}
	return result;
}
VPictureData* VPictureCodecFactory::CreatePictureData(VFile& inFile)
{
	VPictureData* result=0;
	const VPictureCodec* decoder= RetainDecoder(inFile,true);
	if(decoder)
	{
		result=decoder->CreatePictDataFromVFile(inFile);
		decoder->Release();
	}
	return result;
}
VPictureData* VPictureCodecFactory::CreatePictureData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder)
{
	VPictureData* result=0;
	const VPictureCodec* decoder= RetainDecoder(inDataProvider);
	if(decoder)
	{
		result=decoder->CreatePictDataFromDataProvider(inDataProvider,inRecorder);
		decoder->Release();
	}
	return result;
}
VPictureData* VPictureCodecFactory::CreatePictureDataFromMimeType(const VString& inMime,VPictureDataProvider* inDataProvider,_VPictureAccumulator* inRecorder)
{
	VPictureData *result=0;
	
	if(inDataProvider)
	{
		const VPictureCodec *codec=RetainDecoderForMimeType(inMime);
		if(codec)
		{
			result = codec->CreatePictDataFromDataProvider(*inDataProvider,inRecorder);
			codec->Release();
		}
		else
		{
			codec= RetainDecoder(*inDataProvider);
			if(codec)
			{
				result=codec->CreatePictDataFromDataProvider(*inDataProvider,inRecorder);
				codec->Release();
			}
		}
	}
	return result;
}


VPictureCodec::VPictureCodec()
{
	fCanEncode=0;
	fMaxSignLen=0;
	fDisplayName="";
	fPlatform=3;
	fIsBuiltIn=true;
	fPrivate=false;
	fCanValidateData=true;	
#if VERSIONWIN
	fGUID = GUID_NULL;
#endif

#if VERSIONMAC
	SetScrapKind(SCRAP_KIND_PICTURE_PICT,"");
#else
	SetScrapKind(SCRAP_KIND_PICTURE_BITMAP,"");
#endif
};
VPictureCodec::~VPictureCodec()
{
	for(long i=0;i<fSignaturePatterns.GetCount();i++)
	{
		if(fSignaturePatterns[i])
			delete[] fSignaturePatterns[i];
	}
}
void VPictureCodec::UpdateDisplayNameWithSystemInformation()
{
#if VERSIONWIN
	SHFILEINFOW info;
	XBOX::VString ext;
	for(long i=0;i<fFileExtensions.GetCount();i++)
	{
		ext="."+fFileExtensions[i];
		info.szTypeName[0]=0;
		::SHGetFileInfoW(ext.GetCPointer(),FILE_ATTRIBUTE_NORMAL,&info,sizeof(info),SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME);
		if(info.szTypeName[0])
		{ 
			SetDisplayName((UniChar*)info.szTypeName);
			break;
		}
	}
#endif
}
#if VERSIONMAC
CFStringRef VPictureCodec::MAC_FindPublicUTI( CFStringRef inTagClass, CFStringRef inTag) const
{
	CFStringRef uti = ::UTTypeCreatePreferredIdentifierForTag( inTagClass, inTag, CFSTR("public.data"));

	/*
		UTTypeCreatePreferredIdentifierForTag n'echoue jamais car si l'uti n'existe pas, il est cree dous la forme "dyn.xxx"
		donc on rejete les uti sous cette forme pour pouvoir tester les autres tags.
		
	*/
	if ( (uti != NULL) && ::CFStringHasPrefix( uti, CFSTR( "dyn.")) )
	{
		::CFRelease( uti);
		uti = NULL;
	}
	return uti;
}

#endif


VFileKind* VPictureCodec::RetainFileKind() const
{
	VFileKind *fileKind = NULL;

	VStr63 id;

	#if VERSIONMAC
	
	/*
		on essaie de retrouver l'UTI correspondant en utilisant par ordre de priorite : type, extension, mime type

	*/
	
	CFStringRef uti = NULL;
	VIndex count = fFileMacType.GetCount();
	for( VIndex i = 0 ; (i < count) && (uti == NULL) ; ++i)
	{
		CFStringRef tag = ::UTCreateStringForOSType( fFileMacType[i]);
		uti = MAC_FindPublicUTI( kUTTagClassOSType, tag);
		::CFRelease( tag);
	}

	count = fFileExtensions.GetCount();
	for( VIndex i = 0 ; (i < count) && (uti == NULL) ; ++i)
	{
		CFStringRef tag = fFileExtensions[i].MAC_RetainCFStringCopy();
		uti = MAC_FindPublicUTI( kUTTagClassFilenameExtension, tag);
		::CFRelease( tag);
	}

	count = fMimeTypes.GetCount();
	for( VIndex i = 0 ; (i < count) && (uti == NULL) ; ++i)
	{
		CFStringRef tag = fMimeTypes[i].MAC_RetainCFStringCopy();
		uti = MAC_FindPublicUTI( kUTTagClassMIMEType, tag);
		::CFRelease( tag);
	}
	
	if (uti != NULL)
	{
		// ok, we found a public UTI, let's retain it from manager
		id.MAC_FromCFString( uti);
		fileKind = VFileKind::RetainFileKind( id);
		::CFRelease( uti);
	}
	
	#endif

	if (fileKind == NULL)
	{
		// no public file kind is available, let's build a private one

		id = CVSTR( "com.4d.");
		if(!GetDisplayName().IsEmpty())
		{			
			id += GetDisplayName();
			id += ".";

		}
		id += GetDefaultIdentifier();

		// if an id is already registered, take it, else create a new one and register it
		fileKind = VFileKind::RetainFileKind( id);
		if (fileKind == NULL)
		{
			VectorOfFileExtension extensions;
			for(long i=0;i<fFileExtensions.GetCount();i++)
				extensions.push_back( fFileExtensions[i]);
			VFileKindManager::Get()->RegisterPrivateFileKind( id, GetDisplayName(), extensions, VectorOfFileKind(0));
			fileKind = VFileKind::RetainFileKind( id);
		}
	}

	return fileKind;
}

void VPictureCodec::RetainFileKind(std::vector< VRefPtr <VFileKind> > &outFileKind) const
{
	VFileKind *fileKind = NULL;
	
	VStr63 id;

	outFileKind.clear();

	#if VERSIONMAC
	
	/*
		on essaie de retrouver l'UTI correspondant en utilisant par ordre de priorite : type, extension, mime type

	*/
	
	CFStringRef uti = NULL;
	VIndex count = fFileMacType.GetCount();
	for( VIndex i = 0 ; (i < count) && (uti == NULL) ; ++i)
	{
		CFStringRef tag = ::UTCreateStringForOSType( fFileMacType[i]);
		uti = MAC_FindPublicUTI( kUTTagClassOSType, tag);
		::CFRelease( tag);
	}

	count = fFileExtensions.GetCount();
	for( VIndex i = 0 ; (i < count) && (uti == NULL) ; ++i)
	{
		CFStringRef tag = fFileExtensions[i].MAC_RetainCFStringCopy();
		uti = MAC_FindPublicUTI( kUTTagClassFilenameExtension, tag);
		::CFRelease( tag);
	}

	count = fMimeTypes.GetCount();
	for( VIndex i = 0 ; (i < count) && (uti == NULL) ; ++i)
	{
		CFStringRef tag = fMimeTypes[i].MAC_RetainCFStringCopy();
		uti = MAC_FindPublicUTI( kUTTagClassMIMEType, tag);
		::CFRelease( tag);
	}
	
	if (uti != NULL)
	{
		// ok, we found a public UTI, let's retain it from manager
		id.MAC_FromCFString( uti);
		fileKind = VFileKind::RetainFileKind( id);
		::CFRelease( uti);
	}
	
	#endif

	if (fileKind == NULL)
	{
		// no public file kind is available, let's build a private one

		id = CVSTR( "com.4d.");
		if(!GetDisplayName().IsEmpty())
		{			
			id += GetDisplayName();
			id += ".";

		}
		id += GetDefaultIdentifier();

		// if an id is already registered, take it, else create a new one and register it
		fileKind = VFileKind::RetainFileKind( id);
		if (fileKind == NULL)
		{
			VectorOfFileExtension extensions;
			for(long i=0;i<fFileExtensions.GetCount();i++)
				extensions.push_back( fFileExtensions[i]);
			VFileKindManager::Get()->RegisterPrivateFileKind( id, GetDisplayName(), extensions, VectorOfFileKind(0));
			fileKind = VFileKind::RetainFileKind( id);
		}
	}
	if(fileKind)
		outFileKind.push_back(fileKind);
	ReleaseRefCountable( &fileKind);
}

bool VPictureCodec::IsEncodedByMe(const VPictureData& inData)const
{
	bool result=false;
	const VPictureCodec* deco=inData.RetainDecoder();
	if(deco)
	{
		result= deco==this;
		deco->Release();
	}
	return result;
}

bool VPictureCodec::NeedReEncode(const VPictureData& inData,const VValueBag* inCompressionSettings,VPictureDrawSettings* inSet)const
{
	bool result;
	result = !(IsEncodedByMe(inData) && inData.GetDataProvider()!=NULL);
	if(!result)
	{
		if(inSet && !inSet->IsIdentityMatrix())
			result = true;

		#if !VERSION_LINUX
		//if some settings are modified, we need to reencode
		if ((!result) && inCompressionSettings)
		{
			ImageEncoding::stReader reader( inCompressionSettings);
			result = reader.NeedReEncode();
		}
		#endif
	}
	return result;
}

VError VPictureCodec::CreateFileWithBlob(VBlobWithPtr& inBlob,VFile& inFile)
{
	VError result;
	result=inFile.Create();
	if(result==VE_OK)
	{
		VFileDesc* outFileDesc;
		result=inFile.Open(FA_READ_WRITE,&outFileDesc);
		if(result==VE_OK)
		{
			result=outFileDesc->PutData(inBlob.GetDataPtr(),inBlob.GetSize(),0);
			delete outFileDesc;
		}
	}
	return result;
}

VError VPictureCodec::Encode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet)const
{
	VError result;
	VPictureDrawSettings set(inSet);
	if(!NeedReEncode(inData,inSettings,inSet))
	{
		// la pict n'as aucune transformation, on la sauvegarde directement
		VSize outsize;
		result=inData.Save(&outBlob,0,outsize);
	}
	else
	{
		result=DoEncode(inData,inSettings,outBlob,&set);
	}
	return result;
}
VError VPictureCodec::Encode(const VPictureData& inData,const VValueBag* inSettings,VFile& outFile,VPictureDrawSettings* inSet)const
{
	VError result;
	VPictureDrawSettings set(inSet);
	if(!NeedReEncode(inData,inSettings,inSet))
	{
		result = inData.SaveToFile( outFile);
	}
	else
	{
		result=DoEncode(inData,inSettings,outFile,&set);
	}
	return result;
}
	

VError VPictureCodec::Encode(const VPicture& inPict,const VValueBag* inSettings,VBlob& outBlob)const
{
	VError result=VE_INVALID_PARAMETER;
	
	VString id=GetDefaultIdentifier();
	
	// on cherche si la pict source contient deja le format demand
	const VPictureData* data=inPict.RetainPictDataByIdentifier(id);
	
	if(!data)
	{
		data=inPict.RetainPictDataForDisplay();
	}
	
	if(data)
	{
		VPictureDrawSettings set(&inPict);
		if(!NeedReEncode(*data,inSettings,&set))
		{
			// la pict n'as aucune transformation, on la sauvegarde directement
			VSize outsize;
			result=data->Save(&outBlob,0,outsize);
		}
		else
		{
			result=DoEncode(*data,inSettings,outBlob,&set);
		}
		
		data->Release();
	}
	return result;
}
VError VPictureCodec::Encode(const VPicture& inPict,const VValueBag* inSettings,VFile& outFile)const
{
	VError result=VE_INVALID_PARAMETER;
	
	VString id=GetDefaultIdentifier();
	
	const VPictureData* data=inPict.RetainPictDataByMimeType(id);
	
	if(!data)
	{
		data=inPict.RetainPictDataForDisplay();
	}

	if(data)
	{
		VPictureDrawSettings set(&inPict);
		if(!NeedReEncode(*data,inSettings,&set))
		{
			// la pict n'as aucune transformation, on la sauvegarde directement
			// TBD save picture data to file
			result = data->SaveToFile( outFile);
		}
		else
		{
			result=DoEncode(*data,inSettings,outFile,&set);
		}
		
		data->Release();
	}
	return result;
}

#if VERSIONWIN
VError VPictureCodec::_GDIPLUS_Encode(const VString& inMimeType,const VPictureData& inData,const VValueBag* /*inSettings*/,VBlob& outBlob,VPictureDrawSettings* inSet) const
{
	VError result;
	inData.Retain();
	Gdiplus::Image* im=0;
	bool ownimage=false;
	VPictureDrawSettings set(inSet);
	if(set.IsIdentityMatrix())
	{
		im=inData.GetGDIPlus_Image();
		if(im)
		{
			UINT flags = im->GetFlags();
			if(flags & Gdiplus::ImageFlagsHasAlpha)
			{
				if(set.GetDrawingMode()==3)
				{
					// dest image does not support alpha channel
					// let's create a new bitmap
					im=inData.CreateGDIPlus_Bitmap(&set);
					ownimage=true;
				}
			}
		}
	}
	if(!im)
	{
		im=inData.CreateGDIPlus_Bitmap(&set);
		ownimage=true;
	}
	
	if(im)
	{
		VPictureCodecFactoryRef fact;
		const xGDIPlusDecoderList& decoderlist=fact->xGetGDIPlus_Encoder();	
		const	xGDIPlusCodecInfo* codec=decoderlist.FindByMimeType(inMimeType);
		if(codec)
		{
			CLSID encoder;
			codec->GetCLSID(encoder);
			DWORD err;
			IStream* stream=NULL; 
			
			err= ::CreateStreamOnHGlobal(NULL,true,&stream);
			if(err == S_OK)
			{
				Gdiplus::Status stat = Gdiplus::GenericError;
				stat = im->Save(stream,&encoder,NULL);
				if(stat==Gdiplus::Ok)
				{
					HGLOBAL glob;
					GetHGlobalFromStream(stream,&glob);
					char* c=(char*)GlobalLock(glob);
					size_t outSize=GlobalSize(glob);
					result = outBlob.PutData(c,outSize,0);
					GlobalUnlock(glob);
				}
				else
					result=VE_UNKNOWN_ERROR;
				stream->Release();
			}
		}
	}
	if(im && ownimage)
	{
		delete im;
	}
	inData.Release();
	return result;
}
#endif

VError VPictureCodec::_GenericEncodeUnknownData(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet) const
{
	XBOX::VError err=VE_UNIMPLEMENTED;
	if(IsEncodedByMe(inData))
	{
		VSize outsize;
		err=inData.Save(&outBlob,0,outsize);
	}
	return err;
}
VError VPictureCodec::_GenericEncodeUnknownData(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet) const
{
	XBOX::VError err=VE_UNIMPLEMENTED;
	if(IsEncodedByMe(inData))
	{
		VBlobWithPtr blob;
		err=Encode(inData,inSettings,blob,inSet);
		if(err==VE_OK)
		{
			err=CreateFileWithBlob(blob,inFile);
		}
	}
	return err;
}

bool VPictureCodec::CheckMacType(sLONG inExt) const
{
	bool result=false;
	for(long i=0;i<fFileMacType.GetCount();i++)
	{
		result = fFileMacType[i]==inExt;
		if(result)
			break;
		
	}
	return result;
}
VString VPictureCodec::GetDefaultIdentifier() const
{
	VString res=sCodec_none;

	if(fFileExtensions.GetCount()>0)
	{
		res=".";
		res+=fFileExtensions[0];
	}
	else
	{
		if(fMimeTypes.GetCount()>0)
		{
			res=fMimeTypes[0];
		}
		else
		{
			if(fFileMacType.GetCount()>0)
			{
				OsType t=fFileMacType[0];
				#if SMALLENDIAN
				ByteSwapLong(&t);
				#endif
				res.FromOsType(t);
			}
			else
			{
				if(fQTExporterComponentType.GetCount()>0)
				{
					OsType t=fQTExporterComponentType[0];
					#if SMALLENDIAN
					ByteSwapLong(&t);
					#endif
					res.FromOsType(t);
				}
			}
		}
	}
	return res;
}
bool VPictureCodec::CheckExtension(const VString& inExt) const
{
	if (inExt.IsEmpty())
		return false;

	const UniChar *p = inExt.GetCPointer();
	VIndex length = inExt.GetLength();
	if (*p == '.')
	{
		++p;
		--length;
	}
	
	for(long i=0;i<fFileExtensions.GetCount();i++)
	{
		if (VIntlMgr::GetDefaultMgr()->EqualString( fFileExtensions[i].GetCPointer(), fFileExtensions[i].GetLength(), p, length, false))
			return true;
	}
	return false;
}
bool VPictureCodec::CheckQTExporterComponentType(const OsType inType)const
{
	bool result=false;
	for(VSize i=0;i<CountQTExporterComponentType();i++)
	{
		if(fQTExporterComponentType[(VIndex)i]==inType)
		{
			result=true;
			break;
		}
	}
	return result;
}
bool VPictureCodec::CheckMimeType(const VString& inMime) const
{
	bool result=false;
	for(long i=0;i<fMimeTypes.GetCount();i++)
	{
		result = fMimeTypes[i].EqualToString(inMime,false);
		if(result)
			break;
		
	}
	return result;
}

bool VPictureCodec::CheckIdentifier(const VString& inExt)const
{
	bool result;
	if(!inExt.IsEmpty() && (inExt[0] == '.'))
	{
		result=CheckExtension(inExt);
	}
	else
	{
		if(inExt.GetLength()==4)
		{
			result=CheckMacType(inExt.GetOsType());
			if(!result)
			{
				result=CheckQTExporterComponentType(inExt.GetOsType());
			}
		}
		else
		{
			result=CheckMimeType(inExt);
		}
	}
	return result;
}

void VPictureCodec::AppendMacType(sLONG inExt)
{
	fFileMacType.AddTail(inExt);
}
void VPictureCodec::AppendExtension(const VString& inExt)
{
	fFileExtensions.AddTail(inExt);
}
void VPictureCodec::AppendMimeType(const VString& inStr)
{
	fMimeTypes.AddTail(inStr);
}
void VPictureCodec::AppendQTExporterCompentType(const OsType inStr)
{
	fQTExporterComponentType.AddTail(inStr);
}

VPictureData* VPictureCodec::CreatePictDataFromVFile( const VFile& inFile) const
{
	VPictureData* result = NULL;
	VPictureDataProvider* datasrc=VPictureDataProvider::Create(inFile);
	if(datasrc)
	{
		datasrc->SetDecoder(this);
		result=_CreatePictData(*datasrc,0);
		datasrc->Release();
	}
	return result;
}
VPictureData* VPictureCodec::CreatePictDataFromDataProvider(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder) const
{
	inDataProvider.SetDecoder(this);
	VPictureData* result=_CreatePictData(inDataProvider,inRecorder);
	return result;
}

void VPictureCodec::AppendPattern(sWORD len,const uBYTE* inPattern)
{
	uBYTE* pattern;
	pattern=(uBYTE*)new char[len+sizeof(sWORD)];
	*((sWORD*)pattern)=len;
	memcpy(&pattern[2],inPattern,len);
	fSignaturePatterns.AddTail(pattern);
	fMaxSignLen=Max(fMaxSignLen,len);
}
bool VPictureCodec::ValidateData(VFile& inFile)const
{
	bool ok=false;
	if(inFile.Exists())
	{
		VPictureDataProvider* ds=VPictureDataProvider::Create(inFile,false);
		if(ds)
		{
			ok=ValidateData(*ds);
			ds->Release();
		}
	}
	return ok;
}
bool VPictureCodec::ValidateExtention(VFile& inFile)const
{
	VString ext;
	inFile.GetExtension(ext);
	return ValidateExtention(ext);
}	

bool VPictureCodec::ValidateData(VPictureDataProvider& inDataProvider)const
{
	uBYTE* buff=(uBYTE*)new char[fMaxSignLen+1];
	bool ok=false;
	if(buff)
	{
		for(long i=0;i<fSignaturePatterns.GetCount();i++)
		{
			uBYTE* pattern=fSignaturePatterns[i];
			sWORD len=*(sWORD*)pattern;
			if(len)
			{
				pattern+=2;
				if(inDataProvider.GetDataSize()>=(VSize)len)
				{
					inDataProvider.GetData(buff,0,len);
					ok = ValidatePattern(pattern, (uBYTE*)buff, len);
					if(ok)
						break;
				}
			}
		}
		delete[] buff;
	}
	else
		vThrowError(VE_MEMORY_FULL);
	return ok;
}
bool VPictureCodec::ValidatePattern(const uBYTE* inPattern, const uBYTE* inData, sWORD inLen)const
{
	return (memcmp(inPattern, inData, inLen) == 0);
}
bool VPictureCodec::ValidateExtention(const VString& inExt)const
{
	bool ok=false;
	for(long i=0;i<fFileExtensions.GetCount();i++)
	{
		if(fFileExtensions[i]==inExt)
		{
			ok=true;
			break;
		}	
	}
	return ok;
}

VPictureCodec_InMemBitmap::VPictureCodec_InMemBitmap()
{
	SetPrivate(true);
	SetEncode(FALSE);
	
	SetUTI("com.4D.4DMemoryBitmap");
	
	SetDisplayName("mem bitmap");
	AppendExtension("4DMemoryBitmap");
	AppendMimeType("application/4DMemoryBitmap");
	#if VERSIONMAC
	SetPlatform(1);
	#else
	SetPlatform(2);
	#endif
}

VPictureData* VPictureCodec_InMemBitmap::_CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder) const
{
	#if VERSIONMAC
	return new VPictureData_CGImage(&inDataProvider,inRecorder);
	#elif VERSIONWIN
	return new VPictureData_GDIPlus(&inDataProvider,inRecorder);
	#elif VERSION_LINUX
	return new VPictureData_NonRenderable(&inDataProvider,inRecorder);
	#endif
}

VPictureCodec_InMemVector::VPictureCodec_InMemVector()
{
	SetPrivate(true);
	SetEncode(FALSE);
	
	SetUTI("com.4D.4DMemoryVector");
	
	SetDisplayName("mem vector");
	AppendExtension("4DMemoryVector");
	AppendMimeType("application/4DMemoryVector");
	#if VERSIONMAC
	SetPlatform(1);
	#else
	SetPlatform(2);
	#endif
}
VPictureData* VPictureCodec_InMemVector::_CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder) const
{
	#if VERSIONMAC
	return new VPictureData_CGImage(&inDataProvider,inRecorder);
	#elif VERSIONWIN
	return new VPictureData_GDIPlus_Vector(&inDataProvider,inRecorder);
	#elif VERSION_LINUX
	return new VPictureData_NonRenderable(&inDataProvider,inRecorder);
	#endif
}


VPictureCodec_PDF::VPictureCodec_PDF()
{
	SetEncode(TRUE);

	SetUTI("com.adobe.pdf");
	
	SetDisplayName("Pdf");
	
	AppendExtension("pdf");
	AppendExtension("eps");
	AppendMimeType("application/pdf");
	AppendMimeType("application/x-pdf");
	
	uBYTE signature[]={0x25,0x50,0x44,0x46,0x2d};
	uBYTE signature1[]={0x25,0x21,0x50,0x53,0x2d,0x41,0x64,0x6f,0x62,0x65};
	AppendPattern(sizeof(signature),signature);
	AppendPattern(sizeof(signature1),signature1);
	SetPrivateScrapKind(SCRAP_KIND_PICTURE_PDF);
	SetPlatform(1);
	
}
VPictureData* VPictureCodec_PDF::_CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder) const
{
	#if VERSIONMAC
	return new VPictureData_PDF(&inDataProvider,inRecorder);
	#elif VERSIONWIN
	return new VPictureData_NonRenderable(&inDataProvider,inRecorder);
	#elif VERSION_LINUX
	return new VPictureData_NonRenderable(&inDataProvider,inRecorder);
	#endif
}

#if VERSIONMAC
VError VPictureCodec_PDF::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet) const
{
	VError result=VE_UNKNOWN_ERROR;
	
	VPictureDataProvider* datasource;
	CGPDFDocumentRef pdf=inData.CreatePDF(inSet,&datasource);
	if(pdf && datasource)
	{
		result=datasource->WriteToBlob(outBlob,0);
		CGPDFDocumentRelease(pdf);
		datasource->Release();
	}

	return result;
}
VError VPictureCodec_PDF::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet) const
{
	VError result=VE_UNKNOWN_ERROR;
	VPictureDataProvider* datasource;
	CGPDFDocumentRef pdf=inData.CreatePDF(inSet,&datasource);
	if(pdf && datasource)
	{
		result=inFile.Create();
		if(result==VE_OK)
		{
			VFileDesc* outFileDesc;
			result=inFile.Open(FA_READ_WRITE,&outFileDesc);
			if(result==VE_OK)
			{
				result=outFileDesc->PutData(datasource->BeginDirectAccess(),datasource->GetDataSize(),0);
				datasource->EndDirectAccess();
				delete outFileDesc;
			}
		}
		CGPDFDocumentRelease(pdf);
		datasource->Release();
	}
	return result;
}
#else
VError VPictureCodec_PDF::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet) const
{
	return _GenericEncodeUnknownData(inData,inSettings,outBlob,inSet);
}
VError VPictureCodec_PDF::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet) const
{
	return _GenericEncodeUnknownData(inData,inSettings,inFile,inSet);
}
#endif


VPictureCodec_4DMeta::VPictureCodec_4DMeta()
{
	SetPrivate(true);
	
	SetUTI("com.4D.4DMetaPict");
	
	SetEncode(FALSE);
	SetDisplayName("4DMetaPict");
	AppendExtension("4DMetaPict");
	AppendMimeType("image/4DMetaPict");
	
	uBYTE signature[]={'4','M','T','A'};
	uBYTE signature1[]={'A','T','M','4'};
	AppendPattern(sizeof(signature),signature);
	AppendPattern(sizeof(signature1),signature1);
}
VPictureData* VPictureCodec_4DMeta::_CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder) const
{
	return new VPictureData_Meta(&inDataProvider,inRecorder);
}

VPictureCodec_JPEG::VPictureCodec_JPEG()
{
	SetEncode(TRUE);
	
	SetUTI("public.jpeg");
	
#if VERSIONWIN
	fGUID = GUID_ContainerFormatJpeg;
#endif

	SetDisplayName("Jpeg");
	AppendExtension("jpg");
	AppendExtension("jif");
	AppendExtension("jpeg");
	AppendExtension("jpe");
	AppendMimeType("image/jpeg");
	AppendMimeType("image/jpg");
	AppendMimeType("image/pjpeg");
	AppendMimeType("image/jpe_");
	
	AppendQTExporterCompentType('jpeg');
	AppendQTExporterCompentType('JPEG');

	
	uBYTE signature[]={0xFF,0xD8};
	AppendPattern(sizeof(signature),signature);
	SetPrivateScrapKind(SCRAP_KIND_PICTURE_JFIF);
	
}

#if VERSION_LINUX

VError VPictureCodec_JPEG::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet) const
{
	return _GenericEncodeUnknownData(inData,inSettings,outBlob,inSet);
}
VError VPictureCodec_JPEG::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet) const
{
	return _GenericEncodeUnknownData(inData,inSettings,inFile,inSet);
}

#else

VError VPictureCodec_JPEG::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet) const
{
	VError result=VE_OK;
	VPictureDrawSettings	set(inSet);
	#if VERSIONWIN
	//try to encode with WIC codec first
	result=VPictureCodec_WIC::_Encode(static_cast<const VPictureCodec *>(this),inData,inSettings,outBlob,&set);
	
	//backfall to gdiplus codec encoder if WIC encoder failed
	//but not because of encoding modified metadatas
	//(VPictureCodec_WIC::Encode return VE_INVALID_PARAMETER only if 
	// codec failed to encode modified metadatas: if it is the case,
	// we come from 4D command SET PICTURE METADATA and we must return this error
	// and do not try to encode further)
	if (result != VE_OK && result != VE_INVALID_PARAMETER)
	{
		set.SetBackgroundColor(ENCODING_BACKGROUND_COLOR);
		result=_GDIPLUS_Encode("image/jpeg",inData,inSettings,outBlob,&set);
	}
	#else
	result=VPictureCodec_ImageIO::_Encode(fUTI,inData,inSettings,outBlob,&set);
	#endif
	return result;
}
VError VPictureCodec_JPEG::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet) const
{
	VError result=VE_OK;
	VBlobWithPtr blob;
	result=Encode(inData,inSettings,blob,inSet);
	if(result==VE_OK)
	{	
		result=	CreateFileWithBlob(blob,inFile);
	}
	return result;
}
#endif

bool VPictureCodec_JPEG::NeedReEncode(const VPictureData& inData,const VValueBag* inCompressionSettings,VPictureDrawSettings* inSet)const
{
	bool result=inherited::NeedReEncode(inData,inCompressionSettings,inSet);
	return result;
}

VPictureData* VPictureCodec_JPEG::_CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder) const
{
	#if VERSIONMAC
	return new VPictureData_CGImage(&inDataProvider,inRecorder);
	#elif VERSIONWIN
	return new VPictureData_GDIPlus(&inDataProvider,inRecorder);
	#elif VERSION_LINUX
	return new VPictureData_NonRenderable(&inDataProvider,inRecorder);
	#endif
}

VPictureCodec_PNG::VPictureCodec_PNG()
{
	SetEncode(TRUE);
	
	SetUTI("public.png");
	
#if VERSIONWIN
	fGUID = GUID_ContainerFormatPng;
#endif

	SetDisplayName("Png");
	AppendExtension("png");
	AppendMimeType("image/png");
	AppendMimeType("image/x-png");
	AppendQTExporterCompentType('PNGf');
	uBYTE signature[]={0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a};
	AppendPattern(sizeof(signature),signature);
	SetPrivateScrapKind(SCRAP_KIND_PICTURE_PNG);
	
}
VPictureData* VPictureCodec_PNG::_CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder) const
{
	#if VERSIONMAC
	return new VPictureData_CGImage(&inDataProvider,inRecorder);
	#elif VERSIONWIN
	return new VPictureData_GDIPlus(&inDataProvider,inRecorder);
	#elif VERSION_LINUX
	return new VPictureData_NonRenderable(&inDataProvider,inRecorder);	
	#endif
}

#if VERSION_LINUX

VError VPictureCodec_PNG::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet) const
{
	return _GenericEncodeUnknownData(inData,inSettings,outBlob,inSet);
}

VError VPictureCodec_PNG::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& outBlob,VPictureDrawSettings* inSet) const
{
	return _GenericEncodeUnknownData(inData,inSettings,outBlob,inSet);
}

#else

VError VPictureCodec_PNG::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet) const
{
	VError result=VE_OK;
	VPictureDrawSettings	set(inSet);
	#if VERSIONWIN
	//try to encode with WIC codec first
	result=VPictureCodec_WIC::_Encode(static_cast<const VPictureCodec *>(this),inData,inSettings,outBlob,&set);
	
	//backfall to gdiplus codec encoder if WIC encoder failed
	//but not because of encoding modified metadatas
	//(VPictureCodec_WIC::Encode return VE_INVALID_PARAMETER only if 
	// codec failed to encode modified metadatas: if it is the case,
	// we come from 4D command SET PICTURE METADATA and we must return this error
	// and do not try to encode further)
	if (result != VE_OK && result != VE_INVALID_PARAMETER)
		result=_GDIPLUS_Encode("image/png",inData,inSettings,outBlob,inSet);
#else
	result=VPictureCodec_ImageIO::_Encode(fUTI,inData,inSettings,outBlob,inSet);
#endif
	return result;
}

VError VPictureCodec_PNG::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet) const
{
	VError result=VE_OK;
	VBlobWithPtr blob;
	result=Encode(inData,inSettings,blob,inSet);
	if(result==VE_OK)
	{
		result=CreateFileWithBlob(blob,inFile);
	}
	return result;
}

#endif

VPictureCodec_BMP::VPictureCodec_BMP()
{
	SetEncode(TRUE);
	
	SetUTI("com.microsoft.bmp");
	
#if VERSIONWIN
	fGUID = GUID_ContainerFormatBmp;
#endif

	SetDisplayName("Bmp");
	AppendExtension("bmp");
	AppendExtension("dib");
	AppendExtension("rle");
	AppendQTExporterCompentType('BMPf');
	uBYTE signature[] = { 0x42, 0x4D };
	uBYTE signature2[] = { 0x42, 0x41 };
	AppendMimeType("image/bmp");
	AppendMimeType("image/x-bmp");
	AppendPattern(sizeof(signature),signature);
	AppendPattern(sizeof(signature2),signature2);
}
VPictureData* VPictureCodec_BMP::_CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder) const
{
	#if VERSIONMAC
	return new VPictureData_CGImage(&inDataProvider,inRecorder);
	#elif VERSIONWIN
	return new VPictureData_GDIPlus(&inDataProvider,inRecorder);
	#elif VERSION_LINUX
	return new VPictureData_NonRenderable(&inDataProvider,inRecorder);		
	#endif
}

#if VERSION_LINUX

VError VPictureCodec_BMP::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet) const
{
	return _GenericEncodeUnknownData(inData,inSettings,outBlob,inSet);
}

VError VPictureCodec_BMP::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& outBlob,VPictureDrawSettings* inSet) const
{
	return _GenericEncodeUnknownData(inData,inSettings,outBlob,inSet);
}

#else

VError VPictureCodec_BMP::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet) const
{
	VError result=VE_OK;
	VPictureDrawSettings	set(inSet);
	#if VERSIONWIN
	//try to encode with WIC codec first
	result=VPictureCodec_WIC::_Encode(static_cast<const VPictureCodec *>(this),inData,inSettings,outBlob,&set);
	
	//backfall to gdiplus codec encoder if WIC encoder failed
	//but not because of encoding modified metadatas
	//(VPictureCodec_WIC::Encode return VE_INVALID_PARAMETER only if 
	// codec failed to encode modified metadatas: if it is the case,
	// we come from 4D command SET PICTURE METADATA and we must return this error
	// and do not try to encode further)
	if (result != VE_OK && result != VE_INVALID_PARAMETER)
	{
		set.SetBackgroundColor(ENCODING_BACKGROUND_COLOR);
		result=_GDIPLUS_Encode("image/bmp",inData,inSettings,outBlob,&set);
	}
	#else
	result=VPictureCodec_ImageIO::_Encode(fUTI,inData,inSettings,outBlob,&set);
	#endif
	return result;
}

VError VPictureCodec_BMP::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet) const
{
	VError result=VE_OK;
	VBlobWithPtr blob;
	result=Encode(inData,inSettings,blob,inSet);
	if(result==VE_OK)
	{
		result=CreateFileWithBlob(blob,inFile);
	}
	return result;
}

#endif

VPictureCodec_PICT::VPictureCodec_PICT()
{
#if  VERSIONWIN || (VERSIONMAC && ARCH_32)
	SetEncode(TRUE);
#else
	SetEncode(FALSE);
#endif
	SetUTI("com.apple.pict");
	
	SetDisplayName("Mac Picture");
	AppendExtension("pict");
	AppendExtension("pct");
	AppendExtension("pic");
	AppendQTExporterCompentType('PICT');
	AppendMimeType("image/pict");
	AppendMimeType("image/x-pict");
	AppendMimeType("image/x-macpict");
	#if VERSIONMAC
	SetPrivateScrapKind(SCRAP_KIND_PICTURE_PICT);
	#else
	SetPrivateScrapKind(SCRAP_KIND_PICTURE_PICT);
	#endif
	

}
VPictureData* VPictureCodec_PICT::_CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder) const
{
#if  VERSIONWIN || (VERSIONMAC && ARCH_32)
	return new VPictureData_MacPicture(&inDataProvider,inRecorder);
#else
	return new VPictureData_NonRenderable(&inDataProvider,inRecorder);
#endif
}
VPictureData* VPictureCodec_PICT::CreatePictDataFromVFile( const VFile& inFile) const
{
	VPictureData *result=0;
	if(inFile.Exists())
	{
		VFileDesc* desc=NULL;
		if(inFile.Open( FA_READ, &desc)==VE_OK)
		{
			VSize buffsize=(VSize) (desc->GetSize() - 0x200);
			if(buffsize>0)
			{
				VPtr buff=VMemory::NewPtr(buffsize, 'pict');
				if(buff)
				{
					desc->SetPos(0x200);
					if(desc->GetData ( buff, buffsize, 0x200 )==VE_OK)
					{
						VPictureDataProvider* data=VPictureDataProvider::Create(buff,true,buffsize);
						if(data)
						{
							result = _CreatePictData(*data,0);
							data->Release();
						}
					}
					else
					{
						VMemory::DisposePtr(buff);
					}
				}
				else
				{
					vThrowError(VE_MEMORY_FULL);
				}
			} 
			delete desc;
		}
	}
	return result;
}

bool VPictureCodec_PICT::ValidateData(VFile& inFile)const
{
	bool ok=false;
	if(inFile.Exists())
	{
		VFileDesc* desc=NULL;
		if(inFile.Open( FA_READ, &desc)==VE_OK)
		{
			long buffsize= 0x200+sizeof(xMacPictureHeader)+sizeof(sWORD);
			if(desc->GetSize()>buffsize)
			{
				uBYTE* buff=(uBYTE*)new char[buffsize];
				
				if(desc->GetData ( buff, buffsize, 0)==VE_OK)
				{
					bool okheader=true;
					// skip 0x200 null byte
					/* aci58993 les 512 premier octets ne sont forcement a zero
					sLONG i,*l;
					l=(long*)buff;
					for(i=0;i<0x200/4;i++)
					{
						if(l[i]!=0)
						{
							okheader=false;
							break;
						}
					}
					*/
					if(okheader)
					{
						
						if((buffsize-0x200)>sizeof(xMacPicture) + sizeof(sWORD))
						{
							xMacPictureHeader header;
							memcpy(&header,(VPtr)buff+0x200,sizeof(xMacPictureHeader));
							#if SMALLENDIAN
							ByteSwapPictHeader(&header);
							#endif
							ok=OkHeader(&header);
						}
					}
				}
			
				delete[] buff;
			}
			delete desc;
		}
	}
	return ok;
}

bool VPictureCodec_PICT::OkHeader(xMacPictureHeaderPtr inHeader)const
{
	bool ok=false;
	if(inHeader->picVersion==0x1101)
	{
		ok=true;
	}
	else if (inHeader->picVersion==0x0011)
	{
		if(inHeader->version.v1.versionNumber==0x2ff)
		{
			ok= (inHeader->version.v1.headerVersion==-1 || inHeader->version.v1.headerVersion==-2);
		}
	}
	return ok;
}
bool VPictureCodec_PICT::ValidateData(VPictureDataProvider& inDataProvider)const
{
	VSize offset=0;
	bool ok=false;
	VSize datasize=inDataProvider.GetDataSize();
	if(inDataProvider.IsFile())
		offset=0x200;
		
	if(datasize-offset >sizeof(xMacPicture) + sizeof(sWORD))
	{
		xMacPictureHeaderPtr header=(xMacPictureHeaderPtr)malloc(sizeof(xMacPictureHeader));
		if(header)
		{
			inDataProvider.GetData(header,offset,sizeof(xMacPictureHeader));
			#if SMALLENDIAN
			ByteSwapPictHeader(header);
			#endif
			ok=OkHeader(header);
			free((void*) header);
		}
	}
	return ok;
}
void VPictureCodec_PICT::ByteSwapPictHeader(xMacPictureHeaderPtr inHeader)const
{
	ByteSwapWordArray((sWORD*)inHeader,6);
	ByteSwapWord(&inHeader->version.v1.versionNumber);
	ByteSwapWord(&inHeader->version.v1.headerOpcode);
	ByteSwapWord(&inHeader->version.v1.headerVersion);
}

#if VERSION_LINUX

VError VPictureCodec_PICT::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet) const
{
	return _GenericEncodeUnknownData(inData,inSettings,outBlob,inSet);
}

VError VPictureCodec_PICT::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& outBlob,VPictureDrawSettings* inSet) const
{
	return _GenericEncodeUnknownData(inData,inSettings,outBlob,inSet);
}

#else

VError VPictureCodec_PICT::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet) const
{
#if  VERSIONWIN || (VERSIONMAC && ARCH_32)
	VError result=VE_UNKNOWN_ERROR;
	VPictureDrawSettings	set(inSet);
	set.SetBackgroundColor(ENCODING_BACKGROUND_COLOR);
	VHandle pict=inData.CreatePicVHandle(&set);
	if(pict)
	{
		VPtr p=VMemory::LockHandle(pict);
		if(p)
		{
			VSize outSize=VMemory::GetHandleSize(pict);
#if VERSIONWIN
			ByteSwapWordArray((sWORD*)p,5);
#endif
			result=outBlob.PutData(p,outSize,0);
		}
		VMemory::UnlockHandle(pict);
		
		
		result=outBlob.PutData(VMemory::LockHandle(pict),VMemory::GetHandleSize(pict));
		VMemory::UnlockHandle(pict);
		VMemory::DisposeHandle(pict);
	}
	return result;
#else
	return _GenericEncodeUnknownData(inData,inSettings,outBlob,inSet);
#endif
}
VError VPictureCodec_PICT::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet) const
{
#if  VERSIONWIN || (VERSIONMAC && ARCH_32)
	VError result=VE_UNKNOWN_ERROR;
	VPictureDrawSettings	set(inSet);
	set.SetBackgroundColor(ENCODING_BACKGROUND_COLOR);
	VHandle pict=inData.CreatePicVHandle(inSet);
	if(pict)
	{
		result=inFile.Create();
		if(result==VE_OK)
		{
			VFileDesc* outFileDesc;
			result=inFile.Open(FA_READ_WRITE,&outFileDesc);
			if(result==VE_OK)
			{
				char buff[0x200];
				memset(buff,0,0x200);
				result=outFileDesc->PutData(buff,0x200,0);
				
				#if VERSIONWIN
				VPtr p=VMemory::LockHandle(pict);
				if(p)
				{
					ByteSwapWordArray((sWORD*)p,5);
					VMemory::UnlockHandle(pict);
				}
				#endif

				result=outFileDesc->PutData(VMemory::LockHandle(pict),VMemory::GetHandleSize(pict),0x200);
				VMemory::UnlockHandle(pict);
				delete outFileDesc;
			}
		}
		VMemory::DisposeHandle(pict);
	}
	return result;
#else
	return _GenericEncodeUnknownData(inData,inSettings,inFile,inSet);
#endif
}

#endif

VPictureCodec_GIF::VPictureCodec_GIF()
{
	SetEncode(TRUE);
	
	SetUTI("com.compuserve.gif");
	
#if VERSIONWIN
	fGUID = GUID_ContainerFormatGif;
#endif

	SetDisplayName("Gif");
	AppendExtension("gif");
	AppendMimeType("image/gif");
	AppendQTExporterCompentType('GIFf');

	uBYTE signature[] = { 'G', 'I','F','x','x','x' };
	
	AppendPattern(sizeof(signature),signature);
	
	SetPrivateScrapKind(SCRAP_KIND_PICTURE_GIF);
}

VPictureData* VPictureCodec_GIF::_CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder) const
{
#if VERSION_LINUX
	return new VPictureData_NonRenderable(&inDataProvider,inRecorder);	
#else
	return new VPictureData_GIF(&inDataProvider,inRecorder);
#endif
}

#if VERSION_LINUX

VError VPictureCodec_GIF::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet) const
{
	return _GenericEncodeUnknownData(inData,inSettings,outBlob,inSet);
}
VError VPictureCodec_GIF::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet) const
{
	return _GenericEncodeUnknownData(inData,inSettings,inFile,inSet);
}

#else

VError VPictureCodec_GIF::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet) const
{
	VError result=VE_OK;
	VPictureDrawSettings set(inSet);
	if(IsEncodedByMe(inData) && set.IsIdentityMatrix())
	{
		VSize outSize;
		result=inData.Save(&outBlob,0,outSize);
	}
	else
	{
	#if VERSIONWIN
	//long a=1;
	//if(a==0)
	//result=_GDIPLUS_Encode("image/gif",inData,inSettings,outBlob,inSet);
	//else
	{
		set.SetBackgroundColor(ENCODING_BACKGROUND_COLOR);
		Gdiplus::Bitmap* bm=inData.CreateGDIPlus_Bitmap(&set);
		if(bm && bm->GetWidth()>0 && bm->GetHeight()>0)
		{
		VBitmapData src(*bm),*dest;
		delete bm;
		VNNQuantizer qt(255,15);
		dest=qt.Quantize(src);
		if(dest)
		{
			xGifEncoder encoder;
			VHandle vh = VMemory::NewHandle(1);
			VHandleStream* stream = new XBOX::VHandleStream (&vh);
			result = stream->OpenWriting();
			if(result!=XBOX::VE_OK)
			{
				delete stream;
				VMemory::DisposeHandle(vh);
				delete dest;
			}
			else
			{
				short err=encoder.GIFEncode(*stream,dest,0,0,0);
				if(err==0)
				{
					outBlob.PutData(VMemory::LockHandle(vh),VMemory::GetHandleSize(vh));
					VMemory::UnlockHandle(vh);
				}
				delete stream;
				VMemory::DisposeHandle(vh);
				delete dest;
			}
		}
	}
	}
	#else
	{
		if(inData.GetWidth()>0 && inData.GetHeight()>0)
		{
			VPolygon poly=set.TransformSize(inData.GetWidth(),inData.GetHeight());
			
			CGContextRef    context = NULL;
			CGColorSpaceRef colorSpace;
			void *          bitmapData;
			int             bitmapByteCount;
			int             bitmapBytesPerRow;
			sLONG width,height;
			width=poly.GetWidth();
			height=poly.GetHeight();
			bitmapBytesPerRow   = ( 4 * width + 15 ) & ~15;
			bitmapByteCount     = (bitmapBytesPerRow * height);
			
			set.SetBackgroundColor(ENCODING_BACKGROUND_COLOR);	
						
			bitmapData = malloc( bitmapByteCount );
			
			if(bitmapData)
			{
				colorSpace = CGColorSpaceCreateDeviceRGB();

				memset(bitmapData,0,bitmapByteCount);
				context = CGBitmapContextCreate (bitmapData,
									width,
									height,
									8,      // bits per component
									bitmapBytesPerRow,
									colorSpace,
									kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Host);
				if (context)
				{
					CGRect vr;
					vr.origin.x= 0;
					vr.origin.y=  0;
					vr.size.width=poly.GetWidth();
					vr.size.height=poly.GetHeight();
					if(set.GetDrawingMode()==3)
					{
						VColor col=set.GetBackgroundColor();
						::CGContextSetRGBFillColor(context,col.GetRed()/(GReal) 255,col.GetGreen()/(GReal) 255,col.GetBlue()/(GReal) 255,col.GetAlpha()/(GReal) 255);
						::CGContextFillRect(context, vr);
					}
					else
					{
						::CGContextClearRect(context, vr);
						
					}
					VRect pictrect(0,0,poly.GetWidth(),poly.GetHeight());
					set.SetScaleMode(PM_SCALE_TO_FIT);
					set.SetYAxisSwap(poly.GetHeight(),true);
					CGContextSaveGState(context);
					inData.DrawInCGContext(context,pictrect,&set);

					CGContextRestoreGState(context);
					CGContextRelease(context);
					
					{
						VBitmapData src(bitmapData,width,height,bitmapBytesPerRow,VBitmapData::VPixelFormat32bppPARGB),*dest;
					
						VNNQuantizer qt(255,15);
						dest=qt.Quantize(src);
						if(dest)
						{
							xGifEncoder encoder;
							VHandle vh = VMemory::NewHandle(1);
							VHandleStream* stream = new XBOX::VHandleStream (&vh);
							result = stream->OpenWriting();
							if(result!=XBOX::VE_OK)
							{
								delete stream;
								VMemory::DisposeHandle(vh);
								delete dest;
							}
							else
							{
								short err=encoder.GIFEncode(*stream,dest,0,0,0);
								if(err==0)
								{
									outBlob.PutData(VMemory::LockHandle(vh),VMemory::GetHandleSize(vh));
									VMemory::UnlockHandle(vh);
								}
								delete stream;
								VMemory::DisposeHandle(vh);
								delete dest;
							}
						}
					}
				}
				free(bitmapData);
			}
		}
	}
	#endif
	}
	return result;
}

VError VPictureCodec_GIF::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet) const
{
	VError result=VE_OK;
	VPictureDrawSettings	set(inSet);
	VBlobWithPtr blob;
	result=Encode(inData,inSettings,blob,inSet);
	if(result==VE_OK)
	{
		result=CreateFileWithBlob(blob,inFile);
	}
	return result;
}

#endif

bool VPictureCodec_GIF::ValidatePattern(const uBYTE* inPattern, const uBYTE* inData, sWORD /*inLen*/)const
{
	bool result=false;
	
	if(memcmp(inPattern, inData, 3)==0)
	{
		result = ( inData[3] >= '0' && inData[3] <= '9' && inData[4] >= '0' && inData[4] <= '9' && inData[5] >= 'a' && inData[5] <= 'z' );
	}
	return result;
}


VPictureCodec_TIFF::VPictureCodec_TIFF()
{
	SetEncode(TRUE);
	
	SetUTI("public.tiff");
	
#if VERSIONWIN
	fGUID = GUID_ContainerFormatTiff;
#endif

	SetDisplayName("Tiff");
	AppendExtension("tif");
	AppendExtension("tiff");
	AppendMimeType("image/tiff");
	AppendMimeType("image/x-tiff");
	AppendQTExporterCompentType('TIFF');

	uBYTE signature[] = { 0x49, 0x49, 0x2A, 0x00 };
	uBYTE signature1[] = {0x4D, 0x4D, 0x00, 0x2A};
	
	AppendPattern(sizeof(signature),signature);
	AppendPattern(sizeof(signature1),signature1);
	
	SetPrivateScrapKind(SCRAP_KIND_PICTURE_TIFF);
}
VPictureData* VPictureCodec_TIFF::_CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder) const
{
	#if VERSIONMAC
	return new VPictureData_CGImage(&inDataProvider);
	#elif VERSIONWIN
	return new VPictureData_GDIPlus(&inDataProvider,inRecorder);
	#elif VERSION_LINUX
	return new VPictureData_NonRenderable(&inDataProvider,inRecorder);	
	#endif
}

#if VERSION_LINUX

VError VPictureCodec_TIFF::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet) const
{
	return _GenericEncodeUnknownData(inData,inSettings,outBlob,inSet);
}
VError VPictureCodec_TIFF::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet) const
{
	return _GenericEncodeUnknownData(inData,inSettings,inFile,inSet);
}

#else

VError VPictureCodec_TIFF::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet) const
{
	VError result=VE_OK;
	VPictureDrawSettings	set(inSet);
	#if VERSIONWIN
	//try to encode with WIC codec first
	result=VPictureCodec_WIC::_Encode(static_cast<const VPictureCodec *>(this),inData,inSettings,outBlob,&set);
	
	//backfall to gdiplus codec encoder if WIC encoder failed
	//but not because of encoding modified metadatas
	//(VPictureCodec_WIC::Encode return VE_INVALID_PARAMETER only if 
	// codec failed to encode modified metadatas: if it is the case,
	// we come from 4D command SET PICTURE METADATA and we must return this error
	// and do not try to encode further)
	if (result != VE_OK && result != VE_INVALID_PARAMETER)
		result=_GDIPLUS_Encode("image/tiff",inData,inSettings,outBlob,inSet);
	#else
	result=VPictureCodec_ImageIO::_Encode(fUTI,inData,inSettings,outBlob,inSet);
	#endif
	return result;
}
VError VPictureCodec_TIFF::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet) const
{
	VError result=VE_OK;
	VBlobWithPtr blob;
	result=Encode(inData,inSettings,blob,inSet);
	if(result==VE_OK)
	{
		result=CreateFileWithBlob(blob,inFile);
	}
	return result;
}

#endif

VPictureCodec_ICO::VPictureCodec_ICO()
{
	SetUTI("com.microsoft.ico");
}

VPictureData* VPictureCodec_ICO::_CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder) const
{
	assert(false);
	return new VPictureData(&inDataProvider,inRecorder);
}

VPictureCodec_EMF::VPictureCodec_EMF()
{
	SetEncode(VERSIONWIN);
	
	SetUTI("com.microsoft.emf");
	
	SetDisplayName("Windows Metafile");
	AppendExtension("emf");
	AppendMimeType("image/x-emf");
	
	SetPlatform(2);
	SetScrapKind(SCRAP_KIND_PICTURE_EMF,SCRAP_KIND_PICTURE_EMF);
}

VPictureData* VPictureCodec_EMF::_CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder) const
{
	#if VERSIONMAC
	return new VPictureData_NonRenderable(&inDataProvider,inRecorder);
	#elif VERSIONWIN
	return new VPictureData_GDIPlus_Vector(&inDataProvider,inRecorder);
	#elif VERSION_LINUX
	return new VPictureData_NonRenderable(&inDataProvider,inRecorder);	
	#endif
}

#if VERSION_LINUX

VError VPictureCodec_EMF::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet) const
{
	return _GenericEncodeUnknownData(inData,inSettings,outBlob,inSet);
}
VError VPictureCodec_EMF::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet) const
{
	return _GenericEncodeUnknownData(inData,inSettings,inFile,inSet);
}

#else

VError VPictureCodec_EMF::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet) const
{
	#if VERSIONWIN
	VError result;
	inData.Retain();
	Gdiplus::Metafile* im=0;
	HENHMETAFILE meta;

	VPictureDrawSettings set(inSet);

	//here we need to deactivate layer usage
	//to avoid exporting layer graphic contexts bitmaps as raw data in metafile
	set.DeviceCanUseLayers(false);

	im=inData.CreateGDIPlus_Metafile(&set);
	if(im)
	{
		meta=im->GetHENHMETAFILE();
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
						size_t outSize=buffsize;
						result=outBlob.PutData(buffer,outSize,0);
					}
					delete[] buffer;
				}
			}
			DeleteEnhMetaFile(meta);
		}
		delete im;
	}
	inData.Release();
	return result;
	#else
	return VE_UNIMPLEMENTED;
	#endif
}

VError VPictureCodec_EMF::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet) const
{
	VError result=VE_OK;
	#if VERSIONWIN
	VBlobWithPtr blob;
	result=Encode(inData,inSettings,blob,inSet);
	if(result==VE_OK)
	{
		result=CreateFileWithBlob(blob,inFile);
	}
	#endif
	return result;
}
#endif

bool VPictureCodec_EMF::ValidateData(VFile& inFile)const
{
	bool ok=false;
	
	xENHMETAHEADER* header;
	
	if(inFile.Exists())
	{
		sLONG8 fsize=0;
		inFile.GetSize(&fsize);

		if(fsize>=sizeof(xENHMETAHEADER))
		{
			VFileDesc* desc=NULL;
			if(inFile.Open( FA_READ, &desc, false)==VE_OK)
			{
				long buffsize=sizeof(xENHMETAHEADER);
				
				uBYTE* buff=(uBYTE*)new char[buffsize];
				
				if(desc->GetData ( buff, buffsize, 0 )==VE_OK)
				{
					header=(xENHMETAHEADER*)buff;
					if(header->dSignature==xENHMETA_SIGNATURE)
					{
						sLONG8	size=desc->GetSize ();
						if(size==header->nBytes)
						{
							ok=true;
						}
						ok=true;
					}
				}
				
				delete[] buff;
				delete desc;
			}
		}
	}

	return ok;
}
bool VPictureCodec_EMF::ValidateData(VPictureDataProvider& inDataProvider)const
{
	bool ok=false;

	xENHMETAHEADER header;

	if(inDataProvider.GetData(&header,0,sizeof(xENHMETAHEADER)) == VE_OK && header.dSignature==xENHMETA_SIGNATURE)
	{
		ok=true;
	}
	
	return ok;
}

VPictureCodec_VPicture::VPictureCodec_VPicture()
{
	SetEncode(TRUE);
	
	SetUTI("com.4D.4pct");
	
	SetDisplayName("4D Picture");
	AppendExtension("4pct");
	AppendMimeType("application/4DPicture");
	AppendMacType('4PCT');
	
	uBYTE signature[]={'4','P','C','T'};
	uBYTE signature1[]={'T','C','P','4'};
	AppendPattern(sizeof(signature),signature);
	AppendPattern(sizeof(signature1),signature1);
	SetPrivateScrapKind(SCRAP_KIND_PICTURE_4DPICTURE);
}
VError VPictureCodec_VPicture::Encode(const VPicture& inPict,const VValueBag* inSettings,VBlob& outBlob)const
{
	return inPict.SaveToBlob(&outBlob,true);
}
VError VPictureCodec_VPicture::Encode(const VPicture& inPict,const VValueBag* inSettings,VFile& outFile)const
{
	VError result;
	
	VBlobWithPtr blob;
	result = inPict.SaveToBlob(&blob,true);
	if(result==VE_OK)
	{	
		result=	CreateFileWithBlob(blob,outFile);
	}
	return result;
}
VError VPictureCodec_VPicture::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet) const
{
	VError result=VE_OK;
	VPictureDrawSettings	set(inSet);
	VPicture* pict=new VPicture(&inData);
	pict->SetDrawingSettings(set);
	result=pict->SaveToBlob(&outBlob,true);
	delete pict;
	return result;
}
VError VPictureCodec_VPicture::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet) const
{
	VError result=VE_OK;
	VBlobWithPtr blob;
	result=inherited::Encode(inData,inSettings,blob,inSet);
	if(result==VE_OK)
	{	
		result=	CreateFileWithBlob(blob,inFile);
	}
	return result;
}
VPictureData* VPictureCodec_VPicture::_CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder) const
{

	return new VPictureData_VPicture(&inDataProvider,inRecorder);

}

/************************** gdiplus codeclist *****************************/

#if VERSIONWIN

xGDIPlusCodecInfo::xGDIPlusCodecInfo(Gdiplus::ImageCodecInfo* inInfo)
{
	Clsid=inInfo->Clsid;
    FormatID=inInfo->FormatID;
    CodecName=inInfo->CodecName;
    FormatDescription=inInfo->FormatDescription;
    
    VString tmp=inInfo->FilenameExtension;
    tmp.GetSubStrings(L';',fFileExtensions);
    for(long i=1;i<=fFileExtensions.GetCount();i++)
    {
		fFileExtensions.GetValue(tmp,i);
		tmp.Remove(1,2);
		fFileExtensions.FromValue(tmp,i);
    }
    
    tmp=inInfo->MimeType;
    tmp.GetSubStrings(L';',fMimeTypes);
}
xGDIPlusCodecInfo::~xGDIPlusCodecInfo()
{
	
}
const	xGDIPlusCodecInfo* xGDIPlusCodecList::FindByMimeType(const VString& inMimeType)const
{
	long i;
	for( i=0; i < GetCount() ; ++i)
	{
		const xGDIPlusCodecInfo* info=GetNthCodec(i);
		if(info->FindMimeType(inMimeType))
			return info;
	}
	return 0;
}
const	xGDIPlusCodecInfo* xGDIPlusCodecList::FindByMimeType(const WCHAR* inMimeType)const
{
	long i;
	for( i=0; i < GetCount() ; ++i)
	{
		const xGDIPlusCodecInfo* info=GetNthCodec(i);
		if(info->FindMimeType(VString(inMimeType)))
			return info;
	}
	return 0;
}
const	xGDIPlusCodecInfo* xGDIPlusCodecList::FindByGUID(const GUID& inGUID)const
{
	long i;
	for( i=0; i < GetCount() ; ++i)
	{
		const xGDIPlusCodecInfo* info=GetNthCodec(i);
		if(info->IsGUID(inGUID))
		{
			return info;
		}
	}
	return 0;
}

const	xGDIPlusCodecInfo* xGDIPlusCodecList::FindByCLSID(const CLSID& inCLSID)const
{
	long i;
	for( i=0; i < GetCount() ; ++i)
	{
		const xGDIPlusCodecInfo* info=GetNthCodec(i);
		if(info->IsCLSID(inCLSID))
		{
			return info;
		}
	}
	return 0;
}
BOOL	xGDIPlusCodecList::GUIDToClassID(const GUID& format,CLSID& outID)const
{
	GUID tofind=format;
	long i;
	if(::IsEqualGUID(tofind,Gdiplus::ImageFormatMemoryBMP))
	{
		tofind=Gdiplus::ImageFormatPNG;
	}
	for( i=0; i < GetCount() ; ++i)
	{
		const xGDIPlusCodecInfo* info=GetNthCodec(i);
		if(info->IsGUID(tofind))
		{
			info->GetCLSID(outID);
			return true;
		}
	}
	return false;
}

BOOL	xGDIPlusCodecList::FindClassID(Gdiplus::Image& inPict,CLSID& outID) const
{
	GUID format;
	if(inPict.GetRawFormat(&format)==Gdiplus::Ok)
	{
		return GUIDToClassID(format,outID);
	}
	return false;
}

BOOL	xGDIPlusCodecList::Find4DImageKind(Gdiplus::Image& inPict,VString& outKind) const
{
	GUID format;
	outKind=sCodec_none;
	if(inPict.GetRawFormat(&format)==Gdiplus::Ok)
	{
		if(::IsEqualGUID(format,Gdiplus::ImageFormatMemoryBMP))
		{
			outKind=sCodec_membitmap;
		}
		else
		{
			long i;
			for( i=0; i < GetCount() ; ++i)
			{
				const xGDIPlusCodecInfo* info=GetNthCodec(i);
				if(info->IsGUID(format))
				{
					outKind=info->GetDefaultExtension();
					break;
				}
			}
		}
	}
	return outKind!=sCodec_none;
}

xGDIPlusDecoderList::xGDIPlusDecoderList()
:xGDIPlusCodecList()
{
	UINT  num = 0;         
	UINT  size = 0;  
   
	Gdiplus::GetImageDecodersSize(&num, &size);
	if(size != 0)
	{
		Gdiplus::ImageCodecInfo* tmp;
		tmp = ( Gdiplus::ImageCodecInfo*)(malloc(size));
		Gdiplus::GetImageDecoders(num, size, tmp);	
		for(UINT i=0;i<num;i++)	
		{
			fImageCodecInfo.push_back(new xGDIPlusCodecInfo(&tmp[i]));
		}
		free((void*)tmp);
	}
}
xGDIPlusDecoderList::~xGDIPlusDecoderList()
{
}

xGDIPlusEncoderList::xGDIPlusEncoderList()
:xGDIPlusCodecList()
{

	UINT  num = 0;         
	UINT  size = 0;  
   
	Gdiplus::GetImageEncodersSize(&num, &size);
	if(size != 0)
	{
		Gdiplus::ImageCodecInfo* tmp;
		tmp = ( Gdiplus::ImageCodecInfo*)(malloc(size));
		Gdiplus::GetImageEncoders(num, size, tmp);
		for(UINT i=0;i<num;i++)	
		{
			fImageCodecInfo.push_back(new xGDIPlusCodecInfo(&tmp[i]));
		}
		free((void*)tmp);
	}
}
xGDIPlusEncoderList::~xGDIPlusEncoderList()
{
}

#endif

/*************************** lowlevel decoder ****************************/
// convert a DataProvider to blitable bitmap
/*************************************************************************/

#if VERSIONWIN
Gdiplus::Bitmap* VPictureCodec::_CreateGDIPlus_Bitmap(VPictureDataProvider& inDataProvider, Gdiplus::PixelFormat *inPixelFormat)const
{
	//try to decode with WIC first
	Gdiplus::Bitmap* result = VPictureCodec_WIC::Decode( inDataProvider, NULL, inPixelFormat);
	if (result)
		return result;

	//backfall to gdiplus decoder if failed
	VPictureDataProvider_Stream* st=new VPictureDataProvider_Stream(&inDataProvider);
	result=new Gdiplus::Bitmap(st);
	st->Release();
	if(result->GetLastStatus()==Gdiplus::Ok)
		return result;
	else
		return 0;
}
Gdiplus::Metafile* VPictureCodec::_CreateGDIPlus_Metafile(VPictureDataProvider& inDataProvider)const
{
	Gdiplus::Metafile* result;
	VPictureDataProvider_Stream* st=new VPictureDataProvider_Stream(&inDataProvider);
	result=new Gdiplus::Metafile(st);
	st->Release();
	
	if(result->GetLastStatus()==Gdiplus::Ok)
		return result;
	else
		return 0;
}
HBITMAP VPictureCodec::_CreateHBitmap(VPictureDataProvider& /*inDataProvider*/)const
{
	assert(false);
	return NULL;
}
HENHMETAFILE VPictureCodec::_CreateMetafile(VPictureDataProvider& inDataProvider)const
{
	HENHMETAFILE result;
	VPtr data=inDataProvider.BeginDirectAccess();
	if(data)
	{
		result=::SetEnhMetaFileBits((UINT)inDataProvider.GetDataSize(),(uBYTE*)data);
		inDataProvider.EndDirectAccess();
	}
	else
		inDataProvider.ThrowLastError();
	return result;
}
#elif VERSIONMAC
CGImageRef VPictureCodec::_CreateCGImageRef(VPictureDataProvider& inDataProvider)const
{
	return VPictureCodec_ImageIO::Decode( inDataProvider);
}
	
#endif
void* VPictureCodec::_CreateMacPicture(VPictureDataProvider& /*inDataProvider*/)const
{
	assert(false);
	return 0;
}

/*************************************************************/
// use to encapsulate 4d blob in a pict
/*************************************************************/
VPictureCodec_4DVarBlob::VPictureCodec_4DVarBlob()
{
	SetEncode(true);
	SetPrivate(true);
	SetBuiltIn(true);
	
	SetUTI("com.4D.4DVarBlob");
	
	SetDisplayName("4DVarBlob");
	
	AppendExtension("4DVarBlob");
	AppendMimeType("application/4DVarBlob");

	uBYTE signature[]={'B','L','V','R'};
	uBYTE signature1[]={'R','V','L','B'};
	
	AppendPattern(sizeof(signature),signature);
	AppendPattern(sizeof(signature1),signature1);

	SetPlatform(3);
}
VPictureData* VPictureCodec_4DVarBlob::_CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder) const
{
	return new VPictureData_NonRenderable(&inDataProvider,inRecorder);
}

VError VPictureCodec_4DVarBlob::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet) const
{
	XBOX::VError err=VE_UNIMPLEMENTED;
	if(IsEncodedByMe(inData))
	{
		VSize outsize;
		err=inData.Save(&outBlob,0,outsize);
	}
	return err;
}
VError VPictureCodec_4DVarBlob::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet) const
{
	XBOX::VError err=VE_UNIMPLEMENTED;
	if(IsEncodedByMe(inData))
	{
		VBlobWithPtr blob;
		err=Encode(inData,0,blob,inSet);
		if(err==VE_OK)
		{
			err=CreateFileWithBlob(blob,inFile);
		}
	}
	return err;
}
/******************************************************************/
// 
/******************************************************************/
VPictureCodec_UnknownData::VPictureCodec_UnknownData()
{
	SetEncode(true);
	SetBuiltIn(true);
	SetPrivate(true);
	SetPlatform(3);
	SetCanValidateData(false);
}
VPictureCodec_UnknownData::VPictureCodec_UnknownData(const XBOX::VString& inID)
{
	VectorOfVString signArray;
	
	SetEncode(true);
	SetBuiltIn(true);
	SetPrivate(true);
	SetPlatform(3);
	SetCanValidateData(false);
	
	inID.GetSubStrings( ";",signArray);

	for(VectorOfVString::iterator it=signArray.begin();it!=signArray.end();it++)
	{
		const UniChar *p = (*it).GetCPointer();
		if(*p == '.')
		{
			AppendExtension(p+1);
		}
		else
		{
			if((*it).GetLength()==4)
			{
				AppendMacType((*it).GetOsType());
			}
			else
			{
				AppendMimeType((*it));
			}
		}
	}
	SetDisplayName(GetDefaultIdentifier());
}

VPictureCodec_UnknownData::~VPictureCodec_UnknownData()
{
}

bool VPictureCodec_UnknownData::ValidateData(VFile& /*inFile*/)const 
{
	return false;
}

bool VPictureCodec_UnknownData::ValidateData(VPictureDataProvider& /*inDataProvider*/)const 
{
	return false;
}

VPictureData* VPictureCodec_UnknownData::_CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder) const
{
	return new VPictureData_NonRenderable(&inDataProvider,inRecorder);
}

VError VPictureCodec_UnknownData::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet) const
{
	return _GenericEncodeUnknownData(inData,inSettings,outBlob,inSet);
}
VError VPictureCodec_UnknownData::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet) const
{
	return _GenericEncodeUnknownData(inData,inSettings,inFile,inSet);
}

/******************************************************************/
// special codec to recover itkblobpict
/******************************************************************/
VPictureCodec_ITKBlobPict::VPictureCodec_ITKBlobPict()
:VPictureCodec_UnknownData()
{
	
}
VPictureCodec_ITKBlobPict::VPictureCodec_ITKBlobPict(const XBOX::VString& inID)
:VPictureCodec_UnknownData(inID)
{

}

VPictureCodec_ITKBlobPict::~VPictureCodec_ITKBlobPict()
{
}

VPictureData* VPictureCodec_ITKBlobPict::_CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder) const
{
	return new VPictureData_NonRenderable_ITKBlobPict(&inDataProvider,inRecorder);
}

VError VPictureCodec_ITKBlobPict::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet) const
{
	VSize outsize;
	XBOX::VError err=VE_UNIMPLEMENTED;
	VPictureDrawSettings set(inSet);
	if(IsEncodedByMe(inData))
	{
		err=inData.Save(&outBlob,0,outsize);
	}
	else if(inData.IsKind(sCodec_pict))
	{
		err=inData.Save(&outBlob,0,outsize);
		if(err==VE_OK)
		{
			// sawp pictheader	
#if SMALLENDIAN
			sWORD hearder[5];
			outBlob.GetData(&hearder,10,0);
			ByteSwapWordArray(&hearder[0],5);
			outBlob.PutData(&hearder,10,0);
#endif		
			// append picend
			xPictEndBlock endblock;
			endblock.fOriginX=set.GetPicEnd_PosX();
			endblock.fOriginY=set.GetPicEnd_PosY();
			endblock.fTransfer=set.GetPicEnd_TransfertMode();
			
			err=outBlob.PutData(&endblock,sizeof(xPictEndBlock),(VIndex) outsize);
		}
	}
	return err;
}
VError VPictureCodec_ITKBlobPict::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet) const
{
	XBOX::VError err=VE_UNIMPLEMENTED;
	if(IsEncodedByMe(inData) || inData.IsKind(sCodec_pict))
	{
		VBlobWithPtr blob;
		err=Encode(inData,inSettings,blob,inSet);
		if(err==VE_OK)
		{
			err=CreateFileWithBlob(blob,inFile);
		}
	}
	return err;
}



// fake SVG codec

VPictureCodec_FakeSVG::VPictureCodec_FakeSVG()
{
	SetEncode(true);    
	SetDisplayName("Scalable Vector Graphics");
	AppendExtension("svg");
	AppendExtension("svgz");
	AppendMimeType("image/svg+xml");
	#if VERSIONMAC
	SetScrapKind(SCRAP_KIND_PICTURE_PDF,SCRAP_KIND_PICTURE_SVG);
	#else
	SetScrapKind(SCRAP_KIND_PICTURE_EMF,SCRAP_KIND_PICTURE_SVG);
	#endif
}

VPictureCodec_FakeSVG::~VPictureCodec_FakeSVG()
{
}



VPictureData* VPictureCodec_FakeSVG::_CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder) const
{
	return new VPictureData_NonRenderable(&inDataProvider,inRecorder);	
}

VError VPictureCodec_FakeSVG::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet) const
{
	return _GenericEncodeUnknownData(inData,inSettings,outBlob,inSet);
}

VError VPictureCodec_FakeSVG::DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet) const
{
	return _GenericEncodeUnknownData(inData,inSettings,inFile,inSet);
}

bool VPictureCodec_FakeSVG::ValidateData(VFile& inFile)const
{
	XBOX::VString sExt;
	inFile.GetExtension( sExt);
	if (sExt.EqualToString(CVSTR("svg"), false)
		||
		sExt.EqualToString(CVSTR("svgz"), false))
		return true;
	else
		return false;
}

bool VPictureCodec_FakeSVG::NeedReEncode(const VPictureData& inData,const VValueBag* inCompressionSettings,VPictureDrawSettings* inSet)const
{
	return false;
}

void VPictureCodec_FakeSVG::RetainFileKind(VectorOfVFileKind &outFileKind) const
{
	VFileKind *fileKind = NULL;
	outFileKind.clear();
	VStr63 id,tid;
	#if VERSIONMAC
	VString extlist[2]={"svg",""}; // pp temo ACI0064499
	#else
	VString extlist[3]={"svg","svgz",""};
	#endif
	sLONG i=0;
	
	while(!extlist[i].IsEmpty())
	{
		if (fileKind == NULL)
		{
			// no public file kind is available, let's build a private one

			tid = "com.4d.";
			if(!GetDisplayName().IsEmpty())
			{			
				tid += GetDisplayName();
				tid += ".";
			}
			
			id = tid + extlist[i];

			// if an id is already registered, take it, else create a new one and register it
			fileKind = VFileKind::RetainFileKind( id);
			
			if (fileKind == NULL)
			{
				VectorOfFileExtension extensions;
				extensions.push_back( extlist[i]);
				VFileKindManager::Get()->RegisterPrivateFileKind( id, GetDisplayName(), extensions, VectorOfFileKind(0));
				fileKind = VFileKind::RetainFileKind( id);
			}
			
			if(fileKind)
				outFileKind.push_back(fileKind);
			
			ReleaseRefCountable( &fileKind);
		}
		else
		{
			outFileKind.push_back(fileKind);
			ReleaseRefCountable( &fileKind);
		}
		i++;
	}
}

/** return true if buffer has XML data embedded */
bool VPictureCodec_FakeSVG::IsDataXML(const void *inBuffer, VSize inSize)
{
	if (inSize < 20)
		return false;

	char rootXML_Ansi_UTF8[5] = {'<','?','x','m','l'};
	char rootXML_UTF16[10] = {'<',0,'?',0,'x',0,'m',0,'l',0};

	const char *pCar = (const char *)inBuffer;
	for (int i = 0; i < 10; i++, pCar++)
	{
		if (strncmp(pCar, rootXML_Ansi_UTF8, sizeof(rootXML_Ansi_UTF8)) == 0)
			return true;
		else if (strncmp(pCar, rootXML_UTF16, sizeof(rootXML_UTF16)) == 0)
			return true;
	}
	return false;
}

/** return true if buffer has GZIP data embedded */
bool VPictureCodec_FakeSVG::IsDataGZIP(const void *inBuffer, VSize inSize)
{
	unsigned char GZIP_MAGIC_NUMBER[2] = { 0x1F, 0x8B};

	if (inSize < 2)
		return false;

	unsigned char *buf = (unsigned char *)inBuffer;
	return (buf[0] == GZIP_MAGIC_NUMBER[0]
			&&
			buf[1] == GZIP_MAGIC_NUMBER[1]);
}


bool VPictureCodec_FakeSVG::ValidateData(VPictureDataProvider& inDataProvider)const
{
	//xml or gzip data source ?
	char sBuffer[20];
	if (inDataProvider.GetDataSize() >= 20)
	{
		if (inDataProvider.GetData(&sBuffer[0],0,20)==VE_OK)
		{
			return IsDataXML( sBuffer, 20) || IsDataGZIP( sBuffer, 20);
		}
	}
	return false;
}


/*************************************************************************************************/
// helper interface. 
/*************************************************************************************************/

VError VPictureCodecFactory::IPictureHelper_FullExportPicture_GetOutputFileKind(const VValue& inValue,VString& outFileExtension)
{
 return _FullExportPicture(inValue,NULL,&outFileExtension);
}

VError VPictureCodecFactory::IPictureHelper_FullExportPicture_Export(const VValue& inValue,VStream& outPutStream)
{
	return _FullExportPicture(inValue,&outPutStream,NULL);
}

VError VPictureCodecFactory::_BuildVPictureFromValue(const VValue& inValue,VPicture &outPicture)
{
	VError err=VE_UNKNOWN_ERROR;
	
	if(inValue.GetValueKind()==VK_IMAGE)
	{
		outPicture.FromVPicture_Retain(dynamic_cast<const VPicture&>(inValue),false);
		err=VE_OK;
	}
	else if (inValue.GetValueKind()==VK_BLOB)
	{
		VPictureCodecFactoryRef fact;
		const VPictureCodec* codec=fact->RetainDecoderByIdentifier(sCodec_4pct);
		const VBlob& theBlob = dynamic_cast<const VBlob&>(inValue);
		if(codec)
		{
			XBOX::VPictureDataProvider* datasource= XBOX::VPictureDataProvider::Create(theBlob);
			if(datasource)
			{
				if(codec->ValidateData(*datasource))
				{
					VPictureData *data=codec->CreatePictDataFromDataProvider(*datasource);
					if(data)
					{
						outPicture.FromVPictureData(data);
						data->Release();
						err=VE_OK;
					}
				}
				else
					err=VE_INVALID_PARAMETER;
					
				datasource->Release();
			}
			codec->Release();
		}
	}
	return err;
}

VError VPictureCodecFactory::_FullExportPicture(const VValue& inValue,VStream* outPutStream,VString* outExtension)
{
	VPicture thePicture;
	VError err=VE_UNKNOWN_ERROR;
	VPictureCodecFactoryRef fact;
	VString ext="";
	
	
	err = _BuildVPictureFromValue(inValue,thePicture);
	
	if(err!=VE_OK)
		return err;
	
	bool closestream=false;
	
	//***********************************//
	// find the best format for export
	//***********************************//
	
	if(!thePicture.IsNull())
	{
		VPictureDrawSettings	set(&thePicture);
		bool transform = !set.IsIdentityMatrix();
		
		if(thePicture.CountPictureData()==1 && thePicture.GetExtraDataSize()==0 && !transform)
		{
			const VPictureData* data = thePicture.RetainNthPictData(1);
			if(data)
			{
				const VPictureCodec* codec=data->RetainDecoder();
				codec->GetNthExtension(0,ext);
					
				if(!codec->IsPrivateCodec() && !ext.IsEmpty())
				{
					if(outPutStream)
					{
						
						if(!outPutStream->IsWriting())
						{
							err=outPutStream->OpenWriting();
							closestream =(err==VE_OK);
						}
						
						if(err==VE_OK)
						{	
							VPictureDataProvider	*dataprovider = data->GetDataProvider();
							
							if(dataprovider)
							{
								VPtr p=dataprovider->BeginDirectAccess();
								if(p)
								{
									VSize dataSize=dataprovider->GetDataSize();
									
									if(data->IsKind(sCodec_pict))
									{
										char buff[0x200];
										memset(buff,0,0x200);
										err=outPutStream->PutData(buff,0x200);
									}
									if(err==VE_OK)	
										err = outPutStream->PutData(p,dataSize);
									
									dataprovider->EndDirectAccess();
									
								}
								else
								{
									err=dataprovider->GetLastError();
								}
							}
							else
							{
								err=VE_UNKNOWN_ERROR;
							}
						}
						
						if(closestream)
							outPutStream->CloseWriting();
					}
				}
				else
				{
					ext="4PCT";
				}
				codec->Release();
				data->Release();
			}
			else
			{
				err=VE_UNKNOWN_ERROR;
			}
		}
		else
		{
			ext="4PCT";
		}
	}
	else
	{
		ext="4PCT";
	}
	
	if(err==VE_OK && ext=="4PCT" && outPutStream)
	{
		if(!outPutStream->IsWriting())
		{
			err=outPutStream->OpenWriting();
			closestream =(err==VE_OK);
		}
		if(err==VE_OK)
		{
			if(inValue.GetValueKind()==VK_BLOB)
			{
				// pp la source est deja un blob, on l'ecris directement. ca evite la duplication occasion par inPicture.WriteToStream
				const VBlob& theBlob = dynamic_cast<const VBlob&>(inValue);
				VSize bufferSize=1024*8;
				void *buff = XBOX::vMalloc(bufferSize,'BUFF');
				if(buff)
				{
					VSize tot=theBlob.GetSize();
					VIndex pos=0;
					while( (tot > 0) && (err == XBOX::VE_OK))
					{
						VSize chunkSize = (tot > bufferSize) ? bufferSize : tot;
						
						err = theBlob.GetData(buff,chunkSize,pos);
						if(err==VE_OK)
						{
							pos += (VIndex) chunkSize;
							tot -= chunkSize;
							err = outPutStream->PutData(buff,chunkSize);
						}
					}
					XBOX::vFree(buff);
				}
				else
					err = VE_MEMORY_FULL;
			}
			else
			{
				err = thePicture.WriteToStream(outPutStream);
			}
		}
		if(closestream)
			outPutStream->CloseWriting();
	}
	
	
	if(outExtension)
		*outExtension=ext;
	return err;
}

VError VPictureCodecFactory::IPictureHelper_GetVPictureInfo(const VValue& inValue,VString& outMimes,sLONG* outWidth,sLONG* outHeight)
{
	VPicture thePict;
	VError result=VE_OK;
	
	result=_BuildVPictureFromValue(inValue,thePict);
	
	if(result==VE_OK)
	{	
		thePict.GetMimeList(outMimes);
		
		if(!outMimes.IsEmpty() && (outWidth!=NULL || outHeight!=NULL))
		{
			VPoint p=thePict.GetWidthHeight();
			
			if(outWidth!=NULL)
				*outWidth=p.GetX();
			if(outHeight!=NULL)
				*outHeight=p.GetY();
		}
	}
	return result;
}

VError VPictureCodecFactory::IPictureHelper_ExtractBestPictureForWeb(const VValue& inValue,VStream& inOutputStream,const VString& inPreferedTypeList,bool inIgnoreTransform,VString& outMime,sLONG* outWidth,sLONG* outHeight)
{
	VError result=VE_OK;
	VPicture thePict;
	
	VString prefered=inPreferedTypeList;
	VectorOfVString idStrArray;
	bool preferedFound=false;
	
	result=_BuildVPictureFromValue(inValue,thePict);
	
	if(result==VE_OK)
	{
		if(prefered.IsEmpty())
			prefered=".jpeg;.png;.gif;.svg;.pdf";
		
		prefered.GetSubStrings( ";",idStrArray,false,true);	
			
		
		outMime="";
		
		if(outWidth)
			*outWidth=0;
		if(outHeight)
			*outHeight=0;
		
		if(!thePict.IsNull())
		{
			VPictureCodecFactoryRef fact;

			const VPictureCodec*	outputCodec=0;
			VSize					dataSize=0;
			VSize					outDataSize=0;
			const VPictureData		*data;
			VPictureDataProvider	*dataprovider=0;
			VBlobWithPtr			*tmpBlob=0;
			VPictureDrawSettings	set(&thePict);
			bool transform = false;
			
			if(inIgnoreTransform)
				thePict.ResetMatrix();
				
			transform = !set.IsIdentityMatrix();
			
			for(sLONG i=0;i<idStrArray.size();i++)
			{
				data = thePict.RetainPictDataByIdentifier(idStrArray[i]);
				if(data)
				{
					preferedFound=true;
					break;
				}
			}
			if(!data)
			{
				data = thePict.RetainPictDataForDisplay();
				preferedFound=false;
			}
			if(data)
			{
				if(preferedFound)
				{
					// special case for svg/pdf
					if(data->IsKind(sCodec_svg))
					{
						// svg does not support VPicture Transform
						dataprovider = data->GetDataProvider();
						if(!dataprovider)
						{
							// not save yet
							tmpBlob = new VBlobWithPtr();
							result = data->Save(tmpBlob,0,dataSize);
						}
						else
							result = VE_OK;
						transform=false;	
						outputCodec = data->RetainDecoder();
					} 
					else if(data->IsKind(sCodec_pdf))
					{
						dataprovider = data->GetDataProvider();
						outputCodec = data->RetainDecoder();
						if(transform || !dataprovider)
						{
							#if VERSIONMAC
							dataprovider=0;
							tmpBlob = new VBlobWithPtr();
							result = outputCodec->Encode(thePict,NULL,*tmpBlob);
							#else						
							if(dataprovider)
								result = VE_OK;
							#endif
						}
					}
					else
					{
						dataprovider = data->GetDataProvider();
						outputCodec = data->RetainDecoder();
						if(!transform && dataprovider)
						{
							result = VE_OK;
						}
						else
						{
							dataprovider=0;
							tmpBlob = new VBlobWithPtr();
							result = outputCodec->Encode(thePict,NULL,*tmpBlob);
						}
					}
				}
				else
				{
					// si on est ici c'est que le pictdata ne fait pas parti des types prefers.
					// verfification de la compatibilit des codec  input/output
					for(sLONG i=0;i<idStrArray.size();i++)
					{
						outputCodec=fact->RetainEncoderByIdentifier(idStrArray[i]);
						if(outputCodec)
						{
							if(outputCodec->CheckIdentifier(sCodec_svg))
							{
								// svg codec can only save svg source
								if(!data->IsKind(sCodec_svg)) // simple verification
									XBOX::ReleaseRefCountable(&outputCodec);
								else
								{
									// on devrais jamais passer par la car gere dans if(preferedFound)
									break;
								}
							}
							else
							{
								if(outputCodec->CheckIdentifier(sCodec_pdf))
								{
									#if VERSIONWIN
									// cannot generate pdf on windows, if the source is pdf let's use it, otherwiser try the next prefered codec
									if(!data->IsKind(sCodec_pdf)) // simple verification
										XBOX::ReleaseRefCountable(&outputCodec);
									else
									{
										// on devrais jamais passer par la car gere dans if(preferedFound)
										break;
									}
									#endif
								}
								else
								{
									// ras on utilise codec
									break;
								}
							}
						}
					}
					if(outputCodec)
					{
						dataprovider=0;
						tmpBlob = new VBlobWithPtr();
						result = outputCodec->Encode(thePict,NULL,*tmpBlob);
					}
					else
					{
						result=VE_UNKNOWN_ERROR;
					}
				}
			}
			else
			{
				// pas normal
				result=VE_UNKNOWN_ERROR;
			}
			
			if(result==VE_OK)
			{
				if(dataprovider)
				{
					VPtr p=dataprovider->BeginDirectAccess();
					if(p)
					{
						dataSize=dataprovider->GetDataSize();
						result = inOutputStream.PutData(p,dataSize);
						dataprovider->EndDirectAccess();
					}
					else
					{
						result=dataprovider->GetLastError();
					}
				}
				else if(tmpBlob)
				{
					dataSize = tmpBlob->GetSize();
					if(dataSize)
					{
						result = inOutputStream.PutData( tmpBlob->GetDataPtr(), dataSize);
					}
					
				}
				if(outputCodec)
					outputCodec->GetNthMimeType(0,outMime);

			}
			delete tmpBlob;
			
			if(outWidth!=NULL || outHeight!=NULL)
			{
				VPoint p(data->GetWidth(),data->GetHeight());
				if(transform)
					p=thePict.GetPictureMatrix()*p;
				
				if(outWidth)
					*outWidth=p.GetX();
				if(outHeight)
					*outHeight=p.GetY();
			}
			QuickReleaseRefCountable(data);
			ReleaseRefCountable(&outputCodec);
		}
	}
	return result;
}

bool VPictureCodecFactory::IPictureHelper_IsPictureEmpty(VValue* ioValue)
{
	VPicture* vpict = (VPicture*)ioValue;
	return vpict->IsPictEmpty();
}

VError VPictureCodecFactory::IPictureHelper_ReadPictureFromStream(VValue& inValue,VStream& inStream)
{
	VError err=VE_INVALID_PARAMETER;
	
	if(inValue.GetValueKind()==VK_IMAGE)
	{
		VPicture& ioPicture=dynamic_cast<VPicture&>(inValue);
		err = ioPicture.ImportFromStream(inStream);
	}
	return err;
}



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

#ifndef __VPictureDecoder__
#define __VPictureDecoder__

BEGIN_TOOLBOX_NAMESPACE

class _VPictureAccumulator;
class VPicture;
class VPictureData;
class VPictureDataProvider;

XTOOLBOX_API extern const VString sCodec_none;
XTOOLBOX_API extern const VString sCodec_membitmap;
XTOOLBOX_API extern const VString sCodec_memvector;
XTOOLBOX_API extern const VString sCodec_4pct;
XTOOLBOX_API extern const VString sCodec_4dmeta;
XTOOLBOX_API extern const VString sCodec_png;
XTOOLBOX_API extern const VString sCodec_bmp;
XTOOLBOX_API extern const VString sCodec_jpg;
XTOOLBOX_API extern const VString sCodec_tiff;
XTOOLBOX_API extern const VString sCodec_gif;
XTOOLBOX_API extern const VString sCodec_emf;
XTOOLBOX_API extern const VString sCodec_pdf;
XTOOLBOX_API extern const VString sCodec_pict;
XTOOLBOX_API extern const VString sCodec_svg;
XTOOLBOX_API extern const VString sCodec_blob;

XTOOLBOX_API extern const VString sCodec_4DMeta_sign1;
XTOOLBOX_API extern const VString sCodec_4DMeta_sign2;
XTOOLBOX_API extern const VString sCodec_4DMeta_sign3;

class XTOOLBOX_API VQTSpatialSettingsBag :public VObject,public IBaggable
{
	public:
			
	// IBaggable interface
	virtual VError	LoadFromBag( const VValueBag& inBag);
	virtual VError	SaveToBag( VValueBag& ioBag, VString& outKind) const;
	/** ********* ********* **/
	VQTSpatialSettingsBag();
	VQTSpatialSettingsBag(OsType inCodecType,sWORD depth,uLONG inSpatialQuality);
	virtual ~VQTSpatialSettingsBag();
			
	void SetCodecType(OsType inCodecType);
	void SetDepth(sWORD inDepth);
	void SetSpatialQuality(uLONG inCodecQ);
	
	void Reset();

	OsType				GetCodecType() const;
	sWORD				GetDepth() const;
	uLONG				GetSpatialQuality() const;
private:

	OsType				fCodecType;
	sWORD				fDepth;
	uLONG				fSpatialQuality;
};

#if VERSIONWIN

class xGDIPlusCodecInfo :public VObject
{
	public:
	xGDIPlusCodecInfo(Gdiplus::ImageCodecInfo* inInfo);
	virtual ~xGDIPlusCodecInfo();
	bool FindMimeType(const VString& inMime) const
	{
		return fMimeTypes.Find(inMime)>0;
	}
	bool FindExtension(const VString& inExtension) const
	{
		return fFileExtensions.Find(inExtension)>0;
	}
	bool IsGUID(const GUID inGUID) const
	{
		return ::IsEqualGUID(inGUID,FormatID)==1;
	}
	bool IsCLSID(const CLSID inCLSID) const
	{
		return ::IsEqualGUID(inCLSID,Clsid)==1;
	}
	void GetGUID(GUID& outGUID) const
	{
		outGUID=FormatID;
	}
	void GetCLSID(CLSID& outCLSID) const 
	{
		outCLSID=Clsid;
	}
	VString GetDefaultExtension() const
	{
		VString res;
		fFileExtensions.GetValue(res,1);
		return res;
	}
	private:
	CLSID Clsid;
    GUID  FormatID;
    VString CodecName;
    VString FormatDescription;
	VArrayString		fFileExtensions;
	VArrayString		fMimeTypes;
};

class XTOOLBOX_API xGDIPlusCodecList :public VObject
{
	public:
	xGDIPlusCodecList()
	{
		
	}
	virtual ~xGDIPlusCodecList()
	{
		for(size_t i=0;i<fImageCodecInfo.size();i++)
			delete fImageCodecInfo[i];
	}
	long GetCount()const {return (long)fImageCodecInfo.size();}
	const xGDIPlusCodecInfo* GetNthCodec(sLONG inIndex) const
	{
		
		if(inIndex>=0 && inIndex<GetCount())
		{
			return fImageCodecInfo[inIndex];
		}
		return NULL;
	}
	const	xGDIPlusCodecInfo* FindByMimeType(const VString& inMimeType)const;
	const	xGDIPlusCodecInfo* FindByMimeType(const WCHAR* inMimeType)const;
	const	xGDIPlusCodecInfo* FindByGUID(const GUID& inGUID)const;
	const	xGDIPlusCodecInfo* FindByCLSID(const CLSID& inCLSID)const;
	BOOL	GUIDToClassID(const GUID& format,CLSID& outID)const;
	BOOL	Find4DImageKind(Gdiplus::Image& inPict,VString& outKind)const;
	BOOL	FindClassID(Gdiplus::Image& inPict,CLSID& outID)const;
	protected:
	std::vector<xGDIPlusCodecInfo*>	fImageCodecInfo;
	
};
class XTOOLBOX_API xGDIPlusDecoderList :public xGDIPlusCodecList
{
	public:
	xGDIPlusDecoderList();
	virtual ~xGDIPlusDecoderList();
};

class XTOOLBOX_API xGDIPlusEncoderList :public xGDIPlusCodecList
{
	public:
	xGDIPlusEncoderList();
	virtual ~xGDIPlusEncoderList();

};

#endif


class XTOOLBOX_API VPictureCodec : public VObject, public IRefCountable
{
	friend class VPictureData_GDIPlus_Vector;
	public:
	VPictureCodec();
	virtual ~VPictureCodec();
	
	virtual void AutoWash(){};
	
	virtual bool ValidateData(VFile& inFile) const;
	virtual bool ValidateExtention(VFile& inFile)  const;
	
	virtual bool ValidateData(VPictureDataProvider& inDataProvider)  const;
	virtual bool ValidateExtention(const VString& inExt)  const;
	
	
	virtual VPictureData* CreatePictDataFromVFile( const VFile& inFile) const;
	virtual VPictureData* CreatePictDataFromDataProvider(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder=0) const;
	
	virtual VFileKind* RetainFileKind()const;
	virtual void RetainFileKind(VectorOfVFileKind &outFileKind) const;
	
	bool CheckIdentifier(const VString& inExt)const;
	bool CheckExtension(const VString& inExt)const;
	bool CheckMimeType(const VString& inMime)const;
	bool CheckQTExporterComponentType(const OsType inType)const;
	bool CheckMacType(sLONG inExt)const;
	
	/** set accessor on UTI (uniform type identifier) */
	void SetUTI(const VString& inUTI)			{ fUTI = inUTI;	}
	
	/** get accessor on UTI */
	const VString& GetUTI() const				{ return fUTI; }
	
	/** check UTI */
	bool CheckUTI(const VString& inUTI) const	{ return fUTI.EqualToString(inUTI,true); }
	
	virtual bool CanEncodeMetas() const			{ return false; }

#if VERSIONWIN
	/** get accessor on codec container format GUID */ 
	const GUID&	GetGUID () const				{ return fGUID; }
#endif

	//uLONG Get4DType()const {return f4DType;} ;
	const VString& GetDisplayName()const {return fDisplayName;}
	
	VSize CountMimeType()const {return fMimeTypes.GetCount();}
	void GetNthMimeType(VIndex inIndex,VString &outMime)const
	{
		outMime="";
		if(inIndex<fMimeTypes.GetCount())
			outMime=fMimeTypes[inIndex];
	}
	
	VSize CountQTExporterComponentType()const {return fQTExporterComponentType.GetCount();}
	void GetQTExporterComponentType(VIndex inIndex,OsType &outType)const
	{
		outType=0;
		if(inIndex<fQTExporterComponentType.GetCount())
			outType=fQTExporterComponentType[inIndex];
	}

	VSize CountMacType()const {return fFileMacType.GetCount();}
	void GetNthMacType(VIndex inIndex,OsType &outType)const
	{
		outType=0;
		if(inIndex<fFileMacType.GetCount())
			outType=fFileMacType[inIndex];
	}

	VSize CountExtensions()const {return fFileExtensions.GetCount();}
	void GetNthExtension(VIndex inIndex,VString &outExt)const
	{
		outExt="";
		if(inIndex>=0 && inIndex<fFileExtensions.GetCount())
			outExt=fFileExtensions[inIndex];
	}
	
	VString GetDefaultIdentifier() const;
	
	bool IsCross()const {return fPlatform ==3;}
	bool IsMac()const  {return (fPlatform & 1)==1;}
	bool IsWin()const  {return (fPlatform & 2)==2;}
	
	virtual VPictureData* _CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder=0) const =0;
	
	virtual bool ValidatePattern(const uBYTE* inPattern, const uBYTE* inData, sWORD inLen)const;
	void SetDisplayName(const VString& inStr){fDisplayName=inStr;}
	
	void AppendExtension(const VString& inExt);
	void AppendMacType(const sLONG);
	void AppendQTExporterCompentType(const OsType);
	void AppendPattern(sWORD len,const uBYTE* inPattern);
	void SetPictureData(VPictureData* inPictData);
	void AppendMimeType(const VString& inStr);
	
	void SetPlatform(sWORD inPlat){fPlatform=inPlat;}
	void SetCompatibleScrapKind(const VString& inCompatible)
	{
		fCompatibleScrapType=inCompatible;
	}
	void SetPrivateScrapKind(const VString& inPrivate)
	{
		fPrivateScrapType=inPrivate;
	}
	void SetScrapKind(const VString& inCompatible,const VString& inPrivate)
	{
		fPrivateScrapType=inPrivate;
		fCompatibleScrapType=inCompatible;
	}
	void GetScrapKind(VString& outCompatible,VString& outPrivate) const
	{
		outCompatible=fCompatibleScrapType;
		outPrivate=fPrivateScrapType;		
	}
	
	VError Encode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet=NULL)const; 
	VError Encode(const VPictureData& inData,const VValueBag* inSettings,VFile& outFile,VPictureDrawSettings* inSet=NULL)const; 
	
	virtual VError Encode(const VPicture& inPict,const VValueBag* inSettings,VBlob& outBlob)const;
	virtual VError Encode(const VPicture& inPict,const VValueBag* inSettings,VFile& outFile)const;
	
	
	bool IsEncoder()const{return fCanEncode;}	
	bool IsBuiltIn()const{return fIsBuiltIn;}
	bool IsEncodedByMe(const VPictureData& inData) const;
	
	virtual bool NeedReEncode(const VPictureData& inData,const VValueBag* inCompressionSettings,VPictureDrawSettings* inSet)const;
	
	bool IsPrivateCodec()const{return fPrivate;}
	bool CanValidateData()const{return fCanValidateData;}
	void UpdateDisplayNameWithSystemInformation();

	static VError CreateFileWithBlob(VBlobWithPtr& inBlob,VFile& inFile);
	
protected:
	
	#if VERSIONMAC
	CFStringRef MAC_FindPublicUTI( CFStringRef inTagClass, CFStringRef inTag) const;
	#endif
	
	void SetCanValidateData(bool inCanValidate){fCanValidateData=inCanValidate;};
	void SetEncode(bool inEncode){fCanEncode=inEncode;}
	void SetBuiltIn(bool inBuiltin){fIsBuiltIn=inBuiltin;}
	void SetPrivate(bool inPrivate){fPrivate=inPrivate;}
	
	virtual VError DoEncode(const VPictureData& /*inData*/,const VValueBag* /*inSettings*/,VBlob& /*outBlob*/,VPictureDrawSettings* /*inSet=NULL*/)const {return VE_UNIMPLEMENTED;} 
	virtual VError DoEncode(const VPictureData& /*inData*/,const VValueBag* /*inSettings*/,VFile& /*outFile*/,VPictureDrawSettings* /*inSet=NULL*/)const {return VE_UNIMPLEMENTED;} 
	
	// pour factoriser un peu. N'encode le data qui si le codec source & dest est le meme. Utile pour Linux.
	VError	_GenericEncodeUnknownData(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet=NULL)const;
	VError	_GenericEncodeUnknownData(const VPictureData& inData,const VValueBag* inSettings,VFile& outFile,VPictureDrawSettings* inSet=NULL)const;

#if VERSIONWIN
	
	virtual VError					_GDIPLUS_Encode(const VString& inMimeType,const VPictureData& inData,const VValueBag *inSettings,VBlob& outBlob,VPictureDrawSettings* inSet=NULL) const;
	
	virtual Gdiplus::Bitmap*		_CreateGDIPlus_Bitmap(VPictureDataProvider& inDataProvider, Gdiplus::PixelFormat *inPixelFormat = NULL)const;
	virtual Gdiplus::Metafile*		_CreateGDIPlus_Metafile(VPictureDataProvider& inDataProvider)const;
	virtual HBITMAP					_CreateHBitmap(VPictureDataProvider& inDataProvider)const;
	virtual HENHMETAFILE			_CreateMetafile(VPictureDataProvider& inDataProvider)const;
#elif VERSIONMAC
	virtual CGImageRef				_CreateCGImageRef(VPictureDataProvider& inDataProvider)const;
#endif
	virtual void*					_CreateMacPicture(VPictureDataProvider& inDataProvider)const;

	/* uniform type identifier */
	VString	fUTI;					 

#if VERSIONWIN
	/** codec container format GUID */
	GUID fGUID;
#endif

private:

	VString					fDisplayName;
	VArrayOf<VString>		fFileExtensions;
	VArrayOf<sLONG>			fFileMacType;
	VArrayOf<OsType>		fQTExporterComponentType; // for compatibility
	VArrayOf<OsType>		fQTImporterComponentType; // for compatibility
	VArrayOf<VString>		fMimeTypes;
	VArrayOf<uBYTE*>		fSignaturePatterns;
	sWORD					fMaxSignLen;
	
	VString					fPrivateScrapType;
	VString					fCompatibleScrapType;
	
	sWORD					fPlatform; // 3 cross 1 mac 2 win
	
	bool					fIsRender;
	bool					fCanEncode;
	bool					fIsBuiltIn;
	bool					fCanValidateData;
	bool					fPrivate; // si vrai n'apparait pas dans la liste d'export/import et liste des codec 4d.
};

typedef VRefPtr<const VPictureCodec>			VPictureCodecConstRef;
typedef std::vector<VPictureCodecConstRef>		VectorOfVPictureCodecConstRef;

typedef std::vector<VPictureCodec*>				VectorOfPictureCodec;
typedef VectorOfPictureCodec::iterator			VectorOfPictureCodec_Iterator;
typedef VectorOfPictureCodec::const_iterator	VectorOfPictureCodec_Const_Iterator;

#if VERSIONWIN
class VPictureCodec_WIC;
#endif

class XTOOLBOX_API VPictureCodecFactory : public IRefCountable ,public IPictureHelper
{
#if VERSIONWIN
	friend class VPictureCodec_WIC;
#endif

	private:
	VPictureCodecFactory();
	public:
	
	/******** help interface *********/
	
	virtual VError IPictureHelper_ExtractBestPictureForWeb(const VValue& inValue,VStream& inOutputStream,const VString& inPreferedTypeList, bool inIgnoreTransform,VString& outMime,sLONG* outWidth,sLONG* outHeight);	
	virtual VError IPictureHelper_GetVPictureInfo(const VValue& inValue,VString& outMimes,sLONG* outWidth=NULL,sLONG* outHeight=NULL);
	
	virtual VError IPictureHelper_FullExportPicture_GetOutputFileKind(const VValue& inValue,VString& outFileExtension);
	virtual VError IPictureHelper_FullExportPicture_Export(const VValue& inValue,VStream& outPutStream);
	virtual VError IPictureHelper_ReadPictureFromStream(VValue& inValue,VStream& inStream);
	virtual bool IPictureHelper_IsPictureEmpty(VValue* ioValue);
	/********************************/
	
	virtual ~VPictureCodecFactory();
	
	static VPictureCodecFactory* sDecoderFactory;
	
	static void Init()
	{
		if(!sDecoderFactory)
		{
			sDecoderFactory = new VPictureCodecFactory();
			RegisterPictureHelper(sDecoderFactory);
		}
	}
	static void Deinit()
	{
		if(sDecoderFactory)
			sDecoderFactory->Release();
	}
	static VPictureCodecFactory* RetainDecoderFactory()
	{
		if(_GetDecoderFactory())
		{
			sDecoderFactory->Retain();
		}
		else
		{
			assert(false);
		}
		return sDecoderFactory;
	}
	
	void AutoWash(bool inForce=false);

	void GetCodecList(VectorOfVPictureCodecConstRef &outList,bool inEncoderOnly=false,bool inIncludePrivate=false);
	
	void RegisterCodec(VPictureCodec* inCodec);
	void RegisterQuicktimeDecoder(VPictureCodec* inCodec);
	void UnRegisterQuicktimeDecoder();
	bool IsQuicktimeCodec(const VPictureCodec* inCodec);

	const VPictureCodec* RegisterAndRetainUnknownDataCodec(const VString& inIdentifier);

	sLONG CountEncoder()const;
	sLONG CountDecoder()const;
	
	const VPictureCodec* RetainEncoderForVFileKind(const VFileKind& inKind);
	const VPictureCodec* RetainEncoderForMacType(const sLONG inMacType);
	const VPictureCodec* RetainEncoderForMimeType(const VString& inMime);
	const VPictureCodec* RetainEncoderForExtension(const VString& inExt);
	const VPictureCodec* RetainEncoderByIdentifier(const VString& inExt);
	const VPictureCodec* RetainEncoderForQTExporterType(const sLONG inQTType);

	const VPictureCodec* RetainEncoderForUTI(const VString& inUTI);
#if VERSIONWIN	
	const VPictureCodec* RetainEncoderForGUID(const GUID& inGUID);
#endif
	
	const VPictureCodec* RetainDecoderForVFileKind(const VFileKind& inKind);
	const VPictureCodec* RetainDecoderForMacType(const sLONG inMacType);
	const VPictureCodec* RetainDecoderForMimeType(const VString& inMime);
	const VPictureCodec* RetainDecoderForExtension(const VString& inExt);
	const VPictureCodec* RetainDecoderByIdentifier(const VString& inExt);
	const VPictureCodec* RetainDecoderForQTExporterType(const sLONG inQTType);

	const VPictureCodec* RetainDecoderForUTI(const VString& inUTI);
#if VERSIONWIN	
	const VPictureCodec* RetainDecoderForGUID(const GUID& inGUID);
#endif

	const VPictureCodec* RetainDecoder( const VFile& inFile,bool inCheckData);
	const VPictureCodec* RetainDecoder(VPictureDataProvider& inDataProvider);
	const VPictureCodec* RetainDecoder(void* inData,VSize inDataSize);

	bool CheckData(VFile& inFile);
	bool CheckData(VPictureDataProvider& inDataProvider);
	
	void BuildGetFileFilter(VectorOfVFileKind& outList);
	void BuildPutFileFilter(VectorOfVFileKind& outList,const VPictureCodec* inForThisCodec=0);
	VPictureData* CreatePictureDataFromIdentifier(const VString& inID,VPictureDataProvider* inDataProvider,_VPictureAccumulator* inRecorder=0);
	VPictureData* CreatePictureDataFromExtension(const VString& inExt,VPictureDataProvider* inDataProvider=0,_VPictureAccumulator* inRecorder=0);
	VPictureData* CreatePictureDataFromMimeType(const VString& inExt,VPictureDataProvider* inDataProvider=0,_VPictureAccumulator* inRecorder=0);
	VPictureData* CreatePictureData(VFile& inFile);
	VPictureData* CreatePictureData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder=0);
	
	VPictureData* Encode(const VPicture& inPict,const VString& inMimeType,const VValueBag* inEncoderParameter=0);
	VError Encode(const VPicture& inPict,const VString& inMimeType,VPicture& outPict,const VValueBag* inEncoderParameter=0);
	VPictureData* Encode(const VPictureData& inPict,const VString& inMimeType,const VValueBag* inEncoderParameter=0);

	VError EncodeToBlob(const VPicture& inPict,const VString& inMimeType,VBlob& inBlob,const VValueBag* inEncoderParameter=0);
	VError EncodeToBlob(const VPictureData& inPict,const VString& inMimeType,VBlob& inBlob,const VValueBag* inEncoderParameter=0);
	
	VError Encode(const VPicture& inPict,const VFileKind& inKind,VFile& inFile,const VValueBag* inEncoderParameter=0);
	VError Encode(const VPictureData& inPict,const VFileKind& inKind,VFile& inFile,const VValueBag* inEncoderParameter=0);

	VError Encode(const VPicture& inPict,VFile& inFile, const VValueBag* inEncoderParameter=0);
	VError Encode(const VPictureData& inPict,VFile& inFile,const VValueBag* inEncoderParameter=0);
	
	#if VERSIONWIN
	const xGDIPlusDecoderList& xGetGDIPlus_Decoder(){return fGdiPlus_Decoder;}
	const xGDIPlusDecoderList& xGetGDIPlus_Encoder(){return fGdiPlus_Decoder;}
	xGDIPlusDecoderList fGdiPlus_Decoder;
	xGDIPlusEncoderList fGdiPlus_Encoder;
	#endif

	protected:
	
	static VPictureCodecFactory* _GetDecoderFactory()
	{
		Init();	
		return sDecoderFactory;
	}
	
	/**************** Picture Helper ***********/
	VError _BuildVPictureFromValue(const VValue& inValue,VPicture &outPicture);
	VError _FullExportPicture(const VValue& inValue,VStream* outPutStream,VString* outExtension);
	/*******************************************/
	private:
	void _UpdateDisplayNameWithSystemInformation();
	bool _ExtensionExist(const VString& inExt);
	bool _MimeTypeExist(const VString& inMime);

	bool _ExtensionExist_Dyn(const VString& inExt);
	bool _MimeTypeExist_Dyn(const VString& inMime);
	
	void _AppendRetained_BuiltIn(VPictureCodec* inInfo);
	void _AppendRetained_Dyn(VPictureCodec* inInfo);
	void _AppendQTDecoder();

#if VERSIONMAC	
	/** append ImageIO codecs */
	void _AppendImageIOCodecs();
#elif VERSIONWIN
	/** append WIC codecs */
	void _AppendWICCodecs();
#endif
	
	const VPictureCodec* _RetainDecoderForMacType(const VectorOfPictureCodec& inList,const sLONG inMacType);
	const VPictureCodec* _RetainDecoderForMimeType(const VectorOfPictureCodec& inList,const VString& inMime);
	const VPictureCodec* _RetainDecoderForExtension(const VectorOfPictureCodec& inList,const VString& inExt);
	const VPictureCodec* _RetainDecoderForQTExporterType(const VectorOfPictureCodec& inList,const sLONG inQTType);

	const VPictureCodec* _RetainDecoderForUTI(const VectorOfPictureCodec& inList,const VString& inUTI);

#if VERSIONWIN
	const VPictureCodec* _RetainDecoderForGUID(const VectorOfPictureCodec& inList,const GUID& inGUID);
#endif

	VectorOfPictureCodec				fBuiltInCodec;	// thread safe list
	VectorOfPictureCodec				fPlatformCodec;	// thread safe list
	VectorOfPictureCodec				fQTCodec;		// thread safe list
	VectorOfPictureCodec				fDynamicCodec;	// non thread safe list !! use fLock

	mutable VCriticalSection			fLock;		
};

class XTOOLBOX_API VPictureCodecFactoryRef : public VObject
{
	private:
	VPictureCodecFactoryRef& operator=(const VPictureCodecFactoryRef&){assert(false);return *this;}
	VPictureCodecFactoryRef(const VPictureCodecFactoryRef&){assert(false);}
	public:
	VPictureCodecFactoryRef()
	{
		fFactory=VPictureCodecFactory::RetainDecoderFactory();
	}
	virtual ~VPictureCodecFactoryRef()
	{
		fFactory->Release();
	}
	VPictureCodecFactory* operator->()
	{
		return fFactory;
	}
	private:
	VPictureCodecFactory* fFactory;
};

class XTOOLBOX_API VPictureCodec_UnknownData :public VPictureCodec
{
	typedef VPictureCodec inherited;
	public:
	VPictureCodec_UnknownData();
	VPictureCodec_UnknownData(const VString& inIdentifier);
	virtual ~VPictureCodec_UnknownData();
	
	virtual bool ValidateData(VFile& /*inFile*/)const;
	virtual bool ValidateData(VPictureDataProvider& /*inDataProvider*/)const;
	
	virtual VPictureData* _CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder=0) const;

protected:

	virtual VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet=NULL) const;
	virtual VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet=NULL) const;


};

class XTOOLBOX_API VPictureCodec_ITKBlobPict :public VPictureCodec_UnknownData
{
	typedef VPictureCodec_UnknownData inherited;
	public:
	VPictureCodec_ITKBlobPict();
	VPictureCodec_ITKBlobPict(const VString& inIdentifier);
	virtual ~VPictureCodec_ITKBlobPict();
	
	virtual VPictureData* _CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder=0) const;

protected:

	virtual VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet=NULL) const;
	virtual VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet=NULL) const;

};

class XTOOLBOX_API VPictureCodec_InMemBitmap :public VPictureCodec
{
	typedef VPictureCodec inherited;
	public:
	VPictureCodec_InMemBitmap();
	virtual ~VPictureCodec_InMemBitmap(){}
	
	virtual bool ValidateData(VFile& /*inFile*/)const {return false;}
	virtual bool ValidateExtention(VFile& /*inFile*/)const {return false;}
	
	virtual bool ValidateData(VPictureDataProvider& /*inDataProvider*/)const {return false;}
	virtual bool ValidateExtention(const VString& /*inExt*/)const {return false;}
	
	virtual VPictureData* _CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder=0) const;

};

class XTOOLBOX_API VPictureCodec_InMemVector :public VPictureCodec
{
	typedef VPictureCodec inherited;
	public:
	VPictureCodec_InMemVector();
	virtual ~VPictureCodec_InMemVector(){}
	
	virtual bool ValidateData(VFile& /*inFile*/)const {return false;}
	virtual bool ValidateExtention(VFile& /*inFile*/)const {return false;}
	
	virtual bool ValidateData(VPictureDataProvider& /*inDataProvider*/)const {return false;}
	virtual bool ValidateExtention(const VString& /*inExt*/)const{return false;}
	
	virtual VPictureData* _CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder=0) const;

};

class XTOOLBOX_API VPictureCodec_4DMeta :public VPictureCodec
{
	typedef VPictureCodec inherited;
	public:
	VPictureCodec_4DMeta();
	virtual ~VPictureCodec_4DMeta(){}
	
	virtual VPictureData* _CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder=0) const;
};
class XTOOLBOX_API VPictureCodec_JPEG :public VPictureCodec
{
	typedef VPictureCodec inherited;
	public:
	VPictureCodec_JPEG();
	virtual ~VPictureCodec_JPEG(){}
	virtual VPictureData* _CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder=0) const;
	
	virtual bool NeedReEncode(const VPictureData& inData,const VValueBag* inCompressionSettings,VPictureDrawSettings* inSet)const;
	
	bool CanEncodeMetas() const	{ return true; }

	protected:
	virtual VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet=NULL) const;
	virtual VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet=NULL) const;
	
};
class XTOOLBOX_API VPictureCodec_VPicture :public VPictureCodec
{
	typedef VPictureCodec inherited;
	public:
	VPictureCodec_VPicture();
	virtual ~VPictureCodec_VPicture(){}
	virtual VPictureData* _CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder=0) const;
	
	virtual VError Encode(const VPicture& inPict,const VValueBag* inSettings,VBlob& outBlob)const;
	virtual VError Encode(const VPicture& inPict,const VValueBag* inSettings,VFile& outFile)const;
	
	protected:
	virtual VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet=NULL) const;
	virtual VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet=NULL) const;

};
class XTOOLBOX_API VPictureCodec_GIF :public VPictureCodec
{
	typedef VPictureCodec inherited;
	public:
	VPictureCodec_GIF();
	virtual ~VPictureCodec_GIF(){}
	protected:
	virtual bool ValidatePattern(const uBYTE* inPattern, const uBYTE* inData, sWORD inLen)const;
	virtual VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet=NULL) const;
	virtual VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet=NULL) const;

	
	virtual VPictureData* _CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder=0) const;

};
class XTOOLBOX_API VPictureCodec_PNG :public VPictureCodec
{
	typedef VPictureCodec inherited;
	public:
	VPictureCodec_PNG();
	virtual ~VPictureCodec_PNG(){}
	virtual VPictureData* _CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder=0) const;

	bool CanEncodeMetas() const	{ return true; }

	protected:
	virtual VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet=NULL) const;
	virtual VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet=NULL) const;

};

class XTOOLBOX_API VPictureCodec_BMP :public VPictureCodec
{
	typedef VPictureCodec inherited;
	public:
	VPictureCodec_BMP();
	virtual ~VPictureCodec_BMP(){}
	virtual VPictureData* _CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder=0) const;
protected:
	virtual VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet=NULL) const;
	virtual VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet=NULL) const;

};
class XTOOLBOX_API VPictureCodec_TIFF :public VPictureCodec
{
	typedef VPictureCodec inherited;
	public:
	VPictureCodec_TIFF();
	virtual ~VPictureCodec_TIFF(){}
	virtual VPictureData* _CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder=0) const;

	bool CanEncodeMetas() const	{ return true; }

protected:
	virtual VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet=NULL) const;
	virtual VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet=NULL) const;

};

class XTOOLBOX_API VPictureCodec_ICO :public VPictureCodec
{
	typedef VPictureCodec inherited;
	public:
	VPictureCodec_ICO();
	virtual ~VPictureCodec_ICO(){}

	virtual VPictureData* _CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder=0) const;

};
class XTOOLBOX_API VPictureCodec_PICT :public VPictureCodec
{
	typedef VPictureCodec inherited;
	public:
	VPictureCodec_PICT();
	virtual ~VPictureCodec_PICT(){}
	virtual bool ValidateData(VPictureDataProvider& inDataProvider)const;
	virtual bool ValidateData(VFile& inFile)const;

	virtual VPictureData* CreatePictDataFromVFile( const VFile& inFile) const;

	virtual VPictureData* _CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder=0) const;
	protected:
	virtual VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet=NULL) const;
	virtual VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet=NULL) const;

	private:
	void ByteSwapPictHeader(xMacPictureHeaderPtr inHeader)const;
	bool OkHeader(xMacPictureHeaderPtr inHeader)const;

};
class XTOOLBOX_API VPictureCodec_EMF :public VPictureCodec
{
	typedef VPictureCodec inherited;
	public:
	VPictureCodec_EMF();
	virtual ~VPictureCodec_EMF(){}
	
	virtual bool ValidateData(VFile& inFile)const;
	virtual bool ValidateData(VPictureDataProvider& inDataProvider)const;
	virtual VPictureData* _CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder=0) const;
protected:
	virtual VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet=NULL) const;
	virtual VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet=NULL) const;

};

class XTOOLBOX_API VPictureCodec_PDF :public VPictureCodec
{
	typedef VPictureCodec inherited;
	public:
	VPictureCodec_PDF();
	virtual ~VPictureCodec_PDF(){}
		
	virtual VPictureData* _CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder=0) const;
protected:
	virtual VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet=NULL) const;
	virtual VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet=NULL) const;

};

class XTOOLBOX_API VPictureCodec_4DVarBlob :public VPictureCodec
{
	typedef VPictureCodec inherited;
	public:
	VPictureCodec_4DVarBlob();
	virtual ~VPictureCodec_4DVarBlob(){}

	virtual VPictureData* _CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder=0) const;

	protected:

	virtual VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet=NULL) const;
	virtual VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet=NULL) const;

};


// fake SVG codec
class VPictureCodec_FakeSVG :public VPictureCodec
{
	typedef VPictureCodec inherited;
	public:
	VPictureCodec_FakeSVG();
	virtual ~VPictureCodec_FakeSVG();

	bool ValidateData(VFile& inFile)const;
	bool ValidateData(VPictureDataProvider& inDataProvider)const;
	
	class VPictureData* _CreatePictData(VPictureDataProvider& inDataProvider,_VPictureAccumulator* inRecorder=0) const;

	virtual void RetainFileKind(VectorOfVFileKind &outFileKind) const;

	/** return true if buffer has GZIP data embedded */
	static bool IsDataGZIP(const void *inBuffer, VSize inSize);
	/** return true if buffer has XML data embedded */
	
	static bool IsDataXML(const void *inBuffer, VSize inSize);
	
	virtual bool NeedReEncode(const VPictureData& inData,const VValueBag* inCompressionSettings,VPictureDrawSettings* inSet)const;
	
protected:

	VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VBlob& outBlob,VPictureDrawSettings* inSet=NULL) const;
	VError DoEncode(const VPictureData& inData,const VValueBag* inSettings,VFile& inFile,VPictureDrawSettings* inSet=NULL) const;
	
};


END_TOOLBOX_NAMESPACE

#if VERSIONMAC
	#include "VImageIOCodec.h" 
#elif VERSIONWIN
	#include "VWICCodec.h"
#endif

#endif

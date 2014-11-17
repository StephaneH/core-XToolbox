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
#ifndef __VPicture__
#define __VPicture__

#include <vector>
#include <map>
#include <string>

BEGIN_TOOLBOX_NAMESPACE

class VPictureQDBridgeBase;
class VMacHandleAllocatorBase;
class VPictureData;
class xV4DPictureBlobRef;
class VPictureDataProvider;



#if _WIN32 || __GNUC__
#pragma pack(push,4)
#else
#pragma options align = power 
#endif

typedef struct xVPictureBlock
{
	sLONG fKind;
	uLONG fFlags;
	uLONG fHRes;
	uLONG fVRes;
	uLONG fWidth;
	uLONG fHeight;
	sLONG fDataOffset;
	uLONG fDataSize;
}xVPictureBlock, *xVPictureBlockPtr;

typedef struct xVPictureBlockHeaderV1
{
	sLONG fSign;
	sLONG fVersion;
	sLONG fNbPict;
	sLONG fDataSize;
}xVPictureBlockHeaderV1, *xVPictureBlockHeaderV1Ptr;

typedef struct xVPictureBlockHeaderV2
{
	xVPictureBlockHeaderV1 fBase;
	sWORD fPicEnd_PosX;
	sWORD fPicEnd_PosY;
	sWORD fPicEnd_TransfertMode;
}xVPictureBlockHeaderV2, *xVPictureBlockHeaderV2Ptr;

typedef struct xVPictureBlockHeaderV3
{
	xVPictureBlockHeaderV2 fHeaderV2;
	Real fMatrixData[9];
}xVPictureBlockHeaderV3, *xVPictureBlockHeaderV3Ptr;

typedef struct xVPictureBlockHeaderV4
{
	xVPictureBlockHeaderV3 fHeaderV3;
	sWORD fSplitH;
	sWORD fSplitV;
}xVPictureBlockHeaderV4, *xVPictureBlockHeaderV4Ptr;

typedef struct xVPictureBlockHeaderV5
{
	xVPictureBlockHeaderV1 fBase;
	sLONG					fBagSize;
}xVPictureBlockHeaderV5, *xVPictureBlockHeaderV5Ptr;

typedef struct xVPictureBlockHeaderV6
{
	xVPictureBlockHeaderV1 fBase;
	sLONG					fExtensionsBufferSize;
	sLONG					fBagSize;
}xVPictureBlockHeaderV6, *xVPictureBlockHeaderV6Ptr;

typedef struct xVPictureBlockHeaderV7
{
	xVPictureBlockHeaderV1	fBase;
	sLONG					fExtensionsBufferSize;
	sLONG					fBagSize;
	sLONG					fExtraDataSize;
}xVPictureBlockHeaderV7, *xVPictureBlockHeaderV7Ptr;


#if _WIN32 || __GNUC__
#pragma pack(pop)
#else
#pragma options align = reset
#endif



class XTOOLBOX_API VPicture_info : public VValueInfo
{
public:
	VPicture_info( ValueKind inTrueKind = VK_IMAGE):VValueInfo( VK_IMAGE, inTrueKind){;}
	virtual	VValue*					Generate() const;
	virtual	VValue*					Generate(VBlob* inBlob) const;
	virtual	VValue*					LoadFromPtr( const void *inBackStore, bool inRefOnly) const;
	virtual void*					ByteSwapPtr( void *inBackStore, bool inFromNative) const;

	virtual CompareResult			CompareTwoPtrToData( const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean inDiacritical = false) const;
	virtual CompareResult			CompareTwoPtrToDataBegining( const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean inDiacritical = false) const;
	virtual VSize					GetSizeOfValueDataPtr( const void* inPtrToValueData) const;
};

 
typedef std::map < VString , const class VPictureData* > VPictDataMap ;
typedef std::pair < VString , const class VPictureData* > VPictDataPair ;
typedef VPictDataMap::iterator VPictDataMap_Iterrator ;
typedef VPictDataMap::const_iterator VPictDataMap_Const_Iterrator ;

class _VPictureAccumulator :public VObject, public IRefCountable
{
	public:
	_VPictureAccumulator();
	virtual ~_VPictureAccumulator();
	bool FindPictData(const VPictureData* inData,VIndex& outIndex);
	const VPictureData* GetPictureData(VIndex inIndex);
	bool AddPictureData(const VPictureData* inData,VIndex& outIndex);
	VSize GetDataSize();
	private:
	VArrayOf<const VPictureData*> fList;
};

class XTOOLBOX_API VPicture :public VValueSingle
{
	public:
	static	const VPicture_info	sInfo;	
	static void Deinit();
	static void Init(VMacHandleAllocatorBase* inMacAllocator);
	static void InitQDBridge(VPictureQDBridgeBase* inQDBridge);

	VPicture( const VFile& inFile,bool inAllowUnknownData);
	VPicture(const VPictureData* inData);
	VPicture(const VPicture& inData);
	VPicture();
	VPicture(VBlob* inBlob);
	VPicture(VStream* inStream,_VPictureAccumulator* inRecorder=0);
#if !VERSION_LINUX
	VPicture(PortRef inPortRef,VRect inBounds);
#endif
	VPicture& operator=(const VPicture& inPict);
	
	virtual ~VPicture();
	
/*************************** inherited from VValue *******************************/
	
	virtual VError				ReadRawFromStream( VStream* ioStream, sLONG inParam = 0); //used by db4d server to read data without interpretation (for example : pictures)
	virtual Boolean				CanBeEvaluated () const { return false; }
	virtual	const VValueInfo*	GetValueInfo() const {return &sInfo;}
	virtual VPicture*			Clone() const ;
	virtual VValue*				FullyClone(bool ForAPush = false) const; /* no intelligent cloning (for example cloning by refcounting for Blobs)*/
	virtual void				RestoreFromPop(void* context);
	virtual Boolean				FromValueSameKind(const VValue* inValue);
	virtual VValueSingle*		GetDataBlob() const;

	virtual void				Detach(Boolean inMayEmpty = false); /* used by blobs in DB4D */
	virtual CompareResult		CompareTo (const VValueSingle& inValue, Boolean inDiacritic = false) const ;

	virtual	VError				Flush( void* inDataPtr, void* inContext) const;
	virtual void				SetReservedBlobNumber(sLONG inBlobNum);
	virtual void*				WriteToPtr(void* pData,Boolean pRefOnly, VSize inMax = 0 ) const;
	
	virtual VSize				GetFullSpaceOnDisk() const;
	virtual VSize				GetSpace (VSize inMax) const;
	virtual void				FromValue( const VValueSingle& inValue);
	virtual void				GetValue( VValueSingle& outValue) const;

	virtual void				AssociateRecord(void* primkey, sLONG FieldNum);
	virtual void				SetOutsidePath(const VString& inPosixPath, bool inIsRelative = false);
	virtual void				GetOutsidePath(VString& outPosixPath, bool* outIsRelative = NULL);
	virtual void				SetOutsideSuffixe(const VString& inExtention);
	virtual bool				IsOutsidePath();
	virtual VError				ReloadFromOutsidePath();
	virtual bool				ForcePathIfEmpty() const;
	virtual void				SetForcePathIfEmpty(bool x);
	virtual void				SetHintRecNum(sLONG TableNum, sLONG FieldNum, sLONG inHint);

	virtual VError				ReadFromStream (VStream* ioStream, sLONG inParam = 0) ;
	virtual VError				WriteToStream (VStream* ioStream, sLONG inParam = 0) const;
	
	virtual Boolean				EqualToSameKind( const VValue* inValue, Boolean inDiacritical = false) const;
/************************************************************************************************************/
// Debug

			void				DumpPictureInfo(VString& outDump,sLONG level) const;

/*************************************************************************************************************/
// Extra Data	
			void				SetExtraData(const VBlob& inBlob);
			VSize				GetExtraData(VBlob& outBlob)const;
			VSize				GetExtraDataSize()const;
			void				ClearExtraData();

/************************************************************************************************************/	
//	
			void				FromVFile( const VFile& inFile,bool inAllowUnknownData);
			void				FromVPictureData(const VPictureData* inData);
			
			// Si inKeepSettings est faux (cas general) les settings de l'image de destination sont remplacer par ceux de l'image source
			// Si inKeepSettings est vrai, seul les pictdata sont copier. Les transformations, et autres settings de l'image de destination ne sont touché
			// inCopyOutsidePath : dois tj etre FAUX, reservé a DB & javascript. Si vrai l'outside path de la valuesource de l'image de destination sera remplacé

			void				FromVPicture_Copy(const VPicture& inData,bool inKeepSettings,bool inCopyOutsidePath=false);
			void				FromVPicture_Retain(const VPicture& inData,bool inKeepSettings,bool inCopyOutsidePath=false);

			void				FromVPicture_Copy(const VPicture* inData,bool inKeepSettings,bool inCopyOutsidePath=false);
			void				FromVPicture_Retain(const VPicture* inData,bool inKeepSettings,bool inCopyOutsidePath=false);
			
#if !VERSION_LINUX
//TODO - jmo : Voir avec Patrick pour virer le VHandle ?
			void				FromMacHandle(void* inMacHandle,bool trydecode=false,bool inWithPicEnd=false);
#endif

			void				FromPtr(void* inPtr,VSize inSize);
			
			void				FromMacPicHandle(void* inMacPict,bool inWithPicEnd=false);
			void				FromMacPictPtr(void* inMacPtr,VSize inSize,bool inWithPicEnd=false);

			VError				SaveToBlob(VBlob* inBlob,bool inEvenIfEmpty=false,_VPictureAccumulator* inRecorder=0)const;
			VError				ReadFromBlob(VBlob& inBlob);

			VError				ImportFromStream (VStream& ioStream) ;
/********************************************************************************************************/
// Picture data management
			
			bool				AppendPictData(const VPictureData* inData,bool inReplaceIfExist=false,bool inRecalcDefaultPictData=false);
			const				VPictureData* RetainNthPictData(VIndex inIndex) const;
			const				VPictureData* RetainPictDataByMimeType(const VString& inMime) const;
			const				VPictureData* RetainPictDataByExtension(const VString& inExt) const;
			const				VPictureData* RetainPictDataByIdentifier(const VString& inExt) const;
			VIndex				CountPictureData()const {return (VIndex)fPictDataMap.size();}
			void				SelectPictureDataByExtension(const VString& inExt,bool inForDisplay=true,bool inForPrinting=false)const;
			void				SelectPictureDataByMimeType(const VString& inExt,bool inForDisplay=true,bool inForPrinting=false)const;
			
			const VPictureData* RetainPictDataForDisplay() const;
			const VPictureData* RetainPictDataForPrinting() const;

			bool				IsRenderableForDisplay() const;
			bool				IsRenderableForPrinting() const;

			VSize				GetDataSize(_VPictureAccumulator* inRecorder=0) const;	

			void				CalcBestPictData(){_SelectDefault();}
	
			void				GetMimeList(VString& outMimes) const;
	
			const VPictureData* _GetBestPictDataForDisplay()const
			{
				return fBestForDisplay;
			}
			
			const VPictureData* _GetBestPictDataForPrinting()const
			{
				return fBestForPrinting;
			}

			bool				IsSamePictData(const VPicture* inPicture)const;

			
/********************************************************************************************************/
// Picture Info & prop

			sLONG				GetWidth(bool inIgnoreTransform=false) const;
			sLONG				GetHeight(bool inIgnoreTransform=false)const;
			VPoint				GetWidthHeight(bool inIgnoreTransform=false)const;
			VRect				GetCoords(bool inIgnoreTransform=false)const;

			bool				IsSplited(sWORD* ioLine=0,sWORD* ioCol=0)const;
			void				SetSplitInfo(bool inSplit,sWORD inLine,sWORD inCol);
			
			bool				IsPictEmpty() const;
			void				SetEmpty(bool inKeepSettings=false);
			virtual	void					Clear();
			
			sWORD				GetPicEnd_PosX()const{return fDrawingSettings.GetPicEnd_PosX();}
			sWORD				GetPicEnd_PosY()const{return fDrawingSettings.GetPicEnd_PosY();}
			void				SetPicEndPos(sWORD inX,sWORD inY){fDrawingSettings.SetPicEndPos(inX,inY);}
			void				OffsetPicEndPos(sWORD inOffX,sWORD inOffY){fDrawingSettings.OffsetPicEndPos(inOffX,inOffY);}

			void				SetSourceFileName(const VString& inFileName);
			const VString&		GetSourceFileName() const;

			sWORD				GetPicEnd_TransfertMode()const{return fDrawingSettings.GetPicEnd_TransfertMode();}
			void				SetPicEnd_TransfertMode(sWORD inMode){return fDrawingSettings.SetPicEnd_TransfertMode(inMode);}

/******************************************************************************************************/
// transform accessor	
	
			const VAffineTransform& GetPictureMatrix()const{return fDrawingSettings.GetPictureMatrix();}
			void SetPictureMatrix(const VAffineTransform& inMat)
			{
				fDrawingSettings.SetPictureMatrix(inMat);
				_SetValueSourceDirty();
			}
			void ResetMatrix()
			{
				fDrawingSettings.GetPictureMatrix().MakeIdentity();
				_SetValueSourceDirty();
			}
			void MultiplyMatrix(const VAffineTransform& inMat,VAffineTransform::MatrixOrder inOrder=VAffineTransform::MatrixOrderPrepend)
			{
				fDrawingSettings.GetPictureMatrix().Multiply(inMat,inOrder);
				_SetValueSourceDirty();
			}
			void Translate(GReal sX,GReal sY,VAffineTransform::MatrixOrder inOrder=VAffineTransform::MatrixOrderPrepend)
			{
				fDrawingSettings.GetPictureMatrix().Translate(sX,sY,inOrder);
				_SetValueSourceDirty();
			}
			void Scale(GReal sX,GReal sY,VAffineTransform::MatrixOrder inOrder=VAffineTransform::MatrixOrderPrepend)
			{
				fDrawingSettings.GetPictureMatrix().Scale(sX,sY,inOrder);
				_SetValueSourceDirty();
			}

/********************************************************************************************************/
// Draw Settings

			VPictureDrawSettings_Base&			GetSettings(){return fDrawingSettings;}
			const VPictureDrawSettings_Base&	GetSettings()const{return fDrawingSettings;}
	
			void SetDrawingSettings(const VPictureDrawSettings_Base& inSet)
			{
				fDrawingSettings.FromVPictureDrawSettings(&inSet);
			}

/********************************************************************************************************/
// MetaData
			const VValueBag*		RetainMetaDatas(const VString* inPictureIdentifier=NULL);
			void					GetKeywords( VString& outKeywords);
			bool					IsKeyWordInMetaData(const VString& keyword, VIntlMgr* inIntlMgr);

/********************************************************************************************************/
// Draw	
#if !VERSION_LINUX	
			void Draw(PortRef inPortRef,const VRect& r,class VPictureDrawSettings* inSet=NULL)const;
			void Draw(VGraphicContext* inGraphicContext,const VRect& r,class VPictureDrawSettings* inSet=NULL)const;
			void Print(PortRef inPortRef,const VRect& r,class VPictureDrawSettings* inSet=NULL)const;
			void Print(VGraphicContext* inGraphicContext,const VRect& r,class VPictureDrawSettings* inSet=NULL)const;
#endif
	
#if VERSIONWIN
			void Draw(Gdiplus::Graphics* inPortRef,const VRect& r,class VPictureDrawSettings* inSet=NULL)const;
#elif VERSIONMAC
			void Draw(CGContextRef inPortRef,const VRect& r,class VPictureDrawSettings* inSet=NULL)const;
#endif

/********************************************************************************************************/
// Misc & utilities

			static bool IsVPictureData(uCHAR* inBuff,VSize inBuffSize);
			static VFile* SelectPictureFile();

			static VPicture*	EncapsulateBlob(const VBlob* inBlob,bool inOwnData);
			VError				ExtractBlob(VBlob& outBlob);

			void		RemovePastablePicData();

#if WITH_VMemoryMgr
			void*		GetPicHandle(bool inAppendPicEnd=false)const;
#endif

#if !VERSION_LINUX
			VPicture*	CreateGrayPicture()const;
#endif

			sLONG		_GetMaxPictDataRefcount();

			VError	GetPictureForRTF(VString& outRTFKind,VBlob& outBlob);

#if !VERSION_LINUX
			VPicture* BuildThumbnail(sLONG inWidth,sLONG inHeight,PictureMosaic inMode,bool inNoAlpha=false,const VColor& inColor=VColor(255,255,255,255))const;
#endif

protected:

	virtual void					DoNullChanged();

	       
private:
	
	void _Init(bool inSetNULL=false);

	void	_AttachVValueSource(VBlob* inBlob);
	void	_DetachVValueSource(bool inSetNull=true);
	VError	_LoadFromVValueSource();
	VError	_UpdateVValueSource()const;
	void	_SetValueSourceDirty(bool inSet=true)const	{fVValueSourceDirty=inSet;};
	bool	_IsValueSourceDirty()const					{return fVValueSourceDirty;};
	

	void _UpdateNullState();
	
	void _ReadHeaderV1(xVPictureBlockHeaderV1& inHeader,bool inSwap);
	
	bool _Is4DMetaSign(const VString& instr);

	void _ReadExtraInfoBag(VValueBag& inBag);
	VError _SaveExtraInfoBag(VPtrStream& outStream) const;
	VError _SaveExtensions(VPtrStream& outStream) const;
	bool _FindPictData(const VPictureData* inData) const;
	void _ResetExtraInfo();
	void _CopyExtraInfo(const VPicture& inPict);
	void _ResetTransferMode();
	void _ReleaseExtraData();
	
	void _CopyExtraData_Copy(const VPicture& inPict);
	void _CopyExtraData_Retain(const VPicture& inPict);
	
	VSize _CountPictData()const {return fPictDataMap.size();}
	VSize _CountSaveable()const;
	VError _FromStream(VStream* inStream,_VPictureAccumulator* inRecorder=0);
	
	VError _BuildData(xVPictureBlock* inData,sLONG version,std::vector<VString>& inExtList, sLONG nbPict,VStream& inStream,VIndex inStartpos,_VPictureAccumulator* inRecorder=0);
	
	VError _Save(VBlob* inBlob,bool inEvenIfEmpty=false,_VPictureAccumulator* inRecorder=0) const;
	
	void _FullyClone(const VPicture& inData,bool ForAPush);
	
	VSize _GetDataSize(_VPictureAccumulator* inRecorder=0) const;
	void _ClearMap();
	void _CopyMap_Retain(const VPictDataMap& inSrcMap);
	void _CopyMap_Copy(const VPictDataMap& inSrcMap);
	bool _CheckBlobRefCount() const;
	bool _FindMetaPicture() const;
	
	void _SelectDefault();
	const VPictureData* _GetDefault()const;
	
	const VPictureData* _GetBestForDisplay();
	const VPictureData* _GetBestForPrinting();
	
	const VPictureData* _GetPictDataByIdentifier(const VString& inID) const;
	const VPictureData* _GetPictDataByMimeType(const VString& inMime)const;
	const VPictureData* _GetPictDataByExtension(const VString& inExt)const;
	const VPictureData* _GetNthPictData(VIndex inIndex) const;
	
	bool								fIs4DBlob;
	bool								fInCVCache;
	
	mutable XBOX::VBlob*				fVValueSource;
	bool								fVValueSourceLoaded;
	mutable bool						fVValueSourceDirty;								
	
	mutable VPictureDataProvider*		fExtraData;
	
	const mutable VPictureData*			fBestForDisplay;
	const mutable VPictureData*			fBestForPrinting;
	
	XBOX::VString						fSourceFileName;
	XBOX::VString						fOutsidePath;
	
	VPictureDrawSettings_Base			fDrawingSettings;
	
	VPictDataMap						fPictDataMap;

#if VERSIONDEBUG
	static sLONG						sCountPictsInMem;
#endif
};

END_TOOLBOX_NAMESPACE

#endif

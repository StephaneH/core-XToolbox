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
#ifndef __VPictureData__
#define __VPictureData__

#if VERSIONMAC
struct QDPict;
#endif

BEGIN_TOOLBOX_NAMESPACE

#include "VColor.h"

#define xGifInfo_MaxColors 256
#define xGifInfo_MaxColorMapSize 256
#define xGifInfo_CM_RED 0
#define xGifInfo_CM_GREEN 1
#define xGifInfo_CM_BLUE 1
#define xGifInfo_MAX_LWZ_BITS 12
#define xGifInfo_INTERLACE 0x40
#define xGifInfo_LOCALCOLORMAP 0x80

#define xGifInfo_BitSet(byte, bit)	(((byte) & (bit)) == (bit))
#define xGifInfo_LM_to_uint(a,b)		(((b)<<8)|(a))

typedef struct 
{
   sWORD		transparent;
   sWORD		delayTime;
   sWORD		inputFlag;
   sWORD		disposal;
   sWORD		transparent_index;
} xGif89FrameInfo;

class XTOOLBOX_API xGifColorEntry :public VObject
{
	public:
	xGifColorEntry(uBYTE inRed,uBYTE inGreen,uBYTE inBlue)
	{
		fColor.FromRGBAColor(inRed,inGreen,inBlue);
	}
	xGifColorEntry(const VColor &inCol)
	{
		fColor=inCol;
	}
	xGifColorEntry(const xGifColorEntry& inColor)
	{
		fColor=inColor.fColor;
		fUsed=inColor.fUsed;
	}
	virtual ~xGifColorEntry()
	{
		;
	}
	
	VColor	fColor;
	
	bool	fUsed;
};
class XTOOLBOX_API xGifColorPalette :public VObject
{
	public:
	xGifColorPalette()
	{
		;
	}
	xGifColorPalette(const xGifColorPalette& inPal)
	{
		for(long i=0;i<inPal.GetCount();i++)
		{
			fPalette.push_back(inPal[i]);
		}
	}
	virtual ~xGifColorPalette()
	{
	;
	}
	virtual xGifColorEntry operator[](sLONG pIndex) const
	{
		return fPalette[pIndex];
	}
	VColor GetNthEntry(sLONG inIndex)
	{
		return fPalette[inIndex].fColor;
	}
	void AppendColor(VColor inColor)
	{
		fPalette.push_back(xGifColorEntry(inColor));
	}
	void AppendColor(uBYTE inRed,uBYTE inGreen,uBYTE inBlue)
	{
		fPalette.push_back(xGifColorEntry(inRed,inGreen,inBlue));
	}
	sLONG GetCount()const {return (sLONG)fPalette.size();}
	void Reset()
	{
		fPalette.clear();	
	}
	bool IsUsed(sLONG inIndex){return fPalette[inIndex].fUsed;}
	void SetUsedState(bool inval,sLONG inIndex=-1)
	{
		if(inIndex==-1)
		{
			for(long i=0;i<GetCount();i++)
			{
				fPalette[i].fUsed=inval;
			}
		}
		else
		{
			fPalette[inIndex].fUsed=inval;
		}
	}
	typedef std::vector<xGifColorEntry> xGifColorList;
	xGifColorList fPalette;
};
class XTOOLBOX_API xGifFrame :public VObject
{
	private:
	xGifFrame& operator=(xGifFrame&){assert(false);return *this;}
	xGifFrame (xGifFrame&){assert(false);}
	public:
	xGifFrame(sWORD inX,sWORD inY,sWORD inWidth,sWORD inHeight,bool inAllocBitmap);
	virtual ~xGifFrame();
	
	void SetGif89FrameInfo(xGif89FrameInfo& inInfo);
	void FreePixelBuffer();
	uCHAR	*fPixelsBuffer;
	
	VColor GetPaletteEntry(sWORD index);
	
	sWORD fX;
	sWORD fY;
	sWORD fWidth;
	sWORD fHeight;
	xGifColorPalette fPalette;
	sWORD fBitsPerPixel;
	sWORD fColorsTotal;
	sWORD fTransparent;
	sWORD fInterlace;
	bool isGif89;
	xGif89FrameInfo fGif89;
};
class XTOOLBOX_API xGifInfo :public VObject
{
	public:
	xGifInfo();
	virtual void FromVStream(VStream* inStream);
	virtual ~xGifInfo();
	
	sLONG GetFrameCount()const {return (sLONG)fFrameList.size();}
	sWORD GetLoopCount(){return fLoopCount;};
	
	private:
	
	xGifInfo& operator=(const xGifInfo&){assert(false);return *this;}
	xGifInfo(const xGifInfo&){assert(false);}
	
	sWORD ZeroDataBlock;

	virtual bool DoAllocPixBuffer(){return false;}
	
	sWORD ReadOK(void *buffer,sLONG len);
	sWORD ReadColorMap (sWORD number, uCHAR (*buffer)[256]);
	sWORD ReadColorMap (sWORD number, xGifColorPalette& inMap);
	
	sWORD DoExtension ( sWORD label,xGif89FrameInfo* gif89);
	sWORD GetDataBlock (uCHAR *buf);

	sWORD GetCode ( sWORD code_size, sWORD flag);
	sWORD LWZReadByte (sWORD flag, sWORD input_code_size);
	void ReadImage (xGifFrame* im,  sWORD len, sWORD height, uCHAR (*cmap)[256], sWORD interlace, sWORD ignore);
	void ReadImage(xGifFrame* im, sWORD len, sWORD height, xGifColorPalette &inPalette, sWORD interlace, sWORD ignore);

	xGifFrame* gdImageCreate(sWORD sx, sWORD sy,sWORD sw, sWORD sh);
	void gdImageDestroy(xGifFrame* im);
	void gdImageSetPixel(xGifFrame* im, sWORD x, sWORD y, sWORD color);
	sWORD gdImageBoundsSafe(xGifFrame* im, sWORD x, sWORD y);
	
	void _Init(){;}
	typedef sWORD	lzwtable[(1 << xGifInfo_MAX_LWZ_BITS)];
	VStream* fStream;
	
	xGifColorPalette fGlobalPalette;
	
	public:
	typedef std::vector<xGifFrame*> xGifFrameList;
	xGifFrameList fFrameList;
	
	sLONG fWidth;
	sLONG fHeight;
	sWORD fLoopCount;	
};

class VPictureData_MacPicture;
class VPictureData;
class XTOOLBOX_API VPictureQDBridgeBase:public VObject
{
	public:
	VPictureQDBridgeBase(){;}
	virtual ~VPictureQDBridgeBase(){;}
#if VERSIONWIN
	virtual HBITMAP ToHBitmap(const VPictureData_MacPicture& inscrpict) =0;
	virtual HENHMETAFILE ToMetaFile(const VPictureData_MacPicture& inscrpict) =0;
	virtual HENHMETAFILE ToMetaFile(void* inPicHandle)=0;
	virtual Gdiplus::Metafile* ToGdiPlusMetaFile(void* inPicHandle)=0;
	virtual Gdiplus::Metafile* ToGdiPlusMetaFile(const VPictureData_MacPicture& inscrpict)=0;
	
	virtual void* DibToPicHandle(DIBSECTION* inDIB)=0;
	virtual void* HBitmapToPicHandle(HBITMAP inBm)=0;
#elif VERSIONMAC
	
	virtual class VMacQDPortBridge* NewQDMacPort(XBOX::PortRef inPortRef)=0;
	
	virtual void DrawInPortRef(const VPictureData &inCaller,CGImageRef inPict,PortRef inPortRef,const VRect& inBounds,VPictureDrawSettings* inSet=NULL)=0;
	void DrawInCGContext(const VPictureData &inCaller,CGImageRef inPict,ContextRef inContextRef,const VRect& inBounds,VPictureDrawSettings* inSet=NULL);
	
	virtual void DrawInPortRef(const VPictureData &inCaller,CGPDFDocumentRef  inPict,PortRef inPortRef,const VRect& inBounds,VPictureDrawSettings* inSet=NULL)=0;
	void DrawInCGContext(const VPictureData &inCaller,CGPDFDocumentRef  inPict,ContextRef inContextRef,const VRect& inBounds,VPictureDrawSettings* inSet=NULL);
	
	#if ARCH_32
	virtual void DrawInPortRef(const VPictureData &inCaller,xMacPictureHandle inPict,PortRef inPortRef,const VRect& inBounds,VPictureDrawSettings* inSet=NULL)=0;
	void DrawInCGContext(const VPictureData &inCaller,xMacPictureHandle inPict,ContextRef inContextRef,const VRect& inBounds,VPictureDrawSettings* inSet=NULL);

	virtual void DrawInPortRef(const VPictureData &inCaller,struct ::QDPict* inPict,PortRef inPortRef,const VRect& inBounds,VPictureDrawSettings* inSet=NULL)=0;
	void DrawInCGContext(const VPictureData &inCaller,struct ::QDPict* inPict,ContextRef inContextRef,const VRect& inBounds,VPictureDrawSettings* inSet=NULL);
	#endif // #if ARCH_32
	
#endif
	virtual void* VPictureDataToPicHandle(const class VPictureData& inData,VPictureDrawSettings* inSet=NULL)=0;

#if !VERSION_LINUX
	virtual void DrawInMacPort(PortRef inPortRef,const VPictureData_MacPicture& inscrpict,xMacRect* inRect,bool inTrans,bool inRep)=0;
#endif
};

class VPictureDrawSettings;
class _VPictureAccumulator;

class XTOOLBOX_API VPictureData :public VObject, public IRefCountable IWATCHABLE_NOREG(VPictureData)
{

public:
	
	friend class VPictureQDBridgeBase;
	static VPictureQDBridgeBase* sQDBridge;
	
	static void InitMemoryAllocator(VMacHandleAllocatorBase *inAlloc)
	{
		sMacAllocator=inAlloc;
		if(sMacAllocator)
			sMacAllocator->Retain();
	}

	static void InitQDBridge(VPictureQDBridgeBase* inBridge)
	{
		sQDBridge=inBridge;
	}

	static void DeinitStatic()
	{
		if(sQDBridge)
			delete sQDBridge;
		if(sMacAllocator)
			sMacAllocator->Release();
		sQDBridge=0;
		sMacAllocator=0;
	}

	VPictureData(class VPictureDataProvider* inDataProvider,_VPictureAccumulator* inRecorder);
	virtual ~VPictureData();
	
	/** transform input bounds from image space to display space
	    using specified drawing settings 

		@param ioBounds
			input: source bounds in image space
			output: dest bounds in display space
		@param inBoundsDisplay
			display dest bounds
		@param inSet
			drawing settings
	*/
#if !VERSION_LINUX
	void TransformRectFromImageSpaceToDisplayBounds( XBOX::VRect& ioBounds, const XBOX::VRect& inBoundsDisplay, const VPictureDrawSettings *inSet) const;
#endif

#if !VERSION_LINUX
	bool MapPoint(XBOX::VPoint& ioPoint, const XBOX::VRect& inDisplayRect,VPictureDrawSettings* inSet=NULL)const;
#endif

/************************************************************************************/
// Debug
/***********************************************************************************/

	virtual void DumpPictureInfo(VString& outDump,sLONG level)const;

/************************************************************************************/
// Draw
/************************************************************************************/
	
#if !VERSION_LINUX
	virtual void DrawInPortRef(PortRef inPortRef,const VRect& /*r*/,VPictureDrawSettings* /*inSet=NULL*/)const{;}
	virtual void DrawInVGraphicContext(class VGraphicContext& /*inDC*/,const VRect& /*r*/,VPictureDrawSettings* /*inSet=NULL*/)const;
#endif   

#if VERSIONWIN
	virtual void DrawInGDIPlusGraphics(Gdiplus::Graphics* /*inDC*/,const VRect& /*r*/,VPictureDrawSettings* /*inSet=NULL*/)const{;}
	#if ENABLE_D2D
	virtual void DrawInD2DGraphicContext(VWinD2DGraphicContext* /*inDC*/,const VRect& /*r*/,VPictureDrawSettings* /*inSet=NULL*/)const {;};
	#endif
#elif VERSIONMAC
	virtual void DrawInCGContext(CGContextRef /*inDC*/,const VRect& /*r*/,VPictureDrawSettings* /*inSet=NULL*/)const{;}
#endif
	
/************************************************************************************/
// Save Export
/************************************************************************************/	
	
	virtual VError Save(VBlob* inData,VIndex inOffset,VSize& outSize,class _VPictureAccumulator* inRecorder=NULL)const;
	/** save picture data source to the specified file 
	@remarks
		called from VPictureCodec::Encode 
		this method can be derived in order to customize encoding with file extension for instance
	*/
	virtual VError SaveToFile(VFile& outFile) const;

/************************************************************************************/
// Props
/************************************************************************************/	
	
	virtual VSize GetDataSize(_VPictureAccumulator* inRecorder=0)const ;
	virtual sLONG GetWidth() const;
	virtual sLONG GetHeight() const;
	virtual sLONG GetX()const;
	virtual sLONG GetY()const;
	virtual const VRect& GetBounds()const ;
	
	VString GetEncoderID()const;
	bool	IsKind(const VString& inKind)const;
	const class VPictureCodec* RetainDecoder() const;

	virtual bool IsRenderable()const{return true;}

	virtual sLONG GetFrameCount()const {return 1;}

	virtual Boolean IsVector()const{return false;}
	virtual Boolean IsRaster()const{return false;}

/************************************************************************************/
// Data Providers
/************************************************************************************/	

	void SetDataProvider(VPictureDataProvider* inRef,bool inSetDirty=true);
	VPictureDataProvider* GetDataProvider() const;
	bool SameDataProvider(VPictureDataProvider* inRef);


/************************************************************************************/
// Misc
/************************************************************************************/	

	static VPictureData* CreatePictureDataFromVFile(VFile& inFile);

#if !VERSION_LINUX
	static VPictureData* CreatePictureDataFromPortRef(PortRef inPortRef,const VRect& inBounds);
#endif
	
	static void SetMacAllocator(VMacHandleAllocatorBase *inAlloc)
	{
		sMacAllocator=inAlloc;
	}
	static VMacHandleAllocatorBase* GetMacAllocator()
	{
		return sMacAllocator;
	}
	virtual VPictureData* Clone()const;


	virtual	sLONG		Retain(const char* DebugInfo = 0) const;
	virtual	sLONG		Release(const char* DebugInfo = 0) const;


	virtual void BuildTimeLine( std::vector< sLONG > &inTimeLine , sLONG& /*outLoopCount*/) const
	{
		inTimeLine.clear();
	}

/************************************************************************************/
// Native picture
/************************************************************************************/

// pp warning
// les fonction get ci dessous retourne le pointeur sur l'intance dessinable de l'image, ATTENTION ce n'est pas une copie
// pour obtenir une copie d'un type x, qq soit le type de la source il faut utiliser Create... c'est a lutilisateur de lobject reourn de le dispos

#if VERSIONWIN
	virtual HENHMETAFILE		GetMetafile()const{return NULL;}
	virtual HBITMAP				GetHBitmap() const{return NULL;}
	virtual Gdiplus::Bitmap*	GetGDIPlus_Bitmap()const{return NULL;}
	virtual Gdiplus::Metafile*	GetGDIPlus_Metafile() const{return NULL;}
	virtual Gdiplus::Image*		GetGDIPlus_Image()const ;
	
	virtual HENHMETAFILE		CreateMetafile(VPictureDrawSettings* inSet=NULL)const;
	virtual HBITMAP				CreateHBitmap(VPictureDrawSettings* inSet=NULL)const;
	virtual HBITMAP				CreateTransparentHBitmap(VPictureDrawSettings* inSet = NULL)const;
	virtual HICON				CreateHIcon(VPictureDrawSettings* inSet=NULL)const;
			Gdiplus::Bitmap*	CreateGDIPlus_SubBitmap(VPictureDrawSettings* inSet,VRect* inSub)const;
	virtual Gdiplus::Bitmap*	CreateGDIPlus_Bitmap(VPictureDrawSettings* inSet)const	{ return CreateGDIPlus_SubBitmap( inSet, NULL);}
	virtual Gdiplus::Metafile*	CreateGDIPlus_Metafile(VPictureDrawSettings* inSet=NULL)const;
	virtual Gdiplus::Image*		CreateGDIPlus_Image(VPictureDrawSettings* inSet=NULL)const; 
#elif VERSIONMAC
	virtual CGImageRef			GetCGImage()const{return NULL;}
	virtual CGPDFDocumentRef	GetPDF()const{return NULL;}
	virtual CGImageRef			CreateCGImage(VPictureDrawSettings* inSet=NULL,VRect* inSub=0)const;
	virtual CGPDFDocumentRef	CreatePDF(VPictureDrawSettings* inSet=NULL,VPictureDataProvider** outDataProvider=0)const;
#endif

/************************************************************************************/
// Mac picture convertion
/************************************************************************************/	

#if WITH_VMemoryMgr
	virtual xMacPictureHandle			GetPicHandle()const{return NULL;}
	virtual xMacPictureHandle			CreatePicHandle(VPictureDrawSettings* inSet,bool& outCanAddPicEnd)const;
#endif

/************************************************************************************/
// Misc Bitmap
/************************************************************************************/		

#if WITH_VMemoryMgr
	virtual VHandle						CreatePicVHandle(VPictureDrawSettings* inSet=NULL)const;
#endif

#if !VERSION_LINUX
	virtual VBitmapData*				CreateVBitmapData(const VPictureDrawSettings* inSet=NULL,VRect* inSub=0)const;
#endif

#if !VERSION_LINUX
	virtual VPictureData*				BuildThumbnail(sLONG inWidth,sLONG inHeight,PictureMosaic inMode,const VPictureDrawSettings* inSet=NULL,bool inNoAlpha=false,const VColor& inColor=VColor(255,255,255,255))const;
#endif

#if !VERSION_LINUX
	VPictureData*						ConvertToGray(const VPictureDrawSettings* inSet=NULL)const;
#endif

#if !VERSION_LINUX
	void								CalcThumbRect(sLONG inWidth,sLONG inHeight,PictureMosaic inMode,VRect& outRect,const VPictureDrawSettings& inSet)const;
#endif

	VPictureData*						CreateSubPicture(VRect& inSubRect,VPictureDrawSettings* inSet=NULL)const;

	/** compare this picture data with the input picture data
	@param inPictData
		the input picture data to compare with
	@outMask
		if not NULL, *outMask will contain a black picture data where different pixels (pixels not equal in both pictures) have alpha = 0.0f
		and equal pixels have alpha = 1.0f
	@remarks
		if the VPictureDatas have not the same width or height, return false & optionally set *outMask to NULL
	*/
#if !VERSION_LINUX
	bool Compare( const VPictureData *inPictData, VPictureData **outMask = NULL) const;
#endif

/************************************************************************************/
// MetaData
/************************************************************************************/		



	/** get and retain bag of metadatas (thread-safe)
	 
	@see
		fMetadatas
	*/
	const VValueBag *RetainMetadatas() const 
	{ 
		_LoadMetadatas();
		fCrit.Lock();
		if (fMetadatas != NULL) 
		{ 
			fMetadatas->Retain(); 
			VValueBag *metas = fMetadatas;
			fCrit.Unlock();
			return metas; 
		}
		fCrit.Unlock();
		return NULL;
	}

protected:

	VPictureData();
	VPictureData(const VPictureData& inData);
	VPictureData& operator =(const VPictureData& /*inData*/){assert(false);return *this;}

#if VERSIONWIN
	void xDraw(Gdiplus::Image* inPict,PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	void xDraw(HENHMETAFILE inPict,PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	void xDraw(HBITMAP inPict,PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	void xDraw(BITMAPINFO* inPict,PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	
	void xDraw(Gdiplus::Image* inPict,Gdiplus::Graphics* inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	void xDraw(HENHMETAFILE inPict,Gdiplus::Graphics* inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	void xDraw(HBITMAP inPict,Gdiplus::Graphics* inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	void xDraw(BITMAPINFO* inPict,Gdiplus::Graphics* inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const;

	#if ENABLE_D2D
	/** draw image in the provided D2D graphic context
	@remarks
		we must pass a VWinD2DGraphicContext ref in order to properly inherit D2D render target resources
		(D2D render target resources are attached to the VWinD2DGraphicContext object)

		this method will fail if called outside BeginUsingContext/EndUsingContext (but is should never occur)

		we cannot use a Gdiplus::Image * here because in order to initialize a D2D bitmap 
		we need to be able to lock inPict pixel buffer which is not possible if inPict is not a Gdiplus::Bitmap
	*/
	void xDraw(Gdiplus::Bitmap* inPict,VWinD2DGraphicContext* inDC,const VRect& r,VPictureDrawSettings* inSet=NULL, bool inUseD2DCachedBitmap = true)const;
	#endif
	
#elif VERSIONMAC
	void xDraw(CGImageRef inPict,PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	void xDraw(CGImageRef inPict,CGContextRef inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	
	void xDraw(CGPDFDocumentRef  inPict,PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	void xDraw(CGPDFDocumentRef  inPict,CGContextRef inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	
	#if ARCH_32
	void xDraw(struct QDPict *inPict,CGContextRef inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	void xDraw(struct QDPict *inPict,PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet=NULL)const;

	void xDraw(xMacPictureHandle inPict,CGContextRef inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	#endif

	void _PrepareCGContext(CGContextRef inDC,const VRect& inBounds,VPictureDrawSettings* inSet=NULL)const;
#endif

#if !VERSION_LINUX
	void xDraw(xMacPictureHandle inPict,PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
#endif
	
	void _ReleaseDataProvider();
	
	virtual void _Reset() const
	{
		fCrit.Lock();
		_DoReset();
		fBounds.SetCoords(0,0,0,0);
		fCrit.Unlock();
	}
	void _SetSafe()const;

	inline void _Load() const
	{
		(const_cast<VPictureData*>(this)->*fLoadProc)();
	}

	virtual void _UnsafeLoad() const
	{
		fCrit.Lock();
		if(fDataSourceDirty)
		{
			_Reset();
			_DoLoad();
			fDataSourceDirty=false;
		}
		_SetSafe();
		fCrit.Unlock();
	}
	virtual void _SafeLoad() const
	{
	;
	}
	virtual void _DoLoad()const
	{
		assert(false);
	}
	virtual void _DoReset() const
	{
		assert(false);
	}
	
#if !VERSION_LINUX
	void					_CalcDestRect(const VRect& inWanted,VRect &outDestRect,VRect &outSrcRect,VPictureDrawSettings& inSet)const;
	VAffineTransform		_MakeScaleMatrix(const VRect& inDisplayRect,VPictureDrawSettings& inSet)const;
	VAffineTransform		_MakeFinalMatrix(const VRect& inDisplayRect,VPictureDrawSettings& inSet,bool inAutoSwapAxis=false)const;
	void					_AjustFlip(VAffineTransform& inMat,VRect* inPictureBounds=0)const;
	void					_ApplyDrawingMatrixTranslate(VAffineTransform& inMat,VPictureDrawSettings& inSet)const;
#endif	

	const class VPictureCodec*		fDecoder;
	mutable VRect					fBounds;
	mutable VCriticalSection		fCrit;
	mutable bool					fDataSourceDirty;
	
	VPictureDataProvider*			fDataProvider;
	
	void					_SetDecoderByMime(const VString& inMime);
	void					_SetDecoderByExtension(const VString& inMime);
	void					_SetAndRetainDecoder(const VPictureCodec* inDecodeur);
	void					_SetRetainedDecoder(const VPictureCodec* inDecodeur);
	void					_ReleaseDecodeur();
	
	
	typedef void (VPictureData:: *_LoadProc)() const;
	mutable _LoadProc fLoadProc;
	
	/** metatadas current loading proc */
	
	mutable _LoadProc fLoadMetadatasProc;
	
	void _InitSafeProc(bool inSafe)
	{
		if(inSafe)
		{
			fLoadProc=&VPictureData::_SafeLoad;
			fLoadMetadatasProc = &VPictureData::_SafeLoadMetadatas;
		}
		else
		{
			fLoadProc=&VPictureData::_UnsafeLoad;
			fLoadMetadatasProc = &VPictureData::_UnsafeLoadMetadatas;
		}
	}

	mutable bool fDataSourceMetadatasDirty;
	
	/** load metadatas */
	inline void _LoadMetadatas() const
	{
		(const_cast<VPictureData*>(this)->*fLoadMetadatasProc)();
	}
	
	/** load metadatas (unsafe load) */
	virtual void _UnsafeLoadMetadatas() const
	{
		fCrit.Lock();
		if(fDataSourceMetadatasDirty)
		{
			//actually load metadatas
			_DoLoadMetadatas();
			fDataSourceMetadatasDirty=false;
		}
		//set safe
		_LoadProc SafeProc=&VPictureData::_SafeLoadMetadatas;
		VInterlocked::ExchangePtr( (uLONG_PTR**)&fLoadMetadatasProc ,*((uLONG_PTR**)(&SafeProc)) );
		fCrit.Unlock();
	}
	virtual void _SafeLoadMetadatas() const
	{
		//do nothing
		;
	}
	virtual void _DoLoadMetadatas()const
	{
		//must be derived for VPictureData implementation which can support metadatas
	}
	
	/** bag container for image metadatas
	 
	@remarks
	    metadatas are stored in dedicated element per metadata block 
	@see
		ImageMetaIPTC
		ImageMetaTIFF
		ImageMetaEXIF
		ImageMetaGPS
	*/
	mutable VValueBag *fMetadatas;

private:
	
	static VMacHandleAllocatorBase* sMacAllocator;
};

class XTOOLBOX_API VPictureData_Bitmap :public VPictureData
{
	typedef VPictureData inherited;
	protected:
	VPictureData_Bitmap();
	VPictureData_Bitmap(const VPictureData_Bitmap& inData);
	VPictureData_Bitmap& operator =(const VPictureData_Bitmap&){assert(false);return *this;}
	public:
	VPictureData_Bitmap(VPictureDataProvider* inDataProvider,_VPictureAccumulator* inRecorder=0);

#if !VERSION_LINUX
	VPictureData_Bitmap(PortRef /*inPortRef*/,const VRect& /*inBounds*/){;}
#endif

	virtual ~VPictureData_Bitmap(){;}
	virtual Boolean IsVector()const{return false;}
	virtual Boolean IsRaster()const{return true;}
	
};

class XTOOLBOX_API VPictureData_Vector :public VPictureData
{
	typedef VPictureData inherited;
	protected:
	VPictureData_Vector();
	VPictureData_Vector(const VPictureData_Vector&);
	VPictureData_Vector& operator =(const VPictureData_Vector&){assert(false);return *this;}
	public:
	VPictureData_Vector(VPictureDataProvider* inDataProvider,_VPictureAccumulator* inRecorder=0);
	virtual ~VPictureData_Vector(){;}
	virtual Boolean IsVector()const{return true;}
	virtual Boolean IsRaster()const{return false;}
};

class XTOOLBOX_API VPictureData_Meta :public VPictureData_Vector
{
	typedef VPictureData_Vector inherited;
	protected:
	VPictureData_Meta();
	VPictureData_Meta(const VPictureData_Meta& inData);
	VPictureData_Meta& operator =(const VPictureData_Meta&){assert(false);return *this;}
	public:

#if !VERSION_LINUX
	virtual void DrawInPortRef(PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
#endif

#if VERSIONWIN
	virtual void DrawInGDIPlusGraphics(Gdiplus::Graphics* inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	#if ENABLE_D2D
	virtual void DrawInD2DGraphicContext(VWinD2DGraphicContext* inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	#endif
#elif VERSIONMAC
	virtual void DrawInCGContext(CGContextRef inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
#endif

	virtual void DumpPictureInfo(VString& outDump,sLONG level)const;
	enum
	{
		k_HSideBySide=0,
		k_VSideBySide=1,
		k_Stack=2
	};
	
	VPictureData_Meta(VPictureDataProvider* inDataProvider,_VPictureAccumulator* inRecorder=0);
	VPictureData_Meta(class VPicture* inPict1,class VPicture* inPict2,sLONG op);
	virtual ~VPictureData_Meta();
	virtual void _DoLoad()const;
	virtual void _DoReset()const;
	virtual VError Save(VBlob* inData,VIndex inOffset,VSize& outSize,_VPictureAccumulator* inRecorder=0)const;
	virtual VSize GetDataSize(_VPictureAccumulator* inRecorder=0) const;
	virtual VPictureData* Clone()const;
	
	bool	FindDeprecatedPictureData(bool inLookForMacPicture,bool inLookForQuicktimeCodec)const;

#if VERSIONWIN
	virtual Gdiplus::Metafile*	CreateGDIPlus_Metafile(VPictureDrawSettings* inSet=NULL) const;
	virtual HENHMETAFILE		CreateMetafile(VPictureDrawSettings* inSet=NULL) const;
	virtual HBITMAP				CreateHBitmap(VPictureDrawSettings* inSet=NULL)const;
	virtual HICON				CreateHIcon(VPictureDrawSettings* inSet=NULL)const;
	virtual Gdiplus::Bitmap*	CreateGDIPlus_Bitmap(VPictureDrawSettings* inSet=NULL)const;	
#elif VERSIONMAC
#endif

private:

#if !VERSION_LINUX
	bool _PrepareDraw(const VRect& inRect,VPictureDrawSettings* inSet,VRect& outRect1,VRect& outRect2,VPictureDrawSettings& outSet1,VPictureDrawSettings& outSet2)const;
#endif

	typedef struct _4DPictureMetaHeaderV1
	{
		sLONG fSign;
		sLONG fVersion;
		sLONG fOperation;
		sLONG fPict1Offset;
		sLONG fPict1Size;
		sLONG fPict2Offset;
		sLONG fPict2Size;
	}_4DPictureMetaHeaderV1;
	typedef struct _4DPictureMetaHeaderV2
	{
		sLONG fSign;
		sLONG fVersion;
		sLONG fOperation;
		sLONG fSamePicture;
		sLONG fPict1Offset;
		sLONG fPict1Size;
		sLONG fPict2Offset;
		sLONG fPict2Size;
	}_4DPictureMetaHeaderV2;
	typedef _4DPictureMetaHeaderV2 _4DPictureMetaHeader;
	void _InitSize()const;
	mutable class VPicture* fPicture1;
	mutable class VPicture* fPicture2;
	mutable sLONG fOperation;
	mutable _VPictureAccumulator* fRecorder;
};

class XTOOLBOX_API VPictureData_VPicture : public VPictureData_Vector
{
	typedef VPictureData_Vector inherited;
	
protected:
	VPictureData_VPicture();
	VPictureData_VPicture(const VPictureData_VPicture& inData);
	VPictureData_VPicture& operator =(const VPictureData_VPicture&){assert(false);return *this;}
	VPictureData_VPicture(const VPicture& inPict);
	
public:

#if !VERSION_LINUX	
	virtual void DrawInPortRef(PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet=NULL)const;	
#endif

#if VERSIONWIN
	virtual void DrawInGDIPlusGraphics(Gdiplus::Graphics* inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	#if ENABLE_D2D
	virtual void DrawInD2DGraphicContext(VWinD2DGraphicContext* inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	#endif
#elif VERSIONMAC
	virtual void DrawInCGContext(CGContextRef inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
#endif
	
	virtual void DumpPictureInfo(VString& outDump,sLONG level)const;
	
	VPictureData_VPicture(VPictureDataProvider* inDataProvider,_VPictureAccumulator* inRecorder=0);
	
	virtual ~VPictureData_VPicture();
	
	virtual sLONG				GetWidth() const;
	virtual sLONG				GetHeight() const;
	virtual sLONG				GetX()const;
	virtual sLONG				GetY()const;
	virtual const VRect&		GetBounds()const ;
	
	virtual void				_DoLoad()const;
	virtual void				_DoReset()const;
	virtual VError				Save(VBlob* inData,VIndex inOffset,VSize& outSize,_VPictureAccumulator* inRecorder=0)const;
	virtual VSize				GetDataSize(_VPictureAccumulator* inRecorder=0) const;
	virtual VPictureData*		Clone()const;
	const VPicture*				GetVPicture()const{_Load();return fPicture;}
	
private:

	mutable class VPicture* fPicture;
};

class XTOOLBOX_API VPictureData_NonRenderable :public VPictureData_Bitmap
{
	typedef VPictureData_Bitmap inherited;
protected:
	
	VPictureData_NonRenderable();
	VPictureData_NonRenderable(const VPictureData_NonRenderable& inData);
	virtual ~VPictureData_NonRenderable();
	VPictureData_NonRenderable& operator =(const VPictureData_NonRenderable&){assert(false);return *this;}

#if WITH_UNKNOWN_CODEC_PICTURE
#if !VERSION_LINUX
	virtual void DrawInPortRef(PortRef inPortRef, const VRect& /*r*/, VPictureDrawSettings* /*inSet=NULL*/)const;
	virtual void DrawInVGraphicContext(class VGraphicContext& /*inDC*/, const VRect& /*r*/, VPictureDrawSettings* /*inSet=NULL*/)const;
#endif   

#if VERSIONWIN
	virtual void DrawInGDIPlusGraphics(Gdiplus::Graphics* /*inDC*/, const VRect& /*r*/, VPictureDrawSettings* /*inSet=NULL*/)const;
#if ENABLE_D2D
	virtual void DrawInD2DGraphicContext(VWinD2DGraphicContext* /*inDC*/, const VRect& /*r*/, VPictureDrawSettings* /*inSet=NULL*/)const;
#endif
#elif VERSIONMAC
	virtual void DrawInCGContext(CGContextRef /*inDC*/, const VRect& /*r*/, VPictureDrawSettings* /*inSet=NULL*/)const;
#endif
#endif

public:
	
	VPictureData_NonRenderable(VPictureDataProvider* inDataProvider,_VPictureAccumulator* inRecorder=0);
	
	virtual bool IsRenderable()const{return false;}

#if !WITH_UNKNOWN_CODEC_PICTURE
	virtual void _DoLoad()const{ ; }
	virtual void _DoReset()const{ ; }
#else
	virtual void _DoLoad()const;
	virtual void _DoReset()const;

private : 

	mutable VPictureData* fInfoPict;

#endif
};

class XTOOLBOX_API VPictureData_NonRenderable_ITKBlobPict :public VPictureData_NonRenderable
{
	typedef VPictureData_NonRenderable inherited;
	
protected:
	
	VPictureData_NonRenderable_ITKBlobPict();
	VPictureData_NonRenderable_ITKBlobPict(const VPictureData_NonRenderable& inData);
	VPictureData_NonRenderable_ITKBlobPict& operator =(const VPictureData_NonRenderable&){assert(false);return *this;}
	
public:
	
	VPictureData_NonRenderable_ITKBlobPict(VPictureDataProvider* inDataProvider,_VPictureAccumulator* inRecorder=0);
	
	virtual bool IsRenderable()const{return false;}
	
#if WITH_VMemoryMgr
	virtual xMacPictureHandle			CreatePicHandle(VPictureDrawSettings* inSet,bool &outCanAddPicEnd)const;
#endif

	virtual void _DoLoad()const{;}
	virtual void _DoReset()const{;}
	
};

class XTOOLBOX_API VPictureData_Animator : public VObject
{
	public:
	VPictureData_Animator();
	
	VPictureData_Animator& operator=(const VPictureData_Animator& inAnimator);
	VPictureData_Animator(const VPictureData_Animator& inAnimator);
	
	virtual ~VPictureData_Animator();
	
	void AttachPictureData(const VPictureData* inPictData);
	void AttachPictureData(const VPictureData* inPictData,sLONG inFrameCount,sLONG inDelay,sLONG inLoopCount);
	
	bool IsPictureDataAttached()
	{
		return fPictData!=0;
	}
	const VPictureData* RetainAttachedPictureData()
	{
		if(fPictData)
		{
			fPictData->Retain();
			return fPictData;
		}
		else
			return 0;
	}

#if !VERSION_LINUX	
	void Draw(PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet=NULL);
	void Draw(sLONG inFrameNumer,PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet=NULL);

	void Draw(VGraphicContext *inGraphicContext,const VRect& r,VPictureDrawSettings* inSet=NULL);
	void Draw(sLONG inFrameNumer,VGraphicContext *inGraphicContext,const VRect& r,VPictureDrawSettings* inSet=NULL);
#endif

	bool Idle();
	bool Start();
	void Stop();
	sLONG GetFrameCount();
	
private:

	void CopyFrom(const VPictureData_Animator& inAnimator);
	
	sLONG									fTimer;
	uLONG									fLastTick;
	sLONG									fCurrentFrame;
	sLONG									fLastFrame;
	const VPictureData*						fPictData;
	std::vector<sLONG>						fTimeLine;
	sLONG									fLoopCount;	
	
	bool									fPlaying;
	sLONG									fCurrentLoop;	
	
	sLONG									fFrameCount; // 0 for true animated gif
	std::vector< VPicture >					fFrameCache;
};

END_TOOLBOX_NAMESPACE

#endif

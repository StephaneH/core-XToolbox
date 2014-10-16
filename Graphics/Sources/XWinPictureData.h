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
#ifndef __XWinPictureData__
#define __XWinPictureData__



BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API xGifInfo_GDIPlus :public xGifInfo
{
	public:
	xGifInfo_GDIPlus(Gdiplus::Bitmap& inPicture);
	virtual ~xGifInfo_GDIPlus();
	virtual bool DoAllocPixBuffer(){return false;}
};

class XTOOLBOX_API VPictureData_GDIBitmap :public VPictureData_Bitmap
{
typedef VPictureData_Bitmap inherited;
protected:
	VPictureData_GDIBitmap();
	VPictureData_GDIBitmap(const VPictureData_GDIBitmap& inData);
	VPictureData_GDIBitmap& operator=(const VPictureData_GDIBitmap& ){assert(false);return *this;}
public:
	
	VPictureData_GDIBitmap(VPictureDataProvider* inDataSource,_VPictureAccumulator* inRecorder=0);
	VPictureData_GDIBitmap(HBITMAP inBitmap);
	virtual ~VPictureData_GDIBitmap();

	virtual void DrawInPortRef(PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	virtual void DrawInGDIPlusGraphics(Gdiplus::Graphics* inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
#if ENABLE_D2D
	virtual void DrawInD2DGraphicContext(VWinD2DGraphicContext* inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
#endif
	virtual VError Save(VBlob* inData,VIndex inOffset,VSize& outSize,_VPictureAccumulator* inRecorder=0)const;

	
	virtual VPictureData* Clone()const;
	//virtual VPictureData* BuildThumbnail(sLONG inWidth,sLONG inHeight,PictureMosaic inMode,VPictureDrawSettings* inSet=NULL);
	virtual HBITMAP			GetHBitmap()const;
protected:
	
	
	virtual void _DoLoad()const;
	virtual void _DoReset()const;
private:
	void _Init();
	void _InitDIBData(BITMAPINFO* inInfo)const;
	mutable BITMAPINFO* fDIBInfo;
	mutable void*		fDIBData;
	mutable HBITMAP		fBitmap;
};


class XTOOLBOX_API VPictureData_GDIPlus :public VPictureData_Bitmap
{
typedef VPictureData_Bitmap inherited;
protected:
	VPictureData_GDIPlus();
	VPictureData_GDIPlus(const VPictureData_GDIPlus& inData);
	VPictureData_GDIPlus& operator=(const VPictureData_GDIPlus&){assert(false);return *this;}
public:
	
	VPictureData_GDIPlus(VPictureDataProvider* inDataSource,_VPictureAccumulator* inRecorder=0);
	VPictureData_GDIPlus(HBITMAP inBitmap);
	VPictureData_GDIPlus(PortRef inPortRef,const VRect inBounds);
	VPictureData_GDIPlus(Gdiplus::Bitmap* inPicture,BOOL inOwnedPicture);
	VPictureData_GDIPlus(HICON inIcon,sLONG inWantedSize);
	
	virtual ~VPictureData_GDIPlus();
	
	virtual void DrawInPortRef(PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	virtual void DrawInGDIPlusGraphics(Gdiplus::Graphics* inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
#if ENABLE_D2D
	virtual void DrawInD2DGraphicContext(VWinD2DGraphicContext* inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
#endif
	virtual VError Save(VBlob* inData,VIndex inOffset,VSize& outSize,_VPictureAccumulator* inRecorder=0)const;

	
	virtual VPictureData* Clone()const;
	virtual sLONG GetFrameCount() const;
	virtual void BuildTimeLine(std::vector<sLONG>& inTimeLine,sLONG &outLoopCount)const;
	//virtual VPictureData* BuildThumbnail(sLONG inWidth,sLONG inHeight,PictureMosaic inMode,VPictureDrawSettings* inSet=NULL);
	virtual Gdiplus::Bitmap*	GetGDIPlus_Bitmap()const;
	
	/** return bitmap & give ownership to caller: caller becomes the owner & internal bitmap reference is set to NULL to prevent bitmap to be released 
	@remarks
		caller should release this picture data just after this call because picture data is no longer valid
		it should be used only when caller needs to take ownership of the internal bitmap (in order to modify it without copying it) & 
		if picture data is released after
	*/
	virtual Gdiplus::Bitmap*	GetAndOwnGDIPlus_Bitmap()const;

protected:
	/** load metadatas */
	virtual void _DoLoadMetadatas()const;

	virtual void _DoLoad()const;
	virtual void _DoReset()const;
	mutable Gdiplus::Bitmap* fBitmap;
private:
	//xGifInfo* fGifInfo;
	void _Init();
	
};

class XTOOLBOX_API VPictureData_GIF :public VPictureData_GDIPlus
{
	typedef VPictureData_GDIPlus inherited;
	protected:
	VPictureData_GIF();
	VPictureData_GIF(const VPictureData_GIF& inData);
	VPictureData_GIF& operator=(const VPictureData_GIF& /*inData*/){assert(false);return *this;}
	public:
	
	VPictureData_GIF(VPictureDataProvider* inDataSource,_VPictureAccumulator* inRecorder=0);
	virtual ~VPictureData_GIF();
	
	virtual sLONG GetFrameCount() const;
	virtual void BuildTimeLine(std::vector<sLONG>& inTimeLine,sLONG &outLoopCount)const;
	
	virtual VPictureData* Clone()const;
	
	protected:
	virtual void _DoLoad()const;
	virtual void _DoReset()const;
	private:
	
	void _Init();
	
	mutable xGifInfo* fGifInfo;
};

class XTOOLBOX_API VPictureData_EMF :public VPictureData_Vector
{
	typedef VPictureData_Vector inherited;
	protected:
	VPictureData_EMF();
	VPictureData_EMF(const VPictureData_EMF& inData);
	VPictureData_EMF& operator=(const VPictureData_EMF& /*inData*/){assert(false);return *this;}
	public:
	
	VPictureData_EMF(VPictureDataProvider* inDataSource,_VPictureAccumulator* inRecorder=0);	
	VPictureData_EMF(HMETAFILEPICT inMetaPict);
	VPictureData_EMF(HENHMETAFILE inMetaFile);
	
	virtual void DrawInPortRef(PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	virtual void DrawInGDIPlusGraphics(Gdiplus::Graphics* inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
#if ENABLE_D2D
	virtual void DrawInD2DGraphicContext(VWinD2DGraphicContext* inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
#endif
	
	void FromMetaFilePict(METAFILEPICT* inMetaPict);
	void FromEnhMetaFile(HENHMETAFILE inMetaFile);
	
	virtual ~VPictureData_EMF();
	virtual VPictureData* Clone()const;
	void FromMacHandle(void* inMacHandle);
	
	virtual VError Save(VBlob* inData,VIndex inOffset,VSize& outSize,_VPictureAccumulator* inRecorder=0)const;
	
	//virtual VPictureData* BuildThumbnail(sLONG inWidth,sLONG inHeight,PictureMosaic inMode,VPictureDrawSettings* inSet=NULL);

	virtual HENHMETAFILE	GetMetaFile()const ;
	virtual xMacPictureHandle			GetPicHandle()const;
	protected:
	
	virtual void _DoLoad()const;
	virtual void _DoReset()const;
	private:
	
	void _Init();
	void _DisposeMetaFile()const;
	
	mutable void* fPicHandle;
	mutable HENHMETAFILE fMetaFile;
};

class XTOOLBOX_API VPictureData_GDIPlus_Vector :public VPictureData_Vector
{
typedef VPictureData_Vector inherited;
protected:
VPictureData_GDIPlus_Vector();
VPictureData_GDIPlus_Vector(const VPictureData_GDIPlus_Vector& inData);
VPictureData_GDIPlus_Vector& operator=(const VPictureData_GDIPlus_Vector& /*inData*/){assert(false);return *this;}
public:
	
	VPictureData_GDIPlus_Vector(VPictureDataProvider* inDataSource,_VPictureAccumulator* inRecorder=0);
	VPictureData_GDIPlus_Vector(METAFILEPICT* inMetaPict);
	VPictureData_GDIPlus_Vector(HENHMETAFILE inMetaFile);
	VPictureData_GDIPlus_Vector(Gdiplus::Metafile* inMetafile);
	
	virtual ~VPictureData_GDIPlus_Vector();
	
	virtual void DrawInPortRef(PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	virtual void DrawInGDIPlusGraphics(Gdiplus::Graphics* inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
#if ENABLE_D2D
	virtual void DrawInD2DGraphicContext(VWinD2DGraphicContext* inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
#endif
	virtual VError Save(VBlob* inData,VIndex inOffset,VSize& outSize,_VPictureAccumulator* inRecorder=0)const;
 
	virtual VPictureData* Clone()const;
	
	//virtual VPictureData* BuildThumbnail(sLONG inWidth,sLONG inHeight,PictureMosaic inMode,VPictureDrawSettings* inSet=NULL);
	virtual Gdiplus::Metafile*	GetGDIPlus_Metafile()const;

protected:

	
	virtual void _DoLoad()const ;
	virtual void _DoReset()const;
private:

	void _FromEnhMetaFile(HENHMETAFILE inMetaFile);
	void _FromMetaFilePict(METAFILEPICT* inMetaPict);

	void _Init();
	void _DisposeMetaFile()const;
	void _InitSize()const;
	mutable Gdiplus::Metafile * fMetafile;	
};


END_TOOLBOX_NAMESPACE

#endif
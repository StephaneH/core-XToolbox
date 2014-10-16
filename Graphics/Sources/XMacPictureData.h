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
#ifndef __MACPICTUREDATA__
#define __MACPICTUREDATA__

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API VPictureData_CGImage :public VPictureData_Bitmap
{
	typedef VPictureData_Bitmap inherited;
	protected:
	VPictureData_CGImage();
	VPictureData_CGImage(const VPictureData_CGImage& inData);
	VPictureData_CGImage& operator =(const VPictureData_CGImage& inData){assert(false);return *this;}
	public:
	
	VPictureData_CGImage(CGImageRef inPicture);
	VPictureData_CGImage(IconRef inIcon,sLONG inWantedSize);
	VPictureData_CGImage(VPictureDataProvider* inDataProvider,_VPictureAccumulator* inRecorder=0);
	
	VPictureData_CGImage(PortRef inPortRef,const VRect& inBounds);
	virtual ~VPictureData_CGImage();
	
	virtual VError Save(VBlob* inData,VIndex inOffset,VSize& outSize,_VPictureAccumulator* inRecorder=0)const;
	
	virtual VPictureData* Clone()const;
	virtual void DrawInPortRef(PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	virtual void DrawInCGContext(CGContextRef inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	virtual CGImageRef	GetCGImage()const;
	virtual void FromVFile(VFile& inFile);
	//virtual VPictureData* BuildThumbnail(sLONG inWidth,sLONG inHeight,PictureMosaic inMode,VPictureDrawSettings* inSet=NULL);
	protected:
	virtual void _DoReset()const;
	virtual void _DoLoad()const;
	/** load metadatas */
	virtual void _DoLoadMetadatas()const;
	private:
	
	void _Init();
	
	mutable CGImageRef fBitmap;
	mutable CGImageRef fTransBitmap;
};

class XTOOLBOX_API VPictureData_PDF :public VPictureData_Vector
{
	typedef VPictureData_Vector inherited;
	protected:
	VPictureData_PDF();
	VPictureData_PDF(const VPictureData_PDF& inData);
	VPictureData_PDF& operator =(const VPictureData_PDF& inData){assert(false);return *this;}
	public:
	VPictureData_PDF(VPictureDataProvider* inDataProvider,_VPictureAccumulator* inRecorder=0);
	VPictureData_PDF(PortRef inPortRef,const VRect& inBounds);
	virtual ~VPictureData_PDF();
	
	virtual VError Save(VBlob* inData,VIndex inOffset,VSize& outSize,_VPictureAccumulator* inRecorder=0)const;
	
	virtual VPictureData* Clone()const;
	virtual void DrawInPortRef(PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	virtual void DrawInCGContext(CGContextRef inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	virtual CGPDFDocumentRef	GetPDF()const;
	virtual void FromVFile(VFile& inFile);
	//virtual VPictureData* BuildThumbnail(sLONG inWidth,sLONG inHeight,PictureMosaic inMode,VPictureDrawSettings* inSet=NULL);
	protected:
	virtual void _DoReset()const;
	virtual void _DoLoad()const;
	private:
	
	void _Init();
	
	mutable CGPDFDocumentRef fPDFDocRef;
};

class xGifInfo_QTImage :public xGifInfo
{
	typedef xGifInfo inherited;
	public:
	xGifInfo_QTImage(VStream* inStream);
	xGifInfo_QTImage();
	virtual ~xGifInfo_QTImage();
	virtual void FromVStream(VStream* inStream);
	virtual bool DoAllocPixBuffer(){return true;}
	CGImageRef GetNthFrame(sLONG inIndex);
	void AddFrame(xGifFrame* inFrame,CGImageRef inPrev);
	
	std::vector<CGImageRef> fCGImageList;
};

class XTOOLBOX_API VPictureData_GIF :public VPictureData_CGImage
{
	typedef VPictureData_CGImage inherited;
	
	protected:
	VPictureData_GIF();
	VPictureData_GIF(const VPictureData_GIF& inData);
	VPictureData_GIF(const VPictureData_GIF* inData);
	VPictureData_GIF& operator =(const VPictureData_GIF& inData){assert(false);return *this;}
	
	public:
	
	VPictureData_GIF(VPictureDataProvider* inDataProvider,_VPictureAccumulator* inRecorder=0);
	
	virtual ~VPictureData_GIF();
	virtual void DrawInPortRef(PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	virtual void DrawInCGContext(CGContextRef inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	virtual VError Save(VBlob* inData,VIndex inOffset,VSize& outSize,_VPictureAccumulator* inRecorder=0)const;
	
	virtual VPictureData* Clone()const;
	
	virtual sLONG GetFrameCount()const;
	virtual void BuildTimeLine(std::vector<sLONG>& inTimeLine,sLONG& outLoopCount)const;
	
	virtual CGImageRef	GetCGImage()const;
	
	protected:
	
	virtual void _DoReset()const;
	virtual void _DoLoad()const;

	private:
	
	void _Init();
	
	mutable xGifInfo_QTImage *fGifInfo;
};

END_TOOLBOX_NAMESPACE

#endif
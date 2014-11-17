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
#ifndef __VPictureData_QDPICT__
#define __VPictureData_QDPICT__

#if VERSIONMAC
struct QDPict;
#endif

BEGIN_TOOLBOX_NAMESPACE

#if VERSIONWIN || (VERSIONMAC && ARCH_32)
class XTOOLBOX_API VPictureData_MacPicture :public VPictureData_Vector
{
	typedef VPictureData_Vector inherited;
private:
	VPictureData_MacPicture();
	VPictureData_MacPicture(const VPictureData_MacPicture& inData);
	VPictureData_MacPicture& operator =(const VPictureData_MacPicture&){ assert(false); return *this; }
public:

	VPictureData_MacPicture(VPictureDataProvider* inDataProvider, _VPictureAccumulator* inRecorder = 0);
#if VERSIONWIN || WITH_QUICKDRAW
	VPictureData_MacPicture(PortRef inPortRef, const VRect& inBounds);
#endif

	VPictureData_MacPicture(xMacPictureHandle inMacPicHandle);

	virtual ~VPictureData_MacPicture();
	virtual VPictureData* Clone()const;
	void FromMacHandle(void* inMacHandle);
	//virtual void FromVFile(VFile& inFile);
	virtual void DoDataSourceChanged();


	virtual void DrawInPortRef(PortRef inPortRef, const VRect& r, VPictureDrawSettings* inSet = NULL)const;
#if VERSIONWIN

	virtual void DrawInGDIPlusGraphics(Gdiplus::Graphics* inDC, const VRect& r, VPictureDrawSettings* inSet = NULL)const;
#if ENABLE_D2D
	virtual void DrawInD2DGraphicContext(VWinD2DGraphicContext* inDC, const VRect& r, VPictureDrawSettings* inSet = NULL)const;
#endif
	virtual Gdiplus::Metafile*	GetGDIPlus_Metafile() const;
	virtual HENHMETAFILE	GetMetafile() const;
#elif VERSIONMAC
	virtual void DrawInCGContext(CGContextRef inDC, const VRect& r, VPictureDrawSettings* inSet = NULL)const;
#endif
	virtual VError Save(VBlob* inData, VIndex inOffset, VSize& outSize, _VPictureAccumulator* inRecorder = 0)const;
	virtual VError SaveToFile(VFile& inFile)const;

#if WITH_VMemoryMgr
	virtual xMacPictureHandle GetPicHandle()const; // return the pichandle in cache
	virtual xMacPictureHandle CreatePicHandle(VPictureDrawSettings* inSet, bool& outCanAddPicEnd) const; // return a new pichandle, owner is the caller
#endif

	virtual VSize GetDataSize(_VPictureAccumulator* inRecorder = 0) const;

protected:

	virtual void _DoLoad()const;
	virtual void _DoReset()const;
private:

	void* _BuildPicHandle()const;

	void	_CreateTransparent(VPictureDrawSettings* inSet)const;

	mutable void* fPicHandle;
	void _DisposeMetaFile()const;

#if VERSIONWIN // metafile cache
	mutable Gdiplus::Metafile*	fGdiplusMetaFile;
	mutable HENHMETAFILE		fMetaFile;
	mutable Gdiplus::Bitmap*	fTrans;
#elif VERSIONMAC
	mutable struct QDPict*	/*QDPictRef*/ fMetaFile;
	mutable CGImageRef	fTrans;
#endif

};

#endif

END_TOOLBOX_NAMESPACE
#endif
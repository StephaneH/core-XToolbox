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
#if 0

#ifndef __VPicture__
#define __VPicture__



#include "Graphics/Sources/IBoundedShape.h"

BEGIN_TOOLBOX_NAMESPACE

// Needed declarations
class VRect;


// Defined bellow
class VOldImageData;
class VOldPictData;
class VOldPixelData;


XTOOLBOX_API VOldImageData*	NewImageFromImageKind (ImageKind pKind);
XTOOLBOX_API sLONG	GetPictOpcodeSkipSize (VStream& pStream,uWORD opcode);


class XTOOLBOX_API VOldImageData : public VValue
{
public:
	Boolean CanBeEvaluated() const;
	ValueKind GetValueKind() const;
	VError ReadFromStream( VStream *pStream, sLONG pParam = 0);
	VError WriteToStream( VStream *pStream,sLONG pParam = 0) const;
	virtual ImageKind GetImageKind() const = 0;
	virtual Boolean IsValid() const = 0;
	virtual Real GetWidth() const = 0;
	virtual Real GetHeight() const = 0;
	virtual void GetWidthHeight(Real* w,Real* h) const = 0;
	virtual void* GetData() const = 0;
	virtual void* ForgetData() = 0;
	virtual void FromData(void* pData,Boolean pOwnIt = false) = 0;
	virtual void FromBytes(VPtr pData,sLONG pSize) = 0;
	virtual void FromImage(const VOldImageData& pImage) = 0;
	virtual void FromFile(const VFile& pFile) = 0;
	virtual void DumpHexa(VStream* pStream) = 0;
};


class XTOOLBOX_API VOldPictData : public VOldImageData
{
public:
	VOldPictData();
	VOldPictData(void* pPict,Boolean pOwnIt = true);
	VOldPictData(VPtr srcdata,sLONG srcsize);
	VOldPictData(const VOldPictData& srcdata);
	VOldPictData(const VOldImageData& srcdata);
	VOldPictData(const VFile& srcfile);
	virtual ~VOldPictData();
	VValue*	Clone() const ;
	VError ReadFromStream( VStream *pStream, sLONG pParam = 0);
	VError WriteToStream( VStream *pStream,sLONG pParam = 0) const;
	ImageKind GetImageKind() const;
	Boolean IsValid() const;
	Real GetWidth() const;
	Real GetHeight() const;
	void GetWidthHeight(Real* w,Real* h) const;
	void* GetData() const;
	void* ForgetData();
	void FromData(void* pData,Boolean pOwnIt = false);
	void FromBytes(VPtr pData,sLONG pSize);
	void FromImage(const VOldImageData& pImage);
	void FromFile(const VFile& pFile);
	void DumpHexa(VStream* pStream);
	
protected:
	void KillData();
	friend class VOldPicture;

#if USE_MAC_PICTS
	void* fData;
#else
	VHandle	fData;
#endif

	Boolean fOwned;
};


class XTOOLBOX_API VOldPixelData : public VOldImageData
{
 public:
	VOldPixelData();
	VOldPixelData(void* pPixMapRef,Boolean pOwnIt = true);
	VOldPixelData(VPtr srcdata,sLONG srcsize);
	VOldPixelData(const VOldImageData& srcdata);
	VOldPixelData(const VOldPixelData& srcdata);
	VOldPixelData(const VOldPixelData& pPixMap,const VRect& pRect);
	VOldPixelData(const VFile& srcfile);
	virtual ~VOldPixelData();
	VValue*	Clone() const ;
	VError ReadFromStream( VStream *pStream, sLONG pParam = 0);
	VError WriteToStream( VStream *pStream,sLONG pParam = 0) const;
	ImageKind GetImageKind() const;
	Boolean IsValid() const;
	Real GetWidth() const;
	Real GetHeight() const;
	void GetWidthHeight(Real* w,Real* h) const;
	void* GetData() const;
	void* ForgetData();
	void FromData(void* pData,Boolean pOwnIt = false);
	void FromBytes(VPtr pData,sLONG pSize);
	void FromImage(const VOldImageData& pImage);
	void FromFile(const VFile& pFile);
	void FromPixelData(const VOldPixelData& pPixMap,VRect* pRect = 0);
	void DumpHexa(VStream* pStream);
	
 protected:
	friend class VOldPicture;
	void KillData();
	PixMapRef fData;
	Boolean fOwned;
};


class XTOOLBOX_API VOldEmfData : public VOldImageData
{
public:
	VOldEmfData();
	VOldEmfData(void* srcemf,Boolean pOwnIt = true);
	VOldEmfData(VPtr srcdata,sLONG srcsize);
	VOldEmfData(const VOldEmfData& srcdata);
	VOldEmfData(const VOldImageData& srcdata);
	VOldEmfData(const VFile& srcfile);
	virtual ~VOldEmfData();
	VValue*	Clone() const ;
	VError ReadFromStream( VStream *pStream, sLONG pParam = 0);
	VError WriteToStream( VStream *pStream,sLONG pParam = 0) const;
	ImageKind GetImageKind() const;
	Boolean IsValid() const;
	Real GetWidth() const;
	Real GetHeight() const;
	void GetWidthHeight(Real* w,Real* h) const;
	void* GetData() const;
	void* ForgetData();
	void FromData(void* pData,Boolean pOwnIt = false);
	void FromBytes(VPtr pData,sLONG pSize);
	void FromWmfBytes(VPtr pData,sLONG pSize,sLONG pWidth, sLONG pHeight);
	void FromImage(const VOldImageData& pImage);
	void FromFile(const VFile& pFile);
	void DumpHexa(VStream* pStream);
	void DumpWmfHexa(VStream* pStream);
	
protected:
	friend class VOldPicture;
	void KillData();
	#if VERSIONWIN
	HENHMETAFILE fData;
	#else
	VHandle	fData;
	#endif
	Boolean fOwned;
};

#if VERSIONWIN
class XTOOLBOX_API VOldQTimeData : public VOldImageData 
{
public:
	VOldQTimeData();
	VOldQTimeData(void* pPict,Boolean pOwnIt = true);
	VOldQTimeData(VPtr srcdata,sLONG srcsize);
	VOldQTimeData(const VOldImageData& srcdata);
	VOldQTimeData(const VOldQTimeData& srcdata);
	VOldQTimeData(const VFile& srcfile);
	virtual ~VOldQTimeData();
	VValue*	Clone() const ;
	VError ReadFromStream( VStream *pStream, sLONG pParam = 0);
	VError WriteToStream( VStream *pStream,sLONG pParam = 0) const;
	ImageKind GetImageKind() const;
	Boolean IsValid() const;
	Real GetWidth() const;
	Real GetHeight() const;
	void GetWidthHeight(Real* w,Real* h) const;
	void* GetData() const;
	void* ForgetData();
	void FromData(void* pData,Boolean pOwnIt = false);
	void FromBytes(VPtr pData,sLONG pSize);
	void FromImage(const VOldImageData& pImage);
	void FromFile(const VFile& pFile);
	void DumpHexa(VStream* pStream);
	Boolean HasQuicktimeData();
	Boolean HasAciEmfData();
	
protected:
	void KillData();
	friend class VOldPicture;
	void*		fData;
	Boolean 	fOwned;
};
#endif


class XTOOLBOX_API VOldPicture : public VObject, public IBoundedShape
{
 public:
	VOldPicture(VPtr srcdata,sLONG srcsize,ImageKind kind);
	VOldPicture(VOldImageData* srcdata);
	VOldPicture(VResourceFile* pResFile,sWORD pResID,VString* pResName = 0);
	virtual ~VOldPicture();
	PictureMosaic SetMosaic(PictureMosaic pp);
	PictureMosaic GetMosaic() const;
	TransferMode SetPictMode(TransferMode tm);
	TransferMode GetPictMode() const;
	VOldImageData* GetImage() const;
	VOldImageData* ForgetImage();
	void SetImage(VOldImageData* pData);
	// pp 
	virtual void	SetPosBy (Real /*inHoriz*/, Real /*inVert*/) { /* TBD */ };
	virtual void	SetSizeBy (Real inHoriz, Real inVert) { fBounds.SetWidth(GetWidth()+inHoriz);fBounds.SetHeight(GetHeight()+inVert); };
	virtual Boolean	HitTest (const VPoint& /*inPoint*/) const { return false; /* TBD */ };
	virtual void	Inset (Real /*inHoriz*/, Real /*inVert*/) { /* TBD */ };
	
protected:
	friend class VEngine;
	friend class VDial4D;
	VOldImageData* fImage;
	short fMosaic; // PictureMosaic
	short fTransfer; // TransferMode
	
	void	_ComputeBounds () { /* TBD */ };
};


inline PictureMosaic VOldPicture::SetMosaic(PictureMosaic pp)
{
	PictureMosaic oldpos;
	
	oldpos = (PictureMosaic)fMosaic;
	fMosaic = pp;
	return oldpos;
}


inline PictureMosaic VOldPicture::GetMosaic() const
{
	return (PictureMosaic)fMosaic;
}


inline TransferMode VOldPicture::SetPictMode(TransferMode tm)
{
	TransferMode oldmode;
	
	oldmode = (TransferMode)fTransfer;
	fTransfer = tm;
	return oldmode;
}


inline TransferMode VOldPicture::GetPictMode() const
{
	return (TransferMode)fTransfer;
}


inline void VOldPicture::SetImage(VOldImageData* pData)
{
	if(fImage) delete fImage;
	fImage = pData;
}


inline VOldImageData* VOldPicture::GetImage() const
{
	return fImage;
};


inline VOldImageData* VOldPicture::ForgetImage()
{
	VOldImageData* result;
	
	result = fImage;
	fImage = 0;
	return result;
};

END_TOOLBOX_NAMESPACE

#endif

#endif

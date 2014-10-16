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
#ifndef __V4DPICTURE_TOOLS__
#define __V4DPICTURE_TOOLS__

BEGIN_TOOLBOX_NAMESPACE



#pragma pack(push, 8)

#if VERSIONWIN

typedef ENHMETAHEADER		xENHMETAHEADER;
#define xENHMETA_SIGNATURE	ENHMETA_SIGNATURE

#else

typedef struct xLRECT
{
    sLONG    left;
    sLONG    top;
    sLONG    right;
    sLONG    bottom;
} xLRECT;

typedef struct xENHMETAHEADER
{
    uLONG   iType;              // Record type EMR_HEADER
    uLONG   nSize;              // Record size in bytes.  This may be greater
                                // than the sizeof(ENHMETAHEADER).
    xLRECT   rclBounds;          // Inclusive-inclusive bounds in device units
    xLRECT   rclFrame;           // Inclusive-inclusive Picture Frame of metafile in .01 mm units
    uLONG   dSignature;         // Signature.  Must be ENHMETA_SIGNATURE.
    uLONG   nVersion;           // Version number
    uLONG   nBytes;             // Size of the metafile in bytes
}xENHMETAHEADER;
 

#if BIGENDIAN
	#define xENHMETA_SIGNATURE		0x20454D46
#else
	#define xENHMETA_SIGNATURE      0x464D4520
#endif

#endif


#pragma pack(pop)
#pragma pack(push, 2)


typedef struct xPictEndBlock
{
	sWORD			fOriginX;
	sWORD			fOriginY;
	sWORD			fTransfer;
}xPictEndBlock, *xPictEndPtr, **xPictEndHdl;

typedef struct xMacRect
{
	sWORD		top;
	sWORD		left;
	sWORD		bottom;
	sWORD		right;
}xMacRect;
typedef struct xMacPicture
{
	sWORD			picSize;
	xMacRect		picFrame;
}xMacPicture,*xMacPicturePtr,**xMacPictureHandle;

typedef struct xMacPictureHeader
{
	sWORD			picSize;
	xMacRect		picFrame;
	sWORD			picVersion;
	union
	{
		struct
		{
		}v0;
		struct
		{
			sWORD versionNumber;
			sWORD headerOpcode;
			sWORD headerVersion;
		}v1;
	}version;	
}xMacPictureHeader,*xMacPictureHeaderPtr,**xMacPictureHeaderHandle;

typedef char **xMacHandle;
		
class XTOOLBOX_API VMacHandleAllocatorBase:public VObject , public IRefCountable
{
	public:
	VMacHandleAllocatorBase(){;};
	virtual ~VMacHandleAllocatorBase(){;}
	virtual void*	Allocate(VSize inSize)=0;
	virtual void*	AllocateFromBuffer(void* inBuff,VSize inSize)=0;
	virtual void*	Duplicate(void* inHandle)=0;
	virtual void	Free(void* inHandle)=0;
	virtual bool	SetSize(void* inHandle,VSize inSize)=0;
	virtual VSize	GetSize(void* inHandle)=0;
	virtual void	Lock(void* inHandle)=0;
	virtual void	Unlock(void* inHandle)=0;
	virtual bool	CheckBlock(void* inHandle)=0;
	virtual VHandle ToVHandle(void* inHandle)=0;
	virtual XBOX::VPtr	ToVPtr(void* inHandle,XBOX::VSize& outSize)=0;
	virtual bool	IsLocked(void* inHandle)=0;
	virtual short	MemError()=0;
};


#pragma pack(pop)

class V4DPictureDataPrefetchedInformation:public VObject
{
	public:
	V4DPictureDataPrefetchedInformation()
	{
		fFlags=0;
		fWidth=0;
		fHeight=0;
		fVRes=0;
		fHRes=0;
	}
	
	V4DPictureDataPrefetchedInformation(const V4DPictureDataPrefetchedInformation& inInfo)
	{
		fFlags=inInfo.fFlags;
		fWidth=inInfo.fWidth;
		fHeight=inInfo.fHeight;
		fVRes=inInfo.fVRes;
		fHRes=inInfo.fHRes;
	}
	
	enum flags
	{
		kSizeIsValid=0x01,
		kResIsValid=0x02
	};
	void SetSize(sLONG inWidth,sLONG inHeight)
	{
		fWidth=inWidth;
		fHeight=inHeight;
		fFlags|=kSizeIsValid;
	}
	void SetResolution(sLONG inHRes,sLONG inVRes)
	{
		fVRes=inVRes;
		fVRes=inHRes;
		fFlags|=kResIsValid;
	}
	bool GetSize(sLONG& outWidth,sLONG& outHeight)
	{
		if((fFlags & kResIsValid) == kSizeIsValid)
		{
			outWidth=fWidth;
			outHeight=fHeight;
			return true;
		}
		return false;
	}
	bool GetResolution(sLONG& outHRes,sLONG& outVRes)
	{
		if((fFlags & kResIsValid) == kResIsValid)
		{
			outHRes=fHRes;
			outVRes=fVRes;
			return true;
		}
		return false;
	}
	void ClearSize()
	{
		fFlags=fFlags &~ kSizeIsValid;
	}
	void ClearResolution()
	{
		fFlags=fFlags &~ kResIsValid;
	}
	private:
	sLONG	fFlags;
	sLONG	fWidth;
	sLONG	fHeight;
	sLONG	fVRes;
	sLONG	fHRes;
};

class XTOOLBOX_API VPictureDrawSettings_Base :public VObject ,public IBaggable
{
	public:
	
	// IBaggable interface
	virtual VError	LoadFromBag( const VValueBag& inBag);
	virtual VError	SaveToBag( VValueBag& ioBag, VString& outKind) const;
	/**********************/
	
	VPictureDrawSettings_Base()
	{
		_Reset();
	};
	VPictureDrawSettings_Base(const VPictureDrawSettings_Base& inset)
	{
		_FromVPictureDrawSettings(inset);
	};
	void _FromVPictureDrawSettings(const VPictureDrawSettings_Base& inset)
	{
		fInterpolationMode=inset.fInterpolationMode;
		fDrawingMode=inset.fDrawingMode;
		fTransparentColor=inset.fTransparentColor;
		fAlphaBlendFactor=inset.fAlphaBlendFactor;
		fBackgroundColor=inset.fBackgroundColor;
		fFrameNumber=inset.fFrameNumber;
		fX=inset.fX;
		fY=inset.fY;
		fPictureMatrix=inset.fPictureMatrix;
		fLine=inset.fLine;
		fCol=inset.fCol;
		fSplit=inset.fSplit;

		fPicEnd_PosX=inset.fPicEnd_PosX; // old picend
		fPicEnd_PosY=inset.fPicEnd_PosY;
		fPicEnd_TransfertMode=inset.fPicEnd_TransfertMode;
	}
	virtual void Reset()
	{
		
	}
	void _Reset()
	{
		fInterpolationMode=1;
		fDrawingMode=0;
		fTransparentColor.SetColor(0xff,0xff,0xff);
		fBackgroundColor.SetColor(0xff,0xff,0xff);
		fAlphaBlendFactor=100;
		fFrameNumber=0;
		fX=0;
		fY=0;
		fPictureMatrix.MakeIdentity();
		fCol=1;
		fLine=1;
		fSplit=false;

		fPicEnd_PosX=0; // old picend
		fPicEnd_PosY=0;
		fPicEnd_TransfertMode=0;
	}
	void FromVPictureDrawSettings(const VPictureDrawSettings_Base& inset)
	{
		_FromVPictureDrawSettings(inset);
	}
	void _FromVPictureDrawSettings(const VPictureDrawSettings_Base* inset)
	{
		if(inset)
		{
			_FromVPictureDrawSettings(*inset);
		}
		else
		{
			_Reset();
		}
	}
	void FromVPictureDrawSettings(const VPictureDrawSettings_Base* inset)
	{
		_FromVPictureDrawSettings(inset);
	}
	VPictureDrawSettings_Base(const VPictureDrawSettings_Base* inset)
	{
		_FromVPictureDrawSettings(inset);
	};
	VPictureDrawSettings_Base(const class VPicture* inPicture);
	bool IsIdentityMatrix()const{return fPictureMatrix.IsIdentity();};
	virtual ~VPictureDrawSettings_Base(){;};
	
	virtual bool IsDefaultSettings();
	
	void SetInterpolationMode(sLONG inMode)
	{
		fInterpolationMode=inMode;
	};
	sLONG GetInterpolationMode() const
	{
		return fInterpolationMode;
	};
	void SetTransparentColor(const VColor& inColor=VColor(255,255,255,255))
	{
		fDrawingMode=1;
		fTransparentColor=inColor;
	};
	void SetBackgroundColor(const VColor& inColor=VColor(255,255,255,255))
	{
		fDrawingMode=3;
		fBackgroundColor=inColor;
	};
	
	void SetAlphaBlend(uBYTE inAlpha)
	{
		fDrawingMode=2;
		fAlphaBlendFactor= inAlpha>100 ? 100 : inAlpha;
	};
	void SetOpaque(){fDrawingMode=0;};
	sWORD GetDrawingMode()const{return fDrawingMode;};
	uBYTE GetAlpha()const{return fAlphaBlendFactor;};
	const VColor& GetTransparentColor()const {return fTransparentColor;};
	const VColor& GetBackgroundColor()const {return fBackgroundColor;};
	sLONG GetFrameNumber()const {return fFrameNumber;};
	
	virtual void SetOrigin(sLONG inX,sLONG inY)
	{
		fX=inX;fY=inY;
	}
	void GetOrigin(sLONG &outX,sLONG& outY) const
	{
		outX=fX;outY=fY;
	}
	
	void SetFrameNumber(sLONG inFrame)
	{
		fFrameNumber=inFrame;
	}
	
#if !VERSION_LINUX
	VPolygon TransformRect(const VRect& inRect)
	{
		VPolygon result(inRect);
		result=fPictureMatrix*result;
		return result;
	}
	VPolygon TransformSize(sLONG inWidth,sLONG inHeight)
	{
		VPolygon result(VRect(0,0,inWidth,inHeight));
		result=fPictureMatrix*result;
		return result;
	}
#endif
	
	
	inline VAffineTransform&	GetPictureMatrix(){return fPictureMatrix;};
	
	inline const VAffineTransform&	GetPictureMatrix()const{return fPictureMatrix;};
	
	inline void SetPictureMatrix(const VAffineTransform& inTransform)
	{
		fPictureMatrix=inTransform;
	}
	inline sLONG GetSplit_Line()const{return fLine;};
	inline sLONG GetSplit_Col()const{return fCol;};
	inline void SetSplit_Line(sLONG inLine){inLine<0 ? fLine=1 : fLine=inLine;};
	inline void SetSplit_Col(sLONG inCol){inCol<0 ? fCol=1 : fCol=inCol;};
	inline void SetSplitInfo(bool inSplit,sLONG inLine,sLONG inCol)
	{
		fSplit=inSplit;
		inLine<1 ? fLine=1 : fLine=inLine;
		inCol<1 ? fCol=1 : fCol=inCol;
	};
	inline bool IsSplited()const {return fSplit;}

	sWORD GetPicEnd_PosX()const{return fPicEnd_PosX;}
	sWORD GetPicEnd_PosY()const{return fPicEnd_PosY;}
	void SetPicEndPos(sWORD inX,sWORD inY){fPicEnd_PosX=inX;fPicEnd_PosY=inY;}
	void  OffsetPicEndPos(sWORD inOffX,sWORD inOffY){fPicEnd_PosX+=inOffX;fPicEnd_PosY+=inOffY;}
	
	sWORD GetPicEnd_TransfertMode()const{return fPicEnd_TransfertMode;}
	void SetPicEnd_TransfertMode(sWORD inMode){fPicEnd_TransfertMode=inMode;}

	private:
	
	VAffineTransform			fPictureMatrix;
	
	sLONG			fX;
	sLONG			fY;
	sWORD			fDrawingMode;
	VColor			fTransparentColor;
	VColor			fBackgroundColor;
	uBYTE			fAlphaBlendFactor;
	sLONG			fFrameNumber;
	sLONG			fInterpolationMode;
	
	sWORD			fLine;
	sWORD			fCol;
	bool			fSplit;

	sWORD								fPicEnd_PosX; // old picend
	sWORD								fPicEnd_PosY;
	sWORD								fPicEnd_TransfertMode;
	
};

class XTOOLBOX_API  VPictureDrawSettings:public VPictureDrawSettings_Base
{
	typedef  VPictureDrawSettings_Base inherited;
	public:
	
	virtual VError	LoadFromBag( const VValueBag& inBag);
	virtual VError	SaveToBag( VValueBag& ioBag, VString& outKind) const;
	
	VPictureDrawSettings()
	:inherited()
	{
		fUseEMFForMacPicture = false;
		fDevicePrinter		= false;
		fDeviceForce72dpi   = true;
		fDeviceCanUseLayers = true;
		fDeviceCanUseClearType = true;
		fYAxisSwap=0;
		fXAxisSwap=0;
		fYLocalAxisSwap=false;
		fXLocalAxisSwap=false;
		fScaleMode=PM_SCALE_TO_FIT;
		fSVGSettings = NULL;
	}
	virtual ~VPictureDrawSettings()
	{
		ReleaseRefCountable(&fSVGSettings);
	}

	VPictureDrawSettings(const VPictureDrawSettings_Base& inSettings)
	:inherited(inSettings)
	{
		fUseEMFForMacPicture = false;
		fDevicePrinter		= false;
		fDeviceForce72dpi   = true;
		fDeviceCanUseLayers = true;
		fDeviceCanUseClearType = true;
		fYAxisSwap=0;
		fXAxisSwap=0;
		fYLocalAxisSwap=false;
		fXLocalAxisSwap=false;
		fSVGSettings = NULL;
		fScaleMode=PM_SCALE_TO_FIT;
		_UpdateDrawingMatrix();
	}
	VPictureDrawSettings(const VPictureDrawSettings& inSettings)
	:inherited(inSettings)
	{
		fUseEMFForMacPicture = inSettings.fUseEMFForMacPicture;
		fXAxisSwap=inSettings.fXAxisSwap;
		fYAxisSwap=inSettings.fYAxisSwap;
		fYLocalAxisSwap=inSettings.fYLocalAxisSwap;
		fXLocalAxisSwap=inSettings.fXLocalAxisSwap;
		fDrawingMatrix=inSettings.fDrawingMatrix;
		fScaleMode=inSettings.fScaleMode;
		fDevicePrinter		= inSettings.fDevicePrinter;
		fDeviceForce72dpi   = inSettings.fDeviceForce72dpi;
		fDeviceCanUseLayers = inSettings.fDeviceCanUseLayers;
		fDeviceCanUseClearType = inSettings.fDeviceCanUseClearType;
		fSVGSettings = NULL;
		CopyRefCountable(&fSVGSettings,inSettings.fSVGSettings);
	}
	VPictureDrawSettings(const VPictureDrawSettings_Base* inSettings)
	:inherited(inSettings)
	{
		fYAxisSwap=0;
		fXAxisSwap=0;
		fYLocalAxisSwap=false;
		fXLocalAxisSwap=false;
		fScaleMode=PM_SCALE_TO_FIT;
		fDevicePrinter		= false;
		fDeviceForce72dpi   = true;
		fDeviceCanUseLayers = true;
		fDeviceCanUseClearType = true;
		fUseEMFForMacPicture = false;
		fSVGSettings = NULL;
		_UpdateDrawingMatrix();
	}
	VPictureDrawSettings(const VPictureDrawSettings* inSettings)
	:inherited(inSettings)
	{
		if(inSettings)
		{
			fUseEMFForMacPicture= inSettings->fUseEMFForMacPicture;
			fDevicePrinter		= inSettings->fDevicePrinter;
			fDeviceForce72dpi   = inSettings->fDeviceForce72dpi;
			fDeviceCanUseLayers = inSettings->fDeviceCanUseLayers;
			fDeviceCanUseClearType = inSettings->fDeviceCanUseClearType;
			fDrawingMatrix=inSettings->fDrawingMatrix;
			fYAxisSwap=inSettings->fYAxisSwap;
			fXAxisSwap=inSettings->fXAxisSwap;
			fYLocalAxisSwap=inSettings->fYLocalAxisSwap;
			fXLocalAxisSwap=inSettings->fXLocalAxisSwap;
			fScaleMode=inSettings->fScaleMode;
			fSVGSettings = NULL;
			CopyRefCountable(&fSVGSettings,inSettings->fSVGSettings);
		}
		else
		{
			fUseEMFForMacPicture = false;
			fDevicePrinter		= false;
			fDeviceForce72dpi   = true;
			fDeviceCanUseLayers = true;
			fDeviceCanUseClearType = true;
			fScaleMode=PM_SCALE_TO_FIT;
			fYAxisSwap=0;
			fXAxisSwap=0;
			fYLocalAxisSwap=false;
			fXLocalAxisSwap=false;
			fSVGSettings = NULL;
		}
	}
	VPictureDrawSettings(const class VPicture* inPicture)
	:inherited(inPicture)
	{
		fUseEMFForMacPicture = false;
		fScaleMode=PM_SCALE_TO_FIT;
		fYAxisSwap=0;
		fXAxisSwap=0;
		fYLocalAxisSwap=false;
		fXLocalAxisSwap=false;
		fDevicePrinter		= false;
		fDeviceForce72dpi   = true;
		fDeviceCanUseLayers = true;
		fDeviceCanUseClearType = true;
		fSVGSettings = NULL;
		_UpdateDrawingMatrix();
	}
	void _FromVPictureDrawSettings(const VPictureDrawSettings& inset)
	{
		fUseEMFForMacPicture = inset.fUseEMFForMacPicture;
		fScaleMode=inset.fScaleMode;
		fYAxisSwap=inset.fYAxisSwap;
		fXAxisSwap=inset.fXAxisSwap;
		fYLocalAxisSwap=inset.fYLocalAxisSwap;
		fXLocalAxisSwap=inset.fXLocalAxisSwap;
		fDevicePrinter		= inset.fDevicePrinter;
		fDeviceForce72dpi   = inset.fDeviceForce72dpi;
		fDeviceCanUseLayers = inset.fDeviceCanUseLayers;
		fDeviceCanUseClearType = inset.fDeviceCanUseClearType;
		CopyRefCountable(&fSVGSettings,inset.fSVGSettings);
		_UpdateDrawingMatrix();
	}
	void FromVPictureDrawSettings(const VPictureDrawSettings& inset)
	{
		inherited::FromVPictureDrawSettings(inset);
		_FromVPictureDrawSettings(inset);
	}
	void FromVPictureDrawSettings(const VPictureDrawSettings* inset)
	{
		inherited::FromVPictureDrawSettings(inset);
		_FromVPictureDrawSettings(inset);
	}
	void _FromVPictureDrawSettings(const VPictureDrawSettings* inset)
	{
		if(inset)
			_FromVPictureDrawSettings(*inset);
		else
		{
			fUseEMFForMacPicture=false;
			fDevicePrinter=false;
			fDeviceForce72dpi=true;
			fDeviceCanUseLayers=true;
			fDeviceCanUseClearType=true;
			fScaleMode=PM_SCALE_TO_FIT;
			fYAxisSwap=0;
			fXAxisSwap=0;
			fYLocalAxisSwap=false;
			fXLocalAxisSwap=false;
			fDrawingMatrix.MakeIdentity();
			ReleaseRefCountable(&fSVGSettings);
		}
	}
	void SetDrawingMatrix(const VAffineTransform& inTransform)
	{
		fDrawingMatrix=inTransform;
	}
	
	void	GetDrawingMatrix(VAffineTransform& outMat,bool inAutoSwapAxis);
	
	const VAffineTransform&	GetDrawingMatrix()const{return fDrawingMatrix;};
	VAffineTransform&	GetDrawingMatrix(){return fDrawingMatrix;};
	
	virtual void SetOrigin(sLONG inX,sLONG inY)
	{
		inherited::SetOrigin(inX,inY);
		fDrawingMatrix.Translate(-inX,-inY,VAffineTransform::MatrixOrderAppend);
	}
	
	void SetYAxisSwap(sLONG inY,bool inLocalSwap){fYAxisSwap=inY;fYLocalAxisSwap=inLocalSwap;};
	sLONG GetYAxisSwap()const {return fYAxisSwap;};
	bool GetYLocalAxisSwap()const {return fYLocalAxisSwap;};
	
	void SetXAxisSwap(sLONG inX,bool inLocalSwap){fXAxisSwap=inX;;fXLocalAxisSwap=inLocalSwap;};
	sLONG GetXAxisSwap()const {return fXAxisSwap;};
	bool GetXLocalAxisSwap()const {return fXLocalAxisSwap;};
	
	void GetAxisSwap(sLONG &outX,sLONG& outY,bool& outSwapLocalX,bool& outSwapLocalY)
	{
		outX=fXAxisSwap;
		outY=fYAxisSwap;
		outSwapLocalX=fXLocalAxisSwap;
		outSwapLocalY=fYLocalAxisSwap;
	}
	void SetAxisSwap(sLONG inX,sLONG inY,bool inSwapLocalX,bool inSwapLocalY)
	{
		fXAxisSwap=inX;
		fYAxisSwap=inY;
		fXLocalAxisSwap=inSwapLocalX;
		fYLocalAxisSwap=inSwapLocalY;
	}
	
	PictureMosaic GetScaleMode()const {return fScaleMode;};
	void SetScaleMode(PictureMosaic inMode)
	{
		fScaleMode=inMode;
	};
	
	//printer device flag accessors
	inline bool IsDevicePrinter()
	{
		return fDevicePrinter != 0;
	}
	inline void SetDevicePrinter( bool inDevicePrinter)
	{
		fDevicePrinter = inDevicePrinter;
	}

	inline bool CanUseEMFForMacPicture()
	{
		return fUseEMFForMacPicture;
	}
	inline void UseEMFForMacPicture( bool inUse)
	{
		fUseEMFForMacPicture = inUse;
	}

	//printer device flag accessors
	inline bool NeedToForce72dpi()
	{
		return fDeviceForce72dpi;
	}
	inline void Force72dpi( bool inForce72dpi)
	{
		fDeviceForce72dpi = inForce72dpi;
	}
	//layer usage flag accessor
	inline bool DeviceCanUseLayers()
	{
		return fDeviceCanUseLayers != 0;
	}
	inline void DeviceCanUseLayers( bool inDeviceCanUseLayers)
	{
		fDeviceCanUseLayers = inDeviceCanUseLayers;
	}

	inline bool DeviceCanUseClearType()
	{
		return fDeviceCanUseClearType != 0;
	}
	inline void DeviceCanUseClearType( bool inDeviceCanUseClearType)
	{
		fDeviceCanUseClearType = inDeviceCanUseClearType;
	}

	/** return retained SVG settings */
	IRefCountable *RetainSVGSettings() const
	{
		if (fSVGSettings)
		{
			fSVGSettings->Retain();
			return fSVGSettings;
		}
		return NULL;
	}
	/** set and retain new SVG settings */
	void SetAndRetainSVGSettings( IRefCountable *inSettings)
	{
		ReleaseRefCountable(&fSVGSettings);
		if (inSettings)
			CopyRefCountable(&fSVGSettings, inSettings);
	}
	/** release SVG settings */
	void ReleaseSVGSettings()
	{
		ReleaseRefCountable(&fSVGSettings);
	}

	private:
	
	void _UpdateDrawingMatrix()
	{
		sLONG x,y;
		GetOrigin(x,y);
		fDrawingMatrix.Translate(-x,-y,VAffineTransform::MatrixOrderAppend);
	}
	PictureMosaic				fScaleMode;
	VAffineTransform			fDrawingMatrix;
	
	// changement de system de coords
	sLONG						fYAxisSwap;
	bool						fXLocalAxisSwap;
	
	sLONG						fXAxisSwap;
	bool						fYLocalAxisSwap;
	
	//runtime usage flags
	bool			fDevicePrinter;			//true if drawing in printer device context
	bool			fDeviceForce72dpi;		//true when printing pictures in 72 dpi (like in 4D), else adjust to screen resolution
	bool			fDeviceCanUseLayers;	//true if the device context can use layers
	bool			fDeviceCanUseClearType; //true if the device context can use ClearType antialiasing
	bool			fUseEMFForMacPicture;

	/** SVG settings 
	@remarks
		in order to maintain drawing attributes set by
		4D command "SVG set attribute"
		we need to maintain here SVG settings

		these settings will be created only if 
		command "SVG set attribute" is called at least once on the SVG picture object
		and will stay alive until this class instance destruction
	@see
		VSVGGraphTree (SVG component)
	*/
	IRefCountable	*fSVGSettings;
};

class XTOOLBOX_API VBitmapData :public VObject
{
	private:
	VBitmapData(const VBitmapData&){assert(false);};
	VBitmapData& operator=(const VBitmapData&){assert(false);return *this;};
	
	
	public:
	
	// pp pas tres clean je sais.....

#pragma pack(push,1)

typedef struct _ARGBPix
{
	union 
	{
		uLONG ARGB;
		struct
		{
			#if VERSIONMAC
				#if SMALLENDIAN
				uBYTE A;
				uBYTE B;
				uBYTE G;
				uBYTE R;
				#else
				uBYTE R;
				uBYTE G;
				uBYTE B;
				uBYTE A;
				#endif
			#else
			uBYTE B;
			uBYTE G;
			uBYTE R;
			uBYTE A;
			#endif
		}col;
		struct
		{
			uBYTE A;
			uBYTE B;
			uBYTE G;
			uBYTE R;
		}_ARGB;
		struct
		{
			uBYTE B;
			uBYTE G;
			uBYTE R;
			uBYTE A;
		}_BGRA;
	}variant;
}_ARGBPix;

	
#pragma pack(pop)

	
	
	#define    _PixelFormatIndexed      0x00010000 // Indexes into a palette
	#define    _PixelFormatAlpha        0x00040000 // Has an alpha component
	#define    _PixelFormatPAlpha       0x00080000 // Pre-multiplied alpha
	#define    _PixelFormatCanonical    0x00200000 

	enum VPixelFormat
	{
		VPixelFormatInvalide=0,
		//VPixelFormat1bppIndexed=(1 | ( 1 << 8) | _PixelFormatIndexed),
		VPixelFormat4bppIndexed=(2 | ( 4 << 8) | _PixelFormatIndexed),
		VPixelFormat8bppIndexed=(3 | ( 8 << 8) | _PixelFormatIndexed),
		
		VPixelFormat32bppRGB=(9 | (32 << 8) ),
		VPixelFormat32bppARGB=(10 | (32 << 8) | _PixelFormatAlpha),
		VPixelFormat32bppPARGB=(11 | (32 << 8) | _PixelFormatAlpha | _PixelFormatPAlpha)
	};
	VBitmapData(const class VPictureData& inData,VPictureDrawSettings* inSet=0);
	#if VERSIONWIN
	VBitmapData(Gdiplus::Bitmap& inBitmap,Gdiplus::Bitmap* inMask=0);
	#elif VERSIONMAC
	VBitmapData(CGImageRef inCGImage,CGImageRef inMask);
	#endif
	VBitmapData(void* pixbuffer,sLONG width,sLONG height,sLONG byteperrow,VBitmapData::VPixelFormat pixelsformat,std::vector<VColor> *inColortable=0);
	VBitmapData(sLONG width,sLONG height,VBitmapData::VPixelFormat pixelsformat,std::vector<VColor> *inColortable=0, bool inClearColorTransparent = false);
	virtual ~VBitmapData();
	
	sLONG GetWidth() const {return fWidth;};
	sLONG GetHeight() const {return fHeight;};
	VBitmapData::VPixelFormat GetPixelsFormat() const {return fPixelsFormat;};

	void* GetPixelBuffer() const {return fPixBuffer;};
	void* GetLinePtr(sLONG lineindex) const {return ((char*)fPixBuffer)+ (fBytePerRow*lineindex);};
	const std::vector<VColor>& GetColorTable() const {return fColorTable;};
	sLONG GetRowByte() const {return fBytePerRow;};
	void SetColorTable(std::vector<VColor>& inCTable){fColorTable=inCTable;};
	
	void ConvertToGray();
	VBitmapData* MakeTransparent(const VColor& inTransparentColor);
	//fill bitmap data with the specified color but alpha
	//@remark: original alpha and luminance are preserved
	void ApplyColor(const VColor &inColor);

	//fill bitmap data with the specified color
	//@remark: pixel color included alpha is replaced without luminance preservation
	void Clear(const VColor &inColor);

	void MergeMask(VBitmapData& inMask);
	#if VERSIONWIN
	Gdiplus::Bitmap* CreateNativeBitmap();
	#elif VERSIONMAC
	CGImageRef CreateNativeBitmap();

	//create a bitmap graphic context associated with the internal pixel buffer
	//@remark: caller is responsible to release the context
	//		   but do not release VBitmapData before the context
	//		   because context uses VBitmapData pixel buffer
	CGContextRef CreateBitmapContext();
	#endif
	class VPictureData* CreatePictureData();
	
	inline uLONG GetPixelFormatSize() const {return GetPixelFormatSize(fPixelsFormat);}
	inline uLONG GetPixelFormatSize(VBitmapData::VPixelFormat pixfmt) const
	{
		return (pixfmt >> 8) & 0xff;
	}
	inline bool IsIndexedPixelFormat() const {return IsIndexedPixelFormat(fPixelsFormat);}
	inline bool IsIndexedPixelFormat(VBitmapData::VPixelFormat pixfmt) const 
	{
		return (pixfmt & _PixelFormatIndexed) != 0;
	}
	inline bool IsAlphaPixelFormat() const {return IsAlphaPixelFormat(fPixelsFormat);}
	inline bool IsAlphaPixelFormat(VBitmapData::VPixelFormat pixfmt) const
	{
		return (pixfmt & _PixelFormatAlpha) != 0;
	}
	inline bool IsPreAlphaPixelFormat() const {return IsPreAlphaPixelFormat(fPixelsFormat);}
	inline bool IsPreAlphaPixelFormat(VBitmapData::VPixelFormat pixfmt) const
	{
		return (pixfmt & _PixelFormatPAlpha) != 0;
	}
	const VColor& GetIndexedColor(sLONG inIndex); 

	/** compare this bitmap data with the input bitmap data
	@param inBmpData
		the input bitmap data to compare with
	@outMask
		if not NULL, *outMask will contain a bitmap data where different pixels (pixels not equal in both pictures) have alpha = 0.0f
		and equal pixels have alpha = 1.0f
	@remarks
		if the bitmap datas have not the same width or height, return false & optionally set *outMask to NULL
	*/
	bool Compare( const VBitmapData *inBmpData, VBitmapData **outMask = NULL) const;

	private:
	void _Init();
	bool _AllocateBuffer(sLONG inWidth,sLONG inHeight,VBitmapData::VPixelFormat inFormat, bool inClearColorTransparent = false);
	bool fInited;
	bool	fOwnBuffer;
	void*	fPixBuffer;
	sLONG	fWidth;
	sLONG	fHeight;
	sLONG	fBytePerRow;
	VBitmapData::VPixelFormat	fPixelsFormat;

	std::vector<VColor> fColorTable;
	VColor fFakeColor;
};

END_TOOLBOX_NAMESPACE
#endif

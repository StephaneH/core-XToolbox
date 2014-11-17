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

#if VERSIONMAC && !WITH_QUICKDRAW && !ARCH_64

// some QD apis removed from OSX sdk 10.7
extern "C" CGRect QDPictGetBounds(struct QDPict* pictRef);
extern "C" void QDPictRelease(struct QDPict* pictRef);
extern "C" Rect *QDGetPictureBounds( Picture** picH, Rect *outRect);
extern "C" struct QDPict* QDPictCreateWithProvider(CGDataProviderRef provider);
extern "C" OSStatus QDPictDrawToCGContext( CGContextRef ctx, CGRect rect, struct QDPict* pictRef);


#endif

using namespace std;

#include "V4DPictureIncludeBase.h"
#if ENABLE_D2D
#include "XWinD2DGraphicContext.h"
#endif

#if VERSIONMAC
void VPictureQDBridgeBase::DrawInCGContext(const VPictureData &inCaller,CGImageRef inPict,ContextRef inContextRef,const VRect& inBounds,VPictureDrawSettings* inSet)
{
	inCaller.xDraw(inPict,inContextRef,inBounds,inSet);
}
void VPictureQDBridgeBase::DrawInCGContext(const VPictureData &inCaller,CGPDFDocumentRef  inPict,ContextRef inContextRef,const VRect& inBounds,VPictureDrawSettings* inSet)
{
	inCaller.xDraw(inPict,inContextRef,inBounds,inSet);
}

#if ARCH_32
void VPictureQDBridgeBase::DrawInCGContext(const VPictureData &inCaller,xMacPictureHandle inPict,ContextRef inContextRef,const VRect& inBounds,VPictureDrawSettings* inSet)
{
	inCaller.xDraw(inPict,inContextRef,inBounds,inSet);
}

void VPictureQDBridgeBase::DrawInCGContext(const VPictureData &inCaller,struct QDPict* inPict,ContextRef inContextRef,const VRect& inBounds,VPictureDrawSettings* inSet)
{
	inCaller.xDraw(inPict,inContextRef,inBounds,inSet);
}
#endif

#endif



#if VERSIONWIN

class _HDC_RightToLeft_SaverSetter
{
	public:
	_HDC_RightToLeft_SaverSetter(HDC inDC,VPictureDrawSettings& ioSet)
	{
		fDC=inDC;
		fNeedRestore=false;
		fLayout=GetLayout(fDC);
		
		if(fLayout &  LAYOUT_RTL)
		{
			DWORD err;
			POINT p={0,0};
			sLONG newlayout=fLayout;
			
			DPtoLP(fDC,&p,1);
			ioSet.SetXAxisSwap(p.x,false);
			newlayout &= ~LAYOUT_RTL;
			err=SetLayout(fDC,newlayout); 
			fNeedRestore=true;
		}
	}
	virtual ~_HDC_RightToLeft_SaverSetter()
	{
		if(fNeedRestore)
			SetLayout(fDC,fLayout);
	}
	private:
	sLONG	fLayout;
	HDC		fDC;
	bool	fNeedRestore;
};
class _Gdiplus_TransformSaver
{
	public:
	_Gdiplus_TransformSaver(Gdiplus::Graphics* inDC)
	{
		fDC=inDC;
		if(fDC)
		{
			Gdiplus::REAL tmp[6];
			fDC->GetTransform(&fMat);
			fMat.GetElements(tmp);
			if(tmp[0]==0.0)
			{
				
			}
		}
	}
	virtual ~_Gdiplus_TransformSaver()
	{
		if(fDC)
			fDC->SetTransform(&fMat);
	}
	private:
	Gdiplus::Graphics* fDC;
	Gdiplus::Matrix fMat;
};
class _HDC_TransformSetter
{
	public:
	_HDC_TransformSetter(HDC inDC,VAffineTransform& inMat)
	{
		
		BOOL oksetmode;
		BOOL oksetworld;
		fDC=inDC;
		fOldMode=::GetGraphicsMode (fDC);
		if(fOldMode==GM_ADVANCED)
		{
			::GetWorldTransform (fDC,&fOldTransform);
		}
		else
		{
			oksetmode=::SetGraphicsMode(fDC,GM_ADVANCED );
			::GetWorldTransform (fDC,&fOldTransform);
		}
		XFORM form;
		inMat.ToNativeMatrix(form);
		
		oksetworld=::SetWorldTransform (fDC,&form);
		
	}
	_HDC_TransformSetter(HDC inDC)
	{
		
		BOOL oksetmode;
		BOOL oksetworld;
		fDC=inDC;
		fOldMode=::GetGraphicsMode (fDC);
		if(fOldMode==GM_ADVANCED)
		{
			::GetWorldTransform (fDC,&fOldTransform);
		}
		else
		{
			oksetmode=::SetGraphicsMode(fDC,GM_ADVANCED );
			::GetWorldTransform (fDC,&fOldTransform);
		}
	}
	virtual ~_HDC_TransformSetter()
	{
		if(fOldMode!=GM_ADVANCED)
		{
			XFORM form;
			::ModifyWorldTransform(fDC,&form,MWT_IDENTITY);
			::SetGraphicsMode(fDC,fOldMode );
		}
		else
		{
			::SetWorldTransform (fDC,&fOldTransform);
		}
	}
	void SetTransform(const VAffineTransform& inMat)
	{
		XFORM form;
		inMat.ToNativeMatrix(form);
		::SetWorldTransform (fDC,&form);
	}
	void ResetTransform()
	{
		::SetWorldTransform (fDC,&fOldTransform);
	}
	private:
	HDC fDC;
	DWORD fOldMode;
	XFORM fOldTransform;
};


#endif

#if !VERSION_LINUX
VAffineTransform VPictureData::_MakeScaleMatrix(const VRect& inDisplayRect,VPictureDrawSettings& inSet)const
{
	// calcul de la matrice pour obtenir le rect dest a partir de rect de l'image et du mode de dessin
	// ne prend pas en compte le facteur de translation de l'image
	// ne map pas sur l'origine du rect dest
	
	VAffineTransform ouMat;
	switch(inSet.GetScaleMode())
	{
		case PM_SCALE_EVEN:
		case PM_SCALE_CENTER:
		case PM_4D_SCALE_EVEN:
		case PM_4D_SCALE_CENTER:
		{
			const VRect& pictrect=GetBounds();
			VPolygon pictpoly(pictrect);
			pictpoly=inSet.GetPictureMatrix().TransformVector(pictpoly);
			GReal wr = inDisplayRect.GetWidth()  / pictpoly.GetWidth();
			GReal hr = inDisplayRect.GetHeight() / pictpoly.GetHeight();
			if(wr-hr < 0.000001)
			{
				ouMat.Scale(wr,wr);
			}
			else
			{
				ouMat.Scale(hr,hr);
			}
			break;
		}
		
		case PM_SCALE_TO_FIT:
		{
			const VRect& pictrect=GetBounds();
			VPolygon pictpoly(pictrect);
			pictpoly=inSet.GetPictureMatrix().TransformVector(pictpoly);
			
			ouMat.Scale(inDisplayRect.GetWidth()/pictpoly.GetWidth(),inDisplayRect.GetHeight()/pictpoly.GetHeight());
			break;
		}
		case PM_CENTER:
		case PM_TOPLEFT:
		case PM_TILE:
		case PM_MATRIX:
		{
			// identity
			break;
		}
	}
	return ouMat;
}
#endif

#if !VERSION_LINUX
void VPictureData::_AjustFlip(VAffineTransform& inMat, VRect* inPictureBounds) const
{
	#if 0
	Gdiplus::Matrix mat;
	VAffineTransform mat1;
	
	Real step=15;
	
	for(long i=step;i<360;i+=step)
	{
		Gdiplus::REAL gmat[6];
		mat.Rotate(step)	;
		mat1.Rotate(step);
		mat.GetElements(gmat);
		if(i==888)
		{
			long zz=0;
			zz++;
		}
		else
		{
			long zz=0;
			zz++;
		}
	}
	#endif
	
	if(inMat.GetScaleY()<0 || inMat.GetScaleX()<0 /*&& (inMat.GetShearX()<0.0 && inMat.GetShearY()<0.0)*/)
	{
		VRect r;
		if(inPictureBounds)
			r=*inPictureBounds;
		else
			r.SetCoords(0,0,GetWidth(),GetHeight());
		VPolygon pictpoly(r);
		GReal transx,transy;
		pictpoly=inMat.TransformVector(pictpoly);
				
		transx= inMat.GetScaleX()<0 ? Abs(pictpoly.GetX()) -( pictpoly.GetX() + pictpoly.GetWidth()) : 0; 
		transy= inMat.GetScaleY()<0 ? Abs(pictpoly.GetY()) - (pictpoly.GetY() + pictpoly.GetHeight()) : 0; 
		inMat.Translate(transx, transy,VAffineTransform::MatrixOrderAppend);					
	}
}
#endif

#if !VERSION_LINUX
void VPictureData::_ApplyDrawingMatrixTranslate(VAffineTransform& inMat,VPictureDrawSettings& inSet)const
{
	VAffineTransform tmp(inSet.GetDrawingMatrix());
	
	inSet.GetDrawingMatrix(tmp,true);
	
	inMat.Multiply(tmp,VAffineTransform::MatrixOrderAppend);		
}
#endif

#if !VERSION_LINUX
VAffineTransform VPictureData::_MakeFinalMatrix(const VRect& inDisplayRect,VPictureDrawSettings& inSet,bool inAutoSwapAxis)const
{
	GReal swp_x,swp_y;
	VRect pictrect(0,0,GetWidth(),GetHeight());
	VPolygon pictpoly(pictrect);
	VAffineTransform scalemat;
	
	// calcul de la matrice de scalling
	
	switch(inSet.GetScaleMode())
	{
		case PM_SCALE_EVEN:
		case PM_SCALE_CENTER:
		{
			pictpoly=inSet.GetPictureMatrix().TransformVector(pictpoly);
			GReal wr = inDisplayRect.GetWidth()  / pictpoly.GetWidth();
			GReal hr = inDisplayRect.GetHeight() / pictpoly.GetHeight();
			if(wr-hr < 0)
			{
				scalemat.Scale(wr,wr);
			}
			else
			{
				scalemat.Scale(hr,hr);
			}
			break;
		}
		case PM_4D_SCALE_EVEN:
		case PM_4D_SCALE_CENTER:
		{
			pictpoly=inSet.GetPictureMatrix().TransformVector(pictpoly);
			GReal wr = inDisplayRect.GetWidth()  / pictpoly.GetWidth();
			GReal hr = inDisplayRect.GetHeight() / pictpoly.GetHeight();
			if(wr<1.0 || hr<1.0)
			{
				if(wr-hr < 0)
				{
					scalemat.Scale(wr,wr);
				}
				else
				{
					scalemat.Scale(hr,hr);
				}
			}
			break;
		}
		
		case PM_SCALE_TO_FIT:
		{
			pictpoly=inSet.GetPictureMatrix().TransformVector(pictpoly);
			scalemat.Scale(inDisplayRect.GetWidth()/pictpoly.GetWidth(),inDisplayRect.GetHeight()/pictpoly.GetHeight());
			break;
		}
		case PM_CENTER:
		case PM_TOPLEFT:
		case PM_TILE:
		case PM_MATRIX:
		{
			// identity
			break;
		}
	}
	
	// calcul de la matrice final, relatif a rectange de destination
	
	pictpoly.FromVRect(pictrect);
	
	switch(inSet.GetScaleMode())
	{
		case PM_SCALE_CENTER:
		{
			pictpoly=inSet.GetPictureMatrix().TransformVector(pictpoly);

			pictpoly=scalemat.TransformVector(pictpoly);
			
			if(inDisplayRect.GetWidth()-pictpoly.GetWidth()>1)
			{
				scalemat.Translate((inDisplayRect.GetWidth()-pictpoly.GetWidth())/2,0,VAffineTransform::MatrixOrderAppend);
				scalemat =  inSet.GetPictureMatrix() * scalemat;
				
				_AjustFlip(scalemat);
				
			}
			else
			{
				scalemat.Translate(0,(inDisplayRect.GetHeight()-pictpoly.GetHeight())/2,VAffineTransform::MatrixOrderAppend);
				scalemat =  inSet.GetPictureMatrix() * scalemat;
				_AjustFlip(scalemat);
			}
			break;
		}
		
		case PM_4D_SCALE_CENTER:
		{
			pictpoly=inSet.GetPictureMatrix().TransformVector(pictpoly);

			pictpoly=scalemat.TransformVector(pictpoly);
			
			if(scalemat.GetScaleX()==1 && scalemat.GetScaleY()==1)
			{
				// pp image plus petite que le rectdest, on centre simplement
				scalemat = inSet.GetPictureMatrix()*scalemat;
			
				_AjustFlip(scalemat);

				VAffineTransform mat;
				
				mat.Translate(  ceil((inDisplayRect.GetWidth()-pictpoly.GetWidth())/2), ceil(((inDisplayRect.GetHeight()-pictpoly.GetHeight())/2)),VAffineTransform::MatrixOrderPrepend);

				scalemat*=mat;
			}
			else
			{
				if(inDisplayRect.GetWidth()-pictpoly.GetWidth()>1)
				{
					scalemat.Translate((inDisplayRect.GetWidth()-pictpoly.GetWidth())/2,0,VAffineTransform::MatrixOrderAppend);
					scalemat =  inSet.GetPictureMatrix() * scalemat;
					
					_AjustFlip(scalemat);
					
					/*if(inSet.GetYAxis())
					{
						pictpoly=scalemat.TransformPolygon(pictpoly);
						scalemat.Translate(0, (inDisplayRect.GetHeight()-(pictpoly.GetY()+pictpoly.GetHeight())),VAffineTransform::MatrixOrderAppend);
					}*/
				}
				else
				{
					scalemat.Translate(0,(inDisplayRect.GetHeight()-pictpoly.GetHeight())/2,VAffineTransform::MatrixOrderAppend);
					scalemat =  inSet.GetPictureMatrix() * scalemat;
					_AjustFlip(scalemat);
				}
			}
			break;
		}
		case PM_CENTER:
		{
			scalemat = inSet.GetPictureMatrix()*scalemat;
			
			_AjustFlip(scalemat);

			VAffineTransform mat;
			
			pictpoly=scalemat.TransformVector(pictpoly);
			mat.Translate( ceil((inDisplayRect.GetWidth()-pictpoly.GetWidth())/2), ceil(((inDisplayRect.GetHeight()-pictpoly.GetHeight())/2)),VAffineTransform::MatrixOrderPrepend);

			scalemat*=mat;
			
			break;
		}
		case PM_SCALE_TO_FIT:
		{
			scalemat = inSet.GetPictureMatrix() * scalemat;
			
			_AjustFlip(scalemat);

			if(inSet.GetYLocalAxisSwap() || inSet.GetXLocalAxisSwap() )
			{
				pictpoly=scalemat.TransformPolygon(pictpoly);
				inSet.GetYLocalAxisSwap() ? swp_y= (inDisplayRect.GetHeight()-(pictpoly.GetY()+pictpoly.GetHeight())) : swp_y=0;
				inSet.GetXLocalAxisSwap() ? swp_x= (inDisplayRect.GetWidth()-(pictpoly.GetX()+pictpoly.GetWidth())) : swp_x=0;
				scalemat.Translate(swp_x,swp_y ,VAffineTransform::MatrixOrderAppend);
			}
			break;
		}
		case PM_SCALE_EVEN:
		case PM_4D_SCALE_EVEN:
		{
			scalemat = scalemat * inSet.GetPictureMatrix();
			
			_AjustFlip(scalemat);

			if(inSet.GetXLocalAxisSwap() || inSet.GetYLocalAxisSwap() )
			{
				pictpoly=scalemat.TransformPolygon(pictpoly);
				inSet.GetYLocalAxisSwap() ? swp_y= (inDisplayRect.GetHeight()-(pictpoly.GetY()+pictpoly.GetHeight())) : swp_y=0;
				inSet.GetXLocalAxisSwap() ? swp_x= (inDisplayRect.GetWidth()-(pictpoly.GetX()+pictpoly.GetWidth())) : swp_x=0;
				scalemat.Translate(swp_x,swp_y ,VAffineTransform::MatrixOrderAppend);			
			}
			break;
		}
		case PM_MATRIX:
		{
			scalemat = scalemat * inSet.GetPictureMatrix();
			if(inSet.GetXLocalAxisSwap() || inSet.GetYLocalAxisSwap())
			{
				pictpoly=scalemat.TransformPolygon(pictpoly);
				inSet.GetYLocalAxisSwap() ? swp_y= (inDisplayRect.GetHeight()-(pictpoly.GetY()+pictpoly.GetHeight()))-pictpoly.GetY() : swp_y=0;
				inSet.GetXLocalAxisSwap() ? swp_x= (inDisplayRect.GetWidth()-(pictpoly.GetX()+pictpoly.GetWidth()))-pictpoly.GetX() : swp_x=0;
				scalemat.Translate(swp_x,swp_y ,VAffineTransform::MatrixOrderAppend);				
			}
			break;
		}
		case PM_TILE:
		case PM_TOPLEFT:
		{
			scalemat = scalemat * inSet.GetPictureMatrix();
			
			_AjustFlip(scalemat);
			
			if(inSet.GetXLocalAxisSwap() || inSet.GetYLocalAxisSwap())
			{
				pictpoly=scalemat.TransformPolygon(pictpoly);
				inSet.GetYLocalAxisSwap() ? swp_y= (inDisplayRect.GetHeight()-(pictpoly.GetY()+pictpoly.GetHeight()))-pictpoly.GetY() : swp_y=0;
				inSet.GetXLocalAxisSwap() ? swp_x= (inDisplayRect.GetWidth()-(pictpoly.GetX()+pictpoly.GetWidth()))-pictpoly.GetX() : swp_x=0;
				scalemat.Translate(swp_x,swp_y ,VAffineTransform::MatrixOrderAppend);				
			}
			break;
		}
	}
	
	if(inAutoSwapAxis)
	{
		GReal tx=inDisplayRect.GetX(),ty=inDisplayRect.GetY();
		
		if(inSet.GetYAxisSwap())
			ty = inSet.GetYAxisSwap()-(inDisplayRect.GetY() + inDisplayRect.GetHeight());
		
		if(inSet.GetXAxisSwap())
			tx = inSet.GetXAxisSwap()-(inDisplayRect.GetX() + inDisplayRect.GetWidth());
		
		scalemat.Translate(tx,ty,VAffineTransform::MatrixOrderAppend);
	}
	
	return scalemat;
}
#endif

VPictureData_Animator::VPictureData_Animator()
{
	fTimer=0;
	fPlaying=false;
	fLastTick=0;
	fCurrentFrame=0;
	fLastFrame=-1;
	fPictData=0;
	fLoopCount=0;
	fFrameCount=0;
}
VPictureData_Animator::~VPictureData_Animator()
{
	AttachPictureData(0);
}

VPictureData_Animator::VPictureData_Animator(const VPictureData_Animator& inAnimator)
{
	CopyFrom(inAnimator);
}

VPictureData_Animator& VPictureData_Animator::operator=(const VPictureData_Animator& inAnimator)
{
	ReleaseRefCountable(&fPictData);
	CopyFrom(inAnimator);
	return *this;
}

void VPictureData_Animator::CopyFrom(const VPictureData_Animator& inAnimator)
{
	fTimer=inAnimator.fTimer;
	fLastTick=inAnimator.fLastTick;
	fLastFrame=inAnimator.fLastFrame;
	
	fPictData=inAnimator.fPictData;
	
	if(fPictData)
		fPictData->Retain();
	
	fTimeLine=inAnimator.fTimeLine;
	fLoopCount=inAnimator.fLoopCount;
	fPlaying=inAnimator.fPlaying;
	fCurrentLoop=inAnimator.fCurrentLoop;
	fFrameCount=inAnimator.fFrameCount;
	fFrameCache=inAnimator.fFrameCache;
}

void VPictureData_Animator::AttachPictureData(const VPictureData* inPictData)
{
	if(inPictData==fPictData)
		return;
	if(fPictData)
	{
		fPictData->Release();
		fPictData=0;
	}
	
	if(inPictData)
	{
		inPictData->Retain();
		fTimer=0;
		fPlaying=false;
		fLastTick=0;
		fCurrentFrame=0;
		fCurrentLoop=0;
		fLastFrame=-1;
		fPictData=inPictData;
		fPictData->BuildTimeLine(fTimeLine,fLoopCount);	
	}
	else
	{
		fTimeLine.clear();
	}
}

void VPictureData_Animator::AttachPictureData(const VPictureData* inPictData,sLONG inFrameCount,sLONG inDelay,sLONG inLoopCount)
{
	if(inPictData==fPictData)
		return;
	if(fPictData)
	{
		fPictData->Release();
		fPictData=0;
	}
	
	if(inPictData)
	{
		
		fFrameCache.clear();
		
		inPictData->Retain();
		fPictData=inPictData;

		fTimer=0;
		fPlaying=false;
		fLastTick=0;
		fCurrentFrame=0;
		fCurrentLoop=0;
		fLastFrame=-1;
		fPictData=inPictData;
		fFrameCount = inFrameCount;
		
		sLONG frameheight=fPictData->GetHeight()/fFrameCount;

		XBOX::VRect subrect(0,0,fPictData->GetWidth(),frameheight);		
		
		VPicture tmppict;
		for(sLONG i=0;i<fFrameCount;i++)
		{
			fTimeLine.push_back(inDelay);
			VPictureData* sub=fPictData->CreateSubPicture(subrect,NULL);
			tmppict.FromVPictureData(sub);
			fFrameCache.push_back(tmppict);
			ReleaseRefCountable(&sub);
			subrect.SetPosBy(0,frameheight);
		}
		fCurrentLoop = inLoopCount;
	}
	else
	{
		fTimeLine.clear();
		fFrameCache.clear();
	}
}

sLONG VPictureData_Animator::GetFrameCount()
{
	if(fPictData!=0)
		 return (sLONG) fTimeLine.size();
	else
		return 0;
}

#if !VERSION_LINUX
void VPictureData_Animator::Draw(sLONG inFrameNumber,PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet)
{
	VPictureDrawSettings set(inSet);
	if(fFrameCount==0) // true gif
	{
		set.SetFrameNumber(inFrameNumber);
		if(fPictData) 
			fPictData->DrawInPortRef(inPortRef,r,&set);
	}
	else
	{
		if(inFrameNumber<0 || inFrameNumber>fFrameCount)
			inFrameNumber=0;
		if(fFrameCache.size())
		{
			if(!fFrameCache[inFrameNumber].IsNull()) 
				fFrameCache[inFrameNumber].Draw(inPortRef,r,&set);
		}
		else
		{
			VRect destrect=r;
			destrect.SetHeight(r.GetHeight()*fFrameCount);
			destrect.SetPosBy(0,-inFrameNumber * r.GetHeight());
			if(fPictData) 
				fPictData->DrawInPortRef(inPortRef,destrect,&set);
		}
	}
}
void VPictureData_Animator::Draw(sLONG inFrameNumber,VGraphicContext* inGraphicContext,const VRect& r,VPictureDrawSettings* inSet)
{
	VPictureDrawSettings set(inSet);
	if(fFrameCount==0) // true gif
	{
		set.SetFrameNumber(inFrameNumber);
		if(fPictData) 
			inGraphicContext->DrawPictureData( fPictData,r,&set);
	}
	else
	{
		if(inFrameNumber<0 || inFrameNumber>fFrameCount)
			inFrameNumber=0;
		if(fFrameCache.size())
		{
			if(!fFrameCache[inFrameNumber].IsNull()) 
				inGraphicContext->DrawPicture(fFrameCache[inFrameNumber],r,&set);
		}
		else
		{
			VRect destrect=r;
			destrect.SetHeight(r.GetHeight()*fFrameCount);
			destrect.SetPosBy(0,-inFrameNumber * r.GetHeight());
			if(fPictData) 
				inGraphicContext->DrawPictureData( fPictData,destrect,&set);
		}
	}
}
void VPictureData_Animator::Draw(PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet)
{
	Draw(fCurrentFrame,inPortRef,r,inSet);
}
void VPictureData_Animator::Draw(VGraphicContext* inGraphicContext,const VRect& r,VPictureDrawSettings* inSet)
{
	Draw(fCurrentFrame,inGraphicContext,r,inSet);
}
#endif

bool VPictureData_Animator::Idle()
{
	bool needredraw=false;
	sLONG timeline= (sLONG) fTimeLine.size();
	if(fPlaying && timeline)
	{
		uLONG cur=VSystem::GetCurrentTime();
		fTimer-=(cur - fLastTick);
		if(fTimer<=0)
		{
			needredraw=true;
			
			fCurrentFrame++;
			if(fCurrentFrame>=timeline)
			{
				if(fLoopCount==0)
				{
					fCurrentFrame=0;
				}
				else
				{
					fCurrentLoop++;
					if(fCurrentLoop>=fLoopCount)
					{
						fPlaying=false;
						fCurrentFrame=GetFrameCount()-1;
					}
					else
					{
						fCurrentFrame=0;
					}
				}
			}
			
			fTimer=fTimeLine[fCurrentFrame];
		}
		fLastTick=cur;
	}
	return needredraw;
}

bool VPictureData_Animator::Start()
{
	if(fTimeLine.size()>1)
	{
		fCurrentFrame=0;
		fCurrentLoop=0;
		fLastTick=VSystem::GetCurrentTime();
		fTimer=fTimeLine[fCurrentFrame];
		fPlaying=true;
	}
	else
		fPlaying=false;
	return fPlaying;
}
void VPictureData_Animator::Stop()
{
	fPlaying=false;
}



void xGifFrame::SetGif89FrameInfo(xGif89FrameInfo& inInfo)
{
	isGif89=true;
	fGif89=inInfo;
}

xGifFrame::xGifFrame(sWORD inX,sWORD inY,sWORD inWidth,sWORD inHeight,bool inAllocBitmap)
{
	fPixelsBuffer = 0;
	if(inAllocBitmap)
		fPixelsBuffer=(uCHAR*) VMemory::NewPtr(inWidth * inHeight, 'gif ');
	fWidth = inWidth;
	fHeight = inHeight;
	fX=inX;
	fY=inY;
	fColorsTotal = 0;
	fTransparent = (-1);
	fInterlace = 0;
	isGif89=false;
}
xGifFrame::~xGifFrame()
{
	if(fPixelsBuffer)
		VMemory::DisposePtr((VPtr)fPixelsBuffer);
}
void xGifFrame::FreePixelBuffer()
{
	if(fPixelsBuffer)
		VMemory::DisposePtr((VPtr)fPixelsBuffer);
	fPixelsBuffer=0;
}

VColor xGifFrame::GetPaletteEntry(sWORD index)
{
	VColor c=fPalette.GetNthEntry(index);
	if(isGif89 && fGif89.transparent && fGif89.transparent_index==index)
	{
		 c.SetAlpha(0);      
	}
	return c;
}

xGifInfo::xGifInfo()
{
	fLoopCount=1;
	fStream=0;
	_Init();
}
void xGifInfo::FromVStream(VStream* inStream)
{
	
	fStream=inStream;
	fStream->OpenReading();
	_Init();
	
	sWORD imageNumber;
	sWORD BitPixel;
	sWORD ColorResolution;
	sWORD Background;
	sWORD AspectRatio;
	//sWORD Transparent = (-1);
	uCHAR   buf[16];
	uCHAR   c;
	//uCHAR   ColorMap[3][xGifInfo_MaxColorMapSize];
	//uCHAR   localColorMap[3][xGifInfo_MaxColorMapSize];
	
	xGifColorPalette LocalColorMap;
	
	sWORD             imw, imh,imx,imy;
	sWORD             useGlobalColormap;
	sWORD             bitPixel;
	sWORD             imageCount = 0;
	char				version[4];
	sWORD				globalbitsperpixel;	 // added for ACI PACK
	xGif89FrameInfo		gif89;
	bool				gif89isvalid;
	xGifFrame* im = 0;
	ZeroDataBlock = false;

	imageNumber = 0;
	if (! ReadOK(buf,6)) {
		//return 0;
	}
	if (strncmp((char *)buf,"GIF",3) != 0) {
		//return 0;
	}
	strncpy(version, (char *)buf + 3, 3);
	version[3] = '\0';

	if ((strcmp(version, "87a") != 0) && (strcmp(version, "89a") != 0)) {
		//return 0;
	}
	if (! ReadOK(buf,7)) {
		//return 0;
	}
	fWidth = buf[0] | (buf[1] << 8);
	fHeight = buf[2] | (buf[3] << 8 );
	
	BitPixel        = 2<<(buf[4]&0x07);
	ColorResolution = (sWORD) (((buf[4]&0x70)>>3)+1);
	Background      = buf[5];
	AspectRatio     = buf[6];

	globalbitsperpixel = (buf[4] & 0x07) + 1;	 // added for ACI PACK
		
	if (xGifInfo_BitSet(buf[4], xGifInfo_LOCALCOLORMAP)) {    /* Global Colormap */
		if (ReadColorMap( BitPixel, fGlobalPalette)) 
		{
			//return 0;
		}
 	}
 	gif89isvalid=false;
	for (;;) {
		
		if (! ReadOK(&c,1)) {
			break;
		}
		if (c == ';') {         /* GIF terminator */
			sWORD i;
			if (imageCount <= imageNumber) {
				break;
			}
			/* Terminator before any image was declared! */
			if (!im) 
			{
				//return 0;
			}
			/* Check for open colors at the end, so
		          we can reduce colorsTotal and ultimately
		          BitsPerPixel */
		       /*for (i=((im->fColorsTotal-1)); (i>=0); i--) 
		       {
					if (im->fPalette.IsUsed(i)) 
					{
						im->fColorsTotal--;
		            } 
		            else 
		            {
						break;
		            }
		       } */
		       //return (Handle)imarray;
		}

		if (c == '!') /* Extension */
		{         
		      if (! ReadOK(&c,1)) 
		      {
		         break;
		      }
		      if(DoExtension( c,&gif89))
				gif89isvalid=true;
		      continue;
		}

		if (c != ',') {         /* Not a valid start character */
		       continue;
		}

		++imageCount;
		++imageNumber;
		if (! ReadOK(buf,9)) {
		   break;
		}

		useGlobalColormap = ! xGifInfo_BitSet(buf[8], xGifInfo_LOCALCOLORMAP);

		bitPixel = 1<<((buf[8]&0x07)+1);

		imx = xGifInfo_LM_to_uint(buf[0],buf[1]);
		imy = xGifInfo_LM_to_uint(buf[2],buf[3]);
		imw = xGifInfo_LM_to_uint(buf[4],buf[5]);
		imh = xGifInfo_LM_to_uint(buf[6],buf[7]);
		if (!(im = gdImageCreate(imx,imy,imw, imh))) {
			break;
		}
		if(gif89isvalid)
		{
			im->SetGif89FrameInfo(gif89);
			gif89isvalid=false;
		}
		fFrameList.push_back(im);
		im->fInterlace = xGifInfo_BitSet(buf[8], xGifInfo_INTERLACE);
		if (! useGlobalColormap) {
				im->fBitsPerPixel = (buf[8] & 0x07) + 1;
		       if (ReadColorMap( bitPixel, LocalColorMap)) {
		       		gdImageDestroy(im);
		                 break;
		       }
		       
				//if (/*table && gifstack*/1)	 // test added for ACI PACK
				{
		       		ReadImage(im, imw, imh, LocalColorMap, 
		                 xGifInfo_BitSet(buf[8], xGifInfo_INTERLACE), 
		                 imageCount != imageNumber);
		                
		        }
		} 
		else 
		{
			im->fBitsPerPixel = globalbitsperpixel;	
		    ReadImage(im, imw, imh,
		                 fGlobalPalette, 
		                 xGifInfo_BitSet(buf[8], xGifInfo_INTERLACE), 
		                 imageCount != imageNumber);
		                 
		}   
	}
	fStream->CloseReading();
}
xGifInfo::~xGifInfo()
{
	for(sLONG i=0;i<fFrameList.size();i++)
		delete fFrameList[i];
}
sWORD xGifInfo::ReadOK(void *buffer,sLONG len)
{
	VError err;
	VSize l=len;
	if(fStream->GetSize()>fStream->GetPos())
		err=fStream->GetData((sCHAR*)buffer,&l);	
	else
		err=VE_STREAM_EOF;
		
	return err==VE_OK;
}

sWORD xGifInfo::ReadColorMap(sWORD number, uCHAR (*buffer)[256])
{
       sWORD             i;
       uCHAR   rgb[3];


       for (i = 0; i < number; ++i) {
               if (! ReadOK(rgb, sizeof(rgb))) {
                       return true;
               }
               buffer[xGifInfo_CM_RED][i] = rgb[0] ;
               buffer[xGifInfo_CM_GREEN][i] = rgb[1] ;
               buffer[xGifInfo_CM_BLUE][i] = rgb[2] ;
       }


       return false;
}

sWORD xGifInfo::ReadColorMap(sWORD number, xGifColorPalette& inMap)
{
    sWORD             i;
    uCHAR   rgb[3];

	inMap.Reset();
    for (i = 0; i < number; ++i) 
    {
       if (! ReadOK(rgb, sizeof(rgb))) 
       {
          return true;
       }
       inMap.AppendColor(rgb[0],rgb[1],rgb[2]);
    }
    return false;
}

sWORD xGifInfo::DoExtension(sWORD label,xGif89FrameInfo* Gif89)
{
      sWORD result=false;
       static uCHAR     buf[256];

       switch (label) 
       {
			case 0xf9:              /* Graphic Control Extension */
			{
				(void) GetDataBlock((uCHAR*) buf);
				Gif89->disposal    = (buf[0] >> 2) & 0x7;
				Gif89->inputFlag   = (buf[0] >> 1) & 0x1;
				Gif89->delayTime   = xGifInfo_LM_to_uint(buf[1],buf[2]);
				Gif89->transparent =	((buf[0] & 0x1) != 0);
				if(Gif89->transparent)
				{
					Gif89->transparent_index=buf[3];
				}	
				result=true;
				break;
              }
			case 0xff: /* Application Extension */
			{
				sWORD extdatasize,subdatasize;
				extdatasize=GetDataBlock((uCHAR*)buf);
				if(extdatasize>=11)
				{
					if (strncmp((const char*)buf, "NETSCAPE2.0",11) == 0)
					{
						subdatasize=GetDataBlock((uCHAR*)buf);
						if(subdatasize==3)
						{
							fLoopCount=buf[1];
						}
					}
				}
				break;
			}
			default:
               break;
       }
       while (GetDataBlock( (uCHAR*) buf) > 0)
               ;

       return result;
}

sWORD xGifInfo::GetDataBlock(uCHAR *buf)
{
       uCHAR   count;

       if (! ReadOK(&count,1)) {
               return -1;
       }

       ZeroDataBlock = count == 0;

       if ((count != 0) && (! ReadOK(buf, count))) {
               return -1;
       }

       return count;
}

sWORD xGifInfo::GetCode(sWORD code_size, sWORD flag)
{
       static uCHAR    buf[280];
       static sWORD              curbit, lastbit, done, last_byte;
       sWORD                     i, j, ret;
       uCHAR           count;

       if (flag) {
               curbit = 0;
               lastbit = 0;
               done = false;
               return 0;
       }

       if ( (curbit+code_size) >= lastbit) {
               if (done) {
                       if (curbit >= lastbit) {
                                /* Oh well */
                       }                        
                       return -1;
               }
               buf[0] = buf[last_byte-2];
               buf[1] = buf[last_byte-1];

               if ((count = GetDataBlock(&buf[2])) == 0)
                       done = true;

               last_byte = 2 + count;
               curbit = (curbit - lastbit) + 16;
               lastbit = (2+count)*8 ;
       }

       ret = 0;
       for (i = curbit, j = 0; j < code_size; ++i, ++j)
               ret |= ((buf[ i / 8 ] & (1 << (i % 8))) != 0) << j;

       curbit += code_size;

       return ret;
}

sWORD xGifInfo::LWZReadByte( sWORD flag, sWORD input_code_size)
{
	static sWORD      fresh = false;
	sWORD             code, incode;
	static sWORD      code_size, set_code_size;
	static sWORD      max_code, max_code_size;
	static sWORD      firstcode, oldcode;
	static sWORD      clear_code, end_code;
	static sWORD      table[2][(1<< xGifInfo_MAX_LWZ_BITS)];			// for ACI PACK table is allocated
	static sWORD      gifstack[(1<<(xGifInfo_MAX_LWZ_BITS))*2], *sp;		// for ACI PACK table is allocated
	register sWORD    i;

       if (flag) {
               set_code_size = input_code_size;
               code_size = set_code_size+1;
               clear_code = 1 << set_code_size ;
               end_code = clear_code + 1;
               max_code_size = 2*clear_code;
               max_code = clear_code+2;

               GetCode(0, true);
               
               fresh = true;

               for (i = 0; i < clear_code; ++i) {
                       table[0][i] = 0;
                       table[1][i] = i;
               }
               for (; i < (1<<xGifInfo_MAX_LWZ_BITS); ++i)
                       table[0][i] = table[1][0] = 0;

               sp = gifstack;

               return 0;
       } else if (fresh) {
               fresh = false;
               do {
                       firstcode = oldcode =
                               GetCode(code_size, false);
               } while (firstcode == clear_code);
               return firstcode;
       }

       if (sp > gifstack)
               return *--sp;

       while ((code = GetCode( code_size, false)) >= 0) {
               if (code == clear_code) {
                       for (i = 0; i < clear_code; ++i) {
                               table[0][i] = 0;
                               table[1][i] = i;
                       }
                       for (; i < (1<<xGifInfo_MAX_LWZ_BITS); ++i)
                               table[0][i] = table[1][i] = 0;
                       code_size = set_code_size+1;
                       max_code_size = 2*clear_code;
                       max_code = clear_code+2;
                       sp = gifstack;
                       firstcode = oldcode =
                                       GetCode( code_size, false);
                       return firstcode;
               } else if (code == end_code) {
                       sWORD             count;
                       uCHAR   buf[260];

                       if (ZeroDataBlock)
                               return -2;

                       while ((count = GetDataBlock( buf)) > 0)
                               ;

                       if (count != 0)
                       return -2;
               }

               incode = code;

               if (code >= max_code) {
                       *sp++ = firstcode;
                       code = oldcode;
               }

               while (code >= clear_code) {
                       *sp++ = table[1][code];
                       if (code == table[0][code]) {
                               /* Oh well */
                       }
                       code = table[0][code];
               }

               *sp++ = firstcode = table[1][code];

               if ((code = max_code) <(1<<xGifInfo_MAX_LWZ_BITS)) {
                       table[0][code] = oldcode;
                       table[1][code] = firstcode;
                       ++max_code;
                       if ((max_code >= max_code_size) &&
                               (max_code_size < (1<<xGifInfo_MAX_LWZ_BITS))) {
                               max_code_size *= 2;
                               ++code_size;
                       }
               }

               oldcode = incode;

               if (sp > gifstack)
                       return *--sp;
       }
       return code;
}

void xGifInfo::ReadImage(xGifFrame* im, sWORD len, sWORD height, xGifColorPalette &inPalette, sWORD interlace, sWORD ignore)
{
       uCHAR   c;      
       sWORD             v;
       sWORD             xpos = 0, ypos = 0, pass = 0;
       sWORD i;
       
       im->fPalette=inPalette;
       im->fPalette.SetUsedState(1);
       /* Many (perhaps most) of these colors will remain marked open. */
       im->fColorsTotal = 0;//xGifInfo_MaxColors;
       /*
       **  Initialize the Compression routines
       */
       if (! ReadOK(&c,1)) {
               return; 
       }
       if (LWZReadByte(true, c) < 0) {
               return;
       }

       /*
       **  If this is an "uninteresting picture" ignore it.
       */
       if (ignore) {
               while (LWZReadByte( false, c) >= 0)
                       ;
               return;
       }

       while ((v = LWZReadByte(false,c)) >= 0 ) {
               /* This how we recognize which colors are actually used. */
          
               im->fPalette.SetUsedState(1,v);
               gdImageSetPixel(im, xpos, ypos, v);
               ++xpos;
               if (xpos == len) {
                       xpos = 0;
                       if (interlace) {
                               switch (pass) {
                               case 0:
                               case 1:
                                       ypos += 8; break;
                               case 2:
                                       ypos += 4; break;
                               case 3:
                                       ypos += 2; break;
                               }

                               if (ypos >= height) {
                                       ++pass;
                                       switch (pass) {
                                       case 1:
                                               ypos = 4; break;
                                       case 2:
                                               ypos = 2; break;
                                       case 3:
                                               ypos = 1; break;
                                       default:
                                               goto fini;
                                       }
                               }
                       } else {
                               ++ypos;
                       }
               }
               if (ypos >= height)
                       break;
       }

fini:
	for (i=0;i<im->fPalette.GetCount();i++) 
	{
		if (im->fPalette.IsUsed(i)) 
		{
			im->fColorsTotal++;
		} 
	} 
	if (LWZReadByte(false,c)>=0) 
    {
               /* Ignore extra */
    }
}

xGifFrame* xGifInfo::gdImageCreate(sWORD sx, sWORD sy,sWORD sw, sWORD sh)	// adapted ACI PACK
{
	return new xGifFrame(sx,sy,sw,sh,DoAllocPixBuffer());
}

void xGifInfo::gdImageDestroy(xGifFrame* im)	// adapted ACI PACK
{
	if (!im) 
		return;
	delete im;
}

sWORD xGifInfo::gdImageBoundsSafe(xGifFrame* im, sWORD x, sWORD y)
{
	return (!(((y < 0) || (y >= im->fHeight)) ||
		((x < 0) || (x >= im->fWidth))));
}

void xGifInfo::gdImageSetPixel(xGifFrame* im, sWORD x, sWORD y, sWORD color)	// adapted ACI PACK
{
	if (gdImageBoundsSafe(im, x, y) && im->fPixelsBuffer)
	{
		im->fPixelsBuffer[x + (y*im->fWidth)]=color;
	}
}

/******************************************************************/

void VPictureData::_SetSafe() const
{
	_LoadProc SafeProc=&VPictureData::_SafeLoad;
	VInterlocked::ExchangePtr( (uLONG_PTR**)&fLoadProc ,*((uLONG_PTR**)(&SafeProc)) );
}


VPictureQDBridgeBase* VPictureData::sQDBridge=NULL;
VMacHandleAllocatorBase* VPictureData::sMacAllocator=NULL;


#if !VERSION_LINUX
VPictureData* VPictureData::CreatePictureDataFromPortRef(PortRef inPortRef,const VRect& inBounds)
{
	VPictureData *data=NULL;
	
#if VERSIONMAC 
	#if WITH_QUICKDRAW
		data=new VPictureData_MacPicture(inPortRef,inBounds);
	#endif
#elif VERSIONWIN
	data=new VPictureData_GDIPlus(inPortRef,inBounds);
#endif
	
	return data;
}
#endif

sLONG	VPictureData::Retain(const char* DebugInfo) const
{
	return IRefCountable::Retain(DebugInfo);
}

sLONG	VPictureData::Release(const char* DebugInfo) const
{
	return IRefCountable::Release(DebugInfo);
}

VString VPictureData::GetEncoderID() const
{
	VString res;
	if(fDecoder)
		res=fDecoder->GetDefaultIdentifier();
	else
		res= sCodec_none;
	return res;

}

bool VPictureData::IsKind(const VString& inID)const
{
	if(fDecoder)
	{
		return fDecoder->CheckIdentifier(inID);
	}
	else
		assert(false);
	return false;
}

VPictureData::VPictureData(VPictureDataProvider* inDataProvider,_VPictureAccumulator* inRecorder)
{
	_InitSafeProc(false);
	fDataSourceDirty=false;
	fDataProvider=NULL;
	fDecoder=0;
	fMetadatas = NULL;
	fDataSourceMetadatasDirty = false;
	fBounds.SetCoords(0,0,0,0);
	SetDataProvider(inDataProvider);
	
	if(inRecorder)
	{
		VIndex outIndex;
		inRecorder->AddPictureData(this,outIndex);
	}
}

VPictureData::VPictureData()
{
	_InitSafeProc(false);
	fDataSourceDirty=false;
	fDataProvider=NULL;
	fMetadatas = NULL;
	fDataSourceMetadatasDirty = false;
	fBounds.SetCoords(0,0,0,0);
	fDecoder=0;

	_SetDecoderByExtension(sCodec_membitmap);
}

VPictureData::VPictureData(const VPictureData& inData)
{
	_InitSafeProc(false);
	fDataSourceDirty=false;
	fDataProvider=NULL;
	fMetadatas = NULL;
	fDataSourceMetadatasDirty = false;
	fBounds.SetCoords(0,0,0,0);
	fDecoder=inData.RetainDecoder();

	SetDataProvider(inData.GetDataProvider());
}

VPictureData::~VPictureData()
{
	//release metadatas
	if (fMetadatas)
	{
		fMetadatas->Release();
		fMetadatas = NULL;
	}
	
	_ReleaseDataProvider();

	if(fDecoder)
		fDecoder->Release();
}

VPictureData* VPictureData::CreatePictureDataFromVFile(VFile& inFile)
{	
	VPictureCodecFactoryRef fact;
	return fact->CreatePictureData(inFile);
}

VPictureData* VPictureData::Clone()const
{
	return new VPictureData(*this);
}

const class VPictureCodec* VPictureData::RetainDecoder() const
{
	if(fDecoder)
	{
		fDecoder->Retain();
		return fDecoder;
	}
	else
		return 0;
}

void VPictureData::_SetDecoderByMime(const VString& inMime)
{
	VPictureCodecFactoryRef fact;
	_ReleaseDecodeur();
	fDecoder=fact->RetainDecoderForMimeType(inMime);
}

void VPictureData::_SetDecoderByExtension(const VString& inExt)
{
	VPictureCodecFactoryRef fact;
	_ReleaseDecodeur();
	fDecoder=fact->RetainDecoderForExtension(inExt);
}

void VPictureData::_SetAndRetainDecoder(const VPictureCodec* inDecodeur)
{
	_ReleaseDecodeur();
	if(inDecodeur)
	{
		fDecoder=inDecodeur;
		fDecoder->Retain();
	}
}

void VPictureData::_SetRetainedDecoder(const VPictureCodec* inDecodeur)
{
	_ReleaseDecodeur();
	if(inDecodeur)
	{
		fDecoder=inDecodeur;
	}
}

void VPictureData::_ReleaseDecodeur()
{
	if(fDecoder)
		fDecoder->Release();
	fDecoder=0;
}

VPictureData*	VPictureData::CreateSubPicture(VRect& inSubRect,VPictureDrawSettings* inSet)const
{
	VPictureData* result=0;
	#if VERSIONWIN
	Gdiplus::Bitmap* bm=CreateGDIPlus_SubBitmap(inSet,&inSubRect);
	if(bm)
	{
		result=new VPictureData_GDIPlus(bm,true);
	}
	#elif VERSIONMAC
	CGImageRef bm=CreateCGImage(inSet,&inSubRect);
	if(bm)
	{
		result=new VPictureData_CGImage(bm);
		CGImageRelease(bm);
	}
	#endif
	return result;
}

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
bool VPictureData::Compare( const VPictureData *inPictData, VPictureData **outMask) const
{
	xbox_assert(inPictData);
	_Load();
	inPictData->_Load();

	sLONG width = GetWidth();
	sLONG height = GetHeight();
	if (width != inPictData->GetWidth() || height != inPictData->GetHeight())
	{
		if (outMask)
			*outMask = NULL;
		return false;
	}
	if (!width || !height)
	{
		if (outMask)
			*outMask = new VPictureData( *this);
		return true;
	}

	VBitmapData *bmpData = CreateVBitmapData();
	VBitmapData *bmpData2 = inPictData->CreateVBitmapData();
	if (!bmpData || !bmpData2)
	{
		if (bmpData)
			delete bmpData;
		if (bmpData2)
			delete bmpData2;
		if (outMask)
			*outMask = NULL;
		return false;
	}
	VBitmapData *outBmpData = NULL;
	bool isEqual = bmpData->Compare( bmpData2, outMask ? &outBmpData : NULL);
	if (outMask)
	{
		if (outBmpData)
		{
			*outMask = outBmpData->CreatePictureData();
			delete outBmpData;
		}
		else
			*outMask = new VPictureData();
	}
	delete bmpData;
	delete bmpData2;

	return isEqual;
}
#endif


#if VERSIONWIN

Gdiplus::Image*		VPictureData::GetGDIPlus_Image() const
{
	if(GetGDIPlus_Bitmap())
		return GetGDIPlus_Bitmap();
	else if(GetGDIPlus_Metafile())
		return GetGDIPlus_Metafile();
	else
		return 0;		
};

HENHMETAFILE	VPictureData::CreateMetafile(VPictureDrawSettings* inSet)const
{
	VPictureDrawSettings set(inSet);
	HENHMETAFILE result=0;
	
	if (!IsRenderable())
		return NULL;

	if(GetMetafile())
	{
		if(set.IsIdentityMatrix())
		{
			result=::CopyEnhMetaFile(GetMetafile(),0);
		}
		else
		{
			Gdiplus::Metafile* tmpmeta=CreateGDIPlus_Metafile(&set);
			if(tmpmeta)
			{
				result=tmpmeta->GetHENHMETAFILE();
				delete tmpmeta;
			}
		}
	}
	else if(GetGDIPlus_Metafile())
	{
		if(set.IsIdentityMatrix())
		{
			Gdiplus::Metafile* clone=(Gdiplus::Metafile*)GetGDIPlus_Metafile()->Clone();
			result=clone->GetHENHMETAFILE();
			delete clone;
		}
		else
		{
			Gdiplus::Metafile* tmpmeta=CreateGDIPlus_Metafile(&set);
			if(tmpmeta)
			{
				result=tmpmeta->GetHENHMETAFILE();
				delete tmpmeta;
			}
		}
	}
	else if(GetGDIPlus_Bitmap())
	{
		Gdiplus::Metafile* tmpmeta=CreateGDIPlus_Metafile(&set);
		if(tmpmeta)
		{
			result=tmpmeta->GetHENHMETAFILE();
			delete tmpmeta;
		}
	}
	else if(GetHBitmap())
	{
		Gdiplus::Metafile* tmpmeta=CreateGDIPlus_Metafile(&set);
		if(tmpmeta)
		{
			result=tmpmeta->GetHENHMETAFILE();
			delete tmpmeta;
		}
	}
	else if(GetPicHandle() && sQDBridge)
	{
		result=sQDBridge->ToMetaFile(GetPicHandle());
		if(!set.IsIdentityMatrix() && result)
		{
			HDC refdc;
			refdc=GetDC(NULL);
			VRect pictrect;
			VPolygon poly=set.TransformSize(GetWidth(),GetHeight());
			pictrect.SetCoords(0,0,poly.GetWidth(),poly.GetHeight());
			
			Gdiplus::RectF rf(0,0,poly.GetWidth(),poly.GetHeight());
			Gdiplus::Metafile *tmpmeta=new Gdiplus::Metafile(refdc,rf,Gdiplus::MetafileFrameUnitPixel);
			Gdiplus::Graphics graph(tmpmeta);
			
			//set.SetScaleMode(PM_MATRIX);
			
			set.SetScaleMode(PM_SCALE_TO_FIT );
			xDraw(result,&graph,pictrect,&set);
			ReleaseDC(NULL,refdc);
			DeleteEnhMetaFile(result);
			result=tmpmeta->GetHENHMETAFILE();
		}
	}
	else
	{
		// pp on a rien trouv√©, on cree un metafile et on dessine dedans
		Gdiplus::Metafile* tmpmeta=CreateGDIPlus_Metafile(&set);
		if(tmpmeta)
		{
			result=tmpmeta->GetHENHMETAFILE();
			delete tmpmeta;
		}
	}
	return result;
}

HBITMAP			VPictureData::CreateHBitmap(VPictureDrawSettings* inSet) const
{
	HBITMAP result=0;
	
	if (!IsRenderable())
		return NULL;

	VPictureDrawSettings set(inSet);
	set.SetBackgroundColor(VColor(0xff,0xff,0xff));// pp no alpha
	
	Gdiplus::Bitmap* bm=CreateGDIPlus_Bitmap(&set);
	if(bm)
	{
		bm->GetHBITMAP(Gdiplus::Color(0xff,0xff,0xff),&result);
		delete bm;
	}
	return result;
}

HBITMAP			VPictureData::CreateTransparentHBitmap(VPictureDrawSettings* inSet) const
{
	HBITMAP result = 0;

	if (!IsRenderable())
		return NULL;

	VPictureDrawSettings set(inSet);

	Gdiplus::Bitmap* bm = CreateGDIPlus_Bitmap(&set);
	if(bm)
	{
		bm->GetHBITMAP(Gdiplus::Color(0, 0, 0), &result);
		delete bm;
	}
	return result;
}


HICON VPictureData::CreateHIcon(VPictureDrawSettings* inSet)const
{
	HICON result = NULL;
	
	if (!IsRenderable())
		return NULL;

	Gdiplus::Bitmap *bm = CreateGDIPlus_Bitmap(inSet);
	if(bm)
	{
		bm->GetHICON(&result);
		delete bm;
	}
	return result;
}

Gdiplus::Image*	VPictureData::CreateGDIPlus_Image(VPictureDrawSettings* /*inSet*/) const
{
	assert(false);

	return 0;
}

Gdiplus::Bitmap* VPictureData::CreateGDIPlus_SubBitmap(VPictureDrawSettings* inSet,VRect* inSub) const
{
	Gdiplus::Bitmap* result=0;
	VPictureDrawSettings set(inSet);
	
	if (!IsRenderable())
		return NULL;
	
	set.DeviceCanUseClearType(false); //disable ClearType for offscreen rendering (used by SVG component)
	
	if(set.IsIdentityMatrix() && GetGDIPlus_Bitmap() && set.GetDrawingMode()!=3 && inSub==0)
	{
		Gdiplus::Bitmap* bm=GetGDIPlus_Bitmap();
		result=bm->Clone(0,0,GetWidth(),GetHeight(),bm->GetPixelFormat());
	}
	else
	{
		sLONG width,height;
		VRect r;
		VPolygon poly= set.TransformSize(GetWidth(),GetHeight());
		if(inSub)
		{
			width=inSub->GetWidth();
			height=inSub->GetHeight();
			r.SetCoords(-inSub->GetX(),-inSub->GetY(),width+inSub->GetX(),height+inSub->GetY());
			set.SetScaleMode(PM_TOPLEFT);
		}
		else
		{
			width=poly.GetWidth();
			height=poly.GetHeight();
			r.SetCoords(0,0,width,height);
			//set.SetScaleMode(PM_MATRIX);
			set.SetScaleMode(PM_SCALE_TO_FIT);
		}
		result=new Gdiplus::Bitmap(width,height,PixelFormat32bppARGB);
		Gdiplus::Graphics graph(result);
		graph.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
		
		if(set.GetDrawingMode()==3)
		{
			Gdiplus::Color col;
			col.SetFromCOLORREF(set.GetBackgroundColor().WIN_ToCOLORREF());
			Gdiplus::SolidBrush brush(col);
			Gdiplus::Rect loc(0,0,width,height);
			graph.FillRectangle(&brush,loc);
		}

		DrawInGDIPlusGraphics(&graph,r,&set);		
	}
	return result;
}

Gdiplus::Metafile* VPictureData::CreateGDIPlus_Metafile(VPictureDrawSettings* inSet) const
{
	Gdiplus::Metafile* result=0;
	VPictureDrawSettings set(inSet);
	
	if (!IsRenderable())
		return NULL;
	
	if(set.IsIdentityMatrix() && (GetGDIPlus_Metafile() || GetMetafile() || GetPicHandle()))
	{
		if(GetGDIPlus_Metafile())
			result=(Gdiplus::Metafile*)GetGDIPlus_Metafile()->Clone();
		else if(GetMetafile())
		{
			HENHMETAFILE meta=CreateMetafile();
			if(meta)
			{
				result=new Gdiplus::Metafile(meta,true);
			}
		}
		else if(GetPicHandle() && sQDBridge)
		{
			HENHMETAFILE meta=sQDBridge->ToMetaFile(GetPicHandle());
			if(meta)
			{
				result=new Gdiplus::Metafile(meta,true);
			}
		}
	}
	else
	{
		HDC refdc;
		refdc=GetDC(NULL);
		VRect pictrect;
		VPolygon poly=set.TransformSize(GetWidth(),GetHeight());
		Gdiplus::RectF rf(0,0,poly.GetWidth(),poly.GetHeight());
		
		pictrect.SetCoords(0,0,poly.GetWidth(),poly.GetHeight());
		
		result=new Gdiplus::Metafile(refdc,rf,Gdiplus::MetafileFrameUnitPixel);
		Gdiplus::Graphics graph(result);

		//set.SetScaleMode(PM_MATRIX);
		
		set.SetScaleMode(PM_SCALE_TO_FIT);
		DrawInGDIPlusGraphics(&graph,pictrect,&set);
		ReleaseDC(NULL,refdc);
	}
	return result;
}

#if WITH_VMemoryMgr
xMacPictureHandle	VPictureData::CreatePicHandle(VPictureDrawSettings* inSet,bool& outCanAddPicEnd) const
{
	void* result=NULL;
	VPictureDrawSettings set(inSet);
	
	if (!IsRenderable())
		return NULL;
	
	outCanAddPicEnd=true;
	set.SetBackgroundColor(VColor(0xff,0xff,0xff));// pp no alpha
	if(set.IsIdentityMatrix())
	{
		if(GetPicHandle())
		{
			result=GetMacAllocator()->Duplicate(GetPicHandle());
		}
		else if(GetGDIPlus_Bitmap() && sQDBridge)
		{
			UINT gdipformat=GetGDIPlus_Bitmap()->GetPixelFormat();
			if(!Gdiplus::IsAlphaPixelFormat(gdipformat))
			{
				HBITMAP bm;
				GetGDIPlus_Bitmap()->GetHBITMAP(Gdiplus::Color(0xff,0xff,0xff),&bm);
				if(bm)
				{
					result=sQDBridge->HBitmapToPicHandle(bm);
					DeleteObject(bm);
				}
			}
			else
			{
				Gdiplus::Bitmap* bm=CreateGDIPlus_Bitmap(&set);
				if(bm)
				{
					HBITMAP hbm;
					bm->GetHBITMAP(Gdiplus::Color(0xff,0xff,0xff),&hbm);
					if(hbm)
					{
						result=sQDBridge->HBitmapToPicHandle(hbm);
						DeleteObject(hbm);
					}
					delete bm;
				}
			}
		}
		else
		{
			if(sQDBridge)
			{
				Gdiplus::Bitmap* bm=CreateGDIPlus_Bitmap(&set);
				if(bm)
				{
					HBITMAP hbm;
					bm->GetHBITMAP(Gdiplus::Color(0xff,0xff,0xff),&hbm);
					if(hbm)
					{
						result=sQDBridge->HBitmapToPicHandle(hbm);
						DeleteObject(hbm);
					}
					delete bm;
				}
			}
		}
	}
	else
	{
		if(sQDBridge)
		{
			Gdiplus::Bitmap* bm=CreateGDIPlus_Bitmap(&set);
			if(bm)
			{
				HBITMAP hbm;
				bm->GetHBITMAP(Gdiplus::Color(0xff,0xff,0xff),&hbm);
				if(hbm)
				{
					result=sQDBridge->HBitmapToPicHandle(hbm);
					DeleteObject(hbm);
				}
				delete bm;
			}
		}
	}
	return (xMacPictureHandle)result;
}
#endif

VBitmapData*	VPictureData::CreateVBitmapData(const VPictureDrawSettings* inSet,VRect* inSub)const
{
	VBitmapData* result=0;
	
	if (!IsRenderable())
		return NULL;
	
	VPictureDrawSettings set(inSet);
	Gdiplus::Bitmap* bm=CreateGDIPlus_Bitmap(&set);
	if(bm)
	{
		result=new VBitmapData(*bm);
		delete bm;
	}
	return result;
}


#elif VERSIONMAC

VBitmapData*	VPictureData::CreateVBitmapData(const VPictureDrawSettings* inSet,VRect* inSub)const
{
	VBitmapData* result=0;
	
	if (!IsRenderable())
		return NULL;
	
	VPictureDrawSettings set(inSet);
	VPolygon poly=set.TransformSize(GetWidth(),GetHeight());
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
			
			::CGContextClearRect(context, vr);
				
			VRect pictrect(0,0,poly.GetWidth(),poly.GetHeight());
			set.SetScaleMode(PM_SCALE_TO_FIT);
			set.SetYAxisSwap(poly.GetHeight(),true);
			CGContextSaveGState(context);
			DrawInCGContext(context,pictrect,&set);
			CGContextRestoreGState(context);
			CGContextRelease(context);
			
			result= new VBitmapData (bitmapData,width,height,bitmapBytesPerRow,VBitmapData::VPixelFormat32bppPARGB);
			
			free(bitmapData);	
		}
	}
	return result;
}
	
CGImageRef		VPictureData::CreateCGImage(VPictureDrawSettings* inSet,VRect* inSub)const
{
	CGImageRef result=0; 
	
	if (!IsRenderable())
		return NULL;
	
	VPictureDrawSettings set(inSet);
	if(GetWidth() && GetHeight())
	{
		if(set.IsIdentityMatrix() && GetCGImage() && set.GetDrawingMode()!=3 && inSub==0)
		{
			result=CGImageCreateCopy(GetCGImage());
		}
		else
		{
			VPolygon poly=set.TransformSize(GetWidth(),GetHeight());
			CGContextRef    context = NULL;
			CGColorSpaceRef colorSpace;
			void *          bitmapData;
			int             bitmapByteCount;
			int             bitmapBytesPerRow;
			sLONG width,height;
			
			if(!inSub)
			{
				width=poly.GetWidth();
				height=poly.GetHeight();
			}
			else
			{
				width=inSub->GetWidth();
				height=inSub->GetHeight();
			}
			bitmapBytesPerRow   = ( 4 * width + 15 ) & ~15;
			bitmapByteCount     = (bitmapBytesPerRow * height);
			
			bitmapData = malloc( bitmapByteCount );
		
			if(bitmapData)
			{
				colorSpace =CGColorSpaceCreateDeviceRGB();

				memset(bitmapData,0,bitmapByteCount);
				context = CGBitmapContextCreate (bitmapData,
								width,
								height,
								8,      // bits per component
								bitmapBytesPerRow,
								colorSpace,
								kCGImageAlphaPremultipliedLast);
				if (context)
				{
					CGRect vr,contextrect;
					
					contextrect.origin.x= 0;
					contextrect.origin.y=  0;
					contextrect.size.width=width;
					contextrect.size.height=height;

					
					vr.origin.x= 0;
					vr.origin.y=  0;
					vr.size.width=poly.GetWidth();
					vr.size.height=poly.GetHeight();
			
					if(set.GetDrawingMode()==3)
					{
						VColor col=set.GetBackgroundColor();
						::CGContextSetRGBFillColor(context,col.GetRed()/(GReal)255,col.GetGreen()/(GReal)255,col.GetBlue()/(GReal)255,col.GetAlpha()/(GReal)255);
						::CGContextFillRect(context, contextrect);
					}
					else
					{
						::CGContextClearRect(context, contextrect);
						
					}
					
					VRect pictrect;
					
					
					if(inSub)
					{
						set.SetScaleMode(PM_TOPLEFT);
						set.SetYAxisSwap(height,true);
						//JQ 08/03/2010 fixed ACI0065123
						// for svg drawing we use temp XMacQuartzGraphicContext so here we need to feed pictrect with bitmap context bounds
						// in order XMacQuartzGraphicContext uses proper QD bounds (otherwise transform matrix from QD axis to Quartz axis is not correct)
						//if (IsKind( sCodec_svg))
						//	pictrect.SetCoords(-inSub->GetX(),-inSub->GetY(),width,height);
						//else 
						
						// pp 29/04/10 ACI0065123 + ACI0065882
						// Y axis info is in the draw settings set.SetYAxisSwap(height,true);
						// no special case for svg
						pictrect.SetCoords(-inSub->GetX(),-inSub->GetY(),poly.GetWidth(),poly.GetHeight());
					}
					else
					{
						set.SetScaleMode(PM_SCALE_TO_FIT);
						set.SetYAxisSwap(poly.GetHeight(),true);
						pictrect.SetCoords(0,0,poly.GetWidth(),poly.GetHeight());
					}
					CGContextSaveGState(context);
					
					DrawInCGContext(context,pictrect,&set);
					
					CGContextRestoreGState(context);
					CGDataProviderRef dataprov= xV4DPicture_MemoryDataProvider::CGDataProviderCreate((char*)bitmapData,bitmapByteCount,true);
			
					result=CGImageCreate(width,height,8,32,bitmapBytesPerRow,colorSpace,kCGImageAlphaPremultipliedLast,dataprov,0,0,kCGRenderingIntentDefault);
					CGContextRelease(context);
					CGDataProviderRelease(dataprov);
					if(set.GetDrawingMode()==1 && CGImageGetAlphaInfo(result)== kCGImageAlphaNone)
					{
						VColor col=set.GetTransparentColor();
						GReal MaskingColors[6];
						MaskingColors[0]=col.GetRed();
						MaskingColors[1]=col.GetGreen();
						MaskingColors[2]=col.GetBlue();
						MaskingColors[3]=col.GetRed();
						MaskingColors[4]=col.GetGreen();
						MaskingColors[5]=col.GetBlue();

						
						CGImageRef trans=CGImageCreateWithMaskingColors(result,MaskingColors);
						if(trans)
						{
							CFRelease(result);
							result=trans;
						}	
					}
				}
				CGColorSpaceRelease( colorSpace );
			}
			free(bitmapData);
		}
	}
	return result;
}


#if WITH_VMemoryMgr
xMacPictureHandle VPictureData::CreatePicHandle(VPictureDrawSettings* inSet,bool &outCanAddPicEnd)const
{
	xMacPictureHandle result=0;

	if (!IsRenderable())
		return NULL;

	outCanAddPicEnd=true;
	if(sQDBridge)
	{
		result = (xMacPictureHandle)sQDBridge->VPictureDataToPicHandle(*this, inSet);
	}
	return result;
}
#endif

static size_t MyPutBytes (void* info, const void* buffer, size_t count)// 1
{
    CFDataAppendBytes ((CFMutableDataRef) info,(const UInt8*) buffer, count);
    return count;
}


CGPDFDocumentRef		VPictureData::CreatePDF(VPictureDrawSettings* inSet,VPictureDataProvider** outDataProvider)const
{
	CGPDFDocumentRef result=NULL;
	VPictureDrawSettings set(inSet);
	
	if (!IsRenderable())
		return NULL;

	if(outDataProvider)
		*outDataProvider=NULL;
	
	if(GetWidth() && GetHeight())
	{
		
		CGDataConsumerCallbacks callbacks = { MyPutBytes, NULL };

		CFDataRef data = CFDataCreateMutable (kCFAllocatorDefault, 0);
		if (data != NULL)
		{
			CGDataConsumerRef consumer = NULL;
			consumer = CGDataConsumerCreate ((void*) data, &callbacks);// 3
			if (consumer != NULL) 
			{
				VPolygon poly=set.TransformSize(GetWidth(),GetHeight());
				CGContextRef context = NULL;
				CGRect bounds;
				
				bounds.origin.x = 0;
				bounds.origin.y = 0;
				bounds.size.width=poly.GetWidth();
				bounds.size.height=poly.GetHeight();
				
				context = CGPDFContextCreate (consumer, &bounds, NULL);// 4
				CGDataConsumerRelease (consumer);
				if (context != NULL) 
				{
					VRect pictrect(0,0,poly.GetWidth(),poly.GetHeight());
					CGContextBeginPage (context, &bounds);
					
					
					VColor col=set.GetBackgroundColor();
					::CGContextSetRGBFillColor(context,1,1,1,0);
					::CGContextFillRect(context, bounds);
										
					set.SetScaleMode(PM_MATRIX);
					set.SetYAxisSwap(poly.GetHeight(),true);
					set.SetDevicePrinter(true); // pp ACI0081630 pour forcer la deactivation du rendu offscreen en svg
					DrawInCGContext(context,pictrect,&set);
										
					CGContextEndPage (context);
					CGContextRelease (context);

					if(outDataProvider)
					{
						VPictureCodecFactoryRef fact;
						const UInt8 *dataptr=CFDataGetBytePtr(data);
						const VPtr p=(const VPtr)dataptr;
						sLONG datalen=CFDataGetLength(data);
						VHandle vh=VMemory::NewHandle(datalen);
						
						if(vh)
						{
							VPtr vp=VMemory::LockHandle(vh);
							if(vp)
							{
								memcpy(vp,p,datalen);
								VMemory::UnlockHandle(vh);
								*outDataProvider=VPictureDataProvider::Create(vh,true);
								if(*outDataProvider)
								{
									const VPictureCodec* decoder=fact->RetainDecoderForExtension(sCodec_pdf);
									(*outDataProvider)->SetDecoder(decoder);
									decoder->Release();
									CGDataProviderRef dataprov=xV4DPicture_MemoryDataProvider::CGDataProviderCreate(*outDataProvider);
									if(dataprov)
									{
										result=CGPDFDocumentCreateWithProvider(dataprov);
										CGDataProviderRelease(dataprov);
									}
								}
							}
							else
							{
								vThrowError(VMemory::GetLastError());
							}
						}
						else
						{
							vThrowError(VMemory::GetLastError());
						}
					}
					else
					{
						// pp attention sans datasource on ne peut sauvegarder un pdfdoref
						CGDataProviderRef dataprov=CGDataProviderCreateWithCFData(data);
						if(dataprov)
						{
							result=CGPDFDocumentCreateWithProvider(dataprov);
							CFRelease(dataprov);
						}
					}
				}
			}
			CFRelease (data); 
		}
	}
	return result;
}
	
#endif

#if WITH_VMemoryMgr
VHandle			VPictureData::CreatePicVHandle(VPictureDrawSettings* inSet)const
{
	bool b;
	VHandle result=0;
	void* pict=CreatePicHandle(inSet,b);
	if(pict && GetMacAllocator())
	{
		result = GetMacAllocator()->ToVHandle(pict);
		GetMacAllocator()->Free(pict);
	};
	return result;
}
#endif

VSize VPictureData::GetDataSize(_VPictureAccumulator* inRecorder) const
{
	VSize result=0;
	if(fDataProvider)
		result= fDataProvider->GetDataSize();
	else
	{
		VSize outSize;
		VBlobWithPtr b;

		if( const_cast<VPictureData*>(this)->Save(&b,0,outSize,inRecorder) ==VE_OK )
		{
			result = outSize;
		}
	}
	return result;
}

sLONG VPictureData::GetWidth()  const
{
	_Load();
	return fBounds.GetWidth();
}

sLONG VPictureData::GetHeight() const
{
	_Load();
	return fBounds.GetHeight();
}

sLONG VPictureData::GetX()  const
{
	_Load();
	return fBounds.GetX();
}

sLONG VPictureData::GetY() const
{
	_Load();
	return fBounds.GetY();
}

const VRect& VPictureData::GetBounds() const
{
	_Load();
	return fBounds;
}

#if !VERSION_LINUX
void VPictureData::_CalcDestRect(const VRect& inWanted,VRect &outDestRect,VRect & /*outSrcRect*/,VPictureDrawSettings& inSet)const
{
	sLONG scalemode=inSet.GetScaleMode();
	if(scalemode!=PM_TILE)
	{
		VRect pictrect=GetBounds();
		VAffineTransform finalmat=_MakeFinalMatrix(inWanted,inSet);
		
		outDestRect=pictrect;
		if(scalemode==PM_TOPLEFT || scalemode==PM_MATRIX)
			outDestRect=inSet.GetPictureMatrix() * outDestRect;
		else
			outDestRect=inSet.GetPictureMatrix().TransformVector(outDestRect); // pour les autre mode, on ignore la translation
		outDestRect=finalmat * outDestRect;
		outDestRect=inSet.GetPictureMatrix() * outDestRect;
		outDestRect.SetPosBy(inWanted.GetX(),inWanted.GetY());
	}
	else
	{
		outDestRect=inWanted;
	}
}
#endif


void VPictureData::DumpPictureInfo(VString& outDump,sLONG level) const
{
	VString kind=GetEncoderID();
	VString space;
	for(long i=level;i>0;i--)
		space+="\t";
	
	uLONG_PTR ptr=(uLONG_PTR)const_cast<VPictureData*>(this);
	
	outDump.AppendPrintf("%S%i : kind=%S\r\n",&space,ptr,&kind);

	outDump.AppendPrintf("%SPictData Refcount : %i\r\n",&space,GetRefCount());
	if(fDataProvider)
		outDump.AppendPrintf("%SData Source Refcount : %i\r\n",&space,fDataProvider->GetRefCount());
	else
		outDump.AppendPrintf("%SNo Data source\r\n",&space);
	outDump.AppendPrintf("%S\twidth : %i height : %i\r\n",&space,GetWidth(),GetHeight());
	outDump.AppendPrintf("%S\tsize : %i ko\r\n",&space,GetDataSize()/1024);
	
	//dump metadatas if any
	const VValueBag *metas = RetainMetadatas();
	if (metas && (!metas->IsEmpty()))
	{
		VString metasDump;
		metas->DumpXML(metasDump, VString("Metadatas"), true);
		metasDump.ExchangeAll("\n","\n"+space);
		outDump.AppendPrintf("\r\n%S%S\r\n",&space,&metasDump);
		metas->Release();
	}
}
VError VPictureData::Save(VBlob* inData,VIndex inOffset,VSize& outSize,_VPictureAccumulator* /*inRecorder*/)const
{
	VError result=VE_OK;
	if(fDataProvider && fDataProvider->GetDataSize())
	{
		VPtr p=fDataProvider->BeginDirectAccess();
		if(p)
		{
			outSize=fDataProvider->GetDataSize();
			result=inData->PutData(p,outSize,inOffset);
			fDataProvider->EndDirectAccess();
		}
		else
			fDataProvider->ThrowLastError();
		
	}
	return result;
}

/** save picture data source to the specified file 
@remarks
	called from VPictureCodec::Encode 
	this method can be derived in order to customize encoding with file extension for instance
*/
VError  VPictureData::SaveToFile(VFile& inFile) const
{
	VError result=VE_OK;
	if(fDataProvider && fDataProvider->GetDataSize())
	{
		VPtr p=fDataProvider->BeginDirectAccess();
		if(p)
		{
			result=inFile.Create();
			if(result==VE_OK)
			{
				VFileDesc* inFileDesc;
				result=inFile.Open(FA_READ_WRITE,&inFileDesc);
				if(result==VE_OK)
				{
					result=inFileDesc->PutData(p,fDataProvider->GetDataSize(),0);
					delete inFileDesc;
				}
			}
			fDataProvider->EndDirectAccess();
		}
		else
			fDataProvider->ThrowLastError();
		
	}
	else
	{
		VBlobWithPtr blob;
		VSize outsize;
		result=Save(&blob,0,outsize);
		if(result==VE_OK)
		{
			result=inFile.Create();
			if(result==VE_OK)
			{
				VFileDesc* inFileDesc;
				result=inFile.Open(FA_READ_WRITE,&inFileDesc);
				if(result==VE_OK)
				{
					result=inFileDesc->PutData(blob.GetDataPtr(),blob.GetSize(),0);
					delete inFileDesc;
				}
			}
		}
	}
	return result;
}


void VPictureData::_ReleaseDataProvider()
{
	if(fDataProvider)
	{
		fDataProvider->Release();
		fDataProvider=NULL;
	}
	fDataSourceDirty=false;
	fDataSourceMetadatasDirty = false;
}
void VPictureData::SetDataProvider(VPictureDataProvider* inRef,bool inSetDirty)
{
	if(fDataProvider==0 && inRef==0)
	{
		return;
	}
	_ReleaseDataProvider();
	
	fDataProvider=inRef;
	if(fDataProvider)
	{
		fDataProvider->Retain();
		_SetRetainedDecoder(fDataProvider->RetainDecoder());
		//fKind=fDataProvider->GetPictureKind();
	}
	else
	{
		// pp attention il faut pas toucher au kind si on clear la datasource
	}
	if(inSetDirty)
	{
		fDataSourceDirty=true;
		fDataSourceMetadatasDirty = true;
	}
}

VPictureDataProvider* VPictureData::GetDataProvider() const
{
	return fDataProvider;
}

bool VPictureData::SameDataProvider(VPictureDataProvider* inRef)
{
	return inRef==fDataProvider;
};

#if !VERSION_LINUX
VPictureData*	VPictureData::ConvertToGray(const VPictureDrawSettings* inSet)const
{
	VPictureData* result=0;
	VPictureDrawSettings set(inSet);
	#if VERSIONWIN
	if(set.IsIdentityMatrix() && GetGDIPlus_Bitmap())
	{
		VBitmapData bmdata(*GetGDIPlus_Bitmap());
		bmdata.ConvertToGray();
		result=bmdata.CreatePictureData();
	}
	else
	{
		Gdiplus::Bitmap* bm=CreateGDIPlus_Bitmap(&set);
		if(bm)
		{
			VBitmapData bmdata(*bm);
			bmdata.ConvertToGray();
			result=bmdata.CreatePictureData();
			delete bm;
		}
	}
	#elif VERSIONMAC
	VBitmapData *bm=CreateVBitmapData(inSet);
	if(bm)
	{
		bm->ConvertToGray();
		result=bm->CreatePictureData();
		delete bm;
	}
	#endif
	return result;
}
#endif

#if !VERSION_LINUX
VPictureData* VPictureData::BuildThumbnail(sLONG inWidth,sLONG inHeight,PictureMosaic inMode,const VPictureDrawSettings* inSet,bool inNoAlpha,const VColor& inColor)const
{
	VPictureDrawSettings set(inSet);
	VPictureData* result=NULL;
	_Load();
	VRect dstrect;
	CalcThumbRect(inWidth,inHeight,inMode,dstrect,set);
	set.SetScaleMode(PM_SCALE_TO_FIT);

	if(inMode==PM_SCALE_EVEN)
	{
		inWidth=dstrect.GetWidth();
		inHeight=dstrect.GetHeight();
	}
#if VERSIONWIN
	Gdiplus::Bitmap* bm_thumbnail;
	int pixelfmt = inNoAlpha ? PixelFormat32bppRGB : PixelFormat32bppARGB;
	bm_thumbnail=new Gdiplus::Bitmap(inWidth,inHeight,pixelfmt);
	
	Gdiplus::Graphics graph(bm_thumbnail);
	
	if(inNoAlpha)
	{
		Gdiplus::Color col(inColor.GetRed(),inColor.GetGreen(),inColor.GetBlue());
		graph.Clear(col);
	}
	else
		graph.Clear(Gdiplus::Color(0,0,0,0));

	Gdiplus::InterpolationMode oldmode=graph.GetInterpolationMode();
	graph.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
		
	DrawInGDIPlusGraphics(&graph,dstrect,&set);
	
	graph.SetInterpolationMode(oldmode);
		
	result=new VPictureData_GDIPlus(bm_thumbnail,true);
	
#elif VERSIONMAC

	CGContextRef    context = NULL;
	CGColorSpaceRef colorSpace;
	void *          bitmapData;
	int             bitmapByteCount;
	int             bitmapBytesPerRow;
		
	bitmapBytesPerRow   = ( 4*inWidth + 15 ) & ~15;

	bitmapByteCount     = (bitmapBytesPerRow * inHeight);
		
	colorSpace = CGColorSpaceCreateDeviceRGB();

	bitmapData = malloc( bitmapByteCount );
		
	if(bitmapData)
	{
		sLONG pixfmt=inNoAlpha ? kCGImageAlphaNoneSkipLast : kCGImageAlphaPremultipliedLast;
		memset(bitmapData,0,bitmapByteCount);
		context = CGBitmapContextCreate (bitmapData,
                                   inWidth,
                                   inHeight,
                                   8,      // bits per component
                                   bitmapBytesPerRow,
                                   colorSpace,
                                   pixfmt);
		 if (context)
		 {
			CGRect vr;
			vr.origin.x= 0;
			vr.origin.y= 0;
			vr.size.width=inWidth;
			vr.size.height=inHeight;
			
			if(inNoAlpha)
			{
			//	::CGContextSetFillColorWithColor(context,inColor);
				::CGContextSetRGBFillColor(context, inColor.GetRed() / (GReal)255, inColor.GetGreen() / (GReal)255, inColor.GetBlue() / (GReal)255, inColor.GetAlpha() / (GReal)255);
				 ::CGContextFillRect(context, vr); 
			}
			else
				 ::CGContextClearRect(context, vr); 
			
			set.SetScaleMode(PM_SCALE_TO_FIT);
			set.SetYAxisSwap(inHeight,true);
			DrawInCGContext(context,dstrect,&set);
			
			::CGContextFlush(context);
			CGImageRef im;
				
			CGDataProviderRef dataprov= xV4DPicture_MemoryDataProvider::CGDataProviderCreate((char*)bitmapData,bitmapByteCount,true);
			
			im=CGImageCreate(inWidth,inHeight,8,32,bitmapBytesPerRow,colorSpace,pixfmt,dataprov,0,0,kCGRenderingIntentDefault);
			CGDataProviderRelease(dataprov);
			if(im)
			{
				result=new VPictureData_CGImage(im);
				CGImageRelease(im);
			}
			CGContextRelease(context);
		 }
		free(bitmapData);
	}
	CGColorSpaceRelease( colorSpace );
#endif
	
	if(result)
	{
		const VPictureCodec* deco;
		if(IsKind(".jpg") || IsKind(".tiff"))
		{
			deco=RetainDecoder();
		}
		else
		{
			VPictureCodecFactoryRef fact;
			deco=fact->RetainDecoderByIdentifier(".png");
		}
		result->_SetAndRetainDecoder(deco);
		ReleaseRefCountable(&deco);
	}
	
	return result;
}
#endif

#if !VERSION_LINUX 
void VPictureData::CalcThumbRect(sLONG inWidth,sLONG inHeight,PictureMosaic inMode,VRect& outRect,const VPictureDrawSettings& inSet)const
{
	VRect r(0,0,GetWidth(),GetHeight());
	VPolygon pictpoly;
	sLONG thumb_width=inWidth,thumb_height=inHeight;
	sLONG thumb_x=0,thumb_y=0;
	
	pictpoly=inSet.GetPictureMatrix().TransformVector(r);
	
	if(pictpoly.GetHeight()>0 && pictpoly.GetWidth()>0)
	{
		switch(inMode)
		{	
			case PM_SCALE_EVEN:
			case PM_4D_SCALE_CENTER:
			case PM_4D_SCALE_EVEN:
			{
				if (pictpoly.GetHeight() <= thumb_height && pictpoly.GetWidth() <= thumb_width) 
				{
					thumb_height=pictpoly.GetHeight();
					thumb_width=pictpoly.GetWidth();
				} 
				else 
				{
					if (thumb_width * pictpoly.GetHeight() < thumb_height * pictpoly.GetWidth()) 
					{
						thumb_height =  ((long)pictpoly.GetHeight()) * thumb_width / pictpoly.GetWidth();
					} 
					else 
					{
						thumb_width =  ((long)pictpoly.GetWidth()) * thumb_height / pictpoly.GetHeight();
					}
				}
				if(inMode==PM_4D_SCALE_CENTER)
				{
					thumb_x=(inWidth/2)-(thumb_width/2);
					thumb_y=(inHeight/2)-(thumb_height/2);
				}
				outRect.SetCoords(thumb_x,thumb_y,thumb_width,thumb_height);
				break;
			}
			case PM_SCALE_TO_FIT:
			default:
			{
				outRect.SetCoords(0,0,inWidth,inHeight);
			}
		}
	}
	else
		outRect.SetCoords(0,0,0,0);	
}
#endif

/******************** vector ****************************/


VPictureData_Vector::VPictureData_Vector()
:VPictureData()
{
}
VPictureData_Vector::VPictureData_Vector(const VPictureData_Vector& inData)
:VPictureData(inData)
{
}
VPictureData_Vector::VPictureData_Vector(VPictureDataProvider* inDataProvider,_VPictureAccumulator* inRecorder)
:VPictureData(inDataProvider,inRecorder)
{
}

/******************** bitmap ****************************/
VPictureData_Bitmap::VPictureData_Bitmap()
:VPictureData()
{
}
VPictureData_Bitmap::VPictureData_Bitmap(const VPictureData_Bitmap& inData)
:VPictureData(inData)
{
}
VPictureData_Bitmap::VPictureData_Bitmap(VPictureDataProvider* inDataProvider,_VPictureAccumulator* inRecorder)
:VPictureData(inDataProvider,inRecorder)
{
}

/*******************************************************************************/
#if 0
#if VERSIONWIN || (VERSIONMAC && ARCH_32)

VPictureData_MacPicture::VPictureData_MacPicture()
:inherited()
{
	_SetDecoderByExtension(sCodec_pict);
	fPicHandle=NULL;
	fMetaFile=NULL;
	#if VERSIONWIN
	fGdiplusMetaFile=NULL;
	#endif
	fTrans=NULL;
}

VPictureData_MacPicture::VPictureData_MacPicture(VPictureDataProvider* inDataProvider,_VPictureAccumulator* inRecorder)
:inherited(inDataProvider,inRecorder)
{
	_SetDecoderByExtension(sCodec_pict);
	fPicHandle=NULL;
	fMetaFile=NULL;
	fTrans=NULL;
	#if VERSIONWIN
	fGdiplusMetaFile=NULL;
	#endif
}
VPictureData_MacPicture::VPictureData_MacPicture(const VPictureData_MacPicture& inData)
:inherited(inData)
{
	_SetDecoderByExtension(sCodec_pict);
	fPicHandle=NULL;
	fMetaFile=NULL;
	fTrans=NULL;
	#if VERSIONWIN
	fGdiplusMetaFile=NULL;
	#endif
	if(!fDataProvider && inData.fPicHandle && GetMacAllocator())
	{
		fPicHandle=GetMacAllocator()->Duplicate(inData.fPicHandle);
	}
}

VPictureData_MacPicture::~VPictureData_MacPicture()
{
	if(fPicHandle && GetMacAllocator())
	{
		if(GetMacAllocator()->CheckBlock(fPicHandle))
			GetMacAllocator()->Free(fPicHandle);
	}
	_DisposeMetaFile();
}

#if VERSIONWIN || WITH_QUICKDRAW
VPictureData_MacPicture::VPictureData_MacPicture(PortRef inPortRef,const VRect& inBounds)
{
	_SetDecoderByExtension(sCodec_pict);
	fPicHandle=NULL;
	fMetaFile=NULL;
	fTrans=NULL;
	#if VERSIONWIN
	inPortRef; // pp pour warning
	inBounds;
	fGdiplusMetaFile=NULL;
	#endif
	#if VERSIONMAC
	GrafPtr oldPort;
	GetPort(&oldPort);
	SetPort(inPortRef);
	RGBColor saveFore,saveBack,white={0xffff,0xffff,0xffff},black={0,0,0};
	Rect r;
	
	inBounds.MAC_ToQDRect(r);
	
	GetBackColor(&saveFore);
	GetForeColor(&saveFore);
	
	RGBBackColor(&white);
	RGBForeColor(&black);

	PicHandle	newPicture = ::OpenPicture(&r);
	const BitMap*	portBits = ::GetPortBitMapForCopyBits(inPortRef);
	
	::ClipRect(&r);
	::CopyBits(portBits, portBits, &r, &r, srcCopy, NULL);
	::ClosePicture();
	
	RGBBackColor(&saveFore);
	RGBForeColor(&saveFore);

	SetPort(oldPort);
	fPicHandle=newPicture;
	
	#if VERSIONWIN
	qtime::Rect rect;
	QDGetPictureBounds((qtime::Picture**)fPicHandle,&rect);
	fBounds.SetCoords(0,0,rect.right-rect.left,rect.bottom-rect.top);
	#else
	Rect rect;
	QDGetPictureBounds((Picture**)fPicHandle,&rect);
	fBounds.SetCoords(0,0,rect.right-rect.left,rect.bottom-rect.top);
	#endif
	
	#if VERSIONMAC
	if(fPicHandle)
	{
		GetMacAllocator()->Lock(fPicHandle);
		CGDataProviderRef dataprov= xV4DPicture_MemoryDataProvider::CGDataProviderCreate(*(char**)fPicHandle,GetMacAllocator()->GetSize(fPicHandle),true);
		GetMacAllocator()->Unlock(fPicHandle);
		fMetaFile=QDPictCreateWithProvider (dataprov);
		CGDataProviderRelease (dataprov);
	}
	#endif
	#endif
}
#endif	// #if VERSIONWIN || WITH_QUICKDRAW


#if VERSIONWIN
Gdiplus::Metafile*	VPictureData_MacPicture::GetGDIPlus_Metafile() const
{	
	_Load();
	return fGdiplusMetaFile;
}
HENHMETAFILE	VPictureData_MacPicture::GetMetafile() const
{	
	_Load();
	return fMetaFile;
}

#if ENABLE_D2D
void VPictureData_MacPicture::DrawInD2DGraphicContext(VWinD2DGraphicContext* inDC,const VRect& inBounds,VPictureDrawSettings* inSet)const
{
	VPictureDrawSettings drset(inSet);
	
	_Load();
	bool istrans=false;
	if(inSet && (inSet->GetDrawingMode()==1 || (inSet->GetDrawingMode()==2 && inSet->GetAlpha()!=100)))
		istrans=true;
	
	// pp on utilsie pas l'emf pour l'impression (bug hair line)
	// sauf !!! si l'appel viens du write ou view
	// Dans ce cas, l'impression ne passe plus du tout par altura, donc impossible d'utiliser drawpicture
	if(fMetaFile && (!drset.IsDevicePrinter() || drset.CanUseEMFForMacPicture()))
	{
		drset.SetInterpolationMode(0);
		if(istrans)
		{
			if(!fTrans)
			{
				_CreateTransparent(&drset);			
			}
			if(fTrans)
			{
				xDraw(fTrans,inDC,inBounds,&drset);
			}
			else
			{
				//here we need to get a HDC from D2D render target 
				//because otherwise D2D render target does not support windows metafiles
				//(if called between BeginUsingContext/EndUsingContext,
				// this will force a EndDraw() before getting hdc and a BeginDraw() to resume drawing after releasing hdc)				
				StSaveContext_NoRetain saveCtx(inDC);
				HDC hdc = inDC->BeginUsingParentContext();
				if (hdc)
					xDraw(fMetaFile,hdc,inBounds,&drset);
				inDC->EndUsingParentContext( hdc);
			}
		}
		else
		{
			//here we need to get a HDC from D2D render target 
			//because otherwise D2D render target does not support windows metafiles
			//(if called between BeginUsingContext/EndUsingContext,
			// this will force a EndDraw() before getting hdc and a BeginDraw() to resume drawing after releasing hdc)				
			StSaveContext_NoRetain saveCtx(inDC);
			HDC hdc = inDC->BeginUsingParentContext();
			if (hdc)
				xDraw(fMetaFile,hdc,inBounds,&drset);
			inDC->EndUsingParentContext( hdc);
		}
		return;
	}
	else if(GetPicHandle() && sQDBridge)
	{
		VRect dstRect,srcRect;
		_CalcDestRect(inBounds,dstRect,srcRect,drset);
	
		xMacRect r;
		r.left=dstRect.GetX();
		r.top=dstRect.GetY();
		r.right=dstRect.GetX()+dstRect.GetWidth();
		r.bottom=dstRect.GetY()+dstRect.GetHeight();
	
		if(drset.GetScaleMode()==PM_TOPLEFT)
		{
			sLONG ox,oy;
			drset.GetOrigin(ox,oy);
			r.left-=ox;
			r.top-=oy;
			r.right=r.left+GetWidth();
			r.bottom=r.top+GetHeight();
		}
		{
			//here we need to get a HDC from D2D render target 
			//(if called between BeginUsingContext/EndUsingContext,
			// this will force a EndDraw() before getting hdc and a BeginDraw() to resume drawing after releasing hdc)				
			StSaveContext_NoRetain saveCtx(inDC);
			HDC hdc = inDC->BeginUsingParentContext();
			if (hdc)
				sQDBridge->DrawInMacPort(hdc,*this,(xMacRect*)&r,drset.GetDrawingMode()==1,drset.GetScaleMode()==PM_TILE);
			inDC->EndUsingParentContext( hdc);
		}
	}
}
#endif

void VPictureData_MacPicture::DrawInGDIPlusGraphics(Gdiplus::Graphics* inDC,const VRect& inBounds,VPictureDrawSettings* inSet) const
{
	VPictureDrawSettings drset(inSet);
	
	_Load();
	bool istrans=false;
	if(inSet && (inSet->GetDrawingMode()==1 || (inSet->GetDrawingMode()==2 && inSet->GetAlpha()!=100)))
		istrans=true;
	
	// pp on utilsie pas l'emf pour l'impression (bug hair line)
	// sauf !!! si l'appel viens du write ou view
	// Dans ce cas, l'impression ne passe plus du tout par altura, donc impossible d'utiliser drawpicture
	if(fMetaFile && (!drset.IsDevicePrinter() || drset.CanUseEMFForMacPicture()))
	{
		drset.SetInterpolationMode(0);
		if(istrans)
		{
			if(!fTrans)
			{
				_CreateTransparent(&drset);			
			}
			if(fTrans)
			{
				xDraw(fTrans,inDC,inBounds,&drset);
			}
			else
				xDraw(fMetaFile,inDC,inBounds,&drset);
		}
		else
		{
			xDraw(fMetaFile,inDC,inBounds,&drset);
		}
		return;
	}
	else if(GetPicHandle() && sQDBridge)
	{
		VRect dstRect,srcRect;
		_CalcDestRect(inBounds,dstRect,srcRect,drset);
	
		xMacRect r;
		r.left=dstRect.GetX();
		r.top=dstRect.GetY();
		r.right=dstRect.GetX()+dstRect.GetWidth();
		r.bottom=dstRect.GetY()+dstRect.GetHeight();
		
	
		if(drset.GetScaleMode()==PM_TOPLEFT)
		{
			sLONG ox,oy;
			drset.GetOrigin(ox,oy);
			r.left-=ox;
			r.top-=oy;
			r.right=r.left+GetWidth();
			r.bottom=r.top+GetHeight();
		}
		{
			StUseHDC dc(inDC);
			sQDBridge->DrawInMacPort(dc,*this,(xMacRect*)&r,drset.GetDrawingMode()==1,drset.GetScaleMode()==PM_TILE);
		}
	}
}
void		VPictureData_MacPicture::_CreateTransparent(VPictureDrawSettings* /*inSet*/)const
{
	if(fTrans)
		return;
	if(fMetaFile)
	{
		fTrans = new Gdiplus::Bitmap(GetWidth(),GetHeight(),PixelFormat32bppRGB );
		if(fTrans)
		{
			Gdiplus::Metafile gdipmeta(fMetaFile,false);
			fTrans->SetResolution(gdipmeta.GetHorizontalResolution(),gdipmeta.GetVerticalResolution());
			{
				Gdiplus::Graphics graph(fTrans);
				graph.Clear(Gdiplus::Color(0xff,0xff,0xff));
				graph.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf); 
				graph.DrawImage(&gdipmeta,0,0,0,0,GetWidth(),GetHeight(),Gdiplus::UnitPixel);
			}
		}
	}
}
#elif VERSIONMAC
void		VPictureData_MacPicture::_CreateTransparent(VPictureDrawSettings* inSet)const
{
	CGImageRef result=0;
	
	if(fTrans)
		return;
	VPictureDrawSettings set(inSet);
	if(GetWidth() && GetHeight())
	{
		if(fMetaFile)
		{

			CGContextRef    context = NULL;
			CGColorSpaceRef colorSpace;
			void *          bitmapData;
			int             bitmapByteCount;
			int             bitmapBytesPerRow;
			sLONG width,height;
			
			CGRect cgr=QDPictGetBounds(fMetaFile);
			
			width=cgr.size.width;
			height=cgr.size.height;
			bitmapBytesPerRow   = ( 4 * width + 15 ) & ~15;
			bitmapByteCount     = (bitmapBytesPerRow * height);
			
			bitmapData = malloc( bitmapByteCount );
			if(bitmapData)
			{
				colorSpace = CGColorSpaceCreateDeviceRGB();//CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);

				memset(bitmapData,0,bitmapByteCount);
				context = CGBitmapContextCreate (bitmapData,
								width,
								height,
								8,      // bits per component
								bitmapBytesPerRow,
								colorSpace,
								kCGImageAlphaNoneSkipLast);
				if (context)
				{
					CGRect vr;
					vr.origin.x= 0;
					vr.origin.y=  0;
					vr.size.width=width;
					vr.size.height=height;
			
					VRect pictrect(0,0,width,height);
					set.SetScaleMode(PM_SCALE_TO_FIT);
					set.SetYAxisSwap(height,true);
					set.SetPictureMatrix(VAffineTransform());
					CGContextSaveGState(context);
					
					CGContextSetRGBFillColor(context,1,1,1,1);
					CGContextFillRect(context,vr);
					
					xDraw(fMetaFile,context,pictrect,&set);
					
					CGContextRestoreGState(context);
					CGDataProviderRef dataprov= xV4DPicture_MemoryDataProvider::CGDataProviderCreate((char*)bitmapData,bitmapByteCount,true);
			
					result=CGImageCreate(width,height,8,32,bitmapBytesPerRow,colorSpace,kCGImageAlphaNoneSkipLast,dataprov,0,0,kCGRenderingIntentDefault);
					CGContextRelease(context);
					CGDataProviderRelease(dataprov);
					if(set.GetDrawingMode()==1)
					{
						const VColor& col=set.GetTransparentColor();
						CGFloat MaskingColors[6];
						MaskingColors[0]=col.GetRed();
						MaskingColors[1]=col.GetGreen();
						MaskingColors[2]=col.GetBlue();
						MaskingColors[3]=col.GetRed();
						MaskingColors[4]=col.GetGreen();
						MaskingColors[5]=col.GetBlue();
							
						CGImageRef trans=CGImageCreateWithMaskingColors(result,MaskingColors);
						if(trans)
						{
							CFRelease(result);
							fTrans=trans;
						}
					}
				}
				CGColorSpaceRelease( colorSpace );
			}
			free(bitmapData);
		}
	}
}


void VPictureData_MacPicture::DrawInCGContext(CGContextRef inDC,const VRect& r,VPictureDrawSettings* inSet) const
{
	_Load();
	if(fMetaFile)
	{
		#if VERSIONWIN
		xDraw(fMetaFile,inDC,r,inSet);
		#else
		if(inSet && inSet->GetDrawingMode()==1)
		{
			if(!fTrans)
			{
				_CreateTransparent(inSet);
			}
			if(fTrans)
				xDraw(fTrans,inDC,r,inSet);
			else
				xDraw(fMetaFile,inDC,r,inSet);
				
		}
		else
			xDraw(fMetaFile,inDC,r,inSet);
		#endif
	}
	else if(GetPicHandle())
		xDraw(GetPicHandle(),inDC,r,inSet);	
}

#endif

void VPictureData_MacPicture::DrawInPortRef(PortRef inPortRef,const VRect& inBounds,VPictureDrawSettings* inSet) const
{
	VPictureDrawSettings drset(inSet);
	
	_Load();
	bool istrans=false;
	if(inSet && (inSet->GetDrawingMode()==1 || (inSet->GetDrawingMode()==2 && inSet->GetAlpha()!=100)))
		istrans=true;
	
	// pp on utilsie pas l'emf pour l'impression (bug hair line)
	// sauf !!! si l'appel viens du write ou view
	// Dans ce cas, l'impression ne passe plus du tout par altura, donc impossible d'utiliser drawpicture
	if(fMetaFile && (!drset.IsDevicePrinter() || drset.CanUseEMFForMacPicture()))
	{
		drset.SetInterpolationMode(0);
		if(istrans)
		{
			if(!fTrans)
			{
				_CreateTransparent(&drset);			
			}
			if(fTrans)
			{
				xDraw(fTrans,inPortRef,inBounds,&drset);
			}
			else
				xDraw(fMetaFile,inPortRef,inBounds,&drset);
		}
		else
		{
			xDraw(fMetaFile,inPortRef,inBounds,&drset);
		}
		return;
	}
	else if(GetPicHandle() && sQDBridge)
	{
		VRect dstRect,srcRect;
		_CalcDestRect(inBounds,dstRect,srcRect,drset);
	
		xMacRect r;
		r.left=dstRect.GetX();
		r.top=dstRect.GetY();
		r.right=dstRect.GetX()+dstRect.GetWidth();
		r.bottom=dstRect.GetY()+dstRect.GetHeight();
		
	
		if(drset.GetScaleMode()==PM_TOPLEFT)
		{
			sLONG ox,oy;
			drset.GetOrigin(ox,oy);
			r.left-=ox;
			r.top-=oy;
			r.right=r.left+GetWidth();
			r.bottom=r.top+GetHeight();
		}
		sQDBridge->DrawInMacPort(inPortRef,*this,(xMacRect*)&r,drset.GetDrawingMode()==1,drset.GetScaleMode()==PM_TILE);
	}
}

VPictureData* VPictureData_MacPicture::Clone()const
{
	VPictureData_MacPicture* clone;
	clone= new VPictureData_MacPicture(*this);
	return clone;
}

VSize VPictureData_MacPicture::GetDataSize(_VPictureAccumulator* /*inRecorder*/) const
{
	if(fDataProvider)
		return (sLONG) fDataProvider->GetDataSize();
	else if(fPicHandle && GetMacAllocator())
	{
		return (sLONG) GetMacAllocator()->GetSize(fPicHandle);
	}
	return 0;
}

VError VPictureData_MacPicture::Save(VBlob* inData,VIndex inOffset,VSize& outSize,_VPictureAccumulator* inRecorder)const
{
	VError result=VE_OK;
	VSize headersize=0;
	if(fDataProvider)
	{
		if(fDataProvider->IsFile())
		{
			if(inData->IsOutsidePath())
			{
				result= inherited::Save(inData,inOffset,outSize,inRecorder);
			}
			else
			{
				assert(fDataProvider->GetDataSize()>0x200);
				VPtr dataptr=fDataProvider->BeginDirectAccess();
				if(dataptr)
				{
					dataptr+=0x200;
					result=inData->PutData(dataptr,fDataProvider->GetDataSize()-0x200,inOffset);
					fDataProvider->EndDirectAccess();
				}
				else
					fDataProvider->GetLastError();
			}
		}
		else
		{
			if(inData->IsOutsidePath())
			{
				char buff[0x200];
				headersize=0x200;
				memset(buff,0,headersize);
				result=inData->PutData(buff,headersize,inOffset);
				inOffset+=headersize;
				
				inherited::Save(inData,inOffset,outSize,inRecorder);
				
				outSize+=headersize;
			}
			else
			{
				result= inherited::Save(inData,inOffset,outSize,inRecorder);
			}
		}	
	}
	else
	{
		if(GetMacAllocator())
		{
			if(fPicHandle)
			{
				GetMacAllocator()->Lock(fPicHandle);
				VPtr p=*(char**)fPicHandle;
				if(p)
				{
					
					outSize=GetMacAllocator()->GetSize(fPicHandle);
					
					if(inData->IsOutsidePath())
					{
						char buff[0x200];
						headersize=0x200;
						memset(buff,0,headersize);
						result=inData->PutData(buff,headersize,inOffset);
						inOffset+=headersize;
					}
					
	#if VERSIONWIN
					ByteSwapWordArray((sWORD*)p,5);
	#endif
					result=inData->PutData(p,outSize,inOffset);
	#if VERSIONWIN
					ByteSwapWordArray((sWORD*)p,5);
	#endif
					outSize+=headersize;
				}
				GetMacAllocator()->Unlock(fPicHandle);
			}
		}
		else
			result=VE_UNIMPLEMENTED;
	}
	return result;
}

void VPictureData_MacPicture::_DisposeMetaFile()const
{
	if(fMetaFile)
	{
		#if VERSIONWIN
		DeleteEnhMetaFile( fMetaFile);
		#else
		QDPictRelease(fMetaFile);
		#endif
		fMetaFile=NULL;
	}
	if(fTrans)
	{
#if VERSIONWIN
	#if ENABLE_D2D
		VWinD2DGraphicContext::ReleaseBitmap( fTrans);
	#endif
		delete fTrans;
#elif VERSIONMAC
		CFRelease(fTrans);
#endif
		fTrans=NULL;
	}
#if VERSIONWIN
	if(fGdiplusMetaFile)
		delete fGdiplusMetaFile;
	fGdiplusMetaFile=0;
#endif
}
void VPictureData_MacPicture::DoDataSourceChanged()
{
	if(fPicHandle && GetMacAllocator())
	{
		GetMacAllocator()->Free(fPicHandle);
		fPicHandle=0;
	}
	_DisposeMetaFile();
}

VPictureData_MacPicture::VPictureData_MacPicture(xMacPictureHandle inMacPicHandle)
:VPictureData_Vector()
{
	_SetDecoderByExtension(sCodec_pict);
	fDataSourceDirty=true;
	fDataSourceMetadatasDirty = true;
	fPicHandle=NULL;
	fMetaFile=NULL;
	fTrans=NULL;
#if VERSIONWIN
	fGdiplusMetaFile=0;
#endif
	
	if(inMacPicHandle)
	{
		
	#if VERSIONWIN // pp c'est une resource, sous windows elle est deja swap√©
		ByteSwapWordArray((sWORD*)(*inMacPicHandle),5);
	#endif

		VPictureDataProvider *prov=VPictureDataProvider::Create((xMacHandle)inMacPicHandle,false);
		
	#if VERSIONWIN // pp c'est une resource, sous windows elle est deja swap√©
		ByteSwapWordArray((sWORD*)(*inMacPicHandle),5);
	#endif
		
		if(prov)
		{
			prov->SetDecoder(fDecoder);
			SetDataProvider(prov,true);
			prov->Release();
		}
		
	}
}

void VPictureData_MacPicture::_DoReset()const
{
	_DisposeMetaFile();
	if(fPicHandle && GetMacAllocator())
	{
		GetMacAllocator()->Free(fPicHandle);
		fPicHandle=0;
	}
}

void* VPictureData_MacPicture::_BuildPicHandle()const
{
	void* outPicHandle=NULL;
	VIndex offset=0;
	if(GetMacAllocator() && fDataProvider && fDataProvider->GetDataSize())
	{
		if(fDataProvider->IsFile())
		{
			outPicHandle=GetMacAllocator()->Allocate(fDataProvider->GetDataSize()-0x200);
			offset=0x200;
		}
		else
			outPicHandle=GetMacAllocator()->Allocate(fDataProvider->GetDataSize());
	}
	if(outPicHandle)
	{
		VSize datasize=fDataProvider->GetDataSize();
		if(fDataProvider->IsFile())
		{
			datasize-=0x200;
		}
		GetMacAllocator()->Lock(outPicHandle);
		fDataProvider->GetData(*(char**)outPicHandle,offset,datasize);
#if VERSIONWIN
		ByteSwapWordArray(*(sWORD**)outPicHandle,5);
#endif
		GetMacAllocator()->Unlock(outPicHandle);
	}
	return outPicHandle;
}

void VPictureData_MacPicture::_DoLoad()const
{
	if(fMetaFile)
		return;
	VIndex offset=0;
	if(fDataProvider && fDataProvider->GetDataSize() && !fPicHandle)
	{
		fPicHandle = _BuildPicHandle();
	}
	if(fPicHandle)
	{
#if VERSIONWIN
		xMacRect rect;
		rect=   (*((xMacPicture**)fPicHandle))->picFrame;
		//QDGetPictureBounds((qtime::Picture**)fPicHandle,&rect);
		fBounds.SetCoords(0,0,rect.right-rect.left,rect.bottom-rect.top);
#elif VERSIONMAC
		Rect rect;
		QDGetPictureBounds((Picture**)fPicHandle,&rect);
		fBounds.SetCoords(0,0,rect.right-rect.left,rect.bottom-rect.top);
#endif
	}
	if(fPicHandle)
	{
#if VERSIONMAC
		GetMacAllocator()->Lock(fPicHandle);
		CGDataProviderRef dataprov= xV4DPicture_MemoryDataProvider::CGDataProviderCreate(*(char**)fPicHandle,GetMacAllocator()->GetSize(fPicHandle),true);
		GetMacAllocator()->Unlock(fPicHandle);
		fMetaFile=QDPictCreateWithProvider (dataprov);
		CGDataProviderRelease (dataprov);
#elif VERSIONWIN
		if(sQDBridge)
			fMetaFile=sQDBridge->ToMetaFile(fPicHandle);
#endif
	}
}

VError VPictureData_MacPicture::SaveToFile(VFile& inFile)const
{
	VError result=VE_OK;
	if(fDataProvider && fDataProvider->GetDataSize())
	{
		VPtr p=fDataProvider->BeginDirectAccess();
		if(p)
		{
			result=inFile.Create();
			if(result==VE_OK)
			{
				VFileDesc* inFileDesc;
				result=inFile.Open(FA_READ_WRITE,&inFileDesc);
				if(result==VE_OK)
				{
					char buff[0x200];
					memset(buff,0,0x200);
					result=inFileDesc->PutData(buff,0x200,0);
					if(result==VE_OK)
					{
						result=inFileDesc->PutData(p,fDataProvider->GetDataSize(),0x200);
					}
					delete inFileDesc;
				}
			}
			fDataProvider->EndDirectAccess();
		}
		else
			fDataProvider->ThrowLastError();
		
	}
	else
	{
		VBlobWithPtr blob;
		VSize outsize;
		result=Save(&blob,0,outsize);
		if(result==VE_OK)
		{
			result=inFile.Create();
			if(result==VE_OK)
			{
				VFileDesc* inFileDesc;
				result=inFile.Open(FA_READ_WRITE,&inFileDesc);
				if(result==VE_OK)
				{
					char buff[0x200];
					memset(buff,0,0x200);
					result=inFileDesc->PutData(buff,0x200,0);
					if(result==VE_OK)
					{
						result=inFileDesc->PutData(blob.GetDataPtr(),blob.GetSize(),0x200);
					}
					delete inFileDesc;
				}
			}
		}
	}
	return result;
}

#if WITH_VMemoryMgr
xMacPictureHandle	VPictureData_MacPicture::CreatePicHandle(VPictureDrawSettings* inSet,bool& outCanAddPicEnd) const
{
	void* result=NULL;
	VPictureDrawSettings set(inSet);
	outCanAddPicEnd=true;
	set.SetBackgroundColor(VColor(0xff,0xff,0xff));// pp no alpha
	if(set.IsIdentityMatrix())
	{
		result=_BuildPicHandle();
	}
	if(sQDBridge)
	{
		if( !result) // YT & PP - 24-Nov-2008 - ACI0059927 & ACI0059923
#if VERSIONWIN
		{
			Gdiplus::Bitmap* bm=CreateGDIPlus_Bitmap(&set);
			if(bm)
			{
				HBITMAP hbm;
				bm->GetHBITMAP(Gdiplus::Color(0xff,0xff,0xff),&hbm);
				if(hbm)
				{
					result=sQDBridge->HBitmapToPicHandle(hbm);
					DeleteObject(hbm);
				}
				delete bm;
			}
		}
#elif VERSIONMAC
		{
			result = sQDBridge->VPictureDataToPicHandle(*this, inSet);
		}
#endif
	}
	return (xMacPictureHandle)result;
}
#endif

#if WITH_VMemoryMgr
xMacPictureHandle VPictureData_MacPicture::GetPicHandle()const
{
	_Load();
	if(fPicHandle)
	{
		assert(GetMacAllocator()->CheckBlock(fPicHandle));
	}
	return (xMacPictureHandle)fPicHandle;
}
#endif
#endif // end version 64 bit
#endif
#if !VERSION_LINUX
bool VPictureData::MapPoint(XBOX::VPoint& ioPoint, const XBOX::VRect& inDisplayRect,VPictureDrawSettings* inSet) const
{
	bool res=false;
	VPictureDrawSettings drset(inSet);
	VAffineTransform finalmat=_MakeFinalMatrix(inDisplayRect,drset,true);
	switch(drset.GetScaleMode())
	{
		case PM_TILE:
		{
			ioPoint.SetPosBy(-inDisplayRect.GetX(),-inDisplayRect.GetY());
			res=true;
			break;
		}
		case PM_TOPLEFT:
		case PM_CENTER:
		case PM_4D_SCALE_CENTER:
		case PM_SCALE_TO_FIT:
		case PM_4D_SCALE_EVEN:
		default:
		{
			VRect pictrect(0,0,GetWidth(),GetHeight());
			_ApplyDrawingMatrixTranslate(finalmat,drset);
			finalmat=finalmat.Inverse ();	
			ioPoint = finalmat * ioPoint;
			
			res=(pictrect.HitTest(ioPoint)==TRUE);
			ioPoint = drset.GetPictureMatrix().TransformVector(ioPoint);
			break;
		}
	}
	return res;
}
#endif


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
void VPictureData::TransformRectFromImageSpaceToDisplayBounds( XBOX::VRect& ioBounds, const XBOX::VRect& inBoundsDisplay, const VPictureDrawSettings *inSet) const
{
	//force non euclidian y axis (y pointing down)
	VPictureDrawSettings drset(inSet);
	drset.SetYAxisSwap(0,false);
	
	VAffineTransform finalmat=_MakeFinalMatrix(inBoundsDisplay,drset,true);
	finalmat.Multiply(drset.GetDrawingMatrix(),VAffineTransform::MatrixOrderAppend);

	ioBounds = finalmat.TransformRect(ioBounds);
}
#endif

#if !VERSION_LINUX
void VPictureData::DrawInVGraphicContext(class VGraphicContext& inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const
{
	inDC.DrawPictureData(this,r,inSet);
}
#endif

/**************************************************************************************************/
// bitmap drawing
/**************************************************************************************************/
#if VERSIONWIN

void VPictureData::xDraw(Gdiplus::Image* inPict,Gdiplus::Graphics* inDC,const VRect& inBounds,VPictureDrawSettings* inSet)const
{
	if(inPict)
	{
		Gdiplus::Unit unit=Gdiplus::UnitPixel;
		Gdiplus::PixelOffsetMode oldpixoffmode;
		Gdiplus::InterpolationMode oldInterpolationMode;
		_Gdiplus_TransformSaver saver(inDC);
		bool needrestorepagescale=false;
		VPictureDrawSettings drset(inSet);
		
		Gdiplus::RectF loc(0.0,0.0,inPict->GetWidth(),inPict->GetHeight());
		
		Gdiplus::GraphicsState  state=0;
		Gdiplus::REAL dpiX = inDC->GetDpiX();
		Gdiplus::ImageAttributes attr;
		HDC refdc=GetDC(NULL);
		int logx=GetDeviceCaps(refdc,LOGPIXELSX);
		
		Gdiplus::Unit oldUnit=inDC->GetPageUnit();
		inDC->SetPageUnit(Gdiplus::UnitPixel );
		
		if(dpiX!=logx)
		{
			if ( drset.NeedToForce72dpi() )
				logx = 72.0;

			state=inDC->Save();
			inDC->SetPageUnit(Gdiplus::UnitPixel );
			inDC->SetPageScale(dpiX/logx);
			needrestorepagescale=true;
		}
		
		Gdiplus::Matrix ctm;
		inDC->GetTransform( &ctm);

		ReleaseDC(NULL,refdc);
		switch(drset.GetDrawingMode()) 
		{
			case 0:
			{
				break;
			}
			case 1:
			{	
				Gdiplus::Color col;
				UINT flags= inPict->GetFlags();
				if(!(flags & Gdiplus::ImageFlagsHasAlpha))
				{
					col.SetFromCOLORREF(drset.GetTransparentColor().WIN_ToCOLORREF());
					attr.SetColorKey(col,col,Gdiplus::ColorAdjustTypeDefault);
				}
				break;
			}
			case 2:
			{
				Gdiplus::ColorMatrix colorMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
													0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
													0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
													0.0f, 0.0f, 0.0f, drset.GetAlpha()/(GReal)100, 0.0f,
													0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
				attr.SetColorMatrix(&colorMatrix, Gdiplus::ColorMatrixFlagsDefault,Gdiplus::ColorAdjustTypeDefault);
				break;
			}
			//3 = efface le fond avec la couleur de background vant de dessiner l'image
			// utiliser pour convertir des image avec alph ou transparence, vers un format sans transparentce
		}
		inPict->SelectActiveFrame(&Gdiplus::FrameDimensionTime,drset.GetFrameNumber());
		
		Gdiplus::Matrix picturemat;
		Gdiplus::Matrix drawingmat;
		
		VAffineTransform finalmat=_MakeFinalMatrix(inBounds,drset,true);
		
		oldInterpolationMode=inDC->GetInterpolationMode();
		
		//JQ 22/09/2010: fixed interpolation mode
		// PP 23/09/2010 Pas touche au mode d'interpolation !!!!!
		if(drset.GetInterpolationMode()==0 || finalmat.GetScaleX()>1.0 || finalmat.GetScaleY()>1.0)
			inDC->SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
		
		oldpixoffmode = inDC->GetPixelOffsetMode(); 
		inDC->SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf); 

		switch(drset.GetScaleMode())
		{
			case PM_TILE:
			{
				if(inBounds.GetWidth()>0 && inBounds.GetHeight()>0 && GetWidth()>0 && GetHeight()>0)
				{
					sLONG x,y;
					VRect dr,rr;
					sLONG		nbx,nby,curx,cury,w,h,wtrans;
					VPolygon poly(GetBounds());
					poly = drset.GetPictureMatrix().TransformVector(poly);
					
					w = poly.GetWidth();
					h = poly.GetHeight();
					if(w!=0 && h!=0)
					{
						nbx = (inBounds.GetWidth()/w) + (1 * ( (long)inBounds.GetWidth() % w !=0));
						nby = (inBounds.GetHeight()/h) + (1 * ((long)inBounds.GetHeight()% h !=0));
						
						x=inBounds.GetX();
						y=inBounds.GetY();
						
						finalmat.ToNativeMatrix(picturemat);
						
						Gdiplus::SolidBrush* brush=0;
						
						if(drset.GetDrawingMode()==3)
						{
							Gdiplus::Color col;
							col.SetFromCOLORREF(drset.GetBackgroundColor().WIN_ToCOLORREF());
							brush=new Gdiplus::SolidBrush(col);
						}
						
						drset.GetXAxisSwap() !=0 ? wtrans= -w : wtrans= w;
						
						for(curx = 0;curx < nbx; curx++, x+=w )
						{
							for(cury = 0;cury < nby; cury++, y+=h )
							{
								//JQ 22/09/2010: fixed transform: we must multiply with ctm 
								inDC->SetTransform( &ctm);
								inDC->MultiplyTransform( &picturemat);
								if(brush)
								{
									inDC->FillRectangle(brush,loc);
								}
								inDC->DrawImage(inPict,loc,GetX(),GetY(),GetWidth(),GetHeight(),Gdiplus::UnitPixel,&attr); 
								picturemat.Translate(0,h,Gdiplus::MatrixOrderAppend);
							}
							
							picturemat.Translate(wtrans,-(h*nby),Gdiplus::MatrixOrderAppend);
							
						}
						if(brush)
							delete brush;
					}
				}
				break;
			}
			case PM_TOPLEFT:
			case PM_CENTER:
			case PM_4D_SCALE_CENTER:
			case PM_SCALE_TO_FIT:
			case PM_4D_SCALE_EVEN:
			default:
			{
				_ApplyDrawingMatrixTranslate(finalmat,drset);

				//JQ 24/07/2014: fixed ACI0087994 
				//	it is a workaround of image bad scaling by Gdiplus:
				//	here we pre-transform dest rect & use D2D to scale the bitmap (the only way i found to fix the scaling artifact)
				//	(if there is only scaling > 1 and/or translation and nearest neighbor interpolation and not printing)
				VAffineTransform vctm;
				vctm.FromNativeTransform( ctm);
				vctm.Multiply( finalmat, VAffineTransform::MatrixOrderPrepend);

#if ENABLE_D2D
				if (!needrestorepagescale
					&&
					vctm.GetShearX() == 0
					&&
					vctm.GetShearY() == 0
					&&
					(vctm.GetScaleX() > 1 || vctm.GetScaleY() > 1)
					&&
					GetWidth() && GetHeight()
					)
				{
					VRect bounds( GetX(),GetY(),GetWidth(),GetHeight());
					bounds = vctm.TransformRect(bounds);
					loc.X = bounds.GetX();
					loc.Y = bounds.GetY();
					loc.Width = bounds.GetWidth();
					loc.Height = bounds.GetHeight();
					Gdiplus::Matrix identity;
					identity.Reset();
					inDC->SetTransform( &identity);

					Gdiplus::InterpolationMode interpolation = inDC->GetInterpolationMode();

					Gdiplus::Bitmap *bmpSrc = dynamic_cast<Gdiplus::Bitmap *>(inPict);
					if (bmpSrc && loc.Width && loc.Height && loc.Width <= 4096 && loc.Height <= 4096 && floor(loc.Width) == loc.Width && floor(loc.Height) == loc.Height)
					{
						//scale using D2D

						VWinD2DGraphicContext *gcWIC = new VWinD2DGraphicContext((sLONG)loc.Width, (sLONG)loc.Height, 1, VPoint(0,0), true, false);
						xbox_assert(gcWIC);
						gcWIC->BeginUsingContext();
						ID2D1RenderTarget *rt = (ID2D1RenderTarget *)gcWIC->GetNativeRef();
						ID2D1Bitmap *bmpD2D = VPictureCodec_WIC::DecodeD2D( rt, bmpSrc);
						if (bmpD2D)
						{
							D2D1_RECT_F rectdst, rectsrc;
							rectdst.left = 0;
							rectdst.top = 0;
							rectdst.right = loc.Width;
							rectdst.bottom = loc.Height;
						
							rectsrc.left = GetX();
							rectsrc.top = GetY();
							rectsrc.right = GetWidth();
							rectsrc.bottom = GetHeight();
							rt->DrawBitmap( bmpD2D, rectdst, 1, interpolation == Gdiplus::InterpolationModeNearestNeighbor ? D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR : D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, rectsrc);

							bmpD2D->Release();
						}
						
						gcWIC->EndUsingContext();

						IWICBitmap *bmpWIC = gcWIC->RetainWICBitmap();
						xbox_assert(bmpWIC);
						Gdiplus::PixelFormat pf = PixelFormat32bppARGB;
						Gdiplus::Bitmap *bmpGDI = VPictureCodec_WIC::FromWIC( static_cast<IWICBitmapSource *>(bmpWIC), &pf);
						bmpWIC->Release();

						gcWIC->Release();

						if (bmpGDI)
						{
							if(drset.GetDrawingMode()==3)
							{
								Gdiplus::Color col;
								col.SetFromCOLORREF(drset.GetBackgroundColor().WIN_ToCOLORREF());
								Gdiplus::SolidBrush brush(col);
								inDC->FillRectangle(&brush,loc);
							}
							inDC->DrawImage(static_cast<Gdiplus::Image *>(bmpGDI),loc,0,0,loc.Width,loc.Height,unit ,&attr); 
							delete bmpGDI;

							break;
						}
					}
				}
				else
#endif
				{
					//JQ 22/09/2010: fixed transform: we must multiply with ctm 
					vctm.ToNativeMatrix(picturemat);
					inDC->SetTransform(&picturemat);
				}

				if(drset.GetDrawingMode()==3)
				{
					Gdiplus::Color col;
					col.SetFromCOLORREF(drset.GetBackgroundColor().WIN_ToCOLORREF());
					Gdiplus::SolidBrush brush(col);
					inDC->FillRectangle(&brush,loc);
				}
				inDC->DrawImage(inPict,loc,GetX(),GetY(),GetWidth(),GetHeight(),unit ,&attr); 
				break;
			}
		}

		inDC->SetPixelOffsetMode(oldpixoffmode);
		inDC->SetInterpolationMode(oldInterpolationMode); 
		inDC->SetPageUnit(oldUnit );

		if(needrestorepagescale)
		{
			inDC->Restore(state);
		}
	}
}

#if ENABLE_D2D
/** draw image in the provided D2D graphic context
@remarks
	we must pass a VWinD2DGraphicContext ref in order to properly inherit D2D render target resources
	(D2D render target resources are attached to the VWinD2DGraphicContext object)

	this method will fail if called outside BeginUsingContext/EndUsingContext (but it should never occur)

	we cannot use a Gdiplus::Image * here because in order to initialize a D2D bitmap 
	we need to be able to lock inPict pixel buffer which is not possible if inPict is not a Gdiplus::Bitmap
*/
void VPictureData::xDraw(Gdiplus::Bitmap* inBmp,VWinD2DGraphicContext* inDC,const VRect& inBounds,VPictureDrawSettings* inSet, bool inUseD2DCachedBitmap)const
{
	if(inBmp)
	{
		//get render target
		ID2D1RenderTarget* rt = (ID2D1RenderTarget*)inDC->GetNativeRef();

		//save transform
		VAffineTransform ctm;
		inDC->GetTransform( ctm);

		VPictureDrawSettings drset(inSet);
			
		//create D2D device-dependent bitmap from Gdiplus bitmap 
		inBmp->SelectActiveFrame(&Gdiplus::FrameDimensionTime,drset.GetFrameNumber());

		//determine opacity 
		Real opacity = 1.0f;
		bool isTransparentNoAlpha = false;
		switch(drset.GetDrawingMode()) 
		{
			case 0:
			{
				break;
			}
			case 1:
			{	
				Gdiplus::Color col;
				UINT flags = inBmp->GetFlags();
				if ((!(flags & Gdiplus::ImageFlagsHasAlpha)) && inBmp->GetWidth() && inBmp->GetHeight())
					isTransparentNoAlpha = true;
				break;
			}
			case 2:
			{
				opacity = drset.GetAlpha()/100.0f;
				break;
			}
		}

		VAffineTransform xform = _MakeFinalMatrix(inBounds, drset, true);

		//determine interpolation mode:
		//it is set to linear filtering if there is a rotation or a scaling or 
		//if translation is not aligned on pixel boundaries
		D2D1_BITMAP_INTERPOLATION_MODE interpolationMode = D2D1_BITMAP_INTERPOLATION_MODE_LINEAR;
		bool bDetermineInterpolationMode = false;
		if (drset.GetInterpolationMode() == 0 || xform.GetScaleX()>1.0 || xform.GetScaleY()>1.0)
			interpolationMode = D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;
		else 
			bDetermineInterpolationMode = true;

		

		//determine if we need to pre-scale bitmap using Gdiplus 
		//(Direct2D linear interpolation is too jaggy if bitmap is scaled by a factor < 0.5f
		// so we use Gdiplus to pre-scale the bitmap)
		if (drset.GetScaleMode() != PM_TILE)
		{
			_ApplyDrawingMatrixTranslate(xform,drset);
			xform *= ctm;

			if (bDetermineInterpolationMode
				&&
				xform.GetScaleX() == 1.0f && xform.GetScaleY() == 1.0f
			    &&
				xform.GetShearX() == 0.0f && xform.GetShearY() == 0.0f
				&&
				floor(xform.GetTranslateX()) == xform.GetTranslateX()
				&&
				floor(xform.GetTranslateY()) == xform.GetTranslateY())
				interpolationMode = D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;

			if (interpolationMode == D2D1_BITMAP_INTERPOLATION_MODE_LINEAR 
				&& 
				xform.GetShearX() == 0.0f && xform.GetShearY() == 0.0f
				&&
				(xform.GetScaleX() < 0.5f || xform.GetScaleY() < 0.5f))
			{
				//determine scaled rectangle
				VRect vSrcRect(0.0f, 0.0f, inBmp->GetWidth(), inBmp->GetHeight());
				VAffineTransform xformScale = xform;
				xformScale.SetTranslation(0.0f, 0.0f);
				VRect vDestRect = xformScale.TransformRect( vSrcRect);
				
				vDestRect.NormalizeToInt();

				GReal dx = vDestRect.GetX(), dy = vDestRect.GetY();
				vDestRect.SetPosTo(0,0);
				if (dx != 0 || dy != 0)
					//add translation if reversed axis (inverse of transformed rect origin here as we draw first in local bitmap space)
					xformScale.SetTranslation( -dx, -dy);

				drset.SetPictureMatrix( VAffineTransform());
				drset.SetDrawingMatrix( xformScale);
				drset.SetScaleMode( PM_TOPLEFT);

				//create Gdiplus scaled bitmap
				Gdiplus::Bitmap *bmpScaled = new Gdiplus::Bitmap(vDestRect.GetWidth(), vDestRect.GetHeight(), PixelFormat32bppPARGB);
				{
				Gdiplus::Graphics gc( static_cast<Gdiplus::Image *>(bmpScaled));
				gc.Clear( Gdiplus::Color(0, 0, 0, 0));
				xDraw( inBmp, &gc, vDestRect, &drset);
				}
				ID2D1Bitmap *bmpD2D = VPictureCodec_WIC::DecodeD2D( rt, bmpScaled);
				if (!testAssert(bmpD2D))
				{
					delete bmpScaled;
					return;
				}

				//set current transform to translation only
				xform.SetScaling( 1.0f, 1.0f);
				if (dx != 0 || dy != 0)
					//append translation if reversed axis
					xform.SetTranslation(xform.GetTranslateX()+dx, xform.GetTranslateY()+dy);
				inDC->SetTransform(xform);
					
				//draw scaled image
				rt->DrawBitmap( bmpD2D, NULL, 1.0f, interpolationMode);

				//release D2D bitmap
				bmpD2D->Release();

				//release scaled bitmap
				delete bmpScaled;

				//restore native transform
				inDC->SetTransform( ctm);
				return;
			}
		}

		ID2D1Bitmap *bmpD2D = NULL;
		if (inUseD2DCachedBitmap && drset.GetFrameNumber() == 0)
			bmpD2D = inDC->CreateOrRetainBitmap( inBmp, isTransparentNoAlpha ? &drset.GetTransparentColor() : NULL);
		else
			bmpD2D = VPictureCodec_WIC::DecodeD2D( rt, inBmp);
		if (!testAssert(bmpD2D))
			return;
		
		D2D1_SIZE_F size = bmpD2D->GetSize();
		D2D1_RECT_F rectSource = D2D1::RectF( 0.0f, 0.0f, size.width, size.height);

		switch(drset.GetScaleMode())
		{
			case PM_TILE:
			{
				if (inBounds.GetWidth()>0 && inBounds.GetHeight()>0 && GetWidth()>0 && GetHeight()>0)
				{
					sLONG x,y;
					VRect dr,rr;
					sLONG		nbx,nby,curx,cury,w,h,wtrans;
					VPolygon poly(GetBounds());
					poly = drset.GetPictureMatrix().TransformVector(poly);
					
					w = poly.GetWidth();
					h = poly.GetHeight();
					if(w!=0 && h!=0)
					{
						nbx = (inBounds.GetWidth()/w) + (1 * ( (long)inBounds.GetWidth() % w !=0));
						nby = (inBounds.GetHeight()/h) + (1 * ((long)inBounds.GetHeight()% h !=0));
						
						x=inBounds.GetX();
						y=inBounds.GetY();
					
						CComPtr<ID2D1SolidColorBrush> brushBackground;
						if(drset.GetDrawingMode()==3)
							rt->CreateSolidColorBrush(D2D_COLOR( drset.GetBackgroundColor()), &brushBackground);

						drset.GetXAxisSwap() !=0 ? wtrans= -w : wtrans= w;
						
						for(curx = 0;curx < nbx; curx++, x+=w )
						{
							for(cury = 0;cury < nby; cury++, y+=h )
							{
								VAffineTransform fullxform = xform*ctm;
								inDC->SetTransform(fullxform);
								if (bDetermineInterpolationMode)
								{
									if (fullxform.GetScaleX() == 1.0f && fullxform.GetScaleY() == 1.0f
										&&
										fullxform.GetShearX() == 0.0f && fullxform.GetShearY() == 0.0f
										&&
										floor(fullxform.GetTranslateX()) == fullxform.GetTranslateX()
										&&
										floor(fullxform.GetTranslateY()) == fullxform.GetTranslateY())
										interpolationMode = D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;
									else
										interpolationMode = D2D1_BITMAP_INTERPOLATION_MODE_LINEAR;
								}
								if(brushBackground != NULL)
									rt->FillRectangle(rectSource, brushBackground);
								rt->DrawBitmap( bmpD2D, rectSource, opacity, interpolationMode);
								xform.Translate(0,h,VAffineTransform::MatrixOrderAppend);
							}
							xform.Translate(wtrans,-(h*nby), VAffineTransform::MatrixOrderAppend);
						}
					}
				}
				break;
			}
			case PM_TOPLEFT:
			case PM_CENTER:
			case PM_4D_SCALE_CENTER:
			case PM_SCALE_TO_FIT:
			case PM_4D_SCALE_EVEN:
			default:
			{
				inDC->SetTransform(xform);
				if(drset.GetDrawingMode()==3)
				{
					CComPtr<ID2D1SolidColorBrush> brushBackground;
					rt->CreateSolidColorBrush(D2D_COLOR( drset.GetBackgroundColor()), &brushBackground);
					if(brushBackground != NULL)
						rt->FillRectangle(rectSource, brushBackground);
				}

				rt->DrawBitmap( bmpD2D, NULL, opacity, interpolationMode);
				break;
			}
		}

		//release D2D bitmap
		bmpD2D->Release();

		//restore native transform
		inDC->SetTransform( ctm);
	}
}
#endif

void VPictureData::xDraw(HENHMETAFILE inPict,Gdiplus::Graphics* inDC,const VRect& r,VPictureDrawSettings* inSet)const
{
	if(inPict)
	{
		StUseHDC dc(inDC);
		//HDC dc=inDC->GetHDC();
		xDraw(inPict,dc,r,inSet);
		//inDC->ReleaseHDC(dc);
	}
}
void VPictureData::xDraw(HBITMAP inPict,Gdiplus::Graphics* inDC,const VRect& r,VPictureDrawSettings* inSet)const
{
	Gdiplus::Bitmap bm(inPict,0);
	xDraw(&bm,inDC,r,inSet);
}
void VPictureData::xDraw(BITMAPINFO* /*inPict*/,Gdiplus::Graphics* /*inDC*/,const VRect& /*r*/,VPictureDrawSettings* /*inSet*/)const
{
	assert(false);
}	
void VPictureData::xDraw(HBITMAP inPict,PortRef inPortRef,const VRect& inBounds,VPictureDrawSettings* inSet)const
{
	if(inPict)
	{
		sLONG ox,oy;
		VPictureDrawSettings drset(inSet);
		_HDC_RightToLeft_SaverSetter rtlsetter(inPortRef,drset);
		VRect dstRect,srcRect;
		
		
		HDC srcdc=CreateCompatibleDC(inPortRef);
		HBITMAP oldbm=(HBITMAP)SelectObject(srcdc,inPict);
		
		HRGN newclip=CreateRectRgn(inBounds.GetX(),inBounds.GetY(),inBounds.GetX()+inBounds.GetWidth(),inBounds.GetY()+inBounds.GetHeight());
		HRGN oldclip=CreateRectRgn(0,0,0,0);
		
		if(GetClipRgn(inPortRef,oldclip)==0)
		{
			DeleteObject(oldclip);
			oldclip=0;
		}
		
		::ExtSelectClipRgn(inPortRef,newclip,RGN_AND);
		
		VAffineTransform finalmat=_MakeFinalMatrix(inBounds,drset,true);
		
		if(drset.GetScaleMode()!=PM_TILE)
		{
			_ApplyDrawingMatrixTranslate(finalmat,drset);
			
			switch(drset.GetDrawingMode())
			{
				case 0:
				{
					_HDC_TransformSetter setter(inPortRef,finalmat);
					BitBlt(inPortRef,0,0,(int)GetWidth(),(int)GetHeight(),srcdc,0,0,SRCCOPY);
					break;
				}
				case 1:
				{
					_HDC_TransformSetter setter(inPortRef,finalmat);
					TransparentBlt(inPortRef,0,0,(int)GetWidth(),GetHeight(),
					srcdc,0,0,(int)GetWidth(),(int)GetHeight(),drset.GetTransparentColor().WIN_ToCOLORREF());
					break;
				}
				case 2:
				{
					BLENDFUNCTION blend;
					blend.BlendOp =AC_SRC_OVER;
					blend.BlendFlags =0;
					blend.SourceConstantAlpha =drset.GetAlpha();
					blend.AlphaFormat =0;
					_HDC_TransformSetter setter(inPortRef,finalmat);
					AlphaBlend(inPortRef,0,0,(int)GetWidth(),(int)GetHeight(),
					srcdc,0,0,(int)GetWidth(),(int)GetHeight(),blend);
					break;
				}
			}
		}
		else
		{
			if(inBounds.GetWidth()>0 && inBounds.GetHeight()>0 && GetWidth()>0 && GetHeight()>0)
			{
				RECT dr;
				sLONG		nbx,nby,curx,cury,w,h,wtrans;
				VPolygon bounds(GetBounds());
				bounds=finalmat.TransformVector(bounds);	
				
				w = bounds.GetWidth();
				h = bounds.GetHeight();
				
				nbx = (inBounds.GetWidth()/w) + (1 * ( (long)inBounds.GetWidth() % w !=0));
				nby = (inBounds.GetHeight()/h) + (1 * ((long)inBounds.GetHeight()% h !=0));
				
				_HDC_TransformSetter setter(inPortRef);
				
				drset.GetXAxisSwap() !=0 ? wtrans= -w : wtrans= w;
				
				for(curx = 0;curx < nbx; curx++)
				{
					for(cury = 0;cury < nby; cury++)
					{
						setter.SetTransform(finalmat);
						
						switch(drset.GetDrawingMode())
						{
							case 0:							
							{
								BitBlt(inPortRef,0,0,(int)w,(int)h,srcdc,0,0,SRCCOPY);
								break;
							}
							case 1:
							{
								TransparentBlt(inPortRef,0,0,(int)w,h,
								srcdc,0,0,(int)w,(int)h,drset.GetTransparentColor().WIN_ToCOLORREF());
								break;
							}
							case 2:
							{
								BLENDFUNCTION blend;
								blend.BlendOp =AC_SRC_OVER;
								blend.BlendFlags =0;
								blend.SourceConstantAlpha =drset.GetAlpha();
								blend.AlphaFormat =0;
								AlphaBlend(inPortRef,0,0,(int)w,(int)h,srcdc,0,0,(int)w,(int)h,blend);
								break;
							}
						}
						finalmat.Translate(0,h*1.0,VAffineTransform::MatrixOrderAppend);
					}
					finalmat.Translate(wtrans,-(h*nby),VAffineTransform::MatrixOrderAppend);
				}
			}
		}
		SelectClipRgn(inPortRef,oldclip);
		if(oldclip)
			DeleteObject(oldclip);
		DeleteObject(newclip);
		SelectObject(srcdc,oldbm);
		DeleteDC(srcdc);
	}
}

void VPictureData::xDraw(Gdiplus::Image* inPict,PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet)const
{
	if(inPict)
	{
		VPictureDrawSettings set(inSet);
		_HDC_RightToLeft_SaverSetter rtlsetter(inPortRef,set);
		
		{
			Gdiplus::Graphics graph(inPortRef);
			xDraw(inPict,&graph,r,&set);
		}
	}
}

void VPictureData::xDraw(HENHMETAFILE inPict,PortRef inPortRef,const VRect& inBounds,VPictureDrawSettings* inSet)const
{
	if(inPict)
	{
		VPictureDrawSettings drset(inSet);
		_HDC_RightToLeft_SaverSetter rtlsaver(inPortRef,drset);
		
		VAffineTransform devicemat;
		HDC screendc=GetDC(NULL);
		RECT rr={0,0,GetWidth(),GetHeight()};
		RECT dst={inBounds.GetX(),inBounds.GetY(),inBounds.GetX()+inBounds.GetWidth(),inBounds.GetY()+inBounds.GetHeight()};
		BOOL test;
		VRect dstRect,srcRect;
		
		
		HRGN oldclip=CreateRectRgn(0,0,0,0);
		if(::GetClipRgn(inPortRef,oldclip)==0)
		{
			DeleteObject(oldclip);
			oldclip=0;
		}
		
		int		destresx, destresy,screenresx, screenresy;
	
		destresy  = GetDeviceCaps(inPortRef, LOGPIXELSY);
		destresx  = GetDeviceCaps(inPortRef, LOGPIXELSX);
		screenresx  = GetDeviceCaps(screendc, LOGPIXELSY);
		screenresy  = GetDeviceCaps(screendc, LOGPIXELSX);
		ReleaseDC(NULL,screendc);

		if(destresy != screenresy || screenresx !=destresx)
		{
			if ( drset.NeedToForce72dpi() )
			{
				screenresx = 72.0;
				screenresy = 72.0;
			}

			devicemat.Scale((Real)destresx/screenresx,(Real)destresy/screenresy);
			dst.left=MulDiv(dst.left,destresx,screenresx);
			dst.right=MulDiv(dst.right,destresx,screenresx);
			dst.top=MulDiv(dst.top,destresy,screenresy);
			dst.bottom=MulDiv(dst.bottom,destresy,screenresy);
		}
		
		HRGN newclip=CreateRectRgn(dst.left,dst.top,dst.right,dst.bottom);
		::ExtSelectClipRgn(inPortRef,newclip,RGN_AND);
		
		VAffineTransform finalmat=_MakeFinalMatrix(inBounds,drset,true);
		
		if(drset.GetScaleMode()!=PM_TILE)
		{
			_ApplyDrawingMatrixTranslate(finalmat,drset);
			
			finalmat.Multiply(devicemat,VAffineTransform::MatrixOrderAppend);
			_HDC_TransformSetter trans(inPortRef,finalmat);
			
			test = ::PlayEnhMetaFile(inPortRef, inPict,&rr);
		}
		else
		{
			if(inBounds.GetWidth()>0 && inBounds.GetHeight()>0 && GetWidth()>0 && GetHeight()>0)
			{
				VAffineTransform tmpmat;
				sLONG		nbx,nby,curx,cury,w,h,wtrans;
				VPolygon bounds(GetBounds());
				bounds=finalmat.TransformVector(bounds);
				
				w = bounds.GetWidth();
				h = bounds.GetHeight();
				
				nbx = (inBounds.GetWidth()/w) + (1 * ( (long)inBounds.GetWidth() % w !=0));
				nby = (inBounds.GetHeight()/h) + (1 * ((long)inBounds.GetHeight()% h !=0));
				
				_HDC_TransformSetter setter(inPortRef);
				
				drset.GetXAxisSwap() !=0 ? wtrans= -w : wtrans= w;
				
				for(curx = 0;curx < nbx; curx++)
				{
					for(cury = 0;cury < nby; cury++)
					{
						tmpmat=finalmat;
						tmpmat.Multiply(devicemat,VAffineTransform::MatrixOrderAppend);
						setter.SetTransform(tmpmat);
						test = ::PlayEnhMetaFile(inPortRef, inPict,&rr);
						finalmat.Translate(0,h,VAffineTransform::MatrixOrderAppend);
					}
					finalmat.Translate(wtrans,-(h*nby),VAffineTransform::MatrixOrderAppend);
				}
			}
		}
		
		SelectClipRgn(inPortRef,oldclip);
		if(oldclip)
			DeleteObject(oldclip);
		DeleteObject(newclip);	
	}
}

#endif

#if VERSIONMAC

#if ARCH_32
void VPictureData::xDraw(struct QDPict *inPict,CGContextRef inDC,const VRect& inBounds,VPictureDrawSettings* inSet)const
{
	if(inPict)
	{
		CGAffineTransform affine;
		VPictureDrawSettings set(inSet);
		Rect portrect;
		OSStatus err;
		CGRect vr,clip;
		
		GReal trans=set.GetYAxisSwap();
		GReal scale= 1;

		VAffineTransform mmm;
		VAffineTransform finalmat=_MakeFinalMatrix(inBounds,set,true);
		
		//finalmat.Translate(inBounds.GetX(),trans-(inBounds.GetY() + inBounds.GetHeight()),VAffineTransform::MatrixOrderAppend);
		
		CGContextSaveGState(inDC);

		if(set.GetInterpolationMode()==0)
			CGContextSetInterpolationQuality(inDC,kCGInterpolationNone);


		clip.origin.x= inBounds.GetX();
		clip.origin.y=  trans- (inBounds.GetY() + inBounds.GetHeight());
		clip.size.width=inBounds.GetWidth();
		clip.size.height=inBounds.GetHeight();	
		
		//fixed ACI0072483 
		//CGContextClipToRect(inDC,clip);
		
		vr.origin.x=0;
		vr.origin.y=0;
		vr.size.width= GetWidth();
		vr.size.height= GetHeight();
		
		switch(set.GetDrawingMode())
		{
			case 0:
			case 1:
				break;
			case 2:
			{
				::CGContextSetAlpha(inDC, set.GetAlpha()/(GReal)100);
				break;
			}
		}
		
		if(set.GetScaleMode()!=PM_TILE)
		{
			mmm.Translate(set.GetDrawingMatrix().GetTranslateX(),-set.GetDrawingMatrix().GetTranslateY());
			finalmat.Multiply(mmm,VAffineTransform::MatrixOrderAppend);
			finalmat.ToNativeMatrix(affine);
			CGContextConcatCTM(inDC,affine);
			QDPictDrawToCGContext(inDC,vr,inPict);
		}
		else
		{
			if(inBounds.GetWidth()>0 && inBounds.GetHeight()>0 && GetWidth()>0 && GetHeight()>0)
			{
				CGAffineTransform inverse;
				VPolygon poly(GetBounds());
				sLONG		nbx,nby,curx,cury,w,h;
				poly=set.GetPictureMatrix().TransformVector(poly);
				w = poly.GetWidth();
				h = poly.GetHeight();
				if(w!=0 && h!=0)
				{
					nbx = (inBounds.GetWidth()/w) + (1 * ( (sLONG)inBounds.GetWidth() % w !=0));
					nby = (inBounds.GetHeight()/h) + (1 * ((sLONG)inBounds.GetHeight()% h !=0));
					
									
					for(curx = 0;curx < nbx; curx++)
					{
						for(cury = 0;cury < nby; cury++)
						{
							finalmat.ToNativeMatrix(affine);
							CGContextConcatCTM(inDC,affine);
							QDPictDrawToCGContext(inDC,vr,inPict);
							inverse=CGAffineTransformInvert(affine);
							CGContextConcatCTM(inDC,inverse);
							finalmat.Translate(0,-h,VAffineTransform::MatrixOrderAppend);
						}
						finalmat.Translate(w,(h*nby),VAffineTransform::MatrixOrderAppend);
					}
				}
			}
		}
		CGContextRestoreGState(inDC);
	}
}


void VPictureData::xDraw(struct QDPict *inPict,PortRef inPortRef,const VRect& inBounds,VPictureDrawSettings* inSet)const
{
	if(inPict && sQDBridge)
	{
		sQDBridge->DrawInPortRef(*this, inPict, inPortRef,inBounds,inSet);
	}
}
#endif // #if ARCH_32


void VPictureData::_PrepareCGContext(CGContextRef inDC,const VRect& inBounds,VPictureDrawSettings* inSet)const 
{
	VPictureDrawSettings set(inSet);
	CGAffineTransform affine;
	VAffineTransform mmm;
	VAffineTransform finalmat=_MakeFinalMatrix(inBounds,set,true);
	//finalmat.Translate(inBounds.GetX(),set.GetYAxis()-(inBounds.GetY()+inBounds.GetHeight()),VAffineTransform::MatrixOrderAppend);		
	mmm.Translate(set.GetDrawingMatrix().GetTranslateX(),-set.GetDrawingMatrix().GetTranslateY());
	finalmat.Multiply(mmm,VAffineTransform::MatrixOrderAppend);
	finalmat.ToNativeMatrix(affine);
	CGContextConcatCTM(inDC,affine);
}
void VPictureData::xDraw(CGImageRef inPict,CGContextRef inDC,const VRect& inBounds,VPictureDrawSettings* inSet)const
{
	if(inPict)
	{
		CGAffineTransform affine;
		VPictureDrawSettings set(inSet);
		OSStatus err;
		CGRect vr,clip;

		sLONG trans=set.GetYAxisSwap();

		VAffineTransform mmm;
		VAffineTransform finalmat=_MakeFinalMatrix(inBounds,set,true);
		
		//finalmat.Translate(inBounds.GetX(),trans-(inBounds.GetY()+inBounds.GetHeight()),VAffineTransform::MatrixOrderAppend);		
		
		CGContextSaveGState(inDC);
		
		if(set.GetInterpolationMode()==0 || finalmat.GetScaleX()>1.0 || finalmat.GetScaleY()>1.0)
			CGContextSetInterpolationQuality(inDC,kCGInterpolationNone);
		else
			CGContextSetInterpolationQuality(inDC,kCGInterpolationHigh);
			
		clip.origin.x= inBounds.GetX();
		clip.origin.y=  trans- (inBounds.GetY() + inBounds.GetHeight());
		clip.size.width=inBounds.GetWidth();
		clip.size.height=inBounds.GetHeight();	
		
		//fixed ACI0072483 
		//CGContextClipToRect(inDC,clip);
		
		vr.origin.x= 0;
		vr.origin.y= 0;
		vr.size.width=GetWidth();
		vr.size.height=GetHeight();
				
		switch(set.GetDrawingMode())
		{
			case 0:
			case 1:
				break;
			case 2:
			{
				::CGContextSetAlpha(inDC, set.GetAlpha()/(GReal)100);
				break;
			}
		}
		
		if(set.GetScaleMode()!=PM_TILE)
		{
			mmm.Translate(set.GetDrawingMatrix().GetTranslateX(),-set.GetDrawingMatrix().GetTranslateY());
			finalmat.Multiply(mmm,VAffineTransform::MatrixOrderAppend);
			finalmat.ToNativeMatrix(affine);
			CGContextConcatCTM(inDC,affine);
			CGContextDrawImage(inDC,vr,inPict);
		}
		else
		{
			if(inBounds.GetWidth()>0 && inBounds.GetHeight()>0 && GetWidth()>0 && GetHeight()>0)
			{
				CGAffineTransform inverse;
				VPolygon poly(GetBounds());
				sLONG		nbx,nby,curx,cury,w,h;
				poly=set.GetPictureMatrix().TransformVector(poly);
				w = poly.GetWidth();
				h = poly.GetHeight();
				if(w!=0 && h!=0)
				{
					nbx = (inBounds.GetWidth()/w) + (1 * ( (long)inBounds.GetWidth() % w !=0));
					nby = (inBounds.GetHeight()/h) + (1 * ((long)inBounds.GetHeight()% h !=0));
					
					for(curx = 0;curx < nbx; curx++)
					{
						for(cury = 0;cury < nby; cury++)
						{
							finalmat.ToNativeMatrix(affine);
							CGContextConcatCTM(inDC,affine);
							CGContextDrawImage(inDC,vr,inPict);
							inverse=CGAffineTransformInvert(affine);
							CGContextConcatCTM(inDC,inverse);
							finalmat.Translate(0,-h,VAffineTransform::MatrixOrderAppend);
						}
						finalmat.Translate(w,(h*nby),VAffineTransform::MatrixOrderAppend);
					}
				}
			}
		}
		CGContextRestoreGState(inDC);
	}
}
void VPictureData::xDraw(CGPDFDocumentRef  inPict,CGContextRef inDC,const VRect& inBounds,VPictureDrawSettings* inSet)const
{
	if(inPict)
	{
		CGRect vr,clip;
		CGAffineTransform affine;
		CGPDFPageRef page;
		VPictureDrawSettings set(inSet);
		OSStatus err;
		
		
		VRect dstRect,srcRect;
		sLONG trans=set.GetYAxisSwap();
		
		VAffineTransform mmm;
		VAffineTransform finalmat=_MakeFinalMatrix(inBounds,set,true);
		
		//finalmat.Translate(inBounds.GetX(),trans-(inBounds.GetY() + inBounds.GetHeight()),VAffineTransform::MatrixOrderAppend);
		
		CGContextSaveGState(inDC);
		
		clip.origin.x= inBounds.GetX();
		clip.origin.y= trans-(inBounds.GetY() + inBounds.GetHeight());
		clip.size.width=inBounds.GetWidth();
		clip.size.height=inBounds.GetHeight();				
					
		//fixed ACI0072483 
		//CGContextClipToRect(inDC,clip);

		vr.origin.x=0;
		vr.origin.y=0;
		vr.size.width=GetWidth();
		vr.size.height=GetHeight();

		switch(set.GetDrawingMode())
		{
			case 0:
			case 1:
				break;
			case 2:
			{
				::CGContextSetAlpha(inDC, set.GetAlpha()/(GReal)100);
				break;
			}
		}
		
		page=CGPDFDocumentGetPage (inPict,1);
		CGRect docre={{0,0},{static_cast<CGFloat>(GetWidth()),static_cast<CGFloat>(GetHeight())}};
		CGAffineTransform doctransform = CGPDFPageGetDrawingTransform(page, kCGPDFMediaBox,docre,0,false);
		
		if(set.GetScaleMode()!=PM_TILE)
		{
			page=CGPDFDocumentGetPage (inPict,1);
			mmm.Translate(set.GetDrawingMatrix().GetTranslateX(),-set.GetDrawingMatrix().GetTranslateY());
			finalmat.Multiply(mmm,VAffineTransform::MatrixOrderAppend);
		
			finalmat.ToNativeMatrix(affine);
			CGContextConcatCTM(inDC,affine);
			
			CGContextConcatCTM(inDC,doctransform);
			
			CGContextDrawPDFPage(inDC,page);

		}
		else
		{
			if(inBounds.GetWidth()>0 && inBounds.GetHeight()>0 && GetWidth()>0 && GetHeight()>0)
			{
				CGAffineTransform inverse;
				VPolygon poly(GetBounds());
				sLONG		nbx,nby,curx,cury,w,h;
				poly=set.GetPictureMatrix().TransformVector(poly);
				w = poly.GetWidth();
				h = poly.GetHeight();
				if(w!=0 && h!=0)
				{
					nbx = (inBounds.GetWidth()/w) + (1 * ( (long)inBounds.GetWidth() % w !=0));
					nby = (inBounds.GetHeight()/h) + (1 * ((long)inBounds.GetHeight()% h !=0));
					
					CGAffineTransform inverse_doctransform=CGAffineTransformInvert(doctransform);
													
					for(curx = 0;curx < nbx; curx++)
					{
						for(cury = 0;cury < nby; cury++)
						{
							finalmat.ToNativeMatrix(affine);
							CGContextConcatCTM(inDC,affine);
							CGContextConcatCTM(inDC,doctransform);
							
							CGContextDrawPDFPage(inDC,page);
							
							CGContextConcatCTM(inDC,inverse_doctransform);
							
							inverse=CGAffineTransformInvert(affine);
							CGContextConcatCTM(inDC,inverse);
							
							finalmat.Translate(0,-h,VAffineTransform::MatrixOrderAppend);
						}
						finalmat.Translate(w,(h*nby),VAffineTransform::MatrixOrderAppend);
					}
				}
			}
		}
		CGContextRestoreGState(inDC);
	}
}



void VPictureData::xDraw(CGImageRef inPict,PortRef inPortRef,const VRect& inBounds,VPictureDrawSettings* inSet)const
{
	if(inPict && sQDBridge)
	{
		sQDBridge->DrawInPortRef(*this,inPict,inPortRef,inBounds,inSet);
	}
}

void VPictureData::xDraw(CGPDFDocumentRef inPict,PortRef inPortRef,const VRect& inBounds,VPictureDrawSettings* inSet)const
{
	
	if(inPict && sQDBridge)
	{
		sQDBridge->DrawInPortRef(*this,inPict,inPortRef,inBounds,inSet);
	}
}

#if ARCH_32
void VPictureData::xDraw(xMacPictureHandle inPict,CGContextRef inDC,const VRect& r,VPictureDrawSettings* inSet)const
{
	// attention le rect doit etre dans le system de coords de quartz
	
	CGAffineTransform affine;
	Rect portrect;
	OSStatus err;
	struct QDPict* pictref;
	
	GetMacAllocator()->Lock(inPict);
	CGDataProviderRef dataprov= xV4DPicture_MemoryDataProvider::CGDataProviderCreate(*(char**)inPict,GetMacAllocator()->GetSize(inPict),true);
	GetMacAllocator()->Unlock(inPict);
	pictref=QDPictCreateWithProvider (dataprov);
	CGDataProviderRelease (dataprov);
	if(pictref)
    {
        xDraw(pictref,inDC,r,inSet);
	
        CFRelease(pictref);
    }
}
#endif

#endif


#if WITH_QUICKDRAW
void VPictureData::xDraw(xMacPicture** inPict,PortRef inPortRef,const VRect& inBounds,VPictureDrawSettings* inSet)const
{
	
	if(inPict && sQDBridge)
	{
		sQDBridge->DrawInPortRef(*this,inPict,inPortRef,inBounds,inSet);
	}
}
#else

#endif

VPictureData_NonRenderable::VPictureData_NonRenderable()
{
#if WITH_UNKNOWN_CODEC_PICTURE
	fInfoPict = NULL;
#endif
}

VPictureData_NonRenderable::VPictureData_NonRenderable(VPictureDataProvider* inDataProvider,_VPictureAccumulator* inRecorder)
:inherited(inDataProvider,inRecorder)
{
#if	WITH_UNKNOWN_CODEC_PICTURE
	fInfoPict = NULL;
#endif
}

VPictureData_NonRenderable::VPictureData_NonRenderable(const VPictureData_NonRenderable& inData)
: inherited(inData)
{
#if WITH_UNKNOWN_CODEC_PICTURE
	fInfoPict = RetainRefCountable(inData.fInfoPict);
#endif
}

VPictureData_NonRenderable::~VPictureData_NonRenderable()
{
#if WITH_UNKNOWN_CODEC_PICTURE
	ReleaseRefCountable(&fInfoPict);
#endif
}

#if WITH_UNKNOWN_CODEC_PICTURE

#if !VERSION_LINUX
void VPictureData_NonRenderable::DrawInPortRef(PortRef inPortRef, const VRect& r, VPictureDrawSettings* inSet)const
{
	_Load();
	if (fInfoPict)
	{
		VPictureDrawSettings set(inSet);
		set.SetScaleMode(PM_SCALE_CENTER);
		fInfoPict->DrawInPortRef(inPortRef, r, &set);
	}
}
void VPictureData_NonRenderable::DrawInVGraphicContext(class VGraphicContext& inDC, const VRect& r, VPictureDrawSettings* inSet)const
{
	_Load();
	if (fInfoPict)
	{
		VPictureDrawSettings set(inSet);
		set.SetScaleMode(PM_SCALE_CENTER);
		fInfoPict->DrawInVGraphicContext(inDC, r, &set);
	}
}
#endif   

#if VERSIONWIN 
void VPictureData_NonRenderable::DrawInGDIPlusGraphics(Gdiplus::Graphics* inDC, const VRect& r, VPictureDrawSettings* inSet)const
{
	_Load();
	if (fInfoPict)
	{
		VPictureDrawSettings set(inSet);
		set.SetScaleMode(PM_SCALE_CENTER);
		fInfoPict->DrawInGDIPlusGraphics(inDC, r, &set);
	}
}
#if ENABLE_D2D 
void VPictureData_NonRenderable::DrawInD2DGraphicContext(VWinD2DGraphicContext* inDC, const VRect& r, VPictureDrawSettings* inSet)const
{
	_Load();
	if (fInfoPict)
	{
		VPictureDrawSettings set(inSet);
		set.SetScaleMode(PM_SCALE_CENTER);
		fInfoPict->DrawInD2DGraphicContext(inDC, r, &set);
	}
}
#endif
#elif VERSIONMAC
void VPictureData_NonRenderable::DrawInCGContext(CGContextRef inDC, const VRect& r, VPictureDrawSettings* inSet)const
{
	_Load();
	if(fInfoPict)
	{
		VPictureDrawSettings set(inSet);
		set.SetScaleMode(PM_SCALE_CENTER);
		fInfoPict->DrawInCGContext(inDC, r, &set);
	}
}
#endif

void VPictureData_NonRenderable::_DoLoad()const
{
#if !VERSION_LINUX
	VString xml = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><svg xmlns=\"http://www.w3.org/2000/svg\" height=\"320\" width=\"320\"><path d=\"M40,100 C40,100 40,80 60,80 L60,65 80,65 80,80 C80,80 100,80 100,60 C100,60 100,40 120,40 L180,40 C180,40 200,40 200,60 C200,60 200,80 220,80 L220,60 260,60 260,80 C260,80 280,80 280,100 L280,180 C280,180 280,200 260,200 L60,200 C60,200 40,200 40,180z\" fill=\"grey\" stroke=\"black\" stroke-width=\"2\"/><ellipse cx=\"150\" cy=\"130\" fill=\"rgb(60,60,60)\" rx=\"50\" ry=\"50\" stroke=\"black\" stroke-width=\"2\"/><ellipse cx=\"150\" cy=\"130\" fill=\"lightgrey\" rx=\"44\" ry=\"44\" stroke=\"black\" stroke-width=\"2\"/><line stroke=\"red\" stroke-width=\"12\" x1=\"0\" x2=\"320\" y1=\"0\" y2=\"320\"/><line stroke=\"red\" stroke-width=\"12\" x1=\"0\" x2=\"320\" y1=\"320\" y2=\"0\"/><textArea font-family=\"'Arial Unicode MS','Arial'\" font-size=\"80\" font-style=\"normal\" font-weight=\"normal\" height=\"100\" text-align=\"center\" text-decoration=\"none\" width=\"320\" x=\"0\" y=\"220\">XPICTX</textArea></svg>";
	
	VString codecid = CVSTR("?");
	if (fDecoder)
	{
		VString tmp = fDecoder->GetDefaultIdentifier();
		if (!tmp.IsEmpty())
		{
			codecid = tmp;
			codecid.TranscodeToXML(true);
		}
	}

	xml.Exchange(CVSTR("XPICTX"), codecid);
	{
		XBOX::VStringConvertBuffer buffer(xml, XBOX::VTC_UTF_8);
		sLONG utf8_size = buffer.GetSize();
		VPtr utf8buffer = VMemory::NewPtr(utf8_size, 'pict');

		if (utf8buffer)
		{
			memcpy(utf8buffer, buffer.GetCPointer(), utf8_size);
			VPictureDataProvider* provider = new VPictureDataProvider_VPtr(utf8buffer, utf8_size, 0);
			if (provider)
			{
				VPictureCodecFactoryRef fact;
				
				fInfoPict = fact->CreatePictureData(*provider);
				fBounds.SetCoords(0, 0, 0, 0);
				// pp test si le svg est renderable. Dans le cas de wakanda server le codec svg est un fake qui herite de VPictureData_NonRenderable
				// dans ce context l'appel de GetBounds cause une boucle sans fin
				if (fInfoPict && fInfoPict->IsRenderable())
					fBounds = fInfoPict->GetBounds();
				
				provider->Release();
			}
		}
	}
#endif
}
void VPictureData_NonRenderable::_DoReset()const
{
	ReleaseRefCountable(&fInfoPict);
}
#endif // WITH_UNKNOWN_CODEC_PICTURE

VPictureData_Meta::VPictureData_Meta()
{
	_SetDecoderByExtension(sCodec_4dmeta);
	fRecorder=0;
	fPicture1=0;
	fPicture2=0;
	fOperation=k_HSideBySide;
}
VPictureData_Meta::VPictureData_Meta(const VPictureData_Meta& inData)
:inherited(inData)
{
	fRecorder=0;
	fPicture1=new VPicture();
	fPicture2=new VPicture();
	fPicture1->FromVPicture_Retain(inData.fPicture1,false);
	fPicture2->FromVPicture_Retain(inData.fPicture2,false);
	fOperation=inData.fOperation;
}
VPictureData_Meta::VPictureData_Meta(VPictureDataProvider* inDataProvider,_VPictureAccumulator* inRecorder)
:inherited(inDataProvider,inRecorder)
{
	fPicture1=0;
	fPicture2=0;
	fOperation=0;
	fRecorder=inRecorder;
	if(fRecorder)
		fRecorder->Retain();
	else
		fRecorder=new _VPictureAccumulator();
}

VPictureData_Meta::VPictureData_Meta(class VPicture* inPict1,class VPicture* inPict2,sLONG op)
{
	fRecorder=0;
	_SetDecoderByExtension(sCodec_4dmeta);
	
	fPicture1=new VPicture();
	fPicture1->FromVPicture_Retain(inPict1,false);
	
	fPicture2=new VPicture();
	fPicture2->FromVPicture_Retain(inPict2,false);
	
	fOperation=op;
	
	_InitSize();
}
VPictureData_Meta::~VPictureData_Meta()
{
	if(fPicture1)
		delete fPicture1;
	if(fPicture2)
		delete fPicture2;
	ReleaseRefCountable(&fRecorder);
}
VPictureData* VPictureData_Meta::Clone()const
{
	return new VPictureData_Meta(*this);
}
void VPictureData_Meta::_InitSize()const
{
	VPoint scale1,scale2;
	sLONG width,height;
	switch(fOperation)
	{
		case k_HSideBySide:
		{
			width=fPicture1->GetWidth()+fPicture2->GetWidth();
			height=Max(fPicture1->GetHeight(),fPicture2->GetHeight());
			break;
		}
		case k_VSideBySide:
		{
			height=fPicture1->GetHeight()+fPicture2->GetHeight();
			width=Max(fPicture1->GetWidth(),fPicture2->GetWidth());
			break;
		}
		case k_Stack:
		{
			VRect r1,r2,u;
			r1=fPicture1->GetCoords();
			r2=fPicture2->GetCoords();
			
			u=r1;
			u.Union(r2);
			
			height=u.GetHeight();
			width=u.GetWidth();
			break;
		}
	}
	fBounds.SetCoords(0,0,width,height);
}

#if !VERSION_LINUX
bool VPictureData_Meta::_PrepareDraw(const VRect& inRect,VPictureDrawSettings* inSet,VRect& outRect1,VRect& outRect2,VPictureDrawSettings& outSet1,VPictureDrawSettings& outSet2)const
{	
	if(fPicture1 && fPicture2)
	{
		VRect bounds1,bounds2;
		sLONG savey,savex;
		bool savemx,savemy;
		
		VPictureDrawSettings set(inSet);
		
		VAffineTransform matp1(fPicture1->GetPictureMatrix());
		VAffineTransform matp2(fPicture2->GetPictureMatrix());
		
		outSet1.FromVPictureDrawSettings(set);
		outSet2.FromVPictureDrawSettings(set);
		
		if(set.GetDrawingMode()==2)
		{
			outSet1.SetAlphaBlend(fPicture1->GetSettings().GetAlpha()*(set.GetAlpha()/100.0));
			outSet2.SetAlphaBlend(fPicture2->GetSettings().GetAlpha()*(set.GetAlpha()/100.0));
		}
		else
		{
			if(fPicture1->GetSettings().GetDrawingMode()==2)
				outSet1.SetAlphaBlend(fPicture1->GetSettings().GetAlpha());
			if(fPicture2->GetSettings().GetDrawingMode()==2)
				outSet2.SetAlphaBlend(fPicture2->GetSettings().GetAlpha());
		}
		
		outRect1=inRect;
		outRect2=inRect;
		
		bounds1=fPicture1->GetCoords(true);
		bounds2=fPicture2->GetCoords(true);
		
		VAffineTransform finalmat;
		
		outSet1.SetScaleMode(PM_MATRIX);
		outSet2.SetScaleMode(PM_MATRIX);
		
		// attention il faut desactiver le swap de l axe, sinon les transformations sont cumul√àe
		
		set.GetAxisSwap(savex,savey,savemx,savemy);
		set.SetAxisSwap(0,0,false,false);
		
		finalmat=_MakeFinalMatrix(inRect,set);
		
		set.SetAxisSwap(savex,savey,savemx,savemy);
		
		if(fOperation==k_Stack)
		{
			_AjustFlip(matp1,&bounds1);
			_AjustFlip(matp2,&bounds2);
			
			outSet1.SetPictureMatrix(matp1 * finalmat);
			outSet2.SetPictureMatrix(matp2 * finalmat);
		}
		else
		{
			GReal transx,transy;
		
			VRect pictrect1=fPicture1->GetCoords();
			VRect pictrect2=fPicture2->GetCoords();
			
			transx= fOperation==k_HSideBySide ? pictrect1.GetWidth() : 0;
			transy= fOperation==k_VSideBySide ? pictrect1.GetHeight() : 0;
			
			_AjustFlip(matp1,&bounds1);
			_AjustFlip(matp2,&bounds2);
			
			matp2.Translate(transx,transy,VAffineTransform::MatrixOrderAppend);
			
			
			
			outSet1.SetPictureMatrix(matp1 * finalmat);
			outSet2.SetPictureMatrix(matp2 * finalmat);
			
		}							
		return true;
	}	
	return false;
}
#endif

bool VPictureData_Meta::FindDeprecatedPictureData(bool inLookForMacPicture,bool inLookForQuicktimeCodec)const
{
	const VPictureData_Meta* meta=NULL;
	const VPictureData* pdata=NULL;
	bool result=false;
	
	_Load();

	if(fPicture1)
	{
		meta=(const VPictureData_Meta*)fPicture1->RetainPictDataByIdentifier(sCodec_4dmeta);
		if(meta)
		{
			result=meta->FindDeprecatedPictureData(inLookForMacPicture,inLookForQuicktimeCodec);
			ReleaseRefCountable(&meta);
		}
		else
		{
			if(inLookForMacPicture)
			{
				pdata = fPicture1->RetainPictDataByIdentifier(sCodec_pict);
				result= pdata!=NULL;
				ReleaseRefCountable(&pdata);
			}
			if(!result && inLookForQuicktimeCodec)
			{
				pdata = fPicture1->RetainPictDataForDisplay();
				if(pdata)
				{
					const XBOX::VPictureCodec* codec = pdata->RetainDecoder();
					if(codec)
					{
						VPictureCodecFactoryRef fact;

						result = fact->IsQuicktimeCodec(codec);
		
						XBOX::ReleaseRefCountable(&codec);
					}
					pdata->Release();
				}
			}
		}
	}
	if(fPicture2 && !result)
	{
		meta=(const VPictureData_Meta*)fPicture2->RetainPictDataByIdentifier(sCodec_4dmeta);
		if(meta)
		{
			result=meta->FindDeprecatedPictureData(inLookForMacPicture,inLookForQuicktimeCodec);
			ReleaseRefCountable(&meta);
		}
		else
		{
			if(inLookForMacPicture)
			{
				pdata = fPicture2->RetainPictDataByIdentifier(sCodec_pict);
				result= pdata!=NULL;
				ReleaseRefCountable(&pdata);
			}
			if(!result && inLookForQuicktimeCodec)
			{
				pdata = fPicture2->RetainPictDataForDisplay();
				if(pdata)
				{
					const XBOX::VPictureCodec* codec = pdata->RetainDecoder();
					if(codec)
					{
						VPictureCodecFactoryRef fact;

						result = fact->IsQuicktimeCodec(codec);
		
						XBOX::ReleaseRefCountable(&codec);
					}
					pdata->Release();
				}
			}
		}
	}
	return result;
}

#if !VERSION_LINUX
void VPictureData_Meta::DrawInPortRef(PortRef inPortRef,const VRect& inBounds,VPictureDrawSettings* inSet)const
{
	//_HDC_RightToLeft_SaverSetter rtlsetter(inDC,*inSet);
	
	VRect dst1(0,0,0,0);
	VRect dst2(0,0,0,0);
	
	VPictureDrawSettings set1;
	VPictureDrawSettings set2;
	
	_Load();

	if(_PrepareDraw(inBounds,inSet,dst1,dst2,set1,set2))
	{
		VPictureDrawSettings set(inSet);
		const VPictureData* pData1=fPicture1->RetainPictDataForDisplay();
		const VPictureData* pData2=fPicture2->RetainPictDataForDisplay();
		
		switch(set.GetScaleMode())
		{
			case PM_TILE:
			{
				if(inBounds.GetWidth()>0 && inBounds.GetHeight()>0 && GetWidth()>0 && GetHeight()>0)
				{
					VRect pictRect(GetBounds());
					VPolygon poly(pictRect);
					VRect dr,rr;
					sLONG		nbx,nby,curx,cury,w,h;
					
					poly = set.GetPictureMatrix() * poly;
					pictRect.SetCoords(0,0,poly.GetWidth(),poly.GetHeight());
					
					w = pictRect.GetWidth();
					h = pictRect.GetHeight();
					if(w!=0 && h!=0)
					{
						nbx = (inBounds.GetWidth()/w) + (1 * ( (long)inBounds.GetWidth() % w !=0));
						nby = (inBounds.GetHeight()/h) + (1 * ((long)inBounds.GetHeight()% h !=0));
								
						for(curx = 0;curx < nbx; curx++, dst1.SetPosBy(w,0),dst2.SetPosBy(w,0) )
						{
							GReal y1,y2;
							y1=dst1.GetY();
							y2=dst2.GetY();
							for(cury = 0;cury < nby; cury++, dst1.SetPosBy(0,h),dst2.SetPosBy(0,h) )
							{
								if(pData1)
									pData1->DrawInPortRef(inPortRef,dst1,&set1);
								if(pData2)
									pData2->DrawInPortRef(inPortRef,dst2,&set2);
							}
							dst1.SetY(y1);
							dst2.SetY(y2);
						}
					}
				}
				break;
			}
			default:
			{
				if(pData1)
					pData1->DrawInPortRef(inPortRef,dst1,&set1);
				if(pData2)
					pData2->DrawInPortRef(inPortRef,dst2,&set2);
				break;
			}
		}
		if(pData1)
			pData1->Release();
		if(pData2)
			pData2->Release();
	}
}
#endif

#if VERSIONWIN && ENABLE_D2D
void VPictureData_Meta::DrawInD2DGraphicContext(VWinD2DGraphicContext* inDC,const VRect& inBounds,VPictureDrawSettings* inSet)const
{
	VRect dst1(0,0,0,0);
	VRect dst2(0,0,0,0);
	VPictureDrawSettings set1;
	VPictureDrawSettings set2;
	
	_Load();
	
	if(_PrepareDraw(inBounds,inSet,dst1,dst2,set1,set2))
	{
		VPictureDrawSettings set(*inSet);
		const VPictureData* pData1=fPicture1->RetainPictDataForDisplay();
		const VPictureData* pData2=fPicture2->RetainPictDataForDisplay();
		
		switch(set.GetScaleMode())
		{
			case PM_MATRIX:
			case PM_4D_SCALE_EVEN:
			case PM_4D_SCALE_CENTER:
			case PM_SCALE_EVEN:
			case PM_SCALE_CENTER:
			case PM_SCALE_TO_FIT:
			case PM_CENTER:
			case PM_TOPLEFT:
			{
				if(pData1)
					pData1->DrawInD2DGraphicContext(inDC,dst1,&set1);
				if(pData2)
					pData2->DrawInD2DGraphicContext(inDC,dst2,&set2);
				break;
			}
			case PM_TILE:
			{
				if(inBounds.GetWidth()>0 && inBounds.GetHeight()>0 && GetWidth()>0 && GetHeight()>0)
				{
					VRect dr,rr;
					sLONG		nbx,nby,curx,cury,w,h;
					VPolygon poly(GetBounds());
					poly = set.GetPictureMatrix() * poly;
					
					w = poly.GetWidth();
					h = poly.GetHeight();
					if(w!=0 && h!=0)
					{
						nbx = (inBounds.GetWidth()/w) + (1 * ( (long)inBounds.GetWidth() % w !=0));
						nby = (inBounds.GetHeight()/h) + (1 * ((long)inBounds.GetHeight()% h !=0));
								
						for(curx = 0;curx < nbx; curx++, dst1.SetPosBy(w,0),dst2.SetPosBy(w,0) )
						{
							GReal y1,y2;
							y1=dst1.GetY();
							y2=dst2.GetY();
							for(cury = 0;cury < nby; cury++, dst1.SetPosBy(0,h),dst2.SetPosBy(0,h) )
							{
								if(pData1)
									pData1->DrawInD2DGraphicContext(inDC,dst1,&set1);
								if(pData2)
									pData2->DrawInD2DGraphicContext(inDC,dst2,&set2);
							}
							dst1.SetY(y1);
							dst2.SetY(y2);
						}
					}
				}
				break;
			}
		}
		if(pData1)
			pData1->Release();
		if(pData2)
			pData2->Release();
	}
}
#endif 

#if !VERSION_LINUX
	#if VERSIONWIN
	void VPictureData_Meta::DrawInGDIPlusGraphics(Gdiplus::Graphics* inDC,const VRect& inBounds,VPictureDrawSettings* inSet)const
	#elif VERSIONMAC
	void VPictureData_Meta::DrawInCGContext(CGContextRef inDC,const VRect& inBounds,VPictureDrawSettings* inSet)const
	#endif
{
	VRect dst1(0,0,0,0);
	VRect dst2(0,0,0,0);
	VPictureDrawSettings set1;
	VPictureDrawSettings set2;

	_Load();

	if(_PrepareDraw(inBounds,inSet,dst1,dst2,set1,set2))
	{
		VPictureDrawSettings set(*inSet);
		const VPictureData* pData1=fPicture1->RetainPictDataForDisplay();
		const VPictureData* pData2=fPicture2->RetainPictDataForDisplay();
		
		switch(set.GetScaleMode())
		{
			case PM_MATRIX:
			case PM_4D_SCALE_EVEN:
			case PM_4D_SCALE_CENTER:
			case PM_SCALE_EVEN:
			case PM_SCALE_CENTER:
			case PM_SCALE_TO_FIT:
			case PM_CENTER:
			case PM_TOPLEFT:
			{
				if(pData1)
				#if VERSIONWIN 
					pData1->DrawInGDIPlusGraphics(inDC,dst1,&set1);
				#else
					pData1->DrawInCGContext(inDC,dst1,&set1);
				#endif
				if(pData2)
				#if VERSIONWIN
					pData2->DrawInGDIPlusGraphics(inDC,dst2,&set2);
				#else
					pData2->DrawInCGContext(inDC,dst2,&set2);
				#endif
				break;
			}
			case PM_TILE:
			{
				if(inBounds.GetWidth()>0 && inBounds.GetHeight()>0 && GetWidth()>0 && GetHeight()>0)
				{
					VRect dr,rr;
					sLONG		nbx,nby,curx,cury,w,h;
					VPolygon poly(GetBounds());
					poly = set.GetPictureMatrix() * poly;
					
					w = poly.GetWidth();
					h = poly.GetHeight();
					if(w!=0 && h!=0)
					{
						nbx = (inBounds.GetWidth()/w) + (1 * ( (long)inBounds.GetWidth() % w !=0));
						nby = (inBounds.GetHeight()/h) + (1 * ((long)inBounds.GetHeight()% h !=0));
								
						for(curx = 0;curx < nbx; curx++, dst1.SetPosBy(w,0),dst2.SetPosBy(w,0) )
						{
							GReal y1,y2;
							y1=dst1.GetY();
							y2=dst2.GetY();
							for(cury = 0;cury < nby; cury++, dst1.SetPosBy(0,h),dst2.SetPosBy(0,h) )
							{
								if(pData1)
									#if VERSIONWIN 
									pData1->DrawInGDIPlusGraphics(inDC,dst1,&set1);
									#else
									pData1->DrawInCGContext(inDC,dst1,&set1);
									#endif
								if(pData2)
									#if VERSIONWIN 
									pData2->DrawInGDIPlusGraphics(inDC,dst2,&set2);
									#else
									pData2->DrawInCGContext(inDC,dst2,&set2);
									#endif
							}
							dst1.SetY(y1);
							dst2.SetY(y2);
						}
					}
				}
				break;
			}
		}
		if(pData1)
			pData1->Release();
		if(pData2)
			pData2->Release();
	}
}
#endif

void VPictureData_Meta::_DoLoad()const
{
	if(fDataProvider)
	{
		VPtr dataptr=fDataProvider->BeginDirectAccess();
		if(dataptr)
		{
			_4DPictureMetaHeaderV1 header;
			memcpy(&header,dataptr,sizeof(_4DPictureMetaHeaderV1));
			if(header.fSign=='4MTA' || header.fSign=='ATM4')
			{
				if(header.fSign=='ATM4')
				{
					ByteSwapLongArray((uLONG*)&header,7);
				}
				if(header.fVersion==1)
				{
					fOperation=header.fOperation;
					VConstPtrStream st1(dataptr+header.fPict1Offset,header.fPict1Size);
					fPicture1=new VPicture(&st1,fRecorder);
					
					if(header.fPict2Offset!=-1)
					{
						VConstPtrStream st2(dataptr+header.fPict2Offset,header.fPict2Size);
						fPicture2=new VPicture(&st2,fRecorder);
					}
					else
					{
						fPicture2=new VPicture();
						fPicture2->FromVPicture_Retain(*fPicture1,false);
					}
				}
				else if(header.fVersion==2)
				{
					_4DPictureMetaHeaderV2 headerv2;
					memcpy(&headerv2,dataptr,sizeof(_4DPictureMetaHeaderV2));
					if(headerv2.fSign=='ATM4')
					{
						ByteSwapLongArray((uLONG*)&headerv2,(signed int)8);
					}
					fOperation=headerv2.fOperation;
					
					VPictureDataProvider* ds1=NULL;
					if(headerv2.fPict1Size>0)
						ds1=VPictureDataProvider::Create(fDataProvider,headerv2.fPict1Size, headerv2.fPict1Offset);
					
					VPictureDataProvider* ds2=NULL;
					if(headerv2.fPict2Size>0)
						ds2=VPictureDataProvider::Create(fDataProvider,headerv2.fPict2Size, headerv2.fPict2Offset);
					
					if(ds1)
					{
						VPictureDataStream st1(ds1);
						fPicture1=new VPicture(&st1,fRecorder);
						ds1->Release();
						fPicture1->GetWidth(); // pour forcer le chargement
					}
					else
						fPicture1=new VPicture();
					
					if(ds2)
					{
						VPictureDataStream st2(ds2);
						fPicture2=new VPicture(&st2,fRecorder);
						ds2->Release();
						fPicture2->GetWidth(); // pour forcer le chargement
					}
					else
						fPicture2=new VPicture();
					
					if(headerv2.fSamePicture)
					{
						fPicture2->SetEmpty(true);
						fPicture2->FromVPicture_Retain(fPicture1,true);
					}
				}
				_InitSize();
			}
			fDataProvider->EndDirectAccess();
		}
	}
	ReleaseRefCountable(&fRecorder);
}
void VPictureData_Meta::_DoReset()const
{
	if(fPicture1)
		delete fPicture1;
	if(fPicture2)
		delete fPicture2;
	fPicture1=0;
	fPicture2=0;
}

VError VPictureData_Meta::Save(VBlob* inData,VIndex inOffset,VSize& outSize,_VPictureAccumulator* inRecorder)const
{
	VError err=VE_OK;
	if(fDataProvider)
	{
		return inherited::Save(inData,inOffset,outSize,inRecorder);
	}
	else
	{
		bool samepicture=false;
		_4DPictureMetaHeader header;
		VBlobWithPtr blob1;
		VBlobWithPtr blob2;
		
		if(fPicture1)
		{
			fPicture1->SaveToBlob(&blob1,false,inRecorder);
		}
		if(fPicture2)
		{
			fPicture2->SaveToBlob(&blob2,false,inRecorder);
		}
		header.fSign='4MTA';
		header.fVersion=2;
		header.fOperation=fOperation;
		header.fPict1Offset=sizeof(_4DPictureMetaHeader);
		header.fPict1Size=(sLONG) blob1.GetSize();
		header.fSamePicture=samepicture;
			
		header.fPict2Offset=header.fPict1Offset+header.fPict1Size;
		header.fPict2Size=(sLONG) blob2.GetSize();
			
		outSize=sizeof(_4DPictureMetaHeader)+header.fPict2Size+header.fPict1Size;
		err=inData->SetSize(inData->GetSize()+outSize);
		if(err==VE_OK)
		{
			inData->PutData(&header,sizeof(_4DPictureMetaHeader),inOffset);
			inData->PutData(blob1.GetDataPtr(),header.fPict1Size,inOffset+header.fPict1Offset);
			inData->PutData(blob2.GetDataPtr(),header.fPict2Size,inOffset+header.fPict2Offset);
		}
		else
			outSize=0;
	}
	return err;
}

void VPictureData_Meta::DumpPictureInfo(VString& outDump,sLONG level)const
{
	VString space;
	inherited::DumpPictureInfo(outDump,level);
	
	for(long i=level;i>0;i--)
		space+="\t";
	
	outDump.AppendPrintf("%SMode : ",&space);
	switch(fOperation)
	{
		case k_HSideBySide:
			outDump.AppendPrintf("Horizontal Concat\r\n");
			break;
		case k_VSideBySide:
			outDump.AppendPrintf("Vertical Concat\r\n");
			break;
		case k_Stack:
			outDump.AppendPrintf("Stack\r\n");
			break;
	}
	if(fPicture1)
	{
		outDump.AppendPrintf("%S\tFirst Picture\r\n",&space);
		fPicture1->DumpPictureInfo(outDump,level+1);
	}
	else
	{
		outDump.AppendPrintf("%S\tFirst Picture==NULL\r\n",&space);
	}
	if(fPicture2)
	{
		outDump.AppendPrintf("%S\tSecond Picture\r\n",&space);
		fPicture2->DumpPictureInfo(outDump,level+1);
	}
	else
	{
		outDump.AppendPrintf("%S\tSecond Picture==NULL\r\n",&space);
	}
}

VSize VPictureData_Meta::GetDataSize(_VPictureAccumulator* inRecorder) const
{
	VSize result=0;
	
	if(fPicture1)
		result+=fPicture1->GetDataSize(inRecorder);
	if(fPicture2)
			result+=fPicture2->GetDataSize(inRecorder);
	
	if(result==0 && fDataSourceDirty && fDataProvider) // pp l'image n'est pas encore chargé, on retourne la tailel de la datasource
		result = fDataProvider->GetDataSize();

	return result;
}
#if VERSIONWIN
Gdiplus::Metafile*	VPictureData_Meta::CreateGDIPlus_Metafile(VPictureDrawSettings* inSet)const
{
	VPictureDrawSettings set(inSet);
	VRect bounds;
	VPolygon poly=set.TransformSize(GetWidth(),GetHeight());
	HDC refdc=GetDC(NULL);
	Gdiplus::Metafile* result=new Gdiplus::Metafile(refdc,Gdiplus::EmfTypeEmfPlusDual);
	Gdiplus::Graphics graph(result);
	//set.SetScaleMode(PM_MATRIX);
	bounds.SetCoords(0,0,poly.GetWidth(),poly.GetHeight());
	set.SetScaleMode(PM_SCALE_TO_FIT);
	DrawInGDIPlusGraphics(&graph,bounds,&set);
	ReleaseDC(NULL,refdc);
	return result;
}
HENHMETAFILE	VPictureData_Meta::CreateMetafile(VPictureDrawSettings* inSet)const
{
	HENHMETAFILE result=0;
	Gdiplus::Metafile* meta=CreateGDIPlus_Metafile(inSet);
	if(meta)
	{
		result=meta->GetHENHMETAFILE();
		delete meta;
	}
	return result;
}
HBITMAP	VPictureData_Meta::CreateHBitmap(VPictureDrawSettings* inSet)const
{
	HBITMAP result;
	Gdiplus::Bitmap* bm=CreateGDIPlus_Bitmap(inSet);
	if(bm)
	{
		bm->GetHBITMAP(Gdiplus::Color(0xff,0xff,0xff),&result);
		delete bm;
	}
	return result;
}

HICON VPictureData_Meta::CreateHIcon(VPictureDrawSettings* inSet)const
{
	HICON result = NULL;
	Gdiplus::Bitmap *bm = CreateGDIPlus_Bitmap(inSet);
	if(bm)
	{
		bm->GetHICON(&result);
		delete bm;
	}
	return result;
}

Gdiplus::Bitmap*	VPictureData_Meta::CreateGDIPlus_Bitmap(VPictureDrawSettings* inSet)const
{
	VPictureDrawSettings set(inSet);
	VRect bounds;
	VPolygon poly=set.TransformSize(GetWidth(),GetHeight());
	bounds.SetCoords(0,0,poly.GetWidth(),poly.GetHeight());
	Gdiplus::Bitmap* result=new Gdiplus::Bitmap(poly.GetWidth(),poly.GetHeight(),PixelFormat32bppARGB);
	Gdiplus::Graphics graph(result);
	if(set.GetDrawingMode()==3)
	{
		Gdiplus::Color col;
		col.SetFromCOLORREF(set.GetBackgroundColor().WIN_ToCOLORREF());
		Gdiplus::SolidBrush brush(col);
		Gdiplus::Rect loc(0,0,poly.GetWidth(),poly.GetHeight());
		graph.FillRectangle(&brush,loc);
	}
	//set.SetScaleMode(PM_MATRIX);
	set.SetScaleMode(PM_SCALE_TO_FIT);
	DrawInGDIPlusGraphics(&graph,bounds,&set);
	return result;
}
#endif

/************************************************************************/
// container pour les vpicture, utiliser uniquement pour enregistr√© et charger des vpicture
/*********************************************************************/
VPictureData_VPicture::VPictureData_VPicture()
{
	_SetDecoderByExtension(sCodec_4pct);
	fPicture=0;
}
VPictureData_VPicture::VPictureData_VPicture(const VPictureData_VPicture& inData)
:inherited(inData)
{
	fPicture=new VPicture();
	fPicture->FromVPicture_Retain(inData.GetVPicture(),false);
}
VPictureData_VPicture::VPictureData_VPicture(VPictureDataProvider* inDataProvider,_VPictureAccumulator* inRecorder)
:inherited(inDataProvider,inRecorder)
{
	fPicture=0;
}

VPictureData_VPicture::VPictureData_VPicture(const VPicture& inPict)
{
	_SetDecoderByExtension(sCodec_4pct);
	
	fPicture=new VPicture();
	fPicture->FromVPicture_Retain(inPict,false);
}
VPictureData_VPicture::~VPictureData_VPicture()
{
	if(fPicture)
		delete fPicture;
}
VPictureData* VPictureData_VPicture::Clone()const
{
	return new VPictureData_VPicture(*this);
}
void VPictureData_VPicture::DumpPictureInfo(VString& outDump,sLONG level)const
{
	outDump="VPicture Container\r";
}

#if !VERSION_LINUX
void VPictureData_VPicture::DrawInPortRef(PortRef inPortRef,const VRect& inBounds,VPictureDrawSettings* inSet)const
{
	if(fPicture)
	{
		const XBOX::VPictureData* data=fPicture->RetainPictDataForDisplay();
		if(data)
		{
			data->DrawInPortRef(inPortRef,inBounds,inSet);
			data->Release();
		}
	}
}
#endif

#if VERSIONWIN && ENABLE_D2D
void VPictureData_VPicture::DrawInD2DGraphicContext(VWinD2DGraphicContext* inDC,const VRect& inBounds,VPictureDrawSettings* inSet)const
{
	if(fPicture)
	{
		const XBOX::VPictureData* data=fPicture->RetainPictDataForDisplay();
		if(data)
		{
			data->DrawInD2DGraphicContext(inDC,inBounds,inSet);
			data->Release();
		}
	}
}
#endif

#if !VERSION_LINUX
	#if VERSIONWIN
	void VPictureData_VPicture::DrawInGDIPlusGraphics(Gdiplus::Graphics* inDC,const VRect& inBounds,VPictureDrawSettings* inSet)const
	#elif VERSIONMAC
	void VPictureData_VPicture::DrawInCGContext(CGContextRef inDC,const VRect& inBounds,VPictureDrawSettings* inSet)const
	#endif
{
	if(fPicture)
	{
		const XBOX::VPictureData* data=fPicture->RetainPictDataForDisplay();
		if(data)
		{
			#if VERSIONWIN
			data->DrawInGDIPlusGraphics(inDC,inBounds,inSet);
			#elif VERSIONMAC
			data->DrawInCGContext(inDC,inBounds,inSet);
			#endif
			data->Release();
		}
	}
}
#endif

void VPictureData_VPicture::_DoLoad()const
{
	if(fDataProvider)
	{
		VPtr dataptr=fDataProvider->BeginDirectAccess();
		if(dataptr)
		{
			VConstPtrStream st(dataptr,fDataProvider->GetDataSize());
			fPicture=new VPicture(&st,0);
			fDataProvider->EndDirectAccess();
			fBounds.SetCoords(0,0,0,0);
		}
	}
}

sLONG VPictureData_VPicture::GetWidth()  const
{
	_Load();
	if(fPicture)
		return fPicture->GetHeight();
	else
		return 0;
}
sLONG VPictureData_VPicture::GetHeight() const
{
	_Load();
	if(fPicture)
		return fPicture->GetHeight();
	else
		return 0;
}
sLONG VPictureData_VPicture::GetX()  const
{
	_Load();
	return 0;
}
sLONG VPictureData_VPicture::GetY() const
{
	_Load();
	return 0;
}

const VRect& VPictureData_VPicture::GetBounds() const
{
	_Load();
	if(fPicture)
		fBounds.SetCoords(0,0,fPicture->GetWidth(),fPicture->GetHeight());
	return fBounds;
}
void VPictureData_VPicture::_DoReset()const
{
	if(fPicture)
		delete fPicture;
	fPicture=0;
}

VError VPictureData_VPicture::Save(VBlob* inData,VIndex inOffset,VSize& outSize,_VPictureAccumulator* inRecorder)const
{
	VError err=VE_OK;
	if(fDataProvider)
	{
		return inherited::Save(inData,inOffset,outSize,inRecorder);
	}
	else
	{
		if(fPicture)
		{
			VSize s=inData->GetSize();
			fPicture->SaveToBlob(inData,false,inRecorder);
			outSize=inData->GetSize()-s;
		}
	}
	return err;
}

VSize VPictureData_VPicture::GetDataSize(_VPictureAccumulator* inRecorder) const
{
	VSize result=0;
	
	if(fPicture)
		result=fPicture->GetDataSize();
	return result;
}



VPictureData_NonRenderable_ITKBlobPict::VPictureData_NonRenderable_ITKBlobPict()
{

}
VPictureData_NonRenderable_ITKBlobPict::VPictureData_NonRenderable_ITKBlobPict(VPictureDataProvider* inDataProvider,_VPictureAccumulator* inRecorder)
:inherited(inDataProvider,inRecorder)
{

}
VPictureData_NonRenderable_ITKBlobPict::VPictureData_NonRenderable_ITKBlobPict(const VPictureData_NonRenderable& /*inData*/)
{

}

#if WITH_VMemoryMgr
xMacPictureHandle VPictureData_NonRenderable_ITKBlobPict::CreatePicHandle(VPictureDrawSettings* inSet,bool& outCanAddPicEnd)const
{
	void* outPicHandle=NULL;
	VIndex offset=0;
	outCanAddPicEnd=false;
	if(GetMacAllocator() && fDataProvider && fDataProvider->GetDataSize())
	{
		outPicHandle=GetMacAllocator()->Allocate(fDataProvider->GetDataSize());
	}
	if(outPicHandle)
	{
		VSize datasize=fDataProvider->GetDataSize();
		
		GetMacAllocator()->Lock(outPicHandle);
		fDataProvider->GetData(*(char**)outPicHandle,offset,datasize);
		GetMacAllocator()->Unlock(outPicHandle);
	}
	return (xMacPictureHandle)outPicHandle;
}
#endif

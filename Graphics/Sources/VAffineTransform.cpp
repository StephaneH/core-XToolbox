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
#include "VPoint.h"
#include "VRect.h"
#include "VPolygon.h"
#include "VAffineTransform.h"

#if ENABLE_D2D
#include "XWinD2DGraphicContext.h"
#endif

// identity
//	1	0	0
//	0	1	0
//	0	0	1
// scaling
//	X	0	0
//	0	Y	0
//	0	0	1
// translate
//	1	0	0
//	0	1	0
//	X	Y	1
// shearing
//	1	Y	0
//	X	1	0
//	0	0	1
// rotate
//	cos		sin		0
//	-sin	cos		0
//	0		0		1

using namespace std;

namespace VAffineTransformBagKeys
{
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( m11,VReal,GReal,1);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( m12,VReal,GReal,0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( m13,VReal,GReal,0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( m21,VReal,GReal,0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( m22,VReal,GReal,1);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( m23,VReal,GReal,0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( m31,VReal,GReal,0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( m32,VReal,GReal,0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( m33,VReal,GReal,1);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( idty,VBoolean,bool,0);
};

VError	VAffineTransform::LoadFromBag( const VValueBag& inBag)
{
	VError result=VE_UNKNOWN_ERROR;
	bool identity=false;
	identity=VAffineTransformBagKeys::idty.Get(&inBag);
	if(identity)
	{
		MakeIdentity();
		result=VE_OK;
	}
	else
	{
		fMatrix[0][0]=VAffineTransformBagKeys::m11.Get(&inBag);
		fMatrix[0][1]=VAffineTransformBagKeys::m12.Get(&inBag);
		
		fMatrix[1][0]=VAffineTransformBagKeys::m21.Get(&inBag);
		fMatrix[1][1]=VAffineTransformBagKeys::m22.Get(&inBag);
		
		fMatrix[2][0]=VAffineTransformBagKeys::m31.Get(&inBag);
		fMatrix[2][1]=VAffineTransformBagKeys::m32.Get(&inBag);
		
		#ifndef _mat32
		fMatrix[0][2]=VAffineTransformBagKeys::m13.Get(&inBag);
		fMatrix[1][2]=VAffineTransformBagKeys::m23.Get(&inBag);
		fMatrix[2][2]=VAffineTransformBagKeys::m33.Get(&inBag);
		#endif
		result=VE_OK;
	}
	return result;
}
VError	VAffineTransform::SaveToBag( VValueBag& ioBag, VString& outKind) const
{
	if(IsIdentity())
	{
		VAffineTransformBagKeys::idty.Set(&ioBag,true);
	}
	else
	{
		VAffineTransformBagKeys::m11.Set(&ioBag,fMatrix[0][0]);
		VAffineTransformBagKeys::m12.Set(&ioBag,fMatrix[0][1]);
		
		VAffineTransformBagKeys::m21.Set(&ioBag,fMatrix[1][0]);
		VAffineTransformBagKeys::m22.Set(&ioBag,fMatrix[1][1]);
		
		VAffineTransformBagKeys::m31.Set(&ioBag,fMatrix[2][0]);
		VAffineTransformBagKeys::m32.Set(&ioBag,fMatrix[2][1]);
		
		#ifndef _mat32
		VAffineTransformBagKeys::m13.Set(&ioBag,fMatrix[0][2]);
		VAffineTransformBagKeys::m23.Set(&ioBag,fMatrix[1][2]);
		VAffineTransformBagKeys::m33.Set(&ioBag,fMatrix[2][2]);
		#endif
	}
	outKind = L"AffineTransform";
	return VE_OK;
}
void VAffineTransform::xDebugDump(VString& inInfo)const
{
#if VERSIONDEBUG
	VString s1=inInfo;
	VString buff;
	buff.AppendPrintf("%s sx=%f sy=%f shx=%f shy=%f tx=%f ty=%f\r\n",&s1,GetScaleX(),GetScaleY(),GetShearX(),GetShearY(),GetTranslateX(),GetTranslateY());
	DebugMsg(buff);
#endif
}
#if VERSIONMAC
VAffineTransform::VAffineTransform(const CGAffineTransform& inMat)
{
	FromNativeTransform(inMat);
}
void VAffineTransform::FromNativeTransform(const CGAffineTransform& inMat)
{
	fMatrix[0][0]=inMat.a;
	fMatrix[0][1]=inMat.b;
	
	fMatrix[1][0]=inMat.c;
	fMatrix[1][1]=inMat.d;

	fMatrix[2][0]=inMat.tx;
	fMatrix[2][1]=inMat.ty;
	
	#ifndef _mat32
	fMatrix[0][2]=0;
	fMatrix[1][2]=0;
	fMatrix[2][2]=1;
	#endif
}
#elif VERSIONWIN
VAffineTransform::VAffineTransform(const Gdiplus::Matrix& inMat)
{
	FromNativeTransform(inMat);
}
VAffineTransform::VAffineTransform(const XFORM& inMat)
{
	FromNativeTransform(inMat);
}
#if ENABLE_D2D
VAffineTransform::VAffineTransform(const D2D_MATRIX_REF inMat)
{
	FromNativeTransform(inMat);
}
#endif
void VAffineTransform::FromNativeTransform(const XFORM& inMat)
{
	fMatrix[0][0]=inMat.eM11;
	fMatrix[0][1]=inMat.eM12;
	
	fMatrix[1][0]=inMat.eM21;
	fMatrix[1][1]=inMat.eM22;

	fMatrix[2][0]=inMat.eDx;
	fMatrix[2][1]=inMat.eDy;
	
	#ifndef _mat32
	fMatrix[0][2]=0;
	fMatrix[1][2]=0;
	fMatrix[2][2]=1;
	#endif
}
void VAffineTransform::FromNativeTransform(const Gdiplus::Matrix& inMat)
{
	Gdiplus::REAL mat[6];
	
	inMat.GetElements(mat);
	
	fMatrix[0][0]=mat[0];
	fMatrix[0][1]=mat[1];
	
	fMatrix[1][0]=mat[2];
	fMatrix[1][1]=mat[3];

	fMatrix[2][0]=mat[4];
	fMatrix[2][1]=mat[5];
	
	#ifndef _mat32
	fMatrix[0][2]=0;
	fMatrix[1][2]=0;
	fMatrix[2][2]=1;
	#endif
}

#if ENABLE_D2D
void VAffineTransform::FromNativeTransform(const D2D_MATRIX_REF inMat)
{
	const D2D1_MATRIX_3X2_F *matNative = (const D2D1_MATRIX_3X2_F *)inMat;

	fMatrix[0][0]=matNative->_11;
	fMatrix[0][1]=matNative->_12;
	
	fMatrix[1][0]=matNative->_21;
	fMatrix[1][1]=matNative->_22;

	fMatrix[2][0]=matNative->_31;
	fMatrix[2][1]=matNative->_32;
	
	#ifndef _mat32
	fMatrix[0][2]=0;
	fMatrix[1][2]=0;
	fMatrix[2][2]=1;
	#endif
}
#endif

#endif

VAffineTransform::VAffineTransform(float m11,float m12,float m21,float m22,float m31,float m32)
{
	fMatrix[0][0]=m11;
	fMatrix[0][1]=m12;
	
	fMatrix[1][0]=m21;
	fMatrix[1][1]=m22;

	fMatrix[2][0]=m31;
	fMatrix[2][1]=m32;
	
	#ifndef _mat32
	fMatrix[0][2]=0;
	fMatrix[1][2]=0;
	fMatrix[2][2]=1;
	#endif
}
VAffineTransform::VAffineTransform()
{
	MakeIdentity();
}
VAffineTransform::VAffineTransform(const VAffineTransform& inMat)
{
	if (&inMat == this)
		return;

	//JQ 13/08/2008: faster implementation than memcpy on small arrays
	//				 (because use only registers and no branch test)
	fMatrix[0][0]=inMat.fMatrix[0][0];
	fMatrix[0][1]=inMat.fMatrix[0][1];
	
	fMatrix[1][0]=inMat.fMatrix[1][0];
	fMatrix[1][1]=inMat.fMatrix[1][1];

	fMatrix[2][0]=inMat.fMatrix[2][0];
	fMatrix[2][1]=inMat.fMatrix[2][1];
	
	#ifndef _mat32
	fMatrix[0][2]=inMat.fMatrix[0][2];
	fMatrix[1][2]=inMat.fMatrix[1][2];
	fMatrix[2][2]=inMat.fMatrix[2][2];
	#endif
}
VAffineTransform::~VAffineTransform()
{
}
#if VERSIONWIN
void VAffineTransform::ToNativeMatrix(Gdiplus::Matrix& inMat) const
{
	inMat.SetElements(fMatrix[0][0],fMatrix[0][1],fMatrix[1][0],fMatrix[1][1],fMatrix[2][0],fMatrix[2][1]);
}
void VAffineTransform::ToNativeMatrix(XFORM& inMat)const
{
	inMat.eM11=fMatrix[0][0];
	inMat.eM12=fMatrix[0][1];
	inMat.eM21=fMatrix[1][0];
	inMat.eM22=fMatrix[1][1];
	inMat.eDx=fMatrix[2][0];
	inMat.eDy=fMatrix[2][1];
}
#if ENABLE_D2D
void VAffineTransform::ToNativeMatrix(D2D_MATRIX_REF outMat)const
{
	D2D1_MATRIX_3X2_F *matNative = (D2D1_MATRIX_3X2_F *)outMat;

	matNative->_11=fMatrix[0][0];
	matNative->_12=fMatrix[0][1];
	matNative->_21=fMatrix[1][0];
	matNative->_22=fMatrix[1][1];
	matNative->_31=fMatrix[2][0];
	matNative->_32=fMatrix[2][1];
}
#endif
#elif VERSIONMAC
void VAffineTransform::ToNativeMatrix( CGAffineTransform &inMat)const
{
	inMat.a=fMatrix[0][0];
	inMat.b=fMatrix[0][1];
	inMat.c=fMatrix[1][0];
	inMat.d=fMatrix[1][1];
	inMat.tx=fMatrix[2][0];
	inMat.ty=fMatrix[2][1];

}
#endif
VAffineTransform::MatrixKind VAffineTransform::GetKind()
{
	assert(false);
	return Identity;
}

GReal	VAffineTransform::Determinant () const
{
	// forme complete
	//JQ 13/08/2008: fixed wrong #ifdef test that was leading to random stack corruption error
	#ifndef _mat32
	return	(	  fMatrix[0][0] * fMatrix[1][1] * fMatrix[2][2] 
				- fMatrix[0][0] * fMatrix[1][2] * fMatrix[2][1] 
				+ fMatrix[0][1] * fMatrix[1][2] * fMatrix[2][0] 
				- fMatrix[0][1] * fMatrix[1][0] * fMatrix[2][2] 
				+ fMatrix[0][2] * fMatrix[1][0] * fMatrix[2][1] 
				- fMatrix[0][2] * fMatrix[1][1] * fMatrix[2][0]);
	#else
	// optim pour matrice 2d
	return	(	  fMatrix[0][0] * fMatrix[1][1] - fMatrix[0][1]  * fMatrix[1][0] );	
	#endif		
}
VAffineTransform	VAffineTransform::Adjoint () const
{
	VAffineTransform	result;
	//JQ 13/08/2008: fixed wrong #ifdef test that was leading to random stack corruption error
	#ifndef _mat32
	// first row of cofactors.
	result.fMatrix[0][0] = fMatrix[1][1] * fMatrix[2][2] - fMatrix[1][2] * fMatrix[2][1];
	result.fMatrix[1][0] = fMatrix[1][2] * fMatrix[2][0] - fMatrix[1][0] * fMatrix[2][2];
	result.fMatrix[2][0] = fMatrix[1][0] * fMatrix[2][1] - fMatrix[1][1] * fMatrix[2][0];

	// second row of cofactors.
	result.fMatrix[0][1] = fMatrix[0][2] * fMatrix[2][1] - fMatrix[0][1] * fMatrix[2][2];
	result.fMatrix[1][1] = fMatrix[0][0] * fMatrix[2][2] - fMatrix[0][2] * fMatrix[2][0];
	result.fMatrix[2][1] = fMatrix[0][1] * fMatrix[2][0] - fMatrix[0][0] * fMatrix[2][1];

	// third row of cofactors.
	result.fMatrix[0][2] = fMatrix[0][1] * fMatrix[1][2] - fMatrix[0][2] * fMatrix[1][1];
	result.fMatrix[1][2] = fMatrix[0][2] * fMatrix[1][0] - fMatrix[0][0] * fMatrix[1][2];
	result.fMatrix[2][2] = fMatrix[0][0] * fMatrix[1][1] - fMatrix[0][1] * fMatrix[1][0];
	
	#else
	
	// first row of cofactors.
	result.fMatrix[0][0] = fMatrix[1][1];
	result.fMatrix[1][0] = - fMatrix[1][0];
	result.fMatrix[2][0] = fMatrix[1][0] * fMatrix[2][1] - fMatrix[1][1] * fMatrix[2][0];

	// second row of cofactors.
	result.fMatrix[0][1] = - fMatrix[0][1];
	result.fMatrix[1][1] = fMatrix[0][0];
	result.fMatrix[2][1] = fMatrix[0][1] * fMatrix[2][0] - fMatrix[0][0] * fMatrix[2][1];
	#endif
	return result;
}
VAffineTransform	VAffineTransform::Inverse () const
{
	GReal	determinant = Determinant();
	if (determinant)
		return Adjoint() * ((GReal)1.0/determinant);
	else
		return VAffineTransform();
}
bool VAffineTransform::operator != (const VAffineTransform& inMatrix) const
{
	#ifndef _mat32
	return	((fMatrix[0][0] != inMatrix.fMatrix[0][0]) ||
			 (fMatrix[0][1] != inMatrix.fMatrix[0][1]) ||
			 (fMatrix[0][2] != inMatrix.fMatrix[0][2]) ||
			
			 (fMatrix[1][0] != inMatrix.fMatrix[1][0]) ||
			 (fMatrix[1][1] != inMatrix.fMatrix[1][1]) ||
			 (fMatrix[1][2] != inMatrix.fMatrix[1][2]) ||
			
			 (fMatrix[2][0] != inMatrix.fMatrix[2][0]) ||
			 (fMatrix[2][1] != inMatrix.fMatrix[2][1]) ||
			 (fMatrix[2][2] != inMatrix.fMatrix[2][2]));
	#else
	
	return	((fMatrix[0][0] != inMatrix.fMatrix[0][0]) ||
			 (fMatrix[0][1] != inMatrix.fMatrix[0][1]) ||
			
			 (fMatrix[1][0] != inMatrix.fMatrix[1][0]) ||
			 (fMatrix[1][1] != inMatrix.fMatrix[1][1]) ||
			
			 (fMatrix[2][0] != inMatrix.fMatrix[2][0]) ||
			 (fMatrix[2][1] != inMatrix.fMatrix[2][1]));
	#endif
}
bool VAffineTransform::operator == (const VAffineTransform& inMatrix) const
{
	#ifndef _mat32
	return	((fMatrix[0][0] == inMatrix.fMatrix[0][0]) &&
			 (fMatrix[0][1] == inMatrix.fMatrix[0][1]) &&
			 (fMatrix[0][2] == inMatrix.fMatrix[0][2]) &&
			
			 (fMatrix[1][0] == inMatrix.fMatrix[1][0]) &&
			 (fMatrix[1][1] == inMatrix.fMatrix[1][1]) &&
			 (fMatrix[1][2] == inMatrix.fMatrix[1][2]) &&
			
			 (fMatrix[2][0] == inMatrix.fMatrix[2][0]) &&
			 (fMatrix[2][1] == inMatrix.fMatrix[2][1]) &&
			 (fMatrix[2][2] == inMatrix.fMatrix[2][2]));
	#else
	return	((fMatrix[0][0] == inMatrix.fMatrix[0][0]) &&
			 (fMatrix[0][1] == inMatrix.fMatrix[0][1]) &&
			
			 (fMatrix[1][0] == inMatrix.fMatrix[1][0]) &&
			 (fMatrix[1][1] == inMatrix.fMatrix[1][1]) &&
			
			 (fMatrix[2][0] == inMatrix.fMatrix[2][0]) &&
			 (fMatrix[2][1] == inMatrix.fMatrix[2][1]));
	#endif
}
VAffineTransform VAffineTransform::operator +(const VAffineTransform& inMat) const
{
	VAffineTransform result;
	#ifndef _mat32
	result.fMatrix[0][0]= fMatrix[0][0]+inMat.fMatrix[0][0];
	result.fMatrix[0][1]= fMatrix[0][1]+inMat.fMatrix[0][1];
	result.fMatrix[0][2]= fMatrix[0][2]+inMat.fMatrix[0][2];

	result.fMatrix[1][0]= fMatrix[1][0]+inMat.fMatrix[1][0];
	result.fMatrix[1][1]= fMatrix[1][1]+inMat.fMatrix[1][1];
	result.fMatrix[1][2]= fMatrix[1][2]+inMat.fMatrix[1][2];

	result.fMatrix[2][0]= fMatrix[2][0]+inMat.fMatrix[2][0];
	result.fMatrix[2][1]= fMatrix[2][1]+inMat.fMatrix[2][1];
	result.fMatrix[2][2]= fMatrix[2][2]+inMat.fMatrix[2][2];
	#else
	result.fMatrix[0][0]= fMatrix[0][0]+inMat.fMatrix[0][0];
	result.fMatrix[0][1]= fMatrix[0][1]+inMat.fMatrix[0][1];

	result.fMatrix[1][0]= fMatrix[1][0]+inMat.fMatrix[1][0];
	result.fMatrix[1][1]= fMatrix[1][1]+inMat.fMatrix[1][1];

	result.fMatrix[2][0]= fMatrix[2][0]+inMat.fMatrix[2][0];
	result.fMatrix[2][1]= fMatrix[2][1]+inMat.fMatrix[2][1];
	#endif
	return result;
}
VAffineTransform VAffineTransform::operator *(const VAffineTransform& inMat) const
{
	VAffineTransform result;
	
	if(inMat.IsIdentity())
	{
		result=*this;
	}
	else if(IsIdentity())
	{
		result=inMat;
	}
	else
	{
		#ifndef _mat32
		result.fMatrix[0][0]= (fMatrix[0][0]*inMat.fMatrix[0][0]) + (fMatrix[0][1]*inMat.fMatrix[1][0]) + (fMatrix[0][2]*inMat.fMatrix[2][0]);
		result.fMatrix[0][1]= (fMatrix[0][0]*inMat.fMatrix[0][1]) + (fMatrix[0][1]*inMat.fMatrix[1][1]) + (fMatrix[0][2]*inMat.fMatrix[2][1]);
		result.fMatrix[0][2]= (fMatrix[0][0]*inMat.fMatrix[0][2]) + (fMatrix[0][1]*inMat.fMatrix[1][2]) + (fMatrix[0][2]*inMat.fMatrix[2][2]);

		result.fMatrix[1][0]= (fMatrix[1][0]*inMat.fMatrix[0][0]) + (fMatrix[1][1]*inMat.fMatrix[1][0]) + (fMatrix[1][2]*inMat.fMatrix[2][0]);
		result.fMatrix[1][1]= (fMatrix[1][0]*inMat.fMatrix[0][1]) + (fMatrix[1][1]*inMat.fMatrix[1][1]) + (fMatrix[1][2]*inMat.fMatrix[2][1]);
		result.fMatrix[1][2]= (fMatrix[1][0]*inMat.fMatrix[0][2]) + (fMatrix[1][1]*inMat.fMatrix[1][2]) + (fMatrix[1][2]*inMat.fMatrix[2][2]);

		result.fMatrix[2][0]= (fMatrix[2][0]*inMat.fMatrix[0][0]) + (fMatrix[2][1]*inMat.fMatrix[1][0]) + (fMatrix[2][2]*inMat.fMatrix[2][0]);
		result.fMatrix[2][1]= (fMatrix[2][0]*inMat.fMatrix[0][1]) + (fMatrix[2][1]*inMat.fMatrix[1][1]) + (fMatrix[2][2]*inMat.fMatrix[2][1]);
		result.fMatrix[2][2]= (fMatrix[2][0]*inMat.fMatrix[0][2]) + (fMatrix[2][1]*inMat.fMatrix[1][2]) + (fMatrix[2][2]*inMat.fMatrix[2][2]);
		#else
		result.fMatrix[0][0]= (fMatrix[0][0]*inMat.fMatrix[0][0]) + (fMatrix[0][1]*inMat.fMatrix[1][0]) ;
		result.fMatrix[0][1]= (fMatrix[0][0]*inMat.fMatrix[0][1]) + (fMatrix[0][1]*inMat.fMatrix[1][1]) ;
	
		result.fMatrix[1][0]= (fMatrix[1][0]*inMat.fMatrix[0][0]) + (fMatrix[1][1]*inMat.fMatrix[1][0]) ;
		result.fMatrix[1][1]= (fMatrix[1][0]*inMat.fMatrix[0][1]) + (fMatrix[1][1]*inMat.fMatrix[1][1]) ;
		
		result.fMatrix[2][0]= (fMatrix[2][0]*inMat.fMatrix[0][0]) + (fMatrix[2][1]*inMat.fMatrix[1][0]) + inMat.fMatrix[2][0];
		result.fMatrix[2][1]= (fMatrix[2][0]*inMat.fMatrix[0][1]) + (fMatrix[2][1]*inMat.fMatrix[1][1]) + inMat.fMatrix[2][1];
		#endif
	}
	return result;
}
VPoint VAffineTransform::TransformVector(const VPoint& inVector) const
{
	return VPoint(	(fMatrix[0][0] * inVector.x + fMatrix[1][0] * inVector.y ),
					(fMatrix[0][1] * inVector.x + fMatrix[1][1] * inVector.y ));
}

#if !VERSION_LINUX
VPolygon VAffineTransform::TransformVector(const VPolygon& inVector) const
{
	VPolygon result;
	if(!IsIdentity())
	{
		for(VIndex i=1;i<=inVector.GetPointCount();i++)
		{
			VPoint p;
			inVector.GetNthPoint(i,p);
			p=TransformVector(p);
			result.AddPoint(p);
		}
	}
	else
	{
		result=inVector;
	}
	return result;
}
#endif

VRect VAffineTransform::TransformVector(const VRect& inVector) const
{
	if(!IsIdentity())
	{
		VRect result;
		VPoint p0,p1;
		p0=inVector.GetTopLeft();
		p1=inVector.GetBotRight();
		p0=TransformVector(p0);
		p1=TransformVector(p1);
		result.SetCoords(p0.GetX(),p0.GetY(),p1.GetX()-p0.GetX(),p1.GetY()-p0.GetY());
		return result;
		
	}
	else
		return inVector;

}

#if !VERSION_LINUX
VPolygon		VAffineTransform::operator * (const VPolygon& inVector) const
{
	VPolygon result;
	if(!IsIdentity())
	{
		for(VIndex i=1;i<=inVector.GetPointCount();i++)
		{
			VPoint p;
			inVector.GetNthPoint(i,p);
			p=operator*(p);
			result.AddPoint(p);
		}
	}
	else
	{
		result=inVector;
	}
	return result;
}
#endif

VPoint		VAffineTransform::operator * (const VPoint& inVector) const
{
	return VPoint(	(fMatrix[0][0] * inVector.x + fMatrix[1][0] * inVector.y + fMatrix[2][0]),
					(fMatrix[0][1] * inVector.x + fMatrix[1][1] * inVector.y + fMatrix[2][1]));
}
VRect	VAffineTransform::operator * (const VRect& inRect) const
{
	if(!IsIdentity())
	{
		VRect result;
		VPoint aa(inRect.GetX(),inRect.GetY());
		VPoint bb(inRect.GetX()+inRect.GetWidth(),inRect.GetY());
		VPoint cc(inRect.GetX(),inRect.GetY()+inRect.GetHeight());
		VPoint dd(inRect.GetX()+inRect.GetWidth(),inRect.GetY()+inRect.GetHeight());
		aa = TransformPoint( aa);
		bb = TransformPoint( bb);
		cc = TransformPoint( cc);
		dd = TransformPoint( dd);
		
		GReal x1 = aa.GetX();
		GReal y1 = aa.GetY();
		GReal x2 = x1;
		GReal y2 = y1;
		if (bb.GetX() < x1)
			x1 = bb.GetX();
		if (cc.GetX() < x1)
			x1 = cc.GetX();
		if (dd.GetX() < x1)
			x1 = dd.GetX();
		
		if (bb.GetX() > x2)
			x2 = bb.GetX();
		if (cc.GetX() > x2)
			x2 = cc.GetX();
		if (dd.GetX() > x2)
			x2 = dd.GetX();
		
		if (bb.GetY() < y1)
			y1 = bb.GetY();
		if (cc.GetY() < y1)
			y1 = cc.GetY();
		if (dd.GetY() < y1)
			y1 = dd.GetY();
		
		if (bb.GetY() > y2)
			y2 = bb.GetY();
		if (cc.GetY() > y2)
			y2 = cc.GetY();
		if (dd.GetY() > y2)
			y2 = dd.GetY();
		
		result.SetCoords(x1,y1,x2-x1,y2-y1);
		return result;
	}
	else
	{
		return inRect; 
	}
}

VAffineTransform	VAffineTransform::operator * (GReal inValue) const
{
	VAffineTransform	result;
	if (inValue != 0)
	{
		#ifndef _mat32
		result.fMatrix[0][0]=fMatrix[0][0]* inValue;
		result.fMatrix[0][1]=fMatrix[0][1]* inValue;
		result.fMatrix[0][2]=fMatrix[0][2]* inValue;	
		
		result.fMatrix[1][0]=fMatrix[1][0]* inValue;
		result.fMatrix[1][1]=fMatrix[1][1]* inValue;
		result.fMatrix[1][2]=fMatrix[1][2]* inValue;	
		
		result.fMatrix[2][0]=fMatrix[2][0]* inValue;
		result.fMatrix[2][1]=fMatrix[2][1]* inValue;
		result.fMatrix[2][2]=fMatrix[2][2]* inValue;	
		#else
		result.fMatrix[0][0]=fMatrix[0][0]* inValue;
		result.fMatrix[0][1]=fMatrix[0][1]* inValue;
		
		result.fMatrix[1][0]=fMatrix[1][0]* inValue;
		result.fMatrix[1][1]=fMatrix[1][1]* inValue;
			
		result.fMatrix[2][0]=fMatrix[2][0]* inValue;
		result.fMatrix[2][1]=fMatrix[2][1]* inValue;
			
		#endif
	}
	else
		result.Reset();
	return result;
}

VAffineTransform	VAffineTransform::operator / (GReal inValue) const
{
	VAffineTransform	result;
	if (inValue != 0)
	{
		#ifndef _mat32
		result.SetElement(0,0,fMatrix[0][0]/ inValue);
		result.SetElement(0,1,fMatrix[0][1]/ inValue);
		result.SetElement(0,2,fMatrix[0][2]/ inValue);	
		
		result.SetElement(1,0,fMatrix[1][0]/ inValue);
		result.SetElement(1,1,fMatrix[1][1]/ inValue);
		result.SetElement(1,2,fMatrix[1][2]/ inValue);	
		
		result.SetElement(2,0,fMatrix[2][0]/ inValue);
		result.SetElement(2,1,fMatrix[2][1]/ inValue);
		result.SetElement(2,2,fMatrix[2][2]/ inValue);
		#else
		result.SetElement(0,0,fMatrix[0][0]/ inValue);
		result.SetElement(0,1,fMatrix[0][1]/ inValue);
			
		result.SetElement(1,0,fMatrix[1][0]/ inValue);
		result.SetElement(1,1,fMatrix[1][1]/ inValue);
			
		result.SetElement(2,0,fMatrix[2][0]/ inValue);
		result.SetElement(2,1,fMatrix[2][1]/ inValue);
		
		#endif	
	}
	return result;
}

void VAffineTransform::Reset()
{
	memset(fMatrix,0,sizeof(_Matrix));
}
void VAffineTransform::MakeIdentity()
{
	Reset();
	fMatrix[0][0]=1;
	fMatrix[1][1]=1;
	#ifndef _mat32
	fMatrix[2][2]=1;
	#endif
}
bool VAffineTransform::IsIdentity()const
{
	#ifndef _mat32
	return	fMatrix[0][0]==1 && fMatrix[0][1]==0  && fMatrix[0][2]==0 &&
			fMatrix[1][0]==0 && fMatrix[1][1]==1  && fMatrix[1][2]==0 &&
			fMatrix[2][0]==0 && fMatrix[2][1]==0   && fMatrix[2][2]==1;
			
	#else
	return	fMatrix[0][0]==1 && fMatrix[0][1]==0 && 
			fMatrix[1][0]==0 && fMatrix[1][1]==1 &&
			fMatrix[2][0]==0 && fMatrix[2][1]==0 ;
	#endif
}
void VAffineTransform::Scale(GReal sX,GReal sY,MatrixOrder inOrder)
{
	VAffineTransform scale(sX,0,0,sY,0,0);
	if(inOrder==MatrixOrderAppend)
	{
		operator*=(scale);
	}
	else
	{
		*this = scale.operator*(*this);
	}
}
void VAffineTransform::Translate(GReal sX,GReal sY,MatrixOrder inOrder)
{
	VAffineTransform scale(1,0,0,1,sX,sY);
	if(inOrder==MatrixOrderAppend)
	{
		operator*=(scale);
	}
	else
	{
		*this = scale.operator*(*this);
	}
}
void VAffineTransform::Shear(GReal sX,GReal sY,MatrixOrder inOrder)
{
	VAffineTransform scale(1,sY,sX,1,0,0);
	if(inOrder==MatrixOrderAppend)
	{
		operator*=(scale);
	}
	else
	{
		*this = scale.operator*(*this);
	}
}

void VAffineTransform::Multiply(const VAffineTransform& inMat,MatrixOrder inOrder)
{
	if(inOrder==MatrixOrderAppend)
	{
		operator*=(inMat);
	}
	else
	{
		*this = inMat.operator*(*this);
	}
}

void VAffineTransform::Rotate(GReal inAngle,MatrixOrder inOrder)
{
	GReal pi_value = (GReal) 3.14159265358979323846;
	GReal sinus=sin((inAngle*pi_value)/180);
	GReal cosinus=cos((inAngle*pi_value)/180);
	
	VAffineTransform rot(cosinus,sinus,-sinus,cosinus,0,0);
	
	if(inOrder==MatrixOrderAppend)
	{
		operator*=(rot);
	}
	else
	{
		*this = rot.operator*(*this);
	}
}

void VAffineTransform::RotateAt(GReal inAngle,GReal inX,GReal inY,MatrixOrder inOrder)
{
	if(inOrder==MatrixOrderPrepend)
	{
		Translate(inX,inY,inOrder);
		Rotate(inAngle,inOrder);
		Translate(-inX,-inY,inOrder);
	}
	else
	{
		Translate(-inX,-inY,inOrder);
		Rotate(inAngle,inOrder);
		Translate(inX,inY,inOrder);
	}
}

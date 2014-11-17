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
#ifndef __VAffineTransform__
#define __VAffineTransform__

BEGIN_TOOLBOX_NAMESPACE

typedef void *D2D_MATRIX_REF;

// identity
//	1	0	0
//	0	1	0
//	0	0	1
// scaling
//	X	0	0
//	0	Y	0
//	0	0	1
// translate
//	1	0	X
//	0	1	Y
//	0	0	1
// shearing
//	1	X	0
//	Y	1	0
//	0	0	1
// rotate
//	cos		-sin	0
//	sin		cos		0
//	0		0		1


#define _mat32

class XTOOLBOX_API VAffineTransform : public VObject,public IBaggable
{
public:
	// IBaggable interface
	virtual VError	LoadFromBag( const VValueBag& inBag);
	virtual VError	SaveToBag( VValueBag& ioBag, VString& outKind) const;
	/**********************/
	
	typedef enum MatrixOrder
	{
		MatrixOrderPrepend=0,
		MatrixOrderAppend=1
	}MatrixOrder;
	
	typedef enum MatrixKind
	{
		Identity=0x0,
		Scaling=0x01,
		Scaling_Uniform=0x02,
		Translation=0x04
			
	}MatrixKind;
#if VERSIONMAC
	VAffineTransform(const CGAffineTransform& inMat);
	void FromNativeTransform(const CGAffineTransform& inMat);
#elif VERSIONWIN
	VAffineTransform(const Gdiplus::Matrix& inMat);
	VAffineTransform(const XFORM& inMat);
	#if ENABLE_D2D
	VAffineTransform(const D2D_MATRIX_REF inMat);
	#endif
	void FromNativeTransform(const XFORM& inMat);
	void FromNativeTransform(const Gdiplus::Matrix& inMat);
	#if ENABLE_D2D
	void FromNativeTransform(const D2D_MATRIX_REF inMat);
	#endif
#endif
	VAffineTransform(float m11,float m12,float m13,float m21,float m22,float m23);
	VAffineTransform();
	VAffineTransform(const VAffineTransform& inMat);
	virtual ~VAffineTransform();
	
	MatrixKind GetKind();
	const GReal&	operator () (VIndex inLine,VIndex inCol) const
	{
		return fMatrix[inLine][inCol]; 
	}
	GReal	operator () (VIndex inLine,VIndex inCol)
	{
		return fMatrix[inLine][inCol]; 
	}
	GReal	Determinant () const;
	VAffineTransform	Adjoint () const;
	VAffineTransform	Inverse () const;
	bool operator != (const VAffineTransform& inMatrix) const;
	bool operator == (const VAffineTransform& inMatrix) const;
	
	VAffineTransform operator +(const VAffineTransform& inMat) const;
	void operator +=(const VAffineTransform& inMat)
	{
		*this=operator+(inMat);
	}
	
	VAffineTransform operator *(const VAffineTransform& inMat) const;
	void operator *=(const VAffineTransform& inMat)
	{
		*this=operator*(inMat);
	}
	VPoint TransformVector(const VPoint& inVector) const;
	VRect TransformVector(const VRect& inVector) const;

#if !VERSION_LINUX
	VPolygon TransformVector(const VPolygon& inVector) const;
#endif	

	VPoint		operator * (const VPoint& inVector) const;
	VRect	operator * (const VRect& inRect) const;

#if !VERSION_LINUX
	VPolygon	operator * (const VPolygon& inRect) const;
#endif

	void Multiply(const VAffineTransform& inMat,MatrixOrder inOrder=MatrixOrderPrepend);

#if !VERSION_LINUX
	VPolygon	TransformPolygon(const VPolygon& inPoly) const
	{
		return operator*(inPoly);
	}
#endif

	VPoint	TransformPoint(const VPoint& inPoint) const
	{
		return operator*(inPoint);
	}
	VRect	TransformRect(const VRect& inRect) const
	{
		return operator*(inRect);
	}
	VAffineTransform	operator * (GReal inValue) const;
	void	operator *= (GReal inValue)
	{
		*this = operator*(inValue);
	}
	VAffineTransform	operator / (GReal inValue) const;
	void	operator /= (GReal inValue)
	{
		*this = operator/(inValue);
	}
#if VERSIONWIN
	void ToNativeMatrix(Gdiplus::Matrix& inMat)const;
	#if ENABLE_D2D
	void ToNativeMatrix(D2D_MATRIX_REF outMat)const;
	#endif
	void ToNativeMatrix(XFORM& inMat)const;
#elif VERSIONMAC
	void ToNativeMatrix( CGAffineTransform &inMat)const;
#endif
	void xDebugDump(VString& inInfo)const;
	void Reset();
	void MakeIdentity();
	bool IsIdentity()const;
	void Scale(GReal sX,GReal sY,MatrixOrder inOrder=MatrixOrderPrepend);
	void Translate(GReal sX,GReal sY,MatrixOrder inOrder=MatrixOrderPrepend);
	void Shear(GReal sX,GReal sY,MatrixOrder inOrder=MatrixOrderPrepend);
	void Rotate(GReal inAngle,MatrixOrder inOrder=MatrixOrderPrepend);
	void RotateAt(GReal inAngle,GReal inX,GReal inY,MatrixOrder inOrder=MatrixOrderPrepend);
	void SetElement(VIndex inLine,VIndex inCol,GReal inVal)
	{
		fMatrix[inLine][inCol]=inVal;
	}
	void SetTranslation(GReal inX,GReal inY) 
	{
		fMatrix[2][0]=inX;
		fMatrix[2][1]=inY;
	}
	VPoint GetTranslation() const
	{
		return VPoint(fMatrix[2][0],fMatrix[2][1]);
	}
	void SetScaling(GReal inX,GReal inY) 
	{
		fMatrix[0][0]=inX;
		fMatrix[1][1]=inY;
	}
	VPoint GetScaling() const
	{
		return VPoint(fMatrix[0][0],fMatrix[1][1]);
	}
	void SetShearing(GReal inX,GReal inY) 
	{
		fMatrix[1][0]=inX;
		fMatrix[0][1]=inY;
	}
	VPoint GetShearing() const
	{
		return VPoint(fMatrix[1][0],fMatrix[0][1]);
	}
	GReal GetTranslateX() const
	{
		return fMatrix[2][0];
	}
	GReal GetTranslateY() const
	{
		return fMatrix[2][1];
	}
	GReal GetScaleX()const
	{
		return fMatrix[0][0];
	}
	GReal GetScaleY()const
	{
		return fMatrix[1][1];
	}
	GReal GetShearX()const
	{
		return fMatrix[1][0];
	}
	GReal GetShearY()const
	{
		return fMatrix[0][1];
	}
private:
	#ifndef _mat32
	typedef GReal _Matrix[3][3]; 
	#else
	typedef GReal _Matrix[3][2]; 
	#endif
	_Matrix fMatrix;
};

END_TOOLBOX_NAMESPACE
#endif

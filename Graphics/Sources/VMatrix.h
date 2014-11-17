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
#ifndef __VMatrix__
#define __VMatrix__

#include <math.h>


BEGIN_TOOLBOX_NAMESPACE

// 	The matrix is described as a one dimension array dispatched as follows:
//	arrayIndex = i * 3 + j with i as row number and j as column number
//	[index = 0, j = 0]	[index = 0, j = 1]	[index = 0, j = 2]
//	[index = 1, j = 0]	[index = 1, j = 1]	[index = 1, j = 2]
//	[index = 2, j = 0]	[index = 2, j = 1]	[index = 2, j = 2]


template <class T>
class VMatrixOf
{
public:
	VMatrixOf (T inEl[9]) : fIsIdentity(false), fRotationAngle(0), fScalingVector(1, 1)
	{
		memcpy(fElements,inEl,sizeof(inEl));
		
		fTranslationVector.x=fElements[0 * 3 + 2];
		fTranslationVector.y=fElements[1 * 3 + 2];
		
		fScalingVector.x=fElements[0 * 3 + 0];
		fScalingVector.y=fElements[1 * 3 + 1];
		fIsIdentity=IsIdentity();
	}
	VMatrixOf () : fIsIdentity(false), fRotationAngle(0), fScalingVector(1, 1)
	{
		MakeIdentity();
	}
	
	
	VMatrixOf (const VMatrixOf<T>& inOriginal)
	{
		_CopyElementsFrom(inOriginal);
		fIsIdentity = inOriginal.fIsIdentity;
		fRotationAngle = inOriginal.fRotationAngle;
		fTranslationVector = inOriginal.fTranslationVector;
		fScalingVector = inOriginal.fScalingVector;
		fShearingVector = inOriginal.fShearingVector;
	}
	
	virtual ~VMatrixOf ()
	{
	}
	
	void WriteToBuff(T outEl[9]) const 
	{
		memcpy(outEl,fElements,sizeof(T)*9);
	}
	void ReadFromBuff(T inEl[9]) 
	{
		memcpy(fElements,inEl,sizeof(T)*9);
		
		fTranslationVector.x=fElements[0 * 3 + 2];
		fTranslationVector.y=fElements[1 * 3 + 2];
		
		fScalingVector.x=fElements[0 * 3 + 0];
		fScalingVector.y=fElements[1 * 3 + 1];
		fIsIdentity=IsIdentity();
	}
	
	// Basics support
	void MakeEmpty ()
	{
		for (sLONG index = 0; index < 9; index++)
			fElements[index] = 0;
			
		fIsIdentity = false;
	}


	void MakeIdentity ()
	{
		MakeEmpty();
		
		for (sLONG index = 0; index < 3; index++)
			fElements[index * 3 + index] = 1;
		
		fRotationAngle=0;
		fShearingVector.SetPos(0,0);
		fScalingVector.SetPos(1, 1);
		fTranslationVector.SetPos(0,0);
		fIsIdentity = true;
	}


	bool IsIdentity () const
	{
		return	((fElements[0 * 3 + 0] == 1) &&
				 (fElements[1 * 3 + 1] == 1) &&
				 (fElements[2 * 3 + 2] == 1) &&
					
				 (fElements[0 * 3 + 1] == 0) &&
				 (fElements[0 * 3 + 2] == 0) &&
					
				 (fElements[1 * 3 + 0] == 0.0) &&
				 (fElements[1 * 3 + 2] == 0.0) &&
				
				 (fElements[2 * 3 + 0] == 0.0) &&
				 (fElements[2 * 3 + 1] == 0.0));
	}
	
	
	// Mathematical support
	T	Determinant () const
	{
		return	(fElements[0 * 3 + 0] * fElements[1 * 3 + 1] * fElements[2 * 3 + 2]
				 - fElements[0 * 3 + 0] * fElements[1 * 3 + 2] * fElements[2 * 3 + 1]
				 + fElements[0 * 3 + 1] * fElements[1 * 3 + 2] * fElements[2 * 3 + 0]
				 - fElements[0 * 3 + 1] * fElements[1 * 3 + 0] * fElements[2 * 3 + 2]
				 + fElements[0 * 3 + 2] * fElements[1 * 3 + 0] * fElements[2 * 3 + 1]
				 - fElements[0 * 3 + 2] * fElements[1 * 3 + 1] * fElements[2 * 3 + 0]);
	}


	VMatrixOf<T>	Adjoint () const
	{
		VMatrixOf<T>	result;

		// first row of cofactors.
		result[0 * 3 + 0] = fElements[1 * 3 + 1] * fElements[2 * 3 + 2] - fElements[1 * 3 + 2] * fElements[2 * 3 + 1];
		result[1 * 3 + 0] = fElements[1 * 3 + 2] * fElements[2 * 3 + 0] - fElements[1 * 3 + 0] * fElements[2 * 3 + 2];
		result[2 * 3 + 0] = fElements[1 * 3 + 0] * fElements[2 * 3 + 1] - fElements[1 * 3 + 1] * fElements[2 * 3 + 0];

		// second row of cofactors.
		result[0 * 3 + 1] = fElements[0 * 3 + 2] * fElements[2 * 3 + 1] - fElements[0 * 3 + 1] * fElements[2 * 3 + 2];
		result[1 * 3 + 1] = fElements[0 * 3 + 0] * fElements[2 * 3 + 2] - fElements[0 * 3 + 2] * fElements[2 * 3 + 0];
		result[2 * 3 + 1] = fElements[0 * 3 + 1] * fElements[2 * 3 + 0] - fElements[0 * 3 + 0] * fElements[2 * 3 + 1];

		// third row of cofactors.
		result[0 * 3 + 2] = fElements[0 * 3 + 1] * fElements[1 * 3 + 2] - fElements[0 * 3 + 2] * fElements[1 * 3 + 1];
		result[1 * 3 + 2] = fElements[0 * 3 + 2] * fElements[1 * 3 + 0] - fElements[0 * 3 + 0] * fElements[1 * 3 + 2];
		result[2 * 3 + 2] = fElements[0 * 3 + 0] * fElements[1 * 3 + 1] - fElements[0 * 3 + 1] * fElements[1 * 3 + 0];

		return result;
	}


	VMatrixOf<T>	Inverse () const
	{
		T	determinant = Determinant();
		if (determinant)
			return Adjoint() * ((T)1/determinant);
		else
			return VMatrixOf<T>();
	}

	
	// Operators
	VPointOf<T>		operator * (const VPointOf<T> inVector) const
	{
		GReal	value = fElements[2 * 3 + 0] * inVector.x + fElements[2 * 3 + 1] * inVector.y + fElements[2 * 3 + 2];
		return (value == 0 ? inVector : VPointOf<T>((fElements[0 * 3 + 0] * inVector.x + fElements[0 * 3 + 1] * inVector.y + fElements[0 * 3 + 2]) / value, (fElements[1 * 3 + 0] * inVector.x + fElements[1 * 3 + 1] * inVector.y + fElements[1 * 3 + 2]) / value));
	}
	
	VRect	operator * (const VRect inRect) const
	{
		VPoint p0=inRect.GetTopLeft();
		VPoint p1=inRect.GetBotRight();
		VRect result;
		
		p0 = *this *p0;
		p1 = *this *p1;
		
		result.SetTopLeft(p0);
		result.SetBotRight(p1);
		return result;
	}

	VMatrixOf<T>	operator * (const VMatrixOf<T>& inMatrix) const
	{
		VMatrixOf<T>	result;
		for (sLONG row = 0; row < 3; row++)
		for (sLONG collumn = 0; collumn < 3; collumn++)
			result.fElements[row * 3 + collumn] = fElements[row * 3 + 0] * inMatrix.fElements[0 * 3 + collumn] + fElements[row * 3 + 1] * inMatrix.fElements[1 * 3 + collumn] + fElements[row * 3 + 2] * inMatrix.fElements[2 * 3 + collumn];
		
		result.fTranslationVector = inMatrix.fTranslationVector + fTranslationVector;
		result.fScalingVector = inMatrix.fScalingVector * fScalingVector;					
		result.fRotationAngle = inMatrix.fRotationAngle + fRotationAngle;
		result.fIsIdentity=IsIdentity();
		return result;
	}
	
	
	VMatrixOf<T>	operator * (T inValue) const
	{
		VMatrixOf<T>	result;
		if (inValue != 0.0)
		{
			for (sLONG index = 0; index < 9; index++)
				result.fElements[index] = fElements[index] * inValue;
		}
		return result;
	}
	

	VMatrixOf<T>	operator *= (T inValue)
	{
		*this = operator*(inValue);
		return *this;
	}
	
	
	VMatrixOf<T>	operator / (T inValue) const
	{
		VMatrixOf<T>	result;
		if (inValue != 0.0)
		{
			for (sLONG index = 0; index < 9; index++)
				result.fElements[index] = fElements[index] / inValue;
		}
		return result;
	}
	

	VMatrixOf<T>	operator /= (T inValue)
	{
		*this = operator/(inValue);
		return *this;
	}
	
	
	const T&	operator [] (VIndex index) const
	{
		return fElements[index]; 
	}
	
	
	T&	operator [] (VIndex index)
	{
		return fElements[index]; 
	}
	
	
	VMatrixOf<T>&	operator = (const VMatrixOf<T>& inMatrix)
	{
		_CopyElementsFrom(inMatrix);
		
		fIsIdentity = inMatrix.fIsIdentity;
		fRotationAngle = inMatrix.fRotationAngle;
		fTranslationVector = inMatrix.fTranslationVector;
		fScalingVector = inMatrix.fScalingVector;
		fShearingVector = inMatrix.fShearingVector;
		
		_UpdateMatrix ();
		return *this;
	}


	bool operator == (const VMatrixOf<T>& inMatrix) const
	{
		return	((fElements[0 * 3 + 0] == inMatrix.fElements[0 * 3 + 0]) &&
				 (fElements[0 * 3 + 1] == inMatrix.fElements[0 * 3 + 1]) &&
				 (fElements[0 * 3 + 2] == inMatrix.fElements[0 * 3 + 2]) &&
				
				 (fElements[1 * 3 + 0] == inMatrix.fElements[1 * 3 + 0]) &&
				 (fElements[1 * 3 + 1] == inMatrix.fElements[1 * 3 + 1]) &&
				 (fElements[1 * 3 + 2] == inMatrix.fElements[1 * 3 + 2]) &&
				
				 (fElements[2 * 3 + 0] == inMatrix.fElements[2 * 3 + 0]) &&
				 (fElements[2 * 3 + 1] == inMatrix.fElements[2 * 3 + 1]) &&
				 (fElements[2 * 3 + 2] == inMatrix.fElements[2 * 3 + 2]));
	}


	bool operator != (const VMatrixOf<T>& inMatrix) const
	{
		return	((fElements[0 * 3 + 0] != inMatrix.fElements[0 * 3 + 0]) ||
				 (fElements[0 * 3 + 1] != inMatrix.fElements[0 * 3 + 1]) ||
				 (fElements[0 * 3 + 2] != inMatrix.fElements[0 * 3 + 2]) ||
				
				 (fElements[1 * 3 + 0] != inMatrix.fElements[1 * 3 + 0]) ||
				 (fElements[1 * 3 + 1] != inMatrix.fElements[1 * 3 + 1]) ||
				 (fElements[1 * 3 + 2] != inMatrix.fElements[1 * 3 + 2]) ||
				
				 (fElements[2 * 3 + 0] != inMatrix.fElements[2 * 3 + 0]) ||
				 (fElements[2 * 3 + 1] != inMatrix.fElements[2 * 3 + 1]) ||
				 (fElements[2 * 3 + 2] != inMatrix.fElements[2 * 3 + 2]));
	}


	// Utility accessors
	void SetRotation (GReal rot)
	{
		fRotationAngle = rot;
		_UpdateMatrix();
	}
	
	
	void SetTranslation (const VPointOf<T>& inTranslationVector)
	{
		fTranslationVector = inTranslationVector;
		_UpdateMatrix();
	}
	
	
	void SetScaling (const VPointOf<T>& inScalingVector)
	{
		fScalingVector = inScalingVector;
		_UpdateMatrix();
	}
	
	
	void SetShearing (const VPointOf<T>& inShearingVector)
	{
		fShearingVector = inShearingVector;
		_UpdateMatrix();
	}
	
	GReal	GetRotation () const { return fRotationAngle; };
	VPointOf<T> GetTranslation () const { return fTranslationVector; };
	VPointOf<T> GetScaling () const { return fScalingVector; };
	VPointOf<T> GetShearing () const { return fShearingVector; };
	
	bool	IsTranslatedOnly () const	{ return (IsTranslated () && !IsRotated () && !IsScaled () && !IsSheared ()); }
	bool	IsRotatedOnly () const	{ return (!IsTranslated () && IsRotated () && !IsScaled () && !IsSheared ()); }
	bool	IsScaledOnly () const	{ return (!IsTranslated () && !IsRotated () && IsScaled () && !IsSheared ()); }
	bool	IsSheardOnly () const	{ return (!IsTranslated () && !IsRotated () && !IsScaled () && IsSheared ()); }
	
	bool	IsTranslated () const	{ return (fTranslationVector.x != (T)0 || fTranslationVector.y != (T)0);}
	bool	IsRotated () const	{ return (fRotationAngle != (T)0);}
	bool	IsScaled () const	{ return (fScalingVector.x != (T)1 || fScalingVector.y != (T)1);}
	bool	IsSheared () const	{ return (fShearingVector.x != (T)0 || fShearingVector.y != (T)0);}
	
	// Private utility
	void	_UpdateMatrix ()
	{
		VMatrixOf<T> matrix;
		if (fIsIdentity == false)
			_MakeIdentity();
		
		// TRANSLATION
		if (IsTranslated())
		{
			matrix.fElements[0 * 3 + 0] = 1;
			matrix.fElements[0 * 3 + 1] = 0;
			matrix.fElements[0 * 3 + 2] = fTranslationVector.x;
			matrix.fElements[1 * 3 + 0] = 0;
			matrix.fElements[1 * 3 + 1] = 1;
			matrix.fElements[1 * 3 + 2] = fTranslationVector.y;
			matrix.fElements[2 * 3 + 0] = 0;
			matrix.fElements[2 * 3 + 1] = 0;
			matrix.fElements[2 * 3 + 2] = 1;

			fIsIdentity = false;
			VMatrixOf<T> result = operator*(matrix);
			_CopyElementsFrom(result);
		}

		// ROTATION
		if (IsRotated())
		{
			matrix.fElements[0 * 3 + 0] = (T) + cos(fRotationAngle);
			matrix.fElements[0 * 3 + 1] = (T) - sin(fRotationAngle);
			matrix.fElements[0 * 3 + 2] = 0;
			matrix.fElements[1 * 3 + 0] = (T) + sin(fRotationAngle);
			matrix.fElements[1 * 3 + 1] = (T) + cos(fRotationAngle);
			matrix.fElements[1 * 3 + 2] = 0;
			matrix.fElements[2 * 3 + 0] = 0;
			matrix.fElements[2 * 3 + 1] = 0;
			matrix.fElements[2 * 3 + 2] = 1;

			fIsIdentity = false;
			VMatrixOf<T>	result = operator*(matrix);
			_CopyElementsFrom(result);
		}
		
		// SCALING
		if (IsScaled())
		{
			matrix.fElements[0 * 3 + 0] = fScalingVector.x;
			matrix.fElements[0 * 3 + 1] = 0;
			matrix.fElements[0 * 3 + 2] = 0;
			matrix.fElements[1 * 3 + 0] = 0;
			matrix.fElements[1 * 3 + 1] = fScalingVector.y;
			matrix.fElements[1 * 3 + 2] = 0;
			matrix.fElements[2 * 3 + 0] = 0;
			matrix.fElements[2 * 3 + 1] = 0;
			matrix.fElements[2 * 3 + 2] = 1;

			fIsIdentity = false;
			VMatrixOf<T>	result = operator*(matrix);
			_CopyElementsFrom(result);
		}
		
		// SHEARING
		if (IsSheared())
		{
			matrix.fElements[0 * 3 + 0] = 1;
			matrix.fElements[0 * 3 + 1] = (T)tan(fShearingVector.x);
			matrix.fElements[0 * 3 + 2] = 0;
			matrix.fElements[1 * 3 + 0] = (T)tan(fShearingVector.y);
			matrix.fElements[1 * 3 + 1] = 1;
			matrix.fElements[1 * 3 + 2] = 0;
			matrix.fElements[2 * 3 + 0] = 0;
			matrix.fElements[2 * 3 + 1] = 0;
			matrix.fElements[2 * 3 + 2] = 1;

			fIsIdentity = false;
			VMatrixOf<T>	result = operator*(matrix);
			_CopyElementsFrom(result);
		}
	}
	
	
	void	_CopyElementsFrom (const VMatrixOf<T>& inOriginal)
	{
		for (sLONG index = 0; index < 9; index++)
			fElements[index] = inOriginal.fElements[index];
	}
	void _MakeIdentity ()
	{
		MakeEmpty();
		
		for (sLONG index = 0; index < 3; index++)
			fElements[index * 3 + index] = 1;
		fIsIdentity = true;
	}
protected:
	T			fElements[3 * 3];	// Matrix elements
	bool		fIsIdentity;
	GReal		fRotationAngle;		// Current transformation parameters
	VPointOf<T>	fTranslationVector;
	VPointOf<T>	fScalingVector;
	VPointOf<T>	fShearingVector;
};


typedef VMatrixOf<GReal> VMatrix;


END_TOOLBOX_NAMESPACE

#endif
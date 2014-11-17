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
#ifndef __VFloat__
#define __VFloat__

#include "Kernel/Sources/VValueSingle.h"

// Class definitions
struct	M_APM_struct;

BEGIN_TOOLBOX_NAMESPACE

// Defined bellow
class VFloat;


class XTOOLBOX_API VFloat_info : public VValueInfo_default<VFloat, VK_FLOAT>
{
public:
    VFloat_info()					{};
	virtual	VValue*					LoadFromPtr( const void *inBackStore, bool inRefOnly) const;
	virtual void*					ByteSwapPtr( void *inBackStore, bool inFromNative) const;

	virtual CompareResult			CompareTwoPtrToData( const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean inDiacritical = false) const;
	virtual CompareResult			CompareTwoPtrToDataBegining( const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean inDiacritical = false) const;
	virtual	CompareResult			CompareTwoPtrToDataWithOptions( const void* inPtrToValueData1, const void* inPtrToValueData2, const VCompareOptions& inOptions) const;

	virtual VSize					GetSizeOfValueDataPtr( const void* inPtrToValueData) const;
	virtual	VSize					GetAvgSpace() const;
};


class XTOOLBOX_API VFloat : public VValueSingle
{
public:
	typedef VFloat_info	TypeInfo;
	static const TypeInfo	sInfo;

			VFloat ();
			VFloat (uBYTE* inDataPtr, Boolean inInit);
			VFloat (const VFloat& inOriginal);
			VFloat (Real inValue);
			VFloat (sLONG inValue);
			VFloat (sLONG8 inValue);
	virtual	~VFloat ();
		
	// Arithmetic support
	void	Add (VFloat& outResult, const VFloat& inValue);
	void	Sub (VFloat& outResult, const VFloat& inValue);
	void	Divide (VFloat& outResult, const VFloat& inValue);
	void	Multiply (VFloat& outResult, const VFloat& inValue);

	void	Abs (VFloat& outFloat) const;
	void	Negate (VFloat& outFloat) const;
	
	sWORD	Sign () const;
	sLONG	Exponent () const;
	sLONG	SignificantDigits () const;
	Boolean	IsInteger () const;
	
	// Trigonometric support
	void	Sqrt (VFloat& outResult);
	void	Cbrt (VFloat& outResult);
	void	Log (VFloat& outResult);
	void	Exp (VFloat& outResult);
	void	Log10 (VFloat& outResult);
	void	Sin (VFloat& outResult);
	void	Asin (VFloat& outResult);
	void	Cos (VFloat& outResult);
	void	Acos (VFloat& outResult);
	void	Tan (VFloat& outResult);
	void	Atan (VFloat& outResult);
	void	Sinh (VFloat& outResult);
	void	Asinh (VFloat& outResult);
	void	Cosh (VFloat& outResult);
	void	Acosh (VFloat& outResult);
	void	Tanh (VFloat& outResult);
	void	Atanh (VFloat& outResult);
	void	Pow (VFloat& outResult, const VFloat& inPower) const;

	// Rounding support
	void	Floor (VFloat& outResult) const;
	void	Ceil (VFloat& outResult) const;
	
	void	Round (VFloat& outFloat, sLONG inToDigits) const;

	void	IntegerDivide (VFloat& outResult, const VFloat &inDenom) const;
	void	IntegerDivRemainder (const VFloat &outResult, const VFloat &inDenom) const;

	// Operators
	VFloat&	operator = (const VFloat &inValue);
	VFloat&	operator = (Real inValue);
	VFloat&	operator = (sLONG inValue);
	
	bool operator == (const VFloat &inValue) const;
	bool operator != (const VFloat &inValue) const;
	bool operator < (const VFloat &inValue) const;
	bool operator <= (const VFloat &inValue) const;
	bool operator > (const VFloat &inValue) const;
	bool operator >= (const VFloat &inValue) const;
	
	// Inherited from VValueSingle
	virtual	const VValueInfo*	GetValueInfo() const;
	virtual Boolean	CanBeEvaluated () const;

	virtual Boolean	GetBoolean () const;
	virtual sWORD	GetWord () const;
	virtual sLONG	GetLong () const;
	virtual sLONG8	GetLong8 () const;
	virtual Real	GetReal () const;
	virtual void	GetFloat (VFloat& outValue) const;
	virtual void	GetDuration (VDuration& outValue) const;
	virtual void	GetString (VString& outValue) const;
	virtual void	GetValue (VValueSingle& outValue) const;

	virtual void	FromBoolean (Boolean inValue); 
	virtual void	FromWord (sWORD inValue);
	virtual void	FromLong (sLONG inValue);
	virtual void	FromLong8 (sLONG8 inValue);
	virtual void	FromReal (Real inValue);
	virtual void	FromFloat (const VFloat& inValue);
	virtual void	FromDuration (const VDuration& inValue);
	virtual void	FromString (const VString& inValue);
	virtual void	FromValue (const VValueSingle& inValue);

	virtual void*	LoadFromPtr (const void* inDataPtr, Boolean inRefOnly = false);
	virtual void*	WriteToPtr (void* inDataPtr, Boolean inRefOnly = false, VSize inMax = 0) const;
	virtual VSize	GetSpace (VSize inMax = 0) const;

	virtual VError	ReadFromStream (VStream* inStream, sLONG inParam = 0);
	virtual VError	WriteToStream (VStream* inStream, sLONG inParam = 0) const;
	
	virtual VFloat*	Clone () const;
	virtual void	Clear ();
	
	virtual CompareResult	CompareTo (const VValueSingle& inValue, Boolean inDiacritical = false) const;
	virtual CompareResult	CompareToSameKind (const VValue* inValue, Boolean inDiacritical =false) const;
	virtual Boolean	EqualToSameKind (const VValue* inValue, Boolean inDiacritical = false) const;
	virtual Boolean	FromValueSameKind (const VValue* inValue);

	virtual CompareResult	CompareToSameKindPtr (const void* inPtrToValueData, Boolean inDiacritical = false) const;
	virtual Boolean	EqualToSameKindPtr (const void* inPtrToValueData, Boolean inDiacritical = false) const;

	virtual	CompareResult		CompareToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const;
	virtual	bool				EqualToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const;
	virtual	CompareResult		CompareToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const;
	virtual bool				EqualToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const;

	// Utilities
	static void	Random (VFloat& outResult);
	static void	Pi (VFloat& outResult);

protected:
	::M_APM_struct*	fValue;
};

END_TOOLBOX_NAMESPACE

#endif

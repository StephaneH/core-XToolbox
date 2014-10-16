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
#include "VKernelPrecompiled.h"
#include "VFloat.h"
#include "VString.h"
#include "VTime.h"
#include "VStream.h"
#include "VMemoryCpp.h"
#include "M_APM/m_apm.h"

#if COMPIL_VISUAL
inline int finite( double a) { return _finite( a);}
#endif


// This is the default number of digits to use for 1-ary functions like sin, cos, tan, etc.
//	It's the larger of my digits and cpp_min_precision.
inline sLONG _digits(M_APM inValue)
{
	return Max(m_apm_significant_digits(inValue), 30);
}


// This is the default number of digits to use for 2-ary functions like divide, atan2, etc.
//	It's the larger of inValue, otherVal, and cpp_min_precision.
inline sLONG _digits(M_APM inValue, const VFloat& otherVal)
{
	return Max(otherVal.SignificantDigits(), _digits(inValue)); 
}


const VFloat::TypeInfo	VFloat::sInfo;

VValue *VFloat_info::LoadFromPtr( const void *inBackStore, bool /*inRefOnly*/) const
{
	return new VFloat( (uBYTE*) inBackStore, false);
}


void* VFloat_info::ByteSwapPtr( void *inBackStore, bool inFromNative) const
{
	uBYTE* ptr = (uBYTE*) inBackStore;

	ByteSwap( (sLONG*)ptr);
	ptr += sizeof(sLONG);

	ptr += sizeof(sBYTE);
	
	sLONG len;
	if (inFromNative)
		len = *(sLONG*)ptr;
	ByteSwap( (sLONG*)ptr);
	if (!inFromNative)
		len = *(sLONG*)ptr;
	ptr += sizeof(sLONG) + len;
	return ptr;
}

static void makeTmp_M_APM_FromPtr(M_APM_struct& tmp, const void* p)
{
	uBYTE* ptr = (uBYTE*)p;
	tmp.m_apm_exponent = *(sLONG*)ptr;
	ptr += sizeof(sLONG);

	tmp.m_apm_sign = *(sBYTE*) ptr;
	ptr += sizeof(sBYTE);

	tmp.m_apm_datalength = *(sLONG*) ptr;
	ptr += sizeof(sLONG);

	tmp.m_apm_data = ptr;
}


CompareResult VFloat_info::CompareTwoPtrToData(const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean /*inDiacritical*/) const
{
	M_APM_struct tmp1, tmp2;
	makeTmp_M_APM_FromPtr(tmp1, inPtrToValueData1);
	makeTmp_M_APM_FromPtr(tmp2, inPtrToValueData2);
	int res = m_apm_compare(&tmp1, &tmp2);
	if (res > 0)
		return CR_BIGGER;
	else
	{
		if (res < 0)
			return CR_SMALLER;
		else
			return CR_EQUAL;
	}
}


CompareResult VFloat_info::CompareTwoPtrToDataBegining(const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean inDiacritical) const
{
	return CompareTwoPtrToData(inPtrToValueData1, inPtrToValueData2, inDiacritical);
}


CompareResult VFloat_info::CompareTwoPtrToDataWithOptions( const void* inPtrToValueData1, const void* inPtrToValueData2, const VCompareOptions& inOptions) const
{
	M_APM_struct tmp1, tmp2;
	makeTmp_M_APM_FromPtr(tmp1, inPtrToValueData1);
	makeTmp_M_APM_FromPtr(tmp2, inPtrToValueData2);
	int res = m_apm_compare(&tmp1, &tmp2);
	if (res > 0)
		return CR_BIGGER;
	else
	{
		if (res < 0)
			return CR_SMALLER;
		else
			return CR_EQUAL;
	}
}


VSize VFloat_info::GetSizeOfValueDataPtr(const void* inPtrToValueData) const
{
	M_APM_struct tmp;
	makeTmp_M_APM_FromPtr(tmp, inPtrToValueData);
	return sizeof(sLONG)+sizeof(sLONG)+sizeof(sBYTE)+tmp.m_apm_datalength;
}


VSize VFloat_info::GetAvgSpace() const
{
	return sizeof(sLONG) + sizeof(sBYTE) + sizeof(sLONG) + 16;
}



//=======================================================================================================================================


VFloat::VFloat()
{
	fValue = m_apm_init();
}


VFloat::VFloat(uBYTE* inDataPtr, Boolean /*inInit*/)
{
	fValue = m_apm_init();
	LoadFromPtr(inDataPtr);
}


VFloat::VFloat(const VFloat& inOriginal)
{
	fValue = m_apm_init();
	m_apm_copy(fValue, inOriginal.fValue);
}


VFloat::VFloat(Real inValue):VValueSingle( finite( inValue) == 0)
{
	fValue = m_apm_init();
	if (!IsNull())
		m_apm_set_double(fValue, inValue);
}


VFloat::VFloat(sLONG inValue)
{
	fValue = m_apm_init();
	m_apm_set_long(fValue, inValue);
}


VFloat::VFloat(sLONG8 inValue)
{
	fValue = m_apm_init();
	VStr<32> str;
	str.FromLong8(inValue);
	FromString(str);
}


VFloat::~VFloat()
{
	m_apm_free(fValue);
}


VFloat* VFloat::Clone() const 
{
	return new VFloat(*this);
}


Boolean VFloat::CanBeEvaluated() const
{
	return true;
}


const VValueInfo *VFloat::GetValueInfo() const
{
	return &sInfo;
}


CompareResult VFloat::CompareTo (const VValueSingle& inValue, Boolean /*inDiacritical*/) const
{
	VFloat val;
	inValue.GetFloat(val);

	int result = m_apm_compare(fValue, val.fValue) ;
		 
	if (result > 0)
		return CR_BIGGER;
	else if (result < 0)
		return CR_SMALLER;
	else
		return CR_EQUAL;
}


void VFloat::Clear()
{
	m_apm_set_long(fValue, 0);
}


Boolean VFloat::GetBoolean() const
{
	return (0 != m_apm_compare(fValue, MM_Zero));
}


sWORD VFloat::GetWord() const
{
	return (sWORD) GetLong();
}


sLONG VFloat::GetLong() const
{
	char str[1024];
	m_apm_to_integer_string(str, fValue);
	return (sLONG) atol(str);
}


sLONG8 VFloat::GetLong8() const
{
	char str[1024];
	m_apm_to_integer_string(str, fValue);
	
	VString vstr;
	vstr.FromCString(str);
	return vstr.GetLong8();
}


Real VFloat::GetReal() const
{
	char str[1024];
	m_apm_to_string(str, -1, fValue);
	return atof(str);
}


void VFloat::GetFloat(VFloat& outValue) const
{
	outValue.SetNull(IsNull());
	m_apm_copy(outValue.fValue, fValue);
}


void VFloat::GetDuration(VDuration& outValue) const
{
	if (IsNull())
		outValue.SetNull(true);
	else
		outValue.FromNbMilliseconds(GetLong8());
}


void VFloat::GetString(VString& outValue) const
{
	if (IsNull())
	{
		outValue.SetNull(true);
	}
	else
	{
		char str[10000];
		m_apm_to_fixpt_string(str, -1, fValue);
		//m_apm_to_string(str, -1, fValue);
		outValue.FromCString(str);
	}
}


void VFloat::GetValue(VValueSingle& outValue) const
{
	outValue.FromFloat(*this);
}


void VFloat::FromBoolean(Boolean inValue)
{
	SetNull(false);
	if (inValue)
		m_apm_set_long(fValue, 1);
	else
		m_apm_set_long(fValue, 0);
}


void VFloat::FromWord(sWORD inValue)
{
	SetNull(false);
	m_apm_set_long(fValue, (sLONG) inValue);
}


void VFloat::FromLong(sLONG inValue)
{
	SetNull(false);
	m_apm_set_long(fValue, inValue);
}


void VFloat::FromLong8(sLONG8 inValue)
{
	VString str;
	str.FromLong8(inValue);
	FromString(str);
}


void VFloat::FromReal(Real inValue)
{
	if (finite( inValue))	// mapm crash on inf on mac.
	{
		m_apm_set_double(fValue, inValue);
		GotValue();
	}
	else
	{
		SetNull( true);
	}
}


void VFloat::FromFloat(const VFloat& inValue)
{
	SetNull(inValue.IsNull());
	m_apm_copy(fValue, inValue.fValue);
}


void VFloat::FromDuration(const VDuration& inValue)
{
	FromLong8(inValue.ConvertToMilliseconds());
}


void VFloat::FromString(const VString& inValue)
{
	if (inValue.IsNull())
	{
		SetNull(true);
	}
	else
	{
		SetNull(false);
		VStringConvertBuffer convert(inValue, VTC_StdLib_char);
		m_apm_set_string(fValue, (char*) convert.GetCPointer());
	}
}


void VFloat::FromValue(const VValueSingle& inValue)
{
	inValue.GetFloat(*this);
}


VSize VFloat::GetSpace(VSize /* inMax */) const
{
	return sizeof(sLONG) + sizeof(sBYTE) + sizeof(sLONG) + fValue->m_apm_datalength;
}


void* VFloat::LoadFromPtr(const void* inDataPtr, Boolean /*inRefOnly*/)
{
	M_APM_struct tmp;
	uBYTE* ptr = (uBYTE*) inDataPtr;

	tmp.m_apm_exponent = *(sLONG*)ptr;
	ptr += sizeof(sLONG);

	tmp.m_apm_sign = *(sBYTE*) ptr;
	ptr += sizeof(sBYTE);
	
	tmp.m_apm_datalength = *(sLONG*) ptr;
	ptr += sizeof(sLONG);

	tmp.m_apm_data = ptr;
	m_apm_copy(fValue, &tmp);

	return ptr + tmp.m_apm_datalength;
}


void* VFloat::WriteToPtr(void* inDataPtr, Boolean /*inRefOnly*/, VSize inMax) const
{
	uBYTE*	ptr = (uBYTE*) inDataPtr;

	*(sLONG*) ptr = (sLONG) fValue->m_apm_exponent;
	ptr += sizeof(sLONG);

	*(sBYTE*) ptr = fValue->m_apm_sign;
	ptr += sizeof(sBYTE);
	
	*(sLONG*) ptr = (sLONG) fValue->m_apm_datalength;
	ptr += sizeof(sLONG);
	
	VMemory::CopyBlock(fValue->m_apm_data, ptr, fValue->m_apm_datalength);

	return ptr + fValue->m_apm_datalength;
}


VError VFloat::ReadFromStream(VStream* inStream, sLONG /*inParam*/)
{
	M_APM_struct tmp;

	tmp.m_apm_exponent   = inStream->GetLong();
	tmp.m_apm_sign       = inStream->GetByte();
	tmp.m_apm_datalength = inStream->GetLong();
	
	tmp.m_apm_data = (uBYTE*) vMalloc(tmp.m_apm_datalength, 'mapm');
	
	inStream->GetData( tmp.m_apm_data, tmp.m_apm_datalength);
	m_apm_copy(fValue, &tmp);
	
	vFree(tmp.m_apm_data);

	return inStream->GetLastError();
}


VError VFloat::WriteToStream(VStream* inStream, sLONG inParam) const
{
	VValue::WriteToStream(inStream, inParam);
	
	inStream->PutLong((sLONG) fValue->m_apm_exponent);
	inStream->PutByte(fValue->m_apm_sign);
	inStream->PutLong((sLONG) fValue->m_apm_datalength);
	inStream->PutData(fValue->m_apm_data, fValue->m_apm_datalength);

	return inStream->GetLastError();
}


CompareResult VFloat::CompareToSameKindPtr(const void* inPtrToValueData, Boolean /*inDiacritical*/) const
{
	M_APM_struct tmp;
	makeTmp_M_APM_FromPtr(tmp, inPtrToValueData);
	int res = m_apm_compare(fValue, &tmp);
	if (res > 0)
		return CR_BIGGER;
	else
	{
		if (res < 0)
			return CR_SMALLER;
		else
			return CR_EQUAL;
	}
}


Boolean VFloat::EqualToSameKindPtr(const void* inPtrToValueData, Boolean /*inDiacritical*/) const
{
	M_APM_struct tmp;
	makeTmp_M_APM_FromPtr(tmp, inPtrToValueData);
	return m_apm_compare(fValue, &tmp) == 0;
}


CompareResult VFloat::CompareToSameKind(const VValue* inValue, Boolean /*inDiacritical*/) const
{
	XBOX_ASSERT_KINDOF(VFloat, inValue);
	const VFloat* theValue = reinterpret_cast<const VFloat*>(inValue);

	int result = m_apm_compare(fValue, theValue->fValue) ;
		 
	if (result > 0)
		return CR_BIGGER;
	else if (result < 0)
		return CR_SMALLER;
	else
		return CR_EQUAL;
}


Boolean VFloat::EqualToSameKind(const VValue* inValue, Boolean /*inDiacritical*/) const
{
	XBOX_ASSERT_KINDOF(VFloat, inValue);
	const VFloat* theValue = reinterpret_cast<const VFloat*>(inValue);

	return 0 == m_apm_compare(fValue, theValue->fValue);
}


CompareResult VFloat::CompareToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const
{
	XBOX_ASSERT_KINDOF(VFloat, inValue);
	const VFloat* theValue = reinterpret_cast<const VFloat*>(inValue);

	int result = m_apm_compare(fValue, theValue->fValue) ;
		 
	if (result > 0)
		return CR_BIGGER;
	else if (result < 0)
		return CR_SMALLER;
	else
		return CR_EQUAL;
}


bool VFloat::EqualToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const
{
	XBOX_ASSERT_KINDOF(VFloat, inValue);
	const VFloat* theValue = reinterpret_cast<const VFloat*>(inValue);

	return 0 == m_apm_compare(fValue, theValue->fValue);
}


CompareResult VFloat::CompareToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const
{
	M_APM_struct tmp;
	makeTmp_M_APM_FromPtr(tmp, inPtrToValueData);
	int res = m_apm_compare(fValue, &tmp);
	if (res > 0)
		return CR_BIGGER;
	else
	{
		if (res < 0)
			return CR_SMALLER;
		else
			return CR_EQUAL;
	}
}


bool VFloat::EqualToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const
{
	M_APM_struct tmp;
	makeTmp_M_APM_FromPtr(tmp, inPtrToValueData);
	return m_apm_compare(fValue, &tmp) == 0;
}


Boolean VFloat::FromValueSameKind(const VValue* inValue)
{
	XBOX_ASSERT_KINDOF(VFloat, inValue);
	const VFloat* theValue = reinterpret_cast<const VFloat*>(inValue);

	m_apm_copy(fValue, theValue->fValue);
	GotValue(theValue->IsNull());
	return true;
}


VFloat& VFloat::operator=(const VFloat &inValue)
{
	m_apm_copy(fValue, inValue.fValue);
	GotValue(inValue.IsNull());
	return *this;
}


VFloat& VFloat::operator=(Real inValue)
{
	if (finite( inValue))	// mapm crash on inf on mac.
	{
		m_apm_set_double(fValue, inValue);
		GotValue();
	}
	else
	{
		SetNull( true);
	}
	return *this;
}


VFloat& VFloat::operator=(sLONG inValue)
{
	m_apm_set_long(fValue, inValue);
	GotValue();
	return *this;
}


bool VFloat::operator == (const VFloat &inValue) const
{
	return m_apm_compare(fValue, inValue.fValue) == 0;
}


bool VFloat::operator != (const VFloat &inValue) const
{
	return m_apm_compare(fValue, inValue.fValue) != 0;
}


bool VFloat::operator < (const VFloat &inValue) const
{
	return m_apm_compare(fValue, inValue.fValue) < 0;
}


bool VFloat::operator <= (const VFloat &inValue) const
{
	return m_apm_compare(fValue, inValue.fValue) <= 0;
}


bool VFloat::operator > (const VFloat &inValue) const
{
	return m_apm_compare(fValue, inValue.fValue) > 0;
}


bool VFloat::operator >= (const VFloat &inValue) const
{
	return m_apm_compare(fValue, inValue.fValue) >= 0;
}


void VFloat::Add(VFloat& outResult, const VFloat& inValue)
{
	m_apm_add(outResult.fValue, fValue, inValue.fValue);
}


void VFloat::Sub(VFloat& outResult, const VFloat& inValue)
{
	m_apm_subtract(outResult.fValue, fValue, inValue.fValue);
}


void VFloat::Divide(VFloat& outResult, const VFloat& inValue)
{
	m_apm_divide(outResult.fValue, _digits(fValue), fValue, inValue.fValue);
}


void VFloat::Multiply(VFloat& outResult, const VFloat& inValue)
{
	m_apm_multiply(outResult.fValue, fValue, inValue.fValue);
}


void VFloat::Pi(VFloat& outResult)
{
	m_apm_copy(outResult.fValue, MM_PI);
}


sWORD VFloat::Sign() const
{
	return (sWORD) m_apm_sign(fValue);
}


sLONG VFloat::Exponent() const 
{
	return m_apm_exponent(fValue);
}


sLONG VFloat::SignificantDigits() const 
{
	return m_apm_significant_digits(fValue);
}


Boolean VFloat::IsInteger() const 
{
	return 0 != m_apm_is_integer(fValue);
}


void VFloat::Abs(VFloat& outFloat) const
{
	m_apm_absolute_value(outFloat.fValue, fValue);
}


void VFloat::Negate(VFloat& outFloat) const
{
	m_apm_negate(outFloat.fValue, fValue);
}


void VFloat::Round(VFloat& outFloat, sLONG inToDigits) const
{
	m_apm_round(outFloat.fValue, inToDigits, fValue);
}


void VFloat::Sqrt(VFloat& outResult)
{
	m_apm_sqrt(outResult.fValue, _digits(fValue), fValue);
}


void VFloat::Cbrt(VFloat& outResult)
{
	m_apm_cbrt(outResult.fValue, _digits(fValue), fValue);
}


void VFloat::Log(VFloat& outResult)
{
	m_apm_log(outResult.fValue, _digits(fValue), fValue);
}


void VFloat::Exp(VFloat& outResult)
{
	m_apm_exp(outResult.fValue, _digits(fValue), fValue);
}


void VFloat::Log10(VFloat& outResult)
{
	m_apm_log10(outResult.fValue, _digits(fValue), fValue);
}


void VFloat::Sin(VFloat& outResult)
{
	m_apm_sin(outResult.fValue, _digits(fValue), fValue);
}


void VFloat::Asin(VFloat& outResult)
{
	m_apm_asin(outResult.fValue, _digits(fValue), fValue);
}


void VFloat::Cos(VFloat& outResult)
{
	m_apm_cos(outResult.fValue, _digits(fValue), fValue);
}


void VFloat::Acos(VFloat& outResult)
{
	m_apm_acos(outResult.fValue, _digits(fValue), fValue);
}


void VFloat::Tan(VFloat& outResult)
{
	m_apm_tan(outResult.fValue, _digits(fValue), fValue);
}


void VFloat::Atan(VFloat& outResult)
{
	m_apm_atan(outResult.fValue, _digits(fValue), fValue);
}


void VFloat::Sinh(VFloat& outResult)
{
	m_apm_sinh(outResult.fValue, _digits(fValue), fValue);
}


void VFloat::Asinh(VFloat& outResult)
{
	m_apm_asinh(outResult.fValue, _digits(fValue), fValue);
}


void VFloat::Cosh(VFloat& outResult)
{
	m_apm_cosh(outResult.fValue, _digits(fValue), fValue);
}


void VFloat::Acosh(VFloat& outResult)
{
	m_apm_acosh(outResult.fValue, _digits(fValue), fValue);
}


void VFloat::Tanh(VFloat& outResult)
{
	m_apm_tanh(outResult.fValue, _digits(fValue), fValue);
}


void VFloat::Atanh(VFloat& outResult)
{
	m_apm_atanh(outResult.fValue, _digits(fValue), fValue);
}


void VFloat::Pow(VFloat& outValue, const VFloat &inPower) const
{
	m_apm_pow(outValue.fValue, _digits(fValue, inPower), fValue, inPower.fValue);
}


void VFloat::Random(VFloat& outResult) 
{
	m_apm_get_random(outResult.fValue);
}


void VFloat::Floor(VFloat& outResult) const
{
	m_apm_floor(outResult.fValue, fValue);
}


void VFloat::Ceil(VFloat& outResult) const
{
	m_apm_ceil(outResult.fValue, fValue);
}


void VFloat::IntegerDivide(VFloat& outResult, const VFloat &inDenom) const
{
	m_apm_integer_divide(outResult.fValue, fValue, inDenom.fValue);
}


void VFloat::IntegerDivRemainder(const VFloat &outResult, const VFloat &inDenom) const
{
	VFloat q;
	m_apm_integer_div_rem(q.fValue, outResult.fValue, fValue, inDenom.fValue);
}

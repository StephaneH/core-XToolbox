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

#include <cmath>

#include "VValueSingle.h"
#include "VProcess.h"
#include "VMemoryCpp.h"
#include "VStream.h"
#include "VString.h"
#include "VFloat.h"
#include "VTime.h"
#include "VUUID.h"
#include "VJSONValue.h"


#if COMPIL_VISUAL
inline int isnan( double a) { return _isnan( a);}
#elif COMPIL_GCC
inline int isnan( double a) { return std::isnan( a);}
#endif

template<class T>
inline void CastToInteger( double inValue, T *outValue)
{
	double r = floor( inValue);
	T v = (T) r;
	*outValue = ( (double) v == r) ? v : 0;
}


const VEmpty::InfoType		VEmpty::sInfo;
const VWord::InfoType		VWord::sInfo;
const VLong::InfoType		VLong::sInfo;
const VLong8::InfoType		VLong8::sInfo;
const VReal::InfoType		VReal::sInfo;
const VBoolean::InfoType	VBoolean::sInfo;
const VByte::InfoType		VByte::sInfo;


//================================================================================================


template<class VALUE_INFO, class SCALAR_TYPE>
inline CompareResult CompareToSameKindWithOptions_scalar( const VValue* inValue, SCALAR_TYPE inScalar)
{
	XBOX_ASSERT_KINDOF( typename VALUE_INFO::vvalue_class, inValue);
	const typename VALUE_INFO::vvalue_class*	theValue = reinterpret_cast<const typename VALUE_INFO::vvalue_class*>(inValue);
	
	if (inScalar == theValue->Get())
		return CR_EQUAL;
	else if (inScalar > theValue->Get())
		return CR_BIGGER;
	else
		return CR_SMALLER;
}


template<class VALUE_INFO, class SCALAR_TYPE>
inline bool EqualToSameKindWithOptions_scalar( const VValue* inValue, SCALAR_TYPE inScalar)
{
	XBOX_ASSERT_KINDOF(typename VALUE_INFO::vvalue_class, inValue);
	const typename VALUE_INFO::vvalue_class* theValue = reinterpret_cast<const typename VALUE_INFO::vvalue_class*>(inValue);
	return (inScalar == theValue->Get());
}


template<class VALUE_INFO, class SCALAR_TYPE>
inline CompareResult CompareToSameKindPtrWithOptions_scalar( const void* inPtrToValueData, SCALAR_TYPE inScalar)
{
	if (inScalar == *((typename VALUE_INFO::backstore_type*)inPtrToValueData))
		return CR_EQUAL;
	else if (inScalar > *((typename VALUE_INFO::backstore_type*)inPtrToValueData))
		return CR_BIGGER;
	else
		return CR_SMALLER;
}


template<class VALUE_INFO, class SCALAR_TYPE>
inline bool EqualToSameKindPtrWithOptions_scalar( const void* inPtrToValueData, SCALAR_TYPE inScalar)
{
	return inScalar == *((typename VALUE_INFO::backstore_type*)inPtrToValueData);
}


//================================================================================================

#if VERSIONDEBUG
VValueSingle::~VValueSingle()
{
	sLONG xxxdebug = 1; // break here
}
#endif


void VValueSingle::GetFloat(VFloat& outValue) const
{
	outValue.Clear();
}


void VValueSingle::GetDuration(VDuration& outValue) const
{
	outValue.Clear();
}


void VValueSingle::GetTime(VTime& outValue) const
{
	outValue.Clear();
}


void VValueSingle::GetString(VString& outValue) const
{
	outValue.Clear();
}


void VValueSingle::GetVUUID(VUUID& outValue) const
{
	outValue.Clear();
}


void VValueSingle::GetValue(VValueSingle& outValue) const
{
	outValue.Clear();
}


VValueSingle* VValueSingle::ConvertTo(sLONG inWhatType) const
{
	VValueSingle*	result = NULL;

	//if (inWhatType < kMAX_PREDEFINED_VALUE_TYPES && GetValueKind() < kMAX_PREDEFINED_VALUE_TYPES)
	{
		VValue* value = NewValueFromValueKind((ValueKind)inWhatType);
		if (value != NULL)
		{
			result = value->IsVValueSingle();
			if (result != NULL)
			{
				GetValue(*result);
				result->SetNull(IsNull());
			}
			else
				delete value;
		}
	}
	/*
	else
	{
		// We must one day manage here a tree of promotor to convert user added types
		assert(!(inWhatType < kMAX_PREDEFINED_VALUE_TYPES && GetValueKind() < kMAX_PREDEFINED_VALUE_TYPES));
	}
	*/

	return result;
}


Boolean VValueSingle::EqualTo (const VValueSingle& inValue, Boolean inDiacritical ) const
{
	return CompareTo(inValue, inDiacritical) == CR_EQUAL;
}


CharSet VValueSingle::DebugDump(char* inTextBuffer, sLONG& inBufferSize) const
{
	if (VProcess::Get() != NULL)
	{
		VString	dump;
		GetString(dump);
		dump.Truncate(inBufferSize / 2);
		inBufferSize = (sLONG) dump.ToBlock(inTextBuffer, inBufferSize, VTC_UTF_16, false, false);
	}
	else
	{
		inBufferSize = 0;
	}

	return VTC_UTF_16;
}


void VValueSingle::GetXMLString( VString& outString, XMLStringOptions /*inOptions*/) const
{
	GetString( outString);
}


bool VValueSingle::FromXMLString( const VString& inString)
{
	FromString( inString);
	return true;
}


VError VValueSingle::FromJSONString( const VString& inJSONString, JSONOption /*inModifier*/)
{
	FromString(inJSONString);
	return VE_OK;
}


VError VValueSingle::GetJSONString( VString& outJSONString, JSONOption /*inModifier*/) const
{
	GetString(outJSONString);
	return VE_OK;
}


VError VValueSingle::FromJSONValue(const VJSONValue& /*inJSONValue*/)
{
	Clear();
	return VE_UNIMPLEMENTED;
}

VError VValueSingle::GetJSONValue(VJSONValue& outJSONValue, bool /*allowDates*/) const
{
	outJSONValue.SetNull();
	return VE_UNIMPLEMENTED;
}


#pragma mark-
VEmpty::VEmpty()
 : VValueSingle(true)
{
}

VEmpty::~VEmpty()
{
}

Boolean VEmpty::CanBeEvaluated() const
{
	return false;
}

VEmpty* VEmpty::Clone() const
{
	return new VEmpty();
}

CompareResult VEmpty::CompareTo( const VValueSingle& /*inValue*/, Boolean inDiacritic) const
{
	return inDiacritic ? CR_UNRELEVANT : CR_UNRELEVANT;
}


const VValueInfo *VEmpty::GetValueInfo() const
{
	return &sInfo;
}


VValue *VEmpty_info::Generate() const
{
	return new VEmpty;
}

VValue *VEmpty_info::Generate(VBlob* /*inBlob*/) const
{
	return new VEmpty;
}


VValue *VEmpty_info::LoadFromPtr( const void* /*ioBackStore*/, bool /*inRefOnly*/) const
{
	return new VEmpty;
}


void* VEmpty_info::ByteSwapPtr( void *inBackStore, bool /*inFromNative*/) const
{
	return inBackStore;
}

#pragma mark-


VByte::VByte(sBYTE* inDataPtr, Boolean inInit)
{
	if (inInit)
		Clear();
	else
		fValue = *inDataPtr;
}


void VByte::Clear()
{
	fValue = 0;
	GotValue();
}


CompareResult VByte::CompareTo (const VValueSingle& inValue, Boolean /*inDiacritic*/) const
{
	sLONG	value = inValue.GetLong();
	if (fValue == value)
		return CR_EQUAL;
	else if (fValue > value)
		return CR_BIGGER;
	else
		return CR_SMALLER;
	
}


void VByte::GetFloat(VFloat& outValue) const
{
	outValue.FromWord(fValue);
}


void VByte::GetDuration(VDuration& outValue) const
{
	outValue.FromWord(fValue);
}


void VByte::GetString(VString& outValue) const
{
	outValue.FromWord(fValue);
}


void VByte::GetValue(VValueSingle& outValue) const
{
	outValue.FromWord(fValue);
}


void VByte::FromBoolean(Boolean inValue)
{
	fValue = inValue;
	GotValue();
} 


void VByte::FromByte(sBYTE inValue)
{
	fValue = inValue;
	GotValue();
}


void VByte::FromWord(sWORD inValue)
{
	fValue = sBYTE(inValue);
	GotValue();
}


void VByte::FromLong(sLONG inValue)
{
	fValue = sBYTE(inValue);
	GotValue();
}


void VByte::FromLong8(sLONG8 inValue)
{
	fValue = sBYTE(inValue);
	GotValue();
}


void VByte::FromReal(Real inValue)
{
	fValue = sBYTE(inValue);
	GotValue();
}


void VByte::FromFloat(const VFloat& inValue)
{
	GotValue(inValue.IsNull());
	fValue = (sBYTE) inValue.GetWord();
}


void VByte::FromDuration(const VDuration& inValue)
{
	GotValue(inValue.IsNull());
	fValue = (sBYTE) inValue.GetWord();
}


void VByte::FromString(const VString& inValue)
{
	GotValue(inValue.IsNull());
	fValue = (sBYTE) inValue.GetWord();
}


void VByte::FromValue(const VValueSingle& inValue)
{
	fValue = (sBYTE) inValue.GetWord();
	GotValue(inValue.IsNull());
}


VError VByte::FromJSONValue( const VJSONValue& inJSONValue)
{
	CastToInteger( inJSONValue.GetNumber(), &fValue);
	GotValue( inJSONValue.IsNull());
	return VE_OK;
}


VError VByte::GetJSONValue(VJSONValue& outJSONValue, bool /*allowDates*/) const
{
	if (IsNull())
		outJSONValue.SetNull();
	else
		outJSONValue.SetNumber( fValue);
	return VE_OK;
}


void* VByte::LoadFromPtr(const void* inDataPtr,Boolean /*inRefOnly*/)
{
	fValue = *(sBYTE*)inDataPtr;
	GotValue();
	return ((sBYTE*)inDataPtr)+1;
}


void* VByte::WriteToPtr(void* inDataPtr,Boolean /*inRefOnly*/, VSize /*inMax*/) const
{
	*(sBYTE*)inDataPtr = fValue;
	return ((sBYTE*)inDataPtr)+1;
}


VError VByte::ReadFromStream(VStream* inStream, sLONG /*inParam*/)
{
	fValue = (sBYTE) inStream->GetWord();
	return inStream->GetLastError();
}


VError VByte::WriteToStream(VStream* inStream,sLONG inParam) const
{
	inherited::WriteToStream(inStream, inParam);

	inStream->PutWord(fValue);
	return inStream->GetLastError();
}


Boolean VByte::FromValueSameKind(const VValue* inValue)
{
	XBOX_ASSERT_KINDOF(VByte, inValue);
	const VByte*	theValue = reinterpret_cast<const VByte*>(inValue);
	fValue = theValue->fValue;
	GotValue(theValue->IsNull());
	return true;
}


CompareResult VByte::CompareToSameKind(const VValue* inValue, Boolean /*inDiacritic*/) const
{
	XBOX_ASSERT_KINDOF(VByte, inValue);
	const VByte*	theValue = reinterpret_cast<const VByte*>(inValue);
	
	if (fValue == theValue->fValue)
		return CR_EQUAL;
	else if (fValue > theValue->fValue)
		return CR_BIGGER;
	else
		return CR_SMALLER;
}


Boolean VByte::EqualToSameKind(const VValue* inValue, Boolean /*inDiacritic*/) const
{
	XBOX_ASSERT_KINDOF(VByte, inValue);
	const VByte*	theValue = reinterpret_cast<const VByte*>(inValue);
	return (fValue == theValue->fValue);
}


CompareResult VByte::CompareToSameKindPtr(const void* inPtrToValueData, Boolean /*inDiacritic*/) const
{
	if (fValue == *((sBYTE*)inPtrToValueData))
		return CR_EQUAL;
	else if (fValue > *((sBYTE*)inPtrToValueData))
		return CR_BIGGER;
	else
		return CR_SMALLER;
}


Boolean VByte::EqualToSameKindPtr(const void* inPtrToValueData, Boolean /*inDiacritic*/) const
{
	return fValue == *((sBYTE*)inPtrToValueData);
}


CompareResult VByte::CompareToSameKindWithOptions( const VValue* inValue, const VCompareOptions& /*inOptions*/) const
{
	return CompareToSameKindWithOptions_scalar<InfoType>( inValue, fValue);
}


bool VByte::EqualToSameKindWithOptions( const VValue* inValue, const VCompareOptions& /*inOptions*/) const
{
	return EqualToSameKindWithOptions_scalar<InfoType>( inValue, fValue);
}


CompareResult VByte::CompareToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& /*inOptions*/) const
{
	return CompareToSameKindPtrWithOptions_scalar<InfoType>( inPtrToValueData, fValue);
}


bool VByte::EqualToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& /*inOptions*/) const
{
	return EqualToSameKindPtrWithOptions_scalar<InfoType>( inPtrToValueData, fValue);
}


const VValueInfo *VByte::GetValueInfo() const
{
	return &sInfo;
}


#pragma mark-


VWord::VWord(sWORD* inDataPtr, Boolean inInit)
{
	if (inInit)
		Clear();
	else
		fValue = *inDataPtr;
}


void VWord::Clear()
{
	fValue = 0;
	GotValue();
}


CompareResult VWord::CompareTo (const VValueSingle& inValue, Boolean /*inDiacritic*/) const 
{
	sLONG value = inValue.GetLong();
	if (fValue == value)
		return CR_EQUAL;
	else if (fValue > value)
		return CR_BIGGER;
	else
		return CR_SMALLER;
	
}


void VWord::GetFloat(VFloat& outValue) const
{
	outValue.FromWord(fValue);
}


void VWord::GetDuration(VDuration& outValue) const
{
	outValue.FromWord(fValue);
}


void VWord::GetString(VString& outValue) const
{
	outValue.FromWord(fValue);
}


void VWord::GetValue(VValueSingle& outValue) const
{
	outValue.FromWord(fValue);
}


void VWord::FromBoolean(Boolean inValue)
{
	fValue = inValue;
	GotValue();
}


void VWord::FromByte(sBYTE inValue)
{
	fValue = inValue;
	GotValue();
}


void VWord::FromWord(sWORD inValue)
{
	fValue = inValue;
	GotValue();
}


void VWord::FromLong(sLONG inValue)
{
	fValue = sWORD(inValue);
	GotValue();
}


void VWord::FromLong8(sLONG8 inValue)
{
	fValue = sWORD(inValue);
	GotValue();
}


void VWord::FromReal(Real inValue)
{
	fValue = sWORD(inValue);
	GotValue();
}


void VWord::FromFloat(const VFloat& inValue)
{
	GotValue(inValue.IsNull());
	fValue = inValue.GetWord();
}


void VWord::FromDuration(const VDuration& inValue)
{
	GotValue(inValue.IsNull());
	fValue = inValue.GetWord();
}


void VWord::FromString(const VString& inValue)
{
	GotValue(inValue.IsNull());
	fValue = inValue.GetLong();
}


void VWord::FromValue(const VValueSingle& inValue)
{
	fValue = inValue.GetWord();
	GotValue(inValue.IsNull());
}


VError VWord::FromJSONValue( const VJSONValue& inJSONValue)
{
	CastToInteger( inJSONValue.GetNumber(), &fValue);
	GotValue( inJSONValue.IsNull());
	return VE_OK;
}


VError VWord::GetJSONValue(VJSONValue& outJSONValue, bool /*allowDates*/) const
{
	if (IsNull())
		outJSONValue.SetNull();
	else
		outJSONValue.SetNumber( fValue);
	return VE_OK;
}


void* VWord::LoadFromPtr(const void* inDataPtr,Boolean /*inRefOnly*/)
{
	fValue = *(sWORD*)inDataPtr;
	GotValue();
	return ((sWORD*)inDataPtr)+1;
}


void* VWord::WriteToPtr(void* inDataPtr,Boolean /*inRefOnly*/, VSize /*inMax*/) const
{
	*(sWORD*)inDataPtr = fValue;
	return ((sWORD*)inDataPtr)+1;
}


VError VWord::ReadFromStream(VStream* inStream, sLONG /*inParam*/)
{
	fValue = inStream->GetWord();
	return inStream->GetLastError();
}


VError VWord::WriteToStream(VStream* inStream,sLONG inParam) const
{
	inherited::WriteToStream(inStream, inParam);

	inStream->PutWord(fValue);
	return inStream->GetLastError();
}


CompareResult VWord::CompareToSameKind(const VValue* inValue, Boolean /*inDiacritic*/) const
{
	XBOX_ASSERT_KINDOF(VWord, inValue);
	const VWord*	theValue = reinterpret_cast<const VWord*>(inValue);
	
	if (fValue == theValue->fValue)
		return CR_EQUAL;
	else if (fValue > theValue->fValue)
		return CR_BIGGER;
	else
		return CR_SMALLER;
}


Boolean VWord::EqualToSameKind(const VValue* inValue, Boolean /*inDiacritic*/) const
{
	XBOX_ASSERT_KINDOF(VWord, inValue);
	const VWord*	theValue = reinterpret_cast<const VWord*>(inValue);
	return (fValue == theValue->fValue);
}


Boolean VWord::FromValueSameKind(const VValue* inValue)
{
	XBOX_ASSERT_KINDOF(VWord, inValue);
	const VWord*	theValue = reinterpret_cast<const VWord*>(inValue);
	fValue = theValue->fValue;
	GotValue(theValue->IsNull());
	return true;
}


CompareResult VWord::CompareToSameKindPtr(const void* inPtrToValueData, Boolean /*inDiacritic*/) const
{
	if (fValue == *((sWORD*)inPtrToValueData))
		return CR_EQUAL;
	else if (fValue > *((sWORD*)inPtrToValueData))
		return CR_BIGGER;
	else
		return CR_SMALLER;
}


Boolean VWord::EqualToSameKindPtr(const void* inPtrToValueData, Boolean /*inDiacritic*/) const
{
	return fValue == *((sWORD*)inPtrToValueData);
}


CompareResult VWord::CompareToSameKindWithOptions( const VValue* inValue, const VCompareOptions& /*inOptions*/) const
{
	return CompareToSameKindWithOptions_scalar<InfoType>( inValue, fValue);
}


bool VWord::EqualToSameKindWithOptions( const VValue* inValue, const VCompareOptions& /*inOptions*/) const
{
	return EqualToSameKindWithOptions_scalar<InfoType>( inValue, fValue);
}


CompareResult VWord::CompareToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& /*inOptions*/) const
{
	return CompareToSameKindPtrWithOptions_scalar<InfoType>( inPtrToValueData, fValue);
}


bool VWord::EqualToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& /*inOptions*/) const
{
	return EqualToSameKindPtrWithOptions_scalar<InfoType>( inPtrToValueData, fValue);
}


const VValueInfo *VWord::GetValueInfo() const
{
	return &sInfo;
}


#pragma mark-

VLong::VLong(sLONG* inDataPtr, Boolean inInit)
{
	if (inInit) 
		Clear();
	else
		fValue = *inDataPtr;
}


void VLong::Clear()
{
	fValue = 0;
	GotValue();
}


CompareResult VLong::CompareTo(const VValueSingle& inValue, Boolean /*inDiacritic*/) const 
{
	sLONG	value = inValue.GetLong();

	if (fValue == value)
		return CR_EQUAL;
	else if (fValue > value)
		return CR_BIGGER;
	else
		return CR_SMALLER;
}


void VLong::GetFloat(VFloat& inValue) const
{
	inValue.FromLong(fValue);
}


void VLong::GetDuration(VDuration& inValue) const
{
	inValue.FromLong(fValue);
}


void VLong::GetString(VString& inValue) const
{
	inValue.FromLong(fValue);
}


void VLong::GetValue(VValueSingle& inValue) const
{
	inValue.FromLong(fValue);
}


void VLong::FromBoolean(Boolean inValue)
{
	fValue = inValue;
	GotValue();
}


void VLong::FromByte(sBYTE inValue)
{
	fValue = inValue;
	GotValue();
}


void VLong::FromWord(sWORD inValue)
{
	fValue = inValue;
	GotValue();
} 


void VLong::FromLong(sLONG inValue)
{
	fValue = inValue;
	GotValue();
} 


void VLong::FromLong8(sLONG8 inValue)
{
	fValue = (sLONG) inValue;
	GotValue();
}

void VLong::FromReal(Real inValue)
{
	fValue = sLONG(inValue);
	GotValue();
} 


void VLong::FromFloat(const VFloat& inValue)
{
	GotValue(inValue.IsNull());
	fValue = inValue.GetLong();
}


void VLong::FromDuration(const VDuration& inValue)
{
	GotValue(inValue.IsNull());
	fValue = inValue.GetLong();
}


void VLong::FromString(const VString& inValue)
{
	GotValue(inValue.IsNull());
	fValue = inValue.GetLong();	
}


void VLong::FromValue(const VValueSingle& inValue)
{
	fValue = inValue.GetLong();
	GotValue(inValue.IsNull());
}


VError VLong::FromJSONValue( const VJSONValue& inJSONValue)
{
	CastToInteger( inJSONValue.GetNumber(), &fValue);
	GotValue( inJSONValue.IsNull());
	return VE_OK;
}


VError VLong::GetJSONValue(VJSONValue& outJSONValue, bool /*allowDates*/) const
{
	if (IsNull())
		outJSONValue.SetNull();
	else
		outJSONValue.SetNumber( fValue);
	return VE_OK;
}


void* VLong::LoadFromPtr(const void* inDataPtr,Boolean /*inRefOnly*/)
{
	fValue = *(sLONG*)inDataPtr;
	GotValue();
	return ((sLONG*)inDataPtr)+1;
}


void* VLong::WriteToPtr(void* inDataPtr,Boolean /*inRefOnly*/, VSize /*inMax*/) const
{
	*(sLONG*)inDataPtr = fValue;
	return ((sLONG*)inDataPtr)+1;
}


VError VLong::ReadFromStream(VStream* inStream, sLONG /*inParam*/)
{
	fValue = inStream->GetLong();
	return inStream->GetLastError();
}


VError VLong::WriteToStream(VStream* inStream, sLONG inParam) const
{
	inherited::WriteToStream(inStream, inParam);

	inStream->PutLong(fValue);
	return inStream->GetLastError();
}


CompareResult VLong::CompareToSameKind(const VValue* inValue, Boolean /*inDiacritic*/) const
{
	XBOX_ASSERT_KINDOF(VLong, inValue);
	const VLong*	theValue = reinterpret_cast<const VLong*>(inValue);
	if (fValue == theValue->fValue) return CR_EQUAL;
	else if (fValue>theValue->fValue) return CR_BIGGER;
	else return CR_SMALLER;
}


Boolean VLong::EqualToSameKind(const VValue* inValue, Boolean /*inDiacritic*/) const
{
	XBOX_ASSERT_KINDOF(VLong, inValue);
	const VLong*	theValue = reinterpret_cast<const VLong*>(inValue);
	return (fValue == theValue->fValue);
}


Boolean VLong::FromValueSameKind(const VValue* inValue)
{
	XBOX_ASSERT_KINDOF(VLong, inValue);
	const VLong*	theValue = reinterpret_cast<const VLong*>(inValue);
	fValue = theValue->fValue;
	GotValue(theValue->IsNull());
	return true;
}


CompareResult VLong::CompareToSameKindPtr(const void* inPtrToValueData, Boolean /*inDiacritic*/) const
{
	if (fValue == *((sLONG*)inPtrToValueData))
		return CR_EQUAL;
	else if (fValue > *((sLONG*)inPtrToValueData))
		return CR_BIGGER;
	else
		return CR_SMALLER;
}


Boolean VLong::EqualToSameKindPtr(const void* inPtrToValueData, Boolean /*inDiacritic*/) const
{
	return fValue == *((sLONG*)inPtrToValueData);
}


CompareResult VLong::CompareToSameKindWithOptions( const VValue* inValue, const VCompareOptions& /*inOptions*/) const
{
	return CompareToSameKindWithOptions_scalar<InfoType>( inValue, fValue);
}


bool VLong::EqualToSameKindWithOptions( const VValue* inValue, const VCompareOptions& /*inOptions*/) const
{
	return EqualToSameKindWithOptions_scalar<InfoType>( inValue, fValue);
}


CompareResult VLong::CompareToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& /*inOptions*/) const
{
	return CompareToSameKindPtrWithOptions_scalar<InfoType>( inPtrToValueData, fValue);
}


bool VLong::EqualToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& /*inOptions*/) const
{
	return EqualToSameKindPtrWithOptions_scalar<InfoType>( inPtrToValueData, fValue);
}


const VValueInfo *VLong::GetValueInfo() const
{
	return &sInfo;
}


#pragma mark-

VLong8::VLong8(sLONG8* inDataPtr, Boolean inInit)
{
	if (inInit)
		Clear();
	else
		fValue = *inDataPtr;
}


void VLong8::Clear()
{
	fValue = 0;
	GotValue();
}


void VLong8::GetFloat(VFloat& outValue) const
{
	outValue.FromLong8(fValue);
}


void VLong8::GetDuration(VDuration& outValue) const
{
	outValue.FromLong8(fValue);
}


void VLong8::GetString(VString& inValue) const
{
	inValue.FromLong8(fValue);
}


void VLong8::GetValue(VValueSingle& outValue) const
{
	outValue.FromLong8(fValue);
}


void VLong8::FromBoolean(Boolean inValue)
{
	fValue = inValue;
	GotValue();
}


void VLong8::FromByte(sBYTE inValue)
{
	fValue = inValue;
	GotValue();
}


void VLong8::FromWord(sWORD inValue)
{
	fValue = inValue;
	GotValue();
}


void VLong8::FromLong(sLONG inValue)
{
	fValue = inValue;
	GotValue();
}


void VLong8::FromLong8(sLONG8 inValue)
{
	fValue = inValue;
	GotValue();
}


void VLong8::FromReal(Real inValue)
{
	fValue = (sLONG8) inValue;
	GotValue();
}


void VLong8::FromFloat(const VFloat& inValue)
{
	GotValue(inValue.IsNull());
	fValue = inValue.GetLong8();
}


void VLong8::FromDuration(const VDuration& inValue)
{
	GotValue(inValue.IsNull());
	fValue = inValue.GetLong8();
}


void VLong8::FromString(const VString& inValue)
{
	GotValue(inValue.IsNull());
	if (!IsNull())
		fValue = inValue.GetLong8();
}


void VLong8::FromValue(const VValueSingle& inValue)
{
	fValue = inValue.GetLong8();
	GotValue(inValue.IsNull());
}


VError VLong8::FromJSONValue( const VJSONValue& inJSONValue)
{
	CastToInteger( inJSONValue.GetNumber(), &fValue);
	GotValue( inJSONValue.IsNull());
	return VE_OK;
}


VError VLong8::GetJSONValue(VJSONValue& outJSONValue, bool /*allowDates*/) const
{
	if (IsNull())
		outJSONValue.SetNull();
	else
		outJSONValue.SetNumber( fValue);
	return VE_OK;
}


CompareResult VLong8::CompareTo(const VValueSingle& inValue, Boolean /*inDiacritic*/) const
{
	sLONG8	value = inValue.GetLong8();
	if (fValue == value) return CR_EQUAL;
	else if (fValue > value) return CR_BIGGER;
	else return CR_SMALLER;
}


void* VLong8::LoadFromPtr(const void* inDataPtr, Boolean /*inRefOnly*/)
{
	fValue = *(sLONG8*)inDataPtr;
	GotValue();
	return ((sLONG8*)inDataPtr)+1;
}


void* VLong8::WriteToPtr(void* inDataPtr, Boolean /*inRefOnly*/, VSize /*inMax*/) const
{
	*(sLONG8*)inDataPtr = fValue;
	return ((sLONG8*)inDataPtr)+1;
}


VError VLong8::ReadFromStream(VStream* inStream, sLONG /*inParam*/)
{
	fValue = inStream->GetLong8();
	return inStream->GetLastError();
}


VError VLong8::WriteToStream(VStream* inStream, sLONG inParam) const
{
	inherited::WriteToStream(inStream, inParam);
	
	inStream->PutLong8(fValue);
	return inStream->GetLastError();
}


CompareResult VLong8::CompareToSameKind(const VValue* inValue, Boolean /*inDiacritic*/) const
{
	XBOX_ASSERT_KINDOF(VLong8, inValue);
	const VLong8*	theValue = reinterpret_cast<const VLong8*>(inValue);
	if (fValue == theValue->fValue) return CR_EQUAL;
	else if (fValue>theValue->fValue) return CR_BIGGER;
	else return CR_SMALLER;
}


Boolean VLong8::EqualToSameKind(const VValue* inValue, Boolean /*inDiacritic*/) const
{
	XBOX_ASSERT_KINDOF(VLong8, inValue);
	const VLong8*	theValue = reinterpret_cast<const VLong8*>(inValue);
	return (fValue == theValue->fValue);
}


Boolean VLong8::FromValueSameKind(const VValue* inValue)
{
	XBOX_ASSERT_KINDOF(VLong8, inValue);
	const VLong8*	theValue = reinterpret_cast<const VLong8*>(inValue);
	fValue = theValue->fValue;
	GotValue(theValue->IsNull());
	return true;
}


CompareResult VLong8::CompareToSameKindPtr(const void* inPtrToValueData, Boolean /*inDiacritic*/) const
{
	if (fValue == *((sLONG8*)inPtrToValueData))
		return CR_EQUAL;
	else if (fValue > *((sLONG8*)inPtrToValueData))
		return CR_BIGGER;
	else
		return CR_SMALLER;
}


Boolean VLong8::EqualToSameKindPtr(const void* inPtrToValueData, Boolean /*inDiacritic*/) const
{
	return fValue == *((sLONG8*)inPtrToValueData);
}


CompareResult VLong8::CompareToSameKindWithOptions( const VValue* inValue, const VCompareOptions& /*inOptions*/) const
{
	return CompareToSameKindWithOptions_scalar<InfoType>( inValue, fValue);
}


bool VLong8::EqualToSameKindWithOptions( const VValue* inValue, const VCompareOptions& /*inOptions*/) const
{
	return EqualToSameKindWithOptions_scalar<InfoType>( inValue, fValue);
}


CompareResult VLong8::CompareToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& /*inOptions*/) const
{
	return CompareToSameKindPtrWithOptions_scalar<InfoType>( inPtrToValueData, fValue);
}


bool VLong8::EqualToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& /*inOptions*/) const
{
	return EqualToSameKindPtrWithOptions_scalar<InfoType>( inPtrToValueData, fValue);
}


const VValueInfo *VLong8::GetValueInfo() const
{
	return &sInfo;
}


#pragma mark-


VBoolean::VBoolean(uBYTE* inDataPtr, Boolean inInit)
{
	fValue = (*inDataPtr != 0);
	if (inInit)
		Clear();
	else
		fValue = (*inDataPtr != 0);
}


void VBoolean::GetValue(VValueSingle& inValue) const
{
	inValue.FromBoolean(fValue);
}


void VBoolean::Clear()
{
	fValue = 0;
	GotValue();
}


void VBoolean::GetFloat(VFloat& outValue) const
{
	outValue.FromBoolean(fValue);
}


void VBoolean::GetDuration(VDuration& outValue) const
{
	outValue.FromBoolean(fValue);
}


void VBoolean::GetString(VString& outValue) const
{
	outValue.FromBoolean(fValue);
}


void VBoolean::FromBoolean(Boolean inValue)
{
	fValue = inValue;
	GotValue();
}


void VBoolean::FromByte(sBYTE inValue)
{
	fValue = inValue;
	GotValue();
}


void VBoolean::FromWord(sWORD inValue)
{
	fValue = inValue != 0;
	GotValue();
}


void VBoolean::FromLong(sLONG inValue)
{
	fValue = inValue != 0;
	GotValue();
} 


void VBoolean::FromLong8(sLONG8 inValue)
{
	fValue = inValue != 0;
	GotValue();
}


void VBoolean::FromReal(Real inValue)
{
	fValue = inValue != 0;
	GotValue();
} 


void VBoolean::FromFloat(const VFloat& inValue)
{
	fValue = inValue.GetBoolean();
	GotValue(inValue.IsNull());
}


void VBoolean::FromDuration(const VDuration& inValue)
{
	GotValue(inValue.IsNull());
	fValue = inValue.GetBoolean();
}


void VBoolean::FromString(const VString& inValue)
{
	GotValue(inValue.IsNull());
	fValue = inValue.GetBoolean();
}


void VBoolean::GetXMLString( VString& outString, XMLStringOptions /*inOptions*/) const
{
	static VString sFalse( "false");	// use VString to gain refcounting
	static VString sTrue( "true");
	if (fValue)
		outString.FromString( sTrue);
	else
		outString.FromString( sFalse);
}


bool VBoolean::FromXMLString( const VString& inString)
{
	/*
		Must be fast.
		don't need VIntlMgr overhead.

		accept 'true' & 'false' in any case.
		accept '0' & '1'.
	*/
	bool ok = false;
	if (inString.GetLength() == 4)
	{
		if ( ( (inString[0] == 't') || (inString[0] == 'T') )
			&& ( (inString[1] == 'r') || (inString[1] == 'R') )
			&& ( (inString[2] == 'u') || (inString[2] == 'U') )
			&& ( (inString[3] == 'e') || (inString[3] == 'E') ) )
		{
			fValue = 1;
			GotValue();
			ok = true;
		}
	}
	else if (inString.GetLength() == 5)
	{
		if ( ( (inString[0] == 'f') || (inString[0] == 'F') )
			&& ( (inString[1] == 'a') || (inString[1] == 'A') )
			&& ( (inString[2] == 'l') || (inString[2] == 'L') )
			&& ( (inString[3] == 's') || (inString[3] == 'S') )
			&& ( (inString[4] == 'e') || (inString[4] == 'E') ) )
		{
			fValue = 0;
			GotValue();
			ok = true;
		}
	}
	else if (inString.GetLength() == 1)
	{
		if (inString[0] == '1')
		{
			fValue = 1;
			GotValue();
			ok = true;
		}
		else if (inString[0] == '0')
		{
			fValue = 0;
			GotValue();
			ok = true;
		}
	}
	return ok;
}


VError VBoolean::FromJSONString(const VString& inJSONString, JSONOption /*inModifier*/)
{
	if (!FromXMLString(inJSONString))
	{
		fValue = 0;
		GotValue();
	}
	return VE_OK;
}


VError VBoolean::GetJSONString(VString& outJSONString, JSONOption /*inModifier*/) const
{
	static const UniChar sTrue[] = { 't', 'r', 'u', 'e' };
	static const UniChar sFalse[] = { 'f', 'a', 'l', 's', 'e' };
	if (fValue)
		outJSONString.FromUniString( sTrue, 4);
	else
		outJSONString.FromUniString( sFalse, 5);
	return VE_OK;
}


VError VBoolean::FromJSONValue( const VJSONValue& inJSONValue)
{
	fValue = inJSONValue.GetBool() ? 1 : 0;
	GotValue( inJSONValue.IsNull());
	return VE_OK;
}


VError VBoolean::GetJSONValue(VJSONValue& outJSONValue, bool /*allowDates*/) const
{
	if (IsNull())
		outJSONValue.SetNull();
	else
		outJSONValue.SetBool( fValue != 0);
	return VE_OK;
}


void VBoolean::FromValue(const VValueSingle& inValue)
{
	fValue = inValue.GetBoolean();
	GotValue(inValue.IsNull());
}


void* VBoolean::WriteToPtr(void* inDataPtr,Boolean /*inRefOnly*/, VSize /*inMax*/) const
{
	*(uBYTE*)inDataPtr = fValue ? (uBYTE) 1 : (uBYTE) 0;
	return ((uBYTE*)inDataPtr)+1;
}


void* VBoolean::LoadFromPtr(const void* inDataPtr,Boolean /*inRefOnly*/)
{
	fValue = (*(uBYTE*)inDataPtr != 0);
	GotValue();
	return ((uBYTE*)inDataPtr)+1;
}


VError VBoolean::ReadFromStream(VStream* inStream, sLONG /*inParam*/)
{
	sWORD	ww = inStream->GetWord();
	fValue = ww != 0;
	return inStream->GetLastError();
}


VError VBoolean::WriteToStream(VStream* inStream,sLONG inParam) const
{
	inherited::WriteToStream(inStream, inParam);
		
	inStream->PutWord(fValue);
	return inStream->GetLastError();
}


CompareResult VBoolean::CompareTo (const VValueSingle& inValue, Boolean /*inDiacritic*/) const 
{
	Boolean	value = inValue.GetBoolean();
	
	if (fValue == value)
		return CR_EQUAL;
	else if (fValue)
		return CR_BIGGER;
	else
		return CR_SMALLER;
}


CompareResult VBoolean::CompareToSameKind(const VValue* inValue, Boolean /*inDiacritic*/) const
{
	XBOX_ASSERT_KINDOF(VBoolean, inValue);
	const VBoolean*	theValue = reinterpret_cast<const VBoolean*>(inValue);
	
	if (fValue == theValue->fValue)
		return CR_EQUAL;
	else if (fValue)
		return CR_BIGGER;
	else
		return CR_SMALLER;
}


Boolean VBoolean::EqualToSameKind(const VValue* inValue, Boolean /*inDiacritic*/) const
{
	XBOX_ASSERT_KINDOF(VBoolean, inValue);
	const VBoolean*	theValue = reinterpret_cast<const VBoolean*>(inValue);
	return (fValue == theValue->fValue);
}


Boolean VBoolean::FromValueSameKind(const VValue* inValue)
{
	XBOX_ASSERT_KINDOF(VBoolean, inValue);
	const VBoolean*	theValue = reinterpret_cast<const VBoolean*>(inValue);
	fValue = theValue->fValue;
	GotValue(theValue->IsNull());
	return true;
}


CompareResult VBoolean::CompareToSameKindPtr(const void* inPtrToValueData, Boolean /*inDiacritic*/) const
{
	if (fValue == *((Boolean*)inPtrToValueData))
		return CR_EQUAL;
	else if (fValue )
		return CR_BIGGER;
	else
		return CR_SMALLER;
}


Boolean VBoolean::EqualToSameKindPtr(const void* inPtrToValueData, Boolean /*inDiacritic*/) const
{
	return fValue == *((Boolean*)inPtrToValueData);
}


CompareResult VBoolean::CompareToSameKindWithOptions( const VValue* inValue, const VCompareOptions& /*inOptions*/) const
{
	return CompareToSameKindWithOptions_scalar<InfoType>( inValue, fValue);
}


bool VBoolean::EqualToSameKindWithOptions( const VValue* inValue, const VCompareOptions& /*inOptions*/) const
{
	return EqualToSameKindWithOptions_scalar<InfoType>( inValue, fValue);
}


CompareResult VBoolean::CompareToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& /*inOptions*/) const
{
	return CompareToSameKindPtrWithOptions_scalar<InfoType>( inPtrToValueData, fValue);
}


bool VBoolean::EqualToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& /*inOptions*/) const
{
	return EqualToSameKindPtrWithOptions_scalar<InfoType>( inPtrToValueData, fValue);
}


const VValueInfo *VBoolean::GetValueInfo() const
{
	return &sInfo;
}


#pragma mark-


CompareResult VReal::VValueInfo_real::CompareTwoPtrToDataWithOptions( const void* inPtrToValueData1, const void* inPtrToValueData2, const VCompareOptions& inOptions) const
{
	Real r1 = *(Real*) inPtrToValueData1;
	Real r2 = *(Real*) inPtrToValueData2;
	
	if (isnan( r1))
		return isnan( r2) ? CR_EQUAL : CR_SMALLER;
	else if (isnan( r2))
		return isnan( r1) ? CR_EQUAL : CR_BIGGER;
	
	if (!inOptions.IsDiacritical() )
	{
		if ( fabs ( r1 - r2 ) < inOptions.GetEpsilon() )
			return CR_EQUAL;
		else if (r1 > r2 )
			return CR_BIGGER;
		else
			return CR_SMALLER;
	}
	else
	{
		if ( r1 == r2)
			return CR_EQUAL;
		else if ( r1 > r2)
			return CR_BIGGER;
		else
			return CR_SMALLER;
	}
}


VReal::VReal(Real* inDataPtr, Boolean inInit)
{
	if (inInit)
		Clear();
	else
		fValue = *inDataPtr;
}


void VReal::Clear()
{
	fValue = 0;
	GotValue();
}


void VReal::GetFloat(VFloat& outValue) const
{
	outValue.FromReal(fValue);
}


void VReal::GetDuration(VDuration& outValue) const
{
	outValue.FromReal(fValue);
}


void VReal::GetString(VString& inValue) const
{
	inValue.FromReal(fValue);
}


void VReal::GetValue(VValueSingle& inValue) const
{
	inValue.FromReal(fValue);
}


void VReal::FromBoolean(Boolean inValue)
{
	fValue = (Real) inValue;
	GotValue();
}

void VReal::FromByte(sBYTE inValue)
{
	fValue = (Real) inValue;
	GotValue();
}


void VReal::FromWord(sWORD inValue)
{
	fValue = (Real) inValue;
	GotValue();
}


void VReal::FromLong(sLONG inValue)
{
	fValue = (Real) inValue;
	GotValue();
}


void VReal::FromLong8(sLONG8 inValue)
{
	fValue = (Real) inValue;
	GotValue();
}


void VReal::FromReal(Real inValue)
{
	fValue = inValue;
	GotValue();
}


void VReal::FromFloat(const VFloat& inValue)
{
	fValue = inValue.GetReal();
	GotValue(inValue.IsNull());
}


void VReal::FromDuration(const VDuration& inValue)
{
	fValue = inValue.GetReal();
	GotValue(inValue.IsNull());
}


void VReal::FromString(const VString& inValue)
{
	GotValue(inValue.IsNull());
	if (!IsNull())
		fValue = inValue.GetReal();
}

/*
	static inspired from xalan code.
*/
void VReal::RealToXMLString( Real inValue, VString& outString)
{
	// always with a '.' as decimal separator
	// produce NaN, INF, -INF
	// use exp notation if necessary
	char buff[255];
	int count;
	#if COMPIL_VISUAL
	int			nClass = ::_fpclass ( inValue );
	if ( nClass == _FPCLASS_PINF )
	{
		sLONG		nInf = inValue;
		buff[0] = 'I';
		buff[1] = 'N';
		buff[2] = 'F';
		buff[3] = '\0';
		count = 3;
	}
	else if ( nClass == _FPCLASS_NINF )
	{
		buff[0] = '-';
		buff[1] = 'I';
		buff[2] = 'N';
		buff[3] = 'F';
		buff[4] = '\0';
		count = 3;
	}
	else
		count = sprintf( buff, "%.14G", inValue);
	#else
	count = snprintf( buff, sizeof( buff), "%.14G", inValue);
	
	// change NAN into NaN
	if ( (count == 3) && (buff[1] == 'A') )
		buff[1] = 'a';
	#endif
	
	UniChar *p = outString.GetCPointerForWrite( count);
	if (p != NULL)
	{
		for( int i = 0 ; i < count ; ++i)
			p[i] = (UniChar) buff[i];
		outString.Validate( count);
	}

	//JQ 07/11/2012: fixed ACI0078310 (in XML decimal separator is always '.')
    const char  theDecimalPointChar = ::localeconv()->decimal_point[0];	// inspired from xalan
	if (theDecimalPointChar != '.')
		outString.ExchangeAll((UniChar)theDecimalPointChar,'.');
}


void VReal::GetXMLString( VString& outString, XMLStringOptions /*inOptions*/) const
{
	RealToXMLString( fValue, outString);
}


bool VReal::FromXMLString( const VString& inString)
{
	bool ok;

	char buffer[255];
	VSize size = inString.ToBlock( buffer, sizeof(buffer), VTC_StdLib_char, true, false);
	if (size < sizeof(buffer))
	{
		//for backward compatibility, replace decimal separator ',' by '.'
		char *p = buffer;
		while (*p)
		{
			if (*p == ',')
				*p = '.';
			p++;
		}

		const char *ptr_begin = &buffer[0];
		size_t strLen = strlen( ptr_begin);
		char *ptr_end = NULL;
		Real result = strtod( ptr_begin, &ptr_end);

		// check if all chars are valid char
		if ( (size_t)(ptr_end - ptr_begin) == strLen)
		{
			fValue = result;
			GotValue();
			ok = true;
		}
		else
		{
			ok = false;
		}
	}
	else
	{
		ok = false;
	}
	return ok;
}


VError VReal::FromJSONString(const VString& inJSONString, JSONOption /*inModifier*/)
{
	if (!FromXMLString(inJSONString))
	{
		fValue = 0;
		GotValue();
	}
	return VE_OK;
}


VError VReal::GetJSONString(VString& outJSONString, JSONOption /*inModifier*/) const
{
	char buff[255];
#if COMPIL_VISUAL
	int			nClass = ::_fpclass ( fValue );
	if ( nClass == _FPCLASS_PINF ||  nClass == _FPCLASS_NINF || nClass == _FPCLASS_SNAN || nClass == _FPCLASS_QNAN)
	{
		buff[0] = 'n';
		buff[1] = 'u';
		buff[2] = 'l';
		buff[3] = 'l';
		buff[4] = '\0';
	}
	else
		int count = sprintf( buff, "%.14G", fValue);
#else
	int count = snprintf( buff, sizeof( buff), "%.14G", fValue);

	if ( (strcmp(buff, "NAN") == 0) || (strcmp(buff, "INF") == 0) || (strcmp(buff, "-INF") == 0) )
	{
		buff[0] = 'n';
		buff[1] = 'u';
		buff[2] = 'l';
		buff[3] = 'l';
		buff[4] = '\0';
	}
#endif

	//JQ 07/11/2012: fixed ACI0078310 (in JSON decimal separator is always '.')
    const char theDecimalPointChar = ::localeconv()->decimal_point[0];	// inspired from xalan
	if (theDecimalPointChar != '.')
	{
		char *p = strchr( buff, theDecimalPointChar);
		if (p != NULL)
		{
			*p = '.';
			++p;
		}
	}

	outJSONString.FromCString( buff);

	return VE_OK;
}


VError VReal::FromJSONValue( const VJSONValue& inJSONValue)
{
	if (inJSONValue.IsNull())
		SetNull( true);
	else
	{
		fValue = inJSONValue.GetNumber();
		GotValue();
	}
	return VE_OK;
}


VError VReal::GetJSONValue(VJSONValue& outJSONValue, bool /*allowDates*/) const
{
	if (IsNull())
		outJSONValue.SetNull();
	else
		outJSONValue.SetNumber( fValue);
	return VE_OK;
}


void VReal::FromValue(const VValueSingle& inValue)
{
	fValue = inValue.GetReal();
	GotValue(inValue.IsNull());
}


void* VReal::WriteToPtr(void* inDataPtr, Boolean /*inRefOnly*/, VSize /*inMax*/) const
{
	*(Real*)inDataPtr = fValue;
	return ((Real*)inDataPtr)+1;
}


void* VReal::LoadFromPtr(const void* inDataPtr, Boolean /*inRefOnly*/)
{
	fValue = *(Real*)inDataPtr;
	GotValue();
	return (((Real*)inDataPtr) + 1);
}


VError VReal::ReadFromStream(VStream* inStream, sLONG /*inParam*/)
{
	fValue = inStream->GetReal();
	return inStream->GetLastError();
}


VError VReal::WriteToStream(VStream* inStream,sLONG inParam) const
{
	inherited::WriteToStream(inStream, inParam);

	inStream->PutReal(fValue);
	return inStream->GetLastError();
}


CompareResult VReal::CompareToSameKind(const VValue* inValue, Boolean /*inDiacritic*/) const
{
	XBOX_ASSERT_KINDOF(VReal, inValue);
	const VReal*	theValue = reinterpret_cast<const VReal*>(inValue);

	if ( fabs ( ( fValue - theValue->fValue ) ) < kDefaultEpsilon )
		return CR_EQUAL;
	else if (fValue > theValue->fValue)
		return CR_BIGGER;
	else
		return CR_SMALLER;
}


Boolean VReal::EqualToSameKind(const VValue* inValue, Boolean /*inDiacritic*/) const
{
	XBOX_ASSERT_KINDOF(VReal, inValue);
	const VReal*	theValue = reinterpret_cast<const VReal*>(inValue);

	return ( fabs ( fValue - theValue->fValue ) < kDefaultEpsilon );
}


Boolean VReal::FromValueSameKind(const VValue* inValue)
{
	XBOX_ASSERT_KINDOF(VReal, inValue);
	const VReal*	theValue = reinterpret_cast<const VReal*>(inValue);
	fValue = theValue->fValue;
	GotValue(theValue->IsNull());

	return true;
}


CompareResult VReal::CompareToSameKindPtr(const void* inPtrToValueData, Boolean /*inDiacritic*/) const
{
	if ( fabs ( fValue - ( *((Real*)inPtrToValueData) ) ) < kDefaultEpsilon )
		return CR_EQUAL;
	else if (fValue > *((Real*)inPtrToValueData))
		return CR_BIGGER;
	else
		return CR_SMALLER;
}


Boolean VReal::EqualToSameKindPtr(const void* inPtrToValueData, Boolean /*inDiacritic*/) const
{
	return fabs ( fValue - ( *((Real*)inPtrToValueData) ) ) < kDefaultEpsilon;
}


CompareResult VReal::CompareTo (const VValueSingle& inValue, Boolean /*inDiacritic*/) const 
{
	Real	value = inValue.GetReal();
	if ( fabs ( fValue - value ) < kDefaultEpsilon )
		return CR_EQUAL;
	else if (fValue > value)
		return CR_BIGGER;
	else
		return CR_SMALLER;
}


CompareResult VReal::CompareToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const
{
	XBOX_ASSERT_KINDOF(VReal, inValue);
	const VReal*	theValue = reinterpret_cast<const VReal*>(inValue);
	
	if ( fabs ( ( fValue - theValue->fValue ) ) < inOptions.GetEpsilon() )
		return CR_EQUAL;
	else if (fValue > theValue->fValue)
		return CR_BIGGER;
	else
		return CR_SMALLER;
}


bool VReal::EqualToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const
{
	XBOX_ASSERT_KINDOF(VReal, inValue);
	const VReal*	theValue = reinterpret_cast<const VReal*>(inValue);

	return ( fabs ( fValue - theValue->fValue ) < inOptions.GetEpsilon() );
}


CompareResult VReal::CompareToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const
{
	if ( fabs ( fValue - ( *((Real*)inPtrToValueData) ) ) < inOptions.GetEpsilon() )
		return CR_EQUAL;
	else if (fValue > *((Real*)inPtrToValueData))
		return CR_BIGGER;
	else
		return CR_SMALLER;
}


bool VReal::EqualToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const
{
	return fabs ( fValue - ( *((Real*)inPtrToValueData) ) ) < inOptions.GetEpsilon();
}


const VValueInfo *VReal::GetValueInfo() const
{
	return &sInfo;
}

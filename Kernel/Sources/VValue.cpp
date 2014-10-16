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
#include "VValue.h"
#include "VTask.h"
#include "VStream.h"
#include "VFloat.h"
#include "VTime.h"
#include "VString.h"
#include "VUUID.h"
#include "VBlob.h"
#include "VValueBag.h"
#include "VArrayValue.h"
#include "VJSONValue.h"
#include "VMemoryCpp.h"
#include "VKernelErrors.h"



VError VCompareOptions::ToJSON(VJSONObject* outObj)
{
	outObj->SetPropertyAsBool("diacritical", fDiacritical);
	return VE_OK;
}


VError VCompareOptions::FromJSON(VJSONObject* inObj)
{
	inObj->GetPropertyAsBool("diacritical", &fDiacritical);
	return VE_OK;
}


VValueInfoIndex* VValue::sInfos;	// must remain when the last global object is deleted, so it is intentionnally leaked


//========================================================================================================================

VValueInfo::VValueInfo( ValueKind inKind):fKind( inKind),fTrueKind( inKind)
{
	VValue::RegisterValueInfo( this);
}


VValueInfo::VValueInfo( ValueKind inKind, ValueKind inTrueKind):fKind( inKind),fTrueKind( inTrueKind)
{
	VValue::RegisterValueInfo( this);
}


VValueInfo::~VValueInfo()
{
	VValue::UnregisterValueInfo( this);
}


CompareResult VValueInfo::CompareTwoPtrToData(const void* /*inPtrToValueData1*/, const void* /*inPtrToValueData2*/, Boolean /*inDiacritical*/) const
{
	return CR_EQUAL;
}


CompareResult VValueInfo::CompareTwoPtrToDataBegining(const void* /*inPtrToValueData1*/, const void* /*inPtrToValueData2*/, Boolean /*inDiacritical*/) const
{
	return CR_EQUAL;
}


CompareResult VValueInfo::CompareTwoPtrToData_Like(const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean inDiacritical) const
{
	return CompareTwoPtrToData(inPtrToValueData1, inPtrToValueData2, inDiacritical);
}


CompareResult VValueInfo::CompareTwoPtrToDataBegining_Like(const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean inDiacritical) const
{
	return CompareTwoPtrToDataBegining(inPtrToValueData1, inPtrToValueData2, inDiacritical);
}


CompareResult VValueInfo::CompareTwoPtrToDataWithOptions( const void* inPtrToValueData1, const void* inPtrToValueData2, const VCompareOptions& inOptions) const
{
	return CR_EQUAL;
}


VSize VValueInfo::GetSizeOfValueDataPtr( const void* /*inPtrToValueData*/) const
{
	return 0;
}


VSize VValueInfo::GetAvgSpace() const
{
	return 0;
}



//========================================================================================================================


Boolean VValue::CanBeEvaluated() const
{
	return true;
}


void VValue::Clear()
{
	xbox_assert( false); // needs to be overriden
}


const VValueSingle* VValue::IsVValueSingle() const
{
	return NULL;
}


VValueSingle* VValue::IsVValueSingle()
{
	return NULL;
}


void VValue::SetDirty( bool inSet)
{
	uBYTE set = inSet ? 1 : 0;
	if (fFlags.fIsDirty != set)
	{
		fFlags.fIsDirty = set;
		
		DoDirtyChanged();
	}
}


void VValue::DoDirtyChanged()
{
}


void VValue::SetNull( bool inSet)
{
	uBYTE set = inSet ? 1 : 0;
	if (fFlags.fIsNull != set)
	{
		fFlags.fIsNull = set;
		
		DoNullChanged();
		fFlags.fIsNull = set;
		SetDirty(true);
	}
}


void VValue::DoNullChanged()
{
}


void* VValue::LoadFromPtr( const void* inDataPtr, Boolean /*inRefOnly*/)
{
	return const_cast<void*>( inDataPtr);
}


void* VValue::WriteToPtr(void* inDataPtr, Boolean /*inRefOnly*/, VSize inMax) const
{
	return inDataPtr;
}


VSize VValue::GetSpace(VSize /* inMax */) const
{
	return 0;
}


CompareResult VValue::CompareToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const
{
	return CR_EQUAL;
}


bool VValue::EqualToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const
{
	return (CompareToSameKindWithOptions( inValue, inOptions) == CR_EQUAL);
}


CompareResult VValue::CompareToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const
{
	return CR_EQUAL;
}


CompareResult VValue::Swap_CompareToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const
{
	return InvertCompResult(CompareToSameKindPtrWithOptions( inPtrToValueData, inOptions));
}


bool VValue::EqualToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const
{
	return (CompareToSameKindPtrWithOptions( inPtrToValueData, inOptions) == CR_EQUAL);
}


CompareResult VValue::CompareToSameKind(const VValue* /*inValue*/, Boolean /*inDiacritical*/) const
{
	return CR_EQUAL;
}


CompareResult VValue::CompareToSameKindPtr(const void* /*inPtrToValueData*/, Boolean /*inDiacritical*/) const
{
	return CR_EQUAL;
}


CompareResult VValue::Swap_CompareToSameKindPtr(const void* inPtrToValueData, Boolean inDiacritical) const
{
	return InvertCompResult(CompareToSameKindPtr(inPtrToValueData, inDiacritical));
}


CompareResult VValue::CompareBeginingToSameKindPtr(const void* inPtrToValueData, Boolean inDiacritical ) const
{
	return CompareToSameKindPtr(inPtrToValueData, inDiacritical);
}


CompareResult VValue::IsTheBeginingToSameKindPtr(const void* inPtrToValueData, Boolean inDiacritical ) const
{
	return CompareToSameKindPtr(inPtrToValueData, inDiacritical);
}


Boolean VValue::EqualToSameKindPtr(const void* inPtrToValueData, Boolean inDiacritical) const
{
	return (CompareToSameKindPtr(inPtrToValueData, inDiacritical) == CR_EQUAL);
}

CompareResult VValue::CompareBeginingToSameKind(const VValue* inValue, Boolean inDiacritical ) const
{
	return CompareToSameKind(inValue, inDiacritical);
}


Boolean VValue::EqualToSameKind(const VValue* inValue, Boolean inDiacritical) const
{
	return (CompareToSameKind(inValue, inDiacritical) == CR_EQUAL);
}


Boolean VValue::FromValueSameKind(const VValue* /*inValue*/)
{
	assert(false);
	return false;
}


CompareResult VValue::CompareToSameKind_Like( const VValue* inValue, Boolean inDiacritical) const
{
	return CompareToSameKind(inValue, inDiacritical);
}


CompareResult VValue::CompareBeginingToSameKind_Like( const VValue* inValue, Boolean inDiacritical) const
{
	return CompareBeginingToSameKind(inValue, inDiacritical);
}


Boolean VValue::EqualToSameKind_Like( const VValue* inValue, Boolean inDiacritical) const
{
	return EqualToSameKind(inValue, inDiacritical);
}


CompareResult VValue::CompareToSameKindPtr_Like( const void* inPtrToValueData, Boolean inDiacritical) const
{
	return CompareToSameKindPtr(inPtrToValueData, inDiacritical);
}


CompareResult VValue::Swap_CompareToSameKindPtr_Like( const void* inPtrToValueData, Boolean inDiacritical) const
{
	return InvertCompResult(CompareToSameKindPtr(inPtrToValueData, inDiacritical));
}


CompareResult VValue::CompareBeginingToSameKindPtr_Like( const void* inPtrToValueData, Boolean inDiacritical) const
{
	return CompareBeginingToSameKindPtr(inPtrToValueData, inDiacritical);
}


CompareResult VValue::IsTheBeginingToSameKindPtr_Like( const void* inPtrToValueData, Boolean inDiacritical) const
{
	return IsTheBeginingToSameKindPtr(inPtrToValueData, inDiacritical);
}


Boolean VValue::EqualToSameKindPtr_Like( const void* inPtrToValueData, Boolean inDiacritical) const
{
	return EqualToSameKindPtr(inPtrToValueData, inDiacritical);
}


Boolean VValue::Swap_EqualToSameKindPtr_Like( const void* inPtrToValueData, Boolean inDiacritical) const
{
	return EqualToSameKindPtr(inPtrToValueData, inDiacritical);
}


VError VValue::ReadFromStream(VStream* ioStream, sLONG /*inParam*/)
{
	return ioStream->GetLastError();
}


VError VValue::WriteToStream(VStream* ioStream, sLONG inParam) const
{
	return ioStream->GetLastError();
}


VError VValue::ReadRawFromStream( VStream* ioStream, sLONG inParam) //used by db4d server to read data without interpretation (for example : pictures)
{
	return ReadFromStream(ioStream, inParam);
}


/*
	A VValue may be created passing an adress to a buffer that
	may be used to avoid data copy.
	The given buffer is guaranteed to remain valid until the VValue is deleted.

	The VValue may decide to use this buffer to avoid the copy or not.
	If the VValue decide not to use the buffer and to use its own copy of the value instead,
	It must override VValue::Flush to update the buffer before the VValue is deleted.

	VValue::Flush is guaranteed to be called:
		- only if the constructor using a buffer pointer has been used
		- with the same buffer adress than the one passed in the constructor
*/
VError VValue::Flush(void* /*inDataPtr*/, void* /*inContext*/) const
{
	// Actually used only for Blobs
	return VE_OK;
}


void VValue::SetReservedBlobNumber(sLONG inBlobNum)
{
	// do nothing, this method is only usefull for blobs vvalues
}


const VValueInfo *VValue::GetValueInfo () const
{
	static VValueInfo_default<VEmpty,VK_UNDEFINED>	sInfo;
	assert( false);	// provide your own VValueInfo*
	return &sInfo;
}


/*
	static
*/
void VValue::RegisterValueInfo( const VValueInfo *inInfo)
{
	try
	{
//		assert( ValueInfoFromValueKind( inInfo->GetTrueKind()) == NULL);
		if (sInfos == NULL)
			sInfos = new VValueInfoIndex;
		
		(*sInfos)[inInfo->GetTrueKind()] = inInfo;
	}
	catch (...)
	{
		assert(false);
		// TRES TRES GROS probleme de memoire
	}
}


void VValue::UnregisterValueInfo( const VValueInfo *inInfo)
{
	try
	{
		sInfos->erase( inInfo->GetTrueKind());
	}
	catch (...)
	{
		assert(false);
	}
}


/*
	static
*/
const VValueInfo *VValue::ValueInfoFromValueKind( ValueKind inTrueKind)
{
	VValueInfoIndex::iterator found = sInfos->find( inTrueKind);

	return (found != sInfos->end()) ? found->second : NULL;
}



/*
	static
*/
VValue* VValue::NewValueFromValueKind( ValueKind inTrueKind)
{
	const VValueInfo *info = ValueInfoFromValueKind( inTrueKind);

	assert( info != NULL);
	
	return (info != NULL) ? info->Generate() : NULL;
}


void VValue::Detach(Boolean inMayEmpty) // used by blobs in DB4D
{
}


void VValue::RestoreFromPop(void* context)
{
}


VValue* VValue::FullyClone(bool ForAPush) const // no intelligent cloning (for example cloning by refcounting for Blobs)
{
	return Clone();
}


CompareResult VValue::InvertCompResult(CompareResult comp)
{
	if (comp == CR_BIGGER)
		return CR_SMALLER;
	else
	{
		if (comp == CR_SMALLER)
			return CR_BIGGER;
		else
			return comp;
	}
}


VError VValue::FromJSONString(const VString& inJSONString, JSONOption inModifier)
{
	return vThrowError( VE_UNIMPLEMENTED);
}


VError VValue::GetJSONString(VString& outJSONString, JSONOption inModifier) const
{
	return vThrowError( VE_UNIMPLEMENTED);
}


VError VValue::FromJSONValue( const VJSONValue& inJSONValue)
{
	return vThrowError( VE_UNIMPLEMENTED);
}


VError VValue::GetJSONValue( VJSONValue& outJSONValue) const
{
	outJSONValue.SetUndefined();
	return vThrowError( VE_UNIMPLEMENTED);
}

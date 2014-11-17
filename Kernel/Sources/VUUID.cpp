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
#include "VUUID.h"
#include "VStream.h"
#include "VMemoryCpp.h"
#include "VIntlMgr.h"
#include "VJSONValue.h"

BEGIN_TOOLBOX_NAMESPACE

//const VUUIDBuffer	kNULL_UUID_DATA	=   { 'i', 't', '\'', 's', ' ', 'a', ' ', 'n', 'u', 'l', 'l', ' ', 'u', 'u', 'i', 'd' };
const VUUIDBuffer	kNULL_UUID_DATA	=   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

const VUUID::TypeInfo	VUUID::sInfo;

const VUUID	VUUID::sNullUUID( kNULL_UUID_DATA);

VValue *VUUID_info::LoadFromPtr( const void *inBackStore, bool /*inRefOnly*/) const
{
	return new VUUID( *(const VUUIDBuffer *)inBackStore);
}


CompareResult VUUID_info::CompareTwoPtrToData(const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean /*inDiacritical*/) const
{
	int result = memcmp( inPtrToValueData1, inPtrToValueData2, 16 );

	if ( result < 0 )
		return CR_SMALLER;
	else if ( result > 0 )
		return CR_BIGGER;
	else
		return CR_EQUAL;
}


CompareResult VUUID_info::CompareTwoPtrToDataWithOptions(const void* inPtrToValueData1, const void* inPtrToValueData2, const VCompareOptions& /*inOptions*/) const
{
	int result = memcmp( inPtrToValueData1, inPtrToValueData2, 16 );

	if ( result < 0 )
		return CR_SMALLER;
	else if ( result > 0 )
		return CR_BIGGER;
	else
		return CR_EQUAL;
}


CompareResult VUUID_info::CompareTwoPtrToDataBegining(const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean inDiacritical) const
{
	return CompareTwoPtrToData( inPtrToValueData1, inPtrToValueData2, inDiacritical );
}


VSize VUUID_info::GetSizeOfValueDataPtr(const void* /*inPtrToValueData*/) const
{
	return 16;
}


VSize VUUID_info::GetAvgSpace() const
{
	return 16;
}


//=======================================================================================================================================


VUUID::VUUID() : VValueSingle(true)
{
	fCachedConvertedString = NULL;
	FromBuffer( kNULL_UUID_DATA );
}


VUUID::VUUID(Boolean inGenerate) : VValueSingle(!inGenerate)
{
	fCachedConvertedString = NULL;
	if (inGenerate)
		XUUIDImpl::Generate(&fData);
	else
		fData = kNULL_UUID_DATA;
}


VUUID::VUUID(const VUUIDBuffer& inUUID)
{
	fCachedConvertedString = NULL;
	FromBuffer(inUUID);
}


VUUID::VUUID(const char* inUUIDString)
{
	fCachedConvertedString = NULL;
	FromBuffer(inUUIDString);
}


VUUID::VUUID(const VUUID& inUUID):VValueSingle(inUUID.IsNull())
{
	fCachedConvertedString = NULL;
	fData = inUUID.fData;
}


#if VERSIONWIN
VUUID::VUUID(const GUID& inGUID)
{
	memcpy(&fData, &inGUID, sizeof(VUUIDBuffer));
	ByteSwapLong( (uLONG*) &fData.fBytes[0] );
	ByteSwapWord( (uWORD*) &fData.fBytes[4] );
	ByteSwapWord( (uWORD*) &fData.fBytes[6] );
	fCachedConvertedString = NULL;
}
#endif


VUUID::~VUUID()
{
	if (fCachedConvertedString != NULL)
		delete fCachedConvertedString;
}


void VUUID::Regenerate()
{
	XUUIDImpl::Generate(&fData);
	_GotValue();
}


bool VUUID::operator == (const VUUID& inUUID) const
{
	return   *(uLONG8*) &fData.fBytes[0] == *(uLONG8*) &inUUID.fData.fBytes[0]
		  && *(uLONG8*) &fData.fBytes[8] == *(uLONG8*) &inUUID.fData.fBytes[8];
}


bool VUUID::operator < (const VUUID& inUUID) const
{
	return (memcmp(&fData.fBytes[0], &inUUID.fData.fBytes[0], 16) < 0);
	/*
	if (*(uLONG8*) &fData.fBytes[0] == *(uLONG8*) &inUUID.fData.fBytes[0])
		return *(uLONG8*) &fData.fBytes[8] < *(uLONG8*) &inUUID.fData.fBytes[8];
	else
		return *(uLONG8*) &fData.fBytes[0] < *(uLONG8*) &inUUID.fData.fBytes[0];
	 */
}


bool VUUID::operator != (const VUUID& inUUID) const
{
	return   *(uLONG8*) &fData.fBytes[0] != *(uLONG8*) &inUUID.fData.fBytes[0]
		  || *(uLONG8*) &fData.fBytes[8] != *(uLONG8*) &inUUID.fData.fBytes[8];
}


VError VUUID::ReadFromStream(VStream* inStream, sLONG /*inParam*/)
{
	VSize len = 16;
	VError	error = inStream->GetData(&fData.fBytes[0], &len);
	_GotValue(memcmp(&fData, &kNULL_UUID_DATA, sizeof(VUUIDBuffer)) == 0);
	return error;
}


VError VUUID::WriteToStream(VStream* inStream, sLONG /*inParam*/) const
{
	return inStream->PutData(&fData.fBytes[0], 16);
}


VUUID* VUUID::Clone() const
{
	return new VUUID(fData);
}


const VValueInfo *VUUID::GetValueInfo() const
{
	return &sInfo;
}


void VUUID::GetValue( VValueSingle& outValue) const
{
	outValue.FromVUUID( *this);
}


void VUUID::DoNullChanged()
{
	if (IsNull())
		fData = kNULL_UUID_DATA;
}


void VUUID::_GotValue(bool inNull)
{
	GotValue(inNull);
	if (fCachedConvertedString != NULL)
	{
		GetString(*fCachedConvertedString);
		fCachedConvertedString->SetNull(inNull);
	}
}


VString* VUUID::GetCachedConvertedString(bool inCreateIfMissing)
{
	if (fCachedConvertedString == NULL)
	{
		if (inCreateIfMissing)
		{
			fCachedConvertedString = new VString();
			GetString(*fCachedConvertedString);
			fCachedConvertedString->SetNull(IsNull());
			
		}
	}
	return fCachedConvertedString;
}


CompareResult VUUID::CompareTo (const VValueSingle& inValue, Boolean /*inDiacritical*/) const
{
	if (!testAssert(inValue.GetValueKind() == VK_UUID))
		return CR_UNRELEVANT;
	
	return CompareToSameKind(&inValue);
}


CompareResult VUUID::CompareToSameKind(const VValue* inValue, Boolean /*inDiacritical*/) const
{
	XBOX_ASSERT_KINDOF(VUUID, inValue);
	const VUUID* uuid = reinterpret_cast<const VUUID*>(inValue);

	return sInfo.CompareTwoPtrToData( &fData.fBytes[0], &uuid->fData.fBytes[0], true );
}


Boolean VUUID::EqualToSameKind(const VValue* inValue, Boolean /*inDiacritical*/) const
{
	XBOX_ASSERT_KINDOF(VUUID, inValue);
	const VUUID* uuid = reinterpret_cast<const VUUID*>(inValue);

	return operator==(*uuid);
}


CompareResult VUUID::CompareToSameKindPtr(const void* inPtrToValueData, Boolean inDiacritical) const
{
	return sInfo.CompareTwoPtrToData( &fData.fBytes[0], inPtrToValueData, inDiacritical );
}


Boolean VUUID::EqualToSameKindPtr(const void* inPtrToValueData, Boolean /*inDiacritical*/) const
{
	return   *(uLONG8*) &fData.fBytes[0] == ( (uLONG8*) inPtrToValueData )[0]
		  && *(uLONG8*) &fData.fBytes[8] == ( (uLONG8*) inPtrToValueData )[1];
}



CompareResult VUUID::CompareToSameKindWithOptions( const VValue* inValue, const VCompareOptions& /*inOptions*/) const
{
	XBOX_ASSERT_KINDOF(VUUID, inValue);
	const VUUID* uuid = reinterpret_cast<const VUUID*>(inValue);

	int result = memcmp( &fData.fBytes[0], &uuid->fData.fBytes[0], 16 );

	if ( result < 0 )
		return CR_SMALLER;
	else if ( result > 0 )
		return CR_BIGGER;
	else
		return CR_EQUAL;
}


bool VUUID::EqualToSameKindWithOptions( const VValue* inValue, const VCompareOptions& /*inOptions*/) const
{
	XBOX_ASSERT_KINDOF(VUUID, inValue);
	const VUUID* uuid = reinterpret_cast<const VUUID*>(inValue);

	return operator==(*uuid);
}


CompareResult VUUID::CompareToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& /*inOptions*/) const
{
	int result = memcmp( &fData.fBytes[0], inPtrToValueData, 16 );

	if ( result < 0 )
		return CR_SMALLER;
	else if ( result > 0 )
		return CR_BIGGER;
	else
		return CR_EQUAL;
}


bool VUUID::EqualToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& /*inOptions*/) const
{
	return   *(uLONG8*) &fData.fBytes[0] == ( (uLONG8*) inPtrToValueData )[0]
		  && *(uLONG8*) &fData.fBytes[8] == ( (uLONG8*) inPtrToValueData )[1];
}


Boolean VUUID::FromValueSameKind(const VValue* inValue)
{
	XBOX_ASSERT_KINDOF(VUUID, inValue);
	const VUUID* uuid = reinterpret_cast<const VUUID*>(inValue);

	fData = uuid->fData;

	_GotValue(inValue->IsNull());
	return true;
}


void VUUID::Clear()
{
	fData = kNULL_UUID_DATA;
	_GotValue(true);
}


void VUUID::FromVUUID(const VUUID& inValue)
{
	fData = inValue.fData;
	_GotValue(inValue.IsNull());
}


void VUUID::GetVUUID(VUUID& outValue) const
{
	outValue.FromVUUID(*this);
}


void VUUID::FromBuffer(const VUUIDBuffer& inUUID)
{
	fData = inUUID;
	_GotValue(memcmp(&fData, &kNULL_UUID_DATA, sizeof(VUUIDBuffer)) == 0);
}


void VUUID::FromBuffer(const char* inUUIDString)
{
	size_t len = ::strlen(inUUIDString);

	if (len <= 16)
	{
		fData = *(VUUIDBuffer*) inUUIDString;
		
		char* ptr = (char*)&fData + len;
		for (;len<16;len++)
			*ptr++ = 0;
	}
	else if (testAssert( len == 32))
	{
		for( sLONG srcIndex = 0; srcIndex < 32 ; ++srcIndex)
		{
			uBYTE	digit = (uBYTE) inUUIDString[srcIndex];
			uBYTE	nibble;

			if (digit >= '0' && digit <= '9')
				nibble = (uBYTE) (digit - '0');
			else if (digit >= 'A' && digit <= 'F')
				nibble = (uBYTE) (10 + digit - 'A');
			else if (digit >= 'a' && digit <= 'f')
				nibble = (uBYTE) (10 + digit - 'a');
			else
			{
				assert( false);
				nibble = 0;
			}

			if ((srcIndex & 1) != 0)
			{
				fData.fBytes[srcIndex/2] += nibble;
			}
			else
			{
				fData.fBytes[srcIndex/2] = (uBYTE) (nibble * 16);
			}
		}
	}


	_GotValue(memcmp(&fData, &kNULL_UUID_DATA, sizeof(VUUIDBuffer)) == 0);
}


void VUUID::ToBuffer(VUUIDBuffer& outUUID) const
{
	outUUID = fData;
}


VSize VUUID::GetSpace(VSize /* inMax */) const
{
	return sizeof(fData);
}


void* VUUID::WriteToPtr(void* inData, Boolean /*inRefOnly*/, VSize /*inMax*/) const
{
	*(VUUIDBuffer*)inData = fData;
	return ((VUUIDBuffer*)inData)+1;
}


void* VUUID::LoadFromPtr(const void* inData, Boolean /*inRefOnly*/)
{
	fData = *(VUUIDBuffer*)inData;
	_GotValue(memcmp(&fData, &kNULL_UUID_DATA, sizeof(VUUIDBuffer)) == 0);
	return ((VUUIDBuffer*)inData)+1;
}


void VUUID::FromValue(const VValueSingle& inValue)
{
	inValue.GetVUUID(*this);
}


void VUUID::FromString(const VString& inValue)
{
	sLONG	destIndex = 0;

	// Don't care if it finds non Hexa characters, so you can parse
	// strings like "{19D215C0-3884-45af-A173-14F5CCD73C49}"
	for (sLONG srcIndex = 0; srcIndex < inValue.GetLength(); srcIndex++)
	{
		UniChar	digit = inValue[srcIndex];
		UniChar	nibble;

		if (digit >= CHAR_DIGIT_ZERO && digit <= CHAR_DIGIT_NINE)
			nibble = (UniChar) (digit - CHAR_DIGIT_ZERO);
		else if (digit >= CHAR_LATIN_CAPITAL_LETTER_A && digit <= CHAR_LATIN_CAPITAL_LETTER_F)
			nibble = (UniChar) (10 + digit - CHAR_LATIN_CAPITAL_LETTER_A);
		else if (digit >= CHAR_LATIN_SMALL_LETTER_A && digit <= CHAR_LATIN_SMALL_LETTER_F)
			nibble = (UniChar) (10 + digit - CHAR_LATIN_SMALL_LETTER_A);
		else
			continue;

		if ((destIndex & 1) != 0)
		{
			fData.fBytes[destIndex/2] += nibble;
		}
		else
		{
			fData.fBytes[destIndex/2] = (uBYTE) (nibble * 16);
		}
		if (++destIndex > 31)
			break;
	}
	
	if (destIndex == 32)
		_GotValue(false);
	else
	{
		BuildFromAnsiString(inValue);
		//SetNull();
	}
}


void VUUID::GetString(VString& outValue) const
{
	if (outValue.EnsureSize(32))
	{
		UniChar*	ptr = outValue.GetCPointerForWrite();
		for (sLONG index = 0 ; index < 16 ; ++index)
		{
			uWORD	nibble = (uWORD) (fData.fBytes[index] / 16);
			if (nibble < 10)
				ptr[index * 2] = (UniChar) (CHAR_DIGIT_ZERO + nibble);
			else
				ptr[index * 2] = (UniChar) (CHAR_LATIN_CAPITAL_LETTER_A + (nibble - 10));

			nibble = (uWORD) ((fData.fBytes[index] % 16));
			if (nibble < 10)
				ptr[index * 2 + 1] = (UniChar) (CHAR_DIGIT_ZERO + nibble);
			else
				ptr[index * 2 + 1] = (UniChar) (CHAR_LATIN_CAPITAL_LETTER_A + (nibble - 10));
		}

		outValue.Validate(32);
	}
}


VError VUUID::GetJSONString(VString& outJSONString, JSONOption inModifier) const
{
	VString s;
	GetString(s);
	return s.GetJSONString(outJSONString, inModifier);
}


VError VUUID::FromJSONValue( const VJSONValue& inJSONValue)
{
	if (inJSONValue.IsNull())
		SetNull( true);
	else
	{
		VString s;
		inJSONValue.GetString( s);
		FromString( s);
	}
	return VE_OK;
}


VError VUUID::GetJSONValue(VJSONValue& outJSONValue, bool /*allowDates*/) const
{
	if (IsNull())
		outJSONValue.SetNull();
	else
	{
		VString s;
		GetString(s);
		outJSONValue.SetString( s);
	}
	return VE_OK;
}

void VUUID::BuildFromAnsiString(const VString& inString)
{
	// Fill with spaces
	::memset(fData.fBytes, ' ', sizeof(fData.fBytes));

	VSize	bytesProduced;
	VIndex	charsConsumed;
	bool	isOK = VTextConverters::Get()->ConvertFromVString(inString, &charsConsumed, fData.fBytes, sizeof(fData.fBytes), &bytesProduced, VTC_StdLib_char);

	_GotValue();
}

END_TOOLBOX_NAMESPACE

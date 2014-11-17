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
#include "VString.h"
#include "VTextConverter.h"
#include "VMemoryCpp.h"
#include "VIntlMgr.h"
#include "VError.h"
#include "VStream.h"
#include "VUUID.h"
#include "VTime.h"
#include "VFloat.h"
#include "VArrayValue.h"
#include "VArray.h"
#include "VCollator.h"
#include "VValueBag.h"
#include "VJSONValue.h"
#include "Base64Coder.h"

#if VERSIONWIN
	#include <locale.h>
#else
	#include <xlocale.h>
#endif

BEGIN_TOOLBOX_NAMESPACE


inline VIndex CheckedCastToVIndex( size_t inValue)
{
	xbox_assert( (VIndex) inValue == inValue);
	return (VIndex) inValue;
}


// returns cstring length and detect if some non us-ascii char is inside
inline VIndex CStringLength( const char *inCString, bool *outUSASCII)
{
	char mask = 0;

	const char *p = inCString;
	while( *p)
		mask &= *p++;

	*outUSASCII = (mask & 0x80) == 0;
	return (VIndex) (p - inCString);
}


uLONG __VInlineString_tag = 0;

VOsTypeString::VOsTypeString( OsType inType )
{
	_AdjustPrivateBufferSize( 4);

#if SMALLENDIAN
	XBOX::ByteSwap( &inType);
#endif

	FromBlock(&inType, 4, VTC_StdLib_char);
}

const VString::InfoType	VString::sInfo;

VValue *VString_info::Generate() const
{
	return new VString;
}


VValue *VString_info::Generate(VBlob* inBlob) const
{
	return new VString;
}


VValue *VString_info::LoadFromPtr( const void *inBackStore, bool /*inRefOnly*/) const
{
	const UniChar* string = (reinterpret_cast<const UniChar*>(inBackStore))+2;
	VIndex len = *((uLONG*)inBackStore);
	VString *s = new VString;
	if (s)
		s->FromUniString(string, len);
	return s;
}


void* VString_info::ByteSwapPtr( void *inBackStore, bool inFromNative) const
{
	UniChar* string = (reinterpret_cast<UniChar*>(inBackStore))+2;
	uLONG len = *((uLONG*)inBackStore);
	if (!inFromNative)
		ByteSwap( &len);

	ByteSwap( (uLONG*) inBackStore);
	ByteSwap( string, string + len);

	return string + len + 2;
	
}


CompareResult VString_info::CompareTwoPtrToData( const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean inDiacritical) const
{
	return VIntlMgr::GetDefaultMgr()->CompareString(
		((UniChar*)inPtrToValueData1)+2, *((sLONG*)inPtrToValueData1), 
		((UniChar*)inPtrToValueData2)+2, *((sLONG*)inPtrToValueData2), inDiacritical != 0);
}


CompareResult VString_info::CompareTwoPtrToDataBegining( const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean inDiacritical) const
{
	sLONG len = *((sLONG*)inPtrToValueData1);
	if (len > *((sLONG*)inPtrToValueData2)) len = *((sLONG*)inPtrToValueData2);
	return VIntlMgr::GetDefaultMgr()->CompareString(((UniChar*)inPtrToValueData1)+2, len, ((UniChar*)inPtrToValueData2)+2, *((sLONG*)inPtrToValueData2), inDiacritical != 0);
}


CompareResult VString_info::CompareTwoPtrToData_Like( const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean inDiacritical) const
{
	return VIntlMgr::GetDefaultMgr()->CompareString_Like(
		((UniChar*)inPtrToValueData1)+2, *((sLONG*)inPtrToValueData1), 
		((UniChar*)inPtrToValueData2)+2, *((sLONG*)inPtrToValueData2), inDiacritical != 0);
}


CompareResult VString_info::CompareTwoPtrToDataBegining_Like( const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean inDiacritical) const
{
	sLONG len = *((sLONG*)inPtrToValueData1);
	if (len > *((sLONG*)inPtrToValueData2)) len = *((sLONG*)inPtrToValueData2);
	return VIntlMgr::GetDefaultMgr()->CompareString_Like(((UniChar*)inPtrToValueData1)+2, len, ((UniChar*)inPtrToValueData2)+2, *((sLONG*)inPtrToValueData2), inDiacritical != 0);
}


CompareResult VString_info::CompareTwoPtrToDataWithOptions( const void* inPtrToValueData1, const void* inPtrToValueData2, const VCompareOptions& inOptions) const
{
	if (inOptions.IsBeginsWith())
	{
		sLONG len = Min(*((sLONG*)inPtrToValueData1), *((sLONG*)inPtrToValueData2));
		if (inOptions.IsLike())
			return inOptions.GetIntlManager()->CompareString_Like( ((UniChar*)inPtrToValueData1)+2, len, ((UniChar*)inPtrToValueData2)+2, len, inOptions.IsDiacritical());
		else
			return inOptions.GetIntlManager()->CompareString( ((UniChar*)inPtrToValueData1)+2, len, ((UniChar*)inPtrToValueData2)+2, len, inOptions.IsDiacritical());
	}
	else
	{
		if (inOptions.IsLike())
			return inOptions.GetIntlManager()->CompareString_Like( ((UniChar*)inPtrToValueData1)+2, *((sLONG*)inPtrToValueData1), ((UniChar*)inPtrToValueData2)+2, *((sLONG*)inPtrToValueData2), inOptions.IsDiacritical());
		else
			return inOptions.GetIntlManager()->CompareString( ((UniChar*)inPtrToValueData1)+2, *((sLONG*)inPtrToValueData1), ((UniChar*)inPtrToValueData2)+2, *((sLONG*)inPtrToValueData2), inOptions.IsDiacritical());
	}
}


VSize VString_info::GetSizeOfValueDataPtr( const void* inPtrToValueData) const
{
	return sizeof(sLONG) + (*((sLONG*)inPtrToValueData))*2;
}


VSize VString_info::GetAvgSpace() const
{
	return 64;
}


//=======================================================================================================================================


VString::VString():VValueSingle( false)
{
	fMaxBufferLength = (sizeof(fBuffer) / sizeof(UniChar)) - 1; // zero
	fMaxLength = fMaxBufferLength;
	fString = fBuffer;
	fLength = 0;
	fString[0] = 0;
}


VString::VString( bool inNull):VValueSingle( inNull)
{
	fMaxBufferLength = (sizeof(fBuffer) / sizeof(UniChar)) - 1; // zero
	fMaxLength = fMaxBufferLength;
	fString = fBuffer;
	fLength = 0;
	fString[0] = 0;
}


VString::VString( UniChar* inUniCString, VSize inMaxBytes)
{
	xbox_assert(inMaxBytes == -1 || inMaxBytes >= 2);
	xbox_assert(inMaxBytes == -1 || inMaxBytes >= (UniCStringLength(inUniCString) + 1) * (VSize) sizeof(UniChar));
	
	_ClearStringPointerOwner();
	_ClearSharable();

	fMaxBufferLength = (sizeof(fBuffer) / sizeof(UniChar)) - 1;
	fString = inUniCString;

	fLength = UniCStringLength(inUniCString);
	fMaxLength = (inMaxBytes == -1) ? fLength : (VIndex) (((inMaxBytes / sizeof(UniChar)) - 1));
	xbox_assert(fMaxLength >= fLength);
}


#if !WCHAR_IS_UNICHAR

VString::VString( wchar_t* inUniCString, VSize inMaxBytes)
{
	xbox_assert(inMaxBytes == -1 || inMaxBytes >= 2);
	xbox_assert(inMaxBytes == -1 || inMaxBytes >= (UniCStringLength((UniChar*)inUniCString) + 1) * sizeof(UniChar));

	fMaxBufferLength = (sizeof(fBuffer) / sizeof(UniChar)) - 1;
	
	if (VTC_WCHAR == VTC_UTF_16)
	{
		_ClearStringPointerOwner();
		_ClearSharable();
		fString = (UniChar*)inUniCString;
		fLength = (VIndex) ::wcslen(inUniCString);
		fMaxLength = (inMaxBytes == -1) ? fLength : (VIndex) (((inMaxBytes / sizeof(UniChar)) - 1));
		xbox_assert(fMaxLength >= fLength);
	}
	else
	{
		fString = fBuffer;
		fLength = 0;
		fString[0] = 0;
		fMaxLength = fMaxBufferLength;

		VToUnicodeConverter_UTF32_Raw converter(false);
		converter.ConvertString(inUniCString, ::wcslen(inUniCString) * sizeof(wchar_t), NULL, *this);
	}
}
#endif	//!WCHAR_IS_UNICHAR


VString::VString( const VString& inString):VValueSingle( inString.IsNull())
{
	fMaxBufferLength = (sizeof(fBuffer) / sizeof(UniChar)) - 1;

	UniChar *retainedBuffer = inString._RetainBuffer();
	if (retainedBuffer != NULL)
	{
		fMaxLength = inString.fMaxLength;
		fString = retainedBuffer;
		fLength = inString.fLength;
	}
	else
	{
		fMaxLength = fMaxBufferLength;
		fString = fBuffer;
		fLength = 0;

		if (!inString.IsNull())
		{
			if (_EnsureSize( inString.GetLength(), false))
			{
				fLength = inString.GetLength();
				::memcpy( fString, inString.GetCPointer(), fLength * sizeof(UniChar));
			}
		}

		fString[fLength] = 0;
	}
}


VString::VString( UniChar inUniChar)
{
	fMaxBufferLength = (sizeof(fBuffer) / sizeof(UniChar)) - 1;
	fMaxLength = fMaxBufferLength;
	fString = fBuffer;
	fLength = 0;

	if (_EnsureSize(1, false))
		fString[fLength++] = inUniChar;
	
	fString[fLength] = 0;
}


VString::VString(const UniChar* inUniCString)
{
	fMaxBufferLength = (sizeof(fBuffer) / sizeof(UniChar)) - 1;
	fMaxLength = fMaxBufferLength;
	fString = fBuffer;
	fLength = 0;
	fString[fLength] = 0;

	VIndex len = UniCStringLength(inUniCString);
	if (_EnsureSize(len, false))
	{
		fLength = len;
		::memcpy( fString, inUniCString, fLength * sizeof(UniChar));
		fString[fLength] = 0;
	}
}


#if !WCHAR_IS_UNICHAR

VString::VString(const wchar_t* inUniCString)
{
	fMaxBufferLength = (sizeof(fBuffer) / sizeof(UniChar)) - 1;
	fMaxLength = fMaxBufferLength;
	fString = fBuffer;
	fLength = 0;
	fString[0] = 0;

	if (VTC_WCHAR == VTC_UTF_16)
	{
		size_t len =  ::wcslen(inUniCString);
		if (_EnsureSize( CheckedCastToVIndex( len), false))
		{
			fLength = CheckedCastToVIndex( len);
			::memcpy( fString, inUniCString, fLength * sizeof(UniChar));
			fString[fLength] = 0;
		}
	}
	else
	{
		VToUnicodeConverter_UTF32_Raw converter(false);
		converter.ConvertString(inUniCString, ::wcslen(inUniCString) * sizeof(wchar_t), NULL, *this);
	}
}

#endif	//!WCHAR_IS_UNICHAR


VString::VString(const char* inCString)
{
	fMaxBufferLength = (sizeof(fBuffer) / sizeof(UniChar)) -1;
	fMaxLength = fMaxBufferLength;
	fString = fBuffer;
	fLength = 0;
	fString[fLength] = 0;
	
	bool usascii;
	VIndex len = CStringLength( inCString, &usascii);
	if (usascii)
	{
		if (_EnsureSize( len, false))
		{
			UniChar *pString = fString;
			const char *p = inCString;
			while(*p)
				*pString++ = *p++;

			*pString = 0;
			fLength = pString - fString;
		}
	}
	else
	{
		VTextConverters::Get()->GetStdLibToUnicodeConverter()->ConvertString(inCString, len, NULL, *this);
	}
}


VString::VString(const void* inBuffer, VSize inNbBytes, CharSet inCharSet)
{
	fMaxBufferLength = (sizeof(fBuffer) / sizeof(UniChar)) - 1;
	fMaxLength = fMaxBufferLength;
	fString = fBuffer;
	fLength = 0;
	fString[fLength] = 0;

	FromBlock(inBuffer, inNbBytes, inCharSet);
}


VString::VString( UniChar* inUniCString, VIndex inLength, VSize inMaxBytes):VValueSingle( inUniCString == NULL)
{
	fMaxBufferLength = (sizeof(fBuffer) / sizeof(UniChar)) - 1;
	if (inUniCString != NULL)
	{
		xbox_assert(inUniCString[inLength] == 0);
		xbox_assert(inMaxBytes == -1 || inMaxBytes >= 2);
		xbox_assert(inMaxBytes == -1 || inMaxBytes >= (inLength + 1) * (VSize) sizeof(UniChar));
		
		_ClearStringPointerOwner();
		_ClearSharable();
		fString = inUniCString;

		fLength = inLength;
		fMaxLength = (inMaxBytes == -1) ? fLength : (VIndex) (((inMaxBytes / sizeof(UniChar)) - 1));
		xbox_assert(fMaxLength >= fLength);
	}
	else
	{
		fMaxLength = fMaxBufferLength;
		fString = fBuffer;
		fLength = 0;
		fString[0] = 0;
	}
}


VString::VString( const VInlineString& inString):VValueSingle( false)
{
	fMaxBufferLength = (sizeof(fBuffer) / sizeof(UniChar)) - 1; // zero
	fString = inString.RetainBuffer( &fLength, &fMaxLength);
	if (fString == NULL)
	{
		fMaxLength = fMaxBufferLength;
		fString = fBuffer;
		fLength = 0;
		fString[0] = 0;
	}
}


#if !WCHAR_IS_UNICHAR
VString::VString( size_t inLength, const wchar_t *inString) : VValueSingle( false)
{
	_ClearStringPointerOwner();
	_ClearSharable();
	fString = fBuffer;
	fMaxBufferLength = fLength = fMaxLength = (VIndex) inLength;
	UniChar *dst = fString;
	// we are given a C++ string litteral. So we can assume this is 7-bit ascii.
	for( const wchar_t *src = inString ; *src ; ++dst, ++src)
		*dst = (UniChar) *src;
	*dst = 0;
}
#endif



VString::~VString()
{
	_ReleaseBuffer();
}


VString* VString::NewString( VIndex inNbChars)
{
	VString* str = new( CheckedCastToVIndex( inNbChars*sizeof(UniChar)) ) VString;
	if (str != NULL)
		str->_AdjustPrivateBufferSize(inNbChars);
	return str;
}


bool VString::Validate( VIndex inNbChars)
{
	xbox_assert( !_IsShared());
	
	if (!testAssert(inNbChars < 0 || inNbChars <= fMaxLength))
		return false;
	
	fLength = inNbChars;
	fString[fLength] = 0;

	// ensure there's no extra null char inbetween
	// en fait les developpeurs 4D peuvent mettre des Char(0) dans leur chaine...
	// ca ne devrait pas poser de pb tant qu'on n'utilise pas GetCPointer()
//	xbox_assert(UniCStringLength(fString) == fLength);

	GotValue();
	return true;
}


UniChar	VString::GetUniChar( VIndex inIndex) const
{
	return ((testAssert(!IsNull() && inIndex >= 1 && inIndex <= fLength)) ? fString[inIndex - 1] : (UniChar) 0);
}


bool VString::SetUniChar( VIndex inIndex, UniChar inChar)
{
	if (!testAssert( !IsNull() && (inIndex >= 1) && (inIndex <= fLength) && _PrepareWriteKeepContent( fLength)))
		return false;

	fString[inIndex - 1] = inChar;
	return true;
}


VString& VString::AppendLong(sLONG inValue)
{
 	if (inValue == 0)
	{
		AppendUniCString(L"0");
	}
 	else if ( inValue == kMIN_sLONG )
	{
		AppendUniCString(L"-2147483648");
	}
	else
	{
		UniChar	temp[20];
		UniChar* current = &temp[20];
		*--current = 0;
		bool neg = inValue < 0;
		if (neg)
			inValue = -inValue;
		while(inValue > 0)
		{
			*--current = (UniChar) (CHAR_DIGIT_ZERO + inValue%10);
			inValue /= 10;
		}
		if (neg)
			*--current = CHAR_HYPHEN_MINUS;
		AppendUniCString(current);
	}
	return *this;
}


VString& VString::AppendLong8(sLONG8 inValue)
{
	if ( inValue == LLONG_MIN )
	{
		AppendUniCString(L"-9223372036854775808");
	}
	else if (inValue != 0)
 	{
		UniChar	temp[40];
		UniChar* current = &temp[40];
		*--current = 0;
		bool neg = inValue < 0;
		if (neg)
			inValue = -inValue;
		while(inValue > 0)
		{
			*--current = (UniChar) (CHAR_DIGIT_ZERO + (inValue%10L));
			inValue /= 10L;
		}
		if (neg)
			*--current = CHAR_HYPHEN_MINUS;
		AppendUniCString(current);
	}
	else
	{
		AppendUniCString(L"0");
	}
	return *this;
}

VString& VString::AppendULong8(uLONG8 inValue)
{
 	if (inValue != 0)
 	{
		UniChar	temp[40];
		UniChar* current = &temp[40];
		*--current = 0;
		while(inValue > 0)
		{
			*--current = (UniChar) (CHAR_DIGIT_ZERO + (inValue%10L));
			inValue /= 10L;
		}
		AppendUniCString(current);
	}
	else
	{
		AppendUniCString(L"0");
	}
	return *this;
}

VString& VString::AppendReal(Real inValue)
{
	char buff[255];
	sprintf(buff, "%.14G", inValue);
	return AppendCString(buff);
}


VString& VString::AppendOsType( OsType inType)
{
#if SMALLENDIAN
	XBOX::ByteSwap( &inType);
#endif
	return AppendBlock( &inType, 4, VTC_StdLib_char);
}


VString& VString::AppendValue(const VValueSingle& inValue)
{
	VStr255 str;
	inValue.GetString(str);
	AppendString(str);
	return *this;
}


CompareResult VString::CompareTo (const VValueSingle& inValue, Boolean inDiacritical) const
{
	VStr255 str;
	inValue.GetString(str);
	return VIntlMgr::GetDefaultMgr()->CompareString(GetCPointer(), GetLength(), str.GetCPointer(), str.GetLength(), inDiacritical != 0);
}


CompareResult VString::CompareToSameKind(const VValue* inValue, Boolean inDiacritical) const
{
	XBOX_ASSERT_KINDOF(VString, inValue);
	const VString* theString = reinterpret_cast<const VString*>(inValue);
	return VIntlMgr::GetDefaultMgr()->CompareString(GetCPointer(), GetLength(), theString->GetCPointer(), theString->GetLength(), inDiacritical != 0);
}


CompareResult VString::CompareBeginingToSameKind(const VValue* inValue, Boolean inDiacritical) const
{
	XBOX_ASSERT_KINDOF(VString, inValue);
	const VString* theString = reinterpret_cast<const VString*>(inValue);
	VIndex len = GetLength();
	if (len > theString->GetLength())
		len = theString->GetLength();
	return VIntlMgr::GetDefaultMgr()->CompareString(GetCPointer(), len, theString->GetCPointer(), theString->GetLength(), inDiacritical != 0);
}


bool VString::EqualToUSASCIICString( const char *inUSASCIICString) const
{
	const UniChar *uniPtr = fString;
	for(  ; *inUSASCIICString != 0 ; ++inUSASCIICString, ++uniPtr)
	{
		if (*uniPtr != (UniChar) *inUSASCIICString)
			return false;
	}
	// we exited the loop because we reached the ascii '\0', so must also have reached the null unichar
	return *uniPtr == 0;
}


bool VString::EqualToString(const VString& inString, bool inDiacritical) const
{
	return VIntlMgr::GetDefaultMgr()->EqualString( GetCPointer(), GetLength(), inString.GetCPointer(), inString.GetLength(), inDiacritical);
}


bool VString::EqualToString_Like(const VString& inPattern, bool inDiacritical) const
{
	return VIntlMgr::GetDefaultMgr()->EqualString_Like( GetCPointer(), GetLength(), inPattern.GetCPointer(), inPattern.GetLength(), inDiacritical);
}


bool VString::EqualToStringRaw( const VString& inString) const
{
	return (fLength == inString.fLength) && (memcmp( GetCPointer(), inString.GetCPointer(), fLength * sizeof( UniChar)) == 0);
}


/*
	static
*/
sLONG VString::FindRawString( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize)
{
	// see http://fr.wikipedia.org/wiki/Algorithme_de_Knuth-Morris-Pratt
	
	sLONG targetBuffer[256];
	sLONG *retarget = (inPatternSize < 256) ? targetBuffer : (sLONG*) malloc( sizeof( sLONG) * (inPatternSize + 1));

	{
		sLONG i = 0;
		sLONG j = -1;
		UniChar c = 0;
		
		retarget[0] = j;
		while( i != inPatternSize)
		{
			if (inPattern[i] == c)
			{
				retarget[i + 1] = j + 1;
				++j;
				++i;
			}
			else if (j > 0)
			{
				j = retarget[j];
			}
			else
			{
				retarget[i + 1] = 0;
				++i;
				j = 0;
			}
			c = inPattern[j];
		}
	}

	sLONG m = 0;
	sLONG i = 0;
	while( (m + i != inTextSize) && (i != inPatternSize))
	{
		if (inText[m + i] == inPattern[i])
		{
			++i;
		}
		else
		{
			m += i - retarget[i];
			if (i > 0)
				i = retarget[i];
		}
	}
	
	if (retarget != targetBuffer)
		free( retarget);

	if (i >= inPatternSize)
		return m + 1;
	else
		return 0;
}


void VString::ExchangeRawString( const VString& inStringToFind, const VString& inStringToInsert, VIndex inPlaceToStart, VIndex inCountToReplace)
{
	VIndex morelen = inStringToInsert.GetLength();
	while(inCountToReplace-- && inPlaceToStart <= GetLength())
	{
		VIndex pos = FindRawString( GetCPointer() + inPlaceToStart - 1, GetLength() - inPlaceToStart + 1, inStringToFind.GetCPointer(), inStringToFind.GetLength());
		if (pos <= 0)
			break;
		Replace( inStringToInsert, pos + inPlaceToStart - 1, inStringToFind.GetLength());
		inPlaceToStart = pos + inPlaceToStart - 1 + morelen;
	}
}


CompareResult VString::CompareToSameKindPtr(const void* inPtrToValueData, Boolean inDiacritical) const
{
	return VIntlMgr::GetDefaultMgr()->CompareString(GetCPointer(), GetLength(), ((UniChar*)inPtrToValueData)+2, *((sLONG*)inPtrToValueData), inDiacritical != 0);
}


CompareResult VString::Swap_CompareToSameKindPtr(const void* inPtrToValueData, Boolean inDiacritical) const
{
	return VIntlMgr::GetDefaultMgr()->CompareString(((UniChar*)inPtrToValueData)+2, *((sLONG*)inPtrToValueData), GetCPointer(), GetLength(), inDiacritical != 0);
}


Boolean VString::EqualToSameKindPtr(const void* inPtrToValueData, Boolean inDiacritical) const
{
	return VIntlMgr::GetDefaultMgr()->EqualString(GetCPointer(), GetLength(), ((UniChar*)inPtrToValueData)+2, *((sLONG*)inPtrToValueData), inDiacritical != 0);
}


CompareResult VString::CompareBeginingToSameKindPtr(const void* inPtrToValueData, Boolean inDiacritical) const
{
	sLONG len = GetLength();
	if (len > *((sLONG*)inPtrToValueData)) len = *((sLONG*)inPtrToValueData);
	return VIntlMgr::GetDefaultMgr()->CompareString(GetCPointer(), len, ((UniChar*)inPtrToValueData)+2, *((sLONG*)inPtrToValueData), inDiacritical != 0);
}


CompareResult VString::IsTheBeginingToSameKindPtr(const void* inPtrToValueData, Boolean inDiacritical) const
{
	sLONG	len = Min(*((sLONG*)inPtrToValueData), (sLONG)GetLength());
	return VIntlMgr::GetDefaultMgr()->CompareString(GetCPointer(), GetLength(), ((UniChar*)inPtrToValueData)+2, len, inDiacritical != 0);
}

/****/

CompareResult VString::CompareToSameKind_Like(const VValue* inValue, Boolean inDiacritical) const
{
	XBOX_ASSERT_KINDOF(VString, inValue);
	const VString* theString = reinterpret_cast<const VString*>(inValue);
	return VIntlMgr::GetDefaultMgr()->CompareString_Like(GetCPointer(), GetLength(), theString->GetCPointer(), theString->GetLength(), inDiacritical != 0);
}


CompareResult VString::CompareBeginingToSameKind_Like(const VValue* inValue, Boolean inDiacritical) const
{
	XBOX_ASSERT_KINDOF(VString, inValue);
	const VString* theString = reinterpret_cast<const VString*>(inValue);
	VIndex len = GetLength();
	if (len > theString->GetLength())
		len = theString->GetLength();
	return VIntlMgr::GetDefaultMgr()->CompareString_Like(GetCPointer(), len, theString->GetCPointer(), theString->GetLength(), inDiacritical != 0);
}



CompareResult VString::CompareToSameKindPtr_Like(const void* inPtrToValueData, Boolean inDiacritical) const
{
	return VIntlMgr::GetDefaultMgr()->CompareString_Like(GetCPointer(), GetLength(), ((UniChar*)inPtrToValueData)+2, *((sLONG*)inPtrToValueData), inDiacritical != 0);
}


CompareResult VString::Swap_CompareToSameKindPtr_Like(const void* inPtrToValueData, Boolean inDiacritical) const
{
	return VIntlMgr::GetDefaultMgr()->CompareString_Like(((UniChar*)inPtrToValueData)+2, *((sLONG*)inPtrToValueData), GetCPointer(), GetLength(), inDiacritical != 0);
}


Boolean VString::EqualToSameKindPtr_Like(const void* inPtrToValueData, Boolean inDiacritical) const
{
	return VIntlMgr::GetDefaultMgr()->EqualString_Like(GetCPointer(), GetLength(), ((UniChar*)inPtrToValueData)+2, *((sLONG*)inPtrToValueData), inDiacritical != 0);
}


Boolean VString::Swap_EqualToSameKindPtr_Like(const void* inPtrToValueData, Boolean inDiacritical) const
{
	return VIntlMgr::GetDefaultMgr()->EqualString_Like(((UniChar*)inPtrToValueData)+2, *((sLONG*)inPtrToValueData), GetCPointer(), GetLength(), inDiacritical != 0);
}

CompareResult VString::CompareBeginingToSameKindPtr_Like(const void* inPtrToValueData, Boolean inDiacritical) const
{
	sLONG len = GetLength();
	if (len > *((sLONG*)inPtrToValueData)) len = *((sLONG*)inPtrToValueData);
	return VIntlMgr::GetDefaultMgr()->CompareString_Like(GetCPointer(), len, ((UniChar*)inPtrToValueData)+2, *((sLONG*)inPtrToValueData), inDiacritical != 0);
}


CompareResult VString::IsTheBeginingToSameKindPtr_Like(const void* inPtrToValueData, Boolean inDiacritical) const
{
	sLONG	len = Min(*((sLONG*)inPtrToValueData), (sLONG)GetLength());
	return VIntlMgr::GetDefaultMgr()->CompareString_Like(GetCPointer(), GetLength(), ((UniChar*)inPtrToValueData)+2, len, inDiacritical != 0);
}


/****/


CompareResult VString::CompareToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const
{
	XBOX_ASSERT_KINDOF(VString, inValue);
	const VString* theString = reinterpret_cast<const VString*>(inValue);

	if (inOptions.IsBeginsWith())
	{
		VIndex len = GetLength();
		if (len > theString->GetLength())
			len = theString->GetLength();
		if (inOptions.IsLike())
			return inOptions.GetIntlManager()->CompareString_Like( GetCPointer(), len, theString->GetCPointer(), theString->GetLength(), inOptions.IsDiacritical());
		else
			return inOptions.GetIntlManager()->CompareString( GetCPointer(), len, theString->GetCPointer(), theString->GetLength(), inOptions.IsDiacritical());
	}
	else
	{
		if (inOptions.IsLike())
			return inOptions.GetIntlManager()->CompareString_Like( GetCPointer(), GetLength(), theString->GetCPointer(), theString->GetLength(), inOptions.IsDiacritical());
		else
			return inOptions.GetIntlManager()->CompareString( GetCPointer(), GetLength(), theString->GetCPointer(), theString->GetLength(), inOptions.IsDiacritical());
	}
}


bool VString::EqualToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const
{
	XBOX_ASSERT_KINDOF(VString, inValue);
	const VString* theString = reinterpret_cast<const VString*>(inValue);
	if (inOptions.IsLike())
		return inOptions.GetIntlManager()->EqualString_Like( GetCPointer(), GetLength(), theString->GetCPointer(), theString->GetLength(), inOptions.IsDiacritical());
	else
		return inOptions.GetIntlManager()->EqualString( GetCPointer(), GetLength(), theString->GetCPointer(), theString->GetLength(), inOptions.IsDiacritical());
}


CompareResult VString::CompareToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const
{
	if (inOptions.IsBeginsWith())
	{
		sLONG len = Min(*((sLONG*)inPtrToValueData), (sLONG)GetLength());
		if (inOptions.IsLike())
			return inOptions.GetIntlManager()->CompareString_Like( GetCPointer(), len, ((UniChar*)inPtrToValueData)+2, len, inOptions.IsDiacritical());
		else
			return inOptions.GetIntlManager()->CompareString( GetCPointer(), len, ((UniChar*)inPtrToValueData)+2, len, inOptions.IsDiacritical());
	}
	else
	{
		if (inOptions.IsLike())
			return inOptions.GetIntlManager()->CompareString_Like( GetCPointer(), GetLength(), ((UniChar*)inPtrToValueData)+2, *((sLONG*)inPtrToValueData), inOptions.IsDiacritical());
		else
			return inOptions.GetIntlManager()->CompareString( GetCPointer(), GetLength(), ((UniChar*)inPtrToValueData)+2, *((sLONG*)inPtrToValueData), inOptions.IsDiacritical());
	}
}


CompareResult VString::Swap_CompareToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const
{
	if (inOptions.IsBeginsWith())
	{
		sLONG len = Min(*((sLONG*)inPtrToValueData), (sLONG)GetLength());
		if (inOptions.IsLike())
			return inOptions.GetIntlManager()->CompareString_Like( ((UniChar*)inPtrToValueData)+2, len, GetCPointer(), len, inOptions.IsDiacritical());
		else
			return inOptions.GetIntlManager()->CompareString( ((UniChar*)inPtrToValueData)+2, len, GetCPointer(), len, inOptions.IsDiacritical());
	}
	else
	{
		if (inOptions.IsLike())
			return inOptions.GetIntlManager()->CompareString_Like( ((UniChar*)inPtrToValueData)+2, *((sLONG*)inPtrToValueData), GetCPointer(), GetLength(), inOptions.IsDiacritical());
		else
			return inOptions.GetIntlManager()->CompareString( ((UniChar*)inPtrToValueData)+2, *((sLONG*)inPtrToValueData), GetCPointer(), GetLength(), inOptions.IsDiacritical());
	}
}


bool VString::EqualToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const
{
	if (inOptions.IsLike())
		return inOptions.GetIntlManager()->EqualString_Like( GetCPointer(), GetLength(), ((UniChar*)inPtrToValueData)+2, *((sLONG*)inPtrToValueData), inOptions.IsDiacritical());
	else
		return inOptions.GetIntlManager()->EqualString( GetCPointer(), GetLength(), ((UniChar*)inPtrToValueData)+2, *((sLONG*)inPtrToValueData), inOptions.IsDiacritical());
}



/****/


UniChar *VString::_RetainBuffer() const
{
	if (!_IsBufferRefCounted() || IsNull())
		return NULL;

	xbox_assert( *_GetBufferRefCountPtr() > 0);
	VInterlocked::Increment( _GetBufferRefCountPtr());
	return fString;
}


void VString::_ReleaseBuffer() const
{
	if (_IsBufferRefCounted())
	{
		xbox_assert( *_GetBufferRefCountPtr() > 0);
		sLONG newCount = VInterlocked::Decrement( _GetBufferRefCountPtr());
		if (newCount == 0)
		{
			VCppMemMgr* allocator = _GetBufferAllocator();
			if (allocator != NULL)
				allocator->Free( fString);
		}
	}
	else if ((fString != fBuffer) && _IsStringPointerOwner())
	{
		VCppMemMgr* allocator = _GetBufferAllocator();
		if (allocator != NULL)
			allocator->Free( fString);
	}
}


bool VString::_EnlargeBuffer( VIndex inNbChars, bool inCopyCharacters)
{
	// may be called only to increase the buffer size.
	xbox_assert(inNbChars > fLength);
	
	if (_IsFixedSize())
	{
		// fixed size string
		vThrowError( VE_STRING_RESIZE_FIXED_STRING);
		return false;
	}
	
	// we assume the current string block is bigger than the private buffer.
	// that may not be the case when building a VString passing a private buffer smaller than our own.
	// but this is not a real issue, just a failure at optimizing mem usage.
	
	VIndex newSize = inNbChars;

	// enlarge 50% when > 128 but not for the first allocation: allocate exact size because most of the time VStrings are const.
	if ( (newSize >= 128) && (fString != fBuffer) )
		newSize += newSize/2;
		
	return _ReallocBuffer( newSize, inCopyCharacters);
}


bool VString::_ReallocBuffer( VIndex inNbChars, bool inCopyCharacters)
{
	// final string length may not increase but it may decrease.
	VIndex newLength = inCopyCharacters ? Min( fLength, inNbChars) : 0;

	// see if we may use our private buffer
	if (inNbChars <= fMaxBufferLength)
	{
		if (fString != fBuffer)
		{
			::memcpy( fBuffer, fString, newLength * sizeof(UniChar));
			_ReleaseBuffer();
			fString = fBuffer;
			fString[newLength] = 0;
			fMaxLength = fMaxBufferLength;
		}
	}
	else
	{
		UniChar* newBuffer;

		// extra bytes for trailing null UniChar
		VIndex newSize = inNbChars + 1;

		VCppMemMgr* allocator = _GetBufferAllocator();
		if (allocator != NULL)
		{
			// increase 50% to speed up progressive enlarging. Except for first allocation.
			// VString currently cannot shrink so be careful...
			VIndex bonus = (fLength == 0 || fLength == inNbChars) ? 0 : std::max<VIndex>( 32, (newSize / 2));
			bonus = std::min<VIndex>( 10*1024*1024L, bonus);	// no more than 10Mo

			// extra bytes for refcount (2 unichars to make a sLONG)
			if (bonus < 2)
				bonus = 2;

			newSize += bonus;
			
			// 16 bytes rounding (match mac stdlib and xbox allocators rounding)
			// now we can be sure that the last 4 bytes which contain the refcount are 4 bytes aligned.
			newSize = (newSize + 15) & -16;

			newBuffer = (UniChar*) allocator->NewPtr( newSize * sizeof(UniChar), false, 'str ');

			if (newBuffer != NULL)
			{
				::memcpy( newBuffer, fString, newLength * sizeof(UniChar));
				_ReleaseBuffer();
				_SetStringPointerOwner();
				_SetSharable();
				fString = newBuffer;
				fString[newLength] = 0;
				fMaxLength = newSize - 3;
				*_GetBufferRefCountPtr() = 1;
			}
			else
			{
				vThrowError( VE_STRING_ALLOC_FAILED);
			}
		}
		else
		{
			// no allocator is available: we assume this is a C++ global variable
			// we use malloc and we won't dispose it.
			// the string won't be sharable.
			newBuffer = (UniChar*) malloc( newSize * sizeof(UniChar));
			if (newBuffer != NULL)
			{
				::memcpy( newBuffer, fString, newLength * sizeof(UniChar));
				xbox_assert( (fString == fBuffer) || !_IsStringPointerOwner());	// cause we don't have any allocator to dispose the buffer (and it can't be sharable so soon)
				_ClearStringPointerOwner();
				_ClearSharable();
				fString = newBuffer;
				fString[newLength] = 0;
				fMaxLength = newSize - 1;
			}
		}
	}
	
	return (inNbChars <= fMaxLength);
}


static void _AdjustPrintfFormat( const char* inFormat, char *inNewFormat, size_t inNewFormatSize, std::vector<VString>& outParams, va_list inArgList)
{
	// comme sprintf en reconnaissant certains tags prives
#if (VERSIONMAC || VERSION_LINUX) && ARCH_64
	// sc 27/05/2010 do not use inArgList directly to keep it clean, may be enabled for 32 bits architecture. Note: va_copy does not exist on Windows
	va_list argListCopy;
    va_copy( argListCopy, inArgList);
	char* curArg = va_arg( argListCopy, char*);
#else
	char* curArg = va_arg( inArgList, char*);
#endif
	const char* curArgPos = ::strchr(inFormat, '%');

	// on remplace les '%S' par des ';;;%p;;;' qu'on remplace apres par des chaines vides
	inNewFormat[0] = 0;
	const char* oldP = inFormat;
	int advance = 1;
	const char* ptr;
	for(ptr = ::strchr(inFormat, '%') ; ptr != NULL && oldP[0] != 0 ; ptr = ::strchr(oldP+advance, '%'))
	{	
		::strncat(inNewFormat, oldP, ptr-oldP);
		oldP = ptr;
		advance = 1;
		switch(ptr[1])
		{
			case 'S':	// VString*
			case 'V':	// VObject* (dump)
			case 'A':	// VValueSingle* (GetString)
				{
					VString temp;
					::strncat(inNewFormat, ";;;%p;;;", (inNewFormatSize - ::strlen(inNewFormat)) - 1);
					VObject* theVObject = reinterpret_cast<VObject*>(VObject::GetMainMemMgr()->CheckVObject(curArg));
					if (theVObject == NULL)
					{
						temp = L"@@invalid VObject@@";
					}
					else
					{
						switch(ptr[1])
						{
							case 'S':	// VString*
								{
									VString* theVString = dynamic_cast<VString*>( theVObject);
									if (theVString == NULL)
										temp = L"@@invalid VString@@";
									else
										temp = *theVString;
									break;
								}
								
							case 'V':	// VObject* (dump)
								{
									theVObject->DebugDump(temp);
									break;
								}
								
							case 'A':	// VValueSingle* (GetString)
								{
									VValueSingle* theValue = dynamic_cast<VValueSingle*>( theVObject);
									if (theValue == NULL)
										temp = L"@@invalid VValueSingle@@";
									else
										theValue->GetString(temp);
									break;
								}
						}
					}
					outParams.push_back( temp);
					oldP += 2;
					advance = 0;
                #if (VERSIONMAC || VERSION_LINUX) && ARCH_64
					curArg = va_arg(argListCopy, char*);
				#else
					curArg = va_arg(inArgList, char*);
				#endif
					break;
				}
			
			case '%':
				break;
			
			case '*':	// means next argument is a field size
            #if (VERSIONMAC || VERSION_LINUX) && ARCH_64
				curArg = va_arg(argListCopy, char*);
				curArg = va_arg(argListCopy, char*);
			#else
				curArg = va_arg(inArgList, char*);
				curArg = va_arg(inArgList, char*);
			#endif
				break;
			
			default:
            #if (VERSIONMAC || VERSION_LINUX) && ARCH_64
				curArg = va_arg(argListCopy, char*);
			#else
				curArg = va_arg(inArgList, char*);
			#endif
				break;
				
		}

	}
	::strncat( inNewFormat, oldP, (inNewFormatSize - ::strlen(inNewFormat)) - 1);
	
#if (VERSIONMAC || VERSION_LINUX) && ARCH_64
    va_end( argListCopy);
#endif
}


void VString::_Printf(const char* inFormat, va_list inArgList)
{
	// comme sprintf en reconnaissant certains tags prives

	std::vector<VString> params;
	char	format[1024L];
	_AdjustPrintfFormat( inFormat, format, sizeof( format), params, inArgList);

	char	buffer[1024L];
	#if VERSIONMAC || VERSION_LINUX // M.B 04/07/01
		#if ARCH_64
			// sc 27/05/2010 vsnprintf() function modify the list so we use a copy to keep it clean, may be enabled for 32 bits architecture. Note: va_copy does not exist on Windows
			va_list argListCopy;
			va_copy( argListCopy, inArgList);
			int count = vsnprintf( buffer, sizeof(buffer), format, argListCopy);
			va_end( argListCopy);
		#else // ARCH_64
			int count = vsnprintf( buffer, sizeof(buffer), format, inArgList);
		#endif
	#elif VERSIONWIN
		int count = _vsnprintf( buffer, sizeof(buffer), format, inArgList);
	#endif
	
	// first appended char pos
	sLONG where = GetLength() + 1;

	AppendCString(buffer);
	
	// on remplace les patterns ';;;xxxxxxxx;;;'
	for( size_t pos = 0 ; pos < params.size() ; ++pos)
	{
		sLONG start = Find(CVSTR(";;;"), where);
		sLONG end = Find(CVSTR(";;;"), start + 3) + 3;
		Replace( params[pos], start, end - start);
		where = start + params[pos].GetLength();
	}
}


VString& VString::AppendPrintf(const char* inMessage, ...)
{
	va_list argList;
	va_start(argList, inMessage);
	_Printf(inMessage, argList);
	va_end(argList);
	return *this;
}


void VString::VPrintf(const char* inMessage, va_list argList)
{
	Truncate(0);
	_Printf(inMessage, argList);
}


VString& VString::Format(const VValueBag *inParameters, bool inClearUnresolvedReferences)
{
	if (inParameters != NULL && !inParameters->IsEmpty())
	{
		// replace attributes.
		// each {tag} sequence is tentatively replaced by a matching attribute or 'n/a' if none matches.
		// to allow the use of { or } in error messages, a tag must not include any space.
		
		VIndex pos_current = 1;
		while( pos_current < GetLength())
		{
			VIndex pos_left = FindUniChar( CHAR_LEFT_CURLY_BRACKET, pos_current);
			if (pos_left <= 0)
				break;

			if(pos_left + 1 < GetLength()){ //escape character ( {{ )
				VIndex pos_left2 = FindUniChar( CHAR_LEFT_CURLY_BRACKET, pos_left + 1);
				if(pos_left + 1 == pos_left2){ 
					pos_current = pos_left2 + 1;
					continue;
				}
			}

			VIndex pos_right = FindUniChar( CHAR_RIGHT_CURLY_BRACKET, pos_left);
			if (pos_right <= 0)
				break;
			
			if(pos_right + 1 < GetLength()){
				VIndex pos_right2 = FindUniChar( CHAR_RIGHT_CURLY_BRACKET, pos_right + 1);
				if(pos_right + 1 == pos_right2){ //escape character ( }} )
					pos_current = pos_right2 + 1;
					continue;
				}
			}

			// no space allowed in tag
			VIndex pos_space = FindUniChar( CHAR_SPACE, pos_left);
			if ((pos_right != pos_left + 1) && ((pos_space <= 0) || (pos_space > pos_right)) )
			{
				VStr15 tag;
				GetSubString(  pos_left + 1, pos_right - pos_left - 1, tag);

				VStr31 replacement;

				// look for matching attribute
				const VValueSingle*	value = inParameters->GetAttribute( tag);
				if (value != NULL)
				{
					value->GetString( replacement);
					Replace( replacement, pos_left, pos_right - pos_left + 1);
					pos_current = pos_left + replacement.GetLength();
				}
				else if (inClearUnresolvedReferences)
				{
					Remove( pos_left, pos_right - pos_left + 1);
					pos_current = pos_left;
				}
				else
				{
					pos_current = pos_left + 1;
				}
			}
			else
			{
				pos_current = pos_left + 1;
			}
		}
	}

	return *this;
}


CompareResult VString::CompareToString(const VString& inValue, bool inDiacritical) const
{
	return VIntlMgr::GetDefaultMgr()->CompareString(GetCPointer(), GetLength(), inValue.GetCPointer(), inValue.GetLength(), inDiacritical);
}


CompareResult VString::CompareToString_Like(const VString& inPattern, bool inDiacritical) const
{
	return VIntlMgr::GetDefaultMgr()->CompareString_Like( GetCPointer(), GetLength(), inPattern.GetCPointer(), inPattern.GetLength(), inDiacritical);
}


const VValueInfo *VString::GetValueInfo() const
{
	return &sInfo;
}


VSize VString::GetSpace(VSize inMax) const
{
	if (inMax == 0)
		return (fLength + 2) * sizeof(UniChar);
	else
	{
		VSize res = (fLength + 2) * sizeof(UniChar);
		if (res > inMax)
			res = inMax;
		return res;
	}
}


Boolean VString::FromValueSameKind( const VValue* inValue)
{
	XBOX_ASSERT_KINDOF(VString, inValue);
	const VString* theString = reinterpret_cast<const VString*>(inValue);
	FromString(*theString);
	return true;
}


void VString::GetValue( VValueSingle& inValue) const
{
	inValue.FromString(*this);
}


UniChar *VString::RetainBuffer( VIndex *outLength, VIndex *outMaxLength) const
{
	UniChar *retainedBuffer;
	if (!IsEmpty())
	{
		retainedBuffer = _RetainBuffer();
		if (retainedBuffer == NULL)
		{
			// 3 extra chars for trailing null UniChar and refcount,
			// add 1 to be sure that the last 4 bytes which contain the refcount are 4 bytes aligned.
			VSize newSize = (fLength + 4L) & 0xFFFFFFFE;

			retainedBuffer = (UniChar*) _GetBufferAllocator()->NewPtr( newSize * sizeof(UniChar), false, 'strX');
			if (retainedBuffer != NULL)
			{
				::memcpy( retainedBuffer, fString, (fLength + 1) * sizeof(UniChar));
				*(sLONG*) (retainedBuffer + ((fLength + 2) & 0xFFFFFFFE)) = 1;	// set refcount
				*outLength = *outMaxLength = fLength;
			}
			else
			{
				*outLength = *outMaxLength = 0;
			}
		}
		else
		{
			*outLength = fLength;
			*outMaxLength = fMaxLength;
		}
	}
	else
	{
		retainedBuffer = NULL;
		*outLength = *outMaxLength = 0;
	}
	return retainedBuffer;
}


void* VString::WriteToPtr(void* ioData, Boolean /*inRefOnly*/, VSize inMax) const
{
	VIndex len = fLength;
	if (inMax > 0 && (((len*sizeof(UniChar))+4) > inMax))
		len = (VIndex) ((inMax-4)/sizeof(UniChar));
	*((uLONG*)ioData) = len;
	::memcpy( ((UniChar*)ioData)+2, fString, len*sizeof(UniChar));
	return ((UniChar*) ioData)+ len + 2;
}


void* VString::LoadFromPtr(const void* inData, Boolean /*inRefOnly*/)
{
	const UniChar* string = (reinterpret_cast<const UniChar*>(inData))+2;
	uLONG len = *((uLONG*)inData);
	FromUniString(string, len);
	return ((UniChar*) inData)+ len + 2;
}


Boolean VString::EqualToSameKind(const VValue* inValue, Boolean inDiacritical) const
{
	XBOX_ASSERT_KINDOF(VString, inValue);
	const VString* theString = reinterpret_cast<const VString*>(inValue);
	return VIntlMgr::GetDefaultMgr()->EqualString(GetCPointer(), GetLength(), theString->GetCPointer(), theString->GetLength(), inDiacritical != 0);
}


Boolean VString::EqualToSameKind_Like(const VValue* inValue, Boolean inDiacritical) const
{
	XBOX_ASSERT_KINDOF(VString, inValue);
	const VString* theString = reinterpret_cast<const VString*>(inValue);
	return VIntlMgr::GetDefaultMgr()->EqualString_Like(GetCPointer(), GetLength(), theString->GetCPointer(), theString->GetLength(), inDiacritical != 0);
}


void VString::Printf(const char* inMessage, ...)
{
	Truncate(0);
	
	va_list argList;
	va_start(argList, inMessage);
	_Printf(inMessage, argList);
	va_end(argList);
}


VString& VString::AppendCString( const char* inCString)
{
	bool usascii;
	VIndex len = CStringLength( inCString, &usascii);
	if (usascii)
	{
		if (_PrepareWriteKeepContent( fLength + len))
		{
			UniChar *pString = fString + fLength;
			const char *p = inCString;
			while(*p)
				*pString++ = *p++;

			*pString = 0;
			fLength = pString - fString;
			GotValue();
		}
	}
	else
	{
		VTextConverters::Get()->GetStdLibToUnicodeConverter()->ConvertString( inCString, len, NULL, *this);
	}
	return *this;
}


VString& VString::AppendUniChars( const UniChar *inCharacters, VIndex inCountCharacters)
{
	if (_PrepareWriteKeepContent( fLength + inCountCharacters))
	{
		::memcpy( fString + fLength, inCharacters, inCountCharacters * sizeof( UniChar));
		fLength += inCountCharacters;
		fString[fLength] = 0;
		GotValue();
	}
	return *this;
}


VString& VString::AppendBlock(const void* inBuffer, VSize inCountBytes, CharSet inSet)
{
	if (inSet == VTC_UTF_16)
	{
		xbox_assert((inCountBytes >= 0) && (inCountBytes % sizeof(UniChar) == 0));
		AppendUniChars( (const UniChar*) inBuffer, (VIndex) (inCountBytes / sizeof(UniChar)) );
	}
	else
	{
		VToUnicodeConverter* converter = VTextConverters::Get()->RetainToUnicodeConverter(inSet);
		converter->ConvertString( inBuffer, inCountBytes, NULL, *this);
		converter->Release();
	}
	return *this;
}


VSize VString::GetSizeInStream() const
{
	VSize tot = GetLength();
	if (tot > 0)
		tot = (tot+1)*sizeof(sWORD) + sizeof(sLONG);

	tot = tot + sizeof(sLONG);
	return tot;
}


VError VString::WriteToStream( VStream* inStream, sLONG inParam) const
{
	sLONG len = GetLength();
	inStream->PutLong( -len);
	if (len > 0)
	{
		inStream->PutWords( fString, len); // do not include the null char
	}
	return inStream->GetLastError();
}


VError VString::ReadFromStream( VStream* inStream, sLONG /*inParam*/)
{
	sLONG len = inStream->GetLong();
	if (len < 0)
	{
		// more compact version: always UTF_16, no trailing zero
		len = -len;
		if (_PrepareWriteTrashContent( len))
		{
			inStream->GetWords( fString, &len);
			Validate( len);
		}
		else
		{
			Clear();
		}
	}
	else if (len > 0)
	{
		// leave this version for compatibility
		CharSet cs = (CharSet)inStream->GetLong();
		if (testAssert(cs == VTC_UTF_16 || cs == VTC_UTF_16_ByteSwapped))
		{
			if (_PrepareWriteTrashContent( len))
			{
				sLONG count = len+1;
				inStream->GetWords( fString, &count);
				fLength = len;
				fString[fLength] = 0;
				GotValue();
			}
			else
			{
				inStream->SetPosByOffset((len+1) * sizeof(UniChar));
			}
		}
		else
		{
			inStream->SetPosByOffset(len+1);
		}
	}
	else
	{
		Clear();
	}
	return inStream->GetLastError();
}


void VString::Truncate( VIndex inNbChars)
{
	xbox_assert(inNbChars >= 0);
	if (inNbChars < fLength)
	{
		bool needRealloc;
		if (_IsShared())
		{
			needRealloc = true;
		}
		else if (fString == fBuffer)
		{
			needRealloc = false;
		}
		else
		{
			// Don't realloc buffer each time Truncate() is called;
			// shrink buffer not to waste more than 1/8 of asked size
			needRealloc = fMaxLength > (inNbChars + 32 + (inNbChars / 8));
			if (needRealloc)
			{
				needRealloc = true;
			}
		}
		
		// set fLength before calling _ReallocBuffer so that buffer is allocated without bonus
		VIndex savedLength = fLength;
		fLength = (inNbChars <= 0) ? 0 : inNbChars;

		if (!needRealloc || _ReallocBuffer( fLength, true))
			fString[fLength] = 0;
		else
			fLength = savedLength;

		// GotValue(); // retire par L.R le 12 mai 2005
	}
}

void VString::RemoveWhiteSpaces( bool inLeadings, bool inTrailings, VIndex* outLeadingsRemoved, VIndex* outTrailingsRemoved, VIntlMgr *inIntlMgr, bool inExcludingNoBreakSpace)
{
	if (inIntlMgr)
	{
		if ( inLeadings )
		{
			VIndex i = 0;

			while ( i < fLength && inIntlMgr->IsSpace(fString[ i ], inExcludingNoBreakSpace) )
				i++;

			if ( i > 0 )
				Remove( 1, i );

			if ( NULL != outLeadingsRemoved )
				*outLeadingsRemoved = i;
		}
		
		if ( inTrailings )
		{
			VIndex i = fLength;
			VIndex removed = 0;
			while ( i > 0 && inIntlMgr->IsSpace(fString[ i - 1 ], inExcludingNoBreakSpace) )
			{
				removed++;
				i--;
			}

			if ( i < fLength )
				Truncate( i );

			if ( NULL != outTrailingsRemoved )
				*outTrailingsRemoved = removed;
		}
		return;
	}

	if ( inLeadings )
	{
		VIndex i = 0;

		while ( i < fLength && ( fString[ i ] == ' ' || fString[ i ] == 0x09 ) )
			i++;

		if ( i > 0 )
			Remove( 1, i );

		if ( NULL != outLeadingsRemoved )
			*outLeadingsRemoved = i;
	}
	
	if ( inTrailings )
	{
		VIndex i = fLength;
		VIndex removed = 0;
		while ( i > 0 && ( fString[ i - 1 ] == ' ' || fString[ i - 1 ] == 0x09 ) )
		{
			removed++;
			i--;
		}

		if ( i < fLength )
			Truncate( i );

		if ( NULL != outTrailingsRemoved )
			*outTrailingsRemoved = removed;
	}
}


void VString::ToUpperCase( bool inStripDiacritics)
{
	VIntlMgr::GetDefaultMgr()->ToUpperLowerCase(*this, inStripDiacritics, true);
}


void VString::ToLowerCase( bool inStripDiacritics)
{
	VIntlMgr::GetDefaultMgr()->ToUpperLowerCase(*this, inStripDiacritics, false);
}


void VString::Decompose()
{
	VIntlMgr::GetDefaultMgr()->NormalizeString( *this, NM_NFD);
}

void VString::Compose()
{
	VIntlMgr::GetDefaultMgr()->NormalizeString( *this, NM_NFC);
}


void VString::GetString(VString& outString) const
{
	outString.FromString(*this);
}


void VString::GetVUUID(VUUID& outValue) const
{
	outValue.FromString(*this);
}


void VString::GetTime(VTime& outTime) const
{
	outTime.FromString(*this);
}


void VString::GetDuration(VDuration& outDuration) const
{
	outDuration.FromString(*this);
}


uLONG VString::GetHashValue() const
{
    uLONG stringLength = GetLength();
    uLONG result = stringLength;
    uLONG characterIndex;

	const UniChar *uContents = GetCPointer();
	if (stringLength <= 16) {
		for (characterIndex = 0; characterIndex < stringLength; ++characterIndex) 
			result = result * 257 + uContents[characterIndex];
	} else {
		for (characterIndex = 0; characterIndex < 8; ++characterIndex) 
			result = result * 257 + uContents[characterIndex];
		for (characterIndex = stringLength - 8; characterIndex < stringLength; ++characterIndex) 
			result = result * 257 + uContents[characterIndex];
	}
	
    result += (result << (stringLength & 31));
    return result;
}


Real VString::GetReal() const
{
	Real result;

#if VERSIONWIN
	static _locale_t sCLocale = _create_locale( LC_NUMERIC, "C");
	result = _wcstod_l( GetCPointer(), NULL, sCLocale);
#elif VERSIONMAC
	static locale_t sCLocale = newlocale( LC_NUMERIC_MASK, NULL, NULL);
	VStringConvertBuffer buffer( *this, VTC_UTF_8);
	result = strtod_l( buffer.GetCPointer(), NULL, sCLocale);
#else
	static locale_t sCLocale = newlocale( LC_NUMERIC_MASK, "C", NULL);
	locale_t oldLocale = uselocale( sCLocale);
	VStringConvertBuffer buffer( *this, VTC_UTF_8);
	result = strtod( buffer.GetCPointer(), NULL);
	uselocale( oldLocale);
#endif

	return result;
}


sLONG8 VString::GetLong8() const
{
	sLONG8 result = 0;
	UniChar decimalSeparator = 0;
	bool	isneg = false;

	const UniChar* begin = GetCPointer();
	const UniChar* end = GetCPointer() + GetLength();
	for( const UniChar *p = begin ; p != end ; ++p)
	{
		if (*p >= CHAR_DIGIT_ZERO && *p <= CHAR_DIGIT_NINE)
		{
			result *= 10;
			result += *p - CHAR_DIGIT_ZERO;
		}
		else if (*p == CHAR_HYPHEN_MINUS)
		{
			if (result == 0)
				isneg = true;
		}
		else
		{
			if (decimalSeparator == 0)
				decimalSeparator = VIntlMgr::GetDefaultMgr()->GetDecimalSeparator()[0];
			if (*p == decimalSeparator)
				break;
		}
	}
	if (isneg)
		result = -result;
	return result;
}


sLONG VString::GetLong() const
{
	sLONG8 value = GetLong8();
	return (value >= kMIN_sLONG && value <= kMAX_sLONG) ? (sLONG) value : 0;
}


sWORD VString::GetWord() const
{
	sLONG8 value = GetLong8();
	return (value >= kMIN_sWORD && value <= kMAX_sWORD) ? (sWORD) value : 0;
}


Boolean VString::GetBoolean() const
{
	return EqualToString( CVSTR( "true"), false);
}


OsType VString::GetOsType() const
{
	OsType type = 0;
	unsigned char spKind[5];

	ToBlock(&spKind[0], sizeof(spKind), VTC_StdLib_char, false, true);
	if (testAssert(spKind[0] <= 4))
	{
		for(long i = spKind[0]+1 ; i <= 4 ; ++i)
		{
			spKind[0] += 1;
			spKind[i] = ' ';
		}
		type = *(OsType*) &spKind[1];
	}

#if SMALLENDIAN		// sc 07/03/2007, was __GNUC__ && __i386__
	XBOX::ByteSwap( &type);
#endif

	return type;	
}


VSize VString::ToBlock(void* inBuffer, VSize inBufferSize, CharSet inSet, bool inWithTrailingZero, bool inWithLengthPrefix) const
{
	bool isOK;
	VSize bytesProduced;
	VIndex charsConsumed;

	VSize size = inBufferSize;

	if (inSet == VTC_UTF_16)
	{
		UniChar* unip = (UniChar*) inBuffer;

		if (inWithTrailingZero)
			size -= 2;
		
		if (inWithLengthPrefix)
		{
			unip += 1;
			size -= 2;
		}

		isOK = VTextConverters::Get()->ConvertFromVString(*this, &charsConsumed, unip, size, &bytesProduced, inSet);

		if (inWithTrailingZero)
			unip[bytesProduced/2] = 0;
		
		if (inWithLengthPrefix)
		{
			if (!testAssert(bytesProduced < 65536*2))
				bytesProduced = 0;
			((UniChar*)inBuffer)[0] = (UniChar) (bytesProduced/2);
		}
		
	}
	else
	{
		unsigned char* ptr = (unsigned char*) inBuffer;

		if (inWithTrailingZero)
			size -= 1;
		
		if (inWithLengthPrefix)
		{
			ptr += 1;
			size -= 1;
		}

		isOK = VTextConverters::Get()->ConvertFromVString(*this, &charsConsumed, ptr, size, &bytesProduced, inSet);

		if (inWithTrailingZero)
			ptr[bytesProduced] = 0;
		
		if (inWithLengthPrefix)
		{
			if (!testAssert(bytesProduced < 256))
				bytesProduced = 0;
			((unsigned char*)inBuffer)[0] = (unsigned char) bytesProduced;
		}
		
	}

	return bytesProduced;
}


void VString::GetFloat(VFloat& outFloat) const
{
	outFloat.FromString(*this);
}


void VString::SubString( VIndex inFirst, VIndex inNbChars)
{
	xbox_assert(inFirst >= 1 && inFirst <= fLength);
	xbox_assert(inNbChars >= 0);

	if (inFirst + inNbChars - 1 > (VIndex)fLength)
		inNbChars = fLength - inFirst + 1;

	if (_PrepareWriteKeepContent( fLength))
	{
		if (inFirst > 1)
			::memmove( fString, fString + inFirst - 1, inNbChars * sizeof(UniChar));
		fLength = inNbChars;
		fString[fLength] = 0;
		GotValue();
	}
}


void VString::DoNullChanged()
{
	if (IsNull())
		_Clear();
}


void VString::Replace(const VString& inStringToInsert, VIndex inPlaceToInsert, VIndex inLenToReplace)
{
	if (inPlaceToInsert > (VIndex)fLength || inLenToReplace == 0)
		return;

	if (!testAssert(inPlaceToInsert > 0))
		return;
	
	if (!testAssert(inLenToReplace > 0))
		return;

	if (inPlaceToInsert + inLenToReplace - 1 >= fLength)
	{
		if (_PrepareWriteKeepContent( inPlaceToInsert - 1 + inStringToInsert.fLength))
		{
			fLength = inPlaceToInsert - 1;
			fString[fLength] = 0;
			GotValue();
			AppendString(inStringToInsert);
		}
	}
	else if (inLenToReplace == inStringToInsert.GetLength())
	{
		if (_PrepareWriteKeepContent( fLength))
		{
			::memmove( fString + inPlaceToInsert - 1, inStringToInsert.GetCPointer(), inLenToReplace * sizeof(UniChar));
			GotValue();
		}
	}
	else
	{
		Remove(inPlaceToInsert, inLenToReplace);
		Insert(inStringToInsert, inPlaceToInsert);
	}
}


void VString::Remove(VIndex inPlaceToRemove, VIndex inLenToRemove)
{
	if (!testAssert(inPlaceToRemove >= 1 && inPlaceToRemove <= fLength))
		return;
	
	if (!testAssert(inLenToRemove >= 0))
		return;
	
	if (_PrepareWriteKeepContent( fLength))
	{
		if (inPlaceToRemove + inLenToRemove - 1 >= fLength)
		{
			fLength = inPlaceToRemove - 1;
		}
		else
		{
			::memmove( fString + inPlaceToRemove - 1, fString + inPlaceToRemove - 1 + inLenToRemove, (fLength - (inPlaceToRemove + inLenToRemove - 1)) * sizeof(UniChar));
			fLength -= inLenToRemove;
		}
		fString[fLength] = 0;
		GotValue();
	}
}


void VString::Swap( VString& inOther)
{
	if ( (fString != fBuffer) && (inOther.fString != inOther.fBuffer) )
	{
		// exchange pointers
		bool other__IsStringPointerOwner = inOther._IsStringPointerOwner();
		bool other__IsSharable = inOther._IsSharable();
		UniChar *other_fString = inOther.fString;
		VIndex other_fLength = inOther.fLength;
		VIndex other_fMaxLength = inOther.fMaxLength;

		inOther.fString = fString;
		inOther.fLength = fLength;
		inOther.fMaxLength = fMaxLength;

		if (_IsStringPointerOwner())
			inOther._SetStringPointerOwner();
		else
			inOther._ClearStringPointerOwner();

		if (_IsSharable())
			inOther._SetSharable();
		else
			inOther._ClearSharable();
			
		inOther.GotValue();

		if (other__IsStringPointerOwner)
			_SetStringPointerOwner();
		else
			_ClearStringPointerOwner();

		if (other__IsSharable)
			_SetSharable();
		else
			_ClearSharable();
		
		fString = other_fString;
		fLength = other_fLength;
		fMaxLength = other_fMaxLength;

		GotValue();
	}
	else if (fString != fBuffer)
	{
		// give our buffer
		bool other__IsStringPointerOwner = _IsStringPointerOwner();
		bool other__IsSharable = _IsSharable();
		UniChar *other_fString = fString;
		VIndex other_fLength = fLength;
		VIndex other_fMaxLength = fMaxLength;

		fString = fBuffer;
		fMaxLength = fMaxBufferLength;
		fLength = 0;
		fString[0] = 0;
		
		FromString( inOther);

		if (other__IsStringPointerOwner)
			inOther._SetStringPointerOwner();
		else
			inOther._ClearStringPointerOwner();

		if (other__IsSharable)
			inOther._SetSharable();
		else
			inOther._ClearSharable();

		inOther.fString = other_fString;
		inOther.fMaxLength = other_fMaxLength;
		inOther.fLength = other_fLength;

		inOther.GotValue();
	}
	else if (inOther.fString != inOther.fBuffer)
	{
		// take its buffer
		bool other__IsStringPointerOwner = inOther._IsStringPointerOwner();
		bool other__IsSharable = inOther._IsSharable();
		UniChar *other_fString = inOther.fString;
		VIndex other_fLength = inOther.fLength;
		VIndex other_fMaxLength = inOther.fMaxLength;

		inOther.fString = inOther.fBuffer;
		inOther.fMaxLength = inOther.fMaxBufferLength;
		inOther.fLength = 0;
		inOther.fString[0] = 0;
		
		inOther.FromString( *this);

		if (other__IsStringPointerOwner)
			_SetStringPointerOwner();
		else
			_ClearStringPointerOwner();

		if (other__IsSharable)
			_SetSharable();
		else
			_ClearSharable();
		
		fString = other_fString;
		fMaxLength = other_fMaxLength;
		fLength = other_fLength;

		GotValue();
	}
	else if (_PrepareWriteKeepContent( inOther.GetLength()) && inOther._PrepareWriteKeepContent( GetLength()) )
	{
		VIndex i;
		if (fLength > inOther.fLength)
		{
			for( i = 0 ; i < inOther.fLength ; ++i)
			{
				UniChar c = inOther.fString[i];
				inOther.fString[i] = fString[i];
				fString[i] = c;
			}
			::memcpy( &inOther.fString[i], &fString[i], (fLength - inOther.fLength + 1) * sizeof( UniChar));
		}
		else
		{
			for( i = 0 ; i < fLength ; ++i)
			{
				UniChar c = inOther.fString[i];
				inOther.fString[i] = fString[i];
				fString[i] = c;
			}
			::memcpy( &fString[i], &inOther.fString[i], (inOther.fLength - fLength + 1) * sizeof( UniChar));
		}
		VIndex other_length = inOther.fLength;
		inOther.fLength = fLength;
		fLength = other_length;
	}
}


void VString::Insert(UniChar inCharToInsert, VIndex inPlaceToInsert)
{
	if (!testAssert(inCharToInsert != 0))
		return;
	
	if (!testAssert(inPlaceToInsert >= 1))
		return;

	if (_PrepareWriteKeepContent( fLength + 1))
	{
		if (inPlaceToInsert > fLength)
			fString[fLength++] = inCharToInsert;
		else
		{
			::memmove( fString + inPlaceToInsert, fString + inPlaceToInsert - 1, (fLength - (inPlaceToInsert - 1)) * sizeof(UniChar));
			fString[inPlaceToInsert - 1] = inCharToInsert;
			fLength++;
		}
		GotValue();
		fString[fLength] = 0;
	}
}


void VString::Insert(const VString& inStringToInsert, VIndex inPlaceToInsert)
{
	if (!testAssert(inPlaceToInsert >= 1))
		return;

	if (inPlaceToInsert > fLength)
		AppendString(inStringToInsert);
	else
	{
		if (_PrepareWriteKeepContent(fLength + inStringToInsert.GetLength()))
		{
			::memmove( fString + inPlaceToInsert - 1 + inStringToInsert.GetLength(), fString + inPlaceToInsert - 1, (fLength - (inPlaceToInsert - 1)) * sizeof(UniChar));
			::memmove( fString + inPlaceToInsert - 1, inStringToInsert.GetCPointer(), inStringToInsert.GetLength() * sizeof(UniChar));
			fLength += inStringToInsert.GetLength();
			fString[fLength] = 0;
			GotValue();
		}
	}
}


void VString::GetSubString( VIndex inFirst, VIndex inNbChars, VString& outResult) const
{
	if (!testAssert( (inNbChars == 0) || (inFirst > 0 && inFirst <= fLength && inNbChars > 0)))
		outResult.Clear();
	else
	{
		if (!testAssert(inFirst + inNbChars - 1 <= fLength))
			inNbChars = fLength - (inFirst - 1);
		
		outResult.FromUniString(fString + inFirst - 1, inNbChars);
	}
}


void VString::FromValue(const VValueSingle& inValue)
{
	inValue.GetString(*this);
}


void VString::FromString( const VString& inString)
{
	if (inString.IsNull())
	{
		SetNull( true);
	}
	else if (fString != inString.fString)
	{
/*
		let _PrepareWrite throw an error instead of silently truncate
		
		if (GetMaxLength() < inString.GetLength())
		{
			FromBlock( inString.fString, GetMaxLength() * sizeof(UniChar), VTC_UTF_16);
		}
*/
		UniChar *retainedBuffer = inString._RetainBuffer();
		if (retainedBuffer != NULL)
		{
			_ReleaseBuffer();
			fString = retainedBuffer;
			fMaxLength = inString.fMaxLength;
			fLength = inString.fLength;
			GotValue();
		}
		else
		{
			if (_PrepareWriteTrashContent( inString.GetLength()))
			{
				fLength = inString.GetLength();
				::memcpy( fString, inString.GetCPointer(), fLength * sizeof(UniChar));
				fString[fLength] = 0;
				GotValue();
			}
		}
	}
}


void VString::FromString( const VInlineString& inString)
{
	if (fString != inString.fString)
	{
		_ReleaseBuffer();
		fString = inString.RetainBuffer( &fLength, &fMaxLength);
		if (fString == NULL)
		{
			fMaxLength = fMaxBufferLength;
			fString = fBuffer;
			fString[fLength] = 0;
			fLength = 0;
		}
		GotValue();
	}
}


void VString::FromUniChar( UniChar inChar)
{
	if (inChar == 0)
	{
		Clear();
	}
	else if (_PrepareWriteTrashContent( 1))
	{
		fString[0] = inChar;
		fString[1] = 0;
		fLength = 1;
		GotValue();
	}
}


void VString::FromReal(Real inValue)
{
	Truncate(0);
	AppendReal(inValue);
}


void VString::FromLong8(sLONG8 inValue)
{
	Truncate(0);
	AppendLong8(inValue);
}

void VString::FromULong8(uLONG8 inValue)
{
	Truncate(0);
	AppendULong8(inValue);
}

void VString::FromByte(sBYTE inValue)
{
	Truncate(0);
	AppendLong(inValue);
}

void VString::FromWord(sWORD inValue)
{
	Truncate(0);
	AppendLong(inValue);
}

void VString::FromLong(sLONG inValue)
{
	Truncate(0);
	AppendLong(inValue);
}


void VString::FromTime(const VTime& inTime)
{
	inTime.GetString(*this);
}


void VString::FromDuration(const VDuration& inDuration)
{
	inDuration.GetString(*this);
}

void VString::FromFloat(const VFloat& inFloat)
{
	inFloat.GetString(*this);
}


void VString::FromBoolean(Boolean inValue)
{
	FromString( inValue ? CVSTR( "true") : CVSTR( "false"));
}


void VString::FromVUUID(const VUUID& inValue)
{
	inValue.GetString(*this);
}


void VString::FromUniString( const UniChar *inString, VIndex inLength)
{
	if (_PrepareWriteTrashContent( inLength))
	{
		::memcpy( fString, inString, inLength * sizeof( UniChar));
		fLength = inLength;
		fString[fLength] = 0;
//			xbox_assert(UniCStringLength(fString) == fLength);
		GotValue();
	}
}

void VString::FromBlock( const void* inBuffer, VSize inCountBytes, CharSet inSet)
{
	if (inSet == VTC_UTF_16)
	{
		xbox_assert((inCountBytes >= 0) && (inCountBytes % sizeof(UniChar) == 0));
		FromUniString( (const UniChar *) inBuffer, (VIndex) (inCountBytes / sizeof(UniChar)));
	}
	else
	{
		_Clear();
		VToUnicodeConverter* converter = VTextConverters::Get()->RetainToUnicodeConverter(inSet);
		converter->ConvertString( inBuffer, inCountBytes, NULL, *this);
		converter->Release();
	}
}


void VString::FromBlockWithOptionalBOM( const void* inBuffer, VSize inCountBytes, CharSet inDefaultSet)
{
	size_t bomSize;
	CharSet charSet;
	if (!VTextConverters::ParseBOM( inBuffer, inCountBytes, &bomSize, &charSet))
	{
		charSet = inDefaultSet;
		bomSize = 0;
	}
	
	FromBlock( (const char*) inBuffer + bomSize, inCountBytes - bomSize, charSet);
}


void VString::FromCString( const char *inCString)
{
	bool usascii;
	VIndex len = CStringLength( inCString, &usascii);
	if (usascii)
	{
		if (_PrepareWriteTrashContent( len))
		{
			UniChar *pString = fString;
			const char *p = (const char *) inCString;
			while(*p)
				*pString++ = *p++;
			*pString = 0;

			fLength = len;
			GotValue();
		}
	}
	else
	{
		FromBlock( inCString, len, VTC_StdLib_char);
	}
}


void VString::FromOsType(OsType inType)
{
#if SMALLENDIAN
	XBOX::ByteSwap( &inType);
#endif
	FromBlock( &inType, 4, VTC_StdLib_char);
}


VIndex VString::FindUniChar(UniChar inChar, VIndex inPlaceToStart , bool inIsReverseOrder) const
{
	if (fLength == 0)
		return 0;
	
	if (!testAssert(inPlaceToStart >= 1))
		inPlaceToStart = 1;
	else if (!testAssert(inPlaceToStart <= fLength))
		inPlaceToStart = fLength;
	
	size_t pos = 0;
	if (inIsReverseOrder)
	{
		const UniChar* ptrLast = GetCPointer();
		for(const UniChar* ptr = GetCPointer() + inPlaceToStart - 1 ; ptr >= ptrLast ; --ptr)
		{
			if (*ptr == inChar)
			{
				pos = (ptr - GetCPointer()) +1;
				break;
			}
		}
	}
	else
	{
		const UniChar* ptrLast = GetCPointer() + fLength - 1;
		for(const UniChar* ptr = GetCPointer() + inPlaceToStart - 1 ; ptr <= ptrLast ; ++ptr)
		{
			if (*ptr == inChar)
			{
				pos = (ptr - GetCPointer()) +1;
				break;
			}
		}
	}
	
	return CheckedCastToVIndex( pos);
}


bool VString::BeginsWith(const VString& inStringToFind, bool inDiacritical) const
{
	sLONG matchedLength;
	return VIntlMgr::GetDefaultMgr()->GetCollator()->BeginsWithString( GetCPointer(), XBOX::Min(inStringToFind.GetLength()+16, GetLength()), inStringToFind.GetCPointer(), inStringToFind.GetLength(), inDiacritical, &matchedLength);
}


bool VString::EndsWith( const VString& inStringToFind, bool inDiacritical) const
{
	sLONG matchedLength;
	return VIntlMgr::GetDefaultMgr()->GetCollator()->EndsWithString( GetCPointer(), GetLength(), inStringToFind.GetCPointer(), inStringToFind.GetLength(), inDiacritical, &matchedLength);
}


VIndex VString::Find( const VString& inStringToFind, VIndex inPlaceToStart, bool inDiacritical) const
{
	xbox_assert(inPlaceToStart > 0 && (inPlaceToStart <= GetLength() || inPlaceToStart == 1));

	sLONG matchedLength;
	VIndex pos = VIntlMgr::GetDefaultMgr()->GetCollator()->FindString( GetCPointer() + inPlaceToStart - 1, GetLength() - inPlaceToStart + 1, inStringToFind.GetCPointer(), inStringToFind.GetLength(), inDiacritical, &matchedLength);
	return (pos > 0) ? (pos + inPlaceToStart - 1) : 0;
}


void VString::Exchange(const VString& inStringToFind, const VString& inStringToInsert, VIndex inPlaceToStart, VIndex inCountToReplace, bool inDiacritical, VCollator *inCollator)
{
	VCollator *collator = (inCollator != NULL) ? inCollator : VIntlMgr::GetDefaultMgr()->GetCollator();
	VIndex morelen = inStringToInsert.GetLength();
	while(inCountToReplace-- && inPlaceToStart <= GetLength())
	{
		sLONG matchedLength;
		VIndex pos = collator->FindString( GetCPointer() + inPlaceToStart - 1, GetLength() - inPlaceToStart + 1, inStringToFind.GetCPointer(), inStringToFind.GetLength(), inDiacritical, &matchedLength);
		if ((pos <= 0) || (matchedLength == 0))
			break;
		Replace(inStringToInsert, pos + inPlaceToStart - 1, matchedLength);
		inPlaceToStart = pos + inPlaceToStart - 1 + morelen;
	}
}


void VString::Exchange( UniChar inCharToFind, UniChar inNewChar, VIndex inPlaceToStart, VIndex inCountToReplace)
{
	xbox_assert( (inPlaceToStart > 0) && (inPlaceToStart <= GetLength() || inPlaceToStart == 1));
	xbox_assert( inNewChar != 0);

	if (!IsEmpty() && (inCountToReplace > 0) && (inPlaceToStart > 0) )
	{
		UniChar *begin = fString + inPlaceToStart - 1;
		UniChar *end = fString + fLength;
		
		UniChar *i = begin;
		for( ; (i != end) && (*i != inCharToFind) ; ++i)
			;

		if (*i == inCharToFind)
		{
			size_t found = i - begin;
			if (_PrepareWriteKeepContent( fLength))
			{
				xbox_assert( fString[found] == inCharToFind);
				fString[found] = inNewChar;
				
				if (inCountToReplace > 1)
				{
					i = fString + found + 1;
					end = fString + fLength;
					
					do
					{
						for( ; (i != end) && (*i != inCharToFind) ; ++i)
							;
						if (*i != inCharToFind)
							break;
						*i++ = inNewChar;
					} while( --inCountToReplace > 0);
				}
			}
		}
	}
}

ECarriageReturnMode VString::GetCarriageReturnMode( ECarriageReturnMode inDefault )
{
	for ( sLONG i = 0; i < fLength; i++ )
	{
		if ( fString[ i ] == 0x0D || fString[ i ] == 0x0A )
		{
			if ( fString[ i ] == 0x0A )
				return eCRM_LF;
			else
			{
				if ( i + 1 < fLength && fString[ i + 1 ] == 0x0A )
					 return eCRM_CRLF;
				else
					return eCRM_CR;
			}
		}
	}
	return inDefault;
}


void VString::ConvertCarriageReturns(ECarriageReturnMode inNewMode)
{
	/*
		sur mac: 0d
		sur pc:	0d0a
		
		\n = 0x0a
		\r = 0x0d
		
		pour mac:
			replace all (0d0a -> 0d)
			replace all (0a -> 0d)
		
		pour pc:
			replace all (0d0a -> 0e0e)
			replace all (0d -> 0e0e)
			replace all (0a -> 0e0e)
			replace all (0e0e -> 0d0a)
			
	*/

	if ( (inNewMode != eCRM_NONE) && _PrepareWriteKeepContent( fLength))
	{
		switch(inNewMode)
		{
			case eCRM_CR:
				{
					/*
						 \r\n -> \r
						 \n	-> \r
					*/
					UniChar* firstHole = NULL;
					UniChar* startOfLine = NULL;
					VSize previousLength = fLength;
					for(UniChar* ptr = fString ; *ptr ; ++ptr)
					{
						if (*ptr == '\r')
						{
							if (*(ptr+1) == '\n')
							{
								if ( firstHole )
								{
									VSize uniCharsToMove = ( ptr  + 1 ) - startOfLine;
									::memmove( firstHole, startOfLine, sizeof(UniChar) * uniCharsToMove );
									firstHole += uniCharsToMove;
								}
								else
								{
									firstHole = ptr + 1;
								}

								--fLength;
								startOfLine = ptr + 2;
							}
						}
						else if (*ptr == '\n')
							*ptr = '\r';
					}
	
					if ( firstHole )
					{
						::memmove( firstHole, startOfLine, sizeof(UniChar) * ( ( fString + previousLength ) - startOfLine ) );
					}

					fString[fLength]=0;
					break;
				}
		
			case eCRM_LF:
				{
					/*
						 \r\n -> \n
						 \r	-> \n
					*/ 
					for(UniChar* ptr = fString ; *ptr ; ++ptr)
					{
						if (*ptr == '\r')
						{
							if (*(ptr+1) == '\n')
							{
								::memmove(ptr, ptr+1, sizeof(UniChar) * (fLength - (ptr - fString) - 1));
								--fLength;
							}
							else
								*ptr = '\n';
						}
					}
					fString[fLength]=0;
					break;
				}
			
			case eCRM_CRLF:
				{
					/*
						 \r -> \r\n
						 \n	-> \r\n
					*/
					size_t first = 0;
					VIndex count = 0;
					for(UniChar* ptr = fString ; *ptr ; ++ptr)
					{
						if (*ptr == '\r')
						{
							if (*(ptr+1) != '\n')
							{
								*ptr = 0xffff;
								++count;
								if (first == 0)
									first = (ptr - fString) + 1;
							} else
								++ptr;
						} else if (*ptr == '\n')
						{
							*ptr = 0xffff;
							++count;
							if (first == 0)
								first = (ptr - fString) + 1;
						}
					}
					if (count > 0)
					{
						if (_PrepareWriteKeepContent(fLength + count))
						{	
							VIndex index = fLength;
							fLength += count;
							fString[fLength] = 0;
							UniChar* ptr = fString + index - 1;
							UniChar *firstHole = ptr + count;
							size_t size = 1;

							while (index > 0)
							{
								if (*ptr == 0xffff)
								{
									*ptr = '\n';
									::memmove(firstHole, ptr, size*sizeof(UniChar));
									firstHole--;
									*firstHole = '\r';
									firstHole--;
									size = 1;
								}
								else
								{
									size++;
									firstHole--;

								}
								ptr--;
								index--;
							}
						}
						
					}
					break;
				}

			case eCRM_NONE:
			case eCRM_LAST:
				break;
			
		}
	}
}


CharSet VString::DebugDump(char* inTextBuffer, sLONG& inBufferSize) const
{
	VSize len = GetLength() * sizeof(UniChar);
	if (len > (VSize) (inBufferSize-2))
		len = (VSize) (inBufferSize-2);
	::memcpy( inTextBuffer, GetCPointer(), len);
	inBufferSize = (sLONG) len;
	return VTC_UTF_16;
}


VString *VString::Clone() const
{
	return new VString( *this);
}


uLONG8 VString::GetHexLong() const
{
	// L.E. 19/04/1999 relit un entier long exprime sous forme hexa
	// ignores non hexa chars 0->9, a->z, A->Z
	uLONG8 val = 0L;
	const UniChar *begin = GetCPointer();
	const UniChar *end = begin + GetLength();
	for( const UniChar *p = begin ; p != end ; ++p)
	{
		if (*p >= CHAR_DIGIT_ZERO && *p <= CHAR_DIGIT_NINE)
		{
			val *= 16L;
			val += (*p - CHAR_DIGIT_ZERO);
		}
		else if (*p >= CHAR_LATIN_CAPITAL_LETTER_A && *p <= CHAR_LATIN_CAPITAL_LETTER_F)
		{
			val *= 16L;
			val += (*p - CHAR_LATIN_CAPITAL_LETTER_A) + 10L;
		}
		else if (*p >= CHAR_LATIN_SMALL_LETTER_A && *p <= CHAR_LATIN_SMALL_LETTER_F)
		{
			val *= 16L;
			val += (*p - CHAR_LATIN_SMALL_LETTER_A) + 10L;
		}
	}
	return val;
}


template<class T>
void StringFromHexLong( VString& inString, T inValue, bool inPrettyFormatting)
{
	UniChar *p = inString.GetCPointerForWrite( sizeof( T) * 2 + (inPrettyFormatting ? 2 : 0));
	if (p != NULL)
	{
		VIndex length = 0;
		T mask = 0xf;

		// si la valeur a analyse est 0, il ne sert a rien de passer dans la boucle
		if (inValue != 0)
		{
			for( sLONG shift = 8 * sizeof(T) - 4 ; shift >= 0 ; shift -= 4)
			{
				uBYTE nibble = (inValue & (mask << shift) ) >> shift;
				if ( (nibble != 0) || (length != 0))
				{
					if (nibble < 10)
						p[length++] = '0' + nibble;
					else
						p[length++] = 'A' + (nibble - 10);
				}
			}
		}
		else // il faut gerer ce cas, sinon la longueur de la chaine vaut zero pour une valeur de zero
		{
			p[length] = '0';
			++length;
		}

		// pretty formatting
		if (inPrettyFormatting)
		{
			VIndex extra;
			if (length <= 2 && sizeof( T) == 1)
			{
				extra = 2 - length;
			}
			else if (length <= 4)
			{
				extra = 4 - length;
			}
			else if (length <= 8)
			{
				extra = 8 - length;
			}
			else if (length % 8 != 0)
			{
				extra = 8 - (length % 8);
			}
			else
			{
				extra = 0;
			}
			if (extra != 0)
			{
				::memmove( p+extra, p, length * sizeof( UniChar));
				length += extra;
				while( extra--)
					p[extra] = '0';
			}
			::memmove( p+2, p, length * sizeof( UniChar));
			p[0] = '0';
			p[1] = 'x';
			length += 2;
		}
		
		inString.Validate( length);
	}
	else
	{
		inString.Clear();
	}
}


void VString::FromHexLong( uLONG8 inValue, bool inPrettyFormatting)
{
	StringFromHexLong( *this, inValue, inPrettyFormatting);
}


void VString::FromHexLong( sLONG8 inValue, bool inPrettyFormatting)
{
	StringFromHexLong( *this, static_cast<uLONG8>( inValue), inPrettyFormatting);
}


void VString::FromHexLong( uLONG inValue, bool inPrettyFormatting)
{
	StringFromHexLong( *this, inValue, inPrettyFormatting);
}


void VString::FromHexLong( sLONG inValue, bool inPrettyFormatting)
{
	StringFromHexLong( *this, static_cast<uLONG>( inValue), inPrettyFormatting);
}


void VString::FromHexLong( uBYTE inValue, bool inPrettyFormatting)
{
	StringFromHexLong( *this, inValue, inPrettyFormatting);
}


void VString::FromHexLong( sBYTE inValue, bool inPrettyFormatting)
{
	StringFromHexLong( *this, static_cast<uBYTE>( inValue), inPrettyFormatting);
}


void VString::TranscodeToXML(bool inEscapeControlChars)
{
	//JQ 18/09/2012: escape controls chars 0x1-0x1F & 0x7F (optional) but tab, cr and lf

	static const UniChar sXMLEscapeChars[] =  { CHAR_AMPERSAND, CHAR_GREATER_THAN_SIGN, CHAR_QUOTATION_MARK, CHAR_LESS_THAN_SIGN, CHAR_APOSTROPHE};

	VString*	temp = NULL;
	const UniChar*	ptr = GetCPointer();
	const UniChar*	pFirst = ptr;
	while(*ptr)
	{
		if (inEscapeControlChars && ((*ptr >= 0x01 && *ptr <= 0x1F && *ptr != 0x09 && *ptr != 0x0A && *ptr != 0x0D) || *ptr == 0x7F))
		{
			if (temp == NULL)
				temp = new VString;
			if (temp != NULL)
			{
				char hex[8];
				sprintf(hex,"%X", *ptr);
		
				temp->AppendBlock(pFirst, (ptr-pFirst) * sizeof(UniChar), VTC_UTF_16);
				pFirst = ptr+1;

				if (strlen(hex) == 1)
					temp->AppendString(CVSTR("&#x0")+hex+";");
				else
					temp->AppendString(CVSTR("&#x")+hex+";");
			}
		}
		else for (const UniChar* pEscape = sXMLEscapeChars ; pEscape != sXMLEscapeChars + sizeof(sXMLEscapeChars)/sizeof( UniChar) ; ++pEscape)
		{
			if (*ptr == *pEscape)
			{
				if (temp == NULL)
					temp = new VString;
				if (temp != NULL)
				{
					temp->AppendBlock(pFirst, (ptr-pFirst) * sizeof(UniChar), VTC_UTF_16);
					pFirst = ptr+1;

					switch(*ptr)
					{
						case CHAR_AMPERSAND:			temp->AppendString(CVSTR("&amp;")); break;
						case CHAR_GREATER_THAN_SIGN:	temp->AppendString(CVSTR("&gt;")); break;
						case CHAR_QUOTATION_MARK:		temp->AppendString(CVSTR("&quot;")); break;
						case CHAR_LESS_THAN_SIGN:		temp->AppendString(CVSTR("&lt;")); break;
						case CHAR_APOSTROPHE:			temp->AppendString(CVSTR("&apos;")); break;
						default:
							break;
					}
				}
				break;
			}
		}
		++ptr;
	}

	if (temp != NULL)
	{
		temp->AppendBlock(pFirst, (ptr-pFirst) * sizeof(UniChar), VTC_UTF_16);
		FromString( *temp);
		delete temp;
	}
}


void VString::TranscodeFromXML()
{
	static const UniChar sXMLEscapeChars[] = { CHAR_AMPERSAND, CHAR_GREATER_THAN_SIGN, CHAR_QUOTATION_MARK, CHAR_LESS_THAN_SIGN, CHAR_APOSTROPHE };

	VString*	temp = NULL;
	const UniChar*	ptr = GetCPointer();
	const UniChar*	pFirst = ptr;
	VIndex pos = 0;
	VIndex length = GetLength();
	while(*ptr)
	{
		if (*ptr == CHAR_AMPERSAND)
		{
			UniChar replacement;
			int toSkip;

			if (pos+4 < length && ptr[1] == 'a' && ptr[2] == 'm' && ptr[3] == 'p' && ptr[4] == ';')
			{
				toSkip = 4;
				replacement = CHAR_AMPERSAND;
			}
			else if (pos+3 < length && ptr[1] == 'g' && ptr[2] == 't' && ptr[3] == ';')
			{
				toSkip = 3;
				replacement = CHAR_GREATER_THAN_SIGN;
			}
			else if (pos+5 < length && ptr[1] == 'q' && ptr[2] == 'u' && ptr[3] == 'o' && ptr[4] == 't' && ptr[5] == ';')
			{
				toSkip = 5;
				replacement = CHAR_QUOTATION_MARK;
			}
			else if (pos+3 < length && ptr[1] == 'l' && ptr[2] == 't' && ptr[3] == ';')
			{
				toSkip = 3;
				replacement = CHAR_LESS_THAN_SIGN;
			}
			else if (pos+5 < length && ptr[1] == 'a' && ptr[2] == 'p' && ptr[3] == 'o' && ptr[4] == 's' && ptr[5] == ';')
			{
				toSkip = 5;
				replacement = CHAR_QUOTATION_MARK;
			}
			else if (pos+3 < length && ptr[1] == '#' && ptr[2] == 'x')
			{
				toSkip = 2;
				bool hasDigit = false;
				while (pos+toSkip+1 < length && ptr[toSkip+1] != ';')
				{
					UniChar c = ptr[toSkip+1];
					hasDigit =	(c >= CHAR_DIGIT_ZERO && c <= CHAR_DIGIT_NINE)
								|| 
								(c >= CHAR_LATIN_CAPITAL_LETTER_A && c <= CHAR_LATIN_CAPITAL_LETTER_F)
								||
								(c >= CHAR_LATIN_SMALL_LETTER_A && c <= CHAR_LATIN_SMALL_LETTER_F);
					if (!hasDigit)
						break;
					toSkip++; //skip digit
				}
				if (hasDigit && pos+toSkip+1 < length && ptr[toSkip+1] == ';')
				{
					//JQ 18/09/2012: escaped characters
					VString hex;
					hex.FromUniString( ptr+3, toSkip-2);
					toSkip++; //skip ';'
					replacement = hex.GetHexLong();
					xbox_assert(replacement != 0);
				}
				else
					replacement = 0;
			}
			else
				replacement = 0;

			if (replacement != 0)
			{
				if (temp == NULL)
					temp = new VString;
				if (temp != NULL)
				{
					temp->AppendBlock( pFirst, (ptr-pFirst) * sizeof(UniChar), VTC_UTF_16);
					temp->AppendUniChar( replacement);
				}
				pFirst = ptr+toSkip+1;	// 4 is min size of all cases
			}
		}
		++ptr;
		pos++;
	}

	if (temp != NULL)
	{
		temp->AppendBlock(pFirst, (ptr-pFirst) * sizeof(UniChar), VTC_UTF_16);
		FromString( *temp);
		delete temp;
	}
}


bool VString::EncodeBase64( const void *inDataPtr, size_t inDataSize)
{
	VMemoryBuffer<> resultBuffer;
	bool ok = Base64Coder::Encode( inDataPtr, inDataSize, resultBuffer);

	if (ok)
	{
		// convert base64 characters back into VString. No need for a text converter because it's iso-latin.
		if (_PrepareWriteTrashContent( CheckedCastToVIndex( resultBuffer.GetDataSize())))
		{
			const uBYTE *source_begin = (uBYTE*) resultBuffer.GetDataPtr();
			const uBYTE *source_end = source_begin + resultBuffer.GetDataSize();
			std::copy( source_begin, source_end, fString);
		
			fLength = (VIndex) resultBuffer.GetDataSize();
			fString[fLength] = 0;
			GotValue();
		}
		else
		{
			ok = false;
		}
	}

	return ok;
}


bool VString::EncodeBase64( CharSet inSet)
{
	if (IsEmpty())
		return true;

	VStringConvertBuffer inputBuffer( *this, inSet);
	bool ok = EncodeBase64( inputBuffer.GetCPointer(), inputBuffer.GetSize());

	return ok;
}

bool VString::DecodeBase64( VMemoryBuffer<>& outBuffer) const
{
	// convert base64 characters into iso-latin bytes for decoding.
	VMemoryBuffer<> inputBuffer;
	if (!inputBuffer.SetSize( fLength))
	{
		vThrowError( VE_MEMORY_FULL);
		return false;
	}
	
	const UniChar *source_begin = fString;
	const UniChar *source_end = source_begin + fLength;
	uBYTE *dest = (uBYTE*) inputBuffer.GetDataPtr();
	for( const UniChar *source = source_begin ; source != source_end ; ++source)
	{
		// check it's iso-latin
		if (*source > 0x7f)
		{
			vThrowError( VE_STREAM_TEXT_CONVERSION_FAILURE);
			return false;
		}
		*dest++ = (uBYTE) *source;		
	}

	bool ok = Base64Coder::Decode( inputBuffer.GetDataPtr(), inputBuffer.GetDataSize(), outBuffer);

	return ok;
}


bool VString::DecodeBase64( CharSet inSet)
{
	if (IsEmpty())
		return true;

	VMemoryBuffer<> resultBuffer;
	bool ok = DecodeBase64( resultBuffer);

	if (ok)
	{
		FromBlock( resultBuffer.GetDataPtr(), resultBuffer.GetDataSize(), inSet);
	}

	return ok;
}


void VString::GetXMLString( VString& outString, XMLStringOptions inOptions) const
{
	outString.FromString( *this);
	outString.TranscodeToXML();
}


bool VString::FromXMLString( const VString& inString)
{
	FromString( inString);
	TranscodeFromXML();
	return true;
}


void VString::_Clear()
{
	_ReleaseBuffer();
	fString = fBuffer;
	fMaxLength = fMaxBufferLength;
	fLength = 0;
	fString[fLength] = 0;
}


void VString::Clear()
{
	_Clear();
	GotValue();
}


VString& VString::AppendUniChar(UniChar inChar)
{ 
	if (_PrepareWriteKeepContent( fLength + 1))
	{ 
		fString[fLength] = inChar;
		fString[++fLength] = 0;
		GotValue();
	}
	
	return *this;
}


#if VERSIONMAC

CFStringRef	VString::MAC_RetainCFStringCopy() const
{
	CFStringRef	stringRef = ::CFStringCreateWithCharacters(kCFAllocatorDefault, GetCPointer(), GetLength());
	return stringRef;
}


void VString::MAC_FromCFString( CFStringRef inString)
{
	// Caution: it seams that several common unicode chars aren't currently supported
	//	by VIntlMgr. May be optimized for various impl branches - JT 23/01/04
	if (testAssert(inString != NULL))
	{
		CFIndex stringLength = ::CFStringGetLength( inString);
		
		const UniChar*	uniPtr = ::CFStringGetCharactersPtr(inString);
		if (uniPtr != NULL)
		{
			FromUniString( uniPtr, stringLength);
		}
		else
		{
			Clear();
			CFIndex	rangeStart = 0;
			UniChar	buffer[1024];
			do
			{
				CFIndex count = Min<CFIndex>(stringLength, sizeof(buffer)/sizeof(UniChar));
				stringLength -= count;
				
				::CFStringGetCharacters(inString, CFRangeMake(rangeStart, count), buffer);
				AppendBlock( buffer, count * sizeof( UniChar), VTC_UTF_16);
				
				rangeStart += count;
			}
			while( stringLength > 0);
		}
	}
}

#endif	// VERSIONMAC


/* static */
VIndex VString::UniCStringLength(const UniChar* inSrc)
{
	if (inSrc == NULL) return 0;
	
	const UniChar*	stringPtr = inSrc;

	while (*stringPtr != 0)
		stringPtr++;

	return (VIndex) (stringPtr - inSrc);
}

bool VString::GetSubStrings( UniChar inDivider, VectorOfVString& outResultItems, bool inReturnEmptyStrings, bool inTrimSpaces) const
{
	bool result = false;
	outResultItems.clear();

	if (IsEmpty())
	{
		if (inReturnEmptyStrings)
			outResultItems.push_back( *this);
	}
	else
	{
		VString subString;
		VIndex	oldPos = 1;
		
		do
		{   
			VIndex pos = FindUniChar( inDivider, oldPos);
			if (pos > 0)
			{
				GetSubString( oldPos, pos - oldPos, subString);
				oldPos = pos + 1;
				result = true;
			}
			else
			{
				GetSubString( oldPos, GetLength() + 1 - oldPos, subString);
				oldPos = GetLength() + 1;
			}
			
			if (inTrimSpaces)
			{
				subString.TrimeSpaces();
			}

			if (!subString.IsEmpty() || inReturnEmptyStrings)
			{
				outResultItems.push_back( subString);
			}

			if ( (pos == GetLength()) && inReturnEmptyStrings)
			{
				// aligned behavior to GetSubStrings with VString.
				// If we find a divider at the end of the string, it means that we need to add a last empty string to the result
				subString.Clear();
				outResultItems.push_back( subString);
			}

		} while( oldPos <= GetLength());
	}
	
	return result;
}


bool VString::GetSubStrings( UniChar inDivider, VArrayString& outResultItems, bool inReturnEmptyStrings, bool inTrimSpaces) const
{
	VectorOfVString list;
	bool found = GetSubStrings( inDivider, list, inReturnEmptyStrings, inTrimSpaces);

	outResultItems.Destroy();
	for( VectorOfVString::const_iterator i = list.begin() ; i != list.end() ; ++i)
		outResultItems.AppendString( *i);
	
	return found;
}


bool VString::Join( const VectorOfVString& inStrings, UniChar inSeparator)
{
	if (inStrings.empty())
	{
		Clear();
		return true;
	}
	
	// compute the size to allocate the result in one operation
	VIndex length = 0;
	for( VectorOfVString::const_iterator i = inStrings.begin() ; i != inStrings.end() ; ++i)
		length += i->GetLength();
	
	// add separators
	length += (VIndex) inStrings.size() - 1;
	
	UniChar* dest = GetCPointerForWrite( length);
	if (dest != NULL)
	{
		VectorOfVString::const_iterator last = inStrings.begin() + (inStrings.size() - 1);
		for( VectorOfVString::const_iterator i = inStrings.begin() ; i != last ; ++i)
		{
			::memcpy( dest, i->GetCPointer(), i->GetLength() * sizeof( UniChar));
			dest += i->GetLength();
			*dest++ = inSeparator;
		}
		::memcpy( dest, last->GetCPointer(), last->GetLength() * sizeof( UniChar));
		Validate( length);
	}
	
	return dest != NULL;
}


void VString::TrimeSpaces()
{
	if (fLength > 0)
	{
		VIndex leadingspaces = 0;
		VIndex endingspaces = 0;
		for (UniPtr p = fString, end = fString + fLength; p != end && (*p == ' '); p++, leadingspaces++) ;
		for (UniPtr p = fString+fLength-1, start = fString - 1; p != start && (*p == ' '); p--, endingspaces++) ;
		if (leadingspaces != 0 || endingspaces != 0)
		{
			if (leadingspaces+endingspaces >= fLength)
				Clear();
			else
			{
				SubString(leadingspaces+1, fLength-leadingspaces-endingspaces);
			}
		}
	}
}


bool VString::GetSubStrings( const VString &inDivider, VectorOfVString& outResultItems, bool inReturnEmptyStrings, bool inTrimSpaces) const
{
	outResultItems.clear();

	//Empty string or empty divider case
	if (IsEmpty())
		return false;
	
	//No divider found case
	if (inDivider.IsEmpty() || Find(inDivider, 1, true) == 0)
	{
		outResultItems.push_back(*this);
		return false;
	}
		
	//Now we know that there is at least one divider in the string
	bool endOfTheString = false;
	VIndex lastDividerEnd = 0;
	VIndex dividerStart = 0;
	while(!endOfTheString)
	{
		dividerStart = Find(inDivider, lastDividerEnd + 1, true);
		if(dividerStart == 0)
		{
			// no more divider. We simulate a last divider at the end of the string
			endOfTheString = true;
			dividerStart = GetLength() +1;
		}

		if (dividerStart != 1 && dividerStart != lastDividerEnd + 1)
		{
			//The string doesn't begin with a divider
			VString stringToInsert;
			GetSubString(lastDividerEnd + 1, dividerStart - lastDividerEnd - 1, stringToInsert);
			if (inTrimSpaces)
			{
				stringToInsert.TrimeSpaces();
			}
			if (inReturnEmptyStrings || !stringToInsert.IsEmpty())
				outResultItems.push_back(stringToInsert);
		}
		else if(dividerStart == lastDividerEnd + 1 && inReturnEmptyStrings)
		{
			//Two following dividers were found : add an empty string if needed
			VString emptyString = "";
			outResultItems.push_back(emptyString);
		}

		lastDividerEnd = dividerStart + inDivider.GetLength() - 1;
		if(lastDividerEnd == GetLength() && inReturnEmptyStrings)
		{
			//If we find a divider at the end of the string, it means that we need to add a last empty string to the result
			VString emptyString = "";
			outResultItems.push_back(emptyString);
		}

		if(lastDividerEnd >= GetLength())
			endOfTheString = true;
	}
	return true;
}


bool VString::IsStringValid() const
{
	if (fString != &fBuffer[0])
	{
		if (! testAssert(_GetBufferAllocator()->CheckPtr(fString)) )
			return false;
	}
	if (fLength > 0)
	{
		if (fString[fLength] != 0)
		{
			xbox_assert(false);
			return false;
		}
	}
	return true;
}


VError VString::FromJSONString(const VString& inJSONString, JSONOption inModifier)
{
	if ((inModifier & JSON_AlreadyEscapedChars) != 0)
	{
		FromString(inJSONString);
	}
	else
	{
		sLONG len = inJSONString.GetLength();
		UniChar* dest = GetCPointerForWrite(len);
		const UniChar* source = inJSONString.GetCPointer();
		sLONG len2 = 0;

		for (sLONG i = 0; i < len; i++)
		{
			UniChar c = *source;
			source++;
			if (c == '\\')
			{
				c = *source;
				source++;
				i++;
				if (i < len)
				{
					switch (c)
					{
					case '\\':
					case '"':
					case '/':
						//  c is the same
						break;

					case 't':
						c = 9;
						break;

					case 'r':
						c = 13;
						break;

					case 'n':
						c = 10;
						break;

					case 'b':
						c = 8;
						break;

					case 'f':
						c = 12;
						break;

					case 'u':
				/*	2010-02-22 - T.A.
					http://www.ietf.org/rfc/rfc4627.txt
					. . .
					The application/json Media Type for JavaScript Object Notation (JSON)
					. . .
					Any character may be escaped.  If the character is in the Basic
					Multilingual Plane (U+0000 through U+FFFF), then it may be
					represented as a six-character sequence: a reverse solidus, followed
					by the lowercase letter u, followed by four hexadecimal digits that
					encode the character's code point.  The hexadecimal letters A though
					F can be upper or lowercase.  So, for example, a string containing
					only a single reverse solidus character may be represented as
					"\u005C".
					. . .
			   */
						if( testAssert( (i + 4) < len ) )
						{
							c = 0;
							UniChar	theChar;
							for(sLONG i_fromHex = 1; i_fromHex <= 4; ++i_fromHex)
							{
								theChar = *source++;
								if (theChar >= CHAR_DIGIT_ZERO && theChar <= CHAR_DIGIT_NINE)
								{
									c *= 16L;
									c += (theChar - CHAR_DIGIT_ZERO);
								}
								else if (theChar >= CHAR_LATIN_CAPITAL_LETTER_A && theChar <= CHAR_LATIN_CAPITAL_LETTER_F)
								{
									c *= 16L;
									c += (theChar - CHAR_LATIN_CAPITAL_LETTER_A) + 10L;
								}
								else if (theChar >= CHAR_LATIN_SMALL_LETTER_A && theChar <= CHAR_LATIN_SMALL_LETTER_F)
								{
									c *= 16L;
									c += (theChar - CHAR_LATIN_SMALL_LETTER_A) + 10L;
								}
							}
						}
						i += 4;
						break;
					}
					*dest = c;
					dest++;
					len2++;
				}
			}
			else
			{
				*dest = c;
				dest++;
				len2++;
			}
		}
		Validate(len2);
	}
	return VE_OK;
}


VError VString::GetJSONString(VString& outJSONString, JSONOption inModifier) const
{
	if ((inModifier & JSON_AlreadyEscapedChars) != 0)
	{
		GetString(outJSONString);
		if ((inModifier & JSON_WithQuotesIfNecessary) != 0)
		{
			outJSONString.Clear();
			outJSONString += UniChar('"');
			outJSONString += outJSONString;
			outJSONString += UniChar('"');
		}
	}
	else
	{
		sLONG len = GetLength();

		// ACI0069345: VString indexes are limited by VIndex size (2GB), check that.

		uLONG8	destinationLength = 6 * (uLONG8) len + 4;

		if (destinationLength > kMAX_sLONG)

			return XBOX::VE_STRING_ALLOC_FAILED;

		UniChar* dest = outJSONString.GetCPointerForWrite((VIndex) destinationLength);

		// VString::GetCPointerForWrite() may fail, check that.

		if (dest == NULL)

			return XBOX::VE_MEMORY_FULL;

		const UniChar* source = GetCPointer();
		sLONG len2 = 0;
		if ((inModifier & JSON_WithQuotesIfNecessary) != 0)
		{
			*dest = '"';
			dest++;
			len2++;
		}
		for (sLONG i = 0; i < len; i++)
		{
			UniChar c = *source;
			source++;
			switch (c)
			{

				case '"':
					*dest = '\\';
					dest++;
					*dest = '"';
					dest++;
					len2+=2;
					break;

					/*
				case '/':
					*dest = '\\';
					dest++;
					*dest = '/';
					dest++;
					len2+=2;
					break;
					*/

				case '\\':
					*dest = '\\';
					dest++;
					*dest = '\\';
					dest++;
					len2+=2;
					break;

				case 9:
					*dest = '\\';
					dest++;
					*dest = 't';
					dest++;
					len2+=2;
					break;

				case 13:
					*dest = '\\';
					dest++;
					*dest = 'r';
					dest++;
					len2+=2;
					break;

				case 10:
					*dest = '\\';
					dest++;
					*dest = 'n';
					dest++;
					len2+=2;
					break;

				case 12:
					*dest = '\\';
					dest++;
					*dest = 'f';
					dest++;
					len2+=2;
					break;

				case 8:
					*dest = '\\';
					dest++;
					*dest = 'b';
					dest++;
					len2+=2;
					break;

				default:
					if (c < 32 || c==127)
					{
						*dest = '\\';
						dest++;
						*dest = 'u';
						dest++;
						*dest = '0';
						dest++;
						*dest = '0';
						dest++;
						unsigned char c1 = c >> 4;
						unsigned char c2 = c & 0x000F;
						if (c1>9)
							c1 = c1-10+'A';
						else
							c1 = c1 + '0';
						if (c2>9)
							c2 = c2-10+'A';
						else
							c2 = c2 + '0';
						*dest = c1;
						dest++;
						*dest = c2;
						dest++;

						len2+=6;
					}
					else
					{
						#if 0
						if (c >= 768) // ceci est un fix rapide en attendant de trouver les caracteres qui posent problemes au parser JSON natif
						// disabled for ACI0080592. Don't remember what characters were actually causing problems. Keep the code in case the problem comes back.
						{
							*dest = '\\';
							dest++;
							*dest = 'u';
							dest++;
							UniChar xc1 = c >> 8;
							UniChar xc2 = c & 0x00FF;
							unsigned char cc[4];
							cc[0] = xc1 >> 4;
							cc[1] = xc1 & 0x000F;
							cc[2] = xc2 >> 4;
							cc[3] = xc2 & 0x000F;
							for (sLONG k = 0; k < 4; k++)
							{
								unsigned char c1 = cc[k];
								if (c1>9)
									c1 = c1-10+'A';
								else
									c1 = c1 + '0';
								*dest = c1;
								dest++;
							}
							len2+=6;
						}
						else
						#endif
						{
							*dest = c;
							dest++;
							len2++;
						}
					}
					break;
			}

		}
		if ((inModifier & JSON_WithQuotesIfNecessary) != 0)
		{
			*dest = '"';
			dest++;
			len2++;
		}
		outJSONString.Validate(len2);
	}

	return VE_OK;
}


VError VString::FromJSONValue( const VJSONValue& inJSONValue)
{
	if (inJSONValue.IsNull())
		SetNull( true);
	else
		inJSONValue.GetString( *this);
	return VE_OK;
}


VError VString::GetJSONValue( VJSONValue& outJSONValue, bool allowDates) const
{
	if (IsNull())
		outJSONValue.SetNull();
	else
		outJSONValue.SetString( *this);
	return VE_OK;
}


void VString::GetCleanJSName(VString& outJSName) const
{
	outJSName.Clear();
	sLONG len = fLength;
	const UniChar* p = fString;
	if (len > 0)
	{
		UniChar c = *p;
		if (c >= '0' && c <= '9')
			outJSName.AppendUniChar('_');
	}
	for (sLONG i = 0; i < len; ++i)
	{
		UniChar c = *p;
		++p;
		if (c >= 'a' && c <= 'z')
			outJSName.AppendUniChar(c);
		else if (c >= 'A' && c <= 'Z')
			outJSName.AppendUniChar(c);
		else if (c == '_' || c == '$')
			outJSName.AppendUniChar(c);
		else if (c >= '0' && c <= '9')
			outJSName.AppendUniChar(c);
	}
}



//==============================================================================================================================


StStringConverterBase::StStringConverterBase(CharSet inDestinationCharSet, ECarriageReturnMode inCRMode)
{
	fConverter = VTextConverters::Get()->RetainFromUnicodeConverter(inDestinationCharSet);

	fBuffer = fPrimaryBuffer;
	fBufferSize = sizeof(fPrimaryBuffer);
	fSecondaryBuffer = NULL;
	fCRMode = inCRMode;
	fStringSize = 0;
	fOK = false;
}


StStringConverterBase::~StStringConverterBase()
{
	vFree(fSecondaryBuffer);
	if (fConverter)
		fConverter->Release();
}


void StStringConverterBase::_ConvertString( const UniChar *inUniPtr, VIndex inLength, VSize inCharSize)
{
	fStringSize = 0;
	
	// always add a 0 at the end so keep 2 bytes
	VIndex charsConsumed;
	VSize bytesProduced;
	fOK = fConverter->Convert( inUniPtr, inLength, &charsConsumed, fBuffer, (fBufferSize - inCharSize), &bytesProduced);
	if (fOK)
	{
		if (inLength == charsConsumed)
		{
			fStringSize = bytesProduced;
		}
		else
		{
			// conversion was ok but no all caracters were converted.
			// -> use relocatable secondary buffer.
			
			bytesProduced = 0;
			fOK = fConverter->ConvertRealloc( inUniPtr, inLength, fSecondaryBuffer, bytesProduced, inCharSize);
			if (fOK)
			{
				fBuffer = fSecondaryBuffer;
				fBufferSize = bytesProduced;
				fStringSize = bytesProduced;
			}
		}
	}

	// add trailing 0
	::memset( (char*) fBuffer + fStringSize, 0, inCharSize);
}


VString operator +(const VString& s1, const VString& s2)
{
	VString res = s1;
	res += s2;
	return res;
}

VString ToString(sLONG n)
{
	VString res;
	res.FromLong(n);
	return res;
}

VString ToString(sLONG8 n)
{
	VString res;
	res.FromLong8(n);
	return res;
}

#if ARCH_32 || COMPIL_GCC
VString ToString(uLONG8 n)
{
	VString res;
	res.FromULong8(n);
	return res;
}
#endif

VString ToString(VSize n)
{
	VString res;
	res.FromLong8( n);
	return res;
}

END_TOOLBOX_NAMESPACE

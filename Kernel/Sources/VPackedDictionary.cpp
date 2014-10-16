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
#include "VStream.h"
#include "VPackedDictionary.h"


const uLONG _VHashKeyMap::tag_empty = 0xffffffffu;

size_t _VHashKeyMap::_ComputeHashVectorSize( size_t inCountValues)
{

	static const size_t sVHashKeyMap_Capacities[42] = {
	    4, 8, 17, 29, 47, 76, 123, 199, 322, 521, 843, 1364, 2207, 3571, 5778, 9349,
	    15127, 24476, 39603, 64079, 103682, 167761, 271443, 439204, 710647, 1149851, 1860498,
	    3010349, 4870847, 7881196, 12752043, 20633239, 33385282, 54018521, 87403803, 141422324,
	    228826127, 370248451, 599074578, 969323029, 1568397607, 2537720636U
	};

	static const size_t sVHashKeyMap_Buckets[42] = {	// primes
	    5, 11, 23, 41, 67, 113, 199, 317, 521, 839, 1361, 2207, 3571, 5779, 9349, 15121,
	    24473, 39607, 64081, 103681, 167759, 271429, 439199, 710641, 1149857, 1860503, 3010349,
	    4870843, 7881193, 12752029, 20633237, 33385273, 54018521, 87403763, 141422317, 228826121,
	    370248451, 599074561, 969323023, 1568397599, 2537720629U, 4106118251U
	};

    size_t i = 0;
    while( i < 42 && sVHashKeyMap_Capacities[i] < inCountValues)
    	++i;
    assert( i < 42);
    return sVHashKeyMap_Buckets[i];
}


bool _VHashKeyMap::Rebuild( size_t inCountValues)
{
	bool ok;
	try
	{
		hash_vector temp( _ComputeHashVectorSize( inCountValues), tag_empty);
		fIndex.swap( temp);
		ok = true;
	}
	catch(...)
	{
		clear();
		ok = false;
	}
	return ok;
}


bool _VHashKeyMap::Rebuild( VStream *inStream)
{
	bool ok;
	try
	{
		hash_vector temp( static_cast<size_t>( inStream->GetLong()), tag_empty);
		sLONG count = static_cast<sLONG>( temp.size());
		inStream->GetLongs( &temp.front(), &count);
		fIndex.swap( temp);
		ok = true;
	}
	catch(...)
	{
		clear();
		ok = false;
	}
	return ok;
}


bool _VHashKeyMap::Put( hashcode_type inHash, size_t inValue)
{
	bool ok;
	size_t probe = inHash % fIndex.size();
	size_t start = probe;
	for(;;)
	{
		if (fIndex[probe] == tag_empty)
		{
			fIndex[probe] = static_cast<uLONG>( inValue);
			ok = true;
			break;
		}
		
		++probe;
		if (probe >= fIndex.size())
			probe -= fIndex.size();
		if (probe == start)
		{
			ok = false;
			break;
		}
	}
	return ok;
}


VError _VHashKeyMap::WriteToStream( VStream *inStream)
{
	sLONG count = static_cast<sLONG>( fIndex.size());
	inStream->PutLong( count);
	inStream->PutLongs( &fIndex.front(), count);
	return inStream->GetLastError();
}


//================================================================================================================


StPackedDictionaryKey::StPackedDictionaryKey( const wchar_t *inKey)
: fKeyBuffer( new char_type[256])
{
	VFromUnicodeConverter_UTF8 converter;
	VIndex charsConsumed;
	VSize bytesProduced;

	sLONG input_length = static_cast<sLONG>( ::wcslen( inKey));
	Boolean conversionOK = converter.ConvertFrom_wchar( inKey, input_length, &charsConsumed, fKeyBuffer, 255, &bytesProduced);
	if (!testAssert( conversionOK && (charsConsumed == input_length)))
		bytesProduced = 0;
	fLength = static_cast<size_t>( bytesProduced);
	fKey = fKeyBuffer;
	fKeyBuffer[fLength] = 0;
	fHashCode = GetHashCode( fKey, fLength);
}


StPackedDictionaryKey::StPackedDictionaryKey( const VString& inKey)
: fKeyBuffer( new char_type[256])
, fHashCode(0)
{
	VFromUnicodeConverter_UTF8 converter;
	VIndex charsConsumed;
	VSize bytesProduced;
	bool conversionOK = converter.Convert( inKey.GetCPointer(), inKey.GetLength(), &charsConsumed, fKeyBuffer, 255, &bytesProduced);
	if (!testAssert( conversionOK && (charsConsumed == inKey.GetLength())))
		bytesProduced = 0;
	fLength = static_cast<size_t>( bytesProduced);
	fKey = fKeyBuffer;
	fKeyBuffer[fLength] = 0;
	fHashCode = GetHashCode( fKey, fLength);
}


StPackedDictionaryKey::StPackedDictionaryKey( const char *inKey, size_t inLength)
: fLength( inLength)
, fHashCode(GetHashCode( inKey, inLength))
{
	fKeyBuffer = new char_type[inLength + 1];
	fKey = fKeyBuffer;
	if (fKeyBuffer != NULL)
	{
		::memcpy( fKeyBuffer, inKey, inLength + 1);
		fLength = inLength;
	}
	else
	{
		fLength = 0;
	}
}


void StPackedDictionaryKey::_CopyFrom( const StPackedDictionaryKey& inOther)
{
	fHashCode = inOther.fHashCode;
	if (inOther.fKeyBuffer == NULL)
	{
		fKeyBuffer = NULL;
		fKey = inOther.fKey;
		fLength = inOther.fLength;
	}
	else
	{
		xbox_assert( inOther.fKey == inOther.fKeyBuffer);
		fKeyBuffer = new char_type[inOther.fLength + 1];
		fKey = fKeyBuffer;
		if (fKeyBuffer != NULL)
		{
			::memcpy( fKeyBuffer, inOther.fKeyBuffer, inOther.fLength + 1);
			fLength = inOther.fLength;
		}
		else
		{
			fLength = 0;
		}
	}
}


_VHashKeyMap::hashcode_type StPackedDictionaryKey::GetHashCode( const char_type *inKey, size_t inLength)
{
	// inspired from cfstring: for > 16 chars, consider only first and last 8 chars.
	// always compute with 32 bits integers
	uLONG result = static_cast<uLONG>( inLength);
	const uBYTE *key = reinterpret_cast<const uBYTE*>( inKey);	// use unsigned bytes
	const uBYTE *i;
	if (inLength <= 16)
	{
		for( i = key ; i != key + inLength ; ++i)
			result = result * 257 + *i;
	}
	else
	{
		for( i = key ; i != key + 8 ; ++i)
			result = result * 257 + *i;
		for( i = key + inLength - 8 ; i != key + inLength ; ++i)
			result = result * 257 + *i;
	}
	return (_VHashKeyMap::hashcode_type) result;
}



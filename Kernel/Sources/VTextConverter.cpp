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
#include "VTextConverter.h"
#include "VString.h"
#include "VError.h"
#include "VMemoryCpp.h"
#include "VByteSwap.h"
#include "VIntlMgr.h"

#include "VCharSetNames.h"

VTextConverters*		VTextConverters::sInstance = NULL;


// ---------------------------------------------------------------------------
//  Local static data
//
//  sUTFBytes
//	  A list of counts of trailing bytes for each initial byte in the input.
//
//  sUTFOffsets
//	  A list of values to offset each result char type, according to how
//	  many source bytes when into making it.
//
//  sFirstByteMark
//	  A list of values to mask onto the first byte of an encoded sequence,
//	  indexed by the number of bytes used to create the sequence.
// ---------------------------------------------------------------------------
static const uBYTE sUTFBytes[256] =
{
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	,   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	,   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	,   2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
	,   3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5
};


static const uLONG sUTFOffsets[6] =
{
	0, 0x3080, 0xE2080, 0x3C82080, 0xFA082080, 0x82082080
};


static const uBYTE sFirstByteMark[7] =
{
	0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC
};


/*
 * Copyright 2001-2004 Unicode, Inc.
 * 
 * Disclaimer
 * 
 * This source code is provided as is by Unicode, Inc. No claims are
 * made as to fitness for any particular purpose. No warranties of any
 * kind are expressed or implied. The recipient agrees to determine
 * applicability of information provided. If this file has been
 * purchased on magnetic or optical media from Unicode, Inc., the
 * sole remedy for any claim will be exchange of defective media
 * within 90 days of receipt.
 * 
 * Limitations on Rights to Redistribute This Code
 * 
 * Unicode, Inc. hereby grants the right to freely use the information
 * supplied in this file in the creation of products supporting the
 * Unicode Standard, and to make copies of this file in any form
 * for internal or external distribution as long as this notice
 * remains attached.
 */

static const int halfShift  = 10; /* used for shifting by 10 bits */

static const uLONG halfBase = 0x0010000UL;
static const uLONG halfMask = 0x3FFUL;

#define UNI_SUR_HIGH_START  (uLONG)0xD800
#define UNI_SUR_HIGH_END    (uLONG)0xDBFF
#define UNI_SUR_LOW_START   (uLONG)0xDC00
#define UNI_SUR_LOW_END     (uLONG)0xDFFF

/* Some fundamental constants */
#define UNI_REPLACEMENT_CHAR (uLONG)0x0000FFFD
#define UNI_MAX_BMP (uLONG)0x0000FFFF
#define UNI_MAX_LEGAL_UTF32 (uLONG)0x0010FFFF

typedef enum {
	conversionOK, 		/* conversion successful */
	sourceExhausted,	/* partial character in source, but hit end */
	targetExhausted,	/* insuff. room in target for conversion */
	sourceIllegal		/* source sequence is illegal/malformed */
} ConversionResult;

typedef enum {
	strictConversion = 0,
	lenientConversion
} ConversionFlags;


/* --------------------------------------------------------------------- */


static
ConversionResult ConvertUTF32toUTF16(	const uLONG** sourceStart, const uLONG* sourceEnd, 
										uWORD** targetStart, uWORD* targetEnd,
										ConversionFlags flags, bool needSwap)
{
    ConversionResult result = conversionOK;
    const uLONG* source = *sourceStart;
    uWORD* target = *targetStart;
    while (source < sourceEnd)
	{
		uLONG ch;
		if (target >= targetEnd)
		{
			result = targetExhausted; break;
		}
		ch = *source++;
		if (needSwap)
			ch = XBOX::ByteSwapValue (ch);

		if (ch <= UNI_MAX_BMP)	/* Target is a character <= 0xFFFF */
		{
			/* UTF-16 surrogate values are illegal in UTF-32; 0xffff or 0xfffe are both reserved values */
			if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END)
			{
				if (flags == strictConversion)
				{
					--source; /* return to the illegal value itself */
					result = sourceIllegal;
					break;
				}
				else
				{
					*target++ = UNI_REPLACEMENT_CHAR;
				}
			}
			else
			{
				*target++ = (uWORD)ch; /* normal case */
			}
		}
		else if (ch > UNI_MAX_LEGAL_UTF32)
		{
			if (flags == strictConversion)
			{
				result = sourceIllegal;
			}
			else
			{
				*target++ = UNI_REPLACEMENT_CHAR;
			}
		}
		else
		{
			/* target is a character in range 0xFFFF - 0x10FFFF. */
			if (target + 1 >= targetEnd)
			{
				--source; /* Back up source pointer! */
				result = targetExhausted; break;
			}
			ch -= halfBase;
			*target++ = (uWORD)((ch >> halfShift) + UNI_SUR_HIGH_START);
			*target++ = (uWORD)((ch & halfMask) + UNI_SUR_LOW_START);
		}
    }

	*sourceStart = source;
	*targetStart = target;

	return result;
}


/* --------------------------------------------------------------------- */


static
ConversionResult ConvertUTF16toUTF32 (	const uWORD** sourceStart, const uWORD* sourceEnd, 
										uLONG** targetStart, uLONG* targetEnd,
										ConversionFlags flags, bool needSwap)
{
    ConversionResult result = conversionOK;
    const uWORD* source = *sourceStart;
    uLONG* target = *targetStart;
    uLONG ch, ch2;
    while (source < sourceEnd)
	{
		const uWORD* oldSource = source; /*  In case we have to back up because of target overflow. */
		ch = *source++;
		if (needSwap)
			ch = XBOX::ByteSwapValue (ch);
		/* If we have a surrogate pair, convert to uLONG first. */
		if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END)
		{
			/* If the 16 bits following the high surrogate are in the source buffer... */
			if (source < sourceEnd)
			{
				ch2 = *source;
				/* If it's a low surrogate, convert to uLONG. */
				if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END)
				{
					ch = ((ch - UNI_SUR_HIGH_START) << halfShift)
					+ (ch2 - UNI_SUR_LOW_START) + halfBase;
					++source;
				}
				else if (flags == strictConversion) /* it's an unpaired high surrogate */
				{
					--source; /* return to the illegal value itself */
					result = sourceIllegal;
					break;
				}
			}
			else	/* We don't have the 16 bits following the high surrogate. */
			{
				--source; /* return to the high surrogate */
				result = sourceExhausted;
				break;
			}
		}
		else if (flags == strictConversion)
		{
			/* UTF-16 surrogate values are illegal in UTF-32 */
			if (ch >= UNI_SUR_LOW_START && ch <= UNI_SUR_LOW_END)
			{
				--source; /* return to the illegal value itself */
				result = sourceIllegal;
				break;
			}
		}
		if (target >= targetEnd)
		{
			source = oldSource; /* Back up source pointer! */
			result = targetExhausted; break;
		}
		*target++ = ch;
    }

	*sourceStart = source;
    *targetStart = target;

#ifdef CVTUTF_DEBUG
	if (result == sourceIllegal) {
		fprintf(stderr, "ConvertUTF16toUTF32 illegal seq 0x%04x,%04x\n", ch, ch2);
		fflush(stderr);
	}
#endif

	return result;
}


/* --------------------------------------------------------------------- */


bool VToUnicodeConverter::IsValid() const
{
	return true;
}


bool VToUnicodeConverter::ConvertString(const void* inSource, VSize inSourceBytes, VSize *outBytesConsumed, VString& outDestination)
{
	bool isOK = true;
	
	if (outBytesConsumed != NULL)
		*outBytesConsumed = 0;
	VIndex oldLength = outDestination.GetLength();
	
	// make room for conversion
	VIndex destinationChars = (VIndex) (inSourceBytes + 1);
	
	UniChar *p = outDestination.GetCPointerForWrite( oldLength + destinationChars);
	if (p != NULL)
	{
		VSize bytesConsumed;
		VIndex charsProduced;
		
		isOK = Convert(inSource, inSourceBytes, &bytesConsumed, p + oldLength, destinationChars, &charsProduced);
		
		if (isOK)
		{
			
			if (outBytesConsumed != NULL)
				*outBytesConsumed = bytesConsumed;
			outDestination.Validate(oldLength + charsProduced);

			if (bytesConsumed != inSourceBytes)
			{
				// some bytes remain
				
				if (charsProduced == 0) {
					// nothing was converted so stop now.
					// that must be because the given buffer terminates on an incomplete char.
					// -> we should be near the end of the buffer (the last char).
					assert(inSourceBytes - bytesConsumed <= 4);
				
				} else {
					// recurse
					isOK = ConvertString((char *)inSource + bytesConsumed, inSourceBytes - bytesConsumed, &bytesConsumed, outDestination);
					if (isOK) {
						if (outBytesConsumed != NULL)
							*outBytesConsumed += bytesConsumed;
					} else {
						// abort and set the string back to what it was.
						outDestination.Validate(oldLength);
						if (outBytesConsumed != NULL)
							*outBytesConsumed = 0;
					}
				}
			}
		} else {
			outDestination.Validate(oldLength);
		}
		
	} else {
		isOK = false;
		vThrowError(VE_INTL_TEXT_CONVERSION_FAILED, "conversion failed (EnsureSize failed).");
	}
	
	return isOK;
}


bool VFromUnicodeConverter::IsValid() const
{
	return true;
}


bool VFromUnicodeConverter::ConvertRealloc(const UniChar* inSource, VIndex inSourceChars, void* & ioBuffer, VSize& ioBytesInBuffer, VSize inTrailingBytes)
{
	bool isOK = true;
	
	// start with a guessed size
	VSize newBufferSize = ioBytesInBuffer + (inSourceChars + 2) * GetCharSize()/*sizeof(UniChar)*/ + inTrailingBytes;
	char *buffer = (char *) vRealloc(ioBuffer, newBufferSize);
	
	if (buffer == NULL)
	{
		vThrowError(VE_INTL_TEXT_CONVERSION_FAILED, "mem failure");
		isOK = false;
	}
	else
	{
		ioBuffer = buffer;
		
		VIndex totalCharsConsumed = 0;
		VIndex charsConsumed;
		VSize bytesProduced;
		isOK = Convert( inSource, inSourceChars, &charsConsumed, buffer + ioBytesInBuffer, newBufferSize - ioBytesInBuffer - inTrailingBytes, &bytesProduced);
		if (isOK)
		{
			totalCharsConsumed += charsConsumed;
			ioBytesInBuffer += bytesProduced;
			while( (totalCharsConsumed != inSourceChars) && (bytesProduced != 0) )
			{
				// some chars remain
				xbox_assert(bytesProduced != 0);
				xbox_assert(totalCharsConsumed < inSourceChars);

				isOK = Convert( inSource + totalCharsConsumed, inSourceChars - totalCharsConsumed, &charsConsumed, buffer + ioBytesInBuffer, newBufferSize - ioBytesInBuffer - inTrailingBytes, &bytesProduced);
				if (!isOK)
					break;

				totalCharsConsumed += charsConsumed;
				ioBytesInBuffer += bytesProduced;
			}
			
			if (isOK && (inSourceChars>0) && (bytesProduced == 0) && (totalCharsConsumed > 0) )	
			{
				// means too small buffer 
				// recurse
				isOK = ConvertRealloc( inSource + totalCharsConsumed, inSourceChars - totalCharsConsumed, ioBuffer, ioBytesInBuffer, inTrailingBytes);
			}
		}
	}

	return isOK;
}


bool VToUnicodeConverter_Null::Convert(const void* inSource, VSize inSourceBytes, VSize *outBytesConsumed, UniChar* inDestination, VIndex inDestinationChars, VIndex *outProducedChars)
{
	*outBytesConsumed = Min(inSourceBytes, (VSize) (inDestinationChars * sizeof(UniChar)));
	if (*outBytesConsumed % sizeof(UniChar) != 0)
		*outBytesConsumed -= 1;
	
	::memmove(inDestination, inSource, *outBytesConsumed);
	*outProducedChars = (VIndex) (*outBytesConsumed / sizeof(UniChar));
	
	return true;
}


bool VToUnicodeConverter_Null::ConvertString(const void* inSource, VSize inSourceBytes, VSize *outBytesConsumed, VString& outDestination)
{
	outDestination.AppendBlock(inSource, inSourceBytes, VTC_UTF_16);
	if (outBytesConsumed != NULL)
		*outBytesConsumed = inSourceBytes;
	return true;
}



bool VFromUnicodeConverter_Null::Convert(const UniChar* inSource, VIndex inSourceChars, VIndex *outCharsConsumed, void* inBuffer, VSize inBufferSize, VSize *outBytesProduced)
{
	*outBytesProduced = Min((VSize) (inSourceChars * sizeof(UniChar)), inBufferSize);
	if (*outBytesProduced % 2)
		*outBytesProduced -= 1;
	
	::memmove(inBuffer, inSource, *outBytesProduced);
	*outCharsConsumed = (VIndex) (*outBytesProduced / sizeof(UniChar));

	return true;
}


bool VToUnicodeConverter_UTF32::Convert (const void *inSource, VSize inSourceBytes, VSize *outBytesConsumed, UniChar *inDestination, VIndex inDestinationChars, VIndex *outProducedChars)
{
	const uLONG *sourceStart = (uLONG *)(inSource);
	uLONG *sourceEnd = (uLONG *)((sBYTE*)inSource + inSourceBytes);
	
	uWORD *targetStart = (uWORD*)inDestination;
	uWORD *targetEnd = (uWORD*)(inDestination + inDestinationChars);

	ConversionResult result = ConvertUTF32toUTF16(	&sourceStart,
													sourceEnd,
													&targetStart,
													targetEnd,
													strictConversion,
													fSwap);
	
	*outBytesConsumed = (sBYTE*)sourceStart - (sBYTE*)inSource;
	*outProducedChars  = (targetStart - inDestination);
	

	return (result == conversionOK);
}



bool VFromUnicodeConverter_UTF32::Convert( const UniChar *inSource, VIndex inSourceChars, VIndex *outCharsConsumed, void* inBuffer, VSize inBufferSize, VSize *outBytesProduced)
{
	const UniChar *	sourceStart = inSource;
	const UniChar *	sourceEnd = inSource + inSourceChars;

	uLONG *			destinationStart = (uLONG*)inBuffer;
	uLONG *			destinationEnd = (uLONG*)((char *)inBuffer + inBufferSize);

	ConversionResult result =  ConvertUTF16toUTF32 (&sourceStart,
													sourceEnd,
													&destinationStart,
													destinationEnd,
													strictConversion,
													fSwap);

	*outCharsConsumed = inSourceChars - (sLONG) (((sourceEnd - sourceStart) / sizeof(UniChar)));
	if (result == conversionOK)
		*outBytesProduced = (char*) destinationStart - (char*) inBuffer;
	else
		*outBytesProduced = inBufferSize - (destinationEnd - destinationStart);

	return (result == conversionOK);
}


bool VToUnicodeConverter_UTF32_Raw::Convert(const void* inSource, VSize inSourceBytes, VSize *outBytesConsumed, UniChar* inDestination, VIndex inDestinationChars, VIndex *outProducedChars)
{
	*outBytesConsumed = Min(inSourceBytes, (VSize) (inDestinationChars * 4));
	*outBytesConsumed -= *outBytesConsumed % 4;

	UniChar* pSource = (UniChar*) ((char*) inSource + *outBytesConsumed);
	if ((BIGENDIAN && !fSwap) || (SMALLENDIAN && fSwap))
		pSource -= 1;
	else
		pSource -= 2;

	UniChar* pDestination = inDestination + (*outBytesConsumed / 4) - 1;

	if (fSwap) {
		while(pDestination >= inDestination) {
			*pDestination = (UniChar) ((*pSource << 8) | (*pSource >> 8));
			pSource -= 2;
			pDestination -= 1;
		}
	} else {
		while(pDestination >= inDestination) {
			*pDestination = *pSource;
			pSource -= 2;
			pDestination -= 1;
		}
	}
	
	*outProducedChars = (VIndex) (*outBytesConsumed / 4);
	
	return true;
}


bool VToUnicodeConverter_UTF32_Raw::ConvertString(const void* inSource, VSize inSourceBytes, VSize *outBytesConsumed, VString& outDestination)
{
	VIndex nbChars = (VIndex) (inSourceBytes / 4);
	
	UniChar *p = outDestination.GetCPointerForWrite( outDestination.GetLength() + nbChars);
	if (p == NULL)
	{
		if (outBytesConsumed != NULL)
			*outBytesConsumed = 0;
		return false;
	}
	
	VSize bytesConsumed = inSourceBytes - inSourceBytes % 4;

	UniChar* pSource = (UniChar*) ((char*) inSource + bytesConsumed);
	if ((BIGENDIAN && !fSwap) || (SMALLENDIAN && fSwap))
		pSource -= 1;
	else
		pSource -= 2;
		
	UniChar* pFirst = p + outDestination.GetLength();
	UniChar* pDestination = pFirst + nbChars - 1;

	if (fSwap) {
		while(pDestination >= pFirst) {
			*pDestination = (UniChar) ((*pSource << 8) | (*pSource >> 8));
			pSource -= 2;
			pDestination -= 1;
		}
	} else {
		while(pDestination >= pFirst) {
			*pDestination = *pSource;
			pSource -= 2;
			pDestination -= 1;
		}
	}
	outDestination.Validate(outDestination.GetLength() + nbChars);

	if (outBytesConsumed != NULL)
		*outBytesConsumed = bytesConsumed;
	
	return true;
}


bool VFromUnicodeConverter_UTF32_Raw::Convert(const UniChar* inSource, VIndex inSourceChars, VIndex *outCharsConsumed, void* inBuffer, VSize inBufferSize, VSize *outBytesProduced)
{
	*outBytesProduced = Min((VSize) (inSourceChars * 4), inBufferSize);
	*outBytesProduced -= *outBytesProduced % 4;
	*outCharsConsumed = (VIndex) (*outBytesProduced / 4);

	const UniChar* pSource = inSource + *outCharsConsumed - 1;
	uLONG *pDestination = (uLONG *) inBuffer + *outCharsConsumed - 1;
	
	while(pSource >= inSource)
		*pDestination-- = *pSource--;

	if (fSwap)
		ByteSwapLongArray((sLONG* ) inBuffer, *outCharsConsumed);

	return true;
}


bool VToUnicodeConverter_Swap::ConvertString(const void* inSource, VSize inSourceBytes, VSize *outBytesConsumed, VString& outDestination)
{
	VIndex beforeLength = outDestination.GetLength();

	outDestination.AppendBlock(inSource, inSourceBytes, VTC_UTF_16);
	if (outBytesConsumed != NULL)
		*outBytesConsumed = inSourceBytes;
	
	if(inSourceBytes > 0)
		ByteSwapWordArray((sWORD *) &outDestination[beforeLength], outDestination.GetLength() - beforeLength);

	return true;
}


bool VToUnicodeConverter_Swap::Convert(const void* inSource, VSize inSourceBytes, VSize *outBytesConsumed, UniChar* inDestination, VIndex inDestinationChars, VIndex *outProducedChars)
{
	*outBytesConsumed = Min(inSourceBytes, (VSize) (inDestinationChars * sizeof(UniChar)));
	if (*outBytesConsumed % sizeof(UniChar) != 0)
		*outBytesConsumed -= 1;
	
	::memmove(inDestination, inSource, *outBytesConsumed);
	*outProducedChars = (VIndex) (*outBytesConsumed / sizeof(UniChar));

	ByteSwapWordArray((sWORD *) inDestination, (sLONG) (*outBytesConsumed / sizeof(UniChar)));
	
	return true;
}


bool VFromUnicodeConverter_Swap::Convert(const UniChar* inSource, VIndex inSourceChars, VIndex *outCharsConsumed, void* inBuffer, VSize inBufferSize, VSize *outBytesProduced)
{
	*outBytesProduced = Min((VSize) (inSourceChars * sizeof(UniChar)), inBufferSize);
	if (*outBytesProduced % 2)
		*outBytesProduced -= 1;
	
	::memmove(inBuffer, inSource, *outBytesProduced);
	*outCharsConsumed = (VIndex) (*outBytesProduced / sizeof(UniChar));

	ByteSwapWordArray((sWORD *) inBuffer, *outCharsConsumed);

	return true;
}


bool VToUnicodeConverter_Void::Convert(const void* /*inSource*/, VSize /*inSourceBytes*/, VSize *outBytesConsumed, UniChar* /*inDestination*/, VIndex /*inDestinationChars*/, VIndex *outProducedChars)
{
	*outBytesConsumed = 0;
	*outProducedChars = 0;	
	return false;
}


bool VToUnicodeConverter_Void::IsValid() const
{
	return false;
}


bool VFromUnicodeConverter_Void::Convert(const UniChar* /*inSource*/, VIndex /*inSourceChars*/, VIndex *outCharsConsumed, void* /*inBuffer*/, VSize /*inBufferSize*/, VSize *outBytesProduced)
{
	*outCharsConsumed = 0;
	*outBytesProduced = 0;
	return false;
}


bool VFromUnicodeConverter_Void::IsValid() const
{
	return false;
}


// ---------------------------------------------------------------------------
//  XMLUTF8Transcoder: Implementation of the transcoder API
//	From Xerces
// ---------------------------------------------------------------------------

bool VToUnicodeConverter_UTF8::Convert(const void* inSource, VSize inSourceBytes, VSize *outBytesConsumed, UniChar* inDestination, VIndex inDestinationChars, VIndex *outProducedChars)
{
	bool isOK = true;
	
	//
	//  Get pointers to our start and end points of the input and output
	//  buffers.
	//
	const uBYTE *srcPtr = (uBYTE *) inSource;
	const uBYTE *srcEnd = srcPtr + inSourceBytes;
	UniChar	*outPtr = inDestination;
	UniChar* outEnd = outPtr + inDestinationChars;
	
	//
	//  We now loop until we either run out of input data, or room to store
	//  output chars.
	//
	while ((srcPtr < srcEnd) && (outPtr < outEnd))
	{
		// Get the next leading byte out
		const uBYTE firstByte = *srcPtr;

		// Special-case ASCII, which is a leading byte value of <= 127
		if (firstByte <= 127)
		{
			*outPtr++ = UniChar(firstByte);
			srcPtr++;
			continue;
		}

		// See how many trailing src bytes this sequence is going to require
		const unsigned int trailingBytes = sUTFBytes[firstByte];

		//
		//  If there are not enough source bytes to do this one, then we
		//  are done. Note that we done >= here because we are implicitly
		//  counting the 1 byte we get no matter what.
		//
		//  If we break out here, then there is nothing to undo since we
		//  haven't updated any pointers yet.
		//
		if (srcPtr + trailingBytes >= srcEnd)
			break;

		// Looks ok, so lets build up the value
		uLONG tmpVal = 0;
		switch(trailingBytes)
		{
			case 5 : tmpVal += *srcPtr++; tmpVal <<= 6;
			case 4 : tmpVal += *srcPtr++; tmpVal <<= 6;
			case 3 : tmpVal += *srcPtr++; tmpVal <<= 6;
			case 2 : tmpVal += *srcPtr++; tmpVal <<= 6;
			case 1 : tmpVal += *srcPtr++; tmpVal <<= 6;
			case 0 : tmpVal += *srcPtr++;
					 break;

			default :
				isOK = false;
				goto failure;
		}
		tmpVal -= sUTFOffsets[trailingBytes];

		//
		//  If it will fit into a single char, then put it in. Otherwise
		//  encode it as a surrogate pair. If its not valid, use the
		//  replacement char.
		//
		if (!(tmpVal & 0xFFFF0000))
		{
			*outPtr++ = UniChar(tmpVal);
		}
		else if (tmpVal > 0x10FFFF)
		{
			isOK = false;
			goto failure;
		}
		else
		{
			//
			//  If we have enough room to store the leading and trailing
			//  chars, then lets do it. Else, pretend this one never
			//  happened, and leave it for the next time. Since we don't
			//  update the bytes read until the bottom of the loop, by
			//  breaking out here its like it never happened.
			//
			if (outPtr + 1 >= outEnd)
				break;

			// Store the leading surrogate char
			tmpVal -= 0x10000;
			*outPtr++ = UniChar((tmpVal >> 10) + 0xD800);

			//
			//  And then the treailing char. This one accounts for no
			//  bytes eaten from the source, so set the char size for this
			//  one to be zero.
			//
			*outPtr++ = (UniChar) ((tmpVal & 0x3FF) + 0xDC00);
		}
	}

failure:
	// Update the bytes eaten
	*outBytesConsumed = srcPtr - (uBYTE *) inSource;

	// Return the characters read
	*outProducedChars = (sLONG) (outPtr - inDestination);
	
	return isOK;
}


template <class T>
static bool _Convert(const T* inSource, VIndex inSourceChars, VIndex *outCharsConsumed, void* inBuffer, VSize inBufferSize, VSize *outBytesProduced)
{
	// can accept NULL for inBuffer, it will only calculate result size

	bool isOK = true;

    //
    //  Get pointers to our start and end points of the input and output
    //  buffers.
    //
    const T	*srcPtr = inSource;
    const T	*srcEnd = srcPtr + inSourceChars;
    uBYTE *outPtr = (uBYTE *) inBuffer;
    uBYTE *outEnd = outPtr + inBufferSize;

    while (srcPtr < srcEnd)
    {
        //
        //  Tentatively get the next char out. We have to get it into a
        //  32 bit value, because it could be a surrogate pair.
        //
        uLONG curVal = static_cast<uLONG>( *srcPtr);

        //
        //  If its a leading surrogate, then lets see if we have the trailing
        //  available. If not, then give up now and leave it for next time.
        //
        unsigned int srcUsed = 1;
        if ((curVal >= 0xD800) && (curVal <= 0xDBFF))
        {
            if (srcPtr + 1 >= srcEnd)
                break;

            // Create the composite surrogate pair
            curVal = ((curVal - 0xD800) << 10) + ((*(srcPtr + 1) - 0xDC00) + 0x10000);

            // And indicate that we ate another one
            srcUsed++;
        }

        // Figure out how many bytes we need
        unsigned int encodedBytes;
        if (curVal < 0x80)
            encodedBytes = 1;
        else if (curVal < 0x800)
            encodedBytes = 2;
        else if (curVal < 0x10000)
            encodedBytes = 3;
        else if (curVal < 0x200000)
            encodedBytes = 4;
        else if (curVal < 0x4000000)
            encodedBytes = 5;
        else if (curVal <= 0x7FFFFFFF)
            encodedBytes = 6;
        else
        {            
            isOK = false;
            goto failure;
        }

        //
        //  If we cannot fully get this char into the output buffer,
        //  then leave it for the next time.
        //
        if (outPtr + encodedBytes > outEnd)
            break;

        // We can do it, so update the source index
        srcPtr += srcUsed;

        //
        //  And spit out the bytes. We spit them out in reverse order
        //  here, so bump up the output pointer and work down as we go.
        //
        outPtr += encodedBytes;
		if (inBuffer != NULL)
		{
			switch(encodedBytes)
			{
				case 6 : *--outPtr = uBYTE((curVal | 0x80UL) & 0xBFUL);
						 curVal >>= 6;
				case 5 : *--outPtr = uBYTE((curVal | 0x80UL) & 0xBFUL);
						 curVal >>= 6;
				case 4 : *--outPtr = uBYTE((curVal | 0x80UL) & 0xBFUL);
						 curVal >>= 6;
				case 3 : *--outPtr = uBYTE((curVal | 0x80UL) & 0xBFUL);
						 curVal >>= 6;
				case 2 : *--outPtr = uBYTE((curVal | 0x80UL) & 0xBFUL);
						 curVal >>= 6;
				case 1 : *--outPtr = uBYTE(curVal | sFirstByteMark[encodedBytes]);
			}

			// Add the encoded bytes back in again to indicate we've eaten them
			outPtr += encodedBytes;
		}
    }

failure:

    // Fill in the chars we ate
    *outCharsConsumed = (sLONG) (srcPtr - inSource);

    // And return the bytes we filled in
    *outBytesProduced = (outPtr - (uBYTE *) inBuffer);
    
    return isOK;
}


bool VFromUnicodeConverter_UTF8::Convert(const UniChar* inSource, VIndex inSourceChars, VIndex *outCharsConsumed, void* inBuffer, VSize inBufferSize, VSize *outBytesProduced)
{
	return _Convert( inSource, inSourceChars, outCharsConsumed, inBuffer, inBufferSize, outBytesProduced);
}


bool VFromUnicodeConverter_UTF8::ConvertFrom_wchar( const wchar_t* inSource, VIndex inSourceChars, VIndex *outCharsConsumed, void* inBuffer, VSize inBufferSize, VSize *outBytesProduced)
{
	return _Convert( inSource, inSourceChars, outCharsConsumed, inBuffer, inBufferSize, outBytesProduced);
}



//==========================================================================================


// keep VTC_SystemCharSet definition private
// cause it does not make sense outside win or mac specific code.
#if VERSIONMAC
#define VTC_SystemCharset VTC_MacOSAnsi
#elif VERSION_LINUX
#define VTC_SystemCharset VTC_LinuxAnsi
#elif VERSIONWIN
#define VTC_SystemCharset VTC_Win32Ansi
#endif

VTextConverters *VTextConverters::Get()
{
	/**
		should be thread safe but it's called so early there's no multi-thread yet.
	**/
	if (sInstance == NULL)
		sInstance = new VTextConverters;
	return sInstance;
}


VTextConverters::VTextConverters()
{
	fFromUniConverter = XIntlMgrImpl::NewFromUnicodeConverter( VTC_SystemCharset);
	fToUniConverter = XIntlMgrImpl::NewToUnicodeConverter( VTC_SystemCharset);

	fToUniNullConverter = new VToUnicodeConverter_Null;
	fFromUniNullConverter = new VFromUnicodeConverter_Null;

	fToUniSwapConverter = new VToUnicodeConverter_Swap;
	fFromUniSwapConverter = new VFromUnicodeConverter_Swap;

	fToUni32RawConverter = new VToUnicodeConverter_UTF32_Raw(false);
	fFromUni32RawConverter = new VFromUnicodeConverter_UTF32_Raw(false);

	fToUni32Converter = new VToUnicodeConverter_UTF32 (false);
	fFromUni32Converter = new VFromUnicodeConverter_UTF32 (false);

	fToUTF8Converter = new VToUnicodeConverter_UTF8;
	fFromUTF8Converter = new VFromUnicodeConverter_UTF8;
}


VTextConverters::~VTextConverters()
{
	ReleaseRefCountable( &fToUniConverter);
	ReleaseRefCountable( &fFromUniConverter);

	ReleaseRefCountable( &fToUniNullConverter);
	ReleaseRefCountable( &fFromUniNullConverter);

	ReleaseRefCountable( &fToUniSwapConverter);
	ReleaseRefCountable( &fFromUniSwapConverter);
	
	ReleaseRefCountable( &fToUni32RawConverter);
	ReleaseRefCountable( &fFromUni32RawConverter);
	
	ReleaseRefCountable( &fToUni32Converter);
	ReleaseRefCountable( &fFromUni32Converter);

	ReleaseRefCountable( &fToUTF8Converter);
	ReleaseRefCountable( &fFromUTF8Converter);
}


void VTextConverters::GetNameFromCharSet(CharSet inCharSet, VString &outName)
{
	outName.Clear();

	for (VCSNameMap* mt = sCharSetNameMap; mt->fName ; mt++)
	{
		if (mt->fCharSet == inCharSet)
		{
			outName.FromUniCString(mt->fName);
			break;
		}
	}
}

 
CharSet VTextConverters::GetCharSetFromName(const VString &inName)
{
	CharSet result = VTC_UNKNOWN;
	for (VCSNameMap* mt = sCharSetNameMap; mt->fName ; mt++)
	{
		VString vname(const_cast<wchar_t*>(mt->fName), (VSize) -1);
		if (vname == inName)
		{
			result = mt->fCharSet;
			break;
		}
	}
	return	result;
}


sLONG VTextConverters::GetMIBEnumFromCharSet( CharSet inCharSet )
{
	sLONG mib = 0;

	for (VCSNameMap* mt = sCharSetNameMap; mt->fName ; mt++)
	{
		if (mt->fCharSet == inCharSet)
		{
			mib = mt->fMIBEnum;
			break;
		}
	}

	return mib;
}


sLONG VTextConverters::GetMIBEnumFromName( const VString &inName )
{
	sLONG mib = 0;

	for (VCSNameMap* mt = sCharSetNameMap; mt->fName ; mt++)
	{
		VString name(const_cast<wchar_t*>(mt->fName), (VSize) -1);
		if( name == inName )
		{
			mib = mt->fMIBEnum;
			break;
		}
	}

	return	mib;
}


CharSet VTextConverters::GetCharSetFromMIBEnum( const sLONG inMIBEnum )
{
	CharSet result = VTC_UNKNOWN;

	for( VCSNameMap* mt = sCharSetNameMap; mt->fName ; mt++ )
	{
		if( mt->fMIBEnum == inMIBEnum )
		{
			result = mt->fCharSet;
			break;
		}
	}

	return	result;
}


void VTextConverters::GetNameFromMIBEnum( const sLONG inMIBEnum, VString &outName )
{
	outName.Clear();

	for( VCSNameMap* mt = sCharSetNameMap; mt->fName ; mt++ )
	{
		if( mt->fMIBEnum == inMIBEnum )
		{
			outName.FromUniCString( mt->fName );
			break;
		}
	}
}


void VTextConverters::EnumWebCharsets( std::vector<VString>& outNames, std::vector<CharSet>& outCharSets )
{
	VString string;

	outNames.clear();
	outCharSets.clear();

	for( VCSNameMap* mt = sCharSetNameMap; mt->fName ; mt++ )
	{
		if( mt->fWebCharSet )
		{
			string.FromUniCString( mt->fName );

			outNames.push_back( string);
			outCharSets.push_back( mt->fCharSet);
		}
	}
}

void VTextConverters::EnumCharsets( std::vector<VString>& outNames, std::vector<CharSet>& outCharSets )
{
	VString name;

	std::vector<VString> names;
	std::vector<CharSet> charSets;

	// fill ordered by name, and remove duplicated charsets
	CharSet previousCharSet = VTC_UNKNOWN;
	for( VCSNameMap* mt = sCharSetNameMap ; mt->fName ; ++mt )
	{
		if (mt->fCharSet != previousCharSet)
		{
			previousCharSet = mt->fCharSet;
			name.FromUniCString( mt->fName );
			
			std::vector<VString>::iterator j = std::lower_bound( names.begin(), names.end(), name);
			std::vector<VString>::difference_type d = std::distance( names.begin(), j);
			std::vector<CharSet>::iterator k = charSets.begin() + d;

			names.insert( j, name);
			charSets.insert( k, mt->fCharSet);
		}
	}

	outNames.swap( names);
	outCharSets.swap( charSets);
}


VToUnicodeConverter* VTextConverters::RetainToUnicodeConverter( CharSet inCharSet)
{
	VToUnicodeConverter* converter = NULL;
	switch( inCharSet)
	{
	
		case VTC_SystemCharset:
			converter = fToUniConverter;
			converter->Retain();
			break;
		
		case VTC_UTF_16:
			converter = fToUniNullConverter;
			converter->Retain();
			break;
		
		#if BIGENDIAN
		case VTC_UTF_16_SMALLENDIAN:
		#else
		case VTC_UTF_16_BIGENDIAN:
		#endif
			converter = fToUniSwapConverter;
			converter->Retain();
			break;
		
		case VTC_UTF_32_RAW:
			converter = fToUni32RawConverter;
			converter->Retain();
			break;

		#if BIGENDIAN
		case VTC_UTF_32_RAW_SMALLENDIAN:
		#else
		case VTC_UTF_32_RAW_BIGENDIAN:
		#endif
			converter = new VToUnicodeConverter_UTF32_Raw(true);
			break;
		
		case VTC_UTF_32:
			converter = fToUni32Converter;
			converter->Retain();
			break;

		#if BIGENDIAN
		case VTC_UTF_32_SMALLENDIAN:
		#else
		case VTC_UTF_32_BIGENDIAN:
		#endif
			converter = new VToUnicodeConverter_UTF32 (true);
			break;

		case VTC_UTF_8:
			converter = fToUTF8Converter;
			converter->Retain();
			break;
		
		default:
			converter = XIntlMgrImpl::NewToUnicodeConverter(inCharSet);
			if (converter == NULL)
				converter = new VToUnicodeConverter_Void;
			break;
	}
	
	return converter;
}


VFromUnicodeConverter* VTextConverters::RetainFromUnicodeConverter(CharSet inCharSet)
{
	VFromUnicodeConverter* converter = NULL;
	switch(inCharSet) {
	
		case VTC_SystemCharset:
			converter = fFromUniConverter;
			converter->Retain();
			break;
		
		case VTC_UTF_16:
			converter = fFromUniNullConverter;
			converter->Retain();
			break;
		
		#if BIGENDIAN
		case VTC_UTF_16_SMALLENDIAN:
		#else
		case VTC_UTF_16_BIGENDIAN:
		#endif
			converter = fFromUniSwapConverter;
			converter->Retain();
			break;
		
		case VTC_UTF_32_RAW:
			converter = fFromUni32RawConverter;
			converter->Retain();
			break;
		
		#if BIGENDIAN
		case VTC_UTF_32_RAW_SMALLENDIAN:
		#else
		case VTC_UTF_32_RAW_BIGENDIAN:
		#endif
			converter = new VFromUnicodeConverter_UTF32_Raw(true);
			break;

		case VTC_UTF_32:
			converter = fFromUni32Converter;
			converter->Retain();
			break;
		
		#if BIGENDIAN
		case VTC_UTF_32_SMALLENDIAN:
		#else
		case VTC_UTF_32_BIGENDIAN:
		#endif
			converter = new VFromUnicodeConverter_UTF32 (true);
			converter->Retain();
			break;

		case VTC_UTF_8:
			converter = fFromUTF8Converter;
			converter->Retain();
			break;
		
		default:
			converter = XIntlMgrImpl::NewFromUnicodeConverter(inCharSet);
			if (converter == NULL)
				converter = new VFromUnicodeConverter_Void;
			break;
	}
	
	return converter;
}


VToUnicodeConverter* VTextConverters::GetStdLibToUnicodeConverter()
{
#if COMPIL_GCC
	assert_compile( VTC_StdLib_char == VTC_UTF_8);
	return fToUTF8Converter;
#elif COMPIL_VISUAL
	assert_compile( VTC_StdLib_char == VTC_SystemCharset);
	return fToUniConverter;
#endif
}


VFromUnicodeConverter* VTextConverters::GetStdLibFromUnicodeConverter()
{
#if COMPIL_GCC
	assert_compile( VTC_StdLib_char == VTC_UTF_8);
	return fFromUTF8Converter;
#elif COMPIL_VISUAL
	assert_compile( VTC_StdLib_char == VTC_SystemCharset);
	return fFromUniConverter;
#endif
}


bool VTextConverters::ConvertToVString(const void* inBytes, VSize inByteCount, CharSet inCharSet, VString& outString)
{
	outString.Truncate(0);
	bool isOK = false;
	VSize processedBytes;
	switch(inCharSet)
	{
	
		case VTC_SystemCharset:
			isOK = fToUniConverter->ConvertString(inBytes, inByteCount, &processedBytes, outString);
			break;
		
		case VTC_UTF_16:
			outString.FromBlock(inBytes, inByteCount, VTC_UTF_16);
			isOK = (inByteCount%sizeof(UniChar) == 0);
			break;
		
		#if BIGENDIAN
		case VTC_UTF_16_SMALLENDIAN:
		#else
		case VTC_UTF_16_BIGENDIAN:
		#endif
			isOK = fToUniSwapConverter->ConvertString(inBytes, inByteCount, &processedBytes, outString);
			break;
		
		case VTC_UTF_32_RAW:
			isOK = fToUni32RawConverter->ConvertString(inBytes, inByteCount, &processedBytes, outString);
			break;
		
		case VTC_UTF_32:
			isOK = fToUni32Converter->ConvertString (inBytes, inByteCount, &processedBytes, outString);
			break;

		case VTC_UTF_8:
			isOK = fToUTF8Converter->ConvertString(inBytes, inByteCount, &processedBytes, outString);
			break;
			
		default:
			{
				VToUnicodeConverter* converter = RetainToUnicodeConverter(inCharSet);
				if (converter != NULL)
				{
					isOK = converter->ConvertString(inBytes, inByteCount, &processedBytes, outString);
					converter->Release();
				}
				break;
			}
	}
	return isOK;
}


bool VTextConverters::ConvertFromVString(const VString& inString, VIndex* outCharsConsumed, void* inBuffer, VSize inBufferSize, VSize* outBytesProduced, CharSet inCharSet)
{
	bool isOK = false;

	switch(inCharSet)
	{

		case VTC_SystemCharset:
			isOK = fFromUniConverter->Convert(inString.GetCPointer(), inString.GetLength(), outCharsConsumed, inBuffer, inBufferSize, outBytesProduced);
			break;
		
		case VTC_UTF_16:
			isOK = fFromUniNullConverter->Convert(inString.GetCPointer(), inString.GetLength(), outCharsConsumed, inBuffer, inBufferSize, outBytesProduced);
			break;
		
		#if BIGENDIAN
		case VTC_UTF_16_SMALLENDIAN:
		#else
		case VTC_UTF_16_BIGENDIAN:
		#endif
			isOK = fFromUniSwapConverter->Convert(inString.GetCPointer(), inString.GetLength(), outCharsConsumed, inBuffer, inBufferSize, outBytesProduced);
			break;
		
		case VTC_UTF_32_RAW:
			isOK = fFromUni32RawConverter->Convert(inString.GetCPointer(), inString.GetLength(), outCharsConsumed, inBuffer, inBufferSize, outBytesProduced);
			break;
		
		case VTC_UTF_8:
			isOK = fFromUTF8Converter->Convert(inString.GetCPointer(), inString.GetLength(), outCharsConsumed, inBuffer, inBufferSize, outBytesProduced);
			break;

		default:
			{
				VFromUnicodeConverter* converter = RetainFromUnicodeConverter(inCharSet);
				isOK = converter->Convert(inString.GetCPointer(), inString.GetLength(), outCharsConsumed, inBuffer, inBufferSize, outBytesProduced);
				converter->Release();
				break;
			}

	}
	return isOK;
}

/*
	static
*/
bool VTextConverters::ParseBOM( const void *inBytes, size_t inByteCount, size_t *outBOMSize, CharSet *outFoundCharSet)
{
	const uBYTE *c = static_cast<const uBYTE*>( inBytes);
	size_t BOMSize;
	CharSet charSet;

	if ( (inByteCount >= 3) && c[0] == 0xef && c[1] == 0xbb && c[2] ==0xbf)	// utf-8 bom
	{
		BOMSize = 3;
		charSet = VTC_UTF_8;
	}
	else if ( (inByteCount >= 4) && (c[0] == 0 && c[1] == 0 && c[2] == 0xfe && c[3] == 0xff) )	// utf-32 bom
	{
		BOMSize = 4;
		charSet = VTC_UTF_32_BIGENDIAN;
	}
	else if ( (inByteCount >= 4) && (c[0] == 0xff && c[1] == 0xfe && c[2] == 0 && c[3] == 0) )	// utf-32 bom
	{
		BOMSize = 4;
		charSet = VTC_UTF_32_SMALLENDIAN;
	}
	else if ( (inByteCount >= 2) && c[0] == 0xfe && c[1] == 0xff)	// utf-16 bom
	{
		BOMSize = 2;
		charSet = VTC_UTF_16_BIGENDIAN;
	}
	else if ( (inByteCount >= 2) && c[0] == 0xff && c[1] == 0xfe)	// utf-16 bom
	{
		BOMSize = 2;
		charSet = VTC_UTF_16_SMALLENDIAN;
	}
	else
	{
		BOMSize = 0;
		charSet = VTC_UNKNOWN;
	}

	if (outBOMSize != NULL)
		*outBOMSize = BOMSize;

	if (outFoundCharSet != NULL)
		*outFoundCharSet = charSet;

	return BOMSize != 0;
}

/*
	static
*/
const char *VTextConverters::GetBOMForCharSet( CharSet inCharSet, size_t *outBOMSize)
{
	const char *bom = NULL;
	size_t bomSize = 0;
	switch( inCharSet)
	{
		case VTC_UTF_8:					bom = "\xef\xbb\xbf"; bomSize = 3; break;
		case VTC_UTF_16_BIGENDIAN:		bom = "\xfe\xff"; bomSize = 2; break;
		case VTC_UTF_16_SMALLENDIAN:	bom = "\xff\xfe"; bomSize = 2; break;
		case VTC_UTF_32_BIGENDIAN:		bom = "\x00\x00\xfe\xff"; bomSize = 4; break;
		case VTC_UTF_32_SMALLENDIAN:	bom = "\xff\xfe\x00\x00"; bomSize = 4; break;
		default:
			break;
	}
	*outBOMSize = bomSize;
	return bom;
}

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

#if VERSIONMAC
#include <dlfcn.h>
#endif

#include "VCollator.h"

#include "VIntlMgr.h"
#include "VMemoryCpp.h"
#include "VArrayValue.h"
#include "VProcess.h"
#include "VResource.h"
#include "VFolder.h"
#include "VFile.h"
#include "VFileStream.h"
#include "VUnicodeResources.h"
#include "VUnicodeRanges.h"
#include "VUnicodeTableFull.h"
#include "VProfiler.h"

#if USE_ICU
#include "unicode/uclean.h"
#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/locid.h"
#include "unicode/ustring.h"
#include "unicode/ucnv.h"
#include "unicode/unistr.h"
#include "unicode/coll.h"
#include "unicode/coleitr.h"
#include "unicode/usearch.h"
#include "ucol_imp.h"
#endif


inline bool IsIgnorableBasicLatin( UniChar inChar)
{
	return (inChar < 9) || ((inChar > 13) && (inChar < 32)) || (inChar == 127);
}


UniChar VCollator::sDefaultWildChar = CHAR_COMMERCIAL_AT;


/************************************************************************/
// collator base class
/************************************************************************/

void VCollator::SetWildChar( UniChar inWildChar)
{
	fWildChar = inWildChar;
}

void VCollator::Reset()
{
	SetWildChar( sDefaultWildChar);
}


bool VCollator::BeginsWithString( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics, sLONG *outFoundLength)
{
	if (inPatternSize == 0)
	{
		if (outFoundLength != NULL)
			*outFoundLength = 0;
		return true;
	}

	return FindString( inText, inTextSize, inPattern, inPatternSize, inWithDiacritics, outFoundLength) == 1;
}


bool VCollator::EndsWithString( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics, sLONG *outFoundLength)
{
	if (inPatternSize == 0)
	{
		if (outFoundLength != NULL)
			*outFoundLength = 0;
		return true;
	}

	sLONG lastPos = 0;
	sLONG lastFoundLength = 0;
	do
	{
		sLONG foundLength;
		sLONG pos = FindString( inText + lastPos, inTextSize - lastPos, inPattern, inPatternSize, inWithDiacritics, &foundLength);
		if (pos <= 0)
			break;

		lastPos += pos;
		lastFoundLength = foundLength;

	} while( true);

//	bool found = (lastPos + lastFoundLength - 1 == inTextSize);	doesn't work whene there are ignorable chars at the end of the text
	bool found = (lastPos > 0) && EqualString( inText + lastPos - 1, inTextSize - (lastPos - 1), inPattern, inPatternSize, inWithDiacritics);

	if (outFoundLength != NULL)
		*outFoundLength = found ? (inTextSize - (lastPos - 1)) : 0;	// don't use lastFoundLength because of ignorables at the end
		
	return found;
}


bool VCollator::IsPatternCompatibleWithDichotomyAndDiacritics( const UniChar *inPattern, const sLONG inSize)
{
	bool compatible = true;

	// search for the wildchar
	const UniChar *end = inPattern + inSize;
	const UniChar *p = inPattern;
	for( ; (p != end) && (*p != fWildChar) ; ++p)
		;
	
	// the only place the wildchar is authorized is at the very end
	if (p != end)
	{
		// we found a wild char.
		if (p + 1 != end)
		{
			// the wild char is not the last char -> incompatible
			compatible = false;
		}
		else
		{
			// we must check for incompatible codepoints.
			
			for( const UniChar *pc = inPattern ; (pc != end) && compatible ; ++pc)
			{
				// hiragana and halfwidth katana are not compatible
				if ( (*pc >= 0x3040 && *pc <= 0x3094) || (*pc == 0x309d) || (*pc == 0x309e) )
					compatible = false;
				
				// halfwidth katana are also not compatible
				else if ( (*pc >= 0xFF65 && *pc <= 0xFF9F) )
					compatible = false;
				
				// katana are also not compatible
				else if ( (*pc >= 0x30A1 && *pc <= 0x30FE) )
					compatible = false;
			}
		}
	}

	return compatible;
}


CompareResult VCollator::_CompareString_Like_IgnoreWildCharInMiddle( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics)
{
	CompareResult r;
	
	bool beginswith_wildchar = (inPatternSize > 0) && (inPattern[0] == fWildChar);
	bool endswith_wildchar = (inPatternSize > 0) && (inPattern[inPatternSize-1] == fWildChar);

	if (!beginswith_wildchar && !endswith_wildchar)
	{
		// no wildchar
		r = CompareString( inText, inTextSize, inPattern, inPatternSize, inWithDiacritics);
	}
	else if (beginswith_wildchar && endswith_wildchar)
	{
		// contains
		if (inPatternSize > 2)
		{
			sLONG pos = FindString( inText, inTextSize, inPattern + 1, inPatternSize - 2, inWithDiacritics, NULL);
			if (pos > 0)
				r = CR_EQUAL;
			else
				r = (inTextSize > inPatternSize) ? CR_BIGGER : CR_SMALLER;
		}
		else
		{
			r = CR_EQUAL;	// pattern is wild char alone
		}
	}
	else if (endswith_wildchar)
	{
		// begins with
		sLONG foundLength;
		if (BeginsWithString( inText, inTextSize, inPattern, inPatternSize - 1, inWithDiacritics, &foundLength))
		{
			r = CR_EQUAL;
		}
		else
		{
			r = CompareString( inText, inTextSize, inPattern, inPatternSize - 1, inWithDiacritics);
		}
	}
	else
	{
		// ends with
		if (EndsWithString( inText, inTextSize, inPattern + 1, inPatternSize - 1, inWithDiacritics, NULL))
		{
			r = CR_EQUAL;
		}
		else
		{
			r = (inTextSize > inPatternSize) ? CR_BIGGER : CR_SMALLER;
		}
	}
	return r;
}


CompareResult VCollator::_CompareString_Like_SupportWildCharInMiddle( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics)
{
	/*
		optimized in VICUCollator
	*/
	CompareResult r = CR_EQUAL;
	
	const UniChar *pattern = inPattern;
	const UniChar *pattern_end = inPattern + inPatternSize;

	const UniChar *text = inText;
	const UniChar *text_end = inText + inTextSize;
	
	do
	{
		if (pattern == pattern_end)
		{
			// no more pattern.
			// compare to empty string (beware of ignorable characters)
			r = CompareString( text, (sLONG) (text_end - text), NULL, 0, inWithDiacritics);
			break;
		}
		else if (*pattern == fWildChar)
		{
			// consume all extra wild chars
			while( (pattern != pattern_end) && (*pattern == fWildChar) )
				++pattern;
			
			if (pattern == pattern_end)
			{
				// nothing follows the wild chars -> we are done consuming the pattern
				r = CR_EQUAL;
				break;
			}
			else
			{
				// have to find a sequence.
				// search end of sequence.
				const UniChar *pattern_word = pattern;
				const UniChar *pattern_word_end = pattern;
				while( ++pattern_word_end != pattern_end)
				{
					if (*pattern_word_end == fWildChar)
						break;
				}
				
				if (pattern_word_end == pattern_end)
				{
					// if at the end of the pattern, we need to find the last possible match
					sLONG pos = 0;
					sLONG foundLength = 0;
					do
					{
						sLONG curFoundLength;
						sLONG curPos = FindString( text + pos, (sLONG) ((text_end - text) - pos), pattern_word, (sLONG) (pattern_word_end - pattern_word), inWithDiacritics, &curFoundLength);
						if (curPos <= 0)
							break;

						pos += curPos;
						foundLength = curFoundLength;

					} while( true);
					
					// even if found compare with all remaining text to take care of ignorable characters
					// warning: must be in sync with VICUCollator::_CompareString_LikeNoDiac_primary
					if (pos > 0)
					{
						r = CompareString( text + pos - 1, (sLONG) ((text_end - text) - (pos - 1)), pattern_word, (sLONG) (pattern_word_end - pattern_word), inWithDiacritics);
						if (r != CR_EQUAL)
							r = CR_SMALLER;
					}
					else
					{
						r = CR_SMALLER;	//CompareString( text, (text_end - text), pattern_word, pattern_word_end - pattern_word, inWithDiacritics);
					}
					break;
				}
				else
				{
					sLONG foundLength;
					sLONG pos = FindString( text, (sLONG) (text_end - text), pattern_word, (sLONG) (pattern_word_end - pattern_word), inWithDiacritics, &foundLength);
					if (pos > 0)
					{
						// found.
						// continue with following text and next pattern sequence.
						
						text += pos + foundLength - 1;
						pattern = pattern_word_end;
					}
					else
					{
						// not found.
						// break here with some compare result.
						// warning: must be in sync with VICUCollator::_CompareString_LikeNoDiac_primary
						r = CR_SMALLER;	//CompareString( text, text_end - text, pattern_word, pattern_word_end - pattern_word, inWithDiacritics);
						break;
					}
				}
			}
		}
		else
		{
			// must begins with pattern sequence.
			const UniChar *pattern_word = pattern;
			const UniChar *pattern_word_end = pattern;
			while( ++pattern_word_end != pattern_end)
			{
				if (*pattern_word_end == fWildChar)
					break;
			}
			sLONG foundLength;
			if ( (pattern_word_end != pattern_end) && BeginsWithString( text, (sLONG) (text_end - text), pattern_word, (sLONG) (pattern_word_end - pattern_word), inWithDiacritics, &foundLength))
			{
				// found.
				// continue with following text and next pattern sequence.
				text += foundLength;
				pattern = pattern_word_end;
			}
			else
			{
				// not found.
				// break here with compare result.
				r = CompareString( text, (sLONG) (text_end - text), pattern_word, (sLONG) (pattern_word_end - pattern_word), inWithDiacritics);
				break;
			}
		}
	} while( true);
	
	return r;
}


CompareResult VCollator::CompareString_Like( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics)
{
	CompareResult r;
	if (GetIgnoreWildCharInMiddle())
		r = _CompareString_Like_IgnoreWildCharInMiddle( inText, inTextSize, inPattern, inPatternSize, inWithDiacritics);
	else
		r = _CompareString_Like_SupportWildCharInMiddle( inText, inTextSize, inPattern, inPatternSize, inWithDiacritics);
	
	return r;
}


bool VCollator::EqualString_Like( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics)
{
	const UniChar *p1 = inText;
	const UniChar *p1_end = inText + inTextSize;

	const UniChar *p2 = inPattern;
	const UniChar *p2_end = inPattern + inPatternSize;

	bool equal;

	if (inWithDiacritics)
	{
		do {

			// skip ignorable chars
			while( (p1 != p1_end) && IsIgnorableBasicLatin( *p1) )
				++p1;

			while( (p2 != p2_end) && IsIgnorableBasicLatin( *p2) )
				++p2;
				
			if (p1 == p1_end)
			{
				// skip extra wildchars
				while( (p2 != p2_end) && (*p2 == fWildChar) )
					++p2;
				if (p2 == p2_end)
					equal = true;
				else if (*p2 < 127)
					equal = false;
				else
					equal = CompareString_Like( inText,  inTextSize,  inPattern, inPatternSize, inWithDiacritics) == CR_EQUAL;
				break;
			}
			else if (p2 == p2_end)
			{
				if (*p1 < 127)
					equal = false;
				else
					equal = CompareString_Like( inText,  inTextSize,  inPattern, inPatternSize, inWithDiacritics) == CR_EQUAL;
				break;
			}

			// everything above latin basic and not Grave Accent nor Circumflex Accent
			if ( (*p1 > 127 || *p2 > 127) || (*p1 == 0x60) || (*p2 == 0x60) || (*p1 == 0x5E) || (*p2 == 0x5E) || (*p2 == fWildChar))
			{
				// shortcut for wildchar at the end
				if ( (*p2 == fWildChar) && (p2 + 1 == p2_end) )
				{
					equal = true;
					break;
				}
				else if ( (*p2 != fWildChar) || !GetIgnoreWildCharInMiddle() || (p2 == inPattern) || (p2 + 1 == p2_end) )
				{
					equal = CompareString_Like( p1,  (sLONG) (p1_end - p1),  p2, (sLONG) (p2_end - p2), inWithDiacritics) == CR_EQUAL;
					break;
				}
			}

			if (*p1 != *p2)
			{
				equal = false;
				break;
			}
			
			++p1;
			++p2;
		} while( true);
	}
	else
	{
		do {

			// skip ignorable chars
			while( (p1 != p1_end) && IsIgnorableBasicLatin( *p1) )
				++p1;

			while( (p2 != p2_end) && IsIgnorableBasicLatin( *p2) )
				++p2;
				
			if (p1 == p1_end)
			{
				// skip extra wildchars
				while( (p2 != p2_end) && (*p2 == fWildChar) )
					++p2;
				equal = (p2 == p2_end);
				if (p2 == p2_end)
					equal = true;
				else if (*p2 < 127)
					equal = false;
				else
					equal = CompareString_Like( inText,  inTextSize,  inPattern, inPatternSize, inWithDiacritics) == CR_EQUAL;
				break;
			}
			else if (p2 == p2_end)
			{
				if (*p1 < 127)
					equal = false;
				else
					equal = CompareString_Like( inText,  inTextSize,  inPattern, inPatternSize, inWithDiacritics) == CR_EQUAL;
				break;
			}

			// everything above latin basic and not Grave Accent nor Circumflex Accent
			if ( (*p1 > 127 || *p2 > 127) || (*p1 == 0x60) || (*p2 == 0x60) || (*p1 == 0x5E) || (*p2 == 0x5E) || (*p2 == fWildChar))
			{
				// shortcut for wildchar at the end
				if ( (*p2 == fWildChar) && (p2 + 1 == p2_end) )
				{
					equal = true;
					break;
				}
				else if ( (*p2 != fWildChar) || !GetIgnoreWildCharInMiddle() || (p2 == inPattern) || (p2 + 1 == p2_end) )
				{
					equal = CompareString_Like( p1,  (sLONG) (p1_end - p1),  p2, (sLONG) (p2_end - p2), inWithDiacritics) == CR_EQUAL;
					break;
				}
			}

			UniChar c1 = ( (*p1 >= 'a') && (*p1 <= 'z') ) ? (*p1 + 'A' - 'a') : *p1;
			UniChar c2 = ( (*p2 >= 'a') && (*p2 <= 'z') ) ? (*p2 + 'A' - 'a') : *p2;

			if (c1 != c2)
			{
				equal = false;
				break;
			}
			
			++p1;
			++p2;
		} while( true);
	}
	
	#if VERSIONDEBUG
	bool compare_equal = CompareString_Like( inText,  inTextSize,  inPattern, inPatternSize, inWithDiacritics) == CR_EQUAL;
	xbox_assert( equal == compare_equal);
	#endif

	return equal;
}

#if !VERSION_LINUX

/************************************************************/
// system implementation
/************************************************************/

VCollator_system::VCollator_system( DialectCode inDialect, VIntlMgr *inIntlMgr)
 : inherited( inDialect, true)
 , fImpl( inDialect, inIntlMgr)
{
}


VCollator_system::VCollator_system( const VCollator_system& inCollator)
 : inherited( inCollator)
 , fImpl( inCollator.fImpl)
{
}


VCollator_system* VCollator_system::Create( DialectCode inDialect, VIntlMgr *inIntlMgr)
{
	return new VCollator_system( inDialect, inIntlMgr);
}


CompareResult VCollator_system::CompareString (const UniChar* inText1, sLONG inSize1, const UniChar* inText2, sLONG inSize2, bool inWithDiacritics)
{
	if (inSize1 == 0)
		return inSize2 == 0 ? CR_EQUAL : CR_SMALLER;

	if (inSize2 == 0)
		return CR_BIGGER;
		
	return fImpl.CompareString (inText1, inSize1, inText2, inSize2, inWithDiacritics);
}


bool VCollator_system::EqualString(const UniChar* inText1, sLONG inSize1, const UniChar* inText2, sLONG inSize2, bool inWithDiacritics)
{
	return fImpl.EqualString( inText1, inSize1, inText2, inSize2, inWithDiacritics);
}


sLONG VCollator_system::FindString( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics, sLONG *outFoundLength)
{
	// finding null strings is system dependant. check that case here.
	if ( (inTextSize == 0) || (inPatternSize == 0) )
	{
		if (outFoundLength != NULL)
			*outFoundLength = 0;
		return ( (inPatternSize == 0) && (inTextSize > 0) ) ? 1 : 0;
	}
	
	return fImpl.FindString( inText, inTextSize, inPattern, inPatternSize, inWithDiacritics, outFoundLength);
}


sLONG VCollator_system::ReversedFindString( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics, sLONG *outFoundLength)
{
	// finding null strings is system dependant. check that case here.
	if ( (inTextSize == 0) || (inPatternSize == 0) )
	{
		if (outFoundLength != NULL)
			*outFoundLength = 0;
		return ( (inPatternSize == 0) && (inTextSize > 0) ) ? 1 : 0;
	}

	const UniChar* text = inText;
	sLONG textSize = inTextSize;
	sLONG lastPos = 0;
	sLONG lastFoundLength = 0;
	sLONG pos;
	do
	{
		sLONG foundLength;
		pos = fImpl.FindString( text, textSize, inPattern, inPatternSize, inWithDiacritics, &foundLength);
		if (pos <= 0)
			break;
		
		lastPos = pos;
		lastFoundLength = foundLength;
		
		text += pos;
		textSize -= pos;

	} while(textSize > 0);
	
	if (outFoundLength != NULL)
		*outFoundLength = lastFoundLength;
	
	return lastPos;
}


void VCollator_system::GetStringComparisonAlgorithmSignature( VString& outSignature) const
{
	VString osString;
	VSystem::GetOSVersionString( osString);

	outSignature = CVSTR( "system:");
	outSignature += osString;
}


#endif

/************************************************************/
// icu
/************************************************************/
#if USE_ICU


/*  CEBuf - A struct and some inline functions to handle the saving    */
/*          of CEs in a buffer within ucol_strcoll                     */
/*  stolen from icu/ucol.cpp                                           */

#define UCOL_CEBUF_SIZE 512
typedef struct ucol_CEBuf
{
    uLONG    *buf;
    uLONG    *endp;
    uLONG    *pos;
    uLONG     localArray[UCOL_CEBUF_SIZE];
} ucol_CEBuf;


static inline void UCOL_INIT_CEBUF(ucol_CEBuf& b)
{
	b.buf = b.pos = b.localArray;
	b.endp = b.buf + UCOL_CEBUF_SIZE;
}


static inline void UCOL_DEINIT_CEBUF(ucol_CEBuf& b)
{
	if ( b.buf != b.localArray)
		free( b.buf);
}


static inline void UCOL_RESET_CEBUF(ucol_CEBuf& b)
{
	b.pos = b.buf;
}


static void ucol_CEBuf_Expand(ucol_CEBuf *b)
{
	uLONG  oldSize = (uLONG) (b->pos - b->buf);
	uLONG  newSize = oldSize * 2;
	uLONG  *newBuf = (uLONG *) malloc(newSize * sizeof(uLONG));
	if (newBuf != NULL)
	{
		memcpy(newBuf, b->buf, oldSize * sizeof(uLONG));
		if (b->buf != b->localArray)
		{
			free(b->buf);
		}
		b->buf = newBuf;
		b->endp = b->buf + newSize;
		b->pos = b->buf + oldSize;
	}
}


static inline void UCOL_CEBUF_PUT(ucol_CEBuf& b, uint32_t ce)
{
	// don't collect anything until we get the first primary element
	if ( (b.pos == b.buf) && ((ce & UCOL_PRIMARYMASK) == 0) )
	{
		return;
	}
	if (b.pos == b.endp)
	{
		ucol_CEBuf_Expand(&b);
	}
	*(b.pos++) = ce;
}


static bool UCOL_CLOSE_CEBUF( ucol_CEBuf& inCEBuf, UCollationElements *inIter)
{
	// put all remaining non primary collation elements and terminate the buffer with the UCOL_NULLORDER sentinel. 
	sLONG key;
	do
	{
		UErrorCode status = U_ZERO_ERROR;
		sLONG pos = ucol_getOffset( inIter);
		key = ucol_next( inIter, &status);
		if (key & UCOL_PRIMARYMASK)
		{
			// push back the primary key we shouldn't have read
			// collation iterators are mono-directionnal: one can't mix ucol_next and ucol_previous
			if (key != UCOL_NULLORDER)
				ucol_setOffset( inIter, pos, &status);
			break;
		}
		UCOL_CEBUF_PUT( inCEBuf, key);
	} while( true);
	
	UCOL_CEBUF_PUT( inCEBuf, UCOL_NULLORDER);
	
	return (key == UCOL_NULLORDER);
}


    /* This is the secondary level of comparison */
static CompareResult ucol_CEBuf_CheckSecondary( ucol_CEBuf& inBufText, ucol_CEBuf& inBufPattern, uLONG inWildCharKey)
{
	CompareResult result = CR_EQUAL;

	uLONG secS = 0, secT = 0;
	uLONG *sCE = inBufText.buf;
	uLONG *tCE = inBufPattern.buf;
	for(;;)
	{
		while (secS == 0)
			secS = *(sCE++) & UCOL_SECONDARYMASK;

		while(secT == 0)
		{
			if ( (*tCE & UCOL_PRIMARYMASK) == inWildCharKey)
				secT = UCOL_SECONDARYMASK;
			else
				secT = *tCE & UCOL_SECONDARYMASK;
			tCE++;
		}

		if (secS == secT)
		{
			if (secS == UCOL_SECONDARYMASK)
			{
				break;
			}
			else
			{
			  secS = 0;
			  secT = 0;
			  continue;
			}
		}
		else
		{
			/*
				ucol_next maps UCOL_NO_MORE_CES to UCOL_NULLORDER.
				but we need UCOL_NO_MORE_CES to properly test only secondary elements.
				(see icu::ucol_strcollRegular)
			*/
			if (secS == UCOL_SECONDARYMASK)
				secS = UCOL_NO_MORE_CES_SECONDARY;
			if (secT == UCOL_SECONDARYMASK)
				secT = UCOL_NO_MORE_CES_SECONDARY;
			result = (secS < secT) ? CR_SMALLER : CR_BIGGER;
			break;
		}
	}

	return result;
}


inline bool _NextPrimaryKey( UCollationElements *inIter, uLONG& outPrimaryKey, UErrorCode *status)
{
	sLONG key;
	do
	{
		key = ucol_next( inIter, status);
	} while( (key & UCOL_PRIMARYMASK) == 0 );	// UCOL_NULLORDER is FFFFFFFF so no need to test for it. And don't use inMask but always UCOL_PRIMARYMASK.
	
	outPrimaryKey = (uLONG) (key & UCOL_PRIMARYMASK);
	
	return (key == UCOL_NULLORDER);
}

inline bool _NextPrimaryKey( UCollationElements *inIter, int32_t inMask, uLONG& outPrimaryKey, UErrorCode *status)
{
	sLONG key;
	do
	{
		key = ucol_next( inIter, status);
	} while( (key & UCOL_PRIMARYMASK) == 0 );	// UCOL_NULLORDER is FFFFFFFF so no need to test for it. And don't use inMask but always UCOL_PRIMARYMASK.
	
	outPrimaryKey = (uLONG) (key & inMask);
	
	return (key == UCOL_NULLORDER);
}


inline bool _NextPrimaryKey( UCollationElements *inIter, uLONG& outPrimaryKey, ucol_CEBuf& inCEBuf, UErrorCode *status)
{
	sLONG key;
	do
	{
		key = ucol_next( inIter, status);
		UCOL_CEBUF_PUT( inCEBuf, key);
	} while( (key & UCOL_PRIMARYMASK) == 0 );	// UCOL_NULLORDER is FFFFFFFF so no need to test for it. And don't use inMask but always UCOL_PRIMARYMASK.
	
	outPrimaryKey = (uLONG) (key & UCOL_PRIMARYMASK);
	
	return (key == UCOL_NULLORDER);
}


#if VERSIONMAC
extern "C"
{
bool ICU4D()  __attribute__((weak_import));
void u_init(UErrorCode *status) __attribute__((weak_import));
}
#endif


static void _compile_time_assertions()
{
	assert_compile( (CompareResult) UCOL_EQUAL == CR_EQUAL);
	assert_compile( (CompareResult) UCOL_GREATER == CR_BIGGER);
	assert_compile( (CompareResult) UCOL_LESS == CR_SMALLER);
}


VICUCollator::VICUCollator( DialectCode inDialect, xbox_icu::Locale *inLocale, xbox_icu::Collator *inPrimary, xbox_icu::Collator *inTertiary, UCollationElements *inTextElements, UCollationElements *inPatternElements, CollatorOptions inOptions)
: inherited( inDialect, inOptions)
, fLocale( inLocale)
, fPrimaryCollator( inPrimary)
, fTertiaryCollator( inTertiary)
, fTextElements( inTextElements)
, fPatternElements( inPatternElements)
, fSearchWithDiacritics( NULL)
, fSearchWithoutDiacritics( NULL)
, fUseOptimizedSort(false)
, fElementKeyMask( (inOptions & COL_EqualityUsesSecondaryStrength) ? (UCOL_PRIMARYMASK | UCOL_SECONDARYMASK) : UCOL_PRIMARYMASK)
{
	uWORD primaryLanguage=DC_PRIMARYLANGID(inDialect);
	
	// pp on active l'optimisation uniquement pour les langues sans contraction
	
	fUseOptimizedSort = primaryLanguage==DC_PRIMARYLANGID(DC_FRENCH)	|| 
						primaryLanguage==DC_PRIMARYLANGID(DC_ITALIAN)	|| 
						primaryLanguage==DC_PRIMARYLANGID(DC_ENGLISH_US)||	
						primaryLanguage==DC_PRIMARYLANGID(DC_GERMAN);
		
	_UpdateWildCharPrimaryKey();
}


VICUCollator::~VICUCollator()
{
	delete fPrimaryCollator;
	delete fTertiaryCollator;
	
	delete fLocale;
	
	if (fSearchWithDiacritics != NULL)
	{
		ubrk_close( const_cast<UBreakIterator*>( usearch_getBreakIterator( fSearchWithDiacritics)));
		usearch_close( fSearchWithDiacritics);
	}

	if (fSearchWithoutDiacritics != NULL)
	{
		ubrk_close( const_cast<UBreakIterator*>( usearch_getBreakIterator( fSearchWithoutDiacritics)));
		usearch_close( fSearchWithoutDiacritics);
	}
	
	if (fTextElements != NULL)
		ucol_closeElements( fTextElements);

	if (fPatternElements != NULL)
		ucol_closeElements( fPatternElements);
}


VICUCollator::VICUCollator(const VICUCollator& inCollator)
: inherited( inCollator)
, fSearchWithDiacritics( NULL)
, fSearchWithoutDiacritics( NULL)
, fElementKeyMask( inCollator.fElementKeyMask)
{
	fUseOptimizedSort = inCollator.fUseOptimizedSort;
	fLocale = inCollator.fLocale->clone();
		
	fPrimaryCollator = inCollator.fPrimaryCollator->safeClone();
	fTertiaryCollator = inCollator.fTertiaryCollator->safeClone();
	
	UErrorCode status = U_ZERO_ERROR;
	const UCollator *collator = ((RuleBasedCollator*) fPrimaryCollator)->getUCollator();
	fTextElements = ucol_openElements( collator, NULL, 0, &status);
	fPatternElements = ucol_openElements( collator, NULL, 0, &status);
	
	_UpdateWildCharPrimaryKey();
}


void VICUCollator::SetWildChar( UniChar inWildChar)
{
	inherited::SetWildChar( inWildChar);
	_UpdateWildCharPrimaryKey();
}


const char order_chars[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 28, 29, 30, 31, 32, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 
							19, 20, 21, 22, 23, 24, 25, 26, 33, 41, 45, 57, 65, 58, 56, 44, 46, 47, 53, 59, 38, 37, 
							43, 54, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 40, 39, 60, 61, 62, 42, 52, 77, 79, 81, 
							83, 85, 87, 89, 91, 93, 95, 97, 99, 101, 103, 105, 107, 109, 111, 113, 115, 117, 119, 121, 
							123, 125, 127, 48, 55, 49, 35, 36, 34, 76, 78, 80, 82, 84, 86, 88, 90, 92, 94, 96, 98, 100, 
							102, 104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124, 126, 50, 63, 51, 64, 27, 0 };


CompareResult VICUCollator::CompareString (const UniChar* inText1, sLONG inSize1, const UniChar* inText2, sLONG inSize2, bool inWithDiacritics)
{
	static const UniChar nullStr[] = {0};
	UErrorCode status = U_ZERO_ERROR;
	
	// icu doesn't accept null pointer even if the associated size parameter is zero
	if (inText1 == NULL)
		inText1 = nullStr;

	if (inText2 == NULL)
		inText2 = nullStr;
	
	if (fUseOptimizedSort)
	{
		const UniChar *p1 = inText1;
		const UniChar *p1_end = inText1 + inSize1;

		const UniChar *p2 = inText2;
		const UniChar *p2_end = inText2 + inSize2;

		CompareResult result = CR_UNRELEVANT;

		bool firsttime = true;
		UniChar firstdiff1, firstdiff2;

		if (inWithDiacritics)
		{
			do {
				// skip ignorable chars
				while( (p1 != p1_end) && IsIgnorableBasicLatin( *p1) )
					++p1;

				while( (p2 != p2_end) && IsIgnorableBasicLatin( *p2) )
					++p2;

				if (p1 == p1_end)
				{
					if (p2 == p2_end)
					{
						if (firsttime)
						{
							result = CR_EQUAL;
						}
						else
						{
							if (order_chars[firstdiff1] > order_chars[firstdiff2])
								result = CR_BIGGER;
							else
								result = CR_SMALLER;
						}
					}
					else if (*p2 < 127)
					{
						result = CR_SMALLER;
					}
					else
					{
						result = (CompareResult) fTertiaryCollator->compare( inText1,  inSize1,  inText2, inSize2, status);
					}
					break;
				}
				else if (p2 == p2_end)
				{
					if (*p1 < 127)
						result = CR_BIGGER;
					else
						result = (CompareResult) fTertiaryCollator->compare( inText1,  inSize1,  inText2, inSize2, status);
					break;
				}
				else if ( (*p1 > 127 || *p2 > 127) || (*p1 == 0x60) || (*p2 == 0x60) || (*p1 == 0x5E) || (*p2 == 0x5E) )
				{
					// everything above latin basic and not Grave Accent nor Circumflex Accent
					result = (CompareResult) fTertiaryCollator->compare( inText1,  inSize1,  inText2, inSize2, status);
					break;
				}
				else
				{
					if (*p1 != *p2)
					{
						if (firsttime)
						{
							firstdiff1 = *p1;
							firstdiff2 = *p2;
							firsttime = false;
						}

						UniChar c1 = ( (*p1 >= 'a') && (*p1 <= 'z') ) ? (*p1 + 'A' - 'a') : *p1;
						UniChar c2 = ( (*p2 >= 'a') && (*p2 <= 'z') ) ? (*p2 + 'A' - 'a') : *p2;

						if (c1 != c2)
						{
							if (order_chars[*p1] > order_chars[*p2])
								result = CR_BIGGER;
							else
								result = CR_SMALLER;
							break;
						}
					}

					++p1;
					++p2;
				}
			} while( true);

	#if VERSIONDEBUG
			CompareResult icu_result = (CompareResult) fTertiaryCollator->compare( inText1,  inSize1,  inText2, inSize2, status);
			if (result != icu_result)
			{
				(void)icu_result; // break here
				xbox_assert(result == icu_result);
			}
	#endif
		}
		else
		{
			do {
				// skip ignorable chars
				while( (p1 != p1_end) && IsIgnorableBasicLatin( *p1) )
					++p1;

				while( (p2 != p2_end) && IsIgnorableBasicLatin( *p2) )
					++p2;

				if (p1 == p1_end)
				{
					if (p2 == p2_end)
						result = CR_EQUAL;
					else if (*p2 < 127)
						result = CR_SMALLER;
					else
						result = (CompareResult) fPrimaryCollator->compare( inText1,  inSize1,  inText2, inSize2, status);
					break;
				}
				else if (p2 == p2_end)
				{
					if (*p1 < 127)
						result = CR_BIGGER;
					else
						result = (CompareResult) fPrimaryCollator->compare( inText1,  inSize1,  inText2, inSize2, status);
					break;
				}
				else if ( (*p1 > 127 || *p2 > 127) || (*p1 == 0x60) || (*p2 == 0x60) || (*p1 == 0x5E) || (*p2 == 0x5E) )
				{
					// everything above latin basic and not Grave Accent nor Circumflex Accent
					result = (CompareResult) fPrimaryCollator->compare( inText1,  inSize1,  inText2, inSize2, status);
					break;
				}
				else
				{
					UniChar c1 = ( (*p1 >= 'a') && (*p1 <= 'z') ) ? (*p1 + 'A' - 'a') : *p1;
					UniChar c2 = ( (*p2 >= 'a') && (*p2 <= 'z') ) ? (*p2 + 'A' - 'a') : *p2;
					if (c1 != c2)
					{
						if (order_chars[c1] > order_chars[c2])
							result = CR_BIGGER;
						else
							result = CR_SMALLER;
						break;
					}

					++p1;
					++p2;
				}
			} while( true);

	#if VERSIONDEBUG
			CompareResult icu_result = (CompareResult)fPrimaryCollator->compare( inText1,  inSize1,  inText2, inSize2, status);
			if (result != icu_result)
			{
				(void)icu_result; // break here
				assert(result == icu_result);
			}
	#endif
		}

		return result;
	}
	else
	{

		UCollationResult result;
		if (inWithDiacritics)
			result = fTertiaryCollator->compare( inText1, inSize1, inText2, inSize2, status);
		else
			result = fPrimaryCollator->compare( inText1, inSize1, inText2, inSize2, status);
		
		xbox_assert( U_SUCCESS( status));

		return (CompareResult) result;
	}
	
}


bool VICUCollator::EqualString(const UniChar* inText1, sLONG inSize1, const UniChar* inText2, sLONG inSize2, bool inWithDiacritics)
{
	UErrorCode status = U_ZERO_ERROR;
	const UniChar *p1 = inText1;
	const UniChar *p1_end = inText1 + inSize1;

	const UniChar *p2 = inText2;
	const UniChar *p2_end = inText2 + inSize2;

	bool equal;

	if (inWithDiacritics)
	{
		do {

			// skip ignorable chars
			while( (p1 != p1_end) && IsIgnorableBasicLatin( *p1) )
				++p1;

			while( (p2 != p2_end) && IsIgnorableBasicLatin( *p2) )
				++p2;
				
			if (p1 == p1_end)
			{
				if (p2 == p2_end)
					equal = true;
				else if (*p2 < 127)
					equal = false;
				else
					equal = fTertiaryCollator->compare( inText1,  inSize1,  inText2, inSize2, status) == UCOL_EQUAL;
				break;
			}
			else if (p2 == p2_end)
			{
				if (*p1 < 127)
					equal = false;
				else
					equal = fTertiaryCollator->compare( inText1,  inSize1,  inText2, inSize2, status) == UCOL_EQUAL;
				break;
			}

			// everything above latin basic and not Grave Accent nor Circumflex Accent
			if ( (*p1 > 127 || *p2 > 127) || (*p1 == 0x60) || (*p2 == 0x60) || (*p1 == 0x5E) || (*p2 == 0x5E) )
			{
				equal = fTertiaryCollator->compare( inText1,  inSize1,  inText2, inSize2, status) == UCOL_EQUAL;
				break;
			}
			else
			{
				if (*p1 != *p2)
				{
					equal = false;
					break;
				}
				
				++p1;
				++p2;
			}
		} while( true);
	}
	else
	{
		do {

			// skip ignorable chars
			while( (p1 != p1_end) && IsIgnorableBasicLatin( *p1) )
				++p1;

			while( (p2 != p2_end) && IsIgnorableBasicLatin( *p2) )
				++p2;
				
			if (p1 == p1_end)
			{
				if (p2 == p2_end)
					equal = true;
				else if (*p2 < 127)
					equal = false;
				else
					equal = fPrimaryCollator->compare( inText1,  inSize1,  inText2, inSize2, status) == UCOL_EQUAL;
				break;
			}
			else if (p2 == p2_end)
			{
				if (*p1 < 127)
					equal = false;
				else
					equal = fPrimaryCollator->compare( inText1,  inSize1,  inText2, inSize2, status) == UCOL_EQUAL;
				break;
			}

			// everything above latin basic and not Grave Accent nor Circumflex Accent
			if ( (*p1 > 127 || *p2 > 127) || (*p1 == 0x60) || (*p2 == 0x60) || (*p1 == 0x5E) || (*p2 == 0x5E) )
			{
				equal = fPrimaryCollator->compare( inText1,  inSize1,  inText2, inSize2, status) == UCOL_EQUAL;
				break;
			}
			else
			{
				UniChar c1 = ( (*p1 >= 'a') && (*p1 <= 'z') ) ? (*p1 + 'A' - 'a') : *p1;
				UniChar c2 = ( (*p2 >= 'a') && (*p2 <= 'z') ) ? (*p2 + 'A' - 'a') : *p2;

				if (c1 != c2)
				{
					equal = false;
					break;
				}
				
				++p1;
				++p2;
			}
		} while( true);
	}
	
	#if VERSIONDEBUG
	bool equal_icu = CompareString( inText1,  inSize1,  inText2, inSize2, inWithDiacritics) == CR_EQUAL;
	xbox_assert( equal == equal_icu);
	#endif

	return equal;
}


VICUCollator* VICUCollator::Create( DialectCode inDialect, const xbox_icu::Locale *inLocale, CollatorOptions inOptions)
{
	VICUCollator* col = NULL;
	
	if (VIntlMgr::ICU_Available())
	{
		xbox_icu::Locale *locale;
		const char *keywords = (inOptions & COL_TraditionalStyleOrdering) ? "collation=traditional" : NULL;
		if (inLocale == NULL)
		{
			locale = new xbox_icu::Locale( VIntlMgr::GetISO6391LanguageCode( inDialect), VIntlMgr::GetISO3166RegionCode( inDialect), NULL, keywords);
		}
		else
		{
			if (keywords == NULL)
			{
				locale = inLocale->clone();
			}
			else
			{
				locale = new xbox_icu::Locale( inLocale->getLanguage(), inLocale->getCountry(), NULL, keywords);
			}
		}
		if (locale != NULL)
		{
			UErrorCode status = U_ZERO_ERROR;

			#if 0
			fprintf(stdout,"ICU Locale :\r\nLanguage=%s\r\nCountry=%s\r\nScript=%s\r\nVariant=%s\r\n",locale->getLanguage(),locale->getCountry(),locale->getScript(),locale->getVariant());
			#endif
			xbox_icu::Collator *primaryCollator = xbox_icu::Collator::createInstance( *locale, status);
			
			bool ok = (primaryCollator != NULL) && U_SUCCESS( status);
			
			if (ok)
			{
				// UCOL_FRENCH_COLLATION is incompatible with our way to test for secondary strength in ucol_CEBuf_CheckSecondary
				if ( (inOptions & COL_EqualityUsesSecondaryStrength) == 0)
					primaryCollator->setAttribute( UCOL_FRENCH_COLLATION, UCOL_ON, status);
				
				xbox_icu::Collator *tertiaryCollator = primaryCollator->safeClone();
				if (tertiaryCollator != NULL)
				{
					tertiaryCollator->setAttribute( UCOL_STRENGTH, (inOptions & COL_ComparisonUsesQuaternaryStrength) ? UCOL_QUATERNARY : UCOL_TERTIARY, status);
					if (inOptions & COL_ComparisonUsesHiraganaQuaternary)
						tertiaryCollator->setAttribute( UCOL_HIRAGANA_QUATERNARY_MODE, UCOL_ON, status);
				}
				
				primaryCollator->setAttribute( UCOL_STRENGTH, (inOptions & COL_EqualityUsesSecondaryStrength) ? UCOL_SECONDARY : UCOL_PRIMARY, status);
				
				const UCollator *collator = ((RuleBasedCollator*) primaryCollator)->getUCollator();
				UCollationElements *textElements = ucol_openElements( collator, NULL, 0, &status);
				UCollationElements *patternElements = ucol_openElements( collator, NULL, 0, &status);

				if (testAssert( U_SUCCESS( status)))
				{
					col = new VICUCollator( inDialect, locale, primaryCollator, tertiaryCollator, textElements, patternElements, inOptions);				
				}
				else
				{
					delete locale;
					delete primaryCollator;
					delete tertiaryCollator;
		
					if (textElements != NULL)
						ucol_closeElements( textElements);

					if (patternElements != NULL)
						ucol_closeElements( patternElements);
				}
			}
			else
			{
				delete locale;
				delete primaryCollator;
			}
		}
	}
	return col;
}


void VICUCollator::_UpdateWildCharPrimaryKey()
{
	UErrorCode status = U_ZERO_ERROR;
	ucol_setText( fTextElements, &fWildChar, 1, &status);
	fWildCharKey = ucol_next( fTextElements, &status) & UCOL_PRIMARYMASK;
}


CompareResult VICUCollator::_CompareString_Like_SupportWildCharInMiddle( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics)
{
	CompareResult r;
	if (inWithDiacritics)
	{
		r = inherited::_CompareString_Like_SupportWildCharInMiddle( inText,  inTextSize, inPattern, inPatternSize, inWithDiacritics);
	}
	else
	{
		if (fOptions & COL_EqualityUsesSecondaryStrength)
			r = _CompareString_LikeNoDiac_secondary( inText, inTextSize, inPattern, inPatternSize);
		else
			r = _CompareString_LikeNoDiac_primary( inText, inTextSize, inPattern, inPatternSize);

		#if VERSIONDEBUG
		CompareResult debug_result = inherited::_CompareString_Like_SupportWildCharInMiddle( inText,  inTextSize, inPattern, inPatternSize, inWithDiacritics);
		xbox_assert( debug_result == r);
		#endif
	}
	return r;
}


CompareResult VICUCollator::_CompareString_LikeNoDiac_primary( const UniChar* inText, sLONG inSize, const UniChar* inPattern, sLONG inPatternSize)
{
	UErrorCode err = U_ZERO_ERROR;

	ucol_setText( fTextElements, inText, inSize, &err);
	ucol_setText( fPatternElements, inPattern, inPatternSize, &err);
	
	uLONG text_key;
	uLONG pattern_key;
		
	sLONG oldtext_pos = -1;
	sLONG oldpattern_pos = -1;
		
	CompareResult r = CR_EQUAL;
	bool lookingForWildChar = true;

	bool text_end = _NextPrimaryKey( fTextElements, fElementKeyMask, text_key, &err);
	bool pattern_end = _NextPrimaryKey( fPatternElements, fElementKeyMask, pattern_key, &err);
		
	while( !text_end )
	{
		if (lookingForWildChar)
		{
			if (!pattern_end)
			{
				//ch2 = inWithDiacritics ? *p2 : fTableDiacStrip[*p2];
				
				if ( (pattern_key & UCOL_PRIMARYMASK) == fWildCharKey)
				{
					oldpattern_pos = ucol_getOffset( fPatternElements);
					pattern_end = _NextPrimaryKey( fPatternElements, fElementKeyMask, pattern_key, &err);
					if (!pattern_end)
					{
						oldtext_pos = ucol_getOffset( fTextElements);
					
						lookingForWildChar = false;
					}
					else
					{
						// arobase a la fin on sort
						text_end = true;
					}
				}
				else if (text_key == pattern_key)
				{
					pattern_end = _NextPrimaryKey( fPatternElements, fElementKeyMask, pattern_key, &err);
					text_end = _NextPrimaryKey( fTextElements, fElementKeyMask, text_key, &err);
				}
				else if (oldpattern_pos != -1)
				{
					ucol_setOffset( fPatternElements, oldpattern_pos, &err);
					ucol_setOffset( fTextElements, oldtext_pos, &err);

					pattern_end = _NextPrimaryKey( fPatternElements, fElementKeyMask, pattern_key, &err);
					text_end = _NextPrimaryKey( fTextElements, fElementKeyMask, text_key, &err);

					oldtext_pos = ucol_getOffset( fTextElements);
				
					lookingForWildChar = false;
				}
				else
				{
					r = (text_key > pattern_key) ? CR_BIGGER : CR_SMALLER;
					break;
				}
			}
			else
			{
				if (oldpattern_pos != -1)
				{
					ucol_setOffset( fPatternElements, oldpattern_pos, &err);
					pattern_end = _NextPrimaryKey( fPatternElements, fElementKeyMask, pattern_key, &err);
	
					lookingForWildChar = false;
					
					ucol_setOffset( fTextElements, oldtext_pos, &err);
					text_end = _NextPrimaryKey( fTextElements, fElementKeyMask, text_key, &err);
					oldtext_pos = ucol_getOffset( fTextElements);
				}
				else
				{
					r = CR_BIGGER;
					break;
				}
			}
		}
		else	// if (lookingForWildChar)
		{
			/*
				we are looking for the key just after the wild char
			*/
			if (pattern_key == text_key)
			{
				lookingForWildChar = true;
				pattern_end = _NextPrimaryKey( fPatternElements, fElementKeyMask, pattern_key, &err);
			}
			else
			{
				oldtext_pos = ucol_getOffset( fTextElements);
			}
			text_end = _NextPrimaryKey( fTextElements, fElementKeyMask, text_key, &err);
		}
	}

	if (r == CR_EQUAL)
	{
		sLONG pattern_pos = ucol_getOffset( fPatternElements);
		if (pattern_pos <= inPatternSize && !pattern_end)
		{
			// skip extra wild chars
			while( (pattern_pos <= inPatternSize) && (inPattern[pattern_pos-1] == fWildChar) )
				++pattern_pos;
			if (pattern_pos > inPatternSize)
			{
				pattern_end = true;
			}
		}
		if ((pattern_pos <= inPatternSize) && !pattern_end)
		{
			// le text est consume mais il en reste dans la pattern
			xbox_assert( text_end);
			r = CR_SMALLER;
		}
	}
	return r;
}


/*
	For Japanese, we need to take care of secondary collation elements.
	The algorithm gets a lot more complex and probably a lot slower.
*/
CompareResult VICUCollator::_CompareString_LikeNoDiac_secondary( const UniChar* inText, sLONG inSize, const UniChar* inPattern, sLONG inPatternSize)
{
	UErrorCode err = U_ZERO_ERROR;

	ucol_setText( fTextElements, inText, inSize, &err);
	ucol_setText( fPatternElements, inPattern, inPatternSize, &err);
	
	uLONG text_key;
	uLONG pattern_key;
		
	CompareResult r = CR_EQUAL;

	// will need to collect secondary collation elements
	ucol_CEBuf	textCEs;
	ucol_CEBuf	patternCEs;
	UCOL_INIT_CEBUF( textCEs);
	UCOL_INIT_CEBUF( patternCEs);

	bool text_end = (inSize == 0);
	bool pattern_end = _NextPrimaryKey( fPatternElements, pattern_key, patternCEs, &err);
		
	while( !text_end )
	{
		if (!pattern_end)
		{
			if (pattern_key == fWildCharKey)
			{
				bool continuePatternMatching;
				do
				{
					continuePatternMatching = false;

					// we will have to find a pattern
					// remember the pattern starting position
					sLONG oldpattern_pos;

					do {
						UCOL_RESET_CEBUF( patternCEs);	// start collecting pattern secondary elements
						oldpattern_pos = ucol_getOffset( fPatternElements);
						pattern_end = _NextPrimaryKey( fPatternElements, pattern_key, patternCEs, &err);
					} while( !pattern_end && (pattern_key == fWildCharKey) );	// skip extra wildchars

					if (!pattern_end)
					{

						// search for a text key matching the first pattern key
						do
						{
							UCOL_RESET_CEBUF( textCEs);	// start collecting text secondary elements
							text_end = _NextPrimaryKey( fTextElements, text_key, textCEs, &err);
						} while( !text_end && (text_key != pattern_key) );
						
						if (text_end)
						{
							// no more text
							r = CR_SMALLER;
							break;
						}

						// remember text pos
						sLONG oldtext_pos = ucol_getOffset( fTextElements);
						
						// consume pattern and text while they match
						do {
							pattern_end = _NextPrimaryKey( fPatternElements, pattern_key, patternCEs, &err);
							if ( pattern_end || (pattern_key == fWildCharKey) )
								break;
							text_end = _NextPrimaryKey( fTextElements, text_key, textCEs, &err);
						} while( !text_end && (pattern_key == text_key) );
					
						if (pattern_end)
						{
							// the pattern is consumed
							// did we also reach the end of the text?
							text_end = UCOL_CLOSE_CEBUF( textCEs, fTextElements);
							if (text_end)
								r = ucol_CEBuf_CheckSecondary( textCEs, patternCEs, fWildCharKey);
							else
								r = CR_BIGGER;

							if (r != CR_EQUAL)
							{
								// mismatch -> reset text pos and pattern pos
								pattern_end = false;

								ucol_setOffset( fTextElements, oldtext_pos, &err);
								ucol_setOffset( fPatternElements, oldpattern_pos, &err);
								
								continuePatternMatching = true;
							}
						}
						else if (pattern_key == fWildCharKey)
						{
							// found a new wildchar
							// check secondary match
							// close text CE buffer
							UCOL_CLOSE_CEBUF( textCEs, fTextElements);
							CompareResult r2 = ucol_CEBuf_CheckSecondary( textCEs, patternCEs, fWildCharKey);
							if (r2 != CR_EQUAL)
							{
								// mismatch -> reset text pos and pattern pos
								ucol_setOffset( fTextElements, oldtext_pos, &err);
								ucol_setOffset( fPatternElements, oldpattern_pos, &err);
								continuePatternMatching = true;
							}
						}
						else
						{
							// text is consumed but some pattern remain
							if (text_end)
							{
								r = CR_SMALLER;
								break;
							}
							else
							{
								// we are here because of a pattern mismatch -> reset text pos and pattern pos
								ucol_setOffset( fTextElements, oldtext_pos, &err);
								ucol_setOffset( fPatternElements, oldpattern_pos, &err);
								continuePatternMatching = true;
							}
						}
					}
					else
					{
						// wildchar at the end: get out
						text_end = true;
					}
				} while( continuePatternMatching);
			}
			else
			{
				// consume pattern and text while they match
				// start collecting text secondary elements
				UCOL_RESET_CEBUF( textCEs);
				text_end = _NextPrimaryKey( fTextElements, text_key, textCEs, &err);
				if (text_end)
				{
					r = CR_SMALLER;	// quid des ignorables characters?
					break;
				}
				else if (text_key == pattern_key)
				{
					do {
						pattern_end = _NextPrimaryKey( fPatternElements, pattern_key, patternCEs, &err);
						if ( pattern_end || (pattern_key == fWildCharKey) )
							break;
						text_end = _NextPrimaryKey( fTextElements, text_key, textCEs, &err);
					} while( !text_end && (pattern_key == text_key) );

					if (pattern_end)
					{
						text_end = UCOL_CLOSE_CEBUF( textCEs, fTextElements);
						if (text_end)
							r = ucol_CEBuf_CheckSecondary( textCEs, patternCEs, fWildCharKey);
						else
							r = CR_BIGGER;
						break;
					}
					
					// found a wildchar
					if (pattern_key == fWildCharKey)
					{
						// check secondary match
						// close text CE buffer
						UCOL_CLOSE_CEBUF( textCEs, fTextElements);
						r = ucol_CEBuf_CheckSecondary( textCEs, patternCEs, fWildCharKey);
						if (r != CR_EQUAL)
							break;
					}
					else
					{
						// we are here because of a pattern mismatch
						if (text_end)
						{
							r = CR_SMALLER;
						}
						else
						{
							r = (text_key > pattern_key) ? CR_BIGGER : CR_SMALLER;
						}
						break;
					}
				}
				else
				{
					r = (text_key > pattern_key) ? CR_BIGGER : CR_SMALLER;
					break;
				}
			}
		}
		else
		{
			// no more pattern but some text remain
			r = CR_BIGGER;
			break;
		}
	}

	if (r == CR_EQUAL)
	{
		sLONG pattern_pos = ucol_getOffset( fPatternElements);
		if (pattern_pos <= inPatternSize && !pattern_end)
		{
			// skip extra wild chars
			while( (pattern_pos <= inPatternSize) && (inPattern[pattern_pos-1] == fWildChar) )
				++pattern_pos;
			if (pattern_pos > inPatternSize)
			{
				pattern_end = true;
			}
		}
		if ((pattern_pos <= inPatternSize) && !pattern_end)
		{
			// le text est consume mais il en reste dans la pattern
			xbox_assert( text_end);
			r = CR_SMALLER;
		}
	}
	UCOL_DEINIT_CEBUF( textCEs);
	UCOL_DEINIT_CEBUF( patternCEs);
	return r;
}


CompareResult VICUCollator::_CompareString_LikeDiac( const UniChar* inText, sLONG inSize, const UniChar* inPattern, sLONG inPatternSize)
{
	CompareResult r = CR_EQUAL;
	
	const UniChar *pattern = inPattern;
	const UniChar *pattern_end = inPattern + inPatternSize;

	const UniChar *text = inText;
	const UniChar *text_end = inText + inSize;
	
	while( pattern != pattern_end)
	{
		if (*pattern == fWildChar)
		{
			// consume all extra wild chars
			while( *++pattern == fWildChar)
				;
			
			if (pattern == pattern_end)
			{
				// nothing follows the wild chars -> we are done consuming the pattern
				// check remaining text is empty (beware of ignorable chars)
				if ( (text == text_end) || (CompareString( text, (sLONG) (text_end - text), NULL, 0, true) == CR_EQUAL) )
				{
					r = CR_EQUAL;
				}
				else
				{
					r = CR_BIGGER;
				}
				break;
			}
			else
			{
				// have to find a sequence.
				// search end of sequence.
				const UniChar *pattern_word = pattern;
				const UniChar *pattern_word_end = pattern;
				while( ++pattern_word_end != pattern_end)
				{
					if (*pattern_word_end == fWildChar)
						break;
				}
				
				sLONG foundLength;
				sLONG pos = FindString( text, (sLONG) (text_end - text), pattern_word, (sLONG) (pattern_word_end - pattern_word), true, &foundLength);
				
				if (pos > 0)
				{
					// found.
					// continue with following text and next pattern sequence.
					text += foundLength;
					pattern = pattern_word_end;
				}
				else
				{
					// not found.
					// break here with compare result.
					r = CompareString( text, (sLONG) (text_end - text), pattern_word, (sLONG) (pattern_word_end - pattern_word), true);
					break;
				}
			}
		}
		else
		{
			// must begins with pattern sequence.
			const UniChar *pattern_word = pattern;
			const UniChar *pattern_word_end = pattern;
			while( ++pattern_word_end != pattern_end)
			{
				if (*pattern_word_end == fWildChar)
					break;
			}
			sLONG foundLength;
			if ( (pattern_word_end != pattern_end) && BeginsWithString( text, (sLONG) (text_end - text), pattern_word, (sLONG) (pattern_word_end - pattern_word), true, &foundLength))
			{
				// found.
				// continue with following text and next pattern sequence.
				text += foundLength;
				pattern = pattern_word_end;
			}
			else
			{
				// not found.
				// break here with compare result.
				r = CompareString( text, (sLONG) (text_end - text), pattern_word, (sLONG) (pattern_word_end - pattern_word), true);
				break;
			}
		}
	}
	
	return r;
}

#if 0

// Default rules for break iterator are as follows
// and complies with http://www.unicode.org/reports/tr29/

$CR          = [\p{Grapheme_Cluster_Break = CR}];
$LF          = [\p{Grapheme_Cluster_Break = LF}];
$Control     = [\p{Grapheme_Cluster_Break = Control}];
$Prepend     = [\p{Grapheme_Cluster_Break = Prepend}];
$Extend      = [\p{Grapheme_Cluster_Break = Extend}];
$SpacingMark = [\p{Grapheme_Cluster_Break = SpacingMark}];
$L       = [\p{Grapheme_Cluster_Break = L}];
$V       = [\p{Grapheme_Cluster_Break = V}];
$T       = [\p{Grapheme_Cluster_Break = T}];
$LV      = [\p{Grapheme_Cluster_Break = LV}];
$LVT     = [\p{Grapheme_Cluster_Break = LVT}];
!!chain;
!!forward;
$CR $LF;
$L ($L | $V | $LV | $LVT);
($LV | $V) ($V | $T);
($LVT | $T) $T;
[^$Control $CR $LF] $Extend;
[^$Control $CR $LF] $SpacingMark;
$Prepend [^$Control $CR $LF];
!!reverse;
$LF $CR;
($L | $V | $LV | $LVT) $L;
($V | $T) ($LV | $V);
$T ($LVT | $T);
$Extend      [^$Control $CR $LF];
$SpacingMark [^$Control $CR $LF];
[^$Control $CR $LF] $Prepend;
!!safe_reverse;
!!safe_forward;
#endif

// ACI0074356 default rules don't find "\na" in "\r\na" and that breaks too much 4D parsing code.
// So we use default rules without the ($CR $LF;) line
const VString sCustomCharBreakRules = "\
$CR          = [\\p{Grapheme_Cluster_Break = CR}];\
$LF          = [\\p{Grapheme_Cluster_Break = LF}];\
$Control     = [\\p{Grapheme_Cluster_Break = Control}];\
$Prepend     = [\\p{Grapheme_Cluster_Break = Prepend}];\
$Extend      = [\\p{Grapheme_Cluster_Break = Extend}];\
$SpacingMark = [\\p{Grapheme_Cluster_Break = SpacingMark}];\
$L       = [\\p{Grapheme_Cluster_Break = L}];\
$V       = [\\p{Grapheme_Cluster_Break = V}];\
$T       = [\\p{Grapheme_Cluster_Break = T}];\
$LV      = [\\p{Grapheme_Cluster_Break = LV}];\
$LVT     = [\\p{Grapheme_Cluster_Break = LVT}];\
!!chain;\
!!forward;\
$L ($L | $V | $LV | $LVT);\
($LV | $V) ($V | $T);\
($LVT | $T) $T;\
[^$Control $CR $LF] $Extend;\
[^$Control $CR $LF] $SpacingMark;\
$Prepend [^$Control $CR $LF];\
!!reverse;\
($L | $V | $LV | $LVT) $L;\
($V | $T) ($LV | $V);\
$T ($LVT | $T);\
$Extend      [^$Control $CR $LF];\
$SpacingMark [^$Control $CR $LF];\
[^$Control $CR $LF] $Prepend;\
!!safe_reverse;\
!!safe_forward;\
";

UStringSearch* VICUCollator::_InitSearch( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics, void *outStatus)
{
	UErrorCode status = U_ZERO_ERROR;

	UStringSearch **searchHolder = inWithDiacritics ? &fSearchWithDiacritics : &fSearchWithoutDiacritics;

	if (*searchHolder == NULL)
	{
		static UBreakIterator *sCustomBreakIter = NULL;
		if (sCustomBreakIter == NULL)
		{
			// ubrk_openRules is really slow with icu4 (around 15ms in final, 250ms in debug)
			// so we maintain one and safeClone it which is a lot faster ( < ms).
			UParseError parseError;
			UBreakIterator *breakIter = ubrk_openRules( sCustomCharBreakRules.GetCPointer(), sCustomCharBreakRules.GetLength(), NULL, 0, &parseError, &status);
			xbox_assert( U_SUCCESS( status));
			if (VInterlocked::CompareExchangePtr( (void**) &sCustomBreakIter, NULL, breakIter) != NULL)
			{
				ubrk_close( breakIter);
			}
		}

		int32_t size = U_BRK_SAFECLONE_BUFFERSIZE;
		UBreakIterator *breakIterator = ubrk_safeClone( sCustomBreakIter, NULL, &size, &status);
		xbox_assert( U_SUCCESS( status));

		const UCollator *collator = inWithDiacritics ? ((RuleBasedCollator*) fTertiaryCollator)->getUCollator() : ((RuleBasedCollator*) fPrimaryCollator)->getUCollator();
		*searchHolder = usearch_openFromCollator( inPattern, inPatternSize, inText, inTextSize, collator, breakIterator /* breakiter */, &status);
		if (*searchHolder == NULL)
			ubrk_close( breakIterator);
	}
	else
	{
		usearch_setText( *searchHolder, inText, inTextSize, &status);
		usearch_setPattern( *searchHolder, inPattern, inPatternSize, &status);
	}
	
	*(UErrorCode*)outStatus = status;	// don't want to include icu headers in VCollator.h
	return *searchHolder;
}


sLONG VICUCollator::FindString( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics, sLONG *outFoundLength)
{
	bool done = false;
	sLONG pos;
	sLONG matchedLength;

	// shortcut for one char searching
	if ( (inPatternSize == 1) && !inWithDiacritics && (*inPattern < 256))
	{
		UErrorCode status = U_ZERO_ERROR;
		ucol_setText( fTextElements, inText, inTextSize, &status);
		ucol_setText( fPatternElements, inPattern, inPatternSize, &status);
		
		if (U_SUCCESS(status))
		{
			uLONG pattern_key;
			bool pattern_end = _NextPrimaryKey( fPatternElements, pattern_key, &status);
			
			if (!pattern_end)
			{
				uLONG pattern_key2;
				pattern_end = _NextPrimaryKey( fPatternElements, pattern_key2, &status);
				if (pattern_end)
				{
					pos = 0;
					matchedLength = 0;
					do
					{
						sLONG pos_begin;
						sLONG key;
						do
						{
							pos_begin = ucol_getOffset( fTextElements);
							key = ucol_next( fTextElements, &status);
						} while( (key & UCOL_PRIMARYMASK) == 0 );	// UCOL_NULLORDER is FFFFFFFF so no need to test for it.

						if (key == UCOL_NULLORDER)	// found text end
							break;

						uLONG text_key = (uLONG) (key & UCOL_PRIMARYMASK);
						if (text_key == pattern_key)
						{
							sLONG pos_end = ucol_getOffset( fTextElements);

							// one UniChar may produce multiple collation elements.
							// we only check the first one produced which is the only one that made ucol_getOffset() advance the cursor.
							
							if (pos_end != pos_begin)
							{
								pos = pos_begin + 1;
								matchedLength = pos_end - pos_begin;
								break;
							}
						}
					} while( true);
					done = true;
				}
			}
			else
			{
				// the pattern is an ignorable character
				pos = (inTextSize != 0) ? 1 : 0;
				matchedLength = 0;
				done = true;
			}
		}
	}
	
#if VERSIONDEBUG
	bool debug_done = done;
	sLONG debug_pos = done ? pos : 0;
	sLONG debug_matchedLength = done ? matchedLength : 0;
	done = false;
#endif

	if (!done)
	{
		UErrorCode status;
		UStringSearch *search = _InitSearch( inText, inTextSize, inPattern, inPatternSize, inWithDiacritics, &status);
		if (U_SUCCESS(status))
		{
			sLONG result = usearch_first( search, &status);
			if (result != USEARCH_DONE)
			{
				pos = result + 1;
				matchedLength = usearch_getMatchedLength( search);
			}
			else
			{
				pos = 0;
				matchedLength = 0;
			}
		}
		else if (testAssert( status == U_ILLEGAL_ARGUMENT_ERROR))	// the only one we silently accept
		{
			if ( (inPatternSize == 0) && (inTextSize != 0) )
			{
				pos = 1;
				matchedLength = 0;
			}
			else
			{
				pos = 0;
				matchedLength = 0;
			}
		}
	}

#if VERSIONDEBUG
	if (debug_done)
	{
		xbox_assert( debug_pos == pos);
		xbox_assert( debug_matchedLength == matchedLength);
	}
#endif

	if (outFoundLength)
		*outFoundLength = matchedLength;

	return pos;
}


sLONG VICUCollator::ReversedFindString( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics, sLONG *outFoundLength)
{
	sLONG pos = 0;
	sLONG matchedLength;

	UErrorCode status;
	UStringSearch *search = _InitSearch( inText, inTextSize, inPattern, inPatternSize, inWithDiacritics, &status);
	if (U_SUCCESS(status))
	{
		sLONG result = usearch_last( search, &status);
		if (result != USEARCH_DONE)
		{
			pos = result + 1;
			matchedLength = usearch_getMatchedLength( search);
		}
		else
		{
			pos = 0;
			matchedLength = 0;
		}
	}
	else
	{
		if (testAssert( status == U_ILLEGAL_ARGUMENT_ERROR))	// the only one we silently accept
		{
			if ( (inPatternSize == 0) && (inTextSize != 0) )
			{
				pos = 1;
				matchedLength = 0;
			}
			else
			{
				pos = 0;
				matchedLength = 0;
			}
		}
	}

	if (outFoundLength)
		*outFoundLength = matchedLength;

	return pos;
}


CompareResult VICUCollator::_BeginsWithString_NoDiac( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize)
{
	UErrorCode err = U_ZERO_ERROR;

	ucol_setText( fTextElements, inText, inTextSize, &err);
	ucol_setText( fPatternElements, inPattern, inPatternSize, &err);
	
	uLONG text_key;
	uLONG pattern_key;

	uLONG old_text_key = 0;
	uLONG old_pattern_key = 0;
		
	bool text_end = _NextPrimaryKey( fTextElements, fElementKeyMask, text_key, &err);
	bool pattern_end = _NextPrimaryKey( fPatternElements, fElementKeyMask, pattern_key, &err);
		
	while( !text_end && (text_key == pattern_key) )
	{
		old_text_key = text_key;
		old_pattern_key = pattern_key;
		pattern_end = _NextPrimaryKey( fPatternElements, fElementKeyMask, pattern_key, &err);
		text_end = _NextPrimaryKey( fTextElements, fElementKeyMask, text_key, &err);
	}

	if (pattern_end)
		return CR_EQUAL;
	
	if (text_end)
		return (old_text_key <= old_pattern_key) ? CR_SMALLER : CR_BIGGER;

	return (text_key <= pattern_key) ? CR_SMALLER : CR_BIGGER;
}


bool VICUCollator::BeginsWithString( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics, sLONG *outFoundLength)
{
	if (inPatternSize == 0)
	{
		if (outFoundLength != NULL)
			*outFoundLength = 0;
		return true;
	}

	bool done = false;
	bool found = false;
	sLONG matchedLength = 0;

	if (!inWithDiacritics)
	{
		// faster than usearch
		UErrorCode err = U_ZERO_ERROR;

		ucol_setText( fTextElements, inText, inTextSize, &err);
		ucol_setText( fPatternElements, inPattern, inPatternSize, &err);
		
		uLONG text_key;
		uLONG pattern_key;
			
		bool text_end = _NextPrimaryKey( fTextElements, fElementKeyMask, text_key, &err);
		bool pattern_end = _NextPrimaryKey( fPatternElements, fElementKeyMask, pattern_key, &err);
			
		while( !text_end && (text_key == pattern_key) )
		{
			pattern_end = _NextPrimaryKey( fPatternElements, fElementKeyMask, pattern_key, &err);
			text_end = _NextPrimaryKey( fTextElements, fElementKeyMask, text_key, &err);
		}
		
		if (pattern_end)
			matchedLength = text_end ? inTextSize : (ucol_getOffset( fTextElements) - 1);
		else
			matchedLength = 0;
		
		found = pattern_end;
	}

#if VERSIONDEBUG
	bool debug_done = done;
	bool debug_found = done ? found : 0;
	sLONG debug_matchedLength = done ? matchedLength : 0;
	done = false;
#endif
	
	if (!done)
	{
		sLONG pos = 0;
		UErrorCode status;
		UStringSearch *search = _InitSearch( inText, inTextSize, inPattern, inPatternSize, inWithDiacritics, &status);

		if (U_SUCCESS(status))
		{
			sLONG result = usearch_first( search, &status);
			if (result != USEARCH_DONE)
			{
				pos = result + 1;
				matchedLength = usearch_getMatchedLength( search);
			}
		}
		else if (testAssert( status == U_ILLEGAL_ARGUMENT_ERROR))	// the only one we silently accept
		{
			if ( (inPatternSize == 0) && (inTextSize != 0) )
				pos = 1;
		}
		found = (pos == 1);
	}

#if VERSIONDEBUG
	if (debug_done)
	{
		xbox_assert( debug_found == found);
		xbox_assert( debug_matchedLength == matchedLength);
	}
#endif

	if (outFoundLength)
		*outFoundLength = matchedLength;

	return found;
}


bool VICUCollator::EndsWithString( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics, sLONG *outFoundLength)
{
	sLONG pos = 0;
	sLONG matchedLength = 0;

	UErrorCode status;
	UStringSearch *search = _InitSearch( inText, inTextSize, inPattern, inPatternSize, inWithDiacritics, &status);

	if (U_SUCCESS(status))
	{
		sLONG result = usearch_last( search, &status);
		if (result != USEARCH_DONE)
		{
			pos = result + 1;
			matchedLength = usearch_getMatchedLength( search);
		}
	}
	else if (testAssert( status == U_ILLEGAL_ARGUMENT_ERROR))	// the only one we silently accept
	{
		if (inPatternSize == 0)
			pos = inTextSize + 1;
	}

	if (outFoundLength)
		*outFoundLength = matchedLength;

	return (pos > 0) && (pos + matchedLength - 1 == inTextSize);
}


/*
	static
*/
void VICUCollator::GetLocalesHavingCollator( const xbox_icu::Locale& inDisplayNameLocale, std::vector<const char*>& outLocales, std::vector<VString>& outCollatorDisplayNames)
{
	UniChar	name[256];
	int32_t count = ucol_countAvailable();
	for( int32_t i = 0 ; i < count ; ++i)
	{
		const char* locale = ucol_getAvailable( i);
		outLocales.push_back( locale);

		UnicodeString displayName;
		Collator::getDisplayName( Locale(locale), inDisplayNameLocale, displayName);

		VString s;
		UniChar *p = s.GetCPointerForWrite( displayName.length());
		if (p != NULL)
		{
			UErrorCode err = U_ZERO_ERROR;
			s.Validate( displayName.extract( p, s.GetEnsuredSize(), err));
		}
		outCollatorDisplayNames.push_back( s);
	}
}


void VICUCollator::GetStringComparisonAlgorithmSignature( VString& outSignature) const
{
	outSignature.Printf( "icu:%d.%d.%d", U_ICU_VERSION_MAJOR_NUM, U_ICU_VERSION_MINOR_NUM, U_ICU_VERSION_PATCHLEVEL_NUM);
}


#endif	// #if USE_ICU


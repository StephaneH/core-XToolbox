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
#include "VValueBag.h"
#include "VStream.h"
#include "VString.h"
#include "VProcess.h"
#include "VUUID.h"
#include "VUnicodeTableFull.h"
#include "VIntlMgr.h"
#include "VJSONTools.h"

#include <locale.h>

BEGIN_TOOLBOX_NAMESPACE

/*
	Private class
	
	Delegate for VPackedDictionary of VBagArray
*/

class VPackedValue_VBagArray
{
public:
	typedef VBagArray value_type;
	
	template <class ForwardIterator>
	static	void Clear( ForwardIterator inBegin, ForwardIterator inEnd)
				{
					for( ForwardIterator i = inBegin ; i != inEnd ; ++i)
					{
						if (i->fValue)
						{
							i->fValue->Release();
							i->fValue = NULL;
						}
					}
				}
	
	template <class ForwardIterator>
	static	size_t Clone( ForwardIterator inBegin, ForwardIterator inEnd)
				{
					size_t success_count = 0;
					for( ForwardIterator i = inBegin ; i != inEnd ; ++i, ++success_count)
					{
						i->fValue = new value_type( *i->fValue);
						if (i->fValue == NULL)
						{
							// abort
							for( ++i ; i != inEnd ; ++i)
								i->fValue = NULL;
						}
					}
					return success_count;
				}
	
	template <class ForwardIterator>
	static	VError WriteToStream( VStream *inStream, ForwardIterator inBegin, ForwardIterator inEnd)
				{
					for( ForwardIterator i = inBegin ; i != inEnd ; ++i)
					{
						i->fValue->WriteToStreamMinimal( inStream);
					}
					return inStream->GetLastError();
				}
	
	template <class ForwardIterator>
	static	VError ReadFromStream( VStream *inStream, ForwardIterator inBegin, ForwardIterator inEnd)
				{
					VError err = inStream->GetLastError();
					for( ForwardIterator i = inBegin ; (i != inEnd) && (err == VE_OK) ; ++i)
					{
						i->fValue = new value_type;
						if (i->fValue != NULL)
							err = i->fValue->ReadFromStreamMinimal( inStream);
						else
						{
							err = VE_MEMORY_FULL;
						}
					}
					return err;
				}
	
	static	void SetValue( value_type* inValue, value_type*& ioCachedValue)
				{
					CopyRefCountable( &ioCachedValue, inValue);
				}
	
};



static const UniChar sXMLEscapeChars_Attributes[] = { CHAR_AMPERSAND, CHAR_GREATER_THAN_SIGN,
							CHAR_QUOTATION_MARK, CHAR_LESS_THAN_SIGN, CHAR_APOSTROPHE };

static const UniChar sXMLEscapeChars_Contents[] = { CHAR_AMPERSAND, CHAR_GREATER_THAN_SIGN,
							CHAR_QUOTATION_MARK, CHAR_LESS_THAN_SIGN, CHAR_APOSTROPHE };


const VValueBag::TypeInfo	VValueBag::sInfo;


static bool _CheckOverflowLong8( sLONG8 inValue, const UniChar *inBegin, const UniChar *inEnd)
{
	assert( inEnd - inBegin >= 0);
	
	bool ok = true;
	
	if (inValue != 0)
	{
		sLONG8 value = inValue;
		const UniChar *p = inEnd;
		do {
			if ( (p == inBegin) || (p[-1] != (UniChar) (CHAR_DIGIT_ZERO + (value%10L)) ) )
			{
				ok = false;
				break;
			}
			--p;
			value /= 10L;	// bug CW 9.5 ?
		} while( value > 0);
		if (ok)
			ok = (p == inBegin);
	}
	else
	{
		ok = (inEnd == inBegin) && (*inBegin == CHAR_DIGIT_ZERO);
	}
	
	return ok;
}


static VUUID *_ParseUUIDValue( const UniChar *inNonWhiteBegin, const UniChar *inNonWhiteEnd)
{
	// strings like "{19D215C0-3884-45af-A173-14F5CCD73C49}"
	// { } are optionnal but must be balanced.
	// '-' is optionnal and can be placed anywhere but inside.

	VUUID *value = NULL;
	
	// at least 32 caracters,
	if (inNonWhiteEnd - inNonWhiteBegin >= 32)
	{
		if ( (*inNonWhiteBegin == CHAR_LEFT_CURLY_BRACKET) && (inNonWhiteEnd[-1] == CHAR_RIGHT_CURLY_BRACKET) )
		{
			++inNonWhiteBegin;
			--inNonWhiteEnd;
		}

		VUUIDBuffer	buffer;
		sLONG	destIndex = 0;

		const UniChar *p = inNonWhiteBegin;
		for( ; (p != inNonWhiteEnd) && (destIndex < 32) ; ++p)
		{
			UniChar	digit = *p;
			UniChar	nibble;

			if (digit >= CHAR_DIGIT_ZERO && digit <= CHAR_DIGIT_NINE)
				nibble = (UniChar) (digit - CHAR_DIGIT_ZERO);
			else if (digit >= CHAR_LATIN_CAPITAL_LETTER_A && digit <= CHAR_LATIN_CAPITAL_LETTER_F)
				nibble = (UniChar) (10 + digit - CHAR_LATIN_CAPITAL_LETTER_A);
			else if (digit >= CHAR_LATIN_SMALL_LETTER_A && digit <= CHAR_LATIN_SMALL_LETTER_F)
				nibble = (UniChar) (10 + digit - CHAR_LATIN_SMALL_LETTER_A);
			else if ( ( (digit == CHAR_HYPHEN_MINUS) || (digit == CHAR_MINUS_SIGN) ) && (destIndex != 0) )
				continue;
			else
				break;

			if ((destIndex & 1) != 0)
			{
				buffer.fBytes[destIndex/2] += nibble;
			}
			else
			{
				buffer.fBytes[destIndex/2] = (uBYTE) (nibble * 16);
			}
			++destIndex;
		}
		if ( (destIndex == 32) && (p == inNonWhiteEnd) )
			value = new VUUID( buffer);
	}
	
	return value;
}


static VValueSingle *_ParseDecimalValue( const UniChar *inNonWhiteBegin, const UniChar *inNonWhiteEnd, bool inWithMinusSign)
{
	assert( *inNonWhiteBegin >= CHAR_DIGIT_ZERO && *inNonWhiteBegin <= CHAR_DIGIT_NINE);

	VValueSingle *value = NULL;
	const UniChar *p = inNonWhiteBegin;
	
	// we need to scan the string ourselves to detect simple cases for VLong & VLong8
	// and to perform stronger validation than ::strtod.
	
	// once validation is done and simple cases are solved,
	// we call ::strtod with an ascii copy of the string we have collected.
					
	char	strtod_buff[200];
	sLONG	strtod_len = 0;

	// ::strtod works with current local but we need to expect the '.' char.
    const char  theDecimalPointChar = ::localeconv()->decimal_point[0];	// inspired from xalan
	
	bool failed = false;
	sLONG8 mantissa = 0;
	sLONG8 exponent = 0;
	
	if (inWithMinusSign)
	{
		if (strtod_len < sizeof( strtod_buff)-1)
			strtod_buff[strtod_len++] = '-';
	}
	
	sLONG mantissa_exponent = 0;
	do {
		mantissa = mantissa * 10L + *p - CHAR_DIGIT_ZERO;
		if (strtod_len < sizeof( strtod_buff)-1)
			strtod_buff[strtod_len++] = (char)( '0' + *p - CHAR_DIGIT_ZERO);
		++p;
	} while( *p >= CHAR_DIGIT_ZERO && *p <= CHAR_DIGIT_NINE);

	// consume '.'
	
	bool withDot = (*p == CHAR_FULL_STOP);
	if (withDot)
	{
		if (strtod_len < sizeof( strtod_buff)-1)
			strtod_buff[strtod_len++] = theDecimalPointChar;
		
		++p;
		if ( *p >= CHAR_DIGIT_ZERO && *p <= CHAR_DIGIT_NINE)
		{
			sLONG8 mantissa_temp = mantissa;
			sLONG mantissa_exponent_temp = 0;
			do
			{
				if (strtod_len < sizeof( strtod_buff)-1)
					strtod_buff[strtod_len++] = (char)( '0' + *p - CHAR_DIGIT_ZERO);
				
				mantissa_temp = mantissa_temp * 10L + *p - CHAR_DIGIT_ZERO;
				mantissa_exponent_temp--;
				if (*p != CHAR_DIGIT_ZERO)	// ignore trailing zeros
				{
					mantissa_exponent = mantissa_exponent_temp;
					mantissa = mantissa_temp;
				}
				++p;
			} while( *p >= CHAR_DIGIT_ZERO && *p <= CHAR_DIGIT_NINE);
		}
		else
		{
			failed = true;	// the dot must be followed by an integer
		}
	}
	
	// consume 'e' or 'E'
	bool withExponent = (*p == CHAR_LATIN_SMALL_LETTER_E || *p == CHAR_LATIN_CAPITAL_LETTER_E);
	if (withExponent)
	{
		if (strtod_len < sizeof( strtod_buff)-1)
			strtod_buff[strtod_len++] = 'E';
		++p;

		// comsume '+' or '-'
		
		bool minus_exponent = false;
		bool plus_exponent = false;
		if (*p == CHAR_PLUS_SIGN)
		{
			plus_exponent = true;
			++p;
		}
		else if ( (*p == CHAR_HYPHEN_MINUS) || (*p == CHAR_MINUS_SIGN) )
		{
			minus_exponent = true;
			++p;
		}
	
		if (minus_exponent)
		{
			if (strtod_len < sizeof( strtod_buff)-1)
				strtod_buff[strtod_len++] = '-';
		}

		// consume exponent
		if (*p >= CHAR_DIGIT_ZERO && *p <= CHAR_DIGIT_NINE)
		{
			do {
				if (strtod_len < sizeof( strtod_buff)-1)
					strtod_buff[strtod_len++] = (char)( '0' + *p - CHAR_DIGIT_ZERO);

				exponent = exponent * 10L + *p - CHAR_DIGIT_ZERO;
				++p;
			} while( *p >= CHAR_DIGIT_ZERO && *p <= CHAR_DIGIT_NINE);
		}
		else
		{
			failed = true;	// 'e' or 'E' must be followed by an integer
		}
		
		if (minus_exponent)
			exponent = -exponent;
		
	}

	if (!failed && (p == inNonWhiteEnd) )
	{
		strtod_buff[strtod_len] = 0;
		
		// see if we really need a VReal
		if (withExponent || withDot)
		{
			char *pend = strtod_buff;
			errno = 0;
			double d = ::strtod( strtod_buff, &pend);
			// check overflow
			int err = errno;
			if ( (err != ERANGE) && (pend != strtod_buff) )
			{
				value = new VReal( d);
			}
		}
		else
		{
			assert( exponent == 0);
			assert( mantissa_exponent == 0);
			if (mantissa >= 0 && mantissa <= MaxLongInt)
			{
				value = new VLong( static_cast<sLONG>( inWithMinusSign ? -mantissa : mantissa));
			}
			else
			{
				if (_CheckOverflowLong8( mantissa, inNonWhiteBegin, inNonWhiteEnd))
					value = new VLong8( inWithMinusSign ? -mantissa : mantissa);
			}
		}
	}
	return value;
}


VValueSingle *ParseXMLValue( const VString& inText)
{
	/*
		tries to conform to w3c schema definitions.
		
		int		: optionnal sign followed by digits 		-> VLong
		long	: optionnal sign followed by digits			-> VLong8
		double	: integer , optionnal 'e' or 'E', integer	-> VReal
				leading or trailing zeros are optionnal
				INF, -INF, NaN are accepted
		boolean	: true or false								-> VBoolean
				boolean could also be 0 or 1

		duration	:	tbd
		dateTime	:	tbd
						
		additionnally supports uuid stored as: {8-4-4-4-12}	-> VUUID
		
		string	: anything else
		
	*/

	const UniChar *begin = inText.GetCPointer();
	const UniChar *end = inText.GetCPointer() + inText.GetLength();
	
	VIntlMgr *intl = VIntlMgr::GetDefaultMgr();
	VValueSingle *value = NULL;
	
	// consume leading white spaces
	const UniChar *p = begin;
	while( intl->IsSpace( *p))
		++p;
	
	// find end of non white spaces
	while( (end != begin) && intl->IsSpace( end[-1]) )
	{
		--end;
	}

	// consume + or -
	bool minus = false;
	bool plus = false;

	if (*p == CHAR_PLUS_SIGN)
	{
		plus = true;
		++p;
	}
	else if ( (*p == CHAR_HYPHEN_MINUS) || (*p == CHAR_MINUS_SIGN) )
	{
		minus = true;
		++p;
	}
	
	// consume digits
	switch( *p)
	{
		case CHAR_DIGIT_ZERO:
		case CHAR_DIGIT_ONE:
		case CHAR_DIGIT_TWO:
		case CHAR_DIGIT_THREE:
		case CHAR_DIGIT_FOUR:
		case CHAR_DIGIT_FIVE:
		case CHAR_DIGIT_SIX:
		case CHAR_DIGIT_SEVEN:
		case CHAR_DIGIT_EIGHT:
		case CHAR_DIGIT_NINE:
			{
				value = _ParseDecimalValue( p, end, minus);
				if (value == NULL)
				{
					if (!minus && !plus)
						value = _ParseUUIDValue( p, end);
				}
				break;
			}
		
		case CHAR_LATIN_SMALL_LETTER_T:		// true
			if (p[1] == CHAR_LATIN_SMALL_LETTER_R && p[2] == CHAR_LATIN_SMALL_LETTER_U && p[3] == CHAR_LATIN_SMALL_LETTER_E && p+4 == end && !minus && !plus)
			{
				value = new VBoolean( true);
			}
			break;
		
		case CHAR_LATIN_SMALL_LETTER_F:		// false
			if (p[1] == CHAR_LATIN_SMALL_LETTER_A && p[2] == CHAR_LATIN_SMALL_LETTER_L && p[3] == CHAR_LATIN_SMALL_LETTER_S && p[4] == CHAR_LATIN_SMALL_LETTER_E && p+5 == end && !minus && !plus)
			{
				value = new VBoolean( false);
			}
			else if (!minus && !plus)
				value = _ParseUUIDValue( p, end);
			break;
		
		case CHAR_LATIN_CAPITAL_LETTER_I:	// INF
			if (p[1] == CHAR_LATIN_CAPITAL_LETTER_N && p[2] == CHAR_LATIN_CAPITAL_LETTER_F && p+3 == end && !plus)
			{
				value = new VReal( minus ? -HUGE_VAL : HUGE_VAL);	// inspired from xalan
			}
			break;
		
		case CHAR_LATIN_CAPITAL_LETTER_N:		// NaN
			if (p[1] == CHAR_LATIN_SMALL_LETTER_A && p[2] == CHAR_LATIN_CAPITAL_LETTER_N && p+3 == end && !minus && !plus)
			{
				static Real sNaN = ::sqrt( -2.01);	// inspired from xalan
				value = new VReal( sNaN);
			}
			break;

		case CHAR_LEFT_CURLY_BRACKET:	// maybe a uuid {8-4-4-4-12}
			if (!minus && !plus)
				value = _ParseUUIDValue( p, end);
			break;
	}

	// if could not find anything, clone input
	if (value == NULL)
	{
		value = (VValueSingle *) inText.Clone();
	}
	
	return value;
}



VValueBag::VValueBag( const VValueBag &inOther)
{

	try
	{
		fElements = NULL;
		fAttributes = inOther.fAttributes;
		if (inOther.fElements != NULL)
			fElements = new VPackedVBagArrayDictionary( *inOther.fElements);
	}
	catch(...)
	{
	}
}


VValueBag::~VValueBag()
{
	delete fElements;
}


const VValueInfo *VValueBag::GetValueInfo() const
{
	return &sInfo;
}


VValueBag* VValueBag::Clone() const
{
	return new VValueBag(*this);
}


void VValueBag::Destroy()
{
	fAttributes.Clear();
	delete fElements;
	fElements = NULL;
}


void VValueBag::AppendToCData( const VString& inValue)
{
	VValueSingle *val = fAttributes.GetValue( CDataAttributeName());
	if (val != NULL)
	{
		VString *val_str = dynamic_cast<VString*>( val);
		if (val_str)
			val_str->AppendString( inValue);
		else
		{
			VString *temp = new VString;
			if (temp)
			{
				val->GetString( *temp);
				temp->AppendString( inValue);
				fAttributes.Replace( val, temp);
			}
		}
	}
	else
	{
		fAttributes.SetValue( CDataAttributeName(), inValue);
	}
}


bool VValueBag::IsEmpty() const
{
	return fAttributes.IsEmpty() && (fElements == NULL || fElements->IsEmpty());
}


VBagArray * VValueBag::GetElements( const StKey& inElementName)
{
	return (fElements != NULL) ? fElements->GetValue( inElementName) : NULL;
}

	
void VValueBag::SetElements( const StKey& inElementName, VBagArray* inElements)
{
	xbox_assert( inElementName.GetKeyLength() != 0);

	if (fElements == NULL)
		fElements = new VPackedVBagArrayDictionary;
	if (fElements != NULL)
		fElements->SetValue( inElementName, inElements);
}


VIndex VValueBag::GetElementNamesCount() const
{
	return (fElements != NULL) ? fElements->GetCount() : 0;
}


VBagArray* VValueBag::GetNthElementName( VIndex inIndex, VString *outName)
{
	if (testAssert( fElements != NULL))
		return fElements->GetNthValue( inIndex, outName);
	
	if (outName)
		outName->Clear();
	return NULL;
}


void VValueBag::AddElement( const StKey& inElementName, VValueBag *inBag)
{
	xbox_assert( inElementName.GetKeyLength() != 0);
	
	if (inBag != NULL)
	{
		VBagArray *bags = GetElements( inElementName);
		if (bags)
		{
			bags->AddTail( inBag);
		}
		else
		{
			bags = new VBagArray;
			if (bags != NULL)
			{
				bags->AddTail( inBag);
				if (fElements == NULL)
					fElements = new VPackedVBagArrayDictionary;
				if (fElements != NULL)
					fElements->Append( inElementName, bags);
				bags->Release();
			}
		}
	}
}


void VValueBag::RemoveElements( const StKey& inElementName)
{
	if (fElements != NULL)
		fElements->Clear( inElementName);
}


bool VValueBag::ReplaceElement( const StKey& inElementName, VValueBag *inBag)
{
	xbox_assert( inElementName.GetKeyLength() != 0);
	bool ok = false;

	if (inBag == NULL)
	{
		RemoveElements( inElementName);
	}
	else
	{
		VBagArray *bags = GetElements( inElementName);
		if (bags)
		{
			// nothing to do if we already has only one element and if that's the one we are given
			if ( (bags->GetCount() != 1) || (bags->GetNth( 1) != inBag) )
			{
				// the given bag may come from bags
				inBag->Retain();
				bags->Destroy();
				ok = bags->AddTail( inBag);
				inBag->Release();
			}
		}
		else
		{
			bags = new VBagArray;
			if (bags != NULL)
			{
				ok = bags->AddTail( inBag);
				if (fElements == NULL)
					fElements = new VPackedVBagArrayDictionary;
				if (fElements != NULL)
					fElements->Append( inElementName, bags);
				bags->Release();
			}
		}
	}
	return ok;
}


bool VValueBag::ReplaceElementByPath( const VValueBag::StKeyPath& inPath, VValueBag *inBag)
{
	if (!testAssert( !inPath.empty()))
		return false;
	
	bool ok;
	if (inPath.size() > 1)
	{
		VValueBag::StKeyPath father( inPath);
		father.pop_back();
		if (inBag == NULL)
		{
			// removing
			VValueBag *fatherBag = GetUniqueElementByPath( father);
			if (fatherBag != NULL)
				fatherBag->RemoveElements( inPath.back());
			ok = true;
		}
		else
		{
			VValueBag *fatherBag = GetElementByPathAndCreateIfDontExists( father);
			if (fatherBag != NULL)
				ok = fatherBag->ReplaceElement( inPath.back(), inBag);
			else
				ok = false;
		}
	}
	else
	{
		ok = ReplaceElement( inPath.back(), inBag);
	}
	
	return ok;
}


VValueBag* VValueBag::RetainUniqueElementWithAttribute( const StKey& inElementName,const StKey& inAttributeName,const VValueSingle& inAttributeValue)
{
	VBagArray* bagarray = GetElements( inElementName);

	sLONG nb_found = 0;
	VIndex index_found = 0;
	if (bagarray)
	{
		VIndex nb_bag = bagarray->GetCount();

		VValueSingle *value_same_type = inAttributeValue.Clone();

		if (value_same_type)
		{
			for (VIndex bag_index = 1; bag_index <= nb_bag;++bag_index)
			{
				const VValueBag *bag = bagarray->RetainNth(bag_index);
				if (bag)
				{
					bag->GetAttribute(inAttributeName,*value_same_type);
					if (*value_same_type == inAttributeValue)
					{
						++nb_found;
						index_found = bag_index;
					}
					bag->Release();
				}
				// Stop the loop if 2 or more were found, we'll return NULL
				if(nb_found > 1)
					break;
			}
			delete value_same_type;
		}
	}
	return (nb_found == 1) ? bagarray->RetainNth(index_found) : NULL;
}


VValueBag* VValueBag::GetUniqueElementAnyKind( VString& outKind)
{
	VValueBag* bag = NULL;

	if ((fElements != NULL) && (fElements->GetCount() == 1))
	{
		VBagArray* array = fElements->GetNthValue( 1, &outKind);
		if (array->GetCount() == 1)
		{
			bag = array->GetNth(1);
		}
	}

	if (bag == NULL)
		outKind.Clear();

	return bag;
}


VError VValueBag::AddElement(const IBaggable* inObject)
{
	VError err = VE_OK;

	if (inObject != NULL)
	{
		VValueBag* bag = new VValueBag;
		if (bag != NULL)
		{
			VStr31 kind;
			err = inObject->SaveToBag(*bag, kind);
			if (err == VE_OK)
				AddElement(kind, bag);
			bag->Release();
		}
		else
		{
			err = vThrowError(VE_MEMORY_FULL);
		}
	}

	return err;
}


/*
	static
*/
void VValueBag::GetKeysFromPath( const VString& inPath, StKeyPath& outKeys)
{
	VFromUnicodeConverter_UTF8 converter;

	char buffer[256];
	VIndex charsConsumed;
	VSize bytesProduced;
	const UniChar *begin = inPath.GetCPointer();
	const UniChar *end = begin + inPath.GetLength();
	for( const UniChar *pos = begin ; pos != end ; ++pos)
	{
		if (*pos == '/')
		{
			bool ok = converter.Convert( begin, (VIndex) (pos - begin), &charsConsumed, buffer, sizeof( buffer), &bytesProduced);
			if (ok)
			{
				outKeys.push_back( StKey( buffer, bytesProduced));
			}
			begin = pos + 1;
		}
	}
	if (begin != end)
	{
		bool ok = converter.Convert( begin, (VIndex) (end - begin), &charsConsumed, buffer, sizeof( buffer), &bytesProduced);
		if (ok)
		{
			outKeys.push_back( StKey( buffer, bytesProduced));
		}
	}
}


void VValueBag::GetKeysFromUTF8PathNoCopy( const char *inUTF8Path, StKeyPath& outKeys)
{
	const char *begin = inUTF8Path;
	const char *end = begin + ::strlen( inUTF8Path);
	for( const char *pos = begin ; pos != end ; ++pos)
	{
		if (*pos == '/')
		{
			outKeys.push_back( StKey( begin, pos - begin, true));
			begin = pos + 1;
		}
	}
	if (begin != end)
		outKeys.push_back( StKey( begin, end - begin, true));
}


VValueBag *VValueBag::GetUniqueElementByPath( const VString& inPath)
{
	StKeyPath keys;
	GetKeysFromPath( inPath, keys);
	return GetUniqueElementByPath( keys);
}


VValueBag *VValueBag::GetUniqueElementByPath( const StKeyPath& inPath)
{
	VValueBag *bag = this;
	for( StKeyPath::const_iterator i = inPath.begin() ; (i != inPath.end()) && (bag != NULL) ; ++i)
	{
		VBagArray* array = bag->GetElements( *i);
		bag = (array != NULL && !array->IsEmpty()) ? array->GetNth(1) : NULL;
	}
	return bag;
}


VValueBag *VValueBag::GetElementByPathAndCreateIfDontExists( const StKeyPath& inPath)
{
	VValueBag *bag = this;
	for( StKeyPath::const_iterator i = inPath.begin() ; (i != inPath.end()) && (bag != NULL) ; ++i)
	{
		VBagArray* array = bag->GetElements( *i);
		if ( (array == NULL) || array->IsEmpty() )
		{
			VValueBag *newBag = new VValueBag;
			bool ok = bag->ReplaceElement( *i, newBag);
			if (ok)
				bag = newBag;
			ReleaseRefCountable( &newBag);
		}
		else
		{
			bag = array->GetNth( 1);
		}
	}
	return bag;
}



bool VValueBag::UnionClone( const VValueBag *inBag)
{
	bool ok = true;
	
	if (inBag != NULL)
	{
		// copy attributes
		VIndex count = inBag->GetAttributesCount();
		for ( VIndex i = 1 ; i <= count ; ++i)
		{
			VString name;
			const VValueSingle *value = inBag->fAttributes.GetNthValue( i, &name);
			fAttributes.SetValue( name, *value);
		}
		
		// copy cloned elements
		VIndex elementNamesCount = inBag->GetElementNamesCount();
		for( VIndex i = 1 ; i <= elementNamesCount ; ++i)
		{
			VString name;
			const VBagArray* theBagArray = inBag->GetNthElementName( i, &name);
			for( VIndex j = 1 ; (j <= theBagArray->GetCount()) && ok ; ++j)
			{
				const VValueBag* elemBag = theBagArray->GetNth(j);
				if (elemBag != NULL)
				{
					VValueBag *clonedElemBag = elemBag->Clone();
					if (clonedElemBag != NULL)
						AddElement( name, clonedElemBag);
					else
						ok = false;
					ReleaseRefCountable( &clonedElemBag);
				}
			}
		}
	}
	
	return ok;
}


bool VValueBag::Override( const VValueBag *inOverridingBag)
{
	bool ok = true;
	
	if (inOverridingBag != NULL)
	{
		// override attributes
		VIndex count = inOverridingBag->GetAttributesCount();
		for ( VIndex i = 1 ; i <= count ; ++i)
		{
			VString name;
			const VValueSingle *value = inOverridingBag->fAttributes.GetNthValue( i, &name);
			fAttributes.SetValue( name, *value);
		}
		
		// override elements
		VIndex overridingElementNamesCount = inOverridingBag->GetElementNamesCount();
		for( VIndex i = 1 ; (i <= overridingElementNamesCount) && ok ; ++i)
		{
			VString name;
			const VBagArray* overridingElements = inOverridingBag->GetNthElementName( i, &name);

			VBagArray* elements = GetElements( name);
			
			if ( (overridingElements->GetCount() == 1) && (elements->GetCount() == 1) )
			{
				// if there's exactly one element on both sides, let's override attributes
				ok = elements->GetNth(1)->Override( overridingElements->GetNth(1));
			}
			else
			{
				// add elements
				for( VIndex j = 1 ; (j <= overridingElements->GetCount()) && ok ; ++j)
				{
					const VValueBag* elemBag = overridingElements->GetNth(j);
					if (elemBag != NULL)
					{
						VValueBag *clonedElemBag = elemBag->Clone();
						if (clonedElemBag != NULL)
							AddElement( name, clonedElemBag);
						else
							ok = false;
						ReleaseRefCountable( &clonedElemBag);
					}
				}
			}
		}
	}

	return ok;
}


void VValueBag::_DumpXML( VString& ioDump, const VString& inTitle, sLONG inIndentLevel) const
{
	VStr<50>	temp;
	
	if (inIndentLevel > 0)
		DumpXMLIndentation(ioDump, inIndentLevel);

	// get cdata index
	VIndex index_cdata = GetAttributeIndex( CDataAttributeName());

	// separates attributes dump to save stack space
	DumpXMLAttributes( ioDump, inTitle, index_cdata, inIndentLevel);

	// dump elements
	VIndex elementNamesCount = GetElementNamesCount();
	if (elementNamesCount > 0)
	{
		// elements
		sLONG subIndent = (inIndentLevel < 0) ? inIndentLevel : (inIndentLevel + 1);
		
		for( VIndex i = 1 ; i <= elementNamesCount ; ++i)
		{
			const VBagArray* theBagArray = GetNthElementName( i, &temp);
			for( VIndex j = 1 ; j <= theBagArray->GetCount() ; ++j)
			{
				const VValueBag* temp_bag = theBagArray->GetNth(j);
				if (temp_bag != NULL)
				{
					temp_bag->_DumpXML( ioDump, temp, subIndent);
				}
			}
		}
	}

	// dump cdata
	if (index_cdata > 0)
	{
		DumpXMLCData( ioDump, index_cdata);
	}
	
	// closing tag if not already closed while dumping attributes
	if ( (elementNamesCount > 0) || (index_cdata > 0) )
	{
		// end tag
		temp = CHAR_LESS_THAN_SIGN;
		temp += CHAR_SOLIDUS;
		temp += inTitle;
		temp += CHAR_GREATER_THAN_SIGN;
		if (inIndentLevel > 0)
		{
			temp += CHAR_CONTROL_000A;
//			if (index_cdata <= 0)
			DumpXMLIndentation( ioDump, inIndentLevel);
		}
		
		ioDump += temp;
	}
}


void VValueBag::DumpXML( VString& ioDump, const VString& inTitle, bool inWithIndentation) const
{
	_DumpXML( ioDump, inTitle, inWithIndentation ? 0 : -1);
}


bool VValueBag::NeedsEscapeSequence( const VString& inText, const UniChar* inEscapeFirst, const UniChar* inEscapeLast)
{
	bool found = false;
	for( const UniChar *ptr = inText.GetCPointer() ; *ptr && !found ; ++ptr)
	{
		for (const UniChar* pEscape = inEscapeFirst ; (pEscape != inEscapeLast) && !found ; ++pEscape)
		{
			found = (*ptr == *pEscape);
		}
	}
	return found;
}


void VValueBag::DumpXMLIndentation(VString& ioDump, sLONG inIndentLevel)
{
	sLONG level = Min<sLONG>(100L, inIndentLevel);

	VStr<100>	indent;
	UniChar* ptr = indent.GetCPointerForWrite();
//	ptr[0] = 13;
	for (sLONG i = 0 ; i < level ; ++i)
		ptr[i] = CHAR_CONTROL_0009;	// tab
	indent.Validate(level);

	ioDump += indent;
}


void VValueBag::DumpXMLCData( VString& ioDump, VIndex inCDataAttributeIndex) const
{
	VStr<1000> cdata;

	GetNthAttribute( inCDataAttributeIndex, NULL)->GetString( cdata);

	if (NeedsEscapeSequence( cdata, sXMLEscapeChars_Contents, sXMLEscapeChars_Contents + sizeof(sXMLEscapeChars_Contents)/sizeof(UniChar)))
	{
		ioDump += CVSTR( "<![CDATA[");

		// see if there's a "]]>" for which we need to split
		// someting like: hello ]]> world
		// becomes: <![CDATA[hello ]]>]]<![CDATA[> world]]>
		
		VIndex pos = 1;
		while( pos <= cdata.GetLength())
		{
			VIndex endTag = cdata.Find( CVSTR( "]]>"), pos, true);
			if (endTag > 0)
			{
				// add everything including ]]>
				ioDump.AppendBlock( cdata.GetCPointer() + pos - 1, (endTag - pos + 3) * sizeof( UniChar), VTC_UTF_16);
				// add ]] outside CDATA section
				ioDump.AppendString( CVSTR( "]]"));
				// open a new CDATA section and add remaining >
				ioDump += CVSTR( "<![CDATA[>");
				pos = endTag + 3;
			}
			else
			{
				// add everything left
				ioDump.AppendBlock( cdata.GetCPointer() + pos - 1, (cdata.GetLength() - pos + 1) * sizeof( UniChar), VTC_UTF_16);
				break;
			}
		}
		ioDump += CVSTR( "]]>");
	}
	else
	{
		VString toInsert;
		for (sLONG i = 0, n = cdata.GetLength(); i < n; i++)
		{
			UniChar c = cdata[ i ];
			if (c != 13 && c != 10 && c != 9)
			{
				toInsert += c;
			}
		}
		if ( ! toInsert.IsEmpty() )
			ioDump += toInsert;
	}
}


void VValueBag::DumpXMLAttributes(VString& ioDump, const VString& inTitle, VIndex inCDataAttributeIndex, sLONG inIndentLevel) const
{
	VStr<1000> attributes;
	
	// start tag
	attributes += CHAR_LESS_THAN_SIGN;
	attributes += inTitle;
	
	// attributes
	VStr<50> temp;
	VIndex count = GetAttributesCount();
	for ( VIndex i = 1 ; i <= count ; ++i)
	{
		if (i != inCDataAttributeIndex)
		{
			attributes += CHAR_SPACE;
			
			const VValueSingle *val = GetNthAttribute( i, &temp);
			attributes += temp;

			attributes += CHAR_EQUALS_SIGN;
			attributes += CHAR_QUOTATION_MARK;

			val->GetXMLString( temp, XSO_Default);
			attributes += temp;

			attributes += CHAR_QUOTATION_MARK;
		}
	}

	// need to close now if no element nor cdata
	if ( (GetElementNamesCount() == 0) && (inCDataAttributeIndex <= 0) )
	{
		attributes += CHAR_SOLIDUS;
	}

	attributes += CHAR_GREATER_THAN_SIGN;
	
	// no carriage return if no element but some cdata
	if ( (inIndentLevel >= 0) && ((GetElementNamesCount() > 0) || (inCDataAttributeIndex <= 0) ) )
	{
		attributes += CHAR_CONTROL_000A;
	}

	ioDump += attributes;
}


#define BAG_DATA_TYPE_SIGNATURE 'VBAG'

void VValueBag::_ReadFromStream_v1( VStream *inStream)
{
	VStr31 name;

	VArrayString singleNames;
	singleNames.ReadFromStream( inStream);
	
	VIndex max_singlevalues = inStream->GetLong();
	for( VIndex i = 1 ; i <= max_singlevalues; i++)
	{
		sLONG value_kind = inStream->GetLong();
		VValue* temp_value = NewValueFromValueKind((ValueKind)value_kind);
		VValueSingle* temp_single_value = dynamic_cast<VValueSingle*>(temp_value);
		if (temp_single_value != NULL)
		{
			temp_single_value->ReadFromStream( inStream);
			
			singleNames.GetString( name, i);
			fAttributes.Append( name, *temp_single_value);
		}
		delete temp_value;
	}

	VArrayString				multipleNames;

	multipleNames.ReadFromStream( inStream);
	
	VIndex max_bag_array = inStream->GetLong();
	if (max_bag_array > 0)
	{
		fElements = new VPackedVBagArrayDictionary;
		for( VIndex j = 1 ; j <= max_bag_array ; j++)
		{
			VBagArray* bag_array = new VBagArray;
			bag_array->ReadFromStreamMinimal( inStream);
			multipleNames.GetString( name, j);
			if (fElements)
				fElements->Append( name, bag_array);
			bag_array->Release();
		}
	}
}


VError VValueBag::WriteToStreamMinimal( VStream *inStream, bool inWithIndex) const
{
	uBYTE flags = 0;

	if (GetAttributesCount() > 0)
		flags |= 1;

	if (GetElementNamesCount() > 0)
		flags |= 2;
	
	inStream->PutByte( flags);

	if (GetAttributesCount() > 0)
		fAttributes.WriteToStream( inStream, inWithIndex);

	if (GetElementNamesCount() > 0)
		fElements->WriteToStream( inStream, inWithIndex);

	return inStream->GetLastError();
}


VError VValueBag::ReadFromStreamMinimal( VStream* inStream)
{
	uBYTE flags;
	VError err = inStream->GetByte( flags);
	
	if (flags & 1)
		err = fAttributes.ReadFromStream( inStream);
		
	if (flags & 2)
	{
		fElements = new VPackedVBagArrayDictionary;
		if (fElements)
			err = fElements->ReadFromStream( inStream);
		else
			err = VE_MEMORY_FULL;
	}

	return err;
}


VError VValueBag::WriteToStream(VStream* inStream, sLONG /*inParam*/) const
{
	// Data type
	inStream->PutLong( BAG_DATA_TYPE_SIGNATURE);

	// Data version
	inStream->PutWord( 1);	// major version: increment this if not backward compatible
	inStream->PutWord( 0);	// minor version

	WriteToStreamMinimal( inStream, true);

	return inStream->GetLastError();
}


VError VValueBag::ReadFromStream( VStream* inStream, sLONG /*inParam*/)
{
	assert(inStream != NULL);
	
	Destroy();
	
	VError err = inStream->GetLastError();
	if (err == VE_OK)
	{
		// LP: Read a header from the stream
		// Data type
		sLONG bag_data_type = inStream->GetLong();

		// Data version
		sWORD major_version = inStream->GetWord();
		sWORD minor_version = inStream->GetWord();

		if (bag_data_type != BAG_DATA_TYPE_SIGNATURE)
		{
			err = VE_STREAM_BAD_SIGNATURE;
		}
		else
		{
			if (major_version == 1)
			{
				err = ReadFromStreamMinimal( inStream);
			}
			else
			{
				err = VE_STREAM_BAD_VERSION;
			}
		}
	}

	return err;
}


// static
const VValueBag::StKey& VValueBag::CDataAttributeName()
{
	static const StKey	sName( "<>");
	return sName;
}


CharSet VValueBag::DebugDump(char* inTextBuffer, sLONG& inBufferSize) const
{
	if (VProcess::Get() != NULL)
	{
		VString dump;
		VString temp;
		VIndex i;
		VIndex count = GetAttributesCount();
		for (i = 1 ; i <= count ; ++i)
		{
			const VValueSingle* theValue = GetNthAttribute( i, &temp);
			dump.AppendPrintf("%S = %A\n", &temp, theValue);
		}
		
		count = GetElementNamesCount();
		for (i = 1 ; i <= count ; ++i)
		{
			const VBagArray *theBagArray = GetNthElementName( i, &temp);

			dump.AppendPrintf("=======\n%d %S :\n", theBagArray->GetCount(), &temp);
			
			for (sLONG j = 1 ; j <= theBagArray->GetCount() ; ++j) {
				dump.AppendPrintf("--\n%V", theBagArray->RetainNth(j));
			}
		}

		dump.Truncate(inBufferSize/2);
		inBufferSize = (sLONG) dump.ToBlock(inTextBuffer, inBufferSize, VTC_UTF_16, false, false);
	} else
		inBufferSize = 0;
	return VTC_UTF_16;
}



typedef std::vector< std::pair< VIndex, VIndex> >	VectorOfPositions;

inline	bool PositionComparatorStrict( const VectorOfPositions::value_type& inValue1, const VectorOfPositions::value_type& inValue2)
	{ return inValue1.second < inValue2.second;}


void VValueBag::_SortElements( const StBagElementsOrdering *inElementRule, const StBagElementsOrdering inRules[])
{
	VIndex count = fElements->GetCount();
	if (count > 1)
	{
		bool need_sort = false;
		VIndex currentPosition = 0;
		VectorOfPositions positions;
		for( VIndex i = 1 ; i <= count ; ++i)
		{
			VIndex position = fElements->GetNthKeyPosition( i, inElementRule->fChildren, MAX_BAG_ELEMENTS_ORDERING);
			positions.push_back( VectorOfPositions::value_type( i, position));

			if ( (position != 0) && (position < currentPosition) )
			{
				need_sort = true;
			}
			currentPosition = position;
		}
		
		if (need_sort)
		{
			std::sort( positions.begin(), positions.end(), PositionComparatorStrict);
			VPackedVBagArrayDictionary *newElements = new VPackedVBagArrayDictionary;
			for( VectorOfPositions::const_iterator i = positions.begin() ; i != positions.end() ; ++i)
			{
				VBagArray *elements;
				VIndex position = i->second;
				if (position > 0)
				{
					elements = fElements->GetNthValue( i->first, NULL);
					newElements->Append( *inElementRule->fChildren[position-1], elements);
				}
				else
				{
					VString name;
					elements = fElements->GetNthValue( i->first, &name);
					newElements->Append( name, elements);
				}
			}
			delete fElements;
			fElements = newElements;
		}
	}
	
	// sort children
	for( VIndex i = 1 ; i <= count ; ++i)
	{
		VString name;
		VBagArray *elements = fElements->GetNthValue( i, &name);
		StKey keyName( name);
		VIndex bagArrayCount = elements->GetCount();
		for( VIndex j = 1 ; j <= bagArrayCount ; ++j)
		{
			VValueBag *elem = elements->GetNth( j);
			elem->SortElements( keyName, inRules);
		}
	}
}


void VValueBag::SortElements( const StKey& inElementName, const StBagElementsOrdering inRules[])
{
	if ( (inRules == NULL) || (GetElementNamesCount() <= 0) )
		return;

	// look for base rule.
	// assumes rules uses utf-8 just like the StKey.
	
	for( const StBagElementsOrdering *i = &inRules[0] ; i->fName != NULL ; ++i)
	{
		if (i->fName->Equal( inElementName) )
		{
			_SortElements( i, inRules);
			break;
		}
	}
}


void AppendJSONNewline(VString& outJSONString, bool prettyformat)
{
	if (prettyformat)
		outJSONString.AppendUniChar(10);
}


void AjustJSONTab(VString& outJSONString, sLONG& curlevel, bool prettyformat)
{
	if (prettyformat)
		for (sLONG i = 0; i < curlevel; i++)
			outJSONString.AppendUniChar(9);
}

void AppendBeginJSONObject(VString& outJSONString, sLONG& curlevel, bool prettyformat)
{
	AppendJSONNewline(outJSONString, prettyformat);
	AjustJSONTab(outJSONString, curlevel, prettyformat);
	outJSONString.AppendUniChar('{');
	curlevel++;
	//AppendJSONNewline(outJSONString, prettyformat);
}


void AppendBeginJSONArray(VString& outJSONString, sLONG& curlevel, bool prettyformat)
{
	AppendJSONNewline(outJSONString, prettyformat);
	AjustJSONTab(outJSONString, curlevel, prettyformat);
	outJSONString.AppendUniChar('[');
	curlevel++;
	//AppendJSONNewline(outJSONString, prettyformat);
}


void AppendEndJSONObject(VString& outJSONString, sLONG& curlevel, bool prettyformat)
{
	AppendJSONNewline(outJSONString, prettyformat);
	curlevel--;
	AjustJSONTab(outJSONString, curlevel, prettyformat);
	outJSONString.AppendUniChar('}');
}

void AppendEndJSONArray(VString& outJSONString, sLONG& curlevel, bool prettyformat)
{
	AppendJSONNewline(outJSONString, prettyformat);
	curlevel--;
	AjustJSONTab(outJSONString, curlevel, prettyformat);
	outJSONString.AppendUniChar(']');
}


void AppendJSONPropertyName(VString& outJSONString, sLONG& curlevel, bool prettyformat, const VString name)
{
	VString name2;
	AppendJSONNewline(outJSONString, prettyformat);
	AjustJSONTab(outJSONString, curlevel, prettyformat);
	name.GetJSONString(name2, JSON_WithQuotesIfNecessary);
	outJSONString += name2;
	if (prettyformat)
		outJSONString += L" : ";
	else
		outJSONString.AppendUniChar(':');
}

VError VValueBag::FromJSONString(const VString& inJSONString, JSONOption inModifier)
{
	VJSONImporter	jsonImport(inJSONString);
	
	return jsonImport.JSONObjectToBag(*this);
}


VError VValueBag::_GetJSONString(VString& outJSONString, sLONG& curlevel, bool prettyformat, JSONOption inModifier) const
{
	AppendBeginJSONObject(outJSONString, curlevel, prettyformat);

	VIndex nbatt = GetAttributesCount();
	VIndex nbelem = GetElementNamesCount();
	bool first = true;
	for (VIndex i = 1; i <= nbatt; i++)
	{
		VString s;
		const VValueSingle* val = GetNthAttribute(i, &s);
		if ((s != L"____objectunic") && (s != L"____property_name_in_jsarray"))
		{
			if (first)
				first = false;
			else
				outJSONString.AppendUniChar(',');
			if(s=="<>")
				s = "__CDATA";
			AppendJSONPropertyName(outJSONString, curlevel, prettyformat, s);
			if (val == NULL || val->IsNull())
				outJSONString += L"null";
			else
			{
				val->GetJSONString(s, JSON_WithQuotesIfNecessary);
				outJSONString += s;
			}
		}
	}

	for (VIndex i = 1; i <= nbelem; i++)
	{
		if (first)
			first = false;
		else
			outJSONString.AppendUniChar(',');

		VString s;
		const VBagArray* subelems = GetNthElementName(i, &s);
		AppendJSONPropertyName(outJSONString, curlevel, prettyformat, s);
		
		if ((inModifier & JSON_UniqueSubElementsAreNotArrays) != 0 && subelems->GetCount() == 1)
		{
			subelems->GetNth(1)->_GetJSONString(outJSONString, curlevel, prettyformat, inModifier);
		}
		else if (subelems->GetNth(1)->GetAttribute("____objectunic"))
		{
			subelems->GetNth(1)->_GetJSONString(outJSONString, curlevel, prettyformat, inModifier);
		}
		else
			subelems->_GetJSONString(outJSONString, curlevel, prettyformat, inModifier);

	}

	AppendEndJSONObject(outJSONString, curlevel, prettyformat);
	return VE_OK;
}


VError VValueBag::GetJSONString(VString& outJSONString, JSONOption inModifier) const
{
	outJSONString.Clear();
	sLONG curlevel = 0;
	return _GetJSONString(outJSONString, curlevel, (inModifier & JSON_PrettyFormatting) != 0, inModifier);
}


VJSONObject* VValueBag::BuildJSONObject(VError& outError) const
{
    // will use stringify until redone in a more elegant way
	VJSONObject* result = nil;
	VString jsonstr;
	outError = GetJSONString(jsonstr);
	if (outError == VE_OK)
	{
		VJSONValue val;
		outError = val.ParseFromString(jsonstr);
		if (val.IsObject() && outError == VE_OK)
			result = RetainRefCountable(val.GetObject());
	}
	return result;
}



//================================================================================================================


VError IBaggable::LoadFromBagWithLoader(const VValueBag& inBag, VBagLoader* /*inLoader*/, void* /*inContext*/)
{
	return LoadFromBag(inBag);
}


VError IBaggable::LoadFromBag(const VValueBag& /*inBag*/)
{
	return VE_OK;
}


//================================================================================================================

namespace BagLoaderKeys
{
	CREATE_BAGKEY_WITH_DEFAULT( uuid, XBOX::VUUID, XBOX::VUUID::sNullUUID);
}

VBagLoader::VBagLoader( bool inRegenerateUUIDs, bool inFindReferencesByName)
{
	fRegenerateUUIDs = inRegenerateUUIDs;
	fFindReferencesByName = inFindReferencesByName;
	fStopOnError = true;
	fWithNamesCheck = true;
	fLoadOnly = false;
}


VBagLoader::~VBagLoader()
{
}


bool VBagLoader::GetUUID( const VValueBag& inBag, VUUID& outUUID)
{
	bool ok;
	if (fRegenerateUUIDs)
	{
		VUUID bag_uuid;
		ok = inBag.GetVUUID( BagLoaderKeys::uuid, bag_uuid);
		if (ok && !bag_uuid.IsNull())
		{
			try
			{
				MapVUUID::const_iterator i = fUUIDs.find( bag_uuid);
				if (i != fUUIDs.end())
				{
					outUUID = i->second;
				}
				else
				{
					outUUID.Regenerate();
					fUUIDs[bag_uuid] = outUUID;
				}
			}
			catch(...)
			{
				ok = false;
			}
		}
		else
		{
			outUUID.Regenerate();
		}
	}
	else
	{
		ok = inBag.GetVUUID( BagLoaderKeys::uuid, outUUID);
		if (!ok)
		{
			outUUID.Regenerate();
		}
	}
	
	return ok;
}


bool VBagLoader::ResolveUUID( VUUID& ioUUID) const
{
	bool ok;
	if (fRegenerateUUIDs)
	{
		MapVUUID::const_iterator i = fUUIDs.find( ioUUID);
		if (testAssert( i != fUUIDs.end()))
		{
			ioUUID = i->second;
			ok = true;
		}
		else
		{
			ok = false;
		}
	}
	else
	{
		ok = true;
	}
	return ok;
}


//================================================================================================================


VBagArray::~VBagArray()
{
	Destroy();
}


VBagArray::VBagArray( const VBagArray& inOther)
{
	try
	{
		fArray.reserve( inOther.fArray.size());
		for( bags_vector::const_iterator i = inOther.fArray.begin() ; i != inOther.fArray.end() ; ++i)
		{
			VValueBag *bag = new VValueBag( **i);
			if (bag)
				fArray.push_back( bag);
			else
				break;
		}
	}
	catch(...)
	{
	}
}


bool VBagArray::AddNth( VValueBag *inBag, VIndex inIndex)
{
	bool ok;
	if (testAssert(inBag != NULL))
	{
		if (inIndex <= 0)
			inIndex = 1;
		else if (inIndex > GetCount() + 1)
			inIndex = GetCount() + 1;
		
		try
		{
			fArray.insert( fArray.begin() + inIndex - 1, inBag);
			inBag->Retain();
			ok = true;
		}
		catch(...)
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


bool VBagArray::AddTail( VValueBag* inBag)
{
	bool ok;
	if (testAssert( inBag != NULL))
	{
		try
		{
			fArray.push_back( inBag);
			inBag->Retain();
			ok = true;
		}
		catch(...)
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


VValueBag* VBagArray::RetainNth( VIndex inIndex)
{
	VValueBag* bag = NULL;
	
	if (testAssert(inIndex >= 1 && inIndex <= GetCount()))
		bag = fArray[inIndex-1];
	
	if (bag == NULL)
		bag = new VValueBag;
	else	
		bag->Retain();

	return bag;
}


void VBagArray::SetNth( VValueBag *inBag, VIndex inIndex)
{
	if (testAssert(inIndex >= 1 && inIndex <= GetCount() && inBag != NULL))
	{
		bags_vector::iterator i = fArray.begin() + inIndex - 1;
		CopyRefCountable( &*i, inBag);
	}
}


void VBagArray::DeleteNth( VIndex inIndex)
{
	if (testAssert(inIndex >= 1 && inIndex <= GetCount()))
	{
		bags_vector::iterator i = fArray.begin() + inIndex - 1;
		(*i)->Release();
		fArray.erase( i);
	}
}


void VBagArray::Destroy()
{
	for( bags_vector::iterator i = fArray.begin() ; i != fArray.end() ; ++i)
		(*i)->Release();
		
	fArray.clear();
}


void VBagArray::Delete( VValueBag* inBag)
{
	bags_vector::iterator found = std::find( fArray.begin(), fArray.end(), inBag);
	if (found != fArray.end())
		fArray.erase(found);
}


VIndex VBagArray::Find( VValueBag* inBag) const
{
	bags_vector::const_iterator found = std::find( fArray.begin(), fArray.end(), inBag);
	if (found != fArray.end())
		return (VIndex) (found - fArray.begin() + 1);
	else
		return 0;
}


VError VBagArray::WriteToStreamMinimal( VStream* inStream) const
{
	VIndex max_bag = GetCount();

	inStream->PutLong( max_bag);
	for( bags_vector::const_iterator i = fArray.begin() ; i != fArray.end() ; ++i)
	{
		if ((*i)->WriteToStreamMinimal( inStream, true) != VE_OK)
			break;
	}
	return inStream->GetLastError();
}


VError VBagArray::ReadFromStreamMinimal( VStream* inStream)
{
	VError err = inStream->GetLastError();
	if (err == VE_OK)
	{
		try
		{
			VIndex max_bag = inStream->GetLong();
			fArray.reserve( max_bag);
			for( VIndex i = 1 ; (i <= max_bag) && (err == VE_OK) ; i++)
			{
				VValueBag* bag = new VValueBag;
				if (bag != NULL)
				{
					err = bag->ReadFromStreamMinimal( inStream);
					if (err == VE_OK)
					{
						fArray.push_back( bag);
					}
					else
					{
						bag->Release();
					}
				}
				else
				{
					err = VE_MEMORY_FULL;
				}
			}
		}
		catch(...)
		{
			err = VE_MEMORY_FULL;
		}
	}
	
	return err;
}


VError VBagArray::_GetJSONString(VString& outJSONString, sLONG& curlevel, bool prettyformat, JSONOption inModifier) const
{
	AppendBeginJSONArray(outJSONString, curlevel, prettyformat);

	VIndex nbelem = GetCount();
	for (VIndex i = 1; i <= nbelem; i++)
	{
		GetNth(i)->_GetJSONString(outJSONString, curlevel, prettyformat, inModifier);
		if (i != nbelem)
			outJSONString.AppendUniChar(',');
	}

	AppendEndJSONArray(outJSONString, curlevel, prettyformat);
	return VE_OK;
}



//---------------------------------------------------



END_TOOLBOX_NAMESPACE

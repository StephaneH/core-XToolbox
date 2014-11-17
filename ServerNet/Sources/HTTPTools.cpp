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

#include "VServerNetPrecompiled.h"
#include "HTTPTools.h"


BEGIN_TOOLBOX_NAMESPACE
USING_TOOLBOX_NAMESPACE


//--------------------------------------------------------------------------------------------------


#define LOWERCASE(c) \
	if ((c >= CHAR_LATIN_CAPITAL_LETTER_A) && (c <= CHAR_LATIN_CAPITAL_LETTER_Z))\
	c += 0x0020


template <typename T1, typename T2>
inline bool _EqualASCIICString (const T1 *const inCString1, const sLONG inString1Len, const T2 *const inCString2, const sLONG inString2Len, bool isCaseSensitive)
{
	if (!inCString1 || !inCString2 || (inString1Len != inString2Len))
		return false;
	else if (!(*inCString1) && !(*inCString2) && (inString1Len == 0))
		return true;

	bool result = false;
	const T1 *p1 = inCString1;
	const T2 *p2 = inCString2;

	while (*p1)
	{
		if ((NULL != p1) && (NULL != p2))
		{
			T1 c1 = *p1;
			T2 c2 = *p2;

			if (!isCaseSensitive)
			{
				LOWERCASE (c1);
				LOWERCASE (c2);
			}

			if (c1 == c2)
				result = true;
			else
				return false;
			p1++;
			p2++;
		}
		else
		{
			return false;
		}
	}

	return result;
}


template <typename T1, typename T2>
inline sLONG _FindASCIICString (const T1 *inText, const sLONG inTextLen, const T2 *inPattern, const sLONG inPatternLen, bool isCaseSensitive)
{
	// see http://fr.wikipedia.org/wiki/Algorithme_de_Knuth-Morris-Pratt

	if (inPatternLen > inTextLen)
		return 0;

	sLONG	textSize = inTextLen;
	sLONG	patternSize = inPatternLen;
	sLONG *	target = (sLONG *)malloc (sizeof(sLONG) * (patternSize + 1));

	if (NULL != target)
	{
		sLONG	m = 0;
		sLONG	i = 0;
		sLONG	j = -1;
		T2		c = '\0';

		target[0] = j;
		while (i < patternSize)
		{
			T2 pchar = inPattern[i];

			if (!isCaseSensitive)
				LOWERCASE (pchar);

			if (pchar == c)
			{
				target[i + 1] = j + 1;
				++j;
				++i;
			}
			else if (j > 0)
			{
				j = target[j];
			}
			else
			{
				target[i + 1] = 0;
				++i;
				j = 0;
			}

			pchar = inPattern[j];
			if (!isCaseSensitive)
				LOWERCASE (pchar);

			c = pchar;
		}

		m = 0;
		i = 0;
		while (((m + i) < textSize) && (i < patternSize))
		{
			T1		tchar = inText[m + i];
			T2		pchar = inPattern[i];

			if (!isCaseSensitive)
			{
				LOWERCASE (tchar);
				LOWERCASE (pchar);
			}

			if (tchar == (T1)pchar)
			{
				++i;
			}
			else
			{
				m += i - target[i];
				if (i > 0)
					i = target[i];
			}
		}

		free (target);

		if (i >= patternSize)
			return m + 1; // 1-based position !!! Just to work as VString.Find()
	}

	return 0;
}


namespace HTTPTools {

	const XBOX::VString STRING_EMPTY						= CVSTR ("");
	const XBOX::VString STRING_STAR							= CVSTR ("*");


		/**
		*	Some common HTTP Request headers
		*/
	const XBOX::VString STRING_HEADER_ACCEPT				= CVSTR ("Accept");
	const XBOX::VString STRING_HEADER_ACCEPT_CHARSET		= CVSTR ("Accept-Charset");
	const XBOX::VString STRING_HEADER_ACCEPT_ENCODING		= CVSTR ("Accept-Encoding");
	const XBOX::VString STRING_HEADER_ACCEPT_LANGUAGE		= CVSTR ("Accept-Language");
	const XBOX::VString STRING_HEADER_AUTHORIZATION			= CVSTR ("Authorization");
	const XBOX::VString STRING_HEADER_CONTENT_DISPOSITION	= CVSTR ("Content-Disposition");
	const XBOX::VString STRING_HEADER_COOKIE				= CVSTR ("Cookie");
	const XBOX::VString STRING_HEADER_EXPECT				= CVSTR ("Expect");
	const XBOX::VString STRING_HEADER_FROM					= CVSTR ("From");
	const XBOX::VString STRING_HEADER_HOST					= CVSTR ("Host");
	const XBOX::VString STRING_HEADER_IF_MATCH				= CVSTR ("If-Match");
	const XBOX::VString STRING_HEADER_IF_MODIFIED_SINCE		= CVSTR ("If-Modified-Since");
	const XBOX::VString STRING_HEADER_IF_NONE_MATCH			= CVSTR ("If-None-Match");
	const XBOX::VString STRING_HEADER_IF_RANGE				= CVSTR ("If-Range");
	const XBOX::VString STRING_HEADER_IF_UNMODIFIED_SINCE	= CVSTR ("If-Unmodified-Since");
	const XBOX::VString STRING_HEADER_KEEP_ALIVE			= CVSTR ("Keep-Alive");
	const XBOX::VString STRING_HEADER_MAX_FORWARDS			= CVSTR ("Max-Forwards");
	const XBOX::VString STRING_HEADER_PROXY_AUTHORIZATION	= CVSTR ("Proxy-Authorization");
	const XBOX::VString STRING_HEADER_RANGE					= CVSTR ("Range");
	const XBOX::VString STRING_HEADER_REFERER				= CVSTR ("Referer");
	const XBOX::VString STRING_HEADER_TE					= CVSTR ("TE");							// RFC 2616 - Section 14.39
	const XBOX::VString STRING_HEADER_TRANSFER_ENCODING		= CVSTR	("Transfer-Encoding");
	const XBOX::VString STRING_HEADER_USER_AGENT			= CVSTR ("User-Agent");
	const XBOX::VString	STRING_HEADER_ORIGIN				= CVSTR ("Origin");

		/**
		*	Some common HTTP Response headers
		*/
	const XBOX::VString STRING_HEADER_ACCEPT_RANGES			= CVSTR ("Accept-Ranges");
	const XBOX::VString STRING_HEADER_AGE					= CVSTR ("Age");
	const XBOX::VString STRING_HEADER_ALLOW					= CVSTR ("Allow");
	const XBOX::VString STRING_HEADER_CACHE_CONTROL			= CVSTR ("Cache-Control");
	const XBOX::VString STRING_HEADER_CONNECTION			= CVSTR ("Connection");
	const XBOX::VString STRING_HEADER_DATE					= CVSTR ("Date");
	const XBOX::VString STRING_HEADER_ETAG					= CVSTR ("ETag");
	const XBOX::VString STRING_HEADER_CONTENT_ENCODING		= CVSTR ("Content-Encoding");
	const XBOX::VString STRING_HEADER_CONTENT_LANGUAGE		= CVSTR ("Content-Language");
	const XBOX::VString STRING_HEADER_CONTENT_LENGTH		= CVSTR ("Content-Length");
	const XBOX::VString STRING_HEADER_CONTENT_LOCATION		= CVSTR ("Content-Location");
	const XBOX::VString STRING_HEADER_CONTENT_MD5			= CVSTR ("Content-MD5");
	const XBOX::VString STRING_HEADER_CONTENT_RANGE			= CVSTR ("Content-Range");
	const XBOX::VString STRING_HEADER_CONTENT_TYPE			= CVSTR ("Content-Type");
	const XBOX::VString STRING_HEADER_EXPIRES				= CVSTR ("Expires");
	const XBOX::VString STRING_HEADER_LAST_MODIFIED			= CVSTR ("Last-Modified");
	const XBOX::VString STRING_HEADER_LOCATION				= CVSTR ("Location");
	const XBOX::VString STRING_HEADER_PRAGMA				= CVSTR ("Pragma");
	const XBOX::VString STRING_HEADER_PROXY_AUTHENTICATE	= CVSTR ("Proxy-Authenticate");
	const XBOX::VString STRING_HEADER_RETRY_AFTER			= CVSTR ("Retry-After");
	const XBOX::VString STRING_HEADER_SERVER				= CVSTR ("Server");
	const XBOX::VString STRING_HEADER_SET_COOKIE			= CVSTR ("Set-Cookie");
	const XBOX::VString STRING_HEADER_STATUS				= CVSTR ("Status");
	const XBOX::VString STRING_HEADER_VARY					= CVSTR ("Vary");
	const XBOX::VString STRING_HEADER_WWW_AUTHENTICATE		= CVSTR ("WWW-Authenticate");
	const XBOX::VString STRING_HEADER_X_STATUS				= CVSTR ("X-Status");
	const XBOX::VString STRING_HEADER_X_POWERED_BY			= CVSTR ("X-Powered-By");
	const XBOX::VString STRING_HEADER_X_VERSION				= CVSTR ("X-Version");

		/**
		*	Some common Strings used for MIME Messages
		*/
	const XBOX::VString CONST_MIME_PART_NAME					= CVSTR ("name");
	const XBOX::VString CONST_MIME_PART_FILENAME				= CVSTR ("filename");
	const XBOX::VString CONST_MIME_MESSAGE_ENCODING_URL			= CVSTR ("application/x-www-form-urlencoded");
	const XBOX::VString CONST_MIME_MESSAGE_ENCODING_MULTIPART	= CVSTR ("multipart/form-data");
	const XBOX::VString CONST_MIME_MESSAGE_BOUNDARY				= CVSTR ("boundary");
	const XBOX::VString CONST_MIME_MESSAGE_BOUNDARY_DELIMITER	= CVSTR ("--");
	const XBOX::VString CONST_MIME_MESSAGE_DEFAULT_CONTENT_TYPE	= CVSTR ("multipart/mixed");

	const XBOX::VString CONST_TEXT_PLAIN						= CVSTR ("text/plain");
	const XBOX::VString CONST_TEXT_PLAIN_UTF_8					= CVSTR ("text/plain; charset=utf-8");


	bool EqualASCIIVString (const XBOX::VString& inString1, const XBOX::VString& inString2, bool isCaseSensitive)
	{
		return _EqualASCIICString (inString1.GetCPointer(), inString1.GetLength(), inString2.GetCPointer(), inString2.GetLength(), isCaseSensitive);
	}


	bool EqualASCIICString (const XBOX::VString& inString1, const char *const inCString2, bool isCaseSensitive)
	{
		return _EqualASCIICString (inString1.GetCPointer(), inString1.GetLength(), inCString2, (sLONG)strlen (inCString2), isCaseSensitive);
	}


	sLONG FindASCIIVString (const XBOX::VString& inText, const XBOX::VString& inPattern, bool isCaseSensitive)
	{
		return _FindASCIICString (inText.GetCPointer(), inText.GetLength(), inPattern.GetCPointer(), inPattern.GetLength(), isCaseSensitive);
	}


	sLONG FindASCIICString (const XBOX::VString& inText, const char *inPattern, bool isCaseSensitive)
	{
		return _FindASCIICString (inText.GetCPointer(), inText.GetLength(), inPattern, (sLONG)strlen (inPattern), isCaseSensitive);
	}


	bool BeginsWithASCIIVString (const XBOX::VString& inText, const XBOX::VString& inPattern, bool isCaseSensitive)
	{
		return (_FindASCIICString (inText.GetCPointer(), inText.GetLength(), inPattern.GetCPointer(), inPattern.GetLength(), isCaseSensitive) == 1);
	}


	bool BeginsWithASCIICString (const XBOX::VString& inText, const char *inPattern, bool isCaseSensitive)
	{
		return (_FindASCIICString (inText.GetCPointer(), inText.GetLength(), inPattern, (sLONG)strlen (inPattern), isCaseSensitive) == 1);
	}


	bool EndsWithASCIIVString (const XBOX::VString& inText, const XBOX::VString& inPattern, bool isCaseSensitive)
	{
		sLONG textSize = inText.GetLength();
		sLONG patternSize = inPattern.GetLength();

		return (_FindASCIICString (inText.GetCPointer() + (textSize - patternSize), patternSize, inPattern.GetCPointer(), patternSize, isCaseSensitive) == 1);
	}


	bool EndsWithASCIICString (const XBOX::VString& inText, const char *inPattern, bool isCaseSensitive)
	{
		sLONG textSize = inText.GetLength();
		sLONG patternSize = (sLONG)strlen (inPattern);

		return (_FindASCIICString (inText.GetCPointer() + (textSize - patternSize), patternSize, inPattern, patternSize, isCaseSensitive) == 1);
	}


	void TrimUniChar (XBOX::VString& ioString, const UniChar inCharToTrim)
	{
		if (ioString.GetLength() > 0)
		{
			sLONG			length = ioString.GetLength();
			UniChar *		data = (UniChar *)ioString.GetCPointer();
			XBOX::VIndex	leadingChars = 0;
			XBOX::VIndex	endingChars = 0;

			for (UniChar *p = data, *end = (data + length); (p != end) && (*p == inCharToTrim); p++, leadingChars++);
			for (UniChar *p = (data + length - 1), *start = (data - 1); (p != start) && (*p == inCharToTrim); p--, endingChars++);

			if ((0 != leadingChars) || (0 != endingChars))
			{
				if ((leadingChars + endingChars) >= length)
				{
					ioString.Clear();
				}
				else
				{
					ioString.SubString (leadingChars + 1, length - leadingChars - endingChars);
				}
			}
		}
	}


	sLONG GetLongFromString (const XBOX::VString& inString)		
	{
		bool	isneg = false;		

		XBOX::VIndex sepPos = FindASCIIVString (inString, XBOX::VIntlMgr::GetDefaultMgr()->GetDecimalSeparator());
		if (sepPos <= 0)
			sepPos = inString.GetLength();

		const UniChar* bb = inString.GetCPointer();
		sLONG result = 0;
		for (XBOX::VIndex i = 0; i < sepPos; ++i)
		{
			if ((0 == result) && (bb[i] == CHAR_HYPHEN_MINUS))
				isneg = true;
			if ((bb[i] < CHAR_DIGIT_ZERO) || (bb[i] > CHAR_DIGIT_NINE))
				continue;
			result *= 10;
			result += bb[i] - CHAR_DIGIT_ZERO;
		}

		if (isneg)
			result = -result;

		return result;
	}


	const XBOX::VString& GetHTTPHeaderName (const HTTPCommonHeaderCode inHeaderCode)
	{
		switch (inHeaderCode)
		{
		case HEADER_ACCEPT:						return STRING_HEADER_ACCEPT;
		case HEADER_ACCEPT_CHARSET:				return STRING_HEADER_ACCEPT_CHARSET;
		case HEADER_ACCEPT_ENCODING:			return STRING_HEADER_ACCEPT_ENCODING;
		case HEADER_ACCEPT_LANGUAGE:			return STRING_HEADER_ACCEPT_LANGUAGE;
		case HEADER_AUTHORIZATION:				return STRING_HEADER_AUTHORIZATION;
		case HEADER_COOKIE:						return STRING_HEADER_COOKIE;
		case HEADER_EXPECT:						return STRING_HEADER_EXPECT;
		case HEADER_FROM:						return STRING_HEADER_FROM;
		case HEADER_HOST:						return STRING_HEADER_HOST;
		case HEADER_IF_MATCH:					return STRING_HEADER_IF_MATCH;
		case HEADER_IF_MODIFIED_SINCE:			return STRING_HEADER_IF_MODIFIED_SINCE;
		case HEADER_IF_NONE_MATCH:				return STRING_HEADER_IF_NONE_MATCH;
		case HEADER_IF_RANGE:					return STRING_HEADER_IF_RANGE;
		case HEADER_IF_UNMODIFIED_SINCE:		return STRING_HEADER_IF_UNMODIFIED_SINCE;
		case HEADER_KEEP_ALIVE:					return STRING_HEADER_KEEP_ALIVE;
		case HEADER_MAX_FORWARDS:				return STRING_HEADER_MAX_FORWARDS;
		case HEADER_PROXY_AUTHORIZATION:		return STRING_HEADER_PROXY_AUTHORIZATION;
		case HEADER_RANGE:						return STRING_HEADER_RANGE;
		case HEADER_REFERER:					return STRING_HEADER_REFERER;
		case HEADER_TE:							return STRING_HEADER_TE;							// RFC 2616 - Section 14.39
		case HEADER_USER_AGENT:					return STRING_HEADER_USER_AGENT;
		case HEADER_ORIGIN:						return STRING_HEADER_ORIGIN;

		case HEADER_ACCEPT_RANGES:				return STRING_HEADER_ACCEPT_RANGES;
		case HEADER_AGE:						return STRING_HEADER_AGE;
		case HEADER_ALLOW:						return STRING_HEADER_ALLOW;
		case HEADER_CACHE_CONTROL:				return STRING_HEADER_CACHE_CONTROL;
		case HEADER_CONNECTION:					return STRING_HEADER_CONNECTION;
		case HEADER_DATE:						return STRING_HEADER_DATE;
		case HEADER_ETAG:						return STRING_HEADER_ETAG;
		case HEADER_CONTENT_ENCODING:			return STRING_HEADER_CONTENT_ENCODING;
		case HEADER_CONTENT_LANGUAGE:			return STRING_HEADER_CONTENT_LANGUAGE;
		case HEADER_CONTENT_LENGTH:				return STRING_HEADER_CONTENT_LENGTH;
		case HEADER_CONTENT_LOCATION:			return STRING_HEADER_CONTENT_LOCATION;
		case HEADER_CONTENT_MD5:				return STRING_HEADER_CONTENT_MD5;
		case HEADER_CONTENT_RANGE:				return STRING_HEADER_CONTENT_RANGE;
		case HEADER_CONTENT_TYPE:				return STRING_HEADER_CONTENT_TYPE;
		case HEADER_EXPIRES:					return STRING_HEADER_EXPIRES;
		case HEADER_LAST_MODIFIED:				return STRING_HEADER_LAST_MODIFIED;
		case HEADER_LOCATION:					return STRING_HEADER_LOCATION;
		case HEADER_PRAGMA:						return STRING_HEADER_PRAGMA;
		case HEADER_PROXY_AUTHENTICATE:			return STRING_HEADER_PROXY_AUTHENTICATE;
		case HEADER_RETRY_AFTER:				return STRING_HEADER_RETRY_AFTER;
		case HEADER_SERVER:						return STRING_HEADER_SERVER;
		case HEADER_SET_COOKIE:					return STRING_HEADER_SET_COOKIE;
		case HEADER_STATUS:						return STRING_HEADER_STATUS;
		case HEADER_VARY:						return STRING_HEADER_VARY;
		case HEADER_WWW_AUTHENTICATE:			return STRING_HEADER_WWW_AUTHENTICATE;
		case HEADER_X_STATUS:					return STRING_HEADER_X_STATUS;
		case HEADER_X_POWERED_BY:				return STRING_HEADER_X_POWERED_BY;
		case HEADER_X_VERSION:					return STRING_HEADER_X_VERSION;
		case HEADER_UPGRADE:					break;
		}

		return STRING_EMPTY;
	}


	void ExtractContentTypeAndCharset (const XBOX::VString& inString, XBOX::VString& outContentType, XBOX::CharSet& outCharSet)
	{
		outContentType.FromString (inString);
		outCharSet = XBOX::VTC_UNKNOWN;

		if (!outContentType.IsEmpty())
		{
			sLONG pos = FindASCIICString (outContentType, "charset=");
			if (pos > 0)
			{
				XBOX::VString charSetName;
				charSetName.FromBlock (outContentType.GetCPointer() + pos + 7, (outContentType.GetLength() - (pos + 7)) * sizeof(UniChar), XBOX::VTC_UTF_16);
				if (!charSetName.IsEmpty())
				{
					outCharSet = XBOX::VTextConverters::Get()->GetCharSetFromName (charSetName);
				}

				outContentType.SubString (1, outContentType.FindUniChar (CHAR_SEMICOLON) - 1);
				TrimUniChar (outContentType, CHAR_SPACE);
			}
		}
	}


	bool ExtractFieldNameValue (const XBOX::VString& inString, XBOX::VString& outName, XBOX::VString& outValue)
	{
		bool					isOK = false;
		XBOX::VectorOfVString	nameValueStrings;

		outName.Clear();
		outValue.Clear();

		if (inString.GetSubStrings (CHAR_EQUALS_SIGN, nameValueStrings, true, true))
		{
			if (!nameValueStrings.empty())
				outName = nameValueStrings.at (0);

			if (nameValueStrings.size() > 1)
			{
				outValue = nameValueStrings.at (1);

				// Clean-up value String
				TrimUniChar (outValue, CHAR_QUOTATION_MARK);
				TrimUniChar (outValue, CHAR_SPACE);

				isOK = true;
			}
		}

		return isOK;
	}


	MimeTypeKind GetMimeTypeKind (const XBOX::VString& inContentType)
	{
		// Test "image" first because of "image/svg+xml"...

		if (FindASCIICString (inContentType, "image/") == 1)
		{
			return MIMETYPE_IMAGE;
		}
		else if ((FindASCIICString (inContentType, "text/") == 1) ||
			(FindASCIICString (inContentType, "/json") > 0) ||
			(FindASCIICString (inContentType, "javascript") > 0) ||
			(FindASCIICString (inContentType, "application/xml") == 1) ||
			(FindASCIICString (inContentType, "+xml") > 0))
		{
			return MIMETYPE_TEXT;
		}
		else
		{
			return MIMETYPE_BINARY;
		}
	}

} // namespace HTTPTools


END_TOOLBOX_NAMESPACE

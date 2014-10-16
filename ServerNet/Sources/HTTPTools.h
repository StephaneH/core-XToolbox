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

#ifndef __HTTP_TOOLS_INCLUDED__
#define __HTTP_TOOLS_INCLUDED__

BEGIN_TOOLBOX_NAMESPACE


typedef enum HTTPCommonHeaderCode
{
/*
	Some common HTTP Request headers
*/
	HEADER_ACCEPT,
	HEADER_ACCEPT_CHARSET,
	HEADER_ACCEPT_ENCODING,
	HEADER_ACCEPT_LANGUAGE,
	HEADER_AUTHORIZATION,
	HEADER_COOKIE,
	HEADER_EXPECT,
	HEADER_FROM,
	HEADER_HOST,
	HEADER_IF_MATCH,
	HEADER_IF_MODIFIED_SINCE,
	HEADER_IF_NONE_MATCH,
	HEADER_IF_RANGE,
	HEADER_IF_UNMODIFIED_SINCE,
	HEADER_KEEP_ALIVE,
	HEADER_MAX_FORWARDS,
	HEADER_PROXY_AUTHORIZATION,
	HEADER_RANGE,
	HEADER_REFERER,
	HEADER_TE,
	HEADER_UPGRADE,
	HEADER_USER_AGENT,
	HEADER_ORIGIN,
/*
	Some common HTTP Response headers
*/
	HEADER_ACCEPT_RANGES,
	HEADER_AGE,
	HEADER_ALLOW,
	HEADER_CACHE_CONTROL,
	HEADER_CONNECTION,
	HEADER_DATE,
	HEADER_ETAG,
	HEADER_CONTENT_ENCODING,
	HEADER_CONTENT_LANGUAGE,
	HEADER_CONTENT_LENGTH,
	HEADER_CONTENT_LOCATION,
	HEADER_CONTENT_MD5,
	HEADER_CONTENT_RANGE,
	HEADER_CONTENT_TYPE,
	HEADER_EXPIRES,
	HEADER_LAST_MODIFIED,
	HEADER_LOCATION,
	HEADER_PRAGMA,
	HEADER_PROXY_AUTHENTICATE,
	HEADER_RETRY_AFTER,
	HEADER_SERVER,
	HEADER_SET_COOKIE,
	HEADER_STATUS,
	HEADER_VARY,
	HEADER_WWW_AUTHENTICATE,
	HEADER_X_STATUS,
	HEADER_X_POWERED_BY,
	HEADER_X_VERSION,
} HTTPCommonHeaderCode;


typedef enum MimeTypeKind
{
	MIMETYPE_UNDEFINED = 0,
	MIMETYPE_BINARY,
	MIMETYPE_TEXT,
	MIMETYPE_IMAGE
} MimeTypeKind;


typedef enum HTTPParsingState
{
	PS_Undefined,
	PS_ReadingRequestLine,
	PS_ReadingHeaders,
	PS_ReadingBody,
	PS_ParsingFinished,
	PS_WaitingForBody		/* Used for "Expect: 100-Continue" request header */
} HTTPParsingState;


namespace HTTPTools
{
		/**
		*   @function EqualASCIIxString 
		*	@brief Fast comparision function
		*	@param isCaseSensitive set if the function should work in case sensitive mode or not
		*	@discussion function returns true when both strings are equal
		*/
bool	EqualASCIIVString (const XBOX::VString& inString1, const XBOX::VString& inString2, bool isCaseSensitive = false);
bool	EqualASCIICString (const XBOX::VString& inString1, const char *const inCString2, bool isCaseSensitive = false);

		/**
		*   @function FindASCIIxString
		*	@brief Fast Find function using the KMP alogorithm (http://fr.wikipedia.org/wiki/Algorithme_de_Knuth-Morris-Pratt)
		*	@param isCaseSensitive set if the function should work in case sensitive mode or not
		*	@discussion function returns the found pattern 1-based position in string or 0 (ZERO) if not found
		*/
sLONG	FindASCIIVString (const XBOX::VString& inText, const XBOX::VString& inPattern, bool isCaseSensitive = false);
sLONG	FindASCIICString (const XBOX::VString& inText, const char *inPattern, bool isCaseSensitive = false);

		/**
		*   @function BeginsWithASCIIxString
		*/
bool	BeginsWithASCIIVString (const XBOX::VString& inText, const XBOX::VString& inPattern, bool isCaseSensitive = false);
bool	BeginsWithASCIICString (const XBOX::VString& inText, const char *inPattern, bool isCaseSensitive = false);

		/**
		*   @function EndsWithASCIIxString
		*/
bool	EndsWithASCIIVString (const XBOX::VString& inText, const XBOX::VString& inPattern, bool isCaseSensitive = false);
bool	EndsWithASCIICString (const XBOX::VString& inText, const char *inPattern, bool isCaseSensitive = false);

		/**
		*   @function TrimUniChar
		*	@brief Works like the VString::TrimeSpaces() except we can define the char to trim.
		*/
void	TrimUniChar (XBOX::VString& ioString, const UniChar inCharToTrim);

		/**
		 *	@function FastGetLongFromString 
		 *	@brief VString::GetLong() equivalent and faster function
		 */
sLONG	GetLongFromString (const XBOX::VString& inString);

		/**
		 *	@function GetHTTPHeaderName
		 *	@brief Retrieve common header name from header code (see HTTPCommonHeaderCode HTTPServerTypes.h)
		 */
const XBOX::VString&	GetHTTPHeaderName (const HTTPCommonHeaderCode inHeaderCode);

		/**
		 *	@function ExtractContentTypeAndCharset
		 *	Retrieve Content-Type & CharSet from Content-Type header value
		 *	Example Content-Type: text/plain; charset="UTF-8"
		 */
void	ExtractContentTypeAndCharset (const XBOX::VString& inString, XBOX::VString& outContentType, XBOX::CharSet& outCharSet);

		/**
		 *	@function ExtractFieldNameValue
		 *	Extract Name and Value from an HTTP Field
		 *	Example Keep-Alive: maxCount=100, timeout=15: will extract {{maxCount;100);{timeout;15}} values pairs
		 */
bool	ExtractFieldNameValue (const XBOX::VString& inString, XBOX::VString& outName, XBOX::VString& outValue);

		/**
		 *	@function EqualFirstVStringFunctor
		 *	@brief Functor designed to be used with a multiple XBOX::VString container such as std::map <XBOX::VString, VSomethingObject> or std::multimap<XBOX::VString, VSomethingObject>
		 */

		/**
		 *	@function GetMimeTypeKind
		 *	Try to determine mime-type kind (may be MIMETYPE_BINARY, MIMETYPE_TEXT or MIMETYPE_IMAGE)
		 */
MimeTypeKind GetMimeTypeKind (const XBOX::VString& inContentType);


template <typename T>
struct EqualFirstVStringFunctor
{
	EqualFirstVStringFunctor (const XBOX::VString& inString)
		: fString (inString)
	{
	}

	bool operator() (const std::pair<XBOX::VString, T>& inPair) const
	{
		return HTTPTools::EqualASCIIVString (fString, inPair.first, false);
	}

private:
	const XBOX::VString&	fString;
};


		/**
		 *	@function FindFirstVStringFunctor
		 *	@brief Functor designed to be used with a multiple XBOX::VString container such as std::map <XBOX::VString, VSomethingObject> or std::multimap<XBOX::VString, VSomethingObject>
		 */
template <typename T>
struct FindFirstVStringFunctor
{
	FindFirstVStringFunctor (const XBOX::VString& inString)
		: fString (inString)
	{
	}

	bool operator() (const std::pair<XBOX::VString, T>& inPair) const
	{
		return (HTTPTools::FindASCIIVString (fString, inPair.first, false) > 0);
	}

private:
	const XBOX::VString&	fString;
};


		/**
		 *	@function EqualSecondVStringFunctor
		 *	@brief Functor designed to be used with a multiple XBOX::VString container such as std::map <VSomethingObject, XBOX::VString> or std::multimap<VSomethingObject, XBOX::VString>
		 */
template <typename T>
struct EqualSecondVStringFunctor
{
	EqualSecondVStringFunctor (const XBOX::VString& inString)
		: fString (inString)
	{
	}

	bool operator() (const std::pair<T, XBOX::VString>& inPair) const
	{
		return HTTPTools::EqualASCIIVString (fString, inPair.second, false);
	}

private:
	const XBOX::VString&	fString;
};


		/**
		 *	@function EqualVStringFunctor
		 *	@brief Functor designed to be used with a single XBOX::VString container such as std::vector <XBOX::VString> or std::list<XBOX::VString>
		 */
struct EqualVStringFunctor
{
	EqualVStringFunctor (const XBOX::VString& inString)
	: fString (inString)
	{
	}

	bool operator() (const XBOX::VString& inString) const
	{
		return HTTPTools::EqualASCIIVString (fString, inString, false);
	}

private:
	const XBOX::VString&	fString;
};


		/**
		 *	@function FindVStringFunctor
		 *	@brief Functor designed to be used with a single XBOX::VString container such as std::vector <XBOX::VString> or std::list<XBOX::VString>
		 */
struct FindVStringFunctor
{
	FindVStringFunctor (const XBOX::VString& inString)
	: fString (inString)
	{
	}

	bool operator() (const XBOX::VString&	inString) const
	{
		return (HTTPTools::FindASCIIVString (inString, fString, false) > 0);
	}

private:
	const XBOX::VString&	fString;
};


		/**
		 *	@function FindSecondVStringFunctor
		 *	@brief Functor designed to be used with a multiple XBOX::VString container such as std::map <VSomethingObject, XBOX::VString> or std::multimap<VSomethingObject, XBOX::VString>
		 */
template <typename T>
struct FindSecondVStringFunctor
{
	FindSecondVStringFunctor (const XBOX::VString& inString)
	: fString (inString)
	{
	}

	bool operator() (const std::pair<T, XBOX::VString>& inPair) const
	{
		return (HTTPTools::FindASCIIVString (fString, inPair.second, false) > 0);
	}

private:
	const XBOX::VString&	fString;
};


		/**
		*	Some useful constants strings
		*/
extern const XBOX::VString STRING_EMPTY;
extern const XBOX::VString STRING_STAR;

		/**
		*	Some common HTTP Request headers
		*/
extern const XBOX::VString STRING_HEADER_ACCEPT;
extern const XBOX::VString STRING_HEADER_ACCEPT_CHARSET;
extern const XBOX::VString STRING_HEADER_ACCEPT_ENCODING;
extern const XBOX::VString STRING_HEADER_ACCEPT_LANGUAGE;
extern const XBOX::VString STRING_HEADER_AUTHORIZATION;
extern const XBOX::VString STRING_HEADER_CONTENT_DISPOSITION;
extern const XBOX::VString STRING_HEADER_COOKIE;
extern const XBOX::VString STRING_HEADER_EXPECT;
extern const XBOX::VString STRING_HEADER_FROM;
extern const XBOX::VString STRING_HEADER_HOST;
extern const XBOX::VString STRING_HEADER_IF_MATCH;
extern const XBOX::VString STRING_HEADER_IF_MODIFIED_SINCE;
extern const XBOX::VString STRING_HEADER_IF_NONE_MATCH;
extern const XBOX::VString STRING_HEADER_IF_RANGE;
extern const XBOX::VString STRING_HEADER_IF_UNMODIFIED_SINCE;
extern const XBOX::VString STRING_HEADER_KEEP_ALIVE;
extern const XBOX::VString STRING_HEADER_MAX_FORWARDS;
extern const XBOX::VString STRING_HEADER_PROXY_AUTHORIZATION;
extern const XBOX::VString STRING_HEADER_RANGE;
extern const XBOX::VString STRING_HEADER_REFERER;
extern const XBOX::VString STRING_HEADER_TE;							// RFC 2616 - Section 14.39
extern const XBOX::VString STRING_HEADER_TRANSFER_ENCODING;
extern const XBOX::VString STRING_HEADER_USER_AGENT;
extern const XBOX::VString STRING_HEADER_ORIGIN;

		/**
		*	Some common HTTP Response headers
		*/
extern const XBOX::VString STRING_HEADER_ACCEPT_RANGES;
extern const XBOX::VString STRING_HEADER_AGE;
extern const XBOX::VString STRING_HEADER_ALLOW;
extern const XBOX::VString STRING_HEADER_CACHE_CONTROL;
extern const XBOX::VString STRING_HEADER_CONNECTION;
extern const XBOX::VString STRING_HEADER_DATE;
extern const XBOX::VString STRING_HEADER_ETAG;
extern const XBOX::VString STRING_HEADER_CONTENT_ENCODING;
extern const XBOX::VString STRING_HEADER_CONTENT_LANGUAGE;
extern const XBOX::VString STRING_HEADER_CONTENT_LENGTH;
extern const XBOX::VString STRING_HEADER_CONTENT_LOCATION;
extern const XBOX::VString STRING_HEADER_CONTENT_MD5;
extern const XBOX::VString STRING_HEADER_CONTENT_RANGE;
extern const XBOX::VString STRING_HEADER_CONTENT_TYPE;
extern const XBOX::VString STRING_HEADER_EXPIRES;
extern const XBOX::VString STRING_HEADER_LAST_MODIFIED;
extern const XBOX::VString STRING_HEADER_LOCATION;
extern const XBOX::VString STRING_HEADER_PRAGMA;
extern const XBOX::VString STRING_HEADER_PROXY_AUTHENTICATE;
extern const XBOX::VString STRING_HEADER_RETRY_AFTER;
extern const XBOX::VString STRING_HEADER_SERVER;
extern const XBOX::VString STRING_HEADER_SET_COOKIE;
extern const XBOX::VString STRING_HEADER_STATUS;
extern const XBOX::VString STRING_HEADER_VARY;
extern const XBOX::VString STRING_HEADER_WWW_AUTHENTICATE;
extern const XBOX::VString STRING_HEADER_X_STATUS;
extern const XBOX::VString STRING_HEADER_X_POWERED_BY;
extern const XBOX::VString STRING_HEADER_X_VERSION;
		/**
		*	Some common Strings used for MIME Messages
		*/
extern const XBOX::VString CONST_MIME_PART_NAME;
extern const XBOX::VString CONST_MIME_PART_FILENAME;
extern const XBOX::VString CONST_MIME_MESSAGE_ENCODING_URL;
extern const XBOX::VString CONST_MIME_MESSAGE_ENCODING_MULTIPART;
extern const XBOX::VString CONST_MIME_MESSAGE_BOUNDARY;
extern const XBOX::VString CONST_MIME_MESSAGE_BOUNDARY_DELIMITER;
extern const XBOX::VString CONST_MIME_MESSAGE_DEFAULT_CONTENT_TYPE;

extern const XBOX::VString CONST_TEXT_PLAIN;
extern const XBOX::VString CONST_TEXT_PLAIN_UTF_8;
};


END_TOOLBOX_NAMESPACE

#endif // __HTTP_TOOLS_INCLUDED__

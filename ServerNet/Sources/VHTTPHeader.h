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

#ifndef __HTTP_HEADER_INCLUDED__
#define __HTTP_HEADER_INCLUDED__

#include "ServerNet/Sources/HTTPTools.h"
#include "ServerNet/Sources/VNameValueCollection.h"


BEGIN_TOOLBOX_NAMESPACE


class XTOOLBOX_API VHTTPHeader : public XBOX::VObject
{
public:
										VHTTPHeader();
	virtual								~VHTTPHeader();

	const XBOX::VNameValueCollection&	GetHeaderList() const { return fHeaderList; }

	/* Header manipulation */
	bool								IsHeaderSet (const HTTPCommonHeaderCode inHeaderCode) const;
	bool								IsHeaderSet (const XBOX::VString& inName) const;

	bool								RemoveHeader (const HTTPCommonHeaderCode inHeaderCode);
	bool								RemoveHeader (const XBOX::VString& inName);

	bool								GetHeaderValue (const HTTPCommonHeaderCode inHeaderCode, XBOX::VString& outValue) const;
	bool								GetHeaderValue (const XBOX::VString& inName, XBOX::VString& outValue) const;
	bool								GetHeaderValue (const XBOX::VString& inName, sLONG& outValue) const;
	bool								GetHeaderValue (const XBOX::VString& inName, XBOX::VTime& outValue) const;

	bool								SetHeaderValue (const HTTPCommonHeaderCode inHeaderCode, const XBOX::VString& inValue, bool inOverride = true);
	bool								SetHeaderValue (const XBOX::VString& inName, const XBOX::VString& inValue, bool inOverride = true);
	bool								SetHeaderValue (const XBOX::VString& inName, const sLONG inValue, bool inOverride = true);
	bool								SetHeaderValue (const XBOX::VString& inName, const XBOX::VTime& inValue, bool inOverride = true);

	bool								GetContentType (XBOX::VString& outContentType, XBOX::CharSet *outCharSet = NULL) const;
	bool								SetContentType (const XBOX::VString& inContentType, const XBOX::CharSet& inCharSet = XBOX::VTC_UNKNOWN);
	
	bool								GetContentLength (XBOX::VSize& outValue) const;
	bool								SetContentLength (const XBOX::VSize& inValue);

	bool								GetHeaderValues (const HTTPCommonHeaderCode inHeaderCode, XBOX::VectorOfVString& outValues) const;
	
	bool								IsCookieSet (const XBOX::VString& inCookieName) const;
	bool								GetCookie (const XBOX::VString& inName, XBOX::VString& outValue) const;
	bool								SetCookie (const XBOX::VString& inName, const XBOX::VString& inValue);
	bool								DropCookie (const XBOX::VString& inName);

	bool								GetKeepAliveInfos (sLONG& outTimeout, sLONG& outMaxConnection) const;

	void								GetHeadersList (std::vector<std::pair<XBOX::VString, XBOX::VString> > &outHeadersList) const;

	bool								GetBoundary (XBOX::VString& outBoundary) const;

	void								Clear() { fHeaderList.clear(); } 

	void								ToString (XBOX::VString& outString) const;
	bool								FromString (const XBOX::VString& inString);

	/**
	 *	@function SplitElements
	 *	@brief Splits the given string into separate elements. Elements are expected to be separated by commas.
	 *
	 *	For example, the string "text/plain; q=0.5, text/html, text/x-dvi; q=0.8" is split into the elements
	 *		text/plain; q=0.5
	 *		text/html
	 *		text/x-dvi; q=0.8
	 *
	 *	Commas enclosed in double quotes do not split elements.
	 *	If ignoreEmpty is true, empty elements are not returned.
	 */
	static void							SplitElements (const XBOX::VString& inString, XBOX::VectorOfVString& outElements, bool ignoreEmpty = true);

	/**
	 *	@function SearchValue
	 *	@brief Use this function to search (string comparison not case sensitive) inside a comma separated string (value of a header entry).
	 * 
	 *  For example, search the string "Upgrade" in the following header entry: "Connection: keep-alive, Upgrade". 
	 */

	static bool							SearchValue (const XBOX::VString &inValue, const XBOX::VString &inString);

	/**
	 *	@function SplitParameters
	 *	@brief Splits the given string into a value and a collection of parameters.
	 *	Parameters are expected to be separated by semicolons.
	 *	Enclosing quotes of parameter values are removed.
	 *
	 *	For example, the string multipart/mixed; boundary="MIME_boundary_01234567"
	 *	is split into the value
	 *		multipart/mixed
	 *	and the parameter
	 *		boundary -> MIME_boundary_01234567
	 */
	static void							SplitParameters (const XBOX::VString& inString, XBOX::VString& outValue, XBOX::VNameValueCollection& outParameters, bool withReverseSolidus = false);
	static void							SplitParameters (const UniChar *begin, const UniChar *end, XBOX::VNameValueCollection& outParameters, bool withReverseSolidus = false, const UniChar inUniCharSeparator = CHAR_SEMICOLON);

	/**
	 *	@function Quote
	 *	@brief Checks if the value must be quoted. If so, the value is appended to result, enclosed in double-quotes.
	 *	Otherwise. the value is appended to result as-is.
	 */
	static void							Quote (const XBOX::VString& inValue, XBOX::VString& outResult, bool allowSpace = false);

private:
	XBOX::VNameValueCollection			fHeaderList;
};


END_TOOLBOX_NAMESPACE

#endif // __HTTP_HEADER_INCLUDED__



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
#ifndef __HTTP_COOKIE_INCLUDED__
#define __HTTP_COOKIE_INCLUDED__

BEGIN_TOOLBOX_NAMESPACE


class XTOOLBOX_API VHTTPCookie : public XBOX::VObject, public XBOX::IRefCountable
{
public:
										VHTTPCookie();
										VHTTPCookie (const XBOX::VString& inCookieString);
										VHTTPCookie (const VHTTPCookie& cookie);
	virtual								~VHTTPCookie();

	VHTTPCookie&						operator = (const VHTTPCookie& inCookie);
	bool								operator == (const VHTTPCookie& inCookie) const;

	void								Clear();

	bool								IsValid() const;

	sLONG								GetVersion() const { return fVersion; }
	const XBOX::VString&				GetName() const { return fName; }
	const XBOX::VString&				GetValue() const { return fValue; }
	const XBOX::VString&				GetComment() const { return fComment; }
	const XBOX::VString&				GetDomain() const { return fDomain; }
	const XBOX::VString&				GetPath() const { return fPath; }
	bool								GetSecure() const { return fSecure; }
	sLONG								GetMaxAge() const { return fMaxAge; }
	bool								GetHttpOnly() const { return fHTTPOnly; }

	void								SetVersion (sLONG inVersion) { fVersion = inVersion; }
	void								SetName (const XBOX::VString& inName) { fName.FromString (inName); }
	void								SetValue (const XBOX::VString& inValue) { fValue.FromString (inValue); }
	void								SetComment (const XBOX::VString& inComment) { fComment.FromString (inComment); }
	void								SetDomain (const XBOX::VString& inDomain) { fDomain.FromString (inDomain); }
	void								SetPath (const XBOX::VString& inPath) { fPath.FromString (inPath); }
	void								SetSecure (bool inSecure) { fSecure = inSecure; }
	void								SetMaxAge (sLONG inMaxAge) { fMaxAge = inMaxAge; }
	void								SetHttpOnly (bool flag = true) { fHTTPOnly = flag; }

	XBOX::VString						ToString (bool inAlwaysExpires = true) const;
	void								FromString (const XBOX::VString& inString);

private:
	sLONG								fVersion;		// Version must be either 0 (for Netscape cookie) or 1 (for RFC 2109 cookie).
	XBOX::VString						fName;
	XBOX::VString						fValue;
	XBOX::VString						fComment;		// Comments are only supported for version 1 cookies.
	XBOX::VString						fDomain;
	XBOX::VString						fPath;
	bool								fSecure;
	sLONG								fMaxAge;		// A value of -1 causes the cookie to never expire on the client. A value of 0 deletes the cookie on the client.
	bool								fHTTPOnly;
};

typedef std::vector<XBOX::VRefPtr<XBOX::VHTTPCookie> >	VectorOfCookie;


END_TOOLBOX_NAMESPACE

#endif	// __HTTP_COOKIE_INCLUDED__
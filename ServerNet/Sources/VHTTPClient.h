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

#ifndef __HTTP_CLIENT_INCLUDED__
#define __HTTP_CLIENT_INCLUDED__

#include "VTCPEndPoint.h"
#include "VHTTPHeader.h"


BEGIN_TOOLBOX_NAMESPACE


#define WITH_HTTP_CLIENT_DEBUG_LOG	0	// for Debug purposes only


class XTOOLBOX_API VAuthInfos : public XBOX::VObject
{
public:
	typedef enum { AUTH_NONE = 0, AUTH_BASIC, AUTH_DIGEST, AUTH_NTLM } AuthMethod;

											VAuthInfos() : fAuthenticationMethod (AUTH_NONE) {};
	virtual									~VAuthInfos() {};

	VAuthInfos&								operator = (const VAuthInfos& inAuthenticationInfos);

	const XBOX::VString&					GetUserName() const { return fUserName; }
	const XBOX::VString&					GetPassword() const { return fPassword; }
	const XBOX::VString&					GetRealm() const { return fRealm; }
	const XBOX::VString&					GetURI() const { return fURI; }
	const XBOX::VString&					GetDomain() const { return fDomain; }
	AuthMethod								GetAuthenticationMethod() const { return fAuthenticationMethod; }

	void									SetUserName (const XBOX::VString& inValue) { fUserName.FromString (inValue); }
	void									SetPassword (const XBOX::VString& inValue) { fPassword.FromString (inValue); }
	void									SetRealm (const XBOX::VString& inValue) { fRealm.FromString (inValue); }
	void									SetURI (const XBOX::VString& inValue) { fURI.FromString (inValue); }
	void									SetDomain (const XBOX::VString& inValue) { fDomain.FromString (inValue); }
	void									SetAuthenticationMethod (const AuthMethod inValue) { fAuthenticationMethod = inValue; }

	bool									IsValid() const { return fUserName.GetLength() || fPassword.GetLength(); }

	void									Clear();

private:
	XBOX::VString							fUserName;
	XBOX::VString							fPassword;
	XBOX::VString							fRealm;
	XBOX::VString							fURI;
	XBOX::VString							fDomain;
	AuthMethod								fAuthenticationMethod;	// 0: NONE (automatic), 1: BASIC, 2: DIGEST, 3:NTLM
};


typedef void (* HTTPRequestProgressionCallBack) (void *ioPrivateData, sLONG inMessage, sLONG inProgressionPercentage);
typedef void (* HTTPRequestAuthenticationDialogCallBack) (VAuthInfos& ioAuthenticationInfos, void *inPrivateData);


class XTOOLBOX_API VHTTPClient : public XBOX::VObject
{
public:
	typedef enum { PROGSTATUS_NULL = 0, PROGSTATUS_STARTING, PROGSTATUS_SENDING_REQUEST, PROGSTATUS_RECEIVING_RESPONSE } HTTP_ProgressionStatus;

											VHTTPClient();
											VHTTPClient (const XBOX::VURL& inURL, const XBOX::VString& inContentType, bool inWithTimeStamp, bool inWithKeepAlive, bool inWithHost, bool inWithAuthentication = false);
	virtual									~VHTTPClient();

	/*
	 *	Request
	 */
	bool									SetRequestBody (const void *inDataPtr, XBOX::VSize inDataSize);
	/*
	 *	SetRequestBodyString helper function:
	 *	 - Convert & Set Body using correct charset
	 *	 - Defines Content-Type header with charset param
	 */
	bool									SetRequestBodyString (const XBOX::VString& inString, const XBOX::VString& inContentType, XBOX::CharSet inCharSet = XBOX::VTC_UTF_8);
	bool									AppendToRequestBody (void *inDataPtr, XBOX::VSize inDataSize);
	VHTTPHeader&							GetRequestHTTPHeaders() { return fHeader; }
	const VHTTPHeader&						GetRequestHTTPHeaders() const { return fHeader; }
	bool									AddHeader (const XBOX::VString& inName, const XBOX::VString& inValue);
	bool									SetUserAgent (const XBOX::VString& inUserAgent);
	void									SetUseProxy (const XBOX::VString& inProxy, sLONG inProxyPort);
	bool									SetAuthenticationValues (const XBOX::VString& inUserName, const XBOX::VString& inPassword, VAuthInfos::AuthMethod inAuthenticationMethod, bool proxyAuthentication);

	void									SetProgressCallBack (HTTPRequestProgressionCallBack inCallBackPtr, void *inPrivateData) { fProgressionCallBackPtr = inCallBackPtr; fProgressionCallBackPrivateData = inPrivateData; }
	void									SetAuthenticationDialogCallBack (HTTPRequestAuthenticationDialogCallBack inCallBackPtr, void * inPrivateData) { fAuthenticationDialogCallBackPtr = inCallBackPtr; fAuthenticationDialogCallBackPrivateData = inPrivateData;  }

	void									SetAsUpgradeRequest () { fUpgradeRequest = true;	}
	bool									SetConnectionKeepAlive (bool inKeepAlive);					// Not applicable if it is an upgrade request.

	/*
	 *	Response
	 */
	sLONG									GetHTTPStatusCode() const { return fStatusCode; }
	bool									GetResponseHeaderValue (const XBOX::VString& inName, XBOX::VString& outValue);
	const VHTTPHeader&						GetResponseHeaders() const { return fResponseHeader; }
	void									GetResponseStatusLine (XBOX::VString& firstLine) const;
	const XBOX::VMemoryBuffer<>&			GetResponseBody() const { return fResponseBody; }
	/*
	 *	GetResponseBodyString helper function:
	 *	Convert body into VString using correct charset, only when mimeType is recognized as MIMETYPE_TEXT (return an empty string for 	MIMETYPE_BINARY and MIMETYPE_IMAGE)
	 */
	void									GetResponseBodyString (XBOX::VString& outBodyString);

	/*
	 *	Prepare Request, Open Connection, Send Request and Close Connection
	 */
	XBOX::VError							Send (HTTP_Method inMethod);

	/*
	 *	For WebSockets
	 */

	// Create connection between server and client.

	XBOX::VError							OpenConnection (XBOX::VTCPSelectIOPool *inSelectIOPool);

	// Actually does nothing if the socket has been "stolen".

	XBOX::VError							CloseConnection ();

	// Steal or just get the TCP socket from HTTP client. 

	XBOX::VTCPEndPoint						*StealEndPoint();									// Must be released when done.
	XBOX::VTCPEndPoint						*GetEndPoint()		{	return fTCPEndPoint;	}	

	// Prepare request and send it synchronously, connection must have been opened before and header values set appropriately.

	XBOX::VError							SendRequest (HTTP_Method inMethod);

	// Synchronously read the response header from a request.

	XBOX::VError							ReadResponseHeader ();

	// Asynchronous reading of the response header. Submit read byte(s) to ContinueReadingResponseHeader() until *outIsComplete is true.

	void									StartReadingResponseHeader ();
	XBOX::VError							ContinueReadingResponseHeader (const char *inBuffer, VSize inBufferSize, bool *outIsComplete);

	// While reading the response header, too much data can be read. This is because we use buffered reading and the server can actually
	// send data after response immediately. So while reading the response, we may also read part of the following data.
	// These methods can be called only if a response header has been read correctly.

	const void *							GetLeftOverDataPtr () const { return fLeftOver.GetDataPtr(); }
	XBOX::VSize								GetLeftOverSize () const { return fLeftOver.GetDataSize(); }

	/*
	 *	Some UseFull Getters and Setters
	 */
	void									SetUseSSL (bool inValue) { fUseSSL = inValue; }
	bool									GetUseSSL() const { return fUseSSL; }
	void									SetUseHTTPCompression (bool inValue) { fUseHTTPCompression = inValue; }
	bool									GetUseHTTPCompression() const { return fUseHTTPCompression; }
	void									SetKeepAlive (bool inValue) { fKeepAlive = inValue; }
	bool									GetKeepAlive() const { return fKeepAlive; }

	/*
	 *	Connection Timeout (in seconds)
	 */
	void									SetConnectionTimeout (sLONG inValue) { fConnectionTimeout = inValue; }
	sLONG									GetConnectionTimeout() const { return fConnectionTimeout; }

	/*
	 *	Read/Write operations Timeout (in seconds)
	 */
	void									SetReadWriteTimeout(sLONG inValue) { fReadWriteTimeout = inValue; }
	sLONG									GetReadWriteTimeout() const { return fReadWriteTimeout; }

	/*
	 *	Redirections
	 */
	void									SetFollowRedirect (bool inValue) { fFollowRedirect = inValue; }
	bool									GetFollowRedirect () const { return fFollowRedirect; }
	void									SetMaxRedirections (sLONG inValue) { fMaxRedirections = inValue; }
	sLONG									GetMaxRedirections() const { return fMaxRedirections; }

	/*
	 *	Tools
	 */
	static bool								ParseURL(const XBOX::VURL& inURL, XBOX::VString *outDomain, sLONG *outPort = NULL, XBOX::VString *outFolder = NULL, XBOX::VString *outQuery = NULL, bool *outUseSSL = NULL, XBOX::VString *outUserName = NULL, XBOX::VString *outPassword = NULL, XBOX::VString *outProxy = NULL, PortNumber* outProxyPort = NULL);

	void									Init (const XBOX::VURL& inURL, const XBOX::VString& inContentType, bool inWithTimeStamp, bool inWithKeepAlive, bool inWithHost, bool inWithAuthentication = false);
	void									Clear();

	bool									IsFirstRequest() const { return (0 == fNumberOfRequests); }

	/*
	 *	Request Header manipulation
	 */
	bool									AddAuthorizationHeader (const XBOX::VString& inUserName, const XBOX::VString& inPassword, VAuthInfos::AuthMethod inAuthenticationMethod, HTTP_Method inMethod, bool proxyAuthentication);
	bool									AddAuthorizationHeader (const VAuthInfos& inValue, HTTP_Method inMethod, bool proxyAuthentication);

	bool									FindChosenAuthenticationHeader (const XBOX::VString& inAuthMethodName, XBOX::VString& outAuthMethodName);
	bool									AddBasicAuthorizationHeader (const XBOX::VString& inUserName, const XBOX::VString& inPassword, HTTP_Method inMethod, bool proxyAuthentication);
	bool									AddDigestAuthorizationHeader (const XBOX::VString& inAuthorizationHeaderValue, const XBOX::VString& inUserName, const XBOX::VString& inPassword, HTTP_Method inMethod, bool proxyAuthentication);
	bool									AddNtlmAuthorizationHeader (const XBOX::VString& inAuthorizationHeaderValue, const XBOX::VString& inUserName, const XBOX::VString& inPassword, HTTP_Method inMethod, bool proxyAuthentication);

	static void								ClearSavedAuthenticationInfos();

	/*
	 *	Authentication Dialog & Infos
	 */
	void									SetShowAuthenticationDialog (bool inValue) { fShowAuthenticationDialog = inValue; }
	bool									GetShowAuthenticationDialog() const { return fShowAuthenticationDialog; }
	void									SetUseAuthentication (bool inValue) { fUseAuthentication = inValue; }
	bool									GetUseAuthentication() const { return fUseAuthentication; }
	void									SetResetAuthenticationInfos (bool inValue) { fResetAuthenticationInfos = inValue; }
	bool									GetResetAuthenticationInfos() const { return fResetAuthenticationInfos; }
	void									SetMayUsePreAuthentication(bool inValue) { fMayUsePreAuthentication = inValue; }	// For 4D backward compatibility
	bool									GetMayUsePreAuthentication() const { return fMayUsePreAuthentication; }				// For 4D backward compatibility

	sLONG									GetPort() const { return fPort; }
	const XBOX::VString&					GetDomain() const { return fDomain; }

private:
	XBOX::VError							_SendRequestAndReceiveResponse();
	XBOX::VError							_SendCONNECTToProxy();
	XBOX::VError							_SendRequestHeader();
	bool									_ParseURL (const XBOX::VURL& inURL);
	XBOX::VError							_GenerateRequest (HTTP_Method inMethod);
	bool									_ExtractAuthenticationInfos (XBOX::VString& outRealm, VAuthInfos::AuthMethod& outProxyAuthenticationMethod);
	void									_ReinitResponseHeader();
	void									_InitCustomHeaders();
	void									_ComputeDomain (XBOX::VString& outDomain, bool useProxy);
	sLONG									_ExtractHTTPStatusCode() const;
	void									_SetBodyReply (char *inBody, XBOX::VSize inBodySize);
	void									_ReinitHTTP (bool reinitHeader = true, bool reinitReplyHeader = true, bool reinitBufferPool = true);
	bool									_IsChunkedResponse();
	bool									_SetHostHeader();

	XBOX::VError							_OpenConnection (const XBOX::VString& inDNSNameOrIP, const sLONG inPort, bool inUseSSL, XBOX::VTCPSelectIOPool *inSelectIOPool);
	bool									_ConnectionOpened();
	XBOX::VError							_PromoteToSSL();
	XBOX::VError							_WriteToSocket (void *inBuffer, XBOX::VSize inBufferSize);
	XBOX::VError							_ReadFromSocket (void *ioBuffer, const XBOX::VSize inMaxBufferSize, XBOX::VSize& ioBufferSize);
	XBOX::VError							_ReadExactlyFromSocket(void *ioBuffer, const XBOX::VSize inMaxBufferSize, XBOX::VSize& ioReadDataSize);
	bool									_LastUsedAddressChanged (const XBOX::VString& inDNSNameOrIP, const sLONG inPort);
	void									_SaveLastUsedAddress (const XBOX::VString& inDNSNameOrIP, const sLONG inPort);

#if WITH_HTTP_CLIENT_DEBUG_LOG
	/* Used to Log Data Sent/Received - for Debug purposes only */
	void									_LogData(void *inData, XBOX::VSize inDataSize);
#endif

private:
	/*
	 *	Request
	 */
	VHTTPHeader								fHeader;
	bool									fUseSSL;
	HTTP_Method								fRequestMethod;
	XBOX::VMemoryBuffer<>					fRequestBody;
	XBOX::VString							fDomain;
	XBOX::VString							fFolder;
	XBOX::VString							fQuery;
	XBOX::VString							fProxy;				// contains the proxy name, if any
	sLONG									fProxyPort;
	sLONG									fConnectionTimeout;
	sLONG									fReadWriteTimeout;
	sLONG									fPort;
	bool									fUseProxy;			// tells if request uses proxy
	bool									fNTLMAuthenticationInProgress;	//jmo - true between NTLM negociation and challenge to tell us that http errors 401 & 407 are expected

	/*
	 *	Request Call Backs
	 */
	HTTPRequestProgressionCallBack			fProgressionCallBackPtr;
	void *									fProgressionCallBackPrivateData;
	HTTPRequestAuthenticationDialogCallBack	fAuthenticationDialogCallBackPtr;
	void *									fAuthenticationDialogCallBackPrivateData;

	/*
	 *	Response
	 */
	VHTTPHeader								fResponseHeader;
	XBOX::VMemoryBuffer<>					fResponseHeaderBuffer;
	XBOX::VMemoryBuffer<>					fResponseBody;
	sLONG									fStatusCode;
	XBOX::VMemoryBuffer<>					fLeftOver;
	
	/*
	 *	HTTP & Proxy Authentication
	 */
	bool									fUseAuthentication;
	bool									fResetAuthenticationInfos;
	bool									fMayUsePreAuthentication;	// For 4D backward compatibility
	VAuthInfos								fHTTPAuthenticationInfos;
	VAuthInfos								fProxyAuthenticationInfos;

	/*
	 *	HTTP Authentication infos
	 */
	static VAuthInfos						fSavedHTTPAuthenticationInfos;
	static VAuthInfos						fSavedProxyAuthenticationInfos;

	/*
	 *	Used to display Authentication dialog
	 */
	bool									fShowAuthenticationDialog;

	bool									fWithTimeStamp;
	bool									fWithHost;
	XBOX::VString							fContentType;

	/*
	 *	EndPoint
	 */
	XBOX::VTCPEndPoint *					fTCPEndPoint;

	/*
	 *	Keep-Alive
	 */
	bool									fKeepAlive;
	sLONG									fNumberOfRequests;
	XBOX::VString							fLastAddressUsed;
	bool									fUpgradeRequest;		// If it is a upgrade request, this will override fKeepAlive.

	/*
	 *	Content-Encoding
	 */
	bool									fUseHTTPCompression;

	/*
	 *	Redirections
	 */
	bool									fFollowRedirect;
	sLONG									fMaxRedirections;

	/*
	 *	HTTP Client User-Agent
	 */
	static XBOX::VString					fUserAgent;

	/* Used to Log Data Sent/Received - for Debug purposes only */
#if WITH_HTTP_CLIENT_DEBUG_LOG
	XBOX::VFile *							fLogFile;
	XBOX::VFileDesc *						fLogFileDesc;
#endif
};


END_TOOLBOX_NAMESPACE

#endif // __HTTP_CLIENT_INCLUDED__

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
#ifndef __VXML_CURL_HTTP_REQUEST__
#define __VXML_CURL_HTTP_REQUEST__


#include "JavaScript/VJavaScript.h"


BEGIN_TOOLBOX_NAMESPACE


class XTOOLBOX_API cURLXMLHttpRequest : public VObject
{
 public :

    class Command
    {
    private :
        XBOX::VJSObject fReceiver;
        const XBOX::VJSObject fFunction;

    public :

        Command(const XBOX::VJSObject& inReceiver, const XBOX::VJSObject& inFunction) : fReceiver(inReceiver), fFunction(inFunction) {}

        void Execute()
        {
            XBOX::VJSException except;

            if(fReceiver.HasRef() && fFunction.HasRef() && fFunction.IsFunction())
				fReceiver.CallFunction(fFunction, NULL, NULL, except); //todo : Que faire de l'exception ?
        }
    };


    typedef enum ReadyState_enum {UNSENT=0, OPENED, HEADERS_RECEIVED, LOADING, DONE} ReadyState;
	typedef const char* BinaryPtr;


    //cURLXMLHttpRequest(const XBOX::VString& inProxy="", uLONG inProxyPort=-1);
    cURLXMLHttpRequest();
    virtual ~cURLXMLHttpRequest();

    XBOX::VError    Open                    (const XBOX::VString& inMethod, const XBOX::VString& inUrl, bool inAsync=true);
    XBOX::VError    SetProxy                (const XBOX::VString& inProxy, const uLONG inPort);
	XBOX::VError    SetSystemProxy			(bool inUseSystemProxy=true);
	XBOX::VError    SetUserInfos            (const XBOX::VString& inUser, const XBOX::VString& inPasswd, bool inAllowBasic);
	XBOX::VError	SetClientCertificate	(const XBOX::VString& inKeyPath, const XBOX::VString& inCertPath);
    XBOX::VError    SetRequestHeader        (const XBOX::VString& inKey, const XBOX::VString& inValue);
	XBOX::VError    SetTimeout		        (XBOX::VLong inConnectMs=XBOX::VLong(0) /*defaults to 3000ms*/, XBOX::VLong inTotalMs=XBOX::VLong(0) /*defaults to forever*/);
    XBOX::VError    OnReadyStateChange      (XBOX::VJSObject inReceiver, const XBOX::VJSObject& inFunction);
    XBOX::VError    Send                    (const XBOX::VString& inData="", XBOX::VError* outImplErr=NULL);
	XBOX::VError	SendBinary				(const void* data, sLONG datalen, XBOX::VError* outImplErr);
    XBOX::VError    Abort                   ();
    XBOX::VError    GetReadyState           (XBOX::VLong* outValue) const;
    XBOX::VError    GetStatus               (XBOX::VLong* outValue) const;
    XBOX::VError    GetStatusText           (XBOX::VString* outValue) const;
    XBOX::VError    GetResponseHeader       (const XBOX::VString& inKey, XBOX::VString* outValue) const;
    XBOX::VError    GetAllResponseHeaders   (XBOX::VString* outValue) const;
	XBOX::VError	SetResponseType			(const XBOX::VString& inValue);
	XBOX::VError	GetResponseType			(XBOX::VString* outValue);
    XBOX::VError    GetResponseText         (XBOX::VString* outValue) const;
	XBOX::VError	GetResponse				(XBOX::VJSValue& outValue) const;
	XBOX::VError	GetResponseBinary		(BinaryPtr& outPtr, XBOX::VSize& outLen) const;


 private :

	typedef enum {TEXT, BLOB} ResponseType;

    XBOX::CharSet   GetCharSetFromHeaders		() const;
	XBOX::VString	GetContentType				(const XBOX::VString inDefaultType=XBOX::VString("application/octet-stream")) const;

    const bool      fAsync;
    ReadyState      fReadyState;
    unsigned short  fStatus;
    XBOX::VString   fStatusText;
    XBOX::VString   fResponseText;
	ResponseType	fResponseType;
    XBOX::VString   fResponseXML;
    bool            fSendFlag;
    bool            fErrorFlag;
    XBOX::VString   fProxy;
    uLONG           fPort;
	bool			fUseSystemProxy;
    XBOX::VString   fUser;
    XBOX::VString   fPasswd;
    Command*        fChangeHandler;

    class CWImpl;
    CWImpl*         fCWImpl;

};


class XTOOLBOX_API VJScURLXMLHttpRequest : public XBOX::VJSClass<VJScURLXMLHttpRequest, cURLXMLHttpRequest>
{
 public:

    typedef XBOX::VJSClass<VJScURLXMLHttpRequest, cURLXMLHttpRequest> inherited;

	static XBOX::VJSObject	MakeConstructor (const XBOX::VJSContext& inContext);
	
    static void GetDefinition           (ClassDefinition& outDefinition);
    static void Initialize              (const XBOX::VJSParms_initialize& inParms,   cURLXMLHttpRequest* inXhr);
    static void Finalize                (const XBOX::VJSParms_finalize& inParms,     cURLXMLHttpRequest* inXhr);
    static void Construct               (XBOX::VJSParms_construct& inParms);
    static void CallAsFunction          (XBOX::VJSParms_callAsFunction& ioParms);
    //static void CallAsConstructor     (XBOX::VJSParms_callAsConstructor& ioParms);

    static void _Open                   (XBOX::VJSParms_callStaticFunction& ioParms, cURLXMLHttpRequest* inXhr);
    //static void _SetProxy             (XBOX::VJSParms_callStaticFunction& ioParms, cURLXMLHttpRequest* inXhr);
	static void _SetClientCertificate   (XBOX::VJSParms_callStaticFunction& ioParms, cURLXMLHttpRequest* inXhr);
    static void _SetRequestHeader       (XBOX::VJSParms_callStaticFunction& ioParms, cURLXMLHttpRequest* inXhr);
	static void _SetTimeout		        (XBOX::VJSParms_callStaticFunction& ioParms, cURLXMLHttpRequest* inXhr);
    static bool _OnReadyStateChange     (XBOX::VJSParms_setProperty& ioParms, cURLXMLHttpRequest* inXhr);
    static void _Send                   (XBOX::VJSParms_callStaticFunction& ioParms, cURLXMLHttpRequest* inXhr);
    static void _Abort                  (XBOX::VJSParms_callStaticFunction& ioParms, cURLXMLHttpRequest* inXhr);
    static void _GetReadyState          (XBOX::VJSParms_getProperty& ioParms, cURLXMLHttpRequest* inXhr);
    static void _GetStatus              (XBOX::VJSParms_getProperty& ioParms, cURLXMLHttpRequest* inXhr);
    static void _GetStatusText          (XBOX::VJSParms_getProperty& ioParms, cURLXMLHttpRequest* inXhr);
    static void _GetResponseHeader      (XBOX::VJSParms_callStaticFunction& ioParms, cURLXMLHttpRequest* inXhr);
    static void _GetAllResponseHeaders  (XBOX::VJSParms_callStaticFunction& ioParms, cURLXMLHttpRequest* inXhr);
    static void _GetResponseText        (XBOX::VJSParms_getProperty& ioParms, cURLXMLHttpRequest* inXhr);
    static bool _SetResponseType        (XBOX::VJSParms_setProperty& ioParms, cURLXMLHttpRequest* inXhr);
    static void _GetResponseType        (XBOX::VJSParms_getProperty& ioParms, cURLXMLHttpRequest* inXhr);
    static void _GetResponse	        (XBOX::VJSParms_getProperty& ioParms, cURLXMLHttpRequest* inXhr);
};

#if !WITH_NATIVE_HTTP_BACKEND
typedef cURLXMLHttpRequest VXMLHttpRequest;
typedef VJScURLXMLHttpRequest VJSXMLHttpRequest;
#endif

END_TOOLBOX_NAMESPACE


#endif

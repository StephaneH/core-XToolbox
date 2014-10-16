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
#ifndef __VXML_NATIVE_HTTP_REQUEST__
#define __VXML_NATIVE_HTTP_REQUEST__


BEGIN_TOOLBOX_NAMESPACE


class XTOOLBOX_API VXMLHttpRequest: public VObject, public IRefCountable
{
public:

	class Command: public VJSObject
    {
    public:
		Command(const VJSObject& inReceiver, const VJSObject& inFunction) : VJSObject(inReceiver.GetContext()), fReceiver(inReceiver), fFunction(inFunction) {}

        void Execute()
        {
            VJSException except;

            if(fReceiver.HasRef() && fFunction.HasRef() && fFunction.IsFunction())
				fReceiver.CallFunction(fFunction, NULL, NULL, except); //todo: Que faire de l'exception ?
        }

	private:

		VJSObject fReceiver;
		const VJSObject fFunction;
    };


	enum ReadyState { UNSENT = 0, OPENED, HEADERS_RECEIVED, LOADING, DONE };
	typedef const char* BinaryPtr;

    VXMLHttpRequest();
	virtual ~VXMLHttpRequest();

	VError	Open					(const VString& inMethod, const VString& inUrl, bool inAsync=true);
	VError	SetProxy				(const VString& inProxy, const PortNumber PortNumber);
	VError	SetSystemProxy			(bool inUseSystemProxy=true);
	VError	SetUserInfos			(const VString& inUser, const VString& inPasswd, bool inAllowBasic);
	VError	SetClientCertificate	(const VString& inKeyPath, const VString& inCertPath);
	VError	SetRequestHeader		(const VString& inKey, const VString& inValue);
	VError	SetTimeout				(VLong inConnectMs=VLong(0) /*defaults to 3000ms*/, VLong inTotalMs=VLong(0) /*defaults to forever*/);
	VError	OnReadyStateChange		(const VJSObject& inReceiver, const VJSObject& inFunction);
	VError	Send					(const VString& inData="", VError* outImplErr=NULL);
	VError	SendBinary				(const void* data, sLONG datalen, VError* outImplErr);
	VError	GetReadyState			(VLong* outValue) const;
	VError	GetStatus				(VLong* outValue) const;
	VError	GetStatusText			(VString* outValue) const;
	VError	GetResponseHeader		(const VString& inKey, VString* outValue) const;
	VError	GetAllResponseHeaders   (VString* outValue) const;
	VError	SetResponseType			(const VString& inValue);
	VError	GetResponseType			(VString* outValue);
	VError	GetResponseText			(VString* outValue) const;
	VError	GetResponse				(VJSValue& outValue) const;
	VError	GetResponseBinary		(BinaryPtr& outPtr, VSize& outLen) const;

private:

	VXMLHttpRequest(VXMLHttpRequest&);	//no copy !

	enum ResponseType { TEXT, BLOB };

	CharSet	GetCharSetFromHeaders	() const;
	VString	GetContentType			(const VString& inDefaultType=CVSTR("application/octet-stream")) const;

    ReadyState		fReadyState;
	ResponseType	fResponseType;
    VString			fResponseXML;
    bool            fSendFlag;
    bool            fErrorFlag;
    VString			fProxy;
    PortNumber		fPort;
	bool			fUseSystemProxy;
    VString			fUser;
    Command*		fChangeHandler;
	VURL			fURL;
	HTTP_Method		fMethod;

    VHTTPClient*	fImpl;
};


class XTOOLBOX_API VJSXMLHttpRequest: public VJSClass<VJSXMLHttpRequest, VXMLHttpRequest>
{
 public:

    typedef VJSClass<VJSXMLHttpRequest, VXMLHttpRequest> inherited;

	static VJSObject	MakeConstructor (const VJSContext& inContext);
	
    static void GetDefinition           (ClassDefinition& outDefinition);
    static void Initialize              (const VJSParms_initialize& inParms,   VXMLHttpRequest* inXhr);
    static void Finalize                (const VJSParms_finalize& inParms,     VXMLHttpRequest* inXhr);
    static void Construct               (VJSParms_construct& inParms);
    static void CallAsFunction          (VJSParms_callAsFunction& ioParms);

    static void _Open                   (VJSParms_callStaticFunction& ioParms, VXMLHttpRequest* inXhr);
	static void _SetClientCertificate   (VJSParms_callStaticFunction& ioParms, VXMLHttpRequest* inXhr);
    static void _SetRequestHeader       (VJSParms_callStaticFunction& ioParms, VXMLHttpRequest* inXhr);
	static void _SetTimeout		        (VJSParms_callStaticFunction& ioParms, VXMLHttpRequest* inXhr);
    static bool _OnReadyStateChange     (VJSParms_setProperty& ioParms, VXMLHttpRequest* inXhr);
    static void _Send                   (VJSParms_callStaticFunction& ioParms, VXMLHttpRequest* inXhr);
    static void _GetReadyState          (VJSParms_getProperty& ioParms, VXMLHttpRequest* inXhr);
    static void _GetStatus              (VJSParms_getProperty& ioParms, VXMLHttpRequest* inXhr);
    static void _GetStatusText          (VJSParms_getProperty& ioParms, VXMLHttpRequest* inXhr);
    static void _GetResponseHeader      (VJSParms_callStaticFunction& ioParms, VXMLHttpRequest* inXhr);
    static void _GetAllResponseHeaders  (VJSParms_callStaticFunction& ioParms, VXMLHttpRequest* inXhr);
    static void _GetResponseText        (VJSParms_getProperty& ioParms, VXMLHttpRequest* inXhr);
    static bool _SetResponseType        (VJSParms_setProperty& ioParms, VXMLHttpRequest* inXhr);
    static void _GetResponseType        (VJSParms_getProperty& ioParms, VXMLHttpRequest* inXhr);
    static void _GetResponse	        (VJSParms_getProperty& ioParms, VXMLHttpRequest* inXhr);
};


END_TOOLBOX_NAMESPACE


#endif

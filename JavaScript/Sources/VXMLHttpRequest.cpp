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

#include "VJavaScriptPrecompiled.h" //Fait plaisir a VS

#include "JavaScript/VJavaScript.h"


USING_TOOLBOX_NAMESPACE

#if WITH_NATIVE_HTTP_BACKEND


class VMethodHelper: public VObject
{
public:

    VMethodHelper(const VString& inMethod):
        fIsValid(false), fIsDangerous(false), fDoesSupportData(false), fMethod(HTTP_UNKNOWN)
    {
        VString METHOD(inMethod);
        METHOD.ToUpperCase();

        if(METHOD.EqualToUSASCIICString("GET"))
            fMethod=HTTP_GET, fIsValid=true;
        else if(METHOD.EqualToUSASCIICString("POST"))
            fMethod=HTTP_POST, fIsValid=true;
        else if(METHOD.EqualToUSASCIICString("PUT"))
            fMethod=HTTP_PUT, fDoesSupportData=true, fIsValid=true;
        else if(METHOD.EqualToUSASCIICString("HEAD"))
            fMethod=HTTP_HEAD, fIsValid=true;
        else if(METHOD.EqualToUSASCIICString("DELETE"))
            fMethod=HTTP_DELETE, fIsValid=true;
    }

    bool IsValid()          { return fIsValid; }
    bool IsDangerous()      { return fIsDangerous; }
    bool DoesSupportData()  { return fDoesSupportData; }
	HTTP_Method GetMethod()	{ return fMethod; }


private:

    bool			fIsValid;
    bool			fIsDangerous;
    bool			fDoesSupportData;
	HTTP_Method		fMethod;
};



class VUrlHelper: public VObject
{
public:

    VUrlHelper(const VString& inUrl) { fUrl.FromString(inUrl, false /*do not touch, pretend unencoded*/); }

	VString		DropFragment()				{ VString tmp; fUrl.GetAbsoluteURL(tmp, false /*do not touch, want unencoded*/); return tmp; }
    bool		IsSupportedScheme()			{ return true; }
    bool		IsUserInfoSet()				{ return false; }
    bool		DoesSchemeSupportUserInfo()	{ return false; }

private:

    VURL fUrl;
};


VXMLHttpRequest::VXMLHttpRequest():
    fReadyState(UNSENT),
	fResponseType(TEXT),
    fErrorFlag(false),
    fPort(kBAD_PORT),
	fUseSystemProxy(true),
    fChangeHandler(NULL),
	fImpl(NULL)
{
		//empty
};


VXMLHttpRequest::~VXMLHttpRequest()
{
	if(fChangeHandler!=NULL)
	{
		fChangeHandler->Unprotect();
		delete fChangeHandler;
	}

	delete fImpl;
}


VError VXMLHttpRequest::Open(const VString& inMethod, const VString& inUrl, bool inAsync)
{
    if(fReadyState!=UNSENT)
        return vThrowError(VE_XHRQ_INVALID_STATE_ERROR);   //Doesn't follow the spec, but behave roughly as FireFox

	if(fImpl!=NULL)
		return vThrowError(VE_XHRQ_IMPL_FAIL_ERROR);


    VMethodHelper mh(inMethod);

    if(!mh.IsValid())
        return vThrowError(VE_XHRQ_SYNTAX_ERROR);

    if(mh.IsDangerous())
        return vThrowError(VE_XHRQ_SECURITY_ERROR);

	fMethod=mh.GetMethod();


    VUrlHelper uh(inUrl);

    if(!uh.IsSupportedScheme())
        return vThrowError(VE_XHRQ_NOT_SUPPORTED_ERROR);

    if(uh.IsUserInfoSet() && !uh.DoesSchemeSupportUserInfo())
        return vThrowError(VE_XHRQ_SYNTAX_ERROR);

	bool encodedString=false;
	fURL=VURL(uh.DropFragment(), encodedString);


    fImpl=new VHTTPClient();

    if(fImpl==NULL)
        return vThrowError(VE_XHRQ_IMPL_FAIL_ERROR);

	VString contentType;	//Content type is set later
	bool withTimeStamp=false, withKeepAlive=false, withHost=true, withAuthentication=false;

	fImpl->Init(fURL, contentType, withTimeStamp, withKeepAlive, withHost, withAuthentication);

	//cURL default behavior, enforced by unit tests
	fImpl->AddHeader(CVSTR("Accept"), CVSTR("*/*"));

	fReadyState=OPENED;

    if(fChangeHandler)
        fChangeHandler->Execute();

    return VE_OK;
}


VError VXMLHttpRequest::SetProxy(const VString& inHost, PortNumber inPort)
{
    if(fReadyState!=UNSENT && fReadyState!=OPENED)
        return vThrowError(VE_XHRQ_INVALID_STATE_ERROR);

    fProxy=inHost;
    fPort=inPort;

    return VE_OK;
}


VError VXMLHttpRequest::SetSystemProxy(bool inUseSystemProxy)
{
	fUseSystemProxy=inUseSystemProxy;

    return VE_OK;
}


VError VXMLHttpRequest::SetUserInfos(const VString& inUser, const VString& inPasswd, bool inAllowBasic)
{
    if(fReadyState!=UNSENT && fReadyState!=OPENED)
        return vThrowError(VE_XHRQ_INVALID_STATE_ERROR); 
	
    if(fImpl==NULL)
        return vThrowError(VE_XHRQ_IMPL_FAIL_ERROR);
	
	VAuthInfos auth;

	auth.SetUserName(inUser);
	auth.SetPassword(inPasswd);

	//jmo BUG - In cURL based XHR, inAllowBasic would mean "DIGEST and BASIC allowed" (ie DIGEST was always allowed)
	//          Now it really means "Prefer BASIC over DIGEST" (ie you have to choose)
	//          This was an odd behavior, added to suit Studio's needs. Not sure if it still matters...
	VAuthInfos::AuthMethod method=inAllowBasic ? VAuthInfos::AUTH_BASIC: VAuthInfos::AUTH_DIGEST;
	
	bool proxyAuthentication=false;
	bool res=fImpl->SetAuthenticationValues(inUser, inPasswd, method, proxyAuthentication);
	
    return VE_OK;
}


VError VXMLHttpRequest::SetClientCertificate(const VString& inKeyPath, const VString& inCertPath)
{
	if(fReadyState!=UNSENT && fReadyState!=OPENED)
        return vThrowError(VE_XHRQ_INVALID_STATE_ERROR); 
	
    if(fImpl==NULL)
        return vThrowError(VE_XHRQ_IMPL_FAIL_ERROR);
	
	if(inKeyPath.IsEmpty() || inCertPath.IsEmpty())
		return vThrowError(VE_XHRQ_SECURITY_ERROR);
	
	VError verr=VE_OK;

	//jmo TODO - On n'a pas l'quivalent avec le client natif !

	verr=VE_UNIMPLEMENTED;
		
    return verr;
}


VError VXMLHttpRequest::SetRequestHeader(const VString& inKey, const VString& inValue)
{
    if(fReadyState!=OPENED)
        return vThrowError(VE_XHRQ_INVALID_STATE_ERROR);

    if(fImpl==NULL)
        return vThrowError(VE_XHRQ_IMPL_FAIL_ERROR);

	VError verr=VE_OK;

	fImpl->AddHeader(inKey, inValue);

    return verr;
}


VError VXMLHttpRequest::SetTimeout(VLong inConnectMs, VLong inTotalMs)
{
    if(fReadyState!=OPENED)
        return vThrowError(VE_XHRQ_INVALID_STATE_ERROR);

    if(fImpl==NULL)
        return vThrowError(VE_XHRQ_IMPL_FAIL_ERROR);


	VError verr=VE_OK;

	fImpl->SetConnectionTimeout(inTotalMs);

    return verr;
}


VError VXMLHttpRequest::OnReadyStateChange(const VJSObject& inReceiver, const VJSObject& inHandler)
{
	if(fChangeHandler!=NULL)
	{
		fChangeHandler->Unprotect();
		delete fChangeHandler;
	}

    fChangeHandler=new Command(inReceiver, inHandler);

    if(fChangeHandler==NULL)
        return vThrowError(VE_XHRQ_IMPL_FAIL_ERROR);

	fChangeHandler->Protect();

    return VE_OK;
}



VError VXMLHttpRequest::SendBinary(const void* data, sLONG datalen, VError* outImplErr)
{
	if(fReadyState!=OPENED)
		return vThrowError(VE_XHRQ_INVALID_STATE_ERROR);

	if(fImpl==NULL)
		return vThrowError(VE_XHRQ_IMPL_FAIL_ERROR);

	fErrorFlag=false;

	VError verr=VE_OK;

	if(!fProxy.IsEmpty() || !fUseSystemProxy)
	{
		fImpl->SetUseProxy(fProxy, fPort);
	}
	else if(fUseSystemProxy)
	{
		VString proxy;
		PortNumber port=kBAD_PORT;
        
		verr=VProxyManager::GetProxyForURL(fURL, &proxy, &port);
        
		if(verr==VE_OK && !proxy.IsEmpty())
			fImpl->SetUseProxy(proxy, port);
	}

	if(datalen>0 && data!=NULL)
	{
		bool res=fImpl->SetRequestBody(data, datalen);
		
		xbox_assert(fMethod==HTTP_UNKNOWN || fMethod==HTTP_POST);

		if(!res)
			verr=VE_XHRQ_IMPL_FAIL_ERROR;
	}

	if(verr==VE_OK)
		verr=fImpl->Send(HTTP_POST);

	if(verr==VE_OK)
	{
		fReadyState=HEADERS_RECEIVED;

		if(fChangeHandler)
			fChangeHandler->Execute();

		fReadyState=LOADING;

		if(fChangeHandler)
			fChangeHandler->Execute();
	}

	fReadyState=DONE;

	if(fChangeHandler)
		fChangeHandler->Execute();

	return verr==VE_OK ? VE_OK : vThrowError(VE_XHRQ_SEND_ERROR);
}


VError VXMLHttpRequest::Send(const VString& inData, VError* outImplErr)
{
    if(fReadyState!=OPENED)
        return vThrowError(VE_XHRQ_INVALID_STATE_ERROR);
	
    if(fImpl==NULL)
        return vThrowError(VE_XHRQ_IMPL_FAIL_ERROR);

    fErrorFlag=false;

    VError verr=VE_OK;

	if(!fProxy.IsEmpty() || !fUseSystemProxy)
	{
		fImpl->SetUseProxy(fProxy, fPort);
	}
	else if(fUseSystemProxy)
	{
		VString proxy;
		PortNumber port=kBAD_PORT;

		verr=VProxyManager::GetProxyForURL(fURL, &proxy, &port);

		if(verr==VE_OK && !proxy.IsEmpty())
			fImpl->SetUseProxy(proxy, port);
	}

	if(!inData.IsEmpty())
	{
		VString defaultContentType=CVSTR("text/plain");
		CharSet defaultCharSet=VTC_UTF_8;

		bool res=fImpl->SetRequestBodyString(inData, defaultContentType, defaultCharSet);
		
		if(!res)
			verr=VE_XHRQ_IMPL_FAIL_ERROR;
	}

	if(verr==VE_OK)
		verr=fImpl->Send(fMethod);
	
	if(verr==VE_OK)
    {
        fReadyState=HEADERS_RECEIVED;

        if(fChangeHandler)
            fChangeHandler->Execute();

        fReadyState=LOADING;

        if(fChangeHandler)
            fChangeHandler->Execute();
    }

    fReadyState=DONE;
	
    if(fChangeHandler)
        fChangeHandler->Execute();

    
	return verr==VE_OK ? VE_OK : vThrowError(VE_XHRQ_SEND_ERROR);
}


VError VXMLHttpRequest::GetReadyState(VLong* outValue) const
{
    *outValue=(sLONG)(fReadyState);

    return VE_OK;
}


VError VXMLHttpRequest::GetStatus(VLong* outValue) const
{
    if(fReadyState!=UNSENT && fReadyState!=OPENED && !fErrorFlag)
    {
        if(fImpl==NULL)
            return vThrowError(VE_XHRQ_IMPL_FAIL_ERROR);

        sLONG code=fImpl->GetHTTPStatusCode();
        bool hasCode=code>0;

        if(hasCode)
        {
            *outValue=(sLONG)(code);
            return VE_OK;
        }

        return vThrowError(VE_XHRQ_NO_RESPONSE_CODE_ERROR);
    }

    return vThrowError(VE_XHRQ_INVALID_STATE_ERROR);
}


VError VXMLHttpRequest::GetStatusText(VString* outValue) const
{
    VLong code;
    VError res=GetStatus(&code);

    if(res!=VE_OK)
        return res;

    switch((sLONG)(code))
    {
    case 100: *outValue=CVSTR("Continue") ; break;
    case 101: *outValue=CVSTR("Switching Protocols") ; break;
        //102   Processing (webdav)
    case 200: *outValue=CVSTR("OK") ; break;
    case 201: *outValue=CVSTR("Created") ; break;
    case 202: *outValue=CVSTR("Accepted") ; break;
    case 203: *outValue=CVSTR("Non-Authoritative Information") ; break;
    case 204: *outValue=CVSTR("No Content") ; break;
    case 205: *outValue=CVSTR("Reset Content") ; break;
    case 206: *outValue=CVSTR("Partial Content") ; break;
        //207   Multi-Status (webdav)
        //210   Content Different (webdav)
    case 300: *outValue=CVSTR("Multiple Choices") ; break;
    case 301: *outValue=CVSTR("Moved Permanently") ; break;
    case 302: *outValue=CVSTR("Moved Temporarily") ; break;
    case 303: *outValue=CVSTR("See Other") ; break;
    case 304: *outValue=CVSTR("Not Modified") ; break;
    case 305: *outValue=CVSTR("Use Proxy") ; break;
    case 307: *outValue=CVSTR("Temporary Redirect") ; break;
    case 400: *outValue=CVSTR("Bad Request") ; break;
    case 401: *outValue=CVSTR("Unauthorized") ; break;
    case 402: *outValue=CVSTR("Payment Required") ; break;
    case 403: *outValue=CVSTR("Forbidden") ; break;
    case 404: *outValue=CVSTR("Not Found") ; break;
    case 405: *outValue=CVSTR("Method Not Allowed") ; break;
    case 406: *outValue=CVSTR("Not Acceptable") ; break;
    case 407: *outValue=CVSTR("Proxy Authentication Required") ; break;
    case 408: *outValue=CVSTR("Request Time-out") ; break;
    case 409: *outValue=CVSTR("Conflict") ; break;
    case 410: *outValue=CVSTR("Gone") ; break;
    case 411: *outValue=CVSTR("Length Required") ; break;
    case 412: *outValue=CVSTR("Precondition Failed") ; break;
    case 413: *outValue=CVSTR("Request Entity Too Large") ; break;
    case 414: *outValue=CVSTR("Request-URI Too Long") ; break;
    case 415: *outValue=CVSTR("Unsupported Media Type") ; break;
    case 416: *outValue=CVSTR("Requested range unsatisfiable") ; break;
    case 417: *outValue=CVSTR("Expectation failed") ; break;
        //422   Unprocessable entity (webdav)
        //423   Locked (webdav)
        //424   Method failure (webdav)
    case 500: *outValue=CVSTR("Internal Server Error") ; break;
    case 501: *outValue=CVSTR("Not Implemented") ; break;
    case 502: *outValue=CVSTR("Bad Gateway") ; break;
    case 503: *outValue=CVSTR("Service Unavailable") ; break;
    case 504: *outValue=CVSTR("Gateway Time-out") ; break;
    case 505: *outValue=CVSTR("HTTP Version not supported") ; break;
        //507   Insufficient storage (webdav)
    case 509: *outValue=CVSTR("Bandwidth Limit Exceeded") ; break;

    default:
        *outValue=CVSTR("");
    }

    return VE_OK;
}


VError VXMLHttpRequest::GetResponseHeader(const VString& inKey, VString* outValue) const
{
    if(fReadyState==UNSENT || fReadyState==OPENED || fErrorFlag)
        return vThrowError(VE_XHRQ_INVALID_STATE_ERROR);

    if(fImpl==NULL)
        return vThrowError(VE_XHRQ_IMPL_FAIL_ERROR);

	bool found=fImpl->GetResponseHeaders().GetHeaderValue(inKey, *outValue);

    if(found)
        return VE_OK;

    return VE_XHRQ_NO_HEADER_ERROR;
}


VError VXMLHttpRequest::GetAllResponseHeaders(VString* outValue) const
{
    if(fReadyState==UNSENT || fReadyState==OPENED || fErrorFlag)
        return vThrowError(VE_XHRQ_INVALID_STATE_ERROR);

    if(fImpl==NULL)
        return vThrowError(VE_XHRQ_IMPL_FAIL_ERROR);

	std::vector<std::pair<VString, VString> > hdrs;

	fImpl->GetResponseHeaders().GetHeadersList(hdrs);

    outValue->Clear();
    
	std::vector<std::pair<VString, VString> >::const_iterator cit;

	for(cit=hdrs.begin() ; cit!=hdrs.end() ; ++cit)
    {
        //jmo - Skip case insensitive match of set-cookie2?
        if(cit->first.CompareToString(CVSTR("Set-Cookie"), false)==CR_EQUAL)
            continue;
        
        if(cit->first.CompareToString(CVSTR("Set-Cookie2"), false)==CR_EQUAL)
            continue;
        
		outValue->AppendString(cit->first).AppendUniChar(':').AppendString(cit->second).AppendUniChar('\n');
    }

    return VE_OK;
}


VError VXMLHttpRequest::SetResponseType(const VString& inValue)
{
	VError verr=VE_OK;

	if(inValue.CompareToString(CVSTR("text"), false /*case insensitive*/)==0)
		fResponseType=TEXT;
	else if(inValue.CompareToString(CVSTR("blob"), false /*case insensitive*/)==0)
		fResponseType=BLOB;
	else
	{
		fResponseType=TEXT;
		verr=VE_INVALID_PARAMETER;
	}

    return verr;
}


VError VXMLHttpRequest::GetResponseType(VString* outValue)
{
	VError verr=VE_OK;

	switch(fResponseType)
	{
	case TEXT:
		*outValue=CVSTR("text");
		break;
		
	case BLOB: 
		*outValue=CVSTR("blob");
		break;
	
	default:
		verr=VE_INVALID_PARAMETER;
	}

    return verr;
}


VError VXMLHttpRequest::GetResponseText(VString* outValue) const
{
    if(fReadyState!=LOADING && fReadyState!=DONE)
        return vThrowError(VE_XHRQ_INVALID_STATE_ERROR);

    if(fImpl==NULL)
        return vThrowError(VE_XHRQ_IMPL_FAIL_ERROR);

	if(outValue==NULL)
		return vThrowError(VE_INVALID_PARAMETER);				
	else
		outValue->Clear();

	fImpl->GetResponseBodyString(*outValue);

    return VE_OK;
}


VError VXMLHttpRequest::GetResponse(VJSValue& outValue) const
{
    if(fReadyState!=LOADING && fReadyState!=DONE)
        return vThrowError(VE_XHRQ_INVALID_STATE_ERROR);

    if(fImpl==NULL)
        return vThrowError(VE_XHRQ_IMPL_FAIL_ERROR);

	VError verr=VE_OK;

	switch(fResponseType)
	{
	case BLOB:
		{
			VString mimeType=GetContentType();

			VMemoryBuffer<> buf=fImpl->GetResponseBody();

            // data ptr is copied
            VJSBlobValue_Slice* slice=VJSBlobValue_Slice::Create(buf.GetDataPtr(), buf.GetDataSize(), mimeType);

            buf.ForgetData();	// buf and fImpl share the same data ptr ; fImpl is the owner
            
            outValue=VJSBlob::CreateInstance(outValue.GetContext(), slice);
        
            ReleaseRef(&slice);

			break;
		}

	case TEXT:
	default:
		{
			VString str;
			fImpl->GetResponseBodyString(str);

			outValue.SetString(str);

			break;
		}
	}

	return verr;
}


VError VXMLHttpRequest::GetResponseBinary(BinaryPtr& outPtr, VSize& outLen) const
{
	if(fReadyState!=LOADING && fReadyState!=DONE)
		return vThrowError(VE_XHRQ_INVALID_STATE_ERROR);

	if(fImpl==NULL)
		return vThrowError(VE_XHRQ_IMPL_FAIL_ERROR);

	VMemoryBuffer<> buf=fImpl->GetResponseBody();

	outPtr=reinterpret_cast<char*>(buf.GetDataPtr());
	outLen=buf.GetDataSize();

	buf.ForgetData();	// YT 17-Apr-2014 - WAK0087555 - buf and fImpl share the same data ptr ; fImpl is the owner

	return VE_OK;
}


VString VXMLHttpRequest::GetContentType(const VString& inDefaultType) const
{
	VString value;

	bool found=fImpl->GetResponseHeaders().GetHeaderValue(CVSTR("Content-Type"), value);

	return found ? value: inDefaultType;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// VJSXMLHttpRequest
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


VJSObject	VJSXMLHttpRequest::MakeConstructor( const VJSContext& inContext)
{
	return JS4DMakeConstructor( inContext, Class(), Construct);
}


void VJSXMLHttpRequest::GetDefinition(ClassDefinition& outDefinition)
{
    const JS4D::PropertyAttributes attDontWrite=JS4D::PropertyAttributeReadOnly;

    const JS4D::PropertyAttributes attDontEnumDelete=JS4D::PropertyAttributeDontEnum |
        JS4D::PropertyAttributeDontDelete;

    const JS4D::PropertyAttributes attDontWriteEnumDelete=attDontWrite | attDontEnumDelete;

    static inherited::StaticFunction functions[] =
        {
            { "open",                   js_callStaticFunction<_Open>,                   attDontWriteEnumDelete},
            { "send",                   js_callStaticFunction<_Send>,                   attDontWriteEnumDelete},
			{ "setClientCertificate",   js_callStaticFunction<_SetClientCertificate>,   attDontWriteEnumDelete},
            { "getResponseHeader",      js_callStaticFunction<_GetResponseHeader>,      attDontWriteEnumDelete},
            { "getAllResponseHeaders",  js_callStaticFunction<_GetAllResponseHeaders>,  attDontWriteEnumDelete},
            { "setRequestHeader",       js_callStaticFunction<_SetRequestHeader>,       attDontWriteEnumDelete},
			{ "setTimeout",		        js_callStaticFunction<_SetTimeout>,       		attDontWriteEnumDelete},
            { 0, 0, 0}
        };

    static inherited::StaticValue values[] =
        {
            { "responseText",       js_getProperty<_GetResponseText>, NULL,      attDontWriteEnumDelete},
            { "responseType",       js_getProperty<_GetResponseType>, js_setProperty<_SetResponseType>,      attDontEnumDelete},
            { "response",			js_getProperty<_GetResponse>,NULL,		    attDontWriteEnumDelete},
			{ "readyState",         js_getProperty<_GetReadyState>, NULL,        attDontWriteEnumDelete},
            { "status",             js_getProperty<_GetStatus>, NULL,            attDontWriteEnumDelete},
            { "statusText",         js_getProperty<_GetStatusText>, NULL,        attDontWriteEnumDelete},
            { "onreadystatechange", NULL, js_setProperty<_OnReadyStateChange>,   attDontEnumDelete},
            { 0, 0, 0, 0}
        };

    outDefinition.className         = "VXMLHttpRequest";
    outDefinition.staticFunctions   = functions;

    outDefinition.initialize        = js_initialize<Initialize>;
    outDefinition.finalize          = js_finalize<Finalize>;

    outDefinition.staticValues      = values;
}


void VJSXMLHttpRequest::Initialize(const VJSParms_initialize& inParms, VXMLHttpRequest* inXhr)
{
	RetainRefCountable(inXhr);
}


void VJSXMLHttpRequest::Finalize(const VJSParms_finalize& inParms, VXMLHttpRequest* inXhr)
{
	ReleaseRefCountable(&inXhr);
}

void VJSXMLHttpRequest::Construct(VJSParms_construct& ioParms)
{
    VXMLHttpRequest* xhr=new VXMLHttpRequest();

    if(xhr==NULL)
    {
        vThrowError(VE_XHRQ_IMPL_FAIL_ERROR);
        return;
    }
	

    VJSObject pData(ioParms.GetContext());
	bool resData;
    resData=ioParms.GetParamObject(1, pData);

	if(resData)
	{
        VJSValue hostValue=pData.GetProperty("host");
        
        if(hostValue.IsUndefined())
        {
            //We have an empty (or ill-formed) proxy, clear it, and don't use system proxy
            VError res=xhr->SetProxy(CVSTR(""), kBAD_PORT);

            if(res==VE_OK)
                res=xhr->SetSystemProxy(false);

            if(res!=VE_OK)
                vThrowError(res);
        }
        else
        {
            VString hostString;
            bool hostRes=hostValue.GetString(hostString);

            if(hostRes) //we have a proxy
            {
                //We may have a well-formed proxy, try to use it
                xbox_assert(hostString!=CVSTR("Undefined)"));
                
                VJSValue portValue=pData.GetProperty("port");
                sLONG portLong=0;
                bool portRes=portValue.GetLong(&portLong);

                portLong=(portRes && portLong>0) ? portLong: 80;

                //todo: user and passwd

                VError res=xhr->SetProxy(hostString, portLong);

                if(res!=VE_OK)
                    vThrowError(res);
            }
        }
	}
	VJSObject constructedObject = ConstructInstance(ioParms, xhr);

	ReleaseRefCountable(&xhr);

    ioParms.ReturnConstructedObject(constructedObject);
}

void VJSXMLHttpRequest::_Open(VJSParms_callStaticFunction& ioParms, VXMLHttpRequest* inXhr)
{
    VString pMethod;
    bool resMethod;
    resMethod=ioParms.GetStringParam(1, pMethod);

    VString pUrl;
    bool resUrl;    //jmo - todo: Verifier les longueurs max d'une chaine et d'une URL
    resUrl=ioParms.GetStringParam(2, pUrl);

    bool pAsync;
    bool resAsync;
    resAsync=ioParms.GetBoolParam(3, &pAsync);

    if(resMethod && resUrl)
    {
        VError res=inXhr->Open(pMethod, pUrl, pAsync);  //ASync is optional, we don't care if it's not set

        if(res!=VE_OK)
            vThrowError(res);
    }
    else
        vThrowError(VE_XHRQ_JS_BINDING_ERROR);
}


void VJSXMLHttpRequest::_SetClientCertificate(VJSParms_callStaticFunction& ioParms, VXMLHttpRequest* inXhr)
{
	VString pKeyPath;
    bool resKeyPath;
    resKeyPath=ioParms.GetStringParam(1, pKeyPath);
	
    VString pCertPath;
    bool resCertPath;
    resCertPath=ioParms.GetStringParam(2, pCertPath);
	
    if(resKeyPath && resCertPath)
    {
        VError res=inXhr->SetClientCertificate(pKeyPath, pCertPath);
		
        if(res!=VE_OK)
            vThrowError(res);
    }
    else
        vThrowError(VE_XHRQ_JS_BINDING_ERROR);	
}


void VJSXMLHttpRequest::_SetRequestHeader(VJSParms_callStaticFunction& ioParms, VXMLHttpRequest* inXhr)
{
    VString pHeader;
    VString pValue;

    bool resHeader=ioParms.GetStringParam(1, pHeader);
    bool resValue=ioParms.GetStringParam(2, pValue);

    if(resHeader && resValue)
    {
        VError res=inXhr->SetRequestHeader(pHeader, pValue);

        if(res!=VE_OK)
            vThrowError(res);
    }
    else
        vThrowError(VE_XHRQ_JS_BINDING_ERROR);
}


void VJSXMLHttpRequest::_SetTimeout(VJSParms_callStaticFunction& ioParms, VXMLHttpRequest* inXhr)
{
    sLONG pConnect=-1;
    sLONG pTotal=-1;

    bool resConnect=ioParms.GetLongParam(1, &pConnect);
    bool resTotal=ioParms.GetLongParam(2, &pTotal);

	pConnect = resConnect ? pConnect: 3000;
	pTotal = resTotal &&  pTotal > pConnect ? pTotal: 0;

	VError res=inXhr->SetTimeout(VLong(pConnect), VLong(pTotal));

	if(res!=VE_OK)
		vThrowError(res);
}


bool VJSXMLHttpRequest::_OnReadyStateChange(VJSParms_setProperty& ioParms, VXMLHttpRequest* inXhr)
{
    VJSObject pReceiver=ioParms.GetObject();
    VJSObject pHandler = ioParms.GetPropertyValue().GetObject((VJSException*)NULL);

    if(pHandler.IsFunction())
    {
        VError res=inXhr->OnReadyStateChange(pReceiver, pHandler);

        if(res!=VE_OK)
            vThrowError(res);
    }
    else
        vThrowError(VE_XHRQ_JS_BINDING_ERROR);

    return true;    //todo - mouai, voir ca de plus pres...
}


void VJSXMLHttpRequest::_Send(VJSParms_callStaticFunction& ioParms, VXMLHttpRequest* inXhr)
{
    bool resData;

    VError impl_err=VE_OK;

	VFile* file = ioParms.RetainFileParam(1, false);
	if (file != NULL)
	{
		if (file->Exists())
		{
			VMemoryBuffer<> buffer;
			VError err = file->GetContent( buffer);
			if (err == VE_OK)
			{
				VError res=inXhr->SendBinary(buffer.GetDataPtr(),(sLONG) buffer.GetDataSize(), &impl_err);
				uLONG code=ERRCODE_FROM_VERROR(impl_err);
				uLONG comp=COMPONENT_FROM_VERROR(impl_err);

				//We may have an implementation error which might be documented
				if(impl_err!=VE_OK)
					vThrowError(impl_err);

				//Now throw the more generic error
				vThrowError(res);
			}
		}
		file->Release();
	}
	else
	{
		VString pData;
		resData=ioParms.GetStringParam(1, pData);
		VError res=inXhr->Send(pData, &impl_err);

		if(res!=VE_OK)
		{
			uLONG code=ERRCODE_FROM_VERROR(impl_err);
			uLONG comp=COMPONENT_FROM_VERROR(impl_err);
				
			//We may have an implementation error which might be documented
			if(impl_err!=VE_OK)
				vThrowError(impl_err);

			//Now throw the more generic error
			vThrowError(res);
		}
	}
}


void VJSXMLHttpRequest::_GetReadyState(VJSParms_getProperty& ioParms, VXMLHttpRequest* inXhr)
{
	VLong value(0);
	VError res=inXhr->GetReadyState(&value);
    
    if(res==VE_OK)
        ioParms.ReturnVValue(value, false);
    else
        ioParms.ReturnVValue(VLong(0), false);
}


void VJSXMLHttpRequest::_GetStatus(VJSParms_getProperty& ioParms, VXMLHttpRequest* inXhr)
{
    StErrorContextInstaller ctx(false /*we drop errors*/, true /*from now on, popups are forbidden*/);
    
	VLong value(0);
	VError res=inXhr->GetStatus(&value);
    
    if(res==VE_OK)
        ioParms.ReturnVValue(value, false);
    else
        ioParms.ReturnVValue(VLong(0), false);
}


void VJSXMLHttpRequest::_GetStatusText(VJSParms_getProperty& ioParms, VXMLHttpRequest* inXhr)
{
    StErrorContextInstaller ctx(false /*we drop errors*/, true /*from now on, popups are forbidden*/);

    VString value;
    VError res=inXhr->GetStatusText(&value);
    
    if(res==VE_OK)
        ioParms.ReturnVValue(value, false);
    else
        ioParms.ReturnVValue(VString(), false);
}


void VJSXMLHttpRequest::_GetResponseHeader(VJSParms_callStaticFunction& ioParms, VXMLHttpRequest* inXhr)
{
    VString pHeader;
    bool resHeader;
    resHeader=ioParms.GetStringParam(1, pHeader);

    if(resHeader)
    {
        StErrorContextInstaller ctx(false /*we drop errors*/, true /*from now on, popups are forbidden*/);

        VString value;
        VError res;
        
        //jmo - Skip case insensitive match of set-cookie2?
        if(pHeader.CompareToString(CVSTR("Set-Cookie"), false)!=CR_EQUAL &&
           pHeader.CompareToString(CVSTR("Set-Cookie2"), false)!=CR_EQUAL)
        {
            res=inXhr->GetResponseHeader(pHeader, &value);

            if(res==VE_OK)
                ioParms.ReturnString(value);
            else
                ioParms.ReturnNullValue();
        }
        else
            ioParms.ReturnNullValue();
    }
    else
        ioParms.ReturnNullValue();
}


void VJSXMLHttpRequest::_GetAllResponseHeaders(VJSParms_callStaticFunction& ioParms, VXMLHttpRequest* inXhr)
{
    StErrorContextInstaller ctx(false /*we drop errors*/, true /*from now on, popups are forbidden*/);

    VString value;
    VError res=inXhr->GetAllResponseHeaders(&value);

    if(res==VE_OK)
        ioParms.ReturnString(value);
    else
        ioParms.ReturnVValue(VString(), false);
}


void VJSXMLHttpRequest::_GetResponseText(VJSParms_getProperty& ioParms, VXMLHttpRequest* inXhr)
{
    StErrorContextInstaller ctx(false /*we drop errors*/, true /*from now on, popups are forbidden*/);

    VString value;
    VError res=inXhr->GetResponseText(&value);
    
    if(res==VE_OK)
        ioParms.ReturnString(value);
    else
        ioParms.ReturnVValue(VString(), false);
}


bool VJSXMLHttpRequest::_SetResponseType(VJSParms_setProperty& ioParms, VXMLHttpRequest* inXhr)
{
	if (ioParms.IsValueString())
	{
		VString pType;

		if (ioParms.GetPropertyValue().GetString(pType))
		{
			VError res=inXhr->SetResponseType(pType);
				
			if(res==VE_OK)
				return true;
			else if(res==VE_INVALID_PARAMETER)
				vThrowError(VE_XHRQ_UNSUPPORTED_PARAMETER);
			else
				vThrowError(res);
		}
	}

	return false;
}


void VJSXMLHttpRequest::_GetResponseType(VJSParms_getProperty& ioParms, VXMLHttpRequest* inXhr)
{
    StErrorContextInstaller ctx(false /*we drop errors*/, true /*from now on, popups are forbidden*/);

	VString value;
	VError res=inXhr->GetResponseType(&value);

    if(res==VE_OK)
        ioParms.ReturnString(value);
    else
        ioParms.ReturnVValue(VString(), false);
}


void VJSXMLHttpRequest::_GetResponse(VJSParms_getProperty& ioParms, VXMLHttpRequest* inXhr)
{
    StErrorContextInstaller ctx(false /*we drop errors*/, true /*from now on, popups are forbidden*/);

	VJSValue value(ioParms.GetContext());
	VError res=inXhr->GetResponse(value);
    
    if(res==VE_OK)
        ioParms.ReturnValue(value);
    else
        ioParms.ReturnVValue(VString(), false);
}


#endif


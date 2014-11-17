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


#if !WITH_NATIVE_HTTP_BACKEND

#include <string.h>

#include "JavaScript/VJavaScript.h"
#include "CurlWrapper.h"
#include "VcURLXMLHttpRequest.h"

#include "VJSRuntime_blob.h"

USING_TOOLBOX_NAMESPACE

class cURLXMLHttpRequest::CWImpl
{
public :
    CWImpl() : fReq(NULL)/*, fMethod(CW::HttpRequest::CUSTOM)*/ {};
    ~CWImpl(){ if(fReq) delete fReq; }

    CW::HttpRequest*        fReq;
    //CW::HttpRequest::Method fMethod;
};





class MethodHelper
{
public :

    typedef CW::HttpRequest::Method Meth;

    MethodHelper(const XBOX::VString& inMethod) :
        fIsValid(false), fIsDangerous(false), fDoesSupportData(false), fMethod(CW::HttpRequest::CUSTOM)
    {
        XBOX::VString METHOD(inMethod);
        METHOD.ToUpperCase();

        if(METHOD.EqualToUSASCIICString("GET"))
            fMethod=CW::HttpRequest::GET, fIsValid=true;
        else if(METHOD.EqualToUSASCIICString("POST"))
            fMethod=CW::HttpRequest::POST, fIsValid=true;
        else if(METHOD.EqualToUSASCIICString("PUT"))
            fMethod=CW::HttpRequest::PUT, fDoesSupportData=true, fIsValid=true;
        else if(METHOD.EqualToUSASCIICString("HEAD"))
            fMethod=CW::HttpRequest::HEAD, fIsValid=true;
        else if(METHOD.EqualToUSASCIICString("DELETE"))
            fMethod=CW::HttpRequest::DELETE, fIsValid=true;
    }

    bool IsValid()          { return fIsValid; }
    bool IsDangerous()      { return fIsDangerous; }
    bool DoesSupportData()  { return fDoesSupportData; }
    Meth GetMethod()        { return fMethod; }


private :

    bool    fIsValid;
    bool    fIsDangerous;
    bool    fDoesSupportData;
    Meth    fMethod;
};



class UrlHelper
{
public :

    UrlHelper(const XBOX::VString inUrl) { fUrl.FromString(inUrl, false /*do not touch, pretend unencoded*/); }

	XBOX::VString   DropFragment()              { XBOX::VString tmp; fUrl.GetAbsoluteURL(tmp, false /*do not touch, want unencoded*/); return tmp; }
    bool            IsSupportedScheme()         { return true; }
    bool            IsUserInfoSet()             { return false; }
    bool            DoesSchemeSupportUserInfo() { return false; }

private :

    XBOX::VURL fUrl;
};



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// cURLXMLHttpRequest : Implements xhr logic on top of curl wrapper. Setters and getters return VE_OK and fill params on
//                  success or return an error and don't touch params on failure. Errors are not thrown.
//
// - VE_XHRQ_IMPL_FAIL_ERROR
// - VE_XHRQ_INVALID_STATE_ERROR
// - VE_XHRQ_SYNTAX_ERROR
// - VE_XHRQ_SECURITY_ERROR
// - VE_XHRQ_NOT_SUPPORTED_ERROR
// - VE_XHRQ_SEND_ERROR
// - VE_XHRQ_NO_HEADER_ERROR
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//cURLXMLHttpRequest::cURLXMLHttpRequest(const XBOX::VString& inProxy, uLONG inProxyPort) :
cURLXMLHttpRequest::cURLXMLHttpRequest() :
    fAsync(true),
    fReadyState(UNSENT),
    fStatus(0),
	fResponseType(TEXT),
	fSendFlag(false),
    fErrorFlag(false),
    fPort(kBAD_PORT),
	fUseSystemProxy(true),
    fChangeHandler(NULL)
{
    fCWImpl=new CWImpl();
};


cURLXMLHttpRequest::~cURLXMLHttpRequest()
{
    if(fChangeHandler)
        delete fChangeHandler;
    
    if(fCWImpl)
        delete fCWImpl;
}


XBOX::VError cURLXMLHttpRequest::Open(const XBOX::VString& inMethod, const XBOX::VString& inUrl, bool inAsync)
{
    if(fReadyState!=UNSENT)
        return VE_XHRQ_INVALID_STATE_ERROR;   //Doesn't follow the spec, but behave roughly as FireFox

    if(!fCWImpl)
        return VE_XHRQ_IMPL_FAIL_ERROR;


    MethodHelper mh(inMethod);

    if(!mh.IsValid())
        return VE_XHRQ_SYNTAX_ERROR;

    if(mh.IsDangerous())
        return VE_XHRQ_SECURITY_ERROR;


    UrlHelper uh(inUrl);

    if(!uh.IsSupportedScheme())
        return VE_XHRQ_NOT_SUPPORTED_ERROR;

    if(uh.IsUserInfoSet() && !uh.DoesSchemeSupportUserInfo())
        return VE_XHRQ_SYNTAX_ERROR;

    if(fCWImpl->fReq)
        delete fCWImpl->fReq;

    fCWImpl->fReq=new CW::HttpRequest(uh.DropFragment(), mh.GetMethod());
    if(!fCWImpl->fReq)
        return VE_XHRQ_IMPL_FAIL_ERROR;

	fReadyState=OPENED;

    if(fChangeHandler)
        fChangeHandler->Execute();

    return XBOX::VE_OK;
}


XBOX::VError cURLXMLHttpRequest::SetProxy(const XBOX::VString& inHost, const uLONG inPort)
{
    if(fReadyState!=UNSENT && fReadyState!=OPENED)
        return VE_XHRQ_INVALID_STATE_ERROR;

    fProxy=inHost;
    fPort=inPort;

    return XBOX::VE_OK;
}


XBOX::VError cURLXMLHttpRequest::SetSystemProxy(bool inUseSystemProxy)
{
	fUseSystemProxy=inUseSystemProxy;

    return XBOX::VE_OK;
}


XBOX::VError cURLXMLHttpRequest::SetUserInfos(const XBOX::VString& inUser, const XBOX::VString& inPasswd, bool inAllowBasic)
{
    if(fReadyState!=UNSENT && fReadyState!=OPENED)
        return VE_XHRQ_INVALID_STATE_ERROR; 
	
    if(!fCWImpl)
        return VE_XHRQ_IMPL_FAIL_ERROR;
	
//	if(inUser.IsEmpty() || inPasswd.IsEmpty())
//		return VE_XHRQ_SECURITY_ERROR;
	
	bool res=fCWImpl->fReq->SetUserInfos(inUser, inPasswd, inAllowBasic);

	if(!res)
        return VE_XHRQ_IMPL_FAIL_ERROR;
	
    return XBOX::VE_OK;
}


XBOX::VError cURLXMLHttpRequest::SetClientCertificate(const XBOX::VString& inKeyPath, const XBOX::VString& inCertPath)
{
	if(fReadyState!=UNSENT && fReadyState!=OPENED)
        return VE_XHRQ_INVALID_STATE_ERROR; 
	
    if(!fCWImpl)
        return VE_XHRQ_IMPL_FAIL_ERROR;
	
	if(inKeyPath.IsEmpty() || inCertPath.IsEmpty())
		return VE_XHRQ_SECURITY_ERROR;
	
	bool res=fCWImpl->fReq->SetClientCertificate(inKeyPath, inCertPath);
	
	if(!res)
        return VE_XHRQ_IMPL_FAIL_ERROR;
	
    return XBOX::VE_OK;
}


XBOX::VError cURLXMLHttpRequest::SetRequestHeader(const XBOX::VString& inKey, const XBOX::VString& inValue)
{
    if(fReadyState!=OPENED)
        return VE_XHRQ_INVALID_STATE_ERROR;

    if(!fCWImpl || !fCWImpl->fReq)
        return VE_XHRQ_IMPL_FAIL_ERROR;


    bool res=fCWImpl->fReq->SetRequestHeader(inKey, inValue);

    if(!res)
        return VE_XHRQ_IMPL_FAIL_ERROR;

    return XBOX::VE_OK;
}


XBOX::VError cURLXMLHttpRequest::SetTimeout(XBOX::VLong inConnectMs, XBOX::VLong inTotalMs)
{
    if(fReadyState!=OPENED)
        return VE_XHRQ_INVALID_STATE_ERROR;

    if(!fCWImpl || !fCWImpl->fReq)
        return VE_XHRQ_IMPL_FAIL_ERROR;


    bool res=fCWImpl->fReq->SetTimeout(inConnectMs, inTotalMs);

    if(!res)
        return VE_XHRQ_IMPL_FAIL_ERROR;

    return XBOX::VE_OK;
}


XBOX::VError cURLXMLHttpRequest::OnReadyStateChange(XBOX::VJSObject inReceiver, const XBOX::VJSObject& inHandler)
{
    //No need for state tests here...

    if(fChangeHandler)
        delete fChangeHandler;

    fChangeHandler=NULL;

    fChangeHandler=new Command(inReceiver, inHandler);
    if(!fChangeHandler)
        return VE_XHRQ_IMPL_FAIL_ERROR;

    return XBOX::VE_OK;
}



XBOX::VError cURLXMLHttpRequest::SendBinary(const void* data, sLONG datalen, XBOX::VError* outImplErr)
{
	if(fReadyState!=OPENED)
		return VE_XHRQ_INVALID_STATE_ERROR;

	if(!fCWImpl || !fCWImpl->fReq)
		return VE_XHRQ_IMPL_FAIL_ERROR;

	fErrorFlag=false;

	if(!fProxy.IsEmpty())
		fCWImpl->fReq->SetProxy(fProxy, fPort);
	else if (fUseSystemProxy)
		fCWImpl->fReq->SetSystemProxy();

	if(datalen != 0 && data != nil)
	{
		fCWImpl->fReq->SetBinaryData(data, datalen);
	}

	//if(fCWImpl->fMethod==CW::HttpRequest::GET || fCWImpl->fMethod==CW::HttpRequest::POST || fCWImpl->fMethod==CW::HttpRequest::HEAD || fCWImpl->fMethod==CW::HttpRequest::PUT)
	//{
	//parametrer la req de facon appropriee
	//}

	if(!fUser.IsEmpty())
	{
		//parametrer la req de facon appropriee

		if(!fPasswd.IsEmpty())
		{
			//parametrer la req de facon appropriee
		}
	}

	bool res=fCWImpl->fReq->Perform(outImplErr);

	if(res)
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

	if(!res)
		return VE_XHRQ_SEND_ERROR;

	return XBOX::VE_OK;
}


XBOX::VError cURLXMLHttpRequest::Send(const XBOX::VString& inData, XBOX::VError* outImplErr)
{
    if(fReadyState!=OPENED)
        return VE_XHRQ_INVALID_STATE_ERROR;

    if(!fCWImpl || !fCWImpl->fReq)
        return VE_XHRQ_IMPL_FAIL_ERROR;

    fErrorFlag=false;

    if(!fProxy.IsEmpty())
        fCWImpl->fReq->SetProxy(fProxy, fPort);
	else if (fUseSystemProxy)
		fCWImpl->fReq->SetSystemProxy();

    if(/*(fCWImpl->fMethod==CW::HttpRequest::POST || fCWImpl->fMethod==CW::HttpRequest::PUT) && */!inData.IsEmpty())
    {
        fCWImpl->fReq->SetData(inData);
    }

    //if(fCWImpl->fMethod==CW::HttpRequest::GET || fCWImpl->fMethod==CW::HttpRequest::POST || fCWImpl->fMethod==CW::HttpRequest::HEAD || fCWImpl->fMethod==CW::HttpRequest::PUT)
    //{
    //parametrer la req de facon appropriee
    //}

    if(!fUser.IsEmpty())
    {
        //parametrer la req de facon appropriee

        if(!fPasswd.IsEmpty())
        {
            //parametrer la req de facon appropriee
        }
    }

    bool res=fCWImpl->fReq->Perform(outImplErr);

    if(res)
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

    if(!res)
        return VE_XHRQ_SEND_ERROR;

    return XBOX::VE_OK;
}


XBOX::VError cURLXMLHttpRequest::Abort()
{
    //No need for state tests here...

    fErrorFlag=true;
    fReadyState=UNSENT;

    return XBOX::VE_OK;
}


XBOX::VError cURLXMLHttpRequest::GetReadyState(XBOX::VLong* outValue) const
{
    //No need for state tests here...

    *outValue=(uLONG)(fReadyState);

    return XBOX::VE_OK;
}


XBOX::VError cURLXMLHttpRequest::GetStatus(XBOX::VLong* outValue) const
{
    if(fReadyState!=UNSENT && fReadyState!=OPENED && !fErrorFlag)
    {
        if(!fCWImpl || !fCWImpl->fReq)
            return VE_XHRQ_IMPL_FAIL_ERROR;

        uLONG code=0;
        bool hasCode=fCWImpl->fReq->HasValidResponseCode(&code);

        if(hasCode)
        {
            *outValue=(uLONG)(code);
            return XBOX::VE_OK;
        }

        return VE_XHRQ_NO_RESPONSE_CODE_ERROR;
    }

    return VE_XHRQ_INVALID_STATE_ERROR;
}


XBOX::VError cURLXMLHttpRequest::GetStatusText(XBOX::VString* outValue) const
{
    XBOX::VLong code;
    XBOX::VError res=GetStatus(&code);

    if(res!=XBOX::VE_OK)
        return res;

    switch((uLONG)(code))
    {
    case 100 : *outValue=XBOX::VString("Continue") ; break;
    case 101 : *outValue=XBOX::VString("Switching Protocols") ; break;
        //102   Processing (webdav)
    case 200 : *outValue=XBOX::VString("OK") ; break;
    case 201 : *outValue=XBOX::VString("Created") ; break;
    case 202 : *outValue=XBOX::VString("Accepted") ; break;
    case 203 : *outValue=XBOX::VString("Non-Authoritative Information") ; break;
    case 204 : *outValue=XBOX::VString("No Content") ; break;
    case 205 : *outValue=XBOX::VString("Reset Content") ; break;
    case 206 : *outValue=XBOX::VString("Partial Content") ; break;
        //207   Multi-Status (webdav)
        //210   Content Different (webdav)
    case 300 : *outValue=XBOX::VString("Multiple Choices") ; break;
    case 301 : *outValue=XBOX::VString("Moved Permanently") ; break;
    case 302 : *outValue=XBOX::VString("Moved Temporarily") ; break;
    case 303 : *outValue=XBOX::VString("See Other") ; break;
    case 304 : *outValue=XBOX::VString("Not Modified") ; break;
    case 305 : *outValue=XBOX::VString("Use Proxy") ; break;
    case 307 : *outValue=XBOX::VString("Temporary Redirect") ; break;
    case 400 : *outValue=XBOX::VString("Bad Request") ; break;
    case 401 : *outValue=XBOX::VString("Unauthorized") ; break;
    case 402 : *outValue=XBOX::VString("Payment Required") ; break;
    case 403 : *outValue=XBOX::VString("Forbidden") ; break;
    case 404 : *outValue=XBOX::VString("Not Found") ; break;
    case 405 : *outValue=XBOX::VString("Method Not Allowed") ; break;
    case 406 : *outValue=XBOX::VString("Not Acceptable") ; break;
    case 407 : *outValue=XBOX::VString("Proxy Authentication Required") ; break;
    case 408 : *outValue=XBOX::VString("Request Time-out") ; break;
    case 409 : *outValue=XBOX::VString("Conflict") ; break;
    case 410 : *outValue=XBOX::VString("Gone") ; break;
    case 411 : *outValue=XBOX::VString("Length Required") ; break;
    case 412 : *outValue=XBOX::VString("Precondition Failed") ; break;
    case 413 : *outValue=XBOX::VString("Request Entity Too Large") ; break;
    case 414 : *outValue=XBOX::VString("Request-URI Too Long") ; break;
    case 415 : *outValue=XBOX::VString("Unsupported Media Type") ; break;
    case 416 : *outValue=XBOX::VString("Requested range unsatisfiable") ; break;
    case 417 : *outValue=XBOX::VString("Expectation failed") ; break;
        //422   Unprocessable entity (webdav)
        //423   Locked (webdav)
        //424   Method failure (webdav)
    case 500 : *outValue=XBOX::VString("Internal Server Error") ; break;
    case 501 : *outValue=XBOX::VString("Not Implemented") ; break;
    case 502 : *outValue=XBOX::VString("Bad Gateway") ; break;
    case 503 : *outValue=XBOX::VString("Service Unavailable") ; break;
    case 504 : *outValue=XBOX::VString("Gateway Time-out") ; break;
    case 505 : *outValue=XBOX::VString("HTTP Version not supported") ; break;
        //507   Insufficient storage (webdav)
    case 509 : *outValue=XBOX::VString("Bandwidth Limit Exceeded") ; break;

    default :
        *outValue=XBOX::VString("");
    }

    return XBOX::VE_OK;
}


XBOX::VError cURLXMLHttpRequest::GetResponseHeader(const XBOX::VString& inKey, XBOX::VString* outValue) const
{
    if(fReadyState==UNSENT || fReadyState==OPENED || fErrorFlag)
        return VE_XHRQ_INVALID_STATE_ERROR;

    if(!fCWImpl || !fCWImpl->fReq)
        return VE_XHRQ_IMPL_FAIL_ERROR;

    bool found=fCWImpl->fReq->GetResponseHeader(inKey, outValue);

    if(found)
        return XBOX::VE_OK;

    return VE_XHRQ_NO_HEADER_ERROR;
}


XBOX::VError cURLXMLHttpRequest::GetAllResponseHeaders(XBOX::VString* outValue) const
{
    if(fReadyState==UNSENT || fReadyState==OPENED || fErrorFlag)
        return VE_XHRQ_INVALID_STATE_ERROR;

    if(!fCWImpl || !fCWImpl->fReq)
        return VE_XHRQ_IMPL_FAIL_ERROR;

    bool found=fCWImpl->fReq->GetAllResponseHeaders(outValue);

    if(found)
        return XBOX::VE_OK;

    return VE_XHRQ_NO_HEADER_ERROR;
}


XBOX::VError cURLXMLHttpRequest::SetResponseType(const XBOX::VString& inValue)
{
	XBOX::VError verr=XBOX::VE_OK;

	if(inValue.CompareToString(XBOX::VString("text"), false /*case insensitive*/)==0)
		fResponseType=TEXT;
	else if(inValue.CompareToString(XBOX::VString("blob"), false /*case insensitive*/)==0)
		fResponseType=BLOB;
	else
	{
		fResponseType=TEXT;
		verr=XBOX::VE_INVALID_PARAMETER;
	}

    return verr;
}


XBOX::VError cURLXMLHttpRequest::GetResponseType(XBOX::VString* outValue)
{
	XBOX::VError verr=XBOX::VE_OK;

	switch(fResponseType)
	{
	case TEXT :
		*outValue=XBOX::VString("text");
		break;
		
	case BLOB : 
		*outValue=XBOX::VString("blob");
		break;
	
	default :
		verr=XBOX::VE_INVALID_PARAMETER;
	}

    return verr;
}


XBOX::VError cURLXMLHttpRequest::GetResponseText(XBOX::VString* outValue) const
{
    if(fReadyState!=LOADING && fReadyState!=DONE)
        return VE_XHRQ_INVALID_STATE_ERROR;

    if(!fCWImpl || !fCWImpl->fReq)
        return VE_XHRQ_IMPL_FAIL_ERROR;

    const char* ptr=NULL;
    int len=-1;

    ptr=fCWImpl->fReq->GetContentCPointer();
    len=fCWImpl->fReq->GetContentLength();

	if(ptr==NULL || len==0)
		return XBOX::VE_OK;
	
	if(outValue==NULL)
		return XBOX::VE_INVALID_PARAMETER;				
	else
		outValue->Clear();

    XBOX::CharSet cs=GetCharSetFromHeaders();
	
	XBOX::VConstPtrStream stream(ptr, len);
	stream.OpenReading();
	stream.GuessCharSetFromLeadingBytes(cs!=XBOX::VTC_UNKNOWN ? cs : XBOX::VTC_UTF_8);
	stream.GetText(*outValue);
	stream.CloseReading();

    return XBOX::VE_OK;
}


XBOX::VError cURLXMLHttpRequest::GetResponse(XBOX::VJSValue& outValue) const
{
    if(fReadyState!=LOADING && fReadyState!=DONE)
        return VE_XHRQ_INVALID_STATE_ERROR;

    if(!fCWImpl || !fCWImpl->fReq)
        return VE_XHRQ_IMPL_FAIL_ERROR;

	XBOX::VError verr=XBOX::VE_OK;

    const char* ptr=NULL;
    int len=-1;

    ptr=fCWImpl->fReq->GetContentCPointer();
	len=fCWImpl->fReq->GetContentLength();

	if(ptr==NULL || len==0)
		return verr;

	switch(fResponseType)
	{
	case BLOB :
		{
			XBOX::VString mimeType=GetContentType();
			XBOX::VJSBlobValue_Slice* slice=XBOX::VJSBlobValue_Slice::Create(ptr, len, mimeType);
			outValue = XBOX::VJSBlob::CreateInstance(outValue.GetContext(), slice);
			ReleaseRefCountable(&slice);

			break;
		}

	case TEXT :
	default :
		{
			XBOX::VString string;
			XBOX::CharSet cs=GetCharSetFromHeaders();
	
			XBOX::VConstPtrStream stream(ptr, len);
			stream.OpenReading();
			stream.GuessCharSetFromLeadingBytes(cs!=XBOX::VTC_UNKNOWN ? cs : XBOX::VTC_UTF_8);
	
			stream.GetText(string);
			outValue.SetString(string);
			stream.CloseReading();
	
			break;
		}
	}

	return verr;
}


XBOX::VError cURLXMLHttpRequest::GetResponseBinary(BinaryPtr& outPtr, XBOX::VSize& outLen) const
{
	if(fReadyState!=LOADING && fReadyState!=DONE)
		return VE_XHRQ_INVALID_STATE_ERROR;

	if(!fCWImpl || !fCWImpl->fReq)
		return VE_XHRQ_IMPL_FAIL_ERROR;

	outPtr = fCWImpl->fReq->GetContentCPointer();
	outLen = (XBOX::VSize)fCWImpl->fReq->GetContentLength();

	return XBOX::VE_OK;
}


XBOX::CharSet cURLXMLHttpRequest::GetCharSetFromHeaders() const
{
    const char* c_header=fCWImpl->fReq->GetResponseHeader("Content-Type");

    if(c_header)
    {
        const char* pos=strstr(c_header, "charset=");

        if(pos && strlen(pos)>sizeof("charset="))
        {
            pos+=sizeof("charset=")-1;
            sLONG len=(sLONG) strlen(pos);

            XBOX::VString charSetName(pos, len, XBOX::VTC_US_ASCII);

            if(!charSetName.IsEmpty())
            {
                XBOX::CharSet cs=XBOX::VTextConverters::Get()->GetCharSetFromName(charSetName);
                    
                if(cs!=XBOX::VTC_UNKNOWN)
                    return cs;
            }
        }
    }
	
	return XBOX::VTC_UNKNOWN;
}


XBOX::VString cURLXMLHttpRequest::GetContentType(const XBOX::VString inDefaultType) const
{
    const char* c_header=fCWImpl->fReq->GetResponseHeader("Content-Type");

    if(c_header)
    {
		//On parse un header qui a cette forme :
		//text/javascript; charset=utf-8

		const char* start=NULL;

		for(start=c_header ; *start!='\0' && (*start==':' || *start==' ' || *start=='	') ; start++)
        {
        }

        const char* stop=NULL;
		
		for(stop=start ; *stop!='\0' && *stop!=';' && *stop!=' ' && *stop!='	' ; stop++)
        {
        }
		
		sLONG len=stop-start;

		XBOX::VString type(start, len, XBOX::VTC_US_ASCII);

		return type;
    }
	
	return inDefaultType;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// VJScURLXMLHttpRequest
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


XBOX::VJSObject	VJScURLXMLHttpRequest::MakeConstructor( const XBOX::VJSContext& inContext)
{
	return JS4DMakeConstructor( inContext, Class(), Construct);
}


void VJScURLXMLHttpRequest::GetDefinition(ClassDefinition& outDefinition)
{
    const XBOX::JS4D::PropertyAttributes attDontWrite=XBOX::JS4D::PropertyAttributeReadOnly;

    const XBOX::JS4D::PropertyAttributes attDontEnumDelete=XBOX::JS4D::PropertyAttributeDontEnum |
        XBOX::JS4D::PropertyAttributeDontDelete;

    const XBOX::JS4D::PropertyAttributes attDontWriteEnumDelete=attDontWrite | attDontEnumDelete;

    static inherited::StaticFunction functions[] =
        {
            { "open",                   js_callStaticFunction<_Open>,                   attDontWriteEnumDelete},
            { "send",                   js_callStaticFunction<_Send>,                   attDontWriteEnumDelete},
            //{ "setProxy",             js_callStaticFunction<_SetProxy>,               attDontWriteEnumDelete},
			{ "setClientCertificate",   js_callStaticFunction<_SetClientCertificate>,   attDontWriteEnumDelete},
            { "getResponseHeader",      js_callStaticFunction<_GetResponseHeader>,      attDontWriteEnumDelete},
            { "getAllResponseHeaders",  js_callStaticFunction<_GetAllResponseHeaders>,  attDontWriteEnumDelete},
            { "setRequestHeader",       js_callStaticFunction<_SetRequestHeader>,       attDontWriteEnumDelete},
			{ "setTimeout",		        js_callStaticFunction<_SetTimeout>,       		attDontWriteEnumDelete},
            { 0, 0, 0}
        };

    static inherited::StaticValue values[] =
        {
            { "responseText",       js_getProperty<_GetResponseText>, nil,      attDontWriteEnumDelete},
            { "responseType",       js_getProperty<_GetResponseType>, js_setProperty<_SetResponseType>,      attDontEnumDelete},
            { "response",			js_getProperty<_GetResponse>,nil,		    attDontWriteEnumDelete},
			{ "readyState",         js_getProperty<_GetReadyState>, nil,        attDontWriteEnumDelete},
            { "status",             js_getProperty<_GetStatus>, nil,            attDontWriteEnumDelete},
            { "statusText",         js_getProperty<_GetStatusText>, nil,        attDontWriteEnumDelete},
            { "onreadystatechange", nil, js_setProperty<_OnReadyStateChange>,   attDontEnumDelete},
            { 0, 0, 0, 0}
        };

    outDefinition.className         = "cURLXMLHttpRequest";
    outDefinition.staticFunctions   = functions;

    outDefinition.initialize        = js_initialize<Initialize>;
    outDefinition.finalize          = js_finalize<Finalize>;

    outDefinition.staticValues      = values;
}


void VJScURLXMLHttpRequest::Initialize(const XBOX::VJSParms_initialize& inParms, cURLXMLHttpRequest* inXhr)
{
    //Alloc and init is done in Construct();
}


void VJScURLXMLHttpRequest::Finalize(const XBOX::VJSParms_finalize& inParms, cURLXMLHttpRequest* inXhr)
{
        delete inXhr;
}

void VJScURLXMLHttpRequest::Construct(XBOX::VJSParms_construct& ioParms)
{
    cURLXMLHttpRequest* xhr=new cURLXMLHttpRequest();

    if(!xhr)
    {
        XBOX::vThrowError(VE_XHRQ_IMPL_FAIL_ERROR);
        return;
    }
	

    XBOX::VJSObject pData(ioParms.GetContext());
	bool resData;
    resData=ioParms.GetParamObject(1, pData);

	if(resData)
	{
		XBOX::VJSValue hostValue=pData.GetProperty("host");
		XBOX::VString hostString;
		bool hostRes=hostValue.GetString(hostString);

		if(hostRes) //we have a proxy
		{
			XBOX::VJSValue portValue=pData.GetProperty("port");
			sLONG portLong=0;
			bool portRes=portValue.GetLong(&portLong);

			portLong=(portRes && portLong>0) ? portLong : 80;

			//todo : user and passwd

			XBOX::VError res=xhr->SetProxy(hostString, portLong);

			if(res!=XBOX::VE_OK)
				XBOX::vThrowError(res);
		}
	}

    ioParms.ReturnConstructedObject<VJScURLXMLHttpRequest>( xhr);

}


// void VJScURLXMLHttpRequest::CallAsFunction(XBOX::VJSParms_callAsFunction& ioParms)
// {
//     XBOX::VError err=XBOX::VE_OK;
// }


void VJScURLXMLHttpRequest::_Open(XBOX::VJSParms_callStaticFunction& ioParms, cURLXMLHttpRequest* inXhr)
{
    XBOX::VString pMethod;
    bool resMethod;
    resMethod=ioParms.GetStringParam(1, pMethod);

    XBOX::VString pUrl;
    bool resUrl;    //jmo - todo : Verifier les longueurs max d'une chaine et d'une URL
    resUrl=ioParms.GetStringParam(2, pUrl);

    bool pAsync;
    bool resAsync;
    resAsync=ioParms.GetBoolParam(3, &pAsync);

    if(resMethod && resUrl && inXhr)
    {
        XBOX::VError res=inXhr->Open(pMethod, pUrl, pAsync);  //ASync is optional, we don't care if it's not set

        if(res!=XBOX::VE_OK)
            XBOX::vThrowError(res);
    }
    else
        XBOX::vThrowError(VE_XHRQ_JS_BINDING_ERROR);
}


// void VJScURLXMLHttpRequest::_SetProxy(XBOX::VJSParms_callStaticFunction& ioParms, cURLXMLHttpRequest* inXhr)
// {
//     XBOX::VJSObject pData(ioParms.GetContext());
//     ioParms.GetParamObject(1, pData);

//     XBOX::VJSValue hostValue=pData.GetProperty("host", NULL);
//     XBOX::VJSValue portValue=pData.GetProperty("port", NULL);

//     if(hostValue.IsString() && portValue.IsNumber() && inXhr)
//     {
//         XBOX::VString hostString;
//         bool hostRes=hostValue.GetString(hostString, NULL);

//         sLONG portLong=0;
//         bool portRes=portValue.GetLong(&portLong, NULL);

//         if(hostRes && portRes && portLong>0 && inXhr)
//         {
//             XBOX::VError res=inXhr->SetProxy(hostString, portLong);

//             if(res!=XBOX::VE_OK)
//                 XBOX::vThrowError(res);
//         }
//     }
//     else
//         XBOX::vThrowError(VE_XHRQ_JS_BINDING_ERROR);
// }


void VJScURLXMLHttpRequest::_SetClientCertificate(XBOX::VJSParms_callStaticFunction& ioParms, cURLXMLHttpRequest* inXhr)
{
	XBOX::VString pKeyPath;
    bool resKeyPath;
    resKeyPath=ioParms.GetStringParam(1, pKeyPath);
	
    XBOX::VString pCertPath;
    bool resCertPath;
    resCertPath=ioParms.GetStringParam(2, pCertPath);
	
    if(resKeyPath && resCertPath && inXhr)
    {
        XBOX::VError res=inXhr->SetClientCertificate(pKeyPath, pCertPath);
		
        if(res!=XBOX::VE_OK)
            XBOX::vThrowError(res);
    }
    else
        XBOX::vThrowError(VE_XHRQ_JS_BINDING_ERROR);	
}


void VJScURLXMLHttpRequest::_SetRequestHeader(XBOX::VJSParms_callStaticFunction& ioParms, cURLXMLHttpRequest* inXhr)
{
    XBOX::VString pHeader;
    XBOX::VString pValue;

    bool resHeader=ioParms.GetStringParam(1, pHeader);
    bool resValue=ioParms.GetStringParam(2, pValue);

    if(resHeader && resValue && inXhr)
    {
        XBOX::VError res=inXhr->SetRequestHeader(pHeader, pValue);

        if(res!=XBOX::VE_OK)
            XBOX::vThrowError(res);
    }
    else
        XBOX::vThrowError(VE_XHRQ_JS_BINDING_ERROR);
}


void VJScURLXMLHttpRequest::_SetTimeout(XBOX::VJSParms_callStaticFunction& ioParms, cURLXMLHttpRequest* inXhr)
{
    sLONG pConnect=-1;
    sLONG pTotal=-1;

    bool resConnect=ioParms.GetLongParam(1, &pConnect);
    bool resTotal=ioParms.GetLongParam(2, &pTotal);

	pConnect = resConnect ? pConnect : 3000;
	pTotal = resTotal &&  pTotal > pConnect ? pTotal : 0;

    if(inXhr)
	{
        XBOX::VError res=inXhr->SetTimeout(XBOX::VLong(pConnect), XBOX::VLong(pTotal));

        if(res!=XBOX::VE_OK)
            XBOX::vThrowError(res);
	}
    else
        XBOX::vThrowError(VE_XHRQ_JS_BINDING_ERROR);
}


bool VJScURLXMLHttpRequest::_OnReadyStateChange(XBOX::VJSParms_setProperty& ioParms, cURLXMLHttpRequest* inXhr)
{
    XBOX::VJSObject pReceiver=ioParms.GetObject();
    XBOX::VJSObject pHandler = ioParms.GetPropertyValue().GetObject((VJSException*)NULL);

    if(pHandler.IsFunction() && inXhr)
    {
        XBOX::VError res=inXhr->OnReadyStateChange(pReceiver, pHandler);

        if(res!=XBOX::VE_OK)
            XBOX::vThrowError(res);
    }
    else
        XBOX::vThrowError(VE_XHRQ_JS_BINDING_ERROR);

    return true;    //todo - mouai, voir ca de plus pres...
}


void VJScURLXMLHttpRequest::_Send(XBOX::VJSParms_callStaticFunction& ioParms, cURLXMLHttpRequest* inXhr)
{
    bool resData;

    if(inXhr)   //Data is optional, we don't care if it's not set
    {
        XBOX::VError impl_err=XBOX::VE_OK;

		XBOX::VFile* file = ioParms.RetainFileParam(1, false);
		if (file != nil)
		{
			if (file->Exists())
			{
				XBOX::VMemoryBuffer<> buffer;
				XBOX::VError err = file->GetContent( buffer);
				if (err == XBOX::VE_OK)
				{
					XBOX::VError res=inXhr->SendBinary(buffer.GetDataPtr(),(sLONG) buffer.GetDataSize(), &impl_err);
					uLONG code=ERRCODE_FROM_VERROR(impl_err);
					uLONG comp=COMPONENT_FROM_VERROR(impl_err);

					//We may have an implementation error which might be documented
					if(impl_err!=XBOX::VE_OK)
						XBOX::vThrowError(impl_err);

					//Now throw the more generic error
					XBOX::vThrowError(res);
				}
			}
			file->Release();
		}
		else
		{
			XBOX::VString pData;
			resData=ioParms.GetStringParam(1, pData);
			XBOX::VError res=inXhr->Send(pData, &impl_err);

			if(res!=XBOX::VE_OK)
			{
				uLONG code=ERRCODE_FROM_VERROR(impl_err);
				uLONG comp=COMPONENT_FROM_VERROR(impl_err);
				
				//We may have an implementation error which might be documented
				if(impl_err!=XBOX::VE_OK)
				   XBOX::vThrowError(impl_err);

				//Now throw the more generic error
				XBOX::vThrowError(res);
			}
		}
    }
    else
        XBOX::vThrowError(VE_XHRQ_JS_BINDING_ERROR);
}


void VJScURLXMLHttpRequest::_Abort(XBOX::VJSParms_callStaticFunction& ioParms, cURLXMLHttpRequest* inXhr)
{
    if(inXhr)
    {
        XBOX::VError res=inXhr->Abort();

        if(res!=XBOX::VE_OK)
            XBOX::vThrowError(res);
    }
    else
        XBOX::vThrowError(VE_XHRQ_JS_BINDING_ERROR);
}


void VJScURLXMLHttpRequest::_GetReadyState(XBOX::VJSParms_getProperty& ioParms, cURLXMLHttpRequest* inXhr)
{
    if(inXhr)
    {
        XBOX::VError res;
        XBOX::VLong value;

        res=inXhr->GetReadyState(&value); //Should never fails...
        ioParms.ReturnVValue(value, false);

        if(res!=XBOX::VE_OK)
            XBOX::vThrowError(res);
    }
    else
        XBOX::vThrowError(VE_XHRQ_JS_BINDING_ERROR);
}


void VJScURLXMLHttpRequest::_GetStatus(XBOX::VJSParms_getProperty& ioParms, cURLXMLHttpRequest* inXhr)
{
    if(inXhr)
    {
        XBOX::VLong value(0);
        XBOX::VError res=inXhr->GetStatus(&value);
        ioParms.ReturnVValue(value, false);

        //Don't throw any error here, but returns 0.
    }
    else
        XBOX::vThrowError(VE_XHRQ_JS_BINDING_ERROR);
}


void VJScURLXMLHttpRequest::_GetStatusText(XBOX::VJSParms_getProperty& ioParms, cURLXMLHttpRequest* inXhr)
{
    if(inXhr)
    {
        XBOX::VString value;
        XBOX::VError res=inXhr->GetStatusText(&value);
        ioParms.ReturnVValue(value, false);

        //Don't throw any error here, but returns an empty string.
    }
    else
        XBOX::vThrowError(VE_XHRQ_JS_BINDING_ERROR);
}


void VJScURLXMLHttpRequest::_GetResponseHeader(XBOX::VJSParms_callStaticFunction& ioParms, cURLXMLHttpRequest* inXhr)
{
    XBOX::VString pHeader;
    bool resHeader;
    resHeader=ioParms.GetStringParam(1, pHeader);

    if(resHeader && inXhr)
    {
        XBOX::VString value;
        XBOX::VError res;

        res=inXhr->GetResponseHeader(pHeader, &value);

        //Don't throw any error here, but returns null

        if(res==XBOX::VE_OK)
            ioParms.ReturnString(value);
        else
            ioParms.ReturnNullValue();
    }
    else
        XBOX::vThrowError(VE_XHRQ_JS_BINDING_ERROR);
}


void VJScURLXMLHttpRequest::_GetAllResponseHeaders(XBOX::VJSParms_callStaticFunction& ioParms, cURLXMLHttpRequest* inXhr)
{
    if(inXhr)
    {
        XBOX::VString value;
        XBOX::VError res=inXhr->GetAllResponseHeaders(&value);

        //returns an empty string on error
        ioParms.ReturnString(value);
    }
    else
        XBOX::vThrowError(VE_XHRQ_JS_BINDING_ERROR);
}


void VJScURLXMLHttpRequest::_GetResponseText(XBOX::VJSParms_getProperty& ioParms, cURLXMLHttpRequest* inXhr)
{
    if(inXhr)
    {
        XBOX::VString value;
        XBOX::VError res=inXhr->GetResponseText(&value);

        //returns an empty string on error
        ioParms.ReturnString(value);
    }
    else
        XBOX::vThrowError(VE_XHRQ_JS_BINDING_ERROR);
}


bool VJScURLXMLHttpRequest::_SetResponseType(XBOX::VJSParms_setProperty& ioParms, cURLXMLHttpRequest* inXhr)
{
	if (ioParms.IsValueString())
	{
		XBOX::VString pType;

		if (ioParms.GetPropertyValue().GetString(pType))
		{
		    if(inXhr)
			{
				XBOX::VError res=inXhr->SetResponseType(pType);
				
				if(res==XBOX::VE_OK)
					return true;
				else if(res==XBOX::VE_INVALID_PARAMETER)
					XBOX::vThrowError(VE_XHRQ_UNSUPPORTED_PARAMETER);
				else
					XBOX::vThrowError(res);
		    }
			else
				XBOX::vThrowError(VE_XHRQ_JS_BINDING_ERROR);
		}
	}

	return false;
}


void VJScURLXMLHttpRequest::_GetResponseType(XBOX::VJSParms_getProperty& ioParms, cURLXMLHttpRequest* inXhr)
{
	if(inXhr)
    {
		XBOX::VString type;
		XBOX::VError verr=inXhr->GetResponseType(&type);
        ioParms.ReturnString(type);
    }
 }



void VJScURLXMLHttpRequest::_GetResponse(XBOX::VJSParms_getProperty& ioParms, cURLXMLHttpRequest* inXhr)
{
	XBOX::VJSValue value(ioParms.GetContext());

	if(inXhr)
    {
        inXhr->GetResponse(value);

        ioParms.ReturnValue(value);
    }
    else
        XBOX::vThrowError(VE_XHRQ_JS_BINDING_ERROR);    
}




#endif // !WITH_NATIVE_HTTP_BACKEND

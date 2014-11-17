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

#include "VJSEndPoint.h"

#include "VJSBuffer.h"



XBOX::VError EndPointHandle::Create(const XBOX::VString& inDnsName, PortNumber inPort, sLONG inMsTimeout)
{
	XBOX::XTCPSock* sock=XBOX::XTCPSock::NewClientConnectedSock(inDnsName, inPort, inMsTimeout);
	
	if(sock==NULL)
		return XBOX::vThrowError(VE_ENDP_SOCKET_CREATION_FAIL_ERROR);
	
	fEp=new XBOX::VTCPEndPoint(sock);
	
	if(fEp==NULL)
		return XBOX::vThrowError(VE_ENDP_ENDPOINT_CREATION_FAIL_ERROR);

	return XBOX::VE_OK;
}


void VJSEndPoint::GetDefinition(ClassDefinition& outDefinition)
{
    const XBOX::JS4D::PropertyAttributes attDontWriteEnumDelete	= XBOX::JS4D::PropertyAttributeReadOnly
																| XBOX::JS4D::PropertyAttributeDontEnum
																| XBOX::JS4D::PropertyAttributeDontDelete;

    static inherited::StaticFunction functions[] =
		{
            { "connect",	js_callStaticFunction<_Connect>,	attDontWriteEnumDelete},
            { "read",		js_callStaticFunction<_Read>,		attDontWriteEnumDelete},
            { "write",		js_callStaticFunction<_Write>,		attDontWriteEnumDelete},
            { "close",      js_callStaticFunction<_Close>,      attDontWriteEnumDelete},
            { 0, 0, 0}
        };

    static inherited::StaticValue values[] =
        {
            { "isConnected",    js_getProperty<_IsConnected>,	nil,    attDontWriteEnumDelete},
			{ "isClosed",		js_getProperty<_IsClosed>,		nil,	attDontWriteEnumDelete},
			{ "totalRead",		js_getProperty<_GetReadCount>,	nil,    attDontWriteEnumDelete},
			{ "totalWrite",		js_getProperty<_GetWriteCount>,	nil,	attDontWriteEnumDelete},
            { 0, 0, 0, 0}
        };

    outDefinition.className         = "EndPoint";
    outDefinition.staticFunctions   = functions;

    outDefinition.initialize        = js_initialize<Initialize>;
    outDefinition.finalize          = js_finalize<Finalize>;

    outDefinition.staticValues      = values;

}

void VJSEndPoint::Initialize(const XBOX::VJSParms_initialize& inParms, EndPointHandle* inEph)
{
    //Alloc and init is done in Construct();
}

void VJSEndPoint::Finalize(const XBOX::VJSParms_finalize& inParms, EndPointHandle* inEph)
{
    if(inEph)
        delete inEph;
}

void VJSEndPoint::Construct(XBOX::VJSParms_construct& ioParms)
{
    EndPointHandle* eph=new EndPointHandle();
	
    if(eph==NULL)
    {
        XBOX::vThrowError(VE_ENDP_IMPL_FAIL_ERROR);
        return;
    }
	
	ioParms.ReturnConstructedObject<VJSEndPoint>( eph);
}



//void VJSEndPoint::CallAsFunction(XBOX::VJSParms_callAsFunction& ioParms){};


void VJSEndPoint::_Connect(XBOX::VJSParms_callStaticFunction& ioParms, EndPointHandle* inEph)
{
	//expected parameters : dnsName,  port, [withSSL], [msTimeout]
	
    XBOX::VString pDnsName;
    bool resDnsName=ioParms.GetStringParam(1, pDnsName);
	
	sLONG pPort;
	bool resPort=ioParms.GetLongParam(2, &pPort);
	
	bool pWithSSL=false;
	bool resWithSSL=ioParms.GetBoolParam(3, &pWithSSL);
	
	sLONG pTimeout;
	bool resTimeout=ioParms.GetLongParam(4, &pTimeout);
	
	
	if(!resTimeout)
	{
		pTimeout=15000 /*ms*/;
		resTimeout=true;
	}
	
	if(!resDnsName || !resPort || !resTimeout)
	{
		XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
		return;
	}
	
	if(inEph==NULL)
	{
		XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
		return;
	}
	
	XBOX::VError verr=inEph->Create(pDnsName, pPort, pTimeout);
	
	if(verr!=XBOX::VE_OK || inEph->Get()==NULL)
	{
		XBOX::vThrowError(VE_ENDP_IMPL_FAIL_ERROR);
		return;
	}
	
	if(resWithSSL && pWithSSL)
	{
		verr=inEph->Get()->PromoteToSSL(true);
	
		if(verr!=XBOX::VE_OK)
		{
			XBOX::vThrowError(VE_ENDP_IMPL_FAIL_ERROR);
			return;
		}
	}
	
	inEph->fIsConnected=true;
	inEph->fIsClosed=false;
}


void VJSEndPoint::_Read(XBOX::VJSParms_callStaticFunction& ioParms, EndPointHandle* inEph)
{
	if(inEph==NULL || inEph->Get()==NULL)
	{
		XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
		return;
	}
	
	uLONG len=8*1024;
	uBYTE* buf=(uBYTE*) ::malloc(len);
	
	if(buf==NULL)
	{
		XBOX::vThrowError(VE_ENDP_IMPL_FAIL_ERROR);
		return;
	}
	
	XBOX::VError verr=inEph->Get()->Read(buf, &len);
	
	if(verr!=XBOX::VE_OK && verr!=XBOX::VE_SRVR_NOTHING_TO_READ)
	{
		XBOX::vThrowError(VE_ENDP_READ_FAILED);
		return;
	}
	
	inEph->fReadCount+=len;
	
	sLONG slen(len);
	
	XBOX::VJSContext ctx=ioParms.GetContext();
	
	XBOX::VJSObject jsBuf=XBOX::VJSBufferClass::NewInstance(ctx, slen, buf);
	
	ioParms.ReturnValue(jsBuf);
}


void VJSEndPoint::_Write(XBOX::VJSParms_callStaticFunction& ioParms, EndPointHandle* inEph)
{
	if(inEph==NULL || inEph->Get()==NULL)
	{
		XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
		return;
	}
	
	XBOX::VJSObject	jsObj(ioParms.GetContext());
	
	if (!ioParms.IsObjectParam(1) || !ioParms.GetParamObject(1, jsObj) || !jsObj.IsInstanceOf("Buffer"))
	{
		XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
		return;
	}
	
	XBOX::VJSBufferObject* jsBuf=jsObj.GetPrivateData<XBOX::VJSBufferClass>();
	
	if(jsBuf==NULL)
	{
		XBOX::vThrowError(VE_ENDP_IMPL_FAIL_ERROR);
		return;
	}
	
	uBYTE* buf=(uBYTE *) jsBuf->GetDataPtr();
	
	if(buf==NULL)
	{
		XBOX::vThrowError(VE_ENDP_IMPL_FAIL_ERROR);
		return;
	}
	
	sLONG slen=(sLONG) jsBuf->GetDataSize();
	
	uLONG len=slen;
	
	XBOX::VError verr=inEph->Get()->WriteExactly(buf, len, 0 /*no timeout*/);
	
	if(verr!=XBOX::VE_OK)
	{
		XBOX::vThrowError(VE_ENDP_WRITE_FAILED);
		return;
	}
	
	inEph->fWriteCount+=len;
}


void VJSEndPoint::_Close(XBOX::VJSParms_callStaticFunction& ioParms, EndPointHandle* inEph)
{
	if(inEph==NULL || inEph->Get()==NULL)
	{
		XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
		return;
	}
	
	inEph->fIsConnected=false;
	inEph->fIsClosed=true;
	
	XBOX::VError verr=inEph->Get()->Close();
	
	if(verr!=XBOX::VE_OK)
	{
		XBOX::vThrowError(VE_ENDP_CLOSE_FAILED);
		return;
	}
}

void VJSEndPoint::_IsConnected(XBOX::VJSParms_getProperty& ioParms, EndPointHandle* inEph)
{
	if(inEph!=NULL && inEph->Get()!=NULL)
	{
		bool flag=inEph->fIsConnected;
		
		ioParms.ReturnBool(flag);
	}
	else
		ioParms.ReturnBool(false);
}


void VJSEndPoint::_IsClosed(XBOX::VJSParms_getProperty& ioParms, EndPointHandle* inEph)
{
	if(inEph!=NULL && inEph->Get()!=NULL)
	{
		bool flag=inEph->fIsClosed;
	
		ioParms.ReturnBool(flag);
	}
	else
		ioParms.ReturnBool(false);
}


void VJSEndPoint::_GetReadCount(XBOX::VJSParms_getProperty& ioParms, EndPointHandle* inEph)
{
	if(inEph!=NULL && inEph->Get()!=NULL)
	{
		sLONG count=inEph->fReadCount;
		
		ioParms.ReturnNumber(count);
	}
	else
		ioParms.ReturnNumber(0);
}


void VJSEndPoint::_GetWriteCount(XBOX::VJSParms_getProperty& ioParms, EndPointHandle* inEph)
{
	if(inEph!=NULL && inEph->Get()!=NULL)
	{
		sLONG count=inEph->fWriteCount;
		
		ioParms.ReturnNumber(count);
	}
	else
		ioParms.ReturnNumber(0);
}

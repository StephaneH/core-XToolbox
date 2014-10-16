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

#include <string>
#include <sstream>
#include <curl/curl.h>

#include "CurlWrapper.h"



#define CASE_CURLE_(err)                        \
    case CURLE_##err :                          \
    return VE_CW_CE_##err;



namespace CW
{
    XBOX::VString VStringFromStdString(const std::string& inStr)
    {
        const char* ptr=inStr.c_str();
        int len=inStr.length();

        XBOX::VString outStr;

        outStr.FromBlock(ptr, len, XBOX::VTC_UTF_8);

        return outStr;
    }


    std::string StdStringFromVString(const XBOX::VString& inStr)
    {
        XBOX::VStringConvertBuffer buffer(inStr, XBOX::VTC_UTF_8);
        return std::string(buffer.GetCPointer(), buffer.GetSize());
    }


    XBOX::VError CurlCodeToVError(CURLcode code)
    {
        switch(code)
        {
            CASE_CURLE_(OK);                        /* 0 */
            CASE_CURLE_(UNSUPPORTED_PROTOCOL);      /* 1 */
            CASE_CURLE_(FAILED_INIT);               /* 2 */
            CASE_CURLE_(URL_MALFORMAT);             /* 3 */
            //CASE_CURLE_(OBSOLETE4);                 /* 4 - NOT USED */
            CASE_CURLE_(COULDNT_RESOLVE_PROXY);     /* 5 */
            CASE_CURLE_(COULDNT_RESOLVE_HOST);      /* 6 */
            CASE_CURLE_(COULDNT_CONNECT);           /* 7 */
            CASE_CURLE_(FTP_WEIRD_SERVER_REPLY);    /* 8 */
#if VERSIONWIN
            CASE_CURLE_(REMOTE_ACCESS_DENIED);      /* 9 a service was denied by the server due to lack of access - when login fails this is not returned. */
#endif
            //CASE_CURLE_(OBSOLETE10);              /* 10 - NOT USED */
            CASE_CURLE_(FTP_WEIRD_PASS_REPLY);      /* 11 */
            //CASE_CURLE_(OBSOLETE12);              /* 12 - NOT USED */
            CASE_CURLE_(FTP_WEIRD_PASV_REPLY);      /* 13 */
            CASE_CURLE_(FTP_WEIRD_227_FORMAT);      /* 14 */
            CASE_CURLE_(FTP_CANT_GET_HOST);         /* 15 */
            //CASE_CURLE_(OBSOLETE16);              /* 16 - NOT USED */
#if VERSIONWIN
            CASE_CURLE_(FTP_COULDNT_SET_TYPE);      /* 17 */
#endif
            CASE_CURLE_(PARTIAL_FILE);              /* 18 */
            CASE_CURLE_(FTP_COULDNT_RETR_FILE);     /* 19 */
            //CASE_CURLE_(OBSOLETE20);              /* 20 - NOT USED */
#if VERSIONWIN
            CASE_CURLE_(QUOTE_ERROR);               /* 21 - quote command failure */
#endif
            CASE_CURLE_(HTTP_RETURNED_ERROR);       /* 22 */
            CASE_CURLE_(WRITE_ERROR);               /* 23 */
            //CASE_CURLE_(OBSOLETE24);              /* 24 - NOT USED */
            CASE_CURLE_(UPLOAD_FAILED);             /* 25 - failed upload "command" */
            CASE_CURLE_(READ_ERROR);                /* 26 - couldn't open/read from file */
            CASE_CURLE_(OUT_OF_MEMORY);             /* 27 - May indicate a conversion error if CURL_DOES_CONVERSIONS is defined */
            CASE_CURLE_(OPERATION_TIMEDOUT);        /* 28 - the timeout time was reached */
            //CASE_CURLE_(OBSOLETE29);              /* 29 - NOT USED */
            CASE_CURLE_(FTP_PORT_FAILED);           /* 30 - FTP PORT operation failed */
            CASE_CURLE_(FTP_COULDNT_USE_REST);      /* 31 - the REST command failed */
            //CASE_CURLE_(OBSOLETE32);              /* 32 - NOT USED */
#if VERSIONWIN
            CASE_CURLE_(RANGE_ERROR);               /* 33 - RANGE "command" didn't work */
#endif
            CASE_CURLE_(HTTP_POST_ERROR);           /* 34 */
            CASE_CURLE_(SSL_CONNECT_ERROR);         /* 35 - wrong when connecting with SSL */
            CASE_CURLE_(BAD_DOWNLOAD_RESUME);       /* 36 - couldn't resume download */
            CASE_CURLE_(FILE_COULDNT_READ_FILE);    /* 37 */
            CASE_CURLE_(LDAP_CANNOT_BIND);          /* 38 */
            CASE_CURLE_(LDAP_SEARCH_FAILED);        /* 39 */
            //CASE_CURLE_(OBSOLETE40);              /* 40 - NOT USED */
            CASE_CURLE_(FUNCTION_NOT_FOUND);        /* 41 */
            CASE_CURLE_(ABORTED_BY_CALLBACK);       /* 42 */
            CASE_CURLE_(BAD_FUNCTION_ARGUMENT);     /* 43 */
            //CASE_CURLE_(OBSOLETE44);              /* 44 - NOT USED */
            CASE_CURLE_(INTERFACE_FAILED);          /* 45 - CURLOPT_INTERFACE failed */
            //CASE_CURLE_(OBSOLETE46);              /* 46 - NOT USED */
            CASE_CURLE_(TOO_MANY_REDIRECTS );       /* 47 - catch endless re-direct loops */
            CASE_CURLE_(UNKNOWN_TELNET_OPTION);     /* 48 - User specified an unknown option */
            CASE_CURLE_(TELNET_OPTION_SYNTAX );     /* 49 - Malformed telnet option */
            //CASE_CURLE_(OBSOLETE50);              /* 50 - NOT USED */
#if VERSIONWIN
            CASE_CURLE_(PEER_FAILED_VERIFICATION);  /* 51 - peer's certificate or fingerprint wasn't verified fine */
#endif
            CASE_CURLE_(GOT_NOTHING);               /* 52 - when this is a specific error */
            CASE_CURLE_(SSL_ENGINE_NOTFOUND);       /* 53 - SSL crypto engine not found */
            CASE_CURLE_(SSL_ENGINE_SETFAILED);      /* 54 - can not set SSL crypto engine as default */
            CASE_CURLE_(SEND_ERROR);                /* 55 - failed sending network data */
            CASE_CURLE_(RECV_ERROR);                /* 56 - failure in receiving network data */
#if VERSIONWIN
            CASE_CURLE_(OBSOLETE57);                /* 57 - NOT IN USE */
#endif
            CASE_CURLE_(SSL_CERTPROBLEM);           /* 58 - problem with the local certificate */
            CASE_CURLE_(SSL_CIPHER);                /* 59 - couldn't use specified cipher */
            CASE_CURLE_(SSL_CACERT);                /* 60 - problem with the CA cert (path?) */
            CASE_CURLE_(BAD_CONTENT_ENCODING);      /* 61 - Unrecognized transfer encoding */
            CASE_CURLE_(LDAP_INVALID_URL);          /* 62 - Invalid LDAP URL */
            CASE_CURLE_(FILESIZE_EXCEEDED);         /* 63 - Maximum file size exceeded */
#if VERSIONWIN
            CASE_CURLE_(USE_SSL_FAILED);            /* 64 - Requested FTP SSL level failed */
#endif
            CASE_CURLE_(SEND_FAIL_REWIND);          /* 65 - Sending the data requires a rewind that failed */
            CASE_CURLE_(SSL_ENGINE_INITFAILED);     /* 66 - failed to initialise ENGINE */
            CASE_CURLE_(LOGIN_DENIED);              /* 67 - user password or similar was not accepted and we failed to login */
            CASE_CURLE_(TFTP_NOTFOUND);             /* 68 - file not found on server */
            CASE_CURLE_(TFTP_PERM);                 /* 69 - permission problem on server */
#if VERSIONWIN
            CASE_CURLE_(REMOTE_DISK_FULL);          /* 70 - out of disk space on server */
#endif
            CASE_CURLE_(TFTP_ILLEGAL);              /* 71 - Illegal TFTP operation */
            CASE_CURLE_(TFTP_UNKNOWNID);            /* 72 - Unknown transfer ID */
#if VERSIONWIN
            CASE_CURLE_(REMOTE_FILE_EXISTS);        /* 73 - File already exists */
#endif
            CASE_CURLE_(TFTP_NOSUCHUSER);           /* 74 - No such user */
            CASE_CURLE_(CONV_FAILED);               /* 75 - conversion failed */
            CASE_CURLE_(CONV_REQD);                 /* 76 - caller must register conversion callbacks using curl_easy_setopt options... */
            CASE_CURLE_(SSL_CACERT_BADFILE);        /* 77 - could not load CACERT file missing or wrong format */
            CASE_CURLE_(REMOTE_FILE_NOT_FOUND);     /* 78 - remote file not found */
            CASE_CURLE_(SSH);                       /* 79 - error from the SSH layer... somewhat generic so the error message will be of interest when this has happened */
            CASE_CURLE_(SSL_SHUTDOWN_FAILED);       /* 80 - Failed to shut down the SSL connection */
#if VERSIONWIN
            CASE_CURLE_(AGAIN);                     /* 81 - socket is not ready for send/recv wait till it's ready and try again */
            CASE_CURLE_(SSL_CRL_BADFILE);           /* 82 - could not load CRL file missing or wrong format */
            CASE_CURLE_(SSL_ISSUER_ERROR);          /* 83 - Issuer check failed. */
#endif
			default:
				break;
        }

        return VE_CW_CE_UNDEFINED_ERROR;
    }



    ////////////////////////////////////////////////////////////////////////////////
    //
    // Buffer
    //
    ////////////////////////////////////////////////////////////////////////////////

    size_t Buffer::WriteFunction(void *ptr, size_t size, size_t nmemb, void *stream)
    {
        assert(stream);

        Buffer* buf=(Buffer*)(stream);

        if(size<=0)
        {
            //On a termine avec ce buffer...
            return 0;
        }

        assert(ptr);

        char* cptr=(char*)(ptr);

        for(uLONG i=0 ; i<size*nmemb ; i++, cptr++)
            buf->Push(*cptr);

        return size*nmemb;
    }


    size_t Buffer::ReadFunction(void *ptr, size_t size, size_t nmemb, void *stream)
    {
        assert(stream);

        Buffer* buf=(Buffer*)(stream);

        assert(ptr);

        char* cptr=(char*)(ptr);

        size_t count=0;

        while(count<size*nmemb && buf->fRead<buf->fBuf.size())
            cptr[count++]=buf->fBuf[buf->fRead++];

        return count;
    }


	int Buffer::SeekFunction(void *stream, size_t offset, int origin)
    {
        assert(stream);
		
		Buffer* buf=(Buffer*)(stream);
		
		xbox_assert(origin==SEEK_SET);
		
		if(origin!=SEEK_SET)
			return CURL_SEEKFUNC_FAIL;
		
		if(offset>=buf->fBuf.size())
			return CURL_SEEKFUNC_FAIL;

		buf->fRead=offset;
		
        return CURL_SEEKFUNC_OK;
    }
	

    void Buffer::Push(char c)
    {
        fBuf.push_back(c);
    }


    void* Buffer::GetReadData()
    {
        return this;
    }
	
	
	void* Buffer::GetSeekData()
    {
        return this;
    }


    CurlReadFunction Buffer::GetReadFunction() const
    {
        return reinterpret_cast<CurlReadFunction>(&Buffer::ReadFunction);
    }
	
	
	CurlSeekFunction Buffer::GetSeekFunction() const
    {
        return reinterpret_cast<CurlSeekFunction>(&Buffer::SeekFunction);
	}
	

    size_t Buffer::GetReadSize() const
    {
        return fRead;
    }


    void* Buffer::GetWriteData()
    {
        return this;
    }


    CurlWriteFunction Buffer::GetWriteFunction() const
    {
        return reinterpret_cast<CurlWriteFunction>(&Buffer::WriteFunction);
    }


    const char* Buffer::GetCPointer() const
    {
        return &fBuf[0];
    }


    size_t Buffer::GetLength() const
    {
        return fBuf.size();
    }


    size_t Buffer::AddRawPtr(const void* ptr, size_t len)
    {
        const char* pos=(char*)ptr;
        const char* past=pos+len;

        while(pos<past)
            Push(*pos++);

        return len;
    }



    ////////////////////////////////////////////////////////////////////////////////
    //
    // ResponseHeaders
    //
    ////////////////////////////////////////////////////////////////////////////////

    size_t ResponseHeaders::WriteFunction(void *ptr, size_t size, size_t nmemb, void* thisPtr)
    {
        assert(thisPtr);

        //Headers* buf=(Headers*)(stream);

        if(size<=0)
        {
            //On a termine avec ce buffer...
            return 0;
        }

        assert(ptr);

        char* start=(char*)(ptr);
        char* past=start+size*nmemb;
        char* pos=start;

        char* key_start=NULL;
        char* key_past=NULL;

        char* value_start=NULL;
        char* value_past=NULL;


        for (pos=start ; pos<past && *pos!=':' ; pos++)
		{
            if (key_start==NULL && *pos!=' ' && *pos!='\t')
                key_start=pos, key_past=pos;
		}

        if (key_start!=NULL)
            key_past=pos;

		for (pos=key_past+1 ; pos<past && *pos!='\r' && *pos!='\n' ; pos++)
		{
            if(value_start==NULL && *pos!=' ' && *pos!='\t')
            {
                value_start=pos, value_past=past;
                break;
            }
		}


        if (key_start!=NULL)
		{
            for (pos=key_past-1 ; pos>=key_start ; pos--)
			{
                if (*pos==' ' || *pos=='\t' || *pos=='\r' || *pos=='\n')
                    key_past--;
                else
                    break;
			}
		}

        if (value_start!=NULL)
		{
            for (pos=value_past-1 ; pos>=value_start ; pos--)
			{
                if (*pos==' ' || *pos=='\t' || *pos=='\r' || *pos=='\n')
                    value_past--;
                else
                    break;
			}
		}

        std::string key(key_start, key_past-key_start);
        std::string value(value_start, value_past-value_start);

        ResponseHeaders* me=static_cast<ResponseHeaders*>(thisPtr);

		HeaderCol::iterator it=me->fHeaders.insert(make_pair(key, value));

        return past-start;
    }


    void* ResponseHeaders::GetWriteData()
    {
        return this;
    }


    CurlWriteFunction ResponseHeaders::GetWriteFunction() const
    {
        return reinterpret_cast<CurlWriteFunction>(&ResponseHeaders::WriteFunction);
    }


    const char* ResponseHeaders::GetHeader(const char* inKey) const
    {
        HeaderCol::const_iterator cit;

        cit=fHeaders.find(std::string(inKey));

        if(cit!=fHeaders.end())
            return cit->second.c_str();

        return NULL;
    }


    bool ResponseHeaders::GetHeader(const XBOX::VString& inKey, XBOX::VString* outValue) const
    {
		bool rv=false;

		outValue->Clear();

        std::string inKeyTmp=StdStringFromVString(inKey);

		std::pair <HeaderCol::const_iterator, HeaderCol::const_iterator> range;
		range=fHeaders.equal_range(inKeyTmp);

		if(range.first!=range.second)
		{
			HeaderCol::const_iterator cit=range.first;

			outValue->AppendString(VStringFromStdString(cit->second));
			++cit;

			rv=true;
			
			while(cit!=range.second)
			{
				outValue->AppendString(CVSTR(", "));
				outValue->AppendString(VStringFromStdString(cit->second));
				++cit;
			}

		}

        return rv;
    }


    void ResponseHeaders::GetAllHeaders(XBOX::VString* outValue) const
    {
        std::ostringstream oss;
        HeaderCol::const_iterator cit;

        for(cit=fHeaders.begin() ; cit!=fHeaders.end() ; ++cit)
            oss << cit->first << ": " << cit->second << std::endl;

        *outValue=VStringFromStdString(oss.str());
    }



    ////////////////////////////////////////////////////////////////////////////////
    //
    // RequestHeaders
    //
    ////////////////////////////////////////////////////////////////////////////////


    void RequestHeaders::SetHeader(const XBOX::VString& inKey, const XBOX::VString& inValue)
    {
        std::map<std::string, std::string>::const_iterator cit;

        std::string inKeyTmp=StdStringFromVString(inKey);
        std::string inValueTmp=StdStringFromVString(inValue);

        fHeaders.erase(inKeyTmp);
        fHeaders.insert(make_pair(inKeyTmp, inValueTmp));
    }


    void RequestHeaders::FixContentThings()
    {
		RemoveContentLength();

        std::map<std::string, std::string>::iterator it;
		
		bool haveContentType=false;

		it=fHeaders.begin();
		
		while(it!=fHeaders.end())
		{
			XBOX::VString header(it->first.c_str());
			
			header.ToLowerCase();
			
			if(header=="content-type")
			{
				haveContentType=true;
				break;
			}
			
			++it;
		}
		
		if(!haveContentType)
			SetHeader("Content-Type", "text/plain; charset=utf-8");
	}

	
	void RequestHeaders::RemoveContentLength()
    {
        std::map<std::string, std::string>::iterator it;

		it=fHeaders.begin();

		while(it!=fHeaders.end())
		{
			XBOX::VString header(it->first.c_str());

			header.ToLowerCase();

			if(header=="content-length")
			{
				fHeaders.erase(it++);
				break;
			}

			++it;
		}
    }

	
	XBOX::VString RequestHeaders::FixAcceptEncoding()
	{
		XBOX::VString encoding;

		std::map<std::string, std::string>::iterator it;

		it=fHeaders.begin();

		while(it!=fHeaders.end())
		{
			XBOX::VString header(it->first.c_str());

			header.ToLowerCase();

			if(header=="accept-encoding")
			{
				encoding=XBOX::VString(it->second.c_str());
				encoding.ToLowerCase();

				fHeaders.erase(it++);
				break;
			}

			++it;
		}

		return encoding;
	}


    const curl_slist* RequestHeaders::GetCurlSlist()
    {
        std::map<std::string, std::string>::const_iterator mit;

        fTmpStack.clear();

        for(mit=fHeaders.begin() ; mit!=fHeaders.end() ; ++mit)
        {
            std::ostringstream oss;

            oss << mit->first << ": " << mit->second;

            fTmpStack.push_back(oss.str());
        }

        if(fSlist)
        {
            curl_slist_free_all(fSlist);
            fSlist=NULL;
        }

        std::vector<std::string>::const_iterator vit;

        for(vit=fTmpStack.begin() ; vit!=fTmpStack.end() ; ++vit)
        {
            curl_slist* tmp=curl_slist_append(fSlist, vit->c_str());

            if(tmp)
                fSlist=tmp;
            else
            {
                curl_slist_free_all(fSlist);
                fSlist=NULL;
                break;
            }
        }

        return fSlist;
    }



    ////////////////////////////////////////////////////////////////////////////////
    //
    // HttpRequest
    //
    ////////////////////////////////////////////////////////////////////////////////

    HttpRequest::HttpRequest(const XBOX::VString& inUrl, const Method inMethod) :
        fUrl(inUrl),
        fHandle(NULL),
        fMethod(inMethod),
        fHasValidResponseCode(false),
        fResponseCode(0),
        fHasValidProxyCode(false),
        fProxyCode(0)
    {
        fHandle=curl_easy_init();
    }


    HttpRequest::~HttpRequest()
    {
        if(fHandle)
            curl_easy_cleanup(fHandle);
    }

	
	bool HttpRequest::SetUserInfos(const XBOX::VString& inUser, const XBOX::VString& inPasswd, bool inAllowBasic)
	{
		if(!fHandle)
            return false;
		
		XBOX::VString userInfos;
		userInfos.AppendString(inUser).AppendCString(":").AppendString(inPasswd);

		XBOX::VStringConvertBuffer buffer(userInfos, XBOX::VTC_UTF_8);
		curl_easy_setopt(fHandle, CURLOPT_USERPWD, buffer.GetCPointer());
	
		if(inAllowBasic)
			curl_easy_setopt(fHandle, CURLOPT_HTTPAUTH, CURLAUTH_BASIC|CURLAUTH_DIGEST);
		else	
			curl_easy_setopt(fHandle, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		
		return true;
	}
	
	
	bool HttpRequest::SetClientCertificate(const XBOX::VString& inKeyPath, const XBOX::VString& inCertPath)
	{
		if(!fHandle)
            return false;

		XBOX::VStringConvertBuffer keyBuffer(inKeyPath, XBOX::VTC_UTF_8);
		curl_easy_setopt(fHandle, CURLOPT_SSLKEY, keyBuffer.GetCPointer());
				
		XBOX::VStringConvertBuffer pathBuffer(inCertPath, XBOX::VTC_UTF_8);
		curl_easy_setopt(fHandle, CURLOPT_SSLCERT, pathBuffer.GetCPointer());
		
		return true;
	}
	
	
    void HttpRequest::SetProxy(const XBOX::VString& inHost, uLONG inPort)
    {
        if(!fHandle)
            return;

		XBOX::VStringConvertBuffer buffer(inHost, XBOX::VTC_UTF_8);
		curl_easy_setopt(fHandle, CURLOPT_SSLCERT, buffer.GetCPointer());

        //si proxy est une chaine vide, aura pour effet de supprimer le proxy
        curl_easy_setopt(fHandle, CURLOPT_PROXY, buffer.GetCPointer());

        if(inPort>0)    //Par defaut, libcURL utilise le port 1080
            curl_easy_setopt(fHandle, CURLOPT_PROXYPORT, inPort);
    }


    void HttpRequest::SetSystemProxy()
    {
		XBOX::VString proxy;
		PortNumber port=kBAD_PORT;

		XBOX::VError verr=XBOX::VProxyManager::GetProxyForURL(fUrl, &proxy, &port);

		if(verr==XBOX::VE_OK && !proxy.IsEmpty())
			SetProxy(proxy, port);
    }
	

    bool HttpRequest::SetRequestHeader(const XBOX::VString& inKey, const XBOX::VString& inValue)
    {
        if(!fHandle || inKey.IsEmpty())
            return false;

        fReqHdrs.SetHeader(inKey, inValue);

        return true;
    }


	bool HttpRequest::SetTimeout(uLONG inConnectMs, uLONG inTotalMs)
    {
		if(!fHandle)
			return false;

		//Don't use signals ; won't honor timeout if blocked in DNS resolution

		curl_easy_setopt (fHandle, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt (fHandle, CURLOPT_CONNECTTIMEOUT_MS, inConnectMs);
		curl_easy_setopt (fHandle, CURLOPT_TIMEOUT_MS, inTotalMs);

		return true;
    }


	bool HttpRequest::SetBinaryData(const void* data, sLONG datalen)
	{
		int count=fData.AddRawPtr(data, datalen);

		fReqHdrs.RemoveContentLength();

		curl_easy_setopt(fHandle, CURLOPT_POSTFIELDSIZE, count);

		return true;
	}


    bool HttpRequest::SetData(const XBOX::VString& inData, XBOX::CharSet inCS)
    {
		XBOX::CharSet cs=(inCS!=XBOX::VTC_UNKNOWN ? inCS : XBOX::VTC_US_ASCII); 

        XBOX::VStringConvertBuffer buffer(inData, cs);
        int count=fData.AddRawPtr(buffer.GetCPointer(), buffer.GetSize());

		fReqHdrs.FixContentThings();

		curl_easy_setopt(fHandle, CURLOPT_POSTFIELDSIZE, count);

        return true;
    }


    bool HttpRequest::Perform(XBOX::VError* outError)
    {
        //On essaie de faire plusieurs send de suite, pas bien !
        if(!fHandle)
        {
            //XBOX::vThrowError(VE_CWRP_SEND_WITHOUT_OPEN_ERROR);
            return false;
        }

        SetOpts();

        CURLcode res_perf=curl_easy_perform(fHandle);

        if(res_perf!=CURLE_OK && outError)
			*outError=XBOX::vThrowError(CurlCodeToVError(res_perf));

        //On recupere 2 ou 3 infos qui pourraient etre utiles plus tard
        long code=0;	//jmo - ACI0071188 : code est lu par cURL comme un long* (64bit) via var_arg
        CURLcode res_inf1=curl_easy_getinfo(fHandle, CURLINFO_RESPONSE_CODE, &code);

        if(code!=0)
            fHasValidResponseCode=true, fResponseCode=code;

        CURLcode res_inf2=curl_easy_getinfo(fHandle, CURLINFO_HTTP_CONNECTCODE, &code);

        if(code!=0)
            fHasValidProxyCode=true, fProxyCode=code;

        //Dans l'immediat, on n'a plus besoin des ressources utilisees par cURL
        curl_easy_cleanup(fHandle);
        fHandle=NULL;

        return res_perf==CURLE_OK && res_inf1==CURLE_OK && res_inf2==CURLE_OK;
    }


    bool HttpRequest::HasValidProxyCode(uLONG* outCode) const
    {
        if(fHasValidProxyCode)
            *outCode=fProxyCode;

        return fHasValidProxyCode;
    }


    bool HttpRequest::HasValidResponseCode(uLONG* outCode) const
    {
        if(fHasValidResponseCode)
            *outCode=fResponseCode;

        return fHasValidResponseCode;
    }


    const char* HttpRequest::GetResponseHeader(const char* inKey) const
    {
        return fRespHdrs.GetHeader(inKey);
    }


    bool HttpRequest::GetResponseHeader(const XBOX::VString& inKey, XBOX::VString* outValue) const
    {
        return fRespHdrs.GetHeader(inKey, outValue);
    }


    bool HttpRequest::GetAllResponseHeaders(XBOX::VString* outValue) const
    {
        fRespHdrs.GetAllHeaders(outValue);
        return true; //return false si req pas envoyee
    }


    const char* HttpRequest::GetContentCPointer() const
    {
        return fContent.GetLength() ? fContent.GetCPointer() : NULL;
    }


    uLONG HttpRequest::GetContentLength() const
    {
        return fContent.GetLength();
    }


    CURL* HttpRequest::GetHandle() const
    {
        return fHandle;
    }


    void HttpRequest::SetOpts()
    {
		XBOX::VStringConvertBuffer buffer(fUrl, XBOX::VTC_UTF_8);
        curl_easy_setopt(fHandle, CURLOPT_URL, buffer.GetCPointer());

		curl_easy_setopt(fHandle, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(fHandle, CURLOPT_WRITEFUNCTION, fContent.GetWriteFunction());
        curl_easy_setopt(fHandle, CURLOPT_WRITEDATA, fContent.GetWriteData());
        curl_easy_setopt(fHandle, CURLOPT_HEADERFUNCTION, fRespHdrs.GetWriteFunction());
        curl_easy_setopt(fHandle, CURLOPT_HEADERDATA, fRespHdrs.GetWriteData());
		curl_easy_setopt(fHandle, CURLOPT_TCP_NODELAY, 1L);
		
		//jmo - Meme comportement que la webview webkit sous Windows : Pas de verification des certificats !
		curl_easy_setopt(fHandle, CURLOPT_SSL_VERIFYPEER, 0);
		
		//jmo - par precaution, doit etre ignore sans VERIFYPEER
		curl_easy_setopt(fHandle, CURLOPT_SSL_VERIFYHOST, 0);
        
		switch(fMethod)
        {
        case GET :
            curl_easy_setopt(fHandle, CURLOPT_HTTPGET, 1L);
            fMethod=GET;
            break;

        case HEAD :
            curl_easy_setopt(fHandle, CURLOPT_NOBODY, 1L);
            break;

        case POST :
            //curl_easy_setopt(fHandle, CURLOPT_HTTPPOST, 1L);
            curl_easy_setopt(fHandle, CURLOPT_POST, 1L);
            curl_easy_setopt(fHandle, CURLOPT_READFUNCTION, fData.GetReadFunction());
            curl_easy_setopt(fHandle, CURLOPT_READDATA, fData.GetReadData());
			curl_easy_setopt(fHandle, CURLOPT_SEEKFUNCTION, fData.GetSeekFunction());
			curl_easy_setopt(fHandle, CURLOPT_SEEKDATA, fData.GetSeekData());
				
            //Est-ce qu'on doit également valoriser CURLOPT_POSTFIELDSIZE ?
            break;

        case PUT :
            curl_easy_setopt(fHandle, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(fHandle, CURLOPT_READFUNCTION, fData.GetReadFunction());
            curl_easy_setopt(fHandle, CURLOPT_READDATA, fData.GetReadData());
			curl_easy_setopt(fHandle, CURLOPT_SEEKFUNCTION, fData.GetSeekFunction());
			curl_easy_setopt(fHandle, CURLOPT_SEEKDATA, fData.GetSeekData());
            curl_easy_setopt(fHandle, CURLOPT_INFILESIZE, fData.GetLength());
            break;

        case DELETE :
			//jmo - Je suppose que seul le status nous interesse sur le DELETE...
			//		A vérifier.
            curl_easy_setopt(fHandle, CURLOPT_NOBODY, 1L);
			curl_easy_setopt(fHandle, CURLOPT_CUSTOMREQUEST, "DELETE");
            break;

        case CUSTOM :
            //Je suppose qu'on va mettre un pot pourri des autres options !
            break;

        default :
            assert(0);
        }


		XBOX::VString encoding=fReqHdrs.FixAcceptEncoding();

		if(encoding==CVSTR("deflate"))
			curl_easy_setopt(fHandle, CURLOPT_ENCODING, "deflate");
		else if(encoding==CVSTR("gzip"))
			curl_easy_setopt(fHandle, CURLOPT_ENCODING, "gzip");
		else if(!encoding.IsEmpty())
			curl_easy_setopt(fHandle, CURLOPT_ENCODING, "identity");

        const curl_slist* customHeaders=fReqHdrs.GetCurlSlist();
        curl_easy_setopt(fHandle, CURLOPT_HTTPHEADER, customHeaders);
    }
}

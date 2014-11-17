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
#ifndef CURL_WRAPPER_H
#define CURL_WRAPPER_H



#include <curl/curl.h>
#include <vector>
#include <map>
#include <string>



#if defined(WIN32) || defined(WIN64)
#   define CW_CDECL __cdecl
#	undef DELETE
#else
#   define CW_CDECL
#endif



namespace CW
{

    ////////////////////////////////////////////////////////////////////////////////
    //
    // Wrappers for Curl easy error codes
    //
    ////////////////////////////////////////////////////////////////////////////////

    //CW_CE -> Curl Wrapper - Curl Easy interface error codes
    //Substract 1100 for original error code.

    const XBOX::VError VE_CW_CE_OK                          = MAKE_VERROR('CWRP', 1100);
    const XBOX::VError VE_CW_CE_UNSUPPORTED_PROTOCOL        = MAKE_VERROR('CWRP', 1101);
    const XBOX::VError VE_CW_CE_FAILED_INIT                 = MAKE_VERROR('CWRP', 1102);
    const XBOX::VError VE_CW_CE_URL_MALFORMAT               = MAKE_VERROR('CWRP', 1103);
    //const XBOX::VError VE_CW_CE_OBSOLETE4                   = MAKE_VERROR('CWRP', 1104);
    const XBOX::VError VE_CW_CE_COULDNT_RESOLVE_PROXY       = MAKE_VERROR('CWRP', 1105);
    const XBOX::VError VE_CW_CE_COULDNT_RESOLVE_HOST        = MAKE_VERROR('CWRP', 1106);
    const XBOX::VError VE_CW_CE_COULDNT_CONNECT             = MAKE_VERROR('CWRP', 1107);
    const XBOX::VError VE_CW_CE_FTP_WEIRD_SERVER_REPLY      = MAKE_VERROR('CWRP', 1108);
    const XBOX::VError VE_CW_CE_REMOTE_ACCESS_DENIED        = MAKE_VERROR('CWRP', 1109);
    //const XBOX::VError VE_CW_CE_OBSOLETE10                = MAKE_VERROR('CWRP', 1110);
    const XBOX::VError VE_CW_CE_FTP_WEIRD_PASS_REPLY        = MAKE_VERROR('CWRP', 1111);
    //const XBOX::VError VE_CW_CE_OBSOLETE12                = MAKE_VERROR('CWRP', 1112);
    const XBOX::VError VE_CW_CE_FTP_WEIRD_PASV_REPLY        = MAKE_VERROR('CWRP', 1113);
    const XBOX::VError VE_CW_CE_FTP_WEIRD_227_FORMAT        = MAKE_VERROR('CWRP', 1114);
    const XBOX::VError VE_CW_CE_FTP_CANT_GET_HOST           = MAKE_VERROR('CWRP', 1115);
    //const XBOX::VError VE_CW_CE_OBSOLETE16                = MAKE_VERROR('CWRP', 1116);
    const XBOX::VError VE_CW_CE_FTP_COULDNT_SET_TYPE        = MAKE_VERROR('CWRP', 1117);
    const XBOX::VError VE_CW_CE_PARTIAL_FILE                = MAKE_VERROR('CWRP', 1118);
    const XBOX::VError VE_CW_CE_FTP_COULDNT_RETR_FILE       = MAKE_VERROR('CWRP', 1119);
    //const XBOX::VError VE_CW_CE_OBSOLETE20                = MAKE_VERROR('CWRP', 1120);
    const XBOX::VError VE_CW_CE_QUOTE_ERROR                 = MAKE_VERROR('CWRP', 1121);
    const XBOX::VError VE_CW_CE_HTTP_RETURNED_ERROR         = MAKE_VERROR('CWRP', 1122);
    const XBOX::VError VE_CW_CE_WRITE_ERROR                 = MAKE_VERROR('CWRP', 1123);
    //const XBOX::VError VE_CW_CE_OBSOLETE24                = MAKE_VERROR('CWRP', 1124);
    const XBOX::VError VE_CW_CE_UPLOAD_FAILED               = MAKE_VERROR('CWRP', 1125);
    const XBOX::VError VE_CW_CE_READ_ERROR                  = MAKE_VERROR('CWRP', 1126);
    const XBOX::VError VE_CW_CE_OUT_OF_MEMORY               = MAKE_VERROR('CWRP', 1127);
    const XBOX::VError VE_CW_CE_OPERATION_TIMEDOUT          = MAKE_VERROR('CWRP', 1128);
    //const XBOX::VError VE_CW_CE_OBSOLETE29                = MAKE_VERROR('CWRP', 1129);
    const XBOX::VError VE_CW_CE_FTP_PORT_FAILED             = MAKE_VERROR('CWRP', 1130);
    const XBOX::VError VE_CW_CE_FTP_COULDNT_USE_REST        = MAKE_VERROR('CWRP', 1131);
    //const XBOX::VError VE_CW_CE_OBSOLETE32                = MAKE_VERROR('CWRP', 1132);
    const XBOX::VError VE_CW_CE_RANGE_ERROR                 = MAKE_VERROR('CWRP', 1133);
    const XBOX::VError VE_CW_CE_HTTP_POST_ERROR             = MAKE_VERROR('CWRP', 1134);
    const XBOX::VError VE_CW_CE_SSL_CONNECT_ERROR           = MAKE_VERROR('CWRP', 1135);
    const XBOX::VError VE_CW_CE_BAD_DOWNLOAD_RESUME         = MAKE_VERROR('CWRP', 1136);
    const XBOX::VError VE_CW_CE_FILE_COULDNT_READ_FILE      = MAKE_VERROR('CWRP', 1137);
    const XBOX::VError VE_CW_CE_LDAP_CANNOT_BIND            = MAKE_VERROR('CWRP', 1138);
    const XBOX::VError VE_CW_CE_LDAP_SEARCH_FAILED          = MAKE_VERROR('CWRP', 1139);
    //const XBOX::VError VE_CW_CE_OBSOLETE40                = MAKE_VERROR('CWRP', 1140);
    const XBOX::VError VE_CW_CE_FUNCTION_NOT_FOUND          = MAKE_VERROR('CWRP', 1141);
    const XBOX::VError VE_CW_CE_ABORTED_BY_CALLBACK         = MAKE_VERROR('CWRP', 1142);
    const XBOX::VError VE_CW_CE_BAD_FUNCTION_ARGUMENT       = MAKE_VERROR('CWRP', 1143);
    //const XBOX::VError VE_CW_CE_OBSOLETE44                = MAKE_VERROR('CWRP', 1144);
    const XBOX::VError VE_CW_CE_INTERFACE_FAILED            = MAKE_VERROR('CWRP', 1145);
    //const XBOX::VError VE_CW_CE_OBSOLETE46                = MAKE_VERROR('CWRP', 1146);
    const XBOX::VError VE_CW_CE_TOO_MANY_REDIRECTS          = MAKE_VERROR('CWRP', 1147);
    const XBOX::VError VE_CW_CE_UNKNOWN_TELNET_OPTION       = MAKE_VERROR('CWRP', 1148);
    const XBOX::VError VE_CW_CE_TELNET_OPTION_SYNTAX        = MAKE_VERROR('CWRP', 1149);
    //const XBOX::VError VE_CW_CE_OBSOLETE50                = MAKE_VERROR('CWRP', 1150);
    const XBOX::VError VE_CW_CE_PEER_FAILED_VERIFICATION    = MAKE_VERROR('CWRP', 1151);
    const XBOX::VError VE_CW_CE_GOT_NOTHING                 = MAKE_VERROR('CWRP', 1152);
    const XBOX::VError VE_CW_CE_SSL_ENGINE_NOTFOUND         = MAKE_VERROR('CWRP', 1153);
    const XBOX::VError VE_CW_CE_SSL_ENGINE_SETFAILED        = MAKE_VERROR('CWRP', 1154);
    const XBOX::VError VE_CW_CE_SEND_ERROR                  = MAKE_VERROR('CWRP', 1155);
    const XBOX::VError VE_CW_CE_RECV_ERROR                  = MAKE_VERROR('CWRP', 1156);
    const XBOX::VError VE_CW_CE_OBSOLETE57                  = MAKE_VERROR('CWRP', 1157);
    const XBOX::VError VE_CW_CE_SSL_CERTPROBLEM             = MAKE_VERROR('CWRP', 1158);
    const XBOX::VError VE_CW_CE_SSL_CIPHER                  = MAKE_VERROR('CWRP', 1159);
    const XBOX::VError VE_CW_CE_SSL_CACERT                  = MAKE_VERROR('CWRP', 1160);
    const XBOX::VError VE_CW_CE_BAD_CONTENT_ENCODING        = MAKE_VERROR('CWRP', 1161);
    const XBOX::VError VE_CW_CE_LDAP_INVALID_URL            = MAKE_VERROR('CWRP', 1162);
    const XBOX::VError VE_CW_CE_FILESIZE_EXCEEDED           = MAKE_VERROR('CWRP', 1163);
    const XBOX::VError VE_CW_CE_USE_SSL_FAILED              = MAKE_VERROR('CWRP', 1164);
    const XBOX::VError VE_CW_CE_SEND_FAIL_REWIND            = MAKE_VERROR('CWRP', 1165);
    const XBOX::VError VE_CW_CE_SSL_ENGINE_INITFAILED       = MAKE_VERROR('CWRP', 1166);
    const XBOX::VError VE_CW_CE_LOGIN_DENIED                = MAKE_VERROR('CWRP', 1167);
    const XBOX::VError VE_CW_CE_TFTP_NOTFOUND               = MAKE_VERROR('CWRP', 1168);
    const XBOX::VError VE_CW_CE_TFTP_PERM                   = MAKE_VERROR('CWRP', 1169);
    const XBOX::VError VE_CW_CE_REMOTE_DISK_FULL            = MAKE_VERROR('CWRP', 1170);
    const XBOX::VError VE_CW_CE_TFTP_ILLEGAL                = MAKE_VERROR('CWRP', 1171);
    const XBOX::VError VE_CW_CE_TFTP_UNKNOWNID              = MAKE_VERROR('CWRP', 1172);
    const XBOX::VError VE_CW_CE_REMOTE_FILE_EXISTS          = MAKE_VERROR('CWRP', 1173);
    const XBOX::VError VE_CW_CE_TFTP_NOSUCHUSER             = MAKE_VERROR('CWRP', 1174);
    const XBOX::VError VE_CW_CE_CONV_FAILED                 = MAKE_VERROR('CWRP', 1175);
    const XBOX::VError VE_CW_CE_CONV_REQD                   = MAKE_VERROR('CWRP', 1176);
    const XBOX::VError VE_CW_CE_SSL_CACERT_BADFILE          = MAKE_VERROR('CWRP', 1177);
    const XBOX::VError VE_CW_CE_REMOTE_FILE_NOT_FOUND       = MAKE_VERROR('CWRP', 1178);
    const XBOX::VError VE_CW_CE_SSH                         = MAKE_VERROR('CWRP', 1179);
    const XBOX::VError VE_CW_CE_SSL_SHUTDOWN_FAILED         = MAKE_VERROR('CWRP', 1180);
    const XBOX::VError VE_CW_CE_AGAIN                       = MAKE_VERROR('CWRP', 1181);
    const XBOX::VError VE_CW_CE_SSL_CRL_BADFILE             = MAKE_VERROR('CWRP', 1182);
    const XBOX::VError VE_CW_CE_SSL_ISSUER_ERROR            = MAKE_VERROR('CWRP', 1183);

    //This one is not really a curl error code !
    const XBOX::VError VE_CW_CE_UNDEFINED_ERROR             = MAKE_VERROR('CWRP', 1199);


    ////////////////////////////////////////////////////////////////////////////////
    //
    // Curl Wrapper specific error codes
    //
    ////////////////////////////////////////////////////////////////////////////////

    //todo - virer tout ca ?
    const XBOX::VError VE_CWRP_SEND_ERROR                   = MAKE_VERROR('CWRP', 1005);
    const XBOX::VError VE_CWRP_SEND_WITHOUT_OPEN_ERROR      = MAKE_VERROR('CWRP', 1006);
    const XBOX::VError VE_CWRP_SEND_CONNECT_ERROR           = MAKE_VERROR('CWRP', 1007);


    const uLONG CW_PROXY_SIZE=256;
	//http://stackoverflow.com/questions/417142/what-is-the-maximum-length-of-a-url
    const uLONG CW_URL_SIZE=2048;
    

    typedef size_t (*CurlWriteFunction) (void *ptr, size_t size, size_t nmemb, void *stream);
    typedef size_t (*CurlReadFunction)  (void *ptr, size_t size, size_t nmemb, void *stream);
	typedef int	   (*CurlSeekFunction)	(void *stream, size_t offset, int origin);


    XBOX::VString   VStringFromAsciiStdString   (const std::string inStr);
    std::string     StdStringFromAsciiVString   (const XBOX::VString inStr);


    class Buffer
    {
    private :
    
        std::vector<char>   fBuf;
        uLONG               fRead;  //Past participle :)

        static size_t CW_CDECL  WriteFunction   (void *ptr, size_t size, size_t nmemb, void *thisPtr);   
        static size_t CW_CDECL  ReadFunction    (void *ptr, size_t size, size_t nmemb, void *stream);
		static int	  CW_CDECL  SeekFunction	(void *stream, size_t offset, int origin);

        void                Push                (char c);

    public :

        Buffer() : fRead(0) {}

        void*               GetReadData         ();
        CurlReadFunction    GetReadFunction     () const;
        size_t              GetReadSize         () const;

		void*               GetSeekData         ();
        CurlSeekFunction    GetSeekFunction     () const;
		
        void*               GetWriteData        ();
        CurlWriteFunction   GetWriteFunction    () const;

        const char*         GetCPointer         () const;
        size_t              GetLength           () const;

        size_t              AddRawPtr           (const void* ptr, size_t len);        
    };
    

    class ResponseHeaders
    {
    private :

		typedef std::multimap<std::string, std::string> HeaderCol;
        HeaderCol fHeaders;

        static size_t CW_CDECL  WriteFunction       (void *ptr, size_t size, size_t nmemb, void *stream); 

    public :

        void*               GetWriteData        ();
        CurlWriteFunction   GetWriteFunction    () const;
        const char*         GetHeader           (const char* inKey) const;
        bool                GetHeader           (const XBOX::VString& inKey, XBOX::VString* outValue) const;
        void                GetAllHeaders       (XBOX::VString* outValue) const;
    };


    class RequestHeaders
    {
    private :

        std::map<std::string, std::string>  fHeaders;
        std::vector<std::string>            fTmpStack;

        curl_slist* fSlist;

    public :

        RequestHeaders() : fSlist(NULL) {}
        ~RequestHeaders() { if(fSlist) curl_slist_free_all(fSlist); }

        void                SetHeader           (const XBOX::VString& inKey, const XBOX::VString& inValue);
        void                FixContentThings    ();
		void				RemoveContentLength	();
		XBOX::VString		FixAcceptEncoding   ();
        const curl_slist*   GetCurlSlist        ();
    };


    class HttpRequest
    {
    public :

        typedef enum {GET, HEAD, POST, PUT, DELETE, CUSTOM} Method;
		typedef enum {BASIC, DIGEST} AuthMethod;

        HttpRequest(const XBOX::VString& inUrl, const Method inMethod);
        ~HttpRequest();
		
		bool		SetUserInfos			(const XBOX::VString& inUser, const XBOX::VString& inPasswd, bool inAllowBasic);
		bool		SetClientCertificate	(const XBOX::VString& inKeyPath, const XBOX::VString& inCertPath);
        void        SetProxy                (const XBOX::VString& inProxy, uLONG inPort=-1);
		void        SetSystemProxy          ();
		bool        SetRequestHeader        (const XBOX::VString& inKey, const XBOX::VString& inValue);
		//SetTimeout, inConnectMs : Full second resolution, 2500ms actually waits for 3s
	    bool		SetTimeout				(uLONG inConnectMs=0 /*Defaults to 300s*/, uLONG inTotalMs=0 /*Forever, should be > to inConnectMs*/);
        //bool        SetData                 (const XBOX::VString& inData);
		bool        SetData(const XBOX::VString& inData, XBOX::CharSet inCS=XBOX::VTC_UTF_8);
		bool        SetBinaryData(const void* data, sLONG datalen);
        bool        Perform                 (XBOX::VError* outError);
        bool        HasValidProxyCode       (uLONG* outCode) const;
        bool        HasValidResponseCode    (uLONG* outCode) const;
        const char* GetResponseHeader       (const char* inKey) const;
        bool        GetResponseHeader       (const XBOX::VString& inKey, XBOX::VString* outValue) const;
        bool        GetAllResponseHeaders   (XBOX::VString* outValue) const;
        const char* GetContentCPointer      () const;
        uLONG       GetContentLength        () const;


    private :

        CURL*       GetHandle               ()  const;
        void        SetOpts                 ();

        XBOX::VString   fUrl;   
        Buffer          fContent;
        ResponseHeaders fRespHdrs;
        RequestHeaders  fReqHdrs;
        Buffer          fData;
        CURL*           fHandle;
        Method          fMethod;
        bool            fHasValidResponseCode;
        uLONG           fResponseCode;
        bool            fHasValidProxyCode;
        uLONG           fProxyCode;
    };


    //     class codeToLabel
    //     {
    //     public:
    //
    //         XBOX::VString operator()(CURLcode code) const
    //         {
    //             return XBOX::VString(curl_easy_strerror(code));
    //         }
    //     };
}


#endif

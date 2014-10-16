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
#ifndef __VJSENDPOINT__
#define __VJSENDPOINT__



#include "JavaScript/VJavaScript.h"



const XBOX::VError VE_ENDP_IMPL_FAIL_ERROR				= MAKE_VERROR('ENDP', 1000);    //thrown
const XBOX::VError VE_ENDP_SOCKET_CREATION_FAIL_ERROR	= MAKE_VERROR('ENDP', 1001);    //thrown
const XBOX::VError VE_ENDP_ENDPOINT_CREATION_FAIL_ERROR	= MAKE_VERROR('ENDP', 1002);    //thrown
const XBOX::VError VE_ENDP_READ_FAILED					= MAKE_VERROR('ENDP', 1003);    //thrown
const XBOX::VError VE_ENDP_WRITE_FAILED					= MAKE_VERROR('ENDP', 1004);    //thrown
const XBOX::VError VE_ENDP_CLOSE_FAILED					= MAKE_VERROR('ENDP', 1005);    //thrown


class EndPointHandle
{
private :

	EndPointHandle(const EndPointHandle&);	//no copy !
	
	XBOX::VTCPEndPoint* fEp;

public :
	
	EndPointHandle() : fEp(NULL), fIsConnected(false), fIsClosed(false), fReadCount(0), fWriteCount(0) {}
	~EndPointHandle() { ReleaseRefCountable(&fEp); }

	XBOX::VError Create(const XBOX::VString& inDnsName, PortNumber inPort, sLONG inMsTimeout);

	XBOX::VTCPEndPoint* Get() { return fEp; }
	
	bool fIsConnected;
	bool fIsClosed;
	
	uLONG fReadCount;
	uLONG fWriteCount;
};


class XTOOLBOX_API VJSEndPoint : public XBOX::VJSClass<VJSEndPoint, EndPointHandle>
{
 public:

    typedef XBOX::VJSClass<VJSEndPoint, EndPointHandle> inherited;

    static void GetDefinition           (ClassDefinition& outDefinition);
    static void Initialize              (const XBOX::VJSParms_initialize& inParms,   EndPointHandle* inEph);
    static void Finalize                (const XBOX::VJSParms_finalize& inParms,     EndPointHandle* inEph);
    static void Construct               (XBOX::VJSParms_construct& inParms);
    static void CallAsFunction          (XBOX::VJSParms_callAsFunction& ioParms);
	
	static void _Connect				(XBOX::VJSParms_callStaticFunction& ioParms, EndPointHandle* inEph);
    static void _Read					(XBOX::VJSParms_callStaticFunction& ioParms, EndPointHandle* inEph);
    static void _Write					(XBOX::VJSParms_callStaticFunction& ioParms, EndPointHandle* inEph);
    static void _Close                  (XBOX::VJSParms_callStaticFunction& ioParms, EndPointHandle* inEph);

	static void _IsConnected			(XBOX::VJSParms_getProperty& ioParms, EndPointHandle* inEph);
	static void _IsClosed				(XBOX::VJSParms_getProperty& ioParms, EndPointHandle* inEph);
	static void _GetReadCount			(XBOX::VJSParms_getProperty& ioParms, EndPointHandle* inEph);
	static void _GetWriteCount			(XBOX::VJSParms_getProperty& ioParms, EndPointHandle* inEph);
};


#endif

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
#ifndef __SNET_TOOLS__
#define __SNET_TOOLS__


#include "ServerNetTypes.h"


BEGIN_TOOLBOX_NAMESPACE


class ICriticalError;
class VLocalizationManager;


namespace ServerNetTools
{
	IpPolicy XTOOLBOX_API GetIpPolicy();

	//Did we support V6 or V4 stack ?
	bool XTOOLBOX_API HasV6Stack();
	bool XTOOLBOX_API HasV4Stack();
	
	//Should we build a V6 or V4 address from a VString ?
	bool XTOOLBOX_API ConvertFromV6();
	bool XTOOLBOX_API ConvertFromV4();
	
	//Should we ask for a V6 or V4 IP on DNS query ?
	bool XTOOLBOX_API ResolveToV6();
	bool XTOOLBOX_API ResolveToV4();
	bool XTOOLBOX_API ResolveWithV4Fallback();
	
	//Should we list local V6 or V4 interfaces ?
	bool XTOOLBOX_API ListV6();
	bool XTOOLBOX_API ListV4();
	
	//Should we try to bind V6 addr when someone asks for any ?
	bool XTOOLBOX_API PromoteAnyToV6();
	
	//Should we allow/use V4 mapped V6 addresses when we ask for V6 ? 
	bool XTOOLBOX_API WithV4MappedV6();
	
	//Used by 4D *only* to set DB4D and SQL servers default (bundled) certificate in OpenSSL *global* context
	void XTOOLBOX_API Set_4D_DefaultSSLPrivateKey(char* inKey, uLONG inSize);
	void XTOOLBOX_API Set_4D_DefaultSSLCertificate(char* inCertificate, uLONG inSize);

	void XTOOLBOX_API AddIntermediateCertificateDirectory(const VFolder& inCertFolder);

	sLONG XTOOLBOX_API GetSelectIODelay();
	void XTOOLBOX_API SetSelectIODelay(sLONG inDelay);
	
	sLONG XTOOLBOX_API GetDefaultClientIdleTimeOut();
	void XTOOLBOX_API SetDefaultClientIdleTimeOut(sLONG inTimeOut);
	
	//TODO - Legacy code ; Need rewrite.
	uLONG XTOOLBOX_API GetEncryptedPKCS1DataSize ( uLONG inKeySize /* 128 for 1024 RSA; X/8 for X RSA*/, uLONG inDataSize );
	VError XTOOLBOX_API Encrypt (uCHAR* inPrivateKeyPEM, uLONG inPrivateKeyPEMSize, uCHAR* inData, uLONG inDataSize, uCHAR* ioEncryptedData, uLONG* ioEncryptedDataSize);

	sLONG XTOOLBOX_API GetNextSimpleID();
	
	VError XTOOLBOX_API ThrowNetError(VError inError, VString const* inKey1=0, VString const* inKey2=0);
	VError XTOOLBOX_API vThrowNativeCombo(VError inImplErr, int inErrno);
	VError XTOOLBOX_API SignalNetError(VError inError);
	
	sLONG XTOOLBOX_API FillDumpBag(VValueBag* ioBag, const void* inPayLoad, sLONG inPayLoadLen, sLONG inOffset);

	XTOOLBOX_API sLONG InetPtoN(sLONG inFamily, const char* inSrc, void* outDst);
	XTOOLBOX_API const char* InetNtoP(sLONG inFamily, const void* inSrc, char* outDst, sLONG inDstLen);

	VString XTOOLBOX_API GetFirstResolvedAddress(const VString& inDnsName);

	VString XTOOLBOX_API GetFirstLocalAddress();
		
	VString XTOOLBOX_API GetAnyIP();
		
	VString XTOOLBOX_API GetLocalIP(const VTCPEndPoint* inEP);
	VString XTOOLBOX_API GetPeerIP(const VTCPEndPoint* inEP);
	PortNumber XTOOLBOX_API GetLocalPort(const VTCPEndPoint* inEP);
	PortNumber XTOOLBOX_API GetPeerPort(const VTCPEndPoint* inEP);

	bool XTOOLBOX_API IsLocalInterface(const VString& inIP);

    bool XTOOLBOX_API IsLocalName(const VString& inName);
    
	bool XTOOLBOX_API IsLoopBack(const VString& inIP);

	bool XTOOLBOX_API IsAny(const VString& inIP);

	sLONG XTOOLBOX_API GetHostIPs(std::vector<VString>* outIPs);

	sLONG XTOOLBOX_API GetHostIPs(VString* outIPs, const VString& inSep);

	VString XTOOLBOX_API GetLoopbackIP(bool withBrackets=false);

	VString XTOOLBOX_API GetStringFromIP4(IP4 inIP);

	IP4 XTOOLBOX_API GetIP4FromString(const VString& inIP);

	VString XTOOLBOX_API AddIPv6Brackets(const VString& inIP);

	VString XTOOLBOX_API RemoveIPv6Brackets(const VString& inIP);

	bool XTOOLBOX_API AreEquivalentIPs(const VString& addr1, const VString& addr2);

	// BeautifyIPString() is intended to make an IP adress string more user friendly.
	// It produces 'localhost' for local or loopback adresses,
	// and remove ::ffff: prefix for ipv6 encapusalated ipv4 adresses.
	VString XTOOLBOX_API BeautifyIPString( const VString& inIP);
};


//StErrorContextInstaller(bool inKeepingErrors, bool inSilentContext)

//An error context for places where we do not care about errors
class StKillErrorContext : public StErrorContextInstaller
{
public :

	StKillErrorContext() : StErrorContextInstaller(false /*we drop errors*/, true /*from now on, popups are forbidden*/) { }
};

//An error context for loop, to flush errors on each iteration.
class StDropErrorContext : public StErrorContextInstaller
{
public :

	StDropErrorContext() : StErrorContextInstaller(false /*we drop errors*/, false /*popups are allowed*/) { }
};


//An error context fore place where we are interested in errors, but don't want popups
class StSilentErrorContext : public StErrorContextInstaller
{
public :

	StSilentErrorContext() : StErrorContextInstaller(true /*we keep errors*/, true /*from now on, popups are forbidden*/) { }
};


//An error context for places where we are not interested in errors that may preceed a success
class StTmpErrorContext : public StErrorContextInstaller
{
public :
	
	StTmpErrorContext() : StErrorContextInstaller(true /*we keep errors*/, true /*from now on, popups are forbidden*/) { }
};


class XTOOLBOX_API VServerNetManager : public VObject
{
public :
	
	static VServerNetManager* Get();
	
	//Init and DeInit are called in specific app object : V4DCoreApp (Mono and Studio), V4DServer, VRIAServerApp
	static void Init(ICriticalError* inCriticalError=NULL, IpPolicy inIpPolicy=IpForceV4);	
	static void DeInit();

	sLONG GetDefaultClientIdleTimeOut();
	void SetDefaultClientIdleTimeOut(sLONG inTimeOut);
	
	sLONG GetSelectIODelay();
	void SetSelectIODelay(sLONG inDelay);
	
	sLONG GetNextSimpleID();
	
	//Returns inError
	VError SignalCriticalError(VError inError);	
		
	IpPolicy GetIpPolicy() const;
	
	sLONG GetIpStacks() const;
	
#if VERSIONWIN
	//Do not call directly ; use ServerNetTools::InetPtoN and ServerNetTools::InetNtoP instead
	sLONG InetPtoN(sLONG inFamily, const char* inSrc, void* outDst) const;
	const char* InetNtoP(sLONG inFamily, const void* inSrc, char* outDst, size_t inDstLen) const;
#endif

private :
	
	VServerNetManager();
	~VServerNetManager();
	
	
	static VServerNetManager* sInstance;
	
	ICriticalError*	fCriticalError;
	
	sLONG fDefaultClientIdleTimeOut;
	sLONG fSelectIOInactivityTimeOut;
	sLONG fEndPointCounter;
	
	IpPolicy fIpPolicy;
	sLONG fIpStacks;

	sLONG fInitCount;

#if VERSIONWIN

	//Pointer to inet_pton, need vista, 2008 server or better
	typedef int (*InetPtoNPtr)(int, const char*, void*);
	InetPtoNPtr fInetPtoN;

	//Pointer to inet_ntop, need vista, 2008 server or better
	typedef const char* (*InetNtoPPtr)(int, const void*, char*, size_t);
	InetNtoPPtr fInetNtoP;

#endif
};


END_TOOLBOX_NAMESPACE


#endif

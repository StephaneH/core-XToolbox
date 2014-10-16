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
#include "VServerNetPrecompiled.h"

#include "Tools.h"

#include "ICriticalError.h"
#include "VSslDelegate.h"
#include "XML/VXML.h" /* For VLocalizationManager */
#include "VNetAddr.h"

#include "VTCPEndPoint.h"

#include "VProxyManager.h"

#if VERSIONMAC || VERSION_LINUX
	#include <netdb.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <ifaddrs.h>
#endif

//jmo - Until we are sure all DeInit crashes are gone...
#if VERSIONDEBUG
	#define ALLOW_SERVERNET_DEINIT 1
#else
	#define ALLOW_SERVERNET_DEINIT 0
#endif

BEGIN_TOOLBOX_NAMESPACE


VServerNetManager*	VServerNetManager::sInstance=NULL;


VServerNetManager::VServerNetManager() :
	fCriticalError(NULL), fDefaultClientIdleTimeOut(20000 /*20s*/),
	fSelectIOInactivityTimeOut(300 /*ms*/), fEndPointCounter(0),
	fIpPolicy(IpForceV4), fIpStacks(0), fInitCount(0)
#if VERSIONWIN
	,fInetPtoN(NULL), fInetNtoP(NULL)
#endif
{

	VError verr=SslFramework::Init();
	xbox_assert(verr==VE_OK);

#if VERSIONWIN
	fInetPtoN=(InetPtoNPtr)::GetProcAddress(GetModuleHandle("Ws2_32.dll"), "inet_pton");
	fInetNtoP=(InetNtoPPtr)::GetProcAddress(GetModuleHandle("Ws2_32.dll"), "inet_ntop");
#endif
}


VServerNetManager::~VServerNetManager()
{
	VErrorBase::UnregisterLocalizer(kSERVER_NET_SIGNATURE);

	VError verr=SslFramework::DeInit();
	xbox_assert(verr==VE_OK);
}


#if VERSIONWIN

sLONG VServerNetManager::InetPtoN(sLONG inFamily, const char* inSrc, void* outDst) const
{
	if(fInetPtoN==NULL)
		return -1;	// error code for 'some system error occurred'

	return fInetPtoN(inFamily, inSrc, outDst);
}


const char* VServerNetManager::InetNtoP(sLONG inFamily, const void* inSrc, char* outDst, size_t inDstLen) const
{
	if(fInetNtoP==NULL)
		return NULL;	// generic error

	return fInetNtoP(inFamily, inSrc, outDst, inDstLen);
}

#endif


//static
VServerNetManager* VServerNetManager::Get()
{
	if(sInstance==NULL)
		sInstance=new VServerNetManager();

	xbox_assert(sInstance!=NULL);

	return sInstance;
}


//static
void VServerNetManager::Init(ICriticalError* inCriticalError, IpPolicy inIpPolicy)
{
	//Call Get() to force new VServerNetManager() and SslFramework::Init()
	VServerNetManager* manager=VServerNetManager::Get();

	xbox_assert(manager!=NULL);

	if (manager != NULL)
		manager->fInitCount++;

	if(manager!=NULL && inCriticalError!=NULL && manager->fCriticalError==NULL)
		manager->fCriticalError=inCriticalError;


	if(manager!=NULL && manager->fIpStacks==0)
	{

#if VERSIONMAC
#	if VERSIONDEBUG
		if(getenv("SERVERNET_WASTE_SOCKETS")!=NULL)
		{
			sLONG min=kMAX_sLONG, max=-1, count=1234, s;

			for(int i=0 ; i<count ; i++)
			{
				s=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
				min = min<s ? min : s ;
				max = max>s ? max : s ;
			}

			sLONG tbl=getdtablesize();

			DebugMsg ("[%d] VServerNetManager::Init() : Waste %d sockets from %d to %d (table size is %d)\n",
					  VTask::GetCurrentID(), count, min, max, tbl);
		}
#	endif
#endif

		//See if we suport IP v4 and v6 and translate 'auto' to some appropriate policy

		//First, pretend we support v4 only and count local interfaces

		manager->fIpStacks=4;
		manager->fIpPolicy=IpForceV4;

		sLONG v4Count=0;

		VNetAddressList v4List;
		VError verr=v4List.FromLocalInterfaces();

		xbox_assert(verr==VE_OK);

		VNetAddressList::const_iterator v4Cit;

		for(v4Cit=v4List.begin() ; v4Cit!=v4List.end() ; ++v4Cit)
			v4Count++;

		//Then pretend we support v6 only and count local interfaces

		manager->fIpStacks=6;
		manager->fIpPolicy=IpForceV6;

		sLONG v6Count=0;

		VNetAddressList v6List;
		verr=v6List.FromLocalInterfaces();

		xbox_assert(verr==VE_OK);

		VNetAddressList::const_iterator v6Cit;

		for(v6Cit=v6List.begin() ; v6Cit!=v6List.end() ; ++v6Cit)
			v6Count++;

		//Conclude if we run dual stack or not and choose policy

		if(v4Count>0 && v6Count==0)
			manager->fIpStacks=4;
		else if(v4Count>0 && v6Count>0)
			manager->fIpStacks=46;
		else if(v4Count==0 && v6Count>0)
			manager->fIpStacks=6;

		manager->fIpPolicy=inIpPolicy;

		if(manager->fIpPolicy==IpAuto)
		{
			if(manager->fIpStacks==46)
				manager->fIpPolicy=IpPreferV6;
			else if(manager->fIpStacks==6)
				manager->fIpPolicy=IpForceV6;
			else
				manager->fIpPolicy=IpForceV4;
		}

		const char* policyLabel="";

		switch(manager->fIpPolicy)
		{
			case IpForceV4 :
				policyLabel="IpForceV4";
				break;

			case IpAnyIsV6 :
				policyLabel="IpAnyIsV6";
				break;

			case IpPreferV6 :
				policyLabel="IpPreferV6";
				break;

			case IpForceV6 :
				policyLabel="IpForceV6";
				break;

			default	:
				policyLabel="<unknown policy>";
				xbox_assert(false);
		}

		DebugMsg ("[%d] VServerNetManager::Init() : ServerNet IP Policy is %s\n",
				  VTask::GetCurrentID(), policyLabel);

		xbox_assert(verr==VE_OK);

		VProxyManager::Init();
	}
}


//static
void VServerNetManager::DeInit()
{
	if (sInstance!=NULL)
	{
		sInstance->fInitCount--;

		if (sInstance->fInitCount<=0)
		{
			xbox_assert(sInstance->fInitCount==0);

#if ALLOW_SERVERNET_DEINIT

			delete sInstance;
			sInstance=NULL;
            
#else
            
            DebugMsg(CVSTR("Skip ServerNet DeInit\n"));
            
#endif
		}
	}
}


sLONG VServerNetManager::GetDefaultClientIdleTimeOut()
{
	return fDefaultClientIdleTimeOut;
}


void VServerNetManager::SetDefaultClientIdleTimeOut(sLONG inTimeOut)
{
	VInterlocked::Exchange(&fDefaultClientIdleTimeOut, inTimeOut);
}


sLONG VServerNetManager::GetSelectIODelay()
{
	return fSelectIOInactivityTimeOut;
}


void VServerNetManager::SetSelectIODelay(sLONG inDelay)
{
	VInterlocked::Exchange(&fSelectIOInactivityTimeOut, inDelay);
}


sLONG VServerNetManager::GetNextSimpleID()
{
	return VInterlocked::Increment(&fEndPointCounter);
}


//static
VError VServerNetManager::SignalCriticalError(VError inError)
{
	xbox_assert(fCriticalError!=NULL);

	if(fCriticalError!=NULL)
		fCriticalError->SignalCriticalNetworkError(inError);

	return inError;
}


IpPolicy VServerNetManager::GetIpPolicy() const
{
	return fIpPolicy;
}


sLONG VServerNetManager::GetIpStacks() const
{
	return fIpStacks;
}


IpPolicy ServerNetTools::GetIpPolicy()
{
	return VServerNetManager::Get()->GetIpPolicy();
}


bool ServerNetTools::HasV6Stack()
{
	sLONG stacks=VServerNetManager::Get()->GetIpStacks();

	if(stacks==6 || stacks==46)
		return true;

	return false;
}


bool ServerNetTools::HasV4Stack()
{
	sLONG stacks=VServerNetManager::Get()->GetIpStacks();

	if(stacks==4 || stacks==46)
		return true;

	return false;
}


bool ServerNetTools::ConvertFromV6()
{
	IpPolicy policy=GetIpPolicy();

	if(policy==IpAnyIsV6 || policy==IpPreferV6 || policy==IpForceV6)
		return true;

	return false;
}


bool ServerNetTools::ConvertFromV4()
{
	IpPolicy policy=GetIpPolicy();

	if(policy==IpForceV4 || policy==IpAnyIsV6 || policy==IpPreferV6)
		return true;

	return false;
}


bool ServerNetTools::ResolveToV6()
{
	IpPolicy policy=GetIpPolicy();

	return policy==IpPreferV6 || policy==IpForceV6;
}


bool ServerNetTools::ResolveToV4()
{
	IpPolicy policy=GetIpPolicy();

#if !WITH_SELECTIVE_GETADDRINFO

	return policy==IpForceV4 || policy==IpAnyIsV6;

#else

	//Contournement pour le pb de resolution avec getaddrinfo : puisqu'on n'arrive pas
	//a avoir de V4-mapped v6, alors on met de l'eau dans notre vin et on accepte du V4
	return policy==IpForceV4  || WithV4MappedV6();

#endif
}


bool XTOOLBOX_API ServerNetTools::ResolveWithV4Fallback()
{
	IpPolicy policy=GetIpPolicy();

	return policy==IpPreferV6;
}


bool ServerNetTools::ListV6()
{
	IpPolicy policy=GetIpPolicy();

	return policy==IpPreferV6 || policy==IpForceV6;
}


bool ServerNetTools::ListV4()
{
	IpPolicy policy=GetIpPolicy();

	return policy==IpForceV4 || policy==IpAnyIsV6;
}


bool ServerNetTools::PromoteAnyToV6()
{
	IpPolicy policy=GetIpPolicy();

	return HasV6Stack() && policy==IpAnyIsV6;
}


bool ServerNetTools::WithV4MappedV6()
{
	IpPolicy policy=GetIpPolicy();

	return policy==IpAnyIsV6 || policy==IpPreferV6;
}


//TODO : Legacy code ; Need rewrite
uLONG ServerNetTools::GetEncryptedPKCS1DataSize(uLONG inKeySize /* 128 for 1024 RSA; X/8 for X RSA*/, uLONG inDataSize)
{
	return SslFramework::GetEncryptedPKCS1DataSize(inKeySize, inDataSize);
}


VError ServerNetTools::Encrypt(uCHAR* inPrivateKeyPEM, uLONG inPrivateKeyPEMSize, uCHAR* inData, uLONG inDataSize,
							   uCHAR* ioEncryptedData, uLONG* ioEncryptedDataSize)
{

	if(inPrivateKeyPEM==NULL || inPrivateKeyPEMSize<886 || inData==NULL || inDataSize==0 ||
	   ioEncryptedData==NULL || ioEncryptedDataSize==0)
		return VE_SRVR_INVALID_PARAMETER;

	//Copy/Paste from Security::Encrypt
	int nTenIndex = 31;

	while( nTenIndex <= 811 )
	{
		inPrivateKeyPEM [ nTenIndex ] = 10;
		nTenIndex += 65;
	}

	inPrivateKeyPEM [ 856 ] = 10;
	inPrivateKeyPEM [ 886 ] = 10;

	VError verr=SslFramework::Encrypt(inPrivateKeyPEM, inPrivateKeyPEMSize, inData, inDataSize, ioEncryptedData, ioEncryptedDataSize);

	return verr;
}


sLONG ServerNetTools::GetSelectIODelay()
{
	return VServerNetManager::Get()->GetSelectIODelay();
}


void ServerNetTools::SetSelectIODelay(sLONG inDelay)
{
	VServerNetManager::Get()->SetSelectIODelay(inDelay);
}


sLONG ServerNetTools::GetNextSimpleID()
{
	return VServerNetManager::Get()->GetNextSimpleID();
}


VError ServerNetTools::ThrowNetError(VError inError, VString const* inKey1, VString const* inKey2)
{
	VErrorBase*	verrBase=new VErrorBase(inError, 0);

	if(inKey1!=NULL)
	{
		VValueBag* vvBagParameters=verrBase->GetBag();

		if(vvBagParameters!=NULL)
		{
			vvBagParameters->SetString("key1", *inKey1);

			if(inKey2!=NULL)
				vvBagParameters->SetString("key2", *inKey2);
		}
	}

	VTask::GetCurrent()->PushRetainedError(verrBase);

	return inError;
}


VError ServerNetTools::vThrowNativeCombo(VError inImplErr, int inErrno)
{
	VErrorTaskContext* errCtx=VTask::GetErrorContext(false);

	if(errCtx!=NULL)
	{
		//Do not push an error that is arlready in the stack. Prevent uncontrolled stack growth in mad loops.
		bool foundImplErr=(errCtx->Find(inImplErr)!=NULL);

		if(!foundImplErr)
		{
			//Exact native error - ie ECONNRESET, EBADF or ENOTSOCK (these are not the same !)
			vThrowNativeError(inErrno);

			//Value added error usefull for VStuff.cpp - ie VE_SOCK_READ_FAILED
			return vThrowError(inImplErr);
		}
	}

	return inImplErr;
}


VError ServerNetTools::SignalNetError(VError inError)
{
 	return VServerNetManager::Get()->SignalCriticalError(inError);
}


sLONG ServerNetTools::GetDefaultClientIdleTimeOut()
{
	return VServerNetManager::Get()->GetDefaultClientIdleTimeOut();
}


void ServerNetTools::SetDefaultClientIdleTimeOut(sLONG inTimeOut)
{
	return VServerNetManager::Get()->SetDefaultClientIdleTimeOut(inTimeOut);
}


void ServerNetTools::Set_4D_DefaultSSLPrivateKey ( char* inKey, uLONG inSize )
{
	VMemoryBuffer<> keyBuffer;

	keyBuffer.PutData(0 /*offset in mem buffer*/, inKey, inSize);

	VError verr=SslFramework::SetDefaultPrivateKey(keyBuffer);

	xbox_assert(verr==VE_OK);
}


void ServerNetTools::Set_4D_DefaultSSLCertificate ( char* inCertificate, uLONG inSize )
{
	VMemoryBuffer<> certBuffer;

	certBuffer.PutData(0 /*offset in mem buffer*/, inCertificate, inSize);

	VError verr=SslFramework::SetDefaultCertificate(certBuffer);

	xbox_assert(verr==VE_OK);
}


void ServerNetTools::AddIntermediateCertificateDirectory(const VFolder& inCertFolder)
{
	VError verr=SslFramework::AddCertificateDirectory(inCertFolder);

	xbox_assert(verr==VE_OK);
}


sLONG ServerNetTools::FillDumpBag(VValueBag* ioBag, const void* inPayLoad, sLONG inPayLoadLen, sLONG inOffset)
{
	if(ioBag==NULL || inPayLoad==NULL || inPayLoadLen<=0 || inOffset>=inPayLoadLen)
		return kMAX_sLONG;

	const char* plStart=reinterpret_cast<const char*>(inPayLoad)+inOffset;
	const char* plPast=plStart+(inPayLoadLen-inOffset<100 ? inPayLoadLen-inOffset : 100);

	int len=plPast-plStart;

	char bufStart[100];
	char *bufPast=bufStart+len;

	const char* plPos=NULL;
	char* bufPos=NULL;

	for(plPos=plStart, bufPos=bufStart ; bufPos<bufPast ; plPos++, bufPos++)
		*bufPos=(isprint((unsigned char)(*plPos)) ? *plPos : '.');

	VString dump(bufStart, len, VTC_US_ASCII);

	ILoggerBagKeys::dump_offset.Set(ioBag, inOffset);
	ILoggerBagKeys::dump_buffer.Set(ioBag, dump);

	return inOffset+len;
}


sLONG XTOOLBOX_API ServerNetTools::InetPtoN(sLONG inFamily, const char* inSrc, void* outDst)
{
	sLONG res=0;

#if VERSIONWIN
	res=VServerNetManager::Get()->InetPtoN(inFamily, inSrc, outDst);
#else
	res=inet_pton(inFamily, inSrc, outDst);
#endif

	return res;
}


XTOOLBOX_API const char* ServerNetTools::InetNtoP(sLONG inFamily, const void* inSrc, char* outDst, sLONG inDstLen)
{
	const char* res=NULL;

#if VERSIONWIN
	res=VServerNetManager::Get()->InetNtoP(inFamily, inSrc, outDst, inDstLen);
#else
	res=inet_ntop(inFamily, inSrc, outDst, inDstLen);
#endif

	return res;
}


VString ServerNetTools::GetFirstResolvedAddress(const XBOX::VString& inDnsName)
{
	VNetAddressList addrs;

	//jmo - todo : Virer ce port 80, revoir FromDnsQuery
	VError verr=addrs.FromDnsQuery(inDnsName, 80);

	VNetAddressList::const_iterator cit=addrs.begin();

	if(cit!=addrs.end())
		return cit->GetIP();

	return VString();
}


VString ServerNetTools::GetFirstLocalAddress()
{
	VNetAddressList addrs;

	addrs.FromLocalInterfaces();

	VNetAddressList::const_iterator cit;

	for(cit=addrs.begin() ; cit!=addrs.end() ; ++cit)
		if(cit->IsLocal())
			return cit->GetIP();

	//jmo - todo : Vérifier tout ça !

	for(cit=addrs.begin() ; cit!=addrs.end() ; ++cit)
		if(cit->IsLocallyAssigned())
			return cit->GetIP();

	cit=addrs.begin();

	if(cit!=addrs.end())
			return cit->GetIP();

	return VString();
}


VString ServerNetTools::GetAnyIP()
{
	return VNetAddress::GetAnyIP();
}


VString ServerNetTools::GetLocalIP(const VTCPEndPoint* inEP)
{
	if(inEP==NULL)
		return VString();

	VNetAddress localIp;

	localIp.FromLocalAddr(inEP->GetRawSocket());

	return localIp.GetIP();
}


VString ServerNetTools::GetPeerIP(const VTCPEndPoint* inEP)
{
	if(inEP==NULL)
		return VString();

	VNetAddress peerIp;

	peerIp.FromPeerAddr(inEP->GetRawSocket());

	return peerIp.GetIP();
}

PortNumber ServerNetTools::GetLocalPort(const VTCPEndPoint* inEP)
{
	if(inEP==NULL)
		return kBAD_PORT;

	VNetAddress localIp;

	localIp.FromLocalAddr(inEP->GetRawSocket());

	return localIp.GetPort();
}


PortNumber ServerNetTools::GetPeerPort(const VTCPEndPoint* inEP)
{
	if(inEP==NULL)
		return kBAD_PORT;

	VNetAddress peerIp;

	peerIp.FromPeerAddr(inEP->GetRawSocket());

	return peerIp.GetPort();
}


bool ServerNetTools::IsLocalInterface(const VString& inIP)
{
	StKillErrorContext errCtx;

	VNetAddress inAddr;

	VError verr=inAddr.FromIpAndPort(inIP);

	if(verr!=VE_OK)
		return false;

	VNetAddressList addrs;

	addrs.FromLocalInterfaces();

	VNetAddressList::const_iterator cit=addrs.begin();

	while(cit!=addrs.end())
	{
		if(AreEquivalentIPs(inIP, cit->GetIP()))
			return true;

		++cit;
	}

	return false;
}

bool ServerNetTools::IsLocalName(const VString& inName)
{
	StKillErrorContext errCtx;
    
    
	VNetAddressList addrs;
    
	VError verrDnsQuery=addrs.FromDnsQuery(inName, 1234);
    
    VNetAddressList::const_iterator addrIt;

    
    VNetAddressList ifs;
    
    VError verrLocalIfs=ifs.FromLocalInterfaces();
    
    VNetAddressList::const_iterator ifIt=ifs.begin();
    
    
    if(verrDnsQuery==VE_OK && verrLocalIfs==VE_OK)
        for(addrIt=addrs.begin() ; addrIt!=addrs.end() ; ++addrIt)
            for(ifIt=ifs.begin() ; ifIt!=ifs.end() ; ++ifIt)
                if(AreEquivalentIPs(addrIt->GetIP(), ifIt->GetIP()))
                    return true;

	
    return false;
}

bool ServerNetTools::IsAny(const VString& inIP)
{
	StKillErrorContext errCtx;

	if(inIP==VString("0"))
		return true;

	if(ConvertFromV4() && inIP==VString("0.0.0.0"))
		return true;

	if(ConvertFromV6() && (inIP==VString("::") || inIP==VString("[::]")))
		return true;

	return false;
}


bool ServerNetTools::IsLoopBack(const VString& inIP)
{
	StKillErrorContext errCtx;

	VString ip=XBOX::ServerNetTools::RemoveIPv6Brackets(inIP);

	VNetAddress addr;

	VError verr=addr.FromIpAndPort(inIP);

	if(verr!=VE_OK)
		return false;

	return addr.IsLoopBack();
}


sLONG ServerNetTools::GetHostIPs(std::vector<VString>* outIPs)
{
	if(outIPs==NULL)
		return -1;

	outIPs->clear();

	VNetAddressList	addrs;
	VError verr=addrs.FromLocalInterfaces();

	if(verr!=VE_OK)
		return 0;

	VNetAddressList::const_iterator cit;


	for(cit=addrs.begin() ; cit!=addrs.end() ; ++cit)
	{
		if(cit->IsLoopBack())
			continue;

		outIPs->push_back(cit->GetIP());
	}

	if(outIPs->size()==0)
		outIPs->push_back(GetLoopbackIP(false));

	return outIPs->size();
}


sLONG XTOOLBOX_API ServerNetTools::GetHostIPs(VString* outIPs, const VString& inSep)
{
	if(outIPs==NULL)
		return -1;

	outIPs->Clear();

	std::vector<VString> IPs;

	sLONG count=GetHostIPs(&IPs);

	sLONG i;

	for(i=0 ; i<count ; i++)
	{
		if(i>0)
			outIPs->AppendString(inSep);

		outIPs->AppendString(IPs[i]);
	}

	return count;
}


VString ServerNetTools::GetLoopbackIP(bool inWithBrackets)
{
	if(inWithBrackets && ListV6())
		return VString("[")+VNetAddress::GetLoopbackIP()+VString("]");

	return VNetAddress::GetLoopbackIP();
}


VString ServerNetTools::AddIPv6Brackets(const VString& inIP)
{
	StSilentErrorContext errCtx;

	VNetAddress addr;
	VError verr=addr.FromIpAndPort(inIP, kBAD_PORT);

	if(verr==VE_OK && addr.IsV6())
		return VString("[")+addr.GetIP()+VString("]");

	return inIP;
}


VString ServerNetTools::RemoveIPv6Brackets(const VString& inIP)
{
	if(inIP.BeginsWith("[") && inIP.EndsWith("]"))
	{
		VString tmp(inIP);
		tmp.SubString(2, tmp.GetLength()-2);
		return tmp;
	}

	return inIP;
}


bool ServerNetTools::AreEquivalentIPs(const VString& inAddr1, const VString& inAddr2)
{
    //DebugMsg(VString("AreEquivalentIPs : ").AppendString(inAddr1).AppendString(" and ").AppendString(inAddr2).AppendString("\n"));
    
	StKillErrorContext errCtx;

	VNetAddress addr1, addr2;

	VError verr1=addr1.FromIpAndPort((RemoveIPv6Brackets(inAddr1)));
	VError verr2=addr2.FromIpAndPort((RemoveIPv6Brackets(inAddr2)));

	if(verr1!=VE_OK || verr2!=VE_OK)
		return false;

	if(addr1.IsLoopBack() && addr2.IsLoopBack())
		return true;

	if(addr1.IsV4() || (WithV4MappedV6() && addr1.IsV4MappedV6()))
	{
		if(addr2.IsV4() || (WithV4MappedV6() && addr2.IsV4MappedV6()))
		{
			IP4 ip1;
			addr1.FillIpV4(&ip1);

			IP4 ip2;
			addr2.FillIpV4(&ip2);

			return ip1==ip2;
		}

		return false;
	}

	IP6 ip1;
	addr1.FillIpV6(&ip1);

	IP6 ip2;
	addr2.FillIpV6(&ip2);

	return memcmp(&ip1, &ip2, sizeof(IP6))==0;
}


VString ServerNetTools::BeautifyIPString( const VString& inIP)
{
	VString ipString( inIP);

	if (ServerNetTools::IsLocalInterface( inIP) || ServerNetTools::IsLoopBack( inIP) )
		ipString = "localhost";
	else
	{
		VNetAddress addr( inIP);
		if (WithV4MappedV6() && addr.IsV4MappedV6() && testAssert( inIP.FindRawString( CVSTR( "::ffff:")) == 1) )
		{
			ipString.Remove( 1, 7);
		}
	}
	return ipString;
}

VString ServerNetTools::GetStringFromIP4(IP4 inIP)
{
	VNetAddress addr(inIP);

	return addr.GetIP();
}


IP4 XTOOLBOX_API ServerNetTools::GetIP4FromString(const VString& inIP)
{
	VNetAddress addr(inIP);

	xbox_assert(addr.IsV4() || (WithV4MappedV6() && addr.IsV4MappedV6()));

	IP4 ip=INADDR_ANY;

	if(addr.IsV4() || (WithV4MappedV6() && addr.IsV4MappedV6()))
		addr.FillIpV4(&ip);

	return ip;
}




END_TOOLBOX_NAMESPACE

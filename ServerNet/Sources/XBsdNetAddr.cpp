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


#include "XBsdNetAddr.h"
#include "VNetAddr.h"

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netinet/tcp.h>
#include <net/if.h>


#include "Tools.h"


BEGIN_TOOLBOX_NAMESPACE


using namespace ServerNetTools;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  XBsdNetAddr
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


XBsdNetAddr::XBsdNetAddr() : fIndex(-1)
{
	memset(&fSockAddr, 0, sizeof(fSockAddr));
}


XBsdNetAddr::XBsdNetAddr(const sockaddr_storage& inSockAddr) : fIndex(-1)
{
	memcpy(&fSockAddr, &inSockAddr, sizeof(fSockAddr));
}


XBsdNetAddr::XBsdNetAddr(const sockaddr_in& inSockAddr) : fIndex(-1)
{
	memset(&fSockAddr, 0, sizeof(fSockAddr));
	memcpy(&fSockAddr, &inSockAddr, sizeof(inSockAddr));
}


XBsdNetAddr::XBsdNetAddr(const sockaddr_in6& inSockAddr) : fIndex(-1)
{
	memset(&fSockAddr, 0, sizeof(fSockAddr));
	memcpy(&fSockAddr, &inSockAddr, sizeof(inSockAddr));
}


XBsdNetAddr::XBsdNetAddr(const sockaddr* inSockAddr, sLONG inIfIndex) : fIndex(inIfIndex)
{
	memset(&fSockAddr, 0, sizeof(fSockAddr));

	if(inSockAddr->sa_family==AF_INET)
		memcpy(&fSockAddr, inSockAddr, sizeof(sockaddr_in));
	else if(inSockAddr->sa_family==AF_INET6)
		memcpy(&fSockAddr, inSockAddr, sizeof(sockaddr_in6));
}


XBsdNetAddr::XBsdNetAddr(IP4 inIp, PortNumber inPort) : fIndex(-1)
{
	sockaddr_in v4;
	::memset(&v4,0,sizeof(v4));
	v4.sin_family=AF_INET;
	v4.sin_addr.s_addr=htonl(inIp);
	v4.sin_port=htons(inPort);
	
	memset(&fSockAddr, 0, sizeof(fSockAddr));
	memcpy(&fSockAddr, &v4, sizeof(v4));
}


VError XBsdNetAddr::FromIP4AndPort(IP4 inIp, PortNumber inPort)
{
	sockaddr_in v4;
	::memset(&v4,0,sizeof(v4));
	v4.sin_family=AF_INET;
	v4.sin_addr.s_addr=inIp;
	v4.sin_port=htons(inPort);
	
	memset(&fSockAddr, 0, sizeof(fSockAddr));
	memcpy(&fSockAddr, &v4, sizeof(v4));
	
	//fSockAddr.ss_len=sizeof(sockaddr_in);
	
	return VE_OK;
}


VError XBsdNetAddr::FromIP6AndPort(IP6 inIp, PortNumber inPort)
{
	sockaddr_in6 v6;
	::memset(&v6,0,sizeof(v6));
	v6.sin6_family=AF_INET6;
	v6.sin6_addr=inIp;
	v6.sin6_port=htons(inPort);
	
	memset(&fSockAddr, 0, sizeof(fSockAddr));
	memcpy(&fSockAddr, &v6, sizeof(v6));
	
	//fSockAddr.ss_len=sizeof(sockaddr_in6);
	
	return VE_OK;
}


VError XBsdNetAddr::FromLocalAddr(Socket inSock)
{
	if(inSock==kBAD_SOCKET)
		vThrowError(VE_INVALID_PARAMETER);
	
	sockaddr_storage addr;
	::memset(&addr,0,sizeof(addr));
	socklen_t len=sizeof(addr);
	
	int err=getsockname(inSock, reinterpret_cast<sockaddr*>(&addr), &len);
	
	if(err==-1)
		return vThrowNativeError(errno);
	
	SetAddr(addr);
	
	xbox_assert(len==GetAddrLen());
	
	return VE_OK;
}


VError XBsdNetAddr::FromPeerAddr(Socket inSock)
{
	if(inSock==kBAD_SOCKET)
		vThrowError(VE_INVALID_PARAMETER);
	
	sockaddr_storage addr;
	::memset(&addr,0,sizeof(addr));
	socklen_t len=sizeof(addr);

	int err=getpeername(inSock, reinterpret_cast<sockaddr*>(&addr), &len);
	
	if(err==-1)
		return vThrowNativeError(errno);
	
	SetAddr(addr);
	
	xbox_assert(len==GetAddrLen());
	
	return VE_OK;
}

VError XBsdNetAddr::FromIpAndPort(const VString& inIP, PortNumber inPort)
{
	StTmpErrorContext errCtx;

	VError verr=VE_OK;
	
	if(ConvertFromV4())
		verr=FromIpAndPort(AF_INET, inIP, inPort);
		
	if(verr!=VE_OK && ConvertFromV6())
		verr=FromIpAndPort(AF_INET6, inIP, inPort);
		
	if(verr==VE_OK )
		errCtx.Flush();
	
	return verr;
}


VError XBsdNetAddr::FromIpAndPort(sLONG inFamily, const VString& inIP, PortNumber inPort)
{
	//Small bug (feature ?) an IP v6 string too long is truncated and may become correct.
	
	char src[INET6_ADDRSTRLEN]; //with trailing 0
	
	VSize len=inIP.ToBlock(src, sizeof(src), VTC_US_ASCII, true /*WithTrailingZero*/, false /*inWithLengthPrefix*/);
	
	if(len<=0)
		return vThrowError(VE_INVALID_PARAMETER);
		
	sLONG res=0;
	
	switch(inFamily)
	{
		case AF_INET :
		{
			IP4 dst;

			res=inet_pton(AF_INET, src, &dst);

			if(res>0)
				return FromIP4AndPort(dst, inPort);
			
			break;
		}
			
		case AF_INET6 :
		{
			IP6 dst;
			
			res=inet_pton(AF_INET6, src, &dst);
			
			if(res>0)
				return FromIP6AndPort(dst, inPort);
			
			break;
		}
			
		default :
			return vThrowError(VE_SOCK_UNEXPECTED_FAMILY);
	}

	if(res<0)	//an error occured
		vThrowNativeCombo(VE_INVALID_PARAMETER, errno);
	
	//if(res==0) ... not parsable (according to family)
	return vThrowError(VE_INVALID_PARAMETER);
}


void XBsdNetAddr::SetAddr(const sockaddr_storage& inSockAddr)
{
	memcpy(&fSockAddr, &inSockAddr, sizeof(fSockAddr));
}


const sockaddr* XBsdNetAddr::GetAddr() const
{
	return reinterpret_cast<const sockaddr*>(&fSockAddr);
}


sLONG XBsdNetAddr::GetAddrLen() const 
{
	switch(reinterpret_cast<const sockaddr*>(&fSockAddr)->sa_family)
	{
		case AF_INET :
			return sizeof(sockaddr_in);
			
		case AF_INET6 :
			return sizeof(sockaddr_in6);
	}
	
	return sizeof(fSockAddr);
}


VString XBsdNetAddr::GetIP(sLONG* outVersion) const
{	
	char buf[INET6_ADDRSTRLEN];
	memset(buf, 0, sizeof(buf));
	
	const char* ip=NULL;
	
	const sockaddr* addr=reinterpret_cast<const sockaddr*>(&fSockAddr);
	
	switch(addr->sa_family)
	{
		case AF_INET :
		{
			if(outVersion!=NULL)
				*outVersion=4;

			sockaddr_in* v4=(sockaddr_in*)&fSockAddr;
			ip=inet_ntop(AF_INET, &v4->sin_addr, buf, sizeof(buf));

			break;
		}
			
		case AF_INET6 :
		{
			if(outVersion!=NULL)
				*outVersion=6;

			sockaddr_in6* v6=(sockaddr_in6*)&fSockAddr;
			ip=inet_ntop(AF_INET6, &v6->sin6_addr, buf, sizeof(buf));

			break;
		}
			
		default :
			vThrowError(VE_SOCK_UNEXPECTED_FAMILY);
	}
	
	if(ip==NULL)
	{
		vThrowNativeError(errno);
	
		return VString();
	}

	return VString(ip, strlen(ip), VTC_UTF_8);
}


PortNumber XBsdNetAddr::GetPort() const
{
	const sockaddr* addr=reinterpret_cast<const sockaddr*>(&fSockAddr);

	PortNumber port=0;
	
	switch(addr->sa_family)
	{
		case AF_INET :
		{
			sockaddr_in* v4=(sockaddr_in*)&fSockAddr;
			port=ntohs(v4->sin_port);
			break;
		}
			
		case AF_INET6 :
		{	sockaddr_in6* v6=(sockaddr_in6*)&fSockAddr;
			port=ntohs(v6->sin6_port);
			break;
		}
			
		default :
			vThrowError(VE_SOCK_UNEXPECTED_FAMILY);
	}

	return port;
}


VString XBsdNetAddr::GetName() const
{
	char ifname[IFNAMSIZ];
	ifname[0]=0;
	
	if(fIndex>0)
		return VString(if_indextoname(fIndex, ifname));
	
	return VString();
}

		   
sLONG XBsdNetAddr::GetIndex() const
{
	return fIndex;
}
		   		   

void XBsdNetAddr::FillAddrStorage(sockaddr_storage* outSockAddr) const
{
	if(outSockAddr!=NULL)
		memcpy(outSockAddr, &fSockAddr, sizeof(*outSockAddr));
}


sLONG XBsdNetAddr::GetPfFamily() const
{
	return reinterpret_cast<const sockaddr*>(&fSockAddr)->sa_family;
}


const sockaddr* XBsdNetAddr::GetSockAddr() const
{
	return reinterpret_cast<const sockaddr*>(&fSockAddr);
}


bool XBsdNetAddr::IsV4() const
{
	return reinterpret_cast<const sockaddr*>(&fSockAddr)->sa_family==AF_INET;
}


bool XBsdNetAddr::IsV6() const
{
	return reinterpret_cast<const sockaddr*>(&fSockAddr)->sa_family==AF_INET6;
}


bool XBsdNetAddr::IsV4MappedV6() const
{	
	if(IsV6())
		return IN6_IS_ADDR_V4MAPPED(&reinterpret_cast<const sockaddr_in6*>(&fSockAddr)->sin6_addr);
	
	return false;
}


bool XBsdNetAddr::IsLoopBack() const
{
	if(IsV4() || (WithV4MappedV6() && IsV4MappedV6()))
		return GetV4()==htonl(INADDR_LOOPBACK);
	else if(IsV6())
		return IN6_IS_ADDR_LOOPBACK(GetV6());

	return false;
}


bool XBsdNetAddr::IsAny() const
{
	if(IsV4())
		return GetV4()==htonl(INADDR_ANY);
	else if(IsV6())
		return IN6_IS_ADDR_UNSPECIFIED(GetV6());
	
	return false;
}


IP4 XBsdNetAddr::GetV4() const
{
	if(IsV4())
		return reinterpret_cast<const sockaddr_in*>(&fSockAddr)->sin_addr.s_addr;

	xbox_assert(WithV4MappedV6() && IsV4MappedV6());

	const IP6* ip6=GetV6();
	const IP4* ip4=reinterpret_cast<const IP4*>(ip6);

	return ip4 ? ip4[3] : 0;
}


const IP6* XBsdNetAddr::GetV6() const
{
	return &reinterpret_cast<const sockaddr_in6*>(&fSockAddr)->sin6_addr;
}



VError XBsdAddrLocalQuery::FillAddrList()
{
	if(fVAddrList==NULL)
		return VE_INVALID_PARAMETER;
	
	/* Pour mémoire, ce qu'on récurpère ici :

	 struct ifaddrs
	 {
		struct ifaddrs	*ifa_next;
		char			*ifa_name;
		unsigned int	ifa_flags;
		struct sockaddr	*ifa_addr;
		struct sockaddr	*ifa_netmask;
		struct sockaddr	*ifa_dstaddr;
		void			*ifa_data;
	 };
	 */
	
	
	ifaddrs* ifList=NULL;
	
	if(getifaddrs(&ifList)==-1)
	{
		return vThrowNativeCombo(VE_SRVR_FAILED_TO_LIST_INTERFACES, errno);
	}
	
	ifaddrs* ifPtr=NULL;
	
	for(ifPtr=ifList ; ifPtr!=NULL ; ifPtr=ifPtr->ifa_next)
	{
		if (ifPtr->ifa_addr==NULL)
			continue;
		
		int family=ifPtr->ifa_addr->sa_family;
		
		if((family==AF_INET && ListV4()) || (family==AF_INET6 && ListV6()))
		{	
			//If the interface is not found, a value of 0 is returned and errno is set to ENXIO.
			sLONG ifIndex=if_nametoindex(ifPtr->ifa_name);
			
			xbox_assert(ifIndex!=0);   
		
			if(ifIndex==0)
			{
				ifIndex=-1;			 
				vThrowNativeError(errno);
			}

			const XBsdNetAddr xaddr(ifPtr->ifa_addr, ifIndex);
			
			fVAddrList->PushXNetAddr(xaddr);
		}
	}
	
	freeifaddrs(ifList);

	return VE_OK;
}


VError XBsdAddrDnsQuery::FillAddrList(const VString& inDnsName, PortNumber inPort, EProtocol inProto)
{
	//Take a shortcut if we have an IP in inDnsName
	//(Do not deal with IP v4/v6 resolve policy if there is nothing to resolve)
	if(inPort!=kBAD_PORT)
	{
		StKillErrorContext errCtx;
		XBsdNetAddr tmp;
		VError verr=tmp.FromIpAndPort(inDnsName, inPort);
	
		if(verr==VE_OK)
		{
			fVAddrList->PushXNetAddr(tmp);
			return VE_OK;
		}
	}

	//Prepare the parameters for getaddrinfo.
	
	char utf8DnsName[255];	//Max DNS name
	
	VSize n=inDnsName.ToBlock(utf8DnsName, sizeof(utf8DnsName), VTC_UTF_8, false /*without trailing 0*/, false /*no lenght prefix*/);
	
	if(n==sizeof(utf8DnsName))
	{
		return vThrowError(VE_INVALID_PARAMETER);	//UTF8 path is too long ; we do not support that.
	}
	
	utf8DnsName[n]=0;
	
	
	char port[sizeof("65535")];	//Highest tcp port. 
	int err=snprintf(port, sizeof(port), "%ld", (long)inPort);
	
	if(err<0 || err>sizeof(port))
	{
		return vThrowError(VE_INVALID_PARAMETER);
	}
		
	
    addrinfo hints;
	::memset(&hints,0,sizeof(hints));
	
#if WITH_SELECTIVE_GETADDRINFO
		
	if(ResolveToV4() && !ResolveToV6())
	    hints.ai_family=AF_INET;
	else if(ResolveToV6() && !ResolveToV4())
	{
		hints.ai_family=AF_INET6;
		
		if(WithV4MappedV6())
			hints.ai_flags|=AI_V4MAPPED|AI_ALL;
	}
	else
		hints.ai_family=AF_UNSPEC, hints.ai_flags|=AI_ADDRCONFIG;
		
	if(inProto==UDP)
		hints.ai_socktype=SOCK_DGRAM, hints.ai_protocol=IPPROTO_UDP;
	else
		hints.ai_socktype=SOCK_STREAM, hints.ai_protocol=IPPROTO_TCP;

#endif	
    
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= __MAC_10_6
	hints.ai_flags=AI_NUMERICSERV; //Numeric service only.
#endif
	
	addrinfo* infoHead=NULL;	//head of addr list
	
	do
		err=getaddrinfo(utf8DnsName, (inPort!=kBAD_PORT ? port : NULL), &hints, &infoHead);
	while(err==EAI_SYSTEM && errno==EINTR);
	
	if (err!=0)
	{		
		DebugMsg("[%d] XBsdAddrDnsQuery::FillAddrList() : getaddrinfo for node %s and service %s failed with err %d, %s\n",
				VTask::GetCurrentID(), utf8DnsName, port, err, gai_strerror(err));

		if(infoHead!=NULL)
			freeaddrinfo(infoHead);
		
		return vThrowNativeError(errno);
	}

	addrinfo* v4Fallback=NULL;
	
    for(addrinfo* infoPtr=infoHead ; infoPtr!=NULL ; infoPtr=infoPtr->ai_next)
	{
		/* Pour mémoire, ce qu'on récurpère ici :
		 
		 struct addrinfo
		 {
		 int	ai_flags;
		 int	ai_family;
		 int	ai_socktype;
		 int	ai_protocol;
		 socklen_t ai_addrlen;
		 char	*ai_canonname;
		 struct	sockaddr *ai_addr;
		 struct	addrinfo *ai_next;
		 };
		 
		 sockaddr_in
		 */
				
		const XBsdNetAddr xaddr(infoPtr->ai_addr);
		
		//xbox_assert(infoPtr->ai_family==xaddr.GetPfFamily());
		//xbox_assert(infoPtr->ai_socktype==SOCK_STREAM);
		//xbox_assert(infoPtr->ai_protocol==IPPROTO_TCP);

#if !WITH_SELECTIVE_GETADDRINFO

		if(ResolveToV4() && !ResolveToV6() && xaddr.IsV6())
			continue;
		
		if(ResolveWithV4Fallback() && v4Fallback==NULL)
			v4Fallback=infoPtr;

		if(ResolveToV6() && !ResolveToV4() && xaddr.IsV4())
			continue;
		
		if(!WithV4MappedV6() && xaddr.IsV4MappedV6())
			continue;
	
		if(infoPtr->ai_protocol==IPPROTO_TCP && inProto==UDP)
			continue;
		
		if(infoPtr->ai_protocol==IPPROTO_UDP && inProto==TCP)
			continue;
		
		VNetAddressList::const_iterator cit;
		
		const VString newIp=xaddr.GetIP();
		
		bool ipAlreadyPushed=false;
		
		for(cit=fVAddrList->begin() ; cit!=fVAddrList->end() ; ++cit)
		{
			const VString ip=cit->GetIP();
			
			if(ip==newIp)
			{
				ipAlreadyPushed=true; 
				break;
			}
		}

		if(ipAlreadyPushed)
			continue;
		
#endif		
		
		fVAddrList->PushXNetAddr(xaddr);
	}

	//According to IP policy, fallback on V4 if we could not find any V6
	if(v4Fallback!=NULL && fVAddrList->begin()==fVAddrList->end())
	{
		const XBsdNetAddr xaddr(v4Fallback->ai_addr);
		fVAddrList->PushXNetAddr(xaddr);
	}
	
	//Info list is no longer needed
	freeaddrinfo(infoHead);
	
	return VE_OK;
}


END_TOOLBOX_NAMESPACE

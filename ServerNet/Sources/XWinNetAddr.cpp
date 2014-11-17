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


#include "XWinNetAddr.h"
#include "VNetAddr.h"

//According to MSDN docs, Iptypes.h should include iphlpapi.h... It seems it's not the case.
#include <Iptypes.h>
#include <iphlpapi.h>

#include "Tools.h"


BEGIN_TOOLBOX_NAMESPACE


using namespace ServerNetTools;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  XWinNetAddr
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


XWinNetAddr::XWinNetAddr() : fIndex(-1)
{
	memset(&fSockAddr, 0, sizeof(fSockAddr));
}


XWinNetAddr::XWinNetAddr(const sockaddr_storage& inSockAddr) : fIndex(-1)
{
	memcpy(&fSockAddr, &inSockAddr, sizeof(fSockAddr));
}


XWinNetAddr::XWinNetAddr(const sockaddr_in& inSockAddr) : fIndex(-1)
{
	memset(&fSockAddr, 0, sizeof(fSockAddr));
	memcpy(&fSockAddr, &inSockAddr, sizeof(inSockAddr));
}


XWinNetAddr::XWinNetAddr(const sockaddr_in6& inSockAddr) : fIndex(-1)
{
	memset(&fSockAddr, 0, sizeof(fSockAddr));
	memcpy(&fSockAddr, &inSockAddr, sizeof(inSockAddr));
}


XWinNetAddr::XWinNetAddr(const sockaddr* inSockAddr, sLONG inIfIndex) : fIndex(inIfIndex)
{
	memset(&fSockAddr, 0, sizeof(fSockAddr));

	if(inSockAddr->sa_family==AF_INET)
		memcpy(&fSockAddr, inSockAddr, sizeof(sockaddr_in));
	else if(inSockAddr->sa_family==AF_INET6)
		memcpy(&fSockAddr, inSockAddr, sizeof(sockaddr_in6));
}


XWinNetAddr::XWinNetAddr(IP4 inIp, PortNumber inPort) : fIndex(-1)
{
	sockaddr_in v4={0};
	v4.sin_family=AF_INET;
	v4.sin_addr.s_addr=htonl(inIp);
	v4.sin_port=htons(inPort);
	
	memset(&fSockAddr, 0, sizeof(fSockAddr));
	memcpy(&fSockAddr, &v4, sizeof(v4));
}


VError XWinNetAddr::FromIP4AndPort(IP4 inIp, PortNumber inPort)
{
	sockaddr_in v4={0};
	v4.sin_family=AF_INET;
	v4.sin_addr.s_addr=inIp;
	v4.sin_port=htons(inPort);
	
	memset(&fSockAddr, 0, sizeof(fSockAddr));
	memcpy(&fSockAddr, &v4, sizeof(v4));
	
	return VE_OK;
}


VError XWinNetAddr::FromIP6AndPort(IP6 inIp, PortNumber inPort)
{
	sockaddr_in6 v6={0};
	v6.sin6_family=AF_INET6;
	v6.sin6_addr=inIp;
	v6.sin6_port=htons(inPort);
	
	memset(&fSockAddr, 0, sizeof(fSockAddr));
	memcpy(&fSockAddr, &v6, sizeof(v6));
	
	return VE_OK;
}


VError XWinNetAddr::FromLocalAddr(Socket inSock)
{
	if(inSock==kBAD_SOCKET)
		vThrowError(VE_INVALID_PARAMETER);
	
	sockaddr_storage addr={0};
	socklen_t len=sizeof(addr);
	
	int err=getsockname(inSock, reinterpret_cast<sockaddr*>(&addr), &len);
	
	if(err==-1)
		return vThrowNativeError(WSAGetLastError());
	
	SetAddr(addr);
	
	xbox_assert(len==GetAddrLen());
	
	return VE_OK;
}


VError XWinNetAddr::FromPeerAddr(Socket inSock)
{
	if(inSock==kBAD_SOCKET)
		vThrowError(VE_INVALID_PARAMETER);
	
	sockaddr_storage addr={0};
	socklen_t len=sizeof(addr);

	int err=getpeername(inSock, reinterpret_cast<sockaddr*>(&addr), &len);
	
	if(err==-1)
		return vThrowNativeError(WSAGetLastError());
	
	SetAddr(addr);
	
	xbox_assert(len==GetAddrLen());
	
	return VE_OK;
}

VError XWinNetAddr::FromIpAndPort(const VString& inIP, PortNumber inPort)
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


VError XWinNetAddr::FromIpAndPort(sLONG inFamily, const VString& inIP, PortNumber inPort)
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

			//Exists on XP and better
			dst=inet_addr(src);

			if(dst!=INADDR_NONE)
				return FromIP4AndPort(dst, inPort);
			
			break;
		}
			
		case AF_INET6 :
		{
			IP6 dst;
			
			res=InetPtoN(AF_INET6, src, &dst);
			
			if(res>0)
				return FromIP6AndPort(dst, inPort);
			
			break;
		}
			
		default :
			return vThrowError(VE_SOCK_UNEXPECTED_FAMILY);
	}

	if(res<0)	//an error occured
		vThrowNativeCombo(VE_INVALID_PARAMETER, WSAGetLastError());
	
	//if(res==0) ... not parsable (according to family)
	return vThrowError(VE_INVALID_PARAMETER);
}


void XWinNetAddr::SetAddr(const sockaddr_storage& inSockAddr)
{
	memcpy(&fSockAddr, &inSockAddr, sizeof(fSockAddr));
}


const sockaddr* XWinNetAddr::GetAddr() const
{
	return reinterpret_cast<const sockaddr*>(&fSockAddr);
}


sLONG XWinNetAddr::GetAddrLen() const 
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


VString XWinNetAddr::GetIP(sLONG* outVersion) const
{	
	char buf[INET6_ADDRSTRLEN];
	memset(buf, 0, sizeof(buf));
	
	const char* ip=NULL;
	
	const sockaddr* addr=reinterpret_cast<const sockaddr*>(&fSockAddr);
	
	if(addr==NULL)
	{
		vThrowError(VE_INVALID_PARAMETER);
		return VString();
	}

	switch(addr->sa_family)
	{
		case AF_INET :
		{
			if(outVersion!=NULL)
				*outVersion=4;

			sockaddr_in* v4=(sockaddr_in*)&fSockAddr;

			//Try NtoP which is thread safe, an if not available, use ntoa
			ip=InetNtoP(AF_INET, &v4->sin_addr, buf, sizeof(buf));

			if(ip==NULL)
			ip=inet_ntoa(v4->sin_addr);

			break;
		}
			
		case AF_INET6 :
		{
			if(outVersion!=NULL)
				*outVersion=6;

			sockaddr_in6* v6=(sockaddr_in6*)&fSockAddr;
			ip=InetNtoP(AF_INET6, &v6->sin6_addr, buf, sizeof(buf));

			break;
		}
			
		default :
			vThrowError(VE_SOCK_UNEXPECTED_FAMILY);
	}
	
	if(ip==NULL)
	{
		vThrowNativeError(WSAGetLastError());
	
		return VString();
}

	return VString(ip, strlen(ip), VTC_UTF_8);
}


PortNumber XWinNetAddr::GetPort() const
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


void XWinNetAddr::FillAddrStorage(sockaddr_storage* outSockAddr) const
{
	if(outSockAddr!=NULL)
		memcpy(outSockAddr, &fSockAddr, sizeof(*outSockAddr));
}


sLONG XWinNetAddr::GetPfFamily() const
{
	return reinterpret_cast<const sockaddr*>(&fSockAddr)->sa_family;
}


const sockaddr* XWinNetAddr::GetSockAddr() const
{
	return reinterpret_cast<const sockaddr*>(&fSockAddr);
}


bool XWinNetAddr::IsV4() const
{
	return reinterpret_cast<const sockaddr*>(&fSockAddr)->sa_family==AF_INET;
}


bool XWinNetAddr::IsV6() const
{
	return reinterpret_cast<const sockaddr*>(&fSockAddr)->sa_family==AF_INET6;
}


bool XWinNetAddr::IsV4MappedV6() const
{	
	if(IsV6())
		return IN6_IS_ADDR_V4MAPPED(&reinterpret_cast<const sockaddr_in6*>(&fSockAddr)->sin6_addr)!=0;
	
	return false;
}


bool XWinNetAddr::IsLoopBack() const
{
	if(IsV4() || (WithV4MappedV6() && IsV4MappedV6()))
		return GetV4()==htonl(INADDR_LOOPBACK);
	else if(IsV6())
		return IN6_IS_ADDR_LOOPBACK(GetV6())!=0;
	
	return false;
}


bool XWinNetAddr::IsAny() const
{
	if(IsV4())
		return GetV4()==htonl(INADDR_ANY);
	else if(IsV6())
		return IN6_IS_ADDR_UNSPECIFIED(GetV6())!=0;
	
	return false;
}


IP4 XWinNetAddr::GetV4() const
{
	if(IsV4())
		return reinterpret_cast<const sockaddr_in*>(&fSockAddr)->sin_addr.s_addr;

	xbox_assert(WithV4MappedV6() && IsV4MappedV6());

	const IP6* ip6=GetV6();
	const IP4* ip4=reinterpret_cast<const IP4*>(ip6);

	return ip4 ? ip4[3] : 0;
}


const IP6* XWinNetAddr::GetV6() const
{
	return &reinterpret_cast<const sockaddr_in6*>(&fSockAddr)->sin6_addr;
}


VError XWinAddrLocalQuery::FillAddrList()
{
	if(fVAddrList==NULL)
		return VE_INVALID_PARAMETER;

	//See doc and examples :
	// http://msdn.microsoft.com/en-us/library/windows/desktop/aa365915(v=vs.85).aspx
	// http://www.ipv6style.jp/files/ipv6/en/apps/20060320_2/GetAdaptersAddresses-EN.c

	ULONG family=AF_UNSPEC;

	if(ListV4() && !ListV6())
	    family=AF_INET;
	else if(ListV6() && !ListV4())
		family=AF_INET6;
		
	///Verifier les flags !
	ULONG flags=GAA_FLAG_SKIP_ANYCAST|GAA_FLAG_SKIP_MULTICAST|GAA_FLAG_SKIP_DNS_SERVER;

	//The recommended method of calling the GetAdaptersAddresses function is to pre-allocate a 15KB working buffer
	char buf[15000];

	PIP_ADAPTER_ADDRESSES addrBuf=(PIP_ADAPTER_ADDRESSES)buf;
	ULONG addrBufLen=sizeof(buf);

    DWORD res=GetAdaptersAddresses(family, flags, NULL /*reserved*/, addrBuf, &addrBufLen);
    
	if(res==ERROR_BUFFER_OVERFLOW)
	{
		addrBuf=(PIP_ADAPTER_ADDRESSES)malloc(addrBufLen);

		if(addrBuf==NULL)
			return vThrowError(VE_MEMORY_FULL);

		res=GetAdaptersAddresses(family, flags, NULL /*reserved*/, addrBuf, &addrBufLen);
	}

	if(res==NO_ERROR)
	{
	    for(PIP_ADAPTER_ADDRESSES adapterAddr=addrBuf ; adapterAddr!=NULL ; adapterAddr=adapterAddr->Next)
		{
			//Obtain the unicast address given to the adapter using the FirstUnicastAddress member.
			for(PIP_ADAPTER_UNICAST_ADDRESS unicastAddr=adapterAddr->FirstUnicastAddress ; unicastAddr!=NULL ; unicastAddr=unicastAddr->Next)
			{
				//Windows gives IPs to down interfaces...
				if(adapterAddr->OperStatus==IfOperStatusUp)
				{
					const XWinNetAddr xaddr(unicastAddr->Address.lpSockaddr, adapterAddr->IfIndex);
			
					fVAddrList->PushXNetAddr(xaddr);
				}
			}
		}
	}

	if(addrBuf!=(PIP_ADAPTER_ADDRESSES)buf)
		free(addrBuf);

	return VE_OK;
}


VError XWinAddrDnsQuery::FillAddrList(const VString& inDnsName, PortNumber inPort, EProtocol inProto)
{
	//Take a shortcut if we have an IP in inDnsName
	//(Do not deal with IP v4/v6 resolve policy if there is nothing to resolve)
	if(inPort!=kBAD_PORT)
	{
		StKillErrorContext errCtx;
		XWinNetAddr tmp;
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
	int err=_snprintf(port, sizeof(port), "%ld", (long)inPort);
	
	if(err<0 || err>sizeof(port))
	{
		return vThrowError(VE_INVALID_PARAMETER);
	}

    addrinfo hints={0};
		
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

	hints.ai_flags=AI_NUMERICSERV; //Numeric service only.

	addrinfo* infoHead=NULL;	//head of addr list
	
	err=getaddrinfo(utf8DnsName, (inPort!=kBAD_PORT ? port : NULL), &hints, &infoHead);
	
	if (err!=0)
	{
		if(infoHead!=NULL)
			freeaddrinfo(infoHead);
		
		vThrowNativeError(WSAGetLastError());
		return NULL;
	}

	addrinfo* v4Fallback=NULL;
	
    for(addrinfo* infoPtr=infoHead ; infoPtr!=NULL ; infoPtr=infoPtr->ai_next)
	{		
		const XWinNetAddr xaddr(infoPtr->ai_addr);

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
		const XWinNetAddr xaddr(v4Fallback->ai_addr);
		fVAddrList->PushXNetAddr(xaddr);
	}
	
	//Info list is no longer needed
	freeaddrinfo(infoHead);
	
	return VE_OK;
}


END_TOOLBOX_NAMESPACE

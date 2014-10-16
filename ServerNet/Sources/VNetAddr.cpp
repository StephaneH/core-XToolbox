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


#include "VNetAddr.h"

#include "Tools.h"


BEGIN_TOOLBOX_NAMESPACE


using namespace ServerNetTools;

VNetAddress::VNetAddress(PortNumber inPort)
{
	if(ListV6())
		fNetAddr.FromIP6AndPort(in6addr_any, inPort);
	else
	fNetAddr.FromIP4AndPort(INADDR_ANY, inPort);
}

/*
 * [XSI] sockaddr_storage
 */
//struct sockaddr_storage {
//	__uint8_t	ss_len;		/* address length */
//	sa_family_t	ss_family;	/* [XSI] address family */
//	char			__ss_pad1[_SS_PAD1SIZE];
//	__int64_t	__ss_align;	/* force structure storage alignment */
//	char			__ss_pad2[_SS_PAD2SIZE];
//};


/*
 * [XSI] Structure used by kernel to store most addresses.
 */
//struct sockaddr {
//	__uint8_t	sa_len;		/* total length */
//	sa_family_t	sa_family;	/* [XSI] address family */
//	char		sa_data[14];	/* [XSI] addr value (actually larger) */
//};

VNetAddress::VNetAddress(const sockaddr_storage& inSockAddr) : fNetAddr(inSockAddr) {}

VNetAddress::VNetAddress(const sockaddr_in& inSockAddr) : fNetAddr(inSockAddr) {}

VNetAddress::VNetAddress(const sockaddr_in6& inSockAddr) : fNetAddr(inSockAddr) {}

VNetAddress::VNetAddress(const XNetAddr& inXAddr) : fNetAddr(inXAddr) {}
	
VNetAddress::VNetAddress(IP4 inIpV4, PortNumber inPort) { vThrowError(fNetAddr.FromIP4AndPort(inIpV4, inPort)); }

VNetAddress::VNetAddress(IP6 inIpV6, PortNumber inPort) { vThrowError(fNetAddr.FromIP6AndPort(inIpV6, inPort)); }

VNetAddress::VNetAddress(const VString& inIp, PortNumber inPort) 
{
	xbox_assert(!inIp.IsEmpty());

	if(ServerNetTools::IsAny(inIp) || inIp.IsEmpty())
		vThrowError(fNetAddr.FromIpAndPort(GetAnyIP(), inPort));
	else
		vThrowError(fNetAddr.FromIpAndPort(inIp, inPort));
}

VError VNetAddress::FromLocalAddr(Socket inSock)
{
	return fNetAddr.FromLocalAddr(inSock);
}

VError VNetAddress::FromPeerAddr(Socket inSock)
{
	return fNetAddr.FromPeerAddr(inSock);
}

VError VNetAddress::FromIpAndPort(const VString& inIP, PortNumber inPort)
{
	return fNetAddr.FromIpAndPort(inIP, inPort);
}

VError VNetAddress::FromAnyIpAndPort(PortNumber inPort)
{
	if(ListV6())
		return fNetAddr.FromIP6AndPort(in6addr_any, inPort);

	return fNetAddr.FromIP4AndPort(INADDR_ANY, inPort);
}


//static
VString VNetAddress::GetAnyIP()
{
	if(ListV6() || PromoteAnyToV6())
		return VString("::");
	
	return VString("0.0.0.0");
}


//static
VString VNetAddress::GetLoopbackIP()
{
	if(ListV6())
		return VString("::1");
	
	return VString("127.0.0.1");
}


void VNetAddress::SetAddr(const sockaddr_storage& inSockAddr)
{
	return fNetAddr.SetAddr(inSockAddr);
}

const sockaddr* VNetAddress::GetAddr() const
{
#if VERSIONDEBUG
			
	VString ip=GetIP();
	PortNumber port=GetPort();
	
	//DebugMsg("VNetAddress::GetAddr() will return a sockaddr* for this IP : %S (on port %d)\n", &ip, port);
			
#endif

	return fNetAddr.GetAddr();
}

sLONG VNetAddress::GetAddrLen() const
{
	return fNetAddr.GetAddrLen();
}

VString VNetAddress::GetIP(sLONG* outVersion) const
{
	return fNetAddr.GetIP(outVersion);
}

#if !VERSIONWIN
VString VNetAddress::GetName() const
{
	return fNetAddr.GetName();
}

sLONG VNetAddress::GetIndex() const
{
	return fNetAddr.GetIndex();
}
#endif

bool VNetAddress::IsV4() const
{
	return fNetAddr.IsV4();
}

bool VNetAddress::IsV6() const
{
	return fNetAddr.IsV6();
}

bool VNetAddress::IsV4MappedV6() const
{	
	if(IsV6())
		return IN6_IS_ADDR_V4MAPPED(fNetAddr.GetV6())!=0;
	
	return false;
}


bool VNetAddress::IsLoopBack() const
{
	return fNetAddr.IsLoopBack();
}


bool VNetAddress::IsAny() const
{
	return fNetAddr.IsAny();
}


bool VNetAddress::IsAPIPA() const
{
	//http://fr.wikipedia.org/wiki/Automatic_Private_Internet_Protocol_Addressing
	//169.254.0.0/16 (0xA9FE0000) APIPA (Automatic Private Internet Protocol Addressing)
	//jmo - APIPA is interesting because it means dhcp is (likely) not working
	if(IsV4())
	{
		IP4 ip=ntohl(fNetAddr.GetV4());
		
		if((ip&0xFFFF0000)==0xA9FE0000)
			return true;
	}
	
	return false;
}


bool VNetAddress::IsULA(bool* outLocallyAssigned) const
{
	//http://en.wikipedia.org/wiki/Unique_Local_Address
	//fc00::/7
	if(IsV6())
		if((fNetAddr.GetV6()->s6_addr[0] & 0xfc) == 0xfc)
		{
			if(outLocallyAssigned!=NULL)
				*outLocallyAssigned=((fNetAddr.GetV6()->s6_addr[0] & 0x01) == 0x01);
			
			return true;
		}
	
	return false;
}


bool VNetAddress::IsLocal() const
{
	if(IsV4() || (WithV4MappedV6() && IsV4MappedV6()))
	{	
		//http://en.wikipedia.org/wiki/Private_Network#Private_IPv4_address_spaces
		//Class A - 10.0.0.0/24 (0x0A000000)
		//Class B - 172.16.0.0/20 (0xAC100000)
		//Class C - 192.168.0.0/16 (xC0A80000)

		IP4 ip=ntohl(fNetAddr.GetV4());
		
		if((ip&0xFF000000)==0x0A000000) 
			return true;
		
		if((ip&0xFFF00000)==0xAC100000)
			return true;
		
		if((ip&0xFFFF0000)==0xC0A80000)
			return true;
		
		if(IsAPIPA())
			return true;
	}
	else if(IsV6())
	{
		if(IN6_IS_ADDR_LINKLOCAL(fNetAddr.GetV6()))
			return true;
		
		if(IN6_IS_ADDR_SITELOCAL(fNetAddr.GetV6()))
		{
			VString ip=GetIP();
			
			DebugMsg("[%d] VNetAddress::IsLocal() : %S is a deprecated site local address\n",
					 VTask::GetCurrentID(), &ip);
		}
		
		if(IsULA())
			return true;
	}
	
	return false;
}


bool VNetAddress::IsLocallyAssigned() const
{
	if(IsAPIPA())
		return true;
	
	bool locallyAssigned=false;
	
	if(IsULA(&locallyAssigned))
		return locallyAssigned;
	
	return false;
}


VString VNetAddress::GetProperties(const VString& inSep) const
{
	VString props;

	xbox_assert(IsV4() || IsV6());
	xbox_assert(!(IsV4() && IsV6()));

#if !VERSIONWIN
	if(!GetName().IsEmpty())
		props.AppendString(GetName()).AppendString(inSep);
#endif
	
	props.AppendCString(IsV4() ? "v4" : "");
	props.AppendCString(IsV6() ? "v6" : "");

	if(IsV4MappedV6())
		props.AppendString(inSep).AppendCString("v4 mapped v6");
	
	if(IsLoopBack())
		props.AppendString(inSep).AppendCString("loopback");
	
	if(IsAny())
		props.AppendString(inSep).AppendCString("any");
	
	if(IsAPIPA())
		props.AppendString(inSep).AppendCString("APIPA");
	
	if(IsULA())
		props.AppendString(inSep).AppendCString("ULA");
	
	if(IsLocal())
		props.AppendString(inSep).AppendCString("Local");
	
	if(IsLocallyAssigned())
		props.AppendString(inSep).AppendCString("LocallyAssigned");

	return props;
}

PortNumber VNetAddress::GetPort() const
{
	return fNetAddr.GetPort();
}

void VNetAddress::FillAddrStorage(sockaddr_storage* outSockAddr) const
{
	return fNetAddr.FillAddrStorage(outSockAddr);
}

void VNetAddress::FillIpV4(IP4* outIpV4) const
{
	if(outIpV4!=NULL)
		*outIpV4=fNetAddr.GetV4();
}

void VNetAddress::FillIpV6(IP6* outIpV6) const
{
	if(outIpV6!=NULL)
		memcpy(outIpV6, fNetAddr.GetV6(), sizeof(*outIpV6));
}

sLONG VNetAddress::GetPfFamily() const
{
	return fNetAddr.GetPfFamily();
}

VError VNetAddressList::FromLocalInterfaces()
{
	XAddrLocalQuery query(this);
	
	VError verr=query.FillAddrList();
	
#if VERSIONDEBUG
	VNetAddressList::const_iterator cit;
	
	if(begin()==end())
		DebugMsg("[%d] VNetAddressList::FromLocalInterfaces() : Fail to find a local IP\n", VTask::GetCurrentID());
	else
		for(cit=begin() ; cit!=end() ; ++cit)
		{
			VString ip=cit->GetIP();
			VString props=cit->GetProperties();
			
			//DebugMsg("[%d] VNetAddressList::FromLocalInterfaces() : Found local IP %S %S\n", VTask::GetCurrentID(), &ip, &props);
		}
#endif
	
	return verr;
}

VError VNetAddressList::FromDnsQuery(const VString& inDnsName, PortNumber inPort)
{
	XAddrDnsQuery query(this);
	
	VError verr=query.FillAddrList(inDnsName, inPort);
	
#if VERSIONDEBUG
	VNetAddressList::const_iterator cit;

	if(begin()==end())
		DebugMsg("[%d] VNetAddressList::FromDnsQuery() : Fail to resolve %S\n", VTask::GetCurrentID(), &inDnsName);
	else
		for(cit=begin() ; cit!=end() ; ++cit)
		{
			VString ip=cit->GetIP();
			VString props=cit->GetProperties();
			
			//DebugMsg("[%d] VNetAddressList::FromDnsQuery() : Resolve %S to %S %S\n", VTask::GetCurrentID(), &inDnsName, &ip, &props);
		}
#endif
	
	return verr;
}

VNetAddressList::const_iterator::const_iterator() {}		

bool VNetAddressList::const_iterator::operator==(const const_iterator& other)
{
	return fAddrIt==other.fAddrIt;
}

bool VNetAddressList::const_iterator::operator!=(const const_iterator& other)
{
	return fAddrIt!=other.fAddrIt;
}

VNetAddressList::const_iterator& VNetAddressList::const_iterator::operator++()
{
	++fAddrIt;

	return *this;
}

VNetAddressList::const_iterator VNetAddressList::const_iterator::operator++(int)
{
	const_iterator tmp(*this);
	
	++fAddrIt;
	
	return tmp;
}

const VNetAddress& VNetAddressList::const_iterator::operator*()
{
	return *fAddrIt;
}

const VNetAddress* VNetAddressList::const_iterator::operator->()
{
	return &(*fAddrIt);
}

VNetAddressList::const_iterator::const_iterator(std::list<VNetAddress>::const_iterator inBeginIt) : fAddrIt(inBeginIt) {}

VNetAddressList::const_iterator VNetAddressList::begin()
{
	return const_iterator(fAddrList.begin());
}

const VNetAddressList::const_iterator VNetAddressList::end()
{
	return(const_iterator(fAddrList.end()));
}

void VNetAddressList::PushXNetAddr(const XNetAddr& inNetAddr)
{
	fAddrList.push_back(VNetAddress(inNetAddr));
}


END_TOOLBOX_NAMESPACE

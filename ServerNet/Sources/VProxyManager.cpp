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
#include "VProxyManager.h"
#include "HTTPTools.h"
#include "Tools.h"

USING_TOOLBOX_NAMESPACE
using namespace HTTPTools;


#if VERSIONMAC
#include <SystemConfiguration/SystemConfiguration.h>
#endif


//--------------------------------------------------------------------------------------------------

const sLONG	DEFAULT_HTTP_PORT	= 80;
const sLONG	DEFAULT_HTTPS_PORT	= 443;


#if VERSIONMAC

/*	code partially inspired from Technical Q&A QA1234 
	http://developer.apple.com/qa/qa2001/qa1234.html
*/

static
VError MAC_GetHTTPProxySettings (VString& outHttpProxy, PortNumber& outHttpPort,
								 VString& outHttpsProxy, PortNumber& outHttpsPort,
								 VectorOfVString *outExceptionsList)
{
	outHttpProxy=outHttpsProxy="";
	outHttpPort=outHttpsPort=kBAD_PORT;

	if (outExceptionsList!=NULL)
		outExceptionsList->clear();

	CFDictionaryRef		proxyDict;
	CFNumberRef			enableNum;
	int					enable;
	CFStringRef			hostStr;
	CFNumberRef			portNum;
	int					portInt;

	// Get the dictionary.

	proxyDict = SCDynamicStoreCopyProxies(NULL);

	if(proxyDict==NULL)
		return VE_SRVR_FAILED_GET_PROXY_SETTINGS;

	// Get the enable flag.	 This isn't a CFBoolean, but a CFNumber.
	enableNum = (CFNumberRef)CFDictionaryGetValue (proxyDict, kSCPropNetProxiesHTTPEnable);

	enable=0;

	if(enableNum!=NULL && CFGetTypeID(enableNum)==CFNumberGetTypeID())
		CFNumberGetValue(enableNum, kCFNumberIntType, &enable);

	// Get the proxy host.	DNS names must be in ASCII.	 If you 
	// put a non-ASCII character  in the "Web Proxy"
	// field in the Network preferences panel, the CFStringGetCString
	// function will fail and this function will return false.
	
	if(enable)
	{
		hostStr = (CFStringRef)CFDictionaryGetValue (proxyDict, kSCPropNetProxiesHTTPProxy);

		if(hostStr!=NULL && CFGetTypeID(hostStr)==CFStringGetTypeID())
			outHttpProxy.MAC_FromCFString(hostStr);
	}
	
	// Get the proxy port. OS X puts a default value if port is empty in network properties.

	if (enable)
	{
		portNum = (CFNumberRef) CFDictionaryGetValue(proxyDict, kSCPropNetProxiesHTTPPort);

		if(portNum!=NULL && CFGetTypeID(portNum) == CFNumberGetTypeID())
			if(CFNumberGetValue (portNum, kCFNumberIntType, &portInt))
				outHttpPort = (PortNumber)portInt;
	}

	// Same code, but for HTTPS

	enableNum = (CFNumberRef)CFDictionaryGetValue (proxyDict, kSCPropNetProxiesHTTPSEnable);

	enable=0;

	if(enableNum!=NULL && CFGetTypeID(enableNum)==CFNumberGetTypeID())
		CFNumberGetValue(enableNum, kCFNumberIntType, &enable);

	if(enable)
	{
		hostStr = (CFStringRef)CFDictionaryGetValue (proxyDict, kSCPropNetProxiesHTTPSProxy);

		if(hostStr!=NULL && CFGetTypeID(hostStr)==CFStringGetTypeID())
			outHttpsProxy.MAC_FromCFString(hostStr);
	}

	if (enable)
	{
		portNum = (CFNumberRef) CFDictionaryGetValue(proxyDict, kSCPropNetProxiesHTTPSPort);

		if(portNum!=NULL && CFGetTypeID(portNum) == CFNumberGetTypeID())
			if(CFNumberGetValue (portNum, kCFNumberIntType, &portInt))
				outHttpsPort = (PortNumber)portInt;
	}

	bool haveProxy=!outHttpProxy.IsEmpty() || !outHttpsProxy.IsEmpty();

    // Get the exception list

	if (outExceptionsList!=NULL && haveProxy)
	{
		CFArrayRef cfExceptionsList = (CFArrayRef)CFDictionaryGetValue (proxyDict, kSCPropNetProxiesExceptionsList);
		if (cfExceptionsList)
		{
			VString string;
			for (CFIndex idx = 0; idx < CFArrayGetCount (cfExceptionsList); idx++)
			{
				CFStringRef cfException = (CFStringRef)CFArrayGetValueAtIndex (cfExceptionsList, idx);

				if(cfException!=NULL && CFGetTypeID(cfException)==CFStringGetTypeID())
				{
					string.MAC_FromCFString (cfException);
					outExceptionsList->push_back (string);
				}
				else
				{
					string.Clear();
				}
			}
		}
	}

	// Clean up.
	if (proxyDict != NULL)
		CFRelease (proxyDict);

	return VE_OK;
}

#elif VERSIONWIN

static
VError WIN_GetHTTPProxySettings(VString& outHttpProxy, PortNumber& outHttpPort,
								VString& outHttpsProxy, PortNumber& outHttpsPort,
								VectorOfVString *outExceptionsList)
{
	outHttpProxy=outHttpsProxy="";
	outHttpPort=outHttpsPort=kBAD_PORT;

	if(outExceptionsList!=NULL)
		outExceptionsList->clear();

	LPCTSTR	ieSettingsPath="Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings";

	HKEY handle=NULL;
	LSTATUS err=0;

	err=RegOpenKeyEx(HKEY_CURRENT_USER, ieSettingsPath, (DWORD)0, (REGSAM)KEY_READ, (PHKEY)&handle);
	xbox_assert(err==0);

	if(err!=0)
		return VE_SRVR_FAILED_GET_PROXY_SETTINGS;

	//Check ProxyEnable key
	DWORD proxyEnable=0;
	DWORD size=sizeof(proxyEnable);
	err=RegQueryValueEx(handle, TEXT("ProxyEnable"), 0, NULL, (LPBYTE)&proxyEnable, &size);
	xbox_assert(err==0);

	if(err!=0)
		return VE_SRVR_FAILED_GET_PROXY_SETTINGS;

	//If proxy is not enabled, proxys, ports and exceptions do not make sense
	if(proxyEnable==0)
		return VE_OK;

	//Proxy is enabled ; Look for proxy values.
	char proxyServer[512];
	size=sizeof(proxyServer);
	err=RegQueryValueEx(handle, TEXT ("ProxyServer"), 0, NULL, (LPBYTE)proxyServer, &size);
	xbox_assert(err==0);

	//As it turn out, IE allows broken proxy setup where ProxyEnabled is true but ProxyServer does not exists
	//(ProxyServer is never empty ?). Consider it's an error.
	if(err!=0)
		return VE_SRVR_FAILED_GET_PROXY_SETTINGS;

	if(*proxyServer==0)
		return VE_OK;

	char* pos=proxyServer;
	char scheme[8];
	scheme[0]=0;
	char proxy[256];
	proxy[0]=0;
	int port=kBAD_PORT;
	int p1=0;
	int p2=0;
	int p3=0;
	int n=0;

	//First try, look for specific http/https proxies : "http=proxy.private.4d.fr:80;https=secproxy.private.4d.fr:443"
	//(port is optional)
	while((n=sscanf(pos, "%8[a-z]=%256[^=:;]%n:%d%n;%n", scheme, proxy, &p1, &port, &p2, &p3))>=2)
	{
		if(strcmp(scheme, "http")==0)
		{
			outHttpProxy=VString(proxy);
			outHttpPort=port;	//kBAD_PORT is ok
		} 
		else if(strcmp(scheme, "https")==0)
		{
			outHttpsProxy=VString(proxy);
			outHttpsPort=port;	//kBAD_PORT is ok
		}

		pos+=(p3>0 ? p3 : (p2>0 ? p2 : p1));

		//If port is lacking, we may have to skip ';' manually 	
		if(*pos==';')
			pos++;

		scheme[0]=0;
		proxy[0]=0;
		port=kBAD_PORT;
		p1=p2=p3=0;
	}

	//Second try, look for generic proxy (same for all protocols) : "proxy.private.4d.fr:80"
	//(Not sure if port is mandatory)
	if(outHttpProxy.IsEmpty() && outHttpsProxy.IsEmpty())
	{
		n=sscanf(pos, "%256[^=:;]%n:%d%n", proxy, &p1, &port, &p2);

		if(n>=1)
		{
			outHttpProxy=VString(proxy);
			outHttpPort=port;	//kBAD_PORT is ok

			outHttpsProxy=VString(proxy);
			outHttpsPort=port;	//kBAD_PORT is ok
		}
	}

	if(outHttpProxy.IsEmpty() && outHttpsProxy.IsEmpty())
	{
		//ProxyEnable is set, but no HTTP proxy was found.	
		//It's weird (worth a break in Debug !), but it's OK...
		//May be there is a FTP/SOCKS proxy !
		//Anyway, without proxy, exceptions do not make sense.
		xbox_assert(false);

		return VE_OK;
	}

	//We have some proxy, look at exception list...

	if(outExceptionsList==NULL)
		return VE_OK;

	if(outExceptionsList!=NULL)
	{	
		char list[1024];
		list[0]=0;
		size=sizeof(list);

		//Read ProxyOverride string : "192.168.*;10.*;127.0.0.1;localhost;*.4d.fr;kb.4d.com;www.4d.com;<local>"
		err=RegQueryValueEx(handle, TEXT("ProxyOverride"), 0, NULL, (LPBYTE)list, &size);

		if(err==0)
		{
			VString tmp((char*)list, size, VTC_StdLib_char);
			tmp.GetSubStrings(CHAR_SEMICOLON, *outExceptionsList, false);
		}
	}

	return VE_OK;
}

#elif VERSION_LINUX

static
VError LINUX_GetHTTPProxySettings(VString& outHttpProxy, PortNumber& outHttpPort,
								  VString& outHttpsProxy, PortNumber& outHttpsPort,
								  VectorOfVString *outExceptionsList)
{
	outHttpProxy=outHttpsProxy="";
	outHttpPort=outHttpsPort=kBAD_PORT;

	//Exceptions are not implemented on Linux
	if (outExceptionsList!=NULL)
		outExceptionsList->clear();

	const char* proxy=getenv("http_proxy");
	
	if(proxy==NULL)
		proxy=getenv("HTTP_PROXY");

	if(proxy!=NULL)
	{
		VURL url(VString(proxy), false /*not encoded*/);

		url.GetHostName(outHttpProxy, false /*not encoded*/);

		VString port;
		url.GetPortNumber(port, false /*not encoded*/);

		if(!port.IsEmpty())
			outHttpPort=port.GetLong();
	}

	proxy=getenv("https_proxy");
	
	if(proxy==NULL)
		proxy=getenv("HTTPS_PROXY");

	if(proxy!=NULL)
	{
		VURL url(VString(proxy), false /*not encoded*/);

		url.GetHostName(outHttpsProxy, false /*not encoded*/);

		VString port;
		url.GetPortNumber(port, false /*not encoded*/);

		if(!port.IsEmpty())
			outHttpsPort=port.GetLong();
	}


	return VE_OK;
}


#endif // #if VERSIONMAC


template <typename T>
bool StrMatch (T* s, T* mask)
{
	T *cp = NULL;
	T *mp = NULL;

	for (; (*s) && (*mask != (T)'*'); mask++, s++)
		if ((*mask != *s) && (*mask != (T)'?'))
			return false;
	for (;;)
	{
		if (!*s)
		{
			while (*mask == (T)'*')
				mask++;
			return !*mask;
		}
		if (*mask == (T)'*')
		{
			if (!*++mask)
				return true;
			mp = mask;
			cp = s + 1;
			continue;
		}
		if ((*mask == *s) || (*mask == (T)'?'))
		{
			mask++;
			s++;
			continue;
		}
		mask = mp;
		s = cp++;
	}
}


//--------------------------------------------------------------------------------------------------


bool			VProxyManager::fUseHttpProxy  = false;
bool			VProxyManager::fUseHttpsProxy  = false;
VString			VProxyManager::fHttpProxyServerAddress;
PortNumber		VProxyManager::fHttpProxyServerPort = DEFAULT_HTTP_PORT;
VString			VProxyManager::fHttpsProxyServerAddress;
PortNumber		VProxyManager::fHttpsProxyServerPort = DEFAULT_HTTPS_PORT;
VectorOfVString	VProxyManager::fProxyExceptionsList;


/* static */
VError VProxyManager::GetInfos (VString& outHttpProxyName, PortNumber& outHttpProxyPort,
							  VString& outHttpsProxyName, PortNumber& outHttpsProxyPort,
							  VectorOfVString *outExceptionsList)
{
	VError verr=VE_OK;

#if VERSIONMAC
	verr=MAC_GetHTTPProxySettings(outHttpProxyName, outHttpProxyPort, outHttpsProxyName, outHttpsProxyPort, outExceptionsList);
#elif VERSIONWIN
	verr=WIN_GetHTTPProxySettings(outHttpProxyName, outHttpProxyPort, outHttpsProxyName, outHttpsProxyPort, outExceptionsList);
#elif VERSION_LINUX
	verr=LINUX_GetHTTPProxySettings(outHttpProxyName, outHttpProxyPort, outHttpsProxyName, outHttpsProxyPort, outExceptionsList);
#endif

	if(verr==VE_OK)
	{
		if(!outHttpProxyName.IsEmpty() && outHttpProxyPort==kBAD_PORT)
			outHttpProxyPort=DEFAULT_HTTP_PORT;

		if(!outHttpsProxyName.IsEmpty() && outHttpsProxyPort==kBAD_PORT)
			outHttpsProxyPort=DEFAULT_HTTPS_PORT;
	}

	return verr;
}


/* static */
VProxyInfos *VProxyManager::CreateProxyInfos()
{
	VString	host;
	PortNumber port;

	VString	tmpHttpsHost;
	PortNumber tmpHttpsPort;

	GetInfos (host, port, tmpHttpsHost, tmpHttpsPort, NULL);

	return new VProxyInfos (host, port);
}


/* static */
void VProxyManager::Init()
{
	
	VError verr=GetInfos(fHttpProxyServerAddress, fHttpProxyServerPort,
						 fHttpsProxyServerAddress, fHttpsProxyServerPort,
						 &fProxyExceptionsList);

	xbox_assert(verr==VE_OK);

	fUseHttpProxy=!fHttpProxyServerAddress.IsEmpty();
	fUseHttpsProxy=!fHttpsProxyServerAddress.IsEmpty();

	
#if VERSIONDEBUG
	if(fUseHttpProxy || fUseHttpsProxy)
	{
		DebugMsg ("***** Proxy settings *****\n");

		if(fUseHttpProxy)
			DebugMsg ("\tHTTP Proxy is %S:%d\n", &fHttpProxyServerAddress, fHttpProxyServerPort);

		if(fUseHttpsProxy)
			DebugMsg ("\tHTTPS Proxy is %S:%d\n", &fHttpsProxyServerAddress, fHttpsProxyServerPort);

		sLONG count=fProxyExceptionsList.size();

		if(count>0)
		{
			DebugMsg ("\tProxy has %d Exception(s):\n", count);

			VectorOfVString::const_iterator cit;

			for (cit=fProxyExceptionsList.begin() ; cit!=fProxyExceptionsList.end() ; ++cit)
			{
				DebugMsg ("\t\t- %S\n", &(*cit));
			}
		}
	}
	else
	{
		DebugMsg ("***** NO PROXY ! *****\n");
	}
#endif
}


/* static */
void VProxyManager::DeInit()
{
	fProxyExceptionsList.erase (fProxyExceptionsList.begin(), fProxyExceptionsList.end());
}


/* static */
bool VProxyManager::MatchProxyException (const VString& inDomain)
{
	if (!fProxyExceptionsList.size())
		return false;

	for (VectorOfVString::const_iterator it = fProxyExceptionsList.begin(); it != fProxyExceptionsList.end(); ++it)
	{
		if (StrMatch (inDomain.GetCPointer(), (*it).GetCPointer()))
			return true;
	}

	return false;
}


/* static */
bool VProxyManager::ByPassProxyOnLocalhost (const VString& inDomain)
{
	return !inDomain.IsEmpty() && (EqualASCIIVString (inDomain, "localhost") 
			                       || ServerNetTools::IsLocalInterface(inDomain)
								   || ServerNetTools::IsLoopBack(inDomain));
}


VError VProxyManager::GetProxyForURL(const VString& inURL, VString* outProxyAddr, PortNumber *outProxyPort)
{
	VURL url(inURL, false /*do not touch, pretend unencoded*/);

	return GetProxyForURL(url, outProxyAddr, outProxyPort);

}


VError VProxyManager::GetProxyForURL(const VURL& inURL, VString* outProxyAddr, PortNumber *outProxyPort)
{
	if(outProxyAddr==NULL || outProxyPort==NULL)
		return VE_INVALID_PARAMETER;
	
	VString scheme;
	inURL.GetScheme(scheme);

	bool needHttpProxy=fUseHttpProxy && scheme.CompareToString("HTTP", false /*not case sensitive*/)==CR_EQUAL;
	bool needHttpsProxy=fUseHttpsProxy && scheme.CompareToString("HTTPS", false /*not case sensitive*/)==CR_EQUAL;

	if(needHttpProxy || needHttpsProxy)
	{
		VString host;
		inURL.GetHostName(host, false /*want unencoded*/);

		if(MatchProxyException(host))
			*outProxyAddr="", *outProxyPort=kBAD_PORT;
		else if(needHttpProxy)
			*outProxyAddr=fHttpProxyServerAddress, *outProxyPort=fHttpProxyServerPort;
		else if(needHttpsProxy)
			*outProxyAddr=fHttpsProxyServerAddress, *outProxyPort=fHttpsProxyServerPort;

#if VERSIONDEBUG
		if(!outProxyAddr->IsEmpty() && ByPassProxyOnLocalhost(host))
			DebugMsg ("Warning - Using proxy %S:%d on %S\n", outProxyAddr, *outProxyPort, &host);
#endif
	}

	return VE_OK;
}

//--------------------------------------------------------------------------------------------------


VString VProxyInfos::GetHttpProxyHost() const
{
	return fHost;
}


PortNumber VProxyInfos::GetHttpProxyPort() const
{
	return fPort;
}


void VProxyInfos::Release()
{
	delete this;
}



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

#ifndef __PROXY_MANAGER_INCLUDED__
#define __PROXY_MANAGER_INCLUDED__

#include "ServerNet/Sources/ServerNetArchTypes.h"


BEGIN_TOOLBOX_NAMESPACE


class XTOOLBOX_API VProxyInfos : public XBOX::VObject
{
public:
												VProxyInfos (const XBOX::VString& inHost, PortNumber inPort) : fHost (inHost), fPort (inPort)	{}

	virtual	XBOX::VString						GetHttpProxyHost() const;
	virtual	sLONG								GetHttpProxyPort() const;

	virtual	void								Release();

private:
	XBOX::VString								fHost;
	PortNumber									fPort;
};


class XTOOLBOX_API VProxyManager : public XBOX::VObject
{
public:
	static VError								GetInfos(VString& outHttpProxyName, PortNumber& outHttpProxyPort,
														 VString& outHttpsProxyName, PortNumber& outHttpsProxyPort,
														 VectorOfVString *outExceptionsList=NULL);

	static VProxyInfos *						CreateProxyInfos();

	static void									Init();
	static void									DeInit();

	static bool									MatchProxyException (const XBOX::VString& inDomain);
	static bool									ByPassProxyOnLocalhost (const XBOX::VString& inDomain);

	static bool									GetUseProxy() { return fUseHttpProxy|fUseHttpsProxy; }

	static const XBOX::VString&					GetProxyServerAddress() { return fHttpProxyServerAddress; }
	static sLONG								GetProxyServerPort() { return fHttpProxyServerPort; }

	static const XBOX::VectorOfVString&			GetProxyExceptionsList() { return fProxyExceptionsList; }

	static VError								GetProxyForURL(const VString& inURL, VString* outProxyAddr, PortNumber *outProxyPort);
	static VError								GetProxyForURL(const VURL& inURL, VString* outProxyAddr, PortNumber *outProxyPort);


private:
	static bool									fUseHttpProxy;
	static bool									fUseHttpsProxy;

	static XBOX::VString						fHttpProxyServerAddress;
	static sLONG								fHttpProxyServerPort;


	static XBOX::VString						fHttpsProxyServerAddress;
	static sLONG								fHttpsProxyServerPort;

	static XBOX::VectorOfVString				fProxyExceptionsList;
};


END_TOOLBOX_NAMESPACE

#endif // __PROXY_MANAGER_INCLUDED__

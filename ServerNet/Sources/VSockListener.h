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
#include "ServerNetTypes.h"

#if VERSIONWIN
	#include "XWinSocket.h"
#else
	#include "XBsdSocket.h"
#endif


#ifndef __SNET_SOCK_LISTENER__
#define __SNET_SOCK_LISTENER__


BEGIN_TOOLBOX_NAMESPACE

class XSBind;

class XTOOLBOX_API VSockListener : public VObject
{
public:
	VSockListener(IRequestLogger* inRequestLogger = 0);
	virtual ~VSockListener();
	
	bool AddListeningPort(VString  iAddr, PortNumber iPort, bool iSsl=false, Socket inBoundSock=kBAD_SOCKET, bool inReuseAddress=true);
	bool AddListeningPort(const VNetAddress& inAddr, bool iSsl=false, Socket inBoundSock=kBAD_SOCKET, bool inReuseAddress=true);
	
	bool StartListening();
	void StopListeningAndClearPorts();
	
	void SetCertificatePaths (const VFilePath& inCertPath, const VFilePath& inKeyPath);

	VError SetKeyAndCertificate(const VMemoryBuffer<>& inKey, const VMemoryBuffer<>& inCertificate);
	VError SetKeyAndCertificate (const XBOX::VString &inKey, const XBOX::VString &inCertificate);

	VError SetCertificateFolder(const VFilePath& inCertFolderPath, 
								const VString& inKey=VString("key.pem"), const VString& inCert=VString("cert.pem"));
	
	VError PushIntermediateCertificate(const VMemoryBuffer<>& inCertificate);
	VError PushIntermediateCertificate(const XBOX::VString &inCertificate);
	
	void setAcceptTimeout(uLONG inMsTimeout);
	bool SetBlocking (bool isBlocking = false);
	
	XTCPSock* GetNewConnectedSocket(sLONG inMsTimeout);
	
	void ReleaseConnection(XTCPSock* in);
	
private:
	
	VError WaitForAccept(sLONG inMsTimeout);
	
	IRequestLogger* fRequestLogger;
	
	typedef std::vector<XSBind*>::const_iterator XSBindCIt;
	
	std::vector<XSBind*> fPlainListens;
	std::vector<XSBind*> fSslListens;
	XTCPAcceptIterator fAcceptIterator;
	bool fListenStarted;
	VKeyCertChain* fKeyCertChain;
};


END_TOOLBOX_NAMESPACE


#endif

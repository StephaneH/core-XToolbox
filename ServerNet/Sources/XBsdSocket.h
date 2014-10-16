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
#ifndef __SNET_BSD_SOCKET__
#define __SNET_BSD_SOCKET__


#include <sys/socket.h>
#include <netdb.h>

#include "ServerNetTypes.h"


BEGIN_TOOLBOX_NAMESPACE


class VKeyCertChain;
class VNetAddress;
class VSslDelegate;


class XTOOLBOX_API XBsdTCPSocket : public VObject
{
 public:
	
	static XBsdTCPSocket* NewClientConnectedSock(const VString& inDnsName, PortNumber inPort, sLONG inMsTimeout);	//Client specific !
	//jmo - TODO : Mettre une VString pour l'adresse.
	static XBsdTCPSocket* NewServerListeningSock(const VNetAddress& inAddr, Socket inBoundSock=kBAD_SOCKET, bool inReuseAddress=true);	//Server specific !
	
	static XBsdTCPSocket* NewServerListeningSock(PortNumber inPorts, Socket inBoundSock=kBAD_SOCKET, bool inReuseAddress=true);	//Server specific !

	virtual ~XBsdTCPSocket();
	
	//DEPRECATED COMPATIBILITY SHORTCUTS
	VString GetIP() const;
	PortNumber GetPort() const;
	PortNumber GetServicePort() const;


	Socket GetRawSocket() const;
	VError Close(bool inWithRecvLoop=false);
	
	VError SetBlocking(bool inBlocking=true);
	bool IsBlocking();


	XBsdTCPSocket* Accept(uLONG inMsTimeout);
	
	VError Read(void* outBuff, uLONG* ioLen);
	VError Write(const void* inBuff, uLONG* ioLen, bool /*inWithEmptyTail*/);

	VError ReadWithTimeout(void* outBuff, uLONG* ioLen, sLONG inMsTimeout, sLONG* outMsSpent=NULL);
	VError WriteWithTimeout(const void* inBuff, uLONG* ioLen, sLONG inMsTimeout, sLONG* outMsSpent=NULL, bool unusedWithEmptyTail=false);

	XBOX::VError SetNoDelay (bool inYesNo);
	
	VError PromoteToSSL(VKeyCertChain* inKeyCertChain=NULL);
	bool IsSSL();

	// Used by SSJS socket implementation only (for doing handshake).
	VSslDelegate	*GetSSLDelegate ()	{ return fSslDelegate; }
	
	
 private :
	
	XBsdTCPSocket(Socket inSock) :
		fSock(inSock), fServicePort(kBAD_PORT), fProfile(NewSock), fSslDelegate(NULL) {}
	
	VError DoRead(void* outBuff, uLONG* ioLen);
	VError DoReadWithTimeout(void* outBuff, uLONG* ioLen, sLONG inMsTimeout, sLONG* outMsSpent=NULL);

	VError DoWrite(const void* inBuff, uLONG* ioLen);
	VError DoWriteWithTimeout(const void* inBuff, uLONG* ioLen, sLONG inMsTimeout, sLONG* outMsSpent=NULL);
	
	//Reads and discard data on Close with receive loop. Helps prevent TCP RST flag.
	static void TrashWithTimeout(Socket inFd, sLONG inMsTimeout, sLONG* outMsSpent=NULL);
	
	//It doesn't matter if we're building a client or server socket, we pass the SERVER address !
	//XBsdTCPSocket(sLONG inSockFD, const sockaddr* inServerAddr=NULL, socklen_t inAddrLen=0);
	
	int GetAddrFamilySize();
	PortNumber GetSockAddrPort() const;

	VError Connect(const VNetAddress& inAddr, sLONG inMsTimeout);			//Client specific !
	VError Listen(const VNetAddress& inAddr, bool inAlreadyBound=false, bool inReuseAddress=true);	//Server specific !

	VError SetServicePort(PortNumber inServicePort);
	
	//WaitFor for is a select wrapper used for timeout.
	typedef enum {kREAD_SET, kWRITE_SET, kERROR_SET} FdSet;
	VError WaitFor(Socket inFd, FdSet inSet, sLONG inMsTimeout, sLONG* outMsSpent=NULL);
	VError WaitForConnect(sLONG inMsTimeout, sLONG* outMsSpent=NULL);
	VError WaitForAccept(sLONG inMsTimeout, sLONG* outMsSpent=NULL);
	VError WaitForRead(sLONG inMsTimeout, sLONG* outMsSpent=NULL);
	VError WaitForWrite(sLONG inMsTimeout, sLONG* outMsSpent=NULL);	

	
	Socket	fSock;
	
	PortNumber	fServicePort; //a virer ?

	typedef enum {NewSock=0, ServiceSock, ConnectedSock, ClientSock} SockProfile; 

	SockProfile fProfile;
	
	VSslDelegate*	fSslDelegate;
};


class XBsdAcceptIterator : public VObject
{
	//Pas thread-safe ! Mais en principe pas necessaire vu que tous les appels decoulent de VTCPConnectionListener::StartListening()

public :
	
	XBsdAcceptIterator();
	~XBsdAcceptIterator();
	
	VError AddServiceSocket(XBsdTCPSocket* inSock);
	VError ClearServiceSockets();
	VError GetNewConnectedSocket(XBsdTCPSocket** outSock, sLONG inMsTimeout);
	
private :

	XBsdAcceptIterator(const XBsdAcceptIterator&);	//Copy doesn't make sense !

	//Needs special error handling, done in the corresponding public method
	VError GetNewConnectedSocket(XBsdTCPSocket** outSock, sLONG inMsTimeout, bool* outShouldRetry);
	
	typedef std::vector<XBsdTCPSocket*> SockPtrColl;
	SockPtrColl fSocks;
	
	SockPtrColl::const_iterator fSockIt;

	//Dynamic alloc to make sure we use ServerNet FD_SETSIZE
	fd_set* fReadSet;
	
};


class XTOOLBOX_API XBsdUDPSocket : public VObject
{
public :

	static XBsdUDPSocket* NewMulticastSock(const VString& inMultiCastIP, PortNumber inPort);

	virtual ~XBsdUDPSocket();
	
	VError SetBlocking(uBOOL inBlocking);
	
	Socket GetRawSocket();

	VError Close();
		
	VError Read(void* outBuff, uLONG* ioLen, VNetAddress* outSenderInfo=NULL);
	
	VError Write(const void *inBuffer, uLONG inLength, const VNetAddress& inReceiverInfo);
		
		
private :
	
	XBsdUDPSocket(Socket inSock) : fSock(inSock) {}
	
	sLONG	fSock;
};


END_TOOLBOX_NAMESPACE


#endif
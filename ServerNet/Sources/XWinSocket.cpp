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

#include "XWinSocket.h"

#include "Tools.h"
#include "VNetAddr.h"
#include "VSslDelegate.h"

#include <Ws2tcpip.h>


BEGIN_TOOLBOX_NAMESPACE


using namespace ServerNetTools;


#define WITH_SNET_SOCKET_LOG 0


//This global object ensure winsock is properlely loaded, initialized and unloaded...
//http://msdn.microsoft.com/en-us/library/ms742213%28v=VS.85%29.aspx

class WinSockInit
{
public :

	WinSockInit()
	{
		fOk=false;

	    WORD wVersionRequested;
		WSADATA wsaData;

	    wVersionRequested=MAKEWORD(2, 2);

		//The WSAStartup function initiates use of the Winsock DLL by a process.
		int err=WSAStartup(wVersionRequested, &wsaData);
		xbox_assert(err==0);

		if(LOBYTE(wsaData.wVersion)==2 && HIBYTE(wsaData.wVersion)==2)
			fOk=true;

		xbox_assert(fOk);
	}

	~WinSockInit()
	{
		//The WSACleanup function terminates use of the Winsock 2 DLL (Ws2_32.dll).
		int err=WSACleanup();
		xbox_assert(err==0);
	}

	bool IsOk()
	{
		return fOk;
	}

private :

	bool fOk;

} gWinSockInit;



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// XWinTCPSocket
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//virtual
XWinTCPSocket::~XWinTCPSocket()
{
	//Close();

#if VERSIONDEBUG
	if(fSock!=kBAD_SOCKET)
		DebugMsg ("[%d] XWinTCPSocket::~XWinTCPSocket() : socket %d not closed yet !\n", VTask::GetCurrentID(), fSock);
#endif
	
	if(fSslDelegate!=NULL)
		delete fSslDelegate;
}


VString XWinTCPSocket::GetIP() const
{
	XWinNetAddr addr;
	
	if(fProfile==ClientSock)
		addr.FromPeerAddr(fSock);
	else
		addr.FromLocalAddr(fSock);
	
	return addr.GetIP();
}


PortNumber XWinTCPSocket::GetPort() const
{
	return GetServicePort();
}


PortNumber XWinTCPSocket::GetServicePort() const
{
	return fServicePort;
}


VError XWinTCPSocket::SetServicePort(PortNumber inServicePort)
{
	fServicePort=inServicePort;

	return VE_OK;
}


Socket XWinTCPSocket::GetRawSocket() const
{
	return fSock;
}


VError XWinTCPSocket::Close(bool inWithRecvLoop)
{
	StSilentErrorContext errCtx;

	Socket invalidSocket=kBAD_SOCKET;

#if ARCH_32
	Socket realSocket=(Socket)XBOX::VInterlocked::Exchange(reinterpret_cast<sLONG*>(&fSock), static_cast<sLONG>(invalidSocket));
#else
	Socket realSocket=(Socket)XBOX::VInterlocked::Exchange(reinterpret_cast<sLONG8*>(&fSock), static_cast<sLONG8>(invalidSocket));
#endif

	if(fSslDelegate!=NULL)
	{
		//Make sure the socket is blocking, SSL shutdown will be easier
		u_long nonBlocking=false;

		int err=ioctlsocket(realSocket, FIONBIO, &nonBlocking);
		xbox_assert(err!=SOCKET_ERROR);
		
		fSslDelegate->Shutdown();
	}

	int err=shutdown(realSocket, SD_SEND);
	
	if(err!=SOCKET_ERROR && inWithRecvLoop)
	{
		StDropErrorContext errCtx;
		
		char garbage[512];
		uLONG len=sizeof(garbage);

		TrashWithTimeout(realSocket, 3000 /*3s timeout*/); 
	}

	if(realSocket!=kBAD_SOCKET)
	{
		err=closesocket(realSocket);
	}

	return err==0 ? VE_OK : vThrowNativeError(WSAGetLastError());
}


VError XWinTCPSocket::SetBlocking(bool inBlocking)
{
	u_long nonBlocking=!inBlocking;

	int err=ioctlsocket(fSock, FIONBIO, &nonBlocking);
		
	if(err==SOCKET_ERROR)
		return vThrowNativeError(WSAGetLastError());

	fIsBlocking=inBlocking;

	return VE_OK;
}


bool XWinTCPSocket::IsBlocking()
{
	return fIsBlocking;
}


VError XWinTCPSocket::WaitFor(Socket inFd, FdSet inSet, sLONG inMsTimeout, sLONG* outMsSpent)
{
	if(inMsTimeout<0)
		return VE_SOCK_TIMED_OUT;
		
	uLONG8 start=VSystem::GetCurrentTime();

	uLONG8 stop=start+inMsTimeout;

	uLONG8 now=start;
	
	for(;;)
	{
		uLONG8 msTimeout=stop-now;

		timeval timeout;
		memset(&timeout, 0, sizeof(timeout));
		
		if(msTimeout>kAUTO_YIELD_TIMEOUT)
			timeout.tv_usec=kAUTO_YIELD_TIMEOUT*1000;
		else
			timeout.tv_usec=msTimeout*1000;
		
        fd_set set;
        FD_ZERO(&set);
        FD_SET(inFd, &set);

		fd_set* rs=(inSet==kREAD_SET) ? &set : NULL;
		fd_set* ws=(inSet==kWRITE_SET) ? &set : NULL;
		fd_set* es=(inSet==kERROR_SET) ? &set : NULL;

        int res=select(inFd+1, rs, ws, es, &timeout);
		
		now=VSystem::GetCurrentTime();
		
		if(outMsSpent!=NULL)
			*outMsSpent=now-start;
		
		if(res==0)
		{
			VTask::Yield();
			
			if(now<stop)
				continue;

			return VE_SOCK_TIMED_OUT;
		}
		
		if(res==1)
			return VE_OK;
		
		return vThrowNativeError(WSAGetLastError());
	}
}


VError XWinTCPSocket::WaitForConnect(sLONG inMsTimeout, sLONG* outMsSpent)
{
	//connect : When the connection has been established asynchronously, select() shall indicate that 
	//          the file descriptor for the socket is ready for writing.
	
	return WaitFor(GetRawSocket(), kWRITE_SET, inMsTimeout, outMsSpent);
}


VError XWinTCPSocket::WaitForAccept(sLONG inMsTimeout, sLONG* outMsSpent)
{
	//accept : When a connection is available, select() indicates that
	//         the file descriptor for the socket is ready for reading.
	return WaitFor(GetRawSocket(), kREAD_SET, inMsTimeout, outMsSpent);
}


VError XWinTCPSocket::WaitForRead(sLONG inMsTimeout, sLONG* outMsSpent)
{
	return WaitFor(GetRawSocket(), kREAD_SET, inMsTimeout, outMsSpent);
}


VError XWinTCPSocket::WaitForWrite(sLONG inMsTimeout, sLONG* outMsSpent)
{
	return WaitFor(GetRawSocket(), kWRITE_SET, inMsTimeout, outMsSpent);
}


VError XWinTCPSocket::Connect(const VNetAddress& inAddr, sLONG inMsTimeout)
{
	xbox_assert(fProfile==NewSock);
	
	VError verr=SetBlocking(false);
	
	if(verr!=VE_OK)
		return verr;
	
	int err=0;

	err=connect(fSock, inAddr.GetAddr(), inAddr.GetAddrLen());
	
	//OUR SOCKET SEEMS TO HAVE WINSOCK 1.x BEHAVIOR : AS I UNDERSTAND THE DOC, WE SHOULDN'T HAVE TO TEST FOR
	//WSAEINVAL NOR WSAEWOULDBLOCK WITH WINSOCK 2.x
	//http://msdn.microsoft.com/en-us/library/windows/desktop/ms737625(v=vs.85).aspx

	if(err!=0 && WSAGetLastError()!=WSAEINPROGRESS && WSAGetLastError()!=WSAEALREADY && WSAGetLastError()!=WSAEWOULDBLOCK)
		return vThrowNativeError(WSAGetLastError());

	//If for any reason the socket is already connected, we take a shortcut to save a select syscall.
	if(err==0)
	{
		verr=SetBlocking(true);
		return verr;
	}

	verr=WaitForConnect(inMsTimeout);

	if(verr!=VE_OK)
		return verr;
	
	//Wait returns, but it doesn't mean the connection succed ! Retry connect to retrieve the status
	//(This test is mandatory on Mac... Although windows impl worked without it, it was conceptually broken)
	err=connect(fSock, inAddr.GetAddr(), inAddr.GetAddrLen());

	if(err!=0 && WSAGetLastError()!=WSAEISCONN)
		return vThrowNativeError(WSAGetLastError());
	
	fProfile=ClientSock;
	
	verr=SetBlocking(true);

	return verr;
}


VError XWinTCPSocket::Listen(const VNetAddress& inAddr, bool inAlreadyBound, bool inReuseAddress)
{
	xbox_assert(fProfile==NewSock);

	int err=0;

	bool isV6=false;
	
	if(!inAlreadyBound && inAddr.IsV6())
	{
		int opt=!WithV4MappedV6();
		
		err=setsockopt(fSock, IPPROTO_IPV6, IPV6_V6ONLY,  reinterpret_cast<const char*>(&opt), sizeof(opt));
		
		if(err!=0)
			return vThrowNativeError(WSAGetLastError());
	}

	if(!inAlreadyBound)
	{
		if (inReuseAddress)
		{
			int opt=true;
			err=setsockopt(fSock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&opt), sizeof(opt));

			if(err!=0)
				return vThrowNativeError(WSAGetLastError());
		}
		
		err=bind(fSock, inAddr.GetAddr(), inAddr.GetAddrLen());

		if(err!=0)
			return vThrowNativeError(WSAGetLastError());
	}
		
	err=listen(fSock, SOMAXCONN);

	if(err==0)
	{
		fProfile=ServiceSock;
		
		return VE_OK;
	}
	
	return vThrowNativeError(WSAGetLastError());
}


XWinTCPSocket* XWinTCPSocket::Accept(uLONG inMsTimeout)
{
	xbox_assert(fProfile==ServiceSock);

	VError verr=VE_OK;

	if(inMsTimeout>0)
	{
		verr=WaitForAccept(inMsTimeout);
		
		if(verr!=VE_OK)
			return NULL;
	}
	
	verr=SetBlocking(false);

	if(verr!=VE_OK)
		return NULL;

	sockaddr_storage sa_storage;
	int len=sizeof(sa_storage);
	memset(&sa_storage, 0, len);
	
	sockaddr* sa=reinterpret_cast<sockaddr*>(&sa_storage);
	
	Socket sock=kBAD_SOCKET;
	
	sock=accept(GetRawSocket(), sa, &len);
	
	if(sock==kBAD_SOCKET)
	{
		vThrowNativeError(WSAGetLastError());
		return NULL;
	}
		
	bool ok=true;

	if(ok && sa->sa_family!=AF_INET && sa->sa_family!=AF_INET6)
		ok=false;
	
	int opt=true;
	int err=0;

	if(ok)
	{
		err=setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&opt), sizeof(opt));
		
		if(err!=0)
			{ ok=false ; vThrowNativeError(WSAGetLastError()); }
	}

	if(ok)
	{
		err=setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char*>(&opt), sizeof(opt));

		if(err!=0)
			{ ok=false ; vThrowNativeError(WSAGetLastError()); }
	}

	XWinTCPSocket* xsock=NULL;
	
	if(ok)
	{
		xsock=new XWinTCPSocket(sock);

		if(xsock==NULL)
			{ ok=false ; vThrowError(VE_MEMORY_FULL); }
	}

	if(ok)
		xsock->fProfile=ConnectedSock;
		
	if(ok)
	{
		verr=xsock->SetBlocking(true);
		
		if(verr!=VE_OK)
			ok=false;
	}

	if(ok)
	{
		verr=xsock->SetServicePort(GetPort());

		if(verr!=VE_OK)
			ok=false;
	}

	if(!ok)
	{
		if(xsock!=NULL)
		{
			xsock->Close(false);
		
			delete xsock;
		
			xsock=NULL;	
		}	
		else
		{
			err=closesocket(sock);
		}
	}	
	
	return xsock;
}


VError XWinTCPSocket::Read(void* outBuff, uLONG* ioLen)
{
	if(outBuff==NULL || ioLen==NULL)
		return vThrowError(VE_INVALID_PARAMETER);
	
	if(fSslDelegate!=NULL)
		return fSslDelegate->Read(outBuff, ioLen);

	int n=recv(fSock, reinterpret_cast<char*>(outBuff), *ioLen, 0 /*flags*/);

	if(n>0)
	{
		*ioLen=static_cast<uLONG>(n);

		return VE_OK;
	}
	
	//We have an error...
	*ioLen=0;

	if(n==0)
	{
		//If no messages are available to be received and the peer has performed an orderly shutdown,
		//recv() shall return 0.
		
		return VE_SOCK_PEER_OVER;
	}

	int	lastError	= WSAGetLastError();
	
	if(lastError==WSAEWOULDBLOCK)
		return VE_SOCK_WOULD_BLOCK;
	
	if(lastError==WSAECONNRESET || lastError==WSAENOTSOCK || lastError==WSAEBADF)
		return vThrowNativeCombo(VE_SOCK_CONNECTION_BROKEN, lastError);

	return vThrowNativeCombo(VE_SOCK_READ_FAILED, lastError);
}


VError XWinTCPSocket::Write(const void* inBuff, uLONG* ioLen, bool inWithEmptyTail)
{
	if(inBuff==NULL || ioLen==NULL)
		return vThrowError(VE_INVALID_PARAMETER);
	
	if(fSslDelegate!=NULL)
		return fSslDelegate->Write(inBuff, ioLen);

	int n=send(fSock, reinterpret_cast<const char*>(inBuff), *ioLen, 0 /*flags*/);

	if(n>=0)
	{
		*ioLen=n;
	
		//bWithEmptyTail is used to determine broken socket connection (should be done in the networking code itself)
		if(inWithEmptyTail)
		{
			char emptyString[]={0};
			uLONG len=0;
			
			return Write(emptyString, &len, false);
		}

		return VE_OK;
	}
	
	//We have an error...
	*ioLen=0;
		
	int	lastError	= WSAGetLastError();

	if(lastError==WSAEWOULDBLOCK)
		return VE_SOCK_WOULD_BLOCK;
	
	if(lastError==WSAECONNRESET || lastError==WSAENOTSOCK || lastError==WSAEBADF)
		return vThrowNativeCombo(VE_SOCK_CONNECTION_BROKEN, lastError);
	
	return vThrowNativeCombo(VE_SOCK_WRITE_FAILED, lastError);
}


VError XWinTCPSocket::ReadWithTimeout(void* outBuff, uLONG* ioLen, sLONG inMsTimeout, sLONG* outMsSpent)
{
	if(outBuff==NULL || ioLen==NULL)
		return vThrowError(VE_INVALID_PARAMETER);
	
	
	VError verr=VE_OK;

	uLONG len=*ioLen;
	
	if(fSslDelegate!=NULL)
	{		
		sLONG spentTotal=0;
		
		sLONG available=fSslDelegate->GetBufferedDataLen();
		
		if(available>=len)
		{
			//First case : OpenSSL has enough buffered data ; No socket IO should occur.
			
			verr=fSslDelegate->Read(outBuff, &len);
			
			spentTotal=0;
		}
		else
		{	
			//Second case : We have to handle SSL protocol IO that might occur before having applicative data
		
			bool wasBlocking=false;
		
			if(IsBlocking())
			{
				SetBlocking(false);
				wasBlocking=true;
			}
				
			sLONG timeout=inMsTimeout;
		
			do
			{
				len=*ioLen;
				
				verr=fSslDelegate->Read(outBuff, &len);

				if(len>0)	//We have some data ; that's enough for now.
					break;
		
				//We have no data ; perhaps we need waiting for something...
			
				sLONG spentOnStep=0;
				
				if(verr==VE_SOCK_WOULD_BLOCK && fSslDelegate->WantRead())
					verr=WaitForRead(timeout, &spentOnStep);
				else if(verr==VE_SOCK_WOULD_BLOCK && fSslDelegate->WantWrite())
					verr=WaitForWrite(timeout, &spentOnStep);
		
				timeout-=spentOnStep;
				spentTotal+=spentOnStep;
			}
			while(verr==VE_OK);

			if(wasBlocking)
				SetBlocking(true);
		}	
		
		if(outMsSpent!=NULL)
			*outMsSpent=spentTotal;
	}
	else
	{
		sLONG timeout=inMsTimeout;
		sLONG spentTotal=0;
		
		do
		{
			len=*ioLen;
			
			sLONG spentOnStep=0;
			
			verr=WaitForRead(timeout, &spentOnStep);
			
			if(verr==VE_OK)
				verr=Read(outBuff, &len);
			else
				len=0;
			
			if(len>0)	//We sent some data ; that's enough for now.
				break;
			
			//No data were sent ; perhaps we need waiting for something...
			
			if(verr==VE_SOCK_WOULD_BLOCK)
			{
				//Although we wait for the socket to be ready, it may
				//happen that DoRead returns VE_SOCK_WOULD_BLOCK.
				//Prevent a mad loop with a small sleep, and retry...
				VTask::Sleep(100);
				timeout-=100;
				spentTotal+=100;
				verr=VE_OK;
			}
			
			timeout-=spentOnStep;
			spentTotal+=spentOnStep;
		}
		while(verr==VE_OK);
	}
	
	if(verr==VE_OK && len==0)
	{
		//Having nothing to read should result in VE_SOCK_TIMED_OUT. If read is OK but len is 0, it really means the peer
		//did a shutdown (or ssl equivalent). We return a special error so the caller dont try to call us in a mad loop.
		
		verr=VE_SOCK_PEER_OVER;
	}
	
	//Check len and err code are consistent
	xbox_assert((len==0 && verr!=VE_OK) || (len>0 && verr==VE_OK));

	//Check this error never escape !
	xbox_assert(verr!=VE_SOCK_WOULD_BLOCK);
	
	*ioLen=len;
	

	//timeout and peer over are not thrown (these are not /real/ errors)
	if(verr==VE_OK || verr==VE_SOCK_TIMED_OUT || verr==VE_SOCK_PEER_OVER)
		return verr;

	return vThrowError(verr);
}


VError XWinTCPSocket::WriteWithTimeout(const void* inBuff, uLONG* ioLen, sLONG inMsTimeout, sLONG* outMsSpent, bool unusedWithEmptyTail)
{	
	if(inBuff==NULL || ioLen==NULL)
		return vThrowError(VE_INVALID_PARAMETER);
	
	
	VError verr=VE_OK;
	
	uLONG len=*ioLen;
	
	if(fSslDelegate!=NULL)
	{
		sLONG spentTotal=0;
		
		bool wasBlocking=false;
		
		if(IsBlocking())
		{
			SetBlocking(false);
			wasBlocking=true;
		}
		
		sLONG timeout=inMsTimeout;
		
		do
		{
			len=*ioLen;
			
			verr=fSslDelegate->Write(inBuff, &len);
			
			if(len>0)	//We sent some data ; that's enough for now.
				break;
			
			//No data were sent ; perhaps we need waiting for something...
			
			sLONG spentOnStep=0;
			
			if(verr==VE_SOCK_WOULD_BLOCK && fSslDelegate->WantRead())
				verr=WaitForRead(timeout, &spentOnStep);
			else if(verr==VE_SOCK_WOULD_BLOCK && fSslDelegate->WantWrite())
				verr=WaitForWrite(timeout, &spentOnStep);
			
			timeout-=spentOnStep;
			spentTotal+=spentOnStep;
		}
		while(verr==VE_OK);
		
		if(wasBlocking)
			SetBlocking(true);
		
	}
	else
	{
		sLONG timeout=inMsTimeout;
		sLONG spentTotal=0;
		
		do
		{
			len=*ioLen;
			
			sLONG spentOnStep=0;
			
			verr=WaitForWrite(timeout, &spentOnStep);
			
			if(verr==VE_OK)
				verr=Write(inBuff, &len, unusedWithEmptyTail);
			else
				len=0;
			
			if(len>0)	//We sent some data ; that's enough for now.
				break;
			
			//No data were sent ; perhaps we need waiting for something...
			
			if(verr==VE_SOCK_WOULD_BLOCK)
			{
				//Although we wait for the socket to be ready, it may
				//happen that DoWrite returns VE_SOCK_WOULD_BLOCK.
				//Prevent a mad loop with a small sleep, and retry...
				VTask::Sleep(100);
				timeout-=100;
				spentTotal+=100;
				verr=VE_OK;
			}
			
			timeout-=spentOnStep;
			spentTotal+=spentOnStep;
		}
		while(verr==VE_OK);
	}

	//Check len and err code are consistent
	xbox_assert((len==0 && verr!=VE_OK) || (len>0 && verr==VE_OK));
	
	//Check those errors never escape !
	xbox_assert(verr!=VE_SOCK_WOULD_BLOCK);
	xbox_assert(verr!=VE_SOCK_PEER_OVER);
	
	*ioLen=len;
	
	return vThrowError(verr);	//might be VE_OK, which throws nothing.
}


//static
void XWinTCPSocket::TrashWithTimeout(Socket inFd, sLONG inMsTimeout, sLONG* outMsSpent)
{
	if(inFd==kBAD_SOCKET)
		return;
	
	if(inMsTimeout<0)
		return;
	
	uLONG8 start=VSystem::GetCurrentTime();

	uLONG8 stop=start+inMsTimeout;
	
	uLONG8 now=start;
	
	for(;;)
	{
		sLONG msTimeout=stop-now;
		
		timeval timeout;
		memset(&timeout, 0, sizeof(timeout));
		
		timeout.tv_sec=msTimeout/1000;
		timeout.tv_usec=1000*(msTimeout%1000);
		
        fd_set rs;
        FD_ZERO(&rs);
        FD_SET(inFd, &rs);
				
        int res=select(inFd+1, &rs, NULL, NULL, &timeout);

		if(outMsSpent!=NULL)
		{
			now=VSystem::GetCurrentTime();
			*outMsSpent=now-start;
		}
		
		//select failed, or timedout
		if(res!=1)
			break;
		
		char buf[1024];
		int len=sizeof(buf);
		int n=0;
		
		n=recv(inFd, buf, len, 0 /*flags*/);

		//recv failed, or all data is read.
		if(n<=0)
			break;
	}
}


//static
XWinTCPSocket* XWinTCPSocket::NewClientConnectedSock(const VString& inDnsName, PortNumber inPort, sLONG inMsTimeout)
{
	XWinTCPSocket* xsock=NULL;
	
	VNetAddressList addrs;
	
	VError verr=addrs.FromDnsQuery(inDnsName, inPort);
	
	VNetAddressList::const_iterator cit;
	
	for(cit=addrs.begin() ; cit!=addrs.end() ; cit++)
	{
		//We have a list of address structures ; we choose the first one that connects successfully.
	
		int sock=socket(cit->GetPfFamily(), SOCK_STREAM, IPPROTO_TCP);		
		if(sock==kBAD_SOCKET)
		{
			vThrowNativeError(WSAGetLastError());
		}
		else
		{
			int opt=true;

			int err=setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&opt), sizeof(opt));
			
			if(err!=0)
			{
				vThrowNativeError(WSAGetLastError());

				err=closesocket(sock);

				return NULL;
			}
			
			err=setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char*>(&opt), sizeof(opt));
			
			if(err!=0)
			{
				vThrowNativeError(WSAGetLastError());

				err=closesocket(sock);

				return NULL;
			}
				
			xsock=new XWinTCPSocket(sock);
			
			if(xsock==NULL)
			{
				err=closesocket(sock);

				vThrowError(VE_MEMORY_FULL);

				break;
			}
			
			VError verr=xsock->Connect(*cit, inMsTimeout);
			
			if(verr!=VE_OK)
			{
				xsock->Close();
				
				delete xsock;

				xsock=NULL;
			}
			else
			{
				//DEPRECATED ? - We have to set the service port (duplicated stuff on client sock).
				PortNumber resolvedPort=cit->GetPort();

				xbox_assert(resolvedPort==inPort);
				
				xsock->SetServicePort(resolvedPort);
			}
		}
	}

	return xsock;
}


//static
XWinTCPSocket* XWinTCPSocket::NewServerListeningSock(const VNetAddress& inAddr, Socket inBoundSock, bool inReuseAddress)
{
	bool alreadyBound=(inBoundSock!=kBAD_SOCKET) ? true : false;
	
	int sock=(alreadyBound) ? inBoundSock : socket(inAddr.GetPfFamily(), SOCK_STREAM, IPPROTO_TCP);
	
	VError verr=VE_OK;
	
	if(sock==kBAD_SOCKET)
		verr=vThrowNativeError(WSAGetLastError());
	
	XWinTCPSocket* xsock=NULL;
	
	if(verr==VE_OK)
	{	
		xsock=new XWinTCPSocket(sock);
	
		if(xsock==NULL)
			verr=vThrowError(VE_MEMORY_FULL);
	}
	
	if(verr==VE_OK)
	{
		xsock->SetServicePort(inAddr.GetPort());
		
		verr=xsock->Listen(inAddr, alreadyBound, inReuseAddress);
	}
	
	if(verr==VE_OK)
		verr=xsock->SetBlocking(false);
	
	if(verr!=VE_OK)
	{
		delete xsock;
		return NULL;
		
#if VERSIONDEBUG
		
		VString ip=inAddr.GetIP();
		PortNumber port=inAddr.GetPort();
		
		DebugMsg ("[%d] XWinTCPSocket::NewServerListeningSock() : Fail to get listening socket for ip %S on port %d\n",
				  VTask::GetCurrentID(), &ip, port);
#endif
	}
	
	return xsock;
}


//static
XWinTCPSocket* XWinTCPSocket::NewServerListeningSock(PortNumber inPort, Socket inBoundSock, bool inReuseAddress)
{
	VNetAddress anyAddr;
	VError verr=anyAddr.FromAnyIpAndPort(inPort);
	
	xbox_assert(verr==VE_OK);
	
	return NewServerListeningSock(anyAddr, inBoundSock, inReuseAddress);
}

XBOX::VError XWinTCPSocket::SetNoDelay (bool inYesNo)
{
	int	opt	= inYesNo;
	int err	= 0;


	err = setsockopt(fSock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&opt), sizeof(opt));

	return !err ? XBOX::VE_OK : vThrowNativeError(WSAGetLastError());
}


XBOX::VError XWinTCPSocket::PromoteToSSL(VKeyCertChain* inKeyCertChain)
{
	VSslDelegate* delegate=NULL;
	
	switch(fProfile)
	{
	case ConnectedSock :
			
			delegate=VSslDelegate::NewServerDelegate(GetRawSocket(), inKeyCertChain);
			break;
			
	case ClientSock :
			
			delegate=VSslDelegate::NewClientDelegate(GetRawSocket(), inKeyCertChain);
			break;
			
	default :

			xbox_assert(fProfile!=ConnectedSock && fProfile!=ClientSock);
	}
	
	if(delegate==NULL)
		return vThrowError(VE_SOCK_FAIL_TO_ADD_SSL);

	fSslDelegate=delegate;
	
	return VE_OK;
}


bool XWinTCPSocket::IsSSL()
{
	return fSslDelegate!=NULL;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// XWinAcceptIterator
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


XWinAcceptIterator::XWinAcceptIterator()
{
	fSockIt=fSocks.end();	// sc 05/07/2012
	fReadSet=new fd_set;
	FD_ZERO(fReadSet);
}


XWinAcceptIterator::~XWinAcceptIterator()
{
	delete fReadSet;
}


VError XWinAcceptIterator::AddServiceSocket(XWinTCPSocket* inSock)
{
	if(inSock==NULL)
		return VE_INVALID_PARAMETER;
	
	fSocks.push_back(inSock);

	//Push back may invalidate the collection iterator...
	fSockIt=fSocks.end();
	
	return VE_OK;
}


VError XWinAcceptIterator::ClearServiceSockets()
{
	fSocks.clear();
	
	//clear will invalidate the collection iterator...
	fSockIt=fSocks.end();
	
	return VE_OK;
}


VError XWinAcceptIterator::GetNewConnectedSocket(XWinTCPSocket** outSock, sLONG inMsTimeout)
{
	if(outSock==NULL)
		return vThrowError(VE_INVALID_PARAMETER);

	bool shouldRetry=false;
	
	VError verr=GetNewConnectedSocket(outSock, inMsTimeout, &shouldRetry);
	
	if(verr==VE_OK && *outSock==NULL && shouldRetry)
		verr=GetNewConnectedSocket(outSock, inMsTimeout, &shouldRetry /*ignored*/);
		
	return verr;
}


VError XWinAcceptIterator::GetNewConnectedSocket(XWinTCPSocket** outSock, sLONG inMsTimeout, bool* outShouldRetry)
{
	if(outSock==NULL || outShouldRetry==NULL)
		return vThrowError(VE_INVALID_PARAMETER);
	
	*outSock=NULL;
	*outShouldRetry=true;	//Indicate wether the select call is done (we should not retry) or not (we should retry).
	
	if(inMsTimeout<0)
		return VE_SOCK_TIMED_OUT;
	
	
	if(fSockIt==fSocks.end())
	{
		//We need to issue a new select call, with timeout
		*outShouldRetry=false;

        FD_ZERO(fReadSet);
		
		sLONG maxFd=-1;	//Ignored on Windows, but tell us we have found something to watch
		
		SockPtrColl::const_iterator cit;
		
		for(cit=fSocks.begin() ; cit!=fSocks.end() ; ++cit)
		{
			Socket fd=(*cit)->GetRawSocket();
			
            if((sLONG)fd>maxFd)
				maxFd=(sLONG)fd;
            
			FD_SET(fd, fReadSet);
		}
        
        //Ensure we have something to watch
        xbox_assert(maxFd!=-1);
        
        if(maxFd==-1)
            return vThrowError(VE_INVALID_PARAMETER);
		
		uLONG8 now=VSystem::GetCurrentTime();

		uLONG8 stop=now+inMsTimeout;

		for(;;)
		{
			uLONG8 msTimeout=stop-now;
			
			timeval timeout;
			memset(&timeout, 0, sizeof(timeout));
			
			timeout.tv_sec=msTimeout/1000;
			timeout.tv_usec=1000*(msTimeout%1000);
		
			int res=select(maxFd+1, fReadSet, NULL, NULL, &timeout);
	
			if(res==0)
				return VE_SOCK_TIMED_OUT;
			
			if(res>=1)
				break;

			return vThrowNativeError(WSAGetLastError());
		}
		
		fSockIt=fSocks.begin();
	}
	
	//We have to handle remaining service sockets ; find next ready one
		
	while(fSockIt!=fSocks.end())
	{
		Socket fd=(*fSockIt)->GetRawSocket();
		
		if(FD_ISSET(fd, fReadSet))
		{
			*outSock=(*fSockIt)->Accept(0 /*No timeout*/);
			
			++fSockIt;	//move to next socket ; prefer equity over perf !
			
			return VE_OK;
		}
		else
		{		
			++fSockIt;	//move to next socket ; prefer equity over perf !
		}
	}

	//No luck ; None of the remaining sockets were ready to accept... The caller 'shouldRetry' to call us !
	return VE_OK;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// XWinUDPSocket
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

XWinUDPSocket::~XWinUDPSocket()
{
	//Close();

#if VERSIONDEBUG
	if(fSock!=kBAD_SOCKET)
		DebugMsg ("[%d] XWinUDPSocket::~XWinUDPSocket() : socket %d not closed yet !\n", VTask::GetCurrentID(), fSock);
#endif
}


VError XWinUDPSocket::SetBlocking(uBOOL inBlocking)
{
	u_long nonBlocking=!inBlocking;
	
	int err=ioctlsocket(fSock, FIONBIO, &nonBlocking);
	
	if(err==SOCKET_ERROR)
		return vThrowNativeError(WSAGetLastError());
	
	return VE_OK;
}


VError XWinUDPSocket::Close()
{
	int err=closesocket(fSock);
			
	fSock=kBAD_SOCKET;

	return err==0 ? VE_OK : vThrowNativeError(WSAGetLastError());
}


VError XWinUDPSocket::Read(void* outBuff, uLONG* ioLen, VNetAddress* outSenderInfo)
{
	if(outBuff==NULL || ioLen==NULL)
		return vThrowError(VE_INVALID_PARAMETER);

	sockaddr_storage addr;
	socklen_t addrLen=sizeof(addr);

	int n=recvfrom(fSock, reinterpret_cast<char*>(outBuff), *ioLen, 0 /*flags*/, reinterpret_cast<sockaddr*>(&addr), &addrLen);
	
	if(n>=0)
	{
		*ioLen=n;

		if(outSenderInfo!=NULL)
			outSenderInfo->SetAddr(addr);

		return VE_OK;
	}
	
	//We have an error...
	*ioLen=0;
	
	//En principe on ne gere pas le mode non bloquant avec UDP
	
	if(WSAGetLastError()==WSAEWOULDBLOCK)
		return VE_SOCK_WOULD_BLOCK;

	// If datagram cannot fit in read buffer, Windows API returns an error.
	// (Not applicable for UNIX (Mac/Linux.)

	if (WSAGetLastError()==WSAEMSGSIZE) 
		return VE_SOCK_UDP_DATAGRAM_TOO_LONG;
	
	if(WSAGetLastError()==WSAECONNRESET || WSAGetLastError()==WSAENOTSOCK || WSAGetLastError()==WSAEBADF)
		return vThrowNativeCombo(VE_SOCK_CONNECTION_BROKEN, WSAGetLastError());

	return vThrowNativeCombo(VE_SOCK_READ_FAILED, WSAGetLastError());
}


VError XWinUDPSocket::Write(const void* inBuff, uLONG inLen, const VNetAddress& inReceiverInfo)
{
	if(inBuff==NULL)
		return vThrowError(VE_INVALID_PARAMETER);

	int n=sendto(fSock, reinterpret_cast<const char*>(inBuff), inLen, 0 /*flags*/, inReceiverInfo.GetAddr(), inReceiverInfo.GetAddrLen());
   	
	if(n>=0)
		return VE_OK;
		
	//En principe on ne gere pas le mode non bloquant avec UDP

	if(WSAGetLastError()==WSAEWOULDBLOCK)
		return VE_SOCK_WOULD_BLOCK;
	
	if(WSAGetLastError()==WSAECONNRESET || WSAGetLastError()==WSAENOTSOCK || WSAGetLastError()==WSAEBADF)
		return vThrowNativeCombo(VE_SOCK_CONNECTION_BROKEN, WSAGetLastError());
	
	return vThrowNativeCombo(VE_SOCK_WRITE_FAILED, WSAGetLastError());
}


//static
XWinUDPSocket* XWinUDPSocket::NewMulticastSock(const VString& inMultiCastIP, PortNumber inPort)
{
	VNetAddress localAddr(inPort);
	VNetAddress mcastAddr(inMultiCastIP);
	
	int sock=socket(localAddr.GetPfFamily(), SOCK_DGRAM, IPPROTO_UDP);
		
	if(sock==kBAD_SOCKET)
	{
		vThrowNativeError(WSAGetLastError());
		return NULL;
	}
	
	XWinUDPSocket* xsock=new XWinUDPSocket(sock);
		
	if(xsock==NULL)
		vThrowError(VE_MEMORY_FULL);

	int err=0;
	
	if(xsock!=NULL && err==0)
	{			
		int opt=true;

		err=setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
		//err=setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
	}
		
	if(xsock!=NULL && err==0)
	{
		if(ListV6() && mcastAddr.IsV6())
		{
			int ifCount=0;

			ipv6_mreq mreq;	
			memset(&mreq, 0, sizeof(mreq));
			
			//If this member specifies an interface index of 0, the default multicast interface is used.
			mreq.ipv6mr_interface=0;
			mcastAddr.FillIpV6(&mreq.ipv6mr_multiaddr);
			
			err=setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, reinterpret_cast<const char*>(&mreq), sizeof(mreq));
			
			if(err==0)
			{
				ifCount++;
				
				DebugMsg("Subscribed multicast addr %S\n", &inMultiCastIP);
			}
			else
			{
				DebugMsg("Fail to subscribe to multicast addr %S\n", &inMultiCastIP);
			}

			//If optval is set to NULL on call to setsockopt, the default IPv6 interface is used.
			//If optval is zero , the default interface for receiving multicast is specified for sending multicast traffic.

			int opt=NULL;
			err=setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, reinterpret_cast<const char*>(&opt), sizeof(opt));
			
			//Si une seule interface passe, c'est bon. Sinon on considere l'erreur de la derniere interface.
			if(ifCount>0)
				err=0;
		}
		else if(ListV4() && mcastAddr.IsV4())
		{
			ip_mreq	mreq;	
			memset(&mreq, 0, sizeof(mreq));
	
			mreq.imr_interface.s_addr=INADDR_ANY;	//Let the OS choose an interface
			mcastAddr.FillIpV4((IP4*)&(mreq.imr_multiaddr.s_addr));	
			
			err=setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<const char*>(&mreq), sizeof(mreq));
		}
		else
		{
			err=1;
			vThrowError(VE_INVALID_PARAMETER);
		}
			
		xbox_assert(err!=1);
		
		if(err==-1)
			vThrowNativeCombo(VE_SRVR_CANT_SUBSCRIBE_MULTICAST, WSAGetLastError());
		else if(err==1)
			vThrowError(VE_SRVR_CANT_SUBSCRIBE_MULTICAST);
			
	}
		
	if(xsock!=NULL && err==0)
	{		
		err=bind(sock, localAddr.GetAddr(), localAddr.GetAddrLen());
		
		if(err!=0)
			vThrowNativeCombo(VE_SRVR_CANT_SUBSCRIBE_MULTICAST, WSAGetLastError());
	}
	
	if(xsock==NULL || err!=0)
	{
		err=closesocket(sock);
		
		delete xsock;
		
		return NULL;
	}
	
	return xsock;
}


END_TOOLBOX_NAMESPACE

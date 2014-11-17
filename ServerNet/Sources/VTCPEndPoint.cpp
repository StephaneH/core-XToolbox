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

#include "VTCPEndPoint.h"

#include "Session.h"
#include "Tools.h"
#include "VSslDelegate.h"

#if VERSIONWIN
	#include "XWinSocket.h"
#else
	#include "XBsdSocket.h"
#endif

 
BEGIN_TOOLBOX_NAMESPACE


using namespace ServerNetTools;


/* A required symbol for components mechanism to link. */
void xDllMain ( void )
{
	;
}


VTCPEndPoint::VTCPEndPoint( XTCPSock* inSock ) : fUsageMutex ( )
{
	fSock = inSock;
	fSIOPool = 0;

	Init ( );
}

VTCPEndPoint::VTCPEndPoint ( XTCPSock* inSock, VTCPSelectIOPool* vtcpSIOPool ) : fUsageMutex ( )
{
	fSock = inSock;
	fSIOPool = vtcpSIOPool;

	if ( fSIOPool )
		fSIOPool-> Retain ( );
	
	Init ( );
}

VTCPEndPoint::~VTCPEndPoint ( )
{
	Close();
	
	if ( fSock )
		delete fSock;
	
	if( fSIOPool )
	{
		fSIOPool-> Release ( );
		fSIOPool = NULL;
	}

	if ( fWasUsedAtLeastOnce )
	{
		VError					vError = VTCPSessionManager::Get ( )-> Remove ( this );
		xbox_assert ( vError == VE_OK );
	}

	if ( fClientSession != 0 )
		fClientSession-> Release ( );
}

void VTCPEndPoint::SetSelectIOPool (VTCPSelectIOPool *inSelectIOPool)
{
	xbox_assert(fSIOPool == NULL && inSelectIOPool != NULL);
	fSIOPool = XBOX::RetainRefCountable<VTCPSelectIOPool>(inSelectIOPool);
}

void VTCPEndPoint::Init ( )
{
	fIsSelectIO = false;
	fIsWatching = false;
	fSIOHandler = 0;
	fLastNonZeroSelectIORead = VSystem::GetCurrentTime ( );
	fSelectIOInactivityTimeout = GetSelectIODelay ( ); // milliseconds
	fIdleTimeout = 0;
	fPostponeTimeout = 0;
	fSignalCriticalError = false;
	fWasUsedAtLeastOnce = false;
	fClientSession = 0;
	fSimpleID = GetNextSimpleID ( );

	if ( fSock != 0 )
	{
		fIP = fSock-> GetIP ( );
		fPort = fSock-> GetPort ( );
		fIsSSL = fSock-> IsSSL ( );
		fIsBlocking = true;
	}
	else
	{
		fPort = -1;
		fIsSSL = false;
		fIsBlocking = true;
	}
	fIsInAutoReconnect = false;
	fIsInUse = false;
	fShouldStop = false;

	VTCPEndPointFactory::GenerateSessionID ( fStringID );

	fID = -1;
}


void VTCPEndPoint::GetIP ( XBOX::VString& outIP )
{	
	outIP=fIP;
}

VString VTCPEndPoint::GetIP() const
{	
	return fIP;
}


bool VTCPEndPoint::IsSSL ( )
{
	if ( fSock != 0 )
		return fSock-> IsSSL ( );
	else
		return fIsSSL;
}

Socket VTCPEndPoint::GetRawSocket ( ) const
{
	return fSock ? fSock-> GetRawSocket ( ) : kBAD_SOCKET;
}


void VTCPEndPoint::SetIsBlocking ( bool inIsBlocking )
{
#if VERSIONDEBUG
	//DebugMsg ("[%d] VTCPEndPoint::SetIsBlocking() : socket=%d block=%d\n",
	//		  VTask::GetCurrentID(), GetRawSocket(), inIsBlocking);
#endif
	
	fIsBlocking = inIsBlocking;

	if ( !fSock )
		return;

	if ( inIsBlocking && fIsSelectIO )
		return;

	if ( fSock )
		fSock-> SetBlocking ( inIsBlocking );
}

void VTCPEndPoint::SetIsSelectIO ( bool bIsSelectIO )
{
#if VERSIONDEBUG
//		DebugMsg ("[%d] VTCPEndPoint::SetIsSelectIO() : socket=%d selio=%d\n",
//				  VTask::GetCurrentID(), GetRawSocket(), bIsSelectIO);
#endif
	
	if ( fIsSelectIO == bIsSelectIO )
		return;

	if ( !bIsSelectIO )
	{
		fIsSelectIO = false;

		return;
	}

	if ( !fSIOPool )
		return;

	SetIsBlocking ( false );
	fIsSelectIO = true;
}

void VTCPEndPoint::SignalCriticalError ( VError inError )
{
	if(fSignalCriticalError)
		SignalNetError(inError);
}

VError VTCPEndPoint::GetSystemReadError ( sLONG inSocketError )
{
	VError		vError = VE_OK;

	VError		vSystemError = VE_SRVR_READ_FAILED;
	int			nSystemError = 0;
	#if VERSIONWIN
		nSystemError = WSAGetLastError ( );	

		if ( nSystemError == 10053 || nSystemError == 10054 )
			vSystemError = VE_SRVR_CONNECTION_BROKEN;
		else if ( nSystemError == 10035 )
			vSystemError = VE_SRVR_RESOURCE_TEMPORARILY_UNAVAILABLE;
	#else
		nSystemError = -1;
	#endif

	if ( vSystemError != VE_SRVR_RESOURCE_TEMPORARILY_UNAVAILABLE )
	{
		VString				vstrErrorCode;
		vstrErrorCode. AppendLong ( inSocketError );
		VString				vstrSystemErrorCode;
		vstrSystemErrorCode. AppendLong ( nSystemError );
		vError = ThrowNetError ( vSystemError, &vstrErrorCode, &vstrSystemErrorCode );
	}

	return vError;
}

Boolean VTCPEndPoint::ClientConnectionIsDropped ( long inSocketError, long inSystemError )
{
	#if VERSIONWIN
	// L.E.07/08/08 leaving bugbase client in the background for a long time issue WSAECONNABORTED
		return	(inSocketError == SOCKET_ERROR && ( inSystemError == 0 || inSystemError == WSAECONNRESET || inSystemError == WSAECONNABORTED)) ||
				inSystemError == WSAENOTSOCK || inSystemError == WSAEBADF;
	#else
		return	(inSocketError == -1 && ( inSystemError == 0 || inSystemError == ECONNRESET || inSystemError == ECONNABORTED)) ||
				inSystemError == ENOTSOCK || inSystemError == EBADF;
	#endif
}

VError VTCPEndPoint::SetReadCallback (CTCPSelectIOHandler::ReadCallback *inCallback , void *inData)
{
	if(fIsWatching)
		return VE_SRVR_SOCKET_ALREADY_WATCHING;

	xbox_assert(inCallback != NULL);

//	xbox_assert(fIsSelectIO);
	xbox_assert(fSIOPool != NULL);
	xbox_assert(!fIsWatching);

	XBOX::VError	error;

	fSIOHandler = fSIOPool-> AddSocketForWatching(this, inData, inCallback, error);
	if (error == XBOX::VE_OK)

		fIsWatching = true;

	return error;
}

VError VTCPEndPoint::DisableReadCallback ()
{
//	xbox_assert(fIsSelectIO);
	xbox_assert(fSIOHandler != NULL);
//	xbox_assert(fIsWatching);

	XBOX::VError	error = VE_OK;

	if ( fSIOHandler != NULL )
	{
		error = fSIOHandler->RemoveSocketForWatching(GetRawSocket());
		fSIOHandler = NULL;
	}
	else
		xbox_assert ( false );

	fIsWatching = false;

	SetIsBlocking(false);
	SetIsSelectIO(true);

	return error;
}


VError VTCPEndPoint::DoRead ( void *outBuff, uLONG *ioLen, sLONG inMsTimeout, sLONG* outMsSpent)
{
	if ( !fSock )
		return ReportError(VE_SRVR_NULL_ENDPOINT);

	if(inMsTimeout<0)
		return ReportError(VE_INVALID_PARAMETER);

	xbox_assert ( !fIsInAutoReconnect || ( fIsInAutoReconnect && fIsInUse ) );

	if ( fIsSelectIO && !fIsWatching)
	{
		uLONG					nStart = VSystem::GetCurrentTime ( );
		uLONG					nOriginalLen = *ioLen;
		VError					vError = _Read_SelectIO ( ( char* ) outBuff, ioLen, inMsTimeout );
		uLONG					nTimeSpent = VSystem::GetCurrentTime ( ) - nStart;
		if ( outMsSpent != NULL )
			*outMsSpent = nTimeSpent;

		if ( vError == VE_SRVR_RESOURCE_TEMPORARILY_UNAVAILABLE )
		{
			VSslDelegate*		sslDelegate = GetSSLDelegate ( );
			if ( sslDelegate != NULL )
			{
				while ( vError == VE_SRVR_RESOURCE_TEMPORARILY_UNAVAILABLE && nTimeSpent < inMsTimeout )
				{
					*ioLen = nOriginalLen;
					vError = _Read_SelectIO ( ( char* ) outBuff, ioLen, inMsTimeout - nTimeSpent );
					nTimeSpent = VSystem::GetCurrentTime ( ) - nStart;
					if ( outMsSpent != NULL )
						*outMsSpent = nTimeSpent;
				}
			}
		}

		return vError;
	}
	
	VError verr=VE_OK;
	
	if(inMsTimeout<=0)
		verr=fSock->Read((char*)outBuff, ioLen);
	else
		verr=fSock->ReadWithTimeout((char*) outBuff, ioLen, inMsTimeout, outMsSpent);

	if ( verr == VE_OK )
		return VE_OK;

	if ( verr == VE_SOCK_CONNECTION_BROKEN )
		return ReportError(VE_SRVR_CONNECTION_BROKEN);
	
	if(verr==VE_SOCK_TIMED_OUT)
		return ReportError(VE_SRVR_READ_TIMED_OUT, false, false);
		
	if ( verr == VE_SOCK_WOULD_BLOCK )
		return ReportError(VE_SRVR_RESOURCE_TEMPORARILY_UNAVAILABLE, false, false);

	if(verr==VE_SOCK_PEER_OVER)
		return ReportError(VE_SRVR_NOTHING_TO_READ, false, false);

	return ReportError(VE_SRVR_READ_FAILED);
}

VError VTCPEndPoint::_Read_SelectIO ( void *outBuff, uLONG *ioLen, uLONG inMsTimeout )
{
	if ( !fSock )
		return ThrowNetError ( VE_SRVR_NULL_ENDPOINT );

	VError							vError = VE_OK;

	VSslDelegate* sslDelegate=GetSSLDelegate();

	/*	- Should keep the time when the last non-zero selectIO read was done
		- If the timeout has not been reached since the last read then attempt direct select
		- If the timeout has been reached then post the socket for reading to fSIOHandler
	*/
	struct timeval					tvTimeout;
	tvTimeout. tv_sec = 0;
	tvTimeout. tv_usec = 100000;

	int nRawSocket = GetRawSocket ( );
	fd_set							fdReadSocket;
	FD_ZERO ( &fdReadSocket );
	FD_SET ( nRawSocket, &fdReadSocket );

	int								nSocketsReadyForRead = 0;

	if(sslDelegate==NULL)
	{
		//No try-to-read-if-ready optimization with SSL : quite often the socket is 'ready' but it contains only ssl
		//protocol datas, and no applicative datas, which we can not handle easily with this algo.

		nSocketsReadyForRead=select ( FD_SETSIZE, &fdReadSocket, 0, 0, &tvTimeout );
	}
	else
	{
		VError verr=VE_OK;

		if(sslDelegate->NeedHandshake())
		{
			//On first read, client need to initiate ssl handshake. With SelectIO, we have to do it manually :(
			//If we don't, the server won't ever send anything, and no activity will take place on the socket !
			
			sslDelegate->DummyWriteToTriggerHandshake();
		}
		else
		{
			//Sometimes, SSL has some already buffered datas... We should not wait on the socket for those datas !

			sLONG bufferedLen=sslDelegate->GetBufferedDataLen();

			if(bufferedLen>0)
			{
				if(*ioLen>bufferedLen)
					*ioLen=bufferedLen;
			
				verr=sslDelegate->Read(outBuff, ioLen);

				return verr;
			}

			nSocketsReadyForRead=select ( FD_SETSIZE, &fdReadSocket, 0, 0, &tvTimeout );
			if ( nSocketsReadyForRead > 0 )
			{
				verr=sslDelegate->Read(outBuff, ioLen);

				return verr;
			}
			else
				nSocketsReadyForRead = 0;
		}
	}
	

	DEBUG_CHECK_SOCK_RESULT( nSocketsReadyForRead, "select from endpoint", nRawSocket);

	uLONG							nCurrentTime = VSystem::GetCurrentTime ( );

	if ( nSocketsReadyForRead == 0 )
	{
		if ( nCurrentTime - fLastNonZeroSelectIORead > fSelectIOInactivityTimeout || sslDelegate != NULL )
		{
			fSIOHandler = fSIOPool-> AddSocketForReading ( this, vError );
			if ( vError == VE_OK )
			{
				//XBOX::DebugMsg ( "SelectIO::Read is transferring the socket to the waiting thread.\n" );
				sLONG				nError = 0;
				sLONG				nSystemError = 0;
				vError = fSIOHandler-> Read ( GetRawSocket ( ), ( char* ) outBuff, ioLen, nError, nSystemError, inMsTimeout );
				if ( vError != VE_OK && vError != VE_SRVR_RESOURCE_TEMPORARILY_UNAVAILABLE )
				{
					VString			vstrError;
					vstrError. AppendLong ( nError );
					VString			vstrSystemError;
					vstrSystemError. AppendLong ( nSystemError );
					ThrowNetError ( vError, &vstrError, &vstrSystemError );
				}

				/* There should be a flag to automatically remove the socket from selectIO reading after a successfull read. */
				if ( fSIOHandler != 0 )
				{
					fSIOHandler-> RemoveSocketForReading ( GetRawSocket ( ) );
					fSIOHandler = 0;
				}
				else
				{
					*ioLen = 0;
					vError = ThrowNetError ( VE_SRVR_CONNECTION_BROKEN );
				}

				if ( *ioLen > 0 )
				{
					nCurrentTime = VSystem::GetCurrentTime ( );
					fLastNonZeroSelectIORead = nCurrentTime;
				}
			}
		}
		else
		{

			*ioLen = 0;
			vError = VE_SRVR_RESOURCE_TEMPORARILY_UNAVAILABLE;
		}
	}
	else if ( nSocketsReadyForRead > 0 )
	{
		fLastNonZeroSelectIORead = nCurrentTime;

#if VERSIONMAC || VERSION_LINUX
		ssize_t nRead=0;
#else
		int	nRead=0;
#endif

		nRead = recv ( GetRawSocket ( ), ( char* ) outBuff, *ioLen, 0 );

		if ( nRead > 0 )
			*ioLen = static_cast<uLONG>(nRead);
		else if ( nRead == 0 )
		{
			*ioLen = 0;
			vError = ThrowNetError ( VE_SRVR_CONNECTION_BROKEN );
			SignalCriticalError ( vError );
		}
		else
		{
			*ioLen = 0;
			vError = GetSystemReadError ( static_cast<sLONG>(nRead) );
		}
	}
	else
	{
		*ioLen = 0;
		vError = GetSystemReadError ( nSocketsReadyForRead );
	}

	return vError;
}

VError VTCPEndPoint::DoReadExactly(void *outBuff, uLONG* ioLen, sLONG inMsTimeout)
{
	if (fSock==NULL)
		return ReportError(VE_SRVR_NULL_ENDPOINT);

	if(ioLen==NULL)
		return ReportError(VE_INVALID_PARAMETER);
	
	if(inMsTimeout<0)
		return ReportError(VE_INVALID_PARAMETER);

	//Update to support partial read log and debug (Do not work with select IO)
	uLONG inLen=*ioLen;
	*ioLen=0;
	
	//To be consistent with WriteExactly...
	if (inLen==0)
		return VE_OK;

	/* Achtung! */
	xbox_assert ( !fIsInAutoReconnect || ( fIsInAutoReconnect && fIsInUse ) );

	if ( fIsSelectIO && !fIsWatching)
	{
		VError verr=_ReadExactly_SelectIO((char*)outBuff, inLen, inMsTimeout);

		if(verr==VE_OK)
			*ioLen=inLen;

		return verr;
	}

	char* start=reinterpret_cast<char*>(outBuff);
	char* past=start+inLen;
	char* pos=start;
	
	
	bool withTimeout=(inMsTimeout>0);
	
	sLONG timeoutMs=inMsTimeout;
	
	
	for(;;)
	{
		if (fSock == NULL)
			return ReportError(VE_SRVR_CONNECTION_BROKEN);

		uLONG len=static_cast<uLONG>(past-pos);
		
		VError verr=VE_OK;

		if(withTimeout)
		{
			sLONG spentMs=0;

			verr=fSock->ReadWithTimeout(pos, &len, timeoutMs, &spentMs);
			
			timeoutMs-=spentMs;
		}
		else
			verr=fSock->Read(pos, &len);
		
		*ioLen+=len;
		
		if(verr==VE_SOCK_CONNECTION_BROKEN)
			return ReportError(VE_SRVR_CONNECTION_BROKEN);

		if(verr==VE_SOCK_TIMED_OUT)
			return ReportError(VE_SRVR_READ_TIMED_OUT, false, false);

		if(verr==VE_SOCK_READ_FAILED || verr==VE_SSL_READ_FAILED)
			return ReportError(VE_SRVR_READ_FAILED);

		if(verr==VE_SOCK_PEER_OVER)
			return ReportError(VE_SRVR_NOTHING_TO_READ, false, false);
			
		xbox_assert(verr==VE_OK);	//No unhandled error !
						
		if(verr!=VE_OK)
			return ReportError(VE_SRVR_READ_FAILED);
		
		pos+=len;

		if(pos>=past)
		{
			xbox_assert(pos==past);
			break;
		}

		if ( fShouldStop )
			return ReportError(VE_SRVR_READ_FAILED, false, false);

		//jmo - Je laisse ce Yield, mais ne suis pas sûr qu'il soit indispensable.
		VTask::Yield();
	}

	return VE_OK;
}

VError VTCPEndPoint::_ReadExactly_SelectIO ( void *outBuff, uLONG inLen, uLONG inMsTimeout )
{
	VError			vError = VE_OK;

	uLONG			nStart = 0;
	uLONG			nTimeLeft = 0;
	if ( inMsTimeout > 0 )
	{
		nStart = VSystem::GetCurrentTime ( );
		nTimeLeft = inMsTimeout;
	}

	uLONG			dejalen = 0;
	do
	{
		uLONG curlen = inLen - dejalen;
		if (fSock == NULL)
			return ThrowNetError ( VE_SRVR_CONNECTION_BROKEN );

		vError = _Read_SelectIO ( ( char* ) outBuff + dejalen, &curlen, nTimeLeft );
		if ( vError != VE_OK && vError != VE_SRVR_RESOURCE_TEMPORARILY_UNAVAILABLE )
			return vError;

		if ( curlen > 0 )
			dejalen = dejalen + curlen;
		if (dejalen >= inLen)
			break;

		if ( inMsTimeout > 0 )
		{
			uLONG		nTimeSpent = VSystem::GetCurrentTime ( ) - nStart;
			if ( nTimeSpent >= inMsTimeout )
				return VE_SRVR_READ_TIMED_OUT;
			else
				nTimeLeft = inMsTimeout - nTimeSpent;
		}

		if ( fShouldStop )
			return VE_SRVR_READ_FAILED;

		VTask::Yield();
	} while (true);

	return VE_OK;
}


VError VTCPEndPoint::DoWrite ( void *inBuff, uLONG *ioLen, sLONG inMsTimeout, sLONG* outMsSpent ,bool inWithEmptyTail )
{
	if ( !fSock )
		return ReportError( VE_SRVR_NULL_ENDPOINT );

	if(inMsTimeout<0)
		return ReportError(VE_INVALID_PARAMETER);

	xbox_assert ( !fIsInAutoReconnect || ( fIsInAutoReconnect && fIsInUse ) );

	VError verr=VE_OK;
	
	if(inMsTimeout<=0)
		verr=fSock->Write((char*)inBuff, ioLen, inWithEmptyTail);
	else
		verr=fSock->WriteWithTimeout((char*)inBuff, ioLen, inMsTimeout, outMsSpent, inWithEmptyTail);
	
	if( verr == VE_OK )
		return VE_OK; 

	if ( verr == VE_SOCK_CONNECTION_BROKEN )
		return ReportError( VE_SRVR_CONNECTION_BROKEN );
	
	if(verr==VE_SOCK_TIMED_OUT)
		return ReportError(VE_SRVR_WRITE_TIMED_OUT, false, false);

	if ( verr == VE_SOCK_WOULD_BLOCK )
		return ReportError(VE_SRVR_RESOURCE_TEMPORARILY_UNAVAILABLE, false, false);
		
	return ReportError( VE_SRVR_WRITE_FAILED );
	}


VError VTCPEndPoint::DoWriteExactly(const void *inBuff, uLONG* ioLen, sLONG inMsTimeout)
{
	if (fSock==NULL)
		return ReportError( VE_SRVR_NULL_ENDPOINT );

	if(ioLen==NULL)
		return ReportError(VE_INVALID_PARAMETER);
	
	if(inMsTimeout<0)
		return ReportError(VE_INVALID_PARAMETER);

	//Update to support partial write log and debug (Do not work with select IO)
	uLONG inLen=*ioLen;
	*ioLen=0;

	if ( inLen == 0 )
		return VE_OK;

	xbox_assert ( !fIsInAutoReconnect || ( fIsInAutoReconnect && fIsInUse ) );

	const char* start=reinterpret_cast<const char*>(inBuff);
	const char* past=start+inLen;
	const char* pos=start;
	
	
	bool withTimeout=(inMsTimeout>0);
	
	sLONG timeoutMs=inMsTimeout;
	
	
	for(;;)
	{
		uLONG len=static_cast<uLONG>(past-pos);
		
		VError verr=VE_OK;
		
		if(withTimeout)
		{
			sLONG spentMs=0;

			verr=fSock->WriteWithTimeout(pos, &len, timeoutMs, &spentMs, false /*withEmptyTail*/);

			timeoutMs-=spentMs;
		}
		else
			verr=fSock->Write(pos, &len, false);

		*ioLen+=len;
		
		if(verr==VE_SOCK_CONNECTION_BROKEN)
			return ReportError(VE_SRVR_CONNECTION_BROKEN);

		if(verr==VE_SOCK_TIMED_OUT)
			return ReportError(VE_SRVR_WRITE_TIMED_OUT, false, false);
			
		if(verr==VE_SOCK_WRITE_FAILED || verr==VE_SSL_WRITE_FAILED)
			return ReportError(VE_SRVR_WRITE_FAILED);

		xbox_assert(verr==VE_OK);	//No unhandled error !

		if(verr!=VE_OK)
			return ReportError(VE_SRVR_WRITE_FAILED);
		
		pos+=len;

		if(pos>=past)
		{
			xbox_assert(pos==past);
			break;
		}

		if ( fShouldStop )
			return ReportError(VE_SRVR_WRITE_FAILED, false, false);
		
		//jmo - To be consistent with ReadExactly					
		VTask::Yield();
	}

	return VE_OK;
}


ReadNotificationMessage::ReadNotificationMessage(VTCPEndPoint* inEndPoint) : fEndPoint(inEndPoint)
{
	RetainRefCountable(fEndPoint);
}

ReadNotificationMessage::~ReadNotificationMessage()
{
	ReleaseRefCountable(&fEndPoint);
}

void ReadNotificationMessage::DoExecute()
{
	//Really nothing to do here, except unlock some waiting task with a donothing message

	ILogger* logger=VProcess::Get()->GetLogger();

	bool shouldTrace=(logger!=NULL) ? logger->ShouldLog(EML_Trace) : false;

	VValueBag* tBag=NULL;

	if(shouldTrace)
		tBag=new VValueBag;

	shouldTrace&=(tBag!=NULL);

	if(shouldTrace)
	{
		ILoggerBagKeys::level.Set(tBag, EML_Trace);

		ILoggerBagKeys::component_signature.Set(tBag, kSERVER_NET_SIGNATURE);

		ILoggerBagKeys::task_id.Set(tBag, VTask::GetCurrentID());

		ILoggerBagKeys::message.Set(tBag, CVSTR("ReadNotificationMessage::DoExecute"));

		ILoggerBagKeys::local_addr.Set(tBag, GetLocalIP(fEndPoint));

		ILoggerBagKeys::peer_addr.Set(tBag, GetPeerIP(fEndPoint));

		ILoggerBagKeys::local_port.Set(tBag, GetLocalPort(fEndPoint));

		ILoggerBagKeys::peer_port.Set(tBag, GetPeerPort(fEndPoint));

		ILoggerBagKeys::socket.Set(tBag, fEndPoint->GetRawSocket());

		ILoggerBagKeys::is_blocking.Set(tBag, fEndPoint->IsBlocking());

		ILoggerBagKeys::is_select_io.Set(tBag, fEndPoint->IsSelectIO());

		ILoggerBagKeys::is_ssl.Set(tBag, fEndPoint->IsSSL());

		logger->LogBag(tBag);

		ReleaseRefCountable(&tBag);
	}
}

OsType ReadNotificationMessage::GetCoalescingSignature() const
{
	return kSERVER_NET_SIGNATURE;
}

bool ReadNotificationMessage::DoCoalesce(const VMessage& inNewMessage)
{
	return false;
}


//static
bool VTCPEndPoint::DoNotifyReadCallback(Socket /*inRawSocket*/, VEndPoint* inEndPoint, void *inData, sLONG /*inErrorCode*/)
{
	VTCPEndPoint* endPoint=reinterpret_cast<VTCPEndPoint*>(inEndPoint);
	xbox_assert(endPoint!=NULL);

	std::pair<VMessage*, IMessageable*>* p=reinterpret_cast<std::pair<VMessage*, IMessageable*>*>(inData);
	xbox_assert(p!=NULL);

	if(endPoint==NULL || p==NULL)
		return false;

	VMessage* msg=reinterpret_cast<VMessage*>(p->first);
	IMessageable* target=reinterpret_cast<IMessageable*>(p->second);

	delete p;
	p=NULL;

	xbox_assert(target!=NULL);

	bool res=(target!=NULL);

	//If not given a specific message, post a generic one
	if(res && msg==NULL)
		msg=new ReadNotificationMessage(endPoint);

	xbox_assert(msg!=NULL);

	res&=(msg!=NULL);

	if(res)
	{
		res=msg->PostTo(target);
//#if WITH_SERVER_NET_OPTIMIZE
//for ( int i = 0; i < 5; i++ )
	XBOX::VTask::GetCurrent ( )-> YieldNow ( ); // IMPROVES A BIT
//#endif
	}

	xbox_assert(res=true);

	ReleaseRefCountable(&msg);
	//ReleaseRefCountable(&target);

	endPoint->fIsWatching=false;

	return false;
}


VError VTCPEndPoint::EnableAutoReconnect ( )
{
	fIsInAutoReconnect = true;
	
	return VE_OK;
}

VError VTCPEndPoint::DisableAutoReconnect ( )
{
	fIsInAutoReconnect = false;
	
	return VE_OK;
}

void VTCPEndPoint::SetClientSession ( VTCPClientSession* inSession )
{
	fClientSession = inSession;
	if ( fClientSession != 0 )
		fClientSession-> Retain ( );
}


bool VTCPEndPoint::IsPostponed ( )
{
	return fSock == 0;
}

VError VTCPEndPoint::Postpone ( )
{
	if ( fIsSelectIO && fSIOHandler )
	{
		fSIOHandler-> RemoveSocketForReading ( GetRawSocket ( ) );
		fSIOHandler = 0;
	}
	if ( fSock )
	{
		fIsSSL = fSock-> IsSSL ( );
		
		fSock->Close(false);
		
		delete fSock;
		
		fSock = NULL;
	}

	VTime::Now ( fPostponeStart );

	return VE_OK;
}

VError VTCPEndPoint::DoClose ( )
{
	if ( fIsSelectIO && fSIOHandler )
	{
		fSIOHandler-> RemoveSocketForReading ( GetRawSocket ( ) );
		fSIOHandler = 0;
	}
	if ( fWasUsedAtLeastOnce )
	{
		VError					vError = VTCPSessionManager::Get ( )-> Remove ( this );
		xbox_assert ( vError == VE_OK );
	}

	ReleaseRefCountable ( &fClientSession );

	if ( fSock )
	{
		fSock->Close();

		delete fSock;
	}
	
	fSock = NULL;

	return VE_OK;
}

VError VTCPEndPoint::DoForceClose ( )
{
	if ( fSock == 0 )
		return VE_OK;

	if ( fWasUsedAtLeastOnce )
	{
		VError					vError = VTCPSessionManager::Get ( )-> Remove ( this );
		xbox_assert ( vError == VE_OK );
	}

	fShouldStop = true;

	int							nRawSocket = GetRawSocket ( );
	CTCPSelectIOHandler*		sioHandler = fSIOHandler;
	if ( fIsSelectIO && sioHandler != 0 )
	{
		sioHandler-> RemoveSocketForReading ( nRawSocket );
		//fSIOHandler = 0; Must not set it to null here.
	}

	if ( fIsBlocking )
		fSock-> Close ( true /*withRecvLoop*/ );
	
	return VE_OK;
}

bool VTCPEndPoint::IsIdleTimedOut ( )
{
	if ( fIdleTimeout == 0 )
		return false;

	xbox_assert ( fIdleStart. GetMilliseconds ( ) > 0 );

	VTime				vtNow;
	VTime::Now ( vtNow );
	bool				bResult = vtNow. GetMilliseconds ( ) - fIdleStart. GetMilliseconds ( ) > fIdleTimeout;

	return bResult;
}

bool VTCPEndPoint::IsPostponeTimedOut ( )
{
	if ( fPostponeTimeout == 0 )
		return false;

	xbox_assert ( fPostponeStart. GetMilliseconds ( ) > 0 );

	VTime				vtNow;
	VTime::Now ( vtNow );
	bool				bResult = vtNow. GetMilliseconds ( ) - fPostponeStart. GetMilliseconds ( ) > fPostponeTimeout;

	return bResult;
}

bool VTCPEndPoint::TryToUse ( )
{
	bool				bResult = fUsageMutex. TryToLock ( );
	if ( bResult )
		fIsInUse = true;

	return bResult;
}

VError VTCPEndPoint::Use ( )
{
	xbox_assert ( fIsInAutoReconnect );
	xbox_assert ( fClientSession != 0 );

	if ( !fUsageMutex. Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;

	fIsInUse = true;

	VError					vError = VE_OK;
	if ( !fWasUsedAtLeastOnce )
	{
		vError = VTCPSessionManager::Get ( )-> Add ( this );
		if ( vError == VE_OK )
			fWasUsedAtLeastOnce = true;
	}
	else if ( IsPostponed ( ) )
	{
		xbox_assert ( fClientSession != 0 );
		if ( fClientSession == 0 )
		{
			ReportError( VE_SRVR_SESSION_NOT_FOUND, true, false );

			vError = ReportError( VE_SRVR_INVALID_INTERNAL_STATE, true, false );

			VTCPSessionManager::DebugMessage ( CVSTR ( "Session not found" ), this, VE_SRVR_SESSION_NOT_FOUND );
		}
		else
		{
			if ( VTCPSessionManager::Get ( )-> IsPostponed ( this ) )
			{
				VTCPEndPoint*				vtcpEndPointRestored = fClientSession-> Restore ( *this, vError );
#if 1
	if ( (vError != VE_OK && vError != VE_SRVR_SESSION_PROTOCOL_FAILED) || vtcpEndPointRestored == 0 )
	{
		sLONG			nSleepTime = ( VSystem::Random ( ) % 30 ) * 1000;
		if ( nSleepTime < 5000 )
			nSleepTime += 5000;

		StErrorContextInstaller			stErrContextTemp ( true, false );
		VErrorContext*					errContext = 0;
		while (	vError != VE_OK &&
				vError != VE_SRVR_SESSION_PROTOCOL_FAILED &&
				VTask::GetCurrent ( )-> GetState ( ) != TS_DYING &&
				VTask::GetCurrent ( )-> GetState ( ) != TS_DEAD &&
				nSleepTime < 3 * 60 * 1000  ) // 3 minutes of waiting
		{
			errContext = stErrContextTemp. GetContext ( );
			if ( errContext != 0 )
				errContext-> Flush ( );

			VTask::GetCurrent ( )-> Sleep ( nSleepTime );
			vtcpEndPointRestored = fClientSession-> Restore ( *this, vError );
			if ( vError != VE_OK || vtcpEndPointRestored == 0 )
			{
				sLONG	nAdditionalSleep = ( VSystem::Random ( ) % 60 ) * 1000;
				if ( nAdditionalSleep < 5000 )
					nAdditionalSleep += 5000;

				nSleepTime += nAdditionalSleep;
			}
		}

		if ( vError == VE_OK )
		{
			errContext = stErrContextTemp. GetContext ( );
			if ( errContext != 0 )
				errContext-> Flush ( );
		}
		else
		{
			stErrContextTemp. MergeAndFlush ( );
		}
	}
#endif
				if ( vError == VE_OK )
				{
					VTCPSessionManager::DebugMessage ( CVSTR ( "Restored connection" ), this );

					XTCPSock*	ncEndPointRestored = vtcpEndPointRestored-> DetachSocket ( );
					vtcpEndPointRestored-> Release ( );
					AttachSocket ( ncEndPointRestored );

					vError = VTCPSessionManager::Get ( )-> Restore ( this );
					if ( vError == VE_OK )
					{
						fIsInUse = true;
						VTCPSessionManager::DebugMessage ( CVSTR ( "Restored end-point" ), this );
					}
					else
						VTCPSessionManager::DebugMessage ( CVSTR ( "Failed to restore end-point" ), this );
				}
				else
				{
					VTCPSessionManager::DebugMessage ( CVSTR ( "Failed to restore connection" ), this, vError );

					VError				verrRemove = VE_OK;
					//verrRemove = VTCPSessionManager::Get ( )-> Remove ( this );
					xbox_assert ( verrRemove == VE_OK );
				}
			}
			else
			{
				VTCPSessionManager::DebugMessage ( CVSTR ( "Postponed end-point not found" ), this );
				vError = ReportError( VE_SRVR_POSTPONED_SESSION_EXPIRED, true, false );
		}
	}
	}
	
	return vError;
}

VError VTCPEndPoint::UnUse ( )
{
	xbox_assert ( fWasUsedAtLeastOnce );
	xbox_assert ( fIsInAutoReconnect );
	xbox_assert ( fClientSession != 0 );

	VTask*					vtCurrent = VTask::GetCurrent ( );
	bool					bResetIdleStart = (vtCurrent-> GetKind ( ) != kServerNetSessionManagerTaskKind);
	if ( bResetIdleStart )
		VTime::Now ( fIdleStart );
	VError					vError = fUsageMutex. Unlock ( ) ? VE_OK : VE_SRVR_FAILED_TO_SYNC_LOCK;
	if ( vError == VE_OK )
		fIsInUse = false;

	return vError;
}

XTCPSock* VTCPEndPoint::DetachSocket ( )
{
	if ( fSock == 0 )
		return 0;

	XTCPSock* ncEndPointResult = fSock;
	fSock = 0;

	return ncEndPointResult;
}

void VTCPEndPoint::AttachSocket ( XTCPSock* inSock )
{
	xbox_assert ( inSock == 0 || ( inSock != 0 && fSock == 0 ) );

	fSock = inSock;
}

VError VTCPEndPoint::StartKeepAlive ( VTCPServerSession* inSession )
{
	if ( inSession == 0 )
		return ReportError( VE_SRVR_INVALID_PARAMETER, true, false);

	VError vError = VTCPSessionManager::Get ( )-> AddForKeepAlive ( this, inSession );

	return vError;
}

VError VTCPEndPoint::StopKeepAlive ( )
{
	VError vError = VTCPSessionManager::Get ( )-> RemoveFromKeepAlive ( *this );

	return vError;
}


bool VTCPEndPoint::HasUnreadData( uLONG inTimeoutMillis )
{
	return WaitForInput ( inTimeoutMillis );
}


bool VTCPEndPoint::WaitForInput ( uLONG inMsTimeout )
{
	//bool		bWasBlocking = fSock-> IsBlocking ( );
	//if ( !bWasBlocking )
	//	fSock-> SetBlocking ( true );

	struct timeval	tvTimeout;
	int				nRawSocket = GetRawSocket ( );
	fd_set			fdReadSocket;

	memset(&tvTimeout, 0, sizeof(tvTimeout));
	tvTimeout. tv_sec = inMsTimeout / 1000;
	tvTimeout. tv_usec = ( inMsTimeout % 1000 ) * 1000;

	FD_ZERO ( &fdReadSocket );
	FD_SET ( nRawSocket, &fdReadSocket );

	bool			bResult = ( select ( FD_SETSIZE, &fdReadSocket, 0, 0, &tvTimeout ) > 0 );
	//if ( !bWasBlocking )
	//	fSock-> SetBlocking ( false );

	return bResult;
}


VError VTCPEndPoint::ReportError(VError inErr, bool needThrow, bool isCritical)
{
	if(inErr!=VE_OK)
	{
		if(needThrow)
			ThrowNetError(inErr);
	
		if(isCritical)
			SignalCriticalError(inErr);
	}
	
	return inErr;
}


XBOX::VError VTCPEndPoint::SetNoDelay (bool inYesNo)
{
	if (!fSock)
		return ReportError(VE_SRVR_NULL_ENDPOINT);

	return fSock->SetNoDelay(inYesNo);
}


XBOX::VError VTCPEndPoint::PromoteToSSL(bool inDoHandshake, XBOX::VKeyCertChain* inClientKeyCertChain)
{
	if(!fSock)
		return ReportError(VE_SRVR_NULL_ENDPOINT);

	xbox_assert(!fIsSSL);
    
    VError verr=VE_OK;
    
    //Client specific stuff (server SSL stuff is done in listener)
    
    verr=fSock->PromoteToSSL(inClientKeyCertChain);
    
	if(verr==XBOX::VE_OK && inDoHandshake)
		verr=fSock->GetSSLDelegate()->HandShake();

	if(verr==VE_OK)
		fIsSSL=true;

	return verr;
}


XBOX::VError VTCPEndPoint::PromoteToSSL(bool inDoHandshake, const XBOX::VMemoryBuffer<>& inCertBuffer, const XBOX::VMemoryBuffer<>& inKeyBuffer)
{
	if (!fSock)
		return ReportError(VE_SRVR_NULL_ENDPOINT);

	xbox_assert(!fIsSSL);

	XBOX::VKeyCertChain *keyCertChain = XBOX::SslFramework::RetainKeyCertChain(inKeyBuffer, inCertBuffer);

    VError verr=PromoteToSSL(inDoHandshake, keyCertChain);

	XBOX::SslFramework::ReleaseKeyCertChain(&keyCertChain);

	return verr;
}


XBOX::VError VTCPEndPoint::DirectSocketRead(void *outBuff, uLONG *ioLen)
{
	if (!fSock)
		return ReportError(VE_SRVR_NULL_ENDPOINT);
	
	return fSock->Read((char *) outBuff, ioLen);
}


XBOX::VError VTCPEndPoint::DirectSocketRead (void *outBuff, uLONG *ioLen, sLONG inTimeOut)
{
	if (!fSock)
		return ReportError(VE_SRVR_NULL_ENDPOINT);

	return fSock->ReadWithTimeout((char *) outBuff, ioLen, inTimeOut);
}


VError VTCPEndPoint::ReadWithTimeout(void *outBuff, uLONG *ioLen, sLONG inMsTimeout)
{
	ILogger* logger=VProcess::Get()->GetLogger();
	
	bool shouldTrace=(logger!=NULL) ? logger->ShouldLog(EML_Trace) : false;	// ODBCDriver use-case: Logger can be null.
	
	VValueBag* tBag=NULL;
	
	if(shouldTrace)
		tBag=new VValueBag;
	
	shouldTrace&=(tBag!=NULL);
	
	if(shouldTrace)
	{
		ILoggerBagKeys::level.Set(tBag, EML_Trace);
		
		ILoggerBagKeys::component_signature.Set(tBag, kSERVER_NET_SIGNATURE);
		
		ILoggerBagKeys::task_id.Set(tBag, VTask::GetCurrentID());
		
		ILoggerBagKeys::message.Set(tBag, CVSTR("VTCPEndPoint::Read"));
		
		ILoggerBagKeys::local_addr.Set(tBag, GetLocalIP(this));
		
		ILoggerBagKeys::peer_addr.Set(tBag, GetPeerIP(this));
		
		ILoggerBagKeys::local_port.Set(tBag, GetLocalPort(this));
		
		ILoggerBagKeys::peer_port.Set(tBag, GetPeerPort(this));

		ILoggerBagKeys::count_bytes_asked.Set(tBag, (ioLen!=NULL ? *ioLen : -1));
		
		ILoggerBagKeys::socket.Set(tBag, GetRawSocket());
		
		ILoggerBagKeys::is_blocking.Set(tBag, IsBlocking());
		
		ILoggerBagKeys::is_select_io.Set(tBag, IsSelectIO());
		
		ILoggerBagKeys::is_ssl.Set(tBag, IsSSL());
		
		ILoggerBagKeys::ms_timeout.Set(tBag, inMsTimeout);
	}

	sLONG spentMs=-1;
	
	VError verr=DoRead(outBuff, ioLen, inMsTimeout, &spentMs);

	if(shouldTrace)
	{
		ILoggerBagKeys::count_bytes_received.Set(tBag, (ioLen!=NULL ? *ioLen : -1));
		
		if(inMsTimeout>0)
			ILoggerBagKeys::ms_spent.Set(tBag, spentMs);
		
		ILoggerBagKeys::error_code.Set(tBag, verr);
		
		logger->LogBag(tBag);
	}
	
	if(tBag!=NULL)
		ReleaseRefCountable(&tBag);
	
	bool shouldDump=(logger!=NULL) ? logger->ShouldLog(EML_Dump) : false;
	
	if(shouldDump)
	{			
		sLONG offset=0;

		while(offset<(ioLen!=NULL ? *ioLen : -1))
		{
			VValueBag* dBag=new VValueBag;
			
			if(dBag==NULL)
				break;

			ILoggerBagKeys::level.Set(dBag, EML_Dump);

			ILoggerBagKeys::component_signature.Set(dBag, kSERVER_NET_SIGNATURE);
				
			ILoggerBagKeys::task_id.Set(dBag, VTask::GetCurrentID());
				
			ILoggerBagKeys::message.Set(dBag, CVSTR("VTCPEndPoint::Read"));
				
			offset=ServerNetTools::FillDumpBag(dBag, outBuff, (ioLen!=NULL ? *ioLen : -1), offset);
				
			logger->LogBag(dBag);

			ReleaseRefCountable(&dBag);
		}
	}

	return verr;
}


VError VTCPEndPoint::ReadExactly(void *outBuff, uLONG inLen, sLONG inMsTimeout)
{
	ILogger* logger=VProcess::Get()->GetLogger();
	
	bool shouldTrace=(logger!=NULL) ? logger->ShouldLog(EML_Trace) : false;
	
	VValueBag* tBag=NULL;
	
	if(shouldTrace)
		tBag=new VValueBag;
	
	shouldTrace&=(tBag!=NULL);
	
	if(shouldTrace)
	{
		ILoggerBagKeys::level.Set(tBag, EML_Trace);
		
		ILoggerBagKeys::component_signature.Set(tBag, kSERVER_NET_SIGNATURE);
		
		ILoggerBagKeys::task_id.Set(tBag, VTask::GetCurrentID());
		
		ILoggerBagKeys::message.Set(tBag, CVSTR("VTCPEndPoint::ReadExactly"));
		
		ILoggerBagKeys::local_addr.Set(tBag, GetLocalIP(this));

		ILoggerBagKeys::peer_addr.Set(tBag, GetPeerIP(this));
	
		ILoggerBagKeys::local_port.Set(tBag, GetLocalPort(this));

		ILoggerBagKeys::peer_port.Set(tBag, GetPeerPort(this));

		ILoggerBagKeys::count_bytes_asked.Set(tBag, inLen);
		
		ILoggerBagKeys::ms_timeout.Set(tBag, inMsTimeout);
		
		ILoggerBagKeys::socket.Set(tBag, GetRawSocket());
		
		ILoggerBagKeys::is_blocking.Set(tBag, IsBlocking());
		
		ILoggerBagKeys::is_select_io.Set(tBag, IsSelectIO());
		
		ILoggerBagKeys::is_ssl.Set(tBag, IsSSL());
	}
	
	//jmo - Non blocking and timeout zero really means blocking.
	//      If not Select IO, silently tweak the socket to reflect that.
	
	bool wasBlocking = fSock!=NULL ? fSock->IsBlocking() : true;
	
	if(inMsTimeout<=0 && !wasBlocking && !IsSelectIO())
		fSock->SetBlocking(true);
	
	
	uLONG partialReadLen=inLen;
	
	VError verr=DoReadExactly(outBuff, &partialReadLen, inMsTimeout);
	
	
	if(fSock!=NULL && fSock->IsBlocking()!=wasBlocking)
		fSock->SetBlocking(wasBlocking);
	
	if(shouldTrace)
	{		
		ILoggerBagKeys::error_code.Set(tBag, verr);
		
		ILoggerBagKeys::count_bytes_received.Set(tBag, partialReadLen);
		
		logger->LogBag(tBag);
	}
	
	if(tBag!=NULL)
		ReleaseRefCountable(&tBag);
		
	bool shouldDump=(logger!=NULL) ? logger->ShouldLog(EML_Dump) : false;

	if(shouldDump)
	{
		sLONG offset=0;

		while(offset<partialReadLen)
		{
			VValueBag* dBag=new VValueBag;
		
			if(dBag==NULL)
				break;

			ILoggerBagKeys::level.Set(dBag, EML_Dump);
			
			ILoggerBagKeys::component_signature.Set(dBag, kSERVER_NET_SIGNATURE);
				
			ILoggerBagKeys::task_id.Set(dBag, VTask::GetCurrentID());
				
			ILoggerBagKeys::message.Set(dBag, CVSTR("ReadExactly"));

			offset=ServerNetTools::FillDumpBag(dBag, outBuff, partialReadLen, offset);

			logger->LogBag(dBag);

			ReleaseRefCountable(&dBag);
		}
	}

	return verr;
}


VError VTCPEndPoint::WriteWithTimeout(void *inBuff, uLONG *ioLen, sLONG inMsTimeout, bool inWithEmptyTail)
{
	ILogger* logger=VProcess::Get()->GetLogger();
	
	bool shouldTrace=(logger!=NULL) ? logger->ShouldLog(EML_Trace) : false;
	
	VValueBag* tBag=NULL;
	
	if(shouldTrace)
		tBag=new VValueBag;
	
	shouldTrace&=(tBag!=NULL);
	
	if(shouldTrace)
	{
		ILoggerBagKeys::level.Set(tBag, EML_Trace);
		
		ILoggerBagKeys::component_signature.Set(tBag, kSERVER_NET_SIGNATURE);
		
		ILoggerBagKeys::task_id.Set(tBag, VTask::GetCurrentID());
		
		ILoggerBagKeys::message.Set(tBag, CVSTR("VTCPEndPoint::Write"));
		
		ILoggerBagKeys::local_addr.Set(tBag, GetLocalIP(this));
		
		ILoggerBagKeys::peer_addr.Set(tBag, GetPeerIP(this));

		ILoggerBagKeys::local_port.Set(tBag, GetLocalPort(this));
		
		ILoggerBagKeys::peer_port.Set(tBag, GetPeerPort(this));
		
		ILoggerBagKeys::count_bytes_asked.Set(tBag, (ioLen!=NULL ? *ioLen : -1));
		
		ILoggerBagKeys::socket.Set(tBag, GetRawSocket());
		
		ILoggerBagKeys::is_blocking.Set(tBag, IsBlocking());
		
		ILoggerBagKeys::is_select_io.Set(tBag, IsSelectIO());
		
		ILoggerBagKeys::is_ssl.Set(tBag, IsSSL());
		
		ILoggerBagKeys::ms_timeout.Set(tBag, inMsTimeout);
	}
	
	sLONG spentMs=-1;
	
	VError verr=DoWrite(inBuff, ioLen, inMsTimeout, &spentMs, inWithEmptyTail);
		
	if(shouldTrace)
	{
		ILoggerBagKeys::count_bytes_sent.Set(tBag, (ioLen!=NULL ? *ioLen : -1));
		
		if(inMsTimeout>0)
			ILoggerBagKeys::ms_spent.Set(tBag, spentMs);
		
		ILoggerBagKeys::error_code.Set(tBag, verr);
		
		logger->LogBag(tBag);
	}
	
	if(tBag!=NULL)
		ReleaseRefCountable(&tBag);
	
	bool shouldDump=(logger!=NULL) ? logger->ShouldLog(EML_Dump) : false;
	
	if(shouldDump)
	{
		sLONG offset=0;
		
		while(offset<(ioLen!=NULL ? *ioLen : -1))
		{
			VValueBag* dBag=new VValueBag;

			if(dBag==NULL)
				break;

			ILoggerBagKeys::level.Set(dBag, EML_Dump);
			
			ILoggerBagKeys::component_signature.Set(dBag, kSERVER_NET_SIGNATURE);
				
			ILoggerBagKeys::task_id.Set(dBag, VTask::GetCurrentID());
				
			ILoggerBagKeys::message.Set(dBag, CVSTR("VTCPEndPoint::Write"));
				
			offset=ServerNetTools::FillDumpBag(dBag, inBuff, (ioLen!=NULL ? *ioLen : -1), offset);
				
			logger->LogBag(dBag);

			ReleaseRefCountable(&dBag);
		}
	}
	
	return verr;
}


VError VTCPEndPoint::WriteExactly(const void *inBuff, uLONG inLen, sLONG inMsTimeout)
{
	ILogger* logger=VProcess::Get()->GetLogger();
	
	bool shouldTrace=(logger!=NULL) ? logger->ShouldLog(EML_Trace) : false;
	
	VValueBag* tBag=NULL;
	
	if(shouldTrace)
		tBag=new VValueBag;
	
	shouldTrace&=(tBag!=NULL);
	
	if(shouldTrace)
	{
		ILoggerBagKeys::level.Set(tBag, EML_Trace);
		
		ILoggerBagKeys::component_signature.Set(tBag, kSERVER_NET_SIGNATURE);
		
		ILoggerBagKeys::task_id.Set(tBag, VTask::GetCurrentID());
		
		ILoggerBagKeys::message.Set(tBag, CVSTR("VTCPEndPoint::WriteExactly"));
		
		ILoggerBagKeys::local_addr.Set(tBag, GetLocalIP(this));
		
		ILoggerBagKeys::peer_addr.Set(tBag, GetPeerIP(this));

		ILoggerBagKeys::local_port.Set(tBag, GetLocalPort(this));
		
		ILoggerBagKeys::peer_port.Set(tBag, GetPeerPort(this));
		
		ILoggerBagKeys::count_bytes_asked.Set(tBag, inLen);
		
		ILoggerBagKeys::ms_timeout.Set(tBag, inMsTimeout);
		
		ILoggerBagKeys::socket.Set(tBag, GetRawSocket());
		
		ILoggerBagKeys::is_blocking.Set(tBag, IsBlocking());
		
		ILoggerBagKeys::is_select_io.Set(tBag, IsSelectIO());
		
		ILoggerBagKeys::is_ssl.Set(tBag, IsSSL());
	}
	
	//jmo - Non blocking and timeout zero really means blocking.
	//      Silently tweak the socket to reflect that (no SelectIO at writing).
	
	bool wasBlocking = fSock!=NULL ? fSock->IsBlocking() : true;
	
	if(inMsTimeout<=0 && !wasBlocking)
		fSock->SetBlocking(true);
	
	
	uLONG partialWriteLen=inLen;
	
	VError verr=DoWriteExactly(inBuff, &partialWriteLen, inMsTimeout);
	
	
	if(fSock!=NULL && fSock->IsBlocking()!=wasBlocking)
		fSock->SetBlocking(wasBlocking);
	
	if(shouldTrace)
	{		
		ILoggerBagKeys::error_code.Set(tBag, verr);
		
		ILoggerBagKeys::count_bytes_sent.Set(tBag, partialWriteLen);
		
		logger->LogBag(tBag);
	}
	
	if(tBag!=NULL)
		ReleaseRefCountable(&tBag);
	
	bool shouldDump=(logger!=NULL) ? logger->ShouldLog(EML_Dump) : false;
	
	if(shouldDump)
	{
		sLONG offset=0;
		
		while(offset<partialWriteLen)
		{
			VValueBag* dBag=new VValueBag;

			if(dBag==NULL)
				break;

			ILoggerBagKeys::level.Set(dBag, EML_Dump);

			ILoggerBagKeys::component_signature.Set(dBag, kSERVER_NET_SIGNATURE);

			ILoggerBagKeys::task_id.Set(dBag, VTask::GetCurrentID());
				
			ILoggerBagKeys::message.Set(dBag, CVSTR("VTCPEndPoint::WriteExactly"));

			offset=ServerNetTools::FillDumpBag(dBag, inBuff, partialWriteLen, offset);

			logger->LogBag(dBag);
			
			ReleaseRefCountable(&dBag);
		}
	}
	
	return verr;
}


VError VTCPEndPoint::NotifyReadWithMessage(VMessage* inMsg)
{
	if(fIsWatching)
		return VE_SRVR_SOCKET_ALREADY_WATCHING;		

	ILogger* logger=VProcess::Get()->GetLogger();

	bool shouldTrace=(logger!=NULL) ? logger->ShouldLog(EML_Trace) : false;

	VValueBag* tBag=NULL;

	if(shouldTrace)
		tBag=new VValueBag;

	shouldTrace&=(tBag!=NULL);

	if(shouldTrace)
	{
			//jmo TODO - Mettre qq logs appropriés...
	}

	std::pair<VMessage*, IMessageable*>* p=new std::pair<VMessage*, IMessageable*>;
	xbox_assert(p!=NULL);

	if(p==NULL)
		return VE_MEMORY_FULL;

	p->first=inMsg;
	p->second=VTask::GetCurrent();

	RetainRefCountable(p->first);
	//RetainRefCountable(p->second);

	SetIsSelectIO(false);
	SetIsBlocking(true);

	VError verr=SetReadCallback(DoNotifyReadCallback, p);

	if(shouldTrace)
	{
		ILoggerBagKeys::error_code.Set(tBag, verr);

		logger->LogBag(tBag);
	}

	if(tBag!=NULL)
		ReleaseRefCountable(&tBag);
	
	return verr;
}


VError VTCPEndPoint::Close()
{
	ILogger* logger=VProcess::Get()->GetLogger();
	
	bool shouldTrace=(logger!=NULL) ? logger->ShouldLog(EML_Trace) : false;
	
	VValueBag* tBag=NULL;
	
	if(shouldTrace)
		tBag=new VValueBag;
	
	shouldTrace&=(tBag!=NULL);
	
	if(shouldTrace)
	{
		ILoggerBagKeys::level.Set(tBag, EML_Trace);
		
		ILoggerBagKeys::message.Set(tBag, CVSTR("VTCPEndPoint::Close"));
		
		ILoggerBagKeys::component_signature.Set(tBag, kSERVER_NET_SIGNATURE);
		
		ILoggerBagKeys::task_id.Set(tBag, VTask::GetCurrentID());
		
		ILoggerBagKeys::socket.Set(tBag, GetRawSocket());
		
		ILoggerBagKeys::is_blocking.Set(tBag, IsBlocking());
		
		ILoggerBagKeys::is_select_io.Set(tBag, IsSelectIO());
		
		ILoggerBagKeys::is_ssl.Set(tBag, IsSSL());
		
	}
	
	VError verr=DoClose();
	
	if(shouldTrace)
	{
		ILoggerBagKeys::error_code.Set(tBag, verr);
		
		logger->LogBag(tBag);
	}
	
	if(tBag!=NULL)
		ReleaseRefCountable(&tBag);

	return verr;
}


VError VTCPEndPoint::ForceClose()
{
	ILogger* logger=VProcess::Get()->GetLogger();
	
	bool shouldTrace=(logger!=NULL) ? logger->ShouldLog(EML_Trace) : false;
	
	VValueBag* tBag=NULL;
	
	if(shouldTrace)
		tBag=new VValueBag;
	
	shouldTrace&=(tBag!=NULL);
	
	if(shouldTrace)
	{
		ILoggerBagKeys::level.Set(tBag, EML_Trace);
		
		ILoggerBagKeys::component_signature.Set(tBag, kSERVER_NET_SIGNATURE);
		
		ILoggerBagKeys::task_id.Set(tBag, VTask::GetCurrentID());
		
		ILoggerBagKeys::message.Set(tBag, CVSTR("VTCPEndPoint::ForceClose"));
		
		ILoggerBagKeys::socket.Set(tBag, GetRawSocket());
		
		ILoggerBagKeys::is_blocking.Set(tBag, IsBlocking());
		
		ILoggerBagKeys::is_select_io.Set(tBag, IsSelectIO());
		
		ILoggerBagKeys::is_ssl.Set(tBag, IsSSL());
		
	}
	
	VError verr=DoForceClose();
	
	if(shouldTrace)
	{
		ILoggerBagKeys::error_code.Set(tBag, verr);
		
		logger->LogBag(tBag);
	}
	
	if(tBag!=NULL)
		ReleaseRefCountable(&tBag);

	return verr;
}


VSslDelegate* VTCPEndPoint::GetSSLDelegate()
{
	return fSock->GetSSLDelegate();
};


VTCPEndPoint* VTCPEndPointFactory::DoCreateClientConnection (
															 VString const & inDNSNameOrIP,
															 PortNumber inPort,
															 bool inIsSSL,
															 bool inIsBlocking,
															 sLONG inMsTimeout,
															 VTCPSelectIOPool* inSelectIOPool,
															 VError& outError )
{
	outError = VE_OK;
	
	XTCPSock* xsock = XTCPSock::NewClientConnectedSock ( inDNSNameOrIP, inPort, inMsTimeout );
	
	if ( !xsock )
	{
		outError = ThrowNetError ( VE_SRVR_FAILED_TO_CREATE_CONNECTED_SOCKET );		
		return NULL;
	}
	
	if(outError==VE_OK && inIsSSL)
	{
        
        VKeyCertChain* kcc=ServerNetTools::RetainClientKeyCertChain();
        
		VError verr=xsock->PromoteToSSL(kcc);
        
        SslFramework::ReleaseKeyCertChain(&kcc);
		
		if(verr!=VE_OK)
			outError=ThrowNetError(VE_SRVR_FAILED_TO_CREATE_CONNECTED_SOCKET);	
	}
	
	if(outError==VE_OK )
	{
		VError verr=xsock->SetBlocking(inIsBlocking);
		
		if(verr!=VE_OK)
			outError=ThrowNetError(VE_SRVR_FAILED_TO_CREATE_CONNECTED_SOCKET);
	}
	
	if(outError!=VE_OK)
	{
		xsock-> Close ( );
		delete xsock;
		xsock = NULL;
		
		return NULL;
	}
	
	VTCPEndPoint* endpResult = new VTCPEndPoint ( xsock, inSelectIOPool );
	
#if EXCHANGE_ENDPOINT_ID
	{
		sLONG id = 0;
		VError debug_error = endpResult->ReadExactly ( &id, sizeof ( sLONG ), 60 * 1000 );
		endpResult->SetID( id);
		xbox_assert ( debug_error == VE_OK );
	}
#endif
	
	return endpResult;
}


VTCPEndPoint* VTCPEndPointFactory::CreateClientConnection(VString const& inDNSNameOrIP, PortNumber inPort,
														  bool inIsSSL, bool inIsBlocking, sLONG inMsTimeout,
														  VTCPSelectIOPool* inSelectIOPool, VError& outError)
{
	ILogger* logger=VProcess::Get()->GetLogger();
	
	bool shouldTrace=(logger!=NULL) ? logger->ShouldLog(EML_Trace) : false;
	
	VValueBag* tBag=NULL;
	
	if(shouldTrace)
		tBag=new VValueBag;
	
	shouldTrace&=(tBag!=NULL);
	
	if(shouldTrace)
	{
		ILoggerBagKeys::level.Set(tBag, EML_Trace);
		
		ILoggerBagKeys::component_signature.Set(tBag, kSERVER_NET_SIGNATURE);
		
		ILoggerBagKeys::task_id.Set(tBag, VTask::GetCurrentID());
		
		ILoggerBagKeys::message.Set(tBag, CVSTR("VTCPEndPointFactory::CreateClientConnection"));
		
#if EXCHANGE_ENDPOINT_ID

		ILoggerBagKeys::exchange_id.Set(tBag, true);

#endif
	}
	
	VTCPEndPoint* ep=DoCreateClientConnection(inDNSNameOrIP, inPort, inIsSSL, inIsBlocking,
											  inMsTimeout, inSelectIOPool, outError);
	
	if(shouldTrace)
	{
		if(ep!=NULL)
		{
			ILoggerBagKeys::socket.Set(tBag, ep->GetRawSocket());
			
			ILoggerBagKeys::is_blocking.Set(tBag, ep->IsBlocking());
			
			ILoggerBagKeys::is_select_io.Set(tBag, ep->IsSelectIO());
			
			ILoggerBagKeys::is_ssl.Set(tBag, ep->IsSSL());
		}
		else
		{
			ILoggerBagKeys::socket.Set(tBag, kBAD_SOCKET);
		}
		
		ILoggerBagKeys::error_code.Set(tBag, outError);
		
		logger->LogBag(tBag);
	}
	
	if(tBag!=NULL)
		ReleaseRefCountable(&tBag);
	
	return ep;
	
}


void VTCPEndPointFactory::GenerateSessionID ( XBOX::VString& outSessionID )
{
	/*VString				vstrData;
	vstrData. AppendString ( inSalt1 );
	sLONG				nRandom = VSystem::Random ( );
	vstrData. AppendLong ( nRandom );
	vstrData. AppendLong ( VSystem::GetCurrentTicks ( ) );
	vstrData. AppendString ( inSalt2 );
	sWORD				szTime [ 7 ];
	VSystem::GetSystemUTCTime ( szTime );
	for ( short i = 0; i < 7; i++ )
		vstrData. AppendLong ( szTime [ i ] );

	VChecksumMD5::Encrypt( vstrData, false ); // Returns 16 characters
	VIndex				nLength = vstrData. GetLength ( );
	for ( VIndex j = 0; j < nLength; j++ )
		outSessionID. AppendLong ( vstrData. GetUniChar ( j + 1 ) );
		*/

	VUUID			vuuID ( true );
	vuuID. GetString ( outSessionID );
}


END_TOOLBOX_NAMESPACE

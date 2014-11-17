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

#include "SelectIO.h"

#include "Tools.h"
#include "VTCPEndPoint.h"

#include "VSslDelegate.h"


#if VERSIONMAC || VERSION_LINUX
	#include <sys/socket.h>
#else 
	#include <functional>
#endif


BEGIN_TOOLBOX_NAMESPACE


using namespace ServerNetTools;


VTCPSelectIOPool::VTCPSelectIOPool ( ) :
fHandlerList ( )
{
	
}

VTCPSelectIOPool::~VTCPSelectIOPool ( )
{
	Close ( );
}

CTCPSelectIOHandler* VTCPSelectIOPool::AddSocketForReading ( VEndPoint* inEndPoint, VError& outError )
{
	return _AddSocket(inEndPoint, NULL, NULL, outError);
}

CTCPSelectIOHandler *VTCPSelectIOPool::AddSocketForWatching (VEndPoint* inEndPoint, void *inData, CTCPSelectIOHandler::ReadCallback *inCallback, VError& outError)
{
	xbox_assert(inCallback != NULL);
	
	return _AddSocket(inEndPoint, inData, inCallback, outError);
}

CTCPSelectIOHandler	*VTCPSelectIOPool::_AddSocket (VEndPoint *inEndPoint, void *inData, CTCPSelectIOHandler::ReadCallback *inCallback, VError& outError)
{
	VTCPEndPoint*			vtcpEndPoint = dynamic_cast<VTCPEndPoint*> ( inEndPoint );
	if ( vtcpEndPoint == 0 )
	{
		ThrowNetError ( VE_SRVR_NULL_ENDPOINT );
		ThrowNetError ( VE_SRVR_INVALID_PARAMETER );
		
		return 0;
	}

	VSslDelegate* sslDelegate=vtcpEndPoint->GetSSLDelegate();				

	if ( !fHandlersLock. Lock ( ) )
	{
		outError = VE_SRVR_FAILED_TO_SYNC_LOCK;
		
		return 0;
	}
	
	CTCPSelectIOHandler*							sioHandler = 0;
	std::list<CTCPSelectIOHandler*>::iterator		iterHandler = fHandlerList. begin ( );
	while ( iterHandler != fHandlerList. end ( ) )
	{
		if (*iterHandler) { 
			
			if (inCallback == NULL) {
				
				if ((*iterHandler)->AddSocketForReading(vtcpEndPoint->GetRawSocket(), sslDelegate) == VE_OK) {
					
					sioHandler = *iterHandler;
					break;
					
				}
				
			} else {
				
				if ((*iterHandler)->AddSocketForWatching(vtcpEndPoint->GetRawSocket(), inEndPoint, inData, inCallback) == VE_OK) {
					
					sioHandler = *iterHandler;
					break;
					
				}
				
			}
			
		}
		iterHandler++;
	}
	
	if ( !sioHandler )
	{
		VTCPSelectIOHandler*		vioh = new VTCPSelectIOHandler ( );
		vioh-> Run ( );
		
		if (inCallback == NULL)
			
			outError = vioh->AddSocketForReading(vtcpEndPoint->GetRawSocket(), sslDelegate);
		
		else 
			
			outError = vioh->AddSocketForWatching(vtcpEndPoint->GetRawSocket(), inEndPoint, inData, inCallback);
		
		sioHandler = vioh;
		fHandlerList. push_back ( sioHandler );
	}
	
	if ( !fHandlersLock. Unlock ( ) )
		if ( outError == VE_OK )
			outError = VE_SRVR_FAILED_TO_SYNC_LOCK;
	
	return sioHandler;
}

VError VTCPSelectIOPool::Close ( )
{
	if ( !fHandlersLock. Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;
	
	std::list<CTCPSelectIOHandler*>::iterator		iterHandler = fHandlerList. begin ( );
	while ( iterHandler != fHandlerList. end ( ) )
	{
		if ( *iterHandler )
			( *iterHandler )-> Stop ( );
		iterHandler++;
	}
	
	fHandlerList. clear ( );
	
	if ( !fHandlersLock. Unlock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;
	
	return VE_OK;
}


VTCPSelectAction::VTCPSelectAction (Socket inSocket)
{
	fSock = inSocket;
	fExpirationTime = 0;
	fLastSocketError = 0;
	fLastSystemSocketError = 0;
	fLastError = VE_OK;
}

VTCPSelectAction::~VTCPSelectAction ()
{
}

bool VTCPSelectAction::Delete (VTCPSelectAction *vtcpSelectAction)
{
	if (vtcpSelectAction) { 

		delete vtcpSelectAction;
		return true;

	} else 

		return false;
}

VTCPSelectReadAction::VTCPSelectReadAction (Socket inSocket, char* inBuffer, uLONG* inBufferSize, VSslDelegate* inSSLDelegate)
: VTCPSelectAction(inSocket), fSyncEvtProcessed(),fProcessedLock()
{
	fIsProcessed = true;
	fBuffer = inBuffer;
	fBufferSize = inBufferSize;
	fSslDelegate=inSSLDelegate;
}

bool VTCPSelectReadAction::IsProcessed ()
{
	bool	bResult = false;

	fProcessedLock.Lock();
	bResult = fIsProcessed;
	fProcessedLock.Unlock();

	XBOX::VTask::GetCurrent()->YieldNow();

	return bResult;
}

void VTCPSelectReadAction::SetProcessed (bool bProcessed)
{
	fProcessedLock.Lock();

	fIsProcessed = bProcessed;
	if (!bProcessed)

		fSyncEvtProcessed.Reset();

	fProcessedLock.Unlock();
}

bool VTCPSelectReadAction::WaitForAction ( )
{
	return fSyncEvtProcessed.Lock();
}

bool VTCPSelectReadAction::NotifyActionComplete ( )
{
	SetProcessed(true);

	bool		bResult = fSyncEvtProcessed.Unlock();

	XBOX::VTask::GetCurrent ( )-> YieldNow ( );

	return bResult;
}

void VTCPSelectReadAction::DoAction (fd_set* fdSockets)
{
	int	nRawSocket	= GetRawSocket();

	if (!FD_ISSET(nRawSocket, fdSockets))

		return;

	if (IsProcessed())

		return;

#if VERSIONMAC || VERSION_LINUX
	ssize_t nRead=0;
#else
	int	nRead=0;
#endif
	
	if(fSslDelegate==NULL)
	{
		nRead = recv(nRawSocket, GetBuffer(), *GetFullBufferSize(), 0);
		
		DEBUG_CHECK_SOCK_RESULT( nRead, "recv", nRawSocket);
	}
	else
	{
		char* buf=GetBuffer();
		uLONG size=*GetFullBufferSize();

		VError verr=fSslDelegate->Read(buf, &size);

		if(verr==VE_OK && size>0)
			nRead=size;
		else if(verr==VE_SOCK_WOULD_BLOCK)
			return;	//Nothing to update, no notification
	}

	if ( nRead > 0 ) {

		UpdateFullBufferSize ( static_cast<uLONG>(nRead) );

	} else if ( nRead == 0 ) {

		UpdateFullBufferSize ( 0 );
		SetLastSocketError ( 0 );
		SetLastSystemSocketError ( VTCPSelectIOHandler::GetLastSocketError ( ) );
		SetLastError ( VE_SRVR_CONNECTION_BROKEN );

	} else {

		UpdateFullBufferSize ( 0 );
		
		//TODO : Put socket error in logs
		SetLastSocketError ( static_cast<sLONG>(nRead));
		SetLastSystemSocketError ( VTCPSelectIOHandler::GetLastSocketError ( ) );
		SetLastError ( VE_SRVR_READ_FAILED );
	
	}
	NotifyActionComplete ( );
}

void VTCPSelectReadAction::HandleError (fd_set* fdSockets)
{
	int	nRawSocket = GetRawSocket ( );

	if (!FD_ISSET ( nRawSocket, fdSockets ))
		return;

	if (IsProcessed())
	
		return;

	int				nError = 0;
#if VERSIONWIN
	int				nSize = sizeof ( nError );
#else
	socklen_t		nSize = sizeof ( nError );
#endif
	int				nResult = getsockopt ( nRawSocket, SOL_SOCKET, SO_ERROR, ( char* ) &nError, &nSize );

	DEBUG_CHECK_SOCK_RESULT( nResult, "getsockopt", nRawSocket);

	if ( nError != 0 )
	{	
		//TODO : Put socket error in logs
		SetLastSocketError ( -1 );
		SetLastSystemSocketError ( nError );
		SetLastError ( VE_SRVR_READ_FAILED );

		UpdateFullBufferSize(0);
		NotifyActionComplete();

	}
}

VTCPSelectWatchAction::VTCPSelectWatchAction (Socket inSocket, VEndPoint *inEndPoint, void *inData, CTCPSelectIOHandler::ReadCallback *inCallback)
: VTCPSelectAction(inSocket)
{
	xbox_assert(inEndPoint != NULL && inCallback != NULL);

	fEndPoint = inEndPoint;
	fData = inData;
	fCallback = inCallback;
}

bool VTCPSelectWatchAction::TriggerReadCallback (sLONG inErrorCode)
{
	return fCallback(GetRawSocket(), fEndPoint, fData, inErrorCode);
}

void VTCPSelectWatchAction::DoAction (fd_set* fdSockets)
{
	xbox_assert(GetType() == VTCPSelectAction::eTYPE_WATCH);

	if (!FD_ISSET(GetRawSocket(), fdSockets))

		return;
		
	else {		
	
		if (!TriggerReadCallback(0))

			SetLastError(VE_SRVR_READ_FAILED);	// May be not a failed read, but this will prevent select() to check this socket.

	}
}

void VTCPSelectWatchAction::HandleError (fd_set* fdSockets)
{
	int	nRawSocket = GetRawSocket();

	if (!FD_ISSET(nRawSocket, fdSockets))

		return;

	int				nError = 0;
#if VERSIONWIN
	int				nSize = sizeof ( nError );
#else
	socklen_t		nSize = sizeof ( nError );
#endif
	int				nResult = getsockopt ( nRawSocket, SOL_SOCKET, SO_ERROR, ( char* ) &nError, &nSize );

	DEBUG_CHECK_SOCK_RESULT( nResult, "getsockopt", nRawSocket);

	if ( nError != 0 ) {	

		//TODO : Put socket error in logs
		SetLastSocketError(-1);
		SetLastSystemSocketError (nError);
		SetLastError(VE_SRVR_READ_FAILED);

		TriggerReadCallback(nError);

	}
}

VTCPSelectIOHandler::VTCPSelectIOHandler ( ) :
									VTask ( NULL, 0, XBOX::eTaskStylePreemptive, NULL ),
									fReadSockLock ( )
{
	SetName ( "ServerNet select I/O handler" );
	fReadCount = 0;
}

VTCPSelectIOHandler::~VTCPSelectIOHandler ( )
{
	if ( !fReadSockLock. Lock ( ) )
		return;

	fReadSockList.clear();

	fReadSockLock. Unlock ( );
}

void VTCPSelectIOHandler::Stop ( )
{
	Kill ( );
}

void VTCPSelectIOHandler::AddToFDSet ( VTCPSelectAction* vtcpSelectAction, fd_set* fdSockets )
{
	if ( vtcpSelectAction-> GetLastError ( ) != VE_OK )
		return;

	if ( vtcpSelectAction-> TimeOutExpired ( ) )
	{
		vtcpSelectAction-> SetLastError ( VE_SRVR_READ_TIMED_OUT );
		if (vtcpSelectAction->GetType() == VTCPSelectAction::eTYPE_READ)

			((VTCPSelectReadAction *) vtcpSelectAction)->NotifyActionComplete();

	}
	else
	{
		xbox_assert( vtcpSelectAction-> GetRawSocket ( ) != -1);
		FD_SET ( vtcpSelectAction-> GetRawSocket ( ), fdSockets );
	}
}


Boolean VTCPSelectIOHandler::DoRun ( )
{	
	int							nSocketsReadyForRead;
	struct timeval				tvTimeout;
	while ( GetState ( ) != TS_DYING && GetState ( ) != TS_DEAD )
	{
		StDropErrorContext errCtx;
		
		FD_ZERO ( &fReadSockSet );

		fd_set	fdsErrors;
		FD_ZERO ( &fdsErrors );

		if ( !fReadSockLock. Lock ( ) )
			break;
		std::for_each (
					fReadSockList. begin ( ), fReadSockList. end ( ),
					std::bind2nd ( std::ptr_fun( AddToFDSet ), &fReadSockSet ) );
		std::for_each (
					fReadSockList. begin ( ), fReadSockList. end ( ),
					std::bind2nd ( std::ptr_fun( AddToFDSet ), &fdsErrors ) );

		int			nMaxSocket = 0;
		for( std::list<XBOX::VRefPtr<VTCPSelectAction> >::iterator i = fReadSockList.begin() ; i != fReadSockList.end() ; ++i)
		{
			int		nSocket = (*i)->GetRawSocket();
			if ( nMaxSocket < nSocket )
				nMaxSocket = nSocket;
		}
		
		if ( !fReadSockLock. Unlock ( ) )
			break;

		tvTimeout. tv_sec = 0;
		tvTimeout. tv_usec = 100000;

		if ( GetActiveReadCount ( ) == 0 )
		{
			Sleep ( 5 );
			nSocketsReadyForRead = 0;
		}
		else
		{
			nSocketsReadyForRead = select ( nMaxSocket + 1, &fReadSockSet, ( fd_set* ) 0, &fdsErrors, &tvTimeout );
			DEBUG_CHECK_RESULT( nSocketsReadyForRead, "select from IOHandler");
//#if WITH_DEBUGMSG && VERSIONWIN
			//if (WSAGetLastError() == WSAENOTSOCK)
			{
				fReadSockLock.Lock();
				XBOX::VString s( "sockets passed to select: ");
				bool first = true;
				for( std::list<XBOX::VRefPtr<VTCPSelectAction> >::iterator i = fReadSockList.begin() ; i != fReadSockList.end() ; ++i)
				{
					if (!first)
						s += (UniChar) ' ';
					first = false;
					s.AppendLong( (*i)->GetRawSocket());
				}
				s += (UniChar) '\n';
				fReadSockLock.Unlock();
				//XBOX::DebugMsg( s);
			}
//#endif
		}

		/*int			nSelectError = GetLastSocketError ( );
		if ( nSelectError != 0 )
			nSelectError = 1 + nSelectError;*/
		if ( nSocketsReadyForRead == 0 )
			continue;

		if ( !fReadSockLock. Lock ( ) )
			break;
		if ( nSocketsReadyForRead < 0 )
			std::for_each (
						fReadSockList. begin ( ), fReadSockList. end ( ),
						std::bind2nd ( std::ptr_fun( HandleError ), &fdsErrors ) );
		else
		{
			fReadCount++;
			/*if ( fReadCount % 100 == 0 )
			{
				VString			vstrMsg ( "Read count == " );
				vstrMsg. AppendLong8 ( fReadCount );
				vstrMsg. AppendCString ( "\n" );
				XBOX::DebugMsg ( vstrMsg );
			}*/

			std::for_each (
						fReadSockList. begin ( ), fReadSockList. end ( ),
						std::bind2nd ( std::ptr_fun( HandleRead ), &fReadSockSet ) );
		}

		if ( !fReadSockLock. Unlock ( ) )
			break;

		
		//Correction pour freeze des clients mono core : Corrige ce qui semble etre un pb d'ordonnancement sur les
		//machines ne disposant que d'un coeur. ACI0068696
		static sLONG cpuCount=VSystem::GetNumberOfProcessors();

		if(cpuCount==1)
			VTask::YieldNow();
		
	}

	/* TODO: Before exit, should notify all waiting threads and let them know that there will be no more reading/writing. */
	return true;
}

VError VTCPSelectIOHandler::AddSocketForReading ( Socket inRawSocket, VSslDelegate* inSSLDelegate )
{
	if ( !fReadSockLock. Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;

	xbox_assert( inRawSocket != -1);

	VError				vError = VE_OK;
	if ( std::find_if (
					fReadSockList. begin ( ),
					fReadSockList. end ( ),
					std::bind2nd ( std::ptr_fun( VTCPSelectAction::HasSameRawSocket ), inRawSocket ) ) == fReadSockList. end ( ) )
	{
		if ( GetActiveReadCount ( ) >= FD_SETSIZE )
			vError = VE_SRVR_TOO_MANY_SOCKETS_FOR_SELECT_IO;
		else
		{
			VTCPSelectAction*			vtcpAction = new VTCPSelectReadAction ( inRawSocket, 0, 0, inSSLDelegate );
			fReadSockList. push_back ( vtcpAction );
			ReleaseRefCountable( &vtcpAction);
		}
	}
	else
		vError = VE_SRVR_SOCKET_ALREADY_READING;

	if ( !fReadSockLock. Unlock ( ) )
		if ( vError == VE_OK )
			vError = VE_SRVR_FAILED_TO_SYNC_LOCK;

	return vError;
}

VError VTCPSelectIOHandler::RemoveSocketForReading ( Socket inRawSocket )
{
	if ( !fReadSockLock. Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;
	
	VError										vError = VE_OK;
	std::list<VRefPtr<VTCPSelectAction> >::iterator		iterAction = std::find_if (
													fReadSockList. begin ( ),
													fReadSockList. end ( ),
													std::bind2nd ( std::ptr_fun( VTCPSelectAction::HasSameRawSocket ), inRawSocket ) );
	if ( iterAction != fReadSockList. end ( ) )
	{
		if ( ( *iterAction ) != 0 ) {

			xbox_assert((*iterAction)->GetType() == VTCPSelectAction::eTYPE_READ);

			 // Socket may be removed for reading by another thread via ForceClose call.
			 // In this case I need to notify original reader that the read is over.

			
			((VTCPSelectReadAction *) (*iterAction).Get())->NotifyActionComplete();

		}

		fReadSockList. erase ( iterAction );
	}
	else
		vError = VE_SRVR_SOCKET_IS_NOT_READING;
	if ( !fReadSockLock. Unlock ( ) )
		vError = VE_SRVR_FAILED_TO_SYNC_LOCK;

	return vError;
}

VError VTCPSelectIOHandler::AddSocketForWatching (Socket inRawSocket, VEndPoint *inEndPoint, void *inData, CTCPSelectIOHandler::ReadCallback *inCallback)
{
	if (!fReadSockLock.Lock())

		return VE_SRVR_FAILED_TO_SYNC_LOCK;

	xbox_assert(inRawSocket != -1);

	VError												vError = VE_OK;
	std::list<VRefPtr<VTCPSelectAction> >::iterator		iterAction;

	iterAction = std::find_if(fReadSockList.begin(), fReadSockList.end(), std::bind2nd(std::ptr_fun(VTCPSelectAction::HasSameRawSocket), inRawSocket));

	if (iterAction == fReadSockList.end()) {

		if (GetActiveReadCount() >= FD_SETSIZE)

			vError = VE_SRVR_TOO_MANY_SOCKETS_FOR_SELECT_IO;

		else {

			VTCPSelectAction	*vtcpAction	= new VTCPSelectWatchAction(inRawSocket, inEndPoint, inData, inCallback);

			fReadSockList.push_back(vtcpAction);
			ReleaseRefCountable(&vtcpAction);

		}

	} else {

		xbox_assert((*iterAction)->GetType() == VTCPSelectAction::eTYPE_WATCH);
		vError = VE_SRVR_SOCKET_ALREADY_WATCHING;

	}

	if (!fReadSockLock.Unlock() && vError == VE_OK)

		vError = VE_SRVR_FAILED_TO_SYNC_LOCK;

	return vError;
}

VError VTCPSelectIOHandler::RemoveSocketForWatching (Socket inRawSocket)
{
	if (!fReadSockLock. Lock())

		return VE_SRVR_FAILED_TO_SYNC_LOCK;
	
	VError											vError = VE_OK;
	std::list<VRefPtr<VTCPSelectAction> >::iterator	iterAction;
	
	iterAction = std::find_if(fReadSockList.begin(), fReadSockList.end(), std::bind2nd(std::ptr_fun(VTCPSelectAction::HasSameRawSocket), inRawSocket));
	if (iterAction != fReadSockList.end()) {

		xbox_assert((*iterAction)->GetType() == VTCPSelectAction::eTYPE_WATCH);
		fReadSockList.erase(iterAction);

	} else

		vError = VE_SRVR_SOCKET_IS_NOT_READING;

	if (!fReadSockLock.Unlock())

		vError = VE_SRVR_FAILED_TO_SYNC_LOCK;

	return vError;
}

VError VTCPSelectIOHandler::Read ( Socket inRawSocket, char* inBuffer, uLONG* nBufferLength, sLONG& outError, sLONG& outSystemError, uLONG inTimeOutMillis )
{
	xbox_assert( inRawSocket != -1);

	if ( !fReadSockLock. Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;

	VError										vError = VE_OK;
	VRefPtr<VTCPSelectReadAction>						vtcpSelectReadAction;
	std::list<VRefPtr<VTCPSelectAction> >::iterator		iterAction = std::find_if (
														fReadSockList. begin ( ),
														fReadSockList. end ( ),
														std::bind2nd ( std::ptr_fun ( VTCPSelectAction::HasSameRawSocket ), inRawSocket ) );
	if ( iterAction == fReadSockList. end ( ) )
		vError = VE_SRVR_SOCKET_IS_NOT_READING;
	else
	{
		xbox_assert((*iterAction)->GetType() == VTCPSelectAction::eTYPE_READ);

		vtcpSelectReadAction = (VTCPSelectReadAction *) (*iterAction).Get();
		vtcpSelectReadAction-> SetBuffer ( inBuffer );
		vtcpSelectReadAction-> SetFullBufferSize ( nBufferLength );
		vtcpSelectReadAction-> SetProcessed ( false );
		vtcpSelectReadAction-> SetTimeOut ( inTimeOutMillis );
	}
	
	if ( !fReadSockLock. Unlock ( ) )
		vError = VE_SRVR_FAILED_TO_SYNC_LOCK;

	if ( vError != VE_OK )
		return vError;

	bool		bWait = vtcpSelectReadAction-> WaitForAction ( );
	if ( !bWait )
		vError = VE_SRVR_FAILED_TO_SYNC_LOCK;

	vError = vtcpSelectReadAction-> GetLastError ( );
	if ( vError != VE_OK )
	{
		outError = vtcpSelectReadAction-> GetLastSocketError ( );
		outSystemError = vtcpSelectReadAction-> GetLastSystemSocketError ( );
	}

	return vError;
}

void VTCPSelectIOHandler::HandleRead ( VTCPSelectAction* vtcpSelectAction, fd_set* fdSockets )
{
	vtcpSelectAction->DoAction(fdSockets);
}

void VTCPSelectIOHandler::HandleError ( VTCPSelectAction* vtcpSelectAction, fd_set* fdSockets )
{
	vtcpSelectAction->HandleError(fdSockets);
}

sLONG VTCPSelectIOHandler::GetLastSocketError ( )
{
	#if VERSIONWIN
		return WSAGetLastError ( );
	#else
		return 0;
	#endif
}

sLONG VTCPSelectIOHandler::GetActiveReadCount ( )
{
	if ( !fReadSockLock. Lock ( ) )
		return -1;

	int		nResult = 0;
	std::list<VRefPtr<VTCPSelectAction> >::iterator		iterAction = fReadSockList. begin ( );
	while ( iterAction != fReadSockList. end ( ) )
	{
		if ( ( *iterAction )-> GetLastError ( ) == VE_OK )
			nResult++;
		++iterAction;
	}

	if ( !fReadSockLock. Unlock ( ) )
		return -1;

	return nResult;
}

END_TOOLBOX_NAMESPACE

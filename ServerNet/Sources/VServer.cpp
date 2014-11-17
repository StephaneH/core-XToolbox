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

#include "VServer.h"

#include "IRequestLogger.h"
#include "SelectIO.h"
#include "Tools.h"
#include "VTCPEndPoint.h"
#include "VWorkerPool.h"


BEGIN_TOOLBOX_NAMESPACE


using namespace ServerNetTools;


VServer::VServer ( ) :
						m_vmtxListenerProtector ( ),
						m_vctrListeners ( )
{
	;
}

VServer::~VServer ( )
{
	;
}

VError VServer::AddConnectionListener ( IConnectionListener* inListener )
{
	if ( !m_vmtxListenerProtector. Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;

	VError			vError = VE_OK;

	if ( inListener )
	{
		m_vctrListeners. push_back ( inListener );
	}
	else
		vError = VE_INVALID_PARAMETER;

	m_vmtxListenerProtector. Unlock ( );

	return vError;
}

VError VServer::GetPublishedPorts ( std::vector<PortNumber>& outPorts )
{
	if ( !m_vmtxListenerProtector. Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;

	VError				vError = VE_OK;
	std::vector<IConnectionListener*>::iterator		iter = m_vctrListeners. begin ( );
	while ( iter != m_vctrListeners. end ( ) )
	{
		if ( ( *iter )-> IsListening ( ) )
		{
			vError = ( *iter )-> GetPorts ( outPorts );
			if ( vError != VE_OK )
				break;
		}

		iter++;
	}

	m_vmtxListenerProtector. Unlock ( );

	return vError;
}

VError VServer::Start ( )
{
	if ( !m_vmtxListenerProtector. Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;

	VError				vError = VE_OK;
	std::vector<IConnectionListener*>::iterator		iter = m_vctrListeners. begin ( );
	while ( iter != m_vctrListeners. end ( ) )
	{
		if ( ( vError = ( *iter )-> StartListening ( ) ) != VE_OK )
			break;
		iter++;
	}

	m_vmtxListenerProtector. Unlock ( );

	return vError;
}

bool VServer::IsRunning ( )
{
	if ( !m_vmtxListenerProtector. Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;

	bool											bIsRunning = false;

	/* Server "IsRunning" if at least one of its listeners is listening. */
	std::vector<IConnectionListener*>::iterator		iter = m_vctrListeners. begin ( );
	while ( iter != m_vctrListeners. end ( ) )
	{
		if ( ( *iter )-> IsListening ( ) )
		{
			bIsRunning = true;

			break;
		}

		iter++;
	}

	m_vmtxListenerProtector. Unlock ( );

	return bIsRunning;
}

VError VServer::Stop ( )
{
	if ( !m_vmtxListenerProtector. Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;

	std::vector<IConnectionListener*>::iterator		iter = m_vctrListeners. begin ( );
	while ( iter != m_vctrListeners. end ( ) )
	{
		( *iter )-> StopListening ( );
		/*( *iter )-> Release ( );*/
		iter++;
	}
	m_vctrListeners. clear ( );

	m_vmtxListenerProtector. Unlock ( );

	return VE_OK;
}


VTCPConnectionListener::VTCPConnectionListener ( IRequestLogger* inRequestLogger ) :
VTask ( NULL, 0, XBOX::eTaskStylePreemptive, NULL ),
fFactories ( ),
fCertificatePath ( ),
fKeyPath ( )
{
	fRequestLogger = inRequestLogger;
	fSockListener = NULL;
	fWorkerPool = NULL;
	fSelectIOPool = NULL;

	fCertificate.Clear();
	fKey.Clear();
	
	SetName ( "TCP connection listener" );
	SetKind ( kServerNetListenerTaskKind );
}

VTCPConnectionListener::~VTCPConnectionListener ( )
{
	;
}

VError VTCPConnectionListener::AddWorkerPool ( VWorkerPool* inPool )
{
	CopyRefCountable( &fWorkerPool, inPool);
	return VE_OK;
}

VError VTCPConnectionListener::AddSelectIOPool ( VTCPSelectIOPool* inPool )
{
	CopyRefCountable( &fSelectIOPool, inPool);
	return VE_OK;
}

void VTCPConnectionListener::SetSSLCertificatePaths (VFilePath const & inCertificatePath, VFilePath const & inKeyPath)
{
	fCertificatePath=inCertificatePath;
	fKeyPath=inKeyPath;
}

void VTCPConnectionListener::SetSSLKeyAndCertificate ( VString const & inCertificate, VString const &inKey)
{
	xbox_assert(fCertificatePath.IsEmpty() && fKeyPath.IsEmpty());

	fCertificate = inCertificate;
	fKey = inKey;
}

VError VTCPConnectionListener::StartListening ( )
{
	StTmpErrorContext errCtx;
	
	if ((fSockListener = new VSockListener(fRequestLogger)) == NULL) 

		return VE_MEMORY_FULL;

	VError	vError = VE_OK;

	if (!fCertificatePath.IsEmpty() && !fKeyPath.IsEmpty())
	{
		fSockListener-> SetCertificatePaths (fCertificatePath, fKeyPath);

	} else if (!fCertificate.IsEmpty() && !fKey.IsEmpty()) {

		vError = fSockListener-> SetKeyAndCertificate(fKey, fCertificate);

	}
	
	if (vError != VE_OK) {

		delete fSockListener;
		fSockListener = NULL;
		return vError;

	}

	VTCPConnectionHandlerFactory*								vtcpCHFactory = NULL;
	std::vector<PortNumber>											vctrPorts;
	std::vector<PortNumber>::iterator								iterPorts;
	std::vector<VTCPConnectionHandlerFactory*>::iterator		iterFactories = fFactories. begin ( );
	while ( iterFactories != fFactories. end ( ) )
	{
		vtcpCHFactory = *iterFactories;
		vError = vtcpCHFactory-> GetPorts ( vctrPorts );
		if ( vError != VE_OK )
			break;
		iterPorts = vctrPorts. begin ( );
		while ( iterPorts != vctrPorts. end ( ) )
		{
			fSockListener-> AddListeningPort ( vtcpCHFactory-> GetIP ( ), *iterPorts, vtcpCHFactory-> IsSSL ( ) );
			iterPorts++;
		}
		vctrPorts. clear ( );
		
		iterFactories++;
	}
	
	if ( vError == VE_OK )
	{
		if ( fSockListener-> StartListening ( ) )
			Run ( );
		else
			vError = ThrowNetError ( VE_SRVR_FAILED_TO_START_LISTENER );
	}
	
	if ( vError != VE_OK )
	{
		DeInit ( );
	}
	else
	{
		errCtx.Flush();
	}
	
	return vError;
}

bool VTCPConnectionListener::IsListening ( )
{
	return GetState ( ) != TS_DYING && GetState ( ) != TS_DEAD && fSockListener;
}

VError VTCPConnectionListener::StopListening ( )
{
	Kill ( );
	
	return VE_OK;
}

void VTCPConnectionListener::DeInit ( )
{
	if ( fSockListener )
	{
		fSockListener-> StopListeningAndClearPorts();
		delete fSockListener;
		fSockListener = NULL;
	}
	
	std::vector<VTCPConnectionHandlerFactory*>::iterator		iter = fFactories. begin ( );
	while ( iter != fFactories. end ( ) )
	{
		if (fWorkerPool)

			fWorkerPool-> StopConnectionHandlers ( ( *iter )-> GetType ( ) );

		( *iter )-> Release ( );
		iter++;
	}
	fFactories. clear ( );
	
	if ( fWorkerPool )
	{
		fWorkerPool-> Release ( );
		fWorkerPool = NULL;
	}
	if ( fSelectIOPool )
	{
		fSelectIOPool-> Release ( );
		fSelectIOPool = NULL;
	}
}

Boolean VTCPConnectionListener::DoRun ( )
{
	VError							vError = VE_OK;
	
	if ( fRequestLogger != 0 )
		fRequestLogger-> Log ( 'SRNT', 0, "SERVER_NET::VTCPConnectionListener::DoRun()::Enter", 1 );
	
	uLONG							nIdlePeriod = VSystem::GetCurrentTime ( );
	std::vector<PortNumber>				vctrPorts;
	std::vector<PortNumber>::iterator	iterPort;
	while ( GetState ( ) != TS_DYING && GetState ( ) != TS_DEAD && fSockListener )
	{
		StDropErrorContext errCtx;
		
		XTCPSock* xsock = fSockListener-> GetNewConnectedSocket(100 /*ms*/);
		if ( xsock )
		{
			if ( fRequestLogger != 0 )
				fRequestLogger-> Log ( 'SRNT', 0, "SERVER_NET::VTCPConnectionListener::DoRun()::NewConnectionAccepted", VSystem::GetCurrentTime ( ) );
			
			VTCPEndPoint*		vtcpEndPoint = new VTCPEndPoint ( xsock, fSelectIOPool );
#if EXCHANGE_ENDPOINT_ID
			static sLONG		nIDGenerator = 0;
			nIDGenerator++;
			vError = vtcpEndPoint-> WriteExactly ( &nIDGenerator, sizeof ( sLONG ), 60 * 1000 );
			vtcpEndPoint-> SetID ( nIDGenerator );
			xbox_assert ( vError == VE_OK );
#endif
			
			/* PLAN: Need to locate an appropriate factory, create new handler,
			 give it the end point and then transfer handler to the thread pool
			 for execution. */
			
			VTCPConnectionHandlerFactory*								vtcpCHFactory = NULL;
			std::vector<VTCPConnectionHandlerFactory*>::iterator		iter = fFactories. begin ( );
			while ( iter != fFactories. end ( ) )
			{
				vtcpCHFactory = *iter;
				vctrPorts. clear ( );
				vtcpCHFactory-> GetPorts ( vctrPorts );
				iterPort = std::find ( vctrPorts. begin ( ), vctrPorts. end ( ), xsock-> GetPort ( ) );
				if ( iterPort != vctrPorts. end ( ) )
					break;
				
				vtcpCHFactory = NULL;
				iter++;
			}
			if ( !vtcpCHFactory )
			{
				if ( fRequestLogger != 0 )
					fRequestLogger-> Log ( 'SRNT', 0, "SERVER_NET::VTCPConnectionListener::DoRun()::ERROR::CONNECTION FACTORY NOT FOUND", VSystem::GetCurrentTime ( ) );

				vtcpEndPoint-> Close ( );
				vtcpEndPoint-> Release ( );
				
				continue;
			}
			
			VConnectionHandler*						vcHandler = vtcpCHFactory-> CreateConnectionHandler ( vError );
			if ( vcHandler == 0 )
			{
				if ( fRequestLogger != 0 )
					fRequestLogger-> Log ( 'SRNT', 0, "SERVER_NET::VTCPConnectionListener::DoRun()::ERROR::FAILED TO CREATE CONNECTION HANDLER", VSystem::GetCurrentTime ( ) );

				vtcpEndPoint-> Close ( );
				vtcpEndPoint-> Release ( );
				
				continue;
			}
			
			vcHandler-> _ResetRedistributionCount ( );
			
			vcHandler-> SetEndPoint ( vtcpEndPoint );
			ReleaseRefCountable ( &vtcpEndPoint );
			
			/* Transfer vcHandler to the thread pool for execution. */
			if ( fWorkerPool )
				fWorkerPool-> AddConnectionHandler ( vcHandler );
			
			if ( fRequestLogger != 0 )
				fRequestLogger-> Log ( 'SRNT', 0, "SERVER_NET::VTCPConnectionListener::DoRun()::New connection is being handled", VSystem::GetCurrentTime ( ) );
		}
		else
		{
			if ( fRequestLogger != 0 && VSystem::GetCurrentTime ( ) - nIdlePeriod > 2000 )
			{
				nIdlePeriod = VSystem::GetCurrentTime ( );
			}
		}
	}
	
	DeInit ( );
	
	if ( fRequestLogger != 0 )
		fRequestLogger-> Log ( 'SRNT', 0, "SERVER_NET::VTCPConnectionListener::DoRun()::Exit", 1 );
	
	return false;
}

VError VTCPConnectionListener::AddConnectionHandlerFactory ( VConnectionHandlerFactory* inFactory )
{
	VTCPConnectionHandlerFactory*			vtcpCHFactory = dynamic_cast<VTCPConnectionHandlerFactory*>( inFactory );
	if ( !vtcpCHFactory )
		return VE_INVALID_PARAMETER;
	
	vtcpCHFactory-> Retain ( );
	/* TODO: Should discover and handle potential port conflicts. */
	fFactories. push_back ( vtcpCHFactory );
	
	return VE_OK;
}

VError VTCPConnectionListener::GetPorts ( std::vector<PortNumber>& outPorts )
{
	std::vector<VTCPConnectionHandlerFactory*>::iterator		iter = fFactories. begin ( );
	while ( iter != fFactories. end ( ) )
	{
		( *iter )-> GetPorts ( outPorts );
		
		iter++;
	}
	
	return VE_OK;
}


END_TOOLBOX_NAMESPACE


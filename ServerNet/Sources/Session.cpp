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

#include "Session.h"

#include "Tools.h"
#include "VTCPEndPoint.h"


BEGIN_TOOLBOX_NAMESPACE


using namespace ServerNetTools;


VTCPSessionManager						VTCPSessionManager::sInstance;
uLONG									VTCPSessionManager::sWorkerSleepDuration = 10000; // 10 seconds
VSyncEvent								VTCPSessionManager::sSyncEventForSleep;
std::vector<VTCPEndPoint*>				VTCPSessionManager::sEndPointsIdle;
std::vector<VTCPEndPoint*>				VTCPSessionManager::sEndPointsPostponed;
VCriticalSection						VTCPSessionManager::sEndPoints;
std::vector<VTCPServerSession*>			VTCPSessionManager::sServerSessions;
VCriticalSection						VTCPSessionManager::sServerSessionsMutex;
std::map<sLONG, std::pair< VTCPEndPoint*, VTCPServerSession* > >		VTCPSessionManager::sMapKeepAliveSessions;
VCriticalSection						VTCPSessionManager::sKeepAliveSessionsMutex;
uLONG									VTCPSessionManager::sKeepAliveTimeOut = 10000;


VTCPSessionManager::VTCPSessionManager ( )
{
	fWorkerTask = 0;
}

void VTCPSessionManager::DebugMessage ( XBOX::VString const & inMessage, VTCPEndPoint* inEndPoint, XBOX::VError inError )
{
	VString					vstrMessage ( "TCPSessionManager: " );
	vstrMessage. AppendString ( inMessage );
	vstrMessage. AppendCString ( " for socket ID = " );

	if ( inEndPoint == 0 )
		vstrMessage. AppendCString ( "NULL" );
	else
	{
		sLONG					nID = inEndPoint-> GetSimpleID ( );
		vstrMessage. AppendLong ( nID );
	}

	if ( inError != VE_OK )
	{
		vstrMessage. AppendCString ( " with error " );
		sLONG					nError = ERRCODE_FROM_VERROR ( inError );
		vstrMessage. AppendLong ( nError );

		vstrMessage. AppendCString ( " from component " );
		sLONG					nComponent = COMPONENT_FROM_VERROR ( inError );
		vstrMessage. AppendLong ( nComponent );
	}

	vstrMessage. AppendCString ( " at " );
	uLONG					nTime = VSystem::GetCurrentTime ( );
	vstrMessage. AppendLong ( nTime );

	vstrMessage. AppendCString ( "\r\n" );
	XBOX::DebugMsg ( vstrMessage );
}

sLONG VTCPSessionManager::Run ( VTask* vTask )
{
	if ( !vTask )
		return 0;
	
	VError											vError = VE_OK;
	
	std::vector<VTCPEndPoint*>::iterator			iterEndPoints;
	VTCPEndPoint*									vtcpEndPoint = 0;
	std::vector<VTCPServerSession*>::iterator		iterServerSessions;
	VTCPServerSession*								vtcpServerSession = 0;
	std::map<sLONG, std::pair< VTCPEndPoint*, VTCPServerSession* > >::iterator		iterKeepAlive;
	std::map<sLONG, std::pair< VTCPEndPoint*, VTCPServerSession* > >::iterator		iterKeepAliveTemp;
	while ( vTask-> GetState ( ) != TS_DYING && vTask-> GetState ( ) != TS_DEAD )
	{
		VTask::FlushErrors ( );
		
		sSyncEventForSleep. Lock ( sWorkerSleepDuration );
		sSyncEventForSleep. Reset ( );
		
		if ( vTask-> GetState ( ) == TS_DYING || vTask-> GetState ( ) == TS_DEAD )
			break;
		
		if ( !sEndPoints. Lock ( ) )
		{
			xbox_assert ( false );
			
			continue;
		}
		
		/* Look at idling end points */
		iterEndPoints = sEndPointsIdle. begin ( );
		while ( iterEndPoints != sEndPointsIdle. end ( ) )
		{
			vtcpEndPoint = *iterEndPoints;
			vError = HandleForIdleTimeOut ( vtcpEndPoint );
			xbox_assert ( vError == VE_OK );
			
			if ( vError == VE_OK )
			{
				if ( vtcpEndPoint-> IsPostponed ( ) )
				{
					//iterEndPoints = std::find ( sEndPointsIdle. begin ( ), sEndPointsIdle. end ( ), vtcpEndPoint );
					iterEndPoints = sEndPointsIdle. erase ( iterEndPoints );
					sEndPointsPostponed. push_back ( vtcpEndPoint );

					DebugMessage ( CVSTR ( "Moved from idle to postponed collection" ), vtcpEndPoint );
				}
				else
					iterEndPoints++;
			}
			else //if ( vError == VE_SRVR_CONNECTION_BROKEN || vError == VE_SRVR_READ_TIMED_OUT )
			{
				DebugMessage ( CVSTR ( "Failed to handle idle timeout" ), vtcpEndPoint, vError );

				iterEndPoints = sEndPointsIdle. erase ( iterEndPoints );
			}
		}
		
		/* Look at postponed end points */
		bool					bIsTimedOut = false;
		iterEndPoints = sEndPointsPostponed. begin ( );
		while ( iterEndPoints != sEndPointsPostponed. end ( ) )
		{
			vtcpEndPoint = *iterEndPoints;
			vError = HandleForPostponedTimeOut ( vtcpEndPoint, bIsTimedOut );
			xbox_assert ( vError == VE_OK );

			if ( vError != VE_OK )
				DebugMessage ( CVSTR ( "Failed to handle postponed timeout" ), vtcpEndPoint, vError );
			
			if ( vError == VE_OK && bIsTimedOut )
			{
				iterEndPoints++;//iterEndPoints = sEndPointsPostponed. erase ( iterEndPoints );
				DebugMessage ( CVSTR ( "Postponed expired" ), vtcpEndPoint, vError );
			}
			else
				iterEndPoints++;
		}
		
		if ( !sEndPoints. Unlock ( ) )
		{
			xbox_assert ( false );
			
			break;
		}
		
		/* Look at postponed server sessions */
		if ( !sServerSessionsMutex. Lock ( ) )
		{
			xbox_assert ( false );
			
			continue;
		}
		
		iterServerSessions = sServerSessions. begin ( );
		while ( iterServerSessions != sServerSessions. end ( ) )
		{
			vtcpServerSession = *iterServerSessions;
			if ( vtcpServerSession-> IsTimedOut ( ) )
			{
				iterServerSessions = sServerSessions. erase ( iterServerSessions );
				vtcpServerSession-> Release ( );
				vtcpServerSession = 0;
			}
			else
				iterServerSessions++;
		}
		
		if ( !sServerSessionsMutex. Unlock ( ) )
		{
			xbox_assert ( false );
			
			break;
		}
		
		/* Look at the keep-alive server sessions */
		if ( !sKeepAliveSessionsMutex. Lock ( ) )
		{
			xbox_assert ( false );
			
			continue;
		}
		
		uLONG				nNow = VSystem::GetCurrentTime ( );
		iterKeepAlive = sMapKeepAliveSessions. begin ( );
		while ( iterKeepAlive != sMapKeepAliveSessions. end ( ) )
		{
			vtcpServerSession = ( *iterKeepAlive ). second. second;
			vtcpEndPoint = ( *iterKeepAlive ). second. first;
			if ( vtcpServerSession != 0 && nNow - vtcpServerSession-> GetLastKeepAlive ( ) > vtcpServerSession-> GetKeepAliveInterval ( ) )
			{
				vError = HandleForKeepAlive ( vtcpEndPoint, vtcpServerSession );
				xbox_assert ( vError == VE_OK );
				if ( vError == VE_SRVR_CONNECTION_BROKEN )
				{
					iterKeepAliveTemp = iterKeepAlive;
					iterKeepAlive++;
					sMapKeepAliveSessions. erase ( iterKeepAliveTemp );
					
					continue;
				}
			}
			iterKeepAlive++;
		}
		
		if ( !sKeepAliveSessionsMutex. Unlock ( ) )
		{
			xbox_assert ( false );
			
			break;
		}
	} // end of while ( vTask-> GetState ( ) != TS_DYING && vTask-> GetState ( ) != TS_DEAD )
	
	return 0;
}

void VTCPSessionManager::Start ( )
{
	bool			bSynced = false;
	bSynced = fWorkerMutex. Lock ( );
	xbox_assert ( bSynced );
	
	if ( fWorkerTask == 0 )
	{
		fWorkerTask = new VTask ( this, 0, eTaskStylePreemptive, Run );
		
		fWorkerTask-> SetName( CVSTR( "TCP Session Manager" ));
		fWorkerTask-> SetKind ( kServerNetSessionManagerTaskKind );
		
		fWorkerTask-> Run ( );
	}
	
	bSynced = fWorkerMutex. Unlock ( );
	xbox_assert ( bSynced );
}


void VTCPSessionManager::Stop ( )
{
	ReleaseAllServerSessions ( );
	
	if ( fWorkerTask != 0 )
	{
		fWorkerTask-> Kill ( );
		sSyncEventForSleep. Unlock ( );
		fWorkerTask = 0;
	}
}

VError VTCPSessionManager::Add ( VTCPEndPoint* inEndPoint )
{
	xbox_assert ( inEndPoint != 0 );
	
	if ( !sEndPoints. Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;
	
	sEndPointsIdle. push_back ( inEndPoint );
	
	VError		vError = VE_OK;
	if ( !sEndPoints. Unlock ( ) )
		vError = VE_SRVR_FAILED_TO_SYNC_LOCK;
	
	if ( fWorkerTask == 0 )
		Start ( );
	
	return vError;
}

VError VTCPSessionManager::Remove ( VTCPEndPoint* inEndPoint )
{
	xbox_assert ( inEndPoint != 0 );
	
	if ( !sEndPoints. Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;
	
	std::vector<VTCPEndPoint*>::iterator		iter = std::find ( sEndPointsIdle. begin ( ), sEndPointsIdle. end ( ), inEndPoint );
	if ( iter != sEndPointsIdle. end ( ) )
		sEndPointsIdle. erase ( iter );
	else
	{
		iter = std::find ( sEndPointsPostponed. begin ( ), sEndPointsPostponed. end ( ), inEndPoint );
		if ( iter != sEndPointsPostponed. end ( ) )
			sEndPointsPostponed. erase ( iter );
	}
	
	VError		vError = VE_OK;
	if ( !sEndPoints. Unlock ( ) )
		vError = VE_SRVR_FAILED_TO_SYNC_LOCK;
	
	return vError;
}

bool VTCPSessionManager::IsPostponed ( VTCPEndPoint* inEndPoint )
{
	xbox_assert ( inEndPoint != 0 );
	
	bool				bResult = false;
	sEndPoints. Lock ( );
	std::vector<VTCPEndPoint*>::iterator		iter = std::find (
																  sEndPointsPostponed. begin ( ),
																  sEndPointsPostponed. end ( ),
																  inEndPoint );
	bResult = ( iter != sEndPointsPostponed. end ( ) );
	sEndPoints. Unlock ( );
	
	return bResult;
}

VError VTCPSessionManager::Restore ( VTCPEndPoint* inEndPoint )
{
	xbox_assert ( inEndPoint != 0 );
	
	VError											vError = VE_OK;
	
	if ( !sEndPoints. Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;
	
	std::vector<VTCPEndPoint*>::iterator		iter = std::find (
																  sEndPointsPostponed. begin ( ),
																  sEndPointsPostponed. end ( ),
																  inEndPoint );
	xbox_assert ( iter != sEndPointsPostponed. end ( ) );
	
	if ( iter != sEndPointsPostponed. end ( ) )
		sEndPointsPostponed. erase ( iter );
	
	sEndPointsIdle. push_back ( inEndPoint );
	
	if ( !sEndPoints. Unlock ( ) )
	{
		xbox_assert ( false );
		
		if ( vError == VE_OK )
			return VE_SRVR_FAILED_TO_SYNC_LOCK;
	}
	
	return vError;
}


VError VTCPSessionManager::StoreServerSession ( VTCPServerSession* inSession )
{
	xbox_assert ( inSession != 0 );
	
	if ( inSession == 0 )
		return ThrowNetError ( VE_SRVR_INVALID_PARAMETER );
	
	if ( !sServerSessionsMutex. Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;
	
	sServerSessions. push_back ( inSession );
	
	VError		vError = VE_OK;
	if ( !sServerSessionsMutex. Unlock ( ) )
		vError = VE_SRVR_FAILED_TO_SYNC_LOCK;
	
	if ( fWorkerTask == 0 )
		Start ( );
	
	return vError;
}

VTCPServerSession* VTCPSessionManager::RemoveServerSession ( XBOX::VString const & inSessionID, VError & outError )
{
	if ( !sServerSessionsMutex. Lock ( ) )
	{
		outError = VE_SRVR_FAILED_TO_SYNC_LOCK;
		
		return 0;
	}
	
	VTCPServerSession*								vtcpServerSession = 0;
	VString											vstrID;
	std::vector<VTCPServerSession*>::iterator		iter = sServerSessions. begin ( );
	while ( iter != sServerSessions. end ( ) )
	{
		vtcpServerSession = *iter;
		vstrID. Clear ( );
		vtcpServerSession-> GetID ( vstrID );
		if ( vstrID. EqualToString ( inSessionID ) )
		{
			sServerSessions. erase ( iter );
			
			break;
		}
		else
			vtcpServerSession = 0;
		
		iter++;
	}
	
	// Do not throw errors on the server, even if it is not in the ServerNet's handling thread
	outError = vtcpServerSession == 0 ? VE_SRVR_SESSION_NOT_FOUND : VE_OK;
	
	if ( !sServerSessionsMutex. Unlock ( ) )
		if ( outError == VE_OK )
			outError = VE_SRVR_FAILED_TO_SYNC_LOCK;
	
	return vtcpServerSession;
}

VError VTCPSessionManager::ReleaseServerSessions ( VUUID const & inClientUUID )
{
	VUUID					vNullUUID;
	if ( vNullUUID. EqualToSameKind ( &inClientUUID ) )
		return VE_OK;
	
	if ( !sServerSessionsMutex. Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;
	
	VError											vError = VE_OK;
	VTCPServerSession*								vtcpServerSession = 0;
	std::vector<VTCPServerSession*>::iterator		iter = sServerSessions. begin ( );
	while ( iter != sServerSessions. end ( ) )
	{
		vtcpServerSession = *iter;
		if ( vtcpServerSession-> HasSameClientUUID ( inClientUUID ) )
		{
			iter = sServerSessions. erase ( iter );
			vtcpServerSession-> Release ( );
			vtcpServerSession = 0;
		}
		else
			iter++;
	}
	
	if ( !sServerSessionsMutex. Unlock ( ) )
		if ( vError == VE_OK )
			vError = VE_SRVR_FAILED_TO_SYNC_LOCK;
	
	return vError;
}

VError VTCPSessionManager::ReleaseAllServerSessions ( )
{
	if ( !sServerSessionsMutex. Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;
	
	VError							vError = VE_OK;
	
	VTCPServerSession*								vtcpServerSession = 0;
	std::vector<VTCPServerSession*>::iterator		iter = sServerSessions. begin ( );
	while ( iter != sServerSessions. end ( ) )
	{
		vtcpServerSession = *iter;
		if ( vtcpServerSession != 0 )
			vtcpServerSession-> Release ( );
		
		iter++;
	}
	sServerSessions. clear ( );
	
	if ( !sServerSessionsMutex. Unlock ( ) )
		if ( vError == VE_OK )
			vError = VE_SRVR_FAILED_TO_SYNC_LOCK;
	
	return vError;
}

VError VTCPSessionManager::HandleForIdleTimeOut ( VTCPEndPoint* vtcpEndPoint )
{
	xbox_assert ( vtcpEndPoint != 0 );
	
	if ( vtcpEndPoint == 0 )
		return VE_SRVR_NULL_ENDPOINT; // No error throwing here - this is a stand-alone non-UI server thread.
	
	if ( vtcpEndPoint-> IsSSL ( ) )
		return VE_OK;
	
	if ( !vtcpEndPoint-> TryToUse ( ) )
		return VE_OK;
	
	VError						vError = VE_OK;
	if ( vtcpEndPoint-> IsIdleTimedOut ( ) )
	{
		VTCPClientSession*		tcpSession = vtcpEndPoint-> GetClientSession ( );
		if ( tcpSession == 0 )
		{
			vError = VE_SRVR_SESSION_NOT_FOUND; // No error throwing here - this is a stand-alone non-UI server thread.
			xbox_assert ( false );
		}
		else
		{
			// Very awkward code to quickly fix PPC deconnection issue.
			//sEndPoints. Unlock ( );
			vError = tcpSession-> Postpone ( *vtcpEndPoint );
			if ( vError == VE_OK )
				vError = vtcpEndPoint-> Postpone ( );
			//sEndPoints. Lock ( );
		}
	}
	
	VError		vUnUseError = vtcpEndPoint-> UnUse ( );
	if ( vError == VE_OK )
		vError = vUnUseError;
	
	return vError;
}

VError VTCPSessionManager::HandleForPostponedTimeOut ( VTCPEndPoint* vtcpEndPoint, bool& outTimedOut )
{
	xbox_assert ( vtcpEndPoint != 0 );
	
	outTimedOut = false;
	if ( vtcpEndPoint == 0 )
	{
		ThrowNetError ( VE_SRVR_NULL_ENDPOINT );
		
		return ThrowNetError ( VE_SRVR_INVALID_PARAMETER );
	}
	
	if ( !vtcpEndPoint-> TryToUse ( ) )
		return VE_OK;
	
	outTimedOut = vtcpEndPoint-> IsPostponeTimedOut ( );
	
	VError					vError = vtcpEndPoint-> UnUse ( );
	
	return vError;
}

VError VTCPSessionManager::HandleForKeepAlive ( VTCPEndPoint* vtcpEndPoint, VTCPServerSession* vtcpServerSession )
{
	if ( vtcpEndPoint == 0 )
		return VE_SRVR_NULL_ENDPOINT; // No error throwing here - this is a stand-alone non-UI server thread.
	
	if ( vtcpServerSession == 0 )
		return VE_SRVR_INVALID_PARAMETER; // No error throwing here - this is a stand-alone non-UI server thread.
	
	uLONG			nRequestLength = 0;
	void*			szRequest = vtcpServerSession-> GetKeepAliveRequest ( nRequestLength );
	if ( szRequest == 0 )
		return VE_OK;
	
	uLONG			nExpectedResponseLength = 0;
	void*			szExpectedResponse = vtcpServerSession-> GetKeepAliveResponse ( nExpectedResponseLength );
	if ( szExpectedResponse == 0 )
		return VE_OK;
	
	/* Switch to non-blocking non-select I/O mode. Keep previous options to be able to restore in the end. */
	bool			bWasBlocking = vtcpEndPoint-> IsBlocking ( );
	bool			bWasSelectIO = vtcpEndPoint-> IsSelectIO ( );
	if ( bWasSelectIO )
		vtcpEndPoint-> SetIsSelectIO ( false );
	if ( bWasBlocking )
		vtcpEndPoint-> SetIsBlocking ( false );
	
	VError			vError = vtcpEndPoint-> WriteExactly ( szRequest, nRequestLength, sKeepAliveTimeOut );
	if ( vError == VE_OK )
	{
		char*		szchResponse = new char [ nExpectedResponseLength ];
		vError = vtcpEndPoint-> ReadExactly ( szchResponse, nExpectedResponseLength, sKeepAliveTimeOut );
		if ( vError == VE_OK )
			if ( ::memcmp ( szchResponse, szExpectedResponse, nExpectedResponseLength ) != 0 )
				vError = VE_SRVR_KEEP_ALIVE_FAILED;
	}
	
	vtcpEndPoint-> SetIsBlocking ( bWasBlocking );
	vtcpEndPoint-> SetIsSelectIO ( bWasSelectIO );
	
	return vError;
}

VError VTCPSessionManager::AddForKeepAlive ( VTCPEndPoint* inEndPoint, VTCPServerSession* inSession )
{
	if ( inEndPoint == 0 || inSession == 0 )
		return ThrowNetError ( VE_SRVR_INVALID_PARAMETER );
	
	if ( !sKeepAliveSessionsMutex. Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;
	
	VError														vError = VE_OK;
	std::map<sLONG, std::pair< VTCPEndPoint*, VTCPServerSession* > >::iterator				iter = sMapKeepAliveSessions. find ( inEndPoint-> GetSimpleID ( ) );
	if ( iter == sMapKeepAliveSessions. end ( ) )
	{
		inSession-> Retain ( );
		inSession-> SetLastKeepAlive ( VSystem::GetCurrentTime ( ) );
		sMapKeepAliveSessions [ inEndPoint-> GetSimpleID ( ) ] = std::pair< VTCPEndPoint*, VTCPServerSession* > ( inEndPoint, inSession );
		
		if ( fWorkerTask == 0 )
			Start ( );
	}
	else
		vError = ThrowNetError ( VE_SRVR_INVALID_PARAMETER );
	
	if ( !sKeepAliveSessionsMutex. Unlock ( ) )
		if ( vError == VE_OK )
			vError = VE_SRVR_FAILED_TO_SYNC_LOCK;
	
	return vError;
}

VError VTCPSessionManager::RemoveFromKeepAlive ( VTCPEndPoint const & inEndPoint )
{
	if ( !sKeepAliveSessionsMutex. Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;
	
	VError														vError = VE_OK;
	VTCPServerSession*											tcpServerSession = 0;
	std::map<sLONG, std::pair< VTCPEndPoint*, VTCPServerSession* > >::iterator				iter = sMapKeepAliveSessions. find ( inEndPoint. GetSimpleID ( ) );
	if ( iter == sMapKeepAliveSessions. end ( ) )
		vError = ThrowNetError ( VE_SRVR_SESSION_NOT_FOUND );
	else
	{
		tcpServerSession = ( ( *iter ). second ). second;
		sMapKeepAliveSessions. erase ( iter );
		xbox_assert ( tcpServerSession != 0 );
		tcpServerSession-> Release ( );
	}
	
	if ( !sKeepAliveSessionsMutex. Unlock ( ) )
		if ( vError == VE_OK )
			vError = VE_SRVR_FAILED_TO_SYNC_LOCK;
	
	return vError;
}



VTCPServerSession::VTCPServerSession ( )
{
	fUuidClient. FromVUUID ( VUUID::sNullUUID );
	fTimeOut = 0;
	fLastKeepAlive = 0;
	fKeepAliveInterval = 30 * 1000; // 30 seconds by default
}

VError VTCPServerSession::Postpone ( )
{
	VTime::Now ( fPostponeStart );
	VError			vError = VTCPSessionManager::Get ( )-> StoreServerSession ( this );
	
	return vError;
}

bool VTCPServerSession::IsTimedOut ( )
{
	if ( fTimeOut == 0 )
		return false;
	
	VTime				vtNow;
	VTime::Now ( vtNow );
	bool				bResult = vtNow. GetMilliseconds ( ) - fPostponeStart. GetMilliseconds ( ) > fTimeOut;
	
	return bResult;
}

void* VTCPServerSession::GetKeepAliveRequest ( uLONG& outLength )
{
	outLength = 0;
	
	return 0;
}

void* VTCPServerSession::GetKeepAliveResponse ( uLONG& outLength )
{
	outLength = 0;
	
	return 0;
}


END_TOOLBOX_NAMESPACE
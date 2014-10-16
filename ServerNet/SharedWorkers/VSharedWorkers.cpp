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
#include "VSharedWorkers.h"


BEGIN_TOOLBOX_NAMESPACE


VSharedWorker::VSharedWorker ( ) :
VTask ( NULL, 0, XBOX::eTaskStylePreemptive, NULL ),
m_qNewConnectionHandlers ( ),
m_vctrConnectionHandlers ( )
{
	m_vcsCHQueueLock = new VCriticalSection ( );
	m_vcsConnectionHandlersProtector = new VCriticalSection ( );
	m_vsyncEventForNewHandlers = new VSyncEvent ( );
	m_vsyncEventForNewHandlers-> Reset ( );
	
	m_CurrentConnectionHandler = NULL;
	m_nLatestHandlingStart = 0;
	
	m_vcsxBusynessLock = new VCriticalSection ( );
	m_bIsHandling = false;
	m_nBusynessInterval = 10000; /* 10 seconds */
	m_nLatestBusynessIntervalStart = VSystem::GetCurrentTime ( );
	m_nBusynessDuration = 0;
	m_nLatestHandlingStart = 0;
	m_nPreviousBusyness = 0;
}

VSharedWorker::~VSharedWorker ( )
{
	if ( m_vcsCHQueueLock )
		delete m_vcsCHQueueLock;
	if ( m_vcsConnectionHandlersProtector )
		delete m_vcsConnectionHandlersProtector;
	if ( m_vsyncEventForNewHandlers )
		delete m_vsyncEventForNewHandlers;
	if ( m_vcsxBusynessLock )
		delete m_vcsxBusynessLock;
}

short VSharedWorker::GetBusyness ( )
{
	short				nResult = 0;
	
	if ( GetConnectionHandlerCount ( ) == 0 )
		return 0;
	
	m_vcsxBusynessLock-> Lock ( );
	/* Let's calculate how many milliseconds I was busy during the
	 previous activity measurement interval. */
	uLONG					nPreviousActivityDuration = ( m_nBusynessInterval * m_nPreviousBusyness ) / 100;
	/* The total activity duration during the previous interval and the current one so far. */
	uLONG					nTotalActivityDuration = nPreviousActivityDuration + m_nBusynessDuration;
	/* Add current handling time ( if handling something right now) */
	uLONG					nCurrentTime = VSystem::GetCurrentTime ( );
	if ( m_bIsHandling )
		nTotalActivityDuration += ( nCurrentTime >= m_nLatestHandlingStart ) ? nCurrentTime - m_nLatestHandlingStart : nCurrentTime;
	/* The length of the current activity interval so far. */
	uLONG					nCurrentIntervalSoFar = nCurrentTime - m_nLatestBusynessIntervalStart;
	/* Overall busyness over the previous and current activity intervals. */
	nResult = ( nTotalActivityDuration * 100 ) / ( m_nBusynessInterval + nCurrentIntervalSoFar );
	if ( nResult > 100 )
		nResult = 100;
	m_vcsxBusynessLock-> Unlock ( );
	
	return nResult;
}

void VSharedWorker::WakeUpFromIdling ( )
{
	m_vsyncEventForNewHandlers-> Unlock ( );
}

VError VSharedWorker::AddConnectionHandler ( VConnectionHandler* inConnectionHandler )
{
	VError				vError = VE_OK;
	
	if ( inConnectionHandler )
	{
		if ( m_vcsCHQueueLock-> Lock ( ) )
		{
			m_qNewConnectionHandlers. push  ( inConnectionHandler );
			m_vcsCHQueueLock-> Unlock ( );
			m_vsyncEventForNewHandlers-> Unlock ( );
		}
		else
			; /* TODO: Handle errors. */
	}
	else
		vError = VE_INVALID_PARAMETER;
	
	return vError;
}

VError VSharedWorker::AcceptNewConnectionHandlers ( )
{
	if ( !m_vcsCHQueueLock-> Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;
	
	/* NOTE: Should I do
	 while ( m_qNewConnectionHandlers. size ( ) > 0 )
	 instead? To accept all new connections first and only then proceed with 
	 command handling? */
	
	VError							vError = VE_OK;
	if ( m_qNewConnectionHandlers. size ( ) > 0 )
	{
		VConnectionHandler*			vcHandler = m_qNewConnectionHandlers. front ( );
		m_qNewConnectionHandlers. pop ( );
		
		if ( m_vcsConnectionHandlersProtector-> Lock ( ) )
		{
			m_vctrConnectionHandlers. push_back ( vcHandler );
			m_vcsConnectionHandlersProtector-> Unlock ( );
		}
		else
			vError = VE_SRVR_FAILED_TO_SYNC_LOCK;
	}
	
	m_vsyncEventForNewHandlers-> Reset ( );
	
	m_vcsCHQueueLock-> Unlock ( );
	
	return vError;
}

VError VSharedWorker::RunConnectionHandlers ( )
{
	uLONG8											nCHCount = 0;
	
	if ( m_vcsConnectionHandlersProtector-> Lock ( ) )
	{
		if ( m_vctrConnectionHandlers. size ( ) == 0 )
		{
			m_vcsxBusynessLock-> Lock ( );
			m_nLatestIdlingStart = VSystem::GetCurrentTime ( );
			m_vcsxBusynessLock-> Unlock ( );
			
			m_vcsConnectionHandlersProtector-> Unlock ( );
			
			m_vsyncEventForNewHandlers-> Lock ( );
			
			return VE_OK;
		}
		else
		{
			nCHCount = m_vctrConnectionHandlers. size ( );
			
			m_vcsConnectionHandlersProtector-> Unlock ( );
		}
	}
	else
		return VE_SRVR_FAILED_TO_SYNC_LOCK;
	
	
	VError											vError = VE_OK;
	
	for ( uLONG8 i = 0; i < nCHCount; i++ )
	{
		if ( !m_vcsConnectionHandlersProtector-> Lock ( ) )
			return VE_SRVR_FAILED_TO_SYNC_LOCK;
		
		m_CurrentConnectionHandler = m_vctrConnectionHandlers [ i ];
		m_CurrentConnectionHandler-> _ResetRedistributionCount ( );
		
		m_vcsxBusynessLock-> Lock ( );
		m_nLatestHandlingStart = VSystem::GetCurrentTime ( );
		m_bIsHandling = true;
		m_vcsxBusynessLock-> Unlock ( );
		
		m_vcsConnectionHandlersProtector-> Unlock ( );
		
		VConnectionHandler::E_WORK_STATUS			wStatus = m_CurrentConnectionHandler-> Handle ( vError );
		
		if ( !m_vcsConnectionHandlersProtector-> Lock ( ) )
			return VE_SRVR_FAILED_TO_SYNC_LOCK;
		
		m_vcsxBusynessLock-> Lock ( );
		uLONG									nCurrentTime = VSystem::GetCurrentTime ( );
		uLONG									nHandlingTime = ( nCurrentTime >= m_nLatestHandlingStart ) ? nCurrentTime - m_nLatestHandlingStart : nCurrentTime;
		m_bIsHandling = false;
		m_nBusynessDuration += nHandlingTime;
		if ( m_nBusynessDuration > m_nBusynessInterval )
			m_nBusynessDuration = m_nBusynessInterval;
		if ( nCurrentTime - m_nLatestBusynessIntervalStart > m_nBusynessInterval )
		{
			m_nLatestBusynessIntervalStart = nCurrentTime;
			m_nPreviousBusyness = ( m_nBusynessDuration * 100 ) / m_nBusynessInterval;
			m_nBusynessDuration = 0;
		}
		m_vcsxBusynessLock-> Unlock ( );
		
		if ( m_vctrConnectionHandlers. size ( ) != nCHCount )
			i = nCHCount;
		
		if ( wStatus == VConnectionHandler::eWS_DONE )
		{
			m_vctrConnectionHandlers. erase (
											 std::find (
														m_vctrConnectionHandlers. begin ( ),
														m_vctrConnectionHandlers. end ( ),
														m_CurrentConnectionHandler ) );
			m_CurrentConnectionHandler-> Release ( );
			nCHCount--;
			i--;
		}
		
		m_CurrentConnectionHandler = NULL;
		
		m_vcsConnectionHandlersProtector-> Unlock ( );
	}
	
	
	
	
	
	
	
	/*VConnectionHandler*								vcHandler;
	 std::vector<VConnectionHandler*>::iterator		iter = m_vctrConnectionHandlers. begin ( );
	 while ( iter != m_vctrConnectionHandlers. end ( ) )
	 {
	 vcHandler = *iter;
	 
	 m_vcsxBusynessLock-> Lock ( );
	 m_nLatestHandlingStart = VSystem::GetCurrentTime ( );
	 m_bIsHandling = true;
	 m_vcsxBusynessLock-> Unlock ( );
	 
	 VConnectionHandler::E_WORK_STATUS			wStatus = vcHandler-> Handle ( vError );
	 
	 m_vcsxBusynessLock-> Lock ( );
	 uLONG									nCurrentTime = VSystem::GetCurrentTime ( );
	 uLONG									nHandlingTime = ( nCurrentTime >= m_nLatestHandlingStart ) ? nCurrentTime - m_nLatestHandlingStart : nCurrentTime;
	 m_bIsHandling = false;
	 m_nBusynessDuration += nHandlingTime;
	 if ( m_nBusynessDuration > m_nBusynessInterval )
	 m_nBusynessDuration = m_nBusynessInterval;
	 if ( nCurrentTime - m_nLatestBusynessIntervalStart > m_nBusynessInterval )
	 {
	 m_nLatestBusynessIntervalStart = nCurrentTime;
	 m_nPreviousBusyness = ( m_nBusynessDuration * 100 ) / m_nBusynessInterval;
	 m_nBusynessDuration = 0;
	 }
	 m_vcsxBusynessLock-> Unlock ( );
	 
	 if ( wStatus == VConnectionHandler::eWS_DONE )
	 {
	 vcHandler-> Release ( );
	 iter = m_vctrConnectionHandlers. erase ( iter );
	 }
	 else
	 iter++;
	 }*/
	
	return VE_OK;
}

Boolean VSharedWorker::DoRun ( )
{
	/* Deal with new and current connection handlers. */
	while ( GetState ( ) != TS_DYING && GetState ( ) != TS_DEAD )
	{
		AcceptNewConnectionHandlers ( );
		RunConnectionHandlers ( );
		
		if ( GetState ( ) == TS_DYING || GetState ( ) == TS_DEAD )
			break;
		
		VTask::GetCurrent ( )-> Sleep ( 10 );
	}
	
	ReleaseAllConnectionHandlers ( );
	
	return false;
}

void VSharedWorker::ReleaseAllConnectionHandlers ( )
{
	/* Release current connection handlers. */
	
	if ( !m_vcsConnectionHandlersProtector-> Lock ( ) )
		return;
	
	std::vector<VConnectionHandler*>::iterator		iter = m_vctrConnectionHandlers. begin ( );
	while ( iter != m_vctrConnectionHandlers. end ( ) )
	{
		( *iter )-> Release ( );
		iter++;
	}
	m_vctrConnectionHandlers. clear ( );
	
	m_vcsConnectionHandlersProtector-> Unlock ( );
	
	/* Release pending connection handlers. */
	VConnectionHandler*								vcHandler;
	m_vcsCHQueueLock-> Lock ( );
	
	while ( m_qNewConnectionHandlers. size ( ) > 0 )
	{
		vcHandler = m_qNewConnectionHandlers. front ( );
		m_qNewConnectionHandlers. pop ( );
		vcHandler-> Release ( );
	}
	
	m_vcsCHQueueLock-> Unlock ( );
}

VError VSharedWorker::ExtractStuckConnectionHandlers ( std::vector<VConnectionHandler*>& outStuckConnectionHandlers )
{
	if ( !m_vcsConnectionHandlersProtector-> Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;
	
	uLONG						nStuckDuration = VSystem::GetCurrentTime ( ) - m_nLatestHandlingStart;
	if ( m_CurrentConnectionHandler && nStuckDuration > m_nBusynessInterval / 2 )
	{
		/* Is stuck! */
		std::vector<VConnectionHandler*>::iterator			iterCH = m_vctrConnectionHandlers. begin ( );
		while ( iterCH != m_vctrConnectionHandlers. end ( ) )
		{
			if ( *iterCH != m_CurrentConnectionHandler && ( *iterCH )-> _GetLoadRedistributionCount ( ) < 2 )
			{
				( *iterCH )-> _IncrementLoadRedistributionCount ( );
				outStuckConnectionHandlers. push_back ( *iterCH );
				iterCH = m_vctrConnectionHandlers. erase ( iterCH );
			}
			else
				iterCH++;
		}
	}
	
	m_vcsConnectionHandlersProtector-> Unlock ( );
	
	if ( !m_vcsCHQueueLock-> Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;
	
	while ( m_qNewConnectionHandlers. size ( ) > 0 )
	{
		outStuckConnectionHandlers. push_back ( m_qNewConnectionHandlers. front ( ) );
		m_qNewConnectionHandlers. pop ( );
	}
	
	m_vcsCHQueueLock-> Unlock ( );
	
	return VE_OK;
}


/*
 Shared load balancing:
 OK		* Synchronize access to the vector of current handlers in a shared worker.
 OK		* Keep a pointer to the handler that's being handled.
 OK		* Add a counter for how many times a handler has been moved from worker to worker.
 OK		* Find idle or not-very-busy workers.
 OK		* Extract handlers from the worker that "stuck" on a handler.
 * Redistribute obtained handlers between non-busy workers.
 */


VSharedLoadBalancer::VSharedLoadBalancer ( VWorkerPool& vwPool ) :
VTask ( NULL, 0, XBOX::eTaskStylePreemptive, NULL ),
fWorkerPool ( vwPool ),
m_vseWait ( )
{
	;
}

VSharedLoadBalancer::~VSharedLoadBalancer ( )
{
	;
}

Boolean VSharedLoadBalancer::DoRun ( )
{
	while ( true )
	{
		m_vseWait. Lock ( 5000 );
		m_vseWait. Reset ( );
		
		if ( GetState ( ) == TS_DYING || GetState ( ) == TS_DEAD )
			break;
		
		fWorkerPool. BalanceSharedLoad ( );
	}
	
	return false;
}


END_TOOLBOX_NAMESPACE
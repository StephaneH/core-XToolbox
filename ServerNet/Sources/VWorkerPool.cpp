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

#include "VWorkerPool.h"

#include "Tools.h"


BEGIN_TOOLBOX_NAMESPACE


using namespace ServerNetTools;


VExclusiveWorker::VExclusiveWorker (
								VWorkerPool& vParentWorkerPool ,
								VConnectionHandlerQueue& vExclusiveCHQueue) :
																		VTask ( NULL, 0, XBOX::eTaskStylePreemptive, NULL ),
																		m_vParentWorkerPool ( vParentWorkerPool ),
																		m_vsynceWaitForHandler ( ),
																		m_vExclusiveCHQueue ( vExclusiveCHQueue )
{
	m_vConnectionHandler = NULL;
	m_nSpareStamp = 0;
	SetKindData((sLONG_PTR) this);
}

VExclusiveWorker::~VExclusiveWorker ( )
{
	;
}

void VExclusiveWorker::WakeUpFromIdling ( )
{
	m_vsynceWaitForHandler. Unlock ( );
}


VError VExclusiveWorker::SetConnectionHandler ( VConnectionHandler* inConnectionHandler )
{
	if ( !inConnectionHandler )
		return VE_INVALID_PARAMETER;

	m_vConnectionHandler = inConnectionHandler;

	m_vsynceWaitForHandler. Unlock ( );

	return VE_OK;
}

Boolean VExclusiveWorker::DoRun ( )
{
	VError											vError = VE_OK;
	VConnectionHandler::E_WORK_STATUS				wStatus;
	while ( GetState ( ) != TS_DYING && GetState ( ) != TS_DEAD )
	{
		StDropErrorContext errCtx;
		
		m_vsynceWaitForHandler. Lock ( );
		m_vsynceWaitForHandler. Reset ( );
		if ( !m_vConnectionHandler )
			continue;

		// Check if user of the worker pool changed the TaskKindData, this is not allowed!
		// If you fall in this assert => you modified the TaskKindData while you should not.
		if( !testAssert(GetKindData() == ( sLONG_PTR ) this ) )
			SetKindData( ( sLONG_PTR ) this );

		if( m_nSpareStamp != 0 )
			++m_nSpareStamp;
		
		// Following values are supposed to be changed ASAP in m_vConnectionHandler-> Handle()
		SetName( CVSTR("Reused spare process") );
		SetKind( kWorkerPool_InUseTaskKind );

		// Called code will (should) set the TaskKind/TaskName to specific values.
		wStatus = m_vConnectionHandler-> Handle ( vError );
		m_vConnectionHandler-> Release ( );
		m_vConnectionHandler = NULL;

		// Check if user of the worker pool changed the TaskKindData, this is not allowed!
		// If you fall in this assert => you modified the TaskKindData while you should not.
		if( !testAssert(GetKindData() == ( sLONG_PTR ) this ) )
			SetKindData( ( sLONG_PTR ) this );

		if ( GetState ( ) == TS_DYING || GetState ( ) == TS_DEAD )
			break;

		/* Check if there are new handlers pending in the queue. */
		if ( !( m_vConnectionHandler = m_vExclusiveCHQueue. Pop ( &vError ) ) )
		{
			m_vParentWorkerPool. UseAsIdling ( this );

			continue;
		}

		m_vsynceWaitForHandler. Unlock ( );
	}

	return false;
}

VError VExclusiveWorker::StopConnectionHandlers ( int inType )
{
	if ( !m_vConnectionHandler || m_vConnectionHandler-> GetType ( ) != inType )
		return VE_OK;

	return m_vConnectionHandler-> Stop ( );
}


VWorkerPool::VWorkerPool (
					unsigned short nSharedCount,
					unsigned short nSharedMaxCount,
					unsigned short nSharedMaxBusyness,
					unsigned short nExclusiveCount,
					unsigned short nExclusiveMaxCount ) :
#if WITH_SHARED_WORKERS
					m_vctrSharedWorkers ( ),
#endif
					m_vctrAllExclusiveWorkers ( ),
					m_vctrExclusiveWorkersIdling ( ),
					m_vExclusiveCHQueue ( )
{
#if WITH_SHARED_WORKERS
	m_vcsSharedProtector = new VCriticalSection ( );
#endif
	m_vcsExclusiveProtector = new VCriticalSection ( );

	m_nInitialSharedCount = nSharedCount;
	m_nSharedMaxCount = nSharedMaxCount;
	m_nSharedMaxBusyness = nSharedMaxBusyness;
	m_nInitialExclusiveCount = nExclusiveCount;
	m_nExclusiveMaxCount = nExclusiveMaxCount;
	m_nExclusiveIdlePageSize = 5;

	m_vstrNameFoSpare = "Spare process";

#if WITH_SHARED_WORKERS
	VSharedWorker*			vsWorker = NULL;
#endif
	
	VExclusiveWorker*		veWorker = NULL;
	
	//TODO : mettre dans une methode init()
#if WITH_SHARED_WORKERS	
	for ( unsigned short i = 0; i < m_nInitialSharedCount; i++ )
	{
		vsWorker = new VSharedWorker ( );
		VString				vstrName ( "SHARED pool worker " );
		vstrName. AppendLong ( i );
		vsWorker-> SetName ( vstrName );
		vsWorker-> Run ( );
		m_vctrSharedWorkers. push_back ( vsWorker );
	}
#endif	
	for ( unsigned short i = 0; i < m_nInitialExclusiveCount; i++ )
	{
		veWorker = new VExclusiveWorker ( *this, m_vExclusiveCHQueue );
		VString				vstrName ( m_vstrNameFoSpare );
		vstrName += " ";
		vstrName. AppendLong ( i );
		veWorker-> SetName ( vstrName );
		veWorker-> SetKind( kWorkerPool_SpareTaskKind );
		veWorker-> Run ( );
		m_vctrExclusiveWorkersIdling. push_back ( veWorker );
		m_vctrAllExclusiveWorkers. push_back ( veWorker );
	}

#if WITH_SHARED_WORKERS
	if ( m_nInitialSharedCount > 0 )
	{
		m_vSharedLoadBalancer = new VSharedLoadBalancer ( *this );
		m_vSharedLoadBalancer-> SetName ( CVSTR ( "Shared Load Balancer" ) );
		m_vSharedLoadBalancer-> Run ( );
	}
	else
		m_vSharedLoadBalancer = 0;
#endif
}

VWorkerPool::~VWorkerPool ( )
{
#if WITH_SHARED_WORKERS	
	if ( m_vSharedLoadBalancer != 0 )
	{
		m_vSharedLoadBalancer-> Kill ( );
		m_vSharedLoadBalancer-> WakeUpFromIdling ( );
	}

	m_vcsSharedProtector-> Lock ( );

		std::vector<VSharedWorker*>::iterator				iterS = m_vctrSharedWorkers. begin ( );
		while ( iterS != m_vctrSharedWorkers. end ( ) )
		{
			( *iterS )-> Kill ( );
			( *iterS )-> WakeUpFromIdling ( );
			ReleaseRefCountable(&(*iterS));
			iterS++;
		}

	m_vcsSharedProtector-> Unlock ( );
	delete m_vcsSharedProtector;
#endif

	m_vcsExclusiveProtector-> Lock ( );

		m_vctrExclusiveWorkersIdling. clear ( );
		std::vector<VExclusiveWorker*>::iterator			iterE = m_vctrAllExclusiveWorkers. begin ( );
		while ( iterE != m_vctrAllExclusiveWorkers. end ( ) )
		{
			( *iterE )-> Kill ( );
			( *iterE )-> WakeUpFromIdling ( );
			ReleaseRefCountable(&(*iterE));
			iterE++;
		}

	m_vcsExclusiveProtector-> Unlock ( );
	delete m_vcsExclusiveProtector;

	m_vExclusiveCHQueue. ReleaseAll ( );
}

VError VWorkerPool::AddExclusiveConnectionHandler ( VConnectionHandler* inConnectionHandler )
{
	if ( !m_vcsExclusiveProtector-> Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;

	VError					vError = VE_OK;
	VExclusiveWorker*		veWorker = NULL;
	if ( m_vctrExclusiveWorkersIdling. size ( ) > 0 )
	{
		veWorker = m_vctrExclusiveWorkersIdling. back ( );
		m_vctrExclusiveWorkersIdling. pop_back ( );
		vError = veWorker-> SetConnectionHandler ( inConnectionHandler );
	}
	else if ( m_vctrAllExclusiveWorkers. size ( ) < m_nExclusiveMaxCount )
	{
		veWorker = new VExclusiveWorker ( *this, m_vExclusiveCHQueue );
		VString				vstrName ( "EXCLUSIVE pool worker " );
		vstrName. AppendLong8 ( m_vctrAllExclusiveWorkers. size ( ) );
		veWorker-> SetName ( vstrName );
		veWorker-> Run ( );
		m_vctrAllExclusiveWorkers. push_back ( veWorker );
		vError = veWorker-> SetConnectionHandler ( inConnectionHandler );
	}
	else
		vError = m_vExclusiveCHQueue. Push ( inConnectionHandler );

	m_vcsExclusiveProtector-> Unlock ( );

	return vError;
}

VError VWorkerPool::AddConnectionHandler ( VConnectionHandler* inConnectionHandler )
{
	if ( !inConnectionHandler-> CanShareWorker ( ) )
		/* If there is an idling thread, use it. If not, create one, if the limit
		allows. Otherwise - queue the handler for later execution. */
		return AddExclusiveConnectionHandler ( inConnectionHandler );

#if WITH_SHARED_WORKERS
	short											nMinBusyness = 100;
	short											nCurrentBusyness = 0;

	if ( !m_vcsSharedProtector-> Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;

		std::vector<VSharedWorker*>::iterator		iter = m_vctrSharedWorkers. begin ( );
		VSharedWorker*								vwLeastBusy = *iter;
		while ( iter != m_vctrSharedWorkers. end ( ) )
		{
			nCurrentBusyness = ( *iter )-> GetBusyness ( );
			if ( nCurrentBusyness == 0 )
			{
				/* Found a worker that does not do anything. */
				vwLeastBusy = *iter;
				nMinBusyness = 0;

				break;
			}

			if ( nCurrentBusyness < nMinBusyness )
			{
				nMinBusyness = nCurrentBusyness;
				vwLeastBusy = *iter;
			}

			iter++;
		}

		if ( nMinBusyness > m_nSharedMaxBusyness && m_vctrSharedWorkers. size ( ) < m_nSharedMaxCount )
		{
			/* All current workers are too busy and I'm allowed to create more workers.
			Let's do just that. */
			vwLeastBusy = new VSharedWorker ( );
			VString				vstrName ( "SHARED pool worker " );
			vstrName. AppendLong8 ( m_vctrSharedWorkers. size ( ) );
			vwLeastBusy-> SetName ( vstrName );
			vwLeastBusy-> Run ( );
			m_vctrSharedWorkers. push_back ( vwLeastBusy );
		}

		vwLeastBusy-> AddConnectionHandler ( inConnectionHandler );

	m_vcsSharedProtector-> Unlock ( );

	return VE_OK;
#else
	return VE_INVALID_PARAMETER;
#endif
}

VError VWorkerPool::UseAsIdling ( VExclusiveWorker* inWorker )
{
	if ( !m_vcsExclusiveProtector-> Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;

		inWorker-> MakeSpareStampDirty();
		inWorker-> SetName( m_vstrNameFoSpare);
		inWorker-> SetKind( kWorkerPool_SpareTaskKind );

		if ( m_vctrExclusiveWorkersIdling. size ( ) > m_nExclusiveIdlePageSize + m_nInitialExclusiveCount - 1 )
		{
			m_vctrExclusiveWorkersIdling. push_back ( inWorker );
			VError			vError = RemoveExclusiveIdlers ( m_nExclusiveIdlePageSize );
			xbox_assert ( vError == VE_OK );
		}
		else
			m_vctrExclusiveWorkersIdling. push_back ( inWorker );

	m_vcsExclusiveProtector-> Unlock ( );

	return VE_OK;
}

VError VWorkerPool::RemoveExclusiveIdlers ( unsigned short inCount )
{
	if ( !m_vcsExclusiveProtector-> Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;

	VError										vError = VE_OK;
	if ( inCount > m_vctrExclusiveWorkersIdling. size ( ) )
		inCount = m_vctrExclusiveWorkersIdling. size ( );

	VExclusiveWorker*							veWorker = 0;
	std::vector<VExclusiveWorker*>::iterator	iterWorker = m_vctrAllExclusiveWorkers. begin ( );
	for ( unsigned short i = 0; i < inCount; i++ )
	{
		veWorker = m_vctrExclusiveWorkersIdling. back ( );
		m_vctrExclusiveWorkersIdling. pop_back ( );
		iterWorker = std::find ( m_vctrAllExclusiveWorkers. begin ( ), m_vctrAllExclusiveWorkers. end ( ), veWorker );
		if ( iterWorker != m_vctrAllExclusiveWorkers. end ( ) )
			m_vctrAllExclusiveWorkers. erase ( iterWorker );

		veWorker-> Kill ( );
		veWorker-> WakeUpFromIdling ( );
		veWorker-> Release ( );
	}

	if ( !m_vcsExclusiveProtector-> Unlock ( ) )
	{
		xbox_assert ( false );
		vError = VE_SRVR_FAILED_TO_SYNC_LOCK;
	}

	return vError;
}


#if WITH_SHARED_WORKERS
VError VWorkerPool::BalanceSharedLoad ( )
{
	/* NOTEs: First, of course, I should look if I can create new workers. */

	std::vector<VSharedWorker*>			vctrNonBusyWorkers;
	VError								vError = GetNonBusySharedWorkers ( vctrNonBusyWorkers );
	if ( vError != VE_OK )
		return vError;
	if ( vctrNonBusyWorkers. size ( ) == 0 )
		return VE_OK;

	std::vector<VConnectionHandler*>	vctrStuckConnectionHandlers;
	vError = ExtractStuckConnectionHandlers ( vctrStuckConnectionHandlers );
	if ( vError != VE_OK )
		/* TODO: Give back all the stuck connection handlers. */
		return vError;
	if ( vctrStuckConnectionHandlers. size ( ) == 0 )
		return VE_OK;

	std::vector<VSharedWorker*>::iterator				iterSW = vctrNonBusyWorkers. begin ( );
	VConnectionHandler*									vcHandler = NULL;
	while ( vctrStuckConnectionHandlers. size ( ) > 0 )
	{
		vcHandler = vctrStuckConnectionHandlers. back ( );
		vctrStuckConnectionHandlers. pop_back ( );
		( *iterSW )-> AddConnectionHandler ( vcHandler );
		iterSW++;
		if ( iterSW == vctrNonBusyWorkers. end ( ) )
			iterSW = vctrNonBusyWorkers. begin ( );
	}

	return VE_UNIMPLEMENTED;
}

VError VWorkerPool::GetNonBusySharedWorkers ( std::vector<VSharedWorker*>& outNonBusy )
{
	if ( !m_vcsSharedProtector-> Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;

		VError										vError = VE_OK;

		std::vector<VSharedWorker*>::iterator		iter = m_vctrSharedWorkers. begin ( );
		while ( iter != m_vctrSharedWorkers. end ( ) )
		{
			if ( ( *iter )-> GetBusyness ( ) < m_nSharedMaxBusyness )
				outNonBusy. push_back ( *iter );

			iter++;
		}

	m_vcsSharedProtector-> Unlock ( );

	return vError;
}

VError VWorkerPool::ExtractStuckConnectionHandlers ( std::vector<VConnectionHandler*>& outStuckConnectionHandlers )
{
	if ( !m_vcsSharedProtector-> Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;

		VError										vError = VE_OK;

		std::vector<VSharedWorker*>::iterator		iter = m_vctrSharedWorkers. begin ( );
		while ( iter != m_vctrSharedWorkers. end ( ) )
		{
			( *iter )-> ExtractStuckConnectionHandlers ( outStuckConnectionHandlers );

			iter++;
		}

	m_vcsSharedProtector-> Unlock ( );

	return vError;
}
#endif

VError VWorkerPool::StopConnectionHandlers ( int inType, VTaskID inTaskID )
{
#if WITH_SHARED_WORKERS
	if ( !m_vcsSharedProtector-> Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;

		std::vector<VSharedWorker*>::iterator				iterS = m_vctrSharedWorkers. begin ( );
		while ( iterS != m_vctrSharedWorkers. end ( ) )
		{
			if ( inTaskID == NULL_TASK_ID || ( *iterS )-> GetID ( ) == inTaskID )
				( *iterS )-> StopConnectionHandlers ( inType );
			iterS++;
		}

	m_vcsSharedProtector-> Unlock ( );
#endif

	if ( !m_vcsExclusiveProtector-> Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;

		std::vector<VExclusiveWorker*>::iterator			iterE = m_vctrAllExclusiveWorkers. begin ( );
		while ( iterE != m_vctrAllExclusiveWorkers. end ( ) )
		{
			if ( inTaskID == NULL_TASK_ID || ( *iterE )-> GetID ( ) == inTaskID )
				( *iterE )-> StopConnectionHandlers ( inType );
			iterE++;
		}

	m_vcsExclusiveProtector-> Unlock ( );

	return VE_OK;
}

VError VWorkerPool::SetSpareTaskName ( VString const & inName )
{
	if ( !m_vcsExclusiveProtector-> Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;

		m_vstrNameFoSpare = inName;

		std::vector<VExclusiveWorker*>::iterator			iterE = m_vctrExclusiveWorkersIdling. begin ( );
		while ( iterE != m_vctrExclusiveWorkersIdling. end ( ) )
		{
			( *iterE )-> SetName ( inName );
			iterE++;
		}

	m_vcsExclusiveProtector-> Unlock ( );

	return VE_OK;
}


END_TOOLBOX_NAMESPACE


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

#include "VServerErrors.h"

#include "VConnectionHandlerFactory.h"

#include <queue>
#include <vector>


#ifndef __SNET_WORKER_POOL__
#define __SNET_WORKER_POOL__


BEGIN_TOOLBOX_NAMESPACE


/** @brief	The TaskKind and task name are set by the worker pool for exclusive workers:
				-> TaskKind data is set to this (I mean it calls SetKindData(this))
				-> When the task becomes spare:
						Kind		kWorkerPool_SpareTaskKind
						Name		"Spare process" (or value set by a call to VWorkerPool::SetSpareTaskName())

				-> And when it's reused. In this case, user of the workerpool is supposed to change the values
				   to more specific ones. Just before calling Handle(), values are set to
						Kind		kWorkerPool_InUseTaskKind
						Name		"Reused spare process"
*/

/** @brief	This class is the one used by clients of the workerpool*/
class XTOOLBOX_API VExclusiveWorker : public VTask
{
	public :

		VExclusiveWorker (
						VWorkerPool& vParentWorkerPool,
						VConnectionHandlerQueue& vExclusiveCHQueue );
		virtual ~VExclusiveWorker ( );

		virtual VError SetConnectionHandler ( VConnectionHandler* inConnectionHandler );

		virtual void WakeUpFromIdling ( );

		virtual VError StopConnectionHandlers ( int inType );

		/**@brief	The spare stamp is modified everytime the task becomes spare, reused, spare, reused,...
					In this first implementation (2009-08-25) access to the spare stamp is not protected
					with a mutex.
		*/
		uLONG GetSpareStamp() const			{ return m_nSpareStamp; }
		void MakeSpareStampDirty()			{ ++m_nSpareStamp; }

	protected :

		virtual Boolean DoRun ( );

		VWorkerPool&								m_vParentWorkerPool;
		VSyncEvent									m_vsynceWaitForHandler;
		VConnectionHandler*							m_vConnectionHandler;
		VConnectionHandlerQueue&					m_vExclusiveCHQueue;

		uLONG										m_nSpareStamp;
};


class XTOOLBOX_API VWorkerPool : public VObject, public IRefCountable
{
	public:

		VWorkerPool (
					unsigned short nSharedCount,
					unsigned short nSharedMaxCount,
					unsigned short nSharedMaxBusyness,
					unsigned short nExclusiveCount,
					unsigned short nExclusiveMaxCount );
		virtual ~VWorkerPool ( );

		virtual VError AddConnectionHandler ( VConnectionHandler* inConnectionHandler );

		VError UseAsIdling ( VExclusiveWorker* inWorker );

		VError BalanceSharedLoad ( );

		/* If inTaskID is NULL_TASK_ID then stops all connection handlers of a given type inType. Otherwise, stops
		only a handler of a given type that's being executed by a task with a given ID. */
		virtual VError StopConnectionHandlers ( int inType, VTaskID inTaskID = NULL_TASK_ID );

		VError SetSpareTaskName ( VString const & inName );

	protected :

		/* Everything related to shared workers. */
		unsigned short								m_nInitialSharedCount;
		unsigned short								m_nSharedMaxCount;
		unsigned short								m_nSharedMaxBusyness;

#if WITH_SHARED_WORKERS
		VCriticalSection*							m_vcsSharedProtector;
		std::vector<VSharedWorker*>					m_vctrSharedWorkers;
#endif
	
		/* Everything related to exclusive workers. */
		VCriticalSection*							m_vcsExclusiveProtector;
		std::vector<VExclusiveWorker*>				m_vctrAllExclusiveWorkers;
		std::vector<VExclusiveWorker*>				m_vctrExclusiveWorkersIdling;
		/*std::queue<>*/

		unsigned short								m_nInitialExclusiveCount;
		unsigned short								m_nExclusiveMaxCount;
		unsigned short								m_nExclusiveIdlePageSize;

		VConnectionHandlerQueue						m_vExclusiveCHQueue;

#if WITH_SHARED_WORKERS
		VSharedLoadBalancer*						m_vSharedLoadBalancer;
#endif

		VString										m_vstrNameFoSpare;


		VError AddExclusiveConnectionHandler ( VConnectionHandler* inConnectionHandler );
		VError RemoveExclusiveIdlers ( unsigned short inCount );

#if WITH_SHARED_WORKERS
		VError GetNonBusySharedWorkers ( std::vector<VSharedWorker*>& outNonBusy );
		VError ExtractStuckConnectionHandlers ( std::vector<VConnectionHandler*>& outStuckConnectionHandlers );
#endif
};


END_TOOLBOX_NAMESPACE


#endif


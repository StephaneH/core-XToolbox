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
#ifndef SNET_SHARED_WORKERS
#define SNET_SHARED_WORKERS


BEGIN_TOOLBOX_NAMESPACE


class XTOOLBOX_API VSharedWorker : public VTask
{
	public :
	
	VSharedWorker ( );
	virtual ~VSharedWorker ( );
	
	virtual VError AddConnectionHandler ( VConnectionHandler* inConnectionHandler );
	virtual uLONG8 GetConnectionHandlerCount ( ) { return m_vctrConnectionHandlers. size ( ); }
	
	virtual volatile Boolean IsHandling ( ) { return m_bIsHandling; }
	virtual volatile uLONG GetLatestHandlingStart ( ) { return m_nLatestHandlingStart; }
	virtual volatile uLONG GetLatestIdlingStart ( ) { return m_nLatestIdlingStart; }
	virtual short GetBusyness ( );
	virtual void WakeUpFromIdling ( );
	virtual VError StopConnectionHandlers ( int inType );
	
	VError ExtractStuckConnectionHandlers ( std::vector<VConnectionHandler*>& outStuckConnectionHandlers );
	
	protected :
	
	virtual Boolean DoRun ( );
	virtual VError RunConnectionHandlers ( );
	
	virtual VError AcceptNewConnectionHandlers ( );
	virtual void ReleaseAllConnectionHandlers ( );
	
	VCriticalSection*							m_vcsCHQueueLock;
	std::queue<VConnectionHandler*>				m_qNewConnectionHandlers;
	
	VCriticalSection*							m_vcsConnectionHandlersProtector;
	std::vector<VConnectionHandler*>			m_vctrConnectionHandlers;
	VConnectionHandler*							m_CurrentConnectionHandler;
	
	VSyncEvent*									m_vsyncEventForNewHandlers;
	
	VCriticalSection*							m_vcsxBusynessLock;
	volatile Boolean							m_bIsHandling;
	uLONG										m_nBusynessInterval;
	uLONG										m_nLatestBusynessIntervalStart;
	uLONG										m_nBusynessDuration;
	volatile uLONG								m_nLatestHandlingStart;
	volatile short								m_nPreviousBusyness;
	volatile uLONG								m_nLatestIdlingStart;
};


class XTOOLBOX_API VSharedLoadBalancer : public VTask
{
public:
	
	VSharedLoadBalancer ( VWorkerPool& vwPool );
	virtual ~VSharedLoadBalancer ( );
	
	virtual void WakeUpFromIdling ( ) { m_vseWait. Unlock ( ); }
	
protected:
	
	VWorkerPool&						fWorkerPool;
	VSyncEvent							m_vseWait;
	
	virtual Boolean DoRun ( );
};


END_TOOLBOX_NAMESPACE


#endif

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


#ifndef __SNET_SESSION__
#define __SNET_SESSION__


BEGIN_TOOLBOX_NAMESPACE


class XTOOLBOX_API VTCPClientSession : public VObject, public IRefCountable
{
	public :
	
	virtual VError Postpone ( VTCPEndPoint& inEndPoint ) = 0;
	virtual VTCPEndPoint* Restore ( VTCPEndPoint& inEndPoint, XBOX::VError& outError ) = 0;
};


class XTOOLBOX_API VTCPServerSession : public VObject, public IRefCountable
{
	public :
	
	VTCPServerSession ( );
	virtual ~VTCPServerSession ( ) { ; }
	
	void SetID ( VString const & inID ) { fStringID. FromString ( inID ); }
	void GetID ( VString & outID ) { outID. FromString ( fStringID ); }
	
	void GetClientUUID ( VUUID& outUUID ) { outUUID. FromVUUID ( fUuidClient ); }
	void SetClientUUID ( VUUID const & inUUID ) { fUuidClient. FromVUUID ( inUUID ); }
	bool HasSameClientUUID ( VUUID const & inUUID ) { return fUuidClient. EqualToSameKind ( &inUUID ) != 0; }
	
	uLONG GetTimeOut ( ) { return fTimeOut; }
	void SetTimeOut ( uLONG inTimeOut ) { fTimeOut = inTimeOut; }
	bool IsTimedOut ( );
	
	VError Postpone ( );
	
	/* Returns a KEEP-ALIVE request for a given protocol.
	 Returned pointer is owned by the VTCPServerSession instance and should not be deleted by the caller.
	 Default implementation returns zero-length null request. */
	virtual void* GetKeepAliveRequest ( uLONG& outLength );
	/* Returns an expected successfull KEEP-ALIVE response for a given protocol.
	 Returned pointer is owned by the VTCPServerSession instance and should not be deleted by the caller.
	 Default implementation returns zero-length null request. */
	virtual void* GetKeepAliveResponse ( uLONG& outLength );
	
	void SetKeepAliveInterval ( uLONG inKeepAliveInterval ) { fKeepAliveInterval = inKeepAliveInterval; }
	uLONG GetKeepAliveInterval ( ) { return fKeepAliveInterval; }
	void SetLastKeepAlive ( uLONG inLastKeepAlive ) { fLastKeepAlive = inLastKeepAlive; }
	uLONG GetLastKeepAlive ( ) { return fLastKeepAlive; }
	
	private :
	
	VString						fStringID;
	VUUID						fUuidClient;
	uLONG						fTimeOut;
	VTime						fPostponeStart; // The time at which this end point was postponed
	
	uLONG						fLastKeepAlive;
	uLONG						fKeepAliveInterval;
};


class XTOOLBOX_API VTCPSessionManager : public VObject
{
	public :
	
	static VTCPSessionManager* Get ( ) { return &sInstance; }
	
	void Stop ( );
	
	VError Add ( VTCPEndPoint* inEndPoint );
	VError Remove ( VTCPEndPoint* inEndPoint );
	bool IsPostponed ( VTCPEndPoint* inEndPoint );
	VError Restore ( VTCPEndPoint* inEndPoint ); // Called only by EndPoint
	
	VError StoreServerSession ( VTCPServerSession* inSession ); // Just stores the object, does not Retain ()
	VTCPServerSession* RemoveServerSession ( XBOX::VString const & inSessionID, VError & outError ); // Just removes the object, does not Release ( )
	VError ReleaseServerSessions ( VUUID const & inClientUUID );
	// Release ( ) on a server session is called only if it expires
	
	VError AddForKeepAlive ( VTCPEndPoint* inEndPoint, VTCPServerSession* inSession );
	VError RemoveFromKeepAlive ( VTCPEndPoint const & inEndPoint );
	
	static void DebugMessage ( XBOX::VString const & inMessage, VTCPEndPoint* inEndPoint, XBOX::VError inError = XBOX::VE_OK );
	
private:
	
	VTCPSessionManager ( );
	
	void Start ( );
	VError ReleaseAllServerSessions ( );
	
	static sLONG Run ( VTask* vTask );
	static VError HandleForIdleTimeOut ( VTCPEndPoint* vtcpEndPoint );
	static VError HandleForPostponedTimeOut ( VTCPEndPoint* vtcpEndPoint, bool& outTimedOut );
	static VError HandleForKeepAlive ( VTCPEndPoint* vtcpEndPoint, VTCPServerSession* vtcpServerSession );

	static VTCPSessionManager					sInstance;
	
	/*
	 static std::vector<SQLNetworkSession*>		fPostponedSessions;
	 static VMutex								fPostponedSessionsLock;
	 */
	
	/* End points that are potential candicates to be postponed if timeout for idling is reached. */
	static std::vector<VTCPEndPoint*>			sEndPointsIdle;
	static std::vector<VTCPEndPoint*>			sEndPointsPostponed;
	static VCriticalSection						sEndPoints;
	
	static std::vector<VTCPServerSession*>		sServerSessions;
	static VCriticalSection						sServerSessionsMutex;
	
	static std::map<sLONG, std::pair< VTCPEndPoint*, VTCPServerSession* > >		sMapKeepAliveSessions;
	static VCriticalSection						sKeepAliveSessionsMutex;
	
	VCriticalSection							fWorkerMutex;
	VTask*										fWorkerTask;
	static VSyncEvent							sSyncEventForSleep;
	static uLONG								sWorkerSleepDuration; // Milliseconds
	static uLONG								sKeepAliveTimeOut;
	/*
	 static VSyncEvent							sSyncEvtForWorker;
	 */
};


END_TOOLBOX_NAMESPACE


#endif
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

#include "VEndPoint.h"
#include "SelectIO.h"


#ifndef __SNET_TCP_ENDPOINT__
#define __SNET_TCP_ENDPOINT__


BEGIN_TOOLBOX_NAMESPACE


class VKeyCertChain;

class XTOOLBOX_API VTCPEndPoint : public VEndPoint
{
public:
	
	VTCPEndPoint( XTCPSock* inSock );
	VTCPEndPoint( XTCPSock* inSock, VTCPSelectIOPool* vtcpSIOPool );
	
	virtual void SetIsBlocking ( bool inIsBlocking );
	virtual bool IsBlocking ( ) { return fIsBlocking; }
	virtual bool IsSelectIO ( ) { return fIsSelectIO; }
	virtual void SetIsSelectIO ( bool bIsSelectIO );
	VTCPSelectIOPool* GetSelectIOPool ( ) { return fSIOPool; }
	void			SetSelectIOPool (VTCPSelectIOPool *inSelectIOPool);
	
	/* When selectio I/O mode is enabled, end-point does not automatically uses it but rather waits a certain
	 period of time if there is no data to read. ::SetSelectIODelay method allows to control this delay.
	 Default value is 3000 milliseconds. */
	virtual void SetSelectIODelay ( uLONG inDelay ) { fSelectIOInactivityTimeout = inDelay; }
	
	// For select I/O mode only, this will trigger inCallback() when data is ready to be read.
	// inData will be passed to the callback.
	
	VError		   SetReadCallback (CTCPSelectIOHandler::ReadCallback *inCallback , void *inData);
	
	// It is a good practice to disable callback as soon as not needed.
	// DisableReadCallback() must not be called inside callback.
	
	VError		   DisableReadCallback ();
	
	
	//Needed because we inherit from VEndPoint
	virtual VError Read ( void *outBuff, uLONG *ioLen) { return ReadWithTimeout(outBuff, ioLen, 0); }
	virtual VError Write ( void *inBuff, uLONG *ioLen, bool inWithEmptyTail=false) { return WriteWithTimeout(inBuff, ioLen, 0, inWithEmptyTail); }

	//inTimeoutMs : if 0 means blocking or not blocking according to socket mode, else timeout is honored and socket mode is changed and restored if needed
	virtual VError ReadWithTimeout ( void *outBuff, uLONG *ioLen, sLONG inTimeoutMs);
	virtual VError WriteWithTimeout ( void *inBuff, uLONG *ioLen, sLONG inTimeoutMs, bool inWithEmptyTail=false);
	
	//inTimeOutMillis : if 0 means blocking, whatever the socket blocking mode (*Exactly*)
	virtual VError ReadExactly(void *outBuff, uLONG inLen, sLONG inTimeOutMillis=0);
	virtual VError WriteExactly(const void *inBuff, uLONG inLen, sLONG inTimeOutMillis=0);

	//Helps to wait (block) on message queue AND network read at the same time
	virtual VError NotifyReadWithMessage(VMessage* inMsg=NULL);

	virtual VError Close ( );
	virtual VError ForceClose ( );
	
	PortNumber GetPort() { return fPort; }
	void GetIP ( XBOX::VString& outIP );	
	VString GetIP() const;

	virtual bool IsSSL ( );
	virtual Socket GetRawSocket ( ) const;
	
	virtual sLONG GetSimpleID ( ) const { return fSimpleID; }
	
	virtual void EnableCriticalErrorSignal ( ) { fSignalCriticalError = true; }
	virtual void DisableCriticalErrorSignal ( ) { fSignalCriticalError = false; }
	virtual bool IsSignalingCriticalError ( ) { return fSignalCriticalError; }
	
	/* Reconnection */
	VError Postpone ( );
	bool IsPostponed ( );
	
	void GetSessionID ( VString& outID ) { outID. FromString ( fStringID ); }
	void SetClientSession ( VTCPClientSession* inSession );
	VTCPClientSession* GetClientSession ( ) { return fClientSession; }
	
	virtual VError EnableAutoReconnect ( );
	virtual VError DisableAutoReconnect ( );
	virtual bool AutoReconnectIsEnabled ( ) { return fIsInAutoReconnect; }
	
	virtual uLONG GetIdleTimeout ( ) { return fIdleTimeout; }
	virtual void SetIdleTimeout ( uLONG inIdleTimeout ) { fIdleTimeout = inIdleTimeout; }
	virtual bool IsIdleTimedOut ( );
	
	virtual uLONG GetPostponeTimeout ( ) { return fPostponeTimeout; }
	virtual void SetPostponeTimeout ( uLONG inPostponeTimeout ) { fPostponeTimeout = inPostponeTimeout; }
	virtual bool IsPostponeTimedOut ( );
	
	virtual bool TryToUse ( );
	virtual VError Use ( );
	virtual VError UnUse ( );
	
	virtual VError StartKeepAlive ( VTCPServerSession* inSession );
	virtual VError StopKeepAlive ( );
	
	XTCPSock* DetachSocket();
	void AttachSocket(XTCPSock* inEndPoint);
	
	void SetID ( sLONG inID ) { fID = inID; }
	sLONG GetID ( ) { return fID; }
	
	bool HasUnreadData( uLONG inTimeoutMillis = 0 );
	
	// Update a "normal" socket to SSL. If inDoHandshake is true, then the handshake is initiated 
	// (using SSL_connect()). This is to be used when a "promotion to SSL" is requested by client 
	// to server. Otherwise, set inDoHandshake to false and socket (server) will passively wait 
	// for peer (client) to start the negociation.
	
    VError  PromoteToSSL(bool inDoHandshake, XBOX::VKeyCertChain* inClientKeyCertChain=NULL);
	VError	PromoteToSSL(bool inDoHandshake, const XBOX::VMemoryBuffer<>& inCertBuffer, const XBOX::VMemoryBuffer<>& inKeyBuffer);

    
	// Do a fSock->Read(), to be called when by "watch" action callbacks using select I/O.
	VError	DirectSocketRead (void *outBuff, uLONG *ioLen);			
	
	// Do a fSock->ReadWithTimeout(), to be called by synchronous socket read.	
	VError	DirectSocketRead (void *outBuff, uLONG *ioLen, sLONG inTimeOut);
	
	// If set to no delay (this is the default behavior when sockets are initialized),
	// then Nagle algorithm is disabled: Data to be sent isn't buffered (as much).
	XBOX::VError SetNoDelay (bool inYesNo);
	
	VSslDelegate* GetSSLDelegate();

private :

	virtual VError DoRead(void *outBuff, uLONG *ioLen, sLONG inTimeoutMs=0, sLONG* outMsSpent=NULL);
	virtual VError DoReadExactly(void *outBuff, uLONG* ioLen, sLONG inTimeOutMillis=0);
	
	virtual VError DoWrite(void *inBuff, uLONG *ioLen, sLONG inTimeoutMs=0, sLONG* outMsSpent=NULL, bool inWithEmptyTail=false);
	virtual VError DoWriteExactly(const void *inBuff, uLONG* ioLen, sLONG inTimeOutMillis=0);

	static bool DoNotifyReadCallback(Socket /*inRawSocket*/, VEndPoint* inEndPoint, void *inData, sLONG /*inErrorCode*/);

	virtual VError DoClose ( );
	virtual VError DoForceClose ( );


protected:
	
	VString											fStringID;
	XTCPSock*										fSock;

	VString											fIP;
	PortNumber										fPort;
	bool											fIsSSL;
	bool											fIsBlocking;
	bool											fIsSelectIO;
	bool											fIsInAutoReconnect;
	bool											fIsInUse; /* Use/UnUse for reconnect */
	sLONG											fSimpleID;
	
	bool											fIsWatching;
	
	virtual ~VTCPEndPoint ( );
	void Init ( );
	Boolean ClientConnectionIsDropped ( long inSocketError, long inSystemError );
	void SignalCriticalError ( VError inError );
	VError GetSystemReadError ( sLONG inSocketError );
	VError _Read_SelectIO ( void *outBuff, uLONG *ioLen, uLONG inTimeOutMillis = 0 );
	VError _ReadExactly_SelectIO ( void *outBuff, uLONG inLen, uLONG inTimeOutMillis = 0 );
	bool WaitForInput ( uLONG inTimeout ); // Returns true if there is something to read, otherwise returns false
	
	VError ReportError(VError inErr, bool needThrow=true, bool isCritical=true);
	
	
	VTCPSelectIOPool*								fSIOPool;
	CTCPSelectIOHandler*							fSIOHandler;
	
	uLONG											fLastNonZeroSelectIORead; // Milliseconds
	uLONG											fSelectIOInactivityTimeout; // Milliseconds
	
	/* If zero then never times out. If non-zero then session manager may disconnect this end point
	 and postpone the session for reconnection later if the end point is idle longer than a timeout value. */
	uLONG											fIdleTimeout; // Milliseconds
	VCriticalSection								fUsageMutex;
	bool											fWasUsedAtLeastOnce;
	VTime											fIdleStart; // The time at which this end point was "unused" (marked as idle)
	uLONG											fPostponeTimeout; // Milliseconds
	VTime											fPostponeStart; // The time at which this end point was postponed
	VTCPClientSession*								fClientSession;
	
	bool											fSignalCriticalError;
	bool											fShouldStop;
	
	sLONG											fID;
};


class XTOOLBOX_API ReadNotificationMessage : public VMessage
{

public:

	ReadNotificationMessage(VTCPEndPoint* inEndPoint);

protected:

	VTCPEndPoint* fEndPoint;
	
	~ReadNotificationMessage();

	virtual void DoExecute();

	virtual OsType GetCoalescingSignature() const;

	virtual bool DoCoalesce(const VMessage& inNewMessage);
};



class XTOOLBOX_API VTCPEndPointFactory : public VObject
{
	public :
	
	/* Can pass inSelectIOPool=null if select IO is not needed */
	static VTCPEndPoint* CreateClientConnection(VString const & inDNSNameOrIP, PortNumber inPort,
												bool inIsSSL, bool inIsBlocking, sLONG inTimeOutMillis,
												VTCPSelectIOPool* inSelectIOPool, VError& outError );
	
	/* A helper method to automate session ID generation. */
	static void GenerateSessionID ( XBOX::VString& outSessionID );
	
	private :

	static VTCPEndPoint* DoCreateClientConnection(VString const & inDNSNameOrIP, PortNumber inPort,
												  bool inIsSSL, bool inIsBlocking, sLONG inTimeOutMillis,
												  VTCPSelectIOPool* inSelectIOPool, VError& outError );
	
	
};


END_TOOLBOX_NAMESPACE


#endif


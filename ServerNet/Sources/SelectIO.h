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


#ifndef __SNET_SELECT_IO__
#define __SNET_SELECT_IO__


BEGIN_TOOLBOX_NAMESPACE


class VSslDelegate;


class XTOOLBOX_API CTCPSelectIOHandler
{
	public :
	
	// Definition of callback that will be triggered by select I/O when data is available for read.
	//
	//	+ inRawSocket is the "raw" (Winsock or POSIX) socket, it is not recommended to use directly;
	//	+ inEndPoint is the endpoint with data available;
	//	+ inData is the pointer passed to AddSocketForWatching();
	//	+ inErrorCode is the error code, should be zero if data is readily available for read.
	//
	// Return true to allow callback to be called again, or false (if error) to stop.
	//
	// Callback shouldn't do much processing except reading the socket and notifying that data is 
	// now available. When reading the socket, care must still be taken to check for errors. This can 
	// be used to detect errors. 
	//
	// When done, callback must always be explicitely disabled, even if stopped.
	
	typedef bool	ReadCallback (Socket inRawSocket, VEndPoint* inEndPoint, void *inData, sLONG inErrorCode);
	
	virtual VError	AddSocketForReading ( Socket inRawSocket, VSslDelegate* inSSLDelegate=NULL ) = 0;
	virtual VError	Read (Socket inRawSocket, char* inBuffer, uLONG* nBufferLength, sLONG& outError, sLONG& outSystemError, uLONG inTimeOutMillis = 0 ) = 0;
	virtual VError	RemoveSocketForReading ( Socket inRawSocket ) = 0;
	
	// Setup the socket to trigger inCallback when data is available. inData will be passed along to the callback.
	
	virtual VError	AddSocketForWatching (Socket inRawSocket, VEndPoint* inEndPoint, void *inData, ReadCallback *inCallback) = 0;
	virtual VError	RemoveSocketForWatching (Socket inRawSocket) = 0;
	
	virtual void Stop ( ) = 0;
};


class XTOOLBOX_API VTCPSelectIOPool : public IRefCountable
{
	public :
	
	VTCPSelectIOPool ( );
	virtual ~VTCPSelectIOPool ( );
	
	CTCPSelectIOHandler* AddSocketForReading ( VEndPoint* inEndPoint, VError& outError );
	CTCPSelectIOHandler* AddSocketForWatching (VEndPoint* inEndPoint, void *inData, CTCPSelectIOHandler::ReadCallback *inCallback, VError& outError);
	
	VError Close ( );
	
private:
	
	std::list<CTCPSelectIOHandler*>			fHandlerList;
	VCriticalSection						fHandlersLock;
	
	// Set a "watch" if inCallback is not NULL, otherwise read socket.
	
	CTCPSelectIOHandler	*_AddSocket (VEndPoint *inEndPoint, void *inData, CTCPSelectIOHandler::ReadCallback *inCallback, VError& outError);
};


class XTOOLBOX_API VTCPSelectAction : public XBOX::VObject, public XBOX::IRefCountable
{
	public :

		enum {

			eTYPE_READ,
			eTYPE_WATCH

		};

virtual	sLONG		GetType () = 0;

		Socket		GetRawSocket ()							{	return fSock;	}

		void		SetTimeOut (uLONG inTimeOutMillis)		{	fExpirationTime = !inTimeOutMillis ? 0 : VSystem::GetCurrentTime() + inTimeOutMillis;	}
		bool		TimeOutExpired ()						{	return !fExpirationTime ? false : VSystem::GetCurrentTime() > fExpirationTime;	}

		void		SetLastSocketError(sLONG nError)		{	fLastSocketError = nError;	}
		sLONG		GetLastSocketError ()					{	return fLastSocketError; }

		void		SetLastSystemSocketError (sLONG nError)	{	fLastSystemSocketError = nError;	}
		sLONG		GetLastSystemSocketError ()				{	return fLastSystemSocketError;	}

		void		SetLastError (VError vError)			{	fLastError = vError;	}
		VError		GetLastError ()							{	return fLastError;	}

		static bool HasSameRawSocket (VTCPSelectAction* vtcpSelectAction, Socket nRawSocket)	{ return vtcpSelectAction-> GetRawSocket ( ) == nRawSocket; }
		static bool Delete (VTCPSelectAction* vtcpSelectAction);

virtual void		DoAction (fd_set* fdSockets) = 0;
virtual void		HandleError (fd_set* fdSockets) = 0;

	protected:

					VTCPSelectAction (Socket inSocket);
		virtual		~VTCPSelectAction ();

	private:

		Socket		fSock;
		uLONG		fExpirationTime;
		sLONG		fLastSocketError;
		sLONG		fLastSystemSocketError;
		VError		fLastError;
};


class XTOOLBOX_API VTCPSelectReadAction : public VTCPSelectAction
{
	public :

	VTCPSelectReadAction (Socket inSocket, char* inBuffer, uLONG* inBufferSize, VSslDelegate* inSSLDelegate=NULL);

virtual sLONG	GetType ()									{	return VTCPSelectAction::eTYPE_READ;	}
		
		char	*GetBuffer ()								{	return fBuffer;	}
		void	SetBuffer (char* inBuffer)					{	fBuffer = inBuffer;	}

		uLONG*	GetFullBufferSize ()						{	return fBufferSize;	}
		void	SetFullBufferSize (uLONG* inBufferSize)		{	fBufferSize = inBufferSize;	}
		void	UpdateFullBufferSize (uLONG inBufferSize)	{	*fBufferSize = inBufferSize;	}

		// uLONG GetProcessedBufferSize ( ) { return ?; }

		bool	IsProcessed ();
		void	SetProcessed (bool bProcessed);

		bool	WaitForAction ();
		bool	NotifyActionComplete ();

virtual void	DoAction (fd_set* fdSockets);
virtual void	HandleError (fd_set* fdSockets);

	protected:

virtual			~VTCPSelectReadAction()	{}

	private:

		char*				fBuffer;
		uLONG*				fBufferSize;
		VSyncEvent			fSyncEvtProcessed;
		bool				fIsProcessed;
		VCriticalSection	fProcessedLock;
		VSslDelegate*		fSslDelegate;
};


class XTOOLBOX_API VTCPSelectWatchAction : public VTCPSelectAction
{
	public :

				VTCPSelectWatchAction (Socket inSocket, 
										VEndPoint *inEndPoint, 
										void *inData, 
										CTCPSelectIOHandler::ReadCallback *inCallback);

virtual sLONG	GetType ()					{	return VTCPSelectAction::eTYPE_WATCH;	}

		bool	TriggerReadCallback (sLONG inErrorCode);

virtual void	DoAction (fd_set* fdSockets);
virtual void	HandleError (fd_set* fdSockets);

	protected:

virtual			~VTCPSelectWatchAction ()	{}

	private:

		VEndPoint							*fEndPoint;
		void								*fData;
		CTCPSelectIOHandler::ReadCallback	*fCallback;		
};


class XTOOLBOX_API VTCPSelectIOHandler : public CTCPSelectIOHandler, public VTask
{
	public :

						VTCPSelectIOHandler ( );
		virtual			~VTCPSelectIOHandler ( );

		virtual VError	AddSocketForReading ( Socket nRawSocket, VSslDelegate* inSSLDelegate=NULL );
		virtual VError	Read ( Socket inRawSocket, char* inBuffer, uLONG* nBufferLength, sLONG& outError, sLONG& outSystemError, uLONG inTimeOutMillis = 0 );
		virtual VError	RemoveSocketForReading ( Socket inRawSocket );

		virtual VError	AddSocketForWatching (Socket inRawSocket, VEndPoint *inEndPoint, void *inData, CTCPSelectIOHandler::ReadCallback *inCallback);
		virtual VError	RemoveSocketForWatching (Socket inRawSocket);

		virtual void	Stop ( );

		static sLONG	GetLastSocketError ( );

	protected :

		 virtual Boolean DoRun ( );

	private :

		std::list<XBOX::VRefPtr<VTCPSelectAction> >		fReadSockList;
		VCriticalSection								fReadSockLock;

		fd_set											fReadSockSet;
		sLONG8											fReadCount;

		static void AddToFDSet ( VTCPSelectAction* vtcpSelectAction, fd_set* fdSockets );
		static void HandleRead ( VTCPSelectAction* vtcpSelectAction, fd_set* fdSockets );
		static void HandleError ( VTCPSelectAction* vtcpSelectAction, fd_set* fdSockets );

		sLONG GetActiveReadCount ( );
};


END_TOOLBOX_NAMESPACE


#endif

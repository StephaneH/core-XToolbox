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
#ifndef __VJS_WEB_SOCKET__
#define __VJS_WEB_SOCKET__

#include "ServerNet/VServerNet.h"
#include "VJSClass.h"
#include "VJSValue.h"
#include "VJSBuffer.h"

BEGIN_TOOLBOX_NAMESPACE

class VJSWorker;

class XTOOLBOX_API VJSWebSocketObject : public XBOX::VObject, public XBOX::IRefCountable
{
public:
								
								VJSWebSocketObject (bool inIsSynchronous, VJSWorker *inWorker, const XBOX::VJSContext &inContext);
								VJSWebSocketObject (XBOX::VJSParms_construct &ioParms, bool inIsSynchronous, VJSWorker *inWorker);
								VJSWebSocketObject (XBOX::VWebSocket *inWebSocket, XBOX::VWebSocketMessage *inWebSocketMessage, const XBOX::VJSContext &inContext);
	virtual						~VJSWebSocketObject ();

	static void					Initialize (const XBOX::VJSParms_initialize &inParms, VJSWebSocketObject *inWebSocket);
	static void					Finalize (const XBOX::VJSParms_finalize &inParms, VJSWebSocketObject *inWebSocket);
	static void					GetProperty (XBOX::VJSParms_getProperty &ioParms, VJSWebSocketObject *inWebSocket);
	static bool					SetProperty (XBOX::VJSParms_setProperty &ioParms, VJSWebSocketObject *inWebSocket);

	static void					Construct(XBOX::VJSParms_construct &ioParms, bool inIsSynchronous);

	static void					Close (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebSocketObject *inWebSocket);	
	static void					Send (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebSocketObject *inWebSocket);	
	static void					Receive (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebSocketObject *inWebSocket);

	sLONG						GetState () const;

	// Neutralize the websocket, its JavaScript (if any) can no longer be used. Set new (dedicated worker) and set object to global.

	VJSWebSocketObject			*RetainAndNeutralize (XBOX::VJSObject &inGlobalObject, XBOX::VJSWorker *inWorker);

	bool						IsDedicatedWorker () const	{	return fIsDedicatedWorker;	}
	const XBOX::VJSObject		&GetObject () const			{	return fObject;	}

	// Initiate closing handshake. If synchronous, then also reads answers (blocking).
	
	XBOX::VError				Close (const uBYTE *inReason, VSize inReasonLength, bool *outIsOk);

	// Read callback for asynchronous operation.

	static bool					ReadCallback (Socket inRawSocket, VEndPoint* inEndPoint, void *inData, sLONG inErrorCode);

private:
								
friend class VJSWebSocketClass;

	static const VSize			BUFFER_SIZE				= 1 << 16;	// Buffer used by asynchronous handshake.
	static const VSize			MAXIMUM_REASON_LENGTH	= 123;
	static const VSize			ADDITIONAL_BYTES		= 10;		// To detect if the UTF-8 encoded string is too long.

	XBOX::VCriticalSection		fMutex;

	bool						fIsNeutralized;
	bool						fIsDedicatedWorker;
	bool						fIsSynchronous;
	VJSWorker					*fWorker;
	XBOX::VJSObject				fObject;
 
	XBOX::VWebSocketConnector	*fWebSocketConnector;
	XBOX::VWebSocket			*fWebSocket;
	XBOX::VWebSocketMessage		*fWebSocketMessage;
	XBOX::VString				fProtocol;		// After connection, protocol and extension(s) selected by server.
	XBOX::VString				fExtensions;

	// Initiate opening handshake. If synchronous, then also reads answers (blocking).

	XBOX::VError				_Connect (const XBOX::VString &inURL, const XBOX::VString &inSubProtocols);
};

class XTOOLBOX_API VJSWebSocketClass : public XBOX::VJSClass<VJSWebSocketClass, VJSWebSocketObject>
{	
public:

	static void					GetDefinition (ClassDefinition &outDefinition);
	static XBOX::VJSObject		MakeConstructor (const XBOX::VJSContext &inContext);
	static XBOX::VJSValue		MakeObject (XBOX::VJSContext &inContext, XBOX::VWebSocket *inWebSocket, VJSWorker *inWorker, bool inSetSelectIO);

private: 

	static void					_Construct(XBOX::VJSParms_construct &ioParms);

	// Simulate EventTarget interface addEventListener() and removeEventListener() methods.
	// _AddEventListener() updates callback properties by prefixing the event wihth "on", example: "close" modifies the "onclose" property.
	// _RemoveEventListener() deletes the event property.
	
	static void					_AddEventListener (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebSocketObject *inWebSocket);
	static void					_RemoveEventListener (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebSocketObject *inWebSocket);
};

class XTOOLBOX_API VJSWebSocketSyncClass : public XBOX::VJSClass<VJSWebSocketSyncClass, VJSWebSocketObject>
{
public:

	static void					GetDefinition (ClassDefinition &outDefinition);
	static XBOX::VJSObject		MakeConstructor (const XBOX::VJSContext &inContext);

private: 

	static void					_Construct(XBOX::VJSParms_construct &ioParms);

};

class VJSWebSocketServerObject;

class XTOOLBOX_API VJSWebSocketHandler : public XBOX::VObject, public XBOX::IRefCountable
{
public:

	// Functions to add/remove WebSocket handlers to HTTP server.
	
	static void						AddHandler (XBOX::VJSParms_callStaticFunction &ioParms, void *);
	static void						RemoveHandler (XBOX::VJSParms_callStaticFunction &ioParms, void *);

	// Clear all the web socket handlers. This function is to be called only by the runtime delegate of the project.
	// It (the runtime delegate of the project) will then remove all handlers from the server.

	static void						RemoveAllhandlers ();

	// Connection handler (to be called by HTTP server).

	static XBOX::VError				ConnectionHandler (XBOX::VJSContext &inContext, XBOX::VWebSocket *inWebSocket, void *inUserData);

private:

	typedef std::map<XBOX::VString, VRefPtr<VJSWebSocketHandler> >	HandlerMap;

									VJSWebSocketHandler (
										const XBOX::VString &inPath, 
										const XBOX::VString &inId,										
										const XBOX::VString &inScriptURL,
										bool inIsSharedWorker);
	virtual							~VJSWebSocketHandler ();

	static XBOX::VCriticalSection	sMutex;
	static HandlerMap				sHandlers;

	XBOX::VCriticalSection			fMutex;
	XBOX::VString					fPath;
	XBOX::VString					fId;			// Also used as the "name" of the shared worker
	XBOX::VString					fScriptURL;	
	bool							fIsSharedWorker;
	
	XBOX::VError					_ConnectionHandler (XBOX::VJSContext &inContext, XBOX::VWebSocket *inWebSocket);
};

class XTOOLBOX_API VJSWebSocketServerObject : public XBOX::VObject, public XBOX::IRefCountable
{
public:

								VJSWebSocketServerObject (const XBOX::VJSContext &inContext, 
									bool inIsSSL,
									XBOX::VString &inAddress, sLONG inPort, const XBOX::VString &inPath);
	virtual						~VJSWebSocketServerObject();

private:

friend class VJSWebSocketServerClass;

	static const sLONG			sTIME_OUT	= 100;

	enum STATES {

		STATE_NOT_READY,	// Listening has not been launched yet.
		STATE_READY,		// Ready, waiting for start() command (locked in VSyncEvent).
		STATE_RUNNING,		// Listener is running and accepting connections.
		STATE_CLOSED,		// Server is closed.
	
	};

	sLONG						fState;

	XBOX::VCriticalSection		fMutex; 
	XBOX::VSyncEvent			fSyncEvent;

	XBOX::VTask					*fListeningTask;
	XBOX::VWebSocketListener	*fListener;	
	VJSWorker					*fWorker;
	XBOX::VJSObject				fObject;

	static sLONG				_RunProc (XBOX::VTask *inVTask);
	void						_DoRun ();
	void						_KillListeningTask ();
};

class XTOOLBOX_API VJSWebSocketServerClass : public XBOX::VJSClass<VJSWebSocketServerClass, VJSWebSocketServerObject>
{
public:

	static void					GetDefinition (ClassDefinition &outDefinition);
	static XBOX::VJSObject		MakeConstructor (const XBOX::VJSContext &inContext);
							
private:

	static void					_Construct(XBOX::VJSParms_construct &ioParms);

	static void					_Initialize (const XBOX::VJSParms_initialize &inParms, VJSWebSocketServerObject *inWebSocketServer);
	static void					_Finalize (const XBOX::VJSParms_finalize &inParms, VJSWebSocketServerObject *inWebSocketServer);

	static void					_Start (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebSocketServerObject *inWebSocketServer);
	static void					_Close (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebSocketServerObject *inWebSocketServer);
};

END_TOOLBOX_NAMESPACE

#endif
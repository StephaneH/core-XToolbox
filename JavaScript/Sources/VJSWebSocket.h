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
#include "VJSWorker.h"
#include "VJSEventEmitter.h"
#include "VJSBuffer.h"

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API VJSWebSocketObject : public XBOX::VObject
{
public:

								VJSWebSocketObject (bool inIsSynchronous, VJSWorker *inWorker);
	virtual						~VJSWebSocketObject ();

	static void					Initialize (const XBOX::VJSParms_initialize &inParms, VJSWebSocketObject *inWebSocket);
	static void					Finalize (const XBOX::VJSParms_finalize &inParms, VJSWebSocketObject *inWebSocket);
	static void					GetProperty (XBOX::VJSParms_getProperty &ioParms, VJSWebSocketObject *inWebSocket);
	static bool					SetProperty (XBOX::VJSParms_setProperty &ioParms, VJSWebSocketObject *inWebSocket);

	static void					Construct (XBOX::VJSParms_callAsConstructor &ioParms, bool inIsSynchronous);	

	static void					Close (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebSocketObject *inWebSocket);	
	static void					Send (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebSocketObject *inWebSocket);	
	static void					Receive (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebSocketObject *inWebSocket);

	sLONG						GetState () const;
	void						SetObjectRef (XBOX::JS4D::ObjectRef inObjectRef)	{	fObjectRef = inObjectRef;	}

private:

	static const VSize			BUFFER_SIZE				= 1 << 16;	// Buffer used by asynchronous handshake.
	static const VSize			MAXIMUM_REASON_LENGTH	= 123;
	static const VSize			ADDITIONAL_BYTES		= 10;		// To detect if the UTF-8 encoded string is too long.

	XBOX::VCriticalSection		fMutex;

	bool						fIsSynchronous;
	VJSWorker					*fWorker;
	XBOX::JS4D::ObjectRef		fObjectRef;

	XBOX::VWebSocketConnector	*fWebSocketConnector;
	XBOX::VWebSocket			*fWebSocket;
	XBOX::VWebSocketMessage		*fWebSocketMessage;
	XBOX::VString				fProtocol;		// After connection, protocol and extension(s) selected by server.
	XBOX::VString				fExtensions;

	XBOX::VMemoryBuffer<>		fBuffer;

	// Initiate opening or closing handshakes. If synchronous, then also reads answers (blocking).

	XBOX::VError				_Connect (const XBOX::VString &inURL, const XBOX::VString &inSubProtocols);
	XBOX::VError				_Close (const uBYTE *inReason, VSize inReasonLength, bool *outIsOk);

	// Read callback for asynchronous operation.

	static bool					_ReadCallback (Socket inRawSocket, VEndPoint* inEndPoint, void *inData, sLONG inErrorCode);
};

class XTOOLBOX_API VJSWebSocketClass : public XBOX::VJSClass<VJSWebSocketClass, VJSWebSocketObject>
{	
public:

	static void					GetDefinition (ClassDefinition &outDefinition);
	static XBOX::VJSObject		MakeConstructor (const XBOX::VJSContext &inContext);

private: 

	static void					_Construct (XBOX::VJSParms_callAsConstructor &ioParms);

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

	static void					_Construct (XBOX::VJSParms_callAsConstructor &ioParms);	
};

class XTOOLBOX_API VJSWebSocketServerObject : public XBOX::VObject, public XBOX::IRefCountable
{
public:

								VJSWebSocketServerObject ();
	virtual						~VJSWebSocketServerObject ();

private:

friend class VJSWebSocketServerClass;

	enum STATES {

		STATE_READY,		// Ready, waiting for start() command (locked in VSyncEvent).
		STATE_RUNNING,		// Listener is running accepting connections.
		STATE_CLOSED,		// Listening has finished.
	
	};

	sLONG						fState;
	XBOX::VCriticalSection		fMutex;
	XBOX::VSyncEvent			fSyncEvent;

	XBOX::VTask					*fListeningTask;
	XBOX::VWebSocketListener	*fListener;	
	VJSWorker					*fWorker;

	static sLONG				_RunProc (XBOX::VTask *inVTask);
	void						_DoRun ();
	void						_KillListeningTask ();
};

class XTOOLBOX_API VJSWebSocketServerClass : public XBOX::VJSClass<VJSWebSocketServerClass, VJSWebSocketServerObject>
{
public:

	static void				GetDefinition (ClassDefinition &outDefinition);
	static XBOX::VJSObject	MakeConstructor (const XBOX::VJSContext &inContext);
							
private:

	static void				_Construct (XBOX::VJSParms_callAsConstructor &ioParms);

	static void				_Initialize (const XBOX::VJSParms_initialize &inParms, VJSWebSocketServerObject *inWebSocketServer);
	static void				_Finalize (const XBOX::VJSParms_finalize &inParms, VJSWebSocketServerObject *inWebSocketServer);

	static void				_Start (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebSocketServerObject *inWebSocketServer);
	static void				_Close (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebSocketServerObject *inWebSocketServer);
};

END_TOOLBOX_NAMESPACE

#endif
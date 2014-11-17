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
#ifndef __VJS_EVENT__
#define __VJS_EVENT__

#include "VJSClass.h"
#include "VJSValue.h"

BEGIN_TOOLBOX_NAMESPACE

class VTCPEndPoint;
class VJSGlobalObject;
class VJSWorker;
class VJSMessagePort;
class VJSTimer;
class VJSStructuredClone;
class VJSEventEmitter;
class VJSNetSocketObject;
class VJSNetServerObject;
class VJSLocalFileSystem;
class VJSEntry;
class VJSDirectoryReader;
class VJSSystemWorker;
class VJSWebSocketObject;
class VWebSocket;

// Worker event interface.

class XTOOLBOX_API IJSEvent : public XBOX::VObject, public XBOX::IRefCountable
{
public:

	enum {

		eTYPE_MESSAGE,				// Web worker "onmessage".
		eTYPE_CONNECTION,			// Shared worker "onconnect".
		eTYPE_ERROR,				// Web worker "onerror".
		eTYPE_TERMINATION,			// Internal use: web worker (dedicated or shared) termination.

		eTYPE_TIMER,				// Timer (setTimeout() and setInterval()) callbacks.
		eTYPE_SYSTEM_WORKER,		// System worker callbacks.

		eTYPE_NET,					// Events for interoperability with NodeJS net object.

		eTYPE_W3C_FS,				// Events for W3C File System API.

		eTYPE_EVENT_EMITTER,		// Implements the "newListener" event.

		eTYPE_WEB_SOCKET,			// Events for WebSocket object.
		eTYPE_WEB_SOCKET_SERVER,	// Events for WebSocketServer object.

		eTYPE_WEB_SOCKET_CONNECT,	// Shared worker WebSocket "onconnect".

		eTYPE_CALLBACK				// Generic callback.
				
	};

	// Return event type.

	uLONG			GetType ()			const	{	return fType;	}

	// Get time at which event is to be triggered.

	const VTime		&GetTriggerTime ()	const	{	return fTriggerTime;	}

	// Process event.

	virtual void	Process (XBOX::VJSContext inContext, VJSWorker *inWorker) = 0;

	// Discard event. Used only to empty event queue when terminating a worker.

	virtual void	Discard () = 0;

	// Insert an event in an event queue, return true if it's been inserted at first position.
	
	static bool		InsertEvent (std::list<IJSEvent *> *ioEventQueue, IJSEvent *inEvent);

protected:

	uLONG			fType;
	XBOX::VTime		fTriggerTime;

					IJSEvent ()	{}
	virtual			~IJSEvent()	{}
};

// Interface to an event generator.

class XTOOLBOX_API IJSEventGenerator : public XBOX::VObject, public XBOX::IRefCountable
{
public:

	// Check if the event generator is running. If it hasn't started yet, it is still considered as running.
	// If it then fails to start, it will be marked as not running. Set inWaitForStartup to true to actually 
	// wait for event generator to either succeed or fail startup.

	virtual bool	IsRunning (bool inWaitForStartup = false)				= 0;

	// If the event generator is running, stop it.

	virtual void	Kill ()													= 0;

	// Return true if inEvent match the object implementing IJSEventGenerator and if it is of type inEventType.

	virtual bool	MatchEvent (const IJSEvent *inEvent, sLONG inEventType)	= 0;
};

// Message event (web workers).

class XTOOLBOX_API VJSMessageEvent : public XBOX::IJSEvent
{
public:

	// Create an event for a message triggering an "onmessage" callback.

	static VJSMessageEvent	*Create (VJSMessagePort *inMessagePort, VJSStructuredClone *inMessage);

	void					Process (XBOX::VJSContext inContext, VJSWorker *inWorker);
	void					Discard ();

	// Remove all messages using given message port.

	static void				RemoveMessageFrom (std::list<IJSEvent *> *ioEventQueue, VJSMessagePort *inMessagePort);
	
private:

	VJSMessagePort			*fMessagePort;		// Message port to receive message.
	VJSStructuredClone		*fData;				// Structured clone of data to send.

							VJSMessageEvent ()	{}
	virtual					~VJSMessageEvent ()	{}
};

// Connection event (shared workers).

class XTOOLBOX_API VJSConnectionEvent : public XBOX::IJSEvent
{
public:

	// Create an event to connect with a shared worker. inMessagePort must be the unidirectional port for "onconnect".

	static VJSConnectionEvent	*Create (VJSMessagePort *inConnectionPort, VJSMessagePort *inMessagePort, VJSMessagePort *inErrorPort);

	void						Process (XBOX::VJSContext inContext, VJSWorker *inWorker);
	void						Discard ();
	
private:

	VJSMessagePort				*fConnectionPort;		// Unidirectional "onconnect" message port to connect shared worker with "outside".
	VJSMessagePort				*fMessagePort;			// Message port that will be used for communication with shared worker.
	VJSMessagePort				*fErrorPort;			// Error port that will be used to report errors.

								VJSConnectionEvent ()	{}
	virtual						~VJSConnectionEvent ()	{}
};

// Error event (web workers).

class XTOOLBOX_API VJSErrorEvent : public XBOX::IJSEvent
{
public:

	// Create an event for an error.

	static VJSErrorEvent	*Create (VJSMessagePort *inMessagePort, 
										const XBOX::VString &inMessage, 
										const XBOX::VString &inFileName, 
										sLONG inLineNumber);

	void					Process (XBOX::VJSContext inContext, VJSWorker *inWorker);
	void					Discard ();
	
private:

	VJSMessagePort			*fMessagePort;		// Message port to receive message.
	XBOX::VString			fMessage;
	XBOX::VString			fFileName;
	sLONG					fLineNumber;
	
							VJSErrorEvent ()	{}
	virtual					~VJSErrorEvent ()	{}
};

// Termination event of dedicated or shared worker.

class XTOOLBOX_API VJSTerminationEvent : public XBOX::IJSEvent
{
public:

	// Worker will be retained.

	static VJSTerminationEvent	*Create (VJSWorker *inWorker);
	void						Process (XBOX::VJSContext inContext, VJSWorker *inWorker);
	void						Discard ();
	
private:

	VJSWorker					*fWorker;	// Must be a dedicated or shared worker.
	
								VJSTerminationEvent ()	{}
	virtual						~VJSTerminationEvent ()	{}
};

// Timer event.

class XTOOLBOX_API VJSTimerEvent : public XBOX::IJSEvent
{
public:

	// Create an event for a timer.	

	static VJSTimerEvent		*Create (VJSTimer *inTimer, XBOX::VTime &inTriggerTime, std::vector<XBOX::VJSValue> *inArguments);
										
	void						Process (XBOX::VJSContext inContext, VJSWorker *inWorker);
	void						Discard ();

	// Remove a timer event from an event queue.

	static void					RemoveTimerEvent (std::list<IJSEvent *> *ioEventQueue, VJSTimer *inTimer);

private:

	VJSTimer					*fTimer;
	std::vector<XBOX::VJSValue>	*fArguments;

								VJSTimerEvent ()	{}
	virtual						~VJSTimerEvent ()	{}
};

// System worker events.

class XTOOLBOX_API VJSSystemWorkerEvent : public XBOX::IJSEvent
{
public:

	enum {

		eTYPE_STDOUT_DATA,
		eTYPE_STDERR_DATA,
		eTYPE_TERMINATION

	};

	struct STerminationData {

		bool	fHasStarted;
		bool	fForcedTermination; 
		sLONG	fExitStatus;
		sLONG	fPID;

	};
	
	static VJSSystemWorkerEvent	*Create (VJSSystemWorker *inSystemWorker, sLONG inType, XBOX::VJSObject &inObjectRef, uBYTE *inData, sLONG inSize);
	void						Process (XBOX::VJSContext inContext, VJSWorker *inWorker);
	void						Discard ();

	sLONG						GetSubType () const			{	return fSubType;		}
	VJSSystemWorker				*GetSystemWorker () const	{	return fSystemWorker;	}

private:

	VJSSystemWorker				*fSystemWorker;	// System worker object, will be used for "event matching".
	sLONG						fSubType;		// Type of event.
	XBOX::VJSObject				fObject;		// System worker proxy object.
	uBYTE						*fData;			// Data (null terminated).
	sLONG						fSize;			// Data length (without last zero byte).
		
								VJSSystemWorkerEvent (XBOX::VJSObject &inObjectRef)	: fObject(inObjectRef)	{}
	virtual						~VJSSystemWorkerEvent ()	{}
};

// net.Socket object events.

class XTOOLBOX_API VJSNetEvent : public XBOX::IJSEvent
{
public:

	static VJSNetEvent	*Create (VJSEventEmitter *inEventEmitter, const XBOX::VString &inEventName);

	// If event is discarded, inData memory will be freed by VJSNetEvent code. Otherwise, it is to be freed by the Buffer object.
	
	static VJSNetEvent	*CreateData (VJSNetSocketObject *inSocketObject, uBYTE *inData, sLONG inSize);

	static VJSNetEvent	*CreateError (VJSEventEmitter *inEventEmitter, const XBOX::VString &inExceptionName);
	
	static VJSNetEvent	*CreateClose (VJSEventEmitter *inEventEmitter, bool inHadError);

	// When event is discarded (after process has called "connection" or "secureConnection" callback or not), inEndPoint is "released".

	static VJSNetEvent	*CreateConnection (VJSNetServerObject *inServerObject, XBOX::VTCPEndPoint *inEndPoint, bool inIsSSL);

	void				Process (XBOX::VJSContext inContext, VJSWorker *inWorker);
	void				Discard ();

private:

	enum {

		eTYPE_NO_ARGUMENT,		// Any event without argument.
		eTYPE_DATA,				// "data" event, argument is a Buffer object.
		eTYPE_ERROR,			// "error" event argument is exception name (object will be created).
		eTYPE_CLOSE,			// "close" event, boolean indicates if an error occured.
		eTYPE_CONNECTION,		// "connection" event, argument is the just accepted socket.
		eTYPE_CONNECTION_SSL,	// "secureConnection" event, argument is the just accepted SSL socket.

	};

	sLONG						fSubType;			// Type of event.
	VJSEventEmitter				*fEventEmitter;		// Object implementing EventEmitter interface.
													// For a "data" event, this must be a VJSNetSocketObject.
	
	XBOX::VString				fEventName;			// eTYPE_NO_ARGUMENT only.

	uBYTE						*fData;				// eTYPE_DATA only.
	sLONG						fSize;				

	XBOX::VString				fExceptionName;		// eTYPE_ERROR only.

	bool						fHadError;			// eTYPE_CLOSE only.

	XBOX::VTCPEndPoint			*fEndPoint;			// eTYPE_CONNECTION and eTYPE_CONNECTION_SSL only.
		
								VJSNetEvent ()	{}
	virtual						~VJSNetEvent ()	{}
};

// A generic event to trigger a callback.

class XTOOLBOX_API VJSCallbackEvent : public XBOX::IJSEvent
{
public:

	// Create an event for a callback. The event has to free memory for arguments after processing or if discarded.

	static VJSCallbackEvent		*Create (XBOX::VJSObject &inObject, const XBOX::VString &inCallbackName, std::vector<XBOX::VJSValue> *inArguments);											
	void						Process (XBOX::VJSContext inContext, VJSWorker *inWorker);
	void						Discard ();

private:

	XBOX::VJSObject				fObject;		// Object containing the callback.
	XBOX::VString				fCallbackName;	// Callback name.
	std::vector<XBOX::VJSValue>	*fArguments;	// Arguments.
	
								VJSCallbackEvent (XBOX::VJSObject &inObject): fObject(inObject)	{;}
	virtual						~VJSCallbackEvent ()	{}
};

// W3C File System API events. They are actually operation requests, executing the operation before triggering the appropriate callback (success or failure).

class XTOOLBOX_API VJSW3CFSEvent : public XBOX::IJSEvent
{
public:

	// LocalFileSystem interface operations.
	
	static VJSW3CFSEvent	*RequestFS (VJSLocalFileSystem *inLocalFileSystem, sLONG inType, VSize inQuota, const XBOX::VString &inFileSystemName, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback);
	static VJSW3CFSEvent	*ResolveURL (VJSLocalFileSystem *inLocalFileSystem, const XBOX::VString &inURL, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback);
	
	// Entry interface common operations.

	static VJSW3CFSEvent	*GetMetadata (VJSEntry *inEntry, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback);
	static VJSW3CFSEvent	*MoveTo (VJSEntry *inEntry, VJSEntry *inTargetEntry, const XBOX::VString &inNewName, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback);
	static VJSW3CFSEvent	*CopyTo (VJSEntry *inEntry, VJSEntry *inTargetEntry, const XBOX::VString &inNewName, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback);
	static VJSW3CFSEvent	*Remove (VJSEntry *inEntry, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback);
	static VJSW3CFSEvent	*GetParent (VJSEntry *inEntry, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback);

	// DirectoryEntry interface specific operations.
	
	static VJSW3CFSEvent	*GetFile (VJSEntry *inEntry, const XBOX::VString &inURL, sLONG inFlags, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback);
	static VJSW3CFSEvent	*GetDirectory (VJSEntry *inEntry, const XBOX::VString &inURL, sLONG inFlags, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback);
	static VJSW3CFSEvent	*RemoveRecursively (VJSEntry *inEntry, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback);	
	static VJSW3CFSEvent	*Folder (VJSEntry *inEntry, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback);
	
	// FileEntry interface specific operations.
	
	static VJSW3CFSEvent	*CreateWriter (VJSEntry *inEntry, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback);	
	static VJSW3CFSEvent	*File (VJSEntry *inEntry, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback);		

	// DirectoryReader interface operation.

	static VJSW3CFSEvent	*ReadEntries (VJSDirectoryReader *inDirectoryReader, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback);
	
	void					Process ( XBOX::VJSContext inContext, VJSWorker *inWorker);
	void					Discard ();

private:
	
	enum {

		eOPERATION_REQUEST_FILE_SYSTEM,
		eOPERATION_RESOLVE_URL,
		
		eOPERATION_GET_METADATA,
		eOPERATION_MOVE_TO,
		eOPERATION_COPY_TO,
		eOPERATION_REMOVE,
		eOPERATION_GET_PARENT,

		eOPERATION_GET_FILE,
		eOPERATION_GET_DIRECTORY,		
		eOPERATION_REMOVE_RECURSIVELY,
		eOPERATION_FOLDER,
		
		eOPERATION_CREATE_WRITER,
		eOPERATION_FILE,

		eOPERATION_READ_ENTRIES,	// DirectoryReader implementation.
		
	};

	sLONG					fSubType;
	XBOX::VJSObject			fSuccessCallback;
	XBOX::VJSObject			fErrorCallback;
	
	VJSLocalFileSystem		*fLocalFileSystem;	// eOPERATION_REQUEST_FS or eOPERATION_RESOLVE_URL only.
	VJSEntry				*fEntry;			// All other operations on entries.
	VJSDirectoryReader		*fDirectoryReader;	// eOPERATION_READ_ENTRIES only.

	// Operations arguments.	
	
	sLONG					fType;
	VSize					fQuota;

	VJSEntry				*fTargetEntry;		
	
	XBOX::VString			fURL;				// URL or new name.
	XBOX::VString			fFileSystemName;	// For NAMED_FS type.
	sLONG					fFlags;	
	
							VJSW3CFSEvent (sLONG inSubType, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback);
};

// EventEmitter's "newListener" event.

class XTOOLBOX_API VJSNewListenerEvent : public XBOX::IJSEvent
{
public:

	static VJSNewListenerEvent	*Create (VJSEventEmitter *inEventEmitter, const XBOX::VString &inEvent, XBOX::VJSObject& inListener);
	void						Process (XBOX::VJSContext inContext, VJSWorker *inWorker);
	void						Discard ();

private:
	
	VJSEventEmitter				*fEventEmitter;		// EventEmitter object.
	XBOX::VString				fEvent;				// Name of the event having a newly added listener.
	XBOX::VJSObject				fListener;			// Listener of the event.
	
								VJSNewListenerEvent (XBOX::VJSObject& inListener) : fListener(inListener)	{}
	virtual						~VJSNewListenerEvent ()	{}
};

// WebSocket object events.

class XTOOLBOX_API VJSWebSocketEvent : public XBOX::IJSEvent
{
public:

	static VJSWebSocketEvent	*CreateOpenEvent (VJSWebSocketObject *inWebSocket, const XBOX::VString &inProtocol, const XBOX::VString &inExtensions);
	static VJSWebSocketEvent	*CreateMessageEvent (VJSWebSocketObject *inWebSocket, bool inIsText, uBYTE *inData, VSize inSize);
	static VJSWebSocketEvent	*CreateErrorEvent (VJSWebSocketObject *inWebSocket);
	static VJSWebSocketEvent	*CreateCloseEvent (VJSWebSocketObject *inWebSocket, bool inWasClean, sLONG inCode, const XBOX::VString &inReason);
	
	void						Process (XBOX::VJSContext inContext, VJSWorker *inWorker);

	void						Discard ();

private:

	enum {

		eTYPE_OPEN,				// "onopen"
		eTYPE_MESSAGE,			// "onmessage"
		eTYPE_ERROR,			// "onerror" 
		eTYPE_CLOSE,			// "onclose" 
		
	};

	sLONG						fSubType;			// Type of event.
	VJSWebSocketObject			*fWebSocket;		// WebSocket object.

	XBOX::VString				fProtocol;
	XBOX::VString				fExtensions;

	bool						fIsText;			// eTYPE_MESSAGE only.	
	uBYTE						*fData;				// If message is text, it will be converted from UTF-8.
	VSize						fSize;				

	bool						fWasClean;			// eTYPE_CLOSE only.
	sLONG						fCode;
	XBOX::VString				fReason;

								VJSWebSocketEvent ()	{}
	virtual						~VJSWebSocketEvent ()	{}
};

// WebSocketServer object events.

class XTOOLBOX_API VJSWebSocketServerEvent : public XBOX::IJSEvent
{
public:

	static VJSWebSocketServerEvent	*CreateConnectionEvent (XBOX::VJSObject& inObject, XBOX::VWebSocket *inWebSocket);
	static VJSWebSocketServerEvent	*CreateErrorEvent (XBOX::VJSObject& inObject);
	static VJSWebSocketServerEvent	*CreateCloseEvent (XBOX::VJSObject& inObject);

	void							Process (XBOX::VJSContext inContext, VJSWorker *inWorker);

	void							Discard ();

private:

	enum {

		eTYPE_CONNECTION,		// "onmessage"
		eTYPE_ERROR,			// "onerror" 
		eTYPE_CLOSE,			// "onclose" 
		
	};

	sLONG						fSubType;			// Type of event.

	XBOX::VJSObject				fObject;			// WebSocketServer object.

	XBOX::VWebSocket			*fWebSocket;		// Only for eTYPE_CONNECTION

								VJSWebSocketServerEvent (XBOX::VJSObject& inObject)	: fObject(inObject) {;}
	virtual						~VJSWebSocketServerEvent ()	{}
};

// Implementation of WebSocket "onconnect" event in a shared worker.

class XTOOLBOX_API VJSWebSocketConnectEvent : public XBOX::IJSEvent
{
public:

	// The connect event will be a WebSocket type object in fact.

	static VJSWebSocketConnectEvent	*Create (XBOX::VWebSocket *inWebSocket);
	void							Process (XBOX::VJSContext inContext, VJSWorker *inWorker);
	void							Discard ();

private:

	XBOX::VWebSocket				*fWebSocket;

									VJSWebSocketConnectEvent () {}
	virtual							~VJSWebSocketConnectEvent () {}
};

END_TOOLBOX_NAMESPACE

#endif
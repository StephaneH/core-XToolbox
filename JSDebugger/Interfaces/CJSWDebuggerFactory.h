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
#ifndef __JSW_DEBUGGER_FACTORY__
#define __JSW_DEBUGGER_FACTORY__

#if defined(_MSC_VER)
#include "conf_XTJavaScript.h"
#endif

#if USE_V8_ENGINE


#ifdef JSDEBUGGER_EXPORTS
#define JSDEBUGGER_API __declspec(dllexport)
#else
#ifdef __GNUC__
#define JSDEBUGGER_API __attribute__((visibility("default")))
#else
#define JSDEBUGGER_API __declspec(dllimport)
#endif
#endif


#define K_STR_MAX_SIZE			(512)
#define K_NB_MAX_DBG_FRAMES		(64)
#define K_MAX_FILESIZE			(512)


#define K_NETWORK_DEBUG_HOST	CVSTR("192.168.6.38")
#define K_NETWORK_DEBUG_BASE	(6789)


class /*JSDEBUGGER_API*/ IWAKDebuggerInfo
{
public:

	enum JSWD_CONTEXT_STATE
	{
		JSWD_CS_RUNNING,
		JSWD_CS_PAUSED,
		JSWD_CS_IDLE
	};

	virtual int GetContextList(uintptr_t** outContextIDs, IWAKDebuggerInfo::JSWD_CONTEXT_STATE** outStates, int& outCount) = 0;

protected:

	IWAKDebuggerInfo() { ; }
	virtual ~IWAKDebuggerInfo() { ; }
};



class WAKDebuggerServerMessage {

public:

	// DBG_ are messages sent by debugger to server
	// SRV_ are messages sent by server to debugger
	typedef enum WAKDebuggerServerMsgType_enum  {
		NO_MSG,							// empty msg
		// JSWD_C_UNKNOWN
		SRV_UNKNOWN_MSG,
		// JSWD_C_GET_SOURCE
		SRV_GET_SOURCE_MSG,				// server request source code
		// JSWD_C_CALL_STACK
		SRV_GET_CALLSTACK_MSG,			// server requests callstack info
		// JSWD_C_GET_EXCEPTION
		SRV_GET_EXCEPTION_MSG,			// server 
		//JSWD_C_EVALUATE
		SRV_EVALUATE_MSG,				// server requests an evaluate
		// JSWD_C_LOOKUP
		SRV_LOOKUP_MSG,					// server requests a lookup
		SRV_SET_BREAKPOINT_MSG,			// server request to set a brkpt
		SRV_REMOVE_BREAKPOINT_MSG,		// server request to remove a brkpt
		SRV_STEP_OVER_MSG,
		SRV_STEP_INTO_MSG,
		SRV_STEP_OUT_MSG,
		// JSWD_C_CONTINUE
		SRV_CONTINUE_MSG,
		// JSWD_C_LOCAL_SCOPE 
		SRV_LOCAL_SCOPE,
		// JSWD_C_SCOPES
		SRV_SCOPES,
		// JSWD_C_SET_CALL_STACK_FRAME
		SRV_SET_CALL_STACK_FRAME,
		// JSWD_C_SUSPEND,
		SRV_SUSPEND,
		// JSWD_C_ABORT_SCRIPT,
		SRV_ABORT,
		// JSWS_LIST_CONTEXTS,
		SRV_LIST_CONTEXTS,
		SRV_CONNEXION_INTERRUPTED,
	} WAKDebuggerServerMsgType_t;

	WAKDebuggerServerMessage()
	:fType(NO_MSG)
	,fLineNumber(0)
	,fObjRef(0.0)
	,fSrcId(0)
	,fLength(0)
	,fUrlLen(0)
	,fWithExceptionInfos(false)
	,fRequestId(0) 
	{
		memset(fString, 0, K_STR_MAX_SIZE);
		memset(fUrl, 0, K_STR_MAX_SIZE);
	}

	WAKDebuggerServerMsgType_t	fType;

	unsigned int				fLineNumber;
	double						fObjRef;
	intptr_t					fSrcId;
	char						fString[K_STR_MAX_SIZE];
	int							fLength;
	char						fUrl[K_STR_MAX_SIZE];
	unsigned int				fUrlLen;
	bool						fWithExceptionInfos;
	intptr_t					fRequestId;

};

typedef void*					OpaqueDebuggerContext;
typedef OpaqueDebuggerContext	WAKDebuggerContext_t;


class VCallstackDescription : public XBOX::VObject {
public:
	VCallstackDescription() { ; }
	bool						fSourceSent;
	XBOX::VString				fFileName;
	XBOX::VectorOfVString		fSourceCode;
};

typedef std::map< XBOX::VString, VCallstackDescription> CallstackDescriptionMap;

class JSDEBUGGER_API IWAKDebuggerCommand : public WAKDebuggerServerMessage
{
public:

	virtual WAKDebuggerServerMsgType_t GetType() const = 0;
	virtual const char * GetParameters() const = 0;
	virtual const char * GetID() const = 0;

	virtual void Dispose() = 0;

protected:

	virtual ~IWAKDebuggerCommand() { ; }
};



typedef enum WAKDebuggerState_e {
	IDLE_STATE,
	RUNNING_STATE,
	PAUSE_STATE
} WAKDebuggerState_t;


// defined the type of debugger
typedef enum WAKDebuggerType_e {
	REGULAR_DBG_TYPE = 0,
	WEB_INSPECTOR_TYPE,
	UNKNOWN_DBG_TYPE,
	NO_DEBUGGER_TYPE = 1024,
} WAKDebuggerType_t;

typedef enum WAKDebuggerTraceLevel_enum {
	WAKDBG_ERROR_LEVEL,
	WAKDBG_INFO_LEVEL
} WAKDebuggerTraceLevel_t;



typedef enum RemoteDebuggerPauseReason_e {
	DEBUGGER_RSN,
	BREAKPOINT_RSN,
	EXCEPTION_RSN
} RemoteDebuggerPauseReason;

typedef unsigned char*			WAKDebuggerUCharPtr_t;


class JSDEBUGGER_API IWAKDebuggerSettings;

class IRemoteDebuggerServer {

public:

	// server should specify which type of debugger it serves
	virtual WAKDebuggerType_t			GetType() = 0;
	virtual int							StartServer() = 0;
	virtual int							StopServer() = 0;
	virtual short						GetServerPort() = 0;
	virtual	bool						IsSecuredConnection() = 0;
	virtual void						SetInfo(IWAKDebuggerInfo* inInfo) = 0;
	virtual void						SetSettings(IWAKDebuggerSettings* inSettings) = 0;
	virtual bool						HasClients() = 0;
	virtual void						SetSourcesRoot(void* /*inSourcesRootVStringPtr*/) {}
	virtual void						SetSourcesRoot(char* /*inRoot*/, int /*inLength*/) {}
	virtual void						Trace(OpaqueDebuggerContext	inContext,
		const void*				inString,
		int						inSize,
		WAKDebuggerTraceLevel_t inTraceLevel = WAKDBG_ERROR_LEVEL) = 0;
	virtual void						GetStatus(
		bool&			outStarted,
		bool&			outConnected,
		long long&		outDebuggingEventsTimeStamp,
		bool&			outPendingContexts) = 0;
};

class IWAKDebuggerServer : public IRemoteDebuggerServer {
public:

	// methods should be called when creating/destroying a concrete debugger
	virtual OpaqueDebuggerContext		AddContext(uintptr_t inContext, void* inContextRef) = 0;
	virtual bool						RemoveContext(OpaqueDebuggerContext inContext) = 0;
	virtual bool						HasBreakpoint(
		OpaqueDebuggerContext				inContext,
		intptr_t							inSourceId,
		int									inLineNumber) = 0;

	// used when debugger waits a (server-side allocated) message sent by the server
	virtual WAKDebuggerServerMessage*	WaitFrom(OpaqueDebuggerContext inContext) = 0;
	// frees a server-side allocated message
	virtual void						DisposeMessage(WAKDebuggerServerMessage* inMessage) = 0;
	virtual WAKDebuggerServerMessage* GetNextBreakPointCommand() = 0;
	virtual WAKDebuggerServerMessage* GetNextSuspendCommand(OpaqueDebuggerContext inContext) = 0;
	virtual WAKDebuggerServerMessage* GetNextAbortScriptCommand(OpaqueDebuggerContext inContext) = 0;

	virtual bool						SetState(OpaqueDebuggerContext inContext, WAKDebuggerState_t state) = 0;
	//virtual bool						SendSource( OpaqueDebuggerContext inContext, intptr_t inSrcId, const char *inData, int inLength, const char* inUrl, unsigned int inUrlLen ) = 0;
	virtual bool						SendSource(
		OpaqueDebuggerContext	inContext,
		intptr_t				inSourceId,
		char*					inFileName = NULL,
		int						inFileNameLength = 0,
		const char*				inData = NULL,
		int						inDataLength = 0) = 0;
	virtual bool						BreakpointReached(
		OpaqueDebuggerContext	inContext,
		int						inLineNumber,
		int 					inExceptionHandle = -1/* -1 ? notException : ExceptionHandle */,
		char* 					inURL = NULL,
		int 					inURLLength = 0/* in bytes */,
		char* 					inFunction = NULL,
		int 					inFunctionLength = 0 /* in bytes */,
		char* 					inMessage = NULL,
		int 					inMessageLength = 0 /* in bytes */,
		char* 					inName = NULL,
		int 					inNameLength = 0 /* in bytes */,
		long 					inBeginOffset = 0,
		long 					inEndOffset = 0 /* in bytes */) = 0;
	// a enlever
	virtual char*						GetAbsolutePath(
		const unsigned short* inAbsoluteRoot, int inRootSize,
		const unsigned short* inRelativePath, int inPathSize,
		int& outSize) = 0;
	// a enlever
	virtual char*						GetRelativeSourcePath(
		const unsigned short* inAbsoluteRoot, int inRootSize,
		const unsigned short* inAbsolutePath, int inPathSize,
		int& outSize) = 0;
	virtual void						Reset() = 0;
	virtual void						WakeUpAllWaiters() = 0;

	virtual long long					GetMilliseconds() = 0;

	virtual bool						SendCallStack(
		OpaqueDebuggerContext	inContext,
		const char*				inData,
		int						inLength,
		const char*				inExceptionInfos = NULL,
		int						inExceptionInfosLength = 0) = 0;
	virtual bool						SendLookup(OpaqueDebuggerContext inContext, void* inVars, unsigned int inSize) = 0;
	virtual bool						SendEval(OpaqueDebuggerContext inContext, void* inVars) = 0;

	virtual WAKDebuggerUCharPtr_t		EscapeForJSON(const unsigned char* inString, int inSize, int& outSize) = 0;
	virtual void						DisposeUCharPtr(WAKDebuggerUCharPtr_t inUCharPtr) = 0;

	// returns a VString (allocated) ptr containing inSize 1st bytes of inString
	virtual void*						UStringToVString(const void* inString, int inSize) = 0;

	virtual int							Write(const char * inData, long inLength, bool inToUTF8 = false) = 0;
	virtual int							WriteFileContent(long inCommandID, uintptr_t inContext, const unsigned short* inFilePath, int inPathSize) = 0;
	virtual int							WriteSource(long inCommandID, uintptr_t inContext, const unsigned short* inSource, int inSize) = 0;

	// inFolderPath must be an unicode c string pointer
	virtual void						SetCertificatesFolder(const void* inFolderPath) = 0;


protected:
	virtual ~IWAKDebuggerServer() { ; }

};


class IChromeDebuggerServer : public IRemoteDebuggerServer {
public:

	virtual bool					HasBreakpoint(
		OpaqueDebuggerContext				inContext,
		intptr_t							inSourceId,
		int									inLineNumber) = 0;

	virtual bool					GetFilenameFromId(
		OpaqueDebuggerContext				inContext,
		intptr_t							inSourceId,
		char*								ioFileName,
		int&								ioFileNameLength) = 0;

	// used when debugger waits a (server-side allocated) message sent by the server
	virtual WAKDebuggerServerMessage*	WaitFrom(OpaqueDebuggerContext inContext) = 0;
	// frees a server-side allocated message
	virtual void						DisposeMessage(WAKDebuggerServerMessage* inMessage) = 0;

#if USE_V8_ENGINE
	// methods should be called when creating/destroying a concrete debugger
	virtual OpaqueDebuggerContext	AddContext(void* inContextRef) = 0;
	virtual bool					RemoveContext(OpaqueDebuggerContext inContext) = 0;
	virtual bool					DeactivateContext(OpaqueDebuggerContext inContext, bool inHideOnly = false) = 0;

	virtual bool						BreakpointReached(
		OpaqueDebuggerContext		inContext,
		XBOX::VString&				inSourceURL,
		int							inLineNumber,
		XBOX::VString&				inSourceLineText,
		RemoteDebuggerPauseReason	inPauseReason,
		XBOX::VString&				inExceptionName,
		bool&						outAbort,
		bool						inIsNew) = 0;

	virtual bool						SendCallStack(
		OpaqueDebuggerContext		inContext,
		const XBOX::VString&		inCallstackStr,
		const XBOX::VString&		inExceptionInfos,
		CallstackDescriptionMap&	inCSDesc) = 0;

	virtual bool						SendSource(
		OpaqueDebuggerContext		inContext,
		intptr_t					inSourceId,
		const XBOX::VString&		inFileName,
		const XBOX::VString&		inData,
		std::set< unsigned >&		outBreakpointsLineNumbers) = 0;

	virtual bool					SendLookup(
		OpaqueDebuggerContext		inContext,
		const XBOX::VString&		inLookupResult) = 0;
#else
	// methods should be called when creating/destroying a concrete debugger
	virtual OpaqueDebuggerContext		AddContext(void* inContextRef) = 0;
	virtual bool						RemoveContext(OpaqueDebuggerContext inContext) = 0;
	virtual bool						DeactivateContext(OpaqueDebuggerContext inContext, bool inHideOnly = false) = 0;
	virtual bool						SendSource(
		OpaqueDebuggerContext	inContext,
		intptr_t				inSourceId,
		char*					inFileName = NULL,
		int						inFileNameLength = 0,
		const char*				inData = NULL,
		int						inDataLength = 0) = 0;

	virtual bool						BreakpointReached(
		OpaqueDebuggerContext		inContext,
		intptr_t					inSourceId,
		int							inLineNumber,
		RemoteDebuggerPauseReason	inPauseReason,
		char* 						inExceptionName,
		int 						inExceptionNameLength,
		bool&						outAbort,
		bool						inIsNew) = 0;

	virtual bool						SendCallStack(
		OpaqueDebuggerContext	inContext,
		const char*				inCallstackStr,
		int						inCallstackStrLength,
		const char*				inExceptionInfos = NULL,
		int						inExceptionInfosLength = 0) = 0;

	virtual bool						SendLookup(OpaqueDebuggerContext inContext, void* inLookupResultVStringPtr) = 0; 
#endif

	virtual void						AbortAll() = 0;

	virtual bool						SendEval(
		OpaqueDebuggerContext	inContext,
		void*					inEvaluationResultVStringPtr,
		intptr_t				inRequestId) = 0;

	virtual WAKDebuggerUCharPtr_t		EscapeForJSON(const unsigned char* inString, int inSize, int& outSize) = 0;
	virtual void						DisposeUCharPtr(WAKDebuggerUCharPtr_t inUCharPtr) = 0;

	// returns a VString (allocated) ptr containing inSize 1st bytes of inString
	virtual void*						UStringToVStringPtr(const void* inUString, int inSize) = 0;

	virtual void						Lock() = 0;
	virtual void						Unlock() = 0;

protected:
	virtual ~IChromeDebuggerServer() { ; }

};

#else

#include "4DDebuggerServer.h"

#ifdef JSDEBUGGER_EXPORTS
    #define JSDEBUGGER_API __declspec(dllexport)
#else
	#ifdef __GNUC__
		#define JSDEBUGGER_API __attribute__((visibility("default")))
	#else
		#define JSDEBUGGER_API __declspec(dllimport)
	#endif
#endif

#endif

class JSDEBUGGER_API JSWDebuggerFactory
{
	public:
		JSWDebuggerFactory ( ) { ; }

		IWAKDebuggerServer*	Get();
		IChromeDebuggerServer*	GetChromeDebugHandler(const XBOX::VString& inSolutionName);
		IChromeDebuggerServer*	GetChromeDebugHandler(
			const XBOX::VString&	inSolutionName,
			const XBOX::VString&	inTracesHTMLSkeleton);
		
		IRemoteDebuggerServer*	GetNoDebugger();
	private:
};


class IAuthenticationInfos;

class JSDEBUGGER_API IWAKDebuggerSettings
{
	public:

		virtual bool	NeedsAuthentication ( ) const = 0;
		/* Returns 'true' if a given user can debug with this JS debugger. Returns 'false' otherwise. */
		virtual bool	UserCanDebug ( const UniChar* inUserName, const UniChar* inUserPassword ) const = 0;
		virtual bool	UserCanDebug(IAuthenticationInfos* inAuthenticationInfos) const = 0;
		virtual bool	HasDebuggerUsers ( ) const = 0;

		virtual	bool	AddBreakPoint( OpaqueDebuggerContext inContext, const XBOX::VString& inUrl, intptr_t inSrcId, int inLineNumber ) = 0;
		virtual	bool	AddBreakPoint( const XBOX::VString& inUrl, int inLineNumber ) = 0;
		virtual	bool	RemoveBreakPoint( const XBOX::VString& inUrl, int inLineNumber ) = 0;
		virtual	bool	RemoveBreakPoint( OpaqueDebuggerContext inContext, const XBOX::VString& inUrl, intptr_t inSrcId, int inLineNumber ) = 0;
		virtual void	Set(OpaqueDebuggerContext	inContext, const XBOX::VString& inUrl, intptr_t inSrcId, const XBOX::VString& inData) = 0;
		virtual void	Add(OpaqueDebuggerContext inContext) = 0;
		virtual void	Remove(OpaqueDebuggerContext	inContext) = 0;
		virtual bool	HasBreakpoint(OpaqueDebuggerContext	inContext,intptr_t inSrcId, unsigned lineNumber) = 0;
		virtual bool	GetData(OpaqueDebuggerContext inContext, intptr_t inSrcId, XBOX::VString& outSourceUrl, XBOX::VectorOfVString& outSourceData) = 0;

#if USE_V8_ENGINE
		virtual bool	GetRelativePosixPath(const XBOX::VString& inUrl, XBOX::VString& outRelativePosixPath) = 0;
#endif

		virtual void	GetJSONBreakpoints(XBOX::VString& outJSONBreakPoints) = 0;
		virtual	void	GetAllJSFileNames(XBOX::VectorOfVString& outFileNameVector) = 0;
		virtual	void	GetSourceFromUrl(OpaqueDebuggerContext inContext, const XBOX::VString& inSourceUrl,XBOX::VectorOfVString& outSourceData) = 0;

	protected:

		IWAKDebuggerSettings ( ) { ; }
		virtual ~IWAKDebuggerSettings ( ) { ; }

};


#endif

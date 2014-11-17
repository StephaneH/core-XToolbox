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
#ifndef __VCHROME_DEBUG_HANDLER__
#define __VCHROME_DEBUG_HANDLER__


#include "KernelIPC/Sources/VSignal.h"
#include "JSDebugger/Interfaces/CJSWDebuggerFactory.h"
#include "HTTPServer/Interfaces/CHTTPServer.h"


#ifdef JSDEBUGGER_EXPORTS
    #define JSDEBUGGER_API __declspec(dllexport)
#else
	#ifdef __GNUC__
		#define JSDEBUGGER_API __attribute__((visibility("default")))
	#else
		#define JSDEBUGGER_API __declspec(dllimport)
	#endif
#endif




template <class Elt>
class TemplProtectedFifo	{
public:
	#define K_NB_MAX_MSGS	(4)
	
	TemplProtectedFifo(unsigned int inMaxSize=K_NB_MAX_MSGS, bool inAssertWhenFull=true) :
					fSem(0,inMaxSize+1), fMaxSize(inMaxSize), fAssertWhenFull(inAssertWhenFull) {;}

	void							Reset()	{
		if (!testAssert(fLock.Lock()))
		{
			// trace
		}
		while (!fMsgs.empty())
		{
			fMsgs.pop();
		}
		while(fSem.TryToLock())
		{
		}
		if (!testAssert(fLock.Unlock()))
		{
			// trace
		}
	}
	XBOX::VError					Put(Elt& inMsg)
	{
		XBOX::VError	err;
		size_t			size;
		err = XBOX::VE_UNKNOWN_ERROR;
		if (!testAssert(fLock.Lock()))
		{
			// trace
		}
		size = fMsgs.size();
		if (size < fMaxSize)
		{
			fMsgs.push(inMsg);
			if (!fSem.Unlock())
			{
				xbox_assert(false);
			}
			else
			{
				err = XBOX::VE_OK;
			}
		}
		else
		{
			err = XBOX::VE_MEMORY_FULL;
			if (fAssertWhenFull)
			{
				xbox_assert(false);
			}
		}
		if (!testAssert(fLock.Unlock()))
		{
			// trace
		}
		return err;
	}
	XBOX::VError					Get(Elt& outMsg, bool& outFound, uLONG inTimeoutMs) {
		bool			ok;
		XBOX::VError	err;

		outFound = false;
		err = XBOX::VE_UNKNOWN_ERROR;

		if (!inTimeoutMs)
		{
			ok = fSem.Lock();
		}
		else
		{
			ok = fSem.Lock((sLONG)inTimeoutMs);
		}
		if (ok)
		{
			if (!testAssert(fLock.Lock()))
			{
				// trace
			}
			outMsg = (Elt)fMsgs.front();
			fMsgs.pop();
			if (!testAssert(fLock.Unlock()))
			{
				// trace
			}
			outFound = true;
			err = XBOX::VE_OK;
		}
		else
		{
			if (inTimeoutMs)
			{
				err = XBOX::VE_OK;
			}
		}
		return err;
	}

	// waits infinitely if inTimeoutMs<0, waits inTimeoutMs ms when >0, does not wait when inTimeoutMs is null
	XBOX::VError					Get2(Elt& outMsg, bool& outFound, sLONG inTimeoutMs) {
		bool			ok;
		XBOX::VError	err;

		outFound = false;
		err = XBOX::VE_UNKNOWN_ERROR;

		if (!inTimeoutMs)
		{
			ok = fSem.TryToLock();
			if (!ok)
			{
				return XBOX::VE_OK;
			}
		}
		else
		{
			if (inTimeoutMs > 0)
			{
				ok = fSem.Lock(inTimeoutMs);
			}
			else
			{
				ok = fSem.Lock();
			}
		}
		if (ok)
		{
			if (!testAssert(fLock.Lock()))
			{
				// trace
			}
			outMsg = (Elt)fMsgs.front();
			fMsgs.pop();
			if (!testAssert(fLock.Unlock()))
			{
				// trace
			}
			outFound = true;
			err = XBOX::VE_OK;
		}
		else
		{
			if (inTimeoutMs > 0)
			{
				err = XBOX::VE_OK;
			}
		}
		return err;
	}
private:
	unsigned int					fMaxSize;
	bool							fAssertWhenFull;
	std::queue<Elt>					fMsgs;
	XBOX::VCriticalSection			fLock;
	XBOX::VSemaphore				fSem;
};

class VRemoteDebugPilot;

#if USE_V8_ENGINE
#else
class VCallstackDescription : public XBOX::VObject {
public:
	VCallstackDescription() {;}
	bool						fSourceSent;
	XBOX::VString				fFileName;
	XBOX::VectorOfVString		fSourceCode;
};

typedef std::map< XBOX::VString, VCallstackDescription> CallstackDescriptionMap;
#endif

class JSDEBUGGER_API VChromeDebugHandler :
						public IWebSocketHandler,
						public IChromeDebuggerServer,
						public XBOX::VObject
{
public:

	static VChromeDebugHandler*			Get(const XBOX::VString&	inSolutionName);

	static VChromeDebugHandler*			Get(
											const XBOX::VString&	inSolutionName,
											const XBOX::VString&	inTracesHTMLSkeleton,
											bool					inV8Mode );

	// to be removed after clean start/stop of dbgr server
	static	void						StaticSetSettings( IWAKDebuggerSettings* inSettings );

	static	void						StaticSendBreakpointsUpdated();

protected:

	friend class VRemoteDebugPilot;
	friend class VChromeDbgHdlPage;

	static	void						AddBreakPointByUrl(const XBOX::VString& inUrl,int inLineNumber);
	static	void						RemoveBreakPointByUrl(const XBOX::VString& inUrl,int inLineNumber);
	static	void						AddBreakPoint(OpaqueDebuggerContext inContext,intptr_t inSourceId,int inLineNumber);
	static	void						RemoveBreakPoint(OpaqueDebuggerContext inContext,intptr_t inSourceId,int inLineNumber);
	static	void						GetAllJSFileNames(XBOX::VectorOfVString& outFileNameVector);
	static	void						GetJSONBreakpoints(XBOX::VString& outJSONBreakPoints);
	static	void						GetSourceFromUrl(
											OpaqueDebuggerContext		inContext,
											const XBOX::VString&		inSourceUrl,
											XBOX::VectorOfVString&		outSourceData );
private:
										VChromeDebugHandler(
											const XBOX::VString&	inSolutionName,
											const XBOX::VString&	inTracesHTML,
											bool					inV8Mode );
										VChromeDebugHandler( const VChromeDebugHandler& inChromeDebugHandler);
	virtual								~VChromeDebugHandler();

	// from IWebSocketHandler
			XBOX::VError				GetPatterns(XBOX::VectorOfVString* outPatterns) const;
			XBOX::VError				HandleRequest(IHTTPResponse* ioResponse);
			bool						IsAuthorized( IHTTPResponse* ioResponse );

	// from IChromeDebuggerServer
	virtual WAKDebuggerType_t			GetType();
	virtual int							StartServer();
	virtual int							StopServer();
	virtual short						GetServerPort();
	virtual	bool						IsSecuredConnection();
	virtual void						SetSettings( IWAKDebuggerSettings* inSettings );
	virtual void						SetInfo( IWAKDebuggerInfo* inInfo );
	virtual bool						HasClients();

	virtual void						GetStatus(
											bool&			outStarted,
											bool&			outConnected,
											long long&		outDebuggingEventsTimeStamp,
											bool&			outPendingContexts);
	virtual OpaqueDebuggerContext		AddContext(void* inContextRef);
	virtual bool						RemoveContext( OpaqueDebuggerContext inContext );
	virtual bool						DeactivateContext( OpaqueDebuggerContext inContext, bool inHideOnly = false );
	virtual bool						SendEval(	OpaqueDebuggerContext	inContext,
													void*					inEvaluationResultVStringPtr,
													intptr_t				inRequestId  );

#if USE_V8_ENGINE
	virtual bool						BreakpointReached(
											OpaqueDebuggerContext		inContext,
											XBOX::VString&				inSourceURL,
											int							inLineNumber,
											XBOX::VString&				inSourceLineText,
											RemoteDebuggerPauseReason	inPauseReason,
											XBOX::VString&				inExceptionName,
											bool&						outAbort,
											bool						inIsNew);

	virtual bool						SendCallStack(
											OpaqueDebuggerContext		inContext,
											const XBOX::VString&		inCallstackStr,
											const XBOX::VString&		inExceptionInfos,
											CallstackDescriptionMap&	inCSDesc);

	virtual	bool						SendSource(
											OpaqueDebuggerContext				inContext,
											intptr_t							inSourceId,
											const XBOX::VString&				inFileName,
											const XBOX::VString&				inData,
											std::set< unsigned >&				outBreakpointsLineNumbers);

	virtual bool						SendLookup(
											OpaqueDebuggerContext		inContext,
											const XBOX::VString&		inLookupResult);
#else
	virtual bool						BreakpointReached(
		OpaqueDebuggerContext		inContext,
		intptr_t					inSourceId,
		int							inLineNumber,
		RemoteDebuggerPauseReason	inPauseReason,
		char* 						inExceptionName,
		int 						inExceptionNameLength,
		bool&						outAbort,
		bool						inIsNew);

	virtual bool						SendCallStack(OpaqueDebuggerContext	inContext,
		const char*				inCallstackStr,
		int						inCallstackStrLength,
		const char*				inExceptionInfos = NULL,
		int						inExceptionInfosLength = 0);

	virtual bool						SendSource(
		OpaqueDebuggerContext	inContext,
		intptr_t				inSourceId,
		char*					inFileName = NULL,
		int						inFileNameLength = 0,
		const char*				inData = NULL,
		int						inDataLength = 0); 

	virtual bool						SendLookup( OpaqueDebuggerContext inContext, void* inLookupResultVStringPtr );
#endif

	virtual void						AbortAll();
	
	virtual bool						HasBreakpoint(
											OpaqueDebuggerContext				inContext,
											intptr_t							inSourceId,
											int									inLineNumber);

	virtual	bool						GetFilenameFromId(
											OpaqueDebuggerContext				inContext,
											intptr_t							inSourceId,
											char*								ioFileName,
											int&								ioFileNameLength);

	virtual WAKDebuggerServerMessage*	WaitFrom(OpaqueDebuggerContext inContext);
	virtual void						DisposeMessage(WAKDebuggerServerMessage* inMessage);

	virtual WAKDebuggerUCharPtr_t		EscapeForJSON( const unsigned char* inString, int inSize, int& outSize );
	virtual void						DisposeUCharPtr( WAKDebuggerUCharPtr_t inUCharPtr );

	virtual void*						UStringToVStringPtr( const void* inString, int inSize );

	virtual void						Trace(	OpaqueDebuggerContext	inContext,
												const void*				inString,
												int						inSize,
												WAKDebuggerTraceLevel_t inTraceLevel = WAKDBG_ERROR_LEVEL );

	virtual void						Lock();
	virtual void						Unlock();

	XBOX::VError						TreatJSONFile(IHTTPResponse* ioResponse);
	XBOX::VError						TreatTracesHTML(IHTTPResponse* ioResponse);

	XBOX::VError						TreatRemoteTraces(IHTTPResponse* ioResponse);

	bool								fStarted;
	VRemoteDebugPilot*					fPilot;
	XBOX::VString						fTracesHTML;


	static								VChromeDebugHandler*			sDebugger;
	static								XBOX::VCriticalSection			sDbgLock;
	// to be removed after clean start/stop of dbgr server
	static								IWAKDebuggerSettings*			sStaticSettings;

	static								intptr_t						sSrcId_GH_TEST;
//#define UNIFIED_DEBUGGER_NET_DEBUG

};

#endif


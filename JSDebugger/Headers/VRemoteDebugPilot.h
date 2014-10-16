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
#ifndef __REMOTE_DEBUG_PILOT_H__
#define __REMOTE_DEBUG_PILOT_H__

#include "KernelIPC/Sources/VSignal.h"
#include "JSDebugger/Interfaces/CJSWDebuggerFactory.h"
#include "HTTPServer/Interfaces/CHTTPServer.h"
#include "VChromeDbgHdlPage.h"


class VRemoteDebugPilot : public XBOX::VObject {

public:
									VRemoteDebugPilot(	CHTTPServer*			inHTTPServer);

									~VRemoteDebugPilot();

			XBOX::VError			Start();

			XBOX::VError			Stop();

			void					GetStatus(bool&	outPendingContexts,bool& outConnected,sLONG& outDebuggingEventsTimeStamp);

			bool					IsActive() {
										return (fState == STARTED_STATE || fState == CONNECTING_STATE || fState == CONNECTED_STATE); }

			void					SetWS(IHTTPWebsocketServer* inWS) { fWS = inWS; }
	
			XBOX::VError			TreatRemoteDbg(IHTTPResponse* ioResponse, bool inNeedsAuthentication);

			XBOX::VError			TreatWS(IHTTPResponse* ioResponse, uLONG inPageNb );

			void					SetSolutionName(const XBOX::VString& inSolutionName) {fSolutionName = inSolutionName;}
			void					GetSolutionName(XBOX::VString& outSolutionName) {outSolutionName = fSolutionName;}

			WAKDebuggerServerMessage*		WaitFrom(OpaqueDebuggerContext inContext);

			XBOX::VError			SendEval( OpaqueDebuggerContext inContext, const XBOX::VString& inEvaluationResult, const XBOX::VString& inRequestId );

			XBOX::VError			SendLookup( OpaqueDebuggerContext inContext, const XBOX::VString& inLookupResult );

			XBOX::VError			SendCallStack(
											OpaqueDebuggerContext				inContext,
											const XBOX::VString&				inCallstackStr,
											const XBOX::VString&				inExceptionInfosStr,
											const CallstackDescriptionMap&		inCallstackDesc);
	
			XBOX::VError			BreakpointReached(
											OpaqueDebuggerContext		inContext,
											int							inLineNumber,
											RemoteDebuggerPauseReason	inDebugReason,
											XBOX::VString&				inException,
											XBOX::VString&				inSourceUrl,
											XBOX::VectorOfVString&		inSourceData,
											bool&						outAbort,
											bool						inIsNew);

			XBOX::VError			NewContext(void* inContextRef, OpaqueDebuggerContext* outContext);
	
			XBOX::VError			ContextRemoved(OpaqueDebuggerContext inContext);

			XBOX::VError			DeactivateContext( OpaqueDebuggerContext inContext, bool inHideOnly );
			
			XBOX::VError			AbortAll();

			void					SendBreakpointsUpdated();

	static	void					GetRelativeUrl( const XBOX::VString& inUrl, XBOX::VString& outRelativeUrl );
	static	void					GetAbsoluteUrl( const XBOX::VString& inUrl, const XBOX::VString& inSolutionName, XBOX::VString& outAbsoluteUrl );

private:
			XBOX::VString						fSolutionName;
			XBOX::VSemaphore					fPendingContextsNumberSem;
			sLONG								fDebuggingEventsTimeStamp;

	class VContextDebugInfos : public XBOX::VObject {
	public:
		VContextDebugInfos() : fLineNb(0) {;} 
		sLONG					fLineNb;
		XBOX::VString			fFileName;
		XBOX::VString			fReason;
		XBOX::VString			fComment;
		XBOX::VString			fSourceLine;
	};

	virtual XBOX::VError			ContextUpdated(	OpaqueDebuggerContext		inContext,
													const VContextDebugInfos&	inDebugInfos,
													XBOX::VSemaphore**			outSem,
													bool						inIsNew);


#define K_NB_MAX_CTXS		(1024)


// the order of declaration is important: INIT/STOPPED then STARTED then all the others states which are sub-states of STARTED
typedef enum RemoteDebugPilotState_enum {
		// pilot is initialized, waits to be started
		INIT_STATE,
		// pilot is stopped (unused as long as destructor is not called)
		STOPPED_STATE,
		// pilot is started, waits for a (browser) client connection
		STARTED_STATE,
		// pilot waits connected client disconnection
		DISCONNECTING_STATE,
		// client has created a WS to communicate but connexion os not complete (e.g. authentication needed)
		CONNECTING_STATE,
		// pilot is working for a connected client, treats the requests coming through the WS
		CONNECTED_STATE
} RemoteDebugPilotState_t;

	static	sLONG					InnerTaskProc(XBOX::VTask* inTask);

	#define K_NB_MAX_PAGES		(16)

	class VContextDescriptor : public XBOX::VObject, public XBOX::IRefCountable {
	public:
		VContextDescriptor(void* inContextRef) : fSem(NULL) , fPage(NULL), fUpdated(false), fContextRef(inContextRef)
		{
			fTraceContainer = new VChromeDbgHdlPage::VTracesContainer();
		}
		~VContextDescriptor();
		XBOX::VSemaphore*							fSem;
		VChromeDbgHdlPage*							fPage;
		VChromeDbgHdlPage::VTracesContainer*		fTraceContainer;
		bool										fUpdated;
		bool										fAborted;
		VContextDebugInfos							fDebugInfos;
		void*										fContextRef;
	private:
		VContextDescriptor(const VContextDescriptor& inContextDescriptor);
	};

	// perform an hysteresis on status to compensate WAKstudio polling mecanism
	class VStatusHysteresis : public XBOX::VObject {
	public:
		VStatusHysteresis();
		void					Get(bool& ioConnected);
		void					SetDuration(sLONG8 inMs);
		void					ResetDuration();
	private:
		XBOX::VCriticalSection	fLock;
		XBOX::VTime				fLastConnectedTime;
		XBOX::VDuration			fHysteresisDuration;
	};

	class VPilotLogListener : public XBOX::VObject, public XBOX::ILogListener
	{
	public:
						VPilotLogListener(VRemoteDebugPilot* inPilot);
		virtual	void	Put( std::vector< const XBOX::VValueBag* >& inValuesVector );
	private:
			VRemoteDebugPilot*		fPilot;
	};

	class VPilotMessage : public XBOX::VMessage
	{
	public:
			XBOX::VError								fStatus;
	protected:
			VRemoteDebugPilot*							fPilot;
			VPilotMessage() : fStatus(XBOX::VE_INVALID_PARAMETER), fPilot(NULL) {;}
	private:
	};

	class VPilotSendBreakpointsUpdateMessage : public VPilotMessage
	{
	public:
		VPilotSendBreakpointsUpdateMessage(VRemoteDebugPilot* inPilot) { fPilot = inPilot; }
	private:
		void	DoExecute();
	};

	class VPilotStartMessage : public VPilotMessage
	{
	public:
		VPilotStartMessage(VRemoteDebugPilot* inPilot) { fPilot = inPilot; }
	private:
		void	DoExecute();
	};

	class VPilotStopMessage : public VPilotMessage
	{
	public:
		VPilotStopMessage(VRemoteDebugPilot* inPilot) { fPilot = inPilot; }
	private:
		void	DoExecute();
	};

	class VPilotAbortAllMessage : public VPilotMessage
	{
	public:
		VPilotAbortAllMessage(VRemoteDebugPilot* inPilot) { fPilot = inPilot; }
	private:
		void	DoExecute();
	};

	class VPilotTreatNewClientMessage : public VPilotMessage
	{
	public:
		VPilotTreatNewClientMessage(VRemoteDebugPilot* inPilot) {
			fPilot = inPilot;
		}
		bool							fNeedsAuthentication;
		IHTTPResponse*					fResponse;
	private:
		void	DoExecute();
	};

	class VPilotWaitFromMessage : public VPilotMessage
	{
	public:
		VPilotWaitFromMessage(VRemoteDebugPilot* inPilot) {
			fPilot = inPilot;
			fPage = NULL;
		}
		VChromeDbgHdlPage**				fPage;
		OpaqueDebuggerContext			fCtx;
	private:
		void	DoExecute();
	};

	class VPilotBreakpointReachedMessage : public VPilotMessage
	{
	public:
		VPilotBreakpointReachedMessage(VRemoteDebugPilot* inPilot, bool &inAbort)
		:fAbort(inAbort)
		{
			fPilot = inPilot;
		}
		OpaqueDebuggerContext			fCtx;
		XBOX::VectorOfVString			fDataVectStr;
		XBOX::VString					fDataStr;
		XBOX::VString					fUrlStr;
		int								fLineNb;
		bool&							fAbort;
	private:
		void	DoExecute();
	};


	class VPilotNewContextMessage : public VPilotMessage
	{
	public:
		VPilotNewContextMessage(VRemoteDebugPilot* inPilot) {
			fPilot = inPilot;
			fOutCtx = NULL;
		}
		OpaqueDebuggerContext*			fOutCtx;
		void*							fContextRef;

	private:
		void	DoExecute();
	};

	class VPilotUpdateContextMessage : public VPilotMessage
	{
	public:
		VPilotUpdateContextMessage(VRemoteDebugPilot* inPilot) {
			fPilot = inPilot;
		}
		OpaqueDebuggerContext			fCtx;
		VContextDebugInfos				fDebugInfos;
		XBOX::VSemaphore**				fOutsem;
		bool							fIsNew;
	private:
		void	DoExecute();
	};

	class VPilotRemoveContextMessage : public VPilotMessage
	{
	public:
		VPilotRemoveContextMessage(VRemoteDebugPilot* inPilot) {
			fPilot = inPilot;
		}
		OpaqueDebuggerContext			fCtx;
	private:
		void	DoExecute();
	};

	class VPilotDeactivateContextMessage : public VPilotMessage
	{
	public:
		VPilotDeactivateContextMessage(VRemoteDebugPilot* inPilot) {
			fPilot = inPilot;
		}
		OpaqueDebuggerContext			fCtx;
		bool							fHideOnly;
	private:
		void	DoExecute();
	};

	class VPilotTreatWSMessage : public VPilotMessage
	{
	public:
		VPilotTreatWSMessage(VRemoteDebugPilot* inPilot) {
			fPilot = inPilot;
			fPageNb = 99999999;
			fPage = NULL;
			fSem = NULL;
		}
		uLONG							fPageNb;
		VChromeDbgHdlPage**				fPage;
		XBOX::VSemaphore**				fSem;
	private:
		void	DoExecute();
	};

			XBOX::VCppMemMgr*							fMemMgr;
			IHTTPWebsocketServer*						fWS;
			XBOX::VTask*								fTask;
			RemoteDebugPilotState_t						fState;
			XBOX::VTime									fDisconnectTime;
			bool										fNeedsAuthentication;
			std::map< OpaqueDebuggerContext, VContextDescriptor*>	fContextArray;
			sBYTE*										fTmpData;
			XBOX::VString								fClientId;
			CHTTPServer*								fHTTPServer;
			VStatusHysteresis							fStatusHysteresis;
			XBOX::VCriticalSection						fLock;
			std::vector<const XBOX::VValueBag*>			fTraces;
			XBOX::VCriticalSection						fTracesLock;
			VPilotLogListener							fLogListener;

			XBOX::VError					SendToBrowser( const XBOX::VString& inResp );
	
			void							DisconnectBrowser();

			void							RemoveContext(
												std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator&	inCtxIt,
												bool																inJSCtxIsValid=true);

			void							TreatSendBreakpointsUpdated();
			void							TreatTraces();
		
			void							HandleDisconnectingState();
			void							HandleConnectedState();
			void							HandleConnectingState();


			XBOX::VError					SendContextToBrowser(
														std::map< OpaqueDebuggerContext,VContextDescriptor* >::iterator& inCtxIt);

			void							AbortContexts(bool inOnlyDebuggedOnes);

			void							DisplayContexts();

	static	sLONG							sRemoteDebugPilotContextId;

};

#endif

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
#ifndef __VCHROME_DEBUG_HANDLER_PAGE__
#define __VCHROME_DEBUG_HANDLER_PAGE__

#include "VLogHandler.h"


#define K_MAX_SIZE					(4096)




class VChromeDbgHdlPage : public XBOX::VObject, public XBOX::IRefCountable {

public:

	class VTracesContainer : public VObject, public XBOX::IRefCountable {
	public:
									VTracesContainer();
									~VTracesContainer();
			void					Put(const XBOX::VValueBag* inTraceBag );
			const XBOX::VValueBag*	Get();
	private:
									VTracesContainer( const VTracesContainer& inOther);
			sLONG					Next(sLONG inIndex);

			#define K_BAGS_ARRAY_SIZE		(32)
			const XBOX::VValueBag*			fBags[K_BAGS_ARRAY_SIZE];
			sLONG							fStartIndex;	
			sLONG							fEndIndex;
	mutable	XBOX::VCriticalSection			fLock;
	};
									VChromeDbgHdlPage(const XBOX::VString& inSolutionName,VTracesContainer* intracesContainer);
	virtual							~VChromeDbgHdlPage();

	virtual void					Init(sLONG inPageNb,CHTTPServer* inHTTPSrv,IWAKDebuggerSettings* inBrkpts);
	virtual XBOX::VError			Start(OpaqueDebuggerContext inCtxId);
	virtual XBOX::VError			Stop();

	virtual XBOX::VError			TreatWS(IHTTPResponse* ioResponse,XBOX::VSemaphore* inSem);
	virtual sLONG					GetPageNb() {return fPageNb;};
	
	virtual XBOX::VError			Abort();
	virtual XBOX::VError			Lookup(const XBOX::VString& inLookupData);
	virtual XBOX::VError			Eval(const XBOX::VString& inEvalData,const XBOX::VString& inRequestId);
	virtual XBOX::VError			Callstack(
										const XBOX::VString&				inCallstackData,
										const XBOX::VString&				inExceptionInfosStr,
										const CallstackDescriptionMap&		inCallstackDesc);
	virtual XBOX::VError			BreakpointReached(
										int									inLineNb,
										const XBOX::VectorOfVString&		inDataStr,
										const XBOX::VString&				inUrlStr,
										const XBOX::VString&				inExceptionStr);

	virtual WAKDebuggerServerMessage*		WaitFrom();

private:
	VChromeDbgHdlPage( const VChromeDbgHdlPage& inHdlPage );

	typedef enum ChrmDbgMsgType_enum {
		NO_MSG,
		STOP_MSG,
		SEND_CMD_MSG,
		CALLSTACK_MSG,
		LOOKUP_MSG,
		EVAL_MSG,
		SET_SOURCE_MSG,
		BRKPT_REACHED_MSG,
		ABORT_MSG
	} ChrmDbgMsgType_t;

	typedef struct ChrmDbgMsgData_st {
		XBOX::VString					_urlStr;
		XBOX::VString					_dataStr;
		XBOX::VectorOfVString			_dataVectStr;
		XBOX::VString					_excStr;
		XBOX::VString					_excInfosStr;
		CallstackDescriptionMap			_callstackDesc;
		XBOX::VString					_requestId;
		WAKDebuggerServerMessage		Msg;
	} ChrmDbgMsgData_t;

	typedef struct ChrmDbgMsg_st {
		ChrmDbgMsgType_t				type;
		ChrmDbgMsgData_t				data;
	} ChrmDbgMsg_t;

	XBOX::VError					CheckInputData();
	XBOX::VError					TreatMsg(XBOX::VSemaphore* inSem);
	XBOX::VError					SendResult(const XBOX::VString& inValue, const XBOX::VString& inRequestId);
	XBOX::VError					SendResult(const XBOX::VString& inValue);
	XBOX::VError					SendMsg(const XBOX::VString& inValue);
	XBOX::VError					SendDbgPaused(const XBOX::VString& inStr);
	XBOX::VError					SendEvaluateResult(const XBOX::VString& inResult, const XBOX::VString& inRequestId);
	XBOX::VError					TreatPageReload(
										const XBOX::VString&			inStr,
										const XBOX::VString&			inExcStr,
										const XBOX::VString&			inExcInfos);
	XBOX::VError					TreatBreakpointReached(ChrmDbgMsg_t& inMsg);
	XBOX::VError					TreatEvaluate(ChrmDbgMsg_t& inMsg);
	XBOX::VError					TreatLookup(ChrmDbgMsg_t& inMsg);
	void							Log(const XBOX::VString& inLoggerID,
										XBOX::EMessageLevel inLevel,
										const XBOX::VString& inMessage,
										XBOX::VString* outFormattedMessage);
	void							Log(const char* inLoggerID,
										XBOX::EMessageLevel inLevel,
										const char* inMessage,
										XBOX::VString* outFormattedMessage);

	XBOX::VError					TreatTraces();
	
	typedef enum ChromeDbgHdlPageState_enum {
		STOPPED_STATE,
		STARTING_STATE,
		CONNECTED_STATE,
	} ChromeDbgHdlPageState_t;

	XBOX::VString							fSolutionName;
	IHTTPWebsocketServer*					fWS;
	TemplProtectedFifo<ChrmDbgMsg_t>		fFifo;
	TemplProtectedFifo<ChrmDbgMsg_t>		fOutFifo;
	XBOX::VTask*							fTask;
	XBOX::VString							fMsgVString;
	XBOX::VString							fMethod;
	XBOX::VString							fId;
	XBOX::VJSONValue						fJsonParamsValue;
	sBYTE									fMsgData[K_MAX_SIZE];
	XBOX::VSize								fOffset;
	XBOX::VSize								fMsgLen;
	bool									fIsMsgTerminated;
	sLONG									fBodyNodeId;
	sLONG									fPageNb;
	intptr_t								fSrcId;
	sLONG									fLineNb;
	XBOX::VString							fFileName;
	//XBOX::VectorOfVString					fSource;
	XBOX::VSemaphore						fSem;
	ChromeDbgHdlPageState_t					fState;
	XBOX::VString							fExcStr;
	XBOX::VString							fExcInfosStr;
	unsigned int							fInternalState;
	OpaqueDebuggerContext					fCtxId;
	CallstackDescriptionMap					fCallstackDescription;
	VTracesContainer*						fTracesContainer;
	bool									fConsoleEnabled;

	static	XBOX::VString*					sSupportedCSSProperties;

};

#endif


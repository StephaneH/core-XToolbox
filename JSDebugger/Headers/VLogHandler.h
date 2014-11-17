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
#ifndef __VCHROME_DEBUG_LOG_HANDLER__
#define __VCHROME_DEBUG_LOG_HANDLER__

#include "VChromeDebugHandler.h"


class VLogHandler : public XBOX::VObject {
public:
	VLogHandler();
	void SetWS(IHTTPWebsocketServer* inWS);
	void Put(WAKDebuggerTraceLevel_t inTraceLevel, const XBOX::VString& inStr);
	void TreatClient(IHTTPResponse* ioResponse);
private:
	~VLogHandler() {;}
	IHTTPWebsocketServer*		fWS;
	XBOX::VSemaphore			fSem;
	typedef enum TraceState_enum {
		STOPPED_STATE,
		STARTED_STATE,
		WORKING_STATE,
	} TraceState_t;

	TraceState_t				fState;

typedef enum LogHandlerMsgType_enum {
		PUT_ERROR_TRACE_MSG = WAKDBG_ERROR_LEVEL,
		PUT_INFO_TRACE_MSG = WAKDBG_INFO_LEVEL,
} LogHandlerMsgType_t;

typedef struct LogHandlerMsg_st {
	LogHandlerMsgType_t		type;
	XBOX::VString			value;
} LogHandlerMsg_t;
	TemplProtectedFifo<LogHandlerMsg_t>	fFifo;

};


#endif


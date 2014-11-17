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


#include "VLogHandler.h"

USING_TOOLBOX_NAMESPACE


VLogHandler::VLogHandler() : fSem(1), fFifo(32,false)
{
	fWS = NULL;
	fState = STOPPED_STATE;
}

void VLogHandler::SetWS(IHTTPWebsocketServer* inWS)
{
	fWS = inWS;
	/*fTask = new XBOX::VTask(this, 64000, XBOX::eTaskStylePreemptive, &LogHandler::InnerTaskProc);
	if (fTask)
	{
		fTask->SetKindData((sLONG_PTR)this);
		fTask->Run();
	}*/
}

void VLogHandler::Put(WAKDebuggerTraceLevel_t inTraceLevel, const XBOX::VString& inStr)
{
	LogHandlerMsg_t		msg;
	if (fState != WORKING_STATE)
	{
		return;
	}
	msg.type = (LogHandlerMsgType_t)inTraceLevel;
	msg.value = inStr;
	if (fFifo.Put(msg) !=  VE_OK )
	{
		DebugMsg("LogHandler::PutE trace lost...\n");
	}
}


void VLogHandler::TreatClient(IHTTPResponse* ioResponse)
{
	LogHandlerMsg_t		msg;
	bool				msgFound;
	char				tmpData[1024];
	VSize				size;
	VString				trace;

	if (!fSem.TryToLock())
	{
		DebugMsg("LogHandler::TreatClient already in use\n"); 
		return;
	}
	XBOX::VError			err;
	fState = STARTED_STATE;
	while (fState != STOPPED_STATE)
	{
		switch(fState)
		{
		case STARTED_STATE:
			err = fWS->TreatNewConnection(ioResponse,false);
			xbox_assert( err == VE_OK );
			if (err == VE_OK)
			{
				fState = WORKING_STATE;
			}
			else
			{
				fState = STOPPED_STATE;
			}
			break;
		case WORKING_STATE:
			err = fFifo.Get(msg,msgFound,500);
			xbox_assert( err == VE_OK );
			if (err != VE_OK)
			{
				fWS->Close();
				fState = STOPPED_STATE;
			}
			else
			{
				if (msgFound)
				{
					VString		traceClean;
					msg.value.GetJSONString(traceClean);
					switch(msg.type)
					{
					case PUT_ERROR_TRACE_MSG:
						trace = "{\"type\":\"ERROR\",\"value\":\"";
						trace += traceClean;
						trace += "\"}";
						break;
#if 1//defined(UNIFIED_DEBUGGER_NET_DEBUG_INFO_LEVEL)
					case PUT_INFO_TRACE_MSG:
						trace = "{\"type\":\"INFO\",\"value\":\"";
						trace += traceClean;
						trace += "\"}";
						break;
#endif
					default:
						trace = "";
						break;
					}
					if (trace.GetLength() > 0)
					{
						size = trace.ToBlock(tmpData,1024,VTC_UTF_8,false,false);
						err = fWS->WriteMessage(tmpData,size,true);
						xbox_assert( err == VE_OK );
						if (err != VE_OK)
						{
							fWS->Close();
							fState = STOPPED_STATE;
						}
					}
				}
			}
			break;
		case STOPPED_STATE:
			break;
		default:
			break;
		}
	}

	fSem.Unlock();
}

/*sLONG LogHandler::InnerTaskProc(XBOX::VTask* inTask)
{
	typedef enum TraceState_enum {
		STOPPED_STATE,
		STARTED_STATE,
		WORKING_STATE,
	} TraceState_t;

	TraceState_t			l_state = STARTED_STATE;
	LogHandler*				l_this = (LogHandler*)inTask->GetKindData();
	XBOX::VError			err;

	switch(l_state)
	{
	case STARTED_STATE:
		err = l_this->fWS->TreatNewConnection(l_msg.response);
		if (!err)
		{
				l_state = WORKING_STATE;
		}
		break;
	default:
	}
	return 0;
}*/

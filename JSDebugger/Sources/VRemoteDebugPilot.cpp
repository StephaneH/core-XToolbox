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

#include "JavaScript/VJavaScript.h"

#include "VRemoteDebugPilot.h"

USING_TOOLBOX_NAMESPACE

#if GUY_DUMP_DEBUG_MSG
#define DEBUG_MSG(x)	DebugMsg((x))
#else
#define DEBUG_MSG(x)
#endif


extern VLogHandler*			sPrivateLogHandler;

const XBOX::VString			K_CHR_DBG_ROOT("/devtools");

sLONG						VRemoteDebugPilot::sRemoteDebugPilotContextId=1;


static bool VRemoteDebugPilotIsInit = false;
VRemoteDebugPilot::VRemoteDebugPilot(CHTTPServer* inHTTPServer, bool inV8Mode ) :
		fTmpData(NULL), 
		fPendingContextsNumberSem(0,1000000),
		fDebuggingEventsTimeStamp(1),
		fLogListener(this),
		fV8Mode(inV8Mode)
{
	
	fHTTPServer = inHTTPServer;
	if (!VRemoteDebugPilotIsInit)
	{
		VRemoteDebugPilotIsInit = true;
	}
	fWS = NULL;
	fTask = new XBOX::VTask(this, 64000, XBOX::eTaskStylePreemptive, &VRemoteDebugPilot::InnerTaskProc);
	if (fTask)
	{
		fTask->SetName("Remote Debug Pilot");
		fTask->SetKindData((sLONG_PTR)this);
		fTask->Run();
	}
}

VRemoteDebugPilot::~VRemoteDebugPilot()
{
	//TBC when destructor will be called -> will pass the state to stopped
}

#define	K_DBG_PROTOCOL_OK_STR			"{\"result\":\"ok\","
#define K_DBG_PROTOCOL_CONNECT_STR		"{\"method\":\"connect\",\"id\":\""
#define K_DBG_PROTOCOL_GET_URL_STR		"{\"method\":\"getURL\",\"id\":\""
#define K_DBG_PROTOCOL_DISCONNECT_STR	"{\"method\":\"disconnect\",\"id\":\""
#define K_DBG_PROTOCOL_ABORT_STR		"{\"method\":\"abort\",\"id\":\""

void VRemoteDebugPilot::AbortContexts(bool inOnlyDebuggedOnes)
{
	std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator ctxIt = fContextArray.begin();

	while( ctxIt != fContextArray.end() )
	{
		if (fState == STARTED_STATE)
		{
			if ((*ctxIt).second->fSem)
			{
				(*ctxIt).second->fSem->Unlock();
				ReleaseRefCountable(&(*ctxIt).second->fSem);
			}
		}
		else
		{
			if (!(*ctxIt).second->fPage)
			{
				if ((*ctxIt).second->fSem)
				{
					if (!inOnlyDebuggedOnes)
					{
						(*ctxIt).second->fSem->Unlock();
						ReleaseRefCountable(&(*ctxIt).second->fSem);
					}
				}
				ctxIt++;
				continue;
			}
			VChromeDbgHdlPage*	page = (*ctxIt).second->fPage;
			(void)testAssert(page->Abort() == VE_OK);
		}
		ctxIt->second->Release();
		ctxIt++;
	}
	//if (fState == STARTED_STATE)
	{
		fContextArray.clear();
	}
}

VRemoteDebugPilot::VContextDescriptor::~VContextDescriptor()
{
	fUpdated = false;
	fAborted = false;
	//testAssert( (void*)fSem == NULL );
	ReleaseRefCountable(&fSem);
	(void)fPage;

	if (fContextRef)
	{
		VJSContext					vjsCtx((JS4D::ContextRef)fContextRef);
		const VJSGlobalContext*		vjsGlobalContext = vjsCtx.GetGlobalContext();
		if ( vjsGlobalContext )
		{
			VJSWorker*		vjsWkr = VJSWorker::RetainWorker(vjsCtx);
			if (vjsWkr)
			{
				vjsWkr->Terminate();
				vjsWkr->Release();
			}
		}
	}
	ReleaseRefCountable(&fTraceContainer);
};

void VRemoteDebugPilot::RemoveContext(std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator& inCtxIt,
									  bool																inJSCtxIsValid)
{
	XBOX::VError			err;
	XBOX::VString			resp;

	if (fState >= STARTED_STATE)
	{
		// ctx could already have been removed by ABORT
		if (inCtxIt != fContextArray.end())
		{
			if (!inJSCtxIsValid)
			{
				(*inCtxIt).second->fContextRef = NULL;
			}

			if ((*inCtxIt).second->fPage)
			{
				ReleaseRefCountable(&(*inCtxIt).second->fPage);
			}

			if (fState == CONNECTED_STATE)
			{
				resp = CVSTR("{\"method\":\"removeContext\",\"contextId\":\"");
				resp.AppendLong8((sLONG8)((*inCtxIt).first));
				resp += CVSTR("\",\"id\":\"");
				resp += fClientId;
				resp += CVSTR("\"}");
				err = SendToBrowser( resp );
				if (err)
				{
					fWS->Close();
					fState = STARTED_STATE;
				}
			}
			inCtxIt->second->Release();
			fContextArray.erase(inCtxIt);
		}
	}
}

void VRemoteDebugPilot::DisconnectBrowser()
{
	fWS->Close();

	std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator	ctxIt = fContextArray.begin();
	while( ctxIt != fContextArray.end()) 
	{
		VChromeDbgHdlPage*	page = (*ctxIt).second->fPage;
		if (page)
		{
			page->Stop();
			ReleaseRefCountable(&(*ctxIt).second->fPage);
		}
		ctxIt++;
	}
	fState = STARTED_STATE;

}


void VRemoteDebugPilot::TreatSendBreakpointsUpdated()
{
	XBOX::VString				brkptsStr;
	XBOX::VString				resp;

	if (fState == CONNECTED_STATE)
	{
		VChromeDebugHandler::GetJSONBreakpoints(brkptsStr);
		
		resp = CVSTR("{\"method\":\"breakpointsUpdate\",\"id\":\"");
		resp += fClientId;
		resp += CVSTR("\",\"breakpoints\":");
		resp += brkptsStr;
		resp += "}";
		SendToBrowser( resp );
	}
}

void VRemoteDebugPilot::TreatTraces()
{
	fTracesLock.Lock();
	if (fState < STARTED_STATE)
	{
		for( std::vector<const VValueBag*>::size_type idx = 0; idx < fTraces.size(); idx++ )
		{
			fTraces[idx]->Release();
		}
	}
	else
	{
		VChromeDbgHdlPage*	page = NULL;

		//std::map< VChromeDbgHdlPage*, std::vector<const VValueBag*> >	tracesMap;
		for( std::vector<const VValueBag*>::size_type idx = 0; idx < fTraces.size(); idx++ )
		{
			sLONG8	debugContext;
			if (ILoggerBagKeys::debug_context.Get( fTraces[idx], debugContext) )
			{
				std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator	ctxIt = fContextArray.find((OpaqueDebuggerContext)debugContext);
				if (ctxIt != fContextArray.end())
				{
					/*page = (*ctxIt).second->fPage;
					if (page)
					{
						std::map< VChromeDbgHdlPage*, std::vector<const VValueBag*> >::iterator itContext = tracesMap.find(page);
						if (itContext == tracesMap.end())
						{
							tracesMap.insert(std::pair< VChromeDbgHdlPage*, std::vector<const VValueBag*> >( page, std::vector<const VValueBag*>() ) );
							itContext = tracesMap.find(page);
						}
						if (testAssert(itContext != tracesMap.end()))
						{
							//(*ioMsg.values)[idx]->Retain();
							(*itContext).second.push_back(fTraces[idx]);
							fTraces[idx] = NULL;
						}
					}*/
					(*ctxIt).second->fTraceContainer->Put(fTraces[idx]);
					fTraces[idx] = NULL;
				}
			}
			if (fTraces[idx])
			{
				fTraces[idx]->Release();
			}
		}
		/*std::map< VChromeDbgHdlPage*, std::vector<const VValueBag*> >::iterator itDebugContexts = tracesMap.begin();
		while(itDebugContexts != tracesMap.end() )
		{
			(*itDebugContexts).first->SendTraces((*itDebugContexts).second);
			itDebugContexts++;
		}*/
	
	}
	fTraces.clear();
	fTracesLock.Unlock();
}

void VRemoteDebugPilot::HandleDisconnectingState()
{
	XBOX::VError			err;
	VSize					msgLen;
	bool					isMsgTerminated;
	sBYTE*					tmpStr;

	msgLen = K_MAX_SIZE;
	tmpStr = NULL;

	err = fWS->ReadMessage((void*)fTmpData,msgLen,isMsgTerminated, 0);
	if (err)
	{
		DebugMsg("VRemoteDebugPilot::HandleDisconnectingState fWS->ReadMessage pb\n");
		//AbortContexts(true);
		DisconnectBrowser();
		return;
	}
	if (msgLen)
	{
		if (msgLen < K_MAX_SIZE)
		{
			fTmpData[msgLen] = 0;
		}
		else
		{
			fTmpData[K_MAX_SIZE-1] = 0;
		}

		DebugMsg("VRemoteDebugPilot::HandleDisconnectingState received msg(len=%d)='%s'\n",msgLen,fTmpData);

		tmpStr = (sBYTE*)strstr((const char*)fTmpData,K_DBG_PROTOCOL_DISCONNECT_STR);
	}
	else
	{
		XBOX::VTime			curtTime;
		XBOX::VDuration		disconnectDuration;
		curtTime.Substract(fDisconnectTime,disconnectDuration);
		if (disconnectDuration.ConvertToSeconds() > 2)
		{
			tmpStr = fTmpData;
		}
	}
	if (tmpStr)
	{
		//AbortContexts(true);
		DisconnectBrowser();
		return;
	}
}

void VRemoteDebugPilot::HandleConnectingState()
{
	XBOX::VError			err;
	VSize					msgLen;
	bool					isMsgTerminated;
	sBYTE*					tmpStr;
	sBYTE*					endStr;
	unsigned long long		id;
	XBOX::VString			resp;

	msgLen = K_MAX_SIZE;
	err = fWS->ReadMessage((void*)fTmpData,msgLen,isMsgTerminated, 0);
	if (err)
	{
		DebugMsg("VRemoteDebugPilot::HandleConnectingState fWS->ReadMessage pb\n");
	}
	/*else
	{
		if (!msgLen)
		{
			DebugMsg("VRemoteDebugPilot::HandleConnectingState no data\n");
		}
	}*/

	if (!err && msgLen)
	{
		if (msgLen < K_MAX_SIZE)
		{
			fTmpData[msgLen] = 0;
		}
		else
		{
			fTmpData[K_MAX_SIZE-1] = 0;
		}

		DebugMsg("VRemoteDebugPilot::HandleConnectingState received msg(len=%d)='%s'\n",msgLen,fTmpData);

		tmpStr = (sBYTE*)strstr((const char*)fTmpData,K_DBG_PROTOCOL_CONNECT_STR);
		if (tmpStr && !fClientId.GetLength())
		{
			if (tmpStr == fTmpData)
			{
				tmpStr += strlen(K_DBG_PROTOCOL_CONNECT_STR);
				endStr = (sBYTE*)strchr((const char*)tmpStr, '"');
				if (endStr)
				{
					*endStr = 0;
					resp = CVSTR("{\"result\":\"ok\",\"solution\":\"");
					resp += fSolutionName;
					resp += ("\",\"needsAuthentication\":");
					resp += (fNeedsAuthentication ? CVSTR("true") : CVSTR("false"));
					resp += (",\"id\":\"");
					fClientId = VString(tmpStr);
					resp += fClientId;
					resp += CVSTR("\"}");
					err = SendToBrowser(resp);
					if (!err)
					{
						if (fNeedsAuthentication)
						{
							fState = DISCONNECTING_STATE;
							fDisconnectTime.FromSystemTime();
							return;
						}
						else
						{
							//DisplayContexts();

							// here the client connection has been accepted without need of authentication
							// we send the already existing contexts
							std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator	ctxIt = fContextArray.begin();
							while (!err && (ctxIt != fContextArray.end()))
							{
								resp = CVSTR("{\"method\":\"newContext\",\"contextId\":\"");
								resp.AppendLong8((sLONG8)((*ctxIt).first));
								resp += CVSTR("\",\"id\":\"");
								resp += fClientId;
								resp += CVSTR("\"}");
								err = SendToBrowser(resp);

								if (!err && (*ctxIt).second->fUpdated)
								{
									if ((*ctxIt).second->fSem == NULL)
									{
										(*ctxIt).second->fSem = new XBOX::VSemaphore(0);
										(*ctxIt).second->fSem->Retain();
									}
									err = SendContextToBrowser(ctxIt);
								}
								ctxIt++;
							}
							if (!err)
							{
								fState = CONNECTED_STATE;
							}
						}
					}
				}
				else
				{
					err = VE_INVALID_PARAMETER;
				}
			}
			else
			{
				xbox_assert(false);
				DebugMsg("VRemoteDebugPilot::HandleConnectingState BAD MESSAGE received msg2(len=%d)='%s'\n", msgLen, fTmpData);
				err = VE_INVALID_PARAMETER;
			}
		}
		else
		{
			DEBUG_MSG("VRemoteDebugPilot::HandleConnectingState first message shoud be 'connect'\n");
			err = VE_INVALID_PARAMETER;
		}
	}

	if (err)
	{
		DEBUG_MSG("VRemoteDebugPilot::HandleConnectingState  pb\n");
		fWS->Close();
		fState = STARTED_STATE;
	}
	else
	{
		TreatSendBreakpointsUpdated();
	}
}

void VRemoteDebugPilot::HandleConnectedState()
{
	XBOX::VError			err;
	VSize					msgLen;
	bool					isMsgTerminated;
	sBYTE*					tmpStr;
	sBYTE*					endStr;
	unsigned long long		id;
	XBOX::VString			resp;

	msgLen = K_MAX_SIZE;
	err = fWS->ReadMessage((void*)fTmpData,msgLen,isMsgTerminated, 0);
	if (err)
	{
		DebugMsg("VRemoteDebugPilot::HandleConnectedState fWS->ReadMessage pb\n");
		DisconnectBrowser();
		fState = STARTED_STATE;
		return;
	}
	if (msgLen)
	{
		if (msgLen < K_MAX_SIZE)
		{
			fTmpData[msgLen] = 0;
		}
		else
		{
			fTmpData[K_MAX_SIZE-1] = 0;
		}

		DebugMsg("VRemoteDebugPilot::HandleConnectedState received msg(len=%d)='%s'\n",msgLen,fTmpData);

		tmpStr = strstr(fTmpData,K_DBG_PROTOCOL_ABORT_STR);
		if (tmpStr && fClientId.GetLength())
		{
			if (testAssert(tmpStr == fTmpData))
			{
				tmpStr += strlen(K_DBG_PROTOCOL_ABORT_STR);
				endStr = strchr(tmpStr, '"');
				*endStr = 0;
				id = atoi(tmpStr);
				//printf("VRemoteDebugPilot::HandleConnectedState id=%lld fTmpData=%s\n", id, fTmpData);
				std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator	ctxIt = fContextArray.find((OpaqueDebuggerContext)id);
				if (testAssert(ctxIt != fContextArray.end()))
				{
					if ( (*ctxIt).second->fPage == NULL)
					{
						if ((*ctxIt).second->fSem)
						{
							(*ctxIt).second->fSem->Unlock();
							ReleaseRefCountable(&(*ctxIt).second->fSem);
						}
						(*ctxIt).second->fUpdated = false;
						(*ctxIt).second->fAborted = true;
						//RemoveContext(ctxIt);
					}
					else
					{
						VChromeDbgHdlPage*	l_page = (*ctxIt).second->fPage;
						(void)testAssert(l_page->Abort() == VE_OK);
					}
					//RemoveContext(id);
				}
			}
			else
			{
				DebugMsg("VRemoteDebugPilot::HandleConnectedState BAD MESSAGE received msg2(len=%d)='%s'\n", msgLen, fTmpData);
				err = VE_INVALID_PARAMETER;
				fWS->Close();
				fState = STARTED_STATE;
				return;
			}
		}
		else
		{
			tmpStr = strstr(fTmpData,K_DBG_PROTOCOL_GET_URL_STR);
			if (tmpStr && fClientId.GetLength())
			{
				if (testAssert(tmpStr == fTmpData))
				{
					tmpStr += strlen(K_DBG_PROTOCOL_GET_URL_STR);
					endStr = strchr(tmpStr, '"');
					*endStr = 0;
					id = atoi(tmpStr);
					std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator	ctxIt = fContextArray.find((OpaqueDebuggerContext)id);
					if (testAssert(ctxIt != fContextArray.end()))
					{
						// test if a a page has already been attributed to this context
						if (!(*ctxIt).second->fPage)
						{
							xbox_assert((*ctxIt).second->fTraceContainer);
							(*ctxIt).second->fPage = new VChromeDbgHdlPage(fSolutionName, (*ctxIt).second->fTraceContainer,fV8Mode);
							(*ctxIt).second->fPage->Init(id, fHTTPServer, NULL);
							err = (*ctxIt).second->fPage->Start((OpaqueDebuggerContext)id);
						}

						if (!err)
						{
							resp = CVSTR("{\"method\":\"setURLContext\",\"contextId\":\"");
							resp.AppendLong8(id);
							resp += CVSTR("\",\"url\":\"");
							resp.AppendLong8((*ctxIt).second->fPage->GetPageNb());
							resp += CVSTR("\",\"id\":\"");
							resp += fClientId;
							resp += CVSTR("\"}");
							err = SendToBrowser(resp);
						}

						if (err)
						{
							fWS->Close();
							fState = STARTED_STATE;
							return;
						}
					}
					else
					{
						DebugMsg("VRemoteDebugPilot::HandleConnectedState unknown context = %d\n", id);
					}
				}
				else
				{
					DebugMsg("VRemoteDebugPilot::HandleConnectedState BAD MESSAGE received msg2(len=%d)='%s'\n", msgLen, fTmpData);
					err = VE_INVALID_PARAMETER;
					fWS->Close();
					fState = STARTED_STATE;
					return;
				}
			}
			else
			{
				tmpStr = strstr(fTmpData, K_DBG_PROTOCOL_OK_STR);
				if (!testAssert(tmpStr == fTmpData))
				{
					DebugMsg("VRemoteDebugPilot::HandleConnectedState BAD MESSAGE received msg2(len=%d)='%s'\n", msgLen, fTmpData);
					err = VE_INVALID_PARAMETER;
					fWS->Close();
					fState = STARTED_STATE;
					return;
				}
			}
		}
	}
}

VRemoteDebugPilot::VPilotLogListener::VPilotLogListener(VRemoteDebugPilot* inPilot)
{
	fPilot = inPilot;
}

void VRemoteDebugPilot::VPilotLogListener::Put( std::vector<const VValueBag*>& inValuesVector )
{
	//RemoteDebugPilotMsg_t	msg;
	if (inValuesVector.size() > 0)
	{
		fPilot->fTracesLock.Lock();
		//msg.values = new std::vector<const VValueBag*>;
		for( std::vector<const VValueBag*>::size_type idx = 0; idx < inValuesVector.size(); idx++ )
		{
			inValuesVector[idx]->Retain();
			fPilot->fTraces.push_back(inValuesVector[idx]);
			//msg.values->push_back(inValuesVector[idx]);
		}
		fPilot->fTracesLock.Unlock();
		//msg.type = PUT_TRACES_MSG;
		//fPilot->Send(msg);
	}
}


sLONG VRemoteDebugPilot::InnerTaskProc(XBOX::VTask* inTask)
{

	VRemoteDebugPilot*		l_this = (VRemoteDebugPilot*)inTask->GetKindData();

	l_this->fTmpData = new sBYTE[K_MAX_SIZE];
	l_this->fState = INIT_STATE;

//wait_connection:
	
	while(!inTask->IsDying(NULL)) 
	{
		l_this->TreatTraces();

		if (l_this->fState == DISCONNECTING_STATE)
		{
			l_this->fLock.Lock();
			l_this->HandleDisconnectingState();
			l_this->fLock.Unlock();
		}
		if (l_this->fState == CONNECTING_STATE)
		{
			l_this->fLock.Lock();
			l_this->HandleConnectingState();
			l_this->fLock.Unlock();
		}
		
		if (l_this->fState == CONNECTED_STATE)
		{
			l_this->fLock.Lock();
			l_this->HandleConnectedState();
			l_this->fLock.Unlock();
		}

		inTask->ExecuteMessagesWithTimeout(50);

	}
	DEBUG_MSG("VRemoteDebugPilot::InnerTaskProc quits \n");
	return 0;
}

XBOX::VError VRemoteDebugPilot::TreatRemoteDbg(IHTTPResponse* ioResponse, bool inNeedsAuthentication)
{

	IAuthenticationInfos*	authInfos = ioResponse->GetRequest().GetAuthenticationInfos();
	VString					userName;
	authInfos->GetUserName(userName);

	VError						err = VE_INVALID_PARAMETER;
	
	VPilotTreatNewClientMessage*		treatNewClientMsg = new VPilotTreatNewClientMessage(this);
	treatNewClientMsg->fNeedsAuthentication = inNeedsAuthentication;
	treatNewClientMsg->fResponse = ioResponse;

	treatNewClientMsg->SendTo( fTask );
	err = treatNewClientMsg->fStatus;
	ReleaseRefCountable(&treatNewClientMsg);

	return XBOX::VE_OK;

}


WAKDebuggerServerMessage* VRemoteDebugPilot::WaitFrom(OpaqueDebuggerContext inContext)
{

	WAKDebuggerServerMessage*	res = NULL;
	VChromeDbgHdlPage*			page = NULL;
	VError						err = VE_INVALID_PARAMETER;

	VPilotWaitFromMessage*		waitFromMsg = new VPilotWaitFromMessage(this);
	waitFromMsg->fPage = &page;
	waitFromMsg->fCtx = inContext;

	waitFromMsg->SendTo( fTask );
	err = waitFromMsg->fStatus;
	ReleaseRefCountable(&waitFromMsg);

	if (!err && page)
	{
		fPendingContextsNumberSem.Unlock();
		res = page->WaitFrom();
		fLock.Lock();
		if (res->fType == WAKDebuggerServerMessage::SRV_CONNEXION_INTERRUPTED)
		{
			std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator	ctxIt = fContextArray.find(inContext);
			if (ctxIt != fContextArray.end())
			{
				ReleaseRefCountable(&(*ctxIt).second->fPage);
			}
		}
		ReleaseRefCountable(&page);
		fLock.Unlock();
		fPendingContextsNumberSem.Lock();
	}
	else
	{
		//int a=1;
	}
	return res;

}

XBOX::VError VRemoteDebugPilot::SendEval( OpaqueDebuggerContext inContext, const XBOX::VString& inEvaluationResult, const XBOX::VString& inRequestId )
{
	VError	err = VE_INVALID_PARAMETER;

	fLock.Lock();
	std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator	ctxIt = fContextArray.find(inContext);
	
	if (testAssert(ctxIt != fContextArray.end()))
	{
		VChromeDbgHdlPage*	page = (*ctxIt).second->fPage;
		if ( page  )
		{
			err = page->Eval(inEvaluationResult,inRequestId);
		}
	}

	fLock.Unlock();
	return err;
}


XBOX::VError VRemoteDebugPilot::SendLookup( OpaqueDebuggerContext inContext, const XBOX::VString& inLookupResult )
{
	VError	err = VE_INVALID_PARAMETER;

	fLock.Lock();
	std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator	ctxIt = fContextArray.find(inContext);

	if (ctxIt != fContextArray.end())
	{
		VChromeDbgHdlPage*	page = (*ctxIt).second->fPage;
		if ( page )
		{
			err = page->Lookup(inLookupResult);
		}
	}
	fLock.Unlock();

	return err;
}

XBOX::VError VRemoteDebugPilot::SendCallStack(
											OpaqueDebuggerContext				inContext,
											const XBOX::VString&				inCallstackStr,
											const XBOX::VString&				inExceptionInfosStr,
											const CallstackDescriptionMap&		inCallstackDesc )
{
	VError	err = VE_INVALID_PARAMETER;

	fLock.Lock();
	std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator	ctxIt = fContextArray.find(inContext);

	if (testAssert(ctxIt != fContextArray.end()))
	{
		VChromeDbgHdlPage*	page = (*ctxIt).second->fPage;
		if ( page )
		{
			err = page->Callstack(inCallstackStr,inExceptionInfosStr,inCallstackDesc);
		}
	}
	fLock.Unlock();
	return err;
}


XBOX::VError VRemoteDebugPilot::BreakpointReached(
											OpaqueDebuggerContext		inContext,
											int							inLineNumber,
											XBOX::VString&				inSourceLineText,
											RemoteDebuggerPauseReason	inDebugReason,
											XBOX::VString&				inException,
											XBOX::VString&				inSourceUrl,
											bool&						outAbort,
											bool						inIsNew)
{
	VError						err = VE_INVALID_PARAMETER;
	VChromeDbgHdlPage*			page = NULL;
	XBOX::VSemaphore*			sem = NULL;
	VContextDebugInfos			debugInfos;
	
	debugInfos.fFileName = inSourceUrl;

	VURL::Decode( debugInfos.fFileName );

	debugInfos.fLineNb = inLineNumber+1;
	debugInfos.fReason = ( inDebugReason == EXCEPTION_RSN ? "exception" : (inDebugReason == BREAKPOINT_RSN ? "breakpoint" : "dbg statement" ) );
	debugInfos.fComment = "To Be Completed";
	debugInfos.fSourceLine = inSourceLineText;
	err = ContextUpdated(inContext, debugInfos, &sem, inIsNew);

	if (err == VE_OK)
	{
		fPendingContextsNumberSem.Unlock();
		// if sem not NULL then wait for a browser page granted for debug
		if (sem)
		{
			sem->Lock();
			ReleaseRefCountable(&sem);
		}

		VPilotBreakpointReachedMessage*		breakpointReachedMsg
					= new VPilotBreakpointReachedMessage(this,outAbort);
		breakpointReachedMsg->fCtx = inContext;
		breakpointReachedMsg->fLineNb = inLineNumber;
		breakpointReachedMsg->fDataStr = inException;
		breakpointReachedMsg->fUrlStr = inSourceUrl;
		breakpointReachedMsg->SendTo( fTask );
		err = breakpointReachedMsg->fStatus;
		ReleaseRefCountable(&breakpointReachedMsg);
		if (err)
		{
			DebugMsg("VRemoteDebugPilot::BreakpointReached breakpointReachedMsg pb\n");
		}
		//testAssert(err == VE_OK);
		fPendingContextsNumberSem.Lock();

	}
	VString trace = VString("VRemoteDebugPilot::BreakpointReached ctx activation pb for ctxid=");
	trace.AppendLong8((unsigned long long)inContext);
	trace += " ,ok=";
	trace += (!err ? "1" : "0" );
	sPrivateLogHandler->Put( (err ? WAKDBG_ERROR_LEVEL : WAKDBG_INFO_LEVEL),trace);
	if (err)
	{
		DebugMsg("<%S>\n",&trace);
	}
	return err;

}

XBOX::VError VRemoteDebugPilot::TreatWS(IHTTPResponse* ioResponse, uLONG inPageNb )
{
	VError						err = VE_INVALID_PARAMETER;
	VChromeDbgHdlPage*			page = NULL;
	XBOX::VSemaphore*			sem = NULL;
	
	VPilotTreatWSMessage*		treatWSMsg = new VPilotTreatWSMessage(this);
	treatWSMsg->fPageNb = inPageNb;
	treatWSMsg->fPage = &page;
	treatWSMsg->fSem = &sem;
	treatWSMsg->SendTo( fTask );
	err = treatWSMsg->fStatus;
	ReleaseRefCountable(&treatWSMsg);
	
	if (testAssert(err == VE_OK))
	{
		if (testAssert(sem && page))
		{
			// page and sem are already retained
			// the debugging page WebSocket is handled thanks to the (HTTP server) calling thread
			if (page->TreatWS(ioResponse,sem))
			{
				
			}
			//fLock.Lock();
			ReleaseRefCountable(&sem);
			ReleaseRefCountable(&page);
			//fLock.Unlock();
			return VE_OK;
		}
	}

	return VE_HTTP_PROTOCOL_INTERNAL_SERVER_ERROR;
}

XBOX::VError VRemoteDebugPilot::NewContext(void* inContextRef, OpaqueDebuggerContext* outContext)
{
	VError						err = VE_INVALID_PARAMETER;
	VChromeDbgHdlPage*			page = NULL;
	XBOX::VSemaphore*			sem = NULL;
	
	VPilotNewContextMessage*		newContextMsg = new VPilotNewContextMessage(this);
	newContextMsg->fOutCtx = outContext;
	newContextMsg->fContextRef = inContextRef;
	newContextMsg->SendTo( fTask );
	err = newContextMsg->fStatus;
	ReleaseRefCountable(&newContextMsg);

	if (!err)
	{
		return err;
	}
	xbox_assert(false);//TBC
	return err;
}

XBOX::VError VRemoteDebugPilot::ContextUpdated(	OpaqueDebuggerContext		inContext,
												const VContextDebugInfos&	inDebugInfos,
												XBOX::VSemaphore**			outSem,
												bool						inIsNew)
{
	VError							err = VE_INVALID_PARAMETER;
	VPilotUpdateContextMessage*		updateContextMsg = new VPilotUpdateContextMessage(this);
	
	updateContextMsg->fCtx = inContext;
	updateContextMsg->fOutsem = outSem;
	updateContextMsg->fDebugInfos = inDebugInfos;
	updateContextMsg->fIsNew = inIsNew;

	updateContextMsg->SendTo( fTask );
	err = updateContextMsg->fStatus;
	ReleaseRefCountable(&updateContextMsg);
	return err;

}
XBOX::VError VRemoteDebugPilot::ContextRemoved(OpaqueDebuggerContext inContext)
{
	VError							err = VE_INVALID_PARAMETER;
	VPilotRemoveContextMessage*		removeContextMsg = new VPilotRemoveContextMessage(this);
	
	removeContextMsg->fCtx = inContext;
	removeContextMsg->SendTo( fTask );
	err = removeContextMsg->fStatus;
	ReleaseRefCountable(&removeContextMsg);
	return err;

}
XBOX::VError VRemoteDebugPilot::DeactivateContext( OpaqueDebuggerContext inContext, bool inHideOnly )
{
	VError								err = VE_INVALID_PARAMETER;
	VPilotDeactivateContextMessage*		deactivateContextMsg = new VPilotDeactivateContextMessage(this);
	
	deactivateContextMsg->fCtx = inContext;
	deactivateContextMsg->fHideOnly = inHideOnly;
	deactivateContextMsg->SendTo( fTask );
	err = deactivateContextMsg->fStatus;
	ReleaseRefCountable(&deactivateContextMsg);
	return err;

}
XBOX::VError VRemoteDebugPilot::Start()
{

	VError					err = VE_INVALID_PARAMETER;
	VPilotStartMessage*		startMsg = new VPilotStartMessage(this);
	startMsg->SendTo( fTask );
	err = startMsg->fStatus;
	ReleaseRefCountable(&startMsg);
	return err;
}

XBOX::VError VRemoteDebugPilot::Stop()
{

	VError					err = VE_INVALID_PARAMETER;
	VPilotStopMessage*		stopMsg = new VPilotStopMessage(this);
	stopMsg->SendTo( fTask );
	err = stopMsg->fStatus;
	ReleaseRefCountable(&stopMsg);
	return err;
}

XBOX::VError VRemoteDebugPilot::AbortAll()
{

	VError						err = VE_INVALID_PARAMETER;
	VPilotAbortAllMessage*		abortAllMsg = new VPilotAbortAllMessage(this);
	abortAllMsg->SendTo( fTask );
	err = abortAllMsg->fStatus;
	ReleaseRefCountable(&abortAllMsg);
	return err;
}

void VRemoteDebugPilot::SendBreakpointsUpdated()
{

	VPilotSendBreakpointsUpdateMessage*		sendBrkptMsg = new VPilotSendBreakpointsUpdateMessage(this);
	sendBrkptMsg->PostTo( fTask );
	ReleaseRefCountable(&sendBrkptMsg);
}

void VRemoteDebugPilot::VPilotSendBreakpointsUpdateMessage::DoExecute()
{
	fPilot->fLock.Lock();
	fPilot->TreatSendBreakpointsUpdated();
	fPilot->fLock.Unlock();
}

void VRemoteDebugPilot::VPilotStartMessage::DoExecute()
{
	fPilot->fLock.Lock();
	switch(fPilot->fState)
	{
	case INIT_STATE:
		fPilot->fState = STARTED_STATE;
		VProcess::Get()->GetLogger()->AddLogListener(&fPilot->fLogListener);
	case STARTED_STATE:
		break;
	case CONNECTED_STATE:
	case CONNECTING_STATE:
	case DISCONNECTING_STATE:
	case STOPPED_STATE:
		// should not happen
		break;
	}
	fPilot->fLock.Unlock();
	fStatus = VE_OK;
}

void VRemoteDebugPilot::VPilotTreatWSMessage::DoExecute()
{
	fPilot->fLock.Lock();
	if (testAssert(fPilot->fState == CONNECTED_STATE))
	{
		std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator	ctxIt = fPilot->fContextArray.begin();
		while( ctxIt != fPilot->fContextArray.end() )
		{
			if ((*ctxIt).second->fPage)
			{
				if ((*ctxIt).second->fPage->GetPageNb() == fPageNb)
				{
					fStatus = VE_OK;
					(*ctxIt).second->fPage->Retain();
					*fPage = (*ctxIt).second->fPage;
					*fSem = (*ctxIt).second->fSem;
					(*ctxIt).second->fSem = NULL;
					break;
				}
			}
			ctxIt++;
		}
		if (!testAssert(fStatus == VE_OK))
		{
			DebugMsg("VRemoteDebugPilot::VPilotTreatWSMessage page %d not found or not active\n", fPageNb);
		}
	}

	fPilot->fLock.Unlock();
}

void VRemoteDebugPilot::VPilotStopMessage::DoExecute()
{
	XBOX::VError	err;

	fPilot->fLock.Lock();	
	
	switch(fPilot->fState)
	{
	case STOPPED_STATE:
	case INIT_STATE:
		fPilot->fState = INIT_STATE;
		break;
	case CONNECTED_STATE:
	case CONNECTING_STATE:
	case DISCONNECTING_STATE:
		VProcess::Get()->GetLogger()->RemoveLogListener(&fPilot->fLogListener);
		fPilot->AbortContexts(false);
		err = fPilot->fWS->Close();
		fPilot->fState = INIT_STATE;
		//fPilot->fClientCompletedSemaphore.Unlock();// there is only a caller to unlock on thyese states
		break;
	case STARTED_STATE:
		VProcess::Get()->GetLogger()->RemoveLogListener(&fPilot->fLogListener);
		fPilot->AbortContexts(false);
		fPilot->fState = INIT_STATE;
		break;
	}

	fPilot->fLock.Unlock();
	fStatus = VE_OK;
}

void VRemoteDebugPilot::VPilotAbortAllMessage::DoExecute()
{
	fPilot->fLock.Lock();

	switch(fPilot->fState)
	{
	case STOPPED_STATE:
	case INIT_STATE:
		break;
	case CONNECTED_STATE:
	case CONNECTING_STATE:
	case DISCONNECTING_STATE:
	case STARTED_STATE:
		fPilot->AbortContexts(false);
	}
	fPilot->fLock.Unlock();
	fStatus = VE_OK;
}

void VRemoteDebugPilot::VPilotTreatNewClientMessage::DoExecute()
{
	XBOX::VError			err;
	
	fPilot->fLock.Lock();
	switch(fPilot->fState)
	{
	case STARTED_STATE:
		err = fPilot->fWS->TreatNewConnection(fResponse,true);
		if (!err)
		{
			fPilot->fNeedsAuthentication = fNeedsAuthentication;
			fPilot->fState = CONNECTING_STATE;
			fPilot->fClientId = VString("");
			fStatus = VE_OK;
		}
		break;
	case STOPPED_STATE:
	case INIT_STATE:
#if GUY_DUMP_DEBUG_MSG
		DebugMsg("VRemoteDebugPilot::TreatNewClientMsg NEW_CLIENT SHOULD NOT HAPPEN in state=%d !!!\n",fPilot->fState);
#endif
		break;
	case CONNECTED_STATE:
	case DISCONNECTING_STATE:
	case CONNECTING_STATE:
#if GUY_DUMP_DEBUG_MSG
		DebugMsg("VRemoteDebugPilot::TreatNewClientMsg already in use, NEW_CLIENT SHOULD NOT HAPPEN in state=%d !!!\n",fPilot->fState);
#endif
		xbox_assert(false);
		break;
	}
	fPilot->fLock.Unlock();

}
void VRemoteDebugPilot::VPilotWaitFromMessage::DoExecute()
{
	fPilot->fLock.Lock();

	std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator	ctxIt = fPilot->fContextArray.find(fCtx);

	if (testAssert(ctxIt != fPilot->fContextArray.end()))
	{
		if (fPilot->fState == CONNECTED_STATE)
		{
			if ( (*ctxIt).second->fPage )
			{
				fStatus = VE_OK;
				*fPage = (*ctxIt).second->fPage;
				(*ctxIt).second->fPage->Retain();
			}
		}
	}

	fPilot->fLock.Unlock();
}

void VRemoteDebugPilot::VPilotBreakpointReachedMessage::DoExecute()
{
	fAbort = false;
	fPilot->fLock.Lock();

	std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator	ctxIt = fPilot->fContextArray.find(fCtx);

	if (ctxIt != fPilot->fContextArray.end())
	{
		if (fPilot->fState == CONNECTED_STATE)
		{
			if ( testAssert((*ctxIt).second->fPage) )
			{
				VChromeDbgHdlPage*	page = (*ctxIt).second->fPage;
				{
					fStatus = page->BreakpointReached(fLineNb,fUrlStr,fDataStr);
				}
			}
			else
			{
				fStatus = VE_OK;
				fAbort = (*ctxIt).second->fAborted;
				VError err = fPilot->SendContextToBrowser(ctxIt);
				if (err)
				{
					fPilot->fWS->Close();
					fPilot->fState = STARTED_STATE;
				}
			}
		}
		//testAssert(fStatus == VE_OK);
	}
	else
	{
#if GUY_DUMP_DEBUG_MSG
		DebugMsg("VRemoteDebugPilot::VPilotBreakpointReachedMessage::DoExecute ctx %d not found\n",fCtx);
#endif
	}

	fPilot->fLock.Unlock();
}

void VRemoteDebugPilot::VPilotNewContextMessage::DoExecute()
{
	XBOX::VError			err = VE_INVALID_PARAMETER;
	XBOX::VString			resp;
	
	fPilot->fLock.Lock();

	if (fPilot->fState >= STARTED_STATE)
	{
		if ( testAssert(fPilot->fContextArray.size() < K_NB_MAX_CTXS) )
		{

			VContextDescriptor*		newCtx = new VContextDescriptor(fContextRef);
			*(fOutCtx) = (OpaqueDebuggerContext)((sLONG8)sRemoteDebugPilotContextId);
			sRemoteDebugPilotContextId++;
			std::pair< OpaqueDebuggerContext, VContextDescriptor* >	l_pair( *fOutCtx, newCtx);
			std::pair< std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator, bool >	newElt =
					fPilot->fContextArray.insert( l_pair );

			if (testAssert(newElt.second))
			{
				if (fPilot->fState == CONNECTED_STATE)
				{
					resp = CVSTR("{\"method\":\"newContext\",\"contextId\":\"");
					resp.AppendLong8((sLONG8)(*fOutCtx));
					resp += CVSTR("\",\"id\":\"");
					resp += fPilot->fClientId;
					resp += CVSTR("\"}");

					err = fPilot->SendToBrowser( resp );
					if (err)
					{
						fPilot->fWS->Close();
						fPilot->fState = STARTED_STATE;
					}
				}
				else
				{
					err = VE_OK;
				}
				if (!err)
				{
					fStatus = VE_OK;
				}
			}
		}
	}
	fPilot->fLock.Unlock();
}

void VRemoteDebugPilot::VPilotUpdateContextMessage::DoExecute()
{
	XBOX::VError			err;
	XBOX::VString			resp;

	*fOutsem = NULL;
	fPilot->fLock.Lock();

	if (fIsNew)
	{
		fPilot->fDebuggingEventsTimeStamp++;
	}
	std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator	ctxIt = fPilot->fContextArray.find(fCtx);

	if ( ctxIt != fPilot->fContextArray.end())
	{
		/*if ((uLONG8)(*ctxIt).first == 11)   // allows to trace what happens for the 2ndly created user JScontext -- the first 9 are crareated by WAKsrv
		{
			int a=1;
		}*/
		(*ctxIt).second->fUpdated = true;
		(*ctxIt).second->fAborted = false;

		switch(fPilot->fState)
		{
		case STOPPED_STATE:
		case INIT_STATE:
			xbox_assert(false);
			break;

		case CONNECTING_STATE:
		case DISCONNECTING_STATE:
		case STARTED_STATE:
			(*ctxIt).second->fDebugInfos = fDebugInfos;
			xbox_assert( (*ctxIt).second->fSem == NULL );
			(*ctxIt).second->fSem = new XBOX::VSemaphore(0);
			(*ctxIt).second->fSem->Retain();
			*fOutsem = (*ctxIt).second->fSem;
			fStatus = VE_OK;
			break;

		case CONNECTED_STATE:
			if ( !(*ctxIt).second->fPage )
			{
				(*ctxIt).second->fDebugInfos = fDebugInfos;
				xbox_assert((*ctxIt).second->fSem == NULL);
				err = fPilot->SendContextToBrowser(ctxIt);

				if (err)
				{
					fPilot->fWS->Close();
					fPilot->fState = STARTED_STATE;
				}
				(*ctxIt).second->fSem = new XBOX::VSemaphore(0);
				(*ctxIt).second->fSem->Retain();
				*fOutsem = (*ctxIt).second->fSem;
				fStatus = VE_OK;
			}
			else
			{
				xbox_assert( (*ctxIt).second->fSem == NULL );
				(*ctxIt).second->fDebugInfos = fDebugInfos;
				err = fPilot->SendContextToBrowser(ctxIt);
				if (err)
				{
					fPilot->fWS->Close();
					fPilot->fState = STARTED_STATE;
				}
				fStatus = VE_OK;
			}
			break;
		}
	}
	fPilot->fLock.Unlock();
}

void VRemoteDebugPilot::VPilotRemoveContextMessage::DoExecute()
{
	fPilot->fLock.Lock();

	std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator ctxIt = fPilot->fContextArray.find(fCtx);
	fPilot->RemoveContext(ctxIt,false);

	fPilot->fLock.Unlock();
	fStatus = VE_OK;
}

void VRemoteDebugPilot::VPilotDeactivateContextMessage::DoExecute()
{
	fPilot->fLock.Lock();

	std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator ctxIt = fPilot->fContextArray.find(fCtx);
	XBOX::VError			err;
	XBOX::VString			resp;

	if (fPilot->fState >= STARTED_STATE)
	{
		if (ctxIt != fPilot->fContextArray.end())
		{
			if (!fHideOnly)
			{
				VChromeDbgHdlPage*	page = (*ctxIt).second->fPage;
				if (page)
				{
					page->Stop();
					ReleaseRefCountable(&(*ctxIt).second->fPage);
				}
			}
			fStatus = VE_OK;

			if ( (fPilot->fState == CONNECTED_STATE) && ((*ctxIt).second->fUpdated) )
			{
				resp = CVSTR("{\"method\":\"hideContext\",\"contextId\":\"");
				resp.AppendLong8((sLONG8)((*ctxIt).first));
				resp += CVSTR("\",\"id\":\"");
				resp += fPilot->fClientId;
				resp += CVSTR("\"}");
				err = fPilot->SendToBrowser( resp );
				if (err)
				{
					fPilot->fWS->Close();
					fPilot->fState = STARTED_STATE;
				}
			}
			(*ctxIt).second->fUpdated = false;
			(*ctxIt).second->fAborted = false;
		}
	}
	fPilot->fLock.Unlock();
}

#define C_DEFAULT_HYSTERESIS_DURATION	(1100)
VRemoteDebugPilot::VStatusHysteresis::VStatusHysteresis()
: fHysteresisDuration(sLONG8(C_DEFAULT_HYSTERESIS_DURATION))
{
	fLastConnectedTime.FromSystemTime();
	XBOX::VDuration		tenSecs(10000);
	fLastConnectedTime.Substract(tenSecs);
}

void VRemoteDebugPilot::VStatusHysteresis::Get(bool& ioConnected)
{
	XBOX::VTime			curtTime;

	curtTime.FromSystemTime();
	fLock.Lock();
	if (ioConnected)
	{
		fLastConnectedTime = curtTime;
	}
	else
	{
		XBOX::VDuration		disconnectedDuration;
		curtTime.Substract(fLastConnectedTime,disconnectedDuration);
		// if disconnection quite recent (default value < approx 1 s) consider it always connected
		if (disconnectedDuration.ConvertToMilliseconds() < fHysteresisDuration.ConvertToMilliseconds())
		{
			ioConnected = true;
		}
	}
	fLock.Unlock();
	
}
void VRemoteDebugPilot::VStatusHysteresis::SetDuration(sLONG8 inMs)
{
	fLock.Lock();
	fHysteresisDuration = VDuration(sLONG8(inMs));
	fLock.Unlock();
}

void VRemoteDebugPilot::VStatusHysteresis::ResetDuration()
{
	fLock.Lock();
	fHysteresisDuration = VDuration(sLONG8(C_DEFAULT_HYSTERESIS_DURATION));
	fLock.Unlock();
}

void VRemoteDebugPilot::GetStatus(bool&	outPendingContexts,bool& outConnected,sLONG& outDebuggingEventsTimeStamp)
{

	outConnected = (fState == CONNECTED_STATE);
	
	fStatusHysteresis.Get(outConnected);

	outPendingContexts = fPendingContextsNumberSem.TryToLock();
	outDebuggingEventsTimeStamp = fDebuggingEventsTimeStamp;
	if (outPendingContexts)
	{
		fPendingContextsNumberSem.Unlock();
	}
}


XBOX::VError VRemoteDebugPilot::SendToBrowser( const XBOX::VString& inResp )
{
	XBOX::VStringConvertBuffer		buffer(inResp,XBOX::VTC_UTF_8);
	XBOX::VSize						size = buffer.GetSize();
	//DebugMsg("SendToBrowser -> msg='%S'\n",&inResp);
	XBOX::VError					err = fWS->WriteMessage((void*)buffer.GetCPointer(),size,true);
	return err;
}

XBOX::VError VRemoteDebugPilot::SendContextToBrowser(
												std::map< OpaqueDebuggerContext,VContextDescriptor* >::iterator& inCtxIt)
{
	XBOX::VString				resp;

	resp = CVSTR("{\"method\":\"updateContext\",\"contextId\":\"");
	resp.AppendLong8((sLONG8)(*inCtxIt).first);
	resp += CVSTR("\",\"debugLineNb\":");
	resp.AppendLong((*inCtxIt).second->fDebugInfos.fLineNb);
	resp += CVSTR(",\"debugFileName\":\"");
	resp += (*inCtxIt).second->fDebugInfos.fFileName;
	resp += CVSTR("\",\"debugReason\":\"");
	resp += (*inCtxIt).second->fDebugInfos.fReason;
	resp += CVSTR("\",\"debugComment\":\"");
	resp += (*inCtxIt).second->fDebugInfos.fComment;
	resp += CVSTR("\",\"sourceLine\":\"");
	resp += (*inCtxIt).second->fDebugInfos.fSourceLine;
	resp += CVSTR("\",\"id\":\"");
	resp += fClientId;
	resp += CVSTR("\"}");
	return SendToBrowser( resp );

}


const VString K_PROTOCOL_STR("http://");



/* transforms the Solution hierarchy files in URLs
     -> when project is whithin the solution  :  ../projectName/Filename  becomes http://SolutionName/projectName/Filename
	 -> when project is 'outside' the solution:  ./projectName/Filename  becomes http://SolutionName/_/projectName/Filename
*/
void VRemoteDebugPilot::GetRelativeUrl( const VString& inUrl, VString& outRelativeUrl )
{
	outRelativeUrl = "";
	VIndex	idxStr = inUrl.Find(K_PROTOCOL_STR);
	if (idxStr > 0)
	{
		idxStr = inUrl.Find("/",idxStr+K_PROTOCOL_STR.GetLength());
	}
	if (testAssert(idxStr > 0))
	{
		VString	tmpStr;
		inUrl.GetSubString(idxStr+1,inUrl.GetLength()-idxStr,tmpStr);
		if (tmpStr.Find("_/") == 1)
		{
			tmpStr.Replace("./",1,2);
		}
		else
		{
			outRelativeUrl = "../";
		}
		outRelativeUrl += tmpStr;
	}
}
// see explanation above
void VRemoteDebugPilot::GetAbsoluteUrl( const XBOX::VString& inUrl, const XBOX::VString& inSolutionName, XBOX::VString& outAbsoluteUrl )
{
	outAbsoluteUrl = "";
	VString		substr;

	if (inUrl.Find("../") == 1)
	{
		inUrl.GetSubString(4,inUrl.GetLength()-3,substr);
	}
	else
	{
		if (inUrl.Find("./") == 1)
		{
			VString		tmpStr;
			inUrl.GetSubString(3,inUrl.GetLength()-2,tmpStr);
			substr = "_/";
			substr += tmpStr;
		}
		else
		{
			xbox_assert(false);
			substr = inUrl;
		}
	}
	outAbsoluteUrl = K_PROTOCOL_STR;
	outAbsoluteUrl += inSolutionName;
	outAbsoluteUrl += "/";
	outAbsoluteUrl += substr;
}

void VRemoteDebugPilot::DisplayContexts()
{
	XBOX::VString	K_EMPTY("empty");
	DEBUG_MSG("\ndisplay ctxs\n");
	std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator	ctxIt = fContextArray.begin();
	while( ctxIt != fContextArray.end() )
	{
		sLONG8 id = (sLONG8)((*ctxIt).first);
		int updated = 0;
		if ((*ctxIt).second->fUpdated)
		{
			updated = 1;
		}
		XBOX::VString tmp = (*ctxIt).second->fDebugInfos.fFileName;
		if (tmp.GetLength() <= 0)
		{
#if GUY_DUMP_DEBUG_MSG
			XBOX::DebugMsg("  id->%lld , name=empty, updated=%d\n",
					id,
					updated );
#endif
		}
		else
		{
#if GUY_DUMP_DEBUG_MSG
			XBOX::DebugMsg("  id->%lld , name=%S, updated=%d\n",
					id,
					&tmp,
					updated );
#endif
		}
		ctxIt++;
	}
	XBOX::DebugMsg("\n\n");
}

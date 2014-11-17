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

#include "VChromeDebugHandler.h"
#include "VRemoteDebugPilot.h"


USING_TOOLBOX_NAMESPACE


const XBOX::VString			K_REMOTE_DEBUGGER_WS("/wka_dbg/devtools/remote_debugger_ws");
const XBOX::VString			K_REMOTE_TRACES_WS("/devtools/remote_traces_ws");
const XBOX::VString			K_CHR_DBG_ROOT("/devtools");
const XBOX::VString			K_CHR_DBG_FILTER("/devtools/page/");
const XBOX::VString			K_CHR_DBG_REMTRA_FILTER("/devtools/remote_traces.html");
const XBOX::VString			K_EMPTY_PAGE_URL("EMPTY");

VChromeDebugHandler*				VChromeDebugHandler::sDebugger=NULL;
XBOX::VCriticalSection				VChromeDebugHandler::sDbgLock;
IWAKDebuggerSettings*				VChromeDebugHandler::sStaticSettings = NULL;


VLogHandler*			sPrivateLogHandler = NULL;



VChromeDebugHandler::VChromeDebugHandler(
	const VString&	inSolutionName,
	const VString&	inTracesHTML,
	bool			inV8Mode )
{
	if (!sDebugger)
	{
		fStarted = false;
		fTracesHTML = inTracesHTML;

		CHTTPServer*	l_http_server = VComponentManager::RetainComponent< CHTTPServer >();
		fPilot = new VRemoteDebugPilot(l_http_server, inV8Mode);
		if (testAssert(fPilot != NULL))
		{
			fPilot->SetWS(l_http_server->NewHTTPWebsocketServerHandler());
			fPilot->SetSolutionName(inSolutionName);
			sPrivateLogHandler->SetWS(l_http_server->NewHTTPWebsocketServerHandler());
		}

		l_http_server->Release();

	}
	else
	{
		//fWS = NULL;
		//DebugMsg("VChrmDebugHandler should be a singleton\n");
		xbox_assert(false);
	}
}



VChromeDebugHandler::~VChromeDebugHandler()
{
	xbox_assert(false);
	/*if (fWS)
	{
		if (fWS->Close())
		{
			DebugMsg("~VChrmDebugHandler error closing websocket\n");
		}
		VTask::Sleep(1500);
		s_vchrm_debug_handler_init = false;
		fWS = NULL;
	}*/
}





XBOX::VError VChromeDebugHandler::TreatTracesHTML(IHTTPResponse* ioResponse)
{
	XBOX::VError		err;
	VSize				size;
	char*				buff;

	err = VE_OK;
	if (ioResponse->GetRequestMethod() != HTTP_GET)
	{
		err = VE_INVALID_PARAMETER;
	}
	if (!err)
	{
		{
			VStringConvertBuffer	buffer(fTracesHTML,VTC_UTF_8);
			size = buffer.GetSize();

			err = ioResponse->SetResponseBody(buffer.GetCPointer(),size);
			if (!err)
			{
				if (!ioResponse->SetContentTypeHeader(CVSTR("text/html"),XBOX::VTC_UTF_8))
				{
					err = VE_INVALID_PARAMETER;
				}
			}
		}
	}
	return err;
}

XBOX::VError VChromeDebugHandler::TreatRemoteTraces(IHTTPResponse* ioResponse)
{
	XBOX::VError		err;

	err = VE_OK;
	sPrivateLogHandler->TreatClient(ioResponse);
	return err;
}




bool VChromeDebugHandler::IsAuthorized( IHTTPResponse* ioResponse )
{
	const VString peerAddr = ServerNetTools::GetPeerIP(ioResponse->GetEndPoint());
	bool isAuthorized( ServerNetTools::IsLoopBack( peerAddr ) );

	if (!isAuthorized)
	{
		isAuthorized = ioResponse->GetEndPoint()->IsSSL();
	}
	return isAuthorized;
}

XBOX::VError VChromeDebugHandler::HandleRequest( IHTTPResponse* ioResponse )
{
	XBOX::VError	err;
	sLONG			pageNb;

#if GUY_DUMP_DEBUG_MSG
	DebugMsg("VChromeDebugHandler::HandleRequest called ioResponse=%x\n",ioResponse);
#endif

	if (!fStarted)
	{
		sPrivateLogHandler->Put(WAKDBG_ERROR_LEVEL,VString("VChromeDebugHandler::HandleRequest SERVER NOT STARTED!!!"));
		return VE_INVALID_PARAMETER;
	}
	if (ioResponse == NULL)
	{
		sPrivateLogHandler->Put(WAKDBG_ERROR_LEVEL,VString("VChromeDebugHandler::HandleRequest called NULL ptr!!!"));
		return VE_INVALID_PARAMETER;
	}

	VString			url = ioResponse->GetRequest().GetURL();

	VString trace("VChromeDebugHandler::HandleRequest called for url=");
	trace += url;
	sPrivateLogHandler->Put(WAKDBG_INFO_LEVEL,trace);

	VString			tmpStr;
	VIndex			idx;
	
	err = VE_OK;

	{
		idx = url.Find(K_CHR_DBG_FILTER);
		if (idx > 0)
		{
#if 1 // GH debug WS
			if (!IsAuthorized(ioResponse))
			{
				err = XBOX::VE_INVALID_PARAMETER;
			}
			else
#endif
			{
				url.GetSubString(
					idx+K_CHR_DBG_FILTER.GetLength(),
					url.GetLength() - K_CHR_DBG_FILTER.GetLength(),
					tmpStr);

				if (!tmpStr.EqualToString(K_EMPTY_PAGE_URL))
				{
					pageNb = tmpStr.GetLong();

					if (!sStaticSettings->UserCanDebug(ioResponse->GetRequest().GetAuthenticationInfos()))
					{
						err = VE_HTTP_PROTOCOL_UNAUTHORIZED;
					}
					if (!err)
					{
						err = fPilot->TreatWS(ioResponse,pageNb);
					}
				}
				else
				{
					err = ioResponse->SetResponseBody(NULL,0);
					if (!err)
					{
						ioResponse->SetContentTypeHeader(CVSTR("text/html"),XBOX::VTC_UTF_8);
					}
				}
			}
		}
		else
		{
			idx = url.Find(K_REMOTE_DEBUGGER_WS);
			if (idx > 0)
			{
#if 1 // GH debug WS
				if (!IsAuthorized(ioResponse))
				{
					err = XBOX::VE_INVALID_PARAMETER;
				}
				else
#endif
				{
					bool needsAuthentication =
						!sStaticSettings->UserCanDebug(ioResponse->GetRequest().GetAuthenticationInfos());

					err = fPilot->TreatRemoteDbg(ioResponse,needsAuthentication);
					err = XBOX::VE_OK;// set err to OK so that HTTP server considers HTTP request successfully ended
				}
			}
			else
			{
				{
					idx = url.Find(K_CHR_DBG_REMTRA_FILTER);
					if (idx > 0)
					{
						err = TreatTracesHTML(ioResponse);
					}
					else
					{
						idx = url.Find(K_REMOTE_TRACES_WS);
						if (idx > 0)
						{
							err = TreatRemoteTraces(ioResponse);
							err = XBOX::VE_OK;// set err to OK so that HTTP server considers HTTP request successfully ended
						}
						else
						{
							err = XBOX::VE_INVALID_PARAMETER;
						}
					}
				}
			}
		}
	}
	return err;

}


XBOX::VError VChromeDebugHandler::GetPatterns(XBOX::VectorOfVString *outPatterns) const
{
	if (NULL == outPatterns)
		return VE_HTTP_INVALID_ARGUMENT;
	outPatterns->clear();
	outPatterns->push_back(K_REMOTE_DEBUGGER_WS);
	outPatterns->push_back(K_CHR_DBG_ROOT);
	return XBOX::VE_OK;
}


bool VChromeDebugHandler::HasClients()
{
	return fPilot->IsActive();

}


void VChromeDebugHandler::DisposeMessage(WAKDebuggerServerMessage* inMessage)
{
	if (inMessage)
	{
		delete inMessage;
	}
}

WAKDebuggerServerMessage* VChromeDebugHandler::WaitFrom(OpaqueDebuggerContext inContext)
{
	WAKDebuggerServerMessage*		l_res;
	l_res = fPilot->WaitFrom(inContext);

	return l_res;
}

void VChromeDebugHandler::AbortAll()
{
	fPilot->AbortAll();
}

const VString	K_SCRIPT_ID_STR("\"location\":{\"scriptId\":\"");

#if USE_V8_ENGINE

bool VChromeDebugHandler::SendCallStack(
	OpaqueDebuggerContext			inContext,
	const XBOX::VString&			inCallstackStr,
	const XBOX::VString&			inExceptionInfos,
	CallstackDescriptionMap&		inCSDesc)
{
	VIndex					idx = 1;
	CallstackDescriptionMap	callstackDesc;
	bool					result = true;

	while (result && ((idx = inCallstackStr.Find(K_SCRIPT_ID_STR, idx)) > 0) && (idx <= inCallstackStr.GetLength()))
	{
		VString				fileName;
		VectorOfVString		tmpData;
		VIndex				endIndex;
		VString				scriptIDStr;
		sLONG8				sourceId;
		VCallstackDescription		callstackElement;

		idx += K_SCRIPT_ID_STR.GetLength();
		endIndex = inCallstackStr.Find(CVSTR("\""), idx);
		if (endIndex)
		{
			inCallstackStr.GetSubString(idx, endIndex - idx, scriptIDStr);
			sourceId = scriptIDStr.GetLong8();
			if (testAssert(sStaticSettings->GetData(
				inContext,
				(intptr_t)sourceId,
				callstackElement.fFileName,
				callstackElement.fSourceCode)))
			{
				VIndex	lastSlashIndex;
				/*lastSlashIndex = callstackElement.fFileName.FindUniChar( L'/', callstackElement.fFileName.GetLength(), true );
				if ( (lastSlashIndex  > 1) && (lastSlashIndex < callstackElement.fFileName.GetLength()) )
				{
				VString	tmpStr;
				callstackElement.fFileName.GetSubString(lastSlashIndex+1,callstackElement.fFileName.GetLength()-lastSlashIndex,tmpStr);
				callstackElement.fFileName = tmpStr;
				}*/
				if (callstackDesc.find(scriptIDStr) == callstackDesc.end())
				{
					std::pair< std::map< XBOX::VString, VCallstackDescription >::iterator, bool >	newElement =
						callstackDesc.insert(std::pair< XBOX::VString, VCallstackDescription >(scriptIDStr, callstackElement));
					xbox_assert(newElement.second);
				}
			}
		}
	}
	/*CallstackDescriptionMap::iterator	itDesc = inCSDesc.begin();
	while (itDesc != inCSDesc.end())
	{
		XBOX::VectorOfVString	scVect;
		VString		fileName = (*itDesc).second.fFileName;
		if (!testAssert(sStaticSettings->GetData(
			inContext,
			(intptr_t)((*itDesc).first.GetLong()),
			(*itDesc).second.fFileName,
			scVect)))
		{
		}
		++itDesc;
	}*/
	result = (fPilot->SendCallStack(inContext, inCallstackStr, inExceptionInfos, callstackDesc) == VE_OK);
	return result;
}

bool VChromeDebugHandler::BreakpointReached(
	OpaqueDebuggerContext		inContext,
	XBOX::VString&				inSourceURL,
	int							inLineNumber,
	XBOX::VString&				inSourceLineText,
	RemoteDebuggerPauseReason	inPauseReason,
	XBOX::VString&				inExceptionName,
	bool&						outAbort,
	bool						inIsNew)
{
	VError				err;
	VString				relativePosixPath;
	sStaticSettings->GetRelativePosixPath(inSourceURL, relativePosixPath);
	VString	sourceLineText;

	inSourceLineText.GetJSONString(sourceLineText);
	err = fPilot->BreakpointReached(
		inContext,
		inLineNumber,
		sourceLineText,
		inPauseReason,//( inExceptionHandle < 0 ? EXCEPTION_RSN : ( inExceptionHandle == 0 ? BREAKPOINT_RSN : DEBUGGER_RSN ) ),
		inExceptionName,
		relativePosixPath,
		outAbort,
		inIsNew);

	//return testAssert(l_err == VE_OK);
	return (err == VE_OK);
}


bool VChromeDebugHandler::SendSource(
	OpaqueDebuggerContext				inContext,
	intptr_t							inSourceId,
	const VString&						inFileName,
	const VString&						inData,
	std::set< unsigned >&				outBreakpointsLineNumbers)
{
	outBreakpointsLineNumbers.clear();
	if (inFileName.Find("test1.js"))
	{
		//outBreakpointsLineNumbers.insert(12);
		//outBreakpointsLineNumbers.insert(6);
		sSrcId_GH_TEST = inSourceId;
#if GUY_DUMP_DEBUG_MSG
		DebugMsg("VChromeDebugHandler::SendSource fileName='%S' srcId=%lld\n", &fileName, sSrcId_GH_TEST);
#endif
	}
	else
	{
		if (inSourceId == sSrcId_GH_TEST)
		{
#if GUY_DUMP_DEBUG_MSG
			DebugMsg("VChromeDebugHandler::SendSource srcId=%lld\n", sSrcId_GH_TEST);
#endif
		}
	}
	sStaticSettings->Set(inContext, inFileName, inSourceId, inData);
	return true;
}


bool VChromeDebugHandler::SendLookup(OpaqueDebuggerContext inContext, const VString& inLookupResult)
{
	bool	res = (fPilot->SendLookup(inContext, inLookupResult) == VE_OK);
	return res;
}


OpaqueDebuggerContext VChromeDebugHandler::AddContext(void* inContextRef)
{
	OpaqueDebuggerContext	l_res = (OpaqueDebuggerContext)-1;

	if (fPilot->NewContext(inContextRef,&l_res) != VE_OK)
	{
		return (OpaqueDebuggerContext)-1;
	}
	sStaticSettings->Add(l_res);
	return l_res;
}

bool VChromeDebugHandler::RemoveContext( OpaqueDebuggerContext inContext )
{
	bool	l_res;
	l_res = (fPilot->ContextRemoved(inContext) == VE_OK);
	if (l_res)
	{
		//sDbgLock.Lock();
		//if (sStaticSettings)
		{
			sStaticSettings->Remove(inContext);
		}
		//sDbgLock.Unlock();
	}
	return l_res;
}

bool VChromeDebugHandler::DeactivateContext( OpaqueDebuggerContext inContext, bool inHideOnly )
{
	bool	l_res;
	l_res = (fPilot->DeactivateContext(inContext,inHideOnly) == VE_OK);
	return l_res;
}
#else


bool VChromeDebugHandler::BreakpointReached(OpaqueDebuggerContext		inContext,
	intptr_t					inSourceId,
	int							inLineNumber,
	RemoteDebuggerPauseReason	inPauseReason,
	char* 						inExceptionName,
	int 						inExceptionNameLength,
	bool&						outAbort,
	bool						inIsNew)
{
	VError				err;
	VString				sourceUrl;
	VectorOfVString		sourceData;
	VString				jsExceptionStr;
	if (inExceptionNameLength > 0)
	{
		jsExceptionStr = VString(inExceptionName,(VSize)inExceptionNameLength,VTC_UTF_16);
		VString				vstrOutput;
		jsExceptionStr.GetJSONString( vstrOutput );
		jsExceptionStr = vstrOutput;
	}

	sStaticSettings->GetData(inContext,inSourceId,sourceUrl,sourceData);
	VString	sourceLineText = (inLineNumber < sourceData.size() ? sourceData[inLineNumber] : " NO SOURCE AVAILABLE  ");

	err = fPilot->BreakpointReached(
		inContext,
		inLineNumber,
		sourceLineText,
		inPauseReason,
		jsExceptionStr,
		sourceUrl,
		outAbort,
		inIsNew);

	//return testAssert(l_err == VE_OK);
	return (err == VE_OK);

}
bool VChromeDebugHandler::SendLookup(OpaqueDebuggerContext inContext, void* inLookupResultVStringPtr)
{
	bool	res = (fPilot->SendLookup(inContext, *(VString*)inLookupResultVStringPtr) == VE_OK);

	delete (VString*)(inLookupResultVStringPtr);
	return res;
}


bool VChromeDebugHandler::SendCallStack(OpaqueDebuggerContext	inContext,
	const char*				inCallstackStr,
	int						inCallstackStrLength,
	const char*				inExceptionInfos,
	int						inExceptionInfosLength)
{
	VString					callstackData(inCallstackStr, inCallstackStrLength * 2, VTC_UTF_16);
	VString					exceptionInfos(inExceptionInfos, inExceptionInfosLength * 2, VTC_UTF_16);
	VIndex					idx = 1;
	CallstackDescriptionMap	callstackDesc;
	bool					result = true;

	while (result && ((idx = callstackData.Find(K_SCRIPT_ID_STR, idx)) > 0) && (idx <= callstackData.GetLength()))
	{
		VString				fileName;
		VectorOfVString		tmpData;
		VIndex				endIndex;
		VString				scriptIDStr;
		sLONG8				sourceId;
		VCallstackDescription		callstackElement;

		idx += K_SCRIPT_ID_STR.GetLength();
		endIndex = callstackData.Find(CVSTR("\""), idx);
		if (endIndex)
		{
			callstackData.GetSubString(idx, endIndex - idx, scriptIDStr);
			sourceId = scriptIDStr.GetLong8();
			if (testAssert(sStaticSettings->GetData(
				inContext,
				(intptr_t)sourceId,
				callstackElement.fFileName,
				callstackElement.fSourceCode)))
			{
				VIndex	lastSlashIndex;
				/*lastSlashIndex = callstackElement.fFileName.FindUniChar( L'/', callstackElement.fFileName.GetLength(), true );
				if ( (lastSlashIndex  > 1) && (lastSlashIndex < callstackElement.fFileName.GetLength()) )
				{
				VString	tmpStr;
				callstackElement.fFileName.GetSubString(lastSlashIndex+1,callstackElement.fFileName.GetLength()-lastSlashIndex,tmpStr);
				callstackElement.fFileName = tmpStr;
				}*/
				if (callstackDesc.find(scriptIDStr) == callstackDesc.end())
				{
					std::pair< std::map< XBOX::VString, VCallstackDescription >::iterator, bool >	newElement =
						callstackDesc.insert(std::pair< XBOX::VString, VCallstackDescription >(scriptIDStr, callstackElement));
					xbox_assert(newElement.second);
				}
			}
		}
	}

	if (result)
	{
		result = (fPilot->SendCallStack(inContext, callstackData, exceptionInfos, callstackDesc) == VE_OK);
	}

	return result;
}

bool VChromeDebugHandler::SendSource(
	OpaqueDebuggerContext				inContext,
	intptr_t							inSourceId,
	char*								inFileName,
	int									inFileNameLength,
	const char*							inData,
	int									inDataLength)
{
	VString		fileName;
	VString		data;

	if (inFileName)
	{
		fileName = VString(inFileName, (VSize)(inFileNameLength), VTC_UTF_16);
	}
	if (inData)
	{
		data = VString(inData, (VSize)(inDataLength), VTC_UTF_16);
	}
	if (fileName.Find("test.js"))
	{
		sSrcId_GH_TEST = inSourceId;
#if GUY_DUMP_DEBUG_MSG
		DebugMsg("VChromeDebugHandler::SendSource fileName='%S' srcId=%lld\n", &fileName, sSrcId_GH_TEST);
#endif
	}
	else
	{
		if (inSourceId == sSrcId_GH_TEST)
		{
#if GUY_DUMP_DEBUG_MSG
			DebugMsg("VChromeDebugHandler::SendSource srcId=%lld\n", sSrcId_GH_TEST);
#endif
		}
	}
	sStaticSettings->Set(inContext, fileName, inSourceId, data);
	return true;
}

OpaqueDebuggerContext VChromeDebugHandler::AddContext(void* inContextRef)
{
	OpaqueDebuggerContext	l_res = (OpaqueDebuggerContext)-1;

	if (fPilot->NewContext(inContextRef,&l_res) != VE_OK)
	{
		return (OpaqueDebuggerContext)-1;
	}
	sStaticSettings->Add(l_res);
	return l_res;
}

bool VChromeDebugHandler::RemoveContext( OpaqueDebuggerContext inContext )
{
	bool	l_res;
	l_res = (fPilot->ContextRemoved(inContext) == VE_OK);
	if (l_res)
	{
		//sDbgLock.Lock();
		//if (sStaticSettings)
		{
			sStaticSettings->Remove(inContext);
		}
		//sDbgLock.Unlock();
	}
	return l_res;
}

bool VChromeDebugHandler::DeactivateContext( OpaqueDebuggerContext inContext, bool inHideOnly )
{
	bool	l_res;
	l_res = (fPilot->DeactivateContext(inContext,inHideOnly) == VE_OK);
	return l_res;
}
#endif


bool VChromeDebugHandler::SendEval( OpaqueDebuggerContext	inContext,
									void*					inEvaluationResultVStringPtr,
									intptr_t				inRequestId )
{
	bool		result;
	VString		evaluationResult = *(VString *)inEvaluationResultVStringPtr;

	delete (VString*)(inEvaluationResultVStringPtr);
	VString		requestId;
	requestId.AppendLong8((sLONG8)inRequestId);
	result = (fPilot->SendEval(inContext,evaluationResult,requestId) == VE_OK);


	return result;
}


WAKDebuggerUCharPtr_t VChromeDebugHandler::EscapeForJSON( const unsigned char* inString, int inSize, int & outSize )
{
	VString				vstrInput;
	vstrInput.AppendBlock( inString, 2*inSize, VTC_UTF_16 );

	VString				vstrOutput;
	vstrInput.GetJSONString( vstrOutput );

	outSize = vstrOutput.GetLength();
	WAKDebuggerUCharPtr_t	l_res = new unsigned char[2*(outSize + 1)];
	vstrOutput.ToBlock( l_res, 2*(outSize + 1), VTC_UTF_16, true, false );

	return l_res;
}

void VChromeDebugHandler::DisposeUCharPtr( WAKDebuggerUCharPtr_t inUCharPtr )
{
	if (inUCharPtr)
	{
		delete [] inUCharPtr;
	}
}

void* VChromeDebugHandler::UStringToVStringPtr( const void* inUString, int inSize )
{
	return (void*) new VString(inUString,(VSize)inSize,VTC_UTF_16);
}


WAKDebuggerType_t VChromeDebugHandler::GetType()
{
	return WEB_INSPECTOR_TYPE;
}

int	VChromeDebugHandler::StartServer()
{
	fStarted = true;
	fPilot->Start();
	return 0;
}
int	VChromeDebugHandler::StopServer()
{
	fStarted = false;
	fPilot->Stop();
	return 0;
}
short VChromeDebugHandler::GetServerPort()
{
	xbox_assert(false);
	return -1;
}


bool VChromeDebugHandler::IsSecuredConnection()
{
	xbox_assert(false);
	return false;
}


void VChromeDebugHandler::StaticSetSettings( IWAKDebuggerSettings* inSettings )
{
	sDbgLock.Lock();

	sStaticSettings = inSettings;
	
	sDbgLock.Unlock();
}

bool VChromeDebugHandler::HasBreakpoint(	OpaqueDebuggerContext				inContext,
											intptr_t							inSourceId,
											int									inLineNumber)
{
	bool	result;
	result = sStaticSettings->HasBreakpoint(inContext,inSourceId,inLineNumber);
	if ((inSourceId == sSrcId_GH_TEST) && result)
	{
#if GUY_DUMP_DEBUG_MSG
		DebugMsg("VChromeDebugHandler::HasBreakpoint srcId=%lld  BKPT.line=%d\n",sSrcId_GH_TEST,inLineNumber);
#endif
	}
	return result;
}

intptr_t		VChromeDebugHandler::sSrcId_GH_TEST = 0;

bool VChromeDebugHandler::GetFilenameFromId(
											OpaqueDebuggerContext				inContext,
											intptr_t							inSourceId,
											char*								ioFileName,
											int&								ioFileNameLength)
{
	bool				result = false;
	VString				fileName;
	VectorOfVString		tmpData;

	if (sStaticSettings->GetData(inContext,inSourceId,fileName,tmpData))
	{
		VIndex				idx;
		idx = fileName.FindUniChar( L'/', fileName.GetLength(), true );
		if ( (idx  > 1) && (idx < fileName.GetLength()) )
		{
			XBOX::VString	absoluteUrl;
			if (fileName.GetLength() > 3)
			{
				VString solutionName;
				fPilot->GetSolutionName(solutionName);
				VRemoteDebugPilot::GetAbsoluteUrl(fileName,solutionName,absoluteUrl);
			}

			absoluteUrl.ToBlock(ioFileName,ioFileNameLength,VTC_UTF_16,true,false);
			ioFileNameLength = absoluteUrl.GetLength();
			result = true;
		}
	}
	if (!result)
	{
		const VString	emptyFileName("UnknowFileId");
		emptyFileName.ToBlock(ioFileName,ioFileNameLength,VTC_UTF_16,true,false);
		ioFileNameLength = emptyFileName.GetLength();
	}

	return result;
}

void VChromeDebugHandler::Lock()
{
	sDbgLock.Lock();
}

void VChromeDebugHandler::Unlock()
{
	sDbgLock.Unlock();
}


void VChromeDebugHandler::SetSettings( IWAKDebuggerSettings* inSettings )
{
	//fDebuggerSettings = inSettings;
	xbox_assert(false);
}
void VChromeDebugHandler::SetInfo( IWAKDebuggerInfo* inInfo )
{
	xbox_assert(false);
}


void VChromeDebugHandler::GetStatus(
			bool&			outStarted,
			bool&			outConnected,
			long long&		outDebuggingEventsTimeStamp,
			bool&			outPendingContexts)
{
	outStarted = fStarted;
	sLONG	evtsTS;
	fPilot->GetStatus(outPendingContexts,outConnected,evtsTS);
	outDebuggingEventsTimeStamp = evtsTS;
}

void VChromeDebugHandler::GetAllJSFileNames(XBOX::VectorOfVString& outFileNameVector)
{
	sDbgLock.Lock();
	
	if (sStaticSettings)
	{
		sStaticSettings->GetAllJSFileNames(outFileNameVector);
	}
	else
	{
		outFileNameVector.clear();
	}

	sDbgLock.Unlock();
}

void VChromeDebugHandler::GetJSONBreakpoints(XBOX::VString& outJSONBreakPoints)
{
	sDbgLock.Lock();
	
	if (sStaticSettings)
	{
		sStaticSettings->GetJSONBreakpoints(outJSONBreakPoints);
	}
	else
	{
		outJSONBreakPoints = "[]";
	}

	sDbgLock.Unlock();
}

void VChromeDebugHandler::GetSourceFromUrl(	OpaqueDebuggerContext		inContext,
											const XBOX::VString&		inSourceUrl,
											XBOX::VectorOfVString&		outSourceData )
{
	sDbgLock.Lock();
	if (sStaticSettings)
	{
		sStaticSettings->GetSourceFromUrl(inContext,inSourceUrl,outSourceData);
	}
	sDbgLock.Unlock();
}


void VChromeDebugHandler::StaticSendBreakpointsUpdated()
{
	sDbgLock.Lock();
	if (sDebugger)
	{
		sDebugger->fPilot->SendBreakpointsUpdated();
	}
	sDbgLock.Unlock();
}
void VChromeDebugHandler::AddBreakPoint(OpaqueDebuggerContext inContext, intptr_t inSourceId,int inLineNumber)
{
	sDbgLock.Lock();
	if (sStaticSettings)
	{
		sStaticSettings->AddBreakPoint(inContext,"",inSourceId,inLineNumber);
	}
	sDbgLock.Unlock();
	sDebugger->fPilot->SendBreakpointsUpdated();
}
void VChromeDebugHandler::AddBreakPointByUrl(const VString& inUrl,int inLineNumber)
{

	sDbgLock.Lock();
	if (sStaticSettings)
	{
		sStaticSettings->AddBreakPoint(inUrl,inLineNumber);
	}
	sDbgLock.Unlock();
	sDebugger->fPilot->SendBreakpointsUpdated();
}
void VChromeDebugHandler::RemoveBreakPointByUrl(const VString& inUrl,int inLineNumber)
{

	sDbgLock.Lock();
	if (sStaticSettings)
	{
		sStaticSettings->RemoveBreakPoint(inUrl,inLineNumber);
	}
	sDbgLock.Unlock();
	sDebugger->fPilot->SendBreakpointsUpdated();
}

void VChromeDebugHandler::RemoveBreakPoint(OpaqueDebuggerContext inContext,intptr_t inSourceId,int inLineNumber)
{
	sDbgLock.Lock();
	if (sStaticSettings)
	{
		sStaticSettings->RemoveBreakPoint(inContext,"",inSourceId,inLineNumber);
	}
	sDbgLock.Unlock();
	sDebugger->fPilot->SendBreakpointsUpdated();
}

void VChromeDebugHandler::Trace(OpaqueDebuggerContext	inContext,
								const void*				inString,
								int						inSize,
								WAKDebuggerTraceLevel_t inTraceLevel )
{
	bool			l_res = true;//(fPilot->Check(inContext) == VE_OK);

#if 1//defined(UNIFIED_DEBUGGER_NET_DEBUG)

	//ChrmDbgHdlPage	*l_page = l_ctx->GetPage();

	VString		l_trace(inString,(VSize)inSize,VTC_UTF_16);
	VString l_tmp = CVSTR("WEBINSP id=");
	if (l_res)
	{
		l_tmp.AppendLong8((sLONG8)inContext);//l_ctx->GetId());
	}
	else
	{
		l_tmp += "BAD_CTX ";
	}
	l_tmp += CVSTR(">>>>' ");
	l_tmp += l_trace;
	l_tmp += CVSTR("'\n");
	sPrivateLogHandler->Put(inTraceLevel,l_tmp);

#if GUY_DUMP_DEBUG_MSG
	DebugMsg(l_tmp);
#endif

#endif

}

VChromeDebugHandler* VChromeDebugHandler::Get(const XBOX::VString& inSolutionName)
{
	if ( sDebugger == 0 )
	{
		//xbox_assert(false);
	}
	else
	{
		if (sDebugger->fPilot)
		{
			sDebugger->fPilot->SetSolutionName(inSolutionName);
		}
	}
	return sDebugger;
}


VChromeDebugHandler* VChromeDebugHandler::Get(
						const XBOX::VString&	inSolutionName,
						const XBOX::VString&	inTracesHTMLSkeleton,
						bool					inV8Mode)
{
	if ( sDebugger == 0 )
	{

		sPrivateLogHandler = new VLogHandler();
		sDebugger = new VChromeDebugHandler(inSolutionName,inTracesHTMLSkeleton,inV8Mode);
		if (sDebugger)
		{
			XBOX::RetainRefCountable(sDebugger);
		}

	}
	return sDebugger;
}



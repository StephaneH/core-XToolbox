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
// JSDebugger.cpp : Defines the exported functions for the DLL application.
//

#include "Kernel/VKernel.h"
#include "ServerNet/VServerNet.h"

#include "CJSWDebuggerFactory.h"
#include "JSWDebugger.h"
#include "JSWServer.h"

#define WKA_USE_UNIFIED_DBG

VJSWDebugger*							VJSWDebugger::sDebugger = 0;


//XBOX::VCriticalSection					VJSWDebugger::sDbgLock;
#if defined(UNIFIED_DEBUGGER_NET_DEBUG)
XBOX::XTCPSock*							VJSWDebugger::sSck=NULL;
#endif


VJSWDebuggerCommand::VJSWDebuggerCommand( IWAKDebuggerCommand::WAKDebuggerServerMsgType_t inCommand, const VString & inID, const VString & inContextID, const VString & inParameters )
{
	fCommand = inCommand;

	fID. FromString ( inID );
	if ( fID. GetLength ( ) == 0 )
		fID. AppendCString ( "NOID" );

	fContextID. FromString ( inContextID );
	if ( fContextID. GetLength ( ) == 0 )
		fContextID. AppendCString ( "UNKNOWN" );

	fParameters. FromString ( inParameters );
}

const char* VJSWDebuggerCommand::GetParameters ( ) const
{
	VIndex		nBufferLength = fParameters. GetLength ( ) + 1;
	char*		szchResult = new char [ nBufferLength ];
	fParameters. ToCString ( szchResult, nBufferLength );

	return szchResult;
}

const char* VJSWDebuggerCommand::GetID ( ) const
{
	VIndex		nBufferLength = fID. GetLength ( ) + 1;
	char*		szchResult = new char [ nBufferLength ];
	fID. ToCString ( szchResult, nBufferLength );

	return szchResult;
}

void VJSWDebuggerCommand::Dispose ( )
{
	delete this;
}


bool VJSWDebuggerCommand::HasSameContextID ( uintptr_t inContextID ) const
{
	if ( fContextID. EqualToString ( CVSTR ( "UNDEFINED" ) ) )
		return true;

	VString			vstrTestID;
	vstrTestID. FromLong8 ( inContextID );

	return fContextID. EqualToString ( vstrTestID );
}


void VJSWDebugger::SetSettings( IWAKDebuggerSettings* inSettings )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
	{
		fSettings = inSettings;

		return;
	}

	if ( inSettings == 0 )
		fSettings = inSettings;

	cHandler-> SetSettings ( inSettings );
	cHandler-> Release ( );
}


void VJSWDebugger::SetInfo( IWAKDebuggerInfo* inInfo )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return;

	cHandler-> SetInfo ( inInfo );
	cHandler-> Release ( );
}

VJSWDebugger* VJSWDebugger::Get ( )
{
	if ( sDebugger == 0 )
	{
		sDebugger = new VJSWDebugger ( );
#if defined(UNIFIED_DEBUGGER_NET_DEBUG)
		if (!sSck)
		{
			sSck = XTCPSock::NewClientConnectedSock(K_NETWORK_DEBUG_HOST,K_NETWORK_DEBUG_BASE,100);
		}
#endif
	}
	return sDebugger;
}

int VJSWDebugger::StopServer ( )
{
	if ( (fServer == 0) || (fTCPListener == NULL) )
	{
		xbox_assert(false);
		return 0;
	}

	fServer->Stop(); // sc 12/02/2014 WAK0086544
	fTCPListener->WaitForDeath( 3000);
	ReleaseRefCountable( &fTCPListener);
	fSecuredConnection = false;

	return 0;
}

void VJSWDebugger::GetStatus(
			bool&			outStarted,
			bool&			outConnected,
			long long&		outDebuggingEventsTimeStamp,
			bool&			outPendingContexts)
{
	outStarted = true;
	outConnected = HasClients();
	outDebuggingEventsTimeStamp = 1;
	outPendingContexts = false;
}


void VJSWDebugger::SetCertificatesFolder( const void* inFolderPath)
{
	VString strPath;
	if (inFolderPath != NULL)
		strPath.FromUniCString( (UniChar*) inFolderPath);

	VFolder *folder = NULL;
	VFilePath path( strPath);
	if (path.IsValid() && path.IsFolder())
		folder = new VFolder( path);

	CopyRefCountable( &fCertificatesFolder, folder);
}


int VJSWDebugger::StartServer ( )
{
	/* TODO: Need multi-threading protection */

	if ( (fServer != 0) && fServer->IsRunning() )
		return 0;

	if (!testAssert(fTCPListener == NULL))
		return 0;

	VError									vError = VE_OK;

	fSecuredConnection = false;

	if (fServer == NULL)
		fServer = new VServer ( );

	VWorkerPool*							vwPool = new VWorkerPool ( 2, 5, 60, 2, 3 );

	PortNumber								nPort = 1919;
	VJSWConnectionHandlerFactory*			jswchFactory = new VJSWConnectionHandlerFactory ( );

	VString									vstrIPAll = VNetAddress::GetAnyIP ( );
	jswchFactory-> SetIP ( vstrIPAll );

	VFilePath vfPathCertificate, vfPathKey;

	if (fCertificatesFolder != NULL)
	{
		// sc 12/02/2014 WAK0086544
		VFilePath vfPath;
		fCertificatesFolder->GetPath ( vfPath);
		vfPathCertificate = vfPath.ToSubFile( CVSTR ( "cert.pem" ) );
		if (vfPathCertificate.IsValid() && vfPathCertificate.IsFile())
		{
			VFile *certificateFile = new VFile( vfPathCertificate);
			if ((certificateFile != NULL) && certificateFile->Exists())
			{
				fCertificatesFolder->GetPath ( vfPath);
				vfPathKey = vfPath.ToSubFile ( CVSTR ( "key.pem" ) );
				if (vfPathKey.IsValid() && vfPathKey.IsFile())
				{
					VFile *keyFile = new VFile( vfPathKey);
					if ((keyFile != NULL) && keyFile->Exists())
					{
						jswchFactory->SetIsSSL ( true );
						fSecuredConnection = true;
					}
					ReleaseRefCountable( &keyFile);
				}
			}
			ReleaseRefCountable( &certificateFile);
		}
	}

	jswchFactory-> SetDebugger ( this );

	/*
		Look for the next available port.
		The code could be generalized and put directly into VServer and VListener themselves.
		In that case a special care and logic is needed to deal with listeners on multiple 
		ports and cases when listeners from different "projects" register within the same server
		to be able to tell which port gets which new value.
	*/
	while ( nPort <= 65535 )
	{
		fTCPListener = new VTCPConnectionListener ( ); // sc 12/02/2014, a new VTCPConnectionListener must be created here
		fTCPListener-> AddWorkerPool ( vwPool );
		jswchFactory-> AddNewPort ( nPort );
		fTCPListener-> AddConnectionHandlerFactory ( jswchFactory );

		if (jswchFactory->IsSSL())
		{
			fTCPListener->SetSSLCertificatePaths ( vfPathCertificate, vfPathKey);
		}

		fServer-> AddConnectionListener ( fTCPListener );

		vError = fServer-> Start ( );
		if ( vError == VE_OK )
			break;

		fServer-> Stop ( );
		fTCPListener->WaitForDeath( 3000);
		ReleaseRefCountable( &fTCPListener);

		jswchFactory-> RemoveAllPorts ( );
		nPort++;
	}

	vwPool-> Release ( );
	jswchFactory-> Release ( );

	/* TODO: Error handling */

	return 0;
}

VJSWDebugger::VJSWDebugger ( )
{
	fServer = NULL;
	fTCPListener = NULL;
	fSettings = 0;
	fCertificatesFolder = NULL;
	fSecuredConnection = false;
}


VJSWDebugger::~VJSWDebugger()
{
	ReleaseRefCountable( &fServer);
	ReleaseRefCountable( &fTCPListener);
	ReleaseRefCountable( &fCertificatesFolder);
}


short VJSWDebugger::GetServerPort ()
{
	if ( fServer == 0 )
		return -1;

	std::vector<PortNumber>						vctrPorts;
	fServer-> GetPublishedPorts ( vctrPorts );

	xbox_assert ( vctrPorts. size ( ) == 1 );

	return vctrPorts [ 0 ];
}


bool VJSWDebugger::IsSecuredConnection()
{
	return fSecuredConnection;
}


bool VJSWDebugger::HasClients ( )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return false;

	bool		bResult = cHandler-> IsHandling ( );
	cHandler-> Release ( );

	return bResult;
}

int VJSWDebugger::Write ( const char * inData, long inLength, bool inToUTF8 )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return 0;

	int		nResult = cHandler-> Write ( inData, inLength, inToUTF8 );
	cHandler-> Release ( );

	return nResult;
}

int VJSWDebugger::WriteFileContent ( long inCommandID, uintptr_t inContext, const unsigned short* inFilePath, int inPathSize )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return 0;

	int		nResult = cHandler-> WriteFileContent ( inCommandID, inContext, inFilePath, inPathSize );
	cHandler-> Release ( );

	return nResult;
}

int VJSWDebugger::WriteSource ( long inCommandID, uintptr_t inContext, const unsigned short* inSource, int inSize )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return 0;

	int		nResult = cHandler-> WriteSource ( inCommandID, inContext, inSource, inSize );
	cHandler-> Release ( );

	return nResult;
}


bool VJSWDebugger::BreakpointReached(	OpaqueDebuggerContext	inContext,
										int						inLineNumber,
										int						inExceptionHandle,
										char*					inURL,
										int						inURLLength,
										char*					inFunction,
										int 					inFunctionLength,
										char*					inMessage,
										int 					inMessageLength,
										char* 					inName,
										int 					inNameLength,
										long 					inBeginOffset,
										long 					inEndOffset )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return 0;

	int		nResult = cHandler-> SendBreakPoint (
											(uintptr_t)inContext,
											inExceptionHandle,
											inURL, inURLLength,
											inFunction, inFunctionLength,
											inLineNumber,
											inMessage, inMessageLength,
											inName, inNameLength,
											inBeginOffset, inEndOffset );
	cHandler-> Release ( );

	return nResult;
}

void VJSWDebugger::SetSourcesRoot ( char* inRoot, int inLength )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return;

	cHandler-> SetSourcesRoot ( inRoot, inLength );
	cHandler-> Release ( );
}


void VJSWDebugger::Reset ( )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return;

	cHandler-> Reset ( );
	cHandler-> Release ( );
}


void VJSWDebugger::WakeUpAllWaiters ( )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return;

	cHandler-> WakeUpAllWaiters ( );
	cHandler-> Release ( );
}


long long VJSWDebugger::GetMilliseconds ( )
{
	return VSystem::GetCurrentTime ( );
}


char* VJSWDebugger::GetRelativeSourcePath (
										const unsigned short* inAbsoluteRoot, int inRootSize,
										const unsigned short* inAbsolutePath, int inPathSize,
										int& outSize )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return 0;

	char*		szchResult = cHandler-> GetRelativeSourcePath ( inAbsoluteRoot, inRootSize, inAbsolutePath, inPathSize, outSize );
	cHandler-> Release ( );

	return szchResult;
}


char* VJSWDebugger::GetAbsolutePath (
									const unsigned short* inAbsoluteRoot, int inRootSize,
									const unsigned short* inRelativePath, int inPathSize,
									int& outSize )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return 0;

	char*		szchResult = cHandler-> GetAbsolutePath ( inAbsoluteRoot, inRootSize, inRelativePath, inPathSize, outSize );
	cHandler-> Release ( );

	return szchResult;
}

VJSWConnectionHandler* VJSWDebugger::_RetainFirstHandler ( )
{
	VJSWConnectionHandler*		cHandler = 0;

	fHandlersLock. Lock ( );

		std::vector<VJSWConnectionHandler*>::iterator	iter = fHandlers. begin ( );
		while ( iter != fHandlers. end ( ) )
		{
			if ( ( *iter )-> IsHandling ( ) )
			{
				cHandler = *iter;
				cHandler-> Retain ( );

				break;
			}
			else if ( ( *iter )-> IsDone ( ) )
			{
				( *iter )-> Release ( );
				iter = fHandlers. erase ( iter );
			}
			else
			{
				++iter;
			}
		}

	fHandlersLock. Unlock ( );

	return cHandler;
}

VError VJSWDebugger::RemoveAllHandlers ( )
{
	fHandlersLock. Lock ( );

		std::vector<VJSWConnectionHandler*>::iterator	iter = fHandlers. begin ( );
		while ( iter != fHandlers. end ( ) )
		{
			( *iter )-> Release ( );

			iter++;
		}
		fHandlers. clear ( );

	fHandlersLock. Unlock ( );

	return VE_OK;
}


WAKDebuggerServerMessage* VJSWDebugger::WaitFrom(OpaqueDebuggerContext inContext)
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler();
	if ( cHandler == 0 )
		return 0;

	WAKDebuggerServerMessage*	dcResult = cHandler->WaitForClientCommand( (uintptr_t)inContext );
	cHandler->Release();

	return dcResult;
}
void VJSWDebugger::DisposeMessage(WAKDebuggerServerMessage* inMessage)
{
	xbox_assert(false);// appeler le free correspondant a cHandler->WaitForClientCommand 
}

bool VJSWDebugger::SetState(OpaqueDebuggerContext inContext, WAKDebuggerState_t state)
{
	xbox_assert(false);//TBC
	return true;
}
bool VJSWDebugger::SendLookup( OpaqueDebuggerContext inContext, void* inVars, unsigned int inSize )
{
	xbox_assert(false);//TBC
	return true;
}
bool VJSWDebugger::SendEval( OpaqueDebuggerContext inContext, void* inVars )
{
	xbox_assert(false);//TBC
	return true;
}

bool VJSWDebugger::SendCallStack( OpaqueDebuggerContext inContext, const char *inData, int inLength,
											const char*				inExceptionInfos,
											int						inExceptionInfosLength )
{
	xbox_assert(false);//TBC
	return true;
}
//bool VJSWDebugger::SendSource( OpaqueDebuggerContext inContext, intptr_t inSrcId, const char *inData, int inLength, const char* inUrl, unsigned int inUrlLen )
bool VJSWDebugger::SendSource(				OpaqueDebuggerContext	inContext,
											intptr_t				inSourceId,
											char*					inFileName,
											int						inFileNameLength,
											const char*				inData,
											int						inDataLength )
{
	bool result = true;
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler();
	if ( cHandler )
	{
		IWAKDebuggerSettings*	settings = cHandler->GetSettings();
		if (settings)
		{
			VString		fileName;
			VString		data;

			if (inFileName)
			{
				fileName = VString( inFileName, (VSize)(inFileNameLength),  VTC_UTF_16 );
			}
			if (inData)
			{
				data = VString( inData, (VSize)(inDataLength),  VTC_UTF_16 );
			}
			settings->Set(inContext,fileName,inSourceId,data);
		}
		cHandler->Release();
	}
	return result;
}

void VJSWDebugger::Trace(OpaqueDebuggerContext inContext, const void* inString, int inSize,
							WAKDebuggerTraceLevel_t inTraceLevel )
{
	VString		l_trace(inString,(VSize)inSize,VTC_UTF_16);
	VString l_tmp = CVSTR("DEBUG >>>> ");
	l_tmp += l_trace;
	l_tmp += CVSTR(" '\n");
#if defined(UNIFIED_DEBUGGER_NET_DEBUG)
	if (!sSck)
	{
#endif
		DebugMsg(l_tmp);
#if defined(UNIFIED_DEBUGGER_NET_DEBUG)
	}
	else
	{
		StErrorContextInstaller errCtx(false, true);

		char	l_data[1024];
		l_tmp.ToCString(l_data,1024);
		uLONG	l_len=strlen(l_data);
		if (sSck->Write(l_data, &l_len, false))
		{
			sSck->Close();
			delete sSck;
			sSck = NULL;
		}
	}
#endif
}


WAKDebuggerServerMessage* VJSWDebugger::GetNextBreakPointCommand()
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return 0;

	WAKDebuggerServerMessage*	dcResult = cHandler-> GetNextBreakPointCommand ( );

	cHandler-> Release ( );

	return dcResult;
}


WAKDebuggerServerMessage* VJSWDebugger::GetNextSuspendCommand( OpaqueDebuggerContext inContext )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return 0;

	WAKDebuggerServerMessage*	dcResult = cHandler-> GetNextSuspendCommand ( (uintptr_t)inContext );

	cHandler-> Release ( );

	return dcResult;
}


WAKDebuggerServerMessage* VJSWDebugger::GetNextAbortScriptCommand ( OpaqueDebuggerContext inContext )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return 0;

	WAKDebuggerServerMessage*	dcResult = cHandler-> GetNextAbortScriptCommand ( (uintptr_t)inContext );

	cHandler-> Release ( );

	return dcResult;
}



WAKDebuggerUCharPtr_t VJSWDebugger::EscapeForJSON( const unsigned char* inString, int inSize, int& outSize )
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

void VJSWDebugger::DisposeUCharPtr( WAKDebuggerUCharPtr_t inUCharPtr )
{
	if (inUCharPtr)
	{
		delete [] inUCharPtr;
	}
}

void* VJSWDebugger::UStringToVString( const void* inString, int inSize )
{
	xbox_assert(false);//should not be used in this dbg srv
	return NULL;
}



VError VJSWDebugger::AddHandler ( VJSWConnectionHandler* inHandler )
{
	xbox_assert ( inHandler != 0 );
	if ( inHandler == 0 )
		return VE_INVALID_PARAMETER;

	fHandlersLock. Lock ( );
		inHandler-> Retain ( );
		fHandlers. push_back ( inHandler );
		if ( fSettings != 0 )
			inHandler-> SetSettings ( fSettings );
	fHandlersLock. Unlock ( );

	return VE_OK;
}

bool VJSWDebugger::HasBreakpoint(			OpaqueDebuggerContext				inContext,
											intptr_t							inSourceId,
											int									inLineNumber)
{
	bool hasBkrpt = false;
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler();
	if ( cHandler )
	{
		IWAKDebuggerSettings*	settings = cHandler->GetSettings();
		if (settings)
		{
			hasBkrpt = settings->HasBreakpoint(inContext,inSourceId,inLineNumber);
		}
		cHandler->Release();
	}
	return hasBkrpt;
}

OpaqueDebuggerContext VJSWDebugger::AddContext( uintptr_t inContext, void* inContextRef )
{
	OpaqueDebuggerContext		newCtx = NULL;
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler();

	if ( cHandler == 0 )
		return newCtx;

	int		nResult = cHandler->SendContextCreated( inContext );

	if (!nResult)
	{
		IWAKDebuggerSettings*	settings = cHandler->GetSettings();
		newCtx = (OpaqueDebuggerContext)inContext;
		if (settings)
		{
			settings->Add(newCtx);
		}
	}
	cHandler->Release();
	return newCtx;
}

bool VJSWDebugger::RemoveContext( OpaqueDebuggerContext inContext )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler();
	if ( cHandler == 0 )
		return 0;

	int		nResult = cHandler->SendContextDestroyed( (uintptr_t)inContext );
	IWAKDebuggerSettings*	settings = cHandler->GetSettings();
	if (settings)
	{
		settings->Remove(inContext);
	}
	cHandler->Release();

	return (nResult == 0);
}

WAKDebuggerType_t VJSWDebugger::GetType()
{
	return REGULAR_DBG_TYPE;
}




IWAKDebuggerServer* JSWDebuggerFactory::Get()
{
	return VJSWDebugger::Get();
}

IRemoteDebuggerServer* JSWDebuggerFactory::GetNoDebugger()
{
	return VNoRemoteDebugger::Get();
}


IChromeDebuggerServer* JSWDebuggerFactory::GetChromeDebugHandler(const XBOX::VString& inSolutionName)
{
	return VChromeDebugHandler::Get(inSolutionName);
}

IChromeDebuggerServer* JSWDebuggerFactory::GetChromeDebugHandler(
		const XBOX::VString&	inSolutionName,
		const XBOX::VString&	inTracesHTMLSkeleton)
{
#if USE_V8_ENGINE
	return VChromeDebugHandler::Get(inSolutionName,inTracesHTMLSkeleton,true);
#else
	return VChromeDebugHandler::Get(inSolutionName,inTracesHTMLSkeleton,false);
#endif
}




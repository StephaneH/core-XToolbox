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
#ifndef __JSW_DEBUGGER_IMPL__
#define __JSW_DEBUGGER_IMPL__

#include "JSDebugger/Interfaces/CJSWDebuggerFactory.h"
#include "ServerNet/VServerNet.h"
#include "VChromeDebugHandler.h"
#include "VNoRemoteDebugger.h"

class VJSWConnectionHandler;

USING_TOOLBOX_NAMESPACE


class VJSWDebuggerCommand :
			public VObject,
			public IWAKDebuggerCommand
{
	public :

		VJSWDebuggerCommand( IWAKDebuggerCommand::WAKDebuggerServerMsgType_t inCommand, const VString & inID, const VString & inContextID, const VString & inParameters );

		virtual ~VJSWDebuggerCommand ( ) { ; }

		virtual IWAKDebuggerCommand::WAKDebuggerServerMsgType_t GetType ( ) const { return fCommand; }

		virtual const char* GetParameters ( ) const;
		virtual const char* GetID ( ) const;
		virtual void Dispose ( );
		virtual bool HasSameContextID ( uintptr_t inContextID ) const;
		//virtual const char* GetContextID ( ) const;

	private :

		IWAKDebuggerCommand::WAKDebuggerServerMsgType_t			fCommand;

		VString								fID;
		VString								fContextID;
		VString								fParameters;
};


class VJSWDebugger :
	public VObject,
	public IWAKDebuggerServer
{
	public :

		virtual ~VJSWDebugger();

		static VJSWDebugger* Get ( );
		virtual bool HasClients ( );
		virtual short GetServerPort ( );
		virtual	bool IsSecuredConnection();
		virtual int StartServer ( );
		virtual int StopServer ( );
		VError RemoveAllHandlers ( );
		virtual int Write ( const char * inData, long inLength, bool inToUTF8 = false );
		virtual int WriteFileContent ( long inCommandID, uintptr_t inContext, const unsigned short* inFilePath, int inPathSize );
		virtual int WriteSource ( long inCommandID, uintptr_t inContext, const unsigned short* inSource, int inSize );
		virtual VError AddHandler ( VJSWConnectionHandler* inHandler );
		virtual void SetSourcesRoot ( char* inRoot, int inLength );
		virtual void SetCertificatesFolder( const void* inFolderPath);
		virtual char* GetAbsolutePath (
									const unsigned short* inAbsoluteRoot, int inRootSize,
									const unsigned short* inRelativePath, int inPathSize,
									int& outSize );
		virtual char* GetRelativeSourcePath (
										const unsigned short* inAbsoluteRoot, int inRootSize,
										const unsigned short* inAbsolutePath, int inPathSize,
										int& outSize );
		virtual long long GetMilliseconds ( );
		virtual void Reset ( );
		virtual void WakeUpAllWaiters ( );

		virtual WAKDebuggerType_t			GetType();
		virtual void						SetInfo( IWAKDebuggerInfo* inInfo );
		virtual void						SetSettings( IWAKDebuggerSettings* inSettings );
		virtual void						SetSourcesRoot( void* inSourcesRootVStringPtr ) {;}
		virtual void						GetStatus(
												bool&			outStarted,
												bool&			outConnected,
												long long&		outDebuggingEventsTimeStamp,
												bool&			outPendingContexts);
		virtual bool						HasBreakpoint(
											OpaqueDebuggerContext				inContext,
											intptr_t							inSourceId,
											int									inLineNumber);
		virtual OpaqueDebuggerContext		AddContext( uintptr_t inContext, void* inContextRef );
		virtual bool						RemoveContext( OpaqueDebuggerContext inContext );
		virtual bool						SetState(OpaqueDebuggerContext inContext, WAKDebuggerState_t state);
		virtual bool						SendLookup( OpaqueDebuggerContext inContext, void* inVars, unsigned int inSize );
		virtual bool						SendEval( OpaqueDebuggerContext inContext, void* inVars );
		virtual bool						BreakpointReached(
												OpaqueDebuggerContext	inContext,
												int						inLineNumber,
												int						inExceptionHandle = -1/* -1 ? notException : ExceptionHandle */,
												char*					inURL  = NULL,
												int						inURLLength = 0/* in bytes */,
												char*					inFunction = NULL,
												int 					inFunctionLength = 0 /* in bytes */,
												char*					inMessage = NULL,
												int 					inMessageLength = 0 /* in bytes */,
												char* 					inName = NULL,
												int 					inNameLength = 0 /* in bytes */,
												long 					inBeginOffset = 0,
												long 					inEndOffset = 0 /* in bytes */ );
		virtual bool						SendCallStack(
												OpaqueDebuggerContext	inContext,
												const char				*inData,
												int						inLength,
												const char*				inExceptionInfos = NULL,
												int						inExceptionInfosLength = 0 );
		//virtual bool						SendSource( OpaqueDebuggerContext inContext, intptr_t inSrcId, const char *inData, int inLength, const char* inUrl, unsigned int inUrlLen );
		virtual WAKDebuggerServerMessage*	WaitFrom(OpaqueDebuggerContext inContext);
		virtual bool						SendSource(
												OpaqueDebuggerContext	inContext,
												intptr_t				inSourceId,
												char*					inFileName = NULL,
												int						inFileNameLength = 0,
												const char*				inData = NULL,
												int						inDataLength = 0 );
		virtual void						DisposeMessage(WAKDebuggerServerMessage* inMessage);
		virtual WAKDebuggerServerMessage* GetNextBreakPointCommand();
		virtual WAKDebuggerServerMessage* GetNextSuspendCommand( OpaqueDebuggerContext inContext );
		virtual WAKDebuggerServerMessage* GetNextAbortScriptCommand ( OpaqueDebuggerContext inContext );

		virtual WAKDebuggerUCharPtr_t		EscapeForJSON( const unsigned char* inString, int inSize, int& outSize );
		virtual void						DisposeUCharPtr( WAKDebuggerUCharPtr_t inUCharPtr );

		virtual void*						UStringToVString( const void* inString, int inSize );

		virtual void						Trace(OpaqueDebuggerContext inContext, const void* inString, int inSize,
												WAKDebuggerTraceLevel_t inTraceLevel = WAKDBG_ERROR_LEVEL );

	private :

		static VJSWDebugger*				sDebugger;
		VServer*							fServer;
		VTCPConnectionListener*				fTCPListener;
		VCriticalSection					fHandlersLock;
		std::vector<VJSWConnectionHandler*>	fHandlers;
		IWAKDebuggerSettings*				fSettings;
		VFolder*							fCertificatesFolder;
		bool								fSecuredConnection;

		VJSWDebugger ( );

		VJSWConnectionHandler* _RetainFirstHandler ( ); // To be used as a shortcut while debugger is single-connection.

//#define UNIFIED_DEBUGGER_NET_DEBUG
#if defined(UNIFIED_DEBUGGER_NET_DEBUG)
static XBOX::XTCPSock*						sSck;
#endif

};

/*
Should inherit from a pure virtual class, exposed to JavaScript Core debugger when a new context event is sent.
This would avoid a mutex lock each time a context needs to know if there is a new pending runtime event for it.

class VJSWContextRunTimeInfo : public VObject
{
	public :

		VJSWContextRunTimeInfo ( uintptr_t inContext );

		uintptr_t GetID ( ) { return fContext; }

		bool HasNewRunTimeInterrupt ( ); // Has either a "pause" or an "abort" commands pending.
		bool HasNewBreakPoint ( );

		void AddRunTimeInterrupt ( );
		void AddBreakPoint ( );

	private :

		uintptr_t							fContext;
		sLONG								fInterruptCount;
		sLONG								fBreakPointCount;
};
*/


#endif //__JSW_DEBUGGER__

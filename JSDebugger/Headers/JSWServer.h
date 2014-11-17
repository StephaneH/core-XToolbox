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
#ifndef __JSW_SERVER__
#define __JSW_SERVER__


#include "Kernel/VKernel.h"

#include "ServerNet/VServerNet.h"
#include "JSWDebugger.h"

USING_TOOLBOX_NAMESPACE

class VJSWConnectionHandler : public VConnectionHandler
{
	public :

		enum	{ Handler_Type = 'JSWH' };

		VJSWConnectionHandler ( );
		virtual ~VJSWConnectionHandler ( );

		virtual VError SetEndPoint ( VEndPoint* inEndPoint );
		virtual bool CanShareWorker ( ) { return false; } /* Exclusive. */

		virtual enum VConnectionHandler::E_WORK_STATUS Handle ( VError& outError );

		virtual int GetType ( ) { return Handler_Type; }
		virtual VError Stop ( );

		virtual void SetInfo( IWAKDebuggerInfo* inInfo );
		virtual void SetSettings( IWAKDebuggerSettings* inSettings );
		virtual IWAKDebuggerSettings* GetSettings() {return fDebuggerSettings;}

		bool IsHandling ( );
		bool IsDone ( ) { return fIsDone; }; /* There is a short but non-zero time in the beginning of life of a handler during which it is not handling yet and not done yet too. */
		int Write ( const char * inData, long inLength, bool inToUTF8 = false );
		int WriteFileContent ( long inCommandID, uintptr_t inContext, const unsigned short* inFilePath, int inPathSize );
		int WriteSource ( long inCommandID, uintptr_t inContext, const unsigned short* inSource, int inSize );

		void SetSourcesRoot ( char* inRoot, int inLength );
		int SendBreakPoint (
						uintptr_t inContext,
						int inExceptionHandle /* -1 ? notException : ExceptionHandle */,
						char* inURL, int inURLLength /* in bytes */,
						char* inFunction, int inFunctionLength /* in bytes */,
						int inLine,
						char* inMessage = 0, int inMessageLength = 0 /* in bytes */,
						char* inName = 0, int inNameLength = 0 /* in bytes */,
						long inBeginOffset = 0, long inEndOffset = 0 /* in bytes */ );
		int SendContextCreated ( uintptr_t inContext );
		int SendContextDestroyed ( uintptr_t inContext );

		void Reset ( ) { fBlockWaiters = true; }
		VError WakeUpAllWaiters ( );

		IWAKDebuggerCommand* WaitForClientCommand( uintptr_t inContext );
		IWAKDebuggerCommand* GetNextBreakPointCommand ( );
		IWAKDebuggerCommand* GetNextSuspendCommand ( uintptr_t inContext );
		IWAKDebuggerCommand* GetNextAbortScriptCommand ( uintptr_t inContext );

		char* GetRelativeSourcePath (
								const unsigned short* inAbsoluteRoot, int inRootSize,
								const unsigned short* inAbsolutePath, int inPathSize,
								int& outSize );
		char* GetAbsolutePath (
								const unsigned short* inAbsoluteRoot, int inRootSize,
								const unsigned short* inRelativePath, int inPathSize,
								int& outSize );


	private :

		VTCPEndPoint*					fEndPoint;
		VCriticalSection				fEndPointMutex;
		bool							fShouldStop;
		bool							fIsDone;

		IWAKDebuggerInfo*				fDebuggerInfo;
		IWAKDebuggerSettings*			fDebuggerSettings;

		bool							fNeedsAuthentication;
		bool							fHasAuthenticated;
		//std::vector<VJSWContextRunTimeInfo*>	fContexts;

		/* TODO: VString as a buffer will do for now. To be replaced by a real buffer when the protocol
		is finalized. */
		VString							fBuffer;

		VString							fNonce;
		VString							fSourcesRoot;
		sLONG							fSequenceGenerator;
		VSyncEvent						fCommandEvent;
		VCriticalSection				fCommandsMutex;
		std::vector<VJSWDebuggerCommand*>		sClientCommands;
		bool							fBlockWaiters;


		void _SetEndPoint ( VTCPEndPoint* inEndPoint );
		VError ShakeHands ( );
		VError ReadLine ( );
		VError SendPacket ( const VString & inBody );
		VError ReadCrossfirePacket ( uLONG inLength, XBOX::VValueBag & outMessage );

		VError ExtractCommands ( );
		VError AddCommand ( const VString & inCommand );

		VError AddCommand( IWAKDebuggerCommand::WAKDebuggerServerMsgType_t inCommand, const VString & inID, const VString & inContextID );
		VError AddCommand( IWAKDebuggerCommand::WAKDebuggerServerMsgType_t inCommand, const VString & inID, const VString & inContextID, const VString & inParameters );
		VError SendContextList ( long inCommandID, uintptr_t* inContextIDs, IWAKDebuggerInfo::JSWD_CONTEXT_STATE* outStates, int inCount );

		VError AddCommand ( const VValueBag & inCommand );
		VError ClearCommands ( );

		VError HandleAuthenticate ( long inCommandID, const VString & inUserName, const VString & inUserPassword );

		VError WriteSource ( long inCommandID, uintptr_t inContext, const VString & vstrSource );

		VError SendAuthenticationHandled ( long inCommandID, bool inSuccess );
		VError SendAuthenticationNeeded ( const VString & inCommandID, const VString & inCommand );

		bool HasAccessRights ( );

		static void CalculateRelativePath ( const VString & inAbsolutePOSIXRoot, const VString & inAbsolutePOSIXPath, VString & outPath );
		static VError ReadFile ( VString inFileURL, VString & outContent );
		static void ParseJSCoreURL ( VString & ioURL );

		//void PostInterruptForContext ( const VString & inContextID );
};

class VJSWConnectionHandlerFactory : public VTCPConnectionHandlerFactory
{
	public :

		VJSWConnectionHandlerFactory ( );
		virtual ~VJSWConnectionHandlerFactory ( );

		virtual VConnectionHandler* CreateConnectionHandler ( VError& outError );

		virtual VError AddNewPort ( PortNumber inPort );
		virtual VError GetPorts ( std::vector<PortNumber>& outPorts );
		VError RemoveAllPorts ( );

		virtual int GetType ( ) { return VJSWConnectionHandler::Handler_Type; }

		VError SetDebugger ( VJSWDebugger* inDebugger );

	private:

		std::vector<PortNumber>				fPorts;

		VJSWDebugger*					fDebugger;
};


#endif //__JSW_SERVER__


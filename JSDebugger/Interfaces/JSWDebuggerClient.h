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
#ifndef __CJSW_DEBUGGER_CLIENT__
#define __CJSW_DEBUGGER_CLIENT__


#include "ServerNet/VServerNet.h"

#ifdef JSDEBUGGER_EXPORTS
	#define JSDEBUGGER_API __declspec(dllexport)
#else
	#ifdef __GNUC__
		#define JSDEBUGGER_API __attribute__((visibility("default")))
	#else
		#define JSDEBUGGER_API __declspec(dllimport)
	#endif
#endif


#include <vector>
#include "JSWDebuggerErrors.h"

class VJSWDebuggerValue;
class VJSDebuggerMessage;
class VJSDebuggerCallback;

class JSDEBUGGER_API IJSDebuggerListener : public XBOX::IRefCountable
{
	public :

		virtual XBOX::VError OnBreakPoint ( const XBOX::VString & inContextID, const XBOX::VString & inFilePath, const XBOX::VString & inFunctionName, int inLineNumber ) = 0;
		virtual XBOX::VError OnException (
									const XBOX::VString & inContextID,
									const XBOX::VString & inFilePath, const XBOX::VString & inFunctionName,
									int inLineNumber, uLONG inBeginOffset, uLONG inEndOffset,
									const XBOX::VString & inMessage, const XBOX::VString & inName,
									sLONG inExceptionHandle ) = 0;
		virtual XBOX::VError OnContextCreated ( const XBOX::VString & inMessage ) = 0;
		virtual XBOX::VError OnContextDestroyed ( const XBOX::VString & inMessage ) = 0;
		// virtual XBOX::VError OnConnection ( const XBOX::VError & inError ) = 0;
		virtual XBOX::VError OnError ( const XBOX::VError & inError ) = 0;

		// Asynchronous callbacks
		virtual XBOX::VError OnAuthenticate ( bool inSuccess, const XBOX::VString & inIPAddress ) = 0;
		virtual XBOX::VError OnEvaluate ( VJSWDebuggerValue* inValue, XBOX::VError inError ) = 0;
		virtual XBOX::VError OnLookup ( VJSWDebuggerValue* inValue, XBOX::VError inError ) = 0;
		virtual XBOX::VError OnScope ( VJSWDebuggerValue* inValue, XBOX::VError inError ) = 0;
		virtual XBOX::VError OnScopeChain ( VJSWDebuggerValue* inValue, XBOX::VError inError ) = 0;
		virtual XBOX::VError OnScript ( VJSWDebuggerValue* inValue, XBOX::VError inError ) = 0;
		virtual XBOX::VError OnCallStack ( VJSWDebuggerValue* inValue, XBOX::VError inError ) = 0;
		virtual XBOX::VError OnListContexts ( VJSWDebuggerValue* inValue, XBOX::VError inError ) = 0;
	
		virtual sLONG GetUIID ( ) { return 0; }
};


class JSDEBUGGER_API VJSWDebuggerValue
{
	public :
		
		VJSWDebuggerValue ( const XBOX::VString & inClassName );
		~VJSWDebuggerValue ( );

		bool IsNull ( ) { return fIsNull; }
		bool IsUndefined ( ) { return fIsUndefined; }
		bool IsUnknown ( ) { return fIsUnknown; }
		bool IsReal ( ) { return fReal != 0; }
		bool IsString ( ) { return fString != 0; }
		bool IsBoolean ( ) { return fBoolean != 0; }
		bool IsObject ( ) { return fProperties != 0; }

		Real GetReal ( ) { return fReal == 0 ? 0 : *fReal; }
		bool GetBoolean ( ) { return fBoolean == 0 ? false : *fBoolean; }
		void GetString ( XBOX::VString& outString );
		XBOX::VError GetJSONForEvaluate ( XBOX::VString& outString );

		void MakeNull ( );
		void MakeUndefined ( );
		void MakeUnknown ( );
		void SetReal ( Real inValue );
		void SetBoolean ( bool inValue );
		void SetString ( const XBOX::VString & inValue );

		XBOX::VError AddProperty ( const XBOX::VString & inName );
		XBOX::VError GetProperties ( XBOX::VectorOfVString& outProperties );

		XBOX::VError AddFunction ( const XBOX::VString & inName );
		XBOX::VError GetFunctions ( XBOX::VectorOfVString& outFunctions );

		void ToString ( XBOX::VString& outString, bool inOneLine = false );

		void SetCallbackID ( const XBOX::VString & inID ) { fCallbackID. FromString ( inID ); }
		//void GetCallbackID ( XBOX::VString & outID ) { outID. FromString ( fCallbackID ); }

		void SetJSON ( const XBOX::VString & inJSON ) { fJSON. FromString ( inJSON ); }
		const XBOX::VString & GetJSON ( ) const { return fJSON; }

	private :

		bool							fIsNull;
		bool							fIsUndefined;
		bool							fIsUnknown;
		Real*							fReal;
		XBOX::VString*					fString;
		bool*							fBoolean;
		std::vector<XBOX::VString>*		fProperties;
		std::vector<XBOX::VString>*		fFunctions;
		XBOX::VString					fClassName;
		XBOX::VString					fCallbackID;
		XBOX::VString					fJSON;

		void Reset ( );
		XBOX::VError PropertyToJSON ( const XBOX::VString& inProperty, XBOX::VString & outJSON );
		XBOX::VError FunctionToJSON ( const XBOX::VString& inFunction, XBOX::VString & outJSON );
};

class VDebuggerInnerListener : public XBOX::VObject, public IJSDebuggerListener
{
	public :

		VDebuggerInnerListener ( ) { ; }
		virtual ~VDebuggerInnerListener ( ) { ; }

		virtual XBOX::VError OnBreakPoint (
									const XBOX::VString & inContextID,
									const XBOX::VString & inFilePath,
									const XBOX::VString & inFunctionName,
									int inLineNumber ) { return XBOX::VE_OK; }
		virtual XBOX::VError OnException (
									const XBOX::VString & inContextID,
									const XBOX::VString & inFilePath, const XBOX::VString & inFunctionName,
									int inLineNumber, uLONG inBeginOffset, uLONG inEndOffset,
									const XBOX::VString & inMessage, const XBOX::VString & inName,
									sLONG inExceptionHandle ) { return XBOX::VE_OK; }
		virtual XBOX::VError OnContextCreated ( const XBOX::VString & inMessage ) { return XBOX::VE_OK; };
		virtual XBOX::VError OnContextDestroyed ( const XBOX::VString & inMessage ) { return XBOX::VE_OK; }
		virtual XBOX::VError OnError ( const XBOX::VError & inError ) { return XBOX::VE_OK; }

		// Asynchronous callbacks
		virtual XBOX::VError OnAuthenticate ( bool inSuccess, const XBOX::VString & inIPAddress ) { return XBOX::VE_OK; }
		virtual XBOX::VError OnEvaluate ( VJSWDebuggerValue* inValue, XBOX::VError inError ) { return XBOX::VE_OK; }
		virtual XBOX::VError OnLookup ( VJSWDebuggerValue* inValue, XBOX::VError inError ) { return XBOX::VE_OK; }
		virtual XBOX::VError OnScript ( VJSWDebuggerValue* inValue, XBOX::VError inError ) { return XBOX::VE_OK; }
		virtual XBOX::VError OnCallStack ( VJSWDebuggerValue* inValue, XBOX::VError inError ) { return XBOX::VE_OK; };
		virtual XBOX::VError OnListContexts ( VJSWDebuggerValue* inValue, XBOX::VError inError ) { return XBOX::VE_OK; };

		// OnGetScript
		// _GetScript - waits on a SyncEvent for a script callback
};

class JSDEBUGGER_API IJSDebugger : public XBOX::VObject, public XBOX::IRefCountable
{
public:
		virtual XBOX::VError	SetBreakPoint ( XBOX::VString const & inRelativeFilePath, XBOX::VIndex inLineNumber ) = 0;
		virtual XBOX::VError	SetBreakPoints ( XBOX::VString const & inRelativePath, std::vector< sLONG >& inLineNumbers, std::vector<bool>& inDisabled ) = 0;
		
		virtual XBOX::VError	RemoveBreakPoint ( XBOX::VString const & inRelativeFilePath, XBOX::VIndex inLineNumber ) = 0;
		virtual XBOX::VError	RemoveBreakPoints ( XBOX::VString const & inRelativePath, std::vector< sLONG >& inLineNumbers, std::vector<bool>& inDisabled ) = 0;
		
		virtual bool			SupportUpdate() = 0;
		virtual XBOX::VError	UpdateBreakPoints( XBOX::VString const & inRelativeFilePath, std::vector< sLONG >& inLineNumbers, std::vector<bool>& inDisabled ) = 0;
};


class JSDEBUGGER_API VJSDebugger : public IJSDebugger
{
	public:

		VJSDebugger ( );
		~VJSDebugger ( );

		XBOX::VError Connect ( XBOX::VString const & inDNSNameOrIP, short inPort, bool inIsSecured );

		bool IsConnected ( );
		XBOX::VError Disconnect ( );
		XBOX::VError Authenticate ( XBOX::VString const & inUserName, XBOX::VString const & inUserPassword );
		bool ServerHasDebuggerUsers ( ) { return fHasDebugUsers; }

		void SetFileRoot ( XBOX::VFilePath const & inRoot ) { fRootFolder. FromFilePath ( inRoot ); }
		void GetNonce ( XBOX::VString & outNonce ) const { outNonce. FromString ( fNonce ); }

		XBOX::VError Continue ( XBOX::VString const & inContext );
		XBOX::VError GetSourceCode ( XBOX::VString const & inContext, sLONG inFrame, XBOX::VString& outCode );
		XBOX::VError StepOver ( XBOX::VString const & inContext );
		XBOX::VError StepIn ( XBOX::VString const & inContext );
		XBOX::VError StepOut ( XBOX::VString const & inContext );
		XBOX::VError AbortScript ( XBOX::VString const & inContext );
		XBOX::VError Suspend ( XBOX::VString const & inContext );
		XBOX::VError GetCallStack ( XBOX::VString const & inContext, uLONG& outSequenceID, XBOX::VectorOfVString& outFunctions );
		XBOX::VError Evaluate ( XBOX::VString const & inContext, XBOX::VString const & inExpression, XBOX::VString const & inCallbackID );
		XBOX::VError Lookup ( XBOX::VString const & inObjectRef, XBOX::VString const & inContext, XBOX::VString const & inCallbackID );
		XBOX::VError GetScope ( XBOX::VString const & inContext, uLONG inFrame, uLONG& outSequenceID ); // Returns the local scope for a given frame
		XBOX::VError GetScopes ( XBOX::VString const & inContext, uLONG inFrame, uLONG& outSequenceID ); // Returns a scope chain for a given frame (local + all closures)
		XBOX::VError GetCurrentContexts ( );

		/* 0-based frame index, where original frame (on break-point) has index 0, its parent frame has index 1, etc... Thus, global (top) frame 
		has index (Call stack size - 1) */
		XBOX::VError SetCallFrame ( uWORD inIndex );

		virtual XBOX::VError SetBreakPoint ( XBOX::VString const & inRelativeFilePath, XBOX::VIndex inLineNumber );
		virtual XBOX::VError SetBreakPoints ( XBOX::VString const & inRelativeFilePath, std::vector< sLONG >& inLineNumbers, std::vector<bool>& inDisabled );
		// Per file??? XBOX::VError SetBreakPoints ( XBOX::VString const & inRelativeFilePath, std::vector<XBOX::VIndex> inLineNumbers );
		// All??? XBOX::VError SetBreakPoints ( XBOX::VValueBag const & inAllBreakPoints );
		virtual XBOX::VError RemoveBreakPoint ( XBOX::VString const & inRelativeFilePath, XBOX::VIndex inLineNumber );
		virtual XBOX::VError RemoveBreakPoints ( XBOX::VString const & inRelativeFilePath, std::vector< sLONG >& inLineNumbers, std::vector<bool>& inDisabled );

		virtual bool			SupportUpdate() {return false;}
		virtual XBOX::VError	UpdateBreakPoints( XBOX::VString const & inRelativeFilePath, std::vector< sLONG >& inLineNumbers, std::vector<bool>& inDisabled ) { return XBOX::VE_UNIMPLEMENTED; }
		/*
			So here is how it all works asynchronously. 
			First of all, an async callback needs to be added to the debugger instance. Then, any async method simply writes a debugger protocol command
			to the end-point and exits without waiting for a response. There is a dedicated VTask that runs and reads responses (events) from the server. 
			Once this task receives an async reponse it figures out its type and calls a corresponding method on the callback instance of the client debugger.
			Async callback has a pointer to a WebArea object so it can execute JavaScript within it to trigger a JavaScript callback associated with the 
			JavaScript instance of the debugger object.

			<SCRIPT type="text/javascript">
				function eval_callback ()
				{
					alert ( "eval callback handler" );
				}
				studio.DEBUGGER.onEvaluate = eval_callback;
			</SCRIPT>

			<form>
				<input type="button" value="Eval async" onclick="studio.DEBUGGER.evaluate();">
			</form>
		*/

		XBOX::VError AddListener ( IJSDebuggerListener* inListener );
		XBOX::VError RemoveListener ( IJSDebuggerListener* inListener );
		XBOX::VError RemoveAllListeners ( );
		XBOX::VError RemoveListenersWithUI ( );

	private:

		XBOX::VTask*						fWorker;
		XBOX::VSyncEvent					fSyncEvent;
		XBOX::VCriticalSection				fEndPointLock;
		XBOX::VTCPEndPoint*					fEndPoint;
		XBOX::VTask*						fCaller; // The "main" task which called ::Connect() method.
		sLONG								fSequenceGenerator;
		XBOX::VFilePath						fRootFolder;
		bool								fHasDoneHandShake;
		XBOX::VString						fServerTools;
		XBOX::VString						fNonce;
		bool								fNeedsAuthentication;
		bool								fHasAuthenticated;
		bool								fHasDebugUsers; // True by default. Is set to false if there are no users with JS debugger rights on the server

		XBOX::VCriticalSection				fListenersLock;
		std::vector<IJSDebuggerListener*>	fListeners;

		VDebuggerInnerListener*				fInnerListener;

		static sLONG Run ( XBOX::VTask* inTask );

		XBOX::VError ShakeHands ( );

		// A bit of an overkill but will do for now until the proper protocol is finalized.
		XBOX::VError ReadLine ( XBOX::VString& outLine );
		VJSWDebuggerValue* ReadValue ( XBOX::VError& outError );
		XBOX::VError SendPacket ( const XBOX::VString & inBody );

		XBOX::VError ReadCrossfireMessage ( uLONG inLength, XBOX::VValueBag & outMessage );
		XBOX::VError HandleAuthenticateResponse ( XBOX::VValueBag & inMessage );
		XBOX::VError HandleEvaluateResponse ( XBOX::VValueBag & inMessage );
		XBOX::VError HandleLookupResponse ( XBOX::VValueBag & inMessage );
		XBOX::VError HandleScopeResponse ( XBOX::VValueBag & inMessage );
		XBOX::VError HandleScopeChainResponse ( XBOX::VValueBag & inMessage );
		XBOX::VError HandleScriptResponse ( XBOX::VValueBag & inMessage );
		XBOX::VError HandleCallStackResponse ( XBOX::VValueBag & inMessage );
		XBOX::VError HandleListContextsResponse ( XBOX::VValueBag & inMessage );

		XBOX::VError HandleRemoteBreakpoint ( const XBOX::VValueBag & inMessage );
		XBOX::VError HandleRemoteException ( const XBOX::VValueBag & inMessage );
		XBOX::VError HandleRemoteContextCreated ( XBOX::VValueBag & inMessage );
		XBOX::VError HandleRemoteContextDestroyed ( XBOX::VValueBag & inMessage );
		XBOX::VError HandleRemoteEvent ( const XBOX::VString& inLine );
		XBOX::VError SendMessageToListeners ( VJSDebuggerMessage* inMessage );
		XBOX::VError SendCallbackToListeners ( VJSDebuggerCallback* inCallback );
		XBOX::VError GetException ( XBOX::VString& outMessage, XBOX::VString& outName, uLONG& outBeginOffset, uLONG& outEndOffset );

		XBOX::VError GetIP ( XBOX::VString & outIP );

		static void _DebugMsg ( const XBOX::VString & inMessage, const XBOX::VString & inContextID ); // OPTIONAL DEBUG ONLY
		static void _GetContextID ( const XBOX::VValueBag & inMessage, XBOX::VString & outContextID ); // OPTIONAL DEBUG ONLY
};


#endif //__CJSW_DEBUGGER_CLIENT__


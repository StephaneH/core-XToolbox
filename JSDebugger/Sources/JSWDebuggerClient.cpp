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
#include "Kernel/VKernel.h"
#include "ServerNet/VServerNet.h"

#include "JSWDebuggerClient.h"
#include "JSWDebuggerErrors.h"

USING_TOOLBOX_NAMESPACE
using namespace std;


#ifndef WAKANDA_DEBUGGER_MSG
	#define WAKANDA_DEBUGGER_MSG
#endif

// TODO: Define this structure in a separate new header file when project file is no longer in use
struct JSWDWorkerParameters
{
	VTCPEndPoint*					fEndPoint;
	VJSDebugger*					fDebugger;
	VSyncEvent*						fSyncEvent;

	JSWDWorkerParameters ( 	VTCPEndPoint* inEndPoint, VJSDebugger* inDebugger, VSyncEvent* inSyncEvent )
	{
		fEndPoint = inEndPoint;
		fDebugger = inDebugger;
		fSyncEvent = inSyncEvent;
	}
};

class VJSDebuggerMessage : public XBOX::VMessage
{
	public :

		enum JSWD_CM_TYPE
		{
			JSWD_CM_ON_BREAK_POINT = 0,
			JSWD_CM_ON_EXCEPTION,
			JSWD_CM_ON_CONTEXT_CREATED,
			JSWD_CM_ON_CONTEXT_DESTROYED
		};

		VJSDebuggerMessage (
						XBOX::VString inContextID,
						VJSDebuggerMessage::JSWD_CM_TYPE inType,
						const XBOX::VString & inMessage,
						const XBOX::VString & inName,
						const XBOX::VString & inFilePath,
						const XBOX::VString & inFunctionName,
						int inLineNumber,
						uLONG inBeginOffset,
						uLONG inEndOffset,
						sLONG nExceptionHandle = -1 );
		virtual ~VJSDebuggerMessage ( ) { ReleaseRefCountable ( &fListener ); }

		VJSDebuggerMessage::JSWD_CM_TYPE GetType ( ) { return fType; }
		void SetListener ( IJSDebuggerListener* inListener ) { CopyRefCountable ( &fListener, inListener ); }
		void Execute ( XBOX::VTask* inHandler );
		XBOX::VError GetError ( ) { return fError; }

	protected :

		virtual void DoExecute ( );

	private :

		XBOX::VString						fContextID;
		JSWD_CM_TYPE						fType;
		XBOX::VString						fMessage;
		XBOX::VString						fName;
		XBOX::VString						fFilePath;
		XBOX::VString						fFunctionName;
		int									fLineNumber;
		uLONG								fBeginOffset;
		uLONG								fEndOffset;
		sLONG								fExceptionHandle;
		IJSDebuggerListener*				fListener;

		XBOX::VError						fError;
};


class VJSDebuggerCallback : public XBOX::VMessage
{
	public :

		enum JSWD_CC_TYPE
		{
			JSWD_CC_ON_EVALUATE_RESULT = 0,
			JSWD_CC_ON_LOOKUP_RESULT,
			JSWD_CC_ON_SCOPE_RESULT,
			JSWD_CC_ON_SCOPE_CHAIN_RESULT,
			JSWD_CC_ON_SCRIPT_RESULT,
			JSWD_CC_ON_CALL_STACK_RESULT,
			JSWD_CC_ON_LIST_CONTEXTS,
			JSWD_CC_ON_AUTHENTICATE
		};

		VJSDebuggerCallback (
							VJSDebuggerCallback::JSWD_CC_TYPE inType,
							VJSWDebuggerValue* inValue );
		virtual ~VJSDebuggerCallback ( ) { ReleaseRefCountable ( &fCallback ); }

		VJSDebuggerCallback::JSWD_CC_TYPE GetType ( ) { return fType; }
		void SetCallback ( IJSDebuggerListener* inListener ) { CopyRefCountable ( &fCallback, inListener ); }
		void Execute ( XBOX::VTask* inHandler );
		XBOX::VError GetError ( ) { return fError; }

	protected :

		virtual void DoExecute ( );

	private :

		JSWD_CC_TYPE						fType;
		IJSDebuggerListener*				fCallback;
		VJSWDebuggerValue*					fValue; // Not the owner here

		XBOX::VError						fError;
};


VJSDebuggerMessage::VJSDebuggerMessage (
							XBOX::VString inContextID,
							VJSDebuggerMessage::JSWD_CM_TYPE inType,
							const XBOX::VString & inMessage,
							const XBOX::VString & inName,
							const XBOX::VString & inFilePath,
							const XBOX::VString & inFunctionName,
							int inLineNumber,
							uLONG inBeginOffset,
							uLONG inEndOffset,
							sLONG inExceptionHandle )
{
	fContextID. FromString ( inContextID );
	fListener = 0;
	fType = inType;
	fMessage. FromString ( inMessage );
	fName. FromString ( inName );
	fFilePath. FromString ( inFilePath );
	fFunctionName. FromString ( inFunctionName );
	fLineNumber = inLineNumber;
	fBeginOffset = inBeginOffset;
	fEndOffset = inEndOffset;
	fExceptionHandle = inExceptionHandle;

	fError = VE_OK;
}

void VJSDebuggerMessage::Execute ( XBOX::VTask* inHandler )
{
	if ( inHandler == 0 )
		return;

	bool		bOK = SendTo ( inHandler, 10 * 1000 );
	xbox_assert ( bOK );
}

void VJSDebuggerMessage::DoExecute ( )
{
	if ( fListener == 0 )
		fError = VE_JSW_DEBUGGER_CLIENT_LISTENER_NOT_FOUND;

	if ( fType == JSWD_CM_ON_BREAK_POINT )
		fError = fListener-> OnBreakPoint ( fContextID, fFilePath, fFunctionName, fLineNumber );
	else if ( fType == JSWD_CM_ON_EXCEPTION )
		fError = fListener-> OnException ( fContextID, fFilePath, fFunctionName, fLineNumber, fBeginOffset, fEndOffset, fMessage, fName, fExceptionHandle );
	else if ( fType == JSWD_CM_ON_CONTEXT_CREATED )
		fError = fListener-> OnContextCreated ( fMessage );
	else if ( fType == JSWD_CM_ON_CONTEXT_DESTROYED )
		fError = fListener-> OnContextDestroyed ( fMessage );
	else
	{
		fError = VE_UNIMPLEMENTED;
		xbox_assert ( false );
	}
}

VJSDebuggerCallback::VJSDebuggerCallback (
					VJSDebuggerCallback::JSWD_CC_TYPE inType,
					VJSWDebuggerValue* inValue )
{
	fCallback = 0;
	RetainRefCountable ( fCallback );
	fType = inType;
	fValue = inValue;

	fError = VE_OK;
}

void VJSDebuggerCallback::Execute ( XBOX::VTask* inHandler )
{
	if ( inHandler == 0 )
		return;

	SendTo ( inHandler, 10 * 1000 );
}

void VJSDebuggerCallback::DoExecute ( )
{
	xbox_assert ( fCallback != 0 );
	if ( fType == JSWD_CC_ON_EVALUATE_RESULT )
		fError = fCallback-> OnEvaluate ( fValue, fError );
	else if ( fType == JSWD_CC_ON_LOOKUP_RESULT )
		fError = fCallback-> OnLookup ( fValue, fError );
	else if ( fType == JSWD_CC_ON_SCOPE_RESULT )
		fError = fCallback-> OnScope ( fValue, fError );
	else if ( fType == JSWD_CC_ON_SCOPE_CHAIN_RESULT )
		fError = fCallback-> OnScopeChain ( fValue, fError );
	else if ( fType == JSWD_CC_ON_SCRIPT_RESULT )
		fError = fCallback-> OnScript ( fValue, fError );
	else if ( fType == JSWD_CC_ON_CALL_STACK_RESULT )
		fError = fCallback-> OnCallStack ( fValue, fError );
	else if ( fType == JSWD_CC_ON_LIST_CONTEXTS )
		fError = fCallback-> OnListContexts ( fValue, fError );
	else if ( fType == JSWD_CC_ON_AUTHENTICATE )
		fError = fCallback-> OnAuthenticate ( fValue-> GetBoolean ( ), fValue-> GetJSON ( ) );
	else
		xbox_assert ( false );
}



VJSWDebuggerValue::VJSWDebuggerValue ( const XBOX::VString & inClassName )
{
	fIsNull = false;
	fIsUndefined = true;
	fIsUnknown = false;
	fReal = 0;
	fString = 0;
	fBoolean = 0;
	fProperties = 0;
	fFunctions = 0;
	fClassName. FromString ( inClassName );
}
		
VJSWDebuggerValue::~VJSWDebuggerValue ( )
{

}

void VJSWDebuggerValue::Reset ( )
{
	fIsNull = false;
	fIsUndefined = true;
	fIsUnknown = false;
	if ( fReal != 0 )
		delete fReal;
	fReal = 0;
	if ( fString != 0 )
		delete fString;
	fString = 0;
	if ( fBoolean != 0 )
		delete fBoolean;
	fBoolean = 0;
	if ( fProperties != 0 )
		delete fProperties;
	fProperties = 0;
	if ( fFunctions != 0 )
		delete fFunctions;
	fFunctions = 0;
}

void VJSWDebuggerValue::GetString ( XBOX::VString& outString )
{
	if ( fString == 0 )
		return;

	outString. FromString ( *fString );
}

VError VJSWDebuggerValue::PropertyToJSON ( const VString& inProperty, VString & outJSON )
{
	VError			vError = VE_OK;

	/*
		Name - Object
	or
		Name - PrimaryType PrimaryValue
	*/

	VIndex			nSpace = inProperty. Find ( CVSTR ( " " ) );
	inProperty. GetSubString ( 1, nSpace - 1, outJSON );
	outJSON. AppendCString ( ":" );
	if ( inProperty. Find ( CVSTR ( "Object" ), nSpace + 3 ) == nSpace + 3 )
		outJSON. AppendCString ( "[object Object]" );
	else
	{
		VIndex		nSpace2 = inProperty. Find ( CVSTR ( " " ), nSpace + 3 );
		VString		vstrType;
		inProperty. GetSubString ( nSpace + 3, nSpace2 - ( nSpace + 3 ), vstrType );
		VString		vstrValue;
		inProperty. GetSubString ( nSpace2 + 1 , inProperty. GetLength ( ) - nSpace2, vstrValue );

		if ( vstrType. EqualToString ( "String" ) && vstrValue. BeginsWith ( CVSTR ( "\"" ) ) )
		{
			vstrValue. Insert ( CVSTR ( "\\" ), 1 );
			vstrValue. Insert ( CVSTR ( "\\" ), vstrValue. GetLength ( ) );
		}
		outJSON. AppendString ( vstrValue );
	}

	outJSON. Insert ( "\"", 1 );
	outJSON. AppendCString ( "\"" );

	return vError;
}

VError VJSWDebuggerValue::FunctionToJSON ( const VString& inFunction, VString & outJSON )
{
	VError			vError = VE_OK;

	outJSON. AppendCString ( "\"" );
	outJSON. AppendString ( inFunction );
	outJSON. AppendCString ( " : function " );
	outJSON. AppendString ( inFunction );
	outJSON. AppendCString ( "() {[native code]}\"" );

	return vError;
}

VError VJSWDebuggerValue::GetJSONForEvaluate ( XBOX::VString& outString )
{
	bool			bAutoBuild = true;

	VValueBag		vbagEval;
	vbagEval. SetString ( CrossfireKeys::type, CVSTR ( "evaluate" ) );
	vbagEval. SetString ( CrossfireKeys::id, fCallbackID );

	VValueBag*		vbagData = new VValueBag ( );
	if ( fIsNull )
	{
		vbagData-> SetString ( CrossfireKeys::type, CVSTR ( "null" ) );
		vbagData-> SetString ( CrossfireKeys::preview, CVSTR ( "null" ) );
	}
	else if ( fIsUndefined )
	{
		vbagData-> SetString ( CrossfireKeys::type, CVSTR ( "undefined" ) );
		vbagData-> SetString ( CrossfireKeys::preview, CVSTR ( "undefined" ) );
	}
	else if ( fIsUnknown )
	{
		vbagData-> SetString ( CrossfireKeys::type, CVSTR ( "unknown" ) );
		vbagData-> SetString ( CrossfireKeys::preview, CVSTR ( "unknown" ) );
	}
	else if ( fReal != 0 )
	{
		vbagData-> SetString ( CrossfireKeys::type, CVSTR ( "number" ) );
		vbagData-> SetReal ( CrossfireKeys::preview, *fReal );
	}
	else if ( fString != 0 )
	{
		vbagData-> SetString ( CrossfireKeys::type, CVSTR ( "string" ) );
		vbagData-> SetString ( CrossfireKeys::preview, *fString );	
	}
	else if ( fBoolean != 0 )
	{
		vbagData-> SetString ( CrossfireKeys::type, CVSTR ( "boolean" ) );
		if ( *fBoolean )
			vbagData-> SetString ( CrossfireKeys::preview, CVSTR ( "true" ) );
		else
			vbagData-> SetString ( CrossfireKeys::preview, CVSTR ( "false" ) );
	}
	else if ( fProperties != 0 || fFunctions != 0 )
	{
		bAutoBuild = false;
		outString. AppendCString ( "{\"type\":\"evaluate\",\"id\":\""  );
		outString. AppendString ( fCallbackID );
		outString. AppendCString ( "\",\"data\":{\"type\":\"object\",\"preview\":[\r\n" );

		sLONG		nTotalCount = 0;
		if ( fProperties != 0 )
		{
			nTotalCount += fProperties-> size ( );

			vector<XBOX::VString>::iterator		iterProps = fProperties-> begin ( );
			while ( iterProps != fProperties-> end ( ) )
			{
				VString		vstrJSONProp;
				PropertyToJSON ( *iterProps, vstrJSONProp );
				outString. AppendString ( vstrJSONProp );
				iterProps++;
				if ( iterProps != fProperties-> end ( ) || ( fFunctions != 0 && fFunctions-> size ( ) > 0 ) )
					outString. AppendCString ( "," );

				outString. AppendCString ( "\r\n" );
			}
		}
		if ( fFunctions != 0 )
		{
			nTotalCount += fFunctions-> size ( );

			vector<XBOX::VString>::iterator		iterFuncs = fFunctions-> begin ( );
			while ( iterFuncs != fFunctions-> end ( ) )
			{
				VString		vstrJSONFunc;
				FunctionToJSON ( *iterFuncs, vstrJSONFunc );
				outString. AppendString ( vstrJSONFunc );
				iterFuncs++;
				if ( iterFuncs != fFunctions-> end ( ) )
					outString. AppendCString ( "," );

				outString. AppendCString ( "\r\n" );
			}
		}
		outString. AppendCString ( "],\"count\":" );
		outString. AppendLong ( nTotalCount );
		outString. AppendCString ( ",\"type\":\"" );
		outString. AppendString ( fClassName );
		outString. AppendCString ( "\"}}" );
	}
	else
	{
		xbox_assert ( false );

		vbagData-> SetString ( CrossfireKeys::type, CVSTR ( "NOT IMPLEMENTED" ) );
		vbagData-> SetString ( CrossfireKeys::preview, CVSTR ( "NOT IMPLEMENTED" ) );
	}

	vbagEval. AddElement( CrossfireKeys::data, vbagData );
	vbagData-> Release ( );

	VError			vError = VE_OK;
	if ( bAutoBuild )
		vError = vbagEval. GetJSONString ( outString, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays );

	return vError;
}

void VJSWDebuggerValue::MakeNull ( )
{
	Reset ( );
	fIsUndefined = false;
	fIsNull = true;
}

void VJSWDebuggerValue::MakeUndefined ( )
{
	Reset ( );
}

void VJSWDebuggerValue::MakeUnknown ( )
{
	Reset ( );
	fIsUndefined = false;
	fIsUnknown = true;
}


void VJSWDebuggerValue::SetReal ( Real inValue )
{
	Reset ( );
	fIsUndefined = false;

	fReal = new Real ( );
	*fReal = inValue;
}

void VJSWDebuggerValue::SetBoolean ( bool inValue )
{
	Reset ( );
	fIsUndefined = false;

	fBoolean = new bool ( );
	*fBoolean = inValue;
}

void VJSWDebuggerValue::SetString ( const XBOX::VString & inValue )
{
	Reset ( );
	fIsUndefined = false;

	fString = new VString ( );
	fString-> FromString ( inValue );
}

XBOX::VError VJSWDebuggerValue::AddProperty ( const XBOX::VString & inName )
{
	fIsUndefined = false;
	if ( fProperties == 0 )
		fProperties = new std::vector<XBOX::VString> ( );

	fProperties-> push_back ( inName );

	return VE_OK;
}

XBOX::VError VJSWDebuggerValue::GetProperties ( XBOX::VectorOfVString& outProperties )
{
	if ( fProperties != 0 )
		outProperties. insert ( outProperties. begin ( ), fProperties-> begin ( ), fProperties-> end ( ) );

	return VE_OK;
}

XBOX::VError VJSWDebuggerValue::AddFunction ( const XBOX::VString & inName )
{
	fIsUndefined = false;
	if ( fFunctions == 0 )
		fFunctions = new std::vector<XBOX::VString> ( );

	fFunctions-> push_back ( inName );

	return VE_OK;
}

XBOX::VError VJSWDebuggerValue::GetFunctions ( XBOX::VectorOfVString& outFunctions )
{
	if ( fFunctions != 0 )
		outFunctions. insert ( outFunctions. begin ( ), fFunctions-> begin ( ), fFunctions-> end ( ) );

	return VE_OK;
}

void VJSWDebuggerValue::ToString ( XBOX::VString& outString, bool inOneLine )
{
	if ( fIsNull )
		outString. AppendCString ( "<NULL>" );
	else if ( fIsUndefined )
		outString. AppendCString ( "<UNDEFINED>" );
	else if ( fIsUnknown )
		outString. AppendCString ( "<UNKNOWN>" );
	else if ( fReal != 0 )
		outString. AppendReal ( *fReal );
	else if ( fString != 0 )
		outString. AppendString ( *fString );
	else if ( fBoolean != 0 )
	{
		if ( *fBoolean )
			outString. AppendCString ( "true" );
		else
			outString. AppendCString ( "false" );
	}

	if ( fProperties != 0  )
	{
		vector<VString>::iterator		iterProperties = fProperties-> begin ( );
		while ( iterProperties != fProperties-> end ( ) )
		{
			if ( inOneLine )
			{
				/* Quick and dirty single-line formatting */
				VString					vstrName;
				vstrName. AppendString ( *iterProperties );
				if ( vstrName. EndsWith ( CVSTR ( " - Object" ) ) )
					vstrName. Remove ( vstrName. GetLength ( ) - 8, 9 );
				else
				{
					VIndex				nSpace = vstrName. Find ( CVSTR ( " " ) );
					if ( nSpace > 0 )
					{
						VIndex			nSpace2 = vstrName. Find ( CVSTR ( " " ), nSpace + 3 );
						if ( nSpace2 > 0 )
							vstrName. Replace ( CVSTR ( "=" ), nSpace, nSpace2 - nSpace + 1 );
					}
				}

				outString. AppendString ( vstrName );
				outString. AppendCString ( "; " );
			}
			else
			{
				outString. AppendString ( *iterProperties );
				outString. AppendCString ( "\r\n" );
			}
			iterProperties++;
		}
	}
	/*if ( fFunctions != 0 )
	{
		;
	}*/
}


VJSDebugger::VJSDebugger ( )
{
	fWorker = 0;
	fEndPoint = 0;
	fCaller = 0;
	fSequenceGenerator = 0;
	fInnerListener = 0;
	fHasDoneHandShake = false;
	fNeedsAuthentication = false;
	fHasAuthenticated = false;
	fHasDebugUsers = true;
}

VJSDebugger::~VJSDebugger ( )
{
	Disconnect ( );
}

VError VJSDebugger::ShakeHands ( )
{
	if ( fEndPoint == 0 )
		ServerNetTools::ThrowNetError ( VE_SRVR_NULL_ENDPOINT );

	fEndPoint-> SetIsBlocking ( false );

	char		szchHandshake [ ] = "CrossfireHandshake\r\n";
	uLONG		nBufferLength = ::strlen ( szchHandshake );
	VError		vError = fEndPoint-> WriteExactly ( szchHandshake, nBufferLength, 30 * 1000 );
	if ( vError == VE_OK )
		vError = fEndPoint-> WriteExactly ( "\r\n", 2, 10 * 1000 );
	if ( vError == VE_OK )
	{
		vError = fEndPoint-> ReadExactly ( szchHandshake, nBufferLength, 30 * 1000 );
		if ( vError != VE_OK || ::strcmp ( szchHandshake, "CrossfireHandshake\r\n" ) != 0 )
			vError = VE_SRVR_CONNECTION_FAILED;
		else
		{
			// Let's read all available tools - a comma separated list that ends with CRLF
			uLONG			nLength = 0;
			VString			vstrCRLF ( "\r\n" );
			while ( vError == VE_OK && !( fServerTools. GetLength ( ) > 0 && fServerTools. EndsWith ( vstrCRLF ) ) )
			{
				nLength = nBufferLength;
				vError = fEndPoint-> Read ( szchHandshake, &nLength );
				if ( vError == VE_SRVR_RESOURCE_TEMPORARILY_UNAVAILABLE )
					vError = VE_OK;
				else if ( vError == VE_OK && nLength > 0 )
					fServerTools. AppendBlock ( szchHandshake, nLength, VTC_UTF_8 );
			}

			fHasDoneHandShake = ( vError == VE_OK );
			if ( fHasDoneHandShake )
			{
				if ( fServerTools. GetLength ( ) > 2 )
				{
					VIndex		nNonceStart = fServerTools. Find ( CVSTR ( "auth," ) );
					if ( nNonceStart > 0 )
					{
						nNonceStart += 5;

						VIndex		nNonceEnd = fServerTools. Find ( CVSTR ( "," ), nNonceStart);
						if ( nNonceEnd == 0 )
							nNonceEnd = fServerTools. GetLength ( ) - 2;

						xbox_assert ( nNonceEnd > 0 );

						fServerTools. GetSubString ( nNonceStart, nNonceEnd - nNonceStart + 1, fNonce );
						vError = VE_JSW_DEBUGGER_AUTHENTICATION_REQUIRED;	//		UNCOMMENT TO ENABLE CLIENT-SIDE DEBUGGER AUTHENTICATION
						fNeedsAuthentication = true;
					}

					if ( fServerTools. Find ( CVSTR ( ",no_debug_users" ) ) )
					{
						fHasDebugUsers = false;
						vError = VE_JSW_DEBUGGER_NO_DEBUGGER_USERS;
					}
					else
						fHasDebugUsers = true;
				}
			}
		}
	}

	::DebugMsg ( "DEBUGGER PERFORMED HANDSHAKE\r\n" );

	return vError;
}

VError VJSDebugger::Connect ( VString const & inDNSNameOrIP, short inPort, bool inIsSecured )
{
	VError					vError = VE_OK;
	fHasDoneHandShake = false;
	fServerTools. Clear ( );
	fNonce. Clear ( );
	fNeedsAuthentication = false;
	fHasAuthenticated = false;

	VTask::GetCurrent ( )->GetDebugContext ( ). DisableUI ( );
	for ( int i = 0; i < 2; i++ )
	{
		fEndPoint = VTCPEndPointFactory::CreateClientConnection (
														inDNSNameOrIP, inPort,
														inIsSecured /* isSSL */, true /* isBlocking */,
														30 * 1000 /* timeout */, 0 /* select IO is not needed */,
														vError );
		if ( fEndPoint != 0 )
			break;

		/* The "Debug" button is usually clicked right after the "Start Server" is clicked. It takes some time to start
		the server so I may need to wait a bit before the debugging server is available. */
		VTask::GetCurrent ( )-> Sleep ( 500 );
		VTask::GetCurrent ( )-> Yield ( );
	}
	VTask::GetCurrent ( )->GetDebugContext ( ). EnableUI ( );
	if ( vError == VE_OK && fEndPoint != 0 )
	{
		vError = ShakeHands ( );
		if ( vError == VE_OK || vError == VE_JSW_DEBUGGER_AUTHENTICATION_REQUIRED )
		{
			VTask::GetCurrent( )-> FlushErrors ( );
			fCaller = VTask::GetCurrent ( );
			fWorker = new VTask ( this, 0, eTaskStylePreemptive, Run );
			struct JSWDWorkerParameters*		workerParameters = new JSWDWorkerParameters ( fEndPoint, this, 0 );
			//fWorker-> SetKindData ( ( sLONG_PTR ) new std::pair<VTCPEndPoint*,IJSDebuggerListener*> ( fEndPoint, inListener ) );
			fWorker-> SetKindData ( ( sLONG_PTR ) workerParameters );
			fWorker-> Run ( );
		}
	}

	if ( vError != VE_OK && vError != VE_JSW_DEBUGGER_AUTHENTICATION_REQUIRED )
		ServerNetTools::ThrowNetError ( VE_SRVR_CONNECTION_FAILED );

	return vError;
}

bool VJSDebugger::IsConnected ( )
{
	bool	bIsConnected = ( fEndPoint != 0 && fHasDoneHandShake ) && ( !fNeedsAuthentication || fHasAuthenticated );
/*	if ( bIsConnected )
		XBOX::DebugMsg ( "DEBUGGER IS CONNECTED\r\n" );
	else
		XBOX::DebugMsg ( "DEBUGGER IS NOT CONNECTED\r\n" );
*/
	return bIsConnected;
}

VError VJSDebugger::Disconnect ( )
{
	VError					vError = VE_OK;

	fEndPointLock. Lock ( );

	if ( fEndPoint != 0 )
	{
		/* TODO: Some sort of synchronizatin mechanism is needed to make sure that this code is called
		only when the socket is actually blocked on reading. */
		if ( fWorker != 0 )
			fWorker-> Kill ( );

		fEndPoint-> ForceClose ( );
		fEndPoint-> Release ( );
		fEndPoint = 0;
	}

	fNonce. Clear ( );
	fServerTools. Clear ( );
	fHasDoneHandShake = false;

	fEndPointLock. Unlock ( );

	return vError;
}

VError VJSDebugger::Authenticate ( VString const & inUserName, VString const & inUserPassword )
{
	_DebugMsg ( CVSTR ( "Send AUTHENTICATE" ), CVSTR ( "NONE" ) );

	if ( fEndPoint == 0 || !fHasDoneHandShake )
	{
		xbox_assert ( false );

		return VE_OK;
	}

	VError					vError = VE_OK;

	VValueBag				vbagPacket;

	vbagPacket. SetString ( CrossfireKeys::type, CVSTR ( "request" ) );
	vbagPacket. SetString ( CrossfireKeys::command, CVSTR ( "authenticate" ) );
	vbagPacket. SetLong ( CrossfireKeys::seq, fSequenceGenerator );
	fSequenceGenerator++;

	VValueBag*				vbagArguments = new VValueBag ( );
	vbagArguments-> SetString ( CrossfireKeys::user, inUserName );
	vbagArguments-> SetString ( CrossfireKeys::ha1, inUserPassword );
	vbagPacket. AddElement( CrossfireKeys::arguments, vbagArguments );
	vbagArguments-> Release ( );

	VString					vstrPacket;
	vError = vbagPacket. GetJSONString ( vstrPacket, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );
	if ( vError == VE_OK )
	{
		vstrPacket. AppendCString ( "\r\n" );
		vError = SendPacket ( vstrPacket );
	}

	return vError;
}

VError VJSDebugger::Continue ( XBOX::VString const & inContext )
{
	_DebugMsg ( CVSTR ( "Send CONTINUE" ), inContext );

	if ( fEndPoint == 0 )
	{
		xbox_assert ( false );

		return VE_OK;
	}

	VError		vError = VE_OK;

	VValueBag				vbagPacket;

	vbagPacket. SetString ( CrossfireKeys::type, CVSTR ( "request" ) );
	vbagPacket. SetString ( CrossfireKeys::command, CVSTR ( "continue" ) );
	vbagPacket. SetString ( CrossfireKeys::context_id, inContext );
	vbagPacket. SetLong ( CrossfireKeys::seq, fSequenceGenerator );
	fSequenceGenerator++;

	VString					vstrPacket;
	vError = vbagPacket. GetJSONString ( vstrPacket, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );
	if ( vError == VE_OK )
	{
		vstrPacket. AppendCString ( "\r\n" );
		vError = SendPacket ( vstrPacket );
	}

	return vError;
}

XBOX::VError VJSDebugger::StepOver ( VString const & inContext )
{
	_DebugMsg ( CVSTR ( "Send STEP OVER" ), inContext );

	if ( fEndPoint == 0 )
	{
		xbox_assert ( false );

		return VE_OK;
	}

	VError		vError = VE_OK;

	VValueBag				vbagPacket;

	vbagPacket. SetString ( CrossfireKeys::type, CVSTR ( "request" ) );
	vbagPacket. SetString ( CrossfireKeys::command, CVSTR ( "stepover" ) );
	vbagPacket. SetString ( CrossfireKeys::context_id, inContext );
	vbagPacket. SetLong ( CrossfireKeys::seq, fSequenceGenerator );
	fSequenceGenerator++;

	VString					vstrPacket;
	vError = vbagPacket. GetJSONString ( vstrPacket, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );
	if ( vError == VE_OK )
	{
		vstrPacket. AppendCString ( "\r\n" );
		vError = SendPacket ( vstrPacket );
	}

//GetCurrentContexts ( );

	return vError;
}

XBOX::VError VJSDebugger::StepIn ( XBOX::VString const & inContext )
{
	_DebugMsg ( CVSTR ( "Send STEP IN" ), inContext );

	if ( fEndPoint == 0 )
	{
		xbox_assert ( false );

		return VE_OK;
	}

	VError		vError = VE_OK;

	VValueBag				vbagPacket;

	vbagPacket. SetString ( CrossfireKeys::type, CVSTR ( "request" ) );
	vbagPacket. SetString ( CrossfireKeys::command, CVSTR ( "stepin" ) );
	vbagPacket. SetString ( CrossfireKeys::context_id, inContext );
	vbagPacket. SetLong ( CrossfireKeys::seq, fSequenceGenerator );
	fSequenceGenerator++;

	VString					vstrPacket;
	vError = vbagPacket. GetJSONString ( vstrPacket, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );
	if ( vError == VE_OK )
	{
		vstrPacket. AppendCString ( "\r\n" );
		vError = SendPacket ( vstrPacket );
	}

	return vError;
}

XBOX::VError VJSDebugger::StepOut ( XBOX::VString const & inContext )
{
	_DebugMsg ( CVSTR ( "Send STEP OUT" ), inContext );

	if ( fEndPoint == 0 )
	{
		xbox_assert ( false );

		return VE_OK;
	}

	VError		vError = VE_OK;

	VValueBag				vbagPacket;

	vbagPacket. SetString ( CrossfireKeys::type, CVSTR ( "request" ) );
	vbagPacket. SetString ( CrossfireKeys::command, CVSTR ( "stepout" ) );
	vbagPacket. SetString ( CrossfireKeys::context_id, inContext );
	vbagPacket. SetLong ( CrossfireKeys::seq, fSequenceGenerator );
	fSequenceGenerator++;

	VString					vstrPacket;
	vError = vbagPacket. GetJSONString ( vstrPacket, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );
	if ( vError == VE_OK )
	{
		vstrPacket. AppendCString ( "\r\n" );
		vError = SendPacket ( vstrPacket );
	}

	return vError;
}

XBOX::VError VJSDebugger::AbortScript ( XBOX::VString const & inContext )
{
	_DebugMsg ( CVSTR ( "Send ABORT" ), inContext );

	if ( fEndPoint == 0 )
	{
		xbox_assert ( false );

		return VE_OK;
	}

	VError		vError = VE_OK;

	VValueBag				vbagPacket;

	vbagPacket. SetString ( CrossfireKeys::type, CVSTR ( "request" ) );
	vbagPacket. SetString ( CrossfireKeys::command, CVSTR ( "abort" ) );
	vbagPacket. SetString ( CrossfireKeys::context_id, inContext );
	vbagPacket. SetLong ( CrossfireKeys::seq, fSequenceGenerator );
	fSequenceGenerator++;

	VString					vstrPacket;
	vError = vbagPacket. GetJSONString ( vstrPacket, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );
	if ( vError == VE_OK )
	{
		vstrPacket. AppendCString ( "\r\n" );
		vError = SendPacket ( vstrPacket );
	}

	return vError;
}

XBOX::VError VJSDebugger::Suspend ( XBOX::VString const & inContext )
{
	_DebugMsg ( CVSTR ( "Send SUSPEND" ), inContext );

	if ( fEndPoint == 0 )
	{
		xbox_assert ( false );

		return VE_OK;
	}

	VError					vError = VE_OK;

	VValueBag				vbagPacket;

	vbagPacket. SetString ( CrossfireKeys::type, CVSTR ( "request" ) );
	vbagPacket. SetString ( CrossfireKeys::command, CVSTR ( "suspend" ) );
	vbagPacket. SetString ( CrossfireKeys::context_id, inContext );
	vbagPacket. SetLong ( CrossfireKeys::seq, fSequenceGenerator );
	fSequenceGenerator++;

	VString					vstrPacket;
	vError = vbagPacket. GetJSONString ( vstrPacket, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );
	if ( vError == VE_OK )
	{
		vstrPacket. AppendCString ( "\r\n" );
		vError = SendPacket ( vstrPacket );
	}

	return vError;
}

XBOX::VError VJSDebugger::GetCallStack ( XBOX::VString const & inContext, uLONG& outSequenceID, XBOX::VectorOfVString& outFunctions )
{
	_DebugMsg ( CVSTR ( "Send GET CALLSTACK" ), inContext );

	if ( fEndPoint == 0 )
	{
		xbox_assert ( false );

		return VE_OK;
	}

	VError		vError = VE_OK;

	VValueBag				vbagPacket;

	vbagPacket. SetString ( CrossfireKeys::type, CVSTR ( "request" ) );
	vbagPacket. SetString ( CrossfireKeys::command, CVSTR ( "backtrace" ) );
	vbagPacket. SetString ( CrossfireKeys::context_id, inContext );
	vbagPacket. SetLong ( CrossfireKeys::seq, fSequenceGenerator );
	outSequenceID = fSequenceGenerator;
	fSequenceGenerator++;

	VValueBag*				vbagArguments = new VValueBag ( );
	vbagArguments-> SetLong ( CrossfireKeys::fromFrame, 0 );
	vbagArguments-> SetBoolean ( CrossfireKeys::includeScopes, false );
	vbagPacket. AddElement( CrossfireKeys::arguments, vbagArguments );
	vbagArguments-> Release ( );

	VString					vstrPacket;
	vError = vbagPacket. GetJSONString ( vstrPacket, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );
	if ( vError == VE_OK )
	{
		vstrPacket. AppendCString ( "\r\n" );
		vError = SendPacket ( vstrPacket );
	}

/*	fEndPointLock. Lock ( );
		vError = fEndPoint-> WriteExactly ( ( void* ) "cstack\r\n", 8 );
	fEndPointLock. Unlock ( );
	if ( vError != VE_OK )
		return vError;

	char		szchHead [ 6 ];
	fEndPointLock. Lock ( );
		vError = fEndPoint-> ReadExactly ( szchHead, 6 );
	fEndPointLock. Unlock ( );
	if ( vError != VE_OK )
		return vError;

	VSize						nLength = ::atol ( szchHead );
	char*						szchCallStack = new char [ nLength ];
	fEndPointLock. Lock ( );
		vError = fEndPoint-> ReadExactly ( szchCallStack, nLength );
	fEndPointLock. Unlock ( );
	VString						vstrCallStack;
	if ( vError == VE_OK )
		vstrCallStack. AppendBlock ( szchCallStack, nLength, VTC_US_ASCII ); // Will do for now until the proper protocol is implemented

	delete [ ] szchCallStack;
	if ( vError != VE_OK )
		return vError;

	vstrCallStack. GetSubStrings ( CVSTR ( "\r\n" ), outFunctions, false, true );
	*/
	outFunctions. push_back ( CVSTR ( "<global_scope> - line 1" ) );

	return VE_OK;
}

VJSWDebuggerValue* VJSDebugger::ReadValue ( XBOX::VError& outError )
{
	if ( fEndPoint == 0 )
	{
		xbox_assert ( false );

		return 0;
	}

	VJSWDebuggerValue*		valResult = 0;

	char		szchHead [ 9 ];
	fEndPointLock. Lock ( );
		outError = fEndPoint-> ReadExactly ( szchHead, 8 );
	fEndPointLock. Unlock ( );
	if ( outError != VE_OK )
		return 0;

	szchHead [ 8 ] = '\0';
	if ( ::strcmp ( szchHead, "    Obje" ) == 0 )
	{
		fEndPointLock. Lock ( );
			outError = fEndPoint-> ReadExactly ( szchHead, 4 );
		fEndPointLock. Unlock ( );
		if ( outError != VE_OK )
			return 0;

		char*		chTemp = szchHead;
		// Not safe, of course, but will be replaced by the proper protocol
		while ( true )
		{
			fEndPointLock. Lock ( );
				outError = fEndPoint-> ReadExactly ( chTemp, 1 );
			fEndPointLock. Unlock ( );
			if ( outError != VE_OK )
				return 0;

			if ( *chTemp == ')' )
			{
				*chTemp = '\0';

				break;
			}
			chTemp++;
		}

		VSize						nLength = ::atol ( szchHead ) - 1;
		char*						szchDescription = new char [ nLength ];
		fEndPointLock. Lock ( );
			outError = fEndPoint-> ReadExactly ( szchDescription, nLength );
		fEndPointLock. Unlock ( );
		VString						vstrDescription;
		if ( outError == VE_OK )
			vstrDescription. AppendBlock ( szchDescription, nLength, VTC_US_ASCII ); // Will do for now until the proper protocol is implemented

		delete [ ] szchDescription;
		if ( outError != VE_OK )
			return 0;
		
		VectorOfVString				vctrDescription;
		vstrDescription. GetSubStrings ( CVSTR ( "\r\n" ), vctrDescription, false, true );

		VString						vstrInstanceOf = *( vctrDescription. begin ( ) );
		VIndex						nBeginClassName = vstrInstanceOf. FindUniChar ( CHAR_QUOTATION_MARK, vstrInstanceOf. GetLength ( ) - 1, true );
		VString						vstrClassName;
		vstrInstanceOf. GetSubString ( nBeginClassName + 1, vstrInstanceOf. GetLength ( ) - nBeginClassName - 1, vstrClassName );

		valResult = new VJSWDebuggerValue ( vstrClassName );

		VectorOfVString::iterator	iter = vctrDescription. begin ( ) + 1;
		VIndex						nEnd;
		while ( iter != vctrDescription. end ( ) )
		{
			nEnd = ( *iter ). Find ( CVSTR ( " - static function - " ) );
			if ( nEnd > 0 )
			{
				VString				vstrName;
				( *iter ). GetSubString ( 1, nEnd - 1, vstrName );
				valResult-> AddFunction ( vstrName );
			}
			else
			{
				nEnd = ( *iter ). Find ( CVSTR ( " " ) );
				if ( nEnd > 0 )
				{
					VString				vstrName;
					/*( *iter ). GetSubString ( 1, nEnd - 1, vstrName );
					valResult-> AddProperty ( vstrName );*/

					valResult-> AddProperty ( *iter );
				}
			}

			iter++;
		}
	}
	else
	{
		VString		vstrLine;
		outError = ReadLine ( vstrLine );
		if ( outError != VE_OK )
			return 0;

		valResult = 0;
		if ( ::strcmp ( szchHead, "    Numb" ) == 0 )
		{
			valResult = new VJSWDebuggerValue ( "Number" );
			vstrLine. Remove ( 1, 3 );
			vstrLine. Remove ( vstrLine. GetLength ( ) - 1, 2 );
			valResult-> SetReal ( vstrLine. GetReal ( ) );
		}
		else if ( ::strcmp ( szchHead, "    Stri" ) == 0 )
		{
			valResult = new VJSWDebuggerValue ( "String" );
			vstrLine. Remove ( 1, 4 );
			vstrLine. Remove ( vstrLine. GetLength ( ) - 2, 3 );
			valResult-> SetString ( vstrLine );
		}
		else if ( ::strcmp ( szchHead, "    Unde" ) == 0 )
		{
			valResult = new VJSWDebuggerValue ( "Undefined" );
			valResult-> MakeUndefined ( );
		}
		else if ( ::strcmp ( szchHead, "    Null" ) == 0 )
		{
			valResult = new VJSWDebuggerValue ( "Null" );
			valResult-> MakeNull ( );
		}
		else if ( ::strcmp ( szchHead, "    Bool" ) == 0 )
		{
			valResult = new VJSWDebuggerValue ( "Boolean" );
			vstrLine. Remove ( 1, 4 );
			vstrLine. Remove ( vstrLine. GetLength ( ) - 1, 2 );
			valResult-> SetBoolean ( vstrLine. GetBoolean ( ) );
		}
		else if ( ::strcmp ( szchHead, "    Unkn" ) == 0 )
		{
			valResult = new VJSWDebuggerValue ( "Unknown" );
			valResult-> MakeUnknown ( );
		}
		else
		{
			outError = VE_JSW_DEBUGGER_UNKNOWN_VALUE_TYPE;
			valResult = 0;
		}
	}

	return valResult;
}

VError VJSDebugger::SendPacket ( const VString & inBody )
{
	VIndex			nMessageLength = inBody. GetLength ( ) * 3;
	char*			szchMessage = new char [ nMessageLength ];
	nMessageLength = inBody. ToBlock ( szchMessage, nMessageLength, VTC_UTF_8, false, false );

	VString			vstrHeader;
	vstrHeader. AppendCString ( "Content-Length:" );
	vstrHeader. AppendLong ( nMessageLength );
	vstrHeader. AppendCString ( "\r\n" );
	char*			szchHeader = new char [ vstrHeader. GetLength ( ) ];
	VIndex			nHeaderLength = vstrHeader. ToBlock ( szchHeader, vstrHeader. GetLength ( ), VTC_UTF_8, false, false );

	fEndPointLock. Lock ( );

		VError		vError = fEndPoint-> WriteExactly ( szchHeader, nHeaderLength );

		delete [ ] szchHeader;
		xbox_assert ( vError == VE_OK );

		if ( vError == VE_OK )
		{
			vError = fEndPoint-> WriteExactly ( szchMessage, nMessageLength );
			xbox_assert ( vError == VE_OK );
		}
		delete [ ] szchMessage;

		if ( vError != VE_OK )
			Disconnect ( );

	fEndPointLock. Unlock ( );

	return vError;
}

XBOX::VError VJSDebugger::Evaluate ( XBOX::VString const & inContext, XBOX::VString const & inExpression, XBOX::VString const & inCallbackID )
{
	_DebugMsg ( CVSTR ( "Send EVALUATE" ), inContext );

	if ( fEndPoint == 0 )
	{
		xbox_assert ( false );
		return VE_JSW_DEBUGGER_NOT_CONNECTED;
	}

	XBOX::VError			vError = VE_OK;

	VValueBag				vbagPacket;

	vbagPacket. SetString ( CrossfireKeys::type, CVSTR ( "request" ) );
	vbagPacket. SetString ( CrossfireKeys::command, CVSTR ( "evaluate" ) );
	vbagPacket. SetString ( CrossfireKeys::context_id, inContext );
	vbagPacket. SetLong ( CrossfireKeys::seq, inCallbackID. GetLong ( ) );
	fSequenceGenerator++;

	VValueBag*				vbagArguments = new VValueBag ( );
	vbagArguments-> SetString ( CrossfireKeys::expression, inExpression );
	vbagPacket. AddElement( CrossfireKeys::arguments, vbagArguments );
	vbagArguments-> Release ( );

	VString					vstrPacket;
	vError = vbagPacket. GetJSONString ( vstrPacket, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );
	if ( vError == VE_OK )
	{
		vstrPacket. AppendCString ( "\r\n" );
		vError = SendPacket ( vstrPacket );
	}

	return vError;
}

VError VJSDebugger::Lookup ( VString const & inObjectRef, VString const & inContext, VString const & inCallbackID )
{
	_DebugMsg ( CVSTR ( "Send LOOKUP" ), inContext );

	if ( fEndPoint == 0 )
	{
		xbox_assert ( false );

		return VE_JSW_DEBUGGER_NOT_CONNECTED;
	}

	VError					vError = VE_OK;

	VValueBag				vbagPacket;

	vbagPacket. SetString ( CrossfireKeys::type, CVSTR ( "request" ) );
	vbagPacket. SetString ( CrossfireKeys::command, CVSTR ( "lookup" ) );
	vbagPacket. SetString ( CrossfireKeys::context_id, inContext );
	vbagPacket. SetLong ( CrossfireKeys::seq, inCallbackID. GetLong ( ) );
	fSequenceGenerator++;

	VValueBag*				vbagArguments = new VValueBag ( );
	vbagArguments-> SetLong ( CrossfireKeys::handle, inObjectRef. GetLong ( ) );
	vbagPacket. AddElement( CrossfireKeys::arguments, vbagArguments );
	vbagArguments-> Release ( );

	VString					vstrPacket;
	vError = vbagPacket. GetJSONString ( vstrPacket, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );
	if ( vError == VE_OK )
	{
		vstrPacket. AppendCString ( "\r\n" );
		vError = SendPacket ( vstrPacket );
	}

	return vError;
}

VError VJSDebugger::GetScope ( VString const & inContext, uLONG inFrame, uLONG& outSequenceID )
{
	_DebugMsg ( CVSTR ( "Send GET SCOPE" ), inContext );

	if ( fEndPoint == 0 )
	{
		xbox_assert ( false );

		return VE_JSW_DEBUGGER_NOT_CONNECTED;
	}

	VError					vError = VE_OK;

	VValueBag				vbagPacket;

	vbagPacket. SetString ( CrossfireKeys::type, CVSTR ( "request" ) );
	vbagPacket. SetString ( CrossfireKeys::command, CVSTR ( "scope" ) );
	vbagPacket. SetString ( CrossfireKeys::context_id, inContext );
	vbagPacket. SetLong ( CrossfireKeys::seq, fSequenceGenerator );
	outSequenceID = fSequenceGenerator;
	fSequenceGenerator++;

	VValueBag*				vbagArguments = new VValueBag ( );
	vbagArguments-> SetLong ( CrossfireKeys::number, 0 ); // Local scope
	vbagArguments-> SetLong ( CrossfireKeys::frameNumber, inFrame );
	vbagPacket. AddElement( CrossfireKeys::arguments, vbagArguments );
	vbagArguments-> Release ( );

	VString					vstrPacket;
	vError = vbagPacket. GetJSONString ( vstrPacket, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );
	if ( vError == VE_OK )
	{
		vstrPacket. AppendCString ( "\r\n" );
		vError = SendPacket ( vstrPacket );
	}

	return vError;
}

XBOX::VError VJSDebugger::GetScopes ( XBOX::VString const & inContext, uLONG inFrame, uLONG& outSequenceID )
{
	_DebugMsg ( CVSTR ( "Send GET SCOPES" ), inContext );

	if ( fEndPoint == 0 )
	{
		xbox_assert ( false );

		return VE_JSW_DEBUGGER_NOT_CONNECTED;
	}

	VError					vError = VE_OK;

	VValueBag				vbagPacket;

	vbagPacket. SetString ( CrossfireKeys::type, CVSTR ( "request" ) );
	vbagPacket. SetString ( CrossfireKeys::command, CVSTR ( "scopes" ) );
	vbagPacket. SetString ( CrossfireKeys::context_id, inContext );
	vbagPacket. SetLong ( CrossfireKeys::seq, fSequenceGenerator );
	outSequenceID = fSequenceGenerator;
	fSequenceGenerator++;

	VValueBag*				vbagArguments = new VValueBag ( );
	vbagArguments-> SetLong ( CrossfireKeys::frameNumber, inFrame );
	vbagPacket. AddElement( CrossfireKeys::arguments, vbagArguments );
	vbagArguments-> Release ( );

	VString					vstrPacket;
	vError = vbagPacket. GetJSONString ( vstrPacket, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );
	if ( vError == VE_OK )
	{
		vstrPacket. AppendCString ( "\r\n" );
		vError = SendPacket ( vstrPacket );
	}

	return vError;
}

XBOX::VError VJSDebugger::GetCurrentContexts ( )
{
	_DebugMsg ( CVSTR ( "Send LIST CONTEXTS" ), CVSTR ( "NO ID" ) );

	if ( fEndPoint == 0 )
	{
		xbox_assert ( false );

		return VE_JSW_DEBUGGER_NOT_CONNECTED;
	}

	VError					vError = VE_OK;

	VValueBag				vbagPacket;

	vbagPacket. SetString ( CrossfireKeys::type, CVSTR ( "request" ) );
	vbagPacket. SetString ( CrossfireKeys::command, CVSTR ( "listcontexts" ) );
	vbagPacket. SetLong ( CrossfireKeys::seq, fSequenceGenerator );
	fSequenceGenerator++;

	VString					vstrPacket;
	vError = vbagPacket. GetJSONString ( vstrPacket, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );
	if ( vError == VE_OK )
	{
		vstrPacket. AppendCString ( "\r\n" );
		vError = SendPacket ( vstrPacket );
	}

	return vError;
}

XBOX::VError VJSDebugger::SetCallFrame ( uWORD inIndex )
{
	_DebugMsg ( CVSTR ( "Send SET CALL FRAME" ), CVSTR ( "NO ID" ) );

	if ( fEndPoint == 0 )
	{
		xbox_assert ( false );

		return VE_OK;
	}

	VError			vError = VE_OK;

	/* TODO:
		- May need to use a mutex to lock access to the end point if we implement support for 
			server-based events when debugger is stopped on a break point or execption
	*/
	VString			vstrCommand;
	vstrCommand. AppendCString ( "sframe " );
	vstrCommand. AppendLong ( inIndex );
	vstrCommand. AppendCString ( "\r\n" );
	VIndex			nBufferSize = vstrCommand. GetLength ( ) + 1;
	char*			szchCommand = new char [ nBufferSize ];
	vstrCommand. ToCString ( szchCommand, nBufferSize );
	fEndPointLock. Lock ( );
		vError = fEndPoint-> WriteExactly ( szchCommand, nBufferSize - 1 );
	fEndPointLock. Unlock ( );
	delete [ ] szchCommand;

	return vError;
}

VError VJSDebugger::GetSourceCode ( VString const & inContext, sLONG inFrame, VString& outCode )
{
	_DebugMsg ( CVSTR ( "Send GET SOURCE" ), inContext );

	if ( fEndPoint == 0 )
	{
		xbox_assert ( false );

		return VE_OK;
	}

	VError					vError = VE_OK;

	VValueBag				vbagPacket;

	vbagPacket. SetString ( CrossfireKeys::type, CVSTR ( "request" ) );
	vbagPacket. SetString ( CrossfireKeys::command, CVSTR ( "script" ) );
	vbagPacket. SetString ( CrossfireKeys::context_id, inContext );
	vbagPacket. SetLong ( CrossfireKeys::seq, fSequenceGenerator );
	fSequenceGenerator++;

	VValueBag*				vbagArguments = new VValueBag ( );
	vbagArguments-> SetBool ( CrossfireKeys::includeSource, true );
	vbagArguments-> SetLong ( CrossfireKeys::frameNumber, inFrame );
	vbagPacket. AddElement( CrossfireKeys::arguments, vbagArguments );
	vbagArguments-> Release ( );

	VString					vstrPacket;
	vError = vbagPacket. GetJSONString ( vstrPacket, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );
	if ( vError == VE_OK )
	{
		vstrPacket. AppendCString ( "\r\n" );
		vError = SendPacket ( vstrPacket );
	}

	if ( vError == VE_OK )
		outCode. AppendCString ( "Loading source..." );
	else
		outCode. AppendCString ( "Failed to load source." );

	return vError;
}


XBOX::VError VJSDebugger::SetBreakPoints ( XBOX::VString const & inRelativeFilePath, std::vector< sLONG >& inLineNumbers, std::vector<bool>& inDisabled )
{
	VError err = VE_OK;

	for ( sLONG i = 0; i < inLineNumbers.size(); i++ )
	{
		if ( ! inDisabled[ i ] )
			err = SetBreakPoint( inRelativeFilePath, inLineNumbers[ i ] );

		if ( err != VE_OK )
			break;
	}

	return err;
}



XBOX::VError VJSDebugger::SetBreakPoint ( XBOX::VString const & inRelativeFilePath, XBOX::VIndex inLineNumber )
{
	VString			vstrMsg ( "Send SET BREAK POINT on line " );
	vstrMsg. AppendLong ( inLineNumber );
	vstrMsg. AppendCString ( " in " );
	vstrMsg. AppendString ( inRelativeFilePath );
	_DebugMsg ( vstrMsg, CVSTR ( "NO ID" ) );

	if ( fEndPoint == 0 )
		return VE_OK;

	VError					vError = VE_OK;

	VValueBag				vbagPacket;

	vbagPacket. SetString ( CrossfireKeys::type, CVSTR ( "request" ) );
	vbagPacket. SetString ( CrossfireKeys::command, CVSTR ( "setbreakpoint" ) );
	vbagPacket. SetString ( CrossfireKeys::context_id, CVSTR ( "UNDEFINED" ) );
	vbagPacket. SetLong ( CrossfireKeys::seq, fSequenceGenerator );
	fSequenceGenerator++;

	VValueBag*				vbagArguments = new VValueBag ( );
	vbagArguments-> SetString ( CrossfireKeys::target, inRelativeFilePath );
	vbagArguments-> SetLong ( CrossfireKeys::line, inLineNumber - 1 );
	vbagPacket. AddElement( CrossfireKeys::arguments, vbagArguments );
	vbagArguments-> Release ( );

	VString					vstrPacket;
	vError = vbagPacket. GetJSONString ( vstrPacket, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );
	if ( vError == VE_OK )
	{
		vstrPacket. AppendCString ( "\r\n" );
		vError = SendPacket ( vstrPacket );
	}

	return vError;
}


XBOX::VError VJSDebugger::RemoveBreakPoints ( XBOX::VString const & inRelativeFilePath, std::vector< sLONG >& inLineNumbers, std::vector<bool>& inDisabled )
{
	VError err = VE_OK;

	for ( sLONG i = 0; i < inLineNumbers.size(); i++ )
	{
		if ( ! inDisabled[ i ] )
			err = RemoveBreakPoint( inRelativeFilePath, inLineNumbers[ i ] );

		if ( err != VE_OK )
			break;
	}

	return err;
}


XBOX::VError VJSDebugger::RemoveBreakPoint ( XBOX::VString const & inRelativeFilePath, XBOX::VIndex inLineNumber )
{
	VString			vstrMsg ( "Send REMOVE BREAK POINT on line " );
	vstrMsg. AppendLong ( inLineNumber );
	vstrMsg. AppendCString ( " in " );
	vstrMsg. AppendString ( inRelativeFilePath );
	_DebugMsg ( vstrMsg, CVSTR ( "NO ID" ) );

	if ( fEndPoint == 0 )
		return VE_OK;

	VError					vError = VE_OK;

	VValueBag				vbagPacket;

	vbagPacket. SetString ( CrossfireKeys::type, CVSTR ( "request" ) );
	vbagPacket. SetString ( CrossfireKeys::command, CVSTR ( "clearbreakpoint" ) );
	vbagPacket. SetString ( CrossfireKeys::context_id, CVSTR ( "UNDEFINED" ) );
	vbagPacket. SetLong ( CrossfireKeys::seq, fSequenceGenerator );
	fSequenceGenerator++;

	VValueBag*				vbagArguments = new VValueBag ( );
	vbagArguments-> SetString ( CrossfireKeys::target, inRelativeFilePath );
	vbagArguments-> SetLong ( CrossfireKeys::line, inLineNumber - 1 );
	vbagPacket. AddElement( CrossfireKeys::arguments, vbagArguments );
	vbagArguments-> Release ( );

	VString					vstrPacket;
	vError = vbagPacket. GetJSONString ( vstrPacket, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );
	if ( vError == VE_OK )
	{
		vstrPacket. AppendCString ( "\r\n" );
		vError = SendPacket ( vstrPacket );
	}

	return vError;
}

sLONG VJSDebugger::Run ( VTask* inTask )
{
	struct JSWDWorkerParameters*		parameters = ( struct JSWDWorkerParameters* ) inTask-> GetKindData ( );
	if ( parameters == 0 )
		return 0;

	inTask-> SetKindData ( 0 );

	VJSDebugger*						vDebugger = parameters-> fDebugger;
	vDebugger-> Retain ( );
	VTCPEndPoint*						tcpEndPoint = parameters-> fEndPoint;
	VSyncEvent*							syncEvent = parameters-> fSyncEvent;

	if ( tcpEndPoint == 0 )
		return 0;

	tcpEndPoint-> Retain ( );
	VError			vError = VE_OK;

	VString			vstrLine;
	while ( vError == VE_OK && inTask-> GetState ( ) != TS_DYING && inTask-> GetState ( ) != TS_DEAD )
	{
		vError = vDebugger-> ReadLine ( vstrLine );
		if ( vError == VE_OK )
		{
			vError = vDebugger-> HandleRemoteEvent ( vstrLine );
			if ( vError == VE_JSW_DEBUGGER_REMOTE_EVENT_IS_MESSAGE )
				vError = VE_OK;
		}
		else
			vDebugger-> Disconnect ( );

		//xbox_assert ( vError == VE_OK );
	}

	vDebugger-> Release ( );
	delete parameters;
	parameters = 0;

	/*if ( vError != VE_OK )
		jswDebugListener-> OnError ( vError );*/

	vError = tcpEndPoint-> Close ( );
	xbox_assert ( vError == VE_OK );

	tcpEndPoint-> Release ( );

	return 0;
}

VError VJSDebugger::ReadLine ( VString& outLine )
{
	outLine. Clear ( );

	VError			vError = VE_OK;
	char			szchBuffer [ 1 ];
	uLONG			nRead = 1;
	VString			vstrCRLF ( "\r\n" );
	while (	!( outLine. GetLength ( ) > 0 && outLine. EndsWith ( vstrCRLF ) ) &&
			vError == VE_OK &&
			VTask::GetCurrent ( )-> GetState ( ) != TS_DYING && VTask::GetCurrent ( )-> GetState ( ) != TS_DEAD )
	{
		nRead = 1;
		fEndPointLock. Lock ( );
			if ( fEndPoint != 0 )
				vError = fEndPoint-> Read ( szchBuffer, &nRead );
			else
				vError = VE_JSW_DEBUGGER_NOT_CONNECTED;
		fEndPointLock. Unlock ( );
		if ( vError == VE_SRVR_RESOURCE_TEMPORARILY_UNAVAILABLE )
			vError = VE_OK;
		if ( vError == VE_OK )
		{
			if ( nRead > 0 )
				outLine. AppendBlock ( szchBuffer, nRead, VTC_StdLib_char );
			else
				VTask::GetCurrent ( )-> Sleep ( 100 );
		}

		xbox_assert ( vError == VE_OK || fEndPoint == 0 );
	}

	return vError;
}

XBOX::VError VJSDebugger::GetException ( XBOX::VString& outMessage, XBOX::VString& outName, uLONG& outBeginOffset, uLONG& outEndOffset )
{
	_DebugMsg ( CVSTR ( "Send GET EXCEPTION" ), CVSTR ( "NO ID" ) );

	if ( fEndPoint == 0 )
	{
		xbox_assert ( false );

		return VE_OK;
	}

	VError				vError = VE_OK;
	VString				vstrBuffer;

	/* TODO:
		- May need to use a mutex to lock access to the end point if we implement support for 
			server-based events when debugger is stopped on a break point or execption
	*/
	fEndPointLock. Lock ( );
		vError = fEndPoint-> WriteExactly ( ( void *)"getexception\r\n", 14 );
	fEndPointLock. Unlock ( );
	if ( vError != VE_OK )
		return vError;

	char				szchBuffer [ 1024 ];
	uLONG				nReadActually;

	fEndPointLock. Lock ( );
		vError = fEndPoint-> ReadExactly ( szchBuffer, 6 );
	fEndPointLock. Unlock ( );
	if ( vError != VE_OK )
		return vError;

	VSize				nSize = ::atol ( szchBuffer );
	VSize				nReadTotal = 0;
	while ( vError == VE_OK && nSize > 0 )
	{
		nReadActually = 1024;
		fEndPointLock. Lock ( );
			vError = fEndPoint-> Read ( szchBuffer, &nReadActually );
		fEndPointLock. Unlock ( );
		if ( vError == VE_SRVR_RESOURCE_TEMPORARILY_UNAVAILABLE )
			vError = VE_OK;
		if ( vError == VE_OK && nReadActually > 0 )
		{
			nSize -= nReadActually;
			vstrBuffer. AppendBlock ( szchBuffer, nReadActually, VTC_UTF_16 ); // Will do for now until the proper protocol is implemented
			nReadTotal += nReadActually;
		}
	}

	if ( vError == VE_OK )
	{
		VectorOfVString			vctrHeaders;
		vstrBuffer. GetSubStrings ( CVSTR ( "\r\n" ), vctrHeaders, false, true );
		uWORD					nSize = vctrHeaders. size ( );
		if ( nSize >= 4 )
		{
			for ( uWORD i = 0; i < nSize - 3; i++ )
				outMessage. AppendString ( vctrHeaders [ i ] + "\r\n" );

			outName. FromString ( vctrHeaders [ nSize - 3 ] );
			outBeginOffset = vctrHeaders [ nSize - 2 ]. GetLong ( );
			outEndOffset = vctrHeaders [ nSize - 1 ]. GetLong ( );
		}
	}

	return vError;
}

VError VJSDebugger::ReadCrossfireMessage ( uLONG inLength, VValueBag& outMessage )
{
	if ( fEndPoint == 0 )
	{
		xbox_assert ( false );

		return VE_SRVR_CONNECTION_BROKEN;
	}

	char*					szchMessage = new char [ inLength ];
	uLONG					nLength = inLength;

	fEndPointLock. Lock ( );
		VError					vError = fEndPoint-> ReadExactly ( szchMessage, nLength );
	fEndPointLock. Unlock ( );
	xbox_assert ( vError == VE_OK && nLength == inLength );

	if ( vError == VE_OK )
	{
		VString				vstrMessage ( szchMessage, nLength, VTC_UTF_8 );
		VJSONImporter		vImporter ( vstrMessage );
		vError = vImporter. JSONObjectToBag ( outMessage );
	}

	delete [ ] szchMessage;

	return vError;
}

XBOX::VError VJSDebugger::HandleAuthenticateResponse ( XBOX::VValueBag & inMessage )
{
	VError					vError = VE_OK;

	VString					vstrContextID;
	_GetContextID ( inMessage, vstrContextID );
	_DebugMsg ( CVSTR ( "Handle EVALUATE response" ), vstrContextID );

	bool					bHasAuthenticated = false;
	if ( inMessage. AttributeExists ( CrossfireKeys::success ) )
		inMessage. GetBool ( CrossfireKeys::success, bHasAuthenticated );

	sLONG					nError = 0;
	if ( inMessage. AttributeExists ( CrossfireKeys::error ) )
		inMessage. GetLong ( CrossfireKeys::error, nError );

	fHasAuthenticated = bHasAuthenticated;

	VJSWDebuggerValue*		valResult = new VJSWDebuggerValue ( CVSTR ( "Boolean" ) );
	valResult-> SetBoolean ( bHasAuthenticated );

	VString					vstrIP;
	vError = GetIP ( vstrIP );
	valResult-> SetJSON ( vstrIP );

	VJSDebuggerCallback*		vjsdCallback = new VJSDebuggerCallback (
																	VJSDebuggerCallback::JSWD_CC_ON_AUTHENTICATE,
																	valResult );
	vError = SendCallbackToListeners ( vjsdCallback );
	vjsdCallback-> Release ( );
	delete valResult;

	return vError;
}

XBOX::VError VJSDebugger::HandleEvaluateResponse ( XBOX::VValueBag & inMessage )
{
	VError					vError = VE_OK;

	VString					vstrContextID;
	_GetContextID ( inMessage, vstrContextID );
	_DebugMsg ( CVSTR ( "Handle EVALUATE response" ), vstrContextID );

	const VValueBag*		vbagBody = inMessage. RetainUniqueElement ( CrossfireKeys::body );
	if ( vbagBody == 0 )
	{
		xbox_assert ( false );

		return VE_INVALID_PARAMETER;
	}

	bool					bIsNullResult = !vbagBody-> AttributeExists ( CrossfireKeys::result ) && !vbagBody-> AttributeExists ( CrossfireKeys::type );
	vbagBody-> Release ( );

	inMessage. SetString ( CrossfireKeys::type, CVSTR ( "evaluated" ) );

	VString					vstrEvalJSON;
	vError = inMessage. GetJSONString ( vstrEvalJSON, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );
	if ( vError == VE_OK )
	{
		if ( bIsNullResult )
		{
			VIndex			nStart = vstrEvalJSON. Find ( CVSTR ( "\"context_id\"" ) );
			vstrEvalJSON. Insert ( CVSTR ( "\"result\":null,\n\t\t" ), nStart );
		}

		VJSWDebuggerValue*		valResult = new VJSWDebuggerValue ( CVSTR ( "" ) );
		valResult-> SetJSON ( vstrEvalJSON );
		VJSDebuggerCallback*		vjsdCallback = new VJSDebuggerCallback (
																		VJSDebuggerCallback::JSWD_CC_ON_EVALUATE_RESULT,
																		valResult );
		vError = SendCallbackToListeners ( vjsdCallback );
		vjsdCallback-> Release ( );
		delete valResult;
	}

	return vError;
}

XBOX::VError VJSDebugger::HandleLookupResponse ( XBOX::VValueBag & inMessage )
{
	VError					vError = VE_OK;

	VString					vstrContextID;
	_GetContextID ( inMessage, vstrContextID );
	_DebugMsg ( CVSTR ( "Handle LOOKUP response" ), vstrContextID );

	inMessage. SetString ( CrossfireKeys::type, CVSTR ( "objectpropertiesreceived" ) );

	VString					vstrLookupJSON;
	vError = inMessage. GetJSONString ( vstrLookupJSON, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );
	if ( vError == VE_OK )
	{
		VJSWDebuggerValue*		valResult = new VJSWDebuggerValue ( CVSTR ( "" ) );
		valResult-> SetJSON ( vstrLookupJSON );
		VJSDebuggerCallback*		vjsdCallback = new VJSDebuggerCallback (
																		VJSDebuggerCallback::JSWD_CC_ON_LOOKUP_RESULT,
																		valResult );
		vError = SendCallbackToListeners ( vjsdCallback );
		vjsdCallback-> Release ( );
		delete valResult;
	}

	return vError;
}

XBOX::VError VJSDebugger::HandleScopeResponse ( XBOX::VValueBag & inMessage )
{
	VError					vError = VE_OK;

	VString					vstrContextID;
	_GetContextID ( inMessage, vstrContextID );
	_DebugMsg ( CVSTR ( "Handle GET SCOPE response" ), vstrContextID );

	inMessage. SetString ( CrossfireKeys::type, CVSTR ( "scopereceived" ) );

	VString					vstrScopeJSON;
	vError = inMessage. GetJSONString ( vstrScopeJSON, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );
	if ( vError == VE_OK )
	{
		VJSWDebuggerValue*		valResult = new VJSWDebuggerValue ( CVSTR ( "" ) );
		valResult-> SetJSON ( vstrScopeJSON );
		VJSDebuggerCallback*		vjsdCallback = new VJSDebuggerCallback (
																		VJSDebuggerCallback::JSWD_CC_ON_SCOPE_RESULT,
																		valResult );
		vError = SendCallbackToListeners ( vjsdCallback );
		vjsdCallback-> Release ( );
		delete valResult;
	}

	return vError;
}

XBOX::VError VJSDebugger::HandleScopeChainResponse ( XBOX::VValueBag & inMessage )
{
	VError					vError = VE_OK;

	VString					vstrContextID;
	_GetContextID ( inMessage, vstrContextID );
	_DebugMsg ( CVSTR ( "Handle GET SCOPE CHAIN response" ), vstrContextID );

	inMessage. SetString ( CrossfireKeys::type, CVSTR ( "scopesreceived" ) );

	VString					vstrScopesJSON;
	vError = inMessage. GetJSONString ( vstrScopesJSON, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );
	if ( vError == VE_OK )
	{
		VJSWDebuggerValue*		valResult = new VJSWDebuggerValue ( CVSTR ( "" ) );
		valResult-> SetJSON ( vstrScopesJSON );
		VJSDebuggerCallback*		vjsdCallback = new VJSDebuggerCallback (
																		VJSDebuggerCallback::JSWD_CC_ON_SCOPE_CHAIN_RESULT,
																		valResult );
		vError = SendCallbackToListeners ( vjsdCallback );
		vjsdCallback-> Release ( );
		delete valResult;
	}

	return vError;
}

XBOX::VError VJSDebugger::HandleScriptResponse ( XBOX::VValueBag & inMessage )
{
	VError					vError = VE_OK;

	VString					vstrContextID;
	_GetContextID ( inMessage, vstrContextID );
	_DebugMsg ( CVSTR ( "Handle GET SCRIPT response" ), vstrContextID );

	const VValueBag*		vbagBody = inMessage. RetainUniqueElement ( CrossfireKeys::body );
	if ( vbagBody == 0 )
	{
		xbox_assert ( false );

		return VE_INVALID_PARAMETER;
	}

	const VValueBag*		vbagScript = vbagBody-> RetainUniqueElement ( CrossfireKeys::script );
	if ( vbagScript == 0 )
	{
		xbox_assert ( false );
		vError = VE_INVALID_PARAMETER;
	}
	else
	{
		VString				vstrSource;
		//vbagScript-> GetString ( CrossfireKeys::source, vstrSource );
		vError = inMessage. GetJSONString ( vstrSource, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );

		VJSWDebuggerValue*		valResult = new VJSWDebuggerValue ( CVSTR ( "" ) );
		valResult-> SetJSON ( vstrSource );
		VJSDebuggerCallback*	vjsdCallback = new VJSDebuggerCallback (
																		VJSDebuggerCallback::JSWD_CC_ON_SCRIPT_RESULT,
																		valResult );
		vError = SendCallbackToListeners ( vjsdCallback );
		vjsdCallback-> Release ( );
		delete valResult;

		vbagScript-> Release ( );
	}

	vbagBody-> Release ( );

	return vError;
}

XBOX::VError VJSDebugger::HandleCallStackResponse ( XBOX::VValueBag & inMessage )
{
	VError					vError = VE_OK;

	VString					vstrContextID;
	_GetContextID ( inMessage, vstrContextID );
	_DebugMsg ( CVSTR ( "Handle GET CALL STACK response" ), vstrContextID );

	inMessage. SetString ( CrossfireKeys::type, CVSTR ( "stacksloaded" ) );

	VValueBag*				vbagBody = inMessage. RetainUniqueElement ( CrossfireKeys::body );
	if ( !testAssert ( vbagBody != 0 ) )
		return VE_INVALID_PARAMETER;

	VValueBag*				vbagFrames = vbagBody-> RetainUniqueElement ( CrossfireKeys::frames );
	if ( !testAssert ( vbagFrames != 0 ) )
		vError = VE_INVALID_PARAMETER;
	else
	{
		VString					vstrRootFolder;
		fRootFolder. GetPath ( vstrRootFolder );
		VString					vstrRoot;
		vstrRootFolder. GetSubString ( 1, vstrRootFolder. FindUniChar ( FOLDER_SEPARATOR ) - 1, vstrRoot );

		sLONG					nIndex = 0;
		bool					bResult = false;
		while ( true )
		{
			VString				vstrIndex;
			vstrIndex. AppendLong ( nIndex );
			VValueBag*			vbagFrame = vbagFrames-> RetainUniqueElement ( vstrIndex );
			if ( vbagFrame == 0 )
				break;

			VString				vstrRelative;
			bResult = vbagFrame-> GetString ( CrossfireKeys::relativePath, vstrRelative );
			if ( testAssert ( bResult ) )
			{
				if ( vstrRelative. GetLength ( ) == 0 )
					vbagFrame-> SetString ( CrossfireKeys::fullPath, CVSTR ( "" ) );
				else
				{
					if ( vstrRelative. GetUniChar ( 1 ) == FOLDER_SEPARATOR )
						vstrRelative. Remove ( 1, 1 );

					VFilePath		vfFullPath;
					vfFullPath. FromRelativePath ( fRootFolder, vstrRelative, FPS_POSIX );
					VString			vstrFullPath;
					vfFullPath. GetPath ( vstrFullPath );
					if ( !vstrFullPath. BeginsWith ( vstrRoot ) )
						vstrFullPath = vstrRoot + vstrFullPath;
					vbagFrame-> SetString ( CrossfireKeys::fullPath, vstrFullPath );
				}
			}

			vbagFrame-> Release ( );
			nIndex++;
		}

		vbagFrames-> Release ( );
	}

	vbagBody-> Release ( );

	if ( vError == VE_OK )
	{
		VString					vstrCallStackJSON;
		vError = inMessage. GetJSONString ( vstrCallStackJSON, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );
		if ( vError == VE_OK )
		{
			VJSWDebuggerValue*		valResult = new VJSWDebuggerValue ( CVSTR ( "" ) );
			valResult-> SetJSON ( vstrCallStackJSON );
			VJSDebuggerCallback*		vjsdCallback = new VJSDebuggerCallback (
																			VJSDebuggerCallback::JSWD_CC_ON_CALL_STACK_RESULT,
																			valResult );
			vError = SendCallbackToListeners ( vjsdCallback );
			vjsdCallback-> Release ( );
			delete valResult;
		}
	}

	return vError;
}

XBOX::VError VJSDebugger::HandleListContextsResponse ( XBOX::VValueBag & inMessage )
{
	VError							vError = VE_OK;

	VString					vstrContextID;
	_GetContextID ( inMessage, vstrContextID );
	_DebugMsg ( CVSTR ( "Handle LIST CONTEXTS response" ), vstrContextID );

	inMessage. SetString ( CrossfireKeys::type, CVSTR ( "contextslisted" ) );

	VString							vstrContextsListJSON;
	vError = inMessage. GetJSONString ( vstrContextsListJSON, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );
	if ( vError == VE_OK )
	{
		VJSWDebuggerValue*			valResult = new VJSWDebuggerValue ( CVSTR ( "" ) );
		valResult-> SetJSON ( vstrContextsListJSON );
		VJSDebuggerCallback*		vjsdCallback = new VJSDebuggerCallback (
																		VJSDebuggerCallback::JSWD_CC_ON_LIST_CONTEXTS,
																		valResult );
		vError = SendCallbackToListeners ( vjsdCallback );
		vjsdCallback-> Release ( );
		delete valResult;
	}

	return vError;
}

XBOX::VError VJSDebugger::HandleRemoteBreakpoint ( const XBOX::VValueBag & inMessage )
{
	VError					vError = VE_OK;

	/*VString					vstrType;
	inMessage. GetString ( CrossfireKeys::type, vstrType );
	if ( !vstrType. EqualToString ( CVSTR ( "event" ) ) )
	{
		xbox_assert ( false );

		return VE_INVALID_PARAMETER;
	}

	VString					vstrEvent;
	inMessage. GetString ( CrossfireKeys::event, vstrEvent );
	if ( !vstrEvent. EqualToString ( CVSTR ( "onBreak" ) ) )
	{
		xbox_assert ( false );

		return VE_INVALID_PARAMETER;
	}*/

	VString					vstrContextID;
	inMessage. GetString ( CrossfireKeys::context_id, vstrContextID );

	_DebugMsg ( CVSTR ( "Handle BREAK POINT" ), vstrContextID );

	const VValueBag*		vbagData = inMessage. RetainUniqueElement ( CrossfireKeys::data );
	if ( vbagData == 0 )
	{
		xbox_assert ( false );

		return VE_INVALID_PARAMETER;
	}

	VString					vstrURL;
	vbagData-> GetString ( CrossfireKeys::url, vstrURL );
	VString					vstrFunction;
	vbagData-> GetString ( CrossfireKeys::function, vstrFunction );
	uLONG					nLine;
	vbagData-> GetLong ( CrossfireKeys::line, nLine );

	vbagData-> Release ( );

	VJSDebuggerMessage*		vjsdMessage = new VJSDebuggerMessage (
																vstrContextID,
																VJSDebuggerMessage::JSWD_CM_ON_BREAK_POINT,
																CVSTR ( "Break point" ),
																CVSTR ( "Noname" ),
																vstrURL,
																vstrFunction,
																nLine,
																0, 0 );
	vError = SendMessageToListeners ( vjsdMessage );
	vjsdMessage-> Release ( );

	return vError;
}

VError VJSDebugger::HandleRemoteException ( const VValueBag & inMessage )
{
	/* For the moment, this is a copy&paste of the ::HandleRemoteBreakpoint.
	The proper implementation will be put in place once the the server 
	implements onContextError json message. */

	VError					vError = VE_OK;

	/*VString					vstrType;
	inMessage. GetString ( CrossfireKeys::type, vstrType );
	if ( !vstrType. EqualToString ( CVSTR ( "event" ) ) )
	{
		xbox_assert ( false );

		return VE_INVALID_PARAMETER;
	}

	VString					vstrEvent;
	inMessage. GetString ( CrossfireKeys::event, vstrEvent );
	if ( !vstrEvent. EqualToString ( CVSTR ( "onException" ) ) )
	{
		xbox_assert ( false );

		return VE_INVALID_PARAMETER;
	}*/

	VString					vstrContextID;
	inMessage. GetString ( CrossfireKeys::context_id, vstrContextID );

	_DebugMsg ( CVSTR ( "Handle EXCEPTION" ), vstrContextID );

	const VValueBag*		vbagData = inMessage. RetainUniqueElement ( CrossfireKeys::data );
	if ( vbagData == 0 )
	{
		xbox_assert ( false );

		return VE_INVALID_PARAMETER;
	}

	VString					vstrURL;
	vbagData-> GetString ( CrossfireKeys::url, vstrURL );
	VString					vstrFunction;
	vbagData-> GetString ( CrossfireKeys::function, vstrFunction );
	uLONG					nLine;
	vbagData-> GetLong ( CrossfireKeys::line, nLine );
	sLONG					nExceptionHandle = -1;
	bool					bHasExceptionHandle = vbagData-> GetLong ( CrossfireKeys::handle, nExceptionHandle );

	vbagData-> Release ( );

	VString			vstrMessage;
	vbagData-> GetString ( CrossfireKeys::message, vstrMessage );
	VString			vstrName;
	vbagData-> GetString ( CrossfireKeys::name, vstrName );
	uLONG			nBeginOffset = 0;
	vbagData-> GetLong ( CrossfireKeys::beginOffset, nBeginOffset );
	uLONG			nEndOffset = 0;
	vbagData-> GetLong ( CrossfireKeys::endOffset, nEndOffset );
	//vError = GetException ( vstrMessage, vstrName, nBeginOffset, nEndOffset );
	if ( vError == VE_OK )
	{
		VJSDebuggerMessage*		vjsdMessage = new VJSDebuggerMessage (
																	vstrContextID,
																	VJSDebuggerMessage::JSWD_CM_ON_EXCEPTION,
																	vstrMessage,
																	vstrName,
																	vstrURL,
																	vstrFunction,
																	nLine,
																	nBeginOffset,
																	nEndOffset,
																	nExceptionHandle );
		vError = SendMessageToListeners ( vjsdMessage );
		vjsdMessage-> Release ( );
	}

	return vError;
}

VError VJSDebugger::HandleRemoteContextCreated ( VValueBag & inMessage )
{
	VError					vError = VE_OK;

	inMessage. SetString ( CrossfireKeys::event, CVSTR ( "contextcreated" ) );

	VString					vstrContextID;
	inMessage. GetString ( CrossfireKeys::context_id, vstrContextID );

	_DebugMsg ( CVSTR ( "Handle CONTEXT CREATED" ), vstrContextID );

	VString					vstrMessage;
	vError = inMessage. GetJSONString ( vstrMessage, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );

	if ( vError == VE_OK )
	{
		VJSDebuggerMessage*		vjsdMessage = new VJSDebuggerMessage (
																	vstrContextID,
																	VJSDebuggerMessage::JSWD_CM_ON_CONTEXT_CREATED,
																	vstrMessage,
																	CVSTR ( "" ),
																	CVSTR ( "" ),
																	CVSTR ( "" ),
																	0,
																	0,
																	0 );
		vError = SendMessageToListeners ( vjsdMessage );
		vjsdMessage-> Release ( );
	}

	return vError;
}

VError VJSDebugger::HandleRemoteContextDestroyed ( VValueBag & inMessage )
{
	VError					vError = VE_OK;

	inMessage. SetString ( CrossfireKeys::event, CVSTR ( "contextdestroyed" ) );
	inMessage. SetString ( CrossfireKeys::type, CVSTR ( "contextdestroyed" ) );

	VString					vstrContextID;
	inMessage. GetString ( CrossfireKeys::context_id, vstrContextID );

	_DebugMsg ( CVSTR ( "Handle CONTEXT DESTROYED" ), vstrContextID );

	VString					vstrMessage;
	vError = inMessage. GetJSONString ( vstrMessage, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );

	if ( vError == VE_OK )
	{
		VJSDebuggerMessage*		vjsdMessage = new VJSDebuggerMessage (
																	vstrContextID,
																	VJSDebuggerMessage::JSWD_CM_ON_CONTEXT_DESTROYED,
																	vstrMessage,
																	CVSTR ( "" ),
																	CVSTR ( "" ),
																	CVSTR ( "" ),
																	0,
																	0,
																	0 );
		vError = SendMessageToListeners ( vjsdMessage );
		vjsdMessage-> Release ( );
	}

	return vError;
}

VError VJSDebugger::HandleRemoteEvent ( const VString& inLine )
{
	VError			vError = VE_OK;

	if ( inLine. BeginsWith ( CVSTR ( "    Stopped" ) ) )
	{
		VString			vstrEvent;
		vstrEvent. FromString ( inLine );
		vstrEvent. Remove ( 1, 20 );

		VString			vstrLineNumber;
		VIndex			nFind = vstrEvent. Find ( CVSTR ( " " ) );
		vstrEvent. GetSubString ( 1, nFind - 1, vstrLineNumber );
		vstrEvent. Remove ( 1, nFind + 13 );

		VString			vstrFunctionName;
		nFind = vstrEvent. Find ( CVSTR ( "\"" ) );
		vstrEvent. GetSubString ( 1, nFind - 1, vstrFunctionName );
		vstrEvent. Remove ( 1, nFind + 10 );

		VString			vstrFilePath;
		vstrEvent. GetSubString ( 1, vstrEvent. GetLength ( ) - 3, vstrFilePath );

		VJSDebuggerMessage*		vjsdMessage = new VJSDebuggerMessage (
																	CVSTR ( "No ID" ),
																	VJSDebuggerMessage::JSWD_CM_ON_BREAK_POINT,
																	CVSTR ( "Break point" ),
																	CVSTR ( "Noname" ),
																	vstrFilePath,
																	vstrFunctionName,
																	vstrLineNumber. GetLong ( ),
																	0, 0 );
		vError = SendMessageToListeners ( vjsdMessage );
		vjsdMessage-> Release ( );
	}
	else if ( inLine. BeginsWith ( CVSTR ( "    Exception" ) ) )
	{
		VString			vstrEvent;
		vstrEvent. FromString ( inLine );
		vstrEvent. Remove ( 1, 22 );

		VString			vstrLineNumber;
		VIndex			nFind = vstrEvent. Find ( CVSTR ( " " ) );
		vstrEvent. GetSubString ( 1, nFind - 1, vstrLineNumber );
		vstrEvent. Remove ( 1, nFind + 13 );

		VString			vstrFunctionName;
		nFind = vstrEvent. Find ( CVSTR ( "\"" ) );
		vstrEvent. GetSubString ( 1, nFind - 1, vstrFunctionName );
		vstrEvent. Remove ( 1, nFind + 10 );

		VString			vstrFilePath;
		vstrEvent. GetSubString ( 1, vstrEvent. GetLength ( ) - 3, vstrFilePath );

		VString			vstrMessage;
		VString			vstrName;
		uLONG			nBeginOffset = 0;
		uLONG			nEndOffset = 0;
		vError = GetException ( vstrMessage, vstrName, nBeginOffset, nEndOffset );
		if ( vError == VE_OK )
		{
			VJSDebuggerMessage*		vjsdMessage = new VJSDebuggerMessage (
																		CVSTR ( "No ID" ),
																		VJSDebuggerMessage::JSWD_CM_ON_EXCEPTION,
																		vstrMessage,
																		vstrName,
																		vstrFilePath,
																		vstrFunctionName,
																		vstrLineNumber. GetLong ( ),
																		nBeginOffset,
																		nEndOffset );
			vError = SendMessageToListeners ( vjsdMessage );
			vjsdMessage-> Release ( );
		}
	}
	else if ( inLine. BeginsWith ( CVSTR ( "    Execution finished" ) ) )
	{
		vError = VE_JSW_DEBUGGER_REMOTE_EVENT_IS_MESSAGE;
	}
	else if ( inLine. BeginsWith ( CVSTR ( "Evaluate result " ) ) )
	{
		VString					vstrID;
		inLine. GetSubString ( 17, inLine. GetLength ( ) - 16 - 2, vstrID );

		VJSWDebuggerValue*	valResult = ReadValue ( vError );
		xbox_assert ( vError == VE_OK );
		if ( vError == VE_OK )
		{
			valResult-> SetCallbackID ( vstrID );
			VJSDebuggerCallback*		vjsdCallback = new VJSDebuggerCallback (
																			VJSDebuggerCallback::JSWD_CC_ON_EVALUATE_RESULT,
																			valResult );
			vError = SendCallbackToListeners ( vjsdCallback );
			vjsdCallback-> Release ( );
		}
		delete valResult;
	}
	else if ( inLine. BeginsWith ( CVSTR ( "Content-Length:" ) ) )
	{
		VString					vstrLength;
		VIndex					nLengthStart = ::strlen ( "Content-Length:" ) + 1;
		inLine. GetSubString ( nLengthStart, inLine. GetLength ( ) - ( nLengthStart - 1 ) - 2, vstrLength );
		VValueBag				vbagMessage;
		vError = ReadCrossfireMessage ( vstrLength. GetLong ( ), vbagMessage );
		xbox_assert ( vError == VE_OK );
		if ( vError == VE_OK )
		{
			VString					vstrType;
			vbagMessage. GetString ( CrossfireKeys::type, vstrType );
			if ( vstrType. EqualToString ( CVSTR ( "event" ) ) )
			{
				VString				vstrEvent;
				vbagMessage. GetString ( CrossfireKeys::event, vstrEvent );
				if ( vstrEvent. EqualToString ( CVSTR ( "onBreak" ) ) )
					vError = HandleRemoteBreakpoint ( vbagMessage );
				else if ( vstrEvent. EqualToString ( CVSTR ( "onException" ) ) )
					vError = HandleRemoteException ( vbagMessage );
				else if ( vstrEvent. EqualToString ( CVSTR ( "onContextCreated" ) ) )
					vError = HandleRemoteContextCreated ( vbagMessage );
				else if ( vstrEvent. EqualToString ( CVSTR ( "onContextDestroyed" ) ) )
					vError = HandleRemoteContextDestroyed ( vbagMessage );
				else
				{
					xbox_assert ( false );
					vError = VE_UNIMPLEMENTED;
				}
			}
			else if ( vstrType. EqualToString ( CVSTR ( "response" ) ) )
			{
				VString				vstrCommand;
				vbagMessage. GetString ( CrossfireKeys::command, vstrCommand );
				if ( vstrCommand. EqualToString ( CVSTR ( "evaluate" ) ) )
					vError = HandleEvaluateResponse ( vbagMessage );
				else if ( vstrCommand. EqualToString ( CVSTR ( "lookup" ) ) )
					vError = HandleLookupResponse ( vbagMessage );
				else if ( vstrCommand. EqualToString ( CVSTR ( "scope" ) ) )
					vError = HandleScopeResponse ( vbagMessage );
				else if ( vstrCommand. EqualToString ( CVSTR ( "scopes" ) ) )
					vError = HandleScopeChainResponse ( vbagMessage );
				else if ( vstrCommand. EqualToString ( CVSTR ( "script" ) ) )
					vError = HandleScriptResponse ( vbagMessage );
				else if ( vstrCommand. EqualToString ( CVSTR ( "backtrace" ) ) )
					vError = HandleCallStackResponse ( vbagMessage );
				else if ( vstrCommand. EqualToString ( CVSTR ( "listcontexts" ) ) )
					vError = HandleListContextsResponse ( vbagMessage );
				else if (	vstrCommand. EqualToString ( CVSTR ( "continue" ) ) ||
							vstrCommand. EqualToString ( CVSTR ( "stepin" ) ) ||
							vstrCommand. EqualToString ( CVSTR ( "stepout" ) ) ||
							vstrCommand. EqualToString ( CVSTR ( "stepover" ) ) )
				{
					 // A reply to "continue" needs not to be propagated to the caller (unless there was an error)
					Boolean			bIsOK = false;
					vbagMessage. GetBoolean ( CrossfireKeys::success, bIsOK );
					xbox_assert ( bIsOK );

					vError = VE_OK;
				}
				else if ( vstrCommand. EqualToString ( CVSTR ( "authenticate" ) ) )
					vError = HandleAuthenticateResponse ( vbagMessage );
				else
				{
					xbox_assert ( false );
					vError = VE_UNIMPLEMENTED;
				}
			}
			else
			{
				xbox_assert ( false );
				vError = VE_UNIMPLEMENTED;
			}
		}
	}
	else
	{
		vError = VE_JSW_DEBUGGER_UNKNOWN_REMOTE_EVENT;
		if ( inLine. GetLength ( ) != 0 )
		{
			// Do not assert on emtpy commands which occur when a connection is broken.
			xbox_assert ( false );
		}
	}

	return vError;
}

XBOX::VError VJSDebugger::AddListener ( IJSDebuggerListener* inListener )
{
	xbox_assert ( inListener != 0 );
	if ( inListener == 0 )
		return VE_OK;

	fListenersLock. Lock ( );
		inListener-> Retain ( );
		fListeners. push_back ( inListener );
	fListenersLock. Unlock ( );

	return VE_OK;
}

XBOX::VError VJSDebugger::RemoveListener ( IJSDebuggerListener* inListener )
{
	xbox_assert ( inListener != 0 );
	if ( inListener == 0 )
		return VE_OK;

	fListenersLock. Lock ( );
		vector<IJSDebuggerListener*>::iterator		iter = std::find ( fListeners. begin ( ), fListeners. end ( ), inListener );
		if ( iter != fListeners. end ( ) )
		{
			fListeners. erase ( iter );
			inListener-> Release ( );
		}
	fListenersLock. Unlock ( );

	return VE_OK;
}

XBOX::VError VJSDebugger::RemoveAllListeners ( )
{
	fListenersLock. Lock ( );
		vector<IJSDebuggerListener*>::iterator		iter = fListeners. begin ( );
		while ( iter != fListeners. end ( ) )
		{
			( *iter )-> Release ( );
			iter++;
		}
		fListeners. clear ( );
	fListenersLock. Unlock ( );

	return VE_OK;
}

XBOX::VError VJSDebugger::RemoveListenersWithUI ( )
{
	fListenersLock. Lock ( );
		vector<IJSDebuggerListener*>::iterator		iter = fListeners. begin ( );
		while ( iter != fListeners. end ( ) )
		{
			if ( ( *iter )-> GetUIID ( ) != 0 )
			{
				( *iter )-> Release ( );
				iter = fListeners. erase ( iter );
			}
			else
				iter++;
		}
	fListenersLock. Unlock ( );

	return VE_OK;
}

XBOX::VError VJSDebugger::SendMessageToListeners ( VJSDebuggerMessage* inMessage )
{
	xbox_assert ( inMessage != 0 );

	VError											vError = VE_OK;

	vector<sLONG>									vctrUIIDs;
	sLONG											nID = 0;
	bool											bIsDuplicate = false;

	vector<IJSDebuggerListener*>					vctrListeners;
	IJSDebuggerListener*							iListener = 0;
	fListenersLock. Lock ( );
		vector<IJSDebuggerListener*>::iterator		iter = fListeners. begin ( );
		while ( iter != fListeners. end ( ) )
		{
			bIsDuplicate = false;
			iListener = *iter;
			nID = iListener-> GetUIID ( );
			
			/*if (	nID != 0 &&
					inMessage-> GetType ( ) == VJSDebuggerMessage::JSWD_CM_ON_BREAK_POINT )
			{
				if ( std::find ( vctrUIIDs. begin ( ), vctrUIIDs. end ( ), nID ) == vctrUIIDs. end ( ) )
					vctrUIIDs. push_back ( nID );
				else
					bIsDuplicate = true;
			}*/

			if ( !bIsDuplicate )
			{
				iListener-> Retain ( );
				vctrListeners. insert ( vctrListeners. begin ( ), iListener );
			}

			iter++;
		}
	fListenersLock. Unlock ( );

	iter = vctrListeners. begin ( );
	while ( iter != vctrListeners. end ( ) )
	{
		inMessage-> SetListener ( *iter );
		inMessage-> Execute ( fCaller );
		vError = inMessage-> GetError ( );
		xbox_assert ( vError == VE_OK );

		( *iter )-> Release ( );
		iter++;
	}

	return VE_OK;

}

XBOX::VError VJSDebugger::SendCallbackToListeners ( VJSDebuggerCallback* inCallback )
{
	xbox_assert ( inCallback != 0 );

	VError											vError = VE_OK;

	vector<sLONG>									vctrUIIDs;
	sLONG											nID = 0;
	bool											bIsDuplicate = false;
	
	IJSDebuggerListener*							iListener = 0;
	vector<IJSDebuggerListener*>					vctrListeners;
	fListenersLock. Lock ( );
		vector<IJSDebuggerListener*>::iterator		iter = fListeners. begin ( );
		while ( iter != fListeners. end ( ) )
		{
			bIsDuplicate = false;
			iListener = *iter;
			nID = iListener-> GetUIID ( );
			if (	nID != 0 &&
					(
						inCallback-> GetType ( ) == VJSDebuggerCallback::JSWD_CC_ON_CALL_STACK_RESULT ||
						inCallback-> GetType ( ) == VJSDebuggerCallback::JSWD_CC_ON_LOOKUP_RESULT /*||
						inCallback-> GetType ( ) == VJSDebuggerCallback::JSWD_CC_ON_SCOPE_RESULT ||
						inCallback-> GetType ( ) == VJSDebuggerCallback::JSWD_CC_ON_SCRIPT_RESULT ||
						inCallback-> GetType ( ) == VJSDebuggerCallback::JSWD_CC_ON_EVALUATE_RESULT*/ ) )
			{
				if ( std::find ( vctrUIIDs. begin ( ), vctrUIIDs. end ( ), nID ) == vctrUIIDs. end ( ) )
					vctrUIIDs. push_back ( nID );
				else
					bIsDuplicate = true;
			}
			
			if ( !bIsDuplicate )
			{
				iListener-> Retain ( );
				vctrListeners. insert ( vctrListeners. begin ( ), iListener );
			}

			iter++;
		}
	fListenersLock. Unlock ( );

	iter = vctrListeners. begin ( );
	while ( iter != vctrListeners. end ( ) )
	{
		inCallback-> SetCallback ( *iter );
		inCallback-> Execute ( fCaller );
		vError = inCallback-> GetError ( );
		xbox_assert ( vError == VE_OK );

		( *iter )-> Release ( );
		iter++;
	}

	return VE_OK;

}

void VJSDebugger::_DebugMsg ( const XBOX::VString & inMessage, const XBOX::VString & inContextID )
{
#ifdef WAKANDA_DEBUGGER_MSG
	XBOX::DebugMsg ( "JS Context %S : %S\r\n", &inContextID, &inMessage );
#endif
}

void VJSDebugger::_GetContextID ( const XBOX::VValueBag & inMessage, XBOX::VString & outContextID )
{
#ifdef WAKANDA_DEBUGGER_MSG
	outContextID. Clear ( );
	if ( inMessage. AttributeExists ( CrossfireKeys::context_id ) )
		inMessage. GetString ( CrossfireKeys::context_id, outContextID );
	else
	{
		const VValueBag*		vbagBody = inMessage. RetainUniqueElement ( CrossfireKeys::body );
		if ( vbagBody == 0 )
			outContextID. AppendCString ( "NO ID" );
		else
		{
			if ( vbagBody-> AttributeExists ( CrossfireKeys::context_id ) )
				vbagBody-> GetString ( CrossfireKeys::context_id, outContextID );
			else
				outContextID. AppendCString ( "NO ID" );

			vbagBody-> Release ( );
		}
	}
#endif
}

VError VJSDebugger::GetIP ( VString & outIP )
{
	if ( fEndPoint == 0 )
	{
		xbox_assert ( false );

		return VE_JSW_DEBUGGER_NOT_CONNECTED;
	}

	fEndPointLock. Lock ( );
		fEndPoint-> GetIP ( outIP );
	fEndPointLock. Unlock ( );

	return VE_OK;
}


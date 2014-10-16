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
#include "JSWServer.h"
#include "JSWDebuggerErrors.h"

using namespace std;

#define WKA_USE_UNIFIED_DBG

VJSWConnectionHandler::VJSWConnectionHandler ( ) :
										fShouldStop ( false ),
										fEndPoint ( 0 ),
										fSequenceGenerator ( 0 ),
										fBlockWaiters ( true ),
										fIsDone ( false ),
										fDebuggerInfo ( 0 ),
										fDebuggerSettings ( 0 ),
										fNeedsAuthentication ( false ),
										fHasAuthenticated ( false )
{
	;
}

VJSWConnectionHandler::~VJSWConnectionHandler ( )
{
	;
}

VError VJSWConnectionHandler::SetEndPoint ( VEndPoint* inEndPoint )
{
	VTCPEndPoint*			vtcpEndPoint = dynamic_cast<VTCPEndPoint*> ( inEndPoint );
	if ( !vtcpEndPoint )
		return VE_INVALID_PARAMETER;

	vtcpEndPoint-> SetIsBlocking ( false );
	//vtcpEndPoint-> SetIsSelectIO ( true );

	_SetEndPoint ( vtcpEndPoint );

	return VE_OK;
}

VError VJSWConnectionHandler::ShakeHands ( )
{
	fEndPointMutex. Lock ( );

		char		szchHandshake [ ] = "CrossfireHandshake\r\n\r\n";
		VError		vError = fEndPoint-> ReadExactly ( szchHandshake, ::strlen ( szchHandshake ), 30 * 1000 );
		if ( vError == VE_OK )
		{
			if ( ::strcmp ( szchHandshake, "CrossfireHandshake\r\n\r\n" ) != 0 )
				vError = VE_SRVR_CONNECTION_FAILED;
			else
			{
				fNeedsAuthentication = ( fDebuggerSettings == 0 ? false : fDebuggerSettings-> NeedsAuthentication ( ) );
				if ( fNeedsAuthentication )
				{
					vError = fEndPoint-> WriteExactly ( "CrossfireHandshake\r\nauth,", ::strlen ( "CrossfireHandshake\r\nauth," ), 10 * 1000 );
					if ( vError == VE_OK )
					{
						VUUID		vNonce;
						vNonce. Regenerate ( );
						vNonce. GetString ( fNonce );
						char		szchNonce [ 33 ];
						fNonce. ToCString ( szchNonce, 33 );
						vError = fEndPoint-> WriteExactly ( szchNonce, ::strlen ( szchNonce ), 10 * 1000 );
					}
					if ( vError == VE_OK && fDebuggerSettings != 0 )
					{
						bool			bHasDebuggerUsers = fDebuggerSettings-> HasDebuggerUsers ( );
						if ( !bHasDebuggerUsers )
							vError = fEndPoint-> WriteExactly ( ",no_debug_users", ::strlen ( ",no_debug_users" ), 10 * 1000 );
					}
					if ( vError == VE_OK )
						vError = fEndPoint-> WriteExactly ( "\r\n", ::strlen ( "\r\n" ), 10 * 1000 );
				}
				else
					vError = fEndPoint-> WriteExactly ( szchHandshake, ::strlen ( szchHandshake ), 10 * 1000 );
			}
		}

	fEndPointMutex. Unlock ( );
	
	::DebugMsg ( "Crossfire HandShake performed\r\n" );

	return vError;
}

VError VJSWConnectionHandler::ReadLine ( )
{
	VError				vError = VE_OK;

	char				szchBuffer [ 1 ];
	uLONG				nRead = 1;
	VString				vstrCRLF ( "\r\n" );
	while ( !( fBuffer. GetLength ( ) > 0 && fBuffer. EndsWith ( vstrCRLF ) ) && vError == VE_OK && VTask::GetCurrent ( )-> GetState ( ) != TS_DYING && VTask::GetCurrent ( )-> GetState ( ) != TS_DEAD )
	{
		nRead = 1;
		fEndPointMutex. Lock ( );
			VTask::GetCurrent ( )-> GetDebugContext ( ). DisableUI ( );
			vError = fEndPoint-> Read ( szchBuffer, &nRead );
			VTask::GetCurrent ( )-> GetDebugContext ( ). EnableUI ( );
		fEndPointMutex. Unlock ( );

		if ( vError == VE_SRVR_RESOURCE_TEMPORARILY_UNAVAILABLE )
			vError = VE_OK;
		if ( vError == VE_OK )
		{
			if ( nRead > 0 )
				fBuffer. AppendBlock ( szchBuffer, nRead, VTC_StdLib_char );
			else
				VTask::GetCurrent ( )-> Sleep ( 10 );
		}

		//xbox_assert ( vError == VE_OK );
	}

	return vError;
}

enum VConnectionHandler::E_WORK_STATUS VJSWConnectionHandler::Handle ( VError& outError )
{
	VError			vError = ShakeHands ( );
	if ( vError != VE_OK )
		fShouldStop = true;

	while ( VTask::GetCurrent ( )-> GetState ( ) != TS_DYING && VTask::GetCurrent ( )-> GetState ( ) != TS_DEAD && !fShouldStop )
	{
		vError = ReadLine ( );
		if ( vError != VE_OK && vError != VE_SRVR_RESOURCE_TEMPORARILY_UNAVAILABLE )
		{
			fShouldStop = true;

			break;
		}

		vError = ExtractCommands ( );
	}

	fIsDone = true;
	_SetEndPoint ( 0 );
	WakeUpAllWaiters ( );
	VJSWConnectionHandler::AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_CONTINUE,
#else
					WAKDebuggerServerMessage::SRV_CONTINUE_MSG,
#endif
					CVSTR ( "" ), CVSTR ( "UNDEFINED" ) );

	return VConnectionHandler::eWS_DONE;
}

VError VJSWConnectionHandler::Stop ( )
{
	fShouldStop = true;

	fEndPointMutex. Lock ( );
		if ( fEndPoint != 0 )
		{
			/* TODO: Some sort of synchronizatin mechanism is needed to make sure that this code is called
			only when the socket is actually blocked on reading. */
			fEndPoint-> SetIsBlocking ( false );
			fEndPoint-> Close ( );
		}
	fEndPointMutex. Unlock ( );

	return VE_OK;
}

void VJSWConnectionHandler::_SetEndPoint ( VTCPEndPoint* inEndPoint )
{
	if ( !fEndPointMutex. Lock ( ) )
		return;

	if ( fEndPoint != 0 )
	{
		fEndPoint-> SetIsBlocking ( false );
		fEndPoint-> Close ( );
		fEndPoint-> Release ( );
		fEndPoint = 0;
	}

	ClearCommands ( );

	fEndPoint = inEndPoint;
	RetainRefCountable ( fEndPoint );

	fEndPointMutex. Unlock ( );
}

bool VJSWConnectionHandler::IsHandling ( )
{
	bool		bResult = false;

	fEndPointMutex. Lock ( );
		bResult = ( fEndPoint != 0 && !fIsDone && HasAccessRights ( ) );
	fEndPointMutex. Unlock ( );

	return bResult;
}

#if !defined(WKA_USE_UNIFIED_DBG)
void VJSWConnectionHandler::SetInfo ( IJSWDebuggerInfo* inInfo )
#else
void VJSWConnectionHandler::SetInfo( IWAKDebuggerInfo* inInfo )
#endif
{
	if ( fDebuggerInfo == 0 )
		fDebuggerInfo = inInfo;
}

#if !defined(WKA_USE_UNIFIED_DBG)
void VJSWConnectionHandler::SetSettings ( IJSWDebuggerSettings* inSettings )
#else
void VJSWConnectionHandler::SetSettings( IWAKDebuggerSettings* inSettings )
#endif
{
	xbox_assert ( fDebuggerSettings == 0 || inSettings == 0 );

	fDebuggerSettings = inSettings;
}

int VJSWConnectionHandler::Write ( const char * inData, long inLength, bool inToUTF8 )
{
	int						nResult = 0;

	
	fEndPointMutex. Lock ( );

	if ( fEndPoint != 0 )
	{
		if ( inToUTF8 )
		{
			VString			vstrPacket;
			vstrPacket. FromBlock ( inData, inLength, VTC_UTF_16 );
			VError			vError = SendPacket ( vstrPacket );
			xbox_assert ( vError == VE_OK );
		}
		else
		{
			StErrorContextInstaller errorContext( false, true);

			fEndPoint-> WriteExactly ( ( void* ) inData, inLength );

			VTask::GetCurrent ( )-> GetDebugContext ( ). EnableUI ( );
		}
	}
	else
		nResult = 1;

	fEndPointMutex. Unlock ( );

	return nResult;
}

VError VJSWConnectionHandler::ReadFile ( VString inFileURL, VString & outContent )
{
	VError				vError = VE_OK;

#if VERSIONWIN
	inFileURL. Remove ( 1, 8 ); // Remove "file:///"
#else
	inFileURL. Remove ( 1, 7 ); // Remove "file://"
#endif

	VURL::Decode ( inFileURL );
	VFile				vf ( inFileURL, FPS_POSIX );
	VFileStream			vfStream ( &vf );
	vError = vfStream. OpenReading ( );
	if ( vError != VE_OK )
	{
		outContent. AppendString ( CVSTR ( "Failed to open file " ) + inFileURL );

		return vError;
	}

	vError = vfStream. GuessCharSetFromLeadingBytes ( VTC_UTF_8 );
	xbox_assert ( vError == VE_OK );
	vError = vfStream. GetText ( outContent );
	vfStream. CloseReading ( );

	return vError;
}

void VJSWConnectionHandler::ParseJSCoreURL ( VString & ioURL )
{
	if ( ioURL. BeginsWith ( CVSTR ( "file:///" ) ) )
	{
#if VERSIONWIN
		ioURL. Remove ( 1, 8 ); // Remove "file:///"
#else
		ioURL. Remove ( 1, 7 ); // Remove "file://"
#endif
	}

	VURL::Decode ( ioURL );
}

int VJSWConnectionHandler::WriteFileContent ( long inCommandID, uintptr_t inContext, const unsigned short* inFilePath, int inPathSize )
{
	VError				vError = VE_OK;

	VValueBag			vbagPacket;

	vbagPacket. SetString ( CrossfireKeys::type, CVSTR ( "response" ) );
	vbagPacket. SetString ( CrossfireKeys::command, CVSTR ( "script" ) );
	VString				vstrContext;
	vstrContext. FromLong8 ( inContext );
	vbagPacket. SetString ( CrossfireKeys::context_id, vstrContext );
	vbagPacket. SetLong ( CrossfireKeys::seq, inCommandID + 1 );
	vbagPacket. SetLong ( CrossfireKeys::request_seq, inCommandID );

	VValueBag*			vbagBody = new VValueBag ( );
	vbagBody-> SetString ( CrossfireKeys::context_id, vstrContext );

	VValueBag*			vbagScript = new VValueBag ( );
	vbagScript-> SetString ( CrossfireKeys::context_id, vstrContext );

	VString				vstrFileURL;
	vstrFileURL. AppendBlock ( inFilePath, inPathSize, VTC_UTF_16 );
	VString				vstrContent;
	vError = ReadFile ( vstrFileURL, vstrContent );
	vbagScript-> SetString ( CrossfireKeys::source, vstrContent );

	ParseJSCoreURL ( vstrFileURL );
	VString				vstrRelativePath;
	CalculateRelativePath ( fSourcesRoot, vstrFileURL, vstrRelativePath );
	vbagScript-> SetString ( CrossfireKeys::relativePath, vstrRelativePath );

	vbagBody-> AddElement ( CrossfireKeys::script, vbagScript );
	vbagScript-> Release ( );

	vbagPacket. AddElement ( CrossfireKeys::body, vbagBody );
	vbagBody-> Release ( );


	VString				vstrPacket;
	vError = vbagPacket. GetJSONString ( vstrPacket, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );
	if ( vError == VE_OK )
	{
		vstrPacket. AppendCString ( "\r\n" );
		vError = SendPacket ( vstrPacket );
	}

	return 0;
}

int VJSWConnectionHandler::WriteSource ( long inCommandID, uintptr_t inContext, const unsigned short* inSource, int inSize )
{
	VString				vstrSource;
	vstrSource. AppendBlock ( inSource, inSize, VTC_UTF_16 );

	VError				vError = WriteSource ( inCommandID, inContext, vstrSource );
	xbox_assert ( vError == VE_OK );

	return 0;
}

VError VJSWConnectionHandler::WriteSource ( long inCommandID, uintptr_t inContext, const VString & vstrSource )
{
	VError				vError = VE_OK;

	VValueBag			vbagPacket;

	vbagPacket. SetString ( CrossfireKeys::type, CVSTR ( "response" ) );
	vbagPacket. SetString ( CrossfireKeys::command, CVSTR ( "script" ) );
	VString				vstrContext;
	vstrContext. FromLong8 ( inContext );
	vbagPacket. SetString ( CrossfireKeys::context_id, vstrContext );
	vbagPacket. SetLong ( CrossfireKeys::seq, inCommandID + 1 );
	vbagPacket. SetLong ( CrossfireKeys::request_seq, inCommandID );

	VValueBag*			vbagBody = new VValueBag ( );
	vbagBody-> SetString ( CrossfireKeys::context_id, vstrContext );

	VValueBag*			vbagScript = new VValueBag ( );
	vbagScript-> SetString ( CrossfireKeys::context_id, vstrContext );

	vbagScript-> SetString ( CrossfireKeys::source, vstrSource );

	vbagBody-> AddElement ( CrossfireKeys::script, vbagScript );
	vbagScript-> Release ( );

	vbagPacket. AddElement ( CrossfireKeys::body, vbagBody );
	vbagBody-> Release ( );


	VString				vstrPacket;
	vError = vbagPacket. GetJSONString ( vstrPacket, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );
	if ( vError == VE_OK )
	{
		vstrPacket. AppendCString ( "\r\n" );
		vError = SendPacket ( vstrPacket );
	}

	return vError;
}

void VJSWConnectionHandler::SetSourcesRoot ( char* inRoot, int inLength )
{
	xbox_assert ( inRoot != 0 && inLength > 2 );

	fSourcesRoot. Clear ( );
	fSourcesRoot. AppendBlock ( inRoot, inLength, VTC_UTF_16 );

	VURL::Decode ( fSourcesRoot );
}

#if !defined(WKA_USE_UNIFIED_DBG)
VError VJSWConnectionHandler::SendContextList ( long inCommandID, uintptr_t* inContextIDs, IJSWDebuggerInfo::JSWD_CONTEXT_STATE* outStates, int inCount )
#else
VError VJSWConnectionHandler::SendContextList ( long inCommandID, uintptr_t* inContextIDs, IWAKDebuggerInfo::JSWD_CONTEXT_STATE* outStates, int inCount )
#endif
{
	VError				vError = VE_OK;

	VValueBag			vbagPacket;

	vbagPacket. SetString ( CrossfireKeys::type, CVSTR ( "response" ) );
	vbagPacket. SetString ( CrossfireKeys::command, CVSTR ( "listcontexts" ) );
	vbagPacket. SetLong ( CrossfireKeys::seq, inCommandID + 1 );
	vbagPacket. SetLong ( CrossfireKeys::request_seq, inCommandID );

	VValueBag*			vbagBody = new VValueBag ( );
	for ( int i = 0; i < inCount; i++ )
	{
		VValueBag*		vbagContext = new VValueBag ( );

		VString			vstrContext;
		vstrContext. FromLong8 ( inContextIDs [ i ] );
		vbagContext-> SetString ( CrossfireKeys::id, vstrContext );

		if ( outStates [ i ] ==
#if !defined(WKA_USE_UNIFIED_DBG)
				IJSWDebuggerInfo::JSWD_CS_PAUSED
#else
			IWAKDebuggerInfo::JSWD_CS_PAUSED
#endif
				)
			vbagContext-> SetString ( CrossfireKeys::state, CVSTR ( "paused" ) );
		else
			vbagContext-> SetString ( CrossfireKeys::state, CVSTR ( "running" ) );

		VString			vstrIndex;
		vstrIndex. FromLong ( i );
		vbagBody-> AddElement ( vstrIndex, vbagContext );
		vbagContext-> Release ( );
	}

	vbagPacket. AddElement ( CrossfireKeys::body, vbagBody );
	vbagBody-> Release ( );

	VString				vstrPacket;
	vError = vbagPacket. GetJSONString ( vstrPacket, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );
	if ( vError == VE_OK )
	{
		vstrPacket. AppendCString ( "\r\n" );
		vError = SendPacket ( vstrPacket );
	}

	return vError;
}


VError VJSWConnectionHandler::SendPacket ( const VString & inBody )
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

	VTask::GetCurrent ( )-> GetDebugContext ( ). DisableUI ( );

	VErrorTaskContext*			vErrTaskContext = VTask::GetErrorContext ( true );
	VErrorContext*				vErrContext = 0;
	if ( vErrTaskContext != 0 )
		vErrContext = vErrTaskContext-> PushNewContext ( false, true );

	fEndPointMutex. Lock ( );

		VError		vError = fEndPoint-> WriteExactly ( szchHeader, nHeaderLength );

		delete [ ] szchHeader;
		xbox_assert ( vError == VE_OK );

		if ( vError == VE_OK )
		{
			vError = fEndPoint-> WriteExactly ( szchMessage, nMessageLength );
			xbox_assert ( vError == VE_OK );
		}
		delete [ ] szchMessage;

	fEndPointMutex. Unlock ( );

	if ( vErrTaskContext != 0 && vErrContext != 0 )
		vErrTaskContext-> PopContext ( );

	VTask::GetCurrent ( )-> GetDebugContext ( ). EnableUI ( );

	return vError;
}

VError VJSWConnectionHandler::ReadCrossfirePacket ( uLONG inLength, XBOX::VValueBag & outMessage )
{
	char*					szchMessage = new char [ inLength ];
	uLONG					nLength = inLength;

	fEndPointMutex. Lock ( );
		VError					vError = fEndPoint-> ReadExactly ( szchMessage, nLength );
	fEndPointMutex. Unlock ( );

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

int VJSWConnectionHandler::SendBreakPoint (
									uintptr_t inContext,
									int inExceptionHandle /* -1 ? notException : ExceptionHandle */,
									char* inURL, int inURLLength,
									char* inFunction, int inFunctionLength,
									int inLine,
									char* inMessage, int inMessageLength /* in bytes */,
									char* inName, int inNameLength /* in bytes */,
									long inBeginOffset, long inEndOffset /* in bytes */ )
{
	xbox_assert ( inContext != 0 && /* inURL != 0 && inURLLength > 0 && inFunction != 0 && inFunctionLength > 0 && */ inLine >= 0 );

	/*
		Crossfire break point message:

		{"seq":58500,"type":"event","event":"onBreak","context_id":"xf0.2::8858239","data":{"url":"http://www.apple.com/","line":15}}

		Wakanda addition: "function" attribute of the "data" object

	*/

	VValueBag			vbagPacket;

	fEndPointMutex. Lock ( );

		fSequenceGenerator++;
		vbagPacket. SetLong ( CrossfireKeys::seq, fSequenceGenerator );
		vbagPacket. SetString ( CrossfireKeys::type, CVSTR ( "event" ) );
		if ( inExceptionHandle != -1 )
			vbagPacket. SetString ( CrossfireKeys::event, CVSTR ( "onException" ) );
		else
			vbagPacket. SetString ( CrossfireKeys::event, CVSTR ( "onBreak" ) );

		VString				vstrContext;
		vstrContext. FromLong8 ( inContext );
		vbagPacket. SetString ( CrossfireKeys::context_id, vstrContext );

		VValueBag*			vbagData = new VValueBag ( );

		VString				vstrURL ( inURL, inURLLength, VTC_UTF_16 );
		if ( inURLLength > 0 )
			ParseJSCoreURL ( vstrURL );

		VString			vstrRelative;
		if ( inURLLength > 0 )
			CalculateRelativePath ( fSourcesRoot, vstrURL, vstrRelative );
		vbagData-> SetString ( CrossfireKeys::url, vstrRelative );

		VString			vstrFunction ( inFunction, inFunctionLength, VTC_UTF_16 );
		vbagData-> SetString ( CrossfireKeys::function, vstrFunction );

		vbagData-> SetLong ( CrossfireKeys::line, inLine );

		if ( inExceptionHandle != -1 )
		{
			vbagData-> SetLong ( CrossfireKeys::handle, inExceptionHandle );
			vbagData-> SetLong ( CrossfireKeys::beginOffset, inBeginOffset );
			vbagData-> SetLong ( CrossfireKeys::endOffset, inEndOffset );
		}
		if ( inMessage != 0 )
		{
			VString			vstrMsg ( inMessage, inMessageLength, VTC_UTF_16 );
			vbagData-> SetString ( CrossfireKeys::message, vstrMsg );
		}
		if ( inName != 0 )
		{
			VString			vstrName ( inName, inNameLength, VTC_UTF_16 );
			vbagData-> SetString ( CrossfireKeys::name, vstrName );
		}

		vbagPacket. AddElement ( CrossfireKeys::data, vbagData );
		vbagData-> Release ( );


		VString				vstrPacket;
		VError				vError = vbagPacket. GetJSONString ( vstrPacket, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );
		if ( vError == VE_OK )
		{
			vstrPacket. AppendCString ( "\r\n" );
			vError = SendPacket ( vstrPacket );
		}

	fEndPointMutex. Unlock ( );

	return ( vError == VE_OK ) ? 0 : -1;
}

int VJSWConnectionHandler::SendContextCreated ( uintptr_t inContext )
{
	fEndPointMutex. Lock ( );

		//fContexts. push_back ( new VJSWContextRunTimeInfo ( inContext ) );

		fSequenceGenerator++;

		VValueBag			vbagPacket;

		vbagPacket. SetLong ( CrossfireKeys::seq, fSequenceGenerator );
		vbagPacket. SetString ( CrossfireKeys::type, CVSTR ( "event" ) );
		vbagPacket. SetString ( CrossfireKeys::event, CVSTR ( "onContextCreated" ) );
		VString				vstrContext;
		vstrContext. AppendLong8 ( inContext );
		vbagPacket. SetString ( CrossfireKeys::context_id, vstrContext );
		VValueBag*			vbagBody = new VValueBag ( );
		vbagBody-> SetString ( CrossfireKeys::href, CVSTR ( "" ) );
		vbagPacket. AddElement( CrossfireKeys::body, vbagBody );
		vbagBody-> Release ( );

		VString				vstrPacket;
		VError				vError = vbagPacket. GetJSONString ( vstrPacket, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays );
		if ( vError == VE_OK )
		{
			vstrPacket. AppendCString ( "\r\n" );
			vError = SendPacket ( vstrPacket );
		}

	fEndPointMutex. Unlock ( );

	return ( vError == VE_OK ) ? 0 : -1;
}

int VJSWConnectionHandler::SendContextDestroyed ( uintptr_t inContext )
{
	fEndPointMutex. Lock ( );

		/*VJSWContextRunTimeInfo*								vjsContextInfo = 0;
		std::vector<VJSWContextRunTimeInfo*>::iterator		iter = fContexts. begin ( );
		while ( iter != fContexts. end ( ) )
		{
			vjsContextInfo = *iter;
			if ( vjsContextInfo-> GetID ( ) == inContext )
			{
				fContexts. erase ( iter );
				delete vjsContextInfo;

				break;
			}

			iter++;
		}*/

		fSequenceGenerator++;

		VValueBag			vbagPacket;

		vbagPacket. SetLong ( CrossfireKeys::seq, fSequenceGenerator );
		vbagPacket. SetString ( CrossfireKeys::type, CVSTR ( "event" ) );
		vbagPacket. SetString ( CrossfireKeys::event, CVSTR ( "onContextDestroyed" ) );
		VString				vstrContext;
		vstrContext. AppendLong8 ( inContext );
		vbagPacket. SetString ( CrossfireKeys::context_id, vstrContext );

		VString				vstrPacket;
		VError				vError = vbagPacket. GetJSONString ( vstrPacket, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays );
		if ( vError == VE_OK )
		{
			vstrPacket. AppendCString ( "\r\n" );
			vError = SendPacket ( vstrPacket );
		}

	fEndPointMutex. Unlock ( );

	return ( vError == VE_OK ) ? 0 : -1;
}

VError VJSWConnectionHandler::ExtractCommands ( )
{
	if ( fBuffer. GetLength ( ) == 0 )
		return VE_OK;

	VError							vError = VE_OK;
	VIndex							nLength = 0;
	while ( ( nLength = fBuffer. Find ( CVSTR ( "\r\n" ) ) ) > 0 )
	{
		VString						vstrCommand;
		fBuffer. GetSubString ( 1, nLength - 1, vstrCommand );
		vstrCommand. TrimeSpaces ( );
		if ( vstrCommand. BeginsWith ( CVSTR ( "Content-Length:" ) ) )
		{
			VString					vstrLength;
			VIndex					nLengthStart = ::strlen ( "Content-Length:" ) + 1;
			vstrCommand. GetSubString ( nLengthStart, vstrCommand. GetLength ( ) - ( nLengthStart - 1 ), vstrLength );
			VValueBag				vbagMessage;
			vError = ReadCrossfirePacket ( vstrLength. GetLong ( ), vbagMessage );
			xbox_assert ( vError == VE_OK );
			if ( vError == VE_OK )
				vError = AddCommand ( vbagMessage );
		}
		else
			AddCommand ( vstrCommand );
		if ( nLength + 1 == fBuffer. GetLength ( ) )
			fBuffer. Clear ( );
		else
			fBuffer. SubString ( nLength + 2, fBuffer. GetLength ( ) - nLength - 1 );
	}

	return vError;
}

VError VJSWConnectionHandler::AddCommand ( const VString & inCommand )
{
	if (	inCommand. Find ( CVSTR ( "eval " ) ) == 1 )
	{
		VIndex		nSpace = inCommand. Find ( CVSTR ( " " ), 6 );
		VString		vstrID;
		inCommand. GetSubString ( 6, nSpace - 6, vstrID );

		VString		vstrParameters;
		inCommand. GetSubString ( nSpace + 1, inCommand. GetLength ( ) - nSpace, vstrParameters );
		vstrParameters. TrimeSpaces ( );

		return VJSWConnectionHandler::AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_EVALUATE,
#else
					WAKDebuggerServerMessage::SRV_EVALUATE_MSG,
#endif
					vstrID, CVSTR ( "" ), vstrParameters );
	}
	else if (	inCommand. EqualToString ( CVSTR ( "localscope" ) ) ||
				inCommand. EqualToString ( CVSTR ( "local scope" ) ) ||
				inCommand. EqualToString ( CVSTR ( "lscope" ) ) ||
				inCommand. EqualToString ( CVSTR ( "ls" ) ) )
		return VJSWConnectionHandler::AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_LOCAL_SCOPE,
#else
					WAKDebuggerServerMessage::SRV_LOCAL_SCOPE,
#endif
					CVSTR ( "" ), CVSTR ( "" ) );
	else if (	inCommand. Find ( CVSTR ( "setframe " ) ) == 1 ||
				inCommand. Find ( CVSTR ( "sframe " ) ) == 1 ||
				inCommand. Find ( CVSTR ( "sf " ) ) == 1 )
	{
		VString		vstrParameters;
		VIndex		nSpaceIndex = inCommand. Find ( CVSTR ( " " ), 1 );
		inCommand. GetSubString ( nSpaceIndex + 1, inCommand. GetLength ( ) - nSpaceIndex, vstrParameters );
		vstrParameters. TrimeSpaces ( );

		return VJSWConnectionHandler::AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_SET_CALL_STACK_FRAME,
#else
					WAKDebuggerServerMessage::SRV_SET_CALL_STACK_FRAME,
#endif
					CVSTR ( "" ), CVSTR ( "" ), vstrParameters );
	}
	else if (	inCommand. EqualToString ( CVSTR ( "stepover" ) ) ||
				inCommand. EqualToString ( CVSTR ( "step over" ) ) ||
				inCommand. EqualToString ( CVSTR ( "sover" ) ) )
		return VJSWConnectionHandler::AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_STEP_OVER,
#else
					WAKDebuggerServerMessage::SRV_STEP_OVER_MSG,
#endif
					CVSTR ( "" ), CVSTR ( "" ) );
	else if (	inCommand. EqualToString ( CVSTR ( "stepin" ) ) ||
				inCommand. EqualToString ( CVSTR ( "step in" ) ) ||
				inCommand. EqualToString ( CVSTR ( "sin" ) ) )
		return VJSWConnectionHandler::AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_STEP_IN,
#else
					WAKDebuggerServerMessage::SRV_STEP_INTO_MSG,
#endif
					CVSTR ( "" ), CVSTR ( "" ) );
	else if (	inCommand. EqualToString ( CVSTR ( "stepout" ) ) ||
				inCommand. EqualToString ( CVSTR ( "step out" ) ) ||
				inCommand. EqualToString ( CVSTR ( "sout" ) ) )
		return VJSWConnectionHandler::AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_STEP_OUT,
#else
					WAKDebuggerServerMessage::SRV_STEP_OUT_MSG,
#endif
					CVSTR ( "" ), CVSTR ( "" ) );
	else if (	inCommand. EqualToString ( CVSTR ( "continue" ) ) ||
				inCommand. EqualToString ( CVSTR ( "c" ) ) )
		return VJSWConnectionHandler::AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_CONTINUE,
#else
					WAKDebuggerServerMessage::SRV_CONTINUE_MSG,
#endif
					CVSTR ( "" ), CVSTR ( "" ) );
	else if (	inCommand. EqualToString ( CVSTR ( "call stack" ) ) ||
				inCommand. EqualToString ( CVSTR ( "callstack" ) ) ||
				inCommand. EqualToString ( CVSTR ( "cstack" ) ) ||
				inCommand. EqualToString ( CVSTR ( "cs" ) ) )
		return VJSWConnectionHandler::AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_CALL_STACK,
#else
					WAKDebuggerServerMessage::SRV_GET_CALLSTACK_MSG,
#endif
					CVSTR ( "" ), CVSTR ( "" ) );
	else if (	inCommand. EqualToString ( CVSTR ( "get source" ) ) ||
				inCommand. EqualToString ( CVSTR ( "getsource" ) ) ||
				inCommand. EqualToString ( CVSTR ( "gsource" ) ) ||
				inCommand. EqualToString ( CVSTR ( "gs" ) ) )
		return VJSWConnectionHandler::AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_GET_SOURCE,
#else
					WAKDebuggerServerMessage::SRV_GET_SOURCE_MSG,
#endif
					CVSTR ( "" ), CVSTR ( "" ) );
	else if (	inCommand. EqualToString ( CVSTR ( "get exception" ) ) ||
				inCommand. EqualToString ( CVSTR ( "getexception" ) ) ||
				inCommand. EqualToString ( CVSTR ( "gexception" ) ) ||
				inCommand. EqualToString ( CVSTR ( "ge" ) ) )
		return VJSWConnectionHandler::AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_GET_EXCEPTION,
#else
					WAKDebuggerServerMessage::SRV_GET_EXCEPTION_MSG,
#endif
					CVSTR ( "" ), CVSTR ( "UNDEFINED" ) );
	else if (	inCommand. EqualToString ( CVSTR ( "abort script" ) ) ||
				inCommand. EqualToString ( CVSTR ( "as" ) ) )
		return VJSWConnectionHandler::AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_ABORT_SCRIPT,
#else
					WAKDebuggerServerMessage::SRV_ABORT,
#endif
					CVSTR ( "" ), CVSTR ( "" ) );
	else if (	inCommand. Find ( CVSTR ( "sbp " ) ) == 1 )
	{
		VString		vstrParameters;
		inCommand. GetSubString ( 5, inCommand. GetLength ( ) - 4, vstrParameters );
		vstrParameters. TrimeSpaces ( );

		return VJSWConnectionHandler::AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_SET_BREAK_POINT,
#else
					WAKDebuggerServerMessage::SRV_SET_BREAKPOINT_MSG,
#endif
					CVSTR ( "" ), CVSTR ( "" ), vstrParameters );
	}
	else if (	inCommand. Find ( CVSTR ( "rbp " ) ) == 1 )
	{
		VString		vstrParameters;
		inCommand. GetSubString ( 5, inCommand. GetLength ( ) - 4, vstrParameters );
		vstrParameters. TrimeSpaces ( );

		return VJSWConnectionHandler::AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_REMOVE_BREAK_POINT,
#else
					WAKDebuggerServerMessage::SRV_REMOVE_BREAKPOINT_MSG,
#endif
					CVSTR ( "" ), CVSTR ( "" ), vstrParameters );
	}

	else if (	inCommand. EqualToString ( CVSTR ( "quit" ) ) ||
				inCommand. EqualToString ( CVSTR ( "q" ) ) )
	{
		fShouldStop = true;

		return VE_OK;
	}
	else if (	inCommand. EqualToString ( CVSTR ( "help" ) ) ||
				inCommand. EqualToString ( CVSTR ( "h" ) ) )
	{
		char			szchHelp1 [ ] = "Command description:\r\n\
\thelp or h - print this message\r\n\
\tcontinue or c - continues execution\r\n\
\tstepover or sover - steps over a statement\r\n\
\tstepin or sin - steps into a statement\r\n\
\tstepout or sout - steps out of a function\r\n\
\tabort script or as - aborts script execution with an exception\r\n\
\tevaluate XXX or eval XXX - calculate an expression XXX, for example - eval MyVar\r\n\
\tcallstack or cstack or cs - current call stack info\r\n\
\tgetsource or gsource or gs - get source code of the current JS being executed\r\n\
\tgetexception or gexception or ge - get current exception description\r\n\
\tsbp file_name line_number - set break point in file 'file_name' on line 'line_number'\r\n\
\trbp file_name line_number - remove break point in file 'file_name' on line 'line_number'\r\n\
\tquit or q - quit debugging session\r\n\
\tTo set a break point in the JS code - call DEBUG_BREAK=1, execution will stop at the statement right after this call\r\n\
\tlocalscope or local scope or lscope or ls - get a list of objects local to the current execution scope\r\n\
\tsetframe or or sframe or sf - set call frame index within the call stack\r\n";
		VJSWConnectionHandler::Write ( szchHelp1, ::strlen ( szchHelp1 ) );
	}
	else
		return VJSWConnectionHandler::AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_UNKNOWN,
#else
					WAKDebuggerServerMessage::SRV_UNKNOWN_MSG,
#endif
					CVSTR ( "" ), CVSTR ( "" ) );

	/* TODO: Should return a proper error. */
	return VE_OK;
}

VError VJSWConnectionHandler::ClearCommands ( )
{
	fCommandsMutex. Lock ( );
		sClientCommands. clear ( );
	fCommandsMutex. Unlock ( );

	/* TODO: Should return a proper error. */
	return VE_OK;
}
#if !defined(WKA_USE_UNIFIED_DBG)
VError VJSWConnectionHandler::AddCommand ( IJSWDebugger::JSWD_COMMAND inCommand, const VString & inID, const VString & inContextID )
#else
VError VJSWConnectionHandler::AddCommand( IWAKDebuggerCommand::WAKDebuggerServerMsgType_t inCommand, const VString & inID, const VString & inContextID )
#endif
{
	return AddCommand ( inCommand, inID, inContextID, CVSTR ( "" ) );
}

#if !defined(WKA_USE_UNIFIED_DBG)
VError VJSWConnectionHandler::AddCommand ( IJSWDebugger::JSWD_COMMAND inCommand, const VString & inID, const VString & inContextID, const VString & inParameters )
#else
VError VJSWConnectionHandler::AddCommand( IWAKDebuggerCommand::WAKDebuggerServerMsgType_t inCommand, const VString & inID, const VString & inContextID, const VString & inParameters )
#endif
{
	fCommandsMutex. Lock ( );

		/*if ( inCommand == IJSWDebugger::JSWD_C_ABORT_SCRIPT )
			PostInterruptForContext ( inContextID );*/

		sClientCommands. push_back ( new VJSWDebuggerCommand ( inCommand, inID, inContextID, inParameters ) );
		fCommandEvent. Unlock ( );
	fCommandsMutex. Unlock ( );

	/* TODO: Should return a proper error. */
	return VE_OK;
}


VError VJSWConnectionHandler::AddCommand ( const VValueBag & inCommand )
{
	VError					vError = VE_OK;

	VString					vstrCommand;
	inCommand. GetString ( CrossfireKeys::command, vstrCommand );
	VString					vstrID;
	inCommand. GetString ( CrossfireKeys::seq, vstrID );
	VString					vstrContextID;
	inCommand. GetString ( CrossfireKeys::context_id, vstrContextID );

XBOX::DebugMsg ( CVSTR ( "Received " ) + vstrCommand + CVSTR ( " for execution\n\r" ) );

	if ( !HasAccessRights ( ) && !vstrCommand. EqualToString ( CVSTR ( "authenticate" ) ) )
	{
//		SendAuthenticationNeeded ( vstrID, vstrCommand );

		return VE_OK;
	}

	if ( vstrCommand. EqualToString ( CVSTR ( "scope" ) ) )
	{
		const VValueBag*	vbagArguments = inCommand. RetainUniqueElement ( CrossfireKeys::arguments );
		if ( vbagArguments == 0 )
		{
			xbox_assert ( false );
			vError = VE_JSW_DEBUGGER_UNKNOWN_REMOTE_EVENT;
		}
		else
		{
			VString			vstrFrameIndex;
			vbagArguments-> GetString ( CrossfireKeys::frameNumber, vstrFrameIndex );

			vError = AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_LOCAL_SCOPE,
#else
					WAKDebuggerServerMessage::SRV_LOCAL_SCOPE,
#endif
					vstrID, vstrContextID, vstrFrameIndex );

			vbagArguments-> Release ( );
		}
	}
	else if ( vstrCommand. EqualToString ( CVSTR ( "scopes" ) ) )
	{
		const VValueBag*	vbagArguments = inCommand. RetainUniqueElement ( CrossfireKeys::arguments );
		if ( vbagArguments == 0 )
		{
			xbox_assert ( false );
			vError = VE_JSW_DEBUGGER_UNKNOWN_REMOTE_EVENT;
		}
		else
		{
			VString			vstrFrameIndex;
			vbagArguments-> GetString ( CrossfireKeys::frameNumber, vstrFrameIndex );

			vError = AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_SCOPES,
#else
					WAKDebuggerServerMessage::SRV_SCOPES,
#endif
					vstrID, vstrContextID, vstrFrameIndex );

			vbagArguments-> Release ( );
		}
	}
	else if ( vstrCommand. EqualToString ( CVSTR ( "backtrace" ) ) )
	{
		vError = AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_CALL_STACK,
#else
					WAKDebuggerServerMessage::SRV_GET_CALLSTACK_MSG,
#endif
					vstrID, vstrContextID );
	}
	else if ( vstrCommand. EqualToString ( CVSTR ( "clearbreakpoint" ) ) )
	{
		const VValueBag*	vbagArguments = inCommand. RetainUniqueElement ( CrossfireKeys::arguments );
		if ( vbagArguments == 0 )
		{
			xbox_assert ( false );
			vError = VE_JSW_DEBUGGER_UNKNOWN_REMOTE_EVENT;
		}
		else
		{
			VString			vstrTarget;
			vbagArguments-> GetString ( CrossfireKeys::target, vstrTarget );
			sLONG			nLine = 1;
			vbagArguments-> GetLong ( CrossfireKeys::line, nLine );
			if (fDebuggerSettings != NULL)
				fDebuggerSettings->RemoveBreakPoint(vstrTarget,nLine);
			vstrTarget. AppendCString ( " " );
			vstrTarget. AppendLong ( nLine );

			vError = AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_REMOVE_BREAK_POINT,
#else
					WAKDebuggerServerMessage::SRV_REMOVE_BREAKPOINT_MSG,
#endif
					vstrID, vstrContextID, vstrTarget );

			vbagArguments-> Release ( );
		}
	}
	else if ( vstrCommand. EqualToString ( CVSTR ( "setbreakpoint" ) ) )
	{
		const VValueBag*	vbagArguments = inCommand. RetainUniqueElement ( CrossfireKeys::arguments );
		if ( vbagArguments == 0 )
		{
			xbox_assert ( false );
			vError = VE_JSW_DEBUGGER_UNKNOWN_REMOTE_EVENT;
		}
		else
		{
			VString			vstrTarget;
			vbagArguments-> GetString ( CrossfireKeys::target, vstrTarget );
			sLONG			nLine = 1;
			vbagArguments-> GetLong ( CrossfireKeys::line, nLine );
			if (fDebuggerSettings != NULL)
				fDebuggerSettings->AddBreakPoint(vstrTarget,nLine);
			vstrTarget. AppendCString ( " " );
			vstrTarget. AppendLong ( nLine );

::DebugMsg ( "Crossfire set break point command" );
			
			vError = AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_SET_BREAK_POINT,
#else
					WAKDebuggerServerMessage::SRV_SET_BREAKPOINT_MSG,
#endif
					vstrID, vstrContextID, vstrTarget );

			vbagArguments-> Release ( );
		}
	}
	else if ( vstrCommand. EqualToString ( CVSTR ( "suspend" ) ) )
	{
		vError = AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_SUSPEND,
#else
					WAKDebuggerServerMessage::SRV_SUSPEND,
#endif
					vstrID, vstrContextID );
	}
	else if ( vstrCommand. EqualToString ( CVSTR ( "abort" ) ) )
	{
		vError = AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_ABORT_SCRIPT,
#else
					WAKDebuggerServerMessage::SRV_ABORT,
#endif
					vstrID, vstrContextID );
	}
	else if ( vstrCommand. EqualToString ( CVSTR ( "evaluate" ) ) )
	{
		const VValueBag*	vbagArguments = inCommand. RetainUniqueElement ( CrossfireKeys::arguments );
		if ( vbagArguments == 0 )
		{
			xbox_assert ( false );
			vError = VE_JSW_DEBUGGER_UNKNOWN_REMOTE_EVENT;
		}
		else
		{
			VString			vstrExpression;
			vbagArguments-> GetString ( CrossfireKeys::expression, vstrExpression );

			vError = AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_EVALUATE,
#else
					WAKDebuggerServerMessage::SRV_EVALUATE_MSG,
#endif
					vstrID, vstrContextID, vstrExpression );

			vbagArguments-> Release ( );
		}
	}
	else if ( vstrCommand. EqualToString ( CVSTR ( "lookup" ) ) )
	{
		const VValueBag*	vbagArguments = inCommand. RetainUniqueElement ( CrossfireKeys::arguments );
		if ( vbagArguments == 0 )
		{
			xbox_assert ( false );
			vError = VE_JSW_DEBUGGER_UNKNOWN_REMOTE_EVENT;
		}
		else
		{
			VString			vstrHandle;
			vbagArguments-> GetString ( CrossfireKeys::handle, vstrHandle );

			vError = AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_LOOKUP,
#else
					WAKDebuggerServerMessage::SRV_LOOKUP_MSG,
#endif
					vstrID, vstrContextID, vstrHandle );

			vbagArguments-> Release ( );
		}
	}
	else if ( vstrCommand. EqualToString ( CVSTR ( "script" ) ) )
	{
		const VValueBag*	vbagArguments = inCommand. RetainUniqueElement ( CrossfireKeys::arguments );
		if ( vbagArguments == 0 )
		{
			xbox_assert ( false );
			vError = VE_JSW_DEBUGGER_UNKNOWN_REMOTE_EVENT;
		}
		else
		{
			VString			vstrFrame;
			vbagArguments-> GetString ( CrossfireKeys::frameNumber, vstrFrame );

			vError = AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_GET_SOURCE,
#else
					WAKDebuggerServerMessage::SRV_GET_SOURCE_MSG,
#endif
					vstrID, vstrContextID, vstrFrame );

			vbagArguments-> Release ( );
		}
	}
	else if ( vstrCommand. EqualToString ( CVSTR ( "continue" ) ) )
	{
		AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_CONTINUE,
#else
					WAKDebuggerServerMessage::SRV_CONTINUE_MSG,
#endif
					vstrID, vstrContextID );
	}
	else if ( vstrCommand. EqualToString ( CVSTR ( "stepover" ) ) )
	{
		AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_STEP_OVER,
#else
					WAKDebuggerServerMessage::SRV_STEP_OVER_MSG,
#endif
					vstrID, vstrContextID );
	}
	else if ( vstrCommand. EqualToString ( CVSTR ( "stepin" ) ) )
	{
		AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_STEP_IN,
#else
					WAKDebuggerServerMessage::SRV_STEP_INTO_MSG,
#endif
					vstrID, vstrContextID );
	}
	else if ( vstrCommand. EqualToString ( CVSTR ( "stepout" ) ) )
	{
		AddCommand (
#if !defined(WKA_USE_UNIFIED_DBG)
					IJSWDebugger::JSWD_C_STEP_OUT,
#else
					WAKDebuggerServerMessage::SRV_STEP_OUT_MSG,
#endif
					vstrID, vstrContextID );
	}
	else if ( vstrCommand. EqualToString ( CVSTR ( "listcontexts" ) ) )
	{
		if ( fDebuggerInfo != 0 )
		{
			uintptr_t*									szContextIDs = 0;
#if !defined(WKA_USE_UNIFIED_DBG)
			IJSWDebuggerInfo::JSWD_CONTEXT_STATE*		szStates = 0;
#else
			IWAKDebuggerInfo::JSWD_CONTEXT_STATE*		szStates = 0;
#endif
			int											nCount = 0;
			fDebuggerInfo-> GetContextList ( &szContextIDs, &szStates, nCount );

			vError = SendContextList ( vstrID. GetLong ( ), szContextIDs, szStates, nCount );

			delete [ ] szContextIDs;
			delete [ ] szStates;
		}
		/*
			Need to handle this command right here, without passing the message to JS debugger.

		VString			vstrContextID;
		inCommand. GetString ( CrossfireKeys::context_id, vstrContextID );

		AddCommand ( IJSWDebugger::JSWD_C_STEP_OUT, vstrID, vstrContextID );*/
	}
	else if ( vstrCommand. EqualToString ( CVSTR ( "authenticate" ) ) )
	{
		const VValueBag*	vbagArguments = inCommand. RetainUniqueElement ( CrossfireKeys::arguments );
		if ( vbagArguments == 0 )
		{
			xbox_assert ( false );
			vError = VE_JSW_DEBUGGER_UNKNOWN_REMOTE_EVENT;
		}
		else
		{
			VString			vstrUserName;
			vbagArguments-> GetString ( CrossfireKeys::user, vstrUserName );
			VString			vstrUserPassword;
			vbagArguments-> GetString ( CrossfireKeys::ha1, vstrUserPassword );

			vError = HandleAuthenticate ( vstrID. GetLong ( ), vstrUserName, vstrUserPassword );

			vbagArguments-> Release ( );
		}
	}
	else
	{
		xbox_assert ( false );
		vError = VE_JSW_DEBUGGER_UNKNOWN_REMOTE_EVENT;
	}

XBOX::DebugMsg ( CVSTR ( "Processed " ) + vstrCommand + CVSTR ( "\n\r" ) );

	return vError;
}


#if 0//!defined(WKA_USE_UNIFIED_DBG)
#define K_SET_BRKPT			(IJSWDebugger::JSWD_C_SET_BREAK_POINT)
#define K_REMOVE_BRKPT		(IJSWDebugger::JSWD_C_REMOVE_BREAK_POINT)
IJSWDebuggerCommand* VJSWConnectionHandler::GetNextBreakPointCommand ( )
#else
//#error GH2
#define K_SET_BRKPT			(WAKDebuggerServerMessage::SRV_SET_BREAKPOINT_MSG)
#define K_REMOVE_BRKPT		(WAKDebuggerServerMessage::SRV_REMOVE_BREAKPOINT_MSG)
IWAKDebuggerCommand* VJSWConnectionHandler::GetNextBreakPointCommand ( )
#endif
{
	VJSWDebuggerCommand*		jswdCommand = 0;

	fCommandsMutex. Lock ( );
		vector<VJSWDebuggerCommand*>::iterator		iter = sClientCommands. begin ( );
		while ( iter != sClientCommands. end ( ) )
		{
			if ( ( *iter )-> GetType ( ) == K_SET_BRKPT || ( *iter )-> GetType ( ) == K_REMOVE_BRKPT )
			{
				jswdCommand = *iter;
				sClientCommands. erase ( iter );

				break;
			}

			iter++;
		}
	fCommandsMutex. Unlock ( );

	return jswdCommand;
}

#if !defined(WKA_USE_UNIFIED_DBG)
#define K_C_SUSPEND			(IJSWDebugger::JSWD_C_SUSPEND)
IJSWDebuggerCommand* VJSWConnectionHandler::GetNextSuspendCommand ( uintptr_t inContext )
#else
#define K_C_SUSPEND		(WAKDebuggerServerMessage::SRV_SUSPEND)
IWAKDebuggerCommand* VJSWConnectionHandler::GetNextSuspendCommand ( uintptr_t inContext )
#endif
{
	VJSWDebuggerCommand*		jswdCommand = 0;

	fCommandsMutex. Lock ( );
		vector<VJSWDebuggerCommand*>::iterator		iter = sClientCommands. begin ( );
		while ( iter != sClientCommands. end ( ) )
		{
			if ( ( *iter )-> GetType ( ) == K_C_SUSPEND && ( *iter )-> HasSameContextID ( inContext ) )
			{
				jswdCommand = *iter;
				sClientCommands. erase ( iter );

				break;
			}

			iter++;
		}
	fCommandsMutex. Unlock ( );

	return jswdCommand;
}

#if !defined(WKA_USE_UNIFIED_DBG)
#define K_ABORT_SCRIPT		(IJSWDebugger::JSWD_C_ABORT_SCRIPT)
IJSWDebuggerCommand* VJSWConnectionHandler::GetNextAbortScriptCommand ( uintptr_t inContext )
#else
#define K_ABORT_SCRIPT		(WAKDebuggerServerMessage::SRV_ABORT)
IWAKDebuggerCommand* VJSWConnectionHandler::GetNextAbortScriptCommand ( uintptr_t inContext )
#endif
{
	VJSWDebuggerCommand*		jswdCommand = 0;

	fCommandsMutex. Lock ( );
		vector<VJSWDebuggerCommand*>::iterator		iter = sClientCommands. begin ( );
		while ( iter != sClientCommands. end ( ) )
		{
			if ( ( *iter )-> GetType ( ) == K_ABORT_SCRIPT && ( *iter )-> HasSameContextID ( inContext ) )
			{
				jswdCommand = *iter;
				sClientCommands. erase ( iter );

				break;
			}

			iter++;
		}
	fCommandsMutex. Unlock ( );

	return jswdCommand;
}

#if !defined(WKA_USE_UNIFIED_DBG)
IJSWDebuggerCommand* VJSWConnectionHandler::WaitForClientCommand ( uintptr_t inContext )
#else
IWAKDebuggerCommand* VJSWConnectionHandler::WaitForClientCommand( uintptr_t inContext )
#endif
{
	/* Needs to be updated to handle multiple waiting threads */

	bool						bDone = false;
	VJSWDebuggerCommand*		jswdCommand = 0;
	while ( !bDone )
	{
		fCommandsMutex. Lock ( );

		vector<VJSWDebuggerCommand*>::iterator	iter = sClientCommands. begin ( );
		while ( iter != sClientCommands. end ( ) )
		{
			jswdCommand = *iter;
			if ( jswdCommand-> HasSameContextID ( inContext ) )
			{
				sClientCommands. erase ( iter );
				fCommandEvent. Reset ( );

				break;
			}

			jswdCommand = 0;
			iter++;
		}
		fCommandsMutex. Unlock ( );

		if ( jswdCommand == 0 && fBlockWaiters )
			fCommandEvent. Lock ( );
		else
			bDone = true;
	}

	return jswdCommand;
}


VError VJSWConnectionHandler::WakeUpAllWaiters ( )
{
	fCommandsMutex. Lock ( );
		fBlockWaiters = false;
		fCommandEvent. Unlock ( );
	fCommandsMutex. Unlock ( );

	return VE_OK;
}

void VJSWConnectionHandler::CalculateRelativePath ( const VString & inAbsolutePOSIXRoot, const VString & inAbsolutePOSIXPath, VString & outPath )
{
	/*	Basic tests
	
	VString			vstrRoot1 ( "C:/4D/SGT_Win64/depot/" ); // POSIX
	VString			vstrAbsolutePath1 ( "C:/4D/SGT_Win64/depot/4eDimension/main/Apps/testExternal.js" );	// POSIX
	CalculateRelativePath ( vstrRoot1, vstrAbsolutePath1 );

	VString			vstrRoot			( "C:/4D/SGT_Win64/depot/4eDimension/main/Apps/Win/Debug/Workspace/MP/" ); // POSIX
	VString			vstrAbsolutePath	( "C:/4D/SGT_Win64/depot/4eDimension/main/Apps/WiTest/testExternal.js" );	// POSIX
	CalculateRelativePath ( vstrRoot, vstrAbsolutePath );

	VString			vstrRoot2			( "C:/4D/SGT_Win64/depot/Test/" ); // POSIX
	VString			vstrAbsolutePath2	( "C:/4D/SGT_Win64/depot/4eDimension/main/Apps/testExternal.js" );	// POSIX
	CalculateRelativePath ( vstrRoot2, vstrAbsolutePath2 );

	VString			vstrRoot3			( "C:/4D/SGT_Win64/depot/4eDimension/main/Apps/" ); // POSIX
	VString			vstrAbsolutePath3	( "C:/4D/SGT_Win64/depot/testExternal.js" );	// POSIX
	CalculateRelativePath ( vstrRoot3, vstrAbsolutePath3 );
	*/

	if ( inAbsolutePOSIXPath. BeginsWith ( inAbsolutePOSIXRoot ) )
	{
		inAbsolutePOSIXPath. GetSubString ( inAbsolutePOSIXRoot. GetLength ( ) + 1, inAbsolutePOSIXPath. GetLength ( ) - inAbsolutePOSIXRoot. GetLength ( ), outPath );

		return;
	}

	// Get to the common root
	VFilePath		vfpRoot ( inAbsolutePOSIXRoot, FPS_POSIX );
	uLONG			nRootDepth = vfpRoot. GetDepth ( );
	VFilePath		vfpPath ( inAbsolutePOSIXPath, FPS_POSIX );
	vfpPath. ToFolder ( );
	uLONG			nPathDepth = vfpPath. GetDepth ( );

	VString			vstrToCommonRoot;

	if ( nRootDepth > nPathDepth )
	{
		// Bring the root to the same level
		while ( nRootDepth > nPathDepth )
		{
			vfpRoot. ToParent ( );
			nRootDepth--;
			vstrToCommonRoot. AppendString ( CVSTR ( "../" ) );
		}
	}
	else if ( nRootDepth < nPathDepth )
	{
		// Bring the path to the same level
		while ( nRootDepth < nPathDepth )
		{
			vfpPath. ToParent ( );
			nPathDepth--;
		}
	}

	// Now lets bring both paths to the common root
	while ( vfpPath. GetDepth ( ) > 0 && vfpPath != vfpRoot )
	{
		vfpRoot. ToParent ( );
		vfpPath. ToParent ( );
		vstrToCommonRoot. AppendString ( CVSTR ( "../" ) );
	}

	VString			vstrCommonRoot;
	vfpPath. GetPosixPath ( vstrCommonRoot );

	inAbsolutePOSIXPath. GetSubString ( vstrCommonRoot. GetLength ( ) + 1, inAbsolutePOSIXPath. GetLength ( ) - vstrCommonRoot. GetLength ( ), outPath );
	outPath. Insert ( vstrToCommonRoot, 1 );
}

char* VJSWConnectionHandler::GetRelativeSourcePath (
										const unsigned short* inAbsoluteRoot, int inRootSize,
										const unsigned short* inAbsolutePath, int inPathSize,
										int& outSize )
{
	VString			vstrRoot;
	vstrRoot. AppendBlock ( inAbsoluteRoot, inRootSize, VTC_UTF_16 );

	VString			vstrAbsolutePath;
	vstrAbsolutePath. AppendBlock ( inAbsolutePath, inPathSize, VTC_UTF_16 );
	ParseJSCoreURL ( vstrAbsolutePath );

	VString			vstrRelativePOSIX;
	CalculateRelativePath ( vstrRoot, vstrAbsolutePath, vstrRelativePOSIX );

	outSize = vstrRelativePOSIX. GetLength ( ) * 2 + 2;
	char*			szchResult = new char [ outSize ];
	vstrRelativePOSIX. ToBlock ( szchResult, outSize, VTC_UTF_16, true, false );

	outSize = outSize / 2 - 1;

	return szchResult;
}

char* VJSWConnectionHandler::GetAbsolutePath (
									const unsigned short* inAbsoluteRoot, int inRootSize,
									const unsigned short* inRelativePath, int inPathSize,
									int& outSize )
{
	VString			vstrRoot;
	vstrRoot. AppendBlock ( inAbsoluteRoot, inRootSize, VTC_UTF_16 );
	VFilePath		vfpRoot ( vstrRoot, FPS_POSIX );

	VString			vstrRelativePath;
	vstrRelativePath. AppendBlock ( inRelativePath, inPathSize, VTC_UTF_16 );
	if ( vstrRelativePath. BeginsWith ( CVSTR ( "./" ) ) )
		vstrRelativePath. Remove( 1, 2 );
	VFilePath		vfpAbsolute ( vfpRoot, vstrRelativePath, FPS_POSIX );

	VURL			vurlAbsolute ( vfpAbsolute );
	VString			vstrAbsolute;
	vurlAbsolute. GetAbsoluteURL ( vstrAbsolute, true );

	outSize = vstrAbsolute. GetLength ( ) * 2 + 2;
	char*			szchResult = new char [ outSize ];
	vstrAbsolute. ToBlock ( szchResult, outSize, VTC_UTF_16, true, false );

	outSize = outSize / 2 - 1;

	return szchResult;
}

VError VJSWConnectionHandler::HandleAuthenticate ( long inCommandID, const VString & inUserName, const VString & inUserPassword )
{
	VError		vError = VE_OK;
	fHasAuthenticated = false;
	if ( fDebuggerSettings == 0 )
		// No settings so full access
		fHasAuthenticated = true;
	else
		fHasAuthenticated = fDebuggerSettings-> UserCanDebug ( inUserName. GetCPointer ( ), inUserPassword. GetCPointer ( ) );

	if ( !fHasAuthenticated )
		vError = VE_JSW_DEBUGGER_AUTHENTICATION_FAILED;

	VError	vErrTemp = SendAuthenticationHandled ( inCommandID, fHasAuthenticated );
	xbox_assert ( vErrTemp == VE_OK );
	if ( vError == VE_OK )
		vError  = vErrTemp;

	return vError;
}

VError VJSWConnectionHandler::SendAuthenticationHandled ( long inCommandID, bool inSuccess )
{
	VError				vError = VE_OK;

	VValueBag			vbagPacket;

	vbagPacket. SetString ( CrossfireKeys::type, CVSTR ( "response" ) );
	vbagPacket. SetString ( CrossfireKeys::command, CVSTR ( "authenticate" ) );
	vbagPacket. SetLong ( CrossfireKeys::seq, inCommandID + 1 );
	vbagPacket. SetLong ( CrossfireKeys::request_seq, inCommandID );
	vbagPacket. SetBool ( CrossfireKeys::success, inSuccess );
	if ( !inSuccess && fDebuggerSettings != 0 )
	{
		// Let's check if there are any users at all that can debug
		bool			bHasDebuggerUsers = fDebuggerSettings-> HasDebuggerUsers ( );
		if ( !bHasDebuggerUsers )
			vbagPacket. SetLong ( CrossfireKeys::error, 1 );
	}

	VString				vstrPacket;
	vError = vbagPacket. GetJSONString ( vstrPacket, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );
	if ( vError == VE_OK )
	{
		vstrPacket. AppendCString ( "\r\n" );
		vError = SendPacket ( vstrPacket );
	}

	return vError;
}

VError VJSWConnectionHandler::SendAuthenticationNeeded ( const VString & inCommandID, const VString & inCommand )
{
	VError				vError = VE_OK;

	VValueBag			vbagPacket;

	vbagPacket. SetString ( CrossfireKeys::type, CVSTR ( "response" ) );
	vbagPacket. SetString ( CrossfireKeys::command, inCommand );
	vbagPacket. SetLong ( CrossfireKeys::seq, inCommandID. GetLong ( ) + 1 );
	vbagPacket. SetLong ( CrossfireKeys::request_seq, inCommandID. GetLong ( ) );
	vbagPacket. SetBool ( CrossfireKeys::success, false );

	VString				vstrPacket;
	vError = vbagPacket. GetJSONString ( vstrPacket, JSON_WithQuotesIfNecessary | JSON_UniqueSubElementsAreNotArrays | JSON_PrettyFormatting );
	if ( vError == VE_OK )
	{
		vstrPacket. AppendCString ( "\r\n" );
		vError = SendPacket ( vstrPacket );
	}

	return vError;
}

bool VJSWConnectionHandler::HasAccessRights ( )
{
	if ( !fNeedsAuthentication )
		return true;

	return fHasAuthenticated;
}

/*void VJSWConnectionHandler::PostInterruptForContext ( const VString & inContextID )
{
	fEndPointMutex. Lock ( );
	
		sLONG8												nContextID = 0;
		nContextID = inContextID. GetLong8 ( );
		std::vector<VJSWContextRunTimeInfo*>::iterator		iter = fContexts. begin ( );
		while ( iter != fContexts. end ( ) )
		{
			if ( ( *iter )-> GetID ( ) == nContextID )
			{
				( *iter )-> AddRunTimeInterrupt ( );

				break;
			}

			iter++;
		}

	fEndPointMutex. Unlock ( );
}*/



VJSWConnectionHandlerFactory::VJSWConnectionHandlerFactory ( )
{
	;
}

VJSWConnectionHandlerFactory::~VJSWConnectionHandlerFactory ( )
{
	;
}

VConnectionHandler* VJSWConnectionHandlerFactory::CreateConnectionHandler ( VError& outError )
{
	if ( fDebugger-> HasClients ( ) )
	{
		outError = VE_JSW_DEBUGGER_ALREADY_DEBUGGING;

		return 0;
	}

	VJSWConnectionHandler*		jswConnectionHandler = new VJSWConnectionHandler ( );
	if ( fDebugger != 0 )
	{
		fDebugger-> AddHandler ( jswConnectionHandler );
	}
	outError = VE_OK;

	return jswConnectionHandler;
}

VError VJSWConnectionHandlerFactory::AddNewPort ( PortNumber inPort )
{
	fPorts. push_back ( inPort );

	return VE_OK;
}

VError VJSWConnectionHandlerFactory::GetPorts ( std::vector<PortNumber>& outPorts )
{
	outPorts. insert ( outPorts. end ( ), fPorts. begin ( ), fPorts. end ( ) );

	return VE_OK;
}

VError VJSWConnectionHandlerFactory::RemoveAllPorts ( )
{
	fPorts. clear ( );

	return VE_OK;
}

VError VJSWConnectionHandlerFactory::SetDebugger ( VJSWDebugger* inDebugger )
{
	xbox_assert ( inDebugger != 0 );
	if ( inDebugger == 0 )
		return VE_INVALID_PARAMETER;

	fDebugger = inDebugger;

	return VE_OK;
}

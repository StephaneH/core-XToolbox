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
#ifndef __SNET_ERRORS__
#define __SNET_ERRORS__


BEGIN_TOOLBOX_NAMESPACE


const OsType	kSERVER_NET_SIGNATURE	= 'srvr';


const VError	VE_SRVR_READ_FAILED = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 1 );
const VError	VE_SRVR_WRITE_FAILED = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 2 );
const VError	VE_SRVR_CONNECTION_BROKEN = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 3 );
const VError	VE_SRVR_CONNECTION_FAILED = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 4 );
const VError	VE_SRVR_RESOURCE_TEMPORARILY_UNAVAILABLE = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 5 );
const VError	VE_SRVR_SOCKET_ALREADY_READING = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 6 );
const VError	VE_SRVR_SOCKET_IS_NOT_READING = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 7 );
const VError	VE_SRVR_TOO_MANY_SOCKETS_FOR_SELECT_IO = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 8 );
const VError	VE_SRVR_INVALID_PARAMETER = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 9 );
const VError	VE_SRVR_INVALID_INTERNAL_STATE = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 10 );
const VError	VE_SRVR_ENCRYPTION_FAILED = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 11 );
const VError	VE_SRVR_FAILED_TO_START_LISTENER = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 12 );

const VError	VE_SRVR_FAILED_GET_PROXY_SETTINGS = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 13 );

const VError	VE_SRVR_SESSION_NOT_FOUND = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 14 );
const VError	VE_SRVR_POSTPONED_SESSION_EXPIRED = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 15 );
const VError	VE_SRVR_NULL_ENDPOINT = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 16 );
const VError	VE_SRVR_READ_TIMED_OUT = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 17 );
const VError	VE_SRVR_WRITE_TIMED_OUT = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 18 );
const VError	VE_SRVR_KEEP_ALIVE_FAILED = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 19 );
const VError	VE_SRVR_FAILED_TO_LOCATE_SSL_CERTIFICATE = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 20 );
const VError	VE_SRVR_CANT_SUBSCRIBE_MULTICAST = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 21 );
const VError	VE_SRVR_SOCKET_ALREADY_WATCHING = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 22 );

const VError	VE_SRVR_NOTHING_TO_READ = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 23 );

const VError	VE_SRVR_SESSION_PROTOCOL_FAILED = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 24 );

const VError	VE_SRVR_FAILED_TO_CREATE_CONNECTED_SOCKET = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 30 );
const VError	VE_SRVR_FAILED_TO_CREATE_LISTENING_SOCKET = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 31 );

const VError	VE_SRVR_FAILED_TO_LIST_INTERFACES = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 32 );


const VError	VE_SSL_FRAMEWORK_INIT_FAILED = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 40 );
const VError	VE_SSL_NEW_CONTEXT_FAILED = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 41 );
const VError	VE_SSL_READ_FAILED = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 42 );
const VError	VE_SSL_WRITE_FAILED = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 43 );
const VError	VE_SSL_SHUTDOWN_FAILED = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 44 );
const VError	VE_SSL_FAIL_TO_GET_CERTIFICATE = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 45 );
const VError	VE_SSL_FAIL_TO_SET_CERTIFICATE = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 46 );
const VError	VE_SSL_FAIL_TO_GET_PRIVATE_KEY = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 47 );
const VError	VE_SSL_FAIL_TO_SET_PRIVATE_KEY = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 48 );

const VError	VE_SSL_STACKED_ERROR = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 49 );

const VError	VE_SOCK_TIMED_OUT = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 50 );
const VError	VE_SOCK_CONNECTION_BROKEN = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 51 );
const VError	VE_SOCK_WOULD_BLOCK = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 52 );
const VError	VE_SOCK_READ_FAILED = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 53 );
const VError	VE_SOCK_WRITE_FAILED = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 54 );
const VError	VE_SOCK_UNEXPECTED_FAMILY = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 55 );
const VError	VE_SOCK_FAIL_TO_ADD_SSL = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 56 );
const VError	VE_SOCK_UDP_DATAGRAM_TOO_LONG = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 57 );

const VError	VE_SRVR_FAILED_TO_SET_KEY_AND_CERTIFICATE = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 58 );
const VError	VE_SOCK_PEER_OVER = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 59 );

const VError	VE_SSL_FAIL_TO_WRITE_BIO = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 60 );
const VError	VE_SSL_FAIL_TO_READ_PEM = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 61 );
const VError	VE_SSL_FAIL_TO_INIT_KEY = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 62 );
const VError	VE_SSL_FAIL_TO_INIT_VERIFIER = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 63 );
const VError	VE_SSL_FAIL_TO_UPDATE_DIGEST = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 64 );
const VError	VE_SSL_FAIL_TO_VERIFY_DIGEST = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 65 );

const VError	VE_SRVR_FAILED_TO_SYNC_LOCK = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 101 );
const VError	VE_SRVR_LOCALIZATOR_NOT_AVAILABLE = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 102 );
const VError	VE_SRVR_FAILED_TO_LOCALIZE = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 103 );

const VError	VE_SRVR_BONJOUR_NAME_TOO_LONG = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 301 );
const VError	VE_SRVR_BONJOUR_CANT_FIT_PACKET = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 302 );
const VError	VE_SRVR_BONJOUR_MALFORMED_PACKET = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 303 );
const VError	VE_SRVR_BONJOUR_INVALID_ATTRIBUTE = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 304 );
const VError	VE_SRVR_BONJOUR_TXT_NOT_ASCII = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 305 );
const VError	VE_SRVR_BONJOUR_NETWORK_FAILURE = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 306 );
const VError	VE_SRVR_BONJOUR_RECORD_ALREADY_EXISTS = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 307 );		// Not used anymore.
const VError	VE_SRVR_BONJOUR_RECORD_NOT_FOUND = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 308 );
const VError	VE_SRVR_BONJOUR_SERVER_NOT_RUNNING = MAKE_VERROR ( kSERVER_NET_SIGNATURE, 309 );

// WebSocket errors:	

const VError	VE_SRVR_WEBSOCKET_FRAME_HEADER_TOO_SHORT		= MAKE_VERROR(kSERVER_NET_SIGNATURE, 401);	// Need to read more byte(s) to get complete frame header.
const VError	VE_SRVR_WEBSOCKET_FRAME_HEADER_INCORRECT		= MAKE_VERROR(kSERVER_NET_SIGNATURE, 402);	// Frame header is incorrect (length not encoded correctly).
const VError	VE_SRVR_WEBSOCKET_FRAME_TOO_SHORT				= MAKE_VERROR(kSERVER_NET_SIGNATURE, 403);	// The frame header was read correctly, but need to read more byte(s) to get complete payload data.

const VError	VE_SRVR_WEBSOCKET_INVALID_STATE_FOR_READ		= MAKE_VERROR(kSERVER_NET_SIGNATURE, 404);	// Frames are supposed to be read in "open" or "closing" (for closing handshake) states.
const VError	VE_SRVR_WEBSOCKET_INVALID_STATE_FOR_WRITE		= MAKE_VERROR(kSERVER_NET_SIGNATURE, 405);	// Can write only in "open" state.
const VError	VE_SRVR_WEBSOCKET_INVALID_STATE_FOR_CLOSE		= MAKE_VERROR(kSERVER_NET_SIGNATURE, 406);	// Closing handshake can be initiated in "open" state and answered in "closing" state.

const VError	VE_SRVR_WEBSOCKET_NOT_OPEN_STATE				= MAKE_VERROR(kSERVER_NET_SIGNATURE, 407);	// Operation is possible only in "open" state.
const VError	VE_SRVR_WEBSOCKET_MESSAGE_ALREADY_STARTED		= MAKE_VERROR(kSERVER_NET_SIGNATURE, 408);	// Trying to start sending a message while in the middle of another one.
const VError	VE_SRVR_WEBSOCKET_MESSAGE_NOT_STARTED			= MAKE_VERROR(kSERVER_NET_SIGNATURE, 409);	// Trying to continue (VWebSocket::ContinueMessage()) when message hasn't been started (VWebSocket::StartMessage()).

const VError	VE_SRVR_WEBSOCKET_PROTOCOL_ERROR				= MAKE_VERROR(kSERVER_NET_SIGNATURE, 410);	// Protocol error (fits all!).

const VError	VE_SRVR_WEBSOCKET_OPENING_HANDSHAKE_NEED_DATA	= MAKE_VERROR(kSERVER_NET_SIGNATURE, 411);	// Not an error, just need data to read complete response from server.
const VError	VE_SRVR_WEBSOCKET_CONNECTION_FAILED				= MAKE_VERROR(kSERVER_NET_SIGNATURE, 412);	// Server response to WebSocket upgrade (connection) is not 101.
const VError	VE_SRVR_WEBSOCKET_INVALID_SERVER_RESPONSE		= MAKE_VERROR(kSERVER_NET_SIGNATURE, 413);	// Server HTTP response from opening handshake is not valid according to spec.
const VError	VE_SRVR_WEBSOCKET_WRONG_ACCEPTANCE_KEY			= MAKE_VERROR(kSERVER_NET_SIGNATURE, 414);	// Server returned a wrong acceptance key.
const VError	VE_SRVR_WEBSOCKET_INVALID_CLIENT_REQUEST		= MAKE_VERROR(kSERVER_NET_SIGNATURE, 415);	// Server received an invalid HTTP request (start of opening handshake).

class SNETGenericError : public VErrorBase
{
public:
	
	SNETGenericError (
				VError inError,
				VString const & inDescription ) : VErrorBase ( inError, 0 )
	{
		m_vError = inError;
		inDescription. GetString ( m_vstrDescription );
	}

	virtual ~SNETGenericError ( ) { ; }

	virtual void DumpToString ( VString& outDump ) const { m_vstrDescription. GetString ( outDump ); }
	virtual	void GetErrorDescription ( VString& outError ) const { m_vstrDescription. GetString ( outError ); }
	virtual	void GetLocation ( VString& outLocation ) const { outLocation. AppendCString ( "ServerNet" ); }
		
	private:
	
		VError						m_vError;
		VString						m_vstrDescription;
};


END_TOOLBOX_NAMESPACE


#endif


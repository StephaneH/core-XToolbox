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

#ifndef __SNET_WEB_SOCKET__
#define __SNET_WEB_SOCKET__

#include "ServerNet/VServerNet.h"
#include "VTCPEndPoint.h"
#include "VHTTPClient.h"

/* See WebSocket protocol specification at: http://tools.ietf.org/html/rfc6455
 *
 * Implementation notes:
 * 
 *  * When encoding a frame, payload data length is limited to 32-bit (0xffffffff) and not 64-bit.
 *
 *  * Interleaved fragmented messages are not supported. Section 5.4 of spec suggests that this is 
 *    allowed if an extension have been negotiated between peers, this is very complicated and probably 
 *    of no practical use.
 */

BEGIN_TOOLBOX_NAMESPACE

// Make WebSocket frame decoder very strict with regard to spec. Not recommended because it is better to 
// be tolerant of not so compliant peers. Incorrect frames are always rejected still.

#define WEBSOCKET_STRICT_FRAME_DECODER	0

// WebSocket protocol framing.

class XTOOLBOX_API VWebSocketFrame : public VObject 
{
public:

	enum FLAGS {

		FLAG_RSV1	= 0x40,
		FLAG_RSV2	= 0x20,
		FLAG_RSV3	= 0x10,

	};

	enum OPCODES {

		OPCODE_CONTINUATION_FRAME	= 0x00,
		OPCODE_TEXT_FRAME			= 0x01,
		OPCODE_BINARY_FRAME			= 0x02,

		// 0x03-0x07 reserved for future non-control frames.

		OPCODE_CONNECTION_CLOSE		= 0x08,
		OPCODE_PING					= 0x09,
		OPCODE_PONG					= 0x0a,

		// 0x0b-0x0f reserved for future control frames.

	};
	
					VWebSocketFrame ()	{}
	virtual			~VWebSocketFrame ()	{}	// Destructor doesn't free payload data memory.

	bool			GetFIN () const									{	return fFIN;								}
	void			SetFIN (bool inYesNo)							{	fFIN = inYesNo;								}

	// Use FLAGS enum constants to get or set.

	uBYTE			GetRSVFlags () const							{	return fRSVFlags;							}
	void			SetRSVFlags (uBYTE inRSVFlags);

	// Opcode must be between within 0x00 - 0x0f.

	uBYTE			GetOpcode () const								{	return fOpcode;								}
	void			SetOpcode (uBYTE inOpcode);

	// GenerateMaskingKey() doesn't use a "secure" random number generator.

	bool			HasMaskingKey () const							{	return fMaskingKeyFlag;						}
	void			ClearMaskingKey ()								{	fMaskingKeyFlag = false;					}
	uLONG			GetMaskingKey () const							{	return fMaskingKey;							}
	void			SetMaskingKey (uLONG inMaskingKey);
	void			GenerateMaskingKey ();

	// Payload data memory is not managed by VWebSocketFrame. When "decoded" from a frame, it is a pointer into the frame,
	// data is left masked. When set for encoding, it is supposed to be clear, generated frame will mask it if needed.

	const uBYTE		*GetPayloadData () const						{	return fPayloadData;						}
	void			SetPayloadData (const uBYTE *inPayloadData)		{	fPayloadData = inPayloadData;				}
	
	uLONG8			GetPayloadLength () const						{	return fPayloadDataLength;					}
	void			SetPayloadLength (VSize inPayloadDataLength)	{	fPayloadDataLength = inPayloadDataLength;	}

	// Key "masking" is "symmetric", same function is used to encode and decode. The key is 4 bytes long, it is indexed modulo 4.
	// To make the method working on a stream of payload data, it is possible to specify the current key index [0..3].

	static void		ApplyMaskingKey (
						const uBYTE *inMaskedData, 
						uBYTE *outUnmaskedData, 
						uLONG inMaskingKey, 
						VSize inDataLength,
						uLONG inModuloIndex);

	// Decode a potentially partial WebSocket protocol frame. The payload data is a pointer into inFrame memory and it 
	// isn't "unmasked" (use ApplyMaskingKey() to do so if necessary). A frame can be "partial", that is only the header
	// and the beginning of the payload data is read, in which case method returns VE_SRVR_WEBSOCKET_FRAME_TOO_SHORT. 
	// It is up to the user to do the needed buffering then. If fragmentation is used, it will also be up to the user to 
	// "concatenate" decoded frames to make a complete message. Function return the actual length of the decoded frame, 
	// which can be smaller (more bytes read than needed for a complete frame).

	XBOX::VError	Decode (const uBYTE *inFrame, VSize *ioFrameLength);

	// Encode a single WebSocket frame. This is a low-level method: All fields must be set appropriately before call.
	// It is possible to encode a partial frame: If inDataLength is smaller than payload data size, then only the first
	// inDataLength byte(s) of the payload data are included in the encoded frame. It is up to the user to send the 
	// remaining byte(s) on the socket. In particular, inDataLength can be set to zero, in which case only the header 
	// is encoded.

	XBOX::VError	Encode (XBOX::VMemoryBuffer<> *outFrame, VSize inDataLength) const;

	// Encode a complete message: Fragment into frames of inMaximumFrameSize bytes (continuation frame opcode and FIN flag 
	// are set as needed). Set inMaximumFrameSize to zero to never fragment. Note that there is no special handling for 
	// extension data should fragmentation happens. If inGenerateMaskingKey is true, then generate a masking key for each 
	// frame (GenerateMaskingKey()). Otherwise, if a masking key has been set, then use it for all frame(s), otherwise don't 
	// do masking.
	
	XBOX::VError	Encode (
						std::vector<XBOX::VMemoryBuffer<> > *outFrames, 
						XBOX::VSize inMaximumFrameSize, bool inGenerateMaskingKey);

	// For a maximum frame size, compute how large the payload data can be.

	static VSize	GetMaximumDataLength (bool inUseMaskingKey, VSize inMaximumFrameSize);

private:

	bool			fFIN;
	uBYTE			fRSVFlags;
	uBYTE			fOpcode;
	bool			fMaskingKeyFlag;
	uLONG			fMaskingKey;
	const uBYTE		*fPayloadData;
	uLONG8			fPayloadDataLength;
};

// VWebSocket objects are to be instantied by VWebSocketConnector or VWebSocketListener only.

class XTOOLBOX_API VWebSocket : public VObject 
{
public:

	static const sLONG			DEFAULT_WRITE_TIMEOUT		= 500;	// In milliseconds.

	// Default maximum frame size is 65535 bytes, not too small, not too big. There may be a better default size.

	static const VSize			DEFAULT_MAXIMUM_FRAME_SIZE	= 0xffff - 8;

	// A frame may require several socket reads. The default read buffer size can almost contain one frame using 2 bytes for payload data length.
	// If the read buffer has less than MINIMUM_AVAILABLE_BYTES of space left, the read buffer size is increased by READ_BUFFER_SIZE. It is never
	// shrinked.

	static const VSize			READ_BUFFER_SIZE			= 1 << 16;
	static const VSize			MINIMUM_AVAILABLE_BYTES		= 1 << 12;

	// See close frame description in spec.

	static const VSize			MAXIMUM_REASON_LENGTH		= 123;

	// To compute the "acceptance" key, the server will append this suffix to Sec-WebSocket-Key header value from client and return a base64 
	// encoded SHA-1 of it.
	
	static const char			SERVER_KEY_SUFFIX[];

	// VWebSocketConnector or VWebSocketListener handle the	opening handshake and produce only "opened" VWebSocket objects, hence they always 
	// starts in STATE_OPEN state. 
	
	enum STATES {

		STATE_CONNECTING	= 0,
		STATE_OPEN			= 1,
		STATE_CLOSING		= 2,
		STATE_CLOSED		= 3,
  
	};

	// VWebSocket only writes (synchronously) the TCP socket and will close it when done. User is responsible to read the incoming data.
	// If inUseMaskingKey is true, then a masking key is generated and applied for each encoded frame. This is mandatory for "clients".
	// If inMaximumFrameSize is zero, then never fragment encoded frames, otherwise the minimum size for inMaximumFrameSize is 15 bytes.
	// It makes no sense to make the frames too small. The client connector or the server listener may have read more data than needed,
	// hence the "left over" arguments.

								VWebSocket (
									XBOX::VTCPEndPoint *inEndPoint, bool inIsSynchronous,
									const void *inLeftOver, VSize inLeftOverSize,
									bool inUseMaskingKey = true, 
									VSize inMaximumFrameSize = DEFAULT_MAXIMUM_FRAME_SIZE);
	virtual						~VWebSocket ();
	
	XBOX::VTCPEndPoint			*GetEndPoint () const							{	return fEndPoint;					} 

	bool						IsUsingMaskingKey () const						{	return fUseMaskingKey;				}

	VSize						GetMaximumFrameSize () const					{	return fMaximumFrameSize;			}
	void						SetMaximumFrameSize (VSize inMaximumFrameSize);

	// See comment for states definition.

	STATES						GetState () const								{	return fState;						}

	// Force close by dropping the TCP socket.

	XBOX::VError				ForceClose ();

	// Read the socket and put data in the read buffer. If synchronous, the socket will block until data is available. Otherwise (asynchronous) 
	// user must be prepared to handle VE_SOCK_WOULD_BLOCK (SSL may needs to exchange internal data, without having user data available). Also 
	// see comments for READ_BUFFER_SIZE and MINIMUM_AVAILABLE_BYTES regarding buffer allocation.

	XBOX::VError				ReadSocket ();

	// HandleFrame() will just parse the data given whereas ReadFrame() will get it from the read buffer.
	//
	// It is mandatory to call one of these methods until the state is STATE_CLOSED as it handles closing handshake and ping/pong frames (pings 
	// are automatically answered with pongs). If the closing handshake has been initiated by user, then it is up to him or her to check that 
	// data sent in close control frame has been mirrored in the response.
	//
	// There are four cases according to the error code returned :  
	//
	// 1) VE_SRVR_WEBSOCKET_FRAME_HEADER_TOO_SHORT: More data needs to be read, no byte(s) have been consumed. Read more bytes from TCP socket 
	//    then retry.
	// 
	// 2) VE_SRVR_WEBSOCKET_FRAME_TOO_SHORT: A frame has been read, but only part of its payload data. Use GetLastFrame() to retrieve the 
	//    decoded VWebSocketFrame. Compute the header size using GetPayloadData() - inFrame. Then substract it from *ioFrameLength to find 
	//    out how much of the payload data has been read. Subsequent reads from the TCP socket will contain the remaining payload data 
	//    content.
    //
	// 3) XBOX::VE_OK: A complete frame has been read. *ioFrameLength will return the number of byte(s) consumed. 
    //
	// 4) All other error codes are fail. In particular, frame may have been parsed successfully but pong or close reply may have failed.

	XBOX::VError				HandleFrame (const uBYTE *inFrame, VSize *ioFrameLength);
	XBOX::VError				ReadFrame ();

	// Return the last decoded WebSocket frame, HandleFrame() must have been successfully called before.

	const VWebSocketFrame		*GetLastFrame ()								{	return &fDecodingWebSocketFrame;	}

	// Send a WebSocket message, fragment it if needed. Use StartMessage() and ContinueMessage() for very long ones.

	XBOX::VError				SendMessage (const uBYTE *inMessage, VSize inLength, bool inIsText);

	// For sending long messages, it is possible to send them as fragments. Use StartMessage() to start sending the message. Then use 
	// ContinueMessage() to send the remaining fragments of it. Set inIsComplete to true to finish it.
	// 
	// Use VWebSocketFrame::GetMaximumDataLength() to find out what is the best size to fragment message data. This will minimize the
	// number of frames.
	
	XBOX::VError				StartMessage (bool inIsText, uBYTE inRSVFlags, const uBYTE *inData, VSize inLength);
	XBOX::VError				ContinueMessage (uBYTE inRSVFlags, const uBYTE *inData, VSize inLength, bool inIsComplete);
	
	// When a ping frame is sent, peer must respond with a pong frame containing the same data as the ping.
	// Note that it is legal to send unsolicited pong frames. See sections 5.5.2 and 5.5.3 of spec.
	
	XBOX::VError				SendPing (uBYTE inRSVFlags, const uBYTE *inData, VSize inDataLength);
	XBOX::VError				SendPong (uBYTE inRSVFlags, const uBYTE *inData, VSize inDataLength);
	
	// In "open" state, initiate closing handshake. If in "closing" state, answer it and go to "closed" state. Data is optional but answers should
	// mirror the data in received close frame. According to spec, if there is a body (data), then first 2 bytes must be the status code, follow by 
	// an optional UTF-8 encoded string. Also, closing handshake is to be initiated by server only, never by client.

	XBOX::VError				SendClose (uBYTE inRSVFlags, const uBYTE *inData, VSize inDataLength);

	// Generate a base64 encoded (of 16 random bytes) client key.

	static XBOX::VError			GenerateWebSocketKey (XBOX::VString *outWebSocketKey);

	// Compute an server acceptance key from client key.

	static XBOX::VError			ComputeAcceptanceKey (const XBOX::VString &inWebSocketKey, XBOX::VString *outAcceptanceKey);

private:

friend class VWebSocketMessage;

	XBOX::VTCPEndPoint			*fEndPoint;
	bool						fIsSynchronous;
	bool						fUseMaskingKey;	
	VSize						fMaximumFrameSize;			// If zero, do not fragment frames.

	XBOX::VCriticalSection		fMutex;
	
	STATES						fState;

	XBOX::VMemoryBuffer<>		fReadBuffer;				
	VWebSocketFrame				fDecodingWebSocketFrame;	
	VSize						fLastFrameSize;

	bool						fMessageStarted;
	VWebSocketFrame				fEncodingWebSocketFrame;
	XBOX::VMemoryBuffer<>		fSendBuffer;				

	XBOX::VError				_SendPingOrPong (uBYTE inRSVFlags, const uBYTE *inData, VSize inDataLength, bool inIsPing);
	XBOX::VError				_UnmaskPayloadData (XBOX::VMemoryBuffer<> *outBuffer);
};

// Helper to receive WebSocket messages.

class XTOOLBOX_API VWebSocketMessage : public VObject 
{
public:

							VWebSocketMessage (VWebSocket *inWebSocket);
	virtual					~VWebSocketMessage ();

	// Receiving function. *outHasReadFrame is true if at least a frame has been received, use VWebSocket::GetLastFrame()
	// to retrieve the last one received. *outIsComplete is true if a complete message has been received. At each "new" 
	// call to Receive(), the buffer is reset.
	//
	// If the websocket is in STATE_CLOSED, then return immediately with no error. 
	// If the websocket is in STATE_CLOSING, then no message will be received but wait (if synchronous) for the close 
	// control frame answer.
	// If the websocket is in STATE_OPEN and no error is reported, then check *outIsComplete to find out if a message has
	// been received. If not, then a close control frame has been received, use VWebSocket::GetLastFrame(). 
	//
	// If the websocket is asynchronous, then VE_SOCK_WOULD_BLOCK means that zero data has been read (SSL may transmit 
	// underlying protocol data but no user data). VE_SRVR_WEBSOCKET_FRAME_HEADER_TOO_SHORT and VE_SRVR_WEBSOCKET_FRAME_TOO_SHORT 
	// also mean that data more is needed, but if *outHasReadFrame is true, then at least one frame has been read.

	XBOX::VError			Receive (bool *outHasReadFrame, bool *outIsComplete);

	// Last received message. The message payload is from a VMemoryBuffer.

	bool					IsText ()		{	xbox_assert(fFlags & FLAG_IS_COMPLETE); return (fFlags & FLAG_IS_TEXT) != 0;	}
	uBYTE					*GetMessage ()	{	xbox_assert(fFlags & FLAG_IS_COMPLETE); return (uBYTE *) fBuffer.GetDataPtr();	}
	VSize					GetSize ()		{	xbox_assert(fFlags & FLAG_IS_COMPLETE); return fBuffer.GetDataSize();			}
	
	// Reset message buffer to zero size, but do not free it. Note that the message buffer will only grow, never shrink.

	void					ResetBuffer ();

	// Steal the current message, only possible if complete.

	void					Steal ();

private:

	enum {

		FLAG_IS_EMPTY		= 0x1,	// Has not started receiving a new message.
		FLAG_IS_TEXT		= 0x2,	// Message is text or binary, relevant only if FLAG_IS_EMPTY isn't set.
		FLAG_IS_COMPLETE	= 0x4	// Complete message has been received.

	};

	VWebSocket				*fWebSocket;
	uBYTE					fFlags;
	XBOX::VMemoryBuffer<>	fBuffer;

	// Append payload content to message buffer.

	XBOX::VError			_AppendToBuffer (const XBOX::VWebSocketFrame *inFrame);
};

// VWebSocketConnector objects are to be used by client to connect to a WebSocket server. 

class XTOOLBOX_API VWebSocketConnector : public VObject
{
public:

	static const sLONG	kDEFAULT_TIME_OUT = 2;	// In second(s), VHTTPClient cannot be more precise.

	static bool			IsValidURL (const XBOX::VString &inURL);
							
						VWebSocketConnector (const XBOX::VString &inURL, sLONG inConnectionTimeOut = kDEFAULT_TIME_OUT);
						VWebSocketConnector (const XBOX::VURL &inURL, sLONG inConnectionTimeOut = kDEFAULT_TIME_OUT);
	virtual				~VWebSocketConnector ();

	// According to section 4.1 of spec, the "Origin" header field is mandatory for browser client.

	void				SetOrigin (const XBOX::VString &inOrigin);
	
	// Set optional "Sec-WebSocket-Protocol" and/or "Sec-WebSocket-Extensions" header field(s) of client connection request.
	// See section 4.2.1, section 4.2.2, and section 9.

	void				SetSubProtocols (const XBOX::VString &inSubProtocols);
	void				SetExtensions (const XBOX::VString &inExtensions);

	// Use StartOpeningHandshake() to initiate opening handshake. This will send the WebSocket HTTP upgrade request.
	// If inSelectIOPool is non NULL, then reading response from server will be asynchronous.

	XBOX::VError		StartOpeningHandshake (XBOX::VTCPSelectIOPool *inSelectIOPool);

	// ContinueOpeningHandshake() method without parameter is synchronous, it will block until success or failure.
	// The asynchronous version will consume data and return VE_SRVR_WEBSOCKET_OPENING_HANDSHAKE_NEED_DATA until 
	// the HTTP response is fully read.

	XBOX::VError		ContinueOpeningHandshake ();
	XBOX::VError		ContinueOpeningHandshake (const char *inBuffer, VSize inBufferSize);

	// Valid only after ContinueOpeningHandshake() has returned XBOX::VE_OK.
	
	XBOX::VTCPEndPoint	*StealEndPoint ()	{	return fHTTPClient.StealEndPoint();	}
	XBOX::VTCPEndPoint	*GetEndPoint ()		{	return fHTTPClient.GetEndPoint();	}

	// Return true if server response has a "Sec-WebSocket-Protocol" and/or "Sec-WebSocket-Extensions" header field(s).
	
	bool				GetSubProtocol (XBOX::VString *outProtocol);
	bool				GetSubExtensions (XBOX::VString *outExtensions);

	// Return the amount of "left over" and pointer to it.

	VSize				GetLeftOverSize () const		{	return fHTTPClient.GetLeftOverSize();		}
	const void			*GetLeftOverDataPtr  () const	{	return fHTTPClient.GetLeftOverDataPtr();	}	

private:
	
	XBOX::VHTTPClient	fHTTPClient;
	XBOX::VString		fAcceptanceKey;

	void				_Init (const XBOX::VURL &inURL, sLONG inConnectionTimeOut);
	XBOX::VError		_IsResponseOk ();
};

// Servers use VWebSocketListener objects to listen for WebSocket client connections.

class XTOOLBOX_API VWebSocketListener : public VObject
{
public:

						VWebSocketListener (bool inIsSSL);
	virtual				~VWebSocketListener ();	

	void				SetAddressAndPort (const XBOX::VString &inAddress, sLONG inPort);
	void				SetPath (const XBOX::VString &inPath);

	void				SetKeyAndCertificate (const XBOX::VString inKey, const XBOX::VString &inCertificate);

	XBOX::VError		StartListening ();

	// Stop listener, always successful. Can be called anytime, notably to force close and cleaning-up.

	void				Close ();

	// Accept an incoming connection, blocking function.
	// Just in case, it is possible to get "left over" byte(s). However, a client isn't supposed to send
	// frames before the server reply has been received and checked.

	XBOX::VError		AcceptConnection (
							XBOX::VTCPEndPoint **outEndPoint, 
							sLONG inTimeOut = 1000, 
							XBOX::VMemoryBuffer<> *outLeftOver = NULL);

	// Check that the request contains the header entries required by the WebSocket protocol. 
	// But this doesn't check the "meaning" of the request.
	// Request method (must be GET) and HTTP version (must be at least 1.1) must have been checked by caller before.
	
	static XBOX::VError	IsRequestOk (const VHTTPHeader *inHeader);

	// Accept from an already read request (IsRequestOk() should have been called before).
	// Should add support for (sub-) protocol and extension.

	static XBOX::VError	SendOpeningHandshake (const VHTTPHeader *inHeader, XBOX::VTCPEndPoint *inEndPoint);	

	sLONG				GetPort () const	{	return fPort;		}
	XBOX::VString		GetAddress () const	{	return fAddress;	}
	XBOX::VString		GetPath () const	{	return fPath;		}

private:

	bool				fIsSSL;

	sLONG				fPort;
	XBOX::VString		fAddress;
	XBOX::VString		fPath;
	
	XBOX::VString		fKey;
	XBOX::VString		fCertificate;

	XBOX::VSockListener	*fSocketListener;
};

END_TOOLBOX_NAMESPACE

#endif 
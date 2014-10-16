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
#include "VServerNetPrecompiled.h"

#include "VWebSocket.h"

USING_TOOLBOX_NAMESPACE

// See section 4.2.2 of spec.

const char	VWebSocket::SERVER_KEY_SUFFIX[]	= "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

void VWebSocketFrame::SetRSVFlags (uBYTE inRSVFlags)
{
	xbox_assert(!(inRSVFlags & ~0x70));

	fRSVFlags = inRSVFlags;
}

void VWebSocketFrame::SetOpcode (uBYTE inOpcode)
{
	xbox_assert(!(inOpcode & ~0xf));

	fOpcode = inOpcode;
}

void VWebSocketFrame::SetMaskingKey (uLONG inMaskingKey)
{	
	fMaskingKeyFlag = true; 
	fMaskingKey = inMaskingKey;			
}

void VWebSocketFrame::GenerateMaskingKey ()
{
	fMaskingKeyFlag = true; 
	fMaskingKey = (uLONG) XBOX::VSystem::Random(false);	// Should use a secure RNG instead!
}

void VWebSocketFrame::ApplyMaskingKey (
	const uBYTE *inMaskedData, 
	uBYTE *outUnmaskedData, 
	uLONG inMaskingKey, 
	VSize inDataLength,
	uLONG inModuloIndex)
{
	xbox_assert(inMaskedData != NULL && outUnmaskedData != NULL && !(inModuloIndex & ~0x3));

	uBYTE	maskingKey[4];

	maskingKey[0] = inMaskingKey >> 24;
	maskingKey[1] = inMaskingKey >> 16;
	maskingKey[2] = inMaskingKey >> 8;
	maskingKey[3] = inMaskingKey;

	for (VSize i = 0; i < inDataLength; i++, inModuloIndex = (inModuloIndex + 1) & 0x3)

		outUnmaskedData[i] = inMaskedData[i] ^ maskingKey[inModuloIndex];
}

XBOX::VError VWebSocketFrame::Decode (const uBYTE *inFrame, VSize *ioFrameLength)
{
	xbox_assert(inFrame != NULL && ioFrameLength != NULL);	

	// A frame header is at least 2 bytes long.

	if (*ioFrameLength < 2)

		return VE_SRVR_WEBSOCKET_FRAME_HEADER_TOO_SHORT;

	fFIN = (inFrame[0] & 0x80) != 0;
	fRSVFlags = inFrame[0] & (FLAG_RSV1 | FLAG_RSV2 | FLAG_RSV3);
	fOpcode = inFrame[0] & 0xf;
	
	if ((fPayloadDataLength = inFrame[1] & 0x7f) < 126) 

		fPayloadData = &inFrame[2];

	else if (fPayloadDataLength == 126) {
		
		if (*ioFrameLength < 4)

			return VE_SRVR_WEBSOCKET_FRAME_HEADER_TOO_SHORT;

		fPayloadDataLength = inFrame[2] << 8;
		fPayloadDataLength |= inFrame[3];

#if WEBSOCKET_STRICT_FRAME_DECODER

		// Length must be encoded using minimal method.

		if (fPayloadDataLength < 126)

			return VE_SRVR_WEBSOCKET_FRAME_HEADER_INCORRECT;

#endif

		fPayloadData = &inFrame[4];

	} else {

		if (*ioFrameLength < 10)

			return VE_SRVR_WEBSOCKET_FRAME_HEADER_TOO_SHORT;

		// 64-bit length must have highest bit as zero.

		if (inFrame[2] & 0x80)

			return VE_SRVR_WEBSOCKET_FRAME_HEADER_INCORRECT;	

		fPayloadDataLength = (uLONG8) inFrame[2] << 56;
		fPayloadDataLength |= (uLONG8) inFrame[3] << 48;
		fPayloadDataLength |= (uLONG8) inFrame[4] << 40;
		fPayloadDataLength |= (uLONG8) inFrame[5] << 32;
		fPayloadDataLength |= inFrame[6] << 24;
		fPayloadDataLength |= inFrame[7] << 16;
		fPayloadDataLength |= inFrame[8] << 8;
		fPayloadDataLength |= inFrame[9];

#if WEBSOCKET_STRICT_FRAME_DECODER

		// Length must be encoded using minimal method.

		if (fPayloadDataLength < 0xffff)

			return VE_SRVR_WEBSOCKET_FRAME_HEADER_INCORRECT;

#endif

		fPayloadData = &inFrame[10];

	}

	if ((fMaskingKeyFlag = (inFrame[1] >> 7) != 0)) {

		if (inFrame + *ioFrameLength < fPayloadData + 4)

			return VE_SRVR_WEBSOCKET_FRAME_HEADER_TOO_SHORT;

		fMaskingKey = fPayloadData[0] << 24;
		fMaskingKey |= fPayloadData[1] << 16;
		fMaskingKey |= fPayloadData[2] << 8;
		fMaskingKey |= fPayloadData[3];

		fPayloadData += 4;

	}

	uLONG8	frameLength;

	frameLength = fPayloadData - inFrame;	// Header size.
	frameLength += fPayloadDataLength;	
	if (*ioFrameLength < frameLength)

		return VE_SRVR_WEBSOCKET_FRAME_TOO_SHORT;
		
	else {

		*ioFrameLength = (VSize) frameLength;
		return XBOX::VE_OK;

	}
}

XBOX::VError VWebSocketFrame::Encode (XBOX::VMemoryBuffer<> *outFrame, VSize inDataLength) const
{
	xbox_assert(outFrame != NULL && inDataLength <= fPayloadDataLength);

	xbox_assert(!(!fFIN && (fOpcode & 0x8)));	// It is forbidden to fragment control message.
	
	VSize	frameSize;
	
	if (fPayloadDataLength < 126) 

		frameSize = fMaskingKeyFlag ? 6 : 2; 

	else if (fPayloadDataLength < 0xffff) 

		frameSize = fMaskingKeyFlag ? 8 : 4; 

	else

		frameSize = fMaskingKeyFlag ? 14 : 10; 

	frameSize += inDataLength;

	if (!outFrame->SetSize(frameSize))

		return XBOX::VE_MEMORY_FULL;

	uBYTE	*frame;

	frame = (uBYTE *) outFrame->GetDataPtr();
	xbox_assert(frame != NULL);

	frame[0] = fFIN ? 0x80 : 0;

	xbox_assert(!(fRSVFlags & ~0x70));
	frame[0] |= fRSVFlags;

	xbox_assert(!(fOpcode & ~0xf));
	frame[0] |= fOpcode;

	xbox_assert(fPayloadDataLength <= kMAX_VSize);

	VIndex	index;
	
	if (fPayloadDataLength < 126) {

		frame[1] = fPayloadDataLength;
		index = 2;
		
	} else if (fPayloadDataLength < 0xffff) {

		frame[1] = 126;
		frame[2] = fPayloadDataLength >> 8;
		frame[3] = fPayloadDataLength;
		index = 4;

	} else {

		xbox_assert(!(fPayloadDataLength & 0x8000000000000000ull));

		frame[1] = 127;
		frame[2] = fPayloadDataLength >> 56;
		frame[3] = fPayloadDataLength >> 48;
		frame[4] = fPayloadDataLength >> 40;
		frame[5] = fPayloadDataLength >> 32;
		frame[6] = fPayloadDataLength >> 24;
		frame[7] = fPayloadDataLength >> 16;
		frame[8] = fPayloadDataLength >> 8;
		frame[9] = fPayloadDataLength;
		index = 10;

	}

	if (fMaskingKeyFlag) {

		frame[1] |= 0x80;

		uBYTE	*maskingKey, *maskedPayloadData;

		maskingKey = &frame[index];
		maskingKey[0] = fMaskingKey >> 24;
		maskingKey[1] = fMaskingKey >> 16;
		maskingKey[2] = fMaskingKey >> 8;
		maskingKey[3] = fMaskingKey;

		maskedPayloadData = maskingKey + 4;
		for (VSize i = 0; i < inDataLength; i++)

			maskedPayloadData[i] = fPayloadData[i] ^ maskingKey[i & 0x3];		

	} else 

		::memcpy(&frame[index], fPayloadData, inDataLength);

	return XBOX::VE_OK;
}

XBOX::VError VWebSocketFrame::Encode (
	std::vector<XBOX::VMemoryBuffer<> > *outFrames, 
	XBOX::VSize inMaximumFrameSize, bool inGenerateMaskingKey)
{
	xbox_assert(outFrames != NULL);

	xbox_assert(fOpcode);	// Must not be a continuation frame, use low-level Encode() instead.

	XBOX::VError	error;

	outFrames->clear();
	if (inMaximumFrameSize || fPayloadDataLength + 14 <= inMaximumFrameSize) {

		fFIN = true;
		if (inGenerateMaskingKey)

			GenerateMaskingKey();

		xbox_assert(fPayloadDataLength <= kMAX_VSize);

		outFrames->resize(1);
		error = Encode(&outFrames->at(0), fPayloadDataLength);

	} else {

		uBYTE		opcode;
		const uBYTE	*payloadData;
		uLONG8		payloadDataLength;
		VSize		maximumDataLength;

		opcode = fOpcode;
		payloadData = fPayloadData;
		payloadDataLength = fPayloadDataLength;
		maximumDataLength = GetMaximumDataLength(inGenerateMaskingKey || fMaskingKeyFlag, inMaximumFrameSize);

		uLONG8	bytesLeft;
		uLONG	numberFrames;
		
		bytesLeft = payloadDataLength;
		numberFrames = 0;
		for ( ; ; ) {

			VSize	dataLength;

			dataLength = bytesLeft < maximumDataLength ? bytesLeft : maximumDataLength;
			
			fFIN = !(bytesLeft - dataLength); 
			if (inGenerateMaskingKey)

				GenerateMaskingKey();
			
			fPayloadDataLength = dataLength;

			// Encode() will always make smallest possible header.

			outFrames->resize(numberFrames + 1);
			if ((error = Encode(&outFrames->at(numberFrames), dataLength)) != XBOX::VE_OK)

				break;

			numberFrames++;

			if (!fFIN) {

				bytesLeft -= dataLength;

				fOpcode = 0;	// Continuation frame.
				fPayloadData += dataLength;

			} else

				break;

		}

		fOpcode = opcode;
		fPayloadData = payloadData;
		fPayloadDataLength = payloadDataLength;

	}

	return error;
}

VSize VWebSocketFrame::GetMaximumDataLength (bool inUseMaskingKey, VSize inMaximumFrameSize)
{
	VSize	smallHeaderSize, mediumHeaderSize, bigHeaderSize; 

	if (inUseMaskingKey) {

		smallHeaderSize = 2 + 4;
		mediumHeaderSize = 4 + 4;
		bigHeaderSize = 10 + 4;
			
	} else {

		smallHeaderSize = 2;
		mediumHeaderSize = 4;
		bigHeaderSize = 10;
			
	}

	xbox_assert(inMaximumFrameSize >= 15);		// Makes no sense to make frame too small.

	if (inMaximumFrameSize - bigHeaderSize > 0xffff)

		return inMaximumFrameSize - bigHeaderSize;

	else if (inMaximumFrameSize - mediumHeaderSize > 125)

		return inMaximumFrameSize - mediumHeaderSize;

	else

		return inMaximumFrameSize - smallHeaderSize;
}

VWebSocket::VWebSocket (
	XBOX::VTCPEndPoint *inEndPoint, bool inIsSynchronous, 
	const void *inLeftOver, VSize inLeftOverSize, 
	bool inUseMaskingKey, VSize inMaximumFrameSize)
{
	xbox_assert(inEndPoint != NULL);
	xbox_assert(!(inLeftOver == NULL && inLeftOverSize));

	fEndPoint = XBOX::RetainRefCountable<XBOX::VTCPEndPoint>(inEndPoint);
	fIsSynchronous = inIsSynchronous;
	fUseMaskingKey = inUseMaskingKey;	
	SetMaximumFrameSize(inMaximumFrameSize);

	fState = STATE_OPEN;
	fMessageStarted = false;
	fLastFrameSize = 0;

	if (inLeftOverSize) 

		fReadBuffer.PutDataAmortized(0, inLeftOver, inLeftOverSize);
}

VWebSocket::~VWebSocket ()
{
	if (fEndPoint != NULL) 

		ForceClose();
}

void VWebSocket::SetMaximumFrameSize (VSize inMaximumFrameSize)
{
	xbox_assert(!inMaximumFrameSize || inMaximumFrameSize >= 15);

	fMaximumFrameSize = inMaximumFrameSize;	
}

XBOX::VError VWebSocket::ForceClose ()
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	XBOX::VError	error;

	fState = STATE_CLOSED;
	if (fEndPoint != NULL) {

		error = fEndPoint->ForceClose();
		XBOX::ReleaseRefCountable<XBOX::VTCPEndPoint>(&fEndPoint);

	} else

		error = XBOX::VE_OK;
	
	return error;
}

XBOX::VError VWebSocket::ReadSocket ()
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);
	VSize									allocatedSize, dataSize;
	uLONG									availableBytes;	

	allocatedSize = fReadBuffer.GetAllocatedSize();
	dataSize = fReadBuffer.GetDataSize();
	if ((availableBytes = (uLONG) (allocatedSize - dataSize)) < MINIMUM_AVAILABLE_BYTES) {

		if (!fReadBuffer.Reserve(allocatedSize + READ_BUFFER_SIZE))

			return XBOX::VE_MEMORY_FULL;
		
		allocatedSize = fReadBuffer.GetAllocatedSize();
		availableBytes = (uLONG) (allocatedSize - dataSize);
		xbox_assert(availableBytes >= MINIMUM_AVAILABLE_BYTES);

	}

	uBYTE			*p;
	XBOX::VError	error;

	p = (uBYTE *) fReadBuffer.GetDataPtr() + fReadBuffer.GetDataSize();
	if (fIsSynchronous)
	
		error = fEndPoint->Read(p, &availableBytes);

	else

		error = fEndPoint->DirectSocketRead(p, &availableBytes);

	if (error == XBOX::VE_OK) {

		xbox_assert(availableBytes);

		// Update the data size of the XBOX::VMemoryBuffer.

		p = (uBYTE *) fReadBuffer.GetDataPtr();
		dataSize += availableBytes;
		xbox_assert(allocatedSize >= dataSize);

		fReadBuffer.ForgetData();
		fReadBuffer.SetDataPtr(p, dataSize, allocatedSize);

	} 

	return error;
}

XBOX::VError VWebSocket::HandleFrame (const uBYTE *inFrame, VSize *ioFrameLength)
{
	xbox_assert(inFrame != NULL && ioFrameLength != NULL);

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	if (GetState() != STATE_OPEN && GetState() != STATE_CLOSING)

		return VE_SRVR_WEBSOCKET_INVALID_STATE_FOR_READ;
	
	XBOX::VError	error;

	if ((error = fDecodingWebSocketFrame.Decode(inFrame, ioFrameLength)) == XBOX::VE_OK) {

		// A complete frame has been successfully received and parsed. Control frames cannot be fragmented.

		uBYTE	opcode;

		if ((opcode = fDecodingWebSocketFrame.GetOpcode()) == VWebSocketFrame::OPCODE_PING) {

			// Answer ping.

			if (fState != STATE_CLOSING) {

				// Be tolerant, if in closing state, just ignore ping frames instead of treating them as errors.

				if (fDecodingWebSocketFrame.HasMaskingKey() && fDecodingWebSocketFrame.GetPayloadLength()) {

					XBOX::VMemoryBuffer<>	buffer;
				
					if ((error = _UnmaskPayloadData(&buffer)) == XBOX::VE_OK)

						error = SendPing(
									fDecodingWebSocketFrame.GetRSVFlags(), 
									(const uBYTE *) buffer.GetDataPtr(), 
									buffer.GetDataSize());

				} else 
					
					error = SendPing(
								fDecodingWebSocketFrame.GetRSVFlags(), 
								fDecodingWebSocketFrame.GetPayloadData(), 
								fDecodingWebSocketFrame.GetPayloadLength());

			}

		} else if (opcode == VWebSocketFrame::OPCODE_CONNECTION_CLOSE) {

			// If we're in "open" state, then closing handshake has been initiated by peer. Send answer and go to "closed" state.
			// Otherwise, we've received the answer from the close control frame we've previously sent, so we can also go to the
			// "closed" state. Still, it is up to the user to check that data (if any) has been mirrored correctly.

			if (fState == STATE_OPEN) {

				if (fDecodingWebSocketFrame.HasMaskingKey() && fDecodingWebSocketFrame.GetPayloadLength()) {

					XBOX::VMemoryBuffer<>	buffer;
				
					if ((error = _UnmaskPayloadData(&buffer)) == XBOX::VE_OK)

						error = SendClose(
									fDecodingWebSocketFrame.GetRSVFlags(), 
									(const uBYTE *) buffer.GetDataPtr(), 
									buffer.GetDataSize());

				} else 
					
					error = SendClose(
								fDecodingWebSocketFrame.GetRSVFlags(), 
								fDecodingWebSocketFrame.GetPayloadData(), 
								fDecodingWebSocketFrame.GetPayloadLength());


			} 

			fState = STATE_CLOSED;
			error = ForceClose();

		} 

	}
	
	return error;
}

XBOX::VError VWebSocket::ReadFrame ()
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);
	uBYTE									*buffer;
	VSize									frameSize;
	XBOX::VError							error;

	buffer = (uBYTE *) fReadBuffer.GetDataPtr();
	frameSize = fReadBuffer.GetDataSize();
	if ((error = HandleFrame((const uBYTE *) buffer, &frameSize)) == XBOX::VE_OK) 

		fLastFrameSize = frameSize;

	else 

		fLastFrameSize = 0;

	return error;
}

XBOX::VError VWebSocket::SendMessage (const uBYTE *inMessage, VSize inLength, bool inIsText)
{
	xbox_assert(!(inMessage == NULL && inLength));

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	if (fState != STATE_OPEN)

		return VE_SRVR_WEBSOCKET_INVALID_STATE_FOR_WRITE;

	XBOX::VError						error;
	std::vector<XBOX::VMemoryBuffer<> > frames;

	fEncodingWebSocketFrame.SetRSVFlags(0);
	fEncodingWebSocketFrame.SetOpcode(inIsText ? VWebSocketFrame::OPCODE_TEXT_FRAME : VWebSocketFrame::OPCODE_BINARY_FRAME);
	fEncodingWebSocketFrame.SetPayloadLength(inLength);
	if (inLength)
	
		fEncodingWebSocketFrame.SetPayloadData(inMessage);

	// FIN flag and masking keys are handled by Encode() function.

	if (!fUseMaskingKey)		

		fEncodingWebSocketFrame.ClearMaskingKey();

	if ((error = fEncodingWebSocketFrame.Encode(&frames, fMaximumFrameSize, fUseMaskingKey)) == XBOX::VE_OK)
		
		for (uLONG i = 0; i < frames.size(); i++) 
	
			if ((error = fEndPoint->WriteExactly(frames[i].GetDataPtr(), (uLONG) frames[i].GetDataSize(), DEFAULT_WRITE_TIMEOUT)) != XBOX::VE_OK)

				break;

	return error;
}

XBOX::VError VWebSocket::StartMessage (bool inIsText, uBYTE inRSVFlags, const uBYTE *inData, VSize inLength)
{
	xbox_assert(inData != NULL && inLength > 0);

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	if (fState != STATE_OPEN)

		return VE_SRVR_WEBSOCKET_INVALID_STATE_FOR_WRITE;

	if (fMessageStarted)

		return VE_SRVR_WEBSOCKET_MESSAGE_ALREADY_STARTED;

	fMessageStarted = true;

	XBOX::VError	error;

	fEncodingWebSocketFrame.SetFIN(false);
	fEncodingWebSocketFrame.SetRSVFlags(inRSVFlags);
	fEncodingWebSocketFrame.SetOpcode(inIsText ? VWebSocketFrame::OPCODE_TEXT_FRAME : VWebSocketFrame::OPCODE_BINARY_FRAME);
	if (fUseMaskingKey)

		fEncodingWebSocketFrame.GenerateMaskingKey();

	else

		fEncodingWebSocketFrame.ClearMaskingKey();

	fEncodingWebSocketFrame.SetPayloadLength(inLength);
	if (inLength)
	
		fEncodingWebSocketFrame.SetPayloadData(inData);

	if ((error = fEncodingWebSocketFrame.Encode(&fSendBuffer, inLength)) == XBOX::VE_OK)

		error = fEndPoint->WriteExactly(fSendBuffer.GetDataPtr(), (uLONG) fSendBuffer.GetDataSize(), DEFAULT_WRITE_TIMEOUT);

	return error;
}

XBOX::VError VWebSocket::ContinueMessage (uBYTE inRSVFlags, const uBYTE *inData, VSize inLength, bool inIsComplete)
{
	xbox_assert(inData != NULL && inLength > 0);

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	if (fState != STATE_OPEN)

		return VE_SRVR_WEBSOCKET_INVALID_STATE_FOR_WRITE;

	if (!fMessageStarted)

		return VE_SRVR_WEBSOCKET_MESSAGE_NOT_STARTED;

	if (inIsComplete)

		fMessageStarted = false;

	XBOX::VError	error;

	fEncodingWebSocketFrame.SetFIN(inIsComplete);
	fEncodingWebSocketFrame.SetRSVFlags(inRSVFlags);
	fEncodingWebSocketFrame.SetOpcode(VWebSocketFrame::OPCODE_CONTINUATION_FRAME);
	if (fUseMaskingKey)

		fEncodingWebSocketFrame.GenerateMaskingKey();

	else

		fEncodingWebSocketFrame.ClearMaskingKey();

	fEncodingWebSocketFrame.SetPayloadLength(inLength);
	if (inLength)
	
		fEncodingWebSocketFrame.SetPayloadData(inData);

	if ((error = fEncodingWebSocketFrame.Encode(&fSendBuffer, inLength)) == XBOX::VE_OK)

		error = fEndPoint->WriteExactly(fSendBuffer.GetDataPtr(), (uLONG) fSendBuffer.GetDataSize(), DEFAULT_WRITE_TIMEOUT);

	return error;
}
	
XBOX::VError VWebSocket::SendPing (uBYTE inRSVFlags, const uBYTE *inData, VSize inDataLength)
{
	return _SendPingOrPong(inRSVFlags, inData, inDataLength, true);
}

XBOX::VError VWebSocket::SendPong (uBYTE inRSVFlags, const uBYTE *inData, VSize inDataLength)
{
	return _SendPingOrPong(inRSVFlags, inData, inDataLength, false);
}

XBOX::VError VWebSocket::SendClose (uBYTE inRSVFlags, const uBYTE *inData, VSize inDataLength)
{
	xbox_assert(!(inData == NULL && inDataLength));
	xbox_assert(!(inDataLength && inDataLength < 2));

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	if (fState != STATE_OPEN && fState != STATE_CLOSING)

		return VE_SRVR_WEBSOCKET_INVALID_STATE_FOR_CLOSE;

	XBOX::VError	error;

	fEncodingWebSocketFrame.SetFIN(true);
	fEncodingWebSocketFrame.SetRSVFlags(inRSVFlags);
	fEncodingWebSocketFrame.SetOpcode(VWebSocketFrame::OPCODE_CONNECTION_CLOSE);
	if (fUseMaskingKey)

		fEncodingWebSocketFrame.GenerateMaskingKey();

	else

		fEncodingWebSocketFrame.ClearMaskingKey();

	fEncodingWebSocketFrame.SetPayloadLength(inDataLength);
	if (inDataLength)
	
		fEncodingWebSocketFrame.SetPayloadData(inData);

	if ((error = fEncodingWebSocketFrame.Encode(&fSendBuffer, inDataLength)) == XBOX::VE_OK
	&& (error = fEndPoint->WriteExactly(fSendBuffer.GetDataPtr(), (uLONG) fSendBuffer.GetDataSize(), DEFAULT_WRITE_TIMEOUT)) == XBOX::VE_OK) {

		if (fState == STATE_OPEN)

			fState = STATE_CLOSING;	// Closing handshake initiated.

		else {

			fState = STATE_CLOSED;
			error = ForceClose();	// Close frame of peer has been answered, can drop the TCP socket.

		}

	}

	return error;
}

XBOX::VError VWebSocket::_SendPingOrPong (uBYTE inRSVFlags, const uBYTE *inData, VSize inDataLength, bool inIsPing)
{
	xbox_assert(!(inData == NULL && inDataLength) && !(inData != NULL && !inDataLength));

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	if (fState != STATE_OPEN)

		return VE_SRVR_WEBSOCKET_NOT_OPEN_STATE;

	XBOX::VError	error;

	fEncodingWebSocketFrame.SetFIN(true);
	fEncodingWebSocketFrame.SetRSVFlags(inRSVFlags);
	fEncodingWebSocketFrame.SetOpcode(inIsPing ? VWebSocketFrame::OPCODE_PING : VWebSocketFrame::OPCODE_PONG);
	if (fUseMaskingKey)

		fEncodingWebSocketFrame.GenerateMaskingKey();

	else

		fEncodingWebSocketFrame.ClearMaskingKey();

	fEncodingWebSocketFrame.SetPayloadLength(inDataLength);
	if (inDataLength)
	
		fEncodingWebSocketFrame.SetPayloadData(inData);

	if ((error = fEncodingWebSocketFrame.Encode(&fSendBuffer, inDataLength)) == XBOX::VE_OK) 

		error = fEndPoint->WriteExactly(fSendBuffer.GetDataPtr(), (uLONG) fSendBuffer.GetDataSize(), DEFAULT_WRITE_TIMEOUT);

	return error;
}

XBOX::VError VWebSocket::_UnmaskPayloadData (XBOX::VMemoryBuffer<> *outBuffer)
{
	xbox_assert(outBuffer != NULL);
	xbox_assert(fDecodingWebSocketFrame.HasMaskingKey() && fDecodingWebSocketFrame.GetPayloadLength());				
	xbox_assert(fDecodingWebSocketFrame.GetPayloadData() != NULL);
	
	if (!outBuffer->SetSize(fDecodingWebSocketFrame.GetPayloadLength()))

		return XBOX::VE_MEMORY_FULL;

	else {

		VWebSocketFrame::ApplyMaskingKey(
			fDecodingWebSocketFrame.GetPayloadData(), 
			(uBYTE *) outBuffer->GetDataPtr(),
			fDecodingWebSocketFrame.GetMaskingKey(),
			fDecodingWebSocketFrame.GetPayloadLength(),
			0);

		return XBOX::VE_OK;

	}
}

XBOX::VError VWebSocket::GenerateWebSocketKey (XBOX::VString *outWebSocketKey)
{
	xbox_assert(outWebSocketKey != NULL);

	uBYTE					random[16];
	XBOX::VMemoryBuffer<>	buffer;
	
	for (uLONG i = 0; i < 16; i++) 

		random[i] = XBOX::VSystem::Random() & 0xff;

	if (!Base64Coder::Encode(random, 16, buffer))

		return XBOX::VE_UNKNOWN_ERROR;	// Base64 encoder doesn't have good error reporting.

	else {

		outWebSocketKey->FromBlock(buffer.GetDataPtr(), buffer.GetDataSize(), VTC_US_ASCII);
		return XBOX::VE_OK;

	}
}

XBOX::VError VWebSocket::ComputeAcceptanceKey (const XBOX::VString &inWebSocketKey, XBOX::VString *outAcceptanceKey)
{
	xbox_assert(outAcceptanceKey != NULL);

	XBOX::VString			key(inWebSocketKey);
	SHA1					checksum;
	XBOX::VMemoryBuffer<>	buffer;

	key.AppendCString(SERVER_KEY_SUFFIX);
	VChecksumSHA1::GetChecksumFromStringUTF8(key, checksum);	// Key is all ASCII characters actually (base64 + ascii suffix).
	if (!Base64Coder::Encode(&checksum, SHA1_SIZE, buffer))

		return XBOX::VE_UNKNOWN_ERROR;	// Base64 encoder doesn't have good error reporting.

	else {

		outAcceptanceKey->FromBlock(buffer.GetDataPtr(), buffer.GetDataSize(), VTC_US_ASCII);
		return XBOX::VE_OK;

	}

	return XBOX::VE_OK;
}

bool VWebSocketConnector::IsValidURL (const XBOX::VString &inURL)
{
	XBOX::VURL		url;
	XBOX::VString	string;

	url.FromString(inURL, false);

	// Scheme must be "ws" (plain) or "wss" (secure).

	url.GetScheme(string);
	if (string.CompareToString("ws") != CR_EQUAL && string.CompareToString("wss") != CR_EQUAL)

		return false;

	// Must be an absolute URL (with a hostname).

	url.GetNetworkLocation(string, false);
	if (string.IsEmpty())

		return false;

	// URL must not have a fragment component.

	url.GetFragment(string, false);
	if (!string.IsEmpty())

		return false;

	// URL must not have a parameter component.

	url.GetParameters(string, false);
	if (!string.IsEmpty())

		return false;

	// Path can be empty and query component is optional.

	return true;
}

VWebSocketMessage::VWebSocketMessage (VWebSocket *inWebSocket)
{
	xbox_assert(inWebSocket != NULL);

	fWebSocket = inWebSocket;
	ResetBuffer();
}

VWebSocketMessage::~VWebSocketMessage ()
{
}

XBOX::VError VWebSocketMessage::Receive (bool *outHasReadFrame, bool *outIsComplete)
{
	xbox_assert(outHasReadFrame != NULL && outIsComplete != NULL);

	*outIsComplete = *outHasReadFrame = false;
	if (fWebSocket->GetState() != XBOX::VWebSocket::STATE_OPEN 
	&& fWebSocket->GetState() != XBOX::VWebSocket::STATE_CLOSING)

		return XBOX::VE_OK;

	XBOX::VError	error;

	// Make sure there is some data to parse.

	if (fWebSocket->fLastFrameSize) {

		// Copy non consummed byte(s) (if any) to front of read buffer.

		uBYTE	*buffer;
		VSize	allocatedSize, dataSize;
		
		buffer = (uBYTE *) fWebSocket->fReadBuffer.GetDataPtr();

		allocatedSize = fWebSocket->fReadBuffer.GetAllocatedSize();

		if ((dataSize = fWebSocket->fReadBuffer.GetDataSize() - fWebSocket->fLastFrameSize))
	
			::memmove(buffer, buffer + fWebSocket->fLastFrameSize, dataSize);

		fWebSocket->fReadBuffer.ForgetData();
		fWebSocket->fReadBuffer.SetDataPtr(buffer, dataSize, allocatedSize);
		fWebSocket->fLastFrameSize = 0;

	}

	if (!fWebSocket->fIsSynchronous) {

		VSize	lastSize;

		lastSize = fWebSocket->fReadBuffer.GetDataSize();
		if ((error = fWebSocket->ReadSocket()) != XBOX::VE_OK)

			return error;

		if (lastSize == fWebSocket->fReadBuffer.GetDataSize())

			return VE_SOCK_WOULD_BLOCK;

		if (fFlags & FLAG_IS_COMPLETE)

			ResetBuffer();

	} else {

		while (!fWebSocket->fReadBuffer.GetDataSize())

			if ((error = fWebSocket->ReadSocket()) != XBOX::VE_OK)

				return error;

		ResetBuffer();

	}

	// Read frame(s) until a complete message, control frame(s) are answered automatically.
	
	for ( ; ; ) {

		if ((error = fWebSocket->ReadFrame()) == VE_SRVR_WEBSOCKET_FRAME_HEADER_TOO_SHORT 
		|| error == VE_SRVR_WEBSOCKET_FRAME_TOO_SHORT) {

			// Needs to read more data, do it if synchronous.

			if (!fWebSocket->fIsSynchronous)

				break;

			if ((error = fWebSocket->ReadSocket()) != XBOX::VE_OK) 

				break;

		} else if (error == XBOX::VE_OK) {

			*outHasReadFrame = true;

			const XBOX::VWebSocketFrame	*frame;

			frame = fWebSocket->GetLastFrame();
			xbox_assert(frame != NULL);

			if (frame->GetOpcode() == XBOX::VWebSocketFrame::OPCODE_CONTINUATION_FRAME) {

				// Continuation frame received but no starting fragmented message.

				if (fFlags == FLAG_IS_EMPTY)

					return VE_SRVR_WEBSOCKET_PROTOCOL_ERROR;

				if ((error = _AppendToBuffer(frame)) != XBOX::VE_OK)

					return error;

			} else if (frame->GetOpcode() == XBOX::VWebSocketFrame::OPCODE_BINARY_FRAME 
			|| frame->GetOpcode() == XBOX::VWebSocketFrame::OPCODE_TEXT_FRAME) {

				fFlags = FLAG_IS_TEXT;
				if ((error = _AppendToBuffer(frame)) != XBOX::VE_OK)

					return error;

				if (frame->GetFIN()) {

					fFlags |= FLAG_IS_COMPLETE;
					*outIsComplete = true;
					break; 

				}

			} else if (frame->GetOpcode() == XBOX::VWebSocketFrame::OPCODE_CONNECTION_CLOSE) {

				// Connection frame received, ignore all pending frames and return.

				break;

			}

		} else

			return error;

	} 

	return error;
}

void VWebSocketMessage::ResetBuffer ()
{
	fFlags = FLAG_IS_EMPTY;
	if (fBuffer.GetDataPtr() != NULL) {

		void	*dataPtr;
		VSize	allocatedSize;

		dataPtr = fBuffer.GetDataPtr();
		allocatedSize = fBuffer.GetAllocatedSize();

		fBuffer.ForgetData();
		fBuffer.SetDataPtr(dataPtr, 0, allocatedSize);

	}
}

void VWebSocketMessage::Steal ()
{
	xbox_assert(fFlags & FLAG_IS_COMPLETE);

	fFlags = FLAG_IS_EMPTY;
	fBuffer.ForgetData();
}

XBOX::VError VWebSocketMessage::_AppendToBuffer (const XBOX::VWebSocketFrame *inFrame)
{
	xbox_assert(inFrame != NULL);

	VSize	payloadLength	= inFrame->GetPayloadLength(),
			oldDataSize		= fBuffer.GetDataSize(), 
			allocatedSize	= fBuffer.GetAllocatedSize(),
			newDataSize;
	void	*dataPtr;

	if (!payloadLength)

		return XBOX::VE_OK;

	// Set the new data (bigger) size, allocating more memory if needed.

	if ((newDataSize = oldDataSize + payloadLength) > allocatedSize) {

		if (!fBuffer.Reserve(newDataSize))

			return XBOX::VE_MEMORY_FULL;

		else {

			dataPtr = fBuffer.GetDataPtr();
			allocatedSize = fBuffer.GetAllocatedSize();

		}

	} else 

		dataPtr = fBuffer.GetDataPtr();

	fBuffer.ForgetData();
	fBuffer.SetDataPtr(dataPtr, newDataSize, allocatedSize);

	// Append the data, unmasking it if needed.
	
	uBYTE	*data	= (uBYTE *) dataPtr + oldDataSize;

	if (inFrame->HasMaskingKey())

		XBOX::VWebSocketFrame::ApplyMaskingKey(inFrame->GetPayloadData(), data, inFrame->GetMaskingKey(), payloadLength, 0);

	else

		::memcpy(data, inFrame->GetPayloadData(), payloadLength);

	return XBOX::VE_OK;
}

VWebSocketConnector::VWebSocketConnector (const XBOX::VString &inURL, sLONG inConnectionTimeOut)
{
	xbox_assert(IsValidURL(inURL));
	xbox_assert(inConnectionTimeOut >= 0);

	XBOX::VURL	url(inURL, false);

	_Init(url, inConnectionTimeOut);
}

VWebSocketConnector::VWebSocketConnector (const XBOX::VURL &inURL, sLONG inConnectionTimeOut)
{
	xbox_assert(inConnectionTimeOut >= 0);

	_Init(inURL, inConnectionTimeOut);
}
 
VWebSocketConnector::~VWebSocketConnector ()
{
}

void VWebSocketConnector::SetOrigin (const XBOX::VString &inOrigin)
{
	fHTTPClient.AddHeader("Origin", inOrigin);	
}

void VWebSocketConnector::SetSubProtocols (const XBOX::VString &inSubProtocols)
{
	xbox_assert(!inSubProtocols.IsEmpty());

	fHTTPClient.AddHeader("Sec-WebSocket-Protocol", inSubProtocols);
}

void VWebSocketConnector::SetExtensions (const XBOX::VString &inExtensions)
{
	xbox_assert(!inExtensions.IsEmpty());

	fHTTPClient.AddHeader("Sec-WebSocket-Extensions", inExtensions);	
}

XBOX::VError VWebSocketConnector::StartOpeningHandshake (XBOX::VTCPSelectIOPool *inSelectIOPool)
{
	XBOX::VError	error;
	XBOX::VString	clientKey;

	if ((error = VWebSocket::GenerateWebSocketKey(&clientKey)) != XBOX::VE_OK
	|| (error = VWebSocket::ComputeAcceptanceKey(clientKey, &fAcceptanceKey)) != XBOX::VE_OK)

		return error;

	fHTTPClient.AddHeader("Sec-WebSocket-Key", clientKey);

	if ((error = fHTTPClient.OpenConnection(inSelectIOPool)) == XBOX::VE_OK
	&& (error = fHTTPClient.SendRequest(HTTP_GET)) == XBOX::VE_OK) {

		if (inSelectIOPool != NULL)

			fHTTPClient.StartReadingResponseHeader();

	}

	return error;	
}

XBOX::VError VWebSocketConnector::ContinueOpeningHandshake ()
{
	XBOX::VError	error;

	if ((error = fHTTPClient.ReadResponseHeader()) == XBOX::VE_OK)
		
		error = _IsResponseOk();

	return error;
}

XBOX::VError VWebSocketConnector::ContinueOpeningHandshake (const char *inBuffer, VSize inBufferSize)
{
	xbox_assert(inBuffer != NULL);

	if (!inBufferSize)

		return VE_SRVR_WEBSOCKET_OPENING_HANDSHAKE_NEED_DATA;

	else {

		XBOX::VError	error;
		bool			isComplete;

		if ((error = fHTTPClient.ContinueReadingResponseHeader(inBuffer, inBufferSize, &isComplete)) == XBOX::VE_OK) {
		
			if (isComplete)

				error = _IsResponseOk();

			else

				error = VE_SRVR_WEBSOCKET_OPENING_HANDSHAKE_NEED_DATA;

		}
		return error;

	}
}

void VWebSocketConnector::_Init (const XBOX::VURL &inURL, sLONG inConnectionTimeOut)
{
	fHTTPClient.Init(inURL, "", false, false, true, false);

	XBOX::VString	scheme;

	inURL.GetScheme(scheme);
	xbox_assert(!scheme.IsEmpty());

	fHTTPClient.SetUseSSL(scheme.EqualToUSASCIICString("wss"));
	fHTTPClient.AddHeader("Upgrade", "websocket");	
	fHTTPClient.AddHeader("Sec-WebSocket-Version", "13");
	fHTTPClient.SetAsUpgradeRequest();
	fHTTPClient.SetUseProxy("", 0);
	fHTTPClient.SetConnectionTimeout(inConnectionTimeOut);
}

XBOX::VError VWebSocketConnector::_IsResponseOk ()
{
	// See section 4.2.2 of spec for requirement of server response.

	if (fHTTPClient.GetHTTPStatusCode() != 101) 

		return VE_SRVR_WEBSOCKET_CONNECTION_FAILED;

	XBOX::VString	string;

	//** HTTPClient.GetResponseHeaderValue() bug (caractère 0xd) 
	
	if (!fHTTPClient.GetResponseHeaderValue("Upgrade", string)
	|| !string.EqualToString("websocket")) 
		
		return VE_SRVR_WEBSOCKET_INVALID_SERVER_RESPONSE;

	if (!fHTTPClient.GetResponseHeaderValue("Connection", string)
	|| !string.EqualToString("Upgrade"))

		return VE_SRVR_WEBSOCKET_INVALID_SERVER_RESPONSE;

	if (!fHTTPClient.GetResponseHeaderValue("Sec-WebSocket-Accept", string)
	|| !string.EqualToString(fAcceptanceKey))

		return VE_SRVR_WEBSOCKET_WRONG_ACCEPTANCE_KEY;

	return XBOX::VE_OK;
}

bool VWebSocketConnector::GetSubProtocol (XBOX::VString *outSubProtocols)
{
	xbox_assert(outSubProtocols != NULL);

	return fHTTPClient.GetResponseHeaderValue("Sec-WebSocket-Protocol", *outSubProtocols);
}

bool VWebSocketConnector::GetSubExtensions (XBOX::VString *outExtensions)
{
	xbox_assert(outExtensions != NULL);

	return fHTTPClient.GetResponseHeaderValue("Sec-WebSocket-Extensions", *outExtensions);
}

VWebSocketListener::VWebSocketListener (bool inIsSSL)
{
	fIsSSL = inIsSSL;
	fAddress = "127.0.0.1";
	fPort = 0;
	fPath = "";
	fSocketListener = NULL;
}

VWebSocketListener::~VWebSocketListener ()
{
	Close();
}

void VWebSocketListener::SetAddressAndPort (const XBOX::VString &inAddress, sLONG inPort)
{
	fAddress = inAddress;
	fPort = inPort;
}

void VWebSocketListener::SetPath (const XBOX::VString &inPath)
{
	fPath = inPath;
}

void VWebSocketListener::SetKeyAndCertificate (const XBOX::VString inKey, const XBOX::VString &inCertificate)
{
	xbox_assert(fIsSSL);

	fKey = inKey;
	fCertificate = inCertificate;
}

XBOX::VError VWebSocketListener::StartListening ()
{
	xbox_assert(fSocketListener == NULL);

	XBOX::VError	error;

	if ((fSocketListener = new XBOX::VSockListener()) == NULL) 
	
		error = XBOX::VE_MEMORY_FULL;

	else if (!fIsSSL || (error = fSocketListener->SetKeyAndCertificate(fKey, fCertificate)) == XBOX::VE_OK) {
		
		if (!fSocketListener->SetBlocking(true)
		|| !fSocketListener->AddListeningPort(fAddress, fPort, fIsSSL)	
		|| !fSocketListener->StartListening()) {

			Close();

			error = XBOX::VE_SRVR_FAILED_TO_CREATE_LISTENING_SOCKET;
		
		} else

			error = XBOX::VE_OK;

	}

	return error;
}

void VWebSocketListener::Close ()
{
	if (fSocketListener != NULL) {

		fSocketListener->StopListeningAndClearPorts();
		delete fSocketListener;
		fSocketListener = NULL;

	}
}

XBOX::VError VWebSocketListener::AcceptConnection (XBOX::VTCPEndPoint **outEndPoint, sLONG inTimeOut, XBOX::VMemoryBuffer<> *outLeftOver)
{
	xbox_assert(outEndPoint != NULL && inTimeOut >= 0);
	xbox_assert(fSocketListener != NULL);

	XBOX::XTCPSock	*socket;

	if (!inTimeOut) {

		// Unlimited wait, not recommended.

		while ((socket = fSocketListener->GetNewConnectedSocket(1000)) == NULL) 

			;

	} else {

		if ((socket = fSocketListener->GetNewConnectedSocket(inTimeOut)) == NULL) 

			return VE_SOCK_TIMED_OUT;
		
	}

	XBOX::VTCPEndPoint	*endPoint = new XBOX::VTCPEndPoint(socket);
	XBOX::VError		error;

	if (endPoint == NULL) 

		error = XBOX::VE_MEMORY_FULL;

	else {

		// Read the request.

		XBOX::VMemoryBuffer<>	request;
		VSize					requestSize;
		
		requestSize = 0;
		for ( ; ; ) {

			char	buffer[1 << 12];
			uLONG	length;

			length = sizeof(buffer) - 1;
			if ((error = endPoint->Read(buffer, &length)) != XBOX::VE_OK)

				break;

			else if (!request.PutDataAmortized(request.GetDataSize(), buffer, length)) {

				error = XBOX::VE_MEMORY_FULL;
				break;

			} else if (request.GetDataSize() > (1 << 17)) {

				// If request is too long, then reject it (this is probably not a valid HTTP request anyway).

				error = VE_SRVR_WEBSOCKET_INVALID_CLIENT_REQUEST;
				break;				
				
			} else {

				const uBYTE	*p;
				VSize		size;

				p = (const uBYTE *) request.GetDataPtr();
				size = request.GetDataSize();
				for (VSize i = 0; i + 4 <= size; i++)

					if (!::memcmp(&p[i], "\r\n\r\n", 4)) {
			
						requestSize = i + 4;
						break;

					}

				if (requestSize)

					break;

			}

		}

		if (error == XBOX::VE_OK) {

			XBOX::VString	string;
			VHTTPHeader		header;

			string.FromBlock(request.GetDataPtr(), requestSize, VTC_US_ASCII);
			if (!header.FromString(string)) 

				error = VE_SRVR_WEBSOCKET_INVALID_CLIENT_REQUEST;

			else if ((error = IsRequestOk(&header)) == XBOX::VE_OK
			&& (error = SendOpeningHandshake(&header, endPoint)) == XBOX::VE_OK) {

				if (outLeftOver != NULL && requestSize < request.GetDataSize()) {

					const uBYTE	*p;
					VSize		leftOver;

					p = (const uBYTE *) request.GetDataPtr() + requestSize;
					leftOver = request.GetDataSize() - requestSize;
					if (!outLeftOver->PutDataAmortized(0, p, leftOver))

						error = XBOX::VE_MEMORY_FULL;

				} else

					*outEndPoint = endPoint; 

			}
			
		}

		if (error != XBOX::VE_OK) {

			if (endPoint == NULL) {

				socket->Close();
				delete socket;

			} else

				XBOX::ReleaseRefCountable<XBOX::VTCPEndPoint>(&endPoint);

		}

	}
	return error;
}

XBOX::VError VWebSocketListener::IsRequestOk (const VHTTPHeader *inHeader)
{
	XBOX::VString	value;

	// "Origin" is not mandatory except for browser according to specification, so don't check it.

	if (!inHeader->GetHeaderValue("Host", value)
	|| !inHeader->GetHeaderValue("Upgrade", value) || !VHTTPHeader::SearchValue(value, "websocket")
	|| !inHeader->GetHeaderValue("Connection", value) || !VHTTPHeader::SearchValue(value, "Upgrade")
	|| !inHeader->GetHeaderValue("Sec-WebSocket-Key", value)
	|| !inHeader->GetHeaderValue("Sec-WebSocket-Version", value) || !VHTTPHeader::SearchValue(value, "13"))

		return XBOX::VE_SRVR_WEBSOCKET_INVALID_CLIENT_REQUEST;

	else

		return XBOX::VE_OK;
}

XBOX::VError VWebSocketListener::SendOpeningHandshake (const VHTTPHeader *inHeader, XBOX::VTCPEndPoint *inEndPoint)
{
	xbox_assert(inHeader != NULL && inEndPoint != NULL);

	XBOX::VString	clientKey, acceptanceKey;

	inHeader->GetHeaderValue("Sec-WebSocket-Key", clientKey);
	VWebSocket::ComputeAcceptanceKey(clientKey, &acceptanceKey);

	char			buffer[1 << 12];
	uLONG			length;
	XBOX::VError	error;

	sprintf(buffer, 
		"HTTP/1.1 101 Switching Protocols\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"		
		"Sec-WebSocket-Accept: ");
	length = (uLONG) ::strlen(buffer);
	if ((error = inEndPoint->WriteExactly(buffer, length)) == XBOX::VE_OK) {

		acceptanceKey.ToBlock(buffer, 1 << 16, VTC_US_ASCII, true, false);
		length = (uLONG) ::strlen(buffer);
		if ((error = inEndPoint->WriteExactly(buffer, length)) == XBOX::VE_OK) {

			sprintf(buffer, "\r\n\r\n");
			length = (uLONG) ::strlen(buffer);
			error = inEndPoint->WriteExactly(buffer, length);

		}

	}

	// If successful, then the opening handshake is complete. 
	// Now only WebSocket protocol frames are to be exchanged.

	return error;
}
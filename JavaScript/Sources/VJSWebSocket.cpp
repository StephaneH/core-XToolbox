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
#include "VJavaScriptPrecompiled.h"
 
#include "VJSWebSocket.h"

#include "VJSContext.h"
#include "VJSGlobalClass.h"
#include "VJSNetSocket.h"

#include "VJSBuffer.h"
#include "VJSEvent.h"
#include "VJSRuntime_blob.h"

USING_TOOLBOX_NAMESPACE

XBOX::VCriticalSection			VJSWebSocketHandler::sMutex;
VJSWebSocketHandler::HandlerMap	VJSWebSocketHandler::sHandlers;

VJSWebSocketObject::VJSWebSocketObject (bool inIsSynchronous, VJSWorker *inWorker, const XBOX::VJSContext &inContext)
:fObject(inContext)
{
	xbox_assert(inWorker != NULL);

	fIsNeutralized = false;
	fIsDedicatedWorker = false;
	fIsSynchronous = inIsSynchronous;
	fWorker = XBOX::RetainRefCountable<VJSWorker>(inWorker);
	fWebSocketConnector = NULL;
	fWebSocket = NULL;	
	fWebSocketMessage = NULL;
	if (inIsSynchronous)
	{
		fObject = VJSWebSocketSyncClass::CreateInstance(inContext, this);
	}
	else
	{
		fObject = VJSWebSocketClass::CreateInstance(inContext, this);
	}
	Retain();
}


VJSWebSocketObject::VJSWebSocketObject (XBOX::VJSParms_construct &ioParms, bool inIsSynchronous, VJSWorker *inWorker)
:fObject(ioParms.GetContext())
{
	xbox_assert(inWorker != NULL);

	fIsNeutralized = false;
	fIsDedicatedWorker = false;
	fIsSynchronous = inIsSynchronous;
	fWorker = XBOX::RetainRefCountable<VJSWorker>(inWorker);
	fWebSocketConnector = NULL;
	fWebSocket = NULL;	
	fWebSocketMessage = NULL;
	if (inIsSynchronous)
	{
		fObject = VJSWebSocketSyncClass::ConstructInstance(ioParms, this);
	}
	else
	{
		fObject = VJSWebSocketClass::ConstructInstance(ioParms, this);
	}
	Retain();
}

VJSWebSocketObject::VJSWebSocketObject (XBOX::VWebSocket *inWebSocket, XBOX::VWebSocketMessage *inWebSocketMessage, const XBOX::VJSContext &inContext)
:fObject(inContext)
{
	xbox_assert(inWebSocket != NULL && inWebSocketMessage != NULL);

	fIsNeutralized = false;
	fIsDedicatedWorker = false;
	fIsSynchronous = false;
	fWorker = NULL;					// RetainAndNeutralize() will do nothing.
	fWebSocketConnector = NULL;
	fWebSocket = inWebSocket;	
	fWebSocketMessage = inWebSocketMessage;
}

VJSWebSocketObject::~VJSWebSocketObject ()
{
	xbox_assert(fWorker != NULL);
	if (fWorker != NULL)

		XBOX::ReleaseRefCountable<VJSWorker>(&fWorker);

	if (fWebSocketMessage != NULL)

		delete fWebSocketMessage;

	if (fWebSocket != NULL) {

		if (fWebSocket->GetState() != VWebSocket::STATE_CLOSED) 

			fWebSocket->ForceClose();

		delete fWebSocket;

	}

	if (fWebSocketConnector != NULL) 

		delete fWebSocketConnector;
}

void VJSWebSocketObject::Initialize (const XBOX::VJSParms_initialize &inParms, VJSWebSocketObject *inWebSocket)
{	
	XBOX::VJSObject	thisObject	= inParms.GetObject();

	thisObject.SetProperty("CONNECTING", (sLONG) VWebSocket::STATE_CONNECTING, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete);
	thisObject.SetProperty("OPEN", (sLONG) VWebSocket::STATE_OPEN, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete);
	thisObject.SetProperty("CLOSING", (sLONG) VWebSocket::STATE_CLOSING, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete);
	thisObject.SetProperty("CLOSED", (sLONG) VWebSocket::STATE_CLOSED, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete);
	thisObject.SetProperty("binaryType", XBOX::VString("buffer"));
	thisObject.SetProperty("isRemote", true, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete);
}

void VJSWebSocketObject::Finalize (const XBOX::VJSParms_finalize &inParms, VJSWebSocketObject *inWebSocket)
{
	if (inWebSocket != NULL)

		XBOX::ReleaseRefCountable<VJSWebSocketObject>(&inWebSocket);
}

void VJSWebSocketObject::GetProperty (XBOX::VJSParms_getProperty &ioParms, VJSWebSocketObject *inWebSocket)
{
	if (inWebSocket != NULL && inWebSocket->fIsNeutralized) {

		// If the object has been neutralized, then do nothing, let the interpreter handle object.
		
		ioParms.ForwardToParent();
		return;

	}

	XBOX::VString	propertyName;

	if (inWebSocket != NULL && ioParms.GetPropertyName(propertyName)) {

		if (propertyName.EqualToUSASCIICString("readyState")) 

			ioParms.ReturnNumber<sLONG>(inWebSocket->GetState());

		else if (propertyName.EqualToUSASCIICString("bufferedAmount")) 

			ioParms.ReturnNumber<sLONG>(0);	// Writes are always synchronous, so bufferedAmount is always zero.
		else 

			ioParms.ForwardToParent();

	} else 

		ioParms.ForwardToParent();
}

bool VJSWebSocketObject::SetProperty (XBOX::VJSParms_setProperty &ioParms, VJSWebSocketObject *inWebSocket)
{
	xbox_assert(inWebSocket != NULL);

	// If object has been neutralized, then prevent manipulation.

	if (inWebSocket->fIsNeutralized) {

		XBOX::vThrowError(XBOX::VE_JVSC_NEUTRALIZED_OBJECT);
		return true;	

	}

	XBOX::VString	propertyName;

	ioParms.GetPropertyName(propertyName);
	if (propertyName.EqualToString("binaryType", true)) {

		XBOX::VJSValue	value	= ioParms.GetPropertyValue();
		XBOX::VString	type;

		// We only support "blob", "buffer", and "string" for "binaryType".

		if (!value.IsString()
		|| !value.GetString(type)
		|| (!type.EqualToString("blob", true) && !type.EqualToString("buffer", true) && !type.EqualToString("string", true))) {

			XBOX::vThrowError(XBOX::VE_JVSC_W3C_SYNTAX_ERROR);

			return true;	// Do not set the property.

		} else

			return false;	

	} else 

		return false;	
}

void VJSWebSocketObject::Construct(XBOX::VJSParms_construct &ioParms, bool inIsSynchronous)
{
	// Read arguments.

	XBOX::VString	url, subProtocols;
		
	if (ioParms.CountParams() < 1 || !ioParms.GetStringParam(1, url)) {

		XBOX::vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
		return;

	}

	if (ioParms.CountParams() >= 2) {

		if (ioParms.IsStringParam(2)) {

			if (!ioParms.GetStringParam(2, subProtocols)) {

				XBOX::vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2");
				return;

			}

		} else if (ioParms.IsArrayParam(2)) {

			XBOX::VJSArray	arrayObject(ioParms.GetContext());

			if (!ioParms.GetParamArray(2, arrayObject)) {

				XBOX::vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_ARRAY, "2");
				return;

			}
			
			// A zero length (empty) array is valid.

			for (sLONG i = 0; i < arrayObject.GetLength(); i++) {

				XBOX::VJSValue	value		= arrayObject.GetValueAt(i);
				XBOX::VString	protocol;

				if (!value.IsString() || !value.GetString(protocol)) {

					XBOX::vThrowError(VE_JVSC_WRONG_PARAMETER, "2");
					return;

				} else if (subProtocols.IsEmpty())

					subProtocols = protocol;

				else {

					subProtocols.AppendCString(", ");
					subProtocols.AppendString(protocol);

				}

			}

		} else {

			XBOX::vThrowError(VE_JVSC_WRONG_PARAMETER, "2");
			return;

		}

	}

	// Construct the WebSocket or WebSocketSync and initiate opening handshake.

	VJSWebSocketObject		*webSocket;	
	XBOX::VError			error;
	StErrorContextInstaller	errorContext;	//** VHTTPClient::Init() problème avec proxy, résoudre ça ! 

	if (!XBOX::VWebSocketConnector::IsValidURL(url)) 

		XBOX::vThrowError(VE_JVSC_W3C_SYNTAX_ERROR);

	else if ((webSocket = new VJSWebSocketObject(ioParms, inIsSynchronous, VJSWorker::RetainWorker(ioParms.GetContext()))) == NULL) 

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
		
	else {

		ioParms.ReturnConstructedObject(webSocket->fObject);

		if ((error = webSocket->_Connect(url, subProtocols)) == XBOX::VE_OK) {

			webSocket->fObject.SetProperty("url", url, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete);

			// For asynchronous WebSocket objects, they will be set to empty strings, as required by spec.

			webSocket->fObject.SetProperty("protocol", webSocket->fProtocol);
			webSocket->fObject.SetProperty("extensions", webSocket->fExtensions);
		
		} else {

			// Created object "finalize" will delete webSocket object.

			XBOX::vThrowError(error);
			
		}

	}
}

void VJSWebSocketObject::Close (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebSocketObject *inWebSocket)
{
	xbox_assert(inWebSocket != NULL);

	if (inWebSocket->fIsNeutralized) {

		XBOX::vThrowError(XBOX::VE_JVSC_NEUTRALIZED_OBJECT);
		return;

	}
	
	uBYTE	reason[2 + MAXIMUM_REASON_LENGTH + ADDITIONAL_BYTES];
	VSize	reasonLength;
	
	if (ioParms.CountParams() >= 1) {

		sLONG	statusCode;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &statusCode)) {

			XBOX::vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
			return;

		} else if (!(statusCode == 1000 || (statusCode >= 3000 && statusCode <= 4999))) {

			XBOX::vThrowError(VE_JVSC_W3C_INVALID_ACCESS_ERROR);
			return;

		} else {

			reason[0] = statusCode >> 8;
			reason[1] = statusCode & 0xff;
			reasonLength = 2;

		}

		if (ioParms.CountParams() >= 2) {

			XBOX::VString	string;

			if (!ioParms.IsStringParam(2) || !ioParms.GetStringParam(2, string)) {
			
				XBOX::vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2");
				return;

			}

			reasonLength = string.ToBlock(&reason[2], MAXIMUM_REASON_LENGTH + ADDITIONAL_BYTES, VTC_UTF_8, false, false);
			if (reasonLength > MAXIMUM_REASON_LENGTH) {
			
				XBOX::vThrowError(VE_JVSC_W3C_SYNTAX_ERROR);
				return;

			} else

				reasonLength += 2;

		}

	} else {

		// Status code 1000 means "normal closure".

		reason[0] = 1000 >> 8;
		reason[1] = 1000 & 0xff;
		reasonLength = 2;

	}

	XBOX::VError	error;
	bool			isOk;

	if ((error = inWebSocket->Close(reason, reasonLength, &isOk)) != XBOX::VE_OK) 

		XBOX::vThrowError(error);

	else

		ioParms.ReturnBool(isOk);
}

void VJSWebSocketObject::Send (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebSocketObject *inWebSocket)
{
	xbox_assert(inWebSocket != NULL);

	if (inWebSocket->GetState() != XBOX::VWebSocket::STATE_OPEN)

		return;

	if (!ioParms.CountParams()) {

		XBOX::vThrowError(XBOX::VE_JVSC_EXPECTING_PARAMETER);
		return;

	}

	uBYTE	*message;
	VSize	messageLength;
	bool	isText;

	if (ioParms.IsObjectParam(1)) {

		XBOX::VJSObject	object(ioParms.GetContext());

		if (!ioParms.GetParamObject(1, object)) {

			XBOX::vThrowError(XBOX::VE_JVSC_EXPECTING_PARAMETER, "1");
			return;

		} else if (object.IsOfClass(VJSBufferClass::Class())) {

			VJSBufferObject	*bufferObject;

			bufferObject = object.GetPrivateData<VJSBufferClass>();	
			xbox_assert(bufferObject != NULL);

			message = (uBYTE *) bufferObject->GetDataPtr();
			messageLength = bufferObject->GetDataSize();
			isText = false;

		} else if (object.IsOfClass(VJSBlob::Class())) {

			VJSBlobValue	*blobValue;

			blobValue = object.GetPrivateData<VJSBlob>();
			xbox_assert(blobValue != NULL);

			VJSDataSlice	*dataSlice;

			dataSlice = blobValue->RetainDataSlice();
			message = (uBYTE *) dataSlice->GetDataPtr();
			messageLength = dataSlice->GetDataSize();
			isText = false;
			dataSlice->Release();
		
		} else {
		
			XBOX::vThrowError(XBOX::VE_JVSC_EXPECTING_PARAMETER, "1");
			return;
		
		}

	} else if (ioParms.IsStringParam(1)) {

		XBOX::VString	string;

		if (!ioParms.GetStringParam(1, string)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
			return;

		} else if ((message = new uBYTE[string.GetLength() * 2]) == NULL) {

			XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
			return;

		} else {

			messageLength = string.ToBlock(message, string.GetLength() * 2, VTC_UTF_8, false, false);
			isText = true;

		}

	} else {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER, "1");
		return;

	}
	
	inWebSocket->fWebSocket->SendMessage(message, messageLength, isText);
	if (isText)

		delete message;
}

void VJSWebSocketObject::Receive (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebSocketObject *inWebSocket)
{
	xbox_assert(inWebSocket != NULL);

	if (inWebSocket->fIsNeutralized) {

		XBOX::vThrowError(XBOX::VE_JVSC_NEUTRALIZED_OBJECT);
		return;

	}

	if (inWebSocket->GetState() != XBOX::VWebSocket::STATE_OPEN)

		return;

	XBOX::VError	error;
	bool			hasReadFrame, isComplete;

	if ((error = inWebSocket->fWebSocketMessage->Receive(&hasReadFrame, &isComplete)) != XBOX::VE_OK) 

		XBOX::vThrowError(error);

	else if (!isComplete) 

		ioParms.ReturnNullValue();
				
	else {

		XBOX::VJSValue	value		= ioParms.GetObject().GetProperty("binaryType");
		XBOX::VString	binaryType;
		bool			isString;

		isString = value.IsString() && value.GetString(binaryType) && binaryType.EqualToString("string");

		uBYTE	*data;
		VSize	size;

		data = inWebSocket->fWebSocketMessage->GetMessage();
		size = inWebSocket->fWebSocketMessage->GetSize();
		if (isString) {

			XBOX::VString	text;

			text.FromBlock(data, size, VTC_UTF_8);
			ioParms.ReturnString(text);

		} else if (binaryType.EqualToString("blob")) {

			// Data will be copied.

			ioParms.ReturnValue(VJSBlob::NewInstance(ioParms.GetContext(), data, size, ""));
			
		} else {

			// binaryType == "buffer"

			inWebSocket->fWebSocketMessage->Steal();
			ioParms.ReturnValue(VJSBufferClass::NewInstance(ioParms.GetContext(), size, data, true));

		}

	}
}

sLONG VJSWebSocketObject::GetState () const
{
	if (fWebSocketConnector != NULL)

		return XBOX::VWebSocket::STATE_CONNECTING;

	else if (fWebSocket == NULL) 

		return XBOX::VWebSocket::STATE_CLOSED;

	else

		return fWebSocket->GetState();
}

VJSWebSocketObject *VJSWebSocketObject::RetainAndNeutralize (XBOX::VJSObject &inGlobalObject, XBOX::VJSWorker *inWorker)
{
	xbox_assert(inWorker != NULL);

	xbox_assert(!fIsNeutralized);
	xbox_assert(!fIsDedicatedWorker);
	
	fIsNeutralized = true;
	fIsDedicatedWorker = true;
	XBOX::ReleaseRefCountable<VJSWorker>(&fWorker);
	fWorker = XBOX::RetainRefCountable<VJSWorker>(inWorker);
	fObject = inGlobalObject;

	return XBOX::RetainRefCountable<VJSWebSocketObject>(this);
}

XBOX::VError VJSWebSocketObject::Close (const uBYTE *inReason, VSize inReasonLength, bool *outIsOk)
{
	xbox_assert(inReason != NULL && inReasonLength <= MAXIMUM_REASON_LENGTH && outIsOk != NULL);

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);
 
	sLONG	state;

	*outIsOk = true;
	if ((state = GetState()) == XBOX::VWebSocket::STATE_CLOSING || state == XBOX::VWebSocket::STATE_CLOSED)
		
		return XBOX::VE_OK;

	else if (state == XBOX::VWebSocket::STATE_CONNECTING) {
	
		if (!fIsSynchronous)

			fWebSocket->GetEndPoint()->DisableReadCallback();		

		fWebSocket->ForceClose();
		if (!fIsSynchronous) {

			// Spec says to "fail the WebSocket connection".

			fWorker->QueueEvent(VJSWebSocketEvent::CreateErrorEvent(this));

			sLONG			code;
			XBOX::VString	reason;

			if (inReasonLength >= 2) {

				code = inReason[1];
				code |= inReason[0] << 8;

			} else

				code = 1000;	// Default is 1000 (normal termination).

			if (inReasonLength > 2) 

				reason.FromBlock(inReason + 2, inReasonLength - 2, VTC_UTF_8);

			fWorker->QueueEvent(VJSWebSocketEvent::CreateCloseEvent(this, false, code, reason));

		}
		return XBOX::VE_OK;

	} else {

		XBOX::VError	error;
		
		if ((error = fWebSocket->SendClose(0, inReason, inReasonLength)) == XBOX::VE_OK && fIsSynchronous) {

			// Executed only if sending successful and synchronous.

			do {

				bool						hasReadFrame, isComplete;
				const XBOX::VWebSocketFrame	*frame;

				if ((error = fWebSocketMessage->Receive(&hasReadFrame, &isComplete)) == XBOX::VE_OK) {

					xbox_assert(hasReadFrame);
					if (hasReadFrame) {

						// Check the connection close answer frame.

						frame = fWebSocket->GetLastFrame();
						if (frame->GetOpcode() == XBOX::VWebSocketFrame::OPCODE_CONNECTION_CLOSE)

							*outIsOk = frame->GetPayloadLength() == inReasonLength
							&& !::memcmp(inReason, frame->GetPayloadData(), inReasonLength);

					}

				} else 

					break;

			} while (GetState() != XBOX::VWebSocket::STATE_CLOSED);

		}

		return error;

	}	
}

XBOX::VError VJSWebSocketObject::_Connect (const XBOX::VString &inURL, const XBOX::VString &inSubProtocols)
{
	XBOX::VError	error;
 
	if ((fWebSocketConnector = new XBOX::VWebSocketConnector(inURL)) == NULL)

		error = XBOX::VE_MEMORY_FULL;

	else if (fIsSynchronous) {
		
		if ((error = fWebSocketConnector->StartOpeningHandshake(NULL)) == XBOX::VE_OK
		&& (error = fWebSocketConnector->ContinueOpeningHandshake()) == XBOX::VE_OK) {

			XBOX::VTCPEndPoint	*endPoint;

			endPoint = fWebSocketConnector->StealEndPoint();
			xbox_assert(endPoint != NULL);
			if ((fWebSocket = new XBOX::VWebSocket(endPoint, true, fWebSocketConnector->GetLeftOverDataPtr(), fWebSocketConnector->GetLeftOverSize())) == NULL)

				error = XBOX::VE_MEMORY_FULL;

			else if ((fWebSocketMessage = new XBOX::VWebSocketMessage(fWebSocket)) == NULL) {
			
				delete fWebSocket;
				fWebSocket = NULL;
				error = XBOX::VE_MEMORY_FULL;
			
			} else {

				fWebSocketConnector->GetSubProtocol(&fProtocol);
				fWebSocketConnector->GetSubExtensions(&fExtensions);

				XBOX::ReleaseRefCountable<XBOX::VTCPEndPoint>(&endPoint);	// Already retained by VWebSocket constructor.
				error = XBOX::VE_OK; 

			}

			delete fWebSocketConnector;
			fWebSocketConnector = NULL;

		}

	} else {

		XBOX::VTCPSelectIOPool	*selectIOPool;

		if ((selectIOPool = XBOX::VJSNetSocketObject::GetSelectIOPool()) != NULL) {

			if ((error = fWebSocketConnector->StartOpeningHandshake(selectIOPool)) == XBOX::VE_OK) {

				XBOX::VTCPEndPoint	*endPoint;

				endPoint = fWebSocketConnector->GetEndPoint();
				xbox_assert(endPoint != NULL);
				if ((fWebSocket = new XBOX::VWebSocket(endPoint, false, fWebSocketConnector->GetLeftOverDataPtr(), fWebSocketConnector->GetLeftOverSize())) != NULL
				&& (fWebSocketMessage = new XBOX::VWebSocketMessage(fWebSocket)) != NULL) {

					endPoint->SetNoDelay(false);
					endPoint->SetReadCallback(ReadCallback, this);

					// Connection continues asynchronously.

				} else {

					error = XBOX::VE_MEMORY_FULL;

					if (fWebSocket != NULL) {

						delete fWebSocket;
						fWebSocket = NULL;

					}

					delete fWebSocketConnector;
					fWebSocketConnector = NULL;

				}

			}

		} else {
	
			xbox_assert(false);
			error = VE_SRVR_INVALID_INTERNAL_STATE;

		}
	
	}

	return error;
}

bool VJSWebSocketObject::ReadCallback (Socket inRawSocket, VEndPoint* inEndPoint, void *inData, sLONG inErrorCode)
{
	xbox_assert(inEndPoint != NULL && inData != NULL);

	if (inEndPoint == NULL || inErrorCode)

		return false;
		
	bool									isOk;
	VJSWebSocketObject						*webSocket					= (VJSWebSocketObject *) inData;	
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&webSocket->fMutex);
	StErrorContextInstaller					errorContext;				// Silence error throw.
	
	switch (webSocket->GetState()) {

	case XBOX::VWebSocket::STATE_CONNECTING: {

		XBOX::VError	error;
		char			buffer[BUFFER_SIZE];
		uLONG			length;

		length = BUFFER_SIZE;
		if ((error = webSocket->fWebSocket->GetEndPoint()->DirectSocketRead(buffer, &length)) != XBOX::VE_OK) 

			isOk = error == XBOX::VE_SOCK_WOULD_BLOCK ? true : false;	// SSL may require more data.
			
		else if ((error = webSocket->fWebSocketConnector->ContinueOpeningHandshake(buffer, length)) == XBOX::VE_OK) {

			// Successful opening handshake, issue an "open" event and free WebSocket connector.

			webSocket->fWebSocketConnector->GetSubProtocol(&webSocket->fProtocol);
			webSocket->fWebSocketConnector->GetSubExtensions(&webSocket->fExtensions);
			webSocket->fWorker->QueueEvent(VJSWebSocketEvent::CreateOpenEvent(webSocket, webSocket->fProtocol, webSocket->fExtensions));

			XBOX::VTCPEndPoint	*endPoint;

			endPoint = webSocket->fWebSocketConnector->StealEndPoint();
			XBOX::ReleaseRefCountable<XBOX::VTCPEndPoint>(&endPoint);	// Already retained by VWebSocket constructor.

			delete webSocket->fWebSocketConnector;
			webSocket->fWebSocketConnector = NULL;

			isOk = true;

		} else if (error == VE_SRVR_WEBSOCKET_OPENING_HANDSHAKE_NEED_DATA) 

			isOk = true;	// Response isn't fully read yet.

		else 

			isOk = false;	// Response parsing failed.

		if (!isOk) {
			
			// Opening handshake failed.
			
			XBOX::VString	emptyString;

			webSocket->fWorker->QueueEvent(VJSWebSocketEvent::CreateErrorEvent(webSocket));
			webSocket->fWorker->QueueEvent(VJSWebSocketEvent::CreateCloseEvent(webSocket, false, 1002, emptyString));

		}

		break;

	}

	case XBOX::VWebSocket::STATE_OPEN:
	case XBOX::VWebSocket::STATE_CLOSING: {

		XBOX::VError	error;
		bool			hasReadFrame, isComplete;
				
		error = webSocket->fWebSocketMessage->Receive(&hasReadFrame, &isComplete);
		if (error == VE_SOCK_WOULD_BLOCK
		|| error == VE_SRVR_WEBSOCKET_FRAME_HEADER_TOO_SHORT 
		|| error == VE_SRVR_WEBSOCKET_FRAME_TOO_SHORT) {

			// If error is VE_SOCK_WOULD_BLOCK, then SSL may transfer data but no user data is available for read.
			// Otherwise data is needed for a frame to be decoded.

			isOk = true;
			
		} else if (isComplete) {

			bool	isText;
			uBYTE	*data;
			VSize	size;

			isText = webSocket->fWebSocketMessage->IsText();
			data = webSocket->fWebSocketMessage->GetMessage();
			size = webSocket->fWebSocketMessage->GetSize();
			webSocket->fWebSocketMessage->Steal();

			webSocket->fWorker->QueueEvent(VJSWebSocketEvent::CreateMessageEvent(webSocket, isText, data, size));

			isOk = true;			

		} else if (hasReadFrame) {

			const XBOX::VWebSocketFrame	*frame;

			frame = webSocket->fWebSocket->GetLastFrame();
			if (webSocket->GetState() == XBOX::VWebSocket::STATE_CLOSED
			&& frame->GetOpcode() == XBOX::VWebSocketFrame::OPCODE_CONNECTION_CLOSE) {

				// Close connection acknowledgement sent or received, issue a close event.

				const uBYTE		*reason	= frame->GetPayloadData();
				VSize			length	= frame->GetPayloadLength();
				sLONG			code;
				XBOX::VString	string;
				
				if (length >= 2) {

					code = reason[0] << 8;
					code |= reason[1];

				} else

					code = 1000;
				
				if (length >= 3) 

					string.FromBlock(&reason[2], length - 2, VTC_UTF_8);

				webSocket->fWorker->QueueEvent(VJSWebSocketEvent::CreateCloseEvent(webSocket, true, code, string));
				isOk = false;	// Ignore further data.

			} else {

				// Fragmented frames are handled by fWebSocketMessage, ignore all control frame(s).

				isOk = true;

			}

		} else {

			// A frame must have been read.

			XBOX::VString	emptyString;

			webSocket->fWorker->QueueEvent(VJSWebSocketEvent::CreateErrorEvent(webSocket));
			webSocket->fWorker->QueueEvent(VJSWebSocketEvent::CreateCloseEvent(webSocket, false, 1002, emptyString));

			isOk = false;

		}

		break;

	} 
	
	case XBOX::VWebSocket::STATE_CLOSED:

		isOk = false;	// Ignore any further data received.
		break;		

	default:

		xbox_assert(false);
		isOk = false;
		break;

	}

	return isOk;
}

void VJSWebSocketClass::GetDefinition (ClassDefinition &outDefinition)
{
	static XBOX::VJSClass<VJSWebSocketClass, VJSWebSocketObject>::StaticFunction functions[] =
	{
		{	"close",				js_callStaticFunction<VJSWebSocketObject::Close>,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"send",					js_callStaticFunction<VJSWebSocketObject::Send>,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"postMessage",			js_callStaticFunction<VJSWebSocketObject::Send>,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"addEventListener",		js_callStaticFunction<_AddEventListener>,			JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"removeEventListener",	js_callStaticFunction<_RemoveEventListener>,		JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	0,						0,													0																	},
	};

    outDefinition.className			= "WebSocket";
	outDefinition.staticFunctions	= functions;
    outDefinition.initialize		= js_initialize<VJSWebSocketObject::Initialize>;
	outDefinition.finalize			= js_finalize<VJSWebSocketObject::Finalize>;
	outDefinition.getProperty		= js_getProperty<VJSWebSocketObject::GetProperty>;
	outDefinition.setProperty		= js_setProperty<VJSWebSocketObject::SetProperty>;
}

XBOX::VJSObject	VJSWebSocketClass::MakeConstructor (const XBOX::VJSContext &inContext)
{
	XBOX::VJSObject	wsConstructor =
		JS4DMakeConstructor(inContext, Class(), _Construct, false, NULL);

	return wsConstructor;

}

XBOX::VJSValue VJSWebSocketClass::MakeObject (XBOX::VJSContext &inContext, XBOX::VWebSocket *inWebSocket, VJSWorker *inWorker, bool inSetSelectIO)
{
	XBOX::VJSValue			value(inContext);
	XBOX::VTCPSelectIOPool	*selectIOPool;
	VJSWebSocketObject		*webSocket;

	value.SetNull();
	if ((selectIOPool = XBOX::VJSNetSocketObject::GetSelectIOPool()) != NULL
	&& (webSocket = new VJSWebSocketObject(false, inWorker,inContext)) != NULL) {
		
		XBOX::VWebSocketMessage	*webSocketMessage;

		if ((webSocketMessage = new XBOX::VWebSocketMessage(inWebSocket)) != NULL) {

			webSocket->fWebSocket = inWebSocket;
			webSocket->fWebSocketMessage = webSocketMessage;

			value = webSocket->fObject;

			XBOX::VTCPEndPoint	*endPoint;

			endPoint = inWebSocket->GetEndPoint();
			xbox_assert(endPoint != NULL);

			if (inSetSelectIO) {
 
				endPoint->SetIsSelectIO(true);
				endPoint->SetSelectIOPool(selectIOPool);

			}
			endPoint->SetNoDelay(false);
			endPoint->SetReadCallback(VJSWebSocketObject::ReadCallback, webSocket);

		} else

			XBOX::ReleaseRefCountable<VJSWebSocketObject>(&webSocket);

	}

	return value;
}

void VJSWebSocketClass::_Construct(XBOX::VJSParms_construct &ioParms)
{
	VJSWebSocketObject::Construct(ioParms, false);
}

void VJSWebSocketClass::_AddEventListener (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebSocketObject *inWebSocket)
{
     xbox_assert(inWebSocket != NULL);

	if (inWebSocket->fIsNeutralized) {

		XBOX::vThrowError(XBOX::VE_JVSC_NEUTRALIZED_OBJECT);
		return;

	}

	XBOX::VString	eventName;

	if (ioParms.CountParams() < 1 || !ioParms.IsStringParam(1) || !ioParms.GetStringParam(1, eventName)) 

		XBOX::vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
		
	else if (ioParms.CountParams() < 2)

		XBOX::vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "2");

	else {

		XBOX::VString	propertyName;
	
		propertyName.AppendString("on");
		propertyName.AppendString(eventName);
		ioParms.GetThis().SetProperty(propertyName, ioParms.GetParamValue(2));

	}
}

void VJSWebSocketClass::_RemoveEventListener (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebSocketObject *inWebSocket)
{
	xbox_assert(inWebSocket != NULL);

	if (inWebSocket->fIsNeutralized) {

		XBOX::vThrowError(XBOX::VE_JVSC_NEUTRALIZED_OBJECT);
		return;

	}

	XBOX::VString	eventName;

	if (ioParms.CountParams() < 1 || !ioParms.IsStringParam(1) || !ioParms.GetStringParam(1, eventName)) 

		XBOX::vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");

	else {

		XBOX::VString	propertyName;
	
		propertyName.AppendString("on");
		propertyName.AppendString(eventName);
		if (ioParms.GetThis().HasProperty(propertyName)) {

			XBOX::VJSException	exception;

			ioParms.GetThis().DeleteProperty(propertyName, exception);

		}

	}
}

void VJSWebSocketSyncClass::GetDefinition (ClassDefinition &outDefinition)
{
	static XBOX::VJSClass<VJSWebSocketClass, VJSWebSocketObject>::StaticFunction functions[] =
	{
		{	"close",	js_callStaticFunction<VJSWebSocketObject::Close>,		JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"send",		js_callStaticFunction<VJSWebSocketObject::Send>,		JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"receive",	js_callStaticFunction<VJSWebSocketObject::Receive>,		JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},		
		{	0,			0,														0																	},
	};

    outDefinition.className			= "WebSocketSync";
	outDefinition.staticFunctions	= functions;
    outDefinition.initialize		= js_initialize<VJSWebSocketObject::Initialize>;
	outDefinition.finalize			= js_finalize<VJSWebSocketObject::Finalize>;
	outDefinition.getProperty		= js_getProperty<VJSWebSocketObject::GetProperty>;
	outDefinition.setProperty		= js_setProperty<VJSWebSocketObject::SetProperty>;
}

XBOX::VJSObject	VJSWebSocketSyncClass::MakeConstructor (const XBOX::VJSContext &inContext)
{
	XBOX::VJSObject	wsSyncConstructor =
		JS4DMakeConstructor(inContext, Class(), _Construct, false, NULL);

	return wsSyncConstructor;

}

void VJSWebSocketSyncClass::_Construct(XBOX::VJSParms_construct &ioParms)
{
	VJSWebSocketObject::Construct(ioParms, true);
}

void VJSWebSocketHandler::AddHandler (XBOX::VJSParms_callStaticFunction &ioParms, void *)
{
	XBOX::VString	path, scriptURL, id;
	bool			isSharedWorker;

	if (!ioParms.GetStringParam(1, path)) 

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");

	else if (!ioParms.GetStringParam(2, scriptURL))  

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2");

	else if (!ioParms.GetStringParam(3, id)) 

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "3");

	else if (!ioParms.GetBoolParam(4, &isSharedWorker))

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "4");

	else {

		XBOX::VTaskLock			lock(&sMutex);
		HandlerMap::iterator	i;

		i = sHandlers.find(path);
		if (i != sHandlers.end())

			XBOX::vThrowError(XBOX::VE_JVSC_FORBIDDEN);	//** Replace with correct error => path already registered

		else {

			XBOX::VJSContext			context	= ioParms.GetContext();
			VJSWorker					*worker	= VJSWorker::RetainWorker(context);
			VJSWebSocketHandler			*webSocketHandler;

			xbox_assert(worker != NULL);

			webSocketHandler = new VJSWebSocketHandler(path, id, scriptURL, isSharedWorker);
			if (webSocketHandler == NULL)

				XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

			else {
				
				XBOX::VJSGlobalObject		*globalObject		= context.GetGlobalObjectPrivateInstance();
				XBOX::IJSRuntimeDelegate	*runtimeDelegate	= globalObject->GetRuntimeDelegate();
				XBOX::VError				error;

				xbox_assert(runtimeDelegate != NULL);

				error = runtimeDelegate->AddWebSocketHandler(context, path, VJSWebSocketHandler::ConnectionHandler, webSocketHandler);
				if (error != XBOX::VE_OK) 

					XBOX::vThrowError(error);

				else

					sHandlers[path] = webSocketHandler;
					
				XBOX::ReleaseRefCountable<VJSWebSocketHandler>(&webSocketHandler);

			}

			XBOX::ReleaseRefCountable<VJSWorker>(&worker);

		}

	}
}

void VJSWebSocketHandler::RemoveHandler (XBOX::VJSParms_callStaticFunction &ioParms, void *)
{
	XBOX::VString	path;

	if (!ioParms.CountParams() || !ioParms.GetStringParam(1, path)) 

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
			
	else {

		XBOX::VTaskLock			lock(&sMutex);
		HandlerMap::iterator	i;

		i = sHandlers.find(path);
		if (i == sHandlers.end())

			XBOX::vThrowError(XBOX::VE_JVSC_FORBIDDEN);	//** Replace with correct error => path not found 

		else {

			XBOX::VJSContext			context				= ioParms.GetContext();
			XBOX::VJSGlobalObject		*globalObject		= context.GetGlobalObjectPrivateInstance();
			XBOX::IJSRuntimeDelegate	*runtimeDelegate	= globalObject->GetRuntimeDelegate();
			XBOX::VError				error;

			xbox_assert(runtimeDelegate != NULL);

			error = runtimeDelegate->RemoveWebSocketHandler(context, path);
			sHandlers.erase(i);
			if (error != XBOX::VE_OK)

				XBOX::vThrowError(error);

		}
		
	}
}

void VJSWebSocketHandler::RemoveAllhandlers ()
{
	XBOX::VTaskLock	lock(&sMutex);

	sHandlers.clear();
}

XBOX::VError VJSWebSocketHandler::ConnectionHandler (XBOX::VJSContext &inContext, XBOX::VWebSocket *inWebSocket, void *inUserData)
{
	xbox_assert(inWebSocket != NULL && inUserData != NULL);

	return ((VJSWebSocketHandler *) inUserData)->_ConnectionHandler(inContext, inWebSocket);
}

VJSWebSocketHandler::VJSWebSocketHandler (
	const XBOX::VString &inPath, 
	const XBOX::VString &inId,										
	const XBOX::VString &inScriptURL,
	bool inIsSharedWorker)
{
	xbox_assert(!inPath.IsEmpty() && !inId.IsEmpty() && !inScriptURL.IsEmpty());

	fPath = inPath;
	fId = inId;
	fScriptURL = inScriptURL;
	fIsSharedWorker = inIsSharedWorker;
}

VJSWebSocketHandler::~VJSWebSocketHandler ()
{	
}

XBOX::VError VJSWebSocketHandler::_ConnectionHandler (XBOX::VJSContext &inContext, XBOX::VWebSocket *inWebSocket)
{
	xbox_assert(inWebSocket != NULL);

	XBOX::VError	error;

	if (fIsSharedWorker) {

		VJSWorker	*sharedWorker;
		bool		needToRun;
		
		sharedWorker = VJSWorker::RetainSharedWorker(inContext, NULL, fScriptURL, fId, false, &needToRun);
		if (sharedWorker == NULL) 

			error = XBOX::VE_MEMORY_FULL;
				
		else {

			if (needToRun)

				sharedWorker->Run();

			sharedWorker->ConnectWebSocket(inWebSocket);
			XBOX::ReleaseRefCountable<VJSWorker>(&sharedWorker);
			error = XBOX::VE_OK;

		}

	} else {

		XBOX::VWebSocketMessage	*webSocketMessage;
	
		webSocketMessage = new XBOX::VWebSocketMessage(inWebSocket);
		if (webSocketMessage == NULL)

			error = XBOX::VE_MEMORY_FULL;

		else {

			VJSWebSocketObject	*webSocketObject;
		
			webSocketObject = new VJSWebSocketObject(inWebSocket, webSocketMessage, inContext);
			if (webSocketObject == NULL) {

				delete webSocketMessage;
				error = XBOX::VE_MEMORY_FULL;

			} else {

				VJSWorker	*dedicatedWorker;

				dedicatedWorker = new VJSWorker(inContext, NULL, fScriptURL, false, webSocketObject);
				if (dedicatedWorker == NULL) 

					error = XBOX::VE_MEMORY_FULL;
				
				else {

					XBOX::VTCPEndPoint	*endPoint;

					endPoint = inWebSocket->GetEndPoint();
					xbox_assert(endPoint != NULL);

					endPoint->SetNoDelay(false);
					endPoint->SetReadCallback(VJSWebSocketObject::ReadCallback, webSocketObject);

					dedicatedWorker->Run();

					XBOX::ReleaseRefCountable<VJSWorker>(&dedicatedWorker);
					error = XBOX::VE_OK;

				}

				XBOX::ReleaseRefCountable<VJSWebSocketObject>(&webSocketObject);

			}

		}

	}

	if (error != XBOX::VE_OK)

		delete inWebSocket;

	return error;
}

VJSWebSocketServerObject::VJSWebSocketServerObject (const XBOX::VJSContext& inContext, 
	bool inIsSSL, 
	XBOX::VString &inAddress, sLONG inPort, const XBOX::VString &inPath)
:fObject(inContext)
{
	fState = STATE_NOT_READY;
	
	fListener = new XBOX::VWebSocketListener(inIsSSL);
	fListener->SetAddressAndPort(inAddress, inPort);
	fListener->SetPath(inPath);

	fListeningTask = NULL;
	fWorker = NULL;
	fObject = VJSWebSocketServerClass::CreateInstance(inContext, this );
}

VJSWebSocketServerObject::~VJSWebSocketServerObject ()
{
	if (fListeningTask != NULL) {

		_KillListeningTask();
		XBOX::ReleaseRefCountable<XBOX::VTask>(&fListeningTask);

	}

	if (fListener != NULL)

		delete fListener;

	if (fWorker != NULL)

		XBOX::ReleaseRefCountable<VJSWorker>(&fWorker);
}

sLONG VJSWebSocketServerObject::_RunProc (XBOX::VTask *inVTask)
{
	((VJSWebSocketServerObject *) inVTask->GetKindData())->_DoRun();
	return 0;
}

void VJSWebSocketServerObject::_DoRun ()
{
	fSyncEvent.Lock();	// Call to start() will unlock.

	XBOX::RetainRefCountable<VJSWorker>(fWorker);
	
	XBOX::VError	error;

	error = XBOX::VE_OK;
	if (fListeningTask->GetState() != TS_DYING) {
		
		if ((error = fListener->StartListening()) == XBOX::VE_OK) {

			fState = VJSWebSocketServerObject::STATE_RUNNING;
			while (fListeningTask->GetState() != TS_DYING) {

				XBOX::VTCPEndPoint		*endPoint;
				XBOX::VMemoryBuffer<>	memoryBuffer;
				
				if ((error = fListener->AcceptConnection(&endPoint, sTIME_OUT, &memoryBuffer)) == XBOX::VE_OK) {

					xbox_assert(endPoint != NULL);
			
					XBOX::VWebSocket	*webSocket;
	
					if ((webSocket = new XBOX::VWebSocket(endPoint, false, memoryBuffer.GetDataPtr(), memoryBuffer.GetDataSize(), false)) == NULL) {

						error = XBOX::VE_MEMORY_FULL;
						XBOX::ReleaseRefCountable<XBOX::VTCPEndPoint>(&endPoint);
						break;

					} else

						fWorker->QueueEvent(VJSWebSocketServerEvent::CreateConnectionEvent(fObject, webSocket));

				} else if (error == VE_SOCK_TIMED_OUT)

					error = XBOX::VE_OK;

				else

					break;

			}

		}

	}

	if (error != XBOX::VE_OK) 

		fWorker->QueueEvent(VJSWebSocketServerEvent::CreateErrorEvent(fObject));

	fWorker->QueueEvent(VJSWebSocketServerEvent::CreateCloseEvent(fObject));
	fListener->Close();
	fState = STATE_CLOSED;

	XBOX::ReleaseRefCountable<VJSWorker>(&fWorker);

	Release();
}

void VJSWebSocketServerObject::_KillListeningTask ()
{
	switch (fState) {

	case STATE_NOT_READY:	
		
		break;

	case STATE_READY:

		fListeningTask->Kill();
		fSyncEvent.Unlock();
		break;

	case STATE_RUNNING:

		fListeningTask->Kill();
		break;

	case STATE_CLOSED:	

		break;

	default:

		xbox_assert(false);
		break;

	}
}

void VJSWebSocketServerClass::GetDefinition (ClassDefinition &outDefinition)
{
	static XBOX::VJSClass<VJSWebSocketServerClass, VJSWebSocketServerObject>::StaticFunction functions[] =
	{
		{	"start",	js_callStaticFunction<_Start>,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"close",	js_callStaticFunction<_Close>,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},		
		{	0,			0,								0																	},
	};

    outDefinition.className			= "WebSocketServer";
	outDefinition.staticFunctions	= functions;
    outDefinition.initialize		= js_initialize<_Initialize>;
	outDefinition.finalize			= js_finalize<_Finalize>;
}

XBOX::VJSObject VJSWebSocketServerClass::MakeConstructor (const XBOX::VJSContext &inContext)
{
	XBOX::VJSObject	wsServerConstructor =
		JS4DMakeConstructor(inContext, Class(), _Construct, false, NULL);

	return wsServerConstructor;
}

void VJSWebSocketServerClass::_Construct(XBOX::VJSParms_construct &ioParms)
{
	bool			isSSL	= false;	//**
	sLONG			port;
	XBOX::VString	address, path;

	// Only port number is mandatory.

	if (ioParms.CountParams() < 1 || !ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &port)) {

		XBOX::vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
		return;

	}

	address = "127.0.0.1";
	if (ioParms.CountParams() >= 2 && (!ioParms.IsStringParam(2) || !ioParms.GetStringParam(2, address))) {

		XBOX::vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2");
		return;

	} 

	if (ioParms.CountParams() >= 3 && (!ioParms.IsStringParam(3) || !ioParms.GetStringParam(3, path))) {

		XBOX::vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2");
		return;

	}

	// Allocate WebSocket state object.

	VJSWebSocketServerObject	*webSocketServer;

	if ((webSocketServer = new VJSWebSocketServerObject(ioParms.GetContext(), isSSL, address, port, path)) == NULL) {

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
		return;

	}

//**
/*	
	if (isSSL) 

		webSocketServer->SetKeyAndCertificate (const XBOX::VString inKey, const XBOX::VString &inCertificate);

*/

	webSocketServer->fListeningTask = new XBOX::VTask(NULL, 0, XBOX::eTaskStylePreemptive, VJSWebSocketServerObject::_RunProc);
	if (webSocketServer->fListeningTask == NULL) {

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
		delete webSocketServer;
		return;

	}
	webSocketServer->fListeningTask->SetKindData((sLONG_PTR) webSocketServer);
	if (!webSocketServer->fListeningTask->Run()) {

		XBOX::vThrowError(VE_JVSC_WEBSOCKET_FAILED_LAUNCH);
		delete webSocketServer;
		return;

	} else {

		webSocketServer->Retain();
		webSocketServer->fState = VJSWebSocketServerObject::STATE_READY;

	}

	webSocketServer->fWorker = VJSWorker::RetainWorker(ioParms.GetContext());
	xbox_assert(webSocketServer->fWorker != NULL);

	// Allocate constructed object and return it.

	ioParms.ReturnConstructedObject(webSocketServer->fObject);
}

void VJSWebSocketServerClass::_Initialize (const XBOX::VJSParms_initialize &inParms, VJSWebSocketServerObject *inWebSocketServer)
{
	if (inWebSocketServer != NULL) {

		XBOX::VJSObject	thisObject	= inParms.GetObject();

		thisObject.SetProperty("port", inWebSocketServer->fListener->GetPort());
		thisObject.SetProperty("address", inWebSocketServer->fListener->GetAddress());
		thisObject.SetProperty("path", inWebSocketServer->fListener->GetPath());

	}
}

void VJSWebSocketServerClass::_Finalize (const XBOX::VJSParms_finalize &inParms, VJSWebSocketServerObject *inWebSocketServer)
{
	if (inWebSocketServer != NULL)

		inWebSocketServer->Release();
}

void VJSWebSocketServerClass::_Start (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebSocketServerObject *inWebSocketServer)
{
	if (inWebSocketServer == NULL || inWebSocketServer->fState == VJSWebSocketServerObject::STATE_CLOSED)

		return;

	if (inWebSocketServer->fState != VJSWebSocketServerObject::STATE_READY) 

		XBOX::vThrowError(VE_JVSC_WEBSOCKET_ALREADY_STARTED);

	else

		inWebSocketServer->fSyncEvent.Unlock();
}

void VJSWebSocketServerClass::_Close (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebSocketServerObject *inWebSocketServer)
{
	// It is even possible to "close" a not started listener.

	if (inWebSocketServer != NULL)

		inWebSocketServer->_KillListeningTask();
}
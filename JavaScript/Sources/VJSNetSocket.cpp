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

#include "VJSNet.h"
#include "VJSNetSocket.h"

#include "VJSContext.h"
#include "VJSGlobalClass.h"

#include "VJSBuffer.h"
#include "VJSEvent.h"
#include "VJSEventEmitter.h"

USING_TOOLBOX_NAMESPACE

#define WRITE_SLICE_SIZE	(1 << 15)

XBOX::VCriticalSection	VJSNetSocketObject::sMutex;
XBOX::VTCPSelectIOPool	*VJSNetSocketObject::sSelectIOPool	= NULL;

void VJSNetSocketObject::ForceClose ()
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	if (fEndPoint != NULL) {

		if (!fIsSynchronous)

			fEndPoint->DisableReadCallback();

		fEndPoint->Close();
		ReleaseRefCountable(&fEndPoint);

	}

	std::list<SPacket>::iterator	i;

	for (i = fBufferedData.begin(); i != fBufferedData.end(); i++) {

		xbox_assert(i->fBuffer != NULL);
		::free(i->fBuffer);

	}

	fBufferedData.clear();
}

VJSNetSocketObject::VJSNetSocketObject (bool inIsSynchronous, sLONG inType, bool inAllowHalfOpen,const VJSContext& inContext)
:fObject(inContext)
{
	xbox_assert(inType == eTYPE_TCP4 || inType == eTYPE_TCP6);

	fType = inType;
	fEndPoint = NULL;
	fIsSynchronous = inIsSynchronous;
	fAllowHalfOpen = inAllowHalfOpen;
	fTimeOut = 0;	
	fEncoding = XBOX::VTC_UNKNOWN;	

	fWorker = NULL;

	fIsPaused = false;
	fBufferedData.clear();

	fBytesRead = fBytesWritten = 0;
}

VJSNetSocketObject::~VJSNetSocketObject ()
{
	xbox_assert(fEndPoint == NULL);
	xbox_assert(fBufferedData.empty());

	if (fWorker != NULL)

		XBOX::ReleaseRefCountable<VJSWorker>(&fWorker);
}

XBOX::VTCPSelectIOPool *VJSNetSocketObject::GetSelectIOPool () 
{
	XBOX::VTCPSelectIOPool	*selectIOPool;

	VJSNetSocketObject::sMutex.Lock();

	if (VJSNetSocketObject::sSelectIOPool == NULL)

		VJSNetSocketObject::sSelectIOPool = new XBOX::VTCPSelectIOPool();

	selectIOPool = VJSNetSocketObject::sSelectIOPool;

	VJSNetSocketObject::sMutex.Unlock();

	xbox_assert(selectIOPool != NULL);

	return selectIOPool;
}

bool VJSNetSocketObject::_ReadCallback (Socket inRawSocket, VEndPoint* inEndPoint, void *inData, sLONG inErrorCode)
{
	xbox_assert(inEndPoint != NULL && inData != NULL);
	
	VJSNetSocketObject	*netSocketObject;

	netSocketObject = (VJSNetSocketObject *) inData;

	// net.Socket object may have been closed just before _ReadCallback() is called. 
	// NULL pointer check will detect that, but object has always been released.
	// Need more robust way.

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&netSocketObject->fMutex);

	if (netSocketObject->fEndPoint == NULL)

		return false;

	xbox_assert(netSocketObject->fEndPoint == (XBOX::VTCPEndPoint *) inEndPoint);

	if (!inErrorCode)

		return netSocketObject->_ReadSocket();

	else

		return false;
}

bool VJSNetSocketObject::_ReadSocket ()
{
	uBYTE			*buffer;
	uLONG			length;
	XBOX::VError	error;

	if ((buffer = (uBYTE *) ::malloc(VJSNetSocketObject::kReadBufferSize)) == NULL) {

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
		return false;

	}
	length = VJSNetSocketObject::kReadBufferSize;

	XBOX::VErrorTaskContext	*taskErrorContext;	
	XBOX::VErrorContext		*context;

	taskErrorContext = XBOX::VTask::GetErrorContext(true);
	xbox_assert(taskErrorContext != NULL);

	context = taskErrorContext->PushNewContext(true, true);
	xbox_assert(context != NULL);

	error = fEndPoint->DirectSocketRead(buffer, &length);	
	xbox_assert(!(error != VE_SOCK_WOULD_BLOCK && error != VE_SOCK_PEER_OVER && context->GetLastError() != error));

	taskErrorContext->PopContext();
	ReleaseRefCountable(&context);

	if (error == XBOX::VE_SOCK_PEER_OVER) {

		xbox_assert(!length);
		error = XBOX::VE_OK;

	}

	bool	isOk;

	if (error != XBOX::VE_OK)  {

		::free(buffer);
		isOk = false;

		if (error == XBOX::VE_SOCK_WOULD_BLOCK) {

			// An SSL_read() without enough data, will return an XBOX::VE_SOCK_WOULD_BLOCK error. 
			// This is not actually an "error", the read will just have to be retried later.

			isOk = true;

		} else if (error == XBOX::VE_SRVR_CONNECTION_BROKEN) {

			// Peer has closed socket.
			
			fWorker->QueueEvent(VJSNetEvent::CreateClose(this, false));
	
		} else {
			
 			fWorker->QueueEvent(VJSNetEvent::CreateClose(this, true));
			if (HasListener("error")) {

				XBOX::VString	errorMessage;
	
				XBOX::VErrorBase::GetLocalizer(XBOX::kSERVER_NET_SIGNATURE)->LocalizeErrorMessage(error, errorMessage);
				fWorker->QueueEvent(VJSNetEvent::CreateError(this, errorMessage));

			} else 
			
				;	// XBOX::vThrowError(error);

		}	

	} else {
		
		if (!length) {

			// Do not support "half close", consider them as "full" close. 
			// Queue both events.

			fWorker->QueueEvent(VJSNetEvent::Create(this, "end"));
			fWorker->QueueEvent(VJSNetEvent::CreateClose(this, false));

			isOk = false;
			
		} else if (fIsPaused) {

			fBytesRead += length;

			SPacket	packet;

			packet.fBuffer = buffer;
			packet.fLength = length;

			fBufferedData.push_back(packet);
			
			isOk = true;

		} else {
			
			fBytesRead += length;

			_FlushBufferedData();
			fWorker->QueueEvent(VJSNetEvent::CreateData(this, buffer, length));

			isOk = true;

		}

	}

	return isOk;
}

void VJSNetSocketObject::_FlushBufferedData()
{
	// fMutex must have been acquired.

	std::list<SPacket>::iterator	i;

	for (i = fBufferedData.begin(); i != fBufferedData.end(); i++) 

		fWorker->QueueEvent(VJSNetEvent::CreateData(this, i->fBuffer, i->fLength));

	fBufferedData.clear();
}

void VJSNetSocketClass::GetDefinition (ClassDefinition &outDefinition)
{
	static inherited::StaticFunction functions[] =
	{
		{ "connect",		js_callStaticFunction<_connect>,		JS4D::PropertyAttributeDontDelete	},
		{ "setEncoding",	js_callStaticFunction<_setEncoding>,	JS4D::PropertyAttributeDontDelete	},
		{ "setSecure",		js_callStaticFunction<_setSecure>,		JS4D::PropertyAttributeDontDelete	},
		{ "write",			js_callStaticFunction<_write>,			JS4D::PropertyAttributeDontDelete	},
		{ "end",			js_callStaticFunction<_end>,			JS4D::PropertyAttributeDontDelete	},
		{ "destroy",		js_callStaticFunction<_destroy>,		JS4D::PropertyAttributeDontDelete	},
		{ "pause",			js_callStaticFunction<_pause>,			JS4D::PropertyAttributeDontDelete	},
		{ "resume",			js_callStaticFunction<_resume>,			JS4D::PropertyAttributeDontDelete	},
		{ "setTimeout",		js_callStaticFunction<_setTimeout>,		JS4D::PropertyAttributeDontDelete	},
		{ "setNoDelay",		js_callStaticFunction<_setNoDelay>,		JS4D::PropertyAttributeDontDelete	},
		{ "setKeepAlive",	js_callStaticFunction<_setKeepAlive>,	JS4D::PropertyAttributeDontDelete	},
		{ "address",		js_callStaticFunction<_address>,		JS4D::PropertyAttributeDontDelete	},
		{ 0,				0,										0									},
	};

    outDefinition.className			= "Socket";
	outDefinition.parentClass		= VJSEventEmitterClass::Class();
	outDefinition.staticFunctions	= functions;
    outDefinition.initialize		= js_initialize<_Initialize>;
    outDefinition.finalize			= js_finalize<_Finalize>;
	outDefinition.getProperty		= js_getProperty<_GetProperty>;
}

void VJSNetSocketClass::Construct (VJSParms_construct &ioParms)
{
	XBOX::VJSObject	nullObject(ioParms.GetContext());
	bool			allowHalfOpen;
	sLONG			ipType;

	allowHalfOpen = false;
	ipType = VJSNetSocketObject::eTYPE_TCP4;

	if (ioParms.CountParams() >= 1 && ioParms.IsObjectParam(1)) {

		XBOX::VJSObject	options(ioParms.GetContext());
		XBOX::VJSValue	value(ioParms.GetContext());
		bool			hasType;
		XBOX::VString	type;

		if (!ioParms.GetParamObject(1, options)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_OBJECT	, "1");
			return;

		} else if (((hasType = options.HasProperty("type")) && (value = options.GetProperty("type")).IsString() && value.GetString(type) 
			&& !type.EqualToString("tcp4") && !type.EqualToString("tcp6"))
		||	(options.HasProperty("fd") && !(value = options.GetProperty("fd")).IsUndefined() && !value.IsNull())) {

			// Only support IPv4 or IPv6, no file or UNIX descriptor.

			XBOX::vThrowError(XBOX::VE_JVSC_UNSUPPORTED_ARGUMENT, "new net.Socket()", "1");
			return;

		} else {

			allowHalfOpen = options.GetPropertyAsBool("allowHalfOpen", NULL, NULL);	// If property is not there, this will return false.
			if (hasType)

				ipType = type.EqualToString("tcp4") ? VJSNetSocketObject::eTYPE_TCP4 : VJSNetSocketObject::eTYPE_TCP6;

		}

	}

	VJSNetSocketObject	*socketObject; 

	if ((socketObject = new VJSNetSocketObject(false, ipType, allowHalfOpen,ioParms.GetContext())) != NULL) {

		socketObject->fObject = VJSNetSocketClass::CreateInstance(ioParms.GetContext(), socketObject);

		socketObject->fWorker = VJSWorker::RetainWorker(ioParms.GetContext());
			
		ioParms.ReturnConstructedObject(socketObject->fObject);

	}
}

void VJSNetSocketClass::Connect (XBOX::VJSParms_withArguments &ioParms, XBOX::VJSObject	&inSocket, bool inIsSSL)
{
	VJSNetSocketObject	*socketObject;

	socketObject = inSocket.GetPrivateData<VJSNetSocketClass>();
	xbox_assert(socketObject != NULL);

	// Check and read arguments.

	sLONG			port;
	XBOX::VString	host;
	XBOX::VJSObject	callbackObject(ioParms.GetContext());
	
	if (!ioParms.CountParams() || !ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &port)) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
		return;

	}
	
	if (ioParms.CountParams() < 2)

		host = "localhost";

	else if (!ioParms.IsStringParam(2) || !ioParms.GetStringParam(2, host)) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2");
		return;

	}

	if (ioParms.CountParams() >= 3
	&& (!ioParms.IsObjectParam(3) || !ioParms.GetParamObject(3, callbackObject) || !callbackObject.IsFunction())) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "3");
		return;

	} 

	if (callbackObject.IsFunction())
	
		socketObject->AddListener(socketObject->fWorker, inIsSSL ? "secureConnect" : "connect", callbackObject, false);

	// Connect socket.

	if (socketObject->fEndPoint != NULL) {

		// Already connected socket, cannot connect twice.

		XBOX::vThrowError(XBOX::VE_JVSC_ALREADY_CONNECTED);
		return;

	}
	
	XBOX::VTCPSelectIOPool	*selectIOPool;
	XBOX::VError			error;

	selectIOPool = VJSNetSocketObject::GetSelectIOPool();
	{
		StErrorContextInstaller	context(false, true);

		// Socket objects are asynchronous for reads and synchronous for writes but actual implementation is fully synchronous:
		// SelectIO is used to find out if data is available, then a blocking recv() or SSL_read() will always have data to 
		// process and will not block. Writes are always synchronous, hence block as needed. Errors are catched by contexts.
	
		socketObject->fEndPoint = VTCPEndPointFactory::CreateClientConnection(host, port, false, true, 1000, selectIOPool, error);
		
		xbox_assert(context.GetLastError() == error);

		if (error == XBOX::VE_OK) {

			xbox_assert(socketObject->fEndPoint != NULL);

			socketObject->fEndPoint->SetNoDelay(false);	

			// Use the "read callback" system of SelectIO, no need to enable SelectIO even if it is used. (This is ugly.)

//			socketObject->fEndPoint->SetIsSelectIO(true);

			error = context.GetLastError();

			// Promote to SSL if needed. Because we use synchronous reads and writes, the connection is synchronus.

			if (error == XBOX::VE_OK && inIsSSL) 

				error = socketObject->fEndPoint->PromoteToSSL();

		}
	}

	if (error != XBOX::VE_OK) {

		if (socketObject->HasListener("error")) {

			XBOX::VString	errorMessage;

			XBOX::VErrorBase::GetLocalizer(XBOX::kSERVER_NET_SIGNATURE)->LocalizeErrorMessage(error, errorMessage);
			socketObject->fWorker->QueueEvent(VJSNetEvent::CreateError(socketObject, errorMessage));

		} else
			
			XBOX::vThrowError(error);

	} else {
		   
		socketObject->fEndPoint->SetReadCallback(VJSNetSocketObject::_ReadCallback, socketObject);
		
		XBOX::VString	address;

		socketObject->fEndPoint->GetIP(address);
		inSocket.SetProperty("remoteAddress", address);
		inSocket.SetProperty("remotePort", (sLONG) socketObject->fEndPoint->GetPort() & 0xffff);

		if (inIsSSL) {

			socketObject->fWorker->QueueEvent(VJSNetEvent::Create(socketObject, "secureConnect"));

		} else 

			socketObject->fWorker->QueueEvent(VJSNetEvent::Create(socketObject, "connect"));

	} 
}


void VJSNetSocketClass::NewInstance (XBOX::VJSContext inContext, 
									 VJSEventEmitter *inEventEmitter, XBOX::VTCPEndPoint *inEndPoint,
									 XBOX::VJSValue *outValue)
{
	xbox_assert(inEndPoint != NULL && outValue != NULL);

	VJSWorker			*worker;
	VJSNetSocketObject	*socketObject; 
	
	worker = VJSWorker::RetainWorker(inContext);
	if ((socketObject = new VJSNetSocketObject(false, VJSNetSocketObject::eTYPE_TCP4, false,inContext)) == NULL) {

		XBOX::VString	errorMessage;

		XBOX::VErrorBase::GetLocalizer(XBOX::kSERVER_NET_SIGNATURE)->LocalizeErrorMessage(XBOX::VE_MEMORY_FULL, errorMessage);
		worker->QueueEvent(VJSNetEvent::CreateError(inEventEmitter, errorMessage));

		outValue->SetNull();	// Prevent callback call.

	} else {

		socketObject->fEndPoint = inEndPoint;

		socketObject->fObject = VJSNetSocketClass::CreateInstance(inContext, socketObject);
		socketObject->fWorker = XBOX::RetainRefCountable<VJSWorker>(worker);

		socketObject->fEndPoint->SetNoDelay(false);
		socketObject->fEndPoint->SetIsSelectIO(false);
		socketObject->fEndPoint->SetIsBlocking(true);

		socketObject->fEndPoint->SetReadCallback(VJSNetSocketObject::_ReadCallback, socketObject);

		XBOX::VString	address;
	
		socketObject->fEndPoint->GetIP(address);
		socketObject->fObject.SetProperty("remoteAddress", address);
		socketObject->fObject.SetProperty("remotePort", (sLONG) socketObject->fEndPoint->GetPort() & 0xffff);

		*outValue = socketObject->fObject;

	}
	XBOX::ReleaseRefCountable<VJSWorker>(&worker);
}

void VJSNetSocketClass::_Initialize (const XBOX::VJSParms_initialize &inParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);
	
	// Writes are never buffered so bufferSize attribute will always be zero.

	inParms.GetObject().SetProperty("bufferSize", (sLONG) 0);
}

void VJSNetSocketClass::_Finalize (const XBOX::VJSParms_finalize &inParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);

	inSocket->ForceClose();
	inSocket->Release();
}

void VJSNetSocketClass::_GetProperty (XBOX::VJSParms_getProperty &ioParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);

	XBOX::VString	name;

	if (!ioParms.GetPropertyName(name))

		ioParms.ReturnUndefinedValue();

	else if (name.EqualToUSASCIICString("bytesRead"))

		ioParms.ReturnNumber<uLONG>(inSocket->fBytesRead);

	else if (name.EqualToUSASCIICString("bytesWritten"))

		ioParms.ReturnNumber<uLONG>(inSocket->fBytesWritten);

	else 

		ioParms.ForwardToParent();
}

void VJSNetSocketClass::_connect (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);

	XBOX::VJSObject	thisObject	= ioParms.GetThis();

	VJSNetSocketClass::Connect(ioParms, thisObject, false);
}

void VJSNetSocketClass::_setEncoding (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);

	if (ioParms.IsNullOrUndefinedParam(1)) 

		inSocket->fEncoding = XBOX::VTC_UNKNOWN;

	else {

		XBOX::VString	encodingString;
		CharSet			encoding;

		if (!ioParms.IsStringParam(1) || !ioParms.GetStringParam(1, encodingString))
		
			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");

		else if ((encoding = VJSBufferObject::GetEncodingType(encodingString)) == XBOX::VTC_UNKNOWN)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_UNSUPPORTED_ENCODING, encodingString);
			
		else 

			inSocket->fEncoding = encoding;

	}
}

void VJSNetSocketClass::_setSecure (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);

	if (inSocket->fEndPoint == NULL || inSocket->fEndPoint->IsSSL())

		XBOX::vThrowError(XBOX::VE_JVSC_INVALID_STATE, "net.Socket.setSecure()");
		
	else {

		XBOX::StLocker<XBOX::VCriticalSection>	lock(&inSocket->fMutex);

		inSocket->fEndPoint->PromoteToSSL();

	}
}

// write() doesn't support callback.

void VJSNetSocketClass::_write (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);

	if (inSocket->fEndPoint == NULL) {

		XBOX::vThrowError(XBOX::VE_JVSC_INVALID_STATE, "net.Socket.write()");
		return;

	}

	if (!ioParms.CountParams()) {

		XBOX::vThrowError(XBOX::VE_JVSC_EXPECTING_PARAMETER, "net.Socket.write()");
		return;

	}
	
	uBYTE	*buffer;
	sLONG	length;
	bool	isAllocatedBuffer;
	
	if (ioParms.IsStringParam(1)) {

		XBOX::VString	string;

		if (!ioParms.GetStringParam(1, string)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
			return;

		}
		
		CharSet	encoding;

		encoding = XBOX::VTC_UTF_8;
		if (ioParms.CountParams() >= 2) {

			XBOX::VString	encodingString;

			if (!ioParms.IsStringParam(2) || !ioParms.GetStringParam(2, encodingString)) {

				XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2");
				return;

			}
			if ((encoding = VJSBufferObject::GetEncodingType(encodingString)) == XBOX::VTC_UNKNOWN) {

				XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_UNSUPPORTED_ENCODING, encodingString);
				return;

			}
			
		}

		buffer = NULL;
		if ((length = VJSBufferObject::FromString(string, encoding, &buffer)) < 0) {

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_ENCODING_FAILED);
			return;

		}
		isAllocatedBuffer = true;

	} else {

		XBOX::VJSObject	object(ioParms.GetContext());

		if (!ioParms.IsObjectParam(1) || !ioParms.GetParamObject(1, object) || !object.IsInstanceOf("Buffer")) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BUFFER, "1");
			return;

		}

		VJSBufferObject	*bufferObject;

		bufferObject = object.GetPrivateData<VJSBufferClass>();	
		xbox_assert(bufferObject != NULL);

		buffer = (uBYTE *) bufferObject->GetDataPtr();
		length = (sLONG) bufferObject->GetDataSize();

		isAllocatedBuffer = false;

	}

	// Write synchronously.

	XBOX::VError	error;
	
	if (length) {

		// If SSL is used, mutex will prevent SSL_write() during SSL_read() in callback.

///		XBOX::StLocker<XBOX::VCriticalSection>	lock(&inSocket->fMutex);	// FIX THAT
		StErrorContextInstaller					context(false, true);

		sLONG	bytesLeft	= length;
		uBYTE	*p			= buffer;

		xbox_assert(bytesLeft);
		while (bytesLeft) {

			uLONG	size;

			size = bytesLeft >= WRITE_SLICE_SIZE ? WRITE_SLICE_SIZE : bytesLeft;
		
			error = inSocket->fEndPoint->Write(p, &size);
			xbox_assert(context.GetLastError() == error);

			if (error != XBOX::VE_OK)

				break;

			else {

				p += size;
				inSocket->fBytesWritten += size;
				bytesLeft -= size;

			}

		}

	} else

		error = XBOX::VE_OK;

	if (error != XBOX::VE_OK) {

		if (inSocket->HasListener("error")) {

			XBOX::VString	errorMessage;

			XBOX::VErrorBase::GetLocalizer(XBOX::kSERVER_NET_SIGNATURE)->LocalizeErrorMessage(error, errorMessage);
			inSocket->fWorker->QueueEvent(VJSNetEvent::CreateError(inSocket, errorMessage));

		} else

			XBOX::vThrowError(error);

	} 

	if (isAllocatedBuffer)

		::free(buffer);

	ioParms.ReturnBool(true);
}

void VJSNetSocketClass::_end (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);

	// Write data if any.

	if (ioParms.CountParams()) 

		VJSNetSocketClass::_write(ioParms, inSocket);

	// Implement as close on both side.

	_destroy(ioParms, inSocket);
}

void VJSNetSocketClass::_destroy (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);

	inSocket->ForceClose();

	ioParms.GetThis().SetNullProperty("remoteAddress");
	ioParms.GetThis().SetNullProperty("remotePort");
}

void VJSNetSocketClass::_pause (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&inSocket->fMutex);

	inSocket->fIsPaused = true;	
}

void VJSNetSocketClass::_resume (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&inSocket->fMutex);

	inSocket->fIsPaused = false;
	inSocket->_FlushBufferedData();
}

void VJSNetSocketClass::_setTimeout (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);

	Real	argument;
	
	if (!ioParms.CountParams() || !ioParms.IsNumberParam(1) || !ioParms.GetRealParam(1, &argument)) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
		return;

	}

	if (argument != argument || argument < 0) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_NUMBER_ARGUMENT, "1");
		return;

	}

	inSocket->fTimeOut = (uLONG) argument;

	// Support in ServerNet?.
}

void VJSNetSocketClass::_setNoDelay (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);

	bool	yesNo;

	yesNo = true;
	if (ioParms.CountParams() && (!ioParms.IsBooleanParam(1) || !ioParms.GetBoolParam(1, &yesNo))) 

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "1");
		
	else if (inSocket->fEndPoint == NULL) 
	
		XBOX::vThrowError(XBOX::VE_JVSC_INVALID_STATE, "net.Socket.setNoDelay()");
	
	else {		

		XBOX::VError	error;

		{
			StErrorContextInstaller	context(false, true);
	
			inSocket->fEndPoint->SetNoDelay(yesNo);
			error = context.GetLastError();
		}

		if (error != XBOX::VE_OK) {

			if (inSocket->HasListener("error")) {

				XBOX::VString	errorMessage;

				XBOX::VErrorBase::GetLocalizer(XBOX::kSERVER_NET_SIGNATURE)->LocalizeErrorMessage(error, errorMessage);
				inSocket->fWorker->QueueEvent(VJSNetEvent::CreateError(inSocket, errorMessage));

			} else

				XBOX::vThrowError(error);

		}

	}
}

void VJSNetSocketClass::_setKeepAlive (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);

	XBOX::vThrowError(XBOX::VE_JVSC_UNSUPPORTED_FUNCTION, "net.Socket.setKeepAlive()");
}

void VJSNetSocketClass::_address (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);

	if (inSocket->fEndPoint == NULL) {

		XBOX::vThrowError(XBOX::VE_JVSC_INVALID_STATE, "net.Socket.address()");
		ioParms.ReturnUndefinedValue();

	} else {

		XBOX::VJSObject	object(ioParms.GetContext());
		XBOX::VString	address;

		object.MakeEmpty();
	
		inSocket->fEndPoint->GetIP(address);
		object.SetProperty("address", address);
		object.SetProperty("port", (sLONG) inSocket->fEndPoint->GetPort() & 0xffff);

		ioParms.ReturnValue(object);

	}	
}

void VJSNetSocketSyncClass::GetDefinition (ClassDefinition &outDefinition)
{
	static inherited::StaticFunction functions[] =
	{
		{	"connect",		js_callStaticFunction<_connect>,		JS4D::PropertyAttributeDontDelete	},
		{	"setEncoding",	js_callStaticFunction<_setEncoding>,	JS4D::PropertyAttributeDontDelete	},
		{	"setSecure",	js_callStaticFunction<_setSecure>,		JS4D::PropertyAttributeDontDelete	},
		{	"read",			js_callStaticFunction<_read>,			JS4D::PropertyAttributeDontDelete	},
		{	"write",		js_callStaticFunction<_write>,			JS4D::PropertyAttributeDontDelete	},
		{	"end",			js_callStaticFunction<_end>,			JS4D::PropertyAttributeDontDelete	},
		{	"destroy",		js_callStaticFunction<_destroy>,		JS4D::PropertyAttributeDontDelete	},
		{	"setTimeout",	js_callStaticFunction<_setTimeout>,		JS4D::PropertyAttributeDontDelete	},
		{	"setNoDelay",	js_callStaticFunction<_setNoDelay>,		JS4D::PropertyAttributeDontDelete	},
		{	"setKeepAlive",	js_callStaticFunction<_setKeepAlive>,	JS4D::PropertyAttributeDontDelete	},
		{	"address",		js_callStaticFunction<_address>,		JS4D::PropertyAttributeDontDelete	},
		{	0,				0,										0									},
	};

    outDefinition.className			= "SocketSync";
	outDefinition.staticFunctions	= functions;
	outDefinition.initialize		= js_initialize<_Initialize>;
    outDefinition.finalize			= js_finalize<_Finalize>;
	outDefinition.getProperty		= js_getProperty<_GetProperty>;
}

void VJSNetSocketSyncClass::_Initialize (const XBOX::VJSParms_initialize &inParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);
	
	// Writes are synchronous so bufferSize attribute will always be zero.

	inParms.GetObject().SetProperty("bufferSize", (sLONG) 0);

	// Set to true if peer has sent FIN.

	inParms.GetObject().SetProperty("hasEnded", false);
}

void VJSNetSocketSyncClass::_Finalize (const XBOX::VJSParms_finalize &inParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);

	inSocket->ForceClose();
	inSocket->Release();
}

void VJSNetSocketSyncClass::_GetProperty (XBOX::VJSParms_getProperty &ioParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);

	XBOX::VString	name;

	if (!ioParms.GetPropertyName(name))

		ioParms.ReturnUndefinedValue();

	else if (name.EqualToUSASCIICString("bytesRead"))

		ioParms.ReturnNumber<uLONG>(inSocket->fBytesRead);

	else if (name.EqualToUSASCIICString("bytesWritten"))

		ioParms.ReturnNumber<uLONG>(inSocket->fBytesWritten);

	else 

		ioParms.ForwardToParent();
}

void VJSNetSocketSyncClass::Construct (VJSParms_construct &ioParms)
{
	bool	allowHalfOpen;
	sLONG	ipType;	
	
	ipType = VJSNetSocketObject::eTYPE_TCP4;
	if (ioParms.CountParams() == 1) {

		XBOX::VJSObject	options(ioParms.GetContext());
		XBOX::VJSValue	value(ioParms.GetContext());
		bool			hasType;
		XBOX::VString	type;

		if (!ioParms.GetParamObject(1, options)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_OBJECT	, "1");
			return;

		} else if (((hasType = options.HasProperty("type")) && (value = options.GetProperty("type")).IsString() && value.GetString(type) 
			&& !type.EqualToString("tcp4") && !type.EqualToString("tcp6"))
		||	(options.HasProperty("fd") && !(value = options.GetProperty("fd")).IsUndefined() && !value.IsNull())) {

			// Only support IPv4 or IPv6, no file or UNIX descriptor.

			XBOX::vThrowError(XBOX::VE_JVSC_UNSUPPORTED_ARGUMENT, "new net.SocketSync()", "1");
			return;

		} else {

			allowHalfOpen = options.GetPropertyAsBool("allowHalfOpen", NULL, NULL);	// If property is not there, this will return false.
			if (hasType)

				ipType = type.EqualToString("tcp4") ? VJSNetSocketObject::eTYPE_TCP4 : VJSNetSocketObject::eTYPE_TCP6;

		}

	} else if (!ioParms.CountParams()) {

		allowHalfOpen = false;
		ipType = VJSNetSocketObject::eTYPE_TCP4;				

	} else {

		XBOX::vThrowError(XBOX::VE_JVSC_EXPECTING_PARAMETER, "new net.SocketSync()");
		return;

	}

	VJSNetSocketObject	*socketObject; 

	if ((socketObject = new VJSNetSocketObject(true, ipType, allowHalfOpen,ioParms.GetContext())) != NULL) {

		socketObject->fObject = VJSNetSocketSyncClass::CreateInstance(ioParms.GetContext(), socketObject);
	
		socketObject->fWorker = VJSWorker::RetainWorker(ioParms.GetContext());
			
		ioParms.ReturnConstructedObject(socketObject->fObject);

	} else 

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
}

void VJSNetSocketSyncClass::Connect (XBOX::VJSParms_withArguments &ioParms, XBOX::VJSObject &inSocket, bool inIsSSL)
{
	VJSNetSocketObject	*socketObject;

	socketObject = inSocket.GetPrivateData<VJSNetSocketSyncClass>();
	xbox_assert(socketObject != NULL);

	// Check if already connected.

	if (socketObject->fEndPoint != NULL) {

		XBOX::vThrowError(XBOX::VE_JVSC_ALREADY_CONNECTED);	
		return;

	}

	sLONG	numberArguments;

	if (!(numberArguments = (sLONG) ioParms.CountParams())) {

		XBOX::vThrowError(XBOX::VE_JVSC_EXPECTING_PARAMETER);
		return;

	}
	
	sLONG			port;
	XBOX::VString	host;
	
	if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &port)) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
		return;

	}
	
	if (numberArguments < 2)

		host = "localhost";

	else if (!ioParms.IsStringParam(2) || !ioParms.GetStringParam(2, host)) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2");
		return; 

	}

	XBOX::VError	error;
	
	socketObject->fEndPoint = VTCPEndPointFactory::CreateClientConnection(host, port, false, true, 1000, NULL, error);
	if (socketObject->fEndPoint != NULL) {
		
		socketObject->fEndPoint->SetNoDelay(false);
		
		XBOX::VString	address;
		
		socketObject->fEndPoint->GetIP(address);
		inSocket.SetProperty("remoteAddress", address);
		inSocket.SetProperty("remotePort", (sLONG) socketObject->fEndPoint->GetPort() & 0xffff);

		if (inIsSSL) 

			socketObject->fEndPoint->PromoteToSSL();

	} else 

		XBOX::vThrowError(error);
}

void VJSNetSocketSyncClass::_connect (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);

	XBOX::VJSObject	thisObject	= ioParms.GetThis();

	VJSNetSocketSyncClass::Connect(ioParms, thisObject, false);
}

void VJSNetSocketSyncClass::_setEncoding (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);

	if (ioParms.IsNullOrUndefinedParam(1)) 

		inSocket->fEncoding = XBOX::VTC_UNKNOWN;

	else {

		XBOX::VString	encodingString;
		CharSet			encoding;

		if (!ioParms.IsStringParam(1) || !ioParms.GetStringParam(1, encodingString))
		
			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");

		else if ((encoding = VJSBufferObject::GetEncodingType(encodingString)) == XBOX::VTC_UNKNOWN)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_UNSUPPORTED_ENCODING, encodingString);

		else

			inSocket->fEncoding = encoding;

	}
}

void VJSNetSocketSyncClass::_setSecure (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);

	if (inSocket->fEndPoint == NULL || inSocket->fEndPoint->IsSSL())

		XBOX::vThrowError(XBOX::VE_JVSC_INVALID_STATE, "net.SocketSync.setSecure()");
		
	else {

		XBOX::StLocker<XBOX::VCriticalSection>	lock(&inSocket->fMutex);

		inSocket->fEndPoint->PromoteToSSL();

	}
}

void VJSNetSocketSyncClass::_read (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);

	// Return null if erroneous or nothing to read (timed-out or peer has sent FIN).

	ioParms.ReturnNullValue();

	sLONG	timeOut;
	
	if (ioParms.CountParams()) {

		if (ioParms.CountParams() != 1 || !ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &timeOut)) {
			
			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
			return;
			
		}

		if (timeOut < 0) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_NUMBER_ARGUMENT, "1");
			return;
			
		}
		
		if (!timeOut)

			timeOut = 1;	// Windows platform seems to have problem with zero milliseconds.
							// Set minimum to 1ms, not a real "peek".

	} else

		timeOut = -1;		// If no argument, wait infinitely.

	uBYTE	*buffer;
	uLONG	length;

	if ((buffer = (uBYTE *) ::malloc(VJSNetSocketObject::kReadBufferSize)) == NULL) {

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
		return;

	}
	length = VJSNetSocketObject::kReadBufferSize;

	XBOX::VError	error;

	if (timeOut > 0)

		error = inSocket->fEndPoint->DirectSocketRead(buffer, &length, timeOut);

	else

		error = inSocket->fEndPoint->DirectSocketRead(buffer, &length);

	if (error == XBOX::VE_OK) {

		if (length)

			ioParms.ReturnValue(VJSBufferClass::NewInstance(ioParms.GetContext(), length, buffer));

		else {

			// A length of zero means the peer has sent FIN.

			ioParms.GetThis().SetProperty("hasEnded", true);

		}

	} else {

		::free(buffer);
		
		// A timed-out read will return null, but other errors will throw.

		if (error != XBOX::VE_SOCK_TIMED_OUT)

			XBOX::vThrowError(error);

	}
}

void VJSNetSocketSyncClass::_write (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);

	if (inSocket->fEndPoint == NULL) {

		XBOX::vThrowError(XBOX::VE_JVSC_INVALID_STATE, "net.SocketSync.write()");
		return;

	}

	if (!ioParms.CountParams()) {

		XBOX::vThrowError(XBOX::VE_JVSC_EXPECTING_PARAMETER, "net.SocketSync.write()");
		return;

	}
	
	uBYTE	*buffer;
	sLONG	length;
	bool	isAllocatedBuffer;
	
	if (ioParms.IsStringParam(1)) {

		XBOX::VString	string;

		if (!ioParms.GetStringParam(1, string)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
			return;

		}
		
		CharSet	encoding;

		encoding = XBOX::VTC_UTF_8;
		if (ioParms.CountParams() >= 2) {

			XBOX::VString	encodingString;

			if (!ioParms.IsStringParam(2) || !ioParms.GetStringParam(2, encodingString)) {

				XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2");
				return;

			} 
			if ((encoding = VJSBufferObject::GetEncodingType(encodingString)) == XBOX::VTC_UNKNOWN) {

				XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_UNSUPPORTED_ENCODING, encodingString);
				return;

			}

		}

		buffer = NULL;
		if ((length = VJSBufferObject::FromString(string, encoding, &buffer)) < 0) {

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_ENCODING_FAILED);
			return;

		}
		isAllocatedBuffer = true;

	} else {

		XBOX::VJSObject	object(ioParms.GetContext());

		if (!ioParms.IsObjectParam(1) || !ioParms.GetParamObject(1, object) || !object.IsInstanceOf("Buffer")) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BUFFER, "1");
			return;

		}

		VJSBufferObject	*bufferObject;

		bufferObject = object.GetPrivateData<VJSBufferClass>();	
		xbox_assert(bufferObject != NULL);

		buffer = (uBYTE *) bufferObject->GetDataPtr();
		length = (sLONG) bufferObject->GetDataSize();

		isAllocatedBuffer = false;

	}
	
	if (length) {

		sLONG	bytesLeft	= length;
		uBYTE	*p			= buffer;

		xbox_assert(bytesLeft);
		while (bytesLeft) {

			uLONG	size;

			size = bytesLeft >= WRITE_SLICE_SIZE ? WRITE_SLICE_SIZE : bytesLeft;		
			if (inSocket->fEndPoint->Write(p, &size) != XBOX::VE_OK)

				break;

			else {

				p += size;
				inSocket->fBytesWritten += size;
				bytesLeft -= size;

			}

		}

	}

	if (isAllocatedBuffer)

		::free(buffer);

	ioParms.ReturnBool(true);
}

void VJSNetSocketSyncClass::_end (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);

	if (ioParms.CountParams())

		VJSNetSocketSyncClass::_write(ioParms, inSocket);

	// Implement as close on both side.

	_destroy(ioParms, inSocket);
}

void VJSNetSocketSyncClass::_destroy (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);

	inSocket->ForceClose();

	ioParms.GetThis().SetNullProperty("remoteAddress");
	ioParms.GetThis().SetNullProperty("remotePort");		
}

void VJSNetSocketSyncClass::_setTimeout (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);

	Real	argument;
	
	if (!ioParms.CountParams() || !ioParms.IsNumberParam(1) || !ioParms.GetRealParam(1, &argument)) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
		return;

	}

	if (argument != argument || argument < 0) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_NUMBER_ARGUMENT, "1");
		return;

	}

	inSocket->fTimeOut = (uLONG) argument;

	// Support in ServerNet?.
}

void VJSNetSocketSyncClass::_setNoDelay (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);

	bool	yesNo;

	yesNo = true;
	if (ioParms.CountParams() && (ioParms.CountParams() != 1 || !ioParms.GetBoolParam(1, &yesNo)))

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "1");

	else if (inSocket->fEndPoint == NULL) 

		XBOX::vThrowError(XBOX::VE_JVSC_INVALID_STATE, "net.SocketSync.setNoDelay()");

	else 

		inSocket->fEndPoint->SetNoDelay(yesNo);
}

void VJSNetSocketSyncClass::_setKeepAlive (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);

	XBOX::vThrowError(XBOX::VE_JVSC_UNSUPPORTED_FUNCTION, "net.SocketSync.setKeepAlive()");
}

void VJSNetSocketSyncClass::_address (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket)
{
	xbox_assert(inSocket != NULL);

	if (inSocket->fEndPoint == NULL) {

		XBOX::vThrowError(XBOX::VE_JVSC_INVALID_STATE, "net.SocketSync.address()");
		ioParms.ReturnUndefinedValue();

	} else {

		XBOX::VJSObject	object(ioParms.GetContext());
		XBOX::VString	address;

		object.MakeEmpty();
	
		inSocket->fEndPoint->GetIP(address);
		object.SetProperty("address", address);
		object.SetProperty("port", (sLONG) inSocket->fEndPoint->GetPort() & 0xffff);

		ioParms.ReturnValue(object);

	}	
}
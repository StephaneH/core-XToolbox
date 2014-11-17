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

#include "ServerNet/VServerNet.h"

#include "VJSValue.h"
#include "VJSContext.h"
#include "VJSClass.h"
#include "VJSRuntime_blob.h"
#include "VJSRuntime_stream.h"
#include "VJSBuffer.h"
#include "VJSNetSocket.h"

USING_TOOLBOX_NAMESPACE

// If an error is reported by VStream, report it (throw it), and then do a
// ResetLastError(). It will fail again thereafter if condition is really faulty.

void VJSStream::Initialize( const VJSParms_initialize& inParms, VStream* inStream)
{
	// rien pour l'instant
}


void VJSStream::Finalize( const VJSParms_finalize& inParms, VStream* inStream)
{
	if (inStream->IsReading())
		inStream->CloseReading();
	if (inStream->IsWriting())
		inStream->CloseWriting(true);
	delete inStream;
}



void VJSStream::_Close(VJSParms_callStaticFunction& ioParms, VStream* inStream)
{
	XBOX::VError	error;

	if (inStream->IsReading() && (error = inStream->CloseReading()) != XBOX::VE_OK) {

		XBOX::vThrowError(error);
		inStream->ResetLastError();

	}
	if (inStream->IsWriting() && (error = inStream->CloseWriting(true)) != XBOX::VE_OK) {

		XBOX::vThrowError(error);
		inStream->ResetLastError();

	}
}
	
void VJSStream::_Flush(VJSParms_callStaticFunction& ioParms, VStream* inStream)
{
	XBOX::VError	error;

	if (inStream->IsWriting() && (error = inStream->Flush()) != XBOX::VE_OK) {

		XBOX::vThrowError(error);
		inStream->ResetLastError();

	}
}


void VJSStream::_GetByte(VJSParms_callStaticFunction& ioParms, VStream* inStream)
{
	uBYTE c;
	VError err = inStream->GetByte(c);

	if (err == XBOX::VE_STREAM_EOF) 

		ioParms.ReturnNullValue();

	else if (err == VE_OK)

		ioParms.ReturnNumber(c);

	else {

		XBOX::vThrowError(err);
		inStream->ResetLastError();

	}
}


void VJSStream::_GetWord(VJSParms_callStaticFunction& ioParms, VStream* inStream)
{
	sWORD w;
	VError err = inStream->GetWord(w);

	if (err == XBOX::VE_STREAM_EOF) 

		ioParms.ReturnNullValue();

	else if (err == VE_OK)

		ioParms.ReturnNumber(w);

	else {

		XBOX::vThrowError(err);
		inStream->ResetLastError();

	}
}


void VJSStream::_GetLong(VJSParms_callStaticFunction& ioParms, VStream* inStream)
{
	sLONG l;
	VError err = inStream->GetLong(l);

	if (err == XBOX::VE_STREAM_EOF) 

		ioParms.ReturnNullValue();

	else if (err == VE_OK)

		ioParms.ReturnNumber(l);

	else {

		XBOX::vThrowError(err);
		inStream->ResetLastError();

	}
}


void VJSStream::_GetReal(VJSParms_callStaticFunction& ioParms, VStream* inStream)
{
	Real r;
	VError err = inStream->GetReal(r);

	if (err == XBOX::VE_STREAM_EOF) 

		ioParms.ReturnNullValue();

	else if (err == VE_OK)

		ioParms.ReturnNumber(r);

	else {

		XBOX::vThrowError(err);
		inStream->ResetLastError();

	}
}


void VJSStream::_GetLong64(VJSParms_callStaticFunction& ioParms, VStream* inStream)
{
	sLONG8 l8;
	VError err = inStream->GetLong8(l8);

	if (err == XBOX::VE_STREAM_EOF) 

		ioParms.ReturnNullValue();

	else if (err == VE_OK)

		ioParms.ReturnNumber(l8);

	else {

		XBOX::vThrowError(err);
		inStream->ResetLastError();

	}
}


void VJSStream::_GetString(VJSParms_callStaticFunction& ioParms, VStream* inStream)
{
	VString s;
	VError err = s.ReadFromStream(inStream);

	if (err == XBOX::VE_STREAM_EOF) 
		
		ioParms.ReturnNullValue();

	else if (err == VE_OK)

		ioParms.ReturnString(s);

	else {

		XBOX::vThrowError(err);
		inStream->ResetLastError();

	}
}

void VJSStream::_GetBlob (VJSParms_callStaticFunction &ioParms, XBOX::VStream *inStream)
{
	xbox_assert(inStream != NULL);

	_GetBinary(ioParms, inStream, false);
}

void VJSStream::_GetBuffer (VJSParms_callStaticFunction &ioParms, XBOX::VStream *inStream)
{
	xbox_assert(inStream != NULL);

	_GetBinary(ioParms, inStream, true);
}

void VJSStream::_GetBinary (VJSParms_callStaticFunction &ioParms, XBOX::VStream* inStream, bool inIsBuffer)
{
	xbox_assert(inStream != NULL);

	sLONG	size;
	void	*buffer;

	if (!ioParms.GetLongParam(1, &size) || size < 0) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
		ioParms.ReturnNullValue();

	} else if (!size) {

		if (inIsBuffer)

			ioParms.ReturnValue(VJSBufferClass::NewInstance(ioParms.GetContext(), 0, NULL));

		else 

			ioParms.ReturnValue(VJSBlob::NewInstance(ioParms.GetContext(), NULL, 0, CVSTR( "")));
			
	} else if ((buffer = ::malloc(size)) == NULL) {
	
		vThrowError(XBOX::VE_MEMORY_FULL);
		ioParms.ReturnNullValue();
	
	} else {

		XBOX::VError	error;
		VSize			bytesRead;

		// Prevent error throwing for XBOX::VE_STREAM_EOF, this will allow to read less than requested.

		{
			StErrorContextInstaller	context(false, true);

			bytesRead = 0;
			error = inStream->GetData(buffer, size, &bytesRead);
		}

/*
		// Do ::realloc() to save memory?

		if (bytesRead < size)

			buffer = ::realloc(buffer, bytesRead);
*/
				
		if (error != XBOX::VE_OK 
		&& error != XBOX::VE_STREAM_EOF 
		&& error != XBOX::VE_SRVR_NOTHING_TO_READ && error != XBOX::VE_SRVR_READ_TIMED_OUT) {
		
			XBOX::vThrowError(error);
			ioParms.ReturnNullValue();
			::free(buffer);

		} else {
			XBOX::VJSObject	obj(ioParms.GetContext());
			if (inIsBuffer)
				obj = VJSBufferClass::NewInstance(ioParms.GetContext(), bytesRead, buffer);


			else
				obj = VJSBlob::NewInstance(ioParms.GetContext(), buffer, bytesRead, CVSTR(""));

			
			ioParms.ReturnValue(obj);
			// Check if instance creation has succeed, if not free memory.
			if (!obj.HasRef())

				::free(buffer);
			
		}	

		inStream->ResetLastError();

	}
}

void VJSStream::_PutByte(VJSParms_callStaticFunction& ioParms, VStream* inStream)
{
	Real r = 0;
	if (ioParms.IsNumberParam(1))
	{
		ioParms.GetRealParam(1, &r);
		uBYTE c = r;
		VError err = inStream->PutByte(c);
		if (err != XBOX::VE_OK) {
			XBOX::vThrowError(err);
			inStream->ResetLastError();
		}
	}
	else
		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
}


void VJSStream::_PutWord(VJSParms_callStaticFunction& ioParms, VStream* inStream)
{
	Real r = 0;
	if (ioParms.IsNumberParam(1))
	{
		ioParms.GetRealParam(1, &r);
		sWORD c = r;
		VError err = inStream->PutWord(c);
		if (err != XBOX::VE_OK) {
			XBOX::vThrowError(err);
			inStream->ResetLastError();
		}
	}
	else
		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
}

void VJSStream::_PutLong(VJSParms_callStaticFunction& ioParms, VStream* inStream)
{
	Real r = 0;
	if (ioParms.IsNumberParam(1))
	{
		ioParms.GetRealParam(1, &r);
		sLONG c = (sLONG) r;
		VError err = inStream->PutLong(c);
		if (err != XBOX::VE_OK) {
			XBOX::vThrowError(err);
			inStream->ResetLastError();
		}
	}
	else
		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
}

void VJSStream::_PutLong64(VJSParms_callStaticFunction& ioParms, VStream* inStream)
{
	Real r = 0;
	if (ioParms.IsNumberParam(1))
	{
		ioParms.GetRealParam(1, &r);
		sLONG8 c = r;
		VError err = inStream->PutLong8(c);
		if (err != XBOX::VE_OK) {
			XBOX::vThrowError(err);
			inStream->ResetLastError();
		}
	}
	else
		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
}

void VJSStream::_PutReal(VJSParms_callStaticFunction& ioParms, VStream* inStream)
{
	Real r = 0;
	if (ioParms.IsNumberParam(1))
	{
		ioParms.GetRealParam(1, &r);
		VError err = inStream->PutReal(r);
		if (err != XBOX::VE_OK) {
			XBOX::vThrowError(err);
			inStream->ResetLastError();
		}
	}
	else
		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
}

void VJSStream::_PutString(VJSParms_callStaticFunction& ioParms, VStream* inStream)
{
	VString s;
	if (ioParms.IsStringParam(1))
	{
		ioParms.GetStringParam(1, s);
		VError err = s.WriteToStream(inStream);
		if (err != XBOX::VE_OK) {
			XBOX::vThrowError(err);
			inStream->ResetLastError();
		}
	}
	else
		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
}

void VJSStream::_PutBlob (VJSParms_callStaticFunction &ioParms, XBOX::VStream *inStream)
{
	_PutBinary(ioParms, inStream);
}

void VJSStream::_PutBuffer (VJSParms_callStaticFunction &ioParms, XBOX::VStream *inStream)
{
	_PutBinary(ioParms, inStream);
}

void VJSStream::_PutBinary (VJSParms_callStaticFunction &ioParms, XBOX::VStream *inStream)
{
	xbox_assert(inStream != NULL);

	XBOX::VJSObject	binaryObject(ioParms.GetContext());
		
	if (!ioParms.GetParamObject(1, binaryObject)) {
	
		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER, "1");
		return;

	} 

	bool			isBuffer;
	VJSBufferObject	*buffer;
	VJSDataSlice	*dataSlice;
	const void		*data;
	VSize			size;

	if (binaryObject.IsOfClass(VJSBufferClass::Class())) {
		
		isBuffer = true;

		buffer = binaryObject.GetPrivateData<VJSBufferClass>();
		xbox_assert(buffer != NULL);

		buffer->Retain();

		data = buffer->GetDataPtr();
		size = buffer->GetDataSize();

	} else if (binaryObject.IsOfClass(VJSBlob::Class())) {

		VJSBlobValue	*blob;

		isBuffer = false;

		blob = binaryObject.GetPrivateData<VJSBlob>();
		xbox_assert(blob != NULL);

		dataSlice = blob->RetainDataSlice();
		xbox_assert(dataSlice != NULL);
		
		data = dataSlice->GetDataPtr();
		size = dataSlice->GetDataSize();

	} else {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER, "1");
		return;

	}

	bool	isOk;
	sLONG	index, length;

	isOk = true;	
		
	if (ioParms.CountParams() >= 2) {
		
		if (!ioParms.GetLongParam(2, &index)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");
			isOk = false;

		} 

	} else

		index = 0;

	if (isOk && ioParms.CountParams() >= 3) {
		
		if (!ioParms.GetLongParam(3, &length)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "3");
			isOk = false;

		} 

	} else {

		length = (sLONG) size;
		if (index > 0)

			length -= index;

	}

	// Silently ignore if out of bound.

	if (isOk && index >= 0 && length > 0 && index + length <= size) {

		XBOX::VError	error;

		if ((error = inStream->PutData((uBYTE *) data + index, length)) != XBOX::VE_OK)

			XBOX::vThrowError(error);
		
	}

	if (isBuffer)

		buffer->Release();

	else

		dataSlice->Release();

}

void VJSStream::_GetPos(VJSParms_callStaticFunction& ioParms, VStream* inStream)
{
	ioParms.ReturnNumber(inStream->GetPos());
}


void VJSStream::_GetSize(VJSParms_callStaticFunction& ioParms, VStream* inStream)
{
	ioParms.ReturnNumber(inStream->GetSize());
}


void VJSStream::_SetPos(VJSParms_callStaticFunction& ioParms, VStream* inStream)
{
	Real r = 0;
	if (ioParms.IsNumberParam(1))
	{
		ioParms.GetRealParam(1, &r);
		sLONG8 newpos = r;
		VError err = inStream->SetPos(newpos);
		if (err != XBOX::VE_OK && err != XBOX::VE_STREAM_EOF) {

			XBOX::vThrowError(err);
			inStream->ResetLastError();

		}
	}
	else
		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
}


void VJSStream::_ChangeByteOrder(VJSParms_callStaticFunction& ioParms, VStream* inStream)
{
	inStream->SetNeedSwap(!inStream->NeedSwap());
}


void VJSStream::_IsByteSwapping(VJSParms_callStaticFunction& ioParms, VStream* inStream)
{
	ioParms.ReturnBool(inStream->NeedSwap());
}



void VJSStream::do_BinaryStream(VJSParms_callStaticFunction& ioParms)
{
	XBOX::VJSObject	object(ioParms.GetContext());

	// Is BinaryStream from a socket ?

	if (ioParms.GetParamObject(1, object)) {

		// net.Socket is asynchronous, hence not support.
		
		if (object.IsOfClass(VJSNetSocketClass::Class())) {

			XBOX::vThrowError(XBOX::VE_JVSC_BINARY_STREAM_SOCKET);
			return;

		}

		// If object is net.SocketSync, proceed.
		
		if (object.IsOfClass(VJSNetSocketSyncClass::Class())) {

			bool	forWrite;
			sLONG	timeOut;

			forWrite = ioParms.GetBoolParam( 2, L"Write", L"Read");
			if (ioParms.CountParams() >= 3) {

				if (!ioParms.GetLongParam(3, &timeOut)) {
				
					XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "3");
					return;

				} 
				
				if (timeOut < 0) {

					XBOX::vThrowError(XBOX::VE_JVSC_WRONG_NUMBER_ARGUMENT, "3");
					return;

				} 

			} else

				timeOut = kDefaultTimeOut;

			VJSNetSocketObject		*socketObject;
			XBOX::VEndPointStream	*stream;			
			XBOX::VError			error;

			socketObject = object.GetPrivateData<VJSNetSocketSyncClass>();
			xbox_assert(socketObject != NULL);

			if ((stream = new XBOX::VEndPointStream(socketObject->GetEndPoint(), timeOut)) == NULL) {

				XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
				ioParms.ReturnNullValue();
				
			} else if ((error = forWrite ? stream->OpenWriting() : stream->OpenReading()) != XBOX::VE_OK) {

				XBOX::vThrowError(error);
				delete stream;
				ioParms.ReturnNullValue();

			} else 

				ioParms.ReturnValue(VJSStream::CreateInstance(ioParms.GetContext(), stream));

			return;

		}

	} 

	//  BinaryStream is from a File.

	VFile* file = ioParms.RetainFileParam( 1);
	bool forwrite = ioParms.GetBoolParam( 2, L"Write", L"Read");
	if (file != NULL)
	{
		VError err = VE_OK;
		if (forwrite)
		{
			if (!file->Exists())
				err = file->Create();
		}
		VFileStream* stream = new VFileStream(file);
		if (err == VE_OK)
		{
			if (forwrite)
				err = stream->OpenWriting();
			else
				err = stream->OpenReading();
		}
		
		if (err == VE_OK)
		{
			ioParms.ReturnValue(VJSStream::CreateInstance(ioParms.GetContext(), stream));
		}
		else
		{
			delete stream;
		}
	}
	else
		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FILE, "1");
	ReleaseRefCountable( &file);
}


void VJSStream::GetDefinition( ClassDefinition& outDefinition)
{
	static inherited::StaticFunction functions[] = 
	{
		{ "close", js_callStaticFunction<_Close>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "flush", js_callStaticFunction<_Flush>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		
		{ "getByte", js_callStaticFunction<_GetByte>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "getWord", js_callStaticFunction<_GetWord>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "getLong", js_callStaticFunction<_GetLong>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "getLong64", js_callStaticFunction<_GetLong64>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "getReal", js_callStaticFunction<_GetReal>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "getString", js_callStaticFunction<_GetString>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "getBlob", js_callStaticFunction<_GetBlob>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "getBuffer", js_callStaticFunction<_GetBuffer>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		
		{ "putByte", js_callStaticFunction<_PutByte>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "putWord", js_callStaticFunction<_PutWord>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "putLong", js_callStaticFunction<_PutLong>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "putLong64", js_callStaticFunction<_PutLong64>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "putReal", js_callStaticFunction<_PutReal>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "putString", js_callStaticFunction<_PutString>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "putBlob", js_callStaticFunction<_PutBlob>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "putBuffer", js_callStaticFunction<_PutBuffer>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },

		{ "getPos", js_callStaticFunction<_GetPos>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "setPos", js_callStaticFunction<_SetPos>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "getSize", js_callStaticFunction<_GetSize>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },

		{ "changeByteOrder", js_callStaticFunction<_ChangeByteOrder>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "isByteSwapping", js_callStaticFunction<_IsByteSwapping>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ 0, 0, 0}
	};
	
	outDefinition.className = "BinaryStream";
	outDefinition.initialize = js_initialize<Initialize>;
	outDefinition.finalize = js_finalize<Finalize>;
	outDefinition.staticFunctions = functions;
}





//======================================================

bool VJSTextStreamState::_ReadUntilDelimiter (const XBOX::VString &inDelimiter, XBOX::VString *outResult)
{
	bool	isFound	= false;

	sLONG	numberCharactersLeft;
	
	numberCharactersLeft = fBuffer.GetLength() - fIndex;
	outResult->Clear();

	if (!inDelimiter.GetLength()) {

		for ( ; ; ) {

			if (!numberCharactersLeft) {

				fBuffer.Clear();
				fIndex = 0;

				XBOX::VError	error;

				if ((error = fStream->GetText(fBuffer, kREAD_SLICE_SIZE)) != XBOX::VE_OK && error != XBOX::VE_STREAM_EOF) 
					
					break;				// Error!
				
				if (!(numberCharactersLeft = fBuffer.GetLength()))
						
					break;				// End-of-file.

			}

			const UniChar	*p, *q;

			for (p = q = fBuffer.GetCPointer() + fIndex; *q != 0 && *q != '\r' && *q != '\n'; q++)

				;

			if (*q) {

				outResult->AppendUniChars(p, q - p);

				fIndex = 1 + (q - fBuffer.GetCPointer());
				if (fIndex == fBuffer.GetLength()) {

					fBuffer.Clear();
					fIndex = 0;

				}
				isFound = true;
				break;

			} else {

				outResult->AppendUniChars(p, fBuffer.GetLength() - fIndex);
				numberCharactersLeft = 0;

			}

		}

	} else {

		UniChar	delimiter;
		
		delimiter = inDelimiter[0];
		for ( ; ; ) {

			if (!numberCharactersLeft) {
				
				fBuffer.Clear();
				fIndex = 0;

				XBOX::VError	error;

				if ((error = fStream->GetText(fBuffer, kREAD_SLICE_SIZE)) != XBOX::VE_OK && error != XBOX::VE_STREAM_EOF) 
					
					break;				// Error!
				
				if (!(numberCharactersLeft = fBuffer.GetLength()))
						
					break;				// End-of-file.

			}

			VIndex	index;

			if ((index = fBuffer.FindUniChar(delimiter, fIndex + 1))) {

				// Remember that fIndex starts at zero and index at one.

				outResult->AppendUniChars(fBuffer.GetCPointer() + fIndex, index - (fIndex + 1));
				if (index == fBuffer.GetLength()) {

					fBuffer.Clear();
					fIndex = 0;

				} else 

					fIndex = index;	// fIndex now indexes next character!

				isFound = true;
				break;

			} else {

				outResult->AppendUniChars(fBuffer.GetCPointer() + fIndex, fBuffer.GetLength() - fIndex);
				numberCharactersLeft = 0;

			}
	
		}

	}

	return isFound;
}

void VJSTextStreamState::_ReadCharacters (sLONG inNumberCharacters, XBOX::VString *outResult)
{
	xbox_assert(outResult != NULL);

	sLONG	numberCharactersLeft;

	numberCharactersLeft = fBuffer.GetLength() - fIndex;

	// To read all files, "bound" using number of bytes.

	if (!inNumberCharacters)

		inNumberCharacters = numberCharactersLeft + fStream->GetSize() - fStream->GetPos();

	if (!inNumberCharacters)

		outResult->Clear();	// Nothing to read, return an empty string.

	else if (inNumberCharacters <= numberCharactersLeft) {

		// If buffer has enough characters, return them.

		fBuffer.GetSubString(fIndex + 1, inNumberCharacters, *outResult);

		fIndex += inNumberCharacters;
		if (fIndex == fBuffer.GetLength()) {

			fBuffer.Clear();
			fIndex = 0;

		}

	} else {

		sLONG	charactersToRead;

		// Buffer may have characters left, if so empty it.

		if (numberCharactersLeft) {

			charactersToRead = inNumberCharacters - numberCharactersLeft;
			fBuffer.GetSubString(fIndex + 1, numberCharactersLeft, *outResult);

			fBuffer.Clear();
			fIndex = 0;

		} else {

			outResult->Clear();
			charactersToRead = inNumberCharacters;

		}

		// Read required characters. 

		fStream->GetText(fBuffer, charactersToRead);
		if (fBuffer.GetLength() > charactersToRead) {

			// If there are more characters than needed, buffer them.

			outResult->AppendUniChars(fBuffer.GetCPointer(), charactersToRead);
			fIndex = charactersToRead;

		} else {

			// Actually, less characters may have been read.

			outResult->AppendString(fBuffer);
			fBuffer.Clear();
			fIndex = 0;

		}

	}
}

void VJSTextStream::GetDefinition (ClassDefinition &outDefinition)
{
	static inherited::StaticFunction functions[] = 
	{
		{	"close",	js_callStaticFunction<_Close>,		JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"getSize",	js_callStaticFunction<_GetSize>,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"getPos",	js_callStaticFunction<_GetPos>,		JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},

		{	"rewind",	js_callStaticFunction<_Rewind>,		JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"read",		js_callStaticFunction<_Read>,		JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},		
		{	"end",		js_callStaticFunction<_End>,		JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},

		{	"write",	js_callStaticFunction<_Write>,		JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"flush",	js_callStaticFunction<_Flush>,		JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},

		{	0,			0,									0}
	};
	
	outDefinition.className = "TextStream";	
	outDefinition.finalize = js_finalize<_Finalize>;	
	outDefinition.hasInstance = js_hasInstance<_HasInstance>;
	outDefinition.staticFunctions = functions;
}


void VJSTextStream::Construct( VJSParms_construct &ioParms)
{
	XBOX::VJSObject	constructedObject(ioParms.GetContext());

	XBOX::VFile		*file;

	if ((file = ioParms.RetainFileParam(1)) == NULL) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_FILE, "1");
		xbox_assert(false);
		ioParms.ReturnConstructedObject(constructedObject);
	}

	XBOX::VString	mode;
	bool			forwrite, overwrite = false;
	CharSet			defaultCharSet = VTC_UTF_8;
	sLONG			xcharset = 0;

	if (ioParms.GetStringParam(2, mode) && mode == L"Overwrite") {

		forwrite = true;
		overwrite = true;

	}
	else

		forwrite = ioParms.GetBoolParam(2, L"Write", L"Read");

	if (ioParms.GetLongParam(3, &xcharset) && xcharset != 0)

		defaultCharSet = (CharSet)xcharset;

	XBOX::VError	err = XBOX::VE_OK;

	if (forwrite) {

		if (!file->Exists())

			err = file->Create();

		else if (overwrite) {

			if ((err = file->Delete()) == XBOX::VE_OK)

				err = file->Create();

		}

	}

	VFileStream	*stream = NULL;

	if (err == XBOX::VE_OK) {

		if ((stream = new VFileStream(file)) == NULL)

			XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

		else if (forwrite) {

			if (err == XBOX::VE_OK)

				err = stream->OpenWriting();

			if (err == XBOX::VE_OK)
			{
				stream->SetCharSet(defaultCharSet);
				stream->SetCarriageReturnMode(eCRM_CRLF);
			}

			stream->SetPos(stream->GetSize());

		}
		else {

			err = stream->OpenReading();
			if (err == XBOX::VE_OK) {

				stream->GuessCharSetFromLeadingBytes(defaultCharSet);
				stream->SetCarriageReturnMode(eCRM_CRLF);

			}

		}

	}

	if (stream != NULL && err == XBOX::VE_OK) {

		VJSTextStreamState	*textStreamState;

		if ((textStreamState = new VJSTextStreamState()) == NULL) {

			delete stream;
			XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

		}
		else {

			textStreamState->fStream = stream;
			textStreamState->fBuffer.Clear();
			textStreamState->fIndex = 0;
			textStreamState->fPosition = 0;

			constructedObject = VJSTextStream::ConstructInstance(ioParms, textStreamState);

		}

	}

	ReleaseRefCountable(&file);

	ioParms.ReturnConstructedObject(constructedObject);
}





bool VJSTextStream::_HasInstance (const	VJSParms_hasInstance &inParms) 
{
	xbox_assert(inParms.GetObject().HasSameRef(inParms.GetObjectToTest()));
	xbox_assert(inParms.GetContext().GetGlobalObject().GetPropertyAsObject("TextStream").HasSameRef(inParms.GetPossibleContructor()));
 
	XBOX::VJSObject	objectToTest	= inParms.GetObject();

	return objectToTest.IsOfClass(VJSTextStream::Class());
}


void VJSTextStream::_Finalize (const VJSParms_finalize &inParms, VJSTextStreamState *inStreamState)
{
	if (inStreamState != NULL) {

		if (inStreamState->fStream->IsReading())

			inStreamState->fStream->CloseReading();

		if (inStreamState->fStream->IsWriting())

			inStreamState->fStream->CloseWriting(true);

		delete inStreamState->fStream;
		delete inStreamState;

	}
}

void VJSTextStream::_Close (VJSParms_callStaticFunction &ioParms, VJSTextStreamState *inStreamState)
{
	if (inStreamState == NULL) 

		XBOX::vThrowError(XBOX::VE_JVSC_INVALID_STATE, L"TextStream.close()");

	else if (inStreamState->fStream->IsReading())

		inStreamState->fStream->CloseReading();

	else if (inStreamState->fStream->IsWriting())

		inStreamState->fStream->CloseWriting(true);

	else

		XBOX::vThrowError(XBOX::VE_JVSC_INVALID_STATE, L"TextStream.close()");
}

void VJSTextStream::_GetSize (VJSParms_callStaticFunction &ioParms, VJSTextStreamState *inStreamState)
{
	if (inStreamState != NULL)

		ioParms.ReturnNumber(inStreamState->fStream->GetSize());
		
	else

		XBOX::vThrowError(XBOX::VE_JVSC_INVALID_STATE, L"TextStream.getSize()");
}

void VJSTextStream::_GetPos (VJSParms_callStaticFunction &ioParms, VJSTextStreamState *inStreamState)
{
	if (inStreamState != NULL)

		ioParms.ReturnNumber(inStreamState->fPosition);
		
	else

		XBOX::vThrowError(XBOX::VE_JVSC_INVALID_STATE, L"TextStream.getPos()");
}

void VJSTextStream::_Rewind (VJSParms_callStaticFunction &ioParms, VJSTextStreamState *inStreamState)
{
	if (inStreamState != NULL || inStreamState->fStream->IsReading()) {

		// VStream::SetPos() always check if last error is XBOX::VE_OK. Reset it if previous "error" was
		// reaching end of file.

		if (inStreamState->fStream->GetLastError() == XBOX::VE_STREAM_EOF) 
		
			inStreamState->fStream->ResetLastError();

		inStreamState->fStream->SetPos(0);
		inStreamState->fBuffer.Clear();
		inStreamState->fIndex = 0;
		inStreamState->fPosition = 0;

	} else

		XBOX::vThrowError(XBOX::VE_JVSC_INVALID_STATE, L"TextStream.rewind()");
}

void VJSTextStream::_Read (VJSParms_callStaticFunction &ioParms, VJSTextStreamState *inStreamState)
{
	if (inStreamState == NULL)

		XBOX::vThrowError(XBOX::VE_JVSC_INVALID_STATE, L"TextStream.read()");

	else if (ioParms.CountParams() == 1 && ioParms.IsStringParam(1)) {

		XBOX::VString	delimiter;

		if (!ioParms.GetStringParam(1, delimiter) || delimiter.GetLength() > 1)

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
			
		else {

			XBOX::VString	result;
			bool			hasBeenFound;

			hasBeenFound = inStreamState->_ReadUntilDelimiter(delimiter, &result);
			inStreamState->fPosition += result.GetLength();
			if (hasBeenFound)

				inStreamState->fPosition++;	// Delimiter is not included.

			ioParms.ReturnString(result);
		
		}

	} else {

		// numberCharacters is the number of actual characters to read, not the number of bytes.
				
		sLONG	numberCharacters;

		numberCharacters = 0;
		if (ioParms.CountParams() >= 1 && (!ioParms.GetLongParam(1, &numberCharacters) || numberCharacters < 0))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
			
		else {

			XBOX::VString	result;
		
			inStreamState->_ReadCharacters(numberCharacters, &result);
			inStreamState->fPosition += result.GetLength();
			ioParms.ReturnString(result);

		}

	}
}

void VJSTextStream::_End (VJSParms_callStaticFunction &ioParms, VJSTextStreamState *inStreamState)
{
	if (inStreamState != NULL && inStreamState->fStream->IsReading())

		ioParms.ReturnBool(!inStreamState->fBuffer.GetLength() 
		&& inStreamState->fStream->GetSize() <= inStreamState->fStream->GetPos());

	else

		XBOX::vThrowError(XBOX::VE_JVSC_INVALID_STATE, L"TextStream.end()");
}

void VJSTextStream::_Write (VJSParms_callStaticFunction &ioParms, VJSTextStreamState *inStreamState)
{
	if (inStreamState == NULL && !inStreamState->fStream->IsWriting()) 

		XBOX::vThrowError(XBOX::VE_JVSC_INVALID_STATE, L"TextStream.write()");

	else {

		XBOX::VString	string;
 
		if (ioParms.CountParams() != 1 || !ioParms.IsStringParam(1) || !ioParms.GetStringParam(1, string))
	
			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else {
	
			inStreamState->fStream->PutText(string);
			inStreamState->fPosition += string.GetLength();

		}

	}
}

void VJSTextStream::_Flush (VJSParms_callStaticFunction &ioParms, VJSTextStreamState *inStreamState)
{
	if (inStreamState != NULL && inStreamState->fStream->IsWriting())

		inStreamState->fStream->Flush();

	else

		XBOX::vThrowError(XBOX::VE_JVSC_INVALID_STATE, L"TextStream.flush()");
}

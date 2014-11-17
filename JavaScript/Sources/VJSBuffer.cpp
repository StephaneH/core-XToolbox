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

#include "VJSBuffer.h"

#include "VJSContext.h"
#include "VJSGlobalClass.h"
#include "VJSRuntime_blob.h"

#include "VJSW3CArrayBuffer.h"

USING_TOOLBOX_NAMESPACE

VJSBufferObject::VJSBufferObject (VSize inLength, void *inBuffer, bool inFromMemoryBuffer)
{
	xbox_assert(inLength >= 0);
	xbox_assert(!(inLength > 0 && inBuffer == NULL));

	fParent = NULL;
	fLength = inLength;
	fBuffer = (uBYTE *) inBuffer;
	fFromMemoryBuffer = inFromMemoryBuffer;
}

VJSBufferObject::VJSBufferObject (VSize inLength)
{
	xbox_assert(inLength >= 0);

	fParent = NULL;
	fLength = inLength;
	fBuffer = inLength ? (uBYTE *) malloc(inLength) : NULL;
	fFromMemoryBuffer = false;
}

CharSet VJSBufferObject::GetEncodingType (const XBOX::VString &inEncodingName)
{
	if (inEncodingName.EqualToUSASCIICString("utf8"))

		return XBOX::VTC_UTF_8;

	#if VERSIONWIN
	else if (inEncodingName.EqualToUSASCIICString("windowsANSICodePage"))

		return VTC_Win32Ansi;
	#endif

	else if (inEncodingName.EqualToUSASCIICString("ascii"))

		return (CharSet) eENCODING_ASCII;

	else if (inEncodingName.EqualToUSASCIICString("ucs2"))

		return XBOX::VTC_UTF_16_SMALLENDIAN;

	else if (inEncodingName.EqualToUSASCIICString("base64"))

		return (CharSet) eENCODING_BASE64;

	else if (inEncodingName.EqualToUSASCIICString("binary"))

		return (CharSet) eENCODING_BINARY;

	else if (inEncodingName.EqualToUSASCIICString("hex"))

		return (CharSet) eENCODING_HEX;

	else

		return VTextConverters::Get()->GetCharSetFromName(inEncodingName);
}

void VJSBufferObject::ToString (CharSet inEncoding, sLONG inStart, sLONG inEnd, XBOX::VString *outString)
{
	xbox_assert(inEncoding != XBOX::VTC_UNKNOWN);
	xbox_assert(inStart >= 0 && inEnd <= fLength && inStart <= inEnd);
	xbox_assert(outString != NULL);

	outString->Clear();
	switch (inEncoding) {

		case (XBOX::CharSet) eENCODING_ASCII: {

			sLONG	n;
			uBYTE	*p;

			for (n = inEnd - inStart, p = &fBuffer[inStart]; n != 0; n--, p++)

				outString->AppendUniChar(*p & 0x7f);

			break;

		}

		case VTC_UTF_16_SMALLENDIAN: {

			// No guarantee for 2 bytes alignment or length, but ok for Intel processors.

			outString->AppendUniChars((UniChar *) &fBuffer[inStart], (inEnd - inStart) / 2);
			break;

		}

		case (XBOX::CharSet) eENCODING_BASE64: {

			VMemoryBuffer<> resultBuffer;
			bool ok = Base64Coder::Encode(&fBuffer[inStart], inEnd - inStart, resultBuffer);
			if (ok)
			{
				outString->FromBlock( resultBuffer.GetDataPtr(), resultBuffer.GetDataSize(), VTC_UTF_8);
			}
			break;

		}

		case (XBOX::CharSet) eENCODING_BINARY: {

			sLONG	n;
			uBYTE	*p;

			for (n = inEnd - inStart, p = &fBuffer[inStart]; n != 0; n--, p++)

				outString->AppendUniChar(*p & 0xff);

			break;

		}

		case (XBOX::CharSet) eENCODING_HEX: {

			sLONG	n;
			uBYTE	*p;

			for (n = inEnd - inStart, p = &fBuffer[inStart]; n != 0; n--, p++) {

				outString->AppendUniChar(_ToHex((*p >> 4) & 0xf));
				outString->AppendUniChar(_ToHex(*p & 0xf));

			}

			break;

		}

		default: {

			outString->FromBlock(&fBuffer[inStart], inEnd - inStart, inEncoding);
			break;

		}

	}
}

sLONG VJSBufferObject::FromString (const XBOX::VString &inString, CharSet inEncoding, uBYTE **outBuffer, sLONG inMaximumLength)
{
	xbox_assert(outBuffer != NULL);
	xbox_assert(!(*outBuffer != NULL && inMaximumLength < 0));

	sLONG	r;

	r = -1;
	switch (inEncoding) {

		case (XBOX::CharSet) eENCODING_ASCII:
		case (XBOX::CharSet) eENCODING_BINARY: {

			sLONG	size;

			size = inString.GetLength();
			if (inMaximumLength >= 0 && inMaximumLength < size)

				size = inMaximumLength;

			if (!size) {

				r = 0;
				break;

			}

			uBYTE	*encodedData;

			if (*outBuffer != NULL)

				encodedData = *outBuffer;

			else if ((encodedData = (uBYTE *) ::malloc(size)) == NULL) {

				XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
				break;

			} else

				*outBuffer = encodedData;

			uBYTE	mask;
			sLONG	i;

			mask = inEncoding == eENCODING_ASCII ? 0x7f : 0xff;
			for (i = 0; i < inString.GetLength(); i++)

				encodedData[i] = inString.GetUniChar(i + 1) & mask;

			r = size;

			break;

		}

		case XBOX::VTC_UTF_16_SMALLENDIAN: {

			sLONG	size;

			size = inString.GetLength() * 2;
			if (inMaximumLength >= 0 && inMaximumLength < size)

				size = inMaximumLength;

			if (size & 1)

				size--;		// inMaximumLength can be an odd number.

			if (!size) {

				r = 0;
				break;

			}

			if (*outBuffer == NULL && (*outBuffer = (uBYTE *) ::malloc(size)) == NULL)

				XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

			else {

				::memcpy(*outBuffer, inString.GetCPointer(), size);
				r = size;

			}

			break;

		}

		case (XBOX::CharSet) eENCODING_BASE64: {

			VStringConvertBuffer inputBuffer( inString, VTC_UTF_8);
			VMemoryBuffer<> resultBuffer;
			bool ok = Base64Coder::Decode( inputBuffer.GetCPointer(), inputBuffer.GetSize(), resultBuffer);

			if (ok)
			{
				if (*outBuffer != NULL)
				{
					// ca n'a pas de sens de tronquer une stream Base64?
					r = std::min<sLONG>( inMaximumLength, (sLONG) resultBuffer.GetDataSize());
					::memcpy(*outBuffer, resultBuffer.GetDataPtr(), r);
				}
				else
				{
					*outBuffer = (uBYTE*) resultBuffer.GetDataPtr();
					r = (sLONG) resultBuffer.GetDataSize();
					resultBuffer.ForgetData();
				}
			}

			break;

		}

		case (XBOX::CharSet) eENCODING_HEX: {

			sLONG	size;

			size = inString.GetLength() / 2;
			if (inMaximumLength >= 0 && inMaximumLength < size)

				size = inMaximumLength;

			if (size & 1)

				size--;		// inMaximumLength can be an odd number.

			if (!size) {

				r = 0;
				break;

			}

			uBYTE	*encodedData;

			if (*outBuffer != NULL)

				encodedData = *outBuffer;

			else if ((encodedData = (uBYTE *) ::malloc(size)) == NULL) {

				XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
				break;

			}

			uLONG			i;
			const UniChar	*p;
			uBYTE			*q;

			p = inString.GetCPointer();
			q = encodedData;
			for (i = 0; i < size; i++) {

				sLONG	c;

				if ((c = _FromHex(*p++)) < 0)

					break;

				*q = c << 4;

				if ((c = _FromHex(*p++)) < 0)

					break;

				*q |= c;

				q++;

			}

			if (i == size) {

				*outBuffer = encodedData;
				r = size;

			} else {

				// An error occured.

				if (outBuffer == NULL)

					::free(encodedData);

			}

			break;

		}

		default: {

			if (*outBuffer != NULL)

				r = (sLONG) inString.ToBlock(*outBuffer, inMaximumLength, inEncoding, false, false);

			else {

				// Note that data is copied from buffer used to make conversion to actual allocated buffer.
				// If the string to encode is long, this is not a good thing.

				XBOX::VStringConvertBuffer buffer(inString, inEncoding);

				if ((r = (sLONG) buffer.GetSize()) > 0) {

					if ((*outBuffer = (uBYTE *) ::malloc(r)) == NULL)

						XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

					else

						::memcpy(*outBuffer, buffer.GetCPointer(), r);

				}

			}
			break;

		}

	}
	return r;
}

VJSBufferObject::VJSBufferObject (VJSBufferObject *inParent, sLONG inStart, sLONG inEnd)
{
	xbox_assert(inParent != NULL);
	xbox_assert(inStart >= 0 && inStart <= inEnd);

	inParent->Retain();

	fParent = inParent;
	fLength = inEnd - inStart;
	fBuffer = &inParent->fBuffer[inStart];
	fFromMemoryBuffer = false;
}

VJSBufferObject::~VJSBufferObject ()
{
	if (fParent != NULL)

		ReleaseRefCountable(&fParent);

	else if (fBuffer != NULL) {

		if (fFromMemoryBuffer)

			VMemory::DisposePtr((void *) fBuffer);

		else

			::free((void *) fBuffer);

	}

	fBuffer	= NULL;
}

UniChar VJSBufferObject::_ToHex (uBYTE inValue)
{
	xbox_assert(inValue >= 0 && inValue <= 0xf);

	if (inValue < 10)

		return '0' + inValue;

	else

		return 'a' + (inValue - 10);
}

sLONG VJSBufferObject::_FromHex (UniChar inUniChar)
{
	if (inUniChar >= '0' && inUniChar <= '9')

		return inUniChar - '0';

	else if (inUniChar >= 'a' && inUniChar <= 'f')

		return 10 + inUniChar - 'a';

	else if (inUniChar >= 'A' && inUniChar <= 'F')

		return 10 + inUniChar - 'A';

	else

		return -1;
}

JS4D::StaticFunction VJSBufferClass::sConstrFunctions[] =
{
	{	"isBuffer",			XBOX::js_callback<VJSBufferObject, VJSBufferClass::_IsBuffer>,	JS4D::PropertyAttributeDontDelete	},
	{	"byteLength",		XBOX::js_callback<VJSBufferObject, VJSBufferClass::_ByteLength>,	JS4D::PropertyAttributeDontDelete	},
	{	0,				0,										0									},
};

void VJSBufferClass::GetDefinition (ClassDefinition &outDefinition)
{

	static inherited::StaticFunction functions[] =
	{
		{	"write",			js_callStaticFunction<_write>,			JS4D::PropertyAttributeDontDelete	},
		{	"toString",			js_callStaticFunction<_toString>,		JS4D::PropertyAttributeDontDelete	},
		{	"toBlob",			js_callStaticFunction<_toBlob>,			JS4D::PropertyAttributeDontDelete	},
//		{	"toArrayBuffer",	js_callStaticFunction<_toArrayBuffer>,	JS4D::PropertyAttributeDontDelete	},

		{	"copy",				js_callStaticFunction<_copy>,			JS4D::PropertyAttributeDontDelete	},
		{	"slice",			js_callStaticFunction<_slice>,			JS4D::PropertyAttributeDontDelete	},
		{	"fill",				js_callStaticFunction<_fill>,			JS4D::PropertyAttributeDontDelete	},

		{	"readUInt8",		js_callStaticFunction<_readUInt8>,		JS4D::PropertyAttributeDontDelete	},
		{	"readInt8",			js_callStaticFunction<_readInt8>,		JS4D::PropertyAttributeDontDelete	},
		{	"writeUInt8",		js_callStaticFunction<_writeUInt8>,		JS4D::PropertyAttributeDontDelete	},
		{	"writeInt8",		js_callStaticFunction<_writeInt8>,		JS4D::PropertyAttributeDontDelete	},

		{	"readUInt16LE",		js_callStaticFunction<_readUInt16LE>,	JS4D::PropertyAttributeDontDelete	},
		{	"readUInt16BE",		js_callStaticFunction<_readUInt16BE>,	JS4D::PropertyAttributeDontDelete	},
		{	"readInt16LE",		js_callStaticFunction<_readInt16LE>,	JS4D::PropertyAttributeDontDelete	},
		{	"readInt16BE",		js_callStaticFunction<_readInt16BE>,	JS4D::PropertyAttributeDontDelete	},
		{	"writeUInt16LE",	js_callStaticFunction<_writeUInt16LE>,	JS4D::PropertyAttributeDontDelete	},
		{	"writeUInt16BE",	js_callStaticFunction<_writeUInt16BE>,	JS4D::PropertyAttributeDontDelete	},
		{	"writeInt16LE",		js_callStaticFunction<_writeInt16LE>,	JS4D::PropertyAttributeDontDelete	},
		{	"writeInt16BE",		js_callStaticFunction<_writeInt16BE>,	JS4D::PropertyAttributeDontDelete	},

		{	"readUInt24LE",		js_callStaticFunction<_readUInt24LE>,	JS4D::PropertyAttributeDontDelete	},
		{	"readUInt24BE",		js_callStaticFunction<_readUInt24BE>,	JS4D::PropertyAttributeDontDelete	},
		{	"readInt24LE",		js_callStaticFunction<_readInt24LE>,	JS4D::PropertyAttributeDontDelete	},
		{	"readInt24BE",		js_callStaticFunction<_readInt24BE>,	JS4D::PropertyAttributeDontDelete	},
		{	"writeUInt24LE",	js_callStaticFunction<_writeUInt24LE>,	JS4D::PropertyAttributeDontDelete	},
		{	"writeUInt24BE",	js_callStaticFunction<_writeUInt24BE>,	JS4D::PropertyAttributeDontDelete	},
		{	"writeInt24LE",		js_callStaticFunction<_writeInt24LE>,	JS4D::PropertyAttributeDontDelete	},
		{	"writeInt24BE",		js_callStaticFunction<_writeInt24BE>,	JS4D::PropertyAttributeDontDelete	},

		{	"readUInt32LE",		js_callStaticFunction<_readUInt32LE>,	JS4D::PropertyAttributeDontDelete	},
		{	"readUInt32BE",		js_callStaticFunction<_readUInt32BE>,	JS4D::PropertyAttributeDontDelete	},
		{	"readInt32LE",		js_callStaticFunction<_readInt32LE>,	JS4D::PropertyAttributeDontDelete	},
		{	"readInt32BE",		js_callStaticFunction<_readInt32BE>,	JS4D::PropertyAttributeDontDelete	},
		{	"writeUInt32LE",	js_callStaticFunction<_writeUInt32LE>,	JS4D::PropertyAttributeDontDelete	},
		{	"writeUInt32BE",	js_callStaticFunction<_writeUInt32BE>,	JS4D::PropertyAttributeDontDelete	},
		{	"writeInt32LE",		js_callStaticFunction<_writeInt32LE>,	JS4D::PropertyAttributeDontDelete	},
		{	"writeInt32BE",		js_callStaticFunction<_writeInt32BE>,	JS4D::PropertyAttributeDontDelete	},

		{	"readFloatLE",		js_callStaticFunction<_readFloatLE>,	JS4D::PropertyAttributeDontDelete	},
		{	"readFloatBE",		js_callStaticFunction<_readFloatBE>,	JS4D::PropertyAttributeDontDelete	},
		{	"writeFloatLE",		js_callStaticFunction<_writeFloatLE>,	JS4D::PropertyAttributeDontDelete	},
		{	"writeFloatBE",		js_callStaticFunction<_writeFloatBE>,	JS4D::PropertyAttributeDontDelete	},

		{	"readDoubleLE",		js_callStaticFunction<_readDoubleLE>,	JS4D::PropertyAttributeDontDelete	},
		{	"readDoubleBE",		js_callStaticFunction<_readDoubleBE>,	JS4D::PropertyAttributeDontDelete	},
		{	"writeDoubleLE",	js_callStaticFunction<_writeDoubleLE>,	JS4D::PropertyAttributeDontDelete	},
		{	"writeDoubleBE",	js_callStaticFunction<_writeDoubleBE>,	JS4D::PropertyAttributeDontDelete	},

		{	0,				0,										0									},
	};

    outDefinition.className			= "Buffer";
	outDefinition.staticFunctions	= functions;
	outDefinition.initialize		= js_initialize<Initialize>;
	outDefinition.finalize			= js_finalize<_Finalize>;
	outDefinition.getProperty		= js_getProperty<_GetProperty>;
	outDefinition.setProperty		= js_setProperty<_SetProperty>;
}

XBOX::VJSObject VJSBufferClass::NewInstance (XBOX::VJSContext inContext, VSize inLength, void *inBuffer, bool inFromMemoryBuffer)
{
	xbox_assert(inLength >= 0);
	xbox_assert(!(inLength > 0 && inBuffer == NULL));

	XBOX::VJSObject	object(inContext);
	VJSBufferObject	*buffer;

	if ((buffer = new VJSBufferObject(inLength, inBuffer, inFromMemoryBuffer)) == NULL) {

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

	} else {

		// Initialization of created object will do a Retain() on buffer.

		object = VJSBufferClass::CreateInstance(inContext, buffer);

	}

	buffer->Release();
	return object;
}

XBOX::VJSObject	VJSBufferClass::MakeConstructor (const XBOX::VJSContext& inContext)
{
	XBOX::VJSObject	bufferConstructor(inContext);

	bufferConstructor = JS4DMakeConstructor(inContext, Class(), _Construct, false,	VJSBufferClass::sConstrFunctions );

	return bufferConstructor;
}


void VJSBufferClass::_Construct( VJSParms_construct &ioParms)
{
	if (!ioParms.CountParams()) {

		XBOX::vThrowError(XBOX::VE_JVSC_EXPECTING_PARAMETER, "Buffer");
		return;

	}

	VJSBufferObject	*buffer;

	if (ioParms.IsNumberParam(1)) {

		sLONG	size;

		if (!ioParms.GetLongParam(1, &size)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
			return;

		}
		if (size < 0) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_NUMBER_ARGUMENT, "1");
			return;

		}
		buffer = new VJSBufferObject(size);

	} else if (ioParms.IsArrayParam(1)) {

		XBOX::VJSArray	arrayObject(ioParms.GetContext());

		if (!ioParms.GetParamArray(1, arrayObject)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_ARRAY, "1");
			return;

		}

		buffer = new VJSBufferObject((sLONG) arrayObject.GetLength());
		if (buffer != NULL && buffer->fBuffer != NULL) {

			XBOX::VJSObject				arrObj = arrayObject;
			XBOX::VJSPropertyIterator	i(arrObj);
			VSize						j;

			for (j = 0; i.IsValid(); ++i) {

				sLONG	value;

				if (!i.GetPropertyNameAsLong(&value))

					continue;

				if (j >= buffer->fLength)

					break;	// Should be impossible.

				VJSValue	tmpVal = i.GetProperty();
				tmpVal.GetLong(&value);
				buffer->fBuffer[j++] = (uBYTE) value;

			}

			xbox_assert(j == buffer->fLength);

		}

	} else if (ioParms.IsStringParam(1)) {

		XBOX::VString	string, encodingName;
		CharSet			encoding;

		if (!ioParms.GetStringParam(1, string)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
			return;

		}

		if (ioParms.CountParams() >= 2) {

			if (!ioParms.IsStringParam(2) || !ioParms.GetStringParam(2, encodingName)) {

				XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2");
				return;

			}
			if ((encoding = VJSBufferObject::GetEncodingType(encodingName)) == XBOX::VTC_UNKNOWN) {

				XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_UNSUPPORTED_ENCODING, encodingName);
				return;

			}

		} else

			encoding = XBOX::VTC_UTF_8;

		uBYTE	*encodedString;
		sLONG	length;

		encodedString = NULL;
		if ((length = VJSBufferObject::FromString(string, encoding, &encodedString)) < 0) {

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_ENCODING_FAILED);
			return;

		} else {

			if ((buffer = new VJSBufferObject(length, encodedString)) == NULL && length)

				::free(encodedString);

		}

	} else {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER, "1");
		return;

	}

	if (buffer == NULL || (buffer->fLength && buffer->fBuffer == NULL)) {

		if (buffer != NULL)

			buffer->Release();

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

	} else {

		ioParms.ReturnConstructedObject<VJSBufferClass>(buffer);
		buffer->Release();

	}
}

void VJSBufferClass::Initialize (const VJSParms_initialize &inParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	inBuffer->Retain();
	inParms.GetObject().SetProperty("length", (sLONG) inBuffer->fLength, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete );
}

void VJSBufferClass::_Finalize (const XBOX::VJSParms_finalize &inParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	inBuffer->Release();
}

void VJSBufferClass::_GetProperty (XBOX::VJSParms_getProperty &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	XBOX::VString	propertyName;
	sLONG			index;

	if (ioParms.GetPropertyName(propertyName) && !propertyName.IsEmpty()
	&& propertyName.GetUniChar(1) >= '0' && propertyName.GetUniChar(1) <= '9'
	&& ioParms.GetPropertyNameAsLong(&index))
	{

		// If the index is not valid, return undefined (just like NodeJS).

		if (index < 0 || index >= inBuffer->fLength)
		{
			ioParms.ReturnUndefinedValue();
		}
		else
		{
			/*if (index == 11)
			{
				uBYTE ub = inBuffer->fBuffer[index];
				int a = 1;
				DebugMsg("VJSBufferClass::_GetProperty fBuf=%x  -> \n   ... ", inBuffer->fBuffer);
				for (int idx = 0; idx < 16; idx++)
				{
					DebugMsg(" %x", inBuffer->fBuffer[idx]);
				}
				DebugMsg("\n");
			}*/
			ioParms.ReturnNumber<uBYTE>(inBuffer->fBuffer[index]);
		}
	}
	else
	{
		ioParms.ForwardToParent();
	}
}

bool VJSBufferClass::_SetProperty (XBOX::VJSParms_setProperty &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	// Only way to write directly into a Buffer object, is through "indexing".

	sLONG	index;

	if (ioParms.GetPropertyNameAsLong(&index))
	{
		// If index is not valid, do nothing.

		if (index >= 0 && index < inBuffer->fLength) {

			sLONG	value;

			// Value can be a number or a string. For a string, first character is taken.

			if (ioParms.IsValueNumber()) {

				if (!ioParms.GetPropertyValue().GetLong(&value))

					value = 0;

			} else if (ioParms.IsValueString()) {

				XBOX::VString	string;

				if (!ioParms.GetPropertyValue().GetString(string) || !string.GetLength())

					value = 0;

				else

					value = string.GetUniChar(1);

			} else

				value = 0;

			inBuffer->fBuffer[index] = value;
			/*if (index == 11)
			{
				DebugMsg("VJSBufferClass::_SetProperty value=%x  fBuf=%x  -> \n   ... ", value, inBuffer->fBuffer);
				for (int idx = 0; idx < 16; idx++)
				{
					DebugMsg(" %x", inBuffer->fBuffer[idx]);
				}
				DebugMsg("\n");
				uBYTE ub = inBuffer->fBuffer[index];
				int a = 1;
			}*/
		}
		return true;

	}
	else

		return false;	// "Forward" to parent.
}

void VJSBufferClass::_IsBuffer (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *)
{
	VJSObject	object(ioParms.GetContext());

	if (ioParms.CountParams() >= 1 && ioParms.IsObjectParam(1) && ioParms.GetParamObject(1, object))

		ioParms.ReturnBool(object.IsInstanceOf("Buffer"));

	else

		ioParms.ReturnBool(false);
}

void VJSBufferClass::_ByteLength (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *)
{
	XBOX::VString	string;

	if (!ioParms.IsStringParam(1) || !ioParms.GetStringParam(1, string)) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
		return;

	}

	CharSet	encoding;

	if (ioParms.CountParams() >= 2) {

		XBOX::VString	encodingName;

		if (!ioParms.IsStringParam(2) || !ioParms.GetStringParam(2, encodingName)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2");
			return;

		}

		if ((encoding = VJSBufferObject::GetEncodingType(encodingName)) == XBOX::VTC_UNKNOWN) {

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_UNSUPPORTED_ENCODING, encodingName);
			return;

		}

	} else

		encoding = XBOX::VTC_UTF_8;

	switch (encoding) {

		case (XBOX::CharSet) VJSBufferObject::eENCODING_ASCII:
		case (XBOX::CharSet) VJSBufferObject::eENCODING_BINARY: {

			ioParms.ReturnNumber<uLONG>(string.GetLength());	// Strip high bits, keeping 7 or 8 bits.
			break;

		}

		case XBOX::VTC_UTF_16_SMALLENDIAN: {

			ioParms.ReturnNumber<uLONG>(string.GetLength() * 2);
			break;

		}

		case (XBOX::CharSet) VJSBufferObject::eENCODING_HEX: {

			// 2 hex characters are needed for one byte.

			ioParms.ReturnNumber<uLONG>(string.GetLength() / 2);
			break;

		}

		case (XBOX::CharSet) VJSBufferObject::eENCODING_BASE64:
		default: {

			// To determine length of encoded buffer, do the actual encoding. This is slow!

			uBYTE	*encodedBuffer	= NULL;
			sLONG	length;

			if ((length = VJSBufferObject::FromString(string, encoding, &encodedBuffer)) >= 0) {

				if (encodedBuffer != NULL)

					::free(encodedBuffer);

				ioParms.ReturnNumber<sLONG>(length);

			} else {

				// An error occured, do not throw XBOX::VE_JVSC_BUFFER_ENCODING_FAILED, but return undefined instead.

				ioParms.ReturnUndefinedValue();

			}
			break;

		}

	}
}

void VJSBufferClass::_write (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	XBOX::VString	string;

	if (!ioParms.IsStringParam(1) || !ioParms.GetStringParam(1, string)) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
		return;

	}

	sLONG	offset, length;
	CharSet	encoding;

	offset = 0;
	length = (sLONG) inBuffer->fLength;
	encoding = XBOX::VTC_UTF_8;

	bool			ignoreRemainingArgument;
	XBOX::VString	encodingName;

	ignoreRemainingArgument = false;
	if (ioParms.CountParams() >= 2) {

		if (ioParms.IsNumberParam(2)) {

			if (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &offset)) {

				XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");
				return;

			}
			if (offset < 0) {

				XBOX::vThrowError(XBOX::VE_JVSC_WRONG_NUMBER_ARGUMENT, "2");
				return;

			}
			if (offset >= inBuffer->fLength) {

				XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);
				return;

			}

		} else if (!ioParms.IsStringParam(2) || !ioParms.GetStringParam(2, encodingName)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2");
			return;

		} else {

			encoding = VJSBufferObject::GetEncodingType(encodingName);
			ignoreRemainingArgument = true;

		}

	}

	if (!ignoreRemainingArgument && ioParms.CountParams() >= 3) {

		if (ioParms.IsNumberParam(3)) {

			if (!ioParms.IsNumberParam(3) || !ioParms.GetLongParam(3, &length)) {

				XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "3");
				return;

			}
			if (length < 0) {

				XBOX::vThrowError(XBOX::VE_JVSC_WRONG_NUMBER_ARGUMENT, "3");
				return;

			}

		} else if (!ioParms.IsStringParam(3) || !ioParms.GetStringParam(3, encodingName)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "3");
			return;

		} else {

			encoding = VJSBufferObject::GetEncodingType(encodingName);
			ignoreRemainingArgument = true;

		}

	}

	if (!ignoreRemainingArgument && ioParms.CountParams() >= 4) {

		if (!ioParms.IsStringParam(4) || !ioParms.GetStringParam(4, encodingName)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "4");
			return;

		} else

			encoding = VJSBufferObject::GetEncodingType(encodingName);

	}

	if (encoding == XBOX::VTC_UNKNOWN) {

		XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_UNSUPPORTED_ENCODING, encodingName);
		return;

	}

	if (!length) {

		ioParms.GetThis().SetProperty("_charsWritten", (sLONG) 0);
		ioParms.ReturnNumber<sLONG>(0);

	} else {

		uBYTE	*p;
		sLONG	writtenBytes;

		p = &inBuffer->fBuffer[offset];
		if (length > inBuffer->fLength - offset)

			length = ((sLONG) inBuffer->fLength) - offset;

		if ((writtenBytes = VJSBufferObject::FromString(string, encoding, &p, length)) >= 0) {

			ioParms.GetThis().SetProperty("_charsWritten", writtenBytes);
			ioParms.ReturnNumber<sLONG>(writtenBytes);

		} else {

			// Silently ignore failed encoding.

			ioParms.GetThis().SetProperty("_charsWritten", (sLONG) 0);
			ioParms.ReturnUndefinedValue();

		}

	}
}

void VJSBufferClass::_toString (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	CharSet	encoding;
	sLONG	start, end;

	encoding = XBOX::VTC_UTF_8;
	start = 0;
	end = (sLONG) inBuffer->fLength;

	if (ioParms.CountParams() >= 1) {

		XBOX::VString	encodingName;

		if (!ioParms.IsStringParam(1) || !ioParms.GetStringParam(1, encodingName)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
			return;

		} else

			encoding = VJSBufferObject::GetEncodingType(encodingName);

	}

	if (ioParms.CountParams() >= 2 && (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &start))) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");
		return;

	}

	if (ioParms.CountParams() >= 3 && (!ioParms.IsNumberParam(3) || !ioParms.GetLongParam(3, &end))) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "3");
		return;

	}

	if (start > end) {

		XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);
		return;

	}

	XBOX::VString	decodedString;

	inBuffer->ToString(encoding, start, end, &decodedString);
	ioParms.ReturnString(decodedString);
}

void VJSBufferClass::_toBlob(XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	VString type;

	// If content type is unspecified, then use empty string (unable to determine type) as default.
	// Same behavior as constructor and W3C FileAPI specification.

	if (!ioParms.GetStringParam(1, type))
		type.Clear();

	VJSBlobValue_Slice *blob = VJSBlobValue_Slice::Create( inBuffer->GetDataPtr(), inBuffer->GetDataSize(), type);
	if (blob == NULL)
	{
		XBOX::vThrowError( VE_MEMORY_FULL);
		ioParms.ReturnUndefinedValue();
	}
	else
	{
		ioParms.ReturnValue( VJSBlob::CreateInstance( ioParms.GetContext(), blob));
	}
	ReleaseRefCountable( &blob);
}

void VJSBufferClass::_toArrayBuffer (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	VJSArrayBufferObject	*arrayBuffer;

	if ((arrayBuffer = new VJSArrayBufferObject(inBuffer)) == NULL) {

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
		ioParms.ReturnUndefinedValue();

	} else {

		xbox_assert(arrayBuffer->GetBufferObject() == inBuffer);

		ioParms.ReturnValue(VJSArrayBufferClass::CreateInstance(ioParms.GetContext(), arrayBuffer));
		arrayBuffer->Release();

	}
}

void VJSBufferClass::_copy (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	XBOX::VJSObject	targetObject(ioParms.GetContext());

	if (!ioParms.GetParamObject(1, targetObject) || !targetObject.IsInstanceOf("Buffer")) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BUFFER, "1");
		return;

	}

	VJSBufferObject	*targetBuffer;

	targetBuffer = targetObject.GetPrivateData<VJSBufferClass>();
	xbox_assert(targetBuffer != NULL);

	// Copy [start, end) (note that end is not inside slice) to &target[offset].

	sLONG	offset, start, end;

	offset = start = 0;
	end = (sLONG) inBuffer->fLength;

	if (ioParms.CountParams() >= 2) {

		if (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &offset)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");
			return;

		}
		if (offset < 0 || offset >= targetBuffer->fLength) {

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);
			return;

		}

	}

	if (ioParms.CountParams() >= 3) {

		if (!ioParms.IsNumberParam(3) || !ioParms.GetLongParam(3, &start)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "3");
			return;

		}

		// Use '>' comparison instead of '>=' to allow zero length buffer, see comment below.

		if (start < 0 || start > inBuffer->fLength) {

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);
			return;

		}

	}

	if (ioParms.CountParams() >= 4) {

		if (!ioParms.IsNumberParam(4) || !ioParms.GetLongParam(4, &end)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "4");
			return;

		}
		if (end < 0 || end > inBuffer->fLength) {

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);
			return;

		}

	}

	// Zero length copies are possible, they do nothing. It is also possible to copy from [0, 0) from a zero length
	// buffer, this will also do nothing. Otherwise start must be within the buffer's bound.

	if (inBuffer->fLength && start == inBuffer->fLength)

		XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	else if (start != end) {

		sLONG	length;

		if (start > end || (length = end - start) > targetBuffer->fLength - offset)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else

			// VMemory::CopyBlock() is implemented using ::memmove(). Buffer copies on itself are possible.

			XBOX::VMemory::CopyBlock(&inBuffer->fBuffer[start], &targetBuffer->fBuffer[offset], length);

	}
}

void VJSBufferClass::_slice (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	// Get slice [start, end) (note that end is not inside slice).

	sLONG	start, end;

	start = 0;
	end = (sLONG) inBuffer->fLength;

	if (ioParms.CountParams()) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &start)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
			return;

		}
		if (ioParms.CountParams() > 1 && (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &end))) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");
			return;

		}
		if (start < 0 || end > inBuffer->fLength || start > end) {

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);
			return;

		}

	}

	// Note that (start == end) is legal.

	VJSBufferObject	*buffer;

	if ((buffer = new VJSBufferObject(inBuffer, start, end)) == NULL) {

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
		ioParms.ReturnNullValue();

	} else {

		ioParms.ReturnValue(VJSBufferClass::CreateInstance(ioParms.GetContext(), buffer));
		buffer->Release();

	}
}

void VJSBufferClass::_fill (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	Real	value;

	if (!ioParms.IsNumberParam(1) && !ioParms.IsStringParam(1)) {

		XBOX::vThrowError(XBOX::VE_JVSC_EXPECTING_PARAMETER, "Buffer.fill()");
		return;

	}

	if (ioParms.IsNumberParam(1)) {

		if (!ioParms.GetRealParam(1, &value)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
			return;

		}

	} else {

		XBOX::VString	string;

		if (!ioParms.GetStringParam(1, string)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
			return;

		}

		if (string.GetLength())

			// Take value of first character.

			value = string.GetUniChar(1);

		else

			value = 0;	// Empty string, consider value as zero.

	}

	sLONG	offset, length;

	offset = 0;
	length = (sLONG) inBuffer->fLength;

	if (ioParms.CountParams() >= 2) {

		if (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &offset)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");
			return;

		}
		if (offset < 0 || offset >= length) {

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);
			return;

		}

	}

	if (ioParms.CountParams() >= 3) {

		if (!ioParms.IsNumberParam(3) || !ioParms.GetLongParam(3, &length)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "3");
			return;

		}
		if (length < 0 || offset + length > inBuffer->fLength) {

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);
			return;

		}

	}

	::memset(&inBuffer->fBuffer[offset], (uBYTE) value, length);
}

void VJSBufferClass::_readUInt8 (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	offset;

	if (ioParms.CountParams() == 1) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (offset < 0 || offset >= inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else

			ioParms.ReturnNumber<uBYTE>(inBuffer->fBuffer[offset]);

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsBooleanParam(2) || !ioParms.GetBoolParam(2, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "2");

		else if (offset >= 0 && offset < inBuffer->fLength)

			ioParms.ReturnNumber<uBYTE>(inBuffer->fBuffer[offset]);

		else if (noChecking)

			ioParms.ReturnNumber<uBYTE>(0);

		else

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

void VJSBufferClass::_readInt8 (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	offset;

	if (ioParms.CountParams() == 1) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (offset < 0 || offset >= inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else

			ioParms.ReturnNumber<sBYTE>(inBuffer->fBuffer[offset]);

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsBooleanParam(2) || !ioParms.GetBoolParam(2, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "2");

		else if (offset >= 0 && offset < inBuffer->fLength)

			ioParms.ReturnNumber<sBYTE>(inBuffer->fBuffer[offset]);

		else if (noChecking)

			ioParms.ReturnNumber<sBYTE>(0);

		else

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

void VJSBufferClass::_writeUInt8 (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	value, offset;

	if (ioParms.CountParams() == 2) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &value))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");

		else if (offset < 0 || offset >= inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else

			inBuffer->fBuffer[offset] = value;

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &value))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");

		else if (!ioParms.IsBooleanParam(3) || !ioParms.GetBoolParam(3, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "3");

		else if (offset >= 0 && offset < inBuffer->fLength)

			inBuffer->fBuffer[offset] = value;

		else if (noChecking)

			;	// Out of bound, do not write, silently ignore.

		else

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

void VJSBufferClass::_writeInt8 (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	VJSBufferClass::_writeUInt8(ioParms, inBuffer);
}

void VJSBufferClass::_readUInt16LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	offset;

	if (ioParms.CountParams() == 1) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (offset < 0 || offset + 2 > inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else

			ioParms.ReturnNumber<uWORD>(*((uWORD *) &inBuffer->fBuffer[offset]));

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsBooleanParam(2) || !ioParms.GetBoolParam(2, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "2");

		else if (offset >= 0 && offset + 2 <= inBuffer->fLength)

			ioParms.ReturnNumber<uWORD>(*((uWORD *) &inBuffer->fBuffer[offset]));

		else if (noChecking) {

			uWORD	result;

			if (offset >= 0 && offset < inBuffer->fLength)

				result = inBuffer->fBuffer[offset];

			else

				result = 0;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset] << 8;

			ioParms.ReturnNumber<uWORD>(result);

		} else

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

void VJSBufferClass::_readUInt16BE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	offset;
	uWORD	result;

	if (ioParms.CountParams() == 1) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (offset < 0 || offset + 2 > inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else {

			result = inBuffer->fBuffer[offset] << 8;
			result |= inBuffer->fBuffer[offset + 1];

			ioParms.ReturnNumber<uWORD>(result);

		}

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsBooleanParam(2) || !ioParms.GetBoolParam(2, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "2");

		else if (offset >= 0 && offset + 2 <= inBuffer->fLength) {

			result = inBuffer->fBuffer[offset] << 8;
			result |= inBuffer->fBuffer[offset + 1];

			ioParms.ReturnNumber<uWORD>(result);

		} else if (noChecking) {

			if (offset >= 0 && offset < inBuffer->fLength)

				result = inBuffer->fBuffer[offset] << 8;

			else

				result = 0;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset];

			ioParms.ReturnNumber<uWORD>(result);

		} else

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

void VJSBufferClass::_readInt16LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	offset;

	if (ioParms.CountParams() == 1) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (offset < 0 || offset + 2 > inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else

			ioParms.ReturnNumber<sWORD>(*((sWORD *) &inBuffer->fBuffer[offset]));

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsBooleanParam(2) || !ioParms.GetBoolParam(2, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "2");

		else if (offset >= 0 && offset + 2 <= inBuffer->fLength)

			ioParms.ReturnNumber<sWORD>(*((sWORD *) &inBuffer->fBuffer[offset]));

		else if (noChecking) {

			sWORD	result;

			if (offset >= 0 && offset < inBuffer->fLength)

				result = inBuffer->fBuffer[offset];

			else

				result = 0;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset] << 8;

			ioParms.ReturnNumber<sWORD>(result);

		} else

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

void VJSBufferClass::_readInt16BE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	offset;
	sWORD	result;

	if (ioParms.CountParams() == 1) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (offset < 0 || offset + 2 > inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else {

			result = inBuffer->fBuffer[offset] << 8;
			result |= inBuffer->fBuffer[offset + 1];

			ioParms.ReturnNumber<sWORD>(result);

		}

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsBooleanParam(2) || !ioParms.GetBoolParam(2, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "2");

		else if (offset >= 0 && offset + 2 <= inBuffer->fLength) {

			result = inBuffer->fBuffer[offset] << 8;
			result |= inBuffer->fBuffer[offset + 1];

			ioParms.ReturnNumber<sWORD>(result);

		} else if (noChecking) {

			if (offset >= 0 && offset < inBuffer->fLength)

				result = inBuffer->fBuffer[offset] << 8;

			else

				result = 0;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset];

			ioParms.ReturnNumber<sWORD>(result);

		} else

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

void VJSBufferClass::_writeUInt16LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	value, offset;

	if (ioParms.CountParams() == 2) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &value))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");

		else if (offset < 0 || offset + 2 > inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else

			*((uWORD *) &inBuffer->fBuffer[offset]) = value;

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &value))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");

		else if (!ioParms.IsBooleanParam(3) || !ioParms.GetBoolParam(3, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "3");

		else if (offset >= 0 && offset + 2 <= inBuffer->fLength)

			*((uWORD *) &inBuffer->fBuffer[offset]) = value;

		else if (noChecking) {

			if (offset >= 0 && offset < inBuffer->fLength)

				inBuffer->fBuffer[offset] = value;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				inBuffer->fBuffer[offset] = value >> 8;

		} else

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

void VJSBufferClass::_writeUInt16BE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	value, offset;

	if (ioParms.CountParams() == 2) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &value))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");

		else if (offset < 0 || offset + 2 > inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else {

			inBuffer->fBuffer[offset] = value >> 8;
			inBuffer->fBuffer[offset + 1] = value;

		}

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &value))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");

		else if (!ioParms.IsBooleanParam(3) || !ioParms.GetBoolParam(3, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "3");

		else if (offset >= 0 && offset + 2 <= inBuffer->fLength) {

			inBuffer->fBuffer[offset] = value >> 8;
			inBuffer->fBuffer[offset + 1] = value;

		} else if (noChecking) {

			if (offset >= 0 && offset < inBuffer->fLength)

				inBuffer->fBuffer[offset] = value >> 8;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				inBuffer->fBuffer[offset] = value;

		} else

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

void VJSBufferClass::_writeInt16LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	VJSBufferClass::_writeUInt16LE(ioParms, inBuffer);
}

void VJSBufferClass::_writeInt16BE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	VJSBufferClass::_writeUInt16BE(ioParms, inBuffer);
}

void VJSBufferClass::_readUInt24LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	offset;
	uLONG	result;

	if (ioParms.CountParams() == 1) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (offset < 0 || offset + 3 > inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else {

			result = *((uWORD *) &inBuffer->fBuffer[offset]);
			result |= inBuffer->fBuffer[offset + 2] << 16;

			ioParms.ReturnNumber<uLONG>(result);

		}

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsBooleanParam(2) || !ioParms.GetBoolParam(2, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "2");

		else if (offset >= 0 && offset + 3 <= inBuffer->fLength) {

			result = *((uWORD *) &inBuffer->fBuffer[offset]);
			result |= inBuffer->fBuffer[offset + 2] << 16;

			ioParms.ReturnNumber<uLONG>(result);

		} else if (noChecking) {

			if (offset >= 0 && offset < inBuffer->fLength)

				result = inBuffer->fBuffer[offset];

			else

				result = 0;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset] << 8;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset] << 16;

			ioParms.ReturnNumber<uLONG>(result);

		} else

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

void VJSBufferClass::_readUInt24BE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	offset;
	uLONG	result;

	if (ioParms.CountParams() == 1) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (offset < 0 || offset + 3 > inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else {

			result = inBuffer->fBuffer[offset] << 16;
			result |= inBuffer->fBuffer[offset + 1] << 8;
			result |= inBuffer->fBuffer[offset + 2];

			ioParms.ReturnNumber<uLONG>(result);

		}

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsBooleanParam(2) || !ioParms.GetBoolParam(2, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "2");

		else if (offset > 0 && offset + 3 <= inBuffer->fLength) {

			result = inBuffer->fBuffer[offset] << 16;
			result |= inBuffer->fBuffer[offset + 1] << 8;
			result |= inBuffer->fBuffer[offset + 2];

			ioParms.ReturnNumber<uLONG>(result);

		} else if (noChecking) {

			if (offset >= 0 && offset < inBuffer->fLength)

				result = inBuffer->fBuffer[offset] << 16;

			else

				result = 0;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset] << 8;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset];

			ioParms.ReturnNumber<uLONG>(result);

		} else

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

void VJSBufferClass::_readInt24LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	offset, result;

	if (ioParms.CountParams() == 1) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (offset < 0 || offset + 3 > inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else {

			result = *((uWORD *) &inBuffer->fBuffer[offset]);
			result |= ((sBYTE) inBuffer->fBuffer[offset + 2]) << 16;

			ioParms.ReturnNumber<sLONG>(result);

		}

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsBooleanParam(2) || !ioParms.GetBoolParam(2, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "2");

		else if (offset >= 0 && offset + 3 <= inBuffer->fLength) {

			result = *((uWORD *) &inBuffer->fBuffer[offset]);
			result |= ((sBYTE) inBuffer->fBuffer[offset + 2]) << 16;

			ioParms.ReturnNumber<sLONG>(result);

		} else if (noChecking) {

			if (offset >= 0 && offset < inBuffer->fLength)

				result = inBuffer->fBuffer[offset];

			else

				result = 0;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset] << 8;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= ((sBYTE) inBuffer->fBuffer[offset]) << 16;

			ioParms.ReturnNumber<sLONG>(result);

		} else

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

void VJSBufferClass::_readInt24BE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	offset, result;

	if (ioParms.CountParams() == 1) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (offset < 0 || offset + 3 > inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else {

			result = ((sBYTE) inBuffer->fBuffer[offset]) << 16;
			result |= inBuffer->fBuffer[offset + 1] << 8;
			result |= inBuffer->fBuffer[offset + 2];

			ioParms.ReturnNumber<sLONG>(result);

		}

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsBooleanParam(2) || !ioParms.GetBoolParam(2, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "2");

		else if (offset >= 0 && offset + 3 <= inBuffer->fLength) {

			result = ((sBYTE) inBuffer->fBuffer[offset]) << 16;
			result |= inBuffer->fBuffer[offset + 1] << 8;
			result |= inBuffer->fBuffer[offset + 2];

			ioParms.ReturnNumber<sLONG>(result);

		} else if (noChecking) {

			if (offset >= 0 && offset < inBuffer->fLength)

				result = ((sBYTE) inBuffer->fBuffer[offset]) << 16;

			else

				result = 0;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset] << 8;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset];

			ioParms.ReturnNumber<sLONG>(result);

		} else

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

void VJSBufferClass::_writeUInt24LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	value, offset;

	if (ioParms.CountParams() == 2) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &value))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");

		else if (offset < 0 || offset + 3 > inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else {

			*((uWORD *) &inBuffer->fBuffer[offset]) = value;
			inBuffer->fBuffer[offset + 2] = value >> 16;

		}

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, (sLONG *) &value))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");

		else if (!ioParms.IsBooleanParam(3) || !ioParms.GetBoolParam(3, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "3");

		else if (offset >= 0 && offset + 3 <= inBuffer->fLength) {

			*((uWORD *) &inBuffer->fBuffer[offset]) = value;
			inBuffer->fBuffer[offset + 2] = value >> 16;

		} else if (noChecking) {

			if (offset >= 0 && offset < inBuffer->fLength)

				inBuffer->fBuffer[offset] = value;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				inBuffer->fBuffer[offset] = value >> 8;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				inBuffer->fBuffer[offset] = value >> 16;

		} else

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

void VJSBufferClass::_writeUInt24BE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	value, offset;

	if (ioParms.CountParams() == 2) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &value))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");

		else if (offset < 0 || offset + 3 > inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else {

			inBuffer->fBuffer[offset] = value >> 16;
			inBuffer->fBuffer[offset + 1] = value >> 8;
			inBuffer->fBuffer[offset + 2] = value;

		}

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, (sLONG *) &value))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");

		else if (!ioParms.IsBooleanParam(3) || !ioParms.GetBoolParam(3, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "3");

		else if (offset >= 0 && offset + 3 <= inBuffer->fLength) {

			inBuffer->fBuffer[offset] = value >> 16;
			inBuffer->fBuffer[offset + 1] = value >> 8;
			inBuffer->fBuffer[offset + 2] = value;

		} else if (noChecking) {

			if (offset >= 0 && offset < inBuffer->fLength)

				inBuffer->fBuffer[offset] = value >> 16;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				inBuffer->fBuffer[offset] = value >> 8;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				inBuffer->fBuffer[offset] = value;

		} else

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

void VJSBufferClass::_writeInt24LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	VJSBufferClass::_writeUInt24LE(ioParms, inBuffer);
}

void VJSBufferClass::_writeInt24BE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	VJSBufferClass::_writeUInt24BE(ioParms, inBuffer);
}

void VJSBufferClass::_readUInt32LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	offset;

	if (ioParms.CountParams() == 1) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (offset < 0 || offset + 4 > inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else

			ioParms.ReturnNumber<uLONG>(*((uLONG *) &inBuffer->fBuffer[offset]));

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsBooleanParam(2) || !ioParms.GetBoolParam(2, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "2");

		else if (offset >= 0 && offset + 4 <= inBuffer->fLength)

			ioParms.ReturnNumber<uLONG>(*((uLONG *) &inBuffer->fBuffer[offset]));

		else if (noChecking) {

			uLONG	result;

			if (offset >= 0 && offset < inBuffer->fLength)

				result = inBuffer->fBuffer[offset];

			else

				result = 0;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset] << 8;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset] << 16;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset] << 24;

			ioParms.ReturnNumber<uLONG>(result);

		} else

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

void VJSBufferClass::_readUInt32BE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	offset;
	uLONG	result;

	if (ioParms.CountParams() == 1) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (offset < 0 || offset + 4 > inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else {

			result  = inBuffer->fBuffer[offset] << 24;
			result |= inBuffer->fBuffer[offset + 1] << 16;
			result |= inBuffer->fBuffer[offset + 2] << 8;
			result |= inBuffer->fBuffer[offset + 3];

			ioParms.ReturnNumber<uLONG>(result);

		}

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsBooleanParam(2) || !ioParms.GetBoolParam(2, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "2");

		else if (offset >= 0 && offset + 4 <= inBuffer->fLength) {

			result = inBuffer->fBuffer[offset] << 24;
			result |= inBuffer->fBuffer[offset + 1] << 16;
			result |= inBuffer->fBuffer[offset + 2] << 8;
			result |= inBuffer->fBuffer[offset + 3];

			ioParms.ReturnNumber<uLONG>(result);

		} else if (noChecking) {

			if (offset >= 0 && offset < inBuffer->fLength)

				result = inBuffer->fBuffer[offset] << 24;

			else

				result = 0;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset] << 16;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset] << 8;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset];

			ioParms.ReturnNumber<uLONG>(result);

		} else

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

void VJSBufferClass::_readInt32LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	offset;

	if (ioParms.CountParams() == 1) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (offset < 0 || offset + 4 > inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else

			ioParms.ReturnNumber<sLONG>(*((sLONG *) &inBuffer->fBuffer[offset]));

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsBooleanParam(2) || !ioParms.GetBoolParam(2, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "2");

		else if (offset >= 0 && offset + 4 <= inBuffer->fLength)

			ioParms.ReturnNumber<sLONG>(*((sLONG *) &inBuffer->fBuffer[offset]));

		else if (noChecking) {

			sLONG	result;

			if (offset >= 0 && offset < inBuffer->fLength)

				result = inBuffer->fBuffer[offset];

			else

				result = 0;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset] << 8;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset] << 16;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset] << 24;

			ioParms.ReturnNumber<sLONG>(result);

		} else

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

void VJSBufferClass::_readInt32BE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	offset, result;

	if (ioParms.CountParams() == 1) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (offset < 0 || offset + 4 > inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else {

			result = inBuffer->fBuffer[offset] << 24;
			result |= inBuffer->fBuffer[offset + 1] << 16;
			result |= inBuffer->fBuffer[offset + 2] << 8;
			result |= inBuffer->fBuffer[offset + 3];

			ioParms.ReturnNumber<sLONG>(result);

		}

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsBooleanParam(2) || !ioParms.GetBoolParam(2, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "2");

		else if (offset >= 0 && offset + 4 <= inBuffer->fLength) {

			result = inBuffer->fBuffer[offset] << 24;
			result |= inBuffer->fBuffer[offset] << 16;
			result |= inBuffer->fBuffer[offset] << 8;
			result |= inBuffer->fBuffer[offset];

			ioParms.ReturnNumber<sLONG>(result);

		} else if (noChecking) {

			uLONG	result;

			if (offset >= 0 && offset < inBuffer->fLength)

				result = inBuffer->fBuffer[offset] << 24;

			else

				result = 0;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset] << 16;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset] << 8;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset];

			ioParms.ReturnNumber<sLONG>(result);

		} else

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

void VJSBufferClass::_writeUInt32LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	offset;
	Real	value;
	sLONG8	lowBits;

	if (ioParms.CountParams() == 2) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetRealParam(1, &value))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");

		else if (offset < 0 || offset + 4 > inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else {

			lowBits = value;
			*((sLONG *) &inBuffer->fBuffer[offset]) = lowBits;

		}

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetRealParam(1, &value))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");

		else if (!ioParms.IsBooleanParam(3) || !ioParms.GetBoolParam(3, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "3");

		else if (offset >= 0 && offset + 4 <= inBuffer->fLength) {

			lowBits = value;
			*((sLONG *) &inBuffer->fBuffer[offset]) = lowBits;

		} else if (noChecking) {

			lowBits = value;
			if (offset >= 0 && offset < inBuffer->fLength)

				inBuffer->fBuffer[offset] = lowBits;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				inBuffer->fBuffer[offset] = lowBits >> 8;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				inBuffer->fBuffer[offset] = lowBits >> 16;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				inBuffer->fBuffer[offset] = lowBits >> 24;

		} else

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

void VJSBufferClass::_writeUInt32BE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	offset;
	Real	value;
	sLONG8	lowBits;

	if (ioParms.CountParams() == 2) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetRealParam(1, &value))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");

		else if (offset < 0 || offset + 4 > inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else {

			lowBits = value;
			inBuffer->fBuffer[offset] = lowBits >> 24;
			inBuffer->fBuffer[offset + 1] = lowBits >> 16;
			inBuffer->fBuffer[offset + 2] = lowBits >> 8;
			inBuffer->fBuffer[offset + 3] = lowBits;

		}

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetRealParam(1, &value))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");

		else if (!ioParms.IsBooleanParam(3) || !ioParms.GetBoolParam(3, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "3");

		else if (offset >= 0 && offset + 3 <= inBuffer->fLength) {

			lowBits = value;
			inBuffer->fBuffer[offset] = lowBits >> 24;
			inBuffer->fBuffer[offset + 1] = lowBits >> 16;
			inBuffer->fBuffer[offset + 2] = lowBits >> 8;
			inBuffer->fBuffer[offset + 3] = lowBits;

		} else if (noChecking) {

			lowBits = value;
			if (offset >= 0 && offset < inBuffer->fLength)

				inBuffer->fBuffer[offset] = lowBits >> 24;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				inBuffer->fBuffer[offset] = lowBits >> 16;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				inBuffer->fBuffer[offset] = lowBits >> 8;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				inBuffer->fBuffer[offset] = lowBits;

		} else

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

void VJSBufferClass::_writeInt32LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	VJSBufferClass::_writeUInt32LE(ioParms, inBuffer);
}

void VJSBufferClass::_writeInt32BE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	VJSBufferClass::_writeUInt32BE(ioParms, inBuffer);
}

void VJSBufferClass::_readFloatLE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	offset;

	if (ioParms.CountParams() == 1) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (offset < 0 || offset + 4 > inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else

			ioParms.ReturnNumber<float>(*((float *) &inBuffer->fBuffer[offset]));

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsBooleanParam(2) || !ioParms.GetBoolParam(2, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "2");

		else if (offset >= 0 && offset + 4 <= inBuffer->fLength)

			ioParms.ReturnNumber<float>(*((float *) &inBuffer->fBuffer[offset]));

		else if (noChecking) {

			uLONG	result;

			if (offset >= 0 && offset < inBuffer->fLength)

				result = inBuffer->fBuffer[offset];

			else

				result = 0;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset] << 8;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset] << 16;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset] << 24;

			ioParms.ReturnNumber<float>(*((float *) &result));

		} else

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

void VJSBufferClass::_readFloatBE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	offset;
	uLONG	result;

	if (ioParms.CountParams() == 1) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (offset < 0 || offset + 4 > inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else {

			result = inBuffer->fBuffer[offset] << 24;
			result |= inBuffer->fBuffer[offset + 1] << 16;
			result |= inBuffer->fBuffer[offset + 2] << 8;
			result |= inBuffer->fBuffer[offset + 3];
			ioParms.ReturnNumber<float>(*((float *) &result));

		}

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsBooleanParam(2) || !ioParms.GetBoolParam(2, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "2");

		else if (offset >= 0 && offset + 4 <= inBuffer->fLength) {

			result = inBuffer->fBuffer[offset] << 24;
			result |= inBuffer->fBuffer[offset + 1] << 16;
			result |= inBuffer->fBuffer[offset + 2] << 8;
			result |= inBuffer->fBuffer[offset + 3];
			ioParms.ReturnNumber<float>(*((float *) &result));

		} else if (noChecking) {

			if (offset >= 0 && offset < inBuffer->fLength)

				result = inBuffer->fBuffer[offset] << 24;

			else

				result = 0;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset] << 16;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset] << 8;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				result |= inBuffer->fBuffer[offset];

			ioParms.ReturnNumber<float>(*((float *) &result));

		} else

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

void VJSBufferClass::_writeFloatLE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	offset;
	Real	value;

	if (ioParms.CountParams() == 2) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetRealParam(1, &value))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");

		else if (offset < 0 || offset + 4 > inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else

			*((float *) &inBuffer->fBuffer[offset]) = (float) value;

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetRealParam(1, &value))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");

		else if (!ioParms.IsBooleanParam(3) || !ioParms.GetBoolParam(3, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "3");

		else if (offset >= 0 && offset + 3 <= inBuffer->fLength)

			*((float *) &inBuffer->fBuffer[offset]) = (float) value;

		else if (noChecking) {

			uLONG	encodedFloat;

			*((float *) &encodedFloat) = (float) value;
			if (offset >= 0 && offset < inBuffer->fLength)

				inBuffer->fBuffer[offset] = encodedFloat;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				inBuffer->fBuffer[offset] = encodedFloat >> 8;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				inBuffer->fBuffer[offset] = encodedFloat >> 16;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				inBuffer->fBuffer[offset] = encodedFloat >> 24;

		} else

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

void VJSBufferClass::_writeFloatBE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	offset;
	uLONG	encodedFloat;
	Real	value;

	if (ioParms.CountParams() == 2) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetRealParam(1, &value))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");

		else if (offset < 0 || offset + 4 > inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else {

			*((float *) &encodedFloat) = (float) value;

			inBuffer->fBuffer[offset] = encodedFloat >> 24;
			inBuffer->fBuffer[offset + 1] = encodedFloat >> 16;
			inBuffer->fBuffer[offset + 2] = encodedFloat >> 8;
			inBuffer->fBuffer[offset + 3] = encodedFloat;

		}

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetRealParam(1, &value))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");

		else if (!ioParms.IsBooleanParam(3) || !ioParms.GetBoolParam(3, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "3");

		else if (offset >= 0 && offset + 3 <= inBuffer->fLength) {

			*((float *) &encodedFloat) = (float) value;

			inBuffer->fBuffer[offset] = encodedFloat >> 24;
			inBuffer->fBuffer[offset + 1] = encodedFloat >> 16;
			inBuffer->fBuffer[offset + 2] = encodedFloat >> 8;
			inBuffer->fBuffer[offset + 3] = encodedFloat;

		} else if (noChecking) {

			*((float *) &encodedFloat) = (float) value;

			if (offset >= 0 && offset < inBuffer->fLength)

				inBuffer->fBuffer[offset] = encodedFloat >> 24;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				inBuffer->fBuffer[offset] = encodedFloat >> 16;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				inBuffer->fBuffer[offset] = encodedFloat >> 8;

			offset++;
			if (offset >= 0 && offset < inBuffer->fLength)

				inBuffer->fBuffer[offset] = encodedFloat;

		} else

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

void VJSBufferClass::_readDoubleLE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	offset;

	if (ioParms.CountParams() == 1) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (offset < 0 || offset + 8 > inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else

			ioParms.ReturnNumber<double>(*((double *) &inBuffer->fBuffer[offset]));

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsBooleanParam(2) || !ioParms.GetBoolParam(2, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "2");

		else if (offset >= 0 && offset + 8 <= inBuffer->fLength)

			ioParms.ReturnNumber<double>(*((double *) &inBuffer->fBuffer[offset]));

		else if (noChecking) {

			double	result	= 0;
			uBYTE	*p		= (uBYTE *) &result;

			for (uLONG i = 0; i < 8; i++, p++, offset++)

				if (offset >= 0 && offset < inBuffer->fLength)

					*p = inBuffer->fBuffer[offset];

			ioParms.ReturnNumber<double>(result);

		} else

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

void VJSBufferClass::_readDoubleBE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	offset;

	double	result;
	uBYTE	*p		= (uBYTE *) &result;

	if (ioParms.CountParams() == 1) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (offset < 0 || offset + 8 > inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else {

			offset += 7;
			for (uLONG i = 0; i < 8; i++, p++, offset--)

				*p = inBuffer->fBuffer[offset];

			ioParms.ReturnNumber<double>(result);

		}

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsBooleanParam(2) || !ioParms.GetBoolParam(2, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "2");

		else if (offset >= 0 && offset + 8 <= inBuffer->fLength) {

			offset += 7;
			for (uLONG i = 0; i < 8; i++, p++, offset--)

				*p = inBuffer->fBuffer[offset];

			ioParms.ReturnNumber<double>(result);

		} else if (noChecking) {

			offset += 7;
			for (uLONG i = 0; i < 8; i++, p++, offset--)

				if (offset >= 0 && offset < inBuffer->fLength)

					*p = inBuffer->fBuffer[offset];

			ioParms.ReturnNumber<double>(result);

		} else

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

void VJSBufferClass::_writeDoubleLE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	offset;
	Real	value;

	if (ioParms.CountParams() == 2) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetRealParam(1, &value))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");

		else if (offset < 0 || offset + 8 > inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else

			*((double *) &inBuffer->fBuffer[offset]) = (double) value;

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetRealParam(1, &value))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");

		else if (!ioParms.IsBooleanParam(3) || !ioParms.GetBoolParam(3, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "3");

		else if (offset >= 0 && offset + 8 <= inBuffer->fLength)

			*((double *) &inBuffer->fBuffer[offset]) = (double) value;

		else if (noChecking) {

			uBYTE	*p = (uBYTE *) &value;

			for (uLONG i = 0; i < 8; i++, p++, offset++)

				if (offset >= 0 && offset < inBuffer->fLength)

					inBuffer->fBuffer[offset] = *p;

		} else

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

void VJSBufferClass::_writeDoubleBE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer)
{
	xbox_assert(inBuffer != NULL);

	sLONG	offset;
	Real	value;
	uBYTE	*p;

	p = ((uBYTE *) &value) + 7;
	if (ioParms.CountParams() == 2) {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetRealParam(1, &value))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");

		else if (offset < 0 || offset + 8 > inBuffer->fLength)

			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

		else

			for (uLONG i = 0; i < 8; i++, p--, offset++)

				inBuffer->fBuffer[offset] = *p;

	} else {

		bool	noChecking;

		if (!ioParms.IsNumberParam(1) || !ioParms.GetRealParam(1, &value))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

		else if (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &offset))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");

		else if (!ioParms.IsBooleanParam(3) || !ioParms.GetBoolParam(3, &noChecking))

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "3");

		else if (offset >= 0 && offset + 8 <= inBuffer->fLength)

			for (uLONG i = 0; i < 8; i++, p--, offset++)

				inBuffer->fBuffer[offset] = *p;

		else if (noChecking)
		{
			for (uLONG i = 0; i < 8; i++, p--, offset++)

				if (offset >= 0 && offset < inBuffer->fLength)
					inBuffer->fBuffer[offset] = *p;
		}
		else
			XBOX::vThrowError(XBOX::VE_JVSC_BUFFER_OUT_OF_BOUND);

	}
}

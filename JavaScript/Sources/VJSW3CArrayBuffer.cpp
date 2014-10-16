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

#include "VJSW3CArrayBuffer.h"

#include "VJSContext.h"
#include "VJSGlobalClass.h"

USING_TOOLBOX_NAMESPACE

VJSArrayBufferObject::VJSArrayBufferObject (VSize inLength, void *inBuffer)
{
	xbox_assert(inLength >= 0);
	xbox_assert(!(inLength > 0 && inBuffer == NULL));

	if ((fBufferObject = new VJSBufferObject(inLength, inBuffer)) == NULL)

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
}

VJSArrayBufferObject::VJSArrayBufferObject (VJSBufferObject *inBufferObject)
{
	xbox_assert(inBufferObject != NULL);

	inBufferObject->Retain();
	fBufferObject = inBufferObject;
}

VJSArrayBufferObject::VJSArrayBufferObject (VSize inLength)
{
	xbox_assert(inLength >= 0);

	if ((fBufferObject = new VJSBufferObject(inLength)) == NULL) 
	
		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

	else {

		// Initialize to zero as required by specification.

		::memset(fBufferObject->GetDataPtr(), 0, inLength);

	}
}

VJSArrayBufferObject::VJSArrayBufferObject ()
{
	fBufferObject = NULL;
}

VJSArrayBufferObject::~VJSArrayBufferObject ()
{
	if (fBufferObject != NULL)

		XBOX::ReleaseRefCountable<VJSBufferObject>(&fBufferObject);	
}

void VJSArrayBufferClass::GetDefinition (ClassDefinition &outDefinition)
{
	static XBOX::VJSClass<VJSArrayBufferClass, VJSArrayBufferObject>::StaticFunction functions[] =
	{
		{	"slice",	js_callStaticFunction<_slice>,		JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"toBuffer",	js_callStaticFunction<_toBuffer>,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	0,			0,									0																		},
	};

    outDefinition.className			= "ArrayBuffer";
	outDefinition.staticFunctions	= functions;	
	outDefinition.initialize		= js_initialize<Initialize>;
	outDefinition.finalize			= js_finalize<_Finalize>;
	outDefinition.getProperty		= js_getProperty<_GetProperty>;
}

XBOX::VJSObject	VJSArrayBufferClass::MakeConstructor (XBOX::VJSContext inContext)
{
	XBOX::VJSObject	arrayBufferConstructor =
		JS4DMakeConstructor(inContext, Class(), _Construct, false, NULL);

	return arrayBufferConstructor;
}

XBOX::VJSObject	VJSArrayBufferClass::NewInstance (XBOX::VJSContext inContext, VSize inLength, void *inBuffer)
{
	XBOX::VJSObject			createdObject(inContext);
	VJSArrayBufferObject	*arrayBuffer;

	if ((arrayBuffer = new VJSArrayBufferObject(inLength, inBuffer)) == NULL) {

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
				
	} else {

		// Check if fBufferObject allocation is successful.			

		if (!arrayBuffer->IsNeutered()) 

			createdObject = VJSArrayBufferClass::CreateInstance(inContext, arrayBuffer);
		
		arrayBuffer->Release();

	}

	return createdObject;
}

void VJSArrayBufferClass::_Construct(XBOX::VJSParms_construct &ioParms)
{
	sLONG	size;

	if (!ioParms.GetLongParam(1, &size)) {
					
		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
		return;	

	}
	if (size < 0) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_NUMBER_ARGUMENT, "1");
		return;

	}

	VJSArrayBufferObject	*arrayBuffer;

	if ((arrayBuffer = new VJSArrayBufferObject(size)) == NULL) {

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
		
	} else if (arrayBuffer->fBufferObject == NULL) {

		// Allocation of fBufferObject failed.

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
		arrayBuffer->Release();
		
	} else {
	
		ioParms.ReturnConstructedObject<VJSArrayBufferClass>(arrayBuffer);
		arrayBuffer->Release();

	}
}

void VJSArrayBufferClass::Initialize (const VJSParms_initialize &inParms, VJSArrayBufferObject *inArrayBuffer)
{
	xbox_assert(inArrayBuffer != NULL);

	inArrayBuffer->Retain();
}

void VJSArrayBufferClass::_Finalize (const XBOX::VJSParms_finalize &inParms, VJSArrayBufferObject *inArrayBuffer)
{
	xbox_assert(inArrayBuffer != NULL);

	inArrayBuffer->Release();
}

void VJSArrayBufferClass::_GetProperty (XBOX::VJSParms_getProperty &ioParms, VJSArrayBufferObject *inArrayBuffer)
{
	xbox_assert(inArrayBuffer != NULL);

	XBOX::VString	name;

	if (ioParms.GetPropertyName(name) && name.EqualToUSASCIICString("byteLength"))

		ioParms.ReturnNumber<VSize>(inArrayBuffer->GetDataSize());

	else 
		
		ioParms.ForwardToParent();
}

void VJSArrayBufferClass::_slice (XBOX::VJSParms_callStaticFunction &ioParms, VJSArrayBufferObject *inArrayBuffer)
{
	xbox_assert(inArrayBuffer != NULL);

	if (inArrayBuffer->IsNeutered()) {

		ioParms.ReturnNullValue();
		return;

	}

	sLONG	dataSize, begin, end, size;

	dataSize = (sLONG) inArrayBuffer->fBufferObject->GetDataSize(); 
	if (!ioParms.CountParams()) {

		begin = 0;
		end = dataSize;

	} else {

		// Negative indexes are ok, they are from end of ArrayBuffer.

		if (!ioParms.GetLongParam(1, &begin)) {
					
			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
			return;	

		}

		if (begin < 0 && (begin = dataSize - begin) < 0)

			begin = 0;

		if (ioParms.CountParams() >= 2) {

			if (!ioParms.GetLongParam(2, &end)) {
					
				XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");
				return;	

			}

			if (end > dataSize)

				end = dataSize;

			else if (end < 0 && (end = dataSize - end) < 0)

				end = 0;

		} else 

			end = dataSize;

	}

	if (begin > end)

		size = 0;

	else

		size = end - begin;

	VJSArrayBufferObject	*arrayBuffer;

	if ((arrayBuffer = size ? new VJSArrayBufferObject(size) : new VJSArrayBufferObject()) == NULL) {

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
		ioParms.ReturnUndefinedValue();
				
	} else {
				
		if (!size) 
	
			ioParms.ReturnValue(VJSArrayBufferClass::CreateInstance(ioParms.GetContext(), arrayBuffer));

		else if (arrayBuffer->fBufferObject != NULL) {

			uBYTE	*p;

			p = (uBYTE *) inArrayBuffer->fBufferObject->GetDataPtr();
			::memcpy(arrayBuffer->fBufferObject->GetDataPtr(), &p[begin], size);
			ioParms.ReturnValue(VJSArrayBufferClass::CreateInstance(ioParms.GetContext(), arrayBuffer));

		} else

			ioParms.ReturnUndefinedValue();		// fBufferObject allocation failed!
			
		arrayBuffer->Release();	
		
	}
}

void VJSArrayBufferClass::_toBuffer (XBOX::VJSParms_callStaticFunction &ioParms, VJSArrayBufferObject *inArrayBuffer)
{
	xbox_assert(inArrayBuffer != NULL);
	
	if (inArrayBuffer->IsNeutered()) {

		// If ArrayBuffer has been "neutered", return null.

		ioParms.ReturnNullValue();

	} else {

		// Created Buffer object will do retain() on inArrayBuffer->fBufferObject.

		ioParms.ReturnValue(VJSBufferClass::CreateInstance(ioParms.GetContext(), inArrayBuffer->fBufferObject));

	}
}

void VJSTypedArrayObject::AddConstructors (XBOX::VJSContext inContext, XBOX::VJSObject inObject)
{
	inObject.SetProperty(
		"Int8Array",
		JS4DMakeConstructor(inContext, VJSInt8ArrayClass::Class(), VJSTypedArrayObject::Construct<VJSInt8ArrayClass, sBYTE>, false, NULL),
		XBOX::JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly);

	inObject.SetProperty(
		"UInt8Array",
		JS4DMakeConstructor(inContext, VJSUInt8ArrayClass::Class(), VJSTypedArrayObject::Construct<VJSUInt8ArrayClass, uBYTE>, false, NULL),
		XBOX::JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly);

	inObject.SetProperty(
		"Int16Array",
		JS4DMakeConstructor(inContext, VJSInt16ArrayClass::Class(), VJSTypedArrayObject::Construct<VJSInt16ArrayClass, sWORD>, false, NULL),
		XBOX::JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly);

	inObject.SetProperty(
		"UInt16Array",
		JS4DMakeConstructor(inContext, VJSUInt16ArrayClass::Class(), VJSTypedArrayObject::Construct<VJSUInt16ArrayClass, uWORD>, false, NULL),
		XBOX::JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly);

	inObject.SetProperty(
		"Int32Array",
		JS4DMakeConstructor(inContext, VJSInt32ArrayClass::Class(), VJSTypedArrayObject::Construct<VJSInt32ArrayClass, sLONG>, false, NULL),
		XBOX::JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly);

	inObject.SetProperty(
		"UInt32Array",
		JS4DMakeConstructor(inContext, VJSUInt32ArrayClass::Class(), VJSTypedArrayObject::Construct<VJSUInt32ArrayClass, uLONG>, false, NULL),
		XBOX::JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly);

	inObject.SetProperty(
		"Float32Array",
		JS4DMakeConstructor(inContext, VJSFloat32ArrayClass::Class(), VJSTypedArrayObject::Construct<VJSFloat32ArrayClass, SmallReal>, false, NULL),
		XBOX::JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly);

	inObject.SetProperty(
		"Float64Array",
		JS4DMakeConstructor(inContext, VJSFloat64ArrayClass::Class(), VJSTypedArrayObject::Construct<VJSFloat64ArrayClass, Real>, false, NULL),
		XBOX::JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly);

}

template <class CLASS, class TYPE> void VJSTypedArrayObject::Construct(XBOX::VJSParms_construct &ioParms)
{
}

template <class TYPE> void VJSTypedArrayObject::Initialize (const VJSParms_initialize &inParms, VJSTypedArrayObject *inTypedArray)
{
	xbox_assert(inTypedArray != NULL);

	inParms.GetObject().SetProperty("byteOffset", inTypedArray->fByteOffset, XBOX::JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly);
	inParms.GetObject().SetProperty("byteLength", inTypedArray->fByteLength, XBOX::JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly);
	inParms.GetObject().SetProperty("length", (sLONG) (inTypedArray->fByteLength / sizeof(TYPE)), XBOX::JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly);
	inParms.GetObject().SetProperty("BYTES_PER_ELEMENT", (sLONG) sizeof(TYPE), XBOX::JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly);
}

void VJSTypedArrayObject::Finalize (const XBOX::VJSParms_finalize &inParms, VJSTypedArrayObject *inTypedArray)
{
	xbox_assert(inTypedArray != NULL);

	delete inTypedArray;
}

template <class TYPE> bool VJSTypedArrayObject::SetProperty (XBOX::VJSParms_setProperty &ioParms, VJSTypedArrayObject *inTypedArray)
{
	xbox_assert(inTypedArray != NULL);

	if (inTypedArray->fArrayBufferObject->IsNeutered()) {

		XBOX::vThrowError(XBOX::VE_JVSC_ARRAY_BUFFER_IS_NEUTERED);
		return true;

	}

	sLONG			index;
	XBOX::VJSValue	value(ioParms.GetContext());

	if (ioParms.GetPropertyNameAsLong(&index)) {

		_SetValue<TYPE>(inTypedArray, index, ioParms.GetPropertyValue(), false);
		return true;

	} else

		return false;	// "Forward" to parent.
}

template <class TYPE> void VJSTypedArrayObject::GetProperty (XBOX::VJSParms_getProperty &ioParms, VJSTypedArrayObject *inTypedArray)
{
	xbox_assert(inTypedArray != NULL);

	sLONG			index;
	XBOX::VString	name;
	
	if (ioParms.GetPropertyNameAsLong(&index)) {
		
		if (index < 0 || (index *= sizeof(TYPE)) >= inTypedArray->fByteLength || index + sizeof(TYPE) >= inTypedArray->fByteLength) {

			// If index is out of bound, return 'undefined'.

			ioParms.ReturnUndefinedValue();
		
		} else if (inTypedArray->fArrayBufferObject->IsNeutered()) {

			// If index is valid but ArrayBuffer has been "neutered", throw an exception. 
			// This is a programming error.

			XBOX::vThrowError(XBOX::VE_JVSC_ARRAY_BUFFER_IS_NEUTERED);
			ioParms.ReturnUndefinedValue();

		} else {

			TYPE	*p;

			index += inTypedArray->fByteOffset;
			p = (TYPE *) ((uBYTE *) inTypedArray->fArrayBufferObject->GetDataPtr() + index);
			ioParms.ReturnNumber<TYPE>(*p);

		}

	} else if (ioParms.GetPropertyName(name)) {

		if (name.EqualToUSASCIICString("byteOffset"))

			ioParms.ReturnNumber<sLONG>(inTypedArray->fArrayBufferObject->IsNeutered() ? 0 : inTypedArray->fByteOffset);

		else if (name.EqualToUSASCIICString("byteLength"))

			ioParms.ReturnNumber<sLONG>(inTypedArray->fArrayBufferObject->IsNeutered() ? 0 : inTypedArray->fByteLength);

		else if (name.EqualToUSASCIICString("length"))

			ioParms.ReturnNumber<sLONG>(inTypedArray->fArrayBufferObject->IsNeutered() ? 0 : inTypedArray->fByteLength / sizeof(TYPE));
		
		else

			ioParms.ForwardToParent();

	} else 

		ioParms.ForwardToParent();
}

template <class TYPE> void VJSTypedArrayObject::Get (XBOX::VJSParms_callStaticFunction &ioParms, VJSTypedArrayObject *inTypedArray)
{
	xbox_assert(inTypedArray != NULL);

	if (inTypedArray->fArrayBufferObject->IsNeutered()) {

		XBOX::vThrowError(XBOX::VE_JVSC_ARRAY_BUFFER_IS_NEUTERED);
		return;

	}

	sLONG	index;

	if (!ioParms.GetLongParam(1, &index)) {
					
		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
		return;	

	}

	if (index < 0 || (index *= sizeof(TYPE)) >= inTypedArray->fByteLength || index + sizeof(TYPE) >= inTypedArray->fByteLength) {
		
		XBOX::vThrowError(XBOX::VE_JVSC_TYPED_ARRAY_OUT_OF_BOUND);
		return;

	}

	TYPE	*p;

	index += inTypedArray->fByteOffset;
	p = (TYPE *) ((uBYTE *) inTypedArray->fArrayBufferObject->GetDataPtr() + index);
	ioParms.ReturnNumber<TYPE>(*p);
}

template <class TYPE> void VJSTypedArrayObject::Set (XBOX::VJSParms_callStaticFunction &ioParms, VJSTypedArrayObject *inTypedArray)
{
	xbox_assert(inTypedArray != NULL);

	if (inTypedArray->fArrayBufferObject->IsNeutered()) {

		XBOX::vThrowError(XBOX::VE_JVSC_ARRAY_BUFFER_IS_NEUTERED);
		return;

	}

	sLONG			index;
	XBOX::VJSArray	arrayObject(ioParms.GetContext());
	XBOX::VJSObject	object(ioParms.GetContext());

	if (ioParms.GetLongParam(1, &index)) {

		// set(index, value);

		if (ioParms.CountParams() < 2) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER, "2");

		} else 

			VJSTypedArrayObject::_SetValue<TYPE>(inTypedArray, index, ioParms.GetParamValue(2), true);

	} else if (ioParms.GetParamArray(1, arrayObject)) {

		// set(Array, optionalIndex);

		index = 0;
		if (ioParms.CountParams() >= 2 && ioParms.GetLongParam(2, &index)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");
			return;

		}

		if (index < 0 || (index *= sizeof(TYPE)) >= inTypedArray->fByteLength) {

			XBOX::vThrowError(XBOX::VE_JVSC_TYPED_ARRAY_OUT_OF_BOUND);
			return;

		}

		sLONG	length;

		length = (inTypedArray->fByteLength - index) / sizeof(TYPE);
		if (length < arrayObject.GetLength()) {

			XBOX::vThrowError(XBOX::VE_JVSC_TYPED_ARRAY_TOO_SMALL);
			return;

		}

		TYPE	*p;

		index += inTypedArray->fByteOffset;
		p = (TYPE *) ((uBYTE *) inTypedArray->fArrayBufferObject->GetDataPtr() + index);
		_CopyFromArray(p, arrayObject);

	} else if (ioParms.GetParamObject(1, object)) {

		// set(TypedArray, optionalIndex);

		index = 0;
		if (ioParms.CountParams() >= 2 && ioParms.GetLongParam(2, &index)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");
			return;

		}

		if (index < 0 || (index *= sizeof(TYPE)) >= inTypedArray->fByteLength) {

			XBOX::vThrowError(XBOX::VE_JVSC_TYPED_ARRAY_OUT_OF_BOUND);
			return;

		}
	
		_CopyFromTypedArray<TYPE>(inTypedArray, index, object);

	} else

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER, "1");
}

template <class CLASS, class TYPE> void	VJSTypedArrayObject::SubArray (XBOX::VJSParms_callStaticFunction &ioParms, VJSTypedArrayObject *inTypedArray)
{
	xbox_assert(inTypedArray != NULL);

	if (inTypedArray->fArrayBufferObject->IsNeutered()) {

		XBOX::vThrowError(XBOX::VE_JVSC_ARRAY_BUFFER_IS_NEUTERED);
		return;

	}

	sLONG	length, begin, end;

	length = inTypedArray->fByteLength / sizeof(TYPE);
	if (!ioParms.GetLongParam(1, &begin)) {
					
		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
		return;	

	}
	if (begin < 0 && (begin += length) < 0)

		begin = 0;

	if (ioParms.CountParams() >= 2) {
		
		if (!ioParms.GetLongParam(2, &end)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");
			return;	

		}
		if (end < 0 && (end += length) < 0)

			end = 0;

	} else

		end = length;

	sLONG	size;

	if (begin > end || begin == length) {

		XBOX::vThrowError(XBOX::VE_JVSC_TYPED_ARRAY_OUT_OF_BOUND);
		return;

	} else

		size = end - begin;

	VJSTypedArrayObject	*typedArray;

	begin = inTypedArray->fByteOffset + begin * sizeof(TYPE);
	size *= sizeof(TYPE); 
	if ((typedArray = new VJSTypedArrayObject(begin, size, inTypedArray->fArrayBufferObject)) == NULL) 

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

	else {

		XBOX::VJSObject	createdObject	= CLASS::CreateInstance(ioParms.GetContext(), typedArray);
		XBOX::VJSObject	arrayBuffer		= ioParms.GetThis().GetProperty("buffer").GetObject();

		xbox_assert(arrayBuffer.IsOfClass(VJSArrayBufferClass::Class()));
		createdObject.SetProperty("buffer", arrayBuffer, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete);
		
		ioParms.ReturnValue(createdObject);		

	}
}

VJSTypedArrayObject::VJSTypedArrayObject (sLONG inByteOffset, sLONG inByteLength, VJSArrayBufferObject *inArrayBufferObject)
{
	xbox_assert(inByteOffset <= inByteLength);
	xbox_assert(inArrayBufferObject != NULL);
	
	fByteOffset = inByteOffset; 
	fByteLength = inByteLength;
	fArrayBufferObject = XBOX::RetainRefCountable<VJSArrayBufferObject>(inArrayBufferObject);
}

VJSTypedArrayObject::~VJSTypedArrayObject ()
{
	xbox_assert(fArrayBufferObject != NULL);

	XBOX::ReleaseRefCountable<VJSArrayBufferObject>(&fArrayBufferObject);
}

template <class TYPE> void VJSTypedArrayObject::_SetValue (VJSTypedArrayObject *inTypedArray, sLONG index, const XBOX::VJSValue &inValue, bool inThrowException)
{
	xbox_assert(inTypedArray != NULL);
	xbox_assert(!inTypedArray->fArrayBufferObject->IsNeutered());

	if (index < 0 || (index *= sizeof(TYPE)) >= inTypedArray->fByteLength || index + sizeof(TYPE) >= inTypedArray->fByteLength) {

		// For an indexed access (typedArray[index]), there is no exception for out of bound access.

		if (inThrowException)

			XBOX::vThrowError(XBOX::VE_JVSC_TYPED_ARRAY_OUT_OF_BOUND);
				
	} else {

		Real	value;

		// Value can be a number or a string. For a string, first character is taken.

		if (inValue.IsNumber()) {

			if (!inValue.GetReal(&value))

				value = 0;

		} else if (inValue.IsString()) {

			XBOX::VString	string;

			if (!inValue.GetString(string) || !string.GetLength())

				value = 0;

			else

				value = string.GetUniChar(1);
				
		} else

			value = 0;

		TYPE	*p;

		index += inTypedArray->fByteOffset;
		p = (TYPE *) ((uBYTE *) inTypedArray->fArrayBufferObject->GetDataPtr() + index);
		*p = (TYPE) value;

	}
}

template <class TYPE> void VJSTypedArrayObject::_CopyFromArray (TYPE *outDestination, const XBOX::VJSArray &inArray)
{
	xbox_assert(outDestination != NULL);

	// All bound checking must have been done before calling this method!

	VSize	i, length;
	
	for (i = 0, length = inArray.GetLength(); i < length; i++) {

		XBOX::VJSValue	value	= inArray.GetValueAt(i);
		Real			v;

		// Value can be a number or a string. For a string, first character is taken.

		if (value.IsNumber()) {

			if (!value.GetReal(&v))

				v = 0;

		} else if (value.IsString()) {

			XBOX::VString	string;

			if (!value.GetString(string) || !string.GetLength())

				v = 0;

			else

				v = string.GetUniChar(1);
				
		} else

			v = 0;

		*outDestination++ = (TYPE) v;

	}
}

template <class TYPE> void VJSTypedArrayObject::_CopyFromTypedArray (VJSTypedArrayObject *inTypedArray, sLONG index, const XBOX::VJSObject &inObject)
{
	xbox_assert(inTypedArray != NULL);

	VJSTypedArrayObject	*typedArrayObject;
	sLONG				type, length;

	// This code is ugly and potentially slow. 
	// Is there a better way to do that? 
	// It would be great if JSC had a function GetClassRef(JSObjectRef *inObjRef).
	// Still can replace long "if then else", by a "for" of a table.

	if (inObject.IsOfClass(VJSInt8ArrayClass::Class())) {

		typedArrayObject = inObject.GetPrivateData<VJSInt8ArrayClass>();
		xbox_assert(typedArrayObject != NULL);
		type = TYPE_INT8;
		length = typedArrayObject->fByteLength / sizeof(sBYTE);

	} else if (inObject.IsOfClass(VJSUInt8ArrayClass::Class())) {

		typedArrayObject = inObject.GetPrivateData<VJSUInt8ArrayClass>();
		xbox_assert(typedArrayObject != NULL);
		type = TYPE_UINT8;
		length = typedArrayObject->fByteLength / sizeof(uBYTE);

	} else if (inObject.IsOfClass(VJSInt16ArrayClass::Class())) {

		typedArrayObject = inObject.GetPrivateData<VJSInt16ArrayClass>();
		xbox_assert(typedArrayObject != NULL);
		type = TYPE_INT16;
		length = typedArrayObject->fByteLength / sizeof(sWORD);

	} else if (inObject.IsOfClass(VJSUInt16ArrayClass::Class())) {

		typedArrayObject = inObject.GetPrivateData<VJSUInt16ArrayClass>();
		xbox_assert(typedArrayObject != NULL);
		type = TYPE_UINT16;
		length = typedArrayObject->fByteLength / sizeof(uWORD);

	} else if (inObject.IsOfClass(VJSInt32ArrayClass::Class())) {

		typedArrayObject = inObject.GetPrivateData<VJSInt32ArrayClass>();
		xbox_assert(typedArrayObject != NULL);
		type = TYPE_INT32;
		length = typedArrayObject->fByteLength / sizeof(sLONG);

	} else if (inObject.IsOfClass(VJSUInt32ArrayClass::Class())) {

		typedArrayObject = inObject.GetPrivateData<VJSUInt32ArrayClass>();
		xbox_assert(typedArrayObject != NULL);
		type = TYPE_UINT32;
		length = typedArrayObject->fByteLength / sizeof(uLONG);

	} else if (inObject.IsOfClass(VJSFloat32ArrayClass::Class())) {

		typedArrayObject = inObject.GetPrivateData<VJSFloat32ArrayClass>();
		xbox_assert(typedArrayObject != NULL);
		type = TYPE_FLOAT32;
		length = typedArrayObject->fByteLength / sizeof(SmallReal);

	} else if (inObject.IsOfClass(VJSFloat64ArrayClass::Class())) {

		typedArrayObject = inObject.GetPrivateData<VJSFloat64ArrayClass>();
		xbox_assert(typedArrayObject != NULL);
		type = TYPE_FLOAT64;
		length = typedArrayObject->fByteLength / sizeof(Real);

	} else {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER, "1");
		return; 

	}

	if (typedArrayObject->fArrayBufferObject->IsNeutered())

		XBOX::vThrowError(XBOX::VE_JVSC_ARRAY_BUFFER_IS_NEUTERED);

	else if ((inTypedArray->fByteLength - index) / sizeof(TYPE) < length)

		XBOX::vThrowError(XBOX::VE_JVSC_TYPED_ARRAY_TOO_SMALL);

	else {
	
		index += inTypedArray->fByteOffset;
		switch (type) {

			case TYPE_INT8:	
				
				_TypedCopy<TYPE, sBYTE>(inTypedArray->fArrayBufferObject->GetDataPtr(), index,
					typedArrayObject->fArrayBufferObject->GetDataPtr(), typedArrayObject->fByteOffset,
					length);
				break;

			case TYPE_UINT8:	
				
				_TypedCopy<TYPE, uBYTE>(inTypedArray->fArrayBufferObject->GetDataPtr(), index,
					typedArrayObject->fArrayBufferObject->GetDataPtr(), typedArrayObject->fByteOffset,
					length);
				break;

			case TYPE_INT16:
				
				_TypedCopy<TYPE, sWORD>(inTypedArray->fArrayBufferObject->GetDataPtr(), index,
					typedArrayObject->fArrayBufferObject->GetDataPtr(), typedArrayObject->fByteOffset,
					length);
				break;

			case TYPE_UINT16:	
				
				_TypedCopy<TYPE, uWORD>(inTypedArray->fArrayBufferObject->GetDataPtr(), index,
					typedArrayObject->fArrayBufferObject->GetDataPtr(), typedArrayObject->fByteOffset,
					length);
				break;

			case TYPE_INT32:
				
				_TypedCopy<TYPE, sLONG>(inTypedArray->fArrayBufferObject->GetDataPtr(), index,
					typedArrayObject->fArrayBufferObject->GetDataPtr(), typedArrayObject->fByteOffset,
					length);
				break;

			case TYPE_UINT32:	
				
				_TypedCopy<TYPE, uLONG>(inTypedArray->fArrayBufferObject->GetDataPtr(), index,
					typedArrayObject->fArrayBufferObject->GetDataPtr(), typedArrayObject->fByteOffset,
					length);
				break;

			case TYPE_FLOAT32:
				
				_TypedCopy<TYPE, SmallReal>(inTypedArray->fArrayBufferObject->GetDataPtr(), index,
					typedArrayObject->fArrayBufferObject->GetDataPtr(), typedArrayObject->fByteOffset,
					length);
				break;

			case TYPE_FLOAT64:	
				
				_TypedCopy<TYPE, Real>(inTypedArray->fArrayBufferObject->GetDataPtr(), index,
					typedArrayObject->fArrayBufferObject->GetDataPtr(), typedArrayObject->fByteOffset,
					length);
				break;

			default:	
				
				xbox_assert(false);
				break;

		}

	}

}

template <class A, class B> void VJSTypedArrayObject::_TypedCopy (void *outDestination, sLONG inDestinationOffset, const void *inSource, sLONG inSourceOffset, sLONG inNumberElements)
{
	A	*p;
	B	*q;

	p = (A *) ((uBYTE *) outDestination + inDestinationOffset);
	q = (B *) ((uBYTE *) inSource + inSourceOffset);
	while (inNumberElements--)

		*p++ = (A) *q++;
}

void VJSInt8ArrayClass::GetDefinition (ClassDefinition &outDefinition)
{
	static XBOX::VJSClass<VJSInt8ArrayClass, VJSTypedArrayObject>::StaticFunction functions[] =
	{		
		{	"get",		js_callStaticFunction<VJSTypedArrayObject::Get<sBYTE> >,							JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"set",		js_callStaticFunction<VJSTypedArrayObject::Set<sBYTE> >,							JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"subArray",	js_callStaticFunction<VJSTypedArrayObject::SubArray<VJSInt8ArrayClass, sBYTE> >,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	0,			0,																					0																	},					
	};

    outDefinition.className			= "Int8Array";
	outDefinition.staticFunctions	= functions;	
	outDefinition.initialize		= js_initialize<VJSTypedArrayObject::Initialize<sBYTE> >;
	outDefinition.finalize			= js_finalize<VJSTypedArrayObject::Finalize>;
	outDefinition.setProperty		= js_setProperty<VJSTypedArrayObject::SetProperty<sBYTE> >;
	outDefinition.getProperty		= js_getProperty<VJSTypedArrayObject::GetProperty<sBYTE> >;	
}

void VJSUInt8ArrayClass::GetDefinition (ClassDefinition &outDefinition)
{
	static XBOX::VJSClass<VJSUInt8ArrayClass, VJSTypedArrayObject>::StaticFunction functions[] =
	{		
		{	"get",		js_callStaticFunction<VJSTypedArrayObject::Get<uBYTE> >,							JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"set",		js_callStaticFunction<VJSTypedArrayObject::Set<uBYTE> >,							JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"subArray",	js_callStaticFunction<VJSTypedArrayObject::SubArray<VJSUInt8ArrayClass, uBYTE> >,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	0,			0,																					0																	},					
	};

    outDefinition.className			= "UInt8Array";
	outDefinition.staticFunctions	= functions;	
	outDefinition.initialize		= js_initialize<VJSTypedArrayObject::Initialize<uBYTE> >;
	outDefinition.finalize			= js_finalize<VJSTypedArrayObject::Finalize>;
	outDefinition.setProperty		= js_setProperty<VJSTypedArrayObject::SetProperty<uBYTE> >;
	outDefinition.getProperty		= js_getProperty<VJSTypedArrayObject::GetProperty<uBYTE> >;	
}

void VJSInt16ArrayClass::GetDefinition (ClassDefinition &outDefinition)
{
	static XBOX::VJSClass<VJSInt16ArrayClass, VJSTypedArrayObject>::StaticFunction functions[] =
	{		
		{	"get",		js_callStaticFunction<VJSTypedArrayObject::Get<sWORD> >,							JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"set",		js_callStaticFunction<VJSTypedArrayObject::Set<sWORD> >,							JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"subArray",	js_callStaticFunction<VJSTypedArrayObject::SubArray<VJSInt16ArrayClass, sWORD> >,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	0,			0,																					0																	},
	};

    outDefinition.className			= "Int16Array";
	outDefinition.staticFunctions	= functions;	
	outDefinition.initialize		= js_initialize<VJSTypedArrayObject::Initialize<sWORD> >;
	outDefinition.finalize			= js_finalize<VJSTypedArrayObject::Finalize>;
	outDefinition.setProperty		= js_setProperty<VJSTypedArrayObject::SetProperty<sWORD> >;
	outDefinition.getProperty		= js_getProperty<VJSTypedArrayObject::GetProperty<sWORD> >;	
}

void VJSUInt16ArrayClass::GetDefinition (ClassDefinition &outDefinition)
{
	static XBOX::VJSClass<VJSUInt16ArrayClass, VJSTypedArrayObject>::StaticFunction functions[] =
	{		
		{	"get",		js_callStaticFunction<VJSTypedArrayObject::Get<uWORD> >,							JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"set",		js_callStaticFunction<VJSTypedArrayObject::Set<uWORD> >,							JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"subArray",	js_callStaticFunction<VJSTypedArrayObject::SubArray<VJSUInt16ArrayClass, uWORD> >,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	0,			0,																					0																	},
	};

    outDefinition.className			= "UInt16Array";
	outDefinition.staticFunctions	= functions;	
	outDefinition.initialize		= js_initialize<VJSTypedArrayObject::Initialize<uWORD> >;
	outDefinition.finalize			= js_finalize<VJSTypedArrayObject::Finalize>;
	outDefinition.setProperty		= js_setProperty<VJSTypedArrayObject::SetProperty<uWORD> >;
	outDefinition.getProperty		= js_getProperty<VJSTypedArrayObject::GetProperty<uWORD> >;	
}

void VJSInt32ArrayClass::GetDefinition (ClassDefinition &outDefinition)
{
	static XBOX::VJSClass<VJSInt32ArrayClass, VJSTypedArrayObject>::StaticFunction functions[] =
	{		
		{	"get",		js_callStaticFunction<VJSTypedArrayObject::Get<sLONG> >,							JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"set",		js_callStaticFunction<VJSTypedArrayObject::Set<sLONG> >,							JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"subArray",	js_callStaticFunction<VJSTypedArrayObject::SubArray<VJSInt32ArrayClass, sLONG> >,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	0,			0,																					0																	},
	};

    outDefinition.className			= "Int32Array";
	outDefinition.staticFunctions	= functions;	
	outDefinition.initialize		= js_initialize<VJSTypedArrayObject::Initialize<sLONG> >;
	outDefinition.finalize			= js_finalize<VJSTypedArrayObject::Finalize>;
	outDefinition.setProperty		= js_setProperty<VJSTypedArrayObject::SetProperty<sLONG> >;
	outDefinition.getProperty		= js_getProperty<VJSTypedArrayObject::GetProperty<sLONG> >;	
}

void VJSUInt32ArrayClass::GetDefinition (ClassDefinition &outDefinition)
{
	static XBOX::VJSClass<VJSUInt32ArrayClass, VJSTypedArrayObject>::StaticFunction functions[] =
	{		
		{	"get",		js_callStaticFunction<VJSTypedArrayObject::Get<uLONG> >,							JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"set",		js_callStaticFunction<VJSTypedArrayObject::Set<uLONG> >,							JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"subArray",	js_callStaticFunction<VJSTypedArrayObject::SubArray<VJSUInt32ArrayClass, uLONG> >,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	0,			0,																					0																	},
	};

    outDefinition.className			= "UInt32Array";
	outDefinition.staticFunctions	= functions;	
	outDefinition.initialize		= js_initialize<VJSTypedArrayObject::Initialize<uLONG> >;
	outDefinition.finalize			= js_finalize<VJSTypedArrayObject::Finalize>;
	outDefinition.setProperty		= js_setProperty<VJSTypedArrayObject::SetProperty<uLONG> >;
	outDefinition.getProperty		= js_getProperty<VJSTypedArrayObject::GetProperty<uLONG> >;	
}

void VJSFloat32ArrayClass::GetDefinition (ClassDefinition &outDefinition)
{
	static XBOX::VJSClass<VJSFloat32ArrayClass, VJSTypedArrayObject>::StaticFunction functions[] =
	{		
		{	"get",		js_callStaticFunction<VJSTypedArrayObject::Get<SmallReal> >,							JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"set",		js_callStaticFunction<VJSTypedArrayObject::Set<SmallReal> >,							JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"subArray",	js_callStaticFunction<VJSTypedArrayObject::SubArray<VJSFloat32ArrayClass, SmallReal> >,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	0,			0,																						0																	},					
	};

    outDefinition.className			= "Float32Array";
	outDefinition.staticFunctions	= functions;	
	outDefinition.initialize		= js_initialize<VJSTypedArrayObject::Initialize<SmallReal> >;
	outDefinition.finalize			= js_finalize<VJSTypedArrayObject::Finalize>;
	outDefinition.setProperty		= js_setProperty<VJSTypedArrayObject::SetProperty<SmallReal> >;
	outDefinition.getProperty		= js_getProperty<VJSTypedArrayObject::GetProperty<SmallReal> >;	
}

void VJSFloat64ArrayClass::GetDefinition (ClassDefinition &outDefinition)
{
	static XBOX::VJSClass<VJSFloat64ArrayClass, VJSTypedArrayObject>::StaticFunction functions[] =
	{		
		{	"get",		js_callStaticFunction<VJSTypedArrayObject::Get<uLONG> >,							JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"set",		js_callStaticFunction<VJSTypedArrayObject::Set<Real> >,								JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"subArray",	js_callStaticFunction<VJSTypedArrayObject::SubArray<VJSFloat64ArrayClass, Real> >,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	0,			0,																					0																	},					
	};

    outDefinition.className			= "Float64Array";
	outDefinition.staticFunctions	= functions;	
	outDefinition.initialize		= js_initialize<VJSTypedArrayObject::Initialize<Real> >;
	outDefinition.finalize			= js_finalize<VJSTypedArrayObject::Finalize>;
	outDefinition.setProperty		= js_setProperty<VJSTypedArrayObject::SetProperty<Real> >;
	outDefinition.getProperty		= js_getProperty<VJSTypedArrayObject::GetProperty<Real> >;	
}
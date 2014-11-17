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

#ifndef __VJS_W3C_ARRAY_BUFFER__
#define __VJS_W3C_ARRAY_BUFFER__

#include "VJSClass.h"
#include "VJSValue.h"
#include "VJSBuffer.h"

// W3C Typed Array API implementation:
//
//	https://www.khronos.org/registry/typedarray/specs/latest/

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API VJSArrayBufferObject : public XBOX::IRefCountable
{
public:

					// Create an ArrayBuffer from binary data.

					VJSArrayBufferObject (VSize inLength, void *inBuffer);

					// From Buffer's object toArrayBuffer() method.

					VJSArrayBufferObject (VJSBufferObject *inBufferObject);

	bool			IsNeutered () const			{	return fBufferObject == NULL;	}

	VJSBufferObject	*GetBufferObject () const	{	return fBufferObject;	}

	VSize			GetDataSize () const		{	return fBufferObject != NULL ? fBufferObject->GetDataSize() : 0;	}
	void			*GetDataPtr () const		{	return fBufferObject != NULL ? fBufferObject->GetDataPtr() : 0;		}

private:

friend class VJSArrayBufferClass;

	// ArrayBuffer objects can be "neutered" after transfer (from one process to another, see cloning). 
	// If so, fBufferObject is NULL then.

	VJSBufferObject	*fBufferObject;		

					// From ArrayBuffer's constructor.

					VJSArrayBufferObject (VSize inLength);

					// If slice() method ends up with a zero size slice, create a zero sized ArrayBuffer.

					VJSArrayBufferObject ();

	virtual			~VJSArrayBufferObject ();	
};

class XTOOLBOX_API VJSArrayBufferClass : public XBOX::VJSClass<VJSArrayBufferClass, VJSArrayBufferObject>
{
public:

	static void				GetDefinition (ClassDefinition &outDefinition);

	static XBOX::VJSObject	MakeConstructor (XBOX::VJSContext inContext);

	// Create an ArrayBuffer object from binary data.

	static XBOX::VJSObject	NewInstance (XBOX::VJSContext inContext, VSize inLength, void *inBuffer);
	static void				Initialize(const VJSParms_initialize &inParms, VJSArrayBufferObject *inArrayBuffer);

private:

	static void				_Construct(XBOX::VJSParms_construct &ioParms);

	static void				_Finalize (const XBOX::VJSParms_finalize &inParms, VJSArrayBufferObject *inArrayBuffer);	

	static void				_GetProperty (XBOX::VJSParms_getProperty &ioParms, VJSArrayBufferObject *inArrayBuffer);

	static void				_slice (XBOX::VJSParms_callStaticFunction &ioParms, VJSArrayBufferObject *inArrayBuffer);
	static void				_toBuffer (XBOX::VJSParms_callStaticFunction &ioParms, VJSArrayBufferObject *inArrayBuffer);
};

class XTOOLBOX_API VJSTypedArrayObject : public VObject 
{
public:

	static void											AddConstructors (XBOX::VJSContext inContext, XBOX::VJSObject inObject);

	template <class CLASS, class TYPE> static void		Construct(XBOX::VJSParms_construct &ioParms);

	template <class TYPE> static void					Initialize (const VJSParms_initialize &inParms, VJSTypedArrayObject *inTypedArray);
	static void											Finalize (const XBOX::VJSParms_finalize &inParms, VJSTypedArrayObject *inTypedArray);

	template <class TYPE> static bool					SetProperty (XBOX::VJSParms_setProperty &ioParms, VJSTypedArrayObject *inTypedArray);
	template <class TYPE> static void					GetProperty (XBOX::VJSParms_getProperty &ioParms, VJSTypedArrayObject *inTypedArray);
	
	template <class TYPE> static void					Get (XBOX::VJSParms_callStaticFunction &ioParms, VJSTypedArrayObject *inTypedArray);
	template <class TYPE> static void					Set (XBOX::VJSParms_callStaticFunction &ioParms, VJSTypedArrayObject *inTypedArray);	
	template <class CLASS, class TYPE> static void		SubArray (XBOX::VJSParms_callStaticFunction &ioParms, VJSTypedArrayObject *inTypedArray);

private:

	enum {

		TYPE_INT8,
		TYPE_UINT8,

		TYPE_INT16,
		TYPE_UINT16,

		TYPE_INT32,
		TYPE_UINT32,

		TYPE_FLOAT32,
		TYPE_FLOAT64,

	};	

	sLONG												fByteOffset, fByteLength;
	VJSArrayBufferObject								*fArrayBufferObject;

														VJSTypedArrayObject (sLONG inByteOffset, sLONG inByteLength, VJSArrayBufferObject *inArrayBufferObject);
	virtual												~VJSTypedArrayObject ();

	template <class TYPE> static void					_SetValue (VJSTypedArrayObject *inTypedArray, sLONG index, const XBOX::VJSValue &inValue, bool inThrowException);
	template <class TYPE> static void					_CopyFromArray (TYPE *outDestination, const XBOX::VJSArray &inArray);
	template <class TYPE> static void					_CopyFromTypedArray (VJSTypedArrayObject *inTypedArray, sLONG index, const XBOX::VJSObject &inObject);
	template <class A, class B> static void				_TypedCopy (void *outDestination, sLONG inDestinationOffset, const void *inSource, sLONG inSourceOffset, sLONG inNumberElements);
};

class XTOOLBOX_API VJSInt8ArrayClass : public XBOX::VJSClass<VJSInt8ArrayClass, VJSTypedArrayObject>
{
public:

	static void		GetDefinition (ClassDefinition &outDefinition);
};

class XTOOLBOX_API VJSUInt8ArrayClass : public XBOX::VJSClass<VJSUInt8ArrayClass, VJSTypedArrayObject>
{
public:

	static void		GetDefinition (ClassDefinition &outDefinition);
};

class XTOOLBOX_API VJSInt16ArrayClass : public XBOX::VJSClass<VJSInt16ArrayClass, VJSTypedArrayObject>
{
public:

	static void		GetDefinition (ClassDefinition &outDefinition);
};

class XTOOLBOX_API VJSUInt16ArrayClass : public XBOX::VJSClass<VJSUInt16ArrayClass, VJSTypedArrayObject>
{
public:

	static void		GetDefinition (ClassDefinition &outDefinition);
};

class XTOOLBOX_API VJSInt32ArrayClass : public XBOX::VJSClass<VJSInt32ArrayClass, VJSTypedArrayObject>
{
public:

	static void		GetDefinition (ClassDefinition &outDefinition);
};

class XTOOLBOX_API VJSUInt32ArrayClass : public XBOX::VJSClass<VJSUInt32ArrayClass, VJSTypedArrayObject>
{
public:

	static void		GetDefinition (ClassDefinition &outDefinition);
};

class XTOOLBOX_API VJSFloat32ArrayClass : public XBOX::VJSClass<VJSFloat32ArrayClass, VJSTypedArrayObject>
{
public:

	static void		GetDefinition (ClassDefinition &outDefinition);
};

class XTOOLBOX_API VJSFloat64ArrayClass : public XBOX::VJSClass<VJSFloat64ArrayClass, VJSTypedArrayObject>
{
public:

	static void		GetDefinition (ClassDefinition &outDefinition);
};

END_TOOLBOX_NAMESPACE

#endif
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
#ifndef __VJS_BUFFER__
#define __VJS_BUFFER__

#include "VJSClass.h"
#include "VJSValue.h"

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API VJSBufferObject : public XBOX::IRefCountable
{
public:

	// Available encodings are those of CharSet. Negative numbers are used for those specific to Buffer.

	enum {

		eENCODING_ASCII		= -10,		// 'ascii' encoding is stripping to 7-bit.
		eENCODING_BASE64	= -11,	
		eENCODING_BINARY	= -12,		// Deprecated by NodeJS, but supported.
		eENCODING_HEX		= -13,

	};

	// Create a buffer object with an already existing buffer.
	// If inFromMemoryBuffer is false, inBuffer must have been allocated using ::malloc() as it will be freed by destructor 
	// using ::free(). If it is a data pointer "stolen" from a VMemoryBuffer<>, VMemory::DisposePtr() will be used instead.
	// It is possible to have a zero length buffer, but then inBuffer must be NULL.

					VJSBufferObject (VSize inLength, void *inBuffer, bool inFromMemoryBuffer = false);

	// Create a buffer object. fBuffer must be checked (out of memory) after constructor called.

					VJSBufferObject (VSize inLength);

	XBOX::VSize		GetDataSize () const	{	return fLength;	}
	void			*GetDataPtr () const	{	return fBuffer; }

	// If the encoding is unknown, then return XBOX::VTC_UNKNOWN.

	static CharSet	GetEncodingType (const XBOX::VString &inEncoding);

	// Decode a buffer to a VString using requested encoding.

	void			ToString (CharSet inEncoding, sLONG inStart, sLONG inEnd, XBOX::VString *outString);
	
	// Encode a string according to requested encoding. Return a negative length if the encoding fails.
	// Set *inBuffer to NULL if memory allocation is desired, it will be allocated using ::malloc(). 
	// Set inMaximumLength to a negative value if no limit.
	
	static sLONG	FromString (const XBOX::VString &inString, CharSet inEncoding, uBYTE **outBuffer, sLONG inMaximumLength = -1);
	
private:

friend class VJSBufferClass; 

	// slice() method allows creation of references to a "parent" buffer.
	// fParent is NULL if it is not a reference.

	VJSBufferObject	*fParent;	
	VSize			fLength;
	uBYTE			*fBuffer;
	bool			fFromMemoryBuffer;

	// Create a reference to a parent buffer (used by slice() method only). 

					VJSBufferObject (VJSBufferObject *inParent, sLONG inStart, sLONG inEnd);

	virtual			~VJSBufferObject ();

	static UniChar	_ToHex (uBYTE inValue);
	static sLONG	_FromHex (UniChar inUniChar);	// Return negative value if invalid.
};

class XTOOLBOX_API VJSBufferClass : public XBOX::VJSClass<VJSBufferClass , VJSBufferObject>
{
public:

	static void				GetDefinition (ClassDefinition &outDefinition);

	// Create a Buffer object from binary data.

	static XBOX::VJSObject	NewInstance (XBOX::VJSContext inContext, VSize inLength, void *inBuffer, bool inFromMemoryBuffer = false);

	static XBOX::VJSObject	MakeConstructor (const XBOX::VJSContext& inContext);
	static void				Initialize(const VJSParms_initialize &inParms, VJSBufferObject *inBuffer);

private:
	static	JS4D::StaticFunction sConstrFunctions[];

	typedef XBOX::VJSClass<VJSBufferClass, VJSBufferObject>	inherited;

	static void				_Construct( VJSParms_construct &ioParms);

	static void				_Finalize (const XBOX::VJSParms_finalize &inParms, VJSBufferObject *inBuffer);	

	static void				_GetProperty (XBOX::VJSParms_getProperty &ioParms, VJSBufferObject *inBuffer);
	static bool				_SetProperty (XBOX::VJSParms_setProperty &ioParms, VJSBufferObject *inBuffer);

	static void				_IsBuffer (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *);
	static void				_ByteLength (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *);

	static void				_write (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);

	static void				_toString (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);

	static void				_toBlob (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_toArrayBuffer (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);

	// Implementation uses ::memmove() to do the copy. It is hence possible to call copy() on itself, on overlapping area.

	static void				_copy (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);

	static void				_slice (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);

	static void				_fill (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);

	// Read/write from/to the buffer.
	//
	// Implementation notes:
	//
	//	* Unaligned reads or writes are possible (ok for x86 processors);
	//	* If "out of bound" reads or writes are done (set optional noAssert argument to true), read value can be padded
	//    with zeroes, and impossible to write bytes ignored;
	//	* The basic reading or writing without noAssert argument, is optimized to be fastest (this is the usual case).

	static void				_readUInt8 (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_readInt8 (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_writeUInt8 (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_writeInt8 (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);

	static void				_readUInt16LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_readUInt16BE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_readInt16LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);	
	static void				_readInt16BE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_writeUInt16LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_writeUInt16BE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_writeInt16LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_writeInt16BE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);

	static void				_readUInt24LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_readUInt24BE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_readInt24LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_readInt24BE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_writeUInt24LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_writeUInt24BE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_writeInt24LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_writeInt24BE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);

	static void				_readUInt32LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_readUInt32BE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_readInt32LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_readInt32BE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_writeUInt32LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_writeUInt32BE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_writeInt32LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_writeInt32BE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);

	static void				_readFloatLE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_readFloatBE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_writeFloatLE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_writeFloatBE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);

	static void				_readDoubleLE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_readDoubleBE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_writeDoubleLE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
	static void				_writeDoubleBE (XBOX::VJSParms_callStaticFunction &ioParms, VJSBufferObject *inBuffer);
};

END_TOOLBOX_NAMESPACE

#endif
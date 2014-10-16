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
#ifndef __VBlob__
#define __VBlob__

#include "Kernel/Sources/VValueSingle.h"

BEGIN_TOOLBOX_NAMESPACE

// Defined bellow
class VBlobWithPtr;

class XTOOLBOX_API VBlob_info : public VValueInfo_default<VBlobWithPtr,VK_BLOB>
{
public:
									VBlob_info( ValueKind inTrueKind = VK_BLOB):VValueInfo_default<VBlobWithPtr,VK_BLOB>( inTrueKind)			{;}
	virtual VSize					GetSizeOfValueDataPtr( const void* inPtrToValueData) const;
	virtual void*					ByteSwapPtr( void *inBackStore, bool inFromNative) const;
};


class XTOOLBOX_API VBlob : public VValueSingle
{
public:
	typedef VValueSingle inherited;

	typedef VBlob_info	TypeInfo;
	static const TypeInfo	sInfo;

	VBlob( bool inIsNull = false):inherited( inIsNull)			{;}

	// Returns bytes count
	virtual VSize	GetSize () const = 0;
	
	// Changes the size. (not needed before PutData).
	virtual VError	SetSize (VSize inNewSize) = 0;
	
	// Reads some bytes, returns number of bytes copied.
	virtual VError	GetData (void* inBuffer, VSize inNbBytes, VIndex inOffset = 0, VSize* outNbBytesCopied = NULL) const = 0;

	// Writes some bytes. Resizes if necessary.
	// WARNING: if the blob is already large enough, it won't be shrunk!
	virtual VError	PutData (const void* inBuffer, VSize inNbBytes, VIndex inOffset = 0) = 0;

	// Set the blob data to given bytes, no less no more. Bytes are copied.
	virtual VError	FromData( const void* inBuffer, VSize inNbBytes);

	virtual	void	Clear();

	// Inherited from VValueSingle
	virtual Boolean	CanBeEvaluated () const { return false; };

	virtual void GetValue( VValueSingle& outValue) const { ; }
	virtual void FromValue( const VValueSingle& inValue) { ; }
	
	virtual void*	LoadFromPtr (const void* inData, Boolean inRefOnly = false);
	virtual void*	WriteToPtr (void* ioData, Boolean inRefOnly = false, VSize inMax = 0) const;
	virtual VSize	GetSpace (VSize inMax = 0) const = 0;
	
	virtual VSize	GetFullSpaceOnDisk() const
	{
		return GetSize();
	}

	virtual CompareResult	CompareTo (const VValueSingle& inValue, Boolean inDiacritic = false) const;
	virtual	const VValueInfo*	GetValueInfo() const;

protected:
	virtual	void	DoNullChanged();
};

#if WITH_VMemoryMgr
// DEPRECATED
class XTOOLBOX_API VBlobWithHandle : public VBlob
{
public:
	typedef VBlob inherited;

	VBlobWithHandle ();
	VBlobWithHandle( const VBlobWithHandle& inBlob );
	VBlobWithHandle (VPtr inData, sLONG inLength);
	virtual ~VBlobWithHandle ();

	// Returns bytes count
	virtual VSize	GetSize () const;

	// Changes the size of the handle. (not needed before PutData).
	virtual VError	SetSize (VSize inNewSize);

	// Reads some bytes, returns number of bytes copied.
	virtual VError	GetData (void* inBuffer, VSize inNbBytes, VIndex inOffset = 0, VSize* outNbBytesCopied = NULL) const;

	// Writes some bytes. Resizes the handle if necessary.
	virtual VError	PutData (const void* inBuffer, VSize inNbBytes, VIndex inOffset = 0);

	// Inherited from VValueSingle
	virtual Boolean	FromValueSameKind (const VValue* inValue);
	virtual VBlobWithHandle*	Clone () const;

	virtual void GetValue( VValueSingle& outValue) const;
	virtual void FromValue( const VValueSingle& inValue);

	virtual void*	LoadFromPtr (const void* inData, Boolean inRefOnly = false);
	virtual void*	WriteToPtr (void* ioData, Boolean inRefOnly = false, VSize inMax = 0) const;
	virtual VSize	GetSpace (VSize inMax = 0) const;

	// Inherited from VObject
	virtual VError	ReadFromStream (VStream* ioStream, sLONG inParam = 0);
	virtual VError	WriteToStream (VStream* ioStream, sLONG inParam = 0) const;	

	// only for VBlobWithHandle

	inline VHandle GetData() const { return fData; };

protected:
	VHandle	fData;
};
#endif

class XTOOLBOX_API VBlobWithPtr : public VBlob
{
public:
	typedef VBlob inherited;

							// jus like other VValue, VBlob is built not null.
							VBlobWithPtr():fData( NULL), fSize( 0), fAllocatedSize( 0)	{;}
							VBlobWithPtr( const VBlob& inBlob );
							VBlobWithPtr( const VBlobWithPtr& inBlob );
							VBlobWithPtr( const void *inData, VSize inSize); // copy

	
	/**
		if inCopy is true, VBlob takes a copy of specified data.
		if inCopy is false, VBlob works directly with it but won't try to delete/resize it.
		The VValue is built not null.
	**/
							VBlobWithPtr( void *inData, VSize inSize, bool inCopy);

	virtual					~VBlobWithPtr();

	/**
		@brief Returns bytes count
	**/
	virtual VSize			GetSize() const;

	/**
		@brief Changes the size of the blob.
		Allocated size and data size become what you asked.
		If inSize is zero, the data pointer becomes NULL but VValue::IsNull is left unchanged.
		The blob becomes the owner of the data.
	**/
	virtual VError			SetSize( VSize inSize);

	/**
		@brief Reads some bytes, returns number of bytes copied.
	**/
	virtual VError			GetData( void* inBuffer, VSize inNbBytes, VIndex inOffset = 0, VSize* outNbBytesCopied = NULL) const;

	/**
		@brief Writes some bytes. Increase the allocated memory if necessary but never decrease it.
		may allocate more bytes than necessary for performance reasons.
		Warning: PutData never shrink the blob. You may need to call SetSize before or prefer FromData.
		The blob becomes the owner of the data.
	**/
	virtual VError			PutData( const void* inBuffer, VSize inNbBytes, VIndex inOffset = 0);

	/**
		@brief Set the blob data to given bytes, no less no more. Bytes are copied.
		The blob becomes the owner of the data.
	**/
	virtual VError			FromData( const void* inBuffer, VSize inNbBytes);

	//	Inherited from VValueSingle

	virtual Boolean			FromValueSameKind( const VValue* inValue);
	virtual VBlobWithPtr*	Clone() const;

	virtual void*			LoadFromPtr( const void* inData, Boolean inRefOnly = false);
	virtual void*			WriteToPtr( void* ioData, Boolean inRefOnly = false, VSize inMax = 0) const;
	virtual VSize			GetSpace( VSize inMax = 0) const;

	virtual void GetValue( VValueSingle& outValue) const;
	virtual void FromValue( const VValueSingle& inValue);

	// Inherited from VObject

	virtual VError			ReadFromStream( VStream* ioStream, sLONG inParam = 0);
	virtual VError			WriteToStream( VStream* ioStream, sLONG inParam = 0) const;	


	// only for VBlobWithPtr

			const void*		GetDataPtr() const						{ return fData; }	// may be NULL
			void*			GetDataPtr()							{ return fData; }	// may be NULL

			bool			IsEmpty() const							{ return (fData == NULL) || (fSize == 0) || IsNull();}

	/**
		@brief Steal the memory pointer from the Blob.
		The blob becomes empty.
		If the blob was the data owner, you must use VMemory::DisposePtr() to deallocate the pointer you get.
	**/
			void*			StealData()								{ void *p = fData; fData = NULL; fSize = 0; return p;}

protected:
			VSize			_AdjusteSize( VSize inSize)				{ return (inSize+1023) & (~1023); }
			bool			_Allocate( VSize inSize);
			bool			_Reallocate( VSize inSize);

			// ~VBlob will dispose fData if true
			bool			_IsPointerOwner() const					{ return GetFlag( Value_flag1);}
			void			_SetPointerOwner()						{ SetFlag( Value_flag1);}
			void			_ClearPointerOwner()					{ ClearFlag( Value_flag1);}

			void*			fData;
			VSize			fSize;
			VSize			fAllocatedSize;
};




END_TOOLBOX_NAMESPACE

#endif

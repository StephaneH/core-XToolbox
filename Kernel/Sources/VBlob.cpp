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
#include "VKernelPrecompiled.h"
#include "VBlob.h"
#include "VMemoryCpp.h"
#include "VStream.h"

const VBlob::TypeInfo	VBlob::sInfo;


VSize VBlob_info::GetSizeOfValueDataPtr( const void* inPtrToValueData) const
{
	return *(sLONG *) inPtrToValueData + sizeof(sLONG);		// sc 03/05/2007, add sizeof(sLONG) (see WriteToPtr())
}

void* VBlob_info::ByteSwapPtr( void *inBackStore, bool /*inFromNative*/) const
{
	ByteSwap( (sLONG *)inBackStore);
	return ( (sLONG *)inBackStore) + 1;
}



//==========================================================================================



CompareResult VBlob::CompareTo (const VValueSingle& /*inVal*/, Boolean /*inDiacritic*/) const
{
	return CR_UNRELEVANT;
}

void* VBlob::LoadFromPtr(const void* /*inData*/, Boolean /*inRefOnly*/)
{
	assert(false);
	return NULL;
}


void* VBlob::WriteToPtr(void* /*inData*/, Boolean /*inRefOnly*/, VSize inMax) const
{
	assert(false);
	return NULL;
}


const VValueInfo *VBlob::GetValueInfo() const
{
	return (&sInfo);
}


VError VBlob::FromData( const void* inBuffer, VSize inNbBytes)
{
	VError err = SetSize( inNbBytes);
	if ( (err == VE_OK) && (inNbBytes > 0) )
		err = PutData( inBuffer, inNbBytes);
	
	return err;
}


void VBlob::DoNullChanged()
{
	if (IsNull())
		SetSize( 0);
}


void VBlob::Clear()
{
	FromData( NULL, 0);
}


					/* -------------------------------------------------------- */

#if WITH_VMemoryMgr

VBlobWithHandle::VBlobWithHandle()
{
	fData = NULL;
}


VBlobWithHandle::VBlobWithHandle(VPtr inData, sLONG inLength)
{
	fData = VMemory::NewHandleFromData(inData, inLength);
	SetNull(false);
}

VBlobWithHandle::VBlobWithHandle( const VBlobWithHandle& inBlob )
{
	fData = inBlob.fData == NULL ? NULL : VMemory::NewHandleFromHandle( inBlob.GetData() );
	SetNull(inBlob.IsNull());
}

VBlobWithHandle::~VBlobWithHandle()
{
	if (fData != NULL)
		VMemory::DisposeHandle(fData);
}

void VBlobWithHandle::GetValue( VValueSingle& outValue) const
{
	xbox_assert(false);
}

void VBlobWithHandle::FromValue( const VValueSingle& inValue)
{
	xbox_assert(false);
}


VBlobWithHandle* VBlobWithHandle::Clone() const
{
	return new VBlobWithHandle(*this);
}



Boolean VBlobWithHandle::FromValueSameKind(const VValue *inValue)
{
	const VBlobWithHandle *theBlob = dynamic_cast<const VBlobWithHandle*>(inValue);
	if (fData != NULL)
		VMemory::DisposeHandle(fData);
	fData = (theBlob->fData == NULL) ? NULL : VMemory::NewHandleFromHandle(theBlob->fData);
	SetNull(theBlob->IsNull());
	return (fData != NULL) || (theBlob->fData == NULL);
}


VError VBlobWithHandle::GetData(void* inBuffer, VSize inNbBytes, VIndex inOffset, VSize* outNbBytesCopied) const
{
	VError err = VE_OK;

	VSize size = (fData != NULL) ? VMemory::GetHandleSize(fData) : 0;

	VSize nbBytes = inNbBytes;
	if (inOffset + nbBytes > size)
		nbBytes = size - inOffset;

	if (nbBytes > 0) {
		VPtr p = VMemory::LockHandle(fData);
		::CopyBlock(p + inOffset, inBuffer, nbBytes);
		VMemory::UnlockHandle(fData);
	}

	if (outNbBytesCopied != NULL)
		*outNbBytesCopied = nbBytes;

	if (nbBytes != inNbBytes)
		err = VE_STREAM_EOF;
	
	return err;
}


VError VBlobWithHandle::PutData(const void* inBuffer, VSize inNbBytes, VIndex inOffset)
{
	VError err = VE_OK;
	if (fData == NULL) {
		if (inOffset == 0) {
			fData = VMemory::NewHandleFromData((VPtr) inBuffer, inNbBytes);
		} else {
			fData = VMemory::NewHandleFromData((VPtr) inBuffer, inOffset + inNbBytes);
			if (fData != NULL) {
				VPtr p = VMemory::LockHandle(fData);
				if (p != NULL)
				{
					::CopyBlock(inBuffer, p + inOffset, inNbBytes);
					SetNull(false);
				}
				VMemory::UnlockHandle(fData);
			}
		}
		if (fData == NULL)
			err = VE_MEMORY_FULL;
	} else {
		if (VMemory::GetHandleSize(fData) < (VSize) (inOffset + inNbBytes)) {
			if (!VMemory::SetHandleSize(fData, inOffset + inNbBytes))
				err = VE_MEMORY_FULL;
		}
		if (err == VE_OK) {
			VPtr p = VMemory::LockHandle(fData);
			if (p != NULL)
			{
				::CopyBlock(inBuffer, p + inOffset, inNbBytes);
				SetNull(false);
			}
			VMemory::UnlockHandle(fData);
		}
	}
	return err;
}


VError VBlobWithHandle::SetSize(VSize inNewSize)
{
	VError err = VE_OK;
	if (fData == NULL)
	{
		if (inNewSize > 0)
		{
			fData = VMemory::NewHandle(inNewSize);
			if (fData == NULL)
				err = VE_MEMORY_FULL;
		}
	}
	else
	{
		if (!VMemory::SetHandleSize(fData, inNewSize))
			err = VE_MEMORY_FULL;
	}
	return err;
}


VSize VBlobWithHandle::GetSize() const
{
	return VMemory::GetHandleSize(fData);
}

void* VBlobWithHandle::LoadFromPtr (const void* inData, Boolean /*inRefOnly*/)
{
	if (fData != NULL)
	{
		VMemory::DisposeHandle( fData);
		fData = NULL;
	}
	sLONG size = *(sLONG *)inData;
	if (size>0)
		PutData( (VPtr)inData+sizeof(sLONG), size);

	return (VPtr)inData+sizeof(sLONG)+size;
}

void* VBlobWithHandle::WriteToPtr (void* ioData, Boolean /*inRefOnly*/, VSize inMax) const
{
	VSize size = (fData) ? VMemory::GetHandleSize(fData) : 0;
	*((sLONG*)ioData) = (sLONG) size;
	if (fData != NULL)
		GetData( (VPtr)ioData+sizeof(sLONG), size);

	return (VPtr)ioData+sizeof(sLONG)+size;
}

VSize VBlobWithHandle::GetSpace (VSize /* inMax */) const
{
	return (((fData)?VMemory::GetHandleSize(fData):0) + sizeof(sLONG));
}

VError	VBlobWithHandle::ReadFromStream(VStream* ioStream, sLONG /*inParam*/)
{
	VError err = VE_OK;
	
	if (fData != NULL)
	{
		VMemory::DisposeHandle( fData);
		fData = NULL;
	}
	sLONG size = 0;
	err = ioStream->GetLong( size);
	if (err==VE_OK && size>0)
	{
		fData = VMemory::GetManager()->NewHandle( size);
		if (fData == NULL)
			err = VE_MEMORY_FULL;
		if (err == VE_OK)
		{
			VPtr ptr = VMemory::LockHandle( fData);
			err = ioStream->GetData( ptr, size);
			VMemory::UnlockHandle( fData);
		}
	}
	return err;
}


VError	VBlobWithHandle::WriteToStream(VStream* ioStream, sLONG /*inParam*/) const
{
	VSize size = (fData) ? VMemory::GetHandleSize(fData) : 0;
	VError err = ioStream->PutLong( (sLONG) size);
	if (err==VE_OK && fData!=NULL)
	{
		VPtr ptr = VMemory::LockHandle( fData);
		err = ioStream->PutData( ptr, size);
		VMemory::UnlockHandle( fData);
	}
	return err;
}

#endif


/* -------------------------------------------------------- */



VBlobWithPtr::VBlobWithPtr( const void *inData, VSize inSize)
{
	if (_Allocate( inSize))
	{
		::CopyBlock( inData, fData, inSize);
	}
}


VBlobWithPtr::VBlobWithPtr( void *inData, VSize inSize, bool inCopy)
{
	if (inCopy)
	{
		if (_Allocate( inSize))
		{
			::CopyBlock( inData, fData, inSize);
		}
	}
	else
	{
		_ClearPointerOwner();
		fData = inData;
		fSize = fAllocatedSize = inSize;
	}
}


VBlobWithPtr::VBlobWithPtr( const VBlobWithPtr& inBlob ):inherited( inBlob)	// copy null state
{
	VSize size = inBlob.IsNull() ? 0 : inBlob.fSize;

	if (_Allocate( size))
	{
		::CopyBlock( inBlob.fData, fData, inBlob.fSize);
	}
}


VBlobWithPtr::VBlobWithPtr( const VBlob& inBlob ):inherited( inBlob)	// copy null state
{
	VSize size = inBlob.IsNull() ? 0 : inBlob.GetSize();
	
	if (_Allocate( size))
	{
		inBlob.GetData( fData, size);
	}
}


bool VBlobWithPtr::_Allocate( VSize inSize)
{
	bool ok;
	_SetPointerOwner();
	if (inSize > 0)
	{
		fData = VMemory::NewPtr( inSize, 'blbP');
		if (fData == NULL)
		{
			fSize = fAllocatedSize = 0;
			ok = false;
		}
		else
		{
			fSize = fAllocatedSize = inSize;
			ok = true;
		}
	}
	else
	{
		fData = NULL;
		fSize = fAllocatedSize = 0;
		ok = true;
	}
	return ok;
}


bool VBlobWithPtr::_Reallocate( VSize inSize)
{
	bool ok;
	if (_IsPointerOwner())
	{
		if (fAllocatedSize != inSize)
		{
			if (inSize == 0)
			{
				VMemory::DisposePtr( fData);
				fData = NULL;
				fSize = fAllocatedSize = 0;
				ok = true;
			}
			else
			{
				void *newData;
				if (fData != NULL)
				{
					newData = VMemory::SetPtrSize( fData, inSize);
					if (newData != NULL)
					{
						fData = newData;
						fSize = fAllocatedSize = inSize;
						ok = true;
					}
					else
					{
						ok = false;
					}
				}
				else
				{
					ok = _Allocate( inSize);
				}
			}
		}
		else
		{
			ok = true;
		}
	}
	else
	{
		const void *oldData = fData;
		VSize oldSize = fSize;
		ok = _Allocate( inSize);
		if (ok && (oldSize > 0))
		{
			// recopie des anciennes donnees
			::CopyBlock( oldData, fData, std::min<VSize>( oldSize, fSize));
		}
	}
	return ok;
}


VBlobWithPtr::~VBlobWithPtr()
{
	if (_IsPointerOwner() && (fData != NULL))
		VMemory::DisposePtr( fData);
}


VBlobWithPtr* VBlobWithPtr::Clone() const
{
	return new VBlobWithPtr( *this);
}


Boolean VBlobWithPtr::FromValueSameKind( const VValue *inValue)
{
	const VBlobWithPtr *theBlob = dynamic_cast<const VBlobWithPtr*>(inValue);
	if ( !theBlob )
		return false;

	VSize size = (theBlob == NULL || theBlob->IsNull()) ? 0 : theBlob->GetSize();

	if (_Reallocate( size))
	{
		::CopyBlock( theBlob->fData, fData, theBlob->fSize);
	}

	GotValue( inValue->IsNull());
	return (fSize != 0) || (size == 0);
}


VError VBlobWithPtr::GetData( void* inBuffer, VSize inNbBytes, VIndex inOffset, VSize* outNbBytesCopied) const
{
	VError err = VE_OK;

	VSize size = fSize;

	VSize nbBytes = inNbBytes;
	if (inOffset + nbBytes > size)
		nbBytes = size - inOffset;

	if (nbBytes > 0)
	{
		::CopyBlock( (char*) fData + inOffset, inBuffer, nbBytes);
	}

	if (outNbBytesCopied != NULL)
		*outNbBytesCopied = nbBytes;

	if (nbBytes != inNbBytes)
		err = VE_STREAM_EOF;

	return err;
}


VError VBlobWithPtr::PutData( const void* inBuffer, VSize inNbBytes, VIndex inOffset)
{
	VError err = VE_OK;
	VSize neededDataLen = inOffset + inNbBytes;

	// a la premiere allocation, on prend la taille exacte,
	// ensuite on agrandit de 50% (max: 1Mo) arrondi a 1024.

	if (fData == NULL)
	{
		if (_Allocate( neededDataLen)) 
		{
			::CopyBlock( inBuffer, (char*) fData + inOffset, inNbBytes);
		}
		else
		{
			err = VE_MEMORY_FULL;
		}
	}
	else if (_IsPointerOwner())
	{
		if (fAllocatedSize < neededDataLen) 
		{
			VSize bonus = neededDataLen / 2;
			if (bonus > 1024*1024)
				bonus = 1024*1024;
			VSize newAllocatedSize = _AdjusteSize( neededDataLen + bonus);

			VPtr newdata = VMemory::SetPtrSize( fData, newAllocatedSize);
			if (newdata == NULL)
			{
				// on reessaie avec le strict necessaire
				newAllocatedSize = neededDataLen;
				newdata = VMemory::SetPtrSize( fData, newAllocatedSize);
			}

			if (newdata == NULL)
				err = VE_MEMORY_FULL;
			else
			{
				fData = newdata;
				fSize = neededDataLen;
				fAllocatedSize = newAllocatedSize;
			}
		}
		else if (neededDataLen > fSize)
		{
			fSize = neededDataLen; // sc 09/01/2006
		}
		if (err == VE_OK)
		{
			::CopyBlock( inBuffer, (char*) fData + inOffset, inNbBytes);
		}
	}
	else
	{
		err = VE_STREAM_CANNOT_WRITE;
	}

	GotValue( false);

	return err;
}


VError VBlobWithPtr::FromData( const void* inBuffer, VSize inNbBytes)
{
	VError err;

	if (_Reallocate( inNbBytes))
	{
		::CopyBlock( inBuffer, fData, inNbBytes);
		err = VE_OK;
	}
	else
	{
		err = VE_MEMORY_FULL;
	}

	GotValue( false);

	return err;
}

void VBlobWithPtr::GetValue( VValueSingle& outValue) const
{
	if (outValue.GetValueKind()==VK_BLOB)
	{
		VBlob *blb = dynamic_cast<VBlob*> (&outValue);

		if (blb != NULL)
		{
			blb->FromData(GetDataPtr(),GetSize());
		}
		else
			xbox_assert(false);
	}
	else
		xbox_assert(false);
}

void VBlobWithPtr::FromValue( const VValueSingle& inValue)
{
	Clear();
	if (inValue.GetValueKind()==VK_BLOB)
	{
		VBlobWithPtr *blb = dynamic_cast<VBlobWithPtr*> ( ( VValueSingle* ) &inValue );

		if (blb != NULL)
		{
			PutData(blb->GetDataPtr(),blb->GetSize());
		}
		else
			xbox_assert(false);
	}
	else
		xbox_assert(false);
}

VError VBlobWithPtr::SetSize( VSize inSize)
{
	VError err;
	if (_Reallocate( inSize))
		err = VE_OK;
	else
		err = VE_MEMORY_FULL;
	return err;
}


VSize VBlobWithPtr::GetSize() const
{
	return fSize;
}

void* VBlobWithPtr::LoadFromPtr (const void* inData, Boolean /*inRefOnly*/)
{
	sLONG datalen = *(sLONG*)inData;
	if (datalen > 0)
		FromData( (const char*) inData + sizeof(sLONG), datalen);

	return (VPtr)inData + sizeof(sLONG) + datalen;
}

void* VBlobWithPtr::WriteToPtr (void* ioData, Boolean /*inRefOnly*/, VSize inMax) const
{
	*((sLONG*)ioData) = (sLONG) fSize;
	if (fData != NULL)
		GetData( (char*) ioData + sizeof(sLONG), fSize);

	return (VPtr)ioData + sizeof(sLONG) + fSize;
}

VSize VBlobWithPtr::GetSpace( VSize /* inMax */) const
{
	return (fSize + sizeof(sLONG));
}

VError	VBlobWithPtr::ReadFromStream(VStream* ioStream, sLONG /*inParam*/)
{
	sLONG datalen = 0;
	VError err = ioStream->GetLong( datalen);
	if (err == VE_OK)
	{
		err = SetSize( datalen);
		if ( (err == VE_OK) && (datalen > 0) )
			err = ioStream->GetData( fData, datalen);
	}
	return err;
}


VError	VBlobWithPtr::WriteToStream(VStream* ioStream, sLONG /*inParam*/) const
{
	VError err = ioStream->PutLong( (sLONG) fSize);
	if ( (err==VE_OK) && (fData!=NULL) )
		err = ioStream->PutData( fData, fSize);

	return err;
}




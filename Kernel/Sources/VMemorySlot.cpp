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
#include "VMemorySlot.h"
#include "VError.h"
#include "VMemoryCpp.h"

#if WITH_VMemoryMgr

BEGIN_TOOLBOX_NAMESPACE

typedef struct HandleBlock
{
	HandleRef	fHandle;
	VSize		fPhysicalSize;
	VSize		fLogicalSize;
	uWORD		fAtomicSize;
	uWORD		fLockCounter;
#if USE_CHECK_LEAKS
	VStackCrawl	fStackCrawl;
#endif
} HandleBlock;

END_TOOLBOX_NAMESPACE


VMemSlotMgr::VMemSlotMgr()
{
	fSlotHandle = fMemMgr.NewHandle(0);
	fSlotPtr = (HandleBlock*) fMemMgr.LockHandle(fSlotHandle, 1);
	
	fAllocatedSlots = 0;
	fUsedSlots = 0;
	fLastFreeSlot = 0;

#if USE_CHECK_LEAKS
	fLeaksChecker.SetStackFramesToSkip(1);
#endif
}


VMemSlotMgr::~VMemSlotMgr()
{
	HandleBlock*	block = fSlotPtr + fUsedSlots;
		
	while (--block >= fSlotPtr)
	{
		if (block->fHandle)
		{
			while (block->fLockCounter--)
				fMemMgr.UnlockHandle(block->fHandle, block->fLockCounter);

			fMemMgr.DisposeHandle(block->fHandle);
		}
	}
	
	fMemMgr.UnlockHandle(fSlotHandle, 0);
	fMemMgr.DisposeHandle(fSlotHandle);
}


bool VMemSlotMgr::SetWithLeaksCheck(bool inSetIt)
{
#if USE_CHECK_LEAKS
	return fLeaksChecker.SetLeaksCheck(inSetIt) != 0;
#else
	return false;
#endif
}


void VMemSlotMgr::DumpLeaks()
{
#if USE_CHECK_LEAKS
	FILE*	file = fLeaksChecker.OpenDumpFile("VHandle");
	HandleBlock*	block = fSlotPtr + fUsedSlots;
		
	while (--block >= fSlotPtr)
	{
		if (block->fHandle)
		{
			fprintf(file, "lost 1 handle of size : %d\n", (int) block->fLogicalSize);
			block->fStackCrawl.Dump(file);
			fprintf(file, "\n");
		}
	}
	
	fclose(file);
#endif
}


void VMemSlotMgr::_LooseSlot(HandleBlock* inHandleBlock)
{
	// on defait les locks de la handle
	while (inHandleBlock->fLockCounter--)
	{
		fMemMgr.UnlockHandle(inHandleBlock->fHandle, inHandleBlock->fLockCounter);
	}

	// on met la handle a zero pour pouvoir liberer le memory manager
	// en sachant quelles sont les handles valides
	inHandleBlock->fHandle = 0;

	// on chaine les slots libre en gardant dans fLastFreeSlot le dernier slot libere
	// et en mettant dans fLogicalSize le numero du prochain slot libre
	inHandleBlock->fLogicalSize = (VSize) fLastFreeSlot;
	fLastFreeSlot = 1 + (inHandleBlock - fSlotPtr);
}


void VMemSlotMgr::DisposeHandle(VHandle ioHandle)
{
	HandleBlock*	block = _GetHandleBlock(ioHandle);
	if (block == NULL) return;
	
	HandleRef	handle = block->fHandle;
	_LooseSlot(block);
	
	fMemMgr.DisposeHandle(handle);
}


VSize VMemSlotMgr::GetHandleSize(const VHandle inHandle)
{
	HandleBlock*	block = _GetHandleBlock(inHandle);
	if (block == NULL) return 0;
	
	return block->fLogicalSize;
}


bool VMemSlotMgr::SetHandleSize(VHandle ioHandle, VSize inNewSize)
{
	return _SetHandleSize(ioHandle, inNewSize, false);
}


bool VMemSlotMgr::SetHandleSizeClear(VHandle ioHandle, VSize inNewSize)
{
	return _SetHandleSize(ioHandle, inNewSize, true);
}


bool VMemSlotMgr::_SetHandleSize(VHandle ioHandle, VSize inNewSize, bool inClear)
{
	VSize	newsize;
	bool	doResize;
	
	HandleBlock*	block = _GetHandleBlock(ioHandle);
	if (block == 0) return false;

	if (block->fLockCounter > 0)
	{
		vThrowError(VE_HANDLE_LOCKED);
		return false;
	}

	if (inNewSize >= block->fLogicalSize)
		doResize = (inNewSize > block->fPhysicalSize);
	else
		doResize = (inNewSize < block->fPhysicalSize - block->fAtomicSize);

	if (!doResize)
	{
		if (inClear && (inNewSize > block->fLogicalSize))
		{
			VPtr pt = fMemMgr.LockHandle(block->fHandle, block->fLockCounter);
			memset(pt + block->fLogicalSize, 0, inNewSize - block->fLogicalSize);
			fMemMgr.UnlockHandle(block->fHandle, block->fLockCounter);
		}
		block->fLogicalSize = inNewSize;
		return true;
	}

	if (block->fAtomicSize)
		newsize = block->fAtomicSize * (1 + (inNewSize / block->fAtomicSize));
	else
		newsize = inNewSize;
 
	HandleRef	handle = fMemMgr.SetLogicalHandleSize(block->fHandle, newsize);

	if (handle == 0)
	{
		vThrowError(VE_MEMORY_FULL);
		return false;
	}
	else
		block->fHandle = handle;

	if (inClear && (inNewSize > block->fLogicalSize))
	{
		VPtr pt = fMemMgr.LockHandle(block->fHandle, block->fLockCounter);
		memset(pt + block->fLogicalSize, 0, inNewSize - block->fLogicalSize);
		fMemMgr.UnlockHandle(block->fHandle, block->fLockCounter);
	}

	block->fPhysicalSize = fMemMgr.GetPhysicalHandleSize(block->fHandle);
	block->fLogicalSize = inNewSize;

	return true;
}


uLONG VMemSlotMgr::_GetSlot()
{
	HandleBlock*	block;
	uLONG	freeSlot;

	// si on a des slots libres on les utilise
	if (fLastFreeSlot)
	{
		// on lit dans fLogicalSize le numero du prochain slot libre
		freeSlot = fLastFreeSlot;
		block = fSlotPtr + freeSlot - 1;
		fLastFreeSlot = (uLONG) block->fLogicalSize;
#if USE_CHECK_LEAKS
		new(&block->fStackCrawl) VStackCrawl;
#endif
		return freeSlot;
	}

	// on a utilise tous nos slots, il faut en reallouer par tranches de 128
	if (fAllocatedSlots == fUsedSlots)
	{
		fMemMgr.UnlockHandle(fSlotHandle, 0);

		if (!fMemMgr.SetLogicalHandleSize(fSlotHandle, (fAllocatedSlots + 128) * sizeof(HandleBlock)))
		{
			 vThrowError(VE_MEMORY_FULL);
			 return 0;
		}

		fSlotPtr = (HandleBlock*) fMemMgr.LockHandle(fSlotHandle, 1);
		fAllocatedSlots += 128;
	}
#if USE_CHECK_LEAKS
	new(&(fSlotPtr + fUsedSlots)->fStackCrawl) VStackCrawl;
#endif	
	return ++fUsedSlots;
}


bool VMemSlotMgr::IsLocked(const VHandle inHandle)
{
	HandleBlock*	block = _GetHandleBlock(inHandle);
	if (block == 0) return false;
	
	return (block->fLockCounter > 0);
}


HandleBlock* VMemSlotMgr::_GetHandleBlock(const VHandle inHandle)
{
	if (inHandle == NULL) return NULL;
	
	HandleBlock*	block;
	
	// L.E. 13/12/1999 to better crash on mac by wrong dereferencing
	uLONG	slot = (uLONG_PTR) inHandle;
	slot = ~(slot | 0x80000000);
	
	if (slot > 0 && slot <= fUsedSlots)
	{
		block = fSlotPtr + slot - 1;
		
		if (block->fHandle)
			return block;
	}

	vThrowError(VE_NOT_A_HANDLE);
	return NULL;
}


uLONG VMemSlotMgr::_NewHandle(HandleBlock** outHandleBlock, VSize inSize, uWORD inAtomicSize)
{
	uLONG	slot = _GetSlot();
	
	if (slot == 0) return 0;
	
	HandleBlock*	block = *outHandleBlock = fSlotPtr + slot - 1;

	// L.E. 13/12/1999 to better crash on mac by wrong dereferencing
	slot = ~(slot | 0x80000000);

	block->fLogicalSize = inSize;
	block->fAtomicSize = inAtomicSize;
	block->fLockCounter = 0;

	if (inAtomicSize)
		inSize = inAtomicSize * (1 + (inSize / inAtomicSize));
	
	block->fHandle = fMemMgr.NewHandle(inSize);
	block->fPhysicalSize = fMemMgr.GetPhysicalHandleSize(block->fHandle);

	//MoB: ACI0052628 Crash after long time in XML IMPORT
	if (block->fHandle == NULL)
	{
		_LooseSlot(block);
		*outHandleBlock = NULL;
		slot=0;
	}

	#if USE_CHECK_LEAKS
	if (slot)
		fLeaksChecker.RegisterBlock(block->fStackCrawl);
	#endif

	return slot;
}


VHandle VMemSlotMgr::NewHandleFromData( const void *inData, VSize inLength)
{
	HandleBlock*	block;
	uLONG	slot = _NewHandle(&block, inLength, 0);

	if (slot == 0) return NULL;

	VPtr	ptr = fMemMgr.LockHandle(block->fHandle, block->fLockCounter);
	CopyBlock(inData, ptr, inLength);
	
	fMemMgr.UnlockHandle(block->fHandle, block->fLockCounter);
	return (VHandle) (uLONG_PTR) slot;
}


VPtr VMemSlotMgr::NewPtr(const VHandle inHandle)
{	
	HandleBlock*	block = _GetHandleBlock(inHandle);
	
	if (block == NULL) return NULL;
	
	VPtr	destPtr = VObject::GetMainMemMgr()->NewPtr(block->fLogicalSize, false, 'slot');

	if (destPtr != NULL && block->fLogicalSize)
	{
		VPtr	srcPtr = fMemMgr.LockHandle(block->fHandle, block->fLockCounter);
		CopyBlock(srcPtr, destPtr, block->fLogicalSize);
		fMemMgr.UnlockHandle(block->fHandle, block->fLockCounter);
	}

	return destPtr;
}


VHandle VMemSlotMgr::NewHandleClear(VSize inSize, uWORD inAtomicSize)
{
	HandleBlock*	block;
	uLONG	slot = _NewHandle(&block, inSize, inAtomicSize);

	if (slot == 0) return NULL;

	VPtr	ptr = fMemMgr.LockHandle(block->fHandle, block->fLockCounter);
	memset(ptr, 0, inSize);
	
	fMemMgr.UnlockHandle(block->fHandle, block->fLockCounter);
	return (VHandle) (uLONG_PTR) slot;
}


VHandle VMemSlotMgr::NewHandleFromHandle(const VHandle inOriginal)
{
	HandleBlock*	block = _GetHandleBlock(inOriginal);
	if (block == 0) return 0;

	HandleBlock*	newHandle;
	uLONG	slot = _NewHandle(&newHandle, block->fLogicalSize, block->fAtomicSize);
	
	if (slot == 0) return 0;
	
	block = _GetHandleBlock(inOriginal);
	if (block == NULL) return 0;

	VPtr	srcPtr = fMemMgr.LockHandle(block->fHandle, block->fLockCounter);
	VPtr	dstPtr = fMemMgr.LockHandle(newHandle->fHandle, newHandle->fLockCounter);

	CopyBlock(srcPtr, dstPtr, newHandle->fLogicalSize);
	
	fMemMgr.UnlockHandle(block->fHandle, block->fLockCounter);
	fMemMgr.UnlockHandle(newHandle->fHandle, newHandle->fLockCounter);
	
	return (VHandle) (uLONG_PTR) slot;
}


VHandle VMemSlotMgr::NewHandle(VSize inLogicalSize, uWORD inAtomicSize)
{
	HandleBlock*	block;
	return (VHandle) (uLONG_PTR) _NewHandle(&block, inLogicalSize, inAtomicSize);
}


HandleRef VMemSlotMgr::ForgetTrueHandle(VHandle ioHandle)
{
	HandleBlock*	block = _GetHandleBlock(ioHandle);
	if (block == 0) return 0;

	HandleRef	handle = block->fHandle;
	_LooseSlot(block);

	return handle;
}


VHandle VMemSlotMgr::InsertTrueHandle(HandleRef inHandleRef)
{
	if (inHandleRef == 0) return 0;

	uLONG	slot = _GetSlot();

	if (slot == 0) return 0;

	HandleBlock*	block = fSlotPtr + slot - 1;

	// L.E. 13/12/1999 to better crash on mac by wrong dereferencing
	slot = ~(slot | 0x80000000);
	
	block->fHandle = inHandleRef;
	block->fPhysicalSize = fMemMgr.GetPhysicalHandleSize(inHandleRef);
	block->fLogicalSize = block->fPhysicalSize;
	block->fAtomicSize = 0;
	block->fLockCounter = 0;

#if USE_CHECK_LEAKS
	fLeaksChecker.RegisterBlock(block->fStackCrawl);
#endif

	return (VHandle) (uLONG_PTR) slot;
}


VPtr VMemSlotMgr::LockHandle(VHandle ioHandle)
{
	HandleBlock*	block = _GetHandleBlock(ioHandle);
	if (block == 0) return 0;
	
	if (block->fLogicalSize == 0) return 0;
	
	block->fLockCounter++;
	
	return fMemMgr.LockHandle(block->fHandle, block->fLockCounter);
}


void VMemSlotMgr::UnlockHandle(VHandle ioHandle)
{
	HandleBlock*	block = _GetHandleBlock(ioHandle);

	if (block == 0)
		return;
	
	if (block->fLogicalSize == 0)
		return;
	
	if (0 == block->fLockCounter)
	{
		vThrowError(VE_HANDLE_NOT_LOCKED);
		return;
	}

	block->fLockCounter--;
	
	fMemMgr.UnlockHandle(block->fHandle, block->fLockCounter);
}


VSize VMemSlotMgr::GetFreeMemory()
{
	return fMemMgr.GetFreeMemory();
}

#endif

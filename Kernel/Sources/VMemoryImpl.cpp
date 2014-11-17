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

#define VERSIONDEBUG_CheckMem 0

#include "VMemoryImpl.h"
#include "VSystem.h"
#include "VMemoryCpp.h"
#include "VTask.h"

#include <set>
#include <map>
using namespace std;

//inline VSize AdjusteSize(VSize size) { return (size+1) & -2; };

VMemImplBlock* VMemImplAllocation::AllocateAtTheEnd(VSize inSize, VSize& ioUsedMem, Boolean isAnObject, Boolean ForceInMem, sLONG inTag)
{
	if (inSize > (fAllocationSize - fDataEnd) )
		return NULL; // on depasse la limite de l'allocation
	else
	{
		VSize prevlen = 0;
		if (fDataEnd != 0)
		{
			prevlen = ((VMemImplBlock*)GetDataStart())->GetLen();
		}
		char* p = GetDataStart() + fDataEnd;
		((VMemImplBlock*)p)->SetPreviousLen(LastLen);
		fDataEnd = fDataEnd + inSize;
		VSize plus = isAnObject ? 1 : 0;
		VSize plus2 = ForceInMem ? 2 : 0;
		((VMemImplBlock*)p)->SetLen(inSize + plus + plus2);
#if CPPMEM_CACHE_INFO
		((VMemImplBlock*)p)->SetTag(inTag);
#endif
		((VMemImplBlock*)p)->SetOwner(this);
		LastLen = inSize;
		//assert(ioUsedMem>=0);
		ioUsedMem = ioUsedMem + inSize;
		return (VMemImplBlock*)p;
	}
}


void VMemImplAllocation::Init(VSize inSize, bool inPhysicalMemory)
{
	fAllocationSize = inSize - sizeof(VMemImplAllocation);
	fPhysicalMemory = inPhysicalMemory;
	fDataEnd = 0;
	LastLen = 0;
}


Boolean VMemImplAllocation::IsLastBlock(const VMemImplBlock* inBlock) const
{
	if ( ((char*)inBlock) + inBlock->GetLen() == GetDataStart() + fDataEnd)
		return true;
	else
	{
		assert( (char*)inBlock >= GetDataStart() );
		assert(((char*)inBlock) + inBlock->GetLen() < GetDataStart() + fDataEnd);
		return false;
	}
}


Boolean VMemImplAllocation::Contains(const VMemImplBlock* inBlock) const 
{ 
	if ( GetDataStart() <= (char*)inBlock && GetDataStart()+fAllocationSize > (char*)inBlock ) 
		return true; 
	else 
		return false; 
}

void VMemImplAllocation::ReduceFrom(const VMemImplBlock* inBlock)
{
	if ((char*)inBlock >= GetDataStart())
	{
		VSize newend = (char*)inBlock - GetDataStart();
		xbox_assert(newend >= 0 && newend < fDataEnd && newend < fAllocationSize);
		LastLen = inBlock->GetPreviousLen();
		fDataEnd = newend;
	}
}




/* --------------------------------------------------- */

VPageAllocationImpl::VPageAllocationImpl(VMemThreadImpl* inOwner, VSize inSize, VSize inElemSize) 
{ 
	fOwner = inOwner;
	fAllocationSize = inSize; 
	fDataEnd = GetDataStart(); 
	fFirstFree = NULL; 
	fNbFull = 0; 
	fNbHole = 0;
	fSizeOfEachElem = (sWORD)inElemSize;
	fMaxElems = (sWORD) ((inSize - (sizeof(VPageAllocationImpl)-sizeof(void*))) / inElemSize);
	fNext = NULL;
	fPrevious = NULL;
};


void VPageAllocationImpl::Free( VMemImplSmallBlock* inBlock)
{
	VMemThreadImpl* owner = fOwner;

	//owner->Lock();
	//fMutex.Lock();
	inBlock->SetNext(fFirstFree);
	fFirstFree = inBlock;
	fNbHole++;
	assert(fNbHole<=fMaxElems);
	fNbFull--;
	assert(fNbFull>=0);
	fOwner->GetOwner()->FreeFromUsed((VSize)fSizeOfEachElem);
	if (fNbFull == 0)
	{
		fOwner->GetOwner()->AddToUsed((VSize)fSizeOfEachElem*(VSize)fMaxElems);
		fOwner->RemovePage(this);
	}
	else
	{
		if (fNbHole == 1)
		{
			fOwner->MarkPageAsNotFull(this);
		}
		//fMutex.Unlock();
	}
	//owner->Unlock();
}



void* VPageAllocationImpl::Malloc( VSize inSize, Boolean isAnObject, sLONG inTag)
{
	void* result = NULL;
	fOwner->GetOwner()->AddToUsed((VSize)fSizeOfEachElem);

	//fMutex.Lock();
	VMemImplSmallBlock* x;
	if (fFirstFree != NULL)
	{
		fNbHole--;
		assert(fNbHole>=0);
		fNbFull++;
		assert(fNbFull<=fMaxElems);
		x = fFirstFree;
		fFirstFree = x->GetNext();
		/*
		if (fFirstFree != NULL)
			fFirstFree->SetPrevious(NULL);
		*/

		// on remet le meme offset en changeant simplement le dernier bit 
		sLONG offset = x->GetOffset(); 
		offset = (-offset) & -2;
		sLONG plus = isAnObject ? 1 : 0;
		x->SetOffset(-(offset + plus));
#if CPPMEM_CACHE_INFO
		x->SetTag(inTag);
#endif
		result = (void*) (((char*)x) + VMemThreadImpl::SizeSmallHeader);
	}
	else
	{
		if (fDataEnd+inSize <= fAllocationSize)
		{
			fNbFull++;
			assert(fNbFull<=fMaxElems);
			x = (VMemImplSmallBlock*) ( ((char*)this)+fDataEnd );
			sLONG plus = isAnObject ? 1 : 0;
			x->SetOffset(-((sLONG)fDataEnd + plus));
#if CPPMEM_CACHE_INFO
			x->SetTag(inTag);
#endif
			result = (void*) (((char*)x) + VMemThreadImpl::SizeSmallHeader);
			fDataEnd = fDataEnd + inSize;
		}
	}

	if (fNbFull == fMaxElems)
	{
		fOwner->MarkPageAsFull(this);
	}

	//fMutex.Unlock();
	return result;
}


Boolean VPageAllocationImpl::IsBlockFree(VMemImplSmallBlock* inBlock)
{
	//VKernelTaskLock lock(&fMutex);
	if (testAssert( ((char*)inBlock > (char*)this) && ((char*)inBlock < ((char*)this) + fAllocationSize) ) )
	{
		Boolean found = false;
		VMemImplSmallBlock* p = fFirstFree;
		while (p != NULL)
		{
			if (p == inBlock)
			{
				found = true;
				break;
			}
			if (testAssert( ((char*)p > (char*)this) && ((char*)p < ((char*)this) + fAllocationSize) ) )
			{
				p = p->GetNext();
			}
			else
			{
				found = true;
				break;
			}
		}

		return found;
	}
	else
		return true;
}


Boolean VPageAllocationImpl::Check(void* skipthisone)
{
	Boolean res = true;
	//fMutex.Lock();

	res = res && testAssert(fNbFull>=0 && fNbFull<=fMaxElems);
	res = res && testAssert(fNbHole>=0 && fNbHole<=fMaxElems);

	if (fNbHole == 0)
		res = res && testAssert(fFirstFree == NULL);
	
	sLONG counthole = 0;
	VMemImplSmallBlock* p = fFirstFree;
	while (p != NULL)
	{
		counthole++;
		if (testAssert( ((char*)p > (char*)this) && ((char*)p < ((char*)this) + fAllocationSize) ) )
		{
			p = p->GetNext();
		}
		else
			res = false;
	}

	res = res && testAssert(counthole == fNbHole);

	VMemImplSmallBlock* pstart = (VMemImplSmallBlock*)((char*)&fDataStart);
	VMemImplSmallBlock* pageEnd = (VMemImplSmallBlock*)(((char*)&fDataStart) + (fDataEnd - (sizeof(VPageAllocationImpl)-sizeof(void*))) );
	VMemImplSmallBlock* pblock;

	std::set<VMemImplSmallBlock*> freeOnes;

	pblock = fFirstFree;
	while (pblock != NULL)
	{
		freeOnes.insert(pblock);
		pblock = pblock->GetNext();
	}

	pblock = pstart;
	while (pblock < pageEnd)
	{
		sLONG offset = pblock->GetOffset();
		Boolean isanobject = (((-offset) & 1) == 1);
		offset = (-offset) & -2;
		res = res && testAssert( offset == ( ((char*)pblock) - ((char*)this) ) );

#if WITH_ASSERT
		if (isanobject && freeOnes.find(pblock) == freeOnes.end())
		{
			VObject* p2 = (VObject*)(((char*)pblock) + VMemThreadImpl::SizeSmallHeader);
			if ( (p2 != skipthisone) && (*(void**)p2 != NULL) )
			{
				try
				{
					const type_info* typinf = &(typeid(*p2));
					assert(typinf != NULL);
				}
				catch (...)
				{
					sLONG debugplus = 1;
				}
			}
		}
#endif

		pblock = (VMemImplSmallBlock*) ( ((char*)pblock)+fSizeOfEachElem );
	}

	//fMutex.Unlock();
	return res;
}


void VPageAllocationImpl::GetStats(VMemStats& stats, bool& isfull)
{
	VMemImplSmallBlock* pstart = (VMemImplSmallBlock*)((char*)&fDataStart);
	VMemImplSmallBlock* pageEnd = (VMemImplSmallBlock*)(((char*)&fDataStart) + (fDataEnd - (sizeof(VPageAllocationImpl)-sizeof(void*))) );

	VMemImplSmallBlock* pblock;
	std::set<VMemImplSmallBlock*> freeOnes;

	stats.fUsedMem = stats.fUsedMem - (fSizeOfEachElem * fMaxElems);
	stats.fFreeMem = stats.fFreeMem + (fSizeOfEachElem * fMaxElems);

	if (fNbFull == 0)
	{
		// ne devrait pas arriver
		sWORD a = fNbFull; // put a break here
	}

	isfull = (fNbFull == fMaxElems);
	stats.fSmallBlockInfo[fSizeOfEachElem].fFreeCount += (fMaxElems-fNbFull);
	stats.fSmallBlockInfo[fSizeOfEachElem].fUsedCount += fNbFull;

	pblock = fFirstFree;
	while (pblock != NULL)
	{
		freeOnes.insert(pblock);
		pblock = pblock->GetNext();
	}
	
	pblock = pstart;
	while (pblock < pageEnd)
	{
		stats.fNbSmallBlocks++;

		sLONG offset = pblock->GetOffset();
		Boolean isanobject = (((-offset) & 1) == 1);
		offset = (-offset) & -2;
		assert( offset == ( ((char*)pblock) - ((char*)this) ) );
		
		if (freeOnes.find(pblock) != freeOnes.end())
		{
			// le block est libre
			stats.fNbSmallBlocksFree++;

			if ((VSize)fSizeOfEachElem > stats.fBiggestBlockFree)
				stats.fBiggestBlockFree = fSizeOfEachElem;
		}
		else
		{
			stats.fFreeMem = stats.fFreeMem - fSizeOfEachElem;
			stats.fUsedMem = stats.fUsedMem + fSizeOfEachElem;

			stats.fNbSmallBlocksUsed++;
			if (isanobject)
			{
				stats.fNbObjects++;
#if CPPMEM_CACHE_INFO
				VObject* p = (VObject*)(((char*)pblock) + VMemThreadImpl::SizeSmallHeader);
				try
				{
					if ( *(void**) p == NULL)
					{
						sLONG tag = pblock->GetTag();
						VObjectInfo* info = &(stats.fBlockInfo[tag]);
						info->fCount++;
						info->fTotalSize += fSizeOfEachElem;
					}
					else
					{
						const type_info* typinf = &(typeid(*p));
						//Cast pour éviter le C4267
						sLONG xlen = (sLONG) strlen(typinf->name());
						VObjectInfo* info = &(stats.fObjectInfo[typinf]);
						info->fCount++;
						info->fTotalSize = info->fTotalSize + fSizeOfEachElem;
						stats.fTypeInfoByVtbl[*(void**)p] = typinf; 
					}
				}
				catch (...)
				{
					sLONG debugplus = 1;
					sLONG tag = pblock->GetTag();
					VObjectInfo* info = &(stats.fBlockInfo[tag]);
					info->fCount++;
					info->fTotalSize = info->fTotalSize + fSizeOfEachElem;
				}
#endif
			}
#if CPPMEM_CACHE_INFO
			else
			{
				sLONG tag = pblock->GetTag();
				VObjectInfo* info = &(stats.fBlockInfo[tag]);
				info->fCount++;
				info->fTotalSize = info->fTotalSize + fSizeOfEachElem;
			}
#endif
		}
		pblock = (VMemImplSmallBlock*) ( ((char*)pblock)+fSizeOfEachElem );
	}
}


/* --------------------------------------------------- */

VMemThreadImpl::VMemThreadImpl()
{
	sLONG i;
	for (i = 0; i < kTotalStepAllocPagesInThread; i++)
	{
		//fPages[i] = NULL;
		fNotFullPages[i] = NULL;
	}
}


void VMemThreadImpl::MarkPageAsNotFull(VPageAllocationImpl* ioPage)
{
	//fMutex.Lock();

	sLONG step = GetStepFromSize(ioPage->GetElemSize() - SizeSmallHeader, NULL);
	//if (fPages[step] != ioPage)
	if (fNotFullPages[step] != ioPage && ioPage->GetNext() == NULL && ioPage->GetPrevious() == NULL)
	{
		VPageAllocationImpl* ff = fNotFullPages[step];
		ioPage->SetPrevious(NULL);
		ioPage->SetNext(ff);
		if (ff != NULL)
			ff->SetPrevious(ioPage);
		fNotFullPages[step] = ioPage;
	}
	//fMutex.Unlock();
}


void VMemThreadImpl::RemovePage(VPageAllocationImpl* ioPage)
{
	//fMutex.Lock();
	//ioPage->Unlock();

	sLONG step = GetStepFromSize(ioPage->GetElemSize() - SizeSmallHeader, NULL);
	//if (fPages[step] != ioPage)
	{
		VPageAllocationImpl* ff = fNotFullPages[step];
		VPageAllocationImpl* next = ioPage->GetNext();
		VPageAllocationImpl* prev = ioPage->GetPrevious();
		
		if (next != NULL)
			next->SetPrevious(prev);
		if (prev != NULL)
			prev->SetNext(next);
		if (ff == ioPage)
		{
			fNotFullPages[step] = next;
		}

		GetOwner()->Free((void*)ioPage);
	}
	//fMutex.Unlock();
}


void VMemThreadImpl::MarkPageAsFull (VPageAllocationImpl* ioPage)
{
	//fMutex.Lock();

	sLONG step = GetStepFromSize(ioPage->GetElemSize() - SizeSmallHeader, NULL);
	{
		VPageAllocationImpl* ff = fNotFullPages[step];
		VPageAllocationImpl* next = ioPage->GetNext();
		VPageAllocationImpl* prev = ioPage->GetPrevious();

		if (next != NULL)
			next->SetPrevious(prev);
		if (prev != NULL)
			prev->SetNext(next);
		if (ff == ioPage)
		{
			fNotFullPages[step] = next;
		}

		ioPage->SetPrevious(NULL);
		ioPage->SetNext(NULL);
	}
	//fMutex.Unlock();
}


void* VMemThreadImpl::Malloc( VSize inSize, Boolean isAnObject, sLONG inTag)
{
	void* result = NULL;
	sLONG stepinc;
	sLONG step = GetStepFromSize(inSize, &stepinc);
	inSize = ((inSize + (VSize)(stepinc - 1)) & (VSize)(-stepinc)) /*+ SizeSmallHeader*/;
	
	//fMutex.Lock();

#if VERSIONDEBUG_CheckMem
	Check();
#endif

	VPageAllocationImpl* page;
	/*
	page = fPages[step];
	if (page != NULL)
	{
		result = page->Malloc(inSize, isAnObject);
	}
	if (result == NULL)
	*/
	{
		page = fNotFullPages[step];
		//fPages[step] = page;


		if (page != NULL)
		{
			/*
			VPageAllocationImpl* next = page->GetNext();
			assert(page->GetPrevious() == NULL);
			if (next != NULL)
				next->SetPrevious(NULL);
			fNotFullPages[step] = next;
			*/
			result = page->Malloc(inSize, isAnObject, inTag);
			assert(result != NULL);
		}
	}
	if (result == NULL)
	{
		sLONG sizepage;
		if (inSize < 256)
			sizepage = 8192 - VMemCppImpl::SizeHeader;
		else if (inSize < 1024)
			sizepage = 16384 - VMemCppImpl::SizeHeader;
		else if (inSize < 4096)
			sizepage = 65536 - VMemCppImpl::SizeHeader;
		else sizepage = 131072 - VMemCppImpl::SizeHeader;
		page = (VPageAllocationImpl*) fOwner->Malloc((VSize)sizepage, true, isAnObject, 0);
		if (page != NULL)
		{
			page = new ((void*)page) VPageAllocationImpl(this, (VSize)sizepage, inSize);
			fOwner->FreeFromUsed(page->GetElemSize() * page->GetMaxElems());
			//page->Init(this, (VSize)sizepage, inSize);
			//fPages[step] = page;
			MarkPageAsNotFull(page);
			result = page->Malloc(inSize, isAnObject, inTag);
		}
	}

#if VERSIONDEBUG_CheckMem
	Check(result);
#endif
	//fMutex.Unlock();

	return result;
}


sLONG VMemThreadImpl::GetStepFromSize(VSize inSize, sLONG* outStepInc)
{
	sLONG res;

	if (inSize <= kFirstStepAlloc)
	{
		if (outStepInc != NULL)
			*outStepInc = kFirstStepAllocInc;
		res = (sLONG) ((inSize+kFirstStepAllocInc-1) / kFirstStepAllocInc);
	}

	else if (inSize <= kSecondStepAlloc)
	{
		if (outStepInc != NULL)
			*outStepInc = kSecondStepAllocInc;
		res = (sLONG) ((inSize+kSecondStepAllocInc-1-kFirstStepAlloc) / kSecondStepAllocInc + stage2);
	}

	else
	{
		if (outStepInc != NULL)
			*outStepInc = kThirdStepAllocInc;
		res = (sLONG) ((inSize+kThirdStepAllocInc-1-kSecondStepAlloc) / kThirdStepAllocInc + stage3);
	}


	return res;
}


Boolean VMemThreadImpl::Check(void* skipthisone)
{
	Boolean res = true;
	sLONG i,j;

	//fMutex.Lock();
	for (i = 0; i < kTotalStepAllocPagesInThread; i++)
	{
		VPageAllocationImpl* page;
		/*
		page = fPages[i];
		if (page != NULL)
		{
			assert(page->GetPrevious() == NULL);

			res = res && page->Check();
			if (page == fNotFullPages[i])
			{
				res = false;
				assert(false);
			}
			else
			{
				for (j = i+1; j < kTotalStepAllocPagesInThread; j++)
				{
					if ( (page == fPages[j]) || (page == fNotFullPages[j]) )
					{
						res = false;
						assert(false);
					}
				}
			}
		}
		*/

		page = fNotFullPages[i];
		if (page != NULL)
		{
			xbox_assert(page->GetPrevious() == NULL);

			VPageAllocationImpl* page2 = page;
			while (page2 != NULL)
			{
				res = res && page2->Check(skipthisone);
				page2 = page2->GetNext();
			}

			//res = res && page->check();
			for (j = i+1; j < kTotalStepAllocPagesInThread; j++)
			{
				if (page == fNotFullPages[j])	// || ( /*(page == fPages[j]) 
				{
					res = false;
					xbox_assert(false);
				}
			}
		}
	}

	//fMutex.Unlock();
	return res;
}



/* --------------------------------------------------- */

VMemCppImpl::VMemCppImpl(sLONG inBlockNumber)
{
	fCanAutoAddAllocation = false;
	fAllocationBlockNumber = inBlockNumber;
	fOwner = NULL;
	fFirstAllocation = NULL;
	fTotalAllocation = 0;
	fUsedMem = 0;
	fLastBlockToAllocateFrom = NULL;

	sLONG i;
	for (i = 0; i < kTotalStepAllocPagesInMain; i++)
		fFirstBlocks[i] = NULL;

	sLONG k;
	VMemThreadImpl* th = fThreads;
	for (k = 0; k < MaxThreads; k++)
	{
		th->SetOwner(this);
		th++;
	}

#if VERSIONDEBUG
	fDebugNbCall = 0;
#endif

}


VMemCppImpl::~VMemCppImpl()
{
}


sLONG VMemCppImpl::GetStepFromSize(VSize inSize, sLONG* outStepInc)
{
	sLONG res;

	if (inSize <= kFourthStepAlloc)
	{
		if (outStepInc != NULL)
			*outStepInc = kFourthStepAllocInc;
		res = (sLONG) ((inSize+kFourthStepAllocInc-1) / kFourthStepAllocInc + stage4);
	}

	else // les objets plus grands ou egaux ‡ 1M sont traites specialement
	{
		if (outStepInc != NULL)
			*outStepInc = 0;
		res = stage5;
	}

	return res;
}


sLONG VMemCppImpl::GetExactStepFromSize(VSize inSize)
{
	sLONG res;

	if (inSize <= kFourthStepAlloc)
	{
		res = (sLONG)(inSize / kFourthStepAllocInc + stage4);
	}

	else // les objets plus grands ou egaux ‡ 1M sont traites specialement
	{
		res = stage5;
	}

	return res;
}


void VMemCppImpl::Init( VCppMemMgr *inOwner)
{
	fOwner = inOwner;
}


Boolean	VMemCppImpl::AddAllocation( VSize inSize, const void *inHintAddress, Boolean	inPhysicalMem)
{
	//VKernelTaskLock lock(&fGlobalMemMutext);
	Boolean ok = false;

	//fMutex.Lock();

	if (inSize > 65535)
	{
		void* buf;
		bool locked = false;
		if (inPhysicalMem)
			buf = VSystem::VirtualAllocPhysicalMemory( inSize, inHintAddress, &locked);
		else
			buf = VSystem::VirtualAlloc( inSize, inHintAddress);

		if (buf != NULL)
		{
			VMemImplAllocation* alloue = (VMemImplAllocation*)buf;
			alloue->SetPrevious(NULL);
			alloue->SetNext(fFirstAllocation);
			alloue->SetOwner(this);
			if (fFirstAllocation != NULL)
				fFirstAllocation->SetPrevious(alloue);
			fFirstAllocation = alloue;
			alloue->Init(inSize, locked);
			ok = true;
		}
	}

	if (ok)
	{
		fTotalAllocation = fTotalAllocation + inSize;
		fOwner->IncMemAllocatedCount(inSize);
	}

	//fMutex.Unlock();

	return ok;
}


void VMemCppImpl::AddAlreadyAllocatedMem(void* allocatedMem, VSize memSize, bool locked)
{
	VMemImplAllocation* alloue = (VMemImplAllocation*)allocatedMem;
	alloue->SetPrevious(NULL);
	alloue->SetNext(fFirstAllocation);
	alloue->SetOwner(this);
	if (fFirstAllocation != NULL)
		fFirstAllocation->SetPrevious(alloue);
	fFirstAllocation = alloue;
	alloue->Init(memSize, locked);
	fTotalAllocation = fTotalAllocation + memSize;
	fOwner->IncMemAllocatedCount(memSize);
}


VIndex VMemCppImpl::CountAllocations()
{
	//VKernelTaskLock lock(&fGlobalMemMutext);
	VIndex count = 0;
	
	//fMutex.Lock();

	for( VMemImplAllocation *allocation = fFirstAllocation ; allocation != NULL ; allocation = allocation->GetNext())
		++count;

	//fMutex.Unlock();
	
	return count;
}


void VMemCppImpl::RemoveBlockFromChainList(const VMemImplBlock* inBlock, VMemImplBlock** inFirst)
{
	VMemImplBlock* prev = inBlock->GetPrevious();
	VMemImplBlock* next = inBlock->GetNext();
	if (inBlock == *inFirst)
	{
		xbox_assert(prev == NULL);
		*inFirst = next;
	}
	else
	{
		xbox_assert(prev != NULL);
		prev->SetNext(next);
	}

	if (next != NULL)
		next->SetPrevious(prev);
}



void* VMemCppImpl::Malloc( VSize inSize, Boolean inForceinMain, Boolean isAnObject, sLONG inTag)
{
	//VKernelTaskLock lock(&fGlobalMemMutext);
#if VERSIONDEBUG
	fDebugNbCall++;
#endif
	char* result = NULL;

	if (inSize < kThirdStepAlloc && !inForceinMain)
	{
		//inSize = AdjusteSize(inSize);
		sLONG curthread = VTask::GetCurrentID() % MaxThreads;
		VMemThreadImpl* th = &(fThreads[curthread]);
		result = (char*)(th->Malloc(inSize + VMemThreadImpl::SizeSmallHeader, isAnObject, inTag));
	}
	else
	{

		//fMutex.Lock();

	#if VERSIONDEBUG
		//sLONG oldusedmem = fUsedMem;
		//sLONG oldsize = inSize;
	#endif

		if (inSize < (2147483647 - 2048))
		{
			inSize = inSize + SizeHeader;
			sLONG stepinc;
			sLONG step = GetStepFromSize(inSize, &stepinc);

			if (stepinc == 0)
				inSize = (inSize + 7) & (VSize)(-8);
			else
				inSize = (inSize + stepinc - 1) & (VSize)(-stepinc);
			
			VMemImplBlock* *p = &fFirstBlocks[step];

			sLONG pass;

			for (pass = 1; pass <= 2; pass++)
			{
				if (pass == 2 || p == NULL || *p == NULL) // on a pas trouve de block libre de la bonne taille
				{     // on va chercher a en alloue un
					VMemImplBlock* x = NULL;
					VMemImplAllocation* a = fFirstAllocation;
					while (a != NULL && x == NULL)
					{
						x = a->AllocateAtTheEnd(inSize, fUsedMem, isAnObject, inForceinMain, inTag);
						a = a->GetNext();
					}
					if (x != NULL)
					{
						result = ((char*)x) + SizeHeader;
					}
					else
					{
						p = NULL;
						VMemImplBlock* goodone = NULL;
						// comme on a pas pu alloue, alors on va cherche a splitter un gros block
						if (fLastBlockToAllocateFrom != NULL && fLastBlockToAllocateFrom->GetLen() >= inSize)
						{
							sLONG step3 = GetExactStepFromSize(fLastBlockToAllocateFrom->GetLen());
							p = &fFirstBlocks[step3];
							xbox_assert(*p != NULL);
							goodone = fLastBlockToAllocateFrom;
						}
						else
						{
							sLONG i = stage5;
							while (i>step) // on va parcourir le tableau des liste chainee de blocks libres pour trouver une entree non nulle
							{
								p = &fFirstBlocks[i];
								if (*p != NULL)
								{
									break;
								}
								i--;
							}
							if (p != NULL)
								goodone = *p;
						}
						if (goodone != NULL)
						{
							VMemImplBlock* remainblock = NULL;
							RemoveBlockFromChainList(goodone, p);
							VSize remainsize = goodone->GetLen() - inSize;
							if (remainsize >= kMinRemainingSizeBlock)
							{
								VMemImplBlock* next = (VMemImplBlock*) ( ((char*)goodone) + goodone->GetLen() );
								remainblock = (VMemImplBlock*) ( ((char*)goodone) + inSize );
								next->SetPreviousLen(remainsize);
								//remainblock->SetLen(-remainsize);
								remainblock->SetAsFree(remainsize);
								remainblock->SetPreviousLen(inSize);
								remainblock->SetOwner(goodone->GetOwner());
								VSize plus = isAnObject ? 1 : 0;
								VSize plus2 = inForceinMain ? 2 : 0;
								goodone->SetLen(inSize + plus + plus2);

								sLONG step2 = GetExactStepFromSize(remainsize);
								VMemImplBlock* first = fFirstBlocks[step2];
								remainblock->SetNext(first);
								if (first != NULL)
									first->SetPrevious(remainblock);
								fFirstBlocks[step2] = remainblock;
								remainblock->SetPrevious(NULL);
							}

							if (goodone == fLastBlockToAllocateFrom)
							{
								fLastBlockToAllocateFrom = remainblock;
							}
							//xbox_assert(fUsedMem >= 0);
							fUsedMem = fUsedMem + goodone->GetLen();
							VSize plus = isAnObject ? 1 : 0;
							VSize plus2 = inForceinMain ? 2 : 0;
							goodone->SetLen(goodone->GetLen() + plus + plus2); // on la remet en positif, pour montrer que le block est occupe
	#if CPPMEM_CACHE_INFO
							goodone->SetTag(inTag);
	#endif
							result = ((char*)goodone) + SizeHeader;
						}
					}
				}
				else // on sait qu'il exite des block libre de la bonne taille
				{
					if (stepinc == 0) // on est dans le cas de gros blocks >= 1M
					{
						VMemImplBlock* x = *p;
						VMemImplBlock* goodone = NULL;
						while (x != NULL)
						{
							if (x->GetLen() >= inSize)
							{
								if (x->GetLen() == inSize)
								{
									goodone = x;
									break;
								}
								else
								{
									if (goodone == NULL)
										goodone = x;
									else
									{
										if (goodone->GetLen() > x->GetLen())
											goodone = x;
									}
								}
							}
							x = x->GetNext();
						}
						if (goodone != NULL)
						{
							VMemImplBlock* remainblock = NULL;
							RemoveBlockFromChainList(goodone, p);
							VSize remainsize = goodone->GetLen() - inSize;
							if (remainsize >= kMinRemainingSizeBlock)
							{
								VMemImplBlock* next = (VMemImplBlock*) ( ((char*)goodone) + goodone->GetLen() );
								remainblock = (VMemImplBlock*) ( ((char*)goodone) + inSize );
								next->SetPreviousLen(remainsize);
								//remainblock->SetLen(-remainsize);
								remainblock->SetAsFree(remainsize);
								remainblock->SetPreviousLen(inSize);
								remainblock->SetOwner(goodone->GetOwner());
								VSize plus = isAnObject ? 1 : 0;
								VSize plus2 = inForceinMain ? 2 : 0;
								goodone->SetLen(inSize + plus + plus2);

								sLONG step2 = GetExactStepFromSize(remainsize);
								VMemImplBlock* first = fFirstBlocks[step2];
								remainblock->SetNext(first);
								if (first != NULL)
									first->SetPrevious(remainblock);
								fFirstBlocks[step2] = remainblock;
								remainblock->SetPrevious(NULL);
							}

							if (goodone == fLastBlockToAllocateFrom)
							{
								fLastBlockToAllocateFrom = remainblock;
							}
							xbox_assert(fUsedMem >= 0);
							fUsedMem = fUsedMem + goodone->GetLen();
							VSize plus = isAnObject ? 1 : 0;
							VSize plus2 = inForceinMain ? 2 : 0;
							goodone->SetLen(goodone->GetLen() + plus + plus2); // on la remet en positif, pour montrer que le block est occupe
	#if CPPMEM_CACHE_INFO
							goodone->SetTag(inTag);
	#endif
							result = ((char*)goodone) + SizeHeader;
						}
					}
					else // on est dans la cas ou la taille est exacte (ou presque)
					{
						VMemImplBlock* x = *p;
						xbox_assert(x->GetPrevious() == NULL);
						*p = x->GetNext();
						if (*p != NULL)
							(*p)->SetPrevious(NULL);
						result = ((char*)x) + SizeHeader;
						xbox_assert(x->GetLen() >= inSize); // on peut trouver un block un peu plus grand qui resulte du merge de deux ou trois plus petits
						VSize plus = isAnObject ? 1 : 0;
						VSize plus2 = inForceinMain ? 2 : 0;
						x->SetLen(x->GetLen() + plus + plus2); // on la remet en positif, pour montrer que le block est occupe
	#if CPPMEM_CACHE_INFO
						x->SetTag(inTag);
	#endif
						xbox_assert(fUsedMem >= 0);
						fUsedMem = fUsedMem + x->GetLen();
						if (x == fLastBlockToAllocateFrom)
							fLastBlockToAllocateFrom = NULL;
					}
				}

				if (result != NULL)
					break;

			} // end du for pass

			if (result == NULL) // si on a vraiment pas pu trouve allors on cherche a alloue une nouvelle page d'allocation
			{
				if (fCanAutoAddAllocation)
				{
					VSize bigsize = kDefaultMemAllocationSize;
					if (bigsize < inSize + (VSize) sizeof(VMemImplAllocation)+1024)
						bigsize = inSize + (VSize) sizeof(VMemImplAllocation)+1024;

					if (AddAllocation(bigsize, NULL, false))
					{
						VMemImplBlock*	x = fFirstAllocation->AllocateAtTheEnd(inSize, fUsedMem, isAnObject, inForceinMain, inTag);
						result = ((char*)x) + SizeHeader;
					}
				}
			}
			
			//assert(fUsedMem>0);
			//assert(fUsedMem >= oldusedmem + oldsize);
		}

		//fMutex.Unlock();
	}

#if 0 && VERSIONDEBUG
	if (fGlobalMemMutext.GetUseCount() == 1)
	{
		VMemStats stats;
		GetStats(stats);
		if (stats.fUsedMem != fUsedMem)
		{
			stats.fUsedMem = stats.fUsedMem;
			// break here;
		}
	}
#endif

	return (void*)result;
}


VSize VMemCppImpl::GetPtrSize( const void *inBlock)
{
	//VKernelTaskLock lock(&fGlobalMemMutext);
	if(!testAssert(inBlock != NULL))
		return (VSize)0; 
			
	VMemImplBlock *x = (VMemImplBlock*) ( ((char*)inBlock) - SizeHeader );
	if (x->IsASmallBlock())
	{
		// si previouslen est negatif, alors en fait le block est un small block et previouslen correspond a offset qui est toujours negatif
		VMemImplSmallBlock *xsmall = (VMemImplSmallBlock*) ( ((char*)inBlock) - VMemThreadImpl::SizeSmallHeader );
		sLONG offset = xsmall->GetOffset();
		offset = (-offset) & -2;
		VPageAllocationImpl* page = (VPageAllocationImpl*) (((char*)xsmall)-offset);
		return page->GetElemSize() - VMemThreadImpl::SizeSmallHeader;
	}
	else
	{
		return x->GetLen() - SizeHeader;
	}
}


sLONG VMemCppImpl::GetAllocationBlockNumber(void *inBlock)
{
	assert(inBlock != NULL);
	VMemImplBlock *x = (VMemImplBlock*) ( ((char*)inBlock) - SizeHeader );
	if (x->IsASmallBlock())
	{
		// si previouslen est negatif, alors en fait le block est un small block et previouslen correspond a offset qui est toujours negatif
		VMemImplSmallBlock *xsmall = (VMemImplSmallBlock*) ( ((char*)inBlock) - VMemThreadImpl::SizeSmallHeader );
		sLONG offset = xsmall->GetOffset();
		offset = (-offset) & -2;
		VPageAllocationImpl* page = (VPageAllocationImpl*) (((char*)xsmall)-offset);
		return page->GetOwner()->GetOwner()->fAllocationBlockNumber;
	}
	else
	{
		VMemImplAllocation* res = x->GetOwner();
		return res->GetOwner()->fAllocationBlockNumber;
	}

}

void VMemCppImpl::MergeContiguousFreeBlock(VMemImplBlock** ioBlockBeingFreed, VMemImplBlock* iFreeBlockToMerge)
{
	VSize mergedSize = (*ioBlockBeingFreed)->GetLen() + iFreeBlockToMerge->GetLen();

	// DH 18-Apr-2013 ACI0080562/ACI0081239 Fix crashes with allocation zones bigger than 2GB (for example when cache is bigger than 8GB)
	// The block size is stored in a sLONG so we don't merge blocks if their total size can't fit in a sLONG
	if (mergedSize > kMAX_sLONG)
	{
		return;
	}

	// Remove the block to merge from the chain list of free blocks
	sLONG step = GetExactStepFromSize(iFreeBlockToMerge->GetLen());
	RemoveBlockFromChainList(iFreeBlockToMerge, &fFirstBlocks[step]);

	// The resulting block starts at the lowest address
	if (iFreeBlockToMerge < *ioBlockBeingFreed)
	{
		*ioBlockBeingFreed = iFreeBlockToMerge;
	}

	// Store the new block length
	(*ioBlockBeingFreed)->SetAsFree(mergedSize);

	// Update the following block
	VMemImplBlock* nextBlock = (*ioBlockBeingFreed)->GetNextInAllocation();
	if (nextBlock)
	{
		nextBlock->SetPreviousLen(mergedSize);
	}
}

void VMemCppImpl::Free( void *inBlock)
{
	//VKernelTaskLock lock(&fGlobalMemMutext);
#if VERSIONDEBUG
	fDebugNbCall++;
#endif
	assert(inBlock != NULL);
	VMemImplBlock *x = (VMemImplBlock*) ( ((char*)inBlock) - SizeHeader );
	if (x->IsASmallBlock())
	{
		// si previouslen est negatif, alors en fait le block est un small block et previouslen correspond a offset qui est toujours negatif
		VMemImplSmallBlock *xsmall = (VMemImplSmallBlock*) ( ((char*)inBlock) - VMemThreadImpl::SizeSmallHeader );
		sLONG offset = xsmall->GetOffset();
		offset = (-offset) & -2;
		VPageAllocationImpl* page = (VPageAllocationImpl*) (((char*)xsmall)-offset);
		if (page->GetOwner()->GetOwner() != this)
		{
			page->GetOwner()->GetOwner()->Free(inBlock);
		}
		else
		{
#if VERSIONDEBUG_CheckMem
			VMemThreadImpl* th = page->GetOwner();
			th->Check(inBlock);
#endif
			page->Free(xsmall);
#if VERSIONDEBUG_CheckMem
			th->Check();
			//put a break point here
			offset++;
			offset--;
#endif
		}
	}
	else
	{
		VMemImplAllocation* res = x->GetOwner();
		if (testAssert(res != NULL)) // on devrait trouver la page d'allocation qui contient le ptr
		{		
			if (res->GetOwner() != this)
			{
				res->GetOwner()->Free(inBlock);
			}
			else
			{
				//fMutex.Lock();

#if VERSIONDEBUG
				//sLONG oldusedmem = fUsedMem;
				//sLONG oldsize = GetPtrSize(inBlock);
#endif

				sLONG step;
				VMemImplBlock* prev = x->GetPreviousInAllocation();
				VMemImplBlock* next = x->GetNextInAllocation();

				// Update used memory
				VSize len = x->GetLen();
				assert(len <= fUsedMem);
				fUsedMem = fUsedMem - len;

				// If we have a previous block and it's free, merge it into the one being freed
				if (prev)
				{
					assert(prev->GetLen() == x->GetPreviousLen());
					if (prev->IsFree())
					{
						MergeContiguousFreeBlock(&x, prev);
					}
				}

				// If we have a next block then we will leave a free block
				if (next)
				{
					assert(next->GetPreviousLen() == x->GetLen());

					// If the next block is free, merge it into the one being freed
					if (next->IsFree())
					{
						MergeContiguousFreeBlock(&x, next);
					}

					// The length may have changed after merging
					len = x->GetLen();
					x->SetAsFree(len);

					// Insert the newly freed block in the chained list of free blocks
					step = GetExactStepFromSize(len);
					VMemImplBlock* first = fFirstBlocks[step];
					x->SetNext(first);
					if (first != NULL)
						first->SetPrevious(x);
					fFirstBlocks[step] = x;
					x->SetPrevious(NULL);
				}
				else // We never leave a free block at the end, we reduce the allocation size instead
				{
					// We may not have been able to merge with previous free block(s) due to block size limitations
					// so search for them to reduce the allocation to the end of the last non-free block
					while ((prev = x->GetPreviousInAllocation()) != NULL && prev->IsFree())
					{
						x = prev;

						// Remove the block to reduce from the chain list of free blocks
						sLONG stepPrev = GetExactStepFromSize(prev->GetLen());
						RemoveBlockFromChainList(prev, &fFirstBlocks[stepPrev]);
					}

					// Reduce the allocation
					res->ReduceFrom(x);

					// Release the allocation if it's empty and it's automaticaly added
					if (res->IsEmpty() && fCanAutoAddAllocation)
					{
						VMemImplAllocation* prev2 = res->GetPrevious();
						VMemImplAllocation* next2 = res->GetNext();
						if (prev2 == NULL)
						{
							assert(fFirstAllocation == res);
							fFirstAllocation->SetNext(next2);
							if (next2 != NULL)
								next2->SetPrevious(NULL);
							fFirstAllocation = next2;
						}
						else
						{
							prev2->SetNext(next2);
							if (next2 != NULL)
								next2->SetPrevious(prev2);
						}
						VSystem::VirtualFree(res, res->GetSystemSize(), res->IsPhysicalMemory());
					}
				}

				//assert(fUsedMem>0);
				//assert(fUsedMem <= oldusedmem - oldsize);

				//fMutex.Unlock();
			}
		}
	}
#if 0 && VERSIONDEBUG
	if (fGlobalMemMutext.GetUseCount() == 1)
	{
		VMemStats stats;
		GetStats(stats);
		if (stats.fUsedMem != fUsedMem)
		{
			stats.fUsedMem = stats.fUsedMem;
			// break here;
		}
	}
#endif
}

void* VMemCppImpl::Realloc( void *inData, VSize inNewSize)
{
	sLONG oldTag = 0;
#if CPPMEM_CACHE_INFO
	VMemImplBlock *x = (VMemImplBlock*) ( ((char*)inData) - SizeHeader );
	if (!x->IsASmallBlock())
	{
		oldTag = x->GetTag();
	}
	else
	{
		// si previouslen est negatif, alors en fait le block est un small block et previouslen correspond a offset qui est toujours negatif
		VMemImplSmallBlock *xsmall = (VMemImplSmallBlock*) ( ((char*)inData) - VMemThreadImpl::SizeSmallHeader );
		oldTag = xsmall->GetTag();
	}
#endif

	void* newdata = Malloc(inNewSize, false, false, oldTag);
	if (newdata != NULL)
	{
		VSize size = GetPtrSize(inData);
		if (size > inNewSize)
			size = inNewSize;
		CopyBlock(inData, newdata, size);
		Free(inData);
	}
	return newdata;
}


Boolean VMemCppImpl::CheckPtr(const void* inBlock)
{
	//VKernelTaskLock lock(&fGlobalMemMutext);
	assert(inBlock != NULL);
	VMemImplBlock *x = (VMemImplBlock*) ( ((char*)inBlock) - SizeHeader );
	if (x->IsASmallBlock())
	{
		// si previouslen est negatif, alors en fait le block est un small block et previouslen correspond a offset qui est toujours negatif
		VMemImplSmallBlock *xsmall = (VMemImplSmallBlock*) ( ((char*)inBlock) - VMemThreadImpl::SizeSmallHeader );
		sLONG offset = xsmall->GetOffset();
		offset = (-offset) & -2;
		VPageAllocationImpl* page = (VPageAllocationImpl*) (((char*)xsmall)-offset);
		return !page->IsBlockFree(xsmall);
	}
	else
	{
		return ! x->IsFree();
	}
	return true;
}



Boolean VMemCppImpl::Check(void* skipthisone)
{
	//VKernelTaskLock lock(&fGlobalMemMutext);
	Boolean res = true;

	if (false)
	{
		sLONG i;
		for (i = 0; i < MaxThreads; i++)
		{
			res = res && fThreads[i].Check(skipthisone);
		}
	}

	VMemImplAllocation* pAlloc = fFirstAllocation;
	while (pAlloc != NULL)
	{
		VMemImplBlock* pblock = (VMemImplBlock*)pAlloc->GetDataStart();
		VMemImplBlock* allocEnd = (VMemImplBlock*)pAlloc->GetDataEnd();
		VSize previouslen = 0;
		Boolean previouswasfree = false;
		while (pblock < allocEnd)
		{
			VSize len = pblock->GetLen();
			assert(pblock->GetOwner() == pAlloc);
			assert(pblock->GetPreviousLen() == previouslen);

			// look for it in free list
			sLONG step = GetExactStepFromSize(pblock->GetLen());
			VMemImplBlock *blockInFreeList = fFirstBlocks[step];
			for(  ; (blockInFreeList != NULL) && (blockInFreeList != pblock) ; blockInFreeList = blockInFreeList->GetNext())
				;

			if (pblock->IsFree())
			{
				assert(!previouswasfree);
				previouswasfree = true;

				xbox_assert( blockInFreeList == pblock);
			}
			else
			{
				if (pblock->IsAPageAllocation())
				{
					VPageAllocationImpl* page = (VPageAllocationImpl*)(((char*)pblock) + SizeHeader);
					res = res && page->Check(skipthisone);
				}

				previouswasfree = false;
				xbox_assert( blockInFreeList == NULL);
			}

			previouslen = len;
			pblock = (VMemImplBlock*)((char*)pblock + pblock->GetLen());
		}

		pAlloc = pAlloc->GetNext();
	}

	return res;
}


void VMemCppImpl::GetMemUsageInfo(VSize& outTotalMem, VSize& outUsedMem)
{
	//VKernelTaskLock lock(&fGlobalMemMutext);
	outTotalMem = fTotalAllocation;
	outUsedMem = fUsedMem;
}


void VMemCppImpl::GetStats(VMemStats& stats, sWORD blocknumber)
{
	//VKernelTaskLock lock(&fGlobalMemMutext);
	VMemImplAllocation* pAlloc = fFirstAllocation;
	while (pAlloc != NULL)
	{
		VMemImplBlock* pblock = (VMemImplBlock*)pAlloc->GetDataStart();
		VMemImplBlock* allocEnd = (VMemImplBlock*)pAlloc->GetDataEnd();

		// push boundary marker
		{
			MemImplBlockInfo implBlockInfo = { (sLONG_PTR) pblock, -2, blocknumber};
			stats.fMemImplBlockInfos.push_back( implBlockInfo);
		}

		VSize previouslen = 0;
		Boolean previouswasfree = false;
		while (pblock < allocEnd)
		{
			VSize len = pblock->GetLen();
			assert(pblock->GetOwner() == pAlloc);
			assert(pblock->GetPreviousLen() == previouslen);

			if (len > stats.fBiggestBlock)
				stats.fBiggestBlock = len;

			stats.fNbBigBlocks++;

			if (pblock->IsFree())
			{
				assert(!previouswasfree);
				if (len > stats.fBiggestBlockFree)
					stats.fBiggestBlockFree = len;

				stats.fNbBigBlocksFree++;
				stats.fFreeMem = stats.fFreeMem + len;

				previouswasfree = true;

				stats.fOtherBlockInfo[len].fFreeCount++;

				MemImplBlockInfo implBlockInfo = { (sLONG_PTR) len, 0, 0 };
				stats.fMemImplBlockInfos.push_back( implBlockInfo);
			}
			else
			{
				stats.fNbBigBlocksUsed++;
				stats.fUsedMem = stats.fUsedMem + len;

				if (pblock->IsAPageAllocation())
				{
					stats.fNbPages++;
					VPageAllocationImpl* page = (VPageAllocationImpl*)(((char*)pblock) + SizeHeader);
					bool isfull;
					page->GetStats(stats, isfull);
					if (isfull)
						stats.fBigBlockInfo[len].fUsedCount++;
					else
						stats.fBigBlockInfo[len].fFreeCount++;
					MemImplBlockInfo implBlockInfo = { (sLONG_PTR) len, (sWORD) page->GetElemSize(), page->GetNbFull()};
					stats.fMemImplBlockInfos.push_back( implBlockInfo);
				}
				else
				{
					stats.fOtherBlockInfo[len].fUsedCount++;
					MemImplBlockInfo implBlockInfo = { (sLONG_PTR) len, -1, 0};
					stats.fMemImplBlockInfos.push_back( implBlockInfo);
					if (pblock->IsAnObject())
					{
						stats.fNbObjects++;
#if CPPMEM_CACHE_INFO
						VObject* p = (VObject*)(((char*)pblock) + SizeHeader);
						try
						{
							// check null vtbl (consructor has not been called yet)
							if ( *(void**) p == NULL)
							{
								sLONG tag = pblock->GetTag();
								VObjectInfo* info = &(stats.fBlockInfo[tag]);
								info->fCount++;
								info->fTotalSize += len;
							}
							else
							{
								const type_info* typinf = &(typeid(*p));
								//Cast pour éviter le C4267
								sLONG xlen = (sLONG) strlen(typinf->name());
								VObjectInfo* info = &(stats.fObjectInfo[typinf]);
								info->fCount++;
								info->fTotalSize = info->fTotalSize + len;

								// collect vtbl -> type info
								stats.fTypeInfoByVtbl[*(void**)p] = typinf; 
							}
						}
						catch (...)
						{
							sLONG debugplus = 1;
							sLONG tag = pblock->GetTag();
							VObjectInfo* info = &(stats.fBlockInfo[tag]);
							info->fCount++;
							info->fTotalSize = info->fTotalSize + len;
						}
#endif
					}
#if CPPMEM_CACHE_INFO
					else
					{
						sLONG tag = pblock->GetTag();
						VObjectInfo* info = &(stats.fBlockInfo[tag]);
						info->fCount++;
						info->fTotalSize = info->fTotalSize + len;
					}
#endif
				}

				previouswasfree = false;
			}

			previouslen = len;
			pblock = (VMemImplBlock*)((char*)pblock + pblock->GetLen());
		}

		VSize lastfreeblocksize = pAlloc->GetUnusedMemAtTheEnd();
		if (lastfreeblocksize > 0)
		{
			MemImplBlockInfo implBlockInfo = { (sLONG_PTR) lastfreeblocksize, 0, 0 };
			stats.fMemImplBlockInfos.push_back( implBlockInfo);
		}

		if (lastfreeblocksize > stats.fBiggestBlock)
			stats.fBiggestBlock = lastfreeblocksize;
		if (lastfreeblocksize > stats.fBiggestBlockFree)
			stats.fBiggestBlockFree = lastfreeblocksize;

		stats.fFreeMem = stats.fFreeMem + lastfreeblocksize;

		pAlloc = pAlloc->GetNext();
	}

}




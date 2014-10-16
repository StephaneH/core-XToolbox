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
#ifndef __VMemoryImpl__
#define __VMemoryImpl__

#include "Kernel/Sources/VSyncObject.h"
#include "Kernel/Sources/VMemoryCpp.h"
#include <map>

BEGIN_TOOLBOX_NAMESPACE

#if WITH_ASSERT
	#define CPPMEM_CACHE_INFO 1
#else
	#define CPPMEM_CACHE_INFO 1
#endif

// Class constants
const VSize	kDefaultMemAllocationSize = 20*1024*1024;

const VSize	kFirstStepAlloc = 1024;
const VSize kFirstStepAllocInc = 16;
const sLONG kFirstStepAllocNbPages = kFirstStepAlloc / kFirstStepAllocInc + 1; // kFirstStepAlloc / kFirstStepAllocInc

const VSize kSecondStepAlloc = 4096;
const VSize kSecondStepAllocInc = 64;
const sLONG kSecondStepAllocNbPages = (kSecondStepAlloc-kFirstStepAlloc) / kSecondStepAllocInc + 1; // (kSecondStepAlloc-kFirstStepAlloc) / kSecondStepAllocInc

const VSize kMinRemainingSizeBlock = 8192;

const VSize kThirdStepAlloc = 8192;
const VSize kThirdStepAllocInc = 256;
const sLONG kThirdStepAllocNbPages = (kThirdStepAlloc-kSecondStepAlloc) / kThirdStepAllocInc + 1; // (kThirdStepAllocInc-kSecondStepAlloc) / kThirdStepAllocInc

const VSize kFourthStepAlloc = 1048576;
const VSize kFourthStepAllocInc = 1024;
const sLONG kFourthStepAllocNbPages = (kFourthStepAlloc/*-kThirdStepAllocInc*/) / kFourthStepAllocInc + 1; // (kFourthStepAlloc-kThirdStepAllocInc) / kFourthStepAllocInc

const sLONG kTotalStepAllocPagesInMain = kFourthStepAllocNbPages + 1;
const sLONG kTotalStepAllocPagesInThread = kFirstStepAllocNbPages + kSecondStepAllocNbPages + kThirdStepAllocNbPages + 1;


// Defined bellow
class VMemImplBlock;
class VMemCppImpl;
class VMemThreadImpl;

class VMemStats;

class VMemImplAllocation
{
public:
	void	Init (VSize inSize, bool inPhysicalMemory);
	
	VMemCppImpl*	GetOwner () const { return fOwner; };
	void	SetOwner (VMemCppImpl* inOwner) { fOwner = inOwner; };
	
	char*	GetDataStart () const { return (char*)&fDataStart; };
	char*	GetDataEnd () const { return ((char*)&fDataStart) + fDataEnd; };
	
	VMemImplAllocation*	GetNext () const { return fNext; };
	void	SetNext (VMemImplAllocation* inNext) { fNext = inNext; };

	VMemImplAllocation*	GetPrevious () const { return fPrevious; };
	void	SetPrevious (VMemImplAllocation* inPrevious) { fPrevious = inPrevious; };

	VMemImplBlock*	AllocateAtTheEnd (VSize inSize, VSize& ioUsedMem, Boolean isAnObject, Boolean ForceInMem, sLONG inTag);
	Boolean	IsLastBlock (const VMemImplBlock* inBlock) const;
	
	Boolean	Contains (const VMemImplBlock* inBlock) const;
	void	ReduceFrom (const VMemImplBlock* inBlock);

	inline VSize GetUnusedMemAtTheEnd() { return (fAllocationSize - fDataEnd); };

	inline Boolean IsEmpty() const { return fDataEnd == 0; };

	inline VSize GetSystemSize() const { return fAllocationSize + sizeof(VMemImplAllocation); };
	bool	IsPhysicalMemory() const	{ return fPhysicalMemory;}

private:
	VMemCppImpl*	fOwner;
	VMemImplAllocation*	fNext;
	VMemImplAllocation*	fPrevious;
	VSize	fAllocationSize;
	VSize	fDataEnd;
	VSize	LastLen;
	bool	fPhysicalMemory;	// tell if this block as been physically locked into memory (needed for deallocation)
	void*	fDataStart;
};



class VMemImplBlock
{
public:
	VMemImplAllocation*	GetOwner () const { return fOwner; };
	void	SetOwner (VMemImplAllocation* inOwner) { fOwner = inOwner; };
	
	VMemImplBlock*	GetNext () const { return fNext; };
	void	SetNext (VMemImplBlock* inNext) { fNext = inNext; };
	
	VMemImplBlock*	GetPrevious () const { return fPrevious; };
	void	SetPrevious (VMemImplBlock* inPrevious) { fPrevious = inPrevious; };
	
	Boolean	IsFree () const { return fLength<0; };
	VSize	GetLen () const { if (fLength>0) return (VSize)(fLength & -4); else return (VSize)(-fLength); };
	VSize	GetPreviousLen () const { return (VSize)fPreviousLength; };

	Boolean IsASmallBlock() const { return fPreviousLength < 0; };

	void	SetLen (VSize inLength) { fLength = (sLONG)inLength; };
	void	SetPreviousLen (VSize inLength) { fPreviousLength = (sLONG)inLength; };
	void	SetAsFree(VSize inLength) { fLength = -((sLONG)inLength); };

	inline Boolean IsAnObject() { return (fLength > 0) && ((fLength & 1) == 1); };
	inline Boolean IsAPageAllocation() { return (fLength > 0) && ((fLength & 2) == 2); };

	VMemImplBlock* GetPreviousInAllocation() const { if (GetPreviousLen()) return (VMemImplBlock*) (((char*)this) - GetPreviousLen()); else return NULL; }
	VMemImplBlock* GetNextInAllocation() const { if (GetOwner () && !GetOwner()->IsLastBlock(this)) return (VMemImplBlock*) (((char*)this) + GetLen()); else return NULL; }

#if CPPMEM_CACHE_INFO
	inline void SetTag(sLONG inTag) { fTag = inTag; };
	inline sLONG GetTag() const { return fTag; };
#endif

private:
	VMemImplAllocation* fOwner;
	sLONG	fLength;
#if CPPMEM_CACHE_INFO && ARCH_64
	sLONG	fUnused;	// ForAlignment. beware: fPreviousLength must be at same offset from the end as VMemImplSmallBlock::fOffset
#endif
	sLONG	fPreviousLength;
#if CPPMEM_CACHE_INFO
	sLONG	fTag;
#endif
	VMemImplBlock*	fNext;	// a partir d'ici ne prend pas de place si block est plein
	VMemImplBlock*	fPrevious;
};



class VMemImplSmallBlock
{
public:
	sLONG	GetOffset () const { return fOffset; };
	void	SetOffset (sLONG xoffset) { fOffset = xoffset; };
	
	VMemImplSmallBlock*	GetNext () const { return fNext; };
	void	SetNext (VMemImplSmallBlock* inNext) { fNext = inNext; };

	inline Boolean IsAnObject() { return ((-fOffset) & 1) == 1; };
	
	//VMemImplSmallBlock*	GetPrevious () const { return fPrevious; };
	//void	SetPrevious (VMemImplSmallBlock* inPrevious) { fPrevious = inPrevious; };

#if CPPMEM_CACHE_INFO
	inline void SetTag(sLONG inTag) { fTag = inTag; };
	inline sLONG GetTag() const { return fTag; };
#endif

private:
	sLONG	fOffset;
#if CPPMEM_CACHE_INFO
	sLONG	fTag;
#endif
	VMemImplSmallBlock*	fNext;	// a partir d'ici ne prend pas de place si block est plein
	//VMemImplSmallBlock* fPrevious;
};



class VPageAllocationImpl
{
public:
	VPageAllocationImpl (VMemThreadImpl* inOwner, VSize inSize, VSize inElemSize);
	
	//void	Init (VMemThreadImpl* inOwner, VSize inSize, VSize inElemSize);
	
	VSize	GetDataStart () { return ((char*)&fDataStart) - ((char*)this); };
	void*	Malloc (VSize inSize, Boolean isAnObject, sLONG inTag);
	void	Free (VMemImplSmallBlock* inBlock);

	VPageAllocationImpl*	GetNext () const { return fNext; };
	void	SetNext (VPageAllocationImpl* inNext) { fNext = inNext; };
	
	VPageAllocationImpl*	GetPrevious () const { return fPrevious; };
	void	SetPrevious (VPageAllocationImpl* inPrevious) { fPrevious = inPrevious; };
	
	VMemThreadImpl*	GetOwner () const { return fOwner; };
	VSize	GetElemSize () const { return (VSize)fSizeOfEachElem; }; 
	VSize	GetMaxElems () const { return (VSize)fMaxElems; }; 
//	void	Unlock () { fMutex.Unlock(); };

	Boolean	Check (void* skipthisone = NULL);
	void GetStats(VMemStats& stats, bool& isfull);

	Boolean IsBlockFree(VMemImplSmallBlock* inBlock);

	sWORD	GetNbFull() const	{ return fNbFull;}
	
private:
	//VKernelCriticalSection	fMutex;
	VMemThreadImpl*	fOwner;
	VMemImplSmallBlock*	fFirstFree;
	VPageAllocationImpl*	fNext;
	VPageAllocationImpl*	fPrevious;
	VSize	fAllocationSize;
	VSize	fDataEnd;
	sWORD	fNbHole;
	sWORD	fNbFull;
	sWORD	fMaxElems;
	sWORD	fSizeOfEachElem;
	void*	fDataStart;
};



class VMemThreadImpl
{
public:
#if CPPMEM_CACHE_INFO
	enum { SizeSmallHeader = sizeof(sLONG) + sizeof(sLONG) }; // 1 offset sur le debut de la page proprietaire du block , plus un tag de 4 octets
#else
	enum { SizeSmallHeader = sizeof(sLONG) }; // 1 offset sur le debut de la page proprietaire du block
#endif

	enum { 
		stage1 = 0,
		stage2 = kFirstStepAllocNbPages, 
		stage3 = kFirstStepAllocNbPages + kSecondStepAllocNbPages
	};

	VMemThreadImpl ();
	
	VMemCppImpl*	GetOwner () const { return fOwner; };
	void	SetOwner (VMemCppImpl* inOwner) { fOwner = inOwner; };
	
	void*	Malloc (VSize inSize, Boolean isAnObject, sLONG inTag);
	void	RemovePage (VPageAllocationImpl* ioPage);
	void	MarkPageAsNotFull (VPageAllocationImpl* ioPage);
	void	MarkPageAsFull (VPageAllocationImpl* ioPage);
	
//	void	Lock () { fMutex.Lock(); };
//	void	Unlock () { fMutex.Unlock(); };

	Boolean	Check (void* skipthisone = NULL);

private:
	VMemCppImpl*	fOwner;
	//VKernelCriticalSection	fMutex;
	//VPageAllocationImpl*	fPages[kTotalStepAllocPagesInThread];
	VPageAllocationImpl*	fNotFullPages[kTotalStepAllocPagesInThread];
	
	sLONG	GetStepFromSize (VSize inSize, sLONG* outStepInc);
};



class XTOOLBOX_API VMemCppImpl : public XMemCppImpl
{
public:
	VMemCppImpl (sLONG inBlockNumber);
	~VMemCppImpl ();
	
	enum { /*
		 stage1 = 0,
		 stage2 = kFirstStepAllocNbPages, 
		 stage3 = kFirstStepAllocNbPages + kSecondStepAllocNbPages,
		 */
		 stage4 = 0,
		 stage5 = kFourthStepAllocNbPages
		 //stage4 = kFirstStepAllocNbPages + kSecondStepAllocNbPages + kThirdStepAllocNbPages,
		 //stage5 = kFirstStepAllocNbPages + kSecondStepAllocNbPages + kThirdStepAllocNbPages + kFourthStepAllocNbPages
	};
#if CPPMEM_CACHE_INFO
	// 1 Ptr qui contient le block d'allocation et 2 sLONG qui contiennent la longueur d'un block et celle du block precedent
	// plus 1 sLONG en 64bit pour garantir un multiple de 8 octets
#if ARCH_64
	enum { SizeHeader = sizeof(void*)+sizeof(sLONG)+sizeof(sLONG) + sizeof(sLONG) + sizeof(sLONG) }; 
#else
	enum { SizeHeader = sizeof(void*)+sizeof(sLONG)+sizeof(sLONG) + sizeof(sLONG)}; 
#endif
																	// en debug on ajoute un tag sur 4 octets
#else
	enum { SizeHeader = sizeof(void*)+sizeof(sLONG)+sizeof(sLONG) }; // 1 Ptr qui contient le block d'allocation et 2 sLONG qui contiennent la longueur d'un block et celle du block precedent
#endif
	enum { MaxThreads = 1 };

	virtual	void	Init (VCppMemMgr* inOwner);

	virtual	Boolean	AddAllocation( VSize inSize, const void *inHintAddress, Boolean inPhysicalMem);
	virtual	void	SetAutoAllocationState (bool inState) { fCanAutoAddAllocation = inState; };

	// return count of VMemImplAllocation mater blocks
	virtual	VIndex	CountAllocations();

	virtual	void*	Malloc (VSize inSize, Boolean inForceinMain, Boolean isAnObject, sLONG inTag);
	virtual	void	Free (void *inBlock);
	virtual	void*	Realloc (void *inData, VSize inNewSize);
	virtual	VSize	GetPtrSize (const void *inBlock);
	
	virtual	Boolean CheckPtr (const void* inBock);
	
	virtual	Boolean Check (void* skipthisone = NULL);

	virtual	void GetStats(VMemStats& stats, sWORD blocknumber);

	virtual	VSize GetTotalAllocation() const { return fTotalAllocation; };

	virtual void GetMemUsageInfo(VSize& outTotalMem, VSize& outUsedMem);

	void AddAlreadyAllocatedMem(void* allocatedMem, VSize memSize, bool locked);

	inline void AddToUsed(VSize len)
	{
		fUsedMem = fUsedMem + len;
	}

	inline void FreeFromUsed(VSize len)
	{
		fUsedMem = fUsedMem - len;
	}

	sLONG GetAllocationBlockNumber(void *inBlock);


private:
	//VKernelCriticalSection	fMutex;
	//VKernelCriticalSection fGlobalMemMutext;
	VMemThreadImpl	fThreads[MaxThreads];
	VCppMemMgr*	fOwner;
	VMemImplAllocation*	fFirstAllocation;
	VMemImplBlock*	fFirstBlocks[kTotalStepAllocPagesInMain];
	VMemImplBlock*	fLastBlockToAllocateFrom;
	VSize	fTotalAllocation;
	VSize	fUsedMem;
	sLONG	fAllocationBlockNumber;
	bool	fCanAutoAddAllocation;
	
	sLONG	GetStepFromSize (VSize inSize, sLONG* outStepInc);
	sLONG	GetExactStepFromSize (VSize inSize);
	void	RemoveBlockFromChainList (const VMemImplBlock* inBlock, VMemImplBlock** inFirst);
	void	MergeContiguousFreeBlock(VMemImplBlock** ioBlockBeingFreed, VMemImplBlock* iFreeBlockToMerge);

#if VERSIONDEBUG
	uLONG	fDebugNbCall;
#endif
};

END_TOOLBOX_NAMESPACE

#endif

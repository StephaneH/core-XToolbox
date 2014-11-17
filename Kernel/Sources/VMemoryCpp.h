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
#ifndef __VMemoryCpp__
#define __VMemoryCpp__

#include "Kernel/Sources/VDebugBlockInfo.h"
#include "Kernel/Sources/VMemory.h"
#include "Kernel/Sources/VSyncObject.h"
#include "Kernel/Sources/VLeaks.h"
#include "Kernel/Sources/VChain.h"

#include <map>


BEGIN_TOOLBOX_NAMESPACE


#if XTOOLBOX_AS_STANDALONE
typedef VSystemCriticalSection	VKernelCriticalSection;
#else
typedef VCriticalSection	VKernelCriticalSection;
#endif
typedef StLocker<VKernelCriticalSection> VKernelTaskLock;
typedef StLocker<VSystemCriticalSection> VSystemTaskLock;


class XTOOLBOX_API VMemoryHog
{
	public:
		inline VMemoryHog(sLONG inTaskID)
		{
			fTaskID = inTaskID;
		};

	private:
		VCriticalSection fMutex;
		sLONG fTaskID;
};


// Needed declaration
class IMemoryWalker;
class VArrayLong;
class VCppMemMgr;

// Class definitions
typedef VSize (*PurgeHandlerProc) (sLONG allocationBlockNumber, VSize inNeededBytes, bool withFlush);


// Class macros (defined here to avoid extra dependencies in VObject.h and VAssert.h)
#undef XBOX_ASSERT_VOBJECT
#undef XBOX_CHECK_VOBJECT

#if WITH_ASSERT
#define XBOX_ASSERT_VOBJECT(_object_) \
	assert(VObject::GetAllocator()->CheckVObject(_object_))

#define XBOX_CHECK_VOBJECT(_object_) \
	(CHECK_NIL(VObject::GetAllocator()->CheckVObject(_object_)))
#else
#define XBOX_ASSERT_VOBJECT(_object_)
#define XBOX_CHECK_VOBJECT(_object_) (_object_)
#endif	// VERSIONDEBUG



class XTOOLBOX_API VObjectInfo
{
public:
	inline VObjectInfo()
	{
		fCount = 0;
		fTotalSize = 0;
	}

	sLONG fCount;
	VSize fTotalSize;
};

class XTOOLBOX_API VMemBlockInfo
{
public:
	inline VMemBlockInfo()
	{
		fUsedCount = 0;
		fFreeCount = 0;
	}

	sLONG fUsedCount;
	sLONG fFreeCount;
};


// describes the content of a VMemImplBlock
struct MemImplBlockInfo
{
	sLONG_PTR	fSize;	// size of block or block start address in case of boundary marker.
	sWORD		fInnerSize;	// 0 : free, -1 : other block, -2 : boundary marker, else page allocation size.
	sWORD		fInnerUsedCount;	// page allocation used count. If this info is a boundary marker, fInnerUsedCount is the allocation block number.
};

typedef XTOOLBOX_TEMPLATE_API std::map< const std::type_info* , VObjectInfo > VMapOfObjectInfo;
typedef XTOOLBOX_TEMPLATE_API std::map< sLONG , VObjectInfo > VMapOfBlockInfo;
typedef XTOOLBOX_TEMPLATE_API std::vector<VMemoryHog*> VStackOfMemHogs;
typedef XTOOLBOX_TEMPLATE_API std::map< VSize , VMemBlockInfo > VMapOfMemBlockInfo;
typedef XTOOLBOX_TEMPLATE_API std::vector<MemImplBlockInfo> VectorOfMemImplBlockInfo;
typedef XTOOLBOX_TEMPLATE_API std::map<void*,const std::type_info*> VMapOfTypeInfoByVtbl;

class VStream;

class XTOOLBOX_API VMemStats
{
public:
	VMemStats()
	{
		fFreeMem = 0;
		fUsedMem = 0;
		fNbPages = 0;
		fNbSmallBlocks = 0;
		fNbSmallBlocksFree = 0;
		fNbSmallBlocksUsed = 0;
		fNbBigBlocks = 0;
		fNbBigBlocksFree = 0;
		fNbBigBlocksUsed = 0;
		fBiggestBlock = 0;
		fBiggestBlockFree = 0;
		fNbObjects = 0;
	};

	void Dump();
	void Clear();
	VError DumpToStream(VStream* outStream);

	VSize fFreeMem;
	VSize fUsedMem;
	sLONG fNbPages;
	sLONG fNbObjects;
	sLONG fNbSmallBlocks;
	sLONG fNbSmallBlocksFree;
	sLONG fNbSmallBlocksUsed;
	sLONG fNbBigBlocks;
	sLONG fNbBigBlocksFree;
	sLONG fNbBigBlocksUsed;
	VSize fBiggestBlock;
	VSize fBiggestBlockFree;
	VMapOfObjectInfo fObjectInfo;
	VMapOfBlockInfo fBlockInfo;
	VMapOfMemBlockInfo fSmallBlockInfo;
	VMapOfMemBlockInfo fBigBlockInfo;
	VMapOfMemBlockInfo fOtherBlockInfo;
	VectorOfMemImplBlockInfo	fMemImplBlockInfos;
	VMapOfTypeInfoByVtbl	fTypeInfoByVtbl;
};




/*
	interface for a memory manager implementation
*/
class XTOOLBOX_API XMemCppImpl
{
public:
	virtual ~XMemCppImpl() {}
	virtual	void	Init( VCppMemMgr* inOwner) = 0;

	virtual	void*	Malloc( VSize inSize, Boolean inForceinMain, Boolean isAnObject, sLONG inTag) = 0;
	virtual	void	Free( void *inBlock) = 0;
	virtual	void*	Realloc( void *inData, VSize inNewSize) = 0;

	virtual	VSize	GetPtrSize( const void *inBlock) = 0;

	virtual	Boolean CheckPtr (const void* inBock) = 0;
	virtual	Boolean Check (void* skipthisone = NULL) = 0;

	virtual	Boolean	AddAllocation( VSize inSize, const void *inHintAddress, Boolean inPhysicalMem) = 0;
	virtual	VIndex	CountAllocations() = 0;
	virtual	void	SetAutoAllocationState( bool inState) = 0;

	virtual	void	GetStats(VMemStats& stats, sWORD blocknumber) = 0;
	virtual	VSize	GetTotalAllocation() const = 0;
	virtual void	GetMemUsageInfo(VSize& outTotalMem, VSize& outUsedMem) = 0;
};


typedef std::vector<XMemCppImpl*> memarray;



class XTOOLBOX_API VCppMemMgr
{
public:
	enum {
		kSizeOfDebugBlockInfoPool = sizeof(VDebugBlockInfoPool) + sizeof(VSize)	// Beeded bytes for new VDebugBlockInfoPool
	};

	enum EAllocatorKind {
		kAllocator_default = 0,	// choose appropriate at runtime
		kAllocator_stdlib = 1,
		kAllocator_xbox = 2
	} ;
					VCppMemMgr( EAllocatorKind inKind = kAllocator_default);
					VCppMemMgr( EAllocatorKind inKind, bool inWithDebugInfo, bool inWithStrangeFill);
	virtual			~VCppMemMgr();

	// Allocation support (some methods are inherited from VMemoryMgr)
			void*	Malloc( VSize inNbBytes, bool inIsVObject, sLONG inTag, sLONG preferedBlock = -1);
			void*	Calloc( VIndex inNbElem, VSize inNbBytesPerElem, sLONG inTag);

			VPtr	NewPtr( VSize inNbBytes, bool inIsVObject, sLONG inTag, sLONG preferedBlock = -1)		{ return (VPtr) Malloc( inNbBytes, inIsVObject, inTag, preferedBlock); }
			//VPtr	NewPtr( VSize inNbBytes)						{ return (VPtr) Malloc( inNbBytes, false); }
			VPtr	NewPtrClear( VSize inNbBytes, bool inIsVObject, sLONG inTag) { void *ptr = Malloc( inNbBytes, inIsVObject, inTag); if (ptr) ::memset( ptr, 0, inNbBytes); return (VPtr) ptr; }

			void	Free( void* ioPtr);
			void	DisposePtr( void* ioPtr)						{ Free( ioPtr); }


			void*	Realloc( void* ioPtr, VSize inNbBytes);
			VPtr	SetPtrSize( void* ioPtr, VSize inNbBytes)		{ return (VPtr) Realloc( ioPtr, inNbBytes); }
			VSize	GetPtrSize( const VPtr ioPtr);

	// Memory state support
			VSize	GetFreeMemory();
			VSize	GetMaxFree();
			VSize	GetMaxUsed();
			VSize	GetAllocatedMem();
			VSize	GetUsedMem();

			void	GetUsage( VArrayString& inTabClassNames, VArrayLong& inTabNbObjects, VArrayLong& inTabTotalSize);
			void	GetMemStatistics( VSize& outNbHeaps, VSize& outNbBlocs, VSize& outTotalAlloc, VSize& outNbUsed, VSize& outTotalUsed, VSize& outMaxUsed, VSize& outNbFree, VSize& outTotalFree, VSize& outMaxFree);


			void	GetMemUsageInfo(VSize& outTotalMem, VSize& outUsedMem);

			void	GetStatistics( VMemStats& outStats, sLONG blockNumber = -1);

	// Leaks support
			void	DumpLeaks();
			bool	Walk( IMemoryWalker& inWalker);

	// Accessors
			bool	SetWithLeaksCheck( bool inSetIt)				{ return fLeaksChecker.SetLeaksCheck( inSetIt); }
			bool	IsWithLeaksCheck() const						{ return fLeaksChecker.IsWithLeaksCheck(); }

			bool	IsWithStrangeFill() const						{ return fWithStrangeFill; }
			bool	IsWithDebugInfo() const							{ return fWithDebugInfo; }

			void	RegisterUser( void* inInstance);
			void	UnregisterUser( void* inInstance);

			PurgeHandlerProc	SetPurgeHandlerProc( PurgeHandlerProc inProc);
			bool	AddVirtualAllocation( VSize inMaxBytes, const void *inHintAddress, bool inPhysicalMemory);
			void	SetAutoAllocationState (bool inState)			{ if (fUseStdLibMgr) fStdMemMgr->SetAutoAllocationState(inState); }

			// return count of VMemImplAllocation mater blocks
			VIndex	CountVirtualAllocations();

	// Check utilities
			bool	IsAPtr( void* inAddress, size_t inSize = 0);
			void*	CheckVObject( const void* ioPtr);

			bool	CheckNow();
			bool	Check();
			bool	CheckPtr( const void* ioPtr);

			// Other utilities
			void	IncMemAllocatedCount( VSize inNbBytes) { fCurrentVirtualAllocatedSize = fCurrentVirtualAllocatedSize + inNbBytes; };
			void	DecMemAllocatedCount( VSize inNbBytes) { fCurrentVirtualAllocatedSize = fCurrentVirtualAllocatedSize - inNbBytes; };

			#if 0
			void*	DoVirtualAlloc( VSize inNbBytes);
			void	DoVirtualFree( void* inAddr, VSize inNbBytes);
			#endif

			// Operators
			void*	operator new( size_t inSize);
			void	operator delete( void* inAddress);

			VMemoryHog*	RegisterMemHog();
			void		UnRegisterMemHog(VMemoryHog* memhog);

			XMemCppImpl* GetMemMgrImpl()
			{
				if (fUseStdLibMgr)
					return fStdMemMgr;
				else
					return fMems[0];
			}

			sLONG GetAllocationBlockNumber(void* inAddr);

			void PurgeMem(sLONG whatBlock = -1);


private:
			void	_Init( EAllocatorKind inKind, bool inWithDebugInfo, bool inWithStrangeFill);
			void*	TryToMalloc( VSize inNbBytes, bool inIsVObject, sLONG inTag, sLONG preferedBlock);

			//XMemCppImpl*					fMemMgr;
			VKernelCriticalSection			fMgrMutex;
			XMemCppImpl*					fStdMemMgr;
			memarray						fMems;
			sLONG							fNbMems;
			sLONG							fMaxMems;
			sLONG							fCurrentMem;
			sLONG							fCurrentlyPurging;
			VSize							fBiggestAllocationBlock;
			VSyncEvent*						fCurrentlyPurgingEvent;


			PurgeHandlerProc				fPurgeProc;
			VSize							fMaxVirtualAllocatedSize;
			VSize							fCurrentVirtualAllocatedSize;
			sLONG							fDebugCheck;
			bool							fWithDebugInfo;
			bool							fWithStrangeFill;
			bool							fUseStdLibMgr;
			VLeaksChecker					fLeaksChecker;
			VDebugBlockInfo*				fInfoFirstFree;
			VSystemCriticalSection			fInfoLock;
			VChainOf<VDebugBlockInfoPool>	fInfoPool;
			VSyncEvent*						fWaitBeforeNewPtr;
			VCriticalSection				fWaitBeforeNewPtrMutex;
			sLONG							fWaitBeforeNewPtrStarter;
			VStackOfMemHogs					fMemHogsStack;

	// Private allocation support
			void	RegisterBlock( DebugBlockHeader* inAddr, VSize inUserSize, bool inIsVObject);
			void	UnregisterBlock( DebugBlockHeader* inAddr);
};


class XTOOLBOX_API StTurnOffLeaksCheck
{
public:
			StTurnOffLeaksCheck()		{ fWasOn = VObject::GetAllocator()->SetWithLeaksCheck(false); };
			~StTurnOffLeaksCheck()		{ assert(!VObject::GetAllocator()->SetWithLeaksCheck(fWasOn)); };

protected:
	bool fWasOn;
};


inline void*	vMalloc (VSize inNbBytes, sLONG inTag)			{ return VObject::GetAllocator()->NewPtr(inNbBytes, false, inTag); }
inline void*	vCalloc (VIndex inNbElem, VSize inNbBytesPerElem, sLONG inTag)	{ return VObject::GetAllocator()->Calloc(inNbElem, inNbBytesPerElem, inTag); }
inline void*	vRealloc (void* inAddr, VSize inNbBytes)			{ return VObject::GetAllocator()->SetPtrSize((VPtr)inAddr, inNbBytes); }
inline void		vFree (void* inAddr)								{ VObject::GetAllocator()->DisposePtr((VPtr)inAddr); }
inline VSize	vMallocSize (const void* inAddr)					{ return VObject::GetAllocator()->GetPtrSize((VPtr)inAddr); }


#ifdef __cplusplus
extern "C" {
#endif

void*	xMalloc (unsigned long inNbBytes);
void*	xRealloc (void* inAddr, unsigned long inNbBytes);
void	xFree (void* inAddr);

#ifdef __cplusplus
}
#endif

END_TOOLBOX_NAMESPACE

#endif

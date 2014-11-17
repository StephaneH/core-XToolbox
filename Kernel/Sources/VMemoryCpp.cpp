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
#include "VMemoryWalker.h"
#include "VArrayValue.h"
#include "VString.h"
#include "VProcess.h"
#include "VTask.h"
#include "VSystem.h"
#include "VInterlocked.h"
#include "VMemoryCpp.h"
#include "VMemoryImpl.h"
#include "VFile.h"
#include "VStream.h"
#include "VFileStream.h"

#include <algorithm>


#if VERSIONMAC

    #include <malloc/malloc.h>
    #define GET_PTR_SIZE(ptr) malloc_size((void*)(ptr))

#elif VERSION_LINUX

    #include <malloc.h>
    #define GET_PTR_SIZE(ptr) malloc_usable_size((void*)(ptr))

#elif VERSIONWIN

    #define GET_PTR_SIZE(ptr) _msize((void*)(ptr))

#endif


#define WITH_PURGE_TRIGGER 0


BEGIN_TOOLBOX_NAMESPACE

class VMemCppImpl_stdlib : public XMemCppImpl
{
public:
	virtual	void	Init( VCppMemMgr* /*inOwner*/)										{}

	virtual	Boolean	AddAllocation( VSize /*inSize*/, const void* /*inHintAddress*/, Boolean /*inPhysicalMem*/) { return false;}
	virtual	VIndex	CountAllocations()	{ return 1;}
	virtual	void	SetAutoAllocationState (bool /*inState*/)							{}

	virtual	void*	Malloc( VSize inSize, Boolean /*inForceinMain*/, Boolean /*isAnObject*/, sLONG /*inTag*/)  { return ::malloc( inSize);}
	virtual	void	Free( void *inBlock)												{ ::free( inBlock);}
	virtual	void*	Realloc( void *inData, VSize inNewSize)								{ return ::realloc( inData, inNewSize);}

	virtual	VSize	GetPtrSize( const void *inBlock)									{ return GET_PTR_SIZE(inBlock);}

	virtual	Boolean CheckPtr (const void* /*inBock*/)									{ return true;}
	virtual	Boolean Check (void* /*skipthisone*/ = NULL)								{ return true;}

	virtual	void	GetStats(VMemStats& /*stats*/, sWORD /*blocknumber*/)				{}
	virtual	VSize	GetTotalAllocation() const											{ return 1024*1024; }

	virtual void	GetMemUsageInfo(VSize& outTotalMem, VSize& outUsedMem)
	{
		outTotalMem = 1024*1024;
		outUsedMem = 0;
	}
};


static void DumpObjectInfo(VMapOfObjectInfo::value_type& x)
{
	const std::type_info* typobj = x.first;
	xbox::VStr<80> s;
	s.FromCString( typobj->name());
	DebugMsg(s);
	s.FromLong(x.second.fCount);
	DebugMsg(L" : ");
	DebugMsg(s);
	s.FromLong8(x.second.fTotalSize);
	DebugMsg(L"   ,  mem = ");
	DebugMsg(s);
	DebugMsg(L"\n");
}


static void DumpBlockInfo(VMapOfBlockInfo::value_type& x)
{
	sLONG tag = x.first;
#if SMALLENDIAN
	ByteSwap(&tag);
#endif
	xbox::VStr<80> s;
	s.FromBlock(&tag, 4, VTC_StdLib_char);
	DebugMsg(s);
	s.FromLong(x.second.fCount);
	DebugMsg(L" : ");
	DebugMsg(s);
	s.FromLong8(x.second.fTotalSize);
	DebugMsg(L"   ,  mem = ");
	DebugMsg(s);
	DebugMsg(L"\n");
}


static void DumpMemBlockInfo(VMapOfMemBlockInfo::value_type& x)
{
	xbox::VStr<80> s;

	s.FromLong8(x.first);
	DebugMsg(s);
	DebugMsg(L" : NbUsed = ");
	s.FromLong(x.second.fUsedCount);
	DebugMsg(s);
	DebugMsg(L" , NbFree = ");
	s.FromLong(x.second.fFreeCount);
	DebugMsg(s);
	DebugMsg(L"\n");
}

static void DumpAllocPageInfo(VMapOfMemBlockInfo::value_type& x)
{
	xbox::VStr<80> s;

	s.FromLong8(x.first);
	DebugMsg(s);
	DebugMsg(L" : NbFull = ");
	s.FromLong(x.second.fUsedCount);
	DebugMsg(s);
	DebugMsg(L" , NbPartial = ");
	s.FromLong(x.second.fFreeCount);
	DebugMsg(s);
	DebugMsg(L"\n");
}



VError VMemStats::DumpToStream(VStream* outStream)
{
	VError err = VE_OK;

	outStream->PutText(L"Used Mem = "+ToString(fUsedMem)+L"\n");
	outStream->PutText(L"Free Mem = "+ToString(fFreeMem)+L"\n");
	outStream->PutText(L"Biggest Free Block = "+ToString(fBiggestBlock)+L"\n");
	outStream->PutText(L"\n\n");

	outStream->PutText(L"Nb Objects = "+ToString(fNbObjects)+L"\n");
	outStream->PutText(L"\n\n");
	for (VMapOfObjectInfo::iterator cur = fObjectInfo.begin(), end = fObjectInfo.end(); cur != end; cur++)
	{
		const std::type_info* typobj = cur->first;
		xbox::VStr<80> s;
		s.FromCString( typobj->name());
		outStream->PutText(s);
		s.FromLong(cur->second.fCount);
		outStream->PutText(L" : ");
		outStream->PutText(s);
		s.FromLong8(cur->second.fTotalSize);
		outStream->PutText(L"   ,  mem = ");
		outStream->PutText(s);
		outStream->PutText(L"\n");
	}
	outStream->PutText(L"\n\n");


	outStream->PutText(L"Blocks by Tags");
	outStream->PutText(L"\n\n");
	for (VMapOfBlockInfo::iterator cur = fBlockInfo.begin(), end = fBlockInfo.end(); cur != end; cur++)
	{
		sLONG tag = cur->first;
		xbox::VStr<80> s;
		s.FromBlock(&tag, 4, VTC_StdLib_char);
		outStream->PutText(s);
		s.FromLong(cur->second.fCount);
		outStream->PutText(L" : ");
		outStream->PutText(s);
		s.FromLong8(cur->second.fTotalSize);
		outStream->PutText(L"   ,  mem = ");
		outStream->PutText(s);
		outStream->PutText(L"\n");
	}


	return err;
}

void VMemStats::Dump()
{
	xbox::VStr<80> s;

	DebugMsg(L"\n");

	DebugMsg(L"Used Mem = ");
	s.FromLong8(fUsedMem);
	DebugMsg(s);
	DebugMsg(L"\n");

	DebugMsg(L"Free Mem = ");
	s.FromLong8(fFreeMem);
	DebugMsg(s);
	DebugMsg(L"\n");

	DebugMsg(L"Nb Big Blocks = ");
	s.FromLong(fNbBigBlocks);
	DebugMsg(s);
	DebugMsg(L"\n");

	DebugMsg(L"Nb Used Big Blocks = ");
	s.FromLong(fNbBigBlocksUsed);
	DebugMsg(s);
	DebugMsg(L"\n");

	DebugMsg(L"Nb Free Big Blocks = ");
	s.FromLong(fNbBigBlocksFree);
	DebugMsg(s);
	DebugMsg(L"\n");

	DebugMsg(L"Nb Pages containing small blocks = ");
	s.FromLong(fNbPages);
	DebugMsg(s);
	DebugMsg(L"\n");

	DebugMsg(L"Nb Small Blocks = ");
	s.FromLong(fNbSmallBlocks);
	DebugMsg(s);
	DebugMsg(L"\n");

	DebugMsg(L"Nb Used Small Blocks = ");
	s.FromLong(fNbSmallBlocksUsed);
	DebugMsg(s);
	DebugMsg(L"\n");

	DebugMsg(L"Nb Free Small Blocks = ");
	s.FromLong(fNbSmallBlocksFree);
	DebugMsg(s);
	DebugMsg(L"\n");

	DebugMsg(L"Biggest Block = ");
	s.FromLong8(fBiggestBlock);
	DebugMsg(s);
	DebugMsg(L"\n");

	DebugMsg(L"Biggest Free Block = ");
	s.FromLong8(fBiggestBlockFree);
	DebugMsg(s);
	DebugMsg(L"\n");

	DebugMsg(L"Nb Objects = ");
	s.FromLong(fNbObjects);
	DebugMsg(s);
	DebugMsg(L"\n");

	DebugMsg(L"\n");
	for_each(fObjectInfo.begin(), fObjectInfo.end(), DumpObjectInfo);

	DebugMsg(L"\n");
	DebugMsg(L"\n");

	DebugMsg(L"Blocks by Tags");
	DebugMsg(L"\n");
	for_each(fBlockInfo.begin(), fBlockInfo.end(), DumpBlockInfo);
	DebugMsg(L"\n");


	DebugMsg(L"\n");
	DebugMsg(L"\n");

	DebugMsg(L"Small Blocks");
	DebugMsg(L"\n");
	for_each(fSmallBlockInfo.begin(), fSmallBlockInfo.end(), DumpMemBlockInfo);
	DebugMsg(L"\n");


	DebugMsg(L"\n");
	DebugMsg(L"\n");

	DebugMsg(L"Allocation Pages");
	DebugMsg(L"\n");
	for_each(fBigBlockInfo.begin(), fBigBlockInfo.end(), DumpAllocPageInfo);
	DebugMsg(L"\n");


	DebugMsg(L"\n");
	DebugMsg(L"\n");

	DebugMsg(L"Other Big Blocks");
	DebugMsg(L"\n");
	for_each(fOtherBlockInfo.begin(), fOtherBlockInfo.end(), DumpMemBlockInfo);
	DebugMsg(L"\n");
}

void VMemStats::Clear()
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
}


// ----------------------------------------


VCppMemMgr::VCppMemMgr( EAllocatorKind inKind)
{
#if VERSIONDEBUG
	_Init( inKind, false, true);
#else
	_Init( inKind, false, false);
#endif
}


VCppMemMgr::VCppMemMgr( EAllocatorKind inKind, bool inWithDebugInfo, bool inWithStrangeFill)
{
	_Init( inKind, inWithDebugInfo, inWithStrangeFill);
}


void VCppMemMgr::_Init( EAllocatorKind inKind, bool inWithDebugInfo, bool inWithStrangeFill)
{
	fWaitBeforeNewPtr = NULL;
	fWaitBeforeNewPtrStarter = 0;
	fPurgeProc = NULL;
	fDebugCheck = 0;
	fInfoFirstFree = NULL;

	EAllocatorKind kind;
	if (inKind == kAllocator_default)
	{
#if VERSIONMAC
		CFStringRef value = (CFStringRef) ::CFBundleGetValueForInfoDictionaryKey( ::CFBundleGetMainBundle(), CFSTR( "MemoryAllocator"));
		if (value != NULL)
		{
			if (::CFStringCompare( value, CFSTR( "stdlib"), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
				kind = kAllocator_stdlib;
			else if (::CFStringCompare( value, CFSTR( "xbox"), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
				kind = kAllocator_xbox;
			else
				kind = kAllocator_stdlib;
		}
		else
		{
			kind = kAllocator_stdlib;
		}
#else
		kind = kAllocator_stdlib;
#endif
	}
	else
	{
		kind = inKind;
	}

	switch( kind)
	{
		case kAllocator_xbox:
		{
			fMaxMems = 4;
			fMems.reserve(fMaxMems);
			for (sLONG i = 0; i < fMaxMems; i++)
			{
				VMemCppImpl* xm = new VMemCppImpl(i);
				fMems.push_back(xm);
				xm->Init(this);
			}
			fNbMems = fMaxMems;
			fCurrentMem = 0;
			fCurrentlyPurging = -1;
			fCurrentlyPurgingEvent = nil;
			fUseStdLibMgr = false;
			fBiggestAllocationBlock = 0;
			break;
		}
		case kAllocator_stdlib:
		{
			fStdMemMgr = new VMemCppImpl_stdlib;
			fUseStdLibMgr = true;
			fStdMemMgr->Init(this);
			break;
		}
		case kAllocator_default:
			break;
	}

	// these two flags cannot be changed while the mgr is running
	fWithDebugInfo = inWithDebugInfo;
	fWithStrangeFill = inWithStrangeFill;

	#if WITH_NEW_XTOOLBOX_GETOPT
	if (VProcess::IsMemoryDebugEnabled()) // reminder
	{}
	#else
	sLONG val;
	if (VProcess::GetCommandLineArgumentAsLong("-memDebugInfo", &val))
		fWithDebugInfo = (val != 0);

	if (VProcess::GetCommandLineArgumentAsLong("-memDebugFill", &val))
		fWithStrangeFill = (val != 0);
	#endif

	fMaxVirtualAllocatedSize = (VSize) MaxLongInt;
	fCurrentVirtualAllocatedSize = 0;
}


VCppMemMgr::~VCppMemMgr()
{
	CheckNow();
	if (fUseStdLibMgr)
	{
		delete fStdMemMgr;
	}
	else
	{
		for (memarray::iterator cur = fMems.begin(), end = fMems.end(); cur != end; cur++)
			delete *cur;
	}
}

#if WITH_ASSERT
bool debug_DisplayStatMem = false;
#endif

#define  withWaitBeforeEvent 0

VCriticalSection gDumpMemMutex;


void* VCppMemMgr::TryToMalloc(VSize inNbBytes, bool inIsVObject, sLONG inTag, sLONG preferedBlock)
{
	void* result = NULL;
	if (preferedBlock >= 0 && preferedBlock < fNbMems)
		result = fMems[preferedBlock]->Malloc(inNbBytes, false, inIsVObject, inTag);
	if (result == NULL)
	{
		sLONG curmem = fCurrentMem;
		do
		{
			if (curmem != fCurrentlyPurging && curmem != preferedBlock)
				result = fMems[curmem]->Malloc(inNbBytes, false, inIsVObject, inTag);
			++curmem;
			if (curmem >= fNbMems)
			{
				curmem = 0;
			}
		} while (curmem != fCurrentMem && result == NULL);
	}
	return result;
}

#define CHECKPURGE 0

void VCppMemMgr::PurgeMem(sLONG whatBlock)
{
	if (!fUseStdLibMgr)
	{
		fMgrMutex.Lock();

		sLONG curmem = whatBlock;
		if (whatBlock == -1)
		{
			curmem = 0;
		}

		bool again = false;
#if VERSIONDEBUG
		VTaskID curtaskid = VTask::GetCurrentID();
#endif
		do
		{
			again = false;
			if (fCurrentlyPurging != -1)
			{
				VSyncEvent* ev = fCurrentlyPurgingEvent;
				xbox_assert(ev != nil);
#if VERSIONDEBUG && CHECKPURGE
				DebugMsg("thread : %d, waiting event, currently purging : %d, event : %p \n", curtaskid, fCurrentlyPurging, fCurrentlyPurgingEvent);
#endif
				ev->Retain();
				fMgrMutex.Unlock();

				ev->Lock();
				ev->Release();
				fMgrMutex.Lock();
#if VERSIONDEBUG && CHECKPURGE
				DebugMsg("thread : %d, stopped waiting event, currently purging : %d, event : %p \n", curtaskid, fCurrentlyPurging, ev);
#endif
				again = true;
			}
			else
			{
				fCurrentlyPurgingEvent = new VSyncEvent;
				fCurrentlyPurging = curmem;
#if VERSIONDEBUG && CHECKPURGE
				DebugMsg("thread : %d, initiated purgemem, currently purging : %d, event : %p \n", curtaskid, fCurrentlyPurging, fCurrentlyPurgingEvent);
#endif
				fMgrMutex.Unlock();

				if (fPurgeProc != NULL)
				{
					fPurgeProc(whatBlock, 0, true);
				}

				fMgrMutex.Lock();
#if VERSIONDEBUG && CHECKPURGE
				DebugMsg("thread : %d, end purgemem, currently purging : %d, event : %p \n", curtaskid, fCurrentlyPurging, fCurrentlyPurgingEvent);
#endif
				fCurrentlyPurgingEvent->Unlock();
				fCurrentlyPurgingEvent->Release();
				fCurrentlyPurging = -1;

			}
		} while (again);

		fMgrMutex.Unlock();
	}

}


void* VCppMemMgr::Malloc(VSize inNbBytes, bool inIsVObject, sLONG inTag, sLONG preferedBlock)
{
	void* result = NULL;
	if (fUseStdLibMgr)
		result = fStdMemMgr->Malloc(inNbBytes, false, inIsVObject, inTag);
	else
	{
		fMgrMutex.Lock();

		Check();

		if (inNbBytes > fBiggestAllocationBlock)
		{
			fMgrMutex.Unlock();
			return NULL;
		}

#if VERSIONDEBUG
		VTaskID curtaskid = VTask::GetCurrentID();
#endif

		VSize size = inNbBytes;
		sLONG startmem = fCurrentMem;
		bool flushToDisk = true;
		do
		{
			result = TryToMalloc(size, inIsVObject, inTag, preferedBlock);
			if (result == NULL)
			{
				preferedBlock = -1;
#if WITH_ASSERT
				if (debug_DisplayStatMem)
				{
					VMemStats stats;
					GetStatistics(stats);
					stats.Dump();
				}
#endif

				if (fCurrentlyPurging != -1)
				{
					VSyncEvent* ev = fCurrentlyPurgingEvent;
					xbox_assert(ev != nil);
#if VERSIONDEBUG && CHECKPURGE
					DebugMsg("thread : %d, waiting event, currently purging : %d, event : %p \n", curtaskid, fCurrentlyPurging, fCurrentlyPurgingEvent);
#endif
					ev->Retain();
					fMgrMutex.Unlock();

					ev->Lock();
					ev->Release();
					fMgrMutex.Lock();
#if VERSIONDEBUG && CHECKPURGE
					DebugMsg("thread : %d, stopped waiting event, currently purging : %d, event : %p \n", curtaskid, fCurrentlyPurging, ev);
#endif
				}
				else
				{
					fCurrentlyPurgingEvent = new VSyncEvent;
					fCurrentlyPurging = fCurrentMem;
#if VERSIONDEBUG && CHECKPURGE
					DebugMsg("thread : %d, initiated purgemem, currently purging : %d, event : %p \n", curtaskid, fCurrentlyPurging, fCurrentlyPurgingEvent);
#endif
					fMgrMutex.Unlock();

					if (fPurgeProc != NULL)
					{
						fPurgeProc(fCurrentMem, size, flushToDisk);
						flushToDisk = false;
					}

					fMgrMutex.Lock();
#if VERSIONDEBUG && CHECKPURGE
					DebugMsg("thread : %d, end purgemem, currently purging : %d, event : %p \n", curtaskid, fCurrentlyPurging, fCurrentlyPurgingEvent);
#endif
					fCurrentlyPurgingEvent->Unlock();
					fCurrentlyPurgingEvent->Release();
					fCurrentlyPurging = -1;

					++fCurrentMem;
					if (fCurrentMem >= fNbMems)
					{
						fCurrentMem = 0;
					}

					result = TryToMalloc(size, inIsVObject, inTag, preferedBlock);
				}
			}
		} while (result == NULL && fCurrentMem != startmem);
		fMgrMutex.Unlock();
	}

	return result;
}


#if oldMemMgr
void* VCppMemMgr::Malloc(VSize inNbBytes, bool inIsVObject, sLONG inTag)
{
#if VERSIONDEBUG_PROFILE
	sLONG8 ticks, ticksoutside = 0;
	VSystem::GetProfilingCounter(ticks);
#endif
	assert(inNbBytes >= 0);

	Check();

	if (inNbBytes < 0)
		return NULL;

#if withWaitBeforeEvent
	if (fPurgeProc != NULL)
	{
		sLONG starter;
		VSyncEvent* wait = NULL;
		{
			VTaskLock lock(&fWaitBeforeNewPtrMutex);
			wait = fWaitBeforeNewPtr;
			starter = fWaitBeforeNewPtrStarter;
			if (wait != NULL)
			{
				wait->Retain();
			}
		}
		if (wait != NULL)
		{
			if (VTaskMgr::Get()->GetCurrentTaskID() != starter)
			{
				wait->Lock();
			}
			wait->Release();
		}
	}
#endif

	VSize size = inNbBytes;

#if VERSIONDEBUG
	if (fWithDebugInfo)
		size += sizeof(DebugBlockHeader) + 4; // DebugBlockHeader before the block and a 4 bytes marker at its end
#endif

#if WITH_PURGE_TRIGGER
	static bool sPurge_state = false;
	bool forcePurge = false;

	if (fPurgeProc != NULL)
	{
		#if VERSIONMAC
		bool trigger = (::GetCurrentKeyModifiers() & optionKey) != 0;
		#elif VERSIONWIN
		bool trigger = ::GetKeyState(VK_MENU) < 0;
		#endif
		if ( sPurge_state && !trigger)
		{
			sPurge_state = false;
		}
		else if (!sPurge_state && trigger)
		{
			sPurge_state = true;
			forcePurge = true;
		}
	}
#endif
	void* newalloc = NULL;
	bool memAlreadyDumped = false;

	{
		bool shoudbreak = false;
		sLONG purgecount = 0;
		bool mustreleaseflag = false;
		do {

	#if WITH_PURGE_TRIGGER
			if (!forcePurge)
				newalloc = fMemMgr->Malloc(size, false, inIsVObject);
			else
				forcePurge = false;
	#else
			newalloc = fMemMgr->Malloc(size, false, inIsVObject, inTag);
	#endif

			if (newalloc != NULL || fPurgeProc == NULL)
				break;

	#if VERSIONDEBUG_PROFILE
			sLONG8 outticks;
			VSystem::GetProfilingCounter(outticks);
	#endif

	#if WITH_ASSERT
			fprintf( stdout, "purging memory for %ld ...", size);
	#endif

			VSize purged = fPurgeProc(size);
			purgecount++;
#if VERSIONWIN && WITH_ASSERT
			if (purgecount > 300 && !memAlreadyDumped)
			{
				VTaskLock lock(&gDumpMemMutex);
				memAlreadyDumped = true;
				VFile dumpfile(L"C:\\MemoryDump4D.txt");
				if (dumpfile.Exists())
					dumpfile.Delete();
				dumpfile.Create();
				{
					VFileStream dumpstream(&dumpfile);
					dumpstream.OpenWriting();
					VMemStats stats;
					fMemMgr->GetStats(stats);
					stats.DumpToStream(&dumpstream);
					dumpstream.CloseWriting();
				}
			}
#endif
			shoudbreak = (purged == 0 || inTag == -2 || (purgecount > 5 && inTag == -3));

	#if WITH_ASSERT
			fprintf( stdout, "done, result = %ld.\n", purged);
	#endif

	#if VERSIONDEBUG_PROFILE
			sLONG8 outticks2;
			VSystem::GetProfilingCounter(outticks2);
			ticksoutside = ticksoutside + (outticks2-outticks);
	#endif
			if (shoudbreak)
				break;
			else
			{
#if withWaitBeforeEvent
				if (purgecount > 40)
				{
					if (mustreleaseflag)
					{
						mustreleaseflag = false;
						VTaskLock lock3(&fWaitBeforeNewPtrMutex);
						assert(fWaitBeforeNewPtr != NULL);
						if (fWaitBeforeNewPtr != NULL)
						{
							fWaitBeforeNewPtr->Unlock();
							fWaitBeforeNewPtr->Release();
							fWaitBeforeNewPtr = NULL;
						}
					}
				}
				else
				{
					if (purgecount > 20 && !mustreleaseflag)
					{
						mustreleaseflag = true;
						{
							VTaskLock lock2(&fWaitBeforeNewPtrMutex);
							if (fWaitBeforeNewPtr == NULL)
							{
								fWaitBeforeNewPtr = new VSyncEvent;
								fWaitBeforeNewPtrStarter = VTaskMgr::Get()->GetCurrentTaskID();
							}
							else
							{
								// should not be here
								mustreleaseflag = mustreleaseflag; // put a breakpoint here
							}
						}
					}
				}
#endif
			}

		} while(true);

	#if WITH_ASSERT
		if (newalloc == NULL)
		{
			newalloc = newalloc;
		}
	#endif

#if withWaitBeforeEvent
		if (mustreleaseflag)
		{
			VTaskLock lock3(&fWaitBeforeNewPtrMutex);
			assert(fWaitBeforeNewPtr != NULL);
			if (fWaitBeforeNewPtr != NULL)
			{
				fWaitBeforeNewPtr->Unlock();
				fWaitBeforeNewPtr->Release();
				fWaitBeforeNewPtr = NULL;
			}
		}
#endif

	}

#if WITH_ASSERT
	if (newalloc == NULL)
	{
		if (debug_DisplayStatMem)
		{
			VMemStats stats;
			fMemMgr->GetStats(stats);
			stats.Dump();
		}
	}
#endif

	if (newalloc != NULL)
	{
		if (inIsVObject)
		{
			xbox_assert( inNbBytes >= sizeof(void*) );
			// clear vtbl not to crash in type_info
			*(void**)newalloc = NULL;
		}

#if VERSIONDEBUG
		if (fWithDebugInfo)
		{
			VSize allocatedSize = fMemMgr->GetPtrSize(newalloc);	// allocater may provide more memory than asked
			assert(allocatedSize >= size);
			assert(size >= sizeof(DebugBlockHeader) + 4);
			DebugBlockHeader*	block = reinterpret_cast<DebugBlockHeader*>(newalloc);
			RegisterBlock(block, inNbBytes, inIsVObject);
			newalloc = (VPtr) block + 1;
		}

		if (fWithStrangeFill)
			::memset(newalloc, 0x63, inNbBytes);
#endif
	}

#if VERSIONDEBUG_PROFILE
	sLONG8 ticks2;
	VSystem::GetProfilingCounter(ticks2);
	VSystem::debug_AddToProfilerCount(0, ticks2-ticks - ticksoutside);
	VSystem::debug_AddToProfilerCount(1, ticksoutside);
#endif

	return newalloc;
}

#endif

void* VCppMemMgr::Calloc(VIndex inNbElem, VSize inNbBytesPerElem, sLONG inTag)
{
	// same as stdlib's calloc
	// may be optimized to avoid calling memset on big VM allocations
	void* inAddr = Malloc(inNbElem * inNbBytesPerElem, false, inTag);
	if (inAddr != NULL)
		::memset(inAddr, 0, inNbElem * inNbBytesPerElem);

	return inAddr;
}


void VCppMemMgr::Free(void* ioPtr)
{
#if VERSIONDEBUG_PROFILE
	sLONG8 ticks;
	VSystem::GetProfilingCounter(ticks);
#endif

	if (fUseStdLibMgr)
		fStdMemMgr->Free(ioPtr);
	else
	{
		VKernelTaskLock lock(&fMgrMutex);
		Check();

		if (ioPtr != NULL)
		{
			if (CheckPtr(ioPtr))
			{
			#if VERSIONDEBUG
				XMemCppImpl* memimpl = GetMemMgrImpl();
				if (fWithDebugInfo)
				{
					DebugBlockHeader *block = reinterpret_cast<DebugBlockHeader*>(ioPtr) - 1;
					UnregisterBlock(block);
					if (fWithStrangeFill)
						::memset(block, 0x42, memimpl->GetPtrSize(block));
					memimpl->Free(block);
				}
				else if (fWithStrangeFill)
				{
					::memset(ioPtr, 0x42, memimpl->GetPtrSize(ioPtr));
				}
			#endif

				fMems[0]->Free(ioPtr);
			}
		}
	}
#if VERSIONDEBUG_PROFILE
	sLONG8 ticks2;
	VSystem::GetProfilingCounter(ticks2);
	VSystem::debug_AddToProfilerCount(0, ticks2-ticks);
#endif
}


void VCppMemMgr::RegisterUser(void* /*inInstance*/)
{
	CheckNow();
}


void VCppMemMgr::UnregisterUser(void* /*inInstance*/)
{
	CheckNow();
}



bool VCppMemMgr::IsAPtr(void* /*inPtr*/, size_t /*inSize*/)
{
	return true;
}


bool VCppMemMgr::Check()
{
	bool isOK = true;

#if VERSIONDEBUG || USE_MEMORY_CHECK_FOR_RELEASE_CODE
	if (fDebugCheck > 0)
	{
		XMemCppImpl* memimpl = GetMemMgrImpl();
		isOK = memimpl->Check() != 0;
	}
#endif

	return isOK;
}


bool VCppMemMgr::CheckNow()
{
#if VERSIONDEBUG || USE_MEMORY_CHECK_FOR_RELEASE_CODE
	VInterlocked::Increment(&fDebugCheck);
	bool	isOK = Check();
	VInterlocked::Decrement(&fDebugCheck);
	return isOK;
#else
	return true;
#endif
}


bool VCppMemMgr::CheckPtr(const void* ioPtr)
{
	bool isOK = true;

#if VERSIONDEBUG
	if (fWithDebugInfo)
	{
		const DebugBlockHeader *block = reinterpret_cast<const DebugBlockHeader*>(ioPtr) - 1;

		isOK = testAssert(block->fMarker == 0xdeadbeef);
		if (isOK && (block->fDebugInfo != NULL))
			isOK = block->fDebugInfo->Check();

		if (isOK && (block->fDebugInfo != NULL))
		{
			VSize userSize = block->fDebugInfo->GetUserSize();
			isOK = testAssert(*((uLONG *) ((char *) (block + 1) + userSize)) == 0xdeadbeef);
		}
	}
#endif


#if VERSIONDEBUG || USE_MEMORY_CHECK_FOR_RELEASE_CODE
	//if (fDebugCheck > 0 && isOK)
	{
		XMemCppImpl* memimpl = GetMemMgrImpl();
		isOK = memimpl->CheckPtr(ioPtr) != 0;
	}
#endif

	return isOK;
}


void VCppMemMgr::RegisterBlock(DebugBlockHeader *inAddr, VSize inUserSize, bool inIsVObject)
{
#if VERSIONDEBUG
	VDebugBlockInfo *info = NULL;

	// critical section for the info pool
	{
		StLocker<VSystemCriticalSection> locker(&fInfoLock);

		// find free slot
		if (fInfoFirstFree == NULL)
		{
			// allocate new pool
			VDebugBlockInfoPool *pool;
			do
			{
				pool = new VDebugBlockInfoPool(fInfoFirstFree);
				if ((pool != NULL) || fPurgeProc == NULL)
					break;
				if (fPurgeProc(0, VCppMemMgr::kSizeOfDebugBlockInfoPool, false) == 0)
					break;
			} while(true);

			if (pool != NULL)
			{
				fInfoPool.AddHead(pool);
				fCurrentVirtualAllocatedSize += VCppMemMgr::kSizeOfDebugBlockInfoPool;
			}
		}

		info = fInfoFirstFree;
		if (info != NULL)
		{
			fInfoFirstFree = info->GetNextFree();
			info->SetNextFree(NULL);
		}
	}

	if (info != NULL)
	{
		info->Use(inAddr + 1, inUserSize, inIsVObject);
		fLeaksChecker.RegisterBlock(info->GetStackCrawl());
	}

	inAddr->fDebugInfo = info;
	inAddr->fMarker = 0xdeadbeef;
	*((uLONG *) ((char *) (inAddr + 1) + inUserSize)) = 0xdeadbeef;
#endif
}


void VCppMemMgr::UnregisterBlock(DebugBlockHeader *inAddr)
{
#if VERSIONDEBUG
	VDebugBlockInfo *info = inAddr->fDebugInfo;
	if (info != NULL)
	{
		assert(info->IsBlockUsed());

		fLeaksChecker.UnregisterBlock(info->GetStackCrawl());

		info->Free();

		assert(info->GetNextFree() == NULL);

		// critical section for the info pool
		{
			StLocker<VSystemCriticalSection> locker(&fInfoLock);

			info->SetNextFree(fInfoFirstFree);
			fInfoFirstFree = info;
		}
	}
#endif
}


bool VCppMemMgr::Walk(IMemoryWalker& inWalker)
{
#if VERSIONDEBUG
	bool isOK = true;

	if (!fWithDebugInfo)
		return false;

	for (VDebugBlockInfoPool *pool = fInfoPool.GetFirst() ; (pool != NULL) && isOK ; pool = pool->GetNext())
	{
		for (sLONG i = 1 ; (i <= VDebugBlockInfoPool::kBlocksPerPool) && isOK ; ++i)
		{
			VDebugBlockInfo& block = pool->GetNthBlock(i);
			if (block.IsBlockUsed())
				isOK = inWalker.Walk(block);
		}
	}

	return isOK;
#else
	return false;
#endif
}


VSize VCppMemMgr::GetPtrSize(const VPtr ioPtr)
{
	if (!Check())
		return 0;

	if (!CheckPtr(ioPtr))
		return 0;

	VSize	size;

#if VERSIONDEBUG
	if (fWithDebugInfo)
	{
		XMemCppImpl* memimpl = GetMemMgrImpl();
		const DebugBlockHeader *block = reinterpret_cast<const DebugBlockHeader*>(ioPtr) - 1;
		size = memimpl->GetPtrSize(block) - (sizeof(DebugBlockHeader) + 4);
	}
	else
	{
		if (fUseStdLibMgr)
			size = fStdMemMgr->GetPtrSize(ioPtr);
		else
			size = fMems[0]->GetPtrSize(ioPtr);
	}
#else
	if (fUseStdLibMgr)
		size = fStdMemMgr->GetPtrSize(ioPtr);
	else
	{
		VKernelTaskLock lock(&fMgrMutex);
		size = fMems[0]->GetPtrSize(ioPtr);
	}
#endif

	return size;
}


void* VCppMemMgr::Realloc(void* ioPtr, VSize inNbBytes)
{
	void* newData = NULL;

	Check();

	// simple new/copy/delete implementation
	// same behavior as stdlib::realloc

	if (ioPtr == NULL)
	{
		newData = Malloc(inNbBytes, false, 0);
	}
	else if (inNbBytes == 0)
	{
		Free(ioPtr);
	}
	else
	{
		if (CheckPtr(ioPtr))
		{
		#if VERSIONDEBUG
			if (fWithDebugInfo)
			{
				XMemCppImpl* memimpl = GetMemMgrImpl();
				DebugBlockHeader *block = reinterpret_cast<DebugBlockHeader*>(ioPtr) -1;
				VSize oldSize = memimpl->GetPtrSize(block);
				VSize newSize = inNbBytes + sizeof(DebugBlockHeader) + 4;
				DebugBlockHeader *newBlock = reinterpret_cast<DebugBlockHeader*>(memimpl->Realloc(block, newSize));
				if (newBlock != NULL) {
					VSize allocatedSize = memimpl->GetPtrSize(newBlock);	// allocater may provide more memory than asked
					assert(allocatedSize >= newSize);

					// update user size
					if (newBlock->fDebugInfo != NULL)
						newBlock->fDebugInfo->SetUserSize(inNbBytes);

					if (fWithStrangeFill && (allocatedSize > oldSize))
						::memset((char *) newBlock + oldSize, 0x63, allocatedSize - oldSize);

					// end marker
					*((uLONG *) ((char *) (newBlock + 1) + inNbBytes)) = 0xdeadbeef;
					newData = (VPtr)newBlock + 1;
				}
			}
			else if (fWithStrangeFill)
			{
				XMemCppImpl* memimpl = GetMemMgrImpl();
				newData = Malloc(inNbBytes, false, 'grow');
				if (newData != NULL)
				{
					VSize size = memimpl->GetPtrSize(ioPtr);
					if (size > inNbBytes)
						size = inNbBytes;
					CopyBlock(ioPtr, newData, size);
					Free(ioPtr);

					VSize allocatedSize = memimpl->GetPtrSize(newData); // allocater may provide more memory than asked
					if (allocatedSize > size)
						::memset((char *) newData + size, 0x63, allocatedSize - size);
				}
			}
			else
			{
				XMemCppImpl* memimpl = GetMemMgrImpl();
				newData = Malloc(inNbBytes, false, 'grow');
				if (newData != NULL)
				{
					VSize size = memimpl->GetPtrSize(ioPtr);
					if (size > inNbBytes)
						size = inNbBytes;
					CopyBlock(ioPtr, newData, size);
					Free(ioPtr);
				}
			}
		#else
			XMemCppImpl* memimpl = GetMemMgrImpl();
			newData = Malloc(inNbBytes, false, 'grow');
			if (newData != NULL)
			{
				VSize size = memimpl->GetPtrSize(ioPtr);
				if (size > inNbBytes)
					size = inNbBytes;
				CopyBlock(ioPtr, newData, size);
				Free(ioPtr);
			}
		#endif
		}
	}

	return newData;
}


void* VCppMemMgr::operator new(size_t inSize)
{
	size_t allocatedSize = inSize + sizeof(VSize);
	VSize *addr = reinterpret_cast<VSize*>(VSystem::VirtualAlloc(allocatedSize, NULL));
	if (addr != NULL)
		*addr++ = allocatedSize;
	return addr;
}


void VCppMemMgr::operator delete (void* inAddr)
{
	VSize *addr = reinterpret_cast<VSize*>(inAddr) - 1;
	VSystem::VirtualFree(addr, *addr, false);
}


void VCppMemMgr::DumpLeaks()
{
#if VERSIONDEBUG
	if (fWithDebugInfo)
	{
		VDumpLeaksWalker walker(&fLeaksChecker);
		Walk(walker);
	}
#endif
}


void VCppMemMgr::GetUsage(VArrayString& inTabClassNames, VArrayLong& inTabNbObjects, VArrayLong& inTabTotalSize)
{
#if VERSIONDEBUG
	if (fWithDebugInfo)
	{
		VGetUsageWalker walker(&inTabClassNames, &inTabNbObjects, &inTabTotalSize);
		Walk(walker);

		// on rajoute les debug pools
		sLONG poolCount = fInfoPool.GetCount();
		sLONG poolSize = poolCount * VCppMemMgr::kSizeOfDebugBlockInfoPool;

		inTabClassNames.AppendString(CVSTR("__debug_info_pool__"));
		inTabNbObjects.AppendLong(poolCount);
		inTabTotalSize.AppendLong(poolSize);
	}
#endif
}


VSize VCppMemMgr::GetFreeMemory()
{
	VSize	nbHeaps;
	VSize	nbBlocs;
	VSize	totalAlloc;
	VSize	nbUsed;
	VSize	totalUsed;
	VSize	maxUsed;
	VSize	nbFree;
	VSize	totalFree;
	VSize	maxFree;

	GetMemStatistics (nbHeaps, nbBlocs, totalAlloc, nbUsed, totalUsed, maxUsed, nbFree, totalFree, maxFree);
	return totalFree;
}


VSize VCppMemMgr::GetMaxFree()
{
	VSize	nbHeaps;
	VSize	nbBlocs;
	VSize	totalAlloc;
	VSize	nbUsed;
	VSize	totalUsed;
	VSize	maxUsed;
	VSize	nbFree;
	VSize	totalFree;
	VSize	maxFree;

	GetMemStatistics (nbHeaps, nbBlocs, totalAlloc, nbUsed, totalUsed, maxUsed, nbFree, totalFree, maxFree);
	return maxFree;
}


VSize VCppMemMgr::GetMaxUsed()
{

	VSize	nbHeaps;
	VSize	nbBlocs;
	VSize	totalAlloc;
	VSize	nbUsed;
	VSize	totalUsed;
	VSize	maxUsed;
	VSize	nbFree;
	VSize	totalFree;
	VSize	maxFree;

	GetMemStatistics (nbHeaps, nbBlocs, totalAlloc, nbUsed, totalUsed, maxUsed, nbFree, totalFree, maxFree);
	return maxUsed;

}


VSize VCppMemMgr::GetUsedMem()
{
	VSize	nbHeaps;
	VSize	nbBlocs;
	VSize	totalAlloc;
	VSize	nbUsed;
	VSize	totalUsed;
	VSize	maxUsed;
	VSize	nbFree;
	VSize	totalFree;
	VSize	maxFree;

	GetMemStatistics (nbHeaps, nbBlocs, totalAlloc, nbUsed, totalUsed, maxUsed, nbFree, totalFree, maxFree);
	return totalUsed;
}


VSize VCppMemMgr::GetAllocatedMem()
{
	VKernelTaskLock lock(&fMgrMutex);
	/*
	VSize	nbHeaps;
	VSize	nbBlocs;
	VSize	totalAlloc;
	VSize	nbUsed;
	VSize	totalUsed;
	VSize	maxUsed;
	VSize	nbFree;
	VSize	totalFree;
	VSize	maxFree;

	GetMemStatistics (nbHeaps, nbBlocs, totalAlloc, nbUsed, totalUsed, maxUsed, nbFree, totalFree, maxFree);
	return totalAlloc;
	*/
	if (fUseStdLibMgr)
		return fStdMemMgr->GetTotalAllocation();
	else
	{
		VSize result = 0;
		for (memarray::iterator cur = fMems.begin(), end = fMems.end(); cur != end; cur++)
		{
			result += (*cur)->GetTotalAllocation();
		}
		return result;
	}
}


void VCppMemMgr::GetMemUsageInfo(VSize& outTotalMem, VSize& outUsedMem)
{
	if (fUseStdLibMgr)
		fStdMemMgr->GetMemUsageInfo(outTotalMem, outUsedMem);
	else
	{
		outTotalMem = 0;
		outUsedMem = 0;

		for (memarray::iterator cur = fMems.begin(), end = fMems.end(); cur != end; cur++)
		{
			VSize totmem, usedmem;
			(*cur)->GetMemUsageInfo(totmem, usedmem);
			outTotalMem += totmem;
			outUsedMem += usedmem;
		}
	}
}


void VCppMemMgr::GetStatistics(VMemStats& outStats, sLONG blockNumber)
{
	VKernelTaskLock lock(&fMgrMutex);
	outStats.Clear();
	if (fUseStdLibMgr)
		return fStdMemMgr->GetStats(outStats, 0);
	else
	{
		if (blockNumber >= 0 && blockNumber <fNbMems)
			fMems[blockNumber]->GetStats(outStats, blockNumber);
		else
		{
			sWORD blocknum = 0;
			for (memarray::iterator cur = fMems.begin(), end = fMems.end(); cur != end; cur++, blocknum++)
			{
				(*cur)->GetStats(outStats, blocknum);
			}
		}
	}
}

void VCppMemMgr::GetMemStatistics(VSize& outNbHeaps, VSize& outNbBlocs, VSize& outTotalAlloc, VSize& outNbUsed, VSize& outTotalUsed, VSize& outMaxUsed, VSize& outNbFree, VSize& outTotalFree, VSize& outMaxFree)
{
	// TBD
	outNbHeaps = 0;
	outNbBlocs = 0;
	outTotalAlloc = GetAllocatedMem();
	outNbUsed = 0;
	outTotalUsed = 0;
	outMaxUsed = 0;
	outNbFree = 0;
	outTotalFree = 0;
	outMaxFree = 0;
}

#if 0
void* VCppMemMgr::DoVirtualAlloc(VSize inNbBytes)
{
	if (fCurrentVirtualAllocatedSize + inNbBytes > fMaxVirtualAllocatedSize)
		return NULL;

	void* buff = VSystem::VirtualAlloc(inNbBytes);
	if (buff != NULL && fPhysicalMemOnly)
	{
		if (! VSystem::VirtualLock(buff, inNbBytes))
		{
			VSystem::VirtualFree(buff, inNbBytes);
			buff = NULL;
		}
	}

	if (buff != NULL)
	{
		fCurrentVirtualAllocatedSize += inNbBytes;
	}

	return buff;
}


void VCppMemMgr::DoVirtualFree(void* inAddr, VSize inNbBytes)
{
	fCurrentVirtualAllocatedSize -= inNbBytes;
	if (fPhysicalMemOnly)
		VSystem::VirtualUnlock(inAddr, inNbBytes);
	VSystem::VirtualFree(inAddr, inNbBytes);
}
#endif


sLONG VCppMemMgr::GetAllocationBlockNumber(void* inAddr)
{
	if (fUseStdLibMgr)
	{
		return 0;
	}
	else
	{
		VMemCppImpl* xm = (VMemCppImpl*)fMems[0];
		return xm->GetAllocationBlockNumber(inAddr);
	}
}


struct addrInfo
{
	void* ptr;
	VSize size;
};

bool VCppMemMgr::AddVirtualAllocation( VSize inMaxBytes, const void *inHintAddress, bool inPhysicalMemory)
{
	bool ok = false;
	bool fullOK = true, first = true;

	const VSize kSecurityMemFringe = 50*1024*1024;	// 50 Mo
	const VSize kMinFreeBlockSize = 10*1024*1024;	// 10 Mo

	if (!fUseStdLibMgr)
	{
		#if ARCH_64 || VERSION_LINUX
		ok = true;

		// DH 26-Mar-2013 ACI0074439 : Don't bother allocating tiny blocks of memory
		if (inMaxBytes >= kMinFreeBlockSize)
		{
			VKernelTaskLock lock(&fMgrMutex);
			VSize sizeToAdd = inMaxBytes / fNbMems;
			for (memarray::iterator cur = fMems.begin(), end = fMems.end(); cur != end; cur++)
			{
				VMemCppImpl* xm = (VMemCppImpl*) *cur;
				bool locked = false;
				void *buf;
				if (inPhysicalMemory)
					buf = VSystem::VirtualAllocPhysicalMemory( sizeToAdd, inHintAddress, &locked);
				else
					buf = VSystem::VirtualAlloc( sizeToAdd, inHintAddress);

				if (testAssert( buf != NULL))
				{
					xm->AddAlreadyAllocatedMem( buf, sizeToAdd, inPhysicalMemory);
					if (sizeToAdd > fBiggestAllocationBlock)
						fBiggestAllocationBlock = sizeToAdd;
				}
				else
				{
					ok = false;
					break;
				}
			}
		}
		#else
		VSize totalSizeToAdd = inMaxBytes;

		do
		{
			if (totalSizeToAdd < kSecurityMemFringe)
			{
				if (first)
					ok = true;
				break;
			}

			typedef std::vector<addrInfo> addrvect;

			// get all available VM blocks
			typedef std::multimap<VSize,const void*> MapOfAddressBySize;
			MapOfAddressBySize vm_mapBySize;

#if VERSIONDEBUG
			typedef std::map<const void*, sLONG8> MapOfAddress;
			MapOfAddress vm_all;
#endif

			const void *address = NULL;
			VSize cumulatedFreeVirtualSize = 0, allFreeVirtualSize = 0;

			do {
				VSize size;
				const void *baseAddress;
				VMStatus status;
				if (!VSystem::VirtualQuery( address, &size, &status, &baseAddress))
					break;
#if VERSIONDEBUG
				if (status & VMS_FreeMemory)
				{
					allFreeVirtualSize += size;
					vm_all.insert(MapOfAddress::value_type(baseAddress, size));
				}
				else
				{
					sLONG8 tempsize = size;
					vm_all.insert(MapOfAddress::value_type(baseAddress, -tempsize));
				}
#endif
				if ((status & VMS_FreeMemory) && (size >= kMinFreeBlockSize))
				{
					cumulatedFreeVirtualSize += size;
					vm_mapBySize.insert( MapOfAddressBySize::value_type( size, baseAddress));
				}
				address = (const char*) baseAddress + size;
			} while( true);

			if (totalSizeToAdd > cumulatedFreeVirtualSize)
				totalSizeToAdd = cumulatedFreeVirtualSize - kSecurityMemFringe;

			if (totalSizeToAdd < kSecurityMemFringe)
				break;

			VSize sizeToAdd = totalSizeToAdd / fNbMems;

			std::vector<addrvect> allocated;
			allocated.resize(fNbMems);
			bool toutOk = true;
			for (sLONG i = 0; i < fNbMems; i++)
			{
				bool subok = false;
				VSize subSize = sizeToAdd, cumulSub = 0;

				addrvect& suballocated = allocated[i];
				do
				{
					if (subSize > 0x7FFFFF00)
						subSize = 0x7FFFFF00; // pour l'instant, chaque grand block ne peut pas depasser 2G
					if (suballocated.size() > 20) // si trop de petit block, on arrete
					{
						toutOk = false;
						break;
					}
					VSize trueSubSize = subSize;

					const void* hintAddr = nil;
					MapOfAddressBySize::iterator closest = vm_mapBySize.lower_bound( subSize); // on cherche le block de VM le plus proche en taille, juste un peu plus grand
					if (closest == vm_mapBySize.end())
					{  // si il n'y en a pas, on prend le plus grand des block, puisqu'il sera inferieur la taille voulue
						if (!vm_mapBySize.empty())
						{
							--closest;
							hintAddr = closest->second;
							subSize = closest->first;

						}
					}
					else
					{
						hintAddr = closest->second;
						VSize closestSize = closest->first;
						if (closestSize > subSize + kMinFreeBlockSize)
						{
							vm_mapBySize.erase(closest);
							VSize newsize = closestSize - subSize;
							vm_mapBySize.insert(MapOfAddressBySize::value_type( newsize, (const void*)((char*)hintAddr + subSize) ) );
						}
						else
						{
							subSize = closestSize;
							vm_mapBySize.erase(closest);
						}
					}

					void* buf;
					bool locked = false;
					if (inPhysicalMemory)
						buf = VSystem::VirtualAllocPhysicalMemory( subSize, hintAddr, &locked);
					else
						buf = VSystem::VirtualAlloc( subSize, hintAddr);

					if (buf == nil) // si ca na pas marche, on reesaye n'importe ou
					{
						subSize = trueSubSize;
						if (inPhysicalMemory)
							buf = VSystem::VirtualAllocPhysicalMemory( subSize, nil, &locked);
						else
							buf = VSystem::VirtualAlloc( subSize, nil);
						// on doit repprendre la liste des bloc VM car elle a surement change
						address = nil;
						vm_mapBySize.clear();
						do {
							VSize size;
							const void *baseAddress;
							cumulatedFreeVirtualSize = 0;
							VMStatus status;
							if (!VSystem::VirtualQuery( address, &size, &status, &baseAddress))
								break;
							if ((status & VMS_FreeMemory) && (size >= kMinFreeBlockSize))
							{
								cumulatedFreeVirtualSize += size;
								vm_mapBySize.insert( MapOfAddressBySize::value_type( size, baseAddress));
							}
							address = (const char*) baseAddress + size;
						} while( true);


					}
					if (buf == nil)
					{
						subSize = subSize / 2;
						if (subSize < kMinFreeBlockSize)
						{
							toutOk = false;
							break;
						}
					}
					else
					{
						addrInfo addr;
						addr.size = subSize;
						addr.ptr = buf;
						suballocated.push_back(addr);
						cumulSub += subSize;
						if (cumulSub+kMinFreeBlockSize >= sizeToAdd)
							subok = true;
						else
							subSize = sizeToAdd - cumulSub;
					}
				} while (!subok);
				if (!toutOk)
					break;
			}

			if (toutOk)
			{
				ok = true;
				VKernelTaskLock lock(&fMgrMutex);
				sLONG i = 0;
				for (memarray::iterator cur = fMems.begin(), end = fMems.end(); cur != end; cur++, i++)
				{
					VMemCppImpl* xm = (VMemCppImpl*) *cur;
					addrvect& suballocated = allocated[i];
					for (sLONG j = 0, nb = suballocated.size(); j < nb; j++)
					{
						VSize subSize = suballocated[j].size;
						xm->AddAlreadyAllocatedMem(suballocated[j].ptr, subSize, inPhysicalMemory);
						if (subSize > fBiggestAllocationBlock)
							fBiggestAllocationBlock = subSize;
					}
				}

			}
			else
			{
				// tout n'est pas alloue, on libere ce qui a deja ete alloue et on recommence avec 2 fois moins
				for (sLONG i = 0; i < fNbMems; i++)
				{
					addrvect& suballocated = allocated[i];
					for (sLONG j = 0, nb = suballocated.size(); j < nb; j++)
					{
						void* buf = suballocated[j].ptr;
						if (buf != nil)
						{
							VSystem::VirtualFree(buf, suballocated[j].size, inPhysicalMemory);
						}
					}
				}

				fullOK = false;
				VSize moins = totalSizeToAdd / 3;
				totalSizeToAdd = totalSizeToAdd - moins;
				if (totalSizeToAdd < fNbMems * kMinFreeBlockSize)
					break;
			}
			first = false;
		} while (!ok);
		#endif // ARCH_64
	}

	/*
	if (!fMemMgr->AddAllocation( inMaxBytes, inHintAddress, inPhysicalMemory))
	{
		ok = false;
	}
	else
	{
		if (fMaxVirtualAllocatedSize == (VSize) MaxLongInt)
			fMaxVirtualAllocatedSize = inMaxBytes;
		else
			fMaxVirtualAllocatedSize = fMaxVirtualAllocatedSize + inMaxBytes;
	}
	*/

	#if 0 && VERSION_LINUX && VERSIONDEBUG
	DebugMsg("[%d] VCppMemMgr::AddVirtualAllocation : request %dMB at %p %s %s\n",
		 VTask::GetCurrentID(), (int)(inMaxBytes/(1024*1024)), inHintAddress,
		 inPhysicalMemory ? "in physical memory" : "not in physical memory",
		 fullOK && ok ? "succeed" : "failed");
	#endif

	return fullOK && ok;
}


VIndex VCppMemMgr::CountVirtualAllocations()
{
	if (fUseStdLibMgr)
		return fStdMemMgr->CountAllocations();
	else
	{
		VIndex result = 0;
		for (memarray::iterator cur = fMems.begin(), end = fMems.end(); cur != end; cur++)
		{
			result += (*cur)->CountAllocations();
		}
		return result;

	}
}


PurgeHandlerProc VCppMemMgr::SetPurgeHandlerProc(PurgeHandlerProc inProc)
{
	PurgeHandlerProc old = fPurgeProc;
	fPurgeProc = inProc;
	return old;
}


VMemoryHog* VCppMemMgr::RegisterMemHog()
{
	VMemoryHog* newhog = new VMemoryHog(VTaskMgr::Get()->GetCurrentTaskID());
	if (newhog != NULL)
	{
		VTaskLock lock(&fWaitBeforeNewPtrMutex);
		try
		{
			fMemHogsStack.push_back(newhog);
		}
		catch (...)
		{
			delete newhog;
			newhog = NULL;
		}
	}

	return newhog;
}


void VCppMemMgr::UnRegisterMemHog(VMemoryHog* memhog)
{
	VTaskLock lock(&fWaitBeforeNewPtrMutex);
	VStackOfMemHogs::iterator found = std::find(fMemHogsStack.begin(), fMemHogsStack.end(), memhog);
	if (found != fMemHogsStack.end())
	{
		fMemHogsStack.erase(found);
		delete memhog;
	}
}


void* VCppMemMgr::CheckVObject(const void* ioPtr)
{
	return const_cast<void*>(ioPtr);

#if 0
	XMemCppImpl* memimpl = GetMemMgrImpl();
	if (!fWithDebugInfo)
		return const_cast<void*>(ioPtr);

	bool isOK = false;

	// L.E. 05/05/1999 do some test to check if it's a good candidate for a VObject *
	// maybe on in the heap, on the stack or embedded in another c++ object
	if (ioPtr != NULL)
	{ // on pourrait tester la validite de l'adresse
		const DebugBlockHeader*	block = reinterpret_cast<const DebugBlockHeader*>(ioPtr) -1;
		VPtrKind kind = memimpl->GetPtrKind(block);
		switch(kind)
		{

			case PTR_NOT_IN_CPLUS_HEAP:
				// maybe on the stack ?
				isOK = true;
				break;

			case PTR_IS_CPLUS_BLOCK:
				// should have been marked as a VObject
				isOK = (block->fFlags & kBlockIsVObject) ? true : false;
				break;

			case PTR_IN_CPLUS_BLOCK:
				// maybe inside another object
				isOK = true;
				break;

		}
	}
	return isOK ? const_cast<void*>(ioPtr) : NULL;
#endif
}

END_TOOLBOX_NAMESPACE

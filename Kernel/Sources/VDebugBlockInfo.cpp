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
#include "VDebugBlockInfo.h"
#include "VSystem.h"
#include "VObject.h"

#if VERSIONDEBUG

VDebugBlockInfoPool::VDebugBlockInfoPool(VDebugBlockInfo*& ioFirstFreeBlock)
{
	fNext = NULL;
	fPrevious = NULL;
	
	for (sLONG i = 0 ; i < kBlocksPerPool ; ++i)
	{
		ioFirstFreeBlock = new(&fInfo[i]) VDebugBlockInfo(this, ioFirstFreeBlock);
	}
	fCountFree = kBlocksPerPool;
}


void *VDebugBlockInfoPool::operator new(size_t inSize)
{
	assert(inSize <= 4096L);
	
	size_t allocatedSize = inSize + sizeof(VSize);
	size_t*	addr = reinterpret_cast<size_t*>(VSystem::VirtualAlloc(allocatedSize, NULL));
	if (addr != NULL)
		*addr++ = allocatedSize;
	return addr;
}


void VDebugBlockInfoPool::operator delete(void* inAddress)
{
	VSize*	addr = reinterpret_cast<VSize*>(inAddress) - 1;
	VSystem::VirtualFree(addr, *addr, false);
}


#pragma mark-

#if VERSIONMAC

static volatile bool	sReadFailure = false;
static ExceptionHandlerTPP	sNewHandler = NULL;
static ExceptionHandlerTPP	sOldHandler = NULL;

static OSStatus MyExceptionHandler(ExceptionInformation* exception)
{
	OSStatus err = noErr;
	
	// If this is a memory-related exception(or if there was no
	// previous exception handler), it's ours
	if (sOldHandler == NULL || 
		exception->theKind == kAccessException || 
		exception->theKind == kUnmappedMemoryException || 
		exception->theKind == kExcludedMemoryException) {
		sReadFailure = true;
#if __LP64__
		exception->machineState->RIP += 4;     // skip current instruction
#elif ( __GNUC__ && __i386__ )
		exception->machineState->EIP += 4;     // skip current instruction
#else
		exception->machineState->PC.lo += 4;     // skip current instruction
#endif
	
	// If not, pass it to the previous exception handler, if there 
	// is one. This lets us do things like use the Metrowerks debugger.
	} else
		err = InvokeExceptionHandlerUPP(exception, sOldHandler);
		
	return err;
}
#endif	// VERSIONMAC


void VDebugBlockInfo::Use(void* inUserData, VSize inUserSize, bool inIsVObject)
{	
	static uLONG sTimeStamp = 0;

	assert((fFlags & kBlockIsUsed) == 0);
	fTimeStamp = ++sTimeStamp;
	fUserSize = inUserSize;
	fData = inUserData;
	fFlags = kBlockIsUsed;
	if (inIsVObject)
		fFlags |= kBlockIsVObject;

	fPool->UseOneBlock();
}


void VDebugBlockInfo::Free()
{
	assert(fFlags & kBlockIsUsed);
	fFlags &= ~kBlockIsUsed;

	fPool->FreeOneBlock();
}


bool VDebugBlockInfo::Check() const
{
	bool isOK = testAssert((fFlags |(kBlockIsUsed | kBlockIsVObject)) ==(kBlockIsUsed | kBlockIsVObject));
	if (isOK)
		isOK = isOK && testAssert((fFlags & kBlockIsUsed) &&(fNextFree == NULL));
	if (isOK)
		isOK = isOK && testAssert(fUserSize >= 0);
	if (isOK)
		isOK = isOK && testAssert(fPool != NULL);
	return isOK;
}


const VObject* VDebugBlockInfo::GetAsVObject(char* outClassName, size_t inClassNameMaxBytes) const
{
	const VObject*	obj = NULL;

	if ((fFlags & kBlockIsVObject) &&(fFlags & kBlockIsUsed)) {

	#if VERSIONWIN

		#if COMPIL_VISUAL
			#pragma warning(push)
			#pragma warning(disable: 4530) // C++ exception handler used, but unwind semantics are not enabled. Specify -GX
			// typeid peut generer une exception meme si dans le projet on dit qu'on en veut pas!! 
			try {
		#endif

			obj = reinterpret_cast<const VObject*>(fData);
			obj = dynamic_cast<const VObject*>(obj);
			if (obj != NULL) {
				// tries to access the typeid which uses the vtbl
				const sBYTE*	className = typeid(*obj).name();
				size_t len = ::strlen(className);
				if (outClassName != NULL)
				{
					outClassName[0] = 0;
					::strncat(outClassName, className, inClassNameMaxBytes-1);
				}

				// tries to access debugdump
			}

		#if COMPIL_VISUAL
			} catch(...) {
				obj = NULL;
			};
			#pragma warning(pop)
		#endif

	#elif VERSIONMAC
		sReadFailure = false;
		sNewHandler = ::NewExceptionHandlerUPP(MyExceptionHandler);
		sOldHandler = ::InstallExceptionHandler(sNewHandler);

		obj = reinterpret_cast<const VObject*>(fData);
		obj = dynamic_cast<const VObject*>(obj);
		if (obj != NULL) {
			// tries to access the typeid which uses the vtbl
			const char*	className = typeid(*obj).name();
			const char*	inAddress = className;
			// tries to compute the len
			sLONG len = 0;
			do {
				volatile char ch = *inAddress++;
				// check access violation or too big class name
				if (sReadFailure ||(++len > 4096L))
				{
					obj = NULL;
					break;
				}
				if (ch == 0)
					break;
			} while(true);

			if (outClassName != NULL)
			{
				outClassName[0] = 0;
				::strncat(outClassName, className, inClassNameMaxBytes-1);
			}
		}

		::InstallExceptionHandler(sOldHandler);
		::DisposeExceptionHandlerUPP(sNewHandler);
		sOldHandler = NULL;
		sNewHandler = NULL;
	#else
		(void) outClassName;
		(void) inClassNameMaxBytes;
	#endif
	}
	return obj;
}

#endif

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
#ifndef __VDebugBlockInfo__
#define __VDebugBlockInfo__

#include "Kernel/Sources/VStackCrawl.h"

BEGIN_TOOLBOX_NAMESPACE

// Class constants
const uLONG	kBlockIsUsed		= 1;
const uLONG	kBlockIsVObject		= 2;


// Defined bellow
class VDebugBlockInfo;
class VDebugBlockInfoPool;


// Needed declatations
class VObject;


// Class definitions
typedef struct DebugBlockHeader
{
	uLONG	fMarker;
	VDebugBlockInfo*	fDebugInfo;
} DebugBlockHeader;


#if !VERSIONDEBUG
class VDebugBlockInfo {};

class VDebugBlockInfoPool {
public:
	VDebugBlockInfoPool*	GetPrevious () const { return NULL; };
	VDebugBlockInfoPool*	GetNext () const { return NULL; };
};
#endif

#if VERSIONDEBUG

class VDebugBlockInfo
{
public:
	VDebugBlockInfo (VDebugBlockInfoPool* inPool, VDebugBlockInfo* inFirstFreeBlock) { fPool = inPool; fNextFree = inFirstFreeBlock; fFlags = 0; };
	
	// Memory block support
	void					Use( void* inUserData, VSize inUserSize, bool inIsVObject);
	void					Free();

	bool					Check() const;
	
	// Accessors
	bool					IsVObject() const						{ return (fFlags & kBlockIsVObject) != 0; }
	bool					IsBlockUsed() const						{ return (fFlags & kBlockIsUsed) != 0; }
	
	void					SetUserSize( VSize inSize)				{ fUserSize = inSize; }
	VSize					GetUserSize() const						{ return fUserSize; }
	
	void					SetNextFree( VDebugBlockInfo* inNext)	{ fNextFree = inNext; }
	VDebugBlockInfo*		GetNextFree() const						{ return fNextFree; }
	
	const VStackCrawl&		GetStackCrawl () const					{ return fStackCrawl; }
	VStackCrawl&			GetStackCrawl ()						{ return fStackCrawl; }
	
	VDebugBlockInfoPool*	GetPool () const						{ return fPool; }
	
	// Operators
	void* operator new (size_t /*inSize*/, void* inAddr)			{ return inAddr; }

	// Utilities
	const VObject*			GetAsVObject( char* outClassName, size_t inClassNameMaxChars) const;
	
protected:
	VStackCrawl				fStackCrawl;
	uLONG					fFlags;
	uLONG					fTimeStamp;
	VSize					fUserSize;
	void*					fData;
	VDebugBlockInfoPool*	fPool;
	VDebugBlockInfo*		fNextFree;
};


class VDebugBlockInfoPool
{
public:
	typedef sBYTE	VDebugBlockInfoData[sizeof(VDebugBlockInfo)];
	enum {
		kBlocksPerPool = (4096L - 16L) / sizeof(VDebugBlockInfo)
	};
	
	VDebugBlockInfoPool( VDebugBlockInfo*& ioFirstFreeBlock);
	
	// Block support
	VDebugBlockInfo&		GetNthBlock( sLONG inIndex)				{ assert (inIndex >= 1 && inIndex <= kBlocksPerPool); return *reinterpret_cast<VDebugBlockInfo*>(&fInfo[inIndex-1]); }
	
	void					SetPrevious( VDebugBlockInfoPool* inPrevious) { fPrevious = inPrevious; }
	VDebugBlockInfoPool*	GetPrevious() const						{ return fPrevious; }

	void					SetNext( VDebugBlockInfoPool* inNext)	{ fNext = inNext; }
	VDebugBlockInfoPool*	GetNext() const							{ return fNext; }

	sLONG					UseOneBlock()							{ return --fCountFree; }
	sLONG					FreeOneBlock()							{ return ++fCountFree; }
	
	bool					IsEmpty() const							{ return fCountFree == kBlocksPerPool; }

	// Operators
	void*	operator new (size_t inSize);
	void	operator delete (void* inAddress);
	
protected:	
	VDebugBlockInfoPool*	fNext;
	VDebugBlockInfoPool*	fPrevious;
	VDebugBlockInfoData		fInfo[kBlocksPerPool];
	sLONG					fCountFree;
};

#endif

END_TOOLBOX_NAMESPACE

#endif

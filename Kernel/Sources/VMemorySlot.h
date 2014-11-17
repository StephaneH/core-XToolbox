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
#ifndef __VMemorySlot__
#define __VMemorySlot__

#include "Kernel/Sources/VMemory.h"
#include "Kernel/Sources/VLeaks.h"


BEGIN_TOOLBOX_NAMESPACE

#if WITH_VMemoryMgr

// Class declarations
struct HandleBlock;

class XTOOLBOX_API VMemSlotMgr : public VMemoryMgr
{
public:
			VMemSlotMgr ();
	virtual	~VMemSlotMgr ();
	
	// Inherited from VMemoryMgr
	virtual	VHandle		NewHandleFromData( const void *inData, VSize inLength);
	virtual	VHandle		NewHandleFromHandle( const VHandle inOriginal);
	virtual	VHandle		NewHandle( VSize inSize, uWORD inAtomicSize = 0);
	virtual	VHandle		NewHandleClear( VSize inSize, uWORD inAtomicSize = 0);
	virtual	void		DisposeHandle( VHandle ioHandle);
	
	virtual VPtr		LockHandle( VHandle ioHandle);
	virtual void		UnlockHandle( VHandle ioHandle);
	virtual bool		IsLocked( const VHandle inHandle);
	
	virtual	VSize		GetHandleSize( const VHandle inHandle);
	virtual	bool		SetHandleSize( VHandle ioHandle, VSize inNewSize);
	virtual	bool		SetHandleSizeClear( VHandle ioHandle, VSize inNewSize);

	virtual	HandleRef	ForgetTrueHandle( VHandle ioHandle);
	virtual	VHandle		InsertTrueHandle( HandleRef inHandleRef);

	virtual	VPtr		NewPtr( const VHandle inHandle);
	
	virtual	VSize		GetFreeMemory ();

	virtual bool		SetWithLeaksCheck( bool inSetIt);
	virtual	void		DumpLeaks();

private:
			XMemMgrImpl		fMemMgr;
			HandleRef		fSlotHandle;
			uLONG			fLastFreeSlot;
			uLONG			fAllocatedSlots;
			uLONG			fUsedSlots;
			HandleBlock*	fSlotPtr;
			VLeaksChecker	fLeaksChecker;
	
	// Private utilities
			uLONG			_GetSlot();
			void			_LooseSlot( HandleBlock* inHandleBlock);
	
			uLONG			_NewHandle( HandleBlock** outHandleBlock, VSize inSize, uWORD inAtomSize);
			bool			_SetHandleSize( VHandle ioHandle, VSize inLogicalSize, bool inClear);
	
			HandleBlock* 	_GetHandleBlock( const VHandle inHandle);
};
#endif

END_TOOLBOX_NAMESPACE


#endif
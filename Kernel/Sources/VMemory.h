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
#ifndef __VMemory__
#define __VMemory__

#if WITH_VMemoryMgr

#if VERSIONMAC
	#include "Kernel/Sources/XMacMemoryMgr.h"
#elif VERSION_LINUX
	#error There is no support for Memory Manager on Linux
#elif VERSIONWIN
	#include "Kernel/Sources/XWinMemoryMgr.h"
#elif VERSION_DARWIN
    #include "Kernel/Sources/XDarwinMemoryMgr.h"
#endif

#endif

BEGIN_TOOLBOX_NAMESPACE

/*
	@brief Handle based memory manager interface. The sole inmplementation is currently VMemSlotMgr.
*/
#if WITH_VMemoryMgr
class XTOOLBOX_API VMemoryMgr
{
public:
			VMemoryMgr()	{;}
	virtual	~VMemoryMgr();
	
	// Handle creation
			VHandle		NewEmptyHandle()	{ return NewHandle( 0);}
	virtual	VHandle		NewHandle( VSize inSize, uWORD inAtomSize = 0) = 0;
	virtual	VHandle		NewHandleFromHandle( const VHandle inOriginal) = 0;
	virtual	VHandle		NewHandleFromData( const void *inData, VSize inSize) = 0;
	virtual	VHandle		NewHandleClear( VSize inSize, uWORD inAtomSize = 0) = 0;
	virtual	void		DisposeHandle( VHandle ioHandle) = 0;
	
	// Handle size support
	virtual	VSize		GetHandleSize( const VHandle inHandle) = 0;
	virtual	bool		SetHandleSize( VHandle ioHandle, VSize inSize) = 0;
	virtual	bool		SetHandleSizeClear( VHandle ioHandle, VSize inSize) = 0;
	
	// Handle state support
	virtual	VPtr		LockHandle( VHandle ioHandle) = 0;
	virtual	void		UnlockHandle( VHandle ioHandle) = 0;
	virtual	bool		IsLocked( const VHandle inHandle) = 0;
	
	// Native handle accessors
	virtual	VHandle		InsertTrueHandle( HandleRef ioHandle) = 0;
	virtual	HandleRef	ForgetTrueHandle( VHandle ioHandle) = 0;
	
	// Pointer creation
	virtual	VPtr		NewPtr( const VHandle inOriginal) = 0;
	
	// Manager memory state
	virtual	VSize		GetFreeMemory() = 0;

	// Manager debug utils
	virtual bool		SetWithLeaksCheck( bool inSetIt) = 0;
	virtual void		DumpLeaks() = 0;
	
private:
};
#endif

class XTOOLBOX_API VMemory
{
public:
	/*
		pointer based memory operations are forwarded to VObject::GetMainMemMgr().
	*/
	static	VPtr		NewPtr( VSize inSize, sLONG inTag);
	static	VPtr		NewPtrClear( VSize inSize, sLONG inTag);
	static	void		DisposePtr( void* ioPtr);
	static	VPtr		SetPtrSize( void* ioPtr, VSize inSize);
	
#if WITH_VMemoryMgr
	static	VHandle		NewHandle( VSize inSize)										{ return sMemoryMgr->NewHandle( inSize); }
	static	VHandle		NewHandleFromHandle( const VHandle inOriginal)					{ return sMemoryMgr->NewHandleFromHandle( inOriginal); }
	static	VHandle		NewHandleFromData( const void *inData, VSize inSize)			{ return sMemoryMgr->NewHandleFromData( inData, inSize); }
	static	VHandle		NewHandleClear( VSize inSize)									{ return sMemoryMgr->NewHandleClear( inSize); }
	static	void		DisposeHandle( VHandle ioHandle)								{ sMemoryMgr->DisposeHandle( ioHandle); }
	
	static	VPtr		LockHandle( VHandle ioHandle)									{ return sMemoryMgr->LockHandle( ioHandle); }
	static	void		UnlockHandle( VHandle ioHandle)									{ sMemoryMgr->UnlockHandle( ioHandle); }
	static	void		HLock( VHandle ioHandle)										{ sMemoryMgr->LockHandle( ioHandle); }
	static	void		HUnlock( VHandle ioHandle)										{ sMemoryMgr->UnlockHandle( ioHandle); }
	static	bool		IsLocked( const VHandle inHandle)								{ return sMemoryMgr->IsLocked( inHandle); }
	
	static	bool		SetHandleSize( VHandle ioHandle, VSize inSize)					{ return sMemoryMgr->SetHandleSize( ioHandle, inSize); }
	static	VSize		GetHandleSize( const VHandle inHandle)							{ return sMemoryMgr->GetHandleSize( inHandle); }
#endif

	static	void		CopyBlock( const void* inSource, void* inDest, VSize inSize)	{ ::memmove( inDest, inSource, inSize);}

	// don't use that!
	static	VError		GetLastError()													{ return VE_OK; }

	// Private init/deinit methods
#if WITH_VMemoryMgr
	static	VMemoryMgr*	GetManager()													{ return sMemoryMgr; }
	static	void		_ProcessEntered( VMemoryMgr* inManager)							{ sMemoryMgr = inManager; }
	static	void		_ProcessExited()												{ sMemoryMgr = NULL; }
#endif

protected:
#if WITH_VMemoryMgr
	static	VMemoryMgr*	sMemoryMgr;
#endif
};


inline void CopyBlock (const void* inSource, void* inDest, VSize inSize) { VMemory::CopyBlock(inSource, inDest, inSize); }



END_TOOLBOX_NAMESPACE

#endif

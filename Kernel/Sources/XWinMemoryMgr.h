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
#ifndef __XWinMemoryMgr__
#define __XWinMemoryMgr__

BEGIN_TOOLBOX_NAMESPACE

typedef HANDLE	HandleRef;

class XWinMemoryMgr
{
public:
	// Handle creation
	HandleRef	NewHandle( VSize inSize)											{ return ::GlobalAlloc( GMEM_MOVEABLE, inSize); }
	void		DisposeHandle( HandleRef ioHandle)									{ ::GlobalFree( ioHandle); }
	
	// Handle size support
	VSize		GetPhysicalHandleSize( const HandleRef inHandle)					{ return ::GlobalSize( inHandle); }
	HandleRef	SetLogicalHandleSize( HandleRef inHandle, VSize inLogicalSize)		{ return (HandleRef) ::GlobalReAlloc( inHandle, inLogicalSize, GMEM_MOVEABLE); }
	
	// Handle state support
	void		UnlockHandle( HandleRef ioHandle, uLONG inLocks)					{ ::GlobalUnlock( ioHandle); }
	VPtr		LockHandle( HandleRef ioHandle, uLONG inLocks)						{ return (VPtr) ::GlobalLock( ioHandle); }

	// Manager memory state
	VSize		GetFreeMemory();
};


typedef XWinMemoryMgr XMemMgrImpl;

END_TOOLBOX_NAMESPACE

#endif
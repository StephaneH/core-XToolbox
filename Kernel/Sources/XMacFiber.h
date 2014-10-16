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
#ifndef __XMacFiber__
#define __XMacFiber__

// mimic Windows Fibers (same prototypes)
// global namespace as in windows to make work :: prefix (may change)

#define FIBER_FLAG_FLOAT_SWITCH 0x1	// context switch floating point

typedef void (*LPFIBER_START_ROUTINE)( void *inParam);

XTOOLBOX_API void	DeleteFiber( void *inFiber);
XTOOLBOX_API void	SwitchToFiber( void *inFiber);
XTOOLBOX_API void*	ConvertThreadToFiber( void *inParam);
XTOOLBOX_API void*	GetCurrentFiber();
XTOOLBOX_API void*	GetFiberData();
XTOOLBOX_API void*	CreateFiberEx( size_t inStackCommitSize, size_t inStackReserveSize, uLONG inFlags, LPFIBER_START_ROUTINE inRunProc, void *inParam);


#endif

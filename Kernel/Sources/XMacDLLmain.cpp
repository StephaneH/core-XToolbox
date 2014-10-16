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
#include "VObject.h"
#include "VProcess.h"
#include "VMemoryCpp.h"
#include "VMemorySlot.h"


// See _dyld_register_func_for_link_module
extern "C" {
	void _EnterLibrary ();
	void _LeaveLibrary ();
}

void _EnterLibrary ()
{
	if (VObject::GetMainMemMgr() == NULL)
		VObject::_ProcessEntered(new VCppMemMgr);
#if WITH_VMemoryMgr
	if (VMemory::GetManager() == NULL)
		VMemory::_ProcessEntered(new VMemSlotMgr);
#endif
}


void _LeaveLibrary ()
{
	// In case we crashed, VProcess::sProcess may not be deleted when we get there.
	// so let's ensure VProcess::sProcess is NULL to tell mem managers the game is over.
	VProcess::_ProcessExited();

#if WITH_VMemoryMgr
	VMemoryMgr*	slotMgr = VMemory::GetManager();
	VMemory::_ProcessExited();
	delete slotMgr;
#endif

	VCppMemMgr*	cppMgr = VObject::GetAllocator();
	VObject::_ProcessExited();
	delete cppMgr;
}

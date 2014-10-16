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

#if XTOOLBOX_AS_STANDALONE
XTOOLBOX_API BOOL XToolBoxDllMain(HANDLE hInst, ULONG fdwReason, LPVOID lpReserved)
#else
BOOL WINAPI DllMain(HANDLE hInst, ULONG fdwReason, LPVOID lpReserved)
#endif
{
	VProcess::WIN_SetToolboxInstance((HINSTANCE)hInst);
	
	BOOL	result = false;

	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
			if (VObject::GetMainMemMgr() == NULL)
				VObject::_ProcessEntered(new VCppMemMgr);
			if (VMemory::GetManager() == NULL)
				VMemory::_ProcessEntered(new VMemSlotMgr);
			result = true;
			break;

		case DLL_PROCESS_DETACH:
		{
			// in case we crashed, VProcess::sProcess may not be deleted when we get there.
			// so let's ensure VProcess::sProcess is NULL to tell mem managers the game is over.
			VProcess::_ProcessExited();

			VMemoryMgr*	slotmgr = VMemory::GetManager();
			VMemory::_ProcessExited();

			// sert pas a grand-chose et fait crasher (m.c, ACI0044339)
			//			delete slotmgr;

			VCppMemMgr*	cppmgr = VObject::GetAllocator();
			//VObject::_ProcessExited();

			//delete cppmgr;

			result = true;
			break;
		}

		case DLL_THREAD_ATTACH:
			result = true;
			break;
			
		case DLL_THREAD_DETACH:
			result = true;
			break;
	}

	return result;
}


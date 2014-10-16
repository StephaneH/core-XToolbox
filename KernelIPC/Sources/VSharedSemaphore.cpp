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
#include "VKernelIPCPrecompiled.h"
#include "VSharedSemaphore.h"



VError VSharedSemaphore::Init(uLONG inKey, uLONG inInitCount)
{
	if(fImpl.Init(inKey, inInitCount)!=VE_OK)
		return vThrowError(VE_SEM_INIT_FAILED);

	fKey=inKey;
	fCount=inInitCount;

	return VE_OK;
}


bool VSharedSemaphore::IsNew()
{
	//IsNew() on invalid IPC doesn't make sense.
	return fImpl.IsNew();
}


VError VSharedSemaphore::Lock()
{
	if(fImpl.Lock()!=VE_OK)
		return vThrowError(VE_SEM_LOCK_FAILED);

	return VE_OK;
}


VError VSharedSemaphore::Unlock()
{
	if(fImpl.Unlock()!=VE_OK)
		return vThrowError(VE_SEM_UNLOCK_FAILED);

	return VE_OK;
}


#if VERSIONMAC || VERSION_LINUX
VError VSharedSemaphore::Remove()
{
	if(fImpl.Remove()!=VE_OK)
		return vThrowError(VE_SEM_REMOVE_FAILED);

	return VE_OK;
}
#endif
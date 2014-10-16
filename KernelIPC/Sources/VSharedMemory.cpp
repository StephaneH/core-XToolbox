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
#include "VSharedMemory.h"


VError VSharedMemory::Init(uLONG inKey, VSize inSize)
{
	if(fImpl.Init(inKey, inSize)!=VE_OK)
		return vThrowError(VE_SHM_INIT_FAILED);
	
	fState=Detached, fSize=inSize;

	return VE_OK;
}


bool VSharedMemory::IsNew()
{
	//IsNew() on invalid IPC doesn't make sense.
	return fImpl.IsNew();
}


void* VSharedMemory::GetAddr()
{
	if(fState==Detached)
	{
		if(fImpl.Attach()!=VE_OK)
		{
			vThrowError(VE_SHM_ATTACH_FAILED);
			return NULL;
		}

		fState=Attached;

		fAddr=fImpl.GetAddr();
	}

	return fAddr;
}


VError VSharedMemory::Detach()
{
	if(fState==Attached)
	{
		if(fImpl.Detach()!=VE_OK)
			return vThrowError(VE_SHM_DETACH_FAILED);

		fState=Detached;
	}

	return VE_OK;
}


#if VERSIONMAC || VERSION_LINUX
VError VSharedMemory::Remove()
{
	if(fState==Attached && Detach()!=VE_OK)
		return vThrowError(VE_SHM_REMOVE_FAILED);

	if(fState==Detached && fImpl.Remove()!=VE_OK)
		return vThrowError(VE_SHM_REMOVE_FAILED);	

	fState=Invalid;

	return VE_OK;
}
#endif

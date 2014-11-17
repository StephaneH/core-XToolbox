
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

#include "VProcessIPC.h"

#include "VSleepNotifier.h"


#if VERSIONMAC
	#include "XMacSleepNotifier.h"
#elif VERSION_LINUX
	#include "XLinuxSleepNotifier.h"
#endif


BEGIN_TOOLBOX_NAMESPACE


//static
VSleepNotifier* VSleepNotifier::Instance()
{
	return VProcessIPC::Get()->GetSleepNotifier();
}


VError VSleepNotifier::RegisterSleepHandler(VSleepHandler* inHandler)
{
	if(inHandler==NULL)
		return VE_INVALID_PARAMETER;

	fMutex.Lock();

	VString tag=inHandler->GetInfoTag();

	std::set<VSleepHandler*>::const_iterator cit=fHandlers.find(inHandler);

	if(cit==fHandlers.end())
	{
		inHandler->Retain();

		fHandlers.insert(inHandler);

		//DebugMsg("[%d] VSleepNotifier::RegisterSleepHandler() : Retained '%S' handler\n",
		//	VTask::GetCurrentID(), &tag);
	}
	else
	{
		//DebugMsg("[%d] VSleepNotifier::RegisterSleepHandler() : Handler '%S' already retained\n",
		//	VTask::GetCurrentID(), &tag);
	}

	fMutex.Unlock();

	return VE_OK;
}


VError VSleepNotifier::UnregisterSleepHandler(VSleepHandler* inHandler)
{
	if(inHandler==NULL)
		return VE_INVALID_PARAMETER;

	fMutex.Lock();

	VString tag=inHandler->GetInfoTag();

	std::set<VSleepHandler*>::const_iterator cit=fHandlers.find(inHandler);

	if(cit!=fHandlers.end())
	{
		inHandler->Release();

		fHandlers.erase(cit);

		DebugMsg("[%d] VSleepNotifier::UnregisterSleepHandler() : Released '%S' handler\n",
				 VTask::GetCurrentID(), &tag);
	}
	else
	{
		DebugMsg("[%d] VSleepNotifier::UnregisterSleepHandler() : Handler '%S' was not found\n",
				 VTask::GetCurrentID(), &tag);
	}

	fMutex.Unlock();

	return VE_OK;
}


VError VSleepNotifier::UnregisterSleepHandlers()
{
	fMutex.Lock();

	std::set<VSleepHandler*>::const_iterator cit;

	for(cit=fHandlers.begin() ; cit!=fHandlers.end() ; ++cit)
		(*cit)->Release();

	fHandlers.clear();

	fMutex.Unlock();

	return VE_OK;
}


VSleepNotifier::VSleepNotifier()
{
#if VERSIONMAC || VERSION_LINUX

	fNotifier=new XSleepNotifier;
	
#endif
}


//virtual
VSleepNotifier::~VSleepNotifier()
{
	UnregisterSleepHandlers();
	
#if VERSIONMAC || VERSION_LINUX

	delete fNotifier;

#endif
}


VError VSleepNotifier::Init()
{
#if VERSIONMAC || VERSION_LINUX

	VError verr=VE_INVALID_PARAMETER;

	if(testAssert(fNotifier!=NULL))
	{
		verr=fNotifier->Init();

		if(testAssert(verr==VE_OK))
			fNotifier->SetOwner(this);
	}
	
	return vThrowError(verr);

#else

	return VE_OK;

#endif

}


void VSleepNotifier::HaveSleepNotification()
{
	fMutex.Lock();

	std::set<VSleepHandler*>::const_iterator cit;

	for(cit=fHandlers.begin() ; cit!=fHandlers.end() ; ++cit)
	{
		(*cit)->OnSleepNotification();

		VString tag=(*cit)->GetInfoTag();

		DebugMsg("[%d] VSleepNotifier::HaveSleepNotification() : Handler '%S' should be ready to sleep\n",
				 VTask::GetCurrentID(), &tag);
	}

	DebugMsg("[%d] VSleepNotifier::HaveSleepNotification() : Process should be ready to sleep\n",
			 VTask::GetCurrentID());

	fMutex.Unlock();
}


void VSleepNotifier::HaveWakeupNotification()
{
	fMutex.Lock();

	std::set<VSleepHandler*>::const_iterator cit;

	for(cit=fHandlers.begin() ; cit!=fHandlers.end() ; ++cit)
	{
		(*cit)->OnWakeUp();

		VString tag=(*cit)->GetInfoTag();
		
		DebugMsg("[%d] VSleepNotifier::HaveWakeupNotification() : Handler '%S' woke up\n",
				 VTask::GetCurrentID(), &tag);
	}

	DebugMsg("[%d] VSleepNotifier::HaveWakeupNotification() : Process woke up\n",
			 VTask::GetCurrentID());

	fMutex.Unlock();
}


//virtual
void VSleepDebugHandler::OnSleepNotification()
{
	DebugMsg("[%d] VSleepDebugHandler::OnSleepNotification() : System is going to sleep\n", VTask::GetCurrentID());
}


//virtual
void VSleepDebugHandler::OnWakeUp()
{
	DebugMsg("[%d] VSleepDebugHandler::OnWakeUp() : System woke up\n", VTask::GetCurrentID());
}


//virtual
const VString VSleepDebugHandler::GetInfoTag()
{
	return CVSTR("VSleepDebugHandler");
}



END_TOOLBOX_NAMESPACE


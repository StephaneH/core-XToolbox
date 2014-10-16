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
#include "VDaemon.h"

#if VERSIONMAC || VERSION_LINUX
	#include "KernelIPC/Sources/XPosixDaemon.h"
#elif VERSIONWIN
	#include "KernelIPC/Sources/XWinDaemon.h"
#endif


VDaemon* VDaemon::sInstance = NULL;



VDaemon::VDaemon()
	: fImpl(NULL)
	, fInitCalled(false)
	, fInitOK(false)
	, fIsDaemon(false)
{
	xbox_assert(sInstance == NULL);
	sInstance = this;
}


VDaemon::~VDaemon()
{
	delete fImpl;
	fImpl = NULL;

	xbox_assert(sInstance == this);
	sInstance = NULL;
}


void VDaemon::DoRun()
{
	while (IsRunning())
	{
		#if VERSIONMAC
		::CFRunLoopRunInMode( kCFRunLoopDefaultMode, 0, false);		// sc 01/10/2010
		#endif
		VTask::ExecuteMessagesWithTimeout( 100, true);
	}
}


bool VDaemon::Init(VProcess::InitOptions inOptions)
{
	if (!fInitCalled)
	{
		if (inherited::Init( inOptions))
		{
			fInitCalled = true;
			fImpl = new XDaemonImpl( this);
			if (fImpl != NULL)
				fInitOK = fImpl->Init();
		}
	}

	#if WITH_DAEMONIZE
	if (fInitOK && IsDaemon())
		fInitOK = (VE_OK == Daemonize());
	#endif

	return fInitOK;
}


void VDaemon::SetIsDaemon(bool inIsDaemon)
{
	fIsDaemon = inIsDaemon;
}


bool VDaemon::IsDaemon()
{
	return fIsDaemon;
}


VError VDaemon::Daemonize()
{
	VError verr = VE_UNIMPLEMENTED;
	#if WITH_DAEMONIZE != 0
	if (fImpl)
		verr = fImpl->Daemonize();
	#endif

	return verr;
}

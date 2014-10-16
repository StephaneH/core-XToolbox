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

#include <mach/mach_port.h>
#include <mach/mach_interface.h>
#include <mach/mach_init.h>

#include <IOKit/pwr_mgt/IOPMLib.h>
#include <IOKit/IOMessage.h>

#include "Kernel/VKernel.h"
#include "VKernelIPCErrors.h"

#include "VSleepNotifier.h"

#include "XMacSleepNotifier.h"


BEGIN_TOOLBOX_NAMESPACE


XMacSleepNotifier::XMacSleepNotifier() :
fOwner(NULL),
fNotifierObject(0),
fRootPort(0),
fLoopRef(NULL),
fSrcRef(NULL)
{
		//empty
}


XMacSleepNotifier::~XMacSleepNotifier()
{
	//Remove the sleep notification port from the application runloop
    CFRunLoopRemoveSource(fLoopRef, fSrcRef, kCFRunLoopCommonModes);

	//Deregister for system sleep notifications
    IODeregisterForSystemPower(&fNotifierObject);

	//IORegisterForSystemPower implicitly opens the Root Power Domain IOService
	//so we close it here
    IOServiceClose(fRootPort);

	//Destroy the notification port allocated by IORegisterForSystemPower
    IONotificationPortDestroy(fNotifyPortRef);
}


VError XMacSleepNotifier::Init()
{
	VError verr=VE_OK;

	//Register to receive system sleep notifications
    fRootPort=IORegisterForSystemPower((void*)this, &fNotifyPortRef, SystemCallback, &fNotifierObject);

    if(fRootPort==MACH_PORT_NULL)
		verr=VE_FAIL_TO_INIT_SLEEP_NOTIFICATIONS;

	//Add the notification port to the application runloop

	if(verr==VE_OK)
	{
		fLoopRef=CFRunLoopGetCurrent();
		fSrcRef=IONotificationPortGetRunLoopSource(fNotifyPortRef);
		
		if(fLoopRef==NULL || fSrcRef==NULL)
			verr=VE_FAIL_TO_INIT_SLEEP_NOTIFICATIONS;
	}

	if(verr==VE_OK)
		CFRunLoopAddSource(fLoopRef, fSrcRef, kCFRunLoopCommonModes);

	//No good reason to fail !
	assert(verr==VE_OK);

	return vThrowError(verr);
}


void XMacSleepNotifier::SetOwner(VSleepNotifier* inOwner)
{
	fOwner=inOwner;
}


//static
void XMacSleepNotifier::SystemCallback(void* inThisPtr, io_service_t inUnused, natural_t inMessageType, void* inMsgArg)
{
	XMacSleepNotifier* thisPtr=reinterpret_cast<XMacSleepNotifier*>(inThisPtr);

	assert(thisPtr!=NULL);

	if(thisPtr==NULL)
		return;

    switch(inMessageType)
    {
        case kIOMessageCanSystemSleep:
		{
            /* 
			 * Idle sleep is about to kick in. This message will not be sent for forced sleep.
			 * Applications have a chance to prevent sleep by calling IOCancelPowerChange.
			 * Most applications should not prevent idle sleep.
			 *
			 * Power Management waits up to 30 seconds for you to either allow or deny idle
			 * sleep. If you don't acknowledge this power change by calling either
			 * IOAllowPowerChange or IOCancelPowerChange, the system will wait 30
			 * seconds then go to sleep.
			 */

			IOAllowPowerChange(thisPtr->fRootPort, (long)inMsgArg);
			//IOCancelPowerChange(thisPtr->fRootPort, (long)inMsgArg);
			
            break;
		}
			
        case kIOMessageSystemWillSleep:
		{
            /* 
			 * The system WILL go to sleep. If you do not call IOAllowPowerChange or
			 * IOCancelPowerChange to acknowledge this message, sleep will be
			 * delayed by 30 seconds.
			 *
			 * NOTE: If you call IOCancelPowerChange to deny sleep it returns
			 * kIOReturnSuccess, however the system WILL still go to sleep.
			 */

			thisPtr->fOwner->HaveSleepNotification();

            IOAllowPowerChange(thisPtr->fRootPort, (long)inMsgArg);
			
            break;
		}

        case kIOMessageSystemWillPowerOn:
		{
			//System has started the wake up process...
			
            break;
		}
			
        case kIOMessageSystemHasPoweredOn:
		{
			//System has finished waking up...

			thisPtr->fOwner->HaveWakeupNotification();
			
			break;
		}

        default:
		{
            break;
		}
    }
}


END_TOOLBOX_NAMESPACE

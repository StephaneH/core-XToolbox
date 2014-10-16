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

#ifndef __XMacSleepNotifier__
#define __XMacSleepNotifier__


BEGIN_TOOLBOX_NAMESPACE


class VSleepNotifier;


class XMacSleepNotifier
{
public :

	XMacSleepNotifier();
	~XMacSleepNotifier();

	void SetOwner(VSleepNotifier* inOwner);

	VError Init();


private :

	XMacSleepNotifier(const XMacSleepNotifier&){};	//no copy

	static void SystemCallback(void* inThisPtr, io_service_t inUnused, natural_t inMessageType, void* inMsgArg);

	VSleepNotifier* fOwner;

	//Notifier object, used to deregister
	io_object_t fNotifierObject;

	//Notification port allocated by IORegisterForSystemPower
	IONotificationPortRef fNotifyPortRef;

	//Reference to the Root Power Domain IOService
	io_connect_t fRootPort;

	//Add / remove the sleep notification port from the application runloop
	CFRunLoopRef fLoopRef;
	CFRunLoopSourceRef fSrcRef;
};


END_TOOLBOX_NAMESPACE


#endif

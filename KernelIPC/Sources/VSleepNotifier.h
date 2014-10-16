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

#ifndef __VSleepNotifier__
#define __VSleepNotifier__


#include <vector>


BEGIN_TOOLBOX_NAMESPACE


class VProcessIPC;


#if VERSIONWIN

//There is no implentation class on Windows !

#elif VERSIONMAC

class XMacSleepNotifier;
typedef class XMacSleepNotifier XSleepNotifier;

#elif VERSION_LINUX

//This feature is not implemented on Linux (You won't get any notifications)

class XLinuxSleepNotifier;
typedef class XLinuxSleepNotifier XSleepNotifier;

#endif


class XTOOLBOX_API VSleepNotifier : public VObject
{
public :

	static VSleepNotifier* Instance();

	class VSleepHandler : public IRefCountable
	{
	public :

		//On Win you have about 2 (two) seconds to do your business.
		//On Mac you have up to 30 sec (the quickest you return, the better !)
		//(both OS : total time per process, handlers are called one after the other)
		virtual void OnSleepNotification()=0;

		virtual void OnWakeUp()=0;

		//A short descriptive string for logs
		virtual const VString GetInfoTag()=0;
	};

	VError RetainSleepHandler(VSleepHandler* inHandler);
	VError ReleaseSleepHandler(VSleepHandler* inHandler);

	VError ReleaseSleepHandlers();


	//Callbacks used by impl (on Mac) or by main window message pump (Win)
	//to notify VSleepNotifier of basic sleep events.

	void HaveSleepNotification();
	void HaveWakeupNotification();
	
protected :

	//VSleepNotifier should be a singleton, created and owned by VProcessIPC 
	friend class VProcessIPC;

	VSleepNotifier();
	virtual ~VSleepNotifier();

	VError Init();

private :

	VSleepNotifier(const VSleepNotifier&);	//no copy

#if VERSIONMAC || VERSION_LINUX

	XSleepNotifier* fNotifier;

	//There is no implementation class on Windows. Instead, there is some specific
	//code in the application message pump to handle power messages sent to the window.
#endif

	std::set<VSleepHandler*> fHandlers;
	VCriticalSection fMutex;

};


//As an example, here is a simple handler which generate traces
//(it is not part of xbox api)

class VSleepDebugHandler : public VSleepNotifier::VSleepHandler
{
public :

	virtual void OnSleepNotification();
	virtual void OnWakeUp();
	virtual const VString GetInfoTag();

private :

		//No datas in this simple handler
};

END_TOOLBOX_NAMESPACE


#endif

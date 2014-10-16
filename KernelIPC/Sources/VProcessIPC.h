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
#ifndef __VProcessIPC__
#define __VProcessIPC__

#include "KernelIPC/Sources/VCommandLineParser.h"




BEGIN_TOOLBOX_NAMESPACE


#if VERSIONWIN
class XWinProcessIPC;
typedef XWinProcessIPC XProcessIPCImpl;
#elif VERSIONMAC || VERSION_LINUX
class XPosixProcessIPC;
typedef XPosixProcessIPC XProcessIPCImpl;
#endif

class VFileSystemNotifier;
class VSleepNotifier;





class XTOOLBOX_API VProcessIPC : public VProcess
{
typedef VProcess inherited;

public:
									VProcessIPC();
	virtual							~VProcessIPC();

	virtual	bool					Init(VProcess::InitOptions inOptions);

	static	VProcessIPC*			Get()							{ return sInstance; }

			XProcessIPCImpl*		GetImpl() const					{ return fImpl; }

			VFileSystemNotifier*	GetFileSystemNotifier() const	{ return fFileSystemNotifier; }

			//Not implemented on Linux (You won't get any notifications)
			VSleepNotifier*			GetSleepNotifier() const		{ return fSleepNotifier; }

			//! \name Command line interaction
			//@{
			//! Get the current exit status
			int GetExitStatus() const { return fExitStatus; }
			//! Set the exit status
			void SetExitStatus(int exitstatus) { fExitStatus = exitstatus; }
			//@}

protected:
	virtual void					DoRun();
	virtual VCommandLineParser::Error DoParseCommandLine(VCommandLineParser&); // new

private:
			bool					fInitCalled;
			bool					fInitOK;
			XProcessIPCImpl*		fImpl;
			VFileSystemNotifier*	fFileSystemNotifier;
			VSleepNotifier*			fSleepNotifier;
			//! Process Exit status code
			int fExitStatus;

	static	VProcessIPC*			sInstance;

}; // class VProcessIPC





END_TOOLBOX_NAMESPACE

#endif

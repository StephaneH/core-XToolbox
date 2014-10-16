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
#ifndef __VDaemon__
#define __VDaemon__

#include "VProcessIPC.h"

BEGIN_TOOLBOX_NAMESPACE

#if VERSIONWIN
class XWinDaemon;
typedef XWinDaemon XDaemonImpl;
#elif VERSIONMAC || VERSION_LINUX
class XPosixDaemon;
typedef XPosixDaemon XDaemonImpl;
#endif

/*
	VDaemon is intended to implement face-less services.
*/
class XTOOLBOX_API VDaemon : public VProcessIPC
{
typedef VProcessIPC inherited;

public:
			VDaemon();
	virtual	~VDaemon();

	virtual	bool			Init(VProcess::InitOptions inOptions);

	static	VDaemon*		Get()						{ return sInstance; }

			XDaemonImpl*	GetImpl() const				{ return  fImpl; }
			
			void			SetIsDaemon(bool inIsDaemon);
			bool			IsDaemon();

			VError Daemonize();

protected:

	virtual void			DoRun();

			bool			fInitCalled;
			bool			fInitOK;
			XDaemonImpl*	fImpl;

	static	VDaemon*		sInstance;
			bool			fIsDaemon;

};

END_TOOLBOX_NAMESPACE

#endif

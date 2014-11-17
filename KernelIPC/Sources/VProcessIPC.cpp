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
#include "VFileSystemNotification.h"
#include "VSleepNotifier.h"
#include "VProcessIPC.h"
#if VERSIONMAC || VERSION_LINUX
#include "KernelIPC/Sources/XPosixDaemon.h"
#elif VERSIONWIN
#include "KernelIPC/Sources/XWinDaemon.h"
#endif
#include <cstdlib> // EXIT_FAILURE, EXIT_SUCCESS
#include <iostream>





BEGIN_TOOLBOX_NAMESPACE

VProcessIPC* VProcessIPC::sInstance = NULL;


#if WITH_NEW_XTOOLBOX_GETOPT

//! Total number of Command line arguments
/*extern*/ XTOOLBOX_API int sProcessCommandLineArgc = 0;

#if !VERSIONWIN // all UNIX versions
//! All command line arguments (native version for later use)
/*extern*/ XTOOLBOX_API const char** sProcessCommandLineArgv = NULL;
#else
//! All command line arguments (native version for later use)
/*extern*/ XTOOLBOX_API const wchar_t** sProcessCommandLineArgv = NULL;
#endif

#endif // WITH_NEW_XTOOLBOX_GETOPT






VProcessIPC::VProcessIPC()
	: fInitCalled(false)
	, fInitOK(false)
	, fImpl(NULL)
	, fFileSystemNotifier(NULL)
	, fSleepNotifier(NULL)
	, fExitStatus(EXIT_FAILURE) // error by default
{
	xbox_assert(sInstance == NULL);
	sInstance = this;
}


VProcessIPC::~VProcessIPC()
{
	delete fFileSystemNotifier;
	delete fSleepNotifier;
	delete fImpl;
	fImpl = NULL;

	xbox_assert(sInstance == this);
	sInstance = NULL;
}


void VProcessIPC::DoRun()
{
	while (IsRunning())
	{
		VTask::ExecuteMessagesWithTimeout( 100, true);
	}
}


VCommandLineParser::Error  VProcessIPC::DoParseCommandLine(VCommandLineParser&)
{
	// empty implementation on purpose
	return VCommandLineParser::eOK;
}


bool VProcessIPC::Init(VProcess::InitOptions inOptions)
{
	if (!fInitCalled)
	{
		if (inherited::Init(inOptions)) // VProcess::Init()
		{
			fInitCalled = true;

			// Parse the command line arguments
			{
				// at this point of execution, we have only grab the arguments from the
				// main entry point
				std::vector<VString> args;

				#if WITH_NEW_XTOOLBOX_GETOPT
				// If an assert occurs here, it probably mean that the main entry point
				// is not properly defined
				// \see MainEntryPoint.h
				// \see int _main()
				xbox_assert(sProcessCommandLineArgv != NULL);
				xbox_assert(sProcessCommandLineArgc > 0); // no real upper bound on unix

				args.reserve(sProcessCommandLineArgc);
				for (uLONG i = 0; i != (uLONG) sProcessCommandLineArgc; ++i)
				{
					#if VERSIONMAC
					// On OSX, we may get some invalid arguments, like a process serial number
					// (Carbon, deprecated on 10.9). For all versions of OSX <10.9, we should ignore
					// all arguments like -psn_X_XXXXXXXX
					if (0 == ::strncmp(sProcessCommandLineArgv[i], "-psn_", 5))
						continue; // silent ignore
					#endif
					args.push_back(sProcessCommandLineArgv[i]);
				}

				#else // WITH_NEW_XTOOLBOX_GETOPT
				// temporary measure to avoid future asserts by adding a dummy value
				args.push_back(CVSTR("./a.out"));
				#endif // WITH_NEW_XTOOLBOX_GETOPT


				// the method `DoParseCommandLine` should always be called, even
				// if obviously not necessary. The user may implement some specific
				// code for initializing its application or part of it
				VCommandLineParser cmdparser(args);
				switch (DoParseCommandLine(cmdparser))
				{
					case VCommandLineParser::eOK:
					{
						fExitStatus = EXIT_SUCCESS;
						fInitOK = true;
						break;
					}
					case VCommandLineParser::eShouldExit:
					{
						fExitStatus = EXIT_SUCCESS;
						fInitOK = false; // force program shutdown
						break;
					}
					default:
					{
						// error in any cases
						fExitStatus = EXIT_FAILURE;
						fInitOK = false;
					}
				}
			} // command line arguments parsing

			if (fInitOK)
			{
				fImpl = new XProcessIPCImpl(this);
				fInitOK = (fImpl != NULL) ? fImpl->Init() : false;
			}

			if (fInitOK)
			{
				if ((inOptions & Init_WithFileSystemNotifier) != 0)
					fFileSystemNotifier = new VFileSystemNotifier;
				fSleepNotifier = new VSleepNotifier;

				#if VERSIONMAC || VERSION_LINUX
				VError verr = fSleepNotifier->Init();
				xbox_assert(verr == VE_OK);
				#endif

				#if VERSIONDEBUG
				//A debug sleep handler which produce some log on sleep events
				VSleepDebugHandler* sleepDbgHandler=new VSleepDebugHandler;
				VSleepNotifier::Instance()->RegisterSleepHandler(sleepDbgHandler);
				ReleaseRefCountable(&sleepDbgHandler);
				#endif

				// fExitStatus will remain to an error value
				// it is the responsability of `main()` to properly reset to EXIT_SUCCESS
				// if the application has been properly initialized.
			}
		}
	}
	return fInitOK;
}








END_TOOLBOX_NAMESPACE


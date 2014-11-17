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
#include "XPosixDaemon.h"
#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE
#endif
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>


#if defined(HAVE_CAP_NG) && (!defined(XTOOLBOX_AS_STANDALONE) || !XTOOLBOX_AS_STANDALONE)
#include <cap-ng.h>
#endif




XPosixProcessIPC::XPosixProcessIPC( VProcessIPC* /*inDaemon*/)
{
}


XPosixProcessIPC::~XPosixProcessIPC()
{
}


void SIGTERM_handler(int /*inSigNum*/)
{
	VProcess::Get()->QuitAsynchronously();
}


void SIGINT_handler(int /*inSigNum*/)
{
	VProcess::Get()->QuitAsynchronously();
}


void SIGQUIT_handler(int /*inSigNum*/)
{
	VProcess::Get()->QuitAsynchronously();
}


bool XPosixProcessIPC::Init()
{
	struct sigaction sa;

	// install handler for SIGTERM signal
	sa.sa_handler = SIGTERM_handler;
	sigemptyset( &sa.sa_mask);
	sigaddset( &sa.sa_mask, SIGINT);
	sigaddset( &sa.sa_mask, SIGQUIT);
	sa.sa_flags = SA_RESTART;

	if (sigaction( SIGTERM, &sa, NULL) != 0)
		return false;

	// install handler for SIGINT signal
	sa.sa_handler = SIGINT_handler;
	sigemptyset( &sa.sa_mask);
	sigaddset( &sa.sa_mask, SIGTERM);
	sigaddset( &sa.sa_mask, SIGQUIT);
	sa.sa_flags = SA_RESTART;

	if (sigaction( SIGINT, &sa, NULL) != 0)
		return false;

	// install handler for SIGQUIT signal
	sa.sa_handler = SIGQUIT_handler;
	sigemptyset( &sa.sa_mask);
	sigaddset( &sa.sa_mask, SIGTERM);
	sigaddset( &sa.sa_mask, SIGINT);
	sa.sa_flags = SA_RESTART | SA_RESETHAND;

	if (sigaction( SIGQUIT, &sa, NULL) != 0)
		return false;

	return true;
}


//=======================================



XPosixDaemon::XPosixDaemon( VDaemon *inDaemon)
{
}


XPosixDaemon::~XPosixDaemon()
{
}


bool XPosixDaemon::Init()
{
	return true;
}


#if WITH_DAEMONIZE

VError XPosixDaemon::Daemonize()
{
	#if defined(HAVE_CAP_NG) && (!defined(XTOOLBOX_AS_STANDALONE) || !XTOOLBOX_AS_STANDALONE)

	//Nice page on capabilities :
	//https://people.redhat.com/sgrubb/libcap-ng/

	//Check if we have any capabilities
	if (capng_have_capabilities(CAPNG_SELECT_CAPS) > CAPNG_NONE)
	{
		capng_clear(CAPNG_SELECT_BOTH);

		capng_type_t type = static_cast<capng_type_t>(CAPNG_EFFECTIVE|CAPNG_PERMITTED);

		int res = capng_updatev(CAPNG_ADD, type,
			CAP_NET_BIND_SERVICE,	// Allows binding to TCP/UDP sockets below 1024
			CAP_NET_BROADCAST,	    // Allow broadcasting, listen to multicast
			-1);

		if (res != 0)
		{
			std::cerr << "error: capng: impossible to update the stored capabilities settings\n";
			return VE_FAIL_TO_DAEMONIZE;
		}

		res = capng_apply(CAPNG_SELECT_BOTH);

		if (res != 0)
		{
			std::cerr << "error: capng: impossible to apply the stored capabilities settings\n";
			return VE_FAIL_TO_DAEMONIZE;
		}
	}
	#endif // HAVE_CAP_NG



	std::cout << "entering in daemon-mode\n";
    pid_t pid = fork();

	// Parent process should now quit asap, with no side effects
	// on ressources shared with its child (eg. do not corrupt files).

	// As it turns out, it's not so easy to properly quit a process
	// that is not fully initialized : QuitAsynchronously() and exit()
	// end in a crash.

	// Two good candidates to quickly quit with no crash are _exit() and kill().

    if (pid > 0)
		_exit(0); // in the parent process

    if (pid < 0) // error
	{
		int err = errno;
		std::cerr << "error: failed to daemonize (errno: " << err << ")\n";
		return VE_FAIL_TO_DAEMONIZE;
	}

	// The child process becomes a session leader
    if (setsid() < 0)
	{
		std::cerr << "error: failed to creates a session and sets the process group ID\n";
        return VE_FAIL_TO_DAEMONIZE;
	}

	//Ignore a few signals - Might need some refinement !
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

	//Reset umask to some permissive and known value
    umask(0);

	//Reset current working directory to some known value
    chdir("/");

	//Close standard file descriptors
	::close(STDIN_FILENO);
	::close(STDOUT_FILENO);
	::close(STDERR_FILENO);

	return VE_OK;
}

#endif // WITH_DAEMONIZE


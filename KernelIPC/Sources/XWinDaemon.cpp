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
#include "XWinDaemon.h"
#include "VDaemon.h"


XWinProcessIPC::XWinProcessIPC( VProcessIPC *inDaemon)
: fQuitEvent( ::CreateEvent(NULL, TRUE, FALSE, NULL))
{
}


XWinProcessIPC::~XWinProcessIPC()
{
	::SetEvent( fQuitEvent);
	::CloseHandle( fQuitEvent);
}


bool XWinProcessIPC::Init()
{
	BOOL res = ::SetConsoleCtrlHandler( XWinProcessIPC::ConsoleCtrlHandler, TRUE);
	if (res == 0)
		return false;

	// test if being run in console mode.
	DWORD pid;
	DWORD count = GetConsoleProcessList( &pid, 1);
	if (count != 0)
	{
		// by default, the Console code page is IBM850 which doesn't contain accentuated character.
		// Change it to Ansi code page (ex: 1252) which is the usual ascii one.
		// This is especially useful for VProcessLauncher so that cmd.exe uses ANSI code page instead of barbone IBM850.
		UINT acp = GetACP();
		BOOL rc1 = SetConsoleCP( acp);
		BOOL rc2 = SetConsoleOutputCP( acp);
		xbox_assert( rc1 && rc2);
	}

	return true;
}


void XWinProcessIPC::HandleSystemMessage( MSG& ioMessage)
{
	::TranslateMessage( &ioMessage);
	::DispatchMessage( &ioMessage);
}


BOOL XWinProcessIPC::ConsoleCtrlHandler( DWORD inCtrlType)
{
	// the system excecutes the handler in a new thread
	BOOL result = FALSE;

	switch (inCtrlType)
	{
		case CTRL_C_EVENT:
		case CTRL_CLOSE_EVENT:
			{
				// we must do the cleanup before leaving the handler.
				// so we wait until the quit event has been set in our destructor
				HANDLE quitEvent;
				::DuplicateHandle( ::GetCurrentProcess(), VProcessIPC::Get()->GetImpl()->fQuitEvent, ::GetCurrentProcess(), &quitEvent, 0, FALSE, DUPLICATE_SAME_ACCESS);
				VProcess::Get()->QuitAsynchronously();
				::WaitForSingleObject( quitEvent, INFINITE);
				::CloseHandle( quitEvent);

				result = TRUE;
			}
			break;

		case CTRL_BREAK_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:
		default:
			break;
	}
	return result;
}



//=======================================



XWinDaemon::XWinDaemon( VDaemon *inDaemon)
{
}


XWinDaemon::~XWinDaemon()
{
}


bool XWinDaemon::Init()
{
	return true;
}


VError XWinDaemon::Daemonize()
{
	// not implemented on windows
	return VE_UNIMPLEMENTED;
}
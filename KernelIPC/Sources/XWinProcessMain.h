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
#ifndef __XWinProcessMain__
#define __XWinProcessMain__


#if USE_TOOLBOX_MAIN

// Process main entry point
int __stdcall	WinMain (HINSTANCE inInstance,HINSTANCE inPrevInstance,LPSTR inCmdLine,int inState);


// on WIN32 inPrevInstance is always NULL
// use ::GetCommandLine & ::CommandLineToArgvW instead of inCmdLine (because inCmdLine is not unicode and does not contain the app name)
// use ::GetStartupInfo instead of inState
// we only care about inInstance
int __stdcall	WinMain (HINSTANCE inInstance,HINSTANCE inPrevInstance,LPSTR inCmdLine,int inState)
{
	// Init Kernel
	VProcess::WIN_SetAppInstance(inInstance);
	
	// Call user entry point
	ProcessMain();
	
	return 0;
}

#endif	// USE_TOOLBOX_MAIN

#endif
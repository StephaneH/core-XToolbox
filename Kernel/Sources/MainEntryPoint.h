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
#ifndef __MainEntryPoint__
#define __MainEntryPoint__
#if WITH_NEW_XTOOLBOX_GETOPT

#include <cassert>
#include <iostream>




/*!
** \brief Main program entry point
*/
int _main(); // forward



#if !VERSIONWIN // all UNIX versions

BEGIN_TOOLBOX_NAMESPACE

//! Total number of Command line arguments
extern XTOOLBOX_API int sProcessCommandLineArgc /*= 0*/;

//! All command line arguments (native version for later use)
extern XTOOLBOX_API const char** sProcessCommandLineArgv /*= NULL*/;

END_TOOLBOX_NAMESPACE



int main(int inArgc, const char** inArgv)
{
	// grab the command line arguments for later use
	XBOX::sProcessCommandLineArgc = inArgc;
	XBOX::sProcessCommandLineArgv = inArgv;

	// call the real main
	return _main();
}


#else // !VERSIONWIN


BEGIN_TOOLBOX_NAMESPACE

//! Total number of Command line arguments
extern XTOOLBOX_API int sProcessCommandLineArgc /*= 0*/;

//! All command line arguments (native version for later use)
extern XTOOLBOX_API const wchar_t** sProcessCommandLineArgv /*= NULL*/;

END_TOOLBOX_NAMESPACE


#if defined(XTOOLBOX_USE_ALTURA) && XTOOLBOX_USE_ALTURA != 0
// forward declaration of altura main entry point
int alturamain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
#endif



/*!
** \brief windows main entry point for Win32 projects
**
** Both entry points are defined (for Win32 projects and console projects)
** If the the linker does not use the appropriate one at startup (CONSOLE will be selected by default),
** please select the corresponding subsystem  in the property page
** (Configuratopn Properties > Linker > System > Subsystem)
** The other "main" function will be ignored.
*/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// retrieve the command line arguments
	// another solution would be __wargc and __wargv, but it would bring
	// an additional dependency on msvcrt.dll
	XBOX::sProcessCommandLineArgv = (const wchar_t**)CommandLineToArgvW(GetCommandLineW(), &XBOX::sProcessCommandLineArgc);
	if (!XBOX::sProcessCommandLineArgv)
	{
		XBOX::sProcessCommandLineArgc = 0; // force reset just in case
		#ifndef NDEBUG
		MessageBoxW(NULL, L"Unable to parse command line", L"Error", MB_OK);
		#endif
	}

	#if defined(XTOOLBOX_USE_ALTURA) && XTOOLBOX_USE_ALTURA != 0
	// the altura main will call _main()
	int ret = alturamain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
	#else
	// call the real main
	int ret = _main();
	#endif

	LocalFree(XBOX::sProcessCommandLineArgv);
	XBOX::sProcessCommandLineArgv = NULL;
	return ret;
}


/*!
** \brief windows main entry point for win32 console projects
**
** Both entry points are defined (for Win32 projects and console projects)
** If the the linker does not use the appropriate one at startup (CONSOLE will be selected by default),
** please select the corresponding subsystem  in the property page
** (Configuratopn Properties > Linker > System > Subsystem)
** The other "main" function will be ignored.
*/
int WINAPIV wmain(int argc, const wchar_t** argv)
{
	// retrieve the command line arguments
	XBOX::sProcessCommandLineArgc = argc;
	XBOX::sProcessCommandLineArgv = argv;

	#if defined(XTOOLBOX_USE_ALTURA) && XTOOLBOX_USE_ALTURA != 0
	assert(false // real assert
		&& "altura should not be used for new console apps. please update the subsystem mode in VisualStudio");
	std::wcerr
		<< L"altura should not be used for new console apps. please update the subsystem mode in VisualStudio"
		<< std::endl;
	return EXIT_FAILURE;
	#else
	// call the real main
	return _main();
	#endif
}



#endif // !VERSIONWIN

#endif // if WITH_NEW_XTOOLBOX_GETOPT
#endif // __MainEntryPoint__

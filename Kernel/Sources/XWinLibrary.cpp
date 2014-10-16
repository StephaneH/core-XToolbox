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
#include "VKernelPrecompiled.h"
#include "VLibrary.h"


static bool _BuildExecutablePath( const VFilePath &inPath, VFilePath& outExecutablePath)
{
	bool ok;
	if (inPath.IsFile())
	{
		outExecutablePath = inPath;
		ok = true;
	}
	else if (inPath.IsFolder())
	{
		// if path is a folder named truc.someextension,
		// look at truc.someextension\Contents\Windows\truc.dll
		
		// afabre : finally, we look the dll near the exe file
		VString name;
		inPath.GetName( name);
		outExecutablePath = inPath;
		outExecutablePath = outExecutablePath.ToParent().ToParent();
		outExecutablePath.SetFileName( name);
		outExecutablePath.SetExtension( CVSTR( "dll"));
		ok = true;
	}
	else
	{
		ok = false;
	}

	return ok;
}


Boolean XWinLibrary::Load( const VFilePath &inPath)
{
	VFilePath executablePath;
	if (_BuildExecutablePath( inPath, executablePath))
	{
		fInstance = ::LoadLibraryExW( executablePath.GetPath().GetCPointer(), NULL, 0);
	}
	
	if (fInstance == NULL)
	{
		XBOX::VString path( executablePath.GetPath());
		DebugMsg("VComponentManager erreur load %V\n", &path);
		StThrowFileError throwErr( inPath, VE_LIBRARY_CANNOT_LOAD, ::GetLastError());
		fInstance = NULL;
	}
	
	return fInstance != NULL;
}


void XWinLibrary::Unload()
{
	::FreeLibrary(fInstance);
	fInstance = NULL;
}


void* XWinLibrary::GetProcAddress(const VString& inProcName)
{
	sBYTE procName[512];
	
	inProcName.WIN_ToWinCString(procName, sizeof(procName));
	
	void*	ptProc = ::GetProcAddress(fInstance, procName);
	
	return ptProc;
}


VFile* XWinLibrary::RetainExecutableFile(const VFilePath &inPath) const
{
	VFilePath executablePath;
	VFile *file = _BuildExecutablePath( inPath, executablePath) ? new VFile( executablePath) : NULL;
	return file;
}


VResourceFile* XWinLibrary::RetainResourceFile(const VFilePath& inPath)
{
	VResourceFile*	resFile = NULL;
	VFilePath executablePath;
	if (_BuildExecutablePath( inPath, executablePath))
		resFile = new VWinResFile( executablePath, FA_READ, false);
	return resFile;
}


VFolder* XWinLibrary::RetainFolder(const VFilePath &inPath, BundleFolderKind inKind) const
{
	VFolder *folder = NULL;
	if (inPath.IsFile())
	{
		switch( inKind)
		{
			case kBF_BUNDLE_FOLDER:					// The folder hosting the bundle
			case kBF_EXECUTABLE_FOLDER:				// The folder of executable file (platform related)
				{
					VFilePath parent = inPath;
					folder = new VFolder( parent.ToParent());
				}

			default:
				assert( false);
				break;
		}
	}
	else if (inPath.IsFolder())
	{
		switch( inKind)
		{
			case kBF_BUNDLE_FOLDER:					// The folder hosting the bundle
				{
					VFilePath parent = inPath;
					folder = new VFolder( parent/*.ToParent()*/);
					break;
				}

			case kBF_EXECUTABLE_FOLDER:				// The folder of executable file (platform related)
				{
					VFilePath executablePath;
					if (_BuildExecutablePath( inPath, executablePath))
					{
						folder = new VFolder( executablePath.ToFolder());
					}
					break;
				}

			case kBF_RESOURCES_FOLDER:				// The folder of main resources
				{
					VFilePath path = inPath;
					path.ToSubFolder( CVSTR("Contents")).ToSubFolder( CVSTR( "Resources"));
					folder = new VFolder( path);
					break;
				}

			case kBF_LOCALISED_RESOURCES_FOLDER:	// The folder of prefered localization
			case kBF_PLUGINS_FOLDER:				// The folder for bundle's plugins
			case kBF_PRIVATE_FRAMEWORKS_FOLDER:		// The folder for private frameworks
			case kBF_PRIVATE_SUPPORT_FOLDER:		// The folder for private support files
			default:
				assert( false);
				break;
		}
	}
	return folder;
}

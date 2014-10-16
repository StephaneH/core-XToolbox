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
#include "VProcess.h"
#include "VFile.h"
#include "VFolder.h"
#include "VResource.h"
#include "VTask.h"
#include "VIntlMgr.h"
#include "VErrorContext.h"
#include "VMemory.h"
#include "VMemoryCpp.h"
#include "VProgressIndicator.h"
#include "VTextConverter.h"
#include "ILogger.h"
#include "VJSONValue.h"
#include "VFileSystem.h"
#include <sys/types.h> // getpid
#if !VERSIONWIN
#include <unistd.h>
#endif

#if VERSIONMAC
	#include "crt_externs.h"
	#include "XMacResource.h"
#elif VERSION_LINUX
	//No resources on Linux !
	#include "XLinuxFsHelpers.h"
#elif VERSIONWIN
	#include "XWinResource.h"
#endif


// Class macros
#define MAIN		1
#define DUMP_LEAKS	0	// Set to 1 to dump leaks: available usind VERSIONDEBUG


// Class statics
VProcess*  VProcess::sInstance = NULL;

#if VERSIONWIN
HINSTANCE  VProcess::sWIN_AppInstance = NULL;
HINSTANCE  VProcess::sWIN_ToolboxInstance = NULL;
#endif




VProcess::VProcess()
: fSleepTicks(1)
#if WITH_RESOURCE_FILE
, fProcessResFile(NULL)
#endif
, fInitCalled(false)
, fInitOK(false)
, fFibered(false)
, fPreferencesInApplicationData(VERSIONMAC)
, fSystemID(0)
, fIntlManager(NULL)
, fLocalizer(NULL)
, fFileSystemNamespace(new VFileSystemNamespace())
{
	#if WITH_NEW_XTOOLBOX_GETOPT
	if (IsMemoryDebugEnabled()) // for reminder
	{
	}
	#else

	_FetchCommandLineArguments( fCommandLineArguments);

	sLONG val = 0;
	if (GetCommandLineArgumentAsLong( "-memDebugLeaks", &val))
	{
		VObject::GetAllocator()->SetWithLeaksCheck(val != 0);
	#if WITH_VMemoryMgr
		VMemory::GetManager()->SetWithLeaksCheck(val != 0);
	#endif
	}
	else
	{
		#if VERSIONDEBUG
		#if DUMP_LEAKS
			VObject::GetAllocator()->SetWithLeaksCheck(true);
			VMemory::GetManager()->SetWithLeaksCheck(true);
		#endif
		#endif
	}
	#endif

	_ReadProductName( fProductName);
	fProductFolderName = fProductName;

	_ReadProductVersion ( fProductVersion);

	xbox_assert(sInstance == NULL);	// set at init
}


VProcess::~VProcess()
{
	#if WITH_RESOURCE_FILE
	ReleaseRefCountable( &fProcessResFile);
	#endif

	ReleaseRefCountable( &fFileSystemNamespace);

	#if WITH_NEW_XTOOLBOX_GETOPT == 0
	sLONG val = 0;
	GetCommandLineArgumentAsLong( "-memDebugLeaks", &val);
	if (val != 0)
	{
		VObject::GetAllocator()->DumpLeaks();
		#if WITH_VMemoryMgr
		VMemory::GetManager()->DumpLeaks();
		#endif
	}
	else
	{
		#if VERSIONDEBUG
		#if DUMP_LEAKS
		VObject::GetAllocator()->DumpLeaks();
		VMemory::DumpLeaks();
		#endif
		#endif
	}
	#endif

	if (fInitCalled)
		_DeInit();
}


// Returns true if init was successfull
// Safe to be called twice during startup
// Override if you want to add custom initialization
// before any other initialisation
//
bool VProcess::Init( VProcess::InitOptions inOptions)
{
	if (!fInitCalled)
	{
		fInitCalled = true;
		fInitOK = _Init( inOptions);

	#if VERSION_LINUX

		//jmo - Init a reference timestamp so the number of ms returned
		//      by GetCurrentTime() is from /now/ and not from EPOCH.

		XLinuxSystem::InitRefTime();

	#endif
	}

	return fInitOK;
}



// Returns true if init was successfull
// Should be called once during startup
// Override if you want to add Toolbox initialization
// Note : memory managers are allready initialized (DLLMain)
bool VProcess::_Init( VProcess::InitOptions inOptions)
{
	bool	ok = _Init_CheckSystemVersion();

	sInstance = this;
	fFibered = (inOptions & Init_Fibered) != 0;

	// When a script convert table is lacking, use default one (JT 4/8/99 - ACI0009140)
	// Should notify the user...
	if (ok)
		fIntlManager = VIntlMgr::Create( DC_USER);
	if (ok)
		ok = VFile::Init();
	if (ok)
		ok = VFileKindManager::Init();
#if WITH_RESOURCE_FILE
	if (ok)
		ok = VResourceFile::Init() != 0;
#endif
	if (ok)
		ok = VTaskMgr::Init(this);
	if (ok)
		ok = VErrorBase::Init();
	if (ok)
		ok = VDebugMgr::Get()->Init();
	if (ok)
		ok = VProgressManager::Init();
	if (ok)
		ok = _Init_FileSystems();

#if VERSIONMAC
	fSystemID = getpid();
#elif VERSIONWIN
	fSystemID = GetCurrentProcessId();
#elif VERSION_LINUX
	fSystemID = getpid();
#endif


	// installe du debug message handler
	#if VERSIONDEBUG
	VNotificationDebugMessageHandler *handler = new VNotificationDebugMessageHandler;
	VDebugMgr::Get()->SetDebugMessageHandler( handler);
	ReleaseRefCountable( &handler);
	#endif

	if (inOptions & Init_EnableLogger)
	{
		fLogger.Adopt(new VLogger());
		fLogger->Start();
	}

	#if VERSION_LINUX
	//jmo - Init a reference timestamp so the number of ms returned
	//      by GetCurrentTime() is from /now/ and not from EPOCH.
	XLinuxSystem::InitRefTime();
	#endif

	return ok;
}


// Allways called (even if InitToolbox returned false)
// Should be called once during exit
// Note : memory managers will be deleted later (DLLMain)
void VProcess::_DeInit()
{
	#if WITH_ASSERT
	sLONG countJSONObjects = VJSONObject::GetInstancesCount();
	sLONG countJSONArrays = VJSONObject::GetInstancesCount();
	if ( (countJSONObjects != 0) || (countJSONArrays != 0) )
	{
		DebugMsg( "Remaining %d VJSONObject and %d VJSONArray\n", countJSONObjects, countJSONArrays);
	}
	#endif

	sInstance = NULL;

	VDebugMgr::Get()->DeInit();
	VErrorBase::DeInit();
	VTaskMgr::DeInit();
	#if WITH_RESOURCE_FILE
	VResourceFile::DeInit();
	#endif
	VFileKindManager::DeInit();
	VFile::DeInit();
	VProgressManager::Deinit();
	XBOX::ReleaseRefCountable( &fIntlManager);

	if (!fLogger.IsNull()) // ACI0086287 MoB: Crash with Python/ODBC
		fLogger->Stop();// not necessary since the included thread has already been stopped by RIAServer

	// Clean up the system resources used by ICU
	VIntlMgr::ICU_Cleanup();
}


bool VProcess::_Init_FileSystems()
{
	VError err = VE_OK;

	// add some system defined file systems
	VFolder *userFolder = VFolder::RetainSystemFolder( eFK_UserDocuments, false );
	if (userFolder != NULL)
	{
		err = fFileSystemNamespace->RegisterFileSystem( CVSTR( ".USER_HOME_DIRECTORY"), userFolder->GetPath(), eFSO_Private);
	}
	ReleaseRefCountable( &userFolder);

	if (err == VE_OK)
	{
		VFolder *bundleFolder = VProcess::Get()->RetainFolder( VProcess::eFS_Bundle);
		if (bundleFolder != NULL)
		{
			VFilePath modulesPath( bundleFolder->GetPath());
#if VERSIONMAC
			modulesPath.ToSubFolder( CVSTR( "Contents"));
#endif
			modulesPath.ToSubFolder( CVSTR( "Modules"));
			err = fFileSystemNamespace->RegisterFileSystem( CVSTR( "WAF-MODULES"), modulesPath, eFSO_Default);
		}
		ReleaseRefCountable( &bundleFolder);
	}

	if (err == VE_OK)
	{
		VFolder *resourcesFolder = RetainFolder( eFS_Resources);
		if (resourcesFolder != NULL)
		{
			VFilePath resourcesPath( resourcesFolder->GetPath());
			// Must be exposed as a private file system '.WAKANDA-RESOURCES' using eFSO_Private.
			// At the moment, the public toolbox API can resolve file paths within public file systems.
			// I will update path resolving to work with private file systems too and then will make the 
			// resources private.
			err = fFileSystemNamespace->RegisterFileSystem( CVSTR( "WAKANDA-RESOURCES"), resourcesPath, eFSO_Default);
		}
		ReleaseRefCountable( &resourcesFolder);
	}

	if (err == VE_OK)
	{
		// read definition of user defined file systems
		VFolder *folder = RetainFolder( eFS_Resources);
		VFile file( *folder, CVSTR( "fileSystems.json"), FPS_POSIX);
		if (file.Exists())
			err = fFileSystemNamespace->LoadFromDefinitionFile( &file);
		ReleaseRefCountable( &folder);
	}

	return err == VE_OK;
}


XBOX::VLogger* VProcess::GetLogger()
{
	return fLogger.Get();
}


VString VProcess::GetLogSourceIdentifier() const
{
	return fProductName;
}


void VProcess::Run()
{
	VTask *mainTask = VTask::GetMain();
	if (!testAssert( (mainTask != NULL) && mainTask->IsCurrent() && (mainTask->GetState() == TS_RUNNING) ))
		return;

	if (!fInitCalled)
		fInitOK = Init();

	if (fInitOK)
	{
		vFlushErrors();

		StartMessaging();

		DoRun();

		DoQuit();
		mainTask->Kill();

		StopMessaging();
	}
	else
	{
		vThrowError(VE_CANNOT_INITIALIZE_APPLICATION, "VApplication::DoInit failed");
	}
}


void VProcess::QuitAsynchronously( const VString *inMessage)
{
	VTask *task = VTask::GetMain();
	if (task != NULL)
	{
		task->Kill();

		// returns early from ExecuteMessagesWithTimeout
		task->SetMessageQueueEvent();
	}
}


bool VProcess::IsRunning()
{
	VTask *task = VTask::GetMain();
	return (task == NULL) ? false : !task->IsDying();
}


// Override to add custom handling when starting running
// This is the first action when running
void VProcess::DoRun()
{
}


// Override to add custom handling when stoping running
// This is the last action when running
void VProcess::DoQuit()
{
}


bool VProcess::_Init_CheckSystemVersion()
{
	SystemVersion minimalSystemVersion;

#if VERSIONWIN
	minimalSystemVersion = WIN_XP;
#elif VERSIONMAC
	minimalSystemVersion = MAC_OSX_10_6;
#elif VERSION_LINUX
    return true;	// Postponed Linux Implementation !
#else
	assert_compile( false);
#endif

	return DoCheckSystemVersion( minimalSystemVersion);
}


bool VProcess::DoCheckSystemVersion( SystemVersion inMinimalSystemVersion)
{
	bool ok = VSystem::IsSystemVersionOrAbove( inMinimalSystemVersion);
	if (!ok)
	{
#if VERSIONWIN
		VString msg = (inMinimalSystemVersion == WIN_XP) ? "This application needs at least Windows XP" : "This application needs at least Windows Vista";
#elif VERSIONMAC
		VString msg = (inMinimalSystemVersion == MAC_OSX_10_5) ? "This application needs at least OS X 10.5" : "This application needs at least OS X 10.6";
#elif VERSION_LINUX
		VString msg="Need a Linux Implementation !";	// Postponed Linux Implementation !
#endif
		VSystem::DisplayNotification( CVSTR( "Initialization failed"), msg, EDN_OK);
	}
	return ok;
}

#if WITH_RESOURCE_FILE
VResourceFile* VProcess::RetainResourceFile()
{
	if (fProcessResFile == NULL)
	{
	#if VERSIONMAC
		#if !__LP64__
		fProcessResFile = new VMacResFile(::CurResFile());
		((VMacResFile*)fProcessResFile)->SetUseResourceChain();
		#endif
	#else
		fProcessResFile = new VWinResFile(sWIN_AppInstance);
	#endif
	}

	return RetainRefCountable( fProcessResFile);
}
#endif

VFilePath VProcess::GetExecutableFilePath() const
{
	VFilePath filePath;

#if VERSIONMAC

	CFURLRef exeURL = ::CFBundleCopyExecutableURL( ::CFBundleGetMainBundle());

	if (testAssert( exeURL != NULL ))
	{
		CFStringRef cfPath = ::CFURLCopyFileSystemPath( exeURL, kCFURLHFSPathStyle);
		if (testAssert( cfPath != NULL ))
		{
			VString thepath;
			thepath.MAC_FromCFString( cfPath);
			thepath.Compose();
			filePath.FromFullPath( thepath, FPS_SYSTEM);
			::CFRelease( cfPath);
		}

		::CFRelease(exeURL );
	}

#elif VERSIONWIN

	// Get a path to the exe.
	UniChar path[4096];
	DWORD pathLength=0;

	path[sizeof(path)/sizeof(UniChar)-2]=0;
	pathLength = ::GetModuleFileNameW(NULL, path, sizeof(path));

	if (testAssert(pathLength != 0 && !path[sizeof(path)/sizeof(UniChar)-2]))
	{
		VString thepath( path);
		filePath.FromFullPath( thepath, FPS_SYSTEM);
	}

#elif VERSION_LINUX

	PathBuffer path;

	VError verr=path.InitWithExe();
	xbox_assert(verr==VE_OK);

	path.ToPath(&filePath);
	xbox_assert(verr==VE_OK);

#endif

	return filePath;
}


VFile* VProcess::RetainExecutableFile() const
{
	return new VFile( GetExecutableFilePath());
}


VFilePath VProcess::GetExecutableFolderPath() const
{
	return GetExecutableFilePath().ToParent();
}


VFolder* VProcess::RetainFolder( VProcess::EFolderKind inSelector) const
{
	VFolder* folder = NULL;

	switch (inSelector)
	{
		case eFS_Executable:
			folder = new VFolder( GetExecutableFolderPath());
			break;

		case eFS_Bundle:
			#if VERSIONWIN || VERSION_LINUX
			folder = new VFolder( GetExecutableFolderPath());
			#elif VERSIONMAC
			folder = new VFolder( GetExecutableFolderPath().ToParent().ToParent());
			#endif
			break;

		case eFS_Resources:
			{
				#if VERSIONWIN || VERSION_LINUX
				VFolder *parent = new VFolder( GetExecutableFolderPath());
				#elif VERSIONMAC
				VFolder *parent = new VFolder( GetExecutableFolderPath().ToParent());	// Contents
				#endif

				folder = new VFolder( *parent, CVSTR( "Resources"));
				ReleaseRefCountable( &parent);
				break;
			}

		case eFS_InfoPlist:
			{
				// Resources folder on windows.
				// Contents folder on mac.
				#if VERSIONWIN || VERSION_LINUX
				folder = RetainFolder( eFS_Resources);
				#elif VERSIONMAC
				folder = new VFolder( GetExecutableFolderPath().ToParent());
				#endif
				break;
			}

		case eFS_NativeComponents:
			{
				#if VERSIONWIN || VERSION_LINUX
				folder = new VFolder( GetExecutableFolderPath().ToSubFolder( CVSTR( "Native Components")));
				#elif VERSIONMAC
				folder = new VFolder( GetExecutableFolderPath().ToParent().ToSubFolder( CVSTR( "Native Components")));
				#endif

				// in debug mode the "Native Components" might be next the bundle folder to be shared among mono, server, studio, server
				#if VERSIONDEBUG
				if (!folder->Exists())
				{
					ReleaseRefCountable( &folder);
					#if VERSIONWIN || VERSION_LINUX
					folder = new VFolder( GetExecutableFolderPath().ToParent().ToSubFolder( CVSTR( "Native Components")));
					#elif VERSIONMAC
					folder = new VFolder( GetExecutableFolderPath().ToParent().ToParent().ToParent().ToSubFolder( CVSTR( "Native Components")));
					#endif
				}
				#endif

				break;
			}

		case eFS_DTDs:
			{
				VFolder *parent = RetainFolder( eFS_Resources);
				if (parent != NULL)
					folder = new VFolder( *parent, CVSTR( "DTD"));
				ReleaseRefCountable( &parent);
				break;
			}

		case eFS_XSDs:
			{
				VFolder *parent = RetainFolder( eFS_Resources);
				if (parent != NULL)
					folder = new VFolder( *parent, CVSTR( "XSD"));
				ReleaseRefCountable( &parent);
				break;
			}

		case eFS_XSLs:
			{
				VFolder *parent = RetainFolder( eFS_Resources);
				if (parent != NULL)
					folder = new VFolder( *parent, CVSTR( "XSL"));
				ReleaseRefCountable( &parent);
				break;
			}

		default:
			break;
	}

	return folder;
}


VFile *VProcess::RetainFile( EFolderKind inPrimarySelector, const VString& inFileName, EFolderKind inSecondarySelector) const
{
	VFile *file = NULL;

	VFolder *parent = RetainFolder( inPrimarySelector);
	if (testAssert( parent != NULL))
	{
		file = new VFile( *parent, inFileName);
		if ( (file != NULL) && !file->Exists())
			ReleaseRefCountable( &file);
	}
	ReleaseRefCountable( &parent);

	if ( (file == NULL) && (inSecondarySelector != 0) )
	{
		parent = RetainFolder( inSecondarySelector);
		if ( (parent != NULL) && parent->Exists())
		{
			file = new VFile( *parent, inFileName);
			if ( (file != NULL) && !file->Exists())
				ReleaseRefCountable( &file);
		}
		ReleaseRefCountable( &parent);
	}

	return file;
}


VFolder *VProcess::RetainProductSystemFolder( ESystemFolderKind inSelector, bool inCreateFolderIfNotFound) const
{
	VFolder *folder = NULL;

	bool allUsers;
	bool useProductSubFolder;
	ESystemFolderKind currentUserFolder;
	switch( inSelector)
	{
		case eFK_UserDocuments:
		case eFK_Temporary:
			allUsers = true;
			useProductSubFolder = true;
			break;

		case eFK_CommonPreferences:
			currentUserFolder = eFK_UserPreferences;
			allUsers = true;
			useProductSubFolder = true;
			break;

		case eFK_CommonApplicationData:
			currentUserFolder = eFK_UserApplicationData;
			allUsers = true;
			useProductSubFolder = true;
			break;

		case eFK_CommonCache:
			currentUserFolder = eFK_UserCache;
			allUsers = true;
			useProductSubFolder = true;
			break;

		case eFK_UserPreferences:
		case eFK_UserApplicationData:
		case eFK_UserCache:
			allUsers = false;
			useProductSubFolder = true;
			break;

		default:
			allUsers = false;
			useProductSubFolder = false;
			break;
	}

	if (useProductSubFolder)
	{
		VFolder *parent = VFolder::RetainSystemFolder( inSelector, inCreateFolderIfNotFound);
		if (parent != NULL)
		{
			VError err;
			VString folderName;
			GetProductFolderName( folderName);

			// accepte un alias
			VString aliasName( folderName);
			#if VERSIONWIN
			aliasName += ".lnk";
			#endif
			VFile aliasFile( *parent, aliasName);
			if (aliasFile.Exists() && aliasFile.IsAliasFile())
			{
				VFilePath target;
				err = aliasFile.ResolveAlias( target);
				if ( (err == VE_OK) && target.IsFolder() )
				{
					folder = new VFolder( target);
				}
			}

			if (folder == NULL)
			{
				folder = new VFolder( *parent, folderName);
			}

			if (!folder->Exists())
			{
				if (inCreateFolderIfNotFound)
				{
					if (allUsers)
					{
						// drop errors if we can retry in current user home directory
						StErrorContextInstaller context( false);
						err = folder->CreateRecursive( allUsers);
					}
					else
					{
						err = folder->CreateRecursive( allUsers);
					}
					if (err != VE_OK)
					{
						ReleaseRefCountable( &folder);

						// si on nous a demande de creer dans "all users", on ressaie dans le dossier de l'utilisateur courant
						if (allUsers)
						{
							folder = RetainProductSystemFolder( currentUserFolder, inCreateFolderIfNotFound);
						}
					}
				}
				else
				{
					ReleaseRefCountable( &folder);
				}
			}
		}

		ReleaseRefCountable( &parent);
	}
	else
	{
		folder = VFolder::RetainSystemFolder( inSelector, inCreateFolderIfNotFound);
	}

	return folder;
}


#if WITH_NEW_XTOOLBOX_GETOPT == 0
/*
	static
*/
void VProcess::_FetchCommandLineArguments( VectorOfVString& outArguments)
{
	VectorOfVString arguments;

#if VERSIONWIN

	int	nbArgs = 0;
	LPWSTR *tabArgs = ::CommandLineToArgvW( ::GetCommandLineW(), &nbArgs);
	for( int i = 0 ; i < nbArgs ; ++i)
	{
		arguments.push_back( VString( tabArgs[i]));
	}
	LocalFree( tabArgs);

#else

	#if VERSIONMAC
	int *argc_p = _NSGetArgc();
	char ***argv_p = _NSGetArgv();
	int argc = (argc_p == NULL) ? 0 : *argc_p;
	char **argv = (argv_p == NULL) ? NULL : *argv_p;
	#elif VERSION_LINUX
    // Postponed Linux Implementation !
	//extern int __libc_argc; jmo - Theese symbols are not available for us !
	//extern char ** __libc_argv;
	int argc = 0;
	char **argv = NULL;
	#endif

	VToUnicodeConverter_UTF8 converter;
	for( int i = 0 ; i < argc ; ++i)
	{
		VString s;
		converter.ConvertString( argv[i], strlen( argv[i]), NULL, s);
		arguments.push_back( s);
	}

#endif

	outArguments.swap( arguments);
}
#endif




#if WITH_NEW_XTOOLBOX_GETOPT == 0
/*
	static
*/
bool VProcess::GetCommandLineArgumentAsLong( const char* inArgumentName, sLONG* outValue)
{
	// in early condition, VProcess may not exist yet and wan't use VString
	if (sInstance == NULL)
		return _GetCommandLineArgumentAsLongStatic( inArgumentName, outValue);

	bool isFound = false;
	*outValue = 0;

	for( VectorOfVString::const_iterator i = sInstance->fCommandLineArguments.begin() ; i != sInstance->fCommandLineArguments.end() ; ++i)
	{
		if (i->EqualToUSASCIICString( inArgumentName))
		{
			if (++i != sInstance->fCommandLineArguments.end())
			{
				*outValue = i->GetLong();
				isFound = true;
			}
			break;
		}
	}
	return isFound;
}
#endif


#if WITH_NEW_XTOOLBOX_GETOPT == 0
/*
	static

	warning: this method is to be called very early at time when VProcess and VCppMem is not initialized yet.
	Don't create any VObject here nor call any other xbox method.
*/
bool VProcess::_GetCommandLineArgumentAsLongStatic( const char* inArgumentName, sLONG* outValue)
{
	bool isFound = false;
	*outValue = 0;

#if VERSIONWIN
	wchar_t wideArgumentName[256];
	size_t len = strlen(inArgumentName);
	if (len > 255)
		len = 255;
	std::copy( inArgumentName, inArgumentName + len + 1, &wideArgumentName[0]);

	int	argc = 0;
	LPWSTR *argv = ::CommandLineToArgvW( ::GetCommandLineW(), &argc);
	for( int i = 0 ; i < argc ; ++i)
	{
		if (wcscmp(argv[i], wideArgumentName) == 0)
		{
			if (++i < argc)
			{
				*outValue = ::_wtol(argv[i]);
				isFound = true;
				break;
			}
		}
	}
	LocalFree( argv);
#else
	#if VERSIONMAC
	int *argc_p = _NSGetArgc();
	char ***argv_p = _NSGetArgv();
	int argc = (argc_p == NULL) ? 0 : *argc_p;
	char **argv = (argv_p == NULL) ? NULL : *argv_p;
	#elif VERSION_LINUX
    // Postponed Linux Implementation !
	//extern int __libc_argc; jmo - Theese symbols are not available for us !
	//extern char ** __libc_argv;
	int argc = 0;
	char **argv = NULL;
	#endif

	for( int i = 0 ; i < argc ; ++i)
	{
		if (strcmp(argv[i], inArgumentName) == 0)
		{
			if (++i < argc)
			{
				*outValue = (sLONG) ::atol(argv[i]);
				isFound = true;
				break;
			}
		}
	}

#endif

	return isFound;
}
#endif



void VProcess::_ReadProductName( VString& outName) const
{
#if VERSIONWIN
#ifndef XTOOLBOX_AS_STANDALONE
	DWORD size = ::GetFileVersionInfoSizeW( GetExecutableFilePath().GetPath().GetCPointer(), NULL);
	void *buffer = malloc( size);
	if ( (buffer != NULL) && GetFileVersionInfoW( GetExecutableFilePath().GetPath().GetCPointer(), NULL, size, buffer))
	{
		void *valueAdress;
		UINT valueSize;
		if (::VerQueryValueW( buffer, L"\\StringFileInfo\\040904B0\\ProductName", &valueAdress, &valueSize))
		{
			outName.FromBlock( valueAdress, (valueSize - 1) * sizeof( UniChar), VTC_UTF_16);
		}
	}
	if (buffer != NULL)
		free( buffer);
#endif
#elif VERSIONMAC
	CFStringRef cfProductName = (CFStringRef) CFBundleGetValueForInfoDictionaryKey( CFBundleGetMainBundle(), kCFBundleNameKey);
	if (cfProductName != NULL)
		outName.MAC_FromCFString( cfProductName);

#elif VERSION_LINUX

	//Product name should be set in main() with something like this
	//VXxxApplication::Get()->SetProductName(xxx);

#endif
}


void VProcess::_ReadProductVersion( VString& outVersion) const
{
#if VERSIONWIN
#ifndef XTOOLBOX_AS_STANDALONE
	DWORD size = ::GetFileVersionInfoSizeW( GetExecutableFilePath().GetPath().GetCPointer(), NULL);
	void *buffer = malloc( size);
	if ( (buffer != NULL) && GetFileVersionInfoW( GetExecutableFilePath().GetPath().GetCPointer(), NULL, size, buffer))
	{
		void *valueAdress;
		UINT valueSize;
		if (::VerQueryValueW( buffer, L"\\StringFileInfo\\040904B0\\ProductVersion", &valueAdress, &valueSize))
		{
			outVersion.FromBlock( valueAdress, (valueSize - 1) * sizeof( UniChar), VTC_UTF_16);
		}
	}
	if (buffer != NULL)
		free( buffer);
#endif
#elif VERSIONMAC
	// The constant for the "short version" property does not exist.
	CFStringRef cfProductVersion = (CFStringRef) CFBundleGetValueForInfoDictionaryKey( CFBundleGetMainBundle(), CFSTR ( "CFBundleShortVersionString" ) /*kCFBundleVersionKey*/);
	if (cfProductVersion != NULL)
	{
		outVersion.MAC_FromCFString( cfProductVersion);
	}

#elif VERSION_LINUX
    //jmo - We get the Linux product version with SetProductVersion() on VRIAServerApplication init
#endif
}



/*
	IMPORTANT:	the build version is created on the compilation machine. It requires that (1) perl is installed and (2) an external file contains the number.
				That's why if you compile 4D on your computer, you'll probably always get a build version of 0 (if the external file is not found, "00000" is used)
*/
sLONG VProcess::GetChangeListNumber() const
{
	sLONG changeListNumber = 0;
	VString s;
	GetBuildNumber( s);
	VIndex pos = s.FindUniChar( '.', s.GetLength(), true);
	if (pos > 0)
	{
		s.SubString( pos + 1, s.GetLength() - pos);
		changeListNumber = s.GetLong();
	}
	return changeListNumber;
}


void VProcess::GetBuildNumber( VString& outName) const
{
	// product version string should be formatted as follows: 1.0 build 2.108412
	VIndex pos = fProductVersion.Find( "build ");
	if (pos > 0)
	{
		fProductVersion.GetSubString( pos + 6, fProductVersion.GetLength() - pos - 5, outName);
		outName.RemoveWhiteSpaces();
	}
	else
	{
		outName.Clear();
	}
}


void VProcess::GetShortProductVersion( VString& outVersion) const
{
	VIndex pos = fProductVersion.Find( "build ");
	if (pos > 0)
	{
		fProductVersion.GetSubString( 1, pos-1, outVersion);
	}
	else
	{
		outVersion = fProductVersion;
	}
	outVersion.RemoveWhiteSpaces();
}

void VProcess::GetMainProductVersion( VString& outVersion) const
{
	GetShortProductVersion( outVersion );

	VIndex pos = outVersion.FindUniChar( '.' );
	if (pos > 0)
	{
		outVersion.SubString( 1, pos - 1 );
	}
}


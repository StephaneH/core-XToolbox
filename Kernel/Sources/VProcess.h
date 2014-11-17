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
#ifndef __VProcess__
#define __VProcess__

#include "Kernel/Sources/VObject.h"
#include "Kernel/Sources/VMessage.h"
#include "Kernel/Sources/VFolder.h"
#include "Kernel/Sources/VLogger.h"


BEGIN_TOOLBOX_NAMESPACE

#if VERSIONWIN || VERSIONMAC
	#define WITH_RESOURCE_FILE 1
#endif

// Needed declaration
class VFile;
class VFolder;
class VFilePath;
#if WITH_RESOURCE_FILE
class VResourceFile;
#endif
class VIntlMgr;
class ILocalizer;
class ILogger;
class VFileSystemNamespace;




class XTOOLBOX_API VProcess : public VObject, public virtual IMessageable
{
public:
	enum
	{
		Init_WithQuickTime		= 1,
		Init_SDI_Application	= 2,
		Init_Fibered			= 4,

		Init_EnableD2D					= 8,
		Init_EnableD2DHardware			= 16,
		Init_EnableLogger				= 32,
		Init_UseCocoaMenubar			= 64,
		Init_WithFileSystemNotifier		= 128,
		#if ARCH_64
		Init_Default			= Init_EnableLogger | Init_WithFileSystemNotifier,
		#else
		Init_Default			= Init_EnableLogger | Init_WithFileSystemNotifier | Init_WithQuickTime,
		#endif
	};
	typedef uLONG InitOptions;

	enum
	{
		eFS_Executable			= 'EXEC',	// folder for executable binary (Contents/MacOS for mac)
		eFS_Bundle				= 'BNDL',	// top level folder for application folder (the bundle on mac and the executable folder on windows)
		eFS_Resources			= 'RSRC',	// the Resources folder
		eFS_InfoPlist			= 'PLST',	// where to find the info.plist file
		eFS_NativeComponents	= 'NCMP',	// the Native components folder
		eFS_DTDs				= 'DTDs',	// where to find xml grammars
		eFS_XSDs				= 'XSDs',	// where to find xml schemas
		eFS_XSLs				= 'XSLs'	// where to find xsl
	};
	typedef uLONG	EFolderKind;


public:
								VProcess();
	virtual						~VProcess();

	static	VProcess*			Get()											{ return sInstance; }

	virtual	bool				Init(InitOptions inOptions = Init_Default);

			void				Run();
	static	bool				IsRunning();

	virtual	void				QuitAsynchronously( const VString *inMessage = NULL);

			bool				IsFibered() const								{ return fFibered; }

			uLONG				GetSystemID() const								{ return fSystemID; }

	#if WITH_RESOURCE_FILE
	virtual	VResourceFile*		RetainResourceFile();
	#endif
			VFile*				RetainExecutableFile() const;

			VFilePath			GetExecutableFilePath() const;
			VFilePath			GetExecutableFolderPath() const;


	// returns folders for various parts of the application.
	virtual	VFolder*			RetainFolder( EFolderKind inSelector) const;

	// Facility to return a file in a folder. inSecondarySelector is used if the file can't be found in primary selector.
	// Returns NULL if file doesn't exist.
			VFile*				RetainFile( EFolderKind inPrimarySelector, const VString& inFileName, EFolderKind inSecondarySelector = 0) const;

	// returns system folders using a sub folder whose name is the company name.
	// ex: eFK_UserApplicationData may return /users/goofy/library/preferences/4d for user goofy using 4d products.
	// If being asked a folder for all users and it can't find it nor create it, it will try the same folder in the user directory.
			VFolder*			RetainProductSystemFolder( ESystemFolderKind inSelector, bool inCreateFolderIfNotFound) const;

			VFileSystemNamespace*	GetFileSystemNamespace() const				{ return fFileSystemNamespace;}

			ILocalizer*			GetLocalizer() const							{ return fLocalizer;}
			void				SetLocalizer( ILocalizer *inLocalizer)			{ fLocalizer = inLocalizer;}


			// returns the product name as stored in the version information or overloaded using SetProductName
			// ex: "4D v14 R"
			void				GetProductName( VString& outName) const			{ outName = fProductName;}
			void				SetProductName( const VString& inName)			{ fProductName = inName;}

			// returns the product name used to name product folders like the one for preferences.
			// ex: "4D"
			void				GetProductFolderName( VString& outName) const	{ outName = fProductFolderName;}
			// set the folder name used to store product preferences.
			// by default this is the Product Name as defined in version resources.
			void				SetProductFolderName(const VString& inName)		{ fProductFolderName = inName; }

			// returns the product version as exposed by OS.
			// usually is something like 2.0 build 2.115342
			void				GetProductVersion( VString& outVersion) const	{ outVersion = fProductVersion;}
			void				SetProductVersion( const VString& inVersion)	{ fProductVersion = inVersion;}

			// returns the product version string minus the build number information, if any
			void				GetShortProductVersion( VString& outVersion) const;

			// returns the main product version string ie. if version string is 12.4 build 12345 main product version will be 12
			void				GetMainProductVersion( VString& outVersion) const;

			// Retreive the sccs changelist sequence number this code comes from
			sLONG				GetChangeListNumber() const;

			// Retreive the build number identifier (usually made from the branch index and the changelist) from the product version string
			void				GetBuildNumber( VString& outName) const;

	/*
		On mac we used to store preferences in /Library/Preferences but the AppStore guidelines we should now store them in /Library/ApplicationSupport
		By default VFolder::RetainSystemFolder will now use the new location only on MAC.
		4D v12 will switch to the new location only for AppStore tagged applications.
	*/
			void				SetPreferencesInApplicationData( bool inInAppData)	{ fPreferencesInApplicationData = inInAppData;}
			bool				GetPreferencesInApplicationData() const				{ return fPreferencesInApplicationData;}

			XBOX::VLogger*		GetLogger();
			//	This string identifies the log messages source at process level. All identifiers of sources of log messages should be prefixed with it.
	virtual	VString				GetLogSourceIdentifier() const;

	#if VERSIONWIN
	static	HINSTANCE			WIN_GetAppInstance()							{ return sWIN_AppInstance; }
	static	void				WIN_SetAppInstance( HINSTANCE inInstance)		{ sWIN_AppInstance = inInstance; }
	static	HINSTANCE			WIN_GetToolboxInstance()						{ return sWIN_ToolboxInstance; }
	static	void				WIN_SetToolboxInstance( HINSTANCE inInstance)	{ sWIN_ToolboxInstance = inInstance; }
	#endif

	#if WITH_NEW_XTOOLBOX_GETOPT
	// temporary function to keep traces of all locations that require
	// info about memory debugging
	static bool IsMemoryDebugEnabled() { return false; }
	#else

	// Returns the command line arguments as an array of VString.
	// The first VString is usually the executable path.
	const VectorOfVString&	GetCommandLineArguments() const				{ return fCommandLineArguments;}

	/*
		Facility to look for an named integer argument in the command line arguments passed to the executable.
		Returns in outValue the integer that follows the name passed as argument name (case sensitive).
		If not found the method returns false and sets outValue to zero.

		Note that GetCommandLineArgumentAsLong() is static and can be called very early before VProcess instance exists.
		This is not true for GetCommandLineArguments().

		example: app.exe -mode 42 -option 123
		GetCommandLineArgumentAsLong( "-mode") returns 42
		GetCommandLineArgumentAsLong( "-option") returns 123
	*/
	static	bool				GetCommandLineArgumentAsLong( const char *inArgumentName, sLONG* outValue);
	#endif

	static	void				_ProcessEntered( VProcess* inProcess)			{ sInstance = inProcess; }
	static	void				_ProcessExited()								{ sInstance = NULL; }

protected:
	friend class VTask;
	friend class VTaskMgr_Root;
	friend class VDaemon;
	friend class VApplication;

	// used only for cloning for new VTask. getting it must be thread safe.
			VIntlMgr*			GetIntlManager() const							{ return fIntlManager;}

	virtual void				DoRun();
	virtual void				DoQuit();

	// Called early to check if system and application bundle is correct
	virtual	bool				DoCheckSystemVersion( SystemVersion inMinimalSystemVersion);

private:
			bool				_Init( VProcess::InitOptions inOptions);
			bool				_Init_CheckSystemVersion();
			bool				_Init_FileSystems();
			void				_DeInit();
			void				_ReadProductName( VString& outName) const;
			void				_ReadProductVersion( VString& outVersion) const;

	#if WITH_NEW_XTOOLBOX_GETOPT
	#else
	static	void				_FetchCommandLineArguments( VectorOfVString& outArguments);
	static	bool				_GetCommandLineArgumentAsLongStatic( const char *inArgumentName, sLONG* outValue);
	#endif

			sLONG					fSleepTicks;
	#if WITH_RESOURCE_FILE
			VResourceFile*			fProcessResFile;
	#endif
			bool					fInitCalled;
			bool					fInitOK;
			bool					fFibered;
			bool					fPreferencesInApplicationData;	// AppStore compatible way of storing preferences
			uLONG					fSystemID;
			VIntlMgr*				fIntlManager;
			ILocalizer*				fLocalizer;
			VFileSystemNamespace*	fFileSystemNamespace;

			VString					fProductName;
			VString					fProductVersion;
			VString					fProductFolderName;		// Name of the folder to store application preferences files (not localized but can be customized by 4D OEM customers)

	#if WITH_NEW_XTOOLBOX_GETOPT
	#else
			VectorOfVString			fCommandLineArguments;
	#endif
			VRefPtr<VLogger>		fLogger;


	static	VProcess*				sInstance;
	#if VERSIONWIN
	static	HINSTANCE				sWIN_AppInstance;
	static	HINSTANCE				sWIN_ToolboxInstance;
	#endif

 }; // class VProcess





END_TOOLBOX_NAMESPACE

#endif


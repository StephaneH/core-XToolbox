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
#ifndef __VLibrary__
#define __VLibrary__

#include "Kernel/VKernel.h"

BEGIN_TOOLBOX_NAMESPACE


// Class constants
typedef enum BundleFolderKind
{
	kBF_BUNDLE_FOLDER	= 1,		// The folder hosting the bundle
	kBF_EXECUTABLE_FOLDER,			// The folder of executable file (platform related)
	kBF_RESOURCES_FOLDER,			// The folder of main resources
	kBF_LOCALISED_RESOURCES_FOLDER,	// The folder of prefered localization
	kBF_PLUGINS_FOLDER,				// The folder for bundle's plugins
	kBF_PRIVATE_FRAMEWORKS_FOLDER,	// The folder for private frameworks
	kBF_PRIVATE_SUPPORT_FOLDER		// The folder for private support files
} BundleFolderKind;


END_TOOLBOX_NAMESPACE

// Native includes
#if VERSIONMAC
    #include "Kernel/Sources/XMacLibrary.h"
#elif VERSIONWIN
    #include "Kernel/Sources/XWinLibrary.h"
#elif VERSION_LINUX
	#include "Kernel/Sources/XLinuxLibrary.h"
#endif

BEGIN_TOOLBOX_NAMESPACE


/*!
	@discussion
			Thread-safe
*/


class XTOOLBOX_API VLibrary : public VObject, public IRefCountable
{
public:
								VLibrary( const VFilePath& inPath);	// May be a file or bundle folder
	virtual						~VLibrary();
	
			// API accessors
			bool				Load();
			void				Unload();

			void*				GetProcAddress( const VString& inProcname);
			
			// Bundle content accessors (you must release the result)
			VFolder*			RetainFolder( BundleFolderKind inKind = kBF_BUNDLE_FOLDER) const;
			VFile*				RetainExecutableFile() const;
			
			// You can use this proc as a compare method
			bool				IsSameFile( const VFilePath& inLibFile) const;
			
			// Returns a ResourceFile object from the library's resource fork if any
			VResourceFile*		RetainResourceFile();
			
			#if VERSIONMAC
			CFBundleRef			MAC_GetBundleRef() const ;
			#elif VERSIONWIN
			HINSTANCE			WIN_GetInstance()  const ;
			#endif
	
protected:
	mutable	VCriticalSection	fCriticalSection;
			VFilePath			fFilePath;
			XLibraryImpl		fImpl;
			VResourceFile*		fResFile;
			bool				fResFileChecked;
			bool				fIsLoaded;
};

END_TOOLBOX_NAMESPACE

#endif

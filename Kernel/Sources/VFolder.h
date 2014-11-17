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
#ifndef __VFolder__
#define __VFolder__

BEGIN_TOOLBOX_NAMESPACE
/*!
		Enum use to specify the location of : the executable folder or the working folder.
	In some cases talking of the executable is not the same as talking of the application. 
	In the Bundle architecture on Mas OS, the executable is located inside the "application bundle".
	That means that the working folder is not the folder that contains the executable file 
	but the application bundle.
*/
typedef enum
{ 
	eFK_Executable,
	eFK_UserDocuments,
	eFK_Temporary,
	eFK_StartupItemsFolder,		// startup items folders at boot time

	// preferences are non-valuable settings
	eFK_CommonPreferences,		// preferences common to all users
	eFK_UserPreferences,		// preferences for current user

	// application data can be a dictionnary, licences, plugins, ...
	eFK_CommonApplicationData,	// application data common to all users
	eFK_UserApplicationData,	// application data for current user

	// cache folder can be emptied by the system or utilities at boot time
	// it is non-roaming.
	eFK_CommonCache,			// cache folder common to all users
	eFK_UserCache,				// cache folder for current user

	#if VERSIONMAC
	eFK_MAC_UserLibrary,		// ~/Library
	#endif
	
	eFK_void	// just to end the comma delimited list

} ESystemFolderKind;

END_TOOLBOX_NAMESPACE

#include "Kernel/Sources/VSyncObject.h"


#if VERSIONMAC
#include "Kernel/Sources/XMacFolder.h"
#elif VERSION_LINUX
#include "Kernel/Sources/XLinuxFolder.h"
#elif VERSIONWIN
#include "Kernel/Sources/XWinFolder.h"
#endif

BEGIN_TOOLBOX_NAMESPACE

// Needed declarations
class VArrayLong;
class VArrayString;
class VFileSystem;

class XTOOLBOX_API VFolder : public VObject, public IRefCountable
{ 
public:
								VFolder( const VFolder& inSource);
								
								// inRelativePath may be just a folder name or some relative path to a sub folder.
								// if inRelativePath doesn't end with a folder separator, one is added.
	explicit					VFolder( const VFolder& inParentFolder, const VString& inRelativePath, FilePathStyle inRelativePathStyle = FPS_DEFAULT);

								// full path must end with a folder separator
	explicit					VFolder( const VString& inFullPath, FilePathStyle inPathStyle = FPS_DEFAULT);

	// It's is your responsibility to ensure that you pass a syntaxically correct folder path.
	// else you'll get an assert.
	// VFileSystem is optionnal and retained. It's there for conveniency for sandboxing support.
	explicit					VFolder( const VFilePath& inPath, VFileSystem *inFileSystem = NULL);

	#if VERSIONMAC
	explicit					VFolder( const FSRef& inRef);
	#endif

	virtual						~VFolder();

	// Creates a VFolder object and immediately tries to resolve it using ResolveAliasFolder.
	// If the file exists as an alias file one couldn't resolve as a folder, it tries to delete it.
	// It only throws an error if the alias file is invalid and one couldn't delete it.
	static	VFolder*			CreateAndResolveAlias( const VFolder& inParentFolder, const VString& inFolderName);

			// Creates the folder on disk.
			VError				Create() const;

			// Creates all necessary subfolder if needed.
			VError				CreateRecursive(bool inMakeWritableByAll = false);

            VError				DeleteContents( bool inRemoveRecursive ) const;
			VError				Delete( bool inRemoveContent ) const;

            VError				MoveToTrash() const;
            
			// Tell if the folder exists.
			bool				Exists() const;
	
			void				GetContents(std::vector< VRefPtr < VFile > >& outFiles, std::vector< VRefPtr < VFolder > >& outFolders) const;
			VArrayString*		GetContents(FolderFilter inFolderFilter) const;
			bool				IsEmpty() const;
			bool				Contains(const XBOX::VString& inExtension) const;
			
			VError				GetTimeAttributes( VTime* outLastModification = NULL, VTime* outCreationTime = NULL, VTime* outLastAccess = NULL ) const;
			VError				SetTimeAttributes( const VTime *inLastModification = NULL, const VTime *inCreationTime = NULL, const VTime *inLastAccess = NULL ) const;

			VError				GetVolumeCapacity(sLONG8* outTotalBytes)const;
			VError				GetVolumeFreeSpace( sLONG8 *outFreeBytes, bool inWithQuotas = true ) const;
			
#if !VERSION_LINUX
			bool				RevealInFinder(bool inInside=false) const;
#endif

			VError				Rename( const VString& inName, VFolder** outFolder ) const;
			
			// Move this folder and its contents inside destination folder.
			// Destination folder must exist and must not contain a folder with same name.
			VError				Move( const VFolder& inNewParent, VFolder** outFolder ) const;
			VError				Move( const VFolder& inNewParent, const VString& inNewName, VFolder** outFolder ) const;
			
			// Copy this folder and its contents recursively inside destination folder.
			// Destination folder must exist.
			// outFolder (may be NULL) returns new folder.
			// if a folder with the same name already exists in destination folder, it is first deleted if FCP_Overwrite is passed else an error is returned.
			VError				CopyTo( const VFolder& inDestinationFolder, VFolder **outFolder, FileCopyOptions inOptions = FCP_Default ) const;
			VError				CopyTo( const VFolder& inDestinationFolder, const VString& inNewName, VFolder **outFolder, FileCopyOptions inOptions = FCP_Default ) const;
			
			// Copy this folder contents recursively inside destination folder.
			// Destination folder is created if necessary (but not recursive).
			// if files in source folder already exist in destination folder, they are replaced if FCP_Overwrite is passed else an error is returned.
			VError				CopyContentsTo( const VFolder& inDestinationFolder, FileCopyOptions inOptions = FCP_Default ) const;
			
			// Retain parent folder.
			// If root has been reached returns NULL.
			// In sandboxed mode, the root is the file system root (if any).
			VFolder*			RetainParentFolder( bool inSandBoxed = false) const;
			
			// Retain relative folder using a relative path.
			// inRelativePath may be just a folder name or some relative path to a sub folder.
			// if inRelativePath doesn't end with a folder separator, one is added.
			// If resulting full path is invalid, NULL is returned (no error is thrown)
			// If there's a fileSystem, and the relative path gets out of the fileSystem root, NULL is returned.
			VFolder*			RetainRelativeFolder( const VString& inRelativePath, FilePathStyle inRelativePathStyle = FPS_DEFAULT) const;

			// Retain relative fiel using a relative path.
			// inRelativePath may be just a file name or some relative path to a sub file.
			// If resulting full path is invalid, NULL is returned (no error is thrown)
			// If there's a fileSystem, and the relative path gets out of the fileSystem root, NULL is returned.
			VFile*				RetainRelativeFile( const VString& inRelativePath, FilePathStyle inRelativePathStyle = FPS_DEFAULT) const;

			void				GetName( VString& outName) const;
			void				GetNameWithoutExtension( VString& outName) const;
			
			// @brief returns extension (no dot)
			void				GetExtension( VString& outExt) const;

			// @brief tells if the folder ends with specified extension (no dot).
			bool				MatchExtension( const VString& inExtension) const;

			const VFilePath&	GetPath() const													{ return fPath; }	// not recommended cause one day the path on mac will be dynamic
			VFilePath&			GetPath( VFilePath& outPath) const								{ outPath = fPath; return outPath;}
			void				GetPath( VString &outPath, FilePathStyle inPathStyle = FPS_SYSTEM, bool relativeToFileSystem = false) const;

			// Tell if the file path is syntaxically correct
			bool				HasValidPath() const	{ return fPath.IsFolder();}

			// check if two VFolder point to the same folder.
			// Comparing the folder paths VString is not same because some file systems are diacritical, some are not.
			bool				IsSameFolder( const VFolder *inFolder) const;
			VError				IsHidden(bool& outIsHidden);

			// Utility function to resolve an alias folder.
			// If the input folder exists as an alias file and is an alias to a folder (and not a file),
			// it returns the resolved alias folder and the input folder is released.
			// If inDeleteInvalidAlias is true and the file appears to be an invalid alias, it is being deleted, else an error is thrown.
	static	VError				ResolveAliasFolder( VFolder **ioFolder, bool inDeleteInvalidAlias);

			VFileSystem*		GetFileSystem() const	{ return fFileSystem;}

#if !VERSION_LINUX
			VError				MakeWritableByAll();
			bool				IsWritableByAll();
#else
            bool                ProcessCanRead();
            bool                ProcessCanWrite();
#endif

#if 0 //VERSIONMAC
			Boolean				MAC_SetCustomIcon( VMacResFile& inResFile, sWORD inId)			{ return fFolder.SetCustomIcon( inResFile, inId); }
			VError				MAC_GetDirectoryID( sLONG *outDirID, sWORD *outVRefNum) const	{ return fFolder.GetDirectoryID( outDirID, outVRefNum); }
#endif

#if VERSIONWIN
			VError				WIN_GetAttributes( DWORD *outAttrb ) const;
			VError				WIN_SetAttributes( DWORD inAttrb ) const;

			bool				WIN_GetIconFile( uLONG inIconSize, HICON &outIconRef ) const;
#endif

#if VERSIONMAC
			bool				MAC_GetIconFile( IconRef &outIconRef ) const;
			
			bool				MAC_GetFSRef( FSRef *outRef) const;
			
			VError				MAC_SetPermissions( uWORD inMode );
			VError				MAC_GetPermissions( uWORD *outMode );
#endif

			VError				GetURL(VString& outURL, bool inEncoded);
			VError				GetRelativeURL(VFolder* inBaseFolder, VString& outURL, bool inEncoded);


			// returns a system folder based on a selector.
			// never returns a non-existing folder (returns NULL instead).
			// pass inCreateFolder to try to create the folder recursively if it doesn't exist.
	static	VFolder*			RetainSystemFolder( ESystemFolderKind inFolderKind, bool inCreateFolder);

private:
			VFolder& operator=( const VFolder& inSource );	// no copy because const object
			
			XFolderImpl			fFolder;
			VFilePath			fPath;
			VFileSystem*		fFileSystem;
};


template <class T, class IMPL>
class VFSObjectIterator : public VObject
{ 
public:
										VFSObjectIterator( const VFolder *inFolder, FileIteratorOptions inOptions ):fFolder( inFolder), fFlags( inOptions)
										{
											fFolder->Retain();
											fCurrent = NULL;
											_Init();
										}

	virtual								~VFSObjectIterator()
										{
											delete fImpl;
											XBOX::ReleaseRefCountable( &fFolder);
											XBOX::ReleaseRefCountable( &fCurrent);
										}

			const VFSObjectIterator&	operator++()									{_Next(); return *this;}
			T*							operator->()									{ return fCurrent;}
			T&							operator*()										{ return *fCurrent;}

										operator T*()									{ return fCurrent;}
										operator T&()									{ return *fCurrent;}

			T*							Current() const									{ return fCurrent;}
			bool						IsValid() const									{ return fCurrent != NULL;}

			T*							First()
										{
												delete fImpl;
												XBOX::ReleaseRefCountable( &fCurrent);
												_Init();
												return fCurrent;
										}

			sLONG						GetPos() const									{ return fPosition;}

			const VFolder&				GetFolder() const								{ return *fFolder; }
			FileIteratorOptions			GetFlags() const								{ return fFlags; }

private:
			void						_Init()
										{
											fPosition = 0;
											fImpl = new IMPL( fFolder, fFlags);
											_Next();
										}

			void						_Next()
										{
											if (fImpl)
											{
												if (fCurrent)
													fCurrent->Release();
												fCurrent = fImpl->Next();
												if (fCurrent != NULL)
												{
													++fPosition;
												}
												else
												{
													delete fImpl;
													fImpl = NULL;
												}
											}
										}
										VFSObjectIterator( const VFSObjectIterator& inOther);	// no copy
			VFSObjectIterator&			operator=( const VFSObjectIterator& inOther);	// no copy

			const VFolder*				fFolder;	// folder in which we iterates
			T*							fCurrent;	// last iterated file or folder
			sLONG						fPosition;
			FileIteratorOptions			fFlags;
			IMPL*						fImpl;
};


class VFolderIterator : public VFSObjectIterator<VFolder, XFolderIteratorImpl>
{
public:
	VFolderIterator( const VFolder *inFolder, FileIteratorOptions inOptions = FI_NORMAL_FOLDERS):VFSObjectIterator<VFolder, XFolderIteratorImpl>( inFolder, inOptions )	{;}
};


typedef std::vector<XBOX::VRefPtr<XBOX::VFolder> >	VectorOfVFolder;


END_TOOLBOX_NAMESPACE

#endif

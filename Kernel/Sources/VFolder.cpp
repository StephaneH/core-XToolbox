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
#include "VFolder.h"
#include "VFile.h"
#include "VURL.h"
#include "VValueBag.h"
#include "VArrayValue.h"
#include "VFileSystem.h"


VFolder::VFolder( const VFilePath& inPath, VFileSystem *inFileSystem)
: fFileSystem( RetainRefCountable( inFileSystem))
, fPath( inPath)
{
	xbox_assert( fPath.IsFolder());
	xbox_assert( (fFileSystem == NULL) || fPath.IsSameOrChildOf( fFileSystem->GetRoot()) );
	fFolder.Init( this);
}


VFolder::VFolder( const VFolder& inSource)
: fFileSystem( RetainRefCountable( inSource.GetFileSystem()))
, fPath( inSource.GetPath())
{
	xbox_assert( fPath.IsFolder());
	fFolder.Init( this);
}


VFolder::VFolder( const VString& inFullPath, FilePathStyle inPathStyle)
: fFileSystem( NULL)
{
	fPath.FromFullPath( inFullPath, inPathStyle);
//	xbox_assert( fPath.IsFolder());
	fFolder.Init( this);
}


VFolder::VFolder( const VFolder& inParentFolder, const VString& inRelativePath, FilePathStyle inRelativePathStyle)
: fFileSystem( RetainRefCountable( inParentFolder.GetFileSystem()))
{
	xbox_assert( inRelativePathStyle == FPS_SYSTEM || inRelativePathStyle == FPS_POSIX);
	UniChar separator = (inRelativePathStyle == FPS_SYSTEM) ? FOLDER_SEPARATOR : '/';
	
	if (!inRelativePath.IsEmpty() && (inRelativePath[inRelativePath.GetLength()-1] != separator) )
	{
		VString path( inRelativePath);
		path += separator;
		fPath.FromRelativePath( inParentFolder.GetPath(), path, inRelativePathStyle);
	}
	else
	{
		fPath.FromRelativePath( inParentFolder.GetPath(), inRelativePath, inRelativePathStyle);
	}
	
	xbox_assert( fPath.IsFolder());
	
	// check if the relative path got us out of our file system
	if ( (fFileSystem != NULL) && !fPath.IsSameOrChildOf( fFileSystem->GetRoot()) )
		ReleaseRefCountable( &fFileSystem);
	
	fFolder.Init( this);
}


#if VERSIONMAC
VFolder::VFolder( const FSRef& inRef)
: fFileSystem( NULL)
{
	VString fullPath;
	XMacFile::FSRefToHFSPath( inRef, fullPath);
	fullPath.AppendUniChar( FOLDER_SEPARATOR);
	fPath.FromFullPath(fullPath);
	xbox_assert( fPath.IsFolder());
	fFolder.Init( this);
}
#endif


VFolder::~VFolder()
{
	ReleaseRefCountable( &fFileSystem);
}


VError VFolder::Create() const
{
	VError	err = VE_OK;
	
	// this is not an error if the folder already exists
	if (!fFolder.Exists( true))
	{
		err = fFolder.Create();
		xbox_assert( (err != VE_OK) || fFolder.Exists( true));

		if (IS_NATIVE_VERROR( err))
		{
			StThrowFileError errThrow( this, VE_FOLDER_CANNOT_CREATE, err);
			err = errThrow.GetError();
		}
	}
	return err;
}

VError VFolder::CreateRecursive(bool inMakeWritableByAll)
{
	VError err = VE_OK;
	if ( !fFolder.Exists( true) )
	{
		VFolder *parent = RetainParentFolder();
		if ( parent )
		{
			err = parent->CreateRecursive(inMakeWritableByAll);
			parent->Release();
			if ( err == VE_OK )
			{
				err = Create();
#if !VERSION_LINUX
				if ((err == VE_OK) && inMakeWritableByAll)
				{
					err = MakeWritableByAll();
				}
#endif
			}
		}
		else
		{
			err = Create();
#if !VERSION_LINUX
			if ((err == VE_OK) && inMakeWritableByAll)
			{
				err = MakeWritableByAll();
			}
#endif
		}
	}
	return err;
}

VError VFolder::DeleteContents( bool inRemoveRecursive ) const
{
	VError err = VE_OK;
	if ( inRemoveRecursive )
	{
		for( VFolderIterator folder( this, FI_WANT_FOLDERS | FI_WANT_INVISIBLES | FI_ITERATE_DELETE ) ; folder.IsValid() && err == VE_OK; ++folder )
		{
			err = folder->DeleteContents( true );
			VTask::GetCurrent()->Yield();
			if ( err == VE_OK )
				err = folder->Delete(false);
		}
	}
	for( VFileIterator file( this, FI_WANT_FILES | FI_WANT_INVISIBLES | FI_ITERATE_DELETE ) ; file.IsValid() && err == VE_OK; ++file )
	{
		err = file->Delete();
	}
	return err;
}

VError VFolder::Delete( bool inRemoveContent ) const
{
	VError err;
	bool needThrow;
	if (IsEmpty())
	{
		err = fFolder.Delete();
		needThrow = IS_NATIVE_VERROR( err);
	}
	else
	{
		if ( inRemoveContent )
		{
			err = DeleteContents( true );
			if ( err == VE_OK )
				err = fFolder.Delete();
			needThrow = IS_NATIVE_VERROR( err);
		}
		else
		{
			err = VE_FOLDER_NOT_EMPTY;
			needThrow = true;
		}
	}

	xbox_assert( (err != VE_OK) || !fFolder.Exists( true));

	if (needThrow)
	{
		StThrowFileError errThrow( this, VE_FOLDER_CANNOT_DELETE, err);
		err = errThrow.GetError();
	}

	return err;
}


VError VFolder::MoveToTrash() const
{
	VError	err = VE_OK;
	
	// this is not an error if the folder already exists
	if (fFolder.Exists( true))
	{
		err = fFolder.MoveToTrash();
		xbox_assert( (err != VE_OK) || !fFolder.Exists( true));
        
		if (IS_NATIVE_VERROR( err))
		{
			StThrowFileError errThrow( this, VE_CANNOT_MOVE_TO_TRASH, err);
			err = errThrow.GetError();
		}
	}
	return err;
}


bool VFolder::IsEmpty() const
{
	if (!fFolder.Exists( true))
		return true;	// if there's no folder, there's nothing inside

	VFolderIterator folder( this, FI_WANT_ALL);
	VFileIterator file( this, FI_WANT_ALL);

	return ((file.Current() == NULL) && (folder.Current() == NULL));
}


bool VFolder::Exists() const
{
	return fPath.IsFolder() && fFolder.Exists( true);
}


VArrayString *VFolder::GetContents(FolderFilter inFolderFilter) const
{
	if (!Exists())
		return NULL;
	
	FileIteratorOptions flags = FI_WANT_NONE;
	switch( inFolderFilter) {
		case FF_NO_FILTER:		flags = FI_WANT_FILES | FI_WANT_FOLDERS; break;
		case FF_NO_FOLDERS:		flags = FI_WANT_FILES; break;
		case FF_NO_FILES:		flags = FI_WANT_FOLDERS; break;
		default:
			xbox_assert( false);
	}

	VArrayString *names = new VArrayString;
	VString name;

	if( flags & FI_WANT_FILES )
		for( VFileIterator i( this, FI_WANT_FILES) ; i.IsValid() ; ++i)
		{
			i->GetName( name);
			names->AppendString( name);
		}

	if( flags & FI_WANT_FOLDERS )
		for( VFolderIterator i( this, FI_WANT_FOLDERS) ; i.IsValid() ; ++i)
		{
			i->GetName( name);
			names->AppendString( name);
		}

	return names;
}

void VFolder::GetContents(std::vector< VRefPtr < VFile > >& outFiles, std::vector< VRefPtr < VFolder > >& outFolders) const
{
	if (!Exists())
		return;
	
	for( VFileIterator i( this, FI_WANT_FILES) ; i.IsValid() ; ++i)
		outFiles.push_back( VRefPtr<VFile>( i ) );

	for( VFolderIterator i( this, FI_WANT_FOLDERS) ; i.IsValid() ; ++i)
		outFolders.push_back( VRefPtr<VFolder>( i ) );
}


bool VFolder::Contains( const XBOX::VString& inExtension) const
{
	bool found = false;

	VArrayString* files = GetContents(FF_NO_FOLDERS);
	if (files)
	{
		VString name;
		for (sLONG i = 1; i <= files->GetCount(); ++i)
		{
			files->GetString(name, i);
			if (name.EndsWith(inExtension))
			{
				found = true;
				break;
			}

		}
		delete files;
	}
	
	return found;
}

VError VFolder::SetTimeAttributes( const VTime *inLastModification, const VTime *inCreationTime, const VTime *inLastAccess ) const
{
	VError err = fFolder.SetTimeAttributes( inLastModification, inCreationTime, inLastAccess );

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_GET_ATTRIBUTES, err);
		err = errThrow.GetError();
	}

	return err;
}
VError VFolder::GetTimeAttributes( VTime* outLastModification, VTime* outCreationTime, VTime* outLastAccess ) const
{
	VError err = fFolder.GetTimeAttributes( outLastModification, outCreationTime, outLastAccess );

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FOLDER_CANNOT_GET_ATTRIBUTES, err);
		err = errThrow.GetError();
	}
	
	return err;
}

VError	VFolder::GetVolumeCapacity(sLONG8* outTotalBytes)const
{
#if !VERSION_LINUX

	VError err = fFolder.GetVolumeCapacity( outTotalBytes );

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FOLDER_CANNOT_GET_ATTRIBUTES, err);
		err = errThrow.GetError();
	}
	
	return err;

#else
    
    //jmo - Build fix rapide pour un truc qui n'est utilise que dans 4D.
    if(outTotalBytes!=NULL)
        *outTotalBytes=0;

    xbox_assert(false);
    return VE_UNIMPLEMENTED;

#endif
}

VError VFolder::GetVolumeFreeSpace(sLONG8 *outFreeBytes, bool inWithQuotas ) const
{
	VError err = fFolder.GetVolumeFreeSpace( outFreeBytes, inWithQuotas );

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FOLDER_CANNOT_GET_ATTRIBUTES, err);
		err = errThrow.GetError();
	}
	
	return err;
}

#if !VERSION_LINUX
bool VFolder::RevealInFinder( bool inInside) const
{
	bool done = false;
	if (inInside)
	{
		VFileIterator iter( this);
		VFile *firstFile = iter.First();
		if (firstFile != NULL)
			done = firstFile->RevealInFinder();
	}
	if (!done)
	{
		VError err = fFolder.RevealInFinder();

		if (IS_NATIVE_VERROR( err))
		{
			StThrowFileError errThrow( this, VE_FOLDER_CANNOT_REVEAL, err);
			err = errThrow.GetError();
		}
		done = err == VE_OK;
	}

	return done;
}
#endif


VError VFolder::Rename( const VString& inName, VFolder** outFolder ) const
{
	VError err = fFolder.Rename( inName, outFolder );

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FOLDER_CANNOT_RENAME, err);
		errThrow->SetString( "newname", inName);
		err = errThrow.GetError();
	}

	return err;
}

VError VFolder::Move( const VFolder& inNewParent, VFolder** outFolder ) const
{
	VError err = fFolder.Move( inNewParent, NULL, outFolder );

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FOLDER_CANNOT_RENAME, err);
		//errThrow->SetString( "newname", inName);
		err = errThrow.GetError();
	}

	return err;
}


VError VFolder::Move( const VFolder& inNewParent, const VString& inNewName, VFolder** outFolder ) const
{
	VError err = fFolder.Move( inNewParent, &inNewName, outFolder );

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FOLDER_CANNOT_RENAME, err);
		//errThrow->SetString( "newname", inName);
		err = errThrow.GetError();
	}

	return err;
}


VError VFolder::CopyTo( const VFolder& inDestinationFolder, VFolder **outFolder, FileCopyOptions inOptions) const
{
	VString name;
	GetName( name);
	
	return CopyTo( inDestinationFolder, name, outFolder, inOptions);
}


VError VFolder::CopyTo( const VFolder& inDestinationFolder, const VString& inNewName, VFolder **outFolder, FileCopyOptions inOptions) const
{
	VError err = VE_OK;
	VFolder *folder = NULL;

	if (IsSameFolder( &inDestinationFolder) || inDestinationFolder.GetPath().IsChildOf( GetPath()))
	{
		StThrowFileError errThrow( this, VE_CANNOT_COPY_ON_ITSELF);
		errThrow->SetString( "destination", inDestinationFolder.GetPath().GetPath());
		err = errThrow.GetError();
	}
	else
	{
		folder = new VFolder( inDestinationFolder, inNewName);
		if (folder != NULL)
		{
			if ( ( (inOptions & FCP_Overwrite) != 0) && folder->Exists() )
				err = folder->Delete( true);
			if (err == VE_OK)
				err = CopyContentsTo( *folder, inOptions);
		}
		else
		{
			err = VE_MEMORY_FULL;
		}
	}

	if (outFolder != NULL)
	{
		*outFolder = folder;
		folder = NULL;
	}

	ReleaseRefCountable( &folder);
		
	return err;
}


VError VFolder::CopyContentsTo( const VFolder& inDestinationFolder, FileCopyOptions inOptions) const
{
	VError err = VE_OK;
	if (!inDestinationFolder.Exists())
	{
		// this easy case should be handled by system implementation
		err = inDestinationFolder.Create();
	}
	else if (IsSameFolder( &inDestinationFolder) || inDestinationFolder.GetPath().IsChildOf( GetPath()))
	{
		StThrowFileError errThrow( this, VE_CANNOT_COPY_ON_ITSELF);
		errThrow->SetString( "destination", inDestinationFolder.GetPath().GetPath());
		err = errThrow.GetError();
	}

	if (err == VE_OK)
	{
		bool ok = true;
		
		FileIteratorOptions flags = FI_WANT_FOLDERS;
		if ((inOptions & FCP_SkipInvisibles) == 0)
			flags |= FI_WANT_INVISIBLES;
		
		for( VFolderIterator folderIterator( this, flags) ; folderIterator.IsValid() && ok ; ++folderIterator)
		{
			VError err2 = folderIterator->CopyTo( inDestinationFolder, NULL, inOptions);
			if (err == VE_OK)
				err = err2;
			ok = (err == VE_OK) | ((inOptions & FCP_ContinueOnError) != 0);
		}
		
		flags = FI_WANT_FILES;
		if ((inOptions & FCP_SkipInvisibles) == 0)
			flags |= FI_WANT_INVISIBLES;

		for( VFileIterator fileIterator( this, flags) ; fileIterator.IsValid() && ok ; ++fileIterator)
		{
			VError err2 = fileIterator->CopyTo( inDestinationFolder, NULL, inOptions);
			if (err == VE_OK)
				err = err2;
			ok = (err == VE_OK) | ((inOptions & FCP_ContinueOnError) != 0);
		}
	}
	
	return err;
}


VFolder *VFolder::RetainParentFolder( bool inSandBoxed) const
{
	VFolder *folder;
	VFilePath fullPath;
	if (fPath.GetParent( fullPath ))
	{
		if ( (fFileSystem == NULL) || !inSandBoxed || fFileSystem->IsAuthorized( fullPath) )
			folder = new VFolder( fullPath, fFileSystem);
		else
			folder = NULL;
	}
	else
		folder = NULL;

	return folder;
}


VFolder *VFolder::RetainRelativeFolder( const VString& inRelativePath, FilePathStyle inRelativePathStyle) const
{
	xbox_assert( inRelativePathStyle == FPS_SYSTEM || inRelativePathStyle == FPS_POSIX);
	UniChar separator = (inRelativePathStyle == FPS_SYSTEM) ? FOLDER_SEPARATOR : '/';
	
	VFilePath path;
	
	if (!inRelativePath.IsEmpty() && (inRelativePath[inRelativePath.GetLength()-1] != separator) )
	{
		VString pathString( inRelativePath);
		pathString += separator;
		path.FromRelativePath( GetPath(), pathString, inRelativePathStyle);
	}
	else
	{
		path.FromRelativePath( GetPath(), inRelativePath, inRelativePathStyle);
	}
	
	if (!testAssert( path.IsFolder()))
		return NULL;
	
	if ( (fFileSystem != NULL) && !fFileSystem->IsAuthorized( path) )
		return NULL;
	
	return new VFolder( path, fFileSystem);
}


VFile *VFolder::RetainRelativeFile( const VString& inRelativePath, FilePathStyle inRelativePathStyle) const
{
	xbox_assert( inRelativePathStyle == FPS_SYSTEM || inRelativePathStyle == FPS_POSIX);

	VFilePath path( GetPath(), inRelativePath, inRelativePathStyle);
	
	if (!testAssert( path.IsFile()))
		return NULL;
	
	if ( (fFileSystem != NULL) && !fFileSystem->IsAuthorized( path) )
		return NULL;
	
	return new VFile( path, fFileSystem);
}


void VFolder::GetName( VString& outName) const
{
	fPath.GetName( outName);
}


void VFolder::GetNameWithoutExtension( VString& outName) const
{
	fPath.GetFolderName( outName, false);
}


void VFolder::GetExtension( VString& outExt) const
{
	fPath.GetExtension(outExt);
}


bool VFolder::MatchExtension( const VString& inExtension) const
{
	// WARNING: diacritical or not? should ask the file system
	return fPath.MatchExtension( inExtension, false);
}


void VFolder::GetPath( VString &outPath, FilePathStyle inPathStyle, bool relativeToFileSystem) const
{
	switch (inPathStyle)
	{
		case FPS_SYSTEM :
			fPath.GetPath( outPath );
			break;

		case FPS_POSIX :
			{
				if (relativeToFileSystem && fFileSystem != nil)
				{
					VFilePath fileSystemPath = fFileSystem->GetRoot();
					fPath.GetRelativePosixPath(fileSystemPath, outPath);
					VString fileSystemName = fFileSystem->GetName();
					outPath = "/"+fileSystemName+"/"+outPath;
				}
				else
				{
					VError err = fFolder.GetPosixPath( outPath);
					if (err != VE_OK)
						fPath.GetPosixPath( outPath);	// boff
				}
				break;
			}
			break;

		default:
			xbox_assert( false);
			outPath.Clear();
			break;
	}
}


bool VFolder::IsSameFolder( const VFolder *inFolder) const
{
	// TBD: should check the filesystem if case sensitivity is to be taken into account.
	if (inFolder != NULL)
		return inFolder->GetPath().GetPath().EqualTo( GetPath().GetPath(), false) != 0;
	else
		return false;
}


VFolder *VFolder::RetainSystemFolder( ESystemFolderKind inFolderKind, bool inCreateFolder)
{
	VFolder *folder = NULL;
	VError err = XFolderImpl::RetainSystemFolder( inFolderKind, inCreateFolder, &folder );
	xbox_assert( err != VE_OK || folder != NULL);
	return folder;
}


VError VFolder::IsHidden(bool &outIsHidden)
{
	return fFolder.IsHidden( outIsHidden );
}


#if !VERSION_LINUX
VError VFolder::MakeWritableByAll()
{
	VError err = fFolder.MakeWritableByAll();
	
	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FOLDER_CANNOT_SET_ATTRIBUTES, err);
		err = errThrow.GetError();
	}
	
	return err;
}


bool VFolder::IsWritableByAll()
{
	return fFolder.IsWritableByAll();
}

#else

bool VFolder::ProcessCanRead()
{
    return fFolder.ProcessCanRead();
}


bool VFolder::ProcessCanWrite()
{
    return fFolder.ProcessCanWrite();
}

#endif

#pragma mark -
#pragma mark Windows

#if VERSIONWIN

VError VFolder::WIN_GetAttributes( DWORD *outAttrb ) const
{
	VError err = fFolder.GetAttributes( outAttrb );

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FOLDER_CANNOT_GET_ATTRIBUTES, err);
		err = errThrow.GetError();
	}
	
	return err;
}


VError VFolder::WIN_SetAttributes( DWORD inAttrb) const
{
	VError err = fFolder.SetAttributes( inAttrb);

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FOLDER_CANNOT_SET_ATTRIBUTES, err);
		err = errThrow.GetError();
	}
	
	return err;
}

bool VFolder::WIN_GetIconFile( uLONG inIconSize, HICON &outIconRef ) const				
{ 
	return fFolder.GetIconFile( inIconSize, outIconRef ); 
}
#endif

#pragma mark -
#pragma mark Mac

#if VERSIONMAC
bool VFolder::MAC_GetIconFile( IconRef &outIconRef ) const				
{ 
	return fFolder.GetIconFile( outIconRef ); 
}
 
bool VFolder::MAC_GetFSRef (FSRef *outRef) const
{
	VString fullPath;
	fPath.GetPath(fullPath);
	return XMacFile::HFSPathToFSRef(fullPath,outRef) == noErr;
}

VError VFolder::MAC_SetPermissions( uWORD inMode )
{
	VError err = fFolder.SetPermissions(inMode);
	
	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FOLDER_CANNOT_SET_ATTRIBUTES, err);
		err = errThrow.GetError();
	}
	
	return err;
}

VError VFolder::MAC_GetPermissions( uWORD *outMode )
{
	VError err = fFolder.GetPermissions(outMode);
	
	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FOLDER_CANNOT_GET_ATTRIBUTES, err);
		err = errThrow.GetError();
	}
	
	return err;
}

#endif

VError VFolder::GetURL(VString& outURL, bool inEncoded)
{
	VFilePath path;
	GetPath(path);
	VURL url(path);
	url.GetAbsoluteURL(outURL, inEncoded);
	return VE_OK;
}


VError VFolder::GetRelativeURL(VFolder* inBaseFolder, VString& outURL, bool inEncoded)
{
	VString folderURL;
	VError err = GetURL(outURL, inEncoded);
	if (inBaseFolder != NULL)
	{
		inBaseFolder->GetURL(folderURL, inEncoded);
		if (outURL.BeginsWith(folderURL))
		{
			outURL.Remove(1, folderURL.GetLength());
		}

	}
	return err;
}


/*
	static
*/
VError VFolder::ResolveAliasFolder( VFolder **ioFolder, bool inDeleteInvalidAlias)
{
	VError err = VE_OK;
	if ( (*ioFolder != NULL) && (*ioFolder)->fPath.IsFolder() && !(*ioFolder)->fFolder.Exists( false /* doesn't check alias */) )
	{
		// the folder doesn't exists
		// maybe is it an alias file?
		VString path = (*ioFolder)->fPath.GetPath();
		if (testAssert( path[path.GetLength()-1] == FOLDER_SEPARATOR))
		{
			path.Truncate( path.GetLength()-1);
			#if VERSIONWIN
			path += ".lnk";
			#endif
			VFile file( path);
			if (file.IsAliasFile() && file.Exists())
			{
				VFilePath resolvedPath;
				err = file.GetImpl()->ResolveAlias( resolvedPath);	// nothrow
				if (err == VE_OK)
				{
					if (resolvedPath.IsFolder())
					{
						(*ioFolder)->Release();
						*ioFolder = new VFolder( resolvedPath);
					}
					else if (inDeleteInvalidAlias)
					{
						err = file.Delete();
					}
					else
					{
						StThrowFileError errThrow( *ioFolder, VE_FILE_CANNOT_RESOLVE_ALIAS_TO_FOLDER);
						err = errThrow.GetError();
					}
				}
				else if (inDeleteInvalidAlias)
				{
					err = file.Delete();
				}
				else
				{
					if (IS_NATIVE_VERROR( err))
					{
						StThrowFileError errThrow( &file, VE_FILE_CANNOT_RESOLVE_ALIAS, err);
						err = errThrow.GetError();
					}
				}
			}
		}
	}
	return err;
}


/*
	static
*/
VFolder *VFolder::CreateAndResolveAlias( const VFolder& inParentFolder, const VString& inFolderName)
{
	VFolder *folder = new VFolder( inParentFolder, inFolderName);
	VError error = ResolveAliasFolder( &folder, true);
	if (error != VE_OK)
		ReleaseRefCountable( &folder);
	
	return folder;
}


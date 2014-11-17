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
#include "VFileSystem.h"
#include "VFilePath.h"
#include "VFile.h"
#include "VFolder.h"
#include "VErrorContext.h"
#include "VJSONTools.h"



VFileSystem::VFileSystem( const VString& inName, const VFilePath& inRoot, EFSOptions inOptions, VSize inQuota, VSize inUsedSpace)
: fName( inName)
, fRoot( inRoot)
, fQuota( inQuota)
, fOptions( inOptions)
{	
	xbox_assert( inQuota >= inUsedSpace);
	xbox_assert( inRoot.IsFolder());
	fAvailable = inQuota - inUsedSpace;
}

VFileSystem::~VFileSystem()
{
	if (fOptions & eFSO_Volatile)
	{
		StErrorContextInstaller errorContext( false /* inKeepingErrors */, false /* inSilentContext */);
		VFolder	folder( fRoot);

		if (folder.Exists())
			VError error = folder.Delete( true /* inRemoveContent */);
	}
}

/*
	static
*/
VFileSystem *VFileSystem::Create( const VString &inName, const VFilePath &inRoot, EFSOptions inOptions, VSize inQuota, VSize inUsedSpace, VError *outError)
{
	VError error = VE_OK;
	VFileSystem *fs = NULL;

	if (inName.IsEmpty() || (inName.FindUniChar( '/') > 0) )
	{
		StThrowError<> err( VE_FS_INVALID_NAME);
		err->SetString( CVSTR( "name"), inName);
		error = err.GetError();
	}
	else
	{
		fs = new VFileSystem( inName, inRoot, inOptions, inQuota, inUsedSpace);
	}
		
	if (outError != NULL)
		*outError = error;
	
	return fs;
}


VError VFileSystem::Request( VSize inSize)
{
	if (!(fOptions & eFSO_Writable))

		return VE_FS_NOT_WRITABLE;

	else if (!fQuota) 

		return VE_OK;	// No limit

	else if (inSize <= fAvailable)
	{
		fAvailable -= inSize;
		return VE_OK;

	} else 

		return VE_FS_OUT_OF_SPACE;
}

VError VFileSystem::Relinquish( VSize inSize)
{
	if (!(fOptions & eFSO_Writable))
		return VE_FS_NOT_WRITABLE;

	if (fQuota)
	{
		fAvailable += inSize;
		xbox_assert(fAvailable <= fQuota);
	} 

	return VE_OK;
}

bool VFileSystem::IsAuthorized( const VFilePath& inPath) const
{
	return inPath.IsSameOrChildOf( fRoot);
}


VError VFileSystem::ParsePosixPath( const VString& inPosixPath, const VFilePath& inCurrentFolder, bool inThrowErrorIfNotFound, VFilePath *outFullPath)
{
	xbox_assert(inCurrentFolder.IsValid() && inCurrentFolder.IsFolder() && IsAuthorized(inCurrentFolder) );
	xbox_assert(outFullPath != NULL);

	// Make a full path from an "absolute" path from root of file system or from a "relative" path from
	// current folder (which must be below the root of file system).

	VError	error = VE_OK;
	
	if (inPosixPath.IsEmpty())
	{
		error = vThrowError( VE_FS_PATH_ENCODING_ERROR);
	}
	else
	{
		if (inPosixPath.GetUniChar(1) == '/')
		{
			VIndex	length;
			if ((length = inPosixPath.GetLength()) == 1)
				outFullPath->FromFilePath(fRoot);
			else
			{
				VString	posixPath;
				inPosixPath.GetSubString(2, length - 1, posixPath);
				outFullPath->FromRelativePath(fRoot, posixPath, FPS_POSIX);
			}
		}
		else
		{
			outFullPath->FromRelativePath(inCurrentFolder, inPosixPath, FPS_POSIX);
		}

		if (!outFullPath->IsValid())
		{
			error = vThrowError( VE_FS_PATH_ENCODING_ERROR);
		}
		else
		{

			// Check that path is authorized. For instance, with a relative path, it can try to 
			// back up its parent "above" the root of file system.
			if (!IsAuthorized(*outFullPath))
			{
				error = vThrowError( VE_FS_NOT_AUTHORIZED);
			}
			else
			{
				// Check for existence of the file or folder.

				if (outFullPath->IsFolder())
				{
					VFolder	folder(*outFullPath);
					if (!folder.Exists())
						error = inThrowErrorIfNotFound ? vThrowError( VE_FS_NOT_FOUND) : VE_FS_NOT_FOUND;
				}
				else
				{
					VFile	file(*outFullPath);
					if (!file.Exists())
					{
						// Allow user to specify a folder without a terminating '/' separator.

						VString	fullPath( outFullPath->GetPath());
						fullPath.AppendUniChar(FOLDER_SEPARATOR);

						VFolder	folder(fullPath);
						if (folder.Exists())
						{
							outFullPath->FromFullPath(fullPath);
						}
						else
						{
							error = inThrowErrorIfNotFound ? vThrowError( VE_FS_NOT_FOUND) : VE_FS_NOT_FOUND;
						}
					}
				}
			}
		}
	}
	return error;
}

VError VFileSystem::ParseURL (const VURL &inUrl, const VFilePath &inCurrentFolder, bool inThrowErrorIfNotFound, VFilePath *outFullPath)
{
	VString	posixPath;

	inUrl.GetRelativePath(posixPath);

	return ParsePosixPath(posixPath, inCurrentFolder, inThrowErrorIfNotFound, outFullPath);
}


//=================================================================


VFileSystemNamespace::VFileSystemNamespace( VFileSystemNamespace *inParent)
: fParent( RetainRefCountable( inParent))
{
}


VFileSystemNamespace::~VFileSystemNamespace()
{
	ReleaseRefCountable( &fParent);
}


void VFileSystemNamespace::RegisterFileSystem( VFileSystem *inFileSystem)
{
	if (inFileSystem != NULL)
	{
		VTaskLock lock( &fMutex);
		
		fMap[inFileSystem->GetName()] = inFileSystem;
	}
}


VError VFileSystemNamespace::RegisterFileSystem( const VString& inName, const VFilePath& inPath, EFSOptions inOptions)
{
	VError err;
	VFileSystem *fs = VFileSystem::Create( inName, inPath, inOptions, 0, 0, &err);
	RegisterFileSystem( fs);
	ReleaseRefCountable( &fs);
	return err;
}


void VFileSystemNamespace::UnregisterFileSystem( const VString& inName)
{
	VTaskLock lock( &fMutex);
	
	fMap.erase( inName);
}


VFileSystem *VFileSystemNamespace::RetainFileSystem( const VString& inName, EFSNOptions inOptions)
{
	VFileSystem *fs = NULL;

	{
		VTaskLock lock( &fMutex);
		
		MapOfFileSystem::const_iterator i = fMap.find( inName);
		if (i != fMap.end())
			fs = i->second.Retain();
	}
	
	if (fs == NULL)
	{
		if ( ((inOptions & eFSN_SearchParent) != 0) && (fParent != NULL) )
			fs = fParent->RetainFileSystem( inName, inOptions);
	}
	else
	{
		if ( ((inOptions & eFSN_PublicOnly) != 0) && fs->IsPrivate() )
			ReleaseRefCountable( &fs);
	}
	
	return fs;
}


VFolder *VFileSystemNamespace::RetainFileSystemRootFolder( const VString& inName, EFSNOptions inOptions)
{
	VFileSystem *fs = RetainFileSystem( inName, inOptions);
	VFolder *folder = (fs != NULL) ? new VFolder( fs->GetRoot(), fs) : NULL;
	ReleaseRefCountable( &fs);
	return folder; 
}


VError VFileSystemNamespace::LoadFromDefinitionFile( VFile *inFile)
{
	if (!testAssert( inFile != NULL))
		return vThrowError( VE_UNKNOWN_ERROR);

	VJSONValue	value;
	VError err = VJSONImporter::ParseFile( inFile, value, VJSONImporter::EJSI_Strict);

	if ( (err == VE_OK) && testAssert( value.IsArray()) )
	{
		VTaskLock lock( &fMutex);
		VJSONArray *array = value.GetArray();
		
		size_t count = array->GetCount();
		for( size_t i = 0 ; (i < count) && (err == VE_OK) ; ++i)
		{
			VJSONValue fileSystemValue = (*array)[i];
			if (testAssert( fileSystemValue.IsObject()))
			{
				EFSOptions options = eFSO_Overridable;
				
				VJSONObject *fileSystemObject = fileSystemValue.GetObject();
				
				VString name;
				fileSystemObject->GetPropertyAsString( CVSTR( "name"), name);

				VString pathString;
				fileSystemObject->GetPropertyAsString( CVSTR( "path"), pathString);

				VString parentFileSystemName;
				fileSystemObject->GetPropertyAsString( CVSTR( "parent"), parentFileSystemName);

				bool writable = false;
				fileSystemObject->GetPropertyAsBool( CVSTR( "writable"), &writable);
				if (writable)
					options |= eFSO_Writable;
				
				// be nice and add the terminating '/' if the user forgot
				if (!pathString.IsEmpty() && pathString[pathString.GetLength()-1] != '/' && pathString[pathString.GetLength()-1] != '\\')
					pathString += '/';

				// resolve path against parent file system if any
				VFilePath path;
				if (parentFileSystemName.IsEmpty())
				{
					#if VERSIONWIN
					// be nice and accepts both posix and ms-dos notations for full paths
					pathString.ExchangeAll( (UniChar) '/', (UniChar) '\\');
					path.FromFullPath( pathString, FPS_SYSTEM);
					#else
					path.FromFullPath( pathString, FPS_POSIX);
					#endif
				}
				else
				{
					VFileSystem *parentFileSystem = RetainFileSystem( parentFileSystemName, eFSN_SearchParent);	// accepts private fileSystems
					if (testAssert( parentFileSystem != NULL) )
					{
						path.FromRelativePath( parentFileSystem->GetRoot(), pathString, FPS_POSIX);
					}
					ReleaseRefCountable( &parentFileSystem);
				}

				// check if one fileSystem with same name already exists
				// test and set is protected by our mutex above.
				VFileSystem *fileSystem = RetainFileSystem( name);
				bool ok = (fileSystem == NULL) || fileSystem->IsOverridable();
				ReleaseRefCountable( &fileSystem);
				
				if (ok)
				{
					err = RegisterFileSystem( name, path, options);
				}
			}
		}
	}
	
	if (err != VE_OK)
	{
		StThrowError<> err2( VE_FS_CANT_LOAD_DEFINITION_FILE);
		err2->SetString( CVSTR( "path"), inFile->GetPath().GetPath());
	}
	
	return err;
}


bool VFileSystemNamespace::ParsePathWithFileSystem( const VString& inPath, VFile **outFile, VFolder **outFolder, EFSNOptions inOptions)
{
	bool ok = false;

	if (outFile != NULL)
		*outFile = NULL;

	if (outFolder != NULL)
		*outFolder = NULL;

	VFileSystem *fs = nil;
	VFilePath path;

	ok = ParsePathWithFileSystem(inPath, fs, path, inOptions);

	if (ok)
	{
		if (path.IsFile())
		{
			if (outFile != NULL)
				*outFile = new VFile( path, fs);
		}
		else
		{
			if (outFolder != NULL)
				*outFolder = new VFolder( path, fs);
		}
	}

	QuickReleaseRefCountable(fs);

	return ok;
}



bool VFileSystemNamespace::ParsePathWithFileSystem( const VString& inPath, VFileSystem* &outFS, VFilePath& outPath, EFSNOptions inOptions)
{
	bool ok = false;
	outFS = nil;

	if (!inPath.IsEmpty() && (inPath[0] == '/') )
	{
		VIndex pos = inPath.FindUniChar( '/', 2);
		if (pos > 0)
		{
			VString fileSystemName;
			inPath.GetSubString( 2, pos-2, fileSystemName);

			VString relativePath;
			inPath.GetSubString( pos+1, inPath.GetLength() - pos, relativePath);

			VFileSystem *fs = RetainFileSystem( fileSystemName, inOptions);
			if (fs != NULL)
			{
				VFilePath path( fs->GetRoot(), relativePath, FPS_POSIX);
				ok = path.IsSameOrChildOf( fs->GetRoot());
				if (ok)
				{
					outFS = RetainRefCountable(fs);
					outPath = path;
				}
			}
			ReleaseRefCountable( &fs);
		}
	}
	return ok;
}


VError VFileSystemNamespace::RetainMapOfFileSystems(OrderedMapOfFileSystem& outMap, EFSNOptions inOptions)
{
	VError err = VE_OK;
	
	if (fParent != NULL && (inOptions & eFSN_SearchParent) != 0)
	{
		err = fParent->RetainMapOfFileSystems(outMap, inOptions);
	}

	if (err == VE_OK)
	{
		VTaskLock lock(&fMutex);
		for (MapOfFileSystem::iterator cur = fMap.begin(), end = fMap.end(); cur != end; ++cur)
		{
			if ((inOptions & eFSN_PublicOnly) != 0 && cur->second->IsPrivate())
			{
				// do not get private ones
			}
			else
				outMap[cur->first] = cur->second;
		}
	}

	return err;
}



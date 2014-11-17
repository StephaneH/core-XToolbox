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
#include "VFile.h"
#include "VFolder.h"
#include "VErrorContext.h"
#include "VTime.h"
#include "VIntlMgr.h"
#include "VMemory.h"
#include "VArrayValue.h"
#include "VFileStream.h"
#include "VFileSystem.h"
#include "VURL.h"
#include "xwinfolder.h"

VError XWinFolder::Create() const
{
	XWinFullPath path( fOwner->GetPath() );

	// NULL= no security attributes
	DWORD winErr = ::CreateDirectoryW( path, NULL) ? 0 : ::GetLastError();
	
	return MAKE_NATIVE_VERROR( winErr);
}


VError XWinFolder::Delete() const
{
	XWinFullPath path( fOwner->GetPath() );

	DWORD winErr = ::RemoveDirectoryW( path) ? 0 : ::GetLastError();

	return MAKE_NATIVE_VERROR( winErr);
}


VError XWinFolder::MoveToTrash() const
{
	// don't use XWinFullPath because SHFileOperation fails on any path prefixed with "\\?\".
	
	// the path parameter is actually a paths list and must be double null terminated.
	// so one must copy the path in a temp buffer.
	UniChar *path = (UniChar*) calloc( fOwner->GetPath().GetPath().GetLength() + 2, sizeof( UniChar));
	if (path == NULL)
		return MAKE_NATIVE_VERROR( ERROR_NOT_ENOUGH_MEMORY);
	
	wcscpy( path, fOwner->GetPath().GetPath().GetCPointer());
	
	SHFILEOPSTRUCTW  info;
	memset( &info, 0, sizeof(info));

	info.fFlags |= FOF_SILENT;				// don't report progress
	info.fFlags |= FOF_NOERRORUI;			// don't report errors
	info.fFlags |= FOF_NOCONFIRMATION;		// don't confirm delete
	info.fFlags |= FOF_ALLOWUNDO;			// move to recycle bin
	info.fFlags |= FOF_NOCONFIRMMKDIR;
	info.fFlags |= FOF_RENAMEONCOLLISION; 
	info.wFunc = FO_DELETE;                   // required: delete operation
	info.pTo = NULL;                          // must be NULL
	info.pFrom = path; 

	VError err;
	int result = SHFileOperationW( &info);
	if (result != 0)
	{
		// unfortunately, GetLastError() is unusable here.
		// and the result code is poorly documented.
		switch( result)
		{
			case 0x7C /*DE_INVALIDFILES*/:	err = MAKE_NATIVE_VERROR( ERROR_PATH_NOT_FOUND); break;
			default:				err = MAKE_NATIVE_VERROR( ERROR_INVALID_PARAMETER); break;
		}
	}
	else
	{
		err = VE_OK;
	}
	
	free( path);
	
	return err;
}


bool XWinFolder::Exists( bool inAcceptFolderAlias) const
{
	bool ok = false;
	XWinFullPath path( fOwner->GetPath() );
	DWORD fileAttributes = ::GetFileAttributesW( path);
	if ( fileAttributes != INVALID_FILE_ATTRIBUTES)
	{
		if (fileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			ok = true;
		}
		else if (inAcceptFolderAlias)
		{
			// en fait sur windows les alias ne sont pas des redirections.
			// on ne peut ecrire c:\test.lnk\truc.txt
/*
			VString thePath;
			fOwner->GetPath( thePath);
			bool isFolder = false;
			HRESULT hr = WIN_ResolveShortcut( thePath, true, false, &isFolder);
			ok = (hr == S_OK ) && isFolder;
*/
		}
	}
	return ok;
}


VError XWinFolder::RevealInFinder() const
{
	VString fullPath;
	fOwner->GetPath(fullPath);

	return XWinFile::RevealPathInFinder( fullPath);
}


VError XWinFolder::Rename( const VString& inName, VFolder** outFile ) const
{
	VFilePath newPath( fOwner->GetPath() );
	newPath.SetName( inName );
	
	DWORD winErr;
	if (newPath.IsValid())
	{
		XWinFullPath oldWinPath( fOwner->GetPath() );
		XWinFullPath newWinPath( newPath );
		
		winErr = ::MoveFileW( oldWinPath, newWinPath ) ? 0 : ::GetLastError();
	}
	else
	{
		winErr = ERROR_INVALID_NAME;
	}

	if (outFile)
	{
		if (winErr == 0)
		{
			*outFile = new VFolder( newPath, fOwner->GetFileSystem());
		}
		else
		{
			*outFile = NULL;
		}
	}
	
	return MAKE_NATIVE_VERROR( winErr);
}

VError XWinFolder::Move( const VFolder& inNewParent, const VString *inNewName, VFolder** outFolder ) const
{
	XBOX::VString name;
	VFilePath newPath( inNewParent.GetPath() );
	
	DWORD winErr;
	if (newPath.IsValid())
	{
		if ( (inNewName == NULL) || inNewName->IsEmpty() )
		{
			VString name;
			fOwner->GetName(name);
			newPath.ToSubFolder(name);
		}
		else
		{
			newPath.ToSubFolder(*inNewName);
		}

		XWinFullPath oldWinPath( fOwner->GetPath() );
		XWinFullPath newWinPath( newPath );
		
		winErr = ::MoveFileW( oldWinPath, newWinPath) ? 0 : ::GetLastError();
	}
	else
	{
		winErr = ERROR_INVALID_NAME;
	}

	if (outFolder)
	{
		if (winErr == 0)
		{
			*outFolder = new VFolder( newPath, inNewParent.GetFileSystem());
		}
		else
		{
			*outFolder = NULL;
		}
	}
	
	return MAKE_NATIVE_VERROR( winErr);
}

VError XWinFolder::SetTimeAttributes( const VTime *inLastModification, const VTime *inCreationTime, const VTime *inLastAccess ) const
{
	WIN32_FILE_ATTRIBUTE_DATA info;
	SYSTEMTIME lastModification;
	SYSTEMTIME creationTime;
	SYSTEMTIME lastAccess;

	XWinFullPath path( fOwner->GetPath());
	DWORD winErr = ::GetFileAttributesExW( path, GetFileExInfoStandard, &info) ? 0 : ::GetLastError();

	if (winErr == 0)
	{
		if ( inLastModification )
		{
			inLastModification->GetUTCTime( (sWORD&)lastModification.wYear, (sWORD&)lastModification.wMonth, (sWORD&)lastModification.wDay, (sWORD&)lastModification.wHour, (sWORD&)lastModification.wMinute, (sWORD&)lastModification.wSecond, (sWORD&)lastModification.wMilliseconds);
			::SystemTimeToFileTime( &lastModification, &info.ftLastWriteTime );
		}
		if ( inCreationTime )
		{
			inCreationTime->GetUTCTime( (sWORD&)creationTime.wYear, (sWORD&)creationTime.wMonth, (sWORD&)creationTime.wDay, (sWORD&)creationTime.wHour, (sWORD&)creationTime.wMinute, (sWORD&)creationTime.wSecond, (sWORD&)creationTime.wMilliseconds);
			::SystemTimeToFileTime( &creationTime, &info.ftCreationTime );
		}
		if ( inLastAccess )
		{
			inLastAccess->GetUTCTime( (sWORD&)lastAccess.wYear, (sWORD&)lastAccess.wMonth, (sWORD&)lastAccess.wDay, (sWORD&)lastAccess.wHour, (sWORD&)lastAccess.wMinute, (sWORD&)lastAccess.wSecond, (sWORD&)lastAccess.wMilliseconds);
			::SystemTimeToFileTime( &lastAccess, &info.ftLastAccessTime );
		}
		HANDLE fileHandle = ::CreateFileW(path, FILE_WRITE_ATTRIBUTES , 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if ( fileHandle != INVALID_HANDLE_VALUE )
		{
			winErr = ::SetFileTime( fileHandle, &info.ftCreationTime, &info.ftLastAccessTime, &info.ftLastWriteTime ) ? 0 : ::GetLastError();
			::CloseHandle( fileHandle);
		}
		else
		{
			winErr = ::GetLastError();
		}
	}

	return MAKE_NATIVE_VERROR( winErr);
}

VError XWinFolder::GetTimeAttributes( VTime* outLastModification, VTime* outCreationTime, VTime* outLastAccess ) const
{
	WIN32_FILE_ATTRIBUTE_DATA info;
	SYSTEMTIME stime;

	XWinFullPath path( fOwner->GetPath());
	DWORD winErr = ::GetFileAttributesExW( path, GetFileExInfoStandard, &info) ? 0 : ::GetLastError();

	if (winErr == 0)
	{
		if ( outLastModification )
		{
			::FileTimeToSystemTime( &info.ftLastWriteTime, &stime);
			outLastModification->FromUTCTime( stime.wYear, stime.wMonth, stime.wDay, stime.wHour, stime.wMinute, stime.wSecond, stime.wMilliseconds );
		}
		if ( outCreationTime )
		{
			::FileTimeToSystemTime( &info.ftCreationTime, &stime);
			outCreationTime->FromUTCTime( stime.wYear, stime.wMonth, stime.wDay, stime.wHour, stime.wMinute, stime.wSecond, stime.wMilliseconds );
		}
		if ( outLastAccess )
		{
			::FileTimeToSystemTime( &info.ftLastAccessTime, &stime);
			outLastAccess->FromUTCTime( stime.wYear, stime.wMonth, stime.wDay, stime.wHour, stime.wMinute, stime.wSecond, stime.wMilliseconds );
		}
	}

	return MAKE_NATIVE_VERROR( winErr);
}

VError XWinFolder::GetVolumeCapacity (sLONG8 *outTotalBytes ) const
{
	VFilePath filePath( fOwner->GetPath() );
	
	XWinFullPath path( filePath);
		
	uLONG8 freeBytesAvailable = 0, totalNumberOfBytes = 0, totalNumberOfFreeBytes = 0;
	DWORD winErr = ::GetDiskFreeSpaceExW( path, (PULARGE_INTEGER) &freeBytesAvailable, (PULARGE_INTEGER) &totalNumberOfBytes, (PULARGE_INTEGER) &totalNumberOfFreeBytes) ? 0 : ::GetLastError();

	if (winErr == 0)
		*outTotalBytes = totalNumberOfBytes;
	
	return MAKE_NATIVE_VERROR( winErr);

}

VError XWinFolder::GetVolumeFreeSpace(sLONG8 *outFreeBytes, bool inWithQuotas ) const
{
	VFilePath filePath( fOwner->GetPath() );
	
	XWinFullPath path( filePath);
		
	uLONG8 freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
	DWORD winErr = ::GetDiskFreeSpaceExW( path, (PULARGE_INTEGER) &freeBytesAvailable, (PULARGE_INTEGER) &totalNumberOfBytes, (PULARGE_INTEGER) &totalNumberOfFreeBytes) ? 0 : ::GetLastError();

	if (winErr == 0)
		*outFreeBytes = inWithQuotas ? freeBytesAvailable : totalNumberOfFreeBytes;
	
	return MAKE_NATIVE_VERROR( winErr);
}

VError XWinFolder::IsHidden(bool &outIsHidden)
{
	VError err;
	
	DWORD outAttrb;
	err=GetAttributes(&outAttrb);
	if(err==VE_OK)
	{
		outIsHidden= (outAttrb & FILE_ATTRIBUTE_HIDDEN) == FILE_ATTRIBUTE_HIDDEN;
	}
	return err;
}
VError XWinFolder::GetAttributes( DWORD *outAttrb ) const
{
	XWinFullPath path( fOwner->GetPath() );
    *outAttrb = ::GetFileAttributesW(path);

	DWORD winErr = (*outAttrb != INVALID_FILE_ATTRIBUTES ) ? 0 : ::GetLastError();

	return MAKE_NATIVE_VERROR( winErr);
}


VError XWinFolder::SetAttributes( DWORD inAttrb) const
{
	XWinFullPath path( fOwner->GetPath() );

	DWORD winErr = ::SetFileAttributesW( path, inAttrb) ? 0 : ::GetLastError();

	return MAKE_NATIVE_VERROR( winErr);
}


VError XWinFolder::GetPosixPath( VString& outPath) const
{
	fOwner->GetPath().GetPosixPath( outPath);
	return VE_OK;
}

bool XWinFolder::GetIconFile( uLONG inIconSize, HICON &outIconFile ) const
{
	if (inIconSize <= 0)
		inIconSize = 32;

	UINT flags;
	if (inIconSize <= 16)
		flags = SHGFI_ICON | SHGFI_SMALLICON;	//on reccupere la 16 * 16
	else
		flags = SHGFI_ICON | SHGFI_LARGEICON;   //on reccupere la 32 * 32

	SHFILEINFOW psfi;
	DWORD winErr = ::SHGetFileInfoW( fOwner->GetPath().GetPath().GetCPointer(), 0, &psfi, sizeof(SHFILEINFOW), flags );

	if ( winErr )
		outIconFile = psfi.hIcon;
	else
		outIconFile = NULL;

	return (winErr != 0);
}



VError XWinFolder::MakeWritableByAll()
{
	// TBD: see in oldbox::VPath
	return VE_OK;
}


bool XWinFolder::IsWritableByAll()
{
	// TBD: see in oldbox::VPath
	return true;
}


VError XWinFolder::RetainSystemFolder( ESystemFolderKind inFolderKind, bool inCreateFolder, VFolder **outFolder )
{
	VString folderPath;
	UniChar path[4096];
	size_t maxLength = sizeof( path)/sizeof( UniChar);
	path[maxLength-2]=0;
	VFolder* sysFolder = NULL;
	DWORD winErr = ERROR_PATH_NOT_FOUND;
	switch( inFolderKind)
	{
		case eFK_Executable:
			{
				DWORD pathLength = ::GetModuleFileNameW( NULL, path, (DWORD) maxLength);
				if (testAssert( pathLength != 0 && !path[maxLength-2])) 
				{
					folderPath.FromUniCString( path );
					VFile exeFile( folderPath );
					sysFolder = exeFile.RetainParentFolder();
					winErr = 0;
				}
				break;
			}

		case eFK_Temporary:
			{
				DWORD length = ::GetTempPathW( (DWORD) maxLength, path);
				if ( (length > 0) && (length <= maxLength) && !path[maxLength-2])
				{
					DWORD size = ::GetLongPathNameW( path, NULL, 0);
					if (size > 0)
					{
						UniChar *p = folderPath.GetCPointerForWrite( size-1);
						if (p != NULL)
						{
							DWORD result = ::GetLongPathNameW( path, p, size);
							if (result == 0)
								winErr = GetLastError();
							else
								winErr = 0;
							folderPath.Validate( result);
							sysFolder = new VFolder( folderPath );
						}
					}
				}
				break;
			}

		default:
			{
				int type;
				switch( inFolderKind)
				{
					case eFK_UserDocuments:			type = CSIDL_PERSONAL; break;
					case eFK_CommonPreferences:		type = CSIDL_COMMON_APPDATA; break;
					case eFK_UserPreferences:		type = CSIDL_APPDATA; break;
					case eFK_CommonApplicationData:	type = CSIDL_COMMON_APPDATA; break;
					case eFK_UserApplicationData:	type = CSIDL_APPDATA; break;
					case eFK_CommonCache:			type = CSIDL_COMMON_APPDATA; break;
					case eFK_UserCache:				type = CSIDL_LOCAL_APPDATA; break;
					case eFK_StartupItemsFolder:	type = CSIDL_COMMON_STARTUP; break;
					default: xbox_assert( false);	type = CSIDL_APPDATA; break;
				}
				HRESULT result = ::SHGetFolderPathW( NULL, type, NULL, SHGFP_TYPE_CURRENT, path);
				if ( result == S_OK || result == S_FALSE )	// S_FALSE means The CSIDL is valid, but the folder does not exist.
				{
					folderPath.FromUniCString( path);
					if (folderPath[folderPath.GetLength()-1] != FOLDER_SEPARATOR)
						folderPath += FOLDER_SEPARATOR;
					sysFolder = new VFolder( folderPath );
					winErr = 0;
				}
				break;
			}
	}

	if ( (sysFolder != NULL) && !sysFolder->Exists() )
	{
		if (inCreateFolder)
		{
			if (sysFolder->CreateRecursive() != VE_OK)
				ReleaseRefCountable( &sysFolder);
		}
		else
		{
			ReleaseRefCountable( &sysFolder);
		}
	}

	*outFolder = sysFolder;
	return MAKE_NATIVE_VERROR( winErr);
}

//========================================================================================

XWinFolderIterator::XWinFolderIterator( const VFolder *inFolder, FileIteratorOptions inOptions)
: fOptions( inOptions)
, fFileSystem( inFolder->GetFileSystem())
{
	fIndex = 0;
	fHandle = INVALID_HANDLE_VALUE;
	fRootPath.FromFilePath( inFolder->GetPath());
}


XWinFolderIterator::~XWinFolderIterator()
{
	if (fHandle != INVALID_HANDLE_VALUE)
		testAssert( ::FindClose( fHandle));
}


VFolder* XWinFolderIterator::Next()
{
	assert( fIndex >= 0);

	WIN32_FIND_DATAW fdw;

	Boolean isOK = false;
	if (fIndex == 0)
	{
		XWinFullPath path( fRootPath, "*");
		fHandle = ::FindFirstFileW( path, &fdw);
		isOK = (fHandle != INVALID_HANDLE_VALUE) ? true : false;
		fIndex = 1;
	}
	else
	{
		isOK = ::FindNextFileW( fHandle, &fdw) != 0;
		assert( isOK || ::GetLastError() == ERROR_NO_MORE_FILES);
		++fIndex;
	}
	
	VFolder *path = NULL;
	if (isOK)
	{
		do {
			VStr255 name;
			DWORD attributes;
			name.FromUniCString( fdw.cFileName);
			attributes = fdw.dwFileAttributes;
			path = _GetPath( name, attributes);
			if (path == NULL)
			{
				isOK = ::FindNextFileW( fHandle, &fdw) != 0;
				++fIndex;
				assert( isOK || ::GetLastError() == ERROR_NO_MORE_FILES);
			}
		} while( path == NULL && isOK);
	}
	
	return path;
}


VFolder *XWinFolderIterator::_GetPath( const VString& inFileName, DWORD inAttributes)
{
	VFolder *path = NULL;
	if (inFileName[0] != CHAR_FULL_STOP)
	{
		bool isInvisible = (inAttributes & FILE_ATTRIBUTE_HIDDEN) ? true : false;
		bool isFolder = (inAttributes & FILE_ATTRIBUTE_DIRECTORY) ? true : false;
		if (!isInvisible || (fOptions & FI_WANT_INVISIBLES))
		{
			if (isFolder)
			{
				if (fOptions & FI_WANT_FOLDERS)
				{
					VFilePath currentPath;
					currentPath.FromFilePath( fRootPath);
					currentPath.ToSubFolder( inFileName);
					path = new VFolder(currentPath, fFileSystem);
				}
			}
			else
			{
				// it's a file, maybe it's an alias to a folder
				if (fOptions & FI_RESOLVE_ALIASES)
				{
					VFilePath currentPath;
					currentPath.FromFilePath( fRootPath);
					currentPath.SetFileName( inFileName);
					VFile theFile( currentPath);
					if (theFile.IsAliasFile())
					{
						VFilePath resolvedPath;
						if ( (theFile.ResolveAlias( resolvedPath) == VE_OK) && resolvedPath.IsFolder())
						{
							if ( (fFileSystem == NULL) || resolvedPath.IsChildOf( fFileSystem->GetRoot()) )
								path = new VFolder( resolvedPath, fFileSystem);
						}
					}
				}
			}
		}
	}
	return path;
}

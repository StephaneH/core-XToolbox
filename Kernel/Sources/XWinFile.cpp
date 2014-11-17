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

const LARGE_INTEGER LARGEZERO = { 0, 0 };

static DWORD CALLBACK CopyProgressRoutine( LARGE_INTEGER TotalFileSize, LARGE_INTEGER TotalBytesTransferred, LARGE_INTEGER StreamSize, LARGE_INTEGER StreamBytesTransferred, DWORD dwStreamNumber, DWORD dwCallbackReason, HANDLE hSourceFile, HANDLE hDestinationFile, LPVOID lpData )
{
	VTask::Yield();
	return PROGRESS_CONTINUE;
}

static HRESULT WIN_ResolveShortcutOneLevel( VString& ioPath, IShellLinkW *psl, IPersistFile *ppf, bool inIsUIAllowed, bool *outIsFolder)
{
	// Load the shortcut
	HRESULT hres = ppf->Load( (LPCWSTR) ioPath.GetCPointer(), STGM_READ);

	// resolve it
	if (hres == S_OK)
	{
		hres = psl->Resolve( NULL,  inIsUIAllowed ? SLR_ANY_MATCH : (SLR_ANY_MATCH | SLR_NO_UI));
		if (testAssert( hres == S_OK))
		{
			const sLONG size=4096;
			UniChar newPath[size];
			WIN32_FIND_DATAW wfd;
			hres = psl->GetPath( newPath, sizeof(newPath), (WIN32_FIND_DATAW*)&wfd, SLGP_UNCPRIORITY);
			if (hres == S_OK)
			{
				*outIsFolder = (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? true : false;
				ioPath.FromUniCString( newPath);
			}
		}
	}
	return hres;
}


static HRESULT WIN_ResolveShortcut( VString& ioPath, bool inIsRecursive, bool inIsUIAllowed, bool *outIsFolder)
{
	*outIsFolder = false; // L.E. 8/01/00 en fait on ne sait pas encore
	IShellLinkW *theShellLink = NULL;

	HRESULT hres = ::CoCreateInstance( CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void **)&theShellLink); 
	if (hres == S_OK)
	{
		IPersistFile *ppf = NULL;
		hres = theShellLink->QueryInterface( IID_IPersistFile, (void **)&ppf);

		if (hres == S_OK)
		{
			sLONG success = 0;
			do {
				hres = ::WIN_ResolveShortcutOneLevel( ioPath, theShellLink, ppf, inIsUIAllowed, outIsFolder);
				if (hres == S_OK)
					++success;
				// si on tombe sur un folder, on s'arrete !
			} while( (hres == S_OK) && inIsRecursive && !*outIsFolder);
			
			// si on a pu en resoudre au moins on dit que c'est bon
			if ( (hres != S_OK) && (success > 0) )
				hres = S_OK;
			
			ppf->Release();
		}
	
		theShellLink->Release();
	}
	return hres;
}


static HRESULT WIN_CreateShortcut( const VString& inShortcutFilePath, const VString& inTarget)
{
	IShellLink		*theShellLink = NULL;
	IPersistFile	*ppf = NULL;

	HRESULT hr = CoCreateInstance( CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void **)&theShellLink); 

	if (hr == S_OK)
	{
		VStringConvertBuffer buff( inTarget, VTC_Win32Ansi);	// J.A. 17/5/2002 : Microsoft® Windows 95 and Microsoft® Windows 98 only support ANSI version (IShellLinkA).

		hr = theShellLink->SetPath( (LPCSTR) buff.GetCPointer()); 

		theShellLink->SetDescription(""); 

		if (hr == S_OK)
		{
			hr = theShellLink->QueryInterface( IID_IPersistFile, (void **)&ppf);
			if (hr == S_OK)
			{
				hr = ppf->Save( (LPWSTR)inShortcutFilePath.GetCPointer(), TRUE); 				
				ppf->Release();
			}
		}
		theShellLink->Release();
	}
	
	return hr;
}

//=====================================================================================================================================

XWinFileDesc::~XWinFileDesc()
{
	::CloseHandle( fHandle);
}

VError XWinFileDesc::GetSize( sLONG8 *outSize) const
{
	LARGE_INTEGER sizex;

	DWORD winErr = ::GetFileSizeEx( fHandle, &sizex) ? 0 : ::GetLastError();

	*outSize = sizex.QuadPart;

	return MAKE_NATIVE_VERROR( winErr);
}

VError XWinFileDesc::SetSize( sLONG8 inSize ) const
{
	LARGE_INTEGER sizex;
	sizex.QuadPart = inSize;

	VTaskLock locker(&fMutex);

	// get the current pos to set it back afterwards
	LARGE_INTEGER oldPos;
	DWORD winErr = ::SetFilePointerEx( fHandle, LARGEZERO, &oldPos, FILE_CURRENT) ? 0 : ::GetLastError();

	if (winErr == 0)
	{
		winErr = ::SetFilePointerEx( fHandle, sizex, NULL, FILE_BEGIN) ? 0 : ::GetLastError();

		if (winErr == 0)
		{
			winErr = ::SetEndOfFile( fHandle) ? 0 : ::GetLastError();
		}

		// tries to set back the cur pos silently
		BOOL ok = ::SetFilePointerEx( fHandle, oldPos, NULL, FILE_BEGIN);
		assert( ok);
	}
	
	return MAKE_NATIVE_VERROR( winErr);
}


VError XWinFileDesc::GetData( void *outData, VSize& ioCount, sLONG8 inOffset, bool inFromStart ) const
{
	DWORD winErr;
	DWORD rcount = 0;

	// sometimes ReadFile or WriteFile return ERROR_NOT_ENOUGH_MEMORY but if we try again it works.
	// so we just retry at most four times.
	sLONG pass = 0;
	do
	{
		if (pass > 0)
			VTask::YieldNow();
		if ( (inOffset != 0) || inFromStart )
		{
			VTaskLock locker(&fMutex);

			LARGE_INTEGER newPosx;
			newPosx.QuadPart = inOffset;

			winErr =::SetFilePointerEx( fHandle, newPosx, NULL, inFromStart ? FILE_BEGIN : FILE_CURRENT) ? 0 : ::GetLastError();
			if (winErr == 0)
				winErr = ::ReadFile( fHandle, outData, (DWORD) ioCount, &rcount, NULL) ? 0 : ::GetLastError();
		}
		else
		{
			winErr = ::ReadFile( fHandle, outData, (DWORD) ioCount, &rcount, NULL) ? 0 : ::GetLastError();
		}
	} while( ((winErr == ERROR_NOT_ENOUGH_MEMORY) || (winErr == ERROR_INVALID_USER_BUFFER)) && (++pass < 4) );

	if ( (winErr == 0) && (rcount != (DWORD) ioCount) )
		winErr = ERROR_HANDLE_EOF;

	ioCount = (VSize) rcount;
	
	return MAKE_NATIVE_VERROR( winErr);
}


VError XWinFileDesc::PutData(const void *inData, VSize& ioCount, sLONG8 inOffset, bool inFromStart) const
{
	DWORD winErr;
	DWORD wrote = 0;

	sLONG pass = 0;
	do
	{
		if (pass > 0)
			VTask::YieldNow();
		if ( (inOffset != 0) || inFromStart )
		{
			VTaskLock locker(&fMutex);

			LARGE_INTEGER newPosx;
			newPosx.QuadPart = inOffset;

			winErr =::SetFilePointerEx( fHandle, newPosx, NULL, inFromStart ? FILE_BEGIN : FILE_CURRENT) ? 0 : ::GetLastError();
			if (winErr == 0)
				winErr = ::WriteFile( fHandle, inData, (DWORD) ioCount, &wrote, NULL) ? 0 : ::GetLastError();
		}
		else
		{
			winErr = ::WriteFile( fHandle, inData, (DWORD) ioCount, &wrote, NULL) ? 0 : ::GetLastError();
		}
	} while( ((winErr == ERROR_NOT_ENOUGH_MEMORY) || (winErr == ERROR_INVALID_USER_BUFFER)) && (++pass < 4) );

	ioCount = (VSize) wrote;

	return MAKE_NATIVE_VERROR( winErr);
}


VError XWinFileDesc::GetPos( sLONG8 *outPos) const
{
	LARGE_INTEGER posx;
	DWORD winErr = ::SetFilePointerEx( fHandle, LARGEZERO, &posx, FILE_CURRENT) ? 0 : ::GetLastError();
	*outPos = posx.QuadPart;

	return MAKE_NATIVE_VERROR( winErr);
}


VError XWinFileDesc::SetPos( sLONG8 inOffset, bool inFromStart ) const
{
	LARGE_INTEGER newPosx;
	newPosx.QuadPart = inOffset;

	DWORD winErr = ::SetFilePointerEx( fHandle, newPosx, NULL, inFromStart ? FILE_BEGIN : FILE_CURRENT) ? 0 : ::GetLastError();

	return MAKE_NATIVE_VERROR( winErr);
}


VError XWinFileDesc::Flush() const
{
	DWORD winErr = ::FlushFileBuffers( fHandle) ? 0 : ::GetLastError();

	return MAKE_NATIVE_VERROR( winErr);
}


// --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---


XWinFile::XWinFile()
{
	fOwner = NULL;
}


XWinFile::~XWinFile()
{
}

VError XWinFile::Create( FileCreateOptions inOptions) const
{
	DWORD winErr;
	XWinFullPath path( fOwner->GetPath() );
	HANDLE fileHandle = ::CreateFileW( path, GENERIC_READ, 0, NULL, (inOptions & FCR_Overwrite) ? CREATE_ALWAYS : CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if ( fileHandle != INVALID_HANDLE_VALUE )
	{
		::CloseHandle( fileHandle);
		winErr = 0;
	}
	else
	{
		winErr = ::GetLastError();
	}

	return MAKE_NATIVE_VERROR( winErr);
}


// return a pointer to the created file. A null pointer value indicates an error.
VError XWinFile::Open( FileAccess inFileAccess, FileOpenOptions inOptions, VFileDesc** outFileDesc) const
{
	DWORD winErr;
	
	// try to open
	XWinFullPath path( fOwner->GetPath(), (inOptions & FO_OpenResourceFork) ? ":AFP_Resource" : NULL);

	HANDLE fileHandle = NULL;
	DWORD access, share;

	DWORD openMode;
	if (inOptions & FO_Overwrite)
	{
		if (inOptions & FO_CreateIfNotFound)
			openMode = CREATE_ALWAYS;	// warning: that keeps security attibutes
		else
			openMode = TRUNCATE_EXISTING;
	}
	else
	{
		if (inOptions & FO_CreateIfNotFound)
			openMode = OPEN_ALWAYS;
		else
			openMode = OPEN_EXISTING;
	}

	DWORD options = FILE_ATTRIBUTE_NORMAL;
	if (inOptions & FO_SequentialScan)
		options |= FILE_FLAG_SEQUENTIAL_SCAN;
	if (inOptions & FO_RandomAccess)
		options |= FILE_FLAG_RANDOM_ACCESS;
	if (inOptions & FO_WriteThrough)
		options |= FILE_FLAG_WRITE_THROUGH;
	
	switch( inFileAccess)
	{
	
		case FA_READ: 
			access = GENERIC_READ;
			share = FILE_SHARE_READ | FILE_SHARE_WRITE;
			fileHandle = ::CreateFileW( path, access, share, NULL, openMode, options, NULL);
			break;

		case FA_READ_WRITE:
			access = GENERIC_READ | GENERIC_WRITE;
			share = FILE_SHARE_READ;
			fileHandle = ::CreateFileW( path, access, share, NULL, openMode, options, NULL);
			break;

		case FA_SHARED:
			access = GENERIC_READ | GENERIC_WRITE;
			share = FILE_SHARE_READ | FILE_SHARE_WRITE;
			fileHandle = ::CreateFileW( path, access, share, NULL, openMode, options, NULL);
			break;

		case FA_MAX:
			access = GENERIC_READ | GENERIC_WRITE;
			share = FILE_SHARE_READ;
			fileHandle = ::CreateFileW( path, access, share, NULL, openMode, options, NULL);
			if (fileHandle == INVALID_HANDLE_VALUE)
			{
				access = GENERIC_READ;
				share = FILE_SHARE_READ | FILE_SHARE_WRITE;
				fileHandle = ::CreateFileW( path, access, share, NULL, openMode, options, NULL);
			}
			break;

		default:
			assert( false);
			fileHandle = INVALID_HANDLE_VALUE;
			break;	//	Bad FileAccess
	}

	if ( fileHandle != INVALID_HANDLE_VALUE )
	{
		winErr = 0;
		if ( outFileDesc )
			*outFileDesc = new VFileDesc( fOwner, fileHandle, inFileAccess );
		else
			::CloseHandle(fileHandle);
	}
	else
	{
		winErr = ::GetLastError();
	}

	return MAKE_NATIVE_VERROR( winErr);
}


VError XWinFile::Copy( const VFilePath& inDestination, VFileSystem *inDestinationFileSystem, VFile** outFile, FileCopyOptions inOptions ) const
{
	VFilePath newPath( inDestination);
	
	if (newPath.IsFolder())
	{
		VStr255 name;
		fOwner->GetName( name);
		newPath.SetFileName( name);
	}

	XWinFullPath oldWinPath( fOwner->GetPath() );
	XWinFullPath newWinPath( newPath );

	DWORD flags = 0;
	if ((inOptions & FCP_Overwrite) == 0)
		flags |= COPY_FILE_FAIL_IF_EXISTS;

	// COPY_FILE_NO_BUFFERING only available in vista and ws2008
	// "The copy operation is performed using unbuffered I/O, bypassing system I/O cache resources. Recommended for very large file transfers."
	if (VSystem::IsSystemVersionOrAbove( WIN_VISTA))
	{
		bool withBuffering = true;
		
		WIN32_FILE_ATTRIBUTE_DATA info;
		if (::GetFileAttributesExW( oldWinPath, GetFileExInfoStandard, &info))
		{
			LARGE_INTEGER i;
			i.LowPart = info.nFileSizeLow;
			i.HighPart = (LONG) info.nFileSizeHigh;
			withBuffering = (i.QuadPart < 100 * 1024*1024);	// 100 Mo
		}

		if (!withBuffering)
		{
			#ifdef COPY_FILE_NO_BUFFERING
			flags |= COPY_FILE_NO_BUFFERING;
			#else
			flags |= 0x00001000;
			#endif
		}
	}

	BOOL canceled = FALSE;
	DWORD winErr = ::CopyFileExW( oldWinPath, newWinPath, &CopyProgressRoutine, NULL, &canceled, flags) ? 0 : ::GetLastError();

	if (outFile)
	{
		*outFile = (winErr == 0) ? new VFile( newPath, inDestinationFileSystem) : NULL;
	}
	
	return MAKE_NATIVE_VERROR( winErr);
}

VError XWinFile::Exchange(const VFile& inExhangeWith) const
{
	DWORD winErr=-1;
	return MAKE_NATIVE_VERROR( winErr);
}


// Rename the file with the given name ( foofoo.html ).
VError XWinFile::Rename( const VString& inName, VFile** outFile ) const
{
	VFilePath newPath( fOwner->GetPath() );
	newPath.SetFileName( inName );
	
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
		*outFile = (winErr == 0) ? new VFile( newPath, fOwner->GetFileSystem()) : NULL;
	}

	return MAKE_NATIVE_VERROR( winErr);
}


VError XWinFile::Move( const VFilePath& inNewPath, VFileSystem *inDestinationFileSystem, VFile** outFile, FileCopyOptions inOptions) const
{
	VFilePath newPath( inNewPath);
	
	if (newPath.IsFolder())
	{
		VStr255 name;
		fOwner->GetName( name);
		newPath.SetFileName( name);
	}

	XWinFullPath oldWinPath( fOwner->GetPath() );
	XWinFullPath newWinPath( newPath );
	
	DWORD flags = MOVEFILE_COPY_ALLOWED;
	if (inOptions & FCP_Overwrite)
		flags |= MOVEFILE_REPLACE_EXISTING;

	DWORD winErr = ::MoveFileWithProgressW( oldWinPath, newWinPath, &CopyProgressRoutine, NULL, flags) ? 0 : ::GetLastError();

	if (outFile)
	{
		*outFile = (winErr == 0) ? new VFile( newPath, inDestinationFileSystem) : NULL;
	}

	return MAKE_NATIVE_VERROR( winErr);
}


VError XWinFile::Delete() const
{
	XWinFullPath path( fOwner->GetPath() );
	
	DWORD winErr = ::DeleteFileW(path) ? 0  : ::GetLastError();

	return MAKE_NATIVE_VERROR( winErr);
}


VError XWinFile::MoveToTrash() const
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



// test if the file allready exist.
bool XWinFile::Exists() const
{
	XWinFullPath path( fOwner->GetPath() );
	
	DWORD retval = ::GetFileAttributesW( path);
	bool found = false;

	if (retval & FILE_ATTRIBUTE_DIRECTORY)
	{
		// it's a directory !
		// don't put an assert but say the file does not exist (m.c)
	}
	else
		found = (retval != INVALID_FILE_ATTRIBUTES);

	return found;
}


bool XWinFile::HasResourceFork() const
{
	XWinFullPath path( fOwner->GetPath(), ":AFP_Resource");

	DWORD retval = ::GetFileAttributesW( path);
	return retval != INVALID_FILE_ATTRIBUTES;
}


VError XWinFile::GetAttributes( DWORD *outAttrb ) const
{
	XWinFullPath path( fOwner->GetPath() );
    *outAttrb = ::GetFileAttributesW( path);

	DWORD winErr = (*outAttrb != INVALID_FILE_ATTRIBUTES ) ? 0 : ::GetLastError();

	return MAKE_NATIVE_VERROR( winErr);
}


VError XWinFile::SetAttributes( DWORD inAttrb) const
{
	XWinFullPath path( fOwner->GetPath() );
	
	DWORD winErr = ::SetFileAttributesW(path,inAttrb) ? 0: ::GetLastError();

	return MAKE_NATIVE_VERROR( winErr);
}

// GetFileAttributes && FSGetCatalogInfo.
VError XWinFile::GetTimeAttributes( VTime* outLastModification, VTime* outCreationTime, VTime* outLastAccess ) const
{
	WIN32_FILE_ATTRIBUTE_DATA info;
	SYSTEMTIME stime;

	XWinFullPath path( fOwner->GetPath());
	DWORD winErr = ::GetFileAttributesExW( path, GetFileExInfoStandard, &info) ? 0 : ::GetLastError();

	if (winErr == 0)
	{
		// m.c: I remove the millisecond precision. Two reasons for that:
		// 1) on Mac there is no such precision and it does't cause any problem
		// 2) it makes it more difficult to compare date values (especially with HTTP,
		//		the protocol doesn't support the second)
		// careful if you change that, you will break the "If-Modified-Since" support in the web server

		if ( outLastModification )
		{
			::FileTimeToSystemTime( &info.ftLastWriteTime, &stime);
			outLastModification->FromUTCTime( stime.wYear, stime.wMonth, stime.wDay, stime.wHour, stime.wMinute, stime.wSecond, 0 /* stime.wMilliseconds */);
		}
		if ( outCreationTime )
		{
			::FileTimeToSystemTime( &info.ftCreationTime, &stime);
			outCreationTime->FromUTCTime( stime.wYear, stime.wMonth, stime.wDay, stime.wHour, stime.wMinute, stime.wSecond, 0 /* stime.wMilliseconds */);
		}
		if ( outLastAccess )
		{
			::FileTimeToSystemTime( &info.ftLastAccessTime, &stime);
			outLastAccess->FromUTCTime( stime.wYear, stime.wMonth, stime.wDay, stime.wHour, stime.wMinute, stime.wSecond,0 /* stime.wMilliseconds */);
		}
	}

	return MAKE_NATIVE_VERROR( winErr);
}

VError XWinFile::GetFileAttributes( EFileAttributes &outFileAttributes ) const
{
	DWORD fileAttributes;
	outFileAttributes = 0;
	VError err = GetAttributes( &fileAttributes );
	if ( fileAttributes & FILE_ATTRIBUTE_HIDDEN )
		outFileAttributes |= FATT_HidenFile;
	if ( fileAttributes & FILE_ATTRIBUTE_READONLY )
		outFileAttributes |= FATT_LockedFile;

	return err;
}

VError XWinFile::SetTimeAttributes( const VTime *inLastModification, const VTime *inCreationTime, const VTime *inLastAccess ) const
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

		XWinFullPath path( fOwner->GetPath() );
		HANDLE fileHandle = ::CreateFileW( path, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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

VError XWinFile::SetFileAttributes( EFileAttributes inFileAttributes ) const
{
	DWORD fileAttributes;
	VError err = GetAttributes( &fileAttributes );

	if ( err == VE_OK )
	{
		if ( inFileAttributes & FATT_HidenFile )
			fileAttributes |= FILE_ATTRIBUTE_HIDDEN;
		else
			fileAttributes &= ~FILE_ATTRIBUTE_HIDDEN;

		if ( inFileAttributes & FATT_LockedFile )
			fileAttributes |= FILE_ATTRIBUTE_READONLY;
		else
			fileAttributes &= ~FILE_ATTRIBUTE_READONLY;

		err = SetAttributes( fileAttributes );
	}
	
	return err;
}

VError XWinFile::GetSize( sLONG8 *outSize ) const
{
	WIN32_FILE_ATTRIBUTE_DATA info;

	XWinFullPath path( fOwner->GetPath());
	DWORD winErr = ::GetFileAttributesExW( path, GetFileExInfoStandard, &info) ? 0 : ::GetLastError();

	if (winErr == 0 )
	{
		LARGE_INTEGER i;
		i.LowPart = info.nFileSizeLow;
		i.HighPart = (LONG) info.nFileSizeHigh;
		*outSize =  i.QuadPart;
	}

	return MAKE_NATIVE_VERROR( winErr);
}

VError XWinFile::GetResourceForkSize( sLONG8 *outSize) const
{
	WIN32_FILE_ATTRIBUTE_DATA info;

	XWinFullPath path( fOwner->GetPath(), ":AFP_Resource");
	DWORD winErr = ::GetFileAttributesExW( path, GetFileExInfoStandard, &info) ? 0 : ::GetLastError();

	if (winErr == 0 )
	{
		LARGE_INTEGER i;
		i.LowPart = info.nFileSizeLow;
		i.HighPart = (LONG) info.nFileSizeHigh;
		*outSize =  i.QuadPart;
	}

	return MAKE_NATIVE_VERROR( winErr);
}

// return DiskFreeSpace.
VError XWinFile::GetVolumeFreeSpace( sLONG8 *outFreeBytes, sLONG8 *outTotalBytes, bool inWithQuotas ) const
{
	VFilePath filePath( fOwner->GetPath() );
	filePath.ToParent();
	
	XWinFullPath path( filePath);
		
	uLONG8 freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
	DWORD winErr = ::GetDiskFreeSpaceExW( path, (PULARGE_INTEGER) &freeBytesAvailable, (PULARGE_INTEGER) &totalNumberOfBytes, (PULARGE_INTEGER) &totalNumberOfFreeBytes) ? 0 : ::GetLastError();

	if (winErr == 0){
		*outFreeBytes = inWithQuotas ? freeBytesAvailable : totalNumberOfFreeBytes;
		if(outTotalBytes != NULL){
			if(inWithQuotas){
				*outTotalBytes = totalNumberOfBytes;
			}
			else{
				
				assert(false);
				
				//Not done for the moment. I only did a few lines, untested.
				/*HANDLE fileHandle = ::CreateFileW( path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				if ( fileHandle != INVALID_HANDLE_VALUE ){
					GET_LENGTH_INFORMATION lengthInformation;
					DWORD lpBytesReturned;
					if(!DeviceIoControl(fileHandle, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &lengthInformation, sizeof(GET_LENGTH_INFORMATION), &lpBytesReturned, NULL)){
						winErr = GetLastError();
					}
					else{
						*outTotalBytes = lengthInformation.Length;
					}
					
					CloseHandle(fileHandle);
				}
				else{
					winErr = GetLastError();
				}*/
			}
		}
	}
	
	return MAKE_NATIVE_VERROR( winErr);
}


VError XWinFile::RevealInFinder() const
{
	VString fullPath;
	fOwner->GetPath( fullPath);
	
	return RevealPathInFinder( fullPath);
}


VError XWinFile::RevealPathInFinder( const VString inPath)
{
  	ULONG			attrs;
	ULONG			chEaten = 0;
	LPSHELLFOLDER	pDeskIntf = NULL;
	LPITEMIDLIST	pShNameID = NULL;

	HRESULT hr = ::SHGetDesktopFolder( &pDeskIntf);
	if (hr == S_OK)
	{
		hr = pDeskIntf->ParseDisplayName( NULL, NULL, (LPOLESTR) inPath.GetCPointer(), &chEaten, &pShNameID, &attrs);
		pDeskIntf->Release();
	}

	if (hr == S_OK)
	{
		typedef HRESULT (*SHOpenFolderAndSelectItemsPtr) (LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST *apidl,DWORD dwFlags);
		static bool sInited = false;
		static SHOpenFolderAndSelectItemsPtr sPtr;
		if (!sInited)
		{
			sPtr = (SHOpenFolderAndSelectItemsPtr) ::GetProcAddress( ::GetModuleHandle( "shell32.dll"), "SHOpenFolderAndSelectItems");
			sInited = true;
		}
		if (sPtr)
		{
			hr = sPtr( pShNameID, 0, NULL, 0);
		}
	}

	DWORD winErr = (hr == S_OK) ? 0 : (DWORD) hr;
	return MAKE_NATIVE_VERROR( winErr);
}


VError XWinFile::ResolveAlias( VFilePath &outTargetPath ) const
{
	VString thePath;
	fOwner->GetPath( thePath);
	bool isFolder = false;

	HRESULT hr = WIN_ResolveShortcut( thePath, true, false, &isFolder);
	if (hr == S_OK )
	{
		if (isFolder && !thePath.IsEmpty() && thePath[thePath.GetLength()-1] != FOLDER_SEPARATOR)
			thePath += FOLDER_SEPARATOR;
			
		outTargetPath.FromFullPath( thePath);
	}

	DWORD winErr = (hr == S_OK) ? 0 : (DWORD) hr;
	return MAKE_NATIVE_VERROR( winErr);
}


VError XWinFile::CreateAlias( const VFilePath &inTargetPath, const VFilePath *inRelativeAliasSource) const
{
	VStr255 target;
	VStr255 aliasPath;
	
	fOwner->GetPath(aliasPath);
	inTargetPath.GetPath(target);

	HRESULT hr = WIN_CreateShortcut( aliasPath, target );

	DWORD winErr = (hr == S_OK) ? 0 : (DWORD) hr;
	return MAKE_NATIVE_VERROR( winErr);
}


bool XWinFile::IsAliasFile() const
{
	VStr31 extension;
	fOwner->GetPath().GetExtension( extension);
	return extension.EqualToString( CVSTR( "LNK"), false);
}

VFileKind *XWinFile::CreatePublicFileKind( const VString& inID)
{
	// no public type yet on Windows.
	return NULL;
}

VFileKind* XWinFile::CreatePublicFileKindFromOsKind( const VString& inOsKind )
{
	return NULL;
}


VFileKind* XWinFile::CreatePublicFileKindFromExtension( const VString& inExtension )
{
	// sc 05/09/2011: create a dynamic file kind and register it
	VString id, ext, desc;
	VUUID uuid( true);

	uuid.GetString( id);
	id.Insert( L"dyn.", 1);
	
	if(!inExtension.BeginsWith(".",true))
		ext="."+inExtension;

	desc = GetFileTypeDescriptionFromExtension( ext);

	VectorOfFileExtension extensions;
	VectorOfFileKind osKind;
	extensions.push_back(inExtension);

	return VFileKind::Create( id, osKind, desc, extensions, VectorOfFileKind(0), true);
}

VString XWinFile::GetFileTypeDescriptionFromExtension(const VString& inPathOrExtension)
{
	SHFILEINFOW info;
	XBOX::VString desc;
	
	info.szTypeName[0]=0;
	::SHGetFileInfoW(inPathOrExtension.GetCPointer(),FILE_ATTRIBUTE_NORMAL,&info,sizeof(info),SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME);
	if(info.szTypeName[0])
		desc.FromUniCString((UniChar*)info.szTypeName);
	return desc;
}

VError XWinFile::ConformsTo( const VFileKind& inKind, bool *outConformant) const
{
	// sc 11/01/2011 method reviewed : check if the kind of this file is conform to inKind

	VStr31 extension;
	fOwner->GetPath().GetExtension( extension);
	
	VFileKind *fileKind = VFileKindManager::Get()->RetainFirstFileKindMatchingWithExtension( extension);
	if (fileKind != NULL)
	{
		*outConformant = IsFileKindConformsTo( *fileKind, inKind);
		fileKind->Release();
	}

	return VE_OK;
}

bool XWinFile::IsFileKindConformsTo( const VFileKind& inFirstKind, const VFileKind& inSecondKind)
{
	bool conform = false;

	if (inFirstKind.GetID() == inSecondKind.GetID())
	{
		conform = true;
	}
	else
	{
		// Check for parent kind IDs
		VectorOfFileKind parentIDs;
		if (inFirstKind.GetParentIDs( parentIDs))
		{
			for (VectorOfFileKind::iterator parentIDsIter = parentIDs.begin() ; parentIDsIter != parentIDs.end() && !conform ; ++parentIDsIter)
			{
				VFileKind *parentKind = VFileKindManager::Get()->RetainFileKind( *parentIDsIter);
				if (parentKind != NULL)
				{
					conform = IsFileKindConformsTo( *parentKind, inSecondKind);
					parentKind->Release();
				}
			}
		}
	}

	return conform;
}

bool XWinFile::GetIconFile( uLONG inIconSize, HICON &outIconFile ) const
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

VError XWinFile::GetPosixPath( VString& outPath) const
{
	fOwner->GetPath().GetPosixPath( outPath);
	return VE_OK;
}


// --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---


XWinFullPath::XWinFullPath( const VFilePath &inTarget, char *inAppendCString)
{
	inTarget.GetPath( fUniPath);

	if ( fUniPath.Find( CVSTR( "\\\\") ) == 1 )
		fUniPath.Insert( CVSTR("\\?\\UNC"), 2); /* \\?\UNC\<server>\<share> */
	else
		fUniPath.Insert( CVSTR("\\\\?\\"), 1); /* \\?\D:\<path> */

	if (inAppendCString != NULL)
		fUniPath.AppendCString( inAppendCString);
}


XWinFullPath::~XWinFullPath()
{
}


//========================================================================================


XWinFileIterator::XWinFileIterator( const VFolder *inFolder, FileIteratorOptions inOptions)
: fOptions( inOptions)
, fFileSystem( inFolder->GetFileSystem())
{
	fIndex = 0;
	fHandle = INVALID_HANDLE_VALUE;
	fRootPath.FromFilePath( inFolder->GetPath());
}


XWinFileIterator::~XWinFileIterator()
{
	if (fHandle != INVALID_HANDLE_VALUE)
		testAssert( ::FindClose( fHandle));
}


VFile* XWinFileIterator::Next()
{
	assert( fIndex >= 0);

	WIN32_FIND_DATAW fdw;

	BOOL isOK = false;
	if (fIndex == 0) {
		XWinFullPath path( fRootPath, "*");
		fHandle = ::FindFirstFileW( path, &fdw);
		isOK = (fHandle != INVALID_HANDLE_VALUE) ? true : false;
		fIndex = 1;
	} else {
		isOK = ::FindNextFileW( fHandle, &fdw);
		assert( isOK || ::GetLastError() == ERROR_NO_MORE_FILES);
		++fIndex;
	}
	
	VFile *path = NULL;
	if (isOK) {
		do {
			VStr255 name;
			DWORD attributes;
			name.FromUniCString( fdw.cFileName);
			attributes = fdw.dwFileAttributes;
			path = _GetPath( name, attributes);
			if (path == NULL) {
				isOK = ::FindNextFileW( fHandle, &fdw);
				++fIndex;
				assert( isOK || ::GetLastError() == ERROR_NO_MORE_FILES);
			}
		} while( path == NULL && isOK);
	}
	
	return path;
}


VFile *XWinFileIterator::_GetPath( const VString& inFileName, DWORD inAttributes)
{
	VFile *path = NULL;

	if (((inFileName.GetLength()==1) && (inFileName[0] == CHAR_FULL_STOP))
		|| ((inFileName.GetLength()==2) && (inFileName[0] == CHAR_FULL_STOP) && (inFileName[1] == CHAR_FULL_STOP)))
	{
		// skip "." and ".."
		;
	}
	else
	{
		bool isInvisible = (inAttributes & FILE_ATTRIBUTE_HIDDEN) ? true : false;
		bool isFolder = (inAttributes & FILE_ATTRIBUTE_DIRECTORY) ? true : false;
		if ( !isFolder && ( !isInvisible || (fOptions & FI_WANT_INVISIBLES)))
		{
			if (fOptions & FI_WANT_FILES)
			{
				VFilePath currentPath;
				currentPath.FromFilePath( fRootPath);
				currentPath.SetFileName( inFileName);
				VFile *theFile = new VFile( currentPath, fFileSystem);
				if ((fOptions & FI_RESOLVE_ALIASES) && theFile->IsAliasFile())
				{
					VFilePath resolvedPath;
					if ( (theFile->ResolveAlias(resolvedPath) == VE_OK) && resolvedPath.IsFile())
					{
						if ( (fFileSystem == NULL) || resolvedPath.IsChildOf( fFileSystem->GetRoot()) )
							path = new VFile(resolvedPath, fFileSystem);
					}
					theFile->Release();
				} else
					path = theFile;
				theFile = NULL;
			}
		}
	}
	return path;
}



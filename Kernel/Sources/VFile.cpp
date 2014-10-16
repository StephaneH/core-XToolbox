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
#include "VURL.h"
#include "VFileSystem.h"
#include "VErrorContext.h"
#include "VMemory.h"
#include "VValueBag.h"
#include "VTime.h"
#include "VTextConverter.h"


BEGIN_TOOLBOX_NAMESPACE

class VVolumeInfo: public VObject, public IRefCountable
{
public:
	VVolumeInfo( uLONG inTime, bool inWithQuotas, sLONG8 inFreeSpace, sLONG8 inTotalSpace)
		: fTime( inTime), fWithQuotas( inWithQuotas), fFreeSpace( inFreeSpace), fTotalSpace( inTotalSpace) {}
	 
	const	uLONG	fTime;
	const	bool	fWithQuotas;	// fFreeSpace & fTotalSpace have been computed with or without quotas
	const	sLONG8	fFreeSpace;
	const	sLONG8	fTotalSpace;
};

END_TOOLBOX_NAMESPACE

// --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---


VFileDesc::VFileDesc( const VFile *inFile, FileDescSystemRef inSystemRef, FileAccess inMode )
: fFile( inFile)
, fMode( inMode)
, fImpl( inSystemRef)
{
	if (fFile)
		fFile->Retain();
	fMode = inMode;
}


VFileDesc::~VFileDesc()
{
	if (fFile)
		fFile->Release();
}


sLONG8 VFileDesc::GetSize() const
{
	sLONG8	size;
	VError err = fImpl.GetSize( &size);
	
	xbox_assert( (size >= 0) || (err != VE_OK));

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( fFile, VE_STREAM_CANNOT_GET_SIZE, err);
		err = errThrow.GetError();
	}
	
	return (err == VE_OK) ? size : 0;
}


VError VFileDesc::SetSize( sLONG8 inSize ) const
{
	xbox_assert( inSize >= 0);

	VError err = fImpl.SetSize( inSize );

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( fFile, VE_STREAM_CANNOT_SET_SIZE, err);
		errThrow->SetLong8( "size", inSize);
		err = errThrow.GetError();
	}
	
	return err;
}


VError VFileDesc::GetData( void *outData, VSize inCount, sLONG8 inOffset, VSize *outActualCount) const
{
	xbox_assert( inCount >= 0);

	VSize size = inCount;
	VError err = fImpl.GetData( outData, size, inOffset, true);
	xbox_assert( (size == inCount) || (err != VE_OK) );

	if (outActualCount)
		*outActualCount = size;

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( fFile, VE_STREAM_CANNOT_GET_DATA, err);
		errThrow->SetLong( "count", inCount);
		errThrow->SetLong8( "offset", inOffset);

		sLONG8 dsize;
		if (fImpl.GetSize( &dsize) == VE_OK)
			errThrow->SetLong8( "size", dsize);

		err = errThrow.GetError();
	}

	return err;
}


VError VFileDesc::GetDataAtPos( void *outData, VSize inCount, sLONG8 inOffset, VSize *outActualCount) const
{
	xbox_assert( inCount >= 0);

	VSize size = inCount;
	VError err = fImpl.GetData( outData, size, inOffset, false);
	xbox_assert( (size == inCount) || (err != VE_OK) );

	if (outActualCount)
		*outActualCount = size;

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( fFile, VE_STREAM_CANNOT_GET_DATA, err);

		errThrow->SetLong( "count", inCount);

		sLONG8 pos;
		if (fImpl.GetPos( &pos) == VE_OK)
			errThrow->SetLong8( "offset", pos + inOffset);

		sLONG8 dsize;
		if (fImpl.GetSize( &dsize) == VE_OK)
			errThrow->SetLong8( "size", dsize);

		err = errThrow.GetError();
	}

	return err;
}


VError VFileDesc::PutData( const void *inData, VSize inCount, sLONG8 inOffset, VSize *outActualCount) const
{
	xbox_assert( inCount >= 0);
	
	VSize size = inCount;
	VError err = fImpl.PutData( inData, size, inOffset, true);
	xbox_assert( (size == inCount) || (err != VE_OK) );

	if (outActualCount)
		*outActualCount = size;

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( fFile, VE_STREAM_CANNOT_PUT_DATA, err);
		errThrow->SetLong( "count", inCount);
		errThrow->SetLong8( "offset", inOffset);

		sLONG8 dsize;
		if (fImpl.GetSize( &dsize) == VE_OK)
			errThrow->SetLong8( "size", dsize);

		err = errThrow.GetError();
	}

	return err;
}


VError VFileDesc::PutDataAtPos( const void *inData, VSize inCount, sLONG8 inOffset, VSize *outActualCount) const
{
	xbox_assert( inCount >= 0);

	VSize size = inCount;
	VError err = fImpl.PutData( inData, size, inOffset, false);
	xbox_assert( (size == inCount) || (err != VE_OK) );

	if (outActualCount)
		*outActualCount = size;

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( fFile, VE_STREAM_CANNOT_PUT_DATA, err);

		errThrow->SetLong( "count", inCount);

		sLONG8 pos;
		if (fImpl.GetPos( &pos) == VE_OK)
			errThrow->SetLong8( "offset", pos + inOffset);

		sLONG8 dsize;
		if (fImpl.GetSize( &dsize) == VE_OK)
			errThrow->SetLong8( "size", dsize);

		err = errThrow.GetError();
	}

	return err;
}


sLONG8 VFileDesc::GetPos() const
{
	sLONG8 pos;
	VError err = fImpl.GetPos( &pos);

	xbox_assert( (pos >= 0) || (err != VE_OK));

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( fFile, VE_STREAM_CANNOT_GET_POS, err);
		err = errThrow.GetError();
	}
	
	return (err == VE_OK) ? pos : 0;
}


VError VFileDesc::SetPos( sLONG8 inOffset, bool inFromStart ) const
{
	xbox_assert( (inOffset >= 0) || !inFromStart);

	VError err = fImpl.SetPos(inOffset, inFromStart);

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( fFile, VE_STREAM_CANNOT_SET_POS, err);

		if (inFromStart)
			errThrow->SetLong8( "offset", inOffset);
		else
		{
			sLONG8 pos;
			if (fImpl.GetPos( &pos) == VE_OK)
				errThrow->SetLong8( "offset", pos + inOffset);
		}

		sLONG8 size;
		if (fImpl.GetSize( &size) == VE_OK)
			errThrow->SetLong8( "size", size);

		err = errThrow.GetError();
	}
	
	return err;
}


VError VFileDesc::Flush() const
{
	VError err = fImpl.Flush();

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( fFile, VE_STREAM_CANNOT_FLUSH, err);
		err = errThrow.GetError();
	}
	
	return err;
}


#pragma mark  -
#pragma mark VFile
// --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---

VFile::VFile( const VFile& inSource )
: fCachedVolumeInfo( NULL)
, fFileSystem( RetainRefCountable( inSource.fFileSystem))
{
	fPath = inSource.GetPath();
	xbox_assert( fPath.IsFile());
	fImpl.Init(this);
}


VFile::VFile( const VURL& inUrl )
: fCachedVolumeInfo( NULL)
, fFileSystem( NULL)
{
	inUrl.GetFilePath( fPath);
	xbox_assert( fPath.IsFile());	
	fImpl.Init(this);
}

VFile::VFile( const VString& inFullPath, FilePathStyle inFullPathStyle)
: fCachedVolumeInfo( NULL)
, fFileSystem( NULL)
{
	fPath.FromFullPath( inFullPath, inFullPathStyle);
	xbox_assert( fPath.IsFile());
	fImpl.Init(this);
}

VFile::VFile( const VFilePath& inPath, VFileSystem* inFileSystem)
: fCachedVolumeInfo( NULL)
, fFileSystem( RetainRefCountable( inFileSystem))
{
	fPath.FromFilePath( inPath );
	xbox_assert( fPath.IsFile());
	xbox_assert( (fFileSystem == NULL) || fPath.IsChildOf( fFileSystem->GetRoot()) );
	fImpl.Init(this);
}


VFile::VFile( const VFolder& inFolder, const VString& inRelativePath, FilePathStyle inRelativePathStyle)
: fCachedVolumeInfo( NULL)
, fFileSystem( RetainRefCountable( inFolder.GetFileSystem()))
{
	fPath.FromRelativePath( inFolder.GetPath(), inRelativePath, inRelativePathStyle);
	xbox_assert( fPath.IsFile());
	
	// check if the relative path got us out of our file system
	if ( (fFileSystem != NULL) && !fPath.IsChildOf( fFileSystem->GetRoot()) )
		ReleaseRefCountable( &fFileSystem);

	fImpl.Init(this);
}


VFile::VFile( const VFilePath& inRootPath, const VString& inRelativePath, FilePathStyle inRelativePathStyle)
: fCachedVolumeInfo( NULL)
, fFileSystem( NULL)
{
	fPath.FromRelativePath( inRootPath, inRelativePath, inRelativePathStyle);
	xbox_assert( fPath.IsFile());
	fImpl.Init( this);
}


VFile::VFile( VFileSystemNamespace* fsNameSpace, const VString& inPath)
: fCachedVolumeInfo( NULL)
, fFileSystem( NULL)
{
	if (fsNameSpace == nil)
	{
		fPath.FromFullPath( inPath, FPS_POSIX);
		xbox_assert( fPath.IsFile());
	}
	else
	{
		fsNameSpace->ParsePathWithFileSystem(inPath, fFileSystem, fPath);
		if (!fPath.IsFile())
		{
			fPath.FromFullPath( inPath, FPS_POSIX);
		}
		xbox_assert( fPath.IsFile());
		xbox_assert( (fFileSystem == NULL) || fPath.IsChildOf( fFileSystem->GetRoot()) );
	}
	fImpl.Init(this);
}



#if VERSIONMAC
VFile::VFile( const FSRef& inRef)
: fCachedVolumeInfo( NULL)
, fFileSystem( NULL)
{
	VString fullPath;
	OSErr macErr = XMacFile::FSRefToHFSPath( inRef, fullPath);
	xbox_assert( macErr == noErr);
	fPath.FromFullPath(fullPath);
	xbox_assert( fPath.IsFile());
	fImpl.Init( this);
}
#endif

VFile::~VFile()
{
	ReleaseRefCountable( &fCachedVolumeInfo);
	ReleaseRefCountable( &fFileSystem);
}

bool VFile::Init()
{
	return true;
}

void VFile::DeInit()
{
}

// exchange the content of 2 file
VError VFile::Exchange( const VFile& inExchangeWith) const
{
	
	VError err = fImpl.Exchange( inExchangeWith);

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_EXCHANGE, err);
		errThrow->SetString( "source", GetPath().GetPath());
		errThrow->SetString( "dest",  GetPath().GetPath());
		err = errThrow.GetError();
	}

	return err;
}


// return a pointer to the created file. A null pointer value indicates an error.
VError VFile::Open( const FileAccess inFileAccess, VFileDesc** outFileDesc, FileOpenOptions inOptions) const
{
	xbox_assert( outFileDesc != NULL);
	*outFileDesc = NULL;
	VError err = fImpl.Open( inFileAccess, inOptions, outFileDesc);

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_OPEN, err);
		errThrow->SetLong( "access", inFileAccess);
		errThrow->SetLong( "options", inOptions);
		err = errThrow.GetError();
	}

	return err;
}


VError VFile::Create( FileCreateOptions inOptions) const
{
	VError err = fImpl.Create( inOptions);

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_CREATE, err);
		errThrow->SetLong( "options", inOptions);
		err = errThrow.GetError();
	}

	return err;
}


VError VFile::CopyTo( const VFilePath& inDestination, VFile** outFile, FileCopyOptions inOptions ) const
{
	xbox_assert( outFile == NULL || *outFile != this);	// prevent file->Copy( path, &file);	because of refcounting leak

	if (outFile)
		*outFile = NULL;

	VError err;
	bool needThrow;
	if (GetPath().IsValid() && inDestination.IsValid())
	{
		// tries to keep file system if destination is compatible
		VFileSystem *fs = GetFileSystem();
		if ( (fs != NULL) && !inDestination.IsSameOrChildOf( fs->GetRoot()) )
			fs = NULL;
		err = fImpl.Copy( inDestination, fs, outFile, inOptions);
		needThrow = IS_NATIVE_VERROR( err);
	}
	else
	{
		err = VE_FILE_CANNOT_COPY;
		needThrow = true;
	}

	if (needThrow)
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_COPY, err);
		errThrow->SetString( "destination", inDestination.GetPath());
		errThrow->SetLong( "options", inOptions);
		err = errThrow.GetError();
	}

	return err;
}


VError VFile::CopyTo( const VFile& inDestinationFile, FileCopyOptions inOptions ) const
{
	VError err = fImpl.Copy( inDestinationFile.GetPath(), NULL, NULL, inOptions);

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_COPY, err);
		errThrow->SetString( "destination", inDestinationFile.GetPath().GetPath());
		errThrow->SetLong( "options", inOptions);
		err = errThrow.GetError();
	}

	return err;
}


VError VFile::CopyTo( const VFolder& inDestinationFolder, VFile** outFile, FileCopyOptions inOptions ) const
{
	xbox_assert( outFile == NULL || *outFile != this);	// prevent file->Copy( path, &file);	because of refcounting leak

	if (outFile)
		*outFile = NULL;

	VError err = fImpl.Copy( inDestinationFolder.GetPath(), inDestinationFolder.GetFileSystem(), outFile, inOptions);

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_COPY, err);
		errThrow->SetString( "destination", inDestinationFolder.GetPath().GetPath());
		errThrow->SetLong( "options", inOptions);
		err = errThrow.GetError();
	}

	return err;
}


VError VFile::CopyFrom( const VFile& inSource, FileCopyOptions inOptions ) const
{
	VError err = inSource.fImpl.Copy( GetPath(), NULL, NULL, inOptions);

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( &inSource, VE_FILE_CANNOT_COPY, err);
		errThrow->SetString( "destination", GetPath().GetPath());
		errThrow->SetLong( "options", inOptions);
		err = errThrow.GetError();
	}
	
	return err;
}

// Rename the file with the given name ( foofoo.html ).
VError VFile::Rename( const VString& inName, VFile** outFile ) const
{
	if (outFile)
		*outFile = NULL;

	VError err = fImpl.Rename( inName, outFile );

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_RENAME, err);
		errThrow->SetString( "newname", inName);
		err = errThrow.GetError();
	}
	
	return err;
}


VError VFile::_Move( const VFilePath& inDestination, VFileSystem *inDestinationFileSystem, VFile** outFile, FileCopyOptions inOptions) const
{
	xbox_assert( outFile == NULL || *outFile != this);	// prevent file->Move( path, &file);	because of refcounting leak
	xbox_assert( (inDestinationFileSystem == NULL) || inDestination.IsSameOrChildOf( inDestinationFileSystem->GetRoot()) );

	if (outFile)
		*outFile = NULL;
		
	VError err;
	bool needThrow;
	
	if (inDestination.IsValid())
	{
		err = fImpl.Move( inDestination, inDestinationFileSystem, outFile, inOptions);
		needThrow = IS_NATIVE_VERROR( err);
	}
	else
	{
		err = VE_FILE_CANNOT_MOVE;
		needThrow = true;
	}
		
	if (needThrow)
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_MOVE, err);
		errThrow->SetString( "destination", inDestination.GetPath());
		errThrow->SetLong( "options", inOptions);
		err = errThrow.GetError();
	}

	return err;
}


VError VFile::Move( const VFilePath& inDestination, VFile** outFile, FileCopyOptions inOptions) const
{
	// tries to keep file system if destination is compatible
	VFileSystem *fs = GetFileSystem();
	if ( (fs != NULL) && !inDestination.IsSameOrChildOf( fs->GetRoot()) )
		fs = NULL;
	return _Move( inDestination, fs, outFile, inOptions);
}


VError VFile::Move( const VFile& inDestinationFile, FileCopyOptions inOptions) const
{
	return _Move( inDestinationFile.GetPath(), inDestinationFile.GetFileSystem(), NULL, inOptions);
}


VError VFile::Move( const VFolder& inDestinationFolder, VFile** outFile, FileCopyOptions inOptions) const
{
	return _Move( inDestinationFolder.GetPath(), inDestinationFolder.GetFileSystem(), outFile, inOptions);
}


VError VFile::Delete() const
{
	VError err;
	bool needThrow;

	if (GetPath().IsValid())
	{
		// deleting an non-existing file is not an error
		err = fImpl.Exists() ? fImpl.Delete() : VE_OK;
		needThrow = IS_NATIVE_VERROR( err);
	}
	else
	{
		err = VE_FILE_CANNOT_DELETE;
		needThrow = true;
	}
		
	if (needThrow)
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_DELETE, err);
		err = errThrow.GetError();
	}

	return err;
}

VError VFile::MoveToTrash() const
{
	VError err;
	bool needThrow;

	if (GetPath().IsValid())
	{
		// deleting an non-existing file is not an error
		err = fImpl.Exists() ? fImpl.MoveToTrash() : VE_OK;
		needThrow = IS_NATIVE_VERROR( err);
	}
	else
	{
		err = VE_FILE_CANNOT_DELETE;
		needThrow = true;
	}
		
	if (needThrow)
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_DELETE, err);
		err = errThrow.GetError();
	}

	return err;
}



// return parent directory.
VFolder *VFile::RetainParentFolder( bool inSandBoxed) const
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

// return full file name ( foo.html ).
void VFile::GetName(VString& outName) const
{
	return fPath.GetName( outName);
}

// return only the extension ( html ).
void VFile::GetExtension(VString& outExt) const
{
	return fPath.GetExtension( outExt);
}


bool VFile::MatchExtension( const VString& inExtension) const
{
	return fPath.MatchExtension( inExtension, false);	// WARNING: diacritical or not? should ask the file system
}


// return file name ( foo ).
void VFile::GetNameWithoutExtension(VString& outName) const
{
	fPath.GetFileName( outName, false);
}


const VFilePath& VFile::GetPath() const
{
	return fPath;
}


VFilePath& VFile::GetPath( VFilePath& outPath) const
{
	outPath = fPath;
	return outPath;
}


void VFile::GetPath( VString& outPath, FilePathStyle inPathStyle, bool relativeToFileSystem ) const
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
					VError err = fImpl.GetPosixPath( outPath);
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

// test if the file allready exist.
bool VFile::Exists() const
{
	return fPath.IsFile() && fImpl.Exists();
}


#if VERSIONWIN
const XFileImpl* VFile::WIN_GetImpl() const
{
	return &fImpl;
}

VError VFile::WIN_GetAttributes( DWORD *outAttrb ) const
{
	VError err = fImpl.GetAttributes( outAttrb );

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_GET_ATTRIBUTES, err);
		err = errThrow.GetError();
	}

	if (err != VE_OK)
		*outAttrb = 0;
	
	return err;
}


VError VFile::WIN_SetAttributes( DWORD inAttrb) const
{
	VError err = fImpl.SetAttributes( inAttrb);

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_SET_ATTRIBUTES, err);
		errThrow->SetLong( "options", inAttrb);
		err = errThrow.GetError();
	}
	
	return err;
}

#endif


VError VFile::GetSize( sLONG8 *outSize) const
{
	VError err = fImpl.GetSize( outSize);

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_GET_ATTRIBUTES, err);
		err = errThrow.GetError();
	}
	
	if (err != VE_OK)
		*outSize = 0;
	
	return err;
}


bool VFile::HasResourceFork() const
{
	return fImpl.HasResourceFork();
}


VError VFile::GetResourceForkSize( sLONG8 *outSize ) const
{
	VError err = fImpl.GetResourceForkSize( outSize);

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_GET_ATTRIBUTES, err);
		err = errThrow.GetError();
	}
	
	if (err != VE_OK)
		*outSize = 0;
	
	return err;
}

// GetFileAttributes && FSGetCatalogInfo.
VError VFile::GetTimeAttributes( VTime* outLastModification, VTime* outCreationTime, VTime* outLastAccess ) const
{
	VError err = fImpl.GetTimeAttributes( outLastModification, outCreationTime, outLastAccess );

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_GET_ATTRIBUTES, err);
		err = errThrow.GetError();
	}
	
	if (err != VE_OK)
	{
		if ( outLastModification )
			outLastModification->Clear();
		if ( outCreationTime )
			outCreationTime->Clear();
		if ( outLastAccess )
			outLastAccess->Clear();
	}
	
	return err;
}

VError VFile::GetFileAttributes( EFileAttributes &outFileAttributes ) const
{
	VError err = fImpl.GetFileAttributes( outFileAttributes );

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_GET_ATTRIBUTES, err);
		err = errThrow.GetError();
	}

	return err;
}

VError VFile::SetTimeAttributes( const VTime *inLastModification, const VTime *inCreationTime, const VTime *inLastAccess ) const
{
	VError err = fImpl.SetTimeAttributes( inLastModification, inCreationTime, inLastAccess );

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_GET_ATTRIBUTES, err);
		err = errThrow.GetError();
	}

	return err;
}

VError VFile::SetFileAttributes( EFileAttributes inFileAttributes ) const
{
	VError err = fImpl.SetFileAttributes( inFileAttributes );

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_GET_ATTRIBUTES, err);
		err = errThrow.GetError();
	}

	return err;
}


// return DiskFreeSpace.
VError VFile::GetVolumeFreeSpace(sLONG8 *outFreeBytes, sLONG8 * outTotalBytes, bool inWithQuotas, uLONG inAccuracyMilliSeconds ) const
{
	VError err = VE_OK;
	sLONG8 resultFreeSpace;
	sLONG8 resultTotalSpace;

	if (inAccuracyMilliSeconds != 0)
	{
		uLONG t = VSystem::GetCurrentTime();

		// capture volume info
		VVolumeInfo *info = VInterlocked::ExchangePtr<VVolumeInfo>( &fCachedVolumeInfo, NULL);

		// check if cache is still valid
		if (info != NULL)
		{
			uLONG delta = (t >= info->fTime) ? (t - info->fTime) : (info->fTime - t);
			if ( (delta > inAccuracyMilliSeconds) || (info->fWithQuotas != inWithQuotas) )
				ReleaseRefCountable( &info);
		}
		
		if (info == NULL)
		{
			err = fImpl.GetVolumeFreeSpace( &resultFreeSpace, &resultTotalSpace, inWithQuotas);
			if (err == VE_OK)
			{
				// build new cache info
				info = new VVolumeInfo( t, inWithQuotas, resultFreeSpace, resultTotalSpace);
			}
		}
		else
		{
			resultFreeSpace = info->fFreeSpace;
			resultTotalSpace = info->fTotalSpace;
		}

		// install new cache info
		VVolumeInfo *oldInfo = VInterlocked::ExchangePtr<VVolumeInfo>( &fCachedVolumeInfo, info);
		ReleaseRefCountable( &oldInfo);
	}
	else
	{
		err = fImpl.GetVolumeFreeSpace( &resultFreeSpace, &resultTotalSpace, inWithQuotas);
	}

	// in case of error, returns 0
	if (err != VE_OK)
	{
		resultFreeSpace = 0;
		resultTotalSpace = 0;
	}
	
	if (outFreeBytes != NULL)
		*outFreeBytes = resultFreeSpace;

	if (outTotalBytes != NULL)
		*outTotalBytes = resultTotalSpace;

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_GET_ATTRIBUTES, err);
		err = errThrow.GetError();
	}
	
	return err;
}

#if !VERSION_LINUX
// reveal in finder
bool VFile::RevealInFinder() const
{
	VError err = fImpl.RevealInFinder();

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_REVEAL, err);
		err = errThrow.GetError();
	}
	
	return err == VE_OK;
}
#endif


VError VFile::ResolveAlias( VFilePath& outTargetPath ) const
{
	VError err = fImpl.ResolveAlias( outTargetPath );

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_RESOLVE_ALIAS, err);
		err = errThrow.GetError();
	}
	
	if (err != VE_OK)
		outTargetPath.Clear();
	
	return err;
}


/*
	static
*/
VError VFile::ResolveAliasFile( VFile **ioFile, bool inDeleteInvalidAlias)
{
	VError err = VE_OK;
	if (*ioFile != NULL)
	{
		VFile *aliasFile = NULL;

		#if VERSIONMAC
		// on mac the alias file we are looking for is the same file
		if ((*ioFile)->IsAliasFile())
			aliasFile = RetainRefCountable( *ioFile);
		#elif VERSIONWIN
		// on windows, we must look at another file with a .lnk extension
		if (!(*ioFile)->Exists())
		{
			VString path = (*ioFile)->GetPath().GetPath();
			path += ".lnk";
			aliasFile = new VFile( path);
			if ( (aliasFile != NULL) && !aliasFile->Exists() )
				ReleaseRefCountable( &aliasFile);
		}
		else
		{
			if ((*ioFile)->IsAliasFile())
				aliasFile = RetainRefCountable(*ioFile);
		}
		#endif

		if (aliasFile != NULL)
		{
			VFilePath resolvedPath;
			err = aliasFile->fImpl.ResolveAlias( resolvedPath);	// no throw
			if (err == VE_OK)
			{
				if (resolvedPath.IsFile())
				{
					(*ioFile)->Release();
					*ioFile = new VFile( resolvedPath);
				}
				else if (inDeleteInvalidAlias)
				{
					err = aliasFile->Delete();
				}
				else
				{
					StThrowFileError errThrow( aliasFile, VE_FILE_CANNOT_RESOLVE_ALIAS_TO_FILE, VNE_OK);
					err = errThrow.GetError();
				}
			}
			else if (inDeleteInvalidAlias)
			{
				err = aliasFile->Delete();
			}
			else
			{
				if (IS_NATIVE_VERROR( err))
				{
					StThrowFileError errThrow( aliasFile, VE_FILE_CANNOT_RESOLVE_ALIAS, err);
					err = errThrow.GetError();
				}
			}
		}
		ReleaseRefCountable( &aliasFile);
	}
	return err;
}


/*
	static
*/
VFile *VFile::CreateAndResolveAlias( const VFolder& inFolder, const VString& inRelativPath, FilePathStyle inRelativPathStyle)
{
	VFile *file = new VFile( inFolder, inRelativPath, inRelativPathStyle);
	VError error = ResolveAliasFile( &file, true);
	if (error != VE_OK)
		ReleaseRefCountable( &file);
	
	return file;
}


VError VFile::CreateAlias( const VFilePath &inTargetPath, const VFilePath *inRelativeAliasSource) const
{
	VError err = fImpl.CreateAlias( inTargetPath, inRelativeAliasSource);

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_CREATE_ALIAS, err);
		err = errThrow.GetError();
	}
	
	return err;
}


bool VFile::IsAliasFile() const
{
	return fImpl.IsAliasFile();
}


bool VFile::IsSameFile( const VFile *inFile) const
{
	// TBD: should check the filesystem if case sensitivity is to be taken into account.
	if (inFile != NULL)
		return inFile->GetPath().GetPath().EqualTo( GetPath().GetPath(), false) != 0;
	else
		return false;
}


bool VFile::ConformsTo( const VFileKind& inKind) const
{
	bool ok=false;
	VError err = fImpl.ConformsTo( inKind, &ok);

	return (err == VE_OK) ? ok : false;
}


bool VFile::ConformsTo( const VString& inKindID) const
{
	bool ok=false;

	VFileKind *kind = VFileKind::RetainFileKind( inKindID);
	if (kind != NULL)
	{
		VError err = fImpl.ConformsTo( *kind, &ok);
		if (err != VE_OK)
			ok = false;
		kind->Release();
	}
	else
	{
		ok = false;
	}
	return ok;
}


#if VERSIONMAC
const XFileImpl* VFile::MAC_GetImpl() const
{
	return &fImpl;
}


VError VFile::MAC_GetKind (OsType *outKind) const
{
	VError err = fImpl.GetKind( outKind);

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_GET_ATTRIBUTES, err);
		err = errThrow.GetError();
	}
	
	return err;
}

VError VFile::MAC_SetKind ( OsType inKind ) const
{
	VError err = fImpl.SetKind(inKind);

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_SET_ATTRIBUTES, err);
		err = errThrow.GetError();
	}
	
	return err;
}

VError VFile::MAC_GetCreator ( OsType *outCreator ) const
{
	VError err = fImpl.GetCreator( outCreator);

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_GET_ATTRIBUTES, err);
		err = errThrow.GetError();
	}
	
	return err;
}

VError VFile::MAC_SetCreator ( OsType inCreator ) const
{
	VError err = fImpl.SetCreator(inCreator);
	return err;
}


bool VFile::MAC_GetFSRef( FSRef *outRef) const
{
	VString fullPath;
	fPath.GetPath(fullPath);
	return XMacFile::HFSPathToFSRef( fullPath, outRef) == noErr;
}


#if !__LP64__
bool VFile::MAC_GetFSSpec( FSSpec *outSpec) const
{
	FSRef fsref;
	return MAC_GetFSRef( &fsref) && (::FSGetCatalogInfo( &fsref, kFSCatInfoNone, NULL, NULL, outSpec, NULL) == noErr);
}
#endif


void VFile::MAC_SetExtensionVisible( bool inShow )
{
	fImpl.SetExtensionVisible(inShow);
}

void VFile::MAC_SetFinderComment( VString& inString )
{
	fImpl.SetFinderComment(inString);
}

void VFile::MAC_GetFinderComment( VString& outString )
{
	fImpl.GetFinderComment(outString);
}

VError VFile::MAC_MakeWritableByAll()
{
	VError err = fImpl.MakeWritableByAll();

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_SET_ATTRIBUTES, err);
		err = errThrow.GetError();
	}
	
	return err;
}

bool VFile::MAC_IsWritableByAll()
{
	return fImpl.IsWritableByAll();
}

VError VFile::MAC_SetPermissions( uWORD inMode )
{
	VError err = fImpl.SetPermissions(inMode);

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_SET_ATTRIBUTES, err);
		err = errThrow.GetError();
	}
	
	return err;
}

VError VFile::MAC_GetPermissions( uWORD *outMode )
{
	VError err = fImpl.GetPermissions(outMode);

	if (IS_NATIVE_VERROR( err))
	{
		StThrowFileError errThrow( this, VE_FILE_CANNOT_GET_ATTRIBUTES, err);
		err = errThrow.GetError();
	}
	
	return err;
}

#endif


VError VFile::GetURL(VString& outURL, bool inEncoded)
{
	VFilePath path;
	GetPath(path);
	VURL url(path);
	url.GetAbsoluteURL(outURL, inEncoded);
	return VE_OK;
}


VError VFile::GetRelativeURL(VFolder* inBaseFolder, VString& outURL, bool inEncoded)
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

/*static*/
VFile *	VFile::CreateTemporaryFile()
{
	VFile			*tempFile = NULL;
	VString			tempName;
	XBOX::VFilePath	folderPath;

	VFolder			*folder = VFolder::RetainSystemFolder(eFK_Temporary, true);// Should not fail. No NULL test follows
	folder->GetPath(folderPath);
	ReleaseRefCountable(&folder);

	/*	The temp name is built from a UUID. So, we should not have a "duplicate file" error when creating it.
	But, creation may still fail for some other reasons linked to the file manager.
	*/
	VUUID	uid(true);

	uid.GetString(tempName);
	tempName.Insert(L"xbx_", 1);

	XBOX::VFilePath		tempPath(folderPath, tempName);
	tempFile = new VFile(tempPath);
	if(tempFile->Create(FCR_Default) != VE_OK)
	{
		delete tempFile;
		tempFile = NULL;
	}

	return tempFile;
}


VError VFile::GetContent( VMemoryBuffer<>& outContent) const
{
	VFileDesc *desc = NULL;
	VError err = Open( FA_READ, &desc);
	if (err == VE_OK)
	{
		sLONG8 size = desc->GetSize();
		if ( (size > kMAX_VSize) || !outContent.SetSize( (VSize) size) )
		{
			StThrowFileError errThrow( this, err, VNE_OK);
			err = errThrow.GetError();
		}
		
		if ( (err == VE_OK) && (outContent.GetDataSize() > 0) )
			err = desc->GetData( outContent.GetDataPtr(), outContent.GetDataSize(), 0, NULL);
	}
	delete desc;
	
	return err;
}


VError VFile::SetContent( const void *inDataPtr, size_t inDataSize) const
{
	VFileDesc *desc = NULL;
	VError err = Open( FA_READ_WRITE, &desc, FO_CreateIfNotFound | FO_Overwrite);
	if (err == VE_OK)
	{
		err = desc->SetSize( inDataSize);
		if (err == VE_OK)
			err = desc->PutData( inDataPtr, inDataSize, 0);
	}
	delete desc;
	
	return err;
}


VError VFile::GetContentAsString( VString& outContent, CharSet inDefaultSet, ECarriageReturnMode inCRMode) const
{
	StErrorContextInstaller errorContext( true);	// catch VString errors
	VMemoryBuffer<> buffer;
	VError err = GetContent( buffer);
	if (err == VE_OK)
	{
		outContent.FromBlockWithOptionalBOM( buffer.GetDataPtr(), buffer.GetDataSize(), inDefaultSet);
		outContent.ConvertCarriageReturns( inCRMode);
	}
	else
	{
		outContent.Clear();
	}
	return errorContext.GetLastError();
}


VError VFile::SetContentAsString( const VString& inContent, CharSet inCharSet, ECarriageReturnMode inCRMode) const
{
	StErrorContextInstaller errorContext( true);	// catch VString errors

	VString s( inContent);
	s.ConvertCarriageReturns( inCRMode);
	
	VFromUnicodeConverter *converter = VTextConverters::Get()->RetainFromUnicodeConverter( inCharSet);

	if ( (converter == NULL) || !converter->IsValid())
		vThrowError( VE_STREAM_NO_TEXT_CONVERTER);
	else
	{
		void *data = NULL;
		VSize dataSize = 0; 
		bool ok = converter->ConvertRealloc( s.GetCPointer(), s.GetLength(), data, dataSize, 0 );
	
		if (ok)
		{
			VFileDesc *desc = NULL;
			VError err = Open( FA_READ_WRITE, &desc, FO_CreateIfNotFound | FO_Overwrite);
			if (err == VE_OK)
			{
				// Write BOM
				size_t bomSize = 0;
				const char *bom = VTextConverters::GetBOMForCharSet( inCharSet, &bomSize);
				err = desc->SetSize( dataSize + bomSize);
				if (err == VE_OK)
					err = desc->PutDataAtPos( bom, bomSize);
				if (err == VE_OK)
					err = desc->PutDataAtPos( data, dataSize);
			}
			delete desc;
		}

		if (data != NULL)
			vFree( data);
	}

	XBOX::ReleaseRefCountable( &converter);

	return errorContext.GetLastError();
}


// -------------------------------------------------------

#pragma mark -
#pragma mark VFileKindManager

VFileKindManager*			VFileKindManager::sManager = NULL;

VFileKindManager::VFileKindManager()
{
	fStringAll.FromCString("All");
	fStringAllReadableDocuments.FromCString("All readable documents");

}

bool VFileKindManager::Init()
{
	xbox_assert( sManager == NULL);
	sManager = new VFileKindManager;
	return true;
}


void VFileKindManager::DeInit()
{
	ReleaseRefCountable( &sManager);
}


VFileKind *VFileKindManager::RetainFileKind( const VString& inID)
{
	VRefPtr<VFileKind> ptr_kind;

	MapOfVFileKind::const_iterator i = fMap.find( inID);
	if (i != fMap.end())
	{
		ptr_kind = i->second;
	}
	else
	{
		// see if that's a public kind not already registered
		ptr_kind.Adopt( XFileImpl::CreatePublicFileKind( inID));
		if (ptr_kind != NULL)
		{
			fMap[inID] = ptr_kind;
		}
	}

	return ptr_kind.Forget();
}


void VFileKindManager::RegisterPrivateFileKind( const VString& inID, const VString& inDescription, const VectorOfFileExtension& inExtensions, const VectorOfFileKind &inParentIDs)
{
	VectorOfFileKind emptyVector;
	RegisterPrivateFileKind( inID, emptyVector, inDescription, inExtensions, inParentIDs);
}

void VFileKindManager::RegisterPrivateFileKind( const VString& inID, const VectorOfFileKind& inOsKind, const VString& inDescription, const VectorOfFileExtension& inExtensions, const VectorOfFileKind &inParentIDs)
{
#if VERSIONWIN || VERSION_LINUX
	VString desc=inDescription;
	if(desc.IsEmpty())
	{
		XBOX::VString ext;
		for(VectorOfFileExtension::const_iterator it=inExtensions.begin();it!=inExtensions.end();it++)
		{
			ext=(*it);
			if(!(*it).BeginsWith(".",true))
				ext="."+(*it);

#if !VERSION_LINUX
			desc=XWinFile::GetFileTypeDescriptionFromExtension(ext);
#else
            desc=VString("Postponed Linux Implementation !");
            // Postponed Linux Implementation !
#endif
			if(!desc.IsEmpty())
				break;
		}
	}
	VRefPtr<VFileKind> ptr_kind( VFileKind::Create( inID, inOsKind, desc, inExtensions,inParentIDs,  false), false);
#else
	VRefPtr<VFileKind> ptr_kind( VFileKind::Create( inID, inOsKind, inDescription, inExtensions, inParentIDs, false), false);
#endif	
	
	fMap[inID] = ptr_kind;
}


void VFileKindManager::RegisterLocalizedString(long inWhich,const VString& inString)
{
	if (inString.GetLength() > 0)
	{
		if (inWhich == VFileKindManager::STRING_ALL)
			fStringAll = inString;
		else if (inWhich == VFileKindManager::STRING_ALL_READABLE_DOCUMENTS)
			fStringAllReadableDocuments = inString;
	}
}

void VFileKindManager::GetLocalizedString(long inWhich,VString &outString)
{
	if (inWhich == VFileKindManager::STRING_ALL)
		outString = fStringAll;
	else if (inWhich == VFileKindManager::STRING_ALL_READABLE_DOCUMENTS)
		outString = fStringAllReadableDocuments;
}

VFileKind* VFileKindManager::RetainFirstFileKindMatchingWithExtension( const VString& inExtension )
{
	VFileKind *result = NULL;
	for ( MapOfVFileKind::const_iterator iter = fMap.begin(); iter != fMap.end(); ++iter )
	{
		if ( iter->second->MatchExtension(inExtension) )
		{
			result = iter->second;
			result->Retain();
			break;
		}
	}

	if (result == NULL)
	{
		result = XFileImpl::CreatePublicFileKindFromExtension( inExtension);
		if (result != NULL)
		{
			if (testAssert( fMap.find(result->GetID()) == fMap.end() ))
			{
				fMap[result->GetID()] = VRefPtr<VFileKind>(result);		// sc 27/09/2011 optimization
			}
		}
	}

	return result;
}

VFileKind* VFileKindManager::RetainFirstFileKindMatchingWithOsKind( const VString& inOsKind )
{
	VFileKind *result = NULL;
	for ( MapOfVFileKind::const_iterator iter = fMap.begin(); iter != fMap.end(); ++iter )
	{
		if ( iter->second->MatchOsKind( inOsKind ) )
		{
			result = iter->second;
			result->Retain();
			break;
		}
	}

	if (result == NULL)
		result = XFileImpl::CreatePublicFileKindFromOsKind( inOsKind );

	return result;
}

// -------------------------------------------------------

#pragma mark -
#pragma mark VFileKind


VFileKind *VFileKind::Create( const VString& inID, const VectorOfFileKind &inOsKind, const VString& inDescription, const VectorOfFileExtension& inExtensions, const VectorOfFileKind &inParentIDs, bool inIsPublic)
{
	return new VFileKind( inID, inOsKind, inDescription, inExtensions, inParentIDs, inIsPublic);
}


bool VFileKind::MatchExtension( const VString& inExtension) const
{
	bool ok = false;
	for( VectorOfFileExtension::const_iterator i = fFileExtensions.begin() ; !ok && (i != fFileExtensions.end()) ; ++i)
		ok = i->EqualToString( inExtension, false);
	return ok;
}

bool VFileKind::MatchOsKind( const VString& inOsKind) const
{
	bool ok = false;
	for( VectorOfFileKind::const_iterator i = fOsKind.begin() ; !ok && (i != fOsKind.end()) ; ++i)
		ok = i->EqualToString( inOsKind, true );
	return ok;
}

bool VFileKind::GetPreferredExtension( VString& outExtension) const
{
	if (fFileExtensions.empty())
		outExtension.Clear();
	else
		outExtension = fFileExtensions.front();

	return !fFileExtensions.empty();
}

bool VFileKind::GetPreferredOsKind( VString& outOsKind) const
{
	if (fOsKind.empty())
		outOsKind.Clear();
	else
		outOsKind = fOsKind.front();

	return !fOsKind.empty();
}


bool VFileKind::GetParentIDs( VectorOfFileKind& outIDs) const
{
	outIDs.clear();
	outIDs.insert( outIDs.begin(), fParentIDs.begin(), fParentIDs.end());
	return !outIDs.empty();

}


bool VFileKind::ConformsTo( const VFileKind& inKind) const
{
	return XFileImpl::IsFileKindConformsTo( *this, inKind);
}


bool VFileKind::ConformsTo( const VString& inID) const
{
	bool result = false;

	VFileKind *otherKind = VFileKindManager::Get()->RetainFileKind( inID);
	if (otherKind != NULL)
	{
		result = XFileImpl::IsFileKindConformsTo( *this, *otherKind);
		otherKind->Release();
	}
	return result;
}


#if VERSIONWIN
bool VFileKind::WIN_BuildExtensionsStringForGetDialog( VString& outExtensionsString) const
{
	// Build the file filter string to be used by Windows in GetSaveFileName api

	bool matchAll = false;
	if (fFileExtensions.empty())
	{
		outExtensionsString = "*.*";
		matchAll = true;
	}
	else
	{
		for( VectorOfFileExtension::const_iterator i = fFileExtensions.begin() ; i != fFileExtensions.end() ; ++i)
		{
			if (!i->IsEmpty())
			{
				if (!outExtensionsString.IsEmpty())
					outExtensionsString += ";";
				outExtensionsString += "*.";
				outExtensionsString += *i;
			}
			else if (fFileExtensions.size() == 1)
			{
				outExtensionsString = "*.*";
				matchAll = true;
			}
		}
	}
	return matchAll;
}

void VFileKind::WIN_BuildFilterStringForGetDialog( VString& outFilterString) const
{
	// Build the file filter string to be used by Windows in GetSaveFileName api

	VStr255 kindstr;
	WIN_BuildExtensionsStringForGetDialog( kindstr);

	outFilterString = GetDescription();
	outFilterString += " (";
	kindstr.ToLowerCase();
	outFilterString += kindstr;
	outFilterString += ")";
	outFilterString += (UniChar)0x0000;
	outFilterString += kindstr;
	outFilterString += (UniChar)0x0000;
}
#endif


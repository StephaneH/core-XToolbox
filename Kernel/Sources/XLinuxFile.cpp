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
#include "VAssert.h"
#include "VFile.h"
#include "VErrorContext.h"
#include "VTime.h"
#include "VFileSystem.h"
#include <unistd.h>

const sLONG	kMAX_PATH_SIZE=PATH_MAX;


namespace // anonymous
{

	static inline VError  SetPosWL(int inFD, sLONG8 inOffset, XLinuxFileDesc::Whence inWhence, sLONG8* outLastPos)
	{
		//Warning : If we set the cursor past the end of the file and then write data, we may create
		//          hole in the file... This is ok but it might be confusing  : the size reported by
		//          ls may be greater than the sum of allocated blocks. Bytes in holes are read as 0.

		int whence;
		switch(inWhence)
		{
			case XLinuxFileDesc::SET : whence = SEEK_SET; break;
			case XLinuxFileDesc::CUR : whence = SEEK_CUR; break;
			case XLinuxFileDesc::END : whence = SEEK_END; break;
		}

		off_t res = ::lseek(inFD, inOffset, whence);
		if (res < 0)
			return MAKE_NATIVE_VERROR(errno);

		if (outLastPos)
			*outLastPos = res;
		return VE_OK;
	}


	static inline VError SetPosFromStartWL(int inFD, sLONG8 inOffset, bool inFromStart)
	{
		return SetPosWL(inFD, inOffset, (inFromStart ? XLinuxFileDesc::SET : XLinuxFileDesc::CUR), NULL);
	}


	static inline VError GetPosWL(int inFD, sLONG8* outPos)
	{
		return SetPosWL(inFD, 0, XLinuxFileDesc::CUR, outPos);
	}


} // anonymous namespace




////////////////////////////////////////////////////////////////////////////////
//
// XLinuxFileDesc
//
////////////////////////////////////////////////////////////////////////////////

XLinuxFileDesc::XLinuxFileDesc(FileDescSystemRef inRef) : fFd(inRef) { }


//virtual
XLinuxFileDesc::~XLinuxFileDesc()
{
	//It would be great if we could check here that we didn't loose our lock, but I can see no easy way to do that.
	//(On Linux, locks set on file are released as soon as the first process's fd on this file is closed, which means
	//if two threads are locking the same file, the first that close it cancels the orther's lock !)

	if (IsValid())
	{
		VTaskLock locker(&fMutex);
		::close(fFd);	//According to win implementation
	}
}


bool XLinuxFileDesc::IsValid() const
{
	return (fFd >= 0);
}


/*
VError XLinuxFileDesc::CreateFile(const char* inPath, FileCreateOptions inCreateOpts)
{
	////////////////////////////////////////////////////////////////////////////////
	//
	// I'm pretty sure it doesn't behave in the same way than the windows platform.
	// (the share and overwrite behavior might be different)
	//
	////////////////////////////////////////////////////////////////////////////////

	//utiliser open helper

	int openFlags=0;
	
	if(inCreateOpts & FCR_Overwrite)
		openFlags|=O_RDONLY|O_CREAT;					//CREATE_ALWAYS ?
	else
		openFlags|=O_RDONLY|O_CREAT|O_EXCL;				//CREATE_NEW
	
	mode_t modeFlags=644;	//jmo - todo : virer ca !
	int fd=open(const_cast<char*>(inPath), openFlags, modeFlags);
	
	if(fd<0)
		return MAKE_NATIVE_VERROR(errno);
	
	close(fd);
	
	return VE_OK;
}
*/


VError XLinuxFileDesc::GetSize(sLONG8 *outSize) const
{
	//There are two easy ways to implement this function : lseek and fstat.
	//Both are easy and should work. I choose lseek in an arbitrary way.

	//Utiliser stat helper !

	if(!IsValid())
		return VE_INVALID_PARAMETER;

	sLONG8 pos=0;
	sLONG8 end=0;

	VTaskLock locker(&fMutex);
	VError verr = GetPosWL(fFd, &pos);
	
	if(verr==VE_OK)
		verr = SetPosWL(fFd, 0, END, NULL);
	else
		return MAKE_NATIVE_VERROR(errno);

	if(verr==VE_OK)
		verr=GetPosWL(fFd, &end);
	
	if(verr==VE_OK)
	{
		*outSize = end;
		verr = SetPosWL(fFd, pos, SET, NULL);
	}

	xbox_assert(verr == VE_OK); // We should always be able to restore seek ptr

    return verr;
}


VError XLinuxFileDesc::SetSize(sLONG8 inSize) const
{
	if(!IsValid())
		return VE_INVALID_PARAMETER;

	//Shall fail if fFd is not in write-mode. Although we could check that, I prefer to report the system error.
	ResizeHelper rszHlp;

	VTaskLock locker(&fMutex);
	return rszHlp.Resize(fFd, inSize);
}


VError XLinuxFileDesc::GetData(void *outData, VSize &ioCount, sLONG8 inOffset, bool inFromStart) const
{
	if(!IsValid())
		return VE_INVALID_PARAMETER;

	VTaskLock locker(&fMutex);
	VError verr = SetPosFromStartWL(fFd, inOffset, inFromStart);

	if (verr!=VE_OK)
	{
		ioCount=0;	//According to win implementation
		return verr;
	}

	ssize_t n=0;
	ssize_t count=0;
	ssize_t bytes=ioCount;
	
	do
	{
		if((n=read(fFd, (char*)(outData)+count, bytes-count))<0)
		{
			if(errno==EINTR)
				continue;
			else
			{
				verr=MAKE_NATIVE_VERROR(errno);
				break;
			}
		}

		count+=n;
	}
	while(n!=0);

    //Callers expect impl. to fail if it can not fill the data buffer...
    //As a result callers fail if impl. succeed ;) So let's simulate an error !
    if(bytes>count)
        verr=VE_STREAM_EOF;

	ioCount=count;

   	return verr;
}


VError XLinuxFileDesc::PutData(const void *inData, VSize& ioCount, sLONG8 inOffset, bool inFromStart) const
{
	if (!IsValid())
		return VE_INVALID_PARAMETER;

	VTaskLock locker(&fMutex);
	VError verr = SetPosFromStartWL(fFd, inOffset, inFromStart);

	if (verr!=VE_OK)
	{
		ioCount=0;	//According to win implementation
		return verr;
	}

	ssize_t n=0;
	ssize_t count=0;
	ssize_t bytes=ioCount;
	
	do
	{
		if((n=write(fFd, (char*)(inData)+count, bytes-count))<0)
		{
			if(errno==EINTR)
				continue;
			else
			{
				verr=MAKE_NATIVE_VERROR(errno);
				break;
			}
		}

		count+=n;
	}
	while(bytes-count>0);

	ioCount=count;

	return verr;
}


VError XLinuxFileDesc::GetPos(sLONG8* outPos) const
{
	if (!IsValid())
		return VE_INVALID_PARAMETER;
	VTaskLock locker(&fMutex);
	return GetPosWL(fFd, outPos);
}


VError XLinuxFileDesc::SetPos(sLONG8 inOffset, bool inFromStart) const
{
	if (!IsValid())
		return VE_INVALID_PARAMETER;
	VTaskLock locker(&fMutex);
	return SetPosFromStartWL(fFd, inOffset, inFromStart);
}


VError XLinuxFileDesc::Flush() const
{
	if(!IsValid())
		return VE_INVALID_PARAMETER;

	VTaskLock locker(&fMutex);
	return (::fsync(fFd) == 0) ? VE_OK : MAKE_NATIVE_VERROR(errno);
}


FileDescSystemRef XLinuxFileDesc::GetSystemRef() const
{
    return fFd;
}



////////////////////////////////////////////////////////////////////////////////
//
// XLinuxFile
//
////////////////////////////////////////////////////////////////////////////////

XLinuxFile::XLinuxFile():fOwner(NULL)
{
	//nothing to do !
}


//virtual
XLinuxFile::~XLinuxFile()
{
	//nothing to do !
}


VError XLinuxFile::Init(VFile *inOwner)
{
	if(inOwner==NULL)
		return VE_INVALID_PARAMETER;

    fOwner=inOwner;

	VFilePath filePath=fOwner->GetPath();

	VError verr=fPath.Init(filePath);

	return verr;
}


VError XLinuxFile::Open(const FileAccess inFileAccess, FileOpenOptions inOptions, VFileDesc** outFileDesc) const
{
	if(outFileDesc==NULL)
		return VE_INVALID_PARAMETER;

	OpenHelper opnHlp;
	FileDescSystemRef fd=-1;
	VError verr=opnHlp.SetFileAccess(inFileAccess).SetFileOpenOpts(inOptions).Open(fPath, &fd);
	
	if(verr!=VE_OK)
		return verr;

	*outFileDesc=new VFileDesc(fOwner, fd, inFileAccess);

	if(outFileDesc==NULL)
		return VE_MEMORY_FULL;

	return verr;
}


VError XLinuxFile::Create(FileCreateOptions inOptions) const
{
	CreateHelper crtHlp;
	return crtHlp.SetCreateOpts(inOptions).Create(fPath);
}


VError XLinuxFile::Copy(const VFilePath& inDestination, VFileSystem *inDestinationFileSystem, VFile** outFile, FileCopyOptions inOptions ) const
{
	VFilePath dstPath(inDestination);
	
	if(dstPath.IsFolder())
	{
		VStr255 name;					//jmo - todo : NAME_MAX ?
		fOwner->GetName(name);
		dstPath.SetFileName(name);
	}

 	PathBuffer pathBuffer;
 	pathBuffer.Init(dstPath);

	CopyHelper cpHlp;
	VError err = cpHlp.Copy(fPath, pathBuffer);
	
	if (outFile != NULL)
	{
		*outFile = (err == VE_OK) ? new VFile( dstPath, inDestinationFileSystem) : NULL;
	}

	return err;
}


VError XLinuxFile::Move(const VFilePath& inDestinationPath, VFileSystem *inDestinationFileSystem, VFile** outFile, FileCopyOptions inOptions) const
{
	VFilePath dstPath(inDestinationPath);
	
	if (dstPath.IsFolder())
	{
		VStr255 name;					//jmo - todo : NAME_MAX ?
		fOwner->GetName(name);
		dstPath.SetFileName(name);
	}

	PathBuffer pathBuffer;
	pathBuffer.Init(dstPath);

	//First we try to rename the file...
	VError verr;
	{
		RenameHelper rnmHlp;
		verr=rnmHlp.Rename(fPath, pathBuffer);
	}

	//If Rename() fails because src and dst are not on the same fs, we try a Copy()
	if(verr!=VE_OK && IS_NATIVE_VERROR(verr) && NATIVE_ERRCODE_FROM_VERROR(verr)==EXDEV)
	{
		CopyHelper cpHlp;
		verr = cpHlp.Copy(fPath, pathBuffer);
		
		// it's a move not a copy, so one must delete the source after a sucessful copy
		if (verr == VE_OK)
		{
			int res=unlink(fPath.GetPath());
			verr = (res==0) ? VE_OK :  MAKE_NATIVE_VERROR(errno);
		}
	}

	if (outFile != NULL)
	{
		*outFile = (verr == VE_OK) ? new VFile( dstPath, inDestinationFileSystem) : NULL;
	}

	return verr;
}


VError XLinuxFile::Rename(const VString& inName, VFile** outFile) const
{
	//jmo - todo : we should check that inName < NAME_MAX or use PathBuffer

	VFilePath newPath(fOwner->GetPath());
	newPath.SetFileName(inName);

	PathBuffer dstPath;
	dstPath.Init(newPath);
	
	RenameHelper rnmHlp;
	VError verr=rnmHlp.Rename(fPath, dstPath);

	if (outFile != NULL)
	{
		*outFile = (verr == VE_OK) ? new VFile( newPath, fOwner->GetFileSystem()) : NULL;
	}

	return verr;
}


VError XLinuxFile::Delete() const
{
	int res=unlink(fPath.GetPath());

	return (res==0) ? VE_OK :  MAKE_NATIVE_VERROR(errno);
}


VError XLinuxFile::MoveToTrash() const
{
	// todo: should move to user trash folder ~/.local/share/Trash
	return Delete();
}


bool XLinuxFile::Exists() const
{
	StatHelper statHlp;
	VError verr=statHlp.Stat(fPath);

    // Check for existence then that it is a "regular" file and nothing else (WAK0073807).

    if(verr==VE_OK && statHlp.IsRegular())

        return true;    
    
    return false;   //The file may exist but we can't access it. Let's pretend it doesn't exist !
}


//There is no creation time on Linux, but there is a status change time (modification of file's inode).
VError XLinuxFile::GetTimeAttributes(VTime* outLastModification, VTime* outLastStatusChange, VTime* outLastAccess) const
{
	StatHelper statHlp;
	VError verr=statHlp.Stat(fPath);

	if(verr!=VE_OK)
		return verr;

	if(outLastAccess!=NULL)
		outLastAccess->FromTime(statHlp.GetLastAccess());

	if(outLastModification!=NULL)
		outLastModification->FromTime(statHlp.GetLastModification());

	if(outLastStatusChange!=NULL)
		outLastStatusChange->FromTime(statHlp.GetLastStatusChange());

    return VE_OK;
}


VError XLinuxFile::SetTimeAttributes(const VTime *inLastModification, const VTime *UNUSED_CREATION_TIME, const VTime *inLastAccess) const
{
	xbox_assert(UNUSED_CREATION_TIME==NULL);

	if(inLastModification==NULL && inLastAccess==NULL)
		return VE_INVALID_PARAMETER;

	VTime modif;
	VTime access;

	VError verr=GetTimeAttributes(&modif, NULL, &access);

	if(verr!=VE_OK)
		return verr;

	const VTime* modifPtr=(inLastModification!=NULL ? inLastModification : &modif);
	const VTime* accessPtr=(inLastAccess!=NULL ? inLastAccess : &access);

	TouchHelper tchHlp;

	verr=tchHlp.Touch(fPath, *accessPtr, *modifPtr);

	return verr;
}


VError XLinuxFile::GetFileAttributes(EFileAttributes &outFileAttributes) const
{
	//FATT_LockedFile = 1,  <- on aimerait bien le savoir :) Creuser la piste mode readonly sur fd et perms ?
	//FATT_HidenFile = 2	<- pas de fichier caché sous Linux

	// Hidden files still no supported. (Check for a file name starting with a dot "." ?)

	StatHelper	statHelper;
	XBOX::VError	error;

	outFileAttributes = 0;
	if ((error = statHelper.Stat(fPath)) == XBOX::VE_OK && !statHelper.ProcessCanWrite()) 

		outFileAttributes |= FATT_LockedFile;

	return error;
}




VError XLinuxFile::SetFileAttributes(EFileAttributes inFileAttributes) const
{
	//FATT_LockedFile = 1,  <- on aimerait bien le savoir :) Creuser la piste mode readonly sur fd et perms ?
	//FATT_HidenFile = 2	<- pas de fichier caché sous Linux

    // Used in VFile::GetFileAttributes
	bool FileAttributesImplementedOnLinux=false;
    xbox_assert(FileAttributesImplementedOnLinux); // Postponed Linux Implementation !

    return VE_UNIMPLEMENTED;
}


VError XLinuxFile::GetVolumeFreeSpace(sLONG8* outFreeBytes, sLONG8* outTotalBytes, bool inWithQuotas) const
{
	if(outFreeBytes==NULL && outTotalBytes==NULL)
		return VE_INVALID_PARAMETER;	//We didn't failed nor succeed. Let's say we failed !

	//jmo - todo : verifier si ca marche correctement avec les quotas utilisateur

	StatHelper statHlp;
	VError verr=statHlp.SetFlags(StatHelper::withFileSystemStats).Stat(fPath);

	if(verr!=VE_OK)
		return verr;
	
	sLONG8 freeBlocks=(inWithQuotas ? statHlp.GetFsAvailableBlocks() : statHlp.GetFsFreeBlocks());
	sLONG8 totalBlocks=statHlp.GetFsTotalBlocks();
	sLONG8 blockSize=statHlp.GetFsBlockSize();

	if(outFreeBytes!=NULL)
		*outFreeBytes=freeBlocks*blockSize;

	if(outTotalBytes!=NULL)
		*outTotalBytes=totalBlocks*blockSize;

	return VE_OK;
}


VError XLinuxFile::CreateAlias(const VFilePath &inTargetPath, const VFilePath *unused) const
{
	PathBuffer targetPath;
	targetPath.Init(inTargetPath);

	LinkHelper lnkHlp;
	VError verr=lnkHlp.Link(fPath, targetPath);

	return verr;
}


//Unusual, but not an error -------------+
//                                       |
//                                       V 
VError XLinuxFile::ResolveAlias(VFilePath& outTargetPath) const
{
	PathBuffer targetPath;
	VError verr=fPath.ReadLink(&targetPath);

	verr=fPath.ToPath(&outTargetPath); //Todo ! .FromFullPath(targetString);
	
    return verr;
}


bool XLinuxFile::IsAliasFile(bool* outIsValidAlias) const
{
	StatHelper statHlp;
	VError verr=statHlp.Stat(fPath);

	if(verr!=VE_OK)
		return false;

    if(statHlp.IsLink())
    {
        if(outIsValidAlias!=NULL)
            *outIsValidAlias=statHlp.Access(fPath);

        return true;
    }
    
    return false;
}


VError XLinuxFile::ConformsTo(const VFileKind& inKind, bool *outConformant) const
{
	// sc 29/07/2011 method reviewed : check if the kind of this file is conform to inKind

	VStr31 extension;
	fOwner->GetPath().GetExtension( extension);
	
	VFileKind *fileKind = VFileKindManager::Get()->RetainFirstFileKindMatchingWithExtension( extension);
	if (fileKind != NULL)
	{
		*outConformant = IsFileKindConformsTo( *fileKind, inKind);
		fileKind->Release();

		return VE_OK;
	}

	return VE_INVALID_PARAMETER;
}


bool XLinuxFile::IsFileKindConformsTo( const VFileKind& inFirstKind, const VFileKind& inSecondKind)
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


VError XLinuxFile::GetPosixPath(VString& outPath) const
{
	return fPath.ToPath(&outPath);
}


VError XLinuxFile::GetSize(sLONG8 *outSize) const
{
	if(outSize==NULL)
		return VE_INVALID_PARAMETER;

	StatHelper statHlp;
	VError verr=statHlp.Stat(fPath);

	if(verr!=VE_OK)
		return verr;

	*outSize=statHlp.GetSize();

	return VE_OK;
}


VError XLinuxFile::GetResourceForkSize(sLONG8 *outSize) const
{
    // Used in VFile::GetResourceForkSize
	bool GetResourceForkSizeMethod=false;
    xbox_assert(GetResourceForkSizeMethod); // Postponed Linux Implementation !
    return VE_UNIMPLEMENTED;
}


bool XLinuxFile::HasResourceFork() const
{
    // Used in VFile::HasResourceFork
    return false; // Postponed Linux Implementation !
}


VError XLinuxFile::SetPermissions(uWORD inMode)
{
    // Optional
    // Used in VFile::LINUX_SetPermissions
    return VE_UNIMPLEMENTED;	// Postponed Linux Implementation !
}


VError XLinuxFile::GetPermissions(uWORD *outMode)
{
    // Optional
    // Used in VFile::LINUX_GetPermissions
    return VE_UNIMPLEMENTED;	// Postponed Linux Implementation !
}


// VError XLinuxFile::ConformsTo(const VFileKind& inKind, bool *outConformant) const
// {
//     //Verifier les liens entre cette methode et GetKind/SetKind et les autres qui n'existent pas sous Linux
//     // Used in VFile::ConformsTo
//     return VE_UNIMPLEMENTED;
// }


VError XLinuxFile::Exchange(const VFile& inExhangeWith) const
{
    // Used in VFile::Exchange - No Windows implementation
    return VE_UNIMPLEMENTED;
}


//static
VFileKind* XLinuxFile::CreatePublicFileKind(const VString& inID)
{
	//No Windows implementation
    return NULL;
}


//static
VFileKind* XLinuxFile::CreatePublicFileKindFromOsKind(const VString& inOsKind)
{
	//No Windows implementation
    return NULL;
}


//static
VFileKind* XLinuxFile::CreatePublicFileKindFromExtension(const VString& inExtension)
{
  DebugMsg(VString("Try to create new file kind from extension : ").AppendString(inExtension).AppendCString("\n"));

  return NULL;
}


XLinuxFileIterator::XLinuxFileIterator(const VFolder *inFolder, FileIteratorOptions inOptions)
: fOpts(0)
, fIt(NULL)
, fFileSystem( inFolder->GetFileSystem())
{
	//Because Next() returns a VFile ptr, we want files and no folders.
	fOpts=inOptions;
	fOpts|=FI_WANT_FILES;
	fOpts&=~FI_WANT_FOLDERS;

	VFilePath tmpPath;
	inFolder->GetPath(tmpPath);

	PathBuffer path;
	path.Init(tmpPath);

	fIt=new FsIterator(path, inOptions);
}


//virtual
XLinuxFileIterator::~XLinuxFileIterator()
{
	delete fIt;
}


VFile* XLinuxFileIterator::XLinuxFileIterator::Next()
{
	PathBuffer path;
	PathBuffer* pathPtr=fIt->Next(&path, fOpts);

	if(pathPtr==NULL)
		return NULL;

	VFilePath tmpPath;
	pathPtr->ToPath(&tmpPath);

	VFile *file;
	if ( (fFileSystem == NULL) || tmpPath.IsChildOf( fFileSystem->GetRoot()) )
		file = new VFile(tmpPath, fFileSystem);
	else
		file = NULL;
	
	return file;
}

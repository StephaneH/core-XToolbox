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
#include "VProcess.h"
#include "XMacfolder.h"

const sLONG	kMAX_PATH_SIZE	= 2048;


static void _CallBackProc( FSFileOperationRef fileOp, const FSRef *currentItem, FSFileOperationStage stage, OSStatus error, CFDictionaryRef statusDictionary, void *info )
{
	if ( stage == kFSOperationStageComplete )
		((VSyncEvent*)info)->Unlock();
}


XMacFolder::~XMacFolder()
{
	if (fPosixPath)
		::free( fPosixPath);
}

VError XMacFolder::Create() const
{
	OSErr macError = dirNFErr;
	FSRef parentRef;
	VString fileName;

	if ( _BuildParentFSRefFile(&parentRef,fileName) )
	{
		FSRef folderRef;
		macError = ::FSCreateDirectoryUnicode( &parentRef, fileName.GetLength(), fileName.GetCPointer(), kFSCatInfoNone, NULL, &folderRef, NULL, NULL);
	}
	return MAKE_NATIVE_VERROR( macError);
}


VError XMacFolder::Delete() const
{
	FSRef fileRef;
	_BuildFSRefFile(&fileRef);
	return MAKE_NATIVE_VERROR(::FSDeleteObject(&fileRef));
}


VError XMacFolder::MoveToTrash() const
{
	FSRef folderRef;
	_BuildFSRefFile( &folderRef);

    OptionBits options = kFSFileOperationDefaultOptions;
    
    OSStatus status;
    if (VTask::GetCurrent()->IsMain() )
    {
        status = ::FSMoveObjectToTrashSync( &folderRef, NULL, options);
    }
    else
    {
        VSyncEvent *syncEvent = new VSyncEvent();
        if (syncEvent)
        {
            FSFileOperationClientContext clientContext;
            clientContext.version = 0;
            clientContext.info = (void*) syncEvent;
            clientContext.retain = NULL;
            clientContext.release = NULL;
            clientContext.copyDescription = NULL;				
            
            FSFileOperationRef fileOp = ::FSFileOperationCreate( kCFAllocatorDefault );
            if ( testAssert(fileOp != NULL) )
            {
                status = ::FSFileOperationScheduleWithRunLoop( fileOp, CFRunLoopGetMain(), kCFRunLoopCommonModes );
                
                if (status == noErr)
                {
                    status = ::FSMoveObjectToTrashAsync ( fileOp, &folderRef, options, &_CallBackProc, 1.0/60.0, &clientContext );
                }
                if (status == noErr)
                {
                    syncEvent->Lock();
                    FSRef dstRef;
                    ::FSFileOperationCopyStatus( fileOp, &dstRef, NULL, &status, NULL, NULL );
                }
                
                ::CFRelease( fileOp );
            }
            else
            {
                status = mFulErr;
            }
        }
        else
        {
           status = mFulErr;
        }
        ReleaseRefCountable( &syncEvent);
    }
	return MAKE_NATIVE_VERROR( status);
}


bool XMacFolder::Exists( bool inAcceptFolderAlias) const
{
	bool ok = false;
	const UInt8 *path = _GetPosixPath();
	if (path != NULL)
	{
		FSRef fileTest;
		Boolean isDirectory;
		OSStatus macError = ::FSPathMakeRef( path, &fileTest, &isDirectory);
		if (macError == noErr)
		{
			if (isDirectory)
			{
				ok = true;
			}
			else if (inAcceptFolderAlias)
			{
				Boolean	wasAliased;
				macError = ::FSResolveAliasFile( &fileTest, true, &isDirectory, &wasAliased);
				ok = (macError == noErr) && isDirectory;
			}
		}
	}

	return ok;
}


VError XMacFolder::RevealInFinder() const
{
	OSErr			macError;
	FSRef			parentRef;
	if ( _BuildFSRefFile( &parentRef) )
	{
		macError = XMacFile::RevealFSRefInFinder( parentRef);
	}
	else
	{
		macError = dirNFErr;
	}

	return MAKE_NATIVE_VERROR( macError);
}

VError XMacFolder::IsHidden( bool &outIsHidden)
{
	outIsHidden = false;
	
	FSRef ref;
	OSErr macError = fnfErr;

	if (_BuildFSRefFile( &ref) )
	{
		LSItemInfoRecord info;
		macError = ::LSCopyItemInfoForRef( &ref, kLSRequestBasicFlagsOnly, &info);
		if ((macError == noErr) && ( (info.flags & kLSItemInfoIsInvisible) != 0 ))
		{
			outIsHidden = true;
		}
	}
	
	return MAKE_NATIVE_VERROR(macError);

}

VError XMacFolder::Rename( const VString& inName, VFolder** outFile ) const
{
	OSErr macError = dirNFErr;
	FSRef srcRef,newRef;
	
	if ( _BuildFSRefFile(&srcRef) )
		macError = ::FSRenameUnicode ( &srcRef, inName.GetLength(), inName.GetCPointer(), kTextEncodingDefaultFormat, &newRef );
	if(outFile) 
	{
		if(macError == noErr)
		{
			VFilePath renamedPath( fOwner->GetPath());
			renamedPath.SetName( inName);
			*outFile = new VFolder( renamedPath, fOwner->GetFileSystem());
		}
		else
			*outFile = NULL;
	}
	return MAKE_NATIVE_VERROR(macError);
}

// pp move a folder. src and dest must be on the volume, destination must not exist

VError XMacFolder::Move( const VFolder& inNewParent, const VString *inNewName, VFolder** outFolder ) const
{
	OSStatus macError = noErr;
	FSRef srcRef, dstRef;
	
	if ( _BuildFSRefFile( &srcRef))
	{
		// get destination file name
		VString destinationName;
		
		fOwner->GetName( destinationName);
		
		// get destination folder ref
		FSRef dstFolderRef;
		VFilePath path_destination_folder;
		path_destination_folder=inNewParent.GetPath();
						
		if (inNewParent.MAC_GetFSRef(&dstFolderRef))
		{
			OptionBits options = 0;

			CFStringRef cfNewName = ( (inNewName != NULL) && !inNewName->IsEmpty() ) ? inNewName->MAC_RetainCFStringCopy() : NULL;
			
			if ( VTask::GetCurrent()->IsMain() || (CFRunLoopGetMain == NULL)  )
			{
				macError = ::FSMoveObjectSync( &srcRef, &dstFolderRef, cfNewName, &dstRef, options);
			}
			else
			{
				VSyncEvent *syncEvent = new VSyncEvent();
				
				if (syncEvent)
				{
					FSFileOperationClientContext clientContext;
					clientContext.version = 0;
					clientContext.info = (void*) syncEvent;
					clientContext.retain = NULL;
					clientContext.release = NULL;
					clientContext.copyDescription = NULL;				
					
					FSFileOperationRef fileOp = ::FSFileOperationCreate ( kCFAllocatorDefault );
					if ( testAssert(fileOp != NULL) )
					{
						macError = ::FSFileOperationScheduleWithRunLoop ( fileOp, CFRunLoopGetMain(), kCFRunLoopCommonModes );
						
						if ( macError == noErr )
						{
							macError = ::FSMoveObjectAsync ( fileOp, &srcRef, &dstFolderRef, cfNewName, options, &_CallBackProc, 1.0/60.0, &clientContext );
						}
						
						if ( macError == noErr )
						{
							syncEvent->Lock();
							::FSFileOperationCopyStatus( fileOp, &dstRef, NULL, &macError, NULL, NULL );
						}
						
						::CFRelease( fileOp );
					}
					else
					{
						macError = mFulErr;
					}
					syncEvent->Release();
				}
				else
				{
					macError = mFulErr;
				}
			}
			if (cfNewName != NULL)
				CFRelease( cfNewName);
		}
	}
	else
	{
		macError = fnfErr;
	}

	if (outFolder != NULL)
	{
		if (macError == noErr)
		{
			VString fullPath;
			if (testAssert( XMacFile::FSRefToHFSPath( dstRef, fullPath) == noErr))
				*outFolder = new VFolder( VFilePath( fullPath.AppendUniChar( FOLDER_SEPARATOR)), inNewParent.GetFileSystem());
			else
				*outFolder = NULL;
		}
		else
			*outFolder = NULL;
	}
	return MAKE_NATIVE_VERROR( macError);
}

VError XMacFolder::SetTimeAttributes( const VTime *inLastModification, const VTime *inCreationTime, const VTime *inLastAccess ) const
{
	OSErr macError = fnfErr;
	
	FSRef fileRef;
	FSCatalogInfo catInfo;
	
	if ( _BuildFSRefFile( &fileRef) )
	{
		sLONG toSet=0;
		sWORD dateTime[7];
		CFAbsoluteTime absTime;
		CFGregorianDate	gregorianDate;
		macError = ::FSGetCatalogInfo(&fileRef, kFSCatInfoContentMod|kFSCatInfoAccessDate|kFSCatInfoCreateDate, &catInfo, NULL, NULL, NULL);
		
		if ( inLastModification )
		{
			inLastModification->GetUTCTime(dateTime[0],dateTime[1],dateTime[2],dateTime[3],dateTime[4],dateTime[5],dateTime[6]);
			gregorianDate.year=dateTime[0];gregorianDate.month=dateTime[1];gregorianDate.day=dateTime[2];
			gregorianDate.hour=dateTime[3];gregorianDate.minute=dateTime[4];gregorianDate.second=dateTime[5];
			absTime = ::CFGregorianDateGetAbsoluteTime(gregorianDate, NULL);
			::UCConvertCFAbsoluteTimeToUTCDateTime(absTime,&catInfo.contentModDate);
			toSet|=kFSCatInfoContentMod;
		}
		
		if ( inCreationTime )
		{
			inCreationTime->GetUTCTime(dateTime[0],dateTime[1],dateTime[2],dateTime[3],dateTime[4],dateTime[5],dateTime[6]);
			gregorianDate.year=dateTime[0];gregorianDate.month=dateTime[1];gregorianDate.day=dateTime[2];
			gregorianDate.hour=dateTime[3];gregorianDate.minute=dateTime[4];gregorianDate.second=dateTime[5];
			absTime = ::CFGregorianDateGetAbsoluteTime(gregorianDate, NULL);
			::UCConvertCFAbsoluteTimeToUTCDateTime(absTime,&catInfo.createDate);
			toSet|=kFSCatInfoCreateDate;
		}
		
		if ( inLastAccess )
		{
			inLastAccess->GetUTCTime(dateTime[0],dateTime[1],dateTime[2],dateTime[3],dateTime[4],dateTime[5],dateTime[6]);
			gregorianDate.year=dateTime[0];gregorianDate.month=dateTime[1];gregorianDate.day=dateTime[2];
			gregorianDate.hour=dateTime[3];gregorianDate.minute=dateTime[4];gregorianDate.second=dateTime[5];
			absTime = ::CFGregorianDateGetAbsoluteTime(gregorianDate, NULL);
			::UCConvertCFAbsoluteTimeToUTCDateTime(absTime,&catInfo.accessDate);
			toSet|=kFSCatInfoAccessDate;
		}
		if(toSet)
			macError = ::FSSetCatalogInfo(&fileRef, toSet, &catInfo);
		else
			macError=noErr;
		
	}
	
	return MAKE_NATIVE_VERROR(macError);
}
VError XMacFolder::GetTimeAttributes( VTime* outLastModification, VTime* outCreationTime, VTime* outLastAccess ) const
{
	OSErr macError = dirNFErr;

	if (outLastModification)
		outLastModification->Clear();
	
	if (outCreationTime)
		outCreationTime->Clear();
	
	if (outLastAccess)
		outLastAccess->Clear();
	
	FSRef fileRef;
	FSCatalogInfo catInfo;

	if ( _BuildFSRefFile(&fileRef) )
	{
		CFAbsoluteTime absTime;
		macError = ::FSGetCatalogInfo(&fileRef, kFSCatInfoContentMod|kFSCatInfoAccessDate|kFSCatInfoCreateDate, &catInfo, NULL, NULL, NULL);

		if ( outLastModification )
		{
			::UCConvertUTCDateTimeToCFAbsoluteTime(&catInfo.contentModDate, &absTime);
			CFGregorianDate	gregorianDate = ::CFAbsoluteTimeGetGregorianDate(absTime, NULL);
			outLastModification->FromUTCTime((sWORD)gregorianDate.year,gregorianDate.month,gregorianDate.day,gregorianDate.hour,gregorianDate.minute,(sWORD)gregorianDate.second,0);
		}

		if ( outCreationTime )
		{
			::UCConvertUTCDateTimeToCFAbsoluteTime(&catInfo.createDate, &absTime);
			CFGregorianDate	gregorianDate = ::CFAbsoluteTimeGetGregorianDate(absTime, NULL);
			outCreationTime->FromUTCTime((sWORD)gregorianDate.year,gregorianDate.month,gregorianDate.day,gregorianDate.hour,gregorianDate.minute,(sWORD)gregorianDate.second,0);
		}

		if ( outLastAccess )
		{
			::UCConvertUTCDateTimeToCFAbsoluteTime(&catInfo.accessDate, &absTime);
			CFGregorianDate	gregorianDate = ::CFAbsoluteTimeGetGregorianDate(absTime, NULL);
			outLastAccess->FromUTCTime((sWORD)gregorianDate.year,gregorianDate.month,gregorianDate.day,gregorianDate.hour,gregorianDate.minute,(sWORD)gregorianDate.second,0);
		}
	}

	return MAKE_NATIVE_VERROR(macError);
}
VError XMacFolder::GetVolumeCapacity (sLONG8 *outTotalBytes ) const
{
	FSRef fileRef;
	OSErr macError = dirNFErr;

	if ( _BuildFSRefFile(&fileRef) )
	{
		FSCatalogInfo info;
		macError = ::FSGetCatalogInfo(&fileRef, kFSCatInfoVolume, &info, NULL, NULL, NULL);
		if (macError == noErr)
		{
			FSVolumeInfo volinfo;
			macError = ::FSGetVolumeInfo(info.volume, 0, NULL, kFSVolInfoSizes, &volinfo, NULL, NULL);
			if (macError == noErr)
			{
				*outTotalBytes = volinfo.totalBytes;
			}
		}
	}

	return MAKE_NATIVE_VERROR(macError);

}

VError XMacFolder::GetVolumeFreeSpace(sLONG8 *outFreeBytes, bool inWithQuotas ) const
{
	FSRef fileRef;
	OSErr macError = dirNFErr;

	if ( _BuildFSRefFile(&fileRef) )
	{
		FSCatalogInfo info;
		macError = ::FSGetCatalogInfo(&fileRef, kFSCatInfoVolume, &info, NULL, NULL, NULL);
		if (macError == noErr)
		{
			FSVolumeInfo volinfo;
			macError = ::FSGetVolumeInfo(info.volume, 0, NULL, kFSVolInfoSizes, &volinfo, NULL, NULL);
			if (macError == noErr)
			{
				*outFreeBytes = volinfo.freeBytes;
			}
		}
	}

	return MAKE_NATIVE_VERROR(macError);
}


const UInt8 *XMacFolder::_GetPosixPath() const
{
	if (fPosixPath == NULL)
	{
		// builds UTF8 posix path now and keep it (VFolder path can't change)
		VString fullPath;
		fOwner->GetPath( fullPath);

		UInt8 *path = XMacFile::PosixPathFromHFSPath( fullPath, true);

		xbox_assert( path != NULL);
		
		// remember that VFile must be thread-safe
		if (VInterlocked::CompareExchangePtr( (void**)&fPosixPath, NULL, path) != NULL)
			::free( path);
	}
	return fPosixPath;
}


bool XMacFolder::_BuildFSRefFile( FSRef *outFileRef) const
{
	const UInt8 *path = _GetPosixPath();
	if (path != NULL)
	{
		Boolean isDirectory;
		OSStatus macError = ::FSPathMakeRef( path, outFileRef, &isDirectory);
		return (macError == noErr) && isDirectory;
	}

	return false;
}


bool XMacFolder::_BuildParentFSRefFile(FSRef *outParentRef, VString &outFileName ) const
{
	VFilePath parentPath;
	fOwner->GetPath().GetName(outFileName);
	fOwner->GetPath().GetParent(parentPath);
	return XMacFile::HFSPathToFSRef(parentPath.GetPath(),outParentRef) == noErr;
}


VError XMacFolder::GetPosixPath( VString& outPath) const
{
	OSStatus macError;

	FSRef fileRef;
	if ( _BuildFSRefFile( &fileRef) && testAssert( fPosixPath != NULL) )
	{
		outPath.FromBlock( fPosixPath, (VSize) ::strlen( (const char *) fPosixPath), VTC_UTF_8);
		outPath+="/";
		
		// fPosixPath is a pointer to *decomposed* utf-8 (follow _BuildFSRefFile()) while a VString is supposed to be composed
		outPath.Compose();
		
		macError = noErr;
	}
	else
	{
		macError = fnfErr;
	}

	return MAKE_NATIVE_VERROR(macError);
}


VError XMacFolder::MakeWritableByAll()
{
	uWORD folderPermissions = 0;
	GetPermissions(&folderPermissions);
	uWORD writablePermission = folderPermissions | 0777;	// 0777 b/c in practice what we want is in fact 'readable and writable and parsable by all' - m.c, ACI0053479
	return SetPermissions(writablePermission);
}

bool XMacFolder::IsWritableByAll()
{
	uWORD folderPermissions = 0;
	GetPermissions( &folderPermissions);
	
	return (folderPermissions & 0222) != 0;
}

VError XMacFolder::SetPermissions( uWORD inMode )
{
	OSErr macError = fnfErr;
	FSRef folderRef;
	
	if ( _BuildFSRefFile( &folderRef) )
	{
		FSCatalogInfo fileInfo;
		macError = ::FSGetCatalogInfo(&folderRef, kFSCatInfoPermissions, &fileInfo, NULL, NULL, NULL);
		if ( macError == noErr )
		{
			#if __LP64__
			fileInfo.permissions.mode = inMode;
			#else
			((FSPermissionInfo*) fileInfo.permissions)->mode = inMode;
			#endif
			macError = ::FSSetCatalogInfo( &folderRef, kFSCatInfoPermissions, &fileInfo );
		}
	}
	
	return MAKE_NATIVE_VERROR(macError);
}

VError XMacFolder::GetPermissions( uWORD *outMode )
{
	OSErr macError = fnfErr;
	FSRef folderRef;
	
	if ( _BuildFSRefFile( &folderRef) )
	{
		FSCatalogInfo fileInfo;
		macError = ::FSGetCatalogInfo(&folderRef, kFSCatInfoPermissions, &fileInfo, NULL, NULL, NULL);
		if ( macError == noErr )
		{
			#if __LP64__
			*outMode = fileInfo.permissions.mode;
			#else
			*outMode = ((FSPermissionInfo*) fileInfo.permissions)->mode;
			#endif
		}
	}
	
	return MAKE_NATIVE_VERROR(macError);
}

bool XMacFolder::GetIconFile( IconRef &outIconRef ) const
{
	FSRef folderRef;
	outIconRef = NULL;
	OSStatus macStatus = noErr;
		
	if ( _BuildFSRefFile( &folderRef) )
	{
		HFSUniStr255 fileName;
		FSCatalogInfo fileInfo;

		macStatus = ::FSGetCatalogInfo( &folderRef, kIconServicesCatalogInfoMask, &fileInfo, &fileName, NULL, NULL );
		if ( macStatus == noErr )
		{
			macStatus = ::GetIconRefFromFileInfo ( &folderRef, fileName.length, fileName.unicode, kIconServicesCatalogInfoMask, &fileInfo, kIconServicesNormalUsageFlag, &outIconRef, NULL );
		}
	}
	return ( macStatus == noErr );
}

VError XMacFolder::RetainSystemFolder( ESystemFolderKind inFolderKind, bool inCreateFolder, VFolder **outFolder )
{
	OSErr macError = dirNFErr;

	FSRef folderRef;
	VFolder* sysFolder = NULL;
	switch( inFolderKind)
	{
		case eFK_Executable:
		{
			CFBundleRef appBundle = ::CFBundleGetMainBundle();
			if (testAssert( appBundle != NULL ))
			{
				CFURLRef bundleURL = ::CFBundleCopyExecutableURL(appBundle ); 
					
				if ( testAssert( bundleURL != NULL))
				{
					FSRef bundleRef;
					if (testAssert( ::CFURLGetFSRef( bundleURL, &bundleRef) ))
					{
						FSRef parentRef;
						macError = ::FSGetCatalogInfo( &bundleRef, kFSCatInfoNone, NULL, NULL, NULL, &folderRef);
					}
					::CFRelease(bundleURL );
				}
			}
			break;
		}
	
		case eFK_UserDocuments:
			macError = ::FSFindFolder( kOnSystemDisk, kDocumentsFolderType, inCreateFolder ? kCreateFolder : kDontCreateFolder, &folderRef);
			break;

		case eFK_CommonPreferences:
		case eFK_UserPreferences:
			// AppStore guidelines require to store everything in ~/Library/Application Support
			macError = ::FSFindFolder(
				  (inFolderKind == eFK_CommonPreferences) ? kLocalDomain : kUserDomain
				, VProcess::Get()->GetPreferencesInApplicationData() ? (OSType) kApplicationSupportFolderType : (OSType) kPreferencesFolderType
				, inCreateFolder ? kCreateFolder : kDontCreateFolder
				, &folderRef);
			break;

		case eFK_CommonApplicationData:
		case eFK_UserApplicationData:
			macError = ::FSFindFolder( (inFolderKind == eFK_CommonApplicationData) ? kLocalDomain : kUserDomain, kApplicationSupportFolderType, inCreateFolder ? kCreateFolder : kDontCreateFolder, &folderRef);
			break;

		case eFK_CommonCache:
		case eFK_UserCache:
			macError = ::FSFindFolder( (inFolderKind == eFK_CommonCache) ? kLocalDomain : kUserDomain, kCachedDataFolderType, inCreateFolder ? kCreateFolder : kDontCreateFolder, &folderRef);
			break;
		
		case eFK_Temporary:
			{
				/*
					kChewableItemsFolderType is generally better because the temp files are deleted at reboot.
					items in kTemporaryFolderType are also deleted but we don't know when.
					
					each folder may or may not exists (no kChewableItemsFolderType on network volumes).
				*/
				
				macError = ::FSFindFolder( kOnAppropriateDisk, kChewableItemsFolderType, inCreateFolder ? kCreateFolder : kDontCreateFolder, &folderRef);
				
				if ( (macError == noErr) && !::FSIsFSRefValid( &folderRef ) )
				{
					// see http://lists.apple.com/archives/carbon-dev/2006/Jun/msg00052.html
					::InvalidateFolderDescriptorCache( 0, 0 );
					macError = ::FSFindFolder( kOnAppropriateDisk, kChewableItemsFolderType, inCreateFolder ? kCreateFolder : kDontCreateFolder, &folderRef);
				}

				if (macError != noErr)
					macError = ::FSFindFolder( kOnAppropriateDisk, kTemporaryFolderType, inCreateFolder ? kCreateFolder : kDontCreateFolder, &folderRef);

				break;
			}
			
		case eFK_StartupItemsFolder:
			macError = ::FSFindFolder( kLocalDomain, kBootTimeStartupItemsFolderType, inCreateFolder ? kCreateFolder : kDontCreateFolder, &folderRef);
			break;
			
		case eFK_MAC_UserLibrary:
			macError = ::FSFindFolder( kUserDomain, kDomainLibraryFolderType, kDontCreateFolder, &folderRef);
			break;
		
	}
	
	if (macError == noErr)
		*outFolder = new VFolder( folderRef);

	return MAKE_NATIVE_VERROR(macError);
}

//========================================================================================


XMacFolderIterator::XMacFolderIterator( const VFolder *inFolder, FileIteratorOptions inOptions)
: fOptions( inOptions)
, fFileSystem( inFolder->GetFileSystem())
{
	fIndex = 0;
	fIterator = NULL;

	VString folderPath;
	inFolder->GetPath( folderPath);
	
	if (XMacFile::HFSPathToFSRef(folderPath, &fFolderRef) == noErr)
	{
		FSIteratorFlags options = kFSIterateFlat;
		if ( inOptions & FI_ITERATE_DELETE )
			options |= kFSIterateDelete;
		OSErr macErr = ::FSOpenIterator( &fFolderRef, options, &fIterator);
		xbox_assert( macErr == noErr);
	}
}


XMacFolderIterator::~XMacFolderIterator()
{
	if (fIterator != NULL)
		::FSCloseIterator( fIterator);
}


VFolder* XMacFolderIterator::Next()
{
	VFolder *result = _GetCatalogInfo();
	if (result != NULL)
		fIndex++;
	
	return result;
}


VFolder* XMacFolderIterator::_GetCatalogInfo ()
{
	VFolder*	object = NULL;
	if (fIterator != NULL)
	{
		OSStatus	error;
		FSRef		objectRef;
		FSCatalogInfo	objectInfo;
		
		do {
			ItemCount	actualCount = 0;
			error = ::FSGetCatalogInfoBulk(fIterator, 1, &actualCount, NULL, kFSCatInfoNodeFlags | kFSCatInfoFinderInfo, &objectInfo, &objectRef, NULL, NULL);
			if (error == noErr)
			{
				xbox_assert(actualCount != 0);
				
				bool	isAlias = (((FileInfo*) objectInfo.finderInfo)->finderFlags & kIsAlias) != 0;
				Boolean	isFolder = (objectInfo.nodeFlags & kFSNodeIsDirectoryMask) != 0;
				
				bool skip = false;
				if ( (fOptions & FI_WANT_INVISIBLES) == 0)
				{
					LSItemInfoRecord info;
					error = ::LSCopyItemInfoForRef( &objectRef, kLSRequestBasicFlagsOnly, &info);
					if ((error == noErr) && ( (info.flags & kLSItemInfoIsInvisible) != 0 ))
						skip = true;
				}
				
				if (isAlias && (fOptions & FI_RESOLVE_ALIASES) && !skip)
				{
					Boolean	wasAliased;
					error = ::FSResolveAliasFileWithMountFlags(&objectRef, true, &isFolder, &wasAliased, kResolveAliasFileNoUI);
					xbox_assert(error == noErr || error == fnfErr);
					
					if ( (error == noErr) && ( (fOptions & FI_WANT_INVISIBLES) == 0) )
					{
						LSItemInfoRecord info;
						error = ::LSCopyItemInfoForRef( &objectRef, kLSRequestBasicFlagsOnly, &info);
						xbox_assert( error == noErr);
						skip = (error == noErr) && ( (info.flags & kLSItemInfoIsInvisible) != 0);
					}
				}
				
				if ( (error == noErr) && !skip )
				{
					if (isFolder && (fOptions & FI_WANT_FOLDERS) )
					{
						VString objectFullPath;
						if (testAssert( XMacFile::FSRefToHFSPath( objectRef, objectFullPath) == noErr))
						{
							objectFullPath.AppendUniChar( FOLDER_SEPARATOR);
							VFilePath objectPath( objectFullPath);
							xbox_assert( objectPath.IsFolder());

							// check not to get out of file system
							if ( (fFileSystem == NULL) || objectPath.IsChildOf( fFileSystem->GetRoot()) )
								object = new VFolder(objectPath, fFileSystem);
						}
					}
				}
			}
			else
			{
				xbox_assert(actualCount == 0);
				xbox_assert(error == errFSNoMoreItems || (error == afpAccessDenied));
			}
		} while( (object == NULL) && (error != errFSNoMoreItems) && (error != afpAccessDenied));//[MI] le 29/11/2010 ACI0068754 
	}
	
	return object;
}


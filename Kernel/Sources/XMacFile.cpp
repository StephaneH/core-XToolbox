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
#include "VValueBag.h"
#include "XMacResource.h"


const sLONG	kMAX_PATH_SIZE	= 2048;


// CFRunLoopGetMain is a 10.5 api that may exist in 10.4.
#if MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_4
typedef CFRunLoopRef (*CFRunLoopGetMainProcPtr)(void);
static CFRunLoopGetMainProcPtr CFRunLoopGetMain = NULL;

typedef OSStatus (*FSGetVolumeParmsProcPtr)(FSVolumeRefNum volume, GetVolParmsInfoBuffer *buffer, ByteCount bufferSize);
static FSGetVolumeParmsProcPtr FSGetVolumeParms = (FSGetVolumeParmsProcPtr) ::CFBundleGetFunctionPointerForName(::CFBundleGetBundleWithIdentifier(CFSTR("com.apple.CoreServices")), CFSTR("FSGetVolumeParms"));

typedef OSErr (*FSUnlinkObjectProcPtr)(const FSRef *);
static FSUnlinkObjectProcPtr FSUnlinkObject = (FSUnlinkObjectProcPtr) ::CFBundleGetFunctionPointerForName(::CFBundleGetBundleWithIdentifier(CFSTR("com.apple.CoreServices")), CFSTR("FSUnlinkObject"));
#endif

//================================================================================================


static OSErr IsRemoteVolume( FSVolumeRefNum inVolumeRef, bool *outIsRemote)
{
	if (!testAssert( FSGetVolumeParms != NULL))
		return unimpErr;
	
	GetVolParmsInfoBuffer info = {0};
	OSStatus err = ::FSGetVolumeParms( inVolumeRef, &info, sizeof( info));
	if (err == noErr)
	{
		bool internal = (info.vMExtendedAttributes & (1 << bIsOnInternalBus)) != 0;
		bool external = (info.vMExtendedAttributes & (1 << bIsOnExternalBus)) != 0;

		*outIsRemote = !internal && !external;
	}
	return err;
}


static OSErr IsOnRemoteVolume( const FSRef *inFileRef, bool *outIsRemote)
{
	FSCatalogInfo catinfo = {0};
	OSStatus err = ::FSGetCatalogInfo( inFileRef, kFSCatInfoVolume, &catinfo, NULL, NULL, NULL);
	
	if (err == noErr)
		err = IsRemoteVolume( catinfo.volume, outIsRemote);
		
	return err;
}


//================================================================================================


VError XMacFileDesc::GetSize( sLONG8 *outSize) const
{
	OSErr macError = ::FSGetForkSize( fForkRef, outSize);

	return MAKE_NATIVE_VERROR( macError);
}


VError XMacFileDesc::SetSize( sLONG8 inSize) const
{
	OSErr macError = ::FSSetForkSize( fForkRef, fsFromStart, inSize);
	return MAKE_NATIVE_VERROR( macError);
}


VError XMacFileDesc::GetData(void *outData, VSize &ioCount, sLONG8 inOffset, bool inFromStart ) const
{
	ByteCount readCount = 0;
	OSErr macError = ::FSReadFork( fForkRef, inFromStart ? fsFromStart : fsAtMark, inOffset, ioCount, outData, &readCount);
	
	ioCount = readCount;

	return MAKE_NATIVE_VERROR( macError);
}


VError XMacFileDesc::PutData( const void *inData, VSize& ioCount, sLONG8 inOffset, bool inFromStart) const
{
	if ( ioCount == 0 )
		return VE_OK;
	else
	{
		ByteCount writenCount = 0;
		OSErr macError = ::FSWriteFork(fForkRef, inFromStart ? fsFromStart : fsAtMark, inOffset, ioCount, inData, &writenCount);

		ioCount = writenCount;
	
		return MAKE_NATIVE_VERROR( macError);
	}
}


VError XMacFileDesc::GetPos( sLONG8 *outPos) const
{
	sLONG8 filePos;
	OSErr macError = ::FSGetForkPosition( fForkRef, outPos);
	
	return MAKE_NATIVE_VERROR( macError);
}


VError XMacFileDesc::SetPos( sLONG8 inOffset, bool inFromStart ) const
{
	OSErr macError = ::FSSetForkPosition(fForkRef, inFromStart ? fsFromStart : fsFromMark, inOffset);

	return MAKE_NATIVE_VERROR( macError);
}


VError XMacFileDesc::Flush() const
{
	OSErr macError = ::FSFlushFork( fForkRef);

	return MAKE_NATIVE_VERROR( macError);
}


/*  -----  XMacFile -----*/


XMacFile::~XMacFile()
{
	if (fPosixPath)
		::free( fPosixPath);
}


/* static */
bool XMacFile::PosixPathFromHFSPath( const VString& inHFSPath, VString& outPosixPath, bool inIsDirectory)
{
	bool ok = false;
	outPosixPath.Clear();

	// convert hfs path to posix path using CFURL
	CFMutableStringRef	ref_hfsPath = ::CFStringCreateMutable( kCFAllocatorDefault, 0);
	if (ref_hfsPath != NULL)
	{
		::CFStringAppendCharacters( ref_hfsPath, inHFSPath.GetCPointer(), inHFSPath.GetLength());
		::CFStringNormalize( ref_hfsPath, kCFStringNormalizationFormD);	// decomposed unicode to please CFURL

		CFURLRef ref_URL = ::CFURLCreateWithFileSystemPath( NULL, ref_hfsPath, kCFURLHFSPathStyle, inIsDirectory);
		if (ref_URL != NULL)
		{
			CFStringRef ref_posixPath = ::CFURLCopyFileSystemPath( ref_URL, kCFURLPOSIXPathStyle);
			if (ref_posixPath != NULL)
			{
				outPosixPath.MAC_FromCFString( ref_posixPath);
				
				// ref_hfsPath/ref_posixPath were built *decomposed* (see previous lines, ::CFStringNormalize()) while a VString is supposed to be composed
				outPosixPath.Compose();
				
				ok = true;
				::CFRelease( ref_posixPath);
			}
			::CFRelease( ref_URL);
		}

		::CFRelease( ref_hfsPath);
	}
	return ok;
}


/* static */
UInt8 *XMacFile::PosixPathFromHFSPath( const VString& inHFSPath, bool inIsDirectory)
{
	UInt8 *buffer = NULL;

	// convert hfs path to posix path using CFURL
	CFMutableStringRef	ref_hfsPath = ::CFStringCreateMutable( kCFAllocatorDefault, 0);
	if (ref_hfsPath != NULL)
	{
		::CFStringAppendCharacters( ref_hfsPath, inHFSPath.GetCPointer(), inHFSPath.GetLength());
		::CFStringNormalize( ref_hfsPath, kCFStringNormalizationFormD);	// decomposed unicode to please CFURL

		CFURLRef ref_URL = ::CFURLCreateWithFileSystemPath( NULL, ref_hfsPath, kCFURLHFSPathStyle, inIsDirectory);
		if (ref_URL != NULL)
		{
			CFStringRef ref_posixPath = ::CFURLCopyFileSystemPath( ref_URL, kCFURLPOSIXPathStyle);
			if (ref_posixPath != NULL)
			{
				// get UTF8 characters

				CFRange range = {0,CFStringGetLength( ref_posixPath)};
				
				// get needed bytes
				CFIndex usedBufLen;
				CFIndex converted = ::CFStringGetBytes( ref_posixPath, range, kCFStringEncodingUTF8, 0 /* lossByte*/, false /*isExternalRepresentation*/, NULL, 0, &usedBufLen);
				buffer = (UInt8 *) malloc( usedBufLen + 1);	// add trailing zero
				if (buffer != NULL)
				{
					CFIndex maxBufLen = usedBufLen;
					converted = ::CFStringGetBytes( ref_posixPath, range, kCFStringEncodingUTF8, 0 /* lossByte*/, false /*isExternalRepresentation*/, buffer, maxBufLen, &usedBufLen);
					xbox_assert( usedBufLen == maxBufLen);
					xbox_assert( converted == range.length);
					buffer[usedBufLen] = 0;
				}
				::CFRelease( ref_posixPath);
			}
			::CFRelease( ref_URL);
		}

		::CFRelease( ref_hfsPath);
	}
	return buffer;
}


VError XMacFile::Create( FileCreateOptions inOptions) const
{
	OSErr macError = noErr;
	FSRef fileRef;
	bool weNeedToCreateANewFile = true;
	
	if ( (inOptions & FCR_Overwrite) && _BuildFSRefFile( &fileRef) )
	{
		// I. New safe overwrite method......
		bool fileOverwriteIsSuccessful = false;	
		weNeedToCreateANewFile = false;
		
		// 1. Create a temporary file with an unique name (in the same folder)
		FSRef parentRef;
		VString fileName;
		if ( _BuildParentFSRefFile( &parentRef, fileName) )
		{
			//  Temp file		
			FSRef tempFileRef;
			int maximumTriesCount = 15; //totally arbitrary choice
			int tryIndex = 0;
			do{
				VUUID uniqueID(true);
				VString uniqueFileName;
				uniqueID.GetString(uniqueFileName);
				uniqueFileName += ".temp";
				FSCatalogInfo tempFileInfo;
				::memset( &tempFileInfo, 0, sizeof( tempFileInfo));
				macError = ::FSCreateFileUnicode( &parentRef, uniqueFileName.GetLength(), uniqueFileName.GetCPointer(), kFSCatInfoNone, &tempFileInfo, &tempFileRef, NULL);
				++tryIndex;
			}
			while(noErr != macError && tryIndex < maximumTriesCount);
			
			// 2. Exchange the contents safely (files must be on the same physical volume)
			macError = ::FSExchangeObjects( &tempFileRef, &fileRef);
			
			if(macError == noErr){
				// 3. Delete the temporary file with the unlink stdio command ("the removal of the file is delayed until all references to it have been closed.")
				CFURLRef tempFileURLRef = ::CFURLCreateFromFSRef(kCFAllocatorDefault, &tempFileRef);
				
				if (tempFileURLRef) {
					CFStringRef tempFileURLStringRef = ::CFURLCopyFileSystemPath (tempFileURLRef, kCFURLPOSIXPathStyle);
					
					if (tempFileURLStringRef) {
						CFIndex tempStringSize = ::CFStringGetLength(tempFileURLStringRef) * 4 + 1;
						char * tempFileURLChar = (char *)malloc(tempStringSize);
					
						if (tempFileURLChar) {
							if(::CFStringGetCString(tempFileURLStringRef, tempFileURLChar, tempStringSize, kCFStringEncodingUTF8))
								if(unlink(tempFileURLChar) == 0)
									fileOverwriteIsSuccessful = true;				
							free(tempFileURLChar);
						}
						::CFRelease(tempFileURLStringRef);
					}
					::CFRelease(tempFileURLRef);
				}
			}
			
			// II. If the safe method didn't work, let's try the old one, and pray to avoid errors (light a candle too)
			if(fileOverwriteIsSuccessful == false){
				if((macError = ::FSDeleteObject( &fileRef)) == noErr){
					weNeedToCreateANewFile = true;
				}
			}
		}
	}
	
	if(weNeedToCreateANewFile == true){
		
		FSRef parentRef;
		VString fileName;
		if ( _BuildParentFSRefFile( &parentRef,fileName) )
		{
			FSCatalogInfo	fileInfo;
			::memset( &fileInfo, 0, sizeof( fileInfo));
			/*
			 ((FileInfo*) fileInfo.finderInfo)->fileType = 0;
			 ((FileInfo*) fileInfo.finderInfo)->fileCreator = 0;
			 ((FileInfo*) fileInfo.finderInfo)->finderFlags = 0;
			 ((FileInfo*) fileInfo.finderInfo)->location.h = 0;
			 ((FileInfo*) fileInfo.finderInfo)->location.v = 0;
			 ((FileInfo*) fileInfo.finderInfo)->reservedField = 0;
			 */
			
			macError = ::FSCreateFileUnicode( &parentRef, fileName.GetLength(), fileName.GetCPointer(), kFSCatInfoNone, &fileInfo, &fileRef, NULL);
		}
		else
		{
			macError = dirNFErr;
		}
	}
	
	return MAKE_NATIVE_VERROR( macError);
}


/*


	Here are the results of some experiments on opening modes for local, appleshare and simba volumes using OSX 10.5
	
	For each possible mode, a text file is opened and I try to reopen it using each possible mode.
	Modes listed are the ones for wich FSOpenFork was successful.
	
	If the opening was successful, I verify if I can read and write the file.

	Note 1: On remote volumes, even if the file opening in fsRdWrPerm mode was successfull, one can't write into it...
	Note 2: I also tried playing with deny modes but that seem useless.
	Note 3: remote volumes forbid opening in read mode a file already opened in shared mode.
	Note 4: possible error codes are permErr (-54) and opWrErr (-49)

	see also http://developer.apple.com/technotes/fl/fl_37.html (seems deprecated)
	
	======= LOCAL volume =======

fsCurPerm
   fsCurPerm (WRITE FAILED -61)
   fsRdPerm

fsRdPerm
   fsCurPerm
   fsRdPerm
   fsWrPerm
   fsRdWrPerm
   fsRdWrShPerm
   fsRdPerm + fsWrPerm + fsRdDenyPerm + fsWrDenyPerm

fsWrPerm
   fsCurPerm (WRITE FAILED -61)
   fsRdPerm

fsRdWrPerm
   fsCurPerm (WRITE FAILED -61)
   fsRdPerm

fsRdWrShPerm
   fsCurPerm (WRITE FAILED -61)
   fsRdPerm
   fsRdWrShPerm

fsRdPerm + fsWrPerm + fsRdDenyPerm + fsWrDenyPerm
   fsCurPerm (WRITE FAILED -61)
   fsRdPerm

	======= AFP volume =======

fsCurPerm
   fsCurPerm (WRITE FAILED -61)
   fsRdPerm
   fsRdWrPerm (WRITE FAILED -61)

fsRdPerm
   fsCurPerm (WRITE FAILED -61)
   fsRdPerm
   fsWrPerm
   fsRdWrPerm
   fsRdWrShPerm

fsWrPerm
   fsCurPerm (WRITE FAILED -61)
   fsRdPerm
   fsRdWrPerm (WRITE FAILED -61)

fsRdWrPerm
   fsCurPerm (WRITE FAILED -61)
   fsRdPerm
   fsRdWrPerm (WRITE FAILED -61)

fsRdWrShPerm
   fsRdWrShPerm

fsRdPerm + fsWrPerm + fsRdDenyPerm + fsWrDenyPerm
   fsCurPerm (WRITE FAILED -61)
   fsRdPerm
   fsRdWrPerm (WRITE FAILED -61)

	======= SMB volume =======

fsCurPerm
   fsCurPerm (WRITE FAILED -61)
   fsRdPerm
   fsRdWrPerm (WRITE FAILED -61)

fsRdPerm
   fsCurPerm (WRITE FAILED -61)
   fsRdPerm
   fsRdWrPerm (WRITE FAILED -61)

fsWrPerm
   fsCurPerm (WRITE FAILED -61)
   fsRdPerm
   fsRdWrPerm (WRITE FAILED -61)

fsRdWrPerm
   fsCurPerm (WRITE FAILED -61)
   fsRdPerm
   fsRdWrPerm (WRITE FAILED -61)

fsRdWrShPerm
   fsRdWrShPerm

fsRdPerm + fsWrPerm + fsRdDenyPerm + fsWrDenyPerm
   fsCurPerm (WRITE FAILED -61)
   fsRdPerm
   fsRdWrPerm (WRITE FAILED -61)

*/

OSErr XMacFile::_Open( const FSRef& inFileRef, const HFSUniStr255& inForkName, const FileAccess inFileAccess, FSIORefNum *outForkRef) const
{
	OSErr macError;
	switch( inFileAccess)
	{
		case FA_READ: 
			macError = ::FSOpenFork( &inFileRef, inForkName.length, inForkName.unicode, fsRdPerm, outForkRef);
			
			// see table above.
			// on a local volume, one should always be able to open a file in read mode.
			// on a remote volume, one can't open in read mode a file that has been opened in shared mode.
			// the only mode allowed is then shared.
			if (macError == permErr)
				macError = ::FSOpenFork( &inFileRef, inForkName.length, inForkName.unicode, fsRdWrShPerm, outForkRef);
			break;

		case FA_READ_WRITE:
			macError = ::FSOpenFork( &inFileRef, inForkName.length, inForkName.unicode, fsRdWrPerm, outForkRef);
			if (macError == noErr)
			{
				// see table above.
				// on a remote volume, one must check if one can truly write into the file
				bool remote;
				if ( (IsOnRemoteVolume( &inFileRef, &remote) == noErr) && remote)
				{
					// check writing first byte on itself
					char c;
					ByteCount n;
					OSErr err = ::FSReadFork( *outForkRef, fsFromStart, 0, 1, &c, &n);
					if ( (err == noErr) && (n == 1) )
					{
						err = ::FSWriteFork( *outForkRef, fsFromStart, 0, 1, &c, &n);
						if (err == wrPermErr)
						{
							// can't write because of lack of permission: let's close it and report appropriate error
							::FSCloseFork( *outForkRef);
							macError = permErr;
						}
						else
						{
							::FSSetForkPosition(*outForkRef, fsFromStart, 0); // ok restore file position
						}	
					}
				}
			}
			break;

		case FA_SHARED:
			macError = ::FSOpenFork( &inFileRef, inForkName.length, inForkName.unicode, fsRdWrShPerm, outForkRef);
			break;

		case FA_MAX:			// fsCurPerm is not reliable
			macError = ::FSOpenFork( &inFileRef, inForkName.length, inForkName.unicode, fsRdWrPerm, outForkRef);
			if (macError != noErr)
				macError = ::FSOpenFork( &inFileRef, inForkName.length, inForkName.unicode, fsRdPerm, outForkRef);
			break;

		default:
			xbox_assert( false);
			macError = -1;
			break;	//	Bad FileAccess
	}
	return macError;
}


VError XMacFile::Exchange(const VFile& inExhangeWith) const
{
	OSErr macError=fnfErr;
	FSRef srcRef;
	FSRef dstRef;
	
	if (_BuildFSRefFile( &srcRef) )
	{
		if(inExhangeWith.MAC_GetImpl()->_BuildFSRefFile( &dstRef))
		{
			macError=::FSExchangeObjects( &srcRef, &dstRef);
		}
	}
	
	return MAKE_NATIVE_VERROR( macError);
}
 
VError XMacFile::Open( const FileAccess inFileAccess, FileOpenOptions inOptions, VFileDesc** outFileDesc) const
{
	OSErr macError;

	FSRef fileRef;
	FSIORefNum fileForkRef;
	HFSUniStr255 forkName;

	if (inOptions & FO_OpenResourceFork)
		::FSGetResourceForkName( &forkName);
	else
		::FSGetDataForkName( &forkName);

	if (_BuildFSRefFile( &fileRef) )
	{
		// try to open
		macError = _Open( fileRef, forkName, inFileAccess, &fileForkRef);
		
		if ( (macError == noErr) && (inOptions & FO_Overwrite) )
		{
			macError = ::FSSetForkSize( fileForkRef, fsFromStart, 0);
		}
	}
	else
	{
		//try to create
		if (inOptions & FO_CreateIfNotFound)
		{
			FSRef parentRef;
			VString fileName;
		
			if ( _BuildParentFSRefFile( &parentRef, fileName) )
			{
				FSCatalogInfo	fileInfo;
				::memset( &fileInfo, 0, sizeof( fileInfo));
				
				macError = ::FSCreateFileUnicode( &parentRef, fileName.GetLength(), fileName.GetCPointer(), kFSCatInfoNone, &fileInfo, &fileRef, NULL);		

				if (inOptions & FO_OpenResourceFork)
					macError = ::FSCreateFork( &fileRef, forkName.length, forkName.unicode);

				// create and open file
				if ( macError == noErr )
				{
					macError = _Open( fileRef, forkName, inFileAccess, &fileForkRef);
				}
			}
			else
			{
				macError = fnfErr;	// should find a better error ?
			}
		}
		else
		{
			macError = fnfErr;
		}
	}

	if ( macError == noErr )
	{
		*outFileDesc = new VFileDesc( fOwner, fileForkRef, inFileAccess );
	}

	return MAKE_NATIVE_VERROR( macError);
}

void _CallBackProc( FSFileOperationRef fileOp, const FSRef *currentItem, FSFileOperationStage stage, OSStatus error, CFDictionaryRef statusDictionary, void *info )
{
	if ( stage ==  kFSOperationStageComplete )
		((VSyncEvent*)info)->Unlock();
}

VError XMacFile::Copy( const VFilePath& inDestinationPath, VFileSystem *inDestinationFileSystem, VFile** outFile, FileCopyOptions inOptions ) const
{
	// CFRunLoopGetMain is a 10.5 api that may exist in 10.4.
	// Let's load it dynamically.
#if MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_4
	if (CFRunLoopGetMain == NULL)
	{
		CFBundleRef cfBundle = ::CFBundleGetBundleWithIdentifier(CFSTR("com.apple.CoreFoundation"));
		if (testAssert( cfBundle != NULL))
		{
			CFRunLoopGetMain = (CFRunLoopGetMainProcPtr) ::CFBundleGetFunctionPointerForName(cfBundle, CFSTR("CFRunLoopGetMain"));
			xbox_assert( CFRunLoopGetMain != NULL);
		}
	}
#endif

	OSStatus macError = noErr;
	FSRef srcRef, dstRef;
	VFilePath path_destination_folder;
	VStr31 destinationFileName;
	if (_BuildFSRefFile( &srcRef))
	{
		// get destination file name
		if (inDestinationPath.IsFile())
		{
			inDestinationPath.GetFileName( destinationFileName);
		}
		else
		{
			fOwner->GetName( destinationFileName);
		}

		// get destination folder ref
		FSRef dstFolderRef;
		inDestinationPath.GetFolder( path_destination_folder);
		macError = HFSPathToFSRef( path_destination_folder.GetPath(), &dstFolderRef);
		
		if (macError == noErr)
		{
			if (FSCopyObjectSync != NULL)	// 10.4 api
			{
				OptionBits options = 0;
				if (inOptions & FCP_Overwrite)
				{
					options |= kFSFileOperationOverwrite;

					/*
						kFSFileOperationOverwrite is not honored (bug radar 7612099)
						so we delete the destination ourselves.
						FSUnlinkObject is better than FSDeleteObject but FSDeleteObject is not available in 10.4 (needed for 4D v11.x)
					*/
					if (FSMakeFSRefUnicode( &dstFolderRef, destinationFileName.GetLength(), destinationFileName.GetCPointer(), kTextEncodingUnknown, &dstRef) == noErr)
					{
						if (FSUnlinkObject != NULL)
							FSUnlinkObject( &dstRef);
						else
							FSDeleteObject( &dstRef);
					}
				}
				
				if ( VTask::GetCurrent()->IsMain() || (CFRunLoopGetMain == NULL) )
				{
					CFStringRef cfName = destinationFileName.MAC_RetainCFStringCopy();
					macError = ::FSCopyObjectSync( &srcRef, &dstFolderRef, cfName, &dstRef, options);
					::CFRelease( cfName);				
				}
				else
				{
					VSyncEvent syncEvent;

					FSFileOperationClientContext clientContext;
					clientContext.version = 0;
					clientContext.info = (void*) &syncEvent;
					clientContext.retain = NULL;
					clientContext.release = NULL;
					clientContext.copyDescription = NULL;				
				
					FSFileOperationRef fileOp = ::FSFileOperationCreate ( kCFAllocatorDefault );
					if ( testAssert(fileOp != NULL) )
					{
						// use main runloop which is the only running one.
						macError = ::FSFileOperationScheduleWithRunLoop ( fileOp, CFRunLoopGetMain(), kCFRunLoopCommonModes );
						
						if ( macError == noErr )
						{
							CFStringRef cfName = destinationFileName.MAC_RetainCFStringCopy();
							macError = ::FSCopyObjectAsync ( fileOp, &srcRef, &dstFolderRef, cfName, options, &_CallBackProc, 1/60, &clientContext );
							::CFRelease( cfName);
						}

						if ( macError == noErr )
						{
							bool executeInProcess0 = false;
							IFiberScheduler * scheduler = VTaskMgr::Get()->RetainFiberScheduler();
							if(scheduler)
								executeInProcess0 = scheduler->ExecuteInProcess0();
							
							if(executeInProcess0)
								scheduler->SetSystemCallCFRunloop(true);
							
							syncEvent.Lock();
							
							
							if(executeInProcess0)
								scheduler->SetSystemCallCFRunloop(false);
							ReleaseRefCountable(&scheduler);
							
							CFDictionaryRef dico = NULL;
							FSFileOperationStage stage = 0;
							void *returnedClientContext = 0;
							OSStatus operationStatus = ::FSFileOperationCopyStatus( fileOp, &dstRef, &stage, &macError, &dico, &returnedClientContext );
							xbox_assert( operationStatus == noErr);
							xbox_assert( returnedClientContext == clientContext.info);

							if (dico != NULL)
							{
								#if 0
								CFShow( dico);
								#endif
								CFRelease( dico);
							}
						}

						::CFRelease( fileOp );
					}
					else
					{
						macError = mFulErr;
					}
				}
			}
			else
			{
				xbox_assert( false);
				macError = -1;
			}
		}
	}

	if (outFile != NULL)
	{
		if (macError == noErr)
		{
			// unfortunately, the FSRef returned by FSFileOperationCopyStatus is sometime invalid!
			// so one can't use it.
			VFilePath path( path_destination_folder, destinationFileName);

			*outFile = new VFile( path, inDestinationFileSystem);
		}
		else
			*outFile = NULL;
	}
	
	return MAKE_NATIVE_VERROR( macError);
}


VError XMacFile::Move( const VFilePath& inDestinationPath, VFileSystem *inDestinationFileSystem, VFile** outFile, FileCopyOptions inOptions ) const
{
	OSStatus macError = noErr;
	FSRef srcRef, dstRef;
	
	// get destination file name
	VStr31 destinationFileName;
	if (inDestinationPath.IsFile())
	{
		inDestinationPath.GetFileName( destinationFileName);
	}
	else
	{
		fOwner->GetName( destinationFileName);
	}

	VFilePath path_destination_folder;
	inDestinationPath.GetFolder( path_destination_folder);

	if ( _BuildFSRefFile( &srcRef))
	{
		// get destination folder ref
		FSRef dstFolderRef;
		macError = HFSPathToFSRef( path_destination_folder.GetPath(), &dstFolderRef);
		
		if (macError == noErr)
		{
			if (FSMoveObjectSync != NULL)	// 10.4 api
			{
				OptionBits options = 0;
				if (inOptions & FCP_Overwrite)
					options |= kFSFileOperationOverwrite;

				if ( VTask::GetCurrent()->IsMain() || (CFRunLoopGetMain == NULL)  )
				{
					CFStringRef cfName = destinationFileName.MAC_RetainCFStringCopy();
					macError = ::FSMoveObjectSync( &srcRef, &dstFolderRef, cfName, &dstRef, options);
					::CFRelease( cfName);				
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
								CFStringRef cfName = destinationFileName.MAC_RetainCFStringCopy();
								macError = ::FSMoveObjectAsync ( fileOp, &srcRef, &dstFolderRef, cfName, options, &_CallBackProc, 1/60, &clientContext );
								::CFRelease( cfName);
							}
							
							if ( macError == noErr )
							{
								bool executeInProcess0 = false;
								IFiberScheduler * scheduler = VTaskMgr::Get()->RetainFiberScheduler();
								if(scheduler)
									executeInProcess0 = scheduler->ExecuteInProcess0();
								
								if(executeInProcess0)
									scheduler->SetSystemCallCFRunloop(true);
								
								syncEvent->Lock();
								
								if(executeInProcess0)
									scheduler->SetSystemCallCFRunloop(false);
								ReleaseRefCountable(&scheduler);
								
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
			}
			else
			{
				// WARNING: don't work across volumes.
				
				VFilePath path_destination = inDestinationPath;

				VFilePath path_source;
				fOwner->GetPath( path_source);
				
				// get source file name
				VStr31 sourceFileName;
				path_source.GetFileName( sourceFileName);

				if (path_destination.IsFolder())
					path_destination.SetFileName( destinationFileName);

				// check if destination exists now
				if (HFSPathToFSRef( path_destination.GetPath(), &dstRef) == noErr)
				{
					if (inOptions & FCP_Overwrite)
						macError = ::FSDeleteObject( &dstRef);
					else
						macError = dupFNErr;
				}

				if (macError == noErr)
				{
					// do we need to rename ?
					bool needRename = !sourceFileName.EqualTo( destinationFileName, true);
					
					if (!needRename)
					{
						macError = ::FSMoveObject( &srcRef, &dstFolderRef, &dstRef);
					}
					else
					{
						// first try by renaming once in destination folder
						FSRef ref_renamed_destination;
						if (::FSMoveObject( &srcRef, &dstFolderRef, &ref_renamed_destination) == noErr)
						{
							macError = ::FSRenameUnicode( &ref_renamed_destination, destinationFileName.GetLength(), destinationFileName.GetCPointer(), kTextEncodingUnknown, &dstRef);
							if (macError != noErr)
							{
								// failed to rename: put it back where it was
								VFilePath sourceFolder;
								path_source.GetFolder( sourceFolder);
								FSRef sourceFolderRef;
								if (HFSPathToFSRef( sourceFolder.GetPath(), &sourceFolderRef) == noErr)
									::FSMoveObject( &ref_renamed_destination, &sourceFolderRef, NULL);
							}
						}
						else
						{
							// we must find a temp file name that does not exist in both source and destination folders
							
							for( sLONG i = 1 ; true ; ++i)
							{
								VStr255 tempFileName;
								tempFileName.Printf( "temp_%d", i);

								path_source.SetFileName( tempFileName);
								path_destination.SetFileName( tempFileName);

								VFile file_source( path_source);
								VFile file_destination( path_destination);

								if (!file_source.Exists() && !file_destination.Exists() )
								{
									FSRef ref_renamed_source;
									macError = ::FSRenameUnicode( &srcRef, tempFileName.GetLength(), tempFileName.GetCPointer(), kTextEncodingUnknown, &ref_renamed_source);
									if (macError == noErr)
									{
										macError = ::FSMoveObject( &ref_renamed_source, &dstFolderRef, &ref_renamed_destination);
										if (macError == noErr)
										{
											macError = ::FSRenameUnicode( &ref_renamed_destination, destinationFileName.GetLength(), destinationFileName.GetCPointer(), kTextEncodingUnknown, &dstRef);
											if (macError != noErr)
											{
												// failed to rename: put it back where it was
												VFilePath sourceFolder;
												path_source.GetFolder( sourceFolder);
												FSRef sourceFolderRef;
												if (HFSPathToFSRef( sourceFolder.GetPath(), &sourceFolderRef) == noErr)
													::FSMoveObject( &ref_renamed_destination, &sourceFolderRef, &ref_renamed_source);
											}
										}
										if (macError != noErr)
										{
											// failed to move: revert to original name
											::FSRenameUnicode( &ref_renamed_source, sourceFileName.GetLength(), sourceFileName.GetCPointer(), kTextEncodingUnknown, NULL);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	else
	{
		macError = fnfErr;
	}

	if (outFile != NULL)
	{
		if (macError == noErr)
			*outFile = new VFile( VFilePath( path_destination_folder, destinationFileName), inDestinationFileSystem);
		else
			*outFile = NULL;
	}
	
	return MAKE_NATIVE_VERROR( macError);
}


// Rename the file with the given name ( foofoo.html ).
VError XMacFile::Rename( const VString& inName, VFile** outFile ) const
{
	OSErr macError = fnfErr;
	FSRef srcRef,newRef;
	
	if ( _BuildFSRefFile( &srcRef) )
		macError = ::FSRenameUnicode ( &srcRef, inName.GetLength(), inName.GetCPointer(), kTextEncodingDefaultFormat, &newRef );
	
	if (outFile != NULL)
	{
		if (macError == noErr)
		{
			VFilePath renamedPath( fOwner->GetPath());
			renamedPath.SetFileName( inName);
			*outFile = new VFile( renamedPath, fOwner->GetFileSystem());
		}
		else
			*outFile = NULL;
	}
	return MAKE_NATIVE_VERROR(macError);
}

VError XMacFile::Delete() const
{
	OSErr macErr;
	FSRef fileRef;
	if (_BuildFSRefFile( &fileRef))
	{
		// DH 05-Nov-2013 Using FSUnlinkObject instead of FSDeleteObject because it plays much better with spotlight
		// so we don't have to do a complex workaround anymore (which wasn't working so well anyway)
		macErr = ::FSUnlinkObject(&fileRef);
	}
	else
		macErr = fnfErr;
	
	return MAKE_NATIVE_VERROR( macErr);
}

VError XMacFile::MoveToTrash() const
{
	FSRef fileRef;
	_BuildFSRefFile( &fileRef);

    OptionBits options = kFSFileOperationDefaultOptions;
    
    OSStatus status;
    if (VTask::GetCurrent()->IsMain() )
    {
        status = ::FSMoveObjectToTrashSync( &fileRef, NULL, options);
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
                    status = ::FSMoveObjectToTrashAsync ( fileOp, &fileRef, options, &_CallBackProc, 1.0/60.0, &clientContext );
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


// test if the file allready exist.
bool XMacFile::Exists() const
{
	FSRef fileTest;
	return _BuildFSRefFile( &fileTest);
}

CFURLRef XMacFile::CreateCFURL() const
{
	CFURLRef theUrl = NULL;
	CFMutableStringRef	ref_hfsPath = ::CFStringCreateMutable( kCFAllocatorDefault, 0);
	if (ref_hfsPath != NULL)
	{
		const VString& hfsPath = fOwner->GetPath().GetPath();
		Boolean isDirectory = fOwner->GetPath().IsFolder();

		::CFStringAppendCharacters( ref_hfsPath, hfsPath.GetCPointer(), hfsPath.GetLength());
		::CFStringNormalize( ref_hfsPath, kCFStringNormalizationFormD);	// decomposed unicode to please CFURL
		
		theUrl = ::CFURLCreateWithFileSystemPath( NULL, ref_hfsPath, kCFURLHFSPathStyle, isDirectory);
		::CFRelease( ref_hfsPath);
	}
	return theUrl;
}


VError XMacFile::GetTimeAttributes( VTime* outLastModification, VTime* outCreationTime, VTime* outLastAccess ) const
{
	OSErr macError = fnfErr;

	FSRef fileRef;
	FSCatalogInfo catInfo;

	if ( _BuildFSRefFile( &fileRef) )
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

VError XMacFile::SetTimeAttributes( const VTime *inLastModification, const VTime *inCreationTime, const VTime *inLastAccess ) const
{
	OSErr macError = fnfErr;
	
	FSRef fileRef;
	FSCatalogInfo catInfo;
	
	if ( _BuildFSRefFile( &fileRef) )
	{
		sWORD dateTime[7];
		CFAbsoluteTime absTime;
		CFGregorianDate	gregorianDate;
		macError = ::FSGetCatalogInfo(&fileRef, kFSCatInfoAllDates, &catInfo, NULL, NULL, NULL);
		
		if ( inLastModification )
		{
			inLastModification->GetUTCTime(dateTime[0],dateTime[1],dateTime[2],dateTime[3],dateTime[4],dateTime[5],dateTime[6]);
			gregorianDate.year=dateTime[0];gregorianDate.month=dateTime[1];gregorianDate.day=dateTime[2];
			gregorianDate.hour=dateTime[3];gregorianDate.minute=dateTime[4];gregorianDate.second=dateTime[5];
			absTime = ::CFGregorianDateGetAbsoluteTime(gregorianDate, NULL);
			::UCConvertCFAbsoluteTimeToUTCDateTime(absTime,&catInfo.contentModDate);
		}
		
		if ( inCreationTime )
		{
			inCreationTime->GetUTCTime(dateTime[0],dateTime[1],dateTime[2],dateTime[3],dateTime[4],dateTime[5],dateTime[6]);
			gregorianDate.year=dateTime[0];gregorianDate.month=dateTime[1];gregorianDate.day=dateTime[2];
			gregorianDate.hour=dateTime[3];gregorianDate.minute=dateTime[4];gregorianDate.second=dateTime[5];
			absTime = ::CFGregorianDateGetAbsoluteTime(gregorianDate, NULL);
			::UCConvertCFAbsoluteTimeToUTCDateTime(absTime,&catInfo.createDate);
		}
		
		if ( inLastAccess )
		{
			inLastAccess->GetUTCTime(dateTime[0],dateTime[1],dateTime[2],dateTime[3],dateTime[4],dateTime[5],dateTime[6]);
			gregorianDate.year=dateTime[0];gregorianDate.month=dateTime[1];gregorianDate.day=dateTime[2];
			gregorianDate.hour=dateTime[3];gregorianDate.minute=dateTime[4];gregorianDate.second=dateTime[5];
			absTime = ::CFGregorianDateGetAbsoluteTime(gregorianDate, NULL);
			::UCConvertCFAbsoluteTimeToUTCDateTime(absTime,&catInfo.accessDate);
		}
		
		macError = ::FSSetCatalogInfo(&fileRef, kFSCatInfoAllDates, &catInfo);
		
	}
	
	return MAKE_NATIVE_VERROR(macError);
}

VError XMacFile::GetFileAttributes( EFileAttributes &outFileAttributes ) const
{
	FSRef fileRef;
	CFURLRef fileUrl = NULL;
	VError err = VE_OK;
	
	if ( _BuildFSRefFile( &fileRef) )
		fileUrl = ::CFURLCreateFromFSRef( NULL, &fileRef );
	
	if ( fileUrl != NULL )
	{
		CFBooleanRef  lockedFlag;
		
		const CFIndex keyCount = 2;
		const void* keys[keyCount] = {kCFURLIsHiddenKey,kCFURLIsUserImmutableKey};
		
		CFArrayRef cfKeyArray = ::CFArrayCreate(NULL,keys,keyCount,&kCFTypeArrayCallBacks);
		
		if(cfKeyArray != NULL)
		{
			CFErrorRef error = NULL;
			CFDictionaryRef cfProperties = CFURLCopyResourcePropertiesForKeys(fileUrl,cfKeyArray,&error);
			
			if(cfProperties != NULL)
			{
				CFBooleanRef val = (CFBooleanRef) CFDictionaryGetValue( cfProperties, kCFURLIsHiddenKey);
				if( (val != NULL) && CFBooleanGetValue(val))
				{
					outFileAttributes |= FATT_HidenFile;
				}
				else
				{
					outFileAttributes &= ~FATT_HidenFile;
				}
				val = (CFBooleanRef) CFDictionaryGetValue( cfProperties, kCFURLIsUserImmutableKey);
				if( (val != NULL) && CFBooleanGetValue(val))
				{
					outFileAttributes |= FATT_LockedFile;
				}
				else
				{
					outFileAttributes &= ~FATT_LockedFile;
				}
				CFRelease(cfProperties);
			}
			err = MakeVErrorFromCFErrorAnRelease(error);
			CFRelease(cfKeyArray);
		}
		else
		{
			err = VE_MEMORY_FULL;
		}
		CFRelease(fileUrl);
	}
	else
	{
		err = MAKE_NATIVE_VERROR(fnfErr);
	}
	return err;
}


VError XMacFile::SetFileAttributes( EFileAttributes inFileAttributes ) const
{
	FSRef fileRef;
	CFURLRef fileUrl = NULL;
	VError err = VE_OK;
	
	if ( _BuildFSRefFile( &fileRef) )
		fileUrl = ::CFURLCreateFromFSRef( NULL, &fileRef );
	
	if ( fileUrl != nil )
	{
		CFDictionaryRef cfProperties = NULL;
		const CFIndex keyCount = 2;
		const void* keys[] = {kCFURLIsHiddenKey,kCFURLIsUserImmutableKey};
		const void* values[] = {kCFBooleanFalse,kCFBooleanFalse};
		
		if ( inFileAttributes & FATT_HidenFile )
		{
			values[0] = kCFBooleanTrue;
		}
		
		if ( inFileAttributes & FATT_LockedFile )
		{
			values[1] = kCFBooleanTrue;
		}
		
		cfProperties = ::CFDictionaryCreate(NULL,keys,values,keyCount,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
		if(cfProperties != NULL)
		{
			CFErrorRef error = NULL;
			CFURLSetResourcePropertiesForKeys(fileUrl,cfProperties,&error);
			
			CFRelease(cfProperties);
			if(error)
			{
				err = MAKE_NATIVE_VERROR(CFErrorGetCode(error));
				CFRelease(error);
			}
		}
		else
		{
			err = VE_MEMORY_FULL;
		}
		CFRelease(fileUrl);
	}
	else
	{
		err = MAKE_NATIVE_VERROR(fnfErr);
	}
	return err;
}


OSErr XMacFile::GetFSVolumeRefNum( FSVolumeRefNum *outRefNum) const
{
	FSRef fileRef;
	OSErr macError = fnfErr;

	if ( _BuildFSRefFile( &fileRef) )
	{
		FSCatalogInfo info;
		macError = ::FSGetCatalogInfo(&fileRef, kFSCatInfoVolume, &info, NULL, NULL, NULL);
		if (macError == noErr)
		{
			*outRefNum = info.volume;
		}
	}
	return macError;
}


// return DiskFreeSpace.
VError XMacFile::GetVolumeFreeSpace(sLONG8 *outFreeBytes, sLONG8 *outTotalBytes, bool inWithQuotas ) const
{
	FSVolumeRefNum volumeRefNum;
	OSErr macError = GetFSVolumeRefNum( &volumeRefNum);

	if (macError == noErr)
	{
		FSVolumeInfo volinfo;
		macError = ::FSGetVolumeInfo( volumeRefNum, 0, NULL, kFSVolInfoSizes, &volinfo, NULL, NULL);
		if (macError == noErr)
		{
			*outFreeBytes = volinfo.freeBytes;
			if (outTotalBytes != NULL)
				*outTotalBytes = volinfo.totalBytes;
		}
	}

	return MAKE_NATIVE_VERROR(macError);
}


// reveal in finder
OSErr XMacFile::RevealFSRefInFinder( const FSRef& inRef)
{
	AppleEvent		aeEvent = {0, NULL};
	AEAddressDesc	reply = {0, NULL};
	AEAddressDesc	myAddressDesc = {0, NULL};
	AEAddressDesc	fileList = {0, NULL};
	
	const char *finderID = "com.apple.finder";
	OSErr macError = ::AECreateDesc( typeApplicationBundleID, finderID, strlen( finderID), &myAddressDesc);
	if (macError == noErr)
	{
		macError = ::AECreateAppleEvent( kAEMiscStandards, kAEMakeObjectsVisible, &myAddressDesc, kAutoGenerateReturnID, kAnyTransactionID, &aeEvent);
		if (macError == noErr)
		{
			macError = ::AECreateList( NULL, 0, false, &fileList);
			if (macError == noErr)
			{
				macError = ::AEPutPtr( &fileList, 0, typeFSRef, &inRef, sizeof(FSRef));
			   if (macError == noErr)
			   {
					macError = ::AEPutParamDesc( &aeEvent,keyDirectObject, &fileList);
					if (macError == noErr)
					{
						macError = ::AESendMessage( &aeEvent, &reply, kAEWaitReply | kAECanSwitchLayer | kAEAlwaysInteract, kAEDefaultTimeout);
						if (macError == noErr)
						{
							// bring finder to front with a second apple event
							AppleEvent aeEvent2 = {0, NULL};
							macError = ::AECreateAppleEvent( kAEMiscStandards, kAEActivate, &myAddressDesc, kAutoGenerateReturnID, kAnyTransactionID, &aeEvent2);
							if (macError == noErr)
								macError = ::AESendMessage( &aeEvent2, NULL, kAENoReply | kAECanSwitchLayer | kAEAlwaysInteract, kAEDefaultTimeout);
							AEDisposeDesc( &aeEvent2);
						}
					}
				}
			}
		}
	}
	AEDisposeDesc( &reply);
	AEDisposeDesc( &myAddressDesc);
	AEDisposeDesc( &fileList);
	AEDisposeDesc( &aeEvent);
	return macError;
}


VError XMacFile::RevealInFinder() const
{
	OSErr			macError;
	FSRef			ref;
	if ( _BuildFSRefFile( &ref) )
	{
		macError = RevealFSRefInFinder( ref);
	}
	else
	{
		macError = fnfErr;
	}

	return MAKE_NATIVE_VERROR( macError);
}


VError XMacFile::ResolveAlias( VFilePath &outTargetPath ) const
{
	FSRef fileRef;
	VError error = VE_FILE_NOT_FOUND;

	outTargetPath.Clear();
	
	if ( _BuildFSRefFile( &fileRef) )
	{
		CFErrorRef cfError = NULL;
		CFURLRef fileURL = ::CFURLCreateFromFSRef( NULL, &fileRef );
		
		//ACI0084201: Dec 9th 2013, O.R., previous implementation relied on FSResolveAliasFile() which supported symbolic link resolution.
		//The new CFURLCreateBookmarkDataFromFile API does not support resolving symbolic links so we have to support both alias and symlink resolution
		Boolean ok = FALSE;
		CFBooleanRef  cfBoolIsSymLink;
		ok = ::CFURLCopyResourcePropertyForKey(fileURL,kCFURLIsSymbolicLinkKey,&cfBoolIsSymLink,&cfError);
		if (ok)
		{
			if(CFBooleanGetValue(cfBoolIsSymLink))
			{
				char *targetPath = (char*)::malloc(PATH_MAX * 10);
				if(targetPath==NULL)
				{
					error = vThrowError(XBOX::VE_MEMORY_FULL);
				}
				else
				{
					size_t len = readlink((const char*)fPosixPath,targetPath,PATH_MAX);
					if(len ==-1)
					{
						error =  MAKE_NATIVE_VERROR(errno);
					}
					else
					{
						XBOX::VString str;
						str.FromBlock(targetPath,len,VTC_UTF_8);
						outTargetPath.FromFullPath(str,FPS_POSIX);
						error = VE_OK;
					}
					::free(targetPath);
				}
			}
			else
			{
				//Not a symbolic link so let's see if it is an alias
				CFDataRef bkm = ::CFURLCreateBookmarkDataFromFile(NULL, fileURL, &cfError);
				if(bkm != NULL)
				{
					CFURLRef targetUrl = CFURLCreateByResolvingBookmarkData ( NULL, bkm, kCFBookmarkResolutionWithoutUIMask, NULL,NULL, NULL /* isStale*/, &cfError);
					
					if(targetUrl)
					{
						FSRef targetFileRef;
						if(CFURLGetFSRef(targetUrl, &targetFileRef))
						{
							VString fullPath;
							OSErr macError = fnfErr;
							macError = FSRefToHFSPath(targetFileRef,fullPath);
							if ( macError == noErr )
							{
								if(CFURLHasDirectoryPath(targetUrl))
								{
									fullPath += FOLDER_SEPARATOR;
								}
								outTargetPath.FromFullPath(fullPath);
								error = VE_OK;
							}
							else
							{
								error =  MAKE_NATIVE_VERROR(macError);
							}
						}
						CFRelease(targetUrl);
					}
					else if(cfError != NULL)
					{
						error = MakeVErrorFromCFErrorAnRelease(cfError);
						cfError = NULL;
					}
					CFRelease(bkm);
				}
				else if(cfError != NULL)
				{
					error = MakeVErrorFromCFErrorAnRelease(cfError);
					cfError = NULL;
				}
			}
		}
		else if (cfError != NULL)
		{
			error = MakeVErrorFromCFErrorAnRelease(cfError);
			cfError = NULL;
		}
		
		if(cfBoolIsSymLink!= NULL)
			CFRelease(cfBoolIsSymLink);
		
		if(fileURL != NULL)
			CFRelease(fileURL);
	}
	return error;
}

VError XMacFile::CreateAlias( const VFilePath& inTargetPath, const VFilePath *inRelativeAliasSource) const
{
	VError error = VE_OK;
	FSRef srcFileRef,dstAliasRef;
	
	{
		OSErr macError = fnfErr;
		if ( _BuildFSRefFile( &srcFileRef) )
			macError = ::FSDeleteObject(&srcFileRef);
		else
			macError = noErr;
	
		if ( macError == noErr )
			macError = HFSPathToFSRef(inTargetPath.GetPath(), &dstAliasRef);
		
		if ( macError != noErr )
			error = MAKE_NATIVE_VERROR(macError);
	}
	
	
	if ( error == VE_OK)
	{
		//create from URL because srcFileRef is invalid if alias file is non-existent (e.g. previously deleted)
		CFURLRef aliasFileUrl = CreateCFURL();
		CFURLRef aliasTargetUrl = ::CFURLCreateFromFSRef( NULL, &dstAliasRef );
		
		if(aliasFileUrl == NULL || aliasTargetUrl == NULL)
			error = vThrowError(VE_MEMORY_FULL);
		
		if ( error == VE_OK)
		{
			CFDataRef bkm = NULL;
			CFErrorRef cfError = NULL;
		
			bkm = ::CFURLCreateBookmarkData(NULL,aliasTargetUrl,kCFURLBookmarkCreationSuitableForBookmarkFile,NULL,NULL,&cfError);
			if(bkm != NULL)
			{
				Boolean b = ::CFURLWriteBookmarkDataToFile(bkm,aliasFileUrl,kCFURLBookmarkCreationSuitableForBookmarkFile,&cfError);
				if(b)
				{
					error = VE_OK;
				}
				else if (cfError != NULL)
				{
					error = MakeVErrorFromCFErrorAnRelease(cfError);
					cfError = NULL;
				}
				CFRelease(bkm);
			}
			else if (cfError != NULL)
			{
				error = MakeVErrorFromCFErrorAnRelease(cfError);
				cfError = NULL;
			}
		}
		if(aliasFileUrl != NULL)
			CFRelease(aliasFileUrl);
		
		if(aliasTargetUrl != NULL)
			CFRelease(aliasTargetUrl);
	}
	return error;
}


bool XMacFile::IsAliasFile() const
{
	FSRef fileRef;
	Boolean	isAlias = false;
	
	if ( _BuildFSRefFile( &fileRef) )
	{
		CFErrorRef cfError = NULL;
		CFURLRef fileURL = ::CFURLCreateFromFSRef( NULL, &fileRef );
			
		//ACI0084201: Dec 9th 2013, O.R., previous implementation relied on FSIsAliasFile() which supported symbolic link resolution.
		//The new CFURLCreateBookmarkDataFromFile API does not support resolving symbolic links so we have to support both alias and symlink resolution
		Boolean ok = FALSE;
		CFBooleanRef  cfBoolIsSymLink;
		ok = ::CFURLCopyResourcePropertyForKey(fileURL,kCFURLIsSymbolicLinkKey,&cfBoolIsSymLink,&cfError);
		if (ok)
		{
			if(CFBooleanGetValue(cfBoolIsSymLink))
			{
				isAlias = true;
			}
			else
			{
				CFDataRef bkm = ::CFURLCreateBookmarkDataFromFile(NULL, fileURL, &cfError);
				if(cfError != NULL)
				{
					CFRelease(cfError);
					isAlias = false;
				}
				else if(bkm != NULL)
				{
					isAlias = true;
					CFRelease(bkm);
				}
			}
		}
		else if(cfError != NULL)
		{
			CFRelease(cfError);
			isAlias = false;
		}
		
		if(cfBoolIsSymLink!= NULL)
			CFRelease(cfBoolIsSymLink);
		
		if( fileURL != NULL)
			CFRelease(fileURL);
	}
	return isAlias;
}

bool XMacFile::_BuildFSRefFile( FSRef *outFileRef) const
{
	if (fPosixPath == NULL)
	{
		// builds UTF8 posix path now and keep it (VFile path can't change)
		VString fullPath;
		fOwner->GetPath( fullPath);

		UInt8 *path = PosixPathFromHFSPath( fullPath, false);

		xbox_assert( path != NULL);
		
		// remember that VFile must be thread-safe
		if (VInterlocked::CompareExchangePtr( (void**)&fPosixPath, NULL, path) != NULL)
			::free( path);
	}

	if (fPosixPath == NULL)
		return false;

	Boolean isDirectory;
	OSStatus macError = ::FSPathMakeRefWithOptions( fPosixPath, kFSPathMakeRefDoNotFollowLeafSymlink, outFileRef, &isDirectory);
	return (macError == noErr) && !isDirectory;
}


bool XMacFile::_BuildParentFSRefFile( FSRef *outParentRef, VString &outFileName ) const
{
	fOwner->GetName(outFileName);

	VFilePath parentPath;
	fOwner->GetPath(parentPath);
	return HFSPathToFSRef(parentPath.ToParent().GetPath(), outParentRef) == noErr;
}

/*
	static
*/
OSErr XMacFile::HFSPathToFSRef(const VString& inPath, FSRef *outRef)
{
	OSErr macErr = paramErr;

	CFMutableStringRef	stringRef = ::CFStringCreateMutable( kCFAllocatorDefault, 0);
	if (stringRef != NULL)
	{
		::CFStringAppendCharacters( stringRef, inPath.GetCPointer(), inPath.GetLength());
		::CFStringNormalize( stringRef, kCFStringNormalizationFormD);	// decomposed unicode to please CFURL

		CFURLRef	urlRef = ::CFURLCreateWithFileSystemPath(NULL, stringRef, kCFURLHFSPathStyle, false);
		if (urlRef != NULL)
		{
			if (::CFURLGetFSRef(urlRef, outRef))
				macErr = noErr;
			::CFRelease(urlRef);
		}
		::CFRelease(stringRef);
	}

	return macErr;
}

/*
	static
*/

OSErr XMacFile::FSRefToHFSPath( const FSRef &inRef, VString& outPath)
{
	FSRef parentRef = inRef;
	OSErr	macErr;
	FSCatalogInfo	catInfo;
	CFMutableStringRef cfPath = NULL;
	do {
		HFSUniStr255	name;
		macErr = ::FSGetCatalogInfo( &parentRef, kFSCatInfoNodeID, &catInfo, &name, NULL, &parentRef);
		if (macErr == noErr) 
		{
			CFStringRef cfName = ::CFStringCreateWithCharactersNoCopy( NULL, name.unicode, name.length, kCFAllocatorNull);
			if (cfName != NULL)
			{
				if (cfPath == NULL)
					cfPath = ::CFStringCreateMutableCopy( kCFAllocatorDefault, 0, cfName);
				else
				{
					::CFStringInsert( cfPath, 0, CFSTR( ":"));
					::CFStringInsert( cfPath, 0, cfName);
				}
				::CFRelease( cfName);
			}
		}
	} while (macErr == noErr && catInfo.nodeID != fsRtDirID);

	if (cfPath != NULL)
	{
		::CFStringNormalize( cfPath, kCFStringNormalizationFormC); // on recompose
		outPath.MAC_FromCFString( cfPath);
		CFRelease(cfPath);
	}
	else
	{
		outPath.Clear();
	}

	return macErr;
}

/*
	static
*/
#if !__LP64__
OSErr XMacFile::HFSPathToFSSpec (const VString& inPath, FSSpec *outSpec)
{
	FSRef fileRef;
	OSErr macErr = HFSPathToFSRef( inPath, &fileRef );
	if (macErr == noErr )
	{
		macErr = FSRefToFSSpec(fileRef, outSpec);
	}
	else
	{
		outSpec->vRefNum = kFSInvalidVolumeRefNum;
		outSpec->parID = 0;
		outSpec->name[0] = 0;
		
		// Try using parent's FSRef
		VFilePath path( inPath);

		VStr255	objectName;
		path.GetName( objectName);

		path.ToParent();
		
		macErr = HFSPathToFSRef(path.GetPath(), &fileRef);
		if (macErr == noErr)
		{
			FSCatalogInfo	parentInfo;
			macErr = ::FSGetCatalogInfo(&fileRef, kFSCatInfoVolume | kFSCatInfoNodeID, &parentInfo, NULL, NULL, NULL);
			if (testAssert(macErr == noErr))
			{
				outSpec->vRefNum = parentInfo.volume;
				outSpec->parID = parentInfo.nodeID;
				objectName.MAC_ToMacPString(outSpec->name, sizeof(outSpec->name));
				
				xbox_assert(objectName.GetLength() < 32);	// Caution FSSpec should never use name longer than 31 chars
			}
		}
	}

	return macErr;
}

/*
	static
*/
OSErr XMacFile::FSSpecToFSRef (const FSSpec& inSpec, FSRef *outRef)
{
	return ::FSpMakeFSRef(&inSpec, outRef);
}


/*
	static
*/
OSErr XMacFile::FSRefToFSSpec (const FSRef& inRef, FSSpec *outSpec)
{
	OSErr macErr = ::FSGetCatalogInfo(&inRef, kFSCatInfoNone, NULL, NULL, outSpec, NULL);
	
	if (macErr != noErr)
	{
		outSpec->vRefNum = kFSInvalidVolumeRefNum;
		outSpec->parID = 0;
		outSpec->name[0] = 0;
	}

	return macErr;
}


/*
	static
*/
OSErr XMacFile::FSSpecToHFSPath(const FSSpec& inSpec, VString& outPath)
{
	OSErr	macErr;
	
	// Try getting an FSRef (if object exists)
	FSRef	objectRef;
	if (FSSpecToFSRef(inSpec, &objectRef) == noErr)
	{
		macErr = FSRefToHFSPath(objectRef, outPath);
	}
	else
	{
		// Root directory is a special case, however it's allways handled using FSRef
		xbox_assert(inSpec.parID != fsRtParID);
		
		// Get parent's spec (as inSpec may point to an inexistant object)
		FSSpec	folderSpec;
		macErr = ::FSMakeFSSpec(inSpec.vRefNum, inSpec.parID, "\p", &folderSpec);
		if (macErr == noErr)
		{
			// Create parent's FSRef
			FSRef	folderRef;
			macErr = ::FSpMakeFSRef(&folderSpec, &folderRef);
			if (macErr == noErr)
			{
				
				// Iterate parents and construct the path
				VStr255	objectStr;
				outPath.MAC_FromMacPString(inSpec.name);
				
				FSCatalogInfo	objectInfo;
				HFSUniStr255	objectName;
				do {
					macErr = ::FSGetCatalogInfo(&folderRef, kFSCatInfoNodeID, &objectInfo, &objectName, NULL, &folderRef);
					if (macErr == noErr)
					{
						objectStr.FromBlock( objectName.unicode, objectName.length*2, VTC_UTF_16);
						objectStr.AppendUniChar(FOLDER_SEPARATOR);
						outPath.Insert(objectStr, 1);
					}
				} while (objectInfo.nodeID != fsRtDirID && macErr == noErr);
			}
		}
	}
		
	if (macErr != noErr)
		outPath.Clear();
	
	return macErr;
}

#endif // #if !__LP64__


VError XMacFile::GetSize( sLONG8 *outSize ) const
{
	OSErr macError = fnfErr;
	FSRef fileRef;
	
	if ( _BuildFSRefFile( &fileRef) )
	{
		FSCatalogInfo fileInfo;
		macError = ::FSGetCatalogInfo(&fileRef, kFSCatInfoDataSizes, &fileInfo, NULL, NULL, NULL);
		if ( macError == noErr )
		{
			*outSize = (sLONG8) fileInfo.dataLogicalSize;
		}
	}

	return MAKE_NATIVE_VERROR(macError);
}


VError XMacFile::GetResourceForkSize ( sLONG8 *outSize ) const
{
	OSErr macError = fnfErr;
	FSRef fileRef;
	
	if ( _BuildFSRefFile( &fileRef) )
	{
		FSCatalogInfo fileInfo;
		macError = ::FSGetCatalogInfo( &fileRef, kFSCatInfoRsrcSizes, &fileInfo, NULL, NULL, NULL);
		if ( macError == noErr )
		{
			*outSize = (sLONG8) fileInfo.rsrcLogicalSize;
		}
	}

	return MAKE_NATIVE_VERROR(macError);
}

			
bool XMacFile::HasResourceFork() const
{
	bool gotOne = false;
	FSRef fileRef;
	
	if ( _BuildFSRefFile( &fileRef) )
	{
		FSCatalogInfo fileInfo;
		OSErr macError = ::FSGetCatalogInfo( &fileRef, kFSCatInfoRsrcSizes, &fileInfo, NULL, NULL, NULL);
		gotOne = ( macError == noErr ) && (fileInfo.rsrcLogicalSize != 0);
	}
	return gotOne;
}


VError XMacFile::GetKind (OsType *outKind) const
{
	OSErr macError = fnfErr;
	FSRef fileRef;
	
	if ( _BuildFSRefFile( &fileRef) )
	{
		FSCatalogInfo fileInfo;
		macError = ::FSGetCatalogInfo( &fileRef, kFSCatInfoFinderInfo, &fileInfo, NULL, NULL, NULL);
		if ( macError == noErr )
		{
			*outKind = ((FileInfo*) fileInfo.finderInfo)->fileType;
		}
	}

	return MAKE_NATIVE_VERROR(macError);
}

VError XMacFile::SetKind ( OsType inKind ) const
{
	OSErr macError = fnfErr;
	FSRef fileRef;
	
	if ( _BuildFSRefFile( &fileRef) )
	{
		FSCatalogInfo fileInfo;
		macError = ::FSGetCatalogInfo(&fileRef, kFSCatInfoFinderInfo, &fileInfo, NULL, NULL, NULL);
		if ( macError == noErr )
		{
			((FileInfo*) fileInfo.finderInfo)->fileType = inKind;
			macError = ::FSSetCatalogInfo( &fileRef, kFSCatInfoFinderInfo, &fileInfo );
		}
	}

	return MAKE_NATIVE_VERROR(macError);
}

VError XMacFile::GetCreator ( OsType *outCreator ) const
{
	OSErr macError = fnfErr;
	FSRef fileRef;
	
	if ( _BuildFSRefFile( &fileRef) )
	{
		FSCatalogInfo fileInfo;
		macError = ::FSGetCatalogInfo(&fileRef, kFSCatInfoFinderInfo, &fileInfo, NULL, NULL, NULL);
		if ( macError == noErr )
		{
			*outCreator = ((FileInfo*) fileInfo.finderInfo)->fileCreator;
		}
	}

	return MAKE_NATIVE_VERROR(macError);
}

VError XMacFile::SetCreator ( OsType inCreator ) const
{
	OSErr macError = fnfErr;
	FSRef fileRef;
	
	if ( _BuildFSRefFile( &fileRef) )
	{
		FSCatalogInfo fileInfo;
		macError = ::FSGetCatalogInfo(&fileRef, kFSCatInfoFinderInfo, &fileInfo, NULL, NULL, NULL);
		if ( macError == noErr )
		{
			((FileInfo*) fileInfo.finderInfo)->fileCreator = inCreator;
			macError = ::FSSetCatalogInfo( &fileRef, kFSCatInfoFinderInfo, &fileInfo );
		}
	}

	return MAKE_NATIVE_VERROR(macError);
}

void XMacFile::SetExtensionVisible( bool inShow )
{
	OSStatus macStatus;
	FSRef fileRef;
	
	if ( _BuildFSRefFile( &fileRef) )
	{
		macStatus = ::LSSetExtensionHiddenForRef(&fileRef,inShow);
	}
}

bool XMacFile::GetIconFile( IconRef &outIconRef ) const
{
	FSRef fileRef;
	outIconRef = NULL;
	OSStatus macStatus = noErr;
		
	if ( _BuildFSRefFile( &fileRef) )
	{
		HFSUniStr255 fileName;
		FSCatalogInfo fileInfo;

		macStatus = ::FSGetCatalogInfo( &fileRef, kIconServicesCatalogInfoMask, &fileInfo, &fileName, NULL, NULL );
		if ( macStatus == noErr )
		{
			macStatus = ::GetIconRefFromFileInfo ( &fileRef, fileName.length, fileName.unicode, kIconServicesCatalogInfoMask, &fileInfo, kIconServicesNormalUsageFlag, &outIconRef, NULL );
		}
	}
	return ( macStatus == noErr );
}

void XMacFile::SetFinderComment( VString& inString )
{
	xbox_assert(false);
}

void XMacFile::GetFinderComment( VString& outString )
{
	xbox_assert(false);
}


VError XMacFile::RetainUTIString( CFStringRef *outUTI) const
{
	CFStringRef cfUTI = NULL;

	OSStatus macError = noErr;
	FSRef fileRef;
	
	if ( _BuildFSRefFile( &fileRef) )
	{
		macError = ::LSCopyItemAttribute( &fileRef, kLSRolesAll, kLSItemContentType, (CFTypeRef*) outUTI);
		xbox_assert( macError == noErr);
	}
	else
	{
		*outUTI = NULL;
		macError = fnfErr;
	}
	return MAKE_NATIVE_VERROR(macError);
}


VError XMacFile::GetUTIString( VString& outUTI ) const
{
	CFStringRef itemUTI;
	VError err = RetainUTIString( &itemUTI);
	if (err == VE_OK)
	{
		outUTI.MAC_FromCFString(itemUTI);
		::CFRelease( itemUTI );
	}
	else
	{
		outUTI.Clear();
	}
	return err;
}


VError XMacFile::MakeWritableByAll()
{
	return SetPermissions(0666);	// 0666 b/c in practice what we want is in fact 'readable and writable by all' - m.c, ACI0053479
}


bool XMacFile::IsWritableByAll()
{
	uWORD permFile = 0;
	GetPermissions( &permFile);
	
	return (permFile & 0222) != 0;
}


VError XMacFile::SetPermissions( uWORD inMode )
{
	OSErr macError = fnfErr;
	FSRef fileRef;
	
	if ( _BuildFSRefFile( &fileRef) )
	{
		FSCatalogInfo fileInfo;
		macError = ::FSGetCatalogInfo(&fileRef, kFSCatInfoPermissions, &fileInfo, NULL, NULL, NULL);
		if ( macError == noErr )
		{
			#if __LP64__
			fileInfo.permissions.mode |= inMode;
			#else
			((FSPermissionInfo*) &fileInfo.permissions)->mode |= inMode;
			#endif
			macError = ::FSSetCatalogInfo( &fileRef, kFSCatInfoPermissions, &fileInfo );
		}
	}

	return MAKE_NATIVE_VERROR(macError);
}


VError XMacFile::GetPermissions( uWORD *outMode )
{
	OSErr macError = fnfErr;
	FSRef fileRef;
	
	if ( _BuildFSRefFile( &fileRef) )
	{
		FSCatalogInfo fileInfo;
		macError = ::FSGetCatalogInfo(&fileRef, kFSCatInfoPermissions, &fileInfo, NULL, NULL, NULL);
		if ( macError == noErr )
		{
			#if __LP64__
			*outMode = fileInfo.permissions.mode;
			#else
			*outMode = ((FSPermissionInfo*) &fileInfo.permissions)->mode;
			#endif
		}
	}

	return MAKE_NATIVE_VERROR(macError);
}


VFileKind *XMacFile::CreatePublicFileKind( const VString& inID)
{
	// tries to build a public VFileKind
	VFileKind *fileKind = NULL;

	CFStringRef cfUTI = inID.MAC_RetainCFStringCopy();
	
	// sc 11/01/2011 retreive the UTI informations from the dictionary
	CFDictionaryRef utiDictionary = UTTypeCopyDeclaration( cfUTI);
	if (utiDictionary != NULL)
	{
		VString description;
		VectorOfFileExtension extensions;
		VectorOfFileKind osKind, parentIDs;
		
		// Get the description
		CFStringRef cfDescription = (CFStringRef) CFDictionaryGetValue( utiDictionary, kUTTypeDescriptionKey);
		if (cfDescription != NULL)
		{
			description.MAC_FromCFString( cfDescription);
		}
		
		// Get the extensions list
		CFDictionaryRef cfDictionary = (CFDictionaryRef) CFDictionaryGetValue( utiDictionary, kUTTypeTagSpecificationKey);
		if (cfDictionary != NULL)
		{
			CFTypeRef cfTypeRef = (CFTypeRef) CFDictionaryGetValue (cfDictionary, kUTTagClassFilenameExtension);
			if (cfTypeRef != NULL)
			{
				if (CFGetTypeID( cfTypeRef) == CFStringGetTypeID())
				{
					XBOX::VString extension;
					extension.MAC_FromCFString( (CFStringRef) cfTypeRef);
					extensions.push_back( extension);
				}
				else if (CFGetTypeID( cfTypeRef) == CFArrayGetTypeID())
				{
					XBOX::VString extension;
					CFArrayRef cfExtensions = (CFArrayRef) cfTypeRef;
					for (CFIndex i = 0 ; i < CFArrayGetCount(cfExtensions) ; ++i)
					{
						CFStringRef cfExtension = (CFStringRef) CFArrayGetValueAtIndex( cfExtensions, i);
						if (cfExtension != NULL)
						{
							extension.MAC_FromCFString( cfExtension);
							extensions.push_back( extension);
						}
					}
				}
				else
				{
					assert(false);
				}

			}
		}
		
		// Get the parent UTIs list
		CFTypeRef cfTypeRef = (CFTypeRef) CFDictionaryGetValue( utiDictionary, kUTTypeConformsToKey);
		if (cfTypeRef != NULL)
		{
			if (CFGetTypeID( cfTypeRef) == CFStringGetTypeID())
			{
				XBOX::VString parentID;
				parentID.MAC_FromCFString( (CFStringRef) cfTypeRef);
				parentIDs.push_back( parentID);				
			}
			else if (CFGetTypeID( cfTypeRef) == CFArrayGetTypeID())
			{
				CFArrayRef cfParentUTIs = (CFArrayRef) cfTypeRef;
				if (cfParentUTIs != NULL)
				{
					XBOX::VString parentID;
					for (CFIndex i = 0 ; i < CFArrayGetCount(cfParentUTIs) ; ++i)
					{
						CFStringRef cfParentUTI = (CFStringRef) CFArrayGetValueAtIndex( cfParentUTIs, i);
						if (cfParentUTI != NULL)
						{
							parentID.MAC_FromCFString( cfParentUTI);
							parentIDs.push_back( parentID);
						}
					}
				}
			}
			else
			{
				assert(false);
			}
		}	 
		
		fileKind = VFileKind::Create( inID, osKind, description, extensions, parentIDs, true);
		
		CFRelease( utiDictionary);
	}

	if (cfUTI)
		::CFRelease( cfUTI);
	
	return fileKind;
}

VFileKind* XMacFile::CreatePublicFileKindFromOsKind( const VString& inOsKind )
{
	VString fileUTI;
	OSType osType = 0;
	inOsKind.ToBlock((char*)&osType, 4, VTC_StdLib_char, false, false);
#if !BIGENDIAN
	assert_compile( sizeof( osType) == 4);
//		ByteSwapLong(&osType);
#endif
	CFStringRef stringOSType = ::UTCreateStringForOSType( osType );
	if (stringOSType != NULL)
	{
		CFStringRef stringRefUTI = ::UTTypeCreatePreferredIdentifierForTag(kUTTagClassOSType, stringOSType, NULL);
		if (stringRefUTI != NULL)
		{
			fileUTI.MAC_FromCFString(stringRefUTI);
			::CFRelease( stringRefUTI );
		}
		::CFRelease( stringOSType );
	}
	return fileUTI.IsEmpty() ? NULL : XMacFile::CreatePublicFileKind(fileUTI);
}


VFileKind* XMacFile::CreatePublicFileKindFromExtension( const VString& inExtension )
{
	VString fileUTI;

	CFStringRef stringExtension = inExtension.MAC_RetainCFStringCopy();
	if (stringExtension != NULL)
	{
		CFStringRef stringRefUTI = ::UTTypeCreatePreferredIdentifierForTag( kUTTagClassFilenameExtension, stringExtension, NULL);
		fileUTI.MAC_FromCFString(stringRefUTI);
		if (stringRefUTI != NULL)
			::CFRelease( stringRefUTI );
		::CFRelease( stringExtension );
	}

	return fileUTI.IsEmpty() ? NULL : XMacFile::CreatePublicFileKind( fileUTI);
}


VError XMacFile::ConformsTo( const VFileKind& inKind, bool *outConformant) const
{
	OSStatus macError;
	
	FSRef fileRef;
	if ( _BuildFSRefFile( &fileRef) )
	{
		if (inKind.IsPublic())
		{
			CFStringRef cfFileUTI;
			macError = ::LSCopyItemAttribute( &fileRef, kLSRolesAll, kLSItemContentType, (CFTypeRef*) &cfFileUTI);
			if (macError == noErr)
			{
				CFStringRef cfUTI = inKind.GetID().MAC_RetainCFStringCopy();
				*outConformant = ::UTTypeConformsTo( cfFileUTI, cfUTI) != 0;
				::CFRelease( cfUTI );
				::CFRelease( cfFileUTI );
				
				if (!*outConformant)
				{
					// in some case, utis are ambiguous.
					// for example .dif can be both a database file format and a dv-movie file.
					// unfortunately LSCopyItemAttribute returns one uti or another.
					// so if conformance failed and the file has an extension, we check it here.
					XBOX::VString extension;
					fOwner->GetExtension( extension);
					*outConformant = !extension.IsEmpty() && inKind.MatchExtension( extension);
				}
			}
		}
		else
		{
			// check extension.
			// note: on pourrait se contenter de prendre l'extension dans le file path mais ici on check que le fichier existe vraiment.
			CFStringRef cfExtension;
			macError = ::LSCopyItemAttribute( &fileRef, kLSRolesAll, kLSItemExtension, (CFTypeRef*) &cfExtension);
			if (macError == noErr)
			{
				VStr63 extension;
				extension.MAC_FromCFString( cfExtension);
				*outConformant = inKind.MatchExtension( extension);
				::CFRelease( cfExtension);
			}
		}
	}
	else
	{
		macError = fnfErr;
	}

	return MAKE_NATIVE_VERROR(macError);
}


bool XMacFile::IsFileKindConformsTo( const VFileKind& inFirstKind, const VFileKind& inSecondKind)
{
	bool result = false;
	CFStringRef firstUTI = inFirstKind.GetID().MAC_RetainCFStringCopy();
	CFStringRef secondUTI = inSecondKind.GetID().MAC_RetainCFStringCopy();
	
	if (firstUTI != NULL && secondUTI != NULL)
	{
		result =::UTTypeConformsTo( firstUTI, secondUTI);
	}
	
	if (firstUTI != NULL)
		::CFRelease( firstUTI);
	
	if (secondUTI != NULL)
		::CFRelease( secondUTI);
	
	return result;
}


VError XMacFile::GetPosixPath( VString& outPath) const
{
	OSStatus macError;

	FSRef fileRef;
	if ( _BuildFSRefFile( &fileRef) && testAssert( fPosixPath != NULL) )
	{
		outPath.FromBlock( fPosixPath, (VSize) ::strlen( (const char *) fPosixPath), VTC_UTF_8);
		
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


/* ----- XMacFileIterator ----- */



XMacFileIterator::XMacFileIterator( const VFolder *inFolder, FileIteratorOptions inOptions)
: fOptions( inOptions)
, fFileSystem( inFolder->GetFileSystem())
{
	fIndex = 0;
	fIterator = NULL;

	if (inFolder->MAC_GetFSRef( &fFolderRef))
	{
		FSIteratorFlags options = kFSIterateFlat;
		if ( inOptions & FI_ITERATE_DELETE )
			options |= kFSIterateDelete;
		OSErr macErr = ::FSOpenIterator( &fFolderRef, options, &fIterator);
		xbox_assert( macErr == noErr);
	}
}


XMacFileIterator::~XMacFileIterator()
{
	if (fIterator != NULL)
		::FSCloseIterator(fIterator);
}


VFile* XMacFileIterator::Next()
{
	VFile *result = _GetCatalogInfo();
	if (result != NULL)
		fIndex++;
	
	return result;
}


VFile* XMacFileIterator::_GetCatalogInfo ()
{
	VFile*		object = NULL;
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
				
				// skip invisible if asked
				bool	skip = false;
				if ( (fOptions & FI_WANT_INVISIBLES) == 0)
				{
					LSItemInfoRecord info;
					error = ::LSCopyItemInfoForRef( &objectRef, kLSRequestBasicFlagsOnly, &info);
					xbox_assert( error == noErr);
					skip = (error == noErr) && ( (info.flags & kLSItemInfoIsInvisible) != 0);
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
				
				if ( (error == noErr) && !skip)
				{
					if (!isFolder && (fOptions & FI_WANT_FILES))
					{
						VString objectFullPath;
						if (testAssert( XMacFile::FSRefToHFSPath( objectRef, objectFullPath) == noErr))
						{
							VFilePath objectPath( objectFullPath);
							xbox_assert( objectPath.IsFile());

							// check not to get out of file system
							if ( (fFileSystem == NULL) || objectPath.IsChildOf( fFileSystem->GetRoot()) )
								object = new VFile( objectPath, fFileSystem);
						}
					}
				}
			}
			else
			{
				xbox_assert(actualCount == 0);
				xbox_assert( (error == errFSNoMoreItems) || (error == afpAccessDenied) );
			}
		} while( (object == NULL) && (error != errFSNoMoreItems)  && (error != afpAccessDenied));//[MI] le 23/11/2010 ACI0068754 
	}
	
	return object;
}
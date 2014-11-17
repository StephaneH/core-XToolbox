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
#include "VLibrary.h"

#include <dlfcn.h>

bool XMacLibrary::Load( const VFilePath &inFilePath)
{
	assert( fBundle == NULL);
	
	#if !ARCH_64
	if (inFilePath.IsFile())
	{
		VFile theFile( inFilePath);
		FSRef fsref;
		if (theFile.MAC_GetFSRef( &fsref))
		{
			FSSpec spec;
			OSErr macErr = ::FSGetCatalogInfo( &fsref, kFSCatInfoNone, NULL, NULL, &spec, NULL);

			// In order to debug shared libs, CW requires that the fragment name is specified
			// It's also suggested to set the same fragment name as the file name.
			Str63	fragName = "\p";
		#if VERSIONDEBUG
			::memmove( fragName, spec.name, spec.name[0] + 1);
		#endif
			
			Str255	errName;
			Ptr		mainAddr;
			OSErr	err = ::GetDiskFragment(&spec, 0, kCFragGoesToEOF, fragName, kPrivateCFragCopy, &fFragConnectionID, &mainAddr, errName);

			if (err != noErr)
			{
				StThrowFileError errThrow( inFilePath, VE_LIBRARY_CANNOT_LOAD, VNativeError( err));

				VString reason;
				reason.MAC_FromMacPString( errName);
				errThrow->SetString( "reason", reason);
			}
		}
	}
	else
	#endif
	if (inFilePath.IsFolder())
	{
		VFolder theFolder( inFilePath);
		FSRef fsref;
		if (theFolder.MAC_GetFSRef( &fsref))
		{
			CFURLRef libURL = ::CFURLCreateFromFSRef(kCFAllocatorDefault, &fsref);
			fBundle = ::CFBundleCreate(kCFAllocatorDefault, libURL);
			
			::CFRelease(libURL);

			if (fBundle != NULL)
			{
				// see http://lists.apple.com/archives/xcode-users/2006/Feb/msg00234.html
				// that should be optionnal not to do it on non-4D bundles.
				_dlopen_RTLD_GLOBAL( inFilePath);

				Boolean isOK = ::CFBundleLoadExecutable( fBundle);
				
				if (!isOK)
				{
					StThrowFileError errThrow( inFilePath, VE_LIBRARY_CANNOT_LOAD);
					::CFRelease(fBundle);
					fBundle = NULL;
				}
			}
		}
	}
	
	#if ARCH_64
	return 	(fBundle != NULL);
	#else
	return 	(fFragConnectionID != NULL || fBundle != NULL);
	#endif
}


void XMacLibrary::_dlopen_RTLD_GLOBAL( const VFilePath &inFilePath)
{
	assert( fHandle_dlopen == NULL);
	
	char *posix_path_utf8 = NULL;
	CFURLRef urlRef = ::CFBundleCopyExecutableURL( fBundle);
	if (urlRef != NULL)
	{
		CFURLRef absoluteUrl = CFURLCopyAbsoluteURL( urlRef);
		if (absoluteUrl != NULL)
		{
			CFStringRef ref_posixPath = ::CFURLCopyFileSystemPath( absoluteUrl, kCFURLPOSIXPathStyle);
			if (ref_posixPath != NULL)
			{
				// get UTF8 characters

				CFRange range = {0,CFStringGetLength( ref_posixPath)};
				
				// get needed bytes
				CFIndex usedBufLen;
				CFIndex converted = ::CFStringGetBytes( ref_posixPath, range, kCFStringEncodingUTF8, 0 /* lossByte*/, false /*isExternalRepresentation*/, NULL, 0, &usedBufLen);
				posix_path_utf8 = (char *) malloc( usedBufLen + 1);	// add trailing zero
				if (posix_path_utf8 != NULL)
				{
					CFIndex maxBufLen = usedBufLen;
					converted = ::CFStringGetBytes( ref_posixPath, range, kCFStringEncodingUTF8, 0 /* lossByte*/, false /*isExternalRepresentation*/, (UInt8*) posix_path_utf8, maxBufLen, &usedBufLen);
					assert( usedBufLen == maxBufLen);
					assert( converted == range.length);
					posix_path_utf8[usedBufLen] = 0;
				}
				::CFRelease( ref_posixPath);
			}
			::CFRelease( absoluteUrl);
		}
		::CFRelease( urlRef);
	}
	
	if (posix_path_utf8 != NULL)
	{
		fHandle_dlopen = dlopen( posix_path_utf8, RTLD_GLOBAL);
		if (fHandle_dlopen == NULL)
		{
			StThrowFileError errThrow( inFilePath, VE_LIBRARY_CANNOT_LOAD);

			VString reason;
			reason.FromCString( dlerror());
			errThrow->SetString( "reason", reason);
		}
		free((void*) posix_path_utf8);
	}
	
}


void XMacLibrary::Unload()
{
	OSStatus err = noErr;
	
	#if !ARCH_64
	if (fFragConnectionID != NULL)
	{
		err = ::CloseConnection(&fFragConnectionID);
		fFragConnectionID = NULL;
	}
	#endif	
	
	if (fBundle != NULL)
	{
		::CFBundleUnloadExecutable(fBundle);
		::CFRelease(fBundle);
		fBundle = NULL;
	}
	
	if (fHandle_dlopen != NULL)
	{
		dlclose( fHandle_dlopen);
		fHandle_dlopen = NULL;
	}
}


void* XMacLibrary::GetProcAddress(const VString& inProcName)
{
	void* ptProc = NULL;
	
	#if !ARCH_64
	if (fFragConnectionID != NULL)
	{
		Str255 macProcName;
		inProcName.MAC_ToMacPString(macProcName, sizeof(macProcName));

		CFragSymbolClass symClass;
		OSStatus err = ::FindSymbol(fFragConnectionID, macProcName, (Ptr*) &ptProc, &symClass);
		
		// Make sure returned a code pointer
		if (err == noErr && symClass != kCodeCFragSymbol && symClass != kTVectorCFragSymbol && symClass != kGlueCFragSymbol)
		{
/*
			StThrowFileError errThrow( inFilePath, VE_LIBRARY_CANNOT_GET_PROC_ADDRESS);

			errThrow->SetString( "procname", inProcName);
*/
			ptProc = NULL;
		}
	}
	else
	#endif
	if (fBundle != NULL)
	{
		CFStringRef cfProcName = ::CFStringCreateWithCharacters(NULL, inProcName.GetCPointer(), inProcName.GetLength());
		
		if (cfProcName != NULL)
		{
			ptProc = ::CFBundleGetFunctionPointerForName(fBundle, cfProcName);
/*			
			if (ptProc == NULL)
			{
				StThrowFileError errThrow( inFilePath, VE_LIBRARY_CANNOT_GET_PROC_ADDRESS);

				errThrow->SetString( "procname", inProcName);
			}
*/			
			::CFRelease(cfProcName);
		}				 
	}
	
	return ptProc;
}


VFolder* XMacLibrary::RetainFolder(const VFilePath &inFilePath, BundleFolderKind inKind) const
{
	VFolder* folder = NULL;
	
	#if !ARCH_64
	if (fFragConnectionID != NULL)
	{
		VFilePath parentFolder;
		inFilePath.GetParent(parentFolder);
		folder = new VFolder(parentFolder);
	}
	else
	#endif
	if (fBundle != NULL)
	{
		switch (inKind)
		{
			case kBF_BUNDLE_FOLDER:
			{
				folder = new VFolder(inFilePath);
			}
			break;
				
			case kBF_EXECUTABLE_FOLDER:
			{
				VFile*	executableFile = RetainExecutableFile(inFilePath);
				if (executableFile != NULL)
				{
					folder = executableFile->RetainParentFolder();
					executableFile->Release();
				}
				break;
			}
				
			case kBF_RESOURCES_FOLDER:
			{
				CFURLRef	urlRef = ::CFBundleCopyResourcesDirectoryURL(fBundle);
				if (urlRef != NULL)
				{
					CFStringRef cfstr;
					VString str;
					VFilePath path;
					
					cfstr = CFURLCopyFileSystemPath( urlRef, kCFURLPOSIXPathStyle );
					str.MAC_FromCFString( cfstr );					
					str += '/';
					path.FromRelativePath( inFilePath, str, FPS_POSIX );
					folder = new VFolder( path );
					
					::CFRelease( cfstr );
					::CFRelease(urlRef);
				}
				break;
			}
			
			case kBF_LOCALISED_RESOURCES_FOLDER:
			{
				/*
				CFURLRef	urlRef = ::CFBundleCopyResourceURL(fBundle, CFSTR(""), CFSTR(""), NULL);
				if (urlRef != NULL)
				{
					FSRef	fsRef;
					
					if (::CFURLGetFSRef(urlRef, &fsRef))
					{
						resFile = new VFile(fsRef);
						folder = resFile->GetParent();
						delete resFile;
					}
					
					::CFRelease(urlRef);
				}
				*/
				break;
			}
				
			case kBF_PLUGINS_FOLDER:
			{
				CFURLRef	urlRef = ::CFBundleCopyBuiltInPlugInsURL(fBundle);
				if (urlRef != NULL)
				{
					FSRef	fsRef;
					
					if (::CFURLGetFSRef(urlRef, &fsRef))
						folder = new VFolder(fsRef);
					
					::CFRelease(urlRef);
				}
				break;
			}
				
			case kBF_PRIVATE_FRAMEWORKS_FOLDER:
			{
				CFURLRef	urlRef = ::CFBundleCopyPrivateFrameworksURL(fBundle);
				if (urlRef != NULL)
				{
					FSRef	fsRef;
					
					if (::CFURLGetFSRef(urlRef, &fsRef))
						folder = new VFolder(fsRef);
					
					::CFRelease(urlRef);
				}
				break;
			}
				
			case kBF_PRIVATE_SUPPORT_FOLDER:
			{
				CFURLRef	urlRef = ::CFBundleCopySupportFilesDirectoryURL(fBundle);
				if (urlRef != NULL)
				{
					FSRef	fsRef;
					
					if (::CFURLGetFSRef(urlRef, &fsRef))
						folder = new VFolder(fsRef);
					
					::CFRelease(urlRef);
				}
				break;
			}
		}
	}
	
	return folder;
}


VFile* XMacLibrary::RetainExecutableFile(const VFilePath &/*inFSObject*/) const
{
	VFile*	executableFile = NULL;
	
	CFURLRef	urlRef = ::CFBundleCopyExecutableURL(fBundle);
	if (urlRef != NULL)
	{
		FSRef	fsRef;
		
		if (::CFURLGetFSRef(urlRef, &fsRef))
			executableFile = new VFile(fsRef);
		
		::CFRelease(urlRef);
	}
	
	return executableFile;
}


VResourceFile* XMacLibrary::RetainResourceFile(const VFilePath &inFilePath)
{
	VResourceFile*	resFile = NULL;
	
	#if ARCH_32
	if (fFragConnectionID != NULL)
	{
		resFile = new VMacResFile(inFilePath, FA_READ, false);
	}
	else
	#endif
	if (fBundle != NULL)
	{
		sWORD	resNumber = ::CFBundleOpenBundleResourceMap(fBundle);

		// the resource file for component is private so... (m.c)
		OSErr err = DetachResourceFile(resNumber);

		#if ARCH_32
		resFile = new VMacResFile(resNumber);
		#endif
	}
	
	return resFile;
}

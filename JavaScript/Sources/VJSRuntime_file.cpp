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
#include "VJavaScriptPrecompiled.h"

#include "VJSValue.h"
#include "VJSContext.h"
#include "VJSClass.h"
#include "VJSGlobalClass.h"
#include "VJSRuntime_file.h"
#include "VJSW3CFileSystem.h"

#if VERSIONMAC

#include <sys/types.h>
#include <dirent.h>

#endif

USING_TOOLBOX_NAMESPACE

#if VERSIONMAC
XBOX::VCriticalSection	VJSPath::sMutex;
XBOX::VString			VJSPath::sStartupVolumePrefix;
#endif

void VJSPath::CleanPath (XBOX::VString *ioPath)
{
	xbox_assert(ioPath != NULL);

#if VERSIONMAC
    
	if (ioPath->BeginsWith("/Volumes", true)) {
		
        // Determine startup volume, it is the one mounted at root.
	
        if (!sStartupVolumePrefix.GetLength()) {
	
            XBOX::StLocker<XBOX::VCriticalSection>	lock(&sMutex);
		
            // Race-condition possible, recheck.
		
            if (!sStartupVolumePrefix.GetLength()) {
		
                DIR	*dir;
			
                if ((dir = ::opendir("/Volumes")) != NULL) {
				
                    struct dirent	*entryStruct;

                    while ((entryStruct = ::readdir(dir)) != NULL) {

                        // Skip ".", "..", and hidden files or directories.
					
                        if (*entryStruct->d_name == '.')
						
                            continue;
										
                        char	path[PATH_MAX], buffer[PATH_MAX];
                        ssize_t	size;

                        path[0] = '\0';
                        strcat(path, "/Volumes/");
                        strcat(path, entryStruct->d_name);
					
                        // readlink() will fail if it isn't a link.
					
                        if ((size = ::readlink(path, buffer, PATH_MAX)) > 0) {
					
                            buffer[size] = '\0';
                            if (!strcmp(buffer, "/")) {
												
                                sStartupVolumePrefix.FromBlock(path, ::strlen(path), VTC_UTF_8);
                                break;

                            }
						
                        }
						
                    }
                    ::closedir(dir);
				
                }
						
                // Not found, this is impossible!
			
                if (!sStartupVolumePrefix.GetLength()) {
			
                    xbox_assert(false);
				
                    // Fail safe. This will never be a valid prefix, make this function do nothing.
				
                    sStartupVolumePrefix = "#not valid#";
				
                }
			
            }
		
        }
	
        // Strip "/Volumes/my_startup_hardrive" from path if needed
		
        if (ioPath->BeginsWith(sStartupVolumePrefix, "true")) {
		
            VSize	numberCharacters;
		
            numberCharacters = ioPath->GetLength() - sStartupVolumePrefix.GetLength();
            ioPath->SubString(sStartupVolumePrefix.GetLength() + 1, numberCharacters);
		
        }
        
    }
    
	// Remove "./" and resolve "../" from path.
		
	VIndex	index;
	
	index = ioPath->FindUniChar('/', 1, false);
	if (index > 0 && index < ioPath->GetLength()) {

		XBOX::VString	resolved, subString;

		ioPath->GetSubString(1, index, resolved);
		for ( ; ; ) {

			if (index == ioPath->GetLength())

				break;

			VIndex	nextIndex;

			nextIndex = ioPath->FindUniChar('/', index + 1, false);
			if (nextIndex <= 0) {

				ioPath->GetSubString(index + 1, ioPath->GetLength() - index, subString);
				resolved.AppendString(subString);
				break;

			}

			VSize	length;
						
			length = nextIndex - index;
			if (ioPath->GetUniChar(index + 1) == '.') {
							
				if (length == 2) {

					// "./" case. Ignore, so it doesn't show up in cleaned path.
			
					index = nextIndex;
					continue;
				
				} 				
				if (length == 3 && ioPath->GetUniChar(index + 2) == '.') {

					// "../" case. Resolve path, backup to parent directory.

					VIndex	i, j;
                    
                    if (!resolved.GetLength())
                        
                        i = 0;

					else if (resolved.GetUniChar(resolved.GetLength()) == '/')

						i = resolved.GetLength() - 1;

					else

						i = resolved.GetLength();

					if (i >= 1) {

						j = resolved.FindUniChar('/', i, true);
						if (j >= 1)

							resolved.SubString(1, j);
                        
                        else
                            
                            resolved.Clear();
						
					} else
                        
                        resolved.Clear();
                    
					index = nextIndex;
					continue;

				}	
			
			} 

			// Otherwise just copy. Note that "//" is left unchanged in "resolved" path.
				
			ioPath->GetSubString(index + 1, length, subString);
			resolved.AppendString(subString);
			index = nextIndex;

		}
		*ioPath = resolved;

	}
#endif
}

void VJSPath::ResolvePath (XBOX::VString *ioPath)
{
	xbox_assert(ioPath != NULL);

#if VERSIONMAC || VERSION_LINUX
	
	// If ::realpath() returns NULL, then the path is incorrect, 
	// in which case just leave it "unresolved". 

	char	path[PATH_MAX], resolvedPath[PATH_MAX];
	 
	ioPath->ToBlock(path, PATH_MAX, VTC_UTF_8, true, false);
	if (::realpath(path, resolvedPath) != NULL) {
	
		size_t	size;

		size = ::strlen(resolvedPath);
		ioPath->FromBlock(resolvedPath, size, VTC_UTF_8);

	} else {

		// Call to realpath() failed, just remove "//" sequences so VFilePath won't reject it.

		_RemoveMultipleFolderSeparators(ioPath);	

	}

#else 

	_RemoveMultipleFolderSeparators(ioPath);

#endif	
}

void VJSPath::_RemoveMultipleFolderSeparators (XBOX::VString *ioPath)
{
	xbox_assert(ioPath != NULL);

	XBOX::VString	unresolved(*ioPath);
	sLONG			i;
	UniChar			c;

	i = 0;
	ioPath->Clear();
	while (i < unresolved.GetLength())
	{
		c = unresolved.GetUniChar(i + 1);
		ioPath->AppendUniChar(c);
		i++;
		if (c == '/')
		{
			while (i < unresolved.GetLength())
			{
				if (unresolved.GetUniChar(i + 1) == '/')
					i++;
				else
					break;
			}
		}
	}
}

bool VJSPath::CanAccessAllFiles( const VJSContext& inContext)
{
	VJSGlobalObject *globalObject = inContext.GetGlobalObjectPrivateInstance();	// in Studio, the global object may be null when loading a frame
	IJSRuntimeDelegate *runtimeDelegate = (globalObject == NULL) ? NULL : globalObject->GetRuntimeDelegate();
	return (runtimeDelegate == NULL) || runtimeDelegate->CanAccessAllFiles();
}


VFile *VJSPath::ResolveFileParam( const VJSContext& inContext, const VString& inJSPathParam, EFSNOptions inOptions)
{
	VFile *file = NULL;
	VURL url;
	if (JS4D::GetURLFromPath( inJSPathParam, url))
	{
		VString	urlPath;
		url.GetPath( urlPath, eURL_POSIX_STYLE, false);
		urlPath.Insert('/', 1);

		bool ok = false;
		
		VJSGlobalObject *globalObject = inContext.GetGlobalObjectPrivateInstance();	// in Studio, the global object may be null when loading a frame
		IJSRuntimeDelegate *runtimeDelegate = (globalObject == NULL) ? NULL : globalObject->GetRuntimeDelegate();
		if (runtimeDelegate != NULL)
		{
			VFileSystemNamespace *fsNameSpace = runtimeDelegate->RetainRuntimeFileSystemNamespace();

			ok = fsNameSpace->ParsePathWithFileSystem( urlPath, &file, NULL, inOptions);

			ReleaseRefCountable( &fsNameSpace);
		}
		
		if (!ok && ((runtimeDelegate == NULL) || runtimeDelegate->CanAccessAllFiles()) )
		{
			VFilePath		path;
				
	#if VERSIONWIN
			if (urlPath.GetUniChar(1) == '/')
				urlPath.Remove( 1, 1);
	#endif
			
			VJSPath::ResolvePath(&urlPath);
			path.FromFullPath(urlPath, FPS_POSIX);
			
			if (path.IsFile())
				file = new VFile(path);
		}
	}
	
	return file;
}


VFolder *VJSPath::ResolveFolderParam( const VJSContext& inContext, const VString& inJSPathParam)
{
	VFolder *folder = NULL;

	VString jsPathParam( inJSPathParam);
	if (!jsPathParam.IsEmpty() && jsPathParam[jsPathParam.GetLength()-1] != '/')
		jsPathParam += '/';

	VURL url;
	if (JS4D::GetURLFromPath( jsPathParam, url))
	{
		VString	urlPath;
		url.GetPath( urlPath, eURL_POSIX_STYLE, false);
		urlPath.Insert('/', 1);

		bool ok = false;

		VJSGlobalObject *globalObject = inContext.GetGlobalObjectPrivateInstance();	// in Studio, the global object may be null when loading a frame
		IJSRuntimeDelegate *runtimeDelegate = (globalObject == NULL) ? NULL : globalObject->GetRuntimeDelegate();
		if (runtimeDelegate != NULL)
		{
			VFileSystemNamespace *fsNameSpace = runtimeDelegate->RetainRuntimeFileSystemNamespace();

			ok = testAssert(fsNameSpace != NULL) ? fsNameSpace->ParsePathWithFileSystem( urlPath, NULL, &folder) : false;

			ReleaseRefCountable( &fsNameSpace);
		}
		
		if (!ok && ((runtimeDelegate == NULL) || runtimeDelegate->CanAccessAllFiles()) )
		{
			VFilePath path;
			if (url.GetFilePath(path) && path.IsFolder())
			{
				folder = new VFolder(path);
			}
		}
	}
	
	return folder;
}

//======================================================


bool JS4DFolderIterator::Next()
{
	bool result = false;
	if (fIter == NULL)
	{
		return false;
	}
	else
	{
		if (fBefore)
		{
			fBefore = false;
		}
		else
		{
			++(*fIter);
		}
		result = fIter->IsValid();
	}
	return result;
}


//======================================================


JS4DFileIterator::JS4DFileIterator( XBOX::VFileIterator* inIter)
: VJSBlobValue( CVSTR(""))
, fIter( inIter)
, fFile( NULL)
, fBefore( true)
{
}


JS4DFileIterator::JS4DFileIterator( XBOX::VFile* inFile)
: VJSBlobValue( CVSTR(""))
, fIter( NULL)
, fFile( RetainRefCountable( inFile))
, fBefore( false)
{
}


JS4DFileIterator::JS4DFileIterator( XBOX::VFile* inFile, sLONG8 inStart, sLONG8 inEnd, const VString& inContentType)
: VJSBlobValue( inStart, inEnd, inContentType)
, fIter( NULL)
, fFile( RetainRefCountable( inFile))
, fBefore( false)
{
}

JS4DFileIterator::~JS4DFileIterator()
{
	delete fIter;
	ReleaseRefCountable( &fFile);
}


VFile* JS4DFileIterator::GetFile()
{
	fBefore = false;
	if (fIter == NULL)
		return fFile;
	else
		return fIter->Current();
}


bool JS4DFileIterator::Next()
{
	bool result = false;
	if (fIter == NULL)
	{
		return false;
	}
	else
	{
		if (fBefore)
		{
			fBefore = false;
		}
		else
		{
			++(*fIter);
		}
		result = fIter->IsValid();
	}
	return result;
}

	
sLONG8 JS4DFileIterator::GetMaxSize() const
{
	VFile *file = const_cast<JS4DFileIterator*>( this)->GetFile();
	
	if (file == NULL)
		return 0;
	
	sLONG8 fileSize;
	if (file->GetSize( &fileSize) != VE_OK)
		return 0;

	return fileSize;
}


JS4DFileIterator* JS4DFileIterator::Slice( sLONG8 inStart, sLONG8 inEnd, const VString& inContentType) const
{
	VFile *file = const_cast<JS4DFileIterator*>( this)->GetFile();
	
	if (file == NULL)
		return NULL;

	sLONG8 sliceSize = GetSize();	// use slice size and not file size
	
	// normalize
	sLONG8 start = (inStart >= 0) ? std::min<sLONG8>( inStart, sliceSize) : std::max<sLONG8>( sliceSize + inStart, 0);
	sLONG8 end = (inEnd >= 0) ? (sLONG8) std::min<sLONG8>( inEnd, sliceSize) : std::max<sLONG8>( sliceSize + inEnd, 0);
	sLONG8 size = (end > start) ? (end - start) : 0;

	return new JS4DFileIterator( file, start, size, inContentType);
}


VJSDataSlice* JS4DFileIterator::RetainDataSlice() const
{
	VSize start = (VSize) GetStart();
	VSize size = (VSize) GetSize();
	VSize actualCount = 0;
	void *data = NULL;

	VFile *file = const_cast<JS4DFileIterator*>( this)->GetFile();
	if (file != NULL)
	{
		VFileDesc *desc = NULL;
		VError err = file->Open( FA_READ, &desc);
		if (err == VE_OK)
		{
			data = VMemory::NewPtr( size, 'blob');
			if (data != NULL)
			{
				err = desc->GetData( data, size, start, &actualCount);
			}
			else
			{
				vThrowError( VE_MEMORY_FULL);
			}
		}
		delete desc;
	}
	
	VJSBlobData *blobData = new VJSBlobData( data, actualCount);
	VJSDataSlice *slice = new VJSDataSlice( blobData, 0, actualCount);
	ReleaseRefCountable( &blobData);

	return slice;
}


VError JS4DFileIterator::CopyTo( VFile *inDestination, bool inOverwrite)
{
	VError err = VE_OK;
	VFile* file = GetFile();
	if (file != NULL)
	{
		err = file->CopyTo( *inDestination, inOverwrite ? FCP_Overwrite : FCP_Default);
	}	
	return err;
}

//======================================================

JS4DFolderIterator*	VJSFolderIterator::sDummy = new JS4DFolderIterator((VFolderIterator *) NULL);

void VJSFolderIterator::_Initialize( const VJSParms_initialize& inParms, JS4DFolderIterator* inFolderIter)
{
	inFolderIter->Retain();
}


void VJSFolderIterator::_Finalize( const VJSParms_finalize& inParms, JS4DFolderIterator* inFolderIter)
{
	inFolderIter->Release();
}


void VJSFolderIterator::Construct(  VJSParms_construct &ioParms)
{

#if USE_V8_ENGINE
    XBOX::VJSObject constructedObject = ioParms.fConstructedObject;
#else
    XBOX::VJSObject	constructedObject(ioParms.GetContext());
#endif

	VString folderurl;
	if (ioParms.IsStringParam(1) && ioParms.GetStringParam(1, folderurl))
	{
		VFolder *folder = VJSPath::ResolveFolderParam( ioParms.GetContext(), folderurl);
		if (folder != NULL)
		{
			constructedObject.MakeFolder( folder);
		}
		else
		{
			//vThrowError( VE_JVSC_NOT_A_FOLDER);	temp until git extension js code is updated
		}
		ReleaseRefCountable( &folder);
	}
	else
	{
		JS4DFolderIterator* folderiter = ioParms.GetParamObjectPrivateData<VJSFolderIterator>(1);
		if (folderiter != NULL)
		{
			VFolder* folder = folderiter->GetFolder();
			if (folder != NULL)
			{
				VString foldername;
				if (ioParms.IsStringParam(2) && ioParms.GetStringParam(2, foldername))
				{
					VFolder* subfolder = folder->RetainRelativeFolder( foldername, FPS_POSIX);
					constructedObject.MakeFolder( subfolder);
					ReleaseRefCountable( &subfolder);
				}
				else
					vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2");
			} 
		}
		else
			_ConstructFromW3CFS(ioParms,constructedObject);
	}

	ioParms.ReturnConstructedObject( constructedObject);
}

// Implement : 
//
//	new Folder(fileSystem, path);
//	new Folder(directoryEntry, path);
//	new Folder(directoryEntry);

void VJSFolderIterator::_ConstructFromW3CFS( VJSParms_withArguments &ioParms, VJSObject& ioConstructedObject)
{
	bool isOk = true;
	ioConstructedObject.ClearRef();

	// get a directory or a fileSystem as first parameter
	VJSObject object(ioParms.GetContext());
	ioParms.GetParamObject(1, object);

	VJSFileSystem*	fileSystem = NULL;
	VFilePath		rootPath;				

	if (object.IsOfClass(VJSFileSystemClass::Class()) || object.IsOfClass(VJSFileSystemSyncClass::Class()))
	{
		if (object.IsOfClass(VJSFileSystemClass::Class()))
			fileSystem = object.GetPrivateData<VJSFileSystemClass>();
		else
			fileSystem = object.GetPrivateData<VJSFileSystemSyncClass>();

		xbox_assert(fileSystem != NULL);
		rootPath = fileSystem->GetRoot();
		isOk = true;
	}
	else if (object.IsOfClass(VJSDirectoryEntryClass::Class()) || object.IsOfClass(VJSDirectoryEntrySyncClass::Class()))
	{
		VJSEntry	*entry;

		if (object.IsOfClass(VJSDirectoryEntryClass::Class()))
			entry = object.GetPrivateData<VJSDirectoryEntryClass>();
		else
			entry = object.GetPrivateData<VJSDirectoryEntrySyncClass>();

		xbox_assert(entry != NULL);
		fileSystem = entry->GetFileSystem();
		rootPath = entry->GetPath();
		isOk = true;
	}
	else
	{
		vThrowError( VE_JVSC_WRONG_PARAMETER, "1");
		isOk = false;
	}

	// get optionnal relative path
	VString	 relativePath;
	if (isOk)
	{
		if ( (ioParms.CountParams() >= 2) && !ioParms.GetStringParam(2, relativePath))
		{
			vThrowError( VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2");
			isOk = false;
		}
	}
	
	if (isOk)
	{
		if (!relativePath.IsEmpty() && relativePath.GetUniChar( relativePath.GetLength()) != '/')
			relativePath += '/';

		VFilePath path( rootPath, relativePath, FPS_POSIX);
		if (!path.IsFolder())
		{
			vThrowError( VE_JVSC_NOT_A_FOLDER);
		}
		else if ( (fileSystem != NULL) && !fileSystem->GetFileSystem()->IsAuthorized( path) )
		{
			VJSFileErrorClass::Throw( VJSFileErrorClass::SECURITY_ERR);
		}
		else
		{
			VFolder	*folder = new VFolder( path, fileSystem->GetFileSystem());
			if (folder == NULL)
			{ 
				vThrowError( VE_MEMORY_FULL);
			}
			else
			{
				ioConstructedObject.MakeFolder( folder);
			}
			ReleaseRefCountable(&folder);
		}
	}

}

void VJSFolderIterator::_IsFolder (XBOX::VJSParms_callStaticFunction &ioParms, XBOX::VObject *)
{
	VString	path;
	if (ioParms.IsStringParam(1) && ioParms.GetStringParam(1, path))
	{
		VFolder *folder = VJSPath::ResolveFolderParam( ioParms.GetContext(), path);
		ioParms.ReturnBool( (folder != NULL) && folder->Exists() );
		ReleaseRefCountable( &folder);
	}
	else
	{
		VJSObject	object(ioParms.GetContext());
		if (ioParms.IsObjectParam(1) && ioParms.GetParamObject(1, object) && object.IsOfClass(VJSFolderIterator::Class()))
		{
			JS4DFolderIterator	*folderIterator = object.GetPrivateData<VJSFolderIterator>();
			ioParms.ReturnBool( (folderIterator != NULL) && (folderIterator->GetFolder() != NULL) && folderIterator->GetFolder()->Exists() );
		}
		else
		{
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FOLDER, "1");
		}
	}
}

void VJSFolderIterator::_GetName(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VString foldername;
	bool noextension = ioParms.GetBoolParam( 1, L"No Extension", "With Extension");
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		/*
		if (noextension)
			inFolderIter->Current()->GetNameWithoutExtension(foldername);
		else
		 */
			folder->GetName(foldername);
	}
	ioParms.ReturnString(foldername);
}


void VJSFolderIterator::_SetName(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VString foldername;
	VFolder* folder = inFolderIter->GetFolder();

	// WAK0072305: setName() should return true or false depending on success of operation, but not a Folder object. 
	
	ioParms.ReturnBool(false);

	if (folder != NULL)
	{
		if (ioParms.IsStringParam(1) && ioParms.GetStringParam(1, foldername))
		{
			// WAK0071545: Catch error, prevent an exception to be thrown. If erroneous, return false instead.

			StErrorContextInstaller	context(false, true);	

			VFolder	*resultfolder	= nil;		

			if (folder->Rename(foldername, &resultfolder) == XBOX::VE_OK)

				ioParms.ReturnBool(true);

		}
		else
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
	}
}


void VJSFolderIterator::_Valid(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	ioParms.ReturnBool(inFolderIter->GetFolder() != NULL);
}


void VJSFolderIterator::_Next(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	ioParms.ReturnBool(inFolderIter->Next());
}


void VJSFolderIterator::_FirstSubFolder(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VFolderIterator* iter = new VFolderIterator(folder, FI_NORMAL_FOLDERS | FI_WANT_INVISIBLES | FI_ITERATE_DELETE);
		JS4DFolderIterator* result = new JS4DFolderIterator(iter);
		ioParms.ReturnValue(VJSFolderIterator::CreateInstance(ioParms.GetContext(), result));
		ReleaseRefCountable( &result);
	}
}


void VJSFolderIterator::_FirstFile(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VFileIterator* iter = new VFileIterator(folder, FI_NORMAL_FILES | FI_WANT_INVISIBLES | FI_ITERATE_DELETE);
		JS4DFileIterator* result = new JS4DFileIterator(iter);
		ioParms.ReturnValue(VJSFileIterator::CreateInstance(ioParms.GetContext(), result));
		ReleaseRefCountable( &result);
	}
}


void VJSFolderIterator::_IsVisible(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	bool visible = false;
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		bool hidden;
		folder->IsHidden(hidden);
		visible = !hidden;
	}
	ioParms.ReturnBool(visible);
}


void VJSFolderIterator::_SetVisible(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		if (ioParms.GetBoolParam( 1, L"Visible", L"Invisible"))
		{
			// a faire 
		}
		else
		{
			// a faire
		}
	}
}

void VJSFolderIterator::_IsReadOnly(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
#if VERSION_LINUX   // Postponed Linux Implementation ! (Need refactoring)
        bool isReadable=folder->ProcessCanRead();
        bool isWritable=folder->ProcessCanWrite();

        bool isRO=isReadable&&!isWritable;
#else
        bool isRO=!folder->IsWritableByAll();
#endif
		ioParms.ReturnBool(isRO);
	}
}


void VJSFolderIterator::_SetReadOnly(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
#if !VERSION_LINUX  // Postponed Linux Implementation ! (Need refactoring)
	VFolder* folder = inFolderIter->GetFolder();

	if (folder != NULL) {
	
		bool	isReadOnly;

		isReadOnly = ioParms.GetBoolParam( 1, L"ReadOnly", L"ReadWrite"); 
		_SetReadOnly(folder, isReadOnly);

	}
#endif
}


void VJSFolderIterator::_GetPath(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VString folderpath;
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		folder->GetPath(folderpath, FPS_POSIX, false);
		VJSPath::CleanPath(&folderpath);
	}
	ioParms.ReturnString(folderpath);
}

void VJSFolderIterator::_GetURL(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VString folderurl;
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VString path;
		folder->GetPath(path, FPS_POSIX);
		
		VJSPath::CleanPath(&path);
		
		VURL url;
		
		url.FromFilePath(path);
		url.GetAbsoluteURL(folderurl, ioParms.GetBoolParam( 1, L"Encoded", L"Not Encoded"));
	}
	ioParms.ReturnString(folderurl);
}


void VJSFolderIterator::_Delete(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		ioParms.ReturnBool(folder->Delete(true) == VE_OK);
	}
}


void VJSFolderIterator::_DeleteContent(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		ioParms.ReturnBool(folder->DeleteContents(true) == VE_OK);
	}
}


void VJSFolderIterator::_GetFreeSpace(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		sLONG8 freebytes;
		folder->GetVolumeFreeSpace(&freebytes,  ! ioParms.GetBoolParam( 1, L"NoQuotas", L"WithQuotas"));
		ioParms.ReturnNumber(freebytes);
	}
}


void VJSFolderIterator::_GetVolumeSize(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		sLONG8 volumeSize = - 1;
		folder->GetVolumeCapacity(&volumeSize);
		ioParms.ReturnNumber(volumeSize);
	}
}


void VJSFolderIterator::_Create(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		ioParms.ReturnBool(folder->CreateRecursive(true) == VE_OK);
	}
}


void VJSFolderIterator::_Exists(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		ioParms.ReturnBool(folder->Exists());
	}
}


void VJSFolderIterator::_GetParent(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VFolder* parent = folder->RetainParentFolder( true);
		ioParms.ReturnFolder( parent);
		ReleaseRefCountable( &parent);
	}
}


void VJSFolderIterator::_eachFile(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VFilePath path;
		folder->GetPath(path);
		if (path.IsFolder())
		{
			std::vector<VJSValue> params;
			VJSObject objfunc(ioParms.GetContext());
			if (ioParms.GetParamFunc(1, objfunc, true))
			{
				VJSValue elemVal(ioParms.GetContext());
				params.push_back(elemVal);
				VJSValue indiceVal(ioParms.GetContext());
				params.push_back(indiceVal);
				params.push_back(ioParms.GetThis());

				VJSValue thisParamVal(ioParms.GetParamValue(2));
				VJSObject thisParam(thisParamVal.GetObject());

				sLONG counter = 0;
				VFileIterator iter(folder, FI_NORMAL_FILES | FI_WANT_INVISIBLES | FI_ITERATE_DELETE);
				bool cont = true;
				while (iter.IsValid() && cont)
				{
					VJSValue indiceVal2(ioParms.GetContext());
					indiceVal2.SetNumber(counter);
					params[1] = indiceVal2;
					//params[1].SetNumber(counter);
					VFile* file = iter.Current();
					JS4DFileIterator* ifile = new JS4DFileIterator(file);
					VJSObject thisobj(VJSFileIterator::CreateInstance(ioParms.GetContext(), ifile));
					ReleaseRefCountable( &ifile);
					params[0] = thisobj;
					VJSValue result(ioParms.GetContext());
					VJSException except;
					thisParam.CallFunction(objfunc, &params, &result, except);
					if (except.GetValue() != nil)
					{
						cont = false;
						ioParms.SetException(except.GetValue());
					}
					++iter;
					++counter;
				}
			}
		}
	}
	ioParms.ReturnThis();
}


void VJSFolderIterator::_eachFolder(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VFilePath path;
		folder->GetPath(path);
		if (path.IsFolder())
		{
			std::vector<VJSValue> params;
			VJSObject objfunc(ioParms.GetContext());
			if (ioParms.GetParamFunc(1, objfunc, true))
			{
				VJSValue elemVal(ioParms.GetContext());
				params.push_back(elemVal);
				VJSValue indiceVal(ioParms.GetContext());
				params.push_back(indiceVal);
				params.push_back(ioParms.GetThis());

				VJSValue thisParamVal(ioParms.GetParamValue(2));
				VJSObject thisParam(thisParamVal.GetObject());

				sLONG counter = 0;
				VFolderIterator iter(folder, FI_NORMAL_FOLDERS | FI_WANT_INVISIBLES | FI_ITERATE_DELETE);
				bool cont = true;
				while (iter.IsValid() && cont)
				{
					VJSValue indiceVal2(ioParms.GetContext());
					indiceVal2.SetNumber(counter);
					params[1] = indiceVal2;
					//params[1].SetNumber(counter);

					VFolder* subfolder = iter.Current();
					JS4DFolderIterator* ifolder = new JS4DFolderIterator(subfolder);
					VJSObject thisobj(VJSFolderIterator::CreateInstance(ioParms.GetContext(), ifolder));
					ReleaseRefCountable( &ifolder);

					params[0] = thisobj;
					VJSValue result(ioParms.GetContext());
					VJSException except;
					thisParam.CallFunction(objfunc, &params, &result, except);
					if (except.GetValue() != nil)
					{
						cont = false;
						ioParms.SetException(except.GetValue());
					}
					++iter;
					++counter;
				}
			}
		}
	}
	ioParms.ReturnThis();
}

// Lexicographic ordering (much like strcmp() but on XBOX::VString objects).
// This is for coherency with JavaScript runtimes.
// For instance :
//
//	var b = "AAA" < "zz";
//
// Will result in b having a value of true. XBOX::VString::CompareToString()
// doesn't have same behavior.

class CLexCompare 
{
public:

	bool		operator() (const XBOX::VString &a, const XBOX::VString &b) const	{ return isSmaller(a, b); }
	static bool isSmaller (const XBOX::VString &a, const XBOX::VString &b);
};	

bool CLexCompare::isSmaller (const XBOX::VString &a, const XBOX::VString &b)
{ 
	const UniChar	*p = a.GetCPointer();
	const UniChar	*q = b.GetCPointer();

	for ( ; ; )

		if (*p == 0)

			return true;

		else if (*q == 0)

			return false;

		else if (*p < *q)

			return true;

		else if (*p > *q)

			return false;

		else {

			p++;
			q++;

		}
}

// Do a lexicographic depth-first traversal.

bool VJSFolderIterator::parseFolder (VFolder* folder, VJSParms_callStaticFunction& ioParms, VJSObject& objfunc, std::vector<VJSValue>& params, sLONG& counter, VJSObject& thisParam)
{
	typedef	std::map<XBOX::VString, VRefPtr<XBOX::VFolder>, CLexCompare>	SFolderMap;
	typedef	std::map<XBOX::VString, VRefPtr<XBOX::VFile>, CLexCompare>		SFileMap;

	// Look up all folders and files of current folder.

	SFolderMap		folderMap;
	VFolderIterator iterFolder(folder, FI_NORMAL_FOLDERS | FI_WANT_INVISIBLES | FI_ITERATE_DELETE);
	
	while (iterFolder.IsValid()) {

		XBOX::VFolder	*subFolder = iterFolder.Current();
		XBOX::VString	folderName;

		subFolder->GetName(folderName);
		folderMap[folderName] = subFolder;		
		++iterFolder;

	}

	SFileMap		fileMap;
	VFileIterator	iterFile(folder, FI_NORMAL_FILES | FI_WANT_INVISIBLES | FI_ITERATE_DELETE);

	while (iterFile.IsValid()) {

		XBOX::VFile		*file	= iterFile.Current();
		XBOX::VString	fileName;

		file->GetName(fileName);
		fileMap[fileName] = file;
		++iterFile;

	}

	// Do the traversal.

	JS4DFolderIterator	*ifolder = new JS4DFolderIterator(folder);
	VJSObject		folderObject(VJSFolderIterator::CreateInstance(ioParms.GetContext(), ifolder));
	ReleaseRefCountable( &ifolder);
			
	folderObject.Protect();

	SFolderMap::iterator	folderMapIter	= folderMap.begin();
	SFileMap::iterator		fileMapIter		= fileMap.begin();
	bool					cont			= true;

	while (cont && (folderMapIter != folderMap.end() || fileMapIter	!= fileMap.end())) {

		bool	doHandleFile;

		if (folderMapIter == folderMap.end())

			doHandleFile = true;

		else if (fileMapIter == fileMap.end()) 

			doHandleFile = false;

		else

			doHandleFile = CLexCompare::isSmaller(fileMapIter->first, folderMapIter->first);

		if (doHandleFile) {

			JS4DFileIterator	*ifile	= new JS4DFileIterator(fileMapIter->second);
			VJSObject			thisobj(VJSFileIterator::CreateInstance(ioParms.GetContext(), ifile));
			ReleaseRefCountable( &ifile);
			VJSValue			result(ioParms.GetContext());
			VJSException		except;

			params[0] = thisobj;
			VJSValue indiceVal2(ioParms.GetContext());
			indiceVal2.SetNumber(counter);
			params[1] = indiceVal2;
			//params[1].SetNumber(counter);
			params[2] = folderObject;

			thisParam.CallFunction(objfunc, &params, &result, except);
			if (except.GetValue() != nil) {

				cont = false;
				ioParms.SetException(except.GetValue());

			}

			fileMapIter++;

			++counter;

		} else {

			cont = parseFolder(folderMapIter->second, ioParms, objfunc, params, counter, thisParam);
			folderMapIter++;
			
		}

	}

	folderObject.Unprotect();

	return cont;
}

void VJSFolderIterator::_parse(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VFilePath path;
		folder->GetPath(path);
		if (path.IsFolder())
		{
			std::vector<VJSValue> params;
			VJSObject objfunc(ioParms.GetContext());
			if (ioParms.GetParamFunc(1, objfunc, true))
			{
				VJSValue elemVal(ioParms.GetContext());
				params.push_back(elemVal);
				VJSValue indiceVal(ioParms.GetContext());
				params.push_back(indiceVal);

				params.push_back(ioParms.GetThis());	// Will be overwritten by actual current folder at each callback.

				VJSValue thisParamVal(ioParms.GetParamValue(2));
				VJSObject thisParam(thisParamVal.GetObject());

				sLONG counter = 0;
				parseFolder(folder, ioParms, objfunc,params, counter, thisParam);
			}
		}
	}
	ioParms.ReturnThis();
}



void VJSFolderIterator::_files(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VJSArray result(ioParms.GetContext());
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VFilePath path;
		folder->GetPath(path);
		if (path.IsFolder())
		{
			VFileIterator iter(folder, FI_NORMAL_FILES | FI_WANT_INVISIBLES | FI_ITERATE_DELETE);
			while (iter.IsValid())
			{
				result.PushFile( iter);
				++iter;
			}
		}
	}
	ioParms.ReturnValue(result);
}


void VJSFolderIterator::_folders(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VJSArray result(ioParms.GetContext());
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VFilePath path;
		folder->GetPath(path);
		if (path.IsFolder())
		{
			VFolderIterator iter(folder, FI_NORMAL_FOLDERS | FI_WANT_INVISIBLES | FI_ITERATE_DELETE);
			while (iter.IsValid())
			{
				result.PushFolder( iter);
				++iter;
			}
		}
	}
	ioParms.ReturnValue(result);
}

struct Filter 
{
	XBOX::VString	fName;			// Ignore if empty. 
	XBOX::VString	fNameNoExt;		// Ignore if empty.
	XBOX::VString	fExtension;		// Ignore if empty.
	sLONG			fVisible;		// -1 ignore, 0 not visible, 1 visible.
	sLONG			fReadOnly;		// -1 ignore, 0 not read only, 1 read only.
};

// Recursively retrieve file(s) from folder(s) according to filter, errors are silently ignored.

static void _Traverse (XBOX::VFolder *inFolder, const Filter &inFilter, XBOX::VJSArray *outResult)
{
	xbox_assert(outResult != NULL);

	if (inFolder == NULL) 

		return;

	XBOX::VFilePath	path;

	inFolder->GetPath(path);
	if (!path.IsFolder())

		return;

	// Iterate through file(s).

	XBOX::VFileIterator	fileIterator(inFolder, FI_NORMAL_FILES | FI_WANT_INVISIBLES);

	for ( ; fileIterator.IsValid(); ++fileIterator) {

		XBOX::VFile	*file;

		if ((file = fileIterator.Current()) == NULL)

			continue;

		if (!inFilter.fName.IsEmpty()) {

			XBOX::VString	name;

			file->GetName(name);
			if (name.CompareToString(inFilter.fName, true) != CR_EQUAL)

				continue;

		}

		if (!inFilter.fNameNoExt.IsEmpty()) {

			XBOX::VString	nameNoExt;

			file->GetNameWithoutExtension(nameNoExt);
			if (nameNoExt.CompareToString(inFilter.fNameNoExt, true) != CR_EQUAL)

				continue;

		}

		if (!inFilter.fExtension.IsEmpty()) {

			XBOX::VString	extension;

			file->GetExtension(extension);
			if (extension.CompareToString(inFilter.fExtension, true) != CR_EQUAL)

				continue;

		}

		EFileAttributes	fileAttributes;

		if (file->GetFileAttributes(fileAttributes) != XBOX::VE_OK)

			continue;

		if (inFilter.fVisible >= 0 && ((fileAttributes & FATT_HidenFile) == 0) != inFilter.fVisible)

			continue;

		if (inFilter.fReadOnly >= 0 && ((fileAttributes & FATT_LockedFile) != 0) != inFilter.fReadOnly)

			continue;
		
		outResult->PushFile(file);
				
	}

	// Iterate through folder(s).

	XBOX::VFolderIterator	folderIterator(inFolder, FI_NORMAL_FOLDERS | FI_WANT_INVISIBLES);

	for ( ; folderIterator.IsValid(); ++folderIterator)

		_Traverse(folderIterator.Current(), inFilter, outResult);
}

void VJSFolderIterator::_findAllFiles (VJSParms_callStaticFunction &ioParms, JS4DFolderIterator *inFolderIterator)
{
	xbox_assert(inFolderIterator != NULL);

	XBOX::VJSArray	result(ioParms.GetContext());

	ioParms.ReturnValue(result);

	Filter	filter;

	if (ioParms.CountParams()) {

		XBOX::VJSObject	filterArgument(ioParms.GetContext());
		
		if (ioParms.GetParamObject(1, filterArgument)) {

			XBOX::VJSValue	value(ioParms.GetContext());

			if (filterArgument.HasProperty("name")) {

				value = filterArgument.GetProperty("name");
				if (!value.IsString() || !value.GetString(filter.fName))

					filter.fName.Clear();

			}

			if (filterArgument.HasProperty("nameNoExt")) {

				value = filterArgument.GetProperty("nameNoExt");
				if (!value.IsString() || !value.GetString(filter.fNameNoExt))

					filter.fNameNoExt.Clear();

			}

			if (filterArgument.HasProperty("extension")) {

				value = filterArgument.GetProperty("extension");
				if (!value.IsString() || !value.GetString(filter.fExtension)) 

					filter.fExtension.Clear();

			}

			bool	isTrue;

			if (filterArgument.HasProperty("visible")) {

				value = filterArgument.GetProperty("visible");
				if (!value.GetBool(&isTrue))

					filter.fVisible = -1;

				else

					filter.fVisible = isTrue ? 1 : 0;

			}

			if (filterArgument.HasProperty("readOnly")) {

				value = filterArgument.GetProperty("readOnly");
				if (!value.GetBool(&isTrue))

					filter.fReadOnly = -1;

				else

					filter.fReadOnly = isTrue ? 1 : 0;

			}

		} else {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_OBJECT, "1");
			return;	

		}

	}

	_Traverse(inFolderIterator->GetFolder(), filter, &result);
}


void  VJSFolderIterator::_ToJSON( VJSParms_callStaticFunction& ioParms, JS4DFolderIterator *inFolderIter)
{
	// sc 18/06/2013 WAK0082553, standard 'stringify' doesn't work with Folder object because this class is also declared as a function,
	// then, we have to implement a 'toJSON' function
	VJSONObject *jsonObj = new VJSONObject();

	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VString str;
		folder->GetName( str);
		jsonObj->SetPropertyAsString( L"name", str);
		folder->GetNameWithoutExtension( str);
		jsonObj->SetPropertyAsString( L"nameNoExt", str);
		folder->GetExtension( str);
		jsonObj->SetPropertyAsString( L"extension", str);
		folder->GetPath( str, FPS_POSIX, false);
		VJSPath::CleanPath( &str);
		jsonObj->SetPropertyAsString( L"path", str);
		folder->GetPath( str, FPS_POSIX, true);
		VJSPath::CleanPath( &str);
		jsonObj->SetPropertyAsString( L"pathRel", str);
		
		#if VERSIONDEBUG
		if (VJSPath::CanAccessAllFiles( ioParms.GetContext()))
		{
			folder->GetPath( str, FPS_SYSTEM);
			VJSPath::CleanPath( &str);
			jsonObj->SetPropertyAsString( L"systemPath", str);
		}
		#endif
		
		jsonObj->SetPropertyAsBool( L"exists", folder->Exists());

		if (folder->Exists())
		{
			VTime creationDate, modificationDate;
			if (folder->GetTimeAttributes( &modificationDate, &creationDate) == VE_OK)
			{
				creationDate.GetJSONString( str, JSON_Default);
				jsonObj->SetPropertyAsString( L"creationDate", str);
				modificationDate.GetJSONString( str, JSON_Default);
				jsonObj->SetPropertyAsString( L"modificationDate", str);
			}
		}
	}

	ioParms.ReturnJSONValue( VJSONValue( jsonObj));
	
	ReleaseRefCountable( &jsonObj);
}


void VJSFolderIterator::_creationDate(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VTime dd;
		folder->GetTimeAttributes(nil, &dd, nil);
		ioParms.ReturnTime(dd);
	}
	else
		ioParms.ReturnNullValue();
}


bool VJSFolderIterator::_setcreationDate(VJSParms_setProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != nil)
	{
		VTime dd;
		ioParms.GetPropertyValue().GetTime(dd);
		folder->SetTimeAttributes(nil, &dd, nil);
	}
	return true;
}


void VJSFolderIterator::_modificationDate(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VTime dd;
		folder->GetTimeAttributes(&dd, nil, nil);
		ioParms.ReturnTime(dd);
	}
	else
		ioParms.ReturnNullValue();
}


bool VJSFolderIterator::_setmodificationDate(VJSParms_setProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != nil)
	{
		VTime dd;
		ioParms.GetPropertyValue().GetTime(dd);
		folder->SetTimeAttributes(&dd, nil, nil);
	}
	return true;
}




void VJSFolderIterator::_visible(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		bool visible = true;
		// a faire
		ioParms.ReturnBool(visible);
	}
	else
		ioParms.ReturnNullValue();
}


bool VJSFolderIterator::_setvisible(VJSParms_setProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != nil)
	{
		bool visible;
		ioParms.GetPropertyValue().GetBool(&visible);
		// a faire
	}
	return true;
}



void VJSFolderIterator::_extension(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VString extension;
		folder->GetExtension(extension);
		ioParms.ReturnString(extension);
	}
	else
		ioParms.ReturnNullValue();
}


bool VJSFolderIterator::_setextension(VJSParms_setProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != nil)
	{
		VString name;
		VString ext;
		ioParms.GetPropertyValue().GetString(ext);
		folder->GetNameWithoutExtension(name);
		folder->Rename(name+L"."+ext, nil);
	}
	return true;
}


void VJSFolderIterator::_name(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VString filename;
		folder->GetName(filename);
		ioParms.ReturnString(filename);
	}
	else
		ioParms.ReturnNullValue();
}


bool VJSFolderIterator::_setname(VJSParms_setProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != nil)
	{
		VString filename;
		ioParms.GetPropertyValue().GetString(filename);
		folder->Rename(filename, nil);
	}
	return true;
}


void VJSFolderIterator::_nameNoExt(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VString filename;
		folder->GetNameWithoutExtension(filename);
		ioParms.ReturnString(filename);
	}
	else
		ioParms.ReturnNullValue();
}


bool VJSFolderIterator::_setnameNoExt(VJSParms_setProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != nil)
	{
		VString filename;
		ioParms.GetPropertyValue().GetString(filename);
		VString ext;
		folder->GetExtension(ext);
		folder->Rename(filename+L"."+ext, nil);
	}
	return true;
}


void VJSFolderIterator::_next(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	ioParms.ReturnBool(inFolderIter->Next());
}


void VJSFolderIterator::_valid(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	ioParms.ReturnBool(inFolderIter->GetFolder() != NULL);
}



void VJSFolderIterator::_readOnly(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder	*folder	= inFolderIter->GetFolder();

	if (folder != NULL) {

		bool	readonly;

#if VERSIONWIN

		// Should always set readonly to false, see comment for _SetReadOnly() function.

		DWORD	attributes;

		if (folder->WIN_GetAttributes(&attributes) == XBOX::VE_OK)

			readonly = attributes & FILE_ATTRIBUTE_READONLY;

		else

			readonly = false;

#elif VERSIONMAC

		uWORD	mode;

		if (folder->MAC_GetPermissions(&mode) == XBOX::VE_OK) 

			readonly = !(mode & 0400);

		else

			readonly = false;

#else	// Postponed Linux Implementation ! (Need refactoring)
	
		readonly = false;

#endif

		ioParms.ReturnBool(readonly);

	} else

		ioParms.ReturnNullValue();
}


bool VJSFolderIterator::_setreadOnly(VJSParms_setProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
#if !VERSION_LINUX  // Postponed Linux Implementation ! (Need refactoring)
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != nil)
	{
		bool readonly;
		ioParms.GetPropertyValue().GetBool(&readonly);
		_SetReadOnly(folder, readonly);
	}
#endif
	return true;
}


void VJSFolderIterator::_path(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VString folderpath;
		folder->GetPath(folderpath, FPS_POSIX, false);
				
		VJSPath::CleanPath(&folderpath);
						
		ioParms.ReturnString(folderpath);
	}
	else
		ioParms.ReturnNullValue();
}


void VJSFolderIterator::_pathRel(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VString folderpath;
		folder->GetPath(folderpath, FPS_POSIX, true);

		VJSPath::CleanPath(&folderpath);

		ioParms.ReturnString(folderpath);
	}
	else
		ioParms.ReturnNullValue();
}


void VJSFolderIterator::_systemPath(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	if (!VJSPath::CanAccessAllFiles( ioParms.GetContext()))
		vThrowError( VE_FS_NOT_AUTHORIZED);
	else
	{
		VFolder* folder = inFolderIter->GetFolder();
		if (folder != NULL)
		{
			VString folderpath;
			folder->GetPath(folderpath, FPS_SYSTEM);
					
			VJSPath::CleanPath(&folderpath);
							
			ioParms.ReturnString(folderpath);
		}
		else
			ioParms.ReturnNullValue();
	}
}


void VJSFolderIterator::_exists(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		ioParms.ReturnBool(folder->Exists());
	}
	else
		ioParms.ReturnNullValue();
}



void VJSFolderIterator::_parent(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VFolder* parent = folder->RetainParentFolder( true);
		ioParms.ReturnFolder( parent);
		ReleaseRefCountable( &parent);
	}
	else
		ioParms.ReturnNullValue();
}


void VJSFolderIterator::_filesystem( VJSParms_getProperty& ioParms, JS4DFolderIterator *inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if ( (folder != NULL) && (folder->GetFileSystem() != NULL) )
	{
		ioParms.ReturnValue( VJSLocalFileSystem::GetInstance( ioParms.GetContext(), folder->GetFileSystem(), true));
	}
	else
		ioParms.ReturnNullValue();
}


void VJSFolderIterator::_firstFile(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VFileIterator* iter = new VFileIterator(folder, FI_NORMAL_FILES | FI_WANT_INVISIBLES | FI_ITERATE_DELETE);
		JS4DFileIterator* result = new JS4DFileIterator(iter);
		ioParms.ReturnValue(VJSFileIterator::CreateInstance(ioParms.GetContext(), result));
		ReleaseRefCountable( &result);
	}
	else
		ioParms.ReturnNullValue();
}


void VJSFolderIterator::_firstFolder(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VFolderIterator* iter = new VFolderIterator(folder, FI_NORMAL_FOLDERS | FI_WANT_INVISIBLES | FI_ITERATE_DELETE);
		JS4DFolderIterator* result = new JS4DFolderIterator(iter);
		ioParms.ReturnValue(VJSFolderIterator::CreateInstance(ioParms.GetContext(), result));
		ReleaseRefCountable( &result);
	}
	else
		ioParms.ReturnNullValue();
}

void VJSFolderIterator::_SetReadOnly (XBOX::VFolder *inFolder, bool inIsReadOnly)
{
// For integration in WAK3, do nothing for now. Need discussion regarding read-only folder behavior.

/*
#if VERSIONWIN

		// See : http://support.microsoft.com/kb/326549

		DWORD	attributes;

		if (inFolder->WIN_GetAttributes(&attributes) == XBOX::VE_OK) {

			if (inIsReadOnly) 

				attributes |= FILE_ATTRIBUTE_READONLY;

			else

				attributes &= ~FILE_ATTRIBUTE_READONLY;

			inFolder->WIN_SetAttributes(attributes);

		}

#elif VERSIONMAC

		// Enable or disable owner read bit. 
	
		uWORD	mode;

		if (inFolder->MAC_GetPermissions(&mode) == XBOX::VE_OK) {

			if (inIsReadOnly)

				mode &= ~0400;

			else

				mode |= 0400;

			inFolder->MAC_SetPermissions(mode);

		}

#else	

	// TODO.		

#endif
*/
}

void VJSFolderIterator::GetDefinition( ClassDefinition& outDefinition)
{
	static inherited::StaticFunction functions[] = 
	{
		{ "setName", js_callStaticFunction<_SetName>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "valid", js_callStaticFunction<_Valid>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "next", js_callStaticFunction<_Next>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "getURL", js_callStaticFunction<_GetURL>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "remove", js_callStaticFunction<_Delete>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "removeContent", js_callStaticFunction<_DeleteContent>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "getFreeSpace", js_callStaticFunction<_GetFreeSpace>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "getVolumeSize", js_callStaticFunction<_GetVolumeSize>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "create", js_callStaticFunction<_Create>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "each", js_callStaticFunction<_eachFile>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "forEachFile", js_callStaticFunction<_eachFile>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "forEachFolder", js_callStaticFunction<_eachFolder>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "parse", js_callStaticFunction<_parse>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "findAllFiles", js_callStaticFunction<_findAllFiles>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "toJSON", js_callStaticFunction<_ToJSON>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ 0, 0, 0}
	};

	static inherited::StaticValue values[] = 
	{
		{ "creationDate", js_getProperty<_creationDate>, js_setProperty<_setcreationDate>, JS4D::PropertyAttributeDontDelete },
		{ "modificationDate", js_getProperty<_modificationDate>, js_setProperty<_setmodificationDate>, JS4D::PropertyAttributeDontDelete },
		{ "visible", js_getProperty<_visible>, js_setProperty<_setvisible>, JS4D::PropertyAttributeDontDelete },
		{ "extension", js_getProperty<_extension>, js_setProperty<_setextension>, JS4D::PropertyAttributeDontDelete },
		{ "name", js_getProperty<_name>, js_setProperty<_setname>, JS4D::PropertyAttributeDontDelete },
		{ "nameNoExt", js_getProperty<_nameNoExt>, js_setProperty<_setnameNoExt>, JS4D::PropertyAttributeDontDelete },
		{ "readOnly", js_getProperty<_readOnly>, js_setProperty<_setreadOnly>, JS4D::PropertyAttributeDontDelete },
		{ "path", js_getProperty<_path>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},
		{ "pathRel", js_getProperty<_pathRel>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},
		#if VERSIONDEBUG
		{ "systemPath", js_getProperty<_systemPath>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},
		#endif
		{ "exists", js_getProperty<_exists>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},
		{ "parent", js_getProperty<_parent>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},
		{ "firstFile", js_getProperty<_firstFile>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},
		{ "firstFolder", js_getProperty<_firstFolder>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},
		{ "files", js_getProperty<_files>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},
		{ "folders", js_getProperty<_folders>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},
		{ "filesystem", js_getProperty<_filesystem>,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
		{ 0, 0, 0, 0}
	};

	outDefinition.className			= "Folder";
	outDefinition.initialize		= js_initialize<_Initialize>;
	outDefinition.finalize			= js_finalize<_Finalize>;
	outDefinition.staticFunctions	= functions;
	outDefinition.staticValues		= values;
}


JS4D::StaticFunction VJSFolderIterator::sConstrFunctions[] =
{
	{	"isFolder",			XBOX::js_callback<VObject, VJSFolderIterator::_IsFolder>,JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
	{	0,				0,										0									},
};

XBOX::VJSObject	VJSFolderIterator::MakeConstructor( const XBOX::VJSContext& inContext)
{
	XBOX::VJSObject	folderConstructor = 
		JS4DMakeConstructor(inContext, Class(), Construct, true, VJSFolderIterator::sConstrFunctions );

	return folderConstructor;
}

//======================================================


JS4DFileIterator*	VJSFileIterator::sDummy = new JS4DFileIterator((VFileIterator *) NULL);

void VJSFileIterator::_Initialize( const VJSParms_initialize& inParms, JS4DFileIterator* inFileIter)
{
	// rien a faire pour l'instant
	inFileIter->Retain();
}


void VJSFileIterator::_Finalize( const VJSParms_finalize& inParms, JS4DFileIterator* inFileIter)
{
	inFileIter->Release();
}

void VJSFileIterator::_IsFile( VJSParms_callStaticFunction &ioParms, VObject *)
{
	VString	pathStringParam;
	if (ioParms.IsStringParam(1) && ioParms.GetStringParam(1, pathStringParam))
	{
		VFile *file = VJSPath::ResolveFileParam( ioParms.GetContext(), pathStringParam);
		ioParms.ReturnBool( (file != NULL) && file->Exists());
		ReleaseRefCountable( &file);
	}
	else
	{
		VJSObject	object(ioParms.GetContext());
		if (ioParms.IsObjectParam(1) && ioParms.GetParamObject(1, object) && object.IsOfClass(VJSFileIterator::Class()))
		{
			JS4DFileIterator *fileIterator = object.GetPrivateData<VJSFileIterator>();
			ioParms.ReturnBool( (fileIterator != NULL) && (fileIterator->GetFile() != NULL) && fileIterator->GetFile()->Exists());
		}
		else
		{
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FILE, "1");
		}
	}
}


void VJSFileIterator::_GetName( VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	VString filename;
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		bool noextension = ioParms.GetBoolParam( 1, L"No Extension", "With Extension");
		if (noextension)
			file->GetNameWithoutExtension(filename);
		else
			file->GetName(filename);
	}
	ioParms.ReturnString(filename);
}



void VJSFileIterator::_SetName( VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	bool ok = false;
	VString filename;
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		if (ioParms.IsStringParam(1) && ioParms.GetStringParam(1, filename))
		{
			VError err = file->Rename(filename);
			if (err == VE_OK)
				ok = true;
		}
	}
	ioParms.ReturnBool(ok);
}


void VJSFileIterator::_Valid( VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	ioParms.ReturnBool(inFileIter->GetFile() != NULL);
}


void VJSFileIterator::_Next( VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	ioParms.ReturnBool(inFileIter->Next());
}


void VJSFileIterator::_IsVisible(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	bool visible = false;
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		EFileAttributes attrib;
		file->GetFileAttributes(attrib);
		visible = ((attrib & FATT_HidenFile) == 0);
	}
	ioParms.ReturnBool(visible);
}


void VJSFileIterator::_IsReadOnly(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	bool readonly = false;
	VFile* file = inFileIter->GetFile();
    
#if VERSIONMAC
    
    uWORD   mode;
    
    if (file != NULL && file->MAC_GetPermissions(&mode) == XBOX::VE_OK)
        
        readonly = !(mode & 0222);

#else
    
	if (file != NULL)
	{
		EFileAttributes attrib;
		file->GetFileAttributes(attrib);
		readonly = ((attrib & FATT_LockedFile) != 0);
	}
    
#endif
    
	ioParms.ReturnBool(readonly);
}


void VJSFileIterator::_SetVisible(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	bool visible = ioParms.GetBoolParam( 1, L"Visible", L"Invisible");
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		EFileAttributes attrib;
		file->GetFileAttributes(attrib);
		if (visible)
			attrib = attrib ^ FATT_HidenFile;
		else
			attrib = attrib | FATT_HidenFile;
		file->SetFileAttributes(attrib);
	}
}


void VJSFileIterator::_SetReadOnly(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	bool readonly = ioParms.GetBoolParam( 1, L"ReadOnly", L"ReadWrite");
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		EFileAttributes attrib;
		file->GetFileAttributes(attrib);
		if (readonly)
			attrib = attrib | FATT_LockedFile;
		else
			attrib = attrib & ~FATT_LockedFile;
		file->SetFileAttributes(attrib);
	}
}


void VJSFileIterator::_GetPath(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	VString folderpath;
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		file->GetPath(folderpath, FPS_POSIX, false);
		VJSPath::CleanPath(&folderpath);		
	}
	ioParms.ReturnString(folderpath);
}


void VJSFileIterator::_GetURL(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	VString folderurl;
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		VString	path;
		file->GetPath(path, FPS_POSIX);
		
		VJSPath::CleanPath(&path);
		
		VURL url;
		
		url.FromFilePath(path);
		url.GetAbsoluteURL(folderurl, ioParms.GetBoolParam( 1, L"Encoded", L"Not Encoded"));
	}
	ioParms.ReturnString(folderurl);
}


void VJSFileIterator::_Delete(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		ioParms.ReturnBool(file->Delete() == VE_OK);
	}
}


void VJSFileIterator::_GetExtension(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		VString extension;
		file->GetExtension(extension);
		ioParms.ReturnString(extension);
	}
}


void VJSFileIterator::_GetSize(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		sLONG8 outsize;
		file->GetSize(&outsize);
		ioParms.ReturnNumber(outsize);
	}	
}


void VJSFileIterator::_GetFreeSpace(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		sLONG8 freebytes, totalbytes;
		file->GetVolumeFreeSpace(&freebytes, &totalbytes, ! ioParms.GetBoolParam( 1, L"NoQuotas", L"WithQuotas"), 0);
		ioParms.ReturnNumber(freebytes);
	}	
}


void VJSFileIterator::_GetVolumeSize(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		sLONG8			volumeSize	= -1;
		XBOX::VFolder	*folder		= file->RetainParentFolder();

		folder->GetVolumeCapacity(&volumeSize);
		XBOX::ReleaseRefCountable<XBOX::VFolder>(&folder);

		ioParms.ReturnNumber(volumeSize);
	}	
}


void VJSFileIterator::_Create(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		ioParms.ReturnBool(file->Create() == VE_OK);
	}	
}


void VJSFileIterator::_Exists(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		ioParms.ReturnBool(file->Exists());
	}	
}


void VJSFileIterator::_GetParent(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		VFolder* parent = file->RetainParentFolder( true);
		ioParms.ReturnFolder( parent);
		ReleaseRefCountable( &parent);
	}	
}


void VJSFileIterator::_MoveTo(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		VFile* dest = ioParms.RetainFileParam( 1);
		if (dest != NULL)
		{
			VError err = file->Move( *dest, ioParms.GetBoolParam( 2, L"OverWrite", L"KeepExisting") ? FCP_Overwrite : FCP_Default);
		}
		else
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FILE, "1");
		ReleaseRefCountable( &dest);
	}
}


void VJSFileIterator::_Slice( VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	sLONG8 paramStart;
	if (!ioParms.GetLong8Param( 1, &paramStart))
		paramStart = 0;

	sLONG8 paramEnd;
	if (!ioParms.GetLong8Param( 2, &paramEnd))
		paramEnd = kMAX_sLONG8;

	VString contentType;
	ioParms.GetStringParam( 3, contentType);	// w3c says slice doesn't inherit master blob mimetype

	JS4DFileIterator *slice = inFileIter->Slice( paramStart, paramEnd, contentType);
	if (slice != NULL)
	{
		ioParms.ReturnValue( CreateInstance( ioParms.GetContext(), slice));
	}
	else
	{
		ioParms.ReturnNullValue();
	}
	
	ReleaseRefCountable( &slice);
}


void VJSFileIterator::_ToJSON( VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	// sc 18/06/2013 WAK0082553, standard 'stringify' doesn't work with File object because this class is also declared as a function,
	// then, we have to implement a 'toJSON' function
	VJSONObject *jsonObj = new VJSONObject();

	jsonObj->SetPropertyAsString( L"type", inFileIter->GetContentType());

	VFile *file = inFileIter->GetFile();
	if (file != NULL)
	{
		VString str;
		file->GetName( str);
		jsonObj->SetPropertyAsString( L"name", str);
		file->GetNameWithoutExtension( str);
		jsonObj->SetPropertyAsString( L"nameNoExt", str);
		file->GetExtension( str);
		jsonObj->SetPropertyAsString( L"extension", str);
		file->GetPath( str, FPS_POSIX, false);
		VJSPath::CleanPath( &str);
		jsonObj->SetPropertyAsString( L"path", str);
		file->GetPath( str, FPS_POSIX, true);
		VJSPath::CleanPath( &str);
		jsonObj->SetPropertyAsString( L"pathRel", str);
		#if VERSIONDEBUG
		if (VJSPath::CanAccessAllFiles( ioParms.GetContext()))
		{
			file->GetPath( str, FPS_SYSTEM);
			VJSPath::CleanPath( &str);
			jsonObj->SetPropertyAsString( L"systemPath", str);
		}
		#endif
		jsonObj->SetPropertyAsBool( L"exists", file->Exists());

		if (file->Exists())
		{
			EFileAttributes attrib;
			if (file->GetFileAttributes( attrib) == VE_OK)
			{
				jsonObj->SetPropertyAsBool( L"visible", (attrib & FATT_HidenFile) == 0);
				jsonObj->SetPropertyAsBool( L"readOnly", (attrib & FATT_LockedFile) != 0);
			}

			VTime creationDate, modificationDate;
			if (file->GetTimeAttributes( &modificationDate, &creationDate) == VE_OK)
			{
				creationDate.GetJSONString( str, JSON_Default);
				jsonObj->SetPropertyAsString( L"creationDate", str);
				modificationDate.GetJSONString( str, JSON_Default);
				jsonObj->SetPropertyAsString( L"lastModifiedDate", str);
			}
		}
	}

	ioParms.ReturnJSONValue( VJSONValue( jsonObj));
	
	ReleaseRefCountable( &jsonObj);
}


void VJSFileIterator::_creationDate(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		VTime dd;
		file->GetTimeAttributes(nil, &dd);
		ioParms.ReturnTime(dd);
	}
	else
		ioParms.ReturnNullValue();
}


bool VJSFileIterator::_setcreationDate(VJSParms_setProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != nil)
	{
		VTime dd;
		ioParms.GetPropertyValue().GetTime(dd);
		file->SetTimeAttributes(nil, &dd);
	}
	return true;
}


void VJSFileIterator::_lastModifiedDate (VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		VTime dd;
		file->GetTimeAttributes(&dd);
		ioParms.ReturnTime(dd);
	}
	else
		ioParms.ReturnNullValue();
}

void VJSFileIterator::_type (VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter)
{
	ioParms.ReturnString(inFileIter->GetContentType());		
}


bool VJSFileIterator::_setLastModifiedDate(VJSParms_setProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != nil)
	{
		VTime dd;
		ioParms.GetPropertyValue().GetTime(dd);
		file->SetTimeAttributes(&dd);
	}
	return true;
}




void VJSFileIterator::_visible(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		EFileAttributes attrib;
		file->GetFileAttributes(attrib);
		bool visible = ((attrib & FATT_HidenFile) == 0);
		ioParms.ReturnBool(visible);
	}
	else
		ioParms.ReturnNullValue();
}

bool VJSFileIterator::_setvisible(VJSParms_setProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != nil)
	{
		bool visible;
		ioParms.GetPropertyValue().GetBool(&visible);
		EFileAttributes attrib;
		file->GetFileAttributes(attrib);
		if (visible)
			attrib = attrib ^ FATT_HidenFile;
		else
			attrib = attrib | FATT_HidenFile;
		file->SetFileAttributes(attrib);
	}
	return true;
}


void VJSFileIterator::_extension(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		VString extension;
		file->GetExtension(extension);
		ioParms.ReturnString(extension);
	}
	else
		ioParms.ReturnNullValue();
}


bool VJSFileIterator::_setextension(VJSParms_setProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != nil)
	{
		VString name;
		VString ext;
		ioParms.GetPropertyValue().GetString(ext);
		file->GetNameWithoutExtension(name);
		file->Rename(name+L"."+ext);
	}
	return true;
}


void VJSFileIterator::_name(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		VString filename;
		file->GetName(filename);
		ioParms.ReturnString(filename);
	}
	else
		ioParms.ReturnNullValue();
}


bool VJSFileIterator::_setname(VJSParms_setProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != nil)
	{
		VString filename;
		ioParms.GetPropertyValue().GetString(filename);
		file->Rename(filename);
	}
	return true;
}


void VJSFileIterator::_nameNoExt(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		VString filename;
		file->GetNameWithoutExtension(filename);
		ioParms.ReturnString(filename);
	}
	else
		ioParms.ReturnNullValue();
}


bool VJSFileIterator::_setnameNoExt(VJSParms_setProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != nil)
	{
		VString filename;
		ioParms.GetPropertyValue().GetString(filename);
		VString ext;
		file->GetExtension(ext);
		file->Rename(filename+L"."+ext);
	}
	return true;
}


void VJSFileIterator::_next(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter)
{
	ioParms.ReturnBool(inFileIter->Next());
}


void VJSFileIterator::_valid(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter)
{
	ioParms.ReturnBool(inFileIter->GetFile() != NULL);
}



void VJSFileIterator::_readOnly(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
#if VERSIONMAC
      
        bool    readonly;
        uWORD   mode;
        
        if (file->MAC_GetPermissions(&mode) == XBOX::VE_OK)
            
            readonly = !(mode & 0222);
        
        else
            
            readonly = false;

#else
        
		EFileAttributes attrib;
		file->GetFileAttributes(attrib);
		bool readonly = ((attrib & FATT_LockedFile) != 0);
     
#endif
        
		ioParms.ReturnBool(readonly);
	}
	else
		ioParms.ReturnNullValue();
}


bool VJSFileIterator::_setreadOnly(VJSParms_setProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != nil)
	{
		bool readonly;
		ioParms.GetPropertyValue().GetBool(&readonly);
		EFileAttributes attrib;
		file->GetFileAttributes(attrib);
		if (readonly)
			attrib = attrib | FATT_LockedFile;
		else
			attrib = attrib & ~FATT_LockedFile;
		file->SetFileAttributes(attrib);
	}
	return true;
}


void VJSFileIterator::_path(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		VString folderpath;
		file->GetPath(folderpath, FPS_POSIX, false);
				
		VJSPath::CleanPath(&folderpath);
				
		ioParms.ReturnString(folderpath);
	}
	else
		ioParms.ReturnNullValue();
}



void VJSFileIterator::_pathRel(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		VString folderpath;
		file->GetPath(folderpath, FPS_POSIX, true);
				
		VJSPath::CleanPath(&folderpath);
				
		ioParms.ReturnString(folderpath);
	}
	else
		ioParms.ReturnNullValue();
}


void VJSFileIterator::_systemPath(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter)
{
	if (!VJSPath::CanAccessAllFiles( ioParms.GetContext()))
		vThrowError( VE_FS_NOT_AUTHORIZED);
	else
	{
		VFile* file = inFileIter->GetFile();
		if (file != NULL)
		{
			VString folderpath;
			file->GetPath(folderpath, FPS_SYSTEM);
			
			VJSPath::CleanPath(&folderpath);

			ioParms.ReturnString(folderpath);
		}
		else
			ioParms.ReturnNullValue();
	}
}


void VJSFileIterator::_exists(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		ioParms.ReturnBool(file->Exists());
	}
	else
		ioParms.ReturnNullValue();
}



void VJSFileIterator::_parent(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		VFolder* parent = file->RetainParentFolder( true);
		ioParms.ReturnFolder( parent);
		ReleaseRefCountable( &parent);
	}
	else
		ioParms.ReturnNullValue();
}


void VJSFileIterator::_filesystem( VJSParms_getProperty& ioParms, JS4DFileIterator *inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if ( (file != NULL) && (file->GetFileSystem() != NULL) )
	{
		ioParms.ReturnValue( VJSLocalFileSystem::GetInstance( ioParms.GetContext(), file->GetFileSystem(), true));
	}
	else
		ioParms.ReturnNullValue();
}


void VJSFileIterator::Construct( VJSParms_construct& ioParms)
{
#if USE_V8_ENGINE
    XBOX::VJSObject constructedObject = ioParms.fConstructedObject;
#else
    XBOX::VJSObject	constructedObject(ioParms.GetContext());
#endif

	JS4DFolderIterator* folderiter = ioParms.GetParamObjectPrivateData<VJSFolderIterator>(1);
	if (folderiter == NULL)
	{
		VString pathStringParam;
		/*bool isOK1 = ioParms.IsStringParam(1);
		bool isOK2 = ioParms.GetStringParam(1, pathStringParam);
		bool isOK3 = pathStringParam.IsEmpty();
		if (isOK1 && isOK2 && !isOK3)*/
		if (ioParms.IsStringParam(1) && ioParms.GetStringParam(1, pathStringParam) && ! pathStringParam.IsEmpty() )
		{
			VFile *file = VJSPath::ResolveFileParam( ioParms.GetContext(), pathStringParam);
			if (file != NULL)
			{
				constructedObject.MakeFile(file);
			}
			else
			{
				vThrowError( VE_JVSC_NOT_A_FILE);
			}
			ReleaseRefCountable(&file);
		}
		else
			_ConstructFromW3CFS(ioParms,constructedObject);
	}
	else
	{
		VFolder* folder = folderiter->GetFolder();
		if (folder != NULL)
		{
			VString filename;
			if (ioParms.IsStringParam(2) && ioParms.GetStringParam(2, filename))
			{
				VFile* file = folder->RetainRelativeFile( filename, FPS_POSIX);
				constructedObject.MakeFile( file);
				ReleaseRefCountable( &file);
			}
			else 
				vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2");
		} 	

	}
	
	ioParms.ReturnConstructedObject( constructedObject);
}

// Implement : 
//
//	new File(fileSystem, path);
//	new File(directoryEntry, path);
//	new File(fileEntry);

void VJSFileIterator::_ConstructFromW3CFS (VJSParms_withArguments &ioParms, XBOX::VJSObject& ioObject)
{
	//ioObject.ClearRef();

	VJSObject	object(ioParms.GetContext());
	ioParms.GetParamObject(1, object);

	if (object.IsOfClass(VJSFileEntryClass::Class()) || object.IsOfClass(VJSFileEntrySyncClass::Class()))
	{
		VJSEntry	*entry;

		if (object.IsOfClass(VJSFileEntryClass::Class()))
			entry = object.GetPrivateData<VJSFileEntryClass>();
		else
			entry = object.GetPrivateData<VJSFileEntrySyncClass>();

		xbox_assert(entry != NULL);
		
		if (!entry->IsFile())
		{
			vThrowError( VE_JVSC_WRONG_PARAMETER, "1");
		}
		else
		{
			entry->File(ioParms.GetContext(), ioObject);
		}		
	}
	else
	{
		bool			isOk;
		VString			relativePath;
		VFilePath		rootPath;				
		VJSFileSystem*	fileSystem = NULL;

		if (object.IsOfClass(VJSFileSystemClass::Class()) || object.IsOfClass(VJSFileSystemSyncClass::Class()))
		{
			if (object.IsOfClass(VJSFileSystemClass::Class()))
				fileSystem = object.GetPrivateData<VJSFileSystemClass>();
			else
				fileSystem = object.GetPrivateData<VJSFileSystemSyncClass>();

			xbox_assert(fileSystem != NULL);
			rootPath = fileSystem->GetRoot();
			isOk = true;
		}
		else if (object.IsOfClass(VJSDirectoryEntryClass::Class()) || object.IsOfClass(VJSDirectoryEntrySyncClass::Class()))
		{
			VJSEntry	*entry;

			if (object.IsOfClass(VJSDirectoryEntryClass::Class()))
				entry = object.GetPrivateData<VJSDirectoryEntryClass>();
			else
				entry = object.GetPrivateData<VJSDirectoryEntrySyncClass>();

			xbox_assert(entry != NULL);
			fileSystem = entry->GetFileSystem();
			rootPath = entry->GetPath();
			isOk = true;
		}
		else
		{
			vThrowError( VE_JVSC_WRONG_PARAMETER, "1");
			isOk = false;
		}

		// get mandatory relative file path
		if (isOk && !ioParms.GetStringParam(2, relativePath))
		{
			vThrowError( VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2");
			isOk = false;
		}
				
		if (isOk)
		{
			VFilePath path( rootPath, relativePath, FPS_POSIX);
			if (!path.IsFile())
			{
				vThrowError( VE_JVSC_NOT_A_FILE);
			}
			else if ( (fileSystem != NULL) && !fileSystem->GetFileSystem()->IsAuthorized( path) )
			{
				VJSFileErrorClass::Throw( VJSFileErrorClass::SECURITY_ERR);
			}
			else
			{
				VFile *file = new VFile( path, fileSystem->GetFileSystem());
				if (file == NULL) 
				{
					vThrowError( VE_MEMORY_FULL);
				}
				else
				{
					ioObject.MakeFile(file);
				}
				ReleaseRefCountable(&file);
			}
		}
	}

}

void VJSFileIterator::GetDefinition( ClassDefinition& outDefinition)
{
	static inherited::StaticFunction functions[] = 
	{
		{ "setName", js_callStaticFunction<_SetName>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "valid", js_callStaticFunction<_Valid>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "next", js_callStaticFunction<_Next>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "getURL", js_callStaticFunction<_GetURL>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "remove", js_callStaticFunction<_Delete>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "getFreeSpace", js_callStaticFunction<_GetFreeSpace>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "getVolumeSize", js_callStaticFunction<_GetVolumeSize>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "create", js_callStaticFunction<_Create>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "moveTo", js_callStaticFunction<_MoveTo>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "slice", js_callStaticFunction<_Slice>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },	// inherited from Blob
		{ "toJSON", js_callStaticFunction<_ToJSON>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ 0, 0, 0}
	};

	static inherited::StaticValue values[] = 
	{
		{ "creationDate", js_getProperty<_creationDate>, js_setProperty<_setcreationDate>, JS4D::PropertyAttributeDontDelete },
		{ "lastModifiedDate", js_getProperty<_lastModifiedDate>, js_setProperty<_setLastModifiedDate>, JS4D::PropertyAttributeDontDelete },
		{ "type", js_getProperty<_type>, NULL, JS4D::PropertyAttributeDontDelete },	// inherited from Blob
		{ "visible", js_getProperty<_visible>, js_setProperty<_setvisible>, JS4D::PropertyAttributeDontDelete },
		{ "extension", js_getProperty<_extension>, js_setProperty<_setextension>, JS4D::PropertyAttributeDontDelete },
		{ "name", js_getProperty<_name>, js_setProperty<_setname>, JS4D::PropertyAttributeDontDelete },
		{ "nameNoExt", js_getProperty<_nameNoExt>, js_setProperty<_setnameNoExt>, JS4D::PropertyAttributeDontDelete },
		{ "readOnly", js_getProperty<_readOnly>, js_setProperty<_setreadOnly>, JS4D::PropertyAttributeDontDelete },
		{ "path", js_getProperty<_path>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},
		{ "pathRel", js_getProperty<_pathRel>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},
		#if VERSIONDEBUG
		{ "systemPath", js_getProperty<_systemPath>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},
		#endif
		{ "exists", js_getProperty<_exists>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},
		{ "parent", js_getProperty<_parent>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},
		{ "filesystem", js_getProperty<_filesystem>,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
		{ 0, 0, 0, 0}
	};

	outDefinition.className = "File";
	outDefinition.parentClass = VJSBlob::Class();
	outDefinition.initialize = js_initialize<_Initialize>;
	outDefinition.finalize = js_finalize<_Finalize>;
	outDefinition.staticFunctions = functions;
	outDefinition.staticValues = values;
}

JS4D::StaticFunction VJSFileIterator::sConstrFunctions[] =
{
	{	"isFile",		XBOX::js_callback<VObject, VJSFileIterator::_IsFile>,JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
	{	0,				0,										0									},
};

VJSObject VJSFileIterator::MakeConstructor( const VJSContext& inContext)
{
	VJSObject fileConstructor =
		JS4DMakeConstructor(inContext, Class(), Construct, true, VJSFileIterator::sConstrFunctions );

	return fileConstructor;
}


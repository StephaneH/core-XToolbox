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

#include "VJSW3CFileSystem.h"

#include "VJSContext.h"
#include "VJSGlobalClass.h"

#include "VJSEvent.h"
#include "VJSRuntime_file.h"

#define HIDDEN_ATTRIBUTE	(JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeReadOnly)

USING_TOOLBOX_NAMESPACE

const char *VJSLocalFileSystem::sTEMPORARY_PREFIX	= "Temporary";		// Followed by a random number.
const char *VJSLocalFileSystem::sPERSISTENT_NAME	= "Persistent";
const char *VJSLocalFileSystem::sRELATIVE_PREFIX	= "Relative";		// Followed by a path to authorize.

void VJSLocalFileSystem::AddInterfacesSupport (const VJSContext &inContext)
{
	JS4DMakeConstructor(inContext, VJSFileSystemSyncClass::Class(), NULL);
	JS4DMakeConstructor(inContext, VJSFileSystemClass::Class(), NULL);
	JS4DMakeConstructor(inContext, VJSDirectoryEntryClass::Class(), NULL);
	JS4DMakeConstructor(inContext, VJSDirectoryEntrySyncClass::Class(), NULL);
	JS4DMakeConstructor(inContext, VJSDirectoryReaderClass::Class(), NULL);
	JS4DMakeConstructor(inContext, VJSDirectoryReaderSyncClass::Class(), NULL);
	JS4DMakeConstructor(inContext, VJSFileEntryClass::Class(), NULL);
	JS4DMakeConstructor(inContext, VJSFileEntrySyncClass::Class(), NULL);
	JS4DMakeConstructor(inContext, VJSMetadataClass::Class(), NULL);
	JS4DMakeConstructor(inContext, VJSFileErrorClass::Class(), NULL);
	JS4DMakeConstructor(inContext, VJSFileErrorConstructorClass::Class(), NULL);

	VJSObject	globalObject(inContext.GetGlobalObject());

	globalObject.SetProperty("TEMPORARY", (sLONG) TEMPORARY, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
	globalObject.SetProperty("PERSISTENT", (sLONG) PERSISTENT, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);

	VJSObject	function(inContext);

	function.MakeCallback(js_callback<void, _requestFileSystem>);
	globalObject.SetProperty("requestFileSystem", function, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);

	function.MakeCallback(js_callback<void, _fileSystem>);
	globalObject.SetProperty("FileSystem", function, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
	function = globalObject.GetPropertyAsObject("FileSystem");
	_addTypeConstants(function);

	function.MakeCallback(js_callback<void, _resolveLocalFileSystemURL>);
	globalObject.SetProperty("resolveLocalFileSystemURL", function, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);

	function.MakeCallback(js_callback<void, _requestFileSystemSync>);
	globalObject.SetProperty("requestFileSystemSync", function, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
	globalObject.SetProperty("FileSystemSync", function, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
	function = globalObject.GetPropertyAsObject("FileSystemSync");
	_addTypeConstants(function);

	function.MakeCallback(js_callback<void, _resolveLocalFileSystemSyncURL>);
	globalObject.SetProperty("resolveLocalFileSystemSyncURL", function, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
}

VJSLocalFileSystem *VJSLocalFileSystem::CreateLocalFileSystem (VJSContext &inContext, const VFilePath &inRelativeRoot)
{	
	return new VJSLocalFileSystem(inContext, inRelativeRoot);
}


VJSFileSystem *VJSLocalFileSystem::RetainTemporaryFileSystem (VSize inQuota)
{
	VString	name;

	do {

		name = sTEMPORARY_PREFIX;
		name.AppendLong(VSystem::Random() & 0x0fffffff);

	} while (fFileSystems.find(name) != fFileSystems.end());
		
	VFilePath	root(fProjectPath, VString(name).AppendUniChar('/'), FPS_POSIX);
	VJSFileSystem	*fileSystem = new VJSFileSystem(name, root, eFSO_Writable | eFSO_Volatile, inQuota);

	if (fileSystem != NULL)
	{
		VFolder	folder(root);
		VError	error = folder.Create();

		if (error != VE_OK)
		{
			ReleaseRefCountable(&fileSystem);
		}
		else
		{
			xbox_assert(folder.Exists());
			fFileSystems[name] = fileSystem;
		}	
	}
	
	return fileSystem;
}

VJSFileSystem *VJSLocalFileSystem::RetainPersistentFileSystem (VSize inQuota)
{	  
	VJSFileSystem	*fileSystem;
	SMap::iterator	it = fFileSystems.find(sPERSISTENT_NAME);
	
	if (it == fFileSystems.end())
	{
		VFilePath	root(fProjectPath, VString(sPERSISTENT_NAME).AppendUniChar('/'), FPS_POSIX);
		fileSystem = new VJSFileSystem(sPERSISTENT_NAME, root, eFSO_Writable, inQuota);
		if (fileSystem != NULL)
		{
			VFolder	folder(root);
			VError	error;

			if (!folder.Exists()) {

				if ((error = folder.Create() != VE_OK)) {
		
					vThrowError(error);
					ReleaseRefCountable<VJSFileSystem>(&fileSystem);

				} else {

					xbox_assert(folder.Exists());
					fFileSystems[sPERSISTENT_NAME] = fileSystem;
			
				}

			} else 

				fFileSystems[sPERSISTENT_NAME] = fileSystem;

		} else 

			vThrowError(VE_MEMORY_FULL);

	}
	else
	{
		fileSystem = it->second.Retain();
	}

	return fileSystem;
}


VJSFileSystem *VJSLocalFileSystem::RetainNamedFileSystem (const VJSContext &inContext, const VString &inFileSystemName)
{
	VJSFileSystem *jsFileSystem	= NULL;
	SMap::iterator	it = fFileSystems.find( inFileSystemName);
	if (it == fFileSystems.end())
	{
		IJSRuntimeDelegate	*rtDeleguate = inContext.GetGlobalObjectPrivateInstance()->GetRuntimeDelegate();
		xbox_assert(rtDeleguate != NULL);

		VFileSystemNamespace	*fsNameSpace = rtDeleguate->RetainRuntimeFileSystemNamespace();

		if (fsNameSpace != NULL)
		{
			VFileSystem	*fileSystem = fsNameSpace->RetainFileSystem( inFileSystemName);
			if (fileSystem != NULL)
			{
				jsFileSystem = new VJSFileSystem(fileSystem);
				if (jsFileSystem != NULL)
					fFileSystems.insert( SMap::value_type( inFileSystemName, jsFileSystem));
			}
			else
			{
				VJSFileErrorClass::Throw( VJSFileErrorClass::NOT_FOUND_ERR);
			}
			ReleaseRefCountable( &fileSystem);
		}
		ReleaseRefCountable( &fsNameSpace);
	}
	else
	{
		jsFileSystem = it->second.Retain();
	}

	return jsFileSystem;
}


VJSFileSystem *VJSLocalFileSystem::RetainJSFileSystemForFileSystem( const VJSContext& inContext, VFileSystem *inFileSystem)
{
	VJSFileSystem *jsFileSystem	= NULL;
	SMap::iterator	it = fFileSystems.find( inFileSystem->GetName());
	if (it == fFileSystems.end())
	{
		jsFileSystem = new VJSFileSystem( inFileSystem);
		if (jsFileSystem != NULL)
			fFileSystems.insert( SMap::value_type( inFileSystem->GetName(), jsFileSystem));
	}
	else
	{
		jsFileSystem = it->second.Retain();
	}
	return jsFileSystem;
}


void VJSLocalFileSystem::ReleaseAllFileSystems ()
{
	// This will release refcount of all file systems in map.

	fFileSystems.clear();
}

void VJSLocalFileSystem::RequestFileSystem (const VJSContext &inContext, sLONG inType, VSize inQuota, VJSObject& outResult, bool inIsSync, const VString &inFileSystemName)
{
	VJSFileSystem	*fileSystem;

	switch (inType)
	{

		case NAMED_FS:		fileSystem = RetainNamedFileSystem(inContext, inFileSystemName); break;
		case TEMPORARY:		fileSystem = RetainTemporaryFileSystem(inQuota); break;
		case PERSISTENT:	fileSystem = RetainNamedFileSystem(inContext, CVSTR( "DATA")); break;
		
		default:			xbox_assert(false); fileSystem = NULL; break;

	}

	outResult = VJSLocalFileSystem::GetInstance(inContext, fileSystem, inIsSync);

	ReleaseRefCountable(&fileSystem);
}

void VJSLocalFileSystem::ResolveURL (const VJSContext &inContext, const VString &inURL, bool inIsSync, VJSObject& outResult)
{
	// TODO: should we loop thru all available namespace instead of just "PROJECT" ?
	
	VJSFileSystem	*fs = RetainNamedFileSystem( inContext, "PROJECT");

	if (fs != NULL)
	{
		xbox_assert(fs != NULL);

		VFilePath	path(fs->GetRoot());
		bool isFile;
		if (fs->ParseURL(inURL, &path, true /* inThrowErrorIfNotFound */, &isFile) == VE_OK)
		{
			outResult = VJSEntry::CreateObject(inContext, inIsSync, fs, path, isFile);
		}
	}

	ReleaseRefCountable( &fs);
}

VJSLocalFileSystem::VJSLocalFileSystem (VJSContext &inContext, const VFilePath &inRelativeRoot)
{
	VFolder	*folder;
	
	folder = inContext.GetGlobalObjectPrivateInstance()->GetRuntimeDelegate()->RetainScriptsFolder();
	fProjectPath = folder->GetPath();
	ReleaseRefCountable<VFolder>(&folder);
}

VJSLocalFileSystem::~VJSLocalFileSystem ()
{
	xbox_assert(fFileSystems.empty());
}

void VJSLocalFileSystem::_requestFileSystem (VJSParms_callStaticFunction &ioParms, void *)
{
	_requestFileSystem(ioParms, false);
}

void VJSLocalFileSystem::_resolveLocalFileSystemURL (VJSParms_callStaticFunction &ioParms, void *)
{
	_resolveLocalFileSystemURL(ioParms, false);
}

void VJSLocalFileSystem::_requestFileSystemSync (VJSParms_callStaticFunction &ioParms, void *)
{
	_requestFileSystem(ioParms, true);
}

void VJSLocalFileSystem::_resolveLocalFileSystemSyncURL (VJSParms_callStaticFunction &ioParms, void *)
{
	_resolveLocalFileSystemURL(ioParms, true);
}

void VJSLocalFileSystem::_fileSystem (VJSParms_callStaticFunction &ioParms, void *)
{
	_requestFileSystem(ioParms, false, true);
}

void VJSLocalFileSystem::_requestFileSystem (VJSParms_callStaticFunction &ioParms, bool inIsSync, bool inFromFunction)
{	
	sLONG			type;
	VString	fileSystemName;
	
	if (ioParms.IsStringParam(1)) {

		if (!ioParms.GetStringParam(1, fileSystemName)) {

			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
			return;

		} else

			type = NAMED_FS;

	} else {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &type)) {

			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
			return;

		}
		if (type < FS_FIRST_TYPE || type > FS_LAST_TYPE) {

			vThrowError(VE_JVSC_FS_WRONG_TYPE, "1");
			return;

		}

	}
	
	sLONG	size;
	VSize	quota;
	sLONG	callbackStartIndex;

	if (type == NAMED_FS) {
		
		quota = size = 0;			// Not applicable for named file systems.
		callbackStartIndex = 2;

	} else {

		if (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &size)) {

			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");
			return;

		}
		quota = size;
		if (size < 0)
		{

			vThrowError(VE_JVSC_WRONG_NUMBER_ARGUMENT, "2");
			return;

		}
		callbackStartIndex = 3;

	}

	VJSObject	successCallback(ioParms.GetContext());
	VJSObject	errorCallback(ioParms.GetContext());

	if (!inIsSync && !inFromFunction) {

		VString	argumentNumber;

		if (!ioParms.IsObjectParam(callbackStartIndex) || !ioParms.GetParamFunc(callbackStartIndex, successCallback)) {

			argumentNumber.AppendLong(callbackStartIndex);
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, argumentNumber);
			return;

		}
		if (ioParms.CountParams() >= callbackStartIndex + 1 
		&& (!ioParms.IsObjectParam(callbackStartIndex + 1) || !ioParms.GetParamFunc(callbackStartIndex + 1, errorCallback))) {

			argumentNumber.AppendLong(callbackStartIndex + 1);
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, argumentNumber);
			return;

		}

	}

	VJSWorker			*worker				= VJSWorker::RetainWorker(ioParms.GetContext());
	VJSLocalFileSystem	*localFileSystem	= testAssert( worker != NULL) ? worker->RetainLocalFileSystem() : NULL;

	// If worker has failed to create a local file system, that error has already been reported. Ignore.
	if (localFileSystem != NULL)
	{
		// If inFromFunction is true, this will return an asynchronous FileSystem object. 
		// We use this to implement FileSystem().

		if (inIsSync || inFromFunction)
		{
			VJSObject	resultObject(ioParms.GetContext());
			localFileSystem->RequestFileSystem(ioParms.GetContext(), type, quota, resultObject, inIsSync, fileSystemName);
			ioParms.ReturnValue(resultObject);
		}
		else
		{
			VJSW3CFSEvent	*request = VJSW3CFSEvent::RequestFS(localFileSystem, type, quota, fileSystemName, successCallback, errorCallback);
			if (request != NULL)
				worker->QueueEvent(request);
		}
	}

	ReleaseRefCountable(&worker);
	ReleaseRefCountable(&localFileSystem);
}

void VJSLocalFileSystem::_resolveLocalFileSystemURL (VJSParms_callStaticFunction &ioParms, bool inIsSync)
{
	VString	url;

	if (!ioParms.IsStringParam(1) || !ioParms.GetStringParam(1, url)) {

		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
		return;

	}

	VJSObject	successCallback(ioParms.GetContext());
	VJSObject	errorCallback(ioParms.GetContext());

	if (!inIsSync) {

		if (!ioParms.IsObjectParam(2) || !ioParms.GetParamFunc(2, successCallback)) {

			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "2");
			return;

		}
		if (ioParms.CountParams() >= 3 
		&& (!ioParms.IsObjectParam(3) || !ioParms.GetParamFunc(3, errorCallback))) {

			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "3");
			return;

		}

	}

	VJSWorker			*worker				= VJSWorker::RetainWorker(ioParms.GetContext());
	VJSLocalFileSystem	*localFileSystem	= worker->RetainLocalFileSystem();

	// See comment for _requestFileSystem().
	
	if (localFileSystem == NULL) {

		ReleaseRefCountable<VJSWorker>(&worker);
		return;		

	}

	if (inIsSync)
	{
		VJSObject	resultObject(ioParms.GetContext());
		localFileSystem->ResolveURL(ioParms.GetContext(), url, true, resultObject);
		ioParms.ReturnValue(resultObject);
	}
	else
	{

		VJSW3CFSEvent	*request;

		if ((request = VJSW3CFSEvent::ResolveURL(localFileSystem, url, successCallback, errorCallback)) == NULL)

			vThrowError(VE_MEMORY_FULL);

		else

			worker->QueueEvent(request);

	}	
	ReleaseRefCountable<VJSWorker>(&worker);
	ReleaseRefCountable<VJSLocalFileSystem>(&localFileSystem);
}

void VJSLocalFileSystem::_addTypeConstants (VJSObject &inFileSystemObject)
{
	inFileSystemObject.SetProperty( CVSTR( "TEMPORARY"), (sLONG) TEMPORARY, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
	inFileSystemObject.SetProperty( CVSTR( "PERSISTENT"), (sLONG) PERSISTENT, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
	
	VJSObject fs( inFileSystemObject.GetContext());
	fs.MakeEmpty();
	inFileSystemObject.SetProperty( CVSTR( "_fs"), fs, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
}


VJSObject VJSLocalFileSystem::GetInstance (const VJSContext &inContext, VJSFileSystem *inFileSystem, bool inSync)
{
	// Each FileSystemSync object is unique.

	VJSObject	globalObject( inContext.GetGlobalObject());
	VJSObject	function( globalObject.GetPropertyAsObject( inSync ? CVSTR( "FileSystemSync") : CVSTR( "FileSystem")));
	VJSObject	fsMap( function.GetPropertyAsObject( CVSTR("_fs")));
	VJSObject	fs( (inFileSystem != NULL) ? fsMap.GetPropertyAsObject( inFileSystem->GetName()) : VJSObject( inContext));

	if ( !fs.IsObject() && (inFileSystem != NULL) )
	{
		fs = inSync ? VJSFileSystemSyncClass::CreateInstance(inContext, inFileSystem) : VJSFileSystemClass::CreateInstance(inContext, inFileSystem);
		fsMap.SetProperty( inFileSystem->GetName(), fs, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete);
	}

	return fs;
}


VJSObject VJSLocalFileSystem::GetInstance( const VJSContext& inContext, VFileSystem *inFileSystem, bool inSync)
{
	// Each FileSystemSync object is unique.

	VJSObject	globalObject( inContext.GetGlobalObject());
	VJSObject	function( globalObject.GetPropertyAsObject( inSync ? CVSTR( "FileSystemSync") : CVSTR( "FileSystem")));
	VJSObject	fsMap( function.GetPropertyAsObject( CVSTR("_fs")));
	VJSObject	fs( (inFileSystem != NULL) ? fsMap.GetPropertyAsObject( inFileSystem->GetName()) : VJSObject( inContext));

	if ( !fs.IsObject() && (inFileSystem != NULL) )
	{
		VJSWorker *worker = VJSWorker::RetainWorker( inContext);
		VJSLocalFileSystem *localFileSystem = worker->RetainLocalFileSystem();

		VJSFileSystem *jsFileSystem	= localFileSystem->RetainJSFileSystemForFileSystem( inContext, inFileSystem);
		
		ReleaseRefCountable( &localFileSystem);
		ReleaseRefCountable( &worker);

		fs = inSync ? VJSFileSystemSyncClass::CreateInstance( inContext, jsFileSystem) : VJSFileSystemClass::CreateInstance( inContext, jsFileSystem);
		fsMap.SetProperty( inFileSystem->GetName(), fs, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete);
	}

	return fs;
}

//==============================================================================================


VJSFileSystem::VJSFileSystem (const VString &inName, const VFilePath &inRoot, EFSOptions inOptions, VSize inQuota)
{
	fFileSystem = VFileSystem::Create(inName, inRoot, inOptions, inQuota);
	xbox_assert(fFileSystem != NULL);
}

VJSFileSystem::VJSFileSystem (VFileSystem *inFileSystem)
{
	xbox_assert(inFileSystem != NULL);

	fFileSystem = RetainRefCountable(inFileSystem);
}

VJSFileSystem::~VJSFileSystem ()
{
	ReleaseRefCountable(&fFileSystem);
}


VError VJSFileSystem::ParseURL (const VString &inURL, VFilePath *ioPath, bool inThrowErrorIfNotFound, bool *outIsFile)
{
	xbox_assert(ioPath != NULL && outIsFile != NULL);

	VURL		url(inURL, true);
	VFilePath	currentFolder(*ioPath);
	VError	error = fFileSystem->ParseURL(url, currentFolder, inThrowErrorIfNotFound, ioPath);

	if (error == VE_OK)
	{
		*outIsFile = ioPath->IsFile();
	}

	return error;
}


VJSObject VJSFileSystem::GetRootEntryObject( const VJSContext& inContext, bool inIsSync)
{
	VJSObject fs( VJSLocalFileSystem::GetInstance( inContext, this, inIsSync));
	return fs.GetPropertyAsObject( CVSTR( "root"));
}


void VJSFileSystemClass::GetDefinition (ClassDefinition &outDefinition)
{	
    outDefinition.className		= "FileSystem";
	outDefinition.initialize	= js_initialize<_Initialize>;
	outDefinition.finalize		= js_finalize<_Finalize>;
	outDefinition.hasInstance	= js_hasInstance<_HasInstance>;
}


void VJSFileSystemClass::_Initialize (const VJSParms_initialize &inParms, VJSFileSystem *inFileSystem)
{
	xbox_assert(inFileSystem != NULL);

	inFileSystem->Retain();

	VString path;
	inFileSystem->GetRoot().GetPosixPath(path);

	VJSObject	rootObject( VJSEntry::CreateRootObject(inParms.GetContext(), false, inFileSystem));
	inParms.GetObject().SetProperty("name", inFileSystem->GetName(), JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
	inParms.GetObject().SetProperty("path", path, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
	inParms.GetObject().SetProperty("root", rootObject, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
}

void VJSFileSystemClass::_Finalize (const VJSParms_finalize &inParms, VJSFileSystem *inFileSystem)
{
	xbox_assert(inFileSystem != NULL);
	inFileSystem->Release();
}

bool VJSFileSystemClass::_HasInstance (const VJSParms_hasInstance &inParms) 
{
	xbox_assert(inParms.GetObject().HasSameRef(inParms.GetObjectToTest()));
 
	return inParms.GetObject().IsOfClass(VJSFileSystemClass::Class());
}

void VJSFileSystemSyncClass::GetDefinition (ClassDefinition &outDefinition)
{	
    outDefinition.className		= "FileSystemSync";
	outDefinition.initialize	= js_initialize<_Initialize>;
	outDefinition.finalize		= js_finalize<_Finalize>;
	outDefinition.hasInstance	= js_hasInstance<_HasInstance>;
}


void VJSFileSystemSyncClass::_Initialize (const VJSParms_initialize &inParms, VJSFileSystem *inFileSystem)
{
	xbox_assert(inFileSystem != NULL);

	inFileSystem->Retain();
	VString path;
	inFileSystem->GetRoot().GetPosixPath(path);

	VJSObject	rootObject( VJSEntry::CreateRootObject(inParms.GetContext(), true, inFileSystem));
	inParms.GetObject().SetProperty("name", inFileSystem->GetName(), JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
	inParms.GetObject().SetProperty("path", path, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
	inParms.GetObject().SetProperty("root", rootObject, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
}

void VJSFileSystemSyncClass::_Finalize (const VJSParms_finalize &inParms, VJSFileSystem *inFileSystem)
{
	xbox_assert(inFileSystem != NULL);
	inFileSystem->Release();
}

bool VJSFileSystemSyncClass::_HasInstance (const VJSParms_hasInstance &inParms) 
{
	xbox_assert(inParms.GetObject().HasSameRef(inParms.GetObjectToTest()));
 
	return inParms.GetObject().IsOfClass(VJSFileSystemSyncClass::Class());
}


//========================================================================================

VJSEntry::VJSEntry (VJSFileSystem *inFileSystem, const VFilePath &inPath, bool inIsFile, bool inIsSync)
{
	xbox_assert(inFileSystem != NULL);

	fFileSystem = inFileSystem;	// doesn't retain parent. (avoid cyclic dependancy)
	fPath = inPath;
	fIsFile = inIsFile;
	fIsSync = inIsSync;
}


VJSEntry::~VJSEntry ()
{
}


VJSObject VJSEntry::CreateObject( const VJSContext &inContext, bool inIsSync, VJSFileSystem *inFileSystem, const VFilePath &inPath, bool inIsFile)
{
	xbox_assert(inFileSystem != NULL);

	if (!inIsFile && (inFileSystem->GetRoot() == inPath) )
	{
		// Root entry is unique.
		return inFileSystem->GetRootEntryObject( inContext, inIsSync);
	}
	return _CreateObject( inContext, inIsSync, inFileSystem, inPath, inIsFile, false);
}



VJSObject VJSEntry::CreateRootObject( const VJSContext &inContext, bool inIsSync, VJSFileSystem *inFileSystem)
{
	return _CreateObject( inContext, inIsSync, inFileSystem, inFileSystem->GetRoot(), false, true);
}


VJSObject VJSEntry::_CreateObject( const VJSContext &inContext, bool inIsSync, VJSFileSystem *inFileSystem, const VFilePath &inPath, bool inIsFile, bool inIsRoot)
{
	VJSObject	createdObject(inContext);
	VJSEntry		*entry = new VJSEntry(inFileSystem, inPath, inIsFile, inIsSync);

	if (entry == NULL)
	{
		vThrowError(VE_MEMORY_FULL);
	}
	else
	{
		if (inIsSync)
		{
			if (inIsFile) 
				createdObject = VJSFileEntrySyncClass::CreateInstance(inContext, entry);
			else
			{
				createdObject = VJSDirectoryEntrySyncClass::CreateInstance(inContext, entry);
			}
		}
		else
		{
			if (inIsFile) 
				createdObject = VJSFileEntryClass::CreateInstance(inContext, entry);
			else
			{
				createdObject = VJSDirectoryEntryClass::CreateInstance(inContext, entry);
			}
		}

		createdObject.SetProperty("isFile", inIsFile, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
		createdObject.SetProperty("isDirectory", !inIsFile, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);

		VString	string;
		if (!inIsRoot) 
			inPath.GetName(string);
		createdObject.SetProperty("name", string, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);

		VString	rootPath;
		inFileSystem->GetRoot().GetPosixPath(rootPath);
		inPath.GetPosixPath(string);
		string.SubString(rootPath.GetLength(), string.GetLength() - rootPath.GetLength() + 1);		
		createdObject.SetProperty("fullPath", string, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
	}
	
	return createdObject;	
}


void VJSEntry::GetMetadata (const VJSContext &inContext, VJSObject& outResult)
{
	VTime	modificationTime;

	VError error = VE_OK;
	if (!fFileSystem->IsValid())
	{
		error = VJSFileErrorClass::Throw( VJSFileErrorClass::INVALID_STATE_ERR);
	}
	else if (fIsFile)
	{
		VFile	file(fPath);
		if (!file.Exists()) 
			error = VJSFileErrorClass::Throw( VJSFileErrorClass::NOT_FOUND_ERR);
		else if (file.GetTimeAttributes(&modificationTime, NULL, NULL) != VE_OK)
			error = VJSFileErrorClass::Throw( VJSFileErrorClass::SECURITY_ERR);
	}
	else
	{
		VFolder	folder(fPath);
				
		if (!folder.Exists())
			error = VJSFileErrorClass::Throw( VJSFileErrorClass::NOT_FOUND_ERR);
		else if (folder.GetTimeAttributes(&modificationTime, NULL, NULL) != VE_OK) 
			error = VJSFileErrorClass::Throw( VJSFileErrorClass::SECURITY_ERR);
	}

	if (error == VE_OK)
		outResult = VJSMetadataClass::NewInstance(inContext, modificationTime);
}


void VJSEntry::MoveTo (const VJSContext &inContext, VJSEntry *inTargetEntry, const VString &inNewName, VJSObject& outResult)
{
	xbox_assert(inTargetEntry != NULL && !inTargetEntry->fIsFile);
	VFilePath	sourceParent;
	VFolder	targetFolder(inTargetEntry->fPath);
	VFilePath	resultPath;
	VError		error = VE_OK;

	if (!_IsNameCorrect(inNewName))
	{
		error = VJSFileErrorClass::Throw( VJSFileErrorClass::ENCODING_ERR);
	}
	else if (!fFileSystem->IsValid())
	{
		error = VJSFileErrorClass::Throw( VJSFileErrorClass::INVALID_STATE_ERR);
	}
	else if (fFileSystem != inTargetEntry->fFileSystem || _IsRoot() || !fPath.GetParent(sourceParent))
	{
		// Not same file systems for source and target. 
		// Cannot move the root directory. 
		// Or unable to get parent folder (this should never happen).

		error = VJSFileErrorClass::Throw( VJSFileErrorClass::SECURITY_ERR);
	}
	else if (!fIsFile && inTargetEntry->fPath.GetPath().BeginsWith(fPath.GetPath(), true))
	{
		// It is forbidden to move a directory into itself or one of its child.
	
		error = VJSFileErrorClass::Throw( VJSFileErrorClass::INVALID_MODIFICATION_ERR);
	}
	else if (!targetFolder.Exists())
	{
		// Target folder doesn't exists.
		error = VJSFileErrorClass::Throw( VJSFileErrorClass::NOT_FOUND_ERR);
	}
	else
	{
		VString	fullPath( inTargetEntry->fPath.GetPath());
		if (!inNewName.IsEmpty())
		{
			fullPath.AppendString(inNewName);
		}
		else
		{
			VString	string;
			fPath.GetName(string);
			fullPath.AppendString(string);
		}

		VString	filePath, folderPath;

		if (fullPath.GetUniChar(fullPath.GetLength()) == FOLDER_SEPARATOR)
		{
			fullPath.GetSubString(1, fullPath.GetLength() - 1, filePath);
			folderPath = fullPath;
		}
		else
		{
			filePath = fullPath;
			folderPath = fullPath;
			folderPath.AppendUniChar(FOLDER_SEPARATOR);
		}

		VFile file(filePath);
		VFolder	folder(folderPath);
		bool isRename = false;

		if (inTargetEntry->fPath.GetPath().EqualToString(sourceParent.GetPath()))
		{
			isRename = true;
			// Trying to move on itself?
			if (fIsFile)
			{
				if (filePath.EqualToString(fPath.GetPath()))
					error = VJSFileErrorClass::Throw( VJSFileErrorClass::INVALID_MODIFICATION_ERR);
			}
			else
			{
				if (folderPath.EqualToString(fPath.GetPath()))
					error = VJSFileErrorClass::Throw( VJSFileErrorClass::INVALID_MODIFICATION_ERR);
			}
		}
		
		if (error == VE_OK)
		{
			if (file.Exists())
			{
				if (!fIsFile || file.Delete() != VE_OK)
				{
					// Forbidden to replace a file by a folder.
					// Or failed to remove existing file.
					error = VJSFileErrorClass::Throw( VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR);
				}
			}
			else if (folder.Exists())
			{
				if (fIsFile || !folder.IsEmpty() || folder.Delete(false) != VE_OK)
				{
					// Forbidden to replace a folder by a file.
					// If replacing a folder, it must be empty. 
					// Deletion must succeed.
					error = VJSFileErrorClass::Throw( VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR);
				}
			}
		}
			
		// Do actual move.

		if (error == VE_OK)
		{
			if (fIsFile)
			{
				VFile	*resultFile = NULL;
				if (isRename)
				{
					if (VFile(fPath).Rename(inNewName, &resultFile) != VE_OK)
					{
						error = VJSFileErrorClass::Throw( VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR);
					}
				}
				else
				{
					VFilePath	path(filePath);
					if (VFile(fPath).Move(path, &resultFile) != VE_OK)
					{
						error = VJSFileErrorClass::Throw( VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR);
					}
				}
				if (error == VE_OK)
					resultPath = resultFile->GetPath();
				ReleaseRefCountable(&resultFile);
			}
			else
			{
				VFolder	*resultFolder = NULL;
				if (isRename)
				{
					if (VFolder(fPath).Rename(inNewName, &resultFolder) != VE_OK)
					{
						error = VJSFileErrorClass::Throw( VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR);
					}
				}
				else if (inNewName.IsEmpty())
				{
					if (VFolder(fPath).Move(VFolder(inTargetEntry->fPath), &resultFolder) != VE_OK)
					{
						error = VJSFileErrorClass::Throw( VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR);
					}
				}
				else
				{
					// Move with a rename.
					if (VFolder(fPath).Move(VFolder(inTargetEntry->fPath), inNewName, &resultFolder) != VE_OK)
					{
						error = VJSFileErrorClass::Throw( VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR);
					}
				}
				if (error == VE_OK)
					resultPath = resultFolder->GetPath();
				ReleaseRefCountable(&resultFolder);
			}
		}
	}

	if (error == VE_OK)
		outResult = VJSEntry::CreateObject(inContext, fIsSync, fFileSystem, resultPath, fIsFile);
}


void VJSEntry::CopyTo (const VJSContext &inContext, VJSEntry *inTargetEntry, const VString &inNewName, VJSObject& outResult)
{
	xbox_assert(inTargetEntry != NULL && !inTargetEntry->fIsFile);

	VError		error = VE_OK;
	VFilePath	sourceParent;
	VFolder	targetFolder(inTargetEntry->fPath);
	VFilePath	resultPath;

	if (!_IsNameCorrect(inNewName))
	{
		error = VJSFileErrorClass::Throw( VJSFileErrorClass::ENCODING_ERR);
	}
	else if (!fFileSystem->IsValid())
	{
		error = VJSFileErrorClass::Throw( VJSFileErrorClass::INVALID_STATE_ERR);
	}
	else if (fFileSystem != inTargetEntry->fFileSystem || !fPath.GetParent(sourceParent))
	{
		// Not same file systems for source and target. Or unable to get parent folder (this should never happen).
		error = VJSFileErrorClass::Throw( VJSFileErrorClass::SECURITY_ERR);
	}
	else if (!fIsFile && inTargetEntry->fPath.GetPath().BeginsWith(fPath.GetPath(), true))
	{
		// It is forbidden to copy a directory into itself or one of its child.
		error = VJSFileErrorClass::Throw( VJSFileErrorClass::INVALID_MODIFICATION_ERR);
	}
	else if (inTargetEntry->fPath.GetPath().EqualToString(sourceParent.GetPath(), true) && !inNewName.GetLength())
	{
		// A file or directory can be moved to its parent only in case of a rename.
		error = VJSFileErrorClass::Throw( VJSFileErrorClass::INVALID_MODIFICATION_ERR);
	}
	else if (!targetFolder.Exists())
	{
		// Target folder doesn't exists.
		error = VJSFileErrorClass::Throw( VJSFileErrorClass::NOT_FOUND_ERR);
	}
	else
	{
		VString	fullPath( inTargetEntry->fPath.GetPath());
		if (!inNewName.IsEmpty())
		{
			fullPath.AppendString(inNewName);
		}
		else
		{
			VString	string;
			fPath.GetName(string);
			fullPath.AppendString(string);
		}

		VString	filePath, folderPath;

		if (fullPath.GetUniChar(fullPath.GetLength()) == FOLDER_SEPARATOR)
		{
			fullPath.GetSubString(1, fullPath.GetLength() - 1, filePath);
			folderPath = fullPath;
		}
		else
		{
			filePath = fullPath;
			folderPath = fullPath;
			folderPath.AppendUniChar(FOLDER_SEPARATOR);
		}

		VFile		file(filePath);
		VFolder	folder(folderPath);

		if (file.Exists())
		{
			if (!fIsFile || file.Delete() != VE_OK)
			{
				// Forbidden to replace a file by a copy of a folder.
				// Or failed to remove existing file.

				error = VJSFileErrorClass::Throw( VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR);
			} 
		}
		else if (folder.Exists() && fIsFile)
		{
			// Forbidden to replace a folder by a file.

			error = VJSFileErrorClass::Throw( VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR);
		}

		// Do actual copy.

		if (error == VE_OK)
		{
			if (fIsFile)
			{
				VFilePath	path(filePath);
				VFile		*resultFile = NULL;

				if (VFile(fPath).CopyTo(path, &resultFile, FCP_Overwrite) == VE_OK)
				{
					resultPath = resultFile->GetPath();
				}
				else
				{
					error = VJSFileErrorClass::Throw( VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR);
				}
				ReleaseRefCountable(&resultFile);
			}
			else
			{
				VFilePath	path(folderPath); 
			
				if ((folder.Exists() || folder.Create() == VE_OK) && VFolder(fPath).CopyContentsTo(folder, FCP_Overwrite) == VE_OK)
				{
					resultPath = folder.GetPath();
				}
				else
				{
					error = VJSFileErrorClass::Throw( VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR);
				}
			}
		}
	}

	if (error == VE_OK)
		outResult = VJSEntry::CreateObject(inContext, fIsSync, fFileSystem, resultPath, fIsFile);
}


void VJSEntry::Remove (const VJSContext &inContext)
{
	if (!fFileSystem->IsValid())
	{
		VJSFileErrorClass::Throw( VJSFileErrorClass::INVALID_STATE_ERR);
	}
	else if (fIsFile)
	{
		VFile	file(fPath);

		if (!file.Exists())
		{
			VJSFileErrorClass::Throw( VJSFileErrorClass::NOT_FOUND_ERR);
		}
		else if (file.Delete() != VE_OK)
		{
			VJSFileErrorClass::Throw( VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR);
		}
		else
		{
			xbox_assert(!file.Exists());
		}
	}
	else
	{
		VFolder	folder(fPath);

		if (_IsRoot()) 
		{
			VJSFileErrorClass::Throw( VJSFileErrorClass::SECURITY_ERR);	// Forbidden to remove root directory!
		}
		else if (!folder.Exists())
		{
			VJSFileErrorClass::Throw( VJSFileErrorClass::NOT_FOUND_ERR);
		}
		else if (!folder.IsEmpty())
		{
			VJSFileErrorClass::Throw( VJSFileErrorClass::INVALID_MODIFICATION_ERR);
		}
		else if (folder.Delete(false) != VE_OK) 
		{
			VJSFileErrorClass::Throw( VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR);
		}
		else
		{
			xbox_assert(!folder.Exists());
		}
	}
}


void VJSEntry::GetParent (const VJSContext &inContext, VJSObject& outResult)
{
	if (!fFileSystem->IsValid())
	{
		VJSFileErrorClass::Throw( VJSFileErrorClass::INVALID_STATE_ERR);
	}
	else if (_IsRoot())
	{
		// Root entry is unique.
		outResult = fFileSystem->GetRootEntryObject( inContext, fIsSync);
		xbox_assert(outResult.HasRef() && outResult.IsObject());
	}
	else
	{
		VFilePath	parentPath;
		fPath.GetParent(parentPath);
		xbox_assert(fFileSystem->IsAuthorized(parentPath));

		outResult = VJSEntry::CreateObject(inContext, fIsSync, fFileSystem, parentPath, false);
	}
}


void VJSEntry::GetFile (const VJSContext &inContext, const VString &inURL, sLONG inFlags, VJSObject& outResult)
{
	xbox_assert(!IsFile());

	VFilePath	path(GetPath());
	bool		isFile;
	VError error = fFileSystem->ParseURL( inURL, &path, false /* inThrowErrorIfNotFound */, &isFile);
	if (error == VE_OK)
	{
		// Path alredy exists.
		if (inFlags == (eFLAG_CREATE | eFLAG_EXCLUSIVE))
		{
			error = VJSFileErrorClass::Throw( VJSFileErrorClass::PATH_EXISTS_ERR);
		}
		else if (!isFile)
		{
			// Path is actually a directory.
			if (inFlags & eFLAG_CREATE)
			{
				VFolder	folder(path);
				VFile		file(path);

				xbox_assert(folder.Exists());

				// Try to remove the directory and create the file instead.

				if (path == fFileSystem->GetRoot())
				{
					error = VJSFileErrorClass::Throw( VJSFileErrorClass::SECURITY_ERR);
				}
				else if (!folder.IsEmpty() || folder.Delete(false) != VE_OK || file.Create() != VE_OK)
				{
					error = VJSFileErrorClass::Throw( VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR);
				}
				else
				{
					xbox_assert(!folder.Exists() && file.Exists());
				}
			}
			else
			{
				error = VJSFileErrorClass::Throw( VJSFileErrorClass::TYPE_MISMATCH_ERR);
			}
		}
	}
	else if ( (error == VE_FS_NOT_FOUND) && (inFlags & eFLAG_CREATE) )
	{
		// File doesn't exists and eFLAG_CREATE is set, create it.
		// It is forbidden to create a file with no existing parent folder.
		VFilePath	parentPath;
		if (path.GetParent(parentPath) && VFolder(parentPath).Exists())
		{
			VFile	file(path);
			error = file.Create();
			if (error != VE_OK)
			{
				error = VJSFileErrorClass::Throw( VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR);
			}
			else
			{
				xbox_assert(file.Exists());
			}
		}
		else
		{
			error = VJSFileErrorClass::Throw( VJSFileErrorClass::NOT_FOUND_ERR);
		}
	}
	else
	{
		error = VJSFileErrorClass::Throw( VJSFileErrorClass::NOT_FOUND_ERR);
	}

	if (error == VE_OK) 
		outResult = VJSEntry::CreateObject(inContext, fIsSync, fFileSystem, path, true);
}


void VJSEntry::GetDirectory (const VJSContext &inContext, const VString &inURL, sLONG inFlags, VJSObject& outResult)
{
	xbox_assert(!IsFile());

	VFilePath	path(GetPath());
	bool		isFile;

	VError error = fFileSystem->ParseURL(inURL, &path, false /* inThrowErrorIfNotFound */, &isFile);
	if (error == VE_OK)
	{
		// Path already exists.
		if (inFlags == (eFLAG_CREATE | eFLAG_EXCLUSIVE))
		{
			error = VJSFileErrorClass::Throw( VJSFileErrorClass::PATH_EXISTS_ERR);
		}
		else if (isFile)
		{
			// Path is actually a file.
			if (inFlags & eFLAG_CREATE)
			{
				VFile		file(path);
				VString	fullPath;

				path.GetPath(fullPath);
				fullPath.AppendUniChar(FOLDER_SEPARATOR);

				VFolder	folder(fullPath);
				
				xbox_assert(file.Exists());

				// Try to remove the file and create the directory instead.
				error = file.Delete();
				if (error == VE_OK)
					error = folder.Create();
		
				if (error != VE_OK)
				{
					error = VJSFileErrorClass::Throw( VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR);
				}
				else
				{
					xbox_assert(!file.Exists() && folder.Exists());
				}
			}
			else
			{
				error = VJSFileErrorClass::Throw( VJSFileErrorClass::TYPE_MISMATCH_ERR);
			}
		}
	}
	else if ( (error == VE_FS_NOT_FOUND) && (inFlags & eFLAG_CREATE) )
	{
		// Folder doesn't exists and eFLAG_CREATE is set, create it.
		// Same as file, it is forbidden to create a folder with no existing parent folder.
		VFilePath	parentPath;
		if (path.GetParent(parentPath) && VFolder(parentPath).Exists())
		{
			VString	fullPath;
			path.GetPath(fullPath);
			if (fullPath.GetUniChar(fullPath.GetLength()) != FOLDER_SEPARATOR)
			{
				fullPath.AppendUniChar(FOLDER_SEPARATOR);
				path.FromFullPath(fullPath);
			}

			VFolder	folder(fullPath);
			error = folder.Create();
			if (error != VE_OK)
			{
				error = VJSFileErrorClass::Throw( VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR);
			}
			else
			{
				xbox_assert(folder.Exists());
			}
		}
		else
		{
			error = VJSFileErrorClass::Throw( VJSFileErrorClass::NOT_FOUND_ERR);
		}
	}
	else
	{
		error = VJSFileErrorClass::Throw( VJSFileErrorClass::NOT_FOUND_ERR);
	}

	if (error == VE_OK) 
		outResult = VJSEntry::CreateObject(inContext, fIsSync, fFileSystem, path, false);
}


void VJSEntry::RemoveRecursively (const VJSContext &inContext)
{
	xbox_assert(!fIsFile);

	if (!fFileSystem->IsValid())
	{
		VJSFileErrorClass::Throw( VJSFileErrorClass::INVALID_STATE_ERR);
	}
	else
	{
		VFolder	folder(fPath);

		if (_IsRoot()) 
		{
			VJSFileErrorClass::Throw( VJSFileErrorClass::SECURITY_ERR);	// Forbidden to remove root directory!
		}
		else if (!folder.Exists())
		{
			VJSFileErrorClass::Throw( VJSFileErrorClass::NOT_FOUND_ERR);
		}
		else if (folder.Delete(true) != VE_OK) 
		{
			VJSFileErrorClass::Throw( VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR);
		}
		else
		{
			xbox_assert(!folder.Exists());
		}
	}
}

void VJSEntry::Folder (const VJSContext &inContext, VJSObject& outResult)
{
	xbox_assert(!fIsFile);

	if (!fFileSystem->IsValid())
	{
		VJSFileErrorClass::Throw( VJSFileErrorClass::INVALID_STATE_ERR);
	}
	else
	{
		VFolder *folder = new VFolder(fPath, fFileSystem->GetFileSystem());
		if (folder != NULL) 
		{
			if (!folder->Exists())
			{
				VJSFileErrorClass::Throw( VJSFileErrorClass::NOT_FOUND_ERR);
			}
			else
			{
				JS4DFolderIterator	*folderIterator = new JS4DFolderIterator(folder);
				if (folderIterator != NULL)
				{
					outResult = VJSFolderIterator::CreateInstance(inContext, folderIterator);
				}
				ReleaseRefCountable(&folderIterator);
			}
		}
		ReleaseRefCountable(&folder);
	}
}

void VJSEntry::CreateWriter (const VJSContext &inContext, VJSObject& outResult)
{
	xbox_assert(fIsFile);

	//** TODO.
}

void VJSEntry::File (const VJSContext &inContext, VJSObject& outResult)
{
	xbox_assert(fIsFile);

	if (!fFileSystem->IsValid())
	{
		VJSFileErrorClass::Throw( VJSFileErrorClass::INVALID_STATE_ERR);
	}
	else
	{
		VFile	*file = new VFile(fPath, fFileSystem->GetFileSystem());
		if (file != NULL) 
		{
			if (!file->Exists())
			{
				VJSFileErrorClass::Throw( VJSFileErrorClass::NOT_FOUND_ERR);
			}
			else
			{
				JS4DFileIterator	*fileIterator = new JS4DFileIterator(file);
				if (fileIterator != NULL)
				{
					outResult = VJSFileIterator::CreateInstance(inContext, fileIterator);
				}
				ReleaseRefCountable(&fileIterator);
			}
		}
		ReleaseRefCountable(&file);
	}
}


void VJSEntry::_getMetadata (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry)
{
	xbox_assert(inEntry != NULL);

	if (inEntry->fIsSync)
	{
		VJSObject	resultObject(ioParms.GetContext());
		inEntry->GetMetadata(ioParms.GetContext(), resultObject);
		ioParms.ReturnValue(resultObject);
	}
	else
	{
		VJSObject	successCallback(ioParms.GetContext());
		VJSObject	errorCallback(ioParms.GetContext());

		if (!ioParms.IsObjectParam(1) || !ioParms.GetParamFunc(1, successCallback))
		{
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "1");
		}
		else if (ioParms.CountParams() >= 2 && (!ioParms.IsObjectParam(2) || !ioParms.GetParamFunc(2, errorCallback)))
		{
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "2");
		}
		else
		{
			VJSW3CFSEvent	*request = VJSW3CFSEvent::GetMetadata(inEntry, successCallback, errorCallback);
			if (request != NULL)
			{
				VJSWorker	*worker = VJSWorker::RetainWorker(ioParms.GetContext());
				worker->QueueEvent(request);
				ReleaseRefCountable(&worker);
			}
		}
	}
}


void VJSEntry::_moveTo (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry)
{
	_moveOrCopyTo(ioParms, inEntry, true);
}

void VJSEntry::_copyTo (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry)
{
	_moveOrCopyTo(ioParms, inEntry, false);
}

void VJSEntry::_toURL (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry)
{
// chrome returns something like
//	"filesystem:http://www.w3schools.com/temporary/"

	xbox_assert(inEntry != NULL);

	VString	mimeType;

	if (ioParms.CountParams() && !ioParms.GetStringParam(1, mimeType)) {

		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
		return;

	}
 
	VIndex	rootLength, relativeSize;
	
	rootLength = inEntry->fFileSystem->GetRoot().GetPath().GetLength();

	xbox_assert(inEntry->fPath.GetPath().GetLength() >= rootLength);
	relativeSize = inEntry->fPath.GetPath().GetLength() - rootLength + 1;

	VString	relativePath;

	if (relativeSize) 

		inEntry->fPath.GetPath().GetSubString(rootLength, relativeSize, relativePath);
	
	else

		relativePath = ".";

#if VERSIONWIN == 1 || VERSION_MAC == 1

	UniChar	*p;

	for (p = (UniChar *) relativePath.GetCPointer(); *p != 0; p++)

		if (*p == FOLDER_SEPARATOR)

			*p = '/';

#endif

	//** TODO: Add mimeType ! (What is the syntax?)

	VString	url;

	url = "file://";
	url.AppendString(relativePath);

	ioParms.ReturnString(url);
}

void VJSEntry::_remove (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry)
{
	xbox_assert(inEntry != NULL);

	if (inEntry->fIsSync)
	{
		inEntry->Remove(ioParms.GetContext());
	}
	else
	{
		VJSObject	successCallback(ioParms.GetContext());
		VJSObject	errorCallback(ioParms.GetContext());

		if (!ioParms.IsObjectParam(1) || !ioParms.GetParamFunc(1, successCallback))
		{
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "1");
		}
		else if (ioParms.CountParams() >= 2 && (!ioParms.IsObjectParam(2) || !ioParms.GetParamFunc(2, errorCallback)))
		{
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "2");
		}
		else
		{
			VJSW3CFSEvent	*request = VJSW3CFSEvent::Remove(inEntry, successCallback, errorCallback);
			if (request != NULL)
			{
				VJSWorker	*worker = VJSWorker::RetainWorker(ioParms.GetContext());
				worker->QueueEvent(request);
				ReleaseRefCountable<VJSWorker>(&worker);
			}
		}
	}
}


void VJSEntry::_filesystem( VJSParms_getProperty& ioParms, VJSEntry *inEntry)
{
	ioParms.ReturnValue( VJSLocalFileSystem::GetInstance( ioParms.GetContext(), inEntry->fFileSystem, inEntry->fIsSync));
}


void VJSEntry::_getParent (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry)
{
	xbox_assert(inEntry != NULL);

	if (inEntry->fIsSync)
	{
		VJSObject	resultObject(ioParms.GetContext());
		inEntry->GetParent(ioParms.GetContext(), resultObject);
		ioParms.ReturnValue(resultObject);
	}
	else
	{
		VJSObject	successCallback(ioParms.GetContext());
		VJSObject	errorCallback(ioParms.GetContext());

		if (!ioParms.IsObjectParam(1) || !ioParms.GetParamFunc(1, successCallback))
		{
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "1");
		}
		else if (ioParms.CountParams() >= 2 && (!ioParms.IsObjectParam(2) || !ioParms.GetParamFunc(2, errorCallback)))
		{
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "2");
		}
		else
		{
			VJSW3CFSEvent	*request = VJSW3CFSEvent::GetParent(inEntry, successCallback, errorCallback);
			if (request != NULL)
			{
				VJSWorker	*worker = VJSWorker::RetainWorker(ioParms.GetContext());
				worker->QueueEvent(request);
				ReleaseRefCountable(&worker);
			}
		}
	}
}

void VJSEntry::_createReader (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry)
{
	xbox_assert(inEntry != NULL && !inEntry->fIsFile);

	ioParms.ReturnValue(VJSDirectoryReader::NewReaderObject(ioParms.GetContext(), inEntry));
}

void VJSEntry::_getFile (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry)
{
	_getFileOrDirectory(ioParms, inEntry, true);
}

void VJSEntry::_getDirectory (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry)
{
	_getFileOrDirectory(ioParms, inEntry, false);	
}

void VJSEntry::_removeRecursively (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry)
{
	xbox_assert(inEntry != NULL && !inEntry->fIsFile);

	if (inEntry->fIsSync)
	{
		inEntry->RemoveRecursively(ioParms.GetContext());
	}
	else
	{
		VJSObject	successCallback(ioParms.GetContext());
		VJSObject	errorCallback(ioParms.GetContext());

		if (!ioParms.IsObjectParam(1) || !ioParms.GetParamFunc(1, successCallback))
		{
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "1");
		}
		else if (ioParms.CountParams() >= 2 && (!ioParms.IsObjectParam(2) || !ioParms.GetParamFunc(2, errorCallback)))
		{
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "2");
		}
		else
		{
			VJSW3CFSEvent	*request = VJSW3CFSEvent::RemoveRecursively(inEntry, successCallback, errorCallback);
			if (request != NULL)
			{
				VJSWorker	*worker = VJSWorker::RetainWorker(ioParms.GetContext());
				worker->QueueEvent(request);
				ReleaseRefCountable(&worker);
			}
		}
	}
}

void VJSEntry::_folder (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry)
{
	xbox_assert(inEntry != NULL && !inEntry->fIsFile);

	if (inEntry->fIsSync)
	{
		VJSObject	resultObject(ioParms.GetContext());

		inEntry->Folder(ioParms.GetContext(), resultObject);
		ioParms.ReturnValue(resultObject);
	}
	else
	{
		VJSObject	successCallback(ioParms.GetContext());
		VJSObject	errorCallback(ioParms.GetContext());

		if (!ioParms.IsObjectParam(1) || !ioParms.GetParamFunc(1, successCallback))
		{
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "1");
		}
		else if (ioParms.CountParams() >= 2 && (!ioParms.IsObjectParam(2) || !ioParms.GetParamFunc(2, errorCallback)))
		{
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "2");
		}
		else
		{
			VJSW3CFSEvent	*request = VJSW3CFSEvent::Folder(inEntry, successCallback, errorCallback);
			if (request != NULL)
			{
				VJSWorker	*worker = VJSWorker::RetainWorker(ioParms.GetContext());
				worker->QueueEvent(request);
				ReleaseRefCountable(&worker);
			}
		}
	}
}

void VJSEntry::_createWriter (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry)
{
	xbox_assert(inEntry != NULL && inEntry->fIsFile);

	//** TODO
}

void VJSEntry::_file (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry)
{
	xbox_assert(inEntry != NULL && inEntry->fIsFile);

	if (inEntry->fIsSync)
	{
		VJSObject	resultObject(ioParms.GetContext());
		inEntry->File(ioParms.GetContext(), resultObject);
		ioParms.ReturnValue(resultObject);
	}
	else
	{
		VJSObject	successCallback(ioParms.GetContext());
		VJSObject	errorCallback(ioParms.GetContext());

		if (!ioParms.IsObjectParam(1) || !ioParms.GetParamFunc(1, successCallback))
		{
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "1");
		}
		else if (ioParms.CountParams() >= 2 && (!ioParms.IsObjectParam(2) || !ioParms.GetParamFunc(2, errorCallback)))
		{
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "2");
		}
		else
		{
			VJSW3CFSEvent	*request = VJSW3CFSEvent::File(inEntry, successCallback, errorCallback);
			if (request != NULL)
			{
				VJSWorker	*worker = VJSWorker::RetainWorker(ioParms.GetContext());
				worker->QueueEvent(request);
				ReleaseRefCountable(&worker);
			}
		}
	}
}

void VJSEntry::_moveOrCopyTo (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry, bool inIsMoveTo)
{
	xbox_assert(inEntry != NULL);

	VJSObject			parentEntry(ioParms.GetContext());
	VJSEntry				*targetEntry;
	VString			newName;

	VError			error;
	JS4D::ClassRef	classRef;

	if (inEntry->fIsSync) {

		error = VE_JVSC_FS_EXPECTING_DIR_ENTRY_SYNC;
		classRef = VJSDirectoryEntryClass::Class();
		
	} else {
		
		error = VE_JVSC_FS_EXPECTING_DIR_ENTRY;
		classRef = VJSDirectoryEntrySyncClass::Class();

	}
	
	if (!ioParms.GetParamObject(1, parentEntry) && !parentEntry.IsOfClass(classRef)) {

		vThrowError(error, "1");
		return;

	}

	if (inEntry->fIsSync) 

		targetEntry = parentEntry.GetPrivateData<VJSDirectoryEntrySyncClass>();

	else

		targetEntry = parentEntry.GetPrivateData<VJSDirectoryEntryClass>();

	xbox_assert(targetEntry != NULL);

	// Get optional new name. An empty new name means none was specified.

	sLONG	index;

	if (ioParms.CountParams() >= 2 && ioParms.IsStringParam(2)) {
		
		if (!ioParms.GetStringParam(2, newName)) {

			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2");
			return;

		} else if (!newName.GetLength()) {

			vThrowError(VE_JVSC_FS_NEW_NAME_IS_EMTPY, "2");
			return;
		
		} else

			index = 3;

	} else 

		index = 2;

	if (inEntry->fIsSync) {

		VJSObject	result(ioParms.GetContext());

		if (inIsMoveTo) 
			inEntry->MoveTo(ioParms.GetContext(), targetEntry, newName, result);
		else
			inEntry->CopyTo(ioParms.GetContext(), targetEntry, newName, result);

		ioParms.ReturnValue(result);

	} else {

		VJSObject	successCallback(ioParms.GetContext());
		VJSObject	errorCallback(ioParms.GetContext());
		VString	argumentNumber;

		if (ioParms.CountParams() >= index
		&& (!ioParms.IsObjectParam(index) || !ioParms.GetParamFunc(index, successCallback))) {

			argumentNumber.FromLong(index);
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, argumentNumber);
			return;

		}

		index++;
		if (ioParms.CountParams() >= index
		&& (!ioParms.IsObjectParam(index) || !ioParms.GetParamFunc(index, errorCallback))) {

			argumentNumber.FromLong(index);
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, argumentNumber);
			return;

		}

		VJSW3CFSEvent	*request;

		if (inIsMoveTo) 

			request = VJSW3CFSEvent::MoveTo(inEntry, targetEntry, newName, successCallback, errorCallback);
		else
			request = VJSW3CFSEvent::CopyTo(inEntry, targetEntry, newName, successCallback, errorCallback);

		if (request != NULL)
		{
			VJSWorker	*worker = VJSWorker::RetainWorker(ioParms.GetContext());
			worker->QueueEvent(request);
			ReleaseRefCountable(&worker);
		}
	}
}

void VJSEntry::_getFileOrDirectory (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry, bool inIsGetFile)
{
	xbox_assert(inEntry != NULL && !inEntry->fIsFile);

	VError error = VE_OK;
	VString	url;
	sLONG			flags = 0;
	sLONG	index;

	if (!ioParms.GetStringParam(1, url))
	{
		error = vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
	}
	else
	{
		if (ioParms.CountParams() >= 2)
		{
			VJSObject	options(ioParms.GetContext());
			if (!ioParms.GetParamObject(2, options))
			{
				error = vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_OBJECT, "2");
			}
			else if (!options.IsFunction())
			{
				if (options.GetPropertyAsBool("create", NULL, NULL))
					flags |= eFLAG_CREATE;
				if (options.GetPropertyAsBool("exclusive", NULL, NULL))
					flags |= eFLAG_EXCLUSIVE;
				index = 3;
			} else 
				index = 2;
		} else
			index = 2;
	}

	if (error == VE_OK)
	{
		if (inEntry->fIsSync)
		{
			VJSObject	resultObject(ioParms.GetContext());

			if (inIsGetFile)
				inEntry->GetFile(ioParms.GetContext(), url, flags, resultObject);
			else
				inEntry->GetDirectory(ioParms.GetContext(), url, flags, resultObject);
		
			ioParms.ReturnValue(resultObject);
		}
		else
		{
			VJSObject	successCallback(ioParms.GetContext());
			VJSObject	errorCallback(ioParms.GetContext());
			VString	argumentNumber;

			if (ioParms.CountParams() >= index && (!ioParms.IsObjectParam(index) || !ioParms.GetParamFunc(index, successCallback)))
			{
				argumentNumber.FromLong(index);
				error = vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, argumentNumber);
			}
			else
			{
				index++;
				if (ioParms.CountParams() >= index && (!ioParms.IsObjectParam(index) || !ioParms.GetParamFunc(index, errorCallback)))
				{
					argumentNumber.FromLong(index);
					error = vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, argumentNumber);
				}
			}

			if (error == VE_OK)
			{
				VJSW3CFSEvent	*request;

				if (inIsGetFile)
					request = VJSW3CFSEvent::GetFile(inEntry, url, flags, successCallback, errorCallback);
				else
					request = VJSW3CFSEvent::GetDirectory(inEntry, url, flags, successCallback, errorCallback);
					
				if (request != NULL)
				{
					VJSWorker	*worker = VJSWorker::RetainWorker(ioParms.GetContext());
					worker->QueueEvent(request);
					ReleaseRefCountable( &worker);
				}
			}
		}
	}
}


// Rudimentary encoding checking. Just make sure there is no directory separator if it is a file.
// Or look that directory separator is at the end for a folder. Further errors are catched when trying
// to execute the operation.

bool VJSEntry::_IsNameCorrect (const VString &inName)
{
	VIndex	index	= inName.FindUniChar('/');

	if (!index)

		return true;

	else if (fIsFile)

		return false;

	else

		return index == inName.GetLength() && index != 1;
}


//========================================================================================


void VJSDirectoryEntryClass::GetDefinition (ClassDefinition &outDefinition)
{
	static inherited::StaticFunction functions[] =
	{
		{	"createReader",			js_callStaticFunction<VJSEntry::_createReader>,			JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	"getFile",				js_callStaticFunction<VJSEntry::_getFile>,				JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	"getDirectory",			js_callStaticFunction<VJSEntry::_getDirectory>,			JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	"removeRecursively",	js_callStaticFunction<VJSEntry::_removeRecursively>,	JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	"folder",				js_callStaticFunction<VJSEntry::_folder>,				JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},	// non standard

		{	"getMetadata",			js_callStaticFunction<VJSEntry::_getMetadata>,			JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	"moveTo",				js_callStaticFunction<VJSEntry::_moveTo>,				JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	"copyTo",				js_callStaticFunction<VJSEntry::_copyTo>,				JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	"toURL",				js_callStaticFunction<VJSEntry::_toURL>,				JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	"remove",				js_callStaticFunction<VJSEntry::_remove>,				JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	"getParent",			js_callStaticFunction<VJSEntry::_getParent>,			JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},

		{	0,						0,														0																			},
	};

    static inherited::StaticValue values[] =
        {
            { "filesystem",    js_getProperty<VJSEntry::_filesystem>,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { 0, 0, 0, 0}
        };

    outDefinition.className			= "DirectoryEntry";
	outDefinition.staticFunctions	= functions;	
    outDefinition.staticValues      = values;
	outDefinition.initialize		= js_initialize<_Initialize>;
	outDefinition.finalize			= js_finalize<_Finalize>;
	outDefinition.hasInstance		= js_hasInstance<_HasInstance>;
}

void VJSDirectoryEntryClass::_Initialize (const VJSParms_initialize &inParms, VJSEntry *inEntry)
{
	xbox_assert(inEntry != NULL);

	inEntry->Retain();
}

void VJSDirectoryEntryClass::_Finalize (const VJSParms_finalize &inParms, VJSEntry *inEntry)
{
	xbox_assert(inEntry != NULL);

	inEntry->Release();
}

bool VJSDirectoryEntryClass::_HasInstance (const VJSParms_hasInstance &inParms) 
{
	xbox_assert(inParms.GetObject().HasSameRef(inParms.GetObjectToTest()));
 
	return inParms.GetObject().IsOfClass(VJSDirectoryEntryClass::Class());
}

void VJSDirectoryEntrySyncClass::GetDefinition (ClassDefinition &outDefinition)
{
	static VJSClass<VJSDirectoryEntrySyncClass, VJSEntry>::StaticFunction functions[] =
	{
		{	"createReader",			js_callStaticFunction<VJSEntry::_createReader>,			JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	"getFile",				js_callStaticFunction<VJSEntry::_getFile>,				JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	"getDirectory",			js_callStaticFunction<VJSEntry::_getDirectory>,			JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	"removeRecursively",	js_callStaticFunction<VJSEntry::_removeRecursively>,	JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	"folder",				js_callStaticFunction<VJSEntry::_folder>,				JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},

		{	"getMetadata",			js_callStaticFunction<VJSEntry::_getMetadata>,			JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	"moveTo",				js_callStaticFunction<VJSEntry::_moveTo>,				JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	"copyTo",				js_callStaticFunction<VJSEntry::_copyTo>,				JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	"toURL",				js_callStaticFunction<VJSEntry::_toURL>,				JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	"remove",				js_callStaticFunction<VJSEntry::_remove>,				JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	"getParent",			js_callStaticFunction<VJSEntry::_getParent>,			JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},

		{	0,						0,														0																			},
	};

    static inherited::StaticValue values[] =
        {
            { "filesystem",    js_getProperty<VJSEntry::_filesystem>,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { 0, 0, 0, 0}
        };

    outDefinition.className			= "DirectoryEntrySync";
	outDefinition.staticFunctions	= functions;	
    outDefinition.staticValues      = values;
	outDefinition.initialize		= js_initialize<VJSDirectoryEntryClass::_Initialize>;
	outDefinition.finalize			= js_finalize<VJSDirectoryEntryClass::_Finalize>;
	outDefinition.hasInstance		= js_hasInstance<_HasInstance>;
}

bool VJSDirectoryEntrySyncClass::_HasInstance (const VJSParms_hasInstance &inParms) 
{
	xbox_assert(inParms.GetObject().HasSameRef(inParms.GetObjectToTest()));
 
	return inParms.GetObject().IsOfClass(VJSDirectoryEntrySyncClass::Class());
}

VJSObject VJSDirectoryReader::NewReaderObject (const VJSContext &inContext, VJSEntry *inEntry)
{
	xbox_assert(inEntry != NULL && !inEntry->IsFile());

	VJSObject		createdObject(inContext);
	VJSDirectoryReader	*reader = new VJSDirectoryReader(inEntry->GetFileSystem(), inEntry->GetPath(), inEntry->IsSync());

	if (reader == NULL || reader->fFolder == NULL || reader->fFolderIterator == NULL || reader->fFileIterator == NULL) 

		vThrowError(VE_MEMORY_FULL);

	else {
		
		reader->fFolderIterator->First();
		reader->fFileIterator->First();
		if (inEntry->IsSync())

			createdObject = VJSDirectoryReaderSyncClass::CreateInstance(inContext, reader);

		else 

			createdObject = VJSDirectoryReaderClass::CreateInstance(inContext, reader);

	}
		
	ReleaseRefCountable<VJSDirectoryReader>(&reader);
	return createdObject;
}

bool VJSDirectoryReader::ReadEntries (const VJSContext &inContext, VJSObject& outResult)
{
	bool again = false;
	if (!fFileSystem->IsValid())
	{
		VJSFileErrorClass::Throw( VJSFileErrorClass::INVALID_STATE_ERR);
	}
	else if (!fFolder->Exists())
	{
		VJSFileErrorClass::Throw( VJSFileErrorClass::NOT_FOUND_ERR);
	}
	else
	{
		// Currently folders are always returned first, followed by files. Yet there is no defined order in the spec, 
		// and this (order) can change in the future.

		sLONG			numberEntries = 0;
		VJSArray	entriesArray(inContext);
		
		for ( ; fFolderIterator->IsValid() && numberEntries < kMaximumEntries; ++*fFolderIterator)
		{
			VFolder	*f = fFolderIterator->Current();
			entriesArray.PushValue(VJSEntry::CreateObject(inContext, fIsSync, fFileSystem, fFolderIterator->Current()->GetPath(), false));
			numberEntries++;
		}

		for ( ; fFileIterator->IsValid() && numberEntries < kMaximumEntries; ++*fFileIterator)
		{
			entriesArray.PushValue(VJSEntry::CreateObject(inContext, fIsSync, fFileSystem, fFileIterator->Current()->GetPath(), true));
			numberEntries++;
		}

		outResult = entriesArray;

		again = fFolderIterator->IsValid() || fFileIterator->IsValid();
	}

	return again;
}

VJSDirectoryReader::VJSDirectoryReader (VJSFileSystem *inFileSystem, const VFilePath &inPath, bool inIsSync)
{
	xbox_assert(inFileSystem != NULL);

	fIsSync = inIsSync;
	fFileSystem = RetainRefCountable<VJSFileSystem>(inFileSystem);
	if ((fFolder = new VFolder(inPath)) != NULL) {

		fFolderIterator = new VFolderIterator(fFolder, FI_ITERATE_DELETE | FI_NORMAL_FOLDERS);
		fFileIterator = new VFileIterator(fFolder, FI_ITERATE_DELETE | FI_NORMAL_FILES);

	}
	fIsReading = false;
}

VJSDirectoryReader::~VJSDirectoryReader()
{
	ReleaseRefCountable(&fFolder);

	delete fFolderIterator;
	fFolderIterator = NULL;

	delete fFileIterator;
	fFileIterator = NULL;

	xbox_assert(fFileSystem != NULL);
	ReleaseRefCountable(&fFileSystem);	
}

void VJSDirectoryReaderClass::GetDefinition (ClassDefinition &outDefinition)
{
	static VJSClass<VJSDirectoryReaderClass, VJSDirectoryReader>::StaticFunction functions[] =
	{
		{	"readEntries",	js_callStaticFunction<_readEntries>,	JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	0,				0,										0																			},
	};

    outDefinition.className			= "DirectoryReader";
	outDefinition.staticFunctions	= functions;	
	outDefinition.initialize		= js_initialize<_Initialize>;
	outDefinition.finalize			= js_finalize<_Finalize>;
	outDefinition.hasInstance		= js_hasInstance<_HasInstance>;
}

void VJSDirectoryReaderClass::_Initialize (const VJSParms_initialize &inParms, VJSDirectoryReader *inDirectoryReader)
{
	xbox_assert(inDirectoryReader != NULL);

	inDirectoryReader->Retain();
}

void VJSDirectoryReaderClass::_Finalize (const VJSParms_finalize &inParms, VJSDirectoryReader *inDirectoryReader)
{
	xbox_assert(inDirectoryReader != NULL);

	inDirectoryReader->Release();
}

bool VJSDirectoryReaderClass::_HasInstance (const VJSParms_hasInstance &inParms) 
{
	xbox_assert(inParms.GetObject().HasSameRef(inParms.GetObjectToTest()));
 
	return inParms.GetObject().IsOfClass(VJSDirectoryReaderClass::Class());
}

void VJSDirectoryReaderClass::_readEntries (VJSParms_callStaticFunction &ioParms, VJSDirectoryReader *inDirectoryReader)
{
	xbox_assert(inDirectoryReader != NULL);

	if (inDirectoryReader->IsReading())
	{
		VJSFileErrorClass::Throw( VJSFileErrorClass::INVALID_STATE_ERR);
	}
	else
	{
		VJSObject	successCallback(ioParms.GetContext());
		VJSObject	errorCallback(ioParms.GetContext());

		if (!ioParms.IsObjectParam(1) || !ioParms.GetParamFunc(1, successCallback))
		{
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "1");
		}
		else if (ioParms.CountParams() >= 2 && (!ioParms.IsObjectParam(2) || !ioParms.GetParamFunc(2, errorCallback)))
		{
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "2");
		}
		else
		{
			VJSW3CFSEvent	*request = VJSW3CFSEvent::ReadEntries(inDirectoryReader, successCallback, errorCallback);
			if (request != NULL)
			{
				VJSWorker	*worker = VJSWorker::RetainWorker(ioParms.GetContext());
				inDirectoryReader->SetAsReading();
				worker->QueueEvent(request);
				ReleaseRefCountable(&worker);
			}
		}
	}
}

void VJSDirectoryReaderSyncClass::GetDefinition (ClassDefinition &outDefinition)
{
	static VJSClass<VJSDirectoryReaderSyncClass, VJSDirectoryReader>::StaticFunction functions[] =
	{
		{	"readEntries",	js_callStaticFunction<_readEntries>,	JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	0,				0,										0																			},
	};

    outDefinition.className			= "DirectoryReaderSync";
	outDefinition.staticFunctions	= functions;	
	outDefinition.initialize		= js_initialize<VJSDirectoryReaderClass::_Initialize>;
	outDefinition.finalize			= js_finalize<VJSDirectoryReaderClass::_Finalize>;
	outDefinition.hasInstance		= js_hasInstance<_HasInstance>;
}

bool VJSDirectoryReaderSyncClass::_HasInstance (const VJSParms_hasInstance &inParms) 
{
	xbox_assert(inParms.GetObject().HasSameRef(inParms.GetObjectToTest()));
 
	return inParms.GetObject().IsOfClass(VJSDirectoryReaderSyncClass::Class());
}

void VJSDirectoryReaderSyncClass::_readEntries (VJSParms_callStaticFunction &ioParms, VJSDirectoryReader *inDirectoryReader)
{
	xbox_assert(inDirectoryReader != NULL && inDirectoryReader->IsSync());

	VJSObject	resultObject(ioParms.GetContext());
	inDirectoryReader->ReadEntries(ioParms.GetContext(), resultObject);
	ioParms.ReturnValue(resultObject);
}

void VJSFileEntryClass::GetDefinition (ClassDefinition &outDefinition)
{
	static inherited::StaticFunction functions[] =
	{
		{	"createWriter",	js_callStaticFunction<VJSEntry::_createWriter>,	JS4D::PropertyAttributeDontDelete											},
		{	"file",			js_callStaticFunction<VJSEntry::_file>,			JS4D::PropertyAttributeDontDelete											},

		{	"getMetadata",	js_callStaticFunction<VJSEntry::_getMetadata>,	JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	"moveTo",		js_callStaticFunction<VJSEntry::_moveTo>,		JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	"copyTo",		js_callStaticFunction<VJSEntry::_copyTo>,		JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	"toURL",		js_callStaticFunction<VJSEntry::_toURL>,		JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	"remove",		js_callStaticFunction<VJSEntry::_remove>,		JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	"getParent",	js_callStaticFunction<VJSEntry::_getParent>,	JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},

		{	0,				0,												0																			},
	};

    static inherited::StaticValue values[] =
        {
            { "filesystem",    js_getProperty<VJSEntry::_filesystem>,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { 0, 0, 0, 0}
        };

    outDefinition.className			= "FileEntry";
	outDefinition.staticFunctions	= functions;
    outDefinition.staticValues      = values;
	outDefinition.initialize		= js_initialize<_Initialize>;
	outDefinition.finalize			= js_finalize<_Finalize>;
	outDefinition.hasInstance		= js_hasInstance<_HasInstance>;
}

void VJSFileEntryClass::_Initialize (const VJSParms_initialize &inParms, VJSEntry *inEntry)
{
	xbox_assert(inEntry != NULL);

	inEntry->Retain();
}

void VJSFileEntryClass::_Finalize (const VJSParms_finalize &inParms, VJSEntry *inEntry)
{
	xbox_assert(inEntry != NULL);

	inEntry->Release();
}

bool VJSFileEntryClass::_HasInstance (const VJSParms_hasInstance &inParms) 
{
	xbox_assert(inParms.GetObject().HasSameRef(inParms.GetObjectToTest()));
 
	return inParms.GetObject().IsOfClass(VJSFileEntryClass::Class());
}

void VJSFileEntrySyncClass::GetDefinition (ClassDefinition &outDefinition)
{
	static inherited::StaticFunction functions[] =
	{
		{	"createWriter",	js_callStaticFunction<VJSEntry::_createWriter>,	JS4D::PropertyAttributeDontDelete											},
		{	"file",			js_callStaticFunction<VJSEntry::_file>,			JS4D::PropertyAttributeDontDelete											},

		{	"getMetadata",	js_callStaticFunction<VJSEntry::_getMetadata>,	JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	"moveTo",		js_callStaticFunction<VJSEntry::_moveTo>,		JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	"copyTo",		js_callStaticFunction<VJSEntry::_copyTo>,		JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	"toURL",		js_callStaticFunction<VJSEntry::_toURL>,		JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	"remove",		js_callStaticFunction<VJSEntry::_remove>,		JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	"getParent",	js_callStaticFunction<VJSEntry::_getParent>,	JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},

		{	0,				0,												0																			},
	};

    static inherited::StaticValue values[] =
        {
            { "filesystem",    js_getProperty<VJSEntry::_filesystem>,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { 0, 0, 0, 0}
        };

    outDefinition.className			= "FileEntrySync";
	outDefinition.staticFunctions	= functions;	
    outDefinition.staticValues      = values;
	outDefinition.initialize		= js_initialize<VJSFileEntryClass::_Initialize>;
	outDefinition.finalize			= js_finalize<VJSFileEntryClass::_Finalize>;
	outDefinition.hasInstance		= js_hasInstance<_HasInstance>;
}

bool VJSFileEntrySyncClass::_HasInstance (const VJSParms_hasInstance &inParms) 
{
	xbox_assert(inParms.GetObject().HasSameRef(inParms.GetObjectToTest()));
 
	return inParms.GetObject().IsOfClass(VJSFileEntrySyncClass::Class());
}

void VJSMetadataClass::GetDefinition (ClassDefinition &outDefinition)
{
	outDefinition.className		= "Metadata";
	outDefinition.hasInstance	= js_hasInstance<_HasInstance>;
}

bool VJSMetadataClass::_HasInstance (const VJSParms_hasInstance &inParms) 
{
	xbox_assert(inParms.GetObject().HasSameRef(inParms.GetObjectToTest()));
 
	return inParms.GetObject().IsOfClass(VJSMetadataClass::Class());
}

VJSObject VJSMetadataClass::NewInstance (const VJSContext &inContext, const VTime &inModificationTime)
{
	VJSObject	newMetadata	= CreateInstance(inContext, NULL);
	VJSValue	modificationDate(inContext);

	modificationDate.SetTime(inModificationTime);
	newMetadata.SetProperty("modificationTime", modificationDate, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);

	return newMetadata;
}

const char	*VJSFileErrorClass::sErrorNames[NUMBER_CODES+1] = {

	"",	// OK
	"NOT_FOUND_ERR",
	"SECURITY_ERR", 
	"ABORT_ERR",
	"NOT_READABLE_ERR",
	"ENCODING_ERR",
	"NO_MODIFICATION_ALLOWED_ERR",
	"INVALID_STATE_ERR",
	"SYNTAX_ERR",
	"INVALID_MODIFICATION_ERR",
	"QUOTA_EXCEEDED_ERR",
	"TYPE_MISMATCH_ERR",
	"PATH_EXISTS"

};

const OsType	VJSFileErrorClass::VErrorComponentSignature = 'JSFE';

void VJSFileErrorClass::GetDefinition (ClassDefinition &outDefinition)
{
    static inherited::StaticFunction functions[] =
	{
		{	"toString()",	js_callStaticFunction<_toString>,	JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{	0,				0,									0																			},
	};


    static inherited::StaticValue values[] =
        {
            { "code",						js_getProperty<VJSFileErrorClass::_code>,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { "NOT_FOUND_ERR",				js_getProperty<VJSFileErrorClass::_getCode<VJSFileErrorClass::NOT_FOUND_ERR> >,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { "SECURITY_ERR",				js_getProperty<VJSFileErrorClass::_getCode<VJSFileErrorClass::SECURITY_ERR> >,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { "ABORT_ERR",					js_getProperty<VJSFileErrorClass::_getCode<VJSFileErrorClass::ABORT_ERR> >,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { "NOT_READABLE_ERR",			js_getProperty<VJSFileErrorClass::_getCode<VJSFileErrorClass::NOT_READABLE_ERR> >,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { "ENCODING_ERR",				js_getProperty<VJSFileErrorClass::_getCode<VJSFileErrorClass::ENCODING_ERR> >,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { "NO_MODIFICATION_ALLOWED_ERR",	js_getProperty<VJSFileErrorClass::_getCode<VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR> >,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { "INVALID_STATE_ERR",			js_getProperty<VJSFileErrorClass::_getCode<VJSFileErrorClass::INVALID_STATE_ERR> >,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { "SYNTAX_ERR",					js_getProperty<VJSFileErrorClass::_getCode<VJSFileErrorClass::SYNTAX_ERR> >,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { "INVALID_MODIFICATION_ERR",	js_getProperty<VJSFileErrorClass::_getCode<VJSFileErrorClass::INVALID_MODIFICATION_ERR> >,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { "QUOTA_EXCEEDED_ERR",			js_getProperty<VJSFileErrorClass::_getCode<VJSFileErrorClass::QUOTA_EXCEEDED_ERR> >,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { "TYPE_MISMATCH_ERR",			js_getProperty<VJSFileErrorClass::_getCode<VJSFileErrorClass::TYPE_MISMATCH_ERR> >,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { "PATH_EXISTS_ERR",			js_getProperty<VJSFileErrorClass::_getCode<VJSFileErrorClass::PATH_EXISTS_ERR> >,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { 0, 0, 0, 0}
        };

	outDefinition.className			= "FileError";
	outDefinition.staticFunctions	= functions;	
	outDefinition.staticValues		= values;	
}

VJSObject VJSFileErrorClass::NewInstance (const VJSContext &inContext, sLONG inCode)
{
	xbox_assert(inCode >= FIRST_CODE && inCode <= LAST_CODE);

	return CreateInstance(inContext, (void *) (sLONG_PTR) inCode);
}


void VJSFileErrorClass::_code( VJSParms_getProperty &ioParms, void *inCode)
{
	ioParms.ReturnNumber( (sLONG_PTR) inCode);
}


void VJSFileErrorClass::_toString (VJSParms_callStaticFunction &ioParms, void *inCode)
{
	sLONG	code = (sLONG) (sLONG_PTR) inCode;
	xbox_assert( code >= FIRST_CODE && code <= LAST_CODE);

	ioParms.ReturnString(sErrorNames[code]);
}


VError VJSFileErrorClass::Throw( sLONG inCode)
{
	VError error;
	if (inCode != OK)
	{
		xbox_assert( inCode >= FIRST_CODE && inCode <= LAST_CODE);
		StThrowError<> err( MAKE_VERROR( VErrorComponentSignature, inCode));
		err->SetString( "error_description", GetMessage( inCode));
		error = err.GetError();
	}
	else
	{
		error = VE_OK;
	}
	return error;
}


const VString VJSFileErrorClass::GetMessage( sLONG inCode)
{
	xbox_assert( inCode >= FIRST_CODE && inCode <= LAST_CODE);
	return VString( sErrorNames[inCode]);
}


void VJSFileErrorClass::ConvertFileErrorCodeToException( sLONG inCode, VJSObject& outExceptionObject)
{
	outExceptionObject = NewInstance( outExceptionObject.GetContext(), inCode);
	outExceptionObject.SetProperty( "code", inCode);
	outExceptionObject.SetProperty( "name", CVSTR( "FileError"));
	outExceptionObject.SetProperty( "message", GetMessage( inCode));
}


bool VJSFileErrorClass::ConvertFileErrorToException( VErrorBase *inFileError, VJSObject& outExceptionObject)
{
	VError error = inFileError->GetError();
	OsType component = COMPONENT_FROM_VERROR( error);
	
	// convert some well known xbox errors to standard FileError DOMError
	switch( error)
	{
		case VE_FS_PATH_ENCODING_ERROR:	ConvertFileErrorCodeToException( ENCODING_ERR, outExceptionObject); break;
		case VE_FS_NOT_AUTHORIZED:		ConvertFileErrorCodeToException( SECURITY_ERR, outExceptionObject); break;
		case VE_FS_NOT_FOUND:			ConvertFileErrorCodeToException( NOT_FOUND_ERR, outExceptionObject); break;
		case VE_FS_INVALID_STATE_ERROR:	ConvertFileErrorCodeToException( INVALID_STATE_ERR, outExceptionObject); break;
	
		default:
			switch( component)
			{
				case VErrorComponentSignature:
					{
						ConvertFileErrorCodeToException( ERRCODE_FROM_VERROR( error), outExceptionObject);
						break;
					}
			}
			break;
	}
	return outExceptionObject.IsObject();
}


//=======================================================================


void VJSFileErrorConstructorClass::GetDefinition( ClassDefinition &outDefinition)
{
    static inherited::StaticValue values[] =
        {
            { "NOT_FOUND_ERR",				js_getProperty<VJSFileErrorClass::_getCode<VJSFileErrorClass::NOT_FOUND_ERR> >,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { "SECURITY_ERR",				js_getProperty<VJSFileErrorClass::_getCode<VJSFileErrorClass::SECURITY_ERR> >,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { "ABORT_ERR",					js_getProperty<VJSFileErrorClass::_getCode<VJSFileErrorClass::ABORT_ERR> >,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { "NOT_READABLE_ERR",			js_getProperty<VJSFileErrorClass::_getCode<VJSFileErrorClass::NOT_READABLE_ERR> >,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { "ENCODING_ERR",				js_getProperty<VJSFileErrorClass::_getCode<VJSFileErrorClass::ENCODING_ERR> >,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { "NO_MODIFICATION_ALLOWED_ERR",	js_getProperty<VJSFileErrorClass::_getCode<VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR> >,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { "INVALID_STATE_ERR",			js_getProperty<VJSFileErrorClass::_getCode<VJSFileErrorClass::INVALID_STATE_ERR> >,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { "SYNTAX_ERR",					js_getProperty<VJSFileErrorClass::_getCode<VJSFileErrorClass::SYNTAX_ERR> >,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { "INVALID_MODIFICATION_ERR",	js_getProperty<VJSFileErrorClass::_getCode<VJSFileErrorClass::INVALID_MODIFICATION_ERR> >,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { "QUOTA_EXCEEDED_ERR",			js_getProperty<VJSFileErrorClass::_getCode<VJSFileErrorClass::QUOTA_EXCEEDED_ERR> >,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { "TYPE_MISMATCH_ERR",			js_getProperty<VJSFileErrorClass::_getCode<VJSFileErrorClass::TYPE_MISMATCH_ERR> >,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { "PATH_EXISTS_ERR",			js_getProperty<VJSFileErrorClass::_getCode<VJSFileErrorClass::PATH_EXISTS_ERR> >,	NULL,    JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete},
            { 0, 0, 0, 0}
        };

	outDefinition.className			= "FileErrorConstructor";
	outDefinition.staticValues		= values;	
}


void VJSFileErrorConstructorClass::_construct( VJSParms_construct &ioParms)
{
	sLONG code;
	if (!ioParms.GetLongParam( 1, &code))
		vThrowError( VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
	else if (code < VJSFileErrorClass::FIRST_CODE || code > VJSFileErrorClass::LAST_CODE)
		vThrowError( VE_JVSC_WRONG_NUMBER_ARGUMENT, "1");
	else
		ioParms.ReturnConstructedObject( VJSFileErrorClass::NewInstance( ioParms.GetContext(), code));
}


void VJSFileErrorConstructorClass::_getConstructorObject( VJSParms_getProperty &ioParms, void *)
{
	ioParms.ReturnValue( VJSFileErrorConstructorClass::CreateInstance( ioParms.GetContext(), NULL));
}

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
#ifndef __VJS_W3C_FILE_SYSTEM__
#define __VJS_W3C_FILE_SYSTEM__

#include "VJSClass.h"
#include "VJSValue.h"

#include "VJSWorker.h"

// W3C File System API implementation:
//
//	http://www.w3.org/TR/file-system-api/
//
// Objects must not be shared among workers, not thread safe.
// 
// Notes:
//
//	* DirectoryEntry and DirectoryEntrySync have an additional Folder() function not in W3C specification.

BEGIN_TOOLBOX_NAMESPACE

// LocalFileSystem (3.4.1) and LocalFileSystemSync (3.4.2) interfaces. Each worker has a VJSLocalFileSystem 
// object which encapsulates all the "file systems" (sandboxes) available.

class VJSFileSystem;

class XTOOLBOX_API VJSLocalFileSystem : public VObject, public IRefCountable
{
public:

	enum {

		NAMED_FS		= -1,

		FS_FIRST_TYPE	= 0,

		TEMPORARY		= 0,
		PERSISTENT		= 1,

		DATA			= 2,	// Not part of W3C specification.

		FS_LAST_TYPE	= 2,

	};

	// File systems names. 

	static	const char			*sTEMPORARY_PREFIX;		// Unique for each request.
	static	const char			*sPERSISTENT_NAME;		// Shared by all.
	static	const char			*sRELATIVE_PREFIX;		// Unique for each authorized path.

	// Add LocalFileSystem and LocalFileSystemSync interfaces support to global class. 
	// Only to be used by VJSGlobalClass.

	static	void				AddInterfacesSupport (const VJSContext &inContext);

	// RetainLocalFileSystem() is only to be used by VJSWorker. This will create a new VJSLocalFileSystem object. 
	// By default, project's path is authorized.
	
	static	VJSLocalFileSystem*	CreateLocalFileSystem (VJSContext &inContext, const VFilePath &inProjectPath);

			// Release all file systems. This will invalidate all file systems of this local file system. Memory is actually freed 
			// when reference counts are zero. Temporary file systems have their folders removed.

			void				ReleaseAllFileSystems ();

			// Only to be used by VJSLocalFileSystem and VJSW3CFSEvent. If inType is DATA, inQuota is just ignored.

			void				RequestFileSystem (const VJSContext &inContext, sLONG inType, VSize inQuota, VJSObject& outResult, bool inIsSync, const VString &inFileSystemName);
			void				ResolveURL (const VJSContext &inContext, const VString &inURL, bool inIsSync, VJSObject& outResult);

	static	VJSObject			GetInstance (const VJSContext& inContext, VJSFileSystem *inFileSystem, bool inSync);
	static	VJSObject			GetInstance( const VJSContext& inContext, VFileSystem *inFileSystem, bool inSync);

private: 

	// VJSFileSystem is a IRefCountable, but is stored as a "simple" pointer. VJSFileSystem's
	// destructor will remove itself. Not thread safe (VJSLocalFileSystem is not to be shared
	// with another thread).

	typedef	std::map<VString, VRefPtr<VJSFileSystem>, VStringLessCompareStrict>	SMap;
	
	VFilePath					fProjectPath;
	SMap						fFileSystems;

								VJSLocalFileSystem (VJSContext &inContext, const VFilePath &inRelativeRoot);
	virtual						~VJSLocalFileSystem ();

			// Only to be used by VJSLocalFileSystem.

			VJSFileSystem*		RetainTemporaryFileSystem (VSize inQuota);
			VJSFileSystem*		RetainPersistentFileSystem (VSize inQuota);
			VJSFileSystem*		RetainNamedFileSystem (const VJSContext &inContext, const VString &inFileSystemName);
			VJSFileSystem*		RetainJSFileSystemForFileSystem( const VJSContext& inContext, VFileSystem *inFileSystem);

	static void					_requestFileSystem (VJSParms_callStaticFunction &ioParms, void *);
    static void					_resolveLocalFileSystemURL (VJSParms_callStaticFunction &ioParms, void *);
	static void					_requestFileSystemSync (VJSParms_callStaticFunction &ioParms, void *);
    static void					_resolveLocalFileSystemSyncURL (VJSParms_callStaticFunction &ioParms, void *);
	static void					_fileSystem (VJSParms_callStaticFunction &ioParms, void *);

	// inFromFunction is used for FileSystem() function implementation. Return an asynchronous FileSystem object but the
	// FileSystem() function is synchronous.

	static void					_requestFileSystem (VJSParms_callStaticFunction &ioParms, bool inIsSync, bool inFromFunction = false);

	// resolveLocalFileSystemURL() and resolveLocalFileSystemSyncURL() always return a VJSEntry from the "relative" file system.
	// However the URL can be absolute or relative.

	static void					_resolveLocalFileSystemURL (VJSParms_callStaticFunction &ioParms, bool inIsSync);

	static void					_addTypeConstants (VJSObject &inFileSystemObject);
};

class XTOOLBOX_API VJSFileSystem : public VObject, public IRefCountable
{
public:

							VJSFileSystem (const VString &inName, const VFilePath &inRoot, EFSOptions inOptions, VSize inQuota);

							VJSFileSystem (VFileSystem *inFileSystem);	// Will do a retain on inFileSystem.

	virtual					~VJSFileSystem ();

	const VString&			GetName() const				{	return fFileSystem->GetName();		}
	const VFilePath&		GetRoot() const				{	return fFileSystem->GetRoot();		}	

	VFileSystem*			GetFileSystem() const		{	return fFileSystem;					}
	
	bool					IsValid () const			{	return fFileSystem->IsValid();		}

	VJSObject				GetRootEntryObject( const VJSContext& inContext, bool inIsSync);

	bool					IsAuthorized (const VFilePath &inPath)	{	return fFileSystem->IsAuthorized(inPath);	}

	VError					ParseURL (const VString &inURL, VFilePath *ioPath, bool inThrowErrorIfNotFound, bool *outIsFile);

private: 

	VFileSystem		*fFileSystem;
};

class XTOOLBOX_API VJSFileSystemClass : public VJSClass<VJSFileSystemClass, VJSFileSystem>
{
public:

	static void				GetDefinition (ClassDefinition &outDefinition);	

private:

	static void				_Initialize (const VJSParms_initialize &inParms, VJSFileSystem *inFileSystem);
	static void				_Finalize (const VJSParms_finalize &inParms, VJSFileSystem *inFileSystem);
	static bool				_HasInstance (const VJSParms_hasInstance &inParms);	
};

class XTOOLBOX_API VJSFileSystemSyncClass : public VJSClass<VJSFileSystemSyncClass, VJSFileSystem>
{
public:

	static void				GetDefinition (ClassDefinition &outDefinition);	

private:

	static void				_Initialize (const VJSParms_initialize &inParms, VJSFileSystem *inFileSystem);
	static void				_Finalize (const VJSParms_finalize &inParms, VJSFileSystem *inFileSystem);
	static bool				_HasInstance (const VJSParms_hasInstance &inParms);		
};

// Files and folders have a shared set of attributes and methods.

class XTOOLBOX_API VJSEntry : public VObject, public IRefCountable
{
public:

	// Flags for getFile() and getDirectory().

	enum {
	
		eFLAG_CREATE	= 0x01,		// If doesn't exists, create it.
		eFLAG_EXCLUSIVE	= 0x02,		// If eFLAG_CREATE is set and already exists, fail.
		
	};
	
	// Create a DirectoryEntry/DirectoryEntrySync or FileEntry/FileEntrySync object.
	// It is up to reator to determine if the entry is for a folder or a file, and that it is valid.	

	static VJSObject		CreateObject (const VJSContext &inContext, bool inIsSync, VJSFileSystem *inFileSystem, const VFilePath &inPath, bool inIsFile);
	static VJSObject		CreateRootObject (const VJSContext &inContext, bool inIsSync, VJSFileSystem *inFileSystem);

	VJSFileSystem*			GetFileSystem ()	{	return fFileSystem;	}
	bool					IsSync()			{	return fIsSync;		}
	bool					IsFile ()			{	return fIsFile;		}
	const VFilePath&		GetPath ()			{	return fPath;		}

	// Execute operation. Return error code (see VJSFileErrorClass) along with appropriate object (success) or exception (failure).

	void					GetMetadata (const VJSContext &inContext, VJSObject& outResult);
	void					MoveTo (const VJSContext &inContext, VJSEntry *inTargetEntry, const VString &inNewName, VJSObject& outResult);
	void					CopyTo (const VJSContext &inContext, VJSEntry *inTargetEntry, const VString &inNewName, VJSObject& outResult);
	void					Remove (const VJSContext &inContext);
	void					GetParent (const VJSContext &inContext, VJSObject& outResult);

	// DirectoryEntry and DirectoryEntrySync specific.

	void					GetFile (const VJSContext &inContext, const VString &inURL, sLONG inFlags, VJSObject& outResult);
	void					GetDirectory (const VJSContext &inContext, const VString &inURL, sLONG inFlags, VJSObject& outResult);
	void					RemoveRecursively (const VJSContext &inContext);

	void					Folder (const VJSContext &inContext, VJSObject& outResult);

	// FileEntry and FileEntrySync specific.

	void					CreateWriter (const VJSContext &inContext, VJSObject& outResult);
	void					File (const VJSContext &inContext, VJSObject& outResult);

private:

friend class VJSDirectoryEntryClass;
friend class VJSFileEntryClass;

friend class VJSDirectoryEntrySyncClass;
friend class VJSFileEntrySyncClass;

	VJSFileSystem			*fFileSystem;
	VFilePath				fPath;
	bool					fIsFile;
	bool					fIsSync;

							VJSEntry (VJSFileSystem *inFileSystem, const VFilePath &inPath, bool inIsFile, bool inIsSync);
	virtual					~VJSEntry ();

	// Entry/EntrySync interface functions.
		
	static	void			_getMetadata (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry);
	static	void			_moveTo (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry);
	static	void			_copyTo (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry);
	static	void			_toURL (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry);
    static	void			_remove (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry);
	static	void			_getParent (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry);
	static	void			_filesystem( VJSParms_getProperty& ioParms, VJSEntry *inEntry);

	// Dir	ectoryEntry/DirectoryEntrySync interface functions.

	static	void			_createReader (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry);
    static	void			_getFile (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry);
    static	void			_getDirectory (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry);
    static	void			_removeRecursively (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry);

	static	void			_folder (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry);

	// Fil	eEntry/FileEntrySync interface functions.

	static	void			_createWriter (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry);
	static	void			_file (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry);

	// Hel	per functions.

	static	void			_moveOrCopyTo (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry, bool inIsMoveTo);
	static	void			_getFileOrDirectory (VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry, bool inIsGetFile);

			bool			_IsRoot() const	{ return fFileSystem->GetRoot() == fPath;}

			bool			_IsNameCorrect (const VString &inName);

	static	VJSObject		_CreateObject( const VJSContext &inContext, bool inIsSync, VJSFileSystem *inFileSystem, const VFilePath &inPath, bool inIsFile, bool inIsRoot);
};

// DirectoryEntry (section 5.3) and DirectoryEntrySync (section 6.3) objects.

class XTOOLBOX_API VJSDirectoryEntryClass : public VJSClass<VJSDirectoryEntryClass, VJSEntry>
{
typedef VJSClass<VJSDirectoryEntryClass, VJSEntry> inherited;
public:

	static void	GetDefinition (ClassDefinition &outDefinition);
		
private: 

friend class VJSDirectoryEntrySyncClass;

	static void	_Initialize (const VJSParms_initialize &inParms, VJSEntry *inEntry);
	static void	_Finalize (const VJSParms_finalize &inParms, VJSEntry *inEntry);
	static bool	_HasInstance (const VJSParms_hasInstance &inParms);
};

class XTOOLBOX_API VJSDirectoryEntrySyncClass : public VJSClass<VJSDirectoryEntrySyncClass, VJSEntry>
{
typedef VJSClass<VJSDirectoryEntrySyncClass, VJSEntry> inherited;
public:

	static void	GetDefinition (ClassDefinition &outDefinition);	

private:

	static bool	_HasInstance (const VJSParms_hasInstance &inParms);
};

class XTOOLBOX_API VJSDirectoryReader : public VObject, public IRefCountable
{
public:

	// Maximum number of entries returned by readEntries() at once.

	static const sLONG		kMaximumEntries	= 128;

	static VJSObject		NewReaderObject (const VJSContext &inContext, VJSEntry *inEntry);

	bool					ReadEntries (const VJSContext &inContext, VJSObject& outResult);

	bool					IsSync ()		{	return fIsSync;		}
	bool					IsReading ()	{	return fIsReading;	}

	void					SetAsReading ()	{	fIsReading = true;	}	

private:

	bool					fIsSync;
	VJSFileSystem*			fFileSystem;

	VFolder*				fFolder;
	VFolderIterator*		fFolderIterator;
	VFileIterator*			fFileIterator;

	bool					fIsReading;		// Only for asynchronous operation, prevents calling readEntries() several times.
	
							VJSDirectoryReader (VJSFileSystem *inFileSystem, const VFilePath &inPath, bool inIsSync);
	virtual					~VJSDirectoryReader();
};

class XTOOLBOX_API VJSDirectoryReaderClass : public VJSClass<VJSDirectoryReaderClass, VJSDirectoryReader>
{
public:

	static void	GetDefinition (ClassDefinition &outDefinition);
	
private:

friend class VJSDirectoryReaderSyncClass;

	static void	_Initialize (const VJSParms_initialize &inParms, VJSDirectoryReader *inDirectoryReader);
	static void	_Finalize (const VJSParms_finalize &inParms, VJSDirectoryReader *inDirectoryReader);
	static bool	_HasInstance (const VJSParms_hasInstance &inParms);

	static void	_readEntries (VJSParms_callStaticFunction &ioParms, VJSDirectoryReader *inDirectoryReader);
};

class XTOOLBOX_API VJSDirectoryReaderSyncClass : public VJSClass<VJSDirectoryReaderSyncClass, VJSDirectoryReader>
{
public:

	static void	GetDefinition (ClassDefinition &outDefinition);
		
private:

	static bool	_HasInstance (const VJSParms_hasInstance &inParms);
	static void	_readEntries (VJSParms_callStaticFunction &ioParms, VJSDirectoryReader *inDirectoryReader);
};

// FileEntry (section 5.5) and FileEntrySync objects.

class XTOOLBOX_API VJSFileEntryClass : public VJSClass<VJSFileEntryClass, VJSEntry>
{
typedef VJSClass<VJSFileEntryClass, VJSEntry> inherited;
public:

	static void	GetDefinition (ClassDefinition &outDefinition);
	
private:

friend class VJSFileEntrySyncClass;

	static void	_Initialize (const VJSParms_initialize &inParms, VJSEntry *inEntry);
	static void	_Finalize (const VJSParms_finalize &inParms, VJSEntry *inEntry);
	static bool	_HasInstance (const VJSParms_hasInstance &inParms);
};

class XTOOLBOX_API VJSFileEntrySyncClass : public VJSClass<VJSFileEntrySyncClass, VJSEntry>
{
typedef VJSClass<VJSFileEntrySyncClass, VJSEntry> inherited;
public:

	static void	GetDefinition (ClassDefinition &outDefinition);

private:

	static bool	_HasInstance (const VJSParms_hasInstance &inParms);
};

// Metadata object, see section 4.1.

class XTOOLBOX_API VJSMetadataClass : public VJSClass<VJSMetadataClass, void>
{
public:

	static void				GetDefinition (ClassDefinition &outDefinition);
	static VJSObject	NewInstance (const VJSContext &inContext, const VTime &inModificationTime);

private:

	static bool				_HasInstance (const VJSParms_hasInstance &inParms);
};

// FileError exception object, see section 7.

class XTOOLBOX_API VJSFileErrorClass : public VJSClass<VJSFileErrorClass, void>
{
typedef VJSClass<VJSFileErrorClass, void> inherited;
public:

	enum {

		OK							= 0,

		NOT_FOUND_ERR				= 1,
		SECURITY_ERR				= 2, 
		ABORT_ERR					= 3,
		NOT_READABLE_ERR			= 4,
		ENCODING_ERR				= 5,
		NO_MODIFICATION_ALLOWED_ERR = 6,
		INVALID_STATE_ERR			= 7,
		SYNTAX_ERR					= 8,
		INVALID_MODIFICATION_ERR	= 9,
		QUOTA_EXCEEDED_ERR			= 10,
		TYPE_MISMATCH_ERR			= 11,
		PATH_EXISTS_ERR				= 12,

		FIRST_CODE					= NOT_FOUND_ERR,
		LAST_CODE					= PATH_EXISTS_ERR,
		NUMBER_CODES				= LAST_CODE - FIRST_CODE + 1,

	};
	
	static	const OsType	VErrorComponentSignature;
	
	static	void			GetDefinition (ClassDefinition &outDefinition);
	static	VJSObject		NewInstance (const VJSContext &inContext, sLONG inCode);

	static	VError			Throw( sLONG inCode);

	static	const VString	GetMessage( sLONG inCode);
		
	static	bool			ConvertFileErrorToException( VErrorBase *inFileError, VJSObject& outExceptionObject);
	static	void			ConvertFileErrorCodeToException( sLONG inCode, VJSObject& outExceptionObject);

	template<sLONG CODE>
	static	void			_getCode( VJSParms_getProperty &ioParms, void *)	{ ioParms.ReturnNumber( CODE); }

private:

	static	const char		*sErrorNames[NUMBER_CODES+1];

	static	void			_code( VJSParms_getProperty &ioParms, void *inCode);
	static	void			_toString ( VJSParms_callStaticFunction &ioParms, void *inCode);
};


class XTOOLBOX_API VJSFileErrorConstructorClass : public VJSClass<VJSFileErrorConstructorClass, void>
{
	typedef VJSClass<VJSFileErrorConstructorClass, void>	inherited;
public:

	static	void			GetDefinition (ClassDefinition &outDefinition);

	static	void			_construct(  XBOX::VJSParms_construct &ioParms);
	static	void			_getConstructorObject( VJSParms_getProperty &ioParms, void *);

private:
};

END_TOOLBOX_NAMESPACE

#endif
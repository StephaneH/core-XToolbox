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
#ifndef __VFileSystem__
#define __VFileSystem__

#include "Kernel/Sources/VFilePath.h"
#include "Kernel/Sources/VString.h"
#include "Kernel/Sources/VURL.h"

BEGIN_TOOLBOX_NAMESPACE

/* To implement sandboxing, any file or folder belongs to a "file system".
 *
 * A file system is made of a name, has a root folder, where everything "below" is authorized for access, 
 * along with a writable flag which allows writing, and a quota size (use zero if no limit).
 *
 * Note that this is a "logical" sandboxing: A file or folder authorized for read or/and write access, 
 * may not have its read or/and write bit(s) set on the actual "physical" file system. Also, there may be
 * enough space in quota, but not actual space available on disk.
 */

enum
{
	eFSO_Default		= 0,
	eFSO_Writable		= 1 << 0,	// allows writing in root folder
	eFSO_Overridable	= 1 << 1,	// allows replacing and overrding in namespace
	eFSO_Volatile		= 1 << 2,	// deletes root folder and its contents when disposing VFileSystem object
	eFSO_Private		= 1 << 4,	// this file system is not to be used by the user
};
typedef	uLONG	EFSOptions;

class XTOOLBOX_API VFileSystem : public VObject, public IRefCountable
{
public:

	// When created, a file system is valid by default. Use zero for inQuota if the file system does not have size limit. 
	// Set inUsedSpace (default is zero) to specify already used space, it is ignored if inQuota is zero.
	static	VFileSystem*			Create( const VString &inName, const VFilePath &inRoot, EFSOptions inOptions = eFSO_Default, VSize inQuota = 0, VSize inUsedSpace = 0, VError *outError = NULL);
	
	
			const VString&			GetName() const						{	return fName;	}
			const VFilePath&		GetRoot() const						{	return fRoot;	}	

	// Tells if this namespace can be replaced or overriden in a file system namespace
			bool					IsOverridable() const				{	return (fOptions & eFSO_Overridable) != 0;	}

	// Tells if this namespace root folder is to be deteted when this C++ object is destroyed
			bool					IsVolatile() const					{	return (fOptions & eFSO_Volatile) != 0;	}

	// A file system always allow reading but writing is controllable.
			bool					IsWritable () const					{	return (fOptions & eFSO_Writable) != 0;	}

	// currently a filesystem is always valid. But that may change.
			bool					IsValid() const						{	return true; }

	// this file system is not to be seen or used by the user
			bool					IsPrivate() const					{	return (fOptions & eFSO_Private) != 0;	}

	// If quota is zero, this file system doesn't have limitation.

			VSize					GetQuota () const					{	return fQuota;		}
			bool					HasQuota () const					{	return fQuota > 0;	}

			VSize					GetAvailable () const				{	return fAvailable;	}	

	// Request or relinquish bytes from file system, return appropriate error code.	

			VError					Request (VSize inSize);
			VError					Relinquish (VSize inSize);

	// Return true if the file or folder with given path is accessible in current file system.
	// Remember that it doesn't check actual physical permissions.
	// This method can be used to check access for "true absolute" path.
	
			bool					IsAuthorized( const VFilePath &inPath) const;

	// Parse an URL or a POSIX path, determine if it is a folder or file, then check for existence and if (read) access is authorized. 
	//
	// The URL or POSIX path can be absolute or relative. If an absolute path is used, it is actually absolute to the root of the file 
	// system. Hence there can be no drive letter on Windows platform. If a relative path is used, it is relative to the current folder
	// (inCurrentFolder), which must itself be below the root of the file system.
	// 
	// Method return the appropriate error code. If ok, use VFilePath's IsFile() and IsFolder() methods to determine if the
	// full path is a file or folder.

			VError					ParsePosixPath (const VString &inPosixPath, const VFilePath &inCurrentFolder, bool inThrowErrorIfNotFound, VFilePath *outFullPath);
			VError					ParseURL (const VURL &inUrl, const VFilePath &inCurrentFolder, bool inThrowErrorIfNotFound, VFilePath *outFullPath);	
			
private: 
									VFileSystem( const VString &inName, const VFilePath &inRoot, EFSOptions inOptions = eFSO_Default, VSize inQuota = 0, VSize inUsedSpace = 0);
	virtual							~VFileSystem();
									VFileSystem( const VFileSystem&);	// forbidden
			VFileSystem&			operator=(const VFileSystem&);	// forbidden
			
			VString					fName;
			VFilePath				fRoot;	
			VSize					fQuota;
			EFSOptions				fOptions;	
			VSize					fAvailable;
};



enum
{
	eFSN_PublicOnly		= 1 << 0,	// search only for public fileSystems
	eFSN_SearchParent	= 1 << 1,	// search parent chain
	eFSN_Default		= eFSN_PublicOnly | eFSN_SearchParent
};
typedef	uLONG	EFSNOptions;


class XTOOLBOX_API VFileSystemNamespace : public VObject, public IRefCountable
{
public:
									VFileSystemNamespace( VFileSystemNamespace *inParent = NULL);
	
			void					RegisterFileSystem( VFileSystem *inFileSystem);
			VError					RegisterFileSystem( const VString& inName, const VFilePath& inPath, EFSOptions inOptions = eFSO_Default);
			void					UnregisterFileSystem( const VString& inName);
			
			VFileSystem*			RetainFileSystem( const VString& inName, EFSNOptions inOptions = eFSN_Default);
			VFolder*				RetainFileSystemRootFolder( const VString& inName, EFSNOptions inOptions = eFSN_Default);

			VError					LoadFromDefinitionFile( VFile *inFile);
			
			// parse a string formatted as follows: /FILESYSTEM_NAME/{folder}/{filename}
			// the first path component must be known file system in this namespace.
			// returns false if couldn't produce a valid VFilePath or if the resolved path would get out of file system root
			bool					ParsePathWithFileSystem( const VString& inPath, VFile **outFile, VFolder **outFolder, EFSNOptions inOptions = eFSN_Default);

			bool					ParsePathWithFileSystem( const VString& inPath, VFileSystem* &outFS, VFilePath& outPath, EFSNOptions inOptions = eFSN_Default);

			typedef	std::map<VString, VRefPtr<VFileSystem> >	OrderedMapOfFileSystem;

			VError					RetainMapOfFileSystems(OrderedMapOfFileSystem& outMap, EFSNOptions inOptions = eFSN_Default);

private:
	virtual							~VFileSystemNamespace();
									VFileSystemNamespace( const VFileSystemNamespace&);		// forbidden
			VFileSystemNamespace&	operator=( const VFileSystemNamespace&);	// forbidden

	typedef	unordered_map_VString<VRefPtr<VFileSystem> >	MapOfFileSystem;
	
			VCriticalSection		fMutex;
			MapOfFileSystem			fMap;
			VFileSystemNamespace*	fParent;
};

END_TOOLBOX_NAMESPACE

#endif
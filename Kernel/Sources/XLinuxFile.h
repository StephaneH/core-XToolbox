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
#ifndef __XLinuxFile__
#define __XLinuxFile__

#include "Kernel/Sources/VObject.h"
#include "Kernel/Sources/VFilePath.h"

#include "Kernel/Sources/XLinuxFsHelpers.h"



BEGIN_TOOLBOX_NAMESPACE



class VFile;
class VURL;
class VFileDesc;
class VFileKind;
class VFileSystem;

class XLinuxFileDesc : public VObject
{
public:
	typedef enum {SET, CUR, END} Whence;

	// static VError	  OpenFile(const char* inPath, FileAccess inFileAccess, FileOpenOptions inOpenOpts,FileDescSystemRef* outFd, bool* outFailToGetLock);
	// static VError     CreateFile(const char* inPath, FileCreateOptions inCreateOpts);

    XLinuxFileDesc(FileDescSystemRef inRef);
	virtual ~XLinuxFileDesc();

	bool			  IsValid() const;
    VError            GetSize(sLONG8 *outPos) const;
    VError            SetSize(sLONG8 inSize) const;
    VError            GetData(void *outData, VSize &ioCount, sLONG8 inOffset, bool inFromStart) const;
    VError            PutData(const void *inData, VSize& ioCount, sLONG8 inOffset, bool inFromStart) const;
    VError            GetPos(sLONG8 *outSize) const;
    VError            SetPos(sLONG8 inOffset, bool inFromStart) const;
    VError            Flush() const;

    FileDescSystemRef GetSystemRef() const;

protected:
	mutable VCriticalSection fMutex;
    FileDescSystemRef   fFd;
};



class XTOOLBOX_API XLinuxFile : public VObject
{
 public:

    XLinuxFile();
    virtual ~XLinuxFile();

    VError Init(VFile *inOwner);

	VError Open(const FileAccess inFileAccess, FileOpenOptions inOptions, VFileDesc** outFileDesc) const;
	VError Create(FileCreateOptions inOptions) const;
    VError Copy(const VFilePath& inDestination, VFileSystem *inDestinationFileSystem, VFile** outFile, FileCopyOptions inOptions) const;
    VError Move(const VFilePath& inDestinationPath, VFileSystem *inDestinationFileSystem, VFile** outFile, FileCopyOptions iOptions) const;
    VError Rename(const VString& inName, VFile** outFile) const;
    VError Delete() const;
	VError MoveToTrash() const;
    bool   Exists() const;

	//Not the same as Windows - There is no creation time on Linux !
    VError GetTimeAttributes(VTime* outLastModification=NULL, VTime* outLastStatusChange=NULL, VTime* outLastAccess=NULL) const;
    VError SetTimeAttributes(const VTime* inLastModification=NULL, const VTime* UNUSED_CREATION_TIME=NULL, const VTime* inLastAccess=NULL) const;

    VError GetFileAttributes(EFileAttributes &outFileAttributes) const;
    VError SetFileAttributes(EFileAttributes inFileAttributes) const;

    VError GetVolumeFreeSpace(sLONG8* outFreeBytes, sLONG8* outTotalBytes, bool inWithQuotas) const;

    //   VError RevealInFinder() const; - Not on the server !

    VError CreateAlias(const VFilePath &inTargetPath, const VFilePath* inRelativeAliasSource) const;
    VError ResolveAlias(VFilePath &outTargetPath) const;
    bool   IsAliasFile(bool* outIsValidAlias = NULL) const;

    VError ConformsTo(const VFileKind& inKind, bool *outConforman) const;
	// Returns true if the first file kind is conform to the second file kind
    static	bool IsFileKindConformsTo( const VFileKind& inFirstFileKind, const VFileKind& inSecondFileKind);

    VError GetPosixPath(VString& outPath) const;

    VError GetSize(sLONG8* outSize) const;

    //   Voir ces deux là de plus près !
    VError GetResourceForkSize(sLONG8* outSize) const;
    bool   HasResourceFork() const;

    //
    // - Rajouter la possibilite de mettre un umask par defaut ?
    // - exposer une interface avec symbols type ChMod(Add, User|Group, Read|Write|Exec) (Add/Rem/test) ?
    //


    //   VError MakeWritableByAll();              // Optional
    //   bool   IsWritableByAll();                // Optional

    VError SetPermissions(uWORD inMode); // Optional and usefull
    VError GetPermissions(uWORD* outMode); // Optional and usefull

    //   Voir celle là de plus près !
    //     VError ConformsTo(const VFileKind& inKind, bool* outConformant) const;

    VError Exchange(const VFile& inExhangeWith) const;

	static	VFileKind* CreatePublicFileKind(const VString& inID);
	static	VFileKind* CreatePublicFileKindFromOsKind(const VString& inOsKind);
	static	VFileKind* CreatePublicFileKindFromExtension(const VString& inExtension);


 private:

	VFile*              fOwner;
    PathBuffer			fPath;
};



// class XTOOLBOX_API XLinuxFileIterator : public VObject
// {
//  public:
//     XLinuxFileIterator(const VFolder* inFolder, FileIteratorOptions inOptions);
//     virtual ~XLinuxFileIterator();

//     VFile* Next();

//  protected:

// 	FileIteratorOptions	fOpts;
// 	VFilePath			fFolderPath;
// 	DIR*				fDir;
// 	struct dirent		fEntry;
// };


class XTOOLBOX_API XLinuxFileIterator : public VObject
{
 public:
    XLinuxFileIterator(const VFolder* inFolder, FileIteratorOptions inOptions);
    virtual ~XLinuxFileIterator();

    VFile* Next();

 protected:

	FileIteratorOptions fOpts;
	FsIterator*			fIt;
	VFileSystem*		fFileSystem;
};


typedef XLinuxFile          XFileImpl;
typedef XLinuxFileDesc      XFileDescImpl;
typedef XLinuxFileIterator  XFileIteratorImpl;



END_TOOLBOX_NAMESPACE



#endif

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
#ifndef __XLinuxFsHelpers__
#define __XLinuxFsHelpers__



#include "VErrorContext.h"
#include "VTime.h"
#include "VFilePath.h"
#include "VString.h"

#if VERSION_LINUX
	#include <sys/vfs.h> /* or <sys/statfs.h> */
#endif

#include <sys/mman.h>
#include <dirent.h>
#include <dlfcn.h>

#include <stack>


BEGIN_TOOLBOX_NAMESPACE



typedef int FileDescSystemRef;



////////////////////////////////////////////////////////////////////////////////
//
// PathBuffer
//
////////////////////////////////////////////////////////////////////////////////

class NameBuffer;

class XTOOLBOX_API PathBuffer
{
public :

	typedef enum {withRealPath, withoutRealPath} Mode;


	PathBuffer();
	
	VError Init(const VFilePath& inPath, Mode inMode=withoutRealPath);
	VError Init(const VString& inPath, Mode inMode=withoutRealPath);
	VError InitWithCwd();
	VError InitWithExe();
	VError InitWithSysCacheDir();
	VError InitWithPersistentTmpDir();
    VError InitWithUniqTmpDir();
	VError InitWithUserDir();

	PathBuffer& Reset();
	
	VError RealPath(PathBuffer* outBuf) const;
	VError ReadLink(PathBuffer* outBuf) const;
	
	const char* GetPath() const;
	
	VError ToPath(VFilePath* outPath) const;
	VError ToPath(VString* outPath) const;

	VError Folderify();	//Needed to please VFolder CTOR ; Add a trailing '/' at the end of the path.

	VError AppendName(const NameBuffer& inName);	//(You don't need to call Folderify before AppendName !)
	VError AppendName(const char* inUtf8Name);	

	bool IsDotOrDotDot();


private :
	
	char fPath[PATH_MAX+1];	//Change carrefully, a lot of helpers rely on it.
};



////////////////////////////////////////////////////////////////////////////////
//
// NameBuffer
//
////////////////////////////////////////////////////////////////////////////////

class XTOOLBOX_API NameBuffer
{
public :

	NameBuffer();
	
	VError Init(const VString& inName);

	NameBuffer& Reset();
	
	const char* GetName() const;
	
	VError ToName(VString* outPath) const;

private :
	
	char fName[NAME_MAX+1];
};



////////////////////////////////////////////////////////////////////////////////
//
// OpenHelper
//
////////////////////////////////////////////////////////////////////////////////

class OpenHelper
{
public :
	
	OpenHelper();

	FileAccess	GetFileAccess();
	OpenHelper& SetFileAccess(const FileAccess inAccess);
	OpenHelper& SetFileOpenOpts(const FileOpenOptions inOpenOpts);
   
	VError Open(const PathBuffer& inPath, FileDescSystemRef* outFd);
    
    static bool ProcessCanRead(const PathBuffer& inPath);
    static bool ProcessCanWrite(const PathBuffer& inPath);


private :
	
	VError OpenStd(const char* inPath, FileDescSystemRef* outFd, bool* outFailToGetLock);
	VError OpenMax(const char* inPath, FileDescSystemRef* outFd);

	FileAccess			fAccess;
	FileOpenOptions		fOpenOpts;
};



////////////////////////////////////////////////////////////////////////////////
//
// CreateHelper
//
////////////////////////////////////////////////////////////////////////////////

class CreateHelper
{
public :
	
	CreateHelper();

	CreateHelper& SetCreateOpts(const FileCreateOptions inCreateOpts);
  
	VError Create(const PathBuffer& inPath) const;


private :

	FileCreateOptions	fCreateOpts;
};



////////////////////////////////////////////////////////////////////////////////
//
// RemoveHelper
//
////////////////////////////////////////////////////////////////////////////////

class RemoveHelper
{
public :
	
	VError Remove(const PathBuffer& inPath);
};



////////////////////////////////////////////////////////////////////////////////
//
// StatHelper
//
////////////////////////////////////////////////////////////////////////////////

class StatHelper
{
public :
	
	StatHelper();

	VError Stat(const PathBuffer& inPath);
	VError Stat(FileDescSystemRef fd);

	bool	Access();
	bool	Access(const PathBuffer& inPath);

	typedef enum {vanillaStat=0, followLinks=1, withFileSystemStats=2} StatFlags;
	StatHelper&	SetFlags(StatFlags inFlags);

	VSize	GetSize();
	VTime   GetLastAccess();
	VTime   GetLastModification();
	VTime   GetLastStatusChange();
	sLONG8  GetFsFreeBlocks();
	sLONG8  GetFsAvailableBlocks();
	sLONG8  GetFsTotalBlocks();
	sLONG8  GetFsBlockSize();
	bool	IsBlockSpecial();
	bool	IsCharSpecial();
	bool	IsFifo();
	bool	IsRegular();
	bool	IsDir();
	bool	IsLink();
	bool	IsSocket();
    bool    ProcessCanRead();
    bool    ProcessCanWrite();


private :

	StatFlags fFlags;

	bool fDoesExist;
	bool fDoesNotExist;

	struct stat		fStat;
	struct statfs	fFsStat;
};



////////////////////////////////////////////////////////////////////////////////
//
// TouchHelper
//
////////////////////////////////////////////////////////////////////////////////

class TouchHelper
{
public :

	VError Touch(const PathBuffer& inPath, VTime inAccessTime, VTime inModificationTime) const;
};



////////////////////////////////////////////////////////////////////////////////
//
// LinkHelper
//
////////////////////////////////////////////////////////////////////////////////

class LinkHelper
{
public :

	VError Link(const PathBuffer& inLinkPath, const PathBuffer& inTargetPath) const;
};



////////////////////////////////////////////////////////////////////////////////
//
// ResizeHelper
//
////////////////////////////////////////////////////////////////////////////////

class ResizeHelper
{
public :

	VError Resize(FileDescSystemRef inFd, VSize inSize) const;
};



////////////////////////////////////////////////////////////////////////////////
//
// RenameHelper
//
////////////////////////////////////////////////////////////////////////////////

class RenameHelper
{
public :

	VError Rename(const PathBuffer& inSrc, const PathBuffer& inDst) const;
};



////////////////////////////////////////////////////////////////////////////////
//
// MapHelper
//
////////////////////////////////////////////////////////////////////////////////

class MapHelper
{
public :

	MapHelper();
	
	VError		Map(FileDescSystemRef inFd, VSize inSize);
	VError		UnMap();

	typedef enum {READ=PROT_READ, WRITE=PROT_WRITE, EXEC=PROT_EXEC, NONE=PROT_NONE} Protection;
	typedef enum {SHARED=MAP_SHARED, PRIVATE=MAP_PRIVATE, FIXED=MAP_FIXED} Flags;

	MapHelper&	SetProtection(Protection inProtection);
	MapHelper&	SetFlags(Flags inFlags);
	MapHelper&	SetPreferedAddr(void* inPreferedAddr);
	MapHelper&	SetOffset(VSize inOffset);
	bool		IsValid();
	void*		GetAddr();
 	

private :

	Protection			fProt;
	Flags				fFlags;
	void*				fPreferedAddr;
	VSize				fSize;
	VSize				fOffset;
	void*				fAddr;
};



////////////////////////////////////////////////////////////////////////////////
//
// CopyHelper
//
////////////////////////////////////////////////////////////////////////////////

class CopyHelper
{
public :
	
	CopyHelper();
	~CopyHelper();

	VError Copy(const PathBuffer& inSrc, const PathBuffer& inDst);


private :

	VError DoInit(const PathBuffer& inSrc, const PathBuffer& inDst);
	VError DoCopy();
	VError DoClean(const PathBuffer& inDst);

	VSize			  fSrcSize;
	FileDescSystemRef fSrcFd;
	FileDescSystemRef fDstFd;
};



////////////////////////////////////////////////////////////////////////////////
//
// MkdirHelper
//
////////////////////////////////////////////////////////////////////////////////

class MkdirHelper
{
public :

	VError MakeDir(const PathBuffer& inPath) const;
};



////////////////////////////////////////////////////////////////////////////////
//
// RmdirHelper
//
////////////////////////////////////////////////////////////////////////////////

class RmdirHelper
{
public :

	VError RemoveDir(const PathBuffer& inPath) const;
};



////////////////////////////////////////////////////////////////////////////////
//
// ReaddirHelper
//
////////////////////////////////////////////////////////////////////////////////

class ReaddirHelper
{
public :

    ReaddirHelper(const PathBuffer& inFolderPath, FileIteratorOptions inOptions);
    virtual ~ReaddirHelper();

	PathBuffer* Next(PathBuffer* outNextPath);
	PathBuffer* Next(PathBuffer* outNextPath, FileIteratorOptions inWanted, FileIteratorOptions* outFound);


private :

	//Default methods won't work - We mustn't use them.
	ReaddirHelper(const ReaddirHelper& toto);
	ReaddirHelper& operator=(const ReaddirHelper& toto);

	FileIteratorOptions	fOpts;
	DIR*				fDir;
	PathBuffer			fPath;
	struct dirent		fEntry;
};



////////////////////////////////////////////////////////////////////////////////
//
// FsIterator
//
////////////////////////////////////////////////////////////////////////////////

class FsIterator
{
 public:

	FsIterator(const PathBuffer& inFolderPath, FileIteratorOptions inOptions);
    ~FsIterator();

	PathBuffer* Next(PathBuffer* outNextPath, FileIteratorOptions inWanted);


private :

	//Default methods won't work - We mustn't use them.
	FsIterator(const FsIterator&);
	FsIterator& operator=(const FsIterator&);

	FileIteratorOptions			fOpts;
	std::stack<ReaddirHelper*>	fDirPtrStack;
};



////////////////////////////////////////////////////////////////////////////////
//
// DlHelper
//
////////////////////////////////////////////////////////////////////////////////

class XTOOLBOX_API DynLoadHelper
{
public :

	DynLoadHelper();
	
	VError		Open(const PathBuffer& inLibPath);
	void*		GetSymbolAddr(const NameBuffer& inSymbol);
	VError		Close();

	typedef enum {LAZY=RTLD_LAZY, NOW=RTLD_NOW} Relocation;
	typedef enum {GLOBAL=RTLD_GLOBAL, LOCAL=RTLD_LOCAL, DEEP=RTLD_DEEPBIND} Scope;

	DynLoadHelper&	SetRelocation(Relocation inMode);
	DynLoadHelper&	SetScope(Scope inMode);

private :

	void*	fHandle;
	int		fRelocationMode;
	int		fScopeMode;
};



END_TOOLBOX_NAMESPACE



#endif	//__XLinuxFsHelpers__

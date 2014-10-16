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
// #include "VErrorContext.h"
#include "VTime.h"

#include "XLinuxFsHelpers.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <pwd.h>
#include <utime.h>
#include <limits.h>


#define PERM_755 S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH
#define PERM_644 S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH



//jmo - Ranger ca qq part !
static VTime UnixToXBoxTime(time_t inTime)
{
	VTime res;
	struct tm tm;

	if(gmtime_r(&inTime, &tm)!=NULL)
		//jmo - gmtime month is in the range 0 to 11 but FromUTCTime() wants it in 1 to 12
		// YT - 31-Mar-2011 - tm_year is Year-1900 see struct tm declaration.
		res.FromUTCTime(tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, 0);

	return res;
}

static time_t XBoxToUnixTime(const VTime& inTime)
{
	struct tm tm;
	memset(&tm, 0, sizeof(tm));

	sWORD year, mon, day, hour, min, sec, ms;
	inTime.GetUTCTime(year, mon, day, hour, min, sec, ms /*unused*/);

	//jmo - mktime month is in the range 0 to 11 but GetUTCTime() give it in 1 to 12
	// YT - 31-Mar-2011 - tm_year is Year-1900 see struct tm declaration.
	tm.tm_year=year-1900, tm.tm_mon=mon-1, tm.tm_mday=day, tm.tm_hour=hour, tm.tm_min=min, tm.tm_sec=sec;

	time_t res=mktime(&tm); //Calling mktime() also sets the external variable tzname
	xbox_assert(res!=(time_t)-1);

	return res;
}


////////////////////////////////////////////////////////////////////////////////
//
// PathBuffer
//
////////////////////////////////////////////////////////////////////////////////

PathBuffer::PathBuffer() { memset(fPath, 0, sizeof(fPath)); }


VError PathBuffer::Init(const VFilePath& inPath, Mode inMode)
{
	VString absPath=inPath.GetPath();

	return Init(absPath, inMode);
}


VError PathBuffer::Init(const VString& inPath, Mode inMode)
{
	PathBuffer	tmpBuf;
	char* buf=NULL;

	//If we want the real path of the file, we perform the UTF8 conversion in a tmp buffer and then the realpath transform in 'this'
	if(inMode==withRealPath)
		buf=tmpBuf.fPath;
	else
		buf=fPath;

	VSize n=inPath.ToBlock(buf, sizeof(fPath), VTC_UTF_8, false /*without trailing 0*/, false /*no lenght prefix*/);

	if(n==sizeof(fPath))
		return VE_INVALID_PARAMETER;	//UTF8 path is more than PATH_MAX bytes long ; we do not support that.

	buf[n]=0;

	VError verr=VE_OK;

	if(inMode==withRealPath)
		verr=tmpBuf.RealPath(this);

#if VERSIONDEBUG
	// Try "export O_o=''" in your shell to trigger this file tracking facility
	// Try "export O_o='file1,file2,file3' to break on file matching substring

	const char* O_o=getenv("O_o");

	if(O_o!=NULL)
	{
		DebugMsg("[%d] PathBuffer::Init() : len=%d, path='%s' \n", VTask::GetCurrentID(), n, buf);

		if(strlen(O_o)>0)
		{
			char tmp[512];
			strncpy(tmp, O_o, sizeof(tmp)-1);
			buf[sizeof(tmp)-1]=0;

			char* pos=tmp;

			while(pos!=NULL)
			{
				char* str=strsep(&pos, ",");

				if(*str==0)
					continue;

				if(strstr(buf, str)!=NULL)
				{
					DebugMsg("[%d] PathBuffer::Init() : Found '%s', break !\n", VTask::GetCurrentID(), str);
					DebugBreakInline();
				}
			}
		}
	}
#endif

	return verr;
}


VError PathBuffer::InitWithCwd()
{
	if(getcwd(fPath, sizeof(fPath))==NULL)
		return MAKE_NATIVE_VERROR(errno);

	return VE_OK;
}


VError PathBuffer::InitWithExe()
{
	return Init(VString("/proc/self/exe"), withRealPath);
}


VError PathBuffer::InitWithSysCacheDir()
{
	char* s=strncpy(fPath, "/var/cache", sizeof(fPath));
	xbox_assert(s!=NULL);

	return VE_OK;
}


VError PathBuffer::InitWithPersistentTmpDir()
{
	//Temporary files to be preserved between reboots
	char* s=strncpy(fPath, "/var/tmp/", sizeof(fPath));
	xbox_assert(s!=NULL);

	return VE_OK;
}


VError PathBuffer::InitWithUniqTmpDir()
{
	//A path template with 6 'X' - no more, no less !
	char*s=strncpy(fPath, "/tmp/Wakanda_XXXXXX", sizeof(fPath));
	xbox_assert(s!=NULL);

	//Create the directory and replace the 'X' with the actual path
	if(mkdtemp(fPath)==NULL)
		return MAKE_NATIVE_VERROR(errno);

	return VE_OK;
}


VError PathBuffer::InitWithUserDir()
{
	//Warning : the simple way to get home (home=getenv("HOME")) doesn't work well with
	//sudo -u toto Wakanda
	//$HOME isn't modified so real user's home will be used instead of toto's home.

	long len=sysconf(_SC_GETPW_R_SIZE_MAX);

	if(len!=-1)
	{
		char buf[len];
		struct passwd pwd;
		struct passwd* result=NULL;

		getpwuid_r(getuid(), &pwd, buf, len, &result);

		if(result!=NULL)
		{
			if(result->pw_dir!=NULL && strlen(result->pw_dir)>0 && strcmp(result->pw_dir, "/")!=0)
			{
				strncpy(fPath, result->pw_dir, sizeof(fPath));
				return VE_OK;
			}

			if(result->pw_name!=NULL && strlen(result->pw_name)>0)
			{
				//Temporary files to be preserved between reboots
				snprintf(fPath, sizeof(fPath), "/var/tmp/%s", result->pw_name);
				return VE_OK;
			}
		}
	}

	return VE_INVALID_PARAMETER;
}


PathBuffer& PathBuffer::Reset() { memset(fPath, 0, sizeof(fPath)); return *this; }


VError PathBuffer::RealPath(PathBuffer* outBuf) const
{
	if(outBuf==NULL)
		return VE_INVALID_PARAMETER;


	//2nd arg MUST refer to a buffer capable of storing at least PATH_MAX characters
	char* res=realpath(fPath, outBuf->fPath);

	if(res==NULL)
		return MAKE_NATIVE_VERROR(errno);

	return VE_OK;
}


VError PathBuffer::ReadLink(PathBuffer* outBuf) const
{
	if(outBuf==NULL)
		return VE_INVALID_PARAMETER;

	ssize_t n=readlink(fPath, outBuf->fPath, sizeof(outBuf->fPath));	//Doesn't put a trailing 0

	if(n==-1)
		return MAKE_NATIVE_VERROR(errno);

	if(n==sizeof(outBuf->fPath))
		return VE_INVALID_PARAMETER;	//The target path is longer than PATH_MAX ? It shouldn't be possible !

	return VE_OK;
}


const char* PathBuffer::GetPath() const { return fPath; }


VError PathBuffer::ToPath(VFilePath* outPath) const
{
	if(outPath==NULL)
		return VE_INVALID_PARAMETER;

	VString tmpPath;
	VError verr=ToPath(&tmpPath);

	if(verr!=VE_OK)
		return verr;


	outPath->FromFullPath(tmpPath, FPS_POSIX);	//Do we really need a 'full' path here ?

	return (outPath->IsValid()==true) ? VE_OK : VE_INVALID_PARAMETER;
}


VError PathBuffer::ToPath(VString* outPath) const
{
	if(outPath==NULL)
		return VE_INVALID_PARAMETER;

	outPath->FromBlock(fPath, strlen(fPath), VTC_UTF_8);

	return VE_OK;
}


//Add a final '/' to path, so it looks like a folder. That's ackward, but needed !
VError PathBuffer::Folderify()
{
	size_t pathLen=strlen(fPath);

	if(pathLen>=PATH_MAX)
		return VE_INVALID_PARAMETER;

	if(fPath[pathLen-1]=='/')
		return VE_OK;

	fPath[pathLen]='/';
	fPath[pathLen+1]=0;

	return VE_OK;
}


VError PathBuffer::AppendName(const NameBuffer& inName)
{
	return AppendName(inName.GetName());
}


VError PathBuffer::AppendName(const char* inUtf8Name)
{
	if(inUtf8Name==NULL)
		return VE_INVALID_PARAMETER;

	size_t nameLen=strlen(inUtf8Name);

	if(nameLen>NAME_MAX)
		return VE_INVALID_PARAMETER;

	size_t pathLen=strlen(fPath);

	size_t slashLen=(pathLen>0 && fPath[pathLen-1]!='/') ? 1 : 0;

	size_t newLen=pathLen+slashLen+nameLen;

	if(newLen>PATH_MAX)
		return VE_INVALID_PARAMETER;

	char* pos=fPath+pathLen;

	size_t n;

	if(slashLen>0)
		n=sprintf(pos, "/%s", inUtf8Name);
	else
		n=sprintf(pos, "%s", inUtf8Name);

	xbox_assert(n==slashLen+nameLen);

	return VE_OK;
}


bool PathBuffer::IsDotOrDotDot()
{
	int len=strlen(fPath);

	if(len==1 && fPath[0]=='.')
		return true;

	if(len>1 && fPath[len-1]=='.' && fPath[len-2]=='/')
		return true;

	if(len==2 && fPath[0]=='.' && fPath[1]=='.')
		return true;

	if(len>2 && fPath[len-1]=='.' && fPath[len-2]=='.' && fPath[len-3]=='/')
		return true;

	return false;
}



////////////////////////////////////////////////////////////////////////////////
//
// NameBuffer
//
////////////////////////////////////////////////////////////////////////////////

NameBuffer::NameBuffer() { memset(fName, 0, sizeof(fName)); }


VError NameBuffer::Init(const VString& inName)
{
	VSize n=inName.ToBlock(fName, sizeof(fName), VTC_UTF_8, false, false);	//without trailing 0, no lenght prefix

	if(n==sizeof(fName))
		return VE_INVALID_PARAMETER;	//UTF8 name is more than NAME_MAX bytes long ; we do not support that.

	fName[n]=0;

	return VE_OK;
}


NameBuffer& NameBuffer::Reset() { memset(fName, 0, sizeof(fName)); return *this; }

const char* NameBuffer::GetName() const { return fName; }


VError NameBuffer::ToName(VString* outName) const
{
	if(outName==NULL)
		return VE_INVALID_PARAMETER;

	outName->FromBlock(fName, strlen(fName), VTC_UTF_8);

	return VE_OK;
}



////////////////////////////////////////////////////////////////////////////////
//
// OpenHelper
//
////////////////////////////////////////////////////////////////////////////////

OpenHelper::OpenHelper() : fAccess(FA_READ), fOpenOpts(FO_Default) {}


FileAccess OpenHelper::GetFileAccess() { return fAccess; }


OpenHelper& OpenHelper::SetFileAccess(const FileAccess inAccess) { fAccess=inAccess; return *this; }


OpenHelper& OpenHelper::SetFileOpenOpts(const FileOpenOptions inOpenOpts) { fOpenOpts=inOpenOpts; return *this; }


VError OpenHelper::Open(const PathBuffer& inPath, FileDescSystemRef* outFd)
{
	if(GetFileAccess()==FA_MAX)
		return OpenMax(inPath.GetPath(), outFd);

	bool failToGetLock=false;	//(we don't care)
	VError verr=OpenStd(inPath.GetPath(), outFd, &failToGetLock);

	return verr;
}


VError OpenHelper::OpenMax(const char* inPath, FileDescSystemRef* outFd)
{
	if(inPath==NULL || outFd==NULL)
		return VE_INVALID_PARAMETER;

	VError verr=VE_OK;

	// FileAccess oldAccess=GetFileAccess();

	bool failToGetLock=false;

	SetFileAccess(FA_READ_WRITE);
	verr=OpenStd(inPath, outFd, &failToGetLock);

	if(*outFd<0)
	{
		SetFileAccess(FA_SHARED);
		verr=OpenStd(inPath, outFd, &failToGetLock);
	}

	if(*outFd<0)
	{
		SetFileAccess(FA_READ);
		verr=OpenStd(inPath, outFd, &failToGetLock);
	}

	//jmo - Pourquoi ? Verifier impl win & mac !
	//SetFileAccess(oldAccess);

	return verr;
}


VError OpenHelper::OpenStd(const char* inPath, FileDescSystemRef* outFd, bool* outFailToGetLock)
{
	if(inPath==NULL || outFd==NULL || outFailToGetLock==NULL)
		return VE_INVALID_PARAMETER;

	VError verr=VE_OK;

	//Lock table : Two 'cooperative processes' P1 and P2 open the same file (p1 then p2). P1 always succeed.
	//			   The table below shows the expected results for P2 :
	//
	//			   | \ P2 opens with : |		 |			 |				 |
	//			   |  ---------------  | FA_READ | FA_SHARED | FA_READ_WRITE |
	//			   | P1 opens with : \ |		 |			 |				 |
	//			   |-------------------+---------+-----------+---------------|
	//			   |				   |		 |			 |				 |
	//			   | FA_READ		   | ok		 | ok		 | ok			 |
	//			   | FA_SHARED		   | ok		 | ok		 | KO			 |
	//			   | FA_READ_WRITE	   | ok		 | KO		 | KO			 |
	//
	//WARNING : It doesn't protect against overwrite flag because the file are trucated on open, before taking the lock.

	int openFlags=0;
	int lockType=0;

	FileAccess att=GetFileAccess();

	switch(att)             //WARNING : On Unix, the implicit 'sharing convention' for 'cooperative file locking' is that
	{                       //'shared' portions of files a read-only. But to match windows behavior, our convention is
							//that shared portions are read-write.
	case FA_READ :
		openFlags|=O_RDONLY;
							//Simplest form of read, never fail, don't care about locks : We are not a 'cooperative process',
		break;              //and we take the risk of unpredictable changes, since no one know we are reading the file !
	case FA_SHARED :
		openFlags|=O_RDWR;
		lockType=F_RDLCK;   //DESPITE THE NAME, THIS HAS NOTHING TO DO WITH A READ LOCK !
		break;              //It's a 'sharing attribute' for 'cooperative processes' that agree on a 'sharing convention'

	case FA_READ_WRITE :
		openFlags|=O_RDWR;  //DESPITE THE NAME, THIS HAS (almost) NOTHING TO DO WITH A WRITE LOCK !
		lockType=F_WRLCK;   //It's an 'exclusive attribute' for 'cooperative processes' that agree on a 'sharing convention'
		break;

	case FA_MAX :
	default:
		xbox_assert(0);
		return VE_UNIMPLEMENTED;
	}

	if(fOpenOpts & FO_Overwrite)
	{
		if(fOpenOpts & FO_CreateIfNotFound)
			openFlags|=O_CREAT|O_TRUNC;			//CREATE_ALWAYS
		else
			openFlags|=O_CREAT|O_EXCL|O_TRUNC;	//TRUNCATE_EXISTING
	}
	else
	{
		if(fOpenOpts & FO_CreateIfNotFound)
			openFlags|=O_CREAT;					 //OPEN_ALWAYS
		//		else
		//			openFlags|=0;				 //OPEN_EXISTING
	}


#if VERSION_LINUX
	if(fOpenOpts & FO_WriteThrough)
		openFlags|=O_DSYNC;
#endif

	mode_t modeFlags=PERM_644;
	*outFd=open(const_cast<char*>(inPath), openFlags, modeFlags);

	if(*outFd<0)
		return MAKE_NATIVE_VERROR(errno);

//jmo - REGLER CES FICHUS LOCKS UNE FOIS POUR TOUTES !
//
//        Since kernel 2.0, flock() is implemented as a system call in its own right
//        rather than being emulated in the GNU C library as a call to fcntl(2).  This
//        yields true BSD semantics: there is no interaction between the types of lock
//        placed by flock() and fcntl(2), and flock() does not detect deadlock.

	if(lockType!=0)	//Tout buggé ;(
	{
		struct flock lock;
		memset(&lock, 0, sizeof(lock));

		//We lock the whole file.
		lock.l_type=F_WRLCK;
		lock.l_whence=SEEK_SET;
		lock.l_start=0;
		lock.l_len=0;

		int res=fcntl(*outFd, F_SETLK, &lock);

		if(res<0)
		{
			verr=MAKE_NATIVE_VERROR(errno);

			//Did we fail to get the lock ?
			if(errno==EAGAIN || errno==EACCES)
				*outFailToGetLock=true;

			close(*outFd);
			*outFd=-1;
		}
	}

	return verr;
}


//static
bool OpenHelper::ProcessCanRead(const PathBuffer& inPath)
{
	int fd=open(const_cast<char*>(inPath.GetPath()), O_RDONLY);

	if(fd>0)
	{
		close(fd);
		return true;
	}

	return false;
}


//static
bool OpenHelper::ProcessCanWrite(const PathBuffer& inPath)
{
	int fd=open(const_cast<char*>(inPath.GetPath()), O_WRONLY);

	if(fd>0)
	{
		close(fd);
		return true;
	}

	return false;
}



////////////////////////////////////////////////////////////////////////////////
//
// CreateHelper
//
////////////////////////////////////////////////////////////////////////////////

CreateHelper::CreateHelper() : fCreateOpts(FO_Default) { }


CreateHelper& CreateHelper::SetCreateOpts(const FileCreateOptions inCreateOpts) { fCreateOpts=inCreateOpts; return *this; }


VError CreateHelper::Create(const PathBuffer& inPath) const
{
	//I'm pretty sure it doesn't behave in the same way than the windows platform.
	//(the share and overwrite behavior might be different)

	int openFlags=0;

	if(fCreateOpts & FCR_Overwrite)
		openFlags|=O_RDONLY|O_CREAT;					//CREATE_ALWAYS ?
	else
		openFlags|=O_RDONLY|O_CREAT|O_EXCL;				//CREATE_NEW

	mode_t modeFlags=PERM_644;
	int fd=open(const_cast<char*>(inPath.GetPath()), openFlags, modeFlags);

	if(fd<0)
		return MAKE_NATIVE_VERROR(errno);

	close(fd);

	return VE_OK;
}



////////////////////////////////////////////////////////////////////////////////
//
// RemoveHelper
//
////////////////////////////////////////////////////////////////////////////////

VError RemoveHelper::Remove(const PathBuffer& inPath)
{
	int res=unlink(inPath.GetPath());

	return (res==0) ? VE_OK : MAKE_NATIVE_VERROR(errno);
}



////////////////////////////////////////////////////////////////////////////////
//
// StatHelper
//
////////////////////////////////////////////////////////////////////////////////

StatHelper::StatHelper() : fFlags(vanillaStat), fDoesExist(false), fDoesNotExist(false)
{
	memset(&fStat, 0, sizeof(fStat));
	memset(&fStat, 0, sizeof(fFsStat));
}


VError StatHelper::Stat(const PathBuffer& inPath)
{
	int res=(fFlags&followLinks) ? stat(inPath.GetPath(), &fStat) : lstat(inPath.GetPath(), &fStat);

	if(res!=0)
	{
		if(errno==ENOENT || errno==ENOTDIR)
			fDoesNotExist=true;		//We are sure that the file doesn't exists
		else
			return MAKE_NATIVE_VERROR(errno);
	}
	else
		fDoesExist=true;			//We are sure that the file exists

	if(fFlags&withFileSystemStats)
	{
		//I put stats on file systems here because they are obtained through a file of the fs.
		res=statfs(inPath.GetPath(), &fFsStat);

		if(res!=0)
			return MAKE_NATIVE_VERROR(errno);
	}

	return VE_OK;
}


VError StatHelper::Stat(FileDescSystemRef fd)
{
	int res=fstat(fd, &fStat);

	if(res!=0)
		return MAKE_NATIVE_VERROR(errno);

	fDoesExist=true;			//We are sure that the file exists

	return VE_OK;
}


bool StatHelper::Access()
{
	if(fDoesExist==true)
		return true;

	if(fDoesNotExist==true)
		return false;

	//Now the tricky/buggy case : stat may fail (if we lack permissions on path for ex.). In such a
	//case, we don't really know if the file exists or not... But obviously we can't access it.

	return false;
}


bool StatHelper::Access(const PathBuffer& inPath)
{
	int res=access(inPath.GetPath(), F_OK);

	return (res==0) ? true : false ;
};


StatHelper&	StatHelper::SetFlags(StatFlags inFlags)	{ fFlags=inFlags; return *this; }

VSize  StatHelper::GetSize()						{ return fStat.st_size; }

VTime  StatHelper::GetLastAccess()					{ return UnixToXBoxTime(fStat.st_atime); }

VTime  StatHelper::GetLastModification()			{ return UnixToXBoxTime(fStat.st_mtime); }

VTime  StatHelper::GetLastStatusChange()			{ return UnixToXBoxTime(fStat.st_ctime); }

sLONG8 StatHelper::GetFsFreeBlocks()				{ return fFsStat.f_bfree; }

sLONG8 StatHelper::GetFsAvailableBlocks()			{ return fFsStat.f_bavail; }

sLONG8 StatHelper::GetFsTotalBlocks()				{ return fFsStat.f_blocks; }

sLONG8 StatHelper::GetFsBlockSize()					{ return fFsStat.f_bsize; }

bool StatHelper::IsBlockSpecial()					{ return S_ISBLK(fStat.st_mode); }

bool StatHelper::IsCharSpecial()					{ return S_ISCHR(fStat.st_mode); }

bool StatHelper::IsFifo()							{ return S_ISFIFO(fStat.st_mode); }

bool StatHelper::IsRegular()						{ return S_ISREG(fStat.st_mode); }

bool StatHelper::IsDir()							{ return S_ISDIR(fStat.st_mode); }

bool StatHelper::IsLink()							{ return S_ISLNK(fStat.st_mode); }

bool StatHelper::IsSocket()							{ return S_ISSOCK(fStat.st_mode); }

bool StatHelper::ProcessCanRead()
{
	//The best way to know if we can read a file is to open it for reading.
	//It's easy and reliable, but might change the file's last acces time.
	//This method should be transparent, but ignore mecanisms such as ACL.

	//If we own the file, check owner read permission
	if(geteuid()==fStat.st_uid)
		return fStat.st_mode&S_IRUSR ? true : false;

	//If we are a member of the file's group, check group read permission
	if(getegid()==fStat.st_gid)
		return fStat.st_mode&S_IRGRP ? true : false;

	//In the end, Check other's read permission
	if(fStat.st_mode&S_IROTH)
		return true;

	return false;
}

bool StatHelper::ProcessCanWrite()
{
	// See comments for StatHelper::ProcessCanRead().

	if(geteuid()==fStat.st_uid)
		return fStat.st_mode&S_IWUSR ? true : false;

	if(getegid()==fStat.st_gid)
		return fStat.st_mode&S_IWGRP ? true : false;

	if(fStat.st_mode&S_IWOTH)
		return true;

	return false;
}


////////////////////////////////////////////////////////////////////////////////
//
// TouchHelper
//
////////////////////////////////////////////////////////////////////////////////

VError TouchHelper::Touch(const PathBuffer& inPath, VTime inAccessTime, VTime inModificationTime) const
{
	struct utimbuf buf;
	memset(&buf, 0, sizeof(buf));

	buf.actime=XBoxToUnixTime(inAccessTime);
	buf.modtime=XBoxToUnixTime(inModificationTime);

	int res=utime(inPath.GetPath(), &buf);

	if(res<0)
		return MAKE_NATIVE_VERROR(errno);

	return VE_OK;
}



////////////////////////////////////////////////////////////////////////////////
//
// LinkHelper
//
////////////////////////////////////////////////////////////////////////////////

VError LinkHelper::Link(const PathBuffer& inLinkPath, const PathBuffer &inTargetPath) const
{
	int res=symlink(inTargetPath.GetPath(), inLinkPath.GetPath());

	return (res==0) ? VE_OK : MAKE_NATIVE_VERROR(errno);
}



////////////////////////////////////////////////////////////////////////////////
//
// ResizeHelper
//
////////////////////////////////////////////////////////////////////////////////

VError ResizeHelper::Resize(FileDescSystemRef inFd, VSize inSize) const
{
	int res=ftruncate(inFd, inSize);

	return (res==0) ? VE_OK : MAKE_NATIVE_VERROR(errno);
}



////////////////////////////////////////////////////////////////////////////////
//
// RenameHelper
//
////////////////////////////////////////////////////////////////////////////////

VError RenameHelper::Rename(const PathBuffer& inSrc, const PathBuffer& inDst) const
{
	int res=rename(inSrc.GetPath(), inDst.GetPath());

	return (res==0) ? VE_OK : MAKE_NATIVE_VERROR(errno);
}



////////////////////////////////////////////////////////////////////////////////
//
// MapHelper
//
////////////////////////////////////////////////////////////////////////////////

MapHelper::MapHelper() : fProt(READ), fFlags(SHARED), fPreferedAddr(NULL), fSize(0), fOffset(0), fAddr(MAP_FAILED) {}


VError MapHelper::Map(FileDescSystemRef inFd, VSize inSize)
{
	fSize=inSize;
	fAddr=mmap(fPreferedAddr, fSize, fProt, fFlags, inFd, fOffset);

	return (fAddr!=MAP_FAILED) ? VE_OK : MAKE_NATIVE_VERROR(errno);
}


VError MapHelper::UnMap()
{
	int res=munmap(fAddr, fSize);

	return (res==0) ? VE_OK : MAKE_NATIVE_VERROR(errno);
}


MapHelper& MapHelper::SetProtection(Protection inProtection) { fProt=inProtection; return *this; }

MapHelper& MapHelper::SetFlags(Flags inFlags) { fFlags=inFlags; return *this; }

MapHelper& MapHelper::SetPreferedAddr(void* inPreferedAddr) {fPreferedAddr=inPreferedAddr; return *this; }

MapHelper& MapHelper::SetOffset(VSize inOffset) { fOffset=inOffset; return *this; }

bool MapHelper::IsValid() { return !(fAddr==MAP_FAILED); }

void* MapHelper::GetAddr() { return fAddr; }



////////////////////////////////////////////////////////////////////////////////
//
// CopyHelper
//
////////////////////////////////////////////////////////////////////////////////

CopyHelper::CopyHelper() : fSrcSize(0), fSrcFd(-1), fDstFd(-1) {}


CopyHelper::~CopyHelper()
{
	if(fSrcFd>=0)
		close(fSrcFd);

	if(fDstFd>=0)
		close(fDstFd);
}


VError CopyHelper::Copy(const PathBuffer& inSrc, const PathBuffer& inDst)
{
	VError verr=DoInit(inSrc, inDst);

	if(verr==VE_OK && fSrcSize > 0)

		verr=DoCopy();

	if(verr!=VE_OK)
		DoClean(inDst);

	return verr;
}


VError CopyHelper::DoInit(const PathBuffer& inSrc, const PathBuffer& inDst)
{
	OpenHelper srcOpnHlp;

	VError verr=srcOpnHlp.SetFileAccess(FA_READ).Open(inSrc, &fSrcFd);

	if(verr!=VE_OK)
		return verr;

	StatHelper srcStats;

	verr=srcStats.Stat(fSrcFd);

	if(verr!=VE_OK)
		return verr;

	fSrcSize=srcStats.GetSize();

	OpenHelper dstOpnHlp;

	verr=dstOpnHlp.SetFileAccess(FA_READ_WRITE).SetFileOpenOpts(FO_CreateIfNotFound).Open(inDst, &fDstFd);

	return verr;
}


VError CopyHelper::DoCopy()
{
	ResizeHelper dstRszHlp;

	VError verr=dstRszHlp.Resize(fDstFd, fSrcSize);

	if(verr!=VE_OK)
		return verr;

	MapHelper srcMapHlp;

	verr=srcMapHlp.Map(fSrcFd, fSrcSize);

	if(verr!=VE_OK)
		return verr;

	void* srcAddr=srcMapHlp.GetAddr();

	MapHelper dstMapHlp;

	verr=dstMapHlp.SetProtection(MapHelper::WRITE).Map(fDstFd, fSrcSize);

	if(verr!=VE_OK)
	{
		srcMapHlp.UnMap();
		return verr;
	}

	void* dstAddr=dstMapHlp.GetAddr();

	memcpy(dstAddr, srcAddr, fSrcSize);

	srcMapHlp.UnMap();
	dstMapHlp.UnMap();

	return verr;
}


VError CopyHelper::DoClean(const PathBuffer& inDst)
{
	//May be the file exists, may be not. We do not really care.

	RemoveHelper rmHlp;

	return rmHlp.Remove(inDst);
}



////////////////////////////////////////////////////////////////////////////////
//
// MkdirHelper
//
////////////////////////////////////////////////////////////////////////////////

VError MkdirHelper::MakeDir(const PathBuffer& inPath) const
{
	mode_t modeFlags=PERM_755;

	int res=mkdir(inPath.GetPath(), modeFlags);

	return (res==0) ? VE_OK : MAKE_NATIVE_VERROR(errno);
}



////////////////////////////////////////////////////////////////////////////////
//
// RmdirHelper
//
////////////////////////////////////////////////////////////////////////////////

VError RmdirHelper::RemoveDir(const PathBuffer& inPath) const
{
	int res=rmdir(inPath.GetPath());

	return (res==0) ? VE_OK : MAKE_NATIVE_VERROR(errno);
}



////////////////////////////////////////////////////////////////////////////////
//
// ReaddirHelper
//
////////////////////////////////////////////////////////////////////////////////

ReaddirHelper::ReaddirHelper(const PathBuffer& inFolderPath, FileIteratorOptions inOptions) :
	fOpts(inOptions), fDir(NULL), fPath(inFolderPath)
{
	memset(&fEntry, 0, sizeof(fEntry));

	//If it fails for any reason (inFolderPath is a file for ex.), opendir returns NULL.
	fDir=opendir(inFolderPath.GetPath());
}


ReaddirHelper::~ReaddirHelper()
{
	if(fDir!=NULL)
		closedir(fDir);
}


PathBuffer* ReaddirHelper::Next(PathBuffer* outNextPath)
{
	if(fDir==NULL || outNextPath==NULL)
		return NULL;

	struct dirent* entryPtr=NULL;

	//res replaces errno on failure
	/*int res=*/ readdir_r(fDir, &fEntry, &entryPtr);

	if(entryPtr==NULL)
		return NULL;

	//reset path
	*outNextPath=fPath;

	return (outNextPath->AppendName(entryPtr->d_name)==VE_OK) ? outNextPath : NULL;
}


PathBuffer* ReaddirHelper::Next(PathBuffer* outNextPath, FileIteratorOptions inWanted, FileIteratorOptions* outFound)
{
	outNextPath=Next(outNextPath);

	if(outNextPath==NULL)
		return NULL;


	if(!outNextPath->IsDotOrDotDot())
	{
		StatHelper statHlp;

		if(inWanted&FI_RESOLVE_ALIASES)
			statHlp.SetFlags(StatHelper::followLinks);

		statHlp.Stat(*outNextPath);

		//Unless we prefer to follow them, links are exposed as special files
		if(inWanted&FI_WANT_FILES && (statHlp.IsRegular() || statHlp.IsLink()))
		{
			if(outFound!=NULL)
				*outFound=FI_WANT_FILES;

			return outNextPath;
		}

		if(inWanted&FI_WANT_FOLDERS && statHlp.IsDir())
		{
			if(outFound!=NULL)
				*outFound=FI_WANT_FOLDERS;

			return outNextPath;
		}
	}

	//We didn't found what we where looking for. Let's continue.
	return Next(outNextPath, inWanted, outFound);
}



////////////////////////////////////////////////////////////////////////////////
//
// FsIterator
//
////////////////////////////////////////////////////////////////////////////////

FsIterator::FsIterator(const PathBuffer& inFolderPath, FileIteratorOptions inOptions) : fOpts(inOptions)
{
	ReaddirHelper* rddHlp=new ReaddirHelper(inFolderPath, inOptions);
	fDirPtrStack.push(rddHlp);
}


FsIterator::~FsIterator()
{
	while(!fDirPtrStack.empty())
	{
		delete fDirPtrStack.top();
		fDirPtrStack.pop();
	}
}


PathBuffer* FsIterator::Next(PathBuffer* outNextPath, FileIteratorOptions inWanted)
{
	if(outNextPath==NULL)
		return NULL;

	while(!fDirPtrStack.empty())
	{
		FileIteratorOptions found=0;
		outNextPath=fDirPtrStack.top()->Next(outNextPath, inWanted, &found);

		if(outNextPath!=NULL)
		{
			if(inWanted&FI_RECURSIVE && found&FI_WANT_FOLDERS)
			{
				//We found a folder and want a recursive traversal. Push back for next time.
				ReaddirHelper* rddHlp=new ReaddirHelper(*outNextPath, fOpts);
				fDirPtrStack.push(rddHlp);
			}

			if(inWanted&FI_WANT_FOLDERS && found&FI_WANT_FOLDERS)
			{
				//We found a folder and want a folder - Bingo !
				return outNextPath;
			}

			if(inWanted&FI_WANT_FILES && found&FI_WANT_FILES)
			{
				//We found a file and want a file - Bingo !
				return outNextPath;
			}

			//We found something we don't want - Let's continue.
			continue;
		}

		//This directory is done.
		delete fDirPtrStack.top();
		fDirPtrStack.pop();
	}

	//The stack is empty.
	return NULL;
}



////////////////////////////////////////////////////////////////////////////////
//
// DynLoadHelper
//
////////////////////////////////////////////////////////////////////////////////

DynLoadHelper::DynLoadHelper() : fHandle(NULL), fRelocationMode(LAZY), fScopeMode(LOCAL) {};


VError DynLoadHelper::Open(const PathBuffer& inLibPath)
{
	fHandle=dlopen(inLibPath.GetPath(), fRelocationMode|fScopeMode);

	if(fHandle==NULL)
	{
		char* msg=dlerror();

		if(msg!=NULL)
			DebugMsg("dlopen error %s\n", msg);

		return VE_INVALID_PARAMETER;
	}

	return VE_OK;
}


void* DynLoadHelper::GetSymbolAddr(const NameBuffer& inSymbol)
{

	void* symbol=dlsym(fHandle, inSymbol.GetName());

	if(symbol==NULL)
	{
		char* msg=dlerror();

		if(msg!=NULL)
			DebugMsg("dlsym error %s\n", msg);
	}

	return symbol;
}


VError DynLoadHelper::Close()
{
	int res = dlclose(fHandle);

	if (res != 0)
	{
		char* msg = dlerror();
		if (msg != NULL)
			DebugMsg("dlclose error %s\n", msg);

		return VE_INVALID_PARAMETER;
	}

	return VE_OK;
}


DynLoadHelper& DynLoadHelper::SetRelocation(Relocation inMode) { fRelocationMode=inMode; return *this; }

DynLoadHelper& DynLoadHelper::SetScope(Scope inMode) { fScopeMode=inMode; return *this; }

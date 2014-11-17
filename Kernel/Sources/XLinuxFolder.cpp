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
#include "VProcess.h"
#include "VFileSystem.h"

#include "XLinuxFolder.h"


const sLONG	kMAX_PATH_SIZE=2048;	//jmo - Harmoniser tout ca... (PATH_MAX)


////////////////////////////////////////////////////////////////////////////////
//
// XLinuxFolder
//
////////////////////////////////////////////////////////////////////////////////

XLinuxFolder::XLinuxFolder():fOwner(NULL)
{
	//nothing to do !
}


//virtual
XLinuxFolder::~XLinuxFolder()
{
	//nothing to do !
}


VError XLinuxFolder::Init(VFolder *inOwner)
{
	if(inOwner==NULL)
		return VE_INVALID_PARAMETER;

	fOwner=inOwner;

	VFilePath filePath=fOwner->GetPath();

	VError verr=fPath.Init(filePath);

	if(verr!=VE_OK)
		return verr;

	//VFolder CTOR complains if it can't find a '/' at the end of the folder name.
	//fPath.Folderify(); En principe, déjà fait quand on arrive ici

	return verr;
}


VError XLinuxFolder::Create() const
{
	MkdirHelper mkdHlp;
	return mkdHlp.MakeDir(fPath);
}


VError XLinuxFolder::Delete() const
{
	RmdirHelper rmdHlp;
	return rmdHlp.RemoveDir(fPath);
}


VError XLinuxFolder::MoveToTrash() const
{
	// todo: should move to user trash folder ~/.local/share/Trash
	return Delete();
}


bool XLinuxFolder::Exists(bool /*inAcceptFolderAlias*/) const
{
	//The folder may exist but we can't access it. Let's pretend it doesn't exist !

	StatHelper statHlp;
	VError	err = statHlp.Stat(fPath);

	// Check for existence, and then that it is really a folder (WAK0073807).

	return err == VE_OK && statHlp.Access(fPath) && statHlp.IsDir();
}


VError XLinuxFolder::Rename(const VString& inName, VFolder** outFolder) const
{
	//jmo - todo : we should check that inName < NAME_MAX or use PathBuffer

	VFilePath newPath(fOwner->GetPath());
	newPath.SetName(inName);

	return Rename(newPath, fOwner->GetFileSystem(), outFolder);
}


VError XLinuxFolder::Rename(const VFilePath& inDstPath, VFileSystem *inDestinationFileSystem, VFolder** outFolder) const
{
	PathBuffer dstPath;
	dstPath.Init(inDstPath);

	RenameHelper rnmHlp;
	VError verr=rnmHlp.Rename(fPath, dstPath);

	if (outFolder != NULL)
	{
		*outFolder = (verr==VE_OK) ? new VFolder(inDstPath, inDestinationFileSystem) : NULL;
	}

	return verr;
}


VError XLinuxFolder::Move(const VFolder& inNewParent, const VString *inNewName, VFolder** outFolder) const
{
	VFilePath dstPath;
	inNewParent.GetPath(dstPath);

	if ( (inNewName == NULL) || inNewName->IsEmpty())
	{
		VString name;
		fOwner->GetName(name);
		dstPath.ToSubFolder(name);
	}
	else
	{
		dstPath.ToSubFolder(*inNewName);
	}

	return Rename(dstPath, inNewParent.GetFileSystem(), outFolder);
}


//There is no creation time on Linux, but there is a status change time (modification of file's inode).
VError XLinuxFolder::GetTimeAttributes(VTime* outLastModification, VTime* outLastStatusChange, VTime* outLastAccess) const
{
	StatHelper statHlp;
	VError verr=statHlp.Stat(fPath);

	if(verr!=VE_OK)
		return verr;

	if(outLastAccess!=NULL)
		outLastAccess->FromTime(statHlp.GetLastAccess());

	if(outLastModification!=NULL)
		outLastModification->FromTime(statHlp.GetLastModification());

	if(outLastStatusChange!=NULL)
		outLastStatusChange->FromTime(statHlp.GetLastStatusChange());

	return VE_OK;
}


VError XLinuxFolder::SetTimeAttributes(const VTime* inLastModification, const VTime* UNUSED_CREATION_TIME, const VTime* inLastAccess) const
{
	xbox_assert(UNUSED_CREATION_TIME==NULL);

	if(inLastModification==NULL && inLastAccess==NULL)
		return VE_INVALID_PARAMETER;	//We didn't failed nor succeed. Let's say we failed !

	VTime modif;
	VTime access;

	VError verr=GetTimeAttributes(&modif, NULL, &access);

	if(verr!=VE_OK)
		return verr;

	const VTime* modifPtr=(inLastModification!=NULL ? inLastModification : &modif);
	const VTime* accessPtr=(inLastAccess!=NULL ? inLastAccess : &access);

	TouchHelper tchHlp;

	verr=tchHlp.Touch(fPath, *accessPtr, *modifPtr);

	return verr;
}


VError XLinuxFolder::GetVolumeFreeSpace(sLONG8 *outFreeBytes, bool inWithQuotas) const
{
	if(outFreeBytes==NULL)
		return VE_INVALID_PARAMETER;	//We didn't failed nor succeed. Let's say we failed !

	//jmo - todo : verifier si ca marche correctement avec les quotas utilisateur

	StatHelper statHlp;
	VError verr=statHlp.SetFlags(StatHelper::withFileSystemStats).Stat(fPath);

	if(verr!=VE_OK)
		return verr;

	sLONG8 freeBlocks=(inWithQuotas ? statHlp.GetFsAvailableBlocks() : statHlp.GetFsFreeBlocks());
	sLONG8 blockSize=statHlp.GetFsBlockSize();

	if(outFreeBytes!=NULL)
		*outFreeBytes=freeBlocks*blockSize;

	return VE_OK;
}


VError XLinuxFolder::GetPosixPath(VString& outPath) const
{
	return fPath.ToPath(&outPath);
}


bool XLinuxFolder::ProcessCanRead()
{
	return OpenHelper::ProcessCanRead(fPath);
}


bool XLinuxFolder::ProcessCanWrite()
{
	return OpenHelper::ProcessCanWrite(fPath);
}


VError XLinuxFolder::SetPermissions(uWORD /*inMode*/)
{
	// Used in VFolder::LINUX_SetPermissions
	return VE_UNIMPLEMENTED;	// Postponed Linux Implementation !
}


VError XLinuxFolder::GetPermissions(uWORD* /*outMode*/)
{
	// Used in VFolder::LINUX_GetPermissions
	return VE_UNIMPLEMENTED;	// Postponed Linux Implementation !
}


VError XLinuxFolder::IsHidden(bool& outIsHidden)
{
	//No hidden file on Linux (there is a convention for 'dotfiles' on desktops, but it's not meaningfull on servers)
	outIsHidden=false;
	return VE_OK;
}


VError XLinuxFolder::RetainSystemFolder(ESystemFolderKind inFolderKind, bool inCreateFolder, VFolder **outFolder)
{
	// See http://en.wikipedia.org/wiki/Filesystem_Hierarchy_Standard

	VFolder* sysFolder=NULL;
	VError	 verr=VE_OK;

	switch(inFolderKind)
	{
	case eFK_Executable:
		{
			PathBuffer path;

			VError verr=path.InitWithExe();
			xbox_assert(verr==VE_OK);

			VFilePath filePath;

			path.ToPath(&filePath);
			xbox_assert(verr==VE_OK);

			VFile exeFile(filePath);

			sysFolder=exeFile.RetainParentFolder();
		}
		break;

	case eFK_Temporary:
		{
			//Temporary stuff goes to /tmp/<uniq dir identifier>/
			PathBuffer path;

			VError verr=path.InitWithUniqTmpDir();
			xbox_assert(verr==VE_OK);

			//VFolder CTOR complains if it can't find a '/' at the end of the folder name.
			path.Folderify();

			VFilePath folderPath;

			path.ToPath(&folderPath);
			xbox_assert(verr==VE_OK);

			sysFolder=new VFolder(folderPath);
		}
		break;

	case eFK_UserPreferences:
		{
			//Build some path like this : /home/toto/.Wakanda Linux/UserPref/
			//                       or : /var/tmp/toto/.Wakanda Linux/UserPref/
			//The second (not very standard) is used when user doesn´t have a $HOME

			PathBuffer path;

			VError verr=path.InitWithUserDir();
			xbox_assert(verr==VE_OK);

			//VFolder CTOR complains if it can't find a '/' at the end of the folder name.
			path.Folderify();

			VFilePath folderPath;

			path.ToPath(&folderPath);
			xbox_assert(verr==VE_OK);

			VString product;
			VProcess::Get()->GetProductName(product);
			
			VString dotProduct(".");
			dotProduct.AppendString(product);

			folderPath.ToSubFolder(dotProduct).ToSubFolder(VString("UserPref"));

			sysFolder=new VFolder(folderPath);

			inCreateFolder=true;
		}
		break;

	case eFK_UserDocuments :
		{
			//Build some path like this : /home/toto/.Wakanda Linux/UserDoc/
			//                       or : /var/tmp/toto/.Wakanda Linux/UserDoc/
			//The second (not very standard) is used when user doesn´t have a $HOME

			PathBuffer path;

			VError verr=path.InitWithUserDir();
			xbox_assert(verr==VE_OK);

			//VFolder CTOR complains if it can't find a '/' at the end of the folder name.
			path.Folderify();

			VFilePath folderPath;

			path.ToPath(&folderPath);
			xbox_assert(verr==VE_OK);

			VString product;
			VProcess::Get()->GetProductName(product);
			
			VString dotProduct(".");
			dotProduct.AppendString(product);

			folderPath.ToSubFolder(dotProduct).ToSubFolder(VString("UserDoc"));

			sysFolder=new VFolder(folderPath);

			inCreateFolder=true;
		}
		break;

	case eFK_UserApplicationData:
		{
			//Build some path like this : /home/toto/.Wakanda Linux/UserData/
			//                       or : /var/tmp/toto/.Wakanda Linux/UserData/
			//The second (not very standard) is used when user doesn´t have a $HOME

			PathBuffer path;

			VError verr=path.InitWithUserDir();
			xbox_assert(verr==VE_OK);

			//VFolder CTOR complains if it can't find a '/' at the end of the folder name.
			path.Folderify();

			VFilePath folderPath;

			path.ToPath(&folderPath);
			xbox_assert(verr==VE_OK);

			VString product;
			VProcess::Get()->GetProductName(product);
			
			VString dotProduct(".");
			dotProduct.AppendString(product);

			folderPath.ToSubFolder(dotProduct).ToSubFolder(VString("UserData"));

			sysFolder=new VFolder(folderPath);

			inCreateFolder=true;
		}
		break;

	case eFK_CommonApplicationData:
		{
			//CommonApplicationData is a Windows/Mac concept that Linux doesn't have...
			//(Different places could do the job in different situations.)
			//Let's go for a 'minimalist' approach with /var/tmp, which is a persistent
			//(preserved between reboots) tmp dir, where two processes can meet.

			PathBuffer path;

			VError verr=path.InitWithPersistentTmpDir();
			xbox_assert(verr==VE_OK);

			//VFolder CTOR complains if it can't find a '/' at the end of the folder name.
			path.Folderify();

			VFilePath folderPath;

			path.ToPath(&folderPath);
			xbox_assert(verr==VE_OK);

			sysFolder=new VFolder(folderPath);

			inCreateFolder=true;
		}
		break;

	case eFK_CommonCache:
		{
			//Stuff to cache goes into /var/cache/
			
			PathBuffer path;

			VError verr=path.InitWithSysCacheDir();
			xbox_assert(verr==VE_OK);

			//VFolder CTOR complains if it can't find a '/' at the end of the folder name.
			path.Folderify();

			VFilePath folderPath;

			path.ToPath(&folderPath);
			xbox_assert(verr==VE_OK);

			sysFolder=new VFolder(folderPath);

			inCreateFolder=true;
		}
		break;

	case eFK_UserCache:
		{
			//Build some path like this : /home/toto/.Wakanda Linux/UserCache/
			//                       or : /var/tmp/toto/.Wakanda Linux/UserCache/
			//The second (not very standard) is used when user doesn´t have a $HOME

			PathBuffer path;

			VError verr=path.InitWithUserDir();
			xbox_assert(verr==VE_OK);

			//VFolder CTOR complains if it can't find a '/' at the end of the folder name.
			path.Folderify();

			VFilePath folderPath;

			path.ToPath(&folderPath);
			xbox_assert(verr==VE_OK);

			VString product;
			VProcess::Get()->GetProductName(product);
			
			VString dotProduct(".");
			dotProduct.AppendString(product);

			folderPath.ToSubFolder(dotProduct).ToSubFolder(VString("UserCache"));

			sysFolder = new VFolder(folderPath);

			inCreateFolder=true;
		}
		break;

	case eFK_CommonPreferences:
	default:
		verr=VE_UNIMPLEMENTED;
		xbox_assert(false);
	}

#if VERSIONDEBUG
	//VString tmpPath=sysFolder->GetPath().GetPath();
	//DebugMsg ("[%d] XLinuxFolder::RetainSystemFolder kind=%d path=%S\n",
	//		  VTask::GetCurrentID(), inFolderKind, &tmpPath);
#endif

	if ((sysFolder!=NULL) && !sysFolder->Exists())
	{
		if(inCreateFolder)
		{
			if(sysFolder->CreateRecursive()!=VE_OK)
				ReleaseRefCountable( &sysFolder);
		}
		else
		{
			ReleaseRefCountable(&sysFolder);
		}
	}

	*outFolder = sysFolder;

	return verr;
}



////////////////////////////////////////////////////////////////////////////////
//
// XLinuxFolderIterator
//
////////////////////////////////////////////////////////////////////////////////

XLinuxFolderIterator::XLinuxFolderIterator(const VFolder *inFolder, FileIteratorOptions inOptions)
: fOpts(0)
, fIt(NULL)
, fFileSystem( inFolder->GetFileSystem())
{
	//Because Next() returns a VFolder ptr, we want folders and no files.
	fOpts=inOptions;
	fOpts|=FI_WANT_FOLDERS;
	fOpts&=~FI_WANT_FILES;

	VFilePath tmpPath;
	inFolder->GetPath(tmpPath);

	PathBuffer path;
	path.Init(tmpPath);

	fIt=new FsIterator(path, inOptions);
}


//virtual
XLinuxFolderIterator::~XLinuxFolderIterator()
{
	delete fIt;
}


VFolder* XLinuxFolderIterator::XLinuxFolderIterator::Next()
{
	PathBuffer path;

	if(fIt->Next(&path, fOpts)==NULL)
		return NULL;

	//VFolder CTOR complains if it can't find a '/' at the end of the folder name.
	path.Folderify();

	VFilePath tmpPath;
	path.ToPath(&tmpPath);

	VFolder *folder;
	if ( (fFileSystem == NULL) || tmpPath.IsChildOf( fFileSystem->GetRoot()) )
		folder = new VFolder(tmpPath, fFileSystem);
	else
		folder = NULL;
 
	return folder;
}

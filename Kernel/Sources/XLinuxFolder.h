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
#ifndef __XLinuxFolder__
#define __XLinuxFolder__

#include "Kernel/Sources/VObject.h"
#include "Kernel/Sources/VFilePath.h"

#include "Kernel/Sources/XLinuxFsHelpers.h"



BEGIN_TOOLBOX_NAMESPACE



class VString;
class VFolderIterator;
class VFileSystem;


class XLinuxFolder : public VObject
{
public:
    XLinuxFolder();
    virtual ~XLinuxFolder();

    VError        Init(VFolder *inOwner);
    VError        Create() const;
    VError        Delete() const;
	VError			MoveToTrash() const;
    bool          Exists(bool inAcceptFolderAlias) const;

    //VError      RevealInFinder() const; - Not on the server !

    VError        Rename(const VString& inName, VFolder** outFile) const;
	VError		  Rename(const VFilePath& inDstPath, VFileSystem *inDestinationFileSystem, VFolder** outFolder) const;
    VError        Move(const VFolder& inNewParent, const VString *inNewName, VFolder** outFolder) const;

	//Not the same as Windows - There is no creation time on Linux !
    VError        GetTimeAttributes(VTime* outLastModification, VTime* outLastStatusChange, VTime* outLastAccess) const;
    VError        SetTimeAttributes(const VTime* inLastModification, const VTime* UNUSED_CREATION_TIME, const VTime* inLastAccess) const;

    VError        GetVolumeFreeSpace(sLONG8 *outFreeBytes, bool inWithQuotas=true) const;

    VError        GetPosixPath(VString& outPath) const;

    //VError      MakeWritableByAll();              // Was optional in VFile...
    //bool        IsWritableByAll();                // Was optional in VFile...
    bool          ProcessCanRead();                 // Linux specific
    bool          ProcessCanWrite();                // Linux specific

    VError        SetPermissions(uWORD inMode);     // Optional and usefull
    VError        GetPermissions(uWORD *outMode);   // Optional and usefull

    VError        IsHidden(bool& outIsHidden);

    static VError RetainSystemFolder(ESystemFolderKind inFolderKind, bool inCreateFolder, VFolder **outFolder);

private:
 
	VFolder*	  fOwner;
    PathBuffer	  fPath;


};



class XTOOLBOX_API XLinuxFolderIterator
{
 public:
    XLinuxFolderIterator(const VFolder *inFolder, FileIteratorOptions inOptions);
    virtual ~XLinuxFolderIterator();

    VFolder* Next();

 protected:

	FileIteratorOptions fOpts;
	FsIterator*			fIt;
	VFileSystem*		fFileSystem;

};



typedef XLinuxFolder            XFolderImpl;
typedef XLinuxFolderIterator    XFolderIteratorImpl;



END_TOOLBOX_NAMESPACE



#endif

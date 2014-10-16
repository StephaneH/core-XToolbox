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



VFile* XLinuxLibrary::RetainExecutableFile(const VFilePath &inFilePath) const
{
    //jmo - What are we supposed to retain ? First we try the exact path...

	if(inFilePath.IsFile())
		return new VFile(inFilePath);

    //jmo - If exact path doesn't work, we assume inFilePath to be part of a bundle
    //      and try to find the corresponding shared object.
    
    VFilePath bundlePath=inFilePath;

    while(!bundlePath.MatchExtension(CVSTR("bundle"), true /*case sensitive*/) && bundlePath.IsValid())
        bundlePath=bundlePath.ToParent();

    if(!bundlePath.IsValid())
        return NULL;

    VString name;
    bundlePath.GetFolderName(name, false /*no ext.*/);

    if(name.IsEmpty())
        return NULL;

    VString soSubPath=CVSTR("Contents/Linux/")+VString(name)+CVSTR(".so");

    VFilePath soPath;
    soPath.FromRelativePath(bundlePath, soSubPath, FPS_POSIX);
            
    if(soPath.IsFile())
        return new VFile(soPath);
    
    return NULL;
}


bool XLinuxLibrary::Load(const VFilePath &inFilePath)
{
    VFile* soFile=RetainExecutableFile(inFilePath);
    VFilePath soPath=soFile->GetPath();
    soFile->Release();

	PathBuffer tmpBuf;
	tmpBuf.Init(soPath);

	if(fLib.Open(tmpBuf)!=VE_OK)
		return false;

	return true;
}

void XLinuxLibrary::Unload()
{
	fLib.Close();
}


void* XLinuxLibrary::GetProcAddress(const VString& inProcName)
{
	NameBuffer tmpBuf;
	tmpBuf.Init(inProcName);

	return fLib.GetSymbolAddr(tmpBuf);
}


VResourceFile* XLinuxLibrary::RetainResourceFile(const VFilePath &inFilePath)
{
	return NULL;    //No resources on Linux !
}


VFolder* XLinuxLibrary::RetainFolder(const VFilePath &inFilePath, BundleFolderKind inKind) const
{
    VFilePath bundlePath=inFilePath;

    while(!bundlePath.MatchExtension(CVSTR("bundle"), true /*case sensitive*/) && bundlePath.IsValid())
        bundlePath=bundlePath.ToParent();

    if(!bundlePath.IsValid())
        return NULL;

	switch(inKind)
    {
    case kBF_BUNDLE_FOLDER :            // The folder hosting the bundle
        return new VFolder(bundlePath);
    
    case kBF_EXECUTABLE_FOLDER :        // The (platform related) folder of executable file 
        return new VFolder(VFilePath(bundlePath, CVSTR("Contents/Linux/"), FPS_POSIX));

    case kBF_RESOURCES_FOLDER:			// The folder of main resources
        return new VFolder(VFilePath(bundlePath, CVSTR("Contents/Resources/"), FPS_POSIX));

    default :
        bool RetainFolderMethod=false;
        xbox_assert(RetainFolderMethod); // Need a Linux Implementation !
    }

	return NULL;
}




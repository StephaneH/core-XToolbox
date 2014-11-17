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


VLibrary::VLibrary(const VFilePath& inLibFile)
{
	fFilePath = inLibFile;
	fIsLoaded = false;
	fResFile = NULL;
	fResFileChecked = false;
}


VLibrary::~VLibrary()
{
	Unload();
}


bool VLibrary::Load()
{
	VTaskLock	locker(&fCriticalSection);

	if (!fIsLoaded)
	{
		if (fImpl.Load(fFilePath))
		{
			fIsLoaded = true;
		}
	}

	return fIsLoaded;
}


void VLibrary::Unload()
{
	VTaskLock	locker(&fCriticalSection);

	if (fIsLoaded)
	{
		fImpl.Unload();
		
		fIsLoaded = false;

		if (fResFile != NULL)
		{
			fResFile->Release();
			fResFile = NULL;
			fResFileChecked = false;
		}
	}
}


#if VERSIONMAC
CFBundleRef VLibrary::MAC_GetBundleRef() const
{
	return fImpl.MAC_GetBundleRef();
}
#elif VERSIONWIN
HINSTANCE VLibrary::WIN_GetInstance() const
{
	return fImpl.WIN_GetInstance();
}
#endif

void* VLibrary::GetProcAddress(const VString& inProcName)
{
	VTaskLock	locker(&fCriticalSection);

	void*	ptProc = NULL;

	if (fIsLoaded)
		ptProc = fImpl.GetProcAddress(inProcName);

	return ptProc;
}


bool VLibrary::IsSameFile(const VFilePath &inLibFile) const
{
	VTaskLock	locker(&fCriticalSection);

	bool result = (fFilePath.GetPath().CompareToString(inLibFile.GetPath()) == CR_EQUAL);
	
	return result;
}


VFolder* VLibrary::RetainFolder( BundleFolderKind inKind) const
{
	VTaskLock	locker(&fCriticalSection);

	VFolder *result;
	if ( fIsLoaded )
		result = fImpl.RetainFolder( fFilePath, inKind);
	else
		result = NULL;

	return result;
}


VFile* VLibrary::RetainExecutableFile() const
{
	VTaskLock	locker(&fCriticalSection);

	return (fIsLoaded ? fImpl.RetainExecutableFile(fFilePath) : NULL);
}


VResourceFile* VLibrary::RetainResourceFile()
{
	VTaskLock	locker(&fCriticalSection);

	if (fIsLoaded)
	{
		if (!fResFileChecked)
		{
			fResFile = fImpl.RetainResourceFile(fFilePath);
			fResFileChecked = true;
		}
		
		if (fResFile != NULL)
			fResFile->Retain();
	}
	
	return fResFile;
}

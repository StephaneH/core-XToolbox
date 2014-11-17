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
#include "VLeaks.h"


#if VERSIONWIN
#include <tchar.h>
#endif


VLeaksChecker::VLeaksChecker()
{
	fLeaksCount = 0;
	fIsOn = false;
	fStackDepthToSkip = 4;
}


FILE* VLeaksChecker::OpenDumpFile(const char* inManagerName)
{
	FILE *file = NULL;
	char fullPath[4096L];
	fullPath[0] = 0;

	// Get a path to the exe.

#if VERSIONMAC
	CFURLRef urlBundle = ::CFBundleCopyBundleURL(::CFBundleGetMainBundle());
	CFURLRef urlBundleNoExtension = ::CFURLCreateCopyDeletingPathExtension(NULL, urlBundle);
	::CFURLGetFileSystemRepresentation(urlBundleNoExtension, true, (UInt8 *) &fullPath[0], sizeof(fullPath));
	::CFRelease(urlBundleNoExtension);
	::CFRelease(urlBundle);
/*
	FSSpec spec;
	ProcessInfoRec info;
	::memset(&info, 0, sizeof(info));
	info.processInfoLength = sizeof(info);
	info.processName       = NULL;
	info.processAppSpec    = &spec;

	ProcessSerialNumber	psn = { 0, kCurrentProcess };
	OSStatus err = ::GetProcessInformation(&psn, &info);
	if (err == noErr)
		err = MAC_FSSpecToStdLibFullPath(spec, fullPath, sizeof(fullPath));
*/
#elif VERSIONWIN
	uLONG nbChars = ::GetModuleFileNameA(GetModuleHandle(NULL), fullPath, sizeof(fullPath));
	// remove extension
	for(--nbChars; nbChars >= 0; --nbChars)
	{
		if (fullPath[nbChars] == '\\')
		{
			// no extension, bail out
			break;
		}
		else if (fullPath[nbChars] == '.')
		{
			// remove extension
			fullPath[nbChars] = 0;
			break;
		}
	}
#endif

	if (fullPath[0] > 0)
	{
		strcat(fullPath, ".");
		strcat(fullPath, inManagerName);
		strcat(fullPath, ".leaks");
		file = fopen(fullPath, "w");
	}

	if (file != NULL)
	{
		char s[200];
		time_t now = time(NULL);
		#if !VERSIONWIN
		{
			struct tm nowtm;
			localtime_r(&now, &nowtm);
			strftime(s, sizeof(s),"%A, %B %d, %Y %I:%M:%S %p\n", &nowtm);
		}
		#else
		{
			struct tm* nowtm = localtime(&now);
			strftime(s, sizeof(s),"%A, %B %d, %Y %I:%M:%S %p\n", nowtm);
		}
		#endif
		fprintf(file, "%s", (const char*) s);

		if (fLeaksCount == 0)
		{
			fprintf(file, "No memory leaks.\n");
		}
		else
		{
			if (fLeaksCount == 1)
				fprintf(file, "You have one leak:\n\n");
			else
				fprintf(file, "You have %i leaks:\n\n", (int) fLeaksCount);
		}
	}

	return file;
}


bool VLeaksChecker::SetLeaksCheck(bool inSetIt)
{
	bool old = fIsOn;
	fIsOn = inSetIt;
	return old;
}


VLeaksChecker::~VLeaksChecker()
{
}


void VLeaksChecker::UnregisterBlock(VStackCrawl& ioInfo)
{
	if (ioInfo.IsFramesLoaded())
		--fLeaksCount;
	
	ioInfo.UnloadFrames();
}


void VLeaksChecker::RegisterBlock(VStackCrawl& ioInfo)
{
	if (fIsOn)
	{
		ioInfo.LoadFrames(fStackDepthToSkip, kMaxStackCrawlDepth);

		if (ioInfo.IsFramesLoaded())
			++fLeaksCount;
	}
}


void VLeaksChecker::SetStackFramesToSkip(sLONG inNbFrames)
{
	fStackDepthToSkip = inNbFrames;
}


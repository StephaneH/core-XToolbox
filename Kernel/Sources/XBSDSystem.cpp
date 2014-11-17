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

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "VFilePath.h"
#include "XBSDSystem.h"


/*!
** \brief Check if un input file exists and is executable
**
** Return non-zero if the name is an executable file, and
** zero if it is not executable, or if it does not exist.
*/
static bool IsExecutable(const char* inFilename)
{
	struct stat statinfo;
	if (stat(inFilename, &statinfo) < 0)
		return false;

	if (!S_ISREG(statinfo.st_mode))
		return false;
	if (statinfo.st_uid == geteuid())
		return (statinfo.st_mode & S_IXUSR);
	if (statinfo.st_gid == getegid())
		return (statinfo.st_mode & S_IXGRP);

	return (statinfo.st_mode & S_IXOTH);
}


/*!
** \brief Find executable by searching the PATH environment variable.
*/
static bool Which(char* pth, const char* exe)
{
	if (strchr(exe, '/') != NULL)
	{
		if (realpath(exe, pth) == NULL)
			return false;
		return IsExecutable(pth);
	}

	const char* searchpath = getenv("PATH");
	if (searchpath == NULL || *searchpath == '\0')
		return false;

	const char* beg = searchpath;
	bool stop = false;
	do
	{
		const char* end = strchr(beg, ':');
		int len = 0;
		if (end == NULL)
		{
			stop = true;
			strncpy(pth, beg, PATH_MAX);
			len = strlen(pth);
		}
		else
		{
			strncpy(pth, beg, end - beg);
			len = end - beg;
			pth[len] = '\0';
		}

		if (pth[len - 1] != '/')
			strncat(pth, "/", 1);

		strncat(pth, exe, PATH_MAX - len);
		if (IsExecutable(pth))
			return true;

		if (!stop)
			beg = end + 1;
	}
	while (!stop);

	return false;
}


bool XBSDSystem::SearchExecutablePath(const VString& inExecutableName, VFilePath& outPath)
{
	// the executable name in UTF_8
	XBOX::StStringConverter<char> utf8exename(inExecutableName, VTC_UTF_8);
	// the real executable path
	// since PATH_MAX might be high, using malloc to reduce issues with stack sizes
	char* pth = new char[PATH_MAX + 1];
	// return value
	bool success = false;

	if (Which(pth, utf8exename.GetCPointer()))
	{
		// something has been found
		VString vstr;
		vstr.FromBlock(pth, (VSize) strlen(pth), VTC_UTF_8);
		outPath.FromFullPath(vstr, FPS_POSIX);
		success = true;
	}
	else
		outPath.Clear(); // reset just in case

	delete[] pth;
	return success;
}


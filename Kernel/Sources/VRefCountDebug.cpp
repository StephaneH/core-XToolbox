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
#include "VRefCountDebug.h"

VRefCountDebug VRefCountDebug::sRefCountDebug;

VRefCountDebug::VRefCountDebug()
{
	fDebugInfoCount = 0;
	fDebugInfosMutex = new VCriticalSection();
}

VRefCountDebug::~VRefCountDebug()
{
	delete fDebugInfosMutex;
}

void VRefCountDebug::RetainInfo(const void* object, const char* info)
{
	while (!fDebugInfosMutex->TryToLock())
	{
		VTask::GetCurrent()->Sleep(2);
	}
	fDebugInfoCount++;
	if (fDebugInfoCount == 1)
	{
		VString sInfo(info);
		/*
		VString s;
		s.FromLong((uLONG)object);
		s+=L" + ";
		s+=sInfo;
		s+="\n";
		DebugMsg(s);
		*/

		((fDebugInfos[object])[sInfo])++;
	}
	fDebugInfoCount--;
	fDebugInfosMutex->Unlock();
}


Boolean VRefCountDebug::ReleaseInfo(const void* object, const char* info)
{
	Boolean res = true;
	while (!fDebugInfosMutex->TryToLock())
	{
		VTask::GetCurrent()->Sleep(2);
	}
	fDebugInfoCount++;
	if (fDebugInfoCount == 1)
	{
		VString sInfo(info);
		/*
		VString s;
		s.FromLong((uLONG)object);
		s+=L" - ";
		s+=sInfo;
		s+="\n";
		DebugMsg(s);
		*/

		RefCountDebugInfoMap::iterator found = fDebugInfos.find(object);
		if (found == fDebugInfos.end())
			res = false;
		else
		{
			{
				DebugInfoMap::iterator found2 = found->second.find(sInfo);
				if (found2 == found->second.end())
					res = false;
				else
				{
					if (found2->second == 0)
					{
						assert(false);
						res = false;
					}
					else
						found2->second--;
					if (found2->second == 0)
						found->second.erase(found2);
				}
			}
			if (found->second.size() == 0)
			{
				fDebugInfos.erase(found);
			}
		}
	}
	fDebugInfoCount--;
	fDebugInfosMutex->Unlock();
	return res;
}


Boolean VRefCountDebug::ReleaseInfo(const void* object)
{
	Boolean res = false;
	while (!fDebugInfosMutex->TryToLock())
	{
		VTask::GetCurrent()->Sleep(2);
	}
	RefCountDebugInfoMap::iterator found = fDebugInfos.find(object);
	if (found == fDebugInfos.end())
		res = true;
	else
	{
		fDebugInfos.erase(found);
		res = false;
	}
	fDebugInfosMutex->Unlock();
	return res;
}

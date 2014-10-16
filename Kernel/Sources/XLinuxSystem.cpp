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
#include "XLinuxSystem.h"
#include "XLinuxProcParsers.h"
#include "VString.h"

#include <time.h>
#include <unistd.h>
#include <sys/mman.h>

#include <sys/sysinfo.h>
#include "bsd/stdlib.h"


#ifndef LOGIN_NAME_MAX
	#define LOGIN_NAME_MAX	9
#endif


#ifndef HOST_NAME_MAX
	#define HOST_NAME_MAX	255
#endif


static uLONG8 gBaseTime=0;
static uLONG8 gBaseTicks=0;

void XLinuxSystem::InitRefTime()
{
	if(gBaseTicks==0 || gBaseTime==0)
	{
		struct timespec ts;
		memset(&ts, 0, sizeof(ts));

		int res=clock_gettime(CLOCK_MONOTONIC, &ts);
		xbox_assert(res==0);

		gBaseTicks=ts.tv_sec*60+(ts.tv_nsec*60)/1000000000;

		memset(&ts, 0, sizeof(ts));

		res=clock_gettime(CLOCK_REALTIME, &ts);
		xbox_assert(res==0);

		gBaseTime=ts.tv_sec*1000+ts.tv_nsec/1000000;
	}
}


void XLinuxSystem::Beep()
{
	printf("\a");
}


void XLinuxSystem::GetOSVersionString(VString &outStr)
{
	outStr="Linux version unknown";	// Postponed Linux Implementation !
}


sLONG XLinuxSystem::Random(bool inComputeFast)
{
    // Same code as mac version right now, but we may use /dev/urandom in the future

	sLONG longRandom;

    if (inComputeFast)
        longRandom = random();
    else
        longRandom = arc4random() % ((unsigned)RAND_MAX + 1);

	return longRandom;
}


bool XLinuxSystem::IsSystemVersionOrAbove(SystemVersion inSystemVersion)
{
    // Used in VSystem::IsSystemVersionOrAbove
    return true;	// Postponed Linux Implementation !
}


bool XLinuxSystem::DisplayNotification( const VString& inTitle, const VString& inMessage, EDisplayNotificationOptions inOptions, ENotificationResponse *outResponse)
{
    // Used in VSystem::DisplayNotification
    return false;	// POstponed Linux Implementation !
}


VSize XLinuxSystem::GetVMPageSize()
{
    return getpagesize();
}


void* XLinuxSystem::VirtualAlloc(VSize inNbBytes, const void *inHintAddress)
{
    int prot=PROT_READ|PROT_WRITE;
    int flags=(inHintAddress==NULL ? MAP_PRIVATE|MAP_ANON : MAP_PRIVATE|MAP_ANON|MAP_FIXED);

    void* ptr=mmap(const_cast<void*>(inHintAddress), inNbBytes, prot, flags, 0, 0);

    return (ptr!=MAP_FAILED ? ptr : NULL);
}


void* XLinuxSystem::VirtualAllocPhysicalMemory(VSize inNbBytes, const void *inHintAddress, bool *outCouldLock)
{
	void* addr=VirtualAlloc(inNbBytes, inHintAddress);

	if(addr==NULL)
		return NULL;

	int res=mlock(addr, inNbBytes);	//the lock is removed by munmap() in VirtualFree()

	if(outCouldLock!=NULL)
		*outCouldLock=(res==0) ? true : false;

	return addr;
}


void XLinuxSystem::VirtualFree(void* inBlock, VSize inNbBytes, bool inPhysicalMemory)
{
    int res = munmap(inBlock, inNbBytes);
    xbox_assert(res==0);    //From OS X implementation
	(void) res;
}


void XLinuxSystem::LocalToUTCTime(sWORD ioVals[7])
{
		struct tm tmLocal;
		memset(&tmLocal, 0, sizeof(tmLocal));

		tmLocal.tm_sec=ioVals[5];
		tmLocal.tm_min=ioVals[4];
		tmLocal.tm_hour=ioVals[3];
		tmLocal.tm_mday=ioVals[2];
		tmLocal.tm_mon=ioVals[1] - 1;		// YT - 31-Mar-2011 - tm_mon is in range [0-11]
		tmLocal.tm_year=ioVals[0] - 1900;	// & tm_year is Year-1900 see struct tm declaration

		time_t timeTmp=mktime(&tmLocal);
		xbox_assert(timeTmp!=(time_t)-1);

		struct tm tmUTC;
		memset(&tmUTC, 0, sizeof(tmUTC));

		struct tm* res = gmtime_r(&timeTmp, &tmUTC);
		xbox_assert(res != NULL);
		(void) res;

		ioVals[5]=tmUTC.tm_sec;
		ioVals[4]=tmUTC.tm_min;
		ioVals[3]=tmUTC.tm_hour;
		ioVals[2]=tmUTC.tm_mday;
		ioVals[1]=tmUTC.tm_mon + 1;		// YT - 31-Mar-2011 - tm_mon is in range [0-11]
		ioVals[0]=tmUTC.tm_year + 1900;	// & tm_year is Year-1900 see struct tm declaration.
}


void XLinuxSystem::UTCToLocalTime(sWORD ioVals[7])
{
		struct tm tmUTC;
		memset(&tmUTC, 0, sizeof(tmUTC));

		tmUTC.tm_sec=ioVals[5];
		tmUTC.tm_min=ioVals[4];
		tmUTC.tm_hour=ioVals[3];
		tmUTC.tm_mday=ioVals[2];
		tmUTC.tm_mon=ioVals[1] - 1;		// YT - 31-Mar-2011 - tm_mon is in range [0-11]
		tmUTC.tm_year=ioVals[0] - 1900;	// & tm_year is Year-1900 see struct tm declaration.

		tmUTC.tm_isdst=0;				// jmo - Stress that daylight saving time is not
		                 				//       in effect if dealing with UTC time !
		
		time_t timeTmp=timegm(&tmUTC);	// jmo - Fix WAK0084391
		xbox_assert(timeTmp!=(time_t)-1);

		struct tm tmLocal;
		memset(&tmLocal, 0, sizeof(tmLocal));

		struct tm* res = localtime_r(&timeTmp, &tmLocal);
		xbox_assert(res != NULL);
		(void) res;

		ioVals[5]=tmLocal.tm_sec;
		ioVals[4]=tmLocal.tm_min;
		ioVals[3]=tmLocal.tm_hour;
		ioVals[2]=tmLocal.tm_mday;
		ioVals[1]=tmLocal.tm_mon + 1;		// YT - 31-Mar-2011 - tm_mon is in range [0-11]
		ioVals[0]=tmLocal.tm_year + 1900;	// & tm_year is Year-1900 see struct tm declaration.
}


void XLinuxSystem::GetSystemUTCTime(sWORD outVals[7])
{
	struct timespec ts;
	memset(&ts, 0, sizeof(ts));

	int res = clock_gettime(CLOCK_REALTIME, &ts);
	xbox_assert(res == 0);
	(void) res;

	struct tm tmUTC;
	memset(&tmUTC, 0, sizeof(tmUTC));

	struct tm* tmRes = gmtime_r(&ts.tv_sec, &tmUTC);
	xbox_assert(tmRes!=NULL);
	(void) tmRes;

	// WAK0073286: JavaScript runtime (VJSWorker) needs millisecond precision.

	outVals[6]=ts.tv_nsec / 1000000;
	outVals[5]=tmUTC.tm_sec;
	outVals[4]=tmUTC.tm_min;
	outVals[3]=tmUTC.tm_hour;
	outVals[2]=tmUTC.tm_mday;
	outVals[1]=tmUTC.tm_mon + 1;		// YT - 31-Mar-2011 - tm_mon is in range [0-11]
	outVals[0]=tmUTC.tm_year + 1900;	// & tm_year is Year-1900 see struct tm declaration.
}


void XLinuxSystem::GetSystemLocalTime(sWORD outVals[7])
{
	struct timespec ts;
	memset(&ts, 0, sizeof(ts));

	int res = clock_gettime(CLOCK_REALTIME, &ts);
	xbox_assert(res == 0);
	(void) res;

	struct tm tmLocal;
	memset(&tmLocal, 0, sizeof(tmLocal));

	struct tm* tmRes = localtime_r(&ts.tv_sec, &tmLocal);
	xbox_assert(tmRes != NULL);
	(void) tmRes;

	// See comment for GetSystemUTCTime().

	outVals[6]=ts.tv_nsec / 1000000;
	outVals[5]=tmLocal.tm_sec;
	outVals[4]=tmLocal.tm_min;
	outVals[3]=tmLocal.tm_hour;
	outVals[2]=tmLocal.tm_mday;
	outVals[1]=tmLocal.tm_mon + 1;		// YT - 31-Mar-2011 - tm_mon is in range [0-11]
	outVals[0]=tmLocal.tm_year + 1900;	// & tm_year is Year-1900 see struct tm declaration.
}

void XLinuxSystem::UpdateGMTOffset(sLONG* outOffset, sLONG* outOffsetWithDayLight)
{
	time_t timeTmp=time(NULL);

	struct tm tmLocal;
	memset(&tmLocal, 0, sizeof(tmLocal));

	struct tm* tmRes=localtime_r(&timeTmp, &tmLocal);
	xbox_assert(tmRes!=NULL);

	if(tmRes!=NULL && outOffset!=NULL)
	{
		*outOffset=tmRes->tm_gmtoff;
	}

	if(tmRes!=NULL && outOffsetWithDayLight!=NULL)
	{
		*outOffsetWithDayLight=(tmRes->tm_isdst!=0) ? tmRes->tm_gmtoff-3600 : tmRes->tm_gmtoff-3600;
	}
}


uLONG XLinuxSystem::GetCurrentTicks()
{
	struct timespec ts;
	memset(&ts, 0, sizeof(ts));

	int res = clock_gettime(CLOCK_MONOTONIC, &ts);
	xbox_assert(res == 0);
	(void) res;

	//(TickCount : a tick is approximately 1/60 of a second)
	uLONG8 ticks = ts.tv_sec * 60 + (ts.tv_nsec * 60) / 1000000000;

	return (uLONG) ticks - gBaseTicks;
}

uLONG XLinuxSystem::GetCurrentTimeStamp()
{
	struct timespec ts;
	memset(&ts, 0, sizeof(ts));

	int res = clock_gettime(CLOCK_REALTIME, &ts);
	xbox_assert(res == 0);
	(void) res;

	uLONG8 time=ts.tv_sec*1000+ts.tv_nsec/1000000;

	return time-gBaseTime;
}


void XLinuxSystem::GetProfilingCounter(sLONG8& outVal)
{
	struct timespec ts;
	memset(&ts, 0, sizeof(ts));

	int res = clock_gettime(CLOCK_REALTIME, &ts);
	xbox_assert(res == 0);
	(void) res;

	outVal=ts.tv_sec*1000000000+ts.tv_nsec;
}


sLONG8 XLinuxSystem::GetProfilingFrequency()
{
	return 1000000000;

}


void XLinuxSystem::Delay(sLONG inMilliseconds)
{
	//You should not call this function with high values (>1s)
	xbox_assert(inMilliseconds<=1000);

	struct timespec ts;

	ts.tv_sec=inMilliseconds/1000;

	inMilliseconds-=ts.tv_sec*1000;

	ts.tv_nsec=inMilliseconds*1000000;

	while (nanosleep(&ts, &ts) == -1)
	{
		if (errno!=EINTR)
		{
			xbox_assert(0);	//an error occured
		}
	}
}


sLONG XLinuxSystem::GetNumberOfProcessors()
{
	// the total number of logicial cpus
	sLONG count = 0;

	FILE* fd = ::fopen("/proc/cpuinfo", "r");
	if (fd)
	{
		// the substring to find within /proc/cpuinfo
		static const char* const strToFind = "processor\t";
		// the length of the string to find
		size_t len = ::strlen(strToFind);
		// temporary string for the content of the current line
		char line[256];

		// reading the file line by line (which might be truncated here)
		while (::fgets(line, sizeof(line), fd))
		{
			// we are looking for all occurences of 'processor'
			// (+tab to avoid potential interferences)
			if (0 == ::strncmp(line, strToFind, len))
				++count;
		}

		::fclose(fd);
	}

	// ensure we have at least 1 cpu, even if we had encountered errors
	return (count != 0) ? count : 1;
}


bool XLinuxSystem::GetProcessorsPercentageUse(Real& outPercentageUse)
{
	return false;	// Postponed Linux Implementation !
}


void XLinuxSystem::GetProcessorTypeString(VString& outProcessorType, bool inCleanupSpaces)
{
    outProcessorType="-";	// Postponed Linux Implementation !
}


Real XLinuxSystem::GetApplicationProcessorsPercentageUse()
{
	return -1.0;	// Postponed Linux Implementation !
}


sLONG8 XLinuxSystem::GetPhysicalMemSize()
{
	struct sysinfo infos;
	memset(&infos, 0, sizeof(infos));

	int res = sysinfo(&infos);
	xbox_assert(res == 0);
	(void) res;

	return infos.totalram*infos.mem_unit;
}


sLONG8 XLinuxSystem::GetPhysicalFreeMemSize()
{
	struct sysinfo infos;
	memset(&infos, 0, sizeof(infos));

	int res = sysinfo(&infos);
	xbox_assert(res == 0);
	(void) res;

	return infos.freeram*infos.mem_unit;
}


sLONG8 XLinuxSystem::GetApplicationPhysicalMemSize()
{
	StatmParser statm;

	VError verr = statm.Init();
	xbox_assert(verr == VE_OK);
	(void) verr;

	return statm.GetData();
}


VSize XLinuxSystem::GetApplicationVirtualMemSize()
{
	StatParser stat;

	VError verr = stat.Init();
	xbox_assert(verr == VE_OK);
	(void) verr;

	return stat.GetVSize();
}


VSize XLinuxSystem::VirtualMemoryUsedSize()
{
	return 0;	// Need a Linux Implementation !
}


bool XLinuxSystem::GetInOutNetworkStats(std::vector<Real>& outNetworkStats)
{
	return false;	// Postponed Linux Implementation !
}


bool XLinuxSystem::AllowedToGetSystemInfo()
{
	return false;	// Postponed Linux Implementation !
}


void XLinuxSystem::GetLoginUserName(VString& outName)
{
	char name[LOGIN_NAME_MAX+1];
	name[0]=0;

	int res = getlogin_r(name, sizeof(name));
	xbox_assert(res == 0);
	(void) res;

	outName.FromBlock(name, strlen(name), VTC_UTF_8);
}


void XLinuxSystem::GetHostName(VString& outName)
{
	char name[HOST_NAME_MAX+1];
	name[0]=0;

	int res = gethostname(name, sizeof(name));
	xbox_assert(res == 0);
	(void) res;

	outName.FromBlock(name, strlen(name), VTC_UTF_8);
}


void XLinuxSystem::GetProcessList(std::vector<pid> &outPIDs)
{
	// Postponed Linux Implementation !
}


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
#include "XBSDSystem.h"
#include "XLinuxProcParsers.h"
#include "VString.h"
#include "VFilePath.h"

#include <time.h>
#include <unistd.h>
#include <sys/mman.h>

#include <sys/sysinfo.h>
#include "bsd/stdlib.h"
#include <limits.h> // for HOST_NAME_MAX and LOGIN_NAME_MAX

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>




/*!
** \macro LOGIN_NAME_MAX
** \brief Maximum length for an unix username
**
** \note The current limit is 32 characters (according to useradd man page)
**   But LOGIN_NAME_MAX is defined as 256 in limits.h
*/
#ifndef LOGIN_NAME_MAX
	#define LOGIN_NAME_MAX	256
#endif


#ifndef HOST_NAME_MAX
	#define HOST_NAME_MAX	255
#endif



BEGIN_TOOLBOX_NAMESPACE

namespace XLinuxSystem
{


	namespace // anonymous namespace
	{

		//! The UNIX timestamp at startup
		static uLONG8 gBaseTime  = 0;
		//! The number of clock ticks at startup
		static uLONG8 gBaseTicks = 0;


	} // anonymous namespace







	void InitRefTime()
	{
		if (0 == gBaseTicks || 0 == gBaseTime)
		{
			struct timespec ts;
			memset(&ts, 0, sizeof(ts));

			int res = clock_gettime(CLOCK_MONOTONIC, &ts);
			xbox_assert(0 == res);
			gBaseTicks = ts.tv_sec * 60 + (ts.tv_nsec * 60) / 1000000000;

			memset(&ts, 0, sizeof(ts));

			res = clock_gettime(CLOCK_REALTIME, &ts);
			xbox_assert(0 == res);
			gBaseTime = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
		}
	}


	void Beep()
	{
		write(STDOUT_FILENO, "\a", 1);
	}


	void GetOSVersionString(VString &outStr)
	{
		xbox_assert(false && "missing implementation for GetOSVersionString");
		outStr = CVSTR("Linux version unknown"); // Postponed Linux Implementation !
	}


	sLONG Random(bool inComputeFast)
	{
		// Same code as mac version right now, but we may use /dev/urandom in the future
		return (inComputeFast)
			? (sLONG) (random())
			: (sLONG) (arc4random() & ((unsigned)RAND_MAX + 1));
	}


	bool IsSystemVersionOrAbove(SystemVersion /*inSystemVersion*/)
	{
		// Used in VSystem::IsSystemVersionOrAbove
		xbox_assert(false && "missing implementation for IsSystemVersionOrAbove");
		return true;	// Postponed Linux Implementation !
	}


	bool DisplayNotification( const VString& /*inTitle*/, const VString& /*inMessage*/, EDisplayNotificationOptions /*inOptions*/, ENotificationResponse *outResponse)
	{
		// Used in VSystem::DisplayNotification
		if (outResponse) // temporary fix for linux
		{
			// xbox_assert(false && "response for 'VSystem::DisplayNotification' not implemented on linux");
			*outResponse = ERN_Cancel;
		}
		return false;	// POstponed Linux Implementation !
	}


	VSize GetVMPageSize()
	{
		return getpagesize();
	}


	void* VirtualAlloc(VSize inNbBytes, const void *inHintAddress)
	{
		int prot=PROT_READ|PROT_WRITE;
		int flags=(inHintAddress==NULL ? MAP_PRIVATE|MAP_ANON : MAP_PRIVATE|MAP_ANON|MAP_FIXED);

		void* ptr = mmap(const_cast<void*>(inHintAddress), inNbBytes, prot, flags, 0, 0);

		return (ptr!=MAP_FAILED ? ptr : NULL);
	}


	void* VirtualAllocPhysicalMemory(VSize inNbBytes, const void *inHintAddress, bool *outCouldLock)
	{
		void* addr=VirtualAlloc(inNbBytes, inHintAddress);

		if(addr==NULL)
			return NULL;

		int res=mlock(addr, inNbBytes);	//the lock is removed by munmap() in VirtualFree()

		if(outCouldLock!=NULL)
			*outCouldLock=(res==0) ? true : false;

		return addr;
	}


	void VirtualFree(void* inBlock, VSize inNbBytes, bool /*inPhysicalMemory*/)
	{
		int res = munmap(inBlock, inNbBytes);
		xbox_assert(res==0);    //From OS X implementation
		(void) res;
	}


	void LocalToUTCTime(sWORD ioVals[7])
	{
		struct tm tmLocal;
		memset(&tmLocal, 0, sizeof(tmLocal));

		tmLocal.tm_sec  = ioVals[5];
		tmLocal.tm_min  = ioVals[4];
		tmLocal.tm_hour = ioVals[3];
		tmLocal.tm_mday = ioVals[2];
		tmLocal.tm_mon  = ioVals[1] - 1;     // YT - 31-Mar-2011 - tm_mon is in range [0-11]
		tmLocal.tm_year = ioVals[0] - 1900;  // & tm_year is Year-1900 see struct tm declaration

		time_t timeTmp=mktime(&tmLocal);
		xbox_assert(timeTmp!=(time_t)-1);

		struct tm tmUTC;
		memset(&tmUTC, 0, sizeof(tmUTC));

		struct tm* res = gmtime_r(&timeTmp, &tmUTC);
		xbox_assert(res != NULL);
		(void) res;

		ioVals[5] = tmUTC.tm_sec;
		ioVals[4] = tmUTC.tm_min;
		ioVals[3] = tmUTC.tm_hour;
		ioVals[2] = tmUTC.tm_mday;
		ioVals[1] = tmUTC.tm_mon + 1;      // YT - 31-Mar-2011 - tm_mon is in range [0-11]
		ioVals[0] = tmUTC.tm_year + 1900;  // & tm_year is Year-1900 see struct tm declaration.
	}


	void UTCToLocalTime(sWORD ioVals[7])
	{
		struct tm tmUTC;
		memset(&tmUTC, 0, sizeof(tmUTC));

		tmUTC.tm_sec  = ioVals[5];
		tmUTC.tm_min  = ioVals[4];
		tmUTC.tm_hour = ioVals[3];
		tmUTC.tm_mday = ioVals[2];
		tmUTC.tm_mon  = ioVals[1] - 1;      // YT - 31-Mar-2011 - tm_mon is in range [0-11]
		tmUTC.tm_year = ioVals[0] - 1900;   // & tm_year is Year-1900 see struct tm declaration.

		tmUTC.tm_isdst=0;  // jmo - Stress that daylight saving time is not
		//       in effect if dealing with UTC time !

		time_t timeTmp=timegm(&tmUTC);	// jmo - Fix WAK0084391
		xbox_assert(timeTmp!=(time_t)-1);

		struct tm tmLocal;
		memset(&tmLocal, 0, sizeof(tmLocal));

		struct tm* res = localtime_r(&timeTmp, &tmLocal);
		xbox_assert(res != NULL);
		(void) res;

		ioVals[5] = tmLocal.tm_sec;
		ioVals[4] = tmLocal.tm_min;
		ioVals[3] = tmLocal.tm_hour;
		ioVals[2] = tmLocal.tm_mday;
		ioVals[1] = tmLocal.tm_mon + 1;      // YT - 31-Mar-2011 - tm_mon is in range [0-11]
		ioVals[0] = tmLocal.tm_year + 1900;  // & tm_year is Year-1900 see struct tm declaration.
	}


	void GetSystemUTCTime(sWORD outVals[7])
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

		outVals[6] = ts.tv_nsec / 1000000;
		outVals[5] = tmUTC.tm_sec;
		outVals[4] = tmUTC.tm_min;
		outVals[3] = tmUTC.tm_hour;
		outVals[2] = tmUTC.tm_mday;
		outVals[1] = tmUTC.tm_mon + 1;		// YT - 31-Mar-2011 - tm_mon is in range [0-11]
		outVals[0] = tmUTC.tm_year + 1900;	// & tm_year is Year-1900 see struct tm declaration.
	}


	void GetSystemLocalTime(sWORD outVals[7])
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

		outVals[6] = ts.tv_nsec / 1000000;
		outVals[5] = tmLocal.tm_sec;
		outVals[4] = tmLocal.tm_min;
		outVals[3] = tmLocal.tm_hour;
		outVals[2] = tmLocal.tm_mday;
		outVals[1] = tmLocal.tm_mon + 1;        // YT - 31-Mar-2011 - tm_mon is in range [0-11]
		outVals[0] = tmLocal.tm_year + 1900;    // & tm_year is Year-1900 see struct tm declaration.
	}


	void UpdateGMTOffset(sLONG* outOffset, sLONG* outOffsetWithDayLight)
	{
		time_t timeTmp=time(NULL);

		struct tm tmLocal;
		memset(&tmLocal, 0, sizeof(tmLocal));

		struct tm* tmRes=localtime_r(&timeTmp, &tmLocal);
		xbox_assert(tmRes!=NULL);

		if (tmRes)
		{
			if (outOffset!=NULL)
				*outOffset = tmRes->tm_gmtoff;

			if (outOffsetWithDayLight!=NULL)
				*outOffsetWithDayLight=(tmRes->tm_isdst!=0) ? tmRes->tm_gmtoff-3600 : tmRes->tm_gmtoff-3600;
		}
	}


	uLONG GetCurrentTicks()
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


	uLONG GetCurrentTimeStamp()
	{
		struct timespec ts;
		memset(&ts, 0, sizeof(ts));

		int res = clock_gettime(CLOCK_REALTIME, &ts);
		xbox_assert(res == 0);
		(void) res;

		uLONG8 time = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
		return (uLONG) time - gBaseTime;
	}


	void GetProfilingCounter(sLONG8& outVal)
	{
		struct timespec ts;
		memset(&ts, 0, sizeof(ts));

		int res = clock_gettime(CLOCK_REALTIME, &ts);
		xbox_assert(res == 0);
		(void) res;

		outVal = ts.tv_sec * 1000000000 + ts.tv_nsec;
	}


	void Delay(sLONG inMilliseconds)
	{
		//You should not call this function with high values (>1s)
		xbox_assert(inMilliseconds<=1000);

		struct timespec ts;
		ts.tv_sec       = inMilliseconds / 1000;
		inMilliseconds -= ts.tv_sec * 1000;
		ts.tv_nsec      = inMilliseconds * 1000000;

		while (nanosleep(&ts, &ts) == -1)
		{
			if (errno!=EINTR)
			{
				xbox_assert(0);	//an error occured
			}
		}
	}


	sLONG GetNumberOfProcessors()
	{
		// the total number of logicial cpus
		uLONG count = 0;

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
		return (count > 0) ? count : 1;
	}


	bool GetProcessorsPercentageUse(Real& /*outPercentageUse*/)
	{
		return false;	// Postponed Linux Implementation !
	}


	void GetProcessorTypeString(VString& outProcessorType, bool /*inCleanupSpaces*/)
	{
		outProcessorType = CVSTR("-"); // Postponed Linux Implementation !
	}


	Real GetApplicationProcessorsPercentageUse()
	{
		return -1.0; // Postponed Linux Implementation !
	}


	sLONG8 GetPhysicalMemSize()
	{
		struct sysinfo infos;
		memset(&infos, 0, sizeof(infos));

		int res = sysinfo(&infos);
		xbox_assert(res == 0);
		(void) res;

		return infos.totalram*infos.mem_unit;
	}


	sLONG8 GetPhysicalFreeMemSize()
	{
		struct sysinfo infos;
		memset(&infos, 0, sizeof(infos));

		int res = sysinfo(&infos);
		xbox_assert(res == 0);
		(void) res;

		return infos.freeram*infos.mem_unit;
	}


	sLONG8 GetApplicationPhysicalMemSize()
	{
		StatmParser statm;

		VError verr = statm.Init();
		xbox_assert(verr == VE_OK);
		(void) verr;

		return statm.GetData();
	}


	VSize GetApplicationVirtualMemSize()
	{
		StatParser stat;

		VError verr = stat.Init();
		xbox_assert(verr == VE_OK);
		(void) verr;

		return stat.GetVSize();
	}


	VSize VirtualMemoryUsedSize()
	{
		return 0;	// Need a Linux Implementation !
	}


	bool GetInOutNetworkStats(std::vector<Real>& /*outNetworkStats*/)
	{
		return false;	// Postponed Linux Implementation !
	}


	bool AllowedToGetSystemInfo()
	{
		return false;	// Postponed Linux Implementation !
	}


	void GetLoginUserName(VString& outName)
	{
		char name[LOGIN_NAME_MAX];
		int res = ::getlogin_r(name, LOGIN_NAME_MAX);
		xbox_assert(res == 0 && "failed to call getlogin_r");

		if (res == 0)
			outName.FromBlock(name, ::strlen(name), VTC_UTF_8);
		else
			outName.Clear();
	}


	void GetHostName(VString& outName)
	{
		char name[HOST_NAME_MAX];
		int res = ::gethostname(name, HOST_NAME_MAX);
		xbox_assert(res == 0);

		if (res == 0)
		{
			// making sure that the buffer is zero-terminated:
			// POSIX.1-2001 says that if such truncation occurs, then it is unspecified
			// whether the returned buffer includes a terminating null byte
			name[HOST_NAME_MAX - 1] = '\0';
			// copy host name
			outName.FromBlock(name, ::strlen(name), VTC_UTF_8);
		}
		else
			outName.Clear();
	}


	void GetProcessList(std::vector<pid>& outPIDs)
	{
		outPIDs.clear();
		outPIDs.reserve(256); // arbitrary, but it seems a good value

		// try to open /proc for reading all process ids
		DIR* dirproc = opendir("/proc");
		if (dirproc)
		{
			struct dirent* entry = NULL;
			while ((entry = readdir(dirproc)))
			{
				if (entry->d_type == DT_DIR) // only directories
				{
					// trying to find all folder names which are numeric
					const char* name = entry->d_name;
					if (name && *name >= '0' && *name <= '9')
					{
						int pid = atoi(name);
						if (pid > 0)
							outPIDs.push_back(pid);
					}
				}
			}

			closedir(dirproc);
		}
	}




	namespace // anonymous
	{

		static inline char* GetProcessNameFromProcCmdLine(const char* inFilename, uLONG& outLength)
		{
			char* result = NULL;
			FILE* fd = ::fopen(inFilename, "r");
			if (fd)
			{
				// temporary string for the content of the current line
				char* line = (char*)::malloc(sizeof(char) * PATH_MAX);

				// reading the file line by line (which might be truncated here)
				if (::fgets(line, sizeof(char) * PATH_MAX, fd))
				{
					outLength = ::strlen(line);
					if (outLength > 0)
					{
						result = line;
						line = NULL;
					}
				}

				::free(line);
				::fclose(fd);
			}
			return result;
		}


		template<bool FetchNameInsteadOfPathT>
		static inline bool GetProcessNameImpl(uLONG inPID, VString& outName)
		{
			bool success = false;

			if (inPID > 0) // can not be null but can be an arbitrary value
			{
				// Reading the symlink is the most efficient way for getting the process name / path
				// however, it requires enought privileges for that and may easily fail
				// in this case, we will try to read the command line from /proc/<pid>/cmdline which
				// may not be accurate (and may contain something else)

				// on Linux, the file /proc/<pid>/exe is a symlink to the original program file
				// note: the real file might have been deleted
				char filename[64];
				snprintf(filename, sizeof(filename), "/proc/%u/exe", (unsigned int) inPID);

				// The size of a symbolic link is the length of the pathname it contains
				// (without a terminating null byte)
				// however, when the inode has been deleted, this size will be zero and thus
				// we can not rely on it
				//struct stat sb;
				//if (lstat(filename, &sb) == -1)
				//	break;
				// length: sb.st_size

				// the resolved filename
				char* linkname = NULL;

				uLONG capacity = 1024;
				uLONG length = 0;

				do
				{
					linkname = (char*)::realloc(linkname, sizeof(char) * capacity);
					if (linkname == NULL)
						break; // not enough memory

					// note: readlink does not happen a null byte to the buffer
					ssize_t r = readlink(filename, linkname, capacity - 1);
					if (r == -1)
					{
						// permission error - trying cmdline instead
						free(linkname);
						linkname = NULL;
						// new filename to read
						snprintf(filename, sizeof(filename), "/proc/%u/cmdline", (unsigned int) inPID);
						linkname = GetProcessNameFromProcCmdLine(filename, length);
						break; // error while reading the filename
					}

					if (r < capacity - 1)
					{
						length = (uLONG) r;
						linkname[length] = '\0';
						break;
					}

					// not enough room in the buffer, let's try again
					capacity += 4096;
					if (capacity > 64 * 1024) // just in case, should never happen of course
						break;
				}
				while (true);

				if (length > 0)
				{
					// it may happen that the program has been deleted. In this case we may have ' (deleted)'
					// appended at the end of the filename
					static const char* const deleted = " (deleted)";
					enum { deletedLength = 10 }; // strlen of deleted
					if (length > (uLONG) deletedLength && 0 == strcmp(linkname + length - deletedLength, deleted))
					{
						length -= (uLONG) deletedLength;
						linkname[length] = '\0';
					}

					// trying to find the final separator
					char* separator = strrchr(linkname, '/');
					if (separator)
					{
						// something has been found - retrieving either the name or the path
						if (FetchNameInsteadOfPathT)
						{
							outName.AppendBlock(separator + 1, (VSize) (length - (separator - linkname + 1)), VTC_UTF_8);
						}
						else
						{
							// keeping the final separator for VFilePath
							*(separator + 1) = '\0';
							outName.AppendBlock(linkname, (VSize) (separator - linkname) + 1, VTC_UTF_8);
						}
					}
					else
					{
						if (FetchNameInsteadOfPathT)
							outName.AppendBlock(linkname, (VSize) length, VTC_UTF_8);
						// the path will be empty (now separator, no folder)
					}

					success = true;
				}

				// release
				::free(linkname);
			}
			return success;
		}

	} // anonymous namespace



	void GetProcessName(uLONG inProcessID, VString& outProcessName)
	{
		outProcessName.Clear();
		GetProcessNameImpl<true>(inProcessID, outProcessName);
	}


	void GetProcessPath(uLONG inProcessID, VString &outProcessPath)
	{
		outProcessPath.Clear();
		GetProcessNameImpl<false>(inProcessID, outProcessPath);
	}






} // namespace XLinuxSystem

END_TOOLBOX_NAMESPACE


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
#include "VSystem.h"
#include "VString.h"
#include "VError.h"
#include "VTime.h"
#include "VValueBag.h"

#if VERSIONMAC
#include <AudioToolbox/AudioServices.h>
#include <unistd.h>
#include <mach/mach_host.h>
#include <mach/task.h>
#include <mach/thread_info.h>
#include <mach/thread_act.h>
#include <mach/vm_map.h>
#include <mach/mach_time.h>
#include <mach/mach_port.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <net/route.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <cxxabi.h>
#include "XMacSystem.h"

#elif VERSION_LINUX

#include "XLinuxSystem.h"
#include <cxxabi.h>

#elif VERSIONWIN

#include "Lmcons.h"
#include <pdhmsg.h>
#include <pdh.h>
#include <imagehlp.h>
#include <psapi.h>

#include "XWinSystem.h"

#define SYSTEM_OBJECT_INDEX					2		// 'System' object
#define MEMORY_OBJECT_INDEX					4		// 'Memory' object
#define PROCESS_OBJECT_INDEX				230		// 'Process' object
#define PROCESSOR_OBJECT_INDEX				238		// 'Processor' object
#define NETWORK_OBJECT_INDEX				510		// 'Processor' object
#define TOTAL_PROCESSOR_TIME_COUNTER_INDEX	240		// '% Total processor time' counter (valid in WinNT under 'System' object)
#define PROCESSOR_TIME_COUNTER_INDEX		6		// '% processor time' counter (for Win2K/XP)
#define WORKING_SET_COUNTER_INDEX			180		// 'Working Set' counter (for Win2K/XP, PrivateWorking Set is not available, see http://msdn2.microsoft.com/en-us/library/aa965225(VS.85).aspx)
#define PRIVATE_WORKING_SET_COUNTER_INDEX	1478	// 'Working Set - Private' counter
#define AVAILABLE_BYTES_COUNTER_INDEX		24		// 'Available Bytes' counter
#define RECEIVED_BYTES_S_COUNTER_INDEX		506		// 'Received Bytes/s' counter
#define SENT_BYTES_S_COUNTER_INDEX			264		// 'Sent Bytes/s' counter



//Needed for some counters
static bool _WIN_GetApplicationName(VString& outApplicationName)
{
	bool applicationNameWasFound = false;
	static VString sApplicationName;

	if(sApplicationName.IsEmpty()){
		outApplicationName.Clear();

		//Find the application name
		char    szAppPath[MAX_PATH] = "";
		::GetModuleFileName(0, szAppPath, MAX_PATH);
		outApplicationName = szAppPath;
		sLONG lastSlashPosition = 0;
		while(true){
			sLONG tempSlashPosition = outApplicationName.Find("\\", lastSlashPosition + 1, true);
			if(tempSlashPosition > 0)
				lastSlashPosition = tempSlashPosition;
			else
				break;
		}

		if(lastSlashPosition > 0){
			outApplicationName.SubString(lastSlashPosition + 1, outApplicationName.GetLength() - lastSlashPosition);
			outApplicationName.Exchange(".exe", "");
			sApplicationName = outApplicationName;
			applicationNameWasFound = true;
		}
		else{
			assert(false);
		}
	}
	else{
		outApplicationName.FromString(sApplicationName);
		applicationNameWasFound = true;
	}
	return applicationNameWasFound;
}

static VString LookupPerfNameByIndex(DWORD inIndex)
{
  VString resultName;

  //First call the function to determine the size of buffer required
  DWORD bufferSize = 1024;
  TCHAR* buffer = static_cast<TCHAR*>(_alloca(bufferSize*sizeof(TCHAR)));
  PDH_STATUS pdhStatus = PdhLookupPerfNameByIndex(NULL, inIndex, buffer, &bufferSize);
  if (pdhStatus == PDH_MORE_DATA)
  {
    //Call again with the newly allocated buffer
    buffer = static_cast<TCHAR*>(_alloca(bufferSize * sizeof(TCHAR)));
    pdhStatus = PdhLookupPerfNameByIndex(NULL, inIndex, buffer, &bufferSize);
    if (pdhStatus == ERROR_SUCCESS)
    {
		resultName.FromBlock( buffer, bufferSize - 1, VTC_DefaultTextExport);
    }
  }
  else if (pdhStatus == ERROR_SUCCESS){
	resultName.FromBlock( buffer, bufferSize - 1, VTC_DefaultTextExport);
  }

	assert(pdhStatus == ERROR_SUCCESS);

  return resultName;
}

#endif


// Private constants
const sLONG	kDEFAULT_TIPS_DELAY		= 60;
const sLONG	kDEFAULT_TIPS_TIMEOUT	= 600;
const sLONG	kDEFAULT_SELECT_DELAY	= 15;
const sLONG	kDEFAULT_CLIC_OFFSET	= 2;


// Class statics

sWORD	VSystem::sCenturyPivotYear = 0;

#if VERSIONMAC
//To know CPU usage
static processor_cpu_load_info_t savedCPUTicks = NULL;
static mach_timebase_info_data_t sMAC_Timebase;
static bool sMAC_TimebaseUsingNanoseconds = (::mach_timebase_info( &sMAC_Timebase), (sMAC_Timebase.numer == 1) && (sMAC_Timebase.denom == 1) );
#elif VERSION_LINUX
// Need a Linux Implementation !
#elif VERSIONWIN
//To know CPU usage
static PDH_HCOUNTER totalProcessorCounter = NULL;
static PDH_HQUERY totalProcessorCounterQuery = NULL;
//Application CPU usage
static PDH_HCOUNTER applicationProcessorCounter = NULL;
static PDH_HQUERY applicationProcessorCounterQuery = NULL;
//Application Memory usage
static PDH_HCOUNTER applicationMemoryCounter = NULL;
static PDH_HQUERY  applicationMemoryCounterQuery = NULL;
//System free Memory usage
static PDH_HCOUNTER freeMemoryCounter = NULL;
static PDH_HQUERY  freeMemoryCounterQuery = NULL;
//System Network usage
static PDH_HCOUNTER receivedNetworkCounter = NULL;
static PDH_HCOUNTER sentNetworkCounter = NULL;
static PDH_HQUERY  networkCounterQuery = NULL;
#endif
static XBOX::VString	sOSVersionString;

#if VERSIONMAC
static CFTimeZoneRef MAC_GetUTCZone()
{
    static CFTimeZoneRef utc_zone = ::CFTimeZoneCreateWithTimeIntervalFromGMT(NULL, 0.0);
	return utc_zone;
}
#endif


void VSystem::Beep()
{
#if VERSIONMAC
	::AudioServicesPlayAlertSound( kUserPreferredAlert);

#elif VERSION_LINUX
	return XLinuxSystem::Beep();

#elif VERSIONWIN
	::MessageBeep(MB_OK);

#endif
}

bool VSystem::Init ()
{
	return true;
}

void VSystem::DeInit()
{
#if VERSIONWIN
	if(totalProcessorCounterQuery != NULL){
		if(totalProcessorCounter != NULL){
			PDH_STATUS pdhStatus = PdhRemoveCounter(totalProcessorCounter);
			assert(pdhStatus == ERROR_SUCCESS);
			totalProcessorCounter = NULL;
		}
		PdhCloseQuery(totalProcessorCounterQuery);
		totalProcessorCounterQuery = NULL;
	}

	if(applicationProcessorCounterQuery != NULL){
		if(applicationProcessorCounter != NULL){
			PDH_STATUS pdhStatus = PdhRemoveCounter(applicationProcessorCounter);
			assert(pdhStatus == ERROR_SUCCESS);
			applicationProcessorCounter = NULL;
		}
		PdhCloseQuery(applicationProcessorCounterQuery);
		applicationProcessorCounterQuery = NULL;
	}

	if(applicationMemoryCounterQuery != NULL){
		if(applicationMemoryCounter != NULL){
			PDH_STATUS pdhStatus = PdhRemoveCounter(applicationMemoryCounter);
			assert(pdhStatus == ERROR_SUCCESS);
			applicationMemoryCounter = NULL;
		}
		PdhCloseQuery(applicationMemoryCounterQuery);
		applicationMemoryCounterQuery = NULL;
	}

	if(freeMemoryCounterQuery != NULL){
		if(freeMemoryCounter != NULL){
			PDH_STATUS pdhStatus = PdhRemoveCounter(freeMemoryCounter);
			assert(pdhStatus == ERROR_SUCCESS);
			freeMemoryCounter = NULL;
		}
		PdhCloseQuery(freeMemoryCounterQuery);
		freeMemoryCounterQuery = NULL;
	}

	if(networkCounterQuery != NULL){
		if(receivedNetworkCounter != NULL){
			PDH_STATUS pdhStatus = PdhRemoveCounter(receivedNetworkCounter);
			assert(pdhStatus == ERROR_SUCCESS);
			receivedNetworkCounter = NULL;
		}
		if(sentNetworkCounter != NULL){
			PDH_STATUS pdhStatus = PdhRemoveCounter(sentNetworkCounter);
			assert(pdhStatus == ERROR_SUCCESS);
			sentNetworkCounter = NULL;
		}
		PdhCloseQuery(networkCounterQuery);
		networkCounterQuery = NULL;
	}

	WIN_SetIDList(0);

#elif VERSIONMAC
	if(savedCPUTicks != NULL)
		free(savedCPUTicks);

#endif
}


bool VSystem::IsSystemVersionOrAbove( SystemVersion inSystemVersion)
{
	return XSystemImpl::IsSystemVersionOrAbove( inSystemVersion);
}


#if VERSIONWIN
void* VSystem::fIDList = 0;
void VSystem::WIN_SetIDList(void *inList)
{
	if(fIDList)
	{
		IMalloc* alloc;

		if(::SHGetMalloc(&alloc)==NOERROR)
			alloc->Free(fIDList);
	}
	fIDList = (void*)inList; // l'idee est de l'utiliser pour initialiser le dialogue
}
#endif	// #if VERSIONWIN

#if VERSIONWIN
#include <tchar.h>	// For ::_tcslen

typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
typedef BOOL (WINAPI *PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);

// Those values are not defined currently (2008-08-04) while tested in the msdn sample used below.
#ifndef PRODUCT_UNDEFINED
#define PRODUCT_UNDEFINED						0x00000000
#endif
#ifndef PRODUCT_ULTIMATE
#define PRODUCT_ULTIMATE						0x00000001
#endif
#ifndef PRODUCT_HOME_BASIC
#define PRODUCT_HOME_BASIC						0x00000002
#endif
#ifndef PRODUCT_HOME_PREMIUM
#define PRODUCT_HOME_PREMIUM					0x00000003
#endif
#ifndef PRODUCT_ENTERPRISE
#define PRODUCT_ENTERPRISE						0x00000004
#endif
#ifndef PRODUCT_HOME_BASIC_N
#define PRODUCT_HOME_BASIC_N					0x00000005
#endif
#ifndef PRODUCT_BUSINESS
#define PRODUCT_BUSINESS						0x00000006
#endif
#ifndef PRODUCT_STANDARD_SERVER
#define PRODUCT_STANDARD_SERVER					0x00000007
#endif
#ifndef PRODUCT_DATACENTER_SERVER
#define PRODUCT_DATACENTER_SERVER				0x00000008
#endif
#ifndef PRODUCT_SMALLBUSINESS_SERVER
#define PRODUCT_SMALLBUSINESS_SERVER			0x00000009
#endif
#ifndef PRODUCT_ENTERPRISE_SERVER
#define PRODUCT_ENTERPRISE_SERVER				0x0000000A
#endif
#ifndef PRODUCT_STARTER
#define PRODUCT_STARTER							0x0000000B
#endif
#ifndef PRODUCT_DATACENTER_SERVER_CORE
#define PRODUCT_DATACENTER_SERVER_CORE			0x0000000C
#endif
#ifndef PRODUCT_STANDARD_SERVER_CORE
#define PRODUCT_STANDARD_SERVER_CORE			0x0000000D
#endif
#ifndef PRODUCT_ENTERPRISE_SERVER_CORE
#define PRODUCT_ENTERPRISE_SERVER_CORE			0x0000000E
#endif
#ifndef PRODUCT_ENTERPRISE_SERVER_IA64
#define PRODUCT_ENTERPRISE_SERVER_IA64			0x0000000F
#endif
#ifndef PRODUCT_BUSINESS_N
#define PRODUCT_BUSINESS_N						0x00000010
#endif
#ifndef PRODUCT_WEB_SERVER
#define PRODUCT_WEB_SERVER						0x00000011
#endif
#ifndef PRODUCT_CLUSTER_SERVER
#define PRODUCT_CLUSTER_SERVER					0x00000012
#endif
#ifndef PRODUCT_HOME_SERVER
#define PRODUCT_HOME_SERVER						0x00000013
#endif
#ifndef PRODUCT_STORAGE_EXPRESS_SERVER
#define PRODUCT_STORAGE_EXPRESS_SERVER			0x00000014
#endif
#ifndef PRODUCT_STORAGE_STANDARD_SERVER
#define PRODUCT_STORAGE_STANDARD_SERVER			0x00000015
#endif
#ifndef PRODUCT_STORAGE_WORKGROUP_SERVER
#define PRODUCT_STORAGE_WORKGROUP_SERVER		0x00000016
#endif
#ifndef PRODUCT_STORAGE_ENTERPRISE_SERVER
#define PRODUCT_STORAGE_ENTERPRISE_SERVER		0x00000017
#endif
#ifndef PRODUCT_SERVER_FOR_SMALLBUSINESS
#define PRODUCT_SERVER_FOR_SMALLBUSINESS		0x00000018
#endif
#ifndef PRODUCT_SMALLBUSINESS_SERVER_PREMIUM
#define PRODUCT_SMALLBUSINESS_SERVER_PREMIUM	0x00000019
#endif
#ifndef PRODUCT_UNLICENSED
#define PRODUCT_UNLICENSED						0xABCDABCD
#endif
#ifndef SM_SERVERR2
#define SM_SERVERR2								89
#endif

#endif	// VERSIONWIN

void VSystem::GetOSVersionString (VString &outStr)
{
	if(sOSVersionString.IsEmpty())
	{
#if VERSIONMAC
		VError err = XMacSystem::GetMacOSXFormattedVersionString(sOSVersionString);
		xbox_assert(err == VE_OK);

#elif VERSION_LINUX
		return XLinuxSystem::GetOSVersionString(outStr);

#elif VERSIONWIN
		// T.A. 2008-08-04, ACI0058080, Win Server 2008 draws "Vista (6.0)"
		// The fix uses Microsoft example code, "Getting the System Version" (http://msdn.microsoft.com/en-us/library/ms724429(VS.85).aspx)
		OSVERSIONINFOEX	osvi;
		SYSTEM_INFO		si;
		PGNSI			pGNSI;
		PGPI			pGPI;
		DWORD			dwType;

		ZeroMemory(&si, sizeof(SYSTEM_INFO));
		ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

		if(::GetVersionEx ((OSVERSIONINFO *) &osvi))
		{
			// Call GetNativeSystemInfo if supported or GetSystemInfo otherwise.
			pGNSI = (PGNSI) GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetNativeSystemInfo");
			if(NULL != pGNSI)
				pGNSI(&si);
			else
				GetSystemInfo(&si);

			if ( VER_PLATFORM_WIN32_NT == osvi.dwPlatformId && osvi.dwMajorVersion > 4 )
			{
			// ------------------------------------------------------------
			// Vista et Server 2008, seven
			// ------------------------------------------------------------

				if ( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1 )
				{
					if( osvi.wProductType == VER_NT_WORKSTATION )
						sOSVersionString += "Windows Seven ";
					else
						sOSVersionString += "Windows Server 2008 ";

					pGPI = (PGPI) GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetProductInfo");
					pGPI( 6, 0, 0, 0, &dwType);

					switch( dwType )
					{
					case PRODUCT_ULTIMATE:
						sOSVersionString += "Ultimate Edition";
						break;
					case PRODUCT_HOME_PREMIUM:
						sOSVersionString += "Home Premium Edition";
						break;
					case PRODUCT_HOME_BASIC:
						sOSVersionString += "Home Basic Edition";
						break;
					case PRODUCT_ENTERPRISE:
						sOSVersionString += "Enterprise Edition";
						break;
					case PRODUCT_BUSINESS:
						sOSVersionString += "Business Edition";
						break;
					case PRODUCT_STARTER:
						sOSVersionString += "Starter Edition";
						break;
					case PRODUCT_CLUSTER_SERVER:
						sOSVersionString += "Cluster Server Edition";
						break;
					case PRODUCT_DATACENTER_SERVER:
						sOSVersionString += "Datacenter Edition";
						break;
					case PRODUCT_DATACENTER_SERVER_CORE:
						sOSVersionString += "Datacenter Edition (core installation)";
						break;
					case PRODUCT_ENTERPRISE_SERVER:
						sOSVersionString += "Enterprise Edition";
						break;
					case PRODUCT_ENTERPRISE_SERVER_CORE:
						sOSVersionString += "Enterprise Edition (core installation)";
						break;
					case PRODUCT_ENTERPRISE_SERVER_IA64:
						sOSVersionString += "Enterprise Edition for Itanium-based Systems";
						break;
					case PRODUCT_SMALLBUSINESS_SERVER:
						sOSVersionString += "Small Business Server";
						break;
					case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM:
						sOSVersionString += "Small Business Server Premium Edition";
						break;
					case PRODUCT_STANDARD_SERVER:
						sOSVersionString += "Standard Edition";
						break;
					case PRODUCT_STANDARD_SERVER_CORE:
						sOSVersionString += "Standard Edition (core installation)";
						break;
					case PRODUCT_WEB_SERVER:
						sOSVersionString += "Web Server Edition";
						break;
					}
					if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
						sOSVersionString += ", 64-bit";
					else if (si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_INTEL )
						sOSVersionString += ", 32-bit";
				}

				if ( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0 )
				{
					if( osvi.wProductType == VER_NT_WORKSTATION )
						sOSVersionString += "Windows Vista ";
					else
						sOSVersionString += "Windows Server 2008 ";

					pGPI = (PGPI) GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetProductInfo");
					pGPI( 6, 0, 0, 0, &dwType);

					switch( dwType )
					{
					case PRODUCT_ULTIMATE:
						sOSVersionString += "Ultimate Edition";
						break;
					case PRODUCT_HOME_PREMIUM:
						sOSVersionString += "Home Premium Edition";
						break;
					case PRODUCT_HOME_BASIC:
						sOSVersionString += "Home Basic Edition";
						break;
					case PRODUCT_ENTERPRISE:
						sOSVersionString += "Enterprise Edition";
						break;
					case PRODUCT_BUSINESS:
						sOSVersionString += "Business Edition";
						break;
					case PRODUCT_STARTER:
						sOSVersionString += "Starter Edition";
						break;
					case PRODUCT_CLUSTER_SERVER:
						sOSVersionString += "Cluster Server Edition";
						break;
					case PRODUCT_DATACENTER_SERVER:
						sOSVersionString += "Datacenter Edition";
						break;
					case PRODUCT_DATACENTER_SERVER_CORE:
						sOSVersionString += "Datacenter Edition (core installation)";
						break;
					case PRODUCT_ENTERPRISE_SERVER:
						sOSVersionString += "Enterprise Edition";
						break;
					case PRODUCT_ENTERPRISE_SERVER_CORE:
						sOSVersionString += "Enterprise Edition (core installation)";
						break;
					case PRODUCT_ENTERPRISE_SERVER_IA64:
						sOSVersionString += "Enterprise Edition for Itanium-based Systems";
						break;
					case PRODUCT_SMALLBUSINESS_SERVER:
						sOSVersionString += "Small Business Server";
						break;
					case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM:
						sOSVersionString += "Small Business Server Premium Edition";
						break;
					case PRODUCT_STANDARD_SERVER:
						sOSVersionString += "Standard Edition";
						break;
					case PRODUCT_STANDARD_SERVER_CORE:
						sOSVersionString += "Standard Edition (core installation)";
						break;
					case PRODUCT_WEB_SERVER:
						sOSVersionString += "Web Server Edition";
						break;
					}
					if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
						sOSVersionString += ", 64-bit";
					else if (si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_INTEL )
						sOSVersionString += ", 32-bit";
				}

			// ------------------------------------------------------------
			// Server 2003 et XP Pro x64
			// ------------------------------------------------------------
				if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
				{
					if( GetSystemMetrics(SM_SERVERR2) )
						sOSVersionString += "Windows Server 2003 R2, ";
					else if ( osvi.wSuiteMask==VER_SUITE_STORAGE_SERVER )
						sOSVersionString += "Windows Storage Server 2003";
					else if( osvi.wProductType == VER_NT_WORKSTATION &&
						si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
					{
						sOSVersionString += "Windows XP Professional x64 Edition";
					}
					else
						sOSVersionString += "Windows Server 2003, ";

					// Test for the server type.
					if ( osvi.wProductType != VER_NT_WORKSTATION )
					{
						if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_IA64 )
						{
							if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
								sOSVersionString += "Datacenter Edition for Itanium-based Systems";
							else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
								sOSVersionString += "Enterprise Edition for Itanium-based Systems";
						}

						else if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
						{
							if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
								sOSVersionString += "Datacenter x64 Edition";
							else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
								sOSVersionString += "Enterprise x64 Edition";
							else sOSVersionString += "Standard x64 Edition";
						}

						else
						{
							if ( osvi.wSuiteMask & VER_SUITE_COMPUTE_SERVER )
								sOSVersionString += "Compute Cluster Edition";
							else if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
								sOSVersionString += "Datacenter Edition";
							else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
								sOSVersionString += "Enterprise Edition";
							else if ( osvi.wSuiteMask & VER_SUITE_BLADE )
								sOSVersionString += "Web Edition";
							else
								sOSVersionString += "Standard Edition";
						}
					}
				}

			// ------------------------------------------------------------
			// XP et 2000
			// ------------------------------------------------------------
				if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
				{
					sOSVersionString += "Windows XP ";
					if( osvi.wSuiteMask & VER_SUITE_PERSONAL )
						sOSVersionString += "Home Edition";
					else
						sOSVersionString += "Professional";
				}

				if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
				{
					sOSVersionString += "Windows 2000 ";

					if ( osvi.wProductType == VER_NT_WORKSTATION )
					{
						sOSVersionString += "Professional";
					}
					else
					{
						if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
							sOSVersionString += "Datacenter Server";
						else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
							sOSVersionString += "Advanced Server";
						else
							sOSVersionString += "Server";
					}
				}

				// Include service pack (if any) and build number.

				if( ::_tcslen(osvi.szCSDVersion) > 0 )
				{
					sOSVersionString += " ";
					sOSVersionString += osvi.szCSDVersion;
				}

				XBOX::VString	buildStr;

				buildStr.Printf(" (build %d)", osvi.dwBuildNumber);
				sOSVersionString += buildStr;
			}
			else if ( VER_PLATFORM_WIN32_NT == osvi.dwPlatformId && osvi.dwMajorVersion == 4 )
			{
				sOSVersionString.Printf("Windows NT (%d.%d)", osvi.dwMajorVersion, osvi.dwMinorVersion);
			}
			else
				sOSVersionString.Printf("Windows (Platform ID: %d, MajorVersion: %d, MinorVersion: %d ", osvi.dwPlatformId, osvi.dwMajorVersion, osvi.dwMinorVersion);

		} // if(::GetVersionEx ((OSVERSIONINFO *) &osvi)) )
		else
			sOSVersionString = "Windows version unknown";
#endif
	}
	outStr = sOSVersionString;
}

#if VERSIONMAC

sLONG VSystem::MAC_GetSystemVersionDecimal()
{
	return XMacSystem::GetMacOSXVersionDecimal();
}

#endif

sLONG VSystem::Random( bool inComputeFast )
{
	sLONG longRandom;

	#if VERSIONMAC
		if (inComputeFast)
			longRandom = (sLONG) random();
		else
			longRandom = arc4random() % ((unsigned)RAND_MAX + 1); // YT 27-Aug-2009 - ACI0062605

    #elif VERSION_LINUX
        longRandom=XLinuxSystem::Random(inComputeFast);

	#elif VERSIONWIN
		static HCRYPTPROV phProv = NULL;
		if ( inComputeFast || ( !phProv && !CryptAcquireContext( &phProv, NULL, NULL, PROV_RSA_FULL, 0 ) ) )
		{
			// seed init done in thread startup routine

			// On Windows we don't have random() to generate 4-bytes numbers so we use rand() twice to generate
			// two 2-bytes numbers and concatenate them (and we drop the sign bit to return positive numbers)
			longRandom = ((rand() & 0x7fff) << 16) | rand(); // DH 21-Feb-2013 - ACI0066723 - rand()*rand() is biased towards even numbers
		}
		else
		{
			CryptGenRandom( phProv, 4, ((BYTE*)&longRandom) );
		}
	#endif

	return longRandom;
}


bool VSystem::DisplayNotification( const VString& inTitle, const VString& inMessage, EDisplayNotificationOptions inOptions, ENotificationResponse *outResponse)
{
	return XSystemImpl::DisplayNotification( inTitle, inMessage, inOptions, outResponse);
}


void VSystem::LocalToUTCTime(sWORD ioVals[7])
{
	// convert only non null dates
	if (ioVals[0] != 0 || (ioVals[1] != 0 && ioVals[3] != 0) )
	{
	#if VERSIONMAC
		CFGregorianDate date;
		date.year = ioVals[0];
		date.month = ioVals[1];
		date.day = ioVals[2];
		date.hour = ioVals[3];
		date.minute = ioVals[4];
		date.second = ioVals[5];

		CFTimeZoneRef timeZone = ::CFTimeZoneCopyDefault();

		CFAbsoluteTime time = CFGregorianDateGetAbsoluteTime( date, timeZone);

		date = ::CFAbsoluteTimeGetGregorianDate( time, MAC_GetUTCZone());

		::CFRelease( timeZone);

		ioVals[0] = date.year;
		ioVals[1] = date.month;
		ioVals[2] = date.day;
		ioVals[3] = date.hour;
		ioVals[4] = date.minute;
		ioVals[5] = date.second;

    #elif VERSION_LINUX
		return XLinuxSystem::LocalToUTCTime(ioVals);

    #elif VERSIONWIN
		SYSTEMTIME local, utc;

		local.wYear = ioVals[0];
		local.wMonth = ioVals[1];
		local.wDay = ioVals[2];
		local.wHour = ioVals[3];
		local.wMinute = ioVals[4];
		local.wSecond = ioVals[5];
		local.wMilliseconds = ioVals[6];

		// TzSpecificLocalTimeToSystemTime doesn't support dates before 1/1/1600
		if (::TzSpecificLocalTimeToSystemTime( NULL, &local, &utc))
		{
			ioVals[0] = utc.wYear;
			ioVals[1] = utc.wMonth;
			ioVals[2] = utc.wDay;
			ioVals[3] = utc.wHour;
			ioVals[4] = utc.wMinute;
			ioVals[5] = utc.wSecond;
			ioVals[6] = utc.wMilliseconds;
		}

	#endif
	}
}

void VSystem::UTCToLocalTime(sWORD ioVals[7])
{
	// convert only non null dates
	if (ioVals[0] != 0 || (ioVals[1] != 0 && ioVals[3] != 0) )
	{
	#if VERSIONMAC

		CFGregorianDate date;
		date.year = ioVals[0];
		date.month = ioVals[1];
		date.day = ioVals[2];
		date.hour = ioVals[3];
		date.minute = ioVals[4];
		date.second = ioVals[5];

		CFTimeZoneRef timeZone = ::CFTimeZoneCopyDefault();

		CFAbsoluteTime time = CFGregorianDateGetAbsoluteTime( date, MAC_GetUTCZone());

		date = ::CFAbsoluteTimeGetGregorianDate( time, timeZone);

		::CFRelease( timeZone);

		ioVals[0] = date.year;
		ioVals[1] = date.month;
		ioVals[2] = date.day;
		ioVals[3] = date.hour;
		ioVals[4] = date.minute;
		ioVals[5] = date.second;
		//ioVals[6] = 0;

	#elif VERSION_LINUX
		return XLinuxSystem::UTCToLocalTime(ioVals);

    #elif VERSIONWIN
		SYSTEMTIME local, utc, utc2, local2;

		utc.wYear = ioVals[0];
		utc.wMonth = ioVals[1];
		utc.wDay = ioVals[2];
		utc.wHour = ioVals[3];
		utc.wMinute = ioVals[4];
		utc.wSecond = ioVals[5];
		utc.wMilliseconds = ioVals[6];

		if (::SystemTimeToTzSpecificLocalTime( NULL, &utc, &local))
		{
			// DH 03-Feb-2014 ACI0076772 Workaround for SystemTimeToTzSpecificLocalTime bug
			// This ugly code tries to work around a SystemTimeToTzSpecificLocalTime bug while being compatible with Windows XP
			// As mentioned in the MSDN documentation :
			// " The SystemTimeToTzSpecificLocalTime function may calculate the local time incorrectly under the following conditions:
			// - The time zone uses a different UTC offset for the old and new years. 
			// - The UTC time to be converted and the calculated local time are in different years. "
			// This happens for example in timezone Buenos Aires (UTC-3) on 01/01/2009 between 0h and 1h (local time) because in
			// december 2008 and january 2009 it had a +1 DST offset (resulting in UTC-2) which was not applicable in december 2009.
			// I suspect Windows gets the timezone data for 2009, applies standard timezone correction (-3h) getting a result on 12/31/2008
			// then checks for the DST offset on the result using the already retrieved timezone data, i.e. it checks 2009 timezone data for
			// DST applicable in december ... if it had checked the timezone data for december 2008 the result would have been correct.
			//
			// It seems until we drop support for Windows XP we have no clean solution available from Microsoft.
			// Starting with Vista SP1 we should be able to force the correct timezone by using GetTimeZoneInformationForYear.
			// Starting with Windows 7 we should use SystemTimeToTzSpecificLocalTimeEx which works with "dynamic timezone information" to avoid this problem.
			//
			// The workaround converts the local time we got from SystemTimeToTzSpecificLocalTime back into UTC time and compares its hour
			// with out original input. They should be the same unless we hit the bug. If they're not we try to offset the local time by
			// the difference between the two UTC time, convert this corrected local time into UTC time, and check it converts to the correct
			// UTC time. If they match then we have found the correct local time and we return it, otherwise we didn't manage to work around
			// the bug, we log an assert and keep the buggy SystemTimeToTzSpecificLocalTime result.
			// Also, we do this only if the initial conversion crossed a year boundary, to reduce performance impacts to a minimum.
			if (utc.wYear != local.wYear && ::TzSpecificLocalTimeToSystemTime(NULL, &local, &utc2) && utc.wHour != utc2.wHour)
			{
				// Use VTime for computations to avoid problems when crossing days/month/years boundaries
				VTime correctedLocal, utcVtime, utc2Vtime;
				utcVtime.FromUTCTime(utc.wYear, utc.wMonth, utc.wDay, utc.wHour, utc.wMinute, utc.wSecond, utc.wMilliseconds);
				utc2Vtime.FromUTCTime(utc2.wYear, utc2.wMonth, utc2.wDay, utc2.wHour, utc2.wMinute, utc2.wSecond, utc2.wMilliseconds);

				// Compute correctedLocal = local + (utc1 - utc2)
				// Important : even though we work with local time, we use (From|Get)UTCTime because we don't want the automatic local->UTC conversion
				// (that would create an infinite loop)
				correctedLocal.FromUTCTime(local.wYear, local.wMonth, local.wDay, local.wHour, local.wMinute, local.wSecond, local.wMilliseconds);
				correctedLocal.AddMilliseconds(utcVtime.GetMilliseconds() - utc2Vtime.GetMilliseconds());
				correctedLocal.GetUTCTime((sWORD&)local2.wYear, (sWORD&)local2.wMonth, (sWORD&)local2.wDay, (sWORD&)local2.wHour, (sWORD&)local2.wMinute, (sWORD&)local2.wSecond, (sWORD&)local2.wMilliseconds);

				// Check that the corrected local time converts to the correct UTC time
				if (testAssert(::TzSpecificLocalTimeToSystemTime(NULL, &local2, &utc2) &&
					utc2.wYear == utc.wYear &&
					utc2.wMonth == utc.wMonth &&
					utc2.wDay == utc.wDay &&
					utc2.wHour == utc.wHour &&
					utc2.wMinute == utc.wMinute &&
					utc2.wSecond == utc.wSecond &&
					utc2.wMilliseconds == utc.wMilliseconds))
				{
					// That's better, we can use it
					local = local2;
				}
			}

			ioVals[0] = local.wYear;
			ioVals[1] = local.wMonth;
			ioVals[2] = local.wDay;
			ioVals[3] = local.wHour;
			ioVals[4] = local.wMinute;
			ioVals[5] = local.wSecond;
			ioVals[6] = local.wMilliseconds;
		}

	#endif
	}
}


// remplis un tableau d'entiers avec la date et l'heure UTC courante
void VSystem::GetSystemUTCTime(sWORD outVals[7])
{
#if VERSIONMAC
	CFAbsoluteTime absoluteTime = CFAbsoluteTimeGetCurrent();
	CFGregorianDate gregorianDate = CFAbsoluteTimeGetGregorianDate( absoluteTime, MAC_GetUTCZone() );

	outVals[0] = gregorianDate.year;
	outVals[1] = gregorianDate.month;
	outVals[2] = gregorianDate.day;
	outVals[3] = gregorianDate.hour;
	outVals[4] = gregorianDate.minute;
	outVals[5] = (int)(gregorianDate.second);
	outVals[6] = (int)(gregorianDate.second * 1000) % 1000;

#elif VERSION_LINUX
	return XLinuxSystem::GetSystemUTCTime(outVals);

#elif VERSIONWIN

	SYSTEMTIME st;

	::GetSystemTime(&st);
	outVals[0] = st.wYear;
	outVals[1] = st.wMonth;
	outVals[2] = st.wDay;
	outVals[3] = st.wHour;
	outVals[4] = st.wMinute;
	outVals[5] = st.wSecond;
	outVals[6] = st.wMilliseconds;

#endif
}


// remplis un tableau d'entiers avec la date et l'heure locale courante
void VSystem::GetSystemLocalTime(sWORD outVals[7])
{
#if VERSIONMAC

	CFTimeZoneRef timeZone = ::CFTimeZoneCopyDefault();
	CFAbsoluteTime absoluteTime = CFAbsoluteTimeGetCurrent();
	CFGregorianDate gregorianDate = CFAbsoluteTimeGetGregorianDate( absoluteTime, timeZone );
	::CFRelease( timeZone);

	outVals[0] = gregorianDate.year;
	outVals[1] = gregorianDate.month;
	outVals[2] = gregorianDate.day;
	outVals[3] = gregorianDate.hour;
	outVals[4] = gregorianDate.minute;
	outVals[5] = (int)(gregorianDate.second);
	outVals[6] = (int)(gregorianDate.second * 1000) % 1000;

#elif VERSION_LINUX
	return XLinuxSystem::GetSystemLocalTime(outVals);

#elif VERSIONWIN

	SYSTEMTIME st;

	::GetLocalTime(&st);
	outVals[0] = st.wYear;
	outVals[1] = st.wMonth;
	outVals[2] = st.wDay;
	outVals[3] = st.wHour;
	outVals[4] = st.wMinute;
	outVals[5] = st.wSecond;
	outVals[6] = st.wMilliseconds;

#endif
}

void VSystem::SystemDateTimeChanged()
{
	#if VERSIONMAC
	::CFTimeZoneResetSystem();
	#endif
	UpdateGMTOffset();
}

sLONG	VSystem::sGMTOffset=0;
sLONG	VSystem::sGMTOffsetWithDayLight=0;

#if VERSIONMAC
void VSystem::_SetupDstTransitionTimer(CFAbsoluteTime inNextDstTransition)
{
	// Reference to a timer firing at the next daylight saving time transition so we know when we need to force the refresh of GMT offsets
	// (Needed for Mac OS which doesn't notify DST transition, while Windows issues a WM_TIMECHANGE on all clock changes including DST transition)
	// Memory management rule : a timer should be released only if it was retrieved from this static variable while atomicaly replacing it with something else
	static CFRunLoopTimerRef installedDstTransitionTimer = NULL;
	
	// Capture the reference to the existing timer
	CFRunLoopTimerRef oldDstTransitionTimer = XBOX::VInterlocked::ExchangePtr(&installedDstTransitionTimer);
	
	// Invalidate and release the existing timer if it exists
	if (oldDstTransitionTimer != NULL)
	{
		::CFRunLoopTimerInvalidate(oldDstTransitionTimer);
		::CFRelease(oldDstTransitionTimer);
	}
	
	// Create a new one shot timer to fire at the time of the next DST transition and install it in the main run loop
	// Note the timer doesn't call directly UpdateGMTOffset because of OS X's timer coalescing which could fire the timer
	// a split second before the actual transition which would make above code more complicated. Instead the timer is released
	// and the GMT offset cache invalidated, so it will be updated on the next call to GetGMTOffset.
	CFRunLoopTimerRef newDstTransitionTimer;
	newDstTransitionTimer = ::CFRunLoopTimerCreateWithHandler(kCFAllocatorDefault, inNextDstTransition, 0, 0, 0,
															  ^(CFRunLoopTimerRef timer) {
																  // Clear sDstTransitionTimer but only if it is the same timer that just fired
																  CFRunLoopTimerRef lastInstalledTimer = (CFRunLoopTimerRef) XBOX::VInterlocked::CompareExchangePtr((void**) &installedDstTransitionTimer, timer, NULL);
																  
																  // Release the timer but only if the compare/exchange succeeded
																  if (lastInstalledTimer == timer) ::CFRelease(timer);
																  
																  // Invalidate GMT Offset
																  sGMTOffsetValid = false;
															  });
	::CFRunLoopAddTimer(::CFRunLoopGetMain(), newDstTransitionTimer, kCFRunLoopCommonModes);
	
	// Store the reference to the new timer
	oldDstTransitionTimer = XBOX::VInterlocked::ExchangePtr(&installedDstTransitionTimer, newDstTransitionTimer);
	
	// This may happen if UpdateGMTOffset was called concurrently (this should not happen but better be safe)
	// It means two timers were set up, both should be correct, we can release the one returned by ExchangePtr
	if (oldDstTransitionTimer != NULL)
	{
		::CFRunLoopTimerInvalidate(oldDstTransitionTimer);
		::CFRelease(oldDstTransitionTimer);
	}
}
#endif

void VSystem::UpdateGMTOffset()
{
	sLONG Offset;
	sLONG OffsetDayLight;
	// not thread safe
	// call only to update gmt when the system notify a time change
#if VERSIONWIN

	TIME_ZONE_INFORMATION info = {0};
	DWORD r = ::GetTimeZoneInformation( &info);
	LONG bias = info.Bias;

	Offset = -bias * 60;

	switch( r)
	{
	case TIME_ZONE_ID_STANDARD:	bias += info.StandardBias; break;
	case TIME_ZONE_ID_DAYLIGHT:	bias += info.DaylightBias; break;
	}

	// bias is in minutes, we want seconds.
	OffsetDayLight = -bias * 60;

#elif VERSIONMAC
	CFTimeZoneRef timeZone = ::CFTimeZoneCopyDefault();

	CFAbsoluteTime absoluteTime = ::CFAbsoluteTimeGetCurrent();

	CFTimeInterval offset = CFTimeZoneGetSecondsFromGMT( timeZone, absoluteTime);

	OffsetDayLight = (sLONG) offset;
	Offset = OffsetDayLight;

	Boolean is_daylight = CFTimeZoneIsDaylightSavingTime( timeZone, absoluteTime);
	if (is_daylight)
		Offset -= 3600;

	// Setup a timer for the next DST change in the timezone to invalidate the cached values
	_SetupDstTransitionTimer(::CFTimeZoneGetNextDaylightSavingTimeTransition(timeZone, absoluteTime));
	
	::CFRelease( timeZone);
#else
		
	XLinuxSystem::UpdateGMTOffset(&Offset, &OffsetDayLight);

#endif

	XBOX::VInterlocked::Exchange((sLONG*)&sGMTOffset,(sLONG)Offset);
	XBOX::VInterlocked::Exchange((sLONG*)&sGMTOffsetWithDayLight,(sLONG)OffsetDayLight);
}

bool VSystem::sGMTOffsetValid = false;

sLONG VSystem::GetGMTOffset( bool inIncludingDaylightOffset)
{
	if (!sGMTOffsetValid)
	{
		UpdateGMTOffset();
		sGMTOffsetValid = true;
	}
	if(inIncludingDaylightOffset)
		return sGMTOffsetWithDayLight;
	else
		return sGMTOffset;
}


void VSystem::SetCenturyPivotYear(sWORD inYear)
{
	sCenturyPivotYear = inYear;
}


sWORD VSystem::GetCenturyPivotYear()
{
	return sCenturyPivotYear;
}

sLONG VSystem::GetDoubleClicOffsetWidth()
{
#if VERSIONWIN
	return ::GetSystemMetrics(SM_CXDOUBLECLK);
#elif VERSIONMAC
	return kDEFAULT_CLIC_OFFSET;
#elif VERSION_LINUX
    xbox_assert(0); // No need for that on Linux !
    return 0;
#endif
}

sLONG VSystem::GetDoubleClicOffsetHeight()
{
#if VERSIONWIN
	return ::GetSystemMetrics(SM_CYDOUBLECLK);
#elif VERSIONMAC
	return kDEFAULT_CLIC_OFFSET;
#elif VERSION_LINUX
    xbox_assert(0); // No need for that on Linux !
    return 0;
#endif
}

sLONG VSystem::GetDragOffset()
{
#if VERSIONWIN
	sLONG	result = ::GetSystemMetrics(SM_CXDRAG);

	// Make sure horiz and vert values are equals (as assumed by the Toolbox)
	xbox_assert(::GetSystemMetrics(SM_CYDRAG) == result);
	return result;
#elif VERSIONMAC
	return kDEFAULT_CLIC_OFFSET;
#elif VERSION_LINUX
    xbox_assert(0); // No need for that on Linux !
    return 0;
#endif
}


uLONG VSystem::GetDoubleClicTicks()
{
#if VERSIONMAC
	// value is in seconds
	double interval = 0.0;
	CFPropertyListRef pref = ::CFPreferencesCopyValue( CFSTR("com.apple.mouse.doubleClickThreshold"), kCFPreferencesAnyApplication, kCFPreferencesCurrentUser, kCFPreferencesAnyHost);
	if (pref != NULL)
	{
		CFTypeID id = CFGetTypeID( pref);
		if (id == CFNumberGetTypeID())
		{
			CFNumberRef value = static_cast<CFNumberRef>(pref);
			if (CFNumberGetValue( value, kCFNumberDoubleType, &interval))
			{
				if (interval < 0.0)
				{
					interval = 0.0;
				}
			}
		}
		CFRelease(pref);
	}

	return (interval > 0.0) ? (uLONG) (interval * 60) : 30;
#elif VERSION_LINUX
    xbox_assert(0);	// No need for that on Linux !
    return 0;
#elif VERSIONWIN
	return MsToTicks(::GetDoubleClickTime());
#endif
}


uLONG VSystem::GetCaretTicks()
{
#if VERSIONMAC
	return 60;	//::GetCaretTime();
#elif VERSION_LINUX
    xbox_assert(0); // No need for that on Linux !
    return 0;
#elif VERSIONWIN
	return MsToTicks(::GetCaretBlinkTime());
#endif
}


uLONG VSystem::GetCurrentTicks()
{
#if VERSIONMAC
	return ::TickCount();

#elif VERSION_LINUX
	return XLinuxSystem::GetCurrentTicks();

#elif VERSIONWIN
	return MsToTicks(::GetTickCount());

#endif
}


uLONG VSystem::GetCurrentDeltaTicks( uLONG inBaseTicks)
{
	uLONG t = GetCurrentTicks();
	return (t >= inBaseTicks) ? (t - inBaseTicks) : (inBaseTicks - t);
}


uLONG VSystem::GetCurrentTime()
{
#if VERSIONMAC
	uint64_t t = ::mach_absolute_time();
	if (sMAC_TimebaseUsingNanoseconds)
		return (uLONG) (t / 1000000);

	Nanoseconds nano = AbsoluteToNanoseconds( (AbsoluteTime&) t);
	return (uLONG) ((uLONG8&) nano / 1000000);

#elif VERSION_LINUX
	return XLinuxSystem::GetCurrentTimeStamp();

#elif VERSIONWIN
	return ::timeGetTime();

#endif
}


void VSystem::GetProfilingCounter(sLONG8& outVal)
{
#if VERSIONMAC
	outVal = ::mach_absolute_time();

#elif VERSION_LINUX
	XLinuxSystem::GetProfilingCounter(outVal);

#elif VERSIONWIN
	::QueryPerformanceCounter((LARGE_INTEGER *) &outVal);

#endif
}


sLONG8 VSystem::GetProfilingFrequency()
{
	sLONG8 val;

#if VERSIONMAC
	if (sMAC_TimebaseUsingNanoseconds)
		val = 1000000000L;
	else
		val = (sLONG8) ( (double) (sMAC_Timebase.denom * 1000000000.0) / (double) sMAC_Timebase.numer);

#elif VERSION_LINUX
	val=XLinuxSystem::GetProfilingFrequency();

#elif VERSIONWIN
	::QueryPerformanceFrequency((LARGE_INTEGER *) &val);

#endif

	return val;
}


uLONG VSystem::GetTipsDelayTicks()
{
	return kDEFAULT_TIPS_DELAY;
}


uLONG VSystem::GetTipsTimeoutTicks()
{
	return kDEFAULT_TIPS_TIMEOUT;
}


uLONG VSystem::GetSelectDelayTicks()
{
	return kDEFAULT_SELECT_DELAY;
}


// This function allow a non blocking delay.
//
// You typically use this to synchronize animations or specific loops
// that need high response without blocking the process.
//
// You should not call this function with high values (>1s) and especially
// not with a 'forever' constant. Use VTask::Sleep ou VTask::Freeze
// in those cases.
void VSystem::Delay(sLONG inMilliseconds)
{
	xbox_assert(inMilliseconds > 0);

#if VERSIONMAC
	return XMacSystem::Delay(inMilliseconds);

#elif VERSION_LINUX
	return XLinuxSystem::Delay(inMilliseconds);

#elif VERSIONWIN
	::Sleep((DWORD)inMilliseconds);

#endif
}


sLONG VSystem::GetNumberOfProcessors()
{
	static volatile sLONG sNumProcessors = 0;

	if (sNumProcessors == 0)
	{
		#if VERSIONWIN
		SYSTEM_INFO infoReturn[1];
		::GetSystemInfo(infoReturn);
		sNumProcessors = infoReturn->dwNumberOfProcessors;
		#elif VERSIONMAC
		sNumProcessors = XMacSystem::GetNumberOfProcessors();
		#elif VERSION_LINUX
		sNumProcessors = XLinuxSystem::GetNumberOfProcessors();
		#endif
	}
	return sNumProcessors;
}


bool VSystem::GetProcessorsPercentageUse(Real& outPercentageUse)
{
	bool isResultTrustable = false;
	outPercentageUse = 0;
#if VERSIONWIN

	//First time : Initialize the counter and the query
	if(totalProcessorCounterQuery == NULL){
		VString processusProcessorTimePercentagePath = "\\";
		processusProcessorTimePercentagePath += LookupPerfNameByIndex(PROCESSOR_OBJECT_INDEX);
		processusProcessorTimePercentagePath += "(_Total)\\";
		processusProcessorTimePercentagePath += LookupPerfNameByIndex(PROCESSOR_TIME_COUNTER_INDEX);
		char processusProcessorTimePercentagePathCString[1024];
		processusProcessorTimePercentagePath.ToCString(processusProcessorTimePercentagePathCString, 1024);

		PDH_STATUS pdhStatus = PdhOpenQueryH(NULL, NULL, &totalProcessorCounterQuery);

		if (testAssert(pdhStatus == ERROR_SUCCESS)){
			pdhStatus = PdhAddCounter(totalProcessorCounterQuery, processusProcessorTimePercentagePathCString, NULL, &totalProcessorCounter);
			assert(pdhStatus == ERROR_SUCCESS && totalProcessorCounter != NULL);
		}
	}

	PDH_STATUS pdhStatus = PdhCollectQueryData(totalProcessorCounterQuery);
	if (testAssert(pdhStatus == ERROR_SUCCESS)){
		PDH_FMT_COUNTERVALUE counterValue;
		// Divide the result by 100
		pdhStatus = PdhSetCounterScaleFactor(totalProcessorCounter, -2);
		assert(pdhStatus == ERROR_SUCCESS);
		PDH_STATUS Status = PdhGetFormattedCounterValue(totalProcessorCounter, PDH_FMT_DOUBLE|PDH_FMT_NOCAP100, NULL, &counterValue);
		if(counterValue.CStatus == ERROR_SUCCESS){
			isResultTrustable = true;
			outPercentageUse = counterValue.doubleValue;
		}
	}

#elif VERSIONMAC

	unsigned int				processorCount;
	processor_cpu_load_info_t	processorTickInfo;
	mach_msg_type_number_t		processorMsgCount = PROCESSOR_CPU_LOAD_INFO_COUNT;
	mach_port_t host = mach_host_self();
	if(host_processor_info(host, PROCESSOR_CPU_LOAD_INFO, &processorCount, (processor_info_array_t *)&processorTickInfo, &processorMsgCount) == KERN_SUCCESS){

		if(savedCPUTicks == NULL){
			//First saving of values
			savedCPUTicks = (processor_cpu_load_info_t)malloc(processorCount * sizeof(processor_cpu_load_info));
			int i, j;
			for (i = 0; i < processorCount; i++) {
				for (j = 0; j < CPU_STATE_MAX; j++) {
					savedCPUTicks[i].cpu_ticks[j] = processorTickInfo[i].cpu_ticks[j];
				}
			}
		}
		else{
			unsigned int system = 0, user = 0, nice = 0, idle = 0;
			unsigned int total;
			int i;
			for (i = 0; i < processorCount; i++) {
				if (processorTickInfo[i].cpu_ticks[CPU_STATE_SYSTEM] >= savedCPUTicks[i].cpu_ticks[CPU_STATE_SYSTEM]) {
					system += processorTickInfo[i].cpu_ticks[CPU_STATE_SYSTEM] - savedCPUTicks[i].cpu_ticks[CPU_STATE_SYSTEM];
				}
				else {
					system += processorTickInfo[i].cpu_ticks[CPU_STATE_SYSTEM] + (~savedCPUTicks[i].cpu_ticks[CPU_STATE_SYSTEM] + 1);
				}

				if (processorTickInfo[i].cpu_ticks[CPU_STATE_USER] >= savedCPUTicks[i].cpu_ticks[CPU_STATE_USER]) {
					user += processorTickInfo[i].cpu_ticks[CPU_STATE_USER] - savedCPUTicks[i].cpu_ticks[CPU_STATE_USER];
				}
				else {
					user += processorTickInfo[i].cpu_ticks[CPU_STATE_USER] + (~savedCPUTicks[i].cpu_ticks[CPU_STATE_USER] + 1);
				}

				if (processorTickInfo[i].cpu_ticks[CPU_STATE_NICE] >= savedCPUTicks[i].cpu_ticks[CPU_STATE_NICE]) {
					nice += processorTickInfo[i].cpu_ticks[CPU_STATE_NICE] - savedCPUTicks[i].cpu_ticks[CPU_STATE_NICE];
				}
				else {
					nice += processorTickInfo[i].cpu_ticks[CPU_STATE_NICE] + (~savedCPUTicks[i].cpu_ticks[CPU_STATE_NICE] + 1);
				}

				if (processorTickInfo[i].cpu_ticks[CPU_STATE_IDLE] >= savedCPUTicks[i].cpu_ticks[CPU_STATE_IDLE]) {
					idle += processorTickInfo[i].cpu_ticks[CPU_STATE_IDLE] - savedCPUTicks[i].cpu_ticks[CPU_STATE_IDLE];
				}
				else {
					idle += processorTickInfo[i].cpu_ticks[CPU_STATE_IDLE] + (~savedCPUTicks[i].cpu_ticks[CPU_STATE_IDLE] + 1);
				}
			}

			total = system + user + nice + idle;
			if (total < 1)
				total = 1;

			Real totalMinusIdle = (Real)(total - idle);
			outPercentageUse = (Real)(totalMinusIdle / (Real)total);

			outPercentageUse /= GetNumberOfProcessors();

			// Copy the new data into previous
			int j;
			for (i = 0; i < processorCount; i++)
			{
				for (j = 0; j < CPU_STATE_MAX; j++)
				{
					savedCPUTicks[i].cpu_ticks[j] = processorTickInfo[i].cpu_ticks[j];
				}
			}

			isResultTrustable = true;
		}

		vm_deallocate(mach_task_self(), (vm_address_t)processorTickInfo, (vm_size_t)(processorMsgCount * sizeof(*processorTickInfo)));
	}
    mach_port_deallocate(mach_task_self(), host);

#elif VERSION_LINUX
	isResultTrustable=XLinuxSystem::GetProcessorsPercentageUse(outPercentageUse);

#endif
	return isResultTrustable;
}


void VSystem::GetProcessorTypeString(VString& outProcessorType, bool inCleanupSpaces)
{
	outProcessorType.Clear();

#if VERSIONMAC
	if(XMacSystem::GetProcessorBrandName( outProcessorType) != VE_OK)
	{
		outProcessorType = "-";
	}

#elif VERSION_LINUX
	return XLinuxSystem::GetProcessorTypeString(outProcessorType, inCleanupSpaces);

#elif VERSIONWIN
	DWORD		result;
	HKEY		hKey = NULL;

	result = ::RegOpenKeyEx (HKEY_LOCAL_MACHINE,
							"Hardware\\Description\\System\\CentralProcessor\\0",
							0, KEY_QUERY_VALUE, &hKey);

	if (result == ERROR_SUCCESS && hKey)
	{
		uLONG		dataSize;
		char		name[256] = {0};

		dataSize = sizeof (name);
		result = ::RegQueryValueEx (hKey,
									"ProcessorNameString",
									NULL,
									NULL,(LPBYTE)&name, &dataSize);
		if (result == ERROR_SUCCESS)
			outProcessorType = name;
		else
			outProcessorType = "?";

		::RegCloseKey (hKey);
		hKey = 0;
	}
	else
		outProcessorType = "?";

#endif

	if(inCleanupSpaces && outProcessorType.GetLength() > 1)
	{
		VString		dbleSpaces = "  ";
		VString		space = " ";
		VIndex		foundPos;

		outProcessorType.RemoveWhiteSpaces();
		do
		{
			foundPos = outProcessorType.Find(dbleSpaces);
			if(foundPos > 0)
				outProcessorType.Replace(space, foundPos, 2);
		} while (foundPos > 0);
	}
}


Real VSystem::GetApplicationProcessorsPercentageUse()
{
	Real result = -1.0;
#if VERSIONWIN

	//First time : Initialize the counter and the query
	if(applicationProcessorCounterQuery == NULL){

		VString appName;
		if(_WIN_GetApplicationName(appName)){
			VString processusProcessorTimePercentagePath = "\\";
			processusProcessorTimePercentagePath += LookupPerfNameByIndex(PROCESS_OBJECT_INDEX);
			processusProcessorTimePercentagePath += "(";
			processusProcessorTimePercentagePath += appName;
			processusProcessorTimePercentagePath += ")\\";
			processusProcessorTimePercentagePath += LookupPerfNameByIndex(PROCESSOR_TIME_COUNTER_INDEX);
			char processusProcessorTimePercentagePathCString[1024];
			processusProcessorTimePercentagePath.ToCString(processusProcessorTimePercentagePathCString, 1024);

			PDH_STATUS pdhStatus = PdhOpenQueryH(NULL, NULL, &applicationProcessorCounterQuery);

			if (testAssert(pdhStatus == ERROR_SUCCESS)){
				pdhStatus = PdhAddCounter(applicationProcessorCounterQuery, processusProcessorTimePercentagePathCString, NULL, &applicationProcessorCounter);
				assert(pdhStatus == ERROR_SUCCESS && applicationProcessorCounter != NULL);
			}
		}
		else return -1.0;
	}

	// Collect the data
	PDH_STATUS pdhStatus = PdhCollectQueryData(applicationProcessorCounterQuery);
	if (testAssert(pdhStatus == ERROR_SUCCESS)){
		PDH_FMT_COUNTERVALUE counterValue;
		// Divide the result by 100
		pdhStatus = PdhSetCounterScaleFactor(applicationProcessorCounter, -2);
		assert(pdhStatus == ERROR_SUCCESS);
		PDH_STATUS Status = PdhGetFormattedCounterValue(applicationProcessorCounter, PDH_FMT_DOUBLE|PDH_FMT_NOCAP100, NULL, &counterValue);
		if(counterValue.CStatus == ERROR_SUCCESS){
			result = counterValue.doubleValue;
			//the result is for one processor, but we want the global value
			result = result / (Real)GetNumberOfProcessors();
			if (result < 0){
				result = -1.0;
			}
		}
	}

#elif VERSIONMAC
	//Iterate all the threads to know their cpu usage

	task_t task = current_task();
	thread_act_array_t thread_list = NULL;
	mach_msg_type_number_t threadsCount = 0;
	if(task_threads(task, &thread_list, &threadsCount) == KERN_SUCCESS){
		int totalCPUTime = 0;
		int threadIndex;
		for(threadIndex = 0; threadIndex < threadsCount; ++threadIndex){
			struct thread_basic_info t_info;
			mach_msg_type_number_t count = THREAD_BASIC_INFO_COUNT;
			if(thread_info(thread_list[threadIndex], THREAD_BASIC_INFO, (thread_info_t)&t_info, &count) == KERN_SUCCESS)
				totalCPUTime += t_info.cpu_usage;
            mach_port_deallocate(mach_task_self(), thread_list[threadIndex]); // YT 20-Nov-2013 - ACI0084878
		}
		::vm_deallocate(mach_task_self(), (vm_address_t)thread_list, sizeof(thread_array_t) * threadsCount);

		result = (Real)((Real)totalCPUTime / (Real)TH_USAGE_SCALE);
		result /= GetNumberOfProcessors();
	}

#elif VERSION_LINUX
	return XLinuxSystem::GetApplicationProcessorsPercentageUse();

#endif

	return result;
}


sLONG8 VSystem::GetPhysicalMemSize()
{
	sLONG8 result = 0;

#if VERSIONMAC
    host_basic_info_data_t hostinfo;
    mach_msg_type_number_t count = HOST_BASIC_INFO_COUNT;
	mach_port_t host = mach_host_self();
	int kerr = ::host_info( host, HOST_BASIC_INFO, (host_info_t) &hostinfo, &count);
    if (kerr)
        result = 0;
    else
        result = static_cast<sLONG8>( hostinfo.max_mem);
    mach_port_deallocate(mach_task_self(), host);

#elif VERSION_LINUX
	result=XLinuxSystem::GetPhysicalMemSize();

#elif VERSIONWIN
	MEMORYSTATUSEX	statex;
	::memset(&statex, 0, sizeof(MEMORYSTATUSEX));
	statex.dwLength = sizeof (statex);
	if (GlobalMemoryStatusEx(&statex))
	{
		result = static_cast<sLONG8>(statex.ullTotalPhys);
	}

#endif

	return result;
}

sLONG8 VSystem::GetPhysicalFreeMemSize()
{
	sLONG8 result = 0;

#if VERSIONMAC
	vm_size_t pagesize;
	mach_port_t host = mach_host_self();
	if(host_page_size (host, &pagesize) == KERN_SUCCESS){
		vm_statistics_data_t page_info;
		mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
		if(host_statistics (host, HOST_VM_INFO, (host_info_t)&page_info, &count) == KERN_SUCCESS){
			result = (sLONG8) page_info.free_count * (sLONG8) pagesize;
		}
	}
    mach_port_deallocate(mach_task_self(), host);

#elif VERSION_LINUX
	result=XLinuxSystem::GetPhysicalFreeMemSize();

#elif VERSIONWIN

	MEMORYSTATUSEX	statex;
	::memset(&statex, 0, sizeof(MEMORYSTATUSEX));
	statex.dwLength = sizeof (statex);
	if (GlobalMemoryStatusEx(&statex))
		result = statex.ullAvailPhys;

#if 0
	//First time : Initialize the counter and the query
	if(freeMemoryCounterQuery == NULL)
	{
		VString appName;
		VString freeMemoryCounterPath = "\\";
		freeMemoryCounterPath += LookupPerfNameByIndex(MEMORY_OBJECT_INDEX);
		freeMemoryCounterPath += "\\";
		freeMemoryCounterPath += LookupPerfNameByIndex(AVAILABLE_BYTES_COUNTER_INDEX);
		char freeMemoryCounterPathCString[1024];
		freeMemoryCounterPath.ToCString(freeMemoryCounterPathCString, 1024);

		PDH_STATUS pdhStatus = PdhOpenQueryH(NULL, NULL, &freeMemoryCounterQuery);

		if (testAssert(pdhStatus == ERROR_SUCCESS))
		{
			pdhStatus = PdhAddCounter(freeMemoryCounterQuery, freeMemoryCounterPathCString, NULL, &freeMemoryCounter);
			assert(pdhStatus == ERROR_SUCCESS && freeMemoryCounter != NULL);
		}
	}

	// Collect the data
	PDH_STATUS pdhStatus = PdhCollectQueryData(freeMemoryCounterQuery);
	if (testAssert(pdhStatus == ERROR_SUCCESS))
	{
		PDH_FMT_COUNTERVALUE counterValue;
		PDH_STATUS Status = PdhGetFormattedCounterValue(freeMemoryCounter, PDH_FMT_LARGE|PDH_FMT_NOCAP100, NULL, &counterValue);
		if(counterValue.CStatus == ERROR_SUCCESS)
		{
			result = (sLONG8)counterValue.largeValue;
			if (result < 0)
				result = 0;
		}
	}
#endif

#endif
	return result;
}


sLONG8 VSystem::GetApplicationPhysicalMemSize()
{
	sLONG8 result = 0;
#if VERSIONMAC
	task_t task = current_task();
	struct task_basic_info t_info;
	mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;
	int kerr = ::task_info(task, TASK_BASIC_INFO, (task_info_t)&t_info, &t_info_count);
	if (kerr == 0)
		result = static_cast<sLONG8>(t_info.resident_size);

#elif VERSION_LINUX
	return XLinuxSystem::GetApplicationPhysicalMemSize();

#elif VERSIONWIN
	return XWinSystem::GetApplicationPhysicalMemSize();

#endif
	return result;
}

void VSystem::GetProcessList(std::vector<pid> &pids)
{
	XSystemImpl::GetProcessList(pids);
}

void VSystem::GetProcessName(uLONG inProcessID, VString &outProcessName)
{
	XSystemImpl::GetProcessName(inProcessID, outProcessName);
}

void VSystem::GetProcessPath(uLONG inProcessID, VString &outProcessPath)
{
	XSystemImpl::GetProcessPath(inProcessID, outProcessPath);
}

bool VSystem::KillProcess(uLONG inProcessID)
{
#if !VERSION_LINUX	// Postponed Linux Implementation !
	return XSystemImpl::KillProcess(inProcessID);
#else
	return false;
#endif
}

bool VSystem::GetInOutNetworkStats(std::vector< Real >& outNetworkStats)
{
	bool resultsAreTrustable = false;
	outNetworkStats.clear();

#if VERSIONMAC

	static VTime lastQueryTime;

	static double lastReceivedNetworkValueDouble = 0.0;
	static double lastSentNetworkValueDouble = 0.0;
	double receivedNetworkValueDouble = 0.0;
	double sentNetworkValueDouble = 0.0;

	static size_t sysctlBufferSize;
	static char * sysctlBuffer  = NULL;

	// MIB for sysctl (Management Information Base)
	int	mib[] = { CTL_NET, PF_ROUTE, 0, 0, NET_RT_IFLIST, 0 };
	size_t newSize;

	// Get sizing info from sysctl
	if (sysctl(mib, 6, NULL, &newSize, NULL, 0) < 0) {
		return false;
	}

	if (sysctlBuffer == NULL || (newSize > sysctlBufferSize)) {
		if (sysctlBuffer != NULL) {
			free(sysctlBuffer);
		}
		sysctlBuffer = (char *)malloc(newSize);
		sysctlBufferSize = newSize;
	}

	VTime sysctlQueryTime( eInitWithCurrentTime);

	//Read data
	if (sysctl(mib, 6, sysctlBuffer, &newSize, NULL, 0) < 0) {
		return false;
	}

	char * dataEnd = sysctlBuffer + newSize;
	char * currentData = sysctlBuffer;

	// Step across the returned data
	while (currentData < dataEnd) {
		// Expecting interface data
		struct if_msghdr * ifmsg = (struct if_msghdr *)currentData;
		if (ifmsg->ifm_type == RTM_IFINFO) {
			// Its an interface data block make sure its not a loopback
			if (!(ifmsg->ifm_flags & IFF_LOOPBACK)) {
				struct sockaddr_dl * sdl = (struct sockaddr_dl *)(ifmsg + 1);
				// Only look a link layer items
				if (sdl->sdl_family == AF_LINK) {
					// Build the interface name to string so we can key off it
					VString interfaceName;
					interfaceName.FromBlock(sdl->sdl_data, sdl->sdl_nlen, VTC_UTF_8);

					if (false /*interfaceName.BeginsWith("ppp")*/) {

						// JDG : I just don't know if this will work or not will ppp, and I don't work to work on it if it already works. We need to test before trying to ask pppconfd (seems it could not work with out bytes)
						assert(false);

					} // End of PPP handling code
					else { // Not a PPP connection

						receivedNetworkValueDouble += (Real)ifmsg->ifm_data.ifi_ibytes;
						sentNetworkValueDouble += (Real)ifmsg->ifm_data.ifi_obytes;
					}
				}
			}
		}
		// Continue on
		currentData += ifmsg->ifm_msglen;
	}

	if(!lastQueryTime.IsNull()){

		VDuration duration;
		sysctlQueryTime.Substract(lastQueryTime, duration);
		if(testAssert(lastReceivedNetworkValueDouble <= receivedNetworkValueDouble) && testAssert(lastSentNetworkValueDouble <= sentNetworkValueDouble)){
			Real receivedRate = (receivedNetworkValueDouble - lastReceivedNetworkValueDouble) / duration.GetReal();
			Real sentRate = (sentNetworkValueDouble - lastSentNetworkValueDouble) / duration.GetReal();
			outNetworkStats.push_back(receivedRate * 1000);
			outNetworkStats.push_back(sentRate * 1000);
			//Just send the rates for the moment, because we can get the total on windows easily
			//outNetworkStats.push_back(receivedNetworkValueDouble);
			//outNetworkStats.push_back(sentNetworkValueDouble);
			resultsAreTrustable = true;
		}
	}

	lastQueryTime = sysctlQueryTime;
	lastReceivedNetworkValueDouble = receivedNetworkValueDouble;
	lastSentNetworkValueDouble = sentNetworkValueDouble;

#elif VERSION_LINUX
	return XLinuxSystem::GetInOutNetworkStats(outNetworkStats);

#elif VERSIONWIN

	//First time : Initialize the counter and the query
	if(networkCounterQuery == NULL){

		VString receivedNetworkPath = "\\";
		receivedNetworkPath += LookupPerfNameByIndex(NETWORK_OBJECT_INDEX);
		receivedNetworkPath += "(*)\\";
		receivedNetworkPath += LookupPerfNameByIndex(RECEIVED_BYTES_S_COUNTER_INDEX);
		char receivedNetworkPathCString[1024];
		receivedNetworkPath.ToCString(receivedNetworkPathCString, 1024);

		VString sentNetworkPath = "\\";
		sentNetworkPath += LookupPerfNameByIndex(NETWORK_OBJECT_INDEX);
		sentNetworkPath += "(*)\\";
		sentNetworkPath += LookupPerfNameByIndex(SENT_BYTES_S_COUNTER_INDEX);
		char sentNetworkPathCString[1024];
		sentNetworkPath.ToCString(sentNetworkPathCString, 1024);

		PDH_STATUS pdhStatus = PdhOpenQueryH(NULL, NULL, &networkCounterQuery);

		if (testAssert(pdhStatus == ERROR_SUCCESS)){
			pdhStatus = PdhAddCounter(networkCounterQuery, receivedNetworkPathCString, NULL, &receivedNetworkCounter);
			assert(pdhStatus == ERROR_SUCCESS && receivedNetworkCounter != NULL);
			pdhStatus = PdhAddCounter(networkCounterQuery, sentNetworkPathCString, NULL, &sentNetworkCounter);
			assert(pdhStatus == ERROR_SUCCESS && sentNetworkCounter != NULL);
		}
	}

	if(testAssert(networkCounterQuery != NULL)){
		// Collect the data
		PDH_STATUS pdhStatus = PdhCollectQueryData(networkCounterQuery);
		if (testAssert(pdhStatus == ERROR_SUCCESS)){

			//The system can have multiple network interfaces, so we do the sum of all the interfaces

			double receivedNetworkCounterValueDouble = 0.0;
			bool receivedNetworkCounterValueOK = false;
			double sentNetworkCounterValueDouble = 0.0;
			bool sentNetworkCounterValueOK = false;

			PPDH_FMT_COUNTERVALUE_ITEM items = NULL;
			DWORD nbItemsInArray = 0;

			//First call the function to determine the size of buffer required
			DWORD dwBufferSize = 0;
			pdhStatus = PdhGetFormattedCounterArray(receivedNetworkCounter, PDH_FMT_DOUBLE|PDH_FMT_NOCAP100, &dwBufferSize, &nbItemsInArray, NULL);
			if (pdhStatus == PDH_MORE_DATA)
			{
				//Call again with the newly allocated buffer
				items = reinterpret_cast<PPDH_FMT_COUNTERVALUE_ITEM>(new BYTE[dwBufferSize]);
				pdhStatus = PdhGetFormattedCounterArray(receivedNetworkCounter, PDH_FMT_DOUBLE|PDH_FMT_NOCAP100, &dwBufferSize, &nbItemsInArray, items);

				DWORD valueIndex = 0;
				for(;valueIndex < nbItemsInArray; ++ valueIndex){
					PDH_FMT_COUNTERVALUE receivedNetworkCounterValue = items[valueIndex].FmtValue;
					if(receivedNetworkCounterValue.CStatus == ERROR_SUCCESS){
						receivedNetworkCounterValueOK = true;
						receivedNetworkCounterValueDouble += receivedNetworkCounterValue.doubleValue;
					}
				}

			}

			if(items != NULL)
				delete[] items;

			items = NULL;
			nbItemsInArray = 0;

			//First call the function to determine the size of buffer required
			dwBufferSize = 0;
			pdhStatus = PdhGetFormattedCounterArray(sentNetworkCounter, PDH_FMT_DOUBLE|PDH_FMT_NOCAP100, &dwBufferSize, &nbItemsInArray, NULL);
			if (pdhStatus == PDH_MORE_DATA)
			{
				//Call again with the newly allocated buffer
				items = reinterpret_cast<PPDH_FMT_COUNTERVALUE_ITEM>(new BYTE[dwBufferSize]);
				pdhStatus = PdhGetFormattedCounterArray(sentNetworkCounter, PDH_FMT_DOUBLE|PDH_FMT_NOCAP100, &dwBufferSize, &nbItemsInArray, items);

				DWORD valueIndex = 0;
				for(;valueIndex < nbItemsInArray; ++ valueIndex){
					PDH_FMT_COUNTERVALUE sentNetworkCounterValue = items[valueIndex].FmtValue;
					if(sentNetworkCounterValue.CStatus == ERROR_SUCCESS){
						sentNetworkCounterValueOK = true;
						sentNetworkCounterValueDouble += sentNetworkCounterValue.doubleValue;
					}
				}

			}

			if(sentNetworkCounterValueOK && receivedNetworkCounterValueOK){
				outNetworkStats.push_back(receivedNetworkCounterValueDouble);
				outNetworkStats.push_back(sentNetworkCounterValueDouble);
				resultsAreTrustable = true;
			}
		}
	}

#endif
	return resultsAreTrustable;
}


//[static]
bool VSystem::AllowedToGetSystemInfo()
{
	static bool allowedToGetSystemInfo = false;
	static bool allowedToGetSystemInfoDefined = false;

	if(!allowedToGetSystemInfoDefined){
		#if VERSIONMAC
			allowedToGetSystemInfo = true;

        #elif VERSION_LINUX
			return XLinuxSystem::AllowedToGetSystemInfo();

		#elif VERSIONWIN
			DWORD bufferSize = 1024;
			TCHAR* buffer = static_cast<TCHAR*>(_alloca(bufferSize*sizeof(TCHAR)));
			PDH_STATUS pdhStatus = PdhLookupPerfNameByIndex(NULL, MEMORY_OBJECT_INDEX, buffer, &bufferSize);
			if (pdhStatus == PDH_MORE_DATA)
			{
				//Call again with the newly allocated buffer
				buffer = static_cast<TCHAR*>(_alloca(bufferSize * sizeof(TCHAR)));
				pdhStatus = PdhLookupPerfNameByIndex(NULL, MEMORY_OBJECT_INDEX, buffer, &bufferSize);
			}

			if (pdhStatus == ERROR_SUCCESS)
				allowedToGetSystemInfo = true;

		#endif
		allowedToGetSystemInfoDefined = true;
	}
	return allowedToGetSystemInfo;
}


VSize VSystem::GetVMPageSize()
{
	static VSize sPageSize = 0;

	if (sPageSize == 0)
	{
	#if VERSIONWIN

		SYSTEM_INFO infoReturn[1];
		::GetSystemInfo(infoReturn);
		sPageSize = infoReturn->dwPageSize;

	#elif VERSIONMAC

		sPageSize = vm_page_size;

    #elif VERSION_LINUX

        sPageSize=XLinuxSystem::GetVMPageSize();

	#endif
	}
	return sPageSize;
}


VSize VSystem::RoundUpVMPageSize( VSize inNbBytes)
{
	VSize pageSize = GetVMPageSize();

	return (inNbBytes + pageSize - 1) & ~(pageSize - 1);
}


void *VSystem::VirtualAllocPhysicalMemory( VSize inNbBytes, const void *inHintAddress, bool *outCouldLock)
{
	void *ptr = NULL;
	bool locked = false;

	inNbBytes = RoundUpVMPageSize( inNbBytes);

#if VERSIONWIN
	DWORD err = 0;

	ptr = ::VirtualAlloc( (LPVOID) inHintAddress, inNbBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (ptr != NULL)
	{
		// allocation done.
		// now try to lock it.

		SIZE_T minWSize, maxWSize;
		HANDLE hProcess = ::GetCurrentProcess();
		BOOL ok = ::GetProcessWorkingSetSize( hProcess, &minWSize, &maxWSize);
		if (testAssert(ok))
		{
			minWSize += inNbBytes;
			maxWSize += inNbBytes;
			xbox_assert( minWSize < maxWSize);
			ok = ::SetProcessWorkingSetSize( hProcess, minWSize, maxWSize);
			if (ok)
			{
				ok = ::VirtualLock( ptr, inNbBytes);
				if (ok)
				{
					locked = true;
				}
				else
				{
					minWSize -= inNbBytes;
					maxWSize -= inNbBytes;

					BOOL ok2 = ::SetProcessWorkingSetSize( hProcess, minWSize, maxWSize);
					xbox_assert( ok2);
				}
			}
			else
			{
				err = ::GetLastError();
			}
		}
		else
		{
			err = ::GetLastError();
		}
	}

#elif VERSIONMAC

	vm_address_t address = (vm_address_t) inHintAddress;
	vm_size_t size = round_page( inNbBytes);		//round up to a page boundary (round_page is a macro from mach_init.h

	int flags = (inHintAddress == NULL) ? VM_FLAGS_ANYWHERE : VM_FLAGS_FIXED;

	kern_return_t machErr=0;
#if ARCH_32
	machErr = ::task_wire( ::mach_task_self(), true);
#endif
	if (machErr == 0)
	{
		machErr = ::vm_allocate( ::mach_task_self(), &address, size, flags);
		if (machErr == 0)
			locked = true;

#if ARCH_32
		int machErr2 = ::task_wire( ::mach_task_self(), false);
		xbox_assert( machErr2 == 0);
#endif
	}

	// if failed, retry without asking for wired memory
	if (machErr != 0)
	{
		machErr = ::vm_allocate( ::mach_task_self(), &address, size, flags);
	}

	if (machErr != 0)
		address = 0;

	ptr = (void *) address;

#elif VERSION_LINUX
	return XLinuxSystem::VirtualAllocPhysicalMemory(inNbBytes, inHintAddress, outCouldLock);

#endif

	xbox_assert( !locked || (ptr != NULL) );

	// the caller MUST remember the locked status to pass it back in VirtualFreePhysicalMemory.
	// so outCouldLock CAN'T be NULL.
	*outCouldLock = locked;

	return ptr;
}


void* VSystem::VirtualAlloc( VSize inNbBytes, const void *inHintAddress)
{
	void* ptr = NULL;
	inNbBytes = RoundUpVMPageSize( inNbBytes);

#if VERSIONWIN

	ptr = ::VirtualAlloc( (LPVOID) inHintAddress, inNbBytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	DWORD err = ::GetLastError();

	xbox_assert( (ptr != NULL) || (err != 0) );

#elif VERSIONMAC

	vm_address_t address = (vm_address_t) inHintAddress;
	int flags = (inHintAddress == NULL) ? VM_FLAGS_ANYWHERE : VM_FLAGS_FIXED;
	kern_return_t machErr = ::vm_allocate( mach_task_self(), &address, inNbBytes, flags);
	if (machErr != 0)
		address = 0;

	ptr = (void *) address;

#elif VERSION_LINUX

    ptr=XLinuxSystem::VirtualAlloc(inNbBytes, inHintAddress);

#endif

	return ptr;
}


void VSystem::VirtualFree( void* inBlock, VSize inNbBytes, bool inPhysicalMemory)
{
	inNbBytes = RoundUpVMPageSize( inNbBytes);

#if VERSIONWIN

	// if memmory was locked, first unlock it
	if (inPhysicalMemory)
	{
		BOOL ok2 = ::VirtualUnlock( inBlock, inNbBytes);
		xbox_assert( ok2);
	}

	BOOL ok = ::VirtualFree( inBlock, inNbBytes, MEM_DECOMMIT);

	if (ok)
		ok = ::VirtualFree( inBlock, 0, MEM_RELEASE);

	if (ok)
	{
		// if memmory was locked, shrink our working set after successful deallocation
		if (inPhysicalMemory)
		{
			SIZE_T minWSize, maxWSize;
			HANDLE hProcess = ::GetCurrentProcess();
			if (testAssert( ::GetProcessWorkingSetSize( hProcess, &minWSize, &maxWSize)))
			{
				minWSize -= inNbBytes;
				maxWSize -= inNbBytes;
				BOOL OK = ::SetProcessWorkingSetSize( hProcess, minWSize, maxWSize);
			}
		}
	}
	else
	{
		vThrowNativeError(::GetLastError());
	}

#elif VERSIONMAC

	kern_return_t machErr = ::vm_deallocate( mach_task_self(), (vm_address_t)inBlock, inNbBytes);
	xbox_assert( machErr == 0);

#elif VERSION_LINUX

    return XLinuxSystem::VirtualFree(inBlock, inNbBytes, inPhysicalMemory);

#endif
}


bool VSystem::VirtualQuery( const void *inAddress, VSize *outSize, VMStatus *outStatus, const void **outBaseAddress)
{
	VSize size = 0;
	VMStatus status = 0;
	const void *baseAddress = 0;

	#if VERSIONWIN
	MEMORY_BASIC_INFORMATION info = {0};
	SIZE_T infoSize = ::VirtualQuery( inAddress, &info, sizeof(info));

	if (info.State == MEM_FREE)
		status |= VMS_FreeMemory;

	size = info.RegionSize;
	baseAddress = info.BaseAddress;

	#elif VERSIONMAC

	vm_address_t address = (vm_address_t) inAddress;
	vm_size_t mach_size;
	vm_region_basic_info_64 info;
	mach_msg_type_number_t info_count = VM_REGION_SUBMAP_INFO_COUNT;
	mach_port_t objectName = 0;
	kern_return_t machErr = ::vm_region_64( ::mach_task_self(), &address, &mach_size, VM_REGION_BASIC_INFO_64, (vm_region_info_t) &info, &info_count, &objectName);

	if (machErr == KERN_SUCCESS)
	{
		// if the passed adress was in a free region, vm_region_64 returns the next one.
		if (address > (vm_address_t) inAddress)
		{
			status |= VMS_FreeMemory;
			size = (vm_address_t) address - (vm_address_t) inAddress;
			baseAddress = inAddress;
		}
		else
		{
			size = mach_size;
			baseAddress = (const void *) address;
		}
	}

    #elif VERSION_LINUX
    return false;   // Postponed Linux Implementation !
	#endif

	if (outSize)
		*outSize = size;
	if (outStatus)
		*outStatus = status;
	if (outBaseAddress)
		*outBaseAddress = baseAddress;

	return size != 0;
}

VSize VSystem::VirtualMemoryUsedSize()
{
	VSize result = 0;

#if VERSIONWIN
	MEMORYSTATUSEX	statex;
	::memset(&statex, 0, sizeof(MEMORYSTATUSEX));
	statex.dwLength = sizeof (statex);
	if(GlobalMemoryStatusEx(&statex))
	{
		result = statex.ullTotalVirtual - statex.ullAvailVirtual;
	}

#elif VERSIONMAC
	task_t task = current_task();
	struct task_basic_info t_info;
	mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;
	int kerr = ::task_info(task, TASK_BASIC_INFO, (task_info_t)&t_info, &t_info_count);
	if (kerr == 0)
		result = t_info.virtual_size;

#elif VERSION_LINUX
	result=XLinuxSystem::VirtualMemoryUsedSize();

#endif

	return result;
}


void VSystem::GetLoginUserName( VString& outName)
{
	#if VERSIONMAC

	#if 1
	CFStringRef cfName = CSCopyUserName( false);
	if (testAssert( cfName != NULL))
	{
		outName.MAC_FromCFString( cfName);
		::CFRelease( cfName);
	}

	#else
	const char *name = ::getlogin();

	// that's utf-8
	if (testAssert( name != NULL))
		outName.FromBlock( name, ::strlen( name), VTC_UTF_8);
	else
		outName.Clear();
	#endif

	#elif VERSION_LINUX
	return XLinuxSystem::GetLoginUserName(outName);

    #elif VERSIONWIN

	UniChar	name[UNLEN+1];
	DWORD	buffLen = UNLEN+1;
	if (testAssert(GetUserNameW( name, &buffLen)))
		outName.FromBlock( name, (buffLen-1)*sizeof(UniChar), VTC_UTF_16);
	else
		outName.Clear();
	#endif
}


void VSystem::GetHostName( VString& outName)
{
	#if VERSIONMAC

	CFStringRef cfName = CSCopyMachineName();
	if (testAssert( cfName != NULL))
	{
		outName.MAC_FromCFString( cfName);
		::CFRelease( cfName);
	}

	#elif VERSION_LINUX
	return XLinuxSystem::GetHostName(outName);

    #elif VERSIONWIN

	UniChar	name[MAX_COMPUTERNAME_LENGTH+1];
	DWORD	buffLen = MAX_COMPUTERNAME_LENGTH+1;
	if (testAssert( ::GetComputerNameW( name, &buffLen)))
		outName.FromBlock( name, buffLen*sizeof(UniChar), VTC_UTF_16);
	else
		outName.Clear();
	#endif
}


bool VSystem::SearchExecutablePath( const VString& inExecutableName, VFilePath& outPath)
{
	return XSystemImpl::SearchExecutablePath( inExecutableName, outPath);
}


void VSystem::DemangleSymbol( const char *inMangledSymbol, VString& outDemangledSymbol)
{
#if VERSIONMAC || VERSION_LINUX
	char buffer[1024];
	int status = 0;
/*
status is set to one of the following values:
0: The demangling operation succeeded.
-1: A memory allocation failure occurred.
-2: MANGLED_NAME is not a valid name under the C++ ABI mangling rules.
-3: One of the arguments is invalid
*/
	size_t size = sizeof( buffer);
	char *name = abi::__cxa_demangle( inMangledSymbol, buffer, &size, &status);
	if (status == 0)
		outDemangledSymbol.FromCString( name);
	else
		outDemangledSymbol.FromCString( inMangledSymbol);
#elif VERSIONWIN
	//	VStackCrawl::Init() called from VDebugger::Init has called SymInitialize()b
	char buffer[1024];
	if (UnDecorateSymbolName( inMangledSymbol, buffer, sizeof(buffer), UNDNAME_COMPLETE))
		outDemangledSymbol.FromCString( buffer);
	else
		outDemangledSymbol.FromCString( inMangledSymbol);
#endif
}


#if VERSIONDEBUG

sLONG8 VSystem::debug_profiler_count[20];

sLONG8 VSystem::debug_GetProfilerCount(sLONG CounterNumber)
{
	if (CounterNumber>=0 && CounterNumber<20)
		return debug_profiler_count[CounterNumber];
	else
		return 0;
}


void VSystem::debug_ResetProfilerCount(sLONG CounterNumber)
{
	if (CounterNumber>=0 && CounterNumber<20)
		debug_profiler_count[CounterNumber] = 0;
}


void VSystem::debug_AddToProfilerCount(sLONG CounterNumber, sLONG8 inProfilingCounter)
{
	if (CounterNumber>=0 && CounterNumber<20)
		debug_profiler_count[CounterNumber] += inProfilingCounter;
}


void VSystem::debug_SubToProfilerCount(sLONG CounterNumber, sLONG8 inProfilingCounter)
{
	if (CounterNumber>=0 && CounterNumber<20)
		debug_profiler_count[CounterNumber] -= inProfilingCounter;
}


#endif

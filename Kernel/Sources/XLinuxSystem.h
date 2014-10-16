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
#ifndef __XLinuxSystem__
#define __XLinuxSystem__

#include "Kernel/Sources/VKernelTypes.h"



BEGIN_TOOLBOX_NAMESPACE


class VString;
//class VValueBag;

typedef pid_t pid;


namespace XLinuxSystem 
{
	//jmo - Linux specific. Should be called once on XBox init (not thread safe). Inits a reference timestamp
	//		used by GetCurrentTime (and ticks) to produce a value from init time, rather than a value from EPOCH.
	//		This smaller value is IMHO easier to interpret, and less prone to overflows in sLONG ! This behavior
	//      is also closer to windows behavior.
	void	InitRefTime();

	void	Beep();

	void	GetOSVersionString(VString &outStr);

	sLONG	Random(bool inComputeFast);

	bool	DisplayNotification(const VString& inTitle, const VString& inMessage, EDisplayNotificationOptions inOptions, ENotificationResponse *outResponse);
	bool	IsSystemVersionOrAbove(SystemVersion inSystemVersion);

	VSize   GetVMPageSize();
	void*   VirtualAlloc(VSize inNbBytes, const void *inHintAddress);
	void*	VirtualAllocPhysicalMemory(VSize inNbBytes, const void *inHintAddress, bool *outCouldLock);
	void	VirtualFree(void* inBlock, VSize inNbBytes, bool inPhysicalMemory);

	void	LocalToUTCTime(sWORD ioVals[7]);
	void	UTCToLocalTime(sWORD ioVals[7]);

	void	GetSystemUTCTime(sWORD outVals[7]);
	void	GetSystemLocalTime(sWORD outVals[7]);

	void	UpdateGMTOffset(sLONG* outOffset, sLONG* outOffsetWithDayLight);

	uLONG	GetCurrentTicks();
	uLONG	GetCurrentTimeStamp();

	void	GetProfilingCounter(sLONG8& outVal);
	sLONG8  GetProfilingFrequency();

	void	Delay(sLONG inMilliseconds);

	sLONG	GetNumberOfProcessors();
	bool	GetProcessorsPercentageUse(Real& outPercentageUse);
	void	GetProcessorTypeString(VString& outProcessorType, bool inCleanupSpaces);
	Real	GetApplicationProcessorsPercentageUse();

	sLONG8	GetPhysicalMemSize();
	sLONG8	GetPhysicalFreeMemSize();

	sLONG8	GetApplicationPhysicalMemSize();
	VSize	GetApplicationVirtualMemSize();
	VSize	VirtualMemoryUsedSize();
	
	bool	GetInOutNetworkStats(std::vector<Real>& outNetworkStats);
	bool	AllowedToGetSystemInfo();

	void	GetLoginUserName( VString& outName); //todo
	void	GetHostName( VString& outName); //todo
	void	GetProcessList(std::vector<pid> &outPIDs); //todo

	//bool	VirtualQuery( const void *inAddress, VSize *outSize, VMStatus *outStatus, const void **outBaseAddress); // duno
	//bool	AllowedToGetSystemInfo();	// duno

	//bool	GetInOutNetworkStats(std::vector< Real >& outNetworkStats); // Ouch !
	//void	DemangleSymbol( const char *inMangledSymbol, VString& outDemangledSymbol); // Ouch !

};


#define XSystemImpl XLinuxSystem



END_TOOLBOX_NAMESPACE

#endif

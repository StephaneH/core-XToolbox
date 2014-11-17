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
#ifndef __VSystem__
#define __VSystem__

#include "Kernel/Sources/VKernelTypes.h"

#if VERSIONMAC
#include "XMacSystem.h"
#elif VERSION_LINUX
#include "XLinuxSystem.h"
#elif VERSIONWIN
#include "XWinSystem.h"
#endif


BEGIN_TOOLBOX_NAMESPACE

// Needed declarations
class VString;
class VFilePath;

class XTOOLBOX_API VSystem
{
public:
	static	bool			Init ();
	static	void			DeInit ();

	// Standard system accessors
	static	sLONG			GetDoubleClicOffsetWidth();
	static	sLONG			GetDoubleClicOffsetHeight();

	static	sLONG			GetDragOffset ();

	static	uLONG			GetDoubleClicTicks ();
	static	uLONG			GetCaretTicks ();
	static	uLONG			GetTipsDelayTicks ();
	static	uLONG			GetTipsTimeoutTicks ();
	static	uLONG			GetSelectDelayTicks ();

	// system time in milliseconds since the startup of the process
	static	uLONG			GetCurrentTime();

	// system time in ticks (60 ticks = 1 second).
	// Convenient for checking periodic actions.
	static	uLONG			GetCurrentTicks();

	// Measure the absolute difference between current ticks and specified ticks.
	static	uLONG			GetCurrentDeltaTicks( uLONG inBaseTicks);

	// Highest resolution available. For profiling.
	static	void			GetProfilingCounter( sLONG8& outVal);
	// Count per seconds
	static	sLONG8			GetProfilingFrequency();

	// Date & time accessors
	static	void			GetSystemUTCTime (sWORD outVals[7]);	// Format: year, month, day, h, min, s, ms
	static	void			GetSystemLocalTime (sWORD outVals[7]);

	// returns gmt offfsets in seconds
	static	sLONG			GetGMTOffset( bool inIncludingDaylightOffset);
	static	void			UpdateGMTOffset();

	// call by the window proc or observer when the sytem date change
	static	void			SystemDateTimeChanged();

	static	void			LocalToUTCTime (sWORD ioVals[7]);
	static	void			UTCToLocalTime (sWORD ioVals[7]);

	static	void			SetCenturyPivotYear (sWORD inYear);
	static	sWORD			GetCenturyPivotYear ();

	static	void			GetLoginUserName( VString& outName);
	static	void			GetHostName( VString& outName);

	// System environment
	static	sLONG			GetNumberOfProcessors();
	static	bool			GetProcessorsPercentageUse(Real& outPercentageUse); // call AllowedToGetSystemInfo before
	// cleanupSpaces == true => RemoveWhiteSpaces() + replace all multi-spaces by a single one
	static	void			GetProcessorTypeString(VString& outProcessorType, bool inCleanupSpaces = true);
	static	Real			GetApplicationProcessorsPercentageUse();
	static	VSize			GetVMPageSize();
	static	VSize			RoundUpVMPageSize( VSize inNbBytes);

	// physical mem may be larger than 4Go even on a 32bit system where VSize is 32bits.

	static	sLONG8			GetPhysicalMemSize(); // call AllowedToGetSystemInfo before
	static	sLONG8			GetPhysicalFreeMemSize(); // call AllowedToGetSystemInfo before
	static	sLONG8			GetApplicationPhysicalMemSize(); // call AllowedToGetSystemInfo before

	static  void		    GetProcessList(std::vector<pid> &pids);

	// On Windows platform, running 32-bit and trying to get information from a 64-bit process, will fail.
	// If running a 64-bit process, then it is possible to get information from both 32-bit and 64-bit processes.
	// On Linux, the returned name or path can be empty even if the process really exists (not enough permisions
	// is usually the reason)

	static  void			GetProcessName(uLONG intProcessID, VString &outProcessName);
	static  void			GetProcessPath(uLONG intProcessID, VString &outProcessPath);

	static  bool			KillProcess(uLONG inProcessID);

	//Network
	//1: bytes received/s , 2: bytes sent/s
	// call AllowedToGetSystemInfo before
	static	bool			GetInOutNetworkStats(std::vector< Real >& outNetworkStats);

	/** @brief	On Windows, you need Admin rights to get some system information.
		@returns Always true on Mac. true if you have admin rights or if you are member of the Journal Performance group on Windows*/
	static  bool			AllowedToGetSystemInfo();

	// User notification.
	// available even in non-ui mode (uses separate gui process).
	// returns true if displayed successfully.
	// if outResponse is not NULL, DisplayNotification will wait for a response.
	static	bool			DisplayNotification( const VString& inTitle, const VString& inMessage, EDisplayNotificationOptions inOptions, ENotificationResponse *outResponse = NULL);

	// System version
	// IsSystemVersionOrAbove is the preferred way to check for a system version.
	static	bool			IsSystemVersionOrAbove( SystemVersion inSystemVersion);

#if VERSIONWIN
	static	bool			IsEightOne()					{ return IsSystemVersionOrAbove( WIN_EIGHT_ONE); }
	static	bool			IsEight()							{ return IsSystemVersionOrAbove( WIN_EIGHT); }
	static	bool			IsSeven()							{ return IsSystemVersionOrAbove( WIN_SEVEN); }
	static	bool			IsXP()								{ return IsSystemVersionOrAbove( WIN_XP); }
	static	bool			IsVista ()							{ return IsSystemVersionOrAbove( WIN_VISTA); }
	static	void			WIN_SetIDList(void* inList);
#endif

	static	void			GetOSVersionString( VString& outStr);

#if VERSIONMAC
	// don't use MAC_GetSystemVersionDecimal. Use IsSystemVersionOrAbove instead.
	static	sLONG			MAC_GetSystemVersionDecimal();
#endif

	// Memory support
	static	void*			VirtualAlloc( VSize inNbBytes, const void *inHintAddress);
	static	void*			VirtualAllocPhysicalMemory( VSize inNbBytes, const void *inHintAddress, bool *outCouldLock);
	static	void			VirtualFree( void* inBlock, VSize inNbBytes, bool inPhysicalMemory);
	static	bool			VirtualQuery( const void *inAddress, VSize *outSize, VMStatus *outStatus, const void **outBaseAddress);
	static  VSize			VirtualMemoryUsedSize();

	// Utilities
	static	sLONG			Random( bool inComputeFast = true );
	static	void			Beep();

	// Delay (non-blocking for low-level system tasks).
	// Do not use with values > 1s (use VTask instead).
	static	void			Delay( sLONG inMilliseconds);

	/*!
	** \brief Search an executable by name on the system.
	**
	** It uses various heuristics to try to locate the best match.
	** It's usually what a shell would find if the user type only the executable name.
	**
	** On windows the specified name should not end with '.exe',
	** On mac the specified name should not end with '.app'.
	**
	** \warning On mac, SearchExecutablePath may return a folder path when a .app bundle is found.
	**
	** \return true if it could find a matching executable.
	*/
	static	bool			SearchExecutablePath( const VString& inExecutableName, VFilePath& outPath);

	// Profiler support
#if VERSIONDEBUG
	static	sLONG8	debug_GetProfilerCount (sLONG CounterNumber);
	static	void	debug_ResetProfilerCount (sLONG CounterNumber);
	static	void	debug_AddToProfilerCount (sLONG CounterNumber, sLONG8 inProfilingCounter);
	static	void	debug_SubToProfilerCount (sLONG CounterNumber, sLONG8 inProfilingCounter);
#endif

	// demangles a c++ symbol name such like std::type_info::name()
	static	void			DemangleSymbol( const char *inMangledSymbol, VString& outDemangledSymbol);

protected:

	static	sWORD	sCenturyPivotYear;

	static	bool	sGMTOffsetValid;
	static	sLONG	sGMTOffset;
	static	sLONG	sGMTOffsetWithDayLight;
	
#if VERSIONMAC
	static	void	_SetupDstTransitionTimer(CFAbsoluteTime inNextDstTransition);
#endif

#if VERSIONWIN
	static	void*	fIDList;
#endif

#if VERSIONDEBUG
	static	sLONG8	debug_profiler_count[20];
#endif
};


inline uLONG ABSTICKS(uLONG inT0)
{
	uLONG	t = VSystem::GetCurrentTicks();
	return (t > inT0) ? (t - inT0) : (inT0 - t);
}

END_TOOLBOX_NAMESPACE

#endif

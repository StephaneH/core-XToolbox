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
#ifndef __VKernelFlags__
#define __VKernelFlags__

// sometimes we directly include VKernelFlags, so we must take care of alignment
#if _WIN32
	#pragma pack( push, 8 )
#else
	#pragma options align = power
#endif

#include "Kernel/Sources/VPlatform.h"
#include "Kernel/Sources/VKernelExport.h"

#if VERSIONDEBUG
#define VERSIONDEBUG_PROFILE	0
#else
#define VERSIONDEBUG_PROFILE	0
#endif


// Usefull compile style-related macros
#ifndef UNUSED

#if VERSIONDEBUG
	#define UNUSED(_var_)	(void)_var_;
#else
	#define UNUSED(_var_)
#endif

#endif	// UNUSED


// Namespace automation macro
#ifndef USE_TOOLBOX_NAMESPACE
	#define USE_TOOLBOX_NAMESPACE	1
#endif

#if USE_TOOLBOX_NAMESPACE
	#define XBOX ::xbox
	#define BEGIN_TOOLBOX_NAMESPACE namespace xbox {
	#define END_TOOLBOX_NAMESPACE }
	#define USING_TOOLBOX_NAMESPACE using namespace xbox;
#else
	#define XBOX
	#define BEGIN_TOOLBOX_NAMESPACE
	#define END_TOOLBOX_NAMESPACE
	#define USING_TOOLBOX_NAMESPACE
#endif


// Feature specific macros
//
// Flag to enable old obsolete APIs
#ifndef USE_OBSOLETE_API
	#define USE_OBSOLETE_API	1
#endif


// Flag to implement Macintosh resource files
#ifndef USE_MAC_RSRCFORK
	#define USE_MAC_RSRCFORK VERSIONWIN
#endif


// Flag to enable C++ mangling for componentlib entry points
#ifndef USE_UNMANGLED_EXPORTS
	#define	USE_UNMANGLED_EXPORTS	VERSIONMAC
#endif


// Flag to enable system's memory mgr
#if XTOOLBOX_AS_STANDALONE || defined(VERSION_LINUX)
	#define USE_NATIVE_MEM_MGR	1
#else
	#define USE_NATIVE_MEM_MGR	0
#endif


// Flag to enable cpp allocation check for release code
#ifndef USE_MEMORY_CHECK_FOR_RELEASE_CODE
	#define USE_MEMORY_CHECK_FOR_RELEASE_CODE	0
#endif


// Flag to enable leaks checking
#ifndef USE_CHECK_LEAKS
	#define USE_CHECK_LEAKS VERSIONDEBUG
#endif


#ifndef WITH_ASSERT
	#define WITH_ASSERT	VERSIONDEBUG
#endif

#ifndef WITH_DEBUGMSG
	#define WITH_DEBUGMSG	VERSIONDEBUG
#endif

// ICU configuration
#if VERSION_LINUX
	#define USE_ICU     1
    #define WITH_ICU    1
	#define USE_MECAB	0
//#define icu icu_3_8

#elif XTOOLBOX_AS_STANDALONE
	#define USE_ICU 0
	#define USE_MECAB 0
#else
	#define USE_ICU 1
	#define USE_MECAB 1
	// exclude some unneeded parts (should match icu project settings)
	#if VERSIONMAC
	#define UCONFIG_NO_LEGACY_CONVERSION 1
	#define UCONFIG_NO_FORMATTING 1
	#endif
#endif

#if !VERSION_LINUX
	// VMemoryMgr is not thread safe.
	#define WITH_VMemoryMgr 1
#else
    #define WITH_VMemoryMgr     0
#endif

#if _WIN32
	#pragma pack( pop )
#else
	#pragma options align = reset
#endif

//Isolate ongoing dev on various API

#define WITH_CRYPTO_VERIFIER_API 1

#define WITH_NEW_XTOOLBOX_GETOPT 1

//Daemonize and new Getopt play very well together !
#if VERSION_LINUX
	#define WITH_DAEMONIZE 1
#else
	#define WITH_DAEMONIZE 0
#endif

//VXMLHttpRequest uses VHTTPClient instead of cURL
#define WITH_NATIVE_HTTP_BACKEND 1

//DB4D implements new activity collectors to retrieve specific info in Real Time Monitor
#define WITH_RTM_DETAILLED_ACTIVITY_INFO 1

#endif

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
#ifndef __XOSXPlatform__
#define __XOSXPlatform__

	// Platform & compiler macros
	#define VERSIONMAC		1
	#define VERSIONWIN		0
	#define VERSION_LINUX   0
	#define VERSION_DARWIN	0

	//Penser à tester pour les architectures 64 bits
	//Test de LP64 pour ppc64 et x86_64
	#if defined (__LP64__) || (_LP64) || ( __WORDSIZE == 64)
		#define ARCH_32			0
		#define ARCH_64			1
	#else
		#define ARCH_32			1
		#define ARCH_64			0
	#endif

#if defined(__GNUC__) && (defined(__APPLE_CPP__) || defined(__APPLE_CC__) || defined(__MACOS_CLASSIC__))
	#define COMPIL_VISUAL	0
	#define COMPIL_GCC	1
#endif

#if defined(__clang__)
	#define COMPIL_CLANG	1
#else
	#define COMPIL_CLANG	0
#endif

// Guess VERSIONDEBUG if not defined yet
#ifndef VERSIONDEBUG
#if COMPIL_GCC
	#if __OPTIMIZE__
		#define VERSIONDEBUG	0
	#else
		#define VERSIONDEBUG	1
	#endif
#endif
#endif	// VERSIONDEBUG

// define NDEBUG to disable assert macro in ANSI header file
#if !VERSIONDEBUG
	#define NDEBUG
#endif

	// Minimal supported OS
	// No max means allows compiling with latest (see AvailabilityMacros.h)
#ifndef MAC_OS_X_VERSION_MIN_REQUIRED
	#define	MAC_OS_X_VERSION_MIN_REQUIRED	MAC_OS_X_VERSION_10_3
#elif MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_3
	#warning "MAC_OS_X_VERSION_MIN_REQUIRED is less than XToolbox requirement"
#endif

	// Processor & memory macros
#if defined(__i386__)
	#ifndef BIGENDIAN
		#define BIGENDIAN		0
	#endif
	#ifndef SMALLENDIAN
		#define SMALLENDIAN		1
	#endif
	#define ARCH_386		1
	#define ARCH_POWER		0
#elif __LP64__
	#ifndef BIGENDIAN
		#define BIGENDIAN		0
	#endif
	#ifndef SMALLENDIAN
		#define SMALLENDIAN		1
	#endif
	#define ARCH_386		1
	#define ARCH_POWER		0
#else
	#error "Unkown architecture or not supported"
#endif

	#define VTC_WCHAR	VTC_UTF_32_RAW
	#define WCHAR_IS_UNICHAR	0

	// Compiler relative macros & settings
#if defined(__GNUC__)
	// By default, all symbols are exported using MachO but you may use 'nmedit' to remove some symbols
	#define EXPORT_API		__attribute__((visibility("default")))
	#define IMPORT_API		__attribute__((visibility("default")))
	// GCC 4.0.1 does not support yet the visibility tag on instantiated templates
	#define EXPORT_TEMPLATE_API
	#define IMPORT_TEMPLATE_API
	#define EXPORT_UNMANGLED_API(_return_type_)	extern "C" __attribute__((visibility("default"))) _return_type_
#endif

	// Required includes
#if COMPIL_GCC
	#include <stdbool.h>
	#include <stdlib.h>
	#include <unistd.h>
	#include <sys/stat.h>
	#include <sys/param.h>
	#include <sys/mount.h>
	#include <sys/malloc.h>
	#include <pthread.h>
	#include <semaphore.h>
	#include <iterator>
	#include <typeinfo>
#endif

	#include <ctype.h>
	#include <errno.h>
	#include <fcntl.h>
	#include <float.h>
	#include <limits.h>
	#include <locale.h>
	#include <iso646.h>

#ifndef __SANE__
	#include <math.h>
#endif

	#include <setjmp.h>
	#include <signal.h>
	#include <stdarg.h>
	#include <stddef.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <time.h>

// STL
	#include <vector>

#if COMPIL_GCC
	#include <ext/hash_map>
	// namespace for non-standard yet hash_map
	#define STL_HASH_MAP ::__gnu_cxx
	#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
	#pragma GCC diagnostic ignored "-Wswitch-enum"	// warning: 50 enumeration values not handled in switch:
#endif

#if COMPIL_CLANG != 0
// temporary disabling of the warning `unused variable` to help developpers to fix real warnings
#pragma clang diagnostic ignored "-Wunused-variable"
// do not warn on unknown pragmas (especially when coming from visual studio)
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#endif

#if COMPIL_GCC
    #undef Free
#endif

	#ifndef TARGET_API_MAC_CARBON
	#define TARGET_API_MAC_CARBON	1
	#endif

	#ifndef PM_USE_SESSION_APIS
	#define PM_USE_SESSION_APIS		0
	#endif

	#ifndef __CF_USE_FRAMEWORK_INCLUDES__
	#define __CF_USE_FRAMEWORK_INCLUDES__
	#endif
	//#define __NOEXTENSIONS__	// Sane extensions in fp.h are needed for x80 convertions

	#ifndef _MSL_ISO646_H
	#define _MSL_ISO646_H	// ciso646.h
	#endif

	#ifndef _MSL_TGMATH_H
	#define _MSL_TGMATH_H	// tgmath.h
	#endif

	#ifndef _MSL_WCHAR_H
	#define _MSL_WCHAR_H	// wchar.h
	#endif

	#ifndef _MSL_WCTYPE_H
	#define _MSL_WCTYPE_H	// wctype.h
	#endif

	//#define _MSL_NO_IO
	//#define __STL_USE_NEW_IOSTREAMS

	#include <CoreServices/CoreServices.h>

#endif

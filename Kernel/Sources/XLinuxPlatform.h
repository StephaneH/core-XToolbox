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
#ifndef __XLinuxPlatform__
#define __XLinuxPlatform__


// Platform & compiler macros
#define VERSIONMAC      0
#define VERSIONWIN      0
#define VERSION_LINUX   1
#define VERSION_DARWIN  0

#define ARCH_386        1
#define ARCH_POWER      0
#define BIGENDIAN       0
#define SMALLENDIAN     1

#if defined (__x86_64__)
	#define ARCH_32     0
	#define ARCH_64     1
#else
	#define ARCH_32     1
	#define ARCH_64     0
#endif

#if defined(__GNUC__)
	#define COMPIL_VISUAL       0
	#define COMPIL_CODEWARRIOR  0
	#define COMPIL_GCC          1
	#else
	#error Need a GCC compatible compiler !
#endif

#if defined(__clang__)
	#define COMPIL_CLANG	1
#else
	#define COMPIL_CLANG	0
#endif

#ifndef VERSIONDEBUG
	#ifdef NDEBUG
	#define VERSIONDEBUG   0
	#else
	#define VERSIONDEBUG   1
	#endif
#endif


//Pas sur pour ceux la !
#define VTC_WCHAR          VTC_UTF_32_RAW
#define WCHAR_IS_UNICHAR   0

// Compiler relative macros & settings
#if defined(__GNUC__)
	#define EXPORT_API __attribute__((visibility("default")))
	#define IMPORT_API __attribute__((visibility("default")))
	// GCC 4.0.1 does not support yet the visibility tag on instantiated templates
	#define EXPORT_TEMPLATE_API
	#define IMPORT_TEMPLATE_API
	#define EXPORT_UNMANGLED_API(_return_type_) extern "C" __attribute__((visibility("default"))) _return_type_
#endif

#if COMPIL_GCC
	#include <unistd.h>
	#include <typeinfo>
	#include <inttypes.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <limits.h>
#include <locale.h>

#ifndef __SANE__
	#include <math.h>
#endif

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
// STL
#include <vector>


#if COMPIL_CLANG
// temporary disabling of the warning `unused variable` to help developpers to fix real warnings
#pragma clang diagnostic ignored "-Wunused-variable"
// do not warn on unknown pragmas (especially when coming from visual studio)
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#endif


#if COMPIL_GCC
	#if COMPIL_CLANG
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-W#warnings" // only for hash_map, which is deprecated
	#endif

	#include <ext/hash_map>
	// namespace for non-standard yet hash_map
	#define STL_HASH_MAP ::__gnu_cxx

	#if COMPIL_CLANG
	#pragma clang diagnostic pop
	#endif
#endif
#include <algorithm>

#if COMPIL_GCC
	#undef Free
#endif

//Pas sur pour la suite

#ifndef PM_USE_SESSION_APIS
	#define PM_USE_SESSION_APIS  0
#endif

#ifndef _MSL_ISO646_H
	#define _MSL_ISO646_H   // ciso646.h
#endif

#ifndef _MSL_TGMATH_H
	#define _MSL_TGMATH_H   // tgmath.h
#endif

#ifndef _MSL_WCHAR_H
	#define _MSL_WCHAR_H    // wchar.h
#endif

#ifndef _MSL_WCTYPE_H
	#define _MSL_WCTYPE_H   // wctype.h
#endif


// TRUE / FALSE for compatibility with other OSes
#ifndef TRUE
	#define TRUE   1
#endif
#ifndef FALSE
	#define FALSE  0
#endif


#endif

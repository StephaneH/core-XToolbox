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
#ifndef __XWinPlatform__
#define __XWinPlatform__

	// Platform & compiler macros
	#define VERSIONMAC	  0
	#define VERSION_LINUX 0
	#define VERSIONWIN	  1
	
	#define ARCH_386		1
	#define ARCH_POWER		0
	
	//On teste la presence d'une architecure 64 bits
	#if defined(_WIN64) || defined(WIN64)
		#define ARCH_32			0
		#define ARCH_64			1
	#else
		#define ARCH_32			1
		#define ARCH_64			0
	#endif 
	
	#define VERSION_DARWIN	0
	
#if defined(_MSC_VER)
	#define COMPIL_VISUAL 1
	#define COMPIL_CODEWARRIOR 0
	#define COMPIL_GCC 0
	#define COMPIL_CLANG 0
#else
	#pragma warning("Unknown compiler")
#endif

// Guess VERSIONDEBUG if not defined yet
#ifndef VERSIONDEBUG
#if COMPIL_VISUAL
	#ifndef _DEBUG
		#define VERSIONDEBUG 0
	#else
	   	#define VERSIONDEBUG 1
	#endif
#elif COMPIL_GCC
	#if __OPTIMIZE__
		#define VERSIONDEBUG	0
	#else
		#define VERSIONDEBUG	1
	#endif
#endif
#endif	// VERSIONDEBUG

	// Processor & memory macros
	
	// BIGENDIAN is definedd in winsock2.h
	#ifdef BIGENDIAN
	#if BIGENDIAN != 0
	#error "BIGENDIAN is expected to be 0"
	#endif
	#else
	#define BIGENDIAN	0
	#endif
	
	#define SMALLENDIAN	1
	#define NATIVE_64_BITS	0

	#define VTC_WCHAR VTC_UTF_16
#if COMPIL_VISUAL
	#define WCHAR_IS_UNICHAR 1
/*
	#undef _SECURE_SCL
	#undef _HAS_ITERATOR_DEBUGGING
	#define _SECURE_SCL 0
	#define _HAS_ITERATOR_DEBUGGING 0
*/
#else
	#define WCHAR_IS_UNICHAR 0
#endif

	// Compiler relative macros & settings
	#define EXPORT_API	__declspec (dllexport)
	#define IMPORT_API	__declspec (dllimport)
	#define EXPORT_TEMPLATE_API	__declspec (dllexport)
	#define IMPORT_TEMPLATE_API	__declspec (dllimport)
	#define EXPORT_UNMANGLED_API(_return_type_)	extern "C" __declspec (dllexport) _return_type_ __cdecl		

	#ifndef WINVER
	#define WINVER		0x501
	#endif
	
	#ifndef _WIN32_IE
	#define _WIN32_IE	0x600
	#endif

	#ifndef _WIN32_WINNT
	#define _WIN32_WINNT	WINVER
	#endif
	
	#ifndef _WIN32_WINDOWS
	#define _WIN32_WINDOWS	WINVER
	#endif

	//pour visual 2005 declared deprecated
	#pragma warning (disable: 4996)

	//pour visual 2005 declared deprecated
	#pragma warning (disable: 4018)
	
	/*
	'identifier' : macro redefinition
	The macro identifier is defined twice. The compiler uses the second macro definition.
	*/
	#pragma warning (disable: 4005)

	/*
	unknown pragma
	The compiler ignored an unrecognized pragma. Be sure the pragma is allowed by the compiler you are using. The following sample generates C4068:
	*/
	#pragma warning (disable: 4068)

	/*
	expected identifier for segment name; found 'symbol'
	The name of the segment in #pragma alloc_text must be a string or an identifier. The compiler ignores the pragma if a valid identifier is not found. 

	*/
	#pragma warning (disable: 4080)

	/*
	expected 'token1'; found 'token2'
	The compiler expected a different token in this context. (pragma)
	*/
	#pragma warning (disable: 4081)

	/*
	expected 'token'; found identifier 'identifier'
	An identifier occurs in the wrong place in a #pragma statement.
	*/
	#pragma warning (disable: 4083)
	

	/*
	'identifier' : unreferenced local variable
	The local variable is never used.
	*/
	#pragma warning (disable:  4101)


	/*
	returning address of local variable or temporary
	A function returns the address of a local variable or temporary object. Local variables and temporary	objects are destroyed when a function returns, so the address returned is not valid. 
	Redesign the function so that it does not return the address of a local object.
	*/
	//#pragma warning (disable:  4172)

	/*
	nonstandard extension used : zero-sized array in struct/union
	A structure or union contains an array with zero size.
	*/
	//#pragma warning (disable: 4200)
	
	/*
	*/
	//#pragma warning (disable: 4237)

	/*
	'variable' : conversion from 'type' to 'type', possible loss of data
	You assigned a value of type __int64 to a variable of type unsigned int. A possible loss of data may have occurred.
	*/
	#pragma warning (disable: 4244)

	/*
	'class1' : inherits 'class2::member' via dominance
	Two or more members have the same name. The one in class2 is inherited because it is a base class for the	other classes that contained this member.
	*/
	//#pragma warning (disable: 4250)

	/*
	'identifier' : class 'type' needs to have dll-interface to be used by clients of class 'type2'
	A base class or structure must be declared with the __declspec(dllexport) keyword for a function in a derived class to be exported.
	*/
	#pragma warning (disable: 4251)

	/*
	nonstandard extension used: 'initializing': a non-const 'type1' must be initialized with an l-value, not a function returning 'type2'
	A nonconst reference must be initialized with an l-value, making the reference a name for that l-value. A function call is an l-value only if the return type is a reference. Under Microsoft extensions to the C++ language, treat any function call as an l-value for the purpose of initializing references. If Microsoft extensions are disabled (/Za), an error occurs.
	*/
	//#pragma warning (disable: 4270)

	/*
	function' : member function does not override any base class virtual member function
	A class function definition has the same name as a virtual function in a base class but not the same number or type of arguments. This effectively hides the virtual function in the base class.
	*/
	#pragma warning (3: 4263)
	#pragma warning (3: 4264)
	
	/*
		conversion from 'size_t' to 'int', possible loss of data
	*/
	#pragma warning (disable: 4267)

	/*
	non  DLL-interface classkey 'identifier' used as base for DLL-interface classkey 'identifier'
	An exported class was derived from a class that was not exported.
	*/
	#pragma warning (disable: 4275)

	/*
	'identifier' : truncation from 'type1' to 'type2'
	The identifier is converted to a smaller type, resulting in loss of information.
	*/
	#pragma warning (disable: 4305)

	/*
	warning C4615: #pragma warning : unknown user warning type
	*/
	#pragma warning (disable: 4615)

	/*
	warning C4292: compiler limit : terminating debug information emission for enum 'xbox::UnicodeKey' with member 'CHAR_ORIYA_DIGIT_ZERO'
	*/
	#pragma warning (disable: 4292)

	/*
	warning C4355: 'this' : used in base member initializer list
	*/
	#pragma warning (disable: 4355)

	// Required includes
	#define _USE_OLD_IOSTREAMS


	#pragma pack(push,__before,8)

	#define _WINSOCKAPI_   /* Prevent inclusion of winsock.h in windows.h */
	#include <Windows.h>
	#include <SHLOBJ.H>
	#include <Windowsx.h>
	#include <shellapi.h>
	#include <mlang.h>
	
	// Undefine some Windowsx defines
	#undef Yield
	#undef GetWindowOwner
	#undef GetFirstChild
	#undef GetFirstSibling
	#undef GetLastSibling
	#undef GetNextSibling
	#undef GetPrevSibling
	#undef GetCurrentProcess
	#undef GetMessage
	#undef IsMinimized
	#undef IsMaximized
	#undef IsRestored
	#undef DrawText
	#undef FindWindow

	#pragma pack(pop,__before)
	
#if COMPIL_CODEWARRIOR
	#include <ansi_parms.h>
	#include <div_t.h>
	#include <stat.h>
	#include <null.h>
	#include <size_t.h>
	#include <unistd.h>
	#include <utime.h>
	#include <va_list.h>
	#include <wchar.h>
#elif COMPIL_VISUAL				  
	#include <wchar.h>
#endif

	#include <ctype.h>
	#include <errno.h>
	#include <fcntl.h>
	#include <float.h>
	#include <limits.h>
	#include <locale.h>

#ifndef __SANE__
	#include <math.h>	// Usefull for VS2008
#endif

	#include <setjmp.h>
	#include <signal.h>
	#include <stdarg.h>
	#include <stddef.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <time.h>
	#include <typeinfo.h>

	#include <io.h>

// STL
	#include <vector>
	#include <algorithm>
	#include <hash_map>
	// namespace for non-standard yet hash_map
	#define STL_HASH_MAP stdext
	
#endif

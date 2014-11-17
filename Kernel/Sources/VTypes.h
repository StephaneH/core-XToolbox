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
#ifndef __VTypes__
#define __VTypes__

#include "Kernel/Sources/VKernelFlags.h"


/*

	following scalar types are in common with old xtoolbox.
	we don't put them in a namespace so that we don't have to use a namespace to use them.

	see //depot/4eDimension/main/XToolBox/XToolboxHeaders/VTypes.h
*/

// Standard types aliases

typedef unsigned char	uBOOL;
typedef signed char		sCHAR;
typedef unsigned char	uCHAR;
typedef char			sBYTE;
typedef unsigned char	uBYTE;
typedef signed short	sWORD;
typedef unsigned short	uWORD;

#if COMPIL_VISUAL
	typedef __int64		sLONG8;
	typedef unsigned __int64	uLONG8;
	typedef signed long		sLONG;
	typedef unsigned long	uLONG;
#elif COMPIL_GCC
	#if __LP64__
	typedef signed int			sLONG;
	typedef unsigned int		uLONG;
	#else
	typedef signed long			sLONG;
	typedef unsigned long		uLONG;
	#endif
	typedef signed long long	sLONG8;
	typedef unsigned long long	uLONG8;
#endif

// sLONG_PTR && uLONG_PTR are used for variables that may contain a sLONG which is always 32bits or a pointer which can be greater than 32bits.
// mimic LONG_PTR windows typedef. Of course, these typedefs will whange on 64bits
#if (ARCH_32)

#if COMPIL_VISUAL
	// _W64 tells visual to skip warning on casting back or from a void*
	typedef _W64 sLONG sLONG_PTR;
	typedef _W64 uLONG uLONG_PTR;
#else
	typedef sLONG sLONG_PTR;
	typedef uLONG uLONG_PTR;
#endif

#else

#if COMPIL_VISUAL
	// _W64 tells visual to skip warning on casting back or from a void*
	typedef _W64 sLONG8 sLONG_PTR;
	typedef _W64 uLONG8 uLONG_PTR;
#else
	typedef sLONG8 sLONG_PTR;
	typedef uLONG8 uLONG_PTR;
#endif

#endif

#ifndef nil
	#define nil 0
#endif

#if COMPIL_GCC
#define XBOX_LONG8(_value_) _value_##LL
#else
#define XBOX_LONG8(_value_) _value_
#endif

// Text types
#if VERSIONMAC
typedef unsigned short	unichar;
typedef unichar		UniChar;
#elif VERSIONWIN
typedef wchar_t		UniChar;	// to ease debugging in VStudio 2012 (should better use char16_t when properly recognized)
#else
typedef uWORD		UniChar;
#endif
typedef UniChar*	UniPtr;
typedef uLONG		OsType;

typedef double	Real;
typedef float	SmallReal;


// 'Boolean' is defined in Universal Headers (Carbon), QTML/MacTypes (Win)
//	and CarbonCore/MacTypes (OSX). Leaving it here will ensure its definition and
//	ensure that any change in those files will generate a compile error
typedef unsigned char	Boolean;

// end of common types


BEGIN_TOOLBOX_NAMESPACE

// Basic memory types
typedef char*	VPtr;
typedef struct OpaqueHandle*	VHandle;


// Math constants
#ifndef PI
const Real	PI = 3.1415926535897932384626433832795;
#endif


// Types Min/Max constants
const sBYTE	kMIN_sBYTE	= (sBYTE) 0x80;
const sBYTE kMAX_sBYTE	= 0x7F;
const uBYTE kMIN_uBYTE	= 0x00;
const uBYTE kMAX_suBYTE	= 0xFF;
const sWORD	kMIN_sWORD	= (sWORD) 0x8000;
const sWORD kMAX_sWORD	= 0x7FFF;
const uWORD kMIN_uWORD	= 0x0000;
const uWORD kMAX_uWORD	= 0xFFFF;
const sLONG	kMIN_sLONG	= (sLONG) 0x80000000;
const sLONG kMAX_sLONG	= 0x7FFFFFFF;
const uLONG kMIN_uLONG	= 0x00000000;
const uLONG kMAX_uLONG	= 0xFFFFFFFF;
const sLONG8	kMIN_sLONG8	= (sLONG8) XBOX_LONG8(0x8000000000000000);
const sLONG8	kMAX_sLONG8	= XBOX_LONG8(0x7FFFFFFFFFFFFFFF);
const uLONG8	kMIN_uLONG8	= XBOX_LONG8(0);
const uLONG8	kMAX_uLONG8	= XBOX_LONG8(0xFFFFFFFFFFFFFFFF);
const Real	kMIN_Real	=-1.7976931348623157e+308;	// 0x8FEFFFFFFFFFFFFF
const Real	kMAX_Real	= 1.7976931348623157e+308;	// 0x7FEFFFFFFFFFFFFF
const Real	kPRECISION_Real	= 2.2250738585072020e-308;	// 0x0010000000000001
const SmallReal	kMIN_SmallReal	=-3.4028234663852886e+38f;	// 0xFF7FFFFF
const SmallReal	kMAX_SmallReal	= 3.4028234663852886e+38f;	// 0x8F7FFFFF
const SmallReal	kPRECISION_SmallReal	= 1.175494490952134e-38f;	// 0x00800001

#if COMPIL_VISUAL
// CodeWarrior doesn't support unnormalized values
const Real	kPRECISION_NOT_NORMALIZED_Real	= 5.0000000000000000e-324;	// 0x0000000000000001
const SmallReal	kPRECISION_NOT_NORMALIZED_SmallReal	= 1.401298464324817e-45;	// 0x00000001
#endif

const sWORD	MinInt		= kMIN_sWORD;
const sWORD MaxInt		= kMAX_sWORD;
const sLONG MinLongInt	= kMIN_sLONG;
const sLONG MaxLongInt	= kMAX_sLONG;
const sLONG InvalidHour = 0x80000000;


// Types for handling maximal processor-based values.
// Ideal for counting indexes. Do not store on disk
// as they may change with next processor generations !
//
typedef size_t	VSize;
typedef sLONG	VIndex;
const VIndex	kMAX_VIndex	= 0x7FFFFFFF;

#if (ARCH_32) //VSize->size_t->unsigned int-> 32 bits
	const VSize		kMAX_VSize	= 0xFFFFFFFF;
#else //VSize->size_t->unsigned int_64-> 64 bits
	const VSize		kMAX_VSize	= XBOX_LONG8(0xFFFFFFFFFFFFFFFF);
#endif 

// Text types
typedef UniChar*	UniString;
typedef uBYTE*		PString;
typedef char*		CString;

typedef const uBYTE*	ConstPString;
typedef const char*		ConstCString;


// Tristate compatible with Booleans (writing triState = boolean is correct)
typedef enum {
	TRI_STATE_OFF	= 0,
	TRI_STATE_ON	= 1,
	TRI_STATE_LATENT,
	TRI_STATE_MIXED	= TRI_STATE_LATENT
} VTriState;

// Struct for automatic byteswap handling
typedef struct FourBytes
{
#if BIGENDIAN
	uLONG	zero	: 8;
	uLONG	one		: 8;
	uLONG	two		: 8;
	uLONG	three	: 8;
#else
	uLONG	three	: 8;
	uLONG	two		: 8;
	uLONG	one		: 8;
	uLONG	zero	: 8;
#endif
} FourBytes, *FourBytesPtr;


// Utilities
template <class Type> inline Type Sign (Type a) { return ((a < (Type)0) ? (Type)-1 : 1); }
template <class Type> inline Type Min (Type a, Type b) { return ((a < b) ? a : b); }
template <class Type> inline Type Max (Type a, Type b) { return ((a > b) ? a : b); }
template <class Type> inline Type Abs (Type a) { return ((a < (Type)0) ? -a : a); }

template <class Type> inline Type TicksToMs (Type a) { return (Type) ((Real) a * (1000.0 / 60.0)); }
template <class Type> inline Type MsToTicks (Type a) { return (Type) ((Real) a * (60.0 / 1000.0)); }

// VoidType type definition
template<class T>
class VoidTypeT {};
typedef VoidTypeT<void> VoidType;

END_TOOLBOX_NAMESPACE


#endif

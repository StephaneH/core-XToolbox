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
#ifndef __VByteSwap__
#define __VByteSwap__

#include "Kernel/Sources/VKernelTypes.h"

BEGIN_TOOLBOX_NAMESPACE

#if defined (__cplusplus)
	extern "C" {
#endif
Boolean SwapRsrcProc (Boolean mac2win, uLONG kind, sLONG size, VPtr data);
#if defined (__cplusplus)
	}
#endif

/*
	To detect at compile time the use of a non-defined ByteSwapValue specialization,
	the default implementation use a non existing type.
	We use a templated struct so that the compiler can still compile the template.
*/
template<class T> struct __non_existing_type;

template<class T> T ByteSwapValue( const T& inValue)	{ __non_existing_type<T> a; return inValue;}


template<>
inline uWORD ByteSwapValue<uWORD>( const uWORD& inValue)
	{
	#if VERSIONMAC
	return ::CFSwapInt16( inValue);	// inline asm
	#else
	return ((inValue << 8) & 0xFF00) | ((inValue >> 8) & 0xFF);
	#endif
	}


template<>
inline uLONG ByteSwapValue<uLONG>( const uLONG& inValue)
	{
	#if VERSIONMAC
	return ::CFSwapInt32( inValue);	// inline asm
	#else
	return ((inValue & 0xFF) << 24) | ((inValue & 0xFF00) << 8) | ((inValue >> 8) & 0xFF00) | ((inValue >> 24) & 0xFF);
	#endif
	}


template<>
inline uLONG8 ByteSwapValue<uLONG8>( const uLONG8& inValue)
{
    union Swap {
        uLONG8 sv;
        uLONG ul[2];
    } tmp, result;
    tmp.sv = inValue;
    result.ul[0] = ByteSwapValue(tmp.ul[1]); 
    result.ul[1] = ByteSwapValue(tmp.ul[0]);
    return result.sv;
}


template<>
inline Real ByteSwapValue<Real>( const Real& inValue)
	{
    union Swap {
        Real sv;
        uLONG ul[2];
    } tmp, result;
    tmp.sv = inValue;
    result.ul[0] = ByteSwapValue(tmp.ul[1]); 
    result.ul[1] = ByteSwapValue(tmp.ul[0]);
    return result.sv;
}


template<>
inline SmallReal ByteSwapValue<SmallReal>( const SmallReal& inValue)
	{
    union Swap {
        SmallReal sv;
        uLONG ul;
    } tmp, result;
    tmp.sv = inValue;
    result.ul = ByteSwapValue(tmp.ul); 
    return result.sv;
}


template<> inline sWORD ByteSwapValue<sWORD>( const sWORD& inValue)	{return ByteSwapValue<uWORD>( reinterpret_cast<const uWORD&>( inValue));}
template<> inline sLONG ByteSwapValue<sLONG>( const sLONG& inValue)	{return ByteSwapValue<uLONG>( reinterpret_cast<const uLONG&>( inValue));}
template<> inline sLONG8 ByteSwapValue<sLONG8>( const sLONG8& inValue)	{return ByteSwapValue<uLONG8>( reinterpret_cast<const uLONG8&>( inValue));}
template<> inline sBYTE ByteSwapValue<sBYTE>( const sBYTE& inValue)	{return inValue;}
template<> inline uBYTE ByteSwapValue<uBYTE>( const uBYTE& inValue)	{return inValue;}


template<class T> void ByteSwap( T *ioValue)	{*ioValue = ByteSwapValue( *ioValue);}

template<class ForwardIterator>
void ByteSwap( ForwardIterator inBegin, ForwardIterator inEnd)
{
	for( ; inBegin != inEnd ; ++inBegin)
		ByteSwap( &*inBegin); // syntax qui semble etrange mais * peut etre overloaded 
}

template<class T>
void ByteSwap( T *inFirstValue, VIndex inNbElems)
{
	ByteSwap( inFirstValue, inFirstValue + inNbElems);
}

 
XTOOLBOX_API uLONG	BuildLong (uWORD inHiWord, uWORD inLoWord);
XTOOLBOX_API uLONG8	BuildLong8 (uLONG inHiWord, uLONG inLoWord);
XTOOLBOX_API uWORD	GetHiWord (uLONG inLong);
XTOOLBOX_API sWORD	GetHiWord (sLONG inLong);
XTOOLBOX_API uLONG	GetHiWord (uLONG8 inLong8);
XTOOLBOX_API sLONG	GetHiWord (sLONG8 inLong8);
XTOOLBOX_API uWORD	GetLoWord (uLONG inLong);
XTOOLBOX_API sWORD	GetLoWord (sLONG inLong);
XTOOLBOX_API uLONG	GetLoWord (uLONG8 inLong8);
XTOOLBOX_API sLONG	GetLoWord (sLONG8 inLong8);
XTOOLBOX_API sWORD	GetWord (sLONG inLong);

inline void	ByteSwapWord( sWORD* inWord)		{ByteSwap( inWord);}
inline void	ByteSwapWord( uWORD* inWord)		{ByteSwap( inWord);}
inline void	ByteSwapLong( sLONG* inLong)		{ByteSwap( inLong);}
inline void	ByteSwapLong( uLONG* inLong)		{ByteSwap( inLong);}
inline void	ByteSwapLong8( sLONG8* inLong)		{ByteSwap( inLong);}
inline void	ByteSwapLong8( uLONG8* inLong)		{ByteSwap( inLong);}
inline void	ByteSwapReal8( Real* inReal)		{ByteSwap( inReal);}
inline void	ByteSwapReal4( SmallReal* inReal)	{ByteSwap( inReal);}
inline void	ByteSwapWordArray( sWORD* inWord, sLONG inCount)	{ByteSwap( inWord, inWord + inCount);}
inline void	ByteSwapWordArray( uWORD* inWord, sLONG inCount)	{ByteSwap( inWord, inWord + inCount);}
inline void	ByteSwapLongArray( sLONG* inLong, sLONG inCount)	{ByteSwap( inLong, inLong + inCount);}
inline void	ByteSwapLongArray( uLONG* inLong, sLONG inCount)	{ByteSwap( inLong, inLong + inCount);}
inline void	ByteSwapLong8Array( sLONG8* inLong, sLONG inCount)	{ByteSwap( inLong, inLong + inCount);}
inline void	ByteSwapLong8Array( uLONG8* inLong, sLONG inCount)	{ByteSwap( inLong, inLong + inCount);}
inline void	ByteSwapReal8Array( Real* inReal, sLONG inCount)	{ByteSwap( inReal, inReal + inCount);}
inline void	ByteSwapReal4Array( SmallReal* inReal, sLONG inCount)	{ByteSwap( inReal, inReal + inCount);}

/* A Supprimer car rien n'est utilisÈ
XTOOLBOX_API void	Real8toReal10 (void* inReal8, void* inReal10);
XTOOLBOX_API void	Real10toReal8 (void* inReal10, void* inReal8);
XTOOLBOX_API void	Real8toReal10Array (void* inReal8, void* inReal10, sLONG inCount);
XTOOLBOX_API void	Real10toReal8Array (void* inReal10, void* inReal8, sLONG inCount);
XTOOLBOX_API Real	FixedToReal (sLONG inLong);
*/
XTOOLBOX_API uLONG	GetPaddedSize (uLONG inSize, uWORD inPadding);


inline sWORD GetWord (sLONG inLong)
{
	if (inLong < kMIN_sWORD)
		return kMIN_sWORD;
	
	if (inLong > kMAX_sWORD)
		return kMAX_sWORD;
	
	return static_cast<sWORD>( inLong);
}


inline uLONG BuildLong (uWORD inHiWord, uWORD inLoWord)
{
	return ((((uLONG) inHiWord) << 16) | ((uLONG) inLoWord));
}


inline uLONG8 BuildLong8 (uLONG inHiWord, uLONG inLoWord)
{
	return ((((uLONG8) inHiWord) << 32) | ((uLONG8) inLoWord));
}


inline uWORD GetHiWord (uLONG inLong)
{
	return (uWORD)((inLong >> 16) & 0x0000FFFF);
}


inline uWORD GetLoWord (uLONG inLong)
{
	return (uWORD)(inLong & 0x0000FFFF);
}


inline sWORD GetHiWord (sLONG inLong)
{
	return (sWORD) GetHiWord((uLONG) inLong);
}


inline sWORD GetLoWord (sLONG inLong)
{
	return (sWORD) GetLoWord((uLONG) inLong);
}


inline uLONG GetHiWord (uLONG8 inLong8)
{
	return (uLONG)((inLong8 >> 32) & 0x00000000FFFFFFFF);
}


inline uLONG GetLoWord (uLONG8 inLong8)
{
	return (uLONG)(inLong8 & 0x00000000FFFFFFFF);
}


inline sLONG GetHiWord (sLONG8 inLong8)
{
	return (sLONG) GetHiWord((uLONG8) inLong8);
}


inline sLONG GetLoWord (sLONG8 inLong8)
{
	return (sLONG) GetLoWord((uLONG8) inLong8);
}


inline uLONG GetPaddedSize (uLONG inSize, uWORD inPadding)
{
	return ((inSize + inPadding - 1) & ~(inPadding - 1));
}

END_TOOLBOX_NAMESPACE

#endif

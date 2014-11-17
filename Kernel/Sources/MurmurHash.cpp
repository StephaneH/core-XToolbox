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
#include "MurmurHash.h"

// The following is the reference implementation of a murmur hash function for
// generating 32-bit and 64-bit hashes.  The code was written by Austin Appleby 
// http://murmurhash.googlepages.com/.
// All code is released to the public domain. For business purposes, Murmurhash is
// under the MIT license. 

BEGIN_TOOLBOX_NAMESPACE

XTOOLBOX_API uLONG8 MurmurHash64B ( const void * key, int len, unsigned int seed )
{
	const unsigned int m = 0x5bd1e995;
	const int r = 24;

	unsigned int h1 = seed ^ len;
	unsigned int h2 = 0;

	const unsigned int * data = (const unsigned int *)key;

	while(len >= 8)	{
		unsigned int k1 = *data++;
		k1 *= m; k1 ^= k1 >> r; k1 *= m;
		h1 *= m; h1 ^= k1;
		len -= 4;

		unsigned int k2 = *data++;
		k2 *= m; k2 ^= k2 >> r; k2 *= m;
		h2 *= m; h2 ^= k2;
		len -= 4;
	}

	if(len >= 4) {
		unsigned int k1 = *data++;
		k1 *= m; k1 ^= k1 >> r; k1 *= m;
		h1 *= m; h1 ^= k1;
		len -= 4;
	}

	switch(len)	{
	case 3: h2 ^= ((unsigned char*)data)[2] << 16;
	case 2: h2 ^= ((unsigned char*)data)[1] << 8;
	case 1: h2 ^= ((unsigned char*)data)[0];
			h2 *= m;
	};

	h1 ^= h2 >> 18; h1 *= m;
	h2 ^= h1 >> 22; h2 *= m;
	h1 ^= h2 >> 17; h1 *= m;
	h2 ^= h1 >> 19; h2 *= m;

	uLONG8 h = h1;

	h = (h << 32) | h2;

	return h;
}

XTOOLBOX_API uLONG8 MurmurHash64A ( const void * key, int len, unsigned int seed )
{
	const uLONG8 m = 0xc6a4a7935bd1e995LL;
	const int r = 47;

	uLONG8 h = seed ^ (len * m);

	const uLONG8 * data = (const uLONG8 *)key;
	const uLONG8 * end = data + (len/8);

	while(data != end)
	{
		uLONG8 k = *data++;

		k *= m; 
		k ^= k >> r; 
		k *= m; 
		
		h ^= k;
		h *= m; 
	}

	const unsigned char * data2 = (const unsigned char*)data;

	switch(len & 7)
	{
	case 7: h ^= uLONG8(data2[6]) << 48;
	case 6: h ^= uLONG8(data2[5]) << 40;
	case 5: h ^= uLONG8(data2[4]) << 32;
	case 4: h ^= uLONG8(data2[3]) << 24;
	case 3: h ^= uLONG8(data2[2]) << 16;
	case 2: h ^= uLONG8(data2[1]) << 8;
	case 1: h ^= uLONG8(data2[0]);
	        h *= m;
	};
 
	h ^= h >> r;
	h *= m;
	h ^= h >> r;

	return h;
}

//-----------------------------------------------------------------------------
// MurmurHash2, by Austin Appleby
// Note - This code makes a few assumptions about how your machine behaves -
//
// 1. We can read a 4-byte value from any address without crashing
// 2. sizeof(int) == 4
//
// And it has a few limitations -
//
// 1. It will not work incrementally.
// 2. It will not produce the same results on little-endian and big-endian
//    machines.
XTOOLBOX_API unsigned int MurmurHash2 ( const void * key, int len, unsigned int seed )
{
	// 'm' and 'r' are mixing constants generated offline.
	// They're not really 'magic', they just happen to work well.
	const unsigned int m = 0x5bd1e995;
	const int r = 24;

	// Initialize the hash to a 'random' value
	unsigned int h = seed ^ len;

	// Mix 4 bytes at a time into the hash
	const unsigned char * data = (const unsigned char *)key;

	while(len >= 4) {
		unsigned int k = *(unsigned int *)data;

		k *= m; 
		k ^= k >> r; 
		k *= m; 
		
		h *= m; 
		h ^= k;

		data += 4;
		len -= 4;
	}
	
	// Handle the last few bytes of the input array
	switch(len)	{
	case 3: h ^= data[2] << 16;
	case 2: h ^= data[1] << 8;
	case 1: h ^= data[0];
	        h *= m;
	}

	// Do a few final mixes of the hash to ensure the last few
	// bytes are well-incorporated.
	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
}

XTOOLBOX_API uLONG8 SimpleMurmurHash64( const void *key, int len )
{
	// We make life easier on the caller by picking an initial seed that is a large
	// prime number.  Note that we do *not* modify the value of the seed between calls
	// to the hashing function.  Doing so would mean that we wouldn't get the same hash
	// back given identical sets of input.
	const unsigned int kSeed = 0xC47EF1D7;	// A large, 32-bit prime number as our default seed.

	// There are two different versions of the murmur hash function which return a 64-bit
	// value.  One is optimized for 32-bit target machines, and the other is optimized for
	// 64-bit target machines.  So we are using the target architecture to determine which
	// version of the murmur hash we call.
	#if ARCH_32
		return MurmurHash64B( key, len, kSeed );
	#elif ARCH_64
		return MurmurHash64A( key, len, kSeed );
	#else
		This is an error -- should pick a target architecture
	#endif
}

END_TOOLBOX_NAMESPACE
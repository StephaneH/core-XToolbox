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
#include "VChecksumMD5.h"
#include "VString.h"
#include "Base64Coder.h"

/*
 ***********************************************************************
 ** Copyright (C) 1990, RSA Data Security, Inc. All rights reserved.	**
 ** 																	**
 ** License to copy and use this software is granted provided that		**
 ** it is identified as the "RSA Data Security, Inc. MD5 Message-    	**
 ** Digest Algorithm" in all material mentioning or referencing this  	**
 ** software or this function.											**
 ** 																	**
 ** License is also granted to make and use derivative works			**
 ** provided that such works are identified as "derived from the RSA  	**
 ** Data Security, Inc. MD5 Message-Digest Algorithm" in all			**
 ** material mentioning or referencing the derived work.				**
 ** 																	**
 ** RSA Data Security, Inc. makes no representations concerning 		**
 ** either the merchantability of this software or the suitability		**
 ** of this software for any particular purpose.	It is provided "as	**
 ** is" without express or implied warranty of any kind.              	**
 ** 																	**
 ** These notices must be retained in any copies of any part of this	**
 ** documentation and/or software.										**
 ***********************************************************************
 */

static void _EncodeChecksumBase64( const uBYTE *inDigest, size_t inSize, VString& outChecksumBase64)
{
	outChecksumBase64.Clear();

	VMemoryBuffer<> buffer;
	if (Base64Coder::Encode( inDigest, inSize, buffer))
	{
		outChecksumBase64.FromBlock( buffer.GetDataPtr(), buffer.GetDataSize(), VTC_UTF_8);
	}
}


static void _EncodeChecksumHexa( const uBYTE *inDigest, size_t inSize, VString& outHexaDigest)
{
	UniChar *p = outHexaDigest.GetCPointerForWrite( (VIndex) inSize * 2);
	if (p != NULL)
	{
		for( size_t i = 0 ; i < inSize ; ++i)
		{
			uBYTE nibble1 = inDigest[i] >> 4;
			if (nibble1 < 10)
				p[2*i] = '0' + nibble1;
			else
				p[2*i] = 'a' + (nibble1 - 10);

			uBYTE nibble2 = inDigest[i] & 0x0f;
			if (nibble2 < 10)
				p[2*i+1] = '0' + nibble2;
			else
				p[2*i+1] = 'a' + (nibble2 - 10);
		}
		outHexaDigest.Validate( (VIndex) inSize * 2);
	}
	else
	{
		outHexaDigest.Clear();
	}
}


//========

static uBYTE PADDING[64] = {
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

VChecksumMD5::VChecksumMD5()
: fFinalized( false)
{
	_MD5Init();
}

void VChecksumMD5::Clear()
{
	_MD5Init();
	fFinalized = false;
}

void VChecksumMD5::Update( const void *inData, size_t inSize )
{
	if ( (inData != NULL) && (inSize > 0) && testAssert( !fFinalized) )
	{
		_MD5Update( (uBYTE*) inData, inSize);
	}
}

/* The routine MD5Final terminates the message-digest computation and
	 ends with the desired message digest in digest[0...15].
 */
void VChecksumMD5::GetChecksum( MD5& outDigest)
{
	if (!fFinalized)
	{
		uLONG in[16];
		sWORD mdi;
		uWORD i, ii;
		size_t padLen;

		/* save number of bits */
		in[14] = fContext.i[0];
		in[15] = fContext.i[1];

		/* compute number of bytes mod 64 */
		mdi = (sWORD)((fContext.i[0] >> 3) & 0x3F);

		/* pad out to 56 mod 64 */
		padLen = (uWORD) ((mdi < 56) ? (56 - mdi) : (120 - mdi));
		_MD5Update (PADDING, padLen);

		/* append length in bits and transform */
		for (i = 0, ii = 0; i < 14; i++, ii += 4)
			in[i] = (((uLONG)fContext.in[ii+3]) << 24) |
							(((uLONG)fContext.in[ii+2]) << 16) |
							(((uLONG)fContext.in[ii+1]) << 8) |
							((uLONG)fContext.in[ii]);
		_Transform( fContext.buf, in);

		fFinalized = true;
	}

	/* store buffer in digest */
	for( uLONG i = 0, ii = 0; i < 4; i++, ii += 4)
	{
		outDigest[ii]   = (uBYTE)(fContext.buf[i] & 0xFF);
		outDigest[ii+1] = (uBYTE)((fContext.buf[i] >> 8) & 0xFF);
		outDigest[ii+2] = (uBYTE)((fContext.buf[i] >> 16) & 0xFF);
		outDigest[ii+3] = (uBYTE)((fContext.buf[i] >> 24) & 0xFF);
	}
}

/*
	static
*/
void VChecksumMD5::EncodeChecksumBase64( const MD5& inDigest, VString& outChecksumBase64)
{
	_EncodeChecksumBase64( inDigest, sizeof( inDigest), outChecksumBase64);
}


/*
	static
*/
void VChecksumMD5::EncodeChecksumHexa( const MD5& inDigest, VString& outHexaDigest)
{
	_EncodeChecksumHexa( inDigest, sizeof( inDigest), outHexaDigest);
}

/*
	static
*/
void VChecksumMD5::GetChecksumFromBytes( const void *inBytes, size_t inSize, MD5& outChecksum)
{
	VChecksumMD5	checksum;
	checksum.Update( inBytes, inSize);
	checksum.GetChecksum( outChecksum);
}

/*
	static
*/
void VChecksumMD5::GetChecksumFromBytesHexa( const void *inBytes, size_t inSize, VString& outHexaChecksum)
{
	MD5 checksum;
	GetChecksumFromBytes( inBytes, inSize, checksum);
	EncodeChecksumHexa( checksum, outHexaChecksum);
}

/*
	static
*/
void VChecksumMD5::GetChecksumFromStringUTF8( const VString& inString, MD5& outChecksum)
{
	VStringConvertBuffer buffer( inString, VTC_UTF_8);
	GetChecksumFromBytes( buffer.GetCPointer(), buffer.GetSize(), outChecksum);
}

/*
	static
*/
void VChecksumMD5::GetChecksumFromStringUTF8Hexa( const VString& inString, VString& outHexaChecksum)
{
	MD5 checksum;
	GetChecksumFromStringUTF8( inString, checksum);
	EncodeChecksumHexa( checksum, outHexaChecksum);
}


/* F, G, H and I are basic MD5 functions */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

/* ROTATE_LEFT rotates x left n bits */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4 */
/* Rotation is separate from addition to prevent recomputation */
#define FF(a, b, c, d, x, s, ac) \
	{(a) += F ((b), (c), (d)) + (x) + (uLONG)(ac); \
	 (a) = ROTATE_LEFT ((a), (s)); \
	 (a) += (b); \
	}
#define GG(a, b, c, d, x, s, ac) \
	{(a) += G ((b), (c), (d)) + (x) + (uLONG)(ac); \
	 (a) = ROTATE_LEFT ((a), (s)); \
	 (a) += (b); \
	}
#define HH(a, b, c, d, x, s, ac) \
	{(a) += H ((b), (c), (d)) + (x) + (uLONG)(ac); \
	 (a) = ROTATE_LEFT ((a), (s)); \
	 (a) += (b); \
	}
#define II(a, b, c, d, x, s, ac) \
	{(a) += I ((b), (c), (d)) + (x) + (uLONG)(ac); \
	 (a) = ROTATE_LEFT ((a), (s)); \
	 (a) += (b); \
	}

/* The routine MD5Init initializes the message-digest context
	 mdContext. All fields are set to zero.
 */
void VChecksumMD5::_MD5Init ()
{
	fContext.i[0] = fContext.i[1] = (uLONG)0;

	/* Load magic initialization constants.
	 */
	fContext.buf[0] = (uLONG)0x67452301;
	fContext.buf[1] = (uLONG)0xefcdab89;
	fContext.buf[2] = (uLONG)0x98badcfe;
	fContext.buf[3] = (uLONG)0x10325476;
}

/* The routine MD5Update updates the message-digest context to
	 account for the presence of each of the characters inBuf[0..inLen-1]
	 in the message whose digest is being computed.
 */
void VChecksumMD5::_MD5Update( const uBYTE *inBuf, size_t inLen)
{
	uLONG in[16];
	sWORD mdi;
	size_t i, ii;

	/* compute number of bytes mod 64 */
	mdi = (sWORD)((fContext.i[0] >> 3) & 0x3F);

	/* update number of bits */
	if ((fContext.i[0] + ((uLONG)inLen << 3)) < fContext.i[0])
		fContext.i[1]++;
	fContext.i[0] += ((uLONG)inLen << 3);
	fContext.i[1] += ((uLONG)inLen >> 29);

	while (inLen--) {
		/* add new character to buffer, increment mdi */
		fContext.in[mdi++] = *inBuf++;

		/* transform if necessary */
		if (mdi == 0x40) {
			for (i = 0, ii = 0; i < 16; i++, ii += 4)
				in[i] = (((uLONG)fContext.in[ii+3]) << 24) |
								(((uLONG)fContext.in[ii+2]) << 16) |
								(((uLONG)fContext.in[ii+1]) << 8) |
								((uLONG)fContext.in[ii]);
			_Transform (fContext.buf, in);
			mdi = 0;
		}
	}
}

/* Basic MD5 step. Transforms buf based on in.
 */
void VChecksumMD5::_Transform (uLONG *buf, uLONG *in)
{
	uLONG a = buf[0], b = buf[1], c = buf[2], d = buf[3];

	/* Round 1 */
#define S11 7
#define S12 12
#define S13 17
#define S14 22
	FF ( a, b, c, d, in[ 0], S11, 3614090360UL); /* 1 */
	FF ( d, a, b, c, in[ 1], S12, 3905402710UL); /* 2 */
	FF ( c, d, a, b, in[ 2], S13,  606105819UL); /* 3 */
	FF ( b, c, d, a, in[ 3], S14, 3250441966UL); /* 4 */
	FF ( a, b, c, d, in[ 4], S11, 4118548399UL); /* 5 */
	FF ( d, a, b, c, in[ 5], S12, 1200080426UL); /* 6 */
	FF ( c, d, a, b, in[ 6], S13, 2821735955UL); /* 7 */
	FF ( b, c, d, a, in[ 7], S14, 4249261313UL); /* 8 */
	FF ( a, b, c, d, in[ 8], S11, 1770035416UL); /* 9 */
	FF ( d, a, b, c, in[ 9], S12, 2336552879UL); /* 10 */
	FF ( c, d, a, b, in[10], S13, 4294925233UL); /* 11 */
	FF ( b, c, d, a, in[11], S14, 2304563134UL); /* 12 */
	FF ( a, b, c, d, in[12], S11, 1804603682UL); /* 13 */
	FF ( d, a, b, c, in[13], S12, 4254626195UL); /* 14 */
	FF ( c, d, a, b, in[14], S13, 2792965006UL); /* 15 */
	FF ( b, c, d, a, in[15], S14, 1236535329UL); /* 16 */

	/* Round 2 */
#define S21 5
#define S22 9
#define S23 14
#define S24 20
	GG ( a, b, c, d, in[ 1], S21, 4129170786UL); /* 17 */
	GG ( d, a, b, c, in[ 6], S22, 3225465664UL); /* 18 */
	GG ( c, d, a, b, in[11], S23,  643717713UL); /* 19 */
	GG ( b, c, d, a, in[ 0], S24, 3921069994UL); /* 20 */
	GG ( a, b, c, d, in[ 5], S21, 3593408605UL); /* 21 */
	GG ( d, a, b, c, in[10], S22, 	38016083UL); /* 22 */
	GG ( c, d, a, b, in[15], S23, 3634488961UL); /* 23 */
	GG ( b, c, d, a, in[ 4], S24, 3889429448UL); /* 24 */
	GG ( a, b, c, d, in[ 9], S21,  568446438UL); /* 25 */
	GG ( d, a, b, c, in[14], S22, 3275163606UL); /* 26 */
	GG ( c, d, a, b, in[ 3], S23, 4107603335UL); /* 27 */
	GG ( b, c, d, a, in[ 8], S24, 1163531501UL); /* 28 */
	GG ( a, b, c, d, in[13], S21, 2850285829UL); /* 29 */
	GG ( d, a, b, c, in[ 2], S22, 4243563512UL); /* 30 */
	GG ( c, d, a, b, in[ 7], S23, 1735328473UL); /* 31 */
	GG ( b, c, d, a, in[12], S24, 2368359562UL); /* 32 */

	/* Round 3 */
#define S31 4
#define S32 11
#define S33 16
#define S34 23
	HH ( a, b, c, d, in[ 5], S31, 4294588738UL); /* 33 */
	HH ( d, a, b, c, in[ 8], S32, 2272392833UL); /* 34 */
	HH ( c, d, a, b, in[11], S33, 1839030562UL); /* 35 */
	HH ( b, c, d, a, in[14], S34, 4259657740UL); /* 36 */
	HH ( a, b, c, d, in[ 1], S31, 2763975236UL); /* 37 */
	HH ( d, a, b, c, in[ 4], S32, 1272893353UL); /* 38 */
	HH ( c, d, a, b, in[ 7], S33, 4139469664UL); /* 39 */
	HH ( b, c, d, a, in[10], S34, 3200236656UL); /* 40 */
	HH ( a, b, c, d, in[13], S31,  681279174UL); /* 41 */
	HH ( d, a, b, c, in[ 0], S32, 3936430074UL); /* 42 */
	HH ( c, d, a, b, in[ 3], S33, 3572445317UL); /* 43 */
	HH ( b, c, d, a, in[ 6], S34, 	76029189UL); /* 44 */
	HH ( a, b, c, d, in[ 9], S31, 3654602809UL); /* 45 */
	HH ( d, a, b, c, in[12], S32, 3873151461UL); /* 46 */
	HH ( c, d, a, b, in[15], S33,  530742520UL); /* 47 */
	HH ( b, c, d, a, in[ 2], S34, 3299628645UL); /* 48 */

	/* Round 4 */
#define S41 6
#define S42 10
#define S43 15
#define S44 21
	II ( a, b, c, d, in[ 0], S41, 4096336452UL); /* 49 */
	II ( d, a, b, c, in[ 7], S42, 1126891415UL); /* 50 */
	II ( c, d, a, b, in[14], S43, 2878612391UL); /* 51 */
	II ( b, c, d, a, in[ 5], S44, 4237533241UL); /* 52 */
	II ( a, b, c, d, in[12], S41, 1700485571UL); /* 53 */
	II ( d, a, b, c, in[ 3], S42, 2399980690UL); /* 54 */
	II ( c, d, a, b, in[10], S43, 4293915773UL); /* 55 */
	II ( b, c, d, a, in[ 1], S44, 2240044497UL); /* 56 */
	II ( a, b, c, d, in[ 8], S41, 1873313359UL); /* 57 */
	II ( d, a, b, c, in[15], S42, 4264355552UL); /* 58 */
	II ( c, d, a, b, in[ 6], S43, 2734768916UL); /* 59 */
	II ( b, c, d, a, in[13], S44, 1309151649UL); /* 60 */
	II ( a, b, c, d, in[ 4], S41, 4149444226UL); /* 61 */
	II ( d, a, b, c, in[11], S42, 3174756917UL); /* 62 */
	II ( c, d, a, b, in[ 2], S43,  718787259UL); /* 63 */
	II ( b, c, d, a, in[ 9], S44, 3951481745UL); /* 64 */

	buf[0] += a;
	buf[1] += b;
	buf[2] += c;
	buf[3] += d;
}

/*
 ***********************************************************************
 ** End of md5.c														**
 ******************************** (cut) ********************************
 */
 
 
 
VChecksumSHA1::VChecksumSHA1()
: fFinalized( false)
{
	SHA1Init( &fContext);
}


void VChecksumSHA1::Clear()
{
	SHA1Init( &fContext);
	fFinalized = false;
}


void VChecksumSHA1::Update( const void *inData, size_t inSize)
{
	if ( (inData != NULL) && (inSize > 0) && testAssert( !fFinalized) )
	{
		SHA1Update( &fContext, (const uBYTE*) inData, inSize);
	}
}


void VChecksumSHA1::GetChecksum( SHA1& outChecksum )
{
	if (!fFinalized)
	{
		SHA1Pad( &fContext);
		fFinalized = true;
	}

	for( uLONG i = 0; i < SHA1_SIZE; i++)
	{
		outChecksum[i] = (uBYTE) ((fContext.state[i>>2] >> ((3-(i & 3)) * 8) ) & 255);
	}
}


/*
	static
*/
void VChecksumSHA1::EncodeChecksumBase64( const SHA1& inDigest, VString& outChecksumBase64)
{
	_EncodeChecksumBase64( inDigest, sizeof( inDigest), outChecksumBase64);
}


/*
	static
*/
void VChecksumSHA1::EncodeChecksumHexa( const SHA1& inDigest, VString& outHexaDigest)
{
	_EncodeChecksumHexa( inDigest, sizeof( inDigest), outHexaDigest);
}

/*
	static
*/
void VChecksumSHA1::GetChecksumFromBytes( const void *inBytes, size_t inSize, SHA1& outChecksum)
{
	VChecksumSHA1	checksum;
	checksum.Update( inBytes, inSize);
	checksum.GetChecksum( outChecksum);
}

/*
	static
*/
void VChecksumSHA1::GetChecksumFromBytesHexa( const void *inBytes, size_t inSize, VString& outHexaChecksum)
{
	SHA1 checksum;
	GetChecksumFromBytes( inBytes, inSize, checksum);
	EncodeChecksumHexa( checksum, outHexaChecksum);
}

/*
	static
*/
void VChecksumSHA1::GetChecksumFromStringUTF8( const VString& inString, SHA1& outChecksum)
{
	VStringConvertBuffer buffer( inString, VTC_UTF_8);
	GetChecksumFromBytes( buffer.GetCPointer(), buffer.GetSize(), outChecksum);
}

/*
	static
*/
void VChecksumSHA1::GetChecksumFromStringUTF8Hexa( const VString& inString, VString& outHexaChecksum)
{
	SHA1 checksum;
	GetChecksumFromStringUTF8( inString, checksum);
	EncodeChecksumHexa( checksum, outHexaChecksum);
}


//================================================================================
  
 /*
 * SHA-1 in C
 * By Steve Reid <steve@edmweb.com>
 * 100% Public Domain
 *
 * Test Vectors (from FIPS PUB 180-1)
 * "abc"
 *   A9993E36 4706816A BA3E2571 7850C26C 9CD0D89D
 * "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
 *   84983E44 1C3BD26E BAAE4AA1 F95129E5 E54670F1
 * A million repetitions of "a"
 *   34AA973C D4C4DAA4 F61EEB2B DBAD2731 6534016F
 */




#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

/*
 * blk0() and blk() perform the initial expand.
 * I got the idea of expanding during the round function from SSLeay
 */
#if SMALLENDIAN
# define blk0(i) (block->l[i] = (rol(block->l[i],24)&0xFF00FF00) \
    |(rol(block->l[i],8)&0x00FF00FF))
#else
# define blk0(i) block->l[i]
#endif
#define blk(i) (block->l[i&15] = rol(block->l[(i+13)&15]^block->l[(i+8)&15] \
    ^block->l[(i+2)&15]^block->l[i&15],1))

/*
 * (R0+R1), R2, R3, R4 are the different operations (rounds) used in SHA1
 */
#define R0(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk0(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R1(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R2(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0x6ED9EBA1+rol(v,5);w=rol(w,30);
#define R3(v,w,x,y,z,i) z+=(((w|x)&y)|(w&x))+blk(i)+0x8F1BBCDC+rol(v,5);w=rol(w,30);
#define R4(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0xCA62C1D6+rol(v,5);w=rol(w,30);

typedef union {
	uBYTE c[64];
	uLONG l[16];
} CHAR64LONG16;


/*
 * Hash a single 512-bit block. This is the core of the algorithm.
 */
void VChecksumSHA1::SHA1Transform(uLONG state[5], const uBYTE buffer[SHA1_BLOCK_LENGTH])
{
	uLONG a, b, c, d, e;
	uBYTE workspace[SHA1_BLOCK_LENGTH];
	CHAR64LONG16 *block = (CHAR64LONG16 *)workspace;

	(void)memcpy(block, buffer, SHA1_BLOCK_LENGTH);

	/* Copy context->state[] to working vars */
	a = state[0];
	b = state[1];
	c = state[2];
	d = state[3];
	e = state[4];

	/* 4 rounds of 20 operations each. Loop unrolled. */
	R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
	R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
	R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
	R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
	R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
	R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
	R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
	R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
	R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
	R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
	R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
	R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
	R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
	R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
	R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
	R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
	R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
	R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
	R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
	R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);

	/* Add the working vars back into context.state[] */
	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
	state[4] += e;

	/* Wipe variables */
	a = b = c = d = e = 0;
}


/*
 * SHA1Init - Initialize new context
 */
void VChecksumSHA1::SHA1Init(SHA1_CTX *context)
{

	/* SHA1 initialization constants */
	context->count = 0;
	context->state[0] = 0x67452301;
	context->state[1] = 0xEFCDAB89;
	context->state[2] = 0x98BADCFE;
	context->state[3] = 0x10325476;
	context->state[4] = 0xC3D2E1F0;
}


/*
 * Run your data through this.
 */
void VChecksumSHA1::SHA1Update(SHA1_CTX *context, const uBYTE *data, size_t len)
{
	size_t i, j;

	j = (size_t)((context->count >> 3) & 63);
	context->count += (len << 3);
	if ((j + len) > 63) {
		(void)memcpy(&context->buffer[j], data, (i = 64-j));
		SHA1Transform(context->state, context->buffer);
		for ( ; i + 63 < len; i += 64)
			SHA1Transform(context->state, (uBYTE *)&data[i]);
		j = 0;
	} else {
		i = 0;
	}
	(void)memcpy(&context->buffer[j], &data[i], len - i);
}


/*
 * Add padding and return the message digest.
 */
void VChecksumSHA1::SHA1Pad(SHA1_CTX *context)
{
	uBYTE finalcount[8];
	uLONG i;

	for (i = 0; i < 8; i++)
	{
		finalcount[i] = (uBYTE)((context->count >> ((7 - (i & 7)) * 8)) & 255);	/* Endian independent */
	}
	SHA1Update(context, (uBYTE *)"\200", 1);
	while ((context->count & 504) != 448)
		SHA1Update(context, (uBYTE *)"\0", 1);
	SHA1Update(context, finalcount, 8); /* Should cause a SHA1Transform() */
}


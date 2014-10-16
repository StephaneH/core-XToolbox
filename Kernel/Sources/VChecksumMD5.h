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
#ifndef __VChecksumMD5__
#define __VChecksumMD5__

BEGIN_TOOLBOX_NAMESPACE

class VString;

/* Data structure for MD5 (Message-Digest) computation */
const size_t MD5_SIZE = 16;
typedef uBYTE MD5[MD5_SIZE];

class XTOOLBOX_API VChecksumMD5 : public VObject
{
public:

	typedef MD5	digest_type;

	// common checksum computation
	static	void	GetChecksumFromBytes( const void *inData, size_t inSize, MD5& outChecksum);
	static	void	GetChecksumFromBytesHexa( const void *inData, size_t inSize, VString& outChecksumHexa);

	static	void	GetChecksumFromStringUTF8( const VString& inString, MD5& outChecksum);
	static	void	GetChecksumFromStringUTF8Hexa( const VString& inString, VString& outChecksumHexa);

	static	void	EncodeChecksumHexa( const MD5& inDigest, VString& outChecksumHexa);
	static	void	EncodeChecksumBase64( const MD5& inDigest, VString& outChecksumBase64);

	// incremental computation
					VChecksumMD5();
			void	Clear();
			void	Update( const void *inData, size_t inSize );
			void	GetChecksum( MD5& outChecksum );
	
private:
					VChecksumMD5( const VChecksumMD5&);
					VChecksumMD5& operator=( const VChecksumMD5&);
			void	_MD5Init();
			void	_MD5Update( const uBYTE *inData, size_t inSize);
	static	void	_Transform( uLONG *inData, uLONG *inSize);

	struct {
		uLONG i[2];
		uLONG buf[4];
		unsigned char in[64];
	}	fContext;
			bool	fFinalized;
};


const size_t	SHA1_SIZE = 20;
typedef uBYTE SHA1[SHA1_SIZE];

class XTOOLBOX_API VChecksumSHA1 : public VObject
{
public:

	typedef SHA1	digest_type;

	// common checksum computation
	static	void	GetChecksumFromBytes( const void *inData, size_t inSize, SHA1& outChecksum);
	static	void	GetChecksumFromBytesHexa( const void *inData, size_t inSize, VString& outChecksumHexa);

	static	void	GetChecksumFromStringUTF8( const VString& inString, SHA1& outChecksum);
	static	void	GetChecksumFromStringUTF8Hexa( const VString& inString, VString& outChecksumHexa);

	static	void	EncodeChecksumHexa( const SHA1& inDigest, VString& outChecksumHexa);
	static	void	EncodeChecksumBase64( const SHA1& inDigest, VString& outChecksumBase64);

	// incremental computation
					VChecksumSHA1();
			void	Clear();
			void	Update( const void *inData, size_t inSize );
			void	GetChecksum( SHA1& outChecksum );
	
private:
	enum { SHA1_BLOCK_LENGTH = 64};

	typedef struct {
		uLONG state[5];
		uLONG8 count;
		uBYTE buffer[SHA1_BLOCK_LENGTH];
	} SHA1_CTX;

					VChecksumSHA1( const VChecksumSHA1&);
					VChecksumSHA1& operator=( const VChecksumSHA1&);
	static	void	SHA1Init(SHA1_CTX *context);
	static	void	SHA1Transform(uLONG state[5], const uBYTE buffer[SHA1_BLOCK_LENGTH]);
	static	void	SHA1Update(SHA1_CTX *context, const uBYTE *data, size_t len);
	static	void	SHA1Pad(SHA1_CTX *context);

			SHA1_CTX	fContext;
			bool		fFinalized;
};


END_TOOLBOX_NAMESPACE

#endif
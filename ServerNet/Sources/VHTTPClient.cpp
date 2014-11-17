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

#include "VServerNetPrecompiled.h"
#include "VHTTPClient.h"
#include "VProxyManager.h"
#include "Tools.h"
#include "VSslDelegate.h"
#include <cctype>
#include <zconf.h>
#include <zlib.h>


USING_TOOLBOX_NAMESPACE


#define HTTP_MAX_BUFFER_LENGTH								8192L
#define HTTP_CLIENT_LINE_BUFFER_SIZE						1024L

const XBOX::VSize HTTP_CLIENT_BUFFER_SIZE					= 32768;

/*
 *	For DIGEST Challenges
 */
#define HASHLEN												16
#define HASHHEXLEN											32
typedef unsigned char HASH[HASHLEN];
typedef unsigned char HASHHEX[HASHHEXLEN+1];


const sLONG DEFAULT_HTTP_CONNECTION_TIMEOUT					= 120;		/*  2 mins in seconds */
const sLONG	DEFAULT_HTTP_READ_WRITE_TIMEOUT					= 3600;		/* 60 mins in seconds */
const sLONG DEFAULT_HTTP_MAX_REDIRECTIONS					= 2;
const sLONG DEFAULT_HTTP_PORT								= 80;
const sLONG DEFAULT_HTTPS_PORT								= 443;

const XBOX::VString	HTTP_CLIENT_DEFAULT_USER_AGENT			= CVSTR ("4D_HTTP_Client");

#if WITH_HTTP_CLIENT_DEBUG_LOG
const char REQUEST_MARKER_STRING[] =  "\r\n----- REQUEST  -----\r\n";
const char RESPONSE_MARKER_STRING[] = "\r\n----- RESPONSE -----\r\n";
#endif

/*
 *	Some Common HTTP Header Names
 */
const XBOX::VString CONST_STRING_ACCEPT_ENCODING			= CVSTR ("Accept-Encoding");
const XBOX::VString CONST_STRING_AUTHORIZATION				= CVSTR ("Authorization");
const XBOX::VString CONST_STRING_CONNECTION					= CVSTR ("Connection");
const XBOX::VString CONST_STRING_CONTENT_ENCODING			= CVSTR ("Content-Encoding");
const XBOX::VString CONST_STRING_CONTENT_TYPE				= CVSTR ("Content-Type");
const XBOX::VString CONST_STRING_DATE						= CVSTR ("Date");
const XBOX::VString CONST_STRING_HOST						= CVSTR ("Host");
const XBOX::VString CONST_STRING_LOCATION					= CVSTR ("Location");
const XBOX::VString CONST_STRING_PROXY_AUTHENTICATE			= CVSTR ("Proxy-Authenticate");
const XBOX::VString CONST_STRING_PROXY_AUTHORIZATION		= CVSTR ("Proxy-Authorization");
const XBOX::VString CONST_STRING_TRANSFER_ENCODING			= CVSTR ("Transfer-Encoding");
const XBOX::VString CONST_STRING_USER_AGENT					= CVSTR ("User-Agent");
const XBOX::VString CONST_STRING_WWW_AUTHENTICATE			= CVSTR ("WWW-Authenticate");

/*
 *	Some common HTTP Header Values
 */
const XBOX::VString CONST_STRING_APPLICATION_JSON			= CVSTR ("application/json");
const XBOX::VString CONST_STRING_APPLICATION_OCTET_STREAM	= CVSTR ("application/octet-stream");
const XBOX::VString CONST_STRING_APPLICATION_SOAP_XML		= CVSTR ("application/soap+xml");
const XBOX::VString CONST_STRING_APPLICATION_XML			= CVSTR ("application/xml");
const XBOX::VString CONST_STRING_CHUNKED					= CVSTR ("chunked");
const XBOX::VString CONST_STRING_CLOSE						= CVSTR ("Close");
const XBOX::VString CONST_STRING_DEFLATE					= CVSTR ("deflate");
const XBOX::VString CONST_STRING_GZIP						= CVSTR ("gzip");
const XBOX::VString CONST_STRING_IMAGE_GIF					= CVSTR ("image/gif");
const XBOX::VString CONST_STRING_IMAGE_JPEG					= CVSTR ("image/jpeg");
const XBOX::VString CONST_STRING_KEEP_ALIVE					= CVSTR ("Keep-Alive");
const XBOX::VString CONST_STRING_NO_CACHE					= CVSTR ("no-cache");
const XBOX::VString CONST_STRING_TEXT_HTML					= CVSTR ("text/html");
const XBOX::VString CONST_STRING_TEXT_PLAIN					= CVSTR ("text/plain");
const XBOX::VString CONST_STRING_TEXT_XML					= CVSTR ("text/xml");
const XBOX::VString CONST_STRING_SUPPORTED_ENCODING			= CVSTR ("gzip, deflate");
const XBOX::VString CONST_STRING_UPGRADE					= CVSTR ("Upgrade");

/*
 *	 Some common Authentication Methods
 */
const XBOX::VString CONST_STRING_AUTHENTICATION_BASIC		= CVSTR ("basic");
const XBOX::VString CONST_STRING_AUTHENTICATION_DIGEST		= CVSTR ("digest");
const XBOX::VString CONST_STRING_AUTHENTICATION_NTLM		= CVSTR ("ntlm");


//--------------------------------------------------------------------------------------------------


#define LOWERCASE(c) \
	if ((c >= CHAR_LATIN_CAPITAL_LETTER_A) && (c <= CHAR_LATIN_CAPITAL_LETTER_Z))\
	c += 0x0020


template <typename T1, typename T2>
inline bool _EqualASCIIString (const T1 *const inCString1, const sLONG inString1Len, const T2 *const inCString2, const sLONG inString2Len, bool isCaseSensitive)
{
	if (!inCString1 || !inCString2 || (inString1Len != inString2Len))
		return false;
	else if (!(*inCString1) && !(*inCString2) && (inString1Len == 0))
		return true;

	bool result = false;
	const T1 *p1 = inCString1;
	const T2 *p2 = inCString2;

	while (*p1)
	{
		if ((NULL != p1) && (NULL != p2))
		{
			T1 c1 = *p1;
			T2 c2 = *p2;

			if (!isCaseSensitive)
			{
				LOWERCASE (c1);
				LOWERCASE (c2);
			}

			if (c1 == c2)
				result = true;
			else
				return false;
			p1++;
			p2++;
		}
		else
		{
			return false;
		}
	}

	return result;
}


template <typename T1, typename T2>
inline sLONG _FindASCIIString (const T1 *inText, const sLONG inTextLen, const T2 *inPattern, const sLONG inPatternLen, bool isCaseSensitive)
{
	// see http://fr.wikipedia.org/wiki/Algorithme_de_Knuth-Morris-Pratt

	if (inPatternLen > inTextLen)
		return 0;

	sLONG	textSize = inTextLen;
	sLONG	patternSize = inPatternLen;
	sLONG *	target = (sLONG *)malloc (sizeof(sLONG) * (patternSize + 1));

	if (NULL != target)
	{
		sLONG	m = 0;
		sLONG	i = 0;
		sLONG	j = -1;
		T2		c = '\0';

		target[0] = j;
		while (i < patternSize)
		{
			T2 pchar = inPattern[i];

			if (!isCaseSensitive)
				LOWERCASE (pchar);

			if (pchar == c)
			{
				target[i + 1] = j + 1;
				++j;
				++i;
			}
			else if (j > 0)
			{
				j = target[j];
			}
			else
			{
				target[i + 1] = 0;
				++i;
				j = 0;
			}

			pchar = inPattern[j];
			if (!isCaseSensitive)
				LOWERCASE (pchar);

			c = pchar;
		}

		m = 0;
		i = 0;
		while (((m + i) < textSize) && (i < patternSize))
		{
			T1		tchar = inText[m + i];
			T2		pchar = inPattern[i];

			if (!isCaseSensitive)
			{
				LOWERCASE (tchar);
				LOWERCASE (pchar);
			}

			if (tchar == (T1)pchar)
			{
				++i;
			}
			else
			{
				m += i - target[i];
				if (i > 0)
					i = target[i];
			}
		}

		free (target);

		if (i >= patternSize)
			return m + 1; // 1-based position !!! Just to work as VString.Find()
	}

	return 0;
}


static
bool EqualASCIIString (const XBOX::VString& inString1, const XBOX::VString& inString2, bool isCaseSensitive = false)
{
	return _EqualASCIIString<UniChar, UniChar> (inString1.GetCPointer(), inString1.GetLength(), inString2.GetCPointer(), inString2.GetLength(), isCaseSensitive);
}


static
bool EqualASCIIString (const XBOX::VString& inString1, const char *const inCString2, bool isCaseSensitive = false)
{
	return _EqualASCIIString<UniChar, char> (inString1.GetCPointer(), inString1.GetLength(), inCString2, (sLONG)strlen (inCString2), isCaseSensitive);
}


/*static
sLONG FindASCIIString (const XBOX::VString& inText, const XBOX::VString& inPattern, bool isCaseSensitive = false)
{
	return _FindASCIIString<UniChar, UniChar> (inText.GetCPointer(), inText.GetLength(), inPattern.GetCPointer(), inPattern.GetLength(), isCaseSensitive);
}*/


static
sLONG FindASCIIString (const XBOX::VString& inText, const char *inPattern, bool isCaseSensitive = false)
{
	return _FindASCIIString<UniChar, char> (inText.GetCPointer(), inText.GetLength(), inPattern, (sLONG)strlen (inPattern), isCaseSensitive);
}


static
void _MakeNetAddressString (const XBOX::VString& inDNSNameOrIP, const sLONG inPort, XBOX::VString& outAddressString)
{
	outAddressString.FromString (inDNSNameOrIP);
	outAddressString.AppendUniChar (CHAR_COLON);
	outAddressString.AppendLong (inPort);
}


static
void _GetMethodName (HTTP_Method inMethod, XBOX::VString& outMethodName)
{
	switch (inMethod)
	{
	case HTTP_GET:
		outMethodName.FromCString ("GET");
		break;

	case HTTP_POST:
		outMethodName.FromCString ("POST");
		break;

	case HTTP_HEAD:
		outMethodName.FromCString ("HEAD");
		break;

	case HTTP_TRACE:
		outMethodName.FromCString ("TRACE");
		break;

	case HTTP_OPTIONS:
		outMethodName.FromCString ("OPTIONS");
		break;

	case HTTP_DELETE:
		outMethodName.FromCString ("DELETE");
		break;

	case HTTP_PUT:
		outMethodName.FromCString ("PUT");
		break;

	default:
		outMethodName.Clear();
		break;
	}
}


static
XBOX::VSize _GetChunkSize (char *inDataPtr)
{
	char *		digit = inDataPtr;
	XBOX::VSize chunkSize = 0;

	while (*digit == ' ')
		digit++;

	if ((*digit == '0') && (*(digit+1) == 'x' || *(digit+1) == 'X'))
		digit += 2;

	while (true)
	{
		if ((*digit >= '0') && (*digit <= '9'))
			chunkSize = (chunkSize << 4) + (*digit - '0');
		else if ((*digit >= 'a') && (*digit <= 'f'))
			chunkSize = (chunkSize << 4) + (*digit + 10 - 'a');
		else if ((*digit >= 'A') && (*digit <= 'F'))
			chunkSize = (chunkSize << 4) + (*digit + 10 - 'A');
		else
			break;

		digit++;
	}

	return chunkSize;
}


static
void _ExtractAuthorizationHeaderValues (const XBOX::VString& inString, XBOX::VString& outAuthMethodString, XBOX::VNameValueCollection *outParameters)
{
	/*
	 *	Extract different values and parameters from Authorization header.
	 *	This is the kind of string we may receive:
	 *
	 *	Digest username="toto", realm="test web.4DB", nonce="1068215347:7307f04f919115ff129c7f2d9fb0214c",
	 *	uri="/dfghgdfhfffrdfgh", response="6179a3695580ec07375545dc7c03f960", algorithm="MD5",
	 *	cnonce="750c89c5ce15dee24dffffa38703b492", nc=00000008, qop="auth"
	 */
	const UniChar *	it  = inString.GetCPointer();
	const UniChar *	end = it + inString.GetLength();

	outAuthMethodString.Clear();

	while ((it != end) && std::isspace(*it))
		++it;

	while ((it != end) && !std::isspace(*it))
		outAuthMethodString.AppendUniChar (*it++);

	outAuthMethodString.TrimeSpaces();
	if (it != end)
		++it;

	if (NULL != outParameters)
	{
		outParameters->clear();
		XBOX::VHTTPHeader::SplitParameters (it, end, *outParameters, false, CHAR_COMMA);
	}
}


static
void _CreateNewSessionID (XBOX::VString& outNonce)
{
	XBOX::VChecksumMD5	md5;
	MD5					digest;

	/* 
	 *	Need some entropy ? Here is some:
	 */
	sLONG				l1 = XBOX::VSystem::Random();
	uLONG				l2 = XBOX::VSystem::GetCurrentTime();
	uBYTE				l3[117];

	md5.Update (&l1, sizeof (l1));
	md5.Update (&l2, sizeof (l2));
	md5.Update (l3, sizeof (l3));
	md5.GetChecksum (digest);

	XBOX::VChecksumMD5::EncodeChecksumHexa (digest, outNonce);
}


static
void _DigestCalcHA1 (	unsigned char *pszAlg,
						unsigned char *pszUserName,
						unsigned char *pszRealm,
						unsigned char *pszPassword,
						unsigned char *pszNonce,
						unsigned char *pszCNonce,
						HASHHEX SessionKey)
{
	XBOX::VChecksumMD5	md5;
	HASH				HA1;
	XBOX::VString		sessionKeyString;

	md5.Update (pszUserName, strlen ((char *)pszUserName));
	md5.Update ((unsigned char *)":", 1);
	md5.Update (pszRealm, strlen ((char *)pszRealm));
	md5.Update ((unsigned char *)":", 1);
	md5.Update (pszPassword, strlen ((char *)pszPassword));
	md5.GetChecksum (HA1);
	if (!strcmp ((char *)pszAlg, "md5-sess"))
	{
		md5.Clear();
		md5.Update (HA1, HASHLEN);
		md5.Update ((unsigned char *)":", 1);
		md5.Update (pszNonce, strlen ((char *)pszNonce));
		md5.Update ((unsigned char *)":", 1);
		md5.Update (pszCNonce, strlen ((char *)pszCNonce));
		md5.GetChecksum (HA1);
	}
	XBOX::VChecksumMD5::EncodeChecksumHexa (HA1, sessionKeyString);
	sessionKeyString.ToCString ((char *)SessionKey, sizeof (HASHHEX));
}


static
void _DigestCalcResponse (	HASHHEX HA1,
							unsigned char *pszNonce,
							unsigned char *pszNonceCount,
							unsigned char *pszCNonce,
							unsigned char *pszQop,
							unsigned char *pszMethod,
							unsigned char *pszDigestUri,
							HASHHEX HEntity,
							HASHHEX Response)
{
	XBOX::VChecksumMD5	md5;
	HASH				HA2;
	HASH				RespHash;
	HASHHEX				HA2Hex;
	XBOX::VString		HA2String;
	XBOX::VString		RespHashString;

	md5.Update (pszMethod, strlen ((char *)pszMethod));
	md5.Update ((unsigned char *)":", 1);
	md5.Update (pszDigestUri, strlen ((char *)pszDigestUri));

	if (!strcmp ((char *)pszQop, "auth-int"))
	{
		md5.Update ((unsigned char *)":", 1);
		md5.Update (HEntity, HASHHEXLEN);
	}
	md5.GetChecksum (HA2);
	XBOX::VChecksumMD5::EncodeChecksumHexa (HA2, HA2String);
	HA2String.ToCString ((char *)HA2Hex, sizeof (HASHHEX));
	HA2String.Clear();

	md5.Clear();
	md5.Update (HA1, HASHHEXLEN);
	md5.Update ((unsigned char *)":", 1);
	md5.Update (pszNonce, strlen ((char *)pszNonce));
	md5.Update ((unsigned char *)":", 1);

	if (*pszQop)
	{
		md5.Update (pszNonceCount, strlen ((char *)pszNonceCount));
		md5.Update ((unsigned char *)":", 1);
		md5.Update (pszCNonce, strlen ((char *)pszCNonce));
		md5.Update ((unsigned char *)":", 1);
		md5.Update (pszQop, strlen ((char *)pszQop));
		md5.Update ((unsigned char *)":", 1);
	}
	md5.Update (HA2Hex, HASHHEXLEN);
	md5.GetChecksum (RespHash);
	XBOX::VChecksumMD5::EncodeChecksumHexa (RespHash, RespHashString);
	RespHashString.ToCString ((char *)Response, sizeof (HASHHEX));
}


static
void _CalcDigestPasswordChallenge (	const XBOX::VString& inUserName,
									const XBOX::VString& inUserPassword,
									const XBOX::VString& inAlgorithm,
									const XBOX::VString& inRealm,
									const XBOX::VString& inNonce,
									const XBOX::VString& inCNonce,
									const XBOX::VString& inQOP,
									const XBOX::VString& inNonceCount,
									const XBOX::VString& inURi,
									const XBOX::VString& inMethod,
									XBOX::VString& outResponse )
{
	unsigned char uname[256], passwd[256], algorithm[256],realm[256], nonce[256], cnonce[256], uri[256], qop[256], nonceCount[256], methodName[256];

	inUserName.ToCString( (char*)uname, sizeof(uname));
	inUserPassword.ToCString( (char*)passwd, sizeof(passwd));
	inAlgorithm.ToCString( (char*)algorithm, sizeof(algorithm));
	inRealm.ToCString( (char*)realm, sizeof(realm));
	inNonce.ToCString( (char*)nonce, sizeof(nonce));
	inCNonce.ToCString( (char*)cnonce, sizeof(cnonce));
	inQOP.ToCString( (char*)qop, sizeof(qop));
	inNonceCount.ToCString( (char*)nonceCount, sizeof(nonceCount));
	inURi.ToCString( (char*)uri, sizeof(uri));
	inMethod.ToCString( (char*)methodName, sizeof(methodName));

			
	HASHHEX HA1 = {0};
	HASHHEX HA2 = {0};
	HASHHEX response = {0};

	_DigestCalcHA1( algorithm, uname, realm, passwd, nonce, cnonce, HA1);
	_DigestCalcResponse( HA1, nonce, nonceCount, cnonce, qop, methodName, uri, HA2, response);

	outResponse.FromCString ((char *)response);
}


//--------------------------------------------------------------------------------------------------

/*
 *	Stolen from Zip Component
 */


const VError	VE_SRVR_ZIP_BAD_VERSION					= MAKE_VERROR (kSERVER_NET_SIGNATURE, 500);
const VError	VE_SRVR_ZIP_DECOMPRESSION_FAILED		= MAKE_VERROR (kSERVER_NET_SIGNATURE, 501);
const VError	VE_SRVR_ZIP_NON_COMPRESSED_INPUT_DATA	= MAKE_VERROR (kSERVER_NET_SIGNATURE, 502);


static inline
VError _ExpandMemoryBlock (void *inCompressedBlock, sLONG8 inBlockSize, XBOX::VStream& outExpandedStream)
{
	if ((inCompressedBlock == NULL) || (inBlockSize < 0))
		return vThrowError (VE_INVALID_PARAMETER);

	if (!outExpandedStream.IsWriting())
		return vThrowError (VE_STREAM_NOT_OPENED);

	VError errorToReturn = VE_OK;

	if (errorToReturn == VE_OK)
	{
		sLONG8 bufferMaxSize = 1000000;

		/*
		 *	the decompression will be done by steps if needed, decompressing data until the expanded buffer becomes full each time.
		 */
		sLONG8 expandedBufferLength = bufferMaxSize;
		Bytef * expandedBuffer = (Bytef *) malloc(expandedBufferLength  * sizeof(Bytef));
		if (expandedBuffer == NULL)
			errorToReturn = vThrowError (VE_MEMORY_FULL);

		if (errorToReturn == VE_OK)
		{
			/*
			 *	initialization of zlib structure. For info:
			 *	- next_in/out =  adress of inCompressedBlock/expandedBuffer
			 *	- avail_in = number of bytes available in the inCompressedBlock
			 *	- avail_out = remaining free space in the expandedBuffer
			 *	- total_in = total size of data already expanded
			 *	- total_out = total size of expanded data
			 */
			z_stream zlib_compressedStream;
			memset(&zlib_compressedStream,0,sizeof(z_stream));
			
			zlib_compressedStream.next_in = (Bytef *)inCompressedBlock;
			zlib_compressedStream.avail_in = inBlockSize;
			zlib_compressedStream.total_in = 0;
			zlib_compressedStream.next_out = expandedBuffer;
			zlib_compressedStream.avail_out = (uInt)expandedBufferLength;
			zlib_compressedStream.zalloc = Z_NULL;
			zlib_compressedStream.zfree = Z_NULL;
			zlib_compressedStream.opaque = Z_NULL;
			
			/*
			 *	initializing decompression
			 *	JQ 05/02/2009: we need to add 32 to default windowBits
			 *					in order to enable gzip (+zlib) inflating
			 *				  (with 15 inflate can inflate only zlib stream)
			 *				  (SVG component need gzip inflating in order to inflate svgz files
			 *				   which are gzip compressed)
			 */
			int decompressionError = inflateInit2 (&zlib_compressedStream, 15+32);
			
			if (decompressionError == Z_OK)
			{
				do
				{
					decompressionError = inflate (&zlib_compressedStream, Z_SYNC_FLUSH);
					
					/*
					 *	the remaining size of data to expand must always be ranging between 0 and blockSize.
					 */
					xbox_assert (zlib_compressedStream.total_in >= 0);
					xbox_assert (zlib_compressedStream.total_in <= inBlockSize);

					/*
					 *	the remaining free space in the expandedBuffer must always be ranging between 0 and expandedBufferLength.
					 */
					xbox_assert (zlib_compressedStream.avail_out >= 0);
					xbox_assert (zlib_compressedStream.avail_out <= expandedBufferLength);

					switch (decompressionError)
					{
						case Z_OK:
						case Z_STREAM_END: 	
							/*	if there is some expanded data in the expanded buffer, they must be written in the expandedStream. */
							if ((expandedBufferLength - zlib_compressedStream.avail_out) > 0)
							{
								errorToReturn = outExpandedStream.PutData (expandedBuffer, (expandedBufferLength - zlib_compressedStream.avail_out));
								if (errorToReturn != VE_OK)
								{
									errorToReturn = vThrowError (VE_STREAM_CANNOT_WRITE);
									break;
								}
								/* we update next_out and avail_out with the recent emptied buffer. */
								zlib_compressedStream.next_out = expandedBuffer;
								zlib_compressedStream.avail_out = (uInt)expandedBufferLength;
							} 
							break;

							// error cases
						case Z_DATA_ERROR:
							errorToReturn = vThrowError (VE_SRVR_ZIP_NON_COMPRESSED_INPUT_DATA);
							break;
							
						case Z_VERSION_ERROR:
							errorToReturn = vThrowError (VE_SRVR_ZIP_BAD_VERSION);
							break;
							
						case Z_BUF_ERROR:	
						case Z_NEED_DICT:
						case Z_STREAM_ERROR:
						case Z_MEM_ERROR:
							errorToReturn = vThrowError (VE_SRVR_ZIP_DECOMPRESSION_FAILED);
							break;	
					}
					
				}
				/*
				 *	we stop looping if there are no more data to expand, or if an error occured in decompression
				 */
				while ((errorToReturn == VE_OK) && (decompressionError != Z_STREAM_END));
				/*
				 *	DH 22-Feb-2013 In case the blob is corrupted we have to stop on Z_STREAM_END even if blockSize>zlib_compressedStream.total_in 
				 *	otherwise we loop indefinitely (see http://www.zlib.net/zlib_how.html for usage example)
				 */

				inflateEnd (&zlib_compressedStream);	// YT 13-Mar-2009 - ACI0060740
			}
			/*
			 *	if the initilization of decompression failed.
			 */
			else
			{
				errorToReturn = vThrowError (VE_SRVR_ZIP_DECOMPRESSION_FAILED);
			}
		}

		if (expandedBuffer != NULL)
			free (expandedBuffer);
	}
	
	return errorToReturn;
}


//--------------------------------------------------------------------------------------------------


static
XBOX::VError _ReadWithTimeoutFromEndPoint(XBOX::VTCPEndPoint *inEndPoint, void *ioBuffer, uLONG *ioBufferSize, uLONG inTimeoutMS)
{
	if ((NULL == inEndPoint) || (NULL == ioBuffer) || (NULL == ioBufferSize))
		return XBOX::vThrowError(VE_INVALID_PARAMETER);

	XBOX::VError	error = XBOX::VE_UNKNOWN_ERROR;
	const sLONG		READ_TIMEOUT_MS = XBOX::Min<sLONG>(inTimeoutMS, 1000);
	uLONG			startTime = XBOX::VSystem::GetCurrentTime();

	/*
	 *	Loop until:
	 *	-	ReadWithTimeout returns VE_OK or a true error (not VE_SRVR_READ_TIMED_OUT)
	 *	-	Read Timeout expired
	 *	-	Current Task died
	 */
	while ((XBOX::VSystem::GetCurrentTime() - startTime) <= inTimeoutMS)
	{
		error = inEndPoint->ReadWithTimeout(ioBuffer, ioBufferSize, READ_TIMEOUT_MS);

		/* There were no error or an error != VE_SRVR_READ_TIMED_OUT occured */
		if (VE_SRVR_READ_TIMED_OUT != error)
			break;

		/* Current task is dying */
		if (XBOX::VTask::GetCurrent()->IsDying())
			break;

		XBOX::VTask::Yield();
	}

	return error;
}


//--------------------------------------------------------------------------------------------------


VAuthInfos& VAuthInfos::operator = (const VAuthInfos& inAuthenticationInfos)
{
	if (&inAuthenticationInfos != this)
	{
		fUserName.FromString (inAuthenticationInfos.fUserName);
		fPassword.FromString (inAuthenticationInfos.fPassword);
		fRealm.FromString (inAuthenticationInfos.fRealm);
		fURI.FromString (inAuthenticationInfos.fURI);
		fDomain.FromString (inAuthenticationInfos.fDomain);
		fAuthenticationMethod = inAuthenticationInfos.fAuthenticationMethod;
	}

	return *this;
}


void VAuthInfos::Clear()
{
	fUserName.Clear();
	fPassword.Clear();
	fRealm.Clear();
	fURI.Clear();
	fDomain.Clear();
	fAuthenticationMethod = AUTH_NONE;
}


//--------------------------------------------------------------------------------------------------


VAuthInfos						VHTTPClient::fSavedHTTPAuthenticationInfos;
VAuthInfos						VHTTPClient::fSavedProxyAuthenticationInfos;
XBOX::VString					VHTTPClient::fUserAgent;


VHTTPClient::VHTTPClient()
: fHeader()
, fResponseHeader()
, fUseSSL (false)
, fRequestMethod (HTTP_GET)
, fRequestBody()
, fResponseHeaderBuffer()
, fLeftOver()
, fResponseBody()
, fProgressionCallBackPtr (NULL)
, fProgressionCallBackPrivateData (NULL)
, fAuthenticationDialogCallBackPtr (NULL)
, fAuthenticationDialogCallBackPrivateData (NULL)
, fDomain()
, fFolder()
, fQuery()
, fProxy()
, fProxyPort (DEFAULT_HTTP_PORT)
, fConnectionTimeout (DEFAULT_HTTP_CONNECTION_TIMEOUT)
, fReadWriteTimeout (DEFAULT_HTTP_READ_WRITE_TIMEOUT)
, fPort (DEFAULT_HTTP_PORT)
, fUseProxy (false)
, fNTLMAuthenticationInProgress (false)
, fHTTPAuthenticationInfos ()
, fProxyAuthenticationInfos ()
, fResetAuthenticationInfos (false)
, fMayUsePreAuthentication(false)
, fUseAuthentication (false)
, fStatusCode (0)
, fShowAuthenticationDialog (false)
, fWithTimeStamp (true)
, fWithHost (true)
, fKeepAlive (false)
, fUpgradeRequest (false)
, fContentType()
, fTCPEndPoint (NULL)
, fNumberOfRequests (0)
, fUseHTTPCompression (false)
, fFollowRedirect (true)
, fMaxRedirections (DEFAULT_HTTP_MAX_REDIRECTIONS)
#if WITH_HTTP_CLIENT_DEBUG_LOG
, fLogFile(NULL)
, fLogFileDesc(NULL)
#endif
{
}


VHTTPClient::VHTTPClient (const XBOX::VURL& inURL, const XBOX::VString& inContentType, bool inWithTimeStamp, bool inWithKeepAlive, bool inWithHost, bool inWithAuthentication)
: fHeader()
, fResponseHeader()
, fUseSSL (false)
, fRequestMethod (HTTP_GET)
, fRequestBody()
, fResponseHeaderBuffer()
, fLeftOver()
, fResponseBody()
, fProgressionCallBackPtr (NULL)
, fProgressionCallBackPrivateData (NULL)
, fAuthenticationDialogCallBackPtr (NULL)
, fAuthenticationDialogCallBackPrivateData (NULL)
, fDomain()
, fFolder()
, fQuery()
, fProxy()
, fProxyPort (DEFAULT_HTTP_PORT)
, fConnectionTimeout (DEFAULT_HTTP_CONNECTION_TIMEOUT)
, fReadWriteTimeout (DEFAULT_HTTP_READ_WRITE_TIMEOUT)
, fPort (DEFAULT_HTTP_PORT)
, fUseProxy (false)
, fNTLMAuthenticationInProgress (false)
, fHTTPAuthenticationInfos ()
, fProxyAuthenticationInfos ()
, fResetAuthenticationInfos (false)
, fMayUsePreAuthentication(false)
, fUseAuthentication (false)
, fStatusCode (0)
, fShowAuthenticationDialog (false)
, fWithTimeStamp (true)
, fWithHost (true)
, fKeepAlive (false)
, fUpgradeRequest (false)
, fContentType()
, fTCPEndPoint (NULL)
, fNumberOfRequests (0)
, fUseHTTPCompression (false)
, fFollowRedirect (true)
, fMaxRedirections (DEFAULT_HTTP_MAX_REDIRECTIONS)
#if WITH_HTTP_CLIENT_DEBUG_LOG
, fLogFile(NULL)
, fLogFileDesc(NULL)
#endif
{
	Init (inURL, inContentType, inWithTimeStamp, inWithKeepAlive, inWithHost, inWithAuthentication);
}


void VHTTPClient::Init (const XBOX::VURL& inURL, const XBOX::VString& inContentType, bool inWithTimeStamp, bool inWithKeepAlive, bool inWithHost, bool inWithAuthentication)
{
	fContentType.FromString (inContentType);
	fWithTimeStamp = inWithTimeStamp;
	fKeepAlive = inWithKeepAlive;
	fWithHost = inWithHost;
	fUseAuthentication = inWithAuthentication;

	_ParseURL (inURL);

	// HTTP Authentication Infos...
	XBOX::VString	uri;
	XBOX::VString	domain;

	if (!fFolder.IsEmpty())
		uri.FromString (fFolder);

	_ComputeDomain (domain, fUseProxy);
	if (fUseAuthentication &&
		fSavedHTTPAuthenticationInfos.GetURI().EqualToString (uri) &&
		fSavedHTTPAuthenticationInfos.GetDomain().EqualToString (domain))
	{
		fHTTPAuthenticationInfos = fSavedHTTPAuthenticationInfos;
	}
	else
	{
		fSavedHTTPAuthenticationInfos.Clear();
	}

	// Proxy Authentication Infos
	if (fUseAuthentication && EqualASCIIString (fProxyAuthenticationInfos.GetDomain(), XBOX::VProxyManager::GetProxyServerAddress()))
	{
		fProxyAuthenticationInfos = fSavedProxyAuthenticationInfos;
	}
	else
	{
		fSavedProxyAuthenticationInfos.Clear();
	}

#if WITH_HTTP_CLIENT_DEBUG_LOG
	XBOX::VFolder *	folder = new XBOX::VFolder(VProcess::Get()->GetExecutableFolderPath());

	if (NULL != folder)
	{
		XBOX::VError	error = XBOX::VE_OK;
		XBOX::VString	fileName("REQ_");
		fileName.AppendLong(XBOX::VSystem::GetCurrentTime());
		fileName.AppendCString(".txt");

		fLogFile = new XBOX::VFile(*folder, fileName);
		if (NULL != fLogFile)
			error = fLogFile->Open(XBOX::FA_READ_WRITE, &fLogFileDesc, XBOX::FO_CreateIfNotFound);

		if ((XBOX::VE_OK != error) || (NULL == fLogFileDesc))
		{
			XBOX::ReleaseRefCountable(&fLogFile);
			delete fLogFileDesc;
			fLogFileDesc = NULL;
		}

		XBOX::ReleaseRefCountable(&folder);
	}
#endif
}


VHTTPClient::~VHTTPClient()
{
	fHeader.Clear();
	fResponseHeader.Clear();

	fResponseHeaderBuffer.Clear();
	fRequestBody.Clear();
	fResponseBody.Clear();
	fLeftOver.Clear();

	CloseConnection();

#if WITH_HTTP_CLIENT_DEBUG_LOG
	XBOX::ReleaseRefCountable(&fLogFile);
	delete fLogFileDesc;
#endif
}


void VHTTPClient::Clear()
{
	_ReinitHTTP();
}


void VHTTPClient::_InitCustomHeaders()
{
	if (fUserAgent.IsEmpty())
	{
		XBOX::VString shortVersionString;
		XBOX::VProcess::Get()->GetShortProductVersion (shortVersionString);

		fUserAgent.FromString (HTTP_CLIENT_DEFAULT_USER_AGENT);
		fUserAgent.AppendUniChar (CHAR_SOLIDUS);
		fUserAgent.AppendString (shortVersionString);
	}

	// User-Agent
	if (!fHeader.IsHeaderSet (CONST_STRING_USER_AGENT)) // YT 21-Jun-2011 - ACI0071929
		fHeader.SetHeaderValue (CONST_STRING_USER_AGENT, fUserAgent, true);

	// Date
	if (fWithTimeStamp)
	{
		XBOX::VTime		currentTime;
		XBOX::VString	dateTimeString;

		XBOX::VTime::Now (currentTime);
		currentTime.GetRfc822String (dateTimeString);

		fHeader.SetHeaderValue (CONST_STRING_DATE, dateTimeString, true);
	}

	// Content-Type
	if (!fHeader.IsHeaderSet(CONST_STRING_CONTENT_TYPE) && !fContentType.IsEmpty())
		fHeader.SetHeaderValue (CONST_STRING_CONTENT_TYPE, fContentType, true);

	// Host
	if (!fHeader.IsHeaderSet (CONST_STRING_HOST))
		_SetHostHeader();
	
	// Connection
	SetConnectionKeepAlive (fKeepAlive);
}


/*static*/
void VHTTPClient::ClearSavedAuthenticationInfos()
{
	if (fSavedHTTPAuthenticationInfos.IsValid())
		fSavedHTTPAuthenticationInfos.Clear();
	if (fSavedProxyAuthenticationInfos.IsValid())
		fSavedProxyAuthenticationInfos.Clear();
}


XBOX::VError VHTTPClient::_OpenConnection (const XBOX::VString& inDNSNameOrIP, const sLONG inPort, bool inUseSSL, XBOX::VTCPSelectIOPool *inSelectIOPool)
{
	XBOX::VError error = XBOX::VE_OK;

	if (NULL == fTCPEndPoint)
	{
		sLONG			connectionTimeoutMS = XBOX::Abs(fConnectionTimeout) * 1000;
		XBOX::VError	connectionError = XBOX::VE_OK;

		fTCPEndPoint = XBOX::VTCPEndPointFactory::CreateClientConnection(inDNSNameOrIP, inPort, inUseSSL, true, connectionTimeoutMS, inSelectIOPool, connectionError);
		if (XBOX::VE_OK != connectionError)
		{
			error = XBOX::vThrowError(connectionError);
			XBOX::ReleaseRefCountable(&fTCPEndPoint);
		}
	}

	return error;
}

int a=eFK_Executable;


bool VHTTPClient::_ConnectionOpened()
{
	return (fTCPEndPoint != NULL);
}


XBOX::VError VHTTPClient::_PromoteToSSL()
{
	// For use with HTTP CONNECT tunneling. 
	// Connection is opened and forwarded by proxy in clear.
	// When CONNECT is done, both peers continue with SSL,
	// so the handshake must be done.

	if (NULL == fTCPEndPoint)
		return XBOX::vThrowError (XBOX::VE_MEMORY_FULL);
	
	XBOX::VKeyCertChain* clientCertificate = NULL;
	
#if WITH_CLIENT_CERTIFICATE
	clientCertificate = XBOX::ServerNetTools::RetainClientKeyCertChain();
#endif

	return fTCPEndPoint->PromoteToSSL(true, clientCertificate);

	SslFramework::ReleaseKeyCertChain(&clientCertificate);
}


bool VHTTPClient::_LastUsedAddressChanged (const XBOX::VString& inDNSNameOrIP, const sLONG inPort)
{
	XBOX::VString addressString;

	_MakeNetAddressString (inDNSNameOrIP, inPort, addressString);

	return !EqualASCIIString (addressString, fLastAddressUsed);
}


void VHTTPClient::_SaveLastUsedAddress (const XBOX::VString& inDNSNameOrIP, const sLONG inPort)
{
	_MakeNetAddressString (inDNSNameOrIP, inPort, fLastAddressUsed);
}


XBOX::VError VHTTPClient::OpenConnection (XBOX::VTCPSelectIOPool *inSelectIOPool)
{
	XBOX::VError			error = XBOX::VE_OK;
	XBOX::VString			dnsNameOrIP;
	sLONG					port = 0;
	bool					bSendConnectToProxy = false;

	if (!fProxy.IsEmpty() && !XBOX::VProxyManager::ByPassProxyOnLocalhost (fDomain))
	{
		/*
		 *	if there is a proxy, let's use it...
		 */
		dnsNameOrIP.FromString (fProxy);
		port = fProxyPort;

		/*
		 *	It's an HTTPs connection through a proxy server !!!
		 *	Let's send a CONNECT request to proxy (in HTTP) and promote connection to SSL mode if request succeed.
		 *	See: http://en.wikipedia.org/wiki/Hypertext_Transfer_Protocol#Request_methods :
		 *	CONNECT:	Converts the request connection to a transparent TCP/IP tunnel, usually to facilitate SSL-encrypted
		 *				communication (HTTPS) through an unencrypted HTTP proxy.[14][15]
		 */
		bSendConnectToProxy = fUseSSL;
	}
	else
	{
		dnsNameOrIP.FromString (fDomain);
		port = fPort;
	}

	if (XBOX::VE_OK == error)
	{
		bool addressChanged = ((fNumberOfRequests > 0) && _LastUsedAddressChanged (dnsNameOrIP, port));
		bool connectionReset = false;

		/*
		 *	In Keep-Alive, close Socket when address changed or when Server Keep-Alive limit is reached
		 */
		if (addressChanged || connectionReset || (fNumberOfRequests > 20))
		{
			CloseConnection();
			fNumberOfRequests = 0;
		}

		if ((fNumberOfRequests == 0) || (!_ConnectionOpened()) || addressChanged)
		{
			if (fNumberOfRequests == 0)
				_SaveLastUsedAddress (dnsNameOrIP, port);
		}
	}

	if (XBOX::VE_OK == error)
	{
		/*
		 *	Open Connection according fUseSSL flag except for SSL connection through proxy (always in HTTP)
		 */
		error = _OpenConnection (dnsNameOrIP, port, bSendConnectToProxy ? false : fUseSSL, inSelectIOPool);

		if (XBOX::VE_OK == error)
		{
			if (fNumberOfRequests > 0)
				fLeftOver.Clear();

			if (bSendConnectToProxy)
			{
				XBOX::VSize				bufferSize = HTTP_CLIENT_BUFFER_SIZE;
				char *					buffer = (char *) malloc (HTTP_CLIENT_BUFFER_SIZE);

				if (NULL == buffer)
					return XBOX::vThrowError (XBOX::VE_MEMORY_FULL);
				/*
				 *	Very special case: HTTPS through proxy
				 *	We need to send the CONNECT request (in clear),
				 *	get the response 200 (in clear)
				 *	and _then_ start the SSL connection (handshake + data send)
				 *	on the same socket (m.c)
				 */
				bool bWasProxyConnectSuccessful = false;
				
				error = _SendCONNECTToProxy();

				while (!bWasProxyConnectSuccessful && (XBOX::VE_OK == error) && (bufferSize > 0))
				{
					/*
					 *	Passing 0 to the NC means ReadLine (ie: until CRLF)
					 *	[Later] Strategy: we read line by line at the beginning (slow)
					 *	until we have the header
					 *	Then we extract Content-Length, if any, and it should be faster
					 *	(m.c)
					 */

					bufferSize = 0;
					error = _ReadFromSocket (buffer, HTTP_CLIENT_BUFFER_SIZE, bufferSize);

					if ((XBOX::VE_OK == error) && (bufferSize > 0))
					{
						error = fResponseHeaderBuffer.PutDataAmortized (fResponseHeaderBuffer.GetDataSize(), buffer, bufferSize) ? 0 : 1;

						if ((bufferSize == 2) && (buffer[0] == '\r') && (buffer[1] == '\n'))
							bWasProxyConnectSuccessful = (200 == _ExtractHTTPStatusCode());
					}
				}
				/*
				 *	Reinit the header buffer, used for the CONNECT
				 */
				fResponseHeaderBuffer.Clear();

				if (bWasProxyConnectSuccessful)
				{
					/*
					 *	the CONNECT was successful
					 */
					error = _PromoteToSSL();
				}

				free (buffer);
				buffer = NULL;
			}
		}
	}

	return error;
}


XBOX::VError VHTTPClient::CloseConnection()
{
	XBOX::VError error = XBOX::VE_OK;

	if (NULL != fTCPEndPoint)
	{
		error = fTCPEndPoint->Close();
		XBOX::ReleaseRefCountable (&fTCPEndPoint);
	}

	return error;
}

XBOX::VTCPEndPoint *VHTTPClient::StealEndPoint ()
{
	XBOX::VTCPEndPoint	*endPoint;

	if ((endPoint = fTCPEndPoint) != NULL)
	{
		fTCPEndPoint = NULL;
	}

	return endPoint;
}

XBOX::VError VHTTPClient::SendRequest (HTTP_Method inMethod)
{
	fResponseHeaderBuffer.Clear();

	XBOX::VError	error;

	if ((error = _GenerateRequest(inMethod)) == XBOX::VE_OK)

		error = _SendRequestHeader();

	return error;
}

XBOX::VError VHTTPClient::ReadResponseHeader ()
{	
	XBOX::VError	error;
	sLONG			tryCount;

	StartReadingResponseHeader();
	tryCount = 0;
	for ( ; ; ) {

		VSize	bufferSize;
		char	buffer[HTTP_CLIENT_BUFFER_SIZE];
		bool	isComplete;

		bufferSize = HTTP_CLIENT_BUFFER_SIZE - 1;
		isComplete = false;
		if ((error = _ReadFromSocket(buffer, HTTP_CLIENT_BUFFER_SIZE - 1, bufferSize)) == XBOX::VE_OK) {

			if (error == XBOX::VE_OK && !bufferSize) {

				// Nothing to read, wait a little bit. Maximum cumulative wait is 1 second.

				if (++tryCount < 1000 / 10)

					VTask::Sleep(10); 

				else {

					error = VE_SRVR_NOTHING_TO_READ;
					break;
					
				}

			} else if ((error = ContinueReadingResponseHeader(buffer, bufferSize, &isComplete)) != XBOX::VE_OK || isComplete)
				
				break;

			else

				tryCount = 0;	// Reset waiting counter.

		} else

			break;

	} 

	return error;
}

void VHTTPClient::StartReadingResponseHeader ()
{
	fResponseHeaderBuffer.Clear();
	fLeftOver.Clear();
}

XBOX::VError VHTTPClient::ContinueReadingResponseHeader (const char *inBuffer, VSize inBufferSize, bool *outIsComplete)
{
	if (!fResponseHeaderBuffer.PutDataAmortized(fResponseHeaderBuffer.GetDataSize(), inBuffer, inBufferSize)) 

		return XBOX::VE_MEMORY_FULL;

	// If buffering more than 128k, then consider this as an error (HTTP response header shouldn't be that big). 
	
	if (fResponseHeaderBuffer.GetDataSize() > (1 << 17))

		return VE_SRVR_INVALID_INTERNAL_STATE;

	const uBYTE	*p;
	VSize		size, leftOver;

	p = (const uBYTE *) fResponseHeaderBuffer.GetDataPtr();
	size = fResponseHeaderBuffer.GetDataSize();
	*outIsComplete = false;
	for (VSize i = 0; i + 4 <= size; i++)

		if (!::memcmp(&p[i], "\r\n\r\n", 4)) {

			*outIsComplete = true;
			leftOver = size - (i + 4);
			break;

		}

	if (*outIsComplete) {

		VSize			responseSize	= fResponseHeaderBuffer.GetDataSize() - leftOver;
		XBOX::VString	bufferString(fResponseHeaderBuffer.GetDataPtr(), responseSize, XBOX::VTC_UTF_8);

		fResponseHeader.FromString(bufferString);
		fStatusCode = _ExtractHTTPStatusCode();

		// Copy the left over data of the response and from the TCP stream.

		if (leftOver) {

			p = (const uBYTE *) fResponseHeaderBuffer.GetDataPtr();
			p += responseSize;			

			if (!fLeftOver.PutDataAmortized(0, p, leftOver))

				return XBOX::VE_MEMORY_FULL;

			// Resize Response Header
			fResponseHeaderBuffer.SetSize(responseSize);
		}
	}

	return XBOX::VE_OK;
}

XBOX::VError VHTTPClient::_WriteToSocket (void *inBuffer, XBOX::VSize inBufferSize)
{
	if (NULL == inBuffer)
		return XBOX::VE_INVALID_PARAMETER;

#if WITH_HTTP_CLIENT_DEBUG_LOG
	_LogData(inBuffer, inBufferSize);
#endif

	XBOX::VError error = XBOX::VE_OK;

	do
	{
		error = fTCPEndPoint->WriteExactly (inBuffer, inBufferSize, 0);
		if (VE_SRVR_RESOURCE_TEMPORARILY_UNAVAILABLE == error)
			VTask::GetCurrent()->Sleep (5);
	}
	while (VE_SRVR_RESOURCE_TEMPORARILY_UNAVAILABLE == error);

	return error;
}


XBOX::VError VHTTPClient::_ReadFromSocket(void *ioBuffer, const XBOX::VSize inMaxBufferSize, XBOX::VSize& ioReadDataSize)
{
	if (NULL == ioBuffer)
		return XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);

	XBOX::VError					error = XBOX::VE_OK;
	XBOX::StErrorContextInstaller	filter(XBOX::VE_SRVR_NOTHING_TO_READ, XBOX::VE_OK);
	uLONG							nBytesRead = 0;
	XBOX::VSize						bufferOffset = 0;
	XBOX::VSize						bufferSize = inMaxBufferSize;

	/*
	 *	YT 22-Apr-2014 -	Special case to mimic Legacy NCs and be able to read data till CRLF
	 *						Could be factorized but don't want to break anything...
	 */
	if (ioReadDataSize == 0)
	{
		if ((fLeftOver.GetDataSize() == 0) && (bufferSize > 0))
		{
			char *buffer = (char *)calloc(bufferSize, sizeof(char));
			if (NULL != buffer)
			{
				uLONG	nTotalReadBytes = 0;
				bool	bCRLFFound = false;
				/*
				 *	YT 23-Jul-2014 - ACI0088709 - Loop until an error occurs or we found a CRLF
				 */
				while ((error == XBOX::VE_OK) && (nTotalReadBytes < bufferSize) && (!bCRLFFound))
				{
					uLONG nBytesReadFromSocket = bufferSize;
					error = _ReadWithTimeoutFromEndPoint(fTCPEndPoint, (char *)buffer + nTotalReadBytes, &nBytesReadFromSocket, fReadWriteTimeout * 1000);
					if (XBOX::VE_OK == error)
					{
						nTotalReadBytes += nBytesReadFromSocket;
						if (nTotalReadBytes > 2)
							bCRLFFound = strstr(buffer, "\r\n");
					}

					XBOX::VTask::Yield();
				}
				if (!fLeftOver.PutDataAmortized(0, buffer, nTotalReadBytes))
					error = XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
				free(buffer);
			}
			else
			{
				error = XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
			}
		}

		if ((XBOX::VE_OK == error) && (fLeftOver.GetDataSize() > 0))
		{
			if (fLeftOver.GetDataSize() < inMaxBufferSize)
			{
				char *endLinePtr = strstr((char *)fLeftOver.GetDataPtr(), "\r\n");
				if (endLinePtr != NULL)
				{
					XBOX::VSize lineSize = (endLinePtr - (char *)fLeftOver.GetDataPtr()) + 2;
					memcpy(ioBuffer, fLeftOver.GetDataPtr(), lineSize);
					bufferOffset += lineSize;
					bufferSize -= lineSize;

					XBOX::VSize				tempBufferSize = (fLeftOver.GetDataSize() - lineSize);
					XBOX::VMemoryBuffer<>	tempBuffer;
					if (tempBuffer.PutDataAmortized(0, (char *)fLeftOver.GetDataPtr() + lineSize, tempBufferSize))
					{
						fLeftOver.SetDataPtr(tempBuffer.GetDataPtr(), tempBuffer.GetDataSize(), tempBuffer.GetDataSize());
						tempBuffer.ForgetData();
					}
					else
						error = XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
				}
				else
				{
					memcpy(ioBuffer, fLeftOver.GetDataPtr(), fLeftOver.GetDataSize());
					bufferOffset += fLeftOver.GetDataSize();
					bufferSize -= fLeftOver.GetDataSize();
					fLeftOver.Clear();
				}
			}
			else
			{
				memcpy(ioBuffer, fLeftOver.GetDataPtr(), inMaxBufferSize);
				bufferOffset += inMaxBufferSize;
				bufferSize -= inMaxBufferSize;

				XBOX::VSize				tempBufferSize = (fLeftOver.GetDataSize() - inMaxBufferSize);
				XBOX::VMemoryBuffer<>	tempBuffer;
				if (tempBuffer.PutDataAmortized(0, (char *)fLeftOver.GetDataPtr() + inMaxBufferSize, tempBufferSize))
				{
					fLeftOver.SetDataPtr(tempBuffer.GetDataPtr(), tempBuffer.GetDataSize(), tempBuffer.GetDataSize());
					tempBuffer.ForgetData();
				}
				else
					error = XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
			}
		}

		ioReadDataSize = bufferOffset + nBytesRead;
	}
	else
	{

		if (fLeftOver.GetDataSize() > 0)
		{
			if (fLeftOver.GetDataSize() < inMaxBufferSize)
			{
				memcpy(ioBuffer, fLeftOver.GetDataPtr(), fLeftOver.GetDataSize());
				bufferOffset += fLeftOver.GetDataSize();
				bufferSize -= fLeftOver.GetDataSize();
				fLeftOver.Clear();
			}
			else
			{
				memcpy(ioBuffer, fLeftOver.GetDataPtr(), inMaxBufferSize);

				XBOX::VSize				tempBufferSize = (fLeftOver.GetDataSize() - inMaxBufferSize);
				XBOX::VMemoryBuffer<>	tempBuffer;
				if (tempBuffer.PutDataAmortized(0, (char *)fLeftOver.GetDataPtr() + inMaxBufferSize, tempBufferSize))
				{
					fLeftOver.SetDataPtr(tempBuffer.GetDataPtr(), tempBuffer.GetDataSize(), tempBuffer.GetDataSize());
					tempBuffer.ForgetData();
				}
				else
					error = XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

				bufferOffset += inMaxBufferSize;
				bufferSize -= inMaxBufferSize;
			}
		}

		if ((XBOX::VE_OK == error) && (bufferSize > 0))
		{
			nBytesRead = bufferSize;
			error = _ReadWithTimeoutFromEndPoint(fTCPEndPoint, (char *)ioBuffer + bufferOffset, &nBytesRead, fReadWriteTimeout * 1000);
		}

		ioReadDataSize = bufferOffset + nBytesRead;
	}

	if (XBOX::VE_SRVR_NOTHING_TO_READ == error)
		error = XBOX::VE_OK;

#if WITH_HTTP_CLIENT_DEBUG_LOG
	_LogData(ioBuffer, ioReadDataSize);
#endif

	return error;
}


XBOX::VError VHTTPClient::_ReadExactlyFromSocket(void *ioBuffer, const XBOX::VSize inMaxBufferSize, XBOX::VSize& ioReadDataSize)
{
	if (NULL == ioBuffer)
		return XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);

	XBOX::VError					error = XBOX::VE_OK;
	XBOX::StErrorContextInstaller	filter(XBOX::VE_SRVR_NOTHING_TO_READ, XBOX::VE_OK);
	XBOX::VSize						nBytesRead = 0;
	XBOX::VSize						nTotalBytesRead = 0;
	XBOX::VSize						nBytesToRead = ((ioReadDataSize > 0) && (ioReadDataSize < inMaxBufferSize)) ? ioReadDataSize : inMaxBufferSize;

	do
	{
		nBytesRead = nBytesToRead;

		error = _ReadFromSocket((char *)(ioBuffer)+nTotalBytesRead, nBytesToRead, nBytesRead);

		if ((XBOX::VE_OK == error) && (nBytesRead > 0))
		{
			nBytesToRead -= nBytesRead;
			nTotalBytesRead += nBytesRead;
		}

		XBOX::VTask::Yield();
	}
	while ((XBOX::VE_OK == error) && (nBytesToRead > 0));

	ioReadDataSize = nTotalBytesRead;

	if (XBOX::VE_SRVR_NOTHING_TO_READ == error)
		error = XBOX::VE_OK;

	return error;
}


bool VHTTPClient::SetRequestBody (const void *inDataPtr, XBOX::VSize inDataSize)
{
	return fRequestBody.PutData (0, inDataPtr, inDataSize);
}


bool VHTTPClient::SetRequestBodyString (const XBOX::VString& inBodyString, const XBOX::VString& inContentType, XBOX::CharSet inCharSet)
{
	bool			isOK = false;
	XBOX::CharSet	charSet = inCharSet;

	if (!inBodyString.IsEmpty())
	{
		XBOX::VString contentType;

		/*
		 *	Content-Type already set ?
		 */
		if (fHeader.IsHeaderSet (CONST_STRING_CONTENT_TYPE))
		{
			fHeader.GetContentType (contentType, &charSet);
			if (XBOX::VTC_UNKNOWN == charSet)
				charSet = inCharSet;
		}

		/*
		 *	Convert Body using correct charSet
		 */
		XBOX::StStringConverter<char> converter (inBodyString, charSet);
		isOK = SetRequestBody ((void *)converter.GetCPointer(), converter.GetSize());

		/*
		 *	Define the Content-Type header
		 */
		if (!contentType.IsEmpty())
			fHeader.SetContentType (contentType, charSet);
		else if (!inContentType.IsEmpty())
			fHeader.SetContentType (inContentType, charSet);
	}

	return isOK;
}


bool VHTTPClient::AppendToRequestBody (void *inDataPtr, XBOX::VSize inDataSize)
{
	return fRequestBody.PutData (fRequestBody.GetDataSize(), inDataPtr, inDataSize);
}


void VHTTPClient::_ReinitHTTP (bool reinitHeader, bool reinitResponseHeader, bool reinitBufferPool)
{
// 	++fNumberOfRequests;
	if (reinitHeader)
	{
		fHeader.Clear();
	}

	if (reinitResponseHeader)
		_ReinitResponseHeader();

	if (reinitBufferPool)
		fRequestBody.Clear();

	// YT 23-Sep-2011 - ACI0073046
	fResponseBody.Clear();

	fLeftOver.Clear();	// YT 16-Sep-2014 - ACI0089564

#if WITH_HTTP_CLIENT_DEBUG_LOG
	_LogData((char *)"\r\n", 2);
#endif
}


void VHTTPClient::_ReinitResponseHeader()
{
	fResponseHeaderBuffer.Clear();
	fResponseHeader.Clear();
}


bool VHTTPClient::_SetHostHeader()
{
	if (fWithHost && !fDomain.IsEmpty()) // YT 25-Mar-2009 - ACI0061345
	{
		XBOX::VString	hostHeaderValue (fDomain);
	
		/*
		 *	YT 20-Mar-2014 - WAK0086858 Some ticklish Servers (such as GitHub) does not support port number in the Host header when one of the standard ports is used
		 */
		if ((!fUseSSL && (DEFAULT_HTTP_PORT != fPort)) || (fUseSSL && (DEFAULT_HTTPS_PORT != fPort)))
		{
			hostHeaderValue.AppendUniChar (CHAR_COLON);
			hostHeaderValue.AppendLong (fPort);
		}

		return (fHeader.SetHeaderValue (CONST_STRING_HOST, hostHeaderValue, true));
	}
	
	return 0;
}


void VHTTPClient::SetUseProxy (const XBOX::VString& inProxy, sLONG inProxyPort)
{
	if (!inProxy.IsEmpty())
	{
		fProxy.FromString (inProxy);
		fProxyPort = inProxyPort;
		fUseProxy = true;

		//	YT 09-Apr-2014 - WAK0087736 - Proxy Server may contain a scheme (especially when proxy is passed in XMLHttpRequest constructor)
		if (XBOX::HTTPTools::BeginsWithASCIICString(fProxy, "http://"))
			fProxy.Remove(1, 7);
		else if (XBOX::HTTPTools::BeginsWithASCIICString(fProxy, "https://"))
			fProxy.Remove(1, 8);
	}
	else
	{
		fProxy.Clear();
		fProxyPort = DEFAULT_HTTP_PORT;
		fUseProxy = false;
	}
}


bool VHTTPClient::SetAuthenticationValues (const XBOX::VString& inUserName, const XBOX::VString& inPassword, VAuthInfos::AuthMethod inAuthenticationMethod, bool proxyAuthentication)
{
	if (!inUserName.IsEmpty())
	{
		VAuthInfos *authInfos = proxyAuthentication ? &fProxyAuthenticationInfos : &fHTTPAuthenticationInfos;

		authInfos->SetUserName (inUserName);
		authInfos->SetPassword (inPassword);
		authInfos->SetAuthenticationMethod (inAuthenticationMethod);

		return true;
	}
	
	return false;
}


bool VHTTPClient::AddHeader (const XBOX::VString& inName,const XBOX::VString &inValue)
{
	return fHeader.SetHeaderValue (inName, inValue, true);
}


bool VHTTPClient::SetUserAgent (const XBOX::VString& inUserAgent)
{
	return fHeader.SetHeaderValue (CONST_STRING_USER_AGENT, inUserAgent, true);
}


bool VHTTPClient::SetConnectionKeepAlive (bool keepAlive)
{
	return fHeader.SetHeaderValue (CONST_STRING_CONNECTION, (keepAlive) ? CONST_STRING_KEEP_ALIVE : CONST_STRING_CLOSE);
}


XBOX::VError VHTTPClient::_GenerateRequest (HTTP_Method inMethod)
{
	fRequestMethod = inMethod;

	_InitCustomHeaders();

	XBOX::VSize contentLength = fRequestBody.GetDataSize();

	if (fUpgradeRequest) 

		fHeader.SetHeaderValue(CONST_STRING_CONNECTION, CONST_STRING_UPGRADE);

	else if (fKeepAlive)

		SetConnectionKeepAlive (true);

	/*
	 *	YT 08-Apr-2014 - WAK0087154 -	Do not set authorization header if the server does not request it...
	 *									AuthenticationManager DB may be closed and therefore, authentication may fail
	 */
	if (fMayUsePreAuthentication || !IsFirstRequest())
	{
		// Add a custom header for HTTP Authentification (when it does not already exist)
		if (fHTTPAuthenticationInfos.IsValid())
		{
			if (!fHeader.IsHeaderSet(CONST_STRING_AUTHORIZATION))
				AddAuthorizationHeader(fHTTPAuthenticationInfos, inMethod, false);
		}

		// Add a custom header for Proxy Authentification (when it does not already exist)
		if (fProxyAuthenticationInfos.IsValid())
		{
			//jmo - NTLM authentication needs 2 headers : fNTLMAuthenticationInProgress tells us the first one
			//		(negociation) has been generated and we must now generate the second one (challenge response).
			if (!fHeader.IsHeaderSet(CONST_STRING_PROXY_AUTHORIZATION) || fNTLMAuthenticationInProgress)
				AddAuthorizationHeader(fProxyAuthenticationInfos, inMethod, true);
		}
	}

	// Set Content-Length
	if ((fRequestMethod == HTTP_POST) || (fRequestMethod == HTTP_PUT) || (contentLength > 0)) // YT 28-Feb-2013 - ACI0080753 - Content-Length header is mandatory for POST & PUT requests in HTTP/1.1
		fHeader.SetContentLength (contentLength);

	if (fUseHTTPCompression)
		fHeader.SetHeaderValue (CONST_STRING_ACCEPT_ENCODING, CONST_STRING_SUPPORTED_ENCODING, true);

	return XBOX::VE_OK;
}


bool VHTTPClient::_ExtractAuthenticationInfos (XBOX::VString& outRealm, VAuthInfos::AuthMethod& outProxyAuthenticationMethod)
{
	if ((fStatusCode != 401) && (fStatusCode != 407))
		return false;

	XBOX::VString authHeaderName;
	XBOX::VString authHeaderValue;

	if (fStatusCode == 407)
		authHeaderName.FromCString ("Proxy-Authenticate");
	else
		authHeaderName.FromCString ("WWW-Authenticate");
  
	if (GetResponseHeaderValue (authHeaderName, authHeaderValue) && !authHeaderValue.IsEmpty())
	{
		XBOX::VString				authMethodString;
		XBOX::VNameValueCollection	params;
		
		_ExtractAuthorizationHeaderValues (authHeaderValue, authMethodString, &params);

		if (params.Has ("realm"))
			outRealm.FromString (params.Get ("realm"));

		if (EqualASCIIString (authMethodString, CONST_STRING_AUTHENTICATION_DIGEST))
			outProxyAuthenticationMethod = VAuthInfos::AUTH_DIGEST;
		else if (EqualASCIIString (authMethodString, CONST_STRING_AUTHENTICATION_NTLM))
			outProxyAuthenticationMethod = VAuthInfos::AUTH_NTLM;
		else if (EqualASCIIString (authMethodString, CONST_STRING_AUTHENTICATION_BASIC))
			outProxyAuthenticationMethod = VAuthInfos::AUTH_BASIC;
		else
			outProxyAuthenticationMethod = VAuthInfos::AUTH_NONE;

		return true;
	}
	
	return false;
}


void VHTTPClient::_ComputeDomain (XBOX::VString& outDomain, bool useProxy)
{
	outDomain.Clear();

	outDomain.AppendCString ("http://");
	if (useProxy)
	{
		outDomain.FromString (fProxy);
		if (fProxyPort != DEFAULT_HTTP_PORT)
		{
			outDomain.AppendUniChar (CHAR_COLON);
			outDomain.AppendLong (fProxyPort);
		}
	}
	else
	{
		if (!fDomain.IsEmpty())	// YT 25-Mar-2009 - ACI0061345
			outDomain.AppendString (fDomain);

		if (fPort != DEFAULT_HTTP_PORT)
		{
			outDomain.AppendUniChar (CHAR_COLON);
			outDomain.AppendLong (fPort);
		}
	}
}


XBOX::VError VHTTPClient::Send (HTTP_Method inMethod)
{
	XBOX::VError	error = 0;
	sLONG			lastAuthenticationError = 0;
	sLONG			numOfRedirections = 0;	// YT 04-Apr-2011 - ACI0070458

	fResponseHeaderBuffer.Clear();

	fNTLMAuthenticationInProgress = false;
	
	error = _GenerateRequest (inMethod);

	if (XBOX::VE_OK == error)
		error = _SendRequestAndReceiveResponse();

	while (XBOX::VE_OK == error)
	{
		fStatusCode = GetHTTPStatusCode();

		// Never display the authentication dialog when fShowAuthenticationDialog is false (i.e. for commands)
		if (fUseAuthentication && (fStatusCode == 401 || fStatusCode == 407))	// Authentication failed
		{
			if(!fNTLMAuthenticationInProgress)
			{
				// YT 06-Mar-2009 - ACI0059060
				if (lastAuthenticationError != fStatusCode)
					lastAuthenticationError = fStatusCode;
				else
					break;
			}

			// Ask Proxy User & Passwd
			XBOX::VString			realm;
			bool					proxyAuthentication = (fStatusCode == 407) ? true : false;
			VAuthInfos&				savedAuthInfo = proxyAuthentication ? fSavedProxyAuthenticationInfos : fSavedHTTPAuthenticationInfos;
			VAuthInfos::AuthMethod	authenticationMethod = savedAuthInfo.GetAuthenticationMethod();

			if (_ExtractAuthenticationInfos (realm, authenticationMethod))
			{
				XBOX::VString username;
				XBOX::VString password;

				if (fShowAuthenticationDialog && (fAuthenticationDialogCallBackPtr != NULL))
				{
					// Restore existing Authentication Infos (when realm is identical)...
					if (savedAuthInfo.GetRealm().EqualToString (realm) && savedAuthInfo.IsValid())
					{
						username.FromString (savedAuthInfo.GetUserName());
						password.FromString (savedAuthInfo.GetPassword());
					}
					else
					{
						XBOX::VString domain;

						_ComputeDomain (domain, proxyAuthentication);

						savedAuthInfo.SetDomain(domain);
						savedAuthInfo.SetRealm(realm);

						//Display authentication dialog and retrieve user login & password
						fAuthenticationDialogCallBackPtr(savedAuthInfo, fAuthenticationDialogCallBackPrivateData);

						username.FromString(savedAuthInfo.GetUserName());
						password.FromString(savedAuthInfo.GetPassword());
					}
				}
				else
				{
					if (proxyAuthentication)
					{
						username.FromString (fProxyAuthenticationInfos.GetUserName());
						password.FromString (fProxyAuthenticationInfos.GetPassword());
					}
					else
					{
						username.FromString (fHTTPAuthenticationInfos.GetUserName());
						password.FromString (fHTTPAuthenticationInfos.GetPassword());
					}
				}

				if (SetAuthenticationValues (username, password, authenticationMethod, proxyAuthentication))
				{
					// Regenerate Request
					_ReinitHTTP (false, false, false);

					_GenerateRequest (inMethod);
					_ReinitResponseHeader();

					// And finally post request
					error = _SendRequestAndReceiveResponse();

					// If Authentication failed, erase saved HTTP and/or Proxy Authentication infos
					fStatusCode = GetHTTPStatusCode();

					if ((authenticationMethod == VAuthInfos::AUTH_NTLM) && !fNTLMAuthenticationInProgress)
					{
						fNTLMAuthenticationInProgress = true;
					}
					else
					{
						fNTLMAuthenticationInProgress = false;

						if (fStatusCode == 401)
							fSavedHTTPAuthenticationInfos.Clear();

						if (fStatusCode == 407)
							fSavedProxyAuthenticationInfos.Clear();
					}
				}
			}
		}
		
		/*	Location changed ? (301: Moved Permanently, 302: Found, 303: See Other, 307: Temporary Redirect) 
		 *	Theses following status codes implicitly insure that response contains a Location header pointing
		 *	the new/temporary resource's URL
		 */
		else if (fFollowRedirect && (numOfRedirections < fMaxRedirections) && ((301 == fStatusCode) || (302 == fStatusCode) || (303 == fStatusCode) || (307 == fStatusCode)))
		{
			XBOX::VString locationString;

			if (fResponseHeader.GetHeaderValue (CONST_STRING_LOCATION, locationString) && !locationString.IsEmpty())
			{
				// Change Location
				bool absoluteURL = (((FindASCIIString(locationString, "http") == 1) || (FindASCIIString(locationString, "ws") == 1)) && (FindASCIIString(locationString, "://") > 0)); // YT 04-Sep-2014 - WAK0089392
				if (absoluteURL)
				{
					XBOX::VURL URL (locationString, false);
					_ParseURL (URL);
				}
				else // It's a relative URL, just update folder parts
				{
					fFolder.FromString (locationString); // YT 22-Mar-2011 - ACI0070362
				}

				// Regenerate Request
				_ReinitHTTP (true, false, false);

				_GenerateRequest (inMethod);
				_ReinitResponseHeader();

				// And finally post request
				error = _SendRequestAndReceiveResponse();

				fStatusCode = GetHTTPStatusCode();
			}

			++numOfRedirections;
		}
		else
			break;

		XBOX::VTask::GetCurrent()->YieldNow();
	}

	if (fUseAuthentication && fResetAuthenticationInfos)
	{
		if (fHTTPAuthenticationInfos.IsValid())
			fHTTPAuthenticationInfos.Clear();
		if (fProxyAuthenticationInfos.IsValid())
			fProxyAuthenticationInfos.Clear();
		VHTTPClient::ClearSavedAuthenticationInfos();
	}

	fHeader.Clear();
	fRequestBody.Clear();

	return error;
}


bool VHTTPClient::GetResponseHeaderValue (const XBOX::VString& inName, XBOX::VString& outValue)
{
	return fResponseHeader.GetHeaderValue (inName, outValue);
}


/* static */
bool VHTTPClient::ParseURL (const XBOX::VURL& inURL, XBOX::VString *outDomain, sLONG *outPort, XBOX::VString *outFolder, 
							XBOX::VString *outQuery, bool *outUseSSL, XBOX::VString *outUserName, XBOX::VString *outPassword,
							XBOX::VString *outProxy, PortNumber* outProxyPort)
{
	// a url can be, in the most complicated cases, like this: https://username:password@www.server.com/path
	if (inURL.IsEmpty() || (NULL == outDomain))
		return false;

	bool			isOK = true;
	sLONG			pos = 0;
	XBOX::VString	url;
	XBOX::VString	scheme;
	XBOX::VString	domain;
	XBOX::VString	folder;
	XBOX::VString	query;
	XBOX::VString	userName;
	XBOX::VString	password;
	bool			useSSL = false;
	sLONG			port = DEFAULT_HTTP_PORT;

	inURL.GetAbsoluteURL (url, false);
	inURL.GetScheme (scheme);

	const UniChar *stringPtr = url.GetCPointer();

	if (!scheme.IsEmpty())
	{
		if (EqualASCIIString (scheme, "https") || EqualASCIIString (scheme, "wss"))
		{
			useSSL = true;
			port = 443;	// different default port from HTTP
		}

		inURL.GetNetworkLocation(domain, false);
		inURL.GetPath(folder, eURL_POSIX_STYLE, false);  // YT 03-Sep-2014 - WAK0089322
		inURL.GetQuery(query, false);
/*
		if (pos)//there is a path after the domain
		{
			url.GetSubString (1, pos-1, domain);
			if (pos < url.GetLength())
				url.GetSubString (pos+1, url.GetLength()-pos, folder);

			if (domain.IsEmpty() || folder.IsEmpty())
				isOK = false;
		}
		else// No folder in URL
		{
			url.GetSubString (1, url.GetLength(), domain); // YT 22-Mar-2011 - ACO0070359 - was: inURL.GetSubString (prefixLen, pos+prefixLen-1, domain);
		}
*/
	}
	else //no scheme, look for folder
	{
		pos = FindASCIIString (stringPtr, "/");
		if (pos)
		{
			url.GetSubString (1, pos-1, domain);

			//jmo - BUG : folder is actually folder + query string ?
			url.GetSubString (pos+1, url.GetLength()-pos, folder);

			if (domain.IsEmpty() || folder.IsEmpty())
				isOK = false;
		}
		else// No folder in URL
		{
			domain.FromString (url);
			if (domain.IsEmpty())
				isOK = false;
		}
	}			

	if (!domain.IsEmpty())
	{
		XBOX::VString tempString;

		// parsing the username/password info, if any (m.c)
		sLONG pos_at = domain.FindUniChar (CHAR_COMMERCIAL_AT);
		if (pos_at)
		{
			inURL.GetUserName(userName, false);
			inURL.GetPassword(password, false);
			/*
			 *	YT 03-Jul-2014 - ACI0088664
			 */
 			XBOX::VURL::Decode(userName);
 			XBOX::VURL::Decode(password);

			domain.SubString (pos_at + 1, domain.GetLength() - pos_at);
		}

		sLONG pos_port = domain.FindUniChar (CHAR_COLON);
		if (pos_port)
		{
			domain.GetSubString (pos_port + 1, domain.GetLength() - pos_port, tempString);
			domain.SubString (1, pos_port - 1);
			port = tempString.GetLong();
		}
	}

	if (outDomain)
		outDomain->FromString (domain);

	if (outFolder)
		outFolder->FromString (folder);

	if (outQuery)
		outQuery->FromString (query);

	if (outUserName)
		outUserName->FromString (userName);

	if (outPassword)
		outPassword->FromString (password);

	if (outUseSSL)
		*outUseSSL = useSSL;

	if (outPort)
		*outPort = port;

	if (outProxy)
	{
		VError verr=VProxyManager::GetProxyForURL(inURL, outProxy, outProxyPort);
		xbox_assert(verr == VE_OK);
	}

	return isOK;
}


bool VHTTPClient::_ParseURL (const XBOX::VURL& inURL)
{
	// a url can be, in the most complicated cases, like this: https://username:password@www.server.com/path
	if (inURL.IsEmpty())
		return false;

	bool			isOK = true;
	XBOX::VString	userName;
	XBOX::VString	password;

	//reset domain and folder members just in case we would be reparsing the url (redirection)
	fDomain.Clear();
	fFolder.Clear();
	fQuery.Clear();

	VString proxy;
	PortNumber proxyPort = kBAD_PORT;

	isOK = ParseURL (inURL, &fDomain, &fPort, &fFolder, &fQuery, &fUseSSL, &userName, &password, &proxy, &proxyPort);

	if (!userName.IsEmpty() || !password.IsEmpty())
		AddAuthorizationHeader (userName, password, VAuthInfos::AUTH_BASIC, HTTP_GET, false);

	SetUseProxy(proxy, proxyPort);

	//lower case the domain name
	fDomain.ToLowerCase();

	return isOK;
}


void VHTTPClient::GetResponseStatusLine (XBOX::VString& outStatusLine) const
{
	char *	startLinePtr = (char *)fResponseHeaderBuffer.GetDataPtr();

	outStatusLine.Clear();

	if (NULL != startLinePtr)
	{
		char *endLinePtr = strstr (startLinePtr, "\r\0");
		XBOX::VSize startLineSize = (NULL != endLinePtr) ? XBOX::VSize (endLinePtr - startLinePtr) : fResponseHeaderBuffer.GetDataSize();
		outStatusLine.FromBlock (startLinePtr, startLineSize, XBOX::VTC_UTF_8);
	}
}


void VHTTPClient::GetResponseBodyString (XBOX::VString& outBodyString)
{
	outBodyString.Clear();

	if (fResponseBody.GetDataSize() > 0)
	{
		XBOX::VString	contentType;
		XBOX::VString	mimeType;
		XBOX::CharSet	charSet;

		if (GetResponseHeaderValue (CONST_STRING_CONTENT_TYPE, contentType))
			XBOX::HTTPTools::ExtractContentTypeAndCharset (contentType, mimeType, charSet);

		if (XBOX::MIMETYPE_TEXT == XBOX::HTTPTools::GetMimeTypeKind (contentType))
		{
			// YT 08-Apr-2014 - WAK0087378 - Use VStream::GetText() instead ConvertToVString() in order to deal with BOM
			XBOX::VConstPtrStream stream(fResponseBody.GetDataPtr(), fResponseBody.GetDataSize());
			stream.OpenReading();
			stream.GuessCharSetFromLeadingBytes(charSet != XBOX::VTC_UNKNOWN ? charSet : XBOX::VTC_UTF_8);
			stream.GetText(outBodyString);
			stream.CloseReading();
		}
	}
}


sLONG VHTTPClient::_ExtractHTTPStatusCode() const
{
	sLONG			statusCode = 0;
	XBOX::VString	statusLine;
	
	GetResponseStatusLine (statusLine);

	XBOX::VIndex	pos = statusLine.FindUniChar (CHAR_SPACE) + 1;

	for (sLONG i = 0; i < 3 && pos + i < statusLine.GetLength(); i++) {

		UniChar	c	= statusLine.GetUniChar(pos + i);

		if (c >= '0' && c <= '9') 

			statusCode = statusCode * 10 + (c - '0');

		else

			break;

	}

	return statusCode;
}


void VHTTPClient::_SetBodyReply (char *inBody, XBOX::VSize inBodySize)
{
	fResponseBody.Clear();
	fResponseBody.SetDataPtr (inBody, inBodySize, inBodySize);

	// Decompress body data (when applicable)
	XBOX::VString	valueString;

	if (fResponseHeader.GetHeaderValue (CONST_STRING_CONTENT_ENCODING, valueString) &&
		((EqualASCIIString (valueString, CONST_STRING_DEFLATE)) || 
		(EqualASCIIString (valueString, CONST_STRING_GZIP))))
	{
		XBOX::VPtrStream	stream;
		XBOX::VError		error;

		stream.OpenWriting();
		error = _ExpandMemoryBlock(fResponseBody.GetDataPtr(), fResponseBody.GetDataSize(), stream);
		stream.CloseWriting(true);

		if (error == VE_OK)
		{
			char *		dataPtr = (char *)malloc(stream.GetSize());
			XBOX::VSize	dataSize = stream.GetSize();

			if (dataPtr != NULL)
			{
				stream.OpenReading();
				error = stream.GetData(dataPtr, dataSize);
				stream.CloseReading();

				// Replace compressed body content by the new expanded one
				if (error == VE_OK)
				{
					fResponseBody.Clear();
					fResponseBody.SetDataPtr(dataPtr, dataSize, dataSize);
				}
				else
				{
					free(dataPtr);
					dataPtr = NULL;
				}
			}
		}
	}
}


bool VHTTPClient::AddAuthorizationHeader (const VAuthInfos& inValue, HTTP_Method inHTTPMethod, bool proxyAuthentication)
{
	return AddAuthorizationHeader(	inValue.GetUserName(),
									inValue.GetPassword(),
									inValue.GetAuthenticationMethod(),
									inHTTPMethod,
									proxyAuthentication);
}


bool VHTTPClient::AddAuthorizationHeader (const XBOX::VString& inUserName, const XBOX::VString& inPassword, VAuthInfos::AuthMethod inAuthenticationMethod, HTTP_Method inMethod, bool proxyAuthentication)
{
	bool			isOK = false;
	XBOX::VString	headerValue;

	switch (inAuthenticationMethod)
	{
	case VAuthInfos::AUTH_NONE:
		isOK = true;
		break;

	case VAuthInfos::AUTH_BASIC:
		isOK = AddBasicAuthorizationHeader (inUserName, inPassword, inMethod, proxyAuthentication);
		break;

	case VAuthInfos::AUTH_DIGEST:
		if (FindChosenAuthenticationHeader (CONST_STRING_AUTHENTICATION_DIGEST, headerValue))
			isOK = AddDigestAuthorizationHeader (headerValue, inUserName, inPassword, inMethod, proxyAuthentication);
		break;

	case VAuthInfos::AUTH_NTLM:
//		if (FindChosenAuthenticationHeader (CONST_STRING_AUTHENTICATION_NTLM, headerValue))
//			isOK = AddNtlmAuthorizationHeader (headerValue, inUserName, inPassword, inMethod, proxyAuthentication);
		break;
	}

	return isOK;
}


bool VHTTPClient::FindChosenAuthenticationHeader (const XBOX::VString& inAuthMethodName, XBOX::VString& outAuthenticateHeaderValue)
{
	if (inAuthMethodName.IsEmpty())
		return false;

	bool			result = false;
	XBOX::VString	headerValue;

	outAuthenticateHeaderValue.Clear();

	if (fResponseHeader.GetHeaderValue ((fStatusCode == 407) ? CONST_STRING_PROXY_AUTHENTICATE : CONST_STRING_WWW_AUTHENTICATE, outAuthenticateHeaderValue))
	{
		XBOX::VString authMethodString;
		_ExtractAuthorizationHeaderValues (outAuthenticateHeaderValue, authMethodString, NULL);
		result = (EqualASCIIString (authMethodString, inAuthMethodName));
	}

	return result;
}


bool VHTTPClient::AddBasicAuthorizationHeader (const XBOX::VString& inUserName, const XBOX::VString& inPassword, HTTP_Method inMethod, bool proxyAuthentication)
{
	XBOX::VString	auth;

	auth.FromString (inUserName);
	auth.AppendUniChar (CHAR_COLON);
	auth.AppendString (inPassword);
	
	if (auth.EncodeBase64())
	{
		auth.Insert (CVSTR ("Basic "), 1);
		return fHeader.SetHeaderValue (proxyAuthentication ? CONST_STRING_PROXY_AUTHORIZATION : CONST_STRING_AUTHORIZATION, auth, true);
	}

	return false;
}


bool VHTTPClient::AddDigestAuthorizationHeader (const XBOX::VString& inAuthorizationHeaderValue, const XBOX::VString& inUserName, const XBOX::VString& inPassword, HTTP_Method inMethod, bool proxyAuthentication)
{
	if (!inAuthorizationHeaderValue.IsEmpty())
	{
		sLONG						count = 0;
		XBOX::VString				realmValue;
		XBOX::VString				domainValue;
		XBOX::VString				nonceValue;
		XBOX::VString				opaqueValue;
		XBOX::VString				staleValue;
		XBOX::VString				algorithmValue;
		XBOX::VString				qopValue;
		XBOX::VString				response;
		XBOX::VString				authHeader;
		XBOX::VString				cnonceValue;
		XBOX::VString				uriValue;
		XBOX::VString				ncValue;
		XBOX::VString				methodValue;

		XBOX::VString				authMethodString;
		XBOX::VNameValueCollection	params;

		_ExtractAuthorizationHeaderValues (inAuthorizationHeaderValue, authMethodString, &params);

		realmValue.FromString (params.Get ("realm"));
		domainValue.FromString (params.Get ("domain"));			// optional
		nonceValue.FromString (params.Get ("nonce"));
		opaqueValue.FromString (params.Get ("opaque"));			// optional
		staleValue.FromString (params.Get ("stale"));			// optional
		algorithmValue.FromString (params.Get ("algorithm"));
		qopValue.FromString (params.Get ("qop"));
		ncValue.FromString (params.Get ("nc"));

		if (!qopValue.IsEmpty())
			_CreateNewSessionID (cnonceValue);
		uriValue.FromString (fFolder);
		if (uriValue.GetUniChar (1) != CHAR_SOLIDUS)
			uriValue.Insert (CHAR_SOLIDUS, 1);
			
		if (!ncValue.IsEmpty())
			count = ncValue.GetLong();
		ncValue.Printf ("%08x", ++count);

		_GetMethodName (inMethod, methodValue);

		_CalcDigestPasswordChallenge (inUserName, inPassword, algorithmValue, realmValue, nonceValue, cnonceValue, qopValue, ncValue, uriValue, methodValue, response);
		
		authHeader.Clear();
		authHeader.AppendCString ("Digest username=\"");
		authHeader.AppendString (inUserName);
		authHeader.AppendCString ("\", realm=\"");
		authHeader.AppendString (realmValue);
		authHeader.AppendCString ("\", nonce=\"");
		authHeader.AppendString (nonceValue);
		authHeader.AppendCString ("\", algorithm=");
		authHeader.AppendString (algorithmValue);
		if (!opaqueValue.IsEmpty())
		{
			authHeader.AppendCString (", opaque=\"");
			authHeader.AppendString (opaqueValue);
		}
		authHeader.AppendCString (", uri=\"");
		authHeader.AppendString (uriValue);	// ???
		authHeader.AppendCString ("\", response=\"");
		authHeader.AppendString (response);
		authHeader.AppendCString ("\"");

		if (!qopValue.IsEmpty())
		{
			authHeader.AppendCString (", qop=");
			authHeader.AppendString (qopValue);
			authHeader.AppendCString (", cnonce=\"");
			authHeader.AppendString (cnonceValue);
			authHeader.AppendCString ("\", nc=");
			authHeader.AppendString (ncValue);
		}

		return fHeader.SetHeaderValue (proxyAuthentication ? CONST_STRING_PROXY_AUTHORIZATION : CONST_STRING_AUTHORIZATION, authHeader);
	}

	return false;
}


bool VHTTPClient::AddNtlmAuthorizationHeader (const XBOX::VString& inAuthorizationHeaderValue, const XBOX::VString& inUserName, const XBOX::VString& inPassword, HTTP_Method inMethod, bool proxyAuthentication)
{
	if (inAuthorizationHeaderValue.IsEmpty())
		return false; //jmo - todo : find some meaningfull error !
/* TBD
	sLONG headerLen = strlen (header);

	NTLMSentMessage* ntlmMsg = NTLMSentMessage::BuildNewMessageFromHeader (header, headerLen);
  
	if(!ntlmMsg)
		return false;	//jmo - todo : find some meaningfull error !

	XBOX::VString authHeader ("NTLM ");
	XBOX::VString headerBytes;

	ntlmMsg->SetUserName (inUserName);
	ntlmMsg->SetPassword (inPassword);

	ntlmMsg->FillClientAuthHeader (headerBytes);

	authHeader += headerBytes;

	bool retValue = fHeader.SetHeaderValue (proxyAuthentication ? CONST_STRING_PROXY_AUTHORIZATION : CONST_STRING_AUTHORIZATION, authHeader, true);
	delete ntlmMsg;

	return retValue;
*/
	return false;
}


XBOX::VError VHTTPClient::_SendRequestAndReceiveResponse()
{
	XBOX::VError			error = XBOX::VE_OK;
	bool					isHeaderOK = false;
	sLONG					progressionPercentage = 15;

	if (NULL != fProgressionCallBackPtr)
		fProgressionCallBackPtr(fProgressionCallBackPrivateData, PROGSTATUS_STARTING, 0);

	error = OpenConnection(NULL);

#if WITH_HTTP_CLIENT_DEBUG_LOG
	_LogData((char *)REQUEST_MARKER_STRING, strlen(REQUEST_MARKER_STRING));
#endif

	if (XBOX::VE_OK == error)
	{
		error = _SendRequestHeader();
		
		if (NULL != fProgressionCallBackPtr)
			fProgressionCallBackPtr (fProgressionCallBackPrivateData, PROGSTATUS_SENDING_REQUEST, ++progressionPercentage);
	}

	if ((XBOX::VE_OK == error) && (fRequestBody.GetDataSize() > 0))
	{
		error = _WriteToSocket (fRequestBody.GetDataPtr(), (uLONG)fRequestBody.GetDataSize());

		if ((NULL != fProgressionCallBackPtr) && (progressionPercentage < 25))
			fProgressionCallBackPtr (fProgressionCallBackPrivateData, PROGSTATUS_SENDING_REQUEST, ++progressionPercentage);
	}

	if (NULL != fProgressionCallBackPtr)
		fProgressionCallBackPtr (fProgressionCallBackPrivateData, PROGSTATUS_RECEIVING_RESPONSE, 101);

#if WITH_HTTP_CLIENT_DEBUG_LOG
	_LogData((char *)RESPONSE_MARKER_STRING, strlen(RESPONSE_MARKER_STRING));
#endif

	if (XBOX::VE_OK == error)
	{
		XBOX::VSize	bufferSize = HTTP_CLIENT_BUFFER_SIZE;
		char *		buffer = (char *) malloc (HTTP_CLIENT_BUFFER_SIZE);
		bool		previousLineEndedWithCRLF = false;

		if (NULL == buffer)
			error = XBOX::vThrowError (XBOX::VE_MEMORY_FULL);

		error = ReadResponseHeader ();
		if (XBOX::VE_OK == error)
		{
			if (HTTP_HEAD != fRequestMethod)	// YT 19-Sep-2011 - ACI0073045 - Do not try to read message body with HEAD request
			{
				bool		isChunked = _IsChunkedResponse();
				sLONG8		curr_body_len = -1;
				XBOX::VSize contentLength = 0;

				fResponseHeader.GetContentLength (contentLength);

				if (!isChunked && (contentLength > 0)) // YT 21-Jun-2011 - ACI0071986
					curr_body_len = contentLength;
                
				if (curr_body_len >= 0)
				{
					// cool, there is a Content-Length
					if (curr_body_len > 0)	// if 0, do nothing
					{
						XBOX::VMemoryBuffer<>	memoryBuffer;
						XBOX::VSize				chunkSize = 0;
                        
                        //jmo - Correction partielle pour le pb du keep-alive qui ne rend pas la main :
                        //      S'il reste des données dans leftOver (cad si on a lu plus que les headers
                        //      en première lecture), on place les données non lues au début de memoryBuffer,
                        //      comme si elles venaient tout juste d'être lues (et on ajuste curr_body_len).
                        if(fLeftOver.GetDataSize()>0)
                        {
                            memoryBuffer.PutDataAmortized(0, fLeftOver.GetDataPtr(), fLeftOver.GetDataSize());
                            fLeftOver.Clear();
                            curr_body_len-=memoryBuffer.GetDataSize();
                        }
                        
                        while ((XBOX::VE_OK == error) && (curr_body_len > 0))
						{
							chunkSize = HTTP_CLIENT_BUFFER_SIZE;
							error = _ReadFromSocket(buffer, HTTP_CLIENT_BUFFER_SIZE, chunkSize);

							if ((XBOX::VE_OK == error) && (chunkSize > 0))
							{
								memoryBuffer.PutDataAmortized(memoryBuffer.GetDataSize(), (const void *)buffer, chunkSize);
								curr_body_len -= chunkSize;
							}

							XBOX::VTask::Yield();
						}

						if (NULL != memoryBuffer.GetDataPtr())
						{
							_SetBodyReply((char *)memoryBuffer.GetDataPtr(), memoryBuffer.GetDataSize());
							memoryBuffer.ForgetData();
						}

						error = XBOX::VE_OK;
					}
				}
				else
				{
					XBOX::VMemoryBuffer<> memoryBuffer;

					if (isChunked)
					{
						/*
						 *	Yes, we do support HTTP Chunk encoding
						 *	(that way we really are HTTP/1.1 compliant)
						 */
						bool stopReading = false;
						while ((XBOX::VE_OK == error) && (bufferSize > 0) && !stopReading)
						{
							// first, we need to read the length of the next chunk												
							bufferSize = 0;	// ReadLine b/c chunk length ends with CRLF
							error = _ReadFromSocket (buffer, HTTP_CLIENT_BUFFER_SIZE, bufferSize);

							if ((bufferSize == 2) && (buffer[0] == '\r') && (buffer[1] == '\n'))
							{
								bufferSize = 0;	// ReadLine b/c chunk length ends with CRLF
								error = _ReadFromSocket (buffer, HTTP_CLIENT_BUFFER_SIZE, bufferSize);
							}
							if ((XBOX::VE_OK == error) && (bufferSize > 0))
							{
								XBOX::VSize chunkSize = _GetChunkSize(buffer);
								if (chunkSize)
								{
									bufferSize = chunkSize;	// check size <32k
									if (bufferSize <= HTTP_CLIENT_BUFFER_SIZE)
									{
										// optim: we can use buffer b/c it's big enough
										error = _ReadExactlyFromSocket(buffer, HTTP_CLIENT_BUFFER_SIZE, bufferSize);

										if ((XBOX::VE_OK == error) && (bufferSize > 0))
										{
											memoryBuffer.PutDataAmortized (memoryBuffer.GetDataSize(), (const void *)buffer, bufferSize);
										}
									}
									else
									{
										// DAMNED ! this is a big boy. Deal with it.
										char *temp_buffer2 = (char *)malloc (bufferSize);
										if (temp_buffer2)
										{
											error = _ReadExactlyFromSocket(temp_buffer2, bufferSize, bufferSize);

											if ((XBOX::VE_OK == error) && (bufferSize > 0))
											{
												memoryBuffer.PutDataAmortized (memoryBuffer.GetDataSize(), (const void *)temp_buffer2, bufferSize);
											}
											free (temp_buffer2);
										}
									}

									// Sanity check
									xbox_assert(bufferSize == chunkSize);
								}
								else
									stopReading = true;
							}
						}
					}
					else
					{
						/*
						 *	Some servers do that :-(
						 *	Just read until there is nothing left...
						 */
						if (XBOX::VE_OK == error)
						{
							do
							{
								bufferSize = HTTP_CLIENT_BUFFER_SIZE;	// YT 07-Nov-2008 - ACI0059719 & ACI0058995
								error = _ReadFromSocket (buffer, HTTP_CLIENT_BUFFER_SIZE, bufferSize);

								if ((XBOX::VE_OK == error) && (bufferSize > 0))
								{
									memoryBuffer.PutDataAmortized (memoryBuffer.GetDataSize(), (const void *)buffer, bufferSize);
								}

								XBOX::VTask::Yield();
							}
							while ((XBOX::VE_OK == error) && (bufferSize > 0));
						}
					}

					if (NULL != memoryBuffer.GetDataPtr())
					{
						_SetBodyReply((char *)memoryBuffer.GetDataPtr(), memoryBuffer.GetDataSize());
						memoryBuffer.ForgetData();
					}

					error = XBOX::VE_OK;
				}
			}
		}

		if (NULL != buffer)
		{
			free (buffer);
			buffer = NULL;
		}
	}
	/*
	 *	J.F. Do not close the stream if an error has been raised
	 *	(The comm has already been closed)
	 */
	if (XBOX::VE_OK == error)
	{
		/*
		 *	Verify if the Server does not close the connection
		 */
		bool connectionCloseByServer = false;
		if (fKeepAlive && fResponseHeader.IsHeaderSet (CONST_STRING_CONNECTION))
		{
			XBOX::VString connectionValue;
			fResponseHeader.GetHeaderValue (CONST_STRING_CONNECTION, connectionValue);
			connectionCloseByServer = (FindASCIIString (connectionValue, "keep-alive") == 0);
		}

		if (!fKeepAlive || connectionCloseByServer)
		{
			CloseConnection();
		}
	}
	else
	{
		CloseConnection();
	}

	if (XBOX::VE_OK == error)
	{
		++fNumberOfRequests;
	}

	return error;
}


XBOX::VError VHTTPClient::_SendCONNECTToProxy()
{
	if ((!fUseProxy) || (!fUseSSL))
		return XBOX::VE_OK;

	XBOX::VString	connectString;

	/*
	 *	uh oh, this an HTTPS connection through a proxy... let the fun begin (m.c)
	 */
	connectString.AppendCString ("CONNECT ");
	connectString.AppendString (fDomain);
	connectString.AppendUniChar (CHAR_COLON);
	connectString.AppendLong (fPort);
	connectString.AppendCString (" HTTP/1.0\r\n");
	connectString.AppendCString ("Host: ");
	connectString.AppendString (fDomain);
	connectString.AppendUniChar (CHAR_COLON);
	connectString.AppendLong (fPort);
	connectString.AppendCString ("\r\n\r\n");

	XBOX::StStringConverter<char> converter (connectString, XBOX::VTC_UTF_8);
	return _WriteToSocket ((void *)converter.GetCPointer(), (uLONG)converter.GetSize());
}


XBOX::VError VHTTPClient::_SendRequestHeader()
{
	XBOX::VString	requestString;
	XBOX::VString	headerString;

	fHeader.ToString (headerString);
	_GetMethodName (fRequestMethod, requestString);
	requestString.AppendUniChar (CHAR_SPACE);

	if (!fProxy.IsEmpty() && !XBOX::VProxyManager::ByPassProxyOnLocalhost (fDomain) && !fUseSSL)
	{
		/*
		 *	if there is an HTTP proxy, then we need to use an absolute URI for non SSL requests
		 */
		
		requestString.AppendCString ("http://");
		requestString.AppendString (fDomain);
		requestString.AppendUniChar (CHAR_COLON);
		requestString.AppendLong (fPort);
		
	}

	requestString.AppendUniChar (CHAR_SOLIDUS);

	if (!fFolder.IsEmpty())
		requestString.AppendString (fFolder);

	if (!fQuery.IsEmpty())
		requestString.AppendUniChar (CHAR_QUESTION_MARK).AppendString (fQuery);

	requestString.AppendUniChar (CHAR_SPACE);
	requestString.AppendCString ("HTTP/1.1\r\n");	//Trick to avoid chunked transfer-encoding in http 1.1

	// Write the whole headers
	requestString.AppendString (headerString);

	// Terminate request with a final <CRLF>
	requestString.AppendCString ("\r\n");

	// Write Header
	XBOX::StStringConverter<char> converter (requestString, XBOX::VTC_UTF_8);

	return _WriteToSocket ((void *)converter.GetCPointer(), (uLONG)converter.GetSize());
}


bool VHTTPClient::_IsChunkedResponse()
{
	XBOX::VString	headerValue;

	if (fResponseHeader.GetHeaderValue (CONST_STRING_TRANSFER_ENCODING, headerValue) && EqualASCIIString (headerValue, CONST_STRING_CHUNKED))
		return true;

	return false;
}


#if WITH_HTTP_CLIENT_DEBUG_LOG
void VHTTPClient::_LogData(void *inData, XBOX::VSize inDataSize)
{
	if (NULL == inData)
		return;

	if (NULL != fLogFileDesc)
		fLogFileDesc->PutDataAtPos(inData, inDataSize);
}
#endif

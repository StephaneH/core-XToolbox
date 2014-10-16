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
// Implementation taken from xercesc

/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "VKernelPrecompiled.h"
#include "VError.h"
#include "Base64Coder.h"

static const size_t	B64CODER_BASELENGTH	= 255;
static const size_t	B64CODER_FOURBYTE	= 4;
static const uBYTE	BASE64_PADDING		= 0x3D;

const uBYTE Base64Coder::sBase64Alphabet[] = {
    0x41, 0x42, 0x43, 0x44, 0x45, /* 'A', 'B', 'C', ... */
    0x46, 0x47, 0x48, 0x49, 0x4A,
    0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54,
    0x55, 0x56, 0x57, 0x58, 0x59, 0x5A,
    0x61, 0x62, 0x63, 0x64, 0x65,
    0x66, 0x67, 0x68, 0x69, 0x6A,
    0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
    0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x78, 0x79, 0x7A,
	0x30, 0x31, 0x32, 0x33, 0x34, /* '1', '2', '3', ... */
	0x35, 0x36, 0x37, 0x38, 0x39,
	0x2B, 0x2F, 0x00
};

uBYTE Base64Coder::sBase64Inverse[B64CODER_BASELENGTH];



// -----------------------------------------------------------------------
//  Helper methods
// -----------------------------------------------------------------------
inline bool isPad(const uBYTE& octet)
{
    return ( octet == BASE64_PADDING );
}

inline uBYTE set1stOctet(const uBYTE& b1, const uBYTE& b2)
{
    return (( b1 << 2 ) | ( b2 >> 4 ));
}

inline uBYTE set2ndOctet(const uBYTE& b2, const uBYTE& b3)
{
    return (( b2 << 4 ) | ( b3 >> 2 ));
}

inline uBYTE set3rdOctet(const uBYTE& b3, const uBYTE& b4)
{
    return (( b3 << 6 ) | b4 );
}

inline void split1stOctet(const uBYTE& ch, uBYTE& b1, uBYTE& b2)
{
    b1 = ch >> 2;
    b2 = ( ch & 0x3 ) << 4;
}

inline void split2ndOctet(const uBYTE& ch, uBYTE& b2, uBYTE& b3)
{
    b2 |= ch >> 4;  // combine with previous value
    b3 = ( ch & 0xf ) << 2;
}

inline void split3rdOctet(const uBYTE& ch, uBYTE& b3, uBYTE& b4)
{
    b3 |= ch >> 6;  // combine with previous value
    b4 = ( ch & 0x3f );
}

bool Base64Coder::Encode( const void *inInputData, size_t inInputSize, VMemoryBuffer<>&	outResult, sLONG inQuadsPerLine)
{
	xbox_assert(inQuadsPerLine > 0);

	Init();

	outResult.Clear();

    if (inInputData == NULL)
        return false;

    size_t quadrupletCount = ( inInputSize + 2 ) / 3;
    if (quadrupletCount == 0)
        return false;

    // Number of rows in encoded stream (*not* including the last one, because we don't add a "final" line break)
	size_t lineCount = quadrupletCount / inQuadsPerLine;

	// Compute size of encoded string, note that we add 2 bytes per line breaks, and that it is not null ended.

	if (!outResult.SetSize( quadrupletCount*B64CODER_FOURBYTE + lineCount * 2))
		return false;

    //
    // convert the triplet(s) to quadruplet(s)
    //
    uBYTE  b1, b2, b3, b4;  // base64 binary codes ( 0..63 )

	size_t inputIndex = 0;
	size_t outputIndex = 0;

	uBYTE *encodedData = (uBYTE*) outResult.GetDataPtr();
	const uBYTE *inputData = (const uBYTE *) inInputData;

	//
	// Process all quadruplet(s) except the last
	//
	size_t quad = 1;
	for (; quad <= quadrupletCount-1; quad++ )
	{
		// read triplet from the input stream
		split1stOctet( inputData[ inputIndex++ ], b1, b2 );
		split2ndOctet( inputData[ inputIndex++ ], b2, b3 );
		split3rdOctet( inputData[ inputIndex++ ], b3, b4 );

		// write quadruplet to the output stream
		encodedData[ outputIndex++ ] = sBase64Alphabet[ b1 ];
		encodedData[ outputIndex++ ] = sBase64Alphabet[ b2 ];
		encodedData[ outputIndex++ ] = sBase64Alphabet[ b3 ];
		encodedData[ outputIndex++ ] = sBase64Alphabet[ b4 ];

		// Use CRLF for line breaks.

		if (( quad % inQuadsPerLine) == 0 ) {

			encodedData[ outputIndex++ ] = 0x0D;
			encodedData[ outputIndex++ ] = 0x0A;

		}
	}

	//
	// process the last Quadruplet
	//
	// first octet is present always, process it
	split1stOctet( inputData[ inputIndex++ ], b1, b2 );
	encodedData[ outputIndex++ ] = sBase64Alphabet[ b1 ];

	if( inputIndex < inInputSize)
	{
		// second octet is present, process it
		split2ndOctet( inputData[ inputIndex++ ], b2, b3 );
		encodedData[ outputIndex++ ] = sBase64Alphabet[ b2 ];

		if( inputIndex < inInputSize )
		{
			// third octet present, process it
			// no PAD e.g. 3cQl
			split3rdOctet( inputData[ inputIndex++ ], b3, b4 );
			encodedData[ outputIndex++ ] = sBase64Alphabet[ b3 ];
			encodedData[ outputIndex++ ] = sBase64Alphabet[ b4 ];
		}
		else
		{
			// third octet not present
			// one PAD e.g. 3cQ=
			encodedData[ outputIndex++ ] = sBase64Alphabet[ b3 ];
			encodedData[ outputIndex++ ] = BASE64_PADDING;
		}
	}
	else
	{
		// second octet not present
		// two PADs e.g. 3c==
		encodedData[ outputIndex++ ] = sBase64Alphabet[ b2 ];
		encodedData[ outputIndex++ ] = BASE64_PADDING;
		encodedData[ outputIndex++ ] = BASE64_PADDING;
	}

	xbox_assert(outResult.GetDataSize() == outputIndex);

	// write out end of the last line
	//encodedData[ outputIndex++ ] = 0x0A;
	// write out end of string
	// encodedData[ outputIndex ] = 0;

	outResult.ShrinkSizeNoReallocate( outputIndex);

	return true;
}


void Base64Coder::Init()
{
	static bool sInitialized = false;
	
	if (sInitialized)
		return;

	// create inverse table for base64 decoding
	// if sBase64Alphabet[ 17 ] = 'R', then sBase64Inverse[ 'R' ] = 17
	// for characters not in sBase64Alphabet the sBase64Inverse[] = -1

	size_t i;
	// set all fields to -1
	for ( i = 0; i < B64CODER_BASELENGTH; i++ )
		sBase64Inverse[i] = 0xff;

	// compute inverse table
	for ( i = 0; i < 64; i++ )
		sBase64Inverse[ sBase64Alphabet[i] ] = (uBYTE)i;

	sInitialized = true;
}


bool Base64Coder::Decode ( const void *inInputData, size_t inInputSize, VMemoryBuffer<>& outResult, Conformance inConform)
{
	Init();
	
	outResult.Clear();

	if ((inInputData == NULL) || (inInputSize == 0))
		return false;
	
	//
	// remove all XML whitespaces from the base64Data
	//
	VMemoryBuffer<> rawInputBuffer;

	const uBYTE *inputData = (const uBYTE*) inInputData;
	size_t inputIndex = 0;
	const uBYTE *rawInputData = inputData;
	size_t rawInputLength = inInputSize;

	switch( inConform)
	{
		case Conf_RFC2045:
			{
				static const uBYTE whites[] = {0x20, 0x09, 0x0d, 0x0a};
				if (std::find_first_of( inputData, inputData + inInputSize, whites, whites+sizeof(whites)) != inputData + inInputSize)
				{
					if (rawInputBuffer.SetSize( inInputSize))
					{
						rawInputData = (const uBYTE*) rawInputBuffer.GetDataPtr();
						rawInputLength = 0;
						while ( inputIndex < inInputSize )
						{
							//if (!XMLChar1_0::isWhitespace(inputData[inputIndex]))
							if (std::find( whites, whites+sizeof(whites), inputData[inputIndex]) == whites+sizeof(whites))
							{
								const_cast<uBYTE*>(rawInputData)[ rawInputLength++ ] = inputData[ inputIndex ];
							}
							// RFC2045 does not explicitly forbid more than ONE whitespace 
							// before, in between, or after base64 octects.
							// Besides, S? allows more than ONE whitespace as specified in the production 
							// [3]   S   ::=   (#x20 | #x9 | #xD | #xA)+
							// therefore we do not detect multiple ws

							inputIndex++;
						}
					}
					else
					{
						vThrowError( VE_MEMORY_FULL);
						rawInputData = NULL;
						rawInputLength = 0;
					}
				}
				break;
			}

		case Conf_Schema:
			/*
			// no leading #x20
			if (chSpace == inputData[inputIndex])
				return 0;

			while ( inputIndex < inputLength )
			{
				if (chSpace != inputData[inputIndex])
				{
					rawInputData[ rawInputLength++ ] = inputData[ inputIndex ];
					inWhiteSpace = false;
				}
				else
				{
					if (inWhiteSpace)
						return 0; // more than 1 #x20 encountered
					else
						inWhiteSpace = true;
				}

				inputIndex++;
			}

			// no trailing #x20
			if (inWhiteSpace)
				return 0;
			*/
			break;

		default:
			break;
    }

	if (rawInputData == NULL)
		return false;

    //now rawInputData contains canonical representation 
    //if the data is valid Base64

    // the length of raw data should be divisible by four
    if (!testAssert(( rawInputLength % B64CODER_FOURBYTE ) == 0) )
        return false;

    size_t quadrupletCount = rawInputLength / B64CODER_FOURBYTE;
    if ( quadrupletCount == 0 )
        return true;

	if (!outResult.SetSize( quadrupletCount*3 + 1))
		return false;

    //
    // convert the quadruplet(s) to triplet(s)
    //
    uBYTE d1, d2, d3, d4;  // base64 characters
    uBYTE b1, b2, b3, b4;  // base64 binary codes ( 0..64 )

    size_t rawInputIndex  = 0;
    size_t outputIndex    = 0;
	
    uBYTE *decodedData = (uBYTE *) outResult.GetDataPtr();

    //
    // Process all quadruplet(s) except the last
    //
    size_t quad = 1;
    for (; quad <= quadrupletCount-1; quad++ )
    {
        // read quadruplet from the input stream
        if (!isData( (d1 = rawInputData[ rawInputIndex++ ]) ) ||
            !isData( (d2 = rawInputData[ rawInputIndex++ ]) ) ||
            !isData( (d3 = rawInputData[ rawInputIndex++ ]) ) ||
            !isData( (d4 = rawInputData[ rawInputIndex++ ]) ))
        {
            // if found "no data" just return NULL
            return false;
        }

        b1 = sBase64Inverse[ d1 ];
        b2 = sBase64Inverse[ d2 ];
        b3 = sBase64Inverse[ d3 ];
        b4 = sBase64Inverse[ d4 ];

        // write triplet to the output stream
        decodedData[ outputIndex++ ] = set1stOctet(b1, b2);
        decodedData[ outputIndex++ ] = set2ndOctet(b2, b3);
        decodedData[ outputIndex++ ] = set3rdOctet(b3, b4);
    }

    //
    // process the last Quadruplet
    //
    // first two octets are present always, process them
    if (!isData( (d1 = rawInputData[ rawInputIndex++ ]) ) ||
        !isData( (d2 = rawInputData[ rawInputIndex++ ]) ))
    {
        // if found "no data" just return NULL
        return false;
    }

    b1 = sBase64Inverse[ d1 ];
    b2 = sBase64Inverse[ d2 ];

    // try to process last two octets
    d3 = rawInputData[ rawInputIndex++ ];
    d4 = rawInputData[ rawInputIndex++ ];

    if (!isData( d3 ) || !isData( d4 ))
    {
        // check if last two are PAD characters
        if (isPad( d3 ) && isPad( d4 ))
        {
            // two PAD e.g. 3c==
            if ((b2 & 0xf) != 0) // last 4 bits should be zero
            {
                return false;
            }

            decodedData[ outputIndex++ ] = set1stOctet(b1, b2);
        }
        else if (!isPad( d3 ) && isPad( d4 ))
        {
            // one PAD e.g. 3cQ=
            b3 = sBase64Inverse[ d3 ];
            if (( b3 & 0x3 ) != 0 ) // last 2 bits should be zero
            {
                return false;
            }

            decodedData[ outputIndex++ ] = set1stOctet( b1, b2 );
            decodedData[ outputIndex++ ] = set2ndOctet( b2, b3 );
        }
        else
        {
            // an error like "3c[Pad]r", "3cdX", "3cXd", "3cXX" where X is non data
            return false;
        }
    }
    else
    {
        // no PAD e.g 3cQl
        b3 = sBase64Inverse[ d3 ];
        b4 = sBase64Inverse[ d4 ];
        decodedData[ outputIndex++ ] = set1stOctet( b1, b2 );
        decodedData[ outputIndex++ ] = set2ndOctet( b2, b3 );
        decodedData[ outputIndex++ ] = set3rdOctet( b3, b4 );
    }

    // write out the end of string
    outResult.ShrinkSizeNoReallocate( outputIndex);

	return true;
}





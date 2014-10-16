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
#ifndef __BASE_64_CODER__
#define __BASE_64_CODER__

#include "VMemoryBuffer.h"

BEGIN_TOOLBOX_NAMESPACE

/* The code was borrowed from xerces project. */
class XTOOLBOX_API Base64Coder
{
public:
	enum Conformance
	{
		Conf_RFC2045,
		Conf_Schema
	};

	static const size_t	BASE64_QUADSPERLINE	= 10000;	

	// For SMTP (max 998 bytes per line), set inQuadsPerLine to 249 because 4 * 249 + 2 = 998 (CRLF = 2 bytes).	

	static	bool	Encode( const void *inData, size_t inDataSize, VMemoryBuffer<>& outResult, sLONG inQuadsPerLine = BASE64_QUADSPERLINE);

	static	bool	Decode( const void *inData, size_t inDataSize, VMemoryBuffer<>& outResult, Conformance inConform = Conf_RFC2045 );
	
private:
    Base64Coder();
    Base64Coder(const Base64Coder&);

    static	void			Init();
	static	bool			isData(const uBYTE& octet)	{ return sBase64Inverse[octet] != 0xff; }

    static	const uBYTE		sBase64Alphabet[];
    static	uBYTE			sBase64Inverse[];
};


END_TOOLBOX_NAMESPACE

#endif /* __BASE_64_CODER__ */


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
#ifndef __XLinuxTextConverter__
#define __XLinuxTextConverter__

#include "Kernel/Sources/VTextConverter.h"
#include "unicode/ucnv.h"

BEGIN_TOOLBOX_NAMESPACE


// sizeof("toto") : 5
// sizeof(L"toto") : 20
// sizeof(wchar_t) : 4

class XLinuxToUnicodeConverter : public VToUnicodeConverter
{
public:
			XLinuxToUnicodeConverter (CharSet inCharSet);
	virtual ~XLinuxToUnicodeConverter ();

	virtual bool	Convert( const void *inSource, VSize inSourceBytes, VSize *outBytesConsumed, UniChar* inDestination, VIndex inDestinationChars, VIndex *outProducedChars);
	virtual bool	IsValid() const;

	
private:

    UConverter* fConverter; //ICU converter
};


class XLinuxFromUnicodeConverter : public VFromUnicodeConverter
{
public:
			XLinuxFromUnicodeConverter (CharSet inCharSet);
	virtual ~XLinuxFromUnicodeConverter ();

	virtual bool	Convert( const UniChar* inSource, VIndex inSourceChars, VIndex *outCharsConsumed, void* inBuffer, VSize inBufferSize, VSize *outBytesProduced);
	virtual VSize	GetCharSize() const;
	virtual bool	IsValid() const;
	
private:

    UConverter* fConverter; //ICU converter
};

END_TOOLBOX_NAMESPACE

#endif

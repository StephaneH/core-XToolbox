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
#ifndef __XMacTextConverter__
#define __XMacTextConverter__

#include "Kernel/Sources/VTextConverter.h"


BEGIN_TOOLBOX_NAMESPACE

class XMacToUnicodeConverter : public VToUnicodeConverter
{
public:
			XMacToUnicodeConverter (CharSet inCharSet);
	virtual ~XMacToUnicodeConverter ();

	virtual bool	Convert( const void *inSource, VSize inSourceBytes, VSize *outBytesConsumed, UniChar* inDestination, VIndex inDestinationChars, VIndex *outProducedChars);
	virtual bool	IsValid() const		{ return  (fRef != NULL || fTECObjectRef != NULL); }

	// Public utils
	static TextEncoding	CharSetToTextEncoding (CharSet inCharSet);
	
private:
	TextEncoding		fSource;
	TextToUnicodeInfo	fRef;

	TECObjectRef		fTECObjectRef;
	TextEncoding		fDestination;
};


class XMacFromUnicodeConverter : public VFromUnicodeConverter
{
public:
			XMacFromUnicodeConverter (CharSet inCharSet);
	virtual ~XMacFromUnicodeConverter ();

	virtual bool	Convert( const UniChar* inSource, VIndex inSourceChars, VIndex *outCharsConsumed, void* inBuffer, VSize inBufferSize, VSize *outBytesProduced);
	virtual VSize	GetCharSize() const;
	virtual bool	IsValid() const		{ return  (fRef != NULL || fTECObjectRef != NULL); }
	
private:
	TextEncoding		fDestination;
	UnicodeToTextInfo	fRef;

	TECObjectRef		fTECObjectRef;
	TextEncoding		fSource;
};

END_TOOLBOX_NAMESPACE

#endif

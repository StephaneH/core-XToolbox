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
#include "XLinuxTextConverter.h"
//#include "XLinuxCharSets.h"
#include "VError.h"

//#include "unicode/utypes.h"
#include "unicode/ucnv.h"
#include "VCharSetNames.h"

////////////////////////////////////////////////////////////////////////////////
//
// XLinuxToUnicodeConverter
//
////////////////////////////////////////////////////////////////////////////////


XLinuxToUnicodeConverter::XLinuxToUnicodeConverter(CharSet inCharSet) :
    fConverter(NULL)
{
    const char* converterName=NULL;
    
	for(VCSNameMap* charSetName=sCharSetNameMap ; charSetName->fCharName ; charSetName++)
	{
		if(charSetName->fCharSet==inCharSet)
		{
            converterName=charSetName->fCharName;
			break;
		}
	}

    if(converterName!=NULL)
    {
        UErrorCode err=U_ZERO_ERROR;
        fConverter=ucnv_open(converterName, &err);
    }
}


XLinuxToUnicodeConverter::~XLinuxToUnicodeConverter()
{
    if(fConverter!=NULL)
        ucnv_close(fConverter);
}


bool XLinuxToUnicodeConverter::Convert(const void* inSource, VSize inSourceBytes, VSize *outBytesConsumed,
                                       UniChar* inDestination, VIndex inDestinationChars, VIndex *outProducedChars)
{
    if(!IsValid())
    {
        *outBytesConsumed=0, *outProducedChars=0;
        return false;
    }

    UniChar* target=reinterpret_cast<UniChar*>(inDestination);
    UniChar* targetPos=target;
    UniChar* targetLimit=target+inDestinationChars;

    const char* source=reinterpret_cast<const char*>(inSource);
    const char* sourcePos=source;
    const char* sourceLimit=source+inSourceBytes;

    UErrorCode err=U_ZERO_ERROR;

    //jmo - I assume we only perform one shot conversions : Although our API doesn't prevent it, a
    //      stream cannot be split in arbitrary way and converted accros multiple calls to Convert().

    ucnv_toUnicode(fConverter, 
                   &targetPos,  //will point after the last UniChar used
                   targetLimit,
                   &sourcePos,  //will point after the last char used
                   sourceLimit,
                   NULL,        //offsets - we don't need it
                   true,        //flush - we only perform one shot conversions
                   &err);

    *outBytesConsumed = source!=NULL && sourcePos > source ? sourcePos-source : 0;
    *outProducedChars = static_cast<VIndex>(target!=NULL && targetPos > target ? targetPos-target : 0);
    
    if(err!=U_ZERO_ERROR)
        ucnv_reset(fConverter); //ready for next use !

    return err==U_ZERO_ERROR;
}


bool XLinuxToUnicodeConverter::IsValid() const
{
    return fConverter!=NULL;
}



////////////////////////////////////////////////////////////////////////////////
//
// XLinuxFromUnicodeConverter
//
////////////////////////////////////////////////////////////////////////////////


XLinuxFromUnicodeConverter::XLinuxFromUnicodeConverter(CharSet inCharSet) :
    fConverter(NULL)
{
    const char* converterName=NULL;
    
	for(VCSNameMap* charSetName=sCharSetNameMap ; charSetName->fCharName ; charSetName++)
	{
		if(charSetName->fCharSet==inCharSet)
		{
            converterName=charSetName->fCharName;
			break;
		}
	}

    if(converterName!=NULL)
    {
        UErrorCode err=U_ZERO_ERROR;
        fConverter=ucnv_open(converterName, &err);
    }
}


XLinuxFromUnicodeConverter::~XLinuxFromUnicodeConverter()
{
    if(fConverter!=NULL)
        ucnv_close(fConverter);
}


bool XLinuxFromUnicodeConverter::Convert(const UniChar* inSource, VIndex inSourceChars, VIndex *outCharsConsumed,
                                         void* inBuffer, VSize inBufferSize, VSize *outBytesProduced)
{
    if(!IsValid())
    {
        *outCharsConsumed=0, *outBytesProduced=0;
        return false;
    }

    const UniChar* source=inSource;
    const UniChar* sourcePos=source;
    const UniChar* sourceLimit=source+inSourceChars;

    char* target=reinterpret_cast<char*>(inBuffer);
    char* targetPos=target;
    char* targetLimit=target+inBufferSize;

    UErrorCode err=U_ZERO_ERROR;

    //jmo - I assume we only perform one shot conversions : Although our API doesn't prevent it, a
    //      stream cannot be split in arbitrary way and converted accros multiple calls to Convert().

    ucnv_fromUnicode(fConverter, 
                     &targetPos,  //will point after the last char used
                     targetLimit,
                     &sourcePos,  //will point after the last UniChar used
                     sourceLimit,
                     NULL,        //offsets - we don't need it
                     true,        //flush - we only perform one shot conversions
                     &err);

    *outCharsConsumed = static_cast<VIndex>(source!=NULL && sourcePos > source ? sourcePos-source : 0);
    *outBytesProduced = target!=NULL && targetPos > target ? targetPos-target : 0;

    if(err!=U_ZERO_ERROR)
        ucnv_reset(fConverter); //ready for next use !

    return err==U_ZERO_ERROR;
}


VSize XLinuxFromUnicodeConverter::GetCharSize() const
{
    //jmo - todo : verifier que cette methode ne pose pas pb lorsque la taille des char est variable...
    //int min=ucnv_getMinCharSize(fConverter);
    int max=ucnv_getMaxCharSize(fConverter);

    return max;
}


bool XLinuxFromUnicodeConverter::IsValid() const
{
    return fConverter!=NULL;
}

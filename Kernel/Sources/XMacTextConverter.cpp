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
#include "XMacTextConverter.h"
#include "XMacCharSets.h"
#include "VError.h"


XMacToUnicodeConverter::XMacToUnicodeConverter(CharSet inCharSet)
{
	fRef = NULL;
	fSource = CharSetToTextEncoding(inCharSet);
	fTECObjectRef = NULL;
	fDestination = 0;
	
	// YT 06-Oct-2009 - ACI0060290
	/*
	 see: http://developer.apple.com/mac/library/documentation/Carbon/Conceptual/ProgWithTECM/tecmgr_about/tecmgr_about.html#//apple_ref/doc/uid/TP40000932-CH201-CJBEJFBF
	 
	 Character Encoding Schemes
	 
	 A character encoding scheme is a mapping from a sequence of elements in one or more coded character sets to a sequence of bytes.
	 A character encoding scheme can include coded character sets, but it can also include more complex mapping schemes that combine
	 multiple coded character sets, typically in one of the following ways:
	 
	 *	Packing schemes use a sequence of 8-bit values to encode text. Because of this, they are generally not suitable for electronic mail.
	 In these schemes, certain characters function as a local shift, which controls the interpretation of the next 1 to 3 bytes.
	 The most well known example is Shift-JIS, which includes characters from JIS X0201, JIS X0208, and space for 2444 user-defined characters.
	 The EUC (Extended UNIX Coding) packing schemes were originally developed for UNIX systems; they use units of 1 to 4 bytes.
	 (Appendix B describes Shift-JIS, EUC, and other packing schemes, in detail.) Packing schemes are often used for the World Wide Web, which
	 can handle 8-bit values. Both the Text Encoding Converter and the Unicode Converter support packing schemes.
	 *	Code-switching schemes typically use a sequence of 7-bit values to encode text, so they are suitable for electronic mail. Escape sequences
	 or other special sequences are used to signal a shift among the included character sets. Examples include the ISO 2022 family of encodings
	 (such as ISO 2022-JP), and the HZ encoding used for Chinese. Code switching schemes are often used for Internet mail and news,
	 which cannot handle 8-bit values. The Text Encoding Converter can handle code-switching schemes, but the Unicode Converter cannot.
	 */
	
	if (fSource != -1)
	{
		OSStatus status = ::CreateTextToUnicodeInfoByEncoding(fSource, &fRef);
		if(status==kTextUnsupportedEncodingErr) // pp add fall back for XBOX::VTC_JIS
		{
			fDestination = ::CreateTextEncoding(kTextEncodingUnicodeDefault, kTextEncodingDefaultVariant, kUnicode16BitFormat); // UTF-16
			status = ::TECCreateConverter(&fTECObjectRef, fSource, fDestination);
		}
	}
}


XMacToUnicodeConverter::~XMacToUnicodeConverter()
{
	if (fRef != NULL)
		::DisposeTextToUnicodeInfo(&fRef);
	
	if (fTECObjectRef != NULL)
		::TECDisposeConverter(fTECObjectRef);
}


bool XMacToUnicodeConverter::Convert(const void* inSource, VSize inSourceBytes, VSize *outBytesConsumed, UniChar* inDestination, VIndex inDestinationChars, VIndex *outProducedChars)
{
	ByteCount	bytesConsumed = 0;
	ByteCount	bytesProduced = 0;
	OptionBits	controlFlags = kUnicodeUseFallbacksMask
		// | kUnicodeKeepInfoMask
		| kUnicodeDefaultDirectionMask
		| kUnicodeLooseMappingsMask
		// | kUnicodeStringUnterminatedMask
		// | kUnicodeTextRunMask
		;

	OSStatus status = noErr;

	if (fRef != NULL)
	{
		status = ::ConvertFromTextToUnicode(fRef,
											inSourceBytes,
											inSource,
											controlFlags
											,0, NULL, 0, NULL,
											inDestinationChars * sizeof(UniChar),
											&bytesConsumed,
											&bytesProduced,
											inDestination);
	}
	else
	{
		status = ::TECConvertText(fTECObjectRef,
								  (const UInt8 *)inSource,
								  inSourceBytes,
								  &bytesConsumed,
								  (UInt8 *)inDestination,
								  inDestinationChars * sizeof(UniChar),
								  &bytesProduced);
	}
	
    // pp pour avoir un comportement cross, kTECPartialCharErr n'est pas considerer comme une erreur
    // la gestion du cas ou outBytesConsumed<inSourceBytes est dans la class VTextConverter
    if (status == kTECOutputBufferFullStatus || status==kTECPartialCharErr)
    	status = noErr;

	if (status != noErr)
	{
		vThrowNativeError(status, "ConvertFromTextToUnicode failed");
		if (outBytesConsumed != NULL)
			*outBytesConsumed = 0;
		if (outProducedChars != NULL)
			*outProducedChars = 0;
	}
	else
	{
		xbox_assert(bytesProduced % sizeof(UniChar) == 0);
		
		if (outBytesConsumed != NULL)
			*outBytesConsumed = bytesConsumed;
		if (outProducedChars != NULL)
			*outProducedChars = (sLONG) (bytesProduced / sizeof(UniChar));
	}
	
	return status == noErr;
}


TextEncoding XMacToUnicodeConverter::CharSetToTextEncoding(CharSet inCharSet)
{
	TextEncoding	result = kTextEncodingUnknown;
	
	switch(inCharSet) {
	
		case VTC_MacOSAnsi:
			result = kTextEncodingMacRoman;	//::GetApplicationTextEncoding(); unavailable in CoreServices. CFStringGetSystemEncoding is not the right one
			break;

#if 0
		case VTC_SystemWinCharset:
			{
				result = ::GetApplicationTextEncoding();
				TextEncodingBase base = ::GetTextEncodingBase(result);
				
				// look for an equivalent for windows
				for (MacToWinCharSetMap* mw = sMacToWinCharSetMap ; mw->fMacBase != kTextEncodingUnknown; mw++) {
					if (mw->fMacBase == base) {
						result = ::CreateTextEncoding(mw->fWinBase, kTextEncodingDefaultVariant, kTextEncodingDefaultFormat);	
						break;
					}
				}
				break;
			}
#endif

		case VTC_UTF_16:
			result = ::CreateTextEncoding(kTextEncodingUnicodeDefault, kTextEncodingDefaultVariant, kUnicode16BitFormat);
			break;
		
		case VTC_UTF_8:
			result = ::CreateTextEncoding(kTextEncodingUnicodeDefault, kTextEncodingDefaultVariant, kUnicodeUTF8Format);
			break;
		
		/*
			See note for kUnicodeUTF7Format here: <http://developer.apple.com/documentation/macos8/TextIntlSvcs/TextEncodingConversionManager/TEC1.5/TEC.1c.html>
			The Unicode transformation format in which characters encodings are represented by a sequence of 7-bit values. This format cannot be handled by the
			Unicode Converter, only by the Text Encoding Converter.
		*/
		 case VTC_UTF_7:
			 result = kUnicodeUTF7Format;//::CreateTextEncoding(kTextEncodingUnicodeDefault, kTextEncodingDefaultVariant, kUnicodeUTF7Format);	
			break;
		
		case VTC_UTF_32:
			 result = ::CreateTextEncoding(kTextEncodingUnicodeDefault, kTextEncodingDefaultVariant, kUnicodeUTF32Format);
			break;
			
		default:
			{
				MAC_CharSetMap* mt;
				
				for (mt = sMAC_CharSetMap ; mt->fCode != VTC_UNKNOWN; mt++) 
				{
					if (mt->fCode == inCharSet) 
					{
						result = ::CreateTextEncoding(mt->fEncodingBase, mt->fEncodingVariant, mt->fEncodingFormat );	
						break;
					}
				}
				break;
			}
	}
	
//	xbox_assert( result != kTextEncodingUnknown);
		
	return result;
}


#pragma mark-

XMacFromUnicodeConverter::XMacFromUnicodeConverter(CharSet inCharsetCode)
{
	fRef = NULL;
	fDestination = XMacToUnicodeConverter::CharSetToTextEncoding(inCharsetCode);
	fTECObjectRef = NULL;
	fSource = 0;

	// YT 06-Oct-2009 - ACI0060290
	/*
	 see: http://developer.apple.com/mac/library/documentation/Carbon/Conceptual/ProgWithTECM/tecmgr_about/tecmgr_about.html#//apple_ref/doc/uid/TP40000932-CH201-CJBEJFBF
	 
	 Character Encoding Schemes
	 
	 A character encoding scheme is a mapping from a sequence of elements in one or more coded character sets to a sequence of bytes.
	 A character encoding scheme can include coded character sets, but it can also include more complex mapping schemes that combine
	 multiple coded character sets, typically in one of the following ways:
	 
	 *	Packing schemes use a sequence of 8-bit values to encode text. Because of this, they are generally not suitable for electronic mail.
	 In these schemes, certain characters function as a local shift, which controls the interpretation of the next 1 to 3 bytes.
	 The most well known example is Shift-JIS, which includes characters from JIS X0201, JIS X0208, and space for 2444 user-defined characters.
	 The EUC (Extended UNIX Coding) packing schemes were originally developed for UNIX systems; they use units of 1 to 4 bytes.
	 (Appendix B describes Shift-JIS, EUC, and other packing schemes, in detail.) Packing schemes are often used for the World Wide Web, which
	 can handle 8-bit values. Both the Text Encoding Converter and the Unicode Converter support packing schemes.
	 *	Code-switching schemes typically use a sequence of 7-bit values to encode text, so they are suitable for electronic mail. Escape sequences
	 or other special sequences are used to signal a shift among the included character sets. Examples include the ISO 2022 family of encodings
	 (such as ISO 2022-JP), and the HZ encoding used for Chinese. Code switching schemes are often used for Internet mail and news,
	 which cannot handle 8-bit values. The Text Encoding Converter can handle code-switching schemes, but the Unicode Converter cannot.
	 */
	
	if (fDestination != -1)
	{
		OSStatus status = ::CreateUnicodeToTextInfoByEncoding(fDestination, &fRef);
		if(status==kTextUnsupportedEncodingErr) // pp add fall back, some encoding like XBOX::VTC_JIS, GB2312-80 are not supported by unicode converter
		{
			fSource = ::CreateTextEncoding(kTextEncodingUnicodeDefault, kTextEncodingDefaultVariant, kUnicode16BitFormat); // UTF-16
			status = ::TECCreateConverter(&fTECObjectRef, fSource, fDestination);
		}
	}
}


XMacFromUnicodeConverter::~XMacFromUnicodeConverter()
{
	if (fRef != NULL)
		::DisposeUnicodeToTextInfo(&fRef);

	if (fTECObjectRef != NULL)
		::TECDisposeConverter(fTECObjectRef);
}


bool XMacFromUnicodeConverter::Convert(const UniChar* inSource, VIndex inSourceChars, VIndex *outCharsConsumed, void* inBuffer, VSize inBufferSize, VSize *outBytesProduced)
{
	ByteCount	bytesConsumed = 0;
	ByteCount	bytesProduced = 0;
    
	OptionBits	controlFlags =
		0
		// | kUnicodeUseFallbacksMask
		| kUnicodeLooseMappingsMask
		// | kUnicodeKeepInfoMask
		// | kUnicodeStringUnterminatedMask
		;
		
	OSStatus status = noErr;
	
	if (fRef != NULL)
	{
		status = ::ConvertFromUnicodeToText( fRef,
											 inSourceChars * sizeof(UniChar),
											 inSource,
											 controlFlags,
											 0,NULL,0,NULL,
											 inBufferSize,
											 &bytesConsumed,
											 &bytesProduced,
											 inBuffer);
	}
	else
	{
		status = ::TECConvertText(	fTECObjectRef,
									(const UInt8 *)inSource,
									inSourceChars * sizeof(UniChar),
									&bytesConsumed,
									(UInt8 *)inBuffer,
									inBufferSize,
									&bytesProduced);
	}
    
    if (status == kTECOutputBufferFullStatus)
    	status = noErr;
    
    if (status != noErr && status != kTECUnmappableElementErr)
	{
		vThrowNativeError(status, "ConvertFromUnicodeToText failed.");
		if (outCharsConsumed != NULL)
			*outCharsConsumed = 0;
		if (outBytesProduced != NULL)
			*outBytesProduced = 0;
    }
	else
	{
		if( status == kTECUnmappableElementErr )	// Bad source data. Skip a byte and try again.
		{
			bool isSurrogate=false;
			if( bytesProduced < (inBufferSize/sizeof(UniChar)) )
				((char *)inBuffer)[bytesProduced] = 0x3F; // ?
			bytesProduced++;
			if(inSource[bytesConsumed/sizeof(UniChar)]>=0xD800 && inSource[bytesConsumed/sizeof(UniChar)]<=0xD8FF) // pp unicode surrogate pair leading char
			{
				bytesConsumed += sizeof(UniChar)*2;
			}
			else
				bytesConsumed += sizeof(UniChar);
			
			status = noErr;
		}

		xbox_assert(bytesConsumed % sizeof(UniChar) == 0);
		if (outCharsConsumed != NULL)
			*outCharsConsumed = (sLONG) (bytesConsumed / sizeof(UniChar));
		if (outBytesProduced != NULL)
			*outBytesProduced = bytesProduced;
	}

    return status == noErr;
}


VSize XMacFromUnicodeConverter::GetCharSize() const
{
	VSize	size;
	
	TextEncodingFormat	format = ::GetTextEncodingFormat(fDestination);
	switch (format)
	{
		case kUnicodeUTF16Format:
		case kUnicodeUTF16BEFormat:
		case kUnicodeUTF16LEFormat:
			size = sizeof(uWORD);
			break;
			
		case kUnicodeUTF7Format:
			size = sizeof(uBYTE);
			break;
			
		case kUnicodeUTF8Format:
			size = sizeof(uBYTE);
			break;
			
		case kUnicodeUTF32Format:
		case kUnicodeUTF32BEFormat:
		case kUnicodeUTF32LEFormat:
			size = sizeof(uLONG);
			break;
			
		default:
			xbox_assert(false);	// Unhandled format
			size = sizeof(uBYTE);
			break;
	}
	
	return size;
}

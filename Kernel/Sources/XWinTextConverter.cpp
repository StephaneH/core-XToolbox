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
#include "XWinTextConverter.h"
#include "XWinCharSets.h"
#include "VError.h"


XWinToUnicodeConverter::XWinToUnicodeConverter(IMultiLanguage2* inMLang, CharSet inCharSet)
{
	fCodePage = CharSetToCodePage(inCharSet);
	fMLang = inMLang;
	fMLang->AddRef();
	HRESULT r = fMLang->CreateConvertCharset(fCodePage, 1200 /* utf16 */, MLCONVCHARF_USEDEFCHAR/*0*/ /*MLCONVCHARF_NONE*/, &fConverter);
	if (r != S_OK)
	{
		if (fConverter)
			fConverter->Release();
		fConverter = NULL;
	}
}


XWinToUnicodeConverter::~XWinToUnicodeConverter()
{
	if (fConverter != NULL)
		fConverter->Release();
	fMLang->Release();
}


bool XWinToUnicodeConverter::Convert(const void* inSource, VSize inSourceBytes, VSize* outBytesConsumed, UniChar* inDestination, VIndex inDestinationChars, VIndex* outProducedChars)
{
	HRESULT r = S_OK;

	if ((inSourceBytes == 0) || (fConverter == NULL) || (fMLang == NULL))
	{
		if (outBytesConsumed != NULL)
			*outBytesConsumed = 0;
		if (outProducedChars != NULL)
			*outProducedChars = 0;
	}
	else
	{
		VTaskLock lock(&fMutex);

		UINT srcSize = (UINT) inSourceBytes;
		UINT destSize = (UINT) inDestinationChars;
		if (srcSize > destSize)	// assume a one byte char will always fit in one UniChar
			srcSize = destSize;
			
		DWORD dwMode = 0;
        r = fMLang->ConvertStringToUnicodeEx( &dwMode,
											fCodePage,
											(CHAR* )inSource,
											&srcSize,
											(WCHAR* )inDestination,
											&destSize,
											MLCONVCHARF_USEDEFCHAR,
											L"?" );

		if ((r != S_OK) && ::GetLastError() == ERROR_INSUFFICIENT_BUFFER)	// buff too small
		{
			if (destSize != 0)
			{
				// buff too small but the converter succeeded to convert something.
				r = S_OK;
			}
			else
			{
				// have to loop around DoConversionToUnicode to force it to convert something

				UINT charsInBuffer = 0;
				UINT convertedBytes = 0;
				do {
				
					// make sure we won't get ERROR_INSUFFICIENT_BUFFER again
					destSize = (UINT) inDestinationChars - charsInBuffer;
					srcSize = (UINT) inSourceBytes - convertedBytes;
					if (srcSize > destSize)	// assume a one byte char will always fit in one UniChar
						srcSize = destSize;

					dwMode = 0;
					r = fMLang->ConvertStringToUnicodeEx( &dwMode,
														fCodePage,
														(CHAR* ) inSource + convertedBytes,
														&srcSize,
														(WCHAR* ) inDestination + charsInBuffer,
														&destSize,
														MLCONVCHARF_USEDEFCHAR,
														L"?" );
					if (r == S_OK)
					{
						charsInBuffer += destSize;
						convertedBytes += srcSize;
					}
				} while((r == S_OK) && (convertedBytes < (UINT) inSourceBytes) && (charsInBuffer < (UINT) inDestinationChars) && (destSize != 0));

				if ((r != S_OK) && (::GetLastError() == ERROR_INSUFFICIENT_BUFFER) )
					r = S_OK;
				
				srcSize = convertedBytes;
				destSize = charsInBuffer;
			}
		}

		if (r != S_OK && r!= S_FALSE)
		{
			StThrowError<> throwErr( VE_INTL_TEXT_CONVERSION_FAILED, (VNativeError) r);
			if (outBytesConsumed != NULL)
				*outBytesConsumed = 0;
			if (outProducedChars != NULL)
				*outProducedChars = 0;
		}
		else
		{
			if (outBytesConsumed != NULL)
				*outBytesConsumed = srcSize;
			if (outProducedChars != NULL)
				*outProducedChars = destSize;
			r = S_OK;
		}
	}
	
	return r == S_OK;
}


UINT XWinToUnicodeConverter::CharSetToCodePage(CharSet inCharSet)
{
	UINT	result = 0;
	
	switch(inCharSet) {
	
#if 0
		case VTC_SystemMacCharset:
			{
				CPINFOEX info;
				BOOL r = ::GetCPInfoEx(CP_MACCP, 0, &info);
				result = info.CodePage;	// macintosh code page, usually 10000
				break;
			}
#endif

		case VTC_Win32Ansi:
			result = ::GetACP();	// ansi code page, usually 1252
			break;
		
		default:
			{
				for (WIN_CharSetMap* mt = sWIN_CharSetMap ; mt->fCharSet != VTC_UNKNOWN ; mt++)
				{
					if (mt->fCharSet == inCharSet)
					{
						result = mt->fCodePage;
						break;
					}
				}
				break;
			}
	}
	
	return result;
}



//===============================================================================================

XWinFromUnicodeConverter::XWinFromUnicodeConverter(IMultiLanguage2* inMLang, CharSet inCharSet)
{
	fCodePage = XWinToUnicodeConverter::CharSetToCodePage(inCharSet);
	fMLang = inMLang;
	fMLang->AddRef();
	HRESULT r = fMLang->CreateConvertCharset(1200 /* utf16 */, fCodePage, MLCONVCHARF_USEDEFCHAR /*0*/ /*MLCONVCHARF_NONE*/, &fConverter);
	if (r != S_OK)
	{
		if (fConverter)
			fConverter->Release();
		fConverter = NULL;
	}
}


XWinFromUnicodeConverter::~XWinFromUnicodeConverter()
{
	if (fConverter)
		fConverter->Release();
	fMLang->Release();
}


bool XWinFromUnicodeConverter::Convert(const UniChar* inSource, VIndex inSourceChars, VIndex* outCharsConsumed, void* inBuffer, VSize inBufferSize, VSize* outBytesProduced)
{
	HRESULT r = S_OK;

	if ((inSourceChars == 0) || (fConverter == NULL) || (fMLang == NULL))
	{
		if (outCharsConsumed != NULL)
			*outCharsConsumed = 0;
		if (outBytesProduced != NULL)
			*outBytesProduced = 0;
	}
	else
	{
		VTaskLock lock(&fMutex);

		UINT srcSize = (UINT) inSourceChars;
		UINT destSize = (UINT) inBufferSize;
		DWORD dwMode = 0;

		r = fMLang->ConvertStringFromUnicodeEx( &dwMode,
											fCodePage,
											(WCHAR* )inSource,
											&srcSize,
											(CHAR *)inBuffer,
											&destSize,
											MLCONVCHARF_USEDEFCHAR,
											L"?" );

		if ((r != S_OK) && ::GetLastError() == ERROR_INSUFFICIENT_BUFFER)	// buff too small
		{
			if (destSize != 0)
			{
				// buff too small but the converter succeeded to convert something.
				r = S_OK;
			}
			else
			{
				// have to loop around DoConversionFromUnicode to force it to convert something

				UINT bytesInBuffer = 0;
				UINT convertedChars = 0;
				do {
				
					// make sure we won't get ERROR_INSUFFICIENT_BUFFER again
					destSize = (UINT) inBufferSize - bytesInBuffer;
					if (destSize < 6)
					{
						// for very small buffer, do one by one
						srcSize = 1;
					}
					else
					{
						srcSize = inSourceChars - convertedChars;
						if (srcSize > destSize/3)
							srcSize = destSize/3;
					}

					dwMode = 0;
					r = fMLang->ConvertStringFromUnicodeEx( &dwMode,
														fCodePage,
														(WCHAR* ) inSource + convertedChars,
														&srcSize,
														(CHAR* ) inBuffer + bytesInBuffer,
														&destSize,
														MLCONVCHARF_USEDEFCHAR,
														L"?" );
					if (r == S_OK)
					{
						bytesInBuffer += destSize;
						convertedChars += srcSize;
					}
				} while((r == S_OK) && (convertedChars < (UINT) inSourceChars) && (bytesInBuffer < (UINT) inBufferSize) && (destSize != 0));

				if ((r != S_OK) && (::GetLastError() == ERROR_INSUFFICIENT_BUFFER) )
					r = S_OK;
				
				srcSize = convertedChars;
				destSize = bytesInBuffer;
			}
		}

		if (r != S_OK && r != S_FALSE )
		{
			StThrowError<> throwErr( VE_INTL_TEXT_CONVERSION_FAILED, (VNativeError) r);
			if (outCharsConsumed != NULL)
				*outCharsConsumed = 0;
			if (outBytesProduced != NULL)
				*outBytesProduced = 0;
		}
		else
		{
			if (outCharsConsumed != NULL)
				*outCharsConsumed = srcSize;
			if (outBytesProduced != NULL)
				*outBytesProduced = destSize;
			r = S_OK;
		}
	}
	
	return r == S_OK;
}


VSize XWinFromUnicodeConverter::GetCharSize() const
{
	CPINFO	info;
	BOOL	result = ::GetCPInfo(fCodePage, &info);
	return	info.MaxCharSize;
}

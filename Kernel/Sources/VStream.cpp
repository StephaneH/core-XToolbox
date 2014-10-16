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
#include "VStream.h"
#include "VErrorContext.h"
#include "VMemory.h"
#include "VIntlMgr.h"
#include "VBlob.h"
#include "VFloat.h"
#include "VTime.h"
#include "VValue.h"
#include "VTextConverter.h"


VStream::VStream()
{
#if BIGENDIAN
	fNeedSwap = true;
#else
	fNeedSwap = false;
#endif

	fError = VE_OK;
	fPosition = 0;
	fProgress = NULL;
	fIsReading = false;
	fIsWriting = false;
	fIsWriteOnly = false;
	fIsReadOnly = false;
	fWasFlushed = false;
	fCharSet = VTC_DefaultTextExport;
	fCarriageReturnMode = eCRM_NATIVE;
	fFromUniConverter = NULL;
	fToUniConverter = NULL;
}


VStream::~VStream()
{
	if (fFromUniConverter != NULL)
		fFromUniConverter->Release();

	if (fToUniConverter != NULL)
		fToUniConverter->Release();

	if (fProgress != NULL)
		fProgress->Release();
}


VError VStream::OpenReading()
{
	if (fIsReading || fIsWriting)
		return vThrowError(VE_STREAM_ALREADY_OPENED);

	if (fIsWriteOnly)
		return vThrowError(VE_STREAM_CANNOT_READ);

	if (fToUniConverter == NULL)
		fToUniConverter = VTextConverters::Get()->RetainToUnicodeConverter(fCharSet);

	if (!fToUniConverter->IsValid())
		return vThrowError(VE_STREAM_NO_TEXT_CONVERTER);

	fPosition = 0;
	fError = DoOpenReading();
	if (fError == VE_OK)
	{
		fIsReading = true;
		if (fProgress != NULL)
		{
			VString s(L"Reading From Stream");
			/*
			fProgress->Start();
			fProgress->SetMaxValue(GetSize());
			*/
			fProgress->BeginSession(GetSize(), s);
		}
	}

	return fError;
}


VError VStream::OpenReading(OsType inSignature)
{
	OpenReading();

	if (fError == VE_OK)
	{
		sLONG	signature;
		GetLong(signature);

		if (signature != inSignature)
		{
			ByteSwapLong(&signature);
			if (signature == inSignature)
			{
				SetNeedSwap(true);
			}
		}

		if (signature != inSignature)
		{
			SetPos(0);
			return vThrowError(VE_STREAM_BAD_SIGNATURE);
		}
	}

	return fError;
}


VError VStream::OpenWriting()
{
	if (fIsReading || fIsWriting)
		return vThrowError(VE_STREAM_ALREADY_OPENED);

	if (fIsReadOnly)
		return vThrowError(VE_STREAM_CANNOT_WRITE);

	if (fFromUniConverter == NULL)
		fFromUniConverter = VTextConverters::Get()->RetainFromUnicodeConverter(fCharSet);

	if (!fFromUniConverter->IsValid())
		fError = vThrowError(VE_STREAM_NO_TEXT_CONVERTER);

	fPosition = 0;
	fError = DoOpenWriting();
	if (fError == VE_OK)
	{
		fIsWriting = true;
		if (fProgress != NULL)
		{
			VString s(L"Writing to Stream");
			/*
			fProgress->Start();
			fProgress->SetMaxValue(-1);
			*/
			fProgress->BeginSession(-1, s);
		}
	}

	return fError;
}


VError VStream::CloseReading()
{
	if (!fIsReading)
		return vThrowError(VE_STREAM_CANNOT_READ);

	// don't care the error in finishing reading
	DoCloseReading();

	if (fProgress != NULL)
		fProgress->EndSession();
	/*
	if (fProgress != NULL)
		fProgress->Stop();
		*/

	fIsReading = false;
	return fError;
}


VError VStream::CloseWriting( bool inSetSize)
{
	if (!fIsWriting)
		return vThrowError(VE_STREAM_CANNOT_WRITE);

	// flush (may modify fError)
	if (fError == VE_OK)
		fError = DoFlush();

	// don't care the error while relinquinshing ressources
	DoCloseWriting(inSetSize);

	if (fProgress != NULL)
		fProgress->EndSession();
	/*
	if (fProgress != NULL)
		fProgress->Stop();
		*/

	fIsWriting = false;
	return fError;
}


void VStream::SetNeedSwap( bool inNeedSwap)
{
	fNeedSwap = inNeedSwap;
}


void VStream::SetBigEndian()
{
#if BIGENDIAN
	fNeedSwap = false;
#else
	fNeedSwap = true;
#endif
}


void VStream::SetLittleEndian()
{
#if BIGENDIAN
	fNeedSwap = true;
#else
	fNeedSwap = false;
#endif
}


VError VStream::SetCharSet(CharSet inCharSet)
{
	if (fError == VE_OK) {
		ReleaseRefCountable(&fFromUniConverter);

		ReleaseRefCountable(&fToUniConverter);

		fCharSet = inCharSet;

		if (fIsReading)
		{
			if (fError == VE_OK)
			{
				fToUniConverter = VTextConverters::Get()->RetainToUnicodeConverter(fCharSet);
				if (!fToUniConverter->IsValid())
					fError = vThrowError(VE_STREAM_NO_TEXT_CONVERTER);
			}
		}

		if (fIsWriting)
		{
			if (fError == VE_OK)
			{
				fFromUniConverter = VTextConverters::Get()->RetainFromUnicodeConverter(fCharSet);
				if (!fFromUniConverter->IsValid())
					fError = vThrowError(VE_STREAM_NO_TEXT_CONVERTER);
			}
		}
	}

	return fError;
}

VError VStream::GuessCharSetFromLeadingBytes( CharSet inDefaultCharSet)
{
	CharSet charSet = inDefaultCharSet;

	VError err;
	VSize readBytes = 0;
	uBYTE c[4];
	c[0] = c[1] = c[2] = c[3] = 0;
	{
		StErrorContextInstaller errorContext( false);	// no error
		err = GetData( &c[0], 4, &readBytes);
	}

	if ( (err == VE_OK) || (err == VE_STREAM_EOF) )
	{
		// reset error if we read to much data, otherwise SetCharSet won't work
		if ( err == VE_STREAM_EOF )
			fError = VE_OK;

		if (c[0] == 0xef && c[1] == 0xbb && c[2] ==0xbf)	// utf-8 bom
		{
			charSet = VTC_UTF_8;
			if (readBytes > 3)
				UngetData( &c[3], readBytes - 3);
		}
		else if ( (readBytes == 4) && (c[0] == 0 && c[1] == 0 && c[2] == 0xfe && c[3] == 0xff) )	// utf-32 bom
		{
			charSet = VTC_UTF_32_BIGENDIAN;
		}
		else if ( (readBytes == 4) && (c[0] == 0xff && c[1] == 0xfe && c[2] == 0 && c[3] == 0) )	// utf-32 bom
		{
			charSet = VTC_UTF_32_SMALLENDIAN;
		}
		else if (c[0] == 0xfe && c[1] == 0xff)	// utf-16 bom
		{
			charSet = VTC_UTF_16_BIGENDIAN;
			if (readBytes > 2)
				UngetData( &c[2], readBytes - 2);
		}
		else if (c[0] == 0xff && c[1] == 0xfe)	// utf-16 bom
		{
			charSet = VTC_UTF_16_SMALLENDIAN;
			if (readBytes > 2)
				UngetData( &c[2], readBytes - 2);
		}
		else
		{
			// no bom: try to guess at looking at first bytes
			if ( (readBytes == 4) && (c[0] == 0) && (c[1] != 0) && (c[2] == 0) && (c[3] != 0) )	// guess utf-16
			{
				charSet = VTC_UTF_16_BIGENDIAN;
			}
			else if ( (readBytes == 4) && (c[0] != 0) && (c[1] == 0) && (c[2] != 0) && (c[3] == 0) )	// guess utf-16
			{
				charSet = VTC_UTF_16_SMALLENDIAN;
			}
			else if ( (readBytes == 4) && (charSet == VTC_UTF_16_BIGENDIAN || charSet == VTC_UTF_16_SMALLENDIAN) && (c[0] != 0 && c[0] < 0x80) && (c[1] != 0 && c[1] < 0x80) && (c[2] != 0 && c[2] < 0x80) && (c[3] != 0 && c[3] < 0x80) )
			{
				// looks like plain us-ascii
				charSet = VTC_UTF_8;
			}
			UngetData( &c[0], readBytes);
		}
	}

	return SetCharSet( charSet );
}


VError VStream::WriteBOM()
{
	switch( fCharSet)
	{
		case VTC_UTF_8:
			Put<uBYTE>( 0xef);
			Put<uBYTE>( 0xbb);
			Put<uBYTE>( 0xbf);
			break;

		case VTC_UTF_16_BIGENDIAN:
			Put<uBYTE>( 0xfe);
			Put<uBYTE>( 0xff);
			break;

		case VTC_UTF_16_SMALLENDIAN:
			Put<uBYTE>( 0xff);
			Put<uBYTE>( 0xfe);
			break;

		case VTC_UTF_32_BIGENDIAN:
			Put<uBYTE>( 0);
			Put<uBYTE>( 0);
			Put<uBYTE>( 0xfe);
			Put<uBYTE>( 0xff);
			break;

		case VTC_UTF_32_SMALLENDIAN:
			Put<uBYTE>( 0xff);
			Put<uBYTE>( 0xfe);
			Put<uBYTE>( 0);
			Put<uBYTE>( 0);
			break;
		default:
			break;
	}
	return fError;
}


VError VStream::Flush()
{
	if (!fIsWriting)
		return vThrowError(VE_STREAM_CANNOT_WRITE);

	fError = DoFlush();
	return fError;
}


VError VStream::PutContentInto(VStream* intoStream)
{
	VError err = OpenReading();
	if (err == VE_OK)
	{
		SetPos(0);
		sLONG8 size = GetSize();
		char buffer[4096];
		sLONG8 nbPage = (size+4095) / 4096;
		for (sLONG8 i = 0; i < nbPage && err == VE_OK; ++i)
		{
			sLONG8 count = 4096;
			if (i == nbPage-1)
			{
				count = size % 4096;
				if (count == 0)
					count = 4096;
			}
			VSize outCount;
			err = GetData(&buffer[0], count, &outCount);
			if (err == VE_OK)
			{
				err = intoStream->PutData(&buffer[0], count);
			}
		}
		CloseReading();
	}

	return err;
}


VError VStream::PutData(const void* inBuffer, VSize inNbBytes)
{
	if (fError == VE_OK)
	{
		if (!fIsWriting)
			fError = vThrowError(VE_STREAM_CANNOT_WRITE);
		else
		{
			fError = DoPutData(inBuffer, inNbBytes);
			if (fError == VE_OK)
			{
				fPosition += inNbBytes;
				if (fProgress != NULL && !fProgress->Progress(inNbBytes))
					fError = vThrowError(VE_STREAM_USER_ABORTED);
			}
		}
	}
	return fError;
}


VError VStream::GetData(void* inBuffer, VSize inNbBytes, VSize* outNbBytesCopied)
{
	if (fError == VE_OK)
	{
		if (!fIsReading)
			fError = vThrowError(VE_STREAM_CANNOT_READ);
		else
		{
			VSize nbBytesCopied = inNbBytes;
			fError = DoGetData(inBuffer, &nbBytesCopied);
			fPosition += nbBytesCopied;

			if (outNbBytesCopied != NULL)
				*outNbBytesCopied = nbBytesCopied;

			// whatever happens, call the progress interface so that it can update itself
			if (fProgress != NULL && (inNbBytes > 0) && !fProgress->Progress(inNbBytes))
			{
				if (fError == VE_OK)
					fError = vThrowError(VE_STREAM_USER_ABORTED);
			}
		}
	}
	return fError;
}


VError VStream::UngetData(const void* inBuffer, VSize inNbBytes)
{
	if ((fError == VE_OK || fError == VE_STREAM_EOF) && (inNbBytes > 0))
	{
		if (!fIsReading)
			fError = vThrowError(VE_STREAM_CANNOT_READ);
		else
		{
			//Cast pour Èviter le warning C4018
			if (((sLONG8)  inNbBytes) > fPosition)
				fError = vThrowError(VE_STREAM_TOO_MANY_UNGET_DATA);
			else
			{
				fError = DoUngetData(inBuffer, inNbBytes);

				if (fError == VE_OK)
					fPosition -= inNbBytes;
			}
		}
	}
	return fError;
}


VError VStream::DoUngetData(const void* /*inBuffer*/, VSize /*inNbBytes*/)
{
	return VE_OK;
}


VError VStream::SetSize(sLONG8 inNewSize)
{
	if (fError == VE_OK) {
		if (!fIsWriting) {
			fError = vThrowError(VE_STREAM_CANNOT_WRITE);
		} else {

			fError = DoSetSize(inNewSize);

			// check the current position against new limits
			if (fError == VE_OK && fPosition > inNewSize)
				fPosition = inNewSize;
		}
	}

	return fError;
}


VError VStream::SetPosByOffset(sLONG8 inOffset)
{
	if (fError == VE_OK) {
		if (!fIsWriting && !fIsReading) {
			fError = vThrowError(VE_STREAM_NOT_OPENED);
		} else {

			bool isOK = true;
			if (inOffset < 0)
				isOK = -inOffset <= fPosition;

			if (!isOK)
				fError = vThrowError(VE_INVALID_PARAMETER);
			else {
				//sLONG8 newPosition = (inOffset < 0) ? (fPosition - (uLONG) -inOffset) : (fPosition + (uLONG) inOffset);
				sLONG8 newPosition = fPosition + inOffset;
				fError = DoSetPos(newPosition);
				if (fError == VE_OK)
					fPosition = newPosition;
			}
		}
	}

	return fError;
}


VError VStream::SetPos(sLONG8 inNewPos)
{
	if (fError == VE_OK) {
		if (!fIsWriting && !fIsReading) {
			fError = vThrowError(VE_STREAM_NOT_OPENED);
		} else {
			fError = DoSetPos(inNewPos);
			if (fError == VE_OK)
				fPosition = inNewPos;
		}
	}
	return fError;
}


sLONG8 VStream::GetSize()
{
	return DoGetSize();
}


VError VStream::DoOpenReading()
{
	return VE_OK;
}


VError VStream::DoOpenWriting()
{
	return VE_OK;
}


VError VStream::DoCloseReading()
{
	return VE_OK;
}


VError VStream::DoCloseWriting(Boolean /*inSetSize*/)
{
	return VE_OK;
}


VError VStream::DoSetPos(sLONG8 /*inNewPos*/)
{
	return VE_OK;
}

VError VStream::DoFlush()
{
	return VE_OK;
}


VError VStream::SetReadOnly( bool inReadOnly)
{
	fIsReadOnly = inReadOnly;
	return VE_OK;
}


VError VStream::SetWriteOnly( bool inWriteOnly)
{
	fIsWriteOnly = inWriteOnly;
	return VE_OK;
}


VError VStream::GetWords(sWORD* outValue, sLONG* ioCount)
{
	VSize count = 0;
	GetData(outValue, *ioCount * sizeof(*outValue), &count);
	*ioCount = (sLONG) (count/sizeof(*outValue));
	if (fNeedSwap)
		ByteSwapWordArray(outValue, *ioCount);
	return fError;
}


VError VStream::GetLongs(sLONG* outValue, sLONG* ioCount)
{
	VSize count = 0;
	GetData(outValue, *ioCount * sizeof(*outValue), &count);
	*ioCount = (sLONG) (count/sizeof(*outValue));
	if (fNeedSwap)
		ByteSwapLongArray(outValue, *ioCount);
	return fError;
}


VError VStream::GetLong8s(sLONG8* outValue, sLONG* ioCount)
{
	VSize count = 0;
	GetData(outValue, *ioCount * sizeof(*outValue), &count);
	*ioCount = (sLONG) (count/sizeof(*outValue));
	if (fNeedSwap)
		ByteSwapLong8Array(outValue, *ioCount);
	return fError;
}


VError VStream::GetReals(Real* outValue, sLONG* ioCount)
{
	VSize count = 0;
	GetData(outValue, *ioCount * sizeof(*outValue), &count);
	*ioCount = (sLONG) (count/sizeof(*outValue));
	if (fNeedSwap)
		ByteSwapReal8Array(outValue, *ioCount);
	return fError;
}


#if WITH_VMemoryMgr
VError VStream::GetHandle(VHandle& outValue)
{
	outValue = NULL;

	sLONG len;
	if (GetLong(len) == VE_OK)
	{
		outValue = VMemory::NewHandle(len);
		if (outValue != NULL)
		{
			VPtr data = VMemory::LockHandle(outValue);
			GetData(data, len);
			VMemory::UnlockHandle(outValue);

			if (fError != VE_OK)
			{
				VMemory::DisposeHandle(outValue);
				outValue = NULL;
			}
		}
	}

	return fError;
}
#endif

VError VStream::PutValue (const VValue& inValue, bool inWithKind)
{
	if (inWithKind)
	{
		ValueKind kind = inValue.GetTrueValueKind();
		PutWord( kind);
	}
	return inValue.WriteToStream( this, 0);
}


VValue* VStream::GetValue()
{
	VValue* val = NULL;

	ValueKind kind;
	if (GetWord( kind) == VE_OK)
	{
		val = VValue::NewValueFromValueKind( kind);
		if (val != NULL)
			val->ReadFromStream( this, 0);
	}
	return val;
}


VError VStream::PutHexBytes(const sBYTE* inValue, VSize inCount)
{
	static sBYTE hexaTable[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

	while (inCount--)
	{
		sBYTE hc = *inValue++;
		PutData(&(hexaTable[ (hc & 0xF0) >> 4 ]), 1);
		PutData(&(hexaTable[ hc & 0x0F ]), 1);
	}
	return fError;
}


VError VStream::PutWords( const sWORD* inValue, sLONG inCount)
{
	if (fNeedSwap)
	{
		// need a copy not to touch the original array (pbs in preemptiv threading)
		VPtr data = VMemory::NewPtr(inCount *sizeof(*inValue), 'stm ');
		if (data == NULL)
		{
			fError = vThrowError(VE_MEMORY_FULL);
		}
		else
		{
			::CopyBlock(inValue, data, inCount *sizeof(*inValue));
			::ByteSwapWordArray((sWORD*) data, inCount);
			PutData(data, inCount *sizeof(*inValue));
			VMemory::DisposePtr(data);
		}
	} else
		PutData(inValue, inCount *sizeof(*inValue));

	return fError;
}


VError VStream::PutLongs(const sLONG* inValue, sLONG inCount)
{
	if (fNeedSwap)
	{
		// need a copy not to touch the original array (pbs in preemptiv threading)
		VPtr data = VMemory::NewPtr(inCount *sizeof(*inValue), 'stm ');
		if (data == NULL)
		{
			fError = vThrowError(VE_MEMORY_FULL);
		}
		else
		{
			::CopyBlock(inValue, data, inCount *sizeof(*inValue));
			::ByteSwapLongArray((sLONG*) data, inCount);
			PutData(data, inCount *sizeof(*inValue));
			VMemory::DisposePtr(data);
		}
	} else
		PutData(inValue, inCount *sizeof(*inValue));

	return fError;
}


VError VStream::PutLong8s(const sLONG8* inValue, sLONG inCount)
{
	if (fNeedSwap) {
		// need a copy not to touch the original array (pbs in preemptiv threading)
		VPtr data = VMemory::NewPtr(inCount *sizeof(*inValue), 'stm ');
		if (data == NULL)
		{
			fError = vThrowError(VE_MEMORY_FULL);
		}
		else
		{
			::CopyBlock(inValue, data, inCount *sizeof(*inValue));
			::ByteSwapLong8Array((sLONG8 *) data, inCount);
			PutData(data, inCount *sizeof(*inValue));
			VMemory::DisposePtr(data);
		}
	} else
		PutData(inValue, inCount *sizeof(*inValue));

	return fError;
}


VError VStream::PutReals(const Real* inValue, sLONG inCount)
{
	if (fNeedSwap)
	{
		// need a copy not to touch the original array (pbs in preemptiv threading)
		VPtr data = VMemory::NewPtr(inCount *sizeof(*inValue), 'stm ');
		if (data == NULL)
		{
			fError = vThrowError(VE_MEMORY_FULL);
		}
		else
		{
			::CopyBlock(inValue, data, inCount *sizeof(*inValue));
			::ByteSwapReal8Array((Real*) data, inCount);
			PutData(data, inCount *sizeof(*inValue));
			VMemory::DisposePtr(data);
		}
	} else
		PutData(inValue, inCount *sizeof(*inValue));

	return fError;
}

#if WITH_VMemoryMgr

VError VStream::PutHandle(VHandle inValue)
{
	VSize len = VMemory::GetHandleSize(inValue);

	PutLong( (sLONG) len);
	if (len > 0)
	{
		PutData(VMemory::LockHandle(inValue), len);
		VMemory::UnlockHandle(inValue);
	}

	return fError;
}

#endif


VError VStream::GetText(VString& outText)
{
	if (fError == VE_OK) {

		VSize bytesInBuffer = 0;
		{
			StErrorContextInstaller filter;

			char buffer[4096L];

			do {
				VSize nbBytesCopied = 0;
				VSize remainingBytes = (VSize) ((sLONG8) GetSize() - GetPos());
				GetData(buffer + bytesInBuffer, Min<VSize>( remainingBytes, (VSize)( sizeof(buffer) - bytesInBuffer ) ), &nbBytesCopied);
				bytesInBuffer += nbBytesCopied;

				VSize consumedBytes;
				bool isOK = fToUniConverter->ConvertString( buffer, bytesInBuffer, &consumedBytes, outText);

				if (!isOK || consumedBytes == 0)
					break;

				// conversion may have stopped in the middle of a char.
				// move remaining unprocessed bytes at the beginning of the buffer.
				::memmove(buffer, buffer + consumedBytes, bytesInBuffer - consumedBytes);
				bytesInBuffer -= consumedBytes;

			} while( ( fError == VE_OK ) && ( GetPos() < GetSize() ) );
		}

		if (bytesInBuffer != 0 && (fError == VE_OK))
			fError = vThrowError(VE_STREAM_TEXT_CONVERSION_FAILURE, "unterminated unicode conversion.");

	}

	return fError;
}


VError VStream::GetText( VString& outText, VIndex inNbChars, bool inLoosyMode)
{
	if ((fError == VE_OK) && (inNbChars > 0))
	{
		VIndex oldLength = outText.GetLength();
		UniChar *p = outText.GetCPointerForWrite( oldLength + inNbChars);
		if (p == NULL)
		{
			fError = vThrowError( VE_MEMORY_FULL);
		}
		else
		{
			if (fCharSet == VTC_UTF_16_BIGENDIAN || fCharSet == VTC_UTF_16_SMALLENDIAN)
			{
				// trivial case (no conversion required)

				VSize nbBytesCopied = 0;
				GetData( p + oldLength, inNbChars * sizeof(UniChar), &nbBytesCopied);

				sLONG readChars = (sLONG) (nbBytesCopied/sizeof(UniChar));

				if (readChars * sizeof(UniChar) != nbBytesCopied)
				{

					// put back extra byte
					VSize extraBytes = nbBytesCopied - readChars * sizeof(UniChar);
					UngetData(outText.GetCPointer() + oldLength + readChars, extraBytes);
				}

				// need to swap ?
				if (fCharSet != VTC_UTF_16)
					ByteSwapWordArray((sWORD*) (outText.GetCPointer() + oldLength), readChars);

				outText.Validate(oldLength + readChars);
			}
			else
			{
				// needs to convert the text before counting characters.
				// assumes that chars are roughly 1 byte each.

				StErrorContextInstaller filter(VE_STREAM_EOF, VE_OK);

				char buffer[32L];
				VSize bytesInBuffer = 0;
				VIndex readChars = 0;

				do {
					VSize	estimatedNeededBytes = (inNbChars - readChars) * sizeof(UniChar) + 10;
					VSize	toReadBytes = Min(estimatedNeededBytes, (VSize) sizeof(buffer) - bytesInBuffer);

					VSize	nbBytesCopied = 0;
					GetData(buffer + bytesInBuffer, toReadBytes, &nbBytesCopied);
					bytesInBuffer += nbBytesCopied;

					if (fError == VE_OK || fError == VE_STREAM_EOF)
					{
						VSize consumedBytes;
						VIndex lengthBeforeConvert = outText.GetLength();
						bool isOK = fToUniConverter->ConvertString(buffer, bytesInBuffer, &consumedBytes, outText);
						readChars = outText.GetLength() - oldLength;

						if (!isOK || readChars > inNbChars)
						{

							// EOF is not really an error in this case
							if (fError == VE_STREAM_EOF)
								fError = VE_OK;

							// just read too many chars
							if (inLoosyMode && isOK)
							{
								// the user don't care we got more characters than requested
								// just unget unused bytes
								UngetData(buffer + consumedBytes, bytesInBuffer - consumedBytes);
								bytesInBuffer -= consumedBytes;

							}
							else
							{
								// we got a conversion error or the user wants the exact number of characters
								// so one must retry the buffer byte by byte
								outText.Truncate(lengthBeforeConvert);
								VSize convertedBytes = 0;
								do {

									consumedBytes = 0;

									for (sLONG nbBytes = 1 ; convertedBytes + nbBytes <= bytesInBuffer ; ++nbBytes)
									{
										isOK = fToUniConverter->ConvertString(buffer + convertedBytes, nbBytes, &consumedBytes, outText);
										if (!isOK || consumedBytes != 0)
											break;
									}

									convertedBytes += consumedBytes;

									if (!isOK || consumedBytes == 0)
										break;

									readChars = outText.GetLength() - oldLength;
								} while(readChars < inNbChars);

								// now unget unused bytes
								UngetData(buffer + convertedBytes, bytesInBuffer - convertedBytes);
							}

						}
						else if (readChars < inNbChars)
						{
							// need to get and convert more data
							::memmove(buffer, buffer + consumedBytes, bytesInBuffer - consumedBytes);
							bytesInBuffer -= consumedBytes;

						}
						else if (readChars == inNbChars)
						{
							// we got what we were asked.
							// unget unused bytes.
							UngetData(buffer + consumedBytes, bytesInBuffer - consumedBytes);
							bytesInBuffer -= consumedBytes;
						}
					}

				} while((fError == VE_OK) && (readChars < inNbChars));
			}
		}
	}

	return fError;
}


VError VStream::GetTextLine(VString& outText, bool inAppendCarriageReturn, UniChar* delimiter)
{
	outText.Clear();
	if (fError == VE_OK)
	{
		StErrorContextInstaller filter(VE_STREAM_EOF, VE_OK);

		// Assumes that with any charset except utf16, CR is '\r' and LF is '\n' !!
		if (fCharSet == VTC_UTF_16_BIGENDIAN || fCharSet == VTC_UTF_16_SMALLENDIAN)
		{
			UniChar uniCR = '\r';
			UniChar uniLF = '\n';
			if (fCharSet != VTC_UTF_16)
			{
				ByteSwapWord(&uniCR);
				ByteSwapWord(&uniLF);
			}

			UniChar delimiter1 = uniCR;
			UniChar delimiter2 = uniLF;
			bool withSecondDelimiter = true;

			if (delimiter != NULL)
			{
				UniChar c = *delimiter;
				if (fCharSet != VTC_UTF_16)
					ByteSwapWord(&c);
				delimiter1 = c;
				delimiter2 = 0;
				withSecondDelimiter = false;

			}

			UniChar	uniBuffer[256L];

			bool gotLine = false;
			bool gotSomething = false;
			do {
				VSize	nbBytesCopied = 0;
				GetData(uniBuffer, sizeof(uniBuffer), &nbBytesCopied);

				if (fError != VE_OK && fError != VE_STREAM_EOF)
					break;

				if (!gotSomething && (nbBytesCopied > 0))
					gotSomething = true;

				// look for CR, CRLF or LF
				VIndex	nbChars = (VIndex) (nbBytesCopied / sizeof(UniChar));
				for (VIndex index = 0 ; index < nbChars ; ++index)
				{
					if (uniBuffer[index] == delimiter1)
					{
						// see if next one is LF
						if (withSecondDelimiter && index == nbChars - 1)
						{
							// that was the last buffered char -> needs to pull one more char
							UniChar extra;
							nbBytesCopied=0;
							GetData(&extra, sizeof(UniChar), &nbBytesCopied);
							if (nbBytesCopied == sizeof(UniChar))
							{
								if (extra != delimiter2)
									UngetData(&extra, sizeof(UniChar));
							}
						}
						else
						{
							if (withSecondDelimiter && uniBuffer[index + 1] == delimiter2)
							{
								// unget unused bytes and skip CR + LF
								UngetData(uniBuffer + index + 2, nbBytesCopied - (index + 2) * sizeof(UniChar));
							}
							else
							{
								// unget unused bytes and skip CR
								UngetData(uniBuffer + index + 1, nbBytesCopied - (index + 1) * sizeof(UniChar));
							}
						}

						if (inAppendCarriageReturn)
							outText.AppendBlock(uniBuffer, (index + 1) * sizeof(UniChar), fCharSet);
						else
							outText.AppendBlock(uniBuffer, index * sizeof(UniChar), fCharSet);

						gotLine = true;
						break;

					}
					else if (uniBuffer[index] == uniLF)
					{
						// unget unused bytes and skip LF
						UngetData(uniBuffer + index + 1, nbBytesCopied - (index + 1) * sizeof(UniChar));
						if (inAppendCarriageReturn)
						{
							uniBuffer[index] = uniCR;
							outText.AppendBlock(uniBuffer, (index + 1) * sizeof(UniChar), fCharSet);
						}
						else
						{
							outText.AppendBlock(uniBuffer, index * sizeof(UniChar), fCharSet);
						}
						gotLine = true;
						break;
					}
				}

				if (fError == VE_STREAM_EOF && !gotLine)
				{
					outText.AppendBlock(uniBuffer, nbBytesCopied, fCharSet);
					if (inAppendCarriageReturn)
						outText.AppendUniChar('\r');

					gotLine = true;
				}

			} while(!gotLine);

			// EOF is not really an error in this case
			if (fError == VE_STREAM_EOF && gotSomething)
				fError = VE_OK;
		}
		else
		{
			char delimiter1 = '\r';
			char delimiter2 = '\n';
			bool withSecondDelimiter = true;

			if (delimiter != NULL)
			{
				UniChar c = *delimiter; // penser a tester si c > 127 et a le decomposer en UTF8
				delimiter1 = c;
				delimiter2 = 0;
				withSecondDelimiter = false;

			}

			//	same thing but with single bytes
			char buffer[256L];

			bool gotLine = false;
			bool gotSomething = false;
			do {
				VSize nbChars = 0;
				GetData(buffer, sizeof(buffer), &nbChars);

				if (fError != VE_OK && fError != VE_STREAM_EOF)
					break;

				if (!gotSomething && (nbChars > 0))
					gotSomething = true;

				// look for CR, CRLF or LF
				for (VSize index = 0 ; index < nbChars ; ++index)
				{
					if (buffer[index] == delimiter1)
					{
						// see if next one is LF
						if (withSecondDelimiter && index == nbChars - 1)
						{
							// that was the last buffered char -> needs to pull one more char
							char extra;
							VSize nbBytesCopied=0;
							GetData(&extra, sizeof(char), &nbBytesCopied);
							if (nbBytesCopied == sizeof(char))
							{
								if (extra != delimiter2)
									UngetData(&extra, sizeof(char));
							}
						}
						else
						{
							if (withSecondDelimiter && buffer[index + 1] == delimiter2)
							{
								// unget unused bytes and skip CR + LF
								UngetData(buffer + index + 2, nbChars - (index + 2));
							}
							else
							{
								// unget unused bytes and skip CR
								UngetData(buffer + index + 1, nbChars - (index + 1));
							}
						}
						if (inAppendCarriageReturn)
							outText.AppendBlock(buffer, (index + 1), fCharSet);
						else
							outText.AppendBlock(buffer, index, fCharSet);
						gotLine = true;
						break;

					}
					else if (withSecondDelimiter && buffer[index] == delimiter2)
					{
						// unget unused bytes and skip LF
						UngetData(buffer + index + 1, nbChars - (index + 1));
						if (inAppendCarriageReturn)
						{
							buffer[index] = '\r';
							outText.AppendBlock(buffer, (index + 1), fCharSet);
						}
						else
						{
							outText.AppendBlock(buffer, index, fCharSet);
						}
						gotLine = true;
						break;
					}
				}

				if (fError == VE_STREAM_EOF && !gotLine)
				{
					outText.AppendBlock(buffer, nbChars, fCharSet);
					if (inAppendCarriageReturn)
						outText.AppendUniChar('\r');

					gotLine = true;
				}

			} while(!gotLine);

			// EOF is not really an error in this case
			if (fError == VE_STREAM_EOF && gotSomething)
				fError = VE_OK;
		}
	}

	return fError;
}

VError VStream::GetTTRLine(VArrayString& outArrayText)
{
	outArrayText.Clear();

	VString	string;
	VError	error = GetTextLine(string, false);

	if (VE_OK == error || VE_STREAM_EOF == error)
	{
		VString	lineString;

		for (VIndex index = 0; index < string.GetLength(); index++)
		{
			UniChar	curChar = string[index];
			if (curChar == 9)
			{
				outArrayText.AppendString(lineString);
				lineString.Clear();
			}
			else
			{
				lineString.AppendUniChar(curChar);
			}
		}

		outArrayText.AppendString(lineString);
	}

	return error;
}

VError	VStream::PutVPrintf(const char* inMessage, va_list argList)
{
	VStr255 text;

	text.VPrintf(inMessage, argList);

	return PutText(text);
}

VError VStream::PutPrintf(const char* inMessage, ...)
{
	VStr255 text;

	va_list argList;
	va_start(argList, inMessage);
	text.VPrintf(inMessage, argList);
	va_end(argList);

	return PutText(text);
}


VError VStream::PutText(const VString& inText)
{
	VStringConvertBuffer buffer(fCharSet, fCarriageReturnMode);

	buffer.ConvertString(inText);

	return PutData(buffer.GetCPointer(), buffer.GetSize());
}


#pragma mark-

#if WITH_VMemoryMgr
VHandleStream::VHandleStream( VHandleStream* inHandleStream )
{
	fLogSize = inHandleStream->fLogSize;

	fHandle = VMemory::NewHandle( (VSize) fLogSize );
	if ( NULL != fHandle && fLogSize > 0 )
	{
		VMemory::CopyBlock( VMemory::LockHandle( inHandleStream->fHandle ), VMemory::LockHandle( fHandle ), (VSize) fLogSize );
		VMemory::UnlockHandle( inHandleStream->fHandle );
		VMemory::UnlockHandle( fHandle );
	}

	fPhysSize = fLogSize;
	fData = NULL;
}

VHandleStream::VHandleStream(VHandle* inHandle)
{
	if (inHandle) {
		if (*inHandle == NULL)
			*inHandle = VMemory::NewHandle(0);

		fHandle = *inHandle;
		fOwnHandle = false;
	} else {
		fHandle = VMemory::NewHandle(0);
		fOwnHandle = true;
	}

	fData = NULL;
	if (fHandle != NULL) {
		fLogSize = VMemory::GetHandleSize(fHandle);
		fPhysSize = fLogSize;
	}
}


VHandleStream::~VHandleStream()
{
	if (fOwnHandle) {
		VMemory::DisposeHandle(fHandle);
		fHandle = NULL;
	}
}


VError VHandleStream::DoOpenWriting()
{
	if (fHandle == NULL)
		return vThrowError(VE_STREAM_CANNOT_FIND_SOURCE);

	if (VMemory::IsLocked(fHandle))
		return vThrowError(VE_STREAM_CANNOT_WRITE);

	fData = VMemory::LockHandle(fHandle);

	fLogSize = VMemory::GetHandleSize(fHandle);
	fPhysSize = fLogSize;
	return VE_OK;
}


void* VHandleStream::GetDataPtr()
{
	void* result = NULL;

	if (fHandle == NULL || fData == NULL)
	{
		vThrowError(VE_STREAM_CANNOT_FIND_SOURCE);
	}
	else
	{
		result = (void*)fData;
	}

	return result;
}


VError VHandleStream::DoOpenReading()
{
	if (fHandle == NULL)
		return vThrowError(VE_STREAM_CANNOT_FIND_SOURCE);

	fData = VMemory::LockHandle(fHandle);

	fLogSize = VMemory::GetHandleSize(fHandle);
	fPhysSize = fLogSize;
	return VE_OK;
}


VError VHandleStream::DoCloseReading()
{
	VMemory::UnlockHandle(fHandle);
	fData = NULL;
	fLogSize = 0;
	fPhysSize = 0;
	return VE_OK;
}


VError VHandleStream::DoCloseWriting(Boolean inSetSize)
{
	VMemory::UnlockHandle(fHandle);
	if (inSetSize)
		VMemory::SetHandleSize(fHandle, (VSize) fLogSize); // can't fail because fLogSize<fPhysSize

	fData = NULL;
	fLogSize = 0;
	fPhysSize = 0;

	return VE_OK;
}


VError VHandleStream::DoSetSize(sLONG8 inNewSize)
{
	VError err = VE_OK;
	VMemory::UnlockHandle(fHandle);
	if (VMemory::SetHandleSize(fHandle, (VSize)inNewSize)) {
		fPhysSize = VMemory::GetHandleSize(fHandle);
		fLogSize = inNewSize;
		fData = VMemory::LockHandle(fHandle);
	} else {
		err = vThrowError(VE_MEMORY_FULL);
	}

	return err;
}


VError VHandleStream::DoPutData(const void* inBuffer, VSize inNbBytes)
{
	VError err = VE_OK;
	sLONG8 pos = GetPos();
	//Cast pour Èviter le warning C4018
	if (((sLONG8)(pos + inNbBytes)) > fPhysSize) {
		VMemory::UnlockHandle(fHandle);
		if (!VMemory::SetHandleSize(fHandle, (VSize) (pos + inNbBytes + 1024L)))
			err = vThrowError(VE_MEMORY_FULL);
		fPhysSize = VMemory::GetHandleSize(fHandle);
		fData = VMemory::LockHandle(fHandle);
	}

	if (err == VE_OK) {
		::CopyBlock(inBuffer, fData + pos, inNbBytes);
		//Cast pour Èviter le warning C4018
		if (fLogSize < pos + ((sLONG8)inNbBytes))
			fLogSize = pos + inNbBytes;
	}

	return err;
}


VError VHandleStream::DoGetData(void* inBuffer, VSize* ioCount)
{
	VError err = VE_OK;
	sLONG8 pos = GetPos();
	if (testAssert(pos <= fLogSize))	// caller should have checked this already
	{
		//Cast pour Èviter le warning C4018
		if (pos + ((sLONG8)*ioCount) > fLogSize)
		{
			err = vThrowError(VE_STREAM_EOF);
			*ioCount = (VSize)(fLogSize - pos);
		}
		::CopyBlock(fData + pos, inBuffer, *ioCount);
	}
	return err;
}


sLONG8 VHandleStream::DoGetSize()
{
	return fLogSize;
}


VError VHandleStream::DoSetPos(sLONG8 inNewPos)
{
	VError err = VE_OK;

	if (IsWriting() && (inNewPos > fPhysSize)) {
		VMemory::UnlockHandle(fHandle);
		if (!VMemory::SetHandleSize(fHandle, (VSize)(inNewPos + 1024L)))
			err = vThrowError(VE_MEMORY_FULL);
		fPhysSize = VMemory::GetHandleSize(fHandle);
		fData = VMemory::LockHandle(fHandle);
	}

	if (fLogSize < inNewPos)
		fLogSize = inNewPos;

	return err;
}
#endif


#pragma mark-

VError VConstPtrStream::DoOpenWriting()
{
	return vThrowError(VE_STREAM_CANNOT_WRITE);
}


VError VConstPtrStream::DoGetData(void* inBuffer, VSize* ioCount)
{
	VError err = VE_OK;
	VSize pos = (VSize) GetPos();
	if (testAssert(pos <= fSize))
	{
		if (pos + *ioCount > fSize)
		{
			err = vThrowError(VE_STREAM_EOF);
			*ioCount = fSize - pos;
		}
		::memcpy( inBuffer, (const char *) fData + pos, *ioCount);
	}
	return err;
}


VError VConstPtrStream::DoSetPos(sLONG8 inNewPos)
{
	//Cast pour Èviter le warning C4018
	if (inNewPos > ((sLONG8)fSize))
		return vThrowError(VE_STREAM_EOF);
	return VE_OK;
}



sLONG8 VConstPtrStream::DoGetSize()
{
	return fSize;
}


VError VConstPtrStream::DoPutData(const void* /*inBuffer*/, VSize /*inNbBytes*/)
{
	return vThrowError(VE_STREAM_CANNOT_WRITE);
}


VError VConstPtrStream::DoSetSize(sLONG8 /*inNewSize*/)
{
	return vThrowError(VE_STREAM_CANNOT_WRITE);
}


#pragma mark-



VError VPtrStream::DoPutData( const void* inBuffer, VSize inNbBytes)
{
	bool ok = fBuffer.PutDataAmortized( (VSize) GetPos(), inBuffer, inNbBytes);

	if (ok)
		return VE_OK;
	else
		return VE_MEMORY_FULL;
}


VError VPtrStream::DoGetData( void* inBuffer, VSize* ioCount)
{
	bool ok = fBuffer.GetData( (VSize) GetPos(), inBuffer, *ioCount, ioCount);

	if (ok)
		return VE_OK;
	else
		return vThrowError(VE_STREAM_EOF);
}


VError VPtrStream::DoSetPos( sLONG8 inNewPos)
{
	VError err = VE_OK;
	//Cast pour Èviter le warning C4018
	if (inNewPos > ((sLONG8)fBuffer.GetDataSize()))
		err = DoSetSize( inNewPos);
	return err;
}


sLONG8 VPtrStream::DoGetSize()
{
	return fBuffer.GetDataSize();
}


VError VPtrStream::DoSetSize( sLONG8 inNewSize)
{
	if ( (inNewSize <= kMAX_VSize) && fBuffer.SetSize( (VSize) inNewSize))
		return VE_OK;
	else
		return VE_MEMORY_FULL;
}


#pragma mark-

VBlobStream::VBlobStream(VBlob* inBlob)
{
	if (inBlob != NULL) {
		fBlob = inBlob;
		fIsOwner = false;
	} else {
		fBlob = new VBlobWithPtr;
		fIsOwner = true;
	}
}


VBlobStream::~VBlobStream()
{
	if (fIsOwner)
		delete fBlob;
}


VError VBlobStream::DoPutData(const void* inBuffer, VSize inNbBytes)
{
	return fBlob->PutData(inBuffer, inNbBytes, (VIndex)GetPos());
}


VError VBlobStream::DoGetData(void* inBuffer, VSize* ioNbBytes)
{
	return fBlob->GetData(inBuffer, *ioNbBytes, (VIndex)GetPos(), ioNbBytes);
}


VError VBlobStream::DoSetPos(sLONG8 inNewPos)
{
	VError err = VE_OK;
	if (inNewPos > (sLONG8)fBlob->GetSize())
		err = fBlob->SetSize((VSize)inNewPos);
	return err;
}


sLONG8 VBlobStream::DoGetSize()
{
	return fBlob->GetSize();
}


VError VBlobStream::DoSetSize(sLONG8 inNewSize)
{
	return fBlob->SetSize((VSize)inNewSize);
}

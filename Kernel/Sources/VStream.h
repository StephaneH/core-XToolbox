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
#ifndef __VStream__
#define __VStream__

#include "Kernel/Sources/VObject.h"
#include "Kernel/Sources/VString.h"
#include "Kernel/Sources/VTime.h"
#include "Kernel/Sources/VUUID.h"
#include "Kernel/Sources/VByteSwap.h"
#include "Kernel/Sources/VProgressIndicator.h"	// Included to avoid extra requirements when including VStream.h
#include "Kernel/Sources/VMemoryBuffer.h"


BEGIN_TOOLBOX_NAMESPACE

// Needed declarations
//class VProgressIndicator;
class VBlob;
class VValue;
class VValueSingle;
class VDuration;
class VTime;
class VUUID;

/*!
	@class	VStream
	@abstract	Streaming management class.
	@discussion
		A VStream is a generic tool to put and get data into or from a virtual container.
		The container might be a memory block or a file or a tcp socket.

		The VStream is responsible for little endian/big endian conversions.
		By default, VStream considers data stored in the stream is in little endian format (windows).

		A VStream can be opened for reading or writing but not both.
		Note that a particular implementation may forbid reading or writing.

		VStream is a virtual base class, so you cannot instantiate it directly.
		Use a particular implementation such as VFileStream.

		If you want to provide a new implementation, derive VStream and override followind methods:

			DoPutData/DoGetData
			DoGetSize/DoSetSize

		Error handling: If one VStream method throws an error, all subsequent methods does nothing but returning
		the last thrown error code until a Close method is called. So you are not forced to check each individual Put or Get.
		That let you safely write code like:
			sLONG a = stream.GetLong();
			sLONG b = stream.GetLong();
			if (stream.GetLastError() != VE_OK)
				// something bad happened, a or b might be invalid

*/
class XTOOLBOX_API VStream : public VObject
{
public:
	virtual ~VStream();

	/*!
		@function	OpenReading
		@abstract	Opens a stream for reading.
		@discussion
			Fails if already opened or if underlying implementation forbid reading.
			Opened in little endian mode.
			Initial current position is set to 0.
	*/
	VError	OpenReading();

	/*!
		@function	OpenReading
		@abstract	Opens a stream for reading.
		@param inSignature 4-byte header
		@discussion
			Fails if already opened or if underlying implementation forbid reading.
			Opens the stream in little endian mode and reads an OsType.
			If the OsType is the byteswapped version of inSignature, then the swap mode is changed to big endian.
			Else a VE_STREAM_BAD_SIGNATURE error is thrown.
			Initial current position is then set to 4.
	*/
	VError	OpenReading( OsType inSignature);

	/*!
		@function	OpenWriting
		@abstract	Opens a stream for writing.
		@discussion
			Fails if already opened or if underlying implementation forbid writing.
			Opened in little endian mode.
			Initial current position is set to 0.
	*/
	VError	OpenWriting();


	/*!
		@function	CloseReading
		@abstract	Closes a stream that was opened for reading.
		@discussion
			Fails if already closed or not opened for reading.
	*/
	VError	CloseReading();


	/*!
		@function	CloseWriting
		@abstract	Closes a stream that was opened for writing.
		@param inSetSize ask for shrink the underlying container size to minimal size after closing.
		@discussion
			Fails if already closed or not opened for writing.
			inSetSize might be typically used for VHandleStream implementations to shrink the Handle to
			used bytes.
	*/
	VError	CloseWriting( bool inSetSize = true);


	/*!
		@function	PutData
		@abstract	Sends some bytes to a stream.
		@param inBuffer address of first byte to send.
		@param inNbBytes number of bytes to send.
		@discussion
			Fails if not opened for writing.
			Current position is advanced by the number of bytes actually written.
			No byteswap is performed.
	*/
	VError	PutData( const void* inBuffer, VSize inNbBytes);


	/*!
		@function	GetData
		@abstract	Retreives some bytes from a stream.
		@param inBuffer address where to put the first byte.
		@param inNbBytes number of bytes to receive.
		@param outNbBytesCopied number of bytes actually received.
		@discussion
			Fails if not opened for reading.
			No byteswap is performed.
			Current position is advanced by the number of bytes actually read.
			VE_STREAM_EOF is thrown if less bytes than asked were received.
	*/
	VError	GetData( void* inBuffer, VSize inNbBytes, VSize* outNbBytesCopied = NULL);

	/*!
		@function	GetData
		@abstract	Retreives some bytes from a stream.
		@param inBuffer address where to put the first byte.
		@param ioNbBytes number of bytes to receive in input, number of bytes actually received in output.
		@discussion
			Fails if not opened for reading.
			No byteswap is performed.
			Current position is advanced by the number of bytes actually read.
			VE_STREAM_EOF is thrown if less bytes than asked were received.
	*/
	VError	GetData( void* inBuffer, VSize* ioNbBytes)	{ return GetData (inBuffer, *ioNbBytes, ioNbBytes); }

	/*!
		@function	UngetData
		@abstract	Put back some bytes that came from a stream opened in reading mode.
		@param inBuffer adress of first byte to put back.
		@param inNbBytes number of bytes to put back.
		@discussion
			Fails if not opened for reading.
			Given bytes must have been previously retreived with GetData.
	*/
	VError	UngetData( const void* inBuffer, VSize inNbBytes);

	/*!
		@function	GetSize
		@abstract	Returns the logical size of the stream in bytes.
		@discussion
			The stream may or may nor be opened (depends on underlying implementation).
			When opened for reading, this is typically the maximum number of bytes one can read from the stream.
			When opened for writing, this is typically the maximum number of bytes one has written into the stream.
			May be unavailable for specific implementations.
			See underlying implementation for more details.
	*/
	sLONG8	GetSize();


	/*!
		@function	SetSize
		@abstract	Set the logical size of the stream in bytes.
		@param inNewSize new logical size in bytes.
		@discussion
			Fails if not opened for writing.
			May be unavailable for specific implementations.
			See underlying implementation for more details.
			If current position is after the new logical size, the position is silently set to the new logical size.
	*/
	VError	SetSize( sLONG8 inNewSize);


	/*!
		@function	GetPos
		@abstract	Returns the current position in the stream.
		@discussion
			The stream may or may nor be opened. If not opened, returns the last position before closing.
			This is the number of bytes already read or written since opening.
	*/
	sLONG8	GetPos() const					{ return fPosition; }


	/*!
		@function	SetPos
		@abstract	Change the current position.
		@param inNewPos new current position.
		@discussion
			Fails if not opened.
			Lets you skip some bytes or rewind the stream while reading or writing.
			Rewind (changing position backwards) may be unavailable for specific implementations.
			You may need to use UngetData instead which is always available.
			Skipping forward some bytes while writing may also be unavailable.
	*/
	VError	SetPos( sLONG8 inNewPos);


	/*!
		@function	SetPosByOffset
		@abstract	Change the current position giving a relative offset.
		@param inOffset positive or negative value to add to current position.
		@discussion
			Fails if not opened.
			Lets you skip some bytes or rewind the stream while reading or writing.
			Rewind (changing position backwards) may be unavailable for specific implementations.
			Skipping forward some bytes while writing may also be unavailable.
			Throws VE_INVALID_PARAMETER if given offset would set current position beyond 0.
	*/
	VError	SetPosByOffset( sLONG8 inOffset);

	/*!
		@function	SetNeedSwap
		@abstract	Sets the byteswap mode.
		@param inNeedSwap true to ask for byteswap.
		@discussion
			The stream may or may not be opened.
			Tells is byteswap must be performed on subsequent Get/Put calls.
	*/
	void	SetNeedSwap( bool inNeedSwap);

	/*!
		@function	SetLittleEndian
		@abstract	Sets the byteswap mode to little endian.
		@discussion
			The stream may or may not be opened.
			Byteswap will be performed on subsequent Get/Put calls on big endian machines (mac).
			This is the default.
	*/
	void	SetLittleEndian();


	/*!
		@function	SetBigEndian
		@abstract	Sets the byteswap mode to big endian.
		@discussion
			The stream may or may not be opened.
			Byteswap will be performed on subsequent Get/Put calls on little endian machines (windows).
	*/
	void	SetBigEndian();

	/*!
		@function	NeedSwap
		@abstract	Returns whether byteswap is currently asked.
	*/
	bool	NeedSwap() const				{ return fNeedSwap; }

	/*!
		@function	GetLastError
		@abstract	Returns last thrown error since last opening or VE_OK if none.
	*/
	VError	GetLastError() const			{ return fError; }

	/*!
		@function	ResetLastError
		@abstract	Set last error to VE_OK.
	*/
	void	ResetLastError()				{ fError = VE_OK; }

	void	SetError( VError& inErr)		{ if (fError == VE_OK) fError = inErr; }

	/*!
		@function	SetProgressIndicator
		@abstract	Let's you install a progression callback.
		@discussion
			The callback is called on all Get/Put.
			That let's you show some progression thermometer.
	*/
	void	SetProgressIndicator( VProgressIndicator* inProgress) { IRefCountable::Copy(&fProgress, inProgress); }


	/*!
		@function	IsReading
		@abstract	Returns true is opened for reading.
	*/
	bool	IsReading() const				{ return fIsReading; }


	/*!
		@function	IsReading
		@abstract	Returns true is opened for writing.
	*/
	bool	IsWriting() const				{ return fIsWriting; }


	/*!
		@function	IsReadOnly
		@abstract	Returns true is can be opened for reading.
	*/
	bool	IsReadOnly() const				{ return fIsReadOnly; }


	/*!
		@function	IsWriteOnly
		@abstract	Returns true is can be opened for writing.
	*/
	bool	IsWriteOnly() const				{ return fIsWriteOnly; }


	/*!
		@function	Flush
		@abstract	Flush any buffered data.
		@discussion
			Flush is automatically called on closing a stream.
	*/
	VError	Flush();


/*!
		@function	PutContentInto
		@abstract	copy a stream content into another stream
		@param intoStream specifies the destination stream
		@discussion
			intoStream must already be open for wrting
	*/
	VError PutContentInto(VStream* intoStream);


	/*!
		@function	SetCharSet
		@abstract	Specify stream character set.
		@param inCharSet specifies the character set for bytes stored in the stream.
		@discussion
			Used for PutText and GetText.
			By default, VStream assumes the bytes are in VTC_DefaultTextExport charset and the cr/lf mode
			for writing is eCRM_NATIVE.
			It is not recommended to change the charset on an opened stream but it is supported.
			If no converter is available for specified char set, an error is thrown.
	*/
	VError	SetCharSet (CharSet inCharSet);
	CharSet GetCharSet() const { return fCharSet; }

	void	SetCarriageReturnMode( ECarriageReturnMode inCarriageReturnMode ) { fCarriageReturnMode = inCarriageReturnMode; }
	ECarriageReturnMode GetCarriageReturnMode() const { return fCarriageReturnMode; }

	/**
		@brief GuessCharSetFromLeadingBytes tries to recognize utf-8, utf-16 or itf-16 by reading first 4 bytes in stream.
		The stream must be open for reading and support UngetData protocol.
		if you pass optional parameter "inTryToDetectUTF8FromContent", beginning of text -up to 2048 chars- will be analyzed to try to see if escape
		sequences are compatible with UTF8, and it will set charset to UTF-8 if it matches.
	**/
	VError	GuessCharSetFromLeadingBytes( CharSet inDefaultCharSet, bool inTryToDetectUTF8FromContent = false );

	/**
		@brief WriteBOM writes BOM accordingly to current charset.
	**/
	VError	WriteBOM();

	// Operators
	void operator >> (sBYTE& outValue) { GetByte (outValue); };
	void operator >> (sWORD& outValue) { GetWord (outValue); };
	void operator >> (sLONG& outValue) { GetLong (outValue); };
	void operator >> (uLONG& outValue) { GetLong (outValue); };
	void operator >> (Real& outValue) { GetReal (outValue); };
	void operator >> (SmallReal& outValue) { GetSmallReal (outValue); };
	void operator >> (sLONG8& outValue) { GetLong8 (outValue); };
	void operator >> (uLONG8& outValue) { GetLong8 (outValue); };
	void operator >> (VValue*& outValue) { outValue = GetValue (); };

	VStream& operator << (sBYTE inValue) { PutByte (inValue); return *this; };
	VStream& operator << (sWORD inValue) { PutWord (inValue); return *this; };
	VStream& operator << (sLONG inValue) { PutLong (inValue); return *this; };
	VStream& operator << (sLONG8 inValue) { PutLong8 (inValue); return *this; };
	VStream& operator << (Real inValue) { PutReal (inValue); return *this; };
	VStream& operator << (SmallReal inValue) { PutSmallReal (inValue); return *this; };
	VStream& operator << (const VValue& inValue) { inValue.WriteToStream (this); return *this; };

#if WITH_VMemoryMgr
	void operator >> (VHandle& outValue) { GetHandle (outValue); };
	VStream& operator << (VHandle inValue) { PutHandle (inValue); return *this; };
	VError	GetHandle (VHandle& outValue);
	VHandle GetHandle () { VHandle h; GetHandle (h); return h; };
	VError	PutHandle (VHandle inValue);
#endif

	// Data accessors
	VError	GetByte (sBYTE& outValue) { return GetData (&outValue, sizeof (outValue)); }
	VError	GetByte (uBYTE& outValue) { return GetData (&outValue, sizeof (outValue)); }
	sBYTE	GetByte() { sBYTE cc = 0; GetData (&cc, sizeof (cc)); return cc; }
	uBYTE	GetUByte() { uBYTE cc = 0; GetData (&cc, sizeof (cc)); return cc; }
	VError	GetBytes (void* outValue, sLONG* ioCount) { VSize nbBytes = (VSize)*ioCount; GetData (outValue, *ioCount, &nbBytes); *ioCount = (sLONG) nbBytes; return fError; }
	VError	GetBytes (void* outValue, VSize* ioCount) { VSize nbBytes = *ioCount; GetData (outValue, *ioCount, &nbBytes); *ioCount = nbBytes; return fError; }

	VError	GetWord (sWORD& outValue) { GetData (&outValue, sizeof (outValue)); if (fNeedSwap) ByteSwapWord (&outValue); return fError; }
	VError	GetWord (uWORD& outValue) { GetData (&outValue, sizeof (outValue)); if (fNeedSwap) ByteSwapWord (&outValue); return fError; }
	sWORD	GetWord () { sWORD cc = 0; GetData (&cc, sizeof (cc)); if (fNeedSwap) ByteSwapWord (&cc); return cc; }
	uWORD	GetUWord () { uWORD cc = 0; GetData (&cc, sizeof (cc)); if (fNeedSwap) ByteSwapWord (&cc); return cc; }
	VError	GetWords (sWORD* outValue, sLONG* ioCount);
	VError	GetWords (uWORD* outValue, sLONG* ioCount) { return GetWords ( (sWORD*) outValue, ioCount); }

	VError	GetLong (sLONG& outValue) { GetData (&outValue, sizeof (outValue)); if (fNeedSwap) ByteSwapLong (&outValue); return fError; }
	VError	GetLong (uLONG& outValue) { GetData (&outValue, sizeof (outValue)); if (fNeedSwap) ByteSwapLong (&outValue); return fError; }
	sLONG	GetLong () { sLONG cc = 0; GetData (&cc, sizeof (cc)); if (fNeedSwap) ByteSwapLong (&cc); return cc; }
	uLONG	GetULong () { uLONG cc = 0; GetData (&cc, sizeof (cc)); if (fNeedSwap) ByteSwapLong (&cc); return cc; }
	VError	GetLongs (sLONG* outValue, sLONG* ioCount);
	VError	GetLongs (uLONG* outValue, sLONG* ioCount) { return GetLongs ( (sLONG*)outValue, ioCount); }

	VError	GetLong8 (sLONG8& outValue) { GetData (&outValue, sizeof (outValue)); if (fNeedSwap) ByteSwapLong8 (&outValue); return fError; }
	VError	GetLong8 (uLONG8& outValue) { GetData (&outValue, sizeof (outValue)); if (fNeedSwap) ByteSwapLong8 (&outValue); return fError; }
	sLONG8	GetLong8 () { sLONG8 cc = 0; GetData (&cc, sizeof (cc)); if (fNeedSwap) ByteSwapLong8 (&cc); return cc; }
	uLONG8	GetULong8 () { uLONG8 cc = 0; GetData (&cc, sizeof (cc)); if (fNeedSwap) ByteSwapLong8 (&cc); return cc; }
	VError	GetLong8s (sLONG8* outValue, sLONG* ioCount);
	VError	GetLong8s (uLONG8* outValue, sLONG* ioCount) { return GetLong8s ( (sLONG8*) outValue, ioCount); }

	VError	GetReal (Real& outValue) { GetData (&outValue, sizeof (outValue)); if (fNeedSwap) ByteSwapReal8 (&outValue); return fError; }
	Real	GetReal () { Real cc = 0; GetData (&cc, sizeof (cc)); if (fNeedSwap) ByteSwapReal8(&cc); return cc; }
	VError	GetReals (Real* outValue, sLONG* ioCount);

	VError	GetSmallReal (SmallReal& outValue) { GetData (&outValue, sizeof (outValue)); if (fNeedSwap) ByteSwapReal4 (&outValue); return fError; }
	Real	GetSmallReal () { SmallReal cc = 0; GetData (&cc, sizeof (cc)); if (fNeedSwap) ByteSwapReal4(&cc); return cc; }
	VError	GetSmallReals (SmallReal* outValue, sLONG* ioCount);

	void*	GetPointer() { void* pt; GetData( &pt, sizeof(void*)); return pt; }
//	VError	GetUniChars( UniChar* outValue, sLONG* ioCount) { return GetWords ( (sWORD*) outValue, ioCount); };

	VError	GetValue (VValue& ioValue) { return ioValue.ReadFromStream (this); }
	VValue*	GetValue ();

	template<class T>
	VError	Put( const T& inValue)
		{
			if (!fNeedSwap)
				return PutData( &inValue, sizeof( T));
			T val = inValue;
			ByteSwap( &val);
			return PutData( &val, sizeof( val));
		}

	template<class T>
	VError	Get( T *outValue)
		{
			VError err = GetData( outValue, sizeof( T));
			if (fNeedSwap)
				ByteSwap( outValue);
			return err;
		}

	VError	PutByte (sBYTE inValue) { return PutData(&inValue, sizeof(inValue)); }
	VError	PutBytes (const void* inValue, sLONG inCount) { return PutData(inValue, inCount); }

	VError	PutWord (sWORD inValue) { if (fNeedSwap) ByteSwapWord(&inValue); return PutData(&inValue, sizeof(inValue)); };
	VError	PutWords ( const sWORD* inValue, sLONG inCount);
	VError	PutWords ( const uWORD* inValue, sLONG inCount) { return PutWords((sWORD*) inValue, inCount); };

	VError	PutLong (sLONG inValue) { if (fNeedSwap) ByteSwapLong(&inValue); return PutData(&inValue, sizeof(inValue)); };
	VError	PutLongs (const sLONG* inValue, sLONG inCount);
	VError	PutLongs (const uLONG* inValue, sLONG inCount) { return PutLongs((sLONG*) inValue, inCount); };

	VError	PutLong8 (sLONG8 inValue) { if (fNeedSwap) ByteSwapLong8(&inValue); return PutData(&inValue, sizeof(inValue)); };
	VError	PutLong8s (const sLONG8* inValue, sLONG inCount);
	VError	PutLong8s (const uLONG8* inValue, sLONG inCount) { return PutLong8s((sLONG8*) inValue, inCount); };

	VError	PutReal (Real inValue) { if (fNeedSwap) ByteSwapReal8(&inValue); return PutData(&inValue, sizeof(inValue)); };
	VError	PutReals (const Real* inValue, sLONG inCount);

	VError	PutSmallReal (SmallReal inValue) { if (fNeedSwap) ByteSwapReal4(&inValue); return PutData(&inValue, sizeof(inValue)); };
	VError	PutSmallReals (const SmallReal* inValue, sLONG inCount);

	VError	PutPointer( const void* inValue ) { return PutData( &inValue, sizeof(void*)); };
	VError	PutValue (const VValue& inValue, bool inWithKind = false);

	VError	PutHexBytes (const sBYTE* inValue, VSize inCount);
//	VError	PutUniChars ( const UniChar* inValue, sLONG inCount) { return PutWords((sWORD*) inValue, inCount); };

	/*!
		@function	PutText
		@abstract Sends some characters into the stream.
		@param inText the characters to send
		@discussion
			The stream must be opened for writing.
			Characters are being sent after conversion in the character set specified with VStream::SetCharSet.
			CR/LF conversions are made according to the mode specified with VStream::SetCharSet.
	*/
	VError	PutText (const VString& inText);


	/*!
		@function	PutPrintf
		@abstract Sends some characters into the stream with printf-like formatting capabilities.
		@param inMessage the formatting string.
		@discussion
			Simply builds a temporary VString with VString::VPrintf and calls VStream::PutText.
	*/

	VError	PutPrintf (const char* inMessage,...);
	VError	PutVPrintf(const char* inMessage, va_list argList);


	/*!
		@function	PutCString
		@abstract Sends some had-coded ansi characters into the stream.
		@param inMessage the characters to send
		@discussion
			Simply builds a temporary VString and calls VStream::PutText.
	*/
	VError	PutCString (const char* inMessage) { return PutText(VString(inMessage)); }

	/*!
		@function	GetText
		@abstract Retreive any character it cans from a stream.
		@param outText the VString to receive new characters.
		@discussion
			The stream must be opened for reading.
			All characters are being read until an error occurs (usually VE_STREAM_EOF).
			The error VE_STREAM_EOF is never returned nor thrown as that is the normal case for this function.
			Characters are being appended after conversion in the character set specified with VStream::SetCharSet.
			No CR/LF conversion is performed. You may want to call VString::ConvertCarriageReturns to do that.
	*/
	VError	GetText( VString& outText);

	/*!
		@function	GetText
		@abstract Retreive a specified number of characters from a stream.
		@param outText the VString to receive new characters.
		@param inNbChars the number of unicode characters to read.
		@param inLoosyMode tells you don't care getting more chars than requested.
		@discussion
			The stream must be opened for reading.
			All characters are being read until an error occurs (usually VE_STREAM_EOF)
			or the number of unicode characters requested have been read.
			The error VE_STREAM_EOF may be returned but it is not thrown as that is the normal case for this function.
			Characters are being appended after conversion in the character set specified with VStream::SetCharSet.
			No CR/LF conversion is performed. You may want to call VString::ConvertCarriageReturns to do that.

			If there're enough characters available, VStream::GetText returns exactly the number of utf characters requested
			unless inLoosyMode is set in which case you may get extra characters but performances may be better.

			Note1: For non utf16 char set, this is a complex task to stop reading bytes exactly after a given number of converted unichars.
			So especially for non utf16 char set, ask the loosy mode when possible.

			Note2: In some extraordinay cases, one non utf byte might give 2 utf chars. In that only case VStream::GetText will
			return one more char than requested (don't ask me an example!).

			Note3: VStream::GetText must not be confused with VString::ReadFromStream. The later expects bytes to be stored by
			VString::WriteToStream in an opaque manner.
	*/
	VError	GetText( VString& outText, VIndex inNbChars, bool inLoosyMode = true);


	/*!
		@function	GetTextLine
		@abstract Retreive characters from a stream until a carriage return is encountered.
		@param outText the VString to receive new characters.
		@param inAppendCarriageReturn tells if a CR unichar must be appended to outText.
		@discussion
			The stream must be opened for reading.
			All characters are being read until an error occurs (usually VE_STREAM_EOF) or a CR or a LF is encountered or *delimiter is encountered.
			The error VE_STREAM_EOF may be returned but it is not thrown as that is the normal case for this function.
			Characters are being appended after conversion in the character set specified with VStream::SetCharSet.
			GetTextLine looks for a sequence like CRLF, CR or LF. If such a sequence is found, it is being discarded and
			if inAppendCarriageReturn is true a CR is appended to outText instead.
	*/
	VError	GetTextLine( VString& outText, bool inAppendCarriageReturn, UniChar* delimiter = NULL);

	/*!
		@function	GetTTRLine
		@abstract Fills an arrayString with all the string separated by a tabulation, until a carriage return is encountered.
		@param outArrayText the VArrayString to receive new strings.
		@discussion
			The stream must be opened for reading.
			All characters are being read until an error occurs (usually VE_STREAM_EOF) or a CR or a LF is encountered.
			The error VE_STREAM_EOF may be returned but it is not thrown as that is the normal case for this function.
			Characters are being appended after conversion in the character set specified with VStream::SetCharSet.
			GetTTRLine looks for a sequence like CRLF, CR or LF. If such a sequence is found, it is being discarded.
			Tabulation used to separate strings are also discarded.
	*/
	VError	GetTTRLine( VArrayString& outArrayText);

	void ResetFlushedStatus()
	{
		fWasFlushed = false;
	}

	void SetFlushedStatus()
	{
		fWasFlushed = true;
	}

	bool GetFlushedStatus() const
	{
		return fWasFlushed;
	}

protected:
	VProgressIndicator*		fProgress;
	sLONG8					fPosition;
	bool					fIsReading;
	bool					fIsWriting;
	bool					fIsReadOnly;
	bool					fIsWriteOnly;
	bool					fNeedSwap;
	bool					fWasFlushed;
	VError					fError;

	// charset conversion support
	CharSet					fCharSet;
	ECarriageReturnMode		fCarriageReturnMode;
	VToUnicodeConverter*	fToUniConverter;
	VFromUnicodeConverter*	fFromUniConverter;

	// Private constructor
					VStream ();

	// Private state support
			VError	SetReadOnly( bool inReadOnly);
			VError	SetWriteOnly( bool inWriteOnly);

	// Inherited from VStream
	virtual VError	DoOpenReading ();
	virtual VError	DoOpenWriting ();
	virtual VError	DoCloseReading ();
	virtual VError	DoCloseWriting (Boolean inSetSize);
	virtual VError	DoPutData (const void* inBuffer, VSize inNbBytes) = 0;
	virtual VError	DoGetData (void* inBuffer, VSize* ioCount) = 0;
	virtual VError	DoUngetData (const void* inBuffer, VSize inNbBytes);
	virtual sLONG8	DoGetSize () = 0;
	virtual VError	DoSetSize (sLONG8 inNewSize) = 0;
	virtual VError	DoSetPos (sLONG8 inNewPos);
	virtual VError	DoFlush ();
};


template<>	inline VError	VStream::Put( const VValue& inValue)	{ return PutValue( inValue, false); }
template<>	inline VError	VStream::Get( VValue *outValue)			{ return GetValue( *outValue); }

template<>	inline VError	VStream::Put( const VValueSingle& inValue)	{ return PutValue( inValue, false); }
template<>	inline VError	VStream::Get( VValueSingle *outValue)		{ return GetValue( *outValue); }

template<>	inline VError	VStream::Put( const VString& inValue)	{ return PutValue( inValue, false); }
template<>	inline VError	VStream::Get( VString *outValue)		{ return GetValue( *outValue); }

template<>	inline VError	VStream::Put( const VUUID& inValue)		{ return PutValue( inValue, false); }
template<>	inline VError	VStream::Get( VUUID *outValue)			{ return GetValue( *outValue); }

template<>	inline VError	VStream::Put( const VTime& inValue)		{ return PutValue( inValue, false); }
template<>	inline VError	VStream::Get( VTime *outValue)			{ return GetValue( *outValue); }

template<>	inline VError	VStream::Put( const VDuration& inValue)	{ return PutValue( inValue, false); }
template<>	inline VError	VStream::Get( VDuration *outValue)		{ return GetValue( *outValue); }


#if WITH_VMemoryMgr
// DEPRECATED
class XTOOLBOX_API VHandleStream : public VStream
{
public:
	VHandleStream( VHandleStream* inHandleStream );
	VHandleStream (VHandle* inHandle = NULL);
	virtual	~VHandleStream ();

	// Specific acceessors
	void*	GetDataPtr ();

protected:
	VHandle	fHandle;
	VPtr	fData;
	sLONG8	fLogSize;
	sLONG8	fPhysSize;
	Boolean	fOwnHandle;

	// Inherited from VStream
	virtual VError	DoOpenReading ();
	virtual VError	DoCloseReading ();
	virtual VError	DoOpenWriting () ;
	virtual VError	DoCloseWriting (Boolean inSetSize);
	virtual VError	DoGetData (void* inBuffer, VSize* ioCount);
	virtual VError	DoPutData (const void* inBuffer, VSize inNbBytes);
	virtual VError	DoSetSize (sLONG8 inNewSize);
	virtual VError	DoSetPos (sLONG8 inNewPos);
	virtual sLONG8	DoGetSize ();
};
#endif

class XTOOLBOX_API VConstPtrStream : public VStream
{
public:
							VConstPtrStream( const void* inData, VSize inNbBytes) : fData( inData), fSize(inNbBytes) 	{ SetReadOnly(true); }
							VConstPtrStream() : fData( NULL), fSize(0)	{;}

			void			SetDataPtr( const void* inData, VSize inNbBytes)		{ xbox_assert( !IsReading() && !IsWriting()); fData = inData; fSize = inNbBytes;}

			const void*		GetDataPtr() const										{ return fData; }	// may be NULL
			VSize			GetDataSize() const										{ return fSize; }

			bool			IsEmpty() const											{ return fSize == 0;}

			void*			ForgetDataPtr()											{ xbox_assert( !IsReading() && !IsWriting()); const void *data = fData; fData = NULL; fSize = 0; return const_cast<void*>( data); }

protected:
			const void*	fData;
			VSize		fSize;

	// Inherited from VStream
	virtual VError	DoOpenWriting () ;
	virtual VError	DoPutData (const void* inBuffer, VSize inNbBytes);
	virtual VError	DoGetData (void* inBuffer, VSize* ioCount);
	virtual VError	DoSetPos (sLONG8 inNewPos);
	virtual sLONG8	DoGetSize ();
	virtual VError	DoSetSize (sLONG8 inNewSize);
};


class XTOOLBOX_API VPtrStream : public VStream
{
public:
			VPtrStream()	{;}

			const void*		GetDataPtr() const							{ return fBuffer.GetDataPtr(); }	// may be NULL
			void*			GetDataPtr()								{ return fBuffer.GetDataPtr(); }	// may be NULL
			VSize			GetDataSize() const							{ return fBuffer.GetDataSize(); }

			bool			IsEmpty() const								{ return fBuffer.IsEmpty();}

			void			SetDataPtr( void* inData, VSize inNbBytes)	{ xbox_assert( !IsReading() && !IsWriting()); fBuffer.SetDataPtr( inData, inNbBytes, inNbBytes); }


	/**
		@brief Steal the memory pointer from the stream.
		The stream becomes empty.
		If the blob was the data owner, you must use VMemory::DisposePtr() to deallocate the pointer you get.
	**/
			void*			StealData()									{ xbox_assert( !IsReading() && !IsWriting()); void *p = fBuffer.GetDataPtr(); fBuffer.ForgetData(); return p;}

			void			Clear()										{ xbox_assert( !IsReading() && !IsWriting()); fBuffer.Clear(); }


protected:
			VMemoryBuffer<>	fBuffer;

	// Inherited from VStream
	virtual VError			DoPutData( const void* inBuffer, VSize inNbBytes);
	virtual VError			DoGetData( void* inBuffer, VSize* ioCount);
	virtual VError			DoSetPos( sLONG8 inNewPos);
	virtual sLONG8			DoGetSize();
	virtual VError			DoSetSize(sLONG8 inNewSize);
};


class XTOOLBOX_API VBlobStream : public VStream
{
public:
			VBlobStream (VBlob* inBlob = NULL);
	virtual	~VBlobStream ();

protected:
	VBlob*	fBlob;
	Boolean	fIsOwner;

	// Inherited from VStream
	virtual VError	DoPutData (const void* inBuffer, VSize inNbBytes);
	virtual VError	DoGetData (void* inBuffer, VSize* ioNbBytes);
	virtual VError	DoSetPos (sLONG8 inNewPos);
	virtual VError	DoSetSize (sLONG8 inNewSize);
	virtual sLONG8	DoGetSize ();
};

END_TOOLBOX_NAMESPACE

#endif

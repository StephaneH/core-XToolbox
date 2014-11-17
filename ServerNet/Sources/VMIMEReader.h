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

#ifndef __MIME_READER_INCLUDED__
#define __MIME_READER_INCLUDED__

#include "ServerNet/Sources/VHTTPMessage.h"

BEGIN_TOOLBOX_NAMESPACE


class XTOOLBOX_API VMIMEReader : public XBOX::VObject
{
public:
									VMIMEReader (const XBOX::VString& inBoundary, XBOX::VStream& inStream);
	virtual							~VMIMEReader();

	bool							GetNextPart (VHTTPMessage& ioMessage);
	bool							HasNextPart();
	XBOX::VStream&					GetStream() const;
	const XBOX::VString&			GetBoundary() const;

	// Decode quoted printable ASCII data to 8-bit binary. This binary data can then be decoded to actual character encoding.

	static XBOX::VError				DecodeQuotedPrintable (const void *inData, VSize inDataSize, VMemoryBuffer<> *outResult);

	// Decode all encoded words of an 7-bit data stream (usually from a mail header field-body).

	static XBOX::VError				DecodeEncodedWords (const void *inData, VSize inDataSize, XBOX::VString *outResult);

protected:
	bool							_FindFirstBoundary();
	bool							_GuessBoundary();
	bool							_ParseMessage (VHTTPMessage& ioMessage);
	bool							_ReadLine (XBOX::VString& line, XBOX::VSize inSize);
	bool							_LastPart();

private:
	XBOX::VStream&					fStream;
	XBOX::VString					fBoundary;
	XBOX::VString					fFirstBoundaryDelimiter;
	XBOX::VString					fLastBoundaryDelimiter;
};

class XTOOLBOX_API VMemoryBufferStream : public XBOX::VStream
{
public:

	// It is caller's responsibility to allocate and free memory buffer(s). Also it must be ensured that
	// memory buffer(s) are usable (not already disposed).

							VMemoryBufferStream (const XBOX::VMemoryBuffer<> *inMemoryBuffer);
							VMemoryBufferStream (const XBOX::VMemoryBuffer<> *inMemoryBuffers, sLONG inNumberMemoryBuffers);
	virtual					~VMemoryBufferStream ();

	// Return in sLONG the 8-bit byte or -1 if the end of stream has been reached.

	sLONG					GetNextByte ()				{	uBYTE	c; return GetData(&c, 1) == XBOX::VE_OK ? c : -1;	}

	// Unread last byte, silently ignore error.

	void					PutBackByte (uBYTE inChar)	{	UngetData(&inChar, 1);										}

protected:

	// DoSetPos() fails if inNewPos is smaller than zero or bigger than total size. It is possible to do DoSetPos() at 
	// the total size of all memory buffers, in which case a XBOX::VE_STREAM_EOF is returned.
	
	virtual XBOX::VError	DoSetPos (sLONG8 inNewPos);

	// DoGetSize() returns the total size of all memory buffer(s).

	virtual sLONG8			DoGetSize ();

	// It is an error to use DoSetSize() because memory buffer(s) cannot be resized.

	virtual XBOX::VError	DoSetSize (sLONG8 inNewSize);

	// Write inNbBytes of data in inBuffer back. If inNbBytes is zero, then always return XBOX::VE_OK.

	virtual XBOX::VError	DoUngetData (const void *inBuffer, VSize inNbBytes);

	// Write as much as possible and if end of memory buffers is reached, return XBOX::VE_STREAM_EOF.
	// If inNbBytes is zero, then always return XBOX::VE_OK.

	virtual XBOX::VError	DoPutData (const void *inBuffer, VSize inNbBytes);

	// Return XBOX::VE_STREAM_EOF if end of memory buffers was reached.
	// If *ioCount is zero, then always return XBOX::VE_OK.

	virtual XBOX::VError	DoGetData (void *outBuffer, VSize *ioCount);
	
private: 

	const XBOX::VMemoryBuffer<>	*fMemoryBuffers;
	sLONG						fNumberMemoryBuffers;
	
	VSize						*fStartIndexes;		// Has one more index for total size.
	VSize						fTotalSize;
	
	// fCurrentMemoryBufferIndex is the index of the current memory buffer. 
	// fPosition (inherited from XBOX::VStream) is always within zero and fTotalSize (which is not writable).
	// If fPosition is equal to fTotalSize, fCurrentMemoryBufferIndex is equal to fNumberMemoryBuffers - 1.

	sLONG						fCurrentMemoryBufferIndex;	
	VSize						fTotalBytes;

	void						_initialize (const XBOX::VMemoryBuffer<> *inMemoryBuffers, sLONG inNumberMemoryBuffers);
};

END_TOOLBOX_NAMESPACE

#endif //__MIME_READER_INCLUDED__

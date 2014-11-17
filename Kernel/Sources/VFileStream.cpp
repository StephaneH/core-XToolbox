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
#include "VFileStream.h"
#include "VErrorContext.h"
#include "VMemory.h"
#include "VFile.h"


VFileStream::VFileStream( const VFile *inFile,FileOpenOptions inPreferredWriteOpenMode)
{
	fFile = inFile;	
	assert(inFile != NULL);
	fFile->Retain();
	fFileDesc = NULL;
	fLogSize = 0;
	fBufPos = 0;
	fBufSize = 0;
	fBufCount = 0;
	fBuffer = NULL;
	fPreferredBufSize = 32768L;
	fPreferredWriteOpenMode = inPreferredWriteOpenMode;
}

 
VFileStream::~VFileStream()
{
	if (fBuffer != NULL)
		VMemory::DisposePtr( fBuffer);

	if (fFileDesc != NULL)
		delete fFileDesc;
	fFile->Release();

}


VError VFileStream::DoCloseReading()
{
	VError err = VE_OK;
	
	ReleaseBuffer();
	if (fFileDesc != NULL)
		delete fFileDesc;
	fFileDesc = NULL;

	return err;
}



VError VFileStream::DoGetData( void *inBuffer, VSize *ioCount)
{
	VError	err = VE_OK;
	sLONG8 pos = GetPos();
	VSize byteToRead = *ioCount;
	
	// is there some valuable data in the buffer ?
	VSize readBytes = 0;
	//Cast pour éviter le warning C4018
	if ((pos >= fBufPos) && (pos < fBufPos + ((sLONG8)fBufCount))) {
		readBytes = (VSize) ((fBufPos + fBufCount) - pos);
		if (readBytes > *ioCount)
			readBytes = *ioCount;
		::CopyBlock( fBuffer + pos - fBufPos, inBuffer, readBytes);
		pos += readBytes;
	}

	if (readBytes < *ioCount) {
		VSize toRead = *ioCount - readBytes;
		if (toRead > fBufSize) {	// data cannot fit in buffer, so we read it from file
			err = fFileDesc->GetData( (char *) inBuffer + readBytes, toRead, pos, &toRead );
		} else { // fill one buf
			fBufPos = pos;
			fBufCount = fBufSize;
			if ((sLONG8)fBufCount > fLogSize - fBufPos)
				fBufCount = (VSize)(fLogSize - fBufPos);
			err = fFileDesc->GetData( fBuffer, fBufCount, fBufPos);
			if (toRead > fBufCount) // cant read enough
				toRead = fBufCount;
			::CopyBlock( fBuffer, (char *) inBuffer + readBytes, toRead);
		}
		*ioCount = readBytes + toRead;

		if ( *ioCount == byteToRead )
		{
			if ( err == VE_STREAM_EOF )
				err = VE_OK;
		}
		else
		{
			err = VE_STREAM_EOF;
		}
	}
	return err;
}

VError VFileStream::DoPutData( const void *inBuffer, VSize inNbBytes)
{
	VError err = VE_OK;
	
	sLONG8 pos = GetPos();

	VSize writtenBytes = 0;

	// first start by filling the buffer
	if (fBufCount == 0)
	{
		// the buffer is empty
		if (inNbBytes < fBufSize)
		{
			// the buffer is empty and is bigger than the request
			::CopyBlock( inBuffer, fBuffer, inNbBytes);
			writtenBytes = inNbBytes;
			fBufPos = pos;
			fBufCount = writtenBytes;
		}
	}
	else if ( (pos >= fBufPos) && (pos <= fBufPos + ((sLONG8)fBufCount))) //Cast pour éviter le warning C4018
	{
		// the buffer is not empty and we can give it some more bytes
		writtenBytes = (VSize) (fBufPos + fBufSize - pos);
		if (writtenBytes > inNbBytes)
			writtenBytes = inNbBytes;
		::CopyBlock( inBuffer, fBuffer + pos - fBufPos, writtenBytes);
		fBufCount += writtenBytes;
	}

	if (writtenBytes < inNbBytes)
	{
		// write our buffer (no flush)
		if (fBufCount > 0)
		{
			err = fFileDesc->PutData( fBuffer, fBufCount, fBufPos, &fBufCount);
			SetFlushedStatus();
		}

		if (err == VE_OK)
		{
			VSize toWrite = inNbBytes - writtenBytes;
			if (toWrite >= fBufSize)
			{
				// does not fit into the buffer, write data directly
				fBufCount = 0;
				err = fFileDesc->PutData( (char *) inBuffer + writtenBytes, toWrite, pos + writtenBytes, &toWrite);
				SetFlushedStatus();
				if (err == VE_OK)
					writtenBytes = inNbBytes;
			}
			else
			{
				// fill buffer
				::CopyBlock( (char *) inBuffer + writtenBytes, fBuffer, toWrite);
				fBufPos = pos + writtenBytes;
				fBufCount = toWrite;
				writtenBytes = inNbBytes;
			}
		}
	}

	if (err == VE_OK)
	{
		//Cast pour éviter le warning C4018
		if (pos + ((sLONG8)writtenBytes) > fLogSize)
			fLogSize = pos + writtenBytes;
	}
		
	return err;
}


VError VFileStream::DoOpenWriting()
{
	if (fFile == NULL) 
		return vThrowError( VE_STREAM_CANNOT_FIND_SOURCE);
	
	if (fFileDesc) 
		return vThrowError( VE_STREAM_CANNOT_WRITE); // already open read-only

	VError err = VE_OK;

	err = fFile->Open( FA_SHARED, &fFileDesc, fPreferredWriteOpenMode);

	if (err == VE_OK)
	{
		fLogSize = fFileDesc->GetSize();
		AllocateBuffer( MaxLongInt);
	}

	return err;
}

VError VFileStream::DoCloseWriting( Boolean inSetSize)
{
	VError err = VE_OK;
	
	ReleaseBuffer();
	if (inSetSize)
		err = fFileDesc->SetSize( fLogSize);
	
	if (fFileDesc != NULL)
		delete fFileDesc;
	fFileDesc = NULL;

	return err;
}


VError VFileStream::DoSetPos( sLONG8 inNewPos)
{
	VError err = VE_OK;

	if (IsWriting()) {
		//Cast pour éviter le warning C4018
		if (inNewPos > fBufPos + ((sLONG8)fBufCount))
			err = DoFlush();
		if (inNewPos > fLogSize) {
			VError err2 = fFileDesc->SetSize( inNewPos);
			if (err2 == VE_OK) 
				fLogSize = inNewPos;
			if (err == VE_OK)
				err = err2;
		}
	} else if (IsReading()) {
		if (inNewPos > fLogSize)
			err = vThrowError( VE_STREAM_EOF);
	}

	return err;
}


VError VFileStream::DoFlush()
{
	VError err = VE_OK;
	if (fBufCount > 0)
	{
		err = fFileDesc->PutData( fBuffer, fBufCount, fBufPos, &fBufCount);
		SetFlushedStatus();
	}

	VError err2 = fFileDesc->Flush();
	if (err == VE_OK)
		err = err2;

	fBufPos = GetPos();
	fBufCount = 0;
	return err;
}


VError VFileStream::DoSetSize( sLONG8 inNewSize)
{
	if (fBufPos >= inNewSize) {
		// discard all buf data
		fBufCount = 0;
	} else if (fBufPos + ((sLONG8)fBufCount) > inNewSize) { //Cast pour éviter le warning C4018
		// discard some data
		fBufCount = (VSize) (inNewSize - fBufPos);
	}

	VError err = fFileDesc->SetSize( inNewSize);
	if (err == VE_OK)
		fLogSize = inNewSize;

	return err;
}


VError VFileStream::DoOpenReading()
{
	if (fFile == NULL)
		return vThrowError( VE_STREAM_CANNOT_FIND_SOURCE);
	
	VError err = VE_OK;
	if (fFileDesc == NULL)
	{
		err = fFile->Open( FA_READ, &fFileDesc );
	}
	
	if (err == VE_OK)
	{
		fLogSize = fFileDesc->GetSize();
		
		if (fLogSize > (sLONG8)MaxLongInt)
			AllocateBuffer(MaxLongInt);
		else
			AllocateBuffer( (VSize) fLogSize);
		
	}

	return err;
}


sLONG8 VFileStream::DoGetSize()
{
	return fLogSize;
}


void VFileStream::SetBufferSize( VSize inNbBytes)
{
	if (IsWriting())
		Flush();
	
	ReleaseBuffer();
	fPreferredBufSize = inNbBytes;
	
	if (IsWriting() || IsReading())
		AllocateBuffer( (IsReading() && fLogSize < XBOX_LONG8(0x7FFFFFFF)) ? (VSize) fLogSize : MaxLongInt);
}


void VFileStream::AllocateBuffer( VSize inMax)
{
	if (fBuffer == NULL) {
		assert(inMax >= 0);
		if (inMax <= 0)
			fBufSize = fPreferredBufSize;
		else
			fBufSize = (fPreferredBufSize > inMax) ? inMax : fPreferredBufSize;
		do {
			fBuffer = (sBYTE*) VMemory::NewPtr( fBufSize, 'stmf');
			if (fBuffer == NULL)
				fBufSize /= 2;
		} while( (fBufSize > 0) && (fBuffer == NULL));
	}
	fBufPos = 0;
	fBufCount = 0;
}


void VFileStream::ReleaseBuffer()
{
	if (fBuffer) {
		VMemory::DisposePtr( fBuffer);
		fBuffer = NULL;
		fBufSize = 0;
		fBufPos = 0;
		fBufCount = 0;
	}
}



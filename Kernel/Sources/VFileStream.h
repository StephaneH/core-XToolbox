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
#ifndef __VFileStream__
#define __VFileStream__

#include "VStream.h"
#include "VFile.h"

BEGIN_TOOLBOX_NAMESPACE

// Needed declarations
class VFile;

class XTOOLBOX_API VFileStream : public VStream
{ 
public:
	VFileStream (const VFile* inFile,FileOpenOptions inPreferredWriteOpenMode = FO_Default);

	virtual ~VFileStream ();
	
	void	SetBufferSize (VSize inNbBytes);

	const VFile* GetVFile() const { return fFile; };
	
protected:
	// Inherited from VStream
	virtual VError	DoOpenReading ();
	virtual VError	DoCloseReading ();
	virtual VError	DoGetData (void* inBuffer, VSize* ioCount);
	
	virtual VError	DoOpenWriting ();
	virtual VError	DoCloseWriting (Boolean inSetSize);
	virtual VError	DoPutData (const void* inBuffer, VSize inNbBytes);
	
	virtual VError	DoSetSize (sLONG8 inNewSize);
	virtual VError	DoSetPos (sLONG8 inNewPos);
	virtual sLONG8	DoGetSize ();
	virtual VError	DoFlush ();
	
protected:
	const VFile		*fFile;
	VFileDesc		*fFileDesc;
	sLONG8			fLogSize;	// logical file size
	sLONG8			fBufPos;	// logical position of buffer
	VSize			fBufCount;	// useful bytes in buffer
	VSize			fBufSize;	// physical size of buffer (size of fBuffer)
	VSize			fPreferredBufSize;	// preferred buf size, 32k by default
	sBYTE			*fBuffer;
	FileOpenOptions	fPreferredWriteOpenMode;//preferred open mode when opening for write access
	
	void	AllocateBuffer (VSize inMax);
	void	ReleaseBuffer ();
};

END_TOOLBOX_NAMESPACE

#endif

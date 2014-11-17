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
#include "ServerNetTypes.h"

#ifndef __SERVERNET_ENDPOINT_STREAM__
#define __SERVERNET_ENDPOINT_STREAM__

BEGIN_TOOLBOX_NAMESPACE

// VEndPointStream supports blocking VTCPEndPoint only.

class XTOOLBOX_API VEndPointStream : public XBOX::VStream
{
public:

							VEndPointStream (VTCPEndPoint *inEndPoint, sLONG inTimeOut = 0);
	virtual					~VEndPointStream ();

	void					SetTimeOut (sLONG inTimeOut);

protected:

	// VTCPEndPoint opening or closing is not to be handled by VEndPointStream.
	// Use default implementation of XBOX::VStream, which do nothing.

//	virtual XBOX::VError	DoOpenReading ();
//	virtual XBOX::VError	DoOpenWriting ();
//	virtual XBOX::VError	DoCloseReading ();
//	virtual XBOX::VError	DoCloseWriting (Boolean inSetSize);

	// Not applicable, so do nothing and always successful. 
	// DoUngetData() and DoSetPos() use default implementation
	
//	virtual XBOX::VError	DoUngetData (const void *inBuffer, VSize inNbBytes);
//	virtual XBOX::VError	DoSetPos (sLONG8 inNewPos);

	// DoGetSize() returns the number of byte(s) read or written. DoSetSize() does nothing.

	virtual sLONG8			DoGetSize ();
	virtual XBOX::VError	DoSetSize (sLONG8 inNewSize);

	// Implemented using synchronous reads or writes.

	virtual XBOX::VError	DoPutData (const void *inBuffer, VSize inNbBytes);
	virtual XBOX::VError	DoGetData (void *inBuffer, VSize *ioCount);

	// Writing is always synchronous, so flushing will always be successful which is 
	// the default implementation of XBOX::VStream.
	
//	virtual XBOX::VError	DoFlush ();

private: 

	VTCPEndPoint			*fEndPoint;
	sLONG					fTimeOut;
	sLONG8					fTotalBytes;
};

END_TOOLBOX_NAMESPACE

#endif
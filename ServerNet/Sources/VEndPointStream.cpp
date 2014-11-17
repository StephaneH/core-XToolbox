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

#include "VServerNetPrecompiled.h"

#include "VTCPEndPoint.h"
#include "VEndPointStream.h"

USING_TOOLBOX_NAMESPACE

VEndPointStream::VEndPointStream (VTCPEndPoint *inEndPoint, sLONG inTimeOut)
{
	xbox_assert(inEndPoint != NULL);
	xbox_assert(inEndPoint->IsBlocking());
	xbox_assert(inTimeOut >= 0);
		
	fEndPoint = XBOX::RetainRefCountable<VTCPEndPoint>(inEndPoint);
	fTimeOut = inTimeOut;
	fTotalBytes = 0;
}

VEndPointStream::~VEndPointStream ()
{
	xbox_assert(fEndPoint != NULL);

	XBOX::ReleaseRefCountable<VTCPEndPoint>(&fEndPoint);
}

void VEndPointStream::SetTimeOut (sLONG inTimeOut)
{
	xbox_assert(inTimeOut >= 0);

	fTimeOut = inTimeOut;
}

sLONG8 VEndPointStream::DoGetSize ()
{
	return fTotalBytes;
}

XBOX::VError VEndPointStream::DoSetSize (sLONG8 /*inNewSize*/)
{
	return XBOX::VE_OK;
}

XBOX::VError VEndPointStream::DoPutData (const void *inBuffer, VSize inNbBytes)
{
	xbox_assert(fEndPoint != NULL);

	if (inNbBytes) {

		xbox_assert(inBuffer != NULL);

		uLONG			size;
		XBOX::VError	error;
		
		size = (uLONG) inNbBytes;
		error = fEndPoint->WriteWithTimeout((void *) inBuffer, &size, fTimeOut);
		fTotalBytes += size;

		return error;

	} else

		return XBOX::VE_OK;	
}

XBOX::VError VEndPointStream::DoGetData (void *inBuffer, VSize *ioCount)
{
	xbox_assert(fEndPoint != NULL);
	xbox_assert(ioCount != NULL);

	if (*ioCount) {

		xbox_assert(inBuffer != NULL);

		uBYTE			*p, *q;
		uLONG			bytesLeft, size;
		XBOX::VError	error;

		p = q = (uBYTE *) inBuffer;
		bytesLeft = (uLONG) *ioCount;
		do {

			size = bytesLeft;
			error = fEndPoint->ReadWithTimeout(q, &size, fTimeOut);
			q += size;
			fTotalBytes += size;

		} while (error == XBOX::VE_OK && (bytesLeft -= size)); 
		*ioCount = (VSize) (q - p);

		return error;

	} else 

		return XBOX::VE_OK;
}

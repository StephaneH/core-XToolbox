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


#ifndef __SNET_ENDPOINT__
#define __SNET_ENDPOINT__


BEGIN_TOOLBOX_NAMESPACE


class XTOOLBOX_API VEndPoint : public VObject, public IRefCountable
{
	public :
	
	virtual VError Read ( void *outBuff, uLONG *ioLen ) = 0;
	
	virtual VError ReadExactly (
								void *outBuff,
								uLONG inLen,
								sLONG inTimeOutMillis = 0 /* Currently, time-out is supported only for non-blocking, non-select I/O */ ) = 0;
	
	/* bWithEmptyTail is required only on Windows, really. It is used to
	 determine broken socket connection, which actually should be done in the networking code itself. */
	virtual VError Write ( void *inBuff, uLONG *ioLen, bool inWithEmptyTail = false ) = 0;
	
	virtual VError WriteExactly (
								 const void *inBuff,
								 uLONG inLen,
								 sLONG inTimeOutMillis = 0 /* Currently, time-out is supported only for non-blocking, non-select I/O */ ) = 0;
	
	virtual VError Close ( ) = 0;
};


END_TOOLBOX_NAMESPACE


#endif
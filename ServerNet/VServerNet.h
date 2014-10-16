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
#ifndef __SNET_MAIN_HEADER__
#define __SNET_MAIN_HEADER__


#if _WIN32
	#pragma pack( push, 8 )
#else
	#pragma options align = power
#endif


#if VERSIONWIN
	#pragma pack(push,__before,8)
		#include <winsock2.h>
	#pragma pack(pop,__before)
#endif


#include "ServerNet/Sources/ServerNetTypes.h"
#include "ServerNet/Sources/IRequestLogger.h"
#include "ServerNet/Sources/ICriticalError.h"
#include "ServerNet/Sources/VSslDelegate.h"
#include "ServerNet/Sources/VServer.h"
#include "ServerNet/Sources/VServerErrors.h"
#include "ServerNet/Sources/ServiceDiscovery.h"
#include "ServerNet/Sources/SelectIO.h"
#include "ServerNet/Sources/VTCPEndPoint.h"
#include "ServerNet/Sources/VUDPEndPoint.h"
#include "ServerNet/Sources/VWorkerPool.h"
#include "ServerNet/Sources/Tools.h"
#include "ServerNet/Sources/Session.h"
#include "ServerNet/Sources/VNetAddr.h"
#include "ServerNet/Sources/VEndPointStream.h"

/* MIME Message support */
#include "ServerNet/Sources/VNameValueCollection.h"
#include "ServerNet/Sources/VHTTPHeader.h"
#include "ServerNet/Sources/VHTTPCookie.h"
#include "ServerNet/Sources/VMIMEMessagePart.h"
#include "ServerNet/Sources/VMIMEMessage.h"
#include "ServerNet/Sources/VMIMEWriter.h"
#include "ServerNet/Sources/VMIMEReader.h"
#include "ServerNet/Sources/VHTTPMessage.h"

/* Proxy Manager */
#include "ServerNet/Sources/VProxyManager.h"

/* HTTP Client */
#include "ServerNet/Sources/VHTTPClient.h"

#include "ServerNet/Sources/VWebSocket.h"

#include "ServerNet/Sources/CryptoTools.h"

#if _WIN32
	#pragma pack( pop )
#else
	#pragma options align = reset
#endif


#endif

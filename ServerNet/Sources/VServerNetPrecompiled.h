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
#ifndef __VSRVRNET_PCH__
#define __VSRVRNET_PCH__


// Kernel headers
#include "Kernel/VKernel.h"

#include "KernelIPC/VKernelIPC.h"

// Setup export macro
#define XTOOLBOX_EXPORTS
#include "Kernel/Sources/VKernelExport.h"

#if VERSIONWIN
	#include <winsock2.h>
#else
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

#define EXCHANGE_ENDPOINT_ID	0

#if WITH_DEBUGMSG && VERSIONWIN

	#define DEBUG_SAFE_CHECK_SOCK_RESULT(inResult,inSocketError,inMessage,inRawSocket)	((inResult==SOCKET_ERROR && inSocketError!=WSAEWOULDBLOCK) ? XBOX::DebugMsg("%s on socket %d failed with error %d\n", inMessage, inRawSocket, inSocketError) : (void) 0)
	#define DEBUG_CHECK_SOCK_RESULT(inResult,inMessage,inRawSocket)	((inResult==SOCKET_ERROR && WSAGetLastError()!=WSAEWOULDBLOCK) ? XBOX::DebugMsg("%s on socket %d failed with error %d\n", inMessage, inRawSocket, WSAGetLastError()) : (void) 0)
	#define DEBUG_CHECK_RESULT(inResult,inMessage)	((inResult==SOCKET_ERROR && WSAGetLastError()!=WSAEWOULDBLOCK) ? XBOX::DebugMsg("%s failed with error %d\n", inMessage, WSAGetLastError()) : (void) 0)

#else

	#define DEBUG_SAFE_CHECK_SOCK_RESULT(inResult,inSocketError,inMessage,inRawSocket)
	#define DEBUG_CHECK_SOCK_RESULT(inResult,inMessage,inRawSocket)
	#define DEBUG_CHECK_RESULT(inResult,inMessage)

#endif

#if VERSIONMAC && ARCH_64
	#define WITH_DTRACE 1
#else
	#define WITH_DTRACE 0
#endif

#include "VServerErrors.h"

#endif

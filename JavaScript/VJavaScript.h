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
#ifndef __VJAVASCRIPT__
#define __VJAVASCRIPT__


#pragma pack( push, 8 )

// Kernel Headers Required
#include "Kernel/VKernel.h"

// KernelIPC Headers Required
#include "KernelIPC/VKernelIPC.h"

#define JAVASCRIPT_IMPORTS

#include "Sources/VJSErrors.h"
#include "Sources/VJSValue.h"
#include "Sources/VJSContext.h"
#include "Sources/VJSClass.h"
#include "Sources/VJSRuntime_file.h"
#include "Sources/VJSRuntime_stream.h"
#include "Sources/VJSRuntime_progressIndicator.h"
#include "Sources/VJSRuntime_Image.h"
#include "Sources/VJSRuntime_Atomic.h"
#include "Sources/VJSRuntime_blob.h"
#include "Sources/VJSGlobalClass.h"
#if WITH_NATIVE_HTTP_BACKEND
#include "Sources/VXMLHttpRequest.h"
#endif
#include "Sources/VcURLXMLHttpRequest.h"
#include "Sources/VJSProcess.h"

#include "Sources/VSystemWorker.h"
#include "Sources/VJSWorker.h"
#include "Sources/VJSWebStorage.h"
#include "Sources/VJSJSON.h"
#include "Sources/VJSBuffer.h"
#include "Sources/VJSSystemWorker.h"
#include "Sources/VJSNet.h"
#include "Sources/VJSW3CFileSystem.h"
#include "Sources/VJSMIME.h"
#include "Sources/VJSWebSocket.h"

#pragma pack( pop )

#endif

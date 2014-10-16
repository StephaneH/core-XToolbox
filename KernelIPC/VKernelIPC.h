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
#ifndef __VKernelIPC__
#define __VKernelIPC__

#if _WIN32
	#pragma pack( push, 8 )
#else
	#pragma options align = power
#endif

// Kernel Headers Required
#include "Kernel/VKernel.h"

// Types & Macros Headers
#include "KernelIPC/Sources/VKernelIPCFlags.h"
#include "KernelIPC/Sources/VKernelIPCTypes.h"
#include "KernelIPC/Sources/VKernelIPCErrors.h"

// Components Headers
#include "KernelIPC/Sources/CComponent.h"
#include "KernelIPC/Sources/VComponentManager.h"
#include "KernelIPC/Sources/VComponentLibrary.h"
#include "KernelIPC/Sources/VComponentImp.h"
#include "KernelIPC/Sources/CPlugin.h"
#include "KernelIPC/Sources/VPluginImp.h"

// Processes Headers
#include "KernelIPC/Sources/VDaemon.h"
#include "KernelIPC/Sources/VEnvironmentVariables.h"
#include "KernelIPC/Sources/VProcessLauncher.h"
#if WITH_NEW_XTOOLBOX_GETOPT
#include "KernelIPC/Sources/VCommandLineParser.h"
#endif

#if VERSIONWIN
#include "KernelIPC/Sources/XWinShellLaunch.h"
#endif

// Communication Headers
#include "KernelIPC/Sources/VAttachment.h"
#include "KernelIPC/Sources/VDebugAttachments.h"
#include "KernelIPC/Sources/VCommand.h"
#include "KernelIPC/Sources/VSignal.h"
#include "KernelIPC/Sources/ICommandControler.h"

// Server Infrastructure Headers
#include "KernelIPC/Sources/IStreamRequest.h"

// System Notifications
#include "KernelIPC/Sources/VFileSystemNotification.h"
#include "KernelIPC/Sources/VSleepNotifier.h"

// Shared memory
#include "KernelIPC/Sources/VSharedMemory.h"

// Semaphores
#include "KernelIPC/Sources/VSharedSemaphore.h"

#if _WIN32
	#pragma pack( pop )
#else
	#pragma options align = reset
#endif

#endif

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
#ifndef __VGraphicsPrecompiled__
#define __VGraphicsPrecompiled__

// Setup private compile macro
#define COMPILING_XTOOLBOX_LIB

// Kernel headers
#include "KernelIPC/VKernelIPC.h"

// Setup export macro
#define XTOOLBOX_EXPORTS
#include "Kernel/Sources/VKernelExport.h"

// Widely used Graphics headers
#include "VGraphicsFlags.h"
#include "VGraphicsTypes.h"
#include "VGraphicsErrors.h"

#if VERSIONWIN
	#if ENABLE_D2D

	#pragma warning( push )
	#pragma warning (disable: 4263)	//  member function does not override any base class virtual member function
	#pragma warning (disable: 4264)

		#include "GdiPlus.h"
		#include "d2d1.h"
		#include "dwrite.h"
		#pragma comment(lib, "d2d1.lib")
		#pragma comment(lib, "dwrite.lib")

	#pragma warning( pop)
	#endif
#endif

USING_TOOLBOX_NAMESPACE

#endif
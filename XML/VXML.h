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
#ifndef __VXML__
#define __VXML__


#if _WIN32
	#pragma pack( push, 8 )
#else
	#pragma options align = power
#endif

// Kernel Headers Required
#include "Kernel/VKernel.h"
#include "KernelIPC/VKernelIPC.h"

#include "XML/Sources/VXMLErrors.h"
#include "XML/Sources/XMLSaxParser.h"
#include "XML/Sources/XMLForBag.h"

#include "XML/Sources/VLocalizationManager.h"
#include "XML/Sources/VUTIManager.h"
#include "XML/Sources/VUTI.h"
#include "XML/Sources/VPreferences.h"

#include "XML/Sources/XMLSaxWriter.h"
#include "XML/Sources/IXMLDomWrapper.h"
#include "XML/Sources/XMLJsonUtility.h"
#include "XML/Sources/VInfoPlistTools.h"

#include "XML/Sources/VCSSParser.h"

#if _WIN32
	#pragma pack( pop )
#else
	#pragma options align = reset
#endif

#endif

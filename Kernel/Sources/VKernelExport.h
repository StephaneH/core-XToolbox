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
// Toolbox users don't have to worry about this flag. By default it's set to
// automatic import of XToolbox APIs.
//
// Toolbox developpers (Kernel/Graphics/GUI/etc..) should define XTOOLBOX_EXPORTS
// to ensure their APIs are exported.
//
// A typical use of this macro is:
//
// #include "Kernel.h"	// Will import by default
//
// #define XTOOLBOX_EXPORTS
// #include "VKernelExport.h"
//
// #include "HeaderToExport.h"
// #include "OtherHeaderToExport.h"


// Make sure previous flag is cleared
#ifdef XTOOLBOX_API
#undef XTOOLBOX_API
#undef XTOOLBOX_TEMPLATE_API
#endif

// Define new export flag
#ifdef XTOOLBOX_AS_STANDALONE
	#define XTOOLBOX_API
	#define XTOOLBOX_TEMPLATE_API
#else
#ifndef XTOOLBOX_EXPORTS
	#define XTOOLBOX_API IMPORT_API
	#define XTOOLBOX_TEMPLATE_API IMPORT_TEMPLATE_API
#else
	#define XTOOLBOX_API EXPORT_API
	#define XTOOLBOX_TEMPLATE_API EXPORT_TEMPLATE_API
#endif
#endif

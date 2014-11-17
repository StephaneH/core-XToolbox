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
#ifndef __VGraphicsFlags__
#define __VGraphicsFlags__


// Flag to check when reaching QD 16-bit limits
#ifndef DEBUG_QD_LIMITS
	#define DEBUG_QD_LIMITS 0
#endif

// Flag to check when reaching GDI 28-bit limits
#ifndef DEBUG_GDI_LIMITS
	#define DEBUG_GDI_LIMITS 0
#endif

// Flag to enable use of Mac 'Pict' format
#ifndef USE_MAC_PICTS
#define USE_MAC_PICTS	VERSIONMAC
#endif

#define WITH_UNKNOWN_CODEC_PICTURE 1

// Flag to enable use of GDI+ by default on Windows
#ifndef USE_GDIPLUS
	#if VERSIONWIN
		#define USE_GDIPLUS 0
	#else
		#define USE_GDIPLUS 0	
	#endif
#endif

#endif
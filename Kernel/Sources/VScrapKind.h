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
#ifndef XTOOLBOX_AS_STANDALONE

#ifndef __VScrapKind__
#define __VScrapKind__

#include "Kernel/Sources/VString.h"


BEGIN_TOOLBOX_NAMESPACE

// type cross platform
class VString;
typedef const VString	ScrapKind;

// cross platform scrap kind definition
XTOOLBOX_API extern ScrapKind SCRAP_KIND_NULL;
XTOOLBOX_API extern ScrapKind SCRAP_KIND_TEXT_NATIVE;		// native char set
XTOOLBOX_API extern ScrapKind SCRAP_KIND_TEXT_UTF16;		// unicode char set
XTOOLBOX_API extern ScrapKind SCRAP_KIND_TEXT_RTF;
XTOOLBOX_API extern ScrapKind SCRAP_KIND_PICTURE_PICT;		// mac pict format
// sc 24/01/2008, attention, SCRAP_KIND_PICTURE_PNG = "com.4d.private.picture.pgn" dans les versions anterieures a la v11.2
XTOOLBOX_API extern ScrapKind SCRAP_KIND_PICTURE_PNG;		// office scrap
XTOOLBOX_API extern ScrapKind SCRAP_KIND_PICTURE_GIF;		// office scrap
XTOOLBOX_API extern ScrapKind SCRAP_KIND_PICTURE_JFIF;		// office scrap	
XTOOLBOX_API extern ScrapKind SCRAP_KIND_PICTURE_EMF;
XTOOLBOX_API extern ScrapKind SCRAP_KIND_PICTURE_BITMAP;
XTOOLBOX_API extern ScrapKind SCRAP_KIND_PICTURE_TIFF;
XTOOLBOX_API extern ScrapKind SCRAP_KIND_PICTURE_4DPICTURE;
XTOOLBOX_API extern ScrapKind SCRAP_KIND_PICTURE_4DPICTURE_PTR;
XTOOLBOX_API extern ScrapKind SCRAP_KIND_PICTURE_PDF;
XTOOLBOX_API extern ScrapKind SCRAP_KIND_PICTURE_SVG;
XTOOLBOX_API extern ScrapKind SCRAP_KIND_FILE_URL;
XTOOLBOX_API extern ScrapKind SCRAP_KIND_4DPID;

XTOOLBOX_API extern ScrapKind SCRAP_KIND_SPAN;
XTOOLBOX_API extern ScrapKind SCRAP_KIND_HTML;
XTOOLBOX_API extern ScrapKind SCRAP_KIND_XHTML;			//legacy xhtml
XTOOLBOX_API extern ScrapKind SCRAP_KIND_XHTML4D;		//xhtml document with 4D namespace (private format for serializing VDocText)

// OsType reconnu sous Windows
XTOOLBOX_API extern ScrapKind OsType_text;
XTOOLBOX_API extern ScrapKind OsType_utf16;
XTOOLBOX_API extern ScrapKind OsType_rtf;
XTOOLBOX_API extern ScrapKind OsType_pict;

END_TOOLBOX_NAMESPACE

#endif

#endif
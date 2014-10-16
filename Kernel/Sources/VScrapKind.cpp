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

#include "VKernelPrecompiled.h"
#include "VScrapKind.h"


#ifndef XTOOLBOX_AS_STANDALONE

BEGIN_TOOLBOX_NAMESPACE

// cross platform scrap kind definition
ScrapKind SCRAP_KIND_NULL						= "\0";
ScrapKind SCRAP_KIND_TEXT_NATIVE				= "com.4d.private.text.native";		// native char set
ScrapKind SCRAP_KIND_TEXT_UTF16					= "com.4d.private.text.utf16";		// unicode char set
ScrapKind SCRAP_KIND_TEXT_RTF					= "com.4d.private.text.rtf";
ScrapKind SCRAP_KIND_PICTURE_PICT				= "com.4d.private.picture.pict";	// mac pict format
// sc 24/01/2008, attention, SCRAP_KIND_PICTURE_PNG = "com.4d.private.picture.pgn" dans les versions anterieures a la v11.2
ScrapKind SCRAP_KIND_PICTURE_PNG				= "com.4d.private.picture.png";		// office scrap
ScrapKind SCRAP_KIND_PICTURE_GIF				= "com.4d.private.picture.gif";		// office scrap
ScrapKind SCRAP_KIND_PICTURE_JFIF				= "com.4d.private.picture.jfif";	// office scrap	
ScrapKind SCRAP_KIND_PICTURE_EMF				= "com.4d.private.picture.emf";
ScrapKind SCRAP_KIND_PICTURE_BITMAP				= "com.4d.private.picture.bitmap";
ScrapKind SCRAP_KIND_PICTURE_TIFF				= "com.4d.private.picture.tiff";
ScrapKind SCRAP_KIND_PICTURE_4DPICTURE			= "com.4d.private.picture.4dpicture";
ScrapKind SCRAP_KIND_PICTURE_4DPICTURE_PTR		= "com.4d.private.picture.4dpicture_ptr";
ScrapKind SCRAP_KIND_PICTURE_PDF				= "com.4d.private.picture.pdf";
ScrapKind SCRAP_KIND_PICTURE_SVG				= "com.4d.private.picture.svg";
ScrapKind SCRAP_KIND_FILE_URL					= "com.4d.private.file.url";
ScrapKind SCRAP_KIND_4DPID						= "com.4d.private.pid";

ScrapKind SCRAP_KIND_SPAN						= "com.4d.private.text.span";
ScrapKind SCRAP_KIND_HTML						= "com.4d.private.text.html";
ScrapKind SCRAP_KIND_XHTML						= "com.4d.private.text.xhtml";		//legacy xhtml
ScrapKind SCRAP_KIND_XHTML4D					= "com.4d.private.text.xhtml4D";	//xhtml document with 4D namespace (private format for serializing VDocText)

// OsType reconnu sous Windows
ScrapKind OsType_text		= "TEXT";
ScrapKind OsType_utf16		= "utxt";
ScrapKind OsType_rtf		= "RTF ";
ScrapKind OsType_pict		= "PICT";

END_TOOLBOX_NAMESPACE

#endif
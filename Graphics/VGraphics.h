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
#ifndef __VGraphics__
#define __VGraphics__

#if _WIN32 || __GNUC__
#pragma pack(push,8)
#else
#pragma options align = power 
#endif


#if VERSION_LINUX
//jmo - Temporary stuff : Include minimal header set on Linux

#include "Graphics/Sources/VGraphicsTypes.h"
#include "Graphics/Sources/VPoint.h"
#include "Graphics/Sources/VRect.h"
#include "Graphics/Sources/VPolygon.h"
#include "Graphics/Sources/VColor.h"
#include "Graphics/Sources/VAffineTransform.h"
#include "Graphics/Sources/V4DPictureTools.h"
#include "Graphics/Sources/V4DPictureDecoder.h"
#include "Graphics/Sources/V4DPictureData.h"
#include "Graphics/Sources/V4DPictureDataSource.h"

#include "Graphics/Sources/V4DPicture.h"

#else

// Kernel Headers Required
#include "KernelIPC/VKernelIPC.h"

// Types & Macros Headers
#include "Graphics/Sources/VGraphicsFlags.h"
#include "Graphics/Sources/VGraphicsTypes.h"
#include "Graphics/Sources/VGraphicsErrors.h"



// Drawing Contexts
#include "Graphics/Sources/VGraphicContext.h"
#include "Graphics/Sources/VGraphicSettings.h"
#include "Graphics/Sources/VColor.h"
#include "Graphics/Sources/VPattern.h"

#if VERSIONMAC
#include "Graphics/Sources/XMacQuartzGraphicContext.h"
#endif

#if VERSIONWIN
#include "Graphics/Sources/XWinGDIGraphicContext.h"
#include "Graphics/Sources/XWinGDIPlusGraphicContext.h"
//NDJQ: please DO NOT include XWinD2DGraphicContext.h here (see VGraphicsTypes.h)
#endif



// Shapes
#include "Graphics/Sources/IBoundedShape.h"
#include "Graphics/Sources/VPoint.h"
#include "Graphics/Sources/VRect.h"
#include "Graphics/Sources/VPolygon.h"
#include "Graphics/Sources/VRegion.h"
#include "Graphics/Sources/VBezier.h"
#include "Graphics/Sources/VMatrix.h"
#include "Graphics/Sources/VAffineTransform.h"


// Imaging

#include "Graphics/Sources/V4DPictureTools.h"
#include "Graphics/Sources/V4DPicture.h"
#include "Graphics/Sources/V4DPictureDataSource.h"
#include "Graphics/Sources/V4DPictureData.h"
#if VERSIONWIN
	#include "Graphics/Sources/XWinPictureData.h"
#else
	#include "Graphics/Sources/XMacPictureData.h"
#endif
#include "Graphics/Sources/V4DPictureDecoder.h"
#include "Graphics/Sources/V4DPictureProxyCache.h"

#include "Graphics/Sources/ImageMeta.h"

#include "Graphics/Sources/VIcon.h"

// Text
#include "Graphics/Sources/VFontMgr.h"
#include "Graphics/Sources/VFont.h"
#include "Graphics/Sources/VStyledTextBox.h"
#include "Graphics/Sources/VTextLayout.h"

#endif  //(Postponed Linux Implementation !)


#if _WIN32 || __GNUC__
	#pragma pack( pop )
#else
	#pragma options align = reset
#endif

#endif

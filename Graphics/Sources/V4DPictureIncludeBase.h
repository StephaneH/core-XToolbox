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
#ifndef __V4DPICTUREBASE__
#define __V4DPICTUREBASE__

#include "Kernel/VKernel.h"

#include "VPoint.h"
#include "VRect.h"
#if !VERSION_LINUX
#include "VPolygon.h"
#endif
#include "VAffineTransform.h"
#include "VColor.h"

#if ! VERSION_LINUX
	#include "VGraphicContext.h"
#endif

#include "V4DPictureTools.h"
#include "V4DPictureDecoder.h"
#include "V4DPictureDataSource.h"
#include "V4DPictureData.h"
#include "V4DPictureData_QDPicture.h"

#if VERSIONWIN
	#include "XWinPictureData.h"
#elif VERSIONMAC
	#include "XMacPictureData.h"
#endif

#include "V4DPicture.h"

#endif



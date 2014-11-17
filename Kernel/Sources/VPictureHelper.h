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
#ifndef __IPictureTools__
#define __IPictureTools__

#include "Kernel/Sources/VKernelTypes.h"
#include "Kernel/Sources/VValue.h"
#include "Kernel/Sources/VBlob.h"

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API IPictureHelper
{
	public:
	
	virtual VError IPictureHelper_ExtractBestPictureForWeb(const VValue& inValue,VStream& inOutputStream,const VString& inPreferedTypeList,bool inIgnoreTransform,VString& outMime,sLONG* outWidth=NULL,sLONG* outHeight=NULL)=0;
	virtual VError IPictureHelper_GetVPictureInfo(const VValue& inValue,VString& outMimes,sLONG* outWidth=NULL,sLONG* outHeight=NULL)=0;
	
	virtual VError IPictureHelper_FullExportPicture_GetOutputFileKind(const VValue& inValue,VString& outFileExtension)=0;
	virtual VError IPictureHelper_FullExportPicture_Export(const VValue& inValue,VStream& outPutStream)=0;
	
	virtual VError IPictureHelper_ReadPictureFromStream(VValue& ioValue,VStream& inStream)=0;

	virtual bool IPictureHelper_IsPictureEmpty(VValue* ioValue)=0;

};

XTOOLBOX_API void RegisterPictureHelper(IPictureHelper* inHelper);

XTOOLBOX_API VError ExtractBestPictureForWeb(const VValue& inValue,VStream& inOutputStream,const VString& inPreferedTypeList,bool inIgnoreTransform,VString& outMime,sLONG* outWidth=NULL,sLONG* outHeight=NULL);
XTOOLBOX_API VError GetVPictureInfo(const VValue& inValue,VString& outMimes,sLONG* outWidth=NULL,sLONG* outHeight=NULL);

XTOOLBOX_API VError FullExportPicture_GetOutputFileKind(const VValue& inValue,VString& outFileExtension);
XTOOLBOX_API VError FullExportPicture_Export(const VValue& inValue,VStream& outPutStream);
XTOOLBOX_API VError ReadPictureFromStream(VValue& ioValue,VStream& inStream);

XTOOLBOX_API bool IsPictureEmpty(VValue* ioValue);

END_TOOLBOX_NAMESPACE


#endif

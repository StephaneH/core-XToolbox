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
#include "VPictureHelper.h"

BEGIN_TOOLBOX_NAMESPACE

static IPictureHelper* sHelper=0;

void RegisterPictureHelper(IPictureHelper* inHelper)
{
	sHelper=inHelper;
}

VError GetVPictureInfo(const VValue& inValue,VString& outMimes,sLONG* outWidth,sLONG* outHeight)
{
	VError result=VE_UNIMPLEMENTED;
	if(sHelper)
	{
		result = sHelper->IPictureHelper_GetVPictureInfo(inValue,outMimes,outWidth,outHeight);
	}
	return result;
}

VError ExtractBestPictureForWeb(const VValue& inValue,VStream& inOutputStream,const VString& inPreferedTypeList,bool inIgnoreTransform,VString& outMime,sLONG* outWidth,sLONG* outHeight)
{
	VError result=VE_UNIMPLEMENTED;
	if(sHelper)
	{
		result = sHelper->IPictureHelper_ExtractBestPictureForWeb(inValue,inOutputStream,inPreferedTypeList,inIgnoreTransform,outMime,outWidth,outHeight);
	}
	return result;
}


XTOOLBOX_API VError FullExportPicture_GetOutputFileKind(const VValue& inValue,VString& outFileExtension)
{
	VError result=VE_UNIMPLEMENTED;
	if(sHelper)
	{
		result = sHelper->IPictureHelper_FullExportPicture_GetOutputFileKind(inValue,outFileExtension);
	}
	return result;
}
XTOOLBOX_API VError FullExportPicture_Export(const VValue& inValue,VStream& outPutStream)
{
	VError result=VE_UNIMPLEMENTED;
	if(sHelper)
	{
		result = sHelper->IPictureHelper_FullExportPicture_Export(inValue,outPutStream);
	}
	return result;
}

XTOOLBOX_API VError ReadPictureFromStream(VValue& ioValue,VStream& inStream)
{
	VError result=VE_UNIMPLEMENTED;
	if(sHelper)
	{
		result = sHelper->IPictureHelper_ReadPictureFromStream(ioValue,inStream);
	}
	return result;
}


XTOOLBOX_API bool IsPictureEmpty(VValue* ioValue)
{
	if (ioValue != NULL)
	{
		if(sHelper)
		{
			return sHelper->IPictureHelper_IsPictureEmpty(ioValue);
		}
		else 
			return false;
	}
	else
		return true;
}


END_TOOLBOX_NAMESPACE



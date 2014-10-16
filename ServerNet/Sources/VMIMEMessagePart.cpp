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
#include "VServerNetPrecompiled.h"
#include "VMIMEMessagePart.h"
#include "HTTPTools.h"


BEGIN_TOOLBOX_NAMESPACE
USING_TOOLBOX_NAMESPACE


VMIMEMessagePart::VMIMEMessagePart()
: fName()
, fFileName()
, fMediaType()
, fMediaTypeKind (MIMETYPE_UNDEFINED)
, fMediaTypeCharSet (XBOX::VTC_UNKNOWN)
, fStream()
, fID()
, fIsInline(false)
{
}

VMIMEMessagePart::~VMIMEMessagePart()
{
	fStream.Clear();
}


XBOX::VError VMIMEMessagePart::SetData (void *inDataPtr, XBOX::VSize inSize)
{
	if ((NULL == inDataPtr) || (0 == inSize))
		return XBOX::VE_INVALID_PARAMETER;


	fStream.SetDataPtr (inDataPtr, inSize);

	return XBOX::VE_OK;
}


XBOX::VError VMIMEMessagePart::PutData (void *inDataPtr, XBOX::VSize inSize)
{
	if ((NULL == inDataPtr) || (0 == inSize))
		return XBOX::VE_INVALID_PARAMETER;

	XBOX::VError error = XBOX::VE_OK;

	if (XBOX::VE_OK == (error = fStream.OpenWriting()))
		error = fStream.PutData (inDataPtr, inSize);
	fStream.CloseWriting();

	return error;
}


void VMIMEMessagePart::SetMediaType (const XBOX::VString& inMediaType)
{
	HTTPTools::ExtractContentTypeAndCharset (inMediaType, fMediaType, fMediaTypeCharSet);
	fMediaTypeKind = HTTPTools::GetMimeTypeKind (fMediaType);
}


END_TOOLBOX_NAMESPACE

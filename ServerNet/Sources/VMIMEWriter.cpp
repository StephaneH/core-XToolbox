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
#include "VMIMEWriter.h"

BEGIN_TOOLBOX_NAMESPACE
USING_TOOLBOX_NAMESPACE


VMIMEWriter::VMIMEWriter (const XBOX::VString& inBoundary)
: fMIMEMessage()
{
	if (inBoundary.IsEmpty())
	{
		XBOX::VUUID		uuid (true);
		uuid.GetString (fMIMEMessage.fBoundary);
	}
	else
	{
		fMIMEMessage.fBoundary.FromString (inBoundary);
	}
}


VMIMEWriter::~VMIMEWriter()
{
}


const XBOX::VString& VMIMEWriter::GetBoundary() const
{
	return fMIMEMessage.fBoundary;
}


const XBOX::VMIMEMessage& VMIMEWriter::GetMIMEMessage() const
{
	return fMIMEMessage;
}

void VMIMEWriter::AddTextPart (const XBOX::VString& inName, bool inIsInline, const XBOX::VString& inMIMEType, const XBOX::VString& inContentID, XBOX::VPtrStream& inStream)
{
	fMIMEMessage._AddTextPart(inName, inIsInline, inMIMEType, inContentID, inStream);
}

void VMIMEWriter::AddFilePart (const XBOX::VString& inName, const XBOX::VString& inFileName, bool inIsInline, const XBOX::VString& inMIMEType, const XBOX::VString& inContentID, XBOX::VPtrStream& inStream)
{
	fMIMEMessage._AddFilePart(inName, inFileName, inIsInline, inMIMEType, inContentID, inStream);
}


END_TOOLBOX_NAMESPACE

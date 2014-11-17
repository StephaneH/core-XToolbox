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
#include "VKernelIPCPrecompiled.h"
#include "VDebugAttachments.h"


VBeepAttachment::VBeepAttachment(OsType inEventType, Boolean inExecuteHost)
	: VAttachment(inEventType, inExecuteHost)
{
}


void VBeepAttachment::DoExecute(OsType /*inEventType*/, const void* /*inData*/)
{
	VSystem::Beep();
}


#pragma mark-

VDebugMsgAttachment::VDebugMsgAttachment(const VString& inDebugString, OsType inEventType, Boolean inExecuteHost)
	: VAttachment(inEventType, inExecuteHost)
{
	fDebugString = new VString(inDebugString);
}


VDebugMsgAttachment::~VDebugMsgAttachment()
{
	if (fDebugString != NULL)
		delete fDebugString;
}


void VDebugMsgAttachment::GetParameter(VString& outDebugString) const
{
	outDebugString = (fDebugString != NULL) ? *fDebugString : L"";
}


void VDebugMsgAttachment::DoExecute(OsType /*inEventType*/, const void* /*inData*/)
{
	if (fDebugString != NULL)
		DebugMsg(*fDebugString);
}


CharSet VDebugMsgAttachment::DebugDump(char* inTextBuffer, sLONG& ioBufferSize) const
{
	VStr31	dump;
	sLONG	savedBufferSize = ioBufferSize;
	CharSet	charSet = VAttachment::DebugDump(inTextBuffer, ioBufferSize);
	dump.FromBlock(inTextBuffer, ioBufferSize, charSet);
	
	if (fDebugString != NULL)
		dump.AppendPrintf(" string: '%S'", &fDebugString);
	
	ioBufferSize = savedBufferSize;
	return dump.DebugDump(inTextBuffer, ioBufferSize);
}

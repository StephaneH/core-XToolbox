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
#ifndef __VDebugAttachments__
#define __VDebugAttachments__

#include "KernelIPC/Sources/VAttachment.h"

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API VBeepAttachment : public VAttachment
{
public:
		VBeepAttachment (OsType inEventType = EVT_ANY_EVENT, Boolean inExecuteHost = true);
		
	virtual OsType	GetAttachmentType () const { return ACT_BEEP; };

protected:
	virtual void	DoExecute (OsType inEventType, const void* inData);
};


class XTOOLBOX_API VDebugMsgAttachment : public VAttachment
{
public:
			VDebugMsgAttachment (const VString &inDebugString, OsType inEventType = EVT_ANY_EVENT, Boolean inExecuteHost = true);
	virtual	~VDebugMsgAttachment ();
	
	// Accessors
	virtual OsType	GetAttachmentType () const { return ACT_DEBUG_STRING; };
	virtual void	GetParameter (VString& outDebugString) const;
	
	// Inherited from VObject
	virtual	CharSet DebugDump (char* inTextBuffer, sLONG& inBufferSize) const;

protected:
	virtual void	DoExecute (OsType inEventType, const void* inData);
	
private:
	VString*	fDebugString;
};

END_TOOLBOX_NAMESPACE


#endif
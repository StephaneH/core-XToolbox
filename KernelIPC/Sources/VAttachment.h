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
#ifndef __VAttachment__
#define __VAttachment__

#include "Kernel/VKernel.h"

BEGIN_TOOLBOX_NAMESPACE

// Defined above (nested definitions)
class VAttachment;
class IAttachable;


// Event types as returned by VEvent. Usefull for debugging purpose
// and/or handling Attachments.
const OsType	EVT_NONE			= 0;
const OsType	EVT_ANY_TYPE		= '****';	// Generic attachment may be of any type
const OsType	EVT_ANY_EVENT		= '*Evt';	// Those attachments get a VEvent* as data

// Attachment types
const OsType	ACT_UNDEFINED			= '***A';
const OsType	ACT_COMMAND				= 'cmdA';
const OsType	ACT_BEEP				= 'bipA';
const OsType	ACT_DEBUG_STRING		= 'dbgA';


class XTOOLBOX_API IAttachable
{
public:
			IAttachable ();
	virtual	~IAttachable ();
	
	// Attachment adding/removing
	// CAUTION: if you override RemoveAttachment you must call ReleaseAllAttachments
	// before delete (e.g during PrepareDispose)
	virtual void	AddAttachment (VAttachment* inAttachment, VAttachment* inBefore = NULL, Boolean inOwnsAttachment = true);
	virtual void	RemoveAttachment (VAttachment* inAttachment);
	
	// Attachment management
	Boolean	ExecuteAttachments (OsType inEventType, const void* inData) const;
	void	ReleaseAllAttachments ();
	void	ReleaseAllAttachmentsOfType (OsType inType);
	
	// Attachment iteration
	VIndex	GetAttachmentCount () const;
	VIndex	GetAttachmentCountOfType (OsType inType) const;
	VAttachment*	GetAttachment (VIndex inNth) const;
	VAttachment*	GetAttachmentOfType (VIndex inNth, OsType inType) const;	// inNth is any index between 1 and GetAttachmentCountOfType
	
	// Debug utility
	virtual	void	DebugDumpAttachments (VString& ioDump, sLONG inIndentLevel) const;
	
	// Default attachable managment
	static IAttachable*	GetDefaultAttachable () { return sDefaultAttachable; };
	static void			SetDefaultAttachable (IAttachable* inDefault);

protected:
friend class VAttachment;

	VArrayOf<VAttachment*>* fAttachments;
	static IAttachable*	sDefaultAttachable;
};


class XTOOLBOX_API VAttachment : public VObject
{
public:
			VAttachment (OsType inEventType = EVT_ANY_EVENT, Boolean inExecuteHost = true);
			VAttachment (const VAttachment& inOriginal);
	virtual	~VAttachment ();
	
	// Attachment calling
	virtual Boolean	Execute (OsType inEventType, const void* inData);
	
	// Owning management
	virtual void	SetOwner (IAttachable* inNewOwner);
	IAttachable*	GetOwner () const { return fOwner; };
	
	// Host action masking
	virtual void	SetExecuteHost (Boolean inExecute) { fExecuteHost = inExecute; };
	Boolean			GetExecuteHost () const { return fExecuteHost; };
	
	// Accessors
	virtual OsType	GetAttachmentType () const { return ACT_UNDEFINED; };
	OsType	GetEventType () const { return fEventType; };
	
	// Inherited from VObject
	virtual	CharSet DebugDump (char* inTextBuffer, sLONG& inBufferSize) const;

protected:
friend class IAttachable;
	
	OsType			fEventType;
	IAttachable*	fOwner;
	Boolean			fExecuteHost;
	
	// Attachment implementation - You _must_ override
	virtual void	DoExecute (OsType inEventType, const void* inData) = 0;
};


class XTOOLBOX_API IEventProvider
{
public:
			IEventProvider ();
	virtual	~IEventProvider ();
	
	// Event description support
	virtual VIndex	GetEventCount () const;
	virtual void	GetNthEventName (VIndex inIndex, VString& outName) const;
	virtual OsType	GetNthEventType (VIndex inIndex) const;
	
	virtual Boolean	IsNthEventFilterable (VIndex inIndex) const;
	virtual Boolean	IsNthEventEnabled (VIndex inIndex) const;
	virtual Boolean IsNthEventHiLevel (VIndex inIndex) const;
};

END_TOOLBOX_NAMESPACE

#endif


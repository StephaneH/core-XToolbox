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
#include "VAttachment.h"


// Class definitions
IAttachable*	IAttachable::sDefaultAttachable = NULL;


// ---------------------------------------------------------------------------
//	IAttachable::IAttachable()
// ---------------------------------------------------------------------------
// Default constructor
//
IAttachable::IAttachable()
{
	fAttachments = NULL;
	SetDefaultAttachable(this);
}


// ---------------------------------------------------------------------------
//	IAttachable::~IAttachable()
// ---------------------------------------------------------------------------
// Default destructor
//
IAttachable::~IAttachable()
{
	ReleaseAllAttachments();
	
	if (sDefaultAttachable == this)
		SetDefaultAttachable(NULL);
	
	delete fAttachments;
	fAttachments = NULL;
}


// ---------------------------------------------------------------------------
//	IAttachable::AddAttachment(VAttachment*, VAttachment*, Boolean)
// ---------------------------------------------------------------------------
// Add a connection to connection list. If no before is specified or
// if before is not found, add to end.
// If ownsAttachment is true, the connection will be release by 'this'.
//
void
IAttachable::AddAttachment(VAttachment* inAttachment, VAttachment* inBefore, Boolean inOwnsAttachment)
{
	if (inAttachment == NULL) return;
	
	// Allocate list if needed
	if (fAttachments == NULL)
	{
		fAttachments = new VArrayOf<VAttachment*>;
		if (!testAssert(fAttachments != NULL)) return;
	}
	
	if (inBefore != NULL)
	{
		if (fAttachments->Find(inBefore))
			fAttachments->AddBefore(inAttachment, inBefore);
		else
		{
			assert(false);
			fAttachments->AddTail(inAttachment);
		}
	}
	else
	{
		fAttachments->AddTail(inAttachment);
	}
	
	if (inOwnsAttachment)
		inAttachment->SetOwner(this);
}


// ---------------------------------------------------------------------------
//	IAttachable::RemoveAttachment(VAttachment* inAttachment)
// ---------------------------------------------------------------------------
// On exit the connection will no longer be own by this connectable if it was
// its owner.
//
// CAUTION: if you override RemoveAttachment you must call ReleaseAllAttachments
// before delete (e.g during PrepareDispose)
void
IAttachable::RemoveAttachment(VAttachment* inAttachment)
{
	if (inAttachment != NULL && fAttachments != NULL)
	{
		fAttachments->Delete(inAttachment);
		
		if (inAttachment->GetOwner() == this)
			inAttachment->SetOwner(NULL);
	}
}


// ---------------------------------------------------------------------------
//	IAttachable::ReleaseAllAttachments()
// ---------------------------------------------------------------------------
// If 'this' is currently the owner of some connections, it will release them.
// Called by destructor. You cannot derive this function.
//
void
IAttachable::ReleaseAllAttachments()
{
	if (fAttachments != NULL && fAttachments->GetCount() > 0)
	{
		VArrayIteratorOf<VAttachment*>	iterator(*fAttachments);
		VAttachment*	current;
		
		while ((current = iterator.Previous()) != NULL)
		{
			
			Boolean	isOwner = (current->GetOwner() == this);

			// Remove attachment before deleting to avoid problems
			// with inheritance (see RemoveAttachment for comments)
			RemoveAttachment(current);
			
			if (isOwner)
				delete current;
		}
		
		fAttachments->Destroy();
	}
}


// ---------------------------------------------------------------------------
//	IAttachable::ReleaseAllAttachmentsOfType(OsType inType)
// ---------------------------------------------------------------------------
// Same as ReleaseAllAttachments except that it releases Attachments
// of a specific type.
//
// Note the attachment destructor will call RemoveAttachement.
//
void
IAttachable::ReleaseAllAttachmentsOfType(OsType inType)
{
	if (fAttachments != NULL && fAttachments->GetCount() > 0)
	{
		VArrayIteratorOf<VAttachment*>	iterator(*fAttachments);
		VAttachment*	current;
		
		while ((current = iterator.Previous()) != NULL)
		{
			if (current->GetAttachmentType() == inType)
			{
				if (current->GetOwner() == this)
					delete current;
			}
		}
		
		fAttachments->Destroy();
	}
}


// ---------------------------------------------------------------------------
//	IAttachable::ExecuteAttachments(OsType inEventType, const void* inData) const
// ---------------------------------------------------------------------------
// Call to execute all attached filters. Return false if at least one attachment
// returned false.
//
Boolean
IAttachable::ExecuteAttachments(OsType inEventType, const void* inData) const
{
	Boolean	executeHost = true;

	if (fAttachments != NULL && fAttachments->GetCount() > 0)
	{
		VArrayIteratorOf<VAttachment*>	iterator(*fAttachments);
		VAttachment*	current;
		
		while ((current = iterator.Next()) != NULL)
			executeHost &= current->Execute(inEventType, inData);
	}
	
	return executeHost;
}


// ---------------------------------------------------------------------------
//	IAttachable::SetDefaultAttachable(IAttachable* inDefault)		  [static]
// ---------------------------------------------------------------------------
//
void
IAttachable::SetDefaultAttachable(IAttachable* inDefault)
{
	sDefaultAttachable = inDefault;
}


// ---------------------------------------------------------------------------
//	IAttachable::GetAttachmentCount() const
// ---------------------------------------------------------------------------
// Return attachment count.
//
VIndex IAttachable::GetAttachmentCount() const
{
	return (fAttachments != NULL ? fAttachments->GetCount() : 0);
}


// ---------------------------------------------------------------------------
//	IAttachable::GetAttachmentCountOfType(OsType inType) const
// ---------------------------------------------------------------------------
// Return attachment count of given type.
//
VIndex IAttachable::GetAttachmentCountOfType(OsType inType) const
{
	if (fAttachments == NULL) return 0;
	
	VIndex	count = 0;
	VAttachment*	current;
	VArrayIteratorOf<VAttachment*>	iterator(*fAttachments);
	
	while ((current = iterator.Next()) != NULL)
	{
		if (current->GetAttachmentType() == inType)
			count++;
	}
	
	return count;
}


// ---------------------------------------------------------------------------
//	IAttachable::GetAttachment(VIndex inNth) const
// ---------------------------------------------------------------------------
// Return attachment by a 1-based index
//
VAttachment* IAttachable::GetAttachment(VIndex inNth) const
{
	if (fAttachments == NULL) return NULL;
	
	return fAttachments->GetNth(inNth);
}


// ---------------------------------------------------------------------------
//	IAttachable::GetAttachmentOfType(VIndex inNth) const
// ---------------------------------------------------------------------------
// Return attachment of given type by a 1-based index
//
VAttachment* IAttachable::GetAttachmentOfType(VIndex inNth, OsType inType) const
{
	if (fAttachments == NULL) return NULL;
	
	VIndex	index = 0;
	VAttachment*	current;
	VArrayIteratorOf<VAttachment*>	iterator(*fAttachments);
	
	while (index < inNth && (current = iterator.Next()) != NULL)
	{
		if (current->GetAttachmentType() == inType)
			index++;
	}
	
	return (current != NULL ? current : NULL);
}


// ---------------------------------------------------------------------------
//	IAttachable::DebugDumpAttachments(VString& ioDump, sLONG inIndentLevel) const
// ---------------------------------------------------------------------------
// Debug utility. List all attachments.
//
void IAttachable::DebugDumpAttachments(VString& ioDump, sLONG inIndentLevel) const
{
	if (fAttachments != NULL && fAttachments->GetCount() > 0)
	{
		VStr31	indent;
		for (sLONG index = inIndentLevel; index > 0 ; index--)
			indent += '\t';
		
		VStr31	dump;
		VAttachment*	attachment;
		VArrayIteratorOf<VAttachment*>	iterator(*fAttachments);
		
		ioDump.AppendPrintf("%Sattachments:\n", &indent, &dump);
		
		while ((attachment = iterator.Next()) != NULL)
		{
			sLONG	index = iterator.GetPos();
			((VObject*)attachment)->DebugDump(dump);
			ioDump.AppendPrintf("%S%d) %S\n", &indent, index, &dump);
		}
	}
}


#pragma mark-

// ---------------------------------------------------------------------------
//	VAttachment::VAttachment(OsType inEventType, Boolean inExecuteHost)
// ---------------------------------------------------------------------------
// Default constructor
//
VAttachment::VAttachment(OsType inEventType, Boolean inExecuteHost)
{
	fEventType = inEventType;
	fExecuteHost = inExecuteHost;
	fOwner = NULL;
}


// ---------------------------------------------------------------------------
//	VAttachment::VAttachment(const VAttachment& inOriginal)
// ---------------------------------------------------------------------------
// Default constructor
//
VAttachment::VAttachment(const VAttachment& inOriginal)
{
	fEventType = inOriginal.fEventType;
	fExecuteHost = inOriginal.fExecuteHost;
	fOwner = inOriginal.fOwner;
}


// ---------------------------------------------------------------------------
//	VAttachment::~VAttachment()
// ---------------------------------------------------------------------------
// Default destructor
//
VAttachment::~VAttachment()
{
	if (fOwner != NULL)
		fOwner->RemoveAttachment(this);
}


// ---------------------------------------------------------------------------
//	VAttachment::Execute(OsType inEventType, const void* inData)
// ---------------------------------------------------------------------------
// Execute the attachment if the message concern itself. Return the current
// value of fExecuteHost
//
Boolean
VAttachment::Execute(OsType inEventType, const void* inData)
{
	Boolean	executeHost = true;
	
	if (inEventType == fEventType || fEventType == EVT_ANY_EVENT)
	{
		DoExecute(inEventType, inData);
		executeHost = fExecuteHost;
	}
	
	return executeHost;
}


// ---------------------------------------------------------------------------
//	VAttachment::DoExecute(OsType inEventType, const void* inData)
// ---------------------------------------------------------------------------
// Override this function to be called each time your attachemnt is concerned
// by a message.
//
// You may change the value of fExecuteHost if you want to overwrite the actual
// value
void
VAttachment::DoExecute(OsType /*inEventType*/, const void* /*inData*/)
{
}


// ---------------------------------------------------------------------------
//	VAttachment::SetOwner(IAttachable* inNewOwner)
// ---------------------------------------------------------------------------
// The owner is responsible of releasing this attachment if it no longer need it
//
void
VAttachment::SetOwner(IAttachable* inNewOwner)
{
	fOwner = inNewOwner;
}


// ---------------------------------------------------------------------------
//	VAttachment::DebugDump(char* inTextBuffer, sLONG& ioBufferSize) const
// ---------------------------------------------------------------------------
// Debug utility. Override to add extra infos.
//
CharSet
VAttachment::DebugDump(char* inTextBuffer, sLONG& ioBufferSize) const
{
	if (!VProcess::IsRunning())
	{
		ioBufferSize = 0;
		return VTC_UTF_16;
	}

	VStr4	typeStr, eventStr;
	typeStr.FromOsType(GetAttachmentType());
	eventStr.FromOsType(GetEventType());
	
	VStr63	dump;
	dump.AppendPrintf("type: '%S', event: '%S'", &typeStr, &eventStr);
		
	return dump.DebugDump(inTextBuffer, ioBufferSize);
}


#pragma mark-

// ---------------------------------------------------------------------------
//	IEventProvider::IEventProvider
// ---------------------------------------------------------------------------
// Default constructor
//
IEventProvider::IEventProvider()
{
}


// ---------------------------------------------------------------------------
//	IEventProvider::~IEventProvider
// ---------------------------------------------------------------------------
// Default destructor
//
IEventProvider::~IEventProvider()
{
}


// ---------------------------------------------------------------------------
//	IEventProvider::GetEventCount const
// ---------------------------------------------------------------------------
//
VIndex
IEventProvider::GetEventCount() const
{
	return 0;
}


// ---------------------------------------------------------------------------
//	IEventProvider::GetNthEventName(VIndex inIndex, VString& outName) const
// ---------------------------------------------------------------------------
// 
void
IEventProvider::GetNthEventName(VIndex /*inIndex*/, VString& outName) const
{
	outName.Clear();
}


// ---------------------------------------------------------------------------
//	IEventProvider::GetNthEventType(VIndex /*inIndex*/) const
// ---------------------------------------------------------------------------
// 
OsType
IEventProvider::GetNthEventType(VIndex /*inIndex*/) const
{
	return 0;
}


// ---------------------------------------------------------------------------
//	IEventProvider::IsNthEventFilterable(VIndex inIndex) const
// ---------------------------------------------------------------------------
// 
Boolean
IEventProvider::IsNthEventFilterable(VIndex /*inIndex*/) const
{
	return true;
}


// ---------------------------------------------------------------------------
//	IEventProvider::IsNthEventEnabled(VIndex inIndex) const
// ---------------------------------------------------------------------------
// 
Boolean
IEventProvider::IsNthEventEnabled(VIndex /*inIndex*/) const
{
	return true;
}


// ---------------------------------------------------------------------------
//	IEventProvider::IsNthEventHiLevel(VIndex inIndex) const
// ---------------------------------------------------------------------------
// 
Boolean
IEventProvider::IsNthEventHiLevel(VIndex /*inIndex*/) const
{
	return false;
}
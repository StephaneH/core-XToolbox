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
#include "VCommand.h"

BEGIN_TOOLBOX_NAMESPACE

ICommandListener*	ICommandListener::sDefaultListener	= NULL;


void ICommandListener::CommandTriggered(VCommand* ioCommand, VCommandContext* ioContext)
{
	if (ExecuteAttachments(kEVT_TRIG_COMMAND, ioContext))
		DoCommandTriggered(ioCommand, ioContext);
}

	
void ICommandListener::CommandValueUpdated(VCommand* ioCommand, VCommandContext* ioContext)
{
	if (ExecuteAttachments(kEVT_UPDATE_COMMAND, ioContext))
		DoCommandValueUpdated(ioCommand, ioContext);
}


void ICommandListener::DoCommandTriggered(VCommand* /*ioCommand*/, VCommandContext* /*ioContext*/)
{
}


void ICommandListener::DoOrphanCommandTriggered(VCommand* /*ioCommand*/, VCommandContext* /*ioContext*/)
{
}


Boolean ICommandListener::DoCommandTriggeredOnFocus(VCommand* /*ioCommand*/, VCommandContext* /*ioContext*/)
{
	return false;
}


void ICommandListener::DoCommandEnabled(VCommand* /*ioCommand*/, Boolean /*inIsEnabled*/)
{
}


void ICommandListener::DoCommandValueUpdated(VCommand* /*ioCommand*/, VCommandContext* /*ioContext*/)
{
}


void ICommandListener::DoCommandConnected(VCommand* /*ioCommand*/)
{
}


void ICommandListener::DoCommandDisconnected(VCommand* /*ioCommand*/)
{
}


void ICommandListener::DoCommandAskDisconnect(VCommand* ioCommand)
{
	// Default behaviour is to disconnect (will genrate DoCommandDisconnected notification)
	if (ioCommand != NULL)
		ioCommand->DisconnectListener(this);
}


#pragma mark-

VCommandContext::VCommandContext(VCommand* inCommand, const VValueBag* inBag, const ICommandListener* inExcept)
{
	assert(inCommand != NULL);
	
	fCommand = inCommand;
	fBag = inBag;
	fExcept = inExcept;
	fNeedsToForwardCommandToNextFocusable = true;

	if (fBag != NULL)
		fBag->Retain();
	
	fCommand->Retain();
}


VCommandContext::~VCommandContext()
{
	if (fBag != NULL)
		fBag->Release();
		
	fCommand->Release();
}


VCommandContext* VCommandContext::Clone() const
{
	return new VCommandContext(fCommand, fBag, fExcept);
}


const VValueSingle* VCommandContext::GetValueSingle() const
{
	return (fBag != NULL) ? fBag->GetAttribute(CVSTR("__command_value__")) : NULL;
}



#pragma mark-

VCommand*	VCommand::sNullCommand = NULL;

VCommand::VCommand(const VString& inID)
{
	fID = inID;
	fName = inID;
	fDefaultValue = NULL;
	fChoiceList = NULL;
	fParentGroup = NULL;
	fFocusRelated = false;
}

VCommand::VCommand(const VString& inID, const VString& inName)
{
	fID = inID;
	fName = inName;
	fDefaultValue = NULL;
	fChoiceList = NULL;
	fParentGroup = NULL;
	fFocusRelated = false;
}


VCommand::VCommand(const VValueBag& inBag)
{
	fDefaultValue = NULL;
	fChoiceList = NULL;
	fParentGroup = NULL;
	fFocusRelated = false;
	
	LoadFromBag(inBag);
}


VCommand::~VCommand()
{
	if (fDefaultValue != NULL)
		delete fDefaultValue;
		
	if (fChoiceList != NULL)
		delete fChoiceList;
		
	if (fParentGroup != NULL)
		fParentGroup->RemoveChild(this);
}


void VCommand::ConnectObserverListener(ICommandListener* inListener)
{
	if (!inListener->IsMessagingEnabled())
		inListener->StartMessaging();
	
	assert(inListener->GetMessagingTask() == VTask::GetCurrentID());	// else we need a VMessage
	inListener->DoCommandConnected(this);
	
	fUpdateSignal.Connect(inListener, inListener, &ICommandListener::CommandValueUpdated);
	fEnableSignal.Connect(inListener, inListener, &ICommandListener::DoCommandEnabled);
	fDisconnectSignal.Connect(inListener, inListener, &ICommandListener::DoCommandAskDisconnect);
}


// This call is synchronous
//
void VCommand::ConnectTriggerListener(ICommandListener* inListener)
{
	if (!inListener->IsMessagingEnabled())
		inListener->StartMessaging();
	
	inListener->DoCommandConnected(this);
	
	fTriggerSignal.Connect(inListener, inListener, &ICommandListener::CommandTriggered);
	fDisconnectSignal.Connect(inListener, inListener, &ICommandListener::DoCommandAskDisconnect);
}


void VCommand::AskListenersToDisconnect()
{
	fDisconnectSignal.TriggerExcept(NULL, this);
}


// This call is synchronous
//
void VCommand::DisconnectListener(ICommandListener* inListener)
{
	inListener->DoCommandDisconnected(this);
	
	const VObject* target = dynamic_cast<const VObject*>(inListener);
	fTriggerSignal.Disconnect(target);
	fUpdateSignal.Disconnect(target);
}


bool VCommand::HasTriggerListener(ICommandListener* /*inListener*/) const
{
	assert(false);	// To be done
	return false;
}


bool VCommand::HasObserverListener(ICommandListener* /*inListener*/) const
{
	assert(false);	// To be done
	return false;
}


bool VCommand::Trigger(const VValueBag* inBagValue, const ICommandListener* inExcept)
{
	// To ensure the command won't dispose while it's used, we retain locally
	Retain();

	VIndex	nbCalls = 0;
	VCommandContext context(this, inBagValue, inExcept);
	
	if (fFocusRelated)
	{
		// Sent to default handler (which generally forward directly to focus)
		if (ICommandListener::sDefaultListener != NULL && ICommandListener::sDefaultListener->DoCommandTriggeredOnFocus(this, &context))
			nbCalls = 1;
	}
	else
	{	
		// Sent to target
		nbCalls = fTriggerSignal.TriggerExcept(dynamic_cast<const VObject*>(inExcept), this, &context);
	}
	
	// If not handled, sent to default handler
	if (nbCalls == 0 && ICommandListener::sDefaultListener != NULL)
		ICommandListener::sDefaultListener->DoOrphanCommandTriggered(this, &context);
	
	Release();
	return nbCalls != 0;
}


bool VCommand::Trigger(const VValueSingle& inValue, const ICommandListener* inExcept)
{
	bool called = false;
	VValueBag* bag = new VValueBag;
	if (bag != NULL)
	{
		bag->SetAttribute(CVSTR("__command_value__"), dynamic_cast<VValueSingle*>(inValue.Clone()));
		
		called = Trigger(bag, inExcept);
		
		bag->Release();
	}
	return called;
}


void VCommand::Trigger(const ICommandListener* inExcept)
{
	if (fDefaultValue != NULL)
		Trigger(*fDefaultValue);
	else
		Trigger((VValueBag*) NULL, inExcept);
}


void VCommand::Trigger(sLONG inChoiceIndex, const ICommandListener* inExcept)
{
	VValueSingle* singleValue = NULL;

	if (fChoiceList != NULL)
	{
		VValue*	value = VValue::NewValueFromValueKind(fChoiceList->GetElemValueKind());
		if (value != NULL)
		{
			singleValue = dynamic_cast<VValueSingle*>(value);
			if (singleValue == NULL)
				delete value;
		}
	}

	if (testAssert(singleValue != NULL))
	{
		fChoiceList->GetValue(*singleValue, inChoiceIndex);
		Trigger(*singleValue, inExcept);
		delete singleValue;
	}
}


void VCommand::UpdateValue(const VValueBag* inBagValue, const ICommandListener* inExcept)
{
	Retain();

	VCommandContext context(this, inBagValue, inExcept);	
	fUpdateSignal.TriggerExcept(dynamic_cast<const VObject*>(inExcept), this, &context);
	
	Release();
}


void VCommand::UpdateValue(const VValueSingle& inValue, const ICommandListener* inExcept)
{
	VValueBag* bag = new VValueBag;
	if (bag != NULL)
	{
		bag->SetAttribute(CVSTR("__command_value__"), dynamic_cast<VValueSingle*>(inValue.Clone()));
		
		UpdateValue(bag, inExcept);

		bag->Release();
	}
}


void VCommand::TriggerOneListener(ICommandListener* inListener, VCommandContext* ioContext)
{
	if (inListener == NULL) return;
	
	VCommandContextDelegate*	delegate = new VCommandContextDelegate(inListener, inListener, &ICommandListener::CommandTriggered);
	SignalOneCommandContextDelegate(delegate, ioContext);
	delegate->Release();
}


void VCommand::UpdateOneListener(ICommandListener* inListener, VCommandContext* ioContext)
{
	if (inListener == NULL) return;
	
	VCommandContextDelegate*	delegate = new VCommandContextDelegate(inListener, inListener, &ICommandListener::CommandValueUpdated);
	SignalOneCommandContextDelegate(delegate, ioContext);
	delegate->Release();
}


void VCommand::SignalOneCommandContextDelegate(VSignalDelegate* inDelegate, VCommandContext* ioContext)
{
	if (inDelegate->GetMessageable() != NULL)
	{
		VSignalMessageT<VCommand*,VCommandContext*,VoidType,VoidType>*	msg = new VSignalMessageT<VCommand*,VCommandContext*,VoidType>(inDelegate, this, ioContext, VoidType(), VoidType());
		msg->SendOrPostTo(inDelegate->GetMessagingContext(), 0, IsAllwaysSynchronous() || IsSynchronousIfSameTask(), IsAllwaysSynchronous());
		msg->Release();
	}
}


void VCommand::SetEnabled(bool inEnabled)
{
	if (IsNull()) return;
	
	fTriggerSignal.SetEnabled(inEnabled);
	fEnableSignal.TriggerExcept(NULL, this, inEnabled);
}


bool VCommand::IsEnabled() const
{
	if (IsNull()) return false;
	return (fTriggerSignal.IsEnabled());
}


void VCommand::SetParentGroup(VCommandGroup* inParent)
{
	assert(inParent != NULL);
	
	if (fParentGroup != NULL)
		fParentGroup->RemoveChild(this);
		
	inParent->_AddChild(this);
}


VCommandGroup* VCommand::RetainParentGroup() const
{
	VCommandGroup*	parent = (fParentGroup != NULL) ? fParentGroup : new VCommandGroup(CVSTR(""));
	
	parent->Retain();
	return parent;
}


void VCommand::SetDefaultValue(VValueSingle* ioValue)
{
	if (IsNull() || fDefaultValue == ioValue) return;
		
	if (fDefaultValue != NULL)
		delete fDefaultValue;
		
	fDefaultValue = ioValue;
}


bool VCommand::IsNull() const
{
	// As this function is non virtual it is safe to call it with
	// a NULL pointer (even if its _VERY BAD_).
	// So we add some 'blindage'.
	
	assert(this != NULL);
	return (this == sNullCommand || this == NULL);
}


VError VCommand::SaveToBag(VValueBag& ioBag, VString& outKind) const
{
	ioBag.SetString(L"id", fID);
	ioBag.SetString(L"name", fName);
	
	if (fTriggerSignal.IsAllwaysAsynchronous())
		ioBag.SetBoolean(L"asynchronous", true);
	else if (fTriggerSignal.IsAllwaysSynchronous())
		ioBag.SetBoolean(L"synchronous", true);
	
	ioBag.SetBoolean(L"enabled", IsEnabled());
	
	outKind = "command";
	return vGetLastError();
}


VError VCommand::LoadFromBag(const VValueBag& inBag)
{
	inBag.GetString(L"id", fID);
	inBag.GetString(L"name", fName);
	
	Boolean	flag;
	if (inBag.GetBoolean(L"asynchronous", flag) && flag)
	{
		fTriggerSignal.SetSignalingMode( VSignal::ESM_Asynchonous);
		fUpdateSignal.SetSignalingMode( VSignal::ESM_Asynchonous);
	}
	else if (inBag.GetBoolean(L"synchronous", flag) && flag)
	{
		fTriggerSignal.SetSignalingMode( VSignal::ESM_Synchonous);
		fUpdateSignal.SetSignalingMode( VSignal::ESM_Synchonous);
	}
	else
	{
		fTriggerSignal.SetSignalingMode( VSignal::ESM_SynchonousIfSameTask);
		fUpdateSignal.SetSignalingMode( VSignal::ESM_SynchonousIfSameTask);
	}
	
	if (inBag.GetBoolean(L"enabled", flag))
		SetEnabled(flag != 0);
	
	if (inBag.GetBoolean(L"focus_related", flag))
		SetFocusRelated(flag != 0);
	
	return vGetLastError();
}


bool VCommand::Init()
{
	if (sNullCommand == NULL)
		sNullCommand = new VCommand(CVSTR(""),CVSTR(""));
			
	return (sNullCommand != NULL);
}


void VCommand::DeInit()
{
	if (sNullCommand != NULL)
	{
		sNullCommand->Release();
		sNullCommand = NULL;
	}
}


CharSet VCommand::DebugDump(char* inTextBuffer, sLONG& inBufferSize) const
{
	return fID.DebugDump(inTextBuffer, inBufferSize);
}


#pragma mark-

VCommandGroup::VCommandGroup(const VString& inID) : inherited(inID, inID)
{
}


VCommandGroup::~VCommandGroup()
{
	VCommand* child;
	VArrayIteratorOf<VCommand*>	iterator(fChildren.GetList());
	
	while ((child = iterator.Next()) != NULL)
		child->_SetParentGroup(NULL);
}


void VCommandGroup::AddChild(VCommand* inChild)
{
	assert(inChild != NULL);
	
	fChildren.AddCommand(inChild);
	inChild->SetParentGroup(this);
}


void VCommandGroup::_AddChild(VCommand* inChild)
{
	fChildren.AddCommand(inChild);
	inChild->_SetParentGroup(this);
}


void VCommandGroup::RemoveChild(VCommand* inChild)
{
	assert(inChild != NULL);
	
	fChildren.RemoveCommand(inChild);
	
	if (inChild->_GetParentGroup() == this)
		inChild->_SetParentGroup(NULL);
}


VCommand* VCommandGroup::RetainNthChild(VIndex inIndex)
{
	return fChildren.RetainNthCommand(inIndex);
}


#pragma mark-

VCommand* const* VArrayOfCommands::DoFindData(VCommand* const* inFirst, VCommand* const* inLast, VCommand* const &inData) const
{
	 while(inFirst != inLast && ((*inFirst)->GetCommandID() != inData->GetCommandID()))
		++inFirst;
	 return inFirst;
}


#pragma mark-

VCommandList::VCommandList(VCommandList* inParent)
{
	if (inParent != NULL)
		inParent->Retain();
	fParent = inParent;
}


VCommandList::VCommandList(const VBagArray& inBag, ICommandListener* inTriggerListener)
{
	fParent = NULL;
	LoadCommands(inBag, inTriggerListener);
}


VCommandList::VCommandList(const VCommandList& inOriginal)
{
	fParent = inOriginal.fParent;
	
	if (fParent != NULL)
		fParent->Retain();

	fCommandKinds = inOriginal.fCommandKinds;
	fCmdArray = inOriginal.fCmdArray;
	fCommandConstructors = inOriginal.fCommandConstructors;
}


VCommandList::~VCommandList()
{
	fCmdArray.Destroy(DESTROY);
	if (fParent != NULL)
		fParent->Release();
}


VCommand*	VCommandList::AddCommand(const VString& inID)
{
	if (inID.IsEmpty()) return VCommand::RetainNullCommand();

	VCommand*	result = RetainCommand(inID, true);
	if (result->IsNull()) {
		result->Release();
		
		result = new VCommand(inID);
		if (result != NULL)
			AddCommand(result);
	}

	return result;
}


VCommand*	VCommandList::AddCommand(const VString& inID, const VString& inName)
{
	if (inID.IsEmpty()) return VCommand::RetainNullCommand();

	VCommand*	result = RetainCommand(inID, true);
	if (result->IsNull()) {
		result->Release();
		
		result = new VCommand(inID, inName);
		if (result != NULL)
			AddCommand(result);
	}

	return result;
}


void VCommandList::AddCommand(VCommand* inCommand)
{
	if (inCommand != NULL) {
		VTaskLock lock(&fCriticalSection);
		inCommand->Retain();
		fCmdArray.AddTail(inCommand);
	}
}


void VCommandList::LoadCommands(const VBagArray& inBag, ICommandListener* inTriggerListener, bool inRemovePrevious)
{
	if (inRemovePrevious)
		RemoveAllCommands();
	
	sLONG	count = inBag.GetCount();
	for(sLONG i = 1 ; i <= count ; ++i) {
		const VValueBag* bag = inBag.RetainNth(i);
		VCommand* com = _LoadOneCommand(*bag, inTriggerListener);
		com->Release();
		bag->Release();
	}
}


VCommand* VCommandList::_LoadOneCommand(const VValueBag& inBag, ICommandListener* inListener) 
{
	VCommand* com = NULL;
	
	VStr255 kind;
	if (inBag.GetString(L"kind", kind)) {
		// Custom kind
		sLONG i = fCommandKinds.Find(kind);
		if (testAssert(i > 0)) {
			CommandConstructorProcPtr constructor = fCommandConstructors.GetNth(i);
			com = constructor(inBag);
		}
	} else {
		// Standard command
		com = new VCommand(inBag);
	}
	
	if (com != NULL) {
		VTaskLock lock(&fCriticalSection);	// for accessing fCmdArray
		if (!com->IsFocusRelated() && inListener != NULL)
			com->ConnectTriggerListener(inListener);
		com->Retain();
		fCmdArray.AddTail(com);
	}
	
	if (com == NULL)
		com = VCommand::RetainNullCommand();
	
	return com;
}


VCommand* VCommandList::RetainNthCommand(VIndex inIndex) const
{
	if (inIndex < 1 || inIndex > fCmdArray.GetCount()) return VCommand::RetainNullCommand();
	
	return fCmdArray.GetNth(inIndex);
}


VCommand* VCommandList::RetainCommand(const VString& inCommandID, bool inGlobalSearch) const
{
	VCommand*	com = _GetCommand(inCommandID, inGlobalSearch);
	if (com != NULL)
		com->Retain();
	else if (inGlobalSearch && fParent != NULL)
		com = fParent->RetainCommand(inCommandID, true);

	if (com == NULL)
		com = VCommand::RetainNullCommand();

	return com;
}


VCommand* VCommandList::_GetCommand(const VString& inCommandID, bool /*inGlobalSearch*/) const
{
	// Assumes there's only one command with this ID
	VTaskLock lock(&fCriticalSection);
	VArrayIteratorOf<VCommand*> iterator(fCmdArray);
	for(VCommand* com = iterator.First() ; com != NULL ; com = iterator.Next())
	{
		if (com->IsID(inCommandID))
			return com;
	}
	
	return NULL;
}


bool VCommandList::HasCommand(const VString& inID, bool inGlobalSearch) const
{
	VCommand*	command = _GetCommand(inID, inGlobalSearch);
	return (command != NULL);
}


bool VCommandList::HasCommand(const VCommand* inCommand) const
{
	VIndex	pos = fCmdArray.FindPos(const_cast<VCommand*>(inCommand));
	return (pos > 0);
}


void VCommandList::RemoveCommand(const VString& inID)
{
	VCommand*	command = _GetCommand(inID, false);
	if (command != NULL)
		RemoveCommand(command);
}


void VCommandList::RemoveCommand(const VCommand* inCommand)
{
	VTaskLock lock(&fCriticalSection);
	fCmdArray.Delete(const_cast<VCommand*>(inCommand));
}


void VCommandList::RemoveAllCommands()
{
	VTaskLock lock(&fCriticalSection);
	fCmdArray.Destroy(DESTROY);
}


bool VCommandList::IsCommandEnabled(const VString& inCommandID, bool inGlobalSearch) const
{
	VCommand* com = RetainCommand(inCommandID, inGlobalSearch);
	bool isEnabled = com->IsEnabled();
	com->Release();
	return isEnabled;
}


void VCommandList::SetParent(VCommandList* inParent)
{
	CopyRefCountable(&fParent, inParent);
}


VCommandList* VCommandList::RetainParent()	const
{
	if (fParent != NULL)
		fParent->Retain();
	return fParent;
}


VError VCommandList::SaveToBag(VValueBag& ioBag, VString& outKind) const
{
	VStr31	kind;
	VCommand*	command;
	VArrayIteratorOf<VCommand*>	iterator(fCmdArray);
	while ((command = iterator.Next()) != NULL)
	{
		VValueBag*	bagCommand = new VValueBag;
		command->SaveToBag(*bagCommand, kind);
		ioBag.AddElement(kind, bagCommand);
		bagCommand->Release();
	}
	
	outKind = "command";
	return vGetLastError();
}


VError VCommandList::LoadFromBag(const VValueBag& inBag)
{
	const VBagArray*	bagCommands = inBag.RetainElements(L"command");
	if (bagCommands != NULL)
	{
		LoadCommands(*bagCommands);
		bagCommands->Release();
	}
	
	return vGetLastError();
}


void VCommandList::ConnectTriggerListener(const VString& inCommandID, ICommandListener* inListener, bool inGlobalSearch)
{
	VCommand* com = RetainCommand(inCommandID, inGlobalSearch);
	com->ConnectTriggerListener(inListener);
	com->Release();
}


void VCommandList::RegisterCommandConstructor(const VString& inKind, CommandConstructorProcPtr inConstructor)
{
	if (testAssert(fCommandKinds.Find(inKind) <= 0)) {
		fCommandKinds.AppendString(inKind);
		fCommandConstructors.AddTail(inConstructor);
	}
}


#pragma mark-

VCommandAttachment::VCommandAttachment(OsType inEventType, VCommand* inCommand) : VAttachment(inEventType)
{
	if (inCommand != NULL)
	{
		inCommand->Retain();
		fCommand = inCommand;
	}
	else
	{
		fCommand = VCommand::RetainNullCommand();
	}
}


VCommandAttachment::VCommandAttachment(OsType inEventType, VCommandList* inList, const VString& inCommandID) : VAttachment(inEventType)
{
	fCommand = (inList != NULL) ? inList->AddCommand(inCommandID, inCommandID) : VCommand::RetainNullCommand();
}


VCommandAttachment::VCommandAttachment(const VCommandAttachment& inOriginal) : VAttachment(inOriginal)
{
	fCommand = inOriginal.fCommand;
	fCommand->Retain();
}


VCommandAttachment::~VCommandAttachment()
{
	fCommand->Release();
}


void VCommandAttachment::SetCommand(VCommand* inCommand)
{
	IRefCountable::Copy(&fCommand, inCommand);
}


void VCommandAttachment::DoExecute(OsType /*inEventType*/, const void* inData)
{
	const VValueSingle* value = dynamic_cast<const VValueSingle*>(reinterpret_cast<const VObject*>(inData));

	if (value == NULL)
		fCommand->Trigger();
	else
		fCommand->Trigger(*value);
}


CharSet VCommandAttachment::DebugDump(char* inTextBuffer, sLONG& ioBufferSize) const
{
	if (!VProcess::IsRunning())
	{
		ioBufferSize = 0;
		return VTC_UTF_16;
	}

	VStr31	dump;
	sLONG	savedBufferSize = ioBufferSize;
	CharSet	charSet = VAttachment::DebugDump(inTextBuffer, ioBufferSize);
	dump.FromBlock(inTextBuffer, ioBufferSize, charSet);
	
	if (!fCommand->IsNull())
		dump.AppendPrintf(" commandID: '%S'", &fCommand->GetCommandID());
	
	ioBufferSize = savedBufferSize;
	return dump.DebugDump(inTextBuffer, ioBufferSize);
}

END_TOOLBOX_NAMESPACE


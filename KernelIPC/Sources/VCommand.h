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
#ifndef __VCommand__
#define __VCommand__

#include "Kernel/VKernel.h"
#include "KernelIPC/Sources/VAttachment.h"
#include "KernelIPC/Sources/VSignal.h"

BEGIN_TOOLBOX_NAMESPACE

// Class defined bellow
class ICommandListener;
class VCommand;
class VCommandGroup;
class VCommandContext;
class VCommandList;


// Needed typedefs
typedef	VCommand* (*CommandConstructorProcPtr) (const VValueBag& inBag);

typedef VSignalDelegateT_2<ICommandListener,void (ICommandListener::*) (VCommand*,VCommandContext*),VCommand*,VCommandContext*> VCommandContextDelegate;
typedef VSignalDelegateT_2<ICommandListener,void (ICommandListener::*) (VCommand*,Boolean),VCommand*,Boolean> VCommandEnabledDelegate;
typedef VSignalT_2<VCommand*,VCommandContext*> VCommandContextSignal;
typedef VSignalT_2<VCommand*,Boolean> VCommandEnabledSignal;
typedef VSignalT_1<VCommand*> VCommandConnectionSignal;


#if VERSIONWIN
#pragma warning (push)
#pragma warning (disable: 4275) // XTOOLBOX_API marche pas pour les templates
#endif

class XTOOLBOX_API VArrayOfCommands : public VArrayRetainedPtrOf<VCommand*>
{
public:
	virtual VCommand* const *DoFindData (VCommand* const *inFirst, VCommand* const *inLast, VCommand* const &inData) const;
};

#if VERSIONWIN
#pragma warning (pop)
#endif


// Needed constants
typedef enum {
	CT_ADDITIVE,
	CT_EXCLUSIVE
} ControlerType;


const OsType	kEVT_TRIG_COMMAND		= 'tgCd';	// Data is a VCommandContext*
const OsType	kEVT_UPDATE_COMMAND		= 'udCd';	// Data is a VCommandContext*
const OsType	kEVT_SEND_MESSAGE		= 'sdMs';	// Data is a VMessage*
const OsType	kEVT_RECEIVE_MESSAGE	= 'rvMs';	// Data is a VMessage*


/*
	ICommandListener is an interface for VCommand listeners
	
	Override ICommandListener::DoCommandUpdate to do 
	what you want when a VCommand value has changed.
	
*/

class XTOOLBOX_API ICommandListener : public virtual IMessageable, public virtual IAttachable
{
public:
	// Command support (call to send an action)
	virtual void	CommandTriggered (VCommand* ioCommand, VCommandContext* ioContext);	
	virtual void	CommandValueUpdated (VCommand* ioCommand, VCommandContext* ioContext);

	// Default listener support (invoked for orphan and focus commands)
	static void	SetDefaultListener (ICommandListener* inListener) { sDefaultListener = inListener; };
	static ICommandListener*	GetDefaultListener () { return sDefaultListener; };
	
protected:
friend class VCommand;
	static ICommandListener*	sDefaultListener;
	
	// Override to get notifications
	virtual void	DoCommandTriggered (VCommand* ioCommand, VCommandContext* ioContext);
	virtual void	DoOrphanCommandTriggered (VCommand* ioCommand, VCommandContext* ioContext);
	virtual Boolean	DoCommandTriggeredOnFocus (VCommand* ioCommand, VCommandContext* ioContext);
	
	virtual void	DoCommandEnabled (VCommand* ioCommand, Boolean inIsEnabled);
	virtual void	DoCommandValueUpdated (VCommand* ioCommand, VCommandContext* ioContext);
	
	virtual void	DoCommandConnected (VCommand* ioCommand);
	virtual void	DoCommandDisconnected (VCommand* ioCommand);
	virtual	void	DoCommandAskDisconnect (VCommand* inCommand);
};


class XTOOLBOX_API VCommand : public VObject, public IRefCountable, public IBaggable, public IAttachable
{
public:
	explicit	VCommand (const VString& inID);
	explicit	VCommand (const VString& inID, const VString& inName);
				VCommand (const VValueBag& inBag);
	virtual 	~VCommand ();
	
	// Connexion handling
	void	ConnectObserverListener (ICommandListener* inListener);
	void	ConnectTriggerListener (ICommandListener* inListener);
	void	DisconnectListener (ICommandListener* inListener);
	void	AskListenersToDisconnect ();

	bool	HasListener (ICommandListener* inListener) const	{ return (HasTriggerListener (inListener) || HasObserverListener (inListener)); };
	bool	HasTriggerListener (ICommandListener* inListener) const;
	bool	HasObserverListener (ICommandListener* inListener) const;

	// Trigger with default single value cloned
	void	Trigger (const ICommandListener* inExcept = NULL);

	// Trigger with retained bag
	bool	Trigger (const VValueBag* inBagValue, const ICommandListener* inExcept = NULL);
	void	UpdateValue (const VValueBag* inBagValue, const ICommandListener* inExcept = NULL);

	// Trigger with cloned single value
	bool	Trigger (const VValueSingle& inValue, const ICommandListener* inExcept = NULL);
	void	UpdateValue (const VValueSingle& inValue, const ICommandListener* inExcept = NULL);
	
	// Trigger from choice list
	void	Trigger (sLONG inChoiceIndex, const ICommandListener* inExcept = NULL);

	void	TriggerOneListener (ICommandListener* inListener, VCommandContext* ioContext);	// Not necessarly connected
	void	UpdateOneListener (ICommandListener* inListener, VCommandContext* ioContext);	// Not necessarly connected
	void	SignalOneCommandContextDelegate (VSignalDelegate* inDelegate, VCommandContext* ioContext);

	void	SetEnabled (bool inSet);
	bool	IsEnabled () const;
	
	// Accessors
	const VString&	GetCommandID () const { return fID; };
	const VString&	GetName () const { return fName; };	// CAUTION: Will become obsolete

	void	SetParentGroup (VCommandGroup* inParent);
	VCommandGroup*	RetainParentGroup () const;

	void 	SetDefaultValue (VValueSingle* ioValue);
	VValueSingle*	GetDefaultValue () const { return fDefaultValue; };

	void	SetFocusRelated (bool inSet) { fFocusRelated = inSet; };
	bool	IsFocusRelated () { return fFocusRelated; };

	// Choice List
	void	SetChoiceList (VArrayValue* inChoiceList) { if (fChoiceList != inChoiceList) delete fChoiceList; fChoiceList = inChoiceList; };
	const VArrayValue*	GetChoiceList () const { return fChoiceList; };

	// Helpers
	bool	IsID (const VString &inID) const { return fID.EqualToString (inID,false) != 0; };
	bool	IsAllwaysAsynchronous () const { return fTriggerSignal.IsAllwaysAsynchronous (); };
	bool	IsAllwaysSynchronous () const { return fTriggerSignal.IsAllwaysSynchronous (); };
	bool	IsSynchronousIfSameTask () const { return fTriggerSignal.IsSynchronousIfSameTask (); };
	bool	IsNull () const;
	
	// Utilities
	virtual	CharSet	DebugDump (char* inTextBuffer, sLONG& inBufferSize) const;
	
	// Inherited from IBaggable
	virtual	VError	LoadFromBag (const VValueBag& inBag);
	virtual	VError	SaveToBag (VValueBag& ioBag, VString& outKind) const;
	
	// Class utilities
	static	bool	Init ();
	static	void	DeInit ();

	// Null command support
	static	VCommand*	GetNullCommand () { assert (sNullCommand != NULL); return sNullCommand; };
	static	VCommand*	RetainNullCommand () { assert (sNullCommand != NULL); sNullCommand->Retain (); return sNullCommand; };

protected:
friend	class VCommandGroup;
	VString		fID;
	VString		fName;
	VValueSingle*	fDefaultValue;
	bool			fFocusRelated;
	VCommandGroup*	fParentGroup;
	VArrayValue*	fChoiceList;
	VCommandContextSignal	fTriggerSignal;
	VCommandContextSignal	fUpdateSignal;
	VCommandEnabledSignal	fEnableSignal;
	VCommandConnectionSignal	fDisconnectSignal;

	void	_SetParentGroup (VCommandGroup* inGroup)	{ fParentGroup = inGroup; };
	VCommandGroup*	_GetParentGroup () const { return fParentGroup; };

	static	VCommand*	sNullCommand;
};


class XTOOLBOX_API VCommandContext : public VObject
{
public:
			VCommandContext (VCommand* inCommand, const VValueBag* inBag, const ICommandListener* inExcept);
	virtual	~VCommandContext ();

	// Value accessors
	const VValueSingle*	GetValueSingle () const;
	const VValueBag*	GetValueBag () const { return fBag; };
	
	// Helpers
	sLONG	GetValueAsLong () const { const VValueSingle* val = GetValueSingle (); return (val == NULL) ? 0 : val->GetLong (); };
	sLONG	GetValueAsBoolean () const { const VValueSingle* val = GetValueSingle (); return (val == NULL) ? 0 : val->GetBoolean (); };
	const VUUID*	GetValueAsUUID () const { return dynamic_cast<const VUUID*> (GetValueSingle ()); };
	
	// Attributes accessors
	const ICommandListener*	GetExceptListener () const { return fExcept; };

	void	SetForwardCommandToNextFocusable (bool inSet = true) { fNeedsToForwardCommandToNextFocusable = inSet; };
	bool	GetForwardCommandToNextFocusable () const { return fNeedsToForwardCommandToNextFocusable; };

	VCommandContext*	Clone () const;

private:
	VCommand*			fCommand;	// Retained
	const VValueBag*	fBag;		// Retained
	const ICommandListener*	fExcept;// Dangling pointer, use it for comparison only.
	bool		fNeedsToForwardCommandToNextFocusable;
};


class XTOOLBOX_API VCommandList : public VObject, public IRefCountable, public IBaggable
{
public:
							VCommandList (VCommandList* inParent = NULL);
							VCommandList (const VBagArray& inBag, ICommandListener* inTriggerListener);
	explicit				VCommandList (const VCommandList& inOriginal);
	virtual					~VCommandList ();

	// List handling (never return NULL)
	virtual	void			LoadCommands (const VBagArray& inBag, ICommandListener* inListener = NULL, bool inRemovePrevious = true);
	virtual	void			AddCommand (VCommand* inCommand);	// Command is retained, no unicity check is performed
			VCommand*		AddCommand (const VString& inID);
			VCommand*		AddCommand (const VString& inID, const VString& inName);	// You should release the result

	virtual	VCommand*		RetainNthCommand (VIndex inIndex) const;
	virtual	VCommand*		RetainCommand (const VString& inCommandID, bool inGlobalSearch = true) const;
			VIndex			GetCommandCount () const					{ return fCmdArray.GetCount (); };
	
	virtual	void			RemoveCommand (const VCommand* inCommand);
	virtual	void			RemoveAllCommands ();
			void			RemoveCommand (const VString& inID);
			bool			HasCommand (const VString& inID, bool inGlobalSearch = true) const;
			bool			HasCommand (const VCommand* inCommand) const;
			
			bool			IsEmpty() const	{ return fCmdArray.GetCount() == 0;}
	
			// Command states
			bool			IsCommandEnabled (const VString& inCommandID, bool inGlobalSearch = true) const;
			
			// Hierarchy handling
			VCommandList*	RetainParent() const;
			VCommandList*	GetParent() const	{ return fParent;}
	
	// Inherited from IBaggable
	virtual	VError			LoadFromBag (const VValueBag& inBag);
	virtual	VError			SaveToBag (VValueBag& ioBag, VString& outKind) const;

	// Utilities
	virtual	void			RegisterCommandConstructor (const VString& inKind, CommandConstructorProcPtr inConstructor);
	virtual	void			ConnectTriggerListener (const VString& inCommandID, ICommandListener* inListener, bool inGlobalSearch = true);

	const VArrayOfCommands&	GetList () const { return fCmdArray; };
	
private:
			void			SetParent( VCommandList* inParent);
			VCommandList*						fParent;
			VArrayString						fCommandKinds;
			VArrayOfCommands					fCmdArray;
	mutable	VCriticalSection					fCriticalSection;
			VArrayOf<CommandConstructorProcPtr>	fCommandConstructors;

			VCommand*		_LoadOneCommand (const VValueBag& inBag, ICommandListener* inListener);
			VCommand*		_GetCommand (const VString& inCommandID, bool inGlobalSearch = true) const;
};


class XTOOLBOX_API VCommandGroup : public VCommand
{
typedef VCommand	inherited;
public:
			VCommandGroup (const VString& inID);
	virtual	~VCommandGroup ();

	virtual	void	AddChild (VCommand* inChild);
	virtual	void	RemoveChild (VCommand* inChild);
	
	VIndex	GetChildCount () { return fChildren.GetCommandCount (); };
	virtual	VCommand*	RetainNthChild (VIndex inIndex);
	
protected:
friend	class VCommand;
	VCommandList	fChildren;
	
	virtual	void	_AddChild (VCommand* inChild);
};


class XTOOLBOX_API VCommandAttachment : public VAttachment
{
public:
			VCommandAttachment (OsType inEventType, VCommand* inCommand);
			VCommandAttachment (OsType inEventType, VCommandList* inList, const VString& inCommandID);
			VCommandAttachment (const VCommandAttachment& inOriginal);
	virtual	~VCommandAttachment ();
	
	// Accessors
	void	SetCommand (VCommand* inCommand);
	VCommand*	GetCommand () const { return fCommand; };
	
	virtual	OsType	GetAttachmentType () const { return ACT_COMMAND; };
	
	// Inherited from VObject
	virtual	CharSet	DebugDump (char* inTextBuffer, sLONG& inBufferSize) const;
	
protected:
	VCommand*	fCommand;
	
	virtual	void	DoExecute (OsType inEventType, const void* inData);
};


SIGNAL_PARAMETER_CLONER_RETAIN_RELEASE(VCommand)
SIGNAL_PARAMETER_CLONER_RETAIN_RELEASE(const VCommand)

SIGNAL_PARAMETER_CLONER_CLONE_DELETE(VCommandContext)
SIGNAL_PARAMETER_CLONER_CLONE_DELETE(const VCommandContext)

END_TOOLBOX_NAMESPACE


#endif
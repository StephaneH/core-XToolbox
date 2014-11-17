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
#ifndef __CComponent__
#define __CComponent__

#include "Kernel/Sources/VUUID.h"


BEGIN_TOOLBOX_NAMESPACE

// Needed declaration
class VResourceFile;
class VMemoryMgr;

/*!
	@header	CComponent
	@abstract	Components definition and base class
	@discussion
		A component is an abstract interface with an opaque implementation.
		It is not linked to the caller so you only need the interface file when
		writing code that uses it (no dll or stub is needed during the link).

		Components implementation can be defined as 'inline' code (i.e. hosted in
		an app) or stored in dynamic libraries. This later case may be used to
		handle plug-in architecture.

		You can update any component library as long as you don't change its
		interface file. Any client of your library will be ensured that no link
		nor code change will be necessary. This is helpfull to handle transparent
		update or bug-fix mecanisms.
		
		If you want to make evolve your component by adding methods, you'll have
		to 'publish' a new one (with a new identifier). This will allow your
		clients to use the new features only when they're available. In this case
		the component architecture provides full transparent upward and backward
		compatibilly. <A HREF='/GoldFingerDoc/XToolBox/Kernel/ComponentVersioning.pdf'>This document</A> explains how to implement such a feature.


		Notes: this management is similar to MS-COM technology but a bit simplier.
		The interface is also similar to Apple's Component Manager (CFPlugin)
		and respect MacOSX calling conventions.
		
		
		The use of components with the toolbox involves 3 files:
			KernelIPC/CComponent.h for definitions and base types,
			KernelIPC/VComponentManager.h for loading and releasing components,
			KernelIPC/VComponentLibrary.h for writing a component library.
			KernelIPC/VComponentImp.h for automating part of component implementation.
*/


/*!
	@typedef	CType
	@abstract	Component identifier type
	@discussion
	 	All components are identified by an unique CType. Some specific values
	 	are allready defined (see bellow). All small-capsed items are reserved
	 	for 4D use. Other values may be used by 3rd parties.
*/
typedef	OsType	CType;

/*!
	@typedef	CRef
	@abstract	Component instance type
	@discussion
	 	A component instance is defined by an unique CRef within all connected
	 	clients or servers. This value may be used as cross-process identifier
	 	when managing messaging through components.
*/
typedef	VUUID	CRef;

/*!
	@const	kCOMPONENT_TYPE_UNKNOWN
	@abstract	'????'
*/
const CType	kCOMPONENT_TYPE_UNKNOWN			= 0x3F3F3F3F;

/*!
	@const	kCOMPONENT_TYPE_DRAW
	@abstract	'draw'
*/
const CType	kCOMPONENT_TYPE_DRAW			= 0x64726177;

/*!
	@const	kCOMPONENT_TYPE_CHART
	@abstract	'chrt'
*/
const CType	kCOMPONENT_TYPE_CHART			= 0x63687274;

/*!
	@const	kCOMPONENT_MAKER_4D
	@abstract	' 4d '
*/
const CType	kCOMPONENT_MAKER_4D				= 0x20346420;

/*!
	@const	kCOMPONENT_FAMILY_NONE
	@abstract	'----'
*/
const CType	kCOMPONENT_FAMILY_NONE			= 0x45454545;

/*!
	@const	kCOMPONENT_FAMILY_ANY
	@abstract	'****'
*/
const CType	kCOMPONENT_FAMILY_ANY			= 0x42424242;

/*!
	@const	kCOMPONENT_FAMILY_SHELLCLIENT
	@abstract	'shlc'
*/
const CType	kCOMPONENT_FAMILY_SHELLCLIENT	= 0x73686C63;


// Main entry-point messages (see MainProcPtr)
/*!
	@const	kDLL_EVENT_REGISTER
	@abstract	'entr'
*/
const OsType	kDLL_EVENT_REGISTER	= 'regs';


/*!
	@const	kDLL_EVENT_UNREGISTER
	@abstract	'leav'
*/
const OsType	kDLL_EVENT_UNREGISTER	= 'leav';


// Needed declarations
class CComponent;
class VLibrary;


/*!
	@typedef	MainProcPtr
	@abstract	Library accessor function pointer
	@discussion
	 	This is one of the three function any component library should export
	 	(see also CreateComponentProcPtr and GetNthComponentTypeProcPtr).
	 	This function is called by the component manager to init/deinit the dll.
	 	The init message is also used to link the component library to its VLibrary host.
	 	
	 	When writing a component library using VComponentLibrary, this job
	 	is allready done for you.
	@param	inEvent The event (see constants) that should be handled.
	@param	inLibrary The library object that host this component library.
	@result	Returns an error code
*/
typedef	VError (*MainProcPtr) (OsType inEvent, VLibrary* inLibrary);

/*!
	@typedef	CreateComponentProcPtr
	@abstract	Component creator function pointer
	@discussion
	 	This is one of the three functions any component library should export
	 	(see also GetNthComponentTypeProcPtr and MainProcPtr). When you
	 	handle 'inline' components (not lib-based) you should provide your own
	 	creator to the Component Manager when registering your component.
	 	
	 	This function should instanciate the asked component implementation
	 	and return a generic opaque CComponent pointer to it. The inFamily
	 	parameter specify the family the component is supposed to belong to.
	 	If your component doesn't belong to this family you should return
	 	NULL.
	 	
	 	When writing a component library using VComponentLibrary, this job
	 	is allready done for you.
	@param	inType The asked component you should instanciate
	@param	inFamily The Family the component should belong to
	@result	Returns the newly created component
*/
typedef	CComponent* (*CreateComponentProcPtr) (CType inType, CType inFamily);

/*!
	@typedef	GetNthComponentTypeProcPtr
	@abstract	Component-type iterator function pointer
	@discussion
	 	This is one of the three functions any component library should export
	 	(see also CreateComponentProcPtr and MainProcPtr). This function
	 	is used to automate component registration for a library.
	 	
	 	When writing a component library using VComponentLibrary, this job
	 	is done for you if you provide a simple component list (see
	 	VComponentLibrary for informations).
	@param	inTypeIndex The index of the asked component type. The first type
		has an index of 1.
	@param	outID In this param you should return the asked component type
	@result	Returns true if there's a type for this index (outID is valid).
		The indexes must be contigous (from 1 to n).
*/
typedef	Boolean (*GetNthComponentTypeProcPtr) (sLONG inTypeIndex, CType& outID);


// Component factory automation
// Used internally by VComponentLibrary
typedef	CComponent* (*CreateImpProcPtr) (CType inFamily);

typedef	struct CImpDescriptor
{
	CType				componentType;
	CreateImpProcPtr	impCreator;
} CImpDescriptor;


/*!
	@typedef	CRequirement
	@abstract	Component requirement structure
	@discussion
	 	This struct allows you to specify on which component your component
	 	rely. This will ensure that you're component won't be launched if
	 	another component is lacking.
	 	
	 	Requirement mecanism is handled by the VComponentManager you only have
	 	to return a CInfoPtr in the GetComponentInfos function (see CInfoBlock).
	@field componentType	The component type you rely on.
	@field minComponentVersion	The minimal version for this component (set -1 if
		you don't care about the version).
*/
typedef struct CRequirement
{
	CType		componentType;
	sLONG		minComponentVersion;			// -1 for no min
} CRequirement;

/*!
	@typedef	CInfoBlock
	@abstract	Component information structure
	@discussion
	 	This struct allows is used to return various information about a component.
	 	You can get it by calling GetComponentInfos on any component.
	 	
	 	Default values are returned when you write a component library using
	 	VComponentLibrary. However you can return your own infos if you overload
	 	VComponentImp::GetComponentInfos. This is helpfull if you rely on other
	 	components: in this case you fill the array of CRequirement to describe
	 	your needs (see CRequirement).
	 	
	 	Versioning is implemented for ascent compatibility. Calling code should
	 	check explicitly the version it needs (e.g. using CInfoBlock_10
	 	and kINFO_BLOCK_10). Instantiating code should use latest available version
	 	at compilation time (using CInfoBlock and kINFO_BLOCK_LATEST).
	@field structVersion The version for this struct (currently kINFO_BLOCK_10)
	@field structSize The allocated size of the struct (depends on the number of CRequirement)
	@field componentType The type of the component (default is the Component_Type constant)
	@field componentManufactor The manufactor of the component (default is the Component_Maker constant)
	@field componentFamily The family the component belongs to (the default is the Component_Family constant)
	@field componentVersion The version number for the component (default is 0)
	@field componentRef The CRef for the component (unique, see CRef)
	@field requirementCount The number of valid CRequirement in the struct (default is 0)
	@field requirementArray Array of CRequirements (the allocator should adapt the block size so that it can hold the array)
*/
typedef struct CInfoBlock_10
{
	sLONG			structVersion;				// Set to kINFO_BLOCK_LATEST
	sLONG			structSize;					// Use sizeof(CInfoBlock) by default
	CType			componentType;
	CType			componentManufactor;
	CType			componentFamily;
	uLONG			componentVersion;			// Suggested format is 0x00012100 for version 1.2.1
	VUUIDBuffer		componentRef;
	sLONG			requirementCount;
	CRequirement	requirementArray[1];
} CInfoBlock_10;

typedef CInfoBlock_10	CInfoBlock;
typedef	CInfoBlock*		CInfoPtr;

/*!
	@enum	InforBlockVersion
	@abstract	Values for CMessageBlock.structVersion
	@constant	kINFO_BLOCK_10
*/
enum
{
	kINFO_BLOCK_10		= 0x00010000,
	kINFO_BLOCK_LATEST	= kINFO_BLOCK_10
};


#if 0
// ComponentMessage block
typedef void (*CReplyProcPtr) (struct CMessageBlock*);

/*!
	@typedef	CMessageBlock
	@abstract	Component message structure
	@discussion
	 	This structure may change in a near futur. May become an object.
	 	We're currently waiting to know what the needs are exactly for
	 	component messaging.
	@field structVersion The version for this struct (currently kMESSAGE_BLOCK_10)
	@field fixedStructSize The fixed size of the struct (actually sizeof(CMessageBlock))
	@field totalStructSize The allocated size of the struct (depends on extra parameters)
	@field priorityLevel The level of priority (not used, set to 0)
	@field messageRef The unique reference for the message
	@field senderRef The CRef of the sender component
	@field destinatorRef The CRef of the destinator component
	@field destinatorMask The mask that specify which part of the destinatorRef should be used (see mask values)
	@field resultError The result code for the message. Depends on the message (usually an error code).
	@field replyType The type of reply that is expected (see enum values)
	@field replyProcPtr The function pointer for the reply if replyType is kPROC_BASED_REPLY (NULL if not used)
	@field sendingTime Not used - set to 0
	@field validityDuration Not used - set to 0
	@field param1 Private data for your own use
	@field param2 Private data for your own use
	@field param3 Private data for your own use
*/

typedef struct CMessageBlock
{
	sLONG			structVersion;		// Set to kMESSAGE_BLOCK_10
	sLONG			fixedStructSize;	// Set to sizeof(CMessageBlock)
	sLONG			totalStructSize;	// fixedStructSize + sizeof additionnal data (if any)
	sLONG			priorityLevel;		// Set to 0
	CRef			messageRef;			// Set to 0 for a new message
	CRef			senderRef;			// Set to sender->GetComponentInstanceRef()
	CRef			destinatorRef;		// Set to destinator->GetComponentInstanceRef()
	uLONG8			destinatorMask;		// Defines how to interprete destinatorID (see bellow)
	VError			resultError;		// On return from SendMessage will be an error code
	CType			replyType;			// Specify the action the destinator is expected to do
	CReplyProcPtr	replyProcPtr;		// Set to NULL if replyType is different from kPROC_BASED_REPLY
	uLONG8			sendingTime;		// Set to 0 (not used currently)
	sLONG8			validityDuration;	// Set to 0 (not used currently)
	void*			param1;				// Private data for your own use
	void*			param2;				// Private data for your own use
	void*			param3;				// Private data for your own use
} CMessageBlock;

typedef	CMessageBlock*	CMessagePtr;

#endif

/*!
	@enum	ReplyType
	@abstract	Values for CMessageBlock.replyType
	@constant	kNO_REPLY No reply is expected from the sender
	@constant	kMSG_TO_SENDER The sender expects a reply as message
	@constant	kMSG_TO_ALL_DESTINATORS All destinators should receive a reply as a message
	@constant	kPROC_BASED_REPLY The given reply function pointer should be called
*/
enum
{
	kNO_REPLY				= 'nrpl',
	kMSG_TO_SENDER			= 'rsdr',
	kMSG_TO_ALL_DESTINATORS	= 'rall',
	kPROC_BASED_REPLY		= 'proc'
};

/*!
	@enum	MessageBlockVersion
	@abstract	Values for CMessageBlock.structVersion
	@constant	kMESSAGE_BLOCK_10
*/
enum
{
	kMESSAGE_BLOCK_10		= 0x00010000
};

/*!
	@enum	DestinatorMasks
	@abstract	Values for CMessageBlock.destinatorMask
	@constant	kDEST_MASK_DEST_ONLY Only the given destinator (destinatorRef) should receive the message (unicast)
	@constant	kDEST_MASK_ALL_COMP All component instances should receive the message (broadcast)
	@constant	kDEST_MASK_SAME_TYPE All component instances of the given type should receive the message (multicast)
	@constant	kDEST_MASK_SAME_GROUP All component instances of the given group should receive the message (multicast)
*/
const uLONG8	kDEST_MASK_DEST_ONLY	= (uLONG8) XBOX_LONG8(0xFFFFFFFFFFFFFFFF);
const uLONG8	kDEST_MASK_ALL_COMP		= (uLONG8) XBOX_LONG8(0x0000000000000000);
const uLONG8	kDEST_MASK_SAME_TYPE	= (uLONG8) XBOX_LONG8(0xFFFFFFFF00000000);	// You may use both SAME_TYPE & SAME_GROUP
const uLONG8	kDEST_MASK_SAME_GROUP	= (uLONG8) XBOX_LONG8(0x00000000FFFF0000);	// (kDEST_MASK_SAME_TYPE | kDEST_MASK_SAME_GROUP)


/*!
	@class	CComponent
	@abstract	Basic class for components
	@discussion
		All component should derive from CComponent or any derived class.
		
		This class is only an interface to an opaque implementation class (that
		tipically inherite from VComponentImp) handled by the component creator.
		For this reason a CComponent class in never instanciated. All members have
		to be pure virtual.
		
		The implementation part is done using VComponentImp (see VComponentLibrary) while
		the interface class is provided to the component users (see also the header introduction).
		
		As a convention all classes that inherit from CComponent should be prefixed by 'C'.
		
		Note that you never use operator new or delete with a component. To create a component
		use the Component Manager (see VComponentManager). To release it use myComponent->Release.
*/
class CComponent
{
public:
/*!
	@function	Retain
	@abstract	Increment the ref-counting of a component
	@discussion
		 This function returns a copy of the current instance. This is helpfull when
		 storing a component or passing it as function parameter (as you should allways
		 increment its ref-counting in this case).
		 
		 Example: CComponent*	component2 = component1->Retain();
	@result	Returns a copy of 'this'.
*/
	virtual CComponent*	Retain (const char* DebugInfo = 0) = 0;

/*!
	@function	Release
	@abstract	Decrement the ref-counting of a component
*/
	virtual void		Release (const char* DebugInfo = 0) = 0;

/*!	
	@function	GetRefCount
	@abstract	Returns the number of references the instance already has
*/
	virtual VIndex		GetRefCount() = 0 ;

/*!
	@function	GetComponentInfos
	@abstract	Gets the InfoBlock of a component instance
	@discussion
		use VMemory::DisposePtr for deallocation.
	@result	Returns a pointer to an InfoBlock, you should release it after use.
*/
	virtual CInfoPtr	GetComponentInfos () = 0;

/*!
	@function	GetComponentRef
	@abstract	Gets the unique CRef of a component instance
	@result	Returns a unique instance identifier (see also CRef definition).
*/
//	virtual CRef		GetComponentRef () const = 0;

/*!
	@function	GetComponentType
	@abstract	Gets the component type of a component instance
	@result	Returns the type identifier for the component (see also CType definition).
*/
	virtual CType		GetComponentType () const = 0;

/*!
	@function	GetComponentFamily
	@abstract	Gets the component family of a component instance
	@result	Returns the family the component belongs to.
*/
	virtual CType		GetComponentFamily () = 0;
	
/*!
	@function	RetainComponentResFile
	@abstract	Gets the component resource file of a component instance
	@result	Returns the resource file of the component if any, NULL otherwise.
*/
	virtual VResourceFile*	RetainResourceFile () = 0;
	
/*!
	@var	Component_Type
	@abstract	Component identifier
	@discussion
		Every component defines a unique Component_Type. As far as the component
		evolves without changing it's interface, only the version number change.
		If any modification is made to the class, the Component_Type _must_ be
		changed (it becomes a full new component).
*/
	enum { Component_Type = kCOMPONENT_TYPE_UNKNOWN };
	
/*!
	@var	Component_Family
	@abstract	Component family identifier
	@discussion
		A component may belong to a component family. This may be a specific
		component interface or behaviour (usefull for implementing plug-ins).
*/
	enum { Component_Family = kCOMPONENT_FAMILY_NONE };

/*!
	@var	Component_Maker
	@abstract	Component manufactor
	@discussion
		The default manufactor for Toobox based components is kCOMPONENT_MAKER_4D.
		Every third party should personalize this constant.
*/
	enum { Component_Maker = kCOMPONENT_MAKER_4D };

protected:
	friend class VComponentManager;

/*!
	@function	ListenToMessage
	@abstract	Should be overriden by components that wish to receive messages
	@discussion
		You should response to the message according to the replyType field of the
		MessageBlock structure.
	@param	inMessage Points to a MessageBlock structure (see CMessageBlock for explanation)
*/
	//virtual void	ListenToMessage (CMessagePtr inMessage) = 0;
};

END_TOOLBOX_NAMESPACE

#endif

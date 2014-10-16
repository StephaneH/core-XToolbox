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
#ifndef __VComponentImp__
#define __VComponentImp__

#include "Kernel/VKernel.h"
#include "KernelIPC/Sources/VComponentLibrary.h"
#include "Kernel/Sources/VRefCountDebug.h"

BEGIN_TOOLBOX_NAMESPACE

#ifndef MAKE_COMPONENT_DLL
#define	MAKE_COMPONENT_DLL	1
#endif


// Needed declarations
class VLibrary;


// VComponentImp handle minimal CComponent interface
//
template <class Type>
class VComponentImp : public VObject, public Type
{
public:
			VComponentImp ();
	virtual	~VComponentImp ();
	
	// Refcount support
	virtual CComponent*	Retain (const char* DebugInfo = 0);
	virtual void		Release (const char* DebugInfo = 0);
	virtual VIndex		GetRefCount() { return fRefCount ; } ;
	virtual void		DoCallOnRefCount0() { ; };

	// Accessors
	virtual CInfoPtr	GetComponentInfos ();
//	virtual CRef		GetComponentRef () const { return fInstanceRef; };
	virtual CType		GetComponentType () const { return Type::Component_Type; };
	virtual CType		GetComponentMaker () const { return Type::Component_Maker; };
	virtual CType		GetComponentFamily () { return Type::Component_Family; };
	
	// Resource file support
	virtual VResourceFile*	RetainResourceFile ();
		
protected:
//	virtual void		ListenToMessage (CMessagePtr inMessage);
	
private:
	VIndex		fRefCount;
//	CRef		fInstanceRef;
	VIndex		fInstanceIndex;
};


// ComponentImp registration mecanism
template <class Type>
class VImpCreator {
public:
	// Use to define the CImpDescriptor array of a component library
	static CComponent*	CreateImp (CType inFamily) {
							Type*	component = NULL;
							if (inFamily == kCOMPONENT_FAMILY_ANY || inFamily == Type::Component_Family)
								component = new Type();
								
							return component;
						};
	
	// Use to get an imp object from its component object
	static Type*		GetImpObject (CComponent* inComponent)
						{
							assert(inComponent != NULL);
							if (inComponent == NULL)
								return NULL;
							assert(inComponent->GetComponentType() == Type::Component_Type);
							Type*	compImp = (Type*)(inComponent);
							return compImp;
						};
	
	// Same as previous for const objects
	static const Type*	GetImpObject (const CComponent* inComponent)
						{
							assert(inComponent != NULL);
							if (inComponent == NULL)
								return NULL;
							assert(inComponent->GetComponentType() == Type::Component_Type);
							const Type*	compImp = (const Type*)(inComponent);
							return compImp;
						};
};


// ---------------------------------------------------------------------------
// VComponentImp<Type>::VComponentImp
// ---------------------------------------------------------------------------
// 
template <class Type>
VComponentImp<Type>::VComponentImp()
{
	fRefCount = 1;
	fInstanceIndex = 0;

#if VERSIONDEBUG
	//VRefCountDebug::sRefCountDebug.RetainInfo(this, "new");
#endif

#if MAKE_COMPONENT_DLL
	// No task lock is needed as the VComponentImp is set once
	// and before instanciating any component
	VComponentLibrary*	compLibrary = VComponentLibrary::GetComponentLibrary();
	assert(compLibrary != NULL);
	fInstanceIndex = compLibrary->RegisterOneInstance(Type::Component_Type);
#endif
}


// ---------------------------------------------------------------------------
// VComponentImp<Type>::~VComponentImp
// ---------------------------------------------------------------------------
// A component delete itself when the refcount reaches '0'.
// DO NOT USE DELETE OPERATOR with a VComponent !
//
template <class Type>
VComponentImp<Type>::~VComponentImp()
{
	assert(fRefCount == 0);

#if VERSIONDEBUG
	VRefCountDebug::sRefCountDebug.ReleaseInfo(this);
#endif

#if MAKE_COMPONENT_DLL
	// No task lock is needed as the VComponentImp is set once
	// and before instanciating any component
	VComponentLibrary*	compLibrary = VComponentLibrary::GetComponentLibrary();
	assert(compLibrary != NULL);
	compLibrary->UnRegisterOneInstance(Type::Component_Type);
#endif
}


// ---------------------------------------------------------------------------
// VComponentImp<Type>::Retain
// ---------------------------------------------------------------------------
// Call each time you use a copy of component's object
//
template <class Type>
CComponent* VComponentImp<Type>::Retain(const char* DebugInfo)
{
	VInterlocked::Increment(&fRefCount);

#if VERSIONDEBUG
	if (DebugInfo != 0)
	{
		VRefCountDebug::sRefCountDebug.RetainInfo(this, DebugInfo);
	}
#endif

	return this;
}


// ---------------------------------------------------------------------------
// VComponentImp<Type>::Release
// ---------------------------------------------------------------------------
// Call if you no longer need a component object
//
template <class Type>
void VComponentImp<Type>::Release(const char* DebugInfo)
{
#if VERSIONDEBUG
	if (DebugInfo != 0)
	{
		VRefCountDebug::sRefCountDebug.ReleaseInfo(this, DebugInfo);
	}
#endif
	// Unregister _before_ to ensure it's no more used by the component manager
	if (fRefCount == 1)
		DoCallOnRefCount0();

	if (fRefCount == 1)
		VComponentManager::UnRegisterComponentInstance((CComponent*)this);
		
	// At this point we're sure no one can retain while reaching 0
	if (VInterlocked::Decrement(&fRefCount) == 0)
		delete this;
}


// ---------------------------------------------------------------------------
// VComponentImp<Type>::GetComponentInfos()
// ---------------------------------------------------------------------------
// Returns a new allocated struct containing component infos
//
// Override if you need to request specific components and/or add personnal data
template <class Type>
CInfoPtr VComponentImp<Type>::GetComponentInfos()
{
	CInfoPtr	infos = (CInfoPtr) VMemory::NewPtr(sizeof(CInfoBlock), 'comp');
	if (infos != NULL)
	{
		infos->structVersion = kINFO_BLOCK_LATEST;
		infos->structSize = sizeof(CInfoBlock);
		infos->componentType = GetComponentType();
		infos->componentManufactor = GetComponentMaker();
		infos->componentFamily = GetComponentFamily();
		infos->componentVersion = 0;
		infos->requirementCount = 0;
		infos->requirementArray[0].componentType = 0;
		infos->requirementArray[0].minComponentVersion = 0;
		
		//fInstanceRef.ToBuffer(infos->componentRef);
		
		// Make sure all fields have been initialized (default code
		// should allways implement latest version of structure).
		assert(kINFO_BLOCK_LATEST == kINFO_BLOCK_10);
	}
	
	return infos;
}


// ---------------------------------------------------------------------------
// VComponentImp<Type>::RetainResourceFile()
// ---------------------------------------------------------------------------
// Returns the resource file of a component library.
// Returns NULL for proc-based components.
//
// Override if you need to customize resourse storage (e.g. using 4DResourceFile
// or bundle design or separate data file...)
template <class Type>
VResourceFile* VComponentImp<Type>::RetainResourceFile()
{
	VResourceFile*	resFile = NULL;
	
#if MAKE_COMPONENT_DLL
	VComponentLibrary*	compLibrary = VComponentLibrary::GetComponentLibrary();
	assert(compLibrary != NULL);
	
	VLibrary*	library = compLibrary->GetLibrary();
	assert(library != NULL);
	
	resFile = library->RetainResourceFile();
#endif

	return resFile;
}


// ---------------------------------------------------------------------------
// VComponentImp<Type>::ListenToMessage(VCMessagePtr)
// ---------------------------------------------------------------------------
// This function handle default reply for message listening. You shouldn't
// care on the destinator parameter of the MessageBlock as the ComponentMgr
// allready did it.
//
// Override if you want to handle messages, you can call inherited
// at the end of your function if you don't want to care about reply.
//

#if 0
template <class Type>
void VComponentImp<Type>::ListenToMessage(CMessagePtr inMessage)
{
	if (inMessage == NULL) return;
	
	switch (inMessage->replyType)
	{
		case kNO_REPLY:
			break;
			
		case kMSG_TO_SENDER:
			inMessage->replyType = kNO_REPLY;
			inMessage->destinatorRef = inMessage->senderRef;
			inMessage->senderRef = fInstanceRef;
			inMessage->destinatorMask = kDEST_MASK_DEST_ONLY;
			
			VComponentManager::SendMessage(inMessage);
			break;
			
		case kMSG_TO_ALL_DESTINATORS:
			inMessage->replyType = kNO_REPLY;
			inMessage->senderRef = fInstanceRef;
			
			VComponentManager::SendMessage(inMessage);
			break;
			
		case kPROC_BASED_REPLY:
			if (inMessage->replyProcPtr != NULL)
			{
				(inMessage->replyProcPtr)(inMessage);
			}
			else if (inMessage->resultError == VE_OK)
			{
				inMessage->resultError = VE_COMP_PROC_PTR_NULL;
			}
			break;
		
		default:
			// Unknown reply method
			assert(false);
			break;
	}
}

#endif

END_TOOLBOX_NAMESPACE

#endif
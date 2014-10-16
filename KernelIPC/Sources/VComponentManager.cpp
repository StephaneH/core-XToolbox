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
#include "VComponentManager.h"
#include "VComponentLibrary.h"

BEGIN_TOOLBOX_NAMESPACE

#define	CHECK_COMPONENT_RELEASE		0	// Set to 1 to make sure all components are released on exit

#if USE_UNMANGLED_EXPORTS
	char	kDEFAULT_MAIN_SYMBOL[] = "Main";
	char	kDEFAULT_LOCK_SYMBOL[] = "Lock";
	char	kDEFAULT_UNLOCK_SYMBOL[] = "UnLock";
	char	kDEFAULT_CAN_UNLOAD_SYMBOL[] = "CanUnloadLibrary";
	char	kDEFAULT_CREATE_SYMBOL[] = "CreateComponent";
	char	kDEFAULT_GET_TYPES_SYMBOL[] = "GetNthComponentType";
#elif VERSIONMAC
	char	kDEFAULT_MAIN_SYMBOL[] = "__ZN4xbox17VComponentLibrary4MainEmPNS_8VLibraryE";
	char	kDEFAULT_LOCK_SYMBOL[] = "__ZN4xbox17VComponentLibrary4LockEv";
	char	kDEFAULT_UNLOCK_SYMBOL[] = "__ZN4xbox17VComponentLibrary6UnlockEv";
	char	kDEFAULT_CAN_UNLOAD_SYMBOL[] = "__ZN4xbox17VComponentLibrary16CanUnloadLibraryEv";
	char	kDEFAULT_CREATE_SYMBOL[] = "__ZN4xbox17VComponentLibrary15CreateComponentEmm";
	char	kDEFAULT_GET_TYPES_SYMBOL[] = "__ZN4xbox17VComponentLibrary19GetNthComponentTypeElRm";
#elif VERSIONWIN
#ifndef _WIN64
	char	kDEFAULT_MAIN_SYMBOL[] = "?Main@VComponentLibrary@xbox@@SG_KKPAVVLibrary@2@@Z";
	char	kDEFAULT_LOCK_SYMBOL[] = "?Lock@VComponentLibrary@xbox@@SGXXZ";
	char	kDEFAULT_UNLOCK_SYMBOL[] = "?Unlock@VComponentLibrary@xbox@@SGXXZ";
	char	kDEFAULT_CAN_UNLOAD_SYMBOL[] = "?CanUnloadLibrary@VComponentLibrary@xbox@@SGEXZ";
	char	kDEFAULT_CREATE_SYMBOL[] = "?CreateComponent@VComponentLibrary@xbox@@SGPAVCComponent@2@KK@Z";
	char	kDEFAULT_GET_TYPES_SYMBOL[] = "?GetNthComponentType@VComponentLibrary@xbox@@SGEJAAK@Z";
#else
	char	kDEFAULT_MAIN_SYMBOL[]       = "?Main@VComponentLibrary@xbox@@SA_KKPEAVVLibrary@2@@Z";
	char	kDEFAULT_LOCK_SYMBOL[]       = "?Lock@VComponentLibrary@xbox@@SAXXZ"; 
	char	kDEFAULT_UNLOCK_SYMBOL[]     = "?Unlock@VComponentLibrary@xbox@@SAXXZ"; 
	char	kDEFAULT_CAN_UNLOAD_SYMBOL[] = "?CanUnloadLibrary@VComponentLibrary@xbox@@SAEXZ";
	char	kDEFAULT_CREATE_SYMBOL[]     = "?CreateComponent@VComponentLibrary@xbox@@SAPEAVCComponent@2@KK@Z";
	char	kDEFAULT_GET_TYPES_SYMBOL[]  = "?GetNthComponentType@VComponentLibrary@xbox@@SAEJAEAK@Z";
#endif
#elif VERSION_LINUX
	#if ARCH_64
	char	kDEFAULT_MAIN_SYMBOL[]          = "_ZN4xbox17VComponentLibrary4MainEjPNS_8VLibraryE";
	char	kDEFAULT_LOCK_SYMBOL[]          = "_ZN4xbox17VComponentLibrary4LockEv";
	char	kDEFAULT_UNLOCK_SYMBOL[]        = "_ZN4xbox17VComponentLibrary6UnlockEv";
	char	kDEFAULT_CAN_UNLOAD_SYMBOL[]    = "_ZN4xbox17VComponentLibrary16CanUnloadLibraryEv";
	char	kDEFAULT_CREATE_SYMBOL[]        = "_ZN4xbox17VComponentLibrary15CreateComponentEjj";
	char	kDEFAULT_GET_TYPES_SYMBOL[]     = "_ZN4xbox17VComponentLibrary19GetNthComponentTypeEiRj";
	#else
	char	kDEFAULT_MAIN_SYMBOL[]          = "_ZN4xbox17VComponentLibrary4MainEmPNS_8VLibraryE";
	char	kDEFAULT_LOCK_SYMBOL[]          = "_ZN4xbox17VComponentLibrary4LockEv";
	char	kDEFAULT_UNLOCK_SYMBOL[]        = "_ZN4xbox17VComponentLibrary6UnlockEv";
	char	kDEFAULT_CAN_UNLOAD_SYMBOL[]    = "_ZN4xbox17VComponentLibrary16CanUnloadLibraryEv";
	char	kDEFAULT_CREATE_SYMBOL[]        = "_ZN4xbox17VComponentLibrary15CreateComponentEmm";
	char	kDEFAULT_GET_TYPES_SYMBOL[]     = "_ZN4xbox17VComponentLibrary19GetNthComponentTypeElRm";
	#endif
#endif


// Class Variables
VArrayOf<CreateComponentProcPtr>*	VComponentManager::sComponentProcs = NULL;
VArrayOf<VLibrary*>*	VComponentManager::sComponentLibraries = NULL;
VArrayOf<CType>*		VComponentManager::sComponentTypes = NULL;
VArrayOf<CComponent*>*	VComponentManager::sComponentInstances = NULL;
VArrayOf<CType>*		VComponentManager::sComponentChecking = NULL;
VArrayOf<Boolean>*		VComponentManager::sComponentIsAvailable = NULL;
VArrayOf<Boolean>*		VComponentManager::sComponentHasRequirements = NULL;
VArrayOf<CType>*		VComponentManager::sComponentChecked = NULL;
VCriticalSection*		VComponentManager::sAccessingTypes;
VCriticalSection*		VComponentManager::sAccessingInstances;
VCriticalSection*		VComponentManager::sAccessingChecked;
VCriticalSection*		VComponentManager::sAccessingChecking;
Boolean					VComponentManager::sIsInitialized = false;


// ---------------------------------------------------------------------------
// VComponentManager::RetainComponent								[static]
// ---------------------------------------------------------------------------
// Return a component of a given type. If a component of the same type exists
// it's returned incrementing its reference.
// To make sure you get a newly created component use RetainUniqueComponent.
//
CComponent* VComponentManager::RetainComponent(CType inType, CType inFamily)
{
	if (!IsInitialized()) return NULL;
	
	CComponent*		component = NULL;
	VArrayIteratorOf<CComponent*>	iterator(*sComponentInstances);
	
	sAccessingInstances->Lock();
	
	// Iterate through instances
	while ((component = iterator.Next()) != NULL)
	{
		if (component->GetComponentType() == inType &&
			(inFamily == kCOMPONENT_FAMILY_ANY || inFamily == component->GetComponentFamily()))
		{
			component->Retain();
			break;
		}
	}
	
	if (component == NULL)
	{
		// If not available, create a new one
		component = RetainUniqueComponent(inType, inFamily);
		
		if (component != NULL)
			RegisterComponentInstance(component);
	}
	
	// Make sure only one instance of each type is registred
	// Otherwise could have unlocked before RetainUniqueComponent
	sAccessingInstances->Unlock();
	
	return component;
}


// ---------------------------------------------------------------------------
// VComponentManager::RetainComponent								[static]
// ---------------------------------------------------------------------------
// Retreive a component object from its instance reference.
// 

#if 0
CComponent* VComponentManager::RetainComponent(CRef inRef)
{
	if (!IsInitialized()) return NULL;
	
	CComponent*	component = NULL;
	VArrayIteratorOf<CComponent*>	iterator(*sComponentInstances);
	
	sAccessingInstances->Lock();
	
	// Iterate through instances
	while ((component = iterator.Next()) != NULL)
	{
		if (component->GetComponentRef() == inRef)
		{
			component->Retain();
			break;
		}
	}
	
	sAccessingInstances->Unlock();
	
	return component;
}
#endif


// ---------------------------------------------------------------------------
// VComponentManager::RetainUniqueComponent							[static]
// ---------------------------------------------------------------------------
// Return a 'stand-alone' (with its own datas) component.
//
CComponent* VComponentManager::RetainUniqueComponent(CType inType, CType inFamily, Boolean inSkipRequirements)
{
	if (!IsInitialized()) return NULL;
	
	sAccessingTypes->Lock();
	
	sLONG	index = sComponentTypes->FindPos(inType);
	if (index <= 0)
	{
		sAccessingTypes->Unlock();
		return NULL;
	}
	
	// Look first for Proc based components
	CComponent*	component = NULL;
	CreateComponentProcPtr	compProc = sComponentProcs->GetNth(index);
	
	if (compProc != NULL)
	{
		component = (compProc)(inType, inFamily);
	}
	else
	{
		// Look then for Library based components
		VLibrary*	compLibrary = sComponentLibraries->GetNth(index);
		
		if (compLibrary != NULL)
		{
			CreateComponentProcPtr	createProcPtr = (CreateComponentProcPtr) compLibrary->GetProcAddress(kDEFAULT_CREATE_SYMBOL);
			component = (createProcPtr)(inType, inFamily);
		}
	}
	
	// Can only unlock once procPtr is no longer needed and if the library's
	// refcount have been increased (this is done by callback when creating
	// the component - see VComponentLibrary)
	sAccessingTypes->Unlock();
	
	// Make sure component requirements are verified
	if (component != NULL && !inSkipRequirements && !CheckComponentRequirements(component))
	{
		VError	error = VE_COMP_UNRESOLVED_DEPENDENCIES;
		component->Release();
		component = NULL;
	}
	
	return component;
}


// ---------------------------------------------------------------------------
// VComponentManager::RetainComponentsOfFamily						[static]
// ---------------------------------------------------------------------------
// Return a list of 
//
Boolean VComponentManager::RetainComponentsOfFamily(CType inFamily, VArrayOf<CComponent*>* ioComponentList)
{
	if (ioComponentList == NULL) return false;
	
	sAccessingTypes->Lock();
	
	sLONG	count = sComponentTypes->GetCount();
	for (sLONG index = 1; index <= count; index++)
	{
		// Check each component type
		CComponent*	component = RetainComponent(sComponentTypes->GetNth(index), inFamily);
		if (component != NULL)
			ioComponentList->AddTail(component);
	}
	
	sAccessingTypes->Unlock();
	
	return true;
}


// ---------------------------------------------------------------------------
// VComponentManager::IsComponentAvailable							[static]
// ---------------------------------------------------------------------------
//
Boolean VComponentManager::IsComponentAvailable(CType inType)
{
	if (!IsInitialized()) return false;
	
	return (sComponentTypes->FindPos(inType) > 0);
}


// ---------------------------------------------------------------------------
// VComponentManager::CheckComponentRequirements					[static]
// ---------------------------------------------------------------------------
// This function isn't reentrant. Take care not to generate a dead lock
// by generating a thread that load a component from your component creator
//
Boolean VComponentManager::CheckComponentRequirements(CComponent* inComponent, Boolean inRetainComponents)
{
	if (inComponent == NULL) return false;
	
	CType	componentType = inComponent->GetComponentType();
	
	// Use registred result if allready checked
	sAccessingChecked->Lock();
	
	VIndex	listIndex = sComponentChecked->FindPos(componentType);
	if (listIndex > 0)
	{
		Boolean	isAvailable = sComponentIsAvailable->GetNth(listIndex);
		sAccessingChecked->Unlock();
		return isAvailable;
	}
	
	sAccessingChecked->Unlock();
	
	// Register as being checking (lock other threads during whole operation)
	sAccessingChecking->Lock();
	sComponentChecking->AddTail(componentType);
	
	// Begin checking component infos (CRequirement)
	CInfoPtr	infos = inComponent->GetComponentInfos();
	if (infos == NULL) return false;
	
	// We only care about version 1.0 informations
	if (infos->structVersion < kINFO_BLOCK_10) return false;
	if (infos->structSize < sizeof(CInfoBlock_10)) return false;
	
	sLONG	index = 0;
	Boolean	verified = true;
	
	// Loop until a requirement fails
	if (infos->requirementCount > 0)
	{
		CComponent*	dependence;
		VArrayRetainedPtrOf<CComponent*>	compList;
		
		while (index < infos->requirementCount && verified)
		{
			CType	neededType = infos->requirementArray[index].componentType;
			sLONG	neededVersion = infos->requirementArray[index].minComponentVersion;
			
			// Avoid recursive references
			if (testAssert(neededType != componentType))
			{
				// NOTE: this lock could be removed if arrays are safe for search (doesn't care about sync)
				sAccessingChecked->Lock();
				Boolean	allreadyChecked = sComponentChecked->FindPos(neededType) > 0;
				Boolean	allreadyChecking = sComponentChecking->FindPos(neededType) > 0;
				sAccessingChecked->Unlock();
				
				// Avoid cross references
				if ((!allreadyChecking && !allreadyChecked) || neededVersion != -1)
				{
					// Check component availability
					dependence = RetainUniqueComponent(neededType, kCOMPONENT_FAMILY_ANY, allreadyChecking || allreadyChecked);
					if (dependence == NULL)
					{
						verified = false;
						break;
					}
					
					// Check version
					if (neededVersion != -1)
					{
						CInfoPtr	dependenceInfos = dependence->GetComponentInfos();
						if (dependenceInfos != NULL)
						{
							verified = dependenceInfos->componentVersion >= (uLONG) neededVersion;
							VMemory::DisposePtr((VPtr)dependenceInfos);
						}
					}
					
					if (inRetainComponents)
						compList.AddTail(dependence);
					else
						dependence->Release();
				}
			}
			
			index++;
		}
		
		compList.Destroy((verified && inRetainComponents) ? NO_DESTROY : DESTROY);
	}
	
	VMemory::DisposePtr((VPtr)infos);
	
	// Register checking result
	sAccessingChecked->Lock();
	if (sComponentChecked->FindPos(componentType) > 0)
	{
		sComponentChecked->AddTail(componentType);
		sComponentIsAvailable->AddTail(verified);
		sComponentHasRequirements->AddTail(index > 0);
	}
	sAccessingChecked->Unlock();
	
	// Release checking
	sComponentChecking->Delete(componentType);
	sAccessingChecking->Unlock();
	
	return verified;
}


// ---------------------------------------------------------------------------
// VComponentManager::ResetComponentRequirements					[static]
// ---------------------------------------------------------------------------
// Reset checking for newly available/unavailable components
// Called automatically each time you register/unregister a component
//
Boolean VComponentManager::ResetComponentRequirements(CType inType, Boolean inBecomeAvailable)
{
	VIndex	componentPos = sComponentChecked->FindPos(inType);
	if (componentPos > 0 && inBecomeAvailable == !sComponentIsAvailable->GetNth(componentPos) &&
		(inBecomeAvailable || sComponentHasRequirements->GetNth(componentPos)))
	{
		sComponentChecked->DeleteNth(componentPos);
		sComponentIsAvailable->DeleteNth(componentPos);
		sComponentHasRequirements->DeleteNth(componentPos);
		
		return true;
	}
	
	return false;
}

	
// ---------------------------------------------------------------------------
// VComponentManager::RegisterComponentInstance						[static]
// ---------------------------------------------------------------------------
// Reference a component instance for later use. This allow shared components.
//
// Note: currently only registred instances can receive messages.
VError VComponentManager::RegisterComponentInstance(CComponent* inComponent)
{
	if (!IsInitialized()) return VE_COMP_UNITIALISED;
	if (inComponent == NULL) return VE_INVALID_PARAMETER;
	
	VError	error = VE_COMP_ALLREADY_REGISTRED;
	
	sAccessingInstances->Lock();
	
	if (sComponentInstances->FindPos(inComponent) <= 0)
	{
		sComponentInstances->AddTail(inComponent);
		error = VE_OK;
	}
	
	sAccessingInstances->Unlock();
	
	return error;
}


// ---------------------------------------------------------------------------
// VComponentManager::UnRegisterComponentInstance					[static]
// ---------------------------------------------------------------------------
// Remove a component instance to stop sharing it.
// Existing components still run until they're released.
//
VError VComponentManager::UnRegisterComponentInstance(CComponent* inComponent)
{
	if (!IsInitialized()) return VE_COMP_UNITIALISED;
	if (inComponent == NULL) return VE_INVALID_PARAMETER;
	
	sAccessingInstances->Lock();
	
	VError	error = VE_COMP_NOT_REGISTRED;
	sLONG	index = sComponentInstances->FindPos(inComponent);
	
	if (index > 0)
	{
		sComponentInstances->DeleteNth(index);
		error = VE_OK;
	}
	
	sAccessingInstances->Unlock();
		
	return error;
}


// ---------------------------------------------------------------------------
// VComponentManager::RegisterComponentLibrary						[static]
// ---------------------------------------------------------------------------
// Load all component creators from a given library. Make sure that the
// library exports the VComponentLibrary interface.
//
VError VComponentManager::RegisterComponentLibrary( const VFilePath& inLibraryFile)
{
	DebugMsg("VComponentManager load %V\n", &inLibraryFile.GetPath());
	if (!IsInitialized())
		return vThrowError(VE_COMP_UNITIALISED);
	
	VError	error = VE_OK;
	
	VLibrary*	library;
	VArrayIteratorOf<VLibrary*>	iterator(*sComponentLibraries);
	
	// Iterate through libraries
	while ((library = iterator.Next()) != NULL)
	{
		if (library->IsSameFile(inLibraryFile))
		{
			error = vThrowError(VE_COMP_ALLREADY_REGISTRED);
			break;
		}
	}
	
	if (library == NULL)
	{
		library = new VLibrary(inLibraryFile);
		
		if (library != NULL)
		{
			if (!library->Load())
			{
				error = vThrowError(VE_COMP_CANNOT_LOAD_LIBRARY);
				library->Release();
				library = NULL;
			}
		}
		
		if (library != NULL)
		{
			// Make sure the library export the needed interface
			MainProcPtr	mainPtr = (MainProcPtr) library->GetProcAddress(kDEFAULT_MAIN_SYMBOL);
			GetNthComponentTypeProcPtr	fetchPtr = (GetNthComponentTypeProcPtr) library->GetProcAddress(kDEFAULT_GET_TYPES_SYMBOL);
			CreateComponentProcPtr	creatorPtr = (CreateComponentProcPtr) library->GetProcAddress(kDEFAULT_CREATE_SYMBOL);
			
			// Fetch all implemented components
			if (creatorPtr != NULL && fetchPtr != NULL && mainPtr != NULL)
			{
				sAccessingTypes->Lock();
				sAccessingChecked->Lock();
				
				(mainPtr)(kDLL_EVENT_REGISTER, library);
					
				CType	newType;
				sLONG	index = 1;
				while ((fetchPtr)(index, newType))
				{
					VIndex	oldPos = sComponentTypes->FindPos(newType);
					if (oldPos > 0)
					{
						// Assumes its allready installed using ProcPtr
						assert(sComponentProcs->GetNth(oldPos) != NULL);
						assert(sComponentLibraries->GetNth(oldPos) == NULL);
						
						sComponentLibraries->SetNth(library, oldPos);
					}
					else
					{
						// Add entries
						sComponentLibraries->AddTail(library);
						sComponentProcs->AddTail(NULL);
						sComponentTypes->AddTail(newType);
					}
					
					// Increment library refcount for each type
					library->Retain();
					
					index++;
		
					// Reset checking for unavailable components
					ResetComponentRequirements(newType, false);
				}
				
				sAccessingChecked->Unlock();
				sAccessingTypes->Unlock();
			}
			else
			{
				error = VE_COMP_BAD_LIBRARY_TYPE;
			}
			
			// Release library ('new' balance)
			library->Release();
		}
		else
		{
			error = VE_MEMORY_FULL;
		}
	}
	
	return error;
}


// ---------------------------------------------------------------------------
// VComponentManager::UnRegisterComponentLibrary					[static]
// ---------------------------------------------------------------------------
// Remove all component types previously loaded from a library.
// Subsequent calls to RetainComponent won't be abble to use this lib.
// Existing components from that lib still run until they are released (and
//	the library code and datas will be released at that moment).
//
VError VComponentManager::UnRegisterComponentLibrary( const VFilePath& inLibraryFile)
{
	if (!IsInitialized()) return VE_COMP_UNITIALISED;
	
	VError	error = VE_COMP_LIBRARY_NOT_FOUND;
	Boolean	notified = false;
	
	VLibrary*	library;
	VArrayIteratorOf<VLibrary*>	iterator(*sComponentLibraries);
	
	sAccessingTypes->Lock();
	sAccessingChecked->Lock();
	
	// Iterate through libraries
	while ((library = iterator.Previous()) != NULL)
	{
		if (library->IsSameFile(inLibraryFile))
		{
			sLONG	index = iterator.GetPos();
			
			if (sComponentProcs->GetNth(index) == 0)
			{
				CType	componentType = sComponentTypes->GetNth(index);
				
				// No proc based equivalence, remove the component type
				sComponentLibraries->DeleteNth(index);
				sComponentProcs->DeleteNth(index);
				sComponentTypes->DeleteNth(index);
		
				// Reset checking for available components
				ResetComponentRequirements(componentType, true);
			}
			else
			{
				// If a proc based allocator exists, remove only library accessor
				sComponentLibraries->SetNth(NULL, index);
			}
			
			// Notify the library that it's no longer used
			if (!notified)
			{
				MainProcPtr	mainProcPtr = (MainProcPtr) library->GetProcAddress(kDEFAULT_MAIN_SYMBOL);
				(mainProcPtr)(kDLL_EVENT_UNREGISTER, NULL);
				notified = true;
			}
			
			// As we won't use the library for new components, we release it
			// Note: each component increased the library's ref count
			//	so the library will be really released when all components
			//	are released.
			library->Release();
		
			error = VE_OK;
		}
	}
	
	sAccessingChecked->Unlock();
	sAccessingTypes->Unlock();
	
	return error;
}


// ---------------------------------------------------------------------------
// VComponentManager::RegisterComponentCreator						[static]
// ---------------------------------------------------------------------------
// Install a component creator function. If a library based creator exists,
// the new one will overwrite it.
//
VError VComponentManager::RegisterComponentCreator(CType inType, CreateComponentProcPtr inProcPtr, Boolean inOverwritePrevious)
{
	if (!IsInitialized()) return VE_COMP_UNITIALISED;
	
	VError	error = VE_OK;
	
	sAccessingTypes->Lock();
	sAccessingChecked->Lock();
	
	// Make sure the proc isn't allready installed
	sLONG	index = sComponentTypes->FindPos(inType);
	if (index > 0 && !inOverwritePrevious && sComponentProcs->GetNth(index) == inProcPtr)
	{
		error = VE_COMP_ALLREADY_REGISTRED;
	}
	
	if (error == VE_OK)
	{
		if (index > 0)
		{
			// Replace old one
			sComponentProcs->SetNth(inProcPtr, index);
		}
		else
		{
			// Add new one at end of list
			sComponentProcs->AddTail(inProcPtr);
			sComponentTypes->AddTail(inType);
			sComponentLibraries->AddTail(NULL);
		
			// Reset checking for unavailable components
			ResetComponentRequirements(inType, false);
		}
	}
	
	sAccessingChecked->Unlock();
	sAccessingTypes->Unlock();
		
	return error;
}


// ---------------------------------------------------------------------------
// VComponentManager::UnRegisterComponentCreator					[static]
// ---------------------------------------------------------------------------
// Remove a component creator function. If a library based creator exists,
// it will become the defaut one.
//
VError VComponentManager::UnRegisterComponentCreator(CType inType, CreateComponentProcPtr inProcPtr)
{
	if (!IsInitialized()) return VE_COMP_UNITIALISED;
	
	VError	error = VE_COMP_LIBRARY_NOT_FOUND;
	
	sAccessingTypes->Lock();
	sAccessingChecked->Lock();
	
	// Make sure the proc is installed
	sLONG	index = sComponentTypes->FindPos(inType);
	if (index > 0 && sComponentProcs->GetNth(index) == inProcPtr)
	{
		VLibrary*	library = sComponentLibraries->GetNth(index);
		
		if (library == NULL)
		{
			sComponentLibraries->DeleteNth(index);
			sComponentProcs->DeleteNth(index);
			sComponentTypes->DeleteNth(index);
		
			// Reset checking for available components
			ResetComponentRequirements(inType, true);
		}
		else
		{
			library->Release();
			sComponentProcs->SetNth(NULL, index);
		}
		
		error = VE_OK;
	}
	
	sAccessingChecked->Unlock();
	sAccessingTypes->Unlock();
	
	return error;
}


// ---------------------------------------------------------------------------
// VComponentManager::UnRegisterComponentType						[static]
// ---------------------------------------------------------------------------
// Remove a given component type for all subsequent call to RetainComponent
// (use with caution as you won't be able to restore it)
//
VError VComponentManager::UnRegisterComponentType(CType inType)
{
	if (!IsInitialized()) return VE_COMP_UNITIALISED;
	
	VError	error = VE_COMP_NOT_REGISTRED;
	
	sAccessingTypes->Lock();
	
	// Make sure the component is registred
	sLONG	index = sComponentTypes->FindPos(inType);
	if (index > 0)
	{
		VLibrary*	library = sComponentLibraries->GetNth(index);
		if (library != NULL)
			library->Release();
			
		sComponentLibraries->DeleteNth(index);
		sComponentProcs->DeleteNth(index);
		sComponentTypes->DeleteNth(index);
		
		error = VE_OK;
	}
	
	sAccessingTypes->Unlock();
	
	return error;
}

	
// ---------------------------------------------------------------------------
// VComponentManager::SendMessage									[static]
// ---------------------------------------------------------------------------
// Send a message to the requested component(s).
//
#if 0
VError VComponentManager::SendMessage(CMessagePtr inMessage)
{
	if (!IsInitialized()) return VE_COMP_UNITIALISED;
	if (inMessage == NULL) return VE_INVALID_PARAMETER;
	
	VError	error = VE_OK;
	inMessage->resultError = VE_OK;
	
	if (inMessage->destinatorMask == kDEST_MASK_DEST_ONLY)
	{
		CComponent*	destinator = RetainComponent(inMessage->destinatorRef);
		if (destinator != NULL)
		{
			destinator->ListenToMessage(inMessage);
			destinator->Release();
		}
		else
		{
			error = VE_COMP_NOT_REGISTRED;
		}
	}
	else
	{
		/*
		CRef	commonRef = (inMessage->destinatorRef & inMessage->destinatorMask);
		
		if (commonRef != 0)
		{
			CComponent*		destinator = NULL;
			VArrayIteratorOf<CComponent*>	iterator(*sComponentInstances);
			// Iterate through instances
			while ((destinator = iterator.Next()) != NULL)
			{
				// Don't send to itself to avoid message loop
				if ((destinator->GetComponentRef() & inMessage->destinatorMask) == commonRef
					&& destinator->GetComponentRef() != inMessage->senderRef)
					destinator->ListenToMessage(inMessage);
			}
		}
		else
		{
			error = VE_COMP_INVALID_DESTINATOR;
		}
		*/
	}
	
	if (error != VE_OK)
		inMessage->resultError = error;
	
	return error;
}
#endif

// ---------------------------------------------------------------------------
// VComponentManager::Init											[static]
// ---------------------------------------------------------------------------
// Call once before using VComponentManager (tipically at startup)
//
Boolean VComponentManager::Init()
{
	if (!sIsInitialized)
	{
		sComponentProcs = new VArrayOf<CreateComponentProcPtr>;
		sComponentTypes = new VArrayOf<CType>;
		sComponentLibraries = new VArrayOf<VLibrary*>;
		sComponentInstances = new VArrayOf<CComponent*>;
		sComponentChecked = new VArrayOf<CType>;
		sComponentChecking = new VArrayOf<CType>;
		sComponentIsAvailable = new VArrayOf<Boolean>;
		sComponentHasRequirements = new VArrayOf<Boolean>;
		sAccessingTypes = new VCriticalSection;
		sAccessingInstances = new VCriticalSection;
		sAccessingChecked = new VCriticalSection;
		sAccessingChecking = new VCriticalSection;
		
		sIsInitialized = (	sComponentProcs != NULL
							&& sComponentLibraries != NULL
							&& sComponentTypes != NULL
							&& sComponentInstances != NULL
							&& sComponentChecked != NULL
							&& sComponentChecking != NULL
							&& sComponentIsAvailable != NULL
							&& sComponentHasRequirements != NULL
							&& sAccessingTypes != NULL
							&& sAccessingInstances != NULL
							&& sAccessingChecked != NULL
							&& sAccessingChecking != NULL);
	}
	
	return sIsInitialized;
}


// ---------------------------------------------------------------------------
// VComponentManager::DeInit										[static]
// ---------------------------------------------------------------------------
// Call once before exit
//
void VComponentManager::DeInit()
{
	if (sComponentProcs != NULL)
	{
		delete sComponentProcs;
		sComponentProcs = NULL;
	}
	
	if (sComponentLibraries != NULL)
	{
		delete sComponentLibraries;
		sComponentLibraries = NULL;
	}
	
	if (sComponentTypes != NULL)
	{
		delete sComponentTypes;
		sComponentTypes = NULL;
	}
	
	if (sComponentInstances != NULL)
	{
	#if CHECK_COMPONENT_RELEASE
		assert(sComponentInstances->GetCount() == 0);
	#endif
		
		delete sComponentInstances;
		sComponentInstances = NULL;
	}
	
	if (sComponentChecked != NULL)
	{
		delete sComponentChecked;
		sComponentChecked = NULL;
	}
	
	if (sComponentChecking != NULL)
	{
		delete sComponentChecking;
		sComponentChecking = NULL;
	}
	
	if (sComponentIsAvailable != NULL)
	{
		delete sComponentIsAvailable;
		sComponentIsAvailable = NULL;
	}
	
	if (sComponentHasRequirements != NULL)
	{
		delete sComponentHasRequirements;
		sComponentHasRequirements = NULL;
	}
	
	if (sAccessingTypes != NULL)
	{
		delete sAccessingTypes;
		sAccessingTypes = NULL;
	}
	
	if (sAccessingInstances != NULL)
	{
		delete sAccessingInstances;
		sAccessingInstances = NULL;
	}
	
	if (sAccessingChecked != NULL)
	{
		delete sAccessingChecked;
		sAccessingChecked = NULL;
	}
	
	if (sAccessingChecking != NULL)
	{
		delete sAccessingChecking;
		sAccessingChecking = NULL;
	}
	
	sIsInitialized = false;
}



/*
	static
*/
DialectCode	VComponentManager::GetLocalizationLanguage(VFolder * inLocalizationResourcesFolder,bool inGotoResourceFolder)
{
	DialectCode sResult = XBOX::DC_NONE;	// sc 19/05/2008 was XBOX::DC_ENGLISH_US
	VFolder * localizationResourcesFolder = NULL;
	
	if(testAssert(inLocalizationResourcesFolder != NULL && inLocalizationResourcesFolder->Exists()))
	{
		if (inGotoResourceFolder)
		{
			XBOX::VFilePath componentOrPluginFolderPath = inLocalizationResourcesFolder->GetPath();
			componentOrPluginFolderPath.ToSubFolder(CVSTR("Contents")).ToSubFolder(CVSTR("Resources"));
			localizationResourcesFolder = new XBOX::VFolder(componentOrPluginFolderPath);
		}
		else
		{
			localizationResourcesFolder = inLocalizationResourcesFolder;
			localizationResourcesFolder->Retain();
		}

		bool englishFolderDetected = false;
		XBOX::DialectCode dialectCode = XBOX::DC_NONE;
		
		//Detect what is the favorite language of the OS/App
#if VERSIONWIN
		LCID lcid = ::GetUserDefaultLCID();
		XBOX::DialectCode currentDialectCode = (XBOX::DialectCode)LANGIDFROMLCID(lcid);

#elif VERSION_LINUX

		//jmo - Be coherent with code in VIntlMgr.cpp
		XBOX::DialectCode currentDialectCode=XBOX::DC_ENGLISH_US;   // Postponed Linux Implementation !
		
#else

		CFBundleRef	bundle = ::CFBundleGetMainBundle();
		XBOX::DialectCode currentDialectCode = XBOX::DC_ENGLISH_US;
		if ( bundle != nil ){
			CFArrayRef array_ref = ::CFBundleCopyBundleLocalizations(bundle);			
			if (array_ref != nil){
				CFArrayRef usedLanguages = ::CFBundleCopyPreferredLocalizationsFromArray(array_ref);
				CFStringRef cfPrimaryLanguage = (CFStringRef)CFArrayGetValueAtIndex(usedLanguages, 0);
				XBOX::VString xboxPrimaryLanguage;
				xboxPrimaryLanguage.MAC_FromCFString(cfPrimaryLanguage);
				::CFRelease(usedLanguages);
				if(!XBOX::VIntlMgr::GetDialectCodeWithISOLanguageName(xboxPrimaryLanguage, currentDialectCode))
					if(!XBOX::VIntlMgr::GetDialectCodeWithRFC3066BisLanguageCode(xboxPrimaryLanguage, currentDialectCode))
						currentDialectCode = XBOX::DC_ENGLISH_US;
				CFRelease(array_ref);
			}
		}
#endif
		//Try to see if we have this language. If not, take english
		for ( XBOX::VFolderIterator folderIter( localizationResourcesFolder ); folderIter.IsValid() && currentDialectCode != dialectCode ; ++folderIter )
		{
			XBOX::VString folderName;
			folderIter->GetName(folderName);
			uLONG posStr = folderName.Find(CVSTR(".lproj"), 1, false);
			if ( posStr > 0 )
			{
				folderName.Remove(posStr, folderName.GetLength() - posStr + 1);
				if( XBOX::VIntlMgr::GetDialectCodeWithRFC3066BisLanguageCode(folderName, dialectCode ) || XBOX::VIntlMgr::GetDialectCodeWithISOLanguageName(folderName, dialectCode)){
					if(dialectCode == XBOX::DC_ENGLISH_US)
						englishFolderDetected = true;
					
					if(currentDialectCode == dialectCode)
						sResult = dialectCode;
				}
			}
		}
		
		if ( sResult == XBOX::DC_NONE ){
			if ( englishFolderDetected ) 
				sResult = XBOX::DC_ENGLISH_US;
			else 
				sResult = dialectCode;
		}
		
		ReleaseRefCountable(&localizationResourcesFolder);
	}
	
	return sResult;
}



END_TOOLBOX_NAMESPACE

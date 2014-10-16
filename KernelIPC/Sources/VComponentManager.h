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
#ifndef __VComponentManager__
#define __VComponentManager__

#include "KernelIPC/Sources/CComponent.h"

BEGIN_TOOLBOX_NAMESPACE

// Needed declaration
class VComponentLibrary;


/*!
	@class	VComponentManager
	@abstract	Component instanciation class
	@discussion
		Components implementation can be defined as 'inline' code (ie hosted in an app)
		or stored in dynamic libraries.
		
		For 'inline' components use RegisterComponentCreator to register your class.
		
		For lib-based components use RegisterComponentLibrary to register your classes.
		Library created with VComponentLibrary have a <A HREF='/GoldFingerDoc/XToolBox/Kernel/ComponentRegistering.jpg'>fully automated registration
		mechanism</A>. Other library  must export two specific functions (see Component
		and VComponentLibrary for explanations).
		
		To unload any component use UnRegisterComponentCreator, UnRegisterComponentLibrary
		and UnRegisterComponentType, according to the way you registered your component.
		
		Note that your can 'override' a component by specifying an 'inline' component over
		a lib-based component. In this case the lib-based component will be skipped as
		long as the inline component is available.
		
		To get a component instance use RetainComponent or RetainUniqueComponent (<A HREF='/GoldFingerDoc/XToolBox/Kernel/ComponentLoading.jpg'>mechanism</A>).
		To get all components of a familly use RetainComponentsOfFamily.
		To check if all requirements of a component are verified use CheckComponentRequirements
		(this is usually done automatically).
		
		To use the component messaging simply call SendMessage. To get the resource fork
		of a component use myComponent->GetResourceFile() (see also VComponent.h for
		explanations about those topics).
		
		All functions of this class are static so you don't have to instanciate
		any VComponentManager object.
*/
class XTOOLBOX_API VComponentManager
{
public:
	/*!
		@function	RetainComponent
		@abstract	Component creation 
		@discussion
			Use this function whenever you want to use a component. If this type is
			registred and the creation has been completed, it returns a component
			instance. Otherwise it returns NULL. If you specify a family, the component
			must belong to that family to complete.
			
			Note that if a component of that type has allready been instanciated and
			is still in use (not released) the component manager will return the
			considered instance (in this case it only increase the component ref-count).
			
			If you want to get explicitly an independant instance use RetainUniqueComponent.
			
			Note: you can write "MyComponent* comp = RetainComponent<MyComponent>()" for
			convenience (avoid casting input and output values).
		@param	inType The type of component you want to use.
		@param	inFamily An optionnal family the component should belong to.
		@result	Returns a component instance if all was ok, NULL otherwise.
	*/
	static CComponent*	RetainComponent (CType inType, CType inFamily = kCOMPONENT_FAMILY_ANY);

	template <class Type>
	static Type*	RetainComponentOfType (CType inFamily = kCOMPONENT_FAMILY_ANY) { return (Type*) RetainComponent(Type::Component_Type, inFamily); };

	template <class Type>
	static Type*	RetainComponentOfFamily (CType inType) { return (Type*) RetainComponent(inType, Type::Component_Family); };

#if USE_OBSOLETE_API
	template <class Type>
	static Type*	RetainComponent (CType inFamily = kCOMPONENT_FAMILY_ANY) { return (Type*) RetainComponent((CType) Type::Component_Type, inFamily); };
#endif

	/*!
		@function	RetainComponent
		@abstract	Component retreiving
		@discussion
			Use this function if you want to retreive a component instance from it's
			component ref. This function is only valid for component instances that
			haven't been unregistred (see UnRegisterComponentInstance).
			
			If no instance is available the function returns NULL.
		@param	inRef The component ref whose instance you're looking for (see also CRef definition).
		@result	Returns the component instance if available, NULL otherwise.
	*/
//	static CComponent*	RetainComponent (CRef inRef);

	/*!
		@function	RetainUniqueComponent
		@abstract	Unique component creation
		@discussion
			This function is similar to RetainComponent exept that it doesn't look for existing
			instances and allways returns a new one.
			
			In the same way the new instance isn't registred (see RegisterComponentInstance)
			so it ensures that nobody than the caller will use it.
		@param	inType The type of component you want to use.
		@param	inFamily An optionnal family to which you want to affect the instance.
		@param	inSkipRequirements Specify that you don't care about requirements.
		@result	Returns a component instance if all was ok, NULL otherwise.
	*/
	static CComponent*	RetainUniqueComponent (CType inType, CType inFamily = kCOMPONENT_FAMILY_ANY, Boolean inSkipRequirements = false);
	

	/*!
		@function	RetainComponentsOfFamily
		@abstract	Component family creation
		@discussion
			This function returns a list of components of a given family (similar to
			call RetainComponent for each component type specifying a family).
			
			Each instance will be registred and allready instanciated components will be
			returned incrementing their refcount.
		@param	inFamily An optionnal family to which you want to affect the instance.
		@param	ioComponentList An optionnal family to which you want to affect the instance.
		@result	Returns a component instance if all was ok, NULL otherwise.
	*/

	static Boolean	RetainComponentsOfFamily (CType inFamily, VArrayOf<CComponent*>* ioComponentList);

	/*!
		@function	RegisterComponentLibrary
		@abstract	Registration of all component in a library
		@discussion
			Use this function to register all components hosted in a dynamic
			library. This library should export the two requested functions
			from the component API (see VComponent.h for explanations about
			those funtions).
		@param	inLibraryFile A file or bundle specification for a dynamic library (DLL).
		@result	Returns an error code.
	*/
	static VError	RegisterComponentLibrary (const VFilePath& inLibraryFile);

	/*!
		@function	UnRegisterComponentLibrary
		@abstract	Removing registration for all component of a library
		@discussion
			This function do the reverse operation of RegisterComponentLibrary.
			
			You should call it before changing any component library as it will
			unload all datas. Note that the unregistration will fail if component
			instances handled by the lib are still in use.
		@param	inLibraryFile A file specification for a dynamic library (DLL).
		@result	Returns an error code.
	*/
	static VError	UnRegisterComponentLibrary (const VFilePath& inLibraryFile);
	

	/*!
		@function	RegisterComponentCreator
		@abstract	Registration of an application hosted component creator
		@discussion
			Use this function to register a component creator using a funtion pointer.
			This allows to define a component that is not hosted in a library. In this case
			you have to handle the component creation (see VComponent.h for explanations).
			
			Note that you can override a previously registred creator even if it was registred
			from a library. This allows you to 'override' a component.
		@param	inType The type of component your creator is able to create.
		@param	inProcPtr The function pointer to your creator (see VComponent.h).
		@param	inOverwritePrevious Specify if your creator should override the previous one, if any.
		@result	Returns an error code.
	*/
	static VError	RegisterComponentCreator (CType inType, CreateComponentProcPtr inProcPtr, Boolean inOverwritePrevious = true);

	/*!
		@function	UnRegisterComponentCreator
		@abstract	Removing registration of a component creator
		@discussion
			This function do the reverse operation of RegisterComponentCreator.
			
			If your component overrided a library based creator, it will become available
			again as the default creator.
		@param	inType The type of component for whose your want to remove the creator.
		@param	inProcPtr The function pointer to your creator.
		@result	Returns an error code.
	*/
	static VError	UnRegisterComponentCreator (CType inType, CreateComponentProcPtr inProcPtr);
	

	/*!
		@function	UnRegisterComponentType
		@abstract	Removing any component creator for a specific type
		@discussion	
		@param	inType The type of the component you want to remove.
		@result	Returns an error code.
	*/
	static VError	UnRegisterComponentType (CType inType);

	/*!
		@function	IsComponentAvailable
		@abstract	Checking for component type availability
		@discussion
			Use this function whenever you want to know if a specific component type
			has been registred (usefull to handle dependencies - avoid creating a component).
			
			Note that even if this function returns true you may get NULL as result of
			RetainComponent if any error occured during component creation.
		@param	inType The type of the component you want to check
		@result	Returns true if the component manager has registred a creator for that type.
	*/
	static Boolean	IsComponentAvailable (CType inType);

	/*!
		@function	CheckComponentRequirements
		@abstract	Checking for component requirements availability
		@discussion
			Use this function whenever you want to know if a specific component is
			able to resolve its dependencies (other components and/or versions).
			
			Note: if you specify true for inRetainComponents and the function returns true,
			you're responsible of releasing all dependencies.
		@param	inComponent The component you want to check.
		@param	inRetainComponents Specify if all searched components should be retained.
		@result	Returns true if the component can resolve all its dependencies.
	*/
	static Boolean	CheckComponentRequirements (CComponent* inComponent, Boolean inRetainComponents = false);

	/*!
		@function	ResetComponentRequirements
		@abstract	Reseting component requirements cache
		@discussion
			Use this function whenever you want to reset requirement cache for a component type.
			You tipically need to call this function when you change dynamically the
			requirements of a component. It is automatically called if you register
			or unregister a component.
			
			Note: if you update requirements of a registred component type, you should
			specify true as inBecomeAvailable if you removed dependencies or false
			if you added dependencies.
		@param	inType The component type for which you want to clear the cache.
		@param	inBecomeAvailable Specify if the component becomes available.
		@result	Returns true if the cache was modified.
	*/
	static Boolean	ResetComponentRequirements (CType inType, Boolean inBecomeAvailable);
	
	/*!
		@function	RegisterComponentInstance
		@abstract	Registration of a component instance
		@discussion
			You generally don't need to call this function as it is called automatically
			by the component manager when it creates a new component.
			Once regitred, the instance may be used by the component manager on any
			RetainComponent call.
		@param	inComponent The component instance to be registred.
		@result	Returns an error code.
	*/
	static VError	RegisterComponentInstance (CComponent* inComponent);

	/*!
		@function	UnRegisterComponentInstance
		@abstract	Removing registration of a component instance
		@discussion
			This function as it is called automatically by the component manager
			when it creates a new component. You may however use it if you want
			to make sure that the instance is no longer used by the component manager
			an RetainComponent calls.
		@param	inComponent The component instance to be unregistred.
		@result	Returns an error code.
	*/
	static VError	UnRegisterComponentInstance (CComponent* inComponent);
	

	/*!
		@function	SendMessage
		@abstract	Component messaging handling
		@discussion	See also CMessageBlock definition (VComponent.h)
		@param	inMessage The Message Block that describe the message to send and how to send it.
		@result	Returns an error code.
	*/
	//static VError	SendMessage (CMessagePtr inMessage);


	/*!
		@function	Init
		@abstract	Manager initialization
		@discussion
		 	You should call this function before calling any other VComponentManager
		 	function. The Toolbox automatically call this at launch.
		@result	Returns true if succeeded.
	*/
	static Boolean	Init ();

	/*!
		@function	DeInit
		@abstract	Manager deinitialization
		@discussion
			You should call this function to release all datas created by the
			component manager.
	*/
	static void		DeInit ();

	/*!
		@function	IsInitialized
		@abstract	Check manager initialized state
		@result	Returns true if the class has been correctly initialized.
	*/
	static Boolean	IsInitialized () { return sIsInitialized; };

	static	DialectCode		GetLocalizationLanguage(VFolder * inLocalizationResourcesFolder,bool inGotoResourceFolder);
	
protected:
	friend class VComponentLibrary;
	
	static VArrayOf<CreateComponentProcPtr>*	sComponentProcs;// Lists for component types
	static VArrayOf<VLibrary*>*		sComponentLibraries;		// handling (could be replaced	
	static VArrayOf<CType>*			sComponentTypes;			// with 3D lists).
	static VArrayOf<CComponent*>*	sComponentInstances;		// List of created components
	static VArrayOf<CType>*			sComponentChecking;			// List of components beign checking (for requirements)
	static VArrayOf<CType>*			sComponentChecked;			// List of checked components
	static VArrayOf<Boolean>*		sComponentIsAvailable;		// Specify if checking succeeded
	static VArrayOf<Boolean>*		sComponentHasRequirements;	// Specify if components have req. (CAUTION: only valid while req. needs are statics)
	
	static VCriticalSection*	sAccessingTypes;
	static VCriticalSection*	sAccessingInstances;
	static VCriticalSection*	sAccessingChecked;
	static VCriticalSection*	sAccessingChecking;
	static Boolean	sIsInitialized;
	
	static void		ComponentOfTypeCreated (CType inType);
	static void		ComponentOfTypeDisposed (CType inType);
};

END_TOOLBOX_NAMESPACE

#endif

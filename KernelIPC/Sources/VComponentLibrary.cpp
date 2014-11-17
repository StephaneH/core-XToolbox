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
// CodeWarrior seems to 'loose' some flags with Mach-O compiler - JT 6/08/2002

#ifndef USE_AUTO_MAIN_CALL
#define USE_AUTO_MAIN_CALL		0	// Set to 1 to let the system init/deinit the lib (via main)
#endif

#include "VKernelIPCPrecompiled.h"
#include "VComponentLibrary.h"
 

// Receipe:
//
// 1) To write a component library, define your component class deriving it
// from CComponent. You will provide this class with your library as a header file.
//
// class CComponent1 : public CComponent
// {
//	public:
//	enum { Component_Type = 'cmp1' };
//
//	Blah, blah - Public Interface
// };
//
// Note that a component has no constructor, no data and only pure virtuals (= 0)
//
// 2) Then define a custom class deriving from VComponentImp<YourComponent>
// and implement your public and private functions. If you want to receive messages,
// override ListenToMessage(). You may define several components using the same pattern.
//
// class VComponentImp1 : public VComponentImp<CComponent1>
// {
//	public:
//	Blah, blah - Public Interface
//
//	protected:
//	Blah, blah - Private Stuff
// };
//
// 3) Declare a kCOMPONENT_TYPE_LIST constant.
// This constant will automate the CreateComponent() in the dynamic lib:
//
// const sLONG			kCOMPONENT_TYPE_COUNT	= 1;
// const CImpDescriptor	kCOMPONENT_TYPE_LIST[]	= { { CComponent1::Component_Type,
//													  VImpCreator<VComponentImp1>::CreateImp } };
//
// 4) Finally define a xDllMain and instantiate a VComponentLibrary* object (with the new operator).
// You'll pass your kCOMPONENT_TYPE_LIST array to the constructor. If you need custom library
// initialisations, you can write your own class and override contructor, DoRegister, DoUnregister...
//
// On Mac remember to customize library (PEF) entry-points with __EnterLibrary and __LeaveLibrary


// User's main declaration
// You should define it in your code and instanciate a VComponentLibrary* object.
//
// void xbox::ComponentMain (void)
// {
// 	new VComponentLibrary(kCOMPONENT_TYPE_LIST, kCOMPONENT_TYPE_COUNT);
// }


// Statics declaration
VComponentLibrary*	VComponentLibrary::sComponentLibrary = NULL;
Boolean				VComponentLibrary::sQuietUnloadLibrary = false;


// Shared lib initialisation
// On Windows, all job is done by the DllMain function
// On Mac/CFM, __initialize and __terminate PEF entries must be set to __EnterLibrary and __LeaveLibrary
// On Mac/Mach-O, job is done in the main entry point.
//
// NOTE: currently the ComponentManager call the main entry-point manually to ensure
// 	the Toolbox is initialized before components (USE_AUTO_MAIN_CALL flag)
#if VERSIONWIN
BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	BOOL	result = true;
	
	#if USE_AUTO_MAIN_CALL
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
			xDllMain();	// To be renamed ComponentMain();
			result = (VComponentLibrary::GetComponentLibrary() != NULL);
			break;

		case DLL_PROCESS_DETACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
	}
	#else
	if (fdwReason == DLL_PROCESS_ATTACH)
		XWinStackCrawl::RegisterUser( hinstDLL);
	#endif

	return result;
}
#elif VERSION_LINUX

//jmo - Be coherent with code in XLinuxDLLMain.cpp
extern "C"
{
    void __attribute__ ((constructor)) LibraryInit();
    void __attribute__ ((destructor)) LibraryFini();
}


void LibraryInit()
{
#if USE_AUTO_MAIN_CALL
	xDllMain();
    bool ok=VComponentLibrary::GetComponentLibrary()!=NULL;
    //jmo - I lack ideas, but I'm sure we can do something clever with this 'ok' result !
#endif
}

void LibraryFini()
{
    //Nothing to do.
}

#elif VERSIONMAC

	#if USE_AUTO_MAIN_CALL
	#error "Mach-O main entry point isn't defined"
	#endif

#endif	// VERSIONMAC


// Unmangled export mecanism
//
#if USE_UNMANGLED_EXPORTS
EXPORT_UNMANGLED_API(VError)	Main (OsType inEvent, VLibrary* inLibrary);
EXPORT_UNMANGLED_API(void)		Lock ();
EXPORT_UNMANGLED_API(void)		Unlock ();
EXPORT_UNMANGLED_API(Boolean)	CanUnloadLibrary ();
EXPORT_UNMANGLED_API(CComponent*)	CreateComponent (CType inType, CType inFamily);
EXPORT_UNMANGLED_API(Boolean)		GetNthComponentType (sLONG inTypeIndex, CType& outType);

EXPORT_UNMANGLED_API(CComponent*) CreateComponent (CType inType, CType inFamily)
{
	return VComponentLibrary::CreateComponent(inType, inFamily);
}

EXPORT_UNMANGLED_API(Boolean) GetNthComponentType (sLONG inTypeIndex, CType& outType)
{
	return VComponentLibrary::GetNthComponentType(inTypeIndex, outType);
}

EXPORT_UNMANGLED_API(void) Lock ()
{
	VComponentLibrary::Lock();
}

EXPORT_UNMANGLED_API(void) Unlock ()
{
	VComponentLibrary::Unlock();
}

EXPORT_UNMANGLED_API(Boolean) CanUnloadLibrary ()
{
	return VComponentLibrary::CanUnloadLibrary();
}

EXPORT_UNMANGLED_API(VError) Main (OsType inEvent, VLibrary* inLibrary)
{
	return VComponentLibrary::Main(inEvent, inLibrary);
}
#endif	// USE_UNMANGLED_EXPORTS


// ---------------------------------------------------------------------------
// VComponentLibrary::VComponentLibrary
// ---------------------------------------------------------------------------
// 
VComponentLibrary::VComponentLibrary()
{
	Init();
}


// ---------------------------------------------------------------------------
// VComponentLibrary::VComponentLibrary
// ---------------------------------------------------------------------------
// 
VComponentLibrary::VComponentLibrary(const CImpDescriptor* inTypeList, sLONG inTypeCount)
{
	Init();
	
	fComponentTypeList = inTypeList;
	fComponentTypeCount = inTypeCount;
}


// ---------------------------------------------------------------------------
// VComponentLibrary::~VComponentLibrary
// ---------------------------------------------------------------------------
// 
VComponentLibrary::~VComponentLibrary()
{
	xbox_assert(sComponentLibrary == this);
	sComponentLibrary = NULL;
	
	// Make sure all instances has been released
	if (!sQuietUnloadLibrary)
	{
		xbox_assert(fDebugInstanceCount == 0);
	}
}


// ---------------------------------------------------------------------------
// VComponentLibrary::Main									[static][exported]
// ---------------------------------------------------------------------------
// Cross-platform main entry point
//
VError VComponentLibrary::Main(OsType inEvent, VLibrary* inLibrary)
{
	VError	error = VE_COMP_CANNOT_LOAD_LIBRARY;
	
#if !USE_AUTO_MAIN_CALL
	// Call the user's main to allow custom initialization
	// This is done here if not handled automatically
	if (inEvent == kDLL_EVENT_REGISTER)
		xDllMain();
#endif
	
	VComponentLibrary*	componentLibrary = VComponentLibrary::GetComponentLibrary();
	if (componentLibrary == NULL)
		DebugMsg(L"You must instantiate a VComponentLibrary* in your xDllMain\n");
	
	if (componentLibrary != NULL)
	{
		switch (inEvent)
		{
			case kDLL_EVENT_REGISTER:
				componentLibrary->fLibrary = inLibrary;
				componentLibrary->Register();
				error = VE_OK;
				break;
			
			case kDLL_EVENT_UNREGISTER:
				componentLibrary->Unregister();
				error = VE_OK;
				break;
		}
	}
	
	return error;
}


// ---------------------------------------------------------------------------
// VComponentLibrary::Lock									[static][exported]
// ---------------------------------------------------------------------------
// Call when you want to block component creation (typically before
// releasing the library)
//
void
VComponentLibrary::Lock()
{
	if (sComponentLibrary == NULL)
	{
		DebugMsg(L"You must customize library entry points\n");
		return;
	}
	
	VTaskLock	locker(&sComponentLibrary->fCreatingCriticalSection);
	sComponentLibrary->fLockCount++;
}


// ---------------------------------------------------------------------------
// VComponentLibrary::Unlock								[static][exported]
// ---------------------------------------------------------------------------
// Call if you no longer want to block component creation
//
void
VComponentLibrary::Unlock()
{
	if (sComponentLibrary == NULL)
	{
		DebugMsg(L"You must customize library entry points\n");
		return;
	}
	
	VTaskLock	locker(&sComponentLibrary->fCreatingCriticalSection);
	sComponentLibrary->fLockCount--;
}


// ---------------------------------------------------------------------------
// VComponentLibrary::CanUnloadLibrary						[static][exported]
// ---------------------------------------------------------------------------
// Call to known if library can be unloaded. Make sure you locked before.
//
// This code is automatically called while the library is unloading:
// 1) if the library is unloaded by the toolbox, we're called from VLibrary's destructor
// 2) if the library is unloaded by the OS, the VLibrary hasn't been deleted
// In any cases library has still valid datas (globals).
Boolean
VComponentLibrary::CanUnloadLibrary()
{
	if (sComponentLibrary == NULL)
	{
		DebugMsg(L"You must customize library entry points\n");
		return false;
	}
	
	// Each component in the library as increased the refcount (will be decreased later
	// by the Component Manager). A greater refcount means some component instances are
	// still running.
	VLibrary*	library = sComponentLibrary->fLibrary;
	return ((library != NULL) ? library->GetRefCount() == sComponentLibrary->fComponentTypeCount : true);
}


// ---------------------------------------------------------------------------
// VComponentLibrary::CreateComponent						[static][exported]
// ---------------------------------------------------------------------------
// Entry point in the library for creating any component it defines
//
CComponent*
VComponentLibrary::CreateComponent(CType inType, CType inGroup)
{
	if (sComponentLibrary == NULL)
	{
		DebugMsg(L"You must customize library entry points\n");
		return NULL;
	}
	
	if (sComponentLibrary->fComponentTypeList == NULL)
	{
		DebugMsg(L"You must specify a TypeList for your lib\n");
		return NULL;
	}
	
	CComponent*	component = NULL;
	sLONG		typeIndex = 0;
		
	// Look for the type index
	while (sComponentLibrary->fComponentTypeList[typeIndex].componentType != inType
			&& typeIndex < sComponentLibrary->fComponentTypeCount)
	{
		typeIndex++;
	}
	
	// If found, instanciate the component using factory's member address
	if (sComponentLibrary->fComponentTypeList[typeIndex].componentType == inType)
	{
		VTaskLock	locker(&sComponentLibrary->fCreatingCriticalSection);
		
		// Make sure component creation isn't locked
		if (sComponentLibrary->fLockCount == 0)
			component = (sComponentLibrary->fComponentTypeList[typeIndex].impCreator)(inGroup);
	}
	
	return component;
}


// ---------------------------------------------------------------------------
// VComponentLibrary::GetNthComponentType					[static][exported]
// ---------------------------------------------------------------------------
// Return the list of component types it may create
///
Boolean
VComponentLibrary::GetNthComponentType(sLONG inTypeIndex, CType& outType)
{
	if (sComponentLibrary == NULL)
	{
		DebugMsg(L"You must customize library entry points\n");
		return false;
	}
	
	if (sComponentLibrary->fComponentTypeList == NULL)
	{
		DebugMsg(L"You must specify a TypeList for your lib\n");
		return false;
	}
	
	if (inTypeIndex < 1 || inTypeIndex > sComponentLibrary->fComponentTypeCount)
		return false;
	
	outType = sComponentLibrary->fComponentTypeList[inTypeIndex - 1].componentType;
	return true;
}


// ---------------------------------------------------------------------------
// VComponentLibrary::RegisterOneInstance(CType inType)
// ---------------------------------------------------------------------------
// Increase component instance count
//
VIndex VComponentLibrary::RegisterOneInstance(CType /*inType*/)
{
	// Increase library's refcount as the component may use global datas
	if (fLibrary != NULL)
		fLibrary->Retain();
	
	fDebugInstanceCount++;
	
	return VInterlocked::Increment(&fUniqueInstanceIndex);
}


// ---------------------------------------------------------------------------
// VComponentLibrary::UnRegisterOneInstance(CType inType)
// ---------------------------------------------------------------------------
// Decrease component instance count
//
void VComponentLibrary::UnRegisterOneInstance(CType /*inType*/)
{
	// Decrease library's refcount as the component no longer need it
	if (fLibrary != NULL)
		fLibrary->Release();
		
	fDebugInstanceCount--;
}


// ---------------------------------------------------------------------------
// VComponentLibrary::Register
// ---------------------------------------------------------------------------
// Entry point for library initialisation
//
void VComponentLibrary::Register()
{
	DoRegister();
}


// ---------------------------------------------------------------------------
// VComponentLibrary::Unregister	
// ---------------------------------------------------------------------------
// Entry point for library termination
//
void VComponentLibrary::Unregister()
{
	DoUnregister();
}


// ---------------------------------------------------------------------------
// VComponentLibrary::DoRegister
// ---------------------------------------------------------------------------
// Override to handle custom initialisation
//
void VComponentLibrary::DoRegister()
{
}


// ---------------------------------------------------------------------------
// VComponentLibrary::DoUnregister	
// ---------------------------------------------------------------------------
// Override to handle custom termination
//
void VComponentLibrary::DoUnregister()
{
}


// ---------------------------------------------------------------------------
// VComponentLibrary::Init											[private]
// ---------------------------------------------------------------------------
//
void VComponentLibrary::Init()
{
	fLockCount = 0;
	fUniqueInstanceIndex = 0;
	fComponentTypeList = NULL;
	fComponentTypeCount = 0;
	fLibrary = NULL;
	fDebugInstanceCount = 0;
	
	if (sComponentLibrary != NULL)
		DebugMsg(L"You must instanciate only ONE VComponentLibrary object\n");
		
	sComponentLibrary = this;
}

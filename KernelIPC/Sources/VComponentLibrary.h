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
#ifndef __VComponentLibrary__
#define __VComponentLibrary__

#include "Kernel/VKernel.h"
#include "KernelIPC/Sources/CComponent.h"


BEGIN_TOOLBOX_NAMESPACE


// Needed declarations
class VLibrary;


// Custom main declaration
void	xDllMain ();	// JT : to be renamed ComponentMain to respect calling convention


#if USE_UNMANGLED_EXPORTS
#define ENTRY_POINT 	
#else
#define ENTRY_POINT	EXPORT_API
#endif


// VComponentLibrary is the class defining exported functions and library entry points.
// It should be instantiated once at library launch.
class VComponentLibrary : public VObject
{
public:
			VComponentLibrary (const CImpDescriptor* inTypeList, sLONG inTypeCount);
	virtual	~VComponentLibrary ();

	// Instance support
	VIndex	RegisterOneInstance (CType inType);
	void	UnRegisterOneInstance (CType inType);
	
	// Library support
	void	Register ();
	void	Unregister ();
	
	// Accessors
	VLibrary*	GetLibrary () { return fLibrary; };
	sLONG		GetInstanceCount () { return fDebugInstanceCount; };
	Boolean		IsLocked () { return (fLockCount > 0); };
	
	// Exported entry-points
	ENTRY_POINT static VError	Main (OsType inEvent, VLibrary* inLibrary);
	ENTRY_POINT static void		Lock ();
	ENTRY_POINT static void		Unlock ();
	ENTRY_POINT static Boolean	CanUnloadLibrary ();
	
	ENTRY_POINT static CComponent*	CreateComponent (CType inType, CType inFamily);
	ENTRY_POINT static Boolean		GetNthComponentType (sLONG inTypeIndex, CType& outType);
	
	// Static utilities
	static VComponentLibrary*	GetComponentLibrary () { return sComponentLibrary; };
	static Boolean		sQuietUnloadLibrary;
	
protected:
	VLibrary*	fLibrary;
	VIndex		fLockCount;
	VIndex		fUniqueInstanceIndex;
	sLONG		fComponentTypeCount;
	sLONG		fDebugInstanceCount;
	const CImpDescriptor*	fComponentTypeList;
	VCriticalSection		fCreatingCriticalSection;
	
	static	VComponentLibrary*	sComponentLibrary;
	
	// Library support (override to customize initialisation)
	virtual void	DoRegister ();
	virtual void	DoUnregister ();
	
private:
			VComponentLibrary ();	// Hidden to force using non default constructor
	void	Init ();
};

END_TOOLBOX_NAMESPACE

#endif

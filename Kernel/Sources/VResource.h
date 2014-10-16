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
#ifndef __VResource__
#define __VResource__

#include "Kernel/Sources/IRefCountable.h"
#include "Kernel/Sources/VIterator.h"
#include "Kernel/Sources/VString.h"
#include "Kernel/Sources/VArrayValue.h"


BEGIN_TOOLBOX_NAMESPACE

// Needed declarations
class VFilePath;
class VBlob;


// Defined bellow
class VResourceIterator;

// Class definitions
typedef VOsTypeString	VResTypeString;


class XTOOLBOX_API VResourceFile : public VObject, public IRefCountable
{ 
public:
			VResourceFile ();
	virtual ~VResourceFile ();
	
	virtual VError	Open (const VFilePath& inPath, FileAccess inFileAccess, Boolean inCreate) = 0;
	virtual VError	Close () = 0;
	
	// reads a resource data, return NULL if not found. You're the owner of the handle.
	virtual VHandle	GetResource (const VString& inType, sLONG inID) const = 0;
	virtual VHandle	GetResource (const VString& inType, const VString& inName) const = 0;
	virtual bool	GetResource ( const VString& inType, sLONG inID, VBlob& outData) const = 0;

	// does this resource exists ? (by ID)
	virtual Boolean	ResourceExists (const VString& inType, sLONG inID) const = 0;
	virtual Boolean	ResourceExists (const VString& inType, const VString& inName) const = 0;

	// returns the size of a resource data
	virtual VError	GetResourceSize (const VString& inType, sLONG inID, sLONG* outSize) const = 0;
	virtual VError	GetResourceSize (const VString& inType, const VString& inName, sLONG *outSize) const = 0;
	
	// Creates a new resource (overwrites existing one)
	// inData is optionnal (may be NULL)
	// inName is optionnal (may be NULL)
	virtual VError	WriteResource (const VString& inType, sLONG inID, const void* inData, sLONG inDataLen, const VString* inName = NULL) = 0;

	#if WITH_VMemoryMgr
	VError	WriteResourceHandle (const VString& inType, sLONG inID, const VHandle& inData, const VString* inName = NULL);
	VError	AddResourceHandle (const VString& inType, const VHandle& inData, const VString* inName, sLONG *outID);

	// copy ressources one by one. Slow!. Maybe used to copy from one specific VResourFile implementation to another.
	virtual VError	CopyResourcesInto (VResourceFile& inDestination) const;
	#endif
	
	// Creates a new resource with a new unique ID
	// inData is optionnal (may be NULL)
	virtual VError	AddResource (const VString& inType, const void* inData, sLONG inDataLen, const VString* inName, sLONG *outID) = 0;

	// Deletes a resource
	virtual VError	DeleteResource (const VString& inType, sLONG inID) = 0;
	virtual VError	DeleteResource (const VString& inType, const VString& inName) = 0;

	// Returns the resource ID given its name (the first one found).
	virtual VError	GetResourceID (const VString& inType, const VString& inName, sLONG *outID) const = 0;

	// Returns the resource Name given its ID (the first one found).
	virtual VError	GetResourceName (const VString& inType, sLONG inID, VString& outName) const = 0;

	// Set the resource Name given its ID (the first one found).
	virtual VError	SetResourceName (const VString& inType, sLONG inID, const VString& inName) = 0;

	// Utilities for 'STR ' or StringTable resources. Binary contents are implementation specific.
	virtual Boolean GetString (VString& outString, sLONG inID) const = 0;
	virtual Boolean GetString (VString& outString, sLONG inID, sLONG inIndex) const = 0;
	virtual Boolean GetStringList (sLONG inID, VArrayString& outStrings) const;
	virtual Boolean GetStringList (const VString& inName, VArrayString& outStrings) const;

	// flush data to disk or whatever
	virtual void	Flush ();

	// Is this resource file object usable ?
	virtual Boolean	IsValid () const = 0;

	// provide an iterator for all resources of given type
	virtual VResourceIterator*	NewIterator (const VString& inType) const = 0;
	
	// returns unique and not empty names of resources of given type (if you want them all, use an iterator)
	virtual VError	GetResourceListNames (const VString& inType, VArrayString& outNames) const = 0;

	// returns unique IDs of resources of given type (if you want them all, use an iterator)
	virtual VError	GetResourceListIDs (const VString& inType, VArrayLong& outIDs) const = 0;
	
	// returns used types
	virtual VError	GetResourceListTypes (VArrayString& outTypes) const = 0;
	
#if USE_OBSOLETE_API
	void	MakeString (VString& xstr,PString pstr);
#endif
	
	// Utilities
#if VERSIONWIN
	virtual HMODULE	WIN_GetModule () const { return NULL; };
#endif
	
	static Boolean	Init ();
	static void		DeInit ();

protected:
	// Inherited from IRefCountable
	virtual void	DoOnRefCountZero ();
};


#if VERSIONWIN
#pragma warning (push)
#pragma warning (disable: 4275) // XTOOLBOX_API marche pas pour les templates
#endif

class XTOOLBOX_API VResourceIterator : public VIteratorOf<VHandle>
{ 
public:
	// Inherited from VIteratorOf
	virtual VHandle	Next () = 0;
	virtual VHandle Current () const = 0;
	virtual VHandle Previous () = 0;
	virtual VHandle First () = 0;
	virtual VHandle Last () = 0;
	virtual void	SetPos (sLONG pPos) = 0;
	virtual sLONG	GetPos () const = 0;

	virtual void	SetCurrent (const VHandle& /*inData*/) { assert (false); };	// Meaningless
	
	virtual sLONG	GetCurrentResourceID () = 0;
	virtual void	GetCurrentResourceName (VString& outName) = 0;
};

#if VERSIONWIN
#pragma warning (pop)
#endif 


class XTOOLBOX_API VClassicResourceIterator : public VResourceIterator
{ 
public:
	VClassicResourceIterator (VResourceFile* inResFile, const VString& inType);
	
	// Inherited from VResourceIterator
	virtual VHandle	Next ();
	virtual VHandle	Current () const;
	virtual VHandle Previous ();
	virtual VHandle First ();
	virtual VHandle Last ();
	virtual void	SetPos (sLONG inPos);
	virtual sLONG	GetPos () const;
	
	virtual sLONG	GetCurrentResourceID ();
	virtual void	GetCurrentResourceName (VString& outName);

protected:
	VResourceFile*	fResFile;
	VArrayLong		fIDs;
	VResTypeString	fType;
	sLONG			fPos;
};

END_TOOLBOX_NAMESPACE

#endif

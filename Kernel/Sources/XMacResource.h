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
#ifndef __XMacResource__
#define __XMacResource__

#if !__LP64__

#include "Kernel/Sources/VResource.h"

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API VMacResFile : public VResourceFile
{
public:
			VMacResFile (sWORD inMacRefNum);
			VMacResFile (const VFilePath& inPath, FileAccess inFileAccess, Boolean inCreate);
	virtual ~VMacResFile ();

	virtual VError	Open (const VFilePath& inPath, FileAccess inFileAccess, Boolean inCreate);
	virtual VError	Close ();

	// reads a resource data, return NULL if not found. You're the owner of the handle.
	virtual VHandle	GetResource (const VString& inType, sLONG inID) const;
	virtual VHandle GetResource (const VString& inType, const VString& inName) const;
	virtual bool	GetResource ( const VString& inType, sLONG inID, VBlob& outData) const;

	// does this resource exists ? (by ID)
	virtual Boolean	ResourceExists (const VString& inType, sLONG inID) const;
	virtual Boolean ResourceExists (const VString& inType, const VString& inName) const;

	// returns the size of a resource data
	virtual VError	GetResourceSize (const VString& inType, sLONG inID, sLONG *outSize) const;
	virtual VError	GetResourceSize (const VString& inType, const VString& inName, sLONG *outSize) const;
	
	// Creates a new resource (overwrites existing one)
	// inData is optionnal (may be NULL)
	virtual VError	WriteResource (const VString& inType, sLONG inID, const void *inData, sLONG inDataLen, const VString *inName = NULL);
	
	// Creates a new resource with a new unique ID
	// inData is optionnal (may be NULL)
	virtual VError	AddResource (const VString& inType, const void *inData, sLONG inDataLen, const VString *inName, sLONG *outID);

	// Deletes a resource
	virtual VError	DeleteResource (const VString& inType, sLONG inID);
	virtual VError	DeleteResource (const VString& inType, const VString& inName);

	// Returns the resource ID given its name (the first one found).
	virtual VError	GetResourceID (const VString& inType, const VString& inName, sLONG *outID) const;

	// Returns the resource Name given its ID (the first one found).
	virtual VError	GetResourceName (const VString& inType, sLONG inID, VString& outName) const;

	// Set the resource Name given its ID (the first one found).
	virtual VError	SetResourceName (const VString& inType, sLONG inID, const VString& inName);

	// Utilities for 'STR ' resources. Binary contents are implementation specific.
	virtual Boolean	GetString (VString& outString, sLONG inID) const;
	virtual Boolean	GetString (VString& outString, sLONG inID, sLONG inIndex) const;

	// flush data to disk or whatever
	virtual void	Flush ();

	// Is this resource file object usable ?
	virtual Boolean	IsValid () const;

	VResourceIterator*	NewIterator (const VString& inType) const;
	
	virtual VError	GetResourceListNames (const VString& inType, VArrayString& outNames) const;
	virtual VError	GetResourceListIDs (const VString& inType, VArrayLong& outIDs) const;
	virtual VError	GetResourceListTypes (VArrayString& outTypes) const;
	
	sWORD	MAC_GetRefNum () const { return fRefNum; };
	void	SetUseResourceChain (Boolean isUseIt = true) { fUseResourceChain = isUseIt; };
	
	static void	DebugTest ();

protected:
	mutable Handle	fLastStringList;
	mutable sWORD	fLastStringListID;
	sWORD	fRefNum;
	Boolean	fUseResourceChain;
	Boolean fCloseFile;
	Boolean	fReadOnly;

	VError	_Open (const VFilePath& inPath, FileAccess inFileAccess, Boolean inCreate);
	VError	_GetResource (const VString& inType, sLONG inID, Handle *outHandle) const;
	VError	_GetResource (const VString& inType, const VString& inName, Handle *outHandle) const;
};



class StResFileSetter
{
public:
	StResFileSetter (sWORD inResFileID) { fSavedFile = ::CurResFile(); ::UseResFile(inResFileID); };
	~StResFileSetter () { ::UseResFile(fSavedFile); };
			
protected:
	sWORD	fSavedFile;
	StResFileSetter () { assert(false); };
};



class XTOOLBOX_API VMacResourceIterator : public VResourceIterator
{
public:
	VMacResourceIterator(sWORD inRefNum, ResType inType, Boolean inUseResourceChain);
	
	// Inherited from VResourceIterator
	virtual VHandle	Next();
	virtual VHandle	Current() const;
	virtual VHandle	Previous();
	virtual VHandle	First();
	virtual VHandle	Last ();
	virtual void	SetPos(sLONG inPos);
	virtual sLONG	GetPos() const;
	
	virtual sLONG	GetCurrentResourceID();
	virtual void	GetCurrentResourceName(VString& outName);

protected:
	sWORD		fRefNum;
	Boolean		fUseResourceChain;
	ResType		fType;
	sLONG		fPos;

	VArrayWord	fRefNums;
	VArrayWord	fIDs;

	VHandle	_GetResource(sLONG inIndex) const;
	void	_Load();
};

END_TOOLBOX_NAMESPACE

#endif // #if !__LP64__

#endif
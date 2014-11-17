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
#ifndef __XWinResource__
#define __XWinResource__

#include "Kernel/Sources/VResource.h"


BEGIN_TOOLBOX_NAMESPACE

// Needed declarations
class XWinFullPath;

typedef struct WinResBlock
{
	HRSRC	hrsrc;
	HGLOBAL	handle;
	VPtr	data;
	sLONG	size;
} WinResBlock;

typedef struct WinResIndex
{
	sLONG	curres;
	sLONG	index;
	char*	resname;
} WinResIndex;

class XTOOLBOX_API VWinResFile : public VResourceFile
{
public:
			VWinResFile (HMODULE inModule);
			VWinResFile (const VFilePath& inPath, FileAccess inFileAccess, Boolean inCreate);
	virtual ~VWinResFile ();
	
	virtual VError	Open (const VFilePath& inPath, FileAccess inFileAccess, Boolean inCreate);
	virtual VError	Close ();

	// reads a resource data, return NULL if not found. You're the owner of the handle.
	virtual VHandle	GetResource (const VString& inType, sLONG inID) const;
	virtual VHandle	GetResource (const VString& inType, const VString& inName) const;
	virtual bool	GetResource ( const VString& inType, sLONG inID, VBlob& outData) const;

	// does this resource exists ? (by ID)
	virtual Boolean	ResourceExists (const VString& inType, sLONG inID) const;
	virtual Boolean	ResourceExists (const VString& inType, const VString& inName) const;

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

	// Is this resource file object usable ?
	virtual Boolean	IsValid () const;
	
	virtual void	Flush ();

	// provide an iterator for all resources of given type
	virtual VResourceIterator*	NewIterator (const VString& inType) const;
	
	// returns unique and not empty names of resources of given type (if you want them all, use an iterator)
	virtual VError	GetResourceListNames (const VString& inType, VArrayString& outNames) const;

	// returns unique IDs of resources of given type (if you want them all, use an iterator)
	virtual VError	GetResourceListIDs (const VString& inType, VArrayLong& outIDs) const;
	
	// returns used types
	virtual VError	GetResourceListTypes (VArrayString& outTypes) const;

	// Utilities
	virtual HMODULE	WIN_GetModule () const { return fModule; };
	
protected:
	mutable HMODULE	fModule;
	mutable HANDLE	fUpdateHandle;
	WORD			fLanguage;
	Boolean 		fCloseFile;
	XWinFullPath*	fPath;
	WORD			fLastID;

	VError	_Open (const VFilePath& inPath, FileAccess inFileAccess, Boolean inCreate);
	VError	_GetResource (const VString& inType, sLONG inID, HRSRC *outRsrc) const;
	VError	_GetResource (const VString& inType, const VString& inName, HRSRC *outRsrc) const;
	
	VError	_BeginUpdate ();
	VError	_EndUpdate () const;
};

END_TOOLBOX_NAMESPACE

#endif

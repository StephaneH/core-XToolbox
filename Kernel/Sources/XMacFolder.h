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
#ifndef __XMacFolder__
#define __XMacFolder__

#include "Kernel/Sources/VObject.h"
#include "Kernel/Sources/VFilePath.h"

BEGIN_TOOLBOX_NAMESPACE

// Needed declarations
class VString;
class VFolderIterator;
class VFileSystem;

class XMacFolder : public VObject
{
public:
									XMacFolder():fOwner( NULL), fPosixPath( NULL)	{;}
	virtual							~XMacFolder();

				void 				Init( VFolder *inOwner)	{ fOwner = inOwner;}

				VError 				Create() const;
				VError 				Delete() const;
                VError              MoveToTrash() const;
				bool				Exists( bool inAcceptFolderAlias) const;

				VError				RevealInFinder() const;
				VError 				Rename( const VString& inName, VFolder** outFile ) const;
				VError				Move( const VFolder& inNewParent, const VString *inNewName, VFolder** outFolder ) const;
				
				VError 				GetTimeAttributes( VTime* outLastModification, VTime* outCreationTime,  VTime* outLastAccess ) const;
				VError 				SetTimeAttributes(const VTime* outLastModification,const VTime* outCreationTime,const VTime* outLastAccess ) const;
				VError				GetVolumeCapacity (sLONG8 *outTotalBytes ) const;
				VError 				GetVolumeFreeSpace (sLONG8 *outFreeBytes, bool inWithQuotas = true ) const;

				VError				GetPosixPath( VString& outPath) const;
				
				bool				MAC_GetFSRef( FSRef *outRef ) const {return _BuildFSRefFile(outRef);};

				VError				MakeWritableByAll();
				bool				IsWritableByAll();
				VError				SetPermissions( uWORD inMode );
				VError				GetPermissions( uWORD *outMode );
				
				bool				GetIconFile( IconRef &outIconRef ) const;
				
				VError				IsHidden(bool& outIsHidden);

		static	VError				RetainSystemFolder( ESystemFolderKind inFolderKind, bool inCreateFolder, VFolder **outFolder );

private:
				const UInt8*		_GetPosixPath() const;
				bool				_BuildFSRefFile(FSRef *outFileRef) const ;
				bool				_BuildParentFSRefFile(FSRef *outParentRef, VString &outFileName) const ;

				VFolder*			fOwner;
				UInt8*				fPosixPath;	// cached UTF8 posix path
};

class XTOOLBOX_API XMacFolderIterator : public VObject
{
public:
									XMacFolderIterator( const VFolder *inFolder, FileIteratorOptions inOptions);
	virtual 						~XMacFolderIterator();

				VFolder*			Next();

protected:
				VFolder*			_GetCatalogInfo ();

				FSRef				fFolderRef;
				sLONG				fIndex;
				FileIteratorOptions	fOptions;
				FSIterator			fIterator;
				VFileSystem*		fFileSystem;
};

typedef 		XMacFolder 			XFolderImpl;
typedef 		XMacFolderIterator 	XFolderIteratorImpl;

END_TOOLBOX_NAMESPACE

#endif

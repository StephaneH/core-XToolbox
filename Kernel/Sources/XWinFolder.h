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
#ifndef __XWinFolder__
#define __XWinFolder__

#include "VObject.h"
#include "VFilePath.h"

BEGIN_TOOLBOX_NAMESPACE

// Needed declarations
class VString;
class VFolderIterator;
class VFileSystem;

class XWinFolder : public VObject
{
public:
			void				Init( VFolder *inOwner) {fOwner = inOwner;}

			VError				Create() const;
			VError				Delete() const;
			VError				MoveToTrash() const;
			bool				Exists( bool inAcceptFolderAlias) const;

			VError				RevealInFinder() const;
			VError				Move( const VFolder& inNewParent, const VString *inNewName, VFolder** outFolder ) const;
			VError				Rename( const VString& inName, VFolder** outFile ) const;
			
			VError				GetTimeAttributes( VTime* outLastModification, VTime* outCreationTime, VTime* outLastAccess ) const;
			VError				SetTimeAttributes( const VTime *inLastModification, const VTime *inCreationTime, const VTime *inLastAccess ) const;

			VError				GetVolumeCapacity (sLONG8 *outTotalBytes ) const;
			VError				GetVolumeFreeSpace (sLONG8 *outFreeBytes, bool inWithQuotas = true ) const;
			

			VError				GetAttributes( DWORD *outAttrb ) const;
			VError				SetAttributes( DWORD inAttrb ) const;

			VError				GetPosixPath( VString& outPath) const;
			
			bool				GetIconFile( uLONG inIconSize, HICON &outIconFile ) const;

			VError				IsHidden(bool& outIsHidden);

			VError				MakeWritableByAll();
			bool				IsWritableByAll();

	static	VError				RetainSystemFolder( ESystemFolderKind inFolderKind, bool inCreateFolder, VFolder **outFolder );

private:
			VFolder*			fOwner;
};


class XTOOLBOX_API XWinFolderIterator : public VObject
{
public:
								XWinFolderIterator( const VFolder *inFolder, FileIteratorOptions inOptions);
	virtual						~XWinFolderIterator();

			VFolder*			Next();

protected:
			VFolder*			_GetPath( const VString& inFileName, DWORD inAttributes);
			FileIteratorOptions	fOptions;
			VFilePath			fRootPath;
			HANDLE				fHandle;
			sLONG				fIndex;
			VFileSystem*		fFileSystem;
};

typedef XWinFolder XFolderImpl;
typedef XWinFolderIterator XFolderIteratorImpl;

END_TOOLBOX_NAMESPACE

#endif

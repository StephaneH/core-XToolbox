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
#ifndef __XWinFile__
#define __XWinFile__

#include "VObject.h"
#include "VFilePath.h"
#include "VSyncObject.h"

BEGIN_TOOLBOX_NAMESPACE

// Needed declarations
class VFile;
class VURL;
class VString;
class VFolder;
class VFileDesc;
class VFileIterator;
class VTime;
class VFileKind;
class VFileSystem;

typedef HANDLE	FileDescSystemRef;


class XWinFullPath : public VObject
{
public:
	XWinFullPath( const VFilePath &inTarget, char *inAppendCString = NULL);
	virtual ~XWinFullPath();
	
	operator PWCHAR()  {return (PWCHAR) fUniPath.GetCPointer();}

private:
	VStr255					fUniPath;
};


class XWinFileDesc : public VObject
{
public:
								XWinFileDesc( FileDescSystemRef inRef):fHandle( inRef)		{}
								virtual ~XWinFileDesc();
								
			VError				GetSize( sLONG8 *outSize) const;

			// sets EOF, does not change the current pos
			VError				SetSize( sLONG8 inSize) const;

			// read some bytes. GetData (buf, count) reads at current pos
			VError				GetData( void *outData, VSize &ioCount, sLONG8 inOffset, bool inFromStart) const;

			// write some bytes. PutData (buf, count) writes at current pos
			VError				PutData( const void *inData, VSize& ioCount, sLONG8 inOffset, bool inFromStart) const;

			VError				GetPos( sLONG8 *outPos) const;

			// absolute offset by default
			VError				SetPos( sLONG8 inOffset, bool inFromStart) const;

			VError				Flush() const;

			FileDescSystemRef	GetSystemRef() const		{ return fHandle; }

protected:
			FileDescSystemRef	fHandle;
	mutable	VCriticalSection	fMutex;
};


class XTOOLBOX_API XWinFile : public VObject
{
public:
								XWinFile();
	virtual						~XWinFile ();

			void				Init( VFile *inOwner)		{fOwner = inOwner;}

			// return a pointer to the created file. A null pointer value indicates an error.
			VError				Open ( FileAccess inFileAccess, FileOpenOptions inOptions, VFileDesc** outFileDesc) const;
			VError				Create( FileCreateOptions inOptions ) const;
			
			// Rename the file with the given name ( foofoo.html ).
			VError				Rename( const VString& inName, VFile** outFile ) const;
			VError				Copy( const VFilePath& inDestination, VFileSystem *inDestinationFileSystem, VFile** outFile, FileCopyOptions inOptions ) const;
			VError				Move( const VFilePath& inNewPath, VFileSystem *inDestinationFileSystem, VFile** outFile, FileCopyOptions inOptions ) const;
			VError				Delete() const;
			VError				MoveToTrash() const;

			// test if the file allready exist.
			bool				Exists () const;

			// GetAttributes
			VError				GetAttributes( DWORD *outAttrb ) const;
			VError				SetAttributes( DWORD inAttrb ) const;

			VError				GetSize( sLONG8 *outSize ) const;
			VError				GetResourceForkSize ( sLONG8 *outSize ) const;
			bool				HasResourceFork() const;

			// GetFileAttributes && FSGetCatalogInfo.
			VError				GetTimeAttributes( VTime* outLastModification = NULL, VTime* outCreationTime = NULL, VTime* outLastAccess = NULL ) const;
			VError				GetFileAttributes( EFileAttributes &outFileAttributes ) const;

			VError				SetTimeAttributes( const VTime *inLastModification = NULL, const VTime *inCreationTime = NULL, const VTime *inLastAccess = NULL ) const;
			VError				SetFileAttributes( EFileAttributes inFileAttributes ) const;

			// return DiskFreeSpace.
			VError				GetVolumeFreeSpace (sLONG8 *outFreeBytes, sLONG8 *outTotalBytes, bool inWithQuotas ) const;

			// reveal in finder
			VError				RevealInFinder() const;
			
			VError				ResolveAlias( VFilePath &outTargetPath) const;
			VError				CreateAlias( const VFilePath &inTargetPath, const VFilePath *inRelativeAliasSource) const;
			bool				IsAliasFile() const;

			VError				GetPosixPath( VString& outPath) const;

			VError				ConformsTo( const VFileKind& inKind, bool *outConformant) const;
			// Returns true if the first file kind is conform to the seceond file kind
	static	bool				IsFileKindConformsTo( const VFileKind& inFirstFileKind, const VFileKind& inSecondFileKind);

			bool				GetIconFile( uLONG inIconSize, HICON &outIconFile ) const;

			VError				Exchange(const VFile& inExhangeWith) const;

	static	VFileKind*			CreatePublicFileKind( const VString& inID);
	static	VFileKind*			CreatePublicFileKindFromOsKind( const VString& inOsKind );
	static	VFileKind*			CreatePublicFileKindFromExtension( const VString& inExtension );
	static	VString				GetFileTypeDescriptionFromExtension(const VString& inPathOrExtension);
	static	VError				RevealPathInFinder( const VString inPath);

private:
			VFile*				fOwner;
};


class XTOOLBOX_API XWinFileIterator : public VObject
{
public:
								XWinFileIterator( const VFolder *inFolder, FileIteratorOptions inOptions);
	virtual						~XWinFileIterator();

			VFile*				Next();

protected:
			VFile*				_GetPath( const VString& inFileName, DWORD inAttributes);

			FileIteratorOptions	fOptions;
			VFilePath			fRootPath;
			HANDLE				fHandle;
			sLONG				fIndex;
			VFileSystem*		fFileSystem;
};

typedef XWinFile XFileImpl;
typedef XWinFileDesc XFileDescImpl;
typedef XWinFileIterator XFileIteratorImpl;

END_TOOLBOX_NAMESPACE

#endif

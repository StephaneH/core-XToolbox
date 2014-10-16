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
#ifndef __XMacFile__
#define __XMacFile__

#include "Kernel/Sources/VObject.h"
#include "Kernel/Sources/VFilePath.h"

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

typedef FSIORefNum	FileDescSystemRef;

class XMacFileDesc : public VObject
{
public:

									XMacFileDesc( FileDescSystemRef inRef):fForkRef( inRef)							{}
									~XMacFileDesc()																	{ ::FSCloseFork( fForkRef);}

				VError 				GetSize( sLONG8 *outPos) const;

									// sets EOF, does not change the current pos
				VError				SetSize( sLONG8 inSize) const;

									// read some bytes. GetData (buf, count) reads at current pos
				VError 				GetData( void *outData, VSize &ioCount, sLONG8 inOffset, bool inFromStart ) const;

									// write some bytes. PutData (buf, count) writes at current pos
				VError 				PutData( const void *inData, VSize& ioCount, sLONG8 inOffset, bool inFromStart) const;

				VError 				GetPos( sLONG8 *outSize) const;

									// absolute offset by default
				VError 				SetPos( sLONG8 inOffset, bool inFromStart) const;

				VError 				Flush() const;
				
				FileDescSystemRef	GetSystemRef() const		{ return fForkRef; }

protected:
				FileDescSystemRef	fForkRef;
};


class XTOOLBOX_API XMacFile : public VObject
{
public:

								XMacFile():fOwner( NULL), fPosixPath( NULL)	{;}
	virtual						~XMacFile ();

			void 				Init( VFile *inOwner)	{ fOwner = inOwner;}

									// return a pointer to the created file. A null pointer value indicates an error.
			VError 				Open ( const FileAccess inFileAccess, FileOpenOptions inOptions, VFileDesc** outFileDesc) const;
			VError				Create( FileCreateOptions inOptions) const;

			VError				Copy( const VFilePath& inDestination, VFileSystem *inDestinationFileSystem, VFile** outFile, FileCopyOptions inOptions ) const;
			VError 				Move( const VFilePath& inDestinationPath, VFileSystem *inDestinationFileSystem, VFile** outFile, FileCopyOptions inOptions ) const;	
			VError 				Rename( const VString& inName, VFile** outFile ) const;
			VError 				Delete() const;
			VError 				MoveToTrash() const;

									// test if the file allready exist.
			bool				Exists () const;

									// GetFileAttributes && FSGetCatalogInfo.
			VError				GetTimeAttributes( VTime* outLastModification = NULL, VTime* outCreationTime = NULL, VTime* outLastAccess = NULL ) const;
			VError				GetFileAttributes( EFileAttributes &outFileAttributes ) const;

			VError				SetTimeAttributes( const VTime *inLastModification = NULL, const VTime *inCreationTime = NULL, const VTime *inLastAccess = NULL ) const;
			VError				SetFileAttributes( EFileAttributes inFileAttributes ) const;

								// return DiskFreeSpace.
			VError 				GetVolumeFreeSpace (sLONG8 *outFreeBytes, sLONG8 *outTotalBytes, bool inWithQuotas ) const;
			
								// returns object volume reference
			OSErr				GetFSVolumeRefNum( FSVolumeRefNum *outRefNum) const;
			
								// reveal in finder
			VError				RevealInFinder() const;
	
			VError 				ResolveAlias( VFilePath &outTargetPath) const;
			VError 				CreateAlias( const VFilePath &inTargetPath, const VFilePath *inRelativeAliasSource) const;
			bool				IsAliasFile() const;

			VError				GetPosixPath( VString& outPath) const;
			
	static	OSErr	 			HFSPathToFSRef(const VString& inPath, FSRef *outRef);
	static	OSErr				FSRefToHFSPath( const FSRef &inRef, VString& outPath);
	

	#if !__LP64__
	static	OSErr				FSSpecToHFSPath(const FSSpec& inSpec, VString& outPath);
	static	OSErr				HFSPathToFSSpec (const VString& inPath, FSSpec *outSpec);
	static	OSErr				FSSpecToFSRef (const FSSpec& inSpec, FSRef *outRef);
	static	OSErr				FSRefToFSSpec (const FSRef& inRef, FSSpec *outSpec);
	#endif

	/**
		@brief HFS <-> posix conversion is not trivial because of volumes handling:
		
		if "Macintosh HD" is the boot volume:
			Macintosh HD:folder:file	<->	/folder/file
		else
			Macintosh HD:folder:file	<->	/volumes/folder/file
		
		That's why we need to call the OS.
	**/
	static	bool				PosixPathFromHFSPath( const VString& inHFSPath, VString& outPosixPath, bool inIsDirectory);
	static	UInt8*				PosixPathFromHFSPath( const VString& inHFSPath, bool inIsDirectory);
	
	static	OSErr				RevealFSRefInFinder( const FSRef& inRef);

			VError				GetSize( sLONG8 *outSize ) const;
			VError				GetResourceForkSize ( sLONG8 *outSize ) const;
			bool				HasResourceFork() const;

			VError				GetKind ( OsType *outKind ) const;
			VError				SetKind ( OsType inKind ) const;
			VError				GetCreator ( OsType *outCreator ) const;
			VError				SetCreator ( OsType inCreator ) const;

			void				SetExtensionVisible( bool inShow );
			bool				GetIconFile( IconRef &outIconRef ) const;
		//	void				SetCustomIcon(VMacResFile& inResFile, sWORD inId);
		//	void				GetCustomIcon(VMacResFile& inResFile, sWORD inId);
			void				SetFinderComment( VString& inString );
			void				GetFinderComment( VString& outString );
			
			VError				RetainUTIString( CFStringRef *outUTI) const;
			VError				GetUTIString( VString& outUTI ) const;

			VError				MakeWritableByAll();
			bool				IsWritableByAll();
			VError				SetPermissions( uWORD inMode );
			VError				GetPermissions( uWORD *outMode );

			VError				ConformsTo( const VFileKind& inKind, bool *outConformant) const;
	
	static	bool				IsFileKindConformsTo( const VFileKind& inFirstKind, const VFileKind& inSecondKind);

			VError				Exchange(const VFile& inExhangeWith) const;

	static	VFileKind*			CreatePublicFileKind( const VString& inID);
	static	VFileKind*			CreatePublicFileKindFromOsKind( const VString& inOsKind );
	static	VFileKind*			CreatePublicFileKindFromExtension( const VString& inExtension );

private:
			OSErr				_Open( const FSRef& inFileRef, const HFSUniStr255& inForkName, const FileAccess inFileAccess, FSIORefNum *outForkRef) const;
			bool				_BuildFSRefFile( FSRef *outFileRef) const ;
			bool				_BuildParentFSRefFile( FSRef *outParentRef, VString &outFileName) const ;
			CFURLRef			_CreateURLRefFromPath()const;
				
			VFile*				fOwner;
			UInt8*				fPosixPath;	// cached UTF8 posix path
};

class XTOOLBOX_API XMacFileIterator : public VObject
{
public:
								XMacFileIterator( const VFolder *inFolder, FileIteratorOptions inOptions);
	virtual						~XMacFileIterator();

			VFile* 				Next();

protected:

			VFile*				_GetCatalogInfo ();

			FSRef				fFolderRef;
			sLONG				fIndex;
			FileIteratorOptions	fOptions;
			FSIterator			fIterator;
			VFileSystem*		fFileSystem;
};

typedef	XMacFile 			XFileImpl;
typedef	XMacFileDesc	 	XFileDescImpl;
typedef XMacFileIterator 	XFileIteratorImpl;

END_TOOLBOX_NAMESPACE

#endif

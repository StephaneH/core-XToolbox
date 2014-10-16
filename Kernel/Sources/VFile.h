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
#ifndef __VFile__
#define __VFile__

#if VERSIONMAC
#include "Kernel/Sources/XMacFile.h"
#elif VERSION_LINUX
#include "Kernel/Sources/XLinuxFile.h"
#elif VERSIONWIN
#include "Kernel/Sources/XWinFile.h"
#endif

#include "Kernel/Sources/VFolder.h"
#include "Kernel/Sources/VFileSystemObject.h"
#include "Kernel/Sources/VSyncObject.h"
#include "Kernel/Sources/IRefCountable.h"
#include "Kernel/Sources/VMemoryBuffer.h"


BEGIN_TOOLBOX_NAMESPACE

class VFileKind;
class VVolumeInfo;
class VFileSystem;

// you can not create a VFileDesc by yourself, you have to get one by calling the method VFile::Open
// or "create" with a VFile
class XTOOLBOX_API VFileDesc : public VObject
{
// Pas reussi a le faire compiler autrement ...
#if VERSIONWIN
	friend class XWinFile;
#elif VERSIONMAC
	friend class XMacFile;
#elif VERSION_LINUX
    friend class XLinuxFile;
#endif

public:
	virtual						~VFileDesc();

			// get EOF
			sLONG8				GetSize() const;

			// sets EOF, does not change the current pos
			VError				SetSize( sLONG8 inSize) const;

			// read inCount bytes at inOffset from beginning.
			// if outActualCount is not NULL, it receives the actual count of read bytes (returns an error if not equal to inCount).
			// current pos is moved accordingly.
			VError				GetData( void *outData, VSize inCount, sLONG8 inOffset, VSize *outActualCount = NULL) const;

			// read inCount bytes at inOffset from current position.
			// if outActualCount is not NULL, it receives the actual count of read bytes (returns eof if not equal to inCount).
			// current pos is moved accordingly.
			VError				GetDataAtPos( void *outData, VSize inCount, sLONG8 inOffset = 0, VSize *outActualCount = NULL) const;

			// write inCount bytes at inOffset from beginning.
			// current pos is moved accordingly.
			VError				PutData( const void *inData, VSize inCount, sLONG8 inOffset, VSize *outActualCount = NULL) const;

			// write inCount bytes at inOffset from current position.
			// current pos is moved accordingly.
			VError				PutDataAtPos( const void *inData, VSize inCount, sLONG8 inOffset = 0, VSize *outActualCount = NULL) const;

			sLONG8				GetPos() const;

			// absolute offset by default
			VError				SetPos( sLONG8 inOffset, bool inFromStart = true) const;

			VError				Flush() const; 

			FileAccess			GetMode() const											{ return fMode; }
			
			FileDescSystemRef	GetSystemRef() const									{ return fImpl.GetSystemRef(); }
			
			const VFile*		GetParentVFile() const								{ return fFile; }

protected:
								// the only good way to create a VFileDesc.
								VFileDesc( const VFile *inFile, FileDescSystemRef inSystemRef, FileAccess inMode );

private:
								VFileDesc( const VFileDesc& inOther);	// no copy
								VFileDesc&	operator=( const VFileDesc& inOther);	// no copy

			FileAccess			fMode;
			const VFile*		fFile;	// essentially for info on error
			XFileDescImpl		fImpl;
};

class VFileSystemNamespace;

class XTOOLBOX_API VFile : public VObject, public IRefCountable
{
public:

	explicit					VFile( const VFile& inSource );
	explicit					VFile( const VURL& inUrl );
	explicit					VFile( const VFolder& inFolder, const VString& inRelativePath, FilePathStyle inRelativePathStyle = FPS_DEFAULT);
	explicit					VFile( const VFilePath& inRootPath, const VString& inRelativePath, FilePathStyle inRelativePathStyle = FPS_DEFAULT);
	
	// It's is your responsibility to ensure that you pass a syntaxically correct file path.
	// else you'll get an assert.
	explicit					VFile( const VString& inFullPath, FilePathStyle inFullPathStyle = FPS_DEFAULT);

	// It's is your responsibility to ensure that you pass a syntaxically correct file path.
	// else you'll get an assert.
	// VFileSystem is optionnal and retained. It's there for conveniency for sandboxing support.
	explicit					VFile( const VFilePath& inPath, VFileSystem *inFileSystem = NULL);

	explicit					VFile( VFileSystemNamespace* fsNameSpace, const VString& inPath);

#if VERSIONMAC
	explicit					VFile( const FSRef& inRef);
#endif

	virtual						~VFile();

			/**@brief	Create a file in the system temporary folder.
						Returns the corresponding VFile*, which must be released by the caller.

						Once you're done with the file, you can explicitely Delete() it, else it will remain
						in the temporary system folder and will cleaned up by the system sooner or later after
						the application has quit.

						NOTE
							The routine should never return NULL. But, theorically-hysterical ;->, it is able to
							return NULL: say diskFullErr, or any strange access right bug, or...
							So, caller must be prepared to handle NULL file after the call, or let the application
							to crash, considering that if it can't create an empty temporary file, then there is a
							serious problem somewhere.
			*/
	static	VFile *				CreateTemporaryFile();

	// Creates a VFile and immediately tries to resolve it using ResolveAliasFile.
	// If the file exists as an alias file one couldn't resolve as a file, it tries to delete it.
	// It only throws an error if the alias file is invalid and one couldn't delete it.
	static	VFile*				CreateAndResolveAlias( const VFolder& inFolder, const VString& inRelativPath, FilePathStyle inRelativPathStyle = FPS_DEFAULT);

			// Opens the file.
			// Return a pointer to the created file descriptor. A null pointer value indicates an error.
			// disposing the returned VFileDesc* closes the file.
			VError				Open( FileAccess inFileAccess, VFileDesc** outFileDesc, FileOpenOptions inOptions = FO_Default) const;

			// Creates the file without opening it.
			VError				Create( FileCreateOptions inOptions = FCR_Default) const;

			// Copy this file to destination.
			// Destination folder must exist.
			// outFile (may be null) returns new file.
			VError				CopyTo( const VFilePath& inDestination, VFile** outFile, FileCopyOptions inOptions = FCP_Default ) const;
			VError				CopyTo( const VFile& inDestinationFile, FileCopyOptions inOptions = FCP_Default ) const;
			VError				CopyTo( const VFolder& inDestinationFolder, VFile** outFile, FileCopyOptions inOptions = FCP_Default ) const;
			VError				CopyFrom( const VFile& inSource, FileCopyOptions inOptions = FCP_Default ) const;

			// Rename the file with the given name ( foo.html ).
			VError				Rename( const VString& inName, VFile** outFile = NULL ) const;
			
			// Move the file elsewhere. May rename the file, may move the file on another volume.
			VError				Move( const VFilePath& inDestination, VFile** outFile, FileCopyOptions inOptions = FCP_Default ) const;
			VError				Move( const VFile& inDestinationFile, FileCopyOptions inOptions = FCP_Default ) const;
			VError				Move( const VFolder& inDestinationFolder, VFile** outFile, FileCopyOptions inOptions = FCP_Default ) const;
			
			// Delets the file. Don't throw an error If the file does not exist (but an err code if returned)
			VError				Delete() const;
			VError				MoveToTrash() const;

			// Retain parent folder.
			// If root has been reached returns NULL.
			// In sandboxed mode, the root is the file system root (if any).
			VFolder*			RetainParentFolder( bool inSandBoxed = false) const;

			// return full file name ( ex: foo.html ).
			void				GetName( VString& outName) const;

			// return only the extension without dot ( ex: html ).
			void				GetExtension( VString& outExt) const;

			// @brief tells if the file ends with specified extension (no dot).
			// @brief you should prefer ConformsTo()
			bool				MatchExtension( const VString& inExtension) const;
			
			// return file name ( foo ).
			void				GetNameWithoutExtension( VString& outName) const;

			// return the full path.
			const VFilePath&	GetPath() const;	// not recommended cause one day the path on mac will be dynamic
			VFilePath&			GetPath( VFilePath& outPath) const;
			void				GetPath( VString &outPath, FilePathStyle inPathStyle = FPS_DEFAULT, bool relativeToFileSystem = false) const;

			// Tell if the file path is syntaxically correct
			bool				HasValidPath() const	{ return fPath.IsFile();}

			// Tell if the file exists.
			bool				Exists() const;

			// GetFileAttributes && FSGetCatalogInfo.
			VError				GetTimeAttributes( VTime* outLastModification = NULL, VTime* outCreationTime = NULL, VTime* outLastAccess = NULL ) const;
			

			VError				SetTimeAttributes( const VTime *inLastModification = NULL, const VTime *inCreationTime = NULL, const VTime *inLastAccess = NULL ) const;
			
			//Will set if file is read-only (Finder's user immutable property or Windows read-only property). This is independant of actual file access permission
			//Reports if a file is hidden or read-only. The read-only semantics maps to the Finder's immutable/locked bit or Windows explorer read-only file property. It is
			//Do not assume that because a file is not read-only then it is writable since file-system permission may specify otherwise.
			VError				GetFileAttributes( EFileAttributes &outFileAttributes ) const;		
			VError				SetFileAttributes( EFileAttributes inFileAttributes ) const;

			// return logical size of main fork
			VError				GetSize( sLONG8 *outSize) const;

			/** @brief returns true if the file exists and has a non empty resource fork. No error is thrown. **/
			bool				HasResourceFork() const;
			VError				GetResourceForkSize ( sLONG8 *outSize ) const;

			// return DiskFreeSpace.
			// Because GetVolumeFreeSpace() is sometime slow, the values are cached and you can pass in inAccuracyMilliSeconds the update period of the cached values.
			// pass zero as inAccuracyMilliSeconds to bypass the cache.
			VError				GetVolumeFreeSpace( sLONG8 *outFreeBytes, sLONG8 * outTotalBytes, bool inWithQuotas = true, uLONG inAccuracyMilliSeconds = 0 ) const;

#if !VERSION_LINUX
			// reveal in finder
			bool				RevealInFinder() const;
#endif
			// Resolve an alias file and returns target path.
			// This VFile should points to an alias file.
			VError				ResolveAlias( VFilePath& outTargetPath) const;

			// Utility function to resolve an alias file.
			// If the input file exists and is an alias to a file (and not a folder),
			// It returns the resolved alias file and the input file is released.
			// If inDeleteInvalidAlias is true and the file appears to be an invalid alias, it is being deleted, else an error is thrown.
	static	VError				ResolveAliasFile( VFile **ioFile, bool inDeleteInvalidAlias);
			
			// Creates an alias file at this VFile location pointing to specified target VFilePath
			VError				CreateAlias( const VFilePath& inTargetPath, const VFilePath *inRelativeAliasSource) const;
			
			// Tells if this VFile is an alias file.
			bool				IsAliasFile() const;
			
			// check if two vfile point to the same file.
			// Comparing the file paths VString is not same because some file systems are diacritical, some are not.
			bool				IsSameFile( const VFile *inFile) const;

			/** @brief tells if this file conforms to specified file kind. */
			bool				ConformsTo( const VString& inKindID) const;

			bool				ConformsTo( const VFileKind& inKind) const;

			// exchange 2 vfile, after the operation this wil point to inExhangeWith and inExhangeWith to this
			// This fonction is usefull for safe replace operation
			VError				Exchange(const VFile& inExchangeWith) const;
	
			// Put full content of file in specified buffer.
			// May throw error and returns false if failed.
			VError				GetContent( VMemoryBuffer<>& outContent) const;

			// Open file in FA_READ_WRITE mode and set its contents to provided data only.
			// On success, the file is exactly of size inDataSize.
			VError				SetContent( const void *inDataPtr, size_t inDataSize) const;
			
			// Read full content and convert it as string using optional leading BOM.
			// If no BOM is found, it uses the specified char set.
			// The carriage return mode is adjusted optionally.
			VError				GetContentAsString( VString& outContent, CharSet inDefaultSet, ECarriageReturnMode inCRMode = eCRM_NONE) const;

			// Open file in FA_READ_WRITE mode and set its contents to provided string in specified charset.
			// A BOM is inserted if relevant.
			// The carriage return mode is adjusted optionally.
			VError				SetContentAsString( const VString& inContent, CharSet inCharSet, ECarriageReturnMode inCRMode = eCRM_NONE) const;

			VError				GetURL(VString& outURL, bool inEncoded);
			VError				GetRelativeURL(VFolder* inBaseFolder, VString& outURL, bool inEncoded);

			VFileSystem*		GetFileSystem() const	{ return fFileSystem;}

#if VERSIONWIN
			// file attributes
			VError				WIN_GetAttributes( DWORD *outAttrb ) const;
			VError				WIN_SetAttributes( DWORD inAttrb ) const;
			const XFileImpl*	WIN_GetImpl() const;
#endif

#if VERSIONMAC
			VError				MAC_GetKind( OsType *outKind ) const;
			VError				MAC_SetKind( OsType inKind ) const;

			VError				MAC_GetCreator( OsType *outCreator ) const;
			VError				MAC_SetCreator( OsType inCreator ) const;

			bool				MAC_GetFSRef( FSRef *outRef) const;
			
			#if !__LP64__
			bool				MAC_GetFSSpec( FSSpec *outSpec) const;
			#endif

			void				MAC_SetExtensionVisible( bool inShow );
		//	void				MAC_SetCustomIcon(VMacResFile& inResFile, sWORD inId);
		//	void				MAC_GetCustomIcon(VMacResFile& inResFile, sWORD inId);
			void				MAC_SetFinderComment( VString& inString );
			void				MAC_GetFinderComment( VString& outString );
			
			CFStringRef			MAC_RetainUTIString() const							{ CFStringRef cfUTI; VError err = fImpl.RetainUTIString( &cfUTI); return (err == VE_OK) ? cfUTI : NULL;}
			void				MAC_GetUTIString( VString& outString ) const		{ fImpl.GetUTIString( outString);}

			VError				MAC_MakeWritableByAll();
			bool				MAC_IsWritableByAll();
			VError				MAC_SetPermissions( uWORD inMode );
			VError				MAC_GetPermissions( uWORD *outMode );

			const XFileImpl*	MAC_GetImpl() const;
#endif
			const XFileImpl*	GetImpl() const		{ return &fImpl;}

	static	bool				Init();
	static	void				DeInit();


private:
			VFile& operator=( const VFile& inSource );	// no copy because const object
			VError				_Move( const VFilePath& inDestination, VFileSystem *inDestinationFileSystem, VFile** outFile, FileCopyOptions inOptions) const;

			XFileImpl			fImpl;
			VFilePath			fPath;
			VFileSystem*		fFileSystem;
	mutable	VVolumeInfo*		fCachedVolumeInfo;
};


class VFileIterator	: public VFSObjectIterator<VFile, XFileIteratorImpl>
{
public:
	VFileIterator( const VFolder *inFolder, FileIteratorOptions inOptions = FI_NORMAL_FILES):VFSObjectIterator<VFile, XFileIteratorImpl>( inFolder, inOptions )	{;}
};


typedef std::map<VString, XBOX::VRefPtr<XBOX::VFileKind> >	MapOfVFileKind;
typedef std::vector<VString>								VectorOfFileExtension;
typedef	std::vector<VString>								VectorOfFileKind;


class XTOOLBOX_API VFileKindManager : public VObject, public IRefCountable
{
public:
										VFileKindManager();

			VFileKind*					RetainFileKind( const VString& inID);

			void						RegisterPrivateFileKind( const VString& inID, const VString& inDescription, const VectorOfFileExtension& inExtensions, const VectorOfFileKind &inParentIDs);
			void						RegisterPrivateFileKind( const VString& inID, const VectorOfFileKind& inOsKind, const VString& inDescription, const VectorOfFileExtension& inExtensions, const VectorOfFileKind &inParentIDs);

			enum { STRING_ALL, STRING_ALL_READABLE_DOCUMENTS };
			void						RegisterLocalizedString(long inWhich,const VString& inString);
			void						GetLocalizedString(long inWhich,VString &outString);

										/* This function just parse the map of file kind to try to search the first kind witch correspond to tge given extension */
			VFileKind*					RetainFirstFileKindMatchingWithExtension( const VString& inExtension );
			VFileKind*					RetainFirstFileKindMatchingWithOsKind( const VString& inOsKind );

	static	VFileKindManager*			Get()												{ return sManager;}

	static	bool						Init();
	static	void						DeInit();

private:
	static	VFileKindManager*			sManager;

			MapOfVFileKind				fMap;
			VString fStringAll;
			VString fStringAllReadableDocuments;
};


class XTOOLBOX_API VFileKind : public VObject, public IRefCountable
{ 
public:
			// @brief the ID is the UTI on Mac. It's a reversed-dns style url (ex: com.4d.database)
			const VString&				GetID() const										{ return fID; }

			// @brief this is the localized description of this kind for the user
			const VString&				GetDescription() const								{ return fDescription; }

			// @brief a kind is considered 'public' if it is accessible by other applications.
			// currently on windows, all kinds are private because there's no easy way to access the global registry.
			bool						IsPublic() const									{ return fIsPublic;}

			// @brief returns true if a file with specified extension can be considered of this file kind.
			// but the recommended way to check file kind conformance is to call VFile::ConformsTo.
			bool						MatchExtension( const VString& inExtension) const;

			bool						MatchOsKind( const VString& inOsKind) const;

			// @brief provides the recommanded file extension when creating a new file.
			// returns false if no extension was available.
			bool						GetPreferredExtension( VString& outExtension) const;

			bool						GetPreferredOsKind( VString& outOsKind) const;

			// @brief	returns the parent kind IDs for which the file kind is conform
			bool						GetParentIDs( VectorOfFileKind& outIDs) const;

			bool						ConformsTo( const VFileKind& inKind) const;
			bool						ConformsTo( const VString& inID) const;

			// @brief lookup in current application file kind map.
			// returns NULL for unregistered file kinds.
	static	VFileKind*					RetainFileKind( const VString& inID)				{ return VFileKindManager::Get()->RetainFileKind( inID);}

#if VERSIONWIN
			// @brief builds the appropriate filter string to be used in Win32 explorer-style standard dialogs
			void						WIN_BuildFilterStringForGetDialog( VString& outFilterString) const;
			bool						WIN_BuildExtensionsStringForGetDialog( VString& outExtensionsString) const;
#endif

			// @brief creates a new VFileKind.
			// you should not have to use this method, especially for public kinds.
	static	VFileKind*					Create( const VString& inID, const VectorOfFileKind &inOsKind, const VString& inDescription, const VectorOfFileExtension& inExtensions, const VectorOfFileKind &inParentIDs, bool inIsPublic);
			
private:
										VFileKind( const VString &inID, const VectorOfFileKind &inOsKind, const VString& inDescription, const VectorOfFileExtension& inExtensions, const VectorOfFileKind &inParentIDs, bool inIsPublic)
										: fID( inID), fOsKind(inOsKind), fDescription( inDescription), fFileExtensions( inExtensions), fParentIDs( inParentIDs), fIsPublic( inIsPublic) {;}

										VFileKind( const VFileKind&);
										VFileKind& operator=( const VFileKind& );	// no copy because const object
	
			VString						fDescription;
			VString						fID;
			VectorOfFileKind			fOsKind;
			VectorOfFileExtension		fFileExtensions;	// the extension(s) of the file kind
			VectorOfFileKind			fParentIDs;			// the parent kind IDs (UTIs) for which the file kind is conform
			bool						fIsPublic;
};



typedef std::vector<XBOX::VRefPtr<XBOX::VFile> >		VectorOfVFile;
typedef std::vector<XBOX::VFileDesc*>					VectorOfVFileDesc;
typedef std::vector<XBOX::VRefPtr<XBOX::VFileKind> >	VectorOfVFileKind;

END_TOOLBOX_NAMESPACE

#endif

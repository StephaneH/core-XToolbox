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
#ifndef __VJSRuntime_file__
#define __VJSRuntime_file__

#include "JS4D.h"
#include "VJSClass.h"
#include "VJSRuntime_blob.h"

BEGIN_TOOLBOX_NAMESPACE


class XTOOLBOX_API JS4DFolderIterator : public XBOX::VObject, public XBOX::IRefCountable
{
	public:
		
		JS4DFolderIterator(XBOX::VFolderIterator* inIter)
		{
			fIter = inIter;
			fFolder = NULL;
			fBefore = true;
		}
		
		JS4DFolderIterator(XBOX::VFolder* inFolder)
		{
			fIter = NULL;
			fFolder = inFolder;
			fFolder->Retain();
			fBefore = false;
		}
		
		~JS4DFolderIterator()
		{
			if (fIter != NULL)
				delete fIter;
			if (fFolder != NULL)
				fFolder->Release();
		}
	
		XBOX::VFolder* GetFolder()
		{
			fBefore = false;
			if (fIter == NULL)
				return fFolder;
			else
				return fIter->Current();
		}
		
		bool Next();
		
	protected:
		XBOX::VFolderIterator* fIter;
		XBOX::VFolder* fFolder;
		bool fBefore;
};


class XTOOLBOX_API VJSFolderIterator : public VJSClass<VJSFolderIterator, JS4DFolderIterator>
{
public:
	typedef VJSClass<VJSFolderIterator, JS4DFolderIterator> inherited;
	
	static void				GetDefinition( ClassDefinition& outDefinition);
	static VJSObject		MakeConstructor( const VJSContext& inContext);

	static	void			Construct(VJSParms_construct &ioParms);

private:
	static JS4D::StaticFunction		sConstrFunctions[];

	static bool				parseFolder (VFolder* folder, VJSParms_callStaticFunction& ioParms, VJSObject& objfunc, std::vector<VJSValue>& params, sLONG& counter, VJSObject& thisParam);

	static JS4DFolderIterator*	sDummy;

	static void				_Initialize( const VJSParms_initialize& inParms, JS4DFolderIterator* inFolderIter);
	static void				_Finalize( const VJSParms_finalize& inParms, JS4DFolderIterator* inFolderIter);

	static void				_ConstructFromW3CFS(VJSParms_withArguments& ioParms,XBOX::VJSObject& ioConstructedObject);

	static void				_IsFolder (XBOX::VJSParms_callStaticFunction &ioParms, XBOX::VObject *);

	static void _GetName(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter);  // string : GetName(bool | {"No Extension", "With Extension"})
	static void _SetName(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter);  // bool : SetName( string )
	static void _Valid(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter);  // bool : Valid()
	static void _Next(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter);  // bool : Next()
	static void _FirstSubFolder(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter);  // FolderIterator : FirstSubFolder({"Invisible"|"KeepAlias"})
	static void _FirstFile(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter);  // FileIterator : FirstFile({"Invisible"|"KeepAlias"})
	static void _IsVisible(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter);  // bool : IsVisible()
	static void _SetVisible(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter);  // SetVisible( bool )
	static void _IsReadOnly(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter);  // bool : IsReadOnly()
	static void _SetReadOnly(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter);  // SetReadOnly( bool )
	static void _GetPath(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter);  // string : GetPath()
	static void _GetURL(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter);  // string : GetURL()
	static void _Delete(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter);  // bool : Delete()   |   bool : Drop()
	static void _DeleteContent(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter);  // bool : DeleteContent()
	static void _GetFreeSpace(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter);  // number : GetFreeSpace(bool | "NoQuotas")
	static void _GetVolumeSize(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter);  // number : GetVolumeSize(bool | "NoQuotas")
	static void _Create(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter);  // bool : Create()
	static void _Exists(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter);  // bool : Exists()
	static void _GetParent(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter);  // Folder : GetParent()
	
	static void _eachFile(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter);  // Folder : forEachFile(func)
	static void _eachFolder(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter);  // Folder : forEachFolder(func)
	static void _parse(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter);  // Folder : parse(func)

	static void _files(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter);  // Array : files
	static void _folders(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter);  // Array : folders

	static void _findAllFiles (VJSParms_callStaticFunction &ioParms, JS4DFolderIterator *inFolderIterator);	// Array : files

	static void	_ToJSON( VJSParms_callStaticFunction& ioParms, JS4DFolderIterator *inFolderIter);

	static void _creationDate(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter);
	static void _modificationDate(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter);
	static bool _setcreationDate(VJSParms_setProperty& ioParms, JS4DFolderIterator* inFolderIter); 
	static bool _setmodificationDate(VJSParms_setProperty& ioParms, JS4DFolderIterator* inFolderIter);

	static void _visible(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter);
	static bool _setvisible(VJSParms_setProperty& ioParms, JS4DFolderIterator* inFolderIter); 

	static void _extension(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter);
	static bool _setextension(VJSParms_setProperty& ioParms, JS4DFolderIterator* inFolderIter); 

	static void _name(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter);
	static bool _setname(VJSParms_setProperty& ioParms, JS4DFolderIterator* inFolderIter); 

	static void _nameNoExt(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter);
	static bool _setnameNoExt(VJSParms_setProperty& ioParms, JS4DFolderIterator* inFolderIter); 

	static void _next(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter);

	static void _valid(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter);

	static void _readOnly(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter);
	static bool _setreadOnly(VJSParms_setProperty& ioParms, JS4DFolderIterator* inFolderIter); 

	static void _path(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter);
	static void _pathRel(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter);
	static void _systemPath(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter);

	static void _exists(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter);

	static void _parent(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter);
	static void _filesystem( VJSParms_getProperty& ioParms, JS4DFolderIterator *inFileIter);

	static void _firstFile(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter);
	static void _firstFolder(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter);

	static void	_SetReadOnly (XBOX::VFolder *inFolder, bool inIsReadOnly);

};

//======================================================

class XTOOLBOX_API JS4DFileIterator : public VJSBlobValue
{
public:
									JS4DFileIterator( XBOX::VFileIterator* inIter);
									JS4DFileIterator( XBOX::VFile* inFile);
									JS4DFileIterator( XBOX::VFile* inFile, sLONG8 inStart, sLONG8 inEnd, const VString& inContentType);
		
			XBOX::VFile*			GetFile();
			bool					Next();

	// inherited from VJSBlobValue
	virtual	JS4DFileIterator*		Slice( sLONG8 inStart, sLONG8 inEnd, const VString& inContentType) const;
	virtual	VJSDataSlice*			RetainDataSlice() const;
	virtual	sLONG8					GetMaxSize() const;
	virtual	VError					CopyTo( VFile *inDestination, bool inOverwrite);
		
protected:
									~JS4DFileIterator();
			VFileIterator*			fIter;
			VFile*					fFile;
			bool					fBefore;
};




class XTOOLBOX_API VJSFileIterator : public VJSClass<VJSFileIterator, JS4DFileIterator>
{
public:
	
	typedef VJSClass<VJSFileIterator, JS4DFileIterator> inherited;
	
	static void				GetDefinition (ClassDefinition& outDefinition);
	static VJSObject		MakeConstructor( const VJSContext& inContext);

	static	void			Construct( VJSParms_construct& ioParms);
	
private:
	static JS4D::StaticFunction	sConstrFunctions[];
	static JS4DFileIterator*	sDummy;

	static void				_Initialize( const VJSParms_initialize& inParms, JS4DFileIterator* inFileIter);
	static void				_Finalize( const VJSParms_finalize& inParms, JS4DFileIterator* inFileIter);
	static void 			_ConstructFromW3CFS (VJSParms_withArguments &ioParms, XBOX::VJSObject& ioConstructedObject);

	static void				_IsFile (XBOX::VJSParms_callStaticFunction &ioParms, XBOX::VObject *);

	static void _GetName(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter);  // string : GetName(bool | {"No Extension", "With Extension"})
	static void _SetName(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter);  // bool : SetName( string )
	static void _Valid(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter);  // bool : Valid()
	static void _Next(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter);  // bool : Next()
	static void _IsVisible(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter);  // bool : IsVisible()
	static void _SetVisible(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter);  // SetVisible(bool)
	static void _IsReadOnly(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter);  // bool : IsReadOnly()
	static void _SetReadOnly(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter);  // SetReadOnly( bool )
	static void _GetPath(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter);  // string : GetPath()
	static void _GetURL(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter);  // string : GetURL()
	static void _Delete(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter);  // bool : Delete()   |   bool : Drop()
	static void _GetExtension(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter);  // string : GetExtension()
	static void _GetSize(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter);  // number : GetSize()
	static void _GetFreeSpace(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter);  // number : GetFreeSpace(bool | "NoQuotas")
	static void _GetVolumeSize(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter);  // number : GetVolumeSize(bool | "NoQuotas")
	static void _Create(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter);  // bool : Create()
	static void _Exists(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter);  // bool : Exists()
	static void _GetParent(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter);  // Folder : GetParent()
	static void _MoveTo(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter);  // bool : MoveTo( File | StringURL, { bool | "OverWrite"} )
	static void	_Slice( VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter);
	static void	_ToJSON( VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter);

	static void _creationDate(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter);
	static void _lastModifiedDate(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter);
	static void _type(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter);
	static bool _setcreationDate(VJSParms_setProperty& ioParms, JS4DFileIterator* inFileIter); 
	static bool _setLastModifiedDate(VJSParms_setProperty& ioParms, JS4DFileIterator* inFileIter);

	static void _visible(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter);
	static bool _setvisible(VJSParms_setProperty& ioParms, JS4DFileIterator* inFileIter); 

	static void _extension(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter);
	static bool _setextension(VJSParms_setProperty& ioParms, JS4DFileIterator* inFileIter); 

	static void _name(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter);
	static bool _setname(VJSParms_setProperty& ioParms, JS4DFileIterator* inFileIter); 

	static void _nameNoExt(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter);
	static bool _setnameNoExt(VJSParms_setProperty& ioParms, JS4DFileIterator* inFileIter); 

	static void _next(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter);

	static void _valid(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter);

	static void _readOnly(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter);
	static bool _setreadOnly(VJSParms_setProperty& ioParms, JS4DFileIterator* inFileIter); 

	static void _path(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter);
	static void _pathRel(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter);
	static void _systemPath(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter);

	static void _exists(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter);

	static void _parent(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter);
	static void _filesystem( VJSParms_getProperty& ioParms, JS4DFileIterator *inFileIter);
};

// Additionnal routines for POSIX path management in JavaScript runtime.

class XTOOLBOX_API VJSPath : public XBOX::VObject
{
public:

	// Mac path have startup volume as prefix, remove this if not needed. Does nothing on Linux and Windows platform
	static void			CleanPath( VString *ioPath);
		
	// mac and linux: Resolve a POSIX path, especially follow all its link(s). 
	// window: Remove double slashes from a POSIX path, must no be used with "file://c:/folder/test.txt" type of URL.
		
	static void			ResolvePath( VString *ioPath);

	static	VFile*		ResolveFileParam( const VJSContext& inContext, const VString& inJSPathParam, EFSNOptions inOptions = eFSN_Default);
	static	VFolder*	ResolveFolderParam( const VJSContext& inContext, const VString& inJSPathParam);
	
	static	bool		CanAccessAllFiles( const VJSContext& inContext);

private:
	
#if VERSIONMAC
	
	static XBOX::VCriticalSection	sMutex;
	static XBOX::VString			sStartupVolumePrefix;
	
#endif

	// Remove "//" sequences from path. This is needed because VFilePath will reject them.

	static void			_RemoveMultipleFolderSeparators (XBOX::VString *ioPath);
};

END_TOOLBOX_NAMESPACE

#endif
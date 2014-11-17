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
#include "VJavaScriptPrecompiled.h"

#include "VSystemWorker.h"
#include "VJSErrors.h"

USING_TOOLBOX_NAMESPACE


VSystemWorker::VSystemWorker ( const VString &inName, const VFilePath &inPath )
: fName ( inName )
, fPath ( inPath )
{
	;
}

/*
	static
*/
VSystemWorker* VSystemWorker::Create ( const VString &inName, const VFilePath &inPath, VError* outError )
{
	VError				vError = VE_OK;
	VSystemWorker*		sysWorker = NULL;

	if ( inName. IsEmpty ( ) ) // Other naming constraints may be necessary
	{
		StThrowError<>	err ( VE_SW_INVALID_NAME );
		err-> SetString ( CVSTR ( "p1" ), inName );
		vError = err. GetError ( );
	}

	if ( vError == VE_OK )
	{
		VFile			vf ( inPath );
		if ( !vf. Exists ( ) )
		{
			StThrowError<>	err ( VE_FILE_NOT_FOUND );
			err-> SetString ( CVSTR ( "name" ), inPath. GetPath ( ) );
			vError = err. GetError ( );
		}
	}

	if ( vError == VE_OK )
	{
		sysWorker = new VSystemWorker ( inName, inPath );
	}
		
	if ( outError != NULL )
		*outError = vError;
	
	return sysWorker;
}



VSystemWorkerNamespace::VSystemWorkerNamespace ( VSystemWorkerNamespace* inParent )
: fParent ( RetainRefCountable ( inParent ) )
{

}

VSystemWorkerNamespace::~VSystemWorkerNamespace ( )
{
	ReleaseRefCountable( &fParent);
}

void VSystemWorkerNamespace::RegisterSystemWorker ( VSystemWorker* inSystemWorker )
{
	if ( inSystemWorker != NULL )
	{
		VTaskLock	lock ( &fMutex );
		
		fMap [ inSystemWorker-> GetName ( ) ] = inSystemWorker;
	}
}

void VSystemWorkerNamespace::UnregisterSystemWorker ( const VString& inName )
{
	VTaskLock	lock ( &fMutex );
	
	fMap. erase ( inName );
}

VSystemWorker* VSystemWorkerNamespace::RetainSystemWorker ( const VString& inName )
{
	VSystemWorker*				sysWorker = NULL;

	{
		VTaskLock				lock ( &fMutex );
		
		MapOfSystemWorkers::const_iterator	i = fMap. find ( inName );
		if ( i != fMap. end ( ) )
			sysWorker = i-> second. Retain ( );
	}
	
	if ( sysWorker == NULL && fParent != NULL )
		sysWorker = fParent-> RetainSystemWorker ( inName );
	
	return sysWorker;
}

VError VSystemWorkerNamespace::LoadFromDefinitionFile ( VFile* inFile, VFileSystemNamespace & inFSNamespace )
{
	if ( !testAssert ( inFile != NULL ) )
		return vThrowError ( VE_INVALID_PARAMETER );

	VJSONValue							valDefintion;
	VError								vError = VJSONImporter::ParseFile ( inFile, valDefintion, VJSONImporter::EJSI_Strict );

	if ( ( vError == VE_OK ) && testAssert ( valDefintion. IsArray ( ) ) )
	{
		VTaskLock						lock ( &fMutex );
		VJSONArray*						arrWorkers = valDefintion. GetArray ( );
		
		bool							bOK = false;
		size_t							nCount = arrWorkers-> GetCount ( );
		for ( size_t i = 0 ; ( i < nCount ) && ( vError == VE_OK ) ; i++ )
		{
			const VJSONValue &			jsonWorker = ( *arrWorkers ) [ i ];
			if ( testAssert ( jsonWorker. IsObject ( ) ) )
			{
				VJSONObject*			workerObject = jsonWorker. GetObject ( );

				VString					vstrName;
				bOK = workerObject-> GetPropertyAsString ( CVSTR ( "name" ), vstrName );
				if ( bOK )
				{
					// Check if a system worker with the given name exists already
					VSystemWorker*		vSysAlready = RetainSystemWorker ( vstrName );
					if ( vSysAlready != NULL )
					{
						ReleaseRefCountable ( &vSysAlready );

						continue;
					}

					VJSONValue			jsonExecutableProperty = jsonWorker. GetProperty ( CVSTR ( "executable" ) );
					if ( jsonExecutableProperty. IsArray ( ) )
					{
						VJSONArray*		arrPaths = jsonExecutableProperty. GetArray ( );
						size_t			nPathCount = arrPaths-> GetCount ( );
						for ( size_t j = 0; j < nPathCount; j++ )
						{
							const VJSONValue &				jsonPathValue = ( *arrPaths ) [ j ];
							VJSONValue						jsonPathProperty = jsonPathValue. GetProperty ( CVSTR ( "path" ) );
							if ( testAssert ( jsonPathProperty. IsString ( ) ) )
							{
								VString						vstrPath;
								vError = jsonPathProperty. GetString ( vstrPath );
								if ( testAssert ( vError == VE_OK ) )
								{
									VFile*					flWorker = NULL;
									VFolder*				fldrWorker = NULL;
									bool					bExists = false;
									VFilePath				vWorkerPath;
									bOK = inFSNamespace. ParsePathWithFileSystem ( vstrPath, &flWorker, &fldrWorker, eFSN_SearchParent );
									if ( bOK && flWorker != NULL )
									{
										bExists = flWorker-> Exists ( );
										if ( bExists )
											vWorkerPath = flWorker-> GetPath ( );
#if VERSIONWIN
										else
										{
											VString			vstrExtension;
											flWorker-> GetExtension ( vstrExtension );
											if ( vstrExtension. GetLength ( ) == 0 )
											{
												// Let's try with ".exe"
												VFilePath	vfPathExe = flWorker-> GetPath ( );
												vfPathExe. SetExtension ( CVSTR ( "exe" ) );
												VFile		vExeFile ( vfPathExe );
												bExists = vExeFile. Exists ( );
												if ( bExists )
													vWorkerPath = vfPathExe;
											}
										}
#endif
									}
									else
									{
										// Let's try to parse this as an absolute POSIX file path
										vWorkerPath. FromFullPath ( vstrPath, FPS_POSIX );
										if ( vWorkerPath. IsValid ( ) )
										{
											VFile			vfWorker ( vstrPath, FPS_POSIX );
											bExists = vfWorker. Exists ( );
										}
									}

									ReleaseRefCountable ( &flWorker );
									ReleaseRefCountable ( &fldrWorker );

									if ( bExists )
									{
										VSystemWorker*		vSysWorker = VSystemWorker::Create ( vstrName, vWorkerPath, &vError );
										if ( vSysWorker != NULL && vError == VE_OK )
											RegisterSystemWorker ( vSysWorker );

										ReleaseRefCountable ( &vSysWorker );

										break; // The first path that exists wins
									}
								}
							}
							else
								vError = vThrowError ( VE_MALFORMED_JSON_DESCRIPTION );
						}
					}
					else
					{
						// If "executable" property is not present then try to find an executable with exactly the same name as worker's label
						VFilePath							vWorkerPath;
						bOK = VSystem::SearchExecutablePath ( vstrName, vWorkerPath );
						if ( bOK )
						{
							VSystemWorker*					vSysWorker = VSystemWorker::Create ( vstrName, vWorkerPath, &vError );
							if ( vSysWorker != NULL && vError == VE_OK )
								RegisterSystemWorker ( vSysWorker );

							ReleaseRefCountable ( &vSysWorker );
						}
						else
							vError = vThrowError ( VE_SW_WORKER_NOT_FOUND, vstrName );
					}
				}
				else
					vError = vThrowError ( VE_MALFORMED_JSON_DESCRIPTION );
			}
			else
				vError = vThrowError ( VE_MALFORMED_JSON_DESCRIPTION );
		}
	}

	if ( vError != VE_OK )
	{
		StThrowError<>			errLoad ( VE_SW_CANT_LOAD_DEFINITION_FILE );
		errLoad-> SetString ( CVSTR ( "path" ), inFile-> GetPath ( ). GetPath ( ) );
	}
	
	return vError;
}

VError VSystemWorkerNamespace::InitAppFileSystems ( VFileSystemNamespace* inFSNamespace )
{
	VError						vError = VE_OK;

	if ( inFSNamespace != NULL )
	{
		VFilePath		vfPathUsrBin;
		VFilePath		vfPathUsrLocal;
		VFilePath		vfpSystem;
#if VERSIONWIN
		// TODO: Need to implement and use CSIDL_PROGRAM_FILESX86
		VFolder*		vfUserDocs = VFolder::RetainSystemFolder( eFK_UserDocuments, false );
		VString			vstrPUserDocs;
		vfUserDocs->GetPath ( vstrPUserDocs );
		ReleaseRefCountable ( &vfUserDocs );

		VString			vstrProgramFiles;
		vstrProgramFiles. AppendUniChar ( vstrPUserDocs. GetUniChar ( 1 ) );
		vstrProgramFiles. AppendCString ( ":\\Program Files (x86)\\" );

		vfPathUsrBin. FromFullPath ( vstrProgramFiles, FPS_SYSTEM );
		inFSNamespace-> RegisterFileSystem ( CVSTR ( ".USR_BIN" ), vfPathUsrBin, eFSO_Private );

		vfPathUsrLocal. FromFullPath ( vstrProgramFiles, FPS_SYSTEM );
		inFSNamespace-> RegisterFileSystem ( CVSTR ( ".USR_LOCAL" ), vfPathUsrLocal, eFSO_Private );

		VString			vstrSystem;
		vstrSystem. AppendUniChar ( vstrPUserDocs. GetUniChar ( 1 ) );
		vstrSystem. AppendCString ( ":\\Windows\\System32\\" );
		vfpSystem. FromFullPath ( vstrSystem, FPS_SYSTEM );
		inFSNamespace-> RegisterFileSystem ( CVSTR ( ".SYSTEM" ), vfpSystem, eFSO_Private );
#else
		vfPathUsrBin. FromFullPath ( CVSTR ( "/usr/bin/" ), FPS_POSIX);
		inFSNamespace-> RegisterFileSystem ( CVSTR ( ".USR_BIN" ), vfPathUsrBin, eFSO_Private );

		vfPathUsrLocal. FromFullPath ( CVSTR ( "/usr/local/" ), FPS_POSIX );
		inFSNamespace-> RegisterFileSystem ( CVSTR ( ".USR_LOCAL" ), vfPathUsrLocal, eFSO_Private );

		vfpSystem. FromFullPath ( CVSTR ( "/bin/" ), FPS_POSIX );
		inFSNamespace-> RegisterFileSystem ( CVSTR ( ".SYSTEM" ), vfpSystem, eFSO_Private );
#endif
	}

	return vError;
}

VError VSystemWorkerNamespace::GetPath ( VString const & inName, VFilePath & outPath )
{
	VError						vError = VE_OK;
	VSystemWorker*				sysWorker = RetainSystemWorker ( inName );
	if ( sysWorker != NULL )
		outPath = sysWorker-> GetPath ( );
	else
		vError = vThrowError ( VE_SW_WORKER_NOT_FOUND, inName );

	ReleaseRefCountable ( &sysWorker );

	return vError;
}


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

#include "VJSSystemWorker.h"

#include "VJSClass.h"
#include "VJSGlobalClass.h"
#include "VJSEvent.h" 
#include "VJSRuntime_file.h" 
#include "VJSRuntime_stream.h"
#include "VJSBuffer.h"
#include "VSystemWorker.h"

// Set to 1 to enable tracing of starting and ending of system worker, 
// along with data received from stdout and stderr.

#define TRACE_SYSTEM_WORKER	0

USING_TOOLBOX_NAMESPACE

XBOX::VCriticalSection			VJSSystemWorker::sMutex;
std::list<VJSSystemWorker *>	VJSSystemWorker::sRunningSystemWorkers;
std::set<sLONG *>				VJSSystemWorker::sRunningCommandLines;

sLONG			VJSSystemWorker::sNumberRunning;
VFilePath		VJSSystemWorker::sDefaultSettingsPath;
VJSONValue*		VJSSystemWorker::sjsonDefaultSettings = NULL;
VFilePath		VJSSystemWorker::sSolutionSettingsPath;
VJSONValue*		VJSSystemWorker::sjsonSolutionSettings = NULL;
VString			VJSSystemWorker::sSolutionParentFullPath;
	
bool VJSSystemWorker::IsRunning (bool inWaitForStartup) 
{
	bool	isOk;

	for ( ; ; ) {

		sLONG	startupStatus;

		if ((startupStatus = XBOX::VInterlocked::AtomicGet((sLONG *) &fStartupStatus))) {
			
			if (startupStatus > 0) {

				// External process has started properly, but is it still running?

				isOk = !fIsTerminated;

			} else
				
				isOk = false;

			break;

		} else {
			
			// Hasn't been launched yet, wait if needed .

			if (inWaitForStartup) 
	
				XBOX::VTask::Sleep(kPollingInterval);

			else {

				// Hasn't start yet, so can't have failed yet!

				isOk = true;
				break;

			}

		}

	}

	return isOk;
}

void VJSSystemWorker::Kill (bool inPanicTermination)
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fCriticalSection);	

	fPanicTermination = inPanicTermination;			// If true, this will prevent generation of a termination event.
	if (!fIsTerminated && fProcessLauncher.IsRunning()) {
		
		fProcessLauncher.Shutdown(true, fKillProcessTree);
		fForcedTermination = fIsTerminated = true;	// _DoRun() will quit.
		
	} else {
	
		// Make sure the process tree is killed.

		if (fKillProcessTree)

			fProcessLauncher.Shutdown(false, true);

	}
}

void VJSSystemWorker::KillAll ()
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&sMutex);

	// When both kind of system workers quit, they will remove themselves from 
	// sRunningSystemWorkers and sRunningCommandLines as needed.

	std::list<VJSSystemWorker *>::iterator	i;

	for (i = sRunningSystemWorkers.begin(); i != sRunningSystemWorkers.end(); i++) 

		(*i)->Kill(true);	

	std::set<sLONG *>::iterator	j;

	for (j = sRunningCommandLines.begin(); j != sRunningCommandLines.end(); j++) 

		**j = true;
}

bool VJSSystemWorker::MatchEvent (const IJSEvent *inEvent, sLONG inEventType)
{
	xbox_assert(inEvent != NULL);

	if (inEvent->GetType() != IJSEvent::eTYPE_SYSTEM_WORKER) 

		return false;

	else {
		
		VJSSystemWorkerEvent	*systemWorkerEvent;

		systemWorkerEvent = (VJSSystemWorkerEvent *) inEvent;
		return systemWorkerEvent->GetSystemWorker() && systemWorkerEvent->GetSubType() == inEventType;

	}
}

VError VJSSystemWorker::Init ( VFilePath const & inSettings, enum ESettingsType inType )
{
	VJSONValue**				vjsonSettings = NULL;
	VFilePath*					vSettingsPath = NULL;
	if ( inType == eTYPE_DEFAULT )
	{
		if ( sDefaultSettingsPath == inSettings )
			return VE_OK;

		vSettingsPath = &sDefaultSettingsPath;
		vjsonSettings = &sjsonDefaultSettings;
	}
	else if ( inType == eTYPE_SOLUTION )
	{
		if ( sSolutionSettingsPath == inSettings )
			return VE_OK;

		if ( inSettings. IsEmpty ( ) )
		{
			delete sjsonSolutionSettings;
			sjsonSolutionSettings = NULL;
			sSolutionSettingsPath. Clear ( );

			return VE_OK;
		}

		vSettingsPath = &sSolutionSettingsPath;
		vjsonSettings = &sjsonSolutionSettings;

		VFilePath		vfpSolutionParent;
		inSettings. GetParent ( vfpSolutionParent );
		vfpSolutionParent. ToParent ( );
		vfpSolutionParent. GetPosixPath ( sSolutionParentFullPath );
	}
	else
		xbox_assert ( false );

	xbox_assert ( inSettings. IsValid ( ) );

	VError			vError = VE_OK;

	VFile			vfSettings ( inSettings );
	if ( vfSettings. Exists ( ) )
	{
		if ( *vjsonSettings != NULL )
			delete ( *vjsonSettings );

		( *vjsonSettings ) = new VJSONValue ( );
		vError = VJSONImporter::ParseFile ( &vfSettings, **vjsonSettings, VJSONImporter::EJSI_Strict );
		if ( vError != VE_OK )
		{
			delete ( *vjsonSettings );
			*vjsonSettings = NULL;
		}
	}
	else
		// Do not throw an error for now while the feature is not ready yet, just return an error.
		vError = VE_FILE_NOT_FOUND; // vThrowError ( VE_FILE_NOT_FOUND );

	if ( vError == VE_OK )
	{
		vSettingsPath-> FromFilePath ( inSettings );
	}

	return vError;
}

VError VJSSystemWorker::GetPath ( const VJSContext& inContext, VString const & inName, VString & outFullPath )
{
	// Start with solution level settings
	VError					vError = GetPath ( inContext, inName, sjsonSolutionSettings, outFullPath );
	if ( vError != VE_OK )
		// Not found, let's search in the global settings
		vError = GetPath ( inContext, inName, sjsonDefaultSettings, outFullPath );

	// It is perfectly normal to have a worker declared but not actually present on disk
	//if ( vError == VE_FILE_NOT_FOUND )
	//	vThrowError ( VE_FILE_NOT_FOUND );

	return vError;
}

VError VJSSystemWorker::GetPath ( const VJSContext& inContext, VString const & inName, VJSONValue const * inSettings, VString & outFullPath )
{
	outFullPath. Clear ( );
	if ( inSettings == NULL )
		return VE_FILE_NOT_FOUND;

	VError			vError = VE_OK;
	VJSONArray*		arrNames = inSettings-> GetArray ( );
	if ( arrNames == NULL )
		return VE_FILE_NOT_FOUND;

	bool			bExists = false;

	for ( size_t i = 0; i < arrNames-> GetCount ( ); i++ )
	{
		const VJSONValue &		jsonNameValue = ( *arrNames ) [ i ];
		VJSONValue				jsonNameProperty = jsonNameValue. GetProperty ( CVSTR ( "name" ) );
		VString					vstrName;
		vError = jsonNameProperty. GetString ( vstrName );
		if ( vError == VE_OK )
		{
			if ( vstrName. EqualToString ( inName ) )
			{
				VJSONValue		jsonExecutableProperty = jsonNameValue. GetProperty ( CVSTR ( "executable" ) );
				xbox_assert ( jsonExecutableProperty. IsArray ( ) );
				VJSONArray*		arrExecutables = jsonExecutableProperty. GetArray ( );
				if ( arrExecutables != NULL )
				{
					VTask::GetCurrent ( )-> GetDebugContext ( ). DisableUI ( );
					for ( size_t j = 0; j < arrExecutables-> GetCount ( ); j++ )
					{
						const VJSONValue &		jsonPathValue = ( *arrExecutables ) [ j ];
						VJSONValue				jsonPathProperty = jsonPathValue. GetProperty ( CVSTR ( "path" ) );
						VString					vstrPath;
						vError = jsonPathProperty. GetString ( vstrPath );
						if ( vstrPath. GetLength ( ) == 0 )
						{
							bExists = true;

							break;
						}

#if VERSIONWIN
// set PATHEXT returns PATHEXT=.COM;.EXE;.BAT;.CMD;.VBS;.VBE;.JS;.JSE;.WSF;.WSH;.MSC
// TODO: Do all these extensions need to be handled?
						if ( !vstrPath. EndsWith ( CVSTR ( ".exe" ) ) )
							vstrPath. AppendCString ( ".exe" );
#endif

						VFile*					vfToTest = VJSPath::ResolveFileParam ( inContext, vstrPath, eFSN_SearchParent );
						if ( vfToTest != NULL )
						{
							VString					vstrResolvedPath;
#if VERSIONWIN
							vfToTest-> GetPath ( vstrResolvedPath, FPS_SYSTEM );
#else
							vfToTest-> GetPath ( vstrResolvedPath, FPS_POSIX );
#endif
							bExists = vfToTest-> Exists ( );
							ReleaseRefCountable ( &vfToTest );
							if ( bExists )
							{
								outFullPath. AppendString ( vstrResolvedPath );

								break;
							}
						}
					}
					VTask::GetCurrent ( )-> GetDebugContext ( ). EnableUI ( );
				}
				else
				{
					vError = vThrowError ( VE_MALFORMED_JSON_DESCRIPTION );

					break;
				}

				break;
			}
		}
		else
		{
			vError = vThrowError ( VE_MALFORMED_JSON_DESCRIPTION );

			break;
		}
	}

	if ( !bExists && vError == VE_OK )
		vError = VE_FILE_NOT_FOUND;

	return vError;
}

VJSSystemWorker::VJSSystemWorker(const XBOX::VString &inCommandLine, const XBOX::VString &inFolderPath, VJSWorker *inWorker, const VJSObject& inObject)
:fThis(inObject)
{
	xbox_assert(inWorker != NULL);

	fWorker = XBOX::RetainRefCountable<VJSWorker>(inWorker);

	fCommandLine = inCommandLine;
	
	fProcessLauncher.CreateArgumentsFromFullCommandLine(inCommandLine);
	fProcessLauncher.SetWaitClosingChildProcess(false);
	fProcessLauncher.SetRedirectStandardInput(true);
	fProcessLauncher.SetRedirectStandardOutput(true);

	// Always kill the entire process tree when terminating or if the object destructor is called.

	fProcessLauncher.EnableProcessTreeKill();

    // inFolderPath must be in "native" format (i.e. HFS for Mac).
    
	if (inFolderPath.GetLength())
		
		fProcessLauncher.SetDefaultDirectory(inFolderPath);
	
#if VERSIONWIN
	
	fProcessLauncher.WIN_DontDisplayWindow();
	fProcessLauncher.WIN_CanGetExitStatus();
	
#endif

	fVTask = new XBOX::VTask(NULL, 0, XBOX::eTaskStylePreemptive, VJSSystemWorker::_RunProc);
	fVTask->SetKindData((sLONG_PTR) this);
	fVTask->SetName( CVSTR( "JS System Worker"));

	fTerminationRequested = fPanicTermination = fForcedTermination = fIsTerminated = fTerminationEventDiscarded = false;
	fStartupStatus = 0;

	fExitStatus = 0;

#if VERSIONMAC || VERSION_LINUX

	fPID = -1;

#else	// VERSIONWIN

	fPID = 0;

#endif

#if USE_V8_ENGINE
#else
	fThis = VJSSystemWorkerClass::CreateInstance(inObject.GetContext(), this);
#endif

	fKillProcessTree = false;
}

VJSSystemWorker::~VJSSystemWorker ()
{
	xbox_assert(fVTask == NULL);
	xbox_assert(fWorker != NULL);

	XBOX::ReleaseRefCountable<VJSWorker>(&fWorker);
}

void VJSSystemWorker::SetEnvironmentVariable(const XBOX::VString &inName, const XBOX::VString &inValue)
{
	xbox_assert(!inName.IsEmpty());

	fProcessLauncher.SetEnvironmentVariable(inName, inValue);
}

void VJSSystemWorker::AddArgument(const XBOX::VString &inArgument)
{
	xbox_assert(!inArgument.IsEmpty());

	fProcessLauncher.AddArgument(inArgument);
}

void VJSSystemWorker::_DoRun ()
{
	// WAK0073145: Object finalization can be called even before external processed launcher had 
	// a chance to run. This can happen if the SystemWorker is created at end of a script and there 
	// is no wait(). It is destroyed immediately. That is why a retain() is called before the vtask
	// run(). Note that VJSSystemWorker may be freed after script's context is freed.

	if (fPanicTermination) {

		// Script already terminated, don't even bother to launch.

		fVTask = NULL;
		Release();
		return;

	}

	// Keep a list of running system workers, this is needed for implementation of KillAll().

	sMutex.Lock();
	sRunningSystemWorkers.push_back(this);
	sMutex.Unlock();

	sLONG	startingStatus;

	startingStatus = !fProcessLauncher.Start() ? +1 : -1;
	XBOX::VInterlocked::Exchange((sLONG *) &fStartupStatus, startingStatus);
 
	if (startingStatus < 0) {

#if TRACE_SYSTEM_WORKER

		XBOX::DebugMsg("SysWorker %p (\"", this);
		XBOX::DebugMsg(fCommandLine);
		XBOX::DebugMsg("\") failed to start!\n");

#endif

		fIsTerminated = true;	// Launched failed!

	} else {

#if TRACE_SYSTEM_WORKER

		XBOX::DebugMsg("SysWorker %p (\"", this);
		XBOX::DebugMsg(fCommandLine);
		XBOX::DebugMsg("\") started.\n");

#endif
				
		fPID = fProcessLauncher.GetPid();
				
		XBOX::VInterlocked::Increment((sLONG *) &VJSSystemWorker::sNumberRunning);

		uBYTE	*readBuffer;

		readBuffer = (uBYTE *) ::malloc(kBufferSize);
		xbox_assert(readBuffer != NULL);

		while (!fIsTerminated) {

			bool	hasProducedData;
			sLONG	size;
			
			hasProducedData	= false;
				
			fCriticalSection.Lock();		

			// Data on stdout?
			
			if (!fPanicTermination 
			&& (size = fProcessLauncher.ReadFromChild((char *) readBuffer, kBufferSize - 1)) > 0) {

				fCriticalSection.Unlock();

				readBuffer[size] = '\0';

#if TRACE_SYSTEM_WORKER

				XBOX::DebugMsg("SysWorker %p received on stdout: \"%s\".\n", this, readBuffer);

#endif

				fWorker->QueueEvent(VJSSystemWorkerEvent::Create(this, VJSSystemWorkerEvent::eTYPE_STDOUT_DATA, fThis, readBuffer, size));
				hasProducedData = true;

				readBuffer = (uBYTE *) ::malloc(kBufferSize);
				xbox_assert(readBuffer != NULL);
				
				fCriticalSection.Lock();

			}
			
			// Data on stderr?

			if (!fPanicTermination 
			&& (size = fProcessLauncher.ReadErrorFromChild((char *) readBuffer, kBufferSize - 1)) > 0) {

				fCriticalSection.Unlock();
				
				readBuffer[size] = '\0';

#if TRACE_SYSTEM_WORKER

				XBOX::DebugMsg("SysWorker %p received on stderr: \"%s\".\n", this, readBuffer);

#endif

				fWorker->QueueEvent(VJSSystemWorkerEvent::Create(this, VJSSystemWorkerEvent::eTYPE_STDERR_DATA, fThis, readBuffer, size));
				hasProducedData = true;

				readBuffer = (uBYTE *) ::malloc(kBufferSize);
				xbox_assert(readBuffer != NULL);
				
				fCriticalSection.Lock();
							
			}
			
			// Check for termination condition. 
			
			if (fVTask->GetState() == XBOX::TS_DYING || fTerminationRequested || fPanicTermination) {
							
				fProcessLauncher.Shutdown(true, fKillProcessTree); 

				fCriticalSection.Unlock();

				fIsTerminated = fForcedTermination = true;
				break;
					
			} else if (!hasProducedData) {
			
				// Check external process termination only if no data has been produced.
				// This will ensure that data is fully read. IsRunning() will clean-up
				// the terminated process, and any data not yet read with it.
			
				if (!fProcessLauncher.IsRunning()) {
				
					// If process tree is to be killed, then make sure it is.

					if (fKillProcessTree)

						fProcessLauncher.Shutdown(false, true); 

					fCriticalSection.Unlock();

					fForcedTermination = false;
					fIsTerminated = true;				

					break;

				} else {
				
					// No data, sleep then poll later.
				
					fCriticalSection.Unlock();
					XBOX::VTask::Sleep(kPollingInterval);
				
				}
			
			} else {
			
				// Data has been produced, just yield. Try to flush read buffers. 
				
				fCriticalSection.Unlock();
				XBOX::VTask::Yield();

			}

		}

		::free(readBuffer);
				
		XBOX::VInterlocked::Decrement((sLONG *) &VJSSystemWorker::sNumberRunning);
		
		if (!fForcedTermination && !fPanicTermination) {

			fCriticalSection.Lock();
			fExitStatus = fProcessLauncher.GetExitStatus();	
			fCriticalSection.Unlock();

		}
		
	}

	// If not a "panic" termination, trigger an event.

	if (!fPanicTermination) {

		VJSSystemWorkerEvent::STerminationData	*data = new VJSSystemWorkerEvent::STerminationData();

		data->fHasStarted = fStartupStatus > 0;
		data->fForcedTermination = fForcedTermination;
		data->fExitStatus = fExitStatus;
		data->fPID = fPID;
		
#if TRACE_SYSTEM_WORKER
		
		XBOX::DebugMsg("SysWorker %p queues termination event.\n", this);
		
#endif

		fWorker->QueueEvent(VJSSystemWorkerEvent::Create(this, VJSSystemWorkerEvent::eTYPE_TERMINATION, fThis, (uBYTE *) data, sizeof(*data)));
			
	} else {
		
#if TRACE_SYSTEM_WORKER
		
		XBOX::DebugMsg("SysWorker %p panic termination!\n", this);
		
#endif
		
	}
	
	fVTask = NULL;

#if TRACE_SYSTEM_WORKER

	XBOX::DebugMsg("SysWorker %p has terminated.\n", this);

#endif

	// Remove from running system workers list.

	sMutex.Lock();
	
	std::list<VJSSystemWorker *>::iterator	i;

	for (i = sRunningSystemWorkers.begin(); i != sRunningSystemWorkers.end(); i++) 

		if (*i == this) {

			sRunningSystemWorkers.erase(i);
			break;

		}

	xbox_assert(i != sRunningSystemWorkers.end());

	sMutex.Unlock();

	Release();
}

sLONG VJSSystemWorker::_RunProc (XBOX::VTask *inVTask)
{
	((VJSSystemWorker *) inVTask->GetKindData())->_DoRun();	
	return 0;
}

void VJSSystemWorkerClass::GetDefinition (ClassDefinition &outDefinition)
{
    static inherited::StaticFunction functions[] =
	{
		{ "postMessage",		js_callStaticFunction<_postMessage>,		JS4D::PropertyAttributeDontDelete	},
		{ "endOfInput",			js_callStaticFunction<_endOfInput>,			JS4D::PropertyAttributeDontDelete	},
		{ "terminate",			js_callStaticFunction<_terminate>,			JS4D::PropertyAttributeDontDelete	},	
		{ "getInfos",			js_callStaticFunction<_getInfos>,			JS4D::PropertyAttributeDontDelete	},
		{ "getNumberRunning",	js_callStaticFunction<_getNumberRunning>,	JS4D::PropertyAttributeDontDelete	},
		{ "setBinary",			js_callStaticFunction<_setBinary>,			JS4D::PropertyAttributeDontDelete	},
		{ "wait",				js_callStaticFunction<_wait>,				JS4D::PropertyAttributeDontDelete	},
		{ 0, 0, 0 },
	};
#if USE_V8_ENGINE
	// on V8 a class name should not be the same as a global object property name -> indeed a global function named SystemWorker is already added 
    outDefinition.className			= "SystemWorker_";
#else
	outDefinition.className			= "SystemWorker";
#endif
    outDefinition.staticFunctions	= functions;
    outDefinition.finalize			= js_finalize<_Finalize>;
}

JS4D::StaticFunction VJSSystemWorkerClass::sConstrFunctions[] =
{
	{	"exec",	XBOX::js_callback<void, _Exec>,	JS4D::PropertyAttributeNone },
	{	0,		0,								0							},
};

XBOX::VJSObject VJSSystemWorkerClass::MakeConstructor (const XBOX::VJSContext& inContext)
{
	XBOX::VJSObject	constructor =
		JS4DMakeConstructor(inContext, Class(), _Construct, false,	VJSSystemWorkerClass::sConstrFunctions );

	return constructor;
}

bool VJSSystemWorkerClass::_ExtractCommandArray (XBOX::VJSArray &inArray, XBOX::VString *outCommandLine, std::vector<XBOX::VString> *ioArguments)
{
	xbox_assert(outCommandLine != NULL && ioArguments != NULL && ioArguments->empty());

	bool	isOk;

	isOk = false;
	if (inArray.GetLength() > 0) {

		XBOX::VJSValue	value(inArray.GetContext());

		value = inArray.GetValueAt(0);
		if (value.IsString()) {

			value.GetString(*outCommandLine);
			for (VIndex i = 1; i < inArray.GetLength(); i++) {

				value = inArray.GetValueAt(i);
				if (value.IsString()) {

					XBOX::VString	string;

					value.GetString(string);
					ioArguments->push_back(string);

				}
				else if (value.IsNumber()) {

					Real			floatingPointValue;
					XBOX::VString	numberString;

					value.GetReal(&floatingPointValue);	// Internally JavaScript stores numbers in double floating-point.
					numberString.FromReal(floatingPointValue);
					ioArguments->push_back(numberString);

				}
				else {

					// Silently ignore everything else.

				}

			}
			isOk = true;

		}

	}

	return isOk;
}

void VJSSystemWorkerClass::_Construct (VJSParms_construct &ioParms)
{
#if USE_V8_ENGINE
	XBOX::VJSObject constructedObject = ioParms.fConstructedObject;
#else
	XBOX::VJSObject	constructedObject(ioParms.GetContext());
#endif

	XBOX::VString				commandLine;
	std::vector<XBOX::VString>	arguments;
	XBOX::VString				folderPath;
	bool						bQuoteArgumentsIfNeeded = true;
	bool						isOk;	
	bool						bErrorAlreadyThrown = false;
	bool						bKillProcessTree = false;
	
	std::vector< std::pair<VString, VString> >			vctrEnvironmentVars;

#if WITH_SANDBOXED_SYSTEM_WORKERS
	bool						bIsNewSignature = false;
	if ( ioParms. CountParams ( ) == 2 && ( ioParms. IsStringParam ( 1 ) || ioParms. IsArrayParam ( 1 ) ) && ioParms. IsObjectParam ( 2 ) )
	{
		VFolder*				vf = ioParms. RetainFolderParam ( 2, false );
		bIsNewSignature = ( vf == NULL );
		ReleaseRefCountable ( &vf );
	}

	bIsNewSignature = bIsNewSignature || ( ioParms. CountParams ( ) > 3 ); // FORCE THE NEW MODE. FOR TESTS ONLY!!!

	if ( bIsNewSignature ) // The new sand-boxed workers
	{
		if ( ioParms. CountParams ( ) > 3 )
		{
			VString		vstrStdInContent;
			VError		vError = _ReadParametersFS ( ioParms, commandLine, folderPath, vstrStdInContent );
			isOk = ( vError == VE_OK );
		}
		else
		{
			VString		vstrStdInContent;
			VError		vError = _ReadObjectParametersFS ( ioParms, arguments, folderPath, vstrStdInContent, vctrEnvironmentVars, bKillProcessTree );
			if ( vError == VE_OK && arguments. size ( ) > 0 )
			{
				commandLine = arguments [ 0 ];
				arguments. erase ( arguments. begin ( ) );
				bQuoteArgumentsIfNeeded = false;
			}

			isOk = ( vError == VE_OK );
			if ( vError != VE_OK )
				bErrorAlreadyThrown = ( VTask::GetCurrent ( )-> GetLastError ( ) != VE_OK );
		}
	}
	else
#endif	// WITH_SANDBOXED_SYSTEM_WORKERS
	if (!ioParms.CountParams() || !ioParms.IsStringParam(1) || !ioParms.GetStringParam(1, commandLine)) {

		XBOX::VJSArray	argumentArray(ioParms.GetContext());

		if (ioParms.CountParams() > 0 && ioParms.IsArrayParam(1) && ioParms.GetParamArray(1, argumentArray)) {

			isOk = _ExtractCommandArray(argumentArray, &commandLine, &arguments);			
			if (!isOk) 

				XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_ARRAY, "1");
							
		} else {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
			isOk = false;

		}
				
	} else

		isOk = _RetrieveFolder(ioParms, 2, &folderPath);

	XBOX::VJSObject	variables(ioParms.GetContext());

	variables.ClearRef();
	
	// Get optional environment variables.

	if (isOk) {

		if (ioParms.CountParams() > 3) {

			// Sand-boxed workers.

			if (!ioParms.IsNullOrUndefinedParam( 6) && !ioParms.GetParamObject(6, variables)) {

				XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_OBJECT, "6");
				isOk = false;

			}
			
		} else {
			
			if (!ioParms.IsNullOrUndefinedParam( 3) && !ioParms.GetParamObject(3, variables)) {

				XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_OBJECT, "3");
				isOk = false;

			}

		}
		
	}
	
	if (isOk) {
 
		VJSSystemWorker	*systemWorker;
		VJSWorker		*worker;
				
		worker = VJSWorker::RetainWorker(ioParms.GetContext());

		systemWorker = new VJSSystemWorker(commandLine, folderPath, worker, constructedObject);		

#if USE_V8_ENGINE

// WAK0089473: Associated C++ object of SystemWorker JavaScript object isn't properly set.

		ClassDefinition	definition;

		VJSSystemWorkerClass::GetDefinition(definition);
		constructedObject.SetPrivateData(systemWorker, &definition);

#endif

		XBOX::ReleaseRefCountable<VJSWorker>(&worker);

		systemWorker->fKillProcessTree = bKillProcessTree;

		systemWorker->fProcessLauncher.SetQuoteArgumentsIfNeeded ( bQuoteArgumentsIfNeeded );

		// Push arguments if any.

		for (VIndex i = 0; i < arguments.size(); i++)

			systemWorker->AddArgument(arguments.at(i));

		// If there are (optional) environment variable definition(s), then set them.

		if ( vctrEnvironmentVars. size ( ) > 0 )
		{
			for ( size_t i = 0; i < vctrEnvironmentVars. size ( ); i++ )
			{
				systemWorker-> SetEnvironmentVariable ( vctrEnvironmentVars [ i ]. first, vctrEnvironmentVars [ i ]. second );
			}
		}

		if (variables.HasRef()) {

			XBOX::VJSPropertyIterator	it(variables);

			for (; it.IsValid(); ++it) {
				
				XBOX::VJSValue	jsValue(it.GetProperty());
				XBOX::VString	name, value;

				if (jsValue.IsString()) {

					it.GetPropertyName(name);
					if (jsValue.GetString(value))

						systemWorker->SetEnvironmentVariable(name, value);

				} else if (jsValue.IsNumber()) {

					Real	number;	// JavaScript numbers are double type floats.

					it.GetPropertyName(name);
					if (jsValue.GetReal(&number)) {

						value.FromReal(number);
						systemWorker->SetEnvironmentVariable(name, value);

					}

				} else {

					// Silently ignore if value is not a string or number.

				}

			}

		}

		if (systemWorker->fThis.HasRef())
		{
			systemWorker->Retain();	// WAK0073145: See comment at start of VJSSystemWorker::_DoRun().

			systemWorker->fVTask->Run();
		
//** Prevent SystemWorker object from being garbage collected.

			systemWorker->fThis.Protect();
			ioParms.ReturnConstructedObject(systemWorker->fThis);

		}
	}
	else
	{
		if ( !bErrorAlreadyThrown )
			XBOX::vThrowError ( VE_FILE_NOT_FOUND ); // Throw this generic error only if there is no other error.
	}
}

VError VJSSystemWorkerClass::_ReadParametersFS ( XBOX::VJSParms_withArguments& ioParms, XBOX::VString& outCommandLine, XBOX::VString& outRootFolderPath, XBOX::VString& outStdInContent )
{
	if ( ioParms. CountParams ( ) < 4 )
		return vThrowError ( VE_JVSC_EXPECTING_PARAMETER );

	VError			vError = VE_UNKNOWN_ERROR;

	VString			vstrExecutableLabel;
	VString			vstrFullPath;
	VString			vstrPattern;
	VFolder*		fldrExecutionRoot = NULL;
	VJSArray		arrParameters ( ioParms. GetContext ( ) );

	outCommandLine. Clear ( );

	bool		bOk = ioParms. GetStringParam ( 1, vstrExecutableLabel );
	if ( !bOk )
		vError = vThrowError ( XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1" );

	if ( bOk )
	{
		bOk = ioParms. GetStringParam ( 2, vstrPattern );
		if ( !bOk )
			vError = vThrowError ( XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2" );
	}

	if ( bOk )
	{
		fldrExecutionRoot = ioParms. RetainFolderParam ( 3, false );
	}

	bool			bParamsAreNull = ioParms. IsNullOrUndefinedParam( 4 );
	if ( bOk && !bParamsAreNull )
		bOk = ioParms. GetParamArray( 4, arrParameters );

	if ( bOk && ioParms. IsStringParam ( 5 ) )
	{
		bOk = ioParms. GetStringParam ( 5, outStdInContent );
		if ( !bOk )
			vError = XBOX::vThrowError ( XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "5" );
	}

	if ( bOk )
		vError = _ResolvePath ( vstrExecutableLabel, ioParms. GetContext ( ), vstrFullPath );

	VString			vstrExpandedPattern;
	if ( bOk && vError == VE_OK )
	{
		std::vector< XBOX::VString >		vctrPattern;
		vctrPattern. push_back ( vstrPattern );
		std::vector< XBOX::VString >		vctrCommand;
		VString								vstrQuote;
		_GetDefaultQuote ( vstrQuote );
		vError = _PatternParametersToString ( arrParameters, vctrPattern, vstrQuote, vctrCommand );
		if ( vError == VE_OK  && vctrCommand. size ( ) > 0 )
			vstrExpandedPattern = vctrCommand [ 0 ];
	}

	if ( vError == VE_OK )
	{
#if VERSIONWIN
		if ( !( vstrExecutableLabel. EqualToString ( CVSTR ( ".shell" ) ) && vstrFullPath. GetLength ( ) == 0 ) )
			outCommandLine += ( "\"" + vstrFullPath + "\" " );

		outCommandLine += vstrExpandedPattern;
#else
		if ( vstrExecutableLabel. EqualToString ( CVSTR ( ".shell" ) ) && vstrFullPath. GetLength ( ) == 0 )
			outCommandLine += vstrExpandedPattern;
		else
			outCommandLine += ( "bash -c \"'" + vstrFullPath + "' " + vstrExpandedPattern + " 2>&1\"" );
#endif

		// May need to go up one level if this is git worker as there is no JS API to get to the repository root folder.
		if ( fldrExecutionRoot != NULL )
			fldrExecutionRoot-> GetPath ( outRootFolderPath, FPS_SYSTEM );
	}

	ReleaseRefCountable ( &fldrExecutionRoot );
	if ( !bOk && vError == VE_OK )
		vError = VE_UNKNOWN_ERROR;

	return vError;
}

VError VJSSystemWorkerClass::_ReadObjectParametersFS ( XBOX::VJSParms_withArguments& ioParms, std::vector<XBOX::VString>& outCommandLine, XBOX::VString& outRootFolderPath, XBOX::VString& outStdInContent, std::vector< std::pair<VString, VString> >& outEnvironmentVars, bool& outKillProcessTree )
{
	if ( ioParms. CountParams ( ) != 2 )
		return vThrowError ( VE_JVSC_EXPECTING_PARAMETER );

	VError						vError = VE_OK;

	VString						vstrExecutableParameters;
	VString						vstrExecutableLabel;
	VString						vstrFullPath;
	VString						vstrPattern;
	VFolder*					fldrExecutionRoot = NULL;

//outCommandLine. Clear ( );

	std::vector< XBOX::VString >		vctrPattern;

	bool						bWithArguments = true;
	bool						bOk = !( ioParms. IsNullOrUndefinedParam ( 1 ) );
	if ( bOk )
	{
		if ( ioParms. IsArrayParam ( 1 ) )
		{
			XBOX::VJSArray		varrCommand ( ioParms. GetContext ( ) );
			bOk = ioParms. GetParamArray ( 1, varrCommand );
			if ( bOk && varrCommand. GetLength ( ) > 0 )
			{
				// The first array element is the system worker label, the rest - command parameters
				VJSValue		vjsvalWorkerName = varrCommand. GetValueAt ( 0 );
				if ( vjsvalWorkerName. IsNull ( ) || vjsvalWorkerName. IsUndefined ( ) || !vjsvalWorkerName. IsString ( ) )
				{
					vError = vThrowError ( XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1" );
					bOk = false;
				}
				else
				{
					bOk = vjsvalWorkerName. GetString ( vstrExecutableLabel );
					if ( bOk )
					{
						for ( size_t i = 1; i < varrCommand. GetLength ( ); i++ )
						{
							VJSValue		vjsvalArgument = varrCommand. GetValueAt ( i );
							if ( !vjsvalArgument. IsNull ( ) && !vjsvalArgument. IsUndefined ( ) && vjsvalArgument. IsString ( ) )
							{
								VString		vstrArgument;
								bOk = vjsvalArgument. GetString ( vstrArgument );
								if ( bOk )
									vctrPattern. push_back ( vstrArgument );
							}
						}
					}
					else
						vError = vThrowError ( XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1" );
				}
			}
		}
		else
		{
			// The case where the whole command line is provided as one string parameter
			bWithArguments = false;
			bOk = ioParms. GetStringParam ( 1, vstrExecutableLabel );
			if ( bOk )
			{
				vstrExecutableLabel. RemoveWhiteSpaces ( true, false );
				VIndex				nSpaceIndex = vstrExecutableLabel. FindUniChar ( CHAR_SPACE );
				if ( nSpaceIndex > 0 )
				{
					vstrExecutableLabel. GetSubString ( nSpaceIndex + 1, vstrExecutableLabel. GetLength ( ) - nSpaceIndex, vstrExecutableParameters );
					vstrExecutableLabel. SubString ( 1, nSpaceIndex - 1 );
					vctrPattern. push_back ( vstrExecutableParameters );
				}
			}
			else
				vError = vThrowError ( XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1" );
		}
	}

	if ( bOk )
	{
		vError = _ResolvePath ( vstrExecutableLabel, ioParms. GetContext ( ), vstrFullPath );
		bOk = ( vError == VE_OK );
	}

	if ( bOk )
		bOk = !( ioParms. IsNullOrUndefinedParam ( 2 ) );

	VJSObject					vjsParamObject ( ioParms. GetContext ( ) );
	if ( bOk )
		bOk = ioParms. GetParamObject ( 2, vjsParamObject );

	VString						vstrQuote;
	if ( bOk && vjsParamObject. HasProperty ( "quote" ) )
	{
		VJSValue				vjsQuoteProperty = vjsParamObject. GetProperty ( CVSTR ( "quote" ) ); // "quote" is optional
		if ( vjsQuoteProperty. IsString ( ) )
			vjsParamObject. GetPropertyAsString ( CVSTR ( "quote" ), NULL, vstrQuote );
		else if ( !vjsQuoteProperty. IsNull ( ) && !vjsQuoteProperty. IsUndefined ( ) )
			vError = vThrowError ( XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "quote" );
	}
	else
		_GetDefaultQuote ( vstrQuote );

	if ( bOk && vjsParamObject. HasProperty ( "parameters" ) )
	{
		VJSObject				vjsParamPropertyObj = vjsParamObject. GetPropertyAsObject ( "parameters" );
		if ( vjsParamPropertyObj. IsArray ( ) ) // To make sure that the property is not null and is not undefined
		{
			VJSArray			arrParameters ( vjsParamPropertyObj );
			vError = _PatternParametersToString ( arrParameters, vctrPattern, vstrQuote, outCommandLine );
			bOk = ( vError == VE_OK );
		}
		else if ( vjsParamPropertyObj. IsObject ( ) )
		{
			vError = _PatternParametersToString ( vjsParamPropertyObj, vctrPattern, vstrQuote, outCommandLine );
			bOk = ( vError == VE_OK );
		}
		else
		{
			VJSArray				arrFakeParameters ( ioParms. GetContext ( ) );
			vError = _PatternParametersToString ( arrFakeParameters, vctrPattern, vstrQuote, outCommandLine );
			bOk = ( vError == VE_OK );
		}
	}
	else
	{
		// There are no named parameters to replace. Add command arguments as is.
		outCommandLine. insert ( outCommandLine. begin ( ), vctrPattern. begin ( ), vctrPattern. end ( ) );
	}

	if ( bOk )
	{
		if ( bWithArguments )
			outCommandLine. insert ( outCommandLine. begin ( ), vstrFullPath );
		else
		{
			// Reconstruct one execution string
			if ( outCommandLine. size ( ) > 0 )
			{
				vstrFullPath. AppendCString ( " " );
				vstrFullPath. AppendString ( outCommandLine [ 0 ] );
				outCommandLine. clear ( );
			}
			outCommandLine. push_back( vstrFullPath );
		}
	}

	/*if ( bOk )
	{
#if VERSIONWIN
		outCommandLine += ( "\"" + vstrFullPath + "\" " );
#else
		outCommandLine += ( "bash -c \"'" + vstrFullPath + "' " );
#endif

		for ( size_t i = 0; i < vctrCommand. size ( ); i++ )
			outCommandLine += " " + vctrCommand [ i ];

#if !VERSIONWIN
		outCommandLine += " 2>&1\"";
#endif
	}*/

	if ( bOk)
	{
		VJSValue				vjsFolderProperty = vjsParamObject. GetProperty ( CVSTR ( "folder" ) );
		VFolder*				vfldrExecutionRoot = vjsFolderProperty. GetFolder ( );
		if (vfldrExecutionRoot != NULL)
		{
			vfldrExecutionRoot-> GetPath ( outRootFolderPath, FPS_SYSTEM );
		}
		else if ( vjsFolderProperty. IsString ( ) )
		{
			VString				vstrFolderPath;
			bOk = vjsFolderProperty. GetString ( vstrFolderPath );
			if ( bOk )
			{
				vfldrExecutionRoot = VJSPath::ResolveFolderParam( ioParms. GetContext ( ), vstrFolderPath );
				if ( vfldrExecutionRoot != NULL )
					vfldrExecutionRoot-> GetPath ( outRootFolderPath, FPS_SYSTEM );

				ReleaseRefCountable ( &vfldrExecutionRoot );
			}
		}
		else if (!vjsFolderProperty.IsUndefined() && !vjsFolderProperty.IsNull())
		{
			bOk = false;
			vError = XBOX::vThrowError ( XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_FOLDER, "folder" );
		}
	}

	if ( bOk && vjsParamObject. HasProperty ( "stdin" ) )
	{
		VJSValue				vjsStdInProperty = vjsParamObject. GetProperty ( CVSTR ( "stdin" ) ); // "stdin" is optional
		if ( vjsStdInProperty. IsString ( ) )
			vjsStdInProperty. GetString ( outStdInContent );
		else if ( !vjsStdInProperty. IsNull ( ) && !vjsStdInProperty. IsUndefined ( ) )
			vError = vThrowError ( XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "stdin" );
	}

	if ( bOk && vjsParamObject. HasProperty ( "variables" ) )
	{
		VJSValue				vjsVariablesProperty = vjsParamObject. GetProperty ( "variables" );
		if ( !vjsVariablesProperty. IsNull ( ) && !vjsVariablesProperty. IsUndefined ( ) )
		{
			if ( vjsVariablesProperty. IsObject ( ) )
			{
				VJSObject				vjsEndVarObject = vjsParamObject. GetPropertyAsObject ( "variables" );
				std::vector<VString>	vctrProperties;
				vjsEndVarObject. GetPropertyNames ( vctrProperties );
				for ( size_t i = 0; i < vctrProperties. size ( ); i++ )
				{
					VString				vstrPropValue;
					bOk = vjsEndVarObject. GetPropertyAsString ( vctrProperties [ i ], NULL, vstrPropValue );
					if ( bOk )
						outEnvironmentVars. push_back ( std::make_pair ( vctrProperties [ i ], vstrPropValue ) );
				}
			}
			else
			{
				bOk = false;
				vError = XBOX::vThrowError ( XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_OBJECT, "variables" );
			}
		}
	}

	if ( bOk && vjsParamObject. HasProperty ( "kill_process_tree" ) )
	{
		VJSValue				vjsKillProcTreeProperty = vjsParamObject. GetProperty ( CVSTR ( "kill_process_tree" ) ); // "kill_process_tree" is optional
		if ( vjsKillProcTreeProperty. IsBoolean ( ) )
		{
			bool				bExists = false;
			outKillProcessTree = vjsParamObject. GetPropertyAsBool ( CVSTR ( "kill_process_tree" ), NULL, &bExists );
			xbox_assert ( bExists );
		}
		else if ( !vjsKillProcTreeProperty. IsNull ( ) && !vjsKillProcTreeProperty. IsUndefined ( ) )
			vError = vThrowError ( XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN, "kill_process_tree" );
	}
	else
		outKillProcessTree = false;


	return vError;
}

VError VJSSystemWorkerClass::_ResolvePath ( XBOX::VString const & inName, XBOX::VJSContext const & inContext, XBOX::VString & outPath )
{
	VError							vError = VE_OK;

	VJSGlobalObject*				globalObject = inContext. GetGlobalObjectPrivateInstance ( );
	IJSRuntimeDelegate*				runtimeDelegate = ( globalObject == NULL ) ? NULL : globalObject-> GetRuntimeDelegate ( );
	if ( runtimeDelegate != NULL )
	{
		VSystemWorkerNamespace*		sysWorkerNamespace = runtimeDelegate-> RetainRuntimeSystemWorkerNamespace ( );
		if ( testAssert ( sysWorkerNamespace != NULL ) )
		{
			VFilePath					vfPathWorker;
			vError = sysWorkerNamespace-> GetPath ( inName, vfPathWorker );
			if ( vError == VE_OK )
			{
#if VERSIONWIN
				vfPathWorker. GetPath ( outPath );
#else
				vfPathWorker. GetPosixPath ( outPath );
#endif
			}
			else
				vError = VE_OK; // FOR NOW ONLY! WORK IN PROGRESS. WILL FALL BACK TO THE OLD CODE BELOW
		}

		ReleaseRefCountable ( &sysWorkerNamespace );
	}

	if ( outPath. GetLength ( ) == 0 )
		vError = VJSSystemWorker::GetPath ( inContext, inName, outPath );

	if ( vError != VE_OK && inName. EqualToString ( CVSTR ( "git" ) ) )
	{
		outPath. FromString ( CVSTR ( "git" ) );
		vError = VE_OK;
	}

	return vError;
}

VError VJSSystemWorkerClass::_VerifyStringIndex ( VIndex inIndex, XBOX::VString const & inString )
{
	if ( inIndex >= inString. GetLength ( ) )
		return vThrowError ( VE_INVALID_PARAMETER );

	return VE_OK;
}

VError VJSSystemWorkerClass::_VerifyUniChar ( UniChar inToVerify, UniChar inExpected )
{
	if ( inToVerify != inExpected )
		return vThrowError ( VE_INVALID_PARAMETER );

	return VE_OK;
}

VError VJSSystemWorkerClass::_VerifyNonNullObject ( VObject const * inObject )
{
	if ( inObject == NULL )
		return vThrowError ( VE_INVALID_PARAMETER );

	return VE_OK;
}

VError VJSSystemWorkerClass::_GetFilePath ( VJSValue const & inFileOrFolder, XBOX::VString & outPath, bool inRemoveTrailingSeparatorForFolders )
{
	VError				vError = VE_OK;

	outPath. Clear ( );
	FilePathStyle		fpStyle = FPS_POSIX;
#if VERSIONWIN
	fpStyle = FPS_SYSTEM;
#endif

	if ( inFileOrFolder. IsInstanceOf ( "File" ) )
	{
		VFile*				vFile = inFileOrFolder. GetFile ( );
		if ( ( vError = _VerifyNonNullObject ( vFile ) ) == VE_OK )
			vFile-> GetPath ( outPath, fpStyle );
	}
	else if ( inFileOrFolder. IsInstanceOf ( "Folder" ) )
	{
		VFolder*				vFolder = inFileOrFolder. GetFolder ( );
		if ( ( vError = _VerifyNonNullObject ( vFolder ) ) == VE_OK )
			vFolder-> GetPath ( outPath, fpStyle );

		if ( inRemoveTrailingSeparatorForFolders && outPath. GetUniChar ( outPath. GetLength ( ) ) == CHAR_REVERSE_SOLIDUS )
			outPath. Remove ( outPath. GetLength ( ), 1 );
	}
	else
		vError = vThrowError ( VE_INVALID_PARAMETER );

	return vError;
}

void VJSSystemWorkerClass::_AppendFilePath ( XBOX::VString & inDestination, XBOX::VString const & inPath, XBOX::VString const & inQuote )
{
#if VERSIONWIN
	inDestination += ( inQuote + inPath + inQuote );
#else
	inDestination += ( inQuote + inPath + inQuote );
#endif
}

void VJSSystemWorkerClass::_GetDefaultQuote ( XBOX::VString & outQuote )
{
#if VERSIONWIN
	outQuote += "\"";
#else
	outQuote += "'";
#endif
}

VError VJSSystemWorkerClass::_PatternParametersToString ( XBOX::VJSArray const & inParameters, std::vector< XBOX::VString > const & inPattern, XBOX::VString const & inQuote, std::vector< VString > & outCommand )
{
	VError						vError = VE_OK;

	VString						vstrResult;
	size_t						nParameterIndex = 0;

	for ( size_t i = 0; i < inPattern. size ( ) && vError == VE_OK; i++ )
	{
		vstrResult. Clear ( );

		VString					vstrPattern = inPattern [ i ];
		VIndex					nCurrentIndex = 0;
		UniChar					nCurrentChar;
		VJSException			jsException;
		while ( nCurrentIndex < vstrPattern. GetLength ( ) )
		{
			if ( vstrPattern [ nCurrentIndex ] != CHAR_LEFT_CURLY_BRACKET )
			{
				vstrResult. AppendUniChar ( vstrPattern [ nCurrentIndex ] );
				nCurrentIndex++;

				continue;
			}

			nCurrentIndex++;
			nCurrentChar = vstrPattern [ nCurrentIndex ];
			if ( ( vError = _VerifyStringIndex ( nCurrentIndex, vstrPattern ) ) != VE_OK )
				break;

			if ( nCurrentChar == CHAR_LEFT_CURLY_BRACKET )
			{
				// A double left curly bracket - an escape sequence
				vstrResult. AppendUniChar ( CHAR_LEFT_CURLY_BRACKET );
				nCurrentIndex++;

				continue;
			}

			nCurrentIndex++;
			if ( nCurrentChar == CHAR_LATIN_SMALL_LETTER_S )
			{
				if ( ( vError = _VerifyStringIndex ( nCurrentIndex, vstrPattern ) ) != VE_OK )
					break;

				nCurrentChar = vstrPattern [ nCurrentIndex ];
				if ( ( vError = _VerifyUniChar ( nCurrentChar, CHAR_RIGHT_CURLY_BRACKET ) ) != VE_OK )
					break;

				// So this is the {s} pattern parameter
				nCurrentIndex++;

				if ( nParameterIndex >= inParameters. GetLength ( ) )
				{
					vError = vThrowError ( VE_INVALID_PARAMETER );

					break;
				}

				VJSValue			jsValue = inParameters. GetValueAt ( nParameterIndex, &jsException );
#if USE_V8_ENGINE
				if ( !jsException.IsEmpty() || !jsValue. IsString ( ) )
#else
				if ( jsException.GetValue() != NULL || !jsValue. IsString ( ) )
#endif
				{
					vError = vThrowError ( VE_INVALID_PARAMETER );

					break;
				}
				nParameterIndex++;
				VString				strParam;
				jsValue. GetString ( strParam );
				_AppendFilePath ( vstrResult, strParam, inQuote );	// String parameters need to be enclosed in system-specific quotes too
			}
			else if ( nCurrentChar == CHAR_LATIN_SMALL_LETTER_F )
			{
				if ( ( vError = _VerifyStringIndex ( nCurrentIndex, vstrPattern ) ) != VE_OK )
					break;

				nCurrentChar = vstrPattern [ nCurrentIndex ];
				if ( ( vError = _VerifyUniChar ( nCurrentChar, CHAR_RIGHT_CURLY_BRACKET ) ) != VE_OK )
					break;

				// So this is the {f} pattern parameter
				nCurrentIndex++;

				VJSValue			jsValue = inParameters. GetValueAt ( nParameterIndex, &jsException );
#if USE_V8_ENGINE
				if (!jsException.IsEmpty())
#else
				if (jsException.GetValue() != NULL )
#endif
				{
					vError = vThrowError ( VE_INVALID_PARAMETER );

					break;
				}

				nParameterIndex++;

				VString				vstrPath;
				if ( ( vError = _GetFilePath ( jsValue, vstrPath ) ) != VE_OK )
					break;

				_AppendFilePath ( vstrResult, vstrPath, inQuote );
			}
			else if ( nCurrentChar == CHAR_LEFT_SQUARE_BRACKET )
			{
				if ( ( vError = _VerifyStringIndex ( nCurrentIndex, vstrPattern ) ) != VE_OK )
					break;

				nCurrentChar = vstrPattern [ nCurrentIndex ];
				if ( ( vError = _VerifyUniChar ( nCurrentChar, CHAR_LATIN_SMALL_LETTER_F ) ) != VE_OK )
					break;

				nCurrentIndex++;
				nCurrentChar = vstrPattern [ nCurrentIndex ];
				if ( ( vError = _VerifyUniChar ( nCurrentChar, CHAR_RIGHT_SQUARE_BRACKET ) ) != VE_OK )
					break;

				nCurrentIndex++;
				nCurrentChar = vstrPattern [ nCurrentIndex ];
				if ( ( vError = _VerifyUniChar ( nCurrentChar, CHAR_RIGHT_CURLY_BRACKET ) ) != VE_OK )
					break;

				// So this is the {[f]} pattern parameter
				nCurrentIndex++;

				VJSValue			jsValue = inParameters. GetValueAt ( nParameterIndex, &jsException );
#if USE_V8_ENGINE
				if (!jsException.IsEmpty() || jsValue.IsNull() || jsValue.IsUndefined() || !jsValue.IsArray())
#else
				if (jsException.GetValue() != NULL || jsValue.IsNull() || jsValue.IsUndefined() || !jsValue.IsArray())
#endif
				{
					vError = vThrowError ( VE_INVALID_PARAMETER );

					break;
				}
				nParameterIndex++;

				VJSArray			jsarrFF ( jsValue );
				for ( size_t i = 0; i < jsarrFF. GetLength ( ); i++ )
				{
					VJSValue		jsArrayValue = jsarrFF. GetValueAt ( i, &jsException );
#if USE_V8_ENGINE
					if (!jsException.IsEmpty())
#else
					if (jsException.GetValue() != NULL)
#endif
					{
						vError = vThrowError ( VE_INVALID_PARAMETER );

						break;
					}

					VString			vstrPath;
					if ( ( vError = _GetFilePath ( jsArrayValue, vstrPath ) ) != VE_OK )
						break;

					if ( i > 0 )
						vstrResult += " ";

					_AppendFilePath ( vstrResult, vstrPath, inQuote );
				}

				if ( vError != VE_OK )
					break;
			}
			else if ( nCurrentChar == CHAR_LATIN_SMALL_LETTER_R || nCurrentChar == CHAR_LATIN_CAPITAL_LETTER_R )
			{
				if ( ( vError = _VerifyStringIndex ( nCurrentIndex, vstrPattern ) ) != VE_OK )
					break;

				bool			bIsCapitalR = ( nCurrentChar == CHAR_LATIN_CAPITAL_LETTER_R );
				nCurrentChar = vstrPattern [ nCurrentIndex ];
				if ( ( vError = _VerifyUniChar ( nCurrentChar, CHAR_RIGHT_CURLY_BRACKET ) ) != VE_OK )
					break;

				// So this is the {r} or {R} pattern parameter
				nCurrentIndex++;

				VString				vstrSolutionParentPath = VJSSystemWorker::sSolutionParentFullPath;
				if ( bIsCapitalR )
					vstrSolutionParentPath. AppendString ( CVSTR ( ".git" ) );

				_AppendFilePath ( vstrResult, vstrSolutionParentPath, inQuote );
			}
			else
			{
				// Unknown pattern
				vError = vThrowError ( VE_INVALID_PARAMETER );

				break;
			}
		}

		if ( vError == VE_OK )
			outCommand. push_back ( vstrResult );
	}

	return vError;
}

VError VJSSystemWorkerClass::_PatternParametersToString ( XBOX::VJSObject const & inParameters, std::vector< XBOX::VString > const & inPattern, XBOX::VString const & inQuote, std::vector< VString > & outCommand )
{
	VError									vError = VE_OK;
	bool									bOk = true;

	// Loop through the list of all attributes and replace each reference to {attributeN} with 'valueN' using the optional quote character.
	std::vector<XBOX::VString>				vctrAttributes;
	inParameters. GetPropertyNames ( vctrAttributes );

	VString									vstrGitWorkTree_r = ( inQuote + VJSSystemWorker::sSolutionParentFullPath + inQuote );
	VString									vstrGitDir_R = ( inQuote + VJSSystemWorker::sSolutionParentFullPath + inQuote );
	vstrGitDir_R. AppendString ( CVSTR ( ".git" ) );

	for ( size_t i = 0; bOk && vError == VE_OK && i < inPattern. size ( ); i++ )
	{
		bool								bAlreadyPushed = false;
		VString								vstrPattern = inPattern [ i ];
		vstrPattern. ExchangeAll ( CVSTR ( "{r}" ), vstrGitWorkTree_r, true );
		vstrPattern. ExchangeAll ( CVSTR ( "{R}" ), vstrGitDir_R, true );

		for ( size_t j = 0; bOk && vError == VE_OK && j < vctrAttributes. size ( ); j++ )
		{
			VString							vstrAttribute = vctrAttributes [ j ];
			VJSValue						vjsValue = inParameters. GetProperty ( vstrAttribute );
			vstrAttribute = "{" + vstrAttribute + "}";
			if ( vstrPattern. Find ( vstrAttribute ) == 0 )
				continue;

			if ( vjsValue. IsString ( ) )
			{
				VString						vstrValue;
				bOk = vjsValue. GetString ( vstrValue );
				if ( bOk )
				{
					vstrValue = inQuote + vstrValue + inQuote;
					vstrPattern. ExchangeAll ( vstrAttribute, vstrValue );
				}
			}
			else if ( vjsValue. IsInstanceOf ( "File" ) )
			{
				VFile*						vFile = vjsValue. GetFile ( );
				if ( ( vError = _VerifyNonNullObject ( vFile ) ) == VE_OK )
				{
					VString					vstrPath;
#if VERSIONWIN
					vFile-> GetPath ( vstrPath, FPS_SYSTEM );
#else
					vFile-> GetPath ( vstrPath, FPS_POSIX );
#endif
					vstrPath = inQuote + vstrPath + inQuote;
					vstrPattern. ExchangeAll ( vstrAttribute, vstrPath );

// static	VFile*		ResolveFileParam( const VJSContext& inContext, const VString& inJSPathParam, EFSNOptions inOptions = eFSN_Default);
// static	VFolder*	ResolveFolderParam( const VJSContext& inContext, const VString& inJSPathParam);

				}
			}
			else if ( vjsValue. IsInstanceOf ( "Folder" ) )
			{
				VFolder*					vFolder = vjsValue. GetFolder ( );
				if ( ( vError = _VerifyNonNullObject ( vFolder ) ) == VE_OK )
				{
					VString					vstrPath;
#if VERSIONWIN
					vFolder-> GetPath ( vstrPath, FPS_SYSTEM );
#else
					vFolder-> GetPath ( vstrPath, FPS_POSIX );
#endif
					vstrPath = inQuote + vstrPath + inQuote;
					vstrPattern. ExchangeAll ( vstrAttribute, vstrPath );
				}
			}
			else if ( vjsValue. IsArray ( ) )
			{
				// Can be an array of strings, of files or folders
				// Each element of an array of files or folders needs to be converted into a path string and added to the "outCommand" as a separate element
				VJSException		jsException;
				VJSArray			jsarrFF ( vjsValue );
				for ( size_t k = 0; k < jsarrFF. GetLength ( ); k++ )
				{
					VJSValue		jsArrayValue = jsarrFF. GetValueAt ( k, &jsException );
#if USE_V8_ENGINE
					if (!jsException.IsEmpty())
#else
					if (jsException.GetValue() != NULL)
#endif
					{
						vError = vThrowError ( VE_INVALID_PARAMETER );

						break;
					}

					VString			vstrValue;
					if ( jsArrayValue. IsString ( ) )
						bOk = jsArrayValue. GetString ( vstrValue );
					else
					{
						if ( ( vError = _GetFilePath ( jsArrayValue, vstrValue ) ) != VE_OK )
							break;
					}
					vstrValue = inQuote + vstrValue + inQuote;

					if ( bOk )
					{
						outCommand. push_back ( vstrValue );
						bAlreadyPushed = true;
					}
				}
			}
			else
			{
				// Ignore ???
				xbox_assert ( false );
			}
		}

		if ( bOk && !bAlreadyPushed )
			outCommand. push_back ( vstrPattern );
	}

	return vError;
}

void VJSSystemWorkerClass::_Finalize (const XBOX::VJSParms_finalize &inParms, VJSSystemWorker *inSystemWorker)
{
	xbox_assert(inSystemWorker != NULL);

	// Garbage collection of SystemWorker object. If not terminated already, force quit.

#if TRACE_SYSTEM_WORKER
	
	XBOX::DebugMsg("Finalize called for SysWorker %p.\n", inSystemWorker);
	
#endif
	
	inSystemWorker->Kill(true);

	inSystemWorker->Release();
}

void VJSSystemWorkerClass::_postMessage (XBOX::VJSParms_callStaticFunction &ioParms, VJSSystemWorker *inSystemWorker)
{
	xbox_assert(inSystemWorker != NULL);

	ioParms.ReturnNumber(0);

	if (!inSystemWorker->IsRunning(true))

		return;

	if (!ioParms.CountParams())
		
		return;
	
	XBOX::VString	string;

	bool			isBuffer	= false;
	VJSBufferObject	*bufferObject;
		
	if (ioParms.IsStringParam(1)) {
		
		if (!ioParms.GetStringParam(1, string)) 
		
			return;		
				
	} else if (ioParms.IsObjectParam(1)) {
		
		XBOX::VJSObject	object(ioParms.GetContext());
		
		if (!ioParms.GetParamObject(1, object)) {
			
			XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
			return;
			
		}

		if (object.IsInstanceOf("BinaryStream")) {
			
			XBOX::VStream	*stream;		
		 
			stream = object.GetPrivateData<VJSStream>();
			stream->GetText(string);

		} else if (object.IsOfClass(VJSBufferClass::Class())) {
	
			isBuffer = true;

			bufferObject = object.GetPrivateData<VJSBufferClass>();
			xbox_assert(bufferObject != NULL);

		}
		
	}

#define SLICE_SIZE	1024
#define BUFFER_SIZE	(2 * SLICE_SIZE + 1)

	VSize	totalWritten = 0;

	if (!isBuffer) {
	
		sLONG	stringIndex, stringLength;

		stringIndex = 1;
		stringLength = string.GetLength();
			
		while (stringLength) {
		
			XBOX::VString	subString;
			char			buffer[BUFFER_SIZE];
			sLONG			size;

			size = stringLength > SLICE_SIZE ? SLICE_SIZE : stringLength;
			string.GetSubString(stringIndex, size, subString);
			stringIndex += size;
			stringLength -= size;
			
			if ((size = (sLONG) subString.ToBlock(buffer, BUFFER_SIZE, XBOX::VTC_StdLib_char, true, false)) > 0) {
		
				inSystemWorker->fCriticalSection.Lock();
				if ((size = inSystemWorker->fProcessLauncher.WriteToChild(buffer, size)) < 0) {
				
					totalWritten = 0;
					break;
				
				} else 
					
					totalWritten += size;

				inSystemWorker->fCriticalSection.Unlock();
				
			}
			
		}

	} else {

		VSize	totalSize, size;
		uBYTE	*p;
		
		totalSize = bufferObject->GetDataSize();
		p = (uBYTE *) bufferObject->GetDataPtr();
		while (totalWritten < totalSize) {

			size = totalSize - totalWritten > SLICE_SIZE ? SLICE_SIZE : totalSize - totalWritten;

			inSystemWorker->fCriticalSection.Lock();
			if (inSystemWorker->fProcessLauncher.WriteToChild(p, (uLONG) size) < 0) {
			
				totalWritten = 0;
				break;
				
			} else {
					
				totalWritten += size;
				p += size;

			}
			inSystemWorker->fCriticalSection.Unlock();

		}

	}
	ioParms.ReturnNumber(totalWritten);
}

void VJSSystemWorkerClass::_endOfInput (XBOX::VJSParms_callStaticFunction &ioParms, VJSSystemWorker *inSystemWorker)
{
	xbox_assert(inSystemWorker != NULL);

	if (inSystemWorker->IsRunning(true)) {

		XBOX::StLocker<XBOX::VCriticalSection>	lock(&inSystemWorker->fCriticalSection);

		inSystemWorker->fProcessLauncher.CloseStandardInput();

	}
}

void VJSSystemWorkerClass::_terminate (XBOX::VJSParms_callStaticFunction &ioParms, VJSSystemWorker *inSystemWorker)
{
	xbox_assert(inSystemWorker != NULL);

	bool	doWait, killProcessTree;

	if (!ioParms.CountParams() || !ioParms.IsBooleanParam(1) || !ioParms.GetBoolParam(1, &doWait))
		
		doWait = false;	

	if (ioParms.CountParams() < 2 || !ioParms.IsBooleanParam(2) || !ioParms.GetBoolParam(2, &killProcessTree))

		killProcessTree = false;

	if (!inSystemWorker->IsRunning(true)) {

		// System worker may have already terminated, but the process tree hasn't been killed.

		if (killProcessTree)

			inSystemWorker->fProcessLauncher.Shutdown(false, true);

		return;

	}

	// Force close stdin. This way, if the external process was blocked waiting data on stdin, it will quit.
		
	inSystemWorker->fTerminationRequested = true;
	if (killProcessTree)

		inSystemWorker->fKillProcessTree = true;

	inSystemWorker->fProcessLauncher.CloseStandardInput();

	if (doWait && !inSystemWorker->fWorker->IsInsideWaitFor()) {

		XBOX::VJSContext	context(ioParms.GetContext());
		
		inSystemWorker->fWorker->WaitFor(context, -1, inSystemWorker, VJSSystemWorkerEvent::eTYPE_TERMINATION);

	}
}

void VJSSystemWorkerClass::_getInfos (XBOX::VJSParms_callStaticFunction &ioParms, VJSSystemWorker *inSystemWorker)
{
	xbox_assert(inSystemWorker != NULL);

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&inSystemWorker->fCriticalSection);
	XBOX::VJSObject							object(ioParms.GetContext());
	
	object.MakeEmpty();	
	object.SetProperty("commandLine", inSystemWorker->fCommandLine);	
	object.SetProperty("hasStarted", inSystemWorker->fStartupStatus > 0);
	object.SetProperty("isTerminated", inSystemWorker->fIsTerminated);
	object.SetProperty("pid", inSystemWorker->fPID);	
	
	ioParms.ReturnValue(object);
}

void VJSSystemWorkerClass::_getNumberRunning (XBOX::VJSParms_callStaticFunction &ioParms, VJSSystemWorker *inSystemWorker)
{
	xbox_assert(inSystemWorker != NULL);

	ioParms.ReturnNumber(XBOX::VInterlocked::AtomicGet((sLONG *) &VJSSystemWorker::sNumberRunning));
}

void VJSSystemWorkerClass::_setBinary (XBOX::VJSParms_callStaticFunction &ioParms, VJSSystemWorker *inSystemWorker)
{
	xbox_assert(inSystemWorker != NULL);

	bool	isBinary	= true;

	if (ioParms.CountParams()) 

		ioParms.GetBoolParam(1, &isBinary);

	ioParms.GetThis().SetProperty("isbinary", isBinary);
}

void VJSSystemWorkerClass::_wait (XBOX::VJSParms_callStaticFunction &ioParms, VJSSystemWorker *inSystemWorker)
{
	xbox_assert(inSystemWorker != NULL);

	sLONG	waitDuration;
	bool	isTerminated;

	// Check if the termination process has already been processed or discarded. If so, do not wait trying to match this event.

	waitDuration = VJSWorker::GetWaitDuration(ioParms);
	{
		XBOX::StLocker<XBOX::VCriticalSection>	lock(&inSystemWorker->fCriticalSection);

		isTerminated = inSystemWorker->fTerminationEventDiscarded;

	}
	if (!isTerminated) {

		XBOX::VJSContext	context(ioParms.GetContext());
		VJSWorker			*worker;

		worker = VJSWorker::RetainWorker(context);
		isTerminated = (worker->WaitFor(context, waitDuration, inSystemWorker, VJSSystemWorkerEvent::eTYPE_TERMINATION) == -1);
		XBOX::ReleaseRefCountable<VJSWorker>(&worker);		

	}

	ioParms.ReturnBool(isTerminated);
}

void VJSSystemWorkerClass::_Exec (XBOX::VJSParms_callStaticFunction &ioParms, void *)
{	
	XBOX::VString				commandLine, executionPath, inputString;
	std::vector<XBOX::VString>	arguments;
	XBOX::VJSBufferObject		*inputBuffer;
	bool						bQuoteArgumentsIfNeeded = true;
	bool						isOk = true;
	bool						killProcessTree = false;

	std::vector< std::pair<VString, VString> >			vctrEnvironmentVars;

	ioParms.ReturnNullValue();
	inputBuffer = NULL;

#if WITH_SANDBOXED_SYSTEM_WORKERS
	bool						bIsNewSignature = false;
	if ( ioParms. CountParams ( ) == 2 && ( ioParms. IsStringParam ( 1 ) || ioParms. IsArrayParam ( 1 ) ) && ioParms. IsObjectParam ( 2 ) )
	{
		VFolder*				vf = ioParms. RetainFolderParam ( 2, false );
		bIsNewSignature = ( vf == NULL );
		ReleaseRefCountable ( &vf );
	}

	bIsNewSignature = bIsNewSignature || ( ioParms. CountParams ( ) >= 5 && !ioParms.IsBooleanParam(5));

	if ( bIsNewSignature ) // The new sand-boxed workers
	{
		VError					vError = VE_OK;
		VString					vstrStdInContent;
		if ( ioParms. CountParams ( ) >= 5 )
			vError = _ReadParametersFS ( ioParms, commandLine, executionPath, vstrStdInContent );
		else
		{
			vError = _ReadObjectParametersFS ( ioParms, arguments, executionPath, vstrStdInContent, vctrEnvironmentVars, killProcessTree );
			if ( vError == VE_OK && arguments. size ( ) > 0 )
			{
				commandLine = arguments [ 0 ];
				arguments. erase ( arguments. begin ( ) );
				bQuoteArgumentsIfNeeded = false;
			}
		}

		isOk = ( vError == VE_OK );
	}
	else
#endif
{
	if (!ioParms.IsStringParam(1) || !ioParms.GetStringParam(1, commandLine)) {

		XBOX::VJSArray	argumentArray(ioParms.GetContext());

		isOk = false;
		if (ioParms.CountParams() > 0 && ioParms.IsArrayParam(1) && ioParms.GetParamArray(1, argumentArray)) {

			isOk = _ExtractCommandArray(argumentArray, &commandLine, &arguments);
			if (!isOk)

				XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_ARRAY, "1");

		} else 
		
			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");

		if (!isOk)
		
			return;
				
	} 
	
	if (ioParms.CountParams() >= 2) {

		if (ioParms.IsStringParam(2)) {

			if (!ioParms.GetStringParam(2, inputString)) {

				XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2");
				return;

			}

		} else if (ioParms.IsObjectParam(2)) {

			XBOX::VJSObject	bufferObject(ioParms.GetContext());

			if (!ioParms.GetParamObject(2, bufferObject) || !bufferObject.IsOfClass(VJSBufferClass::Class())) {

				XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BUFFER, "2");
				return;

			}

			inputBuffer = XBOX::RetainRefCountable<VJSBufferObject>(bufferObject.GetPrivateData<VJSBufferClass>());
			xbox_assert(inputBuffer != NULL);

		} else if (ioParms.IsNullOrUndefinedParam(2)) {

			// null is acceptable, like an empty string: There is no input.

		} else {
			
			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER, "2");
			return;

		}

	} 

	if (!ioParms.IsBooleanParam(3) && !_RetrieveFolder(ioParms, 3, &executionPath)) {
		
		ReleaseRefCountable<VJSBufferObject>(&inputBuffer);
		return;

	}
}

	// Get optional environment variables.

	XBOX::VJSObject	variables(ioParms.GetContext());

	variables.ClearRef();
	if (ioParms.CountParams() >= 5 && !ioParms.IsBooleanParam(5)) {

		// Sand-boxed workers.

		if (!ioParms.IsNullOrUndefinedParam( 6) && !ioParms.GetParamObject(6, variables)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_OBJECT, "6");
			ReleaseRefCountable<VJSBufferObject>(&inputBuffer);
			return;

		}
			
		if (ioParms.CountParams() >= 7 && ioParms.IsBooleanParam(7)) 
	
			ioParms.GetBoolParam(7, &killProcessTree);

	} else {

		if (ioParms.IsBooleanParam(4)) {

			ioParms.GetBoolParam(4, &killProcessTree);

		} else {
			
			if (!ioParms.IsNullOrUndefinedParam(4) && !ioParms.GetParamObject(4, variables)) {

				XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_OBJECT, "4");
				ReleaseRefCountable<VJSBufferObject>(&inputBuffer);
				return;

			}

			if (ioParms.IsBooleanParam(5))

				ioParms.GetBoolParam(5, &killProcessTree);

		}

	}
			
	sLONG	*panicTerminationFlag = new sLONG;

	if (panicTerminationFlag == NULL) {

		ReleaseRefCountable<VJSBufferObject>(&inputBuffer);
		return;

	} else

		*panicTerminationFlag = false;

	VJSSystemWorker::sMutex.Lock();
	VJSSystemWorker::sRunningCommandLines.insert(panicTerminationFlag);
	VJSSystemWorker::sMutex.Unlock();

#if VERSIONWIN
#	define OPTION	(VProcessLauncher::eVPLOption_HideConsole | VProcessLauncher::eVPLOption_NonBlocking)
#else
#	define OPTION	(VProcessLauncher::eVPLOption_None | VProcessLauncher::eVPLOption_NonBlocking)
#endif
		
	VMemoryBuffer<>	stdinBuffer;
	XBOX::VString	*defaultDirectory;
	VMemoryBuffer<>	*stdinBufferPointer;
	
	defaultDirectory = executionPath.GetLength() ? &executionPath : NULL;
	if (inputString.GetLength()) {

		sLONG	stringIndex, stringLength;

		stringIndex = 1;
		stringLength = inputString.GetLength();
			
		while (stringLength) {
		
			XBOX::VString	subString;
			char			buffer[kBufferSize];
			sLONG			size;

			size = stringLength > kBufferSize ? kBufferSize : stringLength;
			inputString.GetSubString(stringIndex, size, subString);
			stringLength -= size;
			
			if ((size = (sLONG)subString.ToBlock(buffer, kBufferSize, XBOX::VTC_StdLib_char, true, false)) > 0)
		
				stdinBuffer.PutData(stringIndex - 1, buffer, size);
			
			stringIndex += size;
		}

		stdinBufferPointer = &stdinBuffer;

	} else if (inputBuffer != NULL) {

		stdinBuffer.SetDataPtr(inputBuffer->GetDataPtr(), inputBuffer->GetDataSize(), inputBuffer->GetDataSize());
		stdinBufferPointer = &stdinBuffer;

	} else

		stdinBufferPointer = NULL;

	EnvVarNamesAndValuesMap	envVarMap;

	if (variables.HasRef()) {

		XBOX::VJSPropertyIterator	it(variables);

		for (; it.IsValid(); ++it) {

			XBOX::VJSValue	jsValue(it.GetProperty());
			XBOX::VString	name, value;

			if (jsValue.IsString()) {

				it.GetPropertyName(name);
				if (jsValue.GetString(value))

					envVarMap[name] = value;
					
			}
			else if (jsValue.IsNumber()) {

				Real	number;	// JavaScript numbers are double type floats.

				it.GetPropertyName(name);
				if (jsValue.GetReal(&number)) {

					value.FromReal(number);
					envVarMap[name] = value;

				}

			}
			else {

				// Silently ignore if value is not a string or number.

			}

		}

	}

	if ( vctrEnvironmentVars. size ( ) > 0 )
	{
		for ( size_t i = 0; i < vctrEnvironmentVars. size ( ); i++ )
		{
			envVarMap [ vctrEnvironmentVars [ i ]. first ] = vctrEnvironmentVars [ i ]. second;
		}
	}

	
	XBOX::VProcessLauncher		*processLauncher;
		
	isOk = !VProcessLauncher::ExecuteCommandLine(commandLine, OPTION, stdinBufferPointer, NULL, NULL, &envVarMap, defaultDirectory, NULL, &processLauncher, &arguments, bQuoteArgumentsIfNeeded, true);

    XBOX::VMemoryBuffer<>	stdoutBuffer, stderrBuffer;
    sLONG                   stdoutBytesRead, stderrBytesRead;
    char                    buffer[kBufferSize];
    
#if !VERSIONWIN
	
	// XPosixProcessLauncher.cpp will return an error in stderr. Hence in case of error, isOk can still be true. So check error code.
	
	if (isOk) {
        
        static const char	errorMessage[]	= "********** XPosixProcessLauncher::Start/fork() -> child -> Error:";
        static const int	messageLength	= sizeof(errorMessage) - 1;
        bool                isFirstTime     = true;

        for ( ; ; ) {
        
            stderrBytesRead = processLauncher->ReadErrorFromChild(buffer, kBufferSize);
            if (stderrBytesRead > 0) {
        	
                stderrBuffer.PutData(stderrBuffer.GetDataSize(), buffer, stderrBytesRead);
                if (stderrBuffer.GetDataSize() >= messageLength)
                    
                    break;
    
            } else if (!stderrBytesRead) {
                
                if (isFirstTime) {
        
                    // If there is no data on stderr, then sleep 1ms and then try to read again.
                    // This is to make sure a failed fork()/exec() is properly reported (WAK0086434).
                    
                    VTask::Sleep(1);
                    isFirstTime = false;
                    
                }
                
            } else
                
                break;
        
        }
		isOk = stderrBuffer.GetDataSize() < messageLength
                || ::memcmp(stderrBuffer.GetDataPtr(), errorMessage, messageLength);
	
	}

#endif
	
	if (isOk) {

		// Successful start, wait for termination while retrieving stdout and stderr content (if any).

		xbox_assert(processLauncher != NULL);

		bool    isPanicTermination;
		sLONG   exitStatus;
		
		isPanicTermination = false;
		for ( ; ; ) {

			// If terminating all system workers, then force termination.

			if (XBOX::VInterlocked::AtomicGet(panicTerminationFlag)) {

				isPanicTermination = true;
				break;

			}

			// Read stdout and stderr.

			bool	hasProducedData = false;
				
			if ((stdoutBytesRead = processLauncher->ReadFromChild(buffer, kBufferSize)) > 0) {

				stdoutBuffer.PutData(stdoutBuffer.GetDataSize(), buffer, stdoutBytesRead);
				hasProducedData = true;

			}

			if ((stderrBytesRead = processLauncher->ReadErrorFromChild(buffer, kBufferSize)) > 0) {

				stderrBuffer.PutData(stderrBuffer.GetDataSize(), buffer, stderrBytesRead);
				hasProducedData = true;

			}

			// If data has been produced, then continue to read stdout and stderr.
			// Otherwise, check for termination and sleep during polling interval.

			if (hasProducedData)

				continue;

			else if (!processLauncher->IsRunning()) {
				
				exitStatus = processLauncher->GetExitStatus();
				isPanicTermination = false;
				break;

			} else 

				XBOX::VTask::Sleep(VJSSystemWorker::kPollingInterval);

		}
		
		if (!isPanicTermination) {

			// Set up "result" object.	

			XBOX::VJSObject	resultObject(ioParms.GetContext());

			resultObject.MakeEmpty();
			resultObject.SetProperty("exitStatus", (Real) exitStatus);
			resultObject.SetProperty("output", VJSBufferClass::NewInstance(ioParms.GetContext(), stdoutBuffer.GetDataSize(), stdoutBuffer.GetDataPtr()));
			resultObject.SetProperty("error", VJSBufferClass::NewInstance(ioParms.GetContext(), stderrBuffer.GetDataSize(), stderrBuffer.GetDataPtr()));

			// Buffer objects will free memory.

			stdoutBuffer.ForgetData();
			stderrBuffer.ForgetData();

			ioParms.ReturnValue(resultObject);

		}

	}

	processLauncher->Shutdown(true, killProcessTree);
	delete processLauncher;

	stdinBuffer.ForgetData();
	ReleaseRefCountable<VJSBufferObject>(&inputBuffer);	

	VJSSystemWorker::sMutex.Lock();
	VJSSystemWorker::sRunningCommandLines.erase(panicTerminationFlag);
	VJSSystemWorker::sMutex.Unlock();

	delete panicTerminationFlag;
}

bool VJSSystemWorkerClass::_RetrieveFolder (VJSParms_withArguments &inParms, sLONG inIndex, XBOX::VString *outFolderPath)
{
	xbox_assert(inIndex >= 1);
	xbox_assert(outFolderPath != NULL);	

	outFolderPath->Clear();

	bool			isOk;
	XBOX::VString	indexString;

	isOk = true;	
	indexString.FromLong(inIndex);

	if (inParms.IsStringParam(inIndex)) {

		if (!inParms.GetStringParam(inIndex, *outFolderPath)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER, indexString);
			isOk = false;

		} else {

#if VERSIONWIN

			XBOX::VFilePath	path(*outFolderPath, FPS_SYSTEM);

#else

			XBOX::VFilePath	path(*outFolderPath, FPS_POSIX);

#endif

			path.GetPath(*outFolderPath);

		}
			
	} else if (inParms.IsObjectParam(inIndex)) {

		XBOX::VJSObject	folderObject(inParms.GetContext());

		if (!inParms.GetParamObject(inIndex, folderObject) || !folderObject.IsOfClass(VJSFolderIterator::Class()) ) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER, indexString);
			isOk = false;

		} else {

			JS4DFolderIterator	*folderIterator;

			folderIterator = folderObject.GetPrivateData<VJSFolderIterator>();
			xbox_assert(folderIterator != NULL);

			folderIterator->GetFolder()->GetPath(*outFolderPath);

		}			
			
	} else if (!inParms.IsNullOrUndefinedParam(inIndex)) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER, indexString);
		isOk = false;

	}

	return isOk;
}	



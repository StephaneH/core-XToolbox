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
#ifndef __VJS_SYSTEM_WORKER__
#define __VJS_SYSTEM_WORKER__

#include <list>

#include "VJSContext.h"
#include "VJSClass.h"
#include "VJSValue.h"
#include "VJSWorker.h"
#include "VJSEvent.h"

BEGIN_TOOLBOX_NAMESPACE

#define WITH_SANDBOXED_SYSTEM_WORKERS		1
 
class XTOOLBOX_API IJSSystemWorkerDelegate  
{
public:

	// Returns a valid file path that points to an executable on disk that corresponds to the inName label.
	// Matching between labels and full paths is done in the "systemWorkers.json" settings file
	// For example a "git" label lookup should return something like "C:\\Program Files (x86)\\Git\\bin\\git.exe"
	virtual	VError		GetExecutablePath ( VString const & inName, VFilePath& outPath ) = 0;
};


// System worker object.

class XTOOLBOX_API VJSSystemWorker : public XBOX::IJSEventGenerator
{
public:

	enum ESettingsType
	{
		eTYPE_DEFAULT	= 0,	// Application package level
		eTYPE_SOLUTION	= 1		// Solution level
	};

	virtual bool	IsRunning (bool inWaitForStartup);

	// If inPanicTermination is true, then do not generate a termination event. 
	// This is the case when the garbage collector frees the SystemWorker object or when the solution is to be closed.
	// Hence the default behavior when "killing", should be "panic termination".

			void	Kill (bool inPanicTermination);
	virtual void	Kill ()	{	Kill(true);	}

	// Kill all running system workers.

	static void		KillAll ();

	virtual bool	MatchEvent (const XBOX::IJSEvent *inEvent, sLONG inEventType);

	static VError	Init ( VFilePath const & inSettings, enum ESettingsType inType );

	// Looks up worker's binary full path using worker's name
	static VError	GetPath ( const VJSContext& inContext, VString const & inName, VString & outFullPath );

private:

friend class VJSSystemWorkerClass;
friend class VJSSystemWorkerEvent;

	// Events (data available on stdout or stderr, process termination) are polled at regular interval. 
	// Default is 100ms. Use a smaller interval to reduce latency.

	static const sLONG		kPollingInterval			= 100;

	// Buffer size.

	static const sLONG		kBufferSize					= 4096;

	// If a SystemWorker object is to be destroyed, termination is automatically requested. 
	// Wait for a limited delay only, so program will not be stuck. 

	static const sLONG		kFailsafeTerminationDelay	= 15000;

	static XBOX::VCriticalSection		sMutex;
	static std::list<VJSSystemWorker *>	sRunningSystemWorkers;
	static std::set<sLONG *>			sRunningCommandLines;

	static sLONG			sNumberRunning;	

	static VFilePath		sDefaultSettingsPath;
	static VJSONValue*		sjsonDefaultSettings;
	static VFilePath		sSolutionSettingsPath;
	static VJSONValue*		sjsonSolutionSettings;
	static VString			sSolutionParentFullPath;
			
	XBOX::VCriticalSection	fCriticalSection;
	VJSWorker				*fWorker;
	XBOX::VString			fCommandLine;
	XBOX::VProcessLauncher	fProcessLauncher;
	XBOX::VTask				*fVTask;
	XBOX::VJSObject			fThis;						// To be set by VJSSystemWorkerClass after Construct(). 

	sLONG					fStartupStatus;				// +1 successful launch, -1 failed, 0 pending.
	bool					fForcedTermination;			// True if termination has been "forced" (user requested or garbage collect).
	bool					fPanicTermination;			// Garbage collector destroying proxy object!
	bool					fTerminationRequested;	
	bool					fIsTerminated;
	bool					fTerminationEventDiscarded;

	sLONG					fExitStatus;				// Exit status code, valid only after proper launch and not forced termination.
	sLONG					fPID;						// PID valid only if fStartupStatus > 0.

	bool					fKillProcessTree;			// When the system worker terminates, either through _terminate() or "by itself", kill its entire process tree.

	// inFolderPath is the path to execute command into, use empty string "" to execute in Wakanda application folder.

					VJSSystemWorker (const XBOX::VString &inCommandLine, const XBOX::VString &inFolderPath, VJSWorker *inWorker, const VJSObject& inObject);

	virtual			~VJSSystemWorker ();

	// Environment variable(s) must be set before the VJSSystemWorker's task is run.

	void			SetEnvironmentVariable (const XBOX::VString &inName, const XBOX::VString &inValue);

	// Shells are launched using "/bin/sh -c <commandLine>" or "cmd /c <commandLine>" commands.
	// This method allows to set the arguments of the call to "/bin/bash" and "cmd".

	void			AddArgument (const XBOX::VString &inArgument);
	
	// Will poll the external process for data.

	void			_DoRun();

	// Will call _DoRun().

	static sLONG	_RunProc (XBOX::VTask *inVTask);	

	static VError	GetPath ( const VJSContext& inContext, VString const & inName, VJSONValue const * inSettings, VString & outFullPath );
};

// System worker JavaScript proxy object.

class XTOOLBOX_API VJSSystemWorkerClass : public XBOX::VJSClass<VJSSystemWorkerClass, VJSSystemWorker>
{
public:

	static const VSize		kBufferSize	= 1 << 14;

    typedef XBOX::VJSClass<VJSSystemWorkerClass, VJSSystemWorker> inherited;
	
	static void				GetDefinition (ClassDefinition &outDefinition);
	
	static XBOX::VJSObject	MakeConstructor (const XBOX::VJSContext& inContext);
	
	static VError			_GetFilePath ( VJSValue const & inFileOrFolder, XBOX::VString & outPath, bool inRemoveTrailingSeparatorForFolders = true );

private:
friend class VJSSystemWorkerShellClass;

	static JS4D::StaticFunction sConstrFunctions[];

	static bool				_ExtractCommandArray (XBOX::VJSArray &inArray, XBOX::VString *outCommandLine, std::vector<XBOX::VString> *ioArguments);

	static void				_Construct (XBOX::VJSParms_construct &ioParms);
    static void				_Finalize (const XBOX::VJSParms_finalize &inParms, VJSSystemWorker *inSystemWorker);
		
	static void				_postMessage (XBOX::VJSParms_callStaticFunction &ioParms, VJSSystemWorker *inSystemWorker);
	static void				_endOfInput (XBOX::VJSParms_callStaticFunction &ioParms, VJSSystemWorker *inSystemWorker);
	static void				_terminate (XBOX::VJSParms_callStaticFunction &ioParms, VJSSystemWorker *inSystemWorker);
	static void				_getInfos (XBOX::VJSParms_callStaticFunction &ioParms, VJSSystemWorker *inSystemWorker);
	static void				_getNumberRunning (XBOX::VJSParms_callStaticFunction &ioParms, VJSSystemWorker *inSystemWorker);
	static void				_setBinary (XBOX::VJSParms_callStaticFunction &ioParms, VJSSystemWorker *inSystemWorker);
	static void				_wait (XBOX::VJSParms_callStaticFunction &ioParms, VJSSystemWorker *inSystemWorker);

	static void				_Exec (XBOX::VJSParms_callStaticFunction &ioParms, void *);
	
	static bool				_RetrieveFolder (VJSParms_withArguments &inParms, sLONG inIndex, XBOX::VString *outFolderPath);

	static VError			_ReadParametersFS ( XBOX::VJSParms_withArguments& ioParms, XBOX::VString& outCommandLine, XBOX::VString& outRootFolderPath, XBOX::VString& outStdInContent );
	static VError			_ReadObjectParametersFS ( XBOX::VJSParms_withArguments& ioParms, std::vector<XBOX::VString>& outCommandLine, XBOX::VString& outRootFolderPath, XBOX::VString& outStdInContent, std::vector< std::pair<VString, VString> >& outEnvironmentVars, bool& outKillProcessTree );
	static VError			_PatternParametersToString ( XBOX::VJSArray const & inParameters, std::vector< XBOX::VString > const & inPattern, XBOX::VString const & inQuote, std::vector< VString > & outCommand );
	static VError			_PatternParametersToString ( XBOX::VJSObject const & inParameters, std::vector< XBOX::VString > const & inPattern, XBOX::VString const & inQuote, std::vector< VString > & outCommand );
	static void				_GetDefaultQuote ( XBOX::VString & outQuote );
	static void				_AppendFilePath ( XBOX::VString & inDestination, XBOX::VString const & inPath, XBOX::VString const & inQuote );
	static VError			_VerifyStringIndex ( VIndex inIndex, XBOX::VString const & inString );
	static VError			_VerifyUniChar ( UniChar inToVerify, UniChar inExpected );
	static VError			_VerifyNonNullObject ( VObject const * inObject );
	static VError			_ResolvePath ( XBOX::VString const & inName, XBOX::VJSContext const & inContext, XBOX::VString & outPath );
};

END_TOOLBOX_NAMESPACE

#endif

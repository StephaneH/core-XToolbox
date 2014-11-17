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
#ifndef __XPosixProcessLauncher__
#define __XPosixProcessLauncher__

#include "VEnvironmentVariables.h"
#include <signal.h>

BEGIN_TOOLBOX_NAMESPACE

/** @brief	See VProcessLauncher.h for a comment about the routines.
			XMacProcessLauncher is the "Mac delegate" of VProcessLauncher.h and is not supposed to be used alone.
*/
class XTOOLBOX_API XPosixProcessLauncher : public VObject
{
public:
					XPosixProcessLauncher();
		virtual		~XPosixProcessLauncher();

		void		SetDefaultDirectory(const VString &inDirectory);

		void		SetEnvironmentVariable(const VString &inVarName, const VString &inValue);
		void		SetEnvironmentVariables(const EnvVarNamesAndValuesMap &inVarAndValues);
		
		void		SetBinaryPath(const VString &inBinaryPath);

		void		CreateArgumentsFromFullCommandLine(const VString &inFullCommandLine);
		void		AddArgument( const VString& inArgument);
		void		ClearArguments();
		
		void		SetWaitClosingChildProcess(bool isWaitingClosingChildProcess);
		
		void		SetRedirectStandardInput(bool isStandardInputRedirected);
		void		SetRedirectStandardOutput(bool isStandardOutputRedirected);
		void		CloseStandardInput();
		
		sLONG		WriteToChild(const void *inBuffer, uLONG inBufferSizeInBytes, bool inClosePipeAfterWritting = false);
	
		sLONG		WaitForData ();
	
		// Reads are non-blocking.
		
		sLONG		ReadFromChild(char *outBuffer, long inBufferSize);			
		sLONG		ReadErrorFromChild(char *outBuffer, long inBufferSize);
	
		void		SetQuoteArgumentsIfNeeded(bool inQuoteArgumentsIfNeeded);
		
		void		EnableProcessTreeKill ()	{	fUseProcessGroup = true;	}
		sLONG		Start();
		sLONG		Start(const EnvVarNamesAndValuesMap &inVarToUse);

		bool		IsRunning();
		sLONG		GetPid () { return fProcessID; }			// Valid only if IsRunning() returns true.
		uLONG		GetExitStatus () { return fExitStatus; }	// Valid only if terminated.

		sLONG		Shutdown(bool inWithKillIndependantChildProcess = false, bool inKillProcessTree = false);
	
static  bool        KillProcess (sLONG inPid);
	
private:
	
		char						*fCurrentDirectory;
		EnvVarNamesAndValuesMap		fEnvirVariables;
		
		char		*fBinaryPath;
		std::vector<VString>	fArrayArg;
		char**		fArrayArgCStrArray;
	
		bool		fIsRunning;
		bool		fShutdown;
		int			fExitStatus;

		bool		fWaitClosingChildProcess;
		
		int			fProcessID;
		int			fPipeParentToChild[2];
		int			fPipeChildToParent[2];
		int			fPipeChildErrorToParent[2];
		
		enum
		{
					pReadSide = 0,
					pWriteSide = 1
		};
							
		// By default, fRedirectStandardInput = true and fRedirectStandardOutput = true. 
		// On PC, if true	: standard input/output are redirected (only affect Windows platform at the moment) mbucatariu, 30/09/2003
		// On MacOSX		: standard input/output are always redirected.
		bool		fRedirectStandardInput;		
		bool		fRedirectStandardOutput;
		
		bool		fHideWindow;
	
		bool		fQuoteArgumentsIfNeeded;
	
		bool		fUseProcessGroup;
		int			fProcessGroupID;		// Same a fProcessID, but save it so we can zero fProcessID.
	
		void		_AdjustBinaryPathIfNeeded();
		bool		_DoesBinaryExist(char *binaryPath);
		
// It's char** on Mac, char* on Windows
		void		_BuildArrayArguments(char **&outArrayArg);
		void		_BuildArrayEnvironmentVariables(const EnvVarNamesAndValuesMap &inVarToUse, char **&outArrayEnv);

		int			_SetNonBlocking(int fd);
		
		void		_Free2DCharArray(char **&array);

		void		_blocksigchild(int inBlock);

		sLONG		_ReadFromPipe(int inPipe, char *outBuffer, long inBufferSize);
		void		_CloseRWPipe(int *outPipe);
		void		_CloseOnePipe(int *outPipe);
		void		_InvalidateRWPipe(int *outPipe);

		bool		_TerminateIndependantChildProcess(int inSignal, sLONG inTimeOutInMS, bool inKillProcessTree);

	/*	Convert the VString to UTF8-C string
		The buffer must be deallocated with delete [] because it is created with new char[...]
		If inString is "" and inEvenIfStringIsEmpty == true, the routine creates a buffer of one byte (contains a zero)
		WARNING: the returned buffer has a size > inString.GetLength() + 1, because there is room for special conversion
		(for example, œ may become eo). *outFullSizeOfBuffer contains the full size - in bytes - of the buffer.
	*/
		char *		_CreateCString(const VString &inString, bool inEvenIfStringIsEmpty = false, sLONG *outFullSizeOfBuffer = NULL) const;

	// Taken from 4D/fmInfos.c/unix_analyze_commandline, but fills a vectore of VStrings instead of an array of CStrings.
	// If outAllArgs is NULL, just return the count of arguments
		sLONG		_AnalyzeCommandLine(char *str_in, VectorOfVString *outAllArgsPtr = NULL);
	
		static void	_CleanWatchSet ();
		static void	_SignalHandler (int signum, siginfo_t *info, void *p);
};

typedef XPosixProcessLauncher		XProcessLauncherImpl;

END_TOOLBOX_NAMESPACE

#endif

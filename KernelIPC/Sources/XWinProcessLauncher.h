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
#ifndef __XWinProcessLauncher__
#define __XWinProcessLauncher__

#include "VEnvironmentVariables.h"

BEGIN_TOOLBOX_NAMESPACE


/** @brief	See VProcessLauncher.h for a comment about the routines.
			XWinProcessLauncher is the "Windows delegate" of VProcessLauncher.h and is not supposed to be used alone.
*/
class XTOOLBOX_API XWinProcessLauncher : public VObject
{
public:
					XWinProcessLauncher();
		virtual		~XWinProcessLauncher();
		
		void		SetDefaultDirectory(const VString &inDirectory);

		void		SetEnvironmentVariable(const VString &inVarName, const VString &inValue);
		void		SetEnvironmentVariables(const EnvVarNamesAndValuesMap &inVarAndValues);
		
		void		SetBinaryPath(const VString &inBinaryPath);

		void		CreateArgumentsFromFullCommandLine(const VString &inFullCommandLine);
		void		AddArgument( const VString& inArgument);
		void		ClearArguments();
		
inline	void		SetWaitClosingChildProcess(bool isWaitingClosingChildProcess)	{ fWaitClosingChildProcess = isWaitingClosingChildProcess; }
		
inline	void		SetRedirectStandardInput(bool isStandardInputRedirected)		{ fRedirectStandardInput = isStandardInputRedirected; }
inline	void		SetRedirectStandardOutput(bool isStandardOutputRedirected)		{ fRedirectStandardOutput = isStandardOutputRedirected; }
		void		CloseStandardInput();
				
		sLONG		WriteToChild(const void *inBuffer, uLONG inBufferSizeInBytes, bool inClosePipeAfterWritting = false);

		sLONG		WaitForData ()	{ return 0;	/* TODO */ }
	
		sLONG		ReadFromChild(char *outBuffer, long inBufferSize);
		sLONG		ReadErrorFromChild(char *outBuffer, long inBufferSize);
		
inline	void		DontDisplayWindow()		{ fHideWindow = true; }
inline	void		CanGetExitStatus()		{ fCanGetExitStatus = true; }

		void		SetQuoteArgumentsIfNeeded(bool inQuoteArgumentsIfNeeded)		{ fQuoteArgumentsIfNeeded = inQuoteArgumentsIfNeeded; }
		
		void		EnableProcessTreeKill ()	{	fUseJobObject = true;	}

		sLONG		Start();
		sLONG		Start(const EnvVarNamesAndValuesMap &inVarToUse);

		bool		IsRunning();
		uLONG		GetExitStatus ()	{ return (uLONG) fExitStatus; }	// Valid only if process has properly terminated.

		sLONG		Shutdown(bool inWithKillIndependantChildProcess = false, bool inKillProcessTree = false);

		sLONG		GetPid ()	{	return fProcessID;	}

static	bool		KillProcess (sLONG inPid);

private:
		wchar_t					*fCurrentDirectory;
		EnvVarNamesAndValuesMap	fEnvirVariables;
		
		wchar_t					*fBinaryPath;
		std::vector<VString>	fArrayArg;
		
		bool					fIsRunning;	// if true, then pipes and fProcessID are valid
		bool					fShutdown;

		bool					fWaitClosingChildProcess;
		
		int						fProcessID;
		PROCESS_INFORMATION		fProcInfo;
	
		DWORD					fExitStatus;

		HANDLE					fChildStdInRead, fChildStdInWrite, fChildStdInWriteDup;
		HANDLE					fChildStdOutRead, fChildStdOutWrite, fChildStdOutReadDup;
		HANDLE					fChildStdErrRead, fChildStdErrWrite, fChildStdErrReadDup;
							
		// By default, fRedirectStandardInput = true and fRedirectStandardOutput = true. 
		// If true	: standard input/output are redirected (only affect Windows platform at the moment) mbucatariu, 30/09/2003
		// On MacOSX : standard input/output are always redirected.

		bool					fRedirectStandardInput;		
		bool					fRedirectStandardOutput;
		
		bool					fHideWindow;
		bool					fCanGetExitStatus;

		bool					fQuoteArgumentsIfNeeded;

//**

		bool					fUseJobObject;
		HANDLE					fJobObject;

//**

		wchar_t *				_CreateConcatenetedArguments();
		wchar_t *				_CreateEnvironmentVariablesBlock(const EnvVarNamesAndValuesMap &inVarToUse);

	/*	Convert the VString to Windows UniCode (UTF-16) C-string.

		Previous version of this function used to convert VString to Win-ANSI, but this may fail for international 
		(japanese for example, ACI0065675).

		The buffer must be deallocated with delete [] because it is created with new char[...]
		If inString is "" and inEvenIfStringIsEmpty == true, the routine creates a buffer of one byte (contains a zero)
		WARNING: the returned buffer has a size > inString.GetLength() + 1, because there is room for special conversion
		(for example, œ may become eo). *outFullSizeOfBuffer contains the full size - in bytes - of the buffer.
	*/
		wchar_t *				_CreateCString(const VString &inString, bool inEvenIfStringIsEmpty = false, sLONG *outFullSizeOfBuffer = NULL) const;

		//	Utility. Just avoid writting 2-3 lines in the code

		void					_CloseAndInvalidateHandle(HANDLE *ioHandle);
		void					_CloseAllPipes();
};	

typedef XWinProcessLauncher		XProcessLauncherImpl;

END_TOOLBOX_NAMESPACE

#endif
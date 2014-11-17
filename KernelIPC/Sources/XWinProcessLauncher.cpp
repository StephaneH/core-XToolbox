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
#include "VKernelIPCPrecompiled.h"
#include "XWinProcessLauncher.h"

extern VMemory *gMemory;

XWinProcessLauncher::XWinProcessLauncher()
{	
	fBinaryPath = NULL;
	fCurrentDirectory = NULL;
		
	fIsRunning = false;
	fShutdown = false;

	fWaitClosingChildProcess = true;

	fProcessID = 0;
	ZeroMemory(&fProcInfo, sizeof(fProcInfo));

	fChildStdInRead = INVALID_HANDLE_VALUE;
	fChildStdInWrite = INVALID_HANDLE_VALUE; 
	fChildStdInWriteDup = INVALID_HANDLE_VALUE;
	
	fChildStdOutRead = INVALID_HANDLE_VALUE;
	fChildStdOutWrite = INVALID_HANDLE_VALUE;
	fChildStdOutReadDup = INVALID_HANDLE_VALUE;
	
	fChildStdErrRead = INVALID_HANDLE_VALUE;
	fChildStdErrWrite = INVALID_HANDLE_VALUE;
	fChildStdErrReadDup = INVALID_HANDLE_VALUE;

	fRedirectStandardInput = true;
	fRedirectStandardOutput = true;

	fHideWindow = false;
	fCanGetExitStatus = false;

	fQuoteArgumentsIfNeeded = true;

	fUseJobObject = false;
	fJobObject = NULL;
}

XWinProcessLauncher::~XWinProcessLauncher()
{
	_CloseAllPipes();
	
	if (fIsRunning)

		Shutdown();

	if (fUseJobObject && fJobObject != NULL) {
	
		::CloseHandle(fJobObject);
		fJobObject = NULL;

	}

	delete [] fCurrentDirectory;
	delete [] fBinaryPath;
}

void XWinProcessLauncher::_CloseAllPipes()
{
	_CloseAndInvalidateHandle(&fChildStdInRead);
	_CloseAndInvalidateHandle(&fChildStdInWrite);
	_CloseAndInvalidateHandle(&fChildStdInWriteDup);
	
	_CloseAndInvalidateHandle(&fChildStdOutRead);
	_CloseAndInvalidateHandle(&fChildStdOutWrite);
	_CloseAndInvalidateHandle(&fChildStdOutReadDup);
	
	_CloseAndInvalidateHandle(&fChildStdErrRead);
	_CloseAndInvalidateHandle(&fChildStdErrWrite);
	_CloseAndInvalidateHandle(&fChildStdErrReadDup);	
}

void XWinProcessLauncher::SetDefaultDirectory(const VString &inDirectory)
{
	delete [] fCurrentDirectory;
	fCurrentDirectory = NULL;
	
	if(!inDirectory.IsEmpty())
	{
		VFolder		theFolder(inDirectory);
		VString		currentDirectoryPath = theFolder.GetPath().GetPath();

		fCurrentDirectory = _CreateCString(currentDirectoryPath, true);
	}
}

void XWinProcessLauncher::SetEnvironmentVariable(const VString &inVarName, const VString &inValue)
{
	if(!inVarName.IsEmpty())
		fEnvirVariables[inVarName] = inValue;
}

void XWinProcessLauncher::SetEnvironmentVariables(const EnvVarNamesAndValuesMap &inVarAndValues)
{
	fEnvirVariables = inVarAndValues;
}
			
void XWinProcessLauncher::SetBinaryPath(const VString &inBinaryPath)
{
	delete [] fBinaryPath;
	fBinaryPath = NULL;

	// ACI0065260 - 2010-03-11, T.A.
	// Add quotes if the path contains a space and is not already quoted
	if( !inBinaryPath.IsEmpty() && inBinaryPath.FindUniChar(CHAR_SPACE) > 0 && inBinaryPath[0] != CHAR_QUOTATION_MARK )
	{
		VString		quotedPath;

		quotedPath = CHAR_QUOTATION_MARK;
		quotedPath += inBinaryPath;
		quotedPath += CHAR_QUOTATION_MARK;
		fBinaryPath = _CreateCString(quotedPath);
	}
	else
	{
		fBinaryPath = _CreateCString(inBinaryPath);
	}
}

void XWinProcessLauncher::CreateArgumentsFromFullCommandLine(const VString &inFullCommandLine)
{
	fArrayArg.clear();
	
	/*	ACI0065349 - 2010-03-15, T.A.
		SetBinaryPath() may quote the expression, but here, when CreateArgumentsFromFullCommandLine()
		is called, we assume it's the caller responsibility to properly quote what must be quoted
	*/
	delete [] fBinaryPath;
	fBinaryPath = _CreateCString(inFullCommandLine);
}

void XWinProcessLauncher::AddArgument(const VString& inArgument)
{
	if(!inArgument.IsEmpty())
		fArrayArg.push_back(inArgument);
}

void XWinProcessLauncher::ClearArguments()
{
	fArrayArg.clear();
}

void XWinProcessLauncher::CloseStandardInput()
{
	_CloseAndInvalidateHandle(&fChildStdInWriteDup);
}


// Tested on PHP with Vincent L.: sending more than 8192 on the launched process can stuck 4D... so send it by chunk (m.c)
static const sLONG kMAX_CHUNK_SIZE = 8192;
sLONG XWinProcessLauncher::WriteToChild(const void *inBuffer, uLONG inBufferSizeInBytes, bool inClosePipeAfterWritting)
{
	if(testAssert(inBuffer != NULL))
	{
		if(fChildStdInWriteDup == INVALID_HANDLE_VALUE)
		{
			assert(false); // Caller is not supposed to WriteToChild() if he did'nt Start() successfully
		}
		else
		{
			DWORD	totalWritten = 0;
			
			if(inBufferSizeInBytes >= 0)
			{
				VTask::Yield();

				// Read from a file and write its contents to a pipe.
				if (inBufferSizeInBytes > kMAX_CHUNK_SIZE)
				{
					short		countOfChunks = inBufferSizeInBytes / kMAX_CHUNK_SIZE;
					bool		writeFileWasOK;
					DWORD		dwWritten;
					char		*inBufferCharPtr = (char *) inBuffer;

					sLONG	i;
					for (i = 0; i < countOfChunks; ++i)
					{
						dwWritten = 0;
						writeFileWasOK = (::WriteFile(fChildStdInWriteDup, inBufferCharPtr + (kMAX_CHUNK_SIZE * i), kMAX_CHUNK_SIZE, &dwWritten, NULL) !=  0);
						xbox_assert (dwWritten == kMAX_CHUNK_SIZE);

						totalWritten += dwWritten;
						VTask::Yield();
						
						if (!writeFileWasOK) // means "error occured". Can use ::GetLastError()
							break;
					}

					// Send the remaining bytes
					if (writeFileWasOK)
					{
						dwWritten = 0;
						::WriteFile(fChildStdInWriteDup, inBufferCharPtr + (kMAX_CHUNK_SIZE * i), (inBufferSizeInBytes % kMAX_CHUNK_SIZE), &dwWritten, NULL);
						totalWritten += dwWritten;
					}
				}
				else
				{
					::WriteFile(fChildStdInWriteDup, inBuffer, inBufferSizeInBytes, &totalWritten, NULL);
				}
			}
			
			if(inClosePipeAfterWritting)
				CloseStandardInput();
			
			return totalWritten;
		}
	}
	
	return -1;
}


sLONG XWinProcessLauncher::ReadFromChild(char *outBuffer, long inBufferSize)
{
	if(testAssert(fChildStdOutReadDup != INVALID_HANDLE_VALUE))
	{
		DWORD dwRead = 0;
		
		VTask::Yield();
		
		// Close the write end of the pipe before reading from the read end of the pipe.
		_CloseAndInvalidateHandle(&fChildStdOutWrite);
		
		// Read output from the child process, and write to parent's STDOUT. 
		if (!ReadFile(fChildStdOutReadDup, outBuffer, inBufferSize, &dwRead, NULL)) {
			
			// ReadFile() has been set as non-blocking.

			if (GetLastError() != ERROR_NO_DATA)	

				dwRead = -1; 

		} else if (dwRead == 0) {
		
			dwRead = -1;	

		}
		
		return dwRead;
	}
	
	return -1;
}


sLONG XWinProcessLauncher::ReadErrorFromChild(char *outBuffer, long inBufferSize)
{
	if(testAssert(fChildStdErrReadDup != INVALID_HANDLE_VALUE))
	{
		DWORD dwRead = 0;
		
		VTask::Yield();
		
	// Close the write end of the pipe before reading from the read end of the pipe.
		_CloseAndInvalidateHandle(&fChildStdErrWrite);
		
		
	// Read output from the child process, and write to parent's STDERR. 
		if (!ReadFile(fChildStdErrReadDup, outBuffer, inBufferSize, &dwRead, NULL)) {
			
			// ReadFile() has been set as non-blocking.

			if (GetLastError() != ERROR_NO_DATA)	

				dwRead = -1; 

		} else if (dwRead == 0) {
		
			dwRead = -1;	

		}
		
		return dwRead;
	}
	
	return -1;
}

sLONG XWinProcessLauncher::Start()
{
	return Start(fEnvirVariables);
}

sLONG XWinProcessLauncher::Start(const EnvVarNamesAndValuesMap &inVarToUse)
{
	sLONG					err = 0;
	sLONG					pathLen = 0;
	STARTUPINFOW			theStartupInfo;
	PROCESS_INFORMATION		theProcessInformation;
	SECURITY_ATTRIBUTES		theSecurityAttributes;

	wchar_t		*envVarsBlock = NULL;
	wchar_t		*argsAsCString= NULL;

	if(fBinaryPath == NULL)
		return -50;	// Invalid parameter

	pathLen = (sLONG) ::wcslen(fBinaryPath);
	if(pathLen == 0)
		return -50;	// Invalid parameter

	// Sets the bInheritHandle flag so that pipes are inherited
	theSecurityAttributes.nLength = sizeof(theSecurityAttributes);
	theSecurityAttributes.bInheritHandle = TRUE;
	theSecurityAttributes.lpSecurityDescriptor = NULL;
	
	::SetLastError(0);

	// ______________________________________________ STDIN pipes
	if(err == 0 && fRedirectStandardInput)
	{
	// Creates a pipe for the child process STDIN
		if(!CreatePipe(&fChildStdInRead, &fChildStdInWrite, &theSecurityAttributes, 0))
			err = ::GetLastError();

	// Duplicates the write handle so that it is not inherited
		if(err == 0)
		{
			if(!DuplicateHandle(GetCurrentProcess(), fChildStdInWrite, GetCurrentProcess(), &fChildStdInWriteDup, 0, FALSE, DUPLICATE_SAME_ACCESS))
				err = ::GetLastError();
		}

		_CloseAndInvalidateHandle(&fChildStdInWrite);
	}
	
	// ______________________________________________ STDOUT/STDERR pipes
	if(err == 0 && fRedirectStandardOutput)
	{
	// Creates a pipe for the child process's STDOUT
		if(!CreatePipe(&fChildStdOutRead, &fChildStdOutWrite, &theSecurityAttributes, 0))
			err = ::GetLastError();

		if (err == 0) {

			DWORD	mode = PIPE_NOWAIT;
			
			if (!SetNamedPipeHandleState(fChildStdOutRead, &mode, NULL, NULL))
		
				err = ::GetLastError();

		}

	// Duplicates the read handle to the pipe so that it is not inherited
		if(err == 0)
		{
			if(!DuplicateHandle(GetCurrentProcess(), fChildStdOutRead, GetCurrentProcess(), &fChildStdOutReadDup, 0, FALSE, DUPLICATE_SAME_ACCESS))
				err = ::GetLastError();
		}

		_CloseAndInvalidateHandle(&fChildStdOutRead);

	// The same for STDERR
		if(err == 0)
		{
			if(!CreatePipe(&fChildStdErrRead, &fChildStdErrWrite, &theSecurityAttributes, 0))
				err = ::GetLastError();
		}

		if (err == 0) {

			DWORD	mode = PIPE_NOWAIT;
			
			if (!SetNamedPipeHandleState(fChildStdErrRead, &mode, NULL, NULL))
		
				err = ::GetLastError();

		}

		if(err == 0)
		{
			if(!DuplicateHandle(GetCurrentProcess(), fChildStdErrRead, GetCurrentProcess(), &fChildStdErrReadDup, 0, FALSE, DUPLICATE_SAME_ACCESS))
				err = ::GetLastError();
		}
		
		_CloseAndInvalidateHandle(&fChildStdErrRead);
	} // if(err == 0 && fRedirectStandardOutput)
	
	// ______________________________________________ Setup CreateProcess() parameters
	if(err == 0)
	{
	// Sets up members of the PROCESS_INFORMATION structure
		ZeroMemory(&theProcessInformation, sizeof(theProcessInformation));
		
	// Sets up members of the STARTUPINFOW structure
		ZeroMemory(&theStartupInfo, sizeof(theStartupInfo));
		theStartupInfo.cb = sizeof(theStartupInfo);
		if(fRedirectStandardInput || fRedirectStandardOutput)
			theStartupInfo.dwFlags |= STARTF_USESTDHANDLES;
		theStartupInfo.hStdInput = NULL;
		theStartupInfo.hStdOutput = NULL;
		theStartupInfo.hStdError = NULL;
		if(fRedirectStandardInput)
			theStartupInfo.hStdInput = fChildStdInRead;
		if(fRedirectStandardOutput)
		{
			theStartupInfo.hStdOutput = fChildStdOutWrite;
			theStartupInfo.hStdError = fChildStdErrWrite;
		}
		
	// Hides the command window if specified
		if(fHideWindow)
		{
			theStartupInfo.dwFlags |= STARTF_USESHOWWINDOW;
			theStartupInfo.wShowWindow = SW_HIDE;
		}
		
	// Create the environment variables block
		envVarsBlock = _CreateEnvironmentVariablesBlock(inVarToUse);
		xbox_assert(envVarsBlock != NULL);
		
	// Create the CString arguments. See _CreateConcatenetedArguments().
		argsAsCString = _CreateConcatenetedArguments();
		if(argsAsCString == NULL)
		{
			assert(false);
			err = -108;
		}
	} // if(err == 0)
	
	// ______________________________________________ CreateProcess()
	if(err == 0)
	{
		sLONG	argsLen = (sLONG) ::wcslen(argsAsCString) + 1;
		sLONG	commandFullSize = pathLen + 1; //'\0'
		if(argsLen > 0)
			commandFullSize += 1 + argsLen;// ' '
		
		wchar_t *commandLine = NULL;
		
		if(argsLen > 0)
		{
			commandLine = new wchar_t[commandFullSize];
			if(commandLine == NULL)
			{
				err = -108;
			}
			else
			{
				::memcpy(commandLine, fBinaryPath, pathLen * sizeof(wchar_t));
				if(argsLen > 0)
				{
					commandLine[pathLen] = ' ';
					::memcpy(commandLine + pathLen + 1, argsAsCString, argsLen * sizeof(wchar_t));
				}
				commandLine[commandFullSize - 1] = 0;
			}
		}
		else
		{
			sLONG	theSize = (sLONG) ::wcslen(fBinaryPath) + 1;
			commandLine = new wchar_t[theSize];
			::memcpy(commandLine, fBinaryPath, theSize * sizeof(wchar_t));
		}

		// If killing process tree, create the job object.

		if (!err && fUseJobObject) {

			fJobObject = ::CreateJobObject(NULL, NULL);
			if (fJobObject == NULL) 

				err = ::GetLastError();

		}
		
		if(err == 0)
		{
			bool	bInheritHandles = fRedirectStandardInput || fRedirectStandardOutput ? TRUE : FALSE;
			wchar_t *lpCurrentDirectory = (fCurrentDirectory != NULL && fCurrentDirectory[0] > 0) ? fCurrentDirectory : NULL;
			
			SetLastError(0);

			long	tempLastError = 0;
			DWORD	creationFlags;

			creationFlags = CREATE_UNICODE_ENVIRONMENT ;		
			if (fUseJobObject)
				
				creationFlags |= CREATE_SUSPENDED;

			long creaProcResult = CreateProcessW(NULL,						// Application name
												commandLine,				// Command line
												NULL,						// Process security attributes
												NULL,						// Primary thread security attributes
												bInheritHandles,			// Handles are inherited
												creationFlags,				// creation flags, environment is UNICODE
												envVarsBlock,				// NULL == use parent's environment
												lpCurrentDirectory,			// NULL == use parent's directory
												&theStartupInfo,			// STARTUPINFOW pointer	
												&theProcessInformation		// Receives PROCESS_INFORMATION
												);
			tempLastError = GetLastError(); // For debug
			if(creaProcResult == 0)	// "If the function succeeds, the return value is nonzero"
			{
				err = ::GetLastError();

				fIsRunning = false;
				fProcessID = 0;
				ZeroMemory(&fProcInfo, sizeof(fProcInfo));
			}
			else
			{
				fIsRunning = true;
				fProcInfo = theProcessInformation;
				fProcessID = fProcInfo.dwProcessId;

				// If job objects are used, processes are created suspended. 
				// Assign them a job object, then resume (start) them.
				if (fUseJobObject) {
				
					BOOL	successful;

					successful = ::AssignProcessToJobObject(fJobObject, theProcessInformation.hProcess);
					if (!successful)

						err = ::GetLastError();

					else {
					
						DWORD	result;

						result = ::ResumeThread(theProcessInformation.hThread);
						if (result == -1 )

							err = ::GetLastError();					

					}

					if (err) {

						TerminateProcess(fProcInfo.hProcess, 0);
						CloseHandle(fProcInfo.hProcess);
						CloseHandle(fProcInfo.hThread);
						CloseHandle(fJobObject);
						fJobObject = NULL;

					}

				}
			
				// If process is independant and we don't need to retrieve exit status at
				// termination, immediatly close those handles from our part
				if(!err && !fWaitClosingChildProcess && !fCanGetExitStatus)
				{
					CloseHandle(theProcessInformation.hProcess);
					CloseHandle(theProcessInformation.hThread);
					
				}
			
			}
		} // if(err == 0)
		
		delete [] commandLine;
		
	} //  if(err == 0)
	
	if(err != 0)
		_CloseAllPipes();
	
	delete [] argsAsCString;
	delete [] envVarsBlock;
	
	return err;
}

bool XWinProcessLauncher::IsRunning()
{
	if(!fWaitClosingChildProcess)
	{
		if(fProcessID!= 0 && fIsRunning) {
			fIsRunning = GetProcessVersion(fProcessID) != 0;// Does this work *every* time?
			if (!fIsRunning) {

				if (fCanGetExitStatus) {

					GetExitCodeProcess(fProcInfo.hProcess, &fExitStatus);

					CloseHandle(fProcInfo.hProcess);
					CloseHandle(fProcInfo.hThread);

				}

			}
				
		}
	}
	
	return fIsRunning;
}

sLONG XWinProcessLauncher::Shutdown(bool inWithKillIndependantChildProcess, bool inKillProcessTree)
{
	if(IsRunning())
	{
		if(fWaitClosingChildProcess)
		{
			WaitForSingleObject(fProcInfo.hProcess, INFINITE);
			GetExitCodeProcess(fProcInfo.hProcess, &fExitStatus);
			CloseHandle(fProcInfo.hProcess);
			CloseHandle(fProcInfo.hThread);
		}
		else if(inWithKillIndependantChildProcess)
		{
		// fProcInfo.hProcess and fProcInfo.hProcess have been closed in Start()
			if(fProcessID != 0)
			{
				HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, fProcessID);
				TerminateProcess(hProcess, 0);
				if (fCanGetExitStatus) 

					GetExitCodeProcess(hProcess, &fExitStatus);

				CloseHandle(hProcess);

				if (fCanGetExitStatus) {
					
					CloseHandle(fProcInfo.hProcess);
					CloseHandle(fProcInfo.hThread);

				}
									
			}
		}
	}
	
	if (inKillProcessTree) {

		xbox_assert(fUseJobObject);
		if (fUseJobObject && fJobObject != NULL) {
		
			TerminateJobObject(fJobObject, 0);

			::CloseHandle(fJobObject);
			fJobObject = NULL;
		
		} 
	
	}
	
	// Cleanup
	_CloseAllPipes();
	fIsRunning = false;
	fProcessID = 0;
	ZeroMemory(&fProcInfo, sizeof(fProcInfo));

	return 0;
}

bool XWinProcessLauncher::KillProcess (sLONG inPid)
{
	HANDLE	handle;

	if ((handle = OpenProcess(PROCESS_TERMINATE, FALSE, inPid)) != NULL) {

		bool	isOk;

		isOk = TerminateProcess(handle, 0) != 0;
		CloseHandle(handle);
		return isOk;

	} else

		return false;
}

wchar_t * XWinProcessLauncher::_CreateConcatenetedArguments()
{
	sLONG		nbArguments	= (sLONG) fArrayArg.size();
	wchar_t *	theArgs		= NULL;
	
	if(nbArguments < 1)
	{
		theArgs = new wchar_t[1];
		theArgs[0] = 0;
	}
	else
	{
		VString		concatenetedArgs;
		concatenetedArgs.SetEmpty();
		
		for(sLONG i = 0; i < nbArguments; ++i)
		{
			// ACI0065260 - 2010-03-11, T.A.
			// Add quotes if the path contains a space and is not already quoted
			if( fQuoteArgumentsIfNeeded && !fArrayArg[i].IsEmpty() && fArrayArg[i].FindUniChar(CHAR_SPACE) > 0 && fArrayArg[i][0] != CHAR_QUOTATION_MARK )
			{
				VString		quotedArg;

				quotedArg = CHAR_QUOTATION_MARK;
				quotedArg += fArrayArg[i];
				quotedArg += CHAR_QUOTATION_MARK;

				concatenetedArgs.AppendString(quotedArg);
			}
			else
			{
				concatenetedArgs.AppendString(fArrayArg[i]);
			}
			if(i < (nbArguments - 1))
				concatenetedArgs.AppendUniChar(' ');
		}

		theArgs = _CreateCString(concatenetedArgs, true);
	}
	
	return theArgs;
}

// Environment block == null-terminated block of null-terminated strings of the form: name=value\0
wchar_t * XWinProcessLauncher::_CreateEnvironmentVariablesBlock(const EnvVarNamesAndValuesMap &inVarToUse)
{
	wchar_t *		initialEnvStrings	= NULL;
	VMemoryBuffer<>	allStrings;
	wchar_t *		theEnvVarBlock		= NULL;

	// Initial environment variables

	initialEnvStrings = ::GetEnvironmentStringsW();
	if(initialEnvStrings != NULL)
	{
		wchar_t	*currentStr = initialEnvStrings;
		size_t	fullStrSize;

		while (*currentStr)
		{
			fullStrSize = ::wcslen(currentStr) + 1;
			allStrings.PutData(allStrings.GetDataSize(), currentStr, fullStrSize * sizeof(wchar_t));
			currentStr += fullStrSize;
		}
	}

	// Prepare our envir. variables
	if(!inVarToUse.empty())
	{
		XBOX::VString							oneEnvVarAndValue;
		EnvVarNamesAndValuesMap::const_iterator	envVarIterator;

		// Calculate final buffer size (concatenate name=value and add the 0 terminattion)
		 
		for (envVarIterator = inVarToUse.begin(); envVarIterator != inVarToUse.end(); envVarIterator++)
		{
			oneEnvVarAndValue = envVarIterator->first;
			if(!oneEnvVarAndValue.IsEmpty())
			{
				oneEnvVarAndValue += "=";
				oneEnvVarAndValue += envVarIterator->second;

				const wchar_t *	varValueCStr = oneEnvVarAndValue.GetCPointer();	// Null terminated UniCode string.

				if(testAssert(varValueCStr != NULL))
				{
					allStrings.PutData(allStrings.GetDataSize(), varValueCStr, (oneEnvVarAndValue.GetLength() + 1) * sizeof(wchar_t));				
				}
			}						
		}
	} //if(!inVarToUse.empty())

	if(allStrings.GetDataSize() > 0)
	{
		wchar_t	theZero = 0;

		allStrings.PutData(allStrings.GetDataSize(), &theZero, sizeof(wchar_t));

		theEnvVarBlock = new wchar_t[allStrings.GetDataSize()];
		if(testAssert(theEnvVarBlock))
			allStrings.GetData(0, theEnvVarBlock, allStrings.GetDataSize());
	}
	
	if(initialEnvStrings != NULL)
		::FreeEnvironmentStringsW(initialEnvStrings);

	return theEnvVarBlock;
}

wchar_t * XWinProcessLauncher::_CreateCString(const VString &inString, bool inEvenIfStringIsEmpty, sLONG *outFullSizeOfBuffer) const
{
	wchar_t	*cStr			= NULL;
	uLONG	cStrFullSize	= 0;
	
	if(inString.IsEmpty())
	{
		if(inEvenIfStringIsEmpty)
		{
			cStrFullSize = 1;
			cStr = new wchar_t[1];
			*cStr = 0;
		}
	}
	else
	{
		cStrFullSize = inString.GetLength() + 1;
		cStr = new wchar_t[cStrFullSize];
		
		if(cStr != NULL) {

			// XBOX::VString::fString is null terminated.

			::memcpy(cStr, inString.GetCPointer(), cStrFullSize * sizeof(wchar_t));			

		} else
			cStrFullSize = 0;
	}
	
	if(outFullSizeOfBuffer != NULL)
		*outFullSizeOfBuffer = cStrFullSize;
	
	return cStr;
}

void XWinProcessLauncher::_CloseAndInvalidateHandle(HANDLE *ioHandle)
{
	if(testAssert(ioHandle != NULL) && *ioHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(*ioHandle);
		*ioHandle = INVALID_HANDLE_VALUE;
	}
}
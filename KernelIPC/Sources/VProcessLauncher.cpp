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
#include "VProcessLauncher.h"

VProcessLauncher::VProcessLauncher()
{
	fProcessLauncherImpl = new XProcessLauncherImpl();
}

VProcessLauncher::~VProcessLauncher()
{
	delete fProcessLauncherImpl;
}

void VProcessLauncher::SetDefaultDirectory(const VString &inDirectory)
{
	fProcessLauncherImpl->SetDefaultDirectory(inDirectory);
}

void VProcessLauncher::SetEnvironmentVariable(const VString &inVarName, const VString &inValue)
{
	fProcessLauncherImpl->SetEnvironmentVariable(inVarName, inValue);
}
void VProcessLauncher::SetEnvironmentVariables(const EnvVarNamesAndValuesMap &inVarAndValues)
{
	fProcessLauncherImpl->SetEnvironmentVariables(inVarAndValues);
}

void VProcessLauncher::SetBinaryPath(const VString &inBinaryPath)
{
	fProcessLauncherImpl->SetBinaryPath(inBinaryPath);
}

void VProcessLauncher::CreateArgumentsFromFullCommandLine(const VString &inFullCommandLine)
{
	fProcessLauncherImpl->CreateArgumentsFromFullCommandLine(inFullCommandLine);
}


void VProcessLauncher::AddArgument( const VString& inArgument, bool inAddQuote )
{
	VString arg( inArgument );
	if ( inAddQuote )
	{
		arg.Insert("\"",1);
		arg += "\"";
	}
	fProcessLauncherImpl->AddArgument(arg);
}

void VProcessLauncher::ClearArguments()
{
	fProcessLauncherImpl->ClearArguments();
}

void VProcessLauncher::SetWaitClosingChildProcess(bool isWaitingClosingChildProcess)
{
	fProcessLauncherImpl->SetWaitClosingChildProcess(isWaitingClosingChildProcess);
}

void VProcessLauncher::SetRedirectStandardInput(bool isStandardInputRedirected)
{
	fProcessLauncherImpl->SetRedirectStandardInput(isStandardInputRedirected);
}

void VProcessLauncher::SetRedirectStandardOutput(bool isStandardOutputRedirected)
{
	fProcessLauncherImpl->SetRedirectStandardOutput(isStandardOutputRedirected);
}

void VProcessLauncher::CloseStandardInput()
{
	fProcessLauncherImpl->CloseStandardInput();
}

sLONG VProcessLauncher::WriteToChild(const void *inBuffer, uLONG inBufferSizeInBytes, bool inClosePipeAfterWritting)
{
	return fProcessLauncherImpl->WriteToChild(inBuffer, inBufferSizeInBytes, inClosePipeAfterWritting);
}

sLONG VProcessLauncher::WaitForData ()
{
	return fProcessLauncherImpl->WaitForData();
}

sLONG VProcessLauncher::ReadFromChild(char *outBuffer, long inBufferSize)
{
	return fProcessLauncherImpl->ReadFromChild(outBuffer, inBufferSize);
}

sLONG VProcessLauncher::ReadErrorFromChild(char *outBuffer, long inBufferSize)
{
	return fProcessLauncherImpl->ReadErrorFromChild(outBuffer, inBufferSize);
}
			
#if VERSIONWIN
void VProcessLauncher::WIN_DontDisplayWindow()
{
	fProcessLauncherImpl->DontDisplayWindow();
}
void VProcessLauncher::WIN_CanGetExitStatus()
{
	fProcessLauncherImpl->CanGetExitStatus();
}
#endif

sLONG VProcessLauncher::GetPid ()
{
	return fProcessLauncherImpl->GetPid();
}

bool VProcessLauncher::KillProcess (sLONG inPid)
{
	return XProcessLauncherImpl::KillProcess(inPid);
}

sLONG VProcessLauncher::Start()
{
	return fProcessLauncherImpl->Start();
}

sLONG VProcessLauncher::Start(const EnvVarNamesAndValuesMap &inVarToUse)
{
	return fProcessLauncherImpl->Start(inVarToUse);
}

bool VProcessLauncher::IsRunning()
{
	return fProcessLauncherImpl->IsRunning();
}

sLONG VProcessLauncher::Shutdown(bool inWithKillIndependantChildProcess)
{
	return fProcessLauncherImpl->Shutdown(inWithKillIndependantChildProcess);
}

uLONG VProcessLauncher::GetExitStatus ()
{
	return fProcessLauncherImpl->GetExitStatus();
}


/*static*/
sLONG VProcessLauncher::ExecuteCommandLine(const VString& inCommandLine,
										   sLONG inOptions,
										   VMemoryBuffer<> *inStdIn,
										   VMemoryBuffer<> *outStdOut,
										   VMemoryBuffer<> *outStdErr,
										   EnvVarNamesAndValuesMap *inEnvVar,
										   VString *inDefaultDirectory,
										   sLONG *outExitStatus, 
										   VProcessLauncher **outProcessLauncher,
										   std::vector<XBOX::VString> *inArguments)
{
	sLONG				err = -1;
	VProcessLauncher	*processLauncher;
	bool				isBlocking = true; // default value

	if ((processLauncher = new VProcessLauncher()) == NULL)

		return err;
	
	processLauncher->CreateArgumentsFromFullCommandLine(inCommandLine);
	if(inEnvVar != NULL)
		processLauncher->SetEnvironmentVariables(*inEnvVar);
	
	if(inDefaultDirectory != NULL)
		processLauncher->SetDefaultDirectory(*inDefaultDirectory);
	
	if( (inOptions & eVPLOption_NonBlocking) == eVPLOption_NonBlocking )
	{
		processLauncher->SetWaitClosingChildProcess(false);
		isBlocking = false;
	}
	
#if VERSIONWIN
	if( (inOptions & eVPLOption_HideConsole) == eVPLOption_HideConsole )
		processLauncher->WIN_DontDisplayWindow();
#endif

/*
	// If calling "gzip.exe" with no argument, it will return an error message on stderr.
	// Problem is when use VProcessLauncher, stdin is "opened", so even if we close it
	// using "processLauncher.CloseStandardInput()". gzip treats that as a zero length
	// write on stdin and thus create a (minimal, with just header) gz file on stdout.
	// Asynchronous implemention of VJSSystemWorker has same problem.
	// But still, on Windows, not redirecting stdin results in an error message from gzip 
	// because stdin descriptor is not opened. And on Linux, stdin is still considered as
	// opened and gzip is waiting for input.

	if (inStdIn == NULL)

		processLauncher.SetRedirectStandardInput(false);
*/

#if VERSIONWIN

	if (!(outProcessLauncher == NULL && !isBlocking))

		processLauncher->WIN_CanGetExitStatus();

#endif

	// Add argument(s) if any.

	if (inArguments != NULL) {

		for (VSize i = 0; i < inArguments->size(); i++)

			processLauncher->AddArgument(inArguments->at(i));

	}

	err = processLauncher->Start();
	if(err == 0)
	{
		if (inStdIn != NULL)
		{
			sLONG	writtenBytes = processLauncher->WriteToChild( inStdIn->GetDataPtr(), (sLONG)inStdIn->GetDataSize() );			
		}
		processLauncher->CloseStandardInput();
		
#define BUFFER_SIZE	4096
		
		char		miniBuffer[BUFFER_SIZE];
		sLONG		stdoutBytesRead, stderrBytesRead;
				
		if(isBlocking && (outStdOut != NULL || outStdErr != NULL))
		{
			// "Overlap" stdout and stderr reads (prevent system buffer overflow of stdout or stderr).

			do {				

				if ((stdoutBytesRead = processLauncher->ReadFromChild(miniBuffer, BUFFER_SIZE)) > 0)

					outStdOut->PutData(outStdOut->GetDataSize(), miniBuffer, stdoutBytesRead);

				if ((stderrBytesRead = processLauncher->ReadErrorFromChild(miniBuffer, BUFFER_SIZE)) > 0)

					outStdErr->PutData(outStdErr->GetDataSize(), miniBuffer, stderrBytesRead);

			} while (stdoutBytesRead >= 0 || stderrBytesRead >= 0);
		}

		// In non-blocking behavior, this will not kill the external process, but free associated resources.
		// Do not free if we need to further read stdout and stderr.
		
		if (outProcessLauncher == NULL) 

			err = processLauncher->Shutdown(); 

	}

	if (!err && outExitStatus != NULL)

		*outExitStatus = processLauncher->GetExitStatus();

	if (outProcessLauncher != NULL)

		*outProcessLauncher = processLauncher;

	else

		delete processLauncher;
	
	return err;
}

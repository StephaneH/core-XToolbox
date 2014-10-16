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
#include "XPosixProcessLauncher.h"
#include "VProcessLauncher.h"

#include <sys/wait.h>
#include <sys/select.h>
#include <stdlib.h>
#include <set>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define kInvalidDescriptor -1




extern VMemory *gMemory;

// PID of independant children left running are inserted in a watch set, so their zombie processes are cleaned-up.

static pthread_mutex_t	sMutex		= PTHREAD_MUTEX_INITIALIZER;
static std::set<pid_t>	sWatchSet;

// Iterate the entire watch set and clean zombie process.

void XPosixProcessLauncher::_CleanWatchSet ()
{
	std::set<pid_t>::iterator	it, next;
	
	for (it = ::sWatchSet.begin(); it != ::sWatchSet.end(); ) {
		
		int	status;
		
		next = it;
		next++;
		if (waitpid(*it, &status, WNOHANG) == *it)
			
			::sWatchSet.erase(it);	// Doesn't invalidate iterator next.
		
		it = next;
		
	}
}

// Catch SIGCHLD signals so terminated (zombie) processes of "free running" independent children are cleaned-up.

void XPosixProcessLauncher::_SignalHandler (int signum, siginfo_t *info, void *p)
{
	if (signum != SIGCHLD)
		
		return;
	
	// If the terminating child is in watch list, do a waitpid() to clean-up its zombie process.
	
	pthread_mutex_lock(&::sMutex);
	
	std::set<pid_t>::iterator	it;
	
	if ((it = ::sWatchSet.find(info->si_pid)) != ::sWatchSet.end()) {
		
		int	status;
		
		// Clean-up the (now) zombie process.
		
		waitpid(info->si_pid, &status, WNOHANG);
		
		::sWatchSet.erase(it);
		
	}
	
	// If a SIGCHLD signal is sent while another child is terminating, the other SIGCHLD will be lost.
	// Check the entire watch set so missed SIGCHLD are cleaned-up.
	
	_CleanWatchSet();
	
	pthread_mutex_unlock(&::sMutex);	
}

XPosixProcessLauncher::XPosixProcessLauncher()
{	
	fBinaryPath = NULL;
	fCurrentDirectory = NULL;
		
	fIsRunning = false;
	fShutdown = false;
	
	fArrayArgCStrArray = NULL;

	fWaitClosingChildProcess = true;
	
	fProcessID = 0;
	fPipeParentToChild[pReadSide]		= kInvalidDescriptor ;
	fPipeParentToChild[pWriteSide]		= kInvalidDescriptor ;
	fPipeChildToParent[pReadSide]		= kInvalidDescriptor ;
	fPipeChildToParent[pWriteSide]		= kInvalidDescriptor ;
	fPipeChildErrorToParent[pReadSide]	= kInvalidDescriptor ;
	fPipeChildErrorToParent[pWriteSide] = kInvalidDescriptor ;

	fRedirectStandardInput = true;
	fRedirectStandardOutput = true;

	fHideWindow = false;	// use the _4D_OPTION_HIDE_CONSOLE from the langage to change this (m.c)

	// _blocksigchild(1);			
	
	struct sigaction        sigact;

	::memset(&sigact, 0, sizeof(sigact));
	
	sigact.sa_sigaction = &_SignalHandler;
	sigact.sa_flags = SA_NOCLDSTOP | SA_SIGINFO;
		
	// Not used, potentially dangerous.
	
//	sigaction(SIGCHLD, &sigact, NULL);
}

XPosixProcessLauncher::~XPosixProcessLauncher()
{
	if(IsRunning())
		Shutdown();
    
    // ACI0078545, ACI0080952:
    //
    // File descriptor leaks. Shutdown() doesn't necessarly close all file descriptors.
    // So make sure here that all file descriptors are closed. This is ok, because the
    // process launcher destructor's is called, that means no more read or write will
    // happen.
    
    _CloseRWPipe(fPipeParentToChild);
    _CloseRWPipe(fPipeChildToParent);
    _CloseRWPipe(fPipeChildErrorToParent);
    			
	delete [] fCurrentDirectory;
	delete [] fBinaryPath;
	
	_Free2DCharArray(fArrayArgCStrArray);
}

void XPosixProcessLauncher::SetDefaultDirectory(const VString &inDirectory)
{
	delete [] fCurrentDirectory;
	fCurrentDirectory = NULL;
	
	if(!inDirectory.IsEmpty())
	{
		VFolder		theFolder(inDirectory);
		VString		currentDirectoryUnixPath;
	
        theFolder.GetPath().GetPosixPath(currentDirectoryUnixPath);
		fCurrentDirectory = _CreateCString(currentDirectoryUnixPath);
	}
}

void XPosixProcessLauncher::SetEnvironmentVariable(const VString &inVarName, const VString &inValue)
{
	if(!inVarName.IsEmpty())
		fEnvirVariables[inVarName] = inValue;
}

void XPosixProcessLauncher::SetEnvironmentVariables(const EnvVarNamesAndValuesMap &inVarAndValues)
{
	fEnvirVariables = inVarAndValues;
}

void XPosixProcessLauncher::SetBinaryPath(const VString &inBinaryPath)
{
	delete [] fBinaryPath;
	
	fBinaryPath = _CreateCString(inBinaryPath);
	
	// CR 23/04/2007
	// Adds the binary path as the first argument in the array of arguments
	if(fArrayArg.size() == 0)
		fArrayArg.push_back(inBinaryPath);
	else
		fArrayArg[0] = inBinaryPath;
}

void XPosixProcessLauncher::CreateArgumentsFromFullCommandLine(const VString &inFullCommandLine)
{
	fArrayArg.clear();
	
	if(!inFullCommandLine.IsEmpty())
	{
		char		*commandLineCStr = _CreateCString(inFullCommandLine);
		sLONG		argCount = _AnalyzeCommandLine(commandLineCStr, &fArrayArg);
		
		if(argCount > 0)
			SetBinaryPath(fArrayArg[0]);
		
		delete [] commandLineCStr;
	}
}

void XPosixProcessLauncher::AddArgument(const VString& inArgument)
{
	if(!inArgument.IsEmpty())
		fArrayArg.push_back(inArgument);
}

void XPosixProcessLauncher::ClearArguments()
{
	fArrayArg.clear();
}

void XPosixProcessLauncher::SetWaitClosingChildProcess(bool isWaitingClosingChildProcess)
{
	fWaitClosingChildProcess = isWaitingClosingChildProcess;
}

void XPosixProcessLauncher::SetRedirectStandardInput(bool isStandardInputRedirected)
{
	fRedirectStandardInput = isStandardInputRedirected;
}

void XPosixProcessLauncher::SetRedirectStandardOutput(bool isStandardOutputRedirected)
{
	fRedirectStandardOutput = isStandardOutputRedirected;
}

void XPosixProcessLauncher::CloseStandardInput()
{
	_CloseOnePipe(&fPipeParentToChild[pWriteSide]);
}

sLONG XPosixProcessLauncher::WriteToChild(const void *inBuffer, uLONG inBufferSizeInBytes, bool inClosePipeAfterWritting)
{
	if(testAssert(inBuffer != NULL))
	{
		if (fIsRunning && fPipeParentToChild[pWriteSide] != kInvalidDescriptor)
		{
			sLONG nb_written = 0;
			
			if(inBufferSizeInBytes > 0)
			{
				// yield first
				VTask::Yield();//gEngine->GiveHand(-1);
				
	//			_blocksigchild(1);
				nb_written = (sLONG) write(fPipeParentToChild[pWriteSide], inBuffer, inBufferSizeInBytes);
	//			_blocksigchild(0);
			}
			
			if(inClosePipeAfterWritting)
				CloseStandardInput();
			
			return nb_written;
		}
	}
	
	return -1;
}

sLONG XPosixProcessLauncher::WaitForData ()
{
	sigset_t	sigmask;
	fd_set		fds;
	int			maximumDescriptor, r;
	sLONG		code;
		
	sigprocmask (0, NULL, &sigmask);
	sigdelset(&sigmask, SIGCHLD);

	if (fPipeChildToParent[pReadSide] > fPipeChildErrorToParent[pReadSide])
				
		maximumDescriptor = fPipeChildToParent[pReadSide];
		
	else
		
		maximumDescriptor = fPipeChildErrorToParent[pReadSide];
	
	FD_ZERO(&fds);
	FD_SET(fPipeChildToParent[pReadSide], &fds);
	FD_SET(fPipeChildErrorToParent[pReadSide], &fds);
	
	r = pselect(maximumDescriptor + 1, &fds, NULL, NULL, NULL, &sigmask);
	
	if (r == -1) {
	
		if (errno == EINTR && !IsRunning())
			
			// pselect() interrupted by SIGCHLD.
			
			return VProcessLauncher::eVPLTerminated;
		
		else 
			
			return VProcessLauncher::eVPLError;
					
	} else if (r > 0) {
		
		code = 0;
		if (FD_ISSET(fPipeChildToParent[pReadSide], &fds))
			
			code |= VProcessLauncher::eVPLStdOutFlag;
	
		if (FD_ISSET(fPipeChildErrorToParent[pReadSide], &fds))
			
			code |= VProcessLauncher::eVPLStdErrFlag;		
		
		return !code ? VProcessLauncher::eVPLError : code;
		
	} else 
		
		return VProcessLauncher::eVPLError;	
}


sLONG XPosixProcessLauncher::ReadFromChild(char *outBuffer, long inBufferSize)
{
	return _ReadFromPipe(fPipeChildToParent[pReadSide], outBuffer, inBufferSize);
}


sLONG XPosixProcessLauncher::ReadErrorFromChild(char *outBuffer, long inBufferSize)
{
	return _ReadFromPipe(fPipeChildErrorToParent[pReadSide], outBuffer, inBufferSize);
}

sLONG XPosixProcessLauncher::Start()
{
	return Start(fEnvirVariables);
}


sLONG XPosixProcessLauncher::Start(const EnvVarNamesAndValuesMap &inVarToUse)
{
	sLONG		error = 0;	
	int			i;
	
	_AdjustBinaryPathIfNeeded(); 
	
	if(fRedirectStandardInput)
	{
		error = pipe(fPipeParentToChild);
	}
	
	if(error == -1)
		return error;
		
	if (fRedirectStandardOutput)
	{
		error = pipe(fPipeChildToParent);
		if( error == -1)
		{
		// Close the first pipe
			_CloseRWPipe(fPipeParentToChild);
			
		// Invalidate this one
			_InvalidateRWPipe(fPipeChildToParent);
		}
		else
		{
			_SetNonBlocking(fPipeChildToParent[pReadSide]);
			
			error = pipe(fPipeChildErrorToParent);
			if(error == -1)
			{
			// Close the previous pipes
				_CloseRWPipe(fPipeParentToChild);
				_CloseRWPipe(fPipeChildToParent);
				
			// Invalidate this one
				_InvalidateRWPipe(fPipeChildErrorToParent);
			}
			else
			{
				_SetNonBlocking(fPipeChildErrorToParent[pReadSide]);
			}
		}
	}
	if(error != 0)
		return error;
	
	_BuildArrayArguments(fArrayArgCStrArray);	

	// Iterate the watch set for ended (zombie) child processes and free them.
	
	pthread_mutex_lock(&::sMutex);
	_CleanWatchSet();
	pthread_mutex_unlock(&::sMutex);

	// Create child process.
	
	fIsRunning = true;	
	fProcessID = fork();
			
	switch(fProcessID)
	{
	// -------------------------------------------------
		case -1:// Error
	// -------------------------------------------------
			fIsRunning = false;
			
			error = -1; 
			
			_CloseRWPipe(fPipeParentToChild);
			_CloseRWPipe(fPipeChildToParent);
			_CloseRWPipe(fPipeChildErrorToParent);
			break ;
			
	
	// -------------------------------------------------
		case 0:	// Child
	// -------------------------------------------------
		{
            
            /* ACI0073005:
			 *
			 * SQLServer seems to have added callback(s) (using some sort of calls to atexit()) and exit() just doesn't quit.
			 * Use _Exit() instead, this will quit without bothering with callback(s). It may not do all the flushing of exit(),
			 * but this is ok as we just forked the child process and failed directory change or execution, so there is not much
			 * to clean-up.
			 */
            
			_CloseOnePipe(&fPipeParentToChild[pWriteSide]);
			_CloseOnePipe(&fPipeChildToParent[pReadSide]);
			_CloseOnePipe(&fPipeChildErrorToParent[pReadSide]);
			
			if(fPipeParentToChild[pReadSide] != kInvalidDescriptor)
				dup2(fPipeParentToChild[pReadSide], STDIN_FILENO);
			
			if(fPipeChildToParent[pWriteSide] != kInvalidDescriptor)
				dup2(fPipeChildToParent[pWriteSide], STDOUT_FILENO);
			
			if (fPipeChildErrorToParent[pWriteSide] != kInvalidDescriptor)
				dup2(fPipeChildErrorToParent[pWriteSide], STDERR_FILENO);
			
			for(i = 3 ; i < getdtablesize() ; i++)
				close(i);
			
			char**		arrayOfCStrEnvVar = NULL;
			_BuildArrayEnvironmentVariables(inVarToUse, arrayOfCStrEnvVar);
									
			// Moves to the specified current directory
			if(fCurrentDirectory != NULL)
			{
				error = chdir(fCurrentDirectory);
				if(error == -1)
				{
					fprintf(stderr, "********** XPosixProcessLauncher::Start/fork() -> child -> Error: [%s] chdir failed (%s)\n", fCurrentDirectory, strerror(errno));
					fflush(stderr);
					_Exit(-1);
				}
			}
			
			// Executes the process with the specified arguments
			int	execReturnValue = 0;
			if(fArrayArgCStrArray != NULL)
			{
				if(arrayOfCStrEnvVar != NULL)
				{
					execReturnValue = execve(fBinaryPath, fArrayArgCStrArray, arrayOfCStrEnvVar);
				}
				else
				{
					execReturnValue = execv(fBinaryPath, fArrayArgCStrArray);
				}
			}
			else
			{
				char *argv[] = { fBinaryPath, NULL, NULL };
				if (arrayOfCStrEnvVar !=  NULL)
				{
					execReturnValue = execve((char *) fBinaryPath, argv, arrayOfCStrEnvVar);
				}
				else
				{
					execReturnValue = execv((char *) fBinaryPath, argv);
				}
			}

			// execv() and execve() don't return to the calling process unless a problem occured
			fprintf(stderr, "********** XPosixProcessLauncher::Start/fork() -> child -> Error: [%s] execve/execv failed (%s)\n", fBinaryPath, strerror(errno));
			fflush(stderr);
						
			_Exit(1);
		}
			break;
			
	
	// -------------------------------------------------
		default:	//Father
	// -------------------------------------------------
							
			_CloseOnePipe(&fPipeParentToChild[pReadSide]);
			_CloseOnePipe(&fPipeChildToParent[pWriteSide]);
			_CloseOnePipe(&fPipeChildErrorToParent[pWriteSide]);

			break;
	}
	
	return error;
}

bool XPosixProcessLauncher::IsRunning()
{
	if (fIsRunning) {
		
		int status;
		
		if (fProcessID > 0 && waitpid(fProcessID, &status, WNOHANG) == fProcessID) {
			
			fExitStatus = WEXITSTATUS(status);		
			fProcessID = 0;
			fIsRunning = false;	

		}		
		
	}
	return fIsRunning;	
}

sLONG XPosixProcessLauncher::Shutdown(bool inWithKillIndependantChildProcess)
{
	int error = 0;
	int status = 0;
	
	if(IsRunning())
	{
		_CloseOnePipe(&fPipeParentToChild[pWriteSide]);		
		_CloseOnePipe(&fPipeChildToParent[pReadSide]);
		_CloseOnePipe(&fPipeChildErrorToParent[pReadSide]);
	
		if(fWaitClosingChildProcess)
		{
			// Signal handler will set fIsRunning to false.
			
			while(IsRunning())
			{
				VTask::Sleep(15);
				VTask::Yield();
			}
		
			if(status != 0)
			{
			/*
				if( WIFEXITED(status) )		// True if the process terminated normally by a call to _exit(2) or exit(3)
				{
					sLONG	existStatus = WEXITSTATUS(status);
				}
				
				if( WIFSIGNALED(status) )	// True if the process terminated due to receipt of a signal.
				{
					sLONG	whichSignal = WTERMSIG(status);
					
					if( WCOREDUMP(status) ) // true if the termination of the process was accompanied by the creation of a core file containing an image of the process when the signal was received.
					{
					}
				}
				
				if( WIFSTOPPED(status) )	// rue if the process has not terminated, but has stopped and can be restarted.
				{
					sLONG	whichSignal = WSTOPSIG(status);
				}
			*/
				
				error = -1;
			}			
		}
		else if(inWithKillIndependantChildProcess)
		{
			uLONG	timeOut;
			bool	done = false;
			
			if(IsRunning()) {
				
				if(!_TerminateIndependantChildProcess(SIGTERM, 5000))
				{
					if(!_TerminateIndependantChildProcess(SIGQUIT, 5000))
					{
						if(!_TerminateIndependantChildProcess(SIGKILL, 5000))
						{
							// Should always succeed.
						}
					}
				}
								
				if (fProcessID > 0 && waitpid(fProcessID, &status, WNOHANG) == fProcessID)
												
					fExitStatus = WEXITSTATUS(status);		
				
				fProcessID = 0;
				fIsRunning = false;	
			}
		}
		else
		{
			// Child is free running.	
			
			if (fProcessID > 0) {
				
				// Check if already finished, if so do not insert it in watch set.
				
				if (waitpid(fProcessID, &status, WNOHANG) == fProcessID) {
				
					fExitStatus = WEXITSTATUS(status);		
									
				} else {
					
					// Watch set to look for zombie processes.
					
					pthread_mutex_lock(&::sMutex);
					sWatchSet.insert(fProcessID);
					pthread_mutex_unlock(&::sMutex);				
					
				}
				
			}
			
			fProcessID = 0;
			fIsRunning = false;	
		}
	}
	
	return error;
}

bool XPosixProcessLauncher::KillProcess (sLONG inPid)
{
    if (inPid >=0)
        
        return kill(inPid, SIGTERM) == 0;
    
    else
        
        return false;    
}

#pragma mark -

void XPosixProcessLauncher::_AdjustBinaryPathIfNeeded()
{
	if (fBinaryPath != NULL && (fBinaryPath[0] != '/'))
	{
		// Binary name is not a full path, so apply the smart exec finding:
		char *envPATH = getenv("PATH");
		if(envPATH == NULL)
			envPATH = getenv("path");
		
		// check if it is not a relativ path and that the PATH variable exists
		if ( envPATH != NULL && !_DoesBinaryExist(fBinaryPath) )
		{
		// Not a relativ path, not a full path, use the PATH environment variable to build the commabd:
		// ex:	fBinaryPath is "open"
		//			=> does not exist
		//			PATH is /sw/bin:/sw/sbin:/bin:/sbin:/usr/bin:/usr/sbin:/usr/X11R6/bin
		//			=> we try  /sw/bin/open, then /sw/sbin/open, then /bin/open, etc.
			char	fullPath[1024];
			bool	notFound = true;
			char	*lapChar = NULL;
			char	*lapBeginChar = NULL;
			
			lapChar = envPATH;
			
			while (notFound && lapChar != NULL && *lapChar != 0)
			{
				lapBeginChar = lapChar;
				while (*lapChar != 0 && *lapChar != ':')
				{
					++lapChar;
				}
				
				memcpy(fullPath, lapBeginChar, lapChar - lapBeginChar);
				fullPath[lapChar - lapBeginChar] = '/';
				memcpy(fullPath + (lapChar-lapBeginChar) + 1, fBinaryPath, strlen(fBinaryPath) + 1);
				if (_DoesBinaryExist(fullPath))
				{
					sLONG	fullPathLen = (sLONG) strlen(fullPath);
					delete [] fBinaryPath;
					fBinaryPath = new char[ fullPathLen + 1 ];
					memcpy(fBinaryPath, fullPath, fullPathLen);
					fBinaryPath[fullPathLen] = 0;
					notFound = false;
					return;
				}								
				
				if (*lapChar)	// ACI0039929: the last entry of PATH was not used (m.c)
				{
					++lapChar;	// next entry
				}
			}
		}
	}
}

bool XPosixProcessLauncher::_DoesBinaryExist(char * binaryPath)
{
	struct stat theStat;
	
	// Gets information about the file pointed to by binaryPath
	if(stat(binaryPath, &theStat) == 0)
	{
		// If it is a regular file (S_IFREG), then the file exists 
		if( (theStat.st_mode & S_IFMT) == S_IFREG)
			return true;
	}
			
	return false;
}

int XPosixProcessLauncher::_SetNonBlocking(int fd)
{
	int		error = 0;
	int		nonBlockingFlag = O_NONBLOCK;
	int		orginalFlags;

	orginalFlags = fcntl(fd, F_GETFL, 0);
	if (orginalFlags == -1 || fcntl(fd, F_SETFL, orginalFlags | nonBlockingFlag) == -1)
		error = -1;

	return error;
}

void XPosixProcessLauncher::_BuildArrayArguments(char **&outArrayArg)
{	
	sLONG nbArguments = (sLONG) fArrayArg.size();
	
	// Dispose the previous array of arguments if not NULL
	_Free2DCharArray(outArrayArg);
	
	if(nbArguments < 1)
		return;
	
	size_t outArrayArgLength = (nbArguments + 1 ) * sizeof(char*);
	outArrayArg = (char **) gMemory->NewPtr(outArrayArgLength, 0);
	
	if(testAssert(outArrayArg != NULL))
	{
		for(sLONG i = 0; i < nbArguments; ++i)
		{
			VString		*currentArgPtr = &fArrayArg[i];
			size_t		argFullLength = currentArgPtr->GetLength() * 2;// + 1;
			char		*newArgPtr = new char[argFullLength];
			
			if(newArgPtr != NULL)
				currentArgPtr->ToCString(newArgPtr, argFullLength);
			
			outArrayArg[i] = newArgPtr;
		}
		outArrayArg[nbArguments] = NULL;
	}
}

void XPosixProcessLauncher::_BuildArrayEnvironmentVariables(const EnvVarNamesAndValuesMap &inVarToUse, char **&outArrayEnv)
{
/*	An environment variable array for Unix is a pointer to a null-terminated array of character pointers to null-terminated strings
	Each element is formated "var_name=var_value", ie "PHP_FCGI_CHILDREN=1"
*/
	// Dispose the previous array of arguments if not NULL
	_Free2DCharArray(outArrayEnv);
	
	if(inVarToUse.empty())
		return;
	
	// Don't handle empty names
	sLONG		nbNonEmptyNames = 0;
	
	EnvVarNamesAndValuesMap::const_iterator	envVarIterator = inVarToUse.begin();
	while(envVarIterator != inVarToUse.end())
	{
		if(!envVarIterator->first.IsEmpty())
			++nbNonEmptyNames;
		
		++envVarIterator;
	}
	
	if(nbNonEmptyNames == 0)
		return;
	
	size_t outArrayEnvLength = (nbNonEmptyNames + 1) * sizeof(char *);// +1 for last NULL element
	outArrayEnv = (char **)gMemory->NewPtrClear(outArrayEnvLength, 0);
	if(outArrayEnv == NULL)
	{
		assert(false);
		return;
	}
		
	XBOX::VString	oneEnvVarAndValue;
	
	nbNonEmptyNames = 0;
	
	envVarIterator = inVarToUse.begin();
	while(envVarIterator != inVarToUse.end())
	{
		oneEnvVarAndValue = envVarIterator->first;
		if(!oneEnvVarAndValue.IsEmpty())
		{
			oneEnvVarAndValue += "=";
			oneEnvVarAndValue += envVarIterator->second;
			
			char *oneFinalEntry = _CreateCString(oneEnvVarAndValue);
			if(testAssert(oneFinalEntry != NULL))
			{
				++nbNonEmptyNames;
				outArrayEnv[nbNonEmptyNames - 1] = oneFinalEntry;
			}
			else
			{
				break;
			}
		}
		
		++envVarIterator;
	}
	outArrayEnv[nbNonEmptyNames] = NULL;
}

void XPosixProcessLauncher::_Free2DCharArray(char **&array)
{
	if(array != NULL)
	{
		sLONG i = 0;
		while(array[i] != NULL)
		{
			delete [] array[i];
			array[i] = NULL;
			++i;
		}
		gMemory->DisposePtr((void*) array);
		array = NULL;
	}
}

void XPosixProcessLauncher::_blocksigchild(int block)
{
	sigset_t	mask;
	
	sigemptyset(&mask);
	sigaddset(&mask, SIGCHLD);
	sigprocmask(block ? SIG_BLOCK : SIG_UNBLOCK, &mask, NULL);
}

// Quick utilities
void XPosixProcessLauncher::_CloseRWPipe(int *outPipe)
{
	if(outPipe != NULL)
	{
		if(outPipe[pReadSide] != kInvalidDescriptor)
			close(outPipe[pReadSide]);
		outPipe[pReadSide] = kInvalidDescriptor;
		
		if(outPipe[pWriteSide] != kInvalidDescriptor)
			close(outPipe[pWriteSide]);
		outPipe[pWriteSide] = kInvalidDescriptor;
	}
}

void XPosixProcessLauncher::_CloseOnePipe(int *outPipe)
{
	if(outPipe != NULL)
	{
		if(*outPipe != kInvalidDescriptor)
			close(*outPipe);
		
		*outPipe = kInvalidDescriptor;
	}
}


void XPosixProcessLauncher::_InvalidateRWPipe(int *outPipe)
{
	if(outPipe != NULL)
	{
		outPipe[pReadSide] = kInvalidDescriptor;
		outPipe[pWriteSide] = kInvalidDescriptor;
	}
}

sLONG XPosixProcessLauncher::_ReadFromPipe(int inPipe, char *outBuffer, long inBufferSize)
{
	sLONG nb_read = 0;
	
	if(!testAssert(outBuffer != NULL))
		return -1;
	
	if(inBufferSize == 0)
		return -1;
	
	if (!fIsRunning || (inPipe == kInvalidDescriptor))
		return -1;
	
//	_blocksigchild(1);
	
	nb_read = (sLONG) read(inPipe, outBuffer, (unsigned long) inBufferSize);
	
//	_blocksigchild(0);
	
	if ((nb_read == -1) && (errno == EAGAIN))
	{
		// this is not a real error, just a non blobking socket
		// that has no data to be read
		nb_read = 0;
	}
	else if (nb_read == 0)
	{
		// there is nothing more to be read
		nb_read = -1;
	}
	
	// give time to the other 4D processes
	VTask::Yield();
	
	return nb_read;
}

bool XPosixProcessLauncher::_TerminateIndependantChildProcess(int inSignal, sLONG inTimeOutInMS)
{
	bool	wasTerminated = false;
	int		status = 0;
	uLONG	timeOut;
	
	if(inTimeOutInMS <= 0)
		inTimeOutInMS = 30;
	
	timeOut = XBOX::VSystem::GetCurrentTime() + inTimeOutInMS;
	if(kill(fProcessID, inSignal) == 0)
	{
		while(IsRunning() && XBOX::VSystem::GetCurrentTime() < timeOut) {
			
			VTask::Sleep(30);
			VTask::Yield();
			
		} 
		wasTerminated = !IsRunning();
		
	}
	else if(errno == ESRCH)
	{
		wasTerminated = true;
	}
	
	return wasTerminated;
}

char * XPosixProcessLauncher::_CreateCString(const VString &inString, bool inEvenIfStringIsEmpty, sLONG *outFullSizeOfBuffer) const
{
	char *cStr = NULL;
	uLONG cStrFullSize = 0;
	
	if(inString.IsEmpty())
	{
		if(inEvenIfStringIsEmpty)
		{
			cStrFullSize = 1;
			cStr = new char[1];
			*cStr = 0;
		}
	}
	else
	{
		cStrFullSize = inString.GetLength() * 2;
		cStr = new char[cStrFullSize];
		
		if(cStr != NULL)
			inString.ToCString(cStr, cStrFullSize);
		else
			cStrFullSize = 0;
	}
	
	if(outFullSizeOfBuffer != NULL)
		*outFullSizeOfBuffer = cStrFullSize;
	
	return cStr;
}

sLONG XPosixProcessLauncher::_AnalyzeCommandLine(char *str_in, VectorOfVString *outAllArgsPtr)
{
	if(outAllArgsPtr != NULL)
		outAllArgsPtr->clear();
	/*
	 Here is an example of what we want to parse:
	 /usr/bin/ls -l "/my folder"
	 */
	char *begin_token = NULL;	
	
	sLONG l_nb_words = 0;
	char *lap_char = str_in;
	
	while (*lap_char)
	{
		// skip spaces...
		while (*lap_char && (*lap_char == ' '))
			++lap_char;
		
		if (*lap_char &&
			((*lap_char == '"') || (*lap_char == ('\'')))
			)
		{
			// special case for argument between "" or '', which can contain white space...
			char l_matching_end = *lap_char;	// m.c, ACI0035114
			
			++lap_char;
			begin_token = lap_char;	// the token starts _after_ the " (m.c, ACI0033949)
			
			// find the closing '"'
			while (*lap_char && (*lap_char != l_matching_end))
			{
				++lap_char;
				
				/*
				 check for the escape character '\'
				 
				 ACI0061609, TA, 2009-05-15. The argument may contain sereval escaped spaces, for example: open /Users/ta/Desktop/Test\ \ 2spaces.rtf
				 */
				/*if*/while(*lap_char && (*lap_char == '\\'))
				{
					if (outAllArgsPtr == NULL)
					{
						// we are just counting, don't move the characters
						++lap_char;	// skip '\'
						if (*lap_char)
							++lap_char;	// skip the escaped charcater
					}
					else
					{
						// a bit more tricky here: we need to move the end of the string,
						// to get rid of the '\'
						memmove(lap_char,lap_char+1,strlen(lap_char+1)+1);
						
						if (*lap_char)
							++lap_char;
					}
				}
				
			}
			
			// here we have a valid entry
			if (outAllArgsPtr != NULL)
			{
				if (*lap_char)
				{
					*lap_char = '\0';	// replace the " by a NULL chat
					++lap_char;
				}
				outAllArgsPtr->push_back(begin_token); //arg_array[l_nb_words] = begin_token;
			}
			else
			{
				if (*lap_char)
					++lap_char;
			}
			
			if (*lap_char)
				++lap_char;
			
			++l_nb_words;
		}
		else if (*lap_char)	// m.c, ACI0043818
		{
			// find the following space
			begin_token = lap_char;
			++lap_char;
			
			// find the closing ' '
			while (*lap_char && (*lap_char != ' '))
			{
				++lap_char;
				
				/*
				 check for the escape character '\'
				 
				 ACI0061609, TA, 2009-05-15. The argument may contain sereval escaped spaces, for example: open /Users/ta/Desktop/Test\ \ 2spaces.rtf
				 */
				/*if*/while(*lap_char && (*lap_char == '\\'))
				{
					if (outAllArgsPtr == NULL)
					{
						// we are just counting, don't move the characters
						++lap_char;	// skip '\'
						if (*lap_char)
							++lap_char;	// skip the escaped charcater
					}
					else
					{
						// a bit more tricky here: we need to move the end of the string,
						// to get rid of the '\'
						memmove(lap_char,lap_char+1,strlen(lap_char+1)+1);
						
						if (*lap_char)
							++lap_char;
					}
				}
			}
			
			// here we have a valid entry
			if (outAllArgsPtr != NULL)
			{
				if (*lap_char)
				{
					*lap_char = '\0';
					++lap_char;
				}
				outAllArgsPtr->push_back(begin_token); //arg_array[l_nb_words] = begin_token;
			}
			else
			{
				if (*lap_char)
					++lap_char;
			}
			
			++l_nb_words;
		}
	}
	
	return l_nb_words;
}

// --EOF--

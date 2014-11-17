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
#ifndef __VProcessLauncher__
#define __VProcessLauncher__

#if VERSIONMAC || VERSION_LINUX
	#include "XPosixProcessLauncher.h"
#elif VERSIONWIN
	#include "XWinProcessLauncher.h"
#endif

BEGIN_TOOLBOX_NAMESPACE

/** brief	class VProcessLauncher
 
	Launch an "external" (== not the current app.) process: a command file, an application, a monitor's shell command, ...
	Basically, it does about the same as if you entered the command in the Terminal (Mac) or the Command prompt (Windows).

	Different optionnal parameters/settings let the caller to:
			- Fill STDIN
			- Read the result from STDOUT
			- Read an error from STDERR
			- Send environment variables to the child
			- Set a default directory for the child
			- Wait until the child ends or return immediately

	By default:
			- Call is blocking: father waits until child ends.
			- A console window is opened on Windows (not on Mac)

	There are 2 way for using this class:
		- A high level static routine lets you to do everyting in one call. For example:
				sLONG err = VProcessLauncher::ExecuteCommandLine( CVSTR("chmod +x /Users/username/Desktop/TestLEP.txt") );

			or, with results expected:
				VString		commandLine("/bin/cat /Users/username/Desktop/TestLEP.txt");
				VMemoryBuffer<>		resultOut, resultErr;
	 
				sLONG err = VProcessLauncher::ExecuteCommandLine(commandLine, eVPLOption_HideConsole, NULL, &resultOut, &outStdErr);

		- More low level routines lets you to set the parameters, read result manually, shutdown the child, ...
		  Basically, if you look how VProcessLauncher::ExecuteCommandLine() works, you'll see how low level routines work

	--------------------
	IMPORTANT
	--------------------
	When using VProcessLauncher, don't forget to quote what must be quoted, to escape chars that must be espaced, etc. For example, on Windows,
	if you open a document using the "start" command, this will work...
 
			VProcessLauncher::ExecuteCommandLine("cmd.exe /C start \"\" \"C:\\My Document With Spaces.txt\"");
 
	...because the document <My Document With Spaces.txt> is quoted. But the following call will fail because there are no quotes and spaces:
			VProcessLauncher::ExecuteCommandLine("cmd.exe /C start \"\" C:\\My Document With Spaces.txt");
	
*/
class XTOOLBOX_API VProcessLauncher : public VObject
{
public:
	
	/** @brief	High level routines that does everything in one shot.
				Remember that, _by_default_ in VProcessLauncher:
					-> The call is blocking. Father waits until yhe child terminates.
					-> On Windows, the console is displayed.
	 
				* Examples
					- Mac
						* Read the content of a file on disk
								VString		commandLine("/bin/cat /Users/username/Desktop/TestLEP.txt");
								VMemoryBuffer<>		resultOut, resultErr;
					 
								sLONG err = VProcessLauncher::ExecuteCommandLine(commandLine, eVPLOption_HideConsole, NULL, &resultOut, &outStdErr);
								// Convert resultOut to a string. You're supposed to know what is the encoding returned by the command

						* Change file access
								VString		commandLine("chmod +x /Users/username/Desktop/TestLEP.txt")
								sLONG err = VProcessLauncher::ExecuteCommandLine(commandLine);
	 
					- Windows
						* Launch an exe with default directory and no concole window:
								VString		commandLine("C:\\MyFolder\\MySubFolder\\MyCommand.exe";
								sLONG err = VProcessLauncher::ExecuteCommandLine(commandLine, eVPLOption_HideConsole + eVPLOption_NonBlocking);

	*/
	enum {
		eVPLOption_None					= 0,
		eVPLOption_HideConsole			= 1,	// Used on windows only
		eVPLOption_NonBlocking			= 2
	};
	
	
	/** @brief	Return values for WaitForData(). First check for error and termination codes, then use flags to check for available data.
	 
					- eVPLError				Something really wrong happened. You should consider external process as faulty and terminate
											it (Shutdown()) immediately.
	 
					- eVPLStdOutFlag		Data is available on stdout.	
	 
					- eVPLStdErrFlag		Data is available on stderr.
	 
					- eVPLBothFlags			Data is available on both stdout and stderr.
	 
					- eVPLTerminated		External process is terminated.
	 
	 */
	
	enum {

		eVPLError			= 0,
		eVPLStdOutFlag		= 1, 
		eVPLStdErrFlag		= 2,
		eVPLBothFlags		= eVPLStdOutFlag | eVPLStdErrFlag,
		eVPLTerminated		= 4
		
	};

	// outProcessLauncher allows to retrieve the VProcessLauncher used to execute the command line.
	// There is no use for it, because it is possible to retrieve stdout, stderr, and the exit status directly.
	// However, this is useful if used in non blocking mode. In which case, when done, Shutdown(true) must be 
	// called for proper clean-up and the object deleted.
	// Arguments are "pushed" in order to "argv". It is best not to mix with argument(s) in inCommandLine.

static	sLONG		ExecuteCommandLine(const VString& inCommandLine,
									   sLONG inOptions = eVPLOption_None,
									   VMemoryBuffer<> *inStdIn = NULL,
									   VMemoryBuffer<> *outStdOut = NULL,
									   VMemoryBuffer<> *outStdErr = NULL,
									   EnvVarNamesAndValuesMap *inEnvVar = NULL,
									   VString *inDefaultDirectory = NULL,
									   sLONG *outExitStatus = NULL,
									   VProcessLauncher **outProcessLauncher = NULL,
									   std::vector<XBOX::VString> *inArguments = NULL,
									   bool inQuoteArgumentsIfNeeded = true,
									   bool inKillProcessTree = false);
	
	/** @brief	Constuctor/Destructor */
					VProcessLauncher();
		virtual		~VProcessLauncher();
		
	/** @name Settings. They must be set _before_ calling Start(). */
	//@{
		/** @brief	A full command line is something like:  "[here the full path]php-cgi -b 127.0.0.1:8002"
					If you call CreateArgumentsFromFullCommandLine(), you don't need to SetBinaryPath();
		*/
		void		CreateArgumentsFromFullCommandLine(const VString &inFullCommandLine);
		void		AddArgument(const VString& inArgument, bool inAddQuote = false);
		void		ClearArguments();
	
		/** @brief	Tells which executable to launch. Arguments can also be passed on Windows.
					If you use CreateArgumentsFromFullCommandLine() you don't need to SetBinaryPath(),
					the binary path has been extracted from the full command line.
		*/
		void		SetBinaryPath(const VString &inBinaryPath);

		/** @brief	Specific environment variables passed to the child. For example, if you launch a fast-cgi-php, you
					can set eh "PHP_FCGI_CHILDREN" envir. variable.
		*/
		void		SetEnvironmentVariable(const VString &inVarName, const VString &inValue);
		void		SetEnvironmentVariables(const EnvVarNamesAndValuesMap &inVarAndValues);
	
		/** @brief	The default directory set for the child */
		void		SetDefaultDirectory(const VString &inDirectory);
		
		/** @brief	By default, VProcessLauncher waits until the child has quit. You can change this behavior by using SetWaitClosingChildProcess() */
		void		SetWaitClosingChildProcess(bool isWaitingClosingChildProcess);
	
		/** @brief	Windows only: when calling the command prompt, its window is displayed by the OS. This is annoying. You can change this behavior */
#if VERSIONWIN
	// If set, the lauched process will not open any window.
		void		WIN_DontDisplayWindow();

	// If doesn't wait closing of child process. Allow the retrieval of the exit status after termination.
	// Terminated process clean-up will no longer be automatic. Calling IsRunning() or Shutdown() will be needed.
		void		WIN_CanGetExitStatus();

#endif
		
		/** @brief	By default, STDIN, STDOUT and STDERR are redirected, so the father can write to/read from the child
					If you set the values to false, routines that write to/read from the child will fail (WriteToChild(), ReadFromChild(), ...)
		*/
		void		SetRedirectStandardInput(bool isStandardInputRedirected);
		void		SetRedirectStandardOutput(bool isStandardOutputRedirected);
	
		/** @brief	Get process's PID, platform specific numbering, valid only if IsRunning() returns true. 
		 *			The PID must be sufficient to find and kill the process.
		 */
		
		sLONG		GetPid ();

		/** @brief	Kill the process with the given PID, return true if successful. 
         *          On Windows, the process is terminated. On Mac/Linux, the process receives a SIGTERM signal (this is needed for PHP).
         */

static	bool		KillProcess (sLONG inPid);

	
	//@}
 
	/** @name	Write to / Read from the child
	 *			If the redirection of STDIN/STDOUT/STDERR has been set to false before Start(), all read/write routine return an error (-1 most of the time)
	 */
	//@{
		/** @brief	Write to the child's STDIN. Returns of the number of bytes written or -1 in case of error (if you write 0 bytes, it returns 0) */
		sLONG		WriteToChild(const void *inBuffer, uLONG inBufferSizeInBytes, bool inClosePipeAfterWritting = false);
	
		/** @brief	Closing STDIN pipe after writting into it is a good habit */
		void		CloseStandardInput();
	
		/** @brief	Block and wait for data or external process termination. */	
		sLONG		WaitForData ();
		
		/** @brief	Read from the child's STDOUT. Returns the number of bytes read or -1. Non-blocking call, return 0 if no data.*/
		sLONG		ReadFromChild(char *outBuffer, long inBufferSize);
		
		/** @brief	Read from the child's STDERR. Returns the number of bytes read or -1. Non-blocking call, return 0 if no data. */
		sLONG		ReadErrorFromChild(char *outBuffer, long inBufferSize);
	//@}

		void		SetQuoteArgumentsIfNeeded(bool inQuoteArgumentsIfNeeded);
	
	/** @name	Launch and shutdown
	 *			If the redirection of STDIN/STDOUT/STDERR has been set to false before Start(), all read/write routine return an error (-1 most of the time)
	 */
	//@{

		/** @brief	When an external process is launched, it can spawn its own children. It may sometimes be needed to terminate the entire created process tree. 
					To do so, EnableProcessTreeKill() must be called before Start(). It is "sticky", it cannot be disabled thereafter. Shutdown()'s optional "kill
					process tree" argument can then be used. Note that destructor doesn't kill the process tree. Process groups are used on POSIX platforms 
					(Mac/Linux). For Windows, job objects are used.					
		*/
		void		EnableProcessTreeKill ();

		/** @brief	Start() returns an error code, 0 == no error
					Uses the variables passed to SetEnvironmentVariable()/SetEnvironmentVariables(), if any.
		*/
		sLONG		Start();
	
		/** @brief	Start(const EnvVarNamesAndValuesMap &inVarToUse) returns an error code, 0 == no error
					Uses inVarToUse, and ignore previous calls to SetEnvironmentVariable()/SetEnvironmentVariables()
		*/
		sLONG		Start(const EnvVarNamesAndValuesMap &inVarToUse);

		/** @brief	When SetWaitClosingChildProcess(false); has been used, IsRunning() doesn't rely on internal flags: it uses system APIs to check
					if the child process is running. For example, on Mac it returns (kill(pid, 0) == 0). For POSIX only: Upon detection of termination
					the zombie child process is cleaned-up.
		*/
		bool		IsRunning();

		/** @brief	When SetWaitClosingChildProcess(false); has been used, the child process is about totally independant. If you want
					to (try to) kill it while stopping this VProcessLauncher, use Shutdown(true).
					Note that after the call, the internal member that stores the child process id is always reset to 0 and, so, can't be used later.
					For POSIX: the zombie child process is cleaned-up after Shutdown(). However, if the child is free running (independant and not killed
					at shutdown), its PID is added to a watch set. Zombie processes are cleaned-up if fork() runs out of free process.
					If inKillProcessTree is true, then EnableProcessTreeKill() must have been called before Start()
		*/
		sLONG		Shutdown(bool inWithKillIndependantChildProcess = false, bool inKillProcessTree = false);
	
		/** @brief	Retrieve exit status, only valid if IsRunning() is false or Shutdown() has been called. 
					** For Windows, if SetWaitClosingChildProcess(false), WIN_CanGetExitStatus() must be called before Start(). **
					Exit status is system dependent. 
						- On POSIX systems (Mac or Linux), the exit status is the low 8 bits. 
						- On Windows, it is a 32-bit value.
		*/ 
		uLONG		GetExitStatus ();
	
	//@}
		
private:
		XProcessLauncherImpl	*fProcessLauncherImpl;
};

END_TOOLBOX_NAMESPACE

#endif
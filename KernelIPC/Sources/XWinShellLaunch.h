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
#ifndef __XWinShellLaunch__
#define __XWinShellLaunch__

#include "Kernel/VKernel.h"
#include <vector>

BEGIN_TOOLBOX_NAMESPACE
/**
 * \brief Helper class to launch a program/application using shell services
 * \details Enables launching a particular program or application using Vista and
 * onwards standard security framework (UAC). Application can be launch
 * using elevated (with privileges) or normal security token. The launched 
 * application must explicitly require an elevated execution level, which is
 * doumented in the manifest.
 */
class XTOOLBOX_API XWinShellLaunchHelper:public XBOX::VObject
{
public:
	XWinShellLaunchHelper();
	virtual ~XWinShellLaunchHelper();

	enum LaunchMode
	{
		LM_Normal = 0,//normal security token
		LM_Elevated //elevated security token
	};

	void SetLaunchMode(LaunchMode inMode){fLaunchMode = inMode;}
	LaunchMode  GetLaunchMode()const{return fLaunchMode;}

	///Indicates if the launched process should show a window or hide it
	void SetShowWindowEnabled(bool inEnabled){fShowWindow = inEnabled;}
	bool GetShowWindowEnabled()const{return fShowWindow;}
	
	///Indicate if the @c Run() or @c RunAsDesktopUser() methods should wait for the target process termination
	void SetWaitForTerminationEnabled(bool inEnabled){fWaitTermination = inEnabled;}
	bool GetWaitForTerminationEnabled()const{return fWaitTermination;}

	void SetProgramPath(const XBOX::VFilePath& inPath){fProgramPath = inPath;}
	const XBOX::VFilePath& GetProgramPath()const{ return fProgramPath;}

	void SetWorkingDirectory(const XBOX::VFilePath& inPath){fWorkingDirectory = inPath;}
	const XBOX::VFilePath& GetWorkingDirectory()const{return fWorkingDirectory;}

	/**
	 * \brief Adds an argument to the application's command line
	 * \param inArg the argument name
	 * \param inValue the argument value, NULL for no value
	 * \param inQuoteVal if true and @c inValue is not NULL then it will be surrounded
	 * by block quotes on the command line.
	 * <pre>
	 * AddArgument("MyArg",NULL,true) or AddArgument("MyArg",NULL) -> <appname> MyArg 
	 * AddArgument("MyArg","MyVal") -> <appname> MyArg MyVal
	 * AddArgument("MyArg","MyVal",true) -> <appname> MyArg "MyVal"
	 * </pre>
	 */
	void AddArgument(const XBOX::VString& inArg,const XBOX::VString& inValue,bool inQuoteVal=false);
	void AddArgument(const XBOX::VString& inArg,bool inQuoteVal=false);
	void AddArgument(const XBOX::VString& inArg,const XBOX::VFilePath& inValue);
	void AddArgument(const XBOX::VFilePath& inValue);

	void ClearArguments();

	/**
	 * \brief Starts the application using the current configuration
	 * \param outError if not NULL this will contain the result of the launch operation
	 * \param outReturnCode if not NULL and launched app termination wait is enabled then 
	 * this will contain the exit status of the app
	 */
	bool Start(XBOX::VError* outError = NULL,sLONG* outReturnCode = NULL)const;

	/**
	 * \brief Starts the command as the currently logged on desktop user
	 * \details This method will only run on an elevated process. The launch mode
	 * is ignored
	 * are ignored 
	 * \param outError the error that occured if the command could not be launched
	 * \param outReturnedCode the exit status of the launched command if launched succeeded
	 * \return true if the command could be launched, false otherwise
	 */
	bool StartAsDesktopUser(XBOX::VError& outError,sLONG* outReturnCode = NULL);

	void GetCommandLine(XBOX::VString& outCmdLine)const;

private:
	void _BuildCommandLineArguments(XBOX::VString& ioArgs)const;
	void _BuildCommandLineArguments(wchar_t** ioArgs)const;
	bool _AdjustPrivileges();
	
	/**
	 * \brief Runs the command
	 * \param inMode the launch mode for the target program
	 * \param outError the error that occured if the command could not be launched
	 * \param outReturnedCode the exit status of the launched command if launched succeeded
	 * \return true if the command could be launched, false otherwise
	 */
	bool Run(LaunchMode inMode,XBOX::VError& outError,sLONG* outReturnCode = NULL)const;

private:
	LaunchMode								fLaunchMode;
	bool									fWaitTermination;
	bool									fShowWindow;
	std::vector<XBOX::VString>				fArgs;
	wchar_t*								fCmdLineOptions;
	XBOX::VFilePath							fProgramPath;
	XBOX::VFilePath							fWorkingDirectory;
};

END_TOOLBOX_NAMESPACE

#endif //__XWinShellLaunch__

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
#include "XWinShellLaunch.h"
#include <Windows.h>

USING_TOOLBOX_NAMESPACE


XWinShellLaunchHelper::XWinShellLaunchHelper():XBOX::VObject(),
fLaunchMode(LM_Normal),
fWaitTermination(false),
fShowWindow(false),
fCmdLineOptions(NULL)
{

}

XWinShellLaunchHelper::~XWinShellLaunchHelper()
{
	if (fCmdLineOptions != NULL)
	{
		VMemory::DisposePtr(fCmdLineOptions);
	}
	fCmdLineOptions = NULL;
	fArgs.clear();
}
void XWinShellLaunchHelper::AddArgument(const XBOX::VString& inArg,const XBOX::VFilePath& inValue)
{
	XBOX::VString vv;
	if(!inArg.IsEmpty())
	{
		XBOX::VString str;
		inValue.GetPath(str);
		//On Windows a folder path with a final backslash will break command line parsing so strip it
		if (str.EndsWith(CVSTR("\\")))
		{
			str.SubString(1, str.GetLength() - 1);
		}
		//Ensure path is quoted in case it contains spaces
		vv.AppendPrintf("%S \"%S\"",&inArg,&str);
		fArgs.push_back(vv);
	}
}
void XWinShellLaunchHelper::AddArgument(const XBOX::VFilePath& inValue)
{
	XBOX::VString vv;
	inValue.GetPath(vv);
	//On Windows a folder path with a final backslash will break command line parsing so strip it
	if (vv.EndsWith(CVSTR("\\")))
	{
		vv.SubString(1, vv.GetLength() - 1);
	}
	//Ensure path is quoted in case it contains spaces
	vv.Insert(CHAR_QUOTATION_MARK,1);
	vv.AppendUniChar(CHAR_QUOTATION_MARK);
	fArgs.push_back(vv);
}

void XWinShellLaunchHelper::AddArgument(const XBOX::VString& inArg,bool inQuoteVal)
{
	XBOX::VString vv;
	if(!inArg.IsEmpty())
	{
		if (inQuoteVal)
		{
			vv.AppendPrintf("\"%S\"",&inArg);
		}
		else
		{
			vv.AppendPrintf("%S",&inArg);
		}
		fArgs.push_back(vv);
	}
}

void XWinShellLaunchHelper::AddArgument(const XBOX::VString& inArg,const XBOX::VString& inValue,bool inQuoteVal)
{
	XBOX::VString vv;
	if(!inArg.IsEmpty())
	{
		if(!inValue.IsEmpty())
		{
			if (inQuoteVal)
			{
				vv.AppendPrintf("%S \"%S\"",&inArg,&inValue);
			}
			else if(testAssert(!inValue.IsEmpty()))
			{
				vv.AppendPrintf("%S %S",&inArg,&inValue);
			}
		}
		else
		{
			vv.AppendPrintf("%S",&inArg);
		}
		fArgs.push_back(vv);
	}
}

void XWinShellLaunchHelper::ClearArguments()
{
	fArgs.clear();
}

bool XWinShellLaunchHelper::StartAsDesktopUser(XBOX::VError& outError,sLONG* outReturnCode)
{
	StErrorContextInstaller errContext;
	outError = XBOX::VE_INVALID_PARAMETER;
	bool ok = false;

	if(testAssert(fProgramPath.IsFile()))
	{
		_AdjustPrivileges();
	}

	if(errContext.GetLastError() == VE_OK)
	{
		HWND hShellWnd = NULL;
		
		hShellWnd = GetShellWindow();
		if (hShellWnd != NULL)
		{
			DWORD shellPid = 0;
			if (GetWindowThreadProcessId(hShellWnd, &shellPid))
			{
				HANDLE hShellProc = NULL;
				hShellProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, shellPid);
				if (hShellProc != NULL)
				{
					HANDLE hShellProcToken = NULL;
					if (OpenProcessToken(hShellProc , TOKEN_DUPLICATE, &hShellProcToken))
					{
						// Duplicate the shell's process token to get a primary token.
						// Based on experimentation, this is the minimal set of rights required for CreateProcessWithTokenW (contrary to current documentation).
						const DWORD dwTokenRights = TOKEN_QUERY | TOKEN_ASSIGN_PRIMARY | TOKEN_DUPLICATE | TOKEN_ADJUST_DEFAULT | TOKEN_ADJUST_SESSIONID;
						HANDLE hPrimaryToken = NULL;
						if (DuplicateTokenEx(hShellProcToken, dwTokenRights, NULL, SecurityImpersonation, TokenPrimary, &hPrimaryToken))
						{
							PROCESS_INFORMATION   pi      = {0};
							STARTUPINFOW          si   = {0};

							SecureZeroMemory(&si, sizeof(si));
							SecureZeroMemory(&pi, sizeof(pi));
							si.cb = sizeof(si);

							_BuildCommandLineArguments(&fCmdLineOptions);
							const UniChar* workDir = NULL;
							if(fWorkingDirectory.IsFolder())
							{
								workDir = fWorkingDirectory.GetPath().GetCPointer();
							}

							if (CreateProcessWithTokenW(hPrimaryToken,0,
								fProgramPath.GetPath().GetCPointer(),
								fCmdLineOptions,
								0,NULL,workDir,&si,&pi))
							{
								if (fWaitTermination)
								{
									WaitForSingleObject( pi.hProcess, INFINITE );
									if (outReturnCode)
									{
										DWORD xx;
										GetExitCodeProcess( pi.hProcess,&xx);
										*outReturnCode = xx;
									}
								}
								CloseHandle(pi.hThread);
								CloseHandle(pi.hProcess);
								ok = true;
							}
							else
							{
								vThrowNativeError(GetLastError());
							}
							CloseHandle(hPrimaryToken);
						}
						else
						{
							vThrowNativeError(GetLastError());
						}
						CloseHandle(hShellProcToken);
					}
					else
					{
						vThrowNativeError(GetLastError());
					}
					CloseHandle(hShellProc);
				}
				else
				{
					vThrowNativeError(GetLastError());
				}
			}
			else
			{
				vThrowNativeError(GetLastError());
			}
		}
	}
	outError = errContext.GetLastError();
	return ok;
}

void XWinShellLaunchHelper::GetCommandLine(XBOX::VString& outCmdLine)const
{
	XBOX::VString args;
	outCmdLine.Clear();
	outCmdLine.AppendPrintf("\"%S\"",&(fProgramPath.GetPath()));
	_BuildCommandLineArguments(args);
	if(!args.IsEmpty())
		outCmdLine.AppendPrintf(" %S",&args);
}

bool XWinShellLaunchHelper::Start(XBOX::VError* outError,sLONG* outReturnCode)const
{
	bool ok = false;

	if (outError)
		*outError = XBOX::VE_INVALID_PARAMETER;

	if(testAssert(fProgramPath.IsFile()))
	{
		SHELLEXECUTEINFOW info={0};				
		XBOX::VString params;

		info.cbSize = sizeof(info);
		if (fWaitTermination)
		{
			info.fMask = SEE_MASK_NOCLOSEPROCESS ;
		}
		else
		{
			info.fMask = 0;
		}
		info.hwnd = NULL;
		switch (fLaunchMode)
		{
			case XWinShellLaunchHelper::LM_Elevated:
			info.lpVerb = L"runas";
			break;
			case XWinShellLaunchHelper::LM_Normal:
			default:
				info.lpVerb = L"open";
			break;
		}
		info.lpFile = fProgramPath.GetPath().GetCPointer();

		_BuildCommandLineArguments(params);
	
		if (params.IsEmpty())
		{
			info.lpParameters = NULL;
		}
		else
		{
			info.lpParameters = params.GetCPointer();
		}
		XBOX::VFilePath wdir;
		if(fWorkingDirectory.IsEmpty())
		{
			info.lpDirectory = NULL;
		}
		else if(testAssert(fWorkingDirectory.IsFolder()))
		{
			XBOX::VString name;
			wdir = fWorkingDirectory;
			wdir.GetFolderName(name,true);
			wdir.ToParent();
			wdir.ToSubFile(name);
			info.lpDirectory = wdir.GetPath().GetCPointer();
		}
	
		if(fShowWindow)
		{
			info.nShow = SW_NORMAL;
		}
		else
			info.nShow = SW_HIDE;

		::ShellExecuteExW(&info);
		DWORD err = GetLastError();
		if(err != 0)
		{
			if (outError != NULL)
				*outError = MAKE_NATIVE_VERROR(err);
			ok = false;
		}
		else
		{
			err = 0;
			if (fWaitTermination)
			{
				WaitForSingleObject( info.hProcess, INFINITE );
				GetExitCodeProcess( info.hProcess,&err );
			}
			if(info.hProcess != 0)
			{
				CloseHandle(info.hProcess);
			}

			if(outReturnCode != NULL)
			{
				*outReturnCode = err;
			}
			if (outError != NULL)
				*outError = XBOX::VE_OK;
			ok = true;
		}
	}
	return ok;
}

void XWinShellLaunchHelper::_BuildCommandLineArguments(wchar_t** ioArgs)const
{
	if(*ioArgs)
	{
		delete [] (*ioArgs);
	}
	*ioArgs = NULL;
	XBOX::VString args;
	_BuildCommandLineArguments(args);
	if(!args.IsEmpty())
	{
		VSize strSize = sizeof(wchar_t) * (args.GetLength());

		*ioArgs = (wchar_t*)VMemory::NewPtrClear((strSize + sizeof(wchar_t)),'UP4D');
		
		if((*ioArgs)!= NULL) 
		{
			CopyBlock(args.GetCPointer(),*ioArgs,strSize);
		}
	}
}

void XWinShellLaunchHelper::_BuildCommandLineArguments(XBOX::VString& ioArgs)const
{
	ioArgs.Clear();
	std::vector<XBOX::VString>::const_iterator it  = fArgs.begin();
	if(it != fArgs.end())
	{
		const XBOX::VString& temp = *it;
		ioArgs.AppendPrintf("%S",&temp);
		++it;
	}
	for(;it != fArgs.end();++it)
	{
		ioArgs.AppendPrintf(" %S",&(*it));
	}
	if(!ioArgs.IsEmpty())
		ioArgs.TrimeSpaces();
}

bool XWinShellLaunchHelper::_AdjustPrivileges()
{
	StErrorContextInstaller errContext;
	//Current process must actually be run with elevation because we will modify 
	//thread token with advanced privileges.
	//The whole purpose of this method is to create a process using the shell's (file explorer) security token
	//Taken from http://blogs.msdn.com/b/aaron_margosis/archive/2009/06/06/faq-how-do-i-start-a-program-as-the-desktop-user-from-an-elevated-app.aspx
	if (ImpersonateSelf(SecurityImpersonation))
	{
		HANDLE hThreadHandle = NULL;

		hThreadHandle = OpenThread(THREAD_ALL_ACCESS, FALSE, GetCurrentThreadId());
		if(hThreadHandle != NULL)
		{
			HANDLE hThreadToken = NULL;
			if (OpenThreadToken(hThreadHandle, TOKEN_ADJUST_PRIVILEGES,FALSE, &hThreadToken))
			{
				//adjust privileges on the token
				TOKEN_PRIVILEGES tkp;
				tkp.PrivilegeCount = 1;
				LookupPrivilegeValue(NULL, SE_INCREASE_QUOTA_NAME, &tkp.Privileges[0].Luid);
				tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
				if (!AdjustTokenPrivileges(hThreadToken, FALSE, &tkp, 0, NULL, NULL))
				{
					vThrowNativeError(GetLastError());
				}
			}
			else
			{
				vThrowNativeError(GetLastError());
			}
			if(hThreadToken != NULL)
				CloseHandle(hThreadToken);
		}
		else
		{
			vThrowNativeError(GetLastError());
		}
		if(hThreadHandle != NULL)
			CloseHandle(hThreadHandle);
	}
	else
	{
		vThrowNativeError(GetLastError());
	}
	return (errContext.GetLastError() == XBOX::VE_OK);
}

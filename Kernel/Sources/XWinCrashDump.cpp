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

#include "VKernelPrecompiled.h"
#include "XWinCrashDump.h"
#include <tchar.h>
#include "dbghelp.h"

typedef BOOL (*MiniDumpWriteDumpPtr)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType, PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, PMINIDUMP_CALLBACK_INFORMATION CallbackParam );
void DumpMiniDump(HANDLE hFile, PEXCEPTION_POINTERS excpInfo)
{
	if ( excpInfo )
	{
		MINIDUMP_EXCEPTION_INFORMATION eInfo;
		eInfo.ClientPointers = false;
		eInfo.ExceptionPointers = excpInfo;
		eInfo.ThreadId = GetCurrentThreadId();

		MiniDumpWriteDumpPtr l_ptr;

		l_ptr = (MiniDumpWriteDumpPtr) ::GetProcAddress(GetModuleHandle("DbgHelp.dll"),"MiniDumpWriteDump");
		if (l_ptr)
		{
#if WITH_ASSERT
			l_ptr( GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpWithFullMemory, &eInfo, NULL, NULL);
#else
			l_ptr( GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &eInfo, NULL, NULL);
#endif
		}
	}
}

int SaveMiniDump(PEXCEPTION_POINTERS pExceptPtrs)
{
	TCHAR fileName[4096+4];
	strcpy(fileName,"\\\\?\\"); 

	fileName[sizeof(fileName)-5]=0;
	GetModuleFileName(0, fileName+4, sizeof(fileName)-4-4 );

	if (testAssert(!fileName[sizeof(fileName)-5]))
	{
		lstrcat(fileName+4,".dmp");
		HANDLE hFile = CreateFile( strlen(fileName+4)>MAX_PATH?fileName:fileName+4, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			DumpMiniDump(hFile, pExceptPtrs);
			CloseHandle(hFile);
		}
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

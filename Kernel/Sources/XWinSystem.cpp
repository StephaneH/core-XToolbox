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
#include "VString.h"
#include "VFilePath.h"
#include "VFile.h"
#include "XWinSystem.h"

#include <pdhmsg.h>
#include <pdh.h>
#include <imagehlp.h>
#include <psapi.h>
#include <tlhelp32.h>

#define BUFFER_SIZE	2048

class VDisplayNotificationParam : public VObject
{
public:
	VDisplayNotificationParam( const VString& inTitle, const VString& inMessage, UINT inFlags, HWND inParent)
		: fTitle( inTitle), fMessage( inMessage), fFlags( inFlags), fParent( inParent)	{}

	VString	fTitle;
	VString	fMessage;
	UINT	fFlags;
	HWND	fParent;
};


static DWORD WINAPI DisplayNotificationProc( LPVOID inParam)
{
	VDisplayNotificationParam *param = reinterpret_cast<VDisplayNotificationParam*>( inParam);

	int r = ::MessageBoxExW( param->fParent, param->fMessage.GetCPointer(), param->fTitle.GetCPointer(), param->fFlags, 0);
	
	delete param;
	return 1;
}


bool XWinSystem::DisplayNotification( const VString& inTitle, const VString& inMessage, EDisplayNotificationOptions inOptions, ENotificationResponse *outResponse)
{
	bool ok;
	
	UINT flags = MB_TASKMODAL | MB_SERVICE_NOTIFICATION;
	flags |= (inOptions & EDN_StyleCritical) ? MB_ICONWARNING : MB_ICONINFORMATION;

	switch( inOptions & EDN_LayoutMask)
	{
		case EDN_OK:
			flags |= MB_OK;
			break;
		
		case EDN_OK_Cancel:
			flags |= MB_OKCANCEL;
			break;
		
		case EDN_Yes_No:
			flags |= MB_YESNO;
			break;
		
		case EDN_Yes_No_Cancel:
			flags |= MB_YESNOCANCEL;
			break;
		
		case EDN_Abort_Retry_Ignore:
			flags |= MB_ABORTRETRYIGNORE;
			break;
	}

	switch( inOptions & EDN_DefaultMask)
	{
		case EDN_Default1:
			flags |= MB_DEFBUTTON1;
			break;

		case EDN_Default2:
			flags |= MB_DEFBUTTON2;
			break;

		case EDN_Default3:
			flags |= MB_DEFBUTTON3;
			break;
	}
	
	if (outResponse == NULL)
	{
		/*
			we need a modeless asynchronous message.
			so we create a thread.
		*/

		// MB_SERVICE_NOTIFICATION est adapte a notre cas (face-less app) mais donne un look Win95 tres moche, et surtout ca serialise les messages !!


		VDisplayNotificationParam *param = new VDisplayNotificationParam( inTitle, inMessage, flags, NULL);
		
		DWORD threadID;
		HANDLE thread = ::CreateThread( NULL, 0, DisplayNotificationProc, param, 0, &threadID);

		ok = (thread != NULL);

		if (thread != NULL)
			CloseHandle( thread);
	}
	else
	{
		int r = ::MessageBoxExW( NULL, inMessage.GetCPointer(), inTitle.GetCPointer(), flags, 0);
		ok = r != 0;
		switch( r)
		{
			case IDYES:			*outResponse = ERN_Yes; break;
			case IDNO:			*outResponse = ERN_No; break;
			case IDRETRY:		*outResponse = ERN_Retry; break;
			case IDIGNORE:		*outResponse = ERN_Ignore; break;
			case IDABORT:		*outResponse = ERN_Abort; break;
			case IDOK:			*outResponse = ERN_OK; break;
			case IDCANCEL:
			default:			*outResponse = ERN_Cancel; break;
		}
	}

	return ok;
}


bool XWinSystem::GUIDisplayNotification( HWND inParent, const VString& inTitle, const VString& inMessage, EDisplayNotificationOptions inOptions)
{
	UINT flags = MB_OK | MB_TASKMODAL;
	flags |= (inOptions & EDN_StyleCritical) ? MB_ICONWARNING : MB_ICONINFORMATION;

	VDisplayNotificationParam *param = new VDisplayNotificationParam( inTitle, inMessage, flags, inParent);
	
	DWORD threadID;
	HANDLE thread = ::CreateThread( NULL, 0, DisplayNotificationProc, param, 0, &threadID);

	bool ok = (thread != NULL);

	if (thread != NULL)
		CloseHandle( thread);

	return ok;
}


sLONG8 XWinSystem::GetApplicationPhysicalMemSize()
{
	sLONG8					result = 0;
	PROCESS_MEMORY_COUNTERS	info;
		
	::memset( &info, 0, sizeof( info));
	if (GetProcessMemoryInfo( ::GetCurrentProcess(), &info, sizeof( info)))
	{
		result = (sLONG8) info.WorkingSetSize;
	}
	
	return result;
}


SystemVersion XWinSystem::GetSystemVersion()
{
		static SystemVersion sWinVersion = WIN_UNKNOWN;
	if (sWinVersion == WIN_UNKNOWN)
	{
		OSVERSIONINFO vers;
		OSVERSIONINFOEX OSversion;
		DWORDLONG dwlConditionMask = 0;

		vers.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

		VER_SET_CONDITION( dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL );
		VER_SET_CONDITION( dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL );
		// Initialize the OSVERSIONINFOEX structure.
		ZeroMemory(&OSversion, sizeof(OSVERSIONINFOEX));
		OSversion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
		
		// set Windows 8.1
		OSversion.dwMajorVersion = 6;
		OSversion.dwMinorVersion = 3;
		if ( VerifyVersionInfo(&OSversion, VER_MAJORVERSION | VER_MINORVERSION , dwlConditionMask) )
		{
			sWinVersion = WIN_EIGHT_ONE;
			
			OSversion.wProductType = VER_NT_SERVER;

			VER_SET_CONDITION( dwlConditionMask, VER_PRODUCT_TYPE, VER_GREATER_EQUAL );
			if(VerifyVersionInfo(&OSversion, VER_MAJORVERSION | VER_MINORVERSION | VER_PRODUCT_TYPE, dwlConditionMask))
			{
				sWinVersion = WIN_SERVER_2012R2;
			}
		}
		else
		{
			// set Windows 8
			OSversion.dwMajorVersion = 6;
			OSversion.dwMinorVersion = 2;
			if ( VerifyVersionInfo(&OSversion, VER_MAJORVERSION | VER_MINORVERSION , dwlConditionMask) )
			{
				sWinVersion = WIN_EIGHT;
					
				if(::GetVersionEx((LPOSVERSIONINFO)&OSversion))
				{
					if(OSversion.wProductType!=VER_NT_WORKSTATION)
						sWinVersion = WIN_SERVER_2012;
				}
			}
			else
			{
				// set Windows 7
				OSversion.dwMajorVersion = 6;
				OSversion.dwMinorVersion = 1;
				if ( VerifyVersionInfo(&OSversion, VER_MAJORVERSION | VER_MINORVERSION , dwlConditionMask) )
				{
					sWinVersion = WIN_SEVEN;
					
					if(::GetVersionEx((LPOSVERSIONINFO)&OSversion))
					{
						if(OSversion.wProductType!=VER_NT_WORKSTATION)
							sWinVersion = WIN_SERVER_2008R2;
					}
				}
				else
				{
					// set Windows Vista Version
					OSversion.dwMajorVersion = 6;
					OSversion.dwMinorVersion = 0;
					if ( VerifyVersionInfo(&OSversion, VER_MAJORVERSION | VER_MINORVERSION , dwlConditionMask) )
					{
						sWinVersion = WIN_VISTA;
						
						if(::GetVersionEx((LPOSVERSIONINFO)&OSversion))
						{
							if(OSversion.wProductType!=VER_NT_WORKSTATION)
								sWinVersion = WIN_SERVER_2008;
						}
					}
					else
					{
						// set Windows 2003 Server
						OSversion.dwMajorVersion = 5;
						OSversion.dwMinorVersion = 2;
						if ( VerifyVersionInfo(&OSversion, VER_MAJORVERSION | VER_MINORVERSION , dwlConditionMask) )
							sWinVersion = WIN_SERVER_2003;
						else
						{
							// set Windows 2003 Server
							OSversion.dwMajorVersion = 5;
							OSversion.dwMinorVersion = 1;
							if ( VerifyVersionInfo(&OSversion, VER_MAJORVERSION | VER_MINORVERSION , dwlConditionMask) )
								sWinVersion = WIN_XP;
						}
					}
				}
			}
		}
		if(sWinVersion == WIN_UNKNOWN)
		{
			OSVERSIONINFO vers;
			vers.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

			if (::GetVersionEx(&vers))
			{
				switch(vers.dwPlatformId)
				{
					case VER_PLATFORM_WIN32_NT:
						switch(vers.dwMajorVersion)
						{
						case 5:
							if(vers.dwMinorVersion==1)
								sWinVersion = WIN_XP;
							else if(vers.dwMinorVersion==2)
								sWinVersion = WIN_SERVER_2003;
							break;
						case 6:
							if(vers.dwMinorVersion==0)
								sWinVersion = WIN_VISTA;
							else if(vers.dwMinorVersion==1)
								sWinVersion = WIN_SEVEN;
							break;
						}
						break;
				}
			}
		}
	}
	xbox_assert(sWinVersion != WIN_UNKNOWN);
	return sWinVersion;
}


bool XWinSystem::GetMajorAndMinorFromSystemVersion( SystemVersion inSystemVersion, DWORD *outMajor, DWORD *outMinor)
{
	bool ok = true;
	switch( inSystemVersion)
	{
	case WIN_XP:			*outMajor = 5; *outMinor = 1; break;
	case WIN_SERVER_2003:	*outMajor = 5; *outMinor = 2; break;
	case WIN_VISTA:			*outMajor = 6; *outMinor = 0; break;
	case WIN_SERVER_2008:	*outMajor = 6; *outMinor = 0; break;
	case WIN_SERVER_2008R2:	*outMajor = 6; *outMinor = 1; break;
	case WIN_SEVEN:			*outMajor = 6; *outMinor = 1; break;
	case WIN_EIGHT:			*outMajor = 6; *outMinor = 2; break;
	case WIN_SERVER_2012:	*outMajor = 6; *outMinor = 2; break;
	case WIN_EIGHT_ONE:		*outMajor = 6; *outMinor = 3; break;
	case WIN_SERVER_2012R2:	*outMajor = 6; *outMinor = 3; break;
	default:
		xbox_assert( false);
		ok = false;
	}
	return ok;
}


bool XWinSystem::IsSystemVersionOrAbove( SystemVersion inSystemVersion)
{
	OSVERSIONINFOEXW version;

	::memset(&version, 0, sizeof(version));
	version.dwOSVersionInfoSize = sizeof(version);
	
	DWORDLONG conditionMask = 0;
	VER_SET_CONDITION( conditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL );
	VER_SET_CONDITION( conditionMask, VER_MINORVERSION, VER_GREATER_EQUAL );

	if (!GetMajorAndMinorFromSystemVersion( inSystemVersion, &version.dwMajorVersion, &version.dwMinorVersion))
		return false;

	return ::VerifyVersionInfoW( &version, VER_MAJORVERSION | VER_MINORVERSION , conditionMask) != 0;
}

void XWinSystem::GetProcessList(std::vector<pid> &pids)
{
	pids.clear();

	// It is a good idea to use a large array, because it is hard to predict how
	// many processes there will be at the time you call EnumProcesses. 

	pid		processIds[2048];
	DWORD	bytesReturned;

	if (EnumProcesses(processIds, 2048 * sizeof(pid), &bytesReturned))
	{
		for (unsigned int i = 0; i < (bytesReturned / sizeof(pid)); ++i)
		{
			pids.push_back(processIds[i]);
		}
	}
}

void XWinSystem::GetProcessName(uLONG processID, VString &outProcessName)
{
	GetProcessPath(processID, outProcessName);
	if (!outProcessName.IsEmpty()) {

		VIndex	index;

		index = outProcessName.FindUniChar('\\', outProcessName.GetLength(), true);
		if (index != 0) {

			index++;
			outProcessName.SubString(index, outProcessName.GetLength() - index + 1);

		}

	}
}

void XWinSystem::GetProcessPath(uLONG processID, VString &outProcessPath)
{
	outProcessPath.Clear();

	HANDLE	handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);

	if (handle != NULL)
	{
		// GetProcessImageFileName() (psapi.h) will always correctly retrieve the path of the process,
		// even if running from a 32-bit process and trying to query a 64-bit process (in which cas 
		// GetModuleFileNameEx() will fail). However, GetProcessImageFileName() returns paths in device
		// format (example "\Device\Harddisk0\Partition1\Windows\System32\Ctype.nls").

		UniChar	buffer[BUFFER_SIZE];
		VSize	size = GetModuleFileNameExW(handle, NULL, buffer, BUFFER_SIZE);
	
		if (size != 0)
		{
			outProcessPath.FromBlock(buffer, 2 * size, VTC_UTF_16);	// Size has been returned in number of UniChar.
		}
		CloseHandle(handle);
	}	
}

bool XWinSystem::KillProcess(uLONG inProcessID)
{
	bool result = false;

	HANDLE	hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, inProcessID );

	if ( NULL != hProcess )
	{
		TerminateProcess(hProcess, 0);
		CloseHandle(hProcess);
		result = true;
	}

	return result;
}


bool XWinSystem::SearchExecutablePath( const VString& inExecutableName, VFilePath& outPath)
{
	bool found = false;

	VString executableName( inExecutableName);
	if (!executableName.EndsWith( CVSTR( ".exe")))
		executableName += CVSTR( ".exe");

	// first look in PATH environnment variable
	// "An environment variable has a maximum size limit of 32,767 characters, including the null-terminating character."
	WCHAR *pathBuffer = new WCHAR[32768];
	if (pathBuffer != NULL)
	{
		DWORD rc = GetEnvironmentVariableW( L"PATH", pathBuffer, 32768);
		if (rc > 0)
		{
			// check each individual folder
			VString pathsString( pathBuffer);
			VectorOfVString paths;
			pathsString.GetSubStrings( ';', paths, false /* returnEmptyStrings */, true /* trimSpaces */);
			for( VectorOfVString::const_iterator i = paths.begin() ; (i != paths.end()) && !found; ++i)
			{
				VString path( *i);
				if (path[path.GetLength()-1] != '\\')
					path += (UniChar) '\\';
				path += executableName;
				VFile file( path, FPS_SYSTEM);
				if (file.Exists())
				{
					outPath = file.GetPath();
					found = true;
				}
			}
		}
		delete[] pathBuffer;
	}

	if (!found)
	{
		// look in registry at HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths
		VString keyPath( L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\");
		keyPath += executableName;

		HKEY appPathKey;
		LONG error = RegOpenKeyExW( HKEY_LOCAL_MACHINE, keyPath.GetCPointer(), 0, KEY_QUERY_VALUE, &appPathKey);

		if (error == ERROR_SUCCESS)
		{
			DWORD valueType;
			WCHAR value[4096];
			DWORD valueSize = sizeof( value);
			error = RegQueryValueExW( appPathKey, NULL, NULL, &valueType, (BYTE*) value, &valueSize);
			if (testAssert( error == ERROR_SUCCESS))
			{
				if (testAssert( valueType == REG_SZ || valueType == REG_EXPAND_SZ))
				{
					if (valueType == REG_EXPAND_SZ)
					{
						WCHAR valueExpanded[4096];
						DWORD count = ExpandEnvironmentStringsW( value, valueExpanded, sizeof( valueExpanded));
						if ( testAssert( (count > 0) && (count <= sizeof( valueExpanded)) ))
						{
							::memcpy( value, valueExpanded, count * sizeof( WCHAR));
						}
					}

					// expand 8.3 short path names
					const WCHAR *prefix = L"\\\\?\\";	// necessary to remove the 260 chars limit
					size_t lenValue = wcslen( value);
					size_t lenPrefix = wcslen( prefix);
					if (testAssert( (lenValue + lenPrefix) * sizeof( WCHAR) < sizeof( value)))
					{
						memmove( value + lenPrefix, value, (1 + lenValue) * sizeof( WCHAR));
						memmove( value, prefix,  lenPrefix * sizeof( WCHAR));
						DWORD rc =  GetLongPathNameW( value, value, sizeof( value));
						// remove prefix
						if (testAssert( wcsncmp ( value, prefix, lenPrefix) == 0))
						{
							memmove( value, value + lenPrefix, (wcslen( value + lenPrefix) + 1) * sizeof( WCHAR));
						}
					}

					outPath.FromFullPath( VString( value), FPS_SYSTEM);
					found = true;
				}
			}
			RegCloseKey( appPathKey);
		}
	}

	return found;
}


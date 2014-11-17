/*
 *  File:       WStackCrawl.cpp
 *  Summary:   	Cross platform stack crawl class.
 *  Written by: Jesse Jones
 *
 *  Copyright © 1998-1999 Jesse Jones. 
 *	For conditions of distribution and use, see copyright notice in XTypes.h  
 *
 *  Change History (most recent first):	
 *
 *		 <3>	 5/27/99	JDJ		Symbol info buffer is no longer static.
 *		 <2>	 4/18/99	JDJ		GetLogicalAddress checks for NULL module handle.
 *									GetFrame checks GetLogicalAddress return flag.
 *		 <1>	10/05/98	JDJ		Created
 */

#include "VKernelPrecompiled.h"
#include "VWinStackCrawl.h"
#include "VString.h"
#include "VProcess.h"

#pragma pack(push,__before,8)
#include <imagehlp.h>
#pragma pack(pop,__before)

CRITICAL_SECTION sCriticalSection;	// debughelp functions are not thread safe

// ===================================================================================
//	Internal Functions
// ===================================================================================

//---------------------------------------------------------------
//
// GetLogicalAddress
// 
// From Microsoft Systems Journal, "Under the Hood", May edition.
//
//---------------------------------------------------------------
static bool GetLogicalAddress(const void* addr, char* moduleName, uLONG len, uLONG& section, uLONG& offset )
{
    MEMORY_BASIC_INFORMATION memoryInfo;

    if (!VirtualQuery(addr, &memoryInfo, sizeof(memoryInfo)))
        return FALSE;

    void *moduleH = memoryInfo.AllocationBase;
    if (moduleH == 0)
    	return false;

    if (!GetModuleFileNameA((HMODULE) moduleH, moduleName, len))
        return false;

    // Point to the DOS header in memory
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER) moduleH;

    // From the DOS header, find the NT (PE) header
    PIMAGE_NT_HEADERS peHeader = (PIMAGE_NT_HEADERS) ( (char*) moduleH + dosHeader->e_lfanew);

    PIMAGE_SECTION_HEADER sectionPtr = IMAGE_FIRST_SECTION(peHeader);

    uLONG rva = (char*) addr - (char*) moduleH; // RVA is offset from module load address

    // Iterate through the section table, looking for the one that encompasses
    // the linear address.
    for (uLONG i = 0; i < peHeader->FileHeader.NumberOfSections; i++, sectionPtr++) {
        uLONG sectionStart = sectionPtr->VirtualAddress;
        uLONG sectionEnd = sectionStart + Max(sectionPtr->SizeOfRawData, sectionPtr->Misc.VirtualSize);

        // Is the address in this section???
        if ((rva >= sectionStart) && (rva <= sectionEnd)) {
            // Yes, address is in the section.  Calculate section and offset,
            // and store in the "section" & "offset" params, which were
            // passed by reference.
            section = i+1;
            offset = rva - sectionStart;
            return TRUE;
        }
    }
    
    //DEBUGSTR(L"Should never get here!");

    return false;  
}



bool XWinStackCrawl::operator < (const XWinStackCrawl& other) const
{
	bool res;
	if (fNumFrames == other.fNumFrames)
	{
		res = false;
		for (int i = 0; i < fNumFrames; ++i)
		{
			if (fFrame[i] < other.fFrame[i])
			{
				res = true;
				break;
			}
		}
	}
	else
	{
		if (fNumFrames <= 0 && other.fNumFrames <= 0)
			res = false;
		else
			res = (fNumFrames < other.fNumFrames);
	}
	return res;
}


//---------------------------------------------------------------
//
// XWinStackCrawl::XWinStackCrawl
//
//---------------------------------------------------------------

typedef USHORT (WINAPI *CaptureStackBackTraceProc)(ULONG, ULONG, PVOID*, PULONG);

void XWinStackCrawl::LoadFrames(uLONG inStartFrame, uLONG inNumFrames)
{
	xbox_assert(inNumFrames > 0);

	if (inNumFrames > kMaxScrawlFrames)
		inNumFrames = kMaxScrawlFrames;

	EnterCriticalSection( &sCriticalSection);

	static CaptureStackBackTraceProc sCaptureStackBackTraceProcPtr = (CaptureStackBackTraceProc) ::GetProcAddress( ::GetModuleHandle("ntdll.dll"), "RtlCaptureStackBackTrace");  
    
    if (sCaptureStackBackTraceProcPtr != NULL)
    {
		fNumFrames = (*sCaptureStackBackTraceProcPtr)( inStartFrame, inNumFrames, (PVOID*) &fFrame[0], NULL);
	}
	else
	{
		
		fNumFrames = 0;

		STACKFRAME64 stackFrame = {0};
		CONTEXT context = {0};

	#if ARCH_32
		DWORD machine = IMAGE_FILE_MACHINE_I386;
		__asm {
			start:
			lea ebx, stackFrame   
			lea	eax, start						// $$$ Presumbably there's a simpler way to get the program counter...
			mov	context.Eip, eax				
			mov	context.Ebp, ebp			
			mov	context.Esp, esp
		}
		stackFrame.AddrPC.Offset = context.Eip;
		stackFrame.AddrPC.Mode    = AddrModeFlat; 
		stackFrame.AddrFrame.Offset = context.Ebp;
		stackFrame.AddrFrame.Mode = AddrModeFlat;
		stackFrame.AddrStack.Offset = context.Esp;
		stackFrame.AddrStack.Mode = AddrModeFlat; 
	#elif ARCH_64
		DWORD machine = IMAGE_FILE_MACHINE_AMD64;
		RtlCaptureContext( &context);	// doesn't work for 32bits!
		stackFrame.AddrPC.Offset = context.Rip;
		stackFrame.AddrPC.Mode    = AddrModeFlat; 
		stackFrame.AddrFrame.Offset = context.Rsp;
		stackFrame.AddrFrame.Mode = AddrModeFlat;
		stackFrame.AddrStack.Offset = context.Rsp;
		stackFrame.AddrStack.Mode = AddrModeFlat; 
	#endif 

		++inStartFrame;							// first frame should be our caller so skip the first frame

		sLONG succeeded = true;
		HANDLE curProcess = ::GetCurrentProcess();
		HANDLE curThread = ::GetCurrentThread();
		for (uLONG i = 0; i < inStartFrame + inNumFrames && succeeded; i++)
		{
			succeeded = ::StackWalk64( machine,	// machine type
				  curProcess,		// process handle
				  curThread,		// thread handle
				  &stackFrame,				// returned stack frame
				  &context,						// context frame (not needed for Intel)
				  NULL,						// ReadMemoryRoutine 
				  ::SymFunctionTableAccess64,	// FunctionTableAccessRoutine 
				  ::SymGetModuleBase64,			// GetModuleBaseRoutine
				  NULL);						// TranslateAddress 


			if ( 0 == stackFrame.AddrFrame.Offset ) // Basic sanity check to make sure
				break;                      // the frame is OK.  Bail if not.
			
			if (succeeded && i >= inStartFrame)
			{
				fFrame[fNumFrames++] = (char *) 0 + stackFrame.AddrPC.Offset;
			}
		}
	}

	LeaveCriticalSection( &sCriticalSection);
}



//---------------------------------------------------------------
//
// XWinStackCrawl::GetFrame (uLONG)
//
//---------------------------------------------------------------


void XWinStackCrawl::Dump( FILE *inFile) const
{
	if (inFile != NULL && fNumFrames > 0)
	{
		for (sLONG index = fNumFrames - 1; index >= 0; index--)
		{
			SStackFrame frame;
			ReadFrame( fFrame[index], frame);

			::fprintf( inFile, "%s + 0x%lX\n", frame.name, frame.offset);
		}
	}
}


void XWinStackCrawl::Dump( VString& ioString) const
{
	for (sLONG index = fNumFrames - 1; index >= 0; index--)
	{
		SStackFrame frame;
		ReadFrame( fFrame[index], frame);

		ioString.AppendPrintf( "  %s\n", frame.name);
	}
}

void XWinStackCrawl::RegisterUser( void *inInstance)
{
	EnterCriticalSection( &sCriticalSection);
	HANDLE processH = ::GetCurrentProcess();
	char modulePath[4096];
	char searchPath[4096+4];
	strcpy(searchPath,"\\\\?\\"); 

	// Get the module file name.
	modulePath[sizeof(modulePath)-2]=0;
	::GetModuleFileNameA( (HINSTANCE) inInstance, modulePath, sizeof(modulePath)-4);

	if testAssert(0==modulePath[sizeof(modulePath)-2])
	{
		::OutputDebugString( modulePath);
		uLONG_PTR baseAddr = ::SymLoadModule( processH, NULL, modulePath, NULL, (uLONG_PTR) inInstance, 0);
		if (baseAddr == 0)
			::OutputDebugString( "SymLoadModule failed");


		// skip app name
		char* p = modulePath + ::strlen(modulePath);
		while( *--p != '\\')
			;
		*p = 0;
		searchPath[sizeof(searchPath)-2]=0;
		BOOL isOK = ::SymGetSearchPath( processH, searchPath+4, sizeof(searchPath)-5);

		if (testAssert(0==searchPath[sizeof(searchPath)-2])
			&& testAssert((strlen(searchPath+4)+strlen(modulePath)+2)<=(sizeof(searchPath)-5)))
		{
			// Verify if the module path exists in the search path.
			size_t pos=0;
			size_t length=strlen(searchPath);
			for (size_t i=4;i<length;i++)
			{
				if (modulePath[pos++]!=searchPath[i])
				{
					// Verify if found.
					if ((modulePath[pos-1]==0) && (searchPath[i]==';'))
						break;

					// Search for the next ';'
					pos=0;
					while ((i<length) && (searchPath[i]!=';'))
						i++;
				}
			}

			// Verify if found
			if (!pos)
			{
				// Not found: Append to search path
				::strcat( searchPath+4, ";");
				::strcat( searchPath+4, modulePath);
				::SymSetSearchPath( processH, strlen(searchPath)>MAX_PATH?searchPath:searchPath+4);
			}
		}
		else
			::OutputDebugString( "Mem overflow on search path concatenation.");
	}
	else
		::OutputDebugString( "Mem overflow on module path.");
	LeaveCriticalSection( &sCriticalSection);
}


void XWinStackCrawl::UnregisterUser( void *inInstance)
{
	//BOOL isOK = ::SymUnloadModule( ::GetCurrentProcess(), (DWORD) inInstance);
}

void XWinStackCrawl::Init()
{
	InitializeCriticalSectionAndSpinCount( &sCriticalSection, 3000);

	::SymSetOptions(SYMOPT_UNDNAME | SYMOPT_OMAP_FIND_NEAREST /*| SYMOPT_DEFERRED_LOADS*/ | SYMOPT_LOAD_LINES);

	HANDLE processH = ::GetCurrentProcess();

	sLONG succeeded = ::SymInitialize(processH, NULL, false);

	RegisterUser( VProcess::WIN_GetAppInstance());
	RegisterUser( VProcess::WIN_GetToolboxInstance());
}


void XWinStackCrawl::DeInit()
{
	::SymCleanup( ::GetCurrentProcess());
	DeleteCriticalSection( &sCriticalSection);
}



void XWinStackCrawl::ReadFrame( const void *inID, SStackFrame& outFrame) const
{
	EnterCriticalSection( &sCriticalSection);
	const uLONG kMaxSymbolName = 512;
	uBYTE buffer[sizeof(IMAGEHLP_SYMBOL64) + kMaxSymbolName];

	IMAGEHLP_SYMBOL64* symbolName = reinterpret_cast<IMAGEHLP_SYMBOL64*>(buffer);
	symbolName->SizeOfStruct  = sizeof(IMAGEHLP_SYMBOL64);
	symbolName->MaxNameLength = kMaxSymbolName;
	
	// Note that SymGetSymFromAddr only works with Win98 and NT 4 or later.
	HANDLE processH = ::GetCurrentProcess();
	
	DWORD64 offset64 = 0;
	if (::SymGetSymFromAddr64(processH, (char*) inID - (char*) 0, &offset64, symbolName))
	{
		strncpy_s( outFrame.name, 256, symbolName->Name, _TRUNCATE);
		outFrame.start  = (char *) 0 + symbolName->Address;
		outFrame.offset = (DWORD) offset64;
	}
	else
	{
		DWORD dd = ::GetLastError();
		char moduleName[4096];
		uLONG section = 0;
		uLONG offset = 0;

		// It's possible to use adr with the map file to determine the
		// function name, but it's rather awkward since you have to use
		// the "Rva+Base" column in the map file. By calling GetLogicalAddress
		// we get the address used by the first column in the map file which
		// is much easier to handle.
		if (!::GetLogicalAddress( inID, moduleName, sizeof(moduleName), section, offset))
			moduleName[0] = '\0';
			
		strcpy( outFrame.name, moduleName);
		outFrame.start  = (char *) 0 + section;
		outFrame.offset = offset;

		// strip off the path info
		char *p = &outFrame.name[0];
		sLONG lastPos = -1;
		sLONG pos = 0;
		while(*p)
		{
			if (*p == '\\')
				lastPos = pos;
			++pos;
			++p;
		}
		if (lastPos > 0)
		{
			p = (char *) &outFrame.name[0];
			memmove(p, p+lastPos+1, strlen( p+lastPos+1)+1);
		}
	}
	LeaveCriticalSection( &sCriticalSection);
}

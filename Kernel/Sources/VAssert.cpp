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
#include "VStackCrawl.h"
#include "VString.h"
#include "VProcess.h"
#include "VFile.h"
#include "VFileStream.h"
#include "VAssert.h"
#include "VTask.h"
#include "VSystem.h"
#include "VValueBag.h"
#include "ILogger.h"

#if VERSION_LINUX
#include <sys/types.h>
#include <unistd.h>
#endif


/*
	VStackCrawl::Dump() produces a stack crawl with fully qualified symbols separated by \n.
	for	ILogger, we prefer smaller one line information enough for debugging.
*/
static VString ShortenStackCrawlOneLine( const VString& inStackCrawl)
{
	VString result;

	VectorOfVString items;
	inStackCrawl.GetSubStrings( '\n', items, true /* inReturnEmptyStrings */, true /* inTrimSpaces */);

	for( VectorOfVString::iterator i = items.begin() ; i != items.end() ; ++i)
	{
		// remove function parameter list
		VString s = *i;
		VIndex posLeft = s.FindUniChar( '(');
		VIndex posRight = s.FindUniChar( ')');
		if ( (posLeft > 0) && (posRight > posLeft) )
		{
			s.Remove( posLeft, posRight - posLeft + 1);
		}

		// remove xbox:: namespaces
		s.ExchangeAll( CVSTR( "xbox::"), CVSTR(""));

		// remove some extra useless info
		s.ExchangeAll( CVSTR( "non-virtual thunk to "), CVSTR(""));

		// remove const qualifier
		VIndex pos = s.FindRawString( " const");
		if ( (pos > 0) && (pos == s.GetLength() - 5) )
			s.Remove( pos, 6);

		if (i != items.begin())
			result += " <- ";
		result += s;
	}
	return result;
}


/*
	sometimes the __FILE__ macro returns a full path.
	We only want the filename for ILogger
*/
static VString ShortenFilePath( const VString& inFilePath)
{
	#if VERSIONMAC || VERSION_LINUX
	UniChar sep = '/';
	#elif VERSIONWIN
	UniChar sep = '\\';
	#endif

	VIndex pos = inFilePath.FindUniChar( sep, inFilePath.GetLength(), true);
	if (pos <= 0)
		return inFilePath;

	VString s;
	inFilePath.GetSubString( pos + 1, inFilePath.GetLength() - pos, s);
	return s;
}


VDebuggerActionCode VNotificationDebugMessageHandler::_TranslateNotificationResponse( ENotificationResponse inResponse)
{
	VDebuggerActionCode code;
	switch( inResponse)
	{
		case ERN_Retry:
			code = DAC_Continue;
			break;

		case ERN_Ignore:
			code = DAC_Ignore;
			break;

		case ERN_Abort:
		default:
			code = VDebugMgr::Get()->IsSystemDebuggerAvailable() ? DAC_Break : DAC_Abort;
			break;
	}
	return code;
}


VDebuggerActionCode VNotificationDebugMessageHandler::AssertFailed( VDebugContext& inContext, const char *inTestString, const char *inFileName, sLONG inLineNumber)
{
	VDebuggerActionCode action;
	if (inContext.IsUIEnabled())
	{
		VString message;
		message.Printf( "Condition: %s\nFile: %s, %d", inTestString, inFileName, inLineNumber);

		ENotificationResponse response;
		VSystem::DisplayNotification( CVSTR( "Assert Failed"), message, EDN_Abort_Retry_Ignore, &response);

		action = _TranslateNotificationResponse( response);
	}
	else
	{
		action = DAC_Continue;
	}
	return action;
}


VDebuggerActionCode VNotificationDebugMessageHandler::ErrorThrown( VDebugContext& inContext, VErrorBase *inError)
{
	VDebuggerActionCode action;
	if (inContext.IsUIEnabled())
	{
		VStr255	message( "Error: ");

		VStr255 description;
		inError->DumpToString( description);
		message += description;
		message += (UniChar) '\n';

		ENotificationResponse response;
		VSystem::DisplayNotification( CVSTR( "Error Thrown"), message, EDN_Abort_Retry_Ignore, &response);

		action = _TranslateNotificationResponse( response);
	}
	else
	{
		action = DAC_Continue;
	}
	return action;
}


//======================================================================================================


VDebugMgr				VDebugMgr::sInstance;


VDebugMgr::VDebugMgr()
: fIgnoredAsserts( NULL)
{
	fDebuggerAvailable = _GetDebuggerType( NULL);

	fMessageEnabled = false;		// Disabled until Init

#if VERSIONDEBUG
	fBreakAllowed = true;
	fBreakOnAssert = false;
	fBreakOnDebugMsg = false;
	fLogInDebugger = true;
	fBreakOnError = false;

#if VERSION_LINUX
    fMessageEnabled = true;
#endif

#else
	fBreakAllowed = false;
	fBreakOnAssert = false;
	fBreakOnDebugMsg = false;
	fLogInDebugger = false;
	fBreakOnError = false;
#endif	// VERSIONDEBUG

	fIgnoreAllAsserts = false;
	fIgnoreAllErrors = false;

	// use ILogger in all configurations
	fLogInFile = false;

	fMessageHandler = NULL;

	fAssertFilePath = NULL;
	fAssertFileAlreadyOpened = false;	// assert file has already been opened once
	fAssertFileNumber = 0;
}


VDebugMgr::~VDebugMgr()
{
}


bool VDebugMgr::Init()
{
#if WITH_ASSERT
	fMessageEnabled = true;
#else
	fMessageEnabled = false;
#endif

	VStackCrawl::Init();

	return true;
}


void VDebugMgr::DeInit()
{
	ReleaseRefCountable( &fMessageHandler);
	fMessageEnabled = false;
	fLogInFile = false;
	delete [] fAssertFilePath;
}


void VDebugMgr::DebugMsg( const char* inMessage, va_list inArgList)
{
	// Messages are quiet if deactivated
	if (fMessageEnabled)
	{
		VDebugContext& context = VTask::GetCurrent()->GetDebugContext();

		// forbid recursion
		if (context.IsLowLevelMode())
		{
			// Log to debugger console
			if (fLogInDebugger && fDebuggerAvailable)
			{
				vfprintf_console( inMessage, inArgList);
			}

			if (fBreakOnDebugMsg)
				Break();
		}
		else
		{
			VStr255 text;
			text.VPrintf( inMessage, inArgList);
			DebugMsg(text);
		}
	}
}

#if WAKANDA_OUTPUT_TRACES_IN_CONSOLE
#include <iostream>
#endif
void VDebugMgr::DebugMsg( const VString& inMessage)
{
	// Messages are quiet if lowlevel or deactivated
	if (fMessageEnabled)
	{
		if (VTask::GetCurrent() != NULL)
		{
			// Avoid reentrancy
			VDebugContext& context = VTask::GetCurrent()->GetDebugContext();
			if (context.Enter())
			{
				if (!context.IsLowLevelMode())
				{
					// Log to output file
					if (fLogInFile)
					{
						WriteToAssertFile( inMessage, true, true);
					}

					// log to ILogger
					ILogger *logger = VProcess::Get()->GetLogger();//->RetainLogger();
					if ((logger != NULL) && logger->ShouldLog( EML_Debug))
					{
						// remove ending \n because line management is managed by the logger
						VString s( inMessage);
						if (!s.IsEmpty() && s[s.GetLength()-1] == '\n')
							s.Truncate( s.GetLength()-1);

						VValueBag *bag = new VValueBag;

						ILoggerBagKeys::message.Set( bag, s);
						ILoggerBagKeys::level.Set( bag, EML_Debug);

						logger->LogBag( bag);
						ReleaseRefCountable( &bag);
					}
				}

				// Log to debugger console
				if (fLogInDebugger && fDebuggerAvailable)
				{
					#if VERSIONWIN
#if WAKANDA_OUTPUT_TRACES_IN_CONSOLE
					VStringConvertBuffer buffer(inMessage, VTC_UTF_8);
					std::cout << buffer.GetCPointer() << std::endl;
#endif
					::OutputDebugStringW( inMessage.GetCPointer());
					#else
					VStringConvertBuffer buff( inMessage, VTC_StdLib_char);
					::fwrite( buff.GetCPointer(), 1, buff.GetSize(), stderr);
					#endif
				}
				context.Exit();
			}
		}
	}

	if (fBreakOnDebugMsg)
		Break();
}

VDebugMgr* VDebugMgr::Get ( )
{
	return &sInstance;
}

#if VERSION_LINUX
bool VDebugMgr::AssertFailed(const char* inTestString, const char* inFileName, sLONG inLineNumber, const char* inFunctionName)
#else
bool VDebugMgr::AssertFailed(const char* inTestString, const char* inFileName, sLONG inLineNumber)
#endif
{
	bool needBreak = fBreakOnAssert;

	if (fMessageEnabled)
	{
		// Log to debugger console, no call to toolbox since we might be in low level mode
		if (fLogInDebugger && fDebuggerAvailable)
		{
            #if VERSION_LINUX
			fprintf_console( "+%d %s # %s (%s)\n", inLineNumber, inFileName, inFunctionName, inTestString);
            #else
			fprintf_console( "Assert Failed: %s, %s, %d\n", inTestString, inFileName, inLineNumber);
            #endif
		}

		if (VTask::GetCurrent() != NULL)
		{
			// check if restricted to low level
			VDebugContext& context = VTask::GetCurrent()->GetDebugContext();
			if (!context.IsLowLevelMode())
			{
				// check recursion
				if (context.Enter())
				{

					// check if ignored
					VStr255 signature;
					signature.Printf("%s%d", inFileName, inLineNumber);
					if (fIgnoreAllAsserts || ((fIgnoredAsserts != NULL) && (fIgnoredAsserts->Find(signature) > 0)))
					{
						needBreak = false;
					}
					else
					{
						// Log to file
						if (fLogInFile)
						{
							VStr255	text;
							text.AppendPrintf("Assert Failed: %s, %s, %d\n", inTestString, inFileName, inLineNumber);
							WriteToAssertFile( text, true, true);
						}

						// log to ILogger
						ILogger *logger = VProcess::Get()->GetLogger();
						if ( (logger != NULL) && logger->ShouldLog( EML_Assert))
						{
							VValueBag *bag = new VValueBag;

							ILoggerBagKeys::level.Set( bag, EML_Assert);
							ILoggerBagKeys::file_name.Set( bag, ShortenFilePath( inFileName));
							ILoggerBagKeys::line_number.Set( bag, inLineNumber);
							ILoggerBagKeys::message.Set( bag, inTestString);

							VStackCrawl crawl;
							crawl.LoadFrames( 2, 8);
							VString stackCrawl;
							crawl.Dump( stackCrawl);
							if (!stackCrawl.IsEmpty())
							{
								ILoggerBagKeys::stack_crawl.Set( bag, ShortenStackCrawlOneLine( stackCrawl));
							}

							logger->LogBag( bag);

							ReleaseRefCountable( &bag);
						}

						if (fMessageHandler != NULL)
						{
							VDebuggerActionCode action = fMessageHandler->AssertFailed( context, inTestString, inFileName, inLineNumber);
							switch( action)
							{
								case DAC_Break:
									if (fDebuggerAvailable)
										needBreak = true;
									else
										Abort();
									break;

								case DAC_Abort:
									Abort();
									break;

								case DAC_Ignore:
									if (fIgnoredAsserts == NULL)
										fIgnoredAsserts = new VArrayString;
									fIgnoredAsserts->AppendString( signature);
									needBreak = false;
									break;

								case DAC_IgnoreAll:
									fIgnoreAllAsserts = true;
									needBreak = false;
									break;

								default:
									needBreak = false;
									break;
							}
						}
					}
					context.Exit();
				}
			}
			else
			{
				// you can put a break point here
				sLONG nothing = 0;
				nothing++;
			}
		}
	}

	return needBreak;
}


void VDebugMgr::ErrorThrown(VErrorBase* inError)
{
	// Assert allways display during lowlevel mode
	bool needBreak = fBreakOnError;

	if (fMessageEnabled && (VTask::GetCurrent() != NULL))
	{
		// check if restricted to low level
		VDebugContext& context = VTask::GetCurrent()->GetDebugContext();
		if (!context.IsLowLevelMode())
		{
			// check recursion
			if (context.Enter())
			{
				// check if ignored
				if (!fIgnoreAllErrors && (fIgnoredErrors.find( inError->GetError()) == fIgnoredErrors.end()) )
				{
					VString dump;
					inError->DumpToString(dump);

					VString	text( "Error: ");
					text += dump;
					text += (UniChar) '\n';

					// Log to file
					if (fLogInFile)
					{
						WriteToAssertFile( text, true, true);
					}

					// log to ILogger
					ILogger *logger = VProcess::Get()->GetLogger();
					if ( (logger != NULL) && logger->ShouldLog( EML_Error))
					{
						VValueBag *bag = new VValueBag;
						if (bag != NULL)
						{
							ILoggerBagKeys::level.Set( bag, EML_Error);
							inError->SaveToBag( *bag);	// put error code and parameters (needed for localized message on client side)

							VString errorDescription;
							inError->GetErrorDescription( errorDescription);	// localized description
							ILoggerBagKeys::message.Set( bag, errorDescription);

							OsType componentSignature = COMPONENT_FROM_VERROR( inError->GetError());
							if (componentSignature != 0)
								ILoggerBagKeys::component_signature.Set( bag, componentSignature);

							bool sourceIdentifierSet = false;
							const VValueBag* properties = VTask::GetCurrent()->RetainProperties();
							if (properties != NULL)
							{
								VString source;
								sourceIdentifierSet = properties->GetString( ILoggerBagKeys::source, source);
								if (sourceIdentifierSet)
									ILoggerBagKeys::source.Set( bag, source);

								VString jobID;
								if (properties->GetString( ILoggerBagKeys::jobId, jobID))
									ILoggerBagKeys::jobId.Set( bag, jobID);
							}
							ReleaseRefCountable( &properties);

							if (!sourceIdentifierSet)
								ILoggerBagKeys::source.Set( bag, VProcess::Get()->GetLogSourceIdentifier());

							VString stackCrawl;
							inError->GetStackCrawl( stackCrawl);
							if (!stackCrawl.IsEmpty())
							{
								ILoggerBagKeys::stack_crawl.Set( bag, ShortenStackCrawlOneLine( stackCrawl));
							}

							logger->LogBag( bag);
						}
						ReleaseRefCountable( &bag);
					}

					// Log to debugger console
					if (fLogInDebugger && fDebuggerAvailable)
					{
						#if VERSIONWIN
						::OutputDebugStringW( text.GetCPointer());
						#else
						VStringConvertBuffer buff( text, VTC_StdLib_char);
						::fwrite( buff.GetCPointer(), 1, buff.GetSize(), stderr);
						#endif
					}

					if (fMessageHandler != NULL)
					{
						VDebuggerActionCode action = fMessageHandler->ErrorThrown( context, inError);
						switch( action)
						{
							case DAC_Ignore:
								fIgnoredErrors.insert( inError->GetError());
								needBreak = false;
								break;

							case DAC_IgnoreAll:
								fIgnoreAllErrors = true;
								needBreak = false;
								break;

							case DAC_Break:
								if (fDebuggerAvailable)
									needBreak = true;
								else
								{
									Abort();
								}
								break;

							case DAC_Abort:
								Abort();
								break;

							default:
								needBreak = false;
								break;
						}
					}
				}
				else
				{
					needBreak = false; // ignored
				}
				context.Exit();
			}
		}
	}

	if (needBreak)
	{
		// notify the debugger that we would like to 'break' (but not assert)
		// for friendly error tracking
		// However this behavior is not welcome on linux
		#if !VERSION_LINUX
		Break();
		#endif
	}
}


void VDebugMgr::Break()
{
	VDebugContext& context = VTask::GetCurrent()->GetDebugContext();
	if (context.IsBreakEnabled())
	{
		#if VERSION_LINUX
		XBOX_BREAK_INLINE;
		#elif COMPIL_GCC && ARCH_386
		asm( "int3");
		#elif VERSIONMAC
		::Debugger();
		#elif VERSIONWIN
		::DebugBreak();
		#endif
	}
}


void VDebugMgr::Abort()
{
	::exit(0);
}


void VDebugMgr::vfprintf_console( const char *inCString, va_list inArgList)
{
	#if VERSIONMAC || VERSION_LINUX
	::vfprintf( stderr, inCString, inArgList);
	#elif VERSIONWIN
	char* buffer;
	int count=_vscprintf_p(inCString, inArgList);
	if(count>0)
	{
		buffer=(char*)malloc(count+1);
		if(buffer)
		{
			::_vsnprintf( buffer, count+1, inCString, inArgList);
			::OutputDebugStringA( buffer);
			free((void*)buffer);
		}
	}
	#endif
}


void VDebugMgr::fprintf_console( const char *inCString, ...)
{
	va_list argList;
	va_start( argList, inCString);
	vfprintf_console( inCString, argList);
	va_end(argList);
}


void VDebugMgr::SetDebugMessageHandler( IDebugMessageHandler* inDisplayer)
{
	CopyRefCountable( &fMessageHandler, inDisplayer);
}


const char *VDebugMgr::_GetAssertFilePath()
{
	if (fAssertFilePath != NULL)
	{
		if (fAssertFilePath[0] == 0)
		{
			delete [] fAssertFilePath;
			fAssertFilePath = NULL;
		}
	}

	if (fAssertFilePath == NULL)
	{
		char path[4096L];

		#if VERSIONMAC

		CFURLRef cfBundleURL = ::CFBundleCopyBundleURL( CFBundleGetMainBundle());
		CFStringRef cfPath = ::CFURLCopyFileSystemPath( cfBundleURL, kCFURLPOSIXPathStyle);
		::CFStringGetCString( cfPath, path, sizeof( path), kCFStringEncodingASCII);
		::CFRelease( cfPath);
		::CFRelease( cfBundleURL);

		#elif VERSIONWIN

		DWORD pathLength = ::GetModuleFileNameA( NULL, path, sizeof(path));
		#endif

		// remove extension (.exe on windows and .app on mac)
		for (size_t i= strlen(path)-1;i>0;i--)
		{
			if (path[i] == '.')
			{
				path[i] = 0;
				break;
			}
		}

		fAssertFilePath = new char[strlen(path)+12+1];	// +12 for .assert
		strcpy( fAssertFilePath, path);
		strcat( fAssertFilePath, "_asserts.txt");
	}
	return fAssertFilePath;
}


FILE *VDebugMgr::_OpenAssertFile( bool inWithTimeStamp, bool inWithStackCrawl)
{
	char buffer[200];


	FILE *assertFile = NULL;

	const char *filePath = _GetAssertFilePath();

	if (!fAssertFileAlreadyOpened)
	{
		assertFile = fopen( filePath, "w");
		if (assertFile)
		{
			time_t today = time( NULL);
			strftime( buffer, sizeof( buffer),"%A, %B %d, %Y %I:%M:%S %p", localtime(&today));
			fprintf( assertFile, "4D log file opened on %s\n\n", buffer);
		}
		fAssertFileAlreadyOpened = true;
	}
	else
	{
		assertFile = fopen( filePath, "a");

		if (assertFile)
		{
			// check size, segments the log in 10 Mo chunks
			long pos = ::ftell( assertFile);
			if (pos > 10 * 1024L * 1024L)
			{
				::fclose( assertFile);

				char newname[4096];
				::sprintf( newname, "%s%d", filePath, (int) (fAssertFileNumber++));

				::rename( filePath, newname);

				assertFile = fopen( filePath, "w");
				time_t today = time( NULL);
				strftime( buffer, sizeof( buffer),"%A, %B %d, %Y %I:%M:%S %p", localtime(&today));
				fprintf( assertFile, "%d nth log opened on %s\n\n", (int) (fAssertFileNumber+1), buffer);
			}
		}
	}

	if ( (inWithTimeStamp || inWithStackCrawl) && (assertFile != NULL) )
		fprintf( assertFile, "\n");

	if (inWithTimeStamp && (assertFile != NULL))
	{
		time_t now = time( NULL);
		strftime( buffer, sizeof( buffer),"%m/%d/%y, %X", localtime( &now));
		fprintf( assertFile, "%s\n", buffer);
	}

	if (inWithStackCrawl && (assertFile != NULL))
	{
		VStackCrawl crawl;
#if VERSIONDEBUG
		crawl.LoadFrames( 4, 8);
#else
		crawl.LoadFrames( 4, 8);
#endif
		crawl.Dump( assertFile);
	}

	return assertFile;
}


void VDebugMgr::WriteToAssertFile( const VString& inMessage, bool inWithTimeStamp, bool inWithStackCrawl)
{
	VStringConvertBuffer buff( inMessage, VTC_StdLib_char);

	FILE *file = _OpenAssertFile( inWithTimeStamp, inWithStackCrawl);
	if (file != NULL)
	{
		::fprintf( file, "%s", buff.GetCPointer());
		::fclose( file);
	}
}


void VDebugMgr::StartLowLevelMode()
{
	VTask* theTask = VTask::GetCurrent();
	if (theTask != NULL)
		theTask->GetDebugContext().StartLowLevelMode();
}


void VDebugMgr::StopLowLevelMode()
{
	VTask* theTask = VTask::GetCurrent();
	if (theTask != NULL)
		theTask->GetDebugContext().StopLowLevelMode();
}


bool VDebugMgr::SetBreakOnAssert(bool inSet)
{
	bool oldValue = fBreakOnAssert;
	fBreakOnAssert = inSet;
	return oldValue;
}


bool VDebugMgr::GetBreakOnAssert()
{
	return fBreakOnAssert;
}


bool VDebugMgr::SetBreakOnDebugMsg(bool inSet)
{
	bool oldValue = fBreakOnDebugMsg;
	fBreakOnDebugMsg = inSet;
	return oldValue;
}


bool VDebugMgr::GetBreakOnDebugMsg()
{
	return fBreakOnDebugMsg;
}


bool VDebugMgr::SetMessagesEnabled(bool inSet)
{
	bool oldValue = fMessageEnabled;
	fMessageEnabled = inSet;
	return oldValue;
}


bool VDebugMgr::GetMessagesEnabled()
{
	return fMessageEnabled;
}


bool VDebugMgr::SetLogInDebugger(bool inSet)
{
	bool oldValue = fLogInDebugger;
	fLogInDebugger = inSet;
	return oldValue;
}


bool VDebugMgr::GetLogInDebugger()
{
	return fLogInDebugger;
}




#if COMPIL_GCC
#define PROC_STATUS_TRACER_PID "TracerPid:"

static inline bool FindGDB()
{
	#if VERSION_LINUX
	// On linux, we can detect if the a process is being traced by inspecting
	// the file `/proc/<pid>/status`. A special field 'TracerPid' provides
	// the PID of process tracing this process (0 if not being traced)

	// preparing the filename from the current process id
	char filename[36];
	if (0 < ::snprintf(filename, sizeof(filename), "/proc/%i/status", (int)::getpid()))
	{
		FILE* fd = ::fopen(filename, "r");
		if (fd)
		{
			// temporary string for the content of the current line
			char line[256];
			// flag to determine whether the debugger has been found
			bool found = false;
			// length of the filed we are looking for
			size_t len = ::strlen(PROC_STATUS_TRACER_PID);

			// reading the file line by line
			while (::fgets(line, sizeof(line), fd))
			{
				// we are looking for the line which starts with 'TracerPid'
				if (0 == ::strncmp(line, PROC_STATUS_TRACER_PID, len))
				{
					// extracting the tracer pid
					const char* strpid = (const char*) line + len;
					int tracerid = ::atoi(strpid);

					// there is a debugguer only if the pid is in
					// a valid range (pid: 1..65535)
					found = (tracerid > 0 && tracerid < 65535);
					break;
				}
			}

			::fclose(fd);
			return found;
		}
	}
	return false;

	#else
	// fallback - always return true by default (historical behavior)
	return true;
	#endif
}
#endif



bool VDebugMgr::_GetDebuggerType( VDebuggerType *outType)
{
	bool foundDebugger = false;
	VDebuggerType type = kDEBUGGER_TYPE_NONE;

	#if COMPIL_GCC
	type = kDEBUGGER_TYPE_GCC;
	foundDebugger = FindGDB();
	foundDebugger = true;
	#elif VERSIONWIN
	// pp || VERSIONDEBUG afin pouvoir intercepter les assert si 4d est attaché au debugger apres lancement.
	if (::IsDebuggerPresent() || VERSIONDEBUG)
	{
		type = kDEBUGGER_TYPE_MSVISUAL;
		foundDebugger = true;
	}
	#endif

	if (outType)
		*outType = type;

	return foundDebugger;
}


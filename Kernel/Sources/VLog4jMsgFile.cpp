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
#include "VFolder.h"
#include "VFile.h"
#include "VProcess.h"
#include "VLog4jMsgFile.h"



static const char *GetMessageLevelName( EMessageLevel inLevel)
{
	switch( inLevel)
	{
		case EML_Dump:			return "DUMP";
		case EML_Trace:			return "TRACE";
		case EML_Debug:			return "DEBUG";
		case EML_Warning:		return "WARN";
		case EML_Error:			return "ERROR";
		case EML_Fatal:			return "FATAL";
		case EML_Assert:		return "ASSERT";	// non standard
		case EML_Information:
		default:				return "INFO";
	}
}



void VLog4jMsgFileLogger::VLog4jLogListener::Log( const VString& inLoggerID, EMessageLevel inLevel, const VString& inMessage)
{
	StStringConverter<char> loggerConverter( VTC_StdLib_char);
	StStringConverter<char> messageConverter( VTC_StdLib_char);
	Log( loggerConverter.ConvertString( inLoggerID), inLevel, messageConverter.ConvertString( inMessage) );
}


void VLog4jMsgFileLogger::VLog4jLogListener::Log( const char* inLoggerID, EMessageLevel inLevel, const char* inMessage)//, VString* outFormattedMessage)
{

	if (fLogger->ShouldLog(inLevel))
	{
		bool done = false;
		char szTime[512];
		time_t now = ::time( NULL);
		::strftime( szTime, sizeof( szTime),"%Y-%m-%d %X", ::localtime( &now));

		/*if (outFormattedMessage != NULL)
		{
			outFormattedMessage->Clear();

			void *buffer = ::malloc( ::strlen( inMessage) + 256);
			if (buffer != NULL)
			{
				::sprintf( (char*)buffer, "%s [%s] %s - %s\n", szTime, inLoggerID, GetMessageLevelName(inLevel), inMessage);

				outFormattedMessage->FromCString( (char*)buffer);
				fOutput->AppendString( (char*)buffer);
				done = true;

				::free( buffer);
			}
		}*/

		if (!done)
		{
			fLogger->fOutput->AppendFormattedString( "%s [%s] %s - %s\n", szTime, inLoggerID, GetMessageLevelName(inLevel), inMessage);
		}

		// need to see all messages in log before crashing
		//fLogger->fOutput->Flush();
	}

}
void VLog4jMsgFileLogger::VLog4jLogListener::Put( std::vector< const XBOX::VValueBag* >& inValuesVector )
{
	if (fLogger)
	{
		fLogger->fLock.Lock();

		if (fLogger->fOutput)
		{
			for( std::vector< const XBOX::VValueBag* >::size_type idx = 0; idx < inValuesVector.size(); idx++ )
			{
				VString			loggerID;
				EMessageLevel	level;
				VString			message;
				VLog4jMsgFileLogger::GetLogInfosFromBag( inValuesVector[idx], loggerID, level, message );
				Log( loggerID, level, message );
			}
			fLogger->fOutput->Flush();
		}
		fLogger->fLock.Unlock();
	}
}




VLog4jMsgFileLogger::VLog4jMsgFileLogger( const VFolder& inLogFolder, const VString& inLogName)
: fLogName(inLogName)
, fLog4jLogListener(this)
, fOutput(NULL)
, fIsStarted( false)
, fFilter((1<<EML_Information) | (1<<EML_Warning) | (1<<EML_Error) | (1<<EML_Fatal) | (1<<EML_Debug) | (1<<EML_Assert) /*| (1<<EML_Trace) | (1<<EML_Dump)*/)
{
	inLogFolder.GetPath( fFolderPath);
}


VLog4jMsgFileLogger::~VLog4jMsgFileLogger()
{
	Stop();
}


void VLog4jMsgFileLogger::Start( sLONG inLogNumber)
{
	fLock.Lock();
	if (fOutput == NULL)
	{
		if (!fFolderPath.IsEmpty() && fFolderPath.IsValid() && fFolderPath.IsFolder() && !fLogName.IsEmpty())
		{
			xbox_assert(inLogNumber > 0);

			VFolder folder( fFolderPath);

			fOutput = new VSplitableLogFile( folder, fLogName, inLogNumber);
			if (fOutput != NULL)
			{
				if (fOutput->Open( false))
				{
					VTime::Now( fStartTime);
					fIsStarted = true;

					VString path;
					fOutput->GetCurrentFilePath( path);
					
					/*fReadersLock.Lock();

					for (std::vector<IReader*>::iterator iter = fReaders.begin() ; iter != fReaders.end() ; ++iter)
						(*iter)->DoStart( path);

					fReadersLock.Unlock();*/
					VProcess::Get()->GetLogger()->AddLogListener(&fLog4jLogListener);
				}
				else
				{
					fOutput->Close();
					delete fOutput;
					fOutput = NULL;
				}
			}
		}
	}
	fLock.Unlock();
}


void VLog4jMsgFileLogger::Start()
{
	if (!fFolderPath.IsEmpty() && fFolderPath.IsValid() && fFolderPath.IsFolder() && !fLogName.IsEmpty())
	{
		// Find the highest log file number
		VString fileName;
		VFolder folder(fFolderPath);
		sLONG highestNumber = -1, curNumber = 1;

		while (highestNumber == -1)
		{
			BuildLogFileName( fLogName, curNumber, fileName);
			
			VFile file( folder, fileName);
			if (file.Exists())
				++curNumber;
			else
				highestNumber = (curNumber > 1) ? curNumber - 1 : 1;
		}

		Start( highestNumber);
	}
}


void VLog4jMsgFileLogger::Stop()
{
	fLock.Lock();
	if (fOutput != NULL)
	{
		VProcess::Get()->GetLogger()->RemoveLogListener(&fLog4jLogListener);
		fOutput->Close();
		delete fOutput;
		fOutput = NULL;
		fIsStarted = false;
	}
	fStartTime.Clear();
	fLock.Unlock();
}


bool VLog4jMsgFileLogger::IsStarted() const
{
	return fIsStarted;
}


void VLog4jMsgFileLogger::Flush()
{
	fLock.Lock();
	if(fOutput != NULL)
	{
		fOutput->Flush();
	}
	fLock.Unlock();
}


bool VLog4jMsgFileLogger::ShouldLog( EMessageLevel inMessageLevel) const
{
	// no lock. Must be as fast as possible.
	// no filtering for now. Accept any kind of message.
	return fIsStarted && ((fFilter & (1 << inMessageLevel)) != 0);
}
/*
bool VLog4jMsgFileLogger::WithTag(uLONG inTag, bool inFlag)
{
	uLONG filter=GetLevelFilter();

	inTag=(1<<inTag);

	bool rv=(filter&inTag) ? true : false;
	
	if(inFlag)
		filter|=inTag;
	else
		filter&=~inTag;
	
	SetLevelFilter(filter);
	
	return rv;
}

bool VLog4jMsgFileLogger::WithTrace(bool inWithTag)		{ return WithTag(EML_Trace,		 	inWithTag); }
bool VLog4jMsgFileLogger::WithDump(bool inWithTag)		{ return WithTag(EML_Dump,		 	inWithTag); }
bool VLog4jMsgFileLogger::WithDebug(bool inWithTag)		{ return WithTag(EML_Debug,		 	inWithTag); }
bool VLog4jMsgFileLogger::WithWarning(bool inWithTag)	{ return WithTag(EML_Warning,	 	inWithTag); }
bool VLog4jMsgFileLogger::WithError(bool inWithTag)		{ return WithTag(EML_Error,		 	inWithTag); }
bool VLog4jMsgFileLogger::WithFatal(bool inWithTag)		{ return WithTag(EML_Fatal,		 	inWithTag); }
bool VLog4jMsgFileLogger::WithAssert(bool inWithTag)	{ return WithTag(EML_Assert,	 	inWithTag); }
bool VLog4jMsgFileLogger::WithInfo(bool inWithTag)		{ return WithTag(EML_Information, 	inWithTag); }



*/
void VLog4jMsgFileLogger::GetLogFolderPath( VFilePath& outPath) const
{
	outPath.FromFilePath( fFolderPath);
}


void VLog4jMsgFileLogger::GetLogName( VString& outName) const
{
	outName = fLogName;
}


void VLog4jMsgFileLogger::BuildLogFileName( const VString& inLogName, sLONG inLogNumber, VString& outName)
{
	VSplitableLogFile::BuildLogFileName( inLogName, inLogNumber, outName);
}


void VLog4jMsgFileLogger::GetLogInfosFromBag( const VValueBag *inMessage, VString& outLoggerID, EMessageLevel& outLevel, VString& outMessage)
{
	ILoggerBagKeys::message.Get( inMessage, outMessage);

	VError errorCode = VE_OK;
	ILoggerBagKeys::error_code.Get( inMessage, errorCode);

	outLevel = ILoggerBagKeys::level.Get( inMessage);

	if (!ILoggerBagKeys::source.Get( inMessage, outLoggerID))
		outLoggerID = VProcess::Get()->GetLogSourceIdentifier();

	OsType componentSignature = 0;
	if (!ILoggerBagKeys::component_signature.Get( inMessage, componentSignature))
		componentSignature = COMPONENT_FROM_VERROR( errorCode);

	if (componentSignature != 0)
	{
		if (!outLoggerID.IsEmpty())
			outLoggerID.AppendUniChar( L'.');
		outLoggerID.AppendOsType( componentSignature);
	}

	if (errorCode != VE_OK)
	{
		VString s;
		s.Printf( "[%d] ", ERRCODE_FROM_VERROR( errorCode));
		outMessage.Insert( s, 1);
	}
	
	sLONG taskId=-1;
	if (ILoggerBagKeys::task_id.Get(inMessage, taskId))
	{
		outMessage.AppendString(CVSTR(", task #"));
		outMessage.AppendLong(taskId);
	}
	
	VString taskName;
	ILoggerBagKeys::task_name.Get( inMessage, taskName);
	if (!taskName.IsEmpty())
	{
		outMessage += ", task name is ";
		outMessage += taskName;
	}
	
	sLONG socketDescriptor=-1;
	if (ILoggerBagKeys::socket.Get(inMessage, socketDescriptor))
	{
		outMessage.AppendString(CVSTR(", socket "));
		outMessage.AppendLong(socketDescriptor);
	}
	
	VString localAddr;
	if (ILoggerBagKeys::local_addr.Get(inMessage, localAddr))
	{
		outMessage.AppendString(CVSTR(", local addr is "));
		outMessage.AppendString(localAddr);
	}
	
	VString peerAddr;
	if (ILoggerBagKeys::peer_addr.Get(inMessage, peerAddr))
	{
		outMessage.AppendString(CVSTR(", peer addr is "));
		outMessage.AppendString(peerAddr);
	}
	
	sLONG localPort;
	if (ILoggerBagKeys::local_port.Get(inMessage, localPort))
	{
		outMessage.AppendString(CVSTR(", local port is "));
		outMessage.AppendLong(localPort);
	}
	
	sLONG peerPort;
	if (ILoggerBagKeys::peer_port.Get(inMessage, peerPort))
	{
		outMessage.AppendString(CVSTR(", peer port is "));
		outMessage.AppendLong(peerPort);
	}

	bool exchangeEndPointID=false;
	if (ILoggerBagKeys::exchange_id.Get(inMessage, exchangeEndPointID))
	{
		if(exchangeEndPointID)
			outMessage.AppendString(CVSTR(", exchange endpoint id"));
		else
			outMessage.AppendString(CVSTR(", do not exchange endpoint id"));
	}
	
	bool isBlocking=false;
	if (ILoggerBagKeys::is_blocking.Get(inMessage, isBlocking))
	{
		if(isBlocking)
			outMessage.AppendString(CVSTR(", is blocking"));
		else
			outMessage.AppendString(CVSTR(", is not blocking"));
	}
	
	bool isSSL=false;
	if (ILoggerBagKeys::is_ssl.Get(inMessage, isSSL))
	{
		if(isSSL)
			outMessage.AppendString(CVSTR(", with SSL"));
		else
			outMessage.AppendString(CVSTR(", without SSL"));
	}
	
	bool isSelectIO=false;
	if (ILoggerBagKeys::is_select_io.Get(inMessage, isSelectIO))
	{
		if(isSelectIO)
			outMessage.AppendString(CVSTR(", with SelectIO"));
		else
			outMessage.AppendString(CVSTR(", without SelectIO"));
	}
	
	sLONG ioTimeout=-1;
	if (ILoggerBagKeys::ms_timeout.Get(inMessage, ioTimeout))
	{
		outMessage.AppendString(CVSTR(", with "));
		outMessage.AppendLong(ioTimeout);
		outMessage.AppendString(CVSTR("ms timeout"));
	}
	
	sLONG askedCount=-1;
	if (ILoggerBagKeys::count_bytes_asked.Get(inMessage, askedCount))
	{
		outMessage.AppendString(CVSTR(", asked for "));
		outMessage.AppendLong(askedCount);
		outMessage.AppendString(CVSTR(" byte(s)"));
	}
	
	sLONG sentCount=-1;
	if (ILoggerBagKeys::count_bytes_sent.Get(inMessage, sentCount))
	{
		outMessage.AppendString(CVSTR(", sent "));
		outMessage.AppendLong(sentCount);
		outMessage.AppendString(CVSTR(" byte(s)"));
	}
	
	sLONG receivedCount=-1;
	if (ILoggerBagKeys::count_bytes_received.Get(inMessage, receivedCount))
	{
		outMessage.AppendString(CVSTR(", received "));
		outMessage.AppendLong(receivedCount);
		outMessage.AppendString(CVSTR(" byte(s)"));
	}
	
	sLONG ioSpent=-1;
	if (ILoggerBagKeys::ms_spent.Get(inMessage, ioSpent))
	{
		outMessage.AppendString(CVSTR(", done in "));
		outMessage.AppendLong(ioSpent);
		outMessage.AppendString(CVSTR("ms"));
	}
	
	sLONG dumpOffset=-1;
	if (ILoggerBagKeys::dump_offset.Get(inMessage, dumpOffset))
	{
		outMessage.AppendString(CVSTR(", offset "));
		outMessage.AppendLong(dumpOffset);
	}
	
	VString dumpBuffer;
	if (ILoggerBagKeys::dump_buffer.Get(inMessage, dumpBuffer))
	{
		outMessage.AppendString(CVSTR(", data : "));
		outMessage.AppendString(dumpBuffer);
	}
	
	VString fileName;
	if (ILoggerBagKeys::file_name.Get( inMessage, fileName))
	{
		outMessage += ", ";
		outMessage += fileName;
	}

	sLONG lineNumber;
	if (ILoggerBagKeys::line_number.Get( inMessage, lineNumber))
	{
		outMessage += ", ";
		outMessage.AppendLong( lineNumber);
	}
	
	VString stackCrawl;
	if (ILoggerBagKeys::stack_crawl.Get( inMessage, stackCrawl))
	{
		outMessage += ", {";
		stackCrawl.ExchangeAll( CVSTR( "\n"), CVSTR( " ; "));
		outMessage += stackCrawl;
		outMessage += "}";
	}
}


void VLog4jMsgFileLogger::DoCreateNewLogFile( const VString& inFilePath)
{
	fLock.Lock();
	VString path;
	fOutput->GetCurrentFilePath( path);
	fLock.Unlock();

}


bool VLog4jMsgFileLogger::_IsStarted() const
{
	return (fOutput != NULL) && fOutput->IsOpen();
}




VLog4jMsgFileLogReader::VLog4jMsgFileLogReader(const VFolder& inLogFolder, const VString& inLogName) : fFile(NULL), fOutput(NULL)
{
	if (!inLogFolder.IsEmpty() && !inLogName.IsEmpty())
	{
		// Find the highest log file number
		VString fileName;
		VFolder folder(inLogFolder);
		sLONG highestNumber = -1, curNumber = 1;

		while (highestNumber == -1)
		{
			fileName.Clear();
			fileName += inLogName;
			fileName += "_";
			fileName.AppendLong( curNumber);
			fileName += ".txt";
			
			VFile file( inLogFolder, fileName);
			if (file.Exists())
				++curNumber;
			else
				highestNumber = (curNumber > 1) ? curNumber - 1 : 1;
		}


		fOutput = new VSplitableLogFile( inLogFolder, inLogName, highestNumber);
	}
	if (fOutput != NULL)
	{
		if (fOutput->Open( false))
		{
			VString path;
			fOutput->GetCurrentFilePath( path);

			DoStart( path);

		}
		else
		{
			fOutput->Close();
			delete fOutput;
			fOutput = NULL;
		}
	}
}
VLog4jMsgFileLogReader::~VLog4jMsgFileLogReader()
{
	fLock.Lock();
	if (fOutput != NULL)
	{	
		if (fFile != NULL)
		{
			::fclose( fFile);
			fFile = NULL;
		}
		fOutput->Close();
		delete fOutput;
		fOutput = NULL;
	}
	fLock.Unlock();
}

void VLog4jMsgFileLogReader::PeekMessages( VectorOfVString& outMessages) const
{
	outMessages.clear();
	fLock.Lock();
	outMessages.insert( outMessages.begin(), fMessages.begin(), fMessages.end());
	fMessages.clear();
	_PeekMessages( outMessages);
	fLock.Unlock();
}

//============================================================


void VLog4jMsgFileLogReader::DoStart( const VString& inFilePath)
{
	fLock.Lock();

	if (fFile != NULL)
	{
		// keep the last messages which was recorded in the previous file
		_PeekMessages( fMessages);
		::fclose( fFile);
		fFile = NULL;
	}

	if (!inFilePath.IsEmpty())
	{
		StStringConverter<char> convert( VTC_StdLib_char);
		fFile = ::fopen( convert.ConvertString( inFilePath), "r");
		::fseek( fFile, 0, SEEK_END);
	}

	fLock.Unlock();
}


void VLog4jMsgFileLogReader::_PeekMessages( VectorOfVString& outMessages) const
{
	fLock.Lock();

	if (fFile != NULL)
	{
		char buffer[1024];	// could be increased if need

		while (::fgets( buffer, 1024, fFile) != NULL)
		{
			VString string( buffer);
			outMessages.push_back( string);
		}
#if VERSIONMAC
		// Clean EOF stream flag to allow next fgets() not to fail
		if ( ::feof(fFile) )
			::clearerr(fFile);
#endif
	}
	fLock.Unlock();
}


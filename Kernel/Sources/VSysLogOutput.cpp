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
#include "VProcess.h"
#include <syslog.h>
#include "VSysLogOutput.h"



VSysLogOutput::VSysLogOutput()
: fFilter((1<<EML_Trace) | (1<<EML_Information) | (1<<EML_Warning) | (1<<EML_Error) | (1<<EML_Fatal))
, fIdentifier( NULL)
{
	// the facility may have to be changed according to the context (user process, daemon...)
	openlog( NULL, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);
}


VSysLogOutput::VSysLogOutput( const VString& inIdentifier)
: fFilter((1<<EML_Trace) | (1<<EML_Information) | (1<<EML_Warning) | (1<<EML_Error) | (1<<EML_Fatal))
, fIdentifier( NULL)
{
	if (!inIdentifier.IsEmpty())
	{
		// SysLog retain the identifier buffer pointer until closelog() is called
		StStringConverter<char> converter( inIdentifier, VTC_StdLib_char);
		if (converter.GetSize() > 0)
		{
			fIdentifier = VMemory::NewPtrClear( converter.GetSize() + 1, 'SYSL');
			if (fIdentifier != NULL)
			{
				VMemory::CopyBlock( converter.GetCPointer(), fIdentifier, converter.GetSize());
			}
		}
		
	}
	
	// the facility may have to be changed according to the context (user process, daemon...)
	openlog( fIdentifier, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);
}


VSysLogOutput::~VSysLogOutput()
{
	closelog();
	
	if (fIdentifier != NULL)
	{
		VMemory::DisposePtr( fIdentifier);
		fIdentifier = NULL;
	}
}


void VSysLogOutput::Put( std::vector< const XBOX::VValueBag* >& inValuesVector)
{
	for (std::vector< const XBOX::VValueBag* >::iterator bagIter = inValuesVector.begin() ; bagIter != inValuesVector.end() ; ++bagIter)
	{
		EMessageLevel bagLevel = ILoggerBagKeys::level.Get( *bagIter);
		if ((fFilter & (1 << bagLevel)) != 0)
		{
			VString logMsg;
						
			VError errorCode = VE_OK;
			ILoggerBagKeys::error_code.Get( *bagIter, errorCode);
		
			VString loggerID;
			if (!ILoggerBagKeys::source.Get( *bagIter, loggerID))
				loggerID = VProcess::Get()->GetLogSourceIdentifier();
			
			OsType componentSignature = 0;
			if (!ILoggerBagKeys::component_signature.Get( *bagIter, componentSignature))
				componentSignature = COMPONENT_FROM_VERROR( errorCode);
			
			if (componentSignature != 0)
			{
				if (!loggerID.IsEmpty())
					loggerID.AppendUniChar( L'.');
				loggerID.AppendOsType( componentSignature);
			}
			
			if (!loggerID.IsEmpty())
				logMsg.Printf( "[%S]", &loggerID);
			
			// build message string
			VString message;
			
			if (errorCode != VE_OK)
				message.AppendPrintf( "error %d", ERRCODE_FROM_VERROR( errorCode));
			
			VString bagMsg;
			if (ILoggerBagKeys::message.Get( *bagIter, bagMsg))
			{
				if (!message.IsEmpty())
					message.AppendString( L", ");
				message.AppendString( bagMsg);
			}
						
			sLONG taskId=-1;
			if (ILoggerBagKeys::task_id.Get( *bagIter, taskId))
			{
				if (!message.IsEmpty())
					message.AppendString( L", ");
				message.AppendString( L"task #");
				message.AppendLong( taskId);
			}
			
			VString taskName;
			ILoggerBagKeys::task_name.Get( *bagIter, taskName);
			if (!taskName.IsEmpty())
			{
				if (!message.IsEmpty())
					message.AppendString( L", ");
				message.AppendString( L"task name is ");
				message.AppendString( taskName);
			}
			
			sLONG socketDescriptor=-1;
			if (ILoggerBagKeys::socket.Get( *bagIter, socketDescriptor))
			{
				if (!message.IsEmpty())
					message.AppendString( L", ");
				message.AppendString( L"socket ");
				message.AppendLong(socketDescriptor);
			}
			
			VString localAddr;
			if (ILoggerBagKeys::local_addr.Get( *bagIter, localAddr))
			{
				if (!message.IsEmpty())
					message.AppendString( L", ");
				message.AppendString( L"local addr is ");
				message.AppendString( localAddr);
			}
			
			VString peerAddr;
			if (ILoggerBagKeys::peer_addr.Get( *bagIter, peerAddr))
			{
				if (!message.IsEmpty())
					message.AppendString( L", ");
				message.AppendString( L"peer addr is ");
				message.AppendString( peerAddr);
			}
			
			bool exchangeEndPointID=false;
			if (ILoggerBagKeys::exchange_id.Get( *bagIter, exchangeEndPointID))
			{
				if (!message.IsEmpty())
					message.AppendString( L", ");
				message.AppendString( (exchangeEndPointID) ? L"exchange endpoint id" : L"do not exchange endpoint id");
			}
			
			bool isBlocking=false;
			if (ILoggerBagKeys::is_blocking.Get( *bagIter, isBlocking))
			{
				if (!message.IsEmpty())
					message.AppendString( L", ");
				message.AppendString( (isBlocking) ? L"is blocking" : L"is not blocking");
			}
			
			bool isSSL=false;
			if (ILoggerBagKeys::is_ssl.Get( *bagIter, isSSL))
			{
				if (!message.IsEmpty())
					message.AppendString( L", ");
				message.AppendString( (isSSL) ? L"with SSL" : L"without SSL");
			}
			
			bool isSelectIO=false;
			if (ILoggerBagKeys::is_select_io.Get( *bagIter, isSelectIO))
			{
				if (!message.IsEmpty())
					message.AppendString( L", ");
				message.AppendString( (isSelectIO) ? L"with SelectIO" : L"without SelectIO");
			}
			
			sLONG ioTimeout=-1;
			if (ILoggerBagKeys::ms_timeout.Get( *bagIter, ioTimeout))
			{
				if (!message.IsEmpty())
					message.AppendString( L", ");
				message.AppendString( L"with ");
				message.AppendLong( ioTimeout);
				message.AppendString( L"ms timeout");
			}
			
			sLONG askedCount=-1;
			if (ILoggerBagKeys::count_bytes_asked.Get( *bagIter, askedCount))
			{
				if (!message.IsEmpty())
					message.AppendString( L", ");
				message.AppendString( L"asked for ");
				message.AppendLong( askedCount);
				message.AppendString( L" byte(s)");
			}
			
			sLONG sentCount=-1;
			if (ILoggerBagKeys::count_bytes_sent.Get(*bagIter, sentCount))
			{
				if (!message.IsEmpty())
					message.AppendString( L", ");
				message.AppendString( L"sent ");
				message.AppendLong( sentCount);
				message.AppendString( L" byte(s)");
			}
			
			sLONG receivedCount=-1;
			if (ILoggerBagKeys::count_bytes_received.Get( *bagIter, receivedCount))
			{
				if (!message.IsEmpty())
					message.AppendString( L", ");
				message.AppendString( L"received ");
				message.AppendLong( receivedCount);
				message.AppendString( L" byte(s)");
			}
			
			sLONG ioSpent=-1;
			if (ILoggerBagKeys::ms_spent.Get( *bagIter, ioSpent))
			{
				if (!message.IsEmpty())
					message.AppendString( L", ");
				message.AppendString( L"done in ");
				message.AppendLong( ioSpent);
				message.AppendString( L"ms");
			}
			
			sLONG dumpOffset=-1;
			if (ILoggerBagKeys::dump_offset.Get( *bagIter, dumpOffset))
			{
				if (!message.IsEmpty())
					message.AppendString( L", ");
				message.AppendString( L"offset ");
				message.AppendLong( dumpOffset);
			}
			
			VString dumpBuffer;
			if (ILoggerBagKeys::dump_buffer.Get( *bagIter, dumpBuffer))
			{
				if (!message.IsEmpty())
					message.AppendString( L", ");
				message.AppendString( L"data : ");
				message.AppendString( dumpBuffer);
			}
			
			VString fileName;
			if (ILoggerBagKeys::file_name.Get( *bagIter, fileName))
			{
				if (!message.IsEmpty())
					message.AppendString( L", ");
				message.AppendString( fileName);
			}
			
			sLONG lineNumber;
			if (ILoggerBagKeys::line_number.Get( *bagIter, lineNumber))
			{
				if (!message.IsEmpty())
					message.AppendString( L", ");
				message.AppendLong( lineNumber);
			}
			
			VString stackCrawl;
			if (ILoggerBagKeys::stack_crawl.Get( *bagIter, stackCrawl))
			{
				if (!message.IsEmpty())
					message.AppendString( L", ");
				message.AppendString( L", {");
				stackCrawl.ExchangeAll( L"\n", L" ; ");
				message.AppendString( stackCrawl);
				message.AppendUniChar( L'}');
			}
			
			if (!logMsg.IsEmpty())
				logMsg.AppendUniChar( L' ');
			logMsg.AppendString( message);
			
			if (!logMsg.IsEmpty())
			{
				StStringConverter<char> messageConverter( VTC_StdLib_char);
		
				switch( bagLevel)
				{
					case EML_Dump:
					case EML_Assert:
					case EML_Debug:
						syslog( LOG_DEBUG, messageConverter.ConvertString( logMsg));
						break;
						
					case EML_Trace:
						syslog( LOG_NOTICE, messageConverter.ConvertString( logMsg));
						break;

					case EML_Information:
						syslog( LOG_INFO, messageConverter.ConvertString( logMsg));
						break;
							   
					case EML_Warning:
						syslog( LOG_WARNING, messageConverter.ConvertString( logMsg));
						break;
							   
					case EML_Error:
						syslog( LOG_ERR, messageConverter.ConvertString( logMsg));
						break;
						
					case EML_Fatal:
						syslog( LOG_CRIT, messageConverter.ConvertString( logMsg));
						break;
							   
				   default:
					   assert(false);
					   break;
				}
			}
		}
	}
}
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
					loggerID.AppendUniChar('.');
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
					message.AppendString(CVSTR(", "));
				message.AppendString( bagMsg);
			}

			sLONG taskId=-1;
			if (ILoggerBagKeys::task_id.Get( *bagIter, taskId))
			{
				if (!message.IsEmpty())
					message.AppendString(CVSTR(", "));
				message.AppendString(CVSTR("task #"));
				message.AppendLong( taskId);
			}

			VString taskName;
			ILoggerBagKeys::task_name.Get( *bagIter, taskName);
			if (!taskName.IsEmpty())
			{
				if (!message.IsEmpty())
					message.AppendString(CVSTR(", "));
				message.AppendString(CVSTR("task name is "));
				message.AppendString( taskName);
			}

			sLONG socketDescriptor=-1;
			if (ILoggerBagKeys::socket.Get( *bagIter, socketDescriptor))
			{
				if (!message.IsEmpty())
					message.AppendString(CVSTR(", "));
				message.AppendString(CVSTR("socket "));
				message.AppendLong(socketDescriptor);
			}

			VString localAddr;
			if (ILoggerBagKeys::local_addr.Get( *bagIter, localAddr))
			{
				if (!message.IsEmpty())
					message.AppendString(CVSTR(", "));
				message.AppendString(CVSTR("local addr is "));
				message.AppendString( localAddr);
			}

			VString peerAddr;
			if (ILoggerBagKeys::peer_addr.Get( *bagIter, peerAddr))
			{
				if (!message.IsEmpty())
					message.AppendString( L", ");
				message.AppendString(CVSTR("peer addr is "));
				message.AppendString( peerAddr);
			}

			bool exchangeEndPointID=false;
			if (ILoggerBagKeys::exchange_id.Get( *bagIter, exchangeEndPointID))
			{
				if (!message.IsEmpty())
					message.AppendString(CVSTR(", "));
				message.AppendString( (exchangeEndPointID) ? CVSTR("exchange endpoint id") : CVSTR("do not exchange endpoint id"));
			}

			bool isBlocking=false;
			if (ILoggerBagKeys::is_blocking.Get( *bagIter, isBlocking))
			{
				if (!message.IsEmpty())
					message.AppendString(CVSTR(", "));
				message.AppendString( (isBlocking) ? CVSTR("is blocking") : CVSTR("is not blocking"));
			}

			bool isSSL=false;
			if (ILoggerBagKeys::is_ssl.Get( *bagIter, isSSL))
			{
				if (!message.IsEmpty())
					message.AppendString( CVSTR(", "));
				message.AppendString( (isSSL) ? CVSTR("with SSL") : CVSTR("without SSL"));
			}

			bool isSelectIO=false;
			if (ILoggerBagKeys::is_select_io.Get( *bagIter, isSelectIO))
			{
				if (!message.IsEmpty())
					message.AppendString(CVSTR(", "));
				message.AppendString( (isSelectIO) ? CVSTR("with SelectIO") : CVSTR("without SelectIO"));
			}

			sLONG ioTimeout=-1;
			if (ILoggerBagKeys::ms_timeout.Get( *bagIter, ioTimeout))
			{
				if (!message.IsEmpty())
					message.AppendString(CVSTR(", "));
				message.AppendString(CVSTR("with "));
				message.AppendLong( ioTimeout);
				message.AppendString(CVSTR("ms timeout"));
			}

			sLONG askedCount=-1;
			if (ILoggerBagKeys::count_bytes_asked.Get( *bagIter, askedCount))
			{
				if (!message.IsEmpty())
					message.AppendString(CVSTR(", "));
				message.AppendString(CVSTR("asked for "));
				message.AppendLong( askedCount);
				message.AppendString(CVSTR(" byte(s)"));
			}

			sLONG sentCount=-1;
			if (ILoggerBagKeys::count_bytes_sent.Get(*bagIter, sentCount))
			{
				if (!message.IsEmpty())
					message.AppendString(CVSTR(", "));
				message.AppendString(CVSTR("sent "));
				message.AppendLong( sentCount);
				message.AppendString(CVSTR(" byte(s)"));
			}

			sLONG receivedCount=-1;
			if (ILoggerBagKeys::count_bytes_received.Get( *bagIter, receivedCount))
			{
				if (!message.IsEmpty())
					message.AppendString(CVSTR(", "));
				message.AppendString(CVSTR("received "));
				message.AppendLong( receivedCount);
				message.AppendString(CVSTR(" byte(s)"));
			}

			sLONG ioSpent=-1;
			if (ILoggerBagKeys::ms_spent.Get( *bagIter, ioSpent))
			{
				if (!message.IsEmpty())
					message.AppendString(CVSTR(", "));
				message.AppendString(CVSTR("done in "));
				message.AppendLong( ioSpent);
				message.AppendString(CVSTR("ms"));
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
					message.AppendString(CVSTR(", "));
				message.AppendString(CVSTR("data : "));
				message.AppendString( dumpBuffer);
			}

			VString fileName;
			if (ILoggerBagKeys::file_name.Get( *bagIter, fileName))
			{
				if (!message.IsEmpty())
					message.AppendString(CVSTR(", "));
				message.AppendString( fileName);
			}

			sLONG lineNumber;
			if (ILoggerBagKeys::line_number.Get( *bagIter, lineNumber))
			{
				if (!message.IsEmpty())
					message.AppendString(CVSTR(", "));
				message.AppendLong( lineNumber);
			}

			VString stackCrawl;
			if (ILoggerBagKeys::stack_crawl.Get( *bagIter, stackCrawl))
			{
				if (!message.IsEmpty())
					message.AppendString(CVSTR(", "));
				message.AppendString(CVSTR(", {"));
				stackCrawl.ExchangeAll(CVSTR("\n"), CVSTR(" ; "));
				message.AppendString(stackCrawl);
				message.AppendUniChar('}');
			}

			if (!logMsg.IsEmpty())
				logMsg.AppendUniChar(' ');
			logMsg.AppendString( message);

			if (!logMsg.IsEmpty())
			{
				StStringConverter<char> messageConverter( VTC_StdLib_char);

				switch( bagLevel)
				{
					case EML_Dump:
					case EML_Assert:
					case EML_Debug:
						syslog( LOG_DEBUG, "%s", messageConverter.ConvertString( logMsg));
						break;

					case EML_Trace:
						syslog( LOG_NOTICE, "%s", messageConverter.ConvertString( logMsg));
						break;

					case EML_Information:
						syslog( LOG_INFO, "%s", messageConverter.ConvertString( logMsg));
						break;

					case EML_Warning:
						syslog( LOG_WARNING, "%s", messageConverter.ConvertString( logMsg));
						break;

					case EML_Error:
						syslog( LOG_ERR, "%s", messageConverter.ConvertString( logMsg));
						break;

					case EML_Fatal:
						syslog( LOG_CRIT, "%s", messageConverter.ConvertString( logMsg));
						break;

				   default:
					   assert(false);
					   break;
				}
			}
		}
	}
}

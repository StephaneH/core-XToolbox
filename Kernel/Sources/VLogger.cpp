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
#include "VLogger.h"
#include "VProcess.h"



/*
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
*/



VLogger::VLogger()// const VFolder& inLogFolder, const VString& inLogName)
: fLogName("")//inLogName)
, fIsStarted( false)
, fMessagesLost(false)
, fFilter((1<<EML_Information) | (1<<EML_Warning) | (1<<EML_Error) | (1<<EML_Fatal) | (1<<EML_Debug) | (1<<EML_Assert) /*| (1<<EML_Trace) | (1<<EML_Dump)*/)
, fLogReaderTask(NULL)
{
	fStartIndex = 0;
	fEndIndex = 0;
	//inLogFolder.GetPath( fFolderPath);

}


VLogger::~VLogger()
{
	fLock.Lock();
	std::vector<ILogListener*>::iterator itListener = fLogListeners.begin();
	while (itListener != fLogListeners.end())
	{
		(*itListener)->Release();
		itListener++;
	}
	fLock.Unlock();

	xbox_assert( fLogReaderTask == NULL || fLogReaderTask->GetState() == TS_DEAD);
	ReleaseRefCountable( &fLogReaderTask);
}

void VLogger::Stop()
{
	//task is killed by VProcess/VRiaServer
	/*if (fLogReaderTask)
	{
		fLogReaderTask->Kill();
		fLogReaderTask->WaitForDeath( 3000);
	}*/
}

void VLogger::Start()
{
	fIsStarted = true;
	if (fLogReaderTask == NULL)
	{
		fLogReaderTask = new XBOX::VTask(this, 64000, XBOX::eTaskStylePreemptive, &VLogger::LogReaderTaskProc);
		if (fLogReaderTask)
		{
			fLogReaderTask->SetName("Logger");
			fLogReaderTask->SetKind( kLoggerTaskKind);
			fLogReaderTask->SetKindData((sLONG_PTR)this);
			fLogReaderTask->Run();
		}
	}
}

void VLogger::Flush()
{
	std::vector<const VValueBag*>	valuesVector;

	fLock.Lock();

	sLONG							nbRead = Read(valuesVector,true);
	if (nbRead)
	{
		for( size_t idxListener = 0; idxListener < fLogListeners.size(); idxListener++ )
		{
			fLogListeners[idxListener]->Put(valuesVector);
		}
	}

	fLock.Unlock();

	for( size_t idx = 0; idx < valuesVector.size(); idx++ )
	{
		valuesVector[idx]->Release();
	}
}

sLONG VLogger::LogReaderTaskProc(XBOX::VTask* inTask)
{
	VLogger*						l_this = (VLogger*)inTask->GetKindData();

	while(!inTask->IsDying())
	{
		l_this->Flush();

		VTask::Sleep(100);
	}
	return 0;
}


bool VLogger::AddLogListener(ILogListener* inLogListener)
{
	bool result = false;
	if (inLogListener)
	{
		fLock.Lock();
		std::vector<ILogListener*>::iterator	itListener = fLogListeners.begin();
		while( itListener != fLogListeners.end() )
		{
			if ( (*itListener) == inLogListener )
			{
				inLogListener = NULL;
				break;
			}
			itListener++;
		}
		if (inLogListener)
		{
			std::vector<ILogListener*>::iterator	itListeners = fLogListeners.begin();
			fLogListeners.insert(itListeners,inLogListener);
			inLogListener->Retain();
			result = true;
		}
		fLock.Unlock();
	}
	return result;
}

bool VLogger::RemoveLogListener(ILogListener* inLogListener)
{
	bool result = false;
	if (inLogListener)
	{
		fLock.Lock();
		std::vector<ILogListener*>::iterator	itListener = fLogListeners.begin();
		while( itListener != fLogListeners.end() )
		{
			if ( (*itListener) == inLogListener )
			{
				inLogListener->Release();
				fLogListeners.erase(itListener);
				result = true;
				break;
			}
			itListener++;
		}
		fLock.Unlock();
	}
	return result;
}


sLONG VLogger::Next(sLONG inIndex)
{
	sLONG	nextIndex = inIndex + 1;
	if (nextIndex >= K_NB_MAX_BAGS)
	{
		nextIndex = 0;
	}
	return nextIndex;
}

void VLogger::LogBag( const VValueBag *inMessage)
{
	if ( ShouldLog(ILoggerBagKeys::level.Get(inMessage)) )
	{
		fLock.Lock();
		sLONG	nextIndex = Next(fEndIndex);

		if (nextIndex == fStartIndex)
		{
			fMessagesLost = true;
		}
		else
		{
			inMessage->Retain();
			fValues[fEndIndex] = inMessage;
			fEndIndex = nextIndex;
		}

		fLock.Unlock();
	}
}


void VLogger::LogMessage( ELog4jMessageLevel inLevel, const VString& inMessage, const VString& inSourceIdentifier)
{
	VValueBag* bag = new VValueBag;

	if (!inSourceIdentifier.IsEmpty())
	{
		ILoggerBagKeys::source.Set( bag, inSourceIdentifier);
	}
	else
	{
		bool sourceIdentifierSet = false;

		const VValueBag* properties = VTask::GetCurrent()->RetainProperties();
		if (properties != NULL)
		{
			VString source;
			sourceIdentifierSet = properties->GetString( ILoggerBagKeys::source, source);
			if (sourceIdentifierSet)
				ILoggerBagKeys::source.Set( bag, source);
		}
		ReleaseRefCountable( &properties);

		if (!sourceIdentifierSet)
			ILoggerBagKeys::source.Set( bag, VProcess::Get()->GetLogSourceIdentifier());
	}

	ILoggerBagKeys::level.Set( bag, inLevel);
	ILoggerBagKeys::message.Set( bag, inMessage);

	LogBag( bag);

	ReleaseRefCountable(&bag);
}


bool VLogger::ShouldLog( EMessageLevel inMessageLevel) const
{
	return fIsStarted && ((fFilter & (1 << inMessageLevel)) != 0);
}


bool VLogger::WithTrace(bool inWithTag)		{ return WithTag(EML_Trace,		 	inWithTag); }
bool VLogger::WithDump(bool inWithTag)		{ return WithTag(EML_Dump,		 	inWithTag); }
bool VLogger::WithDebug(bool inWithTag)		{ return WithTag(EML_Debug,		 	inWithTag); }
bool VLogger::WithWarning(bool inWithTag)	{ return WithTag(EML_Warning,	 	inWithTag); }
bool VLogger::WithError(bool inWithTag)		{ return WithTag(EML_Error,		 	inWithTag); }
bool VLogger::WithFatal(bool inWithTag)		{ return WithTag(EML_Fatal,		 	inWithTag); }
bool VLogger::WithAssert(bool inWithTag)	{ return WithTag(EML_Assert,	 	inWithTag); }
bool VLogger::WithInfo(bool inWithTag)		{ return WithTag(EML_Information, 	inWithTag); }

bool VLogger::IsStarted() const
{
	return fIsStarted;
}

bool VLogger::WithTag(uLONG inTag, bool inFlag)
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

sLONG VLogger::Read(std::vector<const VValueBag*>& ioValuesVector,bool inAlreadyLocked )
{
	sLONG	nbRead = 0;
	ioValuesVector.clear();
	if (fStartIndex != fEndIndex)
	{
		if (!inAlreadyLocked)
		{
			fLock.Lock();
		}
		nbRead = fEndIndex - fStartIndex;
		if (nbRead < 0)
		{
			nbRead += K_NB_MAX_BAGS;
		}
		/*if (inSize < nbRead)
		{
			nbRead = inSize;
		}*/
		sLONG curtIndex = fStartIndex;
		for( sLONG idx = 0; idx < nbRead; idx++ )
		{
			ioValuesVector.push_back(fValues[curtIndex]);
			fValues[curtIndex] = NULL;
			curtIndex = Next(curtIndex);
		}
		fStartIndex = curtIndex;
		if (!inAlreadyLocked)
		{
			fLock.Unlock();
		}
	}

	return nbRead;
}


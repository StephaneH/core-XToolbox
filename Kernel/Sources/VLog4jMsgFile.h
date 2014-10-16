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
#ifndef __VLog4jMsgFileLogger__H__
#define __VLog4jMsgFileLogger__H__

#if 1

#include "Kernel/Sources/VSplitableLogFile.h"


BEGIN_TOOLBOX_NAMESPACE




class XTOOLBOX_API VLog4jMsgFileLogger : public VObject, private VSplitableLogFile::IDelegate
{
public:
			class IReader
			{
			public:
				virtual	void	DoStart( const VString& inFilePath) = 0;
			};

			VLog4jMsgFileLogger( const VFolder& inLogFolder, const VString& inLogName);
	virtual ~VLog4jMsgFileLogger();

			/** @brief	Start message logging using the highest log number. Call Start() to continue the logging using last used log file. */
			// inherited from ILogger
	virtual	void					Start();
			/** @brief	Start message logging using the file "LogName_LogNumber.txt".
						Once the file has reached the maximum size, the log number is increased and a new log file is created. */
			void					Start( sLONG inLogNumber);

			// inherited from ILogger
	virtual	void					Stop();
			
			// fast way to know if the log is opened.
			// but remember that the log can be opened or closed from another thread while you are calling IsStarted().
			// inherited from ILogger
	virtual	bool					IsStarted() const;
			void					Flush();


			VTime					GetStartTime() const { return fStartTime; }
			void					GetLogFolderPath( VFilePath& outPath) const;
			void					GetLogName( VString& outName) const;
			
	static	void					BuildLogFileName( const VString& inLogName, sLONG inLogNumber, VString& outName);

	static	void					GetLogInfosFromBag( const VValueBag *inMessage, VString& outLoggerID, EMessageLevel& outLevel, VString& outMessage );


			void					SetLevelFilter( uLONG inFilter)			{ fFilter = inFilter;}
			uLONG					GetLevelFilter() const					{ return fFilter;}
			bool					ShouldLog( EMessageLevel inMessageLevel) const;

private:
	class VLog4jLogListener : public XBOX::VObject, public XBOX::ILogListener
	{
	public:
															VLog4jLogListener(VLog4jMsgFileLogger* inLogger) {fLogger = inLogger;}
		virtual	void										Put( std::vector< const XBOX::VValueBag* >& inValuesVector );

	private:
				void										Log( const VString& inLoggerID, EMessageLevel inLevel, const VString& inMessage);
				void										Log( const char* inLoggerID, EMessageLevel inLevel, const char* inMessage);

				VLog4jMsgFileLogger*						fLogger;
	};

			// Inherited from VSplitableLogFile::IDelegate
	virtual	void					DoCreateNewLogFile( const VString& inFilePath);

			bool					_IsStarted() const;

			bool					WithTag(uLONG inTag, bool inFlag);

	
	mutable	VCriticalSection		fLock;

			VTime					fStartTime;
			VFilePath				fFolderPath;
			VString					fLogName;
			VSplitableLogFile*		fOutput;
			bool					fIsStarted;	// to avoid an expensive lock on fLock just to know if we should log something
			uLONG					fFilter;	// bitfield to know what we are supposed to log

			VLog4jLogListener		fLog4jLogListener;

};




class XTOOLBOX_API VLog4jMsgFileLogReader : public VObject, public IRefCountable, public VLog4jMsgFileLogger::IReader
{
public:
			VLog4jMsgFileLogReader( const VFolder& inLogFolder, const VString& inLogName);

	virtual	~VLog4jMsgFileLogReader();

			/** @brief	Return the last logged messages since last PeekMessages() call. */
			void				PeekMessages( VectorOfVString& outMessages) const;

private:
			// Inherited from VLog4jMsgFileLogger::IReader
	virtual	void				DoStart( const VString& inFilePath);

			void				_PeekMessages( VectorOfVString& outMessages) const;

			FILE*				fFile;
			VSplitableLogFile*	fOutput;
	mutable	VectorOfVString		fMessages;
	mutable	VCriticalSection	fLock;
};




END_TOOLBOX_NAMESPACE

#endif
#endif
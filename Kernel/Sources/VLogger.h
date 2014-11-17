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
#ifndef __VLogger__
#define __VLogger__


#include "Kernel/Sources/VObject.h"
#include "Kernel/Sources/VFilePath.h"
#include "Kernel/Sources/VString.h"
#include "Kernel/Sources/VTime.h"
#include "Kernel/Sources/ILogger.h"


BEGIN_TOOLBOX_NAMESPACE


class VFolder;




/** @brief	Very simple implementation of Log4j format logger using a file as log output. */

enum
{
	eL4JML_Trace		= EML_Trace,
	eL4JML_Debug		= EML_Debug,
	eL4JML_Information	= EML_Information,
	eL4JML_Warning		= EML_Warning,
	eL4JML_Error		= EML_Error,
	eL4JML_Fatal		= EML_Fatal
};
typedef EMessageLevel ELog4jMessageLevel;



class XTOOLBOX_API VLogger : public VObject, public ILogger
{
public:

	enum { kLoggerTaskKind = 'LOGG' };

			VLogger();// const VFolder& inLogFolder, const VString& inLogName);
	virtual ~VLogger();


			/** @brief	Start message logging using the highest log number. Call Start() to continue the logging using last used log file. */
			// inherited from ILogger
	virtual	void					Start();
			/** @brief	Start message logging using the file "LogName_LogNumber.txt".
						Once the file has reached the maximum size, the log number is increased and a new log file is created. */

			// inherited from ILogger
	virtual	void					Stop();
			
			// fast way to know if the log is opened.
			// but remember that the log can be opened or closed from another thread while you are calling IsStarted().
			// inherited from ILogger
	virtual	bool					IsStarted() const;

			// inherited from ILogger
	virtual	void					LogBag( const VValueBag *inMessage);

	virtual	void					LogMessage( EMessageLevel inLevel, const VString& inMessage, const VString& inSourceIdentifier);

	virtual	bool					ShouldLog( EMessageLevel inMessageLevel) const;

	virtual bool					WithTrace	(bool inWithTag);
	virtual bool					WithDump	(bool inWithTag);
	virtual bool					WithDebug	(bool inWithTag);
	virtual bool					WithWarning	(bool inWithTag);
	virtual bool					WithError	(bool inWithTag);
	virtual bool					WithFatal	(bool inWithTag);
	virtual bool					WithAssert	(bool inWithTag);
	virtual bool					WithInfo	(bool inWithTag);

			uLONG					GetLevelFilter() const					{ return fFilter;}
			void					SetLevelFilter( uLONG inFilter)			{ fFilter = inFilter;}

	virtual void					Flush();
			bool					AddLogListener(ILogListener* inLogListener);
			bool					RemoveLogListener(ILogListener* inLogListener);

private:
			bool					WithTag(uLONG inTag, bool inFlag);

	#define K_NB_MAX_BAGS			(1000)
			const VValueBag*		fValues[K_NB_MAX_BAGS];
			sLONG					fStartIndex;	
			sLONG					fEndIndex;
	mutable	VCriticalSection		fLock;
			bool					fMessagesLost;
			VFilePath				fFolderPath;
			VString					fLogName;
			//VSplitableLogFile*		fOutput;
			uLONG					fFilter;	// bitfield to know what we are supposed to log
			bool					fIsStarted;	// to avoid an expensive lock on fLock just to know if we should log something

			sLONG					Read(std::vector<const VValueBag*>& ioValuesVector, bool inAlreadyLocked = false);

			sLONG					Next(sLONG inIndex);

			std::vector<ILogListener*>		fLogListeners;
	static	sLONG				LogReaderTaskProc(XBOX::VTask* inTask);
		XBOX::VTask*			fLogReaderTask;

};

END_TOOLBOX_NAMESPACE


#endif
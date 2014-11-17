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
#ifndef __VSplitableLogFile__H__
#define __VSplitableLogFile__H__

#if 1

#include "Kernel/Sources/VLogger.h"


BEGIN_TOOLBOX_NAMESPACE


/** @brief	Splitable log file management (no thread-safe)

			Each log file is named "BaseName_LogNumber.txt".
			Once the file has reached the maximum size (actually 10Mo), the log number is increased and a new log file is created.
			Use AppendFormattedString() or call CheckLogSize() before adding any data in the file.
*/

class XTOOLBOX_API VSplitableLogFile : public VObject
{
public:
			class IDelegate
			{
			public:
				virtual	void	DoCreateNewLogFile( const VString& inFilePath) = 0;
			};

			VSplitableLogFile( const VFolder& inBasePath, const VString& inBaseName, sLONG inStartNumber = 1);
	virtual ~VSplitableLogFile();

			/**	@brief	Open the log file using current log number.
						if inCreateEmptyFile is true, the content of an existing file is erased. */
			bool		Open( bool inCreateEmptyFile);
			
			/**	@brief	Open the log file using inLogNumber.
						if inCreateEmptyFile is true, the content of an existing file is erased. */
			bool		Open( sLONG inLogNumber, bool inCreateEmptyFile);

			/**	@brief	Standard output redirection. */
			void		OpenStdOut();

			void		Close();
			void		Flush();
			bool		IsOpen() const {return fFile != NULL;}

			/**	@brief	Change the current log file number.
						if inCreateEmptyFile is true, the content of an existing file is erased. */
			void		SetLogNumber( sLONG inLogNumber, bool inCreateEmptyFile);
			sLONG		GetLogNumber() const {return fLogNumber;}

			/** @brief	CheckLogSize() should be called before adding any data in the file.
						Returns 0 if the current log file has not reached the maximum size or returns the log number of the newly created log file. */
			sLONG		CheckLogSize();

			/** @brief	Returns the amount of bytes remaining before a split occurs. This lets the caller to finish
						writting some entries, then call SetLogNumber( GetLogNumber() + 1 ) to start a new log.
						Typically : when writting xml and you want the xml to be properly closed before a new log is started.
						Note that the returned value may be < 0 (next call to CheckLogSize() will split the file)
						*/
			sLONG		CheckRemainingBytes() const;


			/** @brief	Same as printf.
						CheckLogSize() method is called before append the string so a new log file may be created. */
			void		AppendFormattedString( const char* inFormat, ...);
			void		AppendString( const char* inString);
            void        AppendString( const VString& inString);

			// Accessors
			FILE*		GetFile() {return fFile;}
			void		GetCurrentFileName( VString& outName) const;
			void		GetCurrentFilePath( VString& outPath) const {_BuildFilePath( outPath); }
			void		GetFolderPath( VFilePath& outPath) const {outPath = fFolderPath;}

			/**	@brief	The splitable log file never owns the delegate. */
			void		SetDelegate( IDelegate *inDelegate);

	static	void		BuildLogFileName( const VString& inBaseName, sLONG inLogNumber, VString& outName);

private:
			void		_BuildFilePath( VString& outPath) const;

			FILE		*fFile;
			VFilePath	fFolderPath;
			VString		fLogName;
			sLONG		fLogNumber;
			IDelegate	*fDelegate;
            CharSet     fCharSet;
};




END_TOOLBOX_NAMESPACE

#endif
#endif
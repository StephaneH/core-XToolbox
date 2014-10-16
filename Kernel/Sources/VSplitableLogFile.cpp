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
#include "VSplitableLogFile.h"


const sLONG kSPLITABLE_LOG_FILE_MAX = 10485760; // 10 * 1024L * 1024L

VSplitableLogFile::VSplitableLogFile( const VFolder& inBasePath, const VString& inBaseName, sLONG inStartNumber)
: fCharSet( VTC_UTF_8)
, fFile( NULL)
, fLogNumber( inStartNumber)
, fLogName( inBaseName)
, fFolderPath( inBasePath.GetPath())
, fDelegate( NULL)
{
}


VSplitableLogFile::~VSplitableLogFile()
{
	Close();
}


bool VSplitableLogFile::Open( bool inCreateEmptyFile)
{
	if (fFile == NULL)
	{
		VString vpath;
		_BuildFilePath( vpath);
		if(!vpath.IsEmpty())
		{
			StStringConverter<char> convert( VTC_StdLib_char);
			const char *cpath = convert.ConvertString( vpath);
			
			if (inCreateEmptyFile)
			{
				fFile = ::fopen( cpath, "w");
				if (fDelegate != NULL)
					fDelegate->DoCreateNewLogFile( vpath);
			}
			else
			{
				// Check if file exists
				fFile = ::fopen( cpath, "r");
				if (fFile != NULL)
				{
					fFile = ::freopen( cpath, "a", fFile);
				}
				else
				{
					fFile = ::fopen( cpath, "w");
					if (fDelegate != NULL)
						fDelegate->DoCreateNewLogFile( vpath);
				}
			}
		}
	}
	return fFile!=NULL;
}


bool VSplitableLogFile::Open( sLONG inLogNumber, bool inCreateEmptyFile)
{
	fLogNumber = inLogNumber;
	return Open( inCreateEmptyFile);
}


void VSplitableLogFile::OpenStdOut()
{
	if ( (fFile != NULL) && (fFile != stdout) )
		Close();
	fFile = stdout;
}


void VSplitableLogFile::Close()
{
	if (fFile != NULL)
	{
		if (fFile != stdout)
		{
			::fclose( fFile);
		}
		fFile = NULL;
	}
}


void VSplitableLogFile::Flush()
{
	if(fFile != NULL)
	{
		::fflush( fFile);
	}
}


void VSplitableLogFile::SetLogNumber( sLONG inLogNumber, bool inCreateEmptyFile)
{
	fLogNumber = inLogNumber;
	if(IsOpen())
	{
		Close();
		Open( inCreateEmptyFile);
	}
}


sLONG VSplitableLogFile::CheckLogSize()
{
	sLONG res = 0;
	if (fFile != NULL)
	{
		// check size, segments the log in 10 Mo chunks
		long pos = ::ftell( fFile);
		if (pos > kSPLITABLE_LOG_FILE_MAX)
		{
			Close();

			++fLogNumber;

			Open( true);

			res = fLogNumber;
		}
	}
	return res;
}

sLONG VSplitableLogFile::CheckRemainingBytes() const
{
	if (fFile != NULL)
		return (sLONG) (kSPLITABLE_LOG_FILE_MAX - ::ftell( fFile));

	return 0;
}


void VSplitableLogFile::AppendFormattedString( const char* inFormat, ...)
{
	CheckLogSize();

	if (fFile != NULL)
	{
		va_list argList;

		va_start(argList, inFormat);
		::vfprintf( fFile, inFormat, argList);
		va_end( argList);
	}
}


void VSplitableLogFile::AppendString( const char* inString)
{
	CheckLogSize();

	if (fFile != NULL)
	{
		::fputs( inString, fFile);
	}
}


void VSplitableLogFile::AppendString( const VString& inString)
{
    CheckLogSize();

	if (fFile != NULL)
	{
        StStringConverter<char>	converter( fCharSet);
        fputs( converter.ConvertString( inString), fFile);
	}
}


void VSplitableLogFile::GetCurrentFileName( VString& outName) const
{
	VSplitableLogFile::BuildLogFileName( fLogName, fLogNumber, outName);
}


void VSplitableLogFile::BuildLogFileName( const VString& inBaseName, sLONG inLogNumber, VString& outName)
{
	outName.Clear();
	outName += inBaseName;
	outName += "_";
	outName.AppendLong( inLogNumber);
	outName += ".txt";
}


void VSplitableLogFile::SetDelegate( IDelegate *inDelegate)
{
	fDelegate = inDelegate;
}


void VSplitableLogFile::_BuildFilePath( VString& outPath) const
{
	if(!fFolderPath.IsEmpty())
	{
		VString fname;
	#if VERSIONMAC
		fFolderPath.GetPosixPath( outPath);
		if (outPath[outPath.GetLength() - 1] != '/')
			outPath += '/';
	#else
		fFolderPath.GetPath( outPath);
	#endif
		GetCurrentFileName( fname);
		outPath += fname;
	}
	else
	{
		outPath.Clear();
	}
}


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
#ifndef __VCommandLineParser__
#define __VCommandLineParser__

#include "Kernel/VKernel.h"
#include "Kernel/Sources/VNonCopyable.h"
#include "Kernel/Sources/VString.h"
#include "VCommandLineParserOption.h"




BEGIN_TOOLBOX_NAMESPACE


/*!
** \brief A command line options parser implementation [getopt]
**
** ## Forms of arguments
**
** All possible forms of arguments (case sensitive):
**
**  * `-a -B -c`
**  * `-a valueA -B valueB -c valueC`
**  * `-aBc valueA valueB valueC`
**  * `--opt-a --opt-b --opt-c`
**  * `--opt-a valueA --opt-b valueB --opt-c valueC`
**  * `--opt-a=valueA --opt-b=valueB --opt-c=valueC`
**  * `--opt-a --opt-b --opt-c valueA valueB valueC`
**
** Windows only :
**
**  * `/a valueA /B valueB /c valueC`
**  * `/a:valueA /B:valueB /c:valueC`
**  * `/a=valueA /B=valueB /c=valueC` [for UNIX compatibility]
**  * `/a /b /c valueA valueB valueC`
**
** Help :
**
**  * `--help`, `-h`
**  * `/help`, `/?`, `/h`  [windows only]
**
** The special argument ‘--’ forces in all cases the end of option scanning (the remaining
** arguments will be considered as mere values).
**
**
** ## Help Usage
**
** The help is automatically generated according the provided command line options.
** (and the standard flags -h / --help are automatically provided).
**
**
** ## Supported C++ types :
**
**  * `sLONG`, `sLONG8`
**  * `uLONG`, `uLONG8`
**  * `VString`
**  * `bool`
**  * `std::vector<T>`, with `T` one of the supported type
**  * `VRefPtr<T>`, with `T` one of the supported type
**  * `VRefPtr<VFile>`, specialization dedicated to filename resolution (must exist)
**  * `VRefPtr<VFolder>`, specialization dedicated to folder resolution (must exist)
**  * `VRefPtr<VFilePath>`, specialization dedicated to file url resolution
**
**
** ## How to implement a command line parsing
**
** For using this command line parser, the method `DoParseCommandLine` from `VProcessIPC`
** must be overloaded. Here is a minimal example :
**
** \code
** virtual VCommandLineParser::Error VMyApplication::DoParseCommandLine(VCommandLineParser& inParser)
** {
**	// example of a long option
**	VString optFilename;
**	inParser.Add(&optFilename, 'i', CVSTR("input"), CVSTR("A filename"));
**
**	// example of a short option
**	bool optVerbose = false;
**	inParser.AddFlag(&optVerbose, ' ', CVSTR("verbose"), CVSTR("Enable the verbose mode"));
**
**	VCommandLineParser::Error err = inParser.Parse(argc, argv);
**	if (VCommandLineParser::eOK == err): // the parse succeeded, let's continue
**	{
**		// print the input filename
**		console << "input: " << optFilename << std::endl;
**		// print if the verbose has been activated or not
**		console << "verbose: " << optVerbose << std::endl;
**
**		if (0 == optFilename.GetLength())
**			console.error << "please provide a filename (-i/--input)" << std::endl;
**	}
**	return err;
** }
** \endcode
**
** A few notes :
**
**  * An option may have both a short name (`-e`) and a long name (`--some-opt` or `/some-opt`)
**  * An option (`-e, --some-opt`) may have an additionnal value
**    (via the method `Add()`). Otherwise it is a flag (bool, via `AddFlag()`)
**  * To avoid issues, a option flag can only accept the boolean type
**  * All options must have a description
**
**
** ## Hidding options from the help usage
**
** A command line option can be hidden using `Hide(<short option name>)` or
** `Hide(<long option name>)`
**
**
** ## How to handle arguments without options
**
** Sometimes, we just only need to grab a list of something, without any option attached to it.
** We can use the method `SetRemainingArgument()`.
**
** To retrieve a list of numbers (ex `./a.out 42 3 45  23 5 6`)
** \code
** std::vector<sLONG> allNumbers;
** inParser.SetRemainingArguments(&allNumbers);
** \endcode
**
** To retrieve a list of filenames (ex `./a.out test.txt  /tmp/test.log`)
** \code
** std::vector<VRefPtr<VFile> > filenames;
** inParser.SetRemainingArguments(&filenames);
** \endcode
**
**
** ## How to efficiently handle filenames from the command line (with .LNK resolution):
**
** \code
** virtual VCommandLineParser::Error VMyApplication::DoParseCommandLine(VCommandLineParser& inParser)
** {
**	// Using a VFile for input filename parameters. Files that exists
**	// with a .lnk extension will be automatically resolved
**	VRefPtr<VFile> inputFilename;
**	inParser.Add(&inputFilename, 'i', "input", CVSTR("A filename"));
**
**	VCommandLineParser::Error err = inParser.Parse(argc, argv);
**	if (VCommandLineParser::eOK == err): // the parse succeeded, let's continue
**	{
**		// print the input filename
**		if (!(!inputFilename))
**		{
**			// does something with the input filename
**			VString filename = inputFilename->GetPath()->GetPath();
**			console << filename << std::endl;
**		}
**	}
**	return err;
** }
** \endcode
**
** \internal For portability, only US-ASCII are allowed in option names
** \note On Windows, mixing unix-style patterns and windows-style patterns are disallowed
*/
class XTOOLBOX_API VCommandLineParser : public VObject, public VNonCopyable<VCommandLineParser>
{
public:
	//! Error for command line parsing
	enum Error
	{
		//! The parse is complete
		eOK,
		//! The parse has failed (invalid parameter, unknown argument, ...)
		eFailed,
		//! The parse is complete but the program should quit
		//  (the help usage has been printed, or the version or something else...)
		eShouldExit
	};


public:
	/*!
	** \brief Print an error message to the stderr and returns VE_INVALID_PARAMETER
	**
	** \param inText Any message to print
	** \return Always VE_INVALID_PARAMETER
	*/
	static Error PrintArgumentError(const VString& inText);

	/*!
	** \brief Print a message to the stdout
	** \param inText Any message to print
	*/
	static void Print(const VString& inText);


public:
	//! \name Constructors & Destructor
	//@{
	/*!
	** \brief Default constructor, with all arguments
	*/
	explicit VCommandLineParser(const std::vector<VString>& inArgs);
	//! Destructor
	~VCommandLineParser();
	//@}


	//! \name Options management
	//@{
	/*!
	** \brief Append a new command line option which requires an input value
	**
	** \param in A target variable
	** \param inShort The short option name (space if it does not exist)
	** \param inLong  The long option name (empty if it does not exist)
	** \param inDescription Any comments, with any characters (linefeeds or not)
	** \tparam T Any supported type
	*/
	template<class T> void Add(T* out, char inShortName, const char* inLongName, const VString& inDescription);

	/*!
	** \brief Append a new command line flag
	**
	** \param in A target variable
	** \param inShort The short option name (space if it does not exist)
	** \param inLong  The long option name (empty if it does not exist)
	** \param inDescription Any comments, with any characters (linefeeds or not)
	** \tparam T Any supported type
	*/
	void AddFlag(bool* out, char inShortName, const char* inLongName, const VString& inDescription);

	/*!
	** \brief Append a new arbitrary text, for additional custom content in the help
	**
	** \param inDescription Any comments, with any characters (linefeeds or not)
	*/
	void AddParagraph(const VString& inDescription);

	/*!
	** \brief Set the target variable where all arguments without options will be written
	**
	** \param in A target variable
	** \tparam T Any supported type
	*/
	template<class T> void SetRemainingArguments(T* out);

	/*!
	** \brief Clear all
	**
	** This method should never be used, except maybe for two scenarii :
	**   - for parsing several times the same command line (for providing a
	**     different options set according some input values
	**     like `valgrind --tool`)
	**   - when it is not possible to get rid of the instance of
	**     VCommandLineParser. This method will release all internal variables.
	*/
	void Clear();

	//! Hide an option from its short name
	void Hide(UniChar inShortName);
	//! Hide an option from its long name
	void Hide(const char* inLongName);
	//@}


	//! \name Command line parsing
	//@{
	/*!
	** \brief Try to parse the command line
	**
	** \return True if the parse is complete. False if the program should quit. Use
	**   the method `HasError()` to determine if the program should quit due to an error
	** \see Add()
	** \see AddFlag()
	** \see AddParagraph()
	** \see HasError()
	*/
	Error Parse();
	//@}


	//! \name Debugging tools / Help generator
	//@{
	/*!
	** \brief Print all arguments as list
	**
	** \param inErrOutput True to print to the standard error output
	*/
	void PrintAllArguments(bool inErrOutput = false);

	/*!
	** \brief Export informations as JSON object
	**
	** \param out The output string (must not be null)
	*/
	void ExportAsJSON(VString* out);

	/*!
	** \brief Export the options' descriptions as a piece of a man page
	**
	** \param out The output string (must not be null)
	*/
	void ExportAsManPage(VString* out);
	//@}


private:
	//! Informations about all options
	Private::CommandLineParser::StOptionsInfos fOptions;

}; // class VCommandLineParser












template<class T>
inline void VCommandLineParser::Add(T* out, char inShortName, const char* inLongName, const VString& inDescription)
{
	xbox_assert((out != NULL) && "invalid argument");
	if (fOptions.mayRequireOptions)
		fOptions.RegisterOption(true, new Private::CommandLineParser::VArgOption<T>(*out, inShortName, inLongName, inDescription, true));
}


inline void VCommandLineParser::AddFlag(bool* out, char inShortName, const char* inLongName, const VString& inDescription)
{
	xbox_assert((out != NULL) && "invalid argument");
	if (fOptions.mayRequireOptions)
		fOptions.RegisterOption(false, new Private::CommandLineParser::VArgOption<bool>(*out, inShortName, inLongName, inDescription));
}


template<class T>
inline void VCommandLineParser::SetRemainingArguments(T* out)
{
	// It is not safe to call this method several times (will cause memory leak)
	xbox_assert((fOptions.remainingOption == NULL)
		&& "multiple use of the method `SetRemainingArguments` should be avoided");

	if (fOptions.mayRequireOptions)
		fOptions.ResetRemainingArguments(new Private::CommandLineParser::VArgOption<T>(*out));
}


inline void VCommandLineParser::AddParagraph(const VString& inComments)
{
	if (fOptions.mayRequireDocumentation)
		fOptions.options.push_back(new Private::CommandLineParser::VOptionBase(inComments));
}


inline void VCommandLineParser::Hide(UniChar inShortName)
{
	if (fOptions.mayRequireOptions)
		fOptions.HideOption(inShortName);
}

inline void VCommandLineParser::Hide(const char* inLongName)
{
	if (fOptions.mayRequireOptions && '\0' != *inLongName)
		fOptions.HideOption(inLongName);
}



END_TOOLBOX_NAMESPACE

#endif // __VCommandLineParser__

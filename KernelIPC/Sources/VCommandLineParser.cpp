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
#include "VKernelIPCPrecompiled.h"
#include "VCommandLineParser.h"
#include <iostream>







BEGIN_TOOLBOX_NAMESPACE


namespace // anonymous
{

	enum ArgType
	{
		//! The argument is a short option name (-a)
		ttShortOption,
		//! The argument is a long option name (--option)
		ttLongOption,
		//! The argument is a parameter (value)
		ttParameter,
		//! The argument is a windows style argument, and may short or long (/myoption, /y)
		ttBoth,
		//! The argument shall be ignored (most likely a double dash '--')
		ttIgnore,
	};


	enum ArgumentStyle
	{
		//! UNIX style, --option=value or --option value
		asUnixStyle,
		//! Windows style  /option:value or /option value
		asWindowsStyle,
		//! Unknown style
		asUnknown,
	};


	//! Convenient alias to Error
	typedef XBOX::Private::CommandLineParser::Error Error;




	static Error RaiseUnknownOption(const VString& inName)
	{
		VString msg;
		msg += CVSTR("error: unknown option `");
		msg += inName;
		msg += CVSTR("`\n");
		VCommandLineParser::PrintArgumentError(msg);
		return Private::CommandLineParser::eFailed;
	}


	static Error RaiseMissingShortOption(const VString& inName)
	{
		VString msg;
		msg += CVSTR("error: issing short option `");
		msg += inName;
		msg += CVSTR("`\n");
		VCommandLineParser::PrintArgumentError(msg);
		return Private::CommandLineParser::eFailed;
	}


	static Error RaiseMixArgumentStyle(const VString& inName)
	{
		VString msg;
		msg += CVSTR("error: mixing up unix-style and windows-style arguments  `");
		msg += inName;
		msg += CVSTR("`\n");
		VCommandLineParser::PrintArgumentError(msg);
		return Private::CommandLineParser::eFailed;
	}


	static Error RaiseInvalidValue(const VString& inName)
	{
		VString msg;
		msg += CVSTR("error: invalid value for option  `");
		msg += inName;
		msg += CVSTR("`\n");
		VCommandLineParser::PrintArgumentError(msg);
		return Private::CommandLineParser::eFailed;
	}


	static Error RaiseValueNotRequired(const VString& inName)
	{
		VString msg;
		msg += CVSTR("error: a value for option `");
		msg += inName;
		msg += CVSTR("` is not required\n");
		VCommandLineParser::PrintArgumentError(msg);
		return Private::CommandLineParser::eFailed;
	}


	static Error RaiseMissingValue(const Private::CommandLineParser::VOptionBase& inOption)
	{
		VString msg = CVSTR("missing value for option ");
		inOption.AppendFullName(msg);
		VCommandLineParser::PrintArgumentError(msg);
		return Private::CommandLineParser::eFailed;
	}


	template<class T>
	static inline void AppendEscapedString(VString& out, VString& tmp, const T& in)
	{
		tmp.Clear();
		in.GetJSONString(tmp);
		out += tmp;
	}








	class ParserContext /*final*/
	{
	public:
		//! Default constructor
		ParserContext(Private::CommandLineParser::StOptionsInfos& inInfos);
		//! Get if we had encountered some errors (fatal)
		bool HasErrors() const;

		Error operator () ();


	private:
		//! Try to analyze a long option name (such as --key, --key=value or /key)
		Error AnalyzeLongOptionName(const VString& name, ArgumentStyle style);
		//! Try to analyze a long option name with an attached value to it
		// (such as --key=value)
		Error AnalyzeLongOptionNameWithAttachedValue(const VString& inName, VIndex sub);
		//! Try to analyze a long option name without any value attached to it
		// (such as --key or /key)
		Error AnalyzeLongOptionNameWithoutValue(const VString& inName);
		//! Try to analyze short option names (such as -aBc, where 'a', 'B' and 'c' might be options)
		Error AnalyzeShortOptionName(const VString& inArg);

		//! Try to find an argument typed as `parameter`
		bool FindNextParameter(VString* out = NULL);
		//! Determine the type of a single argument according its first characters
		ArgType DetermineArgumentType(const VString& inArg, ArgumentStyle& outStyle);

		#if VERSIONMAC
		//! Detect invalid short arguments on OSX
		bool InterceptOSXInvalidShortArguments(const VString& inArg);
		#endif

	private:
		//! The last parameter index
		uLONG fParameterIndex;
		//! Temporary string for option name manipulation
		VString fTmp;

		// flag for forcing the parameter type for an argument,
		// espcially usefull if we encounter a double dash '--'
		bool fForceParameterType;
		//! All arguments passed from the command line
		const std::vector<VString>& fArgs;

		//! Encountered errors ?
		bool fHasError;

		//! Informations about all comman line options
		Private::CommandLineParser::StOptionsInfos& fInfos;

	}; // class ParserContext





	inline ParserContext::ParserContext(Private::CommandLineParser::StOptionsInfos& inOptions)
		: fParameterIndex(1u)
		, fForceParameterType(false)
		, fArgs(inOptions.args)
		, fHasError(false)
		, fInfos(inOptions)
	{
		xbox_assert(fArgs.size() > 1
			&& "No parsing should take place if there is nothing to parse");
	}


	#if VERSIONMAC
	bool ParserContext::InterceptOSXInvalidShortArguments(const VString& inArg)
	{
		// On OSX, we may get some invalid arguments, like a process serial number
		// (Carbon, deprecated on 10.9 - see VProcessIPC.cpp)

		// -ApplePersistenceIgnoreState YES
		if (inArg.EqualToUSASCIICString("-ApplePersistenceIgnoreState"))
		{
			FindNextParameter(); // ignore the next parameter
			return true;
		}

		// -NSDocumentRevisionsDebugMode YES
		if (inArg.EqualToUSASCIICString("-NSDocumentRevisionsDebugMode"))
		{
			FindNextParameter(); // ignore the next parameter
			return true;
		}

		return false;
	}
	#endif


	inline ArgType ParserContext::DetermineArgumentType(const VString& inArg, ArgumentStyle& outStyle)
	{
		// argument length
		uLONG length = (uLONG) inArg.GetLength();

		if (!fForceParameterType && length > 0) // ignore empty strings
		{
			// Trying UNIX like options
			// we may return here only '--' in some cases
			if (inArg[0] == UniChar('-'))
			{
				outStyle = asUnixStyle;
				if (length > 1 && inArg[1] == UniChar('-'))
				{
					if (length == 2)
					{
						// just encountered '--', which is nothing and should force
						// to consider all other arguments as raw parameters
						fForceParameterType = true;
						return ttIgnore;
					}
					return ttLongOption;
				}
				return ttShortOption;
			}

			if (XBOX::Private::CommandLineParser::eWindowsStyleEnabled)
			{
				// Trying Windows like options
				if (inArg[0] == UniChar('/'))
				{
					outStyle = asWindowsStyle;
					switch (length)
					{
						case 1:
							{
								// Arguments with only a slash should be considered as a parameter
								// and not an option ()
								return ttParameter;
							}
						default:
							{
								// A double-slash should be considered as a path too
								return (inArg[1] == UniChar('/')) ? ttParameter : ttBoth;
							}
					}
				}
			}
		}

		// fallback, obviously a parameter
		outStyle = asUnknown;
		return ttParameter;
	}



	bool ParserContext::FindNextParameter(VString* out)
	{
		ArgumentStyle style; // unused
		for (; fParameterIndex < (uLONG) fArgs.size(); ++fParameterIndex)
		{
			if (ttParameter == DetermineArgumentType(fArgs[fParameterIndex], style))
			{
				if (out)
					*out = fArgs[fParameterIndex];
				// This argument shall not be used again as a parameter
				++fParameterIndex;
				return true;
			}
		}
		return false;
	}


	Error  ParserContext::AnalyzeLongOptionNameWithAttachedValue(const VString& inName, VIndex inSub)
	{
		// the value has been directly provided
		fTmp.Clear();
		inName.GetSubString(1, inSub - 1, fTmp);

		Private::CommandLineParser::VOptionBase::MapByLongNames::iterator i = fInfos.byLongNames.find(fTmp);
		if (i != fInfos.byLongNames.end())
		{
			// does this option requires an additional value ?
			bool requiresAdditionalValue = i->second.second;
			if (requiresAdditionalValue)
			{
				// reusing fTmp - extracting the value from the argument
				fTmp.Clear();
				inName.GetSubString(inSub + 1, inName.GetLength() - inSub, fTmp);

				// append / reset
				return i->second.first->AppendArgument(fTmp);
			}

			// this option does not require an additional value
			// since we have something like `--key=value`, this is obviously an error
			return RaiseValueNotRequired(fTmp);
		}

		// Checking if not a virtual standard value which should not receive an additional value
		if (fTmp.EqualToUSASCIICString("help"))
			return RaiseInvalidValue(fTmp);

		// fallback - unknown option
		return RaiseUnknownOption(fTmp);
	}


	Error  ParserContext::AnalyzeLongOptionNameWithoutValue(const VString& inArg)
	{
		// No additional value has been provided yet
		Private::CommandLineParser::VOptionBase::MapByLongNames::iterator i = fInfos.byLongNames.find(inArg);
		if (i != fInfos.byLongNames.end())
		{
			// does this option requires an additional value ?
			bool requiresAdditionalValue = i->second.second;

			if (!requiresAdditionalValue)
			{
				// This option is merely a flag
				i->second.first->SetFlag();
			}
			else
			{
				// this option requires an additional, which should be found somewhere else
				// in the argument list (reusing fTmp)
				if (!FindNextParameter(&fTmp))
					return RaiseMissingValue(*(i->second.first));

				return i->second.first->AppendArgument(fTmp);
			}
			return Private::CommandLineParser::eOK;
		}

		// the option has not been found but this might be a `system` one
		if (inArg.EqualToUSASCIICString("help"))
			return Private::CommandLineParser::eShouldExit;

		// definitevelty an error
		return RaiseUnknownOption(inArg);
	}


	inline Error  ParserContext::AnalyzeLongOptionName(const VString& inArg, ArgumentStyle style)
	{
		// Is an additional value already provided ?  --param=value
		const VIndex sub = (style != asWindowsStyle)
			? inArg.FindUniChar('=')
			: inArg.FindUniChar(':');

		return (sub == 0)
			? AnalyzeLongOptionNameWithoutValue(inArg)
			: AnalyzeLongOptionNameWithAttachedValue(inArg, sub);
	}


	inline Error  ParserContext::AnalyzeShortOptionName(const VString& inArg)
	{
		#if VERSIONMAC
		// On OSX, we may get some invalid arguments, like a process serial number (-psn_)
		// or some debug parameters provided by XCode (like NSDocumentRevisionsDebugMode)
		// If the argument consists of a single parameter (like -psn_XXXXX), it would be more efficient
		// to completely discard it from the original command line inputs (and allow optimization when
		// no parameters are provided)
		if (InterceptOSXInvalidShortArguments(inArg))
			return Private::CommandLineParser::eOK;
		#endif

		// Argument length
		uLONG argLength = (uLONG) inArg.GetLength();

		// An empty short option is invalid
		if (argLength <= 1)
			return RaiseMissingShortOption(inArg);

		// skip the first char to avoid string replacement, which can be either '-' or '/'
		for (uLONG i = 1; i < argLength; ++i)
		{
			// the current short option name
			UniChar c = inArg[i];

			Private::CommandLineParser::VOptionBase::MapByShortNames::iterator it = fInfos.byShortNames.find(c);
			if (it != fInfos.byShortNames.end())
			{
				// does this option requires an additional value ?
				bool requiresAdditionalValue = it->second.second;

				if (!requiresAdditionalValue)
				{
					// This option is merely a flag
					it->second.first->SetFlag();
				}
				else
				{
					// this option requires an additional, which should be found somewhere else
					// in the argument list (reusing fTmp)
					if (!FindNextParameter(&fTmp))
						return RaiseMissingValue(*(it->second.first));

					Error err = it->second.first->AppendArgument(fTmp);
					if (err != Private::CommandLineParser::eOK)
						return Private::CommandLineParser::eFailed;
				}
			}
			else
			{
				// the option has not been found but this might be a `system` one
				if (c == UniChar('?') || c == UniChar('h'))
					return Private::CommandLineParser::eShouldExit;

				// This short option name is unknown
				fTmp = c;
				return RaiseUnknownOption(fTmp);
			}
		}
		return Private::CommandLineParser::eOK;
	}




	Error  ParserContext::operator () ()
	{
		// temporary string for argument manipulation
		VString tmp;
		// Previous argument style
		ArgumentStyle previousStyle = asUnknown; // dummy value


		// iterate through all arguments
		for (uLONG fTokenIndex = 1; fTokenIndex < (uLONG) fArgs.size(); ++fTokenIndex)
		{
			// the Nth argument - the current one actually
			const VString& argN = fArgs[fTokenIndex];
			// Argument style
			ArgumentStyle style;

			ArgType argumentType = DetermineArgumentType(argN, style);
			switch (argumentType)
			{
				case ttShortOption:
				{
					// re-adjust the index of the index of the current parameter if needed
					if (fTokenIndex >= fParameterIndex)
						fParameterIndex = fTokenIndex + 1; // +1 since the current index is an option

					// The method `AnalyzeShortOptionName` will ignore the first char
					Error err = AnalyzeShortOptionName(argN);
					if (err != Private::CommandLineParser::eOK)
						return err;
					break;
				}

				case ttLongOption:
				{
					// the current argument seems to be a long option name like --key or --key=value

					// re-adjust the index of the index of the current parameter if needed
					if (fTokenIndex >= fParameterIndex)
						fParameterIndex = fTokenIndex + 1; // +1 since the current index is an option

					// get rid of the two first char (--myoption)
					argN.GetSubString(2 + 1, argN.GetLength() - 2, tmp);
					if (tmp.GetLength() > 0)
					{
						Error err = AnalyzeLongOptionName(tmp, style);
						if (err != Private::CommandLineParser::eOK)
							return err;
					}
					else
					{
						// double dash - should consider all other arguments as
						// mere parameters, without option attached to it
						fForceParameterType = true;
					}
					break;
				}

				case ttParameter:
				{
					// the current argument is not attached to any option
					if (fTokenIndex >= fParameterIndex)
					{
						// re-adjust the index of the index of the current parameter just in case
						fParameterIndex = fTokenIndex;

						if (fInfos.remainingOption)
						{
							Error err = fInfos.remainingOption->AppendArgument(argN);
							if (err != Private::CommandLineParser::eOK)
								return Private::CommandLineParser::eFailed;
						}
						else
							return RaiseUnknownOption(argN);
					}
					else
					{
						// this parameter has already been used and shall be ignored
					}
					break;
				}

				case ttBoth:
				{
					if (XBOX::Private::CommandLineParser::eWindowsStyleEnabled)
					{
						// re-adjust the index of the index of the current parameter if needed
						if (fTokenIndex >= fParameterIndex)
							fParameterIndex = fTokenIndex + 1; // +1 since the current index is an option

						// This special flag would only happen on Windows (/myoption, /y)
						switch (argN.GetLength())
						{
							case 1:
							{
								// should never happen since 'argN' should be considered
								// as a parameter, and not an option
								xbox_assert(false && "argN should be considered as a parameter");
								return RaiseInvalidValue(argN);
							}
							case 2:
							{
								// it seems to be a short option name
								// The method `AnalyzeShortOptionName` will ignore the first char
								Error err = AnalyzeShortOptionName(argN);
								if (err != Private::CommandLineParser::eOK)
									return err;
								break;
							}
							default:
							{
								// get rid of the two first char (/y /mylongoption)
								argN.GetSubString(1 + 1, argN.GetLength() - 1, tmp);
								Error err = AnalyzeLongOptionName(tmp, style);
								if (err != Private::CommandLineParser::eOK)
									return err;
							}
						} // switch argN
					} // windows style enabled

					break;
				}

				case ttIgnore:
				{
					// ignore the argument, as requested
					break;
				}

			} // switch argument type

			// checking for argument style
			if (argumentType != ttParameter && style != previousStyle)
			{
				if (previousStyle != asUnknown)
					return RaiseMixArgumentStyle(argN);
				else
					previousStyle = style;
			}

		} // loop on all arguments

		return Private::CommandLineParser::eOK;
	}



	static inline Error CommandLineParserParse(Private::CommandLineParser::StOptionsInfos& inInfos)
	{
		ParserContext parser(inInfos);

		Error err = parser();

		switch (err)
		{
			case Private::CommandLineParser::eOK:
			{
				break;
			}
			case Private::CommandLineParser::eShouldExit:
			{
				// no parser error, but should quit gracefully and print the help usage first
				xbox_assert(!inInfos.args.empty());
				if (!inInfos.args.empty()) // for code safety if not properly initialized
					inInfos.HelpUsage(inInfos.args[0]);
			}
			case Private::CommandLineParser::eFailed:
			{
				// xbox_assert(false && "failed to parse the command line");
				break;
			}
		}
		return err;
	}


} // anonymous namespace







VCommandLineParser::Error  VCommandLineParser::PrintArgumentError(const VString& inText)
{
	xbox_assert(!inText.IsEmpty());

	#if VERSIONWIN
	std::wcerr.write(inText.GetCPointer(), inText.GetLength());
	std::wcerr << std::endl;
	#else
	StStringConverter<char> buff(inText, VTC_UTF_8);
	if (buff.IsOK())
		std::cerr.write(buff.GetCPointer(), buff.GetSize());
	std::cerr << std::endl;
	#endif
	return eFailed;
}


void VCommandLineParser::Print(const VString& inText)
{
	#if VERSIONWIN
	std::wcout.write(inText.GetCPointer(), inText.GetLength());
	std::wcout << std::endl;
	#else
	StStringConverter<char> buff(inText, VTC_UTF_8);
	if (buff.IsOK())
		std::cout.write(buff.GetCPointer(), buff.GetSize());
	std::cout << std::endl;
	#endif
}


VCommandLineParser::VCommandLineParser(const std::vector<VString>& inArgs)
	: fOptions(inArgs)
{
	// nothing to do
}


VCommandLineParser::~VCommandLineParser()
{
	// nothing to do
}


void VCommandLineParser::Clear()
{
	fOptions.Clear();
}


VCommandLineParser::Error  VCommandLineParser::Parse()
{
	// remainder: the first argument is always the binary itself
	// \internal > 1 to take care of non-initialized arguments vector
	if (fOptions.args.size() > 1)
	{
		switch (CommandLineParserParse(fOptions))
		{
			case Private::CommandLineParser::eOK:
			{
				return eOK;
			}
			case Private::CommandLineParser::eShouldExit:
			{
				// should quit, but no error (help usage probably)
				return eShouldExit;
			}
			default:
			{
				// error for all other cases
				break;
			}
		}

		// fallback - error if something is not handled properly
		return eFailed;
	}

	// nothing to parse, no error
	return eOK;
}






void VCommandLineParser::ExportAsJSON(VString* out)
{
	xbox_assert(out != NULL && "invalud input string");

	VString& s = *out;
	s.Clear();
	s += CVSTR("{\n");

	VString tmp;

	bool allArguments = true;
	bool allOptions   = true;

	if (allArguments)
	{
		tmp.Printf("\t\"args\": {\n\t\t\"count\": %u,\n\t\t\"values\": [");
		s += CVSTR("\t\"arguments\": {\n");
		s += CVSTR("\t\t\"count\": "); // the property 'count' is actually only present for humans :)
		tmp.Clear();
		tmp.Printf("%u", (unsigned int) fOptions.args.size());
		s += tmp;
		s += CVSTR(",\n");
		s += CVSTR("\t\t\"values\": [\n");

		uLONG count = (uLONG) fOptions.args.size();
		for (uLONG i = 0; i != count; ++i)
		{
			s += CVSTR("\t\t\t\"");
			AppendEscapedString(s, tmp, fOptions.args[i]);
			s += (i != count - 1) ? CVSTR("\",\n") : CVSTR("\"\n");
		}

		s += CVSTR("\t\t]\n");

		s += (!allOptions) ? CVSTR("\t}\n") : CVSTR("\t},\n");
	}

	if (allOptions)
	{
		s += CVSTR("\t\"options\": {\n");
		s += CVSTR("\t\t\"count\": "); // the property 'count' is actually only present for humans :)
		tmp.Clear();
		tmp.Printf("%u", (unsigned int) fOptions.options.size());
		s += tmp;
		s += CVSTR(",\n");
		s += CVSTR("\t\t\"values\": [\n");

		uLONG count = (uLONG) fOptions.options.size();
		for (uLONG i = 0; i != count; ++i)
		{
			const Private::CommandLineParser::VOptionBase& opt = *fOptions.options[i];

			s += CVSTR("\t\t\t{\n");
			if (! opt.IsParagraph())
			{
				if (opt.GetShortName() != ' ' && opt.GetShortName() != '\0')
				{
					s += CVSTR("\t\t\t\t\"shortName\": \"");
					s += opt.GetShortName();
					s += CVSTR("\",\n");
				}
				if (!opt.GetLongName().IsEmpty())
				{
					s += CVSTR("\t\t\t\t\"longName\": \"");
					s += opt.GetLongName();
					s += CVSTR("\",\n");
				}
				if (opt.IsRequiresParameter())
					s += CVSTR("\t\t\t\t\"requiresValue\": true,\n");

				s += CVSTR("\t\t\t\t\"description\": \"");
				AppendEscapedString(s, tmp, opt.GetDescription());
				s += CVSTR("\"\n");
			}
			else
			{
				s += CVSTR("\t\t\t\t\"paragraph\": \"");
				AppendEscapedString(s, tmp, opt.GetDescription());
				s += CVSTR("\"\n");
			}

			s += (i != count - 1) ? CVSTR("\t\t\t},\n") : CVSTR("\t\t\t}\n");
		}

		s += CVSTR("\t\t]\n");
		s += CVSTR("\t}\n");

	}

	s += CVSTR("}\n");
}


void VCommandLineParser::ExportAsManPage(VString* out)
{
	xbox_assert(out && "invalid parameter");

	// introduction
	*out += CVSTR(".SH OPTIONS\n");

	VString tmp;
	VString desc;
	uLONG count = (uLONG) fOptions.options.size();
	bool hasAtLeastOneOption = false;

	for (uLONG i = 0; i != count; ++i)
	{
		const Private::CommandLineParser::VOptionBase& option = *fOptions.options[i];

		if (!option.IsVisibleFromHelpUsage())
			continue;

		if (! option.IsParagraph())
		{
			hasAtLeastOneOption = true;
			*out += CVSTR(".TP\n");
			*out += CVSTR(".BR ");

			if (option.GetShortName() != ' ' && option.GetShortName() != '\0')
			{
				*out += CVSTR("\\-");
				*out += option.GetShortName();
				if (!option.GetLongName().IsEmpty())
				{
					*out += CVSTR(" \", \" ");
				}
				else
				{
					if (option.IsRequiresParameter())
						*out += CVSTR(" \\fIVALUE\\fR");
				}
			}
			if (!option.GetLongName().IsEmpty())
			{
				*out += CVSTR("\\-\\-");
				*out += option.GetLongName();
				if (option.IsRequiresParameter())
					*out += CVSTR("=\\fIVALUE\\fR");
			}

			// description
			*out += '\n';
			desc = option.GetDescription();
			desc.ExchangeAll(CVSTR("\n"), CVSTR(""));
			desc.ExchangeAll(CVSTR("\t"), CVSTR(""));
			*out += desc;
			*out += '\n';
		}
		else
		{
			// the first paragraph should be removed since it is an introduction[-like]
			if (hasAtLeastOneOption)
			{
				*out += CVSTR(".SS \"");
				desc = option.GetDescription();
				desc.ExchangeAll(CVSTR("\n"), CVSTR(""));
				desc.ExchangeAll(CVSTR("\t"), CVSTR(""));
				AppendEscapedString(*out, tmp, desc);
				*out += CVSTR("\"\n");
			}
		}
	}
}


void VCommandLineParser::PrintAllArguments(bool inErrOutput)
{
	VString buffer;
	ExportAsJSON(&buffer);

	#if VERSIONWIN
	if (inErrOutput)
		std::wcout.write(buffer.GetCPointer(), buffer.GetLength());
	else
		std::wcerr.write(buffer.GetCPointer(), buffer.GetLength());
	std::wcout << std::endl;

	#else

	StStringConverter<char> buff(buffer, VTC_UTF_8);
	if (buff.IsOK())
	{
		if (!inErrOutput)
			std::cout.write(buff.GetCPointer(), buff.GetSize());
		else
			std::cerr.write(buff.GetCPointer(), buff.GetSize());
		std::cout << std::endl;
	}
	#endif
}




END_TOOLBOX_NAMESPACE


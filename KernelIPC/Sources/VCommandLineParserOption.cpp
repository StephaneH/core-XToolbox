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
#include "VCommandLineParserOption.h"
#include <iostream>





BEGIN_TOOLBOX_NAMESPACE

namespace Private
{
namespace CommandLineParser
{



	#if WITH_ASSERT
	//! Determine whether a string is a valid long option name or not
	static inline bool DebugValidateOptionLongName(const VString& inName)
	{
		if (inName.GetLength() > 0)
		{
			// a long option name must be greater than 1, otherwise it will be
			// a short one, and won't be found at runtime
			if (inName.GetLength() == 1)
				return false;

			// checking validity for each char
			for (VIndex i = 0; i < inName.GetLength(); ++i)
			{
				UniChar c = inName[i];
				if (! (c == '-' || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')))
					return false;
			}

			// checking invalid start / end
			if (inName[0] == L'-' || inName[inName.GetLength() - 1] == L'-')
				return false;
		}
		return true;
	}
	#endif


	#if WITH_ASSERT
	//! Determine whether a single char is a valid short option name or not
	static inline bool DebugValidateOptionShortName(UniChar c)
	{
		// for simplicity sake, a short option name should be restricted
		// to mere ascii characters
		return (c == '\0' || c == ' ') // no short option
			|| (c >= 'a' && c <= 'z')
			|| (c >= 'A' && c <= 'Z')
			|| (c >= '0' && c <= '9');
	}
	#endif



	bool VOptionBase::AppendFullName(VString& out) const
	{
		bool hasLongOption = (fLongName.GetLength() > 0);
		bool success = false;

		if (fShortName != UniChar(' ') && fShortName != UniChar('\0'))
		{
			#if VERSIONWIN
			out += UniChar('/');
			#else
			out += UniChar('-');
			#endif
			out += fShortName;
			if (hasLongOption)
				out += CVSTR(", ");
			success = true;
		}
		if (hasLongOption)
		{
			#if VERSIONWIN
			out += UniChar('/');
			#else
			out += CVSTR("--");
			#endif
			out += fLongName;
			success = true;
		}
		return success;
	}


	void VOptionBase::PrintErrorWhenAppendingInvalidValue(const VString& inExpected, VString inValue)
	{
		VString msg;
		msg += CVSTR("invalid value for argument '");

		// full name of the option (e.g. -e, --long-name)
		if (! AppendFullName(msg))
			msg += CVSTR("<anonymous>");

		// got "<value>..."
		msg += CVSTR("' got ");

		if (inValue.GetLength() > 0)
		{
			enum { maxLength = 80 }; // arbitrary value
			msg += UniChar('"');
			if ((uLONG) inValue.GetLength() > (uLONG) maxLength)
			{
				inValue.SubString(1, maxLength);
				msg += inValue;
				msg += CVSTR("...\"");
			}
			else
			{
				msg += inValue;
				msg += UniChar('"');
			}
			msg += UniChar(' ');
		}
		else
			msg += CVSTR("empty value ");

		// Expected value
		msg += inExpected;

		XBOX::VCommandLineParser::PrintArgumentError(msg);
	}






	StOptionsInfos::StOptionsInfos(const std::vector<VString>& inArgs)
		: remainingOption(NULL)
		, args(inArgs)
		, mayRequireOptions((inArgs.size() > 1u))
	{
		// the optimization related to `mayRequireDocumentation` is currently disabled
		// this option will allow (or not) the copy of the description and paragraph, which are
		// often often.
		// However, the heuristic is not bullet proof (some option may requires at some point
		// the provided documentation) and thus disabled.
		//
		mayRequireDocumentation = true;
		//
		if (mayRequireOptions && false)
		{
			// detecting if we should keep extra informations (mayRequireDocumentation)
			for (uLONG i = 1; i < (uLONG) args.size(); ++i)
			{
				const VString& arg = args[i];
				uLONG length = (uLONG) arg.GetLength();

				if (length > 1)
				{
					if (arg[0] == '-')
					{
						if (arg[1] != '-')
						{
							for (uLONG m = 1; m < length; ++m)
							{
								if (arg[m] == 'h')
								{
									mayRequireDocumentation = true;
									break;
								}
							}
							if (mayRequireDocumentation)
								break;
						}
						else
						{
							if (arg.EqualToUSASCIICString("--help"))
							{
								mayRequireDocumentation = true;
								break;
							}
						}
					}

					if (XBOX::Private::CommandLineParser::eWindowsStyleEnabled)
					{
						// not really restrictive and can be greatly improved
						if (arg[0] == UniChar('/'))
						{
							if (arg[1] == UniChar('h') || arg[1] == UniChar('?') || arg.EqualToUSASCIICString("/help"))
							{
								mayRequireDocumentation = true;
								break;
							}
						}
					}
				}
			}
		}
	}


	StOptionsInfos::~StOptionsInfos()
	{
		// delete all options
		for (uLONG i = 0; i != (uLONG) options.size(); ++i)
			delete options[i];

		delete remainingOption;
	}




	void StOptionsInfos::Clear()
	{
		delete remainingOption; // freeAndNil()
		remainingOption = NULL;

		// using clear-and-minimize idiom to really release all the allocated memory
		using namespace Private::CommandLineParser;
		VOptionBase::MapByShortNames().swap(byShortNames);
		VOptionBase::MapByLongNames().swap(byLongNames);
		VOptionBase::Vector().swap(options);
	}



	void StOptionsInfos::RegisterOption(bool requiresValue, Private::CommandLineParser::VOptionBase* inOption)
	{
		// asserts misc
		xbox_assert(inOption
			&& "invalid pointer to VOptionBase");
		xbox_assert(mayRequireOptions
			&& "This routine should not be called if no option are required");

		// alias to the option itself
		const Private::CommandLineParser::VOptionBase& option = *inOption;

		// alias to the short option name
		UniChar c = option.GetShortName();
		// alias to the long option name
		const VString& longname = option.GetLongName();

		// asserts on long option names
		xbox_assert(longname.GetLength() > 1
			&& "the long option name should not looks like a short one (won't be found at runtime)");
		xbox_assert(!longname.EqualToUSASCIICString("help")
			&& "the long option name 'help' is reserved");
		xbox_assert(DebugValidateOptionLongName(longname)
			&& "invalid long name for command line option");

		// asserts on short option names
		xbox_assert(c != UniChar('?')
			&& "the short option name '?' is reserved");
		xbox_assert(c != UniChar('h')
			&& "the short option name 'h' is reserved");
		xbox_assert(DebugValidateOptionShortName(c)
			&& "invalid short name for command line option");

		// asserts on description
		xbox_assert(((c != UniChar(' ') && c != '\0') || (longname.GetLength() > 0))
			&& option.GetDescription().GetLength() > 0
			&& "no description provided for command line option");


		// Short name registration
		if (c != UniChar(' ') && c != UniChar('\0'))
			byShortNames[c] = std::make_pair(inOption, requiresValue);

		// Long name registration
		if (longname.GetLength() > 0)
			byLongNames[longname] = std::make_pair(inOption, requiresValue);

		// appending the option after all others
		options.push_back(inOption);
	}









} // namespace CommandLineParser
} // namespace Private
END_TOOLBOX_NAMESPACE


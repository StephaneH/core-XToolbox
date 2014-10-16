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


#define HELPUSAGE_30CHAR    CVSTR("                             ") // end





BEGIN_TOOLBOX_NAMESPACE

namespace Private
{
namespace CommandLineParser
{

	enum
	{
		//! The maximum number of char per line for the help usage
		helpMaxWidth = 80,
	};





	static uLONG StringFindFirstOf(const VString& in, const VString& inCharsToFind, uLONG inOffset)
	{
		uLONG size = (uLONG) in.GetLength();
		for (uLONG i = inOffset; i < size; ++i)
		{
			if (0 != inCharsToFind.FindUniChar(in[i]))
				return i;
		}
		return (uLONG) -1;
	}


	template<bool Decal, uLONG LengthLimit>
	static void ExportLongDescription(const VString& inDesc, VString* out)
	{
		uLONG size = inDesc.GetLength();

		if (size > 0)
		{
			const UniChar* cstr = inDesc.GetCPointer();
			// reserve enough room
			out->EnsureSize(out->GetLength() + size + 4); // arbitrary

			uLONG start  = 0;
			uLONG end    = 0;
			uLONG offset = 0;

			do
			{
				// Jump to the next separator
				offset = StringFindFirstOf(inDesc, CVSTR(" .\r\n\t"), offset);

				// No separator, aborting
				if (size < offset)
					break;

				if (offset - start < LengthLimit)
				{
					if ('\n' == inDesc[offset])
					{
						out->AppendUniChars(cstr + start, (offset - start));
						*out += '\n';

						if (Decal)
							*out += HELPUSAGE_30CHAR;

						start = offset + 1;
						end = offset + 1;
					}
					else
						end = offset;
				}
				else
				{
					if (0 == end)
						end = offset;

					out->AppendUniChars(cstr + start, (end - start));
					*out += '\n';

					if (Decal)
						*out += HELPUSAGE_30CHAR;

					start = end + 1;
					end = offset + 1;
				}

				++offset;
			}
			while (true);

			// Display the remaining piece of string
			if (start < size)
				*out += (cstr + start);
		}
	}


	/*!
	** \brief Generate the help usage for a single option
	*/
	static inline
	void ExportOptionHelpUsage(UniChar inShort, const VString& inLong, const VString& inDesc, bool requireParam, VString* out)
	{
		if ('\0' != inShort && ' ' != inShort)
		{
			#if VERSIONWIN
			*out += CVSTR("  /");
			#else
			*out += CVSTR("  -");
			#endif
			*out += inShort;

			if (0 == inLong.GetLength())
			{
				if (requireParam)
					*out += CVSTR(" VALUE");
			}
			else
				*out += CVSTR(", ");
		}
		else
			*out += CVSTR("      ");

		// Long name
		if (0 == inLong.GetLength())
		{
			if (requireParam)
				*out += CVSTR("              ");
			else
				*out += CVSTR("                    ");
		}
		else
		{
			#if VERSIONWIN
			*out += CVSTR("/");
			#else
			*out += CVSTR("--");
			#endif
			*out += inLong;

			if (requireParam)
			{
				#if VERSIONWIN
				*out += CVSTR(":VALUE");
				#else
				*out += CVSTR("=VALUE");
				#endif
			}

			#if VERSIONWIN
			sLONG optionPrefixLength = 1; /* / */
			#else
			sLONG optionPrefixLength = 2; /* -- */
			#endif

			if (30 <= inLong.GetLength() + 6 /*-o,*/ + optionPrefixLength + 1 /*space*/ + (requireParam ? 6 : 0))
			{
				*out += CVSTR("\n                             "); // end
			}
			else
			{
				for (uLONG i = 6 + 2 + 1 + (uLONG) inLong.GetLength() + (requireParam ? 6 : 0); i < 30; ++i)
					*out += ' ';
			}

			#if VERSIONWIN
			// One char is missing on Windows, '/' insteand if "--"
			*out += ' ';
			#endif
		}

		// Description
		if ((uLONG) inDesc.GetLength() <= 50 /* 80 - 30 */)
			*out += inDesc;
		else
			ExportLongDescription<true, 50>(inDesc, out);

		*out += '\n';
	}


	static void ExportParagraph(const VString& inDesc, VString* out)
	{
		uLONG size = (uLONG) inDesc.GetLength();
		if (size > 0)
		{
			if (size < helpMaxWidth)
				*out += inDesc;
			else
				ExportLongDescription<false, 80>(inDesc, out);

			// appending a final linefeed
			*out += '\n';
		}
	}


	void StOptionsInfos::HelpUsage(const VString& inArgv0) const
	{
		VString help;

		// first line
		help += CVSTR("Usage: ");
		help += inArgv0; // FIXME : extractFilePath(inArgv0)
		help += CVSTR(" [OPTION]...");
		if (remainingOption)
			help += CVSTR(" [FILE]...");
		help += '\n';

		// iterating through all options
		if (! options.empty())
		{
			Private::CommandLineParser::VOptionBase::Vector::const_iterator end = options.end();
			Private::CommandLineParser::VOptionBase::Vector::const_iterator i   = options.begin();

			// Add a space if the first option is not a paragraph
			// (otherwise the user would do what he wants)
			if (! (*i)->IsParagraph())
			{
				// deal with hidden options at the beginning
				if ((*i)->IsVisibleFromHelpUsage())
				{
					help += '\n';
				}
				else
				{
					// the first element is not visible, thus we can not determine what to do
					// trying to find the first visible element
					for (; i != end; ++i)
					{
						if ((*i)->IsVisibleFromHelpUsage())
						{
							if (! (*i)->IsParagraph())
								help += '\n';
							break;
						}
					}

					// reset the iterator
					i = options.begin();
				}
			}

			// temporary string for description manipulation
			VString tmpDesc;

			for (; i != end; ++i)
			{
				const Private::CommandLineParser::VOptionBase& opt = *(*i);

				if (opt.IsVisibleFromHelpUsage())
				{
					// we should remove all \t produced by end-of-line backslashes
					// (used for writing long text on several lines)
					// anyway the tab-character should not be used within descriptions
					tmpDesc = opt.GetDescription();
					tmpDesc.ExchangeAll(CVSTR("\t"), VString());

					if (! opt.IsParagraph())
					{
						ExportOptionHelpUsage(opt.GetShortName(), opt.GetLongName(),
							tmpDesc, opt.IsRequiresParameter(), &help);
					}
					else
						ExportParagraph(tmpDesc, &help);
				}
			}
		}

		// determining if the --help option is implicit or not
		if (0 == byLongNames.count(CVSTR("help")))
		{
			// --help is implicit
			VString desc = CVSTR("Display the help and exit");

			#if VERSIONWIN
			{
				if (byShortNames.count('h') == 0)
					ExportOptionHelpUsage('?', CVSTR("h, /help"), desc, false, &help);
				else
					ExportOptionHelpUsage('?', CVSTR("help"), desc, false, &help);
			}
			#else
			{
				if (byShortNames.count('h') == 0)
					ExportOptionHelpUsage('h', CVSTR("help"), desc, false, &help);
				else
					ExportOptionHelpUsage(' ', CVSTR("help"), desc, false, &help);
			}
			#endif
		}

		// display the help usage to the console
		#if VERSIONWIN
		std::wcout.write(help.GetCPointer(), help.GetLength());
		#else
		StStringConverter<char> buff(help, VTC_UTF_8);
		if (buff.IsOK())
			std::cout.write(buff.GetCPointer(), buff.GetSize());
		#endif
	}








} // namespace CommandLineParser
} // namespace Private
END_TOOLBOX_NAMESPACE


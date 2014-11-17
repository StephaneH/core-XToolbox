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
#ifndef __VCommandLineParserOption__
#define __VCommandLineParserOption__

#include "Kernel/VKernel.h"
#include "Kernel/Sources/VKernelTypes.h"
#include "Kernel/Sources/VString.h"
#include <vector>
#include "VCommandLineParserExtension.h"



BEGIN_TOOLBOX_NAMESPACE

namespace Private
{
namespace CommandLineParser
{


enum // anonymous
{
	/*!
	** \brief Flag to determine whether CommandLineParser should handle windows style of cmd arguments
	**
	** Example of Windows style arguments: /myoption [long option], /y [short option]
	*/
	#if VERSIONWIN
	eWindowsStyleEnabled =  1,
	#else
	eWindowsStyleEnabled =  0,
	#endif
};



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





/*!
** \brief Abstract representation of a single Command line Option
*/
class XTOOLBOX_API VOptionBase /*: public VObject*/
{
public:
	//! Array of options
	typedef std::vector<VOptionBase*> Vector;
	//! Option + flag to determine whether an additional value is required or not
	typedef std::pair<VOptionBase*, bool> OptionAndValueRequired;

	// \warning std::hash_map is depprecated and should be replaced as soon
	// as possible by std::unordered_map
	//! List of options, ordered by their short names
	typedef STL_EXT_NAMESPACE::hash_map<UniChar, OptionAndValueRequired>  MapByShortNames;
	//! List of options, ordered by their long names
	// \note using here the COW feature of the VString
	typedef STL_EXT_NAMESPACE::hash_map<VString, OptionAndValueRequired>  MapByLongNames;


public:
	//! \name Constructor & Destructor
	//@{
	//! Default empty constructor
	VOptionBase();
	//! Constructor, with only a description
	explicit VOptionBase(const VString& inDesc);
	/*!
	** \brief Default constructor
	**
	** \param inShortName Short name used for calling the option
	** \param inLongName  Long name used for calling the option
	** \param inDesc  Description of the option [mandatory]
	** \param inRequireParam Flag to determine whether this option requires an additional value or not
	*/
	VOptionBase(char inShortName, const char* inLongName, const VString& inDesc, bool inRequireParam);
	//! Empty destructor
	virtual ~VOptionBase() {}
	//@}

	//! AppendArgument a new value
	virtual Error AppendArgument(const VString& value);
	//! Enable the flag
	virtual void SetFlag() {}

	//! Get the short name of the option
	UniChar GetShortName() const;
	//! Get the long name of the option
	const VString& GetLongName() const;
	//! Get the description of the option
	const VString& GetDescription() const;
	//! Get if the option requires an additional parameter
	bool IsRequiresParameter() const;

	//! Get if the option if a paragraphe and not a real option
	bool IsParagraph() const;

	//! Append the full name of the option (e.g. -e, --long-name)
	// \return True if something has been added to \p out
	bool AppendFullName(VString& out) const;

	//! Get if the option is visible from the help usage
	bool IsVisibleFromHelpUsage() const;
	//! hide this option from the help usage
	void Hide();

protected:
	//! Print an error when appending an invalide value to an option
	// \see AppendArgument()
	void PrintErrorWhenAppendingInvalidValue(const VString& inExpected, VString inValue);

private:
	//! The short name
	UniChar fShortName;
	//! The long name
	VString fLongName;
	//! Description associated to the command line option
	VString fDescription;
	//! Flag to determine whether this option requires an additional value or not
	bool fRequiresParameter;
	//! Flag to determine whether this option is hidden from the help usage or not
	bool fHidden;

}; // class VOptionBase




/*!
** \brief Generic reprensentation of a single option with value and its associated variable
*/
template<class T>
class VArgOption /*final*/ : public VOptionBase
{
public:
	/*!
	** \brief Constructor with only a reference to the target variable [remaining arguments]
	*/
	explicit VArgOption(T& out);

	/*!
	** \brief Constructor
	**
	** \param inShortName Short name used for calling the option
	** \param inLongName  Long name used for calling the option
	** \param inDesc  Description of the option [mandatory]
	** \param inRequireParam Flag to determine whether this option requires an additional value or not
	*/
	VArgOption(T& out, char inShortName, const char* inLongName, const VString& inDesc, bool inRequireParam = false);

	virtual Error AppendArgument(const VString& inValue); // override
	virtual void SetFlag(); // override

public:
	//! Reference to the target
	T& fRef;

}; // class VArgOption











/*!
** \brief Internal storage for all command line options
*/
class XTOOLBOX_API StOptionsInfos // final
{
public:
	//! \name Constructor & Destructor
	//@{
	//! Default constructor
	StOptionsInfos(const std::vector<VString>& inArgs);
		//! Destructor
	~StOptionsInfos();
	//@}

	//! Clear all internal data
	void Clear();

	//! Hide an option from its short name
	void HideOption(UniChar inShortName);
	//! Hide an option from its long name
	void HideOption(const char* inLongName);

	//! Reset the option dedicated to remaining arguments
	void ResetRemainingArguments(Private::CommandLineParser::VOptionBase* inOption);

	//! AppendArgument a new option
	void RegisterOption(bool requiresValue, Private::CommandLineParser::VOptionBase* inOption);

	//! Print the help usage to standard output
	void HelpUsage(const VString& inArgv0) const;


public:
	//! All options, ordered by their long name
	Private::CommandLineParser::VOptionBase::MapByShortNames  byShortNames;
	//! All options, ordered by their long name
	Private::CommandLineParser::VOptionBase::MapByLongNames   byLongNames;
	//! The remaining option (the pointer belongs to us)
	Private::CommandLineParser::VOptionBase* remainingOption;
	//! All options
	Private::CommandLineParser::VOptionBase::Vector options;
	//! All arguments
	const std::vector<VString>& args;
	//! Flag to determine whether options may be required or not
	bool mayRequireOptions;
	//! Flag to determine whether extra informations (such as description or paragraphs)
	// may be required or not
	bool mayRequireDocumentation;

}; // class StOptionsInfos
















inline VOptionBase::VOptionBase()
	: fShortName(' ')
	, fRequiresParameter(false)
	, fHidden(false)
{}


inline VOptionBase::VOptionBase(const VString& inDesc)
	: fShortName(' ')
	, fDescription(inDesc)
	, fRequiresParameter(false)
	, fHidden(false)
{}


inline VOptionBase::VOptionBase(char inShortName, const char* inLongName, const VString& inDesc, bool inRequireParam)
	: fShortName((UniChar) inShortName)
	, fLongName(inLongName)
	, fDescription(inDesc)
	, fRequiresParameter(inRequireParam)
	, fHidden(false)
{}


inline Error VOptionBase::AppendArgument(const VString& /*value*/)
{
	return eFailed;
}


inline bool VOptionBase::IsVisibleFromHelpUsage() const
{
	return ! fHidden;
}


inline void VOptionBase::Hide()
{
	fHidden = true;
}


inline bool VOptionBase::IsRequiresParameter() const
{
	return fRequiresParameter;
}


inline UniChar VOptionBase::GetShortName() const
{
	return fShortName;
}


inline const VString& VOptionBase::GetLongName() const
{
	return fLongName;
}


inline const VString& VOptionBase::GetDescription() const
{
	return fDescription;
}


inline bool VOptionBase::IsParagraph() const
{
	return (fShortName == ' ' && fLongName.GetLength() == 0);
}





template<class T>
inline VArgOption<T>::VArgOption(T& out)
	: VOptionBase()
	, fRef(out)
{}


template<class T>
inline VArgOption<T>::VArgOption(T& out, char inShortName, const char* inLongName, const VString& inDesc, bool inRequireParam)
	: VOptionBase(inShortName, inLongName, inDesc, inRequireParam)
	, fRef(out)
{}


template<class T>
inline Error VArgOption<T>::AppendArgument(const VString& inValue)
{
	// AppendArgument the value to our inner variable according its type
	if (! Extension::CommandLineParser::template Argument<T>::Append(fRef, inValue))
	{
		VString expected;
		Extension::CommandLineParser::template Argument<T>::ExpectedValues(expected);
		PrintErrorWhenAppendingInvalidValue(expected, inValue);
		return eFailed;
	}
	return eOK;
}


template<class T>
inline void VArgOption<T>::SetFlag()
{
	// AppendArgument the value to our inner variable according its type
	Extension::CommandLineParser::template Argument<T>::SetFlag(fRef);
}




inline void StOptionsInfos::ResetRemainingArguments(Private::CommandLineParser::VOptionBase* inOption)
{
	delete remainingOption;
	remainingOption = inOption;
}


inline void StOptionsInfos::HideOption(UniChar inShortName)
{
	Private::CommandLineParser::VOptionBase::MapByShortNames::iterator i = byShortNames.find(inShortName);
	if (i != byShortNames.end())
		i->second.first->Hide();
	else
		xbox_assert(false && "short option name not found");
}


inline void StOptionsInfos::HideOption(const char* inLongName)
{
	Private::CommandLineParser::VOptionBase::MapByLongNames::iterator i = byLongNames.find(inLongName);
	if (i != byLongNames.end())
		i->second.first->Hide();
	else
		xbox_assert(false && "long option name not found");
}





} // namespace CommandLineParser
} // namespace Private
END_TOOLBOX_NAMESPACE

#endif // __VCommandLineParserOption__

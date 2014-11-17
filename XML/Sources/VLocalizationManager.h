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
#ifndef __VLOCALIZATIONMANAGER__
#define __VLOCALIZATIONMANAGER__



BEGIN_TOOLBOX_NAMESPACE

/**
* @brief STR# Code.
* - ID : ID of a group that gathers one or more string IDs
* - String ID : ID of a localized string
*/
struct XTOOLBOX_API STRSharpCodes 
{ 
	STRSharpCodes(sLONG inID, uLONG inStringID) : fID(inID), fStringID(inStringID) {} 
	
	sLONG fID; 
	uLONG fStringID; 
};

END_TOOLBOX_NAMESPACE

/**
* @brief Map STR# Code -> String
*/
class CompareTwoSTRSharpCodes : public std::binary_function< XBOX::STRSharpCodes, XBOX::STRSharpCodes, bool >
{
public:
	bool operator()(const XBOX::STRSharpCodes & strSharpCode1, const XBOX::STRSharpCodes & strSharpCode2) const
	{
		if (strSharpCode1.fID !=  strSharpCode2.fID)
			return (strSharpCode1.fID < strSharpCode2.fID);
		else
			return (strSharpCode1.fStringID < strSharpCode2.fStringID);
	}
};

/**
* @brief VFilePath comparison function
*/
class CompareTwoVFilePaths : public std::binary_function< XBOX::VFilePath, XBOX::VFilePath, bool >
{
public:
	bool operator()(const XBOX::VFilePath & filePath1, const XBOX::VFilePath & filePath2) const
	{
		return filePath1.GetPath() < filePath2.GetPath();
	}
};

class CompareTwoVStrings : public std::binary_function< XBOX::VString, XBOX::VString, bool >
{
public:
	bool operator()(const XBOX::VString & inA, const XBOX::VString & inB) const
	{
		return inA.CompareTo(inB,true)==XBOX::CR_SMALLER;
	}
};

typedef std::map< XBOX::VFilePath, XBOX::VTime, CompareTwoVFilePaths >						FilePathAndTimeMap;
typedef std::map<XBOX::STRSharpCodes, XBOX::VString*, CompareTwoSTRSharpCodes>				STRSharpCodeAndStringMap;
typedef std::map<XBOX::VString, XBOX::VString*, CompareTwoVStrings>                         OOSyntaxStringAndStringMap;
typedef std::map<XBOX::VString, std::map< uLONG, XBOX::VString*>, CompareTwoVStrings >      GroupToIDAndStringsMap;
typedef std::map<XBOX::VString, XBOX::VString*, CompareTwoVStrings >                        DotStringsAndStringsMap;
typedef DeletableHashSet< XBOX::VString *, VString_Hash_Function>							StringsSet;

BEGIN_TOOLBOX_NAMESPACE

class VXMLParser;
class VLocalizationXMLHandler;

#define OO_SYNTAX_INTERNAL_DIVIDER L"#}[{@"

/**
* @brief XML Localization Manager. 
* It manages the localization from the default language : 
* - Analysis of the localization files
* - Lookups 
*/
class XTOOLBOX_API VLocalizationManager : public VObject, public IRefCountable, public ILocalizer
{
	
public:
	VLocalizationManager(DialectCode inDialectCode);
	
	/**
	* @brief Load a file into the manager.
	* This file is parsed and the manager extracts all the localized strings that correspond to the localization language of the manager. Complexity : O(n+(n*log(n))) where n is the number of entries in the file.
	* @param inFileToLoad The XML file to parse (currently supported formats: XLIFF)
	* @param inForceLoading Force the loading of the file, don't do regular language checking
	* @return VE_STREAM_CANNOT_FIND_SOURCE if the file doesn't exist
	* @return VE_STREAM_BAD_NAME if the file doesn't fit the format pattern (ie : XLIFF -> *.xlf)
	* @return VE_OK if the file was parsed
	*/
	VError									LoadFile(VFile* inFileToAdd, bool inForceLoading = false);
	
	/**
	* @brief Load the content of a folder into the manager (non-recursive).
	* Look for every compatible file in the folder to load and parse it.
	* @param inFolderToScan The folder with XML files to parse (currently supported formats: XLIFF)
	* @return VE_STREAM_CANNOT_FIND_SOURCE if the folder doesn't exist
	* @return VE_STREAM_EOF if the folder doesn't contain at least one compatible file
	* @return VE_OK if at least one file was parsed
	*/
	VError									ScanAndLoadFolder(VFolder* inFolderToScan, bool inForceLoading = false);

	/**
	*@brief Scan all the default folders containing localization files linked to the default V4DLocalizationManager language.
	*As an example, if the given language is "en-uk", this function will look in every folder ("en-uk", "en", "English") that could contains localization files and load all the localization files.
	*- If the language has a locale ID (i.e. "fr-be"), one will look in all the following folders if they exist, by priority : "fr-be.lproj", "fr.lproj", "French.lproj"
	*- If the language has just a language ID (i.e. "pt"), one will look in the following folders if they exist, by priority : "pt.lproj", "Portuguese.lproj"
	*@param inFolderThatContainsLocalizationFolders Folder that contains all the localization folders (typically .lproj folders)
	*/
	bool									LoadDefaultLocalizationFolders( VFolder * inFolderThatContainsLocalizationFolders);
	
	/**
	 *@brief Scan all the default localization folders of a component or a plugin.
	 *@see ScanAndLoadFolder
	 *@param inComponentOrPluginFolder Folder of the component or the plugin. 
	 */
	bool									LoadDefaultLocalizationFoldersForComponentOrPlugin( VFolder * inComponentOrPluginFolder);
	
	/**
	* @brief Returns the localized string corresponding to a STR# code.
	* If the STR# code (ID + String ID) has been found in a parsed file, the corresponding localized string is returned. Complexity : O(log(n)).
	* @param inKeyToLookUp The STR# Code to lookup
	* @param outLocalizedString The localized string if the manager found it
	* @return the result of the lookup. (false == no translation done)
	* Derived from ILocalizer.
	*/
	virtual	bool							LocalizeStringWithKey(const VString& inKeyToLookUp, VString& outLocalizedString);
	
	/**
	* @brief Returns the localized string corresponding to an object oriented syntax.
	* If the string corresponding to an object oriented syntax has been found in a parsed file, the corresponding localized string is returned. Complexity : O(log(n)).
	* @param inKeyToLookUp The string with an OO syntax to lookup
	* @param outLocalizedString The localized string if the manager found it
	* @return the result of the lookup. (false == no translation done)
	*/
	bool									LocalizeStringWithSTRSharpCodes(const STRSharpCodes& inSTRSharpCodesToLookUp, VString& outLocalizedString);

	/**
	* @brief Returns all the localized strings of specified sharp code ID
	* @param outLocalizedStrings The found strings.
	* @return the result of the lookup. (false == no string found)
	*/
	bool									LocalizeGroupOfStringsWithAStrSharpID( sLONG inID, std::vector<XBOX::VString>& outLocalizedStrings);

	/**
	* @brief Return the strings of a group
	* When a group is defined in a XLIFF file (and doesn't respect the pattern of an object URL), the content of the group is stored and therefore can be retrieve. The order of the elements is preserved.
	* @param groupName The name of the group
	* @param outLocalizedStringsVector the std::vector to fill (non-valid content if the function returns false)
	* @return false if the function didn't achieve to find the group 
	*/
	bool									GetOrderedStringsOfGroup(const VString& groupName, std::vector<VString>& outLocalizedStringsVector);

	/**
	* @brief Returns the localized string corresponding to .strings key.
	* If the string corresponding to .strings key has been found in a parsed .strings file, the corresponding localized string is returned. Complexity : O(log(n)).
	* @param inKeyToLookUp The .strings key
	* @param outLocalizedString The localized string if the manager found it
	* @return the result of the lookup. (false == no translation done)
	* Derived from ILocalizer.
	*/
	virtual	bool							LocalizeStringWithDotStringsKey(const VString& inKeyToLookUp, VString& outLocalizedString);
	
	/**
	* @brief Returns the localized desciption of an error code.
	* The localized text may contain placeholders for parameters provided in a VValueBag.
	* @param inErrorCode The error code for which a description is returned.
	* @param inParameters The text placeholders are replaced by this bag attributes.
	* @param outLocalizedMessage Receives the localized description or an empty string if no description was found.
	* @return false if the function didn't achieve to find a description for provided error code.
	* Derived from ILocalizer.
	*/
	virtual	bool							LocalizeErrorMessage( VError inErrorCode, VString& outLocalizedMessage);

	/**
	* @brief Returns the localization language used by the localization manager.
	* You can get the localization language but not set it. If you want to set it, destroy the localization manager and create another one.
	* @return the current used language
	*/
	virtual DialectCode						GetLocalizationLanguage();
	
	/**
	* @brief Insert a string relative to a STR# code
	* @param inSTRSharpCodeToAdd The STR# code
	* @param inLocalizedStringToAdd The localized string taht corresponds to the STR# code
	* @param inShouldOverwriteExistentValue Specifies if an existent STR# Code should be overwritten
	* return true if no error during the insert (an already existing STR# Code is not an error)
	*/
	bool InsertSTRSharpCodeAndString(const STRSharpCodes inSTRSharpCodeToAdd, VString& inLocalizedStringToAdd, bool inShouldOverwriteExistentValue);

	/**
	* @brief Insert a string relative to an object URL code ([table]form.object).
	* @param inObjectURL The serialized object URL. The different elements of a object URL must be separated by separators (OO_SYNTAX_INTERNAL_DIVIDER)
	* @param inLocalizedStringToAdd The localized string that corresponds to the object URL
	* @param inShouldOverwriteExistentValue Specifies if an existent Object URL should be overwritten
	* return true if no error during the insert (an already existing Object URL is not an error)
	*/
	bool InsertObjectURLAndString(const VString& inObjectURL, const VString& inLocalizedStringToAdd, bool inShouldOverwriteExistentValue);

	/**
	* @brief Insert a string relative to a group name.
	* The insertion order is conserved.
	* @param inID The ID corresponding to the string we insert. The strings are ordered in function of the ID sort.
	* @param inLocalizedString The localized string that corresponds to the group
	* @param inGroup The group name is which the localized string will be inserted
	* @param inShouldOverwriteExistentValue Not used
	* @return true if no error during the insert (an already existing localized string in the group is not an error)
	*/
	bool InsertIDAndStringInAGroup(uLONG inID, const VString& inLocalizedString, const VString& inGroup, bool inShouldOverwriteExistentValue);
	
	/**
	* @brief Insert a string relative to a .strings keyword.
	* @param inPListKey The .strings key. It can be a constant (like CFBundleGetInfoString) or a string surrounding by quotes
	* @param inLocalizedStringToAdd The localized string that corresponds to the .strings key
	* @param inShouldOverwriteExistentValue Specifies if the value corresponding to an existent .strings key should be overwritten
	* return true if no error during the insert (an already existing .strings key is not an error)
	*/
	bool InsertDotStringsKeyAndString(const VString& inDotStringsKey, const VString& inLocalizedStringToAdd, bool inShouldOverwriteExistentValue);
	
	/**
	* @brief Update the localization manager if needed, parsing all the files if at least one has changed since the last update (or since the creation)
	* This operation can take a long time if there is a lot of files to re-parse. Use it cleverly.
	*/
	bool UpdateIfNeeded();
	
	/**
	* xliff group elements with a non empty restype are collected as VValueBag.
	* RetainGroupBag() returns a particular group value bag.
	*/
	virtual	const VValueBag*	RetainGroupBag( const VString& inGroupResname);

	/**
	* Returns all found <group> elements with specified restype attribute value.
	*/
			void				RetainGroupBagsByRestype( const VString& inResType, std::vector<VRefPtr<const VValueBag> >& outBags);

	/**
	* InsertGroupBag() is called while parsing a group element with a non empty restype attribute.
	*/
			void				InsertGroupBag( const VString& inGroupResname, const VString& inGroupRestype, const VValueBag *inBag);

protected:

	/**
	* @brief Parse a XLIFF file.
	* Proceed to the parsing of a XLIFF file, launching the XML analysis.
	* @see AddFileToLoad()
	* @param inForceLoading Force the loading of the file, don't do regular language checking
	* @return VE_OK when the parsing is done.
	*/
	VError			AnalyzeXLIFFFile(VFile* inFileToAnalyze, bool inForceLoading = false);
	
	/**
	* @brief Parse a .strings file.
	* Proceed to the parsing of a .strings file.
	* @see AddFileToLoad()
	* @return VE_OK when the parsing is done.
	*/
	VError			AnalyzeDotStringsFile(VFile* inFileToAnalyze);
	
	/**
	* @brief try to load the content of a file, if we support its format.
	* Currently we only support the XLIFF format
	* @param inFileToAdd The file to load (no release done in the function)
	* @param actuallyLoadingOfAFolder true if the loading of the file corresponds in fact to the loading of the content of a folder (in which case the updates granularity is the folder and not the file)
	* @param inForceLoading Force the loading of the file, don't do regular language checking
	* @return - VE_OK if the file was loaded
	* - VE_STREAM_CANNOT_FIND_SOURCE if the file doesn't exist
	* - VE_STREAM_BAD_NAME if the file extension is not supported 
	*/
	VError			_LoadFile(VFile* inFileToAdd, bool actuallyLoadingOfAFolder, bool inForceLoading = false);
	
	/**
	* @brief Returns true if the receiver needs to be displayed; returns false otherwise..
	*/
	bool			DoesNeedAnUpdate();
	
	/**
	* @brief Restoring. Empty the localization containers. 
	*/
	bool			ClearLocalizations();
		
private:
	virtual									~VLocalizationManager();

											VLocalizationManager( const VLocalizationManager&);	// no
	VLocalizationManager&					operator=( const VLocalizationManager&);	// no
	
	VCriticalSection						fReadWriteCriticalSection;			/**< Critical section, thread-safe behaviour of the class (i/o protection) */

	StringsSet*								fLocalizedStringsSet;			/**< Effective container of the localized strings */
	DialectCode								fCurrentDialectCode; 			/**< Unique language used when parsing localization files */
	STRSharpCodeAndStringMap				fStringsRelativeToSTRSharpCodes;/**< STR# Code -> localized string */
	OOSyntaxStringAndStringMap				fStringsRelativeToObjects; 		/**< OOSyntax -> localized String */
	GroupToIDAndStringsMap					fStringsAndIDsRelativeToGroups; 		/**< Group name -> numeric ID & localized strings */
	DotStringsAndStringsMap					fStringsRelativeToDotStrings; 		/**< Plist Keyword -> localized strings */
	VXMLParser *							fSAXParser; 					/**< XML SAX Parser */
	VLocalizationXMLHandler *				fSAXHandler; 					/**< XML SAX HAndler */

	std::vector<VFilePath>					fFilesAndFoldersProcessed; 		/**< All the files and folders paths processed to extract localization */
	FilePathAndTimeMap						fFilesProcessedAndLastModificationTime; /**< All the files processed linked to the last modification date recorded */
	
	typedef std::map<VString,VRefPtr<const VValueBag> >	MapOfBagByName;
	MapOfBagByName							fGroupBagsByResname;

	typedef std::multimap<VString,VRefPtr<const VValueBag> >	MapOfBagByRestype;
	MapOfBagByRestype						fGroupBagsByRestype;
};

END_TOOLBOX_NAMESPACE

#endif
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
#ifndef __VUTIManager__
#define __VUTIManager__

BEGIN_TOOLBOX_NAMESPACE

class VXMLParser;
class VUTIManagerXMLHandler;
class VUTI;
class VLocalizationManager;

class XTOOLBOX_API VUTIManager : public VObject, public IRefCountable
{
	
public:
	
	//Singleton methods
	static VUTIManager *GetUTIManager ();
	
	static void Delete();
	
	static bool Init();
	static bool DeInit();
	
	/**
		*/
	bool LoadXMLFile(VFile * inFile);

	/** The two UTIs are equal if
	 *	- the UTI strings are identical
	 *	- a dynamic identifier's tag specification is a subset of the other UTI's tag specification.
	 */
	bool UTTypeEqual(const VString& inTypeIdentifier1, const VString& inTypeIdentifier2);
	
	/** @brief Return all the UTIs that correspond to a given identifier
	* @see Apple documentation of function UTTypeCreateAllIdentifiersForTag.
	* @param inTagClass kUTTagClassFilenameExtension4D, kUTTagClassMIMEType4D, or kUTTagClassOSType4D
	* @param inTag Value of the tag
	* @param inConformingToTypeIdentifier Not used for the moment
	* @result NULL if an error occured. Empty vector if no corresponding UTI was found. You are responsible for releasing all the VUTI contained in the vector and to destroy the vector! 
	*/
	std::vector< VUTI * > * UTTypeCreateAllIdentifiersForTag(const VString& inTagClass, const VString& inTag, const VString& inConformingToTypeIdentifier);
	
	/** @brief Creates a uniform type identifier for the type indicated by the specified tag
	* @see Apple documentation of function UTTypeCreatePreferredIdentifierForTag.
	* @param inTagClass kUTTagClassFilenameExtension4D, kUTTagClassMIMEType4D, or kUTTagClassOSType4D
	* @param inTag Value of the tag
	* @param inConformingToTypeIdentifier Identifier the UTI must conform to
	* @result NULL if an error occured of if no UTI was found. You are responsible for releasing the VUTI object.
	*/
	VUTI * UTTypeCreatePreferredIdentifierForTag(const VString& inTagClass, const VString& inTag, const VString& inConformingToTypeIdentifier);
	
	/** @brief Does a file conforms to a given identifier?
	 *	@param inConformsToTypeIdentifier The UTI the file could conform to.
	 *	@return true if the file conforms to the sent identifier
	**/
	bool UTTypeConformsTo(VFile * inFile, const VString& inConformsToTypeIdentifier);
	
	/** @brief Add a new UTI in the system. 
	* You shouldn't use this function if you don't know exactly what you do. This is typically used by the XML hanlder that process plist files
	* @param inNewUTI The property of the object is given to VUTIManager. You can release the VUTI after that call.
	*/
	bool AddNewUTI(VUTI * inNewUTI);
	
	/** @brief Define the localization manager used to translate descriptions */
	void SetLocalizationManager(VLocalizationManager* inLocalizationManager);
	
	//Just for the tests - don't use it please
	uLONG GetUTIsCount();
	
protected:
	
	VUTIManager();
	virtual ~VUTIManager();
	
	VError AnalyzeXMLFile(const VFile* inFileToAnalyze);
	VError ProcessExtractedUTIs();
	
	std::map< VString, VUTI* >		fBaseUTIsMap; ///< Base UTIs
	static VUTIManager *			fSingleton; ///< Singleton
	
	//XML
	VXMLParser*						fSAXParser; 					/**< XML SAX Parser */
	VUTIManagerXMLHandler*			fSAXHandler; 					/**< XML SAX HAndler */
	
	//Localization 
	VLocalizationManager*			fLocalizationManager;
	
};


END_TOOLBOX_NAMESPACE

#endif

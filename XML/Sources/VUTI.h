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
#ifndef __VUTI__
#define __VUTI__

BEGIN_TOOLBOX_NAMESPACE

/** @brief Describe an UTI, as defined on Mac OS X 
 * You should never use setter functions in this class unless you know exactly what you do. If you need t create an UTI , please ask VUTIManager.
 */
class XTOOLBOX_API VUTI : public VObject, public IRefCountable 
{
	friend class VUTIManagerXMLHandler;
	friend class VUTIManagerArrayHandler;
	friend class VUTIManagerUTIHandler;
	friend class VUTIManagerUTIKeyUTISpecificationHandler;
	friend class VUTIManagerUTIKeyArrayStringHandler;
	friend class VUTIManager;
	
public:

	/** @name Public functions */
	//@{ 
	
	VString GetIdentifier();
	VString GetDescription();
	VString GetReference();
	VFilePath GetImagePath();
	
	/** @brief Does the UTI conforms to a given extension?
	 *	@param inExtension Extension of a file name (like "doc" or "txt", not limited to three letters)
	 *	@return true if the UTI or an ancestor conforms to the extension
	**/
	bool ConformsToExtension(const VString& inExtension);
	
	/** @brief Does the UTI conforms to a given OS Type?
	 *	@param inOSType OS Type (like 'TEXT' or 'MooV')
	 *	@return true if the UTI or an ancestor conforms to the OS Type
	**/
	bool ConformsToOSType(const VString& inOSType);
	
	/** @brief Does the UTI conforms to a given MIME/Type?
	 *	@param inMIMEType MIME/Type of a file (like 'TEXT' or 'MooV')
	 *	@return true if the UTI or an ancestor conforms to the MIME/Type
	**/
	bool ConformsToMIMEType(const VString& inMIMEType);
	
	/** @brief Does the UTI conforms to a given UTI?
	 *	@param inIdentifier An UTI (like "com.4d.database")
	 *	@return true if the UTI or an ancestor conforms to the given UTI
	**/
	bool ConformsToIdentifier(const VString& inIdentifier);
	
	/** @brief Not used for the moment
	 *	@return true if the UTI is dynamic (UTI with a dyn.* form)
	**/
	bool IsDynamic();
	
	/** @brief Is the UTI a public UTI?
	 *	@return true if the UTI is public (UTI with a public.* form)
	**/
	bool IsPublic();
	
	//@}
	
protected:;

	/**
	 * Don't create UTIs from scratch! Please ask VUTIManager ! More generally, you should never use setter functions in this class.
	 */
	VUTI();
	virtual ~VUTI();
	void _Detach();
	/** \name basicAttributesGroup Basic attributes
	}**/
	//@{
	void SetIdentifier(VString inIdentifier);
	void SetDescription(VString inDescription);
	void SetReference(VString inReference); 
	void SetImagePath(VFilePath inImagePath);	
	void SetDynamic(bool inIsDynamic);
	//@}
	
	/** \name extensionsGroup Extensions management
	}**/
	//@{
	std::vector< VString > GetExtensionsVector();
	/** @brief Return all the extensions, even the parent ones**/
	void GetAllExtensionsVector(std::vector< VString >& outAllExtensionsVector);
	void SetExtensionsVector(std::vector< VString > inExtensionsVector);
	void AddExtension(VString inExtension);
	VUTI * GetHighestParentThatConformsToExtension(const VString& inExtension);
	//@}
	
	/** \name OSTypesGroup OSTypes Management
	}**/
	//@{
	std::vector< VString > GetOSTypesVector(); 
	void SetOSTypesVector(std::vector< VString > inOSTypesVector); 
	void AddOSType(VString inOSType);
	VUTI * GetHighestParentThatConformsToOSType(const VString& inOSType);
	//@}
	
	/** \name MIMETypesGroup MIME/Type Management
	}**/
	//@{
	std::vector< VString > GetMIMETypesVector(); 
	void SetMIMETypesVector(std::vector< VString > inMIMETypesVector); 
	void AddMIMEType(VString inMIMEType);
	VUTI * GetHighestParentThatConformsToMIMEType(const VString& inMIMEType);
	//@}
	
	/** \name treeGroup Conformity Tree Management
	}**/
	//@{
	std::map< VString, VUTI * > GetParentUTIsMap(); 
	void SetParentUTIsMap(std::map< VString, VUTI * > inParentUTIsMap); 
	void AddParentUTI(VUTI * inParentUTI);
	bool CouldBeAParent(VUTI * inUTI);
	
	std::map< VString, VUTI * > GetChildrenUTIsMap();
	void SetChildrenUTIsMap(std::map< VString, VUTI * > inChildrenUTIsMap);
	void AddChildUTI(VUTI * inChildUTI);
	bool CouldBeAChild(VUTI * inUTI);
	
	bool IsParent(VUTI * inUTI);	
	bool IsChild(VUTI * inUTI);
	bool IsChildOrChildOfAParent(VUTI * inUTI);
	//@}
	
	/** \name tempGroup Allow to know if a UTI is a temporary one (just extracted from an XML file and not processed)
	}**/
	//@{
	bool IsTemporary();
	void SetTemporary(bool inIsTemporary);
	void RemoveTemporaryUTI(VString inUTIIdentifier);
	bool fIsTemporary;
	//@}
	
	VString fIdentifier; ///< Identifier. Ex: com.4d.database (must follow a reverse-DNS format)
	VString fDescription; ///< Description. Ex: 4D Database
	VString fReference; ///< Reference. Ex: http://www.4d.com
	VFilePath fImagePath; ///< Image path. Icon corresponding to the type of file
	bool fIsDynamic; ///< Not used.
	
	std::vector< VString > fOSTypesVector; ///< OSType(s). Ex: "TEXT"
	std::vector< VString > fExtensionsVector; ///< Extension(s) (case-unsensitive and without the dot). Ex : "4db"
	std::vector< VString > fMIMETypesVector; ///< MIME/type(s)
	std::map< VString, VUTI * > fParentUTIsMap; ///< UTI(s) the UTI conforms to...
	std::map< VString, VUTI * > fChildrenUTIsMap; ///< Children UTI(s) of the UTI (UTIs that conform to the instance)...
};

END_TOOLBOX_NAMESPACE

#endif

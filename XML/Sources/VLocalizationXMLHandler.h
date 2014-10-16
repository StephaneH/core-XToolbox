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
#ifndef __VLOCALIZATIONXMLHANDLER__
#define __VLOCALIZATIONXMLHANDLER__

#include <stack>

#include "XML/Sources/XMLSaxHandler.h"
#include "XML/Sources/VLocalizationManager.h"

#include "IXMLHandler.h"

BEGIN_TOOLBOX_NAMESPACE


class XTOOLBOX_API VLocalizationXMLHandler : public VObject, public IXMLHandler
{
public:
	VLocalizationXMLHandler(VLocalizationManager* localizationManager);
	
	virtual	IXMLHandler*		StartElement( const VString& inElementName);
	virtual void				SetAttribute( const VString& inName, const VString& inValue);
	virtual void				SetText( const VString& inText);
	void						SetAvoidLanguageChecking(bool inAvoidLanguageChecking);

private:
	VLocalizationManager*		fLocalizationManager;
	bool						fXLIFFFileLanguageIsValid;
	bool						fShouldOverwriteAnyExistentLocalizationValue;
	VString						fElementName;
	bool						fAvoidLanguageChecking;
};


class VLocalizationGroupHandler : public VObject, public IXMLHandler
{
public:
	VLocalizationGroupHandler(bool inShouldOverwriteAnyExistentLocalizationValue, VLocalizationManager* inLocalizationManager);
	virtual	~VLocalizationGroupHandler();
	
	virtual	IXMLHandler*		StartElement( const VString& inElementName);
	virtual void				EndElement( const VString& inElementName);
	virtual void				SetAttribute( const VString& inName, const VString& inValue);
	virtual void				SetText( const VString& inText);

private:
	VLocalizationManager*			fLocalizationManager;
	bool							fShouldOverwriteAnyExistentLocalizationValue;
	sLONG							fGroupID;
	std::stack<VString>				fGroupResnamesStack;
	VValueBag*						fGroupBag;	// non null if a menu group is being parsed
	VString							fGroupRestype;
};


class VLocalizationTransUnitHandler : public VObject, public IXMLHandler
{
public:
	// parsing trans-unit at top level
	VLocalizationTransUnitHandler( bool inShouldOverwriteAnyExistentLocalizationValue, VLocalizationManager* localizationManager);
	
	// parsing trans-units in a group
	VLocalizationTransUnitHandler( uLONG groupID, const std::stack<XBOX::VString>& inGroupResnamesStack, VValueBag *ioTransUnitBag, bool inShouldOverwriteAnyExistentLocalizationValue, VLocalizationManager* localizationManager);

	virtual	~VLocalizationTransUnitHandler();
	
	virtual	IXMLHandler*		StartElement( const VString& inElementName);
	virtual void				EndElement( const VString& inElementName);
	virtual void				SetAttribute( const VString& inName, const VString& inValue);
	virtual void				SetText( const VString& inText);

private:
			void				CheckPlatformTag( const VString& inTags, bool inIncludeIfTag, bool *ioExcluded);
			
	VLocalizationManager*		fLocalizationManager;
	VString						fElementName;
	sLONG						fGroupID;
	VValueBag*					fGroupBag;	// non null if a menu group is being parsed
	VValueBag*					fTransUnitBag;	// non null if a menu group is being parsed
	uLONG						fStringID;
	VString						fResName;
	VString						fSource;	// content of <source> element
	VString						fTarget;	// content of <target> element
	bool						fIsEntryRecorded;
	bool						fShouldOverwriteAnyExistentLocalizationValue;
	bool						fExcluded;
	std::stack<VString>			fGroupResnamesStack;
};

class VLocalizationAltTransHandler : public VObject, public IXMLHandler
{
public:
	// parsing trans-unit at top level
	VLocalizationAltTransHandler( bool inShouldOverwriteAnyExistentLocalizationValue, VLocalizationManager* localizationManager);
	
	// parsing trans-units in a group
	VLocalizationAltTransHandler( uLONG groupID, const std::stack<XBOX::VString>& inGroupResnamesStack, VValueBag *ioTransUnitBag, bool inShouldOverwriteAnyExistentLocalizationValue, VLocalizationManager* localizationManager);

	virtual	~VLocalizationAltTransHandler();
	
	virtual	IXMLHandler*		StartElement( const VString& inElementName);
	virtual void				EndElement( const VString& inElementName);
	virtual void				SetAttribute( const VString& inName, const VString& inValue);
	virtual void				SetText( const VString& inText);

private:
			void				CheckPlatformTag( const VString& inTags, bool inIncludeIfTag, bool *ioExcluded);
			
	VLocalizationManager*		fLocalizationManager;
	VString						fElementName;
	sLONG						fGroupID;
	VValueBag*					fTransUnitBag;	// non null if a menu group is being parsed
	VValueBag*					fAltTransBag;	// non null if a menu group is being parsed
	uLONG						fStringID;
	VString						fResName;
	VString						fSource;	// content of <source> element
	VString						fTarget;	// content of <target> element
	bool						fIsEntryRecorded;
	bool						fShouldOverwriteAnyExistentLocalizationValue;
	bool						fExcluded;
	std::stack<VString>			fGroupResnamesStack;
};

END_TOOLBOX_NAMESPACE

#endif

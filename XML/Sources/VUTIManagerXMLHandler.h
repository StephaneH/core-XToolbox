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
#ifndef __VUTIMANAGERXMLHANDLER__
#define __VUTIMANAGERXMLHANDLER__

#include <stack>

#include "XML/Sources/XMLSaxHandler.h"
#include "XML/Sources/VUTIManager.h"
#include "XML/Sources/VUTI.h"

#include "IXMLHandler.h"

BEGIN_TOOLBOX_NAMESPACE


class XTOOLBOX_API VUTIManagerXMLHandler : public VObject, public IXMLHandler
{
public:
	VUTIManagerXMLHandler(VUTIManager* inUTIManager);
	
	virtual	IXMLHandler*		StartElement( const VString& inElementName);
	virtual void				EndElement( const VString& inElementName);
	virtual void				SetAttribute( const VString& inName, const VString& inValue);
	virtual void				SetText( const VString& inText);
	
private:
	VUTIManager*				fUTIManager;
	VString						fElementName;
	VString 					fLastKeyFound;
	bool						fCanAnalyseText;
};


class XTOOLBOX_API VUTIManagerArrayHandler : public VObject, public IXMLHandler
{
public:
	VUTIManagerArrayHandler(VUTIManager* inUTIManager);
	
	virtual	IXMLHandler*		StartElement( const VString& inElementName);
	virtual void				EndElement( const VString& inElementName);
	virtual void				SetText( const VString& inText);
	
private:
	VUTIManager*				fUTIManager;
};


class XTOOLBOX_API VUTIManagerUTIHandler : public VObject, public IXMLHandler
{
public:
	VUTIManagerUTIHandler(VUTIManager* inUTIManager);
	virtual ~VUTIManagerUTIHandler();
	
	virtual	IXMLHandler*		StartElement( const VString& inElementName);
	virtual void				EndElement( const VString& inElementName);
	virtual void				SetAttribute( const VString& inName, const VString& inValue);
	virtual void				SetText( const VString& inText);
	
private:
	VUTIManager*				fUTIManager;
	VString						fElementName;
	VString 					fLastKeyFound;
	VString 					fKeyToProcess;
	VUTI *						fCurrentUTI;
	bool						fCanAnalyseText;
	std::stack<XBOX::VString>	fXMLTagsStack;
};


class XTOOLBOX_API VUTIManagerUTIKeyUTISpecificationHandler : public VObject, public IXMLHandler
{
public:
	VUTIManagerUTIKeyUTISpecificationHandler(VUTIManager* inUTIManager, VUTI* inUTI);
	
	virtual	IXMLHandler*		StartElement( const VString& inElementName);
	virtual void				EndElement( const VString& inElementName);
	virtual void				SetAttribute( const VString& inName, const VString& inValue);
	virtual void				SetText( const VString& inText);
	
private:
	VUTIManager*				fUTIManager;
	VUTI*						fCurrentUTI;
	VString						fElementName;
	VString 					fLastKeyFound;
	VString 					fKeyToProcess;
	bool						fCanAnalyseText;
	
	VString						fLastExtensionFound;
	VString						fLastOSTypeFound;
	VString						fLastMIMETypeFound;
};


class XTOOLBOX_API VUTIManagerUTIKeyArrayStringHandler : public VObject, public IXMLHandler
{
public:
	VUTIManagerUTIKeyArrayStringHandler(VUTIManager* inUTIManager, VUTI* inUTI, VString inKey);
	
	virtual	IXMLHandler*		StartElement( const VString& inElementName);
	virtual void				EndElement( const VString& inElementName);
	virtual void				SetAttribute( const VString& inName, const VString& inValue);
	virtual void				SetText( const VString& inText);
	
private:
	VUTIManager*				fUTIManager;
	VUTI*						fCurrentUTI;
	VString 					fKeyName;
	VString						fElementName;
	bool						fCanAnalyseText;
	
	VString						fConformToUTIFound;
	VString						fLastExtensionFound;
	VString						fLastOSTypeFound;
	VString						fLastMIMETypeFound;
};

END_TOOLBOX_NAMESPACE

#endif
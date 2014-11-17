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
#ifndef __XMLForBag__
#define __XMLForBag__

#include "IXMLHandler.h"
#include "XMLSaxParser.h"
#include "IXMLDomWrapper.h"

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API VXMLBagHandler : public VObject, public IXMLHandler
{
public:
								VXMLBagHandler( VValueBag *inRootBag);
	
	virtual	IXMLHandler*		StartElement( const VString& inElementName);
	virtual	void				EndElement( const XBOX::VString& inElementName);
	virtual	void				SetAttribute( const VString& inName, const VString& inValue);
    virtual	void				SetText( const VString& inText);
	
public:
			VBagArray			fBags;
};


class XTOOLBOX_API VXMLBagHandler_UniqueElement : public VObject, public IXMLHandler
{
public:
								VXMLBagHandler_UniqueElement( VValueBag *inRootBag, const VString& inElementName);
	virtual						~VXMLBagHandler_UniqueElement();
	
	virtual	IXMLHandler*		StartElement( const VString& inElementName);
    virtual	void				SetText( const VString& inText);
	
public:
			VValueBag*			fRootBag;
			VString				fElementName;
			sLONG				fCountElements;
};


//====================================================================================================

/*
	Writes a bag into a file using XML-UTF8 format.
	
	Returns True on success (you should check error stack if false).
	
	If inFile is NULL a dialog is presented to select a destination.
*/

VError XTOOLBOX_API WriteBagToFileInXML( const VValueBag& inBag, const VString& inRootElementKind, VFile *inFile, bool inWithIndentation = false );

VError XTOOLBOX_API WriteBagToStreamInXML( const VValueBag& inBag, VStream *inStream);
VError XTOOLBOX_API ReadBagFromStreamInXML( VValueBag& outBag, VStream *inStream);

VError XTOOLBOX_API LoadBagFromXML(const VFile& inFile, const VString& inRootElementKind, VValueBag& outBag, XMLParsingOptions inOptions = XML_Default);
VError XTOOLBOX_API LoadBagFromXML(const VString& inXML, const VString& inRootElementKind, VValueBag& outBag, XMLParsingOptions inOptions = XML_Default);

VError XTOOLBOX_API LoadBagFromXML( const VFile& inFile, const VString& inRootElementKind, VValueBag& outBag, XMLParsingOptions inOptions,  const VString* inSystemIDBase, VFolder* inDTDsFolder);
VError XTOOLBOX_API LoadBagFromXML( const VString& inXML, const VString& inRootElementKind, VValueBag& outBag, XMLParsingOptions inOptions, const VString* inSystemIDBase, VFolder* inDTDsFolder);

/** read bag from DOM Node 
@remarks
	inNode must reference a DOM Node with type xercesc::DOMNode::ELEMENT_NODE 
	(for instance the DOM root element but can be another element)
	
	bag will be build from this element ie:
	attributes of outBag are translated from inNode attributes
	elements of outBag are translated from inNode child elements
*/
VError XTOOLBOX_API ReadBagFromDomXML(VXMLDOMNodeRef inNode, VValueBag& outBag);

/** write bag to the specified DOM Node
@remarks
	ioNode must reference a DOM Node with type xercesc::DOMNode::ELEMENT_NODE 
	(for instance the DOM root element)
	
	attributes of inBag are translated to ioNode attributes
	elements of inBag are translated to ioNode child elements

	if outNodeCreated is not NULL, *outNodeCreated will contain list of all new nodes created
	(can be used by caller to register nodes for instance with XMLMgr)
*/
VError XTOOLBOX_API WriteBagToDomXML(const VValueBag& inBag, VXMLDOMNodeRef ioNode, std::vector<VXMLDOMNodeRef> *outNodesCreated = NULL);

END_TOOLBOX_NAMESPACE


#endif


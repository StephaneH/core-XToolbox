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
#include "VXMLPrecompiled.h"

#include <../../xerces_for_4d/Xerces4DDom.hpp>
#include <../../xerces_for_4d/Xerces4DSax.hpp>
#include <../../xerces_for_4d/XML4DUtils.hpp>

#include "XMLForBag.h"

#include "XMLSaxParser.h"
 
BEGIN_TOOLBOX_NAMESPACE

XERCES_CPP_NAMESPACE_USE

VXMLBagHandler::VXMLBagHandler( VValueBag *inRootBag)
{
	assert( inRootBag != NULL);
	fBags.AddTail( inRootBag);
}

void VXMLBagHandler::EndElement( const XBOX::VString&)
{
	fBags.DeleteNth( fBags.GetCount());
}


IXMLHandler *VXMLBagHandler::StartElement( const VString& inElementName)
{
	VValueBag *element = new VValueBag;
	if (element != NULL)
	{
		VValueBag *bag = fBags.GetNth( fBags.GetCount());
		if (bag)
			bag->AddElement( inElementName, element);

		fBags.AddTail( element);	// becomes current bag
		element->Release();
	}
	
	Retain();
	return this;
}


void VXMLBagHandler::SetAttribute( const VString& inName, const VString& inValue)
{
	VValueBag *bag = fBags.GetNth( fBags.GetCount());
	if (bag)
		bag->SetString( inName, inValue);
}


void VXMLBagHandler::SetText( const VString& inText)
{
	VValueBag *bag = fBags.GetNth( fBags.GetCount());
	if (bag)
		bag->AppendToCData( inText);
}


//====================================================================================================

VXMLBagHandler_UniqueElement::VXMLBagHandler_UniqueElement( VValueBag *inRootBag, const VString& inElementName)
: fRootBag( inRootBag)
, fElementName( inElementName)
{
	assert( fRootBag != NULL);
	fRootBag->Retain();
	fCountElements = 0;
}

VXMLBagHandler_UniqueElement::~VXMLBagHandler_UniqueElement()
{
	ReleaseRefCountable( &fRootBag);
}


IXMLHandler *VXMLBagHandler_UniqueElement::StartElement( const VString& inElementName)
{
	++fCountElements;
	if ((fElementName.GetLength()==0 || (testAssert(inElementName == fElementName)) ) && (fCountElements == 1))
	{
		if(fElementName.GetLength()==0)
			fElementName = inElementName;
		return new VXMLBagHandler( fRootBag);
	}
	return NULL;
}


void VXMLBagHandler_UniqueElement::SetText( const VString& inText)
{
	// ignore contents
}


//====================================================================================================

VError WriteBagToStreamInXML( const VValueBag& inBag, VStream *inStream)
{
	VString xml( "<?xml version=\"1.0\" encoding=\"UTF-16\"?>");

#if VERSIONDEBUG
	inBag.DumpXML( xml, CVSTR( "bag"), false);
#else
	inBag.DumpXML( xml, CVSTR( "bag"), false);
#endif
	// vers
	inStream->PutLong( 1);
	return xml.WriteToStream( inStream, 0);
}


VError WriteBagToFileInXML( const VValueBag& inBag, const VString& inRootElementKind, VFile *inFile, bool inWithIndentation )
{
	VError err = VE_OK;
	
	if (inFile != NULL)
	{
		VString xml;
		inBag.DumpXML( xml, inRootElementKind, inWithIndentation);
		xml.Insert( CVSTR( "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"), 1);
		VStringConvertBuffer buffer( xml, VTC_UTF_8);

		VFileDesc *fd;
		err = inFile->Open( FA_READ_WRITE, &fd, FO_CreateIfNotFound | FO_Overwrite);
		if (err == VE_OK)
		{
			// write BOM
			unsigned char bom[] = {0xef, 0xbb, 0xbf};
			err = fd->PutDataAtPos( bom, sizeof( bom));
			
			if (err == VE_OK)
				err = fd->PutDataAtPos( buffer.GetCPointer(), buffer.GetSize());

			delete fd;
			if (err != VE_OK)
			{
				// delete wrongly written file
				if (inFile->Exists())
					inFile->Delete();
			}
		}
	}
	
	return err;
}


VError ReadBagFromStreamInXML( VValueBag& outBag, VStream *inStream)
{
	// read vers
	sLONG vers = 0;
	VError err = inStream->GetLong( vers);
	if (testAssert( err == VE_OK && vers == 1))
	{
		VString xml;
		err = xml.ReadFromStream( inStream, 0);
		if (err == VE_OK)
		{
			VXMLParser parser;
			VXMLBagHandler_UniqueElement handler( &outBag, L"bag");
			parser.Parse( xml, &handler, XML_Default);
			err = vGetLastError();
		}
	}
	return err;
}


static VError LoadBagFromXML( const VString *inXML, const VFile *inFile, const VString& inRootElementKind, VValueBag& outBag, XMLParsingOptions inOptions, const VString* inSystemIDBase, VFolder* inDTDsFolder)
{
	// pass one file or one string but not both
	xbox_assert( (inXML == NULL) || (inFile == NULL));
	xbox_assert( (inXML != NULL) || (inFile != NULL));

	// temporarly switch off debugging UI & breaks
	VDebugContext& context = VTask::GetCurrent()->GetDebugContext();
	context.DisableUI();
	context.DisableBreak();

	StErrorContextInstaller errContext;		// sc 11/06/2009
	VXMLParser parser;
	VXMLBagHandler_UniqueElement handler( &outBag, inRootElementKind);

	if ((inSystemIDBase != NULL) && (inDTDsFolder != NULL) && inDTDsFolder->Exists())
	{
		handler.SetEntitiesLocalFolder( *inSystemIDBase, inDTDsFolder);
	}

	bool ok;
	if (inXML != NULL)
		ok = parser.Parse( *inXML, &handler, inOptions);
	else
		ok = parser.Parse( const_cast<VFile*>( inFile), &handler, inOptions);

	VError err = errContext.GetLastError();
	xbox_assert((ok && err == VE_OK) || (!ok && err != VE_OK));
	
	context.EnableUI();
	context.EnableBreak();

	return err;
}



VError LoadBagFromXML(const VString& inXML, const VString& inRootElementKind, VValueBag& outBag, XMLParsingOptions inOptions)
{
	return LoadBagFromXML( &inXML, NULL, inRootElementKind, outBag, inOptions, NULL, NULL);	// sc 24/09/2009 factorisation
}


VError LoadBagFromXML(const VFile& inFile, const VString& inRootElementKind, VValueBag& outBag, XMLParsingOptions inOptions)
{
	return LoadBagFromXML( NULL, &inFile, inRootElementKind, outBag, inOptions, NULL, NULL);	// sc 24/09/2009 factorisation
}


VError LoadBagFromXML( const VFile& inFile, const VString& inRootElementKind, VValueBag& outBag, XMLParsingOptions inOptions, const VString* inSystemIDBase, VFolder* inDTDsFolder)
{
	return LoadBagFromXML( NULL, &inFile, inRootElementKind, outBag, inOptions, inSystemIDBase, inDTDsFolder);
}


VError LoadBagFromXML( const VString& inXML, const VString& inRootElementKind, VValueBag& outBag, XMLParsingOptions inOptions, const VString* inSystemIDBase, VFolder* inDTDsFolder)
{
	return LoadBagFromXML( &inXML, NULL, inRootElementKind, outBag, inOptions, inSystemIDBase, inDTDsFolder);
}


/** read bag from DOM Node 
@remarks
	inNode must reference a DOM Node with type xercesc::DOMNode::ELEMENT_NODE 
	(for instance the DOM root element)
	
	bag will be build from this element ie:
	attributes of outBag are translated from inNode attributes
	elements of outBag are translated from inNode child elements
*/
VError ReadBagFromDomXML(VXMLDOMNodeRef inNode, VValueBag& outBag)
{
	VError err = VE_OK;
 
	VString nameRoot;
	xercesc::DOMNode *node = reinterpret_cast<xercesc::DOMNode *>( inNode);
	if (node->getNodeType() != xercesc::DOMNode::ELEMENT_NODE)
	{
		err = XBOX::VE_XML_ParsingError;
		return err;
	}
	else 
		nameRoot = (const UniChar *)node->getNodeName();

	StErrorContextInstaller errContext;		
	VXMLParser parser;
	VXMLBagHandler_UniqueElement handler( &outBag, nameRoot);
	bool ok = IXMLDomWrapper::Parse( inNode, static_cast<IXMLHandler *>(&handler));
	err = errContext.GetLastError();
	if (err == VE_OK && (!ok))
		err = XBOX::VE_XML_ParsingError;
	return err;
}


/** write bag to the specified DOM Element 
@remarks
	attributes of inBag are translated to ioElement attributes
	elements of inBag are translated to ioElement child elements

	if outNodeCreated is not NULL, *outNodeCreated will contain list of all new nodes created
	(can be used by caller to register nodes for instance with XMLMgr)
*/
bool WriteBagToDomXMLElement(const VValueBag& inBag, xercesc::DOMElement *ioElement, std::vector<VXMLDOMNodeRef> *outNodesCreated)
{
	xercesc::DOMDocument *doc = ioElement->getOwnerDocument();
	if (doc == NULL)
		return false;

	//parse and translate attributes
	VIndex numAttribute;
	VString name;
	VString valueXML;
	if ((numAttribute = inBag.GetAttributesCount()) > 0)
	{
		for (int i = 1; i <= numAttribute; i++)
		{
			const VValueSingle *value = inBag.GetNthAttribute( i, &name);
			if (value)
			{
				xbox_assert(sizeof(UniChar) == sizeof(XMLCh));

				if (value->GetValueKind() == VK_STRING)
					//string value will be converted yet by xerces serializer so do not call GetXMLString here
					//to avoid translation to be applied twice but get directly the string value
					//(otherwise a string like "<test>" is serialized to
					// "&amp;lt;test&amp;gt;" because translation is applied twice (once by 4D and once by xerces)
					//which is obviously wrong (!) because the right xml serialized string is:
					//"&lt;test>" (note that > is perfectly legal: unlike '<', '>' does not need to be converted - and xerces does not)
					IXMLDomWrapper::SetNodeElementAttribute( (VXMLDOMElementRef)ioElement, name, *(static_cast<const VString *>(value)), false);
				else
				{
					value->GetXMLString( valueXML, XSO_Default);
					valueXML.TranscodeFromXML(); //value must be passed unescaped as Xerces escapes internally
					IXMLDomWrapper::SetNodeElementAttribute( (VXMLDOMElementRef)ioElement, name, valueXML, false);
				}
			}
		}
	}

	//parse and translate elements
	VIndex numElementName = inBag.GetElementNamesCount();
	if (numElementName > 0)
	{
		for (int i = 1; i <= numElementName; i++)
		{
			const VBagArray *elemArray = inBag.GetNthElementName( i, &name);
			if (elemArray)
			{
				VIndex numElement = elemArray->GetCount();
				for (int index = 1; index <= numElement; index++)
				{
					const VValueBag *bagElem = elemArray->GetNth( index);
					if (bagElem)
					{
						//create new DOM element
#ifdef XERCES_3_0_1
						//JQ 19/05/2010: workaround for the default namespace URI bug
						DOMElement *child = reinterpret_cast<xercesc::DOMElement *>(IXMLDomWrapper::CreateElement( (VXMLDOMDocumentRef)doc, (VXMLDOMElementRef)ioElement, name.GetCPointer()));
#else
						DOMElement *child = doc->createElement( name.GetCPointer());
#endif
						if (child)
						{
							//register element 
							if (outNodesCreated)
								(*outNodesCreated).push_back( (VXMLDOMNodeRef)(static_cast<DOMNode *>(child)));

							//append element to list of parent element childs
							ioElement->appendChild( static_cast<DOMNode *>(child));

							//parse child element bag
							if (!WriteBagToDomXMLElement( *bagElem, child, outNodesCreated))
								return false;
						}
					}
				}
			}
		}
	}
	return true;
}

/** write bag to the specified DOM Node
@remarks
	ioNode must reference a DOM Node with type xercesc::DOMNode::ELEMENT_NODE 
	(for instance the DOM root element)
	
	attributes of inBag are translated to ioNode attributes
	child elements of inBag are translated to ioNode child elements

	if outNodeCreated is not NULL, *outNodeCreated will contain list of all new nodes created
	(can be used by caller to register nodes for instance with XMLMgr)
*/
VError WriteBagToDomXML(const VValueBag& inBag, VXMLDOMNodeRef ioNode, std::vector<VXMLDOMNodeRef> *outNodesCreated)
{
	VError err = VE_OK;
 
	VString nameRoot;
	xercesc::DOMNode *node = reinterpret_cast<xercesc::DOMNode *>( ioNode);
	if (node->getNodeType() != xercesc::DOMNode::ELEMENT_NODE)
	{
		err = XBOX::VE_INVALID_PARAMETER;
		return err;
	}
	xercesc::DOMElement *root = static_cast<xercesc::DOMElement *>(node);
	xbox_assert(root);

	bool ok = WriteBagToDomXMLElement( inBag, root, outNodesCreated);
	if (!ok)
		err = XBOX::VE_UNKNOWN_ERROR;

	return err;
}


END_TOOLBOX_NAMESPACE

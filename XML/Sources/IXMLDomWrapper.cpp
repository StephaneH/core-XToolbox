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

#include <xercesc/dom/DOMProcessingInstruction.hpp>

/*
#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>

#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>
#include <xercesc/dom/impl/DOMDocumentImpl.hpp>
#include <xercesc/dom/DOMImplementationRegistry.hpp>
#include <xercesc/util/XMLUniDefs.hpp>
*/

#ifdef XERCES_3_0_1
#include <xercesc/dom/DOMLSSerializer.hpp>
#include <xercesc/dom/DOMLSOutput.hpp>
#endif


#include "IXMLDomWrapper.h"


BEGIN_TOOLBOX_NAMESPACE

XERCES_CPP_NAMESPACE_USE


bool DOMParseElement( xercesc::DOMNode *inElement, IXMLHandler *inHandler);

bool DOMParseNode( xercesc::DOMNode *inNode, IXMLHandler *inHandler)
{
	bool ok = true;
	xercesc::DOMNode *childelement = inNode->getFirstChild();
	while( childelement)
	{
		
		switch( childelement->getNodeType())
		{
			case xercesc::DOMNode::ELEMENT_NODE:
				{
					ok = DOMParseElement( childelement, inHandler);
					break;
				}
			
			case xercesc::DOMNode::TEXT_NODE:
				{
					const VString text( const_cast<XMLCh*>(childelement->getNodeValue()), (VSize) -1);
					inHandler->SetText( text);
					break;
				}
			
			case xercesc::DOMNode::CDATA_SECTION_NODE:
				{
					const XMLCh *data = childelement->getNodeValue();
					xbox_assert(sizeof(XMLCh) == sizeof(UniChar)); //just ensure Xerces uses UTF_16
					VString text( (const UniChar*)data);
					inHandler->SetText( text);
					break;
				}
			case xercesc::DOMNode::PROCESSING_INSTRUCTION_NODE:
				{
					xercesc::DOMProcessingInstruction *nodePI = static_cast<xercesc::DOMProcessingInstruction *>(childelement);
					xbox_assert(sizeof(XMLCh) == sizeof(UniChar)); //just ensure Xerces uses UTF_16
					if (nodePI->getTarget() && nodePI->getData())
					{
						VString targetName( const_cast<XMLCh*>(nodePI->getTarget()), (VSize) -1);
						VString dataName( const_cast<XMLCh*>(nodePI->getData()), (VSize) -1);
						inHandler->processingInstruction( targetName, dataName);
					}
					break;
				}

			case xercesc::DOMNode::ENTITY_REFERENCE_NODE:
			case xercesc::DOMNode::ENTITY_NODE:
			case xercesc::DOMNode::ATTRIBUTE_NODE:
			case xercesc::DOMNode::COMMENT_NODE:
			case xercesc::DOMNode::DOCUMENT_NODE:
			case xercesc::DOMNode::DOCUMENT_TYPE_NODE:
			case xercesc::DOMNode::DOCUMENT_FRAGMENT_NODE:
			case xercesc::DOMNode::NOTATION_NODE:
				// tbd
				break;
		}
			
		childelement = childelement->getNextSibling();
	}
	return ok;
}

bool DOMParseElement( xercesc::DOMNode *inElement, IXMLHandler *inHandler)
{
	bool ok = false;
	const VString elementName( const_cast<XMLCh*>(inElement->getNodeName()), (VSize) -1);
	IXMLHandler *handler = inHandler->StartElement( elementName);
	if (handler != NULL)
	{
		xercesc::DOMNamedNodeMap* attributes = inElement->getAttributes(); 

		if (attributes != NULL)
		{
			int len = attributes->getLength();
			for( int index = 0; index < len; index++)
			{
				xercesc::DOMNode *attNode = attributes->item( index);
				if (attNode)
				{
					const VString attributeName( const_cast<XMLCh*>(attNode->getNodeName()), (VSize) -1);
					const VString attributeValue( const_cast<XMLCh*>(attNode->getNodeValue()), (VSize) -1);
					handler->SetAttribute( attributeName, attributeValue);
				}
			}
		}
		
		ok = DOMParseNode( inElement, handler);

		handler->EndElement( elementName);
		handler->Release();
	}
	return ok;
}


//parse DOM tree
bool IXMLDomWrapper::Parse( VXMLDOMNodeRef inNode, IXMLHandler *inHandler)
{
	xercesc::DOMNode *node = reinterpret_cast<xercesc::DOMNode *>( inNode);

	if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE)
		return DOMParseElement( node, inHandler);
	else
		return DOMParseNode( node, inHandler);
}


// get the node owner document
VXMLDOMNodeRef  IXMLDomWrapper::GetNodeDocument( VXMLDOMNodeRef inNode)
{
	xercesc::DOMDocument *nodeDocument = NULL;
	try
	{
		xercesc::DOMNode *node = reinterpret_cast<xercesc::DOMNode *>( inNode);
		nodeDocument = (node->getNodeType() == xercesc::DOMNode::DOCUMENT_NODE)?
					   static_cast<xercesc::DOMDocument*>(node) : node->getOwnerDocument();
	}
	catch(...)
	{
	}
	return reinterpret_cast<VXMLDOMNodeRef>(static_cast<xercesc::DOMNode *>(nodeDocument));
}


// get the document URL
bool IXMLDomWrapper::GetDocumentURL(  VXMLDOMNodeRef inNode, VURL& outDocURL)
{
	try
	{
		xercesc::DOMDocument *nodeDocument = static_cast<xercesc::DOMDocument *>(reinterpret_cast<xercesc::DOMNode *>(GetNodeDocument( inNode)));
		if (nodeDocument)
		{
			const XMLCh* sDocURI = nodeDocument->getDocumentURI();
			if (!sDocURI)
				return false;
			XBOX::VString sDocURL( sDocURI);
			outDocURL.FromString( sDocURL, true);
			if (!outDocURL.IsConformRFC())
				return false;
			return true;
		}
	}
	catch(...)
	{
	}
	return false;
}


// clone the specified DOM tree
VXMLDOMNodeRef  IXMLDomWrapper::CloneNodeDocument( VXMLDOMNodeRef inNode)
{
	xercesc::DOMDocument *docClone = NULL;
	try
	{
		xercesc::DOMDocument *doc = static_cast<xercesc::DOMDocument *>( reinterpret_cast<xercesc::DOMNode *>(inNode));
		docClone = (xercesc::DOMDocument *)(doc->cloneNode(true)); //do not use reinterpret_cast here:
																   //cloneNode return value is DOMDocument pointer yet
		docClone->setDocumentURI( doc->getDocumentURI());
	}
	catch(...)
	{
	}
	xercesc::DOMNode *node = static_cast<xercesc::DOMNode *>(docClone);
	return reinterpret_cast<VXMLDOMNodeRef>(node);
}


// clone the specified DOM tree
VXMLDOMDocumentRef  IXMLDomWrapper::CloneNodeDocument( VXMLDOMDocumentRef inNode)
{
	xercesc::DOMDocument *docClone = NULL;
	try
	{
		xercesc::DOMDocument *doc = reinterpret_cast<xercesc::DOMDocument *>(inNode);
		docClone = (xercesc::DOMDocument *)(doc->cloneNode(true)); //do not use reinterpret_cast here:
																   //cloneNode return value is DOMDocument pointer yet
		docClone->setDocumentURI( doc->getDocumentURI());
	}
	catch(...)
	{
	}
	return reinterpret_cast<VXMLDOMDocumentRef>(docClone);
}



	// clone the specified DOMDocument node 
//	static VXMLDOMDocumentRef  CloneNodeDocument( VXMLDOMDocumentRef inNode);


void  IXMLDomWrapper::ReleaseNode( VXMLDOMNodeRef inNode)
{
	try
	{
		xercesc::DOMNode *node = reinterpret_cast<xercesc::DOMNode *>( inNode);
		node->release();
	}
	catch(...)
	{
	}
}


#define NO_MEM_LEAK_ON_TRANSCODE 150

/** copy xercesc::MemBufFormatTarget object buffer to the destination blob 
	and consolidate line endings (optional)
@param inBlobDst
	destination blob
@param inOffset 
	offset in destination blob from which to start writing
@param outSize
	count of bytes written to destination blob
@param inBufFormatSrc
	opaque reference on xerces::MemBufFormatTarget * (source buffer)
@param inEncodingSrc
	source buffer encoding
@param inConsolidateLF
	true: line endings are consolidated - converted to platform-specific line endings (default)
@remarks
	this method can be useful to convert not conforming line endings 
	to platform-specific line endings
*/
bool IXMLDomWrapper::CopyMemBufFormatTargetToBlob( VBlob *inBlobDst, VIndex inOffset, VSize& outSize, VXMLMemBufFormatTarget_opaque *_inBufFormatSrc, const VString& inEncodingSrc, bool inConsolidateLF)
{
	MemBufFormatTarget *inBufFormatSrc = (MemBufFormatTarget *)_inBufFormatSrc;

	VError error = VE_OK;
	uBYTE *pRawText		= (uBYTE *)inBufFormatSrc->getRawBuffer();
	VSize sizeRawText	= inBufFormatSrc->getLen(); 
	bool bFreeRawText	= false;

	//consolidate line endings  
	if (inConsolidateLF)
	{
		bool encodingUTF8 = inEncodingSrc.EqualToString( CVSTR("UTF-8"), false);
		bool encodingUTF16 = false;
		bool encodingUTF32 = false;
		if (!encodingUTF8)
		{
			encodingUTF16 = inEncodingSrc.EqualToString( CVSTR("UTF-16"), false);
			if (!encodingUTF16)
				encodingUTF32 = inEncodingSrc.EqualToString( CVSTR("UTF-32"), false) 
								|| 
								inEncodingSrc.EqualToString( CVSTR("UCS4"), false)
								|| 
								inEncodingSrc.EqualToString( CVSTR("UCS-4"), false)
								|| 
								inEncodingSrc.EqualToString( CVSTR("UCS_4"), false);
		}

		try
		{
			if (encodingUTF8)
			{
				const uBYTE *pCar = reinterpret_cast<const uBYTE *>(inBufFormatSrc->getRawBuffer());
				VIndex size = inBufFormatSrc->getLen();

				pRawText	= new uBYTE[size*2];
				sizeRawText = 0;
				bFreeRawText= true;
				
				uBYTE *pCarDst = pRawText;

				int iCar = 0;
				for (; iCar < size; iCar++, pCar++)
				{
					uBYTE curCar = *pCar;
					//first consolidate mixed line endings to Unix line ending = 0x0A
					if (curCar == 0x0D)
					{
						if (iCar+1 < size)
						{
							if (*(pCar+1) == 0x0A)
								continue;
						}
						curCar = 0x0A;
					}
					//now convert Unix line ending to platform-specific
					if (curCar == 0x0A)
					{
	#if VERSIONMAC
						//Mac OS conforming line endings
						*pCarDst++ = 0x0D;
						sizeRawText += 1;
	#else				
						//Windows OS conforming line endings
						*pCarDst++ = 0x0D;
						*pCarDst++ = 0x0A;
						sizeRawText += 2;
	#endif
					}
					else 
					{
						*pCarDst++ = *pCar;
						sizeRawText++;
					}
				}
			}
			else if (encodingUTF16)
			{
				const uWORD *pCar = reinterpret_cast<const uWORD *>(inBufFormatSrc->getRawBuffer());
				VIndex size = inBufFormatSrc->getLen()/sizeof(uWORD);

				pRawText	= new uBYTE[size*2*sizeof(uWORD)];
				sizeRawText = 0;
				bFreeRawText= true;
				
				uWORD *pCarDst = (uWORD *)pRawText;

				int iCar = 0;
				for (; iCar < size; iCar++, pCar++)
				{
					uWORD curCar = *pCar;
					//first consolidate mixed line endings to Unix line ending = 0x0A
					if (curCar == 0x0D)
					{
						if (iCar+1 < size)
						{
							if (*(pCar+1) == 0x0A)
								continue;
						}
						curCar = 0x0A;
					}
					//now convert Unix line ending to platform-specific
					if (curCar == 0x0A)
					{
	#if VERSIONMAC
						*pCarDst++ = 0x0D;
						sizeRawText += 1;
	#else
						*pCarDst++ = 0x0D;
						*pCarDst++ = 0x0A;
						sizeRawText += 2;
	#endif
					}
					else 
					{
						*pCarDst++ = *pCar;
						sizeRawText++;
					}
				}
				sizeRawText *= sizeof(uWORD); //because UTF16
			}
			else if (encodingUTF32)
			{
				const uLONG *pCar = reinterpret_cast<const uLONG *>(inBufFormatSrc->getRawBuffer());
				VIndex size = inBufFormatSrc->getLen()/sizeof(uLONG);

				pRawText	= new uBYTE[size*2*sizeof(uLONG)];
				sizeRawText = 0;
				bFreeRawText= true;
				
				uLONG *pCarDst = (uLONG *)pRawText;

				int iCar = 0;
				for (; iCar < size; iCar++, pCar++)
				{
					uLONG curCar = *pCar;
					//first consolidate mixed line endings to Unix line ending = 0x0A
					if (curCar == 0x0D)
					{
						if (iCar+1 < size)
						{
							if (*(pCar+1) == 0x0A)
								continue;
						}
						curCar = 0x0A;
					}
					//now convert Unix line ending to platform-specific
					if (curCar == 0x0A)
					{
	#if VERSIONMAC
						*pCarDst++ = 0x0D;
						sizeRawText += 1;
	#else	//Windows
						*pCarDst++ = 0x0D;
						*pCarDst++ = 0x0A;
						sizeRawText += 2;
	#endif
					}
					else 
					{
						*pCarDst++ = *pCar;
						sizeRawText++;
					}
				}
				sizeRawText *= sizeof(uLONG); //because UTF32
			}
		}
		catch(...)
		{
			vThrowError(VE_ACCESS_DENIED);
			error = VE_ACCESS_DENIED;
		}
	}
	if (error == VE_OK)
		error = inBlobDst->PutData( 
							(const void *)pRawText, 
							(VSize)sizeRawText, 
							inOffset);
	
	if (bFreeRawText)
		delete [] pRawText;

	if (error == VE_OK)
	{
		outSize = sizeRawText;
		return true;
	}
	else
	{
		outSize = 0;
		return false;
	}
}



//serialize node to the specified blob
bool _SerializeToBlob( xercesc::DOMNode *inNode, 
					   VBlob *inBlob, VIndex inOffset, VSize& outSize, bool inPrettyPrint = true)
{
	assert(inNode != NULL);
	assert(inBlob != NULL);

	xercesc::DOMDocument *doc =	(inNode->getNodeType() == xercesc::DOMNode::DOCUMENT_NODE)?
								static_cast<xercesc::DOMDocument*>(inNode) : inNode->getOwnerDocument();

	outSize = 0;
	bool bOK = true;

	XMLCh tempStr[NO_MEM_LEAK_ON_TRANSCODE+1];
	XMLCh tempStr2[NO_MEM_LEAK_ON_TRANSCODE+1];

	xercesc::XMLString::transcode("LS", tempStr,NO_MEM_LEAK_ON_TRANSCODE);

	xercesc::DOMImplementation  *impl = xercesc::DOMImplementationRegistry::getDOMImplementation(tempStr);

	if (impl && inNode)
	{
#ifdef XERCES_3_0_1
        xercesc::DOMLSSerializer *theSerializer = ((xercesc::DOMImplementationLS*)impl)->createLSSerializer();

		if (theSerializer)
		{
            DOMConfiguration* cfg=theSerializer->getDomConfig();

            cfg->setParameter(xercesc::XMLUni::fgDOMWRTFormatPrettyPrint, inPrettyPrint);
            
			// set the byte-order-mark feature      
            cfg->setParameter(xercesc::XMLUni::fgDOMWRTBOM, true);
            
            xercesc::DOMLSOutput *output = ((xercesc::DOMImplementationLS*)impl)->createLSOutput();   
			if (output)
			{
				if ((!doc) || (!doc->getXmlEncoding()))
				{
					VString l_encoding("UTF-8");
					output->setEncoding((XMLCh*)l_encoding.GetCPointer());
				}
				else
				{
					output->setEncoding(doc->getXmlEncoding());
				}

				VString encoding(output->getEncoding());

				xercesc::XMLString::transcode("\n",tempStr,NO_MEM_LEAK_ON_TRANSCODE);
				theSerializer->setNewLine(tempStr); 

				xercesc::MemBufFormatTarget *myFormTarget = new xercesc::MemBufFormatTarget();

				if (myFormTarget)
				{
					output->setByteStream(myFormTarget);
					theSerializer->write(inNode, output);


					bOK = IXMLDomWrapper::CopyMemBufFormatTargetToBlob( 
										inBlob, inOffset, outSize, 
										(VXMLMemBufFormatTarget_opaque *)myFormTarget, encoding, 
										true);
					delete myFormTarget;
				}
				output->release();
			}
			else
				bOK = false;
			theSerializer->release();
		}	else
			bOK = false;
#else
			xercesc::DOMWriter *theSerializer = ((xercesc::DOMImplementationLS*)impl)->createDOMWriter();
		if (theSerializer)
		{
			if (theSerializer->canSetFeature(xercesc::XMLUni::fgDOMWRTFormatPrettyPrint, true))
				theSerializer->setFeature(xercesc::XMLUni::fgDOMWRTFormatPrettyPrint, inPrettyPrint);

			// set the byte-order-mark feature      
			if (theSerializer->canSetFeature(xercesc::XMLUni::fgDOMWRTBOM, true))
				theSerializer->setFeature(xercesc::XMLUni::fgDOMWRTBOM, true);

 			if ((!doc) || (!doc->getEncoding()))
			{
				VString l_encoding("UTF-8");
				theSerializer->setEncoding ((XMLCh*)l_encoding.GetCPointer());
			}
			else
			{
				theSerializer->setEncoding(doc->getEncoding());
			}

			VString encoding( theSerializer->getEncoding());

			xercesc::XMLString::transcode("\n",tempStr,NO_MEM_LEAK_ON_TRANSCODE);
			theSerializer->setNewLine(tempStr); 

			xercesc::MemBufFormatTarget *myFormTarget = new xercesc::MemBufFormatTarget();

			if (myFormTarget)
			{
				theSerializer->writeNode( myFormTarget, *inNode);	

				bOK = IXMLDomWrapper::CopyMemBufFormatTargetToBlob( 
									inBlob, inOffset, outSize, 
									(VXMLMemBufFormatTarget_opaque *)myFormTarget, encoding, 
									true);
				delete myFormTarget;
			}
			theSerializer->release();
		}	else
			return false;
#endif
	}	else
		bOK = false;
	return bOK;
}

// serialize the specified DOM tree to a blob
bool IXMLDomWrapper::SerializeToBlob( VXMLDOMNodeRef inNode, VBlob *inBlob, VIndex inOffset, VSize& outSize, bool inPrettyPrint)
{
	return _SerializeToBlob( reinterpret_cast<xercesc::DOMNode *>(inNode), inBlob, inOffset, outSize, inPrettyPrint);
}


bool IXMLDomWrapper::SerializeToFile( VXMLDOMNodeRef inNode, VFileStream& ioFileStream, VSize& outSize, bool inPrettyPrint)
{
	//write to temp blob
	XBOX::VBlobWithPtr blob;
	bool ok = SerializeToBlob( inNode, &blob, 0, outSize, inPrettyPrint);
	if (ok)
	{
		//serialize to file stream
		XBOX::VError error = ioFileStream.PutData( blob.GetDataPtr(), blob.GetSize());
		ok = (error == VE_OK);
	}

	return ok;
}

bool IXMLDomWrapper::WriteToFile( VXMLDOMNodeRef inNode, const VFile& inFile, bool inPrettyPrint)
{
	//write to temp blob
	VSize sizeBlob;
	XBOX::VBlobWithPtr blob;
	bool ok = SerializeToBlob( inNode, &blob, 0, sizeBlob, inPrettyPrint);
	if (ok)
	{
		XBOX::VError error = VE_OK;

		//write temp blob to file
		if (inFile.Exists())
			inFile.Delete();
		if (!inFile.Exists())
			error = inFile.Create( false );
		if (error == VE_OK)
		{
			XBOX::VFileStream stream( const_cast<VFile *>(&inFile) );
			error = stream.SetCharSet( XBOX::VTC_UTF_8 );
			stream.SetCarriageReturnMode(XBOX::eCRM_NONE);
			error = stream.OpenWriting();
			if (error == VE_OK)
			{
				error = stream.PutData( blob.GetDataPtr(), blob.GetSize());
				error = stream.CloseWriting();
			}
		}
		ok = (error == VE_OK);
	}

	return ok;
}

#ifdef XERCES_3_0_1
/** workaround for Xerces 3.0.1 bug (does not properly propagate default namespace URI) */
VXMLDOMElementRef IXMLDomWrapper::CreateElement( VXMLDOMDocumentRef inDocument, VXMLDOMElementRef inNodeParent, const UniChar *inName)
{
	xercesc::DOMDocument *doc = reinterpret_cast<xercesc::DOMDocument *>( inDocument);
	xercesc::DOMElement *nodeParent = reinterpret_cast<xercesc::DOMElement *>( inNodeParent);

	//fixed ACI0076075: lookup proper prefix namespace
	XBOX::VString prefix( inName);
	XBOX::VIndex semicolonPos = prefix.FindUniChar(':');
	if (semicolonPos >= 1)
		prefix.Truncate( semicolonPos-1);
	else
		prefix.SetEmpty();
	const XMLCh *uri = nodeParent ? nodeParent->lookupNamespaceURI( prefix.IsEmpty() ? NULL : prefix.GetCPointer()) : NULL;
	xercesc::DOMElement *elem = NULL;
	if (uri)
	{
		elem = doc->createElementNS( uri, inName);
		//JQ 10/12/2013: fixed ACI0084794 
		//it seems that createElementNS is buggy as prefix is made sometimes with prefix+localname & not only with prefix
		//so here we force proper prefix setting 
		if (elem)
			elem->setPrefix( prefix.GetCPointer());
	}
	else
		elem = doc->createElement( inName);
	return (VXMLDOMElementRef)elem;
}
#endif

// add element node 
VXMLDOMElementRef  IXMLDomWrapper::AddNodeElement( VXMLDOMElementRef inNodeParent, const VString& inName)
{
	xercesc::DOMElement *nodeParent = reinterpret_cast<xercesc::DOMElement *>( inNodeParent);
	xercesc::DOMDocument *doc = nodeParent->getOwnerDocument();
	if (!testAssert(doc))
		return NULL;

#ifdef XERCES_3_0_1
	//JQ 19/05/2010: workaround for the default namespace URI bug
	xercesc::DOMElement *elem = reinterpret_cast<xercesc::DOMElement *>(CreateElement( (VXMLDOMDocumentRef)doc, inNodeParent, inName.GetCPointer()));
#else
	xercesc::DOMElement *elem = doc->createElement( inName.GetCPointer());
#endif
	xbox_assert(elem != 0);
	nodeParent->appendChild( elem);

	return ((VXMLDOMElementRef)(elem));
}


// add element node with the specified value as its unique child text node
VXMLDOMElementRef  IXMLDomWrapper::AddNodeElement( VXMLDOMElementRef inNodeParent, const VString& inName, const VDuration& inValue, XMLStringOptions inOptions)
{
	VString sval;
	inValue.GetXMLString( sval, inOptions);
	return AddNodeElement( inNodeParent, inName, sval);
}


// add element node with the specified value as its unique child text node
VXMLDOMElementRef  IXMLDomWrapper::AddNodeElement( VXMLDOMElementRef inNodeParent, const VString& inName, const VValueSingle * inValue)
{
	switch (inValue->GetValueKind())
	{
	case VK_TIME:
		return AddNodeElement( inNodeParent, inName, *(static_cast<const VTime *>(inValue)));
		break;
	case VK_DURATION:
		return AddNodeElement( inNodeParent, inName, *(static_cast<const VDuration *>(inValue)));
		break;
	case VK_REAL:
		return AddNodeElement( inNodeParent, inName, (static_cast<const VReal *>(inValue))->GetReal());
		break;
	case VK_STRING:
		{
			XBOX::VString sValue;
			(static_cast<const VString *>(inValue))->GetString( sValue);
			return AddNodeElement( inNodeParent, inName, sValue);
		}
		break;
	case VK_BOOLEAN:
		return AddNodeElement( inNodeParent, inName, (static_cast<const VBoolean *>(inValue))->GetBoolean());
		break;
	case VK_LONG:
		return AddNodeElement( inNodeParent, inName, (static_cast<const VLong *>(inValue))->GetLong());
		break;
	default:
		xbox_assert(false);
	}
	return NULL;
}


// add element node with the specified value as its unique child text node
VXMLDOMElementRef  IXMLDomWrapper::AddNodeElement( VXMLDOMElementRef inNodeParent, const VString& inName, Boolean inValue)
{
	VBoolean val( inValue);
	VString sval;
	val.GetXMLString( sval, XSO_Default);
	return AddNodeElement( inNodeParent, inName, sval);
}

// add element node with the specified value as its unique child text node
VXMLDOMElementRef  IXMLDomWrapper::AddNodeElement( VXMLDOMElementRef inNodeParent, const VString& inName, sLONG inValue)
{
	VString sval;
	sval.FromLong( inValue);
	return AddNodeElement( inNodeParent, inName, sval);
}



// add element node with the specified value as its unique child text node
VXMLDOMElementRef  IXMLDomWrapper::AddNodeElement( VXMLDOMElementRef inNodeParent, const VString& inName, Real inValue)
{
	VReal val( inValue);
	VString sval;
	val.GetXMLString( sval, XSO_Default);
	return AddNodeElement( inNodeParent, inName, sval);
}

// add element node with the specified value as its unique child text node
VXMLDOMElementRef  IXMLDomWrapper::AddNodeElement( VXMLDOMElementRef inNodeParent, const VString& inName, const VTime& inValue, XMLStringOptions inOptions)
{
	VString sval;
	inValue.GetXMLString( sval, inOptions);
	return AddNodeElement( inNodeParent, inName, sval);
}

// add element node with the specified value as its unique child text node
VXMLDOMElementRef  IXMLDomWrapper::AddNodeElement( VXMLDOMElementRef inNodeParent, const VString& inName, const VString& inValue)
{
	xercesc::DOMElement *nodeParent = reinterpret_cast<xercesc::DOMElement *>( inNodeParent);
	xercesc::DOMDocument *doc = nodeParent->getOwnerDocument();
	if (!testAssert(doc))
		return NULL;

#ifdef XERCES_3_0_1
	//JQ 19/05/2010: workaround for the default namespace URI bug
	xercesc::DOMElement *elem = reinterpret_cast<xercesc::DOMElement *>(CreateElement( (VXMLDOMDocumentRef)doc, inNodeParent, inName.GetCPointer()));
#else
	xercesc::DOMElement *elem = doc->createElement( inName.GetCPointer());
#endif
	xbox_assert(elem != 0);
	nodeParent->appendChild( elem);

	xercesc::DOMText *elemText = doc->createTextNode( inValue.GetCPointer()); 
	xbox_assert(elemText != 0);
	elem->appendChild( elemText); 

	return ((VXMLDOMElementRef)(elem));
}




// set element node value (add a DOMText child element if not yet created)
void IXMLDomWrapper::SetNodeElementValue( VXMLDOMElementRef inNodeParent, Boolean inValue)
{
	VBoolean val( inValue);
	VString sval;
	val.GetXMLString( sval, XSO_Default);
	SetNodeElementValue( inNodeParent, sval);
}

// set element node value (add a DOMText child element if not yet created)
void IXMLDomWrapper::SetNodeElementValue( VXMLDOMElementRef inNodeParent, sLONG inValue)
{
	VString sval;
	sval.FromLong( inValue);
	SetNodeElementValue( inNodeParent, sval);
}



// set element node value (add a DOMText child element if not yet created)
void IXMLDomWrapper::SetNodeElementValue( VXMLDOMElementRef inNodeParent, Real inValue)
{
	VReal val( inValue);
	VString sval;
	val.GetXMLString( sval, XSO_Default);
	SetNodeElementValue( inNodeParent, sval);
}

// set element node value (add a DOMText child element if not yet created)
void IXMLDomWrapper::SetNodeElementValue( VXMLDOMElementRef inNodeParent, const VTime& inValue, XMLStringOptions inOptions)
{
	VString sval;
	inValue.GetXMLString( sval, inOptions);
	return SetNodeElementValue( inNodeParent, sval);
}

// set element node value (add a DOMText child element if not yet created)
void IXMLDomWrapper::SetNodeElementValue( VXMLDOMElementRef inNodeParent, const VDuration& inValue, XMLStringOptions inOptions)
{
	VString sval;
	inValue.GetXMLString( sval, inOptions);
	return SetNodeElementValue( inNodeParent, sval);
}


// set element node value (add a DOMText child element if not yet created)
void IXMLDomWrapper::SetNodeElementValue( VXMLDOMElementRef inNodeParent, const VString& inValue)
{
	xercesc::DOMElement *nodeParent = reinterpret_cast<xercesc::DOMElement *>( inNodeParent);
	if (!testAssert(nodeParent))
		return;

	xercesc::DOMText *elemText = NULL;
	if (nodeParent->hasChildNodes() 
		&& 
		(nodeParent->getFirstChild()->getNodeType() == xercesc::DOMNode::TEXT_NODE))
	{
		//update existing child text node

		elemText = static_cast<xercesc::DOMText *>(nodeParent->getFirstChild());
		elemText->setNodeValue( inValue.GetCPointer());
	}	else
	{
		xercesc::DOMDocument *doc = nodeParent->getOwnerDocument();
		if (!testAssert(doc))
			return;

		//first remove existing childs
		xercesc::DOMNode* current = nodeParent->getFirstChild();
        while (current != NULL) 
        {
            nodeParent->removeChild(current);
            current = nodeParent->getFirstChild();
        }

		//then create unique child text node
		elemText = doc->createTextNode( inValue.GetCPointer()); 
		xbox_assert(elemText != 0);
		nodeParent->appendChild( elemText); 
	}
}


//return node element value of the specified type
Boolean IXMLDomWrapper::GetNodeElementValueBoolean( VXMLDOMElementRef inNodeParent)
{
	XBOX::VString sval;
	GetNodeElementValueString( inNodeParent, sval);
	VBoolean val(0);
	val.FromXMLString( sval);
	return val.GetBoolean();
}

//return node element value of the specified type
sLONG IXMLDomWrapper::GetNodeElementValueLong( VXMLDOMElementRef inNodeParent)
{
	XBOX::VString sval;
	GetNodeElementValueString( inNodeParent, sval);
	VLong val(0);
	val.FromString( sval);
	return val.GetLong();
}

//return node element value of the specified type
Real IXMLDomWrapper::GetNodeElementValueReal( VXMLDOMElementRef inNodeParent)
{
	XBOX::VString sval;
	GetNodeElementValueString( inNodeParent, sval);
	VReal val(0);
	val.FromString( sval);
	return val.GetReal();
}


//return node element value of the specified type
bool IXMLDomWrapper::GetNodeElementValueTime( VXMLDOMElementRef inNodeParent, VTime& outValue)
{
	XBOX::VString sval;
	GetNodeElementValueString( inNodeParent, sval);

	return outValue.FromXMLString( sval);
}

//return node element value of the specified type
bool IXMLDomWrapper::GetNodeElementValueDuration( VXMLDOMElementRef inNodeParent, VDuration& outValue)
{
	XBOX::VString sval;
	GetNodeElementValueString( inNodeParent, sval);

	return outValue.FromXMLString( sval);
}



//return node element value of the specified type
void IXMLDomWrapper::GetNodeElementValueString( VXMLDOMElementRef inNodeParent, VString& outValue)
{
	xercesc::DOMElement *node = reinterpret_cast<xercesc::DOMElement *>( inNodeParent);
	xercesc::DOMNode *nodeChild = node->getFirstChild();
	if (testAssert(nodeChild && (nodeChild->getNodeType() == xercesc::DOMNode::TEXT_NODE)))
	{
		XBOX::VString sval( (static_cast<xercesc::DOMText *>(nodeChild))->getNodeValue());
		outValue = sval;
	}	else
		outValue = CVSTR("");
}


/** return element with the specified ID */
VXMLDOMElementRef IXMLDomWrapper::FindNodeElementByID( VXMLDOMNodeRef inNode, const VString& inID)
{
	xercesc::DOMNode *node = reinterpret_cast<xercesc::DOMNode *>( inNode);
	if (!node)
		return NULL;
	xercesc::DOMDocument *nodeDocument = (node->getNodeType() == xercesc::DOMNode::DOCUMENT_NODE)?
				   static_cast<xercesc::DOMDocument*>(node) : node->getOwnerDocument();
	if (nodeDocument)
	{
		try
		{
			return (VXMLDOMElementRef) nodeDocument->getElementById( inID.GetCPointer());
		}
		catch(...)
		{
			return NULL;
		}
	}
	else
		return NULL;
}

/** return root element  */
VXMLDOMElementRef IXMLDomWrapper::GetRootNodeElement( VXMLDOMNodeRef inNode)
{
	xercesc::DOMNode *node = reinterpret_cast<xercesc::DOMNode *>( inNode);
	if (!node)
		return NULL;
	xercesc::DOMDocument *nodeDocument = (node->getNodeType() == xercesc::DOMNode::DOCUMENT_NODE)?
				   static_cast<xercesc::DOMDocument*>(node) : node->getOwnerDocument();
	if (nodeDocument)
	{
		try
		{
			return (VXMLDOMElementRef) nodeDocument->getDocumentElement();
		}
		catch(...)
		{
			return NULL;
		}
	}
	else
		return NULL;
}


/** set element attribute */
bool IXMLDomWrapper::SetNodeElementAttribute( VXMLDOMElementRef inNodeElement, const VString& inName, const VString& inValue, bool inRemoveIfValueEmpty, bool *outIsDirtyMapID)
{
	xercesc::DOMElement *element =  reinterpret_cast<xercesc::DOMElement *>(inNodeElement);
	if (outIsDirtyMapID)
		*outIsDirtyMapID = false;
	if (!element)
		return false;

	try
	{
		//fixed ACI0076075: Xerces DOM2 compliancy: use NS-aware method in order to properly manage prefixed names
		//					(otherwise Xerces treats prefixed names as local names & uri lookup per prefix fails)

		bool done = false;
		bool hasNSPrefix = false;
		bool isRemoved = false;
		XBOX::VIndex afterPrefixPos = inName.FindUniChar(':');
		const XMLCh *uri = NULL;
		if (afterPrefixPos >= 2)
		{
			XBOX::VString prefix( inName);
			prefix.Truncate( afterPrefixPos-1);
			uri = element->lookupNamespaceURI( prefix.GetCPointer());
			if (!uri)
			{
				//ensure we pass DOM2 compliant URI for default namespaces (to prevent DOM2 exception)
				static VString uriXML("http://www.w3.org/XML/1998/namespace");
				static VString uriXMLNS("http://www.w3.org/2000/xmlns/");
				if (prefix.EqualToString("xml",true))
					uri = uriXML.GetCPointer();
				else if (prefix.EqualToString("xmlns",true))
					uri = uriXMLNS.GetCPointer();
			}
			if (uri)
			{
				if (!inValue.IsEmpty())
					element->setAttributeNS( uri, inName.GetCPointer(), inValue.GetCPointer());
				else if (inRemoveIfValueEmpty)
				{
					element->removeAttributeNS( uri, inName.GetCPointer());
					isRemoved = true;
				}
				else
					// m.c, ACI0044878
					element->setAttributeNS( uri, inName.GetCPointer(), (XMLCh*) "\0\0");
				hasNSPrefix = true;
				done = true;
			}
		}
		if (!done)
		{
			if (!inValue.IsEmpty())
				element->setAttribute( inName.GetCPointer(), inValue.GetCPointer());
			else if (inRemoveIfValueEmpty)
			{
				element->removeAttribute( inName.GetCPointer());
				isRemoved = true;
			}
			else
				// m.c, ACI0044878
				element->setAttribute( inName.GetCPointer(), (XMLCh*) "\0\0");
		}
		//JQ 28/04/2011: ensure xerces will consider this attribute as a ID (otherwise getElementById would fail)
		if (outIsDirtyMapID || !isRemoved)
		{
			if (hasNSPrefix)
			{
				VString nameLocal( inName.GetCPointer()+afterPrefixPos);
				if (nameLocal.EqualToUSASCIICString("id"))
				{
					if (!isRemoved)
						element->setIdAttributeNS( uri, nameLocal.GetCPointer(), true);
					if (outIsDirtyMapID)
						*outIsDirtyMapID = true;
				}
			}
			else
				if (inName.EqualToUSASCIICString("id"))
				{
					if (!isRemoved)
						element->setIdAttribute( inName.GetCPointer(), true);
					if (outIsDirtyMapID)
						*outIsDirtyMapID = true;
				}
		}
	}
	catch(...)
	{
		//probably invalid attribute name
		return false;
	}
	return true;
}

END_TOOLBOX_NAMESPACE
